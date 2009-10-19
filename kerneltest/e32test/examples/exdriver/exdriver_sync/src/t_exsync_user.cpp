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
// This application tests the driver for its synchronous request handling and
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
// 3) If driver enables debug messages, the timeout values used here
// will have to be increased
// 
//

// include files
//
#include <e32test.h>			// for test framework
#include "exsync.h"				// driver interface
#include <hal.h>				// for HAL API
#include <hal_data.h>			// for HAL attributes
#include "t_exsync_testdata.h" 	// test data

// Literal string descriptors for LDD and PDD names of the driver
//
_LIT(KExDriverLdd, "d_exsync_ldd");	// Ldd name
_LIT(KExDriverPdd, "d_exsync_pdd");	// Pdd name

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
const TInt KTestRxSize = 1000;  // Rx request data size in bytes
const TInt KHeapSize=0x2000;

// Following class is used to pass Ldd and Transfer data handle information 
// to different Transfer Thread.
class TData
	{
public:
	
	RSemaphore *iSem1;
	RExDriverChannel* iLdd;							// Ldd handle
	const TDesC8 *iTxData;							// Data to be transferred
	
	TData(RExDriverChannel* aLdd, const TDesC8 *aTxData,RSemaphore *sem1) 
		{
		iLdd = aLdd;
		iTxData = aTxData;
		iSem1 = sem1;
		}
	};
	

// 40 sec, interms of microseconds. This is more, as large buffers
// of data is being used. However, this timeout value is only used
// to provide an exit method from wait on asynch requests in some
// error conditions.
//
//const TUint KTimeOutTxRx=40000000;

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
	void TestDevicePresent();
	// Test the driver for channels and units support
	void TestChannelsAndUnits();
	// Test synchronous requests - caps, configure, loopback
	void TestSynchRequests();
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
	// Test multiple same requests
	void TestMultipleSameRequests();
	// Test Tx and Rx duplex mode, concurrent requests in loopback mode
	void TestConcurrentTxRx(TInt aLoopback, const TDesC8& aTxData, TInt aRxSize);
	/*
	 Data flow tests
	 */
	void TestDataFlow(TInt aUnit, TInt aLoopback);
public:
	RTest iTest;					// RTest reference object
	RTimer iTimer;					// Timer reference object
	RExDriverChannel iLdd;			// Logical channel
	TRequestStatus	iTimeStatus;	// Asynch request object
	RSemaphore iSem1;
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
	exDrv.iTest.Start(_L("Tutorial Driver:Phase-IV:B"));

	// Create the semaphore.
	exDrv.iSem1.CreateLocal(0);
		
	// Load the driver
	exDrv.TestLoadDriver();

	// Test the device functionality
	exDrv.TestDevicePresent();

	// Test Channels and units
	exDrv.TestChannelsAndUnits();

	// Test Synchronous requests
	exDrv.TestSynchRequests();

	// Test Dataflow on multiple units with loopback
	exDrv.TestDataflowMultipleUnitsLoopback();

 	// Unload the driver
	exDrv.TestUnloadDriver();

	// If debugport is changed then, revert it back to the default debug port
 	if (debugport>=0)
 		{
 		exDrv.iTest.Printf(_L("Reverting the debugport to port %d\n"),debugport);
 		// Revert debugport?
 		exDrv.ChangeDebugPort(debugport);
 		}

	// Close the Sempahore.
	exDrv.iSem1.Close();
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
:iTest(_L("Tutorial Driver:Phase-IV:B"))
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
//! @SYMTestCaseID      PBASE-T_EXSYNC-0650
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver (LDD and PDD) installing
//! @SYMTestActions     1.	Load the PDD by name, d_exsync.pdd
//! 					2.	Load the LDD by name, d_exsync.ldd
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
//! @SYMTestCaseID      PBASE-T_EXSYNC-0651
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
//! @SYMTestCaseID      PBASE-T_EXSYNC-0652
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
 
void TestExDriver::TestDevicePresent()
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
	
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXSYNC-0653
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
//! @SYMTestCaseID      PBASE-T_EXSYNC-0654
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
	iTest(r==KErrInUse);
	iTest.Printf(_L("Open Channel-1 on Unit2: Failed as Expected\n"));

	// Close the channel1 that is opened successfuly
	iTest.Printf(_L("Close Channel-1 on Unit1\n"));
	iLdd.Close();
	// It is not strictly necessary to call close for following two channels
	// as the destructor will be called by default as both goes out of scope.
	//
	ldd1.Close();
	ldd2.Close();
	
	}

// Create RTest handle for this thread.
LOCAL_D RTest testDuplicateHandle(_L("TransferTestThread"));

int DuplicateHandleTestThread(TAny* aData)
{

	TData& data = *(TData *)aData;
	
	RExDriverChannel ldd2=*(data.iLdd);
	
	// Check handle duplication.
	TInt r = ldd2.Duplicate(RThread(),EOwnerProcess);
	testDuplicateHandle(r == KErrNone);
	
	ldd2.Close();
			
	return KErrNone;
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXSYNC-0655
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver support for sharing handles 
//! @SYMTestActions     1.	Duplicate handle with EOwnerProcess
//! 					2.	Duplicate handle with  EOwnerThread
//!
//! @SYMTestExpectedResults 1.	Duplicate handle with EOwnerProcess returns KErrNone, success case
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
	RThread		DuplicateThread;
 	TRequestStatus  stat;
 	
 	_LIT(KThreadName, "DuplicateThread");

	// Check if the driver can verify and deny access to wrong clients
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

	// Above API should return KErrNone.
	//
    iTest(r==KErrNone);
    ldd2.Close();

	//Handle duplication
    iTest.Printf(_L("Check handle duplication\n"));
    ldd2=iLdd;

	// Check handle duplication, with ownership as this Thread.
    r=ldd2.Duplicate(RThread(),EOwnerThread);
    iTest(r==KErrNone);
    ldd2.Close();
    iLdd.Close();
    
    
    // Open a channel on Unit1
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);
	
    TData TData(&iLdd,NULL,NULL);
 	
 	// Create the thread and pass currently opened iLdd handle to it.
 	// This thread shall check the handle duplication.
 	
  	TInt ret = DuplicateThread.Create(		KThreadName, 		// Thread name
							DuplicateHandleTestThread,		// Function to be called
							KDefaultStackSize,
							KHeapSize,
							KHeapSize,
							(TAny *)&TData
						);
    
    DuplicateThread.Logon(stat);
    DuplicateThread.Resume();
    
    User::WaitForRequest(stat);
    
    DuplicateThread.Close();
    iLdd.Close();    
    
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXSYNC-0656
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
  @pre 	LoadDriver(), TestOpenChannel() called with their pre conditions
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
 Data Transfer Thread.
 This thread is called by TestConcurrentTxRx() function. It transmits the data.
 Also checks how ReceiveData function behaves when already ReceiveData is pending
 in main thread. 
 
 @pre 	LoadDriver(), TestOpenChannel() called with their pre conditions
 
 */
LOCAL_D RTest testTransferThread(_L("TransferTestThread"));


int TransferTestThread(TAny* aData) {

	// This delay is needed to make sure this thread starts only after
	// receive data function is called from the main thread.
	
	TData& data = *(TData *)aData;
	
	// Wait On the semaphore. 
	// This is to make sure that this thread do not start untill main thread
	// calls ReceiveData method.
	data.iSem1->Wait();
	
	testTransferThread.Start(_L("Start Transfer Thread\n"));
	RExDriverChannel  *iLdd2;
	
	iLdd2= data.iLdd;
			
	RBuf8 rxBuf;
 	
 	TInt r=rxBuf.Create(1000);
 	
 	testTransferThread.Printf(_L("Check Multiple receive request on same channel : "));
 	// Try calling Receive Data again from the same channel when already another
 	// receive data request is pending.
 	// It should return KErrInUse. 	
 	r = iLdd2->ReceiveData(rxBuf);
 	testTransferThread(r == KErrInUse);
 	testTransferThread.Printf(_L("Fails : As Expected \n"));
 	
	r = iLdd2->TransmitData(*data.iTxData);
	testTransferThread(r == KErrNone);
	
	testTransferThread.Printf(_L("Data Transfer Complete.\n"));
	
	testTransferThread.End();
	testTransferThread.Close();
	
	return KErrNone;
	
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXSYNC-0657
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's support for supporting  
//! 					synchronous requests, Tx and Rx full duplex mode
//! @SYMTestActions     1.	Transmit and Receive data over device concurrently (in Full Duplex mode) with loopback
//! 						a.	Enable loopback of transmit and receive
//! 						b.	Start Trasmit Thread.
//!							c.  Issue a syncronous receive request 
//! 						d.	Main Thread is pending till data is received.
//!
//! @SYMTestExpectedResults 1.	Data received is  same as data transmitted
//! 							a.	Internal loopback enabled successfully
//! 							b.	Transmit thread starts succesfully.
//! 							c.	Transmit completes successfully
//! 							e.	Receive completes with KErrNone
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
void TestExDriver::TestConcurrentTxRx(TInt aLoopback, const TDesC8& aTxData, TInt aRxSize)
	{
	TInt r;
	TRequestStatus stat;

 
	iTest.Printf(_L("Test Concurrent Synchronous Requests - Tx/Rx\n"));

	// Open channel
	r=iLdd.Open(KUnit1);
	iTest(r==KErrNone);

 	// Create a buffer that has to be filled and returned by the driver
 	RBuf8 rxBuf;
 	r=rxBuf.Create(aRxSize);
 	iTest(r==KErrNone);

	r=iLdd.SetIntLoopback(aLoopback);
	iTest(r==KErrNone);

 
 	TData TData(&iLdd,&aTxData,&iSem1);
 	
 
 	// Call ldd interface ReceiveData() API to get data to rxBuf
 	RThread		TransferThread;
 	_LIT(KThreadName, "TestThread");
 	
  	TInt ret = TransferThread.Create(		KThreadName, 		// Thread name
							TransferTestThread,		// Function to be called
							KDefaultStackSize,
							KHeapSize,
							KHeapSize,
							(TAny *)&TData
						);
	
	
 	iTest.Printf(_L("Receive Data\n"));
 	
 	TransferThread.Logon(stat);
 	
 	TransferThread.Resume();
  	
  	iSem1.Signal();
 	r = iLdd.ReceiveData(rxBuf);
 	// In case of zero length request
 	if (aRxSize==0)
 		{
 		 // Driver should return error immediately
 		iTest(r!=KErrNone);
 		
 		TransferThread.Kill(KErrNone);
		TransferThread.Close();
	
 		// Close the RBuf
 		rxBuf.Close();
 		// Close channel
		iLdd.Close();
 		return;
 		}
 	
  	// Print the receive data to display.
  	// It automatically checks the Tx and Rx data. Fails if not matched.
  	//
 	TInt i;
 	iTest.Printf(_L("Received Data of size (%d):"),rxBuf.Size());
 	for (i=0;i<rxBuf.Size();i++)
 		{
 		iTest.Printf(_L("%c"),(rxBuf.Ptr())[i]);
 		if ((TUint8)(rxBuf.Ptr())[i] != aTxData[i])
 			{
 			iTest.Printf(_L("Transmit and Receive data do not match\n"));		
 			iTest(EFalse);
 			}
 		}
 	iTest.Printf(_L("\n"));


	User::WaitForRequest(stat);
	
	TransferThread.Close();
 	// Free the receive buffer
 	rxBuf.Close();
	// Close channel
	iLdd.Close();
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_EXSYNC-0658
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1212
//! @SYMTestCaseDesc    This test case executes driver's Tx and Rx path for various buffer sizes
//! 					and loopback modes on all supported units
//! 					synchronous requests, Tx and Rx full duplex mode
//! @SYMTestActions    1.	Transmit and Receive Data in full duplex mode on unit1 and internal loopback with buffers of
//! 						a.	Tx size 0, Rx size 0 
//! 						b.	Tx size less than Rx size
//! 						d.	Tx and Rx size less than LDD frame size (256 bytes) but greater than PDD frame (64 bytes)
//! 						e.	Tx and Rx large buffers (>3000 bytes) same size
//! 					2.	Repeat step (1) on unit1 with external loopback
//! 					3.	Repeat step (1) on unit2 with internal loopback
//! 					4.	Repeat step (1) on unit2 with external loopback
//!
//! @SYMTestExpectedResults 1.	Transmit and Receive complete successfully in full duplex mode on unit1, internal loopback and
//! 							a.	Tx and Rx requests returns KErrArgument
//! 							b.	Tx and Rx are successful,  returning KErrNone.
//! 							d.	Tx and Rx are successful,  returning KErrNone.
//! 							e.	Tx and Rx are successful,  returning KErrNone.
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
	iTest.Printf(_L("TestDataflow2:Unit1,External Loopback\n"));
	iTest.Printf(_L("COM0 needs to be looped back externally\n"));
	TestDataFlow(KUnit1, KIntLoopbackDisable);

	// Test data flow on unit2, internal loopback
	iTest.Printf(_L("TestDataflow3:Unit2,Internal Loopback\n"));
	TestDataFlow(KUnit2, KIntLoopbackEnable);

	// Test data flow on unit2, external loopback
	// external loopback is not possible on unit2 and external
	iTest.Printf(_L("TestDataflow4:Unit2,External Loopback\n"));
	TestDataFlow(KUnit2, KIntLoopbackDisable);
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

	// case1: Tx size 0, Rx size 0
	TestConcurrentTxRx(aLoopback,KTestTxDataZero,0);

	// case2: Tx size less than Rx size, will result in Rx timeout
	TestConcurrentTxRx(aLoopback,KTestTxDataMedium,KTestRxSize);
	
	// case3: Tx and Rx size less than LDD frame size but > than PDD frame
	TestConcurrentTxRx(aLoopback,KTestTxDataSmall,KTestRxSize/10);
	
	// case4: Tx and Rx large buffers same size
	TestConcurrentTxRx(aLoopback,KTestTxDataLarge,sizeof(KTestTxDataLarge));
	
	
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
	  			iTest.Printf(_L("\nDebugPort Changed to (%d))\n"),newval);

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
//  End of t_exsync_user.cpp
