// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// USB Mass Storage Application - also used as an improvised boot loader mechanism
// 
//



/**
 @file
*/

#include "usbtestclient.h"

#include <e32std.h>
#include <e32svr.h>
#include <e32cons.h>

#include <usbmsshared.h>

#include <d32usbc.h>
#include <d32otgdi.h>

#include <nkern/nk_trace.h>
#include <hal.h>

#include "rusbmassstorage.h"

enum
	{
	EUsbDeviceStateUndefined  = EUsbcDeviceStateUndefined,
	EUsbDeviceStateConfigured = EUsbcDeviceStateConfigured,
	};

static CConsoleBase* console = NULL;
static RFs fs;
static TInt selectedDriveIndex = 0;
static TBuf<0x40> mountList;

static TFixedArray<TBool, KMaxDrives>                   msfsMountedList;  ///< 'true' entry corresponds to the drive with mounted MSFS.FSY
static TFixedArray<CFileSystemDescriptor*, KMaxDrives>  unmountedFsList;  ///< every non-NULL entry corresponds to the unmounted original FS for the drive

_LIT(KMsFsy, "USBTESTMSCLIENT.FSY");
_LIT(KMsFs, "MassStorageFileSystem");
_LIT(KOk,"OK");
_LIT(KError,"Error");
_LIT(KBytesTransferredFmt, "%c:%d/%d ");
_LIT(KErrFmt, "Error: %d\r");

_LIT(KTxtApp,"USB TEST CLIENT");
_LIT(KDefPwd,"123");

//-- if defined, some useful information will be printed out via RDebug interface
//#define LOGGING_ENABLED

//-----------------------------------------------------------------------------
/**
    prints a line to the console and copies it to the debug log if LOGGING_ENABLED
*/
void LogPrint(TRefByValue<const TDesC> aFmt,...)
    {
    VA_LIST list;
    VA_START(list, aFmt);

    TBuf<0x100> buf;
    buf.FormatList(aFmt, list); //-- ignore overflows

    if(console)
        console->Write(buf);

#ifdef LOGGING_ENABLED
    //-- print out the line via RDebug::Print
    const TInt bufLen = buf.Length();
    if(bufLen >0 && buf[bufLen-1] == '\n')
        {
        buf.Insert(bufLen-1, _L("\r"));
        }
    else
        {
        buf.Append(_L("\r\n"));
        }

    RDebug::RawPrint(buf);
#endif
    }

//-----------------------------------------------------------------------------
/**
    prints a line to the debug log if LOGGING_ENABLED
*/
void Log(TRefByValue<const TDesC> aFmt,...)
    {
#ifdef LOGGING_ENABLED

    VA_LIST list;
    VA_START(list, aFmt);

    TBuf<0x100> buf;
    buf.FormatList(aFmt, list); //-- ignore overflows

    //-- print out the line via RDebug::Print
    const TInt bufLen = buf.Length();
    if(bufLen >0 && buf[bufLen-1] == '\n')
        {
        buf.Insert(bufLen-1, _L("\r"));
        }

    RDebug::RawPrint(buf);
#else
    (void)aFmt;
#endif
    }


//-----------------------------------------------------------------------------

static void Clear(int row, int count=1)
	{
	_LIT(KBlank,"                                        ");
	for(TInt i=0; i<count; i++)
		{
		console->SetPos(0,row+i);
		console->Printf(KBlank);
		}
	console->SetPos(0,row);
	}


static void ShowDriveSelection()
	{
	console->SetPos(0,15);
	if(PropertyHandlers::allDrivesStatus.Length()/2 > selectedDriveIndex)
		{
		LogPrint(_L("Selected Drive: %c"), 'A' + PropertyHandlers::allDrivesStatus[selectedDriveIndex*2]);
		}
	else
		{
		LogPrint(_L("Selected Drive: (none)"));
		}
	}



class CPeriodUpdate : public CActive
	{
public:
	static CPeriodUpdate* NewLC();
private:
	CPeriodUpdate();
	void ConstructL();
	~CPeriodUpdate();
	void RunL();
	void DoCancel();

	RTimer iTimer;
	TUint iUpTime;
	};

CPeriodUpdate* CPeriodUpdate::NewLC()
	{
	CPeriodUpdate* me=new(ELeave) CPeriodUpdate();
	CleanupStack::PushL(me);
	me->ConstructL();
	return me;
	}

CPeriodUpdate::CPeriodUpdate()
	: CActive(0), iUpTime(0)
	{}

void CPeriodUpdate::ConstructL()
	{
	CActiveScheduler::Add(this);
	iTimer.CreateLocal();
	RunL();
	}

CPeriodUpdate::~CPeriodUpdate()
	{
	Cancel();
	}

void CPeriodUpdate::DoCancel()
	{
	}

void CPeriodUpdate::RunL()
	{
	SetActive();
	// Print RAM usage & up time

	iUpTime++;
	TUint totmins=(iUpTime/60);
	TUint tothrs=(totmins/60);
	TInt mem=0;
	if (HAL::Get(HALData::EMemoryRAMFree, mem)==KErrNone)
		{
		console->SetPos(0,22);
		console->Printf(_L("mem (bytes) : %d\n"), mem);
		console->Printf(_L("up time     : %dh:%dm:%ds\n"),
					tothrs, totmins%60, iUpTime%60);
		}
	iTimer.After(iStatus, 1000000);
	}

//-----------------------------------------------------------------------------
/**
    Dismounts the originally mounted FS and optional primary extension from the drive and stores
    this information in the FS descriptor

    @return on success returns a pointer to the instantinated FS descriptor
*/
static CFileSystemDescriptor* DoDismountOrginalFS(RFs& aFs, TInt aDrive)
    {
    TInt        nRes;
    TBuf<128>   fsName;
    TBuf<128>   primaryExtName;
    TBool       bDrvSync = EFalse;

    Log(_L("# DoDismountOrginalFS drv:%d\n"), aDrive);

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
        LogPrint(_L("Non-primary extensions are not supported!\n"));
        return NULL;
        }

    Log(_L("# DoDismountOrginalFS FS:%S, Prim ext:%S, synch:%d\n"), &fsName, &primaryExtName, bDrvSync);

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
        Log(_L("# DoDismountOrginalFS Dismounting Err:%d\n"), nRes);
        }

    return pFsDesc;
}

//-----------------------------------------------------------------------------
/**
    Tries to restore the original FS on the drive using the FS descriptor provided
    @return standard error code.
*/
static TInt DoRestoreFS(RFs& aFs, TInt aDrive, CFileSystemDescriptor* apFsDesc)
    {
    TInt nRes;

    Log(_L("# DoRestoreFS drv:%d\n"), aDrive);

    //-- 1. check that there is no FS installed
        {
        TBuf<128>   fsName;
        nRes = aFs.FileSystemName(fsName, aDrive);
        if(nRes == KErrNone)
            {//-- probably no file system installed at all
            Log(_L("# This drive already has FS intalled:%S \n"), &fsName);
            return KErrAlreadyExists;
            }
        }

    TPtrC ptrN  (apFsDesc->FsName());
    TPtrC ptrExt(apFsDesc->PrimaryExtName());
    Log(_L("# Mounting FS:%S, Prim ext:%S, synch:%d\n"), &ptrN, &ptrExt, apFsDesc->DriveIsSynch());

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
        Log(_L("# Mount failed! code:%d\n"),nRes);
        }

    return nRes;
    }


//-----------------------------------------------------------------------------
/**
    Dismount the original FS from the drive and mount MsFS instead
*/
static void MountMsFs(TInt driveNumber)
	{
	TInt x = console->WhereX();
	TInt y = console->WhereY();

    //-- 1. try dismounting the original FS
    CFileSystemDescriptor* fsDesc = DoDismountOrginalFS(fs, driveNumber);
    unmountedFsList[driveNumber] = fsDesc;

    console->SetPos(0, 10);

    if(fsDesc)
        {
        TPtrC ptrN(fsDesc->FsName());
        LogPrint(_L("drv:%d FS:%S Dismounted OK"),driveNumber, &ptrN);
        }
    else
        {
        LogPrint(_L("drv:%d Dismount FS Failed!"),driveNumber);
        }

    console->ClearToEndOfLine();

    //-- 2. try to mount the "MSFS"
    TInt error;
    error = fs.MountFileSystem(KMsFs, driveNumber);
	console->SetPos(0, 11);
	LogPrint(_L("MSFS Mount:   %S (%d)"), (error?&KError:&KOk), error);
	console->ClearToEndOfLine();

	if (!error)
		msfsMountedList[driveNumber] = ETrue;

	// restore console position
	console->SetPos(x,y);
	}

//-----------------------------------------------------------------------------
/**
    Dismount MsFS and mount the original FS
*/
static TInt RestoreMount(TInt driveNumber)
	{
	TInt err = KErrNone;

	TInt x = console->WhereX();
	TInt y = console->WhereY();

    //-- 1. try dismounting the "MSFS"
	if (msfsMountedList[driveNumber])
		{
		err = fs.DismountFileSystem(KMsFs, driveNumber);
		console->SetPos(0, 11);
		LogPrint(_L("MSFS Dismount:%S (%d)"), (err?&KError:&KOk), err);
		console->ClearToEndOfLine();
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
        console->SetPos(0, 10);
        LogPrint(_L("%S Mount:    %S (%d)"), &ptrN, (err?&KError:&KOk), err);
        console->ClearToEndOfLine();

        delete fsDesc;
        unmountedFsList[driveNumber] = NULL;
        }


    // restore console position
	console->SetPos(x,y);
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
	CleanupStack::PushL(me);
	me->ConstructL(aSubkey);
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
	CleanupStack::PushL(me);
	me->ConstructL();
	return me;
	}

CUsbWatch::CUsbWatch(RUsb& aUsb)
	:
	CActive(0),
	iUsb(aUsb),
	iUsbDeviceState(EUsbDeviceStateUndefined),
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
//	iUsb.DeviceStateNotificationCancel();
	iUsb.AlternateDeviceStatusNotifyCancel();
	}

void CUsbWatch::DoCancel()
	{
//	iUsb.DeviceStateNotificationCancel();
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
	return(!mountList.Length() || KErrNotFound != mountList.Find(&driveLetter16, 1));
	}

void CUsbWatch::RunL()
	{
//	RDebug::Print(_L(">> CUsbWatch[%d] %d"), iUsbDeviceState, iWasConfigured);

//	const TUint stateMask = 0xFF;
//	iUsb.DeviceStateNotification(stateMask, iUsbDeviceState, iStatus);
	iUsb.AlternateDeviceStatusNotify(iStatus, iUsbDeviceState);
	SetActive();

	//RDebug::Print(_L("CUsbWatch DeviceStateNotification: iUsbDeviceState=%d"), iUsbDeviceState);

	// If the cable is disconnected, unmount all the connected drives.
	if(iWasConfigured && iUsbDeviceState == EUsbDeviceStateUndefined)
		{
		for(TInt i=0; i<PropertyHandlers::allDrivesStatus.Length()/2; i++)
			{
			if(IsDriveConnected(i))
				{
				//RDebug::Print(_L("CUsbWatch calling RestoreMount"));
				RestoreMount(PropertyHandlers::allDrivesStatus[2*i]);
				}
			}
		iWasConfigured = EFalse;
		}

    // If cable is connected, mount all drives in the auto-mount list. This is
    // done for performance, since if this is not done here, mounting will
    // happen later after each drive enters the Connecting state.
	if (iUsbDeviceState == EUsbDeviceStateConfigured)
		{
		for (TInt i=0; i<PropertyHandlers::allDrivesStatus.Length()/2; i++)
			{
			TInt driveNumber = PropertyHandlers::allDrivesStatus[2*i];
			if (!IsDriveConnected(i) && IsDriveInMountList(DriveNumberToLetter(driveNumber)))
				{
				//RDebug::Print(_L("CUsbWatch calling MountMsFs"));
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
	console->SetPos(0,1);
	console->Printf(_L("KB R/W:  "));
	TInt err = aProperty.Get(aReadOrWritten);
	if(err == KErrNone)
		{
		for(TInt i = 0; i < allDrivesStatus.Length()/2; i++)
			{
			console->Printf(KBytesTransferredFmt,
				(char)DriveNumberToLetter(allDrivesStatus[2*i]), iKBytesRead[i], iKBytesWritten[i]);
			}
		console->ClearToEndOfLine();
		}
	else
		{
		console->Printf(KErrFmt, err);
		}
	}

void PropertyHandlers::DriveStatus(RProperty& aProperty)
	{
//	RDebug::Print(_L(">> PropertyHandlers::DriveStatus"));
	TInt err = aProperty.Get(allDrivesStatus);
	console->SetPos(0,0);
	if(err == KErrNone)
		{
		console->Printf(_L("Status:  "));
		for(TInt i = 0; i < allDrivesStatus.Length()/2; i++)
			{
			TInt driveNumber = allDrivesStatus[2*i];
			TInt driveStatus = allDrivesStatus[2*i+1];
			TChar driveLetter = DriveNumberToLetter(driveNumber);

//			RDebug::Print(_L("%c:%d   "), (char)driveLetter, driveStatus);

			switch(driveStatus)
				{
				case EUsbMsDriveState_Disconnected:
					{
					LogPrint(_L("%c:%d:Disconnected "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Connecting:
					{
					LogPrint(_L("%c:%d:Connecting   "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Connected:
					{
					LogPrint(_L("%c:%d:Connected    "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Disconnecting:
					{
					LogPrint(_L("%c:%d:Disconnecting"), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Active:
					{
					LogPrint(_L("%c:%d:Active       "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Locked:
					{
					LogPrint(_L("%c:%d:Locked       "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_MediaNotPresent:
					{
					LogPrint(_L("%c:%d:Not Present  "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Removed:
					{
					LogPrint(_L("%c:%d:Removed      "), (char)driveLetter, driveStatus);
					break;
					}
				case EUsbMsDriveState_Error:
					{
					LogPrint(_L("%c:%d:Error        "), (char)driveLetter, driveStatus);
					break;
					}
				default :
					{
					LogPrint(_L("%c:%d:Unknown      "), (char)driveLetter, driveStatus);
					break;
					}
				}

			if(IsDriveInMountList(driveLetter))
				{
				if (driveStatus == EUsbMsDriveState_Connecting)
					{
					MountMsFs(driveNumber);
					}
				else if (driveStatus == EUsbMsDriveState_Disconnected)
					{
					RestoreMount(driveNumber);
					}
				else
					{
					//RDebug::Print(_L("PropertyHandlers::DriveStatus: nothing to do"));
					}
				}
			else
				{
				//RDebug::Print(_L("PropertyHandlers::DriveStatus: %c: is not in mountList\n"), driveLetter);
				}
			}
		}
	else
		{
		LogPrint(KErrFmt, err);
		}

	//RDebug::Print(_L("<< PropertyHandlers::DriveStatus"));
	}

void PropertyHandlers::MediaError(RProperty& aProperty)
	{
	TInt err = aProperty.Get(iMediaError);
	if (err != KErrNone)
		{
		// RDebug::Printf("RProperty::Get returned %d", err);
		return;
		}

	//RDebug::Printf("PropertyHandlers::MediaError %x", iMediaError);

	TInt x = console->WhereX();
	TInt y = console->WhereY();
	Clear(27,1);
	LogPrint(_L("Media Error %x"), iMediaError);
	// restore console position
	console->SetPos(x,y);
	}

//////////////////////////////////////////////////////////////////////////////
//
// CMessageKeyProcessor
//
//////////////////////////////////////////////////////////////////////////////
CMessageKeyProcessor::CMessageKeyProcessor(CConsoleBase* aConsole)
	: CActive(CActive::EPriorityUserInput), iConsole(aConsole)
	{
	}

CMessageKeyProcessor* CMessageKeyProcessor::NewLC(CConsoleBase* aConsole)
	{
	CMessageKeyProcessor* self=new (ELeave) CMessageKeyProcessor(aConsole);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CMessageKeyProcessor* CMessageKeyProcessor::NewL(CConsoleBase* aConsole)
	{
	CMessageKeyProcessor* self = NewLC(aConsole);
	CleanupStack::Pop();
	return self;
	}

void CMessageKeyProcessor::ConstructL()
	{
	// Add to active scheduler
	CActiveScheduler::Add(this);
	RequestCharacter();
	}

void CMessageKeyProcessor::MakePassword(TMediaPassword &aPassword)
	{
	//  Create password with same format as eshell and S60
	TBuf<3> password(KDefPwd);

	// fill aPassword with contents of password, not converting to ASCII
	const TInt byteLen = password.Length() * 2;
	aPassword.Copy(reinterpret_cast<const TUint8 *>(password.Ptr()), byteLen);
	}

CMessageKeyProcessor::~CMessageKeyProcessor()
	{
	// Make sure we're cancelled
	Cancel();
	}

void  CMessageKeyProcessor::DoCancel()
	{
	iConsole->ReadCancel();
	}

void  CMessageKeyProcessor::RunL()
	{
	  // Handle completed request
	ProcessKeyPress(TChar(iConsole->KeyCode()));
	}

void CMessageKeyProcessor::RequestCharacter()
	{
	  // A request is issued to the CConsoleBase to accept a
	  // character from the keyboard.
	iConsole->Read(iStatus);
	SetActive();
	}

void CMessageKeyProcessor::ProcessKeyPress(TChar aChar)
	{

	TInt error = KErrNone;

    aChar.UpperCase();
	switch(aChar)
		{
		case 'Q':
		case EKeyEscape:
			{
			TInt err = KErrNone;
			for(TInt j=0; j<KMaxDrives; j++)
				{
				err = RestoreMount(j);

				if (err)
					{
					// Mount is busy/locked and can not be restored.
					break;
					}

				}

			if (err == KErrNone)
				{
				CActiveScheduler::Stop();
				return;
				}

			}
			break;

#if defined(_DEBUG)
		case 'T':
			iTraceEnable = !iTraceEnable;
			if (iTraceEnable)	// 0x44008401
				User::SetDebugMask(KHARDWARE|KDLL|KSCRATCH|KPOWER|KMEMTRACE);
			else
				User::SetDebugMask(0);
			break;
#endif

		case 'D':
			if(++selectedDriveIndex >= PropertyHandlers::allDrivesStatus.Length()/2)
				{
				selectedDriveIndex = 0;
				}
			ShowDriveSelection();
			break;

		case 'M':
			if(PropertyHandlers::allDrivesStatus.Length())
				{
				MountMsFs(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2]);
				}
			break;

		case 'U':
			if(PropertyHandlers::allDrivesStatus.Length())
				{
				RestoreMount(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2]);
				}
			break;

		case 'L':
			{
			// lock unprotected drive
			TMediaPassword password;
			MakePassword(password);

			_LIT(KEmpty, "");
			TMediaPassword nul;
			nul.Copy(KEmpty);
			error = fs.LockDrive(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2],
                                 nul, password, ETrue);
			console->SetPos(0,9);
			LogPrint(_L("LockDrive %S (%d)"), (error?&KError:&KOk), error);
			break;
			}

		case 'I':
            {
            // lock password protected drive
            TMediaPassword password;
            MakePassword(password);
            error = fs.LockDrive(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2],
                                 password, password, ETrue);
            console->SetPos(0,9);
            LogPrint(_L("LockDrive %S (%d)"), (error?&KError:&KOk), error);
            break;
            }

        case 'N':
            {
            TMediaPassword password;
            MakePassword(password);
            error = fs.UnlockDrive(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2],
                                   password, ETrue);
            Clear(9);
            LogPrint(_L("UnlockDrive %S (%d)"), (error?&KError:&KOk), error);
            }
			break;

        case 'C':
            {
            TMediaPassword password;
            MakePassword(password);
            error = fs.ClearPassword(PropertyHandlers::allDrivesStatus[selectedDriveIndex*2],
                                     password);
            Clear(9);
            LogPrint(_L("ClearPassword %S (%d)"), (error?&KError:&KOk), error);
            }
			break;
		default:
			break;
		}
	RequestCharacter();
	}


//////////////////////////////////////////////////////////////////////////////
//
// Application entry point
//
//////////////////////////////////////////////////////////////////////////////
static void RunAppL()
	{

    TInt error = KErrUnknown;

	//RDebug::Print(_L("USBMSAPP: Creating console\n"));
	console = Console::NewL(KTxtApp,TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);

	console->SetPos(0,2);
	console->Printf(_L("========================================"));

	// Command line: list of drive letters to auto-mount (all if not specified)
	User::CommandLine(mountList);
	mountList.UpperCase();

	CActiveScheduler* sched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	fs.Connect();
	CleanupClosePushL(fs);

	_LIT(KMountAllDefault,"(all)");
	console->SetPos(0,3);
	LogPrint(_L("Drives to auto-mount: %S"), (mountList.Length() ? &mountList : &KMountAllDefault));

	// Add MS file system
	error = fs.AddFileSystem(KMsFsy);
	if(error != KErrNone && error != KErrAlreadyExists)
		{
		//RDebug::Print(_L("AddFileSystem failed, err=%d\n"), error);
		User::Leave(error);
		}
	console->SetPos(0,4);
	LogPrint(_L("MSFS file system:\tAdded OK\n"));

	RUsb usb;

	// Load the logical device
	_LIT(KDriverFileName,"EUSBC.LDD");
	error = User::LoadLogicalDevice(KDriverFileName);
	if (error != KErrAlreadyExists)
		User::LeaveIfError(error);

	error = usb.Open(0);
	User::LeaveIfError(error);

	_LIT(KOtgdiLddFilename, "otgdi");
	// Check for OTG support
	TBuf8<KUsbDescSize_Otg> otg_desc;
	error = usb.GetOtgDescriptor(otg_desc);
	if (!(error == KErrNotSupported || error == KErrNone))
		{
		LogPrint(_L("Error %d while fetching OTG descriptor"), error);
		User::Leave(-1);
		return;
		}

	// On an OTG device we have to start the OTG driver, otherwise the Client
	// stack will remain disabled forever.
	if (error == KErrNotSupported)
	{
		CleanupClosePushL(usb);
		User::Leave(-1);
	}

	error = User::LoadLogicalDevice(KOtgdiLddFilename);
	if (error != KErrNone)
		{
		LogPrint(_L("Error %d on loading OTG LDD"), error);
		User::Leave(-1);
		return;
		}

	RUsbOtgDriver iOtgPort;

	error = iOtgPort.Open();
	if (error != KErrNone)
		{
		LogPrint(_L("Error %d on opening OTG port"), error);
		User::Leave(-1);
		return;
		}
	error = iOtgPort.StartStacks();
	if (error != KErrNone)
		{
		LogPrint(_L("Error %d on starting USB stack"), error);
		User::Leave(-1);
		return;
		}

	CleanupClosePushL(usb);

//		RDebug::Print(_L("USBMSAPP: Create active objects\n"));
	CMessageKeyProcessor::NewLC(console);
	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesRead, PropertyHandlers::Read);
	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesWritten, PropertyHandlers::Written);
	CPropertyWatch::NewLC(EUsbMsDriveState_DriveStatus, PropertyHandlers::DriveStatus);
	CPropertyWatch::NewLC(EUsbMsDriveState_MediaError, PropertyHandlers::MediaError);
	CUsbWatch::NewLC(usb);
	CPeriodUpdate::NewLC();

	RUsbMassStorage UsbMs;
	TBuf<8>  t_vendorId(_L("vendor"));
	TBuf<16> t_productId(_L("product"));
	TBuf<4>  t_productRev(_L("1.00"));

	TMassStorageConfig msConfig;
	msConfig.iVendorId.Copy(t_vendorId);
	msConfig.iProductId.Copy(t_productId);
	msConfig.iProductRev.Copy(t_productRev);

//   	console->Printf(_L("Connect to Mass Storage"));
	error = UsbMs.Connect();
	User::LeaveIfError(error);

//   	console->Printf(_L("Start Mass Storage"));
	error = UsbMs.Start(msConfig);
	User::LeaveIfError(error);

	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	error = usb.GetDeviceDescriptor(deviceDescriptor);
	User::LeaveIfError(error);

	const TInt KUsbSpecOffset = 2;
	const TInt KUsbDeviceClassOffset = 4;
	const TInt KUsbVendorIdOffset = 8;
	const TInt KUsbProductIdOffset = 10;
	const TInt KUsbDevReleaseOffset = 12;
	//Change the USB spec number to 2.00
	deviceDescriptor[KUsbSpecOffset]   = 0x00;
	deviceDescriptor[KUsbSpecOffset+1] = 0x02;
	//Change the Device Class, Device SubClass and Device Protocol
	deviceDescriptor[KUsbDeviceClassOffset] = 0x00;
	deviceDescriptor[KUsbDeviceClassOffset+1] = 0x00;
	deviceDescriptor[KUsbDeviceClassOffset+2] = 0x00;
	//Change the device vendor ID (VID) to 0x0E22 (Symbian)
	deviceDescriptor[KUsbVendorIdOffset]   = 0x22;   // little endian
	deviceDescriptor[KUsbVendorIdOffset+1] = 0x0E;
	//Change the device product ID (PID) to 0x1111
	deviceDescriptor[KUsbProductIdOffset]   = 0x12;
	deviceDescriptor[KUsbProductIdOffset+1] = 0x11;
	//Change the device release number to 3.05
	deviceDescriptor[KUsbDevReleaseOffset]   = 0x05;
	deviceDescriptor[KUsbDevReleaseOffset+1] = 0x03;
	error = usb.SetDeviceDescriptor(deviceDescriptor);
	User::LeaveIfError(error);

	// Remove possible Remote-Wakup support in Configuration descriptor,
	// so that we can use the MSC device also easily for Chapter9 testing.
	TBuf8<KUsbDescSize_Config> configDescriptor;
	error = usb.GetConfigurationDescriptor(configDescriptor);
	User::LeaveIfError(error);
	const TInt KConfDesc_AttribOffset = 7;
	configDescriptor[KConfDesc_AttribOffset] &= ~KUsbDevAttr_RemoteWakeup;
	error = usb.SetConfigurationDescriptor(configDescriptor);
	User::LeaveIfError(error);

	_LIT16(productID_L, "Symbian USB Mass Storage Device (Base)");
	TBuf16<KUsbStringDescStringMaxSize / 2> productID(productID_L);
	error = usb.SetProductStringDescriptor(productID);
	User::LeaveIfError(error);

	TRequestStatus enum_status;
	console->SetPos(0,5);
	LogPrint(_L("Re-enumerating...\n"));
	usb.ReEnumerate(enum_status);
	User::LeaveIfError(error);
	console->SetPos(0,5);
	User::WaitForRequest(enum_status);
	if(enum_status.Int() == KErrNone)
		LogPrint(_L("Re-enumeration Done\n"));
	else
		LogPrint(_L("Re-enumeration not successfully done\n"));


    console->SetPos(0,14);
    TBuf<3>password(KDefPwd);
    LogPrint(_L("Password: %S"), &password);

	ShowDriveSelection();

	console->SetPos(0,17);

	_LIT(KMsgTitleB,"Menu: q=quit  d=chg drv\n      m=mount u=unmount\n       l=lock i=lock n=unlock\n      c=clr pwd");


	//RDebug::Print(_L("USBMSAPP: Start CActiveScheduler\n"));

	console->Printf(KMsgTitleB);

	CActiveScheduler::Start();

	error = UsbMs.Stop();
	User::LeaveIfError(error);
	UsbMs.Close();
	error = fs.RemoveFileSystem(KMsFs);
	User::LeaveIfError(error);

	CleanupStack::PopAndDestroy(11);

	iOtgPort.StopStacks();
	iOtgPort.Close();
	error = User::FreeLogicalDevice(RUsbOtgDriver::Name());
	User::LeaveIfError(error);

	error = User::FreeLogicalDevice(_L("USBC"));
	User::LeaveIfError(error);

	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();

    msfsMountedList.Reset();
    unmountedFsList.Reset();


	TRAPD(error,RunAppL());
	__ASSERT_ALWAYS(!error, User::Panic(KTxtApp, error));

	delete cleanup;
	__UHEAP_MARKEND;
	return 0;
	}


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
