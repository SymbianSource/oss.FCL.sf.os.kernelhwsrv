// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
@file
@internalTechnology
*/

#include <e32cmn.h>
#include <f32ver.h>
#include <u32std.h>
#include <f32file.h>



#include "filesystem_automounter.h"
#include "automounter.h"


//-----------------------------------------------------------------------------

void Fault(TFault aFault)
    {
    _LIT(KPanicName, "AutoMounter_fsy");
    User::Panic(KPanicName, aFault);
    }


//-----------------------------------------------------------------------------

/**
    Factory function, Create a new object of this file system 
*/
extern "C" 
{
EXPORT_C CFileSystem* CreateFileSystem()
    {
    return CAutoMounterFileSystem::New();
    }
}


//#######################################################################################################################################
//#  CAutoMounterFileSystem class implementation
//#######################################################################################################################################

/**
    Factory method
*/
CAutoMounterFileSystem* CAutoMounterFileSystem::New()
    {
    CAutoMounterFileSystem* pThis = new CAutoMounterFileSystem();
    return pThis;
    }


CAutoMounterFileSystem::CAutoMounterFileSystem() 
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::CAutoMounterFileSystem() [0x%x]"), this);

    SetState(EInvalid);
    }   

CAutoMounterFileSystem::~CAutoMounterFileSystem()
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::~CAutoMounterFileSystem() [0x%x]"), this);
    }

//-----------------------------------------------------------------------------

/**
    Install iand initialise file system.
*/
TInt CAutoMounterFileSystem::Install()
    {
   
    SetState(ENotInitialised);

    __PRINT1(_L("#<<- CAutoMounterFileSystem::Install() [0x%x]"), this);

    iVersion=TVersion(KF32MajorVersionNumber,KF32MinorVersionNumber,KF32BuildVersionNumber);

    InitialiseFileSystem();

    return SetName(&KFileSystemName_AutoMounter);
    }


//-----------------------------------------------------------------------------
/** 
    Create a new mount control block. 
    This method migh be called by the file server in some unusual cases, when the actual mount is needed only temporarily to get access to the 
    corresponding media driver. E.g. TDrive::ForceRemountDrive()
    Produce the _default_ file system mount
*/
CMountCB* CAutoMounterFileSystem::NewMountL() const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::NewMountL() [0x%x]"), this);
    ASSERT(State() == EInitialised);

    __PRINT1(_L("#<<- producing the _default_ filesystem:%S"), &iFSNames[KDefaultFSNo]);

    CFileSystem* pFS = GetChildFileSystem(KDefaultFSNo);
    ASSERT(pFS);

    return pFS->NewMountL();
    }

//-----------------------------------------------------------------------------
/** 
    Create a new file. 
*/
CFileCB* CAutoMounterFileSystem::NewFileL() const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::NewFileL() [0x%x]"), this);
    Fault(EMustNotBeCalled);
    return NULL;
    }

//-----------------------------------------------------------------------------
/** 
    Create a new directory object 
*/
CDirCB* CAutoMounterFileSystem::NewDirL() const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::NewDirL() [0x%x]"), this);
    Fault(EMustNotBeCalled);
    return NULL;
    }

//-----------------------------------------------------------------------------
/** 
    Create a new media formatter 
*/
CFormatCB* CAutoMounterFileSystem::NewFormatL() const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::NewFormatL() [0x%x]"), this);
    Fault(EMustNotBeCalled);
    return NULL;
    }


//-----------------------------------------------------------------------------

#ifdef _DEBUG
/**
    Called by File Server before deleting File System object.    
*/
TInt CAutoMounterFileSystem::Remove()
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::Remove() [0x%x]"), this);
    return CFileSystem::Remove();
    }

//-----------------------------------------------------------------------------

/**
*/
TBool CAutoMounterFileSystem::QueryVersionSupported(const TVersion& aVer) const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::QueryVersionSupported() [0x%x]"), this);
    return CFileSystem::QueryVersionSupported(aVer);
    }

#endif

//-----------------------------------------------------------------------------

/**
    Find out if drive extensions are supported. In order to have consistent behaviour, _all_ child 
    file systems shall behave the same way.   
    
    @return ETrue if drive extensions are supported.    
*/
TBool CAutoMounterFileSystem::IsExtensionSupported() const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::IsExtensionSupported() [0x%x]"), this);
    
    ASSERT(State() == EInitialised && iFSNames.Count() > 1);


    //-- in debug mode check file systems compatibility: ALL childs must support this feature
    for(TUint i=0; i<iFSNames.Count(); ++i)
        {
        if( !(GetChildFileSystem(i)->IsExtensionSupported()))
            {
            DBG_STATEMENT(Fault(EIncompatibleFileSystems));
            __PRINT(_L("#<<- ::IsExtensionSupported(): Incompatible file sytems!"));
            return EFalse;
            }
        }


    return ETrue; 
    }

//-----------------------------------------------------------------------------

/** 
    Return the initial default path. 
*/
TInt CAutoMounterFileSystem::DefaultPath(TDes& aPath) const
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::DefaultPath() [0x%x]"), this);

    aPath=_L("?:\\");
    aPath[0] = (TUint8) RFs::GetSystemDriveChar();
    return KErrNone;
    }


//-----------------------------------------------------------------------------

/**
    Additional interfaces support.    
*/
TInt CAutoMounterFileSystem::GetInterface(TInt aInterfaceId, TAny*& aInterface, TAny* aInput)
    {

    __PRINT2(_L("#<<- CAutoMounterFileSystem::GetInterface(id:%d) [0x%x]"), aInterfaceId, this);

    switch(aInterfaceId)
        {
        //-- It is this filesystem private interface.
        case EExtendedFunctionality:
        aInterface = (CFileSystem::MFileSystemExtInterface*)this;
        return KErrNone;
        
        //-- a special case for child filesystems.
        //-- ALL of them must respond to this interface query exactly the same way. I.e. It is impossible
        //-- to have some of the child FS supporting it and some not.
        case EProxyDriveSupport:
        return DoProcessProxyDriveSupport();


        default:
        //-- This is the request to other (child file system) from the file server
        //-- Actually, this part must never be called. File Server shall query the file system interfaces _after_ mounting the concrete FS 
        //-- calling TDrive::iCurrentMount->FileSystem().GetInterface() 
        ASSERT(0);
        return CFileSystem::GetInterface(aInterfaceId, aInterface, aInput);
        
        }
    }


//-----------------------------------------------------------------------------

/**
    Find out if _all_ child file systems support the proxy drive. All childs shall behave exactly the same way.
    @return KErrNone if all child file systems support proxy drives, or KErrNotSupported if all of them do not.
*/
TInt CAutoMounterFileSystem::DoProcessProxyDriveSupport()
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::DoProcessProxyDriveSupport[0x%x]"), this);
    ASSERT(State() == EInitialised);

    const TUint cnt = iFSNames.Count();
    ASSERT(cnt > 1);

    
    //-- query the default filesystem #0
    const TBool bRes = GetChildFileSystem(KDefaultFSNo)->IsProxyDriveSupported();

    //-- query the rest of child filesystems
    for(TUint i=1; i<cnt; ++i)
        {
        const TBool b = GetChildFileSystem(i)->IsProxyDriveSupported();

        if(BoolXOR(b, bRes))
            Fault(EIncompatibleFileSystems);
        }


    return  bRes ? KErrNone : KErrNotSupported;
    }

//-----------------------------------------------------------------------------
/**
    Get the child file system name by its index (enumerator).
    
    @param  aFsNumber   index of the child FS 0...KMaxTInt
    @param  aFsName     on success the child file system name will be placed into this buffer  

    @return KErrNone        if there is a child FS name with index 'aFsNumber' (child FS 'aFsNumber' is supported by automounter)
            KErrNotFound    if child FS 'aFsNumber' is not supported
*/
TInt CAutoMounterFileSystem::GetSupportedFileSystemName(TInt aFsNumber, TDes& aFsName) const
    {
    __PRINT2(_L("#<<- CAutoMounterFileSystem::GetSupportedFileSystemName[0x%x](%d)"), this, aFsNumber);

    if(aFsNumber == RFs::KRootFileSystem)
        {//-- this is a name query for "root filesystem" or automounter
        aFsName = Name(); //-- ourselves
        return KErrNone;
        }
    
    //-- this is a query for one of the child filesystems
    if((TUint)aFsNumber < iFSNames.Count())
        {
        aFsName = iFSNames[aFsNumber]; 
        return KErrNone;
        }

    
    return KErrNotFound;
    }


//-----------------------------------------------------------------------------
/**
    This is the only factory method that can be called by file server for this file system.
    In this method the automounter sequentially tries to mount every child and on success produces the corresponding CMountCB object.

    @param  apDrive         pointer to the TDrive, child FS will need this to access media.
    @param  apFileSystem    on return will contain the pointer to the CFileSystem that has produced the proped CMountCB if 
                            one of the child file system has recognised the volume.
    @param  aForceMount     if ETrue the appropriate child FS (designated by aFsNameHash) will be forcedly mounted on the volume. for volume formatting purposes.
    @param  aFsNameHash     if !=0 specifies the file system name, see TVolFormatParam::CalcFSNameHash(). 0 means "file system name is not specified"

    @return pointer to the constructed CMountCB by one of the child file systems (and pointer to this child FS in apFileSystem)
            NULL if it was impossible to produce proper CMountCB object.

*/
CMountCB* CAutoMounterFileSystem::NewMountExL(TDrive* apDrive, CFileSystem** apFileSystem, TBool aForceMount, TUint32 aFsNameHash)
    {
    __PRINT4(_L("#<<- CAutoMounterFileSystem::NewMountExL[0x%x] drv:%d, ForceMount:%d, FSNameHash:0x%x"), this, apDrive->DriveNumber(), aForceMount, aFsNameHash);

    ASSERT(State() == EInitialised && apDrive);

    //-- This method is usually called from the appropriate drive thread; this file system is intended to be bound to
    //-- removable drives. Having removable drive runnind in the main file server thread means that something is terribly wrongly configured.
    if(apDrive->IsSynchronous())
        Fault(EWrongDriveAttributes);

    if(iFSNames.Count() < 2)
        Fault(EWrongConfiguration);


    //-- if aForceMount is true, this means that the TDrive tries mount the filesystem by force for formatting because normal mounting has failed before. 
    //-- in our case it means that the file system on the volume hadn't been recognised by any child FS.
    //-- aFsNameHash shall designate the file system to be forcedly mounted. Depending on this the appropriat CMounCB object will be produced.
    //-- if aFsNameHash is 0, i.e. not provided, this method will fail with KErrNotFound because it is impossible to select appropriat child FS.
    if(aForceMount)
        {
        if(aFsNameHash == 0)
            {//-- the file system to mount forcedly is not specified
            __PRINT(_L("#<<- Unable to select appropriate child FS for formatting!"));
            User::Leave(KErrNotFound);
            }
        else
            {//-- try to find appropriate child FS by its name hash
            CFileSystem *pFS = GetChildFileSysteByNameHash(aFsNameHash);
            if(!pFS)
                {
                __PRINT(_L("#<<- no child FS found by its name hash!"));
                ASSERT(0);
                User::Leave(KErrNotFound);
                }

            CMountCB* pMount = pFS->NewMountL();
            ASSERT(pMount);

            *apFileSystem = pFS; 
            return pMount;
            }
        }//if(aForceMount)



    //-- try instantiate a new CMountCB depending on the file system on the media

    CMountCB* pMatchedMount;
    TInt nRes = TryMountFilesystem(apDrive, &pMatchedMount, apFileSystem);
    
    if(nRes == KErrNone)
        {
        ASSERT(pMatchedMount);
        return pMatchedMount;
        }



    User::Leave(nRes);
    return NULL;
    }

//-----------------------------------------------------------------------------
/**
    Initialise this file system. Reads and processes configuration, fills in file system names container, etc. 
*/
void CAutoMounterFileSystem::InitialiseFileSystem()
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::InitialiseFileSystem() [0x%x]"), this);

    ASSERT(State() == ENotInitialised);

    TInt nRes;
    

    //-- 1. initialise the array of file system names. These names shall be listed in a config string.
    //-- the config string is taken from estart.txt usually and its format is like this: 
    //-- section: [AutoMounter] and property "FSNames fat,exfat"
    //-- in debug version a special text property can override the config string. This allows controlling automounter from
    //-- the test environment.

    TBuf8<0x100> buf(0);   
    TBuf<0x100>  fsName;   


#ifdef _DEBUG
    const TUid KSID_Test1={0x10210EB3}; //-- SID of the test that will define and set test property to control volume mounting
    const TUint KPropKey = 0; //-- property key

    //-- in debug mode the property will override the estart.txt config
    if(RProperty::Get(KSID_Test1, KPropKey, buf) == KErrNone)
        {
        __PRINT(_L("#<<- reading config from the debug propery..."));
        }
    else
#endif
        {
        __PRINT(_L("#<<- reading config from estart.txt..."));
        _LIT8(KSection,  "AutoMounter");
        _LIT8(KProperty, "FSNames");

        nRes = F32Properties::GetString(KSection, KProperty, buf);
        if(!nRes)
            Fault(EPluginInitialise);
        }


    fsName.Copy(buf);
    __PRINT1(_L("#<<- config:'%S'"), &fsName);

    //-- parse CSV config line and fill in the file system names array
    const TChar chDelim = ','; //-- token delimiter, comma
    buf.Trim();
    TPtrC8 ptrCurrLine(buf);
    for(TInt i=0; ;++i)
        {
        const TInt delimPos = ptrCurrLine.Locate(chDelim);
        if(delimPos <= 0)
            {
            fsName.Copy(ptrCurrLine);
            }
        else
            {
            TPtrC8 temp(ptrCurrLine.Ptr(), delimPos);
            fsName.Copy(temp);
            }

        fsName.Trim();
        __PRINT2(_L("#<<- child FS[%d]: '%S'"), i, &fsName);

        
        if(fsName.Length() <= 0) 
            Fault(EPluginInitialise);

        //-- check if the FS name being appended is unique
        for(TUint j=0; j<iFSNames.Count(); ++j)
            {
            if(iFSNames[j] == fsName)
                {
                Fault(EPluginInitialise);
                }
            }
        
        
        nRes = iFSNames.Append(fsName);
        ASSERT(nRes ==KErrNone);
        
        if(delimPos <=0 )
            break;

        ptrCurrLine.Set(ptrCurrLine.Ptr()+delimPos+1, ptrCurrLine.Length()-delimPos-1);
        }


    SetState(EInitialised);

    //-- 2. check that the file server has all filesystems we need instantiated and stored in a global container
    TUint cnt = iFSNames.Count();
    if(cnt < 2)
        {
        __PRINT(_L("#<<- ::InitialiseFileSystem(): too few File Systems bound!"));
        Fault(EPluginInitialise);
        }

    while(cnt--)
        {
        GetChildFileSystem(cnt);
        }


    }



//-----------------------------------------------------------------------------
/**
    Tries to find out if some of the child file systems can be mounted on the given volume.

    @param  apDrive         pointer to the TDrive, child FS will need this to access media.
    @param                  on return will contain the pointer to the CMountCB object if some of the childs has decided that it can be mounted.
    @param  apFS            on return will contain the pointer to the CFileSystem that has produced the proped CMountCB if 

    @return KErrNone on success, otherwise standard error code.

*/
TInt CAutoMounterFileSystem::TryMountFilesystem(TDrive* apDrive, CMountCB** apMount, CFileSystem** apFS)
    {
    __PRINT1(_L("#<<- CAutoMounterFileSystem::TryMountFilesystem()[0x%x]"), this);

    const TInt KNumFS = iFSNames.Count();
  
    ASSERT(State() == EInitialised && (KNumFS >1));


    *apMount = NULL;
    *apFS    = NULL;
    
    
    TInt nRes;
    TInt cntFS;
    CMountCB*       pMountCB = NULL;
    CFileSystem*    pMatchedFS = NULL;

    for(cntFS=0; cntFS < KNumFS; ++cntFS)
        {

        __PRINT2(_L("#<<-@@ trying FS[%d]:%S"), cntFS, &iFSNames[cntFS]);

        CFileSystem* pFS = GetChildFileSystem(cntFS); //-- Find current filesystem object in the FileServer's global container

        //-- 2. create CMountCB instance and set it up
        pMountCB = NULL;
        
        TRAP(nRes, pMountCB = pFS->NewMountL());
        if(nRes != KErrNone)
            {
            return KErrNoMemory;
            }

        ASSERT(pMountCB);
        pMountCB->SetDrive(apDrive);

        
        //-- 2.1 Firstly try using special CMountCB interface to find out if current FS can be mounted on this media
        nRes = pMountCB->CheckFileSystemMountable();
        if(nRes == KErrNotSupported)
            {
            //-- file system doesn't support this feature,
            //-- 2.2 try to check the file system by mounting and dismounting later. It can result in some long activity, like FAT scanning etc.
            TRAP(nRes, pMountCB->MountL(EFalse));
            pMountCB->Dismounted(); //-- dismount the mountCB, it will be required in dismounted state anyway 
            }
        
        //-- 2.3 analyse the result of mounting 
        if(nRes != KErrNone)
            {
            if(nRes == KErrLocked)
                {//-- this is a special case; The media (SD card for example) is locked. 
                //-- Pretend that everything is OK and return CMountCB instance that is produced by the _default_ file system.
                //-- locked media case will be handled by the file server later.
                ASSERT(cntFS == KDefaultFSNo); //-- the default FS is tried first and must detect the locked media
                pMatchedFS = pFS;
                __PRINT(_L("#<<-@@ The media is LOCKED !"));
                break;
                }

            //-- failed to mount the file system, most likey it is not recognised
            pMountCB->Close(); //-- this is a self-destructing object!
            pMountCB = NULL;
            __PRINT(_L("#<<-@@ Mount FAILED !"));
            }
        else
            {
            //-- mounted OK, the file system is recognised
            __PRINT(_L("#<<-@@ Mount OK!"));
            
            pMatchedFS = pFS;
            break;
            }
    
        }//for(cntFS=0; cntFS<KNumFS; ++cntFS)

    if(cntFS >= KNumFS)
        {//-- no one from the FS factories recognised the file system
        __PRINT1(_L("#<<- ::TryMountFilesystem()[0x%x] No file system recognised!"), this);
        
        SetName(&KFileSystemName_AutoMounter); 
        return KErrCorrupt;
        }

    ASSERT(pMountCB && pMatchedFS); 
    *apMount = pMountCB;
    *apFS    = pMatchedFS;
    
    //-- set this FS name to the name of recognised FS. In this case the automounter "pretends" to be one of the child file systems on 
    //-- successful mounting. This behaviour was considered to be incorrect.
    //TPtrC fsName = pMatchedFS->Name();
    //SetName(&fsName); 

    return KErrNone;

    }

//-----------------------------------------------------------------------------
/**
    get the child file system object by the index in file child system names container
    
    @param  aIndex  index in the iFSNames
    @return pointer to the FS object if it is found, NULL otherwise
*/
CFileSystem* CAutoMounterFileSystem::GetChildFileSystem(TUint aIndex) const
    {
    ASSERT(State() == EInitialised && (iFSNames.Count() >1) && aIndex < iFSNames.Count());

    const TDesC& fsName = iFSNames[aIndex]; //-- registered child file system name
    CFileSystem* pFS = GetFileSystem(fsName); //-- Find filesystem object in the FileServer's global container

    if(!pFS)
        {
        __PRINT1(_L("#<<- CAutoMounterFileSystem::GetChildFileSystem() FileServer doesn't have FS:%S Added!"), &fsName);
        Fault(EFileSysNotAdded);
        }

    return pFS;
    }

//-----------------------------------------------------------------------------
/**
    Find the child file system object by the file system name name hash. 
    @param  aFsNameHash FS name hash
    @return pointer to the FS object if it is found, NULL otherwise
*/
CFileSystem* CAutoMounterFileSystem::GetChildFileSysteByNameHash(TUint32 aFsNameHash) const
    {
    ASSERT(State() == EInitialised && (iFSNames.Count() >1) && aFsNameHash);
    
    for(TUint i=0; i<iFSNames.Count(); ++i)
        {
        if(aFsNameHash == iFSNames.GetStringHash(i))
            {
            const TDesC& fsName = iFSNames[i];        //-- registered child file system name
            __PRINT2(_L("#<<- ::GetChildFileSysteByNameHash() found FsName:%S by hash:0x%x"), &fsName, aFsNameHash);
            CFileSystem* pFS = GetFileSystem(fsName); //-- Find filesystem object in the FileServer's global container
            ASSERT(pFS);
            return pFS;
            }
   
        }
   
    __PRINT1(_L("#<<- ::GetChildFileSysteByNameHash() No FS name found by hash:0x%x"), aFsNameHash);
   
    return NULL;
    }




//#######################################################################################################################################
//#     XStringArray implementation
//#######################################################################################################################################

XStringArray::XStringArray()
    {
    }

XStringArray::~XStringArray()
    {
    Reset();
    iStrings.Close();
    }

//-----------------------------------------------------------------------------
void XStringArray::Reset()
    {
    iStrings.ResetAndDestroy();
    }

//-----------------------------------------------------------------------------
const TDesC& XStringArray::operator[](TUint aIndex) const
    {
    if(aIndex >= (TUint)iStrings.Count())
        Panic(EIndexOutOfRange);

    HBufC* des=iStrings[aIndex];
    return *des;
    }

//-----------------------------------------------------------------------------
/**
    Append file system name to the container. The name is converted to upper case.
    @param aString name descriptor.
    @return standard error code
*/
TInt XStringArray::Append(const TDesC& aString)
    {
    HBufC* pBuf = aString.Alloc();
    if(!pBuf) 
        return KErrNoMemory;
    
    //-- string being appended shall be unique
    for(TUint i=0; i<Count(); ++i)
        {
        if(operator[](i).CompareF(aString) == 0)
            {
            ASSERT(0);
            return KErrNone;
            }
        }

    pBuf->Des().UpperCase(); //-- convert the FS name to upper-case in order to correctly calculate hash later
    return iStrings.Append(pBuf);
    }


//-----------------------------------------------------------------------------
/**
    Get child FS name hash by index in the names container.
    @param  aIndex name index in the container
    @return file system name hash (crc32)
*/
TUint32 XStringArray::GetStringHash(TUint aIndex) const
    {
    const TDesC& des = operator[](aIndex);
    return TVolFormatParam::CalcFSNameHash(des);
    }



void XStringArray::Panic(TPanicCode aPanicCode) const
    {
    _LIT(KPanicCat,"XStringArray");
    User::Panic(KPanicCat, aPanicCode);
    }
































