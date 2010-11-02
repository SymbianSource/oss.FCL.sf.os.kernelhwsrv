// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//
 
#include "sf_std.h"
#include "sf_file_cache.h"
#include "cl_std.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_fileTraces.h"
#endif
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

TInt OutputTraceInfo(CFsRequest* aRequest,TCorruptNameRec* aNameRec)
	{	
	RThread tT;
	RProcess tP;
	TBool nameUnknown=EFalse;
	TInt r=aRequest->Message().Client(tT,EOwnerThread);
	if(r!=KErrNone)
		{
		nameUnknown=ETrue;
		}
	else
		{
		r=tT.Process(tP);
		if(r!=KErrNone)	
			{
			tT.Close();	
			nameUnknown=ETrue;
			}
		}
	TName n;
	if(!nameUnknown)
		{
		n=tP.Name();
		TInt b=n.Locate('[');
		if (b>=0)
			n.SetLength(b);
		tP.Close();
		tT.Close();
		}
	else
		{
		n=_L("*Unknown*");
		}
	TPtrC t(aRequest->Src().FullName());
	// output a message via the debug port
	RDebug::Print(_L("@@@@ Corrupt file check %S tried to open %S"),&n,&t);
	// make a new log record & chain it in
	TCorruptLogRec* pLogRec= new TCorruptLogRec;
	if(pLogRec==NULL)
		return KErrNoMemory;
	TPtrC nPtr(n);
	if(pLogRec->Construct(aNameRec,&nPtr,gCorruptLogRecordList)!=KErrNone)
		{
		delete pLogRec;
		return KErrNoMemory;
		}
	else
		{
		gCorruptLogRecordList=pLogRec;
		// really a count of number of log records
		gNumberOfCorruptHits++;
		}
	return KErrNone;
	}

TCorruptLogRec::TCorruptLogRec()
	:iProcessName(NULL),iNameRec(NULL),iNext(NULL)
	{}

TCorruptLogRec::~TCorruptLogRec()
	{ // free off name memory
	delete iProcessName;
	}


void TCorruptLogRec::DestroyList()
	{
	TCorruptLogRec* pList=gCorruptLogRecordList;

	while(pList!=NULL)
		{
		TCorruptLogRec* pThis=pList;
		pList=pList->iNext;
		delete pThis;
		}
	gCorruptLogRecordList=NULL;
	gNumberOfCorruptHits=0;
	}

TInt TCorruptLogRec::Construct(TCorruptNameRec* aNameRec, TPtrC* aProcessName, TCorruptLogRec* aChain)
	{
	iProcessName=aProcessName->Alloc();
	if(iProcessName==NULL)
		return KErrNoMemory;

	iNameRec=aNameRec;
	iNext=aChain;
	return KErrNone;
	}

TInt TCorruptLogRec::GetLogRecord(TFsDebugCorruptLogRecordBuf& aLogRecord,TInt aLogRecNum)
	{
	if(aLogRecNum<=0)
		{
		return KErrArgument;
		}
	else if(aLogRecNum>gNumberOfCorruptHits)
		{
		return KErrNotFound;
		}

	TCorruptLogRec* pList=gCorruptLogRecordList;

	for(TInt i=1;i<aLogRecNum && pList!=NULL;i++)
		{	
		pList=pList->iNext;
		}

	TInt r=KErrNotFound;

	if(pList)
		{
		aLogRecord().iProcessName=pList->iProcessName->Des();
		aLogRecord().iFileName=pList->iNameRec->Name();
		aLogRecord().iError=pList->iNameRec->ReturnCode();
		r=KErrNone;
		}

	return r;
	}

TCorruptNameRec::TCorruptNameRec()
:iName(NULL),iNext(NULL){} 

TInt TCorruptNameRec::Construct(TPtr* aName,TInt aReturnCode, TBool aUseOnce, TCorruptNameRec* aChain)
	{
	iName=aName->Alloc();
	if(iName==NULL)
		return KErrNoMemory;
	iReturnCode=aReturnCode;
	iUseOnce=aUseOnce;
	iConsumed=EFalse;
	iNext=aChain;
	return KErrNone;
	}

void TCorruptNameRec::ResetListConsumed()
	{
	TCorruptNameRec* pList=gCorruptFileNameList;
	while(pList!=NULL)
		{
		pList->iConsumed=EFalse;
		pList=pList->Next();
		}
	}

LOCAL_C void checkCorruptNamesList(CFsRequest* aRequest, TInt &aError)
	{
	aError=KErrNone;
	TPtrC path(aRequest->Src().FullName());
	TCorruptNameRec* pList=gCorruptFileNameList;
	while(pList)
		{
		if(pList->Name().MatchF(path)==0)
			{
			if(!pList->Consumed())
				{
				aError=pList->ReturnCode();
				pList->SetConsumed();
				OutputTraceInfo(aRequest,pList);
				}
			break;
			}
		pList=pList->Next();
		}
	}
#endif


LOCAL_C TInt DoInitNoParse(CFsRequest* aRequest)
//
// Common init for read and write access to files
//
	{
	CFileShare* share = GetShareFromHandle(aRequest->Session(), aRequest->Message().Int3());
	if(!share)
		return(KErrBadHandle);
	aRequest->SetDrive(&share->File().Drive());
	aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );
	return KErrNone;
	}

_LIT(KDrivePath,"?:");
LOCAL_C TInt DoInitialise(CFsRequest* aRequest)
//
//	Common initialisation code use file share to determine asychronicity
//
	{
	CFileShare* share = GetShareFromHandle(aRequest->Session(), aRequest->Message().Int3());
	if(!share)
		return(KErrBadHandle);
	aRequest->SetDrive(&share->File().Drive());
	aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );
	TBuf<2> drive(KDrivePath);
	drive[0]=TText(aRequest->DriveNumber()+'A');
	aRequest->Src().Set(share->File().FileName(),NULL,&drive);
	return KErrNone;
	}

LOCAL_C TInt InitialiseScratchToShare(CFsRequest* aRequest)
//
// Common code used to initialise the scratch value to the CFileShare* from the request
//	
	{
	CFileShare* share=GetShareFromHandle(aRequest->Session(), aRequest->Message().Int3());
	if(!share)
		return(KErrBadHandle);
	aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );

	return(KErrNone);
	}

LOCAL_C TInt FsFileOpenL(CFsRequest* aRequest, TFileOpen anOpen)
//
// Open a file.
//
	{
	TInt r;
    
    TUint32 mode=aRequest->Message().Int1();
	if (anOpen==EFileCreate || anOpen==EFileReplace)
		{
		r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
		if(r != KErrNone)
            return r;
        
        mode|=EFileWrite;
		}

	TInt h;
    r=aRequest->Drive()->FileOpen(aRequest,h,aRequest->Src().FullName().Mid(2),mode,anOpen);
	if (r!=KErrNone)
		return(r);
	
    TPtrC8 pH((TUint8*)&h,sizeof(TInt));
	TRAP(r, aRequest->WriteL(KMsgPtr3,pH))
	CheckForLeaveAfterOpenL(r, aRequest, h);
	aRequest->Session()->IncResourceCount();
	
    return(KErrNone);
	}

TInt TFsFileOpen::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	__PRINT(_L("TFsFileOpen::DoRequestL(CFsRequest* aRequest)"));
	return FsFileOpenL(aRequest, EFileOpen);
	}

TInt TFsFileCreate::DoRequestL(CFsRequest* aRequest)
//
//
//
	{

	__PRINT(_L("TFsFileCreate::DoRequestL(CFsRequest* aRequest)"));
	return FsFileOpenL(aRequest, EFileCreate);
	}

TInt TFsFileCreate::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
 	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest->Message(),aRequest->Src().FullName().Mid(2),&KCapFsSysFileCreate,&KCapFsPriFileCreate,&KCapFsROFileCreate, __PLATSEC_DIAGNOSTIC_STRING("Create File")); 
	if (r!=KErrNone)
		return(r);
	if (OpenOnDriveZOnly)
		{
		aRequest->SetDrive(&TheDrives[EDriveZ]);
		aRequest->SetSubstedDrive(NULL);
		}
	return(r);
	}



TInt TFsFileReplace::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	__PRINT(_L("TFsFileReplace::DoRequestL(CFsRequest* aRequest)"));
	return FsFileOpenL(aRequest, EFileReplace);
	}

TInt TFsFileReplace::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
 	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest->Message(),aRequest->Src().FullName().Mid(2),&KCapFsSysFileReplace,&KCapFsPriFileReplace,&KCapFsROFileReplace, __PLATSEC_DIAGNOSTIC_STRING("Replace File")); 
	if (r!=KErrNone)
		return(r);

	if (OpenOnDriveZOnly)	// Yuck! yet another global
		{
		aRequest->SetDrive(&TheDrives[EDriveZ]);
		aRequest->SetSubstedDrive(NULL);
		}
	return(r);
	}


#ifdef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

#define __PLATSEC_DIAGNOSTIC_MESSAGE(s)	NULL
static const TInt KMsgBuffSize = KMaxPath;

#else

	#if defined(_UNICODE) && !defined(__KERNEL_MODE__)

static const TInt KCharMsgMaxLen = KMaxPath - 1;
static const TInt KMsgBuffSize = KMaxPath;

	#else

static const TInt KCharMsgMaxLen = 50;
static const TInt KMsgBuffSize = KMaxPath + KMsgMaxLen + 1;

	#endif	// #if defined(_UNICODE) && !defined(__KERNEL_MODE__)

// Local function to format a message
static
const char* FmtPlatSecMessage(TBufC<KMsgBuffSize>& buff, CFsRequest& req, const char* str)
    {
    char* p = (char*)buff.Ptr();
	const char* const base = p;
    // copy message string (if any)
    if(str)
        {
        while(*str && p < &base[KCharMsgMaxLen - 2]) // 2 for trailing ": "
            *p++ = *str++;
        *p++ = ':';
        *p++ = ' ';
        }
    // append filename
    const TDesC& fname = req.Src().FullName();
    const TInt end = Min(fname.Length(), 
                         KMsgBuffSize * sizeof(*buff.Ptr()) - (p - base) - 1);
    for(TInt i = 0; i < end; ++i)
        *p++ = (char)fname[i];
    *p = 0;
    return base;
    }
    
#define __PLATSEC_DIAGNOSTIC_MESSAGE(s) FmtPlatSecMessage(thisPath, *aRequest, s)

#endif	// #ifdef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__


TInt TFsFileOpen::Initialise(CFsRequest* aRequest)
//
// Parse and execute FileOpen service otherwise sets flag for  
// asynchronous service
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
 	if (r!=KErrNone)
		return(r);

	TBufC<KMsgBuffSize> thisPath(aRequest->Src().FullName().Mid(2));
	TUint32 mode = (aRequest->Message().Int1() & ~(EFileStreamText | EFileReadAsyncAll | EFileBigFile));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
// see if file is on our "Most Wanted" list
	TInt errorCode;
	checkCorruptNamesList(aRequest,errorCode);
	if(errorCode!=KErrNone)
		{
		return errorCode;
		}
#endif

	CFsMessageRequest* msgRequest = (CFsMessageRequest*)aRequest;
	if (OpenOnDriveZOnly)
		{
		aRequest->SetDrive(&TheDrives[EDriveZ]);
		aRequest->SetSubstedDrive(NULL);
		}

	if(msgRequest->IsPluginRequest())
		{
		// Always allow plugins to open files, regardless of the clients policy
		return KErrNone;
		}

	if(ComparePrivate(thisPath))
		{
		if(! SIDCheck(aRequest->Message(),thisPath))
			{
			if(!KCapFsPriFileOpen.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_MESSAGE("File Open in private path")))
				return KErrPermissionDenied;
			}
		}
	else if(CompareResource(thisPath))
		{
 		if(mode != EFileShareReadersOrWriters && mode != EFileShareReadersOnly && mode != EFileRead) 
 		// File opening mode EFileShareReadersOrWriters|EFileRead will fail the above test and not 
 		// be checked for policy, whereas file opening mode EFileShareReadersOrWriters|EFileWrite 
 		// will pass the test and will be checked for policy. 
 		// EFileRead is 0 whereas EFileWrite is non 0.
 			{
			if(!KCapFsROFileOpenWr.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_MESSAGE("File Open in resource path")))
				return KErrPermissionDenied;
			}
		}
	else if(CompareSystem(thisPath))
		{
		if(!(mode & EFileShareReadersOnly) && (mode & EFileWrite))
			{
			if(!KCapFsSysFileOpenWr.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_MESSAGE("File Open in system path")))
				return KErrPermissionDenied;
			}
		else
			{
			if(!KCapFsSysFileOpenRd.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_MESSAGE("File Open in system path")))
				return KErrPermissionDenied;
			}
		}

	return(r);
	}

#undef __PLATSEC_DIAGNOSTIC_MESSAGE	
	
TInt TFsIsFileOpen::DoRequestL(CFsRequest* aRequest)
//
// Return whether a file is open or not
//
	{

	__PRINT(_L("TFsIsFileOpen::DoRequestL(CFsRequest* aRequest)"));
	CFileCB* file;
	TInt r = aRequest->Drive()->IsFileOpen(aRequest->Src().FullName().Mid(2), file);
	if (r != KErrNone)
		return (r);
	TBool isOpen = file ? (TBool)ETrue : (TBool)EFalse;
	if (!isOpen)
		{
		// perform the existance check to retain compatibility with old-style clients
		TEntry e;
		r = aRequest->Drive()->Entry(aRequest->Src().FullName().Mid(2), e);
		if (r == KErrNone && e.IsDir())
			r = KErrArgument;
		}
	if (r != KErrNone)
		return (r);

	TPtrC8 pA((TUint8*)&isOpen,sizeof(TBool));
	aRequest->WriteL(KMsgPtr1,pA);
	return (KErrNone);
	}

TInt TFsIsFileOpen::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstCheckPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest->Message(),aRequest->Src().FullName().Mid(2),&KCapFsSysIsFileOpen,&KCapFsPriIsFileOpen, __PLATSEC_DIAGNOSTIC_STRING("Is File Open")); 
	return(r);
	}


TInt TFsListOpenFiles::DoRequestL(CFsRequest* aRequest)
//
// List open files
//
	{
	
	__PRINT(_L("TFsListOpenFiles::DoRequestL(CFsRequest* aRequest)"));

	TOpenFileListPos listPos;
	TPckg<TOpenFileListPos> listPkg(listPos);
	aRequest->ReadL(KMsgPtr0,listPkg);
	TBuf8<KEntryArraySize> entryArray(0);
	
	TThreadId idClient;
	TPckgC<TThreadId> id(idClient);	

	CSessionFs* session;
	TBool fileFound=(listPos.iEntryListPos) ? (TBool)ETrue : EFalse;
	TInt entryListPos;
	TInt count;
Start:
	FOREVER
		{
		session=(*TheFileServer)[listPos.iSession];	//this global may not be the best way AJ
		if (session==NULL)
			goto End;
		session->Handles().Lock();
		count=session->Handles().Count();
		if (count)
			break;
		session->Handles().Unlock();
		listPos.iSession++;
		}

	entryListPos=listPos.iEntryListPos;
	while (entryListPos<count)
		{
		CObjPromotion* obj=(CObjPromotion*)session->Handles()[entryListPos];
		if (obj==NULL || obj->UniqueID()!=FileShares->UniqueID())
			{
			entryListPos++;
			continue; // Is not a CFileShare
			}
		CFileCB& fileCb=((CFileShare*)obj)->File();

		TEntry fileEntry;
		// Set kEntryAttPacked to indicate it is in packed form
		fileEntry.iAtt=fileCb.Att() | KEntryAttPacked;
		TInt64 fileSize = fileCb.Size64();
		fileEntry.iSize = I64LOW(fileSize);
		fileEntry.iModified=fileCb.Modified();
		fileEntry.iName=fileCb.FileName();
		
		// Pack - Copy iSizeHigh and reset iReserved in packed form
		TUint32* pSizeHigh = PtrAdd((TUint32*)&fileEntry, EntrySize(fileEntry, EFalse));
		
		*pSizeHigh++ = I64HIGH(fileSize);	// Copy iSizeHigh
		*pSizeHigh 	 = 0;					// Reset iReserved
		
		TInt entrySize=EntrySize(fileEntry, ETrue);
		if (entryArray.Length()+entrySize>entryArray.MaxLength())
			break;
		TPtrC8 pfileEntry((TUint8*)&fileEntry,entrySize);
		entryArray.Append(pfileEntry);
		entryListPos++;
		}
	idClient = session->ThreadId();
	session->Handles().Unlock();

	if (entryArray.Length()==0)
		listPos.iSession++;
	if (fileFound==EFalse && entryArray.Length()==0)
		goto Start;
	listPos.iEntryListPos=entryListPos;

End:
	aRequest->WriteL(KMsgPtr1,id);
	aRequest->WriteL(KMsgPtr0,listPkg);
	aRequest->WriteL(KMsgPtr2,entryArray);
	return(KErrNone);
	}

TInt TFsListOpenFiles::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

LOCAL_C void FsFileTempFinishL(CFsRequest* aRequest,TFileName& aN,TInt aH)
	{

	aRequest->WriteL(KMsgPtr2,aRequest->Src().Drive());
	aRequest->WriteL(KMsgPtr2,aN,2);
	TPtrC8 pH((TUint8*)&aH,sizeof(TInt));
	aRequest->WriteL(KMsgPtr3,pH);
	}

TInt TFsFileTemp::DoRequestL(CFsRequest* aRequest)
//
// Create a temporary file.
//
	{
	__PRINT(_L("TFsFileTemp::DoRequestL(CFsRequest* aRequest)"));
    
    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;
	
    TFileName n;
	TInt h;
	r=aRequest->Drive()->FileTemp(aRequest,h,aRequest->Src().FullName().Mid(2),n,aRequest->Message().Int1());
	if (r!=KErrNone)
		return(r);
	
    TRAP(r, FsFileTempFinishL(aRequest,n,h))
	CheckForLeaveAfterOpenL(r,aRequest,h);
	aRequest->Session()->IncResourceCount();
	
    return(KErrNone);
	}

TInt TFsFileTemp::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ParseNoWildSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest->Message(),aRequest->Src().FullName().Mid(2),&KCapFsSysFileTemp,&KCapFsPriFileTemp,&KCapFsROFileTemp, __PLATSEC_DIAGNOSTIC_STRING("Temp File")); 
	if (r!=KErrNone)
		return(r);
	if (aRequest->Src().NameOrExtPresent())
		return(KErrBadName);
	return(r);
	}


TInt TFsFileRead::DoRequestL(CFsRequest* aRequest)
//
// Read from a file.
//
	{
	
	__PRINT(_L("TFsFileRead::DoRequestL(CFsRequest* aRequest)"));
	__PRINT1(_L("aRequest->Session() = 0x%x"),aRequest->Session());

	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;
	__ASSERT_DEBUG(msgRequest.CurrentOperationPtr() != NULL, Fault(EBadOperationIndex));
	TMsgOperation& currentOperation = msgRequest.CurrentOperation();

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	TInt r = file->CheckMount();
	__PRINT1(_L("share->CheckMount() returned = %d"),r);
	if (r!=KErrNone)
		return(r);

	TInt& len = currentOperation.iReadWriteArgs.iLength;
	TInt& totalLen = currentOperation.iReadWriteArgs.iTotalLength;
	TInt64 pos = currentOperation.iReadWriteArgs.iPos;
	TInt& offset = currentOperation.iReadWriteArgs.iOffset;

	// Fair scheduling - 
	// Needs extended file API to work (so that we can sepcify an offset)
	// Also needs a separate drive thread to prevent excessive stack usage
	len = Min(len, totalLen);
	if (file->ExtendedFileInterfaceSupported() && !FsThreadManager::IsDriveSync(aRequest->DriveNumber(), EFalse))
		{
		len = Min(len, file->FairSchedulingLen());
		}
	if (pos == KCurrentPosition64)
		pos = share->iPos;

	currentOperation.iReadWriteArgs.iPos = pos;

	__ASSERT_DEBUG(len > 0, Fault(EInvalidReadLength));
	__ASSERT_DEBUG(len <= totalLen, Fault(EInvalidReadLength));

	// The mount and any extensions must all support local buffers in order to support
	// internally generated requests (ie - requests originating from plugins)
	if ((aRequest->Message().Handle() == KLocalMessageHandle || !currentOperation.iClientRequest) && !file->LocalBufferSupport())
		{
		r = KErrNotSupported;
		}

	TInt reqLen = len;
	if(r == KErrNone)
		{
		if (currentOperation.iClientRequest)	
			{
			// Current operation points to a descriptor
			// The request originated from a client (with a normal message handle) or a plugin (KLocalMessageHandle)
			TRAP(r,file->ReadL(pos, len, (TPtr8*) aRequest->Message().Ptr0(), aRequest->Message(), offset))
			}
		else
			{
			// Current operation points to a local buffer
			// The request originated from the file server (e.g. file cache) with a local message handle (KLocalMessageHandle)
			TPtr8 dataDesc((TUint8*) currentOperation.iReadWriteArgs.iData + currentOperation.iReadWriteArgs.iOffset, len, len);

			// save the client's RMessage2
			const RMessage2 msgClient = aRequest->Message();
			
			// overwrite RMessage2 in CFsMessageRequest with RLocalMessage 
			const RLocalMessage msgLocal;					
			const_cast<RMessage2&> (aRequest->Message()) = msgLocal;

			TRAP(r,file->ReadL(pos, len, &dataDesc, aRequest->Message(), 0));
							
			// restore the client's RMessage2
			const_cast<RMessage2&> (aRequest->Message()) = msgClient;
			}
		}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	CCacheManager* manager = CCacheManagerFactory::CacheManager();
	if (manager)
		{
		manager->Stats().iUncachedPacketsRead++;
		manager->Stats().iUncachedBytesRead+= len;
		}
#endif

//RDebug::Print(_L("ReadR: req %08X pos %ld\t len %d file %08X\n"), aRequest, pos, len, file);


#if defined (_DEBUG_READ_AHEAD)
	RDebug::Print(_L("ReadR: req %08X pos %ld\t len %d nextPos %ld file %08X\n"), aRequest, pos, len, pos + len, file);
#endif

	offset+= len;
	currentOperation.iReadWriteArgs.iPos+= len;
	totalLen-= reqLen;


	// update the file share's position IF this request came from the client
	if (share && r==KErrNone && currentOperation.iClientRequest)
		{
		__e32_atomic_store_ord64(&share->iPos, pos + len);
		}

	// re-issue request if not complete (to support fair scheduling)
	if (r == KErrNone && totalLen > 0)
		return CFsRequest::EReqActionBusy;	// dispatch request again to back of queue

	return(r);
	}


TInt TFsFileRead::Initialise(CFsRequest* aRequest)
//
//
//
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	TInt r = DoInitNoParse(aRequest);
	if (r != KErrNone)
		return r;

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	TMsgOperation* msgOp = msgRequest.CurrentOperationPtr();
	if (!msgOp)	// initialised already ?
		{
		r = msgRequest.PushOperation(TFsFileRead::Complete);
		if (r != KErrNone)
			return r;
		msgOp = msgRequest.CurrentOperationPtr();
		}
	// try to serialize requests to prevent asynchronous requests being processed out of sequence -
	// this isn't possible if a plugin is loaded as this may issue it's own requests 
	if (!share->RequestStart(&msgRequest))
		return CFsRequest::EReqActionPending;

	TDrive& drive = share->File().Drive();

	TInt64 pos, reqPos;
	TInt len, reqLen;

	reqLen = len = aRequest->Message().Int1();

	if(aRequest->IsDescData(KMsgPtr2))
		{//-- 64-bit file addressing, absolute read position is TInt64
			TPckg<TInt64> pkPos(reqPos);
			aRequest->ReadL(KMsgPtr2, pkPos);
		}
	else
		{
		if(aRequest->Message().Int2() == (TInt)I64LOW(KCurrentPosition64))
			reqPos = KCurrentPosition64; // Position is KCurrentPosition64 (-1)
		else //-- legacy, RFile addressing
			reqPos = MAKE_TINT64(0,aRequest->Message().Int2());	// Position is absolute value < 4GB, it's TUint
		}
	
    msgOp->iClientPosition = pos = reqPos;
	
	if (len < 0)
		return KErrArgument;

	if (len == 0)
		return CFsRequest::EReqActionComplete;
	
	if (pos == KCurrentPosition64)
		pos = share->iPos;

	const TInt64 fileSize = file->CachedSize64();
	if (pos > fileSize)
		pos = fileSize;

	if ((r = file->CheckLock64(share,pos,len)) != KErrNone)
		return r;

	
	TDes8* pD = (TDes8*) aRequest->Message().Ptr0();

	if((share->iMode & EFileReadAsyncAll) && (aRequest->Message().ClientStatus() != NULL))
		{
		drive.Lock();
		if (pos + len > fileSize)
			{
			r = share->File().AddAsyncReadRequest(share, reqPos, reqLen, aRequest);
			drive.UnLock();
			return (r == KErrNone)?CFsRequest::EReqActionComplete:r;
			}
		drive.UnLock();
		}
	
    if (pos == fileSize)
		{
		__e32_atomic_store_ord64(&share->iPos, pos);
		r = aRequest->Write(KMsgPtr0, KNullDesC8);
		return(r == KErrNone?CFsRequest::EReqActionComplete:r);
		}
	
    if (pos + len > fileSize)
		{
		// filesize - pos shall of TInt size
		// Hence to suppress warning
		len = (TInt)(fileSize - pos);
		}

	msgOp->Set(pos, len, (TDesC8*) pD);

//RDebug::Print(_L("ReadI: req %08X pos %ld\t len %d file %08X\n"), aRequest, pos, len, file);

#if defined (_DEBUG_READ_AHEAD)
	RDebug::Print(_L("ReadI: req %08X pos %ld\t len %d file %08X\n"), aRequest, pos, len, file);
#endif

	return KErrNone;
	}

TInt TFsFileRead::PostInitialise(CFsRequest* aRequest)
	{
	CFileShare* share = (CFileShare*) aRequest->ScratchValue();
	CFileCB* file = &share->File();
	TInt r = KErrNone;

	CFileCache* fileCache = file->FileCache();
	if (fileCache)
		{
		r = fileCache->ReadBuffered(*(CFsMessageRequest*)aRequest, share->iMode);

		// if we're not reading from cache, force read ahead position & length to be recalculated
		if (r == KErrNone)
			fileCache->ResetReadAhead();
		}
	
	return r;
	}

TInt TFsFileRead::Complete(CFsRequest* aRequest)
	{
//		RDebug::Print(_L("TFsFileRead::Complete() aRequest %08X"), aRequest); 

	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;
	
	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	// Flag the request as having ended to allow another async read to occur
	share->RequestEnd(&msgRequest);


	// issue read-ahead
	CFileCache* fileCache = file->FileCache();
	if (fileCache && msgRequest.LastError() == KErrNone)
		fileCache->ReadAhead(msgRequest, share->iMode);

	return CFsRequest::EReqActionComplete;
	}


void GetFileFromScratch(CFsRequest* aRequest, CFileShare*& aShare, CFileCB*& aFile)
	{

	TInt64 scratchValue = aRequest->ScratchValue64();
	TBool scratchValueIsShare  I64HIGH(scratchValue);
	TUint32 scratchValueLow = I64LOW(scratchValue);
	
	aShare = NULL;
	aFile = NULL;

	if (scratchValueIsShare)
		{
		aShare = (CFileShare*) scratchValueLow;
		if (aShare)
			aFile = &aShare->File();
		}
	else
		{
		aFile = (CFileCB*) scratchValueLow;
		}
	}

/**
    Common init preamble for TFsFileWrite::Initialise() and TFsFileWrite::DoRequestL()
    
    @param   aShare     pointer to the file share
    @param   aFile      pointer to the file object this function is called for
    @param   aPos       file position to write data. Note that it can be KCurrentPosition64 i.e. KMaxTUint64
    @param   aLen       length of the data to write
    @param   aFileSize  current file size
    @param   aFsOp      File Server message code. See TFsMessage. It must be ether EFsFileWrite for normal write, or EFsFileWriteDirty when file cache flushes dirty data
*/

TInt TFsFileWrite::CommonInit(CFileShare* aShare, CFileCB* aFile, TInt64& aPos, TInt& aLen, TInt64 aFileSize, TFsMessage aFsOp)
	{
	
    if (aShare && aPos==KCurrentPosition64)
		{//-- write to the current position in the file
		aPos = aShare->iPos;
		}

	if(aPos > aFileSize)
		aPos = aFileSize;

        //-- check that the new position won't exceed maximum file size
        {
        const TUint64 endPos = aPos+aLen;

	    //-- Large file mode check. Legacy RFile size can't exceed 2G-1
        if(aShare && !(aShare->IsFileModeBig()) && (endPos > KMaxLegacyFileSize))
		    return KErrTooBig;

        //-- check CMountCB limitation on maximum file size
        if(endPos > aFile->MaxSupportedSize())
            return KErrNotSupported; //-- this is for the sake of error codes consistency; current FSYs return 
                                     //-- this code in the case of accessing a file beyond its limit
       }

	if (aShare)
		{
		TInt r;
		if ((r=aFile->CheckLock64(aShare,aPos,aLen))!=KErrNone)
			return(r);
		}

    ASSERT(aFsOp == EFsFileWrite || aFsOp == EFsFileWriteDirty);
    if(aFsOp == EFsFileWrite)
        {//-- this call is originated from explicit file write operation. Set 'Archive' attribute and new file time.
        aFile->SetArchiveAttribute(); //-- it will also set KEntryAttModified
        }


	return KErrNone;
	}

void TFsFileWrite::CommonEnd(CFsMessageRequest* aMsgRequest, TInt aRetVal, TUint64 aInitSize, TUint64 aNewSize, TInt64 aNewPos, TBool aFileWrite)
//
// Common end for TFsFileWrite::DoRequestL() and CFileCache::WriteBuffered()
//
	{

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aMsgRequest, share, file);
	CFileCache* fileCache = file->FileCache();
	ASSERT(aFileWrite || fileCache);

	TMsgOperation& currentOperation = aMsgRequest->CurrentOperation();

	if (aRetVal == KErrNone || aRetVal == CFsRequest::EReqActionComplete)
		{
		if (share)
			{
			__e32_atomic_store_ord64(&share->iPos, aNewPos);
			}
		
		if ((TUint64)aNewPos > aNewSize)
			{
			if(aFileWrite)
				file->SetSize64(aNewPos, EFalse);
			else
				fileCache->SetSize64(aNewPos);
			aNewSize = aNewPos;
			}
		
		// ensure cached file is at least as big as uncached file
		if (fileCache && fileCache->Size64() < aNewPos)
			{
			file->SetCachedSize64(aNewPos);
			}
		
		// Service async reads if the file has grown & this is the last fair-scheduled request
		
		// If the file has grown, flag this and call CFileCB::NotifyAsyncReaders()
		// later in TFsFileWrite::Complete() - we need to delay the call because
		// CFileCB::NotifyAsyncReaders() may requeue a request which will cause 
		// the drive thread to spin because this file share is still marked as in use
		// (by CFileShare::RequestStart())
		if((aNewSize > aInitSize) && (currentOperation.iReadWriteArgs.iTotalLength == 0))
			{
			file->SetNotifyAsyncReadersPending(ETrue);
			}
		}
	else if (aRetVal == KErrCorrupt)
		file->SetFileCorrupt(ETrue);
	else if (aRetVal == KErrBadPower || (aRetVal == KErrAbort && !PowerOk()))
		file->SetBadPower(ETrue);

	file->ResetReadAhead();
	aMsgRequest->SetFreeChanged(aNewSize != aInitSize);
	}

TInt TFsFileWrite::DoRequestL(CFsRequest* aRequest)
//
// Write to a file.
//
	{
	__PRINT(_L("TFsFileWrite::DoRequestL(CFsRequest* aRequest)"));

	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;
	__ASSERT_DEBUG(msgRequest.CurrentOperationPtr() != NULL, Fault(EBadOperationIndex));
	TMsgOperation& currentOperation = msgRequest.CurrentOperation();

	TInt r;

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	TInt64 initSize = file->Size64();

	r = file->CheckMount();
	if (r!=KErrNone)
		return(r);


	TInt& len = currentOperation.iReadWriteArgs.iLength;

	// Fair scheduling - 
	// Needs extended file API to work (so that we can specify an offset)
	// Also needs a separate drive thread to prevent excessive stack usage
	len = Min(len, currentOperation.iReadWriteArgs.iTotalLength);
	if (file->ExtendedFileInterfaceSupported() && !FsThreadManager::IsDriveSync(aRequest->DriveNumber(), EFalse))
		{
		len = Min(len, file->FairSchedulingLen());
		}

	__ASSERT_DEBUG(len <= currentOperation.iReadWriteArgs.iTotalLength, Fault(EInvalidWriteLength));
	
	
    const TFsMessage fsOp = (TFsMessage)aRequest->Operation()->Function(); 
    r = CommonInit(share, file, currentOperation.iReadWriteArgs.iPos, len, initSize, fsOp);

	if (r != KErrNone)
		return r;
	
    TInt64 pos = currentOperation.iReadWriteArgs.iPos;

	TInt64 upos = pos+len;
	if (upos > initSize)
		{
		r = CheckDiskSpace(upos - initSize, aRequest);
        if(r != KErrNone)
            return r;
		}

	// The mount and any extensions must all support local buffers in order to support
	// internally generated requests (ie - requests originating from plugins)
	if ((aRequest->Message().Handle() == KLocalMessageHandle || !currentOperation.iClientRequest) && !file->LocalBufferSupport())
		{
		r = KErrNotSupported;
		}

	if(r == KErrNone)
		{
		if (currentOperation.iClientRequest)
			{
			TRAP(r,file->WriteL(pos, len, (const TPtrC8*) aRequest->Message().Ptr0(), aRequest->Message(), currentOperation.iReadWriteArgs.iOffset))
			}
		else
			{
			TPtr8 dataDesc((TUint8*) currentOperation.iReadWriteArgs.iData + currentOperation.iReadWriteArgs.iOffset, len, len);

			// save the client's RMessage2
			const RMessage2 msgClient = aRequest->Message();
			
			// overwrite RMessage2 in CFsMessageRequest with RLocalMessage 
			const RLocalMessage msgLocal;					
			const_cast<RMessage2&> (aRequest->Message()) = msgLocal;

			TRAP(r,file->WriteL(pos, len, &dataDesc, aRequest->Message(), 0));
							
			// restore the client's RMessage2
			const_cast<RMessage2&> (aRequest->Message()) = msgClient;
			}
		}

//RDebug::Print(_L("WriteR: req %08X pos %ld\t len %d file %08X\n"), aRequest, pos, len, file);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	CCacheManager* manager = CCacheManagerFactory::CacheManager();
	if (manager)
		{
		manager->Stats().iUncachedPacketsWritten++;
		manager->Stats().iUncachedBytesWritten+= len;
		}
#endif

	if (r == KErrNone)
		{
		// update position, offset  & length remaining
		currentOperation.iReadWriteArgs.iOffset+= len;
		currentOperation.iReadWriteArgs.iPos+= len;
		currentOperation.iReadWriteArgs.iTotalLength-= len;
		}
	TUint64 currentSize = MAKE_TUINT64(file->iBody->iSizeHigh,file->iSize);
	CommonEnd(&msgRequest, r, initSize, currentSize, pos+len, ETrue);
	
	// re-issue request if not complete (to support fair scheduling)
	if (r == KErrNone && currentOperation.iReadWriteArgs.iTotalLength > 0)
		return CFsRequest::EReqActionBusy;	// dispatch request again to back of queue

	return(r);
	}

TInt TFsFileWrite::Initialise(CFsRequest* aRequest)
//
//
//
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	TInt r = DoInitNoParse(aRequest);
	if (r != KErrNone)
		return r;

	// If the drive has been dismounted, don't even attempt to write to the file as that
	// will create dirty data which can then not be flushed without hitting ASSERTs etc.
	if (!FsThreadManager::IsDriveAvailable(aRequest->DriveNumber(), EFalse))
		return KErrNotReady;

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	// Bail out if there's a new mount which isn't ours - this would fail
	// later on anyway when TFsFileWrite::DoRequestL() called CFileCB::CheckMount()
	if ( !file->Drive().IsCurrentMount(file->Mount())  )
		return KErrDisMounted;


	TMsgOperation* msgOp = msgRequest.CurrentOperationPtr();
	if (!msgOp)	// initialised already ?
		{
		r = msgRequest.PushOperation(TFsFileWrite::Complete);
		if (r != KErrNone)
			return r;
		msgOp = msgRequest.CurrentOperationPtr();
		}
	// try to serialize requests to prevent asynchronous requests being processed out of sequence -
	// this isn't possible if a plugin is loaded as this may issue it's own requests 
	if (share && !share->RequestStart(&msgRequest))
		return CFsRequest::EReqActionPending;
	
	TInt64 pos;
	
	if(aRequest->IsDescData(KMsgPtr2))
		{//-- 64-bit file addressing, absolute read position is TInt64
			TPckg<TInt64> pkPos(pos);
			aRequest->ReadL(KMsgPtr2, pkPos);
		}
	else
		{
		if(aRequest->Message().Int2() == (TInt)I64LOW(KCurrentPosition64))
			pos = KCurrentPosition64;// Position is KCurrentPosition64 (-1)
		else
			pos = MAKE_TINT64(0,aRequest->Message().Int2());// Position is absolute value < 4GB, it's a TUint type
		}

	msgOp->iClientPosition = pos;
	TInt len = aRequest->Message().Int1();

	TDesC8* pD = (TDes8*) aRequest->Message().Ptr0();
	
	if (len == 0)
		return CFsRequest::EReqActionComplete;
	
	if (len < 0)
		return KErrArgument;
	
	//if this request was sent down from a plugin, then we want to
	//ignore the mode that the files was opened in.
	if (share && !msgRequest.IsPluginRequest())
		{
		if ((share->iMode & EFileWrite)==0 || 
			(share->iMode & KFileShareMask) == EFileShareReadersOnly)
			{
			return(KErrAccessDenied);
			}
		}
	
	
    const TFsMessage fsOp = (TFsMessage)aRequest->Operation()->Function(); 
    r = CommonInit(share, file, pos, len, file->CachedSize64(), fsOp);
	if (r != KErrNone)
		return r;

	msgOp->Set(pos, len, pD);

	return KErrNone;
	}


TInt TFsFileWrite::PostInitialise(CFsRequest* aRequest)
	{
	CFileShare* share = (CFileShare*) aRequest->ScratchValue();
	CFileCB* file = &share->File();
	TInt r = KErrNone;

	CFileCache* fileCache = file->FileCache();
	if (fileCache)
		{
		// If there's no data left proceed straight to completion stage
		// This can happen when re-posting a completed write request to 
		// start the dirty data timer
		if (((CFsMessageRequest*) aRequest)->CurrentOperation().iReadWriteArgs.iTotalLength == 0)
			return CFsRequest::EReqActionComplete;

		r = fileCache->WriteBuffered(*(CFsMessageRequest*)aRequest, share->iMode);
		}
	
	return r;
	}

TInt TFsFileWrite::Complete(CFsRequest* aRequest)
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;
	

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(aRequest, share, file);

	if (share)
		share->RequestEnd(&msgRequest);

	if (file->NotifyAsyncReadersPending())
		file->NotifyAsyncReaders();
	
	return CFsRequest::EReqActionComplete;
	}

TInt TFsFileLock::DoRequestL(CFsRequest* aRequest)
//
// Lock a region of the file.
//
	{

	__PRINT(_L("TFsFileLock::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();

	// We must wait for ALL shares to have no active requests (see RequestStart())
	// and post this request to the back of the queue if there are any. This is to 
	// avoid a fair-scheduled write from writing to a locked region of a file
	CSessionFs* session = aRequest->Session();
	if (session)
		{
		TBool fileInUse = EFalse;
		session->Handles().Lock();
		TInt count = session->Handles().Count();
		CFileCB* file = &share->File();

		for (TInt n=0; n<count; n++)
			{
			CObjPromotion* obj = (CObjPromotion*)session->Handles()[n];
			if (obj != NULL && 
				obj->UniqueID() == FileShares->UniqueID() &&
				(file == &((CFileShare*) obj)->File()) &&
				((CFileShare*) obj)->RequestInProgress())
				{
				CFsMessageRequest& msgRequest = *(CFsMessageRequest*)aRequest;
				if(msgRequest.IsPluginRequest())
					break;

				fileInUse = ETrue;
				break;
				}
			}
		session->Handles().Unlock();
		if (fileInUse)
			return CFsRequest::EReqActionBusy;
		}
	
	TInt64 pos, length;
	if(aRequest->IsDescData(KMsgPtr0))
		{
		TPckg<TInt64> pkPos(pos);
		aRequest->ReadL(KMsgPtr0, pkPos);
		}
	else
		{
		pos = MAKE_TINT64(0, aRequest->Message().Int0());
		}
	
    if(aRequest->IsDescData(KMsgPtr1))
		{
		TPckg<TInt64> pkLength(length);
		aRequest->ReadL(KMsgPtr1, pkLength);
		if(length <= 0)
			User::Leave(ELockLengthZero);
		}
	else
		{
		length = aRequest->Message().Int1();
		if(length <= 0) 
			User::Leave(ELockLengthZero);
		}
	return(share->File().AddLock64(share, pos, length));
	}

TInt TFsFileLock::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileUnlock::DoRequestL(CFsRequest* aRequest)
//
// Unlock a region of the file.
//
	{
	__PRINT(_L("TFsFileUnlock::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	
	TInt64 pos, length;
	
    if(aRequest->IsDescData(KMsgPtr0))
		{
		TPckg<TInt64> pkPos(pos);
		aRequest->ReadL(KMsgPtr0, pkPos);
		}
	else
		{
		pos = MAKE_TINT64(0, aRequest->Message().Int0());
		}
	
    if(aRequest->IsDescData(KMsgPtr1))
		{
		TPckg<TInt64> pkLength(length);
		aRequest->ReadL(KMsgPtr1, pkLength);
		if(length <= 0)
			User::Leave(EUnlockLengthZero);
		}
	else
		{
		length = aRequest->Message().Int1();
		if(length <= 0) 
			User::Leave(EUnlockLengthZero);
		}
	return(share->File().RemoveLock64(share, pos, length));
	}

TInt TFsFileUnlock::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileSeek::DoRequestL(CFsRequest* aRequest)
//
// Set the file position.
//
	{

	__PRINT(_L("TFsFileSeek::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TInt64 size = share->File().CachedSize64();
	TInt64 pos;
	if(aRequest->IsDescData(KMsgPtr0))
		{
		TPckg<TInt64> pkPos(pos);
		aRequest->ReadL(KMsgPtr0, pkPos);
		}
	else
		{
		pos = aRequest->Message().Int0();
		}
	
	TInt r,t;
	
	if (share->iPos != pos)
		share->File().ResetReadAhead();
	
	switch (aRequest->Message().Int1())
		{
	case ESeekCurrent:
		pos+=share->iPos;
		if (pos<0)
			pos=0;
		
        if (pos>size)
			pos=size;

		// Large file mode check
		if((!(share->IsFileModeBig())) && ((TUint64)pos > KMaxLegacyFileSize))
			return (KErrTooBig);

		break;
	case ESeekEnd:
		pos+=size;
		if (pos<0)
			pos=0;
		if (pos>size)
			pos=size;
		// Large file mode check
		if((!(share->IsFileModeBig())) && ((TUint64)pos > KMaxLegacyFileSize))
			return (KErrTooBig);
		
        break;
	case ESeekAddress:
		t = (TUint)pos;
		r = share->File().Address(t);
		pos = (TUint)t;
		if(KErrNone != r)
			return(r);
		goto writeBackPos;
	case ESeekStart:
		if (pos>=0)
            {
            share->iPos = pos;
            return KErrNone;
            }
		return(KErrArgument);
	default:
		return(KErrArgument);
		}
	__e32_atomic_store_ord64(&share->iPos, pos);
writeBackPos:
	TPckgC<TInt64> pkNewPos(pos);
	aRequest->WriteL(KMsgPtr2, pkNewPos);
	return(KErrNone);
	}

TInt TFsFileSeek::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileFlush::DoRequestL(CFsRequest* aRequest)
//
// Commit any data in memory to the media.
//
	{

	__PRINT(_L("TFsFileFlush::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();

	// if any write requests are being fair scheduled, wait for them to complete
	if (share->RequestInProgress())
		return CFsRequest::EReqActionBusy;

	// flush the write cache
	TInt r;
	CFileCache* fileCache = share->File().FileCache();
	if (fileCache && (r = fileCache->FlushDirty(aRequest)) != CFsRequest::EReqActionComplete)
	    {
		return r;
	    }

	if ((share->File().Att()&KEntryAttModified)==0)
		return(KErrNone);
	if ((share->iMode&EFileWrite)==0)
		return(KErrAccessDenied);
	r=share->CheckMount();
	if (r!=KErrNone)
		return(r);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBFLUSHDATAL1, "this %x", &share->File());
	TRAP(r,share->File().FlushDataL());
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBFLUSHDATAL1RET, "r %d", r);
	return(r);
	}

TInt TFsFileFlush::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileSize::DoRequestL(CFsRequest* aRequest)
//
// Get the file size.
//
	{

	__PRINT(_L("TFsFileSize::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TInt64 size = share->File().CachedSize64();
	// Large file mode check and error handling is done at client library side.
	// Return file size even if it is greater than 2GB - 1.
	TPckgC<TInt64> pkSize(size);
	aRequest->WriteL(KMsgPtr0, pkSize);
	return(KErrNone);
	}

TInt TFsFileSize::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileSetSize::DoRequestL(CFsRequest* aRequest)
//
// Set the file size.
//
	{
	__PRINT(_L("TFsFileSetSize::DoRequestL(CFsRequest* aRequest)"));

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	if ((share->iMode&EFileWrite)==0)
		return(KErrAccessDenied);
	TInt r=share->CheckMount();
	if (r!=KErrNone)
		return(r);

	TInt64 size;
	if(aRequest->IsDescData(KMsgPtr0))
		{
		TPckg<TInt64> pkSize(size);
		aRequest->ReadL(KMsgPtr0, pkSize);
		}
	else
		{
		size = aRequest->Message().Int0();
		}

	if(size < 0)
		User::Leave(ESizeNegative);
	
	// Large file mode check
	if((!(share->IsFileModeBig())) && ((TUint64)size > KMaxLegacyFileSize))
		return (KErrTooBig);
	
	CFileCB& file=share->File();

	if (size==file.Size64())
		{
		file.SetCachedSize64(size);	// Ensure the cache size doesn't exceeed the physical size
		return(KErrNone);
		}
	
	TBool fileHasGrown = size > file.Size64();
	if (fileHasGrown)
		{
		r = CheckDiskSpace(size - file.Size64(), aRequest);
        if(r != KErrNone)
            return r;

		r=file.CheckLock64(share,file.Size64(),size-file.Size64());
		}	
	else
		r=file.CheckLock64(share,size,file.Size64()-size);
	if (r!=KErrNone)
		return(r);
	__PRINT1(_L("Owner mount size before SetSize() = %d"),I64LOW(file.Mount().Size()));
	TRAP(r,file.SetSizeL(size))
	if (r!=KErrNone)
		{
		//	Set size failed
		__PRINT1(_L("SetSize() failed = %d"),r);
		__PRINT1(_L("Owner mount size now = %d"),I64LOW(file.Mount().Size()));
		return(r);
		}
	file.SetArchiveAttribute();
	file.SetSize64(size, EFalse);
	file.SetCachedSize64(size);

	if(fileHasGrown)
		file.NotifyAsyncReaders();	// Service outstanding async reads if the file has grown

	return(r);
	}

TInt TFsFileSetSize::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileAtt::DoRequestL(CFsRequest* aRequest)
//
// Get the file attributes.
//
	{

	__PRINT(_L("TFsFileAtt::DoRequestL(CFsRequest* aRequest)"));

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
//	TInt att=(TInt)aRequest->FileShare()->File().Att()&KEntryAttMaskSupported;
	TInt att=(TInt)share->File().Att();	// DRM: let ROM XIP attribute through
	att&= ~KEntryAttModified;	// this is an internal attribute and should not be returned to the client
	TPtrC8 pA((TUint8*)&att,sizeof(TInt));
	aRequest->WriteL(KMsgPtr0,pA);
	
    return(KErrNone);
	}

TInt TFsFileAtt::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileSetAtt::DoRequestL(CFsRequest* aRequest)
//
// Set the file attributes.
//
	{
	__PRINT(_L("TFsFileSetAtt::DoRequestL(CSessionFs* aSession)"));
    
    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	r=share->CheckMount();
	if (r!=KErrNone)
		return(r);
	
    if ((share->iMode&EFileWrite)==EFalse)
		return(KErrAccessDenied);
	
    TUint setAttMask=(TUint)(aRequest->Message().Int0());
	TUint clearAttMask=(TUint)aRequest->Message().Int1();
	ValidateAtts(setAttMask,clearAttMask);
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL1, "this %x aSetAttMask %x aClearAttMask %x", (TUint) &share->File(), (TUint) setAttMask, (TUint) clearAttMask);
	TRAP(r,share->File().SetEntryL(share->File().Modified(),setAttMask,clearAttMask))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL1RET, "r %d", r);
	return(r);
	}


TInt TFsFileSetAtt::Initialise(CFsRequest* aRequest)
//
//	Call GetShareFromHandle to determine asynchronicity ***
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileModified::DoRequestL(CFsRequest* aRequest)
//
// Get the modified date and time.
//
	{
	__PRINT(_L("TFsFileModified::DoRequestL(CFsRequest* aRequest)"));
        	
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TTime mod=share->File().Modified();
	TPtrC8 pM((TUint8*)&mod,sizeof(TTime));
	aRequest->WriteL(KMsgPtr0,pM);
	
    return(KErrNone);
	}


TInt TFsFileModified::Initialise(CFsRequest* aRequest)
//
//	Call GetShareFromHandle to determine asynchronicity ***
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileSetModified::DoRequestL(CFsRequest* aRequest)
//
// Set the modified date and time.
//
	{
	__PRINT(_L("TFsFileSetModified::DoRequestL(CFsRequest* aRequest)"));
    
    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;


	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	r=share->CheckMount();
	if (r!=KErrNone)
		return(r);

	if ((share->iMode&EFileWrite)==EFalse)
		return(KErrAccessDenied);
	
	TTime time;
	TPtr8 t((TUint8*)&time,sizeof(TTime));
	aRequest->ReadL(KMsgPtr0,t);
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL2, "this %x aSetAttMask %x aClearAttMask %x", (TUint) &share->File(), (TUint) KEntryAttModified, (TUint) 0);
	TRAP(r,share->File().SetEntryL(time,KEntryAttModified,0))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL2RET, "r %d", r);
	return(r);
	}


TInt TFsFileSetModified::Initialise(CFsRequest* aRequest)
//
//	Call GetShareFromHandle to determine asynchronicity ***
//
	{
	return(DoInitNoParse(aRequest));
	}

TInt TFsFileSet::DoRequestL(CFsRequest* aRequest)
//
// Set the attributes and the modified date and time.
//
	{
	__PRINT(_L("TFsFileSet::DoRequestL(CFsRequest* aRequest)"));

    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	r=share->CheckMount();
	if (r!=KErrNone)
		return(r);
	
    if (share->File().Att()&KEntryAttReadOnly)
		return(KErrAccessDenied);
	
    if ((share->iMode&EFileWrite)==0)
		{
		if(share->File().Drive().IsWriteProtected())
			return(KErrAccessDenied);
		}
	
    TTime time;
	TPtr8 t((TUint8*)&time,sizeof(TTime));
	aRequest->ReadL(KMsgPtr0,t);
	TUint setAttMask=(TUint)(aRequest->Message().Int1());
	TUint clearAttMask=(TUint)aRequest->Message().Int2();
	ValidateAtts(setAttMask,clearAttMask);//	Validate attributes

	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL3, "this %x aSetAttMask %x aClearAttMask %x", (TUint) &share->File(), (TUint) setAttMask, (TUint) clearAttMask);
	TRAP(r,share->File().SetEntryL(time,setAttMask|KEntryAttModified,clearAttMask))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBSETENTRYL3RET, "r %d", r);
	return(r);
	}

TInt TFsFileSet::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileChangeMode::DoRequestL(CFsRequest* aRequest)
//
// The access to a file may be changed from share exclusive to share readers only or vice versa.
// KErrAccessDenied is returned if the file has multiple readers.
//
	{
	__PRINT(_L("TFsFileChangeMode::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TInt r=share->CheckMount();
	if (r!=KErrNone)
		return(r);
	
	const TFileMode currentMode = (TFileMode)share->iMode;
	const TFileMode newMode = (TFileMode)aRequest->Message().Int0();
	
		// check argument 
		// (only EFileShareExclusive and EFileShareReadersOnly are allowed)
	if((newMode & ~EFileShareReadersOnly) != 0)
		return KErrArgument;
	
		// check if the file is in EFileShareAny mode
	if( (currentMode & EFileShareAny) ||
	    // or the file has been opened for writing in EFileShareExclusive mode, 
		// and an attempt is made to change the access mode to EFileShareReadersOnly
		((currentMode & EFileWrite) && 
		 (currentMode & KFileShareMask) == EFileShareExclusive && 
		 newMode == EFileShareReadersOnly) )
		return KErrAccessDenied;

		// check if an attempt is made to change the share mode to EFileShareExclusive
		// while the file has multiple readers
	if (newMode == EFileShareExclusive && (currentMode & KFileShareMask) != EFileShareExclusive)
		{
		// Check that this is the file's only fileshare/client
		TDblQue<CFileShare>& aShareList = (&share->File())->FileShareList();
		if (!(aShareList.IsFirst(share) && aShareList.IsLast(share)))
			return KErrAccessDenied;
		}
	
	share->iMode&=~KFileShareMask;
	share->iMode|=newMode;
	share->File().SetShare(newMode);
	return(KErrNone);
	}


TInt TFsFileChangeMode::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r = DoInitNoParse(aRequest);
	if(r != KErrNone)
		return r;
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	if(CompareResource(share->File().FileName().Mid(2)) )
		{
		if(!KCapFsFileChangeMode.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("File Change Mode")))
			return KErrPermissionDenied;
		}
	return KErrNone;
	}


TInt TFsFileRename::DoRequestL(CFsRequest* aRequest)
//
// Rename the file if it was openned EFileShareExclusive
//
	{
	__PRINT(_L("TFsFileRename::DoRequestL(CFsRequest* aRequest)"));

    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	r=share->CheckMount();
	if (r!=KErrNone)
		return(r);

	TInt currentMode=(share->iMode&KFileShareMask);
	if ((currentMode&EFileShareAny) || (currentMode&EFileShareReadersOnly))
		return(KErrAccessDenied); // File must be EFileShareExclusive

	if ((share->iMode&EFileWrite)==0)
		return(KErrAccessDenied); // File must have write permission

	TPtrC filePath = aRequest->Dest().FullName().Mid(2);
	CFileCB& file = share->File();
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBRENAMEL, "this %x", (TUint) &file);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECFILECBRENAMELYS_EFILENAME, "FileName %S", filePath.Ptr(), filePath.Length()<<1);
	TRAP(r,file.RenameL(filePath));
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBRENAMELRET, "r %d", r);
	// Re-write the file's folded name & re-calculate the hash
	if (r == KErrNone)
		{
		TFileName filePathF;
		filePathF.CopyF(filePath);
	    TRAP(r, AllocBufferL(file.iFileNameF, filePathF));
		file.iNameHash=CalcNameHash(*file.iFileNameF);
		}

	return(r);
	}

TInt TFsFileRename::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=DoInitialise(aRequest);
	if(r!=KErrNone)
		return(r);
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TFileName newName;
	TRAP(r,aRequest->ReadL(KMsgPtr0,newName));
	if(r!=KErrNone)
		return(r);
	TDriveUnit currentDrive(share->File().Mount().Drive().DriveNumber());
	TDriveName driveName=currentDrive.Name();
	r=aRequest->Dest().Set(newName,&share->File().FileName(),&driveName);
	if (r!=KErrNone)
		return(r);
	if (aRequest->Dest().IsWild())
		return(KErrBadName);
	TInt driveNo;
	if ((r=RFs::CharToDrive(aRequest->Dest().Drive()[0],driveNo))!=KErrNone)
		return(r);
	TDrive& drive=TheDrives[driveNo];
 	if(drive.IsSubsted())
		{
		if ((drive.Subst().Length()+aRequest->Dest().FullName().Length())>(KMaxFileName+3))
			return(KErrBadName);
		TFileName n=drive.Subst();
		n+=aRequest->Dest().FullName().Mid(3);
		r=aRequest->Dest().Set(n,NULL,NULL);
		if(r!=KErrNone)
			return(r);
		}

	TDriveUnit newDrive(aRequest->Dest().Drive());
	if (newDrive!=currentDrive)
		return(KErrBadName);
	if (IsIllegalFullName(aRequest->Dest().FullName().Mid(2)))
		return(KErrBadName);
	r=PathCheck(aRequest->Message(),aRequest->Dest().FullName().Mid(2),&KCapFsSysFileRename,&KCapFsPriFileRename,&KCapFsROFileRename, __PLATSEC_DIAGNOSTIC_STRING("File Rename"));
	return(r);
	}


TInt TFsFileDrive::DoRequestL(CFsRequest* aRequest)
//
// Get the drive info for the file.
//
	{
	__PRINT(_L("TFsFileDrive::DoRequestL(CFsRequest* aRequest)"));
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TDrive *dr = &share->File().Drive();

	TPckgBuf<TInt> pkiF(dr->DriveNumber());		// copy drive number to user
	aRequest->WriteL(KMsgPtr0, pkiF);

	TDriveInfo di;								// copy drive info to user
	dr->DriveInfo(di);
	TPckgC<TDriveInfo> pkdiF(di);
	aRequest->WriteL(KMsgPtr1, pkdiF);

	return(KErrNone);
	}


TInt TFsFileDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return(DoInitNoParse(aRequest));
	}


TInt TFsFileDuplicate::DoRequestL(CFsRequest* aRequest)
//
// Duplicate the received file handle ready for transfer to another process.
// The new file handle is written back to the client in a a mangled form to prevent
// it from being used. Calling TFsFileAdopt will de-mangle the handle.
//
	{
	CFileShare* pS = (CFileShare*)aRequest->ScratchValue();

	// get the file control block from the client's file share
	CFileCB& fileCB = pS->File();

	// Create a new file share and initialize it with the 
	// client file share's file control block, position & mode
	
	CFileShare* pNewFileShare = new CFileShare(&fileCB);
	if (pNewFileShare == NULL)
		return KErrNoMemory;

	// We need to call CFileCB::PromoteShare immediately after the CFileShare 
	// instance is created since the destructor calls CFileCB::DemoteShare()
	// which checks the share count is non-zero
	pNewFileShare->iMode = pS->iMode;
	fileCB.PromoteShare(pNewFileShare);
	
	
	TInt r = fileCB.Open();	// increment the ref count

	if (r == KErrNone)
		TRAP(r, pNewFileShare->InitL());
	__e32_atomic_store_ord64(&pNewFileShare->iPos, pS->iPos);
	
	// Add new file share to the global file share container
	if (r == KErrNone)
		TRAP(r, FileShares->AddL(pNewFileShare,ETrue));
	
	// Add new file share to list owned by this session
	TInt newHandle;
	if (r == KErrNone)
		TRAP(r,newHandle = aRequest->Session()->Handles().AddL(pNewFileShare,ETrue));

	if (r!=KErrNone)
		{
		pNewFileShare->Close();
		return r;
		}

	newHandle^= KSubSessionMangleBit;

	TPtrC8 pH((TUint8*)&newHandle, sizeof(TInt));
	aRequest->WriteL(KMsgPtr3, pH);
	aRequest->Session()->IncResourceCount();
	return(KErrNone);
	}

TInt TFsFileDuplicate::Initialise(CFsRequest* aRequest)
	{
	TInt handle = aRequest->Message().Int0();
	CFileShare* share = GetShareFromHandle(aRequest->Session(), handle);
	
	// If the handle is invalid, either panic (CFsMessageRequest::Dispatch() will 
	// panic if we return KErrBadHandle) or complete the request with KErrArgument.
	// The latter case is the behaviour for the (deprecated) RFile::Adopt() to
	// prevent a server from panicing if the passed file handle is invalid.
	if(!share)
		return aRequest->Message().Int1()?KErrBadHandle:KErrArgument;

	aRequest->SetDrive(&share->File().Drive());
	aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );
	return(KErrNone);
	}



TInt TFsFileAdopt::DoRequestL(CFsRequest* aRequest)
//
// Adopt the passed file handle. This assumes that the handle has already been 
// duplicated by calling TFsFileDuplicate and is therefore mangled.
//
	{
	if (((CFileShare*)aRequest->ScratchValue()) == NULL)
		return KErrBadHandle;
	
	TInt adoptType = aRequest->Message().Int1();
	// EFileBigFile mode check
	switch(adoptType)
		{
		case KFileAdopt32:
			// Request is from RFile::Adopt or RFile::AdoptFromXXX: Force NO EFileBigFile
			((CFileShare*)aRequest->ScratchValue())->iMode &= ~EFileBigFile;
			break;
		case KFileAdopt64:
			// Request is from RFile64::AdoptFromXXX: Force EFileBigFile
			((CFileShare*)aRequest->ScratchValue())->iMode |= EFileBigFile;
			break;
		case KFileDuplicate:
			// Request is from RFile::Duplucate
			// adopt original file mode - do nothing
			break;
		//default:
			// Do nothing
		}
	
	// De-mangle the existing sub-session handle and return it to the client
	TInt newHandle = aRequest->Message().Int0() ^ KSubSessionMangleBit;

	TPtrC8 pH((TUint8*)&newHandle,sizeof(TInt));
	aRequest->WriteL(KMsgPtr3,pH);
	return(KErrNone);
	}

TInt TFsFileAdopt::Initialise(CFsRequest* aRequest)
	{
	TInt handle = aRequest->Message().Int0() ^KSubSessionMangleBit;

	CFileShare* share = GetShareFromHandle(aRequest->Session(), handle);
	// Normally returning KErrBadHandle will result in a panic, but when a server calls 
	// RFile::AdoptXXX() and it's client has given it a bad handle then it's not a good 
	// idea to panic the server. So we return KErrNone here and allow 
	// TFsFileAdopt::DoRequestL() to return KErrBadHandle
	if (share)
		aRequest->SetDrive(&share->File().Drive());

	aRequest->SetScratchValue64( MAKE_TINT64(ETrue, (TUint) share) );
	return(KErrNone);
	}


TInt TFsFileName::DoRequestL(CFsRequest* aRequest)
//
// Get the name of a file. 
// i.e. including the name & extension but excluding the drive and path
//
	{
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();

	// Search backwards until a backslash is found
	// This should always succeed as this is a full pathname
	TPtrC name(share->File().FileName());
	TInt offset = name.LocateReverse('\\');
	aRequest->WriteL(KMsgPtr0, name.Mid(offset+1));

	return(KErrNone);
	}

TInt TFsFileName::Initialise(CFsRequest* aRequest)
//
// Get the full name of a file, including path and drive 
//
//
	{
	return InitialiseScratchToShare(aRequest);
	}
	
	
TInt TFsFileFullName::DoRequestL(CFsRequest* aRequest)
//
// Get the full name of a file, including path and drive 
//
	{
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();

	// Write the drive letter and ':'
	TBuf<2> driveBuf(KDrivePath);
	driveBuf[0]=TText('A' + share->File().Drive().DriveNumber());
	aRequest->WriteL(KMsgPtr0, driveBuf);

	// Write the file and path including leading '\'
	TPtrC name(share->File().FileName());
	aRequest->WriteL(KMsgPtr0, name, 2);
	
	return(KErrNone);
	}


TInt TFsFileFullName::Initialise(CFsRequest* aRequest)
//
//
//
	{
	return InitialiseScratchToShare(aRequest);
	}

TInt TFsGetMediaSerialNumber::DoRequestL(CFsRequest* aRequest)
//
// Acquire capability from media and return serial number if supported.
//
    {
    // Get request parameters
    const TInt drvNum = aRequest->Message().Int1();

	// Get media capability
    TLocalDriveCapsV5Buf capsBuf;

	TInt r = KErrNone;

	// is the drive local?
	if (!IsProxyDrive(drvNum))
		{
		// if not valid local drive, use default values in localDriveCaps
		// if valid local drive and not locked, use TBusLocalDrive::Caps() values
		// if valid drive and locked, hard-code attributes
		r = GetLocalDrive(drvNum).Caps(capsBuf);
		}
	else  // this need to be made a bit nicer
		{   
		CExtProxyDrive* pD = GetProxyDrive(drvNum);
		if(pD)
			r = pD->Caps(capsBuf);
		else
			r = KErrNotReady;
		}
	
	if (r != KErrNone)
        return r;

    TLocalDriveCapsV5& capsV5 = capsBuf();

    // Return serial number if supported
    if (capsV5.iSerialNumLength == 0)
        return KErrNotSupported;

    TPtrC8 snPtr(capsV5.iSerialNum, capsV5.iSerialNumLength);
    aRequest->WriteL(KMsgPtr0, snPtr);

	return KErrNone;
    }

TInt TFsGetMediaSerialNumber::Initialise(CFsRequest* aRequest)
//
// Validate drive number and its attributes passed in a request object.
//
    {
    const TInt drvNum = aRequest->Message().Int1();

    TInt nRes = ValidateDrive(drvNum, aRequest);
    if(nRes != KErrNone)
        return KErrBadName; //-- incorrect drive number

    if(aRequest->Drive()->IsSubsted())
        return KErrNotSupported; //-- the drive is substed, this operation doesn't make a sense
    
    if(!IsValidLocalDriveMapping(drvNum))
	    return KErrNotReady;

    return KErrNone;
    }

TInt TFsBlockMap::Initialise(CFsRequest* aRequest)
	{
	TInt r = DoInitNoParse(aRequest);
	if(r!=KErrNone)
		return r;

	TInt blockMapUsage = aRequest->Message().Int2();
	if ( blockMapUsage == EBlockMapUsagePaging )
		{
		CFileShare* share = (CFileShare*) aRequest->ScratchValue();
		CFileCB& file = share->File();

		// To determine whether the drive where this file resides is pageable, we need to locate 
		// the specific TBusLocalDrive object first; querying the drive attributes directly from the 
		// (composite) file system is no good as this API treats all slave file systems as "ROM & not pageable.
		TBusLocalDrive* localDrive;
		TInt r = file.LocalDrive(localDrive);
		if (r != KErrNone)
			return r;
		TLocalDriveCapsV4Buf caps;
		r = localDrive->Caps(caps);
		if (r != KErrNone)
			return r;
		__PRINT4(_L("TFsBlockMap::Initialise, drive %d file %S iMediaAtt %08X iDriveAtt %08X\n"), file.DriveNumber(), &file.FileName(), caps().iMediaAtt, caps().iDriveAtt);
		if ( !(caps().iDriveAtt & KDriveAttPageable))
			return KErrNotSupported;
		}

	return KErrNone;
	}

TInt TFsBlockMap::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsBlockMap::DoRequestL(CFsRequest* aRequest)"));
	__PRINT1(_L("aRequest->Session() = 0x%x"), aRequest->Session());

	CFileShare* share = (CFileShare*) aRequest->ScratchValue();

	TInt r = share->CheckMount();

	__PRINT1(_L("share->CheckMount() returned - %d"), r);
	if ( r != KErrNone )
		return(r);

	SBlockMapInfo reqInfo;
	SBlockMapArgs args;
	TPckg<SBlockMapArgs> pkArgs(args);
	aRequest->ReadL(KMsgPtr1, pkArgs);

	CFileCB& file = share->File();
	TInt64& reqStartPos = args.iStartPos;
	TInt64 reqEndPos = args.iEndPos;

	if ( ( reqStartPos > file.Size64() ) || ( reqStartPos < 0 ) )
		return KErrArgument;

	const TInt64 KReadToEOF = -1;
	if ( reqEndPos != KReadToEOF )
 		{
		if ( !( reqEndPos >= reqStartPos ) )
			return KErrArgument;
		}
	else
		reqEndPos = file.Size64();

	// If the requested start position is equal to the size of the file
	// then we read no data and return an empty BlockMap.
	if ( reqStartPos == file.Size64() || reqEndPos == 0 || reqEndPos == reqStartPos )
		{
		TPckg<SBlockMapInfo> pkInfo(reqInfo);
		TRAP(r,aRequest->WriteL(KMsgPtr0,pkInfo) );
 		if ( r == KErrNone )
 			return(KErrArgument);
		else
			return(r);
		}
	r = share->File().BlockMap(reqInfo, reqStartPos, reqEndPos);
	TPckg<SBlockMapInfo> pkInfo(reqInfo);
	aRequest->WriteL(KMsgPtr0, pkInfo);
	aRequest->WriteL(KMsgPtr1, pkArgs);

	return(r);
	}

#pragma warning( disable : 4705 )	// statement has no effect
/**
Default constructor
*/
EXPORT_C CFileCB::CFileCB()
	{
	}
#pragma warning( default : 4705 )




/**
Destructor.

Frees resources before destruction of the object.
*/
EXPORT_C CFileCB::~CFileCB()
	{
	// NB Must be careful to close the file cache BEFORE deleting iFileNameF
	// as CFileCache may steal it (!)
	if (FileCache())
		FileCache()->Close();
	if (iBody && iBody->iDeleteOnClose)
		{
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL2, "drive %d", DriveNumber());
		OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL2_EFILENAME, "FileName %S", FileName().Ptr(), FileName().Length()<<1);
		TInt r;
		TRAP(r, iMount->DeleteL(FileName()));
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL2RET, "r %d", r);
		}

	if(iMount)
		iMount->Close();
	if (iMountLink.iNext!=NULL)
		{
		iMountLink.Deque();
		}
	delete iFileName;
	delete iFileNameF;
	if (iFileLocks)
		{
		iFileLocks->Close();
		delete iFileLocks;
		}

	delete iBody;
	}

/**
    Initialise CFileCB object.
    @internalTechnology

    @param  aDrive
    @param  aCreatedDrive
    @param  aName           file name descriptor
*/
EXPORT_C void CFileCB::InitL(TDrive* aDrive, TDrive* aCreatedDrive, HBufC* aName)
	{
	// Take ownership of heap-allocated objects aName and aLock before attempting any memory allocation 
	// to avoid leaking memory
	iFileName = aName;

    DoInitL(aDrive->DriveNumber());
	iDrive=aDrive;
	iCreatedDrive=aCreatedDrive;
	TFileName tempName;
	tempName.CopyF(*aName);
	iFileNameF=tempName.AllocL();
	iNameHash=CalcNameHash(*iFileNameF);
	

	
	// see whether the file system supports the CFileCB extended API
	MExtendedFileInterface* extendedFileInterface = NULL;
	GetInterfaceTraced(CFileCB::EExtendedFileInterface, (void*&) extendedFileInterface, NULL);
	iBody = new(ELeave)CFileBody(this, extendedFileInterface);

	iMount=&iDrive->CurrentMount();
	iBody->InitL();
	User::LeaveIfError(iMount->Open());
	
	//-- create file locks array
    ASSERT(!iFileLocks);
    iFileLocks = new(ELeave) TFileLocksArray(KFileShareLockGranularity, _FOFF(TFileShareLock, iPosLow));

    }
	

TInt CFileCB::FindLock(TInt aPosLow,TInt aPosHigh)
	{
	return FindLock64(aPosLow, aPosHigh);
	}

TInt CFileCB::AddLock(CFileShare* aFileShare,TInt aPos,TInt aLength)
	{
	return AddLock64(aFileShare, aPos, aLength);
	}

TInt CFileCB::RemoveLock(CFileShare* aFileShare,TInt aPos,TInt aLength)
	{
	return RemoveLock64(aFileShare, aPos, aLength);
	}

TInt CFileCB::CheckLock(CFileShare* aFileShare,TInt aPos,TInt aLength)
	{
	return CheckLock64(aFileShare, aPos, aLength);
	}

/**
    Remove any locks held by aFileShare.
*/
void CFileCB::RemoveLocks(CFileShare* aFileShare)
	{

	TInt i=0;
	while (i<FileLocks().Count())
		{
		if (FileLocks()[i].MatchOwner(aFileShare))
			FileLocks().Remove(i);
		else
			i++;
		}
	}


void CFileCB::PromoteShare(CFileShare* aShare)
/**
	Manages share promotion and checks the EFileSequential file mode
	after the share has been added to the FileShares container.
	
	It assumes the share has already been validated using ValidateShare().
	
	The count of promoted shares (ie - non-EFileShareReadersOrWriters) is incremented
	to allow the share mode to be demoted when the last promoted share is closed.
	
	Similarly, the count of non-EFileSequential file modes is incremented to allow
	the file mode to be enabled when the last non-EFileSequential share is closed.
 */
	{
	TShare reqShare = (TShare)(aShare->iMode & KFileShareMask);
	if(reqShare != EFileShareReadersOrWriters)
		{
		iBody->iPromotedShares++;
		iShare = reqShare;
		}
	
	// If the file mode is not EFileSequential, then disable the 'Sequential' flag
	if(!(aShare->iMode & EFileSequential))
		{
		iBody->iNonSequentialFileModes++;
		SetSequentialMode(EFalse);
		__PRINT(_L("CFileCB::PromoteShare - FileSequential mode is off"));
		}
	}


void CFileCB::DemoteShare(CFileShare* aShare)
/**
	Manages share demotion and checks the EFileSequential file mode
	after the share has been removed from the FileShares container.
	
	If the share being removed is not EFileShareReadersOrWriters, then the current
	share mode may require demotion back to EFileShareReadersOrWriters.
	This is determined by the iPromotedShares count, incremented in PromoteShare().
	
	Similarly, if the share being removed is non-EFileSequential,
	then the EFileSequential flag may need to be enabled,
	which is determined by the iNonSequentialFileModes count.
 */
	{
	if((aShare->iMode & KFileShareMask) != EFileShareReadersOrWriters
		&& --iBody->iPromotedShares == 0)
		{
		// Don't worry if the file has never been opened as EFileShareReadersOrWriters
		//  - in this case the CFileCB object is about to be closed anyway.
		iShare = EFileShareReadersOrWriters;
		}
	__ASSERT_DEBUG(iBody->iPromotedShares>=0, Fault(EFileShareBadPromoteCount));
	
	if(!(aShare->iMode & EFileSequential) && --iBody->iNonSequentialFileModes == 0)
		{
		// As above, if the file has never been opened as EFileSequential,
		// it implies that the CFileCB object is about to be closed anyway.
		SetSequentialMode(ETrue);
		__PRINT(_L("CFileCB::PromoteShare - FileSequential mode is enabled"));
		}
	__ASSERT_DEBUG(iBody->iNonSequentialFileModes>=0, Fault(EFileShareBadPromoteCount));
	}


RArray<TAsyncReadRequest>& CFileCB::AsyncReadRequests()
//
// Gets a reference to the pending asynchronous read requests for this file.
//
//  - The request is completed when all data is available or the request is cancelled.
//
	{
	return(*iBody->iAsyncReadRequests);
	}


TInt CFileCB::AddAsyncReadRequest(CFileShare* aShareP, TInt64 aPos, TInt aLen, CFsRequest* aRequestP)
//
// Adds a pending asynchronous read request to the list.
//
//  - The request is completed when all data is available or the request is cancelled.
//
	{

	__ASSERT_ALWAYS(aRequestP->Operation()->Function() == EFsFileRead, Fault(EBaseRequestMessage));
	
	TAsyncReadRequest req(aPos + aLen, aShareP, aRequestP);
	TInt err = AsyncReadRequests().InsertInSignedKeyOrderAllowRepeats(req);
	if(err != KErrNone)
		return err;

	aRequestP->SetCompleted(EFalse);
	return KErrNone;
	}


TInt CFileCB::CancelAsyncReadRequest(CFileShare* aShareP, TRequestStatus* aStatusP)
//
// Cancels (and completes) an outstanding read request for the specified share.
//
//	- aStatusP == NULL cancels all outstanding read requests.
//
	{

	TInt i=0;
	Drive().Lock();
	while (i < AsyncReadRequests().Count())
		{
		TAsyncReadRequest& req = AsyncReadRequests()[i];
		if(req.CompleteIfMatching(aShareP, aStatusP, KErrCancel))
			{
			iBody->iAsyncReadRequests->Remove(i);
			if(aStatusP != NULL)
				{
				Drive().UnLock();
				return KErrNone;
				}
			}
		else
			{
			i++;
			}
		}

	Drive().UnLock();
	return KErrNone;
	}


void CFileCB::NotifyAsyncReaders()
//
// Determine if any outstanding read requests require completion.
//
//  - Called whenever the file size changes (due to a write operation or SetSize)
//
//  - Any outstanding read requests are re-issued (rather then completed in the 
//    context of the current operation so not to affect performance of the writer).
//
//  - Should the file size shrink before the request is serviced, the request will
//    be added back onto the queue.
//
//	- A future optimisation may issue the request as data becomes available (which
//	  would minimise the final latency between writer and reader) but the current
//	  implementation reads all requested data in one operation.
//
	{
	Drive().Lock();
	
	SetNotifyAsyncReadersPending(EFalse);

	while(AsyncReadRequests().Count())
		{
		TAsyncReadRequest& req = AsyncReadRequests()[0];
		if(req.iEndPos > CachedSize64())
			break;

		// Make a copy and then remove it from the queue before releasing the lock - 
		// because the file server thread could append to the RArray and this might 
		// cause a re-allocation which would move the whole array (!)
		TAsyncReadRequest reqCopy = req;
		AsyncReadRequests().Remove(0);
		Drive().UnLock();

		// allocate a new request, push a TMsgOperation onto the request's stack (duplicating
		// the functionality of TFsFileRead::Initialise()) & dispatch the request
		CFsClientMessageRequest* pRequest = NULL;
		const TOperation& oP = OperationArray[EFsFileRead];
		TInt r = RequestAllocator::GetMessageRequest(oP, reqCopy.iMessage, pRequest);
		if (r != KErrNone)
			{
			reqCopy.iMessage.Complete(r);	// complete the client's message with an error
			continue;
			}
		pRequest->Set(reqCopy.iMessage, oP, reqCopy.iSessionP);

		r = DoInitNoParse(pRequest);
		if (r != KErrNone)
			{
			pRequest->Complete(r);		// complete the client's message with an error
			continue;
			}
		
		r = pRequest->PushOperation(TFsFileRead::Complete);
		if (r != KErrNone)
			{
			pRequest->Complete(r);		// complete the client's message with an error
			continue;
			}

		pRequest->CurrentOperation().Set(
			reqCopy.iEndPos - pRequest->Message().Int1(), 
			pRequest->Message().Int1(), 
			(TDes8*) pRequest->Message().Ptr0());

		// don't call Initialise()
		pRequest->SetState(CFsRequest::EReqStatePostInitialise);

		pRequest->Dispatch();
		Drive().Lock();
		}
	Drive().UnLock();
	}


/**
Gets the address of the file that the file control block represents. 

The default implementation returns KErrNotSupported and should only
be overridden for ROM file systems.

@param aPos On return, should contain the address of the file that
            the file control block represents.

@return KErrNone, if successful,otherwise one of the other system wide error
        codes,
*/
EXPORT_C TInt CFileCB::Address(TInt& /*aPos*/) const
	{

	return(KErrNotSupported);
	}


/**
Sets the archive attribute, KEntryAttArchive, in iAtt.
*/
EXPORT_C void CFileCB::SetArchiveAttribute()
	{

	iAtt|=KEntryAttArchive;
	iAtt|=KEntryAttModified;
	iModified.UniversalTime();
	}


EXPORT_C TInt CFileCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* /*aInput*/)
	{
	switch(aInterfaceId)
		{
		case EGetLocalDrive:
			return Mount().LocalDrive((TBusLocalDrive*&) aInterface);
		default:
			return(KErrNotSupported);
		}
	}


CFileCache* CFileCB::FileCache() const
	{return iBody?iBody->iFileCache:NULL;}

TBool CFileCB::LocalBufferSupport() const
	{return iBody?iBody->iLocalBufferSupport:EFalse;}

void CFileCB::SetLocalBufferSupport(TBool aEnabled)
	{iBody->iLocalBufferSupport = aEnabled;}

TInt CFileCB::CheckMount()
//
// Check that the media is still mounted.
//
	{

	TDrive& d = Drive();
	TInt r=d.CheckMount();
	if (r!=KErrNone)
		return(r);
	if (&Mount() != &d.CurrentMount())
		return(KErrDisMounted);
	if (FileCorrupt())
		return(KErrCorrupt);
	if (BadPower())
		{
		if (PowerOk())
			SetBadPower(EFalse);
		else
			return(KErrBadPower);
		}
	return(KErrNone);
	}

TInt64 CFileCB::CachedSize64() const
	{
	CFileCache* fileCache = iBody?iBody->iFileCache:NULL;
	return fileCache? fileCache->Size64(): Size64();
	}

void CFileCB::SetCachedSize64(TInt64 aSize)
	{
	if (FileCache())
		FileCache()->SetSize64(aSize);
	else
		SetSize64(aSize, EFalse);	// assume not locked
	}

/**
Constructor.
Locks the mount resource to which the shared file resides
and adds the share to the file's FileShare List.

@param aFileCB File to be shared.
*/
CFileShare::CFileShare(CFileCB* aFileCB)
	: iFile(aFileCB)
	{
	AddResource(iFile->Mount());
	iFile->AddShare(*this);
	}

/**
Destructor.

Frees mount resource to which the shared file resides,
removes the share from the file's FileShare List,
removes share status from the shared file and finally closes
the file.
*/
CFileShare::~CFileShare()
	{
	// We shouldn't be deleting the file share with a valid request queue or there will be request (& memory) leakage
	__ASSERT_DEBUG(iCurrentRequest == NULL, Fault(ERequestQueueNotEmpty));

	RemoveResource(iFile->Mount());
	iShareLink.Deque();
	iFile->RemoveLocks(this);
	iFile->DemoteShare(this);
	iFile->CancelAsyncReadRequest(this, NULL);
	iFile->Close();
	}



/**
Check that the media is still mounted.

@return KErrNone if successful.
        KErrDisMounted if media has dismounted.
        KErrCorrupted if shared file is corrupted.
        KErrBadPower if insufficent power supply.
        or other system wide error code.
*/
TInt CFileShare::CheckMount()
	{
	return File().CheckMount();
	}

/**
Initialise the object
*/
void CFileShare::InitL()
	{
	DoInitL(iFile->Drive().DriveNumber());

	// override the close operation so that we can flush the write cache if necessary
	iRequest->Set(FileShareCloseOp,NULL);
	iRequest->SetDriveNumber(DriveNumber());
	iRequest->SetScratchValue((TUint)this);
	}

// Mark the start of a request - 
// the is to prevent fair-scheduled async read/writes from being processed out of sequence. This is especially 
// important when considering a client which appends to a file by issuing more than one asynchronous request as each
// write request must be entirely satisfied before a subsequent request can append to the file
TBool CFileShare::RequestStart(CFsMessageRequest* aRequest)
	{
	TBool ret;
	
	TDrive& drive = File().Drive();
	drive.Lock();

	if (iCurrentRequest == NULL || iCurrentRequest == aRequest)
		{
		iCurrentRequest = aRequest;
		ret = ETrue;
		}
	else
		{
		// add to end of linked list of requests if there is already an active request for this share
		CFsClientMessageRequest* request;
		for (request = (CFsClientMessageRequest*) iCurrentRequest; request->iNext != NULL; request = request->iNext)
			{
			}
		request->iNext = (CFsClientMessageRequest*) aRequest;
		ret =  EFalse;
		}

	drive.UnLock();
	return ret;
	}


// Mark the end of a request
void CFileShare::RequestEnd(CFsMessageRequest* aRequest)
	{
	TDrive& drive = File().Drive();
	drive.Lock();

	if (aRequest == iCurrentRequest)
		{
		// Any requests in the queue ?
		if (((CFsClientMessageRequest*) iCurrentRequest)->iNext)
			{
			iCurrentRequest = ((CFsClientMessageRequest*) aRequest)->iNext;
			((CFsClientMessageRequest*) aRequest)->iNext = NULL;

			// if the current request has been cancelled, cancel all requests in the queue
			TInt lastError = aRequest->LastError();
			if (lastError == KErrCancel || lastError == KErrNotReady)
				{
				// take ownership of this queue and make it invisible to anyone else by setting iCurrentRequest to NULL
				CFsClientMessageRequest* currentRequest = (CFsClientMessageRequest*) iCurrentRequest;
				iCurrentRequest = NULL;
				drive.UnLock();
				while(currentRequest)
					{
					CFsClientMessageRequest* nextRequest = ((CFsClientMessageRequest*) currentRequest)->iNext;
					((CFsClientMessageRequest*) currentRequest)->iNext = NULL;
					currentRequest->Complete(lastError);
					currentRequest = nextRequest;
					}
				}
			else
				{
				drive.UnLock();
				iCurrentRequest->Dispatch(EFalse);
				}
			}
		else	// queue empty
			{
			iCurrentRequest = NULL;
			drive.UnLock();
			}
		}
	else	// if (aRequest == iCurrentRequest)
		{
		drive.UnLock();
		}
	}

TBool CFileShare::RequestInProgress() const
	{
	return (iCurrentRequest != NULL)?(TBool)ETrue:(TBool)EFalse;
	}



/**
Initialise the object
*/
TInt TFsCloseFileShare::DoRequestL(CFsRequest* aRequest)
//
	{
	__PRINT(_L("TFsCloseFileCache::DoRequestL()"));
	
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();

	// flush the write cache before closing the file share
	TInt r;
	CFileCache* fileCache = share->File().FileCache();
	if (fileCache && (r = fileCache->FlushDirty(aRequest)) == CFsRequest::EReqActionBusy)
		return r;
	
	return KErrNone;
	}

TInt TFsCloseFileShare::Complete(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsCloseFileShare::Complete()"));
	return TFsCloseObject::Complete(aRequest);
	}


TAsyncReadRequest::TAsyncReadRequest(TInt64 aEndPos, CFileShare* aOwningShareP, CFsRequest* aRequestP)
//
// Constructor for TAsyncReadRequest
//	- Maintains information about oustanding async read requests
//
  : iEndPos(aEndPos), 
	iOwningShareP(aOwningShareP)
	{
	iMessage  = aRequestP->Message();
	iSessionP = aRequestP->Session();
	iStatusP  = iMessage.ClientStatus();
	}


TBool TAsyncReadRequest::CompleteIfMatching(CFileShare* aShareP, TRequestStatus* aStatusP, TInt aError)
//
// Completes an asynchronous read request.
//
	{
	if (iOwningShareP == aShareP && (aStatusP == NULL || aStatusP == iStatusP))
		{
		iMessage.Complete(aError);
		return ETrue;
		}

	return EFalse;
	}


TInt TFsFileReadCancel::Initialise(CFsRequest* aRequest)
//
// Initialise function for RFile::ReadCancel [EFsReadCancel]
//
	{
	return InitialiseScratchToShare(aRequest);
	}


TInt TFsFileReadCancel::DoRequestL(CFsRequest* aRequest)
//
// Request function for RFile::ReadCancel [EFsReadCancel]
//
	{
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	TRequestStatus* status = (TRequestStatus*)aRequest->Message().Ptr0();
	share->File().CancelAsyncReadRequest(share, status);
	return(KErrNone);
	}

void CFileCB::ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{
	TRACETHREADID(aMessage);
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECFILECBREADLA, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	iBody->iExtendedFileInterface->ReadL(aPos,aLength,aDes,aMessage,aOffset);

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBREADLRET, "r %d", KErrNone);
	}

void CFileCB::WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{
	TRACETHREADID(aMessage);
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECFILECBWRITEL, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	iBody->iExtendedFileInterface->WriteL(aPos,aLength,aDes,aMessage,aOffset);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBWRITELRET, "r %d", KErrNone);
	}

void CFileCB::SetSizeL(TInt64 aSize)
	{
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECFILECBSETSIZEL, "this %x aSize %x:%x", (TUint) this, (TUint) I64HIGH(aSize), (TUint) I64LOW(aSize));
	iBody->iExtendedFileInterface->SetSizeL(aSize);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBSETSIZELRET, "r %d", KErrNone);
	}

TBool CFileCB::ExtendedFileInterfaceSupported()
	{
	return iBody->ExtendedFileInterfaceSupported();
	}

TInt CFileCB::FairSchedulingLen() const
	{
	return iBody->iFairSchedulingLen;
	}

void CFileCB::SetNotifyAsyncReadersPending(TBool aNotifyAsyncReadersPending)
//
// Notify the asynchronous reader that a file has grown so that it may service outstanding async reads
//
	{
	iBody->iNotifyAsyncReadersPending = aNotifyAsyncReadersPending;
	}

TBool CFileCB::NotifyAsyncReadersPending() const
	{
	return iBody->iNotifyAsyncReadersPending;
	}


void CFileCB::ResetReadAhead()
	{
	CFileCache* fileCache = FileCache();
	if (fileCache)
		fileCache->ResetReadAhead();
	}

void CFileCB::SetDeleteOnClose()
	{
	iBody->iDeleteOnClose = ETrue;
	}

TBool CFileCB::DeleteOnClose() const
	{
	return iBody->iDeleteOnClose;
	}

TInt CFileCB::GetInterfaceTraced(TInt aInterfaceId, TAny*& aInterface, TAny* aInput)
	{
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILECBGETINTERFACE, "aInterfaceId %d aInput %x", (TUint) aInterfaceId, (TUint) aInput);
	TInt r = GetInterface(aInterfaceId, aInterface, aInput);
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILECBGETINTERFACERET, "r %d aInterface %x", (TUint) r, (TUint) aInterface);
	return r;
	}

CFileBody::CFileBody(CFileCB* aFileCB, CFileCB::MExtendedFileInterface* aExtendedFileInterface)
  : iFileCB(aFileCB),
	iExtendedFileInterface(aExtendedFileInterface ? aExtendedFileInterface : this),
	iShareList(_FOFF(CFileShare,iShareLink)),
	iSizeHigh(0)
	{
	iFairSchedulingLen = TFileCacheSettings::FairSchedulingLen(iFileCB->DriveNumber());
    iMaxSupportedFileSize = KMaxSupportedFileSize;
	}


CFileBody::~CFileBody()
	{
	if (iAsyncReadRequests)
		{
		iAsyncReadRequests->Close();
		delete iAsyncReadRequests;
		}
	}


void CFileBody::InitL()
	{
	iAsyncReadRequests = new(ELeave) RArray<TAsyncReadRequest>(KAsyncRequestArrayGranularity, _FOFF(TAsyncReadRequest, iEndPos));
	}



TInt TFsFileClamp::Initialise(CFsRequest* aRequest)
//
// Initialise function for RFile::Clamp [EFsFileClamp]
//
	{
	TSecureId aUID = aRequest->Message().SecureId();	
    if (aUID!=KEstartUidValue && aUID!=KFileServerUidValue)
		{
		SSecurityInfo info;
		info.iVendorId=0;
		info.iCaps.iCaps[0]=0;
		info.iCaps.iCaps[1]=0;
		info.iSecureId=KEstartUidValue;
		PlatSec::PolicyCheckFail(aRequest->Message(),info,"File Clamp");
		info.iSecureId=KFileServerUidValue;
		PlatSec::PolicyCheckFail(aRequest->Message(),info,"File Clamp");
		return KErrPermissionDenied;
		}

	TInt r=DoInitialise(aRequest);
	if(r!=KErrNone)
		return r;

	// The clamp API is only supported on non-removable media
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	TDriveInfo di;
	share->File().Drive().DriveInfo(di);
	if (!(di.iDriveAtt & KDriveAttInternal))
		r = KErrNotSupported;

	return(r);
	}


TInt TFsFileClamp::DoRequestL(CFsRequest* aRequest)
//
// Request function for RFile::Clamp [EFsFileClamp]
//
	{
	TInt r;

	// Flush data for this file, if it is open for writing, before clamping
	CFileShare* share=(CFileShare*)aRequest->ScratchValue();

	if (((share->iMode&EFileWrite)) || ((share->File().Att()&KEntryAttModified)))
		{
		r=share->CheckMount();
		if (r!=KErrNone)
			return(r);
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBFLUSHDATAL2, "this %x", &share->File());
		TRAP(r,share->File().FlushDataL());
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILECBFLUSHDATAL2RET, "r %d", r);
		if(r!=KErrNone)
			return(r);
		}

	RFileClamp clamp;
	r=aRequest->Drive()->ClampFile(aRequest->Src().FullName().Mid(2),
									(TAny*)(&clamp));
	// Write clamp information to user
	TPckgC<RFileClamp> pkClamp(clamp);
	aRequest->WriteL(KMsgPtr0, pkClamp);
	return r;
	}


TInt TFsUnclamp::Initialise(CFsRequest* aRequest)
//
// Initialise function for RFs::Unclamp [EFsUnclamp]
//
	{
	TSecureId aUID = aRequest->Message().SecureId();	
    if (aUID!=KEstartUidValue && aUID!=KFileServerUidValue)
		{
		SSecurityInfo info;
		info.iVendorId=0;
		info.iCaps.iCaps[0]=0;
		info.iCaps.iCaps[1]=0;
		info.iSecureId=KEstartUidValue;
		PlatSec::PolicyCheckFail(aRequest->Message(),info,"File Unclamp");
		info.iSecureId=KFileServerUidValue;
		PlatSec::PolicyCheckFail(aRequest->Message(),info,"File Unclamp");
		return KErrPermissionDenied;
		}
	RFileClamp clamp;
	TPckg<RFileClamp> pkClamp(clamp);
	aRequest->ReadL(KMsgPtr0, pkClamp);
	TInt driveNo=(I64HIGH(clamp.iCookie[1]));
	TDrive& drive=TheDrives[driveNo];
	aRequest->SetDrive(&drive);
	return KErrNone;
	}


TInt TFsUnclamp::DoRequestL(CFsRequest* aRequest)
//
// Request function for RFs::Unclamp [EFsUnclamp]
//
	{
	RFileClamp clamp;
	TPckg<RFileClamp> pkClamp(clamp);
	aRequest->ReadL(KMsgPtr0, pkClamp);
	TDrive* drive=aRequest->Drive();
	CMountCB* mount=(CMountCB*)&(drive->CurrentMount());
	return(drive->UnclampFile(mount,&clamp));
	}

CMountBody::CMountBody(CMountCB* aMountCB, CMountCB::MFileAccessor* aFileAccessor, CMountCB::MFileExtendedInterface* aFileInterface)
//
// Constructor for private body class
//
  : iMountCB(aMountCB),
	iFileAccessor(aFileAccessor?aFileAccessor:this),
	iFileExtendedInterface(aFileInterface?aFileInterface:this)
	{
	}

CMountBody::~CMountBody()
//
// Destructor for private body class
//
	{
	__ASSERT_DEBUG(iClampIdentifiers.Count() == 0, User::Invariant());
	iClampIdentifiers.Close();
	}

TInt CMountBody::ClampFile(const TInt aDriveNo,const TDesC& aName,TAny* aHandle)
	{
	// Need CMountCB::MFileAccessor interface support
	if(iFileAccessor==this)
		return KErrNotSupported;

	// Get unique identifier for the file
	TInt64 uniqueId = 0;
	TInt r = iFileAccessor->GetFileUniqueId(aName,uniqueId);
	if(r!=KErrNone)
			return r;

	// Populate the RFileClamp clamp instance and store it in iClampIdentifiers
	RFileClamp* newClamp = (RFileClamp*)aHandle;
	newClamp->iCookie[0]=uniqueId;
	newClamp->iCookie[1]=MAKE_TINT64(aDriveNo,++iClampCount);
	r = iClampIdentifiers.InsertInOrder((const RFileClamp&)*newClamp,&CompareClampsByIdAndCount);
	if(r != KErrNone)
		return r;

	// Indicate that (at least) one file is clamped on this drive
	iMountCB->Drive().SetClampFlag(ETrue);
	AddResource(*iMountCB);
	return KErrNone;
	}


TInt CMountBody::UnclampFile(RFileClamp* aHandle)
	{
	// Need CMountCB::MFileAccessor interface support
	if(iFileAccessor==this)
		return KErrNotSupported;

	TInt idx;
	if((idx = iClampIdentifiers.Find((const RFileClamp&)*aHandle,&FindClampByIdAndCount)) < KErrNone)
		{
		// This file is not 'clamped'
		return idx;
		}

	// If we're removing the last clamp and a dismount has been deferred (due to files being clamped),
	// then DeferredDismount() will trigger a dismount: before this happens we need to flush all
	// dirty data on this drive; 
	TDrive& drive = iMountCB->Drive();
	TInt noOfClamps = NoOfClamps();
	if (noOfClamps == 1 && drive.DismountDeferred())
		{
		TInt r = drive.FlushCachedFileInfo(ETrue);
		if (r == CFsRequest::EReqActionBusy)
			return r;
		}

	RemoveResource(*iMountCB);
	iClampIdentifiers.Remove(idx);

	TInt r = KErrNone;
	// If this was the last clamp, check for outstanding dismount requests
	if (noOfClamps == 1)
		{
		ASSERT(NoOfClamps() == 0);
		drive.SetClampFlag(EFalse);
		// dismount now if no clients waiting...
		r = drive.DeferredDismountCheck();
		}

	return r;
	}


TInt CMountBody::IsFileClamped(const TInt64 aUniqueId)
	{
	// Need CMountCB::MFileAccessor interface support
	if(iFileAccessor==this)
		return KErrNotSupported;

	// Encapsulate the unique identifier in an appropriate class
	RFileClamp newClamp;
	newClamp.iCookie[0]=aUniqueId;
	// Search for (any) entry in iClampIdentifiers holding this value
	TInt index=iClampIdentifiers.Find((const RFileClamp&)newClamp,&FindClampById);
	return (index==KErrNotFound?0:1);
	}

TInt CMountBody::NoOfClamps()
	{
	// Need CMountCB::MFileAccessor interface support
	if(iFileAccessor==this)
		return KErrNotSupported;

	// This will return zero if ClampFile has not previously been invoked
	return iClampIdentifiers.Count();
	}

TInt CMountBody::CompareClampsById(const RFileClamp& aClampA, const RFileClamp& aClampB)
	{
	if(aClampA.iCookie[0] < aClampB.iCookie[0]) return 1;
	if(aClampA.iCookie[0] > aClampB.iCookie[0]) return -1;
	return 0;
	}

TInt CMountBody::CompareClampsByIdAndCount(const RFileClamp& aClampA, const RFileClamp& aClampB)
	{
	if(aClampA.iCookie[0] > aClampB.iCookie[0]) return 1;
	if(aClampA.iCookie[0] < aClampB.iCookie[0]) return -1;
	// Now compare the count values
	if(I64LOW(aClampA.iCookie[1]) > I64LOW(aClampB.iCookie[1])) return 1;
	if(I64LOW(aClampA.iCookie[1]) < I64LOW(aClampB.iCookie[1])) return -1;
	return 0;
	}

TInt CMountBody::FindClampById(const RFileClamp& aClampA, const RFileClamp& aClampB)
	{
	return (TInt)(!CompareClampsById(aClampA, aClampB));
	}


TInt CMountBody::FindClampByIdAndCount(const RFileClamp& aClampA, const RFileClamp& aClampB)
	{
	return (TInt)(!CompareClampsByIdAndCount(aClampA, aClampB));
	}

void CMountBody::SetProxyDriveDismounted()
	{
	iProxyDriveDismounted = ETrue;
	}

TBool CMountBody::ProxyDriveDismounted()
	{
	return iProxyDriveDismounted;
	}


TInt CMountBody::GetFileUniqueId(const TDesC& /*aName*/, TInt64& /*aUniqueId*/)
	{
	return KErrNotSupported;
	}
TInt CMountBody::Spare3(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}
TInt CMountBody::Spare2(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}
TInt CMountBody::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}
void CMountBody::ReadSection64L(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
	{
	if((TUint64)aPos > KMaxLegacyFileSize)
		User::Leave(KErrNotSupported);

	iMountCB->ReadSectionL(aName, I64LOW(aPos), aTrg, aLength, aMessage);
	}

TBool CFileBody::ExtendedFileInterfaceSupported()
	{
	return (iExtendedFileInterface==this) ? (TBool)EFalse : (TBool)ETrue;
	}

// default implementations of MExtendedFileInterface
void CFileBody::ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{
	if ((TUint64)aPos > KMaxLegacyFileSize || aOffset > 0)
		User::Leave(KErrNotSupported);

	iFileCB->ReadL((TInt) aPos, aLength, aDes, aMessage);
	}

void CFileBody::WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{
	if ((TUint64)aPos > KMaxLegacyFileSize || aOffset > 0)
		User::Leave(KErrNotSupported);

	iFileCB->WriteL((TInt) aPos, aLength, aDes, aMessage);
	}

void CFileBody::SetSizeL(TInt64 aSize)
	{
	if ((TUint64)aSize > KMaxLegacyFileSize)
		User::Leave(KErrNotSupported);

	iFileCB->SetSizeL((TInt) aSize);
	}

//---------------------------------------------------------------------------------------------------------------------
/**
    This method allows file system to set maximum file size it supports.
    This can be called on instantiation of the CFileCB derived class object by the file system implementation.
    If this method is not called, the iMaxSupportedFileSize will have default value KMaxTUint64

    @param aMaxFileSize maximum file size supported by file system
*/
EXPORT_C void CFileCB::SetMaxSupportedSize(TUint64 aMaxFileSize)
    {
    iBody->iMaxSupportedFileSize = aMaxFileSize;    
    }

/**
    @return maximum supported file size (depends on the file system created it)
*/
TUint64 CFileCB::MaxSupportedSize(void) const
    {
    return iBody->iMaxSupportedFileSize;
    }

//---------------------------------------------------------------------------------------------------------------------

/**
Gets the size of the file.

This is 64-bit variant for CFileCB::Size().
This shall be used by File Systems supporting file size > 4GB - 1 to query the file size 
inplace of CFileCB::Size() or CFileCB::iSize.

@see CFileCB::iSize
@see CFileCB::Size() 

@prototype

@return The size of the file.
*/
EXPORT_C TInt64 CFileCB::Size64() const
	{
	__ASSERT_DEBUG(iBody != NULL, Fault(EFileBodyIsNull));
	const TInt64 size = MAKE_TINT64(iBody->iSizeHigh,iSize);
	return size;
	}

//---------------------------------------------------------------------------------------------------------------------
/**
Sets the size of the file.

This is 64-bit variant for CFileCB::SetSize().
This should be used by File Systems supporting file size > 4GB - 1 to set the file size 
inplace of CFileCB::SetSize() or CFileCB::iSize.

@see CFileCB::iSize
@see CFileCB::SetSize() 

@prototype

@param aSize The size of the file.
@param aDriveLocked The status of the Drive Lock. If it is EFalse,
the file size shall be modified after acquiring the iLock mutex and if it is ETrue, 
the file size shall be modified without aquiring the iLock mutex.  
*/
EXPORT_C void CFileCB::SetSize64(TInt64 aSize, TBool /*aDriveLocked*/)
	{
	// cuurently this should only be called from the drive thread
	ASSERT(FsThreadManager::IsDriveThread(Drive().DriveNumber(),EFalse));
	iSize = (TInt)I64LOW(aSize);
	iBody->iSizeHigh = (TInt)I64HIGH(aSize);
	}


/** used to organize key comparison for the TFileShareLock*/
TInt LockOrder(const TFileShareLock& aMatch, const TFileShareLock& anEntry)
	{

	if(aMatch.PosLow() > anEntry.PosLow())
		return 1;
	else if(aMatch.PosLow() < anEntry.PosLow())
		return -1;
	else
		return 0;
   
    }

//---------------------------------------------------------------------------------------------------------------------
/**
    Find a lock inclusive of aPosLow to aPosHigh.
*/
TInt CFileCB::FindLock64(TInt64 aPosLow, TInt64 aPosHigh)
    {

	const TInt count=FileLocks().Count();
	for (TInt i=0; i<count; i++)
		{
            
            const TFileShareLock& lock=FileLocks()[i];

            if(lock.PosLow() > (TUint64)aPosHigh)
                return KErrNotFound;

            if(lock.MatchByPos(aPosLow, aPosHigh))
                return i;
		}

	return KErrNotFound;
	}

//---------------------------------------------------------------------------------------------------------------------
/**
    Add a lock on a range.
*/
TInt CFileCB::AddLock64(CFileShare* aFileShare,TInt64 aPos,TInt64 aLength)
	{
	const TUint64 posHigh=aPos+aLength-1;
	
        
        {//-- Lock overflow check
        const TUint64 KMaxFileSize = aFileShare->IsFileModeBig() ? MaxSupportedSize() : KMaxLegacyFileSize;
	    if(posHigh > KMaxFileSize)
                return KErrArgument;
        }
    
    
    TInt r=CheckLock64(NULL, aPos, aLength);
	if (r!=KErrNone)
		return r;
	
    TFileShareLock lock(aFileShare, aPos, posHigh);

    TLinearOrder<TFileShareLock> lockOrder(LockOrder);
	r=FileLocks().InsertInOrder(lock, lockOrder);
	__ASSERT_ALWAYS(r!=KErrAlreadyExists,Fault(EFileDuplicateLock));
	
    return r;
	}

//---------------------------------------------------------------------------------------------------------------------
/**
    Remove a lock on a range.
*/
TInt CFileCB::RemoveLock64(CFileShare* aFileShare, TInt64 aPos, TInt64 aLength)
	{
	const TUint64 posHigh = aPos+aLength-1;

        {//-- Lock overflow check
        const TUint64 KMaxFileSize = aFileShare->IsFileModeBig() ? MaxSupportedSize() : KMaxLegacyFileSize;
	    if(posHigh > KMaxFileSize)
                return KErrArgument;
        }
    
	const TInt pos=FindLock64(aPos, posHigh);
	if (pos==KErrNotFound)
		return KErrNotFound;
	
    const TFileShareLock& lock=FileLocks()[pos];
    if (!lock.MatchOwner(aFileShare) || lock.PosLow() != (TUint64)aPos || lock.PosHigh() != posHigh)
		return KErrNotFound;
	
   
    FileLocks().Remove(pos);

	return KErrNone;
	}

//---------------------------------------------------------------------------------------------------------------------
/** 
    Check if a range is available.
    @param  aFileShare pointer to FileShare object. NULL only when is called from CFileCB::AddLock64()

*/
TInt CFileCB::CheckLock64(CFileShare* aFileShare,TInt64 aPos,TInt64 aLength)
	{
	const TUint64 posHigh=aPos+aLength-1;
	
    //-- Lock overflow check. It is OK to have a lock that is beyond the real file size.
    //-- if aFileShare == NULL, this is the call from AddLock64 and the position is already checked.
    if(aFileShare)
        {
        const TUint64 KMaxFileSize = aFileShare->IsFileModeBig() ? MaxSupportedSize() : KMaxLegacyFileSize;
	    if(posHigh > KMaxFileSize)
            return KErrNone; //-- OK, there can't be any locks beyond the max. supported file length
        }


	TInt lockIdx=FindLock64(aPos, posHigh);
	if (lockIdx == KErrNotFound)
		return KErrNone;

    const TInt count=FileLocks().Count();
	const TFileShareLock* lock=(&FileLocks()[lockIdx]);

	for(;;)
		{
		if (!lock->MatchOwner(aFileShare))
			return KErrLocked;

		if (lock->PosHigh() >= posHigh)
			break;
		
        lockIdx++;
		if (lockIdx >= count)
			break;

		lock=&FileLocks()[lockIdx];
		
        if (posHigh < lock->PosLow())
			break;
		}

    return KErrNone;
	}


//---------------------------------------------------------------------------------------------------------------------
/**
Gets the 'Sequential' mode of the file.

@return	ETrue, if the file is in 'Sequential' mode
*/
EXPORT_C TBool CFileCB::IsSequentialMode() const
	{
	return iBody->iSequential;
	}

/**
Sets the 'Sequential' mode of the file.
 */
void CFileCB::SetSequentialMode(TBool aSequential)
	{
	iBody->iSequential = aSequential;
	}

//---------------------------------------------------------------------------------------------------------------------
/**
Gets the list containing the shares associated with the file.

@return	The FileShare List
*/
TDblQue<CFileShare>& CFileCB::FileShareList() const
	{
	return iBody->iShareList;
	}

/**
Adds the share to the end of the FileShare List.
*/
void CFileCB::AddShare(CFileShare& aFileShare)
	{
	iBody->iShareList.AddLast(aFileShare);
	}


//#####################################################################################################################
//#  TFileShareLock class implementation
//#####################################################################################################################

TFileShareLock::TFileShareLock(const CFileShare* aOwner, TUint64 aPosLow, TUint64 aPosHigh) 
               : iOwner(aOwner), iPosLow(aPosLow), iPosHigh(aPosHigh) 
    {
    }


   
TUint64 TFileShareLock::PosLow()  const 
    {
    return iPosLow;
    }


TUint64 TFileShareLock::PosHigh() const 
    {
    return iPosHigh;
    }

TBool TFileShareLock::MatchOwner(const CFileShare* aShare) const 
    {
    return (aShare == iOwner);
    }

/**
    @return ETrue if aPosLow and PosHigh match the lock boundaries
*/
TBool TFileShareLock::MatchByPos(TUint64 aPosLow, TUint64 aPosHigh) const
    {
        if(PosLow() > aPosHigh)
            return EFalse;

		if ((aPosLow  >= PosLow() && aPosLow   <= PosHigh()) ||
			(aPosHigh >= PosLow() && aPosHigh  <= PosHigh()) ||
			(aPosLow  <= PosLow() && aPosHigh  >= PosHigh() ))
			{
			return ETrue;
			}

         return EFalse;
    }




EXPORT_C TBool CFileCB::DirectIOMode(const RMessagePtr2& aMessage)
	{
	CFsMessageRequest* msgRequest = CFsMessageRequest::RequestFromMessage(aMessage);

	TInt func = msgRequest->Operation()->Function();
	ASSERT(func == EFsFileRead || func == EFsFileWrite || func == EFsFileWriteDirty || func == EFsReadFileSection);

	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch(msgRequest, share, file);
	if (share == NULL)		// no share indicates this is a request originating from the file cache
		return EFalse;

	return func == EFsFileRead ? share->iMode & EFileReadDirectIO : share->iMode & EFileWriteDirectIO; 
	}







