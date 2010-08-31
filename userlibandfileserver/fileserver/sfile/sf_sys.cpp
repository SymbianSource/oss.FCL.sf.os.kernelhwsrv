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
// f32\sfile\sf_sys.cpp
// 
//

#include "sf_std.h"
#include <e32uid.h>
#include "sf_file_cache.h"
#include <kernel/localise.h>
#include <f32file.h>

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_sysTraces.h"
#endif

typedef CFileSystem*(*TFileSystemNew)();
extern CProxyDriveFactory* GetExtension(const TDesC& aName);

#ifndef __WINS__
extern TBool gInitCacheCheckDrivesAndAddNotifications;
#endif

GLREF_D TCodePageUtils TheCodePage;

/**
Default constructor.
*/
EXPORT_C CFileSystem::CFileSystem()
	{
	OstTrace0(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMCONSTRUCTOR, "");
	OstTrace0(TRACE_FILESYSTEM,FILESYS_ECFILESYSTEMCONSTRUCTORRETURN, "");
	}

/**
Destructor.
*/
EXPORT_C CFileSystem::~CFileSystem()
	{
	OstTrace0(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMDESTRUCTOR, "");
	OstTrace0(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMDESTRUCTORRETURN, "");
	}

/**
Uninstalls the file system.

This is called just before the file system object is destroyed, and allows
any clean up to be carried out.

The default implementation does nothing except return KErrNone.
Implementations should return an error code on error detection.

@return KErrNone if successful, otherwise one of the other system wide error
        codes.
*/
EXPORT_C TInt CFileSystem::Remove()
	{

	return(KErrNone);
	}

/**
Tests whether a version is supported.

This is done decided by comparing the supplied version with iVersion.

The default implementation uses User::QueryVersionSupported() to
determine this.

@param aVer The version to be tested.

@return True, if aVer is supported; false otherwise

@see User::QueryVersionSupported
@see CFileSystem::iVersion
*/
EXPORT_C TBool CFileSystem::QueryVersionSupported(const TVersion& aVer) const
	{

	return(User::QueryVersionSupported(iVersion,aVer));
	}
	
//#ifndef __DATA_CAGING__
/**
Retrieves the default path for the file system.

Each session with the file server has a current session path.
When a new session is opened, its session path is set to the default path
of the file server.
At file server start-up, this default path is set to the default path returned
by the local file system. 

The function should return an appropriate error code when the default path
cannot be supplied. 

The derived class should override this base class function.

This default implementation raises an "Fserv fault" 31 panic.

@param aPath On return, contains the default path for the file system for derived classes.

@return KErrNone if successful, otherwise one of the other system wide error codes.

@panic Fserv fault 31 if the default implementation
       for CFileSystem::DefaultPath() is not overridden. 
*/
TInt CFileSystem::DefaultPath(TDes& /*aPath*/) const
	{

	Fault(ESysDefaultPathNotSupported);
	return(KErrNone);
	}
//#endif

/**
Sets the file system's resource library.

This library represents the loaded file system.

This is called internally by InstallFileSystem().

@param aLib The resource library to be set.
*/
EXPORT_C void CFileSystem::SetLibrary(RLibrary aLib)
	{

	iLibrary=aLib;
	}

/**
Gets the file system's resource library.

@return The file system's resource library.
*/
EXPORT_C RLibrary CFileSystem::Library() const
	{
	return(iLibrary);
	}

/**
Tests whether the file system supports extensions.

@return True, if the file system supports extensions, false otherwise.
        The defualt implementation returns false.
*/
EXPORT_C TBool CFileSystem::IsExtensionSupported() const
	{
	return(EFalse);
	}

EXPORT_C void CFileSystem::DriveInfo(TDriveInfo& aInfo, TInt aDriveNumber) const
	{
    GetDriveInfo(aInfo, aDriveNumber);
	}


EXPORT_C TInt CFileSystem::GetInterface(TInt /*aInterfaceId*/,TAny*& /*aInterface*/,TAny* /*aInput*/)
	{
	return(KErrNotSupported);
	}

EXPORT_C TBool CFileSystem::IsProxyDriveSupported()
	{
	TAny* dummyInterface;
	if(GetInterface(EProxyDriveSupport, dummyInterface, NULL) == KErrNone)
		return ETrue;
	
	return EFalse;
	}

//----------------------------------------------------------------------------- 
/** 
    Extended CMountCB factory interface.
    Produces the CMountCB object which can be associated with another CFileSystem owner.
    Used mostly  with "automounter" file system

    @param  apDrive         in:  pointer to TDrive, producing right CMountCB can require media access ("automounter" recognising the file system)
    @param  apFileSystem    out: pointer to the CFileSystem object that actually produced CMountCB instance (might be different from "this")
    @param  aForceMount     in:  ETrue if it is necessarily to force mounting (formatting the media, for example)
    @param  aFsNameHash     in:  desired file system name hash (optional). Specifies which file system will be used to produce appropriate CMountCB object.
                                 The file system that implements NewMountExL() shall decide how to process it. 0 means "default/not specified".

    @return pointer to the instantiated CMountCB object.
*/
CMountCB* CFileSystem::NewMountExL(TDrive* apDrive, CFileSystem** apFileSystem, TBool aForceMount, TUint32 aFsNameHash)
    {
    TAny* pa;

    if(GetInterface(EExtendedFunctionality, pa, NULL) == KErrNone)
        {//-- special interface for the case, when CMountCB object will be produced by not _this_ CFileSystem object, but some different.
         //-- in this case apFileSystem will contain a pointer to the real factory.
        MFileSystemExtInterface* pExtIf = (CFileSystem::MFileSystemExtInterface*)pa;
        ASSERT(pExtIf);
        
        return pExtIf->NewMountExL(apDrive, apFileSystem, aForceMount, aFsNameHash);
        }
    else
        {//--This interface is not supported by current CFileSystem implementation, call normal legacy factory method
         //-- and make _this_ object of CFileSystem produce a new CMountCB 
            ASSERT(aFsNameHash == 0); //-- it is impossible to specify the particular FS to be used
            *apFileSystem = this; 
            return NewMountL();
        }
        
    }


//----------------------------------------------------------------------------- 
/** 
    Get the name of a filesystem from the list of supported on this drive.
    Some filesystems (e.g. "automounter" can support more than one real "child" filesystems.
    For the normal case, only one filesystem is supported (a mouned one).

    @param  aFsNumber   used to enumerate supported filesystems can be:
                        special value KRootFileSystem, or
                        0,1,2... - the sequence number of a "child" FS.
    
    @param  aFsName     out: buffer for the returned file system name

    @return KErrNone        Ok, aFsName contains valid value for the given aFsNumber
            KErrNotFound    There is no supported filesystem for the given aFsNumber
*/
TInt CFileSystem::GetSupportedFileSystemName(TInt aFsNumber, TDes& aFsName) 
    {
    TAny* pa;

    //-- we need a special interface to find out the name of the supported file system number "aFsNumber"
    if(GetInterface(EExtendedFunctionality, pa, NULL) == KErrNone)
        {
        MFileSystemExtInterface* pExtIf = (CFileSystem::MFileSystemExtInterface*)pa;
        ASSERT(pExtIf);
        return pExtIf->GetSupportedFileSystemName(aFsNumber, aFsName);   
        }
    else
        {//--This interface is not supported by current CFileSystem implementation, but in this case "Root" and first "child" filesystem mean
         //-- the same and this is "this" filesystem
            
            if(aFsNumber == RFs::KRootFileSystem || aFsNumber == RFs::KFirstChildFileSystem)
                {
                aFsName = Name();
                return KErrNone;               
                }
            else
                {
                return KErrNotFound;
                }
        }
    }

//----------------------------------------------------------------------------- 

TInt InstallFileSystem(CFileSystem* aSys,RLibrary aLib)
//
// Install a file system.
//
	{

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMINSTALL, "this %x", aSys);

	TInt r=aSys->Install();

	OstTraceData(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMINSTALLYS_EFILESYSTEMNAME, "FileSystemName %S", aSys->Name().Ptr(), aSys->Name().Length()<<1);
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMINSTALLRET, "r %d", r);

	__PRINT1TEMP(_L("InstallFileSystem %S"),aSys->Name());
	if (r==KErrNone)
		{TRAP(r,FileSystems->AddL(aSys,ETrue))}
	if (r!=KErrNone)
		{
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMREMOVE1, "this %x", aSys);
#ifdef OST_TRACE_COMPILER_IN_USE
		TInt r = 
#endif
			aSys->Remove();
		
		OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMREMOVE1RET, "r %d", r);
		}
	if (r==KErrNone)
		aSys->SetLibrary(aLib);
	else
		aSys->Close();
	return(r);
	}

EXPORT_C CFileSystem* GetFileSystem(const TDesC& aName)
//
// Lookup a file system by name.
//
	{

	TInt h=0;
	TInt r=FileSystems->FindByName(h,aName);
	if (r!=KErrNone)
		return(NULL);
	return((CFileSystem*)FileSystems->At(h));
	}

TInt TFsAddFileSystem::DoRequestL(CFsRequest* aRequest)
//
// Add a file system.
//
	{

	__PRINT(_L("TFsAddFileSystem::DoRequestL(CFsRequest* aRequest)"));
	
	RLibrary lib;
	lib.SetHandle(aRequest->Message().Int0()); // Get library handle
	if (lib.Type()[1]!=TUid::Uid(KFileSystemUidValue))
		return KErrNotSupported;

	TFileSystemNew f=(TFileSystemNew)lib.Lookup(1);
	if (!f)
		return KErrCorrupt;
	
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEW, "handle %x", lib.Handle());
	CFileSystem* pS=(*f)();
	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMNEWRET, "fileSystem %x", pS);
	if (!pS)
		return KErrNoMemory;
	TInt r=InstallFileSystem(pS,lib);
	if (r==KErrNone && !LocalFileSystemInitialized)
		{
		_LIT(KLocFSY, "ELOCAL.FSY");
		TFileName fn(lib.FileName());
		TParsePtrC ppc(fn);

		if (ppc.NameAndExt().CompareF(KLocFSY) == 0)
			r = InitializeLocalFileSystem(pS->Name());
		}
	return r;
	}

TInt TFsAddFileSystem::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TSecurityPolicy policy(RProcess().SecureId(), ECapabilityTCB);
	if (!policy.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Add File System")))
		return KErrPermissionDenied;
	
	return KErrNone;
	}

TInt TFsRemoveFileSystem::DoRequestL(CFsRequest* aRequest)
//
// Remove a file system.
//
	{

	TFullName name;
	aRequest->ReadL(KMsgPtr0,name);
	CFileSystem* pF=GetFileSystem(name);
	if (pF==NULL)
		return(KErrNotFound);

	CFileSystem* pFs = NULL;
	for(TInt drvNum=0; drvNum<KMaxDrives; drvNum++)
		{
		FsThreadManager::LockDrive(drvNum);
		pFs=TheDrives[drvNum].GetFSys();
		FsThreadManager::UnlockDrive(drvNum);
		if(!pFs)
			continue;
		
        if(name.CompareF(pFs->Name()) == 0)
			return KErrInUse;
		}
	
    OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMREMOVE2, "this %x", pF);

	TInt r=pF->Remove();

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECFILESYSTEMREMOVE2RET, "r %d", r);

	if (r!=KErrNone)
		return(r);
	
    RLibrary lib=pF->Library();
	pF->Close();
	lib.Close();

    return KErrNone;
	}

TInt TFsRemoveFileSystem::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsRemoveFileSystem.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Remove File System")))
		return KErrPermissionDenied;

    //-- check file system name length. It should not exceed KMaxFSNameLength (32 characters)
    TInt r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;
    
	return KErrNone;
	}

LOCAL_C TInt DoMountFileSystem(CFsRequest* aRequest)
//
//
//
	{
	TInt r = TFileCacheSettings::ReadPropertiesFile(aRequest->Drive()->DriveNumber());
	if (r != KErrNone)
		return r;

	return(aRequest->Drive()->CheckMount());
	}

//----------------------------------------------------------------------------- 
/**
    Read optional drive setting from estart.txt that may contain additional drive attributes, like KDRIVEATTLOGICALLYREMOVABLE, KDRIVEATTHIDDEN
    and process them. 
    This may lead to addding/removing some attributes to the existing drive's ones.
    N.B Implementation of this is quite strange, it knows only about 2 specific additional attributes..

    @param  aDrvNum drive number
    @param  aDrvAtt in: original drive attributes, out: altered drive attributes.    


*/
static void DoProcessAdditionalDriveAttributes(TInt aDrvNum, TUint& aDrvAtt)
{
	TUint add_Att= 0;       //-- additional drive attributes to be added
	TUint remove_Att = 0;   //-- additional drive attributes to be removed

	_LIT8( KAddAtt, "AddDriveAttributes");
	_LIT8( KRemoveAtt, "RemoveDriveAttributes");
	_LIT8( KLogicallyRemovableAtt, "KDRIVEATTLOGICALLYREMOVABLE");
	_LIT8( KHiddenAtt, "KDRIVEATTHIDDEN");
	_LIT8( KLogicallyRemovableAttHex, "0X200");
	_LIT8( KHiddenAttHex, "0X400");
	
    TBuf8<0x100> buf;
	TBuf8<8> sectionName;
    F32Properties::GetDriveSection(aDrvNum, sectionName);
    
    //-- read and parse "AddDriveAttributes" string
    buf.Zero();
    TBool bSectFound = F32Properties::GetString(sectionName, KAddAtt, buf);

	if(bSectFound && buf.Length() > 0)
		{
		TInt pos = 0;
		TInt length = 0;
		TPtrC8 ptr;
		TBool endOfFlag=EFalse; 

		while(!endOfFlag)
		    {
		    ptr.Set(buf.Mid(pos));
		    length = ptr.Locate(',');
	    
		    if(length == KErrNotFound)
			    {
			    endOfFlag = ETrue;
			    } 
		    else{
			    ptr.Set(ptr.Left(length));
			    pos += (length +1);
			    }
		    
		    if(((ptr.MatchF(KLogicallyRemovableAtt)) != KErrNotFound) || ((ptr.MatchF(KLogicallyRemovableAttHex)) != KErrNotFound))
			    add_Att |= KDriveAttLogicallyRemovable;

		    if(((ptr.MatchF(KHiddenAtt)) != KErrNotFound)  || ((ptr.MatchF(KHiddenAttHex)) != KErrNotFound))
			    add_Att |= KDriveAttHidden;
		    
		    }//while(!endOfFlag)
        }

    //-- read and parse "RemoveDriveAttributes" string
    buf.Zero();
    bSectFound = F32Properties::GetString(sectionName, KRemoveAtt, buf);  //oldAtt now contains value of the attributes to be removed from iDriveAtt.

	if(bSectFound && buf.Length() > 0)
		{
		TInt pos = 0;
		TInt length = 0;
		TPtrC8 ptr;
		TBool endOfFlag=EFalse; 

		while(!endOfFlag)
            {
            ptr.Set(buf.Mid(pos));
            length = ptr.Locate(',');
	
            if(length == KErrNotFound)
                {
                endOfFlag = ETrue;
                } 
            else
                {
                ptr.Set(ptr.Left(length));
                pos += (length +1);
                }
		
            if(((ptr.MatchF(KLogicallyRemovableAtt)) != KErrNotFound) || ((ptr.MatchF(KLogicallyRemovableAttHex)) != KErrNotFound))
                remove_Att |= KDriveAttLogicallyRemovable;

            if(((ptr.MatchF(KHiddenAtt)) != KErrNotFound) || ((ptr.MatchF(KHiddenAttHex)) != KErrNotFound))
                remove_Att |= KDriveAttHidden;
		
            }//while(!endOfFlag)
		
        }
	
	if ((add_Att & KDriveAttLogicallyRemovable))
		{
		add_Att |= KDriveAttRemovable; 	//KDriveAttLogicallyRemovale should always set KDriveAttRemovale
		}
	
    if ((remove_Att & KDriveAttRemovable))
		{
		remove_Att |= KDriveAttLogicallyRemovable;
		}
	
    
	aDrvAtt |= add_Att;     //-- add new attributes to drive's ones
    aDrvAtt &= ~remove_Att; //-- remove drive attributes if there are soem indicated
}

//----------------------------------------------------------------------------- 
/**
    Try to find and process "ForceRugged" key in the drive section. It looks like:
    [DriveX]
    ForceRugged N
        
    where when N==0, the drive forced to be non-rugged, when N==1, the drive forced to be rugged,
    otherwise the original value is not overridden.

    @param  aDrvNum drive number
    @param  aRugged out: true/false if the setting is found, not changed if there is no such a key in the config file
*/
static void DoProcessForceRuggedSetting(TInt aDrvNum, TBool& aRugged)
{
    _LIT8(KKeyname, "ForceRugged");
	TBuf8<8> sectionName;
    F32Properties::GetDriveSection(aDrvNum, sectionName);

    TInt32 val=-1;
    if(F32Properties::GetInt(sectionName, KKeyname, val))
        {
        if(val == 0)
            aRugged = EFalse;
        else if(val == 1)
            aRugged = ETrue;
        //else the value is considered to be invalid and nothing changed
        }
}

//----------------------------------------------------------------------------- 
/**
    A helper function that binds a file system to the drive along with the optional primary extension etc.
    
    @param  aRequest        FS request object
    @param  aFsName         file system name to be bound to the drive
    @param  aIsExtension    if true, this means that it's necessary to mount aprimary extension as well
    @param  aIsSync         specifies if this drive is synchronous one or not 

    @return standard error code
*/
static TInt DoMountFsInitialise(CFsRequest* aRequest, TDesC& aFsName, TBool aIsExtension, TBool aIsSync)
	{
	if (!KCapFsMountFileSystem.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount File System")))
		return KErrPermissionDenied;

	TInt r=ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return(r);

	const TBool driveThreadExists = FsThreadManager::IsDriveAvailable(aRequest->DriveNumber(), ETrue);
	TDrive& drive = *aRequest->Drive();
    
	if(driveThreadExists)
		{
		// A drive thread already exists for this drive.This could be because a filesystem
		// is already mounted, or a proxy drive is loaded.  Check the mount to be sure...
		if(drive.GetFSys())
			{
			// Yes, a mount already exists so we can't mount another one!
			return(KErrAccessDenied);
			}

		__ASSERT_DEBUG(IsProxyDrive(aRequest->DriveNumber()), User::Panic(_L("Bad thread state - No Mount or Proxy Drive Exists!"), -999));
		}

	// ...therefore no drive thread can be present
	__ASSERT_DEBUG(! &drive.FSys(),Fault(EMountFileSystemFSys));

	if(drive.IsSubsted())
		return(KErrAccessDenied);

	CFileSystem* pF = GetFileSystem(aFsName);
	if(!pF)
		return(KErrNotFound);


	// Check that if the drive is a proxy drive (not using TBusLocalDrive) then the filesystem supports these...
	const TInt driveNumber = aRequest->DriveNumber();
	if(IsProxyDrive(driveNumber))
		{
		if(!pF->IsProxyDriveSupported())
			return KErrNotSupported;
		
		r = LocalDrives::SetupMediaChange(driveNumber);
		}

	
    //-- get drive information from the file system
	TDriveInfo driveInfo;
	driveInfo.iDriveAtt=0;
	pF->DriveInfo(driveInfo, driveNumber); //-- the file system can override some driveInfo properties
	if(!driveInfo.iDriveAtt)
		r = KErrArgument;
	
    if(r == KErrNone && !driveThreadExists)
	    {// determine whether file system synchronous or not not by flag passed in
		r=FsThreadManager::InitDrive(driveNumber, aIsSync);
        }

	if(r!=KErrNone)
		return(r);

    
    //-- add a primary drive extension if it is specified
    if(aIsExtension && aRequest->Message().Ptr2()!=NULL)
		{
        //-- check extension name length. It should not exceed KMaxFSNameLength (32 characters)
        r = aRequest->GetDesLength(KMsgPtr2);
        if(r <=0 || r >KMaxFSNameLength)
            return KErrArgument;

		TFSName extName;
		
		r = aRequest->Read(KMsgPtr2,extName);
		if (r!=KErrNone)
			return r;
		
		CProxyDriveFactory* pE=GetExtension(extName);
		if(pE==NULL)
			return(KErrNotFound);
		
        r=drive.MountExtension(pE,ETrue);
		if(r!=KErrNone)
			return(r);
		}

	//-- process optional additional drive attributes that can be specified in estart.txt 
    DoProcessAdditionalDriveAttributes(driveNumber, driveInfo.iDriveAtt);

    if(driveInfo.iDriveAtt & KDriveAttLogicallyRemovable)
        {
        ASSERT(driveInfo.iDriveAtt & KDriveAttRemovable);
        }

    
    //-- process 'rugged drive' property
    TBool bRuggedDrive = drive.IsRugged();
    
    //-- reset 'Rugged drive' attribute if this drive is removable. Having 'rugged' file system on a removable drive 
    //-- doesn't make any sense and leads only to the performance degradation
    if(bRuggedDrive && (driveInfo.iDriveAtt & KDriveAttRemovable))
        {
        __PRINT1(_L("DoMountFsInitialise() drv:%d is removable. resetting 'Rugged' flag!"), driveNumber);
        bRuggedDrive = EFalse;
        }

    DoProcessForceRuggedSetting(driveNumber, bRuggedDrive); //-- setting in estart.txt can override this flag
    drive.SetRugged(bRuggedDrive);


    //-- let TDrive object know if the drive is synchronous
	drive.SetSynchronous(aIsSync);
    
    drive.SetAtt(driveInfo.iDriveAtt); //-- finally set drive attributes
	drive.GetFSys()=pF;                //-- bind a file system to the drive 

	
	TClosedFileUtils::Remove(driveNumber); // empty the closed file queue

	return KErrNone;
	}

//----------------------------------------------------------------------------- 
TInt TFsMountFileSystem::DoRequestL(CFsRequest* aRequest)
//
// Mount a filesystem on a drive.
//
	{
	TInt r=DoMountFileSystem(aRequest);
	if( KErrNone == r )
		{
		FsNotify::DiskChange(aRequest->DriveNumber());
		}
		
	// Refresh the loader cache to ensure that the new drive is monitored.
#ifndef __WINS__
	gInitCacheCheckDrivesAndAddNotifications = EFalse;
#endif 

	return r;
	}

//----------------------------------------------------------------------------- 
TInt TFsMountFileSystem::Initialise(CFsRequest* aRequest)
	{
    TInt r;
    
    //-- check file system name length. It should not exceed KMaxFSNameLength (32 characters)
    r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;

    TFSName name;
	r = aRequest->Read(KMsgPtr0, name);

	if (r == KErrNone)
		r = DoMountFsInitialise(aRequest,name,ETrue,aRequest->Message().Int3());
	
	return r;
	}

//----------------------------------------------------------------------------- 
TInt TFsMountFileSystemScan::DoRequestL(CFsRequest* aRequest)
//
// mount file system and then call scandrive
//
	{
	TInt r=DoMountFileSystem(aRequest);
	// run scandrive if successful mount
	TBool isMountSuccess=(KErrNone==r);
	if(isMountSuccess)
		{
		r=aRequest->Drive()->ScanDrive();
		FsNotify::DiskChange(aRequest->DriveNumber());
		}

	TPtrC8 pMS((TUint8*)&isMountSuccess,sizeof(TBool));
	aRequest->WriteL(KMsgPtr3,pMS);
	return(r);
	}


//----------------------------------------------------------------------------- 
TInt TFsMountFileSystemScan::Initialise(CFsRequest* aRequest)
	{
    TInt r;

    //-- check file system name length. It should not exceed KMaxFSNameLength (32 characters)
    r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;
	
    TFSName name;
	r = aRequest->Read(KMsgPtr0,name);
	if (r == KErrNone)
		r = DoMountFsInitialise(aRequest,name,ETrue,EFalse);
	
	return r;
	}

//----------------------------------------------------------------------------- 
static TInt DoDismountFileSystem(const TDesC& aName, TDrive* aDrive, TBool aAllowRom, TBool aForceDismount)
//
// Do file system dismount
//
	{
	TInt drvNumber=aDrive->DriveNumber();

	FsThreadManager::LockDrive(drvNumber);
	CFileSystem* pF=GetFileSystem(aName);
	if(pF==NULL)
		{
		FsThreadManager::UnlockDrive(drvNumber);
		return(KErrNotFound);
		}
	if(aDrive->IsRom() && !aAllowRom)
		{
		FsThreadManager::UnlockDrive(drvNumber);
		return(KErrAccessDenied);
		}

	if(!aForceDismount)
		{
		if(aDrive->IsMounted() && aDrive->CurrentMount().LockStatus()!=0)
			{
			FsThreadManager::UnlockDrive(drvNumber);
			return(KErrInUse);
			}
		if(aDrive->ActiveMounts() > 1)
			{
			FsThreadManager::UnlockDrive(drvNumber);
			return(KErrInUse);
			}
		
		aDrive->ReactivateMounts();
		}

	// ensure that current mount is dismounted
	if(aForceDismount)
		{
		TInt r = aDrive->FlushCachedFileInfo(ETrue);

		// Dismount the file system even if the flush fails for some reason (media permanently removed, user cancels notifier etc
		if (r!=KErrNone && r!=KErrAbort)
			{
			FsThreadManager::UnlockDrive(drvNumber);
			return(r);
			}
		aDrive->ForceDismount();
		}
	else
		{
		aDrive->Dismount();
		}

	aDrive->GetFSys()=NULL;
	aDrive->SetAtt(0);
	aDrive->ExtInfo().iCount=0;

	// no need to cancel requests if synchronous since queued
	if(!FsThreadManager::IsDriveSync(drvNumber,EFalse))
		{
		CDriveThread* pT=NULL;
		TInt r=FsThreadManager::GetDriveThread(drvNumber,&pT);
		__ASSERT_ALWAYS(r==KErrNone && pT,Fault(EDismountFsDriveThread));
		pT->CompleteAllRequests(KErrNotReady);
		}

	if(!IsProxyDrive(drvNumber))
		{
		// Proxy drives are responsible for managing the drive threads...
		FsThreadManager::CloseDrive(drvNumber);
		}

	FsThreadManager::UnlockDrive(drvNumber);
	FsNotify::DiskChange(drvNumber);
	return(KErrNone);
	}

//----------------------------------------------------------------------------- 
TInt TFsDismountFileSystem::DoRequestL(CFsRequest* aRequest)
//
// Dismount a filesystem from a drive.
//
	{
	TDrive* drive=aRequest->Drive();
	__ASSERT_DEBUG(&aRequest->Drive()->FSys() && !drive->IsSubsted(),Fault(EDisMountFileSystemFSys));
	
    TFSName name;
	aRequest->ReadL(KMsgPtr0,name);

	if(drive->DismountDeferred())
		return KErrInUse;

	return DoDismountFileSystem(name, drive, EFalse, EFalse);
	}

//----------------------------------------------------------------------------- 
TInt TFsDismountFileSystem::Initialise(CFsRequest* aRequest)
//
//	
//
	{
    TInt r;

	if (!KCapFsDismountFileSystem.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Dismount File System")))
		return KErrPermissionDenied;

    //-- check file system name length. It should not exceed KMaxFSNameLength (32 characters)
    r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;
	
    
    r = ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r == KErrNone)
		{
		TInt driveNumber = aRequest->DriveNumber();
		if(IsProxyDrive(driveNumber))
			{
			LocalDrives::NotifyChangeCancel(driveNumber);
			}
		}
	return r;
	}

/**
    Return name of file system mounted on a specified drive or one of the file system names if 
    the drive supports several of them.
*/
TInt TFsFileSystemName::DoRequestL(CFsRequest* aRequest)
	{
	//-- ipc parameters: 
    //-- 0 out: file system name decriptor
    //-- 1 drive number
    //-- 2 file system enumerator 
    
    const TInt driveNumber = aRequest->Message().Int1();
	if (driveNumber < 0 || driveNumber >= KMaxDrives)
		return KErrArgument;
	
    const TInt fsNumber = aRequest->Message().Int2(); //-- file system number; for RFs::FileSystemName() it is "-1"

    TFullName fsName;
	// lock drive to synchronise with dismounting a file system
	FsThreadManager::LockDrive(driveNumber);
	CFileSystem* pF=TheDrives[driveNumber].GetFSys();
	FsThreadManager::UnlockDrive(driveNumber);
    
    TInt err = KErrNone;
	
    if(pF)
        {
		if(fsNumber == -1)
            fsName = pF->Name(); //-- this is RFs::FileSystemName() call
        else
            err = pF->GetSupportedFileSystemName(fsNumber, fsName); //-- this is RFs::SupportedFileSystemName() call
	    }
	else
		{//-- the drive doesn't have file system installed
        fsName=_L("");
		err = KErrNotFound;
	    }
    
    aRequest->WriteL(KMsgPtr0, fsName);
	
    return err;
	}

TInt TFsFileSystemName::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}

TInt TFsRemountDrive::DoRequestL(CFsRequest* aRequest)
//
// Force a remount of the specified drive
//
	{	
	const TDesC8 *mountInfo=REINTERPRET_CAST(const TDesC8*,aRequest->Message().Ptr1());
	return(aRequest->Drive()->ForceRemountDrive(mountInfo,aRequest->Message().Handle(),aRequest->Message().Int2()));//changed from thread to message handle
	}

TInt TFsRemountDrive::Initialise(CFsRequest* aRequest)
//
//	
//
	{

	TInt r=ValidateDriveDoSubst(aRequest->Message().Int0(),aRequest);
	return(r);
	}

TInt TFsSetLocalDriveMapping::DoRequestL(CFsRequest* aRequest)
//
// set up drive letter to local drive mapping
//
	{
	return(LocalDrives::SetDriveMappingL(aRequest));
	}

TInt TFsSetLocalDriveMapping::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return KErrNone;
	}
	
_LIT(KCompositeFsName,"Composite");

TInt TFsSwapFileSystem::DoRequestL(CFsRequest* aRequest)
//
// Swap a filesystem on a drive
// Should always leave a filesystem mounted on the drive
//
	{
	TFSName newName;
	aRequest->ReadL(KMsgPtr0,newName);										
	CFileSystem* pF=GetFileSystem(newName);												
	if (pF==NULL)															
		return(KErrNotFound);
	
    TFSName oldName;			
	aRequest->ReadL(KMsgPtr2,oldName);										
	TInt drvNumber=aRequest->Message().Int1();	
	TBool newFsIsComposite = (newName.CompareF(KCompositeFsName) == 0);	
							
	if (newFsIsComposite)
		{
		if(CompFsMounted)
			return(KErrAlreadyExists);
		if(EDriveZ!=drvNumber)
			return(KErrNotSupported);
		}	
	else
		 // swapping filesystem on z: only allow for romfs + compfs
		if(EDriveZ==drvNumber)
			return(KErrNotSupported);
	
	TDrive& drive=TheDrives[drvNumber];
	
	if(drive.DismountDeferred())
		return KErrInUse;

	TBool clamps=drive.ClampFlag();
	if(clamps)
		return KErrInUse;

	// Return an error if the drive is asynchronous.
	// This function is only supported on synchronous drives.
	TBool isSync = FsThreadManager::IsDriveSync(drvNumber,EFalse);
	if(!isSync)
		return KErrNotSupported;
	
	TInt r=DoDismountFileSystem(oldName,&drive,ETrue,EFalse);
	if(r!=KErrNone)
		return(r);
	
	__ASSERT_ALWAYS(drive.GetFSys()==NULL,Fault(ESwapFileSystemNull));

	r=DoMountFsInitialise(aRequest,newName,EFalse,isSync);
	if(r==KErrNone)
		r=DoMountFileSystem(aRequest);

	if(drive.GetFSys()==NULL || (newFsIsComposite && r!=KErrNone)) 
		{
		// remounting of the original filesystem should not fail
		if(drive.GetFSys()!=NULL)
			r=DoDismountFileSystem(newName,&drive,ETrue,EFalse);

		r=DoMountFsInitialise(aRequest,oldName,EFalse,isSync);
		if(r==KErrNone)
			r=DoMountFileSystem(aRequest);

		__ASSERT_ALWAYS(r==KErrNone && drive.GetFSys()!=NULL,Fault(ESwapFileSystemMount));
		}
	else if (newFsIsComposite)
		{
		FsThreadManager::ChangeSync(drvNumber,CompFsSync);
		CompFsMounted=ETrue;
		}
			
	if(drvNumber==EDriveZ)
		{
		__ASSERT_ALWAYS(r==KErrNone,Fault(ESwapFileSystemRom));
		RefreshZDriveCache=ETrue;
		}
	return(r);
	}

TInt TFsSwapFileSystem::Initialise(CFsRequest* aRequest)
	{
    TInt len;
    
    //-- check file system name length. It should not exceed KMaxFSNameLength (32 characters)
    len = aRequest->GetDesLength(KMsgPtr0); //-- new name
    if(len <=0 || len >KMaxFSNameLength)
        return KErrArgument;

    len = aRequest->GetDesLength(KMsgPtr2); //-- old name
    if(len <=0 || len >KMaxFSNameLength)
        return KErrArgument;

	return KErrNone;
	}	


TInt TFsAddCompositeMount::DoRequestL(CFsRequest* aRequest)
//
// Input fsyName, localDriveNUmber, CompositeDriveNumber
// 
//
	{
	__PRINT(_L("TFsAddCompositeMount::DoRequestL"));

	TFileName fsyName;
	aRequest->ReadL(KMsgPtr0,fsyName);										
	CFileSystem* pNewFileSystem=GetFileSystem(fsyName);

	if (pNewFileSystem==NULL)
		return KErrNotFound;
	
	const TInt localDriveNumber=aRequest->Message().Int1();								
	const TInt compositeDriveNumber=aRequest->Message().Int2();
	const TInt sync=aRequest->Message().Int3();									
	
    __PRINT3(_L("TFsAddCompositeMount::DoRequestL fsy:%S, locDrv:%d, compDrv:%d"),&fsyName, localDriveNumber,compositeDriveNumber);

	// Currently, compFS assumed its mounting on romfs, on z:
	if (compositeDriveNumber!=EDriveZ)
		return KErrNotSupported;
	
	// Mounts can only be added to the compfs, before it is mounted.
	if (CompFsMounted)
		return KErrInUse;
	
	// The drive is needed, so the new sub mount, can be mounted
	// on it temporarily.  ROMFS doest care if we do this as it
	// has no local mapping.
	FsThreadManager::LockDrive(compositeDriveNumber);
		
	TRAPD(err, AddFsToCompositeMountL(compositeDriveNumber, *pNewFileSystem, localDriveNumber));
	if (err!=KErrNone)
		{
		FsThreadManager::UnlockDrive(compositeDriveNumber);
		return err;
		}
//		Fault(EMountFileSystemFSys);

	FsThreadManager::UnlockDrive(compositeDriveNumber);
	
	// The drive will end up asynchronous if any sub mounts are.
	if (!sync)
		CompFsSync=EFalse;

	return KErrNone;
	}


TInt TFsAddCompositeMount::Initialise(CFsRequest* aRequest)
//
//	
//
	{
	if (!KCapFsAddCompositeMount.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Add Composite Mount")))
		return KErrPermissionDenied;
	return KErrNone;
	}	

void TFsAddCompositeMount::AddFsToCompositeMountL(TInt aDriveNumber, CFileSystem& aFileSystem, TInt aLocalDriveNumber)
	{
	__PRINT3(_L("TFsAddCompositeMount::AddFsToCompositeMountL()  FS:0x%x, drv:%d, local drive:%d"),&aFileSystem, aDriveNumber,aLocalDriveNumber);
	TInt err;

	TDrive& theDrive=TheDrives[aDriveNumber];
	CFileSystem* pFs = theDrive.GetFSys();
	
	CFileSystem* pCompFS = GetFileSystem(_L("Composite"));
	if ((pCompFS == NULL) || (pFs==NULL))
		User::Leave(KErrNotReady);
	
	CMountCB* pMount = pCompFS->NewMountL(); //-- pMount is, actually, a singleton.
    pMount->InitL(theDrive, pCompFS);

    
	// invalidate the drive previously used by the local drive just added, and swap with a DriveNumber
	TInt drv;
	drv = LocalDrives::GetDriveFromLocalDrive(aLocalDriveNumber);
	__ASSERT_ALWAYS(drv!=KDriveInvalid, User::Leave(KErrNotSupported));
	//__PRINT1(_L("TFsAddCompositeMount::AddFsToCompositeMountL : drive to invalidate %d"),drv);
	LocalDrives::iMapping[drv] = KDriveInvalid;
	LocalDrives::iMapping[aDriveNumber] = aLocalDriveNumber;

	// Ask the composite mount to mount the new filesystem.
	TAny* dummy=NULL;
	err = pMount->GetInterfaceTraced(CMountCB::EAddFsToCompositeMount, dummy, &aFileSystem);
	if(err != KErrNone)
		User::Leave(err);
	}


TInt TDrive::DeferredDismount()
	{
	// Dismount
	TInt err = DoDismountFileSystem(GetFSys()->Name(), this, EFalse, ETrue);
	if (err == CFsRequest::EReqActionBusy)
		return err;

    DoCompleteDismountNotify(err);

	SetDismountDeferred(EFalse);

	return err;
	}

TInt TFsNotifyDismount::Initialise(CFsRequest* aRequest)
//
// Initialise a dismount notifier. 
// - All clients may register with EFsDismountRegisterClient.
// - DiskAdmin is required for EFsDismountNotifyClients and EFsDismountForceDismount
//
	{
	const RMessage2& m=aRequest->Message();
	const TNotifyDismountMode mode = (TNotifyDismountMode)m.Int1();

	switch(mode)
		{
		case EFsDismountForceDismount:
		case EFsDismountNotifyClients:
			{
			// Capabilities are required to dismount a file system
			if(!KCapFsDismountFileSystem.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Notify Dismount")))
				return KErrPermissionDenied;
			break;
			}

		case EFsDismountRegisterClient:
			{
			// No capabilities are required for a client to register for notification
			break;
			}

		default:
			{
			return KErrArgument;
			//break;
			}
		}

	return ValidateDrive(aRequest->Message().Int0() ,aRequest);
	}	

TInt TFsNotifyDismount::DoRequestL(CFsRequest* aRequest)
//
// Register for notification of pending dismount [EFsDismountRegisterClient]
// or notify clients of a pending dismount		 [EFsDismountNotifyClients]
// or forcibly dismount the file system			 [EFsDismountForceDismount]
//
	{
	__ASSERT_DEBUG(&aRequest->Drive()->FSys() && !aRequest->Drive()->IsSubsted(), Fault(ENotifyDismount));

	TInt err = KErrNone;

	const RMessage2& m=aRequest->Message();
	const TNotifyDismountMode mode = (TNotifyDismountMode)m.Int1();
	TDrive* theDrive = aRequest->Drive();
	const TInt driveNumber = theDrive->DriveNumber();

	switch(mode)
		{
		case EFsDismountRegisterClient:
			{
			err = RegisterNotify(aRequest);
			break;
			}

		case EFsDismountNotifyClients:
			{
			if(aRequest->Drive()->DismountLocked())
				{
				err = RegisterNotify(aRequest);
				if (err != KErrNone)
					return err;
				// Complete outstanding client dismount notifiers and flag the drive as pending dismount.
				FsNotify::HandleDismount(EFsDismountRegisterClient, driveNumber, EFalse, KErrNone);
				theDrive->SetDismountDeferred(ETrue);
				}
			else
				{
				// There are no interested clients, so dismount immediately - if no clamps are present
				err = theDrive->ClampsOnDrive();

				// If there are no clamps or clamping is not supported, proceed with the enforced dismount
				// If there are clamps, wait for the clamps to be removed
				if (err > 0)
					{
					err = RegisterNotify(aRequest);
					theDrive->SetDismountDeferred(ETrue);
					}
				else if (err == 0 || err == KErrNotSupported)
					{
					// No clamps to worry about, so dismount immediately and complete the request
					err = DoDismountFileSystem(theDrive->GetFSys()->Name(), theDrive, EFalse, ETrue);
					if (err == CFsRequest::EReqActionBusy)
						return err;
					m.Complete(err);
					}
				}
			break;
			}

		case EFsDismountForceDismount:
			{
			// Prepare for deferred dismount due to the presence of file clamps
			err = theDrive->ClampsOnDrive();

			// If there are no clamps or clamping is not supported, proceed with the enforced dismount
			// If there are clamps, wait for the clamps to be removed
			if(err > 0)
				{
				err = RegisterNotify(aRequest);
				theDrive->SetDismountDeferred(ETrue);
				}
			else if (err == 0 || err == KErrNotSupported)
				{
				// Forced dismount - notify/remove all client notifiers and complete immediately
				err = theDrive->DeferredDismount();
				if (err == CFsRequest::EReqActionBusy)
					return err;
				m.Complete(err);
				}
			break;
			}

		default:
			{
			// We shouldn't ever get here
			Fault(ENotifyDismount);
			break;
			}
		}

	return err;
	}

TInt TFsNotifyDismount::RegisterNotify(CFsRequest* aRequest)
//
// Register for notification of pending dismount [EFsDismountRegisterClient]
// or notify clients of a pending dismount		 [EFsDismountNotifyClients]
// or forcibly dismount the file system			 [EFsDismountForceDismount]
//
	{
	const RMessage2& m=aRequest->Message();
	const TNotifyDismountMode mode = (TNotifyDismountMode)m.Int1();
	TDrive* theDrive = aRequest->Drive();
	const TInt driveNumber = theDrive->DriveNumber();

	if (mode == EFsDismountNotifyClients && theDrive->DismountDeferred())
		{
		return KErrInUse;
		}

	CDismountNotifyInfo* info = new CDismountNotifyInfo;
	if(info == NULL)
		{
		return KErrNoMemory;
		}

	info->Initialise(mode, driveNumber, (TRequestStatus*)m.Ptr2(), m, aRequest->Session());
	TInt err = FsNotify::AddDismountNotify(info);
	if(err != KErrNone)
		{
		delete info;
		return err;
		}

	return KErrNone;
	}

TInt TFsNotifyDismountCancel::DoRequestL(CFsRequest* aRequest)
//
// Cancel a pending dismount notifier - Request
//
	{
	CSessionFs* session = aRequest->Session();
	TInt drive = FsNotify::CancelDismountNotifySession(session, (TRequestStatus*)aRequest->Message().Ptr0());
	if (drive >= 0)
		TheDrives[drive].SetDismountDeferred(EFalse);
	return KErrNone;
	}

TInt TFsNotifyDismountCancel::Initialise(CFsRequest* /*aRequest*/)
//
//	Cancel a pending dismount notifier - Initialise
//
	{
	return KErrNone;
	}	

TInt TFsAllowDismount::DoRequestL(CFsRequest* aRequest)
//
// Notifies the file server that the client is finished with the drive.
// The last client to allow the dismount signals the dismounting thread.
//
	{
	TDrive* theDrive = aRequest->Drive();
	const TInt driveNumber = theDrive->DriveNumber();

	// Verify that the client has registered for notification
	if(!FsNotify::HandlePendingDismount(aRequest->Session(), driveNumber))
		return KErrNotFound;

	if(theDrive->DismountLocked())
		return KErrNone;

	TInt clampErr = theDrive->ClampsOnDrive();
	TInt err = KErrNone;

	if ((theDrive->DismountDeferred()) && (clampErr == 0 || clampErr == KErrNotSupported))
		{
		// No clamps to worry about, so dismount immediately and complete the request
		__ASSERT_DEBUG(aRequest->Drive()->GetFSys(), Fault(EAllowDismount));

		// When the last client has responded, allow the media to be forcibly dismounted
		err = theDrive->DeferredDismount();
		}

	return err;
	}

TInt TFsAllowDismount::Initialise(CFsRequest* aRequest)
//
// Notifies the file server that the client is finished with the drive
//
	{
	return ValidateDrive(aRequest->Message().Int0(),aRequest);
	}	


TInt TFsMountProxyDrive::DoRequestL(CFsRequest* aRequest)
	{
	return LocalDrives::MountProxyDrive(aRequest);
	}

TInt TFsMountProxyDrive::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsMountProxyDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount Proxy Drive")))
		return KErrPermissionDenied;

    TInt err = LocalDrives::InitProxyDrive(aRequest);
	if(err == KErrNone)
	    {
    	// Now create the drive thread - proxy extensions are always asynchronous...
		err = FsThreadManager::InitDrive(aRequest->DriveNumber(), EFalse);
        }

	return err;
	}

TInt TFsLoadCodePage::DoRequestL(CFsRequest* aRequest)
//
// Installs a code page
//
	{
	__PRINT(_L("TFsLoadCodePage::DoRequestL(CFsRequest* aRequest)"));

	RLibrary lib;
	lib.SetHandle(aRequest->Message().Int0());
	if (lib.Type()[1]!=TUid::Uid(KLocaleDllUidValue16))
		return(KErrNotSupported);

	if(TheCodePage.CodepageLoaded() == TCodePageUtils::ECodePageDll)
		{
		return(KErrAlreadyExists);
		}

	/*
	// Actual Functions form the Codepage Dll (fatCnvU.def)
	1	:	void UnicodeConv::ConvertFromUnicodeL(class TDes8 &, class TDesC16 const &)
	2	:	void UnicodeConv::ConvertToUnicodeL(class TDes16 &, class TDesC8 const &)
	3	:	int UnicodeConv::IsLegalShortNameCharacter(unsigned int)
	4	:	int UnicodeConv::ConvertFromUnicodeL(class TDes8 &, class TDesC16 const &, int)
	5	:	int UnicodeConv::ConvertToUnicodeL(class TDes16 &, class TDesC8 const &, int)
	*/

	/*
	Read only the following fns from Codepage Dll ( lib.Lookup(1) and lib.Lookup(2) retained in cpnnn.dll for backward compatibility)
	3	:	int UnicodeConv::IsLegalShortNameCharacter(unsigned int)
	4	:	int UnicodeConv::ConvertFromUnicodeL(class TDes8 &, class TDesC16 const &, int)
	5	:	int UnicodeConv::ConvertToUnicodeL(class TDes16 &, class TDesC8 const &, int)
	*/

	TheCodePage.iCodePageFunctions.iIsLegalShortNameCharacter = (TCodePageFunctions::TIsLegalShortNameCharacter)(lib.Lookup(3));
	TheCodePage.iCodePageFunctions.iConvertFromUnicodeL = (TCodePageFunctions::TConvertFromUnicodeL)(lib.Lookup(4));
	TheCodePage.iCodePageFunctions.iConvertToUnicodeL = (TCodePageFunctions::TConvertToUnicodeL)(lib.Lookup(5));

	if( TheCodePage.iCodePageFunctions.iIsLegalShortNameCharacter == NULL || 
		TheCodePage.iCodePageFunctions.iConvertFromUnicodeL == NULL ||
		TheCodePage.iCodePageFunctions.iConvertToUnicodeL == NULL )
		{
		return(KErrCorrupt);
		}
	TheCodePage.iCodepageLoaded = TCodePageUtils::ECodePageDll;

	return(KErrNone);
	}

TInt TFsLoadCodePage::Initialise(CFsRequest* aRequest)
//
// Installs a code page
//
	{
	__PRINT(_L("TFsLoadCodePage::Initialise(CFsRequest* aRequest)"));
	
	// Set the drive
	TInt drive = aRequest->Session()->CurrentDrive();
	aRequest->SetDrive(&TheDrives[drive]);

	TSecurityPolicy policy(RProcess().SecureId(), ECapabilityDiskAdmin);
	if (!policy.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("TFsLoadCodePage::Initialise")))
		{
		return KErrPermissionDenied;
		}
	
	return KErrNone;
	}

// use second half of ISO Latin 1 character set for extended chars
const TUint KExtendedCharStart=0x80;
const TUint KExtendedCharEnd=0xff;

_LIT8(KLit8ReplacementForUnconvertibleUnicodeCharacters, "_");

TCodePageUtils::TCodePageUtils()
//
// Constructor
//
  :	iCodePageFunctions(),
	iLocaleFatUtilityFunctions(NULL),
	iCodepageLoaded(ENone)
	{
	}

TBool TCodePageUtils::IsCodepageLoaded() const
//
// Returns ETrue if a codepage is loaded
//
	{
	return(iCodepageLoaded != ENone);
	}

TCodePageUtils::TCodepageLoaded TCodePageUtils::CodepageLoaded() const
//
// Returns the type of active codepage
//
	{
	return(iCodepageLoaded);
	}

void TCodePageUtils::SetLocaleCodePage(TFatUtilityFunctions* aFunctions)
//
// Sets the current codepage to that provided by the current Locale DLL
//
	{
	if(iCodepageLoaded == ENone && aFunctions)
		{
		iLocaleFatUtilityFunctions = aFunctions;
		iCodepageLoaded = ELocaleDll;
		}
	}

TFatUtilityFunctions* TCodePageUtils::LocaleFatUtilityFunctions() const
//
// Returns function pointer to the read Locale conversions functions
//
	{
	return(iLocaleFatUtilityFunctions);
	}

TCodePageFunctions TCodePageUtils::CodepageFatUtilityFunctions() const
//
// Returns structure to function pointers to the read Codepage conversions functions
//
	{
	return(iCodePageFunctions);
	}

TBool TCodePageUtils::ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode, TOverflowAction aOverflowAction) const
/**
Convert from Unicode, truncating if there is not enough room in the output.

@param aForeign The output is appended here.
@param aUnicode The input.

@return False if and only if aForeign has not enough space remaining. 
*/
	{
	TBool allConverted = ETrue;
	const TInt maximumLength=aForeign.MaxLength();
	TInt lengthToCopy=aUnicode.Length();
	// do not cross the maximum foreign length
	if (maximumLength<lengthToCopy)
		{
		if (aOverflowAction==TCodePageUtils::EOverflowActionLeave)
			{
			allConverted = EFalse;
			return allConverted;
			}
		lengthToCopy=maximumLength;
		}

	aForeign.SetLength(lengthToCopy);

	TInt j=0; // offset for aForeign[]
	TInt i=0; // offset for aUnicode[]
	for (i=0; i<lengthToCopy; ++i)
		{
		const TDesC8& replacementChar = KLit8ReplacementForUnconvertibleUnicodeCharacters;
		TUint32 unicodeChar = aUnicode[i];

		// if High Surrogate
		if (IsHighSurrogate((TText16)unicodeChar))
			{
			// check for low surrogate
			if (!IsLowSurrogate(aUnicode[++i]))
				{
				aForeign[j++] = (TUint8)replacementChar[0];
				continue;
				}
			unicodeChar = JoinSurrogate((TText16)unicodeChar, (TText16)aUnicode[i]);
			}

		// if Low Surrogate
		if (IsLowSurrogate((TText16)unicodeChar))
			{
			aForeign[j++] = (TUint8)replacementChar[0];
			continue;
			}
		// if Supplementary - Non BMP
		if (IsSupplementary(unicodeChar))
			{
			aForeign[j++] = (TUint8)replacementChar[0];
			}
		else
			{
			// ASCII support
			if((TUint)unicodeChar>=0x100)
				{
				aForeign[j++] = (TUint8)replacementChar[0];
				}
			else
				{
				aForeign[j++] = (TUint8)unicodeChar;
				}
			}
		}

	// if any replacementChar used, aForeign offset(j) shall be less than 
	// lengthToCopy aUnicode offset(i)
	if(j<i)
		{
		aForeign.SetLength(j);
		}

	return(allConverted);
	}

EXPORT_C void TCodePageUtils::ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TOverflowAction aOverflowAction) const
/**
Convert from Unicode, truncating if there is not enough room in the output.

@param aForeign The output is appended here.
@param aUnicode The input.

@leave KErrOverflow if aForeign is too short for the output.
*/
	{
	TInt r = KErrNone;
	TBool LeaveWhenError = (TBool)((aOverflowAction==TCodePageUtils::EOverflowActionLeave)?(TBool)ETrue:(TBool)EFalse);
	// if CodePage dll
	if(GetFatUtilityFunctions() && iCodepageLoaded == ECodePageDll && iCodePageFunctions.iConvertFromUnicodeL)
		{
		r = (*iCodePageFunctions.iConvertFromUnicodeL)(aForeign, aUnicode, LeaveWhenError);
		}
	// if Locale dll
	else if(GetFatUtilityFunctions() && iCodepageLoaded == ELocaleDll && iLocaleFatUtilityFunctions->iConvertFromUnicodeL)
		{
		if(aOverflowAction == TCodePageUtils::EOverflowActionLeave)
			{
			(*iLocaleFatUtilityFunctions->iConvertFromUnicodeL)(aForeign, aUnicode, KLit8ReplacementForUnconvertibleUnicodeCharacters, TFatUtilityFunctions::EOverflowActionLeave);
			}
		else
			{
			(*iLocaleFatUtilityFunctions->iConvertFromUnicodeL)(aForeign, aUnicode, KLit8ReplacementForUnconvertibleUnicodeCharacters, TFatUtilityFunctions::EOverflowActionTruncate);
			}
		}
	// default implementation
	else if (!ConvertFromUnicode(aForeign, aUnicode, aOverflowAction))
		{
		if (aOverflowAction==TCodePageUtils::EOverflowActionLeave)
			{
			User::Leave(KErrBadName);
			}
		}

	r = r; // remove warning
	// File Server do not use this error code so do not send this error code right to File Server
	// rather suppress it. Can be used in future.
	return;
	}

TBool TCodePageUtils::ConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign) const
/* 
Convert to Unicode, truncating if there is not enough room in the output.

@param aUnicode The output is appended here.
@param aForeign The input.

@return False if and only if aUnicode has not enough space remaining.
*/
	{
    const TInt KMaxLengthShortNameWithDot = 12;
    const TUint8 KLeadingE5Replacement = 0x05;
    const TUint8 KEntryErasedMarker=0xE5;           ///< Erased entry marker for a directory entry
	
	// A workaround to handle leading 'E5' byte in short file names 
	TBuf8<KMaxLengthShortNameWithDot> shortFileNameWithLeadingE5;
	TBool convertedLeading05toE5 = EFalse;

	if (0 < aForeign.Length() && aForeign.Length() <= 12 && aForeign[0] == KLeadingE5Replacement)
		{
		shortFileNameWithLeadingE5 = aForeign;
		shortFileNameWithLeadingE5[0] = KEntryErasedMarker;
		convertedLeading05toE5 = ETrue;
		}

	const TInt maximumLength=aUnicode.MaxLength();
	if (maximumLength>=aForeign.Length())
		{
		if (convertedLeading05toE5)
			{
			aUnicode.Copy(shortFileNameWithLeadingE5);
			}
		else
			{
			aUnicode.Copy(aForeign);
			}
		return ETrue;
		}
	else
		{
		if (convertedLeading05toE5)
			{
			aUnicode.Copy(shortFileNameWithLeadingE5.Left(maximumLength));
			}
		else
			{
			aUnicode.Copy(aForeign.Left(maximumLength));
			}
		return EFalse;
		}
	}

EXPORT_C void TCodePageUtils::ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TOverflowAction aOverflowAction) const
/* 
Convert to Unicode, leaving if there is not enough room in the output.

@param aUnicode The output is appended here.
@param aForeign The input.

@leave KErrOverflow if aUnicode is too short for the output.
*/
	{
	TInt r = KErrNone;
	TBool LeaveWhenError = (TBool)((aOverflowAction==TCodePageUtils::EOverflowActionLeave)?(TBool)ETrue:(TBool)EFalse);
	// if CodePage dll
	if(GetFatUtilityFunctions() && iCodepageLoaded == ECodePageDll && iCodePageFunctions.iConvertToUnicodeL)
		{
		r = (*iCodePageFunctions.iConvertToUnicodeL)(aUnicode, aForeign, LeaveWhenError);
		}
	// if Locale dll
	else if(GetFatUtilityFunctions() && iCodepageLoaded == ELocaleDll && iLocaleFatUtilityFunctions->iConvertToUnicodeL)
		{
		if(aOverflowAction == TCodePageUtils::EOverflowActionLeave)
			{
			(*iLocaleFatUtilityFunctions->iConvertToUnicodeL)(aUnicode, aForeign, TFatUtilityFunctions::EOverflowActionLeave);
			}
		else
			{
			(*iLocaleFatUtilityFunctions->iConvertToUnicodeL)(aUnicode, aForeign, TFatUtilityFunctions::EOverflowActionTruncate);
			}
		}
	// default implementation
	else if (!ConvertToUnicode(aUnicode, aForeign))
		{
		if (aOverflowAction==TCodePageUtils::EOverflowActionLeave)
			{
			User::Leave(KErrBadName);
			}
		}

	r = r; // remove warning
	// File Server do not use this error code so do not send this error code right to File Server
	// rather suppress it. Can be used in future.
	return;
	}

EXPORT_C TBool TCodePageUtils::IsLegalShortNameCharacter(TUint aCharacter,TBool aUseExtendedChars) const
/** 
Returns true if the input character is legal in a short name.

@param aCharacter Character, in the foreign character encoding.

@return true if aCharacter is legal in a FAT short name.
*/
	{
	if(GetFatUtilityFunctions() && iCodepageLoaded == ECodePageDll && iCodePageFunctions.iIsLegalShortNameCharacter)
		{
		return (*iCodePageFunctions.iIsLegalShortNameCharacter)(aCharacter);
		}

	if(GetFatUtilityFunctions() && iCodepageLoaded == ELocaleDll && iLocaleFatUtilityFunctions->iIsLegalShortNameCharacter)
		{
		return (*iLocaleFatUtilityFunctions->iIsLegalShortNameCharacter)(aCharacter);
		}

	// For most common cases:
	// Note: lower case characters are considered legal DOS char here.
	if ((aCharacter>='a' && aCharacter<='z') ||
		(aCharacter>='A' && aCharacter<='Z') ||
		(aCharacter>='0' && aCharacter<='9'))
		{
		return ETrue;
		}

	// Default Implmentation
	// Checking for illegal chars:
	// 1. aCharacter <= 0x20
	// Note: leading 0x05 byte should be guarded by callers of this function
	//  as the information of the position of the character is required.
	if (aCharacter < 0x20)
		return EFalse;
	// Space (' ') is not considered as a legal DOS char here.
	if (aCharacter == 0x20)
		return EFalse;

	// 2. 0x20 < aCharacter < 0x80
	if (0x20 < aCharacter && aCharacter < KExtendedCharStart)
		{
		// According to FAT Spec, "following characters are not legal in any bytes of DIR_Name":
		switch (aCharacter)
			{
			case 0x22:	// '"'
			case 0x2A:	// '*'
			case 0x2B:	// '+'
			case 0x2C:	// ','
	//		case 0x2E:	// '.'		// Although '.' is not allowed in any bytes of DIR_Name, it 
									// is a valid character in short file names.
			case 0x2F:	// '/'
			case 0x3A:	// ':'
			case 0x3B:	// ';'
			case 0x3C:	// '<'
			case 0x3D:	// '='
			case 0x3E:	// '>'
			case 0x3F:	// '?'
			case 0x5B:	// '['
			case 0x5C:	// '\'
			case 0x5D:	// ']'
			case 0x7C:	// '|'
				return EFalse;
			default:
			    return ETrue;
			}
		}

	// 3. 0x80 <= aCharacter <= 0xFF
	if (KExtendedCharStart <= aCharacter  && aCharacter <= KExtendedCharEnd)
		{
		if(aUseExtendedChars)
			return(ETrue);
		else
			return EFalse;
		}

	// 4. aCharacter => 0xFF
	return EFalse;
	}
