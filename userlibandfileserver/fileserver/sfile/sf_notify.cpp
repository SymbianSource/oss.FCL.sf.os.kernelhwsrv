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
// f32\sfile\sf_notif.cpp
// 
//

#include "sf_std.h"



TChangeQue FsNotify::iChangeQues[KMaxNotifyQues];
TDiskSpaceQue FsNotify::iDiskSpaceQues[KMaxDiskQues];
TDebugQue FsNotify::iDebugQue;
TDismountNotifyQue FsNotify::iDismountNotifyQue;

void CNotifyInfo::Initialise(TInfoType aType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	iType=aType;
	iStatus=aStatus;
	iMessage=aMessage;
	iSession=aSession;
	};

CNotifyInfo::~CNotifyInfo()
//
//
//
	{
	__ASSERT_DEBUG(!iLink.iNext,Fault(ENotifyInfoDestructor));
	}

void CNotifyInfo::Complete(TInt aError)
//
//
//
	{
	__PRINT2(_L("CNotifyInfo::Complete 0x%x error=%d"),this,aError);
	if (!iMessage.IsNull())				// Dismount notifiers may be completed but remain in the list
		{											// until handled by the client or the session is closed.
		iMessage.Complete(aError);
		}
	}


void CStdChangeInfo::Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	iChangeType=aChangeType;
	CNotifyInfo::Initialise(EStdChange,aStatus,aMessage,aSession);
	}

TUint CStdChangeInfo::RequestNotifyType(CFsRequest* aRequest)
//
// return notification type for the request
//
	{
	TUint notifyType=aRequest->Operation()->NotifyType();
	if(aRequest->Operation()->Function()==EFsRename)
		{
		__ASSERT_DEBUG(notifyType==(ENotifyDir|ENotifyFile|ENotifyEntry),Fault(EStdChangeRequestType));
		if(aRequest->Src().NamePresent())
			notifyType=ENotifyFile|ENotifyEntry;
		else
			notifyType=ENotifyDir|ENotifyEntry;
		}
	return(notifyType);						
	}

TBool CStdChangeInfo::IsMatching(CFsRequest* aRequest)
//
// return ETrue if operation type of request matches that of change notification
//
	{
	if((iChangeType&ENotifyAll) || (iChangeType&aRequest->Operation()->NotifyType()))
		return(ETrue);
	else
		return(EFalse);
	}

void CExtChangeInfo::Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,const TDesC& aName)
//
//
//
	{
	__ASSERT_DEBUG(aName.Length()<=KMaxFileName-2,Fault(EExtChangeNameLength));
	iName=aName;
	iChangeType=aChangeType;
	CNotifyInfo::Initialise(EExtChange,aStatus,aMessage,aSession);
	}


TBool CExtChangeInfo::IsMatching(CFsRequest* aRequest)
//
// return ETrue if operation notify type of request matches that of change notification
// and paths match
//
	{
	TInt function=aRequest->Operation()->Function();
	//	if a rename occurred inform any requests if their path has been changed regardless of the notification type
	if(function==EFsRename)				
		{		
		TBuf<KMaxFileName> renamePath=aRequest->Src().FullName().Mid(2);		
		renamePath+=_L("*");
		if (iName.MatchF(renamePath)!=KErrNotFound)	
			return(ETrue);
		}


	//Special case where the dir the notifier is setup on has just been created
	if(function==EFsMkDir)	
		{		
		TInt notDrive;
		RFs::CharToDrive(aRequest->Src().Drive()[0],notDrive);	//can not fail as the drive letter has been parsed already
		if(aRequest->Src().Path().MatchF(iName) == 0 && aRequest->DriveNumber() == notDrive)
			return ETrue;
		}
	
	//Special case where  the File the notifier is setup on has just been created by temp as the name is not known unti it has been created
	if(function==EFsRename||function==EFsFileOpen||function==EFsFileCreate||function==EFsFileReplace)
		{
		TInt notDrive;
		RFs::CharToDrive(aRequest->Src().Drive()[0],notDrive);	//can not fail as the drive letter has been parsed already
		if(aRequest->Src().FullName().Mid(2).MatchF(iName) == 0 && aRequest->DriveNumber() == notDrive)
			return ETrue;
		}
	
	//For the case of a file created using EFsFileTemp we can probably ignore it for special cases as it 
	//is created with a random name. Not specifically going to be being looked for

	if((iChangeType&ENotifyAll) || (iChangeType&RequestNotifyType(aRequest)))
		{
		switch (function)
			{	
		//	Notify interested requests if a SetDriveName(), SetVolume() or RawDiskWrite() operation
		//	occcurred.  By their nature, these operations have no distinct path.  All outstanding
		//	requests monitoring the relevant TNotifyType are potentially interested in such operations	
			case EFsFileWrite:
			case EFsFileWriteDirty:
			case EFsFileSet:
			case EFsFileSetAtt:
			case EFsFileSetModified:
			case EFsFileSetSize:
			{
			TBuf<KMaxFileName> root=iName;
			root+=_L("*");	
			
			// NB share may be NULL if file server has initiated a flush of the file cache
			CFileShare* share;
			CFileCB* fileCache;
			GetFileFromScratch(aRequest, share, fileCache);
			if (share && share->File().FileName().MatchF(root) != KErrNotFound)
				return(ETrue);

			}
			break;
			case EFsSetDriveName:
			case EFsSetVolume:
			case EFsRawDiskWrite:
			case EFsLockDrive:
			case EFsUnlockDrive:
			case EFsReserveDriveSpace:
				{
				return(ETrue);				
				}
			
			default:
				{	
				TBuf<KMaxFileName> root = iName;
				root+=_L("*");	
				
				if(aRequest->Src().FullName().Mid(2).MatchF(root)!=KErrNotFound)
					return(ETrue);	
				else if(function==EFsRename||function==EFsReplace||function==EFsFileRename)
					{
					// - rename/replace causes the file/path to disappear
					if(aRequest->Dest().FullName().Mid(2).MatchF(root)!=KErrNotFound)
						{
						return(ETrue);
						}

					// - rename/replace causes the file/path to arrive
					root=aRequest->Dest().FullName().Mid(2);
					root+=_L("*");

					if (iName.MatchF(root)!=KErrNotFound)
						{
						return(ETrue);
						}
					}
				}			
			}
		}
	return(EFalse);
	}


void CDiskSpaceInfo::Initialise(TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,TInt64 aThreshold)
//
//
//
	{
	__ASSERT_DEBUG(aThreshold>0,Fault(EDiskSpaceThreshold));
	iThreshold=aThreshold;
	CNotifyInfo::Initialise(EDiskSpace,aStatus,aMessage,aSession);
	}

TBool CDiskSpaceInfo::IsMatching(TInt64& aBefore,TInt64& aAfter)
//
// return ETrue if the threshold has been crossed
//
	{
	if((aBefore>=iThreshold&&aAfter<iThreshold)||(aBefore<=iThreshold&&aAfter>iThreshold))
		return(ETrue);
	return(EFalse);
	}

void CDebugChangeInfo::Initialise(TUint aDebugType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	__ASSERT_DEBUG((aDebugType&KDebugNotifyMask)&&!(aDebugType&~KDebugNotifyMask),Fault(EDebugChangeType));
	iDebugType=aDebugType;
	CNotifyInfo::Initialise(EDebugChange,aStatus,aMessage,aSession);
	}

TBool CDebugChangeInfo::IsMatching(TUint aFunction)
//
// return ETrue if debug notification type matches aFunction
//
	{
	if(iDebugType&aFunction)
		return(ETrue);
	return(EFalse);
	}


CDismountNotifyInfo::~CDismountNotifyInfo()
	{
	switch(iMode)
		{
		case EFsDismountNotifyClients:
			break;
		case EFsDismountRegisterClient:
			__ASSERT_ALWAYS(TheDrives[iDriveNumber].DismountUnlock() >= 0, Fault(ENotifyDismountCancel));
			break;
		default:
			break;
		}
	}

void CDismountNotifyInfo::Initialise(TNotifyDismountMode aMode, TInt aDriveNumber, TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
	{
	iMode = aMode;
	iDriveNumber=aDriveNumber;
	CNotifyInfo::Initialise(EDismount,aStatus,aMessage,aSession);

	if (iMode == EFsDismountRegisterClient)
		TheDrives[iDriveNumber].DismountLock();
	}

TBool CDismountNotifyInfo::IsMatching(TNotifyDismountMode aMode, TInt aDriveNumber, CSessionFs* aSession)
	{
	if((iDriveNumber == aDriveNumber) && (iMode == aMode) && (aSession == NULL || aSession == Session()))
		return(ETrue);
	return(EFalse);
	}

TBaseQue::TBaseQue()
//
//
//
	{
	iHeader.SetOffset(_FOFF(CNotifyInfo,iLink));
	TInt r=iQLock.CreateLocal();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EBaseQueConstruction));
	}

TBaseQue::~TBaseQue()
//
//
//
	{
	iQLock.Close();
	}

void TBaseQue::DoAddNotify(CNotifyInfo* aInfo)
//
// Add notification
// Que should be locked by calling function
//
	{
	iHeader.AddLast(*aInfo);
	}

TBool TBaseQue::DoCancelSession(CSessionFs* aSession,TInt aCompletionCode, TRequestStatus* aStatus)
//
// Cancel notification(s) setup by aSession matching aStatus if set
// Que should be locked by calling function
//
	{
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	TBool isFound=EFalse;
	while((info=q++)!=NULL)
		{
		if(info->Session()==aSession && (!aStatus || aStatus==info->Status()))
			{
			isFound=ETrue;
			info->Complete(aCompletionCode);
			info->iLink.Deque();
			delete(info);
			if(aStatus)
				break;
			}
		}
	return(isFound);
	}

CNotifyInfo* TBaseQue::DoFindEntry(CSessionFs* aSession, TRequestStatus* aStatus)
	{
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		if(info->Session()==aSession && (!aStatus || aStatus==info->Status()))
			return info;
		}
	return NULL;
	}

void TBaseQue::DoCancelAll(TInt aCompletionCode)
//
// Cancel all notifications
// Que should be locked by calling function
//
	{
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		info->Complete(aCompletionCode);
		info->iLink.Deque();
		delete(info);
		}
	__ASSERT_DEBUG(iHeader.IsEmpty(),Fault(EBaseQueCancel));
	}

TBool TBaseQue::IsEmpty()
//
// Que should be locked by calling function
//
	{
	return iHeader.IsEmpty();
	}

TBool TChangeQue::IsEmpty()
//
//
//
	{
	iQLock.Wait();
	TBool empty = TBaseQue::IsEmpty();
	iQLock.Signal();
	return(empty);
	}
	
TInt TChangeQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

TBool TChangeQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBool isFound=TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(isFound);
	}

void TChangeQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TChangeQue::CheckChange(CFsRequest* aRequest)
//
// complete any notification in que that matches aRequest
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EStdChange||info->Type()==CNotifyInfo::EExtChange,Fault(EChangeQueType));
		TBool isMatching;
		if(info->Type()==CNotifyInfo::EStdChange)
			isMatching=((CStdChangeInfo*)info)->IsMatching(aRequest);
		else
			isMatching=((CExtChangeInfo*)info)->IsMatching(aRequest);
		if(isMatching)
			{
			__PRINT1(_L("TChangeQue::CheckChange()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			info->iLink.Deque();
			delete(info);
			}
		}
	iQLock.Signal();
	}

TBool TDiskSpaceQue::IsEmpty()
//
//
//
	{
	iQLock.Wait();
	TBool empty = TBaseQue::IsEmpty();
	iQLock.Signal();
	return(empty);
	}

TInt TDiskSpaceQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TInt r=KErrNone;
	if(iHeader.IsEmpty())
		{
		r=GetFreeDiskSpace(iFreeDiskSpace);
		iReservedDiskSpace = TheDrives[iDriveNumber].ReservedSpace();
		}
	if(r==KErrNone)
		TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(r);
	}

TInt TDiskSpaceQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(KErrNone);
	}

void TDiskSpaceQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}


void TDiskSpaceQue::CheckDiskSpace()
//
// Complete any disk space notification whose threshold has been crossed
//
	{
	iQLock.Wait();
	if(iHeader.IsEmpty())
		{
		iQLock.Signal();
		return;
		}
	TInt64 freeSpace;
	TInt r=GetFreeDiskSpace(freeSpace);
	TInt64 reservedSpace(TheDrives[iDriveNumber].ReservedSpace());
	if(r==KErrNone)
		{
		if((freeSpace==iFreeDiskSpace) && (reservedSpace==iReservedDiskSpace))
			{
			iQLock.Signal();
			return;
			}
		TDblQueIter<CNotifyInfo> q(iHeader);
		CNotifyInfo* info;
		while((info=q++)!=NULL)
			{
			__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDiskSpace,Fault(EDiskSpaceQueType1));

			TInt64 newSessionFreeSpace(freeSpace);
			TInt64 oldSessionFreeSpace(iFreeDiskSpace);
			if(!info->Session()->ReservedAccess(iDriveNumber))
				{
				newSessionFreeSpace -= reservedSpace;
				oldSessionFreeSpace -= iReservedDiskSpace;
				}

			if(((CDiskSpaceInfo*)info)->IsMatching(oldSessionFreeSpace,newSessionFreeSpace))
				{
				__PRINT1(_L("TDiskSpaceQue::CheckDiskSpace()-Matching info=0x%x"),info);
				info->Complete(KErrNone);
				info->iLink.Deque();
				delete(info);
				}
			}
		iFreeDiskSpace=freeSpace;
		iReservedDiskSpace=reservedSpace;
		}
	else
		TBaseQue::DoCancelAll(KErrNone);
	iQLock.Signal();
	}

void TDiskSpaceQue::CheckDiskSpace(TInt64& aFreeDiskSpace)
//
//
//
	{
	iQLock.Wait();
	if(iHeader.IsEmpty())
		{
		iQLock.Signal();
		return;
		}

	TInt64 reservedSpace(TheDrives[iDriveNumber].ReservedSpace());

	if((aFreeDiskSpace==iFreeDiskSpace) && (reservedSpace==iReservedDiskSpace))
		{
		iQLock.Signal();
		return;
		}
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDiskSpace,Fault(EDiskSpaceQueType2));

		TInt64 newSessionFreeSpace(aFreeDiskSpace);
		TInt64 oldSessionFreeSpace(iFreeDiskSpace);
		if(!info->Session()->ReservedAccess(iDriveNumber))
			{
			newSessionFreeSpace -= reservedSpace;
			oldSessionFreeSpace -= iReservedDiskSpace;
			}

		if(((CDiskSpaceInfo*)info)->IsMatching(oldSessionFreeSpace,newSessionFreeSpace))
			{
			__PRINT1(_L("TDiskSpaceQue::CheckDiskSpace()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			info->iLink.Deque();
			delete(info);
			}
		}
	iFreeDiskSpace=aFreeDiskSpace;
	iReservedDiskSpace=reservedSpace;
	iQLock.Signal();
	}

TInt TDiskSpaceQue::GetFreeDiskSpace(TInt64& aFreeDiskSpace)
//
// 
//
	{
	__ASSERT_DEBUG(iDriveNumber>=EDriveA&&iDriveNumber<=EDriveZ,Fault(EDiskSpaceQueDrive));
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=TheDrives[iDriveNumber].FreeDiskSpace(aFreeDiskSpace);
	return(r);
	}

TInt TDebugQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

TInt TDebugQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBool isFound=TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(isFound);
	}

void TDebugQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TDebugQue::CheckDebug(TUint aDebugChange)
//
// Complete any debug notification whose debug type matches aDebugChange
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDebugChange,Fault(EDebugQueType));
		if(((CDebugChangeInfo*)info)->IsMatching(aDebugChange))
			{
			__PRINT1(_L("TDebugQue::CheckDebug()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			info->iLink.Deque();
			delete(info);
			}
		}
	iQLock.Signal();
	}

TInt TDismountNotifyQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

TInt TDismountNotifyQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
// Returns the drive number or KErrNotFound
//
	{
	iQLock.Wait();

	// return the drive number
	CDismountNotifyInfo* info = (CDismountNotifyInfo*) DoFindEntry(aSession, aStatus);
	TInt driveNumber = info ? info->DriveNumber() : KErrNotFound;

	TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);

	iQLock.Signal();

	return(driveNumber);
	}

void TDismountNotifyQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TDismountNotifyQue::CheckDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt aError)
//
// Complete any dismount notifications on the specified drive.
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
		if(((CDismountNotifyInfo*)info)->IsMatching(aMode, aDrive, NULL))
			{
			__PRINT1(_L("TDismountNotifyQue::CheckDismount()-Matching info=0x%x"),info);
			info->Complete(aError);
			if(aRemove)
				{
				info->iLink.Deque();
				delete(info);
				}
			}
		}

	__ASSERT_ALWAYS(!aRemove || TheDrives[aDrive].DismountLocked() == 0, Fault(EDismountLocked));

	iQLock.Signal();
	}

TBool TDismountNotifyQue::HandlePendingDismount(CSessionFs* aSession, TInt aDrive)
//
// Determine if the session has any outstanding dismount notifications on the specified drive.
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
		if(((CDismountNotifyInfo*)info)->IsMatching(EFsDismountRegisterClient, aDrive, aSession))
			{
			__PRINT1(_L("TDismountNotifyQue::CheckDismount()-Pending info=0x%x"),info);
			info->iLink.Deque();
			delete(info);
			iQLock.Signal();
			return ETrue;
			}
		}
	iQLock.Signal();
	return EFalse;
	}

void FsNotify::Initialise()
//
//
//
	{
	for(TInt i=0;i<KMaxDiskQues;++i)
		{
		iDiskSpaceQues[i].SetDriveNumber(i);
		}
	}

TBool FsNotify::IsChangeQueEmpty(TInt aDrive)
//
//
//
	{
	if((iChangeQues[ChangeIndex(aDrive)].IsEmpty()) && (iChangeQues[ChangeIndex(KDriveInvalid)].IsEmpty()))
		return ETrue;

	return EFalse;
	}

TInt FsNotify::AddChange(CNotifyInfo* aInfo,TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG(aInfo->Type()==CNotifyInfo::EStdChange||aInfo->Type()==CNotifyInfo::EExtChange,Fault(EBadChangeNotifyType));
	__PRINT2(_L("FsNotify::AddChange() drive=%d,info=0x%x"),aDrive,aInfo);
	iChangeQues[ChangeIndex(aDrive)].AddNotify(aInfo);
	return(KErrNone);
	}

TBool FsNotify::IsDiskSpaceQueEmpty(TInt aDrive)
//
//
//
	{
	if(iDiskSpaceQues[aDrive].IsEmpty())
		return ETrue;

	return EFalse;
	}

TInt FsNotify::AddDiskSpace(CNotifyInfo* aInfo,TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG(aInfo->Type()==CNotifyInfo::EDiskSpace,Fault(EBadDiskNotifyType));
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskBadIndex1));
	__PRINT2(_L("FsNotify::AddDiskSpace() drive=%d,info=0x%x"),aDrive,aInfo);
	return(iDiskSpaceQues[aDrive].AddNotify(aInfo));
	}

TInt FsNotify::AddDebug(CNotifyInfo* aDebugInfo)
//
//
//
	{
	__ASSERT_DEBUG(aDebugInfo->Type()==CNotifyInfo::EDebugChange,Fault(EBadDebugNotifyType));
	__PRINT1(_L("FsNotify::AddDebug() info=0x%x"),aDebugInfo);
	iDebugQue.AddNotify(aDebugInfo);
	return(KErrNone);
	}

TInt FsNotify::AddDismountNotify(CNotifyInfo* aDismountNotifyInfo)
//
//
//
	{
	__ASSERT_DEBUG(aDismountNotifyInfo->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
	__PRINT1(_L("FsNotify::AddDismountNotify() info=0x%x"),aDismountNotifyInfo);
	iDismountNotifyQue.AddNotify(aDismountNotifyInfo);
	return(KErrNone);
	}

void FsNotify::HandleChange(CFsRequest* aRequest,TInt aDrive)
//
// Check whether any change notifications need to be completed due to aRequest on aDrive
//
	{
	__PRINT2(_L("FsNotify::HandleChange() aRequest=0x%x, aDrive=%d"),aRequest,aDrive);
	if(!aRequest->IsChangeNotify())
		return;
	iChangeQues[ChangeIndex(aDrive)].CheckChange(aRequest);
	iChangeQues[ChangeIndex(KDriveInvalid)].CheckChange(aRequest);
	}
	

void FsNotify::HandleDiskSpace(CFsRequest* aRequest,TInt aDrive)
//
// Check whether any disk space notifications need to be completed due to aRequest on aDrive
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ) || aDrive==KDriveInvalid,Fault(EDiskBadIndex2));
	__PRINT2(_L("FsNotify::HandleDiskSpace() aRequest=0x%x, aDrive=%d"),aRequest,aDrive);
	
	if(!aRequest->Operation()->IsDiskSpaceNotify())
		return;
	TInt f = aRequest->Operation()->Function();
	if ((f == EFsFileWrite || f == EFsFileWriteDirty) && !((CFsClientMessageRequest*)aRequest)->IsFreeChanged())
		return;
	if (FsThreadManager::IsDriveThread(aDrive,EFalse))	
		iDiskSpaceQues[aDrive].CheckDiskSpace();
	}

void FsNotify::HandleDiskSpace(TInt aDrive, TInt64& aFreeSpace)
//
//
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskBadIndex3));
	__PRINT1(_L("FsNotify::HandleDiskSpace() aDrive=%d"),aDrive);
	iDiskSpaceQues[aDrive].CheckDiskSpace(aFreeSpace);
	}

void FsNotify::HandleDebug(TUint aFunction)
//
// Check whether any debug notifications need to be completed due to aFunction
//
	{
	__PRINT1(_L("FsNotify::HandleDebug() aFunction=0x%x"),aFunction);
	if(!(aFunction&KDebugNotifyMask))
		return;
	iDebugQue.CheckDebug(aFunction);
	}

void FsNotify::HandleDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt err)
//
// Handle dismount notifications for the given drive
//
	{
	__PRINT4(_L("FsNotify::HandleDismount() aMode = %d, aDrive=%d, aRemove=%d, err=%d"),aMode,aDrive,aRemove,err);
	iDismountNotifyQue.CheckDismount(aMode, aDrive, aRemove, err);
	}

TBool FsNotify::HandlePendingDismount(CSessionFs* aSession, TInt aDrive)
//
// Checks if the session has an outstanding notification registered on the drive
//
	{
	__PRINT1(_L("FsNotify::HandlePendingDismount() aDrive=%d"),aDrive);
	return iDismountNotifyQue.HandlePendingDismount(aSession, aDrive);
	}

void FsNotify::DiskChange(TInt aDrive)
//
// Complete all notifications in queus due to a disk change
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskChangeDrive));
	__PRINT1(_L("FsNotify::DiskChange() aDrive=%d"),aDrive);
	iChangeQues[ChangeIndex(aDrive)].CancelAll(KErrNone);
	iChangeQues[ChangeIndex(KDriveInvalid)].CancelAll(KErrNone);
	iDiskSpaceQues[aDrive].CancelAll(KErrNone);
	iDebugQue.CancelAll(KErrNone);

	// if there are any files containing dirty data, start issuing write-dirty data requests to trigger
	// a critical notifier (CFileCache::HandleWriteDirtyError())
	// otherwise purge all file caches
	TDrive& drive=TheDrives[aDrive];
	drive.FlushCachedFileInfo(ETrue);	
	}

	
void FsNotify::CancelChangeSession(CSessionFs* aSession,TRequestStatus* aStatus)
//
//	Cancel change notifcation(s) setup by aSession and matching aStatus if not NULL
//
	{
	__PRINT2(_L("FsNotify::CancelChangeSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	for(TInt i=0;i<KMaxNotifyQues;++i)
		{
		TBool isFound=iChangeQues[i].CancelSession(aSession,KErrCancel,aStatus);
		if(aStatus && isFound)
			break;
		}
	}

void FsNotify::CancelDiskSpaceSession(CSessionFs* aSession,TRequestStatus* aStatus)
//
// Cancel disk space notification(s) setup by aSession and matching aStatus if not NULL
//

	{
	__PRINT2(_L("FsNotify::CancelDiskSpaceSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	for(TInt i=0;i<KMaxDiskQues;++i)
		{
		TBool isFound=iDiskSpaceQues[i].CancelSession(aSession,KErrCancel,aStatus);
		if(aStatus && isFound)
			break;
		}
	}

void FsNotify::CancelDebugSession(CSessionFs* aSession, TRequestStatus* aStatus)
//
// Cancel debug notification(s) setup by aSession and matching aStatus if not NULL
//
	{
	__PRINT2(_L("FsNotify::CancelDebugSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	iDebugQue.CancelSession(aSession,KErrCancel,aStatus);
	}

TInt FsNotify::CancelDismountNotifySession(CSessionFs* aSession, TRequestStatus* aStatus)
//
// Cancel all media removal notification(s) setup by aSession (if aStatus == NULL)
// else cancels all outstanding notifications(s) for the session
//
	{
	__PRINT2(_L("FsNotify::CancelDismountNotifySession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	TInt drive = iDismountNotifyQue.CancelSession(aSession,KErrCancel,aStatus);
	return drive;
	}

void FsNotify::CancelSession(CSessionFs* aSession)
//
//
//
	{
	__PRINT(_L("FsNotify::CancelSession"));
	FsNotify::CancelChangeSession(aSession);
	FsNotify::CancelDiskSpaceSession(aSession);
	FsNotify::CancelDebugSession(aSession);
	FsNotify::CancelDismountNotifySession(aSession);
	}


TInt FsNotify::ChangeIndex(TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ) || aDrive==KDriveInvalid,Fault(EChangeBadIndex));
	if(aDrive==KDriveInvalid)
		return(0);
	else
		return(aDrive+1);
	}

