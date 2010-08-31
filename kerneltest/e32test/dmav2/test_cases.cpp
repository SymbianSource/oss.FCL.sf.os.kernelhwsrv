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
const TInt trans64K_1 = ((64 * KKilo) -1); // 65535

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2571
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies the correct behavior of CancelAll API in the new DMA 
//!					framework 
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.  Queue multiple request on the DMA channel.
//!						3.	Call CancelAll () on the DMA channel.
//!						4.  Verify that all transfers have been cancelled.
//!						5   Open a DMA channel for a transfer. This channel should support pause and resume.
//!						6.  Call Pause () on the channel.
//!						7.	Queue multiple request on the DMA channel.
//!						8.	Call CancelAll () on the channel.
//!						9.	Verify that all transfers have been cancelled.
//!
//!
//! TestExpectedResults 1.  DMA channel opens and KErrNone returned.
//!						2.  DMA queue request created and KErrNone returned.
//!						3.  CancelAll () cancels all queued request.
//!						4.  All queued request are cancelled
//!						5.	DMA Request completes
//!
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace CancelAllTest
	{
	const TInt size = 2 * KMega;
	const TDmaTransferArgs transferArgArray[] = {
		TDmaTransferArgs(0, size, size, KDmaMemAddr),
		TDmaTransferArgs(2 * size, 3 * size, size, KDmaMemAddr)
	};

	TResultSet noTransferExpected = TResultSet()
		.PostTransferResult(KErrNone)
		.CallbackRecord(TCallbackRecord::Empty());

	const TResultSet expected[] =
		{
		TResultSet(noTransferExpected),
		TResultSet(noTransferExpected)
		};

	// Test that 2 requests can be queued on a paused channel
	// then cleared using CancelAll.
	// It is expected that no data will have been transferred to the
	// destination buffer.
	CCancelAllTest testcancelall =
		CCancelAllTest(_L("CancelAllTest : Cancel and verify cancellation of requests"), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
			.SetPreTransferTest(&KPreTransferIncrBytes)
			.SetPostTransferTest(&KCheckNoTransfer);

	TTestCase testCase(&testcancelall,EFalse,capAboveV1,pauseRequired_skip);
	}

//--------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2569
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies the correct behavior of Pause and Resume API in the new DMA 
//!					framework 
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.  Setup a DMA request and time how long the transfer takes
//!						3.	Pause the DMA channel
//!						4.  Repeat DMA transfer (queued asynchronously)
//!						5.  Resume DMA Channel
//! 
//!	TestExpectedResults 1.  DMA channel opens and KErrNone returned.
//!						2.  DMA request created and KErrNone returned.
//!						3.  DMA channel Paused.
//!						4.  Request queued and waits until Resume is called
//!						5.	DMA Request completes
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace PauseResumeApiTest
	{
	const TInt srcAddr		= 0;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;	

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TResultSet expectedResults(threadCallback);
	CPauseResumeTest testPauseResume = CPauseResumeTest(_L("Pause and Resume Test"), 1, transferArgs, expectedResults); 
	TTestCase testCasePauseResume(&testPauseResume, EFalse, capAboveV1,pauseRequired_skip);	
	}
//--------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2572
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies the correct behavior of Pause and Resume API in the new DMA 
//!					framework 
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.  Pause and Resume DMA channel.
//!						3.  Close DMA channel.
//!					
//! 
//!	TestExpectedResults 1.  DMA channel opens and KErrNone returned.
//!						2.  KErrNotSupported returned when Pause and Resume API are called.
//!						3.	DMA channel closes and KErrNone returned.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace PauseResumeApiNegTest
	{
	const TInt srcAddr		= 0;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;	

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);	

	TResultSet noTransferExpected = TResultSet().
		RequestResult(TRequestResults().CreationResult(KErrUnknown).FragmentationResult(KErrUnknown).QueueResult(KErrUnknown)).
		CallbackRecord(TCallbackRecord::Empty()).
		PostTransferResult(KErrUnknown);

	CPauseResumeNegTest testPauseResumeNeg = CPauseResumeNegTest(_L("Pause and Resume Negative Test"), 1, transferArgs, noTransferExpected); 
	TTestCase testCasePauseResumeNeg(&testPauseResumeNeg , EFalse, capAboveV1,pauseNotWanted);	
	}
//--------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Simple DMA transfer test using CSingleTransferTest and New DMA APIs
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.	Create single transfer test and run test
//!				
//!
//!
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.	DMA transfer completes with no errors.
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Simple_1
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TResultSet expectedResults(threadCallback);

	CSingleTransferTest simpleTest(_L("Simple Test - New DMA APIs"), 1, transferArgs, expectedResults);

	TTestCase testCase(new (ELeave) CMultiVersionTest(&simpleTest), EFalse);
	TTestCase testCaseConcurrent(new (ELeave) CMultiVersionTest(&simpleTest), ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2573
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA ISR Callback test (Isr Callback - use old request Ctor)
//!
//! TestActions     
//!						1.	Setup DMA transfer to request a callback from ISR using old style DDmaRequest.
//!						2.	Create single transfer test and run test
//!						3.	Verify that DDmaRequest request fails
//!
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.	DMA transfer created with no errors.
//!						3.	DDmaRequest request fails. 
//!						
//!
//! TestPriority        High
//! TestStatus          Implemented
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
//! TestCaseID      KBASE-DMA-2574,KBASE-DMA-2575
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA ISR Reque test
//!
//! TestActions     
//!					1.	Setup DMA transfer to request a callback from ISR.
//!					2.	Set up ISR to requeue the DMA request on the same channel and adjust its transfer parameters.
//!					3.	Create single transfer test and run test
//!					4.	Verify a client can queue the just completed DMA request from within an ISR callback.
//!
//! TestExpectedResults 
//!					
//!					DMA transfer completes and just completed request can be requeued from within an ISR 
//!					callback on the same channel. Requeued DMA request completes successfully
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace ISR_Reque
	{
	const TInt size = 4 * KKilo;
	TDmaTransferArgs tferArgs(0, 2*size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

	const TRequestResults requestResult(KErrNone, 1); // request must be in a single fragment

	//-------------------------------------------------------------
	//This case requeues 4 transfers at the end of an ISR callback
	//-------------------------------------------------------------
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

	//---------------------------------------------------------------
	///This case requeues 4 transfers at the end of a thread callback
	//---------------------------------------------------------------
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
	
	//----------------------------------------------------------------------------------------------
	// This case requeues a transfer from within an thread callback after changing the transfer size
	//----------------------------------------------------------------------------------------------
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

	//--------------------------------------------------------------------------------------------
	// This case requeues a just completed request from within an ISR callback 
	//--------------------------------------------------------------------------------------------
	namespace endOnRedo
		{
		// The transfer size has been made bigger than 4K so that we do not miss the second interrupt when tracing 
		// enabled is. This indicates the PSL's interrupt handler misses an interrupt if it occurs during the interrupt.
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

	//--------------------------------------------------------------------------------------------
	// This case requeues a request from within an ISR callback using invalid requeue parameters
	//--------------------------------------------------------------------------------------------
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
	//--------------------------------------------------------------------------------------------
	// This case requeues a request containing more than 1 fragment from within an ISR callback
	// 
	//		
	// This test case is currently caught by a FAULT instead of a return code
	// as expected. Currently, the facility to return an error code to the test
	// framework is not yet supported.
	//
	// It has been implemented to expect an error code of KErrGeneral as requeues for request that
	// contains more than one fragments are not allowed. 
	//--------------------------------------------------------------------------------------------
	namespace multipleFragments
		{
		TIsrRequeArgs requeArgs[] = {
			TIsrRequeArgs()
		};
		const TInt count = ARRAY_LENGTH(requeArgs);
		const TRequestResults isrequestResult(KErrNone, 2); // request contains 2 fragments

		const TCallbackRecord callbackRecord = TCallbackRecord(TCallbackRecord::EIsr, 1).IsrRedoResult(KErrGeneral);
		TRequestResults results2Fragments = TRequestResults(isrequestResult).FragmentCount(2); 
		const TResultSet expected(KErrNone, results2Fragments, KErrNone, callbackRecord);

		TTestCase testCase(new (ELeave) CIsrRequeTest(_L("Attempt to Requeue 2 fragment request"), 1, tferArgs, requeArgs, count, expected, &KPreTransferIncrBytes, &KCompareSrcDst, size/2), ETrue, capAboveV1);

		}
	}	

//----------------------------------------------------------------------------------------------
//! TestCaseID      PBASE-DMA-FUNC-2586
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA Multiple transfer test
//! TestActions     
//!						1.Setup DMA transfer to using muliple transfers.
//!						2.Create multipart transfer test and run test.	
//!
//!
//! TestExpectedResults 
//!						1.  DMA tranfer set up with no errors. 
//!						2.	Multipart transfer tests completes with no issues.		
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Multipart
	{
	// need long transfer, to try and force adjacent
	// requests to be concatinated
	const TInt size = 2 * KMega;
	const TDmaTransferArgs transferArgArray[] = {
		TDmaTransferArgs(0, size, size, KDmaMemAddr),
		TDmaTransferArgs(2 * size, 3 * size, size, KDmaMemAddr)
	};

	const TResultSet expected[] =
		{
		TResultSet(),
		TResultSet()
		};

	CMultiTransferTest multipart =
		CMultiTransferTest(_L("Sg request concatination"), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
			.SetPreTransferTest(&KPreTransferIncrBytes)
			.SetPostTransferTest(&KCompareSrcDst);

	TTestCase testCase(&multipart, EFalse, hwDesWanted_skip);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2580
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    These tests attempt to queue ISR cb requests while the queue is not 
//!					empty and queing normal requests when an ISR cb is pending
//! TestActions     
//!				
//!					1.	Setup DMA transfer to request a callback from ISR.
//!					2.	Create single transfer test and run test
//!					3.	Queue another request using Queue()before ISR Callback completion
//!					4.	Verify a DMA framework flags an error.
//!
//!
//! TestExpectedResults 
//!					DMA framework flags an error  if ISR callback has not been executed 
//!					for the last time without re-queueing the initial transfer request		
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace IsrAndDfc
	{
	// This test case needs a long transfer so that 1st request is still queued when the second 
	// one is queued. The use of Pause is recommended in order to guarantee this. For this case, 
	// the size has been selected to ensure that the first isr callback request in IsrBeforeDfc
	// will only have one fragment
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
				.SetPostTransferTest(&KCompareSrcDst)
				.PauseWhileQueuing();
		TTestCase testCase(&dfcBeforeIsr, EFalse, pauseRequired_skip, capAboveV1);
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
				.SetPostTransferTest(&KCompareSrcDst)
				.PauseWhileQueuing();
		TTestCase testCase(&dfcBeforeIsr, EFalse, pauseRequired_skip, capAboveV1);
		}

	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      PBASE-DMA-FUNC-2587
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA 2D transfer test
//!
//! TestActions     
//!						1. Set up DMA transfer using TDmaTransArgs parameters specified in order to
//!						   rotate an image by 90 degrees clockwise.
//!						2. Verify that destination buffer gets transformed according to the DMA
//!						   transfer arguments after transfer.
//!
//! TestExpectedResults 
//!						1. DMA tranfer set up with no errors. 
//!						2. Destination buffer gets transformed accordingly.
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
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
//! TestCaseID      KBASE-DMA-2565
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA Fragmentation count test
//!
//! TestActions     
//!					1.	Set up the arguments for Fragment using the setting in the test scenario table below.
//!					2.	Create single transfer test and run test
//!					3.	Verify that FragmentCount API returns the expected value and framework responds correctly
//!					4.	Repeat steps 1 to 3 above until all the settings in the scenario table below have been used.
//!
//! TestExpectedResults 
//!					On calling FragmentCount (), the number of fragments (descriptors / pseudo descriptors) that
//!					the transfer request has been split into is returned. 
//!					
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace FragmentationCount
	{
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 128);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);
	CSingleTransferTest test1(_L("Fragmentation Count - 128 fragments"), 1, transferArgs, expectedResults, KKilo);
	TTestCase testCase(new (ELeave) CMultiVersionTest(&test1), EFalse);

	const TRequestResults requestResult2(KErrNone, 4);
	const TResultSet expectedResults2(KErrNone, requestResult2, KErrNone, threadCallback);
	CSingleTransferTest test2(_L("Fragmentation Count - 4 fragments"), 1, transferArgs, expectedResults2, 32*KKilo);
	TTestCase testCase2(new (ELeave) CMultiVersionTest(&test2), EFalse);

	// Also specifying an element size to get the PIL to further adjust the
	// fragment size(s): element size = 32, transfer length = 128KB, max
	// transfer length = 6500 bytes, # of fragments expected = 21 (20 with 6496
	// bytes each + 1 with 1152 bytes).
	TDmaTransferArgs transferArgs3(0, size, size, KDmaMemAddr, KDmaSyncAuto, 0, KDmaAddrModePostIncrement, 32);
	const TRequestResults requestResult3(KErrNone, 21);
	const TResultSet expectedResults3(KErrNone, requestResult3, KErrNone, threadCallback);
	CSingleTransferTest test3(_L("Fragmentation Count - 21 fragments"),
							  1, transferArgs3, expectedResults3, 6500);
	TTestCase testCase3(new (ELeave) CMultiVersionTest(&test3), EFalse);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2584,KBASE-DMA-2585
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    DMA Benchmark tests
//!					To compare the speed of request fragmentation for a given transfer
//!					between the new and old frameworks on the NaviEngine platform.
//! TestActions     
//!				1.	Fragment a memory to memory dma transfer of 4Mb, where both source and destination are physically contiguous. 
//!				2.	Fragment using the default element size, and also with a 4K limit.
//!				3.	Carry out for a channel using pseudo descriptors and for a scatter gather channel.
//!
//! TestExpectedResults 
//!				 The results obtained for both framework versions should  be comparable.							
//!
//! TestPriority        High
//! TestStatus          Implemented
//-----------------------------------------------------------------------------------------------
namespace Benchmark
	{
	const TInt bmIters = 10;
	namespace Frag
		{
		const TInt size = 1 * KMega;
		TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

		TTestCase testCase_256K(new (ELeave) CDmaBmFragmentation(_L("1 MB transfer - 256K frag size"), bmIters, transferArgs, 256 * KKilo), EFalse);
		TTestCase testCase_8K(new (ELeave) CDmaBmFragmentation(_L("1 MB transfer - 8K frag size"), bmIters, transferArgs, 8 * KKilo), EFalse);
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
			TTestCase testCase_16(new (ELeave) CDmaBmTransfer(_L("128 K - 16K frag size"), bmIters, transferArgs, 16 * KKilo), EFalse);
			TTestCase testCase_4(new (ELeave) CDmaBmTransfer(_L("128 K - 4K frag size"), bmIters, transferArgs, 4 * KKilo), EFalse);
			TTestCase testCase_1(new (ELeave) CDmaBmTransfer(_L("128 K - 1K frag size"), bmIters, transferArgs, 1 * KKilo), EFalse);
			}
		namespace _4MB
			{
			const TInt size = 4 * KMega;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

			CDmaBmTransfer bmTest(_L("4 MB"), bmIters, transferArgs, 0);
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
				TTestCase testCase(&bmTest, EFalse, capAboveV1);
				}
			namespace _4K
				{
				const TInt size = 4 * KKilo;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4K DFC cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse, capAboveV1);
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
				TTestCase testCase(&bmTest, EFalse, capAboveV1);
				}
			namespace _4K
				{
				const TInt size = 4 * KKilo;
				TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);
				CDmaBmTransfer bmTest = CDmaBmTransfer(_L("4K Isr cb"), iterations, transferArgs, 0).
					UseNewDmaApi(ETrue).
					ExpectedResults(expected);
				TTestCase testCase(&bmTest, EFalse, capAboveV1);
				}
			}
		}
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestCaseID      KBASE-DMA-2561
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new & old style fragment using CSingleTransferTest
//!						Test Scenario 1 - DstAddr > SrcAddr & TransferSize=32K & Location is 
//!						address of a memory buffer
//! TestActions     
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
//! TestExpectedResults 
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework					
//!						3.	Fragment request completes and KErrNone returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Fragment_1
	{	
	const TInt srcAddr = 4 * KKilo;
	const TInt desAddr = 64 * KKilo;

	const TInt transferSize =  32 * KKilo;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 32); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("Fragment - Scenario 1"), 1, transferArgs, expectedResults,KKilo));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestCaseID      KBASE-DMA-2561
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new & old style fragment using CSingleTransferTest
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
//! TestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request fails and KErrArgument returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Fragment_2
	{
	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 0;
	
	TDmaTransferArgs transferArgs( srcAddr, desAddr, transferSize,KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 0, KErrArgument, KErrUnknown); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, TCallbackRecord::Empty());


	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("Fragment - Scenario 3"), 1, transferArgs, expectedResults));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestCaseID      KBASE-DMA-2561
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new & old style fragment using CSingleTransferTest
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
//! TestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Fragment_3
	{	
	const TInt srcAddr = 32 * KKilo;
	const TInt desAddr = 64 * KKilo;
	const TInt transferSize = 1;
	
	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone, 1);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("Fragment - Scenario 4"), 1, transferArgs, expectedResults));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestCaseID      KBASE-DMA-2561
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new & old style fragment using CSingleTransferTest
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
//! TestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Fragment_4
	{
	const TInt srcAddr		= 16 * KKilo;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone); 
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("Fragment - Scenario 5"), 1, transferArgs, expectedResults));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2560
//! TestCaseID      KBASE-DMA-2561
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new & old style fragment using CSingleTransferTest
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
//! TestExpectedResults 
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework			
//!						3.	Fragment request completes and KErrNone returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace Fragment_5
	{
	const TInt srcAddr = 16 * KKilo;
	const TInt desAddr = 4 * KMega;
	const TInt transferSize = 3 * KMega;

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);
	const TRequestResults requestResult(KErrNone);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("Fragment - Scenario 6"), 1, transferArgs, expectedResults));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}


//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2562
//! TestCaseID      KBASE-DMA-2563
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new and old style DDmaRequest using CSingleTransferTest
//!						Test Scenario 1 -  aMaxTransferSize=0
//!
//!						1.	Set up the DDmaRequest using  aMaxTransferSize set to 0.
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! TestExpectedResults
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//------------------------------------------------------------------------------------------------
namespace DDmaRequest_1
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 0);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("DDmaRequest - Scenario 1"), 1, transferArgs, expectedResults,0));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//!-------------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2562
//! TestCaseID      KBASE-DMA-2563
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    Test new and old style DDmaRequest using CSingleTransferTest
//!						Test Scenario 2 -  aMaxTransferSize= (64K - 1)   // 65535
//!
//!						1.	Set up the arguments for DDmaRequest using aMaxTransferSize set to (64K - 1).
//!						2.	Setup expected result.
//!						3.	Create single transfer test and run test
//!
//! TestExpectedResults
//!
//!						1.  TransfeArgs set up in DMA framework
//!						2.	Expected results set up in DMA framework
//!						3.	DDmaRequest constructor behaves as expected and KErrArgument returned
//!
//! TestPriority        High
//! TestStatus          Implemented
//---------------------------------------------------------------------------------------------------
namespace DDmaRequest_2
	{
	const TInt desAddr = 4 * KKilo;
	const TInt transferSize = 4 * KKilo;
	TDmaTransferArgs transferArgs(0, desAddr, transferSize, KDmaMemAddr);

	const TRequestResults requestResult(KErrNone, 1);
	const TResultSet expectedResults(KErrNone, requestResult, KErrNone, threadCallback);

	CMultiVersionTest multiVersion(new (ELeave) CSingleTransferTest(_L("DDmaRequest - Scenario 2"), 1, transferArgs, expectedResults, trans64K_1));

	TTestCase testCase(&multiVersion, EFalse);
	TTestCase testCaseConcurrent(&multiVersion, ETrue);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2585
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    SmallFrags: This test provokes the failure seen in DEF140598
//!						The test checks that requests with small fragments
//!						do not trigger a spurious missed interrupt clean up
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.	Create single transfer test using small frags and run test
//!
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.	DMA transfer completes with no errors.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace SmallFrags
	{
	const TInt size = 32;
	TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);

	const TResultSet expectedResults(threadCallback);

	TTestCase testCase(
			new (ELeave) CMultiVersionTest(new (ELeave) CSingleTransferTest(_L("8 * 4byte frags"), 10, transferArgs, expectedResults, 4)),
			EFalse);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2568
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test checks the correct behaviour of Close API in the new DMA framework
//!
//! TestActions     
//!						1.  Open a DMA channel
//!						2.	Open DMA Channel again
//!						3	Close the DMA channel.
//!						4	Open DMA channel to verify that the DMA channel closed.						
//!
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.	DMA Framework returns KErrInUse as channel is already open.					
//!						3.	DMA channel closes and KErrNone returned.
//!						4.	DMA channel opens and KErrNone returned.						
//!							
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace CloseApiTest
	{
	COpenCloseTest testCloseApi = COpenCloseTest(_L("Close API Test"), 1).RunOpenApiTest(EFalse); 
	TTestCase testCaseCloseApi(&testCloseApi, EFalse, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2564
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test checks the correct behaviour of Open API in the new DMA framework
//!
//! TestActions     
//!						1.  Open a DMA channel
//!						2.	Verify that channel is really open by closing DMA channel
//!
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned
//!						2.  DMA channel closes again returns KErrNone.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace OpenApiTest
	{
	COpenCloseTest testOpenApi = COpenCloseTest(_L("Open API Test"), 1).RunOpenApiTest(ETrue); 
	TTestCase testCaseOpenApi(&testOpenApi, EFalse, capAboveV1);
	}

//----------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2567
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies the correct behavior of TBool IsQueueEmpty() in the new DMA 
//!					framework and check its return value 
//!
//! TestActions     
//!						1.	Open a single DMA channel for a transfer.
//!						2.	Setup a DMA request and Fragment the request.
//!						3.	Call IsQueueEmpty().
//!						4.	Queue the DMA request.
//!						5.	Call IsQueueEmpty().
//! TestExpectedResults 
//!						1.  DMA channel opens and KErrNone returned.
//!						2.  DMA request created and fragmented and KErrNone returned.
//!						3.  IsQueueEmpty() returns ETrue.
//!						4.  Request queued and KErrNone returned
//!						5.	IsQueueEmpty() returns EFalse.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace IsQueueEmptyTest
	{
	const TInt size = 2 * KMega;
	const TDmaTransferArgs transferArgArray[] = {
		TDmaTransferArgs(0, size, size, KDmaMemAddr),
		TDmaTransferArgs(2 * size, 3 * size, size, KDmaMemAddr)
	};

	const TResultSet expected[] =
		{
		TResultSet(),
		TResultSet()
		};
	const TResultSet expectedResults(isrCallback);

	CIsQueueEmptyTest isQueueEmpty =
		CIsQueueEmptyTest(_L("IsQueueEmptyTest using muliple frags"), 1, transferArgArray, expected, ARRAY_LENGTH(transferArgArray))
			.SetPreTransferTest(&KPreTransferIncrBytes)
			.SetPostTransferTest(&KCompareSrcDst);

	TTestCase testCase(&isQueueEmpty,EFalse,capAboveV1,pauseRequired_skip);
	}

//--------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2573
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies the correct behavior of the DMA Channel Linking API 
//!					in the new DMA framework 
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.  Link and Unlink DMA channel.
//!						3.  Close DMA channel.
//!					
//! 
//!	TestExpectedResults 1.  DMA channel opens and KErrNone returned.
//!						2.  KErrNotSupported returned when DMA Linking and Unlinking API are called.
//!						3.	DMA channel closes and KErrNone returned.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace ChannelLinkingTest
	{
	const TInt srcAddr		= 0;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;	

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);	

	TResultSet noTransferExpected = TResultSet().
		RequestResult(TRequestResults().CreationResult(KErrUnknown).FragmentationResult(KErrUnknown).QueueResult(KErrUnknown)).
		CallbackRecord(TCallbackRecord::Empty()).
		PostTransferResult(KErrUnknown);

	CLinkChannelTest testChannelLinking = CLinkChannelTest(_L("DMA Channel Linking and Unlinking Negative Test"), 1, transferArgs, noTransferExpected); 
	TTestCase testCaseChannelLinking(&testChannelLinking , EFalse, capAboveV1,LinkingNotWanted);	
	}

//--------------------------------------------------------------------------------------------
//! TestCaseID      KBASE-DMA-2574
//! TestType        CIT
//! PREQ            REQ
//! TestCaseDesc    This test verifies that the DMA Element Counting APIs can be called. The calls 
//!					to these functions are meant to be used to meet a code coverage requirement.These APIs
//!					are not yet supported so the functionality of the APIs are not currently tested.
//!
//! TestActions     
//!						1.	Open a DMA channel for a transfer.
//!						2.  Make calls to Element Counting APIs
//!						3.  Close DMA channel.
//!					
//! 
//!	TestExpectedResults 1.  DMA channel opens and KErrNone returned.
//!						2.  Element Counting APIs are called without crashing the framework
//!						3.	DMA channel closes and KErrNone returned.
//!
//! TestPriority        High
//! TestStatus          Implemented
//----------------------------------------------------------------------------------------------
namespace ElementCountingTest
	{
	const TInt srcAddr		= 0;
	const TInt desAddr		= 2 * KMega;	
	const TInt transferSize = 1 * KMega;	

	TDmaTransferArgs transferArgs(srcAddr, desAddr, transferSize, KDmaMemAddr);	

	const TResultSet expectedResults(threadCallback);
	CElementCountingTest testElementCounting(_L("DMA Element Counting Test"), 1, transferArgs, expectedResults);
	TTestCase testCaseElementCounting(&testElementCounting, EFalse, capAboveV1);
	}

static TTestCase* StaticSimpleTestArray[] = {
	&Simple_1::testCase,
	&Simple_1::testCaseConcurrent,
};

static TTestCase* StaticCallbackTestArray[] = {
	&Callback::testCase,
	&Callback::testCaseOldRequest,	
};

static TTestCase* StaticIsrRequeTestArray[] = {
	&ISR_Reque::endOnRedo::testCase,
	&ISR_Reque::endOnIsrCb::testCase,
	&ISR_Reque::endOnThreadCb::testCase,
	&ISR_Reque::changeSize::testCase,
#ifdef _DEBUG
	&ISR_Reque::invalidAddresses::testCase, // addresses only checked in UDEB
#endif
#ifdef _REMOVEDTEST
	// This test case is currently caught by a FAULT instead of a return code
	// as expected. Currently, the facility to return an error code to the test
	// framework is not yet supported.
	&ISR_Reque::multipleFragments::testCase, 
#endif
};

static TTestCase* StaticMultipartTestArray[] = {
	&Multipart::testCase,
};

static TTestCase* StaticIsrAndDfcTestArray[] = {
	&IsrAndDfc::DfcBeforeIsr::testCase,
	&IsrAndDfc::IsrBeforeDfc::testCase,
};

static TTestCase* Static2DTestArray[] = {
	&_2D_Test::testCase2d
};

static TTestCase* StaticFragmentTestArray[] = {
	&FragmentationCount::testCase,
	&FragmentationCount::testCase2,
	&SmallFrags::testCase,
	&Fragment_1::testCase,
	&Fragment_1::testCaseConcurrent,
	&Fragment_2::testCase,
	&Fragment_2::testCaseConcurrent,
	&Fragment_3::testCase,
	&Fragment_3::testCaseConcurrent,
	&Fragment_4::testCase,
	&Fragment_4::testCaseConcurrent,
	&Fragment_5::testCase,
	&Fragment_5::testCaseConcurrent,
};

static TTestCase* StaticBenchmarkTestArray[] = {
	// Benchmarks are only really meaningful
	// on UREL builds
	&Benchmark::Frag::testCase_256K,
	&Benchmark::Frag::testCase_8K,
	&Benchmark::Transfer::_128K::testCase_128,
	&Benchmark::Transfer::_128K::testCase_16,
	&Benchmark::Transfer::_128K::testCase_4,
	&Benchmark::Transfer::_128K::testCase_1,
	&Benchmark::Transfer::_4Bytes::testCase,
	&Benchmark::Transfer::_4MB::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4K::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4K::testCase,
};

static TTestCase* StaticRequestTestArray[] = {
	&DDmaRequest_1::testCase,
	&DDmaRequest_1::testCaseConcurrent,
	&DDmaRequest_2::testCase,
	&DDmaRequest_2::testCaseConcurrent,
};

static TTestCase* StaticChannelTestArray[] = {
	&CloseApiTest::testCaseCloseApi,
	&OpenApiTest::testCaseOpenApi,
};

static TTestCase* StaticSuspendTestArray[] = {
	&PauseResumeApiTest::testCasePauseResume,  
	&PauseResumeApiNegTest::testCasePauseResumeNeg,
};

static TTestCase* StaticQueueTestArray[] = {
	&CancelAllTest::testCase, 
	&IsQueueEmptyTest::testCase,	
};

//Append new test cases here
static TTestCase* StaticTestArray[] = {
	&Simple_1::testCase,
	&Simple_1::testCaseConcurrent,	
	&Callback::testCase,
	&Callback::testCaseOldRequest,
	&ISR_Reque::endOnRedo::testCase,
	&ISR_Reque::endOnIsrCb::testCase,
	&ISR_Reque::endOnThreadCb::testCase,
	&ISR_Reque::changeSize::testCase,
#ifdef _DEBUG
	&ISR_Reque::invalidAddresses::testCase, // addresses only checked in UDEB
#endif
#ifdef _REMOVEDTEST
	// This test case is currently caught by a FAULT instead of a return code
	// as expected. Currently, the facility to return an error code to the test
	// framework is not yet supported.
	&ISR_Reque::multipleFragments::testCase, 
#endif
	&Multipart::testCase,
	&IsrAndDfc::DfcBeforeIsr::testCase,
	&IsrAndDfc::IsrBeforeDfc::testCase,
	&_2D_Test::testCase2d,
	&FragmentationCount::testCase,
	&FragmentationCount::testCase2,
	&FragmentationCount::testCase3,
	&SmallFrags::testCase,
#ifndef _DEBUG
	// Benchmarks are only really meaningful
	// on UREL builds
	&Benchmark::Frag::testCase_256K,
	&Benchmark::Frag::testCase_8K,
	&Benchmark::Transfer::_128K::testCase_128,
	&Benchmark::Transfer::_128K::testCase_16,
	&Benchmark::Transfer::_128K::testCase_4,
	&Benchmark::Transfer::_128K::testCase_1,
	&Benchmark::Transfer::_4Bytes::testCase,
	&Benchmark::Transfer::_4MB::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4Bytes::testCase,
	&Benchmark::CompareIsrDfcCb::Dfc::_4K::testCase,
	&Benchmark::CompareIsrDfcCb::Isr::_4K::testCase,
#endif
	&Fragment_1::testCase,
	&Fragment_1::testCaseConcurrent,
	&Fragment_2::testCase,
	&Fragment_2::testCaseConcurrent,
	&Fragment_3::testCase,
	&Fragment_3::testCaseConcurrent,
	&Fragment_4::testCase,
	&Fragment_4::testCaseConcurrent,
	&Fragment_5::testCase,
	&Fragment_5::testCaseConcurrent,	
	&DDmaRequest_1::testCase,
	&DDmaRequest_1::testCaseConcurrent,
	&DDmaRequest_2::testCase,
	&DDmaRequest_2::testCaseConcurrent,
	&CloseApiTest::testCaseCloseApi,
	&OpenApiTest::testCaseOpenApi,
	&PauseResumeApiTest::testCasePauseResume,  
	&PauseResumeApiNegTest::testCasePauseResumeNeg,	
	&CancelAllTest::testCase,
	&IsQueueEmptyTest::testCase,
	&ChannelLinkingTest::testCaseChannelLinking,
	&ElementCountingTest::testCaseElementCounting,
};

RPointerArray<TTestCase> TestArray(StaticTestArray, ARRAY_LENGTH(StaticTestArray));
RPointerArray<TTestCase> TestArrayIsrAndDfc(StaticIsrAndDfcTestArray, ARRAY_LENGTH(StaticIsrAndDfcTestArray));
RPointerArray<TTestCase> TestArraySimple(StaticSimpleTestArray, ARRAY_LENGTH(StaticSimpleTestArray));
RPointerArray<TTestCase> TestArrayQueue(StaticQueueTestArray, ARRAY_LENGTH(StaticQueueTestArray));
RPointerArray<TTestCase> TestArrayChannel(StaticChannelTestArray, ARRAY_LENGTH(StaticChannelTestArray));
RPointerArray<TTestCase> TestArraySuspend(StaticSuspendTestArray, ARRAY_LENGTH(StaticSuspendTestArray));
RPointerArray<TTestCase> TestArrayFragment(StaticFragmentTestArray, ARRAY_LENGTH(StaticFragmentTestArray));
RPointerArray<TTestCase> TestArrayBenchmark(StaticBenchmarkTestArray, ARRAY_LENGTH(StaticBenchmarkTestArray));
RPointerArray<TTestCase> TestArray2DTest(Static2DTestArray, ARRAY_LENGTH(Static2DTestArray));
RPointerArray<TTestCase> TestArrayMultiPart(StaticMultipartTestArray, ARRAY_LENGTH(StaticMultipartTestArray));
RPointerArray<TTestCase> TestArrayIsrReque(StaticIsrRequeTestArray, ARRAY_LENGTH(StaticIsrRequeTestArray));
RPointerArray<TTestCase> TestArrayCallback(StaticCallbackTestArray, ARRAY_LENGTH(StaticCallbackTestArray));
RPointerArray<TTestCase> TestArrayRequest(StaticRequestTestArray, ARRAY_LENGTH(StaticRequestTestArray));
