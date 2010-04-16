/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This file contains statically defined test cases, a pointer to each
* new test case should be entered in StaticTestArray
*
*/

#include "t_dma2.h"
#include "cap_reqs.h"

const TCallbackRecord threadCallback(TCallbackRecord::EThread,1);
const TCallbackRecord isrCallback(TCallbackRecord::EIsr,1);

const TInt size = 128 * KKilo;
//--------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    Simple DMA transfer test using CSingleTransferTest and New DMA APIs
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Simple_1
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TResultSet expectedResults(threadCallback);

	CSingleTransferTest simpleTest(_L("Simple Test - New DMA APIs"), 1, transferArgs, expectedResults);

	TTestCase testCase(&simpleTest, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&simpleTest, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    Simple DMA transfer test using CSingleTransferTest and OLD DMA APIs
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.						
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Simple_2
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TResultSet expectedResults(threadCallback);

	CSingleTransferTest simpleTest = CSingleTransferTest(_L("Simple Test - Old DMA APIs"), 1, transferArgs, expectedResults, 0).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&simpleTest, EFalse);
	TTestCase testCaseConcurrent(&simpleTest, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2573
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA ISR Callback test (Isr Callback - use old request Ctor)
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Callback
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

	const TResultSet expectedResults(isrCallback);

	CSingleTransferTest isrTest(_L("Isr Callback"), 1, transferArgs, expectedResults);
	TTestCase testCase(&isrTest, EFalse, capAboveV1);


	const TRequestResults fragmentFails = TRequestResults().
		FragmentationResult(KErrArgument).
		QueueResult(KErrUnknown);

	const TResultSet expectedResultsFail = TResultSet(EFalse).
		ChannelOpenResult(KErrNone).
		RequestResult(fragmentFails).
		PostTransferResult(1); // PostTransferResult of 1 means buffers don't match

	CSingleTransferTest isrTestOldRequest = CSingleTransferTest(_L("Isr Callback - use old request Ctor"), 1, transferArgs, expectedResultsFail)
		.UseNewRequest(EFalse);
	TTestCase testCaseOldRequest(&isrTestOldRequest, EFalse, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2574,KBASE-DMA-2575
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA ISR Reque test
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace ISR_Reque
	{
	const TInt size = 4 * KKilo;
	TDmaTransferArgs tferArgs(0, 2*size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

	const TRequestResults requestResult(KErrNone, 1); // request must be in a single fragment

	namespace endOnIsrCb
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs(),
			TIsrRequeArgs(size,3*size,size,0, ETrue),
			TIsrRequeArgs(size,4*size,size,0, ETrue),
			TIsrRequeArgs(0,5*size,size,0, ETrue),
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		// we expect a cb for each requeue + 1 for the original
		// transfer
		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EIsr, count + 1).IsrRedoResult(KErrNone);
		const TResultSet expected(KErrNone, requestResult, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("4 Requeues - end on isr cb"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst), ETrue, capAboveV1);
		}

	namespace endOnThreadCb
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs(),
			TIsrRequeArgs(size,3*size,size,0, ETrue),
			TIsrRequeArgs(size,4*size,size,0, ETrue),
			TIsrRequeArgs(0,5*size,size,0, EFalse),
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EThread, count + 1).IsrRedoResult(KErrNone);
		const TResultSet expected(KErrNone, requestResult, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("4 Requeues - end on thread cb"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst), ETrue, capAboveV1);
		}

	namespace changeSize
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs(3*size,5*size,2*size,0, EFalse),
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EThread, count + 1).IsrRedoResult(KErrNone);
		const TResultSet expected(KErrNone, requestResult, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("1 Requeues - change transfer size"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst), ETrue, capAboveV1);
		}

	namespace endOnRedo
		{
		// TODO have made this bigger than 4k so that we don't miss the second interrupt when tracing enabled
		// this indicates the PSL's interrupt handler misses an interrupt if it occurs during the interrupt.
		const TInt size = 0x10000;
		TDmaTransferArgs tferArgs(0, 2*size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs(3*size,5*size,2*size,0, ETrue),
			TIsrRequeArgs() //repeat the previous transfer
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EIsr, count + 1).IsrRedoResult(KErrNone);
		const TResultSet expected(KErrNone, requestResult, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("2 Requeues - Isr redo request repeated"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst), EFalse, capAboveV1);
		}

	namespace invalidAddresses
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs(size, size)
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EIsr, 1).IsrRedoResult(KErrArgument);
		const TResultSet expected(KErrNone, requestResult, KErrUnknown, callbackRecord);

		// pre and post test would fail because of bad requeue parameters
		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("Requeue with matching addresses"), 1, tferArgs, requeArgs, count, expected, NULL, NULL), ETrue, capAboveV1);
		}

	namespace multipleFragments
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs()
		};
		const TInt count = ARRAY_LENGTH(requeArgs);

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EThread, count + 1).IsrRedoResult(KErrNone);

		TRequestResults results2Fragments = TRequestResults(requestResult).FragmentCount(2);
		const TResultSet expected(KErrNone, results2Fragments, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("Attempt to Requeue 2 fragment request"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst, size/2), ETrue, capAboveV1);

		}
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-DMA-FUNC-xxx
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA Multiple transfer test
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Multipart
	{
	// need long transfer, to try and force adjacent
	// requests to be concatinated
	const TInt size = 2 * KMega;
	const TDmaTransferArgs transferArgArray[] = {
		TDmaTransferArgs(0, size, size, KDmaMemAddr),
		TDmaTransferArgs(size, 2 * size, size, KDmaMemAddr)
	};

	const TResultSet expected[] =
		{
		TResultSet(),
		TResultSet()
		};
	const TResultSet expectedResults(isrCallback);

	CMultiTransferTest multipart =
		CMultiTransferTest(_L("Sg request concatination"), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
			.SetPreTransferTest(&KPreTransferIncrBytes)
			.SetPostTransferTest(&KCompareSrcDst);

	TTestCase testCase(&multipart, EFalse, hwDesWanted_skip);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2580
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    These tests attempt to queue ISR cb requests while the queue is not 
//!						empty and queing normal requests when an ISR cb is pending
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace IsrAndDfc
	{
	// need long transfer, so that 1st request is still queued
	// when the second one is queued
	// TODO pause is the better way to ensure this
	//const TInt size = 2 * KMega;
	//TODO have changed size to ensure that the first isr callback request in IsrBeforeDfc
	//will only have one fragment
	const TInt size = 0x40000;
	TDmaTransferArgs dfcTransfer(0, size, size, KDmaMemAddr);
	TDmaTransferArgs isrTransfer(2*size, 3*size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

	const TResultSet success = TResultSet();
	TResultSet queueFailure = TResultSet().
			RequestResult(TRequestResults().QueueResult(KErrGeneral)).
			CallbackRecord(TCallbackRecord::Empty()).
			PostTransferResult(1);

	namespace DfcBeforeIsr
		{
		const TDmaTransferArgs transferArgArray[] = {
			dfcTransfer,
			isrTransfer
		};

		const TResultSet expected[] =
			{
			success,
			queueFailure
			};
		CMultiTransferTest dfcBeforeIsr =
			CMultiTransferTest(_L("DFC cb req before ISR cb req "), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
				.SetPreTransferTest(&KPreTransferIncrBytes)
				.SetPostTransferTest(&KCompareSrcDst);
		TTestCase testCase(&dfcBeforeIsr, EFalse, hwDesWanted_skip);
		}

	namespace IsrBeforeDfc
		{
		const TDmaTransferArgs transferArgArray[] = {
			isrTransfer,
			dfcTransfer
		};

		TResultSet isrSuccess = TResultSet(success).CallbackRecord(isrCallback);
		const TResultSet expected[] =
			{
			isrSuccess,
			queueFailure
			};
		CMultiTransferTest dfcBeforeIsr =
			CMultiTransferTest(_L("ISR cb req before DFC cb req "), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
				.SetPreTransferTest(&KPreTransferIncrBytes)
				.SetPostTransferTest(&KCompareSrcDst);
		TTestCase testCase(&dfcBeforeIsr, EFalse, hwDesWanted_skip);
		}

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-DMA-FUNC-xxx
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA 2D transfer test
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace _2D_Test
	{
	// Image @ 0x0 with 640x480 pixels and 24 bits/pixel.

	TDmaTransferConfig src(
		0, /*iAddr*/
		3, /*iElementSize*/
		6, /*iElementsPerFrame*/
		4, /*iFramesPerTransfer*/
		0, /*iElementSkip*/
		0, /*iFrameSkip*/
		KDmaMemAddr /*iFlags*/
		);

	TDmaTransferConfig dst(
		0x708000, /*iAddr*/
		3, /*iElementSize*/
		640, /*iElementsPerFrame*/
		480, /*iFramesPerTransfer*/
		1437, /*iElementSkip*/
		-920166, /*iFrameSkip*/
		KDmaMemAddr /*iFlags*/
		);

	TDmaTransferArgs transferArgs2D(src, dst);

	TResultSet expectedResults; //all KErrNone

	//source buffer is currently filled with increasing values
	//instead of an image, but the test is still valid
	CSingleTransferTest transfer2d(_L("2D Transfer"), 1, transferArgs2D, expectedResults, 0, &KCompare2D);

	TTestCase testCase2d(&transfer2d, EFalse, cap_2DRequired, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2565
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA Fragmentation count test
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace FragmentationCount
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 128);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);
	CSingleTransferTest test1(_L("Fragmentation Count - 128 fragments"), 1, transferArgs, expectedResults, KKilo);
	TTestCase testCase(&test1, EFalse);

	const TRequestResults requestResult2(KErrNone, 4);
	const TResultSet expectedResults2(KErrNone, requestResult2, KErrNone, threadCallback);
	CSingleTransferTest test2(_L("Fragmentation Count - 4 fragments"), 1, transferArgs, expectedResults2, 32*KKilo);
	TTestCase testCase2(&test2, EFalse);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2584,KBASE-DMA-2585
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    DMA Benchmark tests
//!
//! @SYMTestActions     
//!						1.
//!						2.	
//!
//!
//! @SYMTestExpectedResults 
//!						1.  
//!						2.		
//!							
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//-----------------------------------------------------------------------------------------------
namespace Benchmark
	{
	const TInt bmIters = 10;
	namespace Frag
		{
		const TInt size = 1 * KMega;
		TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

		TTestCase testCase_256k(new (ELeave) CDmaBmFragmentation(_L("1 Mb transfer - 256k frag size"), bmIters, transferArgs, 256 * KKilo), EFalse);
		TTestCase testCase_8k(new (ELeave) CDmaBmFragmentation(_L("1 Mb transfer - 8k frag size"), bmIters, transferArgs, 8 * KKilo), EFalse);
		}

	namespace Transfer
		{
		namespace _4Bytes
			{
			const TInt size = 4;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

			CDmaBmTransfer bmTest(_L("4 bytes"), bmIters, transferArgs, 0);
			TTestCase testCase(&bmTest, EFalse);
			}
		namespace _128K
			{
			const TInt size = 128 * KKilo;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

			TTestCase testCase_128(new (ELeave) CDmaBmTransfer(_L("128 K - 128K frag size"), bmIters, transferArgs, 128 * KKilo), EFalse);
			TTestCase testCase_16(new (ELeave) CDmaBmTransfer(_L("128 K - 16k frag size"), bmIters, transferArgs, 16 * KKilo), EFalse);
			TTestCase testCase_4(new (ELeave) CDmaBmTransfer(_L("128 K - 4k frag size"), bmIters, transferArgs, 4 * KKilo), EFalse);
			TTestCase testCase_1(new (ELeave) CDmaBmTransfer(_L("128 K - 1k frag size"), bmIters, transferArgs, 1 * KKilo), EFalse);
			}
		namespace _4Mb
			{
			const TInt size = 4 * KMega;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

			CDmaBmTransfer bmTest(_L("4 Mb"), bmIters, transferArgs, 0);
			TTestCase testCase(&bmTest, EFalse);
			}
		}

	/**
	Compare time taken between queing and callback of 4 byte
	request with both DFC and ISR callback
	The new API calls are used
	*/
	namespace CompareIsrDfcCb
		{
		const TInt iterations = 50;

		namespace Dfc
			{
			TResultSet expected = TResultSet(threadCallback).
				PostTransferResult(KErrUnknown);

			namespace _4Bytes
				{
				const TInt size = 4;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4 bytes DFC cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse);
				}
			namespace _4K
				{
				const TInt size = 4 * KKilo;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4K DFC cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse);
				}
			}

		namespace Isr
			{
			TResultSet expected = TResultSet(isrCallback).
				PostTransferResult(KErrUnknown);

			namespace _4Bytes
				{
				const TInt size = 4;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4 bytes Isr cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse);
				}
			namespace _4K
				{
				const TInt size = 4 * KKilo;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4K Isr cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse);
				}
			}
		}
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 1 - DstAddr > SrcAddr & TransferSize=32K & Location is 
//!						address of a memory buffer
//! @SYMTestActions     
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 4 * KKilo;
//!							desAddr		 = 64 * KKilo;
//!							transferSize = 32 * KKilo;	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework					
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_1
	{	
	const TInt srcAddr = 4 * KKilo;
	const TInt desAddr = 64 * KKilo;

	const TInt transferSize =  32 * KKilo;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 32); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_1(_L("TestNewStyleFragment - Test Scenario 1"), 1, transferArgs, expectedResults,KKilo);

	TTestCase testCase(&testscenario_1, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_1, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 2 -  SrcAddr	== DstAddr   					
//!		
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr	 = 4 * KKilo;
//!							desAddr	 = 4 * KKilo;
//!							transferSize = 32 * KKilo;	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework					
//!						3.	Fragment passes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_2
	{
	const TInt srcAddr = 4 * KKilo;
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize =  32 * KKilo;

	TDmaTransferArgs transferArgs(srcAddr,desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 32); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_2(_L("TestNewStyleFragment - Test Scenario 2"), 1, transferArgs, expectedResults,KKilo);

	TTestCase testCase(&testscenario_2, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_2, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 3 -  TransferSize=0   
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 32 * KKilo;
//!							desAddr		 = 64 * KKilo;
//!							transferSize = 0	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request fails and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_3
	{
	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 0;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize,KDmaMemAddr);
	const TRequestResults requestResult(KErrArgument, 0); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_3(_L("TestNewStyleFragment - Test Scenario 3"), 1, transferArgs, expectedResults);

	TTestCase testCase(&testscenario_3, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_3, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 4 -  TransferSize=1Byte   
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 32K;
//!							desAddr		 = 64K;
//!							transferSize = 1 byte	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_4
	{	
	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 1;
	
	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 1);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_4(_L("TestNewStyleFragment - Test Scenario 4"), 1, transferArgs, expectedResults);

	TTestCase testCase(&testscenario_4, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_4, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 5 -  TransferSize=128KB    
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 16K;
//!							desAddr		 = 2MB;
//!							transferSize = 1MB;
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_5
	{
	
	const TInt srcAddr		= 16 * KKilo;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_5(_L("TestNewStyleFragment - Test Scenario 5"), 1, transferArgs, expectedResults);

	TTestCase testCase(&testscenario_5, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_5, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2560
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleFragment using CSingleTransferTest
//!						Test Scenario 6 -  TransferSize=3MB   
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 16K;
//!							desAddr		 = 4MB;
//!							transferSize = 3MB 	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleFragment_6
	{
	const TInt srcAddr = 16 * KKilo;
	const TInt desAddr = 4 * KMega;
	const TInt transferSize = 3 * KMega;

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_6(_L("TestNewStyleFragment - Test Scenario 6"), 1, transferArgs, expectedResults);

	TTestCase testCase(&testscenario_6, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_6, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 1 - DstAddr > SrcAddr & TransferSize=32K & Location is 
//!						address of a memory buffer
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 4 * KKilo;
//!							desAddr		 = 64 * KKilo;
//!							transferSize = 32 * KKilo;	
//!							iFlags		 = KDmaMemAddr;

//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework					
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_1
	{	
	const TInt srcAddr = 4 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize =  32 * KKilo;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone,32); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_1 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 1"), 1, transferArgs, expectedResults,KKilo).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_1, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_1, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 2 - DstAddr == SrcAddr
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr	 = 4 * KKilo;
//!							desAddr	 = 4 * KKilo;
//!							transferSize = 4 * KKilo
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment passes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_2
	{
	const TInt srcAddr = 4 * KKilo;
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize =  4 * KKilo;

	TDmaTransferArgs transferArgs(srcAddr,desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 4);  
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_2 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 2"), 1, transferArgs, expectedResults,KKilo)
		.UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_2, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_2, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 3 -  TransferSize=0  
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr	 = 32K
//!							desAddr	 = 64K;
//!							transferSize = 0
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request Fails and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_3
	{

	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 0;
	
	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize,KDmaMemAddr);
	const TRequestResults requestResult(KErrArgument, 0); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_3 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 3"), 1, transferArgs, expectedResults).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_3, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_3, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 4 -  TransferSize=1Byte   
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!	
//!							SrcAddr		 = 32K;
//!							desAddr		 = 64K;
//!							transferSize = 1 byte	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//------------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_4
	{	
	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 1;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 1); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_4 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 4"), 1, transferArgs, expectedResults).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_4, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_4, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 5 -  TransferSize=1MB
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr		 = 16K;
//!							desAddr		 = 2MB;
//!							transferSize = 1MB	
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_5
	{
	const TInt srcAddr = 16 * KKilo;	
	const TInt desAddr = 2 * KMega;
	const TInt transferSize = 1 *  KMega;

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_5 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 5"), 1, transferArgs, expectedResults).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_5, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_5, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2561
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldstyleFragment using CSingleTransferTest
//!						Test Scenario 6 -  TransferSize=3MB     
//!
//!						1.	Set up the arguments for aTransfeArgs using the settings below.
//!							
//!							SrcAddr	 = 16K
//!							desAddr	 = 4MB;
//!							transferSize = 3MB  
//!							iFlags		 = KDmaMemAddr;
//!
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestOldStyleFragment_6
	{
	const TInt srcAddr = 16 * KKilo;
	const TInt desAddr = 4 * KMega;
	const TInt transferSize = 3 * KMega; 
	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_6 = CSingleTransferTest(_L("TestOldStyleFragment - Test Scenario 6"), 1, transferArgs, expectedResults).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_6, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_6, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2562
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldStyleDDmaRequest using CSingleTransferTest
//!						Test Scenario 1 -  aMaxTransferSize=0 
//!
//!						1.	Set up the DDmaRequest using  aMaxTransferSize set to 0. 
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//------------------------------------------------------------------------------------------------
namespace TestOldStyleDDmaRequest_1
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 0); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_1 = CSingleTransferTest(_L("TestOldStyleDDmaRequest - Test Scenario 1"), 1, transferArgs, expectedResults,0).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_1, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_1, ETrue, capAboveV1);
	}

//!-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID       KBASE-DMA-2562
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestOldStyleDDmaRequest using CSingleTransferTest
//!						Test Scenario 2 -  aMaxTransferSize= 65535   
//!
//!						1.	Set up the arguments for DDmaRequest using aMaxTransferSize set to 65535.
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------------------------------------------------------------
namespace TestOldStyleDDmaRequest_2
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 1); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_2 = CSingleTransferTest(_L("TestOldStyleDDmaRequest - Test Scenario 2"), 1, transferArgs, expectedResults, 65535).
		UseNewDmaApi(EFalse);

	TTestCase testCase(&testscenario_2, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_2, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2563
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleDDmaRequest using CSingleTransferTest
//!						Test Scenario 1 -  aMaxTransferSize=0 
//!
//!						1.	Set up the DDmaRequest using  aMaxTransferSize set to 0. 
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace TestNewStyleDDmaRequest_1
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 0); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_1(_L("TestNewStyleDDmaRequest - Test Scenario 1"), 1, transferArgs, expectedResults,0);

	TTestCase testCase(&testscenario_1, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_1, ETrue, capAboveV1);
	}

//!-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-DMA-2563
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    TestNewStyleDDmaRequest using CSingleTransferTest
//!						Test Scenario 2 -  aMaxTransferSize= 65535   
//!
//!						1.	Set up the arguments for DDmaRequest using aMaxTransferSize set to 65535.
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! @SYMTestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------------------------------------------------------------
namespace TestNewStyleDDmaRequest_2
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 1); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CSingleTransferTest testscenario_2(_L("TestNewStyleDDmaRequest - Test Scenario 2"), 1, transferArgs, expectedResults, 65535);

	TTestCase testCase(&testscenario_2, EFalse, capAboveV1);
	TTestCase testCaseConcurrent(&testscenario_2, ETrue, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-DMA-FUNC-xxx
//! @SYMTestType        CIT
//! @SYMPREQ            REQ
//! @SYMTestCaseDesc    SmallFrags: This test provokes the failure seen in DEF140598
//!						The test checks that requests with small fragments
//!						do not trigger a spurious missed interrupt clean up
//!
//! @SYMTestExpectedResults 
//!
//!						1.  		
//!						2.	
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace SmallFrags
	{
	const TInt size = 32;
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TResultSet expectedResults(threadCallback);

	TTestCase testCase(
			new (ELeave) CSingleTransferTest(_L("8 * 4byte frags"), 10, transferArgs, expectedResults, 4),
			EFalse, capAboveV1);
	}


//TODO TTestCase could automatically be added to aray by ctor
//
//Append new test cases here
static TTestCase* StaticTestArray[] = {
	&Simple_1::testCase,
	&Simple_1::testCaseConcurrent,
	&Simple_2::testCase,
	&Simple_2::testCaseConcurrent,
	&Callback::testCase,
	&Callback::testCaseOldRequest,
	&ISR_Reque::endOnRedo::testCase,
	&ISR_Reque::endOnIsrCb::testCase,
	&ISR_Reque::endOnThreadCb::testCase,
	&ISR_Reque::changeSize::testCase,
#ifdef _DEBUG
	&ISR_Reque::invalidAddresses::testCase, // addresses only checked in UDEB
#endif
	//&ISR_Reque::multipleFragments::testCase, // This error condition is currently caught by a FAULT instead of a return code
	&Multipart::testCase,
	&IsrAndDfc::DfcBeforeIsr::testCase,
	&IsrAndDfc::IsrBeforeDfc::testCase,
	&_2D_Test::testCase2d,
	&FragmentationCount::testCase,
	&FragmentationCount::testCase2,
	&SmallFrags::testCase,
#ifndef _DEBUG
	// Benchmarks are only really meaningful
	// on UREL builds
	&Benchmark::Frag::testCase_256k,
	&Benchmark::Frag::testCase_8k,
	&Benchmark::Transfer::_128K::testCase_128,
	&Benchmark::Transfer::_128K::testCase_16,
	&Benchmark::Transfer::_128K::testCase_4,
	&Benchmark::Transfer::_128K::testCase_1,
	&Benchmark::Transfer::_4Bytes::testCase,
	&Benchmark::Transfer::_4Mb::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4K::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4K::testCase,
#endif
	&TestNewStyleFragment_1::testCase,
	&TestNewStyleFragment_1::testCaseConcurrent,
	&TestNewStyleFragment_2::testCase,
	&TestNewStyleFragment_2::testCaseConcurrent,
	//&TestNewStyleFragment_3::testCase,
	//&TestNewStyleFragment_3::testCaseConcurrent,
	&TestNewStyleFragment_4::testCase,
	&TestNewStyleFragment_4::testCaseConcurrent,
	&TestNewStyleFragment_5::testCase,
	&TestNewStyleFragment_5::testCaseConcurrent,
	&TestNewStyleFragment_6::testCase,
	&TestNewStyleFragment_6::testCaseConcurrent,
	&TestOldStyleFragment_1::testCase,
	&TestOldStyleFragment_1::testCaseConcurrent,
	&TestOldStyleFragment_2::testCase,
	&TestOldStyleFragment_2::testCaseConcurrent,
	//&TestOldStyleFragment_3::testCase,
	//&TestOldStyleFragment_3::testCaseConcurrent,
	&TestOldStyleFragment_4::testCase,
	&TestOldStyleFragment_4::testCaseConcurrent,
	&TestOldStyleFragment_5::testCase,
	&TestOldStyleFragment_5::testCaseConcurrent,
	&TestOldStyleFragment_6::testCase,
	&TestOldStyleFragment_6::testCaseConcurrent,
	&TestOldStyleDDmaRequest_1::testCase,
	&TestOldStyleDDmaRequest_1::testCaseConcurrent,
	&TestOldStyleDDmaRequest_2::testCase,
	&TestOldStyleDDmaRequest_2::testCaseConcurrent,
	&TestNewStyleDDmaRequest_1::testCase,
	&TestNewStyleDDmaRequest_1::testCaseConcurrent,
	&TestNewStyleDDmaRequest_2::testCase,
	&TestNewStyleDDmaRequest_2::testCaseConcurrent,
};

RPointerArray<TTestCase> TestArray(StaticTestArray, ARRAY_LENGTH(StaticTestArray));
