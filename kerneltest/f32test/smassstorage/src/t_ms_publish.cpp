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
// Component test of Publish and Subscribe
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
#include "scsicmdbuilder.h"
#include "cstatemachine.h"
#include "cpropertywatch.h"


GLREF_D TBuf8<KScsiCmdMaxLen> scsiCmdBuf;
GLREF_D RDevTestUsbcClient usbcClient;

RFs fs;

LOCAL_D TChar driveLetter;
GLDEF_D TInt removalDrvNo;
GLDEF_D TUint8 testLun(0);                // Use MMC card for testing

_LIT(KMsFs, "MassStorageFileSystem");

#define LOG_AND_TEST(a, e) {if (a!=e) {test.Printf(_L("lvalue %d, rvalue%d\n\r"), a,e); test(EFalse);}}


LOCAL_C void ParseCommandArguments()
//
// Parses the command line arguments
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token;
	token.Set(lex.NextToken());
	if (token.Length() != 0)
		{
		driveLetter = token[0];
		driveLetter.UpperCase();
		test.Printf(_L("CmdLine Param=%S"),&token);
		}
	else
		{
		test.Printf(_L("No or not enough command line arguments"));
		// code drive letters based on platform
		TInt uid;
		TInt r=HAL::Get(HAL::EMachineUid,uid);
		LOG_AND_TEST(r,KErrNone);
	
		switch(uid)
   			{
   			case HAL::EMachineUid_Lubbock:
   				driveLetter = 'F';
   				test.Printf(_L("Test is running on Lubbock\r\n"));
				testLun = 2;
				break;
   			case HAL::EMachineUid_Win32Emulator:
   				driveLetter = 'X';
   				test.Printf(_L("Test is running on Win32 Emulator\r\n"));
				testLun = 0;
   				break;
   			default:
				// Assume it's a H2 board for now as no relevant Enum is found
   				driveLetter = 'D';
   				test.Printf(_L("Test is running on H2 board\r\n"));	
   				testLun = 0;				
   				break;
   			}
   		}	
	}

LOCAL_C void doComponentTest()
    //
    // Do the component test
    //
	{
#ifndef __NO_HEAP_CHECK
	__UHEAP_MARK;
#endif	

    TInt ret;
    test.Printf(_L("Start MountStart test. Be sure MMC card is inserted.\n"));
    // Parse the CommandLine argument: removal drive
    ParseCommandArguments();

    // Connect to the server
    LOG_AND_TEST(KErrNone,  fs.Connect());
	CleanupClosePushL(fs); 
	   
    // Convert drive letter to its numerical equivalent
	ret = fs.CharToDrive(driveLetter,removalDrvNo);
	LOG_AND_TEST(ret,  KErrNone);	

	// Load the logical device
	_LIT(KDriverFileName,"TESTUSBC.LDD");
	ret = User::LoadLogicalDevice(KDriverFileName);
	LOG_AND_TEST(KErrNone, ret);

    // Add MS file system
	_LIT(KMsFsFsy, "MSFS.FSY");
	LOG_AND_TEST(KErrNone, fs.AddFileSystem(KMsFsFsy));
	
    // Start Ms file system
    RUsbMassStorage usbMs;
    CleanupClosePushL(usbMs);

    TMassStorageConfig config;
    
    config.iVendorId.Copy(_L("vendorId"));
    config.iProductId.Copy(_L("productId"));
    config.iProductRev.Copy(_L("rev"));

	ret = usbMs.Connect();
    LOG_AND_TEST(KErrNone, ret);
  
    // Start usb mass storage device
    LOG_AND_TEST(KErrNone , usbMs.Start(config));

    // Format removable drive using FAT FS
    RFormat format;
    TBuf<2> removalDrive;
    removalDrive.Append(driveLetter);
    removalDrive.Append(':');
    TInt tracksRemaining;
    test.Printf(_L("Start MMC card formatting\n"));
    LOG_AND_TEST(KErrNone, format.Open(fs, removalDrive, EHighDensity|EQuickFormat, tracksRemaining));
    while (tracksRemaining)
        {
        test.Printf(_L("."));
        LOG_AND_TEST(KErrNone,  format.Next(tracksRemaining));
        }
    format.Close();
  	test.Printf(_L("\nDone!\n"));

    // Open a session to LDD
    test.Printf(_L("Open LDD\n"));
    LOG_AND_TEST(KErrNone, usbcClient.Open(0));

	test.Printf(_L("Creating CActiveScheduler\n"));
	CActiveScheduler* sched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);
  	
	// Create a state machine
	CStateMachine* sm = CStateMachine::NewL();
	CleanupStack::PushL(sm);
    sm->AddState(EUsbMsDriveState_Disconnected);
    sm->AddState(EUsbMsDriveState_Connecting);
    sm->AddState(EUsbMsDriveState_Connected);
    sm->AddState(EUsbMsDriveState_Disconnecting);
    sm->AddState(EUsbMsDriveState_Active);
    sm->AddState(EUsbMsDriveState_Locked);
    sm->AddState(EUsbMsState_Written);
    sm->AddState(EUsbMsState_Read);
    
    sm->SetInitState(EUsbMsDriveState_Disconnected);

  	CPropertyHandler* driveStatusHandler 	= CMsDriveStatusHandler::NewLC(removalDrvNo, *sm);
  	CPropertyHandler* readStatusHandler 	= CMsReadStatusHandler::NewLC(removalDrvNo, *sm);
  	CPropertyHandler* writtenStatusHandler 	= CMsWrittenStatusHandler::NewLC(removalDrvNo, *sm);
  	
 	CPropertyWatch::NewLC(EUsbMsDriveState_DriveStatus, *driveStatusHandler);
 	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesRead, *readStatusHandler);
 	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesWritten, *writtenStatusHandler);
 	
	CActiveScheduler::Start();

	ret = usbMs.Stop();
	test.Printf(_L("usbMs.Stop returned %d\n"), ret);
	test(ret == KErrNone);
	usbMs.Close();
	// 1 sec delay for MSFS to stop
	User::After(1000000);
	ret = fs.RemoveFileSystem(KMsFs);
	test(ret == KErrNone || ret == KErrNotFound);
	test.Printf(_L("RemoveFileSystem returned %d\n"), ret);

	usbcClient.Close();
	ret = User::FreeLogicalDevice(_L("USBC"));
	test.Printf(_L("FreeLogicalDevice returned %d\n"), ret);
	test(ret == KErrNone);			

	CleanupStack::PopAndDestroy(3);	// 3 CPropertyWatches 
	CleanupStack::PopAndDestroy(3);	// 3 property status change handlers
	CleanupStack::PopAndDestroy(sm);
	CleanupStack::PopAndDestroy(sched);
	CleanupStack::PopAndDestroy(&usbMs);
	CleanupStack::PopAndDestroy(&fs);

#ifndef __NO_HEAP_CHECK
	__UHEAP_MARKEND;
#endif
	}
	
GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
	test.Start( _L("Test Publish Subscriber") );
    doComponentTest();
    test.End();
    }

