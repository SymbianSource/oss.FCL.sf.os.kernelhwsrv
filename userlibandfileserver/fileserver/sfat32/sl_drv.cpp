// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\sl_drv.cpp
// 
//

#include "sl_std.h"
#include "sl_cache.h"

const TInt KMaxRecoverableRetries=10;
const TInt KMaxCriticalRetries=10;


//---------------------------------------------------------------------------------------------------------------------------------------


TDriveInterface::TDriveInterface() 
                   :iMount(NULL)
{
}

/**
    Initialise the interface object.
    @param  aMount the CFatMountCB that owns this object
*/
TBool TDriveInterface::Init(CFatMountCB* aMount)
{
    ASSERT(aMount);
    iMount = aMount;
	aMount->LocalDrive()->SetMount(aMount);
    return iProxyDrive.Init(aMount->LocalDrive());
}

/**
    Close the interface to the media driver
*/
void TDriveInterface::Close()
{	 
	 if((iMount != NULL) && (iMount->LocalDrive() != NULL))
		{
		ASSERT(iMount->LocalDrive()->Mount() == NULL || iMount->LocalDrive()->Mount() == iMount);
		iMount->LocalDrive()->SetMount(NULL);
		}
     iMount = NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Read data from the media via CProxyDrive interface.
    This is non-critical read: on error Non-critical notifier is involved

    @param  aPos        absolute media position
    @param  aLength     how many bytes to read
    @param  aTrg        data descriptor

    @return KErrNone - success
    @return KErrNotReady - non-critical error
    @return KErrCorrupt - an illegal write is detected
    @return KErrBadPower - failure due to low power

*/
TInt TDriveInterface::ReadNonCritical(TInt64 aPos, TInt aLength, TDes8& aTrg) const
{
    TInt nRes = KErrNone;
    TInt cntRetry = KMaxRecoverableRetries;

    //__PRINT2(_L("#=+++ Read_nc1: pos:%LU, len:%u"), aPos, aLength);

    for(;;)
    {
        nRes = iProxyDrive.Read(aPos,aLength,aTrg);
        if (nRes==KErrNone)
            break;

        __PRINT4(_L("TDriveInterface::ReadNonCritical() failure! drv:%d Posl=%LU len=%d retval=%d"), iMount->DriveNumber(), aPos, aLength, nRes);
        
        if(--cntRetry <= 0)
        {
            nRes = KErrCorrupt;
            break;
        }

        nRes = HandleRecoverableError(nRes);
        if (nRes !=ERetry)
            break;
    }

    return nRes;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Read data from the media via CProxyDrive interface. 
    This is non-critical read: on error Non-critical notifier is involved

    @param  aPos        absolute media position
    @param  aLength     how many bytes to read
    @param  aTrg        data descriptor
    @param  aMessage
    @param  anOffset

    @return KErrNone - success
    @return KErrNotReady - non-critical error
    @return KErrCorrupt - an illegal write is detected
    @return KErrBadPower - failure due to low power

*/
TInt TDriveInterface::ReadNonCritical(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const
{
    //__PRINT2(_L("#=+++ Read_nc2: pos:%LU, len:%u"), aPos, aLength);

    TInt nRes = KErrNone;
    TInt cntRetry = KMaxRecoverableRetries;

    for(;;)
    {
        nRes = iProxyDrive.Read(aPos, aLength, aTrg, aMessage, anOffset, aFlag);
        if (nRes==KErrNone)
            break;

        __PRINT4(_L("TDriveInterface::ReadNonCritical() Failure! drv:%d aPosl=%d len=%d anOffset=%d"), iMount->DriveNumber(), aPos,aLength, anOffset);
        
        if(--cntRetry <= 0)
        {
            nRes = KErrCorrupt;
            break;
        }

        nRes = HandleRecoverableError(nRes);
        if (nRes !=ERetry)
            break;
    }

    return nRes;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Read data from the media via CProxyDrive interface with a critical notifier.
    This method shall be used to read critical filesystem data, such as directory entries, FAT data.

    @param  aPos        absolute media position
    @param  aLength     how many bytes to read
    @param  aTrg        data descriptor

    @return KErrNone - success
    @return KErrNotReady - non-critical error
    @return KErrCorrupt - an illegal write is detected
    @return KErrAbort - user aborted read
*/
TInt TDriveInterface::ReadCritical(TInt64 aPos,TInt aLength,TDes8& aTrg) const
{
    //__PRINT2(_L("#=+++ Read_C: pos:%LU, len:%u"), aPos, aLength);

    TInt nRes = KErrNone;

    for(;;)
    {
        nRes = iProxyDrive.Read(aPos, aLength, aTrg);
		if(nRes == KErrNone)
            break;

		__PRINT4(_L("TDriveInterface::ReadCritical() Error! drv:%d Posl=%LU len=%d retval=%d"), iMount->DriveNumber(), aPos, aLength, nRes);
		
        nRes=HandleCriticalError(nRes);
		if (nRes != ERetry)
            break;
    }

    return nRes;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Write data to the media via CProxyDrive interface.
    
    @param  aPos        absolute media position
    @param  aLength     how many bytes to write
    @param  aSrc        pointer to the data 
    @param  aMessage
    @param  anOffset
    
    @return KErrNone - success
    @return KErrNotReady - non-critical error
    @return KErrBadPower - write not attempted due to low batteries
    @return KErrCorrupt - an illegal write is detected
    @return KErrAccessDenied - write to protected media
*/
TInt TDriveInterface::WriteNonCritical(TInt64 aPos, TInt aLength, const TAny* aSrc, const RMessagePtr2 &aMessage, TInt anOffset, TUint aFlag)
{
    //__PRINT2(_L("#=+++ Write_NC: pos:%LU, len:%u"), aPos, aLength);

    
    TInt nRes = KErrNone;
    TInt cntRetry = KMaxRecoverableRetries;

    for(;;)
    {
        iMount->OpenMountForWrite(); //-- make a callback to CFatMountCB to perform some actions on 1st write.
        nRes = iProxyDrive.Write(aPos, aLength, aSrc, aMessage, anOffset, aFlag);
        if (nRes==KErrNone)
            break;

        __PRINT4(_L("TDriveInterface::WriteNonCritical() failure! drv:%d, Pos=%LU len=%d anOffset=%d"), iMount->DriveNumber(), aPos, aLength, anOffset);
        
        if(--cntRetry <= 0)
        {
            nRes = KErrCorrupt;
            break;
        }

        nRes = HandleRecoverableError(nRes);
        if (nRes !=ERetry)
            break;
    }


    return nRes;
}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Write data to the media via CProxyDrive interface. On error this method can invoke a critical notifier.
    This method is intended to be called for the filesstem critical data, i.e. FAT metadata, such as directory entries,
    FAT table etc.
    
    @param  aPos  absolute media position
    @param  aSrc  descriptor with the source data
    
    @return KErrNone - success
    @return KErrNotReady - non-critical error
    @return KErrBadPower - write not attempted due to low batteries
    @return KErrCorrupt - an illegal write is detected
    @return KErrAccessDenied - write to protected media
*/
TInt TDriveInterface::WriteCritical(TInt64 aPos, const TDesC8& aSrc)
{
    //__PRINT2(_L("#=+++ Write_C: pos:%LU, len:%u"), aPos, aSrc.Length());

    TInt nRes = KErrNone;

#ifdef _DEBUG
    
    TBool simulatedWriteFailure = EFalse; //-- if true it means that the write failure has been simulated

    //-- debug interface to simulate write failure
	if(iMount->IsWriteFail())
    {
		if(iMount->WriteFailCount() != 0)
        {
            iMount->DecWriteFailCount();
        }
        else
		{//-- simulate write failure
			if(iMount->WriteFailError()==-99)
				UserSvr::ResetMachine(EStartupWarmReset);
			else
			{
			    //-- invalidate caches, because actual write to the drive isn't going to happen
                if(iMount->RawDisk().DirCacheInterface())
                    iMount->RawDisk().DirCacheInterface()->InvalidateCache();

                iMount->SetWriteFail(EFalse);
				
                TRAP_IGNORE(iMount->RawDisk().InvalidateUidCache()); //-- invalidate whole UID data cache
                TRAP_IGNORE(iMount->FAT().InvalidateCacheL());       //-- invalidate whole FAT cache
                
                iMount->InvalidateLeafDirCache();

                nRes = iMount->WriteFailError(); 
                simulatedWriteFailure = ETrue; //-- won't perform actual write later
                __PRINT4(_L("TDriveInterface::WriteCritical() Simulating write failure. drv:%d, aPos=%LU len=%d Code=%d"), iMount->DriveNumber(), aPos,aSrc.Length(),nRes);

			}
		}
    }//if(iMount->IsWriteFail())

    if(!simulatedWriteFailure)
#endif // _DEBUG
    {
        //-- try to write data until success or user gives up
	    for(;;)
	    {
            for(TInt i=0; i<KMaxCriticalRetries; i++)
            {
                iMount->OpenMountForWrite();  //-- make a callback to CFatMountCB to perform some actions on 1st write.
                nRes=iProxyDrive.Write(aPos,aSrc);
			    if (nRes==KErrNone)
                    return nRes;
		    }

            //-- write error occured
            __PRINT4(_L("TDriveInterface::WriteCritical() failure! drv:%d, aPos=%LU len=%d retval=%d"), iMount->DriveNumber(), aPos,aSrc.Length(),nRes);

            nRes=HandleCriticalError(nRes);
            if (nRes!=ERetry)
                break;

	    }//for(;;)
    
    }// if(!simulatedWriteFailure)
    
    return nRes;
}


/**
    Get Last Error Info from the proxy drive
        
    @param  aErrorInfo data descriptor for the error info.
    @return KErrNone - success, interrogate aErrorInfo for further info
    @return KErrNotSupported - media driver does not support
*/
TInt TDriveInterface::GetLastErrorInfo(TDes8& aErrorInfo) const
{
    return iProxyDrive.GetLastErrorInfo(aErrorInfo);
}


//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Handle critical error

    @param aResult result from the media driver (error code)

    @return ERetry - Attempt operation again
    @return KErrAbort - User aborted notifier
    @return KErrAccessDenied - media is read only
    @return KErrCorrupt - cf-card is corrupt
*/
TInt TDriveInterface::HandleCriticalError(TInt aResult) const
	{
    __PRINT2(_L("TDriveInterface::HandleCriticalError drv:%d, code:%d"), iMount->DriveNumber(),aResult);

	TLocaleMessage line1;
	TLocaleMessage line2;

	TInt r=KErrAbort;

	if (aResult==KErrLocked)
	{
		r=KErrLocked;
		goto End;
	}

	if (aResult==KErrAccessDenied)
		{
		r=KErrAccessDenied;
		goto End;
		}
	
	if (aResult==KErrArgument || aResult==KErrBadDescriptor)
		{
		r=KErrCorrupt;
		goto End;
		}

	if (iMount->Drive().IsChanged())
		{//-- check if the media we accessing is the same as it used to be
          if(iMount->CheckVolumeTheSame())
        	{//-- the media is the same
			if(!IsDriveWriteProtected())
				{
				iMount->Drive().SetChanged(EFalse);
				r=ERetry;
				goto End;
				}
			}
		}

	if (aResult==KErrAbort && !iMount->Drive().IsChanged())
		{
		r=ERetry;
		goto End;
		}

	if (aResult==KErrBadPower)
		{
		line1=EFileServer_LowPowerLine1;
		line2=EFileServer_LowPowerLine2;
		}
	else if (iMount->Drive().IsChanged())
		{
		line1=EFileServer_PutTheCardBackLine1;
		line2=EFileServer_PutTheCardBackLine2;
		}
	else
		{
		line1=EFileServer_DiskErrorLine1;
		line2=EFileServer_DiskErrorLine2;
		}
	
	if (NotifyUser())
		{
		FOREVER
			{
			TInt buttonVal;
			TInt ret=iMount->Notifier()->Notify(TLocaleMessageText(line1),
												TLocaleMessageText(line2),
												TLocaleMessageText(EFileServer_Button1),
												TLocaleMessageText(EFileServer_Button2),
												buttonVal);
			if (ret!=KErrNone)
				break; 
			if (buttonVal!=1)
				break; // Abort

			if (iMount->Drive().IsChanged())
				{
//
// Without this code, retry will indiscriminately write over whatever disk happens to be present.
// However if the write error is to the bootsector remounting will always fail because the boot
// sector will have changed and hence the disk is useless.
// 
                if(!iMount->CheckVolumeTheSame())
                    continue; //-- the media isn't the same as originally mounted; continue asking

                if(IsDriveWriteProtected())
                    continue; //-- still can not write to the drive


				iMount->Drive().SetChanged(EFalse);
				}

			r=ERetry; // Retry
			break;
			}
		}
End:
	return(r);
	}

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Handle recoverable error

    @param aResult result from the media driver (error code)

    @return ERetry - retry write
    @return KErrCorrupt - media is corrupt
    @return KErrBadPower - low power failure
    @return KErrNotReady - non-critical error
*/
TInt TDriveInterface::HandleRecoverableError(TInt aResult) const
	{
	__PRINT2(_L("TDriveInterface::HandleRecoverableError drv:%d, code:%d"), iMount->DriveNumber(),aResult);

	if (aResult==KErrAccessDenied)
		return(KErrAccessDenied);
	if (aResult == KErrLocked)
		return KErrLocked;
	if (aResult==KErrArgument || aResult==KErrBadDescriptor)
		return(KErrCorrupt);
	if (aResult==KErrBadPower)
		return(KErrBadPower);
	if (aResult==KErrDied)	// client thread died
		return(KErrDied);
	if (iMount->Drive().IsChanged())
		{

        if(! iMount->CheckVolumeTheSame())
            {//-- the media is different now.
            return KErrNotReady;
			}
		else if(!IsRecoverableRemount())
			{
			return KErrAccessDenied;
			}
		}
	return(ERetry);
	}	

/** @return true if the mount can be remounted for a recoverable error */
TBool TDriveInterface::IsRecoverableRemount() const
	{
	if(IsDriveWriteProtected()&&(iMount->Drive().IsWriteableResource()||iMount->Drive().IsCurrentWriteFunction()))
		return(EFalse);
	return(ETrue);
	}

/** return true if the media is write protected */
TBool TDriveInterface::IsDriveWriteProtected() const
	{
	TLocalDriveCapsV2Buf localDriveCaps;
    TInt r=iProxyDrive.Caps(localDriveCaps);

	if(r!=KErrNone)
		return(EFalse);

	return((localDriveCaps().iMediaAtt&KMediaAttWriteProtected)!=0);
	}



//---------------------------------------------------------------------------------------------------------------------------------------

TDriveInterface::XProxyDriveWrapper::XProxyDriveWrapper() 
                   :iLocalDrive(0) 
{
    TInt nRes = iLock.CreateLocal();
    ASSERT(nRes == KErrNone);
    (void)nRes; 
}


TDriveInterface::XProxyDriveWrapper::~XProxyDriveWrapper() 
{
    iLock.Close();
}

/** 
    Initialise interface wrapper.
    @param  aProxyDrive pointer to the raw drive access interface
    @return true on success
*/
TBool TDriveInterface::XProxyDriveWrapper::Init(CProxyDrive* aProxyDrive) 
{
    ASSERT(aProxyDrive);
    if(!iLock.Handle()) //-- the mutex must have been created by constructor
        return EFalse;
    
    iLocalDrive = aProxyDrive;
    return ETrue;
}

//-- see original TDriveInterface methods

TInt TDriveInterface::XProxyDriveWrapper::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->Read(aPos, aLength, aTrg, aMessage.Handle(), anOffset, aFlag);
    LeaveCriticalSection();
    return nRes;
}
       
TInt TDriveInterface::XProxyDriveWrapper::Read(TInt64 aPos,TInt aLength,TDes8& aTrg) const
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->Read(aPos, aLength, aTrg);
    LeaveCriticalSection();
    return nRes;
}

TInt TDriveInterface::XProxyDriveWrapper::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag)
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->Write(aPos, aLength, aSrc, aMessage.Handle(), anOffset, aFlag);
    LeaveCriticalSection();
    return nRes;
}

TInt TDriveInterface::XProxyDriveWrapper::Write(TInt64 aPos, const TDesC8& aSrc)
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->Write(aPos, aSrc);
    LeaveCriticalSection();
    return nRes;
}

TInt TDriveInterface::XProxyDriveWrapper::GetLastErrorInfo(TDes8& aErrorInfo) const
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->GetLastErrorInfo(aErrorInfo);
    LeaveCriticalSection();
    return nRes;
}

TInt TDriveInterface::XProxyDriveWrapper::Caps(TDes8& anInfo) const
{
    EnterCriticalSection();
    TInt nRes = iLocalDrive->Caps(anInfo);
    LeaveCriticalSection();
    return nRes;
}












