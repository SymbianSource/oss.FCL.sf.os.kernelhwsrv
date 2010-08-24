// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// 
//


#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include <e32debug.h>
#include "TestCaseFactory.h"
#include "debugmacros.h"
#include "testpolicy.h"
#include "testengine.h"
#include "testcaseroot.h"
#include "b2bwatchers.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_otgdiTraces.h"
#endif



TPtrC KMyApplicationName _L("t_otgdi.exe");

// The RTest object (used for console in/output)
RTest test(_L("OTGDI Unit/Dev. Testing"));

// Parameters modified from the command-line
// semi-automation mode off: (if we skip some keyboard prompts)
TBool gSemiAutomated 	= EFalse;
TBool gVerboseOutput 	= EFalse;
TInt  gOpenIterations   = 3;	// default to 3 repeats (used for the open/close 
                                // and other tests that use repeating)
TInt  gOOMIterations    = 10;	// default to 10 allocs
TBool gTestRoleMaster   = ETrue;   			// master by default
TUint gUSBVidPid 		= KTestProductID; 	// Symbian VID+PID 0x2670


extern RUsbOtgDriver  oUsbOtgDriver;

// comment this line out for normal testing
//#define TESTSOMETHING_DEF	1


#ifdef TESTSOMETHING_DEF
// Wait for Event with timeout, EFalse if we time out
// @RETURNS : ETrue if the aStatus was signalled before the timeout
TInt WaitForRequestWTime(TRequestStatus &aStatus, TTimeIntervalMicroSeconds32 aMicroseconds)
	{
	RTimer timer;
	TRequestStatus statusT;
	TRequestStatus *statarray[] = { &aStatus, &statusT};
	
	timer.CreateLocal();
	timer.After(statusT, aMicroseconds);
	User::WaitForNRequest( statarray, 2 );
	if ( statusT == KRequestPending )
		{
		timer.Cancel();
		return(ETrue);
		}
	else
		{
		return(EFalse);
		}
	}

// Test basic API premises function:
// define TESTSOMETHING_DEF if you want to test a new API or the priority of CActive 
// or the scheduler + test framework is creating doubt.
void ProtoTypeCode()
	{
		// This block of code is a prototype area, a rough approximation of a test 
		// without any Active Schedulers interfering
		TInt err(0);

		{
		TRequestStatus status1;	// calls
		
		RUsbOtgDriver::TOtgEvent   event;
		TBuf<MAX_DSTRLEN> aDescription;
		RUsbOtgDriver::TOtgIdPin	OTGIdPin;
		RUsbOtgDriver::TOtgVbus   	OTGVBus;
		RUsbOtgDriver::TOtgEvent 	OtgEvent;
		//RUsbOtgDriver::TOtgIdPin   idPinState;

		// LOAD OTG- User-driver					
		err = User::LoadLogicalDevice(KOTGDeviceInterfaceDriverName);
		if ( (err != KErrNone) && (err != KErrAlreadyExists) )
			{
			test.Printf(_L("<Error %d> Unable to load driver: %S"), err, &KOTGDeviceInterfaceDriverName);
			OstTraceExt2(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE, "<Error %d> Unable to load driver: %S", err, KOTGDeviceInterfaceDriverName);
			}
		err = oUsbOtgDriver.Open();
		if (err != KErrNone)
		    {
			test.Printf(_L("<Error %d> Unable to OPEN driver: %S"), err, &KOTGDeviceInterfaceDriverName);
			OstTraceExt2(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP01, "<Error %d> Unable to OPEN driver: %S", err, KOTGDeviceInterfaceDriverName);
			}
		else
			{	
			test.Printf(_L("OPEN driver: %S OK!"), &KOTGDeviceInterfaceDriverName);
			OstTraceExt1(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP02, "OPEN driver: %S OK!", KOTGDeviceInterfaceDriverName);
			}
		
		oUsbOtgDriver.StartStacks();
		test.Printf(_L("Stack started\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP03, "Stack started\n");
		

		test.Printf(_L("API QueueOtgIdPinNotification test:\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP06, "API QueueOtgIdPinNotification test:\n");

		for (TInt loop=0; loop <6; loop++)
			// TEST Events
			do		
				{
				test.Printf(_L("Waiting for OTG...\n"));
				OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP07, "Waiting for OTG...\n");
				status1 = KRequestPending; // reset the status object
				oUsbOtgDriver.QueueOtgIdPinNotification( OTGIdPin, status1 );
				test.Printf(_L("Current pin %d  \n"), OTGIdPin);
				OstTrace1(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP08, "Current pin %d  \n", OTGIdPin);
				
				// wait			
				User::WaitForRequest(status1);
				TInt completionCode(status1.Int());
				switch(OTGIdPin)
					{
					case RUsbOtgDriver::EIdPinAPlug:
						aDescription = _L("A-plug");
						break;
					case RUsbOtgDriver::EIdPinBPlug:
						aDescription = _L("B-plug");
						break;
					default:
						aDescription = _L("other");
						break;
					}
				test.Printf(_L("Received pin %d '%S' status(%d) \n"), OTGIdPin, &aDescription, completionCode);
				OstTraceExt3(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP09, "Received pin %d '%S' status(%d) \n", OTGIdPin, aDescription, completionCode);
				}
			while (OTGIdPin != RUsbOtgDriver::EIdPinAPlug); // 'A' plug found

		test.Printf(_L("Press any key.\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP10, "Press any key.\n");
		test.Getch();

		test.Printf(_L("Shutting down stack.\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP11, "Shutting down stack.\n");
		oUsbOtgDriver.StopStacks();

		oUsbOtgDriver.Close();

		test.Printf(_L("Free the LDD.\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP12, "Free the LDD.\n");
		err = User::FreeLogicalDevice( RUsbOtgDriver::Name() );			
		if (err != KErrNone)
			{
			test.Printf(_L("<Error %d> Unable to UN-load driver: %S"), err, &KOTGDeviceInterfaceDriverName);
			OstTraceExt2(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP13, "<Error %d> Unable to UN-load driver: %S", err, KOTGDeviceInterfaceDriverName);
			}		
		test.Printf(_L("#############\nPress any key.\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP14, "#############\nPress any key.\n");
		}
		test.Getch();

		test.Printf(_L("Free the LDD.\n"));
		OstTrace0(TRACE_NORMAL, PROTOTYPECODE_PROTOTYPECODE_DUP15, "Free the LDD.\n");
		// end this process, if we do not want to run a test now as well
		RProcess process;
		process.Open(RProcess().Id());
		process.Terminate(0); 
	}
#endif //TESTSOMETHING_DEF



static void MainL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(MAINL_MAINL);
	    }
	// Leave the hooks in for platform security
#ifdef __DATA_CAGING__
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif		

	// Identify the process and main thread
	RProcess testProcess;
	RThread().Process(testProcess);
	testProcess.RenameMe(KMyApplicationName);
	RThread().RenameMe(_L("t_otgdi.exe main thread"));

	// Allocate and provide the console for output
	test.SetConsole(Console::NewL(KMyApplicationName, TSize(KConsFullScreen,KConsFullScreen)));

	
	
#ifdef TESTSOMETHING_DEF	
	ProtoTypeCode()
	// exitprocess
#endif // TESTSOMETHING_DEF
		
	__UHEAP_MARK;

	// Create a new active scheduler for this main thread
	// we do this because console app has no scheduler, and we 
	// want to be able to call async APIs
	CActiveScheduler* sched = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);
	
	__UHEAP_MARK;
	// Create the test case Engine in USB host mode
	CTestEngine* testEngine(NULL);
	
	TRAPD(err, testEngine = CTestEngine::NewL());
	if (err == KErrNone)
		{
		CleanupStack::PushL(testEngine);
			
		// Synchronise with the client (not currently necessary, no peer)
//DS	Commenting out as "not currently necessary"		RProcess::Rendezvous(KErrNone);
		
		// and start the active scheduler
		CActiveScheduler::Start();

		// display results
		testEngine->Report();
		
		// done with the test engine now
		CleanupStack::PopAndDestroy(testEngine);
		}
	else
		{
		if (-2 == err)
		    {
			test.Printf(_L("Warning, no tests were selected!: %d\n"), err);
			OstTrace1(TRACE_NORMAL, MAINL_MAINL_DUP01, "Warning, no tests were selected!: %d\n", err);
			}
		else
		    {
			test.Printf(_L("Unable to create the test engine: %d\n"), err);
			OstTrace1(TRACE_NORMAL, MAINL_MAINL_DUP02, "Unable to create the test engine: %d\n", err);
			}
		}

	// test DONE, if we are running manual, have a delay
	if (!gSemiAutomated)
		{
		// Get the engine to hang around so we can look at the screen output
		test.Printf(KPressAnyKeyToEnd);
		OstTrace0(TRACE_NORMAL, MAINL_MAINL_DUP03, KPressAnyKeyToEnd);
		test.Getch();
		}

	__UHEAP_MARKEND;
#ifdef _DEBUG
	test.Printf(_L("Test heap leaks #1 OK\n"));
	OstTrace0(TRACE_NORMAL, MAINL_MAINL_DUP04, "Test heap leaks #1 OK\n");
#endif

	CleanupStack::PopAndDestroy(sched);
	__UHEAP_MARKEND;
#ifdef _DEBUG
	test.Printf(_L("Test heap leaks #2 OK\n"));
	OstTrace0(TRACE_NORMAL, MAINL_MAINL_DUP05, "Test heap leaks #2 OK\n");
#endif
	
	// Finish test and release resources - this ends up closing the console (our application window)
	test.End();
	test.Close();

	}


TInt E32Main()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(E32MAIN_E32MAIN);
	    }
	// Create the new trap-cleanup mechanism
	CTrapCleanup* cleanup = CTrapCleanup::New();
	
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}
		
	// Perform the tests
	TRAPD(err,MainL());
	if (err != KErrNone)
		{
		test.Printf(_L("MainL error: %d\n"),err);		
		OstTrace1(TRACE_NORMAL, E32MAIN_E32MAIN_DUP01, "MainL error: %d\n",err);		
		}
	
	delete cleanup;
	
	// Provide no error
	return KErrNone;
	}
	
