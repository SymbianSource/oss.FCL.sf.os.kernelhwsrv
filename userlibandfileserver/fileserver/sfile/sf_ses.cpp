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
// f32\sfile\sf_ses.cpp
// 
//

#include "sf_std.h"
#include "sf_file_cache.h"
#include "sf_memory_man.h"
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
#include "sf_notifier.h"
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	

CSessionFs::CSessionFs()
	       :iSessionFlags((TInt)EFsSessionFlagsAll), 
          iReservedDriveAccess(KReservedDriveAccessArrayGranularity, _FOFF(TReservedDriveAccess, iDriveNumber)),
	       iId(0)
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
    __e32_atomic_add_ord32(&SessionCount, 1);
#endif
	}

CSessionFs *CSessionFs::NewL()
	{
	return new(ELeave) CSessionFs;
	}

CSessionFs::~CSessionFs()
	{
	__PRINT1(_L("CSessionFs::~CSessionFs() deleting... = 0x%x"),this);
		
	//take out all the reserved space set by this session
	while(iReservedDriveAccess.Count())
		{
		TReservedDriveAccess& reserved = iReservedDriveAccess[0];
		if(reserved.iReservedSpace)
			{
			TheDrives[reserved.iDriveNumber].SetReservedSpace(TheDrives[reserved.iDriveNumber].ReservedSpace() - reserved.iReservedSpace);
			__ASSERT_DEBUG(TheDrives[reserved.iDriveNumber].ReservedSpace() >= 0,Fault(EReserveSpaceArithmetic));
			}
		iReservedDriveAccess.Remove(0);
		}
	iReservedDriveAccess.Close();
	
	
	//	Need to free the requests that we own from the close queue
	while (iCloseRequestCount > 0)
		RequestAllocator::OpenSubFailed(this);
	
	if (iHandles)
		delete iHandles;

	
	delete iPath;
	iSessionFlagsLock.Close();
	if(iDisconnectRequest)
		delete(iDisconnectRequest);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
    __e32_atomic_add_ord32(&SessionCount, (TUint32) -1);
#endif
	}

void CSessionFs::CreateL()
//
// Create any additional resources.
//
	{
	__PRINT1(_L("CSessionFs::CreateL 0x%x"),this);

	iHandles=CFsObjectIx::NewL();
	TInt r = iSessionFlagsLock.CreateLocal();
	User::LeaveIfError(r);
	RMessage2 m;

	iDisconnectRequest=new(ELeave) CFsDisconnectRequest;
	iDisconnectRequest->Set(m,SessionDisconnectOp,this);


	}

TInt CSessionFs::CurrentDrive()
//
// Return the current drive.
//
	{

	TInt d;
	TInt r=RFs::CharToDrive(Path()[0],d);
	__ASSERT_ALWAYS(r==KErrNone,Fault(ESesPathBadDrive));
	return(d);
	}

TInt CSessionFs::CountResources()
//
// Return the number of resources owned by the session
//
	{
	/* what's this supposed to be doing ??
	TInt count=User::LockedInc(iResourceCount);
	User::LockedDec(iResourceCount);
	return(count);
	*/

	return iResourceCount;	// ... because that's all it does.
	}

void CSessionFs::ResourceCountMarkStart()
//
//	Mark resources at this point
//
	{
	iResourceCountMark = CountResources();
	}

void CSessionFs::ResourceCountMarkEnd(const RMessage2& aMessage)
//
//	End resource count and check same as count start
//
	{
	if (iResourceCountMark!=CountResources())
		{
		_LIT(KCategory,"CSessionFs");
		aMessage.Panic(KCategory,ESesFoundResCountHeaven);
		}
	else
		aMessage.Complete(KErrNone);
	}

void CSessionFs::Disconnect(const RMessage2& aMessage)
	{
	__THRD_PRINT1(_L("CSessionFs::Disconnect() 0x%x"),this);

	iHandles->CloseMainThreadObjects();
	iDisconnectRequest->SetMessage((RMessage2&)aMessage);

	iDisconnectRequest->Dispatch();
	}


TUint CSessionFs::Reserved(TInt aDriveNumber) const
	{
	TReservedDriveAccess reserved(aDriveNumber);
    TInt idx = iReservedDriveAccess.Find(reserved);
    if(idx == KErrNotFound)
        return 0;

    return iReservedDriveAccess[idx].iReservedSpace;
	}


TInt CSessionFs::SetReserved(const TInt aDriveNumber, const TInt aReservedValue)
	{
	TReservedDriveAccess reserved(aDriveNumber, aReservedValue);
	TInt idx = iReservedDriveAccess.Find(reserved);
	if(idx == KErrNotFound)
		return iReservedDriveAccess.InsertInSignedKeyOrder(reserved);

	iReservedDriveAccess[idx].iReservedSpace = aReservedValue;
	return KErrNone;

	}


TBool CSessionFs::ReservedAccess(TInt aDriveNumber) const
	{
	TReservedDriveAccess reserved(aDriveNumber);
	TInt idx = iReservedDriveAccess.Find(reserved);
	return idx == KErrNotFound ? EFalse : iReservedDriveAccess[idx].iReservedAccess;
	}


void CSessionFs::SetReservedAccess(const TInt aDriveNumber, const TBool aReservedAccess)
	{
	TReservedDriveAccess reserved(aDriveNumber);
    TInt idx = iReservedDriveAccess.Find(reserved);
    if(idx != KErrNotFound)
        iReservedDriveAccess[idx].iReservedAccess = aReservedAccess;
	}

TBool CSessionFs::IsChangeNotify()
	{
	return TestSessionFlags(EFsSessionNotifyChange);
	}


TBool CSessionFs::TestSessionFlags(TUint32 aFlags)
	{
	iSessionFlagsLock.Wait();
	TBool b = (iSessionFlags & aFlags) == aFlags;
	iSessionFlagsLock.Signal();
	return(b);
	}


void CSessionFs::SetSessionFlags(TUint32 aBitsToSet, TUint32 aBitsToClear)
	{
	iSessionFlagsLock.Wait();

	iSessionFlags &= ~aBitsToClear;
	iSessionFlags |= aBitsToSet;
	
	iSessionFlagsLock.Signal();
	}

void CSessionFs::CloseRequestCountInc()
	{
	iSessionFlagsLock.Wait();
	iCloseRequestCount++;	
	iSessionFlagsLock.Signal();
	}

void CSessionFs::CloseRequestCountDec()
	{
	iSessionFlagsLock.Wait();
	iCloseRequestCount--;
	iSessionFlagsLock.Signal();
	}

//
// Start resource count
//
TInt TFsResourceCountMarkStart::DoRequestL(CFsRequest* aRequest)
	{
	aRequest->Session()->ResourceCountMarkStart();
	return(KErrNone);
	}

TInt TFsResourceCountMarkStart::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}


//
//  Check for resource heaven
//
TInt TFsResourceCountMarkEnd::DoRequestL(CFsRequest* aRequest)
	{
	aRequest->Session()->ResourceCountMarkEnd(aRequest->Message());
	return(KErrNone);
	}

TInt TFsResourceCountMarkEnd::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}


//
// Return the number of resources owned by the session
//
TInt TFsResourceCount::DoRequestL(CFsRequest* aRequest)
	{
	TInt resCount=aRequest->Session()->CountResources();
	TPckgC<TInt> pckg(resCount);
	aRequest->WriteL(KMsgPtr0,pckg);
	return(KErrNone);
	}

TInt TFsResourceCount::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}


//
// Set iNotifyUser
//
void CSessionFs::SetNotifyUser(TBool aNotification)
	{
	if(aNotification)
		{
		SetSessionFlags(EFsSessionNotifyUser, 0);
		}
	else
		{
		SetSessionFlags(0, EFsSessionNotifyUser);
		}
	}

//
// Get iNotifyUser
//
TBool CSessionFs::GetNotifyUser()
	{
	return TestSessionFlags(EFsSessionNotifyUser);
	}

//
// Notify the user of any read or write failure
//
TInt TFsSetNotifyUser::DoRequestL(CFsRequest* aRequest)
	{
	TBool notification=aRequest->Message().Int0();
	aRequest->Session()->SetNotifyUser(notification);
	return(KErrNone);
	}

TInt TFsSetNotifyUser::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}


//
// Notify the user of any read or write failure
//
TInt TFsGetNotifyUser::DoRequestL(CFsRequest* aRequest)
	{
	TBool notification=aRequest->Session()->GetNotifyUser();
	TPtrC8 pA((TUint8*)&notification,sizeof(TBool));
	aRequest->WriteL(KMsgPtr0,pA);
	return(KErrNone);
	}

TInt TFsGetNotifyUser::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}


//
// Get the drive name
//
TInt TFsGetDriveName::DoRequestL(CFsRequest* aRequest)
	{
	TInt driveNum=aRequest->Drive()->DriveNumber();
	TFileName driveName;
	if (TheDriveNames[driveNum]==NULL)
		driveName.SetLength(0);
	else
		driveName=(*TheDriveNames[driveNum]);
	aRequest->WriteL(KMsgPtr1,driveName);
	return(KErrNone);
	}


TInt TFsGetDriveName::Initialise(CFsRequest* aRequest)
	{
	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest);
	return(r);
	}



//
// Set the drive name
//
TInt TFsSetDriveName::DoRequestL(CFsRequest* aRequest)
	{
	TInt driveNum=aRequest->Drive()->DriveNumber();
	TFileName driveName;
	aRequest->ReadL(KMsgPtr1,driveName);

//	Validate name - return KErrBadName if it contains illegal characters such as
//	* ? / | > <

	TNameChecker checker(driveName);
	TText badChar;
	if (checker.IsIllegalName(badChar))
		return(KErrBadName);
	
	TInt len=((driveName.Length()+31)>>5)<<5; // % 32
	if (TheDriveNames[driveNum]==NULL)
		TheDriveNames[driveNum]=HBufC::New(len);
	else if (TheDriveNames[driveNum]->Des().MaxLength()<len)
		{
		HBufC* temp=TheDriveNames[driveNum]->ReAlloc(len);
		if (temp==NULL)
			return(KErrNoMemory);
		TheDriveNames[driveNum]=temp;
		}
	if (TheDriveNames[driveNum]==NULL || TheDriveNames[driveNum]->Des().MaxLength()<len)
		return(KErrNoMemory);
	*TheDriveNames[driveNum]=driveName;
	return(KErrNone);
	}

TInt TFsSetDriveName::Initialise(CFsRequest* aRequest)
	{
	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest);
	if (r!=KErrNone)
		return(r);
	if (!KCapFsSetDriveName.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Set Drive Name")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt FlushCachedFiles(CSessionFs* aSession)
	{
	TInt retVal = KErrNone;

	aSession->Handles().Lock();
	TInt count = aSession->Handles().Count();
	for (TInt n=0; n<count; n++)
		{
		CObjPromotion* obj = (CObjPromotion*)aSession->Handles()[n];
		if (obj != NULL && obj->UniqueID() == FileShares->UniqueID())
			{
			CFileShare* share = (CFileShare*) obj;
			if (!share->IsCorrectThread())
				continue;
			CFileCB& file=((CFileShare*) obj)->File();

			CFileCache* fileCache = file.FileCache();
			if (fileCache)
				{
				retVal = fileCache->FlushDirty();
				if (retVal == CFsRequest::EReqActionBusy)
					break;
				}
			}
		}
	aSession->Handles().Unlock();

	return retVal;
	}

TInt TFsFlushDirtyData::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	__CHECK_DRIVETHREAD(aRequest->DriveNumber());

	// Flush all dirty data
	TInt r = FlushCachedFiles(aRequest->Session());
	if (r == CFsRequest::EReqActionBusy)
		return r;
	return KErrNone;
	}

TInt TFsFlushDirtyData::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return(KErrNone);
	}


// Iterate though all file shares owned by this session and cancel any async requests
TInt CancelAsyncRequests(CSessionFs* aSession)
	{
	TInt retVal = KErrNone;

	aSession->Handles().Lock();
	TInt count = aSession->Handles().Count();
	for (TInt n=0; n<count; n++)
		{
		CObjPromotion* obj = (CObjPromotion*)aSession->Handles()[n];
		if (obj != NULL && obj->UniqueID() == FileShares->UniqueID())
			{
			CFileShare* share = (CFileShare*) obj;
			if (!share->IsCorrectThread())
				continue;
			CFileCB& file=((CFileShare*) obj)->File();
			file.CancelAsyncReadRequest(share, NULL);
			}
		}
	aSession->Handles().Unlock();

	return retVal;
	}



TInt TFsCancelSession::DoRequestL(CFsRequest* aRequest)
	{
	__CHECK_DRIVETHREAD(aRequest->DriveNumber());

	// Cancel any outstanding requests
	CDriveThread* pT=NULL;
	TInt r=FsThreadManager::GetDriveThread(aRequest->DriveNumber(), &pT);
	if(r==KErrNone)
		pT->CompleteSessionRequests(aRequest->Session(),KErrCancel);
	// We must also cancel any ASYNC requests belonging to this session BEFORE 
	// ~CSessionFs() is called to avoid a KERN-EXEC 44 (EBadMessageHandle)
	CancelAsyncRequests(aRequest->Session());
	return(r);
	}

TInt TFsCancelSession::Initialise(CFsRequest* /*aRequest*/)
	{
	return(KErrNone);
	}

TInt TFsSessionDisconnect::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsSessionDisconnect::DoRequestL()"));
	__ASSERT_DEBUG(FsThreadManager::IsDisconnectThread(),Fault(ESessionDisconnectThread1));
	CDisconnectThread* pT=FsThreadManager::GetDisconnectThread();
	
	// Complete requests on all plugins
	CFsInternalRequest* pR=pT->GetRequest();
	FsPluginManager::CompleteSessionRequests(aRequest->Session(), KErrCancel, pR);

	// ...and on all drives
	for(TInt i=0;i<KMaxDrives;++i)
		{
		FsThreadManager::LockDrive(i);
		if(!FsThreadManager::IsDriveAvailable(i,EFalse)||FsThreadManager::IsDriveSync(i,EFalse))
			{
			FsThreadManager::UnlockDrive(i);
			continue;
			}
		pR->Set(CancelSessionOp,aRequest->Session());
		pR->SetDriveNumber(i);
		pR->Status()=KRequestPending;
		pR->Dispatch();
		FsThreadManager::UnlockDrive(i);
		User::WaitForRequest(pR->Status());
		// check request completed or cancelled (by file system dismount which completes requests with KErrNotReady)
		__ASSERT_ALWAYS(pR->Status().Int()==KErrNone||pR->Status().Int()==KErrNotReady,Fault(ESessionDisconnectThread2));
		__THRD_PRINT2(_L("cancel session requests on drive %d r=%d"),i,pR->Status().Int());

		if (TFileCacheSettings::Flags(i) & (EFileCacheWriteEnabled | EFileCacheWriteOn))
			{
			FsThreadManager::LockDrive(i);
			if(!FsThreadManager::IsDriveAvailable(i,EFalse)||FsThreadManager::IsDriveSync(i,EFalse))
				{
				FsThreadManager::UnlockDrive(i);
				continue;
				}

			// Flush dirty data
			pR->Set(FlushDirtyDataOp,aRequest->Session());
			pR->SetDriveNumber(i);
			pR->Status()=KRequestPending;
			pR->Dispatch();
			FsThreadManager::UnlockDrive(i);
			User::WaitForRequest(pR->Status());
			// check request completed or cancelled (by file system dismount which completes requests with KErrNotReady)
			__ASSERT_ALWAYS(pR->Status().Int()==KErrNone||pR->Status().Int()==KErrNotReady,Fault(ESessionDisconnectThread2));
			__THRD_PRINT2(_L("Flush dirty data on drive %d r=%d"),i,pR->Status().Int());
			}

		}
	FsNotify::CancelSession(aRequest->Session());
	
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
	FsNotificationManager::RemoveNotificationRequest(aRequest->Session());
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION


	// don't delete session here, will be done in CFsDisconnectRequest::Complete()
	return(KErrNone);
	}

TInt TFsSessionDisconnect::Initialise(CFsRequest* /*aRequest*/)
	{
	return(KErrNone);
	}

TInt TFsCancelPlugin::DoRequestL(CFsRequest* aRequest)
	{
	//__ASSERT_DEBUG(FsPluginManager::IsPluginThread(),Fault(EFsPluginThreadError));
	FsPluginManager::CancelPlugin(aRequest->iCurrentPlugin,aRequest->Session());
	TInt err = aRequest->iCurrentPlugin->SessionDisconnect(aRequest->Session());
	return(err);
	}

TInt TFsCancelPlugin::Initialise(CFsRequest* /*aRequest*/)
	{
	// Notify plugin of session disconnect
	return(KErrNone);
	}

TInt TFsSetSessionFlags::DoRequestL(CFsRequest* aRequest)
	{
	aRequest->Session()->SetSessionFlags(aRequest->Message().Int0(), aRequest->Message().Int1());
	return(KErrNone);
	}

TInt TFsSetSessionFlags::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsPlugin. CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Set Session Flags")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt TFsInitialisePropertiesFile::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("**TFsInitialisePropertiesFile::DoRequestL**\n"));
	TInt err = KErrNone;
	TInt leaveErr = KErrNone;
	const TBool isRomParam = aRequest->Message().Int2();
	if(isRomParam)
		{
		const TAny* romAddress = aRequest->Message().Ptr0();
		TBool isInRom = EFalse;
		TRAP(leaveErr, User::IsRomAddress(isInRom, const_cast<TAny*>(romAddress)));
		if (leaveErr == KErrNone)
			{
			const TInt length = aRequest->Message().Int1();
			err = isInRom ? F32Properties::Initialise((TInt)romAddress, length) : KErrNotSupported;
			}
		}
	else
		{
		err = KErrNotSupported;
		}

#ifdef SYMBIAN_ENABLE_FAT_DIRECTORY_OPT
	// Create the global cache memory manager for FAT dir cache (and other caches).
	// Note: file cache uses its own cache memory manager.
	if (CCacheMemoryManagerFactory::CacheMemoryManager() == NULL)
		{
		TGlobalCacheMemorySettings::ReadPropertiesFile();
		TRAPD(r, CCacheMemoryManagerFactory::CreateL());
		__ASSERT_ALWAYS(r==KErrNone,Fault(ECacheMemoryManagerCreateFailed));
		}
#endif // SYMBIAN_ENABLE_FAT_DIRECTORY_OPT

	// Create the page cache for file caching etc.
	TGlobalFileCacheSettings::ReadPropertiesFile();
	if (TGlobalFileCacheSettings::Enabled())
		{
		TRAPD(r, CCacheManagerFactory::CreateL());
		__ASSERT_ALWAYS(r==KErrNone,Fault(EFileCacheCreateFailed));
		}

	User::LeaveIfError(leaveErr);
	return(err);
	}

TInt TFsInitialisePropertiesFile::Initialise(CFsRequest* aRequest)
	{
	if (!KCapDiskAdmin. CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Initialise Property File")))
		{
		return(KErrPermissionDenied);
		}
    
	if (aRequest->Message().SecureId() != KEstartUidValue)
		{
        return(KErrPermissionDenied);
		}

	aRequest->SetDriveNumber(EDriveZ);
	return(KErrNone);
	}
