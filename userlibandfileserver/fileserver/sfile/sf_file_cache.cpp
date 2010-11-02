// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// f32\sfile\sf_file_cache.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32std_private.h>
#include "sf_std.h"
#include <e32uid.h>
#include <e32wins.h>
#include <f32file.h>
#include <hal.h>
#include "sf_file_cache.h"
#include "sf_cache_man.h"
#include "sf_cache_client.h"
#include "sf_file_cache_defs.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_file_cacheTraces.h"
#endif


// disables flushing of stale cachelines before each write
#define LAZY_WRITE

// call SetSizeL() to pre-allocate FAT clusters before flushing dirty data
#define SETSIZE_BEFORE_WRITE	

enum TFileCacheFault
	{
	ECacheSettingsInitFailed,
	ECacheSettingsNotFound,
	ECacheSettingGetFailed,
	ECacheCodeFault,
	ECacheBadOperationIndex,
	ENoFileCache,
	EFileAlreadyClosed,
	EClosingDirtyFile,
	ECompletingWriteWithDataRemaining,
	EPosBeyondSize,
	EMsgAlreadyCompleted,
	EBadRetCode,
	EInternalError,
	ERequestUncancelled,
	ELockAlreadyOpen,
	EBadSegmentCount,
	EReNewingOpenCache,
	EClosingUnNamedFile,
	EFileNameAlreadyOwned,
	EClosedFileHasNoName,
	EReOpeningUnNamedFile,
	EStartingDirtyAWrongStage,
	EUnexpectedReNewLFailure,
	EDirtyDataOwnerNull,
	EFlushingWithSessionNull,
	EFlushingWithAllocatedRequest,
	};


LOCAL_C void Fault(TFileCacheFault aFault)
//
// Report a fault in the file cache
//
	{
	User::Panic(_L("FSFILECACHE"), aFault);
	}

const TInt KMinSequentialReadsBeforeReadAhead = 3;

#ifdef DOUBLE_BUFFERED_WRITING
const TInt KMinSequentialAppendsBeforeFlush = 3;
#endif

//************************************
// CFileCache
//************************************

inline TBool ReadCachingEnabled(CFileShare& aShare)
	{
	TUint mode = aShare.iMode;
	
	TInt fileCacheFlags = TFileCacheSettings::Flags(aShare.File().Drive().DriveNumber());

	if (((mode & EFileReadBuffered) && (fileCacheFlags & (EFileCacheReadEnabled | EFileCacheReadOn))) ||
		(((mode & EFileReadDirectIO) == 0) && (fileCacheFlags & EFileCacheReadOn)))
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

inline TBool WriteCachingEnabled(CFileShare& aShare)
	{
	TUint mode = aShare.iMode;
	
	TInt fileCacheFlags = TFileCacheSettings::Flags(aShare.File().Drive().DriveNumber());

	if ((mode & EFileWrite) == 0)
		{
		return EFalse;
		}
	else if (((mode & EFileWriteBuffered) && (fileCacheFlags & (EFileCacheWriteEnabled | EFileCacheWriteOn))) ||
		(((mode & EFileWriteDirectIO) == 0) && (fileCacheFlags & EFileCacheWriteOn)))
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

void CFileCache::SetFileCacheFlags(CFileShare& aShare)
	{
	TInt fileCacheFlags = TFileCacheSettings::Flags(iDriveNum);

	TUint& mode = aShare.iMode;

	// enable/disable read ahead
	if (((mode & EFileReadAheadOn) && (fileCacheFlags & (EFileCacheReadAheadEnabled | EFileCacheReadAheadOn))) ||
		 (((mode & EFileReadAheadOff) == 0) && (fileCacheFlags & EFileCacheReadAheadOn)))
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheReadAhead ENABLED"));
		mode|= EFileReadAheadOn;
		}
	else
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheReadAhead disabled"));
		mode&= ~EFileReadAheadOn;
		}

	// enable/disable read caching 
	if (ReadCachingEnabled(aShare))
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheRead ENABLED"));
		mode|= EFileReadBuffered;
		}
	else
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheRead disabled"));
		// if read caching is off, turn off read-ahead too
		mode&= ~(EFileReadBuffered | EFileReadAheadOn);
		}

	// enable/disable write caching 
	if (WriteCachingEnabled(aShare))
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheWrite ENABLED"));
		mode|= EFileWriteBuffered;
		}
	else
		{
		__CACHE_PRINT(_L("CACHEFILE: EFileCacheWrite disabled"));
		mode&= ~EFileWriteBuffered;
		}

	}

void CFileCache::ConstructL(CFileShare& aShare)
	{
	iDrive = &aShare.File().Drive();
	iDriveNum = iDrive->DriveNumber();
	iMount=&iDrive->CurrentMount();

	DoInitL(iDriveNum);	
	

	CCacheManager* manager = CCacheManagerFactory::CacheManager();

	iCacheClient = manager->CreateClientL();
	manager->RegisterClient(*iCacheClient);
	

	TInt segmentSize = SegmentSize();
	TInt segmentSizeMask = I64LOW(SegmentSizeMask());
	// Get file cache size
	iCacheSize			= TFileCacheSettings::CacheSize(iDriveNum);
	// must be at least one segment
	iCacheSize			= Max(iCacheSize, segmentSize);
	// round up to nearest whole segment
	iCacheSize			= (iCacheSize + segmentSize - 1) & segmentSizeMask;

	// Get max read-ahead length
	iMaxReadAheadLen	= TFileCacheSettings::MaxReadAheadLen(iDriveNum);
	// must be at least one segment
	iMaxReadAheadLen	= Max(iMaxReadAheadLen, segmentSize);
	// round up to nearest whole segment
	iMaxReadAheadLen	= (iMaxReadAheadLen + segmentSize - 1) & segmentSizeMask;
	// read-ahead should not be greater than the cache size minus one segment
	iMaxReadAheadLen	= Min(iMaxReadAheadLen, iCacheSize - segmentSize);
	// ... or greater than one cacheline (128K should be enough !)
	iMaxReadAheadLen	= Min(iMaxReadAheadLen, iCacheClient->CacheLineSize());

	iFileCacheReadAsync = TFileCacheSettings::FileCacheReadAsync(iDriveNum);

	iClosedFileKeepAliveTime	= TFileCacheSettings::ClosedFileKeepAliveTime(iDriveNum);
	iDirtyDataFlushTime			= TFileCacheSettings::DirtyDataFlushTime(iDriveNum);

	// Calculate max number of segments to cache
	TInt maxSegmentsToCache = iCacheSize >> SegmentSizeLog2();

	__CACHE_PRINT1(_L("CACHEFILE: maxSegmentsToCache %d"), maxSegmentsToCache);

	iCacheClient->SetMaxSegments(maxSegmentsToCache);

	CFileCache* fileCache = ReNewL(aShare);
	__ASSERT_ALWAYS(fileCache != NULL, Fault(EUnexpectedReNewLFailure));
	}

CFileCache* CFileCache::NewL(CFileShare& aShare)
	{
	__CACHE_PRINT(_L("CACHEFILE: CFileCache::NewL()"));

	CFileCB* file = &aShare.File();
	if ((CCacheManagerFactory::CacheManager() == NULL) ||
		(!ReadCachingEnabled(aShare) && !WriteCachingEnabled(aShare)) ||
		(FsThreadManager::IsDriveSync(file->Drive().DriveNumber(),EFalse)))
		return NULL;

	CFileCache* fileCache = new(ELeave) CFileCache();
	CleanupClosePushL(*fileCache);
	fileCache->ConstructL(aShare);
	CleanupStack::Pop(1, fileCache);

	return fileCache;
	}

CFileCache* CFileCache::ReNewL(CFileShare& aShare)
	{
	__CACHE_PRINT(_L("CACHEFILE: CFileCache::ReNewL()"));

	// check not already open i.e. attached to a CFileCB
	__ASSERT_DEBUG(iFileCB == NULL, Fault(EReNewingOpenCache));

	// make sure the drive thread exists (the mount may have been mounted
	// synchronously since the drive was last open)
	const TInt r = FsThreadManager::GetDriveThread(iDriveNum, &iDriveThread);
	if ((r!= KErrNone) || !iDriveThread)
		return NULL;

	// if re-opening in DirectIo mode, destroy the file cache 
	if (!ReadCachingEnabled(aShare) && !WriteCachingEnabled(aShare))
		{
		Close();
		return NULL;
		}

	SetFileCacheFlags(aShare);

	// assign ownership of this object to aFileCB before any leaves occur
	iFileCB = &aShare.File();
	iFileCB->iBody->iFileCache = this;
	
	__ASSERT_DEBUG(iLock.Handle() == KNullHandle, Fault(ELockAlreadyOpen));
	User::LeaveIfError(iLock.CreateLocal());

	// delete the file name to save heap space (it's only needed 
	// while the file is on the closed queue so that it can be matched)
	delete iFileNameF;
	iFileNameF = NULL;

	return this;
	}

void CFileCache::Init(CFileShare& aShare)
	{
	SetFileCacheFlags(aShare);
	}

CFileCache::CFileCache() : 
	iClosedTimer(ClosedTimerEvent, this),
	iDirtyTimer(DirtyTimerEvent, this)
	{
	}

CFileCache::~CFileCache()
	{
	iLock.Close();

	// stop the owning CFileCB from pointing to an about-to-be deleted object
	if (iFileCB && iFileCB->iBody)
		iFileCB->iBody->iFileCache = NULL;

	CCacheManagerFactory::CacheManager()->DeregisterClient(*iCacheClient);

	delete iCacheClient;

	delete iFileNameF;

	OstTraceExt2(TRACE_FILECACHE, FILECACHE_DELETE, "fileCache %x fileCB %x", (TUint) this, (TUint) iFileCB);
	}


TInt CFileCache::ClosedTimerEvent(TAny* aFileCache)
	{
	TClosedFileUtils::Remove((CFileCache*) aFileCache);
	return KErrNone;
	}

TInt CFileCache::DirtyTimerEvent(TAny* aFileCache)
	{
	OstTrace1(TRACE_FILECACHE, FILECACHE_DIRTY_DATA_TIMER_EXPIRY, "fileCache %x", (TUint) aFileCache);

	// Cannot report errors here
	// coverity [unchecked_value]
	(void)((CFileCache*) aFileCache)->FlushDirty();

	return KErrNone;
	}



void CFileCache::Close()
	{
	__CACHE_PRINT1(_L("CFileCache::Close() 0x%x"),this);

	TInt r = KErrNone;

#ifdef _DEBUG
	if (iCacheClient)	// NB Object may not have been fully constructed
		{
		TInt64  pos;
		TUint8* addr;
		__ASSERT_DEBUG(((iCacheClient->FindFirstDirtySegment(pos, addr)) == 0), Fault(EClosingDirtyFile));
		__ASSERT_DEBUG(!iDirtyDataOwner, Fault(EClosingDirtyFile));
		}
#endif

	// there should be no dirty data (see assert above) but clean up just in case
	MarkFileClean();
		
	__CACHE_PRINT2(_L("CFileCache::Close() iFileCB %08X IsClosed %d"), 
		iFileCB, TClosedFileUtils::IsClosed(this));
	// if not already closed move to closed file queue
	if (iFileCB != NULL && 
		!iFileCB->DeleteOnClose() &&
		!TClosedFileUtils::IsClosed(this) && 
		IsDriveThread())
		{
		OstTrace1(TRACE_FILECACHE, FILECACHE_MODE_TO_CLOSEQ, "Moving to close queue, fileCache %x", (TUint) this);

		// add to ClosedFiles container
		__CACHE_PRINT1(_L("CLOSEDFILES: Adding %S\n"), &iFileCB->FileNameF() );
		TRAP(r, TClosedFileUtils::AddL(this, ETrue));

		// Acquire ownership of the CFileCB's file name
		__ASSERT_DEBUG(iFileCB->iFileNameF, Fault(EClosingUnNamedFile));
		__ASSERT_DEBUG(iFileNameF == NULL, Fault(EFileNameAlreadyOwned));
		iFileNameF = iFileCB->iFileNameF;
		iNameHash = iFileCB->iNameHash;
		iFileCB->iFileNameF = NULL;

		// remove pointer to owning CFileCB as this is called from CFileCB's destructor
		iFileCB = NULL;

		// Successfully moved file !
		if (r == KErrNone)
			{
			// close the RFastLock object here to prevent OOM kernel tests from failing
			iLock.Close();
			return;
			}
		}

	iClosedTimer.Stop();

	// if already closed, close for good. N.B. CFsObject::DoClose() will 
	// cause it to be removed from the ClosedFiles container
	CFsDispatchObject::Close();
	}


CMountCB& CFileCache::Mount() const
	{
	return *iMount;
	}

CFileCB* CFileCache::FileCB()
	{
	return iFileCB;
	}



TInt CFileCache::SegmentSize() const
	{
	return iCacheClient->SegmentSize();
	}

TInt CFileCache::SegmentSizeLog2() const
	{
	return iCacheClient->SegmentSizeLog2();
	}

TInt64 CFileCache::SegmentSizeMask() const
	{
	return iCacheClient->SegmentSizeMask();
	}

/**
Sets the cached file size  

@param aSize The size of the file.
*/  
void CFileCache::SetSize64(TInt64 aSize)
	{
	__e32_atomic_store_ord64(&iSize64, aSize);
	}

TDrive& CFileCache::Drive() const
	{
	return *iDrive;
	}

TUint32 CFileCache::NameHash() const
	{
	return(iNameHash);
	}

HBufC& CFileCache::FileNameF() const
	{
	__ASSERT_DEBUG(iFileNameF, Fault(EClosedFileHasNoName));
	return(*iFileNameF);
	}




TBool CFileCache::IsDriveThread()
	{
	return FsThreadManager::IsDriveThread(iDriveNum, EFalse);
	}


void CFileCache::ResetReadAhead()
	{
//	iSequentialReads = 0;
	iReadAheadLen = 0;
	iReadAheadPos = 0;
	}

/**
ReadAhead() - 
dispatches a new message to fill the read cache if 
(ReadAheadPos - ShareReadPos) <= (ReadAheadLen)

XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
		^				^
		|----- ReadAheadLen ----------------->
		|				| 
  ShareReadPos		ReadAheadPos	

Every successful read-ahead doubles ReadAheadLen until it reaches the maximum 
(KDefaultFileCacheMaxReadAheadLen). 
*/
void CFileCache::ReadAhead(CFsMessageRequest& aMsgRequest, TUint aMode)
	{
	if (!(aMode & EFileReadAheadOn) ||
		(iSequentialReads < KMinSequentialReadsBeforeReadAhead) ||
		iMaxReadAheadLen == 0)
		return;


	TInt segmentSize = SegmentSize();
	TInt64 segmentSizeMask = SegmentSizeMask();
	
	// if the read-ahead pos has been reset to zero, then the read-ahead 
	// position and length must be re-calculated

	TBool resetting = (iReadAheadPos == 0)?(TBool)ETrue:(TBool)EFalse;
	if (resetting)
		{
		iReadAheadPos = (iLastReadPos + segmentSize - 1) & segmentSizeMask;	

		// ensure read ahead len at least as big as last read
		iReadAheadLen = Max(iReadAheadLen, iLastReadLen);	
			
		// round up to a segment size
		iReadAheadLen = (iReadAheadLen + segmentSize - 1) & (TInt) segmentSizeMask;

		// ensure read ahead len at least as big as 1 segments 
		iReadAheadLen = Max(iReadAheadLen, segmentSize);	
			
		// ensure read ahead len not greater than the maximum read ahead len
		iReadAheadLen = Min(iReadAheadLen, iMaxReadAheadLen);

		iReadAheadRequest = NULL;
		}

	TInt bytesBuffered = (TInt) (iReadAheadPos - iLastReadPos);
	TInt bytesNotBuffered = iMaxReadAheadLen - bytesBuffered;


	// if read-ahead buffer len > current read-ahead len OR
	// read-ahead buffer is more than half full, do nothing
	if ((iReadAheadRequest) ||
		(bytesBuffered > iReadAheadLen) || 
		(bytesBuffered > (iMaxReadAheadLen>>1)) ||
		(iReadAheadPos >= iSize64))
		{
		return;
		}

	// double the read-ahead length - unless this is the first
	if (!resetting)
		iReadAheadLen<<= 1;

	// ensure read ahead len not greater than the free space available in buffer
	iReadAheadLen = Min(iReadAheadLen, bytesNotBuffered);

	// round up to a segment size
	iReadAheadLen = (iReadAheadLen + segmentSize - 1) & (TInt) segmentSizeMask;

#if defined (_DEBUG_READ_AHEAD)
	TInt64 oldReadAheadPos = iReadAheadPos;
#endif

	DoReadAhead(aMsgRequest, aMode);


//	RDebug::Print(_L("Buffered: old %d new %d"), bytesBuffered, (iReadAheadPos == 0)?bytesBuffered:iReadAheadPos - iLastReadPos);

#if defined (_DEBUG_READ_AHEAD)
RDebug::Print(_L("Buffered: old %d new %d iLastReadPos %d ReadAheadPos old %d new %d iReadAheadLen %d"), 
	bytesBuffered, 
	(TInt) ((iReadAheadPos == 0)?bytesBuffered:iReadAheadPos - iLastReadPos),
	I64LOW(iLastReadPos),
	I64LOW(oldReadAheadPos),
	I64LOW(iReadAheadPos),
	iReadAheadLen);
#endif	// (_DEBUG_READ_AHEAD)

	
	return;
	}


void CFileCache::DoReadAhead(CFsMessageRequest& aMsgRequest, TUint aMode)
	{


	if (iCacheClient->FindSegment(iReadAheadPos) != NULL)
		{
		// if read ahead pos is already cached, then synchronous reads have caught up,
		// so reset read ahead pos.
#if defined (_DEBUG_READ_AHEAD)
		RDebug::Print(_L("ReadAhead: pos %d already cached"), I64LOW(iReadAheadPos));
#endif
		iReadAheadPos = 0;
		return;
		}


	CFsClientMessageRequest* newRequest = NULL;
	TInt r = AllocateRequest(newRequest, EFalse, aMsgRequest.Session());
	if (r != KErrNone)
		return;

	TRACETHREADID(aMsgRequest.Message());
	OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_AHEAD, "Issue read ahead, clientThreadId %x fileCache %x pos %x:%x len %d", 
		 (TUint) threadId, (TUint) this, (TUint) I64HIGH(iReadAheadPos), (TUint) I64LOW(iReadAheadPos), iReadAheadLen);

	r = newRequest->PushOperation(
		iReadAheadPos, 
		iReadAheadLen,
		(TUint8*) NULL, 
		0,						// aOffset 
		NULL);					// aCallback
	if (r != KErrNone)
		{
		newRequest->Free();
		newRequest = NULL;
		return;
		}

	__CACHE_PRINT2(_L("TFsFileRead: ReadAhead pos %ld len %d"), newRequest->CurrentOperation().iReadWriteArgs.iPos, newRequest->CurrentOperation().iReadWriteArgs.iTotalLength);

//	RDebug::Print(_L("ReadH:\tpos %d\tlen %d"), I64LOW(newRequest->CurrentOperation().iReadWriteArgs.iPos), newRequest->CurrentOperation().iReadWriteArgs.iTotalLength);

	r = ReadBuffered(*newRequest, aMode);
	if (r != CFsRequest::EReqActionContinue)
		{
		// if read ahead pos is already cached, then synchronous reads have caught up,
		// so reset read ahead pos.
		if (r == CFsRequest::EReqActionComplete)
			{
#if defined (_DEBUG_READ_AHEAD)
			RDebug::Print(_L("ReadAhead pos %d ALREADY DONE !!!"), I64LOW(iReadAheadPos));
#endif
			iReadAheadPos = 0;
			}

		newRequest->PopOperation();
		newRequest->Free();
		newRequest = NULL;
		return;
		}

	iReadAheadPos = iReadAheadPos + iReadAheadLen;
	iReadAheadRequest = newRequest;

#if defined (_DEBUG_READ_AHEAD)
	RDebug::Print(_L("Dispatching ReadAhead with %s priority"), iFileCacheReadAsync?_S16("HIGH"):_S16("LOW"));
#endif
	// If if media driver reads are synchronous (i.e. not interrupt driven) dispatch read-ahead 
	// with low priority  so that it doesn't prevent client thread from running
	newRequest->Dispatch(
		EFalse,					// don't init
		iFileCacheReadAsync?(TBool)EFalse:(TBool)ETrue, 
		EFalse);				// dispatch to back
	}

TInt CFileCache::CompleteRead(CFsRequest* aRequest)
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	__ASSERT_DEBUG(msgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);
	CFileCache* fileCache = file->FileCache();

	__ASSERT_DEBUG(fileCache != NULL, Fault(ENoFileCache));
#ifdef _DEBUG
	TInt cancelling = (msgRequest.LastError() == KErrCancel)?(TBool)ETrue:(TBool)EFalse;
#endif

	TUint mode = share?share->iMode : EFileReadBuffered;
	TInt r = fileCache->ReadBuffered(msgRequest, mode);

	// if this request has been cancelled we mustn't dispatch it again - 
	// we still need to call state machine however so that any locked segments can be unlocked


	TInt lastError = msgRequest.LastError();

#ifdef _DEBUG
	__ASSERT_DEBUG(!cancelling || msgRequest.LastError() == KErrCancel, Fault(ERequestUncancelled));
#endif

	if (lastError == KErrCancel)
		r = CFsRequest::EReqActionComplete;

	return r;
	}

/**
ReadBuffered - attempts to read from cache. 
Called from TFsFileRead::Initialise() and CFileCache::CompleteRead()

@return CFsRequest::EReqActionComplete if request entirely satisfied
		CFsRequest::EReqActionContinue if request not yet complete. 
			Results in transition from :
				TFsFileRead::PostInitialise() to TFsFileRead::DoRequestL() or
				CFileCache::CompleteRead() to TFsFileRead::DoRequestL()
		CFsRequest::EReqActionBusy if filecache is "busy"
*/
TInt CFileCache::ReadBuffered(CFsMessageRequest& aMsgRequest, TUint aMode)
	{
	iLock.Wait();

	CFsClientMessageRequest* newRequest = NULL;

	__CACHE_PRINT2(_L("CFileCache::ReadBuffered()  pos %d, len %d"), I64LOW(aMsgRequest.CurrentOperation().iReadWriteArgs.iPos), aMsgRequest.CurrentOperation().iReadWriteArgs.iTotalLength);
	TInt r = DoReadBuffered(aMsgRequest, aMode, newRequest);
	
	iLock.Signal();

	if (newRequest)
		newRequest->Dispatch();

	return r;
	}


void CFileCache::UpdateSharePosition(CFsMessageRequest& aMsgRequest, TMsgOperation& aCurrentOperation)
	{
	// update the file share's position if this request came from client
	if (aCurrentOperation.iClientRequest)
		{
		CFileShare* share = (CFileShare*)aMsgRequest.ScratchValue();
		if (aMsgRequest.LastError() == KErrNone)
			{
			__e32_atomic_store_ord64(&share->iPos, aCurrentOperation.iReadWriteArgs.iPos);
			}
		}
	}

TInt CFileCache::DoReadBuffered(CFsMessageRequest& aMsgRequest, TUint aMode, CFsClientMessageRequest*& aNewRequest)
	{
	enum states
		{
		EStBegin=0,
		EStReadFromCache,
		EStReadFromDiskComplete,
		EStCopyToClient,
		EStReStart,
		EStEnd
		};


	TInt retCode = CFsRequest::EReqActionComplete;
	TInt& lastError = aMsgRequest.LastError();
	TBool driveThread = IsDriveThread();

	// temp storage for transition between EStReadFromCache / EStReadFromDiskComplete and EStCopyToClient
	TInt readLen = 0;
	TUint8* addr = NULL;
	TInt64 segmentStartPos = 0;

	TInt segmentSize = SegmentSize();
	TInt segmentSizeLog2 = SegmentSizeLog2();
	TInt64 segmentSizeMask = SegmentSizeMask();

	for(;;)
		{
		TMsgOperation* currentOperation = &aMsgRequest.CurrentOperation();
		TInt64& currentPos = currentOperation->iReadWriteArgs.iPos;
		TInt& totalLen = currentOperation->iReadWriteArgs.iTotalLength;
		TInt& currentOffset = currentOperation->iReadWriteArgs.iOffset;
		TBool readAhead = (currentOperation->iReadWriteArgs.iData == NULL)?(TBool)ETrue:(TBool)EFalse;

		switch(currentOperation->iState)
			{
			case EStBegin:


				// if EFileReadDirectIO, flush write cache and read direct from  disk
				if (!(aMode & EFileReadBuffered))
					{
					iLock.Signal();
					TInt r = FlushDirty(&aMsgRequest);
					iLock.Wait();
					if (r == CFsRequest::EReqActionBusy)
						return CFsRequest::EReqActionBusy;
					return CFsRequest::EReqActionContinue;	// read uncached
					}

				if (currentPos > iSize64)
					currentPos = iSize64;

				currentOperation->iState = EStReadFromCache;

				// count the number of sequential reads for read-ahead
				if (currentOperation->iClientRequest)
					{
					if (currentPos == iLastReadPos)
						{
						iSequentialReads++;
						}
					else
						{
						iSequentialReads = 0;
						ResetReadAhead();
						}
					iLastReadPos = currentPos + totalLen;
					iLastReadLen = totalLen;
					}

				
				// fall into...

			case EStReadFromCache:
				{
				// reading past end of file ?
				if (currentPos + totalLen > iSize64)
					totalLen = (TInt) (iSize64 - currentPos);
				

				if (totalLen == 0 || lastError != KErrNone)
					{
					currentOperation->iState = EStEnd;
					break;
					}

				segmentStartPos = currentPos & segmentSizeMask;

			
				TInt64 endPos = currentPos + totalLen;
				TUint maxLenToRead = (TUint)Min((TInt64)(iCacheClient->CacheLineSize()), (endPos - segmentStartPos));
				TInt maxSegments = (maxLenToRead + segmentSize - 1) >> segmentSizeLog2;


				TInt segmentCount = maxSegments;
				TInt filledSegmentCount;
				TInt lockError;
				addr = iCacheClient->FindAndLockSegments(segmentStartPos, segmentCount, filledSegmentCount, lockError, EFalse);


#if defined (_DEBUG_READ_AHEAD)
				if (addr && readAhead)
					RDebug::Print(_L("READAHEAD CACHELINE ALREADY EXISTS POS %d"), I64LOW(segmentStartPos));
#endif

				// if cacheline contains filled and empty segments, deal with these seperately
				// to simplify the code.....
				if (filledSegmentCount > 0 && segmentCount > filledSegmentCount)
					{
					segmentCount = Min(segmentCount, filledSegmentCount);
					}

				if (lockError == KErrInUse)
					{
					// cacheline in use (by other thread):
					// if this is a read-ahead, abandon it
					// otherwise re-post the request
					if (readAhead)
						{
						totalLen = 0;
						retCode = CFsRequest::EReqActionComplete;
						currentOperation->iState = EStEnd;
						}
					else
						{
#if defined (_DEBUG_READ_AHEAD)
						RDebug::Print(_L("READC CACHELINE BUSY POS %d"), I64LOW(segmentStartPos));
#endif

						return CFsRequest::EReqActionBusy;
						}
					break;
					}

				// if not found, try to allocate as many contiguous segments 
				// as possible so that the number of reads issued to the media 
				// driver is kept to a minumum

				if (addr == NULL)	// not found ?
					{
					// if read len > size of the cache, don't read excess 
					// through the cache as this is wasteful
					if (totalLen > iCacheSize)
						{
						TInt len = totalLen - iCacheSize;

						TRACETHREADID(aMsgRequest.Message());
						OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_UNCACHED1, "Read, media to client, clientThreadId %x fileCache %x pos %x:%x len %d", 
							 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) len);

						TInt r = aMsgRequest.PushOperation(
							currentPos, len, 
							(TDesC8*) currentOperation->iReadWriteArgs.iData, currentOffset,
							CompleteRead, 
							EStReadFromCache);
						if (r != KErrNone)
							{
							currentOperation->iState = EStReStart;
							break;
							}

						currentOffset+= len;
						currentPos+= len;
						totalLen-= len;
						return CFsRequest::EReqActionContinue;
						}

					// if position not cached postpone Initialise() to drive thread
					// as there may be a read ahead already in the queue

					if (!driveThread && !readAhead)
						{
#if defined (_DEBUG_READ_AHEAD)
						RDebug::Print(_L("*** POSTING READ TO DRIVE THREAD POS %d ***"), I64LOW(segmentStartPos));
#endif
						return CFsRequest::EReqActionBusy;
						}

					segmentCount = maxSegments;
					addr = iCacheClient->AllocateAndLockSegments(segmentStartPos, segmentCount, EFalse, !readAhead);
					if (addr == NULL)
						{
						__CACHE_PRINT(_L("AllocateSegment failed"));
						currentOperation->iState = EStReStart;
						break;
						}
					}

				
				readLen = segmentCount << segmentSizeLog2;
				__ASSERT_DEBUG(iSize64 > segmentStartPos, Fault(EPosBeyondSize));
				readLen = (TInt)Min((TInt64)readLen, (iSize64 - segmentStartPos));

				if (iCacheClient->SegmentEmpty(segmentStartPos))
					{
					// store readLen & addr in scratch area
					currentOperation->iScratchValue0 = addr;						
					currentOperation->iScratchValue1 = (TAny*) readLen;						

					// read into cache segment(s)
					__CACHE_PRINT2(_L("CACHEFILE: read pos %ld, readLen %d"), segmentStartPos, readLen);

					TRACETHREADID(aMsgRequest.Message());
					OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_CACHED1, "Read, media to cache, clientThreadId %x fileCache %x pos %x:%x len %d", 
						 (TUint) threadId, (TUint) this, (TUint) I64HIGH(segmentStartPos), (TUint) I64LOW(segmentStartPos), (TUint) readLen);

					TInt r = aMsgRequest.PushOperation(
						segmentStartPos, readLen, addr, 0,
						CompleteRead, 
						EStReadFromDiskComplete);
					if (r != KErrNone)
						{
						iCacheClient->UnlockSegments(segmentStartPos);
						currentOperation->iState = EStReStart;
						break;
						}

					return CFsRequest::EReqActionContinue;
					}
				else
					{
					currentOperation->iState = EStCopyToClient;
					}
				}
				// fall into ...
			
			case EStCopyToClient:
				{
				TInt segmentsLocked = (readLen + segmentSize - 1) >> segmentSizeLog2;

				if (lastError == KErrNone)
					{
					// copy to user buffer
					TInt offset = iCacheClient->CacheOffset(currentPos);	// offset into segment
					TInt len = Min(readLen - offset, totalLen);
					
					// if addr is NULL then this is a read ahead request
					if (currentOperation->iReadWriteArgs.iData != NULL)
						{
						TRACETHREADID(aMsgRequest.Message());
						OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_COPY, "Read, cache to client, clientThreadId %x fileCache %x pos %x:%x len %d", 
							 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) len);

						if (currentOperation->iClientRequest)
							{
							__ASSERT_DEBUG (aMsgRequest.Message().Handle() != NULL, Fault(EMsgAlreadyCompleted));
							TPtrC8 ptr(addr+offset, len);
							lastError = aMsgRequest.Write(0, ptr, currentOffset);
							}
						else
							{
							memcpy(((TUint8*) currentOperation->iReadWriteArgs.iData) + currentOffset, addr+offset, len);
							}
						}
					else
						{
						iReadAheadRequest = NULL;
						}

					currentOffset+= len;
					currentPos+= len;
					totalLen-= len;
					}

				if (lastError == KErrNone)
					iCacheClient->MarkSegmentsAsFilled(segmentStartPos, segmentsLocked);
				iCacheClient->UnlockSegments(segmentStartPos);

				if (lastError != KErrNone)
					{
					retCode = CFsRequest::EReqActionComplete;
					currentOperation->iState = EStEnd;
					break;
					}

				if (totalLen > 0 && lastError == KErrNone)
					{
					currentOperation->iState = EStReadFromCache;
					break;
					}
				currentOperation->iState = EStEnd;
				}
			// fall into ...
			

			case EStEnd:
				// update the file share's position if this request came from client
				UpdateSharePosition(aMsgRequest, *currentOperation);
				return retCode;

			case EStReadFromDiskComplete:
				{
				// restore readLen etc from scratch area
				segmentStartPos = currentPos & segmentSizeMask;
				addr = (TUint8*) currentOperation->iScratchValue0;
				readLen = (TInt) currentOperation->iScratchValue1;
				
				TRACETHREADID(aMsgRequest.Message());
				OstTraceExt3(TRACE_FILECACHE, FILECACHE_READ_COMPLETE, "Read, media to cache, clientThreadId %x fileCache %x r %d", 
					 (TUint) threadId, (TUint) this, (TUint) (TUint) lastError);

				aMsgRequest.CurrentOperation().iState = EStCopyToClient;
				}
				break;

			case EStReStart:

				// ignore the failure if this is a read-ahead
				if (readAhead)
					{
					totalLen = 0;
					retCode = CFsRequest::EReqActionComplete;
					currentOperation->iState = EStEnd;
					break;
					}

				// We need to flush all dirty data to disk in case this read overlaps
				// with any dirty data already in the file cache
				if (aMode & EFileWriteBuffered)
					{
					TInt r = DoFlushDirty(aNewRequest, &aMsgRequest, EFlushAll);
					if (r == CFsRequest::EReqActionBusy || r != CFsRequest::EReqActionComplete)
						return r;
					}

				retCode = CFsRequest::EReqActionContinue;	// read uncached
				currentOperation->iState = EStEnd;

				/*
				We're now going to by-pass the file cache.
				If we've already written something to the client's buffer then, in the flexible
				memory model, the KDesWrittenShift bit will be set and so the descriptor length will 
				updated in RMessageK::CallbackFunc() when the client thread runs. This (shorter) 
				length will overwrite the descriptor length written by the local media subsystem.
				To get round this problem, we set the descriptor length artificially by writing a 
				zero-length descriptor at the end of the client's buffer.
				*/
				if (currentOffset > 0 && currentOperation->iClientRequest)
					{
					__ASSERT_DEBUG (aMsgRequest.Message().Handle() != NULL, Fault(EMsgAlreadyCompleted));
					TPtrC8 ptr(NULL, 0);
					TInt r = aMsgRequest.Write(0, ptr, currentOffset+totalLen);
					__CACHE_PRINT3(_L("CFileCache::DoReadBuffered() failed at pos %lx offset %x totalLen %x\n"), currentPos, currentOffset, totalLen);
					__CACHE_PRINT2(_L("CFileCache::DoReadBuffered() writing zero bytes at offset %x r %d\n"), currentOffset + totalLen, r);
					if (r != KErrNone)
						retCode = r;
					}

				aMsgRequest.ReStart();
				UpdateSharePosition(aMsgRequest, *currentOperation);

				TRACETHREADID(aMsgRequest.Message());
				OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_UNCACHED2, "Read media to client, clientThreadId %x fileCache %x pos %x:%x len %d", 
					 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) totalLen);

				return retCode;


			};
		}	// for (;;)

	}



/**
WriteBuffered - attempts to write to cache. 
Called from TFsFileRead::Initialise and TFsFileRead::DoRequestL

@return CFsRequest::EReqActionComplete if request entirely satisfied
		CFsRequest::EReqActionContinue if request not yet complete
		CFsRequest::EReqActionBusy if filecache is busy
*/
TInt CFileCache::WriteBuffered(CFsMessageRequest& aMsgRequest, TUint aMode)
	{
	iLock.Wait();


	CFsClientMessageRequest* newRequest = NULL;

	__CACHE_PRINT2(_L("CFileCache::WriteBuffered()  pos %d, len %d"), I64LOW(aMsgRequest.CurrentOperation().iReadWriteArgs.iPos), aMsgRequest.CurrentOperation().iReadWriteArgs.iTotalLength);
	TInt r = DoWriteBuffered(aMsgRequest, newRequest, aMode);
	
	iLock.Signal();

	if (newRequest)
		newRequest->Dispatch();

	// completion ?
	if (r == CFsRequest::EReqActionComplete)
		{
		TMsgOperation& currentOperation = aMsgRequest.CurrentOperation();
		
		__ASSERT_DEBUG(aMsgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));

		TFsFileWrite::CommonEnd(&aMsgRequest, r, iInitialSize, iSize64, currentOperation.iReadWriteArgs.iPos, EFalse);
		}
	
	return r;
	}




TInt CFileCache::CompleteWrite(CFsRequest* aRequest)
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	CFileCB& file = share->File();
	CFileCache* fileCache = file.FileCache(); //&file;
	__ASSERT_DEBUG(fileCache != NULL, Fault(ENoFileCache));

#ifdef _DEBUG
	TInt cancelling = (msgRequest.LastError() == KErrCancel)?(TBool)ETrue:(TBool)EFalse;
#endif
	

	TInt r = fileCache->WriteBuffered(msgRequest, share->iMode);

#ifdef _DEBUG
	__ASSERT_DEBUG(!cancelling || msgRequest.LastError() == KErrCancel, Fault(ERequestUncancelled));
#endif

	// if this request has been cancelled we mustn't dispatch it again - 
	// we still need to call state machine however so that any locked segments can be unlocked
	TInt lastError = msgRequest.LastError();
	if (lastError == KErrCancel)
		r = CFsRequest::EReqActionComplete;

	return r;
	}



TInt CFileCache::DoWriteBuffered(CFsMessageRequest& aMsgRequest, CFsClientMessageRequest*& aNewRequest, TUint aMode)

	{
	enum states
		{
		EStBegin=0,
		EStWriteToCache,
		EStReadFromDisk,
		EStReadFromDiskComplete,
		EStWriteToCacheComplete,
		EStCopyFromClient,
		EStReStart,
		EStEnd,
		EStWriteThrough
		};

	enum flags
		{
		EReadFirstSegment	= 0x01,
		EReadLastSegment	= 0x02
		};

	TBool cachingWrites = (aMode & EFileWriteBuffered)?(TBool)ETrue:(TBool)EFalse;
	TInt& lastError = aMsgRequest.LastError();
	TBool driveThread = IsDriveThread();
	TInt segmentSize = SegmentSize();
	TInt segmentSizeLog2 = SegmentSizeLog2();
	TInt64 segmentSizeMask = SegmentSizeMask();

	// temp storage for transition between EStWriteToCache / EStWriteToCacheComplete and EStCopyFromClient
	TInt segmentCount = 0;
	TUint8* addr = NULL;
	TInt64 firstSegmentStartPos = 0;
	TInt64 readPos = 0;
	TUint8* readAddr = NULL;
	TInt readSegmentCount = 0;

	TInt readFlags = 0;
	
	for(;;)
		{
		TMsgOperation* currentOperation = &aMsgRequest.CurrentOperation();
		TInt retCode = CFsRequest::EReqActionComplete;

		TInt64& currentPos = currentOperation->iReadWriteArgs.iPos;
		TInt& totalLen = currentOperation->iReadWriteArgs.iTotalLength;
		TInt& currentOffset = currentOperation->iReadWriteArgs.iOffset;
		switch(currentOperation->iState)
			{
			case EStBegin:
				{				
				// Report background flush errors back to client
				CFileShare* share = (CFileShare*) aMsgRequest.ScratchValue(); 
				if (share->iFlushError != KErrNone)
					{
					TInt r = share->iFlushError;
					share->iFlushError = KErrNone;
					return r;
					}

				if (currentPos > iSize64)
					currentPos = iSize64;
				iInitialSize = iSize64;


#ifdef DOUBLE_BUFFERED_WRITING
				// count the number of sequential write-appends
				if (currentOperation->iClientRequest)
					{
					if (currentPos == iSize64)
						iSequentialAppends++;
					else
						iSequentialAppends = 0;
					}
#endif // DOUBLE_BUFFERED_WRITING

				// if EFileWriteDirectIO OR 
				// (caching writes and requested write len > size of the cache), 
				// flush the write cache
				if ((aMode & EFileWriteBuffered) == 0 || 
					(cachingWrites && totalLen > iCacheSize))
					{
					TInt r = DoFlushDirty(aNewRequest, &aMsgRequest, EFlushAll);
					if (r == CFsRequest::EReqActionBusy || r != CFsRequest::EReqActionComplete)
						return r;
					}
				// if EFileWriteDirectIO then overwrite any non-dirty data and 
				// write direct to disk....

				// if caching writes and requested write len > size of the cache, 
				// don't write excess  through the cache as this is wasteful
				if (cachingWrites && totalLen > iCacheSize)
					{
					// Destroy ALL cached data (dirty and non-dirty) to ensure 
					// cache is consistent with data written to disk
					iCacheClient->Purge(ETrue);

					TInt len = totalLen - iCacheSize;

					TRACETHREADID(aMsgRequest.Message());
					OstTraceExt5(TRACE_FILECACHE, FILECACHE_WRITE_UNCACHED1, "Write, client to media, clientThreadId %x fileCache %x pos %x:%x len %d", 
						 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) len);

					TInt r = aMsgRequest.PushOperation(
						currentPos, len, 
						(TDesC8*) currentOperation->iReadWriteArgs.iData, currentOffset,
						CompleteWrite, 
						EStWriteToCache);
					if (r != KErrNone)
						{
						currentOperation->iState = EStReStart;
						break;
						}

					currentOffset+= len;
					currentPos+= len;
					totalLen-= len;
					return CFsRequest::EReqActionContinue;
					}

				currentOperation->iState = EStWriteToCache;
				}		
				break;

			case EStWriteToCache:
				__ASSERT_DEBUG(aMsgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));
				{
				// NB we must carry on if we get an error to ensure cache is consistent with disk
				if (totalLen == 0 || lastError == KErrCancel)
					{
					currentOperation->iState = EStWriteToCacheComplete;
					break;
					}

				firstSegmentStartPos = currentPos & segmentSizeMask;
				TInt64 endPos = currentPos + totalLen;

				// Find the maximum number of contiguous segments we need to lock
				// in this cacheline - when we unlock these will be marked as filled
				const TInt64 dataRange = Min((TInt64)iCacheSize, (endPos + segmentSize - 1 - firstSegmentStartPos));
				TInt maxSegmentCount =  (TInt)(dataRange >> segmentSizeLog2);
				
				segmentCount = maxSegmentCount;

				TInt lockError;
				TInt filledSegmentCount;
				addr = iCacheClient->FindAndLockSegments(firstSegmentStartPos, segmentCount, filledSegmentCount, lockError, cachingWrites);
				
				if (lockError == KErrInUse)
					return CFsRequest::EReqActionBusy;

#ifdef LAZY_WRITE
				if (cachingWrites && addr == NULL && lastError == KErrNone && iCacheClient->TooManyLockedSegments())
					{
					// Flush a single dirty cacheline (if there is one for this file)
					// or all dirty data  on this drive (if there is any).
					// If there's nothing to flush then the dirty (locked) data
					// must belong to another drive, so there's not much we can do
					// but write through 
					TInt r = DoFlushDirty(aNewRequest, &aMsgRequest, EFlushSingle);
					if (r == CFsRequest::EReqActionBusy)
						return CFsRequest::EReqActionBusy;
					if (r != KErrInUse)
						{
						// Need to switch to drive thread to flush all data on this drive
						if (!driveThread)
							return CFsRequest::EReqActionBusy;
						iLock.Signal();
						TInt r = iDrive->FlushCachedFileInfo();
						iLock.Wait();
						if (r == CFsRequest::EReqActionBusy)
							return CFsRequest::EReqActionBusy;

						lastError = KErrNoMemory;	// write through
						}
					}
#else
				// flush cache before allocating a new cacheline
				if (cachingWrites && addr == NULL && lastError == KErrNone)
					{
					// if no segment available, flush a single dirty cacheline 
					// or wait if already flushing
					if (iFlushBusy)
						return CFsRequest::EReqActionBusy;
					TInt r = DoFlushDirty(aNewRequest, &aMsgRequest, EFlushSingle);
					if (r != CFsRequest::EReqActionBusy && r != CFsRequest::EReqActionComplete)
						lastError = r;
					}
#endif
#ifdef DOUBLE_BUFFERED_WRITING
				if (cachingWrites && lastError == KErrNone && 
					iSequentialAppends >= KMinSequentialAppendsBeforeFlush && iCacheClient->LockedSegmentsHalfUsed())
					{
					DoFlushDirty(aNewRequest, &aMsgRequest, EFlushHalf);
					}
#endif
				// if no cacheline found & write caching is enabled, allocate a new cacheline
				if (cachingWrites && addr == NULL && lastError == KErrNone)
					{
					// try to allocate up to write cache size - this may not 
					// be possible if a segment in the range is already cached
					segmentCount = maxSegmentCount;
					addr = iCacheClient->AllocateAndLockSegments(currentPos, segmentCount, cachingWrites, ETrue);
					
					// continue if alloc failed
					if (addr == NULL)
						lastError = KErrNoMemory;
				
					}

				if (addr == NULL)
					{
					if (cachingWrites && lastError == KErrNone)
						lastError = KErrNoMemory;
					segmentCount = 1;
					currentOperation->iState = EStCopyFromClient;
					break;
					}

				// if the first or last segment are empty then we'll have to read-before-write
				TInt64 lastSegmentStartPos = firstSegmentStartPos + ((segmentCount-1) << segmentSizeLog2);
				
				TInt startPosSegOffset = iCacheClient->CacheOffset(currentPos);
				TInt endPosSegOffset = iCacheClient->CacheOffset(endPos);

				// partial first segment ?
				if (startPosSegOffset != 0 && 
					firstSegmentStartPos < iFileCB->Size64() &&
					iCacheClient->SegmentEmpty(firstSegmentStartPos))
					{
					readFlags|= EReadFirstSegment;
					}
		
				// partial last segment ? 
				// NB this may be the first segment too !
				if (endPosSegOffset != 0 && 
					endPos < lastSegmentStartPos + segmentSize &&
					endPos < iFileCB->Size64() &&
					iCacheClient->SegmentEmpty(lastSegmentStartPos))
					{
					readFlags|= EReadLastSegment;
					}

				// read-before-write required ?
				if (readFlags & EReadFirstSegment)
					{
					readFlags&= ~EReadFirstSegment;
					readPos = firstSegmentStartPos;
					readAddr = addr;
					readSegmentCount = 1;
					// if the last segment is empty and it's the same as the first or contiguous,
					// then read that too 
					if ((readFlags & EReadLastSegment) && (segmentCount <= 2))
						{
						readSegmentCount = segmentCount;
						readFlags&= ~EReadLastSegment;
						}
					currentOperation->iState = EStReadFromDisk;
					}
				else if (readFlags & EReadLastSegment)
					{
					readFlags&= ~EReadLastSegment;
					readPos = lastSegmentStartPos;
					readAddr = addr + ((segmentCount-1) << segmentSizeLog2);
					readSegmentCount = 1;
					currentOperation->iState = EStReadFromDisk;
					}
				else
					{
					currentOperation->iState = EStCopyFromClient;
					}
				}
				break;

			case EStReadFromDisk:
				{
				// Save address & segmentCount in scratch area
				currentOperation->iScratchValue0 = addr;	
				__ASSERT_DEBUG(segmentCount < 0xFFFF, Fault(EBadSegmentCount));
				currentOperation->iScratchValue1 = (TAny*) ((readFlags << 16) | segmentCount);

				TInt maxReadLen = readSegmentCount << segmentSizeLog2;
#ifdef __VC32__
#pragma warning( disable : 4244 )  
//Disable Compiler Warning (levels 3 and 4) C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
// maxReadLen is of 31 bits. Hence Min result is reduced to 31 bits
#endif
				TInt readLen = (TInt)Min((TInt64)maxReadLen, (iSize64 - readPos));
#ifdef __VC32__
#pragma warning( default : 4244 )
#endif
				__ASSERT_DEBUG (aMsgRequest.Message().Handle() != NULL, Fault(EMsgAlreadyCompleted));					

				TRACETHREADID(aMsgRequest.Message());
				OstTraceExt5(TRACE_FILECACHE, FILECACHE_READ_CACHED2, "Read, media to cache, clientThreadId %x fileCache %x pos %x:%x len %d", 
					 (TUint) threadId, (TUint) this, (TUint) I64HIGH(readPos), (TUint) I64LOW(readPos), (TUint) readLen);

				TInt r = aMsgRequest.PushOperation(
					readPos, readLen, readAddr, 0, 
					CompleteWrite,
					EStReadFromDiskComplete,
					EFsFileRead);
				if (r != KErrNone)
					{
					lastError = KErrNoMemory;
					currentOperation->iState = EStEnd;
					iCacheClient->UnlockSegments(firstSegmentStartPos);
					break;
					}

				// requeue this request
				return CFsRequest::EReqActionContinue;
				}

			case EStReadFromDiskComplete:
				{
				__ASSERT_DEBUG(aMsgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));
				// restore addr & segmentCount etc from scratch area
				firstSegmentStartPos = currentPos & segmentSizeMask;
				addr = (TUint8*) currentOperation->iScratchValue0;
				segmentCount = ((TInt) currentOperation->iScratchValue1) & 0xFFFF;
				readFlags = ((TInt) currentOperation->iScratchValue1) >> 16;

				if (readFlags & EReadLastSegment)
					{
					TInt64 lastSegmentStartPos = firstSegmentStartPos + ((segmentCount-1) << segmentSizeLog2);
					readFlags&= ~EReadLastSegment;
					readPos = lastSegmentStartPos;
					readAddr = addr + ((segmentCount-1) << segmentSizeLog2);
					readSegmentCount = 1;
					currentOperation->iState = EStReadFromDisk;
					break;
					}
				
				aMsgRequest.CurrentOperation().iState = EStCopyFromClient;
				}
				break;

			case EStCopyFromClient:
				{
				TInt writeLen = segmentCount << segmentSizeLog2;
				TInt offset = iCacheClient->CacheOffset(currentPos);	// offset into segment
				writeLen = Min(writeLen - offset, totalLen);

				if (addr != NULL)
					{
					__ASSERT_DEBUG (aMsgRequest.Message().Handle() != NULL, Fault(EMsgAlreadyCompleted));
					// copy from user buffer to cache buffer
					TInt writeError = KErrNone;

					TRACETHREADID(aMsgRequest.Message());
					OstTraceExt5(TRACE_FILECACHE, FILECACHE_WRITE_COPY, "Write, client to cache, clientThreadId %x fileCache %x pos %x:%x len %d", 
						 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) writeLen);

					if (currentOperation->iClientRequest)
						{
						TPtr8 ptr(addr+offset, writeLen, writeLen);
						writeError = aMsgRequest.Read(0, ptr, currentOffset);
						}
					else
						{
						memcpy(addr+offset, ((TUint8*) currentOperation->iReadWriteArgs.iData) + currentOffset, writeLen);
						}

					// "writing" past end of file ?
					if (currentPos + writeLen > iSize64 && cachingWrites && lastError == KErrNone && writeError == KErrNone)
						{
						__CACHE_PRINT2(_L("CACHEFILE: extending size old %ld new %ld"), iSize64, currentPos + totalLen);
						iSize64 = currentPos + writeLen;
						}

					TInt anyError = (writeError != KErrNone)?writeError:lastError;

					if (anyError == KErrNone)
						iCacheClient->MarkSegmentsAsFilled(firstSegmentStartPos, segmentCount);

					if (cachingWrites && anyError == KErrNone)
						{
						iCacheClient->MarkSegmentsAsDirty(firstSegmentStartPos, segmentCount);
						// start dirty data timer
						FileDirty(aMsgRequest);
						}

					// unlock if we're not buffering writes (segments won't be unlocked if dirty)
					iCacheClient->UnlockSegments(firstSegmentStartPos);
					}

				currentOffset+= writeLen;
				currentPos+= writeLen;
				totalLen-= writeLen;

				currentOperation->iState = EStWriteToCache;
				break;
				}

			case EStWriteToCacheComplete:
				{
				__ASSERT_DEBUG(aMsgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));

				if (lastError == KErrCancel)
					{
					currentOperation->iState = EStEnd;
					}
				else if ((!cachingWrites) || (lastError != KErrNone))
					{
					// allow TFsFileWrite::DoRequestL() to proceed normally using original pos & len
					__ASSERT_DEBUG(aMsgRequest.CurrentOperationPtr() != NULL, Fault(ECacheBadOperationIndex));
	
					currentOperation->iState = EStWriteThrough;
					}
				else
					{
					__CACHE_PRINT2(_L("CACHEFILE: write buffered pos %ld, len %d"), 
						aMsgRequest.CurrentOperation().iReadWriteArgs.iPos, aMsgRequest.CurrentOperation().iReadWriteArgs.iOffset);
					__ASSERT_DEBUG(totalLen == 0, Fault(ECompletingWriteWithDataRemaining));

					currentOperation->iState = EStEnd;
					}
				break;
				}

			case EStWriteThrough:

				if (lastError == KErrCancel)
					{
					currentOperation->iState = EStEnd;
					break;
					}

				// we're going to issue an uncached write so clear any error
				lastError = KErrNone;

				if (cachingWrites)
					{
					TInt r = DoFlushDirty(aNewRequest, &aMsgRequest, EFlushAll);
#ifdef _DEBUG
					if (r == CFsRequest::EReqActionBusy)
						CCacheManagerFactory::CacheManager()->Stats().iWriteThroughWithDirtyDataCount++;
#endif
					// Need to reset currentOperation.iReadWriteArgs.iTotalLength here to make sure 
					// TFsFileWrite::PostInitialise() doesn't think there's no data left to process
					aMsgRequest.ReStart();
					
					//Need to preserve the current state otherwise if we are over the ram threshold 
					//the request can end up in a livelock trying to repeatedly flush.
					currentOperation->iState = EStWriteThrough;
					
					if (r == CFsRequest::EReqActionBusy || r != CFsRequest::EReqActionComplete)
						return r;
					}

				retCode = CFsRequest::EReqActionContinue;

				// fall into...EStRestart
				currentOperation->iState = EStReStart;

			case EStReStart:
				{
				__ASSERT_DEBUG(retCode == CFsRequest::EReqActionBusy || retCode == CFsRequest::EReqActionContinue, Fault(EBadRetCode));

				aMsgRequest.ReStart();
				UpdateSharePosition(aMsgRequest, *currentOperation);
				if (currentOperation->iClientRequest)
					currentOperation->iReadWriteArgs.iPos = currentOperation->iClientPosition; // NB maybe KCurrentPosition64

				TRACETHREADID(aMsgRequest.Message());
				OstTraceExt5(TRACE_FILECACHE, FILECACHE_WRITE_UNCACHED2, "Write, client to media, clientThreadId %x fileCache %x pos %x:%x len %d", 
					 (TUint) threadId, (TUint) this, (TUint) I64HIGH(currentPos), (TUint) I64LOW(currentPos), (TUint) totalLen);

				return retCode;
				}

			case EStEnd:
				return retCode;
			}
		}
	}



TInt CFileCache::AllocateRequest(CFsClientMessageRequest*& aNewRequest, TBool aWrite, CSessionFs* aSession,TUid aUid)
	{

	RLocalMessage msgNew;
	const TOperation& oP = OperationArray[aWrite?EFsFileWriteDirty:EFsFileRead];
	TInt r = RequestAllocator::GetMessageRequest(oP, msgNew, aNewRequest);
	if (r != KErrNone)
		return r;

	aNewRequest->Set(msgNew, oP, aSession, aUid);
	aNewRequest->SetDrive(iDrive);
	
	// read-aheads and write-dirty requests should not be posted to plugins
	// If there are data-modifying plugins, then these should sit above the file cache
	aNewRequest->iCurrentPlugin = NULL;
	aNewRequest->EnablePostIntercept(EFalse);

	// Scratch value is a CFileCB pointer NOT a CFileShare for requests
	// allocated by the file cache
	aNewRequest->SetScratchValue64( MAKE_TINT64(EFalse, (TUint) iFileCB) );

	// don't call Initialise(), don't call PostInitialise()
	aNewRequest->SetState(CFsRequest::EReqStateDoRequest);
	
	// don't call Message().Complete()
	aNewRequest->SetCompleted(EFalse);		

	__ASSERT_DEBUG(aNewRequest->CurrentOperationPtr() == NULL, Fault(EBadOperationIndex));

	return KErrNone;
	}

TInt CFileCache::FlushDirty(CFsRequest* aOldRequest)
	{
	iLock.Wait();

	CFsClientMessageRequest* newRequest = NULL;

#if defined (OST_TRACE_COMPILER_IN_USE)
	RMessage2 msgOld;
	if (aOldRequest)
		msgOld = const_cast<RMessage2&> (aOldRequest->Message());
	TRACETHREADID(msgOld);
	OstTraceExt2(TRACE_FILECACHE, FILECACHE_FLUSH_DIRTY, "clientThreadId %x fileCache %x", (TUint) threadId, (TUint) this);
#endif	// if defined (OST_TRACE_COMPILER_IN_USE)


	TInt r = DoFlushDirty(newRequest, aOldRequest, EFlushAll);

	iLock.Signal();

	if (newRequest)
	    {
		newRequest->Dispatch();
	    }

	return r;
	}


void CFileCache::Purge(TBool aPurgeDirty)
	{
	iLock.Wait();

	iCacheClient->Purge(aPurgeDirty);

	iLock.Signal();
	}

void CFileCache::PropagateFlushErrorToAllFileShares()
	{
	ASSERT(IsDriveThread());
	TDblQueIter<CFileShare> fileShareIter(iFileCB->FileShareList());
	CFileShare* pFileShare;
	while ((pFileShare = fileShareIter++) != NULL)
		{
		pFileShare->iFlushError = iFlushError;
		}
	}

/**
CFileCache::DoFlushDirty()

@param aFlushMode. The flush mode which indicates how many cache-lines to flush. @See TFlushMode

returns	CFsRequest::EReqActionBusy if a flush is in progress OR cacheline already in use
		CFsRequest::EReqActionComplete if nothing to flush / flush completed 
		KErrNoMemory if failed to allocate a request
		or one of the system wide error codes

 */
TInt CFileCache::DoFlushDirty(CFsClientMessageRequest*& aNewRequest, CFsRequest* aOldRequest, TFlushMode aFlushMode)
	{
	TInt64  pos;
	TUint8* addr;

	__ASSERT_ALWAYS(aNewRequest == NULL, Fault(EFlushingWithAllocatedRequest));

	if (iFlushBusy)
		return CFsRequest::EReqActionBusy;

	TInt segmentCount = iCacheClient->FindFirstDirtySegment(pos, addr);

	if (segmentCount == 0)
		{
		TInt flushError = iFlushError;
		iFlushError = KErrNone;

		// If this flush didn't originate from a client request return last error
		if (!aOldRequest || !aOldRequest->Session())
			return (flushError == KErrNone)? CFsRequest::EReqActionComplete : flushError;

		// Return the last error from CFileShare::iFlushError 
		// and then clear CFileShare::iFlushError 

		CFileShare* share = (CFileShare*) SessionObjectFromHandle(
			aOldRequest->Message().Int3(),
			FileShares->UniqueID(),
			aOldRequest->Session());

		TInt r = KErrNone;
		if (share)
			{
			r = share->iFlushError;
			share->iFlushError = KErrNone;
			}
		if (r == KErrNone)
			return CFsRequest::EReqActionComplete;

		return r;
		}


	// NB aOldRequest->Session may be NULL - e.g for FileShareCloseOp
	CSessionFs* session = aOldRequest && aOldRequest->Session() ? aOldRequest->Session() : iDirtyDataOwner;

	__ASSERT_ALWAYS(session, Fault(EFlushingWithSessionNull));
	
	TInt r = AllocateRequest(aNewRequest, ETrue, session, (aOldRequest) ? aOldRequest->Uid() : TUid::Null());
	if (r != KErrNone)
		return r;
	
	r = aNewRequest->PushOperation(0, 0, (TUint8*) NULL, 0);

	if (r != KErrNone)
		{
		aNewRequest->Free();
		aNewRequest = NULL;
		return r;
		}

	// set the flush mode
	aNewRequest->CurrentOperation().iScratchValue0 = (TAny*) aFlushMode;

	// issue flush request
	r = FlushDirtySm(*aNewRequest);

	// We should only get three possible return codes from FlushDirtySm() :
	// CFsRequest::EReqActionContinue	- a write request (aNewRequest) needs to be dispatched
	// CFsRequest::EReqActionComplete	- completed already - this can happen if dirty data was beyond file end
	// CFsRequest::EReqActionBusy		- first dirty cacheline is already in use.
	__ASSERT_DEBUG(	r == CFsRequest::EReqActionContinue || r == CFsRequest::EReqActionComplete || r == CFsRequest::EReqActionBusy, Fault(EBadRetCode));
	if (r == CFsRequest::EReqActionContinue || r == CFsRequest::EReqActionBusy)
		{
		// requeue the caller's request (aOldRequest) if there is one
		return CFsRequest::EReqActionBusy;
		}
	else	// CFsRequest::EReqActionComplete
		{
		// Return any allocation failures etc to client
		TInt lastError = aNewRequest->LastError();
		if (lastError != KErrNone)
			r = lastError;

		aNewRequest->PopOperation();
		aNewRequest->Free();
		aNewRequest = NULL;
		return r;
		}
	
	}


TInt TFsFileWriteDirty::PostInitialise(CFsRequest* aRequest)
	{
	return CFileCache::CompleteFlushDirty(aRequest);
	}

TInt CFileCache::CompleteFlushDirty(CFsRequest* aRequest)
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);
	CFileCache* fileCache = file->FileCache();

	__ASSERT_DEBUG(fileCache != NULL, Fault(ENoFileCache));

#ifdef _DEBUG
	TInt cancelling = (msgRequest.LastError() == KErrCancel)?(TBool)ETrue:(TBool)EFalse;
#endif

	fileCache->iLock.Wait();
	TInt r = fileCache->FlushDirtySm(msgRequest);
	fileCache->iLock.Signal();

#ifdef _DEBUG
	__ASSERT_DEBUG(!cancelling || msgRequest.LastError() == KErrCancel, Fault(ERequestUncancelled));
#endif

	// if this request has been cancelled we mustn't dispatch it again - 
	// we still need to call state machine however so that any locked segments can be unlocked
	if (msgRequest.LastError() == KErrCancel)
		r = CFsRequest::EReqActionComplete;

	return r;
	}

TInt CFileCache::FlushDirtySm(CFsMessageRequest& aMsgRequest)
	{
	enum states
		{
		EStBegin=0,
		EStWriteToDisk,
		EStWriteToDiskComplete,
		EStMarkAsClean,
		EStEnd
		};

	TMsgOperation* currentOperation = &aMsgRequest.CurrentOperation();
	TInt& lastError = aMsgRequest.LastError();
	TInt64 pos = 0;

	for(;;)
		{

		switch(currentOperation->iState)
			{
			case EStBegin:
				iFlushBusy = ETrue;

				// fall into...

			case EStWriteToDisk:
				{
				currentOperation->iState = EStWriteToDisk;

				TUint8* addr;
				TInt segmentCount = iCacheClient->FindFirstDirtySegment(pos, addr);
				
				// if no more dirty segments of if a genuine error then finish
				// NB if the request has been cancelled then we still need to proceed and mark all the
				// segments as clean, otherwise they will never get freed !
				if (segmentCount == 0 || (lastError != KErrNone && lastError != KErrCancel))
					{
					currentOperation->iState = EStEnd;
					break;
					}

				TInt  len = segmentCount << SegmentSizeLog2();

				if (pos < iSize64)
					// Result of Min shall be of size TInt
					// Hence to suppress warning
					len = (TInt)(Min(iSize64 - pos, (TInt64)len));
				else
					len = 0;


				// if writing past end of file or this request has been cancelled, just mark as clean
				if (len == 0 || lastError == KErrCancel)
					{
					currentOperation->iState = EStMarkAsClean;
					break;
					}

#ifdef SETSIZE_BEFORE_WRITE	
				TInt64 physicalFileSize = iFileCB->Size64();
				if ((pos + (TInt64) len) > physicalFileSize)
					{
					// Need to switch to drive thread before calling CFileCB::SetSizeL()
					if (!IsDriveThread())
						{
						aMsgRequest.SetState(CFsRequest::EReqStatePostInitialise);
						return CFsRequest::EReqActionBusy;
						}
					__CACHE_PRINT2(_L("CACHEFILE: Increasing uncached size from %ld to %ld"), iFileCB->Size64(), iSize64);
					TInt r;

					r = CheckDiskSpace(iSize64 - physicalFileSize, &aMsgRequest);
			        if(r == KErrNone)
						TRAP(r, iFileCB->SetSizeL(iSize64))
					if (r == KErrNone)
						{
 						iFileCB->SetSize64(iSize64, ETrue);	// NB drive might be locked
						}
					}
#endif
				__CACHE_PRINT3(_L("CACHEFILE: FlushDirty first dirty pos %d, addr %08X count %d"), I64LOW(pos), addr, segmentCount);

				TInt filledSegmentCount;
				TInt lockError;
				addr = iCacheClient->FindAndLockSegments(pos, segmentCount, filledSegmentCount, lockError, ETrue);

				// If cacheline is busy, we need to post request to back of drive queue
				// To dispatch to drive thread and intercept before DoRequestL() we must call iPostInitialise(), 
				// so set the state back to CFsRequest::EReqStatePostInitialise
				if (lockError == KErrInUse)
					{
					__CACHE_PRINT(_L("FlushDirtySm() - cacheline BUSY !"));
					aMsgRequest.SetState(CFsRequest::EReqStatePostInitialise);
					return CFsRequest::EReqActionBusy;
					}
				else if (lockError != KErrNone)
					{
					iFlushBusy = EFalse;
					lastError = lockError;
					return CFsRequest::EReqActionComplete;
					}

				currentOperation->Set(pos, len, addr, 0, EStWriteToDiskComplete);
				if (pos < iSize64)
					{
					OstTraceExt4(TRACE_FILECACHE, FILECACHE_WRITE_DIRTY, "Write, cache to media, fileCache %x pos %x:%x len %d", 
						 (TUint) this, (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) len);

					TInt r = aMsgRequest.PushOperation(
						pos, len, addr, 0,
						CompleteFlushDirty, 
						EStWriteToDiskComplete);
					if (r != KErrNone)
						{
						iCacheClient->UnlockSegments(pos);
						iFlushBusy = EFalse;
						lastError = r;
						return CFsRequest::EReqActionComplete;
						}

					return CFsRequest::EReqActionContinue;	// continue on to TFsFileWrite::DoRequestL()()
					}
				}

			case EStWriteToDiskComplete:
				{
#ifdef _DEBUG
				// simulate a media eject to test critical error server
				if (CCacheManagerFactory::CacheManager()->SimulateWriteFailureEnabled())
					{
					lastError = KErrNotReady;
					iDrive->Dismount();
					}
#endif
				TRACETHREADID(aMsgRequest.Message());
				OstTraceExt3(TRACE_FILECACHE, FILECACHE_FLUSH_DIRTY_COMPLETE, "Write, cache to media, clientThreadId %x fileCache %x r %d", 
					 (TUint) threadId, (TUint) this, (TUint) lastError);

				pos = currentOperation->iReadWriteArgs.iPos;
				iCacheClient->UnlockSegments(pos);
				}
				// fall into...

			case EStMarkAsClean:
				{
				// NB pos must be set by EStWriteToDiskComplete or EStWriteToDisk

				if (lastError != KErrNone)
					{
					__CACHE_PRINT1(_L("CACHEFILE: WriteThrough FAILED %d"), lastError);
					
					lastError = HandleWriteDirtyError(lastError);

					// retry ?
					if (lastError == KErrNone)
						{
						// clear error and try again
						currentOperation->iState = EStWriteToDisk;
						break;
						}

					iFlushError = lastError;
					PropagateFlushErrorToAllFileShares();

					__CACHE_PRINT2(_L("CACHEFILE: Resetting size from %ld to %ld"), iSize64, iFileCB->Size64());
 					SetSize64(iFileCB->Size64());
					}

				if (lastError != KErrNone)
					{
					// Destroy ALL cached data (dirty and non-dirty) !
					iCacheClient->Purge(ETrue);
					}
				else
					{
					// Mark segment as clean
					iCacheClient->MarkSegmentsAsClean(pos);
					}


				switch((TFlushMode) (TInt) currentOperation->iScratchValue0)
					{
					case EFlushSingle:
						currentOperation->iState = EStEnd;
						break;
					case EFlushHalf:
						if (iCacheClient->LockedSegmentsHalfUsed())
							currentOperation->iState = EStWriteToDisk;
						else
							currentOperation->iState = EStEnd;
						break;
					case EFlushAll:
						currentOperation->iState = EStWriteToDisk;
						break;
					default:
						ASSERT(0);
						break;
					}
				}
				break;

			case EStEnd:
				{
				iFlushBusy = EFalse;

				TUint8* addr;
				MarkFileClean();
				// Re-start dirty data timer if there is still some dirty data
				if (iCacheClient->FindFirstDirtySegment(pos, addr) != 0)
					FileDirty(aMsgRequest);

				return CFsRequest::EReqActionComplete;
				}
			}
		}
	}

/**
Handle a dirty data write error

Returns aError if dirty data should be thrown away or
		KErrNone if write should be retried 
*/
TInt CFileCache::HandleWriteDirtyError(TInt aError)
	{
	__THRD_PRINT3(_L(">TRACE: CFileCache::HandleWriteDirtyError() aError %d mounted %d changed %d"), aError, iDrive->IsMounted(), iDrive->IsChanged());

	// normally the disk change will have been detected by TDrive::CheckMount() but occasionally,
	// the change will occur while writing - in which case we need to mimick what TDrive::CheckMount does,
	// to make sure we are in a consisten state i.e. dismount the drive and set iCurrentMount to NULL
	if (iDrive->IsChanged())
		{
		iDrive->SetChanged(EFalse);
		if (iDrive->IsMounted())		//	Dismount the mount if it is still marked as mounted
            iDrive->Dismount();
		}

	// if error didn't occur because of a media eject, do nothing 
	if (aError == KErrNotReady && !iDrive->IsMounted())
		{
	
		TLocaleMessage line1;
		TLocaleMessage line2;
		line1=EFileServer_PutTheCardBackLine1;
		line2=EFileServer_PutTheCardBackLine2;

		//-- create Notifier  
		CAsyncNotifier* notifier = CAsyncNotifier::New(); 
		if( !notifier )
			{
			return aError;
			}

		notifier->SetMount(iMount);

		
		// While this (drive) thread is calling the notifier server (& effectively suspended), 
		// the main thread may call TFsFileRead::PostInitialise() or TFsFileWrite::PostInitialise() 
		// which would cause dead-lock unless we release the lock here. We also need to release 
		// the lock before calling CDriveThread::CompleteReadWriteRequests().
		iLock.Signal();	

		FOREVER
			{
			TInt buttonVal;

			TInt ret = notifier->Notify(
				TLocaleMessageText(line1),
				TLocaleMessageText(line2),
				TLocaleMessageText(EFileServer_Button1),
				TLocaleMessageText(EFileServer_Button2),
				buttonVal);
			if (ret!=KErrNone)
				break; 
			if (buttonVal!=1)
				break; // Abort


			// Wait up to 3 seconds for a disk change - this is because there is often a substantial 
			// (one/two second) delay between inserting a card & getting a notification that the card is ready;
			// if we give up too soon and fire of the notifier again, it's not very user-friendly !
			const TTimeIntervalMicroSeconds32 KHalfSecond(500000);
			const TInt KRetries = 6;;
			for (TInt n=0; !iDrive->IsChanged() && n<KRetries; n++)
				User::After(KHalfSecond);



			// Without this code, retry will indiscriminately write over whatever disk happens to be present.
			// However if the write error is to the bootsector remounting will always fail because the boot
			// sector will have changed and hence the disk is useless.
			// 
			OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREMOUNT2, "drive %d", DriveNumber());

			TInt remountSuccess = iDrive->ReMount(*iMount);

			OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREMOUNT2RET, "success %d", remountSuccess);
			if (!remountSuccess)
				continue;
			
			
			iMount->Drive().SetChanged(EFalse);

			aError = KErrNone; // Retry
			break;
			}

		delete notifier;


		// Cancel hung state 
		FsThreadManager::SetDriveHung(DriveNumber(), EFalse);

		
		// media had been removed and NOT replaced: so destroy ALL cached data 
		// (dirty and non-dirty) for all filecaches on this drive
		if (aError != KErrNone)
			{
			CDriveThread* pT=NULL;
			TInt r=FsThreadManager::GetDriveThread(DriveNumber(), &pT);
			if(r==KErrNone)
				{
				pT->CompleteReadWriteRequests();
				iDrive->PurgeDirty(*iMount);
				}
			}

		iLock.Wait();
		}


	return aError;
	}


/**
Mark file as dirty
*/
void CFileCache::FileDirty(CFsMessageRequest& aMsgRequest)
	{
	CSessionFs* session = aMsgRequest.Session();
	__ASSERT_ALWAYS(session, Fault(EDirtyDataOwnerNull));

	// Remember the last session which caused the file to become dirty
	// Always record whether any session has reserved access so the CheckDiskSpace() behaves correctly
	if (iDirtyDataOwner == NULL || session->ReservedAccess(iDriveNum))
		{
		if (iDirtyDataOwner)
			iDirtyDataOwner->Close();
		iDirtyDataOwner = session;
		// open the session to prevent it from being deleted while there is dirty data
		iDirtyDataOwner->Open();
		}

	// start a timer after which file will be flushed
	CDriveThread* driveThread=NULL;
	TInt r = FsThreadManager::GetDriveThread(iDriveNum, &driveThread);
	if(r == KErrNone && driveThread != NULL)
		{
		OstTrace1(TRACE_FILECACHE, FILECACHE_STARTING_DIRTY_DATA_TIMER, "Starting dirty data timer() fileCache %x", (TUint) this);

		iDirtyTimer.Start(driveThread, iDirtyDataFlushTime);
		}
	}

//----------------------------------------------------------------------------
/**
    Mark the file as clean and stop dirty data timer
*/
void CFileCache::MarkFileClean()
	{
	if (iDirtyDataOwner)
		iDirtyDataOwner->Close();

	iDirtyDataOwner = NULL;

	if (!iDriveThread)
		return;

	iDirtyTimer.Stop();
	}



//************************************
// TFileCacheSettings
//************************************

RArray<TFileCacheSettings::TFileCacheConfig>* TFileCacheSettings::iFileCacheSettings = NULL;



const TInt KDriveCacheSettingsArrayGranularity = 4;


TFileCacheSettings::TFileCacheConfig::TFileCacheConfig(TInt aDrive) : 
	iDrive(aDrive), 
	iFileCacheReadAsync(KDefaultFileCacheReadAsync),
	iFairSchedulingLen(KDefaultFairSchedulingLen << KByteToByteShift),
	iCacheSize(KDefaultFileCacheSize << KByteToByteShift),
	iMaxReadAheadLen(KDefaultFileCacheMaxReadAheadLen << KByteToByteShift),
	iClosedFileKeepAliveTime(KDefaultClosedFileKeepAliveTime  << KByteToByteShift),	// convert milliSecs -> microSecs (approximately)
	iDirtyDataFlushTime(KDefaultDirtyDataFlushTime  << KByteToByteShift)				// convert milliSecs -> microSecs (approximately)
	{
	iFlags = ConvertEnumToFlags(KDefaultFileCacheRead, KDefaultFileCacheReadAhead, KDefaultFileCacheWrite);
	}


TFileCacheFlags TFileCacheSettings::TFileCacheConfig::ConvertEnumToFlags(const TInt aFileCacheRead, const TInt aFileCacheReadAhead, const TInt aFileCacheWrite) 
	{
	TInt flags(0);

	// read caching
	if (aFileCacheRead == EFileCacheFlagEnabled)
		flags|= EFileCacheReadEnabled;
	else if (aFileCacheRead == EFileCacheFlagOn)
		flags|= EFileCacheReadEnabled | EFileCacheReadOn;

	// read ahead
	if (aFileCacheReadAhead == EFileCacheFlagEnabled)
		flags|= EFileCacheReadAheadEnabled;
	else if (aFileCacheReadAhead == EFileCacheFlagOn)
		flags|= EFileCacheReadAheadEnabled | EFileCacheReadAheadOn;

	// write caching
	if (aFileCacheWrite == EFileCacheFlagEnabled)
		flags|= EFileCacheWriteEnabled;
	else if (aFileCacheWrite == EFileCacheFlagOn)
		flags|= EFileCacheWriteEnabled | EFileCacheWriteOn;

	return TFileCacheFlags(flags);
	}


static const TPtrC8 KCacheFlagEnumStrings[]=
	{
	_S8("OFF"),
	_S8("ENABLED"),
	_S8("ON"),
	_S8(""),	// terminator
	};
const TInt KMaxEnumLen = 7;

void TFileCacheSettings::ReadEnum(const TDesC8& aSection, const TDesC8& aProperty, TInt32& aEnumVal, const TPtrC8* aEnumStrings)
	{
	TBuf8<KMaxEnumLen> buf;
	if (!F32Properties::GetString(aSection, aProperty, buf))
		return;
	TInt n;
	const TPtrC8* enumString;
	for (enumString=aEnumStrings, n=0; enumString->Length()!= 0; enumString++, n++)
		{
		if (buf.LeftTPtr(enumString->Length()).MatchF(*enumString) == 0)
			{
			aEnumVal = n;
			break;
			}
		}
	}
	
TInt TFileCacheSettings::ReadPropertiesFile(TInt aDriveNumber)
	{
	Init();


	if (!TGlobalFileCacheSettings::Enabled())
		return KErrNone;

	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDriveNumber, driveCacheSettings);
	if (r != KErrNone)
		return r;

	// restore default settings in case they've been changed by SetFlags()
	driveCacheSettings->iFlags = TFileCacheConfig::ConvertEnumToFlags(KDefaultFileCacheRead, KDefaultFileCacheReadAhead, KDefaultFileCacheWrite);


	// Get file cache configuration settings for this drive
	// N.B. Size/length values are specified in Kilobytes, timer values in Milliseconds
	TBuf8<8> sectionName;
	F32Properties::GetDriveSection(aDriveNumber, sectionName);

	TInt32 val;
	//  Read FileCacheSize
	if (F32Properties::GetInt(sectionName, _L8("FileCacheSize"),		val))
		driveCacheSettings->iCacheSize = val << KByteToByteShift;

	// ensure read-ahead len is not greater than the cache size
	driveCacheSettings->iMaxReadAheadLen = 
		Min(driveCacheSettings->iMaxReadAheadLen, driveCacheSettings->iCacheSize);

	// Read FileCacheReadAsync
	TBool bVal;
	if (F32Properties::GetBool(sectionName, _L8("FileCacheReadAsync"), bVal))
		driveCacheSettings->iFileCacheReadAsync = bVal;

	// Read FairSchedulingLen
	if (F32Properties::GetInt(sectionName, _L8("FairSchedulingLen"),	val))
		driveCacheSettings->iFairSchedulingLen = val << KByteToByteShift;

	// Read ClosedFileKeepAliveTime - convert miliSecs to microSecs (approximately)
	if (F32Properties::GetInt(sectionName, _L8("ClosedFileKeepAliveTime"),	val))
		driveCacheSettings->iClosedFileKeepAliveTime = val << KByteToByteShift;

	// Read DirtyDataFlushTime - convert miliSecs to microSecs (approximately)
	if (F32Properties::GetInt(sectionName, _L8("DirtyDataFlushTime"),	val))
		driveCacheSettings->iDirtyDataFlushTime = val << KByteToByteShift;

	// get read, read-ahead and write states
	TInt32 readVal = KDefaultFileCacheRead;
	TInt32 readAheadVal = KDefaultFileCacheReadAhead;
	TInt32 writeVal = KDefaultFileCacheWrite;

	ReadEnum(sectionName, _L8("FileCacheRead"), readVal, &KCacheFlagEnumStrings[0]);
	ReadEnum(sectionName, _L8("FileCacheReadAhead"), readAheadVal, &KCacheFlagEnumStrings[0]);
	ReadEnum(sectionName, _L8("FileCacheWrite"), writeVal, &KCacheFlagEnumStrings[0]);
	driveCacheSettings->iFlags = TFileCacheConfig::ConvertEnumToFlags(readVal, readAheadVal, writeVal);

	__CACHE_PRINT7(_L("ReadPropertiesFile() drive %C flags %08X CacheSize %d FileCacheReadAsync %d iFairSchedulingLen %d iClosedFileKeepAliveTime %d iDirtyDataFlushTime %d\n"),
		aDriveNumber + 'A', 
		driveCacheSettings->iFlags, 
		driveCacheSettings->iCacheSize, 
		driveCacheSettings->iFileCacheReadAsync,
		driveCacheSettings->iFairSchedulingLen,
		driveCacheSettings->iClosedFileKeepAliveTime,
		driveCacheSettings->iDirtyDataFlushTime);
	
	OstTraceExt5(TRACE_FILECACHE, FILECACHE_SETTINGS, 
		"Drive %d Flags %x CacheSize %d FileCacheReadAsync %d FairSchedulingLen %d ", 
		(TUint) aDriveNumber,
		(TUint) driveCacheSettings->iFlags, 
		(TUint) driveCacheSettings->iCacheSize, 
		(TUint) driveCacheSettings->iFileCacheReadAsync,
		(TUint) driveCacheSettings->iFairSchedulingLen);

	return KErrNone;
	}


TInt TFileCacheSettings::GetFileCacheConfig(TInt aDrive, TFileCacheConfig*& aConfig)
	{
	Init();

	aConfig = NULL;

	TFileCacheConfig driveCacheSettingsToMatch(aDrive);
	
	TInt index = iFileCacheSettings->FindInUnsignedKeyOrder(driveCacheSettingsToMatch);
	if (index == KErrNotFound)
		{
		// create a new entry. 
		TInt r = iFileCacheSettings->InsertInUnsignedKeyOrder(driveCacheSettingsToMatch);
		if (r != KErrNone)
			return r;
		index = iFileCacheSettings->FindInUnsignedKeyOrder(driveCacheSettingsToMatch);
		__ASSERT_ALWAYS(index != KErrNotFound, Fault(ECacheSettingsNotFound));
		}

	aConfig = &(*iFileCacheSettings)[index];
	return KErrNone;
	}

void TFileCacheSettings::SetFlags(TInt aDrive, TFileCacheFlags aFlags)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	driveCacheSettings->iFlags = aFlags;
	}

TFileCacheFlags TFileCacheSettings::Flags(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iFlags;
	}

void TFileCacheSettings::Init()
	{
	if (iFileCacheSettings == NULL)
		{
		iFileCacheSettings = new RArray<TFileCacheConfig>(KDriveCacheSettingsArrayGranularity, _FOFF(TFileCacheConfig, iDrive));
		__ASSERT_ALWAYS(iFileCacheSettings != NULL, Fault(ECacheSettingsInitFailed));
		}
	}


TBool TFileCacheSettings::FileCacheReadAsync(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iFileCacheReadAsync;
	}

TInt TFileCacheSettings::FairSchedulingLen(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iFairSchedulingLen;
	}

TInt TFileCacheSettings::MaxReadAheadLen(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iMaxReadAheadLen;
	}

TInt TFileCacheSettings::CacheSize(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iCacheSize;
	}

TInt TFileCacheSettings::ClosedFileKeepAliveTime(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iClosedFileKeepAliveTime;
	}


TInt TFileCacheSettings::DirtyDataFlushTime(TInt aDrive)
	{
	TFileCacheConfig* driveCacheSettings;
	TInt r = GetFileCacheConfig(aDrive, driveCacheSettings);
	__ASSERT_ALWAYS(r == KErrNone && driveCacheSettings != NULL, Fault(ECacheSettingGetFailed));

	return driveCacheSettings->iDirtyDataFlushTime;
	}

//************************************
// TClosedFileUtils
//************************************

CFsObjectCon* TClosedFileUtils::iClosedFiles = NULL;

void TClosedFileUtils::InitL()
	{
	iClosedFiles = TheContainer->CreateL();
	if (iClosedFiles == NULL)
		User::LeaveIfError(KErrNoMemory);

	}


TBool TClosedFileUtils::IsClosed(CFileCache* aFileCache)
	{
	return (aFileCache->Container() == iClosedFiles);
	}

TInt TClosedFileUtils::Count()
	{
	return iClosedFiles->Count();
	}

CFileCache* TClosedFileUtils::At(TInt aIndex)
	{
	return (CFileCache*) (*iClosedFiles)[aIndex];
	}


// Add a closed file to closed file container
void TClosedFileUtils::AddL(CFileCache* aFileCache, TBool aLock)
	{
	if (aFileCache->iDriveThread)
		{
		iClosedFiles->AddL(aFileCache, aLock);

		// start a timer after which file will be removed from closed queue
		CDriveThread* driveThread=NULL;
		TInt r = FsThreadManager::GetDriveThread(aFileCache->iDriveNum, &driveThread);
		if(r == KErrNone && driveThread != NULL)
			aFileCache->iClosedTimer.Start(driveThread, aFileCache->iClosedFileKeepAliveTime);
		}
	}



// Remove a closed file from closed file container so that it can be re-opened
void TClosedFileUtils::ReOpen(CFileCache* aFileCache, TBool aLock)
	{
	// get the drive thread in case it has changed since the file was last open
	const TInt r = FsThreadManager::GetDriveThread(aFileCache->iDriveNum, &aFileCache->iDriveThread);
	if ((r == KErrNone) && aFileCache->iDriveThread)
		aFileCache->iClosedTimer.Stop();

	iClosedFiles->Remove(aFileCache, aLock);
	}

// Remove all closed files from closed file container and close them for good
void TClosedFileUtils::Remove()
	{
	RemoveFiles(NULL, NULL);
	}

// Remove all closed files belonging to a particular TDrive from closed file container and close them for good
void TClosedFileUtils::Remove(TInt aDrvNumber)
	{
	RemoveFiles(&TClosedFileUtils::TestDrive, (TAny*) aDrvNumber);
	}

// Remove a closed file from closed file container and close it for good
void TClosedFileUtils::Remove(CFileCache* aFileCache)
	{
	RemoveFiles(&TClosedFileUtils::TestFile, (TAny*) aFileCache);
	}

void TClosedFileUtils::RemoveFiles(TTestFunc aTestFunc, TAny* aTestVal)
	{
	Lock();

	TInt count = TClosedFileUtils::Count();
	while(count--)
		{
		CFileCache& file = *(CFileCache*)(*iClosedFiles)[count];
		if ((aTestFunc == NULL) || ((*aTestFunc)(file, aTestVal)))
			{
			iClosedFiles->Remove(&file, EFalse);
			file.Close();
			}
		}

	Unlock();
	}


TBool TClosedFileUtils::TestDrive(CFileCache& aFileCache, TAny* aVal)
	{
	TBool r = (aFileCache.iDriveNum == (TInt) aVal)?(TBool) ETrue: (TBool) EFalse;
	if (r)
		{
		__CACHE_PRINT1(_L("CLOSEDFILES: TestDrive() closing %S\n"), &aFileCache.FileNameF() );
		}
	return r;
	}
TBool TClosedFileUtils::TestFile(CFileCache& aFileCache, TAny* aVal)
	{
	TBool r = (&aFileCache == ((CFileCache*) aVal))?(TBool)ETrue:(TBool)EFalse;
	if (r)
		{
		__CACHE_PRINT1(_L("CLOSEDFILES: TestFile() closing %S\n"), &aFileCache.FileNameF() );
		}
	return r;
	}

	
	
void TClosedFileUtils::Lock()
	{
	iClosedFiles->Lock();
	}

void TClosedFileUtils::Unlock()
	{
	iClosedFiles->Unlock();
	}
	
	
