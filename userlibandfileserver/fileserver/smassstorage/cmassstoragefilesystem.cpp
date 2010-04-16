// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CMassStorageFileSystem implementation.
// 
//

/**
 @file
 @internalTechnology
*/

#include <f32file.h>
#include <f32ver.h>
#include "cusbmassstoragecontroller.h"
#include "cmassstoragefilesystem.h"
#include "cmassstoragemountcb.h"
#include "massstoragedebug.h"
#include "massstorage.h"

_LIT(KMsFsyName, "MassStorageFileSystem");
_LIT(KMsThreadName, "MassStorageThread");
_LIT(KMsDeadThreadName, "MassStorageDeadThread");
_LIT(KMsFsysSemName, "MassStorageSemaphore");
LOCAL_D const TInt KMsFsyMajorVersionNumber=1;
LOCAL_D const TInt KMsFsyMinorVersionNumber=0;

CMassStorageFileSystem::CMassStorageFileSystem()
	{
	}

CMassStorageFileSystem::~CMassStorageFileSystem()
	{
	//Kill the controller thread if it exists
	delete iMassStorageController;
	delete iMediaChanged;
	RThread thread;
	TInt err = thread.Open(KMsThreadName);
	if (err == KErrNone)
		{
		thread.Kill(1); //Parameter is irrelevant
		}
	thread.Close();
	iMsDrives.Close();
	}

CMassStorageFileSystem* CMassStorageFileSystem::NewL()
	{
	CMassStorageFileSystem*  self = new (ELeave) CMassStorageFileSystem();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
void CMassStorageFileSystem::ConstructL()
	{
	iMediaChanged = new(ELeave) CArrayFixFlat<TBool>(KMaxDrives);
	}

/**
Set the file system version and name

@return Any of the standard Symbian error codes.
*/
TInt CMassStorageFileSystem::Install()
	{
	__PRINT(_L("CMassStorageFileSystem::Install In\n"));
	iVersion=TVersion(KMsFsyMajorVersionNumber, KMsFsyMinorVersionNumber, KF32BuildVersionNumber);
	TInt err = SetName(&KMsFsyName);
	__PRINT(_L("CMassStorageFileSystem::Install Out\n"));
	return err;
	}

TInt CMassStorageFileSystem::Remove()
	{
	__PRINT(_L("CMassStorageFileSystem::Remove In\n"));
	TInt err = KErrNone;
	if (iInstalled)
		{
		// Try connecting to the server to send a shutdown message.
		// - If the class controller has a session in use, this will return KErrInUse
		RUsbMassStorage usbMs;
		err = usbMs.Connect();
		if(err == KErrNone)
			{
			err = usbMs.Shutdown();
			usbMs.Close();

			if(err == KErrNone)
				{
				User::WaitForRequest(iThreadStat);
				err = iThreadStat.Int();
				}
			else
				{
				__PRINT1(_L("CMassStorageFileSystem::Remove Shutdown Error %d\n"),err);
				}
			}
		else
			{
			__PRINT1(_L("CMassStorageFileSystem::Remove Connect Error %d\n"),err);
			}
		}
	__PRINT(_L("CMassStorageFileSystem::Remove Out\n"));
	return(err);
	}

/**
Creates a new Mass Storage mount object.

@return A new CMassStorageMountCB
@leave KErrNotReady if the Mass Storage controller is not running.
*/
CMountCB* CMassStorageFileSystem::NewMountL() const
	{
	if (!iRunning)
		{
		User::Leave(KErrNotReady);
		}
	return CMassStorageMountCB::NewL(iMsDrives);
	}

/**
Sets the media attributes and type in the aInfo parameter to those of the specified drive.

@param anInfo TDriveInfo object to store the drive information.
@param aDriveNumber The number of the drive to get the information from.
*/
void CMassStorageFileSystem::DriveInfo(TDriveInfo& aInfo, TInt aDriveNumber) const
	{
	__PRINT(_L("CMassStorageFileSystem::DriveInfo In\n"));
	TLocalDriveCapsV2Buf caps;
	if (!IsValidLocalDriveMapping(aDriveNumber))
		{
		return;
		}
	(void)GetLocalDrive(aDriveNumber).Caps(caps);   
    // error ignored as Caps always returns  valid Media and Drive attributes
	aInfo.iMediaAtt=caps().iMediaAtt;
    aInfo.iType = ::EMediaNotPresent;  // Media is not available to the file system
	aInfo.iDriveAtt=caps().iDriveAtt;
	__PRINT(_L("CMassStorageFileSystem::DriveInfo Out\n"));
	}

/**
Returns a reference to the Mass Storage controller.

@return Reference to the Mass Storage controller.
*/
CUsbMassStorageController& CMassStorageFileSystem::Controller()
	{
	return *iMassStorageController;
	}

/**
Fill iMsDrives with a mapping of lun->drive number for supported mass storage drives

*/
TInt CMassStorageFileSystem::EnumerateMsDrivesL()
	{
	__PRINT(_L("CMassStorageFileSystem::EnumerateMsDrives In\n"));
	iMsDrives.Reset();
	TInt driveCount = 0;

	TLocalDriveCapsV2Buf caps;
	for (TInt i = EDriveC; i < KMaxDrives; i++)
		{
		caps.FillZ();
		
		if (IsValidLocalDriveMapping(i))
			{
			TInt err = GetLocalDrive(i).Caps(caps);
			TInt locDrvNum = DriveNumberToLocalDriveNumber(i);
			__PRINT2(_L("Caps: err=%d, att=%d\n"), err, caps().iDriveAtt);
			
			TBool isRemovable = err==KErrNotReady || (caps().iDriveAtt & KDriveAttRemovable);
			__PRINT2(_L("EnumerateMsDrives: Drive %c: is %sremovable\n"),
							'A'+i-EDriveA, 
							isRemovable?_S(""):_S("NOT "));
							
			if (isRemovable)
				{
				//
				// STF: Connect to the local drive here.  This gives us the media changed flag, and
				//		our own TBusLocalDrive object for use by the proxy drive and controller.
				//
				TBool& mediaChanged = (*iMediaChanged).ExtendL();
				mediaChanged = EFalse;
				TBusLocalDrive* localDrive = new(ELeave) TBusLocalDrive;
				iLocalDriveForMediaFlag.Append(*localDrive);
				
				TInt err=iLocalDriveForMediaFlag[driveCount].Connect(locDrvNum, mediaChanged);
				if(err == KErrNone)
					{
					iMsDrives.Append(i);
					}
				driveCount++;
				}
			}
		}

	__PRINT1(_L("CMassStorageFileSystem::EnumerateMsDrives Out, %d MS drives found\n"), driveCount);
	return driveCount;
	}

TBool CMassStorageFileSystem::IsExtensionSupported() const
	{
	return ETrue;
	}

/**
Creates a TrapCleanup and ActiveScheduler and initializes the Mass Storage controller.
Start the ActiveScheduler.

@return Any of the standard Symbian error codes.
*/
TInt CMassStorageFileSystem::InitThread()
	{
	__PRINT(_L("CMassStorageFileSystem::InitThread In\n"));
	
	//Give the thread a name so we can kill it later
	User::RenameThread(KMsThreadName);
		
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}

	TRAPD(err, InitThreadL());
	
	delete cleanup;
	
	__PRINT1(_L("CMassStorageFileSystem::InitThread Out, error=%d\n"), err);
	return err;
	}

TInt CMassStorageFileSystem::InitThreadL()
	{
	__PRINT(_L("CMassStorageFileSystem::InitThreadL In\n"));
	
	RSemaphore gSemThreadReady;

	TInt ret = gSemThreadReady.OpenGlobal(KMsFsysSemName);

	if (ret != KErrNone && ret != KErrAlreadyExists)
		{
		User::Leave(ret);
		}

	// Determine which drives are available for Mass Storage.
	// (this also creates a local TBusLocalDrive for use by the drive controller)
	EnumerateMsDrivesL();

	CActiveScheduler* sched = new CActiveScheduler;
	if (sched == NULL)
		{
		gSemThreadReady.Signal();
		User::Leave(KErrNoMemory);
		}
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	iMassStorageController = new CUsbMassStorageController;
	if (iMassStorageController == NULL)
		{
		gSemThreadReady.Signal();
		User::Leave(KErrNoMemory);
		}
	
	__PRINT(_L("CMassStorageFileSystem::InitThread: Creating Mass Storage Controller\n"));
	TRAPD(err, iMassStorageController->CreateL(iMsDrives));
	if (err != KErrNone)
		{
		gSemThreadReady.Signal();
		CActiveScheduler::Install(NULL);
		User::Leave(err);
		}
	
	CleanupStack::Pop(sched);

	iRunning = ETrue;
	gSemThreadReady.Signal();
	gSemThreadReady.Close();
	CActiveScheduler::Start();

//========= stop thread ================
	delete iMassStorageController;
	iMassStorageController = NULL;
	TInt i=0;
	for (;i<iLocalDriveForMediaFlag.Count();i++)
		{
		iLocalDriveForMediaFlag[i].Disconnect();
		}
	iLocalDriveForMediaFlag.Reset();
	(*iMediaChanged).Reset();	
	delete sched;
	iRunning = EFalse;
	
	__PRINT(_L("CMassStorageFileSystem::InitThread Out\n"));
	return KErrNone;
	}
	
/**
Not supported in Mass Storage file system.

@leave KErrNotReady
*/
CFileCB* CMassStorageFileSystem::NewFileL() const
	{
	__PRINT(_L("CMassStorageFileSystem::NewFileL In\n"));
	User::Leave(KErrNotReady);
	return NULL;
	}

/**
Not supported in Mass Storage file system.

@leave KErrNotReady
*/
CDirCB* CMassStorageFileSystem::NewDirL() const
	{
	User::Leave(KErrNotReady);
	return NULL;
	}

/**
Not supported in Mass Storage file system.

@leave KErrNotReady
*/
CFormatCB* CMassStorageFileSystem::NewFormatL() const
	{
	User::Leave(KErrNotReady);
	return NULL;
	}

/**
Not supported in Mass Storage file system.

@return KErrNotSupported
*/
TInt CMassStorageFileSystem::DefaultPath(TDes& /*aPath*/) const
	{
	return KErrNotSupported;
	}

/**
Not supported in Mass Storage file system.

@return KErrNotSupported
*/
TInt CMassStorageFileSystem::DriveList(TDriveList& /*aList*/) const
	{
	return KErrNotSupported;
	}

/**
Thread entry point.
*/
LOCAL_C TInt MsInitThreadFn(TAny* aPtr)
	{
	User::SetCritical(User::ESystemCritical);
	((CMassStorageFileSystem*)aPtr)->InitThread();
	//Rename the thread so we can create a new one with the same original name later
	User::RenameThread(KMsDeadThreadName);
	return KErrNone;
	}

/**
Standard entry point for file systems.
Creates a new file system object and starts a new thread for the Mass Storage controller.
*/
extern "C" EXPORT_C CFileSystem* CreateFileSystem()
	{
	__PRINT(_L("CMassStorageFileSystem::CreateFileSystem In\n"));
	RSemaphore gSemThreadReady;
	TInt err = gSemThreadReady.CreateGlobal(KMsFsysSemName, 0);
	if (err != KErrNone)
		{
		__PRINT1(_L("CMassStorageFileSystem::CreateFileSystem Out Semaphore Error %d\n"),err);
		return NULL;
		}

	CFileSystem* msFsys = NULL;
	TRAP(err,  msFsys = CMassStorageFileSystem::NewL());
	if (err != KErrNone)
		{
		__PRINT1(_L("CMassStorageFileSystem::CreateFileSystem Out MSFS Error %d\n"),err);
		gSemThreadReady.Close();
		return NULL;
		}

	RThread msThread;
	__PRINT(_L("CMassStorageFileSystem::CreateFileSystem: Creating Mass Storage thread\n"));
	err = msThread.Create(KMsThreadName, MsInitThreadFn, KDefaultStackSize, NULL, msFsys);
	if (err != KErrNone)
		{
		__PRINT1(_L("CMassStorageFileSystem::CreateFileSystem Out Thread Error %d\n"),err);
		gSemThreadReady.Close();
		return msFsys;
		}
	((CMassStorageFileSystem*)msFsys)->iInstalled=ETrue;


	msThread.Logon(((CMassStorageFileSystem*)msFsys)->iThreadStat);
	msThread.Resume();
	gSemThreadReady.Wait();
	gSemThreadReady.Close();
	msThread.Close();
	
	__PRINT(_L("CMassStorageFileSystem::CreateFileSystem Out Clean\n"));
	
	return msFsys;
	}

