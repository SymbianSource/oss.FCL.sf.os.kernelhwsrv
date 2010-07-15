// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "e32cmn.h"

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
#include "sf_notifier.h"
#endif

GLREF_C CProxyDriveFactory* GetExtension(const TDesC& aName);
GLREF_C CExtProxyDriveFactory* GetProxyDriveFactory(const TDesC& aName);

TBusLocalDrive LocalDrives::iLocalDrives[KMaxLocalDrives];			
TInt LocalDrives::iMapping[KMaxDrives];
TInt LocalDrives::iReverseMapping[KMaxLocalDrives];
TBool LocalDrives::iMappingSet;
LocalDrives::TSocketDesc LocalDrives::iSocketDescs[KMaxPBusSockets];
CExtProxyDrive*  LocalDrives::iProxyDriveMapping[KMaxProxyDrives];
TBool LocalDrives::iIsMultiSlotDrive[KMaxDrives];
const TInt KInvalidSocketNumber = -1;

void LocalDrives::Initialise()
//
//
//
	{	
	iMappingSet = EFalse;
	TInt i;
	Mem::FillZ((TAny*)iProxyDriveMapping,sizeof(CExtProxyDriveFactory*)*KMaxProxyDrives);
	// initialise mapping from drive number to local drive
	for(i=0;i<KMaxDrives;i++)
		{
		iMapping[i] = KDriveInvalid;
		iIsMultiSlotDrive[i] = EFalse;
		}
	// initialise reverse mapping from local drive to drive.
	for(i=0;i<KMaxLocalDrives;i++)
		{
		iReverseMapping[i] = KDriveInvalid;
		}
	// initialise mapping from socket number to drive numbers
	for(i=0;i<KMaxPBusSockets;++i)
		{
		TSocketDesc& socketDesc = iSocketDescs[i];
		socketDesc.iMediaType = EInvalidMedia;
		socketDesc.iControllerRelativeSocket = KInvalidSocketNumber;
		for(TInt j=0;j<KMaxDrivesPerSocket;++j)
			socketDesc.iDriveNumbers[j]=KDriveInvalid;
		}
	}

// Searches for a local socket which matches the media type and 
// controller relative socket number. 
// If none is found then this function returns a new socket number.
// If no more free sockets available, returns KErrNoMemory
TInt LocalDrives::GetLocalSocket(TInt aControllerRelativeSocket, TMediaDevice aMediaType)
	{
	TInt i;
	TSocketDesc* socketDesc = NULL;
	for (i=0; i<KMaxPBusSockets; i++)
		{
		socketDesc = &iSocketDescs[i];
		TMediaDevice mediaType = socketDesc->iMediaType;
		if (mediaType == aMediaType && socketDesc->iControllerRelativeSocket == aControllerRelativeSocket)
			return i;
		if (mediaType == EInvalidMedia)	// socket unassigned ?
			break;
		}
	if (i == KMaxPBusSockets)
		return KErrNoMemory;
	
	// assign a new local socket for this controller relative socket number & media type
	socketDesc->iMediaType = aMediaType;
	socketDesc->iControllerRelativeSocket = aControllerRelativeSocket;

	return i;
	}

TBusLocalDrive& LocalDrives::GetLocalDrive(TInt aDrive)
//
// Export localdrives
//
	{
	__ASSERT_DEBUG(aDrive>=0 && aDrive<KMaxDrives,Fault(EGetLocalDrive1));
	__ASSERT_DEBUG(iMapping[aDrive]!=KDriveInvalid &&  iMapping[aDrive]<KMaxLocalDrives,Fault(EGetLocalDrive2));
	return(iLocalDrives[iMapping[aDrive]]);
	}


TInt LocalDrives::GetLocalDriveNumber(TBusLocalDrive* aLocDrv)
//
// Get the local drive number for the local drive object passed in
//
	{
	for(TInt i=0;i<KMaxLocalDrives;++i)
		if(&iLocalDrives[i]==aLocDrv)
			return(i);
	return(KDriveInvalid);
	}



CExtProxyDrive* LocalDrives::GetProxyDrive(TInt aDrive)
	{
	__ASSERT_DEBUG(aDrive>=0 && aDrive<KMaxDrives,Fault(EGetProxyDriveMapping1));
	__ASSERT_DEBUG(iMapping[aDrive]!=KDriveInvalid &&  iMapping[aDrive]>=KMaxLocalDrives && iMapping[aDrive]<KMaxDrives,Fault(EGetProxyDriveMapping1));
	return iProxyDriveMapping[iMapping[aDrive]-KMaxLocalDrives];
	}


LOCAL_C TBool DriveNumberIsInRange(TInt aDrive)
	{
	
	return((aDrive>=0) && (aDrive<KMaxDrives));
	}
	
	
TBool LocalDrives::IsValidDriveMapping(TInt aDrive)
//
//  Is the drive number to local drive mapping valid?
//
	{
	
	__ASSERT_DEBUG(DriveNumberIsInRange(aDrive),Fault(EIsValidDriveMapping));
	return(iMapping[aDrive]!=KDriveInvalid);
	}


TInt LocalDrives::DriveNumberToLocalDriveNumber(TInt aDrive)
//
// Get the mapping from drive number to local drive
//
	{
	return(iMapping[aDrive]);
	}


TInt LocalDrives::SetDriveMappingL(CFsRequest* aRequest)
//
//
//
	{
	
	__PRINT(_L("LocalDrives::SetDriveMappingL()"));
	if (iMappingSet)
		return(KErrAccessDenied);
		
	TLocalDriveMappingInfoBuf mBuf;
	mBuf.FillZ();
	aRequest->ReadL(KMsgPtr0,mBuf);
	TLocalDriveMappingInfo& ldmi=mBuf();
	
	if (ldmi.iOperation==TLocalDriveMappingInfo::ESwapIntMappingAndSet)
		{
		// Only the 1st two entries of the mapping table are valid - holding the drive numbers to be swapped 
		TInt r=KErrNone;
		if (DriveNumberIsInRange(ldmi.iDriveMapping[0]) && DriveNumberIsInRange(ldmi.iDriveMapping[1]))
			r=SwapDriveMapping(ldmi.iDriveMapping[0],ldmi.iDriveMapping[1]);
		iMappingSet=ETrue;
		return(r);
		}
	
	// That just leaves EWriteMappingsAndSet and EWriteMappingsNoSet
	for (TInt i=0;i<KMaxLocalDrives;++i)
		{
		TInt driveLetter=ldmi.iDriveMapping[i];
		if(driveLetter==KDriveInvalid)
			continue;
		if ( !DriveNumberIsInRange(driveLetter))
			{
			// invalid mapping list passed in, clear all mappings set up
			for(TInt j=0;j<KMaxDrives;j++)
				iMapping[j] = KDriveInvalid;
			return(KErrArgument);
			}
		__PRINT2(_L("drive letter %d -> local drive %d"),driveLetter,i);
		
		// If this mapping (letter -> localdrive) is already set then
		// this must be a multislot device. Save this mapping as an 
		// alternative mapping (by storing it in iReverseMapping)
		if(iMapping[driveLetter] != KDriveInvalid)
			{
			iIsMultiSlotDrive[driveLetter] = ETrue;
			}
		// first time we've seen this drive letter
		iMapping[driveLetter]=i;
		// following mapping is used when we want to swap back again. 
		iReverseMapping[i]=driveLetter;
		}

	InitDriveMapping();
	if (ldmi.iOperation==TLocalDriveMappingInfo::EWriteMappingsAndSet)
		iMappingSet=ETrue;
	return(KErrNone);
	}

// Changes here must be reflected in SwapDriveMapping() 	
void LocalDrives::InitDriveMapping()
	{
	__PRINT(_L("InitDriveMapping()"));
	TDriveInfoV1Buf driveInfo;
	TInt r=UserHal::DriveInfo(driveInfo);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EInitDriveMappingDriveInfo));	

	// initialise the local drives
	TInt i;
	for(i=0;i<KMaxLocalDrives;++i)
		{
		TInt driveNo = iReverseMapping[i];
		if(driveNo!=KDriveInvalid)
			{
			r=iLocalDrives[i].Connect(i,TheDrives[driveNo].iChanged);
			__ASSERT_ALWAYS(r==KErrNone,Fault(EInitConnectLocalDrive));
			__PRINT2(_L("connect to locdrv %d using drive %d"),i,driveNo);
			//If this is a multislot then we need to set the iChanged to True
			//So that we are mapped to the correct drive when we're booted.
			if(iIsMultiSlotDrive[driveNo])
				{
				TheDrives[driveNo].iChanged = ETrue;
				}
			if (driveInfo().iDriveName[i].Length()==0)
				continue;
			TheDriveNames[driveNo]=driveInfo().iDriveName[i].Alloc();
			__ASSERT_ALWAYS(TheDriveNames[driveNo],Fault(EInitCreateDriveName));
			}
		}

	TInt drivesPerSocket[KMaxPBusSockets];
	Mem::FillZ(&drivesPerSocket,KMaxPBusSockets*sizeof(TInt));
	TInt nSockets=driveInfo().iTotalSockets;
	for(i=0;i<KMaxLocalDrives;++i)
		{
		TInt socket;
		if(iLocalDrives[i].Handle()==0 || !iLocalDrives[i].IsRemovable(socket))
			{
			TInt driveNo = iReverseMapping[i];
			// Non-removable drive so shouldn't be listed as a Multislot drive
			// Drives such as composite drives may have been
			// set to true as the drive letter had been encountered before.
			// make sure those drives are set to false here.
			iIsMultiSlotDrive[driveNo]=EFalse;
			continue;
			}
		__ASSERT_ALWAYS(socket>=0 && socket<nSockets,Fault(EInitDriveMappingSocketNo));
		TInt drv=GetDriveFromLocalDrive(i);
		// get local socket number
		TMediaDevice mediaDevice = iLocalDrives[i].MediaDevice();
		TInt localSocket = LocalDrives::GetLocalSocket(socket, mediaDevice);
		__ASSERT_ALWAYS(localSocket>=0 && localSocket<KMaxPBusSockets,Fault(EInitDriveMappingSocketNo));
		__PRINT4(_L("InitDriveMapping(), i = %d, , mediaDevice = %d, socket = %d, localSocket = %d"), 
			i, mediaDevice, socket, localSocket);
		__PRINT2(_L("drv = %d (%C:)"), drv, 'A' + drv);

		TSocketDesc& socketDesc = iSocketDescs[localSocket];
		if(drv!=KDriveInvalid)
			{
			TInt& count = drivesPerSocket[localSocket];
			// setup up socket to drive mapping
			__ASSERT_ALWAYS(count < KMaxDrivesPerSocket,Fault(ETooManyDrivesPerSocket));
			socketDesc.iDriveNumbers[count]=drv;
			if(count==0)
				{
				// setup media change notifier if this is first local drive found on socket
				CNotifyMediaChange* pN=new CNotifyMediaChange(&iLocalDrives[i],localSocket);
				__ASSERT_ALWAYS(pN!=NULL,Fault(EInitCreateMediaChangeNotifier));
				__PRINT2(_L("created CNotifyMediaChange media 0x%x using local drive %d"), localSocket,i);
				socketDesc.iMediaChanges = pN;
				CActiveSchedulerFs::Add(pN);
				pN->RunL();
				}
			++count;
			}
		}
	}

TInt LocalDrives::InitProxyDrive(CFsRequest* aRequest)
	{
	__PRINT(_L("LocalDrives::InitProxyDrive"));
	
	TInt drive = aRequest->Message().Int0() ;
	
	if (drive < 0 || drive >= KMaxDrives) 
		return KErrArgument;
	
	if (drive!=EDriveZ && iMapping[drive]!=KDriveInvalid) 
		return KErrInUse; // Z is special case for composite
	
	TFullName extname;
	aRequest->ReadL(KMsgPtr1,extname);

	// leave info thing for now
	CExtProxyDriveFactory* pF = GetProxyDriveFactory(extname);
	if (!pF) 
		return KErrArgument;	// that extension has not been added
	FsThreadManager::LockDrive(drive);
	// find a free mapping to place this drive into
	TInt i;
	for (i=0; i <KMaxProxyDrives; i++)
		{
		if (!iProxyDriveMapping[i]) 
			break;
		}
	FsThreadManager::UnlockDrive(drive);
	if (i==KMaxProxyDrives) 
		return KErrInUse;   // there are no free proxy drives left

	// Create the actual proxy drive...
	CProxyDrive* pD = NULL;
	TInt r = pF->CreateProxyDrive(pD, NULL);
	if (r != KErrNone)
		{
		delete pD;
		return r;
		}
	if (pD == NULL)
		return KErrNoMemory;

	// Create the proxy drive body... which is used to store the library handle
	CProxyDriveBody* pBody = new CProxyDriveBody();
	if (pBody == NULL)
		{
		delete pD;
		return KErrNoMemory;
		}
	pD->iBody = pBody;

	// Re-open the library so that it is safe to call RFs::RemoveProxyDrive() before the proxy drive has been deleted -
	// which can happen if the file system is dismounted with open file handles
	r = pD->SetAndOpenLibrary(pF->Library());
	if (r != KErrNone)
		{
		delete pD;
		return r;
		}

	iMapping[drive] = i+KMaxLocalDrives;

	aRequest->SetDrive(&TheDrives[drive]);
	aRequest->SetScratchValue((TUint)pD);

	return KErrNone;
	}

TInt LocalDrives::MountProxyDrive(CFsRequest* aRequest)
	{
	CExtProxyDrive* pProxyDrive = (CExtProxyDrive*)aRequest->ScratchValue();
	__ASSERT_ALWAYS(pProxyDrive != NULL, User::Panic(_L("MountProxyDrive has NULL proxy extension class"), -999));

	TInt driveNumber = aRequest->Drive()->DriveNumber();


	TInt proxyDriveNo = iMapping[driveNumber] - KMaxLocalDrives;
	FsThreadManager::LockDrive(driveNumber);
	iProxyDriveMapping[proxyDriveNo] = pProxyDrive;
	pProxyDrive->SetDriveNumber(driveNumber);
	FsThreadManager::UnlockDrive(driveNumber);
	//
	// Pass initialisation information onto the extension to allow it to initialise
	//
	TInt err = pProxyDrive->SetInfo(aRequest->Message(), 
									(TAny*)aRequest->Message().Ptr2(), 
									(TAny*)aRequest->Message().Ptr3());
	if (err != KErrNone) 
		{
		//
		// If we fail to initialise the extension, then close the drive (destroying the thread)
		// and remove the mapping so we can attempt to mount again in the future.
		//
		FsThreadManager::LockDrive(driveNumber);
		FsThreadManager::CloseDrive(driveNumber);
		ClearProxyDriveMapping(driveNumber);
		FsThreadManager::UnlockDrive(driveNumber);
		return err;
		}

	return(iMapping[driveNumber]);
 	}

TBool LocalDrives::IsProxyDrive(TInt aDrive)
	{
	__ASSERT_ALWAYS(aDrive>=0 && aDrive<KMaxDrives,Fault(EIsProxyDrive));
	return (iMapping[aDrive] >= KMaxLocalDrives);
	}

TBool LocalDrives::IsProxyDriveInUse(CExtProxyDriveFactory* aDevice)
	{
	for (TInt i=0; i < KMaxProxyDrives; i++)
		if (iProxyDriveMapping[i] && (iProxyDriveMapping[i]->FactoryP() == aDevice))
			return(ETrue);

	return(EFalse);
	}

void LocalDrives::ClearProxyDriveMapping(TInt aDrive)
	{
	__ASSERT_ALWAYS(aDrive>=0 && aDrive<KMaxDrives,Fault(EClearProxyDriveMapping1));
	__ASSERT_DEBUG(iMapping[aDrive]>= KMaxLocalDrives && iProxyDriveMapping[iMapping[aDrive]-KMaxLocalDrives],Fault(EClearProxyDriveMapping2));
	TInt idx = iMapping[aDrive]-KMaxLocalDrives;
	if (iProxyDriveMapping[idx]->Mount() == NULL)	// don't delete if it's still owned by its mount
		{
		RLibrary lib = iProxyDriveMapping[idx]->GetLibrary();
		delete iProxyDriveMapping[idx];
		lib.Close();
		}
	iProxyDriveMapping[idx] = NULL;
	iMapping[aDrive] = KDriveInvalid;
	}

TInt LocalDrives::SetupMediaChange(TInt aDrive)
	{
	CExtProxyDrive* pProxyDrive = LocalDrives::GetProxyDrive(aDrive);
	__ASSERT_ALWAYS(pProxyDrive != NULL,User::Panic(_L("SetupMediaChange - pProxyDrive == NULL"), ESetupMediaChange));

	return pProxyDrive->SetupMediaChange();
	}

void LocalDrives::NotifyChangeCancel(TInt aDrive)
	{
	CExtProxyDrive* pProxyDrive = LocalDrives::GetProxyDrive(aDrive);
	__ASSERT_ALWAYS(pProxyDrive != NULL,User::Panic(_L("NotifyChangeCancel - pProxyDrive == NULL"), ECancelNotifyChange));

	pProxyDrive->NotifyChangeCancel();
	}

TInt LocalDrives::SwapDriveMapping(TInt aFirstDrive,TInt aSecondDrive)
	{
	
	__PRINT(_L("SwapDriveMapping()"));
	TInt firstLocalDrv=iMapping[aFirstDrive];
	TInt secondLocalDrv=iMapping[aSecondDrive];
	
	// First, check this swap doesn't affect removable drives
	TInt socket;
	if (iLocalDrives[firstLocalDrv].Handle()!=0 && iLocalDrives[firstLocalDrv].IsRemovable(socket))
		return(KErrAccessDenied);
	if (iLocalDrives[secondLocalDrv].Handle()!=0 && iLocalDrives[secondLocalDrv].IsRemovable(socket))
		return(KErrAccessDenied);	
		
	// Now swap the mappings over
	iMapping[aFirstDrive]=secondLocalDrv;	
	iMapping[aSecondDrive]=firstLocalDrv;

	iReverseMapping[firstLocalDrv]=aSecondDrive;
	iReverseMapping[secondLocalDrv]=aFirstDrive;
	
	// Finally, swap the drive names over
	HBufC* drvName=TheDriveNames[aSecondDrive];
	TheDriveNames[aSecondDrive]=TheDriveNames[aFirstDrive];
	TheDriveNames[aFirstDrive]=drvName;
	return(KErrNone);
	}
	
void LocalDrives::CompleteNotifications(TInt aSocket)
//
//
//
	{
	__ASSERT_DEBUG(aSocket>=0 && aSocket<KMaxPBusSockets && iSocketDescs[aSocket].iDriveNumbers[0]!=KDriveInvalid,Fault(ECompleteNotifSocketNo));
	TInt i=0;
	
	// In a data-paging environment, the local media subsytem will only update the TDrive::iChanged flag
	// for drives which have a CNotifyMediaChange object, i.e. for drives which call TBusLocalDrive::NotifyChange().
	// Since we only create ONE CNotifyMediaChange object for each socket (no matter how many partitions/local drives
	// are associated with that socket), we need to propagate the TDrive::iChanged flag to all drives on the socket.
	TBool changedFlag = TheDrives[iSocketDescs[aSocket].iDriveNumbers[0]].IsChanged();

	while(i<KMaxDrivesPerSocket && iSocketDescs[aSocket].iDriveNumbers[i]!=KDriveInvalid)
		{
		TheDrives[iSocketDescs[aSocket].iDriveNumbers[i]].SetChanged(changedFlag);
		CompleteDriveNotifications(iSocketDescs[aSocket].iDriveNumbers[i++]);
		}
	}

void LocalDrives::CompleteDriveNotifications(TInt aDrive)
//
//
//
	{
	// If the drive is hung, then don't complete any disk change 
	// notifications until the request causing the hang completes
	if(FsThreadManager::IsDriveHung(aDrive))
		FsThreadManager::SetMediaChangePending(aDrive);
	else
		{
		FsNotify::DiskChange(aDrive);
		
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
		if(FsNotificationManager::IsInitialised())
			{
			__PRINT3(_L("LocalDrives::CompleteDriveNotifications() Initialised=%d, Count=%d, Drive=%d"),FsNotificationManager::IsInitialised(),FsNotificationManager::Count(), aDrive);
			TBuf<2> driveDes;
			driveDes.Append((TChar)aDrive+(TChar)'A');
			driveDes.Append((TChar)':');
			FsNotificationManager::HandleChange(NULL,driveDes,TFsNotification::EMediaChange);
			}
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION		

	 	//If this is a multislot device we should update mappings here.
		TheDrives[aDrive].MultiSlotDriveCheck();
		}
	}

TInt LocalDrives::GetDriveFromLocalDrive(TInt aLocDrv)
//
//
//
	{
	return iReverseMapping[aLocDrv];
	}


CNotifyMediaChange::CNotifyMediaChange(RLocalDrive* aDrive,TInt aSocketNo)
//
// Constructor
//
	: CActive(EPriorityHigh), iDrive(aDrive), iSocket(aSocketNo)
	{}

void CNotifyMediaChange::RunL()
//
// Notification that a card has been mounted/removed
//
	{
	LocalDrives::CompleteNotifications(iSocket);
	iDrive->NotifyChange(&iStatus);
	SetActive();
	}


CExtNotifyMediaChange::CExtNotifyMediaChange(CExtProxyDrive* aDrive)
//
// Constructor
//
	: CActive(EPriorityHigh), 
	  iDrive(aDrive),
	  iPtr((TUint8*)&TheDrives[aDrive->DriveNumber()].iChanged,sizeof(TBool))
	{
	}

	
CExtNotifyMediaChange* CExtNotifyMediaChange::NewL(CExtProxyDrive* aDrive)
	{
	CExtNotifyMediaChange* pSelf = new(ELeave) CExtNotifyMediaChange(aDrive);

	CleanupStack::PushL(pSelf);
	pSelf->ConstructL();
	CleanupStack::Pop();

	return pSelf;
	}

void CExtNotifyMediaChange::ConstructL()
	{
	CActiveSchedulerFs::Add(this);

	TRAPD(err, RunL());
	if(err != KErrNone)
		Deque();

	User::LeaveIfError(err);
	}

CExtNotifyMediaChange::~CExtNotifyMediaChange()
    {
    Cancel();
    }

void CExtNotifyMediaChange::RequestL()
    {
    if (!IsActive())
        {
        User::LeaveIfError(iDrive->NotifyChange(iPtr, &iStatus));
        SetActive();
        }
    }

void CExtNotifyMediaChange::DoCancel()
    {
    iDrive->NotifyChangeCancel();
    }

void CExtNotifyMediaChange::RunL()
	{
    if(iStatus==KErrDisconnected || iStatus==KErrCancel)
        return;

    TInt driveNum = iDrive->DriveNumber();
    LocalDrives::CompleteDriveNotifications(driveNum);

    /* NOTE: We need SetChanged here though the iChanged variable is set in the MSC, since the cache is not getting cleared
        (inside the CompleteDriveNotifications call) during the initial first notification */
    TheDrives[driveNum].SetChanged(ETrue);

    if(iStatus != KErrNotSupported)
        {
        RequestL();
        }
	}



