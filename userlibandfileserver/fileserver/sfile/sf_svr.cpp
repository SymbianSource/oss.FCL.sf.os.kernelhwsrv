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
// f32\sfile\sf_svr.cpp
// 
//

#include "sf_std.h"
#include "sf_cache_man.h"
#include "sf_file_cache.h"
#include "sf_memory_man.h"

TInt DoFsSubClose(CFsRequest* aRequest)
//
// Close a subsession.
//
	{
	const TInt handle(aRequest->Message().Int3());

	CFsObject* pO=SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!pO)
		return KErrBadHandle;

	aRequest->Session()->DecResourceCount();
	aRequest->Session()->Handles().Remove(handle,ETrue);

	// this request has also opened this object, so close it here before it 
	// gets to a plugin as a plugin may attempt to open the file/dir itself
	aRequest->SetScratchValue64( MAKE_TUINT64(I64HIGH(aRequest->ScratchValue64()), 0));

	return(KErrNone);
	}


TInt TFsSubClose::DoRequestL(CFsRequest* aRequest)
//
// Close a subsession.
//
	{

	// Leave and then panic client with KErrBadHandle if necessary.
	CFsObject* pO = SessionObjectFromHandle(aRequest->Message().Int3(),0,aRequest->Session());
	if(!pO)
		return KErrBadHandle;

	if(aRequest->Message().Function() == EFsFileSubClose)
		{
		CFileShare* pShare = (CFileShare*) aRequest->ScratchValue();
		
		// flush the file cache
		CFileCache* fileCache = pShare->File().FileCache();
		if (fileCache && fileCache->FlushDirty(aRequest) == CFsRequest::EReqActionBusy)
			return CFsRequest::EReqActionBusy;

		// if any write requests are being fair scheduled, wait for them to complete
		if (pShare->RequestInProgress())
			return CFsRequest::EReqActionBusy;
		}

	return DoFsSubClose(aRequest);
	}

TInt TFsSubClose::Initialise(CFsRequest* aRequest)
//
//	Now moved to RequestAllocator::GetRequest 
//
	{
	TInt r = KErrNone;
	// Closing a file share may require flushing of dirty data, so deal with this in the drive thread
	// in TFsSubClose::DoRequestL(). For other objects (EFsFormatSubClose, EFsDirSubClose, EFsRawSubClose,
	// FsPluginSubClose) we may complete the message here.
	if(aRequest->FsFunction() == EFsFileSubClose)
		{
		CFileShare* pShare = (CFileShare*)
			SessionObjectFromHandle(aRequest->Message().Int3(), FileShares->UniqueID(), aRequest->Session());

		if(!pShare)
			User::Leave(KErrBadHandle);

		HBufC* pFileName = pShare->File().FileName().Alloc();
		aRequest->SetScratchValue64( MAKE_TUINT64((TUint) pFileName, (TUint) pShare) );
		TInt driveNumber = pShare->File().Drive().DriveNumber();
		aRequest->SetDriveNumber(driveNumber);
		}
	
	// Define a completion routine so that we can see whether this request is cancelled - 
	// if it is we still want to close the subsession (but not bother about flushing dirty data)
	r = ((CFsMessageRequest*) aRequest)->PushOperation(TFsSubClose::Complete);
	


	return r;
	}

TInt TFsSubClose::Complete(CFsRequest* aRequest)
	{
	// if LastError() != KErrNone, this implies the request has been cancelled. 
	// i.e. TFsSubClose::DoRequestL() has not been called, so we need to call DoFsSubClose() here
	if (((CFsMessageRequest*) aRequest)->LastError() != KErrNone)
		DoFsSubClose(aRequest);

	return CFsRequest::EReqActionComplete;
	}


TInt TFsNotifyChange::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	CStdChangeInfo* notificationInfo=new CStdChangeInfo;
	if (notificationInfo==NULL)
		return (KErrNoMemory);
	const RMessage2& m=aRequest->Message();
	notificationInfo->Initialise((TNotifyType)m.Int0(),(TRequestStatus*)m.Ptr1(),m,aRequest->Session());
	TInt r=FsNotify::AddChange(notificationInfo,KDriveInvalid);
	if(r!=KErrNone)
		delete(notificationInfo);
	return(r);
	}


TInt TFsNotifyChange::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return(KErrNone);
	}

TInt TFsNotifyChangeEx::DoRequestL(CFsRequest* aRequest)
//
//	Set up an extended change notification
//
	{
	TUint notifyType=aRequest->Message().Int0();
	if (aRequest->Src().NamePresent())	//	Monitoring a file
		{								//	Reject if iNotifyType is ENotifyDir
		if (notifyType&ENotifyDir)
			return (KErrArgument);
		}
	CExtChangeInfo* notificationInfo=new CExtChangeInfo;
	if (notificationInfo==NULL)
		return (KErrNoMemory);
/*	TEntry t;
	TInt ret=aRequest->Drive()->Entry(aRequest->Src().FullName().Mid(2),t);
	if ((ret==KErrNotFound)||(ret==KErrPathNotFound)||(ret==KErrInUse))			
	//	Path does not yet exist or drive has been locked - still submit request
	//	but mark it as notify all since the client is potentially interested
		notifyType=ENotifyAll;	
	else if (ret!=KErrNone)		
		{
		delete notificationInfo;
		return(ret);
		}*/

	const RMessage2& m=aRequest->Message();
	notificationInfo->Initialise((TNotifyType)notifyType,(TRequestStatus*)m.Ptr2(),m,aRequest->Session(),aRequest->Src().FullName().Mid(2));
	TInt drive;
	if (aRequest->ScratchValue()!=0)
		drive=KDriveInvalid;
	else
		drive=aRequest->DriveNumber();
	TInt ret=FsNotify::AddChange(notificationInfo,drive);
	if(ret!=KErrNone)
		delete(notificationInfo);
	return(ret);
	}


TInt TFsNotifyChangeEx::Initialise(CFsRequest* aRequest)
	{
	//	Establish whether the directory or file to watch actually exists
	//	This sets the aSession members iTheName and iTheParse etc
	TInt ret=KErrNone;
	TBool monitorAllDrives=EFalse;
	TFileName notifyPath;
	
	TRAP(ret,aRequest->ReadL(KMsgPtr1,notifyPath));
	if(ret!=KErrNone)
		return(ret);
	if(notifyPath.Length()==0)
		return(KErrArgument);

	if ((notifyPath[0]==KMatchAny)||(notifyPath[0]==KMatchOne))
		{
	//	Use the default session drive for now
	//	Client has submitted a ? or * for drive
		monitorAllDrives=ETrue;
		ret=ParseNotificationPath(aRequest,aRequest->Src(),notifyPath);
		if (ret!=KErrNone)
			return(ret);
		}
	else
		{
	//	Client has submitted a single drive to monitor
		monitorAllDrives=EFalse;
		
	    ret = ParseNoWildSubstPtr1(aRequest, aRequest->Src());
		if (ret!=KErrNone)
			return(ret);
		}
	aRequest->SetScratchValue(monitorAllDrives);
	ret = PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsNotifyChangeEx, __PLATSEC_DIAGNOSTIC_STRING("Extended Change Notifier"));		
	return(ret);
	}

TInt TFsNotifyChangeCancel::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	FsNotify::CancelChangeSession(aRequest->Session());
#if defined(_DEBUG)
	FsNotify::CancelDebugSession(aRequest->Session());
#endif
	return(KErrNone);
	}



TInt TFsNotifyChangeCancel::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}



TInt TFsNotifyChangeCancelEx::DoRequestL(CFsRequest* aRequest)
//
//
//
	{
	FsNotify::CancelChangeSession(aRequest->Session(),(TRequestStatus*)aRequest->Message().Ptr0());
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	FsNotify::CancelDebugSession(aRequest->Session());
#endif
	return(KErrNone);
	}



TInt TFsNotifyChangeCancelEx::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

//
// Setup a disk space notification request
//
TInt TFsNotifyDiskSpace::DoRequestL(CFsRequest *aRequest)
	{
	//-- first check that the threshold value is correct and is less than the volume size
    const TUint64 threshold = aRequest->ScratchValue64();
    if(threshold <= 0)
        return KErrArgument;

    //-- get the size of the mounted volume to check that the requested threshold isn't larger than the volume
    TUint64 volSz;
    TInt r = aRequest->Drive()->MountedVolumeSize(volSz);
	if (r!=KErrNone)
		return r;

    if(threshold >= volSz)
        return KErrArgument;


    //-- Create the disk space notificator object and append it to the FsNotify queue
	CDiskSpaceInfo* info=new CDiskSpaceInfo;
	if(info==NULL)
		return(KErrNoMemory);
	
	const RMessage2& m=aRequest->Message();
	info->Initialise((TRequestStatus*)m.Ptr2(),m,aRequest->Session(),threshold);
	
	r=FsNotify::AddDiskSpace(info,aRequest->DriveNumber());
	if(r!=KErrNone)
		delete info;
	
	return(r);
	}


TInt TFsNotifyDiskSpace::Initialise(CFsRequest *aRequest)
//
//
//
	{
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int1(),aRequest);
	if (r==KErrNone)
		{
		TInt64 threshold;
		TPtr8 tBuf((TUint8*)&threshold,sizeof(TInt64));
		aRequest->ReadL(KMsgPtr0,tBuf);
		aRequest->SetScratchValue64(threshold);
		}
	return(r);
	}

TInt TFsNotifyDiskSpaceCancel::DoRequestL(CFsRequest *aRequest)
//
// Cancel disk space notification
//
	{
	FsNotify::CancelDiskSpaceSession(aRequest->Session(),(TRequestStatus*)aRequest->Message().Ptr0());
	return(KErrNone);
	}


TInt TFsNotifyDiskSpaceCancel::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return(KErrNone);
	}

TInt TFsSynchroniseDriveThread::DoRequestL(CFsRequest* /*aRequest*/)
//
// This is to ensure that previous message has been handled in drive thread. 
//
	{
	return CFsRequest::EReqActionComplete;
	}

TInt TFsSynchroniseDriveThread::Initialise(CFsRequest *aRequest)
//
// Set the drive thread.
//
	{
	if(aRequest->Message().Int0() == -1) //If no drive thread then complete the message
		{
	    return CFsRequest::EReqActionComplete;
  		}
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return r;
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
EXPORT_C void DebugNotifySessions(TInt aFunction,TInt /*aDrive*/)
//
//	Notify sessions of a debug event
//
    {
	// Notifying involves memory de-allocation on the file server's heap -
	// check if we need to switch heaps.
	RAllocator* current_alloc = &User::Heap();
	RAllocator* svr_alloc = ServerThreadAllocator;
	if (current_alloc != svr_alloc)
		User::SwitchHeap(svr_alloc);
	FsNotify::HandleDebug(aFunction);
	if (current_alloc != svr_alloc)
		User::SwitchHeap(current_alloc);
    }
#else
EXPORT_C void DebugNotifySessions(TInt,TInt)
//
//	Notify sessions of a debug event
//
    {}
#endif


TInt TFsDriveList::DoRequestL(CFsRequest* aRequest)
//
// Get the current drive list.
//
	{
	TDriveList list(KMaxDrives);

	const TUint mask = (TUint) aRequest->Message().Int1();
	const TUint matchedFlags=  mask & KDriveAttMatchedFlags;  //KDriveAttMatchedFlags = 0xFFF
	const TUint matchedAtt = mask & KDriveAttMatchedAtt;	 //KDriveAttMatchedAtt = 0x0FFF0000
	
	TInt r = ValidateMatchMask(mask);
	if(r!=KErrNone) 
	 	return r;
	
	for (TInt i=0;i<KMaxDrives;i++)
		{
		if(RFs::IsValidDrive(i))
			{
			const TUint driveAtt= TheDrives[i].Att();
			if(matchedFlags != 0 )
				{
				switch(matchedAtt)
					{
					case KDriveAttExclude :
						{
						list[i]= (driveAtt & matchedFlags ) ? (TUint8)0:(TUint8)driveAtt ;
						break;
						}
					case 0:
						{
						list[i] = (driveAtt & matchedFlags) ? (TUint8)driveAtt:(TUint8)0 ;
						break;
						}
					case KDriveAttExclusive :
						{
						if(matchedFlags != KDriveAttLogicallyRemovable)
							{
							list[i] = ((TUint8)driveAtt == matchedFlags)  ? (TUint8)driveAtt:(TUint8)0;
							}
						else 
							{
							list[i] = (driveAtt == (matchedFlags | KDriveAttRemovable))  ? (TUint8)driveAtt:(TUint8)0;
							}						
						break;
						}
					case KDriveAttExclude | KDriveAttExclusive:
						{
						if(matchedFlags != KDriveAttLogicallyRemovable)
							{
							list[i] = ((TUint8)driveAtt == matchedFlags)  ?(TUint8)0:(TUint8)driveAtt;
							}
						else 
							{
							list[i] = (driveAtt == (matchedFlags | KDriveAttRemovable))  ? (TUint8)driveAtt:(TUint8)0;
							}
						break;
						}
					default:
						{
						return KErrArgument;
						}
					}
				}
			else  //matchedFlags == 0
				{
				switch(matchedAtt)
					{
					case 0:
						list[i] = (TUint8)0 ;
						break;
					case KDriveAttAll:
						list[i]= (TUint8)driveAtt;
						break;
					default:   //all other cases are incorrect
						return KErrArgument;				
					}
				}
			}
		else //r!=KErrNone
			{
			list[i]=0;
			}			
		}

	aRequest->WriteL(KMsgPtr0,list);

	// Finally, kick off a speculative probe for devices
	ProxyDrives->Lock();
	TInt idx = ProxyDrives->Count();
	while(idx--)
		{
		CExtProxyDriveFactory* pF = (CExtProxyDriveFactory*)(*ProxyDrives)[idx];
		if(pF)
			{
			pF->AsyncEnumerate();
			}
		}
	ProxyDrives->Unlock();

	return(KErrNone);
	}

TInt TFsDriveList::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

TInt TFsDrive::DoRequestL(CFsRequest* aRequest)
//
// Get drive info.
//
	{
	// executed in main thread, therefore lock to ensure that
	// dismount cannot occur during request
	FsThreadManager::LockDrive(aRequest->DriveNumber());
	TDriveInfo info;
	aRequest->Drive()->DriveInfo(info);
	if(aRequest->SubstedDrive())
		info.iDriveAtt=KDriveAttSubsted;
	FsThreadManager::UnlockDrive(aRequest->DriveNumber());
	TPckgC<TDriveInfo> pInfo(info);
	aRequest->WriteL(KMsgPtr0,pInfo);
	return(KErrNone);
	}

TInt TFsDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int1(),aRequest);
	return(r);
	}


/**
    Get volume info.
*/
TInt TFsVolume::DoRequestL(CFsRequest* aRequest)
	{
	TVolumeInfo v;
    TPckg<TVolumeInfo> pV(v);

    //-- read TVolumeInfo from the client side, the client may have provided some data to pass to the server and FSY side
    aRequest->ReadL(KMsgPtr0,pV);

    TRequestStatus* pStat = (TRequestStatus*)aRequest->Message().Ptr2();
    if(pStat)
        {//-- the user called an asynchronous version of the RFs::Volume
         //-- indicate that we request free space asynchronously by setting a special flag that we will pass to the FSY
        v.iVolSizeAsync = ETrue;
        //-- at present the user's request will be completed by file server as the result of TFsVolume operation.
        }
    else
        {
        v.iVolSizeAsync = EFalse;    
        }
    
    //-- ask the FSY to provide us its volume information
    TInt r=aRequest->Drive()->Volume(v);
	
    CSessionFs* session=aRequest->Session();
	if (r==KErrNone)
		{
		TDrive* drive = aRequest->Drive();
        const TInt driveNumber = drive->DriveNumber();
		
        if(!session->ReservedAccess(driveNumber))
		    {
        	const TInt reserve = drive->ReservedSpace();
            if(v.iFree <= reserve)
                v.iFree = 0;
            else
                v.iFree -= reserve;
            }

		if(aRequest->SubstedDrive())
			v.iDrive.iDriveAtt=KDriveAttSubsted;

		
		
        if (drive->IsMounted() && drive->CurrentMount().LocalBufferSupport() == KErrNone && CCacheManagerFactory::CacheManager() != NULL)
			v.iFileCacheFlags = TFileCacheSettings::Flags(driveNumber);
		else
			v.iFileCacheFlags = TFileCacheFlags(0);
		
        
		aRequest->WriteL(KMsgPtr0,pV);
		}
	
    return(r);
	}

TInt TFsVolume::Initialise(CFsRequest* aRequest)
//
//
//
	{

	TInt r=ValidateDriveDoSubst(aRequest->Message().Int1(),aRequest);
	return(r);
	}


TInt TFsSetVolume::DoRequestL(CFsRequest* aRequest)
//
// Set the volume name.
//
	{
    TInt r = CheckDiskSpace(KMinFsCreateObjTreshold, aRequest);
    if(r != KErrNone)
        return r;

	TFileName volumeName;
	aRequest->ReadL(KMsgPtr0,volumeName);
	if (volumeName.Length()>KMaxVolumeNameLength)	//	KMaxVolumeNameLength = 11 
		return(KErrBadName);
	
    //	Validate name - return KErrBadName if it contains illegal characters such as * ? / | > <
	TNameChecker checker(volumeName);
	TText badChar;
	if (checker.IsIllegalName(badChar))
		return(KErrBadName);

	return(aRequest->Drive()->SetVolume(volumeName));
	}


TInt TFsSetVolume::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ValidateDriveDoSubst(aRequest->Message().Int1(),aRequest);
	if (r!=KErrNone)
		return(r);
	if (!KCapFsSetVolume.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Set Volume")))
		return KErrPermissionDenied;
	return KErrNone;
	}


TInt TFsSubst::DoRequestL(CFsRequest* aRequest)
//
// Get a drive substitution.
//
	{
	TInt r=ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return(r);
	TFileName substName;
	__PRINT(_L("aRequest->Drive()->Att()&KDriveAttSubsted"));
	__PRINT1(_L("aRequest->Drive()->Subst()"),&aRequest->Drive()->Subst());
	if (aRequest->Drive()->IsSubsted())
		substName=aRequest->Drive()->Subst();
	aRequest->WriteL(KMsgPtr0,substName);
	return(KErrNone);
	}


TInt TFsSubst::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}


TInt TFsSetSubst::DoRequestL(CFsRequest* aRequest)
//
// Set a drive substitution.
//
	{
	TInt r=ValidateDrive(aRequest->Message().Int1(),aRequest);
	if (r!=KErrNone)
		return(r);
	TDrive* pD=aRequest->Drive();
	r=ParsePathPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	//TODO: no protection for unsubsting drives (could check substed path if system then only CRoot process can unsubst
	if (pD->IsSubsted() && !aRequest->Src().DrivePresent() && !aRequest->Src().PathPresent())
		{
		delete &pD->Subst();
		pD->SetSubst(NULL);
		pD->SetAtt(0);
		pD->SetSubstedDrive(NULL);
		return(KErrNone);
		}
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysSetSubst,&KCapFsPriSetSubst,&KCapFsROSetSubst, __PLATSEC_DIAGNOSTIC_STRING("Set Subst"));
	if (r!=KErrNone)
		return(r);
	if (pD->Att()&(KDriveAttLocal|KDriveAttRom|KDriveAttRedirected|KDriveAttSubsted))
		return(KErrGeneral);
	if (pD==aRequest->Drive())
		return(KErrGeneral);
	if (aRequest->Drive()->Att()&(KDriveAttRedirected|KDriveAttSubsted))
		return(KErrInUse);
	HBufC* pS=aRequest->Src().FullName().Alloc();
	if (pS==NULL)
		return(KErrNoMemory);
	__ASSERT_DEBUG(!&pD->SubstedDrive(),Fault(ETFsSetSubstNotNull));
	delete &pD->Subst();
	pD->SetSubst(pS);
	pD->SetSubstedDrive(aRequest->Drive());
	pD->SetAtt(KDriveAttSubsted);
	return(KErrNone);
	}

TInt TFsSetSubst::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsSetSubst.CheckPolicy(aRequest->Message(),__PLATSEC_DIAGNOSTIC_STRING("Set subst")))
	    return KErrPermissionDenied;
	return KErrNone;
	}


TInt TFsRealName::DoRequestL(CFsRequest* aRequest)
//
// Get the real name of a file.
//
	{
	TInt r=ParseSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest,aRequest->Src().FullName().Mid(2),&KCapFsSysRealName,&KCapFsPriRealName, __PLATSEC_DIAGNOSTIC_STRING("Real Name"));
	if (r!=KErrNone)
		return(r);
	TFileName substName;
	if (aRequest->Drive()->Att()&KDriveAttRedirected)
		substName=aRequest->Drive()->Subst();	//	DANGER?
	else
		substName=aRequest->Src().Drive();
	if ((substName.Length()+aRequest->Src().FullName().Mid(2).Length())>KMaxFileName)
		return(KErrBadName);
	substName+=aRequest->Src().FullName().Mid(2);
	aRequest->WriteL(KMsgPtr1,substName);
	return(KErrNone);
	}

TInt TFsRealName::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

TInt TFsDefaultPath::DoRequestL(CFsRequest* aRequest)
//
// Get the default path.
//
	{
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
		return KErrNotSupported;
	else if(PlatSec::ConfigSetting(PlatSec::EPlatSecDiagnotics))
		{
		//FIXME: to be remove the following when platform is fully secure
		RThread tT;
		TInt r=aRequest->Message().Client(tT,EOwnerThread);
		if(r!=KErrNone)
			return r;
		RProcess tP;
		r=tT.Process(tP);
		if(r!=KErrNone)	
			{
			tT.Close();	
			return r;
			}
		TName n;
		n=tP.Name();
		TInt b=n.Locate('[');
		if (b>=0)
			n.SetLength(b);
		RDebug::Print(_L("**** API violation: %S should not use DefaultPath()\n"),&n);
		tP.Close();
		tT.Close();
		//FIXME END
		aRequest->WriteL(KMsgPtr0,TheDefaultPath);
		return(KErrNone);
		}
	else
		{
		aRequest->WriteL(KMsgPtr0,TheDefaultPath);
		return(KErrNone);
		}
	}

TInt TFsDefaultPath::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

TInt TFsSetDefaultPath::DoRequestL(CFsRequest* aRequest)
//
// Set the default path.
//
	{
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
		return KErrNotSupported;
	else if(PlatSec::ConfigSetting(PlatSec::EPlatSecDiagnotics))
		{
		//FIXME: to be remove the following when platform is fully secure
		RThread tT;
		TInt r=aRequest->Message().Client(tT,EOwnerThread);
		if(r!=KErrNone)
			return r;
		RProcess tP;
		r=tT.Process(tP);
		if(r!=KErrNone)	
			{
			tT.Close();	
			return r;
			}
		TName n;
		n=tP.Name();
		TInt b=n.Locate('[');
		if (b>=0)
			n.SetLength(b);
		RDebug::Print(_L("**** API violation: %S should not use SetDefaultPath()\n"),&n);
		tP.Close();
		tT.Close();
		//FIXME END
		TParse parse;
		/*TInt*/ r=ParsePathPtr0(aRequest,parse);	
		if (r!=KErrNone && r!=KErrInUse)
			return(r);
		if (IsIllegalFullName(parse.FullName()))
			return(KErrBadName);
		TheDefaultPath=parse.FullName();
		return(KErrNone);
		}
	else
		{
		TParse parse;
		TInt r=ParsePathPtr0(aRequest,parse);	
		if (r!=KErrNone && r!=KErrInUse)
			return(r);
		if (IsIllegalFullName(parse.FullName()))
			return(KErrBadName);
		TheDefaultPath=parse.FullName();
		return(KErrNone);
		}
	}


TInt TFsSetDefaultPath::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

TInt TFsSessionPath::DoRequestL(CFsRequest* aRequest)
//
// Get the session path
//
	{
	aRequest->WriteL(KMsgPtr0,aRequest->Session()->Path());
	return(KErrNone);
	}

TInt TFsSessionPath::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}


TInt TFsSetSessionPath::DoRequestL(CFsRequest* aRequest)
//
// Set the session path
//
	{
	TParse parse;
	TInt r=ParsePathPtr0(aRequest,parse);	
	if (r!=KErrNone && r!=KErrInUse)
		return(r);
	r=PathCheck(aRequest,parse.FullName().Mid(2),&KCapFsSysSetSessionPath,&KCapFsPriSetSessionPath, __PLATSEC_DIAGNOSTIC_STRING("Set Session Path"));
	if (r!=KErrNone)
		return(r);
	if (IsIllegalFullName(parse.FullName()))
		return(KErrBadName);
	HBufC* pP=parse.FullName().Alloc();
	if (pP==NULL)
		return(KErrNoMemory);
	delete &aRequest->Session()->Path();
	aRequest->Session()->SetPath(pP);
	return(KErrNone);
	}

TInt TFsSetSessionPath::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return(KErrNone);
	}


TInt TFsParse::DoRequestL(CFsRequest* aRequest)
//
// Parse a file name.
//
	{
	TFileName name;
	aRequest->ReadL(KMsgPtr0,name);
	TFileName rel;
	if (aRequest->Message().Ptr1()!=NULL)
		aRequest->ReadL(KMsgPtr1,rel);
	TParse p;
	TInt r=p.Set(name,&rel,&aRequest->Session()->Path());
	if (r==KErrNone)
		{
		TPckgC<TParse> pP(p);
		aRequest->WriteL(KMsgPtr2,pP);
		}
	return(r);
	}

TInt TFsParse::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}

TInt TFsReserveDriveSpace::DoRequestL(CFsRequest* aRequest)
//
// set reserved value to add to a drives reserved area
//	
	{
	// extract request info
	CSessionFs* session=aRequest->Session(); 
	TDrive* drive = aRequest->Drive();

	if(session->ReservedAccess(drive->DriveNumber()))
		return KErrInUse;

	const TInt reserve = aRequest->Message().Int1(); //-- bytes to reserve on the drive

	// Check if requested reserve space is within the range
	if(reserve > KMaxSessionDriveReserved || reserve < 0) 
		return KErrArgument;
	
    const TInt64 threshold = reserve + drive->ReservedSpace(); //-- free bytes on the volume we actually need 
    
    TInt nRes = drive->RequestFreeSpaceOnMount(threshold);
    if(nRes != KErrNone)
        return nRes;

	const TInt current = session->Reserved(drive->DriveNumber()); 
	const TInt diff = reserve - current;	
	TInt drvReserved = drive->ReservedSpace();
	if((drvReserved + diff) > KMaxTotalDriveReserved)
		return KErrTooBig;

	drvReserved += diff;
	drive->SetReservedSpace(drvReserved);
	return session->SetReserved(drive->DriveNumber(), reserve);

	}

TInt TFsReserveDriveSpace::Initialise(CFsRequest* aRequest)
//
//	Validate drive and set up the session parameters for request
//	
	{
//	if (!KCapFsReserveDriveSpace.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Reserve Drive Space")))
//		return KErrPermissionDenied;
	TInt drvNumber=aRequest->Message().Int1();
	drvNumber=aRequest->Message().Int0();
	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest);	
	return(r);
	}


TInt TFsGetReserveAccess::DoRequestL(CFsRequest* aRequest)
//
//	Get reserved access to a drives reserved area first checking that the session has first reserved some space
//	
	{
	CSessionFs* session=aRequest->Session();

	TInt size=session->Reserved(aRequest->Drive()->DriveNumber());
	if(size <= 0)
		return KErrPermissionDenied;
	else
		session->SetReservedAccess(aRequest->Drive()->DriveNumber(),ETrue);
	return KErrNone;
	}

TInt TFsGetReserveAccess::Initialise(CFsRequest* aRequest)
//
//
//	
	{
//	if (!KCapFsGetReserveAccess.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Get Reserve Access")))
//		return KErrPermissionDenied;
	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest); 
	return(r);
	}


TInt TFsReleaseReserveAccess::DoRequestL(CFsRequest* aRequest)
//
//	Remove access for this session to the locked area for a given drive 
//	
	{
	aRequest->Session()->SetReservedAccess(aRequest->Drive()->DriveNumber(),EFalse);
	return KErrNone;
	}

TInt TFsReleaseReserveAccess::Initialise(CFsRequest* aRequest)
//
//
//	
	{
//	if (!KCapFsReleaseReserveAccess.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Release Reserve Access")))
//		return KErrPermissionDenied;
	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest);	
	return(r);
	}

TInt TFsSetStartupConfiguration::DoRequestL(CFsRequest* aRequest)
//
//  Configure file server according to command and parameters.
//	
    {
    TInt cmd = aRequest->Message().Int0();

    switch (cmd)
    {
    case ELoaderPriority:
    // Set loader thread priority
        {
        TInt r;
        TFullName loaderFullName(RProcess().FullName());
        loaderFullName.Append(_L("::LoaderThread"));
        RThread loader;
        r = loader.Open(loaderFullName);
        if (r != KErrNone)
            return r;
        loader.SetPriority((TThreadPriority)aRequest->Message().Int1());
        loader.Close();
        }
        break;
	case ESetRugged:
		{
        TInt drive = aRequest->Message().Int1();
        TInt r = ValidateDrive(drive, aRequest);
        if (r != KErrNone)
            return r;
        TInt value = aRequest->Message().Int2();
		aRequest->Drive()->SetRugged(value);
		}
		break;
    default:
    	return KErrArgument;
    }

    return KErrNone;
    }

TInt TFsSetStartupConfiguration::Initialise(CFsRequest* aRequest)
//
//  Check SID of message sender. Can be estart only
//	
    {
    if (aRequest->Message().SecureId() != KEstartUidValue)
        return KErrPermissionDenied;
    return KErrNone;
    }


const TInt KinitialChanges = EChangesLocale|EChangesMidnightCrossover|EChangesThreadDeath|EChangesPowerStatus|EChangesSystemTime|EChangesFreeMemory|EChangesOutOfMemory;

CKernEventNotifier* CKernEventNotifier::New(TInt aPriority)
     {
     __PRINT(_L("CKernEventNotifier::New"));
     CKernEventNotifier* self = new CKernEventNotifier(aPriority);
     if (self && (self->iChangeNotifier.Create() != KErrNone))
         delete self, self = NULL;
     return self;
     }
 
CKernEventNotifier::~CKernEventNotifier()
     {
     Cancel();
     iChangeNotifier.Close();
     }
 
void CKernEventNotifier::Start()
     {
     SetActive();
     iChangeNotifier.Logon(iStatus);
     }
 
void CKernEventNotifier::RunL()
     {
     __PRINT(_L("CKernEventNotifier::RunL"));
     iChange = iStatus.Int();
     Start();
     
     /* Avoid being triggered by initial events */
     if (iChange != 0 && (iChange & KinitialChanges) != KinitialChanges)   
         {
         if (iChange & EChangesLocale)
             CKernEventNotifier::LocaleChangeCallback();
         if (iChange & EChangesFreeMemory)
             CKernEventNotifier::FreeMemoryChangeCallback();
         // Add other event capture below this line
         iChange = 0;
         }
     }
 
void CKernEventNotifier::DoCancel()
     {
     iChangeNotifier.LogonCancel();
     }
 

extern TBool FatUtilitiesUpdateDrivesNotified;
extern TBool FatUtilityFunctionsSet;

TInt CKernEventNotifier::LocaleChangeCallback(TAny*)
     {
     __PRINT(_L("CKernEventNotifier::LocaleChangeCallback"));
     
     //-- check if the locale has just been set and the drives are not yet notified about this.
     if(FatUtilityFunctionsSet && !FatUtilitiesUpdateDrivesNotified)
        {//-- notify drives about locale shange, but only once
        for(TInt i=0; i<KMaxDrives; i++)
            {
            TDrive& drive=TheDrives[i];

            if(drive.DriveNumber() >=0 && drive.IsRemovable() && !drive.IsSubsted())
                {
                __PRINT1(_L("CKernEventNotifier::LocaleChangeCallback upd drive: %d"), drive.DriveNumber());
                drive.SetChanged(ETrue);
                }
            }
     
            FatUtilitiesUpdateDrivesNotified = ETrue;
        }
 
     return KErrNone;
     }

TInt TFsQueryVolumeInfoExt::DoRequestL(CFsRequest* aRequest)
	{
	const TInt cmd = aRequest->Message().Int1();
	TInt rel;

    TDrive* pDrive = aRequest->Drive();

    rel = pDrive->CheckMount();
    if(rel != KErrNone)
        return rel;

	switch (cmd)
		{
		
        //-------------------------------------------------
        //-- file system sub type query
        case EFileSystemSubType:
			{
			TFSName name;
			if (pDrive->IsMounted())
				{
				rel = pDrive->CurrentMount().FileSystemSubType(name);

				//-- get the Mount's file system name if the FS subtype query is not supported or there are no subtypes.
                if (rel==KErrNotSupported)
					{
                    pDrive->CurrentMount().FileSystemName(name);
					}

				if (name.Length())
					{
					TPckgBuf<TFSName> pckgBuf(name);
					aRequest->WriteL(KMsgPtr2, pckgBuf);
					return KErrNone;
					}
				else
					{
					return rel;
					}
				}
			else
				{
				return KErrNotReady;
				}
			}
			
		
        //-------------------------------------------------
        //-- this is RFs::VolumeIOParam() query 
        case EIOParamInfo:
			{
			TVolumeIOParamInfo ioInfo;
			// 1. gets block size information via media driver
		    const TInt drive = aRequest->Message().Int0();
		    
		    // validates local drive numbers
		    if(!IsValidLocalDriveMapping(drive))
		    	{
		    	ioInfo.iBlockSize = KErrNotReady;
		    	}
		    else
		    	{
			    // Get media capability
			    TLocalDriveCapsV6Buf capsBuf;

				// is the drive local?
				if (!IsProxyDrive(drive))
					{
					// if not valid local drive, use default values in localDriveCaps
					// if valid local drive and not locked, use TBusLocalDrive::Caps() values
					// if valid drive and locked, hard-code attributes
					rel = GetLocalDrive(drive).Caps(capsBuf);
					}
				else  // this need to be made a bit nicer
					{   
					CExtProxyDrive* pD = GetProxyDrive(drive);
					if(pD)
						rel = pD->Caps(capsBuf);
					else
						rel = KErrNotReady;
					}

			    if (rel != KErrNone)
			    	{
			    	ioInfo.iBlockSize = rel;
			    	}
			    else
			    	{
				    TLocalDriveCapsV6& caps = capsBuf();
					if (caps.iBlockSize)
						{
						ioInfo.iBlockSize = caps.iBlockSize;
						}
					// returns default size (512) when block is not supported by 
					//  underlying media
					else
						{
						ioInfo.iBlockSize = KDefaultVolumeBlockSize;
						}
			    	}
		    	}

			// 2. gets cluster size via mounted file system; Also get Max. file size supported by the file system
			
            ioInfo.iMaxSupportedFileSize = KMaxTUint64; //-- the value for "not supported case"

            if (pDrive->IsMounted())
				{
				rel = pDrive->CurrentMount().FileSystemClusterSize();
				ioInfo.iClusterSize = rel; // return cluster size or an error code if error happened.
				
                pDrive->CurrentMount().GetMaxSupportedFileSize(ioInfo.iMaxSupportedFileSize);
                
                }
			else
				{
				ioInfo.iClusterSize = KErrNotReady;
				}

			// 3. get rec buffer size from estart.txt file
			{
                _LIT8(KLitSectionNameDrive,"Drive%C");
			    ioInfo.iRecReadBufSize = KErrNotSupported;
			    ioInfo.iRecWriteBufSize = KErrNotSupported;

			    TBuf8<8> sectionName;
			    TInt32 bufSize;
			    sectionName.Format(KLitSectionNameDrive, 'A' + drive);
			    // retrieve recommended buffer size information through F32 INI section
			    if (F32Properties::GetInt(sectionName, _L8("RecReadBufSize"), bufSize))
				    {
				    ioInfo.iRecReadBufSize = bufSize;
				    }
			    if (F32Properties::GetInt(sectionName, _L8("RecWriteBufSize"), bufSize))
				    {
				    ioInfo.iRecWriteBufSize = bufSize;
				    }
			    
			    // packaging and returning results
			    TPckgBuf<TVolumeIOParamInfo> pckgBuf(ioInfo);
			    aRequest->WriteL(KMsgPtr2, pckgBuf);
			}

			// always return KErrNone as error codes are packaged and returned via ioInfo members
			return KErrNone;
			} //case EIOParamInfo:

            //-------------------------------------------------
            //-- check if the specified drive is synchronous or not
            case EIsDriveSync:
            {
                const TInt drive = aRequest->Message().Int0();
                const TBool bDrvSynch = FsThreadManager::IsDriveSync(drive, EFalse);
                TPckgBuf<TBool> buf(bDrvSynch);
                aRequest->WriteL(KMsgPtr2, buf);
                
                return KErrNone;
            }

            //-------------------------------------------------
            //-- query if the drive is finalised
            case EIsDriveFinalised:
            {
                TBool bFinalised;
                TInt nRes = pDrive->CurrentMount().IsMountFinalised(bFinalised);
                if(nRes != KErrNone)
                    return nRes;

                TPckgBuf<TBool> buf(bFinalised);
                aRequest->WriteL(KMsgPtr2, buf);

                return KErrNone;
            }
            case EFSysExtensionsSupported:
            {
                TBool supported = pDrive->GetFSys()->IsExtensionSupported();
                TPckgBuf<TBool> data(supported);
                aRequest->WriteL(KMsgPtr2,data);
                return KErrNone;
            }
		default:
			{
			return KErrNotSupported;
			}
		}
	}
	
TInt TFsQueryVolumeInfoExt::Initialise(CFsRequest* aRequest)
	{
	TInt r = ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return r;
	}

TInt CKernEventNotifier::FreeMemoryChangeCallback()
	{
	__PRINT(_L("CKernEventNotifier::FreeMemoryChangeCallback"));
 
	TBool belowThreshold = (iChange & EChangesLowMemory)?(TBool)ETrue:(TBool)EFalse;
	CCacheManager* manager = CCacheManagerFactory::CacheManager();
	if (manager)
		{
		manager->FreeMemoryChanged(belowThreshold);

		// start flushing all dirty data
		if (belowThreshold)
			{
			Files->Lock();
			TInt count=Files->Count();
			while(count--)
				{
				CFileCB* file = (CFileCB*)(*Files)[count];
				if (file->FileCache())
					// Cannot report errors here
					// coverity [unchecked_value]
					(void)file->FileCache()->FlushDirty();
				}
			Files->Unlock();
			}
		}

#ifdef	SYMBIAN_ENABLE_FAT_DIRECTORY_OPT
	CCacheMemoryManager* cacheMemManager = CCacheMemoryManagerFactory::CacheMemoryManager();
	if (cacheMemManager)
		cacheMemManager->FreeMemoryChanged(belowThreshold);
#endif //#ifdef	SYMBIAN_ENABLE_FAT_DIRECTORY_OPT

	return KErrNone;
	}

