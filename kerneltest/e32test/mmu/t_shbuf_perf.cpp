// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/mmu/t_shbuf_perf.cpp
//
//

/**
 *  @file
 *
 *  Performance Testing of shared buffers.
 *
 *  Runs a number of tests using descriptors and RShBuf handles and compares
 *  the results to see the improvements in performance.
 */


#define __E32TEST_EXTENSION__

#include <e32def.h>
#include <e32test.h>
#include <e32debug.h>
#include <e32msgqueue.h>
#include <e32shbuf.h>
#include <hal.h>
#include <u32hal.h>
#include <e32svr.h>

#include "d_shbuf.h"
#include "t_shbuf_perfclient.h"


//
// Test name (and process name!)...
//
_LIT(KTestProcessName, "T_SHBUF_PERF");


/**
 *  Global test object (must be called 'test' to match some macros)...
 */
RTest  test(KTestProcessName);

//
// Number of iterations to run for each test. The timings are worked out by
// running the test X number of times and dividing the total time by X.
//
#ifdef _DEBUG
/**
 *  Number of iterations to run for each test (WINS/WINSCW/Target Debug).
 */
const TInt  KNumberOfIterations(50);      // Used for debuging and hence not measurement.
#else
#ifdef __WINS__
/**
 *  Number of iterations to run for each test (WINS/WINSCW Release).
 */
const TInt  KNumberOfIterations(5000);   // Proper emulator performance testing.
#else
/**
 *  Number of iterations to run for each test (Target Release).
 */
const TInt  KNumberOfIterations(500);   // Proper target performance testing.
#endif
#endif


TUint8  iClearCache[32768];


/**
 *  RShBuf performance test types.
 */
enum TRShBufPerfTest
	{
	/**
	 *  Send buffer from the client to the driver directly and back.
	 */
	ERShBufPerfTestClientToDriverReturn,
	
	/**
	 *  Send buffer from the client to the driver directly one way.
	 */
	ERShBufPerfTestClientToDriverOneWay,
	
	/**
	 *  Send buffer from the client to a second process to the driver and back.
	 */
	ERShBufPerfTestClientToProcessToDriverReturn,
	
	/**
	 *  Send buffer from the client to a second process to the driver one way.
	 */
	ERShBufPerfTestClientToProcessToDriverOneWay,
	
	/**
	 *  Read buffer from the driver directly and send it back.
	 */
	ERShBufPerfTestDriverToClientReturn,
	
	/**
	 *  Read buffer from the driver directly one way.
	 */
	ERShBufPerfTestDriverToClientOneWay,
	
	/**
	 *  Read buffer from the driver via a second process and send it back.
	 */
	ERShBufPerfTestDriverToProcessToClientReturn,
	
	/**
	 *  Read buffer from the driver via a second process one way.
	 */
	ERShBufPerfTestDriverToProcessToClientOneWay
	};


void StartSecondProcessAndDriver(TRShBufPerfTest aTestType,
								 RShBufTestChannel&  aLdd,
								 RShBufTestServerSession& aTestServer,
								 RThread& aTestServerThread,
								 TInt aDriverNum)
	{
	//
	// If a second process is needed start this process as a child...
	//
	if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn  ||
		aTestType == ERShBufPerfTestClientToProcessToDriverOneWay  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientReturn  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientOneWay)
		{
		test.Next(_L("Start slave server process..."));
		test_KErrNone(aTestServer.Connect());
		test.Next(_L("Find slave server thread..."));
		test_KErrNone(aTestServerThread.Open(_L("t_shbuf_perf.exe[00000000]0001::!RShBufServer")));
		}

	//
	// Open the driver (always open it as it is used to get buffers too!)...
	//
	TInt r = User::LoadLogicalDevice(_L("D_SHBUF_CLIENT.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);
	r = User::LoadLogicalDevice(_L("D_SHBUF_OWN.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);
	test_KErrNone(aLdd.Open(aDriverNum));
	} // StartSecondProcessAndDriver


void StopSecondProcessAndDriver(TRShBufPerfTest aTestType,
								RShBufTestChannel&  aLdd,
								RShBufTestServerSession& aTestServer,
								RThread& aTestServerThread)
	{
	//
	// Close the driver..
	//
	aLdd.Close();

	if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn  ||
		aTestType == ERShBufPerfTestClientToProcessToDriverOneWay  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientReturn  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientOneWay)
		{
#ifdef CAN_TRANSFER_SHBUF_TO_ANOTHER_PROCESS
		test.Next(_L("Stop slave server process..."));
		test_KErrNone(aTestServer.ShutdownServer());
#endif
		aTestServerThread.Close();
		aTestServer.Close();
		}
	} // StopSecondProcessAndDriver


/**
 *  Print the TRShBufPerfTest enum.
 */
void PrinTRShBufPerfTestType(const TDesC& aPrefix, TRShBufPerfTest aTestType)
	{
	switch (aTestType)
		{
		case ERShBufPerfTestClientToDriverReturn:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestClientToDriverReturn (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestClientToDriverOneWay:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestClientToDriverOneWay (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestClientToProcessToDriverReturn:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestClientToProcessToDriverReturn (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestClientToProcessToDriverOneWay:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestClientToProcessToDriverOneWay (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestDriverToClientReturn:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestDriverToClientReturn (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestDriverToClientOneWay:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestDriverToClientOneWay (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestDriverToProcessToClientReturn:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestDriverToProcessToClientReturn (%d)"), &aPrefix, aTestType);
			}
			break;
			
		case ERShBufPerfTestDriverToProcessToClientOneWay:
			{
			test.Printf(_L("%SaTestType=ERShBufPerfTestDriverToProcessToClientOneWay (%d)"), &aPrefix, aTestType);
			}
			break;
			
		default:
			{
			test.Printf(_L("%SaTestType=<unknown> (%d)"), &aPrefix, aTestType);
			}
			break;
		}
	} // PrinTRShBufPerfTestType


/**
 *  Print the TShPoolCreateInfo object.
 */
void PrintTShPoolInfo(const TDesC& aPrefix, TShPoolInfo aShPoolInfo)
	{
	test.Printf(_L("%SaShPoolInfo.iBufSize=%d"), &aPrefix, aShPoolInfo.iBufSize);
	test.Printf(_L("%SaShPoolInfo.iInitialBufs=%d"), &aPrefix, aShPoolInfo.iInitialBufs);
	test.Printf(_L("%SaShPoolInfo.iMaxBufs=%d"), &aPrefix, aShPoolInfo.iMaxBufs);
	test.Printf(_L("%SaShPoolInfo.iGrowTriggerRatio=%d"), &aPrefix, aShPoolInfo.iGrowTriggerRatio);
	test.Printf(_L("%SaShPoolInfo.iGrowByRatio=%d"), &aPrefix, aShPoolInfo.iGrowByRatio);
	test.Printf(_L("%SaShPoolInfo.iShrinkHysteresisRatio=%d"), &aPrefix, aShPoolInfo.iShrinkHysteresisRatio);
	test.Printf(_L("%SaShPoolInfo.iAlignment=%d (0x%x)"), &aPrefix, aShPoolInfo.iAlignment,
				2 << (aShPoolInfo.iAlignment - 1));
	test.Printf(_L("%SaShPoolInfo.iFlags=0x%08x"), &aPrefix, aShPoolInfo.iFlags);
	} // PrintTShPoolInfo


void TestSharedBufferPerformanceL(TRShBufPerfTest aTestType,
								  TInt aMinAllocSize, TInt aMaxAllocSize,
								  TInt aBufferSizeSteps,  TInt aTotalIterations,
								  TShPoolCreateFlags aFlags, TInt aDriverNum,
								  TDes& aSummaryBuf)
	{
	TShPoolInfo  shPoolInfo;

    shPoolInfo.iBufSize               = aMaxAllocSize;
    shPoolInfo.iInitialBufs           = 5;
	shPoolInfo.iMaxBufs               = 5;
	shPoolInfo.iGrowTriggerRatio      = 0;
	shPoolInfo.iGrowByRatio           = 0;
	shPoolInfo.iShrinkHysteresisRatio = 0;
	shPoolInfo.iAlignment             = 9;
	shPoolInfo.iFlags                 = aFlags;

	//
	// Start test and print the parameters...
	//
	test.Printf(_L(" Test parameters:"));
	PrinTRShBufPerfTestType(_L("  "), aTestType);
	PrintTShPoolInfo(_L("  "), shPoolInfo);
	test.Printf(_L("  aMinAllocSize=%d"), aMinAllocSize);
	test.Printf(_L("  aMaxAllocSize=%d"), aMaxAllocSize);
	test.Printf(_L("  aBufferSizeSteps=%d"), aBufferSizeSteps);
	test.Printf(_L("  aTotalIterations=%d"), aTotalIterations);
	test.Printf(_L("  aDriverNum=%d"), aDriverNum);

	//
	// Initialise second process and/or open the driver...
	//
	RShBufTestServerSession  testServer;
	RShBufTestChannel  shBufLdd;
	RThread  testServerThread;

	StartSecondProcessAndDriver(aTestType, shBufLdd, testServer, testServerThread, aDriverNum);
	CleanupClosePushL(testServer);
	
	//
	// Allocate a RShPool...
	//
	RShPool  shPool;
		
	if (aFlags & EShPoolPageAlignedBuffer)
		{
		TShPoolCreateInfo  shPoolCreateInfo(TShPoolCreateInfo::EPageAlignedBuffer,
		                                    shPoolInfo.iBufSize, shPoolInfo.iInitialBufs);
		test_KErrNone(shPool.Create(shPoolCreateInfo, KDefaultPoolHandleFlags));
		CleanupClosePushL(shPool);
		
		test_KErrNone(shPool.SetBufferWindow(-1, ETrue));
		shPoolInfo.iAlignment = 12;
		}
	else if (aFlags & EShPoolNonPageAlignedBuffer)
		{
		TShPoolCreateInfo  shPoolCreateInfo(TShPoolCreateInfo::ENonPageAlignedBuffer,
		                                    shPoolInfo.iBufSize, shPoolInfo.iInitialBufs,
				                            shPoolInfo.iAlignment);
		test_KErrNone(shPool.Create(shPoolCreateInfo, KDefaultPoolHandleFlags));
		CleanupClosePushL(shPool);
		}

	test(shPool.Handle() != 0);
	
	if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn  ||
		aTestType == ERShBufPerfTestClientToProcessToDriverOneWay  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientReturn  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientOneWay)
		{
		test_KErrNone(testServer.OpenRShBufPool(shPool.Handle(), shPoolInfo));
		}
	else
		{
		test_KErrNone(shBufLdd.OpenUserPool(shPool.Handle(), shPoolInfo));
		}
	
	//
	// Run the test iterations and time the result...
	//
	TInt fastTimerFreq;
	HAL::Get(HALData::EFastCounterFrequency, fastTimerFreq);
	TReal ticksPerMicroSec = 1.0E-6 * fastTimerFreq;

	// Bind this thread to CPU 0. This is so that timer deltas don't drift from
	// scheduling - else, it causes spurious failures.
    if (UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0) > 1)
	   (void)UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)0, 0);

	TReal64  totalLengthOfDesTest(0);
	TReal64  totalLengthOfShBufTest(0);
	TInt  breakevenPoint = 0;
	TInt  bufferStep;

	test.Printf(_L("BufSize\tTotalTime(Des)\tAvTime(Des)\tTotalTime(ShBuf)\tAvTime(ShBuf)\tSpeedUp(%%)"));
#ifndef __WINS__
	test.Printf(_L("\n"));
#endif
	for (bufferStep = 0;  bufferStep < aBufferSizeSteps;  bufferStep++)
		{
		//
		// Run a single buffer size through these tests...
		//
		TInt  bufferSize = aMinAllocSize +
						   (((aMaxAllocSize - aMinAllocSize) * bufferStep) / (aBufferSizeSteps-1));
		TUint32  startDesTest = 0;
		TUint32 startShBufTest = 0;
		TInt  iteration;

		TUint32  lengthOfDesTest=0;

		//
		// Test normal descriptor methods first...
		//

		for (iteration = 0;  iteration < aTotalIterations;  iteration++)
			{
			//
			// Allocate a local buffer for this test...
			//
			HBufC8*  singleBuf = HBufC8::NewLC(bufferSize);

			startDesTest = User::FastCounter();
			test(singleBuf != NULL);

			TPtr8 singleBufPtr = singleBuf->Des();
			singleBufPtr.SetLength(bufferSize);

			//
			// Are we sending or receiving?
			//
			if (aTestType == ERShBufPerfTestClientToDriverOneWay  ||
				aTestType == ERShBufPerfTestClientToProcessToDriverOneWay)
				{
#ifdef _DEBUG // do not cache
				TUint8* bufptr = const_cast<TUint8*>(singleBuf->Ptr());

				// We are sending...
				for (TInt pos = 0;  pos < bufferSize;  pos++)
					{
					bufptr[pos] = (TUint8)(pos%32);
					}
				// clear cache
				memset(iClearCache, 0xFF, sizeof(iClearCache));
#endif
				}


			//
			// Either send to the driver or to the other process...
			//
			if (aTestType == ERShBufPerfTestClientToDriverReturn)
				{
				test_KErrNone(shBufLdd.FromTPtr8ProcessAndReturn(singleBufPtr, bufferSize));
				test(singleBufPtr.Length() == bufferSize-2);
				}
			else if (aTestType == ERShBufPerfTestClientToDriverOneWay)
				{
				test_KErrNone(shBufLdd.FromTPtr8ProcessAndRelease(singleBufPtr));
				}
			else if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn)
				{
				test_KErrNone(testServer.FromTPtr8ProcessAndReturn(singleBufPtr, bufferSize));
				test(singleBufPtr.Length() == bufferSize-2);
				}
			else if (aTestType == ERShBufPerfTestClientToProcessToDriverOneWay)
				{
				test_KErrNone(testServer.FromTPtr8ProcessAndRelease(singleBufPtr));
				}

			lengthOfDesTest += (User::FastCounter() - startDesTest);

			CleanupStack::PopAndDestroy(singleBuf);
			}

		TInt64  lengthOfShBufTest = 0;

		//
		// Test ShBuf methods...
		//
		for (iteration = 0;  iteration < aTotalIterations;  iteration++)
			{
			RShBuf  shBuf;
			TInt*  lengthPtr;
			//
			// Are we sending or receiving?
			//
			startShBufTest = User::FastCounter();
			if (aTestType == ERShBufPerfTestClientToDriverOneWay ||
				aTestType == ERShBufPerfTestClientToProcessToDriverOneWay)
				{
				// We are sending...

				//
				// Allocate a buffer (using a pool)...
				//

				test_KErrNone(shBuf.Alloc(shPool));
				TUint8*  shBufPtr = shBuf.Ptr();

				lengthPtr = (TInt*)(&shBufPtr[0]); // First 32bit word is length!
				*lengthPtr = bufferSize;
#ifdef _DEBUG // do not cache
				for (TInt pos = 4;  pos < bufferSize;  pos++)
					{
					shBufPtr[pos] = (TUint8)(pos%32);
					}
				// clear cache
				memset(iClearCache, 0xFF, sizeof(iClearCache));
#endif
				}


			//
			// Either send to the driver or to the other process...
			//
			if (aTestType == ERShBufPerfTestClientToDriverReturn)
				{
				TInt retHandle;
				retHandle = shBufLdd.FromRShBufProcessAndReturn(bufferSize);
				test_Compare(retHandle, >, 0);
				shBuf.SetReturnedHandle(retHandle);

				TInt* retPtr = (TInt*)shBuf.Ptr();

				test(*retPtr == bufferSize-2);

				shBuf.Close();
				}
			else if (aTestType == ERShBufPerfTestClientToDriverOneWay)
				{
				test_KErrNone(shBufLdd.FromRShBufProcessAndRelease(shBuf.Handle()));
				}
			else if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn)
				{
				test_KErrNone(testServer.FromRShBufProcessAndReturn(shBuf, bufferSize));
				TInt* retPtr = (TInt*)shBuf.Ptr();

				test(*retPtr == bufferSize-2);

				shBuf.Close();
				}
			else if (aTestType == ERShBufPerfTestClientToProcessToDriverOneWay)
				{
				test_KErrNone(testServer.FromRShBufProcessAndRelease(shBuf));
				}
			lengthOfShBufTest +=  (User::FastCounter() - startShBufTest);
			}

		//
		// Print results of this buffer size...
		//

		test.Printf(_L("%d\t%10.2lfusec\t%10.2lfusec\t%.2f%%"), bufferSize,
					I64REAL(lengthOfDesTest) / (TReal(aTotalIterations) * ticksPerMicroSec),
					I64REAL(lengthOfShBufTest) / (TReal(aTotalIterations) * ticksPerMicroSec),
					((100.0 / I64REAL(lengthOfShBufTest)) * I64REAL(lengthOfDesTest)) - 100.0);
#ifndef __WINS__
		test.Printf(_L("\n"));
#endif
		
		totalLengthOfDesTest   += lengthOfDesTest;
		totalLengthOfShBufTest += lengthOfShBufTest;

		//
		// Track the breakeven point (e.g. the buffer size at which RShBuf is
		// quicker). This is normally when the number of bytes copied by the
		// descriptor takes longer than the handling of the RShBuf.
		//
		if (lengthOfShBufTest >= lengthOfDesTest)
			{
			breakevenPoint = aMinAllocSize +
						   (((aMaxAllocSize - aMinAllocSize) * (bufferStep + 1)) / (aBufferSizeSteps-1));
			}
		}

	//
	// Display timing information...
	//
	test.Printf(_L("Average\t%10.2lfusec\t%10.2lfusec\t%.2f%%"),
				I64REAL(totalLengthOfDesTest) / (TReal(aTotalIterations * aBufferSizeSteps) * ticksPerMicroSec),
				I64REAL(totalLengthOfShBufTest) / (TReal(aTotalIterations * aBufferSizeSteps) * ticksPerMicroSec),
				((100.0 / I64REAL(totalLengthOfShBufTest)) * I64REAL(totalLengthOfDesTest)) - 100.0);
#ifndef __WINS__
	test.Printf(_L("\n"));
#endif

	//
	// Record summary info for later use...
	//
	aSummaryBuf.Zero();
	
	if (breakevenPoint <= aMaxAllocSize)
		{
		aSummaryBuf.AppendFormat(_L("%10.2lfusec\t%10.2lfusec\t%.2f%%%%\t%d"),
								 I64REAL(totalLengthOfDesTest) / TReal(aTotalIterations * aBufferSizeSteps * ticksPerMicroSec),
								 I64REAL(totalLengthOfShBufTest) / TReal(aTotalIterations * aBufferSizeSteps * ticksPerMicroSec),
								 ((100.0 / I64REAL(totalLengthOfShBufTest)) * I64REAL(totalLengthOfDesTest)) - 100.0,
								 breakevenPoint);
		}
	else
		{
		aSummaryBuf.AppendFormat(_L("%10.2lfusec\t%10.2lfusec\t%.2f%%%%\tFailed to breakeven"),
								 I64REAL(totalLengthOfDesTest) / TReal(aTotalIterations * aBufferSizeSteps * ticksPerMicroSec),
								 I64REAL(totalLengthOfShBufTest) / TReal(aTotalIterations * aBufferSizeSteps * ticksPerMicroSec),
								 ((100.0 / I64REAL(totalLengthOfShBufTest)) * I64REAL(totalLengthOfDesTest)) - 100.0);
		}
	
	//
	// Clean up...
	//
	TInt  shPoolHandle = shPool.Handle();
	CleanupStack::PopAndDestroy(&shPool);

	if (aTestType == ERShBufPerfTestClientToProcessToDriverReturn  ||
		aTestType == ERShBufPerfTestClientToProcessToDriverOneWay  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientReturn  ||
		aTestType == ERShBufPerfTestDriverToProcessToClientOneWay)
		{
		testServer.CloseRShBufPool(shPoolHandle);
		}
	else
		{
		test_KErrNone(shBufLdd.CloseUserPool());
		}

	//
	// Shutdown the second process and/or close the driver.
	//
	CleanupStack::Pop(&testServer);
	StopSecondProcessAndDriver(aTestType, shBufLdd, testServer, testServerThread);
	} // TestSharedBufferPerformanceL


/**
 *  Main test process which performs the testing.
 */
void RunTestsL()
	{
	//
	// Setup the test...
	//
	test.Title();
	
	test.Start(_L("Check for Shared Buffers availability"));
	TInt r;
	RShPool pool;
	TShPoolCreateInfo inf(TShPoolCreateInfo::EPageAlignedBuffer, 100, 10);
	r = pool.Create(inf, KDefaultPoolHandleFlags);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Not supported by this memory model.\n"));
		}
	else
		{
		test_KErrNone(r);
		pool.Close();

		test.Next(_L("Performance test shared buffers"));

		//
		// Create a summary buffer to hold the average speeds of different pools...
		//
		HBufC*  summaryBuf = HBufC::NewLC(16 * 128 * 2);
		TPtr  summaryBufPtr = summaryBuf->Des();
		TBuf<128>  testName, testSummary;
		
		summaryBufPtr.Append(_L("Test Type\tAverage Time(Des)\tAverage Time(ShBuf)\tAverage SpeedUp(%%)\tBreakeven Buffer Size\n"));

		//
		// Run tests...
		//
		testName.Copy(_L("Client->Driver (non-aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver (aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver (non-aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver (aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver->Client (non-aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			ERShBufPerfTestClientToDriverReturn,
			/* Min Alloc size */	64,
			/* Max Alloc size */	8192,
			/* Buffer size steps */	128,
			/* Total iterations */	KNumberOfIterations,
			/* Buffer flags */      EShPoolNonPageAlignedBuffer,
			/* Driver to use */     RShBufTestChannel::EClientThread,
			/* Summary string */    testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver->Client (aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver->Client (non-aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Driver->Client (aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver (non-aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver (aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver (non-aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver (aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverOneWay,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver->Process->Client (non-aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver->Process->Client (aligned/client-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EClientThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver->Process->Client (non-aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			 ERShBufPerfTestClientToProcessToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolNonPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		testName.Copy(_L("Client->Process->Driver->Process->Client (aligned/own-thread)"));
		test.Next(testName);
		TestSharedBufferPerformanceL(
			/* Test type */			ERShBufPerfTestClientToProcessToDriverReturn,
			/* Min Alloc size */	 64,
			/* Max Alloc size */	 8192,
			/* Buffer size steps */	 128,
			/* Total iterations */	 KNumberOfIterations,
			/* Buffer flags */       EShPoolPageAlignedBuffer,
			/* Driver to use */      RShBufTestChannel::EOwnThread,
			/* Summary string */     testSummary);
		summaryBufPtr.AppendFormat(_L("%S\t%S\n"), &testName, &testSummary);

		//
		// Print the summary...
		//
		TInt  nextLineBreak = summaryBufPtr.Find(_L("\n"));
		
		test.Next(_L("Results summary (average values for each test)"));
		
		while (nextLineBreak != KErrNotFound)
			{
			test.Printf(summaryBufPtr.Left(nextLineBreak));
#ifndef __WINS__
			test.Printf(_L("\n"));
#endif

			summaryBufPtr = summaryBufPtr.Mid(nextLineBreak+1);
			nextLineBreak = summaryBufPtr.Find(_L("\n"));
			}
		CleanupStack::PopAndDestroy(summaryBuf);
		}
	test.End();
	test.Close();
	} // RunTestsL


/**
 *  Main entry point.
 */
TInt E32Main()
	{
	//
	// Allocate a clean up stack and top level TRAP...
	//
	__UHEAP_MARK;
	CTrapCleanup*  cleanup = CTrapCleanup::New();
	TInt  err = KErrNoMemory;
	
	if (cleanup)
		{
		TRAP(err, RunTestsL());
        delete cleanup;
		}
	
	__UHEAP_MARKEND;
	return err;
	} // E32Main

