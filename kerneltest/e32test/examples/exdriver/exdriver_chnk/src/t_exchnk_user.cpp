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
// This is a test application for tutorial reference driver. This purpose of 
// this application is to illustrate using and calling the driver API. Here 
// this application will load the driver, open and call the driver API to 
// configure the uart, transmit and receive some test data over the uart. 
// This application can be built and run for armv5 platform
// This application tests the driver for its asynchronous request handling and
// and the Tx,Rx functionality with driver functioning in interrupt mode and
// using DFCs
// API:
// Symbian Device driver framework API
// Platforms:
// H4,ARMV5
// Capability:
// CommDD
// Assumptions:
// 1) Tests On COM0,KUnit1 uses internal loopback and external loopback
// External loopback is used on same port COM0 of H4
// 2) Changes the debugport to 3 and reverts back
// 3) Uart is configured to run at 9600bps baudrate
// 3) If driver enables debug messages, the timeout values used here
// will have to be increased 		   
// 
//

// include files
//
#include <e32test.h>
#include "exchnk.h"
#include <hal.h>				// for HAL API
#include <hal_data.h>			// for HAL attributes
#include "t_exchnk_testdata.h" 	// test data

// Literal string descriptors for LDD and PDD names of the driver
//
_LIT(KExDriverLdd, "d_exchnk_ldd");	// Ldd name
_LIT(KExDriverPdd, "d_exchnk_pdd");	// Pdd name

// Constants
//
// Use debug port 3 to output data. This is done as we shall be testing
// this application for UART1 and UART2. To avoid clashing of the tutorial
// driver with default driver, we want the default driver use UART3
//
const TInt KDebugPort=3;
const TInt KUnit1=1;			// Unit number for UART1
const TInt KUnit2=2;			// Unit number for UART2
const TInt KUnit3=3;			// Invalid unit number, UART3

// 40 sec, interms of microseconds. This is more, as large buffers
// of data is being used. However, this timeout value is only used
// to provide an exit method from wait on asynch requests in some
// error conditions.
//
const TUint KTimeOutTxRx=40000000;

/**
 Class for grouping the test cases
 */
class TestExDriver
	{
public:
	// Constructor
	TestExDriver();
	// Destructor
	~TestExDriver();
	/*
	 Called from E32Main()
	 */
	 // Load the driver
	void TestLoadDriver();
	// Unload the driver
	void TestUnloadDriver();
	// Test the logical device API of the driver
	void TestDevice();
	// Test the driver for channels and units support
	void TestChannelsAndUnits();
	// Test synchronous requests - caps, configure, loopback
	void TestSynchRequests();
	// Test asynchronous requests, cancelling, concurrent requests etc
	void TestAsynchRequests();
	// Test data flow Tx&Rx on multiple units using loopback
	void TestDataflowMultipleUnitsLoopback();
	// Change the debug port (required for H4)
	TInt ChangeDebugPort(TInt aPort);
private:
	// Note:: These are not independent tests. Their preconditions
	// have to be satisfied, like loading the driver
	//
	/*
	 Channel & Unit Tests
	 */
	// Test channel on a single valid unit, invalid unit
	void TestChannelOnMultipleUnits();
	// Test multiple channels on a unit
	void TestMultipleChannelsOnUnit();
	// Test dulication of channel handle
	void TestHandleDuplication();
	// Test multi thread access
	void TestMultipleThreadAccess();
	/*
	 Synchronous Tests
	 */
	// Test channel capabilities
	void TestChannelCaps();
	// Configure the device
	void TestConfigure();
	/*
	 Asynchronous Tests
	 */
	// Test Transmit Data, complete
	void TestTransmit();
	// Test cancel asynchronous request
	void TestCancelRequests();
	// Test multiple same requests
	void TestMultipleSameRequests();
	// Test Tx and Rx duplex mode, concurrent requests in loopback mode
	void TestConcurrentTxRx(TInt aLoopback, const TDesC8& aTxData);
	// Test Tx and Rx duplex mode, concurrent requests in loopback mode with
	// two driver channel. Create Shared chunk in one channel and pass shared chunk
	// to second driver channel.Test Tx and Rx in loopback mode on second channel.
	void TestConcurrentTxRxPassSharedChunk(TInt aLoopback, const TDesC8& aTxData,TInt aTxSize , TInt aRxSize, TInt aTxOffset, TInt aRxOffset);
	/*
	 Data flow tests
	 */
	void TestDataFlow(TInt aUnit, TInt aLoopback);
public:
	RTest iTest;					// RTest reference object
	RTimer iTimer;					// Timer reference object
	RExDriverChannel iLdd;			// Logical channel
	TRequestStatus	iTimeStatus;	// Asynch request object	
	TAny* iHolder;
	};

/**
 E32Main - Application, exe main entry point.

 @return KErrNone
 */
GLDEF_C TInt E32Main()
	{	
	TInt debugport;

	// Create a TestExDriver object
	TestExDriver exDrv;

	// Change the debugport. This function is optional and specific
	// to variant. Incase of H4 variant, default debugport is 1. However
	// since this tutorial driver tests both unit1(port1) and unit2(port2),
	// default port is being changed to port3 to avoid conflict
	//
	debugport=exDrv.ChangeDebugPort(KDebugPort);

	// Print the title of the test
	exDrv.iTest.Title();

	// Start the test sets
	exDrv.iTest.Start(_L("Tutorial Driver:Phase-IV:A"));

	// Load the driver
	exDrv.TestLoadDriver();

	// Test the device functionality
	exDrv.TestDevice();

	// Test Channels and units
	exDrv.TestChannelsAndUnits();

	// Test Synchronous requests
	exDrv.TestSynchRequests();

	// Test Asynchronous Requests, Cancel and handle Concurrent Requests
	exDrv.TestAsynchRequests();

	// Test Dataflow on multiple units with loopback
	exDrv.TestDataflowMultipleUnitsLoopback();

 	// Unload the driver
	exDrv.TestUnloadDriver();

	// If debugport is changed then, revert it back to the default debug port
 	if (debugport>=0)
 		{
 		exDrv.iTest.Printf(_L("\nReverting the debugport to Port (%d)\n"),debugport); 		
 		// Revert debugport?
 		exDrv.ChangeDebugPort(debugport);
 		}

	// return success
 	return KErrNone;
	}


/*
 TestExDriver functions
 */

/**
 Constructor
 */
TestExDriver::TestExDriver()
// Initialize RTest with title string
:iTest(_L("Tutorial Driver:Phase-IV:A"))
	{
	}

/**
 Destructor
 */
TestExDriver::~TestExDriver()
	{
	// Ends the current sets of test
	iTest.End();
	// Closes the console and frees any resources acquired
	iTest.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0636
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver (LDD and PDD) installing
//! @SYMTestActions     1.	Load the PDD by name, d_exchnk.pdd
//! 					2.	Load the LDD by name, d_exchnk.ldd
//!
//! @SYMTestExpectedResults 1.	PDD loads successfully
//! 							a.	returns KErrNone if not existing already
//! 							b.	return KErrAlreadyExists when the driver is already loaded.
//! 						2.	LDD loads successfully
//! 							a.	returns KErrNone if not existing already
//! 							b.	return KErrAlreadyExists when the driver is already loaded.
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Load the driver, LDD and PDD and verify if successful
 */
void TestExDriver::TestLoadDriver()
	{
	TInt r;

	// [RTest::Next() starts new set of tests]
	iTest.Next(_L("Test driver Loading"));

	// Load the PDD
 	iTest.Printf(_L("Load Physical Device\n"));

	// Load the PDD. User API will load the PDD dll by name and
	// also enable the loader to search for the PDD object by name.
	//
 	r = User::LoadPhysicalDevice(KExDriverPdd);

	// PDD loading is considered successful, if either it is loaded now
	// or it is already loaded and is existing
	//
 	iTest((r==KErrNone)||(r==KErrAlreadyExists));

	// Load the LDD
 	iTest.Printf(_L("Load Logical Device\n"));

	// Load the LDD. User API will load the LDD dll by name and
	// also enable the loader to search for the LDD object by name.
	//
 	r = User::LoadLogicalDevice(KExDriverLdd);

	// LDD loading is considered successful, if either it is loaded now
	// or it is already loaded and is existing
 	iTest((r==KErrNone)||(r==KErrAlreadyExists));
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0637
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver (LDD and PDD) un-installing
//! @SYMTestActions     1.	Un-Load the PDD by object name
//! 					2.	Un-Load the LDD by object name
//!
//! @SYMTestExpectedResults 1.	PDD un-loads successfully, returns KErrNone
//! 						2.	LDD unloads successfully, returns KErrNone
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Unload the driver, LDD and PDD and verify if successful
 @pre TestLoadDriver() called
 */
void TestExDriver::TestUnloadDriver()
	{
	TInt r;

	iTest.Next(_L("Test driver Un-Loading"));

	// Free the logical device / ldd
	iTest.Printf(_L("Free Logical Device\n"));
 	r=User::FreeLogicalDevice(KDriverName);
 	iTest(r==KErrNone);

 	// Free the physical device / pdd
 	iTest.Printf(_L("Free Physical Device\n"));

 	// Instead of directly using the pdd name, get the PDD factory object name
 	// and append with extension name, to unload the PDD.
 	//
	TName pddName(KDriverName);
	_LIT(KVariantExtension,".pdd");
    pddName.Append(KVariantExtension);

    // Free the PDD, resulting in freeing pdd factory object. Name passed here
    // should match with the one used by driver to register itself in DoCreate().
    //
 	r=User::FreePhysicalDevice(pddName);
 	iTest(r==KErrNone);
	}

/*
 Testing Logical device functionality/API
 */

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0638
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes logical device API/functionality
//! @SYMTestActions     1.	Open the device
//! 					2.	Get the device capabilities
//! 					3.	Close the device
//!
//! @SYMTestExpectedResults 1.	Logical Device is opened successfully
//! 						2.	Reads the device capabilities
//! 						3.	Closes the logical device
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Tests the operations on the logical device
	- Opens the device
	- Reads the device capabilities
	- Closes the device
 @pre 	TestLoadDriver() called
 */
void TestExDriver::TestDevice()
	{
	iTest.Next(_L("Testing Logical Device API"));

	TInt r;

	// RDevice is the user side handle of LDD factory object.
 	RDevice device;

	iTest.Printf(_L("Open the Device\n"));
	// Open the device with reference to the driver name.
 	r = device.Open(KDriverName);
 	if (r==KErrNone)
 		{
 		iTest.Printf(_L("Get Device Capabilities\n"));
		// Package the device capabilities structure to a descriptor
		// suitable to be passed to RDevice::GetCaps()
		//
 		TPckgBuf<TCapsDevCommV01> caps;

		// Get the device capabilities. Driver fills in the capabilities
		// in caps and returns to user.
		//
 		device.GetCaps(caps);

		// Retrieve the information from caps and validate name and version
 		TVersion expectedVer(RExDriverChannel::VersionRequired());
 	    iTest(caps().version.iMajor==expectedVer.iMajor);
    	iTest(caps().version.iMinor==expectedVer.iMinor);
    	iTest(caps().version.iBuild==expectedVer.iBuild);

		iTest.Printf(_L("Close the device\n"));
		// Close the device. This handle is required only to get any device
		// related information from LDD factory object.
		//
 		device.Close();
 		}
	}

/**
 Test the driver for various scenarios of using channels and units
 @pre	TestLoadDriver() called
 */
void TestExDriver::TestChannelsAndUnits()
	{
	iTest.Next(_L("Testing channel access on multiple units"));
	// Test channel on a single valid unit, invalid unit
	TestChannelOnMultipleUnits();

	iTest.Next(_L("Testing multiple channel access on units"));
	// Test multiple channels on a unit
	TestMultipleChannelsOnUnit();

	iTest.Next(_L("Testing multiple client access"));
	// Test dulication of channel handle
	TestHandleDuplication();

	iTest.Next(_L("Testing multiple thread access"));
	// Test multi thread access
	TestMultipleThreadAccess();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0639
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver support for channels on multiple units
//! @SYMTestActions     1.	Open channel on unit1, valid unit
//! 					2.	Open channel on unit2, valid unit
//! 					3.	Open channel on unit3, invalid unit
//!
//! @SYMTestExpectedResults 1.	Open on unit1 returns KErrNone, success case
//! 						2.	Open on unit2 returns KErrNone, success case
//! 						3.	Open on unit3 returns KErrNotSupported, success case
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test opening channel on multiple units
 @pre	TestLoadDriver() called
 */
void TestExDriver::TestChannelOnMultipleUnits()
	{
	TInt r;

 	iTest.Printf(_L("Open Channel on Unit-1\n"));
	// Open thelogical channel, driver for unit 1. This is a user-side wrapper
	// function for RBusLogicalChannel::DoCreate() API. Hence DoCreate() is
	// called in Open() for unit 1.
	//
 	r=iLdd.Open(KUnit1);
 	iTest(r==KErrNone);
 	// Close the channel opened
 	iLdd.Close();

 	iTest.Printf(_L("Open Channel on Unit-2\n"));
 	// Valid unit, hence return success
 	r=iLdd.Open(KUnit2);
 	iTest(r==KErrNone);
 	// Close the channel opened
 	iLdd.Close();

 	iTest.Printf(_L("Open Channel on Unit-3\n"));
 	// Invalid unit, hence return error
 	r=iLdd.Open(KUnit3);
 	iTest(r==KErrNotSupported);
 	iTest.Printf(_L("Open Channel on Unit-3: Failed as Expected\n"));
 	// Channel opened fail, hence no need to close
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0640
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver support for multiple channels on a unit 
//! @SYMTestActions     1.	Open channel-1 on unit1
//! 					2.	Open channel-2 on unit1
//! 					3.	Open channel-1 on unit2
//! 					4.	Close Channel-1 on unit1
//! 					5.	Close Channel-1 on unit2
//!
//! @SYMTestExpectedResults 1.	Open channel-1 on unit1 returns KErrNone
//! 						2.	Open channel-2 on unit1 returns KErrInUse
//! 						3.	Open channel-1 on unit2 returns KErrNone
//! 						4.	Close completes and exits successfully
//! 						5.	Close completes and exits successfully
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test opening multiple channels on units
 @pre	TestLoadDriver() called
 */
void TestExDriver::TestMultipleChannelsOnUnit()
	{
	TInt r;
	RExDriverChannel ldd1;
	RExDriverChannel ldd2;

	iTest.Printf(_L("Open Channel-1 on Unit1\n"));
	// Open a channel on Unit1
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);
	iTest.Printf(_L("Open Channel-1 on Unit1: Success\n"));

	iTest.Printf(_L("Open Channel-2 on Unit1\n"));
	//Open another channel on Unit1
	r=ldd1.Open(KUnit1);
	iTest(r==KErrInUse);
	iTest.Printf(_L("Open Channel-2 on Unit1: Failed as Expected\n"));

	iTest.Printf(_L("Open Channel-1 on Unit2\n"));
	//Open another channel on Unit1
	r=ldd2.Open(KUnit2);
	iTest(r==KErrNone);
	iTest.Printf(_L("Open Channel-1 on Unit2: Success\n"));

	// Close the channel1 that is opened successfuly
	iTest.Printf(_L("Close Channel-1 on Unit1\n"));
	iLdd.Close();

	// Close the channel on unit2
	iTest.Printf(_L("Close Channel-1 on Unit2\n"));
	ldd2.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0641
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver support for sharing handles 
//! @SYMTestActions     1.	Duplicate handle with EOwnerProcess
//! 					2.	Duplicate handle with  EOwnerThread
//!
//! @SYMTestExpectedResults 1.	Duplicate handle with EOwnerProcess returns KErrAccessDenied, success case
//! 						2.	Duplicate handle with  EOwnerThread returns KErrNone, success case
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Tests if there is a duplication in client access and handle
 @pre	TestLoadDriver() called
 */
void TestExDriver::TestHandleDuplication()
	{
	TInt r;

	// Test set 5 - Check if the driver can verify and deny access to wrong clients
 	iTest.Printf(_L("Check Wrong client access\n"));

	// Open a channel on Unit1
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);

	// Create another user side handle for the logical channel
    RExDriverChannel ldd2=iLdd;

	// RBusLogicalChannel::Duplicate() creates a valid handle to the kernel
	// object for which the specified thread already has a handle. Check with
	// ownership as process.
	//
    r=ldd2.Duplicate(RThread(),EOwnerProcess);

	// Above API should return KErrAccessDenied error to verify that driver
	// could deny wrong client access
    iTest(r==KErrAccessDenied);

	// Test set 6 - Handle duplication
    iTest.Printf(_L("Check handle duplication\n"));
    ldd2=iLdd;

	// Check handle duplication, with ownership as this Thread.
    r=ldd2.Duplicate(RThread(),EOwnerThread);
    iTest(r==KErrNone);
    ldd2.Close();

    iLdd.Close();
	}

/**
 Thread that is created from main application. This gets executed
 once RThread::Resume() is called. Tests to validate the access
 from multiple threads are done here
 */
 static TInt ThreadFunc(TAny* aPtr)
    {
    TInt r;
    RTest test(_L("Test Thread"));

    test.Printf(_L("Test Thread scheduled\n"));

    test.Printf(_L("Open channel from Test thread\n"));
    // Test1: Creating channel from other thread
    RExDriverChannel ldd1;
    r=ldd1.Open(KUnit1);
    test((r==KErrNone)||(r==KErrInUse));
    if (r==KErrInUse)
    	{
		test.Printf(_L("Channel already opened in another thread, hence failed:Expected result\n"));
		}

    // Test2: Using the handle created in other thread
    test.Printf(_L("API access from test thread by using shared handle\n"));
    
  	RExDriverChannel *ldd2=(RExDriverChannel*)aPtr;  		
      
	TCommCaps3 chCaps;	
	r=ldd2->Caps(chCaps);	
	test(r==KErrAccessDenied);
    test.Printf(_L("Access denied from other thread:Expected result\n"));
     	
 	        
    test.Close();
     
  	return KErrNone;
    }

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0642
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver support for multiple thread access 
//! @SYMTestActions     1.	Open channels in different threads at a time 
//! 					2.	Share the handle between threads by passing handle to new thread 
//! 						and access driver API
//!
//! @SYMTestExpectedResults 1.	Open channel in new thread returns KErrInUse, success case
//! 						2.	Driver API access in new thread, using handle from original 
//! 							thread. Access is allowed, success case
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test access from a different thread
 @pre	TestLoadDriver() called
 */
void TestExDriver::TestMultipleThreadAccess()
	{
	TInt r;
	RThread thrd;
    TRequestStatus tS;    
	_LIT(KTestThreadName,"TestThread");
	
	iTest.Printf(_L("Test multiple thread access\n"));

	// Create the iTimer that is relative to the thread
 	r = iTimer.CreateLocal();
 	iTest(r==KErrNone);

 	// Open the channel
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);
	 		
	RChunk chunkRx;
	r=iLdd.GetRxChunkHandle(chunkRx); 	
 	iTest(r==KErrNone);
 			
	// Call receive data from main thread	
	TRequestStatus rxStatus;
 	r = iLdd.ReceiveData(rxStatus,10);
 	iTest(r==KErrNone);
 	iTest(rxStatus.Int()==KRequestPending);
 	
	// Create a new thread, where we can test access to the driver
    iTest (thrd.Create(KTestThreadName,&ThreadFunc,KDefaultStackSize,NULL,this)==KErrNone);

	// Resume() schedules the thread created
    thrd.Resume();
 
    // Trigger iTimer expiry after KTimeOut. The status should be pending
	iTimer.After(iTimeStatus,KTimeOutTxRx);
	iTest(iTimeStatus==KRequestPending);
	
 	// Wait on Rx complete. This completes once the Tx from test thread completes
 	User::WaitForRequest(rxStatus);
 	iTest((rxStatus==KErrNone)||(rxStatus==KErrTimedOut));
 	 
    // Incase thread is not scheduled and timeout occurs
    iTest(iTimeStatus!=KErrNone);
    
    // Cancel the timer and close
    iTimer.Cancel();
    iTimer.Close();

    // Close the thread created
    thrd.Close();
    // Close the channel opened
    iLdd.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0643
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes all the synchrnous requests supported by the driver 
//! @SYMTestActions     Call all synchronous API of the driver:
//! 						1.	Get the channel capabilities
//! 						2.	Configure the device
//! 						3.	Set internal loopback
//!
//! @SYMTestExpectedResults 1.	Get channel capabilities reads the channel capabilities to user buffer, success case
//! 						2.	SetConfig() API returns KErrNone
//! 						3.	SetIntLoopback() returns KErrNone
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test Synchronous requests
 	- 1) Get Channel capabilities
 	- 2) Set Configuration
 	- 3) Set loopback
 @pre TestLoadDriver() called
 */
void TestExDriver::TestSynchRequests()
	{
	TInt r;

	iTest.Next(_L("Test Synchronous Requests"));

	// Open Channel
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);

	// Test channel capabilities
	TestChannelCaps();

	// Configure the device
	TestConfigure();

	// Set the device's internal loopback mode, i.e enable or disable
  	r = iLdd.SetIntLoopback(KIntLoopbackEnable);
 	iTest(r==KErrNone);

	// Close channel
	iLdd.Close();
	}


/**
 Get the channel capabilities and verify against the expected capabilities
 @pre 	LoadDriver() called with its pre conditions
 */
void TestExDriver::TestChannelCaps()
	{
	TInt r;

	// Test - Get the channel capabilities
	iTest.Printf(_L("Get the channel capabilities\n"));

	// Package buffer of TCommCapsV03, which holds the uart capabilities
	TCommCaps3 chCaps;
	// Get the channel's capabilities to be configured
	r=iLdd.Caps(chCaps);
	iTest(r==KErrNone);

	// Check if 9600 baud rate is supported,(want to configure to this later)
	iTest(chCaps().iRate&KCapsBps9600);
	}


/**
 Configure the device
 @pre 	LoadDriver(), TestOpenChannel() called with their pre conditions
 */
void TestExDriver::TestConfigure()
	{
	TInt r;

	// Configure the device (uart)
 	iTest.Printf(_L("Configure UART\n"));

 	// Package the comm config structure to descriptor and initialize the data
 	TPckgBuf<TCommConfigV01> config;
 	config().iRate = EBps9600;

 	// Configure the channel
  	r = iLdd.SetConfig(config);
 	iTest(r==KErrNone);
	}

/**
 Test Asynchronous requests
  	- 1) Transmit
  	- 2) Receive
 */
void TestExDriver::TestAsynchRequests()
	{
	TInt r;

	iTest.Next(_L("Test Asynchronous Requests"));

	// Open channel & configure
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);
	// Run the device @ default configuration with loopback

	// Test Transmit data, complete
	TestTransmit();

	// Test cancel asynchronous request
	TestCancelRequests();

	// Test multiple same requests
	TestMultipleSameRequests();

	// close channel
	iLdd.Close();

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0644
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver transmit functionality, as a part of
//! 					testing the asynchrnous request implementation	
//! @SYMTestActions     1.	Transmit data over the device
//! 					2.	Wait for transmit request completion
//!
//! @SYMTestExpectedResults 1.	Transmit returns KErrNone
//! 						2.	Wait for request completes with KErrNone
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Transmits data over the device
 @pre	TestLoadDriver(), channel opened
 */
void TestExDriver::TestTransmit()
	{
	TInt r;

	// Transmit data to the device (uart)
 	iTest.Printf(_L("Test Transmit Asynchronous Request\n"));

	// User side chunk object. 	
 	RChunk chunkTx; 
 	
 	// Get the handle to the chunk. Driver creates the chunk and returns
 	// the handle to the user to access it. Handle will be assigned to
 	// the user side chunk object (done in .inl file) using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=iLdd.GetTxChunkHandle(chunkTx); 	
 	iTest(r==KErrNone);		
 	 	
	TUint8* chunkbase;
	// Retrieve the base address of the chunk. Usign this address user access
	// the shared chunk just like any memory pointer.RChunk::Base() returns 
	// the linear address of teh shared chunk.
	//
	chunkbase=chunkTx.Base(); 
	
	// Writing the data to the shared chunk, using chunk base address.
	// Note, here we need not send data to the driver.We just write to the 
	// buffer and driver directly access the chunk in kernel side.
	//
	TPtr8 inbuf(chunkbase,KTestTxDataMedium().Size());
	inbuf = KTestTxDataMedium;
	
	iTest.Printf(_L("User Chunk Base=%x\n"),chunkbase);
	
 	// Request status object for transmit. This object will be used to read the
 	// status of asynchronous requests after the notification of request completion
 	//
 	TRequestStatus txStatus;

 	// Create the iTimer that is relative to the thread
 	r = iTimer.CreateLocal();
 	iTest(r==KErrNone);

 	// Trigger iTimer expiry after KTimeOut. The status should be pending
	iTimer.After(iTimeStatus,KTimeOutTxRx);
	iTest(iTimeStatus==KRequestPending);

  	 // Call ldd interface TransmitData() API. No need to send the data, instead
 	// we send only TRequestStatus object(since asynchronous), size and if required
 	// offset in the shared chunk can be passed to driver.
 	r = iLdd.TransmitData(txStatus,inbuf.Size());
 	iTest(r==KErrNone);

 	// Wait till the request is complete on txStatus or iTimeStatus. User thread is
 	// blocked with this call, till it is notified about the request completion or
 	// iTimer expiry
 	//
 	User::WaitForRequest(txStatus, iTimeStatus);

 	// if transmit has occured correctly, the iTimeStatus will not be KErrNone, else
 	// no transmit complete has occured and iTimer has expired
 	iTest (iTimeStatus!=KErrNone);

 	// Cancel the iTimer request
 	iTimer.Cancel();

 	// txStatus holds the request completion. TRequestStatus::Int() returns the
 	// completion code. It will be KErrNone in case of successful completion
 	//
 	r = txStatus.Int();
 	iTest(r==KErrNone);

 	// Cancel the iTimer request
 	iTimer.Cancel();

 	// Close the handle to the iTimer
 	iTimer.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0645
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's asynchronous request cancelling
//! 					functionality as a part of testing the asynchrnous request implementation
//! @SYMTestActions     1.	Issue a receive request by calling ReceiveData()
//! 					2.	Cancel the pending receive request
//!
//! @SYMTestExpectedResults 1.	Receive request is pending with KRequestPending
//! 						2.	Request status object status is KErrCancel after cancelling the request
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test cancelling asynchronous requests. This function cancels a pending
 asynchronous receive request.

 @pre	TestLoadDriver(), channel opened
 */
void TestExDriver::TestCancelRequests()
	{
	TInt r;

	// Cancel Asynchronous Receive Request
 	iTest.Printf(_L("Test Cancelling Asynchronous Receive Request\n"));
 	
 	// Create the iTimer that is relative to the thread
 	r = iTimer.CreateLocal();
 	iTest(r==KErrNone);

	// User side receive chunk object	
	RChunk chunkRx; 
	
	// Get the handle to the chunk. Driver creates the chunk and returns
 	// the handle to the user to access it. Handle will be assigned to
 	// the user side chunk object (done in .inl file) using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=iLdd.GetRxChunkHandle(chunkRx); 	
 	iTest(r==KErrNone);
 	
	// Request status object for receive. This object will be used to read the
 	// status of asynchronous requests after the notification of request completion
 	//
 	TRequestStatus rxStatus;

 	// Trigger iTimer expiry after KTimeOutTxRx. The status should be pending
	iTimer.After(iTimeStatus,KTimeOutTxRx);
	iTest(iTimeStatus==KRequestPending);

 	// Call ldd interface ReceiveData() API to get data to RxBuf
 	r = iLdd.ReceiveData(rxStatus,10);
 	iTest(r==KErrNone);

 	// Cancel the Receive Request, This invokes a DoCancel()
 	iLdd.CancelReceive();

 	// Wait till the request is complete on rxStatus. User thread is blocked
 	// with this call, till it is notified about the request completion.
 	//
 	User::WaitForRequest(rxStatus,iTimeStatus);

 	// If receive has occured correctly, the iTimeStatus will not be KErrNone,
 	// else no receive complete has occured and iTimer has expired
 	iTest (iTimeStatus!=KErrNone);

 	// rxStatus holds the request completion. TRequestStatus::Int() returns
 	// the completion code. It should be KErrCancel in case of successful
 	// cancellation of request
 	//
 	r=rxStatus.Int();
    iTest(r==KErrCancel);

 	// Cancel the iTimer
 	iTimer.Cancel();

    // Close the handle to the iTimer
 	iTimer.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0646
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's support for multiple asynchronous requests
//! @SYMTestActions     1.	Issue a receive request by calling ReceiveData()
//! 					2.	Issue another receive request by calling ReceiveData() again
//!
//! @SYMTestExpectedResults 1.	First receive request returns KErrNone
//! 						2.	Second receive request returns KErrInUse
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------

/**
 Test multiple same requests
 */
void TestExDriver::TestMultipleSameRequests()
	{
	TInt r;

	iTest.Printf(_L("Test Multiple same type Asynchronous Requests\n"));

	/* Test multiple Receive */
 		// Request status object for receive. This object will be used to read
	// the status of asynchronous requests after the notification of
	// request completion
 	//
 	TRequestStatus rxStatus1;

 	// Call ldd interface ReceiveData() API to get data to rxBuf
 	r = iLdd.ReceiveData(rxStatus1,10);
 	iTest((r==KErrNone)&&(rxStatus1.Int()==KRequestPending));

 	// Second asynchronous request object for receive
 	TRequestStatus rxStatus2;

 	// Call ldd interface ReceiveData() API to get data to rxBuf
 	r = iLdd.ReceiveData(rxStatus2,10);
 	iTest((r==KErrNone)&&(rxStatus2.Int()==KErrInUse));

 	// Cancel the pending request, to cleanup this test
 	iLdd.CancelReceive();
 	iTest(rxStatus1.Int()==KErrCancel);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0647
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's support for supporting concurrent 
//! 					asynchronous requests, Tx and Rx full duplex mode
//! @SYMTestActions     1.	Transmit and Receive data over device concurrently (in Full Duplex mode) with loopback
//! 						a.	Enable loopback of transmit and receive
//! 						b.	Issue a receive request 
//! 						c.	Issue a transmit request
//! 						d.	Wait on Transmit request to complete
//! 						e.	Wait on Receive request to complete
//!
//! @SYMTestExpectedResults 1.	Data received is  same as data transmitted
//! 							a.	Internal loopback enabled successfully
//! 							b.	Receive completes successfully
//! 							c.	Transmit completes successfully
//! 							d.	Completes the wait on Transmit with KErrNone
//! 							e.	Completes the wait on Receive with KErrNone
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test the driver data flow path by loopback,
 i.e Transmit the data and receive same data back.
 It runs the device @ default configuration

 @param		aLoopback
 				Loopback mode as internal or external
 			aTxData
 				Transmit Data buffer
 			aRxSize
 				Receive data size
 */
void TestExDriver::TestConcurrentTxRx(TInt aLoopback, const TDesC8& aTxData)
	{
	TInt r;

 	// Request status object for transmit and receive. These object will be
 	// used to read the status of asynchronous requests after the notification
 	// of request completion
 	//
 	TRequestStatus txStatus;
 	TRequestStatus rxStatus;

 	// Timers and their respective status objects for Tx and Rx
 	RTimer timerTx;
	RTimer timerRx;
	TRequestStatus timeStatusTx;
	TRequestStatus timeStatusRx;
	 	
	iTest.Printf(_L("Test Concurrent Asynchronous Requests - Tx/Rx\n"));

	// Open channel
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);

	r=iLdd.SetIntLoopback(aLoopback);
	iTest(r==KErrNone);
	
	// User side transmit chunk object. 	
 	RChunk chunkTx; 
 	// User side receive chunk object	
	RChunk chunkRx; 
	
 	// Get the handle to the chunk. Driver creates the chunk and returns
 	// the handle to the user to access it. Handle will be assigned to
 	// the user side chunk object (done in .inl file) using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=iLdd.GetTxChunkHandle(chunkTx); 	
 	iTest(r==KErrNone);		
 		
	// Get the handle to the chunk. Driver creates the chunk and returns
 	// the handle to the user to access it. Handle will be assigned to
 	// the user side chunk object (done in .inl file) using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=iLdd.GetRxChunkHandle(chunkRx); 	
 	iTest(r==KErrNone);
 	
 	
 	TUint8* chunkbase;
	// Retrieve the base address of the chunk. Usign this address user access
	// the shared chunk just like any memory pointer.RChunk::Base() returns 
	// the linear address of teh shared chunk.
	//
	chunkbase=chunkTx.Base(); 
	
	// Writing the data to the shared chunk, using chunk base address.
	// Note, here we need not send data to the driver.We just write to the 
	// buffer and driver directly access the chunk in kernel side.
	//
	TPtr8 inbuf(chunkbase,aTxData.Size());
	inbuf = aTxData;
	
	iTest.Printf(_L("User Chunk Base=%x\n"),chunkbase);
	
 	// Create the timer that is relative to the thread
 	r = timerTx.CreateLocal();
 	iTest(r==KErrNone);
 	r = timerRx.CreateLocal();
 	iTest(r==KErrNone);

 	iTest.Printf(_L("Receive Data\n"));

 	// Trigger timerRx expiry after KTimeOutTxRx. The status should be pending
	timerRx.After(timeStatusRx,KTimeOutTxRx);
	iTest(timeStatusRx==KRequestPending);

 	// Call ldd interface ReceiveData() API to get data to rxBuf
 	r = iLdd.ReceiveData(rxStatus,aTxData.Size());
 	// In case of zero length request
 	if (r==KErrArgument)
 		{
 		 // Driver should return error immediately
 		iTest(r!=KErrNone); 		
 		// Close channel
		iLdd.Close();
		return;
 		}
 	else
 		{
 		// Asynchronous request should be pending, with request message
 		// posted to driver successfully
 		//
 		iTest((r==KErrNone)&&(rxStatus.Int()==KRequestPending));
 		}

   	// Trigger timerTx expiry after KTimeOutTxRx. The status should be pending
	timerTx.After(timeStatusTx,KTimeOutTxRx);
	iTest(timeStatusTx==KRequestPending);

 	// Call ldd interface TransmitData() API test data descriptor as parameter
 	r = iLdd.TransmitData(txStatus,aTxData.Size());
 	// In case of zero length request
 	if (aTxData.Size()==0)
 		{
 		// Driver should return error immediately
 		iTest(r!=KErrNone); 	
		// Close channel
		iLdd.Close();
		return;
 		}
 	else
 		{
 		// Asynchronous request should be pending, with request message
 		// posted to driver successfully
 		//
 		iTest(r==KErrNone);
 		}

 	// Wait till the request is complete on rxStatus and txStatus. User thread
 	// is blocked with this call, till it is notified about the request
 	// completion.
 	//
 	if(txStatus.Int()==KRequestPending)
 	User::WaitForRequest(txStatus,timeStatusTx);
 	if(rxStatus.Int()==KRequestPending)
 	User::WaitForRequest(rxStatus,timeStatusRx);

 	// if transmit has occured correctly, the iTimeStatus will not be KErrNone, else
 	// no transmit complete has occured and iTimer has expired
 	iTest (timeStatusTx!=KErrNone);
 	iTest (timeStatusRx!=KErrNone);

 	// Cancel the iTimer request
 	timerTx.Cancel();
 	timerRx.Cancel();

 	// txStatus holds the request completion. TRequestStatus::Int() returns the
 	// completion code. It will be KErrNone in case of successful completion
 	//
 	r = txStatus.Int();
 	iTest((r==KErrNone)||(r==KErrTimedOut));
 	r = rxStatus.Int();
 	iTest((r==KErrNone)||(r==KErrTimedOut));

	TUint8* rxchunkbase;	
 	// Retrieve the base address of the chunk. Using this address user access
	// the shared chunk just like any memory pointer. RChunk::Base() returns 
	// the linear address of teh shared chunk.
	//
	rxchunkbase=chunkRx.Base(); 
	
	// Read the data from the shared chunk, using chunk base address.
	// Note, driver need not send data in a buffer to user. It just notifies 
	// receive completion, and data is read directly from shared chunk.
	//
	// Print the recieve data to display and verify the data received manually.
 	TInt i;
 	iTest.Printf(_L("Received Data of size (%d):"),inbuf.Size());
 	for (i=0;i<aTxData.Size();i++)
 		{
 		iTest.Printf(_L("%c"),((TUint8*)rxchunkbase)[i]);
 		if (((TUint8*)rxchunkbase)[i] != aTxData[i])
 			{
 			iTest.Printf(_L("Transmit and Receive data do not match\n"));		
 			iTest(EFalse);
 			}
 			
 		}
 	
 	iTest.Printf(_L("\n"));
	
 	// Close the handle to the timerTx and timerRx
 	timerTx.Close();
 	timerRx.Close();
 
	// Close channel
	iLdd.Close();
	}



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0648
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's Tx and Rx path for various buffer sizes
//! 					and loopback modes on all supported units
//! 					asynchronous requests, Tx and Rx full duplex mode
//! @SYMTestActions    1.	Transmit and Receive Data in full duplex mode on unit1 and internal loopback with buffers of
//! 						a.	Tx size 0
//! 						b.	Tx size small
//! 						c.	Tx size medium
//! 						d.	Tx size large
//! 					2.	Repeat step (1) on unit1 with external loopback
//! 					3.	Repeat step (1) on unit2 with internal loopback
//! 					4.	Repeat step (1) on unit2 with external loopback
//!
//! @SYMTestExpectedResults 1.	Transmit and Receive complete successfully in full duplex mode on unit1, internal loopback and
//! 							a.	Tx and Rx requests returns KErrArgument
//! 							b.	Tx and Rx are successful,  returning KErrNone and  Tx completes the wait on the requests with KErrNone 
//! 								and RX complete the wait on request with KErrTimedOut
//! 							c.	Tx and Rx are successful,  returning KErrNone and Tx completes the wait on the requests with KErrNone 
//! 								and RX complete the wait on request with KErrTimedOut
//! 							d.	Tx and Rx are successful,  returning KErrNone and  Tx completes the wait on the requests with KErrNone 
//! 								and RX complete the wait on request with KErrTimedOut
//! 						2.	Gets results as in expected result (1) on unit1 with external loopback
//! 						3.	Gets results as in expected result in (1) on unit2 with internal loopback
//! 						4.	Does not continue with Tx and Rx and exit as unit2 doesn't support external loopback
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test data flow on multiple units with loopback enable and disable
 @pre TestLoadDriver() called
 */
void TestExDriver::TestDataflowMultipleUnitsLoopback()
	{
	iTest.Printf(_L("TestDataflow1:Unit1,Internal Loopback\n"));
	// Test data flow on unit1, internal loopback
	TestDataFlow(KUnit1, KIntLoopbackEnable);

	// Test data flow on unit1, external loopback
	iTest.Printf(_L("\nTestDataflow2:Unit1,External Loopback\n"));
	iTest.Printf(_L("COM0 needs to be loopbed back externally\n\n"));
	User::After(2000000); // 2 secs, in units of microseconds
	TestDataFlow(KUnit1, KIntLoopbackDisable);

	// Test data flow on unit2, internal loopback
	iTest.Printf(_L("\nTestDataflow3:Unit2,Internal Loopback\n"));
	TestDataFlow(KUnit2, KIntLoopbackEnable);

	// Test data flow on unit2, external loopback
	// external loopback is not possible on unit2 and external
	iTest.Printf(_L("\nTestDataflow4:Unit2,External Loopback\n\n"));
	TestDataFlow(KUnit2, KIntLoopbackDisable);
	
	// Test data flow on unit1, Internal loopback
	// Create two channel , Get Shared chunk from second channel and pass two first channel.
	// All the following test cases ( from 5 to 14) tests the serial driver Tx & Rx functionality 
	// with different 1. Offset values 
	//				  2. Tx and Rx sizes and 
	//				  3. Different string data. ( KTestTxDataVerySmall,KTestTxDataMedium,KTestTxDataLarge & KTestTxDataVeryLarge).
	
	iTest.Printf(_L("\nTestDataflow5:Unit1,Internal Loopback With two driver channel\n\n"));
	// Tx Size = 6
	// Rx Size = 2;
	// Tx Offs = 0;
	// Rx Offs = 4090;
	// Tx and Rx chunk size is 4096.
	// Txed data KTestTxDataMedium length = 80
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,6,2,0,4090);
	
	iTest.Printf(_L("\nTestDataflow6:Unit1,Internal Loopback With two driver channel\n\n"));
	// Tx Size = 1413
	// Rx Size = 3913
	// Tx Offs = 0;
	// Rx Offs = 2500
	// Tx and Rx chunk size is 4096.
	// Txed data KTestTxDataMedium length = 3913
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataLarge,1413,KTestTxDataLarge.iTypeLength,0,2500);
	
	iTest.Printf(_L("\nTestDataflow7:Unit1,Internal Loopback With two driver channel\n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataLarge,KTestTxDataLarge.iTypeLength,KTestTxDataLarge.iTypeLength,0,0);
	
	iTest.Printf(_L("\nTestDataflow8:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,6,6,0,4090);
	
	iTest.Printf(_L("\nTestDataflow9:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,10,10,0,4090);

	iTest.Printf(_L("\nTestDataflow10:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,25,25,0,4090);
	
	iTest.Printf(_L("\nTestDataflow11:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,24,24,0,4090);
	
	iTest.Printf(_L("\nTestDataflow12:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,6,6,0,4090);
	
	iTest.Printf(_L("\nTestDataflow13:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataMedium,23,23,0,4090);
	
	iTest.Printf(_L("\nTestDataflow14:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataVeryLarge,12,12,4092,0);
	
	iTest.Printf(_L("\nTestDataflow15:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataVeryLarge,11,11,4092,0);
	
	iTest.Printf(_L("\nTestDataflow16:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataVeryLarge,4,4,4092,0);
	
	iTest.Printf(_L("\nTestDataflow17:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataVeryLarge,5,5,4092,0);
	
	iTest.Printf(_L("\nTestDataflow18:Unit1,Internal Loopback With two driver channel \n\n"));
	TestConcurrentTxRxPassSharedChunk(KIntLoopbackEnable,KTestTxDataVerySmall,KTestTxDataVerySmall.iTypeLength,KTestTxDataVerySmall.iTypeLength,0,0);
	
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXCHNK-0649
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's support for supporting concurrent 
//! 					asynchronous requests, Tx and Rx full duplex mode with two driver channels
//!						Shared chunk created in channel 2 is passed to channel 1. 
//! @SYMTestActions     1.	Transmit and Receive data over device concurrently (in Full Duplex mode) with loopback
//! 						a.	Enable loopback of transmit and receive
//							b.  Create Shared chunk in Channel 2. Pass Shared chunk to Channel 1.
//! 						c.	Issue a receive request on channel 1
//! 						d.	Issue a transmit request on channel 1
//! 						e.	Wait on Transmit request to complete 
//! 						f.	Wait on Receive request to complete
//!
//! @SYMTestExpectedResults 1.	Data received is  same as data transmitted
//! 							a.	Internal loopback enabled successfully
//!								b.  Shared chunk created successfully and passed to other channel successfully.
//! 							c.	Receive completes successfully on channel 1
//! 							d.	Transmit completes successfully on channel 1
//! 							e.	Completes the wait on Transmit with KErrNone
//! 							f.	Completes the wait on Receive with KErrNone
//!							
//! @SYMTestPriority    High
//! @SYMTestStatus      Implemented
//----------------------------------------------------------------------------------------------
/**
 Test the driver data flow path by loopback,
 i.e Transmit the data and receive same data back.
 It runs the device @ default configuration

 @param		aLoopback
 				Loopback mode as internal or external
 			aTxData
 				Transmit Data buffer
 			aRxSize
 				Receive data size
 */
void TestExDriver::TestConcurrentTxRxPassSharedChunk(TInt aLoopback, const TDesC8& aTxData,TInt aTxSize , TInt aRxSize, TInt aTxOffset, TInt aRxOffset)
	{
	TInt r;

 	// Request status object for transmit and receive. These object will be
 	// used to read the status of asynchronous requests after the notification
 	// of request completion
 	//
 	TRequestStatus txStatus;
 	TRequestStatus rxStatus;

 	// Timers and their respective status objects for Tx and Rx
 	RTimer timerTx;
	RTimer timerRx;
	TRequestStatus timeStatusTx;
	TRequestStatus timeStatusRx;
	 
	iTest.Printf(_L("Test Concurrent TxRx With Two driver channel - Shared chunk created in one channel and passed to second channel\n"));

	// Second driver channel instance.
	RExDriverChannel ldd2;			// Logical channel
	
	// Open First Driver Channel.
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);

	r=iLdd.SetIntLoopback(aLoopback);
	iTest(r==KErrNone);
	
	// Open Second Driver channel.
	r=ldd2.Open(KUnit2);
	iTest(r==KErrNone);

	r=ldd2.SetIntLoopback(aLoopback);
	iTest(r==KErrNone);
	
	// User side transmit chunk object. 	
 	RChunk chunkTx; 
 	// User side receive chunk object	
	RChunk chunkRx; 
	
	
 	// Get the handle to the chunk from Driver Channel 2. 
 	// Driver creates the chunk and returns the handle to the user to access it. 
 	// Handle will be assigned to the user side chunk object (done in .inl file) 
 	// using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=ldd2.GetTxChunkHandle(chunkTx); 	
 	iTest(r==KErrNone);		
 		
	// Get the handle to the chunk. Driver creates the chunk and returns
 	// the handle to the user to access it. Handle will be assigned to
 	// the user side chunk object (done in .inl file) using RChunk::SetHandle().
 	// The handle has to be a positivr value. It can be obtained using 
 	// RChunk::Handle(), if required.
 	//
 	r=ldd2.GetRxChunkHandle(chunkRx); 	
 	iTest(r==KErrNone);
 	
 	
 	// Pass the shared chunk created in channel 2 to Channel 1.
 	// It is as good as passing shared chunk from one driver to another.
 	
 	r = iLdd.PassTxChunkHandle(chunkTx);
 	r = iLdd.PassRxChunkHandle(chunkRx);
 	
 	
 	// Perform Loopback Tx and Rx on Driver channel 1.
 	
	TUint8* chunkbase;
	// Retrieve the base address of the chunk. Usign this address user access
	// the shared chunk just like any memory pointer.RChunk::Base() returns 
	// the linear address of teh shared chunk.
	//
	chunkbase=chunkTx.Base(); 
	
	// Writing the data to the shared chunk, using chunk base address.
	// Note, here we need not send data to the driver.We just write to the 
	// buffer and driver directly access the chunk in kernel side.
	//
	TPtr8 inbuf(chunkbase,aTxData.Size());
	inbuf = aTxData;
	
	iTest.Printf(_L("User Chunk Base=%x\n"),chunkbase);
	
 	// Create the timer that is relative to the thread
 	r = timerTx.CreateLocal();
 	iTest(r==KErrNone);
 	r = timerRx.CreateLocal();
 	iTest(r==KErrNone);

 	iTest.Printf(_L("Receive Data\n"));

 	// Trigger timerRx expiry after KTimeOutTxRx. The status should be pending
	timerRx.After(timeStatusRx,KTimeOutTxRx);
	iTest(timeStatusRx==KRequestPending);

 	// Call ldd interface ReceiveData() API to get data to rxBuf
 	r = iLdd.ReceiveData(rxStatus,aRxSize,aRxOffset);
 	
 	
 	// Check return value , if invalid length then it will be KErrArgument.
 	if (r==KErrArgument)
 		{
 		 // Driver should return error immediately
 		iTest(r!=KErrNone); 		
 		// Close channel
		iLdd.Close();
		ldd2.Close();
 		return;
 		}
 	else
 		{
 		// Asynchronous request should be pending, with request message
 		// posted to driver successfully
 		//
 		iTest((r==KErrNone)&&(rxStatus.Int()==KRequestPending));
 		}

   	// Trigger timerTx expiry after KTimeOutTxRx. The status should be pending
	timerTx.After(timeStatusTx,KTimeOutTxRx);
	iTest(timeStatusTx==KRequestPending);

 	// Call ldd interface TransmitData() API test data descriptor as parameter
 	r = iLdd.TransmitData(txStatus,aTxSize,aTxOffset);
 	// In case of zero length request
 	if (aTxData.Size()==0)
 		{
 		// Driver should return error immediately
 		iTest(r!=KErrNone); 	
		// Close channel
		iLdd.Close();
		ldd2.Close();
		return;
 		}
 	else
 		{
 		// Asynchronous request should be pending, with request message
 		// posted to driver successfully
 		//
 		iTest(r==KErrNone);
 		}

 	// Wait till the request is complete on rxStatus and txStatus. User thread
 	// is blocked with this call, till it is notified about the request
 	// completion.
 	//
 	if(txStatus.Int()==KRequestPending)
 	User::WaitForRequest(txStatus,timeStatusTx);
 	if(rxStatus.Int()==KRequestPending)
 	User::WaitForRequest(rxStatus,timeStatusRx);

 	// if transmit has occured correctly, the iTimeStatus will not be KErrNone, else
 	// no transmit complete has occured and iTimer has expired
 	iTest (timeStatusTx!=KErrNone);
 	iTest (timeStatusRx!=KErrNone);

 	// Cancel the iTimer request
 	timerTx.Cancel();
 	timerRx.Cancel();

 	// txStatus holds the request completion. TRequestStatus::Int() returns the
 	// completion code. It will be KErrNone in case of successful completion
 	//
 	r = txStatus.Int();
 	iTest((r==KErrNone)||(r==KErrTimedOut));
 	r = rxStatus.Int();
 	iTest((r==KErrNone)||(r==KErrTimedOut));

	TUint8* rxchunkbase;	
 	// Retrieve the base address of the chunk. Using this address user access
	// the shared chunk just like any memory pointer. RChunk::Base() returns 
	// the linear address of teh shared chunk.
	//
	rxchunkbase=chunkRx.Base(); 
	
	
	// Following logic is completely for verifying the Rx data with Tx data.
	
		
	TInt rxsize;
	TBool rxwraparoundflag = ETrue;
	
	// Find the actual space where data is written.
	if( aRxOffset + aRxSize > chunkRx.Size())
		{
		// This confirms Rx Buffer has less space than aRxSize.
		rxsize = chunkRx.Size() - aRxOffset;
		}
	else 
		{
		// This confirms Rx has enough space.
		rxsize = aRxSize;
		// If txsize is also less than rxsize then in RxBuffer data is not wrap around.
		rxwraparoundflag = EFalse;
		}
	

	TInt txsize;
	if( aTxSize > (aTxData.Size() - aTxOffset ))
		{
		txsize = aTxData.Size() - aTxOffset;
		}
	else 
		{
		txsize = aTxSize;
		if (txsize< rxsize)
			// This is the case when data is not wrap around.
			// Tx and Rx data both matches exactly the same.
			rxsize = txsize;
		}
	
	if ((rxwraparoundflag == EFalse) && (txsize>rxsize))
		{
		txsize = rxsize;
		}
	
	iTest.Printf(_L("Received Data of size (%d):"),rxsize);
	
	if (rxsize<=txsize)
		{
			TInt mod = txsize % rxsize;
			TInt quo = txsize / rxsize;
			
			for (TInt i=0;i<mod;i++)		
				{
				
				iTest.Printf(_L("%c"),(TUint8*)rxchunkbase[i+aRxOffset]);		
				if (((TUint8*)rxchunkbase)[i+aRxOffset] != aTxData[(rxsize*quo) +aTxOffset +i ])
 					{
 					iTest.Printf(_L("Transmit and Receive data do not match\n"));		
 					iTest(EFalse);
 					}
				}
			
			for(TInt i=mod;i<rxsize;i++)					
			{
			iTest.Printf(_L("%c"),(TUint8*)rxchunkbase[i+aRxOffset]);		
			if (((TUint8*)rxchunkbase)[i+aRxOffset] != aTxData[(rxsize*(quo-1)) + aTxOffset +i])
 				{
 				iTest.Printf(_L("Transmit and Receive data do not match\n"));		
 				iTest(EFalse);
 				}
				
			}
		}
	else
		{
		TInt loop = 0;
		for (TInt i=0;i<rxsize;i++)		
			{
				
			if (loop >= txsize)
				{
				loop =0;
				}
				
			iTest.Printf(_L("%c"),((TUint8*)rxchunkbase)[i+aRxOffset]);		
			if (((TUint8*)rxchunkbase)[i+aRxOffset] != aTxData[aTxOffset + loop ])
 				{
 				iTest.Printf(_L("Transmit and Receive data do not match\n"));		
 				iTest(EFalse);
 				}
 			loop++;
			}
				
			
		}
		
	iTest.Printf(_L("\n"));
	
	// Close the handle to the timerTx and timerRx
 
 	timerTx.Close();
 	timerRx.Close();
 
	// Close channel
	iLdd.Close();
	ldd2.Close();
	}

/**
 Test data flow for various buffer sizes
 Loopback Tx data to Rx

 @param		aUnit
 			Unit in which this test is being done
 			aLoopback
 			Loopback mode - internal or external
 */
void TestExDriver::TestDataFlow(TInt aUnit, TInt aLoopback)
	{
	// External loopback is possible only on unit1
	if ((aUnit!=KUnit1)&(aLoopback==KIntLoopbackDisable))
		return;

	// case1: Tx size 0
	TestConcurrentTxRx(aLoopback,KTestTxDataZero);

	// case2Tx size small
	TestConcurrentTxRx(aLoopback,KTestTxDataSmall);
	
	// case3Tx size medium
	TestConcurrentTxRx(aLoopback,KTestTxDataMedium);

	// case4 Tx size large
	TestConcurrentTxRx(aLoopback,KTestTxDataLarge);

	}

/**
 Change the debugport to port KDebugPort(3)

 This function is optional and specific to variant. Incase of H4 variant,
 default debugport is 1. However since this tutorial driver tests both
 unit1(port1) and unit2(port2), default port is being changed to port3
 to avoid conflict

 @post	Debugport changed to COM port3
 */
TInt TestExDriver::ChangeDebugPort(TInt aNewPort)
	{
	TInt muid;
	TInt val=-1;
	TInt newval;

	// HAL API provides interface to read and set some system attributes
	// specified by HALData. Here, HAL::EMachineUid gives the machine uid
	// on which this test is being executed.
	//
	const TInt r = HAL::Get(HAL::EMachineUid, muid);
	if (r==KErrNone)
		{
		// If the board is H4, we would like to change the debugport to 3
		// to enable testing unit1. If unit1 is not being tested, then
		// no need to change the debugport
		//
	  	if (muid == HAL::EMachineUid_OmapH4)
	  		{
	  		// Get the deugport that is set currently using HAL API
	  		HAL::Get(HALData::EDebugPort,val);
	  		if (val!=aNewPort)
	  			{
	  			// Set the debugport to KDebugPort
	  			HAL::Set(HALData::EDebugPort,aNewPort);
	  			// Read again and verify
	  			HAL::Get(HALData::EDebugPort,newval);
	  			// Here, test() macro is not used to check as we dont want
	  			// to panic or exit if the check fails. test() macro check
	  			// panics and exits on the condition cheked being false
	  			//
	  			if(newval!=aNewPort)
	  				{
	  				// Failed changing debugport, we'll not be able to view
	  				// log messages on the debug port, but can see on display
	  				//
	  				iTest.Printf(_L("\nChanging DebugPort Failed\n"));
	  				}

	  			// Debugport changed to KDebugPort
	  			iTest.Printf(_L("\nDebugPort Changed to (%d)\n"),newval);

				// Let the user know the current debug port
				iTest.Printf(_L("\nPlease use COM port%d to view the debug messages on PC's hyperterminal\n"), newval);

				// User::After() suspends the current thread until a specified time
				// interval has expired. Time is given in microseconds.
				// This is added here to give the user view the above messages before
				// continuing
				//
				User::After(2000000); // 2 secs, in units of microseconds
	  			}
	  		}
		}

	// return the initial debug port
	return val;
	}
	
//
//  End of t_exchnk_user.cpp

