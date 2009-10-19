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
// file: t_ms_scsi_steps.cpp
// SCSI protocol test cases.
// 
//

/**
 @file
 @internalTechnology
*/


#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <f32fsys.h>

#include "massstoragedebug.h"

#include "t_ms_main.h"
#include "t_ms_scsi.h"




TInt CScsiTest::TestLun = 1;
// see scsiprot.cpp
LOCAL_D const TUint KDefaultBlockSize = 0x200;

LOCAL_D TBool mediaChanged = EFalse;

LOCAL_D const TInt KMaxLuns = 8;

/**
t_scsi_inquiry_normal case
*/
GLDEF_C void t_scsi_inquiry_normal()
	{
	test.Next(_L("t_scsi_inquiry_normal"));
	
	CScsiTest* scsiTest = CScsiTest::NewLC();

	TMassStorageConfig config;
	config.iVendorId.FillZ(8);
	config.iVendorId.Copy(_L("UnitTest"));
	config.iVendorId.SetLength(8);
	
	config.iProductId.FillZ(16);
	config.iProductId.Copy(_L("ProductTest"));
	config.iProductId.SetLength(16);
	
	config.iProductRev.FillZ(4);
	config.iProductRev.Copy(_L("Rev"));
	config.iProductRev.SetLength(4);
	
	TEST_FOR_VALUE (scsiTest->iScsiProt->SetScsiParameters(config), KErrNone);

	TInquiryCmd cmd;
	TPtrC8 pBuf(cmd.iCmd);
	
	// CASE: media is Not connected
	TEST_FOR_VALUE (scsiTest->DecodePacket(pBuf), ETrue);
	{
	TInquiryData inquiryData(scsiTest->iTransport.iBufWrite);

	// test 		value					expected
	TEST_FOR_VALUE (inquiryData.DeviceType(),	0);
	TEST_FOR_VALUE (inquiryData.RMB(), 			1);
	TEST_FOR_VALUE (inquiryData.Version(), 		0);
	TEST_FOR_VALUE (inquiryData.RespDataFrmt(),	0x02);
	TEST_FOR_VALUE (inquiryData.PeripheralQualifier(), 0);	// to be 'connected'
	
	TEST_FOR_VALUE ((35 <= inquiryData.Length()),ETrue);		// at least min length

	// these values were set above in this function
	TMassStorageConfig configInquiry;
	configInquiry.iVendorId.Copy(inquiryData.VendorId());
	configInquiry.iProductId.Copy(inquiryData.ProductId());
	configInquiry.iProductRev.Copy(inquiryData.RevisionLevel());
	__PRINT1 (_L("VendorId: %S\n"), &configInquiry.iVendorId);
	__PRINT1 (_L("ProductId: %S\n"), &configInquiry.iProductId);
	__PRINT1 (_L("ProductRev: %S\n"), &configInquiry.iProductRev);

	TEST_FOR_VALUE((configInquiry.iVendorId == config.iVendorId),	ETrue);
	TEST_FOR_VALUE((configInquiry.iProductId == config.iProductId), 	ETrue);
	TEST_FOR_VALUE((configInquiry.iProductRev == config.iProductRev), ETrue);
	}
	__PRINT (_L("media not connected ===> passed OK\n"));

	
	// CASE: emulate 'mount and connect media'	
	// set connection to physical device as Connected
	scsiTest->MountTestDrive();
	TEST_FOR_VALUE (scsiTest->DecodePacket(pBuf), ETrue);
	{
	TInquiryData inquiryData(scsiTest->iTransport.iBufWrite);
	TEST_FOR_VALUE(inquiryData.PeripheralQualifier(), 0);	// to be 'connected'
	}
	__PRINT (_L("media not connected ===> passed OK\n"));
	
	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest

	__PRINT (_L("t_scsi_inquiry_normal ===> passed OK\n"));
	}



/**
t_scsi_inquiry_error case
*/
GLDEF_C void t_scsi_inquiry_error()
	{
	test.Next(_L("t_scsi_inquiry_error"));
	
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	TInquiryCmd cmd;
	cmd.iCmd[2] = 0x02; 	// set CmdDt = 1 (2nd bit in 2nd byte)
	TPtrC8 pBuf(cmd.iCmd);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pBuf), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	
	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest
	
	__PRINT (_L("t_scsi_inquiry_error ===> passed OK\n"));
	}


/**
t_scsi_test_unit_ready case
*/
GLDEF_C void t_scsi_test_unit_ready()
	{
	test.Next(_L("t_scsi_test_unit_ready"));
	
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// CASE: NOT registered media
	TTestUnitReadyCmd cmd;
	TPtrC8 pBuf(cmd.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pBuf), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(),
						TSenseInfo::ENotReady,
						TSenseInfo::EMediaNotPresent);


	// CASE: registered media
	scsiTest->MountTestDrive();
	// repeat TEST UNIT READY command
	TEST_FOR_VALUE(scsiTest->DecodePacket(pBuf), ETrue);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENoSense, 0);
	
	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest
	
	__PRINT (_L("t_scsi_test_unit_ready ===> passed OK\n"));
	}

/**
t_scsi_prevent_allow_media_removal case
*/
GLDEF_C void t_scsi_prevent_allow_media_removal()
	{
	test.Next(_L("t_scsi_prevent_allow_media_removal"));
	
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// SPC-2, p. 113
	// Notes: limitation in SCSI protocol:
	// SCSI protocol uses the 1st bit of PREVENT field only 
	TMediaRemovalCmd cmdAllow;
	cmdAllow.iCmd[5] = 0x00;		// allow
	TPtrC8 pCmdAllow(cmdAllow.iCmd);
	
	TMediaRemovalCmd cmdPrevent;
	cmdPrevent.iCmd[5] = 0x01;	// prevent
	TPtrC8 pCmdPrevent(cmdPrevent.iCmd);
	
	
	// CASE: do NOT register media AND allow media removal
	TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdAllow), EFalse);

	TEST_SENSE_CODE_IF_SUPPORTED(scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent)
		{
		 __PRINT(_L("NOT register media AND allow removal ===> passed OK\n"));


		// CASE: do NOT register media AND prevent media removal
		TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdPrevent), EFalse);
		TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		 __PRINT(_L("NOT register media AND prevent removal ===> passed OK\n"));


		// CASE: Register media AND prevent removal 
		scsiTest->MountTestDrive();
		TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdPrevent), ETrue);
		// check setting in a Drive. the drive has to set to Active state
		TEST_FOR_VALUE(scsiTest->TestDrive()->DriveState(),	CMassStorageDrive::EActive);
		 __PRINT(_L("Register media AND prevent removal ===> passed OK\n"));


		// CASE:Register media AND allow removal
		TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdAllow), ETrue);
		// check setting in a Drive. the drive has to set to Idle state
		TEST_FOR_VALUE(scsiTest->TestDrive()->DriveState(),	CMassStorageDrive::EIdle);
		 __PRINT(_L("Register media AND allow removal ===> passed OK\n"));
		}

	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest
	 __PRINT(_L("t_scsi_prevent_allow_media_removal ===> passed OK\n"));
	}


/**
t_scsi_start_stop_unit case
*/
GLDEF_C void t_scsi_start_stop_unit()
	{
	test.Next(_L("t_scsi_start_stop_unit\n"));
	
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// SBC-2, p. 64 of scsi block commands spec
	// Notes: limitation in SCSI protocol:
	// SCSI protocol doesn't support the following fields:
	// -- IMMED
	// -- POWER CONDITIONS
	// => the supported fields are
	// -- START
	// -- LOEJ
	TBuf8<7> cmdStart;
	cmdStart.FillZ(7);
	cmdStart[0] = 0x06;		// size of a command itself
	cmdStart[1] = 0x1B; 	// START STOP UNIT command
	cmdStart[2] = 0x01;		// Get result immediatly
	cmdStart[5] = 0x03;		// start: LOEJ + START
	TPtrC8 pCmdStart(cmdStart);
	
	TBuf8<7> cmdStop;
	cmdStop.FillZ(7);
	cmdStop[0] = 0x06;		// size of a command itself
	cmdStop[1] = 0x1B; 		// START STOP UNIT command
	cmdStop[2] = 0x01;		// Get result immediatly
	cmdStop[5] = 0x02;		// stop: LOEJ
	TPtrC8 pCmdStop(cmdStop);
	
	
	// CASE: do NOT register media AND Start
	// NOT APPLICABLE as there is no such thing as 'register media' in latest implementation


	// CASE: do NOT register media AND stop
	// NOT APPLICABLE as there is no such thing as 'register media' in latest implementation


	// CASE: Start unit
	TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdStart), ETrue);
	// check setting in a Drive. the drive has to set to Connecting state
	TEST_FOR_VALUE (scsiTest->TestDrive()->MountState(), CMassStorageDrive::EConnecting);
	 __PRINT(_L("Registered media AND Start unit ===> passed OK\n"));


 	// CASE: Stop unit
	TEST_FOR_VALUE(scsiTest->DecodePacket(pCmdStop), ETrue);
	// check setting in a Drive. the drive has to set to Disconnecting state
	TEST_FOR_VALUE(scsiTest->TestDrive()->MountState(),		CMassStorageDrive::EDisconnecting);
	 __PRINT(_L("Registered media AND stop unit ===> passed OK\n"));
	
	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest
	
	 __PRINT(_L("t_scsi_start_stop_unit ===> passed OK\n"));
	}


/**
t_scsi_request_sense case
*/
GLDEF_C void t_scsi_request_sense()
	{
	test.Next(_L("t_scsi_request_sense\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();

	// -- check sense BEFORE any other command
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENoSense, 0);


	// CASE: don't register drive and send UNIT READY
	TTestUnitReadyCmd cmdUnitReady;
	TPtrC8 pRequestSenseCmd (cmdUnitReady.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pRequestSenseCmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
	// check sense code right away
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENoSense, 0);


	// CASE: do register media
	scsiTest->MountTestDrive();
	// repeat TEST UNIT READY command
	TEST_FOR_VALUE (scsiTest->DecodePacket(pRequestSenseCmd), ETrue);


	// CASE: deregister media
	scsiTest->DismountTestDrive();
	TEST_FOR_VALUE (scsiTest->DecodePacket(pRequestSenseCmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
	// check sense code right away
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENoSense, 0);

	
	// cleanup
	CleanupStack::PopAndDestroy(1); // scsiTest
	 __PRINT(_L("t_scsi_request_sense ===> passed OK\n"));
	}


/**
t_scsi_read case
*/
GLDEF_C void t_scsi_read()
	{
	test.Next (_L("t_scsi_read\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	// and initialize proxy drive buffers
	scsiTest->iProxyDrive.Initialise();

	// SBC-2 p.50
	// dev notes: latest implementation of scsi protocol supports only:
	//			- Logical block address
	//			- Transfer length
	//			- RDPROTECT (only true or false)
	//     all other fields are not supported
	TReadWrite10Cmd cmd;
	cmd.SetRead();	// setup cmd with correct header
	TPtrC8 pRead10Cmd(cmd.iCmd);
	// extra check for iMediaBuf
	ASSERT (scsiTest->iProxyDrive.iMediaBuf.Length() / KDefaultBlockSize < 0x0000FFFF);
	TUint16 maxNumBlocks = static_cast<TUint16>(scsiTest->iProxyDrive.iMediaBuf.Length() / KDefaultBlockSize);
	TUint16 readBlocks = 1;
	TUint32 offsetBlocks = 1;

	// CASE: register drive; nothing to read
	scsiTest->MountTestDrive();
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), ETrue);
	 __PRINT(_L("Request to read 0 bytes ===> passed OK\n"));


	// CASE: read min (1 block) with offset = 0
	readBlocks = 1;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(0);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), ETrue);
	{
	TPtrC8 transportBuf = scsiTest->iTransport.iBufWrite.Left(readBlocks * KDefaultBlockSize);
	TPtrC8 driveBuf(TPtrC8(scsiTest->iProxyDrive.iMediaBuf).Left(readBlocks * KDefaultBlockSize));
	// data should be identical
	TEST_FOR_VALUE(driveBuf.Compare(transportBuf), 0);
	}
	 __PRINT(_L("Request to read 1 block @ offset 0 ===> passed OK\n"));
	
	
	// CASE: read after the end
	readBlocks = 10;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(maxNumBlocks - readBlocks/2);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to read AFTER the end of a media ===> passed OK\n"));
	

	// CASE: read from after the end
	readBlocks = 10;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(maxNumBlocks + readBlocks/2);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to start read AFTER the end of a media ===> passed OK\n"));
	
	
	// CASE: check for internal buffer overflow (with offset)
	// dev note: this check is specific to SCSI prot implementation
	// 			 and NOT described by SBC-2 
	offsetBlocks = 1;
	readBlocks = static_cast<TUint16>(maxNumBlocks - offsetBlocks);
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT(_L("Check for internal buffer overflow ===> passed OK\n"));

	
	// CASE: read max with offset (= offsetBlocks block)
	offsetBlocks = 2;
	//				max size in blocks supported by SCSI prot  - offsetBlocks
	readBlocks = static_cast<TUint16>((KMaxBufSize / KDefaultBlockSize) - offsetBlocks);
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), ETrue);
	{
	TPtrC8 transportBuf (scsiTest->iTransport.iBufWrite.Left(readBlocks * KDefaultBlockSize));
	TPtrC8 driveBuf(TPtrC8(scsiTest->iProxyDrive.iMediaBuf).Mid(offsetBlocks * KDefaultBlockSize, readBlocks * KDefaultBlockSize));
	// data should be identical
	TEST_FOR_VALUE(driveBuf.Compare(transportBuf), 0);
	}
	 __PRINT(_L("Check for max read ===> passed OK\n"));


	// CASE: boundary conditions
	offsetBlocks = 0xFFFFFFFF;
	readBlocks = 0xFFFF;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Check for boundary conditions ===> passed OK\n"));

	
	// CASE: try RDProtect
	// dev note: scsi protocol doesn't support any value except 000b
	cmd.SetProtect (1);
	offsetBlocks = 1;
	readBlocks = 1;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket(pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT(_L("Check for RD Protect ===> passed OK\n"));


	// CASE: invalid CAPs
	cmd.SetProtect (0);
	offsetBlocks = 1;
	readBlocks = 1;
	cmd.SetTransferLength (readBlocks);
	cmd.SetBlockAddress (offsetBlocks);
	// create error conditions in drive
	TInt error = KErrNone;
	CMassStorageDrive* pDrive = scsiTest->iDriveManager->Drive (CScsiTest::TestLun, error);
	ASSERT (KErrNone == error && pDrive);
	delete pDrive->iLocalDrive;
	pDrive->iLocalDrive = NULL;
	
	TEST_FOR_VALUE (scsiTest->DecodePacket (pRead10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
	 __PRINT(_L("Check for invalid caps ===> passed OK\n"));
	// NOTE: be aware that CMassStorageDrive is in invalid state at this point 
	//		as Drive->iLocalDrive was set to NULL
	
	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT(_L("t_scsi_read ===> passed OK\n"));
	}


/**
t_scsi_write case
*/
GLDEF_C void t_scsi_write()
	{
	test.Next (_L("t_scsi_write\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	// and initialize Read/Write buffers
	scsiTest->iProxyDrive.Initialise();
	scsiTest->iTransport.InitialiseReadBuf();

	// SBC-2 p.80
	// dev notes: latest implementation of scsi protocol supports only:
	//			- Logical block address
	//			- Transfer length
	//			- WRPROTECT (only true or false)
	//     all other fields are not supported
	TReadWrite10Cmd cmd;
	cmd.SetWrite(); // setup cmd with correct header
	TPtrC8 pWrite10Cmd(cmd.iCmd);
	
	TUint16 maxNumBlocks = static_cast<TUint16>(scsiTest->iProxyDrive.iMediaBuf.Length() / KDefaultBlockSize);
	TUint16 writeBlocks = 1;
	TUint32 offsetBlocks = 1;

	// CASE: register drive; nothing to read
	scsiTest->MountTestDrive();
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), ETrue);
	 __PRINT(_L("Request to write 0 bytes ===> passed OK\n"));


	// CASE: write min
	writeBlocks = 1;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(0);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), ETrue);
	// complete read
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrNone), KErrNone);
	{
	// note: offset == 0 in this case
	TPtrC8 transportBuf = scsiTest->iTransport.iBufRead.Left(writeBlocks * KDefaultBlockSize);
	TPtrC8 driveBuf(TPtrC8(scsiTest->iProxyDrive.iMediaBuf).Mid(0, writeBlocks * KDefaultBlockSize));
	// data should be identical
	TEST_FOR_VALUE (driveBuf.Compare(transportBuf), 0);
	}
	 __PRINT(_L("Request to write 1 block @ offset 0 ===> passed OK\n"));


	// CASE: 	KMediaAttWriteProtected
	scsiTest->iProxyDrive.iCaps.iMediaAtt = KMediaAttWriteProtected;
	cmd.SetTransferLength (1);
	cmd.SetBlockAddress (0);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EDataProtection, TSenseInfo::EWriteProtected);
	// and CASE: KMediaAttLocked
	scsiTest->iProxyDrive.iCaps.iMediaAtt = KMediaAttLocked;
	TEST_FOR_VALUE (scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EDataProtection, TSenseInfo::EWriteProtected);
	// restore media attributes
	scsiTest->iProxyDrive.iCaps.iMediaAtt = 0;
	 __PRINT(_L("Request to write to write protected media ===> passed OK\n"));
	
	
	// CASE: check for ReadComplete error during transport READ
	writeBlocks = 1;
	cmd.SetTransferLength (writeBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), ETrue);
	// complete read
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrOverflow), KErrAbort);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EAbortedCommand, 0);
	 __PRINT(_L("Case: error during transport READ ===> passed OK\n"));
	// CASE: KUndefinedCommand in ReadComplete
	// note: keep this test right after CASE: check for ReadComplete
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrNone), KErrAbort);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EAbortedCommand, 0);
	 __PRINT(_L("Case: error Undefined Command in ReadComplete ===> passed OK\n"));


	// CASE: write after the end
	writeBlocks = 10;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(maxNumBlocks - writeBlocks/2);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to write AFTER the end of a media ===> passed OK\n"));
	

	// CASE: write from after the end
	writeBlocks = 10;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(maxNumBlocks + writeBlocks/2);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to start write AFTER the end of a media ===> passed OK\n"));
	
	
	// CASE: check for internal buffer overflow (with offset)
	// dev note: this check is specific to SCSI prot implementation
	// 			 and NOT described by SBC-2 
	//
	// NOTE: Contrary to the the above comment, the USB Compliance Tests require this
	//		 test case to pass, hence the EEinvalidFieldInCdb check has been removed
	//
	offsetBlocks = 1;
	writeBlocks = static_cast<TUint16>(maxNumBlocks - offsetBlocks);
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), ETrue);
//	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT(_L("Check for internal buffer overflow ===> passed OK\n"));

	
	// CASE: write max with offset (= offsetBlocks block)
	offsetBlocks = 2;
	//				max size in blocks supported by SCSI prot  - offsetBlocks
	writeBlocks = static_cast<TUint16>((KMaxBufSize / KDefaultBlockSize) - offsetBlocks);
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), ETrue);
	// complete read
	TInt err = KErrCompletion;
	do
		{
		err = scsiTest->iScsiProt->ReadComplete(KErrNone);
		}
	while(err == KErrCompletion);

	TEST_FOR_VALUE(err, KErrNone);
	{
	TPtrC8 transportBuf (scsiTest->iTransport.iBufRead.Left(writeBlocks * KDefaultBlockSize));
	TPtrC8 driveBuf(TPtrC8(scsiTest->iProxyDrive.iMediaBuf).Mid(offsetBlocks * KDefaultBlockSize, writeBlocks * KDefaultBlockSize));
	// data should be identical
	TEST_FOR_VALUE(driveBuf.Compare(transportBuf), 0);
	}
	 __PRINT(_L("Check for max write ===> passed OK\n"));


	// CASE: boundary conditions
	offsetBlocks = 0xFFFFFFFF;
	writeBlocks = 0xFFFF;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Check for boundary conditions ===> passed OK\n"));

	
	// CASE: try WrProtect
	// dev note: scsi protocol doesn't support any value except 000b
	cmd.SetProtect (1);
	offsetBlocks = 1;
	writeBlocks = 1;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT(_L("Check for WR Protect ===> passed OK\n"));


	// CASE: invalid CAPs
	cmd.SetProtect (0);
	offsetBlocks = 1;
	writeBlocks = 1;
	cmd.SetTransferLength (writeBlocks);
	cmd.SetBlockAddress (offsetBlocks);
	// create error conditions in drive
	TInt error = KErrNone;
	CMassStorageDrive* pDrive = scsiTest->iDriveManager->Drive (CScsiTest::TestLun, error);
	ASSERT (KErrNone == error && pDrive);
	delete pDrive->iLocalDrive;
	pDrive->iLocalDrive = NULL;
	// ATTENTION: iLocalDrive is set to NULL
	
	TEST_FOR_VALUE (scsiTest->DecodePacket (pWrite10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
	 __PRINT(_L("Check for invalid caps ===> passed OK\n"));

	// CASE: drive->Write() with errors in CScsiProtocol::ReadComplete()
	// note: this case is trivial => no test case is required

	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT(_L("t_scsi_write ===> passed OK\n"));
	}
	
	
/**
t_scsi_verify case
*/
GLDEF_C void t_scsi_verify()
	{
	test.Next (_L("t_scsi_verify\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	// initialize Read/Write buffers
	// I need identical initialization here because of specifics of a test
	scsiTest->iProxyDrive.iMediaBuf.FillZ(KMaxBufSize * 2);
	scsiTest->iTransport.iBufRead.FillZ(KMaxBufSize * 2);
	// initialize 1/2 a buffer for testing
	for (TInt i = 0; i<(TInt)KMaxBufSize; i++)
		{
		scsiTest->iTransport.iBufRead[i] = scsiTest->iProxyDrive.iMediaBuf[i] = static_cast<TUint8>(i%0x10);
		}

	
	// SBC-2 p.68
	// dev notes: latest implementation of scsi protocol supports only:
	//			- Logical block address
	//			- Transfer length
	//			- VRPROTECT (only true or false)
	//			- BYTCHK
	//     all other fields are not supported
	TReadWrite10Cmd cmd;
	cmd.SetVerify(); 		// setup cmd with correct header
	cmd.SetBytChk (ETrue);	// set BYTCHK
	TPtrC8 pVerify10Cmd(cmd.iCmd);
	
	TUint16 maxNumBlocks = static_cast<TUint16>(scsiTest->iProxyDrive.iMediaBuf.Length() / KDefaultBlockSize);
	TUint16 verifyBlocks = 1;
	TUint32 offsetBlocks = 0;

	// CASE: register drive; nothing to verify
	scsiTest->MountTestDrive();
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	 __PRINT(_L("Request to verify 0 bytes ===> passed OK\n"));


	// CASE: verify min (1 block) with offset = 0
	// NOTE: buffers are identical as per initialisation at the beginning
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(0);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	// complete cmd
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrNone), KErrNone);
	 __PRINT(_L("Request to verify 1 block @ offset 0 ===> passed OK\n"));
	
	
	// CASE: verify after the end
	verifyBlocks = 10;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress (maxNumBlocks - verifyBlocks/2);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to verify AFTER the end of a media ===> passed OK\n"));
	

	// CASE: verify from after the end
	verifyBlocks = 10;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(maxNumBlocks + verifyBlocks/2);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Request to start verify AFTER the end of a media ===> passed OK\n"));
	
	
	// CASE: check for internal buffer overflow (with offset)
	// dev note: this check is specific to SCSI prot implementation
	// 			 and NOT described by SBC-2 
	offsetBlocks = 1;
	verifyBlocks = static_cast<TUint16>(maxNumBlocks - offsetBlocks);
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT(_L("Check for internal buffer overflow ===> passed OK\n"));

	
	// CASE: verify max with offset (= offsetBlocks block)
	offsetBlocks = 2;
	//				max size in blocks supported by SCSI prot  - offsetBlocks
	verifyBlocks = static_cast<TUint16>((KMaxBufSize / KDefaultBlockSize) - offsetBlocks);
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	// complete read
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrNone), KErrNone);
	 __PRINT(_L("Check for max verify ===> passed OK\n"));


	// CASE: boundary conditions
	offsetBlocks = 0xFFFFFFFF;
	verifyBlocks = 0xFFFF;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
	 __PRINT(_L("Check for boundary conditions ===> passed OK\n"));

	
	// CASE: VrProtect
	// dev note: scsi protocol doesn't support any value except 000b
	cmd.SetProtect (1);
	offsetBlocks = 1;
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(offsetBlocks);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	// restore protect
	cmd.SetProtect (0);
	 __PRINT(_L("Check for VR Protect ===> passed OK\n"));


	// CASE: BYTCHK
	cmd.SetProtect (0);
	cmd.SetBytChk (EFalse);
	offsetBlocks = 1;
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress (offsetBlocks);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	// restore bytchk
	cmd.SetBytChk (ETrue);
	 __PRINT(_L("Check for BYTCHK ===> passed OK\n"));


	// CASE: different read/write buffers => has to fail in Compare()
	scsiTest->iTransport.iBufRead[2] = 0xFF;
	scsiTest->iProxyDrive.iMediaBuf[2] = 0x0F;
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress (0);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	// complete cmd
	TEST_FOR_VALUE (scsiTest->iScsiProt->ReadComplete(KErrNone), KErrAbort);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EMisCompare, 0);
	 __PRINT (_L("Compare different read/write buffers ===> passed OK\n"));

#ifdef _DEBUG
	// CASE: out of memory check
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress(0);
	__UHEAP_FAILNEXT(1);
	TEST_FOR_VALUE(scsiTest->DecodePacket (pVerify10Cmd), ETrue);
	// complete cmd
	TEST_FOR_VALUE(scsiTest->iScsiProt->ReadComplete(KErrNone), KErrAbort);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EAbortedCommand, TSenseInfo::EInsufficientRes);
	__PRINT (_L("Out of memory check ===> passed OK\n"));
#endif

	// CASE: invalid CAPs
	// 	Attention: This case changes internal state of a drive.
	cmd.SetProtect (0);
	offsetBlocks = 1;
	verifyBlocks = 1;
	cmd.SetTransferLength (verifyBlocks);
	cmd.SetBlockAddress (offsetBlocks);
	// create error conditions in drive
	TInt error = KErrNone;
	CMassStorageDrive* pDrive = scsiTest->iDriveManager->Drive (CScsiTest::TestLun, error);
	ASSERT (KErrNone == error && pDrive);
	delete pDrive->iLocalDrive;
	pDrive->iLocalDrive = NULL;
	// ATTENTION: iLocalDrive is set to NULL
	TEST_FOR_VALUE (scsiTest->DecodePacket (pVerify10Cmd), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
	 __PRINT (_L("Check for invalid caps ===> passed OK\n"));

	
	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT (_L("t_scsi_verify ===> passed OK\n"));
	}
	

/**
t_scsi_mode_sense case
*/
GLDEF_C void t_scsi_mode_sense()
	{
	test.Next (_L("t_scsi_mode_sense\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// SPC-2 p.100
	// dev notes: latest implementation of scsi protocol supports only:
	// 		- PC field
	//			- 00b	| supported 
	//			- 10b	| in the same
	//			- 11b	| manner
	//			- 01b - NOT supported
	//		- PAGE CODE (p. 189)
	//			- 3Fh code ONLY
	//
	//  OUTPUT:
	//		- device-specific parameter, field WP only (table 147, p.189 and table 101, p.107 in SBC-2)
	// 		NOT supported:
	//		- block descriptors are not supported (table 146)
	//		- pages
	
	TModeSenseCmd cmd;
	TPtrC8 pCmdModeSense (cmd.iCmd);
	// expected response length is 16 bytes
	TBuf8<4> response;
	response.FillZ(4);
	scsiTest->MountTestDrive();
	
	// CASE: PC = 0; page code = 3Fh, media is Write protected
	cmd.SetPC (0x00);
	cmd.SetPageCode (0x3F);
	scsiTest->iProxyDrive.iCaps.iMediaAtt = KMediaAttWriteProtected;
	TEST_FOR_VALUE (scsiTest->DecodePacket(pCmdModeSense), ETrue);
	response = scsiTest->iTransport.iBufWrite;
	// response has to have header only
	TEST_FOR_VALUE (response[0],		3);		// length of the response - 1, (header only = 4) (see p.190)
	TEST_FOR_VALUE (response[1],		0); 	// medium type
	TEST_FOR_VALUE (response[2],		0x80); 	// device-specific parameter WP flag has to be set to 1, p107 in SBC-2
	TEST_FOR_VALUE (response[3],		0); 	// block descriptor length
	 __PRINT (_L("CASE: Code page 3Fh ===> passed OK\n"));

	// CASE: PC = 10b; page code = 3Fh, media is Write protected
	cmd.SetPC (0x02);	// default values
	cmd.SetPageCode (0x3F);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pCmdModeSense), ETrue);
	// default response has to have 00h in all fields
	TBuf8<4> responseDefault;
	responseDefault.FillZ(4);
	responseDefault[0] = 3;	// size of a response
	TEST_FOR_VALUE (scsiTest->iTransport.iBufWrite.Compare(responseDefault), 0); 
	 __PRINT (_L("CASE: Page control 10b ===> passed OK\n"));
	// CASE: PC = 10b; page code = 3Fh; media is NOT registered
	// 			as per SBC-2 p.102: for PC = 10b:
	//			"default values should be accessible even if the device is not ready"
	scsiTest->DismountTestDrive();
	TEST_FOR_VALUE (scsiTest->DecodePacket(pCmdModeSense), ETrue);
	// default response has to have 00h in all fields
	TEST_FOR_VALUE (scsiTest->iTransport.iBufWrite.Compare(responseDefault), 0); 
	// restore drive 
	scsiTest->MountTestDrive();
	 __PRINT (_L("CASE: Page control 10b ===> passed OK\n"));


	// CASE: PC = 11b; page code = 3Fh, media is Write protected
	cmd.SetPC (0x03); // saved values
	cmd.SetPageCode (0x3F);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pCmdModeSense), ETrue);
	// response has to be identical to the original response
	TEST_FOR_VALUE (scsiTest->iTransport.iBufWrite.Compare(response), 0); 
	 __PRINT (_L("CASE: Page control 11b ===> passed OK\n"));


	// CASE: PC = 0; page code = 3Fh, media is NOT Write protected
	scsiTest->iProxyDrive.iCaps.iMediaAtt = 0; // reset a flag
	cmd.SetPC (0x00);
	cmd.SetPageCode (0x3F);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pCmdModeSense), ETrue);
	TEST_FOR_VALUE (scsiTest->iTransport.iBufWrite[2], 0);	// WP is set to 0
	 __PRINT (_L("CASE: Write not protected ===> passed OK\n"));


	// CASE: PC = 0; page code = 0Ah (not supported)
	cmd.SetPC (0x00);
	cmd.SetPageCode (0x0A);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pCmdModeSense), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT (_L("CASE: unsupported page code ===> passed OK\n"));

	
	// CASE: PC = 01b (not supported); page code = 3Fh 
	cmd.SetPC (0x01);	// changable values
	cmd.SetPageCode (0x3F);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pCmdModeSense), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	 __PRINT (_L("CASE: unsupported page control ===> passed OK\n"));
	
		
	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT (_L("t_scsi_mode_sense ===> passed OK\n"));
	}
	
	
/**
t_scsi_media_change case
*/
GLDEF_C void t_scsi_media_change()
	{
	test.Next (_L("t_scsi_media_change\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// CASE: iMediaRemoved
	scsiTest->MountTestDrive();
	// issue Inquiry cmd
	TInquiryCmd cmdInquiry;
	TPtrC8 pInquiry(cmdInquiry.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pInquiry), ETrue);
	// simulate media changed
	mediaChanged = ETrue;
	// issue TEST UNIT READY
	TTestUnitReadyCmd cmdReady;
	TPtrC8 pReady(cmdReady.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pReady), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EUnitAttention, TSenseInfo::ENotReadyToReadyChange);


	// repeat with prevent allow media removal
	mediaChanged = ETrue;
	TMediaRemovalCmd cmdAllow;
	TPtrC8 pAllow(cmdAllow.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket(pAllow), EFalse);
	TEST_SENSE_CODE_IF_SUPPORTED(scsiTest->GetSenseCodePtr(), TSenseInfo::EUnitAttention,
			TSenseInfo::ENotReadyToReadyChange)
		{
		// yet another test...
		TEST_FOR_VALUE (scsiTest->DecodePacket(pAllow), ETrue);
		}

	// cleanup
	mediaChanged = EFalse;
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT (_L("t_scsi_media_change ===> passed OK\n"));
	}
	
	
/**
t_scsi_read_capacity case
*/
GLDEF_C void t_scsi_read_capacity()
	{
	test.Next (_L("t_scsi_read_capacity\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	
	// SBC-2 p.56
	// dev notes: LOGICAL BLOCK ADDRESS and PMI have to be set to 00h
	//		as a limitation from scsi prot.

	TReadCapacityCmd cmdReadCapacity;
	TPtrC8 pReadCapacity(cmdReadCapacity.iCmd);
	scsiTest->MountTestDrive();
	// CASE: default media settigns
	TEST_FOR_VALUE (scsiTest->DecodePacket (pReadCapacity), ETrue);
	// compare values
	{
	TReadCapacityResponse response (scsiTest->iTransport.iBufWrite);
	TEST_FOR_VALUE (response.Length(),			8);
	TEST_FOR_VALUE (I64HIGH(scsiTest->iProxyDrive.iCaps.MediaSizeInBytes()),	0);
	TEST_FOR_VALUE (response.LBAddress(),		I64LOW(scsiTest->iProxyDrive.iCaps.MediaSizeInBytes()) / response.BlockLength() - 1); // number of blocks - 1
	TEST_FOR_VALUE (response.BlockLength(),		512); // FAT
	}
	 __PRINT (_L("default settings ===> passed OK\n"));
	
	
	// CASE: unsupported LOGICAL BLOCK ADDRESS
	cmdReadCapacity.iCmd[6] = 0x10;	// set some value
	TEST_FOR_VALUE (scsiTest->DecodePacket (pReadCapacity), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	// reset
	cmdReadCapacity.iCmd[6] = 0x00;	// set some value
	__PRINT (_L("unsupported LOGICAL BLOCK ADDRESS ===> passed OK\n"));
	
	
	// CASE: PMI
	cmdReadCapacity.iCmd[9] = 0x01;	// set field
	TEST_FOR_VALUE (scsiTest->DecodePacket (pReadCapacity), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
	// reset
	cmdReadCapacity.iCmd[9] = 0x00;
	__PRINT (_L("unsupported PMI ===> passed OK\n"));
	

	// CASE: Huge size
	scsiTest->iProxyDrive.iCaps.iNumberOfSectors   = 0xF0F0F0F0;
	scsiTest->iProxyDrive.iCaps.iSectorSizeInBytes = 0x200;
	scsiTest->iProxyDrive.iCaps.iNumPagesPerBlock  = 2;
	TEST_FOR_VALUE (scsiTest->DecodePacket (pReadCapacity), ETrue);
	// compare values
	TReadCapacityResponse response (scsiTest->iTransport.iBufWrite);
	TEST_FOR_VALUE (response.Length(),			8);
	TEST_FOR_VALUE (response.LBAddress(),		0xFFFFFFFF);
	TEST_FOR_VALUE (response.BlockLength(),		512);
	// reset
	scsiTest->iProxyDrive.iCaps.iNumberOfSectors = 0;
	 __PRINT (_L("huge media size ===> passed OK\n"));

	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	 __PRINT (_L("t_scsi_read_capacity ===> passed OK\n"));
	}
	
	
/**
t_scsi_bad_lun case
 Issue ALL supported SCSI commsnds with unsupported LUN
*/
GLDEF_C void t_scsi_bad_lun()
	{
	test.Next (_L("t_scsi_bad_lun\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	// mount a drive on different lun
	// all DecodePacket requests will go on TestLun
	scsiTest->MountTestDrive ();
	// set test lun to unsupported LUN
	// note: CScsiTest::TestLun has to be restored at the end of a test
	CScsiTest::TestLun = 10;
	
	
	// CASE: Inquiry
	// not applicable here


	// CASE: Request sense
	// not applicable


	// CASE: Mode sense
	TModeSenseCmd cmdModeSense;
	cmdModeSense.SetPC (0x00);
	cmdModeSense.SetPageCode (0x3F);
	TPtrC8 pModeSense(cmdModeSense.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pModeSense), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);

	
	// CASE: Read capacity
	TReadCapacityCmd cmdReadCapacity;
	TPtrC8 pReadCapacity(cmdReadCapacity.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pReadCapacity), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);

	
	// CASE: Start stop unit
	// not applicable here


	// CASE: Prevent allow media removal
	TMediaRemovalCmd cmdMediaRemoval;
	TPtrC8 pMediaRemoval (cmdMediaRemoval.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pMediaRemoval), EFalse);
	TEST_SENSE_CODE_IF_SUPPORTED(scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported) {}

	
	// CASE: Read
	TReadWrite10Cmd cmdRead;
	cmdRead.SetRead();
	cmdRead.SetTransferLength(2);
	TPtrC8 pRead (cmdRead.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pRead), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);

	
	// CASE: Write
	TReadWrite10Cmd cmdWrite;
	cmdWrite.SetWrite();
	cmdWrite.SetTransferLength(2);
	TPtrC8 pWrite (cmdWrite.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pWrite), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);

	
	// CASE: Verify
	TReadWrite10Cmd cmdVerify;
	cmdVerify.SetVerify();
	cmdVerify.SetTransferLength(2);
	TPtrC8 pVerify (cmdVerify.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pVerify), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
	
	
	// CASE: Test unit ready
	// Dev. note: this case has to be last as it tries to connect the drive
	TTestUnitReadyCmd cmdTestUnitReady;
	TPtrC8 pTestUnitReady(cmdTestUnitReady.iCmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pTestUnitReady), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
	
	
	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	__PRINT (_L("t_scsi_bad_lun ===> passed OK\n"));
	// restore
	CScsiTest::TestLun = 1;
	}
	
/**
t_scsi_unsupported_commands
 Issue several unsupported SCSI commsnds
*/
GLDEF_C void t_scsi_unsupported_commands()
	{
	test.Next (_L("t_scsi_unsupported_commands\n"));
	CScsiTest* scsiTest = CScsiTest::NewLC();
	scsiTest->MountTestDrive ();

	
	// CASE: format unit cmd	
	TBuf8<7> cmd;
	cmd.FillZ(7);
	cmd[0] = 0x06;		// size of a command itself
	cmd[1] = 0x04;		// FORMAT UNIT command
	TPtrC8 pFormat(cmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pFormat), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);

	// CASE: READ(6)
	cmd[1] = 0x08;		// READ(6) command
	TPtrC8 pRead(cmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pRead), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);
	
	// CASE: WRITE(6)
	cmd[1] = 0x0A;		// WRITE(6) command
	TPtrC8 pWrite(cmd);
	TEST_FOR_VALUE (scsiTest->DecodePacket (pWrite), EFalse);
	TEST_SENSE_CODE (scsiTest->GetSenseCodePtr(), TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);

	// cleanup
	CleanupStack::PopAndDestroy (1); // scsiTest
	__PRINT (_L("t_scsi_unsupported_commands ===> passed OK\n"));
	}

	
/**
Entry point for all SCSI protocol test case
*/
GLDEF_C void t_scsi_prot()
	{
	
	__UHEAP_MARK;

	t_scsi_inquiry_normal();
	
	t_scsi_inquiry_error();
	
	t_scsi_test_unit_ready();
	
	t_scsi_prevent_allow_media_removal();
	
	t_scsi_start_stop_unit();
	
	t_scsi_request_sense();
	
	t_scsi_read();
	
	t_scsi_write();
	
	t_scsi_verify();
	
	t_scsi_mode_sense();
	
	t_scsi_media_change();
	
	t_scsi_read_capacity();
	
	t_scsi_bad_lun();
	
	t_scsi_unsupported_commands();
	
	__UHEAP_MARKEND;

	__PRINT (_L("t_scsi_prot test completed O.K\n"));
	}


//
// CScsiTest implementation
//
/**
c'tor
*/
CScsiTest::CScsiTest()
	: iDriveManager(NULL),
	  iScsiProt(NULL)
	{}
	
/**
d'tor
*/
CScsiTest::~CScsiTest()
	{
	iDriveMap.Close();
	delete iDriveManager;
	delete iScsiProt;
	}
	
/**
NewLC
@return instance of CScsiTest
*/
CScsiTest* CScsiTest::NewLC()
	{
	CScsiTest* self = new (ELeave) CScsiTest();
	CleanupStack::PushL(self);
	self->ConstructL();
	
	return self;
	}

/**
ConstructL
*/
void CScsiTest::ConstructL()
	{
	__PRINT(_L("CScsiTest::ConstructL\n"));
	
	for(TInt i=0; i<KMaxLuns; i++)
		{
		iDriveMap.AppendL(EDriveA+i);
		}
	
	iDriveManager = CDriveManager::NewL (iDriveMap);
	iScsiProt = CScsiProtocol::NewL(*iDriveManager);
	iScsiProt->RegisterTransport(&iTransport);
	
	TInt err = KErrNone;
	CMassStorageDrive* drive = iDriveManager->Drive(TestLun, err);
	ASSERT(!err && drive);
	
	// let's set proxy drive
	drive->SetMountState(CMassStorageDrive::EDisconnected, NULL);
	}

/**
Requests of Sense data
@return TPtrC8 pointer to SenseData
*/
TPtrC8& CScsiTest::GetSenseCodePtr()
	{
	__PRINT(_L("CScsiTest::GetSenseCodePtr\n"));
	
	TBuf8<7> buf;
	buf.FillZ(7);
	buf[0] = 0x06;	// size of a command itself
	buf[1] = 0x03; 	// inquiry operation code
	// Note: SCSI protocol supprts 0x70 byte response format only
	buf[4] = 0x70;
	buf[5] = 18;   //expected answer's length
	TPtrC8 pBuf(buf);
	TEST_FOR_VALUE(iScsiProt->DecodePacket(pBuf, TestLun), ETrue);

	return iTransport.iBufWrite;
	}


/**
Mounts test drive, which is defined by CScsi::TestLun
*/
void CScsiTest::MountTestDrive ()
	{
	TInt error = 0;
	CMassStorageDrive* pDrive = iDriveManager->Drive (CScsiTest::TestLun, error);
	if(!pDrive || error)
		{
		__PRINT(_L("Error: Can't get test drive\n"));
		test(EFalse);
		return;
		}
	// SCSI prot checks mount and drive states
	
	pDrive->SetMountConnected(iProxyDrive, mediaChanged);
	
	pDrive->SetDriveState(CMassStorageDrive::EIdle);
	}


/**
Mounts test drive, which is defined by CScsi::TestLun
*/
void CScsiTest::DismountTestDrive()
	{
	TInt error = 0;
	CMassStorageDrive* pDrive = iDriveManager->Drive (CScsiTest::TestLun, error);
	if(!pDrive || error)
		{
		__PRINT(_L("Error: Can't get test drive\n"));
		test(EFalse);
		return;
		}
	// SCSI prot checks mount and drive states
	pDrive->SetMountDisconnected();
	pDrive->SetDriveState(CMassStorageDrive::EMediaNotPresent);
	}


/**
@return Pointer to test drive, which is defined by CScsi::TestLun
*/
CMassStorageDrive* CScsiTest::TestDrive()
	{
	TInt error = 0;
	CMassStorageDrive* pDrive = iDriveManager->Drive(TestLun, error);
	if(!pDrive || error)
		{
		__PRINT(_L("Error: Can't get test drive\n"));
		ASSERT(EFalse);
		return NULL;
		}
	return pDrive;
	}
