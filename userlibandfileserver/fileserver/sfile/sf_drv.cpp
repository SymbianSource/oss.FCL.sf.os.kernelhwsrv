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
// f32\sfile\sf_drv.cpp
// 
//

#include "sf_std.h"
#include "sf_file_cache.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_drvTraces.h"
#endif
//const TInt KMaxNotifierAttempts=4; // not used anywhere

static TPtrC StripBackSlash(const TDesC& aName)
//
// If aName ends in a backslash, strip it.
//
	{

	__ASSERT_DEBUG(aName.Length(),Fault(EStripBackSlashBadName));
	if (aName[aName.Length()-1]==KPathDelimiter)
		return(aName.Left(aName.Length()-1));
	return(aName);
	}

static void CheckSubClose(CFsObject* anObj,TInt aHandle, CSessionFs* aSession)
//
// Close anObj if its not NULL.
//
	{
	__PRINT1(_L("CheckSubClose() session 0x0%x"),aSession);
	__PRINT1(_L("CheckSubClose() anObj 0x0%x"),anObj);

	if(!anObj)
		return;

	if(aHandle==0)
		{
		// can't have been added to the object index
		__ASSERT_DEBUG(KErrNotFound==aSession->Handles().At(anObj,ETrue),Fault(ESubOpenBadHandle));
		anObj->Close();
		}
	else
		aSession->Handles().Remove(aHandle,ETrue);
	}

TInt ValidateDrive(TInt aDriveNumber,CFsRequest* aRequest)
//
// Validate a drive number and set iTheDrive.
//
	{
	if (aDriveNumber==KDefaultDrive)
		aDriveNumber=aRequest->Session()->CurrentDrive();
	if (!RFs::IsValidDrive(aDriveNumber))
		return(KErrBadName);
	aRequest->SetDrive(&TheDrives[aDriveNumber]);
	return(KErrNone);
	}

TInt ValidateDriveDoSubst(TInt aDriveNumber,CFsRequest* aRequest)
//
// Validate a drive number and set iTheDrive.
//
	{

	TInt r=ValidateDrive(aDriveNumber,aRequest);
	if (r!=KErrNone)
		return(r);
	if (aRequest->Drive()->IsSubsted())
		{
		aRequest->SetSubstedDrive(aRequest->Drive());
		aRequest->SetDrive(&aRequest->Drive()->SubstedDrive());
		}
	return(KErrNone);
	}

void ValidateAtts(TUint& aSetAttMask,TUint& aClearAttMask)
//
// Do not allow the entry type to be changed
//
	{
	const TUint KReadOnlyAtts = KEntryAttVolume	| 
								KEntryAttDir	| 
								KEntryAttRemote	|
								KEntryAttModified;

	aSetAttMask   &= ~KReadOnlyAtts;
	aClearAttMask &= ~KReadOnlyAtts;
	}

void CheckForLeaveAfterOpenL(TInt leaveError, CFsRequest* aRequest, TInt aHandle)
//
// Tidy up in the event of a leave after opening a file or directory
	{
	if (leaveError)
		{
		CFsObject* anObj=(CFsObject* )aRequest->ScratchValue();
		CheckSubClose(anObj,aHandle,aRequest->Session());
		User::Leave(leaveError);
		}
	}

TDrive::TDrive()
//
// Constructor.
//
	: iDriveNumber(0),iAtt(0),iChanged(EFalse),
	  iFSys(NULL),iCurrentMount(NULL),iSubstedDrive(NULL),iSubst(NULL),
	  iMount(NULL),iDriveFlags(0),iMountFailures(0)
	{}

void TDrive::CreateL(TInt aDriveNumber)
//
// Allocate the drive number and any resources.
//
	{
	__PRINT1(_L("TDrive::CreateL(%d)"),aDriveNumber);
	iDriveNumber=aDriveNumber;
	iMount=TheContainer->CreateL();
	TInt r=iLock.CreateLocal();
	User::LeaveIfError(r);

	iDeferredDismountRequest = new(ELeave) CFsInternalRequest;
	iDeferredDismountRequest->Set(DeferredDismountOp,NULL);
	iDeferredDismountRequest->SetDriveNumber(aDriveNumber);
	}

TInt TDrive::CheckMountAndEntryName(const TDesC& aName)
//
// Check drive is mounted then check aName is legal
//
	{

	__PRINT1(_L("TDrive::CheckMountAndEntryName Drive%d"),DriveNumber());
	TInt r=CheckMount();
	if (r==KErrNone && IsIllegalFullName(aName))
		return(KErrBadName);
	return(r);
	}

void TDrive::MultiSlotDriveCheck()
	{
	// Check whether the current drive is a dual-slot/multi-slot
	// if so, we need to check which drive is connected now and 
	// swap the mapping in LocalDrives::iMapping such that the 
	// mapping of driveNumber to localDriveNumber is correct.

	Lock();
	//Is this a multislot drive?
	if(LocalDrives::iIsMultiSlotDrive[iDriveNumber])
		{
		for(TInt localDrvNum=0; localDrvNum<KMaxLocalDrives; localDrvNum++)
			{
			// ensure that this local drive is a multi-slot choice for this drive number..
			if(LocalDrives::iReverseMapping[localDrvNum]==iDriveNumber)
				{
				// Caps - find out which one is connected
				TLocalDriveCapsBuf capsInfo;
				TInt r = LocalDrives::iLocalDrives[localDrvNum].Caps(capsInfo);
				if(r==KErrNotReady)
					{
					continue; //go to next localdrive
					}
				//found a connected drive
				//Update mapping
				#ifdef _DEBUG
				RDebug::Print(_L("Multislot drive mapping update: DriveNum %d to LocDrv %d"),iDriveNumber,localDrvNum);
				#endif
				
				LocalDrives::iMapping[iDriveNumber] = localDrvNum;
				break; // Swap complete - don't look any further
				}
			}
		}	
	UnLock();
	}

TInt TDrive::CheckMount()
//
// Check the drive and try to mount a media if not already mounted.
//
	{
	__PRINT2(_L("TDrive::CheckMount Drive%d, changed:%d"),DriveNumber(), iChanged);
	__CHECK_DRIVETHREAD(iDriveNumber);
	
	if (!iFSys)
		return KErrNotReady;
		
	if (iChanged)				//	If a media change has occurred
		{
		iMountFailures = 0;
		iChanged=EFalse;		//	Reset the flag
		if (IsMounted())		//	Dismount the mount if it is still marked as mounted
			{
            DoDismount();
			}
		//If we have a dual/multi removable media slot then we may need to
		//swop the mappings.
		MultiSlotDriveCheck();
		}
	
	if (!IsMounted())				//	Checks that iCurrentMount!=NULL
		{
		__PRINT(_L("TDrive::CheckMount() Not Mounted"));
        const TInt KMaxMountFailures = 3;
		// if we've repeatedly failed to mount, give up until a media change
		if (iMountFailures >= KMaxMountFailures)
			{
			__PRINT1(_L("TDrive::CheckMount() retries exceeded, last Err:%d"), iLastMountError);
			return iLastMountError;
			}

		if (!ReMount())	    //	Have we just remounted a mount we have previously encountered?
			{			
			MountFileSystem(EFalse);	//	If not, mount it for the first time now
			}
		else if(IsWriteProtected() && IsWriteableResource())
			{
			DoDismount();
			return KErrAccessDenied;
			}
		}

	if (iReason==KErrNone && CurrentMount().LockStatus() > 0)
	    {
    	//-- this means that the mount has drive access objects opened (RFormat or RRawDisk)
        __PRINT1(_L("TDrive::CheckMount() Mount is locked! LockStaus:%d"), CurrentMount().LockStatus());
        return KErrInUse;
	    }	

	__PRINT1(_L("TDrive::CheckMount returned %d "),iReason);
		
	return(iReason);
	}

//----------------------------------------------------------------------------
/**
    Try and re-mount any of the pending media
    
    @return ETrue if the mount matching media found and been attached back (set as iCurrentMount)
*/
TBool TDrive::ReMount()
	{
	const TInt mCount=Mount().Count();
    __PRINT1(_L("TDrive::ReMount() MountCnt:%d"), mCount);
	
	const TInt u=(Mount().UniqueID()<<16);
	iReason=KErrNone;
	
    //-- try every instance of CMountCB that is associated with this object of TDrive.
    //-- mounts are stored in the container of mCount elements.
    //-- if some CMountCB recognises the media it belongs, it means that "remount succeded"
    for(TInt i=0; i<mCount; i++)
		{
		CMountCB* pM=(CMountCB*)Mount().At(u|i);
		
        if (ReMount(*pM))
			return ETrue;
		}

	return EFalse;
	}

//----------------------------------------------------------------------------
/**
    Try and re-mount the specified media.

    @return ETrue if remounting succeeded - i.e. the CMountCB instance that matches the media is found in the 
    mounts container (Mount()) and bound to the media.
*/
TBool TDrive::ReMount(CMountCB& aMount)
	{
	__PRINT1(_L("TDrive::ReMount(0x%x)"), &aMount);
	iReason=KErrNone;

	if (!aMount.IsDismounted() && !aMount.ProxyDriveDismounted())
		{
		aMount.SetDrive(this);
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREMOUNT1, "drive %d", DriveNumber());
		
        //-- actually, this is asking CMountCB to see if it belongs to the current media. 
        iReason = aMount.ReMount();

		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREMOUNT1RET, "success %d", iReason);
		
        if (iReason == KErrNone)	//	ReMount succeeded
			{
			aMount.Open();
			iCurrentMount = &aMount;
			__PRINT1(_L("TDrive::ReMount for Mount:0x%x OK!"), &aMount);
			return ETrue;
			}

		__PRINT2(_L("TDrive::ReMount for Mount:0x%x failed iReason=%d"),&aMount,iReason);
		}
	else
		{
		__PRINT1(_L("TDrive::ReMount() failed - Mount:0x%x is dismounted"), &aMount);
		}

	return EFalse;
	}



//----------------------------------------------------------------------------
/**
    Mount the media on the drive. Optionally force a bad media to be mounted.
    
    @param  apMount     out: pointer to the produced CMountCB object; NULL if the CMountCB is not constructed
    @param  aForceMount if ETrue, the filesystem will be forcedly mounted on the drive, disregarding what it contains. 
    @param  aFsNameHash file system name hash; see TDrive::MountFileSystem()   
*/
void TDrive::DoMountFileSystemL(CMountCB*& apMount, TBool aForceMount, TUint32 aFsNameHash)
	{
	CFileSystem* pMountsFs = NULL; //-- reference to the filesystem that will be producing CMountCB
    
    apMount = NULL;

    OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
   
    //-- construct a new CmountCB object.
    //-- on return pMountsFs will be the pointer to the factory object of CFileSystem that produced this mount
    apMount = FSys().NewMountExL(this, &pMountsFs, aForceMount, aFsNameHash);

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTLRET, "r %d CMountCB %x", (TUint) KErrNone, (TUint) apMount);
	__PRINT2(_L("TDrive::MountMediaL created mount:0x%x FileSys:0x%x"), apMount, pMountsFs);

    ASSERT(pMountsFs && apMount);

	apMount->SetMountNumber(iMountNumber++);
    apMount->InitL(*this, pMountsFs);  //-- initialise Mount
    apMount->MountL(aForceMount);      //-- mount the file system  
	Mount().AddL(apMount,EFalse);      //-- add mount object to the mounts container.  

	iCurrentMount=apMount;
    }


//----------------------------------------------------------------------------
/*
	Mount file system on the drive. 
	@param  aForceMount if EFalse, will try to mount the file system normally, the file system implementation will decide if it can work on this drive or not.
                        if ETrue, will mount the file suystem by force, this is used mostly for formatting unrecognisable media.

    @param  aFsNameHash optional parameter. Can specify the concrete file system name (hash). It can be used to force mounting  some specific
                        file system. Default value '0' means "not specified / not used"
                                                              

    TDrive::iReason on return contains the operation result code.
*/
void TDrive::MountFileSystem(TBool aForceMount, TUint32 aFsNameHash /*=0*/ )
	{
	__PRINT2(_L("TDrive::MountFileSystem aForceMount=%d, FSNameHash:0x%x"),aForceMount, aFsNameHash);
	__CHECK_DRIVETHREAD(iDriveNumber);
	
    iCurrentMount=NULL;
	if(!iFSys)
		{
		iReason=KErrNotReady;
		return;
		}
	
    CMountCB* pM=NULL;
    TRAP(iReason, DoMountFileSystemL(pM, aForceMount, aFsNameHash));
	if (iReason == KErrNone)
		{
		iMountFailures = 0;
		ASSERT(iCurrentMount);
        }
	else
		{
		iLastMountError = iReason;
		iMountFailures++;
		__PRINT2(_L("TDrive::MountFileSystem 0x%x failed iReason=%d"),pM,iReason);
		if(pM)
			pM->Close();
		
        ASSERT(!iCurrentMount);
        }
	}


//----------------------------------------------------------------------------
/**
    Generic mount control method.
    @param  aLevel  specifies the operation to perfrom on the mount
    @param  aOption specific option for the given operation
    @param  aParam  pointer to generic parameter, its meaning depends on aLevel and aOption

    @return standard error code.
*/
TInt TDrive::MountControl(TInt aLevel, TInt aOption, TAny* aParam)
    {
	OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBMOUNTCONTROL, "drive %d aLevel %d aOption %x aParam %x", (TUint) DriveNumber(), (TUint) aLevel, (TUint) aOption, (TUint) aParam);
    TInt r = CurrentMount().MountControl(aLevel, aOption, aParam);
	
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBMOUNTCONTROLRET, "r %d", r);
	return r;
    }

//----------------------------------------------------------------------------
/**
    Request aFreeSpaceRequired free bytes from the mount associated with this drive.
    The volume free space on for some filesystems can be changing (usually increasing) after it has been mounted.
    If the mount supports this functionality, it can block this call until certain number of free bytes encounted if its free
    space calculation activity hasn't finished yet.

    @param  aFreeSpaceRequired  required free space, bytes.
    
    @return KErrNone        on success and if there is at least aFreeSpaceRequired bytes on the volume
            KErrDiskFull    on success and if there is no aFreeSpaceRequired bytes on the volume
            system-wide error code otherwise
*/
TInt TDrive::RequestFreeSpaceOnMount(TUint64 aFreeSpaceRequired)
    {
    TInt nRes;

    nRes = CheckMount();
    if(nRes != KErrNone)
        return nRes;

    //-- 1. Try mount-specific request first. If the mount is still performing free space calculations,
    //-- the caller will be suspended until aFreeSpaceRequired bytes is available or scanning process finishes
    {
        TUint64 freeSpaceReq = aFreeSpaceRequired;
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFREESPACE, "drive %d", DriveNumber());
        nRes = CurrentMount().RequestFreeSpace(freeSpaceReq);
		OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFREESPACERET, "r %d FreeSpace %x:%x", (TUint) nRes, (TUint) I64HIGH(freeSpaceReq), (TUint) I64LOW(freeSpaceReq));
        if(nRes == KErrNone)
            {
            return (freeSpaceReq >= aFreeSpaceRequired) ? KErrNone : KErrDiskFull;
            }
    }

    //-- given Mount doesn't support this functionality, use legacy method
    TVolumeInfo volInfo;
    nRes = Volume(volInfo);
    if(nRes !=KErrNone)
        return nRes;

    return ((TUint64)volInfo.iFree >= aFreeSpaceRequired) ? KErrNone : KErrDiskFull;
    }

//----------------------------------------------------------------------------
/**
    Get size of the mounted volume. It can be less than physical volume size because FileSystem data may occupy some space.
    
    @param  aSize on success mounted volume size in bytes will be returned there
    @return KErrNone on success, standard error code otherwise
*/
TInt TDrive::MountedVolumeSize(TUint64& aSize)
    {
    TInt nRes;

    nRes = CheckMount();
    if(nRes != KErrNone)
        return nRes;
    //-- 1. Try mount-specific request first. It won't block this call as CMountCB::VolumeL() can do if some background activity is going on the mount
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBVOLUMESIZE, "drive %d", DriveNumber());
    nRes = CurrentMount().MountedVolumeSize(aSize);
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECMOUNTCBVOLUMESIZERET, "r %d aSize %x:%x", (TUint) nRes, (TUint) I64HIGH(aSize), (TUint) I64LOW(aSize));
    if(nRes == KErrNone)
        return nRes;

    //-- given Mount doesn't support this functionality, use legacy method
    TVolumeInfo volInfo;
    nRes = Volume(volInfo);
    if(nRes == KErrNone)
        {
        aSize = volInfo.iSize;
        }
    
    return nRes;
    }

//----------------------------------------------------------------------------
/**
    Get _current_ amount of free space on the volume. Some mounts implementations can be updating the amount of free space
    in background. 

    @param  aFreeDiskSpace on success will contain a current amount of free space
    @return KErrNone on success, standard error code otherwise

*/
TInt TDrive::FreeDiskSpace(TInt64& aFreeDiskSpace)
	{
    TInt nRes;

    nRes = CheckMount();
    if(nRes != KErrNone)
        return nRes;

    //-- 1. Try mount-specific request first. It won't block this call as CMountCB::VolumeL() can do 
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCURRENTFREESPACE, "drive %d", DriveNumber());
    nRes = CurrentMount().GetCurrentFreeSpaceAvailable(aFreeDiskSpace);
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCURRENTFREESPACERET, "r %d FreeSpace %x:%x", (TUint) nRes, (TUint) I64HIGH(aFreeDiskSpace), (TUint) I64LOW(aFreeDiskSpace));
    if(nRes == KErrNone)
        return nRes;

    //-- given Mount doesn't support this functionality, use legacy method
    TVolumeInfo volInfo;
    nRes = Volume(volInfo);
    if(nRes == KErrNone)
        {
        aFreeDiskSpace = volInfo.iFree;
        }
    
    return nRes;
	}

//----------------------------------------------------------------------------
/**
    Finalise drive (the mount).

    @param  aOperation  describes finalisation operation ,see RFs::TFinaliseDrvMode
    @param  aParam1     not used, for future expansion
    @param  aParam2     not used, for future expansion

    @return Standard error code
*/
TInt TDrive::FinaliseMount(TInt aOperation, TAny* aParam1/*=NULL*/, TAny* aParam2/*=NULL*/)
	{
	TInt r=CheckMount();
	if (r!=KErrNone)
		return(r);

	r = FlushCachedFileInfo();
	if (r!=KErrNone)
		return(r);

	if(IsWriteProtected())
		return(KErrAccessDenied);

	OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFINALISEMOUNT2, "drive %d aOperation %d aParam1 %x aParam2 %x", (TUint) DriveNumber(), (TUint) aOperation, (TUint) aParam1, (TUint) aParam2);
	TRAP(r,CurrentMount().FinaliseMountL(aOperation, aParam1, aParam2));
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFINALISEMOUNT2RET, "r %d", r);
	
	// Pass FinaliseDrive notification down to media driver
	TInt driveNumber = DriveNumber();
	if (LocalDrives::IsValidDriveMapping(driveNumber) && !LocalDrives::IsProxyDrive(driveNumber))
		{
		TBusLocalDrive& drv = LocalDrives::GetLocalDrive(driveNumber);

		TLocalDriveFinaliseInfoBuf finaliseBuf;
		finaliseBuf().iMode = aOperation;

		// notify local drive, ignore the error
		drv.QueryDevice(RLocalDrive::EQueryFinaliseDrive, finaliseBuf);	
		}

    return r;
	}

//----------------------------------------------------------------------------
/** old implementation */
TInt TDrive::FinaliseMount()
	{
	TInt r=CheckMount();
	if (r!=KErrNone)
		return(r);

	r = FlushCachedFileInfo();
	if (r!=KErrNone)
		return(r);

	if(IsWriteProtected())
		return(KErrAccessDenied);

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFINALISEMOUNT1, "drive %d", DriveNumber());
	TRAP(r,CurrentMount().FinaliseMountL());
	
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFINALISEMOUNT1RET, "r %d", r);
	
    return r;
	}



CFileCB* TDrive::LocateFile(const TDesC& aName)
//
//	Locate a file of the same name already open on the drive.
//
	{
	TDblQueIter<CFileCB> q(CurrentMount().iMountQ);
	CFileCB* pF;
	// early out for normal case, list is empty
	if(q==NULL)
		return NULL;
	
	// strip off trailing dots
	TInt length= aName.Length();
	while((length !=0) && (aName[length-1]==KExtDelimiter))
		{
		length--;
		}

	TPtrC temp(aName.Ptr(),length);

	TFileName tempName;
	tempName.CopyF(temp);
	TUint32 nameHash=CalcNameHash(tempName);

	while ((pF=q++)!=NULL)
		{
		if(nameHash==pF->NameHash())
			{
			if (pF->FileNameF().Match(tempName)==KErrNone)
				return(pF);
			}	
		}
	return(NULL);
	}


CFileCache* TDrive::LocateClosedFile(const TDesC& aName, TBool aResurrect)
//
//	Locate a recently closed file of the same name on the drive.
//
	{
	// strip off trailing dots
	TInt length= aName.Length();
	while((length !=0) && (aName[length-1]==KExtDelimiter))
		{
		length--;
		}

	TPtrC temp(aName.Ptr(),length);

	TFileName tempName;
	tempName.CopyF(temp);
	TUint32 nameHash=CalcNameHash(tempName);

	CFileCache* pF = NULL;
	CMountCB* currentMount = &CurrentMount();


	TClosedFileUtils::Lock();

	TInt count = TClosedFileUtils::Count();
	while(count--)
		{
		CFileCache* fileCache = TClosedFileUtils::At(count);
		if (&fileCache->Drive() == this &&
			fileCache->NameHash()== nameHash && 
			fileCache->FileNameF().Match(tempName)==KErrNone &&
			&fileCache->Mount() == currentMount)
			{
			__ASSERT_DEBUG(TClosedFileUtils::IsClosed(fileCache), Fault(EObjRemoveContainerNotFound));
			__CACHE_PRINT2(_L("CLOSEDFILES: LocateClosedFile(%S, %d\n"), &fileCache->FileNameF(), aResurrect);
			if (aResurrect)
				{
				TClosedFileUtils::ReOpen(fileCache, EFalse);
				}
			pF = fileCache;
			break;
			}

		}
	TClosedFileUtils::Unlock();

	if (pF != NULL && !aResurrect)
		{
		pF->Close();
		pF = NULL;
		}

	return(pF);
	}


static TBool IsSubDir(const TDesC& aFullName,const TDesC& aParent)
//
// Returns ETrue if aFullName is a subdirectory of aParent
// Assumes aParent is a path name with the trailing backslash removed
//
	{

	__ASSERT_DEBUG(aParent.Length() && aParent[aParent.Length()-1]!=KPathDelimiter,Fault(EIsSubDirBadDes));
	TPtrC entryFullName(NULL,0);
	TPtrC entryParent(NULL,0);
	TInt posFullName=0;
	TInt posParent=0;

	FOREVER
		{
		NextInPath(aParent,entryParent,posParent);
		if (entryParent.Length()==0)
			break;
		NextInPath(aFullName,entryFullName,posFullName);
		if (entryParent!=entryFullName)
			return(EFalse);
		}

	if (aFullName.Length()<=posFullName)
		return(EFalse);
	if (aFullName[posFullName]!=KPathDelimiter)
		return(EFalse);
	return(ETrue);
	}

CFileCB* TDrive::LocateFileByPath(const TDesC& aPath)
//
// Locate a file opened in a subdirectory of aPath
//
	{

	TDblQueIter<CFileCB> q(CurrentMount().iMountQ);
	CFileCB* pF;
	while ((pF=q++)!=NULL)
		{
		if (IsSubDir(pF->FileName(),aPath))
			return(pF);
		}
	return(NULL);
	}

void TDrive::FlushCachedFileInfoL()
//
// Flush data stored in the file control blocks
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TDblQueIter<CFileCB> q(CurrentMount().iMountQ);
	CFileCB* pF;
	while ((pF=q++)!=NULL)
		{
		if (pF->iAtt&KEntryAttModified)
			pF->FlushAllL();
		}
	}

/**
Flushes (asynchronously) all dirty data on this drive and optionally
purges non-dirty data

aPurgeCache - purges all file caches on this drive AFTER dirty data has ben flushed 

returns KErrNone if complete
		CFsRequest::EReqActionBusy if flushing is in progress
		otherwise one of the other system-wide error codes.
*/
TInt TDrive::FlushCachedFileInfo(TBool aPurgeCache)
	{
	if (iCurrentMount == NULL)
		return KErrNone;

	TBool driveThread = FsThreadManager::IsDriveThread(iDriveNumber,EFalse);

	Lock();
	

	TInt ret = KErrNone;

	TDblQueIter<CFileCB> q(iCurrentMount->iMountQ);
	CFileCB* pF;
	while ((pF=q++)!=NULL)
		{
		CFileCache* fileCache = pF->FileCache();

		// Write dirty data if there is a file cache
		TInt flushDirtyRetCode = CFsRequest::EReqActionComplete;
		if (fileCache)
			{
			flushDirtyRetCode = fileCache->FlushDirty();
			if (flushDirtyRetCode == CFsRequest::EReqActionComplete)	// nothing to flush
				{
				if (aPurgeCache)
					fileCache->Purge(EFalse);
				}
			else if (flushDirtyRetCode == CFsRequest::EReqActionBusy)	// flushing
				{
				ret = flushDirtyRetCode;
				}
			else	// error
				{
				ret = flushDirtyRetCode;
				break;
				}
			}
		// if no file cache or no dirty data left, update the file entry & attributes
		if (driveThread && (pF->iAtt&KEntryAttModified) && flushDirtyRetCode == CFsRequest::EReqActionComplete )
			{
			TRAP(ret, pF->FlushAllL());
			if (ret != KErrNone)
				break;
			}
		}

	UnLock();


	return ret;
	}

//----------------------------------------------------------------------------
/**
    Purge dirty cache data associated with all files on a given mount
*/
void TDrive::PurgeDirty(CMountCB& aMount)
	{
	TDblQueIter<CFileCB> q(aMount.iMountQ);
	CFileCB* pF;
	while ((pF=q++)!=NULL)
		{
		CFileCache* fileCache = pF->FileCache();
		if (fileCache)
		    {
        	fileCache->Purge(ETrue);
            fileCache->MarkFileClean();
            }
		}
	}

//----------------------------------------------------------------------------
TInt TDrive::ValidateShare(CFileCB& aFile, TShare aReqShare)
//
// Check that the sharing rules are obeyed.
//
	{
	// Check the correct share modes are passed in
	switch (aReqShare)
		{
	case EFileShareExclusive:
	case EFileShareReadersOnly:
	case EFileShareAny:
	case EFileShareReadersOrWriters:
		break;
	default:
		return KErrArgument;
		}
	
	// Check the share mode of the file
	switch (aFile.iShare)
		{
	case EFileShareExclusive:
		return KErrInUse;

	case EFileShareReadersOnly:
	case EFileShareAny:
		if (aReqShare != aFile.iShare && aReqShare != EFileShareReadersOrWriters)
		    {
            return KErrInUse;
		    }
		break;

	case EFileShareReadersOrWriters:
		if (aReqShare == EFileShareExclusive)
		    {
            return KErrInUse;
		    }
		//
		// If the file is currently opened as EFileShareReadersOrWriters then
		// promote the share to the requested share mode.
		// 
		// If the requested share is EFileShareReadersOnly, verify that no
		// other share has the file opened for writing.
		//
		if (aReqShare == EFileShareReadersOnly)
			{
			TDblQueIter<CFileShare> fileShareIter(aFile.FileShareList());
			CFileShare* pFileShare;
			while ((pFileShare = fileShareIter++) != NULL)
				{
				if(pFileShare->iMode & EFileWrite)
					{
					return KErrInUse;
					}
				}
			}
		break;
    
	default:
		Fault(EDrvIllegalShareValue);
        break;
		}

	return KErrNone;
	}
void TDrive::DriveInfo(TDriveInfo& anInfo)
//
// Get the drive info.
//
	{
	anInfo.iType=EMediaNotPresent;
	anInfo.iMediaAtt=0;
	anInfo.iBattery=EBatNotSupported;
    anInfo.iConnectionBusType=EConnectionBusInternal;

	if(iFSys)
		{
		OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMDRIVEINFO, "this %x aDriveNumber %d", (TUint) &FSys(), (TUint) DriveNumber());
		FSys().DriveInfo(anInfo,DriveNumber());
		OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMDRIVEINFORET, "type %d driveAtt %x mediaAtt %x", (TUint) anInfo.iType, (TUint) anInfo.iDriveAtt, (TUint) anInfo.iMediaAtt);
		}

	anInfo.iDriveAtt=Att();
	}

TInt TDrive::Volume(TVolumeInfo& aVolume)
//
// Get the drive volume info.
//
	{
	TInt r=CheckMount();
	if (r==KErrNone)
		{
		DriveInfo(aVolume.iDrive);
		CMountCB& m=CurrentMount();
		aVolume.iName=m.VolumeName();
		aVolume.iUniqueID=m.iUniqueID;
		aVolume.iSize=m.iSize;

		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBVOLUMEL, "drive %d", DriveNumber());
		TRAP(r,m.VolumeL(aVolume))
		OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBVOLUMELRETA, "r %d iSize %x:%x iFree %x:%x", (TUint) r, (TUint) I64HIGH(aVolume.iSize), (TUint) I64LOW(aVolume.iSize), (TUint) I64HIGH(aVolume.iFree), (TUint) I64LOW(aVolume.iFree));
		OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECMOUNTCBVOLUMELRETB, "iUniqueID %x iFileCacheFlags %x", (TUint) aVolume.iUniqueID, (TUint) aVolume.iFileCacheFlags);
		}
	return(r);
	}


void TDrive::SetVolumeL(const TDesC& aName,HBufC*& aBuf)
//
// Set the volume name.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	aBuf=aName.AllocL();
	TPtr volumeName=aBuf->Des();
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSETVOLUMEL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_EVOLUMENAME, "VolumeName %S", aName.Ptr(), aName.Length()<<1);
	
	CurrentMount().SetVolumeL(volumeName);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSETVOLUMELRET, "r %d", KErrNone);
	delete &CurrentMount().VolumeName();
	CurrentMount().SetVolumeName(aBuf);
	}

TInt TDrive::SetVolume(const TDesC& aName)
//
// Set the volume name.
//
	{
	TInt r=CheckMount();
	HBufC* pV=NULL;
	if (r==KErrNone)
		{
		if(IsWriteProtected())
			return(KErrAccessDenied);
		TRAP(r,SetVolumeL(aName,pV))
		if (r!=KErrNone)
			delete pV;
		}
	return(r);
	}

TInt TDrive::MkDir(const TDesC& aName)
//
// Make a directory.
//
	{
	TInt r=CheckMount();
	if (r!=KErrNone)
		return(r);
	if(IsWriteProtected())
		return(KErrAccessDenied);
	TParse newDirName;
	newDirName.Set(aName,NULL,NULL);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBMKDIRL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBMKDIRLYS_EDIRNAME, "Dir %S", aName.Ptr(), aName.Length()<<1);
	TRAP(r,CurrentMount().MkDirL(newDirName.FullName()))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBMKDIRLRET, "r %d", r);
	return(r);
	}
TInt TDrive::RmDir(const TDesC& aName)
//
// Remove a directory.
//
	{
	TInt r=CheckMount();
	if (r!=KErrNone)
		return(r);
	TEntry entry;
	r=Entry(aName,entry);
	if (r!=KErrNone)
		return(r);
	if (entry.IsDir()==EFalse)
		return(KErrPathNotFound);
	if ((entry.iAtt&KEntryAttReadOnly) || IsWriteProtected())
		return(KErrAccessDenied);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRMDIRL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRMDIRLYS_EDIRNAME, "Dir %S", aName.Ptr(), aName.Length()<<1);
	TRAP(r,CurrentMount().RmDirL(aName))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRMDIRLRET, "r %d", r);
	return(r);
	}

/*
 Delete files allowing wild cards.
*/
TInt TDrive::Delete(const TDesC& aName)
	{
	TInt r=CheckMountAndEntryName(aName);
	if(r!=KErrNone)
		return r;
	
	if(LocateFile(aName))
		return KErrInUse; //-- the file is already opened by someone

	// remove from closed queue - NB this isn't strictly necessary if file is read-only or write-protected...
	LocateClosedFile(aName, EFalse);

    if (IsWriteProtected())
		return(KErrAccessDenied);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL1, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL1_EFILENAME, "FileName %S", aName.Ptr(), aName.Length()<<1);
	TRAP(r,CurrentMount().DeleteL(aName))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDELETEL1_RET, "r %d", r);
	return r;
	}

TInt TDrive::CheckMountAndEntryNames(const TDesC& anOldName,const TDesC& aNewName)
//
// Check mount, that neither is open, and that both names are legal.
//
	{

	TInt r=CheckMountAndEntryName(anOldName);
	if (r!=KErrNone)
		return(r);
	if (IsIllegalFullName(aNewName))
		return(KErrBadName);
	return(KErrNone);
	}

TInt TDrive::CheckDirectories(const TDesC& anOldName,const TDesC& aNewName)
//
// Return KErrAlreadyExists if aNewName exists and 
// KErrAccessDenied if anOldName is a directory being moved to a subdirectory of itself
//
	{

	TEntry entry;
	TInt r=Entry(anOldName,entry);
	if (r!=KErrNone)
		return(r);
	if (entry.IsDir())
		{
	   	//-- check the length of the destination directory name. It shall not exceed 253 characters.
        //-- aNewName looks like "\\dir1" i.e. drive letter and ':' is removed from the name and there is no trailing '\\' in this case. 

       	const TInt maxDirNameLength = KMaxFileName - 3;
        if(aNewName.Length() > maxDirNameLength)
            return KErrBadName;	
		if(IsSubDir(aNewName,anOldName))
			return(KErrInUse); // rename into a subdir of itself
		if (LocateFileByPath(anOldName))
			return(KErrInUse); // a file inside anOldName is open
		}
	else if (LocateFile(anOldName))
		return(KErrInUse);
	
	r=Entry(aNewName,entry);
	if (r!=KErrNone && r!=KErrNotFound)
		return(r);
	return(KErrNone);
	}

TInt TDrive::Rename(const TDesC& anOldName,const TDesC& aNewName)
//
// Rename files or directories. No wild cards.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=CheckMountAndEntryNames(anOldName,aNewName);
	if (r!=KErrNone)
		return(r);
	TPtrC oldEntryName(StripBackSlash(anOldName));
	TPtrC newEntryName(StripBackSlash(aNewName));
	r=CheckDirectories(oldEntryName,newEntryName);
	if (r!=KErrNone)
		return(r);
	if(IsWriteProtected())
		return(KErrAccessDenied);

	// remove from closed queue
	LocateClosedFile(anOldName, EFalse);
	LocateClosedFile(aNewName, EFalse);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRENAMEL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRENAMELYS_EOLDNAME, "OldName %S", oldEntryName.Ptr(), oldEntryName.Length()<<1);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRENAMELYS_ENEWNAME, "NewName %S", newEntryName.Ptr(), newEntryName.Length()<<1);
	TRAP(r,CurrentMount().RenameL(oldEntryName,newEntryName))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRENAMELRET, "r %d", r);
	return(r);
	}

TInt TDrive::Replace(const TDesC& anOldName,const TDesC& aNewName)
//
// Replace anOldName with aNewName atomically. No wild cards. No directories
//
	{
	TInt r=CheckMountAndEntryNames(anOldName,aNewName);
	if (r!=KErrNone)
		return(r);
	TEntry entry;
	r=Entry(aNewName,entry);
	if (r!=KErrNotFound)
		{
		if (r!=KErrNone)
			return(r);
		if (entry.IsDir() || entry.IsReadOnly())
			return(KErrAccessDenied);
		if (LocateFile(aNewName))
			return(KErrInUse);
		}
	r=Entry(anOldName,entry);
	if (r!=KErrNone)
		return(r);
	if (entry.IsDir() || IsWriteProtected())
		return(KErrAccessDenied);
	if (LocateFile(anOldName))
		return(KErrInUse);

	// remove from closed queue
	LocateClosedFile(anOldName, EFalse);
	LocateClosedFile(aNewName, EFalse);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREPLACEL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREPLACEL_EOLDNAME, "OldName %S", anOldName.Ptr(), anOldName.Length()<<1);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREPLACEL_ENEWNAME, "NewName %S", aNewName.Ptr(), aNewName.Length()<<1);
	TRAP(r,CurrentMount().ReplaceL(anOldName,aNewName))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREPLACELRET, "r %d", r);
	return(r);
	}

TInt TDrive::Entry(const TDesC& aName,TEntry& anEntry)
//
// Get the entry details.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=CheckMountAndEntryName(aName);
	if (r!=KErrNone)
		return(r);
	TPtrC entryName(StripBackSlash(aName));
	TRAP(r,DoEntryL(entryName,anEntry));
	
	if (r==KErrHidden)
		r=KErrNotFound;	
	else if (r==KErrPathHidden)
		r=KErrPathNotFound;

	return(r);
	}

void TDrive::DoEntryL(const TDesC& aName, TEntry& anEntry)
//
// Get entry details
//
	{
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBENTRYL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBENTRYL_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);
	CurrentMount().EntryL(aName,anEntry);

	// If the file is already open then read the file attributes directly from the file
	TFileName foldedName;
	TUint32 nameHash=0;
	foldedName.CopyF(aName);
	nameHash=CalcNameHash(foldedName);

	__CHECK_DRIVETHREAD(iDriveNumber);
	TDblQueIter<CFileCB> q(CurrentMount().iMountQ);
	CMountCB* currentMount = &CurrentMount();
	CFileCB* file;
	while ((file=q++)!=NULL)
		{
		if ((&file->Drive()==this) && 
			&file->Mount() == currentMount &&
			nameHash == file->NameHash() && 
			file->FileNameF().Match(foldedName)==KErrNone)
			{
			anEntry.iAtt = file->Att() & ~KEntryAttModified;
			anEntry.SetFileSize(file->CachedSize64());
			anEntry.iModified = file->Modified();
			break;
			}
		}


	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBENTRYLRET, "att %x modified %x:%x  size %x:%x", (TUint) anEntry.iAtt, (TUint) I64HIGH(anEntry.iModified.Int64()), (TUint) I64LOW(anEntry.iModified.Int64()), (TUint) I64HIGH(anEntry.FileSize()), (TUint) anEntry.FileSize());
	}

TInt TDrive::CheckAttributes(TUint& aSetAttMask,TUint& aClearAttMask)
//
// Validate the changes against the current entry attributes
//
	{
	ValidateAtts(aSetAttMask,aClearAttMask);
	return(KErrNone);
	}

TInt TDrive::SetEntry(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
//
// Set the entry details.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=CheckMountAndEntryName(aName);
	if (r!=KErrNone)
		return(r);
	TPtrC entryName(StripBackSlash(aName));
	CFileCB* pF=LocateFile(entryName);
	if (pF!=NULL)
		return(KErrInUse);
	r=CheckAttributes(aSetAttMask,aClearAttMask);
	if (r!=KErrNone)
		return(r);
	if (IsWriteProtected())
		return(KErrAccessDenied);
	TTime nullTime(0);
	if (aTime!=nullTime)
		aSetAttMask|=KEntryAttModified;
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSETENTRYL, "drive %d aTime %x:%x  aSetAttMask %x aClearAttMask %x", (TUint) DriveNumber(), (TUint) I64HIGH(aTime.Int64()), (TUint) I64LOW(aTime.Int64()), (TUint) aSetAttMask, (TUint) aClearAttMask);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSETENTRYL_EFILEPATH, "FilePath %S", aName.Ptr(), aName.Length()<<1);
	TRAP(r,CurrentMount().SetEntryL(entryName,aTime,aSetAttMask,aClearAttMask))
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSETENTRYLRET, "r %d", r);

	// If the file is already open then write the file attributes directly to the file
	TFileName foldedName;
	TUint32 nameHash=0;
	foldedName.CopyF(aName);
	nameHash=CalcNameHash(foldedName);

	__CHECK_DRIVETHREAD(iDriveNumber);
	TDblQueIter<CFileCB> q(CurrentMount().iMountQ);
	CMountCB* currentMount = &CurrentMount();
	CFileCB* file;
	while ((file=q++)!=NULL)
		{
		if ((&file->Drive()==this) && 
			&file->Mount() == currentMount &&
			nameHash == file->NameHash() && 
			file->FileNameF().Match(foldedName)==KErrNone)
			{
			TUint att = file->Att();
			att |= aSetAttMask;
			att &= ~aClearAttMask;
			file->SetAtt(att | KEntryAttModified);
			file->SetModified(aTime);
			break;
			}
		}

	return(r);
	}

TInt TDrive::FileTemp(CFsRequest* aRequest,TInt& aHandle,const TDesC& aPath,TDes& aName,TUint aMode)
//
// Create a temporary file and return the file name.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	aName=aPath;
	TInt len=aName.Length();
	TInt t=User::TickCount()&0xfffff;
	aMode|=EFileWrite;
	for (TInt retry=0;retry<KMaxTempNameAttempts;retry++)
		{
		aName.SetLength(len);
		aName.AppendFormat(_L("TMP%05x.$$$"),t);
		TEntry e;
		TInt r=Entry(aName,e);
		if (r!=KErrNone)
			{
			if (r!=KErrNotFound)
				return(r);
			return(FileOpen(aRequest,aHandle,aName,aMode,EFileCreate));
			}
		t=((t|1)*13)&0xfffff;
		}
	return(KErrGeneral);
	}

LOCAL_C HBufC* CreateFileNameL(const TDesC& aName)
//
// Create a HBufC from aName
// Converts _L("\\F32.\\GROUP\\release.") to _L("\\F32\\GROUP\\release")
//
	{
	
	TParsePtrC name(aName);
	TFileName fileName;
	fileName.Append(KPathDelimiter);
	
	if (name.Path().Length())
		{
		TInt pos=0;
		TPtrC entry(NULL,0);
		FOREVER
			{
			NextInPath(name.Path(),entry,pos);
			if (entry.Length()==0)
				break;
			fileName.Append(entry);
			fileName.Append(KPathDelimiter);
			}
		}

	fileName.Append(name.Name());
	if (name.Ext().Length()>1)
		fileName.Append(name.Ext());
	return(fileName.AllocL());
	} 

void TDrive::FileOpenL(CFsRequest* aRequest,TInt& aHandle,const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB*& aFileCB,CFileShare*& aFileShare)
//
// Open/Create/Replace a file.
//
	{
	aFileCB=NULL;
	aFileShare=NULL;
	TInt r = CheckMount();
	if (r!=KErrNone)
		User::Leave(r);

	if (IsIllegalFullName(aRequest->Src()))
		User::Leave(KErrBadName);

	if (CurrentMount().Locked())
		User::Leave(KErrInUse);

	if ((aMode & EFileWrite) != 0)
		{
		TDriveInfo driveInfo;
		DriveInfo(driveInfo);
		if (driveInfo.iType==EMediaRom || (driveInfo.iMediaAtt&KMediaAttWriteProtected)!=0)
			User::Leave(KErrAccessDenied);
		}

	TShare share=(TShare)(aMode&KFileShareMask);
	if (share==EFileShareReadersOnly && (aMode&EFileWrite)!=0)
		User::Leave(KErrArgument);
	
	if (aMode & EFileReadAsyncAll)
		{
		// Async read all mode is not compatible with EFileShareExclusive or EFileShareReadersOnly,
		// as these modes prevent a writer from accessing the file and completing the request.
		if(share == EFileShareExclusive || share == EFileShareReadersOnly)
			User::Leave(KErrArgument);
		}

	// check for silly cache on / off combinations
	const TUint KBadWriteMode = EFileWriteBuffered | EFileWriteDirectIO;
	const TUint KBadReadMode = EFileReadBuffered | EFileReadDirectIO;
	const TUint KBadReadAheadMode = EFileReadAheadOn | EFileReadAheadOff;
	const TUint KBadReadAheadMode2 = EFileReadDirectIO | EFileReadAheadOn;
	if (((aMode & KBadWriteMode) == KBadWriteMode) ||
		((aMode & KBadReadMode) == KBadReadMode) ||
		((aMode & KBadReadAheadMode) == KBadReadAheadMode) ||
		((aMode & KBadReadAheadMode2) == KBadReadAheadMode2))
		{
		User::Leave(KErrArgument);
		}

	// Only allow delete on close for a newly created file.
	if ((aMode & EDeleteOnClose) && (anOpen!=EFileCreate))
		User::Leave(KErrArgument);

	CFileCB* pFile=LocateFile(aName);
	CFileCache* pFileCache = NULL;
	TBool openFile=EFalse;	// True if file is being opened for the first time
	if (pFile!=NULL)		// File is already opened on the drive
		{
		if (pFile->iShare==EFileShareReadersOnly && (aMode&EFileWrite))
			User::Leave(KErrInUse);
		if (anOpen==EFileCreate)
			User::Leave(KErrAlreadyExists);
		TInt r=ValidateShare(*pFile,share);
		if (r!=KErrNone)
			User::Leave(r);
		if ((r=pFile->Open())!=KErrNone)
			User::Leave(r);
		
		aFileCB=pFile;
		pFileCache = pFile->FileCache();
		}
	else
		{
		OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWFILEL, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
        //-- construct CFileCB object, belonging to the corresponding mount
        pFile = aFileCB = CurrentMount().NewFileL();

		OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWFILELRET, "r %d CFileCB %x", (TUint) r, (TUint) pFile);
		TDrive* createdDrive=!aRequest->SubstedDrive() ? this : aRequest->SubstedDrive();

    	HBufC* fileName = CreateFileNameL(aName);

        pFile->InitL(this, createdDrive, fileName);

		pFile->iShare = share;
		pFile->SetSequentialMode(aMode & EFileSequential);
		openFile=ETrue;
		CurrentMount().iMountQ.AddLast(*pFile);
		Files->AddL(pFile,ETrue);
		__PRINT1(_L("TDrive::FileOpenL - CFileCB->IsSequentialMode = %d"), pFile->IsSequentialMode());
		}
	
    CFileShare* pFileShare=aFileShare=new(ELeave) CFileShare(pFile);

	// We need to call CFileCB::PromoteShare immediately after the CFileShare 
	// instance is created since the destructor calls CFileCB::DemoteShare()
	// which checks the share count is non-zero
	pFileShare->iMode=aMode;
	pFile->PromoteShare(pFileShare);

	pFileShare->InitL();
	aFileCB=NULL; 
	FileShares->AddL(pFileShare,ETrue);
	aHandle=aRequest->Session()->Handles().AddL(pFileShare,ETrue);


	if (openFile)
		{
		OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFILEOPENL, "drive %d aMode %x anOpen %d aFile %x", (TUint) DriveNumber(), (TUint) aMode, (TUint) anOpen, (TUint) pFile);
		OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFILEOPENL_EFILENAME, "FileName %S", aName.Ptr(), aName.Length()<<1);
		CurrentMount().FileOpenL(aName,aMode,anOpen,pFile);
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFILEOPENLRET, "r %d", KErrNone);
		// Delete on close may now be safely flagged if required.
		// The file did not exist on the media prior to the
		// CMountCB::FileOpenL() call for the case of a create.
		if ((aMode & EDeleteOnClose) && (anOpen==EFileCreate))
			pFile->SetDeleteOnClose();

		TBool localBufferSuppport = (CurrentMount().LocalBufferSupport(pFile) == KErrNone)?(TBool)ETrue:(TBool)EFalse;
		pFile->SetLocalBufferSupport(localBufferSuppport);
		if (localBufferSuppport)
			{
			// If file exists on closed queue resurrect it or discard it,
			// depending on the file open mode
			pFileCache = LocateClosedFile(aName, anOpen == EFileOpen?(TBool)ETrue:(TBool)EFalse);
			if (pFileCache)
				{
				pFileCache = pFileCache->ReNewL(*pFileShare);	// NB may return NULL if caching not enabled

				OstTraceExt3(TRACE_FILECACHE, FILECACHE_RENEW_FILE_CACHE1, "ReNew FileCache %x File %x drive %u", (TUint) pFileCache, (TUint) &pFileShare->File(), (TUint) DriveNumber());
				OstTraceData(TRACE_FILECACHE, FILECACHE_RENEW_FILE_CACHE2, "Name %S", pFileShare->File().iFileNameF->Ptr(), pFileShare->File().iFileNameF->Length()<<1);
				}
			else
				{
				pFileCache = CFileCache::NewL(*pFileShare);		// NB may return NULL if caching not enabled

				OstTraceExt3(TRACE_FILECACHE, FILECACHE_NEW_FILE_CACHE1, "New FileCache %x FileCB %x drive %u", (TUint) pFileCache, (TUint) &pFileShare->File(), (TUint) DriveNumber());
				OstTraceData(TRACE_FILECACHE, FILECACHE_NEW_FILE_CACHE2, "Name %S", pFileShare->File().iFileNameF->Ptr(), pFileShare->File().iFileNameF->Length()<<1);
				}
			if (pFileCache)
				// Set the cached size to be the same as the uncached size
				pFile->SetCachedSize64(pFile->Size64());
			}
		else
			{
			__CACHE_PRINT(_L("TDrive::FileOpenL(), Local buffers not supported"));
			}
		}

	// Initialize share mode flags
	if (pFileCache != NULL)
		pFileCache->Init(*pFileShare);
	}

TInt TDrive::FileOpen(CFsRequest* aRequest,TInt& aHandle,const TDesC& aName,TUint aMode,TFileOpen anOpen)
//
// Open/Create/Replace a file.
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	CFileCB* pF=NULL;
	CFileShare* pS=NULL;
	aHandle=0;
	TRAPD(r,FileOpenL(aRequest,aHandle,aName,aMode,anOpen,pF,pS));

	// Allow files > 2GB-1 to be opened only if EFileBigFile is specified in iMode
	if (r == KErrNone && pS && ((TUint64)pS->File().Size64() > KMaxLegacyFileSize) && (!(pS->IsFileModeBig())))
		r = KErrTooBig;

	if (r!=KErrNone)
		{
		if (r==KErrHidden)
			r=KErrNotFound;	
		else if (r==KErrPathHidden)
			r=KErrPathNotFound;

		if(pF && !pS)
			pF->Close();
		CheckSubClose(pS,aHandle,aRequest->Session());
		}
	return(r);
	}

void TDrive::DirOpenL(CSessionFs* aSession,TInt& aHandle,const TDesC& aName,TUint anAtt,const TUidType& aUidType,CDirCB*& aDir)
//
// Open a directory listing. Leave on error.
//
	{
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWDIRL, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
    CDirCB* pD = aDir = CurrentMount().NewDirL(); //-- construct CDirCB object, belonging to the corresponding mount

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWDIRLRET, "r %d CDirCB %x", (TUint) KErrNone, (TUint) pD);
	pD->InitL(this);
	// modify resource counter after initialisation to ensure correct cleanup
	AddResource(CurrentMount());
	pD->iAtt=anAtt;
	pD->iUidType=aUidType;
	Dirs->AddL(pD,ETrue);
	aHandle=aSession->Handles().AddL(pD,ETrue);
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDIROPENL, "drive %d aDir %x", (TUint) DriveNumber(), (TUint) pD);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDIROPENL_EDIRNAME, "Dir %S", aName.Ptr(), aName.Length()<<1);
	CurrentMount().DirOpenL(aName,pD);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDIROPENLRET, "r %d", KErrNone);
	}

TInt TDrive::DirOpen(CSessionFs* aSession,TInt& aHandle,const TDesC& aName,TUint anAtt,const TUidType& aUidType)
//
// Open a directory listing.
//
	{
	TInt r=CheckMountAndEntryName(aName);
	if (r!=KErrNone)
		return(r);
	if (CurrentMount().Locked())
		return(KErrInUse);
	CDirCB* pD=NULL;
	aHandle=0;
	TRAP(r,DirOpenL(aSession,aHandle,aName,anAtt,aUidType,pD));
	
	if (r==KErrHidden)
		r=KErrNotFound;	
	else if (r==KErrPathHidden)
		r=KErrPathNotFound;

	if (r!=KErrNone)
		CheckSubClose(pD,aHandle,aSession);
	return(r);
	}


TInt TDrive::ReadFileSection(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
//
//	Starting from aPos, read aLength bytes of a file into a Trg, 
//	regardless of lock state
//
	{
	return ReadFileSection64(aName, aPos, aTrg, aLength, aMessage);
	}


TInt TDrive::ReadFileSection64(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
//
//	Starting from aPos, read aLength bytes of a file into a Trg, 
//	regardless of lock state
//
	{

	// flush dirty data if already open
	CFileCB* file;
	IsFileOpen(aName, file);
		if (file && file->FileCache())
		{
		if (file->FileCache()->FlushDirty() == CFsRequest::EReqActionBusy)
			return CFsRequest::EReqActionBusy;
		}

	__PRINT(_L("TDrive::ReadSection"));
	TInt r=CheckMountAndEntryName(aName);
	if (r!=KErrNone)
		return(r);
	TPtrC entryName(StripBackSlash(aName));
	TRACETHREADID(aMessage);
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREADFILESECTIONL, "drive %d clientThreadId %x aPos %x:%x aLength %d", (TUint) DriveNumber(), (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREADFILESECTIONL_EFILENAME, "FileName %S", aName.Ptr(), aName.Length()<<1);
	TRAP(r,ReadSectionL(entryName,aPos,aTrg,aLength,aMessage));
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBREADFILESECTIONLRET, "r %d", r);
	if (r==KErrHidden)
		r=KErrNotFound;	
	else if (r==KErrPathHidden)
		r=KErrPathNotFound;

	return(r);
	}


void TDrive::ReadSectionL(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
//
//	Starting from aPos, read aLength bytes of a file into a Trg, 
//	regardless of lock state
//
	{
	__PRINT(_L("TDrive::ReadSectionL"));
	
	FlushCachedFileInfoL();
	CurrentMount().ReadSection64L(aName,aPos,aTrg,aLength,aMessage);
	}

/**
    Check the disk's integrity
*/
TInt TDrive::CheckDisk()
	{
	TInt r=CheckMount();
	if (r==KErrNone)
		TRAP(r,FlushCachedFileInfoL());
	if (r==KErrNone)
		{
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCHECKDISK1, "drive %d", DriveNumber());
		r=CurrentMount().CheckDisk();
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCHECKDISK1RET, "r %d", r);
		}
	return(r);
	}

/**
    @prototype
*/
TInt TDrive::CheckDisk(TInt aOperation, TAny* aParam1/*=NULL*/, TAny* aParam2/*=NULL*/)
    {
	TInt r=CheckMount();
	if (r==KErrNone)
		TRAP(r,FlushCachedFileInfoL());
	if (r==KErrNone)
		{
		OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCHECKDISK2, "drive %d aOperation %d aParam1 %x aParam2 %x", (TUint) DriveNumber(), (TUint) aOperation, (TUint) aParam1, (TUint) aParam2);
		r=CurrentMount().CheckDisk(aOperation, aParam1, aParam2);
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCHECKDISK2RET, "r %d", r);
		}

	return(r);
    }

TInt TDrive::ScanDrive()
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=CheckMount();
	if(r==KErrNone)
		{
		if(IsWriteProtected())
			return(KErrAccessDenied);
		TRAP(r,FlushCachedFileInfoL());
		}
	if(r!=KErrNone)
		return r;

	// Empty closed file queue
	TClosedFileUtils::Remove(DriveNumber());

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSCANDRIVE1, "drive %d", DriveNumber());
	r = CurrentMount().ScanDrive();
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSCANDRIVE1RET, "r %d", r);
	return r;
	}


/**
    @prototype
*/
TInt TDrive::ScanDrive(TInt aOperation, TAny* aParam1/*=NULL*/, TAny* aParam2/*=NULL*/)
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=CheckMount();
	if(r==KErrNone)
		{
		if(IsWriteProtected())
			return(KErrAccessDenied);
		TRAP(r,FlushCachedFileInfoL());
		}
	if(r!=KErrNone)
		return r;

	// Empty closed file queue
	TClosedFileUtils::Remove(DriveNumber());

	OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSCANDRIVE2, "drive %d aOperation %d aParam1 %x aParam2 %x", (TUint) DriveNumber(), (TUint) aOperation, (TUint) aParam1, (TUint) aParam2);
	r = CurrentMount().ScanDrive(aOperation, aParam1, aParam2);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBSCANDRIVE2RET, "r %d", r);
	return r;
	}


TInt TDrive::GetShortName(const TDesC& aName,TDes& aShortName)
//
// Get the short name associated with a long file name
//
	{
	TInt r=CheckMountAndEntryName(aName);
	if (r!=KErrNone)
		return(r);
	TPtrC entryName(StripBackSlash(aName));
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETSHORTNAMEL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETSHORTNAMEL_ELONGNAME, "LongName %S", entryName.Ptr(), entryName.Length()<<1);
	TRAP(r,CurrentMount().GetShortNameL(entryName,aShortName));
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETSHORTNAMEL_ESHORTNAME, "ShortName %S", aShortName.Ptr(), aShortName.Length()<<1);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETSHORTNAMEL_ECMOUNTCBGETSHORTNAMELRET, "r %d", r);
	return(r);
	}

TInt TDrive::GetLongName(const TDesC& aShortName,TDes& aLongName)
//
// Get the long name associated with a short file name
//
	{
	TInt r=CheckMountAndEntryName(aShortName);
	if (r!=KErrNone)
		return(r);
	TPtrC entryName(StripBackSlash(aShortName));
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETLONGNAMEL, "drive %d", DriveNumber());
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETLONGNAMEL_ESHORTNAME, "ShortName %S", entryName.Ptr(), entryName.Length()<<1);
	TRAP(r,CurrentMount().GetLongNameL(entryName,aLongName));
	OstTraceData(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETLONGNAMEL_ELONGNAME, "LongName %S", aLongName.Ptr(), aLongName.Length()<<1);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETLONGNAMELRET, "r %d", r);
	return(r);
	}


/**
    Query whether the file is open or not.
*/
TInt TDrive::IsFileOpen(const TDesC& aFileName,CFileCB*& aFileCB)
	{
	__CHECK_DRIVETHREAD(iDriveNumber);

	aFileCB = NULL;
	
	TInt r=CheckMountAndEntryName(aFileName);
	if (r!=KErrNone)
		return(r);
    
	Files->Lock();
	TInt count=Files->Count();

	// create a hash to speed up the search

	TFileName foldedName;
	TUint32 nameHash=0;
	if (count > 0)
		{
		foldedName.CopyF(aFileName);
		nameHash=CalcNameHash(foldedName);
		}

	while(count--)
		{
		CFileCB* file=(CFileCB*)(*Files)[count];

		if ((&file->Drive()==this) && nameHash == file->NameHash() && file->FileNameF().Match(foldedName)!=KErrNotFound)
			{
			aFileCB = file;
			break;
			}
		}
	Files->Unlock();
	return(KErrNone);
	}

TInt TDrive::IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart)
//
// Return the start of the file if it is in rom
//
	{
	TInt r=CheckMount();
	if (r==KErrNone)
		CurrentMount().IsFileInRom(aFileName,aFileStart);
	return(r);
	}

TBool TDrive::IsWriteProtected()
//
// return true if the media is write protected
//
	{
//	__CHECK_DRIVETHREAD(iDriveNumber);
	TDriveInfo drvInfo;
	drvInfo.iMediaAtt=0;
	if(Att() && iFSys)
		FSys().DriveInfo(drvInfo,DriveNumber());
	return((drvInfo.iMediaAtt&KMediaAttWriteProtected)!=0);
	}




/**
Checks whether any resource that could write to disk is open on
the current mount.

@return True, if a resource that could write to disk is open on
        the current mount, false otherwise.
*/
EXPORT_C TBool TDrive::IsWriteableResource() const
	{
//	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iCurrentMount==NULL)
		return(EFalse);
	if(iCurrentMount->LockStatus()>0)
		{
		// check format subsessions
		Formats->Lock();
		TInt count=Formats->Count();
		while(count--)
			{
			CFormatCB* format=(CFormatCB*)(*Formats)[count];
			if(&format->Mount()==iCurrentMount)
				{
				Formats->Unlock();
				return(ETrue);
				}
			}
		Formats->Unlock();
		// check raw disk subsessions
		RawDisks->Lock();
		count=RawDisks->Count();
		while(count--)
			{
			CRawDiskCB* rawDisk=(CRawDiskCB*)(*RawDisks)[count];
			if(&rawDisk->Mount()==iCurrentMount && !rawDisk->IsWriteProtected())
				{
				Formats->Unlock();
				return(ETrue);
				}
			}
		Formats->Unlock();
		}
	else if(iCurrentMount->LockStatus()<0)
		{
		// check file share subsessions
		FileShares->Lock();
		TInt count=FileShares->Count();
		while(count--)
			{
			CFileShare* fileShare=(CFileShare*)(*FileShares)[count];
			if (&fileShare->File().Mount()==iCurrentMount && ((fileShare->iMode&EFileWrite)!=0))
				{
				FileShares->Unlock();
				return(ETrue);
				}
			}
		FileShares->Unlock();
		}
	return(EFalse);
	}




/**
Tests whether the current function can cause a write to disk.

@return True, if the current function can cause a write to disk,
        false otherwise.
*/
EXPORT_C TBool TDrive::IsCurrentWriteFunction() const
	{
//	__CHECK_DRIVETHREAD(iDriveNumber);
	CDriveThread* pT=NULL;
	TInt r=FsThreadManager::GetDriveThread(iDriveNumber, &pT);
	__ASSERT_ALWAYS(r==KErrNone && pT,Fault(EDriveCurrentWriteFunction));
	return(pT->IsRequestWriteable());
	}




TInt TDrive::ForceRemountDrive(const TDesC8* aMountInfo,TInt aMountInfoMessageHandle,TUint aFlags)
//
// Force a remount of the drive
//
	{
	__PRINT(_L("TDrive::ForceRemountDrive"));
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iFSys==NULL)
		return(KErrNotReady);
	TInt r;
	CMountCB* pM=NULL;
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL1, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
	TRAP(r,pM=FSys().NewMountL());
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL1RET, "r %d CMountCB %x", (TUint) r, (TUint) pM);
	if(r!=KErrNone)
		return(r);
	pM->SetDrive(this);
	OstTraceExt4(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFORCEREMOUNTDRIVE, "drive %d aMountInfo %x aMountInfoMessageHandle %x aFlags %x", (TUint) DriveNumber(), (TUint) aMountInfo, (TUint) aMountInfoMessageHandle, (TUint) aFlags);
	r=pM->ForceRemountDrive(aMountInfo,aMountInfoMessageHandle,aFlags);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBFORCEREMOUNTDRIVERET, "r %d", r);
	pM->Close();
	return(r);
	}
TBool TDrive::IsExtensionMounted(CProxyDriveFactory* aFactory)
//
// return ETrue if extension mounted on the drive
//
	{
	for(TInt i=0;i<iExtInfo.iCount;++i)
		{
		if(iExtInfo.iInfo[i].iFactory==aFactory)
			return(ETrue);
		}
	return(EFalse);
	}
TInt TDrive::MountExtension(CProxyDriveFactory* aFactory,TBool aIsPrimary)
//
// Mount an extension
//
	{
	__PRINT1(_L("TDrive::MountExtension aIsPrimary=%d"),aIsPrimary);
	if(aIsPrimary)
		{
		__CHECK_MAINTHREAD();
		// primary extension mounted before file system since it must be present
		// for successful mount
		__ASSERT_ALWAYS(!iFSys,Fault(EMountExtensionFSys));
		if(iExtInfo.iCount!=0)
			return(KErrAccessDenied);
		iExtInfo.iInfo[iExtInfo.iCount].iFactory=aFactory;
		iExtInfo.iInfo[iExtInfo.iCount++].iIsPrimary=ETrue;
		return(KErrNone);
		}
	__CHECK_DRIVETHREAD(iDriveNumber);
	// must be a secondary extension
	if(iFSys==NULL)
		return(KErrNotReady);
	TBool extSupported = iFSys->IsExtensionSupported();
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMISEXTENSIONSUPPORTED1, "%x r %d", (TUint) iFSys, (TUint) extSupported);
	if(!extSupported)
		return(KErrNotSupported);
	if(IsExtensionMounted(aFactory))
		return(KErrAlreadyExists);
	if(iCurrentMount && (CurrentMount().LockStatus()!=0 || Mount().Count()>1))
		return(KErrInUse);
	if(iExtInfo.iCount>=KMaxExtensionCount)
		return(KErrAccessDenied);
	iExtInfo.iInfo[iExtInfo.iCount].iFactory=aFactory;
	iExtInfo.iInfo[iExtInfo.iCount++].iIsPrimary=EFalse;
	// now dismount and mount so that the extension incorporated
	Dismount();
	TInt r=CheckMount();
	// if mount fails then remove the secondary extension
	if(r!=KErrNone)
		{
		--iExtInfo.iCount;
		__ASSERT_DEBUG(iExtInfo.iCount>=0,Fault(EExtensionInfoCount0));
		}
	return(r);
	}

TInt TDrive::DismountExtension(CProxyDriveFactory* aFactory, TBool /*aIsPrimary*/)
//
// Dismount an extension
//
	{
	 __PRINT(_L("TDrive::DismountExtension"));
	 __CHECK_DRIVETHREAD(iDriveNumber);

	// Empty closed file queue
	TClosedFileUtils::Remove(DriveNumber());

	if(iExtInfo.iCount==0)
		return(KErrNotFound);
	if(iCurrentMount && (CurrentMount().LockStatus()!=0 || Mount().Count()>1))
		return(KErrInUse);
	for(TInt i=0;i<iExtInfo.iCount;++i)
		{
		if(iExtInfo.iInfo[i].iFactory==aFactory)
			{
			// cannot dismount a primary extension without dismounting the file system
			if(i==0 && iExtInfo.iInfo[i].iIsPrimary)
				return(KErrAccessDenied);
			// slide any remaining extensions down
			for(TInt j=i+1;j<iExtInfo.iCount;++j)
				iExtInfo.iInfo[j-1].iFactory=iExtInfo.iInfo[j].iFactory;
			iExtInfo.iCount--;
			__ASSERT_DEBUG(iExtInfo.iCount>=0,Fault(EExtensionInfoCount1));
			Dismount();
			return(KErrNone);
			}
		}
	return(KErrNotFound);
	}

TInt TDrive::ExtensionName(TDes& aExtensionName,TInt aPos)
//
// Return the extension name
//
	{
	__CHECK_DRIVETHREAD(iDriveNumber);

	if(iFSys==NULL)
		return(KErrNotReady);

	if(aPos<iExtInfo.iCount)
		{
		aExtensionName=iExtInfo.iInfo[aPos].iFactory->Name();
		return(KErrNone);
		}
	return(KErrNotFound);
	}

#if defined(_LOCKABLE_MEDIA)
	
TInt TDrive::LockDevice(TMediaPassword& aOld,TMediaPassword& aNew,TBool aStore)
//
// Lock media device
//
	{
	__PRINT(_L("TDrive::LockDevice"));
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iFSys==NULL)
		return(KErrNotReady);
	TInt r;
	CMountCB* pM=NULL;
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL2, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
	TRAP(r,pM=FSys().NewMountL());
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL2RET, "r %d CMountCB %x", (TUint) r, (TUint) pM);
	if(r!=KErrNone)
		return(r);
	pM->SetDrive(this);

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECMOUNTCBLOCK, "drive %d aStore %d", (TUint) DriveNumber(), (TUint) aStore);
	r=pM->Lock(aOld,aNew,aStore);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBLOCKRET, "r %d", r);
	pM->Close();
	return(r);
	}

TInt TDrive::UnlockDevice(TMediaPassword& aPassword,TBool aStore)
//
// Unlock media device
//
	{
	__PRINT(_L("TDrive::UnlockDevice"));
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iFSys==NULL)
		return(KErrNotReady);
	TInt r;
	CMountCB* pM=NULL;
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL3, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
	TRAP(r,pM=FSys().NewMountL());
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL3RET, "r %d CMountCB %x", (TUint) r, (TUint) pM);
	if(r!=KErrNone)
		return(r);

	// reset mount failure count - which is likely to be non-zero if drive is locked
	iMountFailures = 0;

	pM->SetDrive(this);

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECMOUNTCBUNLOCK, "drive %d aStore %d", (TUint) DriveNumber(), (TUint) aStore);
	r=pM->Unlock(aPassword,aStore);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBUNLOCKRET, "r %d", r);
	pM->Close();
	return(r);
	}

TInt TDrive::ClearDevicePassword(TMediaPassword& aPassword)
//
// Clear password of media device
//
	{
	__PRINT(_L("TDrive::ClearDevicePassword"));
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iFSys==NULL)
		return(KErrNotReady);
	TInt r;
	CMountCB* pM=NULL;
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL4, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
	TRAP(r,pM=FSys().NewMountL());
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL4RET, "r %d CMountCB %x", (TUint) r, (TUint) pM);
	if(r!=KErrNone)
		return(r);
	pM->SetDrive(this);

	// ClearPassword() will only work if the card is already unlocked. 
	// If the stack powers down, the card will become locked, so now that TBusLocalDrive::Caps()
	// no longer powers up ths stack, we need to unlock the card first - but ignore the error as 
	// the stack may unlock from the password store.
	TDriveInfo info;
	DriveInfo(info);
	if (info.iMediaAtt & KMediaAttLocked)
		UnlockDevice(aPassword, EFalse);

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCLEARPASSWORD, "drive %d", DriveNumber());
	r=pM->ClearPassword(aPassword);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCLEARPASSWORDRET, "r %d", r);
	pM->Close();
	return(r);
	}

TInt TDrive::EraseDevicePassword()
//
// Erase password from the media device
//
	{
	__PRINT(_L("TDrive::EraseDevicePassword"));
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iFSys==NULL)
		return(KErrNotReady);
	TInt r;
	CMountCB* pM=NULL;
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL5, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber());
	TRAP(r,pM=FSys().NewMountL());
	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWMOUNTL5RET, "r %d CMountCB %x", (TUint) r, (TUint) pM);
	if(r!=KErrNone)
		return(r);
	pM->SetDrive(this);

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBERASEPASSWORD, "drive %d", DriveNumber());
	r=pM->ErasePassword();
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBERASEPASSWORDRET, "r %d", r);
	pM->Close();
	return(r);
	}

#else

TInt TDrive::LockDevice(TMediaPassword& /*aOld*/,TMediaPassword& /*aNew*/,TBool /*aStore*/)
//
// Lock media device
//
	{
	return(KErrNotSupported);
	}

TInt TDrive::UnlockDevice(TMediaPassword& /*aPassword*/,TBool /*aStore*/)
//
// Unlock media device
//
	{
	return(KErrNotSupported);
	}

TInt TDrive::ClearDevicePassword(TMediaPassword& /*aPassword*/)
//
// Clear password of media device
//
	{
	return(KErrNotSupported);
	}

TInt TDrive::EraseDevicePassword(TMediaPassword& /*aPassword*/)
//
// Clear password of media device
//
	{
	return(KErrNotSupported);
	}

#endif



	
/**
Gets the current notification state, which indicates whether the client
is notified of any read or write failures.

The notification status is a property of the current session with
the file server, the value of which is stored in CSessionFs::iNotifyUser.
If set to ETrue, the client will receive notifications from the file system.

Called by CMountCB::GetNotifyUser().

@return True, if the client receives notifications from the file system,
        false otherwise.

@see CMountCB
*/	
EXPORT_C TBool TDrive::GetNotifyUser()
	{
	__CHECK_DRIVETHREAD(iDriveNumber);
	if(iDriveFlags & ENotifyOff)
		return(EFalse);
	else
		{
		CDriveThread* pT=NULL;
		
        const TInt r=FsThreadManager::GetDriveThread(iDriveNumber,&pT);
		
        //-- if this drive is synchronous, i.e. all requests are processed in the main FS thread,
        //-- pretend that user notifications are turned off to avoid panic in the assert below.
        //-- for synch. drives pT will always be NULL and it's not possible to obtain CSessionFs by drive number.
        if(r == KErrAccessDenied) 
            return EFalse;
        
		__ASSERT_ALWAYS(r==KErrNone && pT,Fault(EDriveGetNotifyUser));
		return(pT->IsSessionNotifyUser());
		}
	}




/**
    Gracefully dismounts the current mount. This is method is called from outside, so do some finalisation work on mount.
    After calling this function there is no current mount on the drive.
*/
EXPORT_C void TDrive::Dismount()
	{
	__PRINT1(_L("TDrive::Dismount() iCurrentMount:0x%x"),iCurrentMount);

	iMountFailures = 0;
	if (!iCurrentMount)
		return;

    //-- try to do the best flushing file caches
    TRAP_IGNORE(FlushCachedFileInfoL());

    //-- try our best to finalise the mount (the mount can decide to do some job during finalisation, e.g. write some data)
    //-- finalise the mount in RW mode
    TRAP_IGNORE(iCurrentMount->FinaliseMountL(RFs::EFinal_RW));
    
    DoDismount();
	}




/**
    Dismounts the current mount by force.
*/
void TDrive::ForceDismount()
	{
	__PRINT1(_L("TDrive::ForceDismount() iCurrentMount:0x%x"),iCurrentMount);

	iMountFailures = 0;

	if(!iCurrentMount)
		return;
  
	TRAP_IGNORE(FlushCachedFileInfoL());

    //-- try our best to finalise the mount (the mount can decide to do some job during finalisation, e.g. write some data)
    //-- finalise the mount in RW mode. It is possible that the mount is being forcedly dismounted with some files opened on it.
    //-- in this case further attempt to access may will result in successful remountng and re-attaching mount and its files to the media.
    TRAP_IGNORE(iCurrentMount->FinaliseMountL(RFs::EFinal_RW));

    //-- mark the mount as 'Dismounted'; this invalidates all object handles until the mount is successfully "remounted". 
    //-- if there are still some objects opened on this mount, CMountCB::Close() won't destroy it until all objects are closed.
    iCurrentMount->SetDismounted(); 
    
    DoDismount();
	}

/** 
    An internal method. Dismounts and closes a current mount. 
    This method must not involve writing data to the media, because it could have beeen physically changed before.
    Called only from TDrive::CheckMount().
*/
void TDrive::DoDismount()
    {
    __PRINT1(_L("TDrive::DoDismount() iCurrentMount:0x%x"),iCurrentMount);

    iMountFailures = 0;

	if (!iCurrentMount)
		return;

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDISMOUNTED, "drive %d", DriveNumber());
    iCurrentMount->Dismounted();
	OstTrace0(TRACE_FILESYSTEM, FSYS_ECMOUNTCBDISMOUNTEDRET, "");
	iCurrentMount->Close();
	iCurrentMount=NULL;
    }


/**
Return the number of active mounts associated with this drive.
(inactive mounts are those that have been forcibly dismounted)
*/
TInt TDrive::ActiveMounts() const
	{
	TInt activeMounts = 0;

	TInt idx = Mount().Count();
	while(idx--)
		{
		if(((CMountCB*)Mount()[idx])->IsDismounted())
			{
			activeMounts++;
			}
		}

	__PRINT1(_L("TDrive::ActiveMounts = %d"), activeMounts);
	return activeMounts;
	}




/**
Reactivate any disactive mounts on the drive following a dismount.
(inactive mounts are those that have been foribly dismounted)
*/
void TDrive::ReactivateMounts()
	{
	__PRINT(_L("TDrive::ReactivateMounts"));
	
	TInt idx = Mount().Count();
	while(idx--)
		{
		((CMountCB*)Mount()[idx])->SetDismounted(EFalse);
		}
	}




/**
Increments the drive dismount lock.  This defers dismount
of a file system until all clients have notified that it
is safe to do so.

@see RFs::NotifyDismount
*/
void TDrive::DismountLock()
	{ iDismountLock++; }




/**
Decrements the drive dismount lock.  When the lock count
reaches zero, the file system may be unmounted

@see RFs::AllowDismount
@return The new lock count
*/
TInt TDrive::DismountUnlock()
	{ 
	return(--iDismountLock);
	}




/**
Return the state of the dismount lock.
*/
TBool TDrive::DismountLocked() const
	{ return(iDismountLock); }


/**
DismountClientAdded() - 

Increments the count of clients which need to respond to a dismount request - by calling RFs::AllowDismount() -
before the dismount can process

Called when a EFsDismountRegisterClient request is completed by RFs::NotifyDismount(,,EFsDismountNotifyClients)
*/
void TDrive::DismountClientAdded()
	{
    __e32_atomic_add_ord32(&iDismountClientCount, (TUint32) 1);
	}

/**
DismountClientRemoved() - 
Called when a EFsDismountRegisterClient request is deleted. E.g. by RFs::AllowDismount()
*/
void TDrive::DismountClientRemoved()
	{
	ASSERT(iDismountClientCount > 0);
    if ((__e32_atomic_add_ord32(&iDismountClientCount, (TUint32) -1) == 1) &&
		(!FsThreadManager::IsDriveThread(iDriveNumber,EFalse)) &&
		DismountDeferred())
		{
		iDeferredDismountRequest->Dispatch();
		}
	}

TInt TDrive::DismountClientCount()
	{
	return iDismountClientCount;
	}


/*
Dismount the file system if a deferred dismount has been schedulued and there are no waiting clients and no clamps
*/
TInt TDrive::DeferredDismountCheck()
	{
	// Don't dismount if no deferred dismount is scheduled
	if (!DismountDeferred())
		return KErrNone;

	// Don't dismount if clients are waiting
	if (DismountClientCount() > 0)
		return KErrNone;

	// Don't dismount if files are clamped
	TInt clampErr = ClampsOnDrive();
	if (clampErr != 0 && clampErr != KErrNotSupported)
		return KErrNone;

	// Nothing to wait for, so dismount immediately 
	__ASSERT_DEBUG(GetFSys(), Fault(EAllowDismount));
	return DeferredDismount();
	}


/**
Dismount deferred flag - set while waiting for clients to accept the dismount or files to become unclamped
*/
void TDrive::SetDismountDeferred(TBool aPending)
	{
	if(aPending)
		iDriveFlags |= EDismountDeferred;
	else
		iDriveFlags &= ~EDismountDeferred;
	}


TInt TDrive::ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
//
// General purpose test interface - .FSY specific.
//
	{
	TInt r=CheckMount();
	if(r==KErrNone || (r==KErrInUse && iReason==KErrNone))
		{
		TRACETHREADID(aMessage);
		OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCONTROLIO, "drive %d aCommand %d aParam1 %x aParam2 %x clientThreadId %x", (TUint) DriveNumber(), (TUint) aCommand, (TUint) aParam1, (TUint) aParam2, (TUint) threadId);
		r=CurrentMount().ControlIO(aMessage,aCommand,aParam1,aParam2);
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBCONTROLIORET, "r %d", r);
		}
	return(r);
	}




/**
Gets the drive attributes

@return The drive attributes.
*/
EXPORT_C TUint TDrive::Att()
	{
	TUint a=iAtt;
	return(a);
	}

void TDrive::SetAtt(TUint aValue)
//
// set drive attributes
//
	{
	iAtt=aValue;
	}

EXPORT_C TBool TDrive::IsDriveThread() const
//
// Return ETrue if the current thread id is the same as that of the drive's drive thread
//
	{
	return(FsThreadManager::IsDriveThread(iDriveNumber,ETrue));
	}

EXPORT_C TBool TDrive::IsMainThread() const
//
// Reture ETrue if the current thread id is the same as that of the main file server thread
//
	{
	return(FsThreadManager::IsMainThread());
	}

EXPORT_C void TDrive::DriveFault(TBool aDriveError) const
//
//
//
	{
	if(aDriveError)
		::Fault(EFsDriveThreadError);
	else
		::Fault(EFsMainThreadError);
	}

TInt TDrive::ClampFile(const TDesC& aName, TAny* aHandle)
//
// Attempt to clamp file
//
	{
	CMountCB* mount = (CMountCB*)&(CurrentMount());
	TInt driveNo = DriveNumber();
	return(mount->ClampFile(driveNo,aName,aHandle));
	}


TInt TDrive::UnclampFile(CMountCB* aMount, RFileClamp* aHandle)
//
// Attempt to unclamp file
//
	{
	return(aMount->UnclampFile(aHandle));
	}


TInt TDrive::ClampsOnDrive()
//
// Returns the number of clamps on this drive
//
	{
	Lock();
	TInt clamps = IsMounted()?((CMountCB*)&(CurrentMount()))->NoOfClamps():0;	
	UnLock();
	return (clamps);
	}



void TDrive::SetClampFlag(TBool aClamped)
//
//	Indicate if any files are clamped
//
	{
	if(aClamped)
		iDriveFlags |= EClampPresent;
	else
		iDriveFlags &= ~EClampPresent;
	}


TBool TDrive::ClampFlag()
//
// Report if any files are clamped
//
	{ return(iDriveFlags & EClampPresent); }



#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
TInt TDrive::ClearDeferredDismount()
// debug-only function for testing
	{
	FsNotify::HandleDismount(EFsDismountRegisterClient, DriveNumber(), ETrue, KErrNone);
	SetDismountDeferred(EFalse);
	return KErrNone;
	}
#endif
	

TInt TDrive::DismountProxyDrive()
//
// Dismount a proxy drive
//
	{
	 __PRINT(_L("TDrive::DismountProxyDrive"));
	 __CHECK_DRIVETHREAD(iDriveNumber);

	if (!IsProxyDrive(iDriveNumber) || LocalDrives::DriveNumberToLocalDriveNumber(iDriveNumber) == KDriveInvalid)
		return KErrArgument;

	if(iCurrentMount)
		return(KErrInUse);


	// Prevent ALL inactive mounts from EVER being remounted as they MAY (& probably do) point to
	// this proxy-drive which we are about to delete....
	// NB We could try to find out which mounts actually use this particular proxy-drive, but that's 
	// a bit tricky to determine if there are extensions present as CMountCB::ProxyDrive() will only 
	// return the first proxy drive in the chain.
	TInt mCount=Mount().Count();
	TInt u=(Mount().UniqueID()<<16);
	for (TInt i=0;i<mCount;i++)
		{
		CMountCB* pM=(CMountCB*)Mount().At(u|i);
		pM->SetProxyDriveDismounted();
		}

	FsThreadManager::LockDrive(iDriveNumber);
	// Proxy drives are responsible for managing the drive threads...
	FsThreadManager::CloseDrive(iDriveNumber);
	LocalDrives::ClearProxyDriveMapping(iDriveNumber);
	FsThreadManager::UnlockDrive(iDriveNumber);

	return KErrNone;
	}

//----------------------------------------------------------------------------
/**
    a helper method that allows forced unmounting current mount for volume formatting.
*/
TInt TDrive::ForceUnmountFileSystemForFormatting()
    {
    TInt nRes;
    
    //-- check if there are any clamps on this drive
    nRes = ClampsOnDrive();
    if(nRes > 0)
        return KErrInUse;

    //-- purge all dirty data in the files associated with this drive's mount
    CDriveThread* pT=NULL;
    nRes = FsThreadManager::GetDriveThread(DriveNumber(), &pT);
    if(nRes == KErrNone && pT)
        {
        pT->CompleteReadWriteRequests();
        }

    PurgeDirty(CurrentMount());

    //-- 

    ForceDismount();

    return KErrNone;
    }

//----------------------------------------------------------------------------- 
/** 
    Instantiate CFormatCB object for formatting the file ssytem on the given TDrive.
    
    @param  aRequest            file server request object
    @param  aFmtHandle          out: format handle
    @param  aFmtMode            format mode
    @param  apLDFormatInfo      pointer to legacy parameters structure; NULL means "not used"
    @param  apVolFormatParam    pointer to the newparameters structure; NULL means "not used" 

    @return pointer to the instantiated CFormatCB object.
*/
CFormatCB* TDrive::FormatOpenL(CFsRequest* aRequest, TInt& aFmtHandle, TFormatMode aFmtMode, const TLDFormatInfo* apLDFormatInfo, const TVolFormatParam* apVolFormatParam)
    {
    ASSERT(!(apLDFormatInfo && apVolFormatParam));  //-- these parameters are mutually exclusive
    
    OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWFORMATL, "this %x drive %d", (TUint) &FSys(), (TUint) DriveNumber()); 
    CFormatCB* pFormat = CurrentMount().NewFormatL(); 

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWFORMATLRET, "r %d CFormatCB %x", (TUint) KErrNone, (TUint) pFormat); 
	
    Formats->AddL(pFormat, ETrue); 
	pFormat->InitL(this, aFmtMode); 

    if(aFmtMode & ESpecialFormat) 
        {
        if(apLDFormatInfo)
            {//-- the user has specified formatting parameters as TLDFormatInfo
            pFormat->SetFormatParameters(apLDFormatInfo);
            }
        else if(apVolFormatParam && apVolFormatParam->SomeParamsSet())
            {//-- the user has specified formatting parameters as TVolFormatParam
            TInt nRes = pFormat->SetFormatParameters(apVolFormatParam);
            User::LeaveIfError(nRes); //-- the particular file system might not support this feature
            }
        else
            {//-- this is a special case, ESpecialFormat is set, but no parameters provided at all;
             //-- invalidate CFormatCB::iSpecialInfo to make filesystem not to use it
            pFormat->SetFormatParameters((TLDFormatInfo*)NULL);
            }
        }
    

	// modify resource counter after initialised to ensure correct cleanup 
	AddDiskAccess(CurrentMount());	 
	aFmtHandle = aRequest->Session()->Handles().AddL(pFormat, ETrue); 

    return pFormat;
    }





EXPORT_C void UNUSED1() {}
EXPORT_C void UNUSED2() {}
EXPORT_C void UNUSED3() {}









