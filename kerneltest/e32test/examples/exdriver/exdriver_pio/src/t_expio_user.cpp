// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is a test application for tutorial reference driver. The purpose of 
// this application is to illustrate using and calling the driver API. Here 
// this application will load the driver, open and call the driver API to 
// configure the uart, transmit and receive some test data over the uart. 
// All API are Synchronous requests.
// API:
// Symbian Device driver framework API
// Platforms:
// H4,ARMV5 and WISNCW
// Capability:
// CommDD
// Assumptions:
// To run on Emulator(WinsCW)
// 1) 	Loopback connection between two ports (com1 and any other 
// com port is required)
// 2)  HyperTerminal for COM1 is NOT opened
// On H4:
// 1) 	Driver does an internal loopback.	
// 
//

// include files
//
#include <e32test.h>
#include "expio.h"

// Create RTest object (test framework)
//
LOCAL_D RTest test(_L("Tutorial Driver"));

// Litearl string descriptors for LDD and PDD names of the driver
//
_LIT(KExDriverLdd, "d_expio_ldd");	// Ldd name
_LIT(KExDriverPdd, "d_expio_pdd");	// Pdd name

// Rx data request size
const TInt KRxTestSize = 100; // in bytes

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXPIO-0412
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case is testing the PIO serial driver. It tests the Symbian
//! 					driver API and synchronous requests
//! @SYMTestActions     1.	Load the driver, d_expio.ldd and d_expio.pdd
//!						2.	Call the device API and validate
//!						3.	Open the channel
//!						4.	Configure the device
//!						5.	Transmit data
//!						6.	Receive data
//!						7.	Compare the Tx and Rx data, assuming an internal loopback by driver
//!						8.	Close the channel
//!						9.	Unload the driver
//!
//! @SYMTestExpectedResults 1.	Driver loads successfully
//!								a.	returns KErrNone if not existing already
//!								b.	returns KErrAlreadyExists when the driver is already loaded
//!							2.	Device opens, reads the capabilities and closes successfully.
//!							3.	Opens channel successfully by returning KErrNone
//!							4.	Configures the device successfully, by returning KErrNone
//!							5.	Completes transmitting data successfully by returning KErrNone
//!							6.	Completes receiving data successfully
//!							a.	Returns KErrNone, if received data size is equal to requested data size
//!							b.	Return KErrTimedOut, if received data size is less than the requested data size
//!							7.	Data received is same as data transmitted, with driver functioning in PIO mode
//!							8.	Channel closes successfully, completes and exits without panic
//!							9.	Driver unloads successfully, returns KErrNone
//!							
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

/**
 E32Main - Application, exe main entry point. This is a single thread 
 application that runs sequentially.
 
 @return KErrNone or standard error code
 */ 
GLDEF_C TInt E32Main()
	{
	TInt r;
		
	// Initialize the test object with title	
	test.Title();
	
	test.Printf(_L("\nPhase-I: Example Application for Programmed I/O driver\n"));
	
#ifdef __WINSCW__	
	test.Printf(_L("\nUSAGE::\n(1)PLEASE ENSURE COM1 IS NOT USED BY ANY OTHER APPLICATION\n"));
	test.Printf(_L("(2)CONNECT/LOOPBACK COM1 TO ANOTHER PORT, SAY COM2 USING SERIAL CABLE\n"));
	test.Printf(_L("(3)OPEN HYPERTERMINAL FOR LATER PORT, SAY COM2\n"));
	test.Printf(_L("(4)PLEASE FOLLOW THE INSTRUCTIONS DISPLAYED ON EMULATOR SCREEN\n"));
	test.Printf(_L("\nHIT A KEY TO CONTINUE\n"));
	test.Getch();
#endif
		
	// Test set 1 - Load PDD, [RTest::Next() starts new set of tests]	
 	test.Start(_L("Load Physical Device"));

	// Load the PDD. This user API will load the PDD dll by name and
	// also enable the loader to search for the PDD object by name.
	r = User::LoadPhysicalDevice(KExDriverPdd);

	// PDD loading is considered successful, if either it is loaded now
	// or it is already loaded and is existing
 	test((r==KErrNone)||(r==KErrAlreadyExists));
 	
	// Test set 2 - Load the LDD
 	test.Next(_L("Load Logical Device"));

	// Load the LDD. This user API will load the LDD dll by name and
	// also enable the loader to search for the LDD object by name.
 	r = User::LoadLogicalDevice(KExDriverLdd);

	// LDD loading is considered successful, if either it is loaded now
	// or it is already loaded and is existing
 	test((r==KErrNone)||(r==KErrAlreadyExists));

	// Test set 3 - Get the device capabilities from driver
	test.Next(_L("Get Device Capabilities")); 

	// RDevice is the user side handle of LDD factory object.
 	RDevice device;

	// Open the device with reference to the driver name.
 	r = device.Open(KDriverName);
 	if (r==KErrNone)
 		{
		// Package the device capabilities structure to a descriptor
		// suitable to be passed to RDevice::GetCaps()
 		TPckgBuf<TCapsDevCommV01> caps;

		// Get the device capabilities. Driver fills in the capabilities
		// in caps and returns to user.
 		device.GetCaps(caps); 	
	
		// Retrieve the information from caps and validate name and version
 		TVersion expectedVer(RExDriverChannel::VersionRequired()); 		
 	    test(caps().version.iMajor==expectedVer.iMajor);
    	test(caps().version.iMinor==expectedVer.iMinor);
    	test(caps().version.iBuild==expectedVer.iBuild);

		// Close the device. This handle is required only to get any device 
		// related information from LDD factory object.
 		device.Close();
 		}
 
	// Test set 4 - Driver open functionality
 	test.Next(_L("Open Channel"));
	
	// RExdriver is RBusLogicalChannel derived class, user side handle to 
	// logical channel. It defines the driver interface.
 	RExDriverChannel ldd;

	// open thelogical channel. This is a user-side wrapper
	// function for RBusLogicalChannel::DoCreate() API. Hence DoCreate() is 
	// called in Open().
 	r = ldd.Open();
 	test(r==KErrNone);
 	
	// Test set 5 - Check if the driver can verify and deny access to wrong clients 	
 	test.Next(_L("Check Wrong client access"));
 	
	// Create another user side handle for the logical channel
    RExDriverChannel ldd2=ldd;

	// RBusLogicalChannel::Duplicate() creates a valid handle to the kernel 
	// object for which the specified thread already has a handle. Check with 
	// ownership as process.
	r=ldd2.Duplicate(RThread(),EOwnerProcess);

	// Above API should return KErrAccessDenied error to verify that driver 
	// could deny wrong client access
	test(r==KErrAccessDenied);

	// Test set 6 - Handle duplication
    test.Next(_L("Check handle duplication"));
    ldd2=ldd;    
    
	// Check handle duplication, with ownership as this Thread.
    r=ldd2.Duplicate(RThread(),EOwnerThread);    
    test(r==KErrNone);
    ldd2.Close();

	// Test - Get the channel capabilities
	test.Next(_L("Get the channel capabilities"));
	// Package buffer of TCommCapsV03, which holds the uart capabilities
	TCommCaps3 chCaps;
	// Get the channel's capabilities to be configured	
	r=ldd.Caps(chCaps);
	test(r==KErrNone);
	// Check if 9600 baud rate is supported,(want to configure to this later)
	test(chCaps().iRate&KCapsBps9600);			
	
	// Test set 8 - Configure the device (uart)
 	test.Next(_L("Configure UART"));
 	
 	// Package the comm config structure to descriptor and initialize the data
 	TPckgBuf<TCommConfigV01> config;
 	config().iRate = EBps9600;
 	// Configure the channel
  	r = ldd.SetConfig(config);
 	test(r==KErrNone);
 	 	
	// Test set 9 - Transmit data to the device (uart)
 	test.Next(_L("Send Data over UART"));
 	
 	// Create a constant descriptor with test data
 	_LIT8(KTestSendData,"<< TEST DATA FOR TUTORIAL DRIVER EXAMPLE >>");
 	
 	// Call ldd interface TransmitData() API test data descriptor as parameter
 	r = ldd.TransmitData(KTestSendData);
 	test(r==KErrNone);
 		
#ifdef __WINSCW__
	test.Printf(_L("DATA TRANSIMITTED CAN BE VIEWED ON HYPERTERMINAL OPENED FOR SECOND PORT\n\n")); 
#endif	

 	// Test set 10 - Receive data from the device (uart)
 	test.Next(_L("Receive Data over UART"));

#ifdef __WINSCW__
	// Incase of executing this application on emulator, user need to enter
	// data in the hyperterminal for the port to which the default port com1 is
	// looped back externally through serial cable. Then press any key on the 
	// emulator screen.
	// As e.g say com1 is connected to com5, then open hyperterminal for com5
	// and send data through com5 hyperterminal to com1.
 	test.Printf(_L("TO RECEIVE DATA, PLEASE TYPE SOME DATA ON HYPERTERMINAL OPENED FOR SECOND PORT")); 	
 	test.Printf(_L("HIT A KEY TO CONTINUE\n")); 	
 	test.Getch();
#endif
 	
	// Create a buffer that has to be filled and returned by the driver	
 	TBuf8<KRxTestSize> RxBuf;
 	
 	// Call ldd interface ReceiveData() API to get data to RxBuf
 	r = ldd.ReceiveData(RxBuf);
 	test(r==KErrNone);
 	 	
 	// Print the recieve data to display and verify the data received manually.
 	TInt i;
 	test.Printf(_L("Received Data of size (%d):"),RxBuf.Size());
 	for (i=0;i<RxBuf.Size();i++)
 	test.Printf(_L("%c"),RxBuf[i]);
 	test.Printf(_L("\n"));

#ifndef __WINSCW__
	// If the transmit data size and Rx data request size are same,
	// then compare the data.
	// 
	if (sizeof(KTestSendData)==KRxTestSize)
		{
 		// Compare the data received with data sent
 		r=RxBuf.Compare(KTestSendData);
 		test(r==KErrNone); 		
		}
#endif
 	
 	// Test set 11 - Close the logical channel
  	test.Next(_L("Close Channel"));
	ldd.Close();
	
	// Test set 12 - Free the logical device / ldd
	test.Next(_L("Free Logical Device"));
 	r=User::FreeLogicalDevice(KDriverName);
 	test(r==KErrNone);
 	
 	// Test set 13 - Free the physical device / pdd
 	test.Next(_L("Free Physical Device")); 	
 	
 	// Instead of directly using the pdd name, get the PDD factory object name
 	// and append with extension name, to unload the PDD.
	TName pddName(KDriverName);
	_LIT(KVariantExtension,".pdd");
    pddName.Append(KVariantExtension); 
    	
    // Free the PDD, resulting in freeing pdd factory object   
 	r=User::FreePhysicalDevice(pddName);
 	test(r==KErrNone);
 	
 #ifdef __WINSCW__	
 	test.Next(_L("\nHIT ANY KEY TO EXIT THE APPLICATION"));
 	test.Getch();
#endif	

 	// End the tests 	
 	test.End();
 	
 	// Frees the resources used for RTest
 	test.Close();

	//return the result 
 	return r;
	}
//
//  End of t_expio_user.cpp

