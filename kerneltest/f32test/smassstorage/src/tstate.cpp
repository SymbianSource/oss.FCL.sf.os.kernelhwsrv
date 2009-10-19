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
// Implementation of generic finite state machine state
// 
//

/**
 @file
 @internalTechnology
*/

#include <f32file.h>
#include <testusbc.h>

#include "tstate.h"
#include "t_ms_main.h"
#include "scsicmdbuilder.h"
#include "cpropertywatch.h"

_LIT(KMsFs, "MassStorageFileSystem");

GLREF_D RFs fs;
GLREF_D TInt removalDrvNo;
GLREF_D TUint8 testLun;

GLDEF_D TBuf8<KCbwLength> cbwBuf; 
GLDEF_D TBuf8<KCswLength> cswBuf;

GLDEF_D TInt dCBWTag = 1234567;    // arbitrary, any number would do for this test.
GLDEF_D RDevTestUsbcClient usbcClient;

/**
Unmount FAT and mount MSFS.

@param driveNumber
*/
LOCAL_C void MountMsFs(TInt driveNumber)
	{
	test.Printf(_L("MountMsFs driveNumber=%d\n"), driveNumber); 

	TInt err = KErrNone;
	
	TFileName oldFs;
	err = fs.FileSystemName(oldFs, driveNumber);
	test.Printf(_L("FAT file system name %S; error code %d\n"), &oldFs, err);
	test(err == KErrNone || err == KErrNotFound);
	if(err==KErrNone)
		{
		test.Printf(_L("Unmounting FAT FS %S\n"), &oldFs); 
	    err = fs.DismountFileSystem(oldFs, driveNumber);
	   	test.Printf(_L("%S Dismount %c: %d\n"), &oldFs,'A' + driveNumber, err);
	    test(err == KErrNone);
		}
	
	test.Printf(_L("Mounting MSFS\n")); 
	err = fs.MountFileSystem(KMsFs, driveNumber);
	test.Printf(_L("MSFS Mount %c:   %d\n"), 'A' + driveNumber, err);
	}

/**
Unmount MSFS and mount FAT.

@param driveNumber
*/
LOCAL_C void UnmountMsFs(TInt driveNumber)
	{
	test.Printf(_L("UnmountMsFs driveNumber=%d\n"), driveNumber); 
	TInt err = fs.DismountFileSystem(KMsFs, driveNumber);

	test.Printf(_L("MSFS Dismount:%d\n"), err);
	test(err == KErrNone);

	err = fs.MountFileSystem(_L("FAT"), driveNumber);
   	test.Printf(_L("FAT Mount:    %d\n"), err);
	}

LOCAL_C TBool SendAndReceive(TInt aStatus = 0)
	{
    test.Printf(_L("SendAndReceive\n"));
    TRequestStatus status;
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, KCbwLength); 
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Read CSW
    test.Printf(_L("Reading CSW\n"));
    usbcClient.HostRead(status, EEndpoint2, cswBuf, KCswLength);
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Check dCSWTag
    TInt recvedCBWTag = extractInt(&cswBuf[4]);
    test(dCBWTag == recvedCBWTag);
     
    // Check bCSWStatus
    TInt bCSWStatus = cswBuf[KCswLength - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    return(bCSWStatus == aStatus);
	}
	
//////////////////////////////////////////////////////////////

void
TDisconnected::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
        case EUsbMsDriveState_Connecting:
            MoveToConnecting();
            break;
    	case EUsbMsDriveState_Connected:
    		MoveToConnected();
    		break;	
        default:
            test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
            test(EFalse);
    	}
	}

void
TDisconnected::MoveToConnecting() const
	{
    test.Printf(_L("Moving to connecting state\n"));
   	MountMsFs(removalDrvNo);	
	
	// send test unit ready message
	BuildTestUnitReady();
	createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);
	test(SendAndReceive(1));		// 1: the unit is not ready!
	}

void
TDisconnected::MoveToConnected() const
	{
    test.Printf(_L("Moving to connected state\n"));
   	MountMsFs(removalDrvNo);

	BuildTestUnitReady();
	createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);
	test(SendAndReceive(1));
	}

//////////////////////////////////////////////////////////////

void
TConnecting::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsState_Written:
           	MoveToWritten();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}
	
void TConnecting::MoveToWritten() const
	{
	test.Printf(_L("Moving to written state\n"));
	
	// Mount MS file system 
	MountMsFs(removalDrvNo);
	
	BuildTestUnitReady();
	createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);
	test(SendAndReceive(1));

	// Write 1k bytes using testldd. 
	// 0x2A: opcode for write (10); 10: starting sector; 2: total sectors
	BuildReadWrite(0x2A, 10, 2);
	// 0: indicates host writing
	createCBW(cbwBuf, ++dCBWTag, KKiloBytes, 0, scsiCmdBuf, testLun);
		
    // Send write command
    test.Printf(_L("Sending CBW write cmd\n"));
    TRequestStatus status;
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, KCbwLength); 
    User::WaitForRequest(status);
    test(KErrNone == status.Int());
    
	// Write actual data. We don't care the contents.
    TBuf8<KKiloBytes> writeBuf;	
	writeBuf.SetLength(KKiloBytes);
	usbcClient.HostWrite(status, EEndpoint1, writeBuf, KKiloBytes); 
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Check CSW status
    // Read CSW
    test.Printf(_L("Reading CSW\n"));
    usbcClient.HostRead(status, EEndpoint2, cswBuf, KCswLength);
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Check dCSWTag
    TInt recvedCBWTag = extractInt(&cswBuf[4]);
    test(dCBWTag == recvedCBWTag);
     
    // Check bCSWStatus
    TInt bCSWStatus = cswBuf[KCswLength - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    test(bCSWStatus == 0);	
	}

//////////////////////////////////////////////////////////////

void
TConnected::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsDriveState_Active:
           	MoveToActive();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TConnected::MoveToActive() const
	{
    test.Printf(_L("Moving to active state\n"));
    
    // send prevent medium removal using testld
    // 1: prevent medium removal
    BuildMediumRemoval(1);
    createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);

	if(!SendAndReceive())
		{
		// Prevent Media Removal command not supported
		test.Printf(_L("Prevent Media Removal command not supported, issuing read instead\n"));
	
		// Read 1k bytes using testldd. 
		// 0x28: opcode for read (10); 10: starting sector; 2: total sectors
		BuildReadWrite(0x28, 10, 2);
		// 0x80: indicates host writing
		createCBW(cbwBuf, ++dCBWTag, KKiloBytes, 0x80, scsiCmdBuf, testLun);
			
		// Send read command
		test.Printf(_L("Sending CBW read cmd\n"));
		TRequestStatus status;
		usbcClient.HostWrite(status, EEndpoint1, cbwBuf, KCbwLength); 
		User::WaitForRequest(status);
		test(KErrNone == status.Int());
    
		// Read actual data. We don't care the contents.
		TBuf8<KKiloBytes> readBuf;	
		readBuf.SetLength(KKiloBytes);
		usbcClient.HostRead(status, EEndpoint2, readBuf, KKiloBytes); 
		User::WaitForRequest(status);
		test(KErrNone == status.Int());

		// Check CSW status
		// Read CSW
		test.Printf(_L("Reading CSW\n"));
		usbcClient.HostRead(status, EEndpoint2, cswBuf, KCswLength);
		User::WaitForRequest(status);
		test(KErrNone == status.Int());

		// Check dCSWTag
		TInt recvedCBWTag = extractInt(&cswBuf[4]);
		test(dCBWTag == recvedCBWTag);
     
		// Check bCSWStatus
		TInt bCSWStatus = cswBuf[KCswLength - 1];
		test.Printf(_L("CSW status: %d\n"), bCSWStatus);
		test(bCSWStatus == 0);
		}
	}

//////////////////////////////////////////////////////////////

void
TActive::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsDriveState_Locked:
           	MoveToLocked();
            break;
    	case EUsbMsDriveState_Disconnecting:
    		MoveToDisconnecting();
    		break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TActive::MoveToLocked() const
	{
    test.Printf(_L("Moving to locked state\n"));
    // To be implemented. Wait for lock defect fix
	}
	
void
TActive::MoveToDisconnecting() const
	{
    test.Printf(_L("Moving to disconnecting state\n"));
    
    // send allow medium removal using testld
    // 0: allow medium removal
    BuildMediumRemoval(0);
    createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);
	if(!SendAndReceive())
		test.Printf(_L("Prevent Media Removal command not supported, no need to allow\n"));
	
	// Now the state is connected, let's move to disconnecting state
	// by sending a stop unit command
	
	// 0: stop unit
	BuildStartStopUnit(0);
	createCBW(cbwBuf, ++dCBWTag, 0, 0, scsiCmdBuf, testLun);
	test(SendAndReceive());
	}

//////////////////////////////////////////////////////////////

void
TLocked::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsDriveState_Disconnecting:
           	MoveToDisconnecting();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TLocked::MoveToDisconnecting() const
	{
    test.Printf(_L("Moving to disconnecting state\n"));
    // To be implemented once lock issue is resolved
	}

//////////////////////////////////////////////////////////////

void
TDisconnecting::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsDriveState_Disconnected:
           	MoveToDisconnected();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TDisconnecting::MoveToDisconnected() const
	{
    test.Printf(_L("Moving to disconnected state\n"));
    UnmountMsFs(removalDrvNo);
	}

//////////////////////////////////////////////////////////////

void
TWritten::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsState_Read:
           	MoveToRead();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TWritten::MoveToRead() const
	{
    test.Printf(_L("Moving to read state\n"));
	
	// Read 1k bytes using testldd. 
	// 0x28: opcode for read (10); 10: starting sector; 2: total sectors
	BuildReadWrite(0x28, 10, 2);
	// 0x80: indicates host writing
	createCBW(cbwBuf, ++dCBWTag, KKiloBytes, 0x80, scsiCmdBuf, testLun);
		
    // Send read command
    test.Printf(_L("Sending CBW read cmd\n"));
    TRequestStatus status;
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, KCbwLength); 
    User::WaitForRequest(status);
    test(KErrNone == status.Int());
    
	// Read actual data. We don't care the contents.
    TBuf8<KKiloBytes> readBuf;	
	readBuf.SetLength(KKiloBytes);
	usbcClient.HostRead(status, EEndpoint2, readBuf, KKiloBytes); 
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Check CSW status
    // Read CSW
    test.Printf(_L("Reading CSW\n"));
    usbcClient.HostRead(status, EEndpoint2, cswBuf, KCswLength);
    User::WaitForRequest(status);
    test(KErrNone == status.Int());

    // Check dCSWTag
    TInt recvedCBWTag = extractInt(&cswBuf[4]);
    test(dCBWTag == recvedCBWTag);
     
    // Check bCSWStatus
    TInt bCSWStatus = cswBuf[KCswLength - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    test(bCSWStatus == 0);		
	}

//////////////////////////////////////////////////////////////

void
TRead::MoveTo(TInt aStateId) const
	{
    switch (aStateId)
    	{
       	case EUsbMsDriveState_Disconnected:
           	MoveToDisconnected();
            break;
       	default:
           	test.Printf(_L("Cannot reach %d from %d\n"), GetStateId(), aStateId);
           	test(EFalse);
    	}
	}

void
TRead::MoveToDisconnected() const
	{
    test.Printf(_L("Moving to disconnected state\n"));
    UnmountMsFs(removalDrvNo);
	}

