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
// f32\sfile\sf_cache_client.cpp
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
#include "sf_cache_man.h"
#include "sf_cache_client.h"
#include "sf_file_cache_defs.h"

// This macro chnages the code so that new cachelines are stolen from the queue (if it is full)
// thus reducing the calls to RChunk::Commit
#define SYMBIAN_REUSE_STALE_CACHELINES

enum TCacheClientFault
	{
	ECacheClientSegmentNotFound0,
	ECacheClientSegmentNotFound1,
	ECacheClientSegmentNotFound2,
	ECacheClientSegmentNotFound3,
	ECacheClientSegmentNotFound4,
	ECacheClientSegmentNotFound5,
	ECacheClientSegmentNotFound6,
	ECacheClientSegmentNotFound7,
	ECacheClientLruInsertFailed,
	ECacheClientInsertFailed,
	ECacheClientRemovingBusySegment,
	ECacheClientRemovingLockedSegment,
	ECacheClientRemovingDirtySegment,
	ECacheClientInvalidSegmentCount,
	ECacheClientAlreadyLocked,
	ECacheClientSegmentNotLocked,
	ECacheClientSegmentNotDirty,
	ECacheClientSegmentAlreadyUnlocked,
	ECacheClientSegmentBadLockCount,
	ECacheClientTotalByteCountInvalid,
	ECacheClientUnexpectedHole,
	ECacheClientSegmentAlreadyAllocated
	};


LOCAL_C void Fault(TCacheClientFault aFault)
//
// Report a fault in the cache client
//
	{
	User::Panic(_L("FSCACHECLIENT"), aFault);
	}


const TInt KSegmentArrayGranularity = 4;


//******************************
// CCacheClient
//******************************

CCacheClient* CCacheClient::NewL(CCacheManager& aManager)
	{
	CCacheClient* client = new(ELeave) CCacheClient(aManager);
	
	CleanupStack::PushL(client);
	client->ConstructL();
	CleanupStack::Pop(1, client);

	return client;
	}

void CCacheClient::ConstructL()
	{
	iCacheLines = new(ELeave) RPointerArray<CCacheManager::TCacheLine>(KSegmentArrayGranularity);
	
	iLastCacheLineIndex = -1;
	}

CCacheClient::CCacheClient(CCacheManager& aManager) : iManager(aManager)
	{
	__CACHE_PRINT(_L("CACHECLIENT: CCacheClient()"));
	
	iMaxBytesCached = 0;
	}

/** 
Purge() - free all cachelines
if aPurgeDirty == ETrue then discard all dirty data 
				  E.g. if called from the destructor or following media removal
*/
void CCacheClient::Purge(TBool aPurgeDirty)
	{
	TInt cacheLineCount = iCacheLines->Count();
	TInt n;
	for (n=0; n<cacheLineCount; n++)
		{
		CCacheManager::TCacheLine& cacheLine = *(*iCacheLines)[n];

		
		// Purge any locked and/or dirty cachlines if we still own them 
		// Freeing any locked or dirty pages implies that flushing the file 
		// has failed for some reason (media removal for example).
		//
		// NB if we own a cacheline and it is "in use" it's an error
		// NB we need to check if it's locked first and then if we own it
		// to guard against it being stolen by another thread.
		if (aPurgeDirty && iManager.DirtyCount(this, cacheLine) != 0 && cacheLine.iClient == this)
			{
#ifdef _DEBUG
			if (iManager.LockCount(this, cacheLine) != 0)
				__CACHE_PRINT(_L("CACHECLIENT: purging with locked segments"));
			if (iManager.DirtyCount(this, cacheLine) != 0)
				__CACHE_PRINT(_L("CACHECLIENT: purging dirty segments"));
			__ASSERT_DEBUG(!iManager.InUse(cacheLine), Fault(ECacheClientRemovingBusySegment));
#endif
			if (iManager.DirtyCount(this, cacheLine) != 0)
				iManager.ClearDirty(this, cacheLine);
			while (iManager.LockCount(this, cacheLine) != 0)
				iManager.UnlockCacheLine(this, cacheLine);
			}	// if aPurgeDirty

		TBool success = iManager.FreeCacheLine(this, cacheLine);
		
		// if (aPurgeDirty == ETrue), then it is an error
		// if a cacheline could not be freed (because  it was locked).
		if (aPurgeDirty)
			{
			__ASSERT_ALWAYS(success, Fault(ECacheClientRemovingLockedSegment));
			}
		}

	}

CCacheClient::~CCacheClient()
	{
	__CACHE_PRINT1(_L("CACHECLIENT: ~CCacheClient() this %08X"), this);

	if (iCacheLines)
		{
		Purge(ETrue);
		iCacheLines->Close();	
		}

	delete iCacheLines;
	}

void CCacheClient::SetMaxSegments(TInt aMaxSegments)
	{
	iMaxBytesCached = aMaxSegments << iManager.SegmentSizeLog2();
	}


TInt CCacheClient::SegmentSize()
	{
	return iManager.SegmentSize();
	}

TInt CCacheClient::SegmentSizeLog2()
	{
	return iManager.SegmentSizeLog2();
	}

TInt64 CCacheClient::SegmentSizeMask()
	{
	return iManager.SegmentSizeMask();
	}

TInt CCacheClient::CacheLineSize()
	{
	return iManager.CacheLineSize();
	}

TInt CCacheClient::CacheLineSizeInSegments()
	{
	return iManager.CacheLineSizeInSegments();
	}

TUint8* CCacheClient::FindAndLockSegments(TInt64 aPos, TInt& aSegmentCount, TInt& aFilledSegmentCount, TInt& aLockError, TBool aMakeDirty)
	{
	TInt maxSegments = Min(aSegmentCount, CacheLineSizeInSegments());
	aSegmentCount = 0;
	aFilledSegmentCount = 0;

	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	aLockError = KErrNone;

	if (!cacheLine)
		return NULL;

	TInt offset = (TInt) (aPos - cacheLine->iPos);
	TInt len = CacheLineLen(cacheLine) - offset;
	
	TInt segmentSizeLog2 = iManager.SegmentSizeLog2();

	TInt startSegmentNumber = offset >> segmentSizeLog2;
	TInt filledCount = iManager.FillCount(this, *cacheLine);
	
	// round up to a whole number of segments
	aSegmentCount = ((len + iManager.SegmentSize() - 1) >> segmentSizeLog2);
	aSegmentCount = Min(aSegmentCount, maxSegments);

	// get filled count
	aFilledSegmentCount = (filledCount > startSegmentNumber)?filledCount - startSegmentNumber:0;

	__CACHE_PRINT4(_L("CACHECLIENT: FindAndLockSegments(), Client %08X pos %d addr %08X len %d"), 
		this, I64LOW(cacheLine->iPos), cacheLine->iAddr, CacheLineLen(cacheLine));


	aLockError = LockSegments(aPos, aSegmentCount, aMakeDirty);
	if (aLockError != KErrNone)
		return NULL;

	return cacheLine->iAddr + offset;
	}

TUint8* CCacheClient::FindSegment(TInt64 aPos)
	{
	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	if (!cacheLine)
		return NULL;

	TInt offset = (TInt) (aPos - cacheLine->iPos);
	return cacheLine->iAddr + offset;
	}

TUint8* CCacheClient::AllocateAndLockSegments(TInt64 aPos, TInt& aSegmentCount, TBool aMakeDirty, TBool aExtendExisting)
	{
	__ASSERT_DEBUG(aSegmentCount >= 0, Fault(ECacheClientInvalidSegmentCount));

	TInt segmentSizeLog2 = iManager.SegmentSizeLog2();
	TInt segmentSize = iManager.SegmentSize();
	TInt cacheLineSize = iManager.CacheLineSize();

	aPos = (aPos >> segmentSizeLog2) << segmentSizeLog2;


	// count number of uncached segments
	TInt maxSegments = Min(aSegmentCount, CacheLineSizeInSegments());

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// This is test code and changes the behaviour so that we always allocate
	// as large a cacheline as possible, so as to increase the likelihood of "holes"
	if (iManager.AllocateMaxSegments())
		{
		maxSegments = cacheLineSize >> segmentSizeLog2;
		}
#endif

	// make sure requested range doesn't overlap with an existing cacheline - 
	// if it does, shrink requested length down
	TInt64 endPos = aPos + (maxSegments << segmentSizeLog2);
	TInt cacheLineCount = iCacheLines->Count();
	for (TInt index=0; index < cacheLineCount; index++)
		{
		const CCacheManager::TCacheLine* cacheLine = (*iCacheLines)[index];
		TInt64 startPos = cacheLine->iPos;
		if (cacheLine->iClient == this && startPos >= aPos && startPos < endPos)
			{
			endPos = cacheLine->iPos;
			}
		}
	TInt segmentCount = (TInt) ((endPos - aPos) >> segmentSizeLog2);
	
	__ASSERT_DEBUG(segmentCount > 0, Fault(ECacheClientSegmentAlreadyAllocated));
	
	aSegmentCount = Min(segmentCount, aSegmentCount);

	iLastCacheLineIndex = -1;

	TUint8* segmentAddr = NULL;

	__CACHE_PRINT2(_L("CACHECLIENT: AllocateAndLockSegments(%ld, %d)"), aPos, segmentCount);

	// can we extend an existing cacheline ? (this saves wasting virtual memory)
	// NB if we're about the make the new segment(s) dirty, only extend if last segment in existing
	// cacheline is dirty, otherwise we'll end up unnecessarily marking lots of clean segments as dirty
	if (aExtendExisting)
		{
		TInt64 posLastSegment = aPos - segmentSize;
		const CCacheManager::TCacheLine* cacheLineOld;
		TInt r;
		if ((posLastSegment >= 0) && ((cacheLineOld = FindCacheLine(posLastSegment)) != NULL))
			{
			TInt prevSegmentNumber = StartSegment(cacheLineOld, posLastSegment);
			TInt fillCountOld = iManager.FillCount(this, *cacheLineOld);
			TInt fillCountNew = segmentCount + prevSegmentNumber+1;	// fillCountOld;
			if (prevSegmentNumber+1 == fillCountOld &&
				fillCountNew <= (cacheLineSize >> segmentSizeLog2) &&
				(!aMakeDirty || (prevSegmentNumber+1 == cacheLineOld->iDirtySegments))
				)
				{
				__CACHE_PRINT(_L("CACHE: extending cacheline"));

				segmentAddr = cacheLineOld->iAddr + (TInt) (aPos - cacheLineOld->iPos);

				// Lock the cacheline and then try to extend it
				r = LockSegments(cacheLineOld->iPos, cacheLineOld->iAllocatedSegments, ETrue);
				if (r == KErrNone)
					{
					r = iManager.ExtendCacheLine(this, *cacheLineOld, fillCountNew);
//RDebug::Print(_L("GROW addr %08X, prevSegmentNumber %d, fillCount %d, fillCountNew %d, r %d\n"),
//			  cacheLineOld->iAddr, prevSegmentNumber, fillCountOld, fillCountNew, r);
					if (r == KErrNone)
						{
						// If we've exceeded max, use LRU queue to discard old cachelines
						RemoveLru(cacheLineOld);
						return segmentAddr;
						}
					__CACHE_PRINT(_L("LockSegments FAIL"));
					UnlockSegments(posLastSegment);	
					}
				}
			}
		}

	// reserve space in the segment array & LRU queue so we don't have to 
	// worry about failing to append
	if (iCacheLines->Reserve(1) != KErrNone)
		return NULL;

	const CCacheManager::TCacheLine* cacheLineNew = NULL;
	TBool reUsedCacheLineGrown = EFalse;

#ifdef SYMBIAN_REUSE_STALE_CACHELINES
	// try to re-use an old cacheline if possible to avoid having to commit a new one
	// as this involves calling RChunk::Commit() which takes some time....
	TInt lenNewCacheLine = segmentCount << segmentSizeLog2;
	TInt totalBytesCached = CachedBytes() + lenNewCacheLine;
	
	cacheLineCount = iCacheLines->Count();
	if (cacheLineCount > 0)
		{
		const CCacheManager::TCacheLine* cacheLineOld = (*iCacheLines)[cacheLineCount - 1];
		TInt lenOldCacheLine = cacheLineOld->iFilledSegments << segmentSizeLog2;
		reUsedCacheLineGrown = segmentCount > cacheLineOld->iAllocatedSegments;
		if (totalBytesCached + lenNewCacheLine - lenOldCacheLine > iMaxBytesCached &&
			cacheLineOld->iClient == this &&
			cacheLineOld->iLockCount == 0)
			{
			if (iManager.ReAllocateAndLockCacheLine(this, aPos, *cacheLineOld, segmentCount) == KErrNone)
				{
				cacheLineNew = cacheLineOld;
				}
			}
		}
#endif	// SYMBIAN_REUSE_STALE_CACHELINES
	TBool reUsingCacheline = cacheLineNew ? (TBool)ETrue:(TBool)EFalse;

	// Request a free segment from cache manager
	if (reUsingCacheline)
		{
		iCacheLines->Remove(cacheLineCount-1);
		}
	else
		{
		TInt r = iManager.AllocateAndLockCacheLine(this, aPos, cacheLineNew, segmentCount);
		if (r != KErrNone)
			{
			__CACHE_PRINT1(_L("CACHECLIENT: AllocateSegment() failed r %d"), r);
			return NULL;
			}
		// check the address isn't already in the iCacheLines - 
		// this can happen if it was stolen from us and we then steal it back
		TInt indexDup = iCacheLines->Find(cacheLineNew);
		if (indexDup != KErrNotFound)
			iCacheLines->Remove(indexDup);
		}


	segmentAddr = cacheLineNew->iAddr;

	// Add to top of LRU queue
	TInt r = iCacheLines->Insert(cacheLineNew, 0);
	__ASSERT_ALWAYS(r == KErrNone, Fault(ECacheClientInsertFailed));

	__CACHE_PRINT4(_L("CACHECLIENT: AllocateAndLockSegments(), Client %08X pos %d addr %08X len %d"), 
		this, I64LOW(cacheLineNew->iPos), cacheLineNew->iAddr, CacheLineLen(cacheLineNew));

	// assume LRU in use ...
	iLastCacheLineIndex = 0;

	if (!reUsingCacheline || reUsedCacheLineGrown)
		RemoveLru(cacheLineNew);

	return segmentAddr;
	}



TInt CCacheClient::StartSegment(const CCacheManager::TCacheLine* aCacheLine, TInt64 aPos)
	{
	TInt offset = (TInt) (aPos - aCacheLine->iPos);
	return offset >> iManager.SegmentSizeLog2();
	}

TInt CCacheClient::CacheLineLen(const CCacheManager::TCacheLine* aCacheLine)
	{
	return aCacheLine->iAllocatedSegments << iManager.SegmentSizeLog2();
	}


void CCacheClient::MarkSegmentsAsFilled(TInt64 aPos, TInt aSegmentCount)
	{
	__CACHE_PRINT2(_L("CACHECLIENT: Mark Filled Segments(%ld, %d)"), aPos, aSegmentCount);

	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound0));

	iManager.SetFilled(this, *cacheLine,  StartSegment(cacheLine, aPos) + aSegmentCount);
	}

void CCacheClient::MarkSegmentsAsDirty(TInt64 aPos, TInt aSegmentCount)
	{
	__CACHE_PRINT2(_L("CACHECLIENT: Mark Dirty Segments(%ld, %d)"), aPos, aSegmentCount);

	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound1));

	iManager.SetDirty(this, *cacheLine,  StartSegment(cacheLine, aPos) + aSegmentCount);
	}

TInt CCacheClient::FindFirstDirtySegment(TInt64& aPos, TUint8*& aAddr)
	{
	TInt cacheLineCount = iCacheLines->Count();

	aPos = KMaxTInt64;
	TInt segmentCount = 0;
	for (TInt index = 0; index< cacheLineCount; index++)
		{
		CCacheManager::TCacheLine& cacheLine = *(*iCacheLines)[index];

		TInt dirtyCount = iManager.DirtyCount(this, cacheLine);
		if ((dirtyCount > 0 && cacheLine.iPos < aPos))
			{
			aPos = cacheLine.iPos;
			aAddr = cacheLine.iAddr;
			segmentCount = dirtyCount;
			}
		}

	return segmentCount;
	}


void CCacheClient::MarkSegmentsAsClean(TInt64 aPos)
	{
	__CACHE_PRINT1(_L("CACHECLIENT: MarkSegmentsAsClean(%d)"), I64LOW(aPos));
	
	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound2));

	// this sets the dirty count to zero, unlocks the cacheline and 
	// marks all the dirty pages as filled 
	iManager.ClearDirty(this, *cacheLine);

	RemoveLru(cacheLine);
	}


void CCacheClient::RemoveEmptySegments(const CCacheManager::TCacheLine* aCacheLine)
	{
	iManager.RemoveEmptySegments(this, *aCacheLine);
	}

TInt CCacheClient::LockSegments(TInt64 aPos, TInt aSegmentCount, TBool aMakeDirty)
	{
	__CACHE_PRINT1(_L("CACHECLIENT: LockCacheLine(%d, %d)"), I64LOW(aPos));

	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound3));


	TInt startSegmentNumber = StartSegment(cacheLine, aPos);
	
	TInt lockError;
	if (aMakeDirty)
		lockError = iManager.LockCacheLine(this, *cacheLine, 0, cacheLine->iAllocatedSegments);
	else
		lockError = iManager.LockCacheLine(this, *cacheLine, startSegmentNumber, aSegmentCount);

	if (lockError == KErrInUse)
		{
		return lockError;
		}
	else if (lockError != KErrNone)
		{
		RemoveCacheLine(cacheLine);
		return lockError;
		}

	// Check for "holes".i.e. empty segments before this one.
	// If found, discard the empty segments and return KErrNotFound
	TInt filledCount = iManager.FillCount(this, *cacheLine);
	if (startSegmentNumber > filledCount)
		{
		__CACHE_PRINT(_L("CACHE: found hole"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		iManager.Stats().iHoleCount++;
#endif


		RemoveEmptySegments(cacheLine);
		iManager.UnlockCacheLine(this, *cacheLine);
		if (CacheLineLen(cacheLine) == 0)
			FreeCacheLine(cacheLine);
		return KErrNotFound;
		}

	// Move this entry to top of LRU queue
	iCacheLines->Remove(iLastCacheLineIndex);
#ifdef _DEBUG
	TInt r = 
#endif
	iCacheLines->Insert(cacheLine, 0);
	__ASSERT_DEBUG(r == KErrNone, Fault(ECacheClientLruInsertFailed));
	iLastCacheLineIndex = 0;

	
	return KErrNone;
	}


/**
UnlockSegments()
*/
void CCacheClient::UnlockSegments(TInt64 aPos)
	{
	__CACHE_PRINT1(_L("CACHECLIENT: UnlockSegments(%d, %d)"), I64LOW(aPos));

	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound4));

	iManager.UnlockCacheLine(this, *cacheLine);
	}


TInt CCacheClient::FindCacheLineIndex(TInt64 aPos)
	{
	TInt cacheLineCount = iCacheLines->Count();

	if 	(iLastCacheLineIndex != -1 && iLastCacheLineIndex < cacheLineCount)
		{
		const CCacheManager::TCacheLine* cacheLine = (*iCacheLines)[iLastCacheLineIndex];

		if (cacheLine->iClient == this && 
			aPos >= cacheLine->iPos &&
			aPos < cacheLine->iPos+CacheLineLen(cacheLine))
			return iLastCacheLineIndex;
		}		

	for (TInt index=0; index < cacheLineCount; index++)
		{
		const CCacheManager::TCacheLine* cacheLine = (*iCacheLines)[index];

		if (cacheLine->iClient == this && 
			aPos >= cacheLine->iPos &&
			(aPos < cacheLine->iPos+CacheLineLen(cacheLine)))
			{
			iLastCacheLineIndex = index;
			return index;
			}
		}
		
	return KErrNotFound;
	}

const CCacheManager::TCacheLine*  CCacheClient::FindCacheLine(TInt64 aPos)
	{
	TInt index = FindCacheLineIndex(aPos);

	return (index == KErrNotFound) ? NULL : (*iCacheLines)[index];
	}


TBool CCacheClient::SegmentEmpty(TInt64 aPos)
	{
	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound5));

	TInt startSegment = StartSegment(cacheLine, aPos);
	TInt fillCount = iManager.FillCount(this, *cacheLine);


	return startSegment >= fillCount ? (TBool)ETrue : (TBool)EFalse;
	}


TBool CCacheClient::SegmentDirty(TInt64 aPos)
	{
	const CCacheManager::TCacheLine* cacheLine = FindCacheLine(aPos);

	__ASSERT_ALWAYS(cacheLine != NULL, Fault(ECacheClientSegmentNotFound6));

	TInt startSegment = StartSegment(cacheLine, aPos);
	TInt dirtyCount = iManager.DirtyCount(this, *cacheLine);

	return startSegment >= dirtyCount ? (TBool)ETrue : (TBool)EFalse;
	}

// return whether there are too many locked pages globally (for all files)
// or too many for this file
TBool CCacheClient::TooManyLockedSegments()
	{
	if (iManager.TooManyLockedSegments())
		return ETrue;
	if (LockedBytes() > iMaxBytesCached)
		return ETrue;
	return EFalse;
	}

// return true if more than half of the segments in this file are locked
TBool CCacheClient::LockedSegmentsHalfUsed()
	{
	if (LockedBytes() > (iMaxBytesCached >> 1))
		return ETrue;
	return EFalse;
	}

TInt CCacheClient::CachedBytes()
	{
	TInt cacheLineCount = iCacheLines->Count();

	TInt totalBytesCached = 0;
	for (TInt n=0; n < cacheLineCount; n++)
		totalBytesCached+= ((*iCacheLines)[n]->iFilledSegments << iManager.SegmentSizeLog2());
	return totalBytesCached;
	}

TInt CCacheClient::LockedBytes()
	{
	TInt cacheLineCount = iCacheLines->Count();
	TInt segmentSizeLog2 = iManager.SegmentSizeLog2();

	TInt totalBytesLocked = 0;
	for (TInt n=0; n < cacheLineCount; n++)
		{
		CCacheManager::TCacheLine& cacheLine = *(*iCacheLines)[n];
		if (cacheLine.iLockCount > 0)
			totalBytesLocked+=  cacheLine.iAllocatedSegments << segmentSizeLog2;
		}
	return totalBytesLocked;
	}

void CCacheClient::RemoveLru(const CCacheManager::TCacheLine* aCacheLineToExclude)
	{
	if (iMaxBytesCached == 0)	// LRU queue ?
		return;

	// If we've exceeded max, use LRU queue to remove stuff

	TInt cacheLineCount = iCacheLines->Count();
	TInt segmentSizeLog2 = iManager.SegmentSizeLog2();

	// calculate the total bytes cached in all cachelines we own
	// NB it's difficult to keep a track of this in this class because cachelines can be stolen (!)
	TInt totalBytesCached = CachedBytes();
	for (TInt lastIndex = cacheLineCount - 1; 
		lastIndex >= 0 && totalBytesCached > iMaxBytesCached;
		lastIndex--)
		{
		const CCacheManager::TCacheLine* cacheLine = (*iCacheLines)[lastIndex];

//		__CACHE_PRINT3(_L("Test cacheline index %d pos %ld len %d"), lastIndex, cacheLine.iPos, cacheLine.iLen);

		if (cacheLine != aCacheLineToExclude)
			{
			// if removing a cacheline will bring the total to UNDER the "maximum"
			// then don't remove it and abort - to do otherwise could remove
			// data that has just been read-ahead ....
			TInt lenCacheLine = cacheLine->iFilledSegments << segmentSizeLog2;
			if (totalBytesCached - lenCacheLine	<= iMaxBytesCached)
				{
				__CACHE_PRINT(_L("CACHECLIENT: cacheline too big to remove, aborting.."));
				break;
				}

			__CACHE_PRINT3(_L("CACHECLIENT: Removing LRU index %d pos %ld len %d"), lastIndex, cacheLine->iPos, CacheLineLen(cacheLine));
			TInt bytesFreed = cacheLine->iFilledSegments << iManager.SegmentSizeLog2();
			if (FreeCacheLine(cacheLine))
				{
				totalBytesCached-= bytesFreed;
				__ASSERT_ALWAYS(totalBytesCached >= 0, Fault(ECacheClientTotalByteCountInvalid));
				}
			}
		}
	}



TBool CCacheClient::FreeCacheLine(const CCacheManager::TCacheLine* aCacheLine)
	{
	__CACHE_PRINT1(_L("CACHECLIENT: FreeCacheLine(%d) )"), I64LOW(aCacheLine->iPos));


	TBool success = iManager.FreeCacheLine(this, *aCacheLine);

	if (success)
		RemoveCacheLine(aCacheLine);

	return success;
	}

void CCacheClient::RemoveCacheLine(const CCacheManager::TCacheLine* aCacheLine)
	{
	__CACHE_PRINT1(_L("CACHECLIENT: RemoveCacheLine(%d) )"), I64LOW(aCacheLine->iPos));

	TInt index = iCacheLines->Find(aCacheLine);

	__ASSERT_ALWAYS(index != KErrNotFound, Fault(ECacheClientSegmentNotFound7));

	iCacheLines->Remove(index);
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void CCacheClient::DumpCache()
	{
	RDebug::Print(_L("**** CACHECLIENT: CacheLines ****\n"));
	TInt cacheLineCount = iCacheLines->Count();

	for (TInt index = 0; index< cacheLineCount; index++)
		{
		CCacheManager::TCacheLine& cacheLine = *(*iCacheLines)[index];
		iManager.DumpCacheLine(cacheLine);
		}
	}
#endif
