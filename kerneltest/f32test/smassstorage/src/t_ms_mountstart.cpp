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
// Component test of registration and deregistration
// 1. Test registering MS FS
// Test actions:
// a. Load MS Server/File system and start it. 
// b. Select a drive, say, Y, format it using FAT FS
// c. Read and store the boot sector of Y
// d. Unmount FAT FS from Y
// e. Mount MS FS on Y
// f. Read the boot sector of Y by passing SCSI command through TestLdd 
// g. Compare the two boot sectors, which are obtained under FAT MS and MS FS repectively
// h. Try to read Y (assuming it's a removable media) using RFs API.
// Expected results:
// This test should be run for different drives.
// Mounting non-removable media (step b) should fail.
// For removable media: steps from c to g should complete successfully
// Read boot sector should be the same as the saved. 
// Step h should fail (KErrNotSupported).
// 2. Test deregistering MS FS
// Test actions:
// a. Try to unmount MS FS from Y while SCSI is writing to it
// b. Wait for access completion.
// c. Unmount MS FS from Y. 	
// d. Try to read from Y using RFs API. 
// e. Remount FAT FS on Y.
// f. Issue UnitReady and Read SCSI command through test LDD. 
// g. Try to read from Y using RFs API.
// h. Unmount FAT FS from Y and mount MS FS on it
// Expected results:
// Deregister request (step a) should return an appropriate error code when media is in use. 
// Next deregistering (step c) should be successful. 
// Attempt (step d) should fail. 
// SCSI commands (step f) should report error when MS FS is unmounted. 
// File server read request (step g) should be successful when FAT FS is mounted  
// 
//

/**
 @file
 @internalTechnology
*/

#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <hal.h>
#include <massstorage.h>
#include "t_ms_main.h"
#include "testusbc.h"

#define CBW_LENGTH          31
#define CSW_LENGTH          13
#define SCSI_READ_LEN       10
#define SCSI_WRITE_LEN      10
#define SCSI_UNIT_READY_LEN 6
#define SCSI_MED_RMVL_LEN   6
#define BOOTSECTOR_SIZE     512

_LIT(KMsFsyName, "MassStorageFileSystem");

const TUint KMBRFirstPartitionSectorOffset = 0x1BE + 8;
const TUint KBootRecordSignature = 0xAA55;

LOCAL_D TChar driveLetter[2];
LOCAL_D TInt nonRemovalDrvNo;
LOCAL_D TInt removalDrvNo;
LOCAL_D TUint8 testLun(0);                // Use MMC card for testing

#define LOG_AND_TEST(a, e) {if (a!=e) {test.Printf(_L("lvalue %d, rvalue%d\n\r"), a,e); test(EFalse);}}
LOCAL_C void ParseCommandArguments()
//
// Parses the command line arguments
// We expect 3 parameters:
//	- Drive letter for non-removable drive
//	- Drive letter for removable drive
//	- LUN for removable drive (0-7)
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token;
	TBool cmdOk= ETrue;
	for (int i = 0; i < 3; i++)  //
		{
		token.Set(lex.NextToken());
		if (token.Length() != 0)
			{
			if (i <2)
				{
				driveLetter[i] = token[0];
				driveLetter[i].UpperCase();
				test.Printf(_L("CmdLine Param=%S\r\n"),&token);
				}
			else
				{
				TChar cLun = token[0];
				TInt lun = TInt(cLun) - 0x30;
				if (lun>=0 && lun <8)
					{
					testLun = TUint8(lun);
					}
				else
					{
					cmdOk= EFalse;
					}
				}
			}
		else 
			{
			cmdOk= EFalse;
			break;
			}
		}
		

		if (!cmdOk) 
			{
			test.Printf(_L("No or not enough command line arguments - using default arguments\r\n"));
			// code drive letters based on platform
			TInt uid;
			TInt r=HAL::Get(HAL::EMachineUid,uid);
			LOG_AND_TEST(r,KErrNone);
    	
			switch(uid)
	   			{
	   			case HAL::EMachineUid_Lubbock:
	   				driveLetter[0] = 'C';
	   				driveLetter[1] = 'F';
	   				test.Printf(_L("Test is running on Lubbock\r\n"));
					testLun = 2;
  					break;
	   			case HAL::EMachineUid_Win32Emulator:
	   				driveLetter[0] = 'Y';
	   				driveLetter[1] = 'X';
	   				test.Printf(_L("Test is running on Win32 Emulator\r\n"));
					testLun = 1;
	   				break;
	   			case HAL::EMachineUid_OmapH4:
	   				driveLetter[0] = 'C';
	   				driveLetter[1] = 'D';
	   				test.Printf(_L("Test is running on H4 board\r\n"));
					testLun = 0;
	   				break;
	   			default:
					// Assume it's a H2 board for now as no relevant Enum is found
	   				driveLetter[0] = 'C';
	   				driveLetter[1] = 'D';
	   				test.Printf(_L("Test is running on H2 board\r\n"));	
	   				testLun = 0;				
	   				break;
	   			}
	   			
				test.Printf(_L("Parameters used for test:\r\n\tfixed drive\t\t%c\r\n\tremovable drive\t\t%c\r\n\tLUN\t\t\t%d\r\n"), 
		        		    (TUint) driveLetter[0], (TUint) driveLetter[1], testLun);
			}	   		
	}

LOCAL_C void fillInt(TUint8* dest, TInt source)
    // 
    // Copy an int. Little endian
    //
    {
    for (TInt i = 0; i < 4; i++)
        {
        *dest++ = TUint8((source >> i*8) & 0xFF);
        }
    }

LOCAL_C TInt extractInt(const TUint8* aBuf)
    //
    // Extract an integer from a buffer. Assume little endian
    //
    {
    return aBuf[0] + (aBuf[1] << 8) + (aBuf[2] << 16) + (aBuf[3] << 24);
    }

LOCAL_C void createReadCmd(TDes8& aRead10Buf, TInt aLogicalBlkAddr, TInt aTotalBlocks)
    // 
    // Prepare SCSI read(10) command
    //
    {
    // Zero out the whole buffer
    aRead10Buf.FillZ(SCSI_READ_LEN);
    // operation code
    aRead10Buf[0] = 0x28;    
    // Fill in logical block address. Big endian
    aRead10Buf[2] = TUint8((aLogicalBlkAddr >> 24) & 0xFF);
    aRead10Buf[3] = TUint8((aLogicalBlkAddr >> 16) & 0xFF);
    aRead10Buf[4] = TUint8((aLogicalBlkAddr >> 8) & 0xFF);
    aRead10Buf[5] = TUint8(aLogicalBlkAddr & 0xFF);
    
    // Data transfer length (# of sectors). Big endian
    aRead10Buf[7] = TUint8((aTotalBlocks >> 8) & 0xFF);
    aRead10Buf[8] = TUint8((aTotalBlocks & 0xFF));
    }

LOCAL_C void createCBW(TDes8& aCbw, TInt aDCBWTag, TInt aDataTransferLen, TUint8 aInOutFlag, TUint8 aCBLength, TDes8& aCBWCB)
    // 
    // aCbw:            stores CBW
    // aDCBWTag:        a command block tag sent by the host. Used to associates a CSW
    //                  with corresponding CBW
    // aDataTranferLen: the number of bytes the host expects to transfer
    // aInOutFlag:      value for bmCBWFlags field, indicating the direction of transfer
    // aCBLengh:        valid length of CBWCB field in bytes
    // aCBWCB			the actual command to be wrapped
    {
    // Zero out aCbw
    aCbw.FillZ(CBW_LENGTH);

    // dCBWSignature field, the value comes from spec
    TInt dCBWSignature = 0x43425355;
    fillInt(&aCbw[0], dCBWSignature);
    // dCBWTag field
    fillInt(&aCbw[4], aDCBWTag);
    // dCBWDataTransferLength field
    fillInt(&aCbw[8], aDataTransferLen);
    // bmCBWFlags field
    aCbw[12] = aInOutFlag;
	aCbw[13] = testLun;
    // bCBWCBLength field
    aCbw[14] = aCBLength;          

    // CBWCB field
    for (TInt i = 0; i < aCBLength; ++i)
    	{
    	aCbw[15 + i] = aCBWCB[i];
    	}
    }

LOCAL_C void doComponentTest()
    //
    // Do the component test
    //
	{
	__UHEAP_MARK;	

    TInt ret;
    test.Next(_L("Start MountStart test. Be sure MMC card is inserted."));

    // Connect to the server
    RFs fs;
    LOG_AND_TEST(KErrNone,  fs.Connect());
    
    // Convert drive letters to their numerical equivalent
	ret = fs.CharToDrive(driveLetter[0],nonRemovalDrvNo);
	LOG_AND_TEST(ret, KErrNone);
	ret = fs.CharToDrive(driveLetter[1],removalDrvNo);
	LOG_AND_TEST(ret,  KErrNone);	


	// Load the logical device
	_LIT(KDriverFileName,"TESTUSBC.LDD");
	ret = User::LoadLogicalDevice(KDriverFileName);
	test(ret == KErrNone || ret == KErrAlreadyExists);

    // Add MS file system
	_LIT(KMsFs, "MSFS.FSY");
	ret = fs.AddFileSystem(KMsFs);
	test(ret == KErrNone || ret == KErrAlreadyExists);
	
	// DEF080979: RFs::AddFileSystem, wrong error code when re-adding 
	// the mass storage file system. Confirm that RFs::AddFileSystem 
	// returns the correct error code if the file system already exists. 
	// Also confirm that the mass storage file system is usable after such
	// a failed attempt to re-add the file system.
	ret = fs.AddFileSystem(KMsFs);
	test(ret == KErrAlreadyExists);

    // Start Ms file system
    RUsbMassStorage usbMs;

    TMassStorageConfig config;
    
    config.iVendorId.Copy(_L("vendorId"));
    config.iProductId.Copy(_L("productId"));
    config.iProductRev.Copy(_L("rev"));


	ret = usbMs.Connect();
    LOG_AND_TEST(KErrNone, ret);
  
    // Start usb mass storage device
    LOG_AND_TEST(KErrNone , usbMs.Start(config));

	TBuf<128> fsName;
		
#if defined(__WINS__)  //we have no "free" non-removable drive at hardware to run this,
			  
    // Get the file system name on non-removable drive

	LOG_AND_TEST(KErrNone ,fs.FileSystemName(fsName, nonRemovalDrvNo));

    // Mount MS FS on to non-removable drive. This should fail
    test(KErrNone != fs.MountFileSystem(KMsFsyName, nonRemovalDrvNo));

    // Unmount MS FS from non-removable drive
	LOG_AND_TEST(KErrNone, fs.DismountFileSystem(KMsFsyName, nonRemovalDrvNo));

    // Mount FAT FS on to drive (restoring)
    LOG_AND_TEST(KErrNone,  fs.MountFileSystem(fsName, nonRemovalDrvNo));
#endif //WINS

    // Format removable drive using FAT FS
    RFormat format;
    
    TBuf<2> removalDrive;
    removalDrive.Append(driveLetter[1]);
    removalDrive.Append(':');
    TInt tracksRemaining;
    test.Next(_L("Start MMC card formatting"));
    LOG_AND_TEST(KErrNone, format.Open(fs, removalDrive, EHighDensity || EQuickFormat, tracksRemaining));
    while (tracksRemaining)
        {
        test.Printf(_L("."));
        LOG_AND_TEST(KErrNone,  format.Next(tracksRemaining));
        }
    format.Close();
  	test.Printf(_L("\nDone!\n"));
    // Get the boot sector info using FAT FS and save it for later comparison
    TBuf8<512> fatBootSector;
    RRawDisk fatDiskF;
    LOG_AND_TEST(KErrNone, fatDiskF.Open(fs, removalDrvNo));
    LOG_AND_TEST(KErrNone, fatDiskF.Read(0, fatBootSector));
	fatDiskF.Close();    

	LOG_AND_TEST(KErrNone, fs.FileSystemName(fsName, removalDrvNo)); 

	// Set up sessions for dismount API tests

	const TInt KNumClients = 10;
    RFs clientFs[KNumClients];
    RFs dismountFs1;
	TRequestStatus trsClientNotify[KNumClients];
	TRequestStatus trsClientComplete;
	TRequestStatus trsClientComplete1;

	LOG_AND_TEST(KErrNone, dismountFs1.Connect());

	TInt i = 0;
	for(i=0; i< KNumClients; i++)
		{
		LOG_AND_TEST(KErrNone, clientFs[i].Connect());
		}

	// Test invalid mode argument to RFs::NotifyDismount

    test.Next(_L("Test invalid mode argument to RFs::NotifyDismount"));
	for(i=0; i< KNumClients; i++)
		{
   		clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i], (TNotifyDismountMode)0xFEEDFACE);
		test(trsClientNotify[i] == KErrArgument);
		}

	// Register for notification of pending media removal and check status

    test.Next(_L("Register for notification of pending media removal and check status"));
	for(i=0; i< KNumClients; i++)
		{
   		clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i]);
		test(trsClientNotify[i] == KRequestPending);
		}

	// Notify clients of pending media removal and check status

    test.Next(_L("Notify clients of pending media removal and check status"));
   	fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
	test(trsClientComplete == KRequestPending);

	// Check that client has notification of pending media removal

    test.Next(_L("Check that client has notification of pending media removal"));
	for(i=0; i< KNumClients; i++)
		{
		test(trsClientNotify[i] == KErrNone);
		}

	// Respond to the dismount using RFs::AllowDismount (only 2 clients)

	LOG_AND_TEST(KErrNone, clientFs[0].AllowDismount(removalDrvNo));
	test(trsClientComplete == KRequestPending);
	LOG_AND_TEST(KErrNone, clientFs[1].AllowDismount(removalDrvNo));
	test(trsClientComplete == KRequestPending);

	// Check that file system can't be dismounted as all clients haven't responded

	test.Next(_L("Check that file system can't be dismounted as all clients haven't responded"));
	LOG_AND_TEST(KErrInUse, fs.DismountFileSystem(fsName, removalDrvNo));

	// Before all clients have completed, cancel the dismount

	test.Next(_L("Before all clients have completed, cancel the dismount"));
	fs.NotifyDismountCancel(trsClientComplete);
	test(trsClientComplete == KErrCancel);

	for(i=2; i< KNumClients; i++)
		{
		LOG_AND_TEST(KErrNone,     clientFs[i].AllowDismount(removalDrvNo));
		LOG_AND_TEST(KErrNotFound, clientFs[i].AllowDismount(removalDrvNo));
		test(trsClientComplete == KErrCancel);
		}

	// Check that file system can be dismounted after cancelling async dismount

	test.Next(_L("Check that file system can be dismounted after cancelling async dismount"));
	LOG_AND_TEST(KErrNone, fs.DismountFileSystem(fsName, removalDrvNo));

	// ...remount FAT

	test.Next(_L("Mount FAT FS on to the removal drive"));
	LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

	for(i=0; i< KNumClients; i++)
	    clientFs[i].Close();

    dismountFs1.Close();

	//
	// Test dismounting while resourses are open
	//
	
	_LIT(KFileName, ":\\foo");
	TBuf<7> fileName;
	fileName.Append(driveLetter[1]);
	fileName.Append(KFileName);

	RFile file;
	test.Next(_L("Attempting to open a file\n\r"));
	ret = file.Create(fs, fileName, EFileRead | EFileReadDirectIO | EFileWriteDirectIO);
	test(ret == KErrNone || ret == KErrAlreadyExists);
	LOG_AND_TEST(KErrNone, file.Write(0,_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")));
	file.Close();

	TBuf<7> dirName;
	dirName.Append(driveLetter[1]);
	dirName.Append(KFileName);

	RDir dir;
	TEntry dirEntry;
	test.Next(_L("Attempting to open a directory\n\r"));
    LOG_AND_TEST(KErrNone, dir.Open(fs, dirName, KEntryAttNormal));
    LOG_AND_TEST(KErrNone, dir.Read(dirEntry));
	dir.Close();

	fs.Close();

	TInt pass;
	for(pass=0; pass<5; pass++)
	{
		LOG_AND_TEST(KErrNone, fs.Connect());
		LOG_AND_TEST(KErrNone, dismountFs1.Connect());

		TInt i = 0;
		for(i=0; i< KNumClients; i++)
			{
			LOG_AND_TEST(KErrNone, clientFs[i].Connect());
			}

		// Open a file on the removable drive
		
		RFile file;
		LOG_AND_TEST(KErrNone, file.Open(fs, fileName, EFileRead | EFileReadDirectIO | EFileWriteDirectIO));
		TBuf8<0x40> buf1;
		LOG_AND_TEST(KErrNone, file.Read(0, buf1));
		LOG_AND_TEST(36, buf1.Length());
		test(buf1 == _L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));

		// Unmount FAT FS from the removable drive - this should fail with a file open
		
		LOG_AND_TEST(KErrInUse, fs.DismountFileSystem(fsName, removalDrvNo));

		// Open a directory  on the removable drive

		test.Next(_L("Attempting to open a directory\n\r"));
		LOG_AND_TEST(KErrNone, dir.Open(fs, dirName, KEntryAttNormal));
		LOG_AND_TEST(KErrNone, dir.Read(dirEntry));

		// Check simple client dismount notification and cancel (before issuing a dismount request)

		test.Next(_L("Register for notification of pending media removal and check status"));
		for(i=0; i< KNumClients; i++)
			{
   			clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Cancel notification of pending media removal and check status"));
		for(i=0; i< KNumClients; i++)
			{
	   		clientFs[i].NotifyDismountCancel(trsClientNotify[i]);
			test(trsClientNotify[i] == KErrCancel);
			}

		// Check issuing and cancelling a dismount request while clients are responding

		test.Next(_L("Register for notification of pending media removal (again) and check status"));
		for(i=0; i< KNumClients; i++)
			{
   			clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Notify clients of pending media removal and check status"));
   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
		test(trsClientComplete == KRequestPending);

		test.Next(_L("Check that client has notification of pending media removal"));
		for(i=0; i< KNumClients; i++)
			{
			test(trsClientNotify[i] == KErrNone);
			}

		LOG_AND_TEST(KErrNone, clientFs[0].AllowDismount(removalDrvNo));
		test(trsClientComplete == KRequestPending);
		LOG_AND_TEST(KErrNone, clientFs[1].AllowDismount(removalDrvNo));
		test(trsClientComplete == KRequestPending);
		test.Next(_L("Before all clients have completed, cancel the dismount"));
		fs.NotifyDismountCancel(trsClientComplete);
		test(trsClientComplete == KErrCancel);
		
		for(i=2; i< KNumClients; i++)
			{
			LOG_AND_TEST(KErrNone, clientFs[i].AllowDismount(removalDrvNo));
			test(trsClientComplete == KErrCancel);
			}

		// Check dismounting, responding, cancelling and forced remounting

		test.Next(_L("Register for notification of pending media removal (again) and check status"));
		for(i=0; i< KNumClients; i++)
			{
   			clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Notify clients of pending media removal and check status"));
   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
		test(trsClientComplete == KRequestPending);

		test.Next(_L("Check that client has notification of pending media removal"));
		for(i=0; i< KNumClients; i++)
			{
			test(trsClientNotify[i] == KErrNone);
			}

		test.Next(_L("Notify clients of pending media removal with another session and check status"));
   		fs.NotifyDismount(removalDrvNo, trsClientComplete1, EFsDismountNotifyClients);
		test(trsClientComplete1 == KErrInUse);

		TInt expectedAllowDismountRet = KErrNone;
		TInt expectedCompletionCode = KRequestPending;
		if(pass & 0x01)
			{
			test.Next(_L("No response from clients - Force a dismount"));
			test.Next(_L("...cancelling original request"));
	   		fs.NotifyDismountCancel(trsClientComplete);
			test(trsClientComplete == KErrCancel);
			test.Next(_L("...issuing a forced dismount request"));
	   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountForceDismount);
			test(trsClientComplete == KErrNone);
			expectedAllowDismountRet = KErrNotReady;
			expectedCompletionCode = KErrNone;
			}

		test.Next(_L("Allow dismount and check response"));
		for(i=0; i < KNumClients; i++)
			{
			LOG_AND_TEST(expectedAllowDismountRet, clientFs[i].AllowDismount(removalDrvNo));
			if(i == KNumClients-1)
				test(trsClientComplete == KErrNone);
			else
				test(trsClientComplete == expectedCompletionCode);
			}

		// The last test should have dismounted the file system

		LOG_AND_TEST(KErrNotReady, file.Read(0,buf1));
		LOG_AND_TEST(KErrNotReady, dir.Read(dirEntry));

		test.Next(_L("FAT File System should now be dismounted from the drive"));
		LOG_AND_TEST(KErrNotReady, fs.DismountFileSystem(fsName, removalDrvNo));

		test.Next(_L("Mount MS FS on to the removable drive"));
		LOG_AND_TEST(KErrNone, fs.MountFileSystem(KMsFsyName, removalDrvNo));

		LOG_AND_TEST(KErrDisMounted, file.Read(0,buf1));
		LOG_AND_TEST(KErrDisMounted, dir.Read(dirEntry));

		test.Next(_L("Dismount MSFS normally"));
		LOG_AND_TEST(KErrNone, fs.DismountFileSystem(KMsFsyName, removalDrvNo));

		test.Next(_L("Mount FAT FS on to the removal drive"));
		LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

		LOG_AND_TEST(KErrNone, file.Read(0,buf1));
		LOG_AND_TEST(KErrEof, dir.Read(dirEntry));	// drive freshly formatted, so only root dir exists

		// Test multiple notifiers on a single session

		test.Next(_L("Register several notifiers for a single session and check status"));
		for(i=0; i< KNumClients; i++)
			{
   			clientFs[0].NotifyDismount(removalDrvNo, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Notify clients and verify all requests signalled"));
   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);

		test.Next(_L("Allow dismount 3 times from same session"));
		for(i=0; i< KNumClients; i++)
			{
			test(trsClientComplete == KRequestPending);
			test(trsClientNotify[i] == KErrNone);
			LOG_AND_TEST(KErrNone, clientFs[0].AllowDismount(removalDrvNo));
			}

		test.Next(_L("Verify that file system has been dismounted"));
		test(trsClientComplete == KErrNone);

		test.Next(_L("Mount FAT FS on to the removal drive"));
		LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

		// Test multiple notifiers on different drives

		const TInt KNumDrives = 1;

		test.Next(_L("Register several notifiers for different drives and check status"));
		for(i=0; i < KNumDrives; i++)
			{
   			clientFs[0].NotifyDismount(removalDrvNo + i, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Notify clients and verify all requests signalled"));
   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
		test(trsClientComplete == KRequestPending);

		test(trsClientNotify[0] == KErrNone);
		LOG_AND_TEST(KErrNone, clientFs[0].AllowDismount(removalDrvNo));
		for(i=1; i< KNumDrives; i++)
			{
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Verify that file system has been dismounted"));
		test(trsClientComplete == KErrNone);

		test.Next(_L("Check that file can be closed when filesystem is dismounted"));
		file.Close();
		dir.Close();

		test.Next(_L("Mount FAT FS on to the removal drive"));
		LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

		test.Next(_L("Cancel all outstanding notifiers for this session"));
   		clientFs[0].NotifyDismountCancel();
		for(i=1; i< KNumDrives; i++)
			{
			test(trsClientNotify[i] == KErrCancel);
			}

		// Test session disconnect
		test.Next(_L("Register for notification of pending media removal (again) and check status"));
		for(i=0; i< KNumClients; i++)
			{
   			clientFs[i].NotifyDismount(removalDrvNo, trsClientNotify[i]);
			test(trsClientNotify[i] == KRequestPending);
			}

		test.Next(_L("Close client sessions with outstanding notifiers"));
		for(i=0; i< KNumClients; i++)
			clientFs[i].Close();

		// Since all clients have been closed, the next stage should result in a dismount
		test.Next(_L("Notify clients of pending media removal and check status"));
		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
		test(trsClientComplete == KErrNone);

		test.Next(_L("Mount FAT FS on to the removal drive"));
		LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

		TRequestStatus trs1;
		dismountFs1.NotifyDismount(removalDrvNo, trs1);
		test(trs1 == KRequestPending);

   		fs.NotifyDismount(removalDrvNo, trsClientComplete, EFsDismountNotifyClients);
		test(trsClientComplete == KRequestPending);

		fs.NotifyDismountCancel(trsClientComplete);

		fs.Close();
		dismountFs1.Close();
	}

	// Check that files/directories can't be opened on Mass Storage
	
	LOG_AND_TEST(KErrNone, fs.Connect());

	test.Next(_L("Dismount FAT File System"));
	LOG_AND_TEST(KErrNone, fs.DismountFileSystem(fsName, removalDrvNo));

	test.Next(_L("Mount MS FS on to the removable drive"));
	LOG_AND_TEST(KErrNone, fs.MountFileSystem(KMsFsyName, removalDrvNo));

	test.Next(_L("Attempting to open a file\n\r"));
	LOG_AND_TEST(KErrNotReady, file.Open(fs, fileName, EFileRead));

	test.Next(_L("Attempting to open a directory\n\r"));
    LOG_AND_TEST(KErrNotReady, dir.Open(fs, fileName, KEntryAttNormal));

	// Test fix for DEF058681 - Mass Storage reports VolumeName incorrectly.
	// Before the fix, CMassStorageMountCB::MountL() did not set the volume
	// name, resulting in a panic when attempting to copy a null descriptor.
	// Note that the volume name is still not returned client-side, since
	// CMassStorageMountCB::VolumeL() returns KErrNotReady
	TVolumeInfo volInfo;
	ret = fs.Volume(volInfo, removalDrvNo);
	LOG_AND_TEST(ret, KErrNotReady);
	LOG_AND_TEST(volInfo.iName.Length(), 0);

	// -------------------------------------------------
   
    // Get the boot sector info using MS FS and save it for later comparison
    TBuf8<CBW_LENGTH> cbwBuf; 
    TInt dCBWTag = 1234567;    // arbitrary, any number would do for this test.
 
    RDevTestUsbcClient usbcClient;

    // Open a session to LDD
    test.Next(_L("Open LDD"));
    LOG_AND_TEST(KErrNone, usbcClient.Open(0));

	// Build SCSI command test unit ready
    TBuf8<SCSI_UNIT_READY_LEN> unitReadyBuf;
    // Zero out the buf
    unitReadyBuf.FillZ(SCSI_UNIT_READY_LEN);

    createCBW(cbwBuf, dCBWTag, 0, 0, SCSI_UNIT_READY_LEN, unitReadyBuf);

    // Send test unit ready command
    test.Next(_L("Sending CBW with 'Unit Ready' cmd"));
	TRequestStatus status;
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH); 
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Read CSW
	test.Next(_L("Reading CSW"));
    TBuf8<CSW_LENGTH> cswBuf;
    usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Check dCSWTag
    TInt recvedCBWTag = extractInt(&cswBuf[4]);
    LOG_AND_TEST(dCBWTag,  recvedCBWTag);
     
    // Check bCSWStatus
    TInt bCSWStatus = cswBuf[CSW_LENGTH - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    LOG_AND_TEST(bCSWStatus,  1);

	//============================================

	// Create a CBW for SCSI read (10) command to read the boot sector via MS FS 
    TBuf8<SCSI_READ_LEN> readBuf;  
    // 0: starting sector; 1: total blocks    
    createReadCmd(readBuf, 0, 1); 
    
    // 0x80: data-in to the host; 10: read (10) command length
    createCBW(cbwBuf, ++dCBWTag, BOOTSECTOR_SIZE, 0x80, 10, readBuf);
    

	// Send CBW to the LDD
    test.Next(_L("Send CBW with 'Read' Command"));
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone, status.Int());


	
	test.Next(_L("Reading bootsector data"));
    // Read the boot sector 
    TBuf8<BOOTSECTOR_SIZE> msBootSector;
    usbcClient.HostRead(status, EEndpoint2, msBootSector, BOOTSECTOR_SIZE); 
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone, status.Int());

    // Read CSW
    test.Next(_L("Reading CSW"));
    usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone, status.Int());

    // Check dCBWTag
    recvedCBWTag = extractInt(&cswBuf[4]);
    LOG_AND_TEST(dCBWTag,  recvedCBWTag);
     
    // Check bCSWStatus
    bCSWStatus = cswBuf[CSW_LENGTH - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    
    LOG_AND_TEST(bCSWStatus,  0);

    // Compare FAT FS boot sector with MS FS boot sector
    // When accessing the medium through USB, it is accessed raw. That means 
    // we have to find the boot record (BPB), which may be in sector 0 if the 
    // medium has no MBR or elsewhere if it is has a MBR. (Details of the 
    // identification can be found in the FAT32 specification)
    TUint16 signature;
    
    signature = (TUint16) (msBootSector[KMBRSignatureOffset] | 
                msBootSector[KMBRSignatureOffset + 1] << 8);
                
	LOG_AND_TEST(signature, KBootRecordSignature);
	
	if(((msBootSector[0] == 0xEB && msBootSector[2] == 0x90) || (msBootSector[0] == 0xE9)) && 
		(msBootSector[16] >= 1) && ((msBootSector[21] == 0xF0) || (msBootSector[21] >= 0xF8)))
		{
		test.Printf(_L("BPB identified in sector 0.\r\n"));			
		}
	else
		{	
		test.Printf(_L("Assume sector 0 to be MBR - attempting to locate BPB\r\n"));
		// Read the offset to the first partition to find the boot record...
		// 32bit int stored as little endian
		TUint32 bootSectorLocation;
		bootSectorLocation = msBootSector[KMBRFirstPartitionSectorOffset]           | 
							 msBootSector[KMBRFirstPartitionSectorOffset + 1] << 8  |
							 msBootSector[KMBRFirstPartitionSectorOffset + 2] << 16 |
							 msBootSector[KMBRFirstPartitionSectorOffset + 3] << 24;
		
		test.Printf(_L("Reading Boot Sector from offset %d\r\n"), bootSectorLocation);

		// Create a CBW for SCSI read (10) command to read the boot sector via MS FS 
		TBuf8<SCSI_READ_LEN> readBuf;  
		// 0: starting sector; 1: total blocks    
		createReadCmd(readBuf, bootSectorLocation, 1); 
		
		// 0x80: data-in to the host; 10: read (10) command length
		createCBW(cbwBuf, ++dCBWTag, BOOTSECTOR_SIZE, 0x80, 10, readBuf);

		// Send CBW to the LDD
		test.Next(_L("Send CBW with 'Read' Command"));
		usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH);
		User::WaitForRequest(status);
		LOG_AND_TEST(KErrNone, status.Int());
		
		// Read the boot sector 
		usbcClient.HostRead(status, EEndpoint2, msBootSector, BOOTSECTOR_SIZE); 
		User::WaitForRequest(status);
		LOG_AND_TEST(KErrNone, status.Int());
		
		// Read CSW
		test.Next(_L("Reading CSW"));
		usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
		User::WaitForRequest(status);
		LOG_AND_TEST(KErrNone, status.Int());
		
		// Check dCBWTag
		recvedCBWTag = extractInt(&cswBuf[4]);
		LOG_AND_TEST(dCBWTag,  recvedCBWTag);
		 
		// Check bCSWStatus
		bCSWStatus = cswBuf[CSW_LENGTH - 1];
		test.Printf(_L("CSW status: %d\n"), bCSWStatus);
		
		LOG_AND_TEST(bCSWStatus,  0);
		}
    
    test (0 == fatBootSector.Compare(msBootSector));
    
    //
    // Create a CBW to prevent medium removal
    // 
    TBuf8<SCSI_MED_RMVL_LEN> tBuf; 
    // Zero out the buf
    tBuf.FillZ(SCSI_MED_RMVL_LEN);
    tBuf[0] = 0x1E;
    tBuf[4] = 1;        // prevent medium removal 

    cbwBuf.FillZ(CBW_LENGTH);
    createCBW(cbwBuf, ++dCBWTag, 0, 0, SCSI_MED_RMVL_LEN, tBuf);

    // Send prevent medium removal command
    test.Next(_L("Sending CBW with 'Prevent media removal' cmd"));
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH); 
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Read CSW
    test.Next(_L("Reading CSW"));
    usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone, status.Int());

    // Check dCSWTag
    recvedCBWTag = extractInt(&cswBuf[4]);
    LOG_AND_TEST(dCBWTag,  recvedCBWTag);
     
    // Check bCSWStatus
    bCSWStatus = cswBuf[CSW_LENGTH - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    
	if(bCSWStatus ==  0)
		{
    
		// Try to unmount MS FS. This should fail as medium removal is disallowed
		test.Next(_L("Dismounting MSFS"));
		test(KErrNone != fs.DismountFileSystem(KMsFsyName, removalDrvNo));
		
		// 
		// Create a CBW to allow medium removal
		//
		// Zero out the buf
		tBuf.FillZ(SCSI_MED_RMVL_LEN);
		tBuf[0] = 0x1E;
		tBuf[4] = 0;        // allow medium removal 

		createCBW(cbwBuf, ++dCBWTag, 0, 0, SCSI_MED_RMVL_LEN, tBuf);

		// Send allow medium removal command
		test.Next(_L("Sending CBW with 'Allow media removal' cmd"));
		usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH); 
		User::WaitForRequest(status);
		LOG_AND_TEST(KErrNone, status.Int());

		// Read CSW
		test.Next(_L("Reading CSW"));
		usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
		User::WaitForRequest(status);
		LOG_AND_TEST(KErrNone, status.Int());

		// Check dCSWTag
		recvedCBWTag = extractInt(&cswBuf[4]);
		LOG_AND_TEST(dCBWTag, recvedCBWTag);
     
		// Check bCSWStatus
		bCSWStatus = cswBuf[CSW_LENGTH - 1];
		test.Printf(_L("CSW status: %d\n"), bCSWStatus);
		LOG_AND_TEST(bCSWStatus,  0);

		}
    else
		test.Printf(_L("Prevent Media Removal command not supported, skipping appropriate tests"));


    // Try to unmount MS FS again. This time it should succeed
	LOG_AND_TEST(KErrNone, fs.DismountFileSystem(KMsFsyName, removalDrvNo));

    // Read the boot sector while MS FS is unmounted, this should fail
    test(KErrNone != fatDiskF.Open(fs, removalDrvNo));
    fatDiskF.Close();    
    
    // Mount FAT FS on to the removal drive
    LOG_AND_TEST(KErrNone, fs.MountFileSystem(fsName, removalDrvNo));

	// Additional step for DEF079149: File server crash when re-adding 
	// MSFS.FSY. Before the fix was applied this call to RFs::AddFileSystem 
	// would cause a crash.
	LOG_AND_TEST(KErrAlreadyExists,fs.AddFileSystem(KMsFs));

    // Read the boot sector after FAT MS is mounted on to the removal drive
    LOG_AND_TEST(KErrNone,  fatDiskF.Open(fs, removalDrvNo));
    LOG_AND_TEST(KErrNone,  fatDiskF.Read(0, fatBootSector));
    fatDiskF.Close();

    createCBW(cbwBuf, ++dCBWTag, 0, 0, SCSI_UNIT_READY_LEN, unitReadyBuf);

    // Send test unit ready command
    test.Next(_L("Sending CBW with 'Unit Ready' cmd"));
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH); 
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Read CSW
    usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Check dCSWTag
    recvedCBWTag = extractInt(&cswBuf[4]);
    LOG_AND_TEST(dCBWTag,  recvedCBWTag);
     
    // Check bCSWStatus
    bCSWStatus = cswBuf[CSW_LENGTH - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    LOG_AND_TEST(bCSWStatus,  1);
 
    // 
    // Read MS FS using SCSI read command, this should fail as
    // FAT FS is mounted instead
    //
    
    // 0x80: data-in to the host; 10: read (10) command length

    createCBW(cbwBuf, ++dCBWTag, BOOTSECTOR_SIZE, 0x80, 10, readBuf);

    // Send CBW to the LDD
    test.Next(_L("SEnding CBW with 'Read'"));
    usbcClient.HostWrite(status, EEndpoint1, cbwBuf, CBW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Read the sector 
    usbcClient.HostRead(status, EEndpoint2, msBootSector, BOOTSECTOR_SIZE); 
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());


    // Read CSW
    usbcClient.HostRead(status, EEndpoint2, cswBuf, CSW_LENGTH);
    User::WaitForRequest(status);
    LOG_AND_TEST(KErrNone,  status.Int());

    // Check dCSWTag
    recvedCBWTag = extractInt(&cswBuf[4]);
    LOG_AND_TEST(dCBWTag , recvedCBWTag);
     
    // Check bCSWStatus
    bCSWStatus = cswBuf[CSW_LENGTH - 1];
    test.Printf(_L("CSW status: %d\n"), bCSWStatus);
    test(bCSWStatus != 0);
    
   
    // Read FAT FS using RFs API
    LOG_AND_TEST(KErrNone,  fatDiskF.Open(fs, removalDrvNo));
    LOG_AND_TEST(KErrNone,  fatDiskF.Read(0, fatBootSector));
    fatDiskF.Close();    
    
    // Stop usb mass storage device
    LOG_AND_TEST(KErrNone,  usbMs.Stop());
    usbMs.Close();
	User::After(1000000);
    
    ret = fs.RemoveFileSystem(KMsFsyName);
	fs.Close();
	
	usbcClient.Close();
	ret = User::FreeLogicalDevice(_L("USBC"));
	LOG_AND_TEST(ret,  KErrNone);

	test.Printf(_L("Exiting test\r\n"));	
	__UHEAP_MARKEND;
	}
	


GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
    // Parse the CommandLine arguments: removal drive and non-removal drive
    ParseCommandArguments();

    // Run test
	test.Start( _L("Test mountstart") );
    doComponentTest();
    test.End();
    }
