// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a debug thread for testing.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32debug.h>
#include "d_rmdebugthread2.h"

#include "d_rmdebug_step_test.h"
#include "d_demand_paging.h"

EXPORT_C TBuf8<SYMBIAN_RMDBG_MEMORYSIZE> gMemoryAccessBytes;
IMPORT_C extern void RMDebug_BranchTst1();
IMPORT_C extern TInt RMDebugDemandPagingTest();

EXPORT_C TInt TestData;
EXPORT_C TTestFunction FunctionChooser;

const TInt NUMBER_TRACE_CALLS = 200;

EXPORT_C TInt TestFunction()
	{
	//set TestData to an arbitrary value which we check for in t_rmdebug 
	TestData = 0xffeeddcc;

	User::After(3000000);	// pause three seconds.

	return 0;
	}

/**
  Wrapper around RMDebugDemandPagingTest, need to pause for a short time to
  allow time in t_rmdebug.cpp to issue a User::WaitForRequest to catch the break point
  */
EXPORT_C void TestPagedCode()
	{
	User::After(100000);

	// call the function in paged code
	RMDebugDemandPagingTest();
	}

EXPORT_C void TestMultipleTraceCalls()
	{
	//arbitrary function to set a BP on
	RMDebug_BranchTst1();
	
	for(TInt cnt = NUMBER_TRACE_CALLS; cnt>0; cnt--)
		{
		RDebug::Printf("Trace event");
		}
	
	//another arbitrary function to set a BP on
	RMDebug_StepTest_Non_PC_Modifying();
	}

CDebugServThread::CDebugServThread()
//
// Empty constructor
//
	{
	}

GLDEF_C TInt CDebugServThread::ThreadFunction(TAny*)
//
// Generic thread function for testing
//
	{
	// set FunctionChooser to run the default function
	FunctionChooser = EDefaultFunction;

	CTrapCleanup* cleanup=CTrapCleanup::New();
	if (cleanup == NULL)
		{
		User::Leave(KErrNoMemory);
		}

	RThread::Rendezvous(KErrNone);

	TestData = 1;

	while(TestData != 0xFFFFFFFF)
		{
		switch(FunctionChooser)
			{
			case EDemandPagingFunction:
				TestPagedCode();
				break;
			case EDefaultFunction:
				// the default function is the stepping test functions
			case EStepFunction:
				{
				RMDebug_BranchTst1();

				// Single stepping test support code

				// ARM tests
				RMDebug_StepTest_Non_PC_Modifying();

				RMDebug_StepTest_Branch();

				RMDebug_StepTest_Branch_And_Link();

				RMDebug_StepTest_MOV_PC();

				RMDebug_StepTest_LDR_PC();
 
// thumb/interworking tests not supported on armv4
#ifdef __MARM_ARMV5__

				// Thumb tests
				RMDebug_StepTest_Thumb_Non_PC_Modifying();

				RMDebug_StepTest_Thumb_Branch();

				RMDebug_StepTest_Thumb_Branch_And_Link();

				RMDebug_StepTest_Thumb_Back_Branch_And_Link();

				// ARM <-> Thumb interworking tests
				RMDebug_StepTest_Interwork();

				RMDebug_StepTest_Thumb_AddPC();

#endif	// __MARM_ARMV5__
				
				// Single-stepping performance
				RMDebug_StepTest_Count();

				// multiple step test
				RMDebug_StepTest_ARM_Step_Multiple();

				TestData++;

				// Wait 50mSecs. // (suspends this thread)
				User::After(50000);

				break;
				}
			case EMultipleTraceCalls:
				TestMultipleTraceCalls();
				break;
			default:
				//do nothing
				break;
			}
		}

	delete cleanup;

	return (KErrNone);
	}

EXPORT_C TInt StartDebugThread(RThread& aDebugThread, const TDesC& aDebugThreadName)
//
// Starts a test thread
//
{
	TInt res=KErrNone;

	// Create the thread
	res = aDebugThread.Create(	aDebugThreadName,
								CDebugServThread::ThreadFunction,
								KDefaultStackSize,
								KDebugThreadDefaultHeapSize,
								KDebugThreadDefaultHeapSize,
								NULL
								);

	// Check that the creation worked
	if (res == KErrNone)
		{
		TRequestStatus rendezvousStatus;

		aDebugThread.SetPriority(EPriorityNormal);
		// Make a request for a rendezvous
		aDebugThread.Rendezvous(rendezvousStatus);
		// Set the thread as ready for execution
		aDebugThread.Resume();
		// Wait for the resumption
		User::WaitForRequest(rendezvousStatus);
		}                                 
	else
		{
		// Close the handle.
		aDebugThread.Close();
		}

	return res;
	}
