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
// f32\sfile\sf_fmt.cpp
// 
//

#include "sf_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_fmtTraces.h"
#endif

LOCAL_C CFormatCB* GetFormatFromHandle(TInt aHandle,CSessionFs* aSession)
//
// Get the format control block from aHandle
//
	{
	return((CFormatCB*)(SessionObjectFromHandle(aHandle,Formats->UniqueID(),aSession)));
	}



/**
Default constructor.
*/
EXPORT_C CFormatCB::CFormatCB()
	{
	}




/**
    Destructor.
    Frees resources before destruction of the object.
*/
EXPORT_C CFormatCB::~CFormatCB()
	{

	if (iMount)
		{
		RemoveDiskAccess(*iMount);
		iMount->Drive().SetChanged(ETrue);
		iMount->Close();
		}
	}




/**
    Checks that the disk media is still mounted.
    @return KErrNone if the media is still mounted; KErrDisMounted otherwise.
*/
EXPORT_C TInt CFormatCB::CheckMount()
	{

	TDrive& d=Drive();
	TInt r=d.CheckMount();
	if (r!=KErrNone)
		return(r);
	if (&Mount()!=&d.CurrentMount())
		return(KErrDisMounted);
	return(KErrNone);
	}

void CFormatCB::InitL(TDrive* aDrive,TFormatMode aMode)
	{
	DoInitL(aDrive->DriveNumber());
	iDrive=aDrive;
	iMount=&iDrive->CurrentMount();
	iMode=aMode;
	User::LeaveIfError(iMount->Open());
	}


EXPORT_C TInt CFormatCB::GetInterface(TInt /*aInterfaceId*/,TAny*& /*aInterface*/,TAny* /*aInput*/)
	{
	return(KErrNotSupported);
	}


//----------------------------------------------------------------------------
/** 
    set volume formatting parameters, which are provided in TLDFormatInfo structure 
    @param  apLDFormatInfo  pointer to the parameters structure. If NULL, iSpecialInfo will be initialised
*/
void CFormatCB::SetFormatParameters(const TLDFormatInfo* apLDFormatInfo)
    {
    TLDFormatInfo& fmtInfo = iSpecialInfo();

    if(!apLDFormatInfo)
        {//-- special meaning; invalidate iSpecialInfo by setting its package size as 0
        iSpecialInfo.SetLength(0);
        }
    else
        {
        Mem::Copy(&fmtInfo, apLDFormatInfo, sizeof(TLDFormatInfo));
        }
    }

//----------------------------------------------------------------------------
/** set volume formatting parameters, which are provided in TVolFormatParam structure */
TInt CFormatCB::SetFormatParameters(const TVolFormatParam* apVolFormatParam)
    {
    ASSERT(apVolFormatParam);
    TAny* dummy;
    //-- push parameters to the particular implementation of the CFormatCB. Default behaviour: KErrNotSupported
    return GetInterface(ESetFmtParameters, dummy, (TAny*)apVolFormatParam);
    }

//----------------------------------------------------------------------------
#ifdef _DEBUG
#define DUMP_OPENED_OBJECTS
#endif

/**
    Debug helper method. Dumps names of opened files and directories on this drive
    define DUMP_OPENED_OBJECTS to have it called
*/
#ifdef DUMP_OPENED_OBJECTS
static void DumpOpenedObjects(TDrive& aDrive)
    {
        {//-- 1. files 
        const TInt nFiles = Files->Count();
        for(TInt i=0; i<nFiles; ++i)
            {
            CFileCB* pFile=(CFileCB*)(*Files)[i];
            if(pFile->Drive().DriveNumber() == aDrive.DriveNumber())
                {
                __PRINT1(_L("FsFormatOpen() opened file:'%S'"), &pFile->FileName());
                }
            }
        
        }

        {//-- 2. directories; CDirCB doesn't have associated name.
        const TInt nDirs = Dirs->Count();
        TInt cntDirs = 0;
        for(TInt i=0; i<nDirs; ++i)
            {
            CDirCB* pDir = (CDirCB*)(*Dirs)[i];
            if(pDir->Drive().DriveNumber() == aDrive.DriveNumber())
                {
                ++cntDirs;
                }
            }
            
        if(cntDirs)
            {
            __PRINT1(_L("FsFormatOpen() opened directories:%d"), cntDirs);
            }

        }

    }
#endif //DUMP_OPENED_OBJECTS

//----------------------------------------------------------------------------
/**
    Open a drive for formatting.
*/
TInt FsFormatOpen(CFsRequest* aRequest)
	{
    TDrive& drive = *aRequest->Drive();

	__PRINT1(_L("FsFormatOpen() drv:%d"), drive.DriveNumber());
    
    TInt nMountRes = drive.CheckMount();
    //-- KErrNotReady means that there is no file system mounted on this drive
    //-- KErrInUse means that there are some "disk access" objects, like RFormat or RRawDisk opened on the mount.
    if(nMountRes == KErrNotReady || nMountRes == KErrInUse) 
        {
        __PRINT1(_L("FsFormatOpen() ChkMount:%d"), nMountRes);
        return nMountRes;
        }
    
    const TFormatMode fmtMode = (TFormatMode)aRequest->Message().Int1();
    TName buf;
    TUint32 currFsNameHash = 0; //-- current file system name hash, 0 means "not set"; used during forced FS dismounting

    if((nMountRes == KErrNone) && drive.CurrentMount().LockStatus() < 0)
        {//-- the mount is locked, it has normal objects (files, directories) opened on it. 
        
        //-- if someone is interested in the list of opened files and number of opened directories, compile this code in.
        #ifdef DUMP_OPENED_OBJECTS
            DumpOpenedObjects(drive);
        #endif //DUMP_OPENED_OBJECTS


        if(!(fmtMode & EForceFormat))
            {
            __PRINT(_L("FsFormatOpen() The mount is in use"));
            return KErrInUse;
            }    

        //-- there is a special flag that tells to force media dismounting even if it has files or dirs opened.
        __PRINT(_L("FsFormatOpen() The mount is in use, forcing dismounting!"));

        //-- record currently mounted FS name hash, it may be used after forced dismounting
        drive.CurrentMount().FileSystemName(buf); //-- the iCurrentMount is alive
        currFsNameHash = TVolFormatParam::CalcFSNameHash(buf);

        //-- kill the current mount
        FsThreadManager::LockDrive(drive.DriveNumber());
        TInt nRes = drive.ForceUnmountFileSystemForFormatting();
        FsThreadManager::UnlockDrive(drive.DriveNumber());

        
        switch(nRes)
            {
            case KErrInUse: 
            __PRINT(_L("FsFormatOpen() The mount has clamps! Can't force dismount"));    
            return KErrInUse; //-- there are clamps on this drive - can't dismount

            case KErrNone:
            break;

            default:
            ASSERT(0); //-- unexpected error code
            return nRes;

            };

        if(fmtMode & EQuickFormat)
            {//-- quick format may require the normally mounted FS, make the best effrot to mount it
            nMountRes = drive.CheckMount();
            }
        else
            {//-- this will make the FS mounted by force; for full format it will be quicker
            nMountRes = KErrCorrupt;
            }

        }

	//-- if True, we will need mount (probably specific) file system by force because normal mounting has failed
    TBool bNeedForceMount = (nMountRes != KErrNone); 

    //-- find out if we have optional data structure that describes format parameter
    TUint32 newFsNameHash = 0; //-- file system name hash, may be used for selecting which file system to put onto the volume. 0 means "not specified"

    const TLDFormatInfo*    pLDFormatInfo   = NULL;
    const TVolFormatParam*  pVolFormatParam = NULL;

    __ASSERT_COMPILE(sizeof(TVolFormatParam) >= sizeof(TLDFormatInfo));
    TBuf8<sizeof(TVolFormatParam)> paramBuf;
   
    
    if(fmtMode & ESpecialFormat)  
        {   
        //-- the user has provided format parameters structure.
        //-- IPC argument #2 contains a structure: <TUint32>[optional package descriptor]
        //-- where 1st mandatory TUint32 is a pointer to format counter and the optional additional package is a data structure passed to the filesystem by the client of RFormat
        const TInt desLen = aRequest->GetDesLength(KMsgPtr2);
        ASSERT((TUint32)desLen >= sizeof(TUint32));
    
        const TInt dataPckgLen = desLen - sizeof(TUint32);

        if((TUint32)dataPckgLen > sizeof(TUint32))
            {
            aRequest->ReadL(KMsgPtr2, paramBuf); 
            }
        
        if(dataPckgLen == sizeof(TLDFormatInfo))
            {//-- the user has provided formatting parameters via TLDFormatInfo structure.
            pLDFormatInfo = (const TLDFormatInfo*)(paramBuf.Ptr() + sizeof(TUint32));
            }
        else if(dataPckgLen == sizeof(TVolFormatParam))
            {//-- it's likely to be TVolFormatParam, need to check UId to be sure.
            pVolFormatParam = (const TVolFormatParam*)(const TVolFormatParam*)(paramBuf.Ptr() + sizeof(TUint32));

            if(pVolFormatParam->iUId == TVolFormatParam::KUId)  //-- check the class UID
                {//-- this is the real TVolFormatParam object passed
                newFsNameHash = pVolFormatParam->FSNameHash();
                }
            }
        else if(dataPckgLen >0)
            {//-- parameters data structure has strange length
            return KErrArgument;
            }
    
        }

    //-------------------
    if(!newFsNameHash && currFsNameHash)
        {//-- new file system name isn't specified (default formatting), but the volume had been forcedly dismounted.
         //-- restore the original file system   
        newFsNameHash = currFsNameHash;
        }
    
    if(newFsNameHash)
        {//-- check if the specified FS is already mounted on the volume
        if(!bNeedForceMount)
            {
            drive.CurrentMount().FileSystemName(buf); //-- the iCurrentMount is alive
            }
        else
            { //-- the iCurrentMount can be NULL, use the iFsys - the real file system associated with this drive
            buf = drive.GetFSys()->Name();
            }

        const TUint32 currFSNameHash = TVolFormatParam::CalcFSNameHash(buf);
        if(currFSNameHash == newFsNameHash)
            {//-- no need to do anything, the required FS is already mounted
            newFsNameHash = 0; 
            } 
        }

    if(newFsNameHash) 
        {
        //-- the user has specified some filesystem to be mounted on the volume. Check if this FS is supported at all.
        //-- if it is supported, but some other FS is currently mounted, it will be dismounted and the new one will be forced.
        TInt nRes;
            
        for(TInt cntFS=0; ;++cntFS)
            {
            nRes = drive.FSys().GetSupportedFileSystemName(cntFS, buf); //-- enumerate possible child file systems
            
            if(nRes != KErrNone)
                return KErrNotSupported; //-- the filesystem with the given name (fsNameHash) is not supported.

            if(newFsNameHash == TVolFormatParam::CalcFSNameHash(buf))
                {//-- the filesystem with the given name (fsNameHash) is supported, but some other filesystem can be already mounted
                drive.Dismount();
                bNeedForceMount = ETrue; //-- this will force the desired FS to be mounted
                break;
                }
            }
    
        }//if(fsNameHash) 


    //-- try force mounting the desired file system if it is required
    if(bNeedForceMount)
        {
        const TInt KMaxRetries = 3;
        for(TInt cnt=0; ; ++cnt)
            {
            drive.MountFileSystem(ETrue, newFsNameHash);

            nMountRes = drive.GetReason();
            if(nMountRes == KErrNone || nMountRes == KErrLocked)
                break;
            
            drive.Dismount(); //-- will reset mount retries counter
            
            if(cnt >= KMaxRetries)
                {
                __PRINT1(_L("FsFormatOpen() can't mount FS! res:%d"), nMountRes);    
                return nMountRes;
                }
            }
        }

    ASSERT(nMountRes == KErrNone || nMountRes == KErrLocked);
    
    __ASSERT_DEBUG(drive.CurrentMount().LockStatus()==0, Fault(ESvrFormatOpenFailed));


	TDriveInfo dInfo;
	drive.DriveInfo(dInfo);
	const TInt mediaAtt = dInfo.iMediaAtt;

#if defined(_LOCKABLE_MEDIA)
	if (!(fmtMode & EForceErase) && (mediaAtt & KMediaAttLocked))
		{
		// if attempting to format a locked drive, dismount otherwise subsequent 
		// requests will operate on a mount that has been forcibly mounted (a few lines above)
		CMountCB* pM = &drive.CurrentMount();
		
        if(pM)
			pM->Close();

		drive.MountFileSystem(EFalse);	// clear iCurrentMount
		return KErrLocked;
		}
#endif

	if (!(mediaAtt & KMediaAttFormattable) || (mediaAtt & KMediaAttWriteProtected))
		{
		CMountCB* pM = &drive.CurrentMount();
		
        if(pM)
			pM->Close();

		drive.MountFileSystem(EFalse);
        return KErrAccessDenied;
		}

	//-- instantinate and open CFormatCB object for this drive
    CFormatCB* formatCB=NULL;
	TInt fmtHandle;
    
    TRAPD(ret, formatCB = drive.FormatOpenL(aRequest, fmtHandle, fmtMode, pLDFormatInfo, pVolFormatParam ));

	if (ret!=KErrNone)
		{
		if(formatCB)
			formatCB->Close();

		return ret;
		}

	TPtrC8 pH((TUint8*)&fmtHandle,sizeof(TInt));
	aRequest->WriteL(KMsgPtr3,pH);
	TInt count=100;

	TPtrC8 pCount((TUint8*)&count,sizeof(TInt));
	aRequest->WriteL(KMsgPtr2,pCount);
	aRequest->Session()->IncResourceCount();
	
    return KErrNone;
	}

TInt TFsFormatOpen::DoRequestL(CFsRequest* aRequest)
//
// Open a drive for formatting.
//
	{
	// Can not format if any files are clamped
	TInt r=FsFormatOpen(aRequest);
	return r;
	}

TInt TFsFormatOpen::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r;
	if (!KCapFsFormatOpen.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Format Open")))
		return KErrPermissionDenied;
	r=ParseNoWildSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	if (aRequest->Src().NameOrExtPresent())
		return(KErrBadName);
	if (aRequest->SubstedDrive())
		return(KErrAccessDenied);
	return(r);
	}


TInt TFsFormatNext::DoRequestL(CFsRequest* aRequest)
//
// Format the next part of the media.
//
	{

	__PRINT1(_L("TFsFormatNext::DoRequestL() drv:%d"), aRequest->DriveNumber());
	CFormatCB* format=(CFormatCB*)aRequest->ScratchValue();
	TInt r=format->CheckMount();
	if (r!=KErrNone && r!=KErrInUse)
        {
    	__PRINT1(_L("TFsFormatNext::DoRequestL() err:%d"), r);
        return r;
        }

	TPtr8 pStep((TUint8*)&format->CurrentStep(),sizeof(TInt));
	aRequest->ReadL(KMsgPtr0,pStep);

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFORMATCBDOFORMATSTEPL, "this %x", format);

	TRAP(r,format->DoFormatStepL());

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECFORMATCBDOFORMATSTEPLRET, "r %d  iCurrentStep %d", r, (TUint) format->CurrentStep());

	if (r==KErrNone)
		aRequest->WriteL(KMsgPtr0,pStep);
	if (r==KErrNone && format->CurrentStep()==0)
		{
		FsNotify::DiskChange(aRequest->DriveNumber());
		}
	return(r);
	}

TInt TFsFormatNext::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsFormatNext.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Format Next")))
		return KErrPermissionDenied;
	CFormatCB* format;
	format=GetFormatFromHandle(aRequest->Message().Int3(), aRequest->Session());
	if(!format)
		return(KErrBadHandle);	
	aRequest->SetDrive(&format->Drive());
	aRequest->SetScratchValue((TUint)format);
	return KErrNone;
	}
