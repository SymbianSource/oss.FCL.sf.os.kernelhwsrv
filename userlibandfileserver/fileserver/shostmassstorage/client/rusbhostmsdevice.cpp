// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <f32file.h>

#include "rusbhostmsdevice.h"
#include "debug.h"
#include "msgservice.h"

_LIT(KFileSystem, "FAT");

TVersion RUsbHostMsDevice::Version() const
    {
	__FNLOG("RUsbHostMsDevice::Version");
    return(TVersion(KUsbHostMsSrvMajorVersionNumber,
                    KUsbHostMsSrvMinorVersionNumber,
                    KUsbHostMsSrvBuildVersionNumber));
    }


TInt RUsbHostMsDevice::StartServer()
    {
	__FNLOG("RUsbHostMsDevice::StartServer");
    TInt r;
    RProcess server;

    const TUid KServerUid3={0x10286A83};
    const TUidType serverUid(KNullUid,KNullUid,KServerUid3);

    // Create the server process
    if((r=server.Create(KUsbHostMsServerName,KNullDesC,serverUid)) != KErrNone)
        {
        __PRINT1(_L("Server process create = %d\n"), r);
        return r;
        }

    // Create the rendezvous request with the server process
    TRequestStatus stat;
    server.Rendezvous(stat);
    if (stat!=KRequestPending)
        {
        server.Kill(0);    // If the outstanding request is not pending then kill the server
        }
    else
        {
		server.SetPriority(EPriorityHigh);
        server.Resume(); // start the server
        }

    // Test whether the process has ended and if it has ended, return how it ended.
    User::WaitForRequest(stat);
    r=(server.ExitType()==EExitPanic) ? KErrGeneral : stat.Int();

    server.Close();
    return r;
    }

EXPORT_C RUsbHostMsDevice::RUsbHostMsDevice()
    {
        // Intentionally left blank
    }


/**
@internalAll
@prototype

Add the Mass Storage device to the MSC server. This API is asynchronous, upon
completion one could find the number of logical units present in the added
device by calling GetNumLun API. Calling the Remove API before completing this
asynchronous API will complete the pending request notification with KErrCancel.

@param aConfig [In] A constant reference object to
                THostMassStorageConfig containing the confiquration values of
                the massstorage device requested to add to the MSC
@param aStatus [In] A reference to TRequestStatus to be used for asynchronous
                request completion
*/
EXPORT_C void RUsbHostMsDevice::Add(const THostMassStorageConfig& aConfig, TRequestStatus& aStatus)
    {
	__FNLOG("RUsbHostMsDevice::Add");
    TInt err = KErrNone;

    err = CreateSession(KUsbHostMsServerName, Version(), 128, EIpcSession_GlobalSharable);

    // Being a transient server, the first session creation would fail with if
    // the server is not running
    if(err != KErrNone)
        {
        // Find whether the session creation failed because server was not
        // running
        if (err==KErrNotFound || err==KErrServerTerminated)
            {
            // Start the server
            err = StartServer();
            if(err == KErrNone)
                {
                // Try session creation again
                err = CreateSession(KUsbHostMsServerName, Version(), 128, EIpcSession_GlobalSharable);
                }
            }
        }

	TRequestStatus* statusPtr = &aStatus;
    if(err == KErrNone)
        {
        // Create a session handle that can be passed via IPC to another process
        // (also being shared by other threads in the current process)
		err = ShareProtected();
        if(err == KErrNone)
            {
            // synchronous call to register the interface
			TPckg<THostMassStorageConfig> pckg(aConfig);
	        err = SendReceive(EUsbHostMsRegisterInterface, TIpcArgs(&pckg));
			if(err != KErrNone)
				{
		        User::RequestComplete(statusPtr, err);
				}
			else
				{
	            // Asynchronous call to initialise the interface
				SendReceive(EUsbHostMsInitialiseInterface, TIpcArgs(NULL), aStatus);
				}
			}
        else
            {
            Close(); // Close the session handle
            __PRINT1(_L("Could not create a sharable session handle %d\n"), err);
            User::RequestComplete(statusPtr, err);
            }
        }
    else
        {
		// Check whether the error is in starting the server or in creating the
        // session
        __PRINT1(_L("Creating server/session failed with %d\n"), err);
        User::RequestComplete(statusPtr, err);
        }
    }


/**
@internalAll
@prototype

Remove the Mass Storage device from the MSC server.
*/
EXPORT_C void RUsbHostMsDevice::Remove()
    {
	// Note: Here, at present we use only the interface token. But we still take
    // THostMassStorageConfig as parameter for future needs
	__FNLOG("RUsbHostMsDevice::Remove");
	_LIT(KUsbHostMsClientPanicCat, "usbhostmsclient");

	TInt r = SendReceive(EUsbHostMsUnRegisterInterface);

	r = SendReceive(EUsbHostMsFinalCleanup);
	if(r != KErrNone)
		{
		User::Panic(KUsbHostMsClientPanicCat ,KErrCouldNotDisconnect);
		}
    Close(); // Close the session handle
    }


/**
@internalAll
@prototype

Get the number of logical units suppoted by the device.

@param aNumLuns [Out] Outputs the number of logical units found in the
                 added Mass Storage device. A value of 'n' represents there are
                 'n' LUNs present in the device, where "n > 0"

@return TInt
*/
EXPORT_C TInt RUsbHostMsDevice::GetNumLun(TUint32& aNumLuns)
    {
	__FNLOG("RUsbHostMsDevice::GetNumLun");
    TPckg<TUint32> pckg(aNumLuns);
    return SendReceive(EUsbHostMsGetNumLun,TIpcArgs(&pckg));
    }


EXPORT_C TInt RUsbHostMsDevice::MountLun(TUint32 aLunId, TInt aDriveNum)
	{
	__FNLOG("RUsbHostMsDevice::MountLun");
    __MSDEVPRINT2(_L(">>> RUsbHostMsDevice::MountLun Drv=%d LUN=%d"), aDriveNum, aLunId);
	RFs TheFs;
	TInt r = TheFs.Connect();
	if(r == KErrNone)
		{
		TPckgBuf<TMassStorageUnitInfo> unitPkg;
		unitPkg().iLunID = aLunId;

		r = TheFs.MountProxyDrive(aDriveNum, _L("usbhostms"), &unitPkg, *this);
        __MSDEVPRINT1(_L("MountProxyDrive %d"), r);
		if(r >= KErrNone)
			{
			r = TheFs.MountFileSystem(KFileSystem, aDriveNum);
            __MSDEVPRINT1(_L("MountFileSystem %d"), r);
			if(r != KErrNone && r != KErrNotReady && r != KErrCorrupt && r != KErrNotSupported)
				{
				TheFs.DismountFileSystem(KFileSystem, aDriveNum);
				TheFs.DismountProxyDrive(aDriveNum);
				}
			}
		TheFs.Close();
		}
	return r;
	}

EXPORT_C TInt RUsbHostMsDevice::DismountLun(TInt aDriveNum)
	{
	__FNLOG("RUsbHostMsDevice::DismountLun");
    __MSDEVPRINT1(_L(">>> RUsbHostMsDevice::DismountLun Drv=%d"), aDriveNum);
	RFs TheFs;
	TInt r;
	r = TheFs.Connect();
	if(r == KErrNone)
		{
		r = TheFs.DismountFileSystem(KFileSystem, aDriveNum);
		if(r != KErrNone)
			{
			// dismount failed - attempt a forced dismount
			TRequestStatus stat;
			TheFs.NotifyDismount(aDriveNum, stat, EFsDismountForceDismount);
			User::WaitForRequest(stat);
			r = stat.Int();
			}
		if(r == KErrNone)
			{
			r = TheFs.DismountProxyDrive(aDriveNum);
			}
		TheFs.Close();
		}
	return r;
	}
