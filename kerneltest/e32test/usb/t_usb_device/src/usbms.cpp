// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// USB Mass Storage Application
//
//

/**
 @file
*/

#include "general.h"
#include "config.h"
#include "activecontrol.h"

#include <usbmsshared.h>
#include <massstorage.h>

#include "usbms.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "usbmsTraces.h"
#endif


extern CActiveControl* gActiveControl;
extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gTempTest;


LOCAL_D RFs fs;
LOCAL_D TBuf<0x40> mountList;

LOCAL_D TFixedArray<TBool, KMaxDrives>                   msfsMountedList;  ///< 'true' entry corresponds to the drive with mounted MSFS.FSY
LOCAL_D TFixedArray<CFileSystemDescriptor*, KMaxDrives>  unmountedFsList;  ///< every non-NULL entry corresponds to the unmounted original FS for the drive

LOCAL_D RUsbMassStorage UsbMs;

LOCAL_D CUsbWatch * usbWatch;

static const TUint KNumPropWatch = 4;
LOCAL_D CPropertyWatch * propWatch[KNumPropWatch];

_LIT(KMsFsy, "MSFS.FSY");
_LIT(KMsFs, "MassStorageFileSystem");
_LIT(KOk,"OK");
_LIT(KError,"Error");
_LIT(KBytesTransferredFmt, "%c:%d/%d \n");
_LIT(KErrFmt, "Error: %d\r");
_LIT(KConfigured,"Configured");
_LIT(KNotConfigured,"NOT Configured");


_LIT8(KDefPwd,"123");
TMediaPassword  password(KDefPwd);

//-----------------------------------------------------------------------------

CFileSystemDescriptor::~CFileSystemDescriptor()
    {
    iFsName.Close();
    iPrimaryExtName.Close();
    }

//-----------------------------------------------------------------------------
CFileSystemDescriptor* CFileSystemDescriptor::NewL(const TDesC& aFsName, const TDesC& aPrimaryExtName, TBool aDrvSynch)
    {
    CFileSystemDescriptor* pSelf = new (ELeave) CFileSystemDescriptor;

    CleanupStack::PushL(pSelf);

    pSelf->iFsName.CreateMaxL(aFsName.Length());
    pSelf->iFsName.Copy(aFsName);

    pSelf->iPrimaryExtName.CreateMaxL(aPrimaryExtName.Length());
    pSelf->iPrimaryExtName.Copy(aPrimaryExtName);

    pSelf->iDriveSynch = aDrvSynch;

    CleanupStack::Pop();

    return pSelf;
    }

//-----------------------------------------------------------------------------
/**
    Dismounts the originally mounted FS and optional primary extension from the drive and stores
    this information in the FS descriptor

    @return on success returns a pointer to the instantinated FS descriptor
*/
LOCAL_C CFileSystemDescriptor* DoDismountOrginalFS(RFs& aFs, TInt aDrive)
    {
    TInt        nRes;
    TBuf<128>   fsName;
    TBuf<128>   primaryExtName;
    TBool       bDrvSync = EFalse;

    test.Printf(_L("DoDismountOrginalFS drv:%d\n"), aDrive);
    OstTrace1(TRACE_NORMAL, CFILESYSTEMDESCRIPTOR_NEWL, "DoDismountOrginalFS drv:%d\n", aDrive);

    //-- 1. get file system name
    nRes = aFs.FileSystemName(fsName, aDrive);
    if(nRes != KErrNone)
        {//-- probably no file system installed at all
        return NULL;
        }

    //-- 2. find out if the drive sync/async
    TPckgBuf<TBool> drvSyncBuf;
    nRes = aFs.QueryVolumeInfoExt(aDrive, EIsDriveSync, drvSyncBuf);
    if(nRes == KErrNone)
        {
        bDrvSync = drvSyncBuf();
        }

    //-- 3. find out primary extension name if it is present; we will need to add it againt when mounting the FS
    //-- other extensions (non-primary) are not supported yet
    nRes = aFs.ExtensionName(primaryExtName, aDrive, 0);
    if(nRes != KErrNone)
        {
        primaryExtName.SetLength(0);
        }

    //-- 3.1 check if the drive has non-primary extensions, fail in this case, because this FS can't be mounted back normally
    nRes = aFs.ExtensionName(primaryExtName, aDrive, 1);
    if(nRes == KErrNone)
        {
        test.Printf(_L("DoDismountOrginalFS Non-primary extensions are not supported!\n"));
        OstTrace0(TRACE_NORMAL, CFILESYSTEMDESCRIPTOR_NEWL_DUP01, "DoDismountOrginalFS Non-primary extensions are not supported!\n");
        return NULL;
        }

    test.Printf(_L("DoDismountOrginalFS FS:%S, Prim ext:%S, synch:%d\n"), &fsName, &primaryExtName, bDrvSync);
    OstTraceExt3(TRACE_NORMAL, CFILESYSTEMDESCRIPTOR_NEWL_DUP02, "DoDismountOrginalFS FS:%S, Prim ext:%S, synch:%d\n", fsName, primaryExtName, bDrvSync);

    //-- create FS descriptor and dismount the FS
    CFileSystemDescriptor* pFsDesc = NULL;

    TRAP(nRes, pFsDesc = CFileSystemDescriptor::NewL(fsName, primaryExtName, bDrvSync));
    if(nRes != KErrNone)
        return NULL; //-- OOM ?

    nRes = aFs.DismountFileSystem(fsName, aDrive);
    if(nRes != KErrNone)
        {
        delete pFsDesc;
        pFsDesc = NULL;
        test.Printf(_L("DoDismountOrginalFS Dismounting Err:%d\n"), nRes);
        OstTrace1(TRACE_NORMAL, CFILESYSTEMDESCRIPTOR_NEWL_DUP03, "DoDismountOrginalFS Dismounting Err:%d\n", nRes);
        }

    return pFsDesc;
}

//-----------------------------------------------------------------------------
/**
    Tries to restore the original FS on the drive using the FS descriptor provided
    @return standard error code.
*/
LOCAL_C TInt DoRestoreFS(RFs& aFs, TInt aDrive, CFileSystemDescriptor* apFsDesc)
    {
    TInt nRes;

    test.Printf(_L("DoRestoreFS drv:%d\n"), aDrive);
    OstTrace1(TRACE_NORMAL, DORESTOREFS_DORESTOREFS, "DoRestoreFS drv:%d\n", aDrive);

    //-- 1. check that there is no FS installed
    TBuf<128>   fsName;
    nRes = aFs.FileSystemName(fsName, aDrive);
    if(nRes == KErrNone)
        {//-- there is a file system already installed
		test.Printf(_L("DoRestoreFS This drive already has FS intalled:%S \n"), &fsName);
		OstTraceExt1(TRACE_NORMAL, DORESTOREFS_DORESTOREFS_DUP01, "DoRestoreFS This drive already has FS intalled:%S \n", fsName);
        return KErrAlreadyExists;
        }

    TPtrC ptrN  (apFsDesc->FsName());
    TPtrC ptrExt(apFsDesc->PrimaryExtName());
    test.Printf(_L("DoRestoreFS Mounting FS:%S, Prim ext:%S, synch:%d\n"), &ptrN, &ptrExt, apFsDesc->DriveIsSynch());
    OstTraceExt3(TRACE_NORMAL, DORESTOREFS_DORESTOREFS_DUP02, "DoRestoreFS Mounting FS:%S, Prim ext:%S, synch:%d\n", ptrN, ptrExt, apFsDesc->DriveIsSynch());

    if(ptrExt.Length() >0)
        {//-- there is a primary extension to be mounted
        nRes = aFs.AddExtension(ptrExt);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
            {
            return nRes;
            }

        nRes = aFs.MountFileSystem(ptrN, ptrExt, aDrive, apFsDesc->DriveIsSynch());
        }
    else
        {
        nRes = aFs.MountFileSystem(ptrN, aDrive, apFsDesc->DriveIsSynch());
        }

    if(nRes != KErrNone)
        {
        test.Printf(_L("DoRestoreFS Mount failed! code:%d\n"),nRes);
        OstTrace1(TRACE_NORMAL, DORESTOREFS_DORESTOREFS_DUP03, "DoRestoreFS Mount failed! code:%d\n",nRes);
        }

    return nRes;
    }


//-----------------------------------------------------------------------------
/**
    Dismount the original FS from the drive and mount MsFS instead
*/
LOCAL_C void MountMsFs(TInt driveNumber)
	{
	test.Printf(_L("MountMsFs driveNumber=%d\n"), driveNumber);
	OstTrace1(TRACE_NORMAL, MOUNTMSFS_MOUNTMSFS, "MountMsFs driveNumber=%d\n", driveNumber);

    //-- 1. try dismounting the original FS
    CFileSystemDescriptor* fsDesc = DoDismountOrginalFS(fs, driveNumber);
    unmountedFsList[driveNumber] = fsDesc;

    if(fsDesc)
        {
        TPtrC ptrN(fsDesc->FsName());
        test.Printf(_L("drv:%d FS:%S Dismounted OK\n"),driveNumber, &ptrN);
        OstTraceExt2(TRACE_NORMAL, MOUNTMSFS_MOUNTMSFS_DUP01, "drv:%d FS:%S Dismounted OK\n",driveNumber, ptrN);
        }
    else
        {
        test.Printf(_L("drv:%d Dismount FS Failed!\n"),driveNumber);
        OstTrace1(TRACE_NORMAL, MOUNTMSFS_MOUNTMSFS_DUP02, "drv:%d Dismount FS Failed!\n",driveNumber);
        }

    //-- 2. try to mount the "MSFS"
    TInt error;
    error = fs.MountFileSystem(KMsFs, driveNumber);
	test.Printf(_L("MSFS Mount:   %S (%d)\n"), (error?&KError:&KOk), error);
	OstTraceExt2(TRACE_NORMAL, MOUNTMSFS_MOUNTMSFS_DUP03, "MSFS Mount:   %S (%d)\n", (error?KError():KOk()), error);
	if (!error)
		msfsMountedList[driveNumber] = ETrue;

	}

//-----------------------------------------------------------------------------
/**
    Dismount MsFS and mount the original FS
*/
LOCAL_C TInt RestoreMount(TInt driveNumber)
	{
	TInt err = KErrNone;

    //-- 1. try dismounting the "MSFS"
	if (msfsMountedList[driveNumber])
		{
		err = fs.DismountFileSystem(KMsFs, driveNumber);
		test.Printf(_L("MSFS Dismount:%S (%d)\n"), (err?&KError:&KOk), err);
		OstTraceExt2(TRACE_NORMAL, RESTOREMOUNT_RESTOREMOUNT, "MSFS Dismount:%S (%d)\n", (err?KError():KOk()), err);
		if (err)
			return err;

		msfsMountedList[driveNumber] = EFalse;
        }

    //-- 2. try to mount the original FS back
    CFileSystemDescriptor* fsDesc = unmountedFsList[driveNumber];
    if(fsDesc)
        {
        err = DoRestoreFS(fs, driveNumber, fsDesc);

        TPtrC ptrN(fsDesc->FsName());
        test.Printf(_L("%S Mount:    %S (%d)\n"), &ptrN, (err?&KError:&KOk), err);
        OstTraceExt3(TRACE_NORMAL, RESTOREMOUNT_RESTOREMOUNT_DUP01, "%S Mount:    %S (%d)\n", ptrN, (err?KError():KOk()), err);
        delete fsDesc;
        unmountedFsList[driveNumber] = NULL;
        }

	return err;
	}

//////////////////////////////////////////////////////////////////////////////
//
// CPropertyWatch
// An active object that tracks changes to the KUsbMsDriveState properties
//
//////////////////////////////////////////////////////////////////////////////

CPropertyWatch* CPropertyWatch::NewLC(TUsbMsDriveState_Subkey aSubkey, PropertyHandlers::THandler aHandler)
	{
	CPropertyWatch* me=new(ELeave) CPropertyWatch(aHandler);
	CleanupStack::PushL (me);
	me->ConstructL(aSubkey);
	CleanupStack::Pop();
	return me;
	}

CPropertyWatch::CPropertyWatch(PropertyHandlers::THandler aHandler)
	: CActive(0), iHandler(aHandler)
	{}

void CPropertyWatch::ConstructL(TUsbMsDriveState_Subkey aSubkey)
	{
	User::LeaveIfError(iProperty.Attach(KUsbMsDriveState_Category, aSubkey));
	CActiveScheduler::Add(this);
	// initial subscription and process current property value
	RunL();
	}

CPropertyWatch::~CPropertyWatch()
	{
	Cancel();
	iProperty.Close();
	}

void CPropertyWatch::DoCancel()
	{
	iProperty.Cancel();
	}

void CPropertyWatch::RunL()
	{
	// resubscribe before processing new value to prevent missing updates
	iProperty.Subscribe(iStatus);
	SetActive();

	iHandler(iProperty);
	}

//////////////////////////////////////////////////////////////////////////////
//
// CUsbWatch
//
//////////////////////////////////////////////////////////////////////////////

CUsbWatch* CUsbWatch::NewLC(RUsb& aUsb)
	{
	CUsbWatch* me=new(ELeave) CUsbWatch(aUsb);
	CleanupStack::PushL (me);
	me->ConstructL();
	CleanupStack::Pop();
	return me;
	}

CUsbWatch::CUsbWatch(RUsb& aUsb)
	:
	CActive(0),
	iUsb(aUsb),
	iUsbDeviceState(EUsbcDeviceStateUndefined),
	iWasConfigured(EFalse)
	{}

void CUsbWatch::ConstructL()
	{
	CActiveScheduler::Add(this);
	RunL();
	}

CUsbWatch::~CUsbWatch()
	{
	Cancel();
	iUsb.AlternateDeviceStatusNotifyCancel();
	}

void CUsbWatch::DoCancel()
	{
	iUsb.AlternateDeviceStatusNotifyCancel();
	}

static TBool IsDriveConnected(TInt driveStatusIndex)
	{
	TInt driveStatus = PropertyHandlers::allDrivesStatus[2*driveStatusIndex+1];
	return driveStatus >= EUsbMsDriveState_Connected ? ETrue : EFalse;
	}

static TChar DriveNumberToLetter(TInt driveNumber)
	{
	TChar driveLetter = '?';
	fs.DriveToChar(driveNumber, driveLetter);
	return driveLetter;
	}

static TBool IsDriveInMountList(TUint driveLetter)
	{
	TUint16 driveLetter16 = static_cast<TUint16>(driveLetter);
	return KErrNotFound != mountList.Find(&driveLetter16, 1);
	}

void CUsbWatch::RunL()
	{
	gActiveControl->SetMSFinished(EFalse);
	if (gVerbose)
		{
		switch (iUsbDeviceState)
			{
			case EUsbcDeviceStateUndefined : 					// 0
				test.Printf(_L(">> CUSBWatch:Undefined %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL, ">> CUSBWatch:Undefined %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStateAttached :						// 1
				test.Printf(_L(">> CUSBWatch:Attached %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP01, ">> CUSBWatch:Attached %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStatePowered :						// 2
				test.Printf(_L(">> CUSBWatch:Powered %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP02, ">> CUSBWatch:Powered %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStateDefault :						// 3
				test.Printf(_L(">> CUSBWatch:Default %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP03, ">> CUSBWatch:Default %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStateAddress :						// 4
				test.Printf(_L(">> CUSBWatch:Address %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP04, ">> CUSBWatch:Address %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStateConfigured :					// 5
				test.Printf(_L(">> CUSBWatch:Configured %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP05, ">> CUSBWatch:Configured %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			case EUsbcDeviceStateSuspended : 					// 6
				test.Printf(_L(">> CUSBWatch:Suspended %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP06, ">> CUSBWatch:Suspended %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			default :
				test.Printf(_L(">> CUSBWatch:UNKNOWN %S\n"), iWasConfigured ? &KConfigured : &KNotConfigured);
				OstTraceExt1(TRACE_NORMAL, CUSBWATCH_RUNL_DUP07, ">> CUSBWatch:UNKNOWN %S\n", iWasConfigured ? KConfigured() : KNotConfigured());
				break;

			}
		}
	iUsb.AlternateDeviceStatusNotify(iStatus, iUsbDeviceState);
	SetActive();

	// If the cable is disconnected, unmount all the connected drives.
	if(iWasConfigured && iUsbDeviceState == EUsbcDeviceStateUndefined)
		{
		for(TInt i=0; i<PropertyHandlers::allDrivesStatus.Length()/2; i++)
			{
			if(IsDriveConnected(i))
				{
				OstTrace0(TRACE_NORMAL, CUSBWATCH_RUNL_DUP08, "CUsbWatch calling RestoreMount");
				RestoreMount(PropertyHandlers::allDrivesStatus[2*i]);
				}
			}

		iWasConfigured = EFalse;
		}

	// If cable is connected, mount all drives in the auto-mount list.
	// This is done for performance, since if this is not done here,
	// mounting will happen later after each drive enters the
	// Connecting state.
	if(iUsbDeviceState == EUsbcDeviceStateConfigured)
		{
		for(TInt i=0; i<PropertyHandlers::allDrivesStatus.Length()/2; i++)
			{
			TInt driveNumber = PropertyHandlers::allDrivesStatus[2*i];
			if(!IsDriveConnected(i) && IsDriveInMountList(DriveNumberToLetter(driveNumber)))
				{
				OstTrace0(TRACE_NORMAL, CUSBWATCH_RUNL_DUP09, "CUsbWatch calling MountMsFs");
				MountMsFs(driveNumber);
				}
			}

		iWasConfigured = ETrue;
		}
	}

//////////////////////////////////////////////////////////////////////////////
//
// PropertyHandlers
//
//////////////////////////////////////////////////////////////////////////////

TBuf8<16> PropertyHandlers::allDrivesStatus;
TUsbMsBytesTransferred PropertyHandlers::iKBytesRead;
TUsbMsBytesTransferred PropertyHandlers::iKBytesWritten;
TInt PropertyHandlers::iMediaError;

void PropertyHandlers::Read(RProperty& aProperty)
	{
	Transferred(aProperty, iKBytesRead);
	}

void PropertyHandlers::Written(RProperty& aProperty)
	{
	Transferred(aProperty, iKBytesWritten);
	}

void PropertyHandlers::Transferred(RProperty& aProperty, TUsbMsBytesTransferred& aReadOrWritten)
	{
	TInt err = aProperty.Get(aReadOrWritten);
	if(err == KErrNone)
		{
		for(TInt i = 0; i < allDrivesStatus.Length()/2; i++)
			{
			if (gVerbose)
				{
				test.Printf(KBytesTransferredFmt,
						(char)DriveNumberToLetter(allDrivesStatus[2*i]), iKBytesRead[i], iKBytesWritten[i]);
				OstTraceExt3(TRACE_NORMAL, PROPERTYHANDLERS_TRANSFERRED, "%c:%d/%d \n",
						(char)DriveNumberToLetter(allDrivesStatus[2*i]), iKBytesRead[i], iKBytesWritten[i]);
				}
			}
		}
	else
		{
		test.Printf(KErrFmt, err);
		OstTrace1(TRACE_NORMAL, PROPERTYHANDLERS_TRANSFERRED_DUP01, "Error: %d\r", err);
		}
	}

void PropertyHandlers::DriveStatus(RProperty& aProperty)
	{
	if (gVerbose)
		{
		test.Printf(_L(">> PropertyHandlers::DriveStatus"));
		OstTrace0(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS, ">> PropertyHandlers::DriveStatus");
		}
	TInt err = aProperty.Get(allDrivesStatus);
	if(err == KErrNone)
		{
		if (gVerbose)
			{
			test.Printf(_L(" Status:  "));
			OstTrace0(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP01, " Status:  ");
			}
		for(TInt i = 0; i < allDrivesStatus.Length()/2; i++)
			{
			TInt driveNumber = allDrivesStatus[2*i];
			TInt driveStatus = allDrivesStatus[2*i+1];
			TChar driveLetter = DriveNumberToLetter(driveNumber);

			if (gVerbose)
				{
				switch(driveStatus)
					{
					case EUsbMsDriveState_Disconnected:
						{
						test.Printf(_L("%c:%d:Disconnected\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP02, "%c:%d:Disconnected\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Connecting:
						{
						test.Printf(_L("%c:%d:Connecting\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP03, "%c:%d:Connecting\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Connected:
						{
						test.Printf(_L("%c:%d:Connected\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP04, "%c:%d:Connected\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Disconnecting:
						{
						test.Printf(_L("%c:%d:Disconnecting\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP05, "%c:%d:Disconnecting\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Active:
						{
						test.Printf(_L("%c:%d:Active\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP06, "%c:%d:Active\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Locked:
						{
						test.Printf(_L("%c:%d:Locked\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP07, "%c:%d:Locked\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_MediaNotPresent:
						{
						test.Printf(_L("%c:%d:Not Present\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP08, "%c:%d:Not Present\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Removed:
						{
						test.Printf(_L("%c:%d:Removed\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP09, "%c:%d:Removed\n", (char)driveLetter, driveStatus);
						break;
						}
					case EUsbMsDriveState_Error:
						{
						test.Printf(_L("%c:%d:Error\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP10, "%c:%d:Error\n", (char)driveLetter, driveStatus);
						break;
						}
					default :
						{
						test.Printf(_L("%c:%d:Unknown\n"), (char)driveLetter, driveStatus);
						OstTraceExt2(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP11, "%c:%d:Unknown\n", (char)driveLetter, driveStatus);
						break;
						}
					}
				}

			if (driveStatus == EUsbMsDriveState_Connected)
				{
				gActiveControl->SetMSFinished(EFalse);
				}
			if (driveStatus == EUsbMsDriveState_Disconnected)
				{
				gActiveControl->SetMSFinished(ETrue);
				}
			if ((driveStatus == EUsbMsDriveState_Connecting) && (mountList.Length() < 1))
				{ // If there are more than one drive in the device, only mount one to test. 
                  // For the usb test purpose, it is enough.
                mountList.Append(static_cast<TChar>(driveLetter));
				}
			if(IsDriveInMountList(driveLetter))
				{
				if (driveStatus == EUsbMsDriveState_Connecting)
					{
					MountMsFs(driveNumber);
					}
				else if (driveStatus == EUsbMsDriveState_Disconnecting)
					{
					RestoreMount(driveNumber);
					}
    			}
			}
		}
	else
		{
		test.Printf(KErrFmt, err);
		OstTrace1(TRACE_NORMAL, PROPERTYHANDLERS_DRIVESTATUS_DUP14, "Error: %d\r", err);
		}

	}

void PropertyHandlers::MediaError(RProperty& aProperty)
	{
	TInt r = aProperty.Get(iMediaError);
	if(r != KErrNone)
		{
		return;
		}

	test.Printf(_L("Media Error %x\n"), iMediaError);
	OstTrace1(TRACE_NORMAL, PROPERTYHANDLERS_MEDIAERROR, "Media Error %x\n", iMediaError);
	if (iMediaError > 0)
		{
		gActiveControl->SetMSFinished(ETrue);
		}
	}


void StartMassStorage(RDEVCLIENT* aPort)
	{
    TInt r = KErrUnknown;

	test.Start (_L("Start Mass Storage"));

	fs.Connect();

	// Add MS file system
	test.Next (_L("Add MS File System"));
	r = fs.AddFileSystem(KMsFsy);
	test(r == KErrNone || r == KErrAlreadyExists);

#ifdef USB_SC
	aPort->FinalizeInterface();
#endif


	test.Next (_L("Create active objects\n"));
	propWatch[0] = CPropertyWatch::NewLC(EUsbMsDriveState_KBytesRead, PropertyHandlers::Read);
	propWatch[1] = CPropertyWatch::NewLC(EUsbMsDriveState_KBytesWritten, PropertyHandlers::Written);
	propWatch[2] = CPropertyWatch::NewLC(EUsbMsDriveState_DriveStatus, PropertyHandlers::DriveStatus);
	propWatch[3] = CPropertyWatch::NewLC(EUsbMsDriveState_MediaError, PropertyHandlers::MediaError);
	usbWatch = CUsbWatch::NewLC(*aPort);

	TBuf<8>  t_vendorId(_L("vendor"));
	TBuf<16> t_productId(_L("product"));
	TBuf<4>  t_productRev(_L("1.00"));

	TMassStorageConfig msConfig;
	msConfig.iVendorId.Copy(t_vendorId);
	msConfig.iProductId.Copy(t_productId);
	msConfig.iProductRev.Copy(t_productRev);

 	test.Next(_L("Connect to Mass Storage"));
	r = UsbMs.Connect();
	test_KErrNone (r);

	test.Next(_L("Start Mass Storage"));
	r = UsbMs.Start(msConfig);
	test_KErrNone (r);

	test.End();
	}

void StopMassStorage(RDEVCLIENT* aPort)
	{
    TInt r = KErrUnknown;

	test.Start (_L("Stop Mass Storage"));

	r = UsbMs.Stop();
	test_KErrNone (r);
	UsbMs.Close();

	for (TInt driveNumber = 0; driveNumber < KMaxDrives; driveNumber++)
		{
		if (msfsMountedList[driveNumber])
			{
			r = fs.DismountFileSystem(KMsFs, driveNumber);
			test_KErrNone (r);

			msfsMountedList[driveNumber] = EFalse;
			}
		}

	r = fs.RemoveFileSystem(KMsFs);
	test_KErrNone (r);

	fs.Close();

	delete usbWatch;
	for (TUint i =0; i < KNumPropWatch; i++)
		{
		delete propWatch[i];
		}

	aPort->Close();

	test.End();
	}

