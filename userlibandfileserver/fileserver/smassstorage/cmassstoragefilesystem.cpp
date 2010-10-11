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
#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"
#include "cmassstoragefilesystem.h"
#include "cmassstoragemountcb.h"
#include "massstorage.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cmassstoragefilesystemTraces.h"
#endif


_LIT(KMsFsyName, "MassStorageFileSystem");
_LIT(KMsThreadName, "MassStorageThread");
_LIT(KMsDeadThreadName, "MassStorageDeadThread");
_LIT(KMsFsysSemName, "MassStorageSemaphore");
static const TInt KMsFsyMajorVersionNumber=1;
static const TInt KMsFsyMinorVersionNumber=0;

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
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_100);
    iVersion=TVersion(KMsFsyMajorVersionNumber, KMsFsyMinorVersionNumber, KF32BuildVersionNumber);
    TInt err = SetName(&KMsFsyName);
    return err;
    }

TInt CMassStorageFileSystem::Remove()
    {
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_101);
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
                OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_102,
                          "Shutdown Error %d", err);
                }
            }
        else
            {
            OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_103,
                      "Connect Error %d", err);
            }
        }
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
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_104);
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
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_105);
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
            OstTraceExt2(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_106,
                      "Caps: err=%d, att=%d", err, caps().iDriveAtt);

            TBool isRemovable = err==KErrNotReady || (caps().iDriveAtt & KDriveAttRemovable);
            OstTraceExt2(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_107,
                         "EnumerateMsDrives: Drive %c: removable %d", 'A'+i-EDriveA, isRemovable);

            if (isRemovable)
                {
                //
                // STF: Connect to the local drive here.  This gives us the media changed flag, and
                //      our own TBusLocalDrive object for use by the proxy drive and controller.
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

    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_108,
              "%d MS drives found", driveCount);
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
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_109);

    //Give the thread a name so we can kill it later
    User::RenameThread(KMsThreadName);

    CTrapCleanup* cleanup = CTrapCleanup::New();
    if (cleanup == NULL)
        {
        return KErrNoMemory;
        }

    TRAPD(err, InitThreadL());

    delete cleanup;

    OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_110,
              "error=%d", err);
    return err;
    }

TInt CMassStorageFileSystem::InitThreadL()
    {
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_111);

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

    OstTrace0(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_112,
              "Creating Mass Storage Controller");
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
    OstTrace0(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_113,"thread stopping...");
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
    OstTrace0(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_114,"thread stopped.");
    return KErrNone;
    }

/**
Not supported in Mass Storage file system.

@leave KErrNotReady
*/
CFileCB* CMassStorageFileSystem::NewFileL() const
    {
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
static TInt MsInitThreadFn(TAny* aPtr)
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
    OstTraceFunctionEntry0(CMASSSTORAGEFILESYSTEM_115);
    RSemaphore gSemThreadReady;
    TInt err = gSemThreadReady.CreateGlobal(KMsFsysSemName, 0);
    if (err != KErrNone)
        {
        OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_116,
                  "Semaphore CreateGlobal Error %d", err);
        return NULL;
        }

    CFileSystem* msFsys = NULL;
    TRAP(err,  msFsys = CMassStorageFileSystem::NewL());
    if (err != KErrNone)
        {
        OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_117,
                  "MSFS Error %d",err);
        gSemThreadReady.Close();
        return NULL;
        }

    RThread msThread;
    OstTrace0(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_118,
              "Creating Mass Storage thread...");
    err = msThread.Create(KMsThreadName, MsInitThreadFn, KDefaultStackSize, NULL, msFsys);
    if (err != KErrNone)
        {
        OstTrace1(TRACE_SMASSSTORAGE_FS, CMASSSTORAGEFILESYSTEM_119,
                  "Thread Error %d", err);
        gSemThreadReady.Close();
        return msFsys;
        }
    ((CMassStorageFileSystem*)msFsys)->iInstalled=ETrue;


    msThread.Logon(((CMassStorageFileSystem*)msFsys)->iThreadStat);
    msThread.Resume();
    gSemThreadReady.Wait();
    gSemThreadReady.Close();
    msThread.Close();
    return msFsys;
    }

