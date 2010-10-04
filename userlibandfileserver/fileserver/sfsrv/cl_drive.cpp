// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_drive.cpp
// 
//

#include "cl_std.h"




EXPORT_C TDriveUnit::TDriveUnit(TInt aDrive)
/**
Constructor taking a drive number.

@param aDrive The drive number.

@panic FSCLIENT 0 if aDrive is greater than or equal to KMaxDrives or less than 0.

@see KMaxDrives
*/
	{
	__ASSERT_ALWAYS((aDrive>=0 && aDrive<KMaxDrives),Panic(EDriveUnitBadDrive));
	iDrive=aDrive;
	}




EXPORT_C TDriveUnit::TDriveUnit(const TDesC& aDriveText)
/**
Constructor taking a drive letter.

@param aDriveText A descriptor containing text whose first character is
                  the drive letter. Can be upper or lower case. Trailing text
                  is ignored.
                  
@panic FSCLIENT 1 if the drive letter is invalid, i.e. does not correspond
       to a drive number.
       
@see RFs::CharToDrive
*/
	{
	__ASSERT_ALWAYS(RFs::CharToDrive(aDriveText[0],iDrive)==0,Panic(EDriveUnitBadDriveText));
	}




EXPORT_C TDriveUnit& TDriveUnit::operator=(TInt aDrive)
/**
Assigns the drive number to the drive unit

@param aDrive The new drive number.

@return A reference to this drive unit. 

@panic FSCLIENT 0 if aDrive is greater than or equal to KMaxDrives.

@see KMaxDrives
*/
	{
	__ASSERT_ALWAYS(aDrive<KMaxDrives,Panic(EDriveUnitBadDrive));
	iDrive=aDrive;
	return *this;
	}




EXPORT_C TDriveUnit& TDriveUnit::operator=(const TDesC& aDriveText)
/**
Assigns a drive letter to the drive unit.

The letter must be between A and Z or a panic is raised. Any trailing
text within the descriptor is ignored.

@param aDriveText Descriptor containing text whose first character is
                  the drive letter. It can be upper or lower case.
                  
@return A reference to this drive unit. 

@panic FSCLIENT 1 if the drive letter is invalid, i.e. does not correspond
       to a drive number.
       
@see RFs::CharToDrive                  
*/
	{
	__ASSERT_ALWAYS(RFs::CharToDrive(aDriveText[0],iDrive)==0,Panic(EDriveUnitBadDriveText));
	return *this;
	}




EXPORT_C TDriveName TDriveUnit::Name() const
/**
Gets the drive unit as text.

The drive letter is returned with a trailing colon.

@return The drive letter and a trailing colon.

@panic FSCLIENT 0 if RFs::DriveToChar() returned an error.
*/
	{
	TChar driveLetter;
	TInt r = RFs::DriveToChar(iDrive,driveLetter);
	__ASSERT_ALWAYS(r == KErrNone, Panic(EDriveUnitBadDrive));
	TDriveName driveName;
	driveName.SetLength(2);
	driveName[0]=(TText)driveLetter;
	driveName[1]=KDriveDelimiter;
	return driveName;
	}



//-------------------------------------------------------------------------------------------------------------------

/** 
    internal class of the CFsMountHelper, provides the real implementation, It is not supposed to 
    be instantiated by anyone except CFsMountHelper.
*/
class TFsMntHelperImpl
    {
    friend class CFsMountHelper;
   
 private:
    
    TFsMntHelperImpl(RFs& aFs, TInt aDrvNum);
        
    //------ 1:1 interface to the host CFsMountHelper class
    void Init(); 
    TInt GetMountProperties();
    TInt MountFileSystem() const;
    void DismountFileSystem(TRequestStatus& aStat, CFsMountHelper::TFsDismountMode aDismountMode) const;
    //------
    
 private:
    TBool DataValid() const {return iFsName.Length() >0;}
    TBool operator==(const TFsMntHelperImpl& aRhs) const;

    enum {KPrimExtIdx =0}; //-- index of the primary extension name in the iExtNames array

    const TDesC& FSysName() const       {return iFsName;}
    const TDesC& PrimaryExtName() const {return iExtNames[KPrimExtIdx];}

    /** bits in a error bitmap that indicate some, potentially multiple, API failure reasons */
    enum TErrorBits
        {
        Err_GetFsName       = 0x0001, ///< Error when trying to obtain FS name (possible reason: there is no FS at all on this drive)
        Err_MountFs         = 0x0002, ///< Error when mounting the FS. (possible reason: the FS layout on the media is corrupt or not recognised)
        Err_MountFsPrimExt  = 0x0004, ///< Error when mounting the FS with the primary extension
        Err_MountSecExt     = 0x0008, ///< Error when mounting the secondary extension 
        };

        
    void  AddErrorBit(TUint32 aFlag) const {ASSERT(aFlag); iErrorBitmap |= aFlag;}

    /** panic codes */
    enum TPanicCode
        {
        ENotInitialised,  ///< the instance of the implementation is not created
        EDrvNumberInvalid,///< invalid drive number provided
        ENotImplemented   ///< the functionality is not implemented
        };

    void Panic(TPanicCode aPanicCode) const;

 private:
        RFs&    iFs;                            ///< reference to the file server session
        TInt    iDrvNum;                        ///< drive number
        TFSName iFsName;                        ///< file system name.
        TFSName iExtNames[KMaxExtensionCount];  ///< [0] is a primary ext. name + up to several secondary ones
        TBool   iDrvSynch;                      ///< true if the drive is synchronous
        mutable TUint32 iErrorBitmap;           ///< 32 bits indicating API call failures. '1' bit corresponds to some particular reason. See TErrorBits.  
    };


//-------------------------------------------------------------------------------------------------------------------

void TFsMntHelperImpl::DismountFileSystem(TRequestStatus& aStat, CFsMountHelper::TFsDismountMode aDismountMode) const
    {
    if(!this)
        Panic(ENotInitialised); 

    TInt nRes;

#ifdef _DEBUG
    //-- consistency check (debug mode only). Check that we are dismounting file system with the same parameters
    //-- since last GetMountProperties() call
    TFsMntHelperImpl  currSnapshot(iFs, iDrvNum);
    currSnapshot.GetMountProperties();
    ASSERT(currSnapshot == *this);

#endif //_DEBUG

    
    nRes = KErrArgument;
    TRequestStatus* pStat = &aStat;

    switch(aDismountMode)
        {
        case CFsMountHelper::ENormal:
            //-- normal graceful dismounting. Will fail with KErrInUse if there are any opened objects, like files, directories etc.
            //-- aStat is completed with the API return code. 
            nRes = iFs.DismountFileSystem(iFsName, iDrvNum);
            User::RequestComplete(pStat, nRes);
        break;

        case CFsMountHelper::EForceImmediate:
            //-- immediate forced dismount. Don't pay attention to any opened objects. All handles will become invalid.
            //-- The user should wait for aStat completion. Though it is very likely that it is completed immediately.
            iFs.NotifyDismount(iDrvNum, aStat, EFsDismountForceDismount); 
        break;

     
        case CFsMountHelper::ENotifyClients:
            //-- attempt to dismount FS with notifying any interested clients.
            iFs.NotifyDismount(iDrvNum, aStat, EFsDismountNotifyClients); 
        break;

        default:
            ASSERT(0);
            User::RequestComplete(pStat, KErrArgument);
        break;
        };

    }


//-------------------------------------------------------------------
TInt TFsMntHelperImpl::GetMountProperties()
    {
    if(!this)
        Panic(ENotInitialised); 
    Init();

    TInt nRes;

    //-- 1. get file system name
    nRes = iFs.FileSystemName(iFsName, iDrvNum);
    if(nRes != KErrNone)
        {
        AddErrorBit(Err_GetFsName); //-- indicate an error 
        return nRes;
        }

    //-- 2. find out if the drive sync/async
    TPckgBuf<TBool> drvSyncBuf;
    nRes = iFs.QueryVolumeInfoExt(iDrvNum, EIsDriveSync, drvSyncBuf);
    if(nRes != KErrNone)
        {//-- pretend that the drive is asynch. in the case of file system being corrupted. this is 99.9% true
        iDrvSynch = EFalse;
        }
    else
        {
        iDrvSynch = drvSyncBuf();
        }

    //-- 3. find out extension names if there are any. Extension #0 is a primary one and up to several secondary ones
    for(TInt i=0; i<KMaxExtensionCount; ++i)
        {
        nRes = iFs.ExtensionName(iExtNames[i], iDrvNum, i);
        if(nRes != KErrNone)
            {
            iExtNames[i].Zero();
            }
        } 
    
   
    return KErrNone;
 
    }

//-------------------------------------------------------------------
TInt TFsMntHelperImpl::MountFileSystem() const
    {
    if(!this)
        Panic(ENotInitialised); 
    
    ASSERT(DataValid());
    
    TInt nRes;
    const TBool bPrimaryExtExists = (PrimaryExtName().Length() >0);

    //-- all possible extensions that have existed before dismounting should be present in the file server context.
    //-- anyway, it's impossible to load them here, because their file names are unknown 

    if(bPrimaryExtExists)
        {//-- there was a primary extension, use special mounting API
        nRes = iFs.MountFileSystem(FSysName(), PrimaryExtName(), iDrvNum, iDrvSynch);
        }
    else
        {
        nRes = iFs.MountFileSystem(FSysName(), iDrvNum, iDrvSynch);
        }

    //-- actually, if nRes != KErrNone, it doesn't _necessarily_ mean that _mounting_ of the file system failed.
    //-- for example, the FS can be bound to the drive OK, but the media can be corrupted. This can happen when the FS
    //-- had been dismounted from such a corrupted media.
    //-- opposite, KErrNotReady is very likely to mean that the removable media is not present.
    
    const TInt nFsMountRes = nRes;
    if(nFsMountRes != KErrNone)
        {
        AddErrorBit(bPrimaryExtExists ?  Err_MountFsPrimExt : Err_MountFs);
        }

    //-- mount secondary extensions if there were any
    TInt nExtMountRes = KErrNone;
    for(TInt i=1; i<KMaxExtensionCount; ++i)
        {
        if(iExtNames[i].Length() >0)
            {
            nRes = iFs.MountExtension(iExtNames[i], iDrvNum);
            if(nRes != KErrNone)
                {//-- indicate that an error happened while installing some secondary extension 
                AddErrorBit(Err_MountSecExt);    
                nExtMountRes = nRes;
                }
            }
        }

    //-- return FS mounting error code if it wasn't OK, otherwise - extension mounting code.
    //-- for more info see error bitmap
    return (nFsMountRes != KErrNone) ? nFsMountRes : nExtMountRes;
    }

//-------------------------------------------------------------------
void TFsMntHelperImpl::Init() 
    {
    if(!this)
        Panic(ENotInitialised); 
    
    iDrvSynch = EFalse;
    iFsName.Zero();
    iErrorBitmap = 0;
             
    for(TInt i=0; i<KMaxExtensionCount; ++i) 
        {
        iExtNames[i].Zero();
        }

    }

//-------------------------------------------------------------------
/**
    Panics.
    @param aPanicCode   a panic code
*/
void TFsMntHelperImpl::Panic(TPanicCode aPanicCode) const
    {
    _LIT(KPanicCat,"CFsMountHelper");
    User::Panic(KPanicCat, aPanicCode);
    }

TFsMntHelperImpl::TFsMntHelperImpl(RFs& aFs, TInt aDrvNum) 
                 :iFs(aFs), iDrvNum(aDrvNum) 
    {
    if(aDrvNum < EDriveA || aDrvNum >EDriveZ)
        Panic(EDrvNumberInvalid);

    Init();
    }

/**
    Debug only method. Compares 2 instances of the implementation
*/
TBool TFsMntHelperImpl::operator==(const TFsMntHelperImpl& aRhs) const
    {
    ASSERT(this != &aRhs);

#ifdef _DEBUG
    

    if(iFsName.CompareF(aRhs.iFsName) !=0)
        return EFalse;

    for(TInt i=0; i<KMaxExtensionCount; ++i)
        {
        if(iExtNames[i].CompareF(aRhs.iExtNames[i]) !=0)
        return EFalse;
        }

    if(!iDrvSynch != !aRhs.iDrvSynch)
        return EFalse;

    return ETrue;

#else //_DEBUG
    (void)aRhs;
    Panic(ENotImplemented);
    return EFalse;

#endif// _DEBUG
}


//-------------------------------------------------------------------
/** 
    Closes the object, deletes the implementation
*/
void CFsMountHelper::Close()
{
    delete ipImpl;
    ipImpl = NULL;
}

//-------------------------------------------------------------------
/**
    Factory function. Produces an object of this class
    
    @param  aFs     file server session
    @param  aDrvNum drive number

    @return pointer to the constructed object or NULL on error.
*/
EXPORT_C CFsMountHelper* CFsMountHelper::New(RFs& aFs, TInt aDrvNum)
{

    CFsMountHelper* pSelf = new CFsMountHelper;
    
    if(pSelf)
        {
        pSelf->ipImpl = new TFsMntHelperImpl(aFs, aDrvNum);
    
        if(!pSelf->ipImpl)
            {
            delete pSelf;
            pSelf = NULL;
            }
        }
    
    return pSelf;
}



//-------------------------------------------------------------------
/** 
    Acqires drive/mount/file system properties that will be used for mounting the file system back.
    @return Standard Error code.
*/
EXPORT_C TInt CFsMountHelper::GetMountProperties()
    {
    return ipImpl->GetMountProperties();
    }

//-------------------------------------------------------------------
/** 
    Mount the file system onto the drive using properties previously acquired by GetMountProperties() call.
    Note that the drive shouldn't have the file system mounted, this API call will fail in this case.

    @return KErrNone if mounting file system + possible extensions was ok
            the result of file system mounting if the file system mounting failed (e.g. because of the damaged media) 
            the result of mounting secondary extension if file system mounted OK, but secondary extension mounting resulted in some error.
*/
EXPORT_C TInt CFsMountHelper::MountFileSystem() const
    {
    return ipImpl->MountFileSystem();
    }

//-------------------------------------------------------------------

/**
    An asynchronous API to dismount the file system on the specified drive.
    Precondition: The drive / file system parameters at the time of this API call must be the same as acquired by the last call of GetMountProperties().
                  This is checked in debug mode to prevent possible inconsistencied when mounting the file system back later.   
                  This means that the GetMountProperties() should be called at least once before any attempt to dismount the file system.
    
    @param  aStat           request status. On completion will contain the dismounting result code.
    @param  aDismountMode   describes the dismounting method. See TFsDismountMode.
        

*/
EXPORT_C void CFsMountHelper::DismountFileSystem(TRequestStatus& aStat, TFsDismountMode aDismountMode/*=ENormal*/) const
    {
    ipImpl->DismountFileSystem(aStat, aDismountMode);
    }


//-------------------------------------------------------------------
/**
    A simplified synchronous version of the DismountFileSystem() API. 
    Works absolutely the same as RFs::DismountFileSystem()
    
    @return RFs::DismountFileSystem() result code
*/
EXPORT_C TInt CFsMountHelper::DismountFileSystem() const
    {
    TRequestStatus stat;    
    DismountFileSystem(stat, ENormal);
    User::WaitForRequest(stat);
    
    return stat.Int();
    }



EXPORT_C CFsMountHelper::~CFsMountHelper() 
    {
    Close();
    } 



