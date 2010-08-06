// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32test\dmav2\t_dma2.cpp

#include "d_dma2.h"
#include "u32std.h"
#include "t_dma2.h"
#include "cap_reqs.h"

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32debug.h>
#include <e32svr.h>
#include <e32def_private.h>

// DMA test framework command  parameter options

// SelfTest option
_LIT(KArgSelfTest, "/SELFTEST");  
_LIT(KArgSelfTest2, "/S");		  

//Verbose option
_LIT(KArgVerboseOutput, "/VERBOSE"); 
_LIT(KArgVerboseOutput2, "/V");	     
  
//Simple transfer test option
_LIT(KArgSimpleTest, "/simple"); 

//Callback test option
_LIT(KArgCallBackTest, "/callback");

//Suspend test option
_LIT(KArgSuspendTest, "/suspend");

//Multipart transfer tests
_LIT(KArgMultiPartTest, "/multi");

//Isr and dfc test option
_LIT(KArgIsrDfcTest, "/isrdfc");

//Isr reque  test option
_LIT(KArgIsrequeTest, "/isreque");

//Benchmark test option
_LIT(KArgBenchmarkTest, "/bench");

//Graphics test option
_LIT(KArgGraphicTest, "/graphic");

//DMA channel (opening and closing)  test option
_LIT(KArgChannelTest, "/channel");

//Queue test option
_LIT(KArgQueueTest, "/queue");

//Fragment test option
_LIT(KArgFragmentTest, "/fragment");

//Request test option
_LIT(KArgRequestTest, "/request");



TBool gHelpRequested;   // print usage 
TBool gVerboseOutput;   // enable verbose output 
TBool gSelfTest;        // run SelfTest 
TBool gSimpleTest;		// run only Simple transfer test
TBool gCallBack;		// run only Callback test
TBool gSuspend;			// run only Pause and resume tests
TBool gIsrReque;		// run only IsrReque tests
TBool gMultiPart;		// run only Multipart tests
TBool gIsrAndDfc;		// run only IsrAndDfc tests
TBool gBenchmark;		// run only Benchmark tests
TBool gGraphic;			// run only Graphic tests
TBool gFragment;		// run only Fragment related tests
TBool gChannel;			// run only Channel(open/close)tests
TBool gQueue;			// run only Queue related tests
TBool gRequest;			// run only Request related tests

/**
This function prints out the PSL test Information
*/
void Print(const TDmaV2TestInfo& aInfo)
	{
	PRINT(aInfo.iMaxTransferSize);
	PRINT(aInfo.iMemAlignMask);
	PRINT(aInfo.iMemMemPslInfo);
	PRINT(aInfo.iMaxSbChannels);
	for(TInt i=0; i<aInfo.iMaxSbChannels; i++)
		{
		PRINT(aInfo.iSbChannels[i]);
		}
	PRINT(aInfo.iMaxDbChannels);
	for(TInt j=0; j<aInfo.iMaxDbChannels; j++)
		{
		PRINT(aInfo.iDbChannels[j]);
		}
	PRINT(aInfo.iMaxSgChannels);
	for(TInt k=0; k<aInfo.iMaxSgChannels; k++)
		{
		PRINT(aInfo.iSgChannels[k]);
		}
	}

void CDmaTest::PrintTestInfo() const
	{
	TBuf<32> buf;
	buf.AppendFormat(_L("DMA channel %d"), iChannelCookie);
	RDebug::RawPrint(buf);
	}

//////////////////////////////////////////////////////////////////////
// CDmaTest
//////////////////////////////////////////////////////////////////////

void CDmaTest::OpenDmaSession()
	{
	// Only open a new session if one
	// was not already supplied
	if(iDmaSession.Handle() == KNullHandle)
		{
		TInt r = iDmaSession.Open();
		if(KErrNone != r)
			{
			RDebug::Printf("CDmaTest::OpenDmaSession = %d\n", r);
			}
		TEST_ASSERT(r == KErrNone);
		r = iDmaSession.OpenSharedChunk(iChunk);
		TEST_ASSERT(r == KErrNone);
		}
	}

// Open another handle to the test driver
void CDmaTest::OpenDmaSession(const RDmaSession& aSession)
	{
	iDmaSession = aSession;
	TInt r = iDmaSession.Duplicate(RThread(), EOwnerThread);
	TEST_ASSERT(r == KErrNone);

	// open another handle to the test driver chunk
	r = iDmaSession.OpenSharedChunk(iChunk);
	TEST_ASSERT(r == KErrNone);
	}

void CDmaTest::CloseDmaSession()
	{
	iChunk.Close();
	iDmaSession.Close();
	}

void CDmaTest::PreTransferSetup()
	{
	}

TInt CDmaTest::DoPostTransferCheck()
	{
	return KErrNotSupported;
	}

void CDmaTest::ChannelPause(TUint aChannelSessionCookie)
{
	TInt r = iDmaSession.ChannelPause(aChannelSessionCookie);
	TEST_ASSERT(r == KErrNone);
}

void CDmaTest::ChannelResume(TUint aChannelSessionCookie)
{
	TInt r = iDmaSession.ChannelResume(aChannelSessionCookie);
	TEST_ASSERT(r == KErrNone);
}
//////////////////////////////////////////////////////////////////////
// CSingleTransferTest
//////////////////////////////////////////////////////////////////////
void CSingleTransferTest::RunTest()
	{
	OpenDmaSession();
	PreTransferSetup();

	OpenChannel();
	CreateDmaRequest();
	Fragment();
	Queue();
	FreeRequest();
	CloseChannel();
	PostTransferCheck();

	CloseDmaSession();
	}

void CSingleTransferTest::OpenChannel()
	{
	iActual.iChannelOpenResult =
		iDmaSession.ChannelOpen(iChannelCookie, iChannelSessionCookie);
	}

void CSingleTransferTest::CreateDmaRequest()
	{
	if(iUseNewRequest)
	{
		if(gVerboseOutput)
			{
			RDebug::Printf("Calling New Request API\n");
			}
		iActual.iRequestResult.iCreate =
			iDmaSession.RequestCreate(iChannelSessionCookie, iRequestSessionCookie, iMaxFragmentSize);
		}
	else
		{
		if(gVerboseOutput)
			{
			RDebug::Printf("Calling Old Request API\n");
			}
		iActual.iRequestResult.iCreate =
			iDmaSession.RequestCreateOld(iChannelSessionCookie, iRequestSessionCookie, iMaxFragmentSize);
		}
	}

void CSingleTransferTest::Fragment()
	{
	if(iActual.iRequestResult.iCreate != KErrNone)
		return;

	if(iUseNewFragment)
		{
		if(gVerboseOutput)
			{
			RDebug::Printf("Calling New Fragment API\n");
			}
		iActual.iRequestResult.iFragmentationResult =
			iDmaSession.FragmentRequest(iRequestSessionCookie, iTransferArgs);
		}
	else
		{
		if(gVerboseOutput)
			{
			RDebug::Printf("Calling Old Fragment API\n");
			}
		iActual.iRequestResult.iFragmentationResult =
			iDmaSession.FragmentRequestOld(iRequestSessionCookie, iTransferArgs);
		}

	const TInt fragmentCount = iDmaSession.RequestFragmentCount(iRequestSessionCookie);

	// Record the fragment count if a non-zero value was expected,
	// or if it was an error value
	if(iExpected.iRequestResult.iFragmentCount != 0 || fragmentCount < 0)
		iActual.iRequestResult.iFragmentCount = fragmentCount;
	}

void CSingleTransferTest::Queue()
	{
	if(iActual.iRequestResult.iFragmentationResult == KErrNone)
		{
		iActual.iRequestResult.iQueueResult = iDmaSession.QueueRequest(iRequestSessionCookie, &iActual.iCallbackRecord);
		}
	}

void CSingleTransferTest::PostTransferCheck()
	{
	if(iPostTransferCheck)
		iActual.iPostTransferCheck = DoPostTransferCheck();
	}

TInt CSingleTransferTest::DoPostTransferCheck()
	{
	return iPostTransferCheck->Check(*this);
	}

void CSingleTransferTest::FreeRequest()
	{
	if(iActual.iRequestResult.iCreate == KErrNone)
		{
		TInt r = iDmaSession.RequestDestroy(iRequestSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}
	}

void CSingleTransferTest::CloseChannel()
	{
	if(iActual.iChannelOpenResult == KErrNone)
		{
		TInt r = iDmaSession.ChannelClose(iChannelSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}
	}

void CSingleTransferTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Single transfer"));
	}

void CSingleTransferTest::PrintTestInfo() const
	{
	CDmaTest::PrintTestInfo();

	// State which API versions are being used
	if(iUseNewFragment)
		RDebug::RawPrint(_L(", Fragment v2,"));
	else
		RDebug::RawPrint(_L(", Fragment v1,"));

	if(iUseNewRequest)
		RDebug::RawPrint(_L(" DDmaRequest v2"));
	else
		RDebug::RawPrint(_L(" DDmaRequest v1"));
	}

void CSingleTransferTest::PreTransferSetup()
	{
	if(iPreTransfer)
		iPreTransfer->Setup(*this); //initialize test
	}

TBool CSingleTransferTest::Result()
	{
	const TBool result = iExpected == iActual;
	if(!result)
		{
		RDebug::Printf("TResultSets do not match");
		}
	if(!result || gVerboseOutput)
		{
		RDebug::Printf("\nExpected error codes:");
		iExpected.Print();
		RDebug::Printf("Expected callback record:");
		iExpected.iCallbackRecord.Print();

		RDebug::Printf("\nActual error codes:");
		iActual.Print();
		RDebug::Printf("Actual callback record:");
		iActual.iCallbackRecord.Print();
		}
	return result;
	}

//////////////////////////////////////////////////////////////////////
// CDmaTestDecorator
//////////////////////////////////////////////////////////////////////

CDmaTestDecorator::CDmaTestDecorator(CDmaTest* aDecoratedTest)
	: CDmaTest(_L("Decorated Test"), 1, NULL, NULL), iDecoratedTest(aDecoratedTest)
	{}

CDmaTestDecorator::CDmaTestDecorator(const CDmaTestDecorator& aOther)
	: CDmaTest(aOther), iDecoratedTest( static_cast<CDmaTest*>( aOther.iDecoratedTest->Clone() ) )
	// Need cast because Clone does not have a covariant return type,
	// as not all compillers allow it
	{}

//////////////////////////////////////////////////////////////////////
// CMultiVersionTest
//////////////////////////////////////////////////////////////////////

CMultiVersionTest::CMultiVersionTest(CSingleTransferTest* aDmaTest)
	: CDmaTestDecorator(aDmaTest), iNewVersionTest(NULL)
	{
	}

CMultiVersionTest::CMultiVersionTest(const CMultiVersionTest& aOther)
	: CDmaTestDecorator(aOther), iNewVersionTest( aOther.iNewVersionTest ? static_cast<CSingleTransferTest*>(aOther.iNewVersionTest->Clone()) : NULL)
	{
	}

CMultiVersionTest::~CMultiVersionTest()
	{
	delete iDecoratedTest;
	delete iNewVersionTest;
	}

void CMultiVersionTest::SetupL()
	{
	// Open a tempory dma session to find out the
	// capabilities of the dma channel.
	OpenDmaSession();
	Configure();
	CloseDmaSession();
	}

void CMultiVersionTest::Announce() const
	{
	CTest::Announce();

	iDecoratedTest->Announce();

	if(iNewVersionTest)
		iNewVersionTest->Announce();
	}

void CMultiVersionTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Multi version test wrapper"));
	}

void CMultiVersionTest::PrintTestInfo() const
	{
		if(iNewVersionTest)
			{ 		
			RDebug::RawPrint(_L("Running tests using Version 2 PIL"));
			}
		else
			{
			RDebug::RawPrint(_L("Running tests using Version 1 PIL"));		
			}
	}

void CMultiVersionTest::RunTest()
	{
	OpenDmaSession();

	// iDecoratedTest is the test, in the old configuration
	// iNewVersionTest is the same test, configured
	// to use the new APIs
	//
	// 2 objects are needed since they can each store
	// their own results

	iDecoratedTest->OpenDmaSession(iDmaSession);
	(*iDecoratedTest)();

	if(iNewVersionTest)
		{
		iNewVersionTest->OpenDmaSession(iDmaSession);
		(*iNewVersionTest)();
		}

	CloseDmaSession();
	}

/**
Maybe create another test object to run with new API

Pass on the cookie for the channel they must test
*/
void CMultiVersionTest::Configure()
	{
	static_cast<CSingleTransferTest*>(iDecoratedTest)->UseNewDmaApi(EFalse);
	iDecoratedTest->SetChannelCookie(iChannelCookie);

	if(Version2PILAvailable())
		{
		iNewVersionTest = static_cast<CSingleTransferTest*>(iDecoratedTest->Clone());
		TEST_ASSERT(iNewVersionTest != NULL);

		iNewVersionTest->UseNewDmaApi(ETrue);
		iNewVersionTest->SetChannelCookie(iChannelCookie);
		}
	}

/**
Discover from DMA channel what PIL versions are available.
In practice V1 APIs will always be available, V2 may be.
*/
TBool CMultiVersionTest::Version2PILAvailable()
	{
	TUint channelSessionCookie;
	TInt r = iDmaSession.ChannelOpen(iChannelCookie, channelSessionCookie);
	TEST_ASSERT(r == KErrNone);

	TDmacTestCaps channelCaps;
	r = iDmaSession.ChannelCaps(channelSessionCookie, channelCaps);
	TEST_ASSERT(r == KErrNone);

	r = iDmaSession.ChannelClose(channelSessionCookie);
	TEST_ASSERT(r == KErrNone);

	return channelCaps.iPILVersion >= 2;
	}

TBool CMultiVersionTest::Result()
	{
	TBool v1Result = iDecoratedTest->Result();
	if(gVerboseOutput || !v1Result)
		RDebug::Printf("V1 API result: %s", v1Result ? "success" : "failure");

	TBool v2Result = iNewVersionTest ? iNewVersionTest->Result() : ETrue;
	if(gVerboseOutput || !v1Result)
		RDebug::Printf("V2 API result: %s", v2Result ? "success" : "failure");
	return v1Result && v2Result;
	}

//////////////////////////////////////////////////////////////////////
// CDmaBenchmark
//////////////////////////////////////////////////////////////////////
CDmaBenchmark::CDmaBenchmark(const TDesC& aName, TInt aIterations, const TResultSet& aExpectedResults, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize)
	:CSingleTransferTest(aName, aIterations, aTransferArgs, aExpectedResults, aMaxFragmentSize, NULL, NULL)
	{
	UseNewDmaApi(EFalse);
	}

CDmaBenchmark::CDmaBenchmark(const CDmaBenchmark& aOriginal)
	:CSingleTransferTest(aOriginal)
	{
	CopyL(aOriginal.iResultArray, iResultArray);
	}

CDmaBenchmark::~CDmaBenchmark()
	{
	iResultArray.Close();
	}

TUint64 CDmaBenchmark::MeanResult()
	{
	if(gVerboseOutput)
		RDebug::Printf("CDmaBenchmark::MeanResult\n");

	const TInt count = iResultArray.Count();

	TEST_ASSERT(count > 0);
	TEST_ASSERT(count == iIterations);

	TUint64 sum = 0;

	for(TInt i = 0; i < count; i++)
		{
		const TUint64 value = iResultArray[i];
		if(gVerboseOutput)
			RDebug::Printf("iResultArray[%d]: %lu", i, value);

		sum += value;
		}

	return sum / count;
	}

TBool CDmaBenchmark::Result()
	{
	const TBool result = CSingleTransferTest::Result();
	if(result)
		{
		RDebug::Printf("  Mean time: %lu us", MeanResult());
		}

	return result;
	}

//////////////////////////////////////////////////////////////////////
// CDmaBmFragmentation
//////////////////////////////////////////////////////////////////////
CDmaBmFragmentation::CDmaBmFragmentation(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize)
	:CDmaBenchmark(aName, aIterations, ExpectedResults, aTransferArgs, aMaxFragmentSize)
	{}

const TResultSet CDmaBmFragmentation::ExpectedResults(KErrNone,
		TRequestResults(KErrNone, 0, KErrNone, KErrUnknown),
		KErrUnknown,
		TCallbackRecord::Empty()
		);

void CDmaBmFragmentation::Fragment()
	{
	TUint64 time;
	iActual.iRequestResult.iFragmentationResult =
		iDmaSession.FragmentRequestOld(iRequestSessionCookie, iTransferArgs, &time);
	iResultArray.Append(time);
	}

void CDmaBmFragmentation::PrintTestType() const
	{
	RDebug::RawPrint(_L("Fragmentation Benchmark"));
	}

void CDmaBmFragmentation::RunTest()
	{
	OpenDmaSession();

	OpenChannel();
	CreateDmaRequest();
	Fragment();
	FreeRequest();
	CloseChannel();
	CloseDmaSession();
	}

//////////////////////////////////////////////////////////////////////
//	CPauseResumeTest
//
//	-Time how long a given transfer takes
//	-Pause the channel
//	-repeat the transfer (queued asynchronously)
//	-wait for some time (say, 3 times the time measured)
//	-read the value of the TRequestStatus object, to check it is still pending
//	-resume the channel
//	-Wait on the request
//	-Confirm that the request completed
//////////////////////////////////////////////////////////////////////
CPauseResumeTest::~CPauseResumeTest()
	{
	}

void CPauseResumeTest::RunTest()
	{
	OpenDmaSession();

	//Open a single DMA channel for a transfer
	OpenChannel();

	RDebug::Printf("Resume unpaused idle channel");
	TInt r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrCompletion == r);

	RDebug::Printf("Pause idle channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrNone == r);

	RDebug::Printf("Pause paused idle Channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrCompletion == r);

	RDebug::Printf("Resume paused idle channel");
	r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrNone == r);

	//Setup a DMA request and Fragment the request.
	CreateDmaRequest();
	Fragment();

	//Queue the DMA request and time how long a transfer takes
	TUint64 queueTime;
	DoCalibrationTransfer(queueTime);

	RDebug::Printf("Calibration transfer completed in %Lu us",queueTime);
	TUint32 waitQueueReqTime = I64LOW(queueTime*3); //3 times the time measured in DoCalibrationTransfer
	TEST_ASSERT(I64HIGH(queueTime*3) == 0); // If transfer takes over an hour, something has gone wrong anyway

	// Initialise buffers, after calibration transfer
	PreTransferSetup();

	RDebug::Printf("Resume unpaused channel");
	r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrCompletion == r);

	//Pause DMA Transfer
	RDebug::Printf("Pausing DMA Channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrNone == r);

	RDebug::Printf("Pause paused Channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrCompletion == r);

	//Repeat the transfer (queued asynchronously)
	TRequestStatus queueRequestStatus;
	iActual.iRequestResult.iQueueResult = QueueAsyncRequest(queueRequestStatus,queueTime);
	RDebug::Printf("Queue a DMA Request and wait for %u us ", waitQueueReqTime);

	User::After(waitQueueReqTime);
	RDebug::Printf("Finished waiting");
	TEST_ASSERT(queueRequestStatus.Int() == KRequestPending);

	TBool queueEmpty = ETrue;
	r = iDmaSession.ChannelIsQueueEmpty(iChannelSessionCookie,queueEmpty);
	TEST_ASSERT(r == KErrNone);
	TEST_ASSERT(!queueEmpty);

	//Resume DMA channel
	RDebug::Printf("Resuming paused DMA Channel");
	r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrNone == r);

	//Wait for transfer to complete
	User::WaitForRequest(queueRequestStatus);
	if (queueRequestStatus.Int() == KErrNone)
		{
		RDebug::Printf("DMA QueueAsyncRequest completed");
		}

	FreeRequest();
	CloseChannel();

	PostTransferCheck();
	CloseDmaSession();
	}

/**
Time how long transfer takes, with no pausing
*/
void CPauseResumeTest::DoCalibrationTransfer(TUint64 &atime)
	{
	//Queue the DMA request.
	TCallbackRecord pCallbackRecord;
	TInt r = iDmaSession.QueueRequest(iRequestSessionCookie,&pCallbackRecord,&atime);
	TEST_ASSERT(r == KErrNone);
	}

TInt CPauseResumeTest::QueueAsyncRequest(TRequestStatus &aRequestState, TUint64 &atime)
	{
	return iDmaSession.QueueRequest(iRequestSessionCookie,aRequestState, &iActual.iCallbackRecord, &atime);
	}

void CPauseResumeTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Pause and Resume API Test"));
	}

//////////////////////////////////////////////////////////////////////
//	CPauseResumeNegTest
//
//	-Open DMA Channel
//	-Pause and Resume DMA channel
//	-Check that KErrNotSupported is returned
//	-Close DMA Channel
//////////////////////////////////////////////////////////////////////
CPauseResumeNegTest::~CPauseResumeNegTest()
	{
	}

void CPauseResumeNegTest::RunTest()
	{
	OpenDmaSession();

	//Open a single DMA channel for a transfer
	OpenChannel();

	RDebug::Printf("Resume unpaused idle channel");
	TInt r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	RDebug::Printf("Pause idle channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	RDebug::Printf("Pause paused idle Channel");
	r = iDmaSession.ChannelPause(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	RDebug::Printf("Resume paused idle channel");
	r = iDmaSession.ChannelResume(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	CloseChannel();
	CloseDmaSession();
	}

void CPauseResumeNegTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Pause and Resume API Test - Negative Test"));
	}

//////////////////////////////////////////////////////////////////////
//	CLinkChannelTest
//
//	-Open DMA Channel
//	-Link and Unlink DMA channel
//	-Check that KErrNotSupported is returned
//	-Close DMA Channel
//
//////////////////////////////////////////////////////////////////////
CLinkChannelTest::~CLinkChannelTest()
	{
	}

void CLinkChannelTest::RunTest()
	{
	OpenDmaSession();

	//Open a single DMA channel for a transfer
	OpenChannel();

	RDebug::Printf("Linking DMA channels");
	TInt r = iDmaSession.ChannelLinking(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	RDebug::Printf("Unlinking DMA channels");
	r = iDmaSession.ChannelUnLinking(iChannelSessionCookie);
	TEST_ASSERT(KErrNotSupported == r);

	CloseChannel();
	CloseDmaSession();
	}

void CLinkChannelTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Channel Linking API Test - Negative Test"));
	}

//////////////////////////////////////////////////////////////////////
//	CElementCountingTest
//
//	-Open DMA Channel
//	-Create Request
//	-Fragment and Make calls to Element Counting APIs
//  -Check that TotalNumDstElementsTransferred() and TotalNumSrcElementsTransferred()
//	 return non zero values
//  -Check that KErrNone(from test driver) returned for other API calls
//	-Queue Request 
//	-Close DMA Channel
//////////////////////////////////////////////////////////////////////
CElementCountingTest::~CElementCountingTest()
	{
	}

void CElementCountingTest::RunTest()
	{
	OpenDmaSession();
	PreTransferSetup();

	//Open a single DMA channel for a transfer
	OpenChannel();
	
	//Setup a DMA request and Fragment the request.
	RDebug::Printf("Create and Fragment DMA Request");
	CreateDmaRequest();
	Fragment();

	//Enable src/dst counting
	RDebug::Printf("Enable DstElementCounting");
	TInt r = iDmaSession.RequestEnableDstElementCounting(iRequestSessionCookie);
	TEST_ASSERT(KErrNone == r);

	RDebug::Printf("Enable SrcElementCounting");
	r = iDmaSession.RequestEnableSrcElementCounting(iRequestSessionCookie);
	TEST_ASSERT(KErrNone == r);

	//Queue request
	RDebug::Printf("Queue DMA Request");
	Queue();

	//Disable src/dst counting
	RDebug::Printf("Disable DstElementCounting");
	r = iDmaSession.RequestDisableDstElementCounting(iRequestSessionCookie);
	TEST_ASSERT(KErrNone == r);

	RDebug::Printf("Disable SrcElementCounting");
	r = iDmaSession.RequestDisableSrcElementCounting(iRequestSessionCookie);
	TEST_ASSERT(KErrNone == r);

	//check total src/dst elements transferred
	RDebug::Printf("Get Total Number of DstElementsTransferred");
	r = iDmaSession.RequestTotalNumDstElementsTransferred(iRequestSessionCookie);
	TEST_ASSERT(r >= 0);

	RDebug::Printf("Get Total Number of SrcElementsTransferred");
	r = iDmaSession.RequestTotalNumSrcElementsTransferred(iRequestSessionCookie);
	TEST_ASSERT(r >= 0);

	FreeRequest();
	CloseChannel();

	PostTransferCheck();
	CloseDmaSession();
	}

void CElementCountingTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Element Counting Tests"));
	}

//////////////////////////////////////////////////////////////////////
// COpenCloseTest
//////////////////////////////////////////////////////////////////////
COpenCloseTest::~COpenCloseTest()
	{
	}

TBool COpenCloseTest::DoRunClose()
	{
	// For storing cookie during neagtive test i,e open channel twice
	TUint testChannelSessionCookie = 0; 

	// Open a single DMA channel
	TInt r = iDmaSession.ChannelOpen(iChannelCookie, iChannelSessionCookie);
	if (r == KErrNone)//Check that DMA channel opened with no errors
		{
		RDebug::Printf("DMA channel opened");					
		}
	else
		{
		RDebug::Printf("Open DMA channel failed");			
		return EFalse;
		}

	// Open DMA channel again and check that opening DMA channel again fails		
	r = iDmaSession.ChannelOpen(iChannelCookie, testChannelSessionCookie);
	if (r == KErrInUse)
		{
		RDebug::Printf("Opening DMA channel again fails as expected");	
		}
	else
		{
		RDebug::Printf("Open DMA channel again failed");
		return EFalse;
		}

	// Close the DMA channel and check that DMA channel closes with no errors
	r =iDmaSession.ChannelClose(iChannelSessionCookie);	 
	if (r == KErrNone) 
		{
		RDebug::Printf("DMA channel closes with no errors");				
		}
	else
		{
		RDebug::Printf("Close the DMA channel failed");
		return EFalse;
		}
	
	// Verify that the DMA channel was actually closed by opening DMA channel
	r = iDmaSession.ChannelOpen(iChannelCookie, iChannelSessionCookie);
	if (r == KErrNone)
		{
		RDebug::Printf("DMA channel opened after a previous close operation");
		return ETrue;
		}
	else
		{
		RDebug::Printf("Open DMA channel to verify that the DMA channel closed failed");
		return EFalse;
		}	
	}

TBool COpenCloseTest::DoRunOpen()
	{
	// Open a single DMA channel
	TInt r = iDmaSession.ChannelOpen(iChannelCookie, iChannelSessionCookie);
	if (r == KErrNone)//Check that DMA channel opened with no errors
		{			
		RDebug::Printf("DoRunOpen:DMA channel opened");			
		}
	else
		{
		RDebug::Printf("DoRunOpenDMA channel failed to open");				
		return EFalse;
		}	

	// Verify that channel is really open by closing DMA channel
	// and checking that DMA channel closes with no errors
	r = iDmaSession.ChannelClose(iChannelSessionCookie);
	if (r == KErrNone)
		{
		RDebug::Printf("DoRunOpen:DMA channel closes with no errors");			
		return ETrue;
		}
	else
		{
		RDebug::Printf("DoRunOpen:DMA channel failed to close");		
		return EFalse;
		}
	}

TBool COpenCloseTest::DoRunOpenExposed()
	{
	SCreateInfoTest TOpenInfo;
	TOpenInfo.iCookie =iChannelCookie;
	TOpenInfo.iDfcPriority = 3;
	
	const TInt desCount[3] = {0,1,128}; 
	const TBool dynChannel[3] =	{EFalse,EFalse,ETrue};  
	const TInt expectedResults[3] = {KErrArgument,KErrNone,KErrInUse};  
	TInt actualResults[3] = {1, 1, 1};

	for (TInt i =0; i<3; i++)
		{	
		TOpenInfo.iDesCount = desCount[i];
		TOpenInfo.iDynChannel = dynChannel[i];		

		// Open a single DMA channel
		RDebug::Printf("DoRunOpenExposed:Trying to open DMA channel using iDesCount(%d) and iDynChannel(%d)  ", TOpenInfo.iDesCount,TOpenInfo.iDynChannel);
		actualResults[i] = iDmaSession.ChannelOpen(iChannelSessionCookie, TOpenInfo);
		if (actualResults[i] == KErrNone)// Verify that channel is really open by closing DMA channel	
			{
			TInt err = iDmaSession.ChannelClose(iChannelSessionCookie);
			TEST_ASSERT(err == KErrNone)//Check that DMA channel closed with no errors
			}
		}

	// This case should fail if idesCount  =  0.
	// PIL has been changed to return KErrArgument instead of using an assertion check
	if (expectedResults[0] == actualResults[0])
		{
		RDebug::Printf("DoRunOpenExposed:DMA channel failed to open as expected as for iDesCount = 0 ");			
		}
	else
		{
		RDebug::Printf("DoRunOpenExposed:Error code returned (%d), expected KErrArgument as iDesCount= 0) ", actualResults[0]);	
		return EFalse;
		}

	// For this case( idesCount  =  1), DMA channel should open with no issues	
	if (expectedResults[1] == actualResults[1])
		{		
		RDebug::Printf("DoRunOpenExposed:DMA channel closes with no errors as expected for iDesCount = 1 ");
		}
	else
		{
		RDebug::Printf("DoRunOpenExposed:Failed to open DMA channel with error code (%d)", actualResults[1]);	
		return EFalse;
		}

	// For this case(dynaChannel=ETrue), DMA channel now returns KErrInUse. dynaChannel is not supported in the PSL.
	// PSL now returns a NULL pointer when dynaChannel is requested. The PIL interprets a NULL 
	// pointer being returned from opening a DMA channel as a channel in use. Hence, KErrInUse is returned.
	if (expectedResults[2] == actualResults[2])
		{
		RDebug::Printf("DoRunOpenExposed:DMA channel failed to open as expected as dynamic channel is not supported");		
		}
	else
		{
		RDebug::Printf("DoRunOpenExposed:Error code returned (%d), expected KErrInUse as as dynamic channel is not supported", actualResults[2]);			
		return EFalse;
		}

	return ETrue;
	}

void COpenCloseTest::RunTest()
	{
	OpenDmaSession(); 

	if (iRunOpen) 
	{	// Run Open() API test
		iOpenCloseResult = DoRunOpenExposed();
		if(iOpenCloseResult)
			iOpenCloseResult = DoRunOpen();
	}
	else
	{
		// Run Close() API test	
		iOpenCloseResult = DoRunClose();
	}

	CloseDmaSession();
	}

void COpenCloseTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Close/Open API Test"));

	}

TBool COpenCloseTest::Result()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("Results for Close/Open API Test");
		}

	if(!iOpenCloseResult)
		{
		RDebug::Printf("Open/Close test sequence failed"); 
		}
			
	return iOpenCloseResult;
	}
//////////////////////////////////////////////////////////////////////
// CDmaBmTransfer
//////////////////////////////////////////////////////////////////////
CDmaBmTransfer::CDmaBmTransfer(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize)
	:CDmaBenchmark(aName, aIterations,
		TResultSet(KErrNone, TRequestResults(),	KErrUnknown, TCallbackRecord(TCallbackRecord::EThread,1)),
		aTransferArgs, aMaxFragmentSize)
	{}

void CDmaBmTransfer::PrintTestType() const
	{
	RDebug::RawPrint(_L("Transfer Benchmark"));
	}

void CDmaBmTransfer::RunTest()
	{
	OpenDmaSession();

	OpenChannel();
	CreateDmaRequest();
	Fragment();
	Queue();
	FreeRequest();
	CloseChannel();

	CloseDmaSession();
	}

void CDmaBmTransfer::Queue()
	{
	if(iActual.iRequestResult.iFragmentationResult == KErrNone)
		{
		TUint64 time;
		iActual.iRequestResult.iQueueResult = iDmaSession.QueueRequest(iRequestSessionCookie, &iActual.iCallbackRecord, &time);
		iResultArray.Append(time);
		}
	}

/*
1.	Open a DMA channel for a transfer.
2.	Queue multiple request on the DMA channel.
3.	Call CancelAll () on the DMA channel.
4.	Verify that all transfers have been cancelled.
5.	Open a DMA channel for a transfer. This channel should support pause and resume.
6.	Call Pause () on the channel.
7.	Queue multiple request on the DMA channel.
8.	Call CancelAll () on the channel.
9.	Verify that all transfers have been cancelled.

   Note: This check does not add results to TResultSet like some
   other tests as its operation is different. The test checks for 
   the the cancelllation of all transfers queued on a channel by
   calling iDmaSession.ChannelIsQueueEmpty(); 
*/

//////////////////////////////////////////////////////////////////////
// CCancelAllTest
//////////////////////////////////////////////////////////////////////


CCancelAllTest::CCancelAllTest(const TDesC& aName, TInt aIterations,
		const TDmaTransferArgs* aTransferArgs, const TResultSet* aResultSets,
		TInt aCount)
	:CMultiTransferTest(aName, aIterations, aTransferArgs, aResultSets, aCount)
	{}

void CCancelAllTest::RunTest()
	{
	OpenDmaSession();
	PreTransferSetup();

	// Open a DMA channel for a transfer.This channel should support pause and resume.
	OpenChannel();

	//Call Pause () on the channel
	RDebug::Printf("Pausing DMA Channel");
	ChannelPause(iChannelSessionCookie);
	
	// Queue multiple request on the DMA channel.
	CreateDmaRequests();
	Fragment();

	QueueRequestsAsync();

	// Call CancelAll () on the DMA channel and Verify that all transfers have been cancelled.
	TInt r = CancelAllRequests();
	TEST_ASSERT(r == KErrNone);

	//Call Resume () on the channel.
	RDebug::Printf("Cancel should clear Pause state: resuming channel should fail");
	ChannelResume(iChannelSessionCookie);		
	//TEST_ASSERT(r == KErrCompletion);

	r = DoPostTransferCheck();
	TEST_ASSERT(r == KErrNone);
	//Destroy request
    for(TInt i=0; i<2; i++)
		{
		r = iDmaSession.RequestDestroy(iRequestCookies[i]);
		TEST_ASSERT(r == KErrNone);
		}

	//Close DMA channel
	CloseChannel();

	CloseDmaSession();
	}

TInt CCancelAllTest::CancelAllRequests()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("CCancelAllTest::CancelAllRequests()");
		}
	TInt r = KErrGeneral;
	r  = iDmaSession.ChannelCancelAll(iChannelSessionCookie);
	if (r != KErrNone)
		return r;

	TBool queueEmpty;
	r = iDmaSession.ChannelIsQueueEmpty(iChannelSessionCookie,queueEmpty);
	if (r != KErrNone)
		return r;

	if(!queueEmpty)
		return KErrGeneral;

	if(gVerboseOutput)
		{
		RDebug::Printf("Both current and pending requests cancelled");
		}
	return KErrNone;
	}

void CCancelAllTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("CCancelAllTest"));
	}

void CCancelAllTest::QueueRequestsAsync()
	{
	if(iPauseWhileQueuing)
		{
		TInt r = iDmaSession.ChannelPause(iChannelSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}

	// Queue all the DMA requests asynchronously
	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		TResultSet& resultSet = iActualResults[i];
		if(resultSet.iRequestResult.iFragmentationResult != KErrNone)
			continue;

		TInt r = iDmaSession.QueueRequest(iRequestCookies[i], iDummyRequestStatus, &resultSet.iCallbackRecord, NULL);
		resultSet.iRequestResult.iQueueResult = r;
		}

	if(iPauseWhileQueuing)
		{
		TInt r = iDmaSession.ChannelResume(iChannelSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}
	}

//////////////////////////////////////////////////////////////////////
// CIsQueueEmptyTest
//////////////////////////////////////////////////////////////////////
CIsQueueEmptyTest::CIsQueueEmptyTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs* aTransferArgs,
		const TResultSet* aResultSets, TInt aCount)
	:CMultiTransferTest(aName, aIterations, aTransferArgs, aResultSets, aCount)
	{}

CIsQueueEmptyTest::CIsQueueEmptyTest(const CIsQueueEmptyTest& aOther)
	:CMultiTransferTest(aOther)
	{}

CIsQueueEmptyTest::~CIsQueueEmptyTest()
	{
	}

void CIsQueueEmptyTest::RunTest()
	{
	OpenDmaSession();
	PreTransferSetup();
	
	OpenChannel();

	CreateDmaRequests();
	Fragment();
	QueueRequests();

	TInt r = DoPostTransferCheck();
	TEST_ASSERT(r == KErrNone);

	CloseDmaSession();
	}

void CIsQueueEmptyTest::DoIsQueueEmpty()
	{
	TBool queueEmpty;
	TInt r = iDmaSession.ChannelIsQueueEmpty(iChannelSessionCookie,queueEmpty);
	TEST_ASSERT(r == KErrNone);

	if(queueEmpty)
		{
		RDebug::Printf("Verified that calling IsQueueEmpty() returns ETrue before calling Queue()");
		}
	else
		{
		RDebug::Printf("IsQueueEmpty() fails to return ETrue before calling Queue()");
		TEST_ASSERT(queueEmpty);	
		}
	}

void CIsQueueEmptyTest::DoQueueNotEmpty()
	{
	TBool queueEmpty;
	TInt r = iDmaSession.ChannelIsQueueEmpty(iChannelSessionCookie,queueEmpty);
	TEST_ASSERT(r == KErrNone);

	if (!queueEmpty)
		{
		RDebug::Printf("Verified that calling IsQueueEmpty() returns EFalse after calling Queue()");
		}
	else
		{
		RDebug::Printf("IsQueueEmpty() fails to return EFalse after calling Queue()");
		TEST_ASSERT(!queueEmpty);
		}
	}

void CIsQueueEmptyTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("IsQueue Empty Test using Multi Transfer"));
	}

void CIsQueueEmptyTest::QueueRequests()
	{
	// Queue all the DMA requests asynchronously
	TInt i;
	RArray<TRequestStatus> requestStates;
	
	ChannelPause(iChannelSessionCookie);
	DoIsQueueEmpty();

	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	for(i=0; i<iTransferArgsCount; i++)
		{
		TResultSet& resultSet = iActualResults[i];
		if(resultSet.iRequestResult.iFragmentationResult != KErrNone)
			continue;

		TInt r = requestStates.Append(TRequestStatus());
		TEST_ASSERT(r == KErrNone);

		r = iDmaSession.QueueRequest(iRequestCookies[i], requestStates[i], &resultSet.iCallbackRecord, NULL);
		resultSet.iRequestResult.iQueueResult = r;

		DoQueueNotEmpty();
		}
	
	ChannelResume(iChannelSessionCookie);

	// wait for all transfers to complete
	const TInt count = requestStates.Count();

	for(i=0; i<count; i++)
		{
		User::WaitForRequest(requestStates[i]);
		}

	requestStates.Close();
	}

//////////////////////////////////////////////////////////////////////
// CMultiTransferTest
//////////////////////////////////////////////////////////////////////
CMultiTransferTest::CMultiTransferTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs* aTransferArgs,
		const TResultSet* aResultSets, TInt aCount)
	: CDmaTest(aName, aIterations, NULL, NULL), iTransferArgs(aTransferArgs), iTransferArgsCount(aCount), iNewDmaApi(ETrue),
	iChannelSessionCookie(0), iExpectedArray(aResultSets), iPauseWhileQueuing(EFalse)
	{}

CMultiTransferTest::CMultiTransferTest(const CMultiTransferTest& aOther)
	: CDmaTest(aOther), iTransferArgs(aOther.iTransferArgs), iTransferArgsCount(aOther.iTransferArgsCount),
	iNewDmaApi(aOther.iNewDmaApi),
	iExpectedArray(aOther.iExpectedArray), iPauseWhileQueuing(aOther.iPauseWhileQueuing)
	{
	CopyL(aOther.iRequestCookies, iRequestCookies);
	}

CMultiTransferTest::~CMultiTransferTest()
	{
	iRequestCookies.Close();
	iActualResults.Close();
	}

TBool CMultiTransferTest::Result()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("Results for %d transfers:", iTransferArgsCount);
		}

	TBool result = EFalse;
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		result = Result(i);
		if(!result)
			break;
		}
	return result;
	}

TBool CMultiTransferTest::Result(TInt aTransfer)
	{
	const TResultSet& expected = iExpectedArray[aTransfer];
	const TResultSet& actual = iActualResults[aTransfer];
	const TBool result = expected == actual;
	if(!result || gVerboseOutput)
		{
		RDebug::Printf("Compairing results for transfer %d", aTransfer);
		}

	if(!result)
		{
		RDebug::Printf("TResultSets do not match");
		}
	if(!result || gVerboseOutput)
		{
		RDebug::Printf("\nExpected error codes:");
		expected.Print();
		RDebug::Printf("Expected callback record:");
		expected.iCallbackRecord.Print();

		RDebug::Printf("\nActual error codes:");
		actual.Print();
		RDebug::Printf("Actual callback record:");
		actual.iCallbackRecord.Print();
		}
	return result;
	}
void CMultiTransferTest::RunTest()
	{
	OpenDmaSession();

	PreTransferSetup();
	OpenChannel();

	CreateDmaRequests();
	Fragment();

	QueueRequests();

	TInt r = DoPostTransferCheck();
	TEST_ASSERT(r == KErrNone);

	CloseDmaSession();
	}

void CMultiTransferTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("Multi Transfer"));
	}

const TDmaTransferArgs& CMultiTransferTest::TransferArgs(TInt aIndex) const
	{
	TEST_ASSERT(Rng(0, aIndex, iTransferArgsCount-1));

	return iTransferArgs[aIndex];
	}

void CMultiTransferTest::SetPostTransferResult(TInt aIndex, TInt aErrorCode)
	{
	TEST_ASSERT(Rng(0, aIndex, iTransferArgsCount-1));

	iActualResults[aIndex].iPostTransferCheck = aErrorCode;
	}

void CMultiTransferTest::OpenChannel()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("CMultiTransferTest::OpenChannel()");
		}
	TInt r = iDmaSession.ChannelOpen(iChannelCookie, iChannelSessionCookie);

	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		// In a multi transfer test a series of requests are created and queued.
		// They all use the same channel, is opened here at the beginning of the test
		//
		// Each transfer has a TResultSet which holds a result for the channel opening,
		// which we store here. Although in this case it is redundant,
		// in future it might be that different transfers open
		// different channels.
		iActualResults[i].iChannelOpenResult = r;
		}
	}

TInt CMultiTransferTest::CloseChannel()
	{
	return iDmaSession.ChannelClose(iChannelSessionCookie);
	}

void CMultiTransferTest::CreateDmaRequests()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("CMultiTransferTest::CreateDmaRequests() %d", iTransferArgsCount);
		}
	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	//create a DMA request for each transfer arg struct
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		if(iActualResults[i].iChannelOpenResult != KErrNone)
			continue;

		TUint cookie = 0;
		TInt r = KErrGeneral;

		if(iNewDmaApi)
			{
			r = iDmaSession.RequestCreate(iChannelSessionCookie, cookie);
			}
		else
			{
			r = iDmaSession.RequestCreateOld(iChannelSessionCookie, cookie);
			}
		iActualResults[i].iRequestResult.iCreate = r;

		if(r == KErrNone)
			{
			r = iRequestCookies.Append(cookie);
			TEST_ASSERT(r == KErrNone);
			}
		}
	}

void CMultiTransferTest::Fragment()
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("CMultiTransferTest::Fragment() %d", iTransferArgsCount);
		}
	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	// Fragment each dma request
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		TRequestResults& result = iActualResults[i].iRequestResult;
		if(result.iCreate != KErrNone)
			continue;

		TInt r = KErrGeneral;
		if(iNewDmaApi)
			r = iDmaSession.FragmentRequest(iRequestCookies[i], iTransferArgs[i]);
		else
			r = iDmaSession.FragmentRequestOld(iRequestCookies[i], iTransferArgs[i]);

		result.iFragmentationResult = r;
		}
	}

void CMultiTransferTest::QueueRequests()
	{
	if(iPauseWhileQueuing)
		{
		TInt r = iDmaSession.ChannelPause(iChannelSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}

	// Queue all the DMA requests asynchronously
	TInt i;
	RArray<TRequestStatus> requestStates;

	TEST_ASSERT(iActualResults.Count() == iTransferArgsCount);
	for(i=0; i<iTransferArgsCount; i++)
		{
		TResultSet& resultSet = iActualResults[i];
		if(resultSet.iRequestResult.iFragmentationResult != KErrNone)
			continue;

		TInt r = requestStates.Append(TRequestStatus());
		TEST_ASSERT(r == KErrNone);

		r = iDmaSession.QueueRequest(iRequestCookies[i], requestStates[i], &resultSet.iCallbackRecord, NULL);
		resultSet.iRequestResult.iQueueResult = r;
		}

	if(iPauseWhileQueuing)
		{
		TInt r = iDmaSession.ChannelResume(iChannelSessionCookie);
		TEST_ASSERT(r == KErrNone);
		}

	// wait for all transfers to complete
	const TInt count = requestStates.Count();

	for(i=0; i<count; i++)
		{
		User::WaitForRequest(requestStates[i]);
		}

	requestStates.Close();
	}

void CMultiTransferTest::PreTransferSetup()
	{
	// TODO this is the wrong place to do this!
	for(TInt i=0; i<iTransferArgsCount; i++)
		{
		//pre-fill actual results with error values
		TInt r = iActualResults.Append(TResultSet(EFalse));
		TEST_ASSERT(r == KErrNone);
		}
	if(iPreTransfer)
		iPreTransfer->Setup(*this); //initialize test
	}

TInt CMultiTransferTest::DoPostTransferCheck()
	{
	if(iPostTransferCheck)
		return iPostTransferCheck->Check(*this);
	else
		return KErrNone;
	}
//////////////////////////////////////////////////////////////////////
// CIsrRequeTest
//////////////////////////////////////////////////////////////////////
CIsrRequeTest::CIsrRequeTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs& aArgs,
			TIsrRequeArgs* aRequeueArgs, TInt aCount,
			const TResultSet& aExpected,const MPreTransfer* aPreTfer,const MPostTransferCheck* aPostTferChk, TUint aMaxFragmentSize)
	:CSingleTransferTest(aName, aIterations, aArgs, aExpected, aMaxFragmentSize, aPostTferChk, aPreTfer), iRequeArgSet(aRequeueArgs, aCount)
	{}

void CIsrRequeTest::Queue()
	{
	if(iActual.iRequestResult.iFragmentationResult == KErrNone)
		{
		iActual.iRequestResult.iQueueResult = iDmaSession.QueueRequestWithRequeue(iRequestSessionCookie, iRequeArgSet.iRequeArgs, iRequeArgSet.iCount, &iActual.iCallbackRecord);
		}
	}

void CIsrRequeTest::PrintTestType() const
	{
	RDebug::RawPrint(_L("ISR Requeue"));
	}

void CIsrRequeTest::PreTransferSetup()
	{
	if(iPreTransfer)
		iPreTransfer->Setup(*this); //initialize test
	}

TInt CIsrRequeTest::DoPostTransferCheck()
	{
	return iPostTransferCheck->Check(*this);
	}

//////////////////////////////////////////////////////////////////////
// TResultSet
//////////////////////////////////////////////////////////////////////
void TResultSet::Print() const
	{
	PRINT(iChannelOpenResult);
	PRINT(iRequestResult.iCreate);
	PRINT(iRequestResult.iFragmentCount);
	PRINT(iRequestResult.iFragmentationResult);
	PRINT(iRequestResult.iQueueResult);
	PRINT(iPostTransferCheck);
	}

TBool TResultSet::operator == (const TResultSet& aOther) const
	{
	return (memcompare((TUint8*)this, sizeof(*this), (TUint8*)&aOther, sizeof(aOther)) == 0);
	}

//////////////////////////////////////////////////////////////////////
// MPostTransferCheck classes
//////////////////////////////////////////////////////////////////////
TInt TCompareSrcDst::Check(const CSingleTransferTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("Comparing CSingleTransferTest buffers");
		}
	return Check(aTest.TransferArgs(), aTest.Chunk().Base());
	}

// Note: this check will not deal correctly with transfers were subsequent
// requeues overlap previous sources or destinations
// or where the source of transfer depends on a previous transfer.
// This is because it simply compares the source and destination
// pairwise for each transfer
//
// If TPreTransferIncrBytes is used for the pre-test then the transfers
// will be checked however.
TInt TCompareSrcDst::Check(const CIsrRequeTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("Comparing CIsrRequeTest buffers");
		}
	TUint8* chunkBase = aTest.Chunk().Base();
	const TDmaTransferArgs& transferArgs = aTest.TransferArgs();
	// check first transfer
	TInt r = Check(transferArgs, chunkBase);

	if(r != KErrNone)
		return r;

	// check re-queued transfers
	const TIsrRequeArgsSet& requeueArgs = aTest.GetRequeueArgs();
	return Check(requeueArgs, chunkBase, transferArgs);
	}

TInt TCompareSrcDst::Check(const TDmaTransferArgs& aTransferArgs, TUint8* aChunkBase) const
	{	
	const TUint32 srcOffset = aTransferArgs.iSrcConfig.iAddr;
	const TUint32 dstOffset = aTransferArgs.iDstConfig.iAddr;
	const TInt size = aTransferArgs.iTransferCount;

	const TUint8* src = srcOffset + aChunkBase;
	const TUint8* dst = dstOffset + aChunkBase;

	if(gVerboseOutput)
		{
		RDebug::Printf("Comparing TDmaTransferArgs buffers src=0x%08x dst=0x%08x size=0x%08x",
				src, dst, size);
		}

	return memcompare(src, size, dst, size);
	}

TInt TCompareSrcDst::Check(const TIsrRequeArgsSet& aRequeueArgSet, TUint8* aChunkBase, const TDmaTransferArgs& aTferArgs) const
	{
	TIsrRequeArgsSet argSet(aRequeueArgSet); //copy since Fixup will mutate object

	argSet.Substitute(aTferArgs); // replace any default (0) values with the values in aTferArgs

	argSet.Fixup((TLinAddr)aChunkBase); //convert address offsets to virtual user mode addresses

	TInt r = KErrCorrupt;
	while(!argSet.IsEmpty())
		{
		r = Check(argSet.GetArgs());
		if(r != KErrNone)
			break;
		}
	return r;
	}

TInt TCompareSrcDst::Check(const TIsrRequeArgs& aRequeueArgs) const
	{
	const TUint8* src = (TUint8*)aRequeueArgs.iSrcAddr;
	const TUint8* dst = (TUint8*)aRequeueArgs.iDstAddr;
	const TInt size = aRequeueArgs.iTransferCount;

	if(gVerboseOutput)
		{
		RDebug::Printf("Comparing TIsrRequeArgs: src=0x%08x dst=0x%08x size=0x%08x",
				src, dst, size);
		}

	return memcompare(src, size, dst, size);
	}

// Note: this check will not deal correctly with transfers were subsequent
// requeues overlap previous sources or destinations
// or where the source of trasnfer depends on a previous trasnfer.
// This is because it simply compares the source and destination
// pairwise for each transfer
//
// If TCompareSrcDst is used for the pre-test then the transfers
// will be checked however.
TInt TCompareSrcDst::Check(CMultiTransferTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("Comparing CMultiTransferTest buffers");
		}

	const TInt transferCount = aTest.TransferCount();
	TUint8* const chunkBase = aTest.Chunk().Base();

	// check buffers for each transfer
	for(TInt i=0; i<transferCount; i++)
		{
		TInt r = Check(aTest.TransferArgs(i), chunkBase);
		aTest.SetPostTransferResult(i, r);
		}
	// CMultiTransferTest is handled differently to the others.
	// Whereas CSingleTransferTest logs just the return value
	// of the check, here, we write back a result for each transfer
	// so the return value from this function is not important
	return KErrNone;
	}

TInt TCompare2D::Check(const CSingleTransferTest& aTest) const
	{
	const TDmaTransferArgs& args = aTest.TransferArgs();
	TUint8* const chunkBase = aTest.Chunk().Base();

	TInt ret = KErrNone;

	TTransferIter src_iter(args.iSrcConfig, chunkBase);
	TTransferIter dst_iter(args.iDstConfig, chunkBase);
	TTransferIter end;
	for (; (src_iter != end) && (dst_iter !=end); ++src_iter, ++dst_iter)
		{
		if(*src_iter != *dst_iter)
			{
			ret = KErrCorrupt;
			break;
			}
		}
	return ret;
	}

TInt TCompare2D::Check(const CIsrRequeTest&) const
	{
	return KErrNotSupported;
	}

TInt TCompare2D::Check(CMultiTransferTest&) const
	{
	return KErrNotSupported;
	}


TInt TCheckNoTransfer::Check(const CSingleTransferTest&) const
	{
	return KErrNotSupported;
	}

TInt TCheckNoTransfer::Check(const CIsrRequeTest&) const
	{
	return KErrNotSupported;
	}

TInt TCheckNoTransfer::Check(CMultiTransferTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("TCheckNoTransfer Comparing CMultiTransferTest buffers");
		}

	const TInt transferCount = aTest.TransferCount();
	TUint8* const chunkBase = aTest.Chunk().Base();

	// check buffers for each transfer
	for(TInt i=0; i<transferCount; i++)
		{
		TInt r = KErrCorrupt;
		if(IsZeroed(aTest.TransferArgs(i), chunkBase))
			{
			r = KErrNone;
			}

		aTest.SetPostTransferResult(i, r);
		}
	// CMultiTransferTest is handled differently to the others.
	// Whereas CSingleTransferTest logs just the return value
	// of the check, here, we write back a result for each transfer
	// so the return value from this function is not important
	return KErrNone;
	}

TBool TCheckNoTransfer::IsZeroed(const TDmaTransferArgs& aTransferArgs, TUint8* aChunkBase) const
	{
	TAddressParms parms = GetAddrParms(aTransferArgs);
	parms.Fixup(reinterpret_cast<TLinAddr>(aChunkBase));
	const TAddrRange destination = parms.DestRange();

	return destination.IsFilled(0);
	}
//////////////////////////////////////////////////////////////////////
// MPreTransfer classes
//////////////////////////////////////////////////////////////////////
void TPreTransferIncrBytes::Setup(const CSingleTransferTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("TPreTransferIncrBytes(CSingleTransferTest)");
		}
	TAddressParms params = GetAddrParms(aTest.TransferArgs());

	TUint8* const chunkBase = aTest.Chunk().Base();
	params.Fixup((TLinAddr)chunkBase);


	Setup(params);
	}

void TPreTransferIncrBytes::Setup(const TAddressParms& aParams) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("TPreTransferIncrBytes: setup memory buffers: src=0x%08x dst=0x%08x size=0x%08x",
				aParams.iSrcAddr, aParams.iDstAddr, aParams.iTransferCount);
		}
	TUint8* const src = (TUint8*) aParams.iSrcAddr;
	const TInt size = aParams.iTransferCount;

	for(TInt i=0; i<size; i++)
		{src[i] = (TUint8)i;} //each src byte holds its own offset (mod 256)

	TUint8* const dst = (TUint8*) aParams.iDstAddr;
	memclr(dst, size); //clear destination
	}

void TPreTransferIncrBytes::Setup(const CIsrRequeTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("TPreTransferIncrBytes(CIsrRequeTest)");
		}
	if(!CheckBuffers(aTest))
		{
		RDebug::Printf("Successive transfer destinations may not overlap previous src or dst buffers");
		RDebug::Printf("unless the whole transfer is an exact repeat of a previous one");
		TEST_FAULT;
		}

	Setup(static_cast<CSingleTransferTest>(aTest)); // prepare the CSingleTransferTest parts

	TIsrRequeArgsSet requeSet(aTest.GetRequeueArgs());

	requeSet.Substitute(aTest.TransferArgs());

	const TLinAddr chunkBase = (TLinAddr) aTest.Chunk().Base();
	requeSet.Fixup(chunkBase);

	while(!requeSet.IsEmpty())
		{
		TIsrRequeArgs args = requeSet.GetArgs();
		Setup(args); // perform the setup operation for each TIsrRequeArgs
		}
	}

void TPreTransferIncrBytes::Setup(const CMultiTransferTest& aTest) const
	{
	if(gVerboseOutput)
		{
		RDebug::Printf("TPreTransferIncrBytes(CMultiTransferTest)");
		}

	if(!CheckBuffers(aTest))
		{
		RDebug::Printf("Successive transfer destinations may not overlap previous src or dst buffers");
		TEST_FAULT;
		}

	TUint8* const chunkBase = aTest.Chunk().Base();
	const TInt transferCount = aTest.TransferCount();

	// initialise buffers for each transfer
	for(TInt i=0; i<transferCount; i++)
		{
		TAddressParms params = GetAddrParms(aTest.TransferArgs(i));

		params.Fixup((TLinAddr)chunkBase);

		Setup(params);
		}
	}

TBool TPreTransferIncrBytes::CheckBuffers(const CIsrRequeTest& aTest) const
	{
	RArray<const TAddressParms> array;
	array.AppendL(TAddressParms(aTest.TransferArgs()));

	TIsrRequeArgsSet requeSet(aTest.GetRequeueArgs());
	requeSet.Substitute(aTest.TransferArgs());

	const TLinAddr chunkBase = (TLinAddr) aTest.Chunk().Base();
	requeSet.Fixup(chunkBase);
	while(!requeSet.IsEmpty())
		{
		const TIsrRequeArgs requeArgs = requeSet.GetArgs();
		array.AppendL(requeArgs);
		}

	const TBool result = CheckBuffers(array);

	array.Close();
	return result;
	}

/**
A CMultiTransferTest will wait for all transfers to complete
before comapairing source and destination buffers. For this to be successful
each transfer must be independent ie. no destination or source may be
overwritten by another transfer and source buffers may not depend on an
earlier transfer
*/
TBool TPreTransferIncrBytes::CheckBuffers(const CMultiTransferTest& aTest) const
	{
	TUint8* const chunkBase = aTest.Chunk().Base();
	const TInt transferCount = aTest.TransferCount();

	// assemble an array of TAddressParams from aTest, that
	// can then be passed to CheckBuffers(RArray<TAddressParms>)
	RArray<const TAddressParms> array;

	for(TInt i=0; i<transferCount; i++)
		{
		TAddressParms params = GetAddrParms(aTest.TransferArgs(i));
		params.Fixup((TLinAddr)chunkBase);

		array.AppendL(params);
		}

	 // 2nd arg EFalse as there is no need to permit exact repeats
	const TBool r = CheckBuffers(array, EFalse);

	array.Close();
	return r;
	}

/**
Check that the destination of each TAddressParms does not overlap with
any previous source or destination or that if it does the whole transfer
matches.
This is so that successive transfers do not overwrite the destinations or
sources of preceeding ones.

If aAllowExactRepeat is true then exactly matching transfers are allowed
to test the case where a repeat transfer is required - though it can't
then be determined just from looking at the buffers that the repeat was
successful
*/
TBool TPreTransferIncrBytes::CheckBuffers(const RArray<const TAddressParms>& aTransferParams, TBool aAllowExactRepeat) const
	{

	const TInt count = aTransferParams.Count();

	if(gVerboseOutput)
		{
		RDebug::Printf("CheckBuffers, %d transfers", count);
		}

	TBuf<128> buf;
	for(TInt i=1; i<count; i++)
		{
		const TAddressParms& current = aTransferParams[i];

		if(gVerboseOutput)
			{
			buf.Zero();
			current.AppendString(buf);
			RDebug::Print(_L("Check current: %S, against:"), &buf);
			}

		for(TInt j=0; j<i; j++)
			{
			const TAddressParms& previous = aTransferParams[j];
			if(gVerboseOutput)
				{
				buf.Zero();
				previous.AppendString(buf);
				RDebug::Print(_L("Previous: %S"), &buf);
				}

			const TBool curDstOverlapsPrevTfer = previous.Overlaps(current.DestRange());
			const TBool curSrcDependsOnPrevDest = current.SourceRange().Overlaps(previous.DestRange());
			const TBool permitExactRepeat = aAllowExactRepeat && (current == previous);

			const TBool ok = !(curDstOverlapsPrevTfer || curSrcDependsOnPrevDest) || permitExactRepeat;
			if(!ok)
				return EFalse;
			}
		}
	return ETrue;
	}
//////////////////////////////////////////////////////////////////////
// TTransferIter class
//////////////////////////////////////////////////////////////////////
void TTransferIter::operator++ ()
	{
	iPtr++; //the standard post increment
	if(iElem < (iCfg->iElementsPerFrame-1))
		{
		iPtr += iCfg->iElementSkip;
		iElem++;
		iBytes++;
		}
	else
		{
		TEST_ASSERT(iElem == iCfg->iElementsPerFrame-1);
		if(iFrame < iCfg->iFramesPerTransfer-1)
			{
			iPtr += iCfg->iFrameSkip;
			iFrame++;
			iBytes++;
			iElem = 0;
			}
		else
			{
			//we have reached the end
			TEST_ASSERT(iFrame == iCfg->iFramesPerTransfer-1);
			iPtr = NULL;
			}
		}

	Invariant();
	}

void TTransferIter::Invariant() const
	{
	const TInt elemSize = iCfg->iElementSize;
	RTest test(_L("TTransferIter invariant"));
	const TUint bytesTransfered = (
			elemSize * (iFrame * iCfg->iElementsPerFrame + iElem)
			+ ((TUint)iPtr % (elemSize))
			);
	test_Equal(iBytes, bytesTransfered);
	test.Close();
	}

///////////////////////////////////////////////////////////
// TTestCase
///////////////////////////////////////////////////////////
TTestCase::TTestCase(CDmaTest* aTest,
   TBool aConcurrent,
   const TDmaCapability aCap1,
   const TDmaCapability aCap2,
   const TDmaCapability aCap3,
   const TDmaCapability aCap4,
   const TDmaCapability aCap5
   )
:
	iTest(aTest), iConcurrentTest(aConcurrent)
	{
	iChannelCaps[0] = aCap1;
	iChannelCaps[1] = aCap2;
	iChannelCaps[2] = aCap3;
	iChannelCaps[3] = aCap4;
	iChannelCaps[4] = aCap5;
	}

TResult TTestCase::TestCaseValid(const SDmacCaps& aChannelCaps) const
	{
	const TDmaCapability* cap = &iChannelCaps[0];

	TResult ret = ERun;
	//We assume that the array is empty at the first ENone found
	//any caps after this wil be ignored
	while(cap->iCapsReq != ENone)
		{
		TResult t = cap->CompareToDmaCaps(aChannelCaps);
		if(t > ret) //this relies on the enum ordering
			ret = t;
		cap++;
		}
	return ret;
	}

TResult TTestCase::TestCaseValid(const TDmacTestCaps& aChannelCaps) const
	{
	const TDmaCapability* cap = &iChannelCaps[0];

	TResult ret = ERun;
	//We assume that the array is empty at the first ENone found
	//any caps after this wil be ignored
	while(cap->iCapsReq != ENone)
		{
		TResult t = cap->CompareToDmaCaps(aChannelCaps);
		if(t > ret) //this relies on the enum ordering
			ret = t;
		cap++;
		}
	return ret;
	}
/**
Will report whether a value held in aChannelCaps satisfies a
requirement specfied by this object
*/
TBool TDmaCapability::RequirementSatisfied(const SDmacCaps& aChannelCaps) const
	{
	switch(iCapsReq)
		{
	case ENone:
		return ETrue;
	case EChannelPriorities:
		TEST_FAULT;
	case EChannelPauseAndResume:
		return aChannelCaps.iChannelPauseAndResume == (TBool)iValue;
	case EAddrAlignedToElementSize:
		TEST_FAULT;
	case E1DAddressing:
		return aChannelCaps.i1DIndexAddressing == (TBool)iValue;
	case E2DAddressing:
		return aChannelCaps.i2DIndexAddressing == (TBool)iValue;
	case ESynchronizationTypes:
	case EBurstTransactions:
	case EDescriptorInterrupt:
	case EFrameInterrupt:
	case ELinkedListPausedInterrupt:
	case EEndiannessConversion:
	case EGraphicsOps:
	case ERepeatingTransfers:
	case EChannelLinking:	
		return aChannelCaps.iChannelLinking == (TBool)iValue;
	case EHwDescriptors:
		return aChannelCaps.iHwDescriptors == (TBool)iValue;
	case ESrcDstAsymmetry:
	case EAsymHwDescriptors:
		TEST_FAULT;
	case EBalancedAsymSegments:
		return aChannelCaps.iBalancedAsymSegments == (TBool)iValue;
	case EAsymCompletionInterrupt:
		return aChannelCaps.iAsymCompletionInterrupt == (TBool)iValue;
	case EAsymDescriptorInterrupt:
		return aChannelCaps.iAsymDescriptorInterrupt == (TBool)iValue;
	case EAsymFrameInterrupt:
		return aChannelCaps.iAsymFrameInterrupt == (TBool)iValue;
	default:
		TEST_FAULT;
		}

	return EFalse;
	}

/**
Will report whether a value held in aChannelCaps satisfies a
requirement specfied by this object
*/
TBool TDmaCapability::RequirementSatisfied(const TDmacTestCaps& aChannelCaps) const
	{
	switch(iCapsReq)
		{
	case EPilVersion:
		return TestValue(aChannelCaps.iPILVersion);
	default:
		return RequirementSatisfied(static_cast<SDmacCaps>(aChannelCaps));
		}
	}

TResult TDmaCapability::CompareToDmaCaps(const SDmacCaps& aChannelCaps) const
	{
	const TBool reqSatisfied = RequirementSatisfied(aChannelCaps);
	if(reqSatisfied)
		{
		return ERun;
		}
	else
		{
		return iFail ? EFail : ESkip;
		}
	}

TResult TDmaCapability::CompareToDmaCaps(const TDmacTestCaps& aChannelCaps) const
	{
	const TBool reqSatisfied = RequirementSatisfied(aChannelCaps);
	if(reqSatisfied)
		{
		return ERun;
		}
	else
		{
		return iFail ? EFail : ESkip;
		}
	}
/**
Test that aValue satisfies the comparrison (iCapsReqType) with the
reference value held in iValue
*/
TBool TDmaCapability::TestValue(TUint aValue) const
	{
	switch(iCapsReqType)
		{
	case EEqual:
		return aValue == iValue;
	case EGTE:
		return aValue >= iValue;
	case ELTE:
		return aValue <= iValue;
	case EBitsSet:
	case EBitsClear:
	default:
		TEST_FAULT;
		}
	return EFalse;
	}

static RTest test(_L("DMAv2 test"));

//////////////////////////////////////////////////////////////////////
// TTestRunner
//////////////////////////////////////////////////////////////////////
TTestRunner::TTestRunner()
	{
	// Open RDmaSession handle
	TInt r = iDmaSession.Open();
	TEST_ASSERT(r == KErrNone);

	// Get PSI Test info
	r = iDmaSession.GetTestInfo(iPslTestInfo);
	TEST_ASSERT(r == KErrNone);

	//Retrieve PSL cookies
	GetPslCookie();

	//Generate the DMA channel records
	GenerateChannelRecord();
	}

TTestRunner::~TTestRunner()
	{
	RTest::CloseHandleAndWaitForDestruction(iDmaSession);
	iTestCases.Close(); //TestRunner does not own test cases
	iChannelRecords.Close();
	iPslCookies.Close();
	}

void TTestRunner::AddTestCases(RPointerArray<TTestCase>& aTTestCases)
	{
	const TInt count = aTTestCases.Count();
	for(TInt i=0; i < count; i++)
		{
		iTestCases.AppendL(aTTestCases[i]);
		}
	}

void TTestRunner::RunTests()
	{
	//Print PslTestInfo
	if(gVerboseOutput)
		{
		Print(iPslTestInfo);
		}

	//iterate through the test case array
	const TInt testCaseCount = iTestCases.Count();
	for(TInt i=0; i < testCaseCount; i++)
		{
		const TTestCase& testCase = *iTestCases[i];

		//Here, we must create a test thread for each channel
		RPointerArray<CTest> concurrentTests;

		const TInt chanRecCount = iChannelRecords.Count();
		for(TInt j=0; j < chanRecCount; j++)
			{
			const TChannelRecord& record = iChannelRecords[j];
			const TDmacTestCaps& caps = record.iChannelCaps;

			const TResult t = testCase.TestCaseValid(caps);

			switch(t)
				{
			case ERun:
				{
				CDmaTest* dmaTest = static_cast<CDmaTest*>(testCase.iTest->Clone());
				TEST_ASSERT(dmaTest != NULL);

				dmaTest->SetChannelCookie(record.iCookie);
				dmaTest->SetupL();
				if(testCase.iConcurrentTest)
					{
					//Add test to array to be run concurrently
					TInt r = concurrentTests.Append(dmaTest);
					TEST_ASSERT(r == KErrNone);
					}
				else
					{
					dmaTest->Announce();
					//Run test in this thread
					(*dmaTest)();
					TBool result = dmaTest->Result();
					TEST_ASSERT(result);

					delete dmaTest;
					}

				break;
				}
			case ESkip:
				if(gVerboseOutput)
				{
				RDebug::Printf("Skipping test-case %S, PSL channel %d", &testCase.iTest->Name(), record.iCookie);
				}
				break;
			case EFail:
				if(gVerboseOutput)
				{
				RDebug::Printf("Failling test-case %S, PSL channel %d", &testCase.iTest->Name(), record.iCookie);
				}
				TEST_FAULT;
			default:
				TEST_FAULT;
				}
			//Depending on the value of iConcurrentTest the test runner will either block until the thread has completed or
			//alternatively run the current test case on the next channel:

			//if the test case has been run on all channels it will then  wait for all threads to complete.
			}

		// Run the tests which should happen concurrently
		const TInt count = concurrentTests.Count();
		if(count>0)
			{
			RDebug::Printf("== Begin concurrent test run ==");

			TInt i;											// VC++
			for(i=0; i<count; i++)
				{
				concurrentTests[i]->Announce();
				}

			MultipleTestRun(concurrentTests);
			for(i=0; i<count; i++)
				{
				TBool result = static_cast<CDmaTest*>(concurrentTests[i])->Result();
				TEST_ASSERT(result);
				}
			RDebug::Printf("== End concurrent test run ==");
			}

		concurrentTests.ResetAndDestroy();
		}
	}

void TTestRunner::GetPslCookie()
	{
	//Get Sb Channel cookies
	for(TInt sb_channelcount=0; sb_channelcount<iPslTestInfo.iMaxSbChannels; sb_channelcount++)
		{
		iPslCookies.AppendL(iPslTestInfo.iSbChannels[sb_channelcount]);
		}

	//Get Db Channel cookies
	for(TInt db_channelcount=0; db_channelcount<iPslTestInfo.iMaxDbChannels; db_channelcount++)
		{
		iPslCookies.AppendL(iPslTestInfo.iDbChannels[db_channelcount]);
		}

	//Get Sg Channel cookies
	for(TInt sg_channelcount=0; sg_channelcount<iPslTestInfo.iMaxSgChannels; sg_channelcount++)
		{
		iPslCookies.AppendL(iPslTestInfo.iSgChannels[sg_channelcount]);
		}
	}

void TTestRunner::GenerateChannelRecord()
	{
	//for each PSL cookie
	for(TInt count=0; count<iPslCookies.Count(); count++)
		{
		//Get channel cookie
		const TUint pslCookie = iPslCookies[count];
		TUint sessionCookie;
		TInt r = iDmaSession.ChannelOpen(pslCookie, sessionCookie);
		TEST_ASSERT(r == KErrNone);
		if(gVerboseOutput)
		{
		RDebug::Printf("Channel PSL Cookie[%d]  :0x%08x",count,pslCookie);
		}

		TChannelRecord dmaChannelRecord;
		dmaChannelRecord.iCookie = pslCookie;

		//Get Channel Caps
		r = iDmaSession.ChannelCaps(sessionCookie, dmaChannelRecord.iChannelCaps);
		TEST_ASSERT(r == KErrNone);

		r = iDmaSession.ChannelClose(sessionCookie);
		TEST_ASSERT(r == KErrNone);

		//Append array
		iChannelRecords.AppendL(dmaChannelRecord);
		}
	}
//////////////////////////////////////////////////////////////////////
// Global test functions and E32Main
//////////////////////////////////////////////////////////////////////

/**
Displayed if used supplied no parameters, garbage, or a ? in the parameters
*/
void PrintUsage()
	{
	test.Printf(_L("*** DMA TEST FRAMEWORK ***\n"));
	test.Printf(_L("Usage : t_dma2.exe [/option]\n"));
	test.Printf(_L("  /V or /VERBOSE  = Control test output\n"));
	test.Printf(_L("  /S or /SELFTEST = Run DMA self tests\n"));
	test.Printf(_L("  /simple = Run only simple transfer tests\n"));
	test.Printf(_L("  /callback = Run only callback tests\n"));
	test.Printf(_L("  /multi = Run only multipart transfer tests\n"));
	test.Printf(_L("  /isrdfc = Run only isr and dfc tests\n"));
	test.Printf(_L("  /isreque = Run only isr reque tests\n"));
	test.Printf(_L("  /bench = Run only benchmark tests\n"));
	test.Printf(_L("  /suspend = Run only pause and resume tests\n"));
	test.Printf(_L("  /graphic = Run only graphic tests\n"));
	test.Printf(_L("  /channel = Run only DMA channel (opening and closing) tests\n"));
	test.Printf(_L("  /queue = Run only queue tests\n"));
	test.Printf(_L("  /fragment = Run only fragmentation related tests\n"));
	test.Printf(_L("  /request = Run only requests tests related tests\n"));
	test.Printf(_L("\n"));
	}

void ProcessCommandLineL()
{
	test.Printf(_L("Process command line arguments\n"));

	TInt cmdLineLength(User::CommandLineLength());
	HBufC* cmdLine = HBufC::NewMaxLC(cmdLineLength);
	TPtr cmdLinePtr = cmdLine->Des();
	User::CommandLine(cmdLinePtr);
	TBool  tokenParsed(EFalse);

	TLex args(*cmdLine);
	args.SkipSpace(); // args are separated by spaces
	
	// first arg is the exe name, skip it
	TPtrC cmdToken = args.NextToken();
	HBufC* tc = HBufC::NewLC(KParameterTextLenMax);
	*tc = cmdToken;
	while (tc->Length())
		{
		tokenParsed = EFalse;
		
		// '/?' help wanted flag '?' or /? parameter
		if ((0== tc->FindF(_L("?"))) || (0==tc->FindF(_L("/?")))) 
			{
			gHelpRequested = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/SELFTEST'			
		if ((0== tc->FindF(KArgSelfTest)) || (0==tc->FindF(KArgSelfTest2))) 
			{
			// Run self test
			test.Printf(_L("Command Line Options:Selftest option specified.\n"));
			gSelfTest = ETrue;
			tokenParsed = ETrue;
			}

		// '/VERBOSE' option	
		if ((0== tc->FindF(KArgVerboseOutput)) || (0==tc->FindF(KArgVerboseOutput2)))
			{ 
			test.Printf(_L("Command Line Options:Verbose option specified.\n"));
			gVerboseOutput = ETrue;
			tokenParsed = ETrue;			
			}

		// '/suspend' option
		if ((0== tc->FindF(KArgSuspendTest)))
			{
			gSuspend = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/simple' option
		if ((0== tc->FindF(KArgSimpleTest)))
			{
			gSimpleTest = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/multi' option
		if ((0== tc->FindF(KArgMultiPartTest)))
			{
			gMultiPart = ETrue;
			tokenParsed = ETrue;
			}	
	
		// '/callback' option
		if ((0== tc->FindF(KArgCallBackTest)))
			{
			gCallBack = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/IsrAndDfc' option
		if ((0== tc->FindF(KArgIsrDfcTest)))
			{
			gIsrAndDfc  = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/IsrReque' option
		if ((0== tc->FindF(KArgIsrequeTest)))
			{
			gIsrReque = ETrue;
			tokenParsed = ETrue;
			}	

		// '/Benchmark' option
		if ((0== tc->FindF(KArgBenchmarkTest)))
			{
			gBenchmark = ETrue;
			tokenParsed = ETrue;
			}	

		// '/Queue' option
		if ((0== tc->FindF(KArgQueueTest)))
			{
			gQueue = ETrue;
			tokenParsed = ETrue;
			}	

		// '/Fragment' option
		if ((0== tc->FindF(KArgFragmentTest)))
			{
			gFragment = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/Channel' option
		if ((0== tc->FindF(KArgChannelTest)))
			{
			gChannel = ETrue;
			tokenParsed = ETrue;
			}	
		
		// '/Graphic' option
		if ((0== tc->FindF(KArgGraphicTest)))
			{
			gGraphic = ETrue;
			tokenParsed = ETrue;
			}	

		// '/Request' option
		if ((0== tc->FindF(KArgRequestTest)))
			{
			gRequest = ETrue;
			tokenParsed = ETrue;
			}	
	
		if (!tokenParsed)
			{
			// warn about unparsed parameter
			test.Printf(_L("Warning: '%lS'??? not parsed\n"), tc);
			gHelpRequested = ETrue;
			}
			
		// next parameter
		*tc = args.NextToken();
		}
	CleanupStack::PopAndDestroy(tc);
	CleanupStack::PopAndDestroy(cmdLine);
}

void RunDMATests()
	{
	test.Start(_L("Creating test runner\n"));
	TTestRunner testRunner;

	test.Next(_L("Add global test cases to test runner\n"));

	if (gSimpleTest) //Add only simple tranfer test cases
	{
		testRunner.AddTestCases(TestArraySimple);
	}
	else if (gCallBack)  //Add only callback test cases
	{
		testRunner.AddTestCases(TestArrayCallback);
	}
	else if (gIsrReque)  //Add only ISR Reque test cases
	{
		testRunner.AddTestCases(TestArrayIsrReque);
	}
	else if (gMultiPart)  //Add only Multipart test cases
	{
		testRunner.AddTestCases(TestArrayMultiPart);
	}
	else if (gIsrAndDfc)  //Add only IsrAndDfc test cases
	{
		testRunner.AddTestCases(TestArrayIsrAndDfc);
	}
	else if (gBenchmark)  //Add only Benchmark test cases
	{
		testRunner.AddTestCases(TestArrayBenchmark);
	}
	else if (gGraphic)  //Add only 2D test cases
	{
		testRunner.AddTestCases(TestArray2DTest);
	}
	else if (gFragment)  //Add only Fragment test cases
	{
		testRunner.AddTestCases(TestArrayFragment);
	}
	else if (gChannel)  //Add only Channel test cases
	{
		testRunner.AddTestCases(TestArrayChannel);
	}
	else if (gSuspend)  //Add only Suspend test cases
	{
		testRunner.AddTestCases(TestArraySuspend);
	}
	else if (gQueue)  //Add only queue test cases
	{
		testRunner.AddTestCases(TestArrayQueue);
	}
	else if (gRequest)  //Add only request test cases
	{
		testRunner.AddTestCases(TestArrayRequest);
	}
	else
	{
		testRunner.AddTestCases(TestArray);//Add all test cases
	}

	test.Next(_L("call TTestRunner::RunTests()\n"));
	testRunner.RunTests();

	test.End();
	}


struct TSimTest
	{
	TUint iPslId;
	TBool iFragment;
	};

const TSimTest KSimTests[] =
	{
		{0, EFalse},
		{1, EFalse},
		{2, ETrue},
		{3, ETrue},
	};

const TInt KSimTestsCount = ARRAY_LENGTH(KSimTests);

void RunSimDMATests()
	{
	test.Start(_L("Run simulated DMAC tests\n"));

	test.Next(_L("Open session"));
	RDmaSession session;
	TInt r = session.OpenSim();
	test_KErrNone(r);

	for(TInt i=0; i<KSimTestsCount; i++)
		{
		TUint pslId = KSimTests[i].iPslId;
		TBool doFrag = KSimTests[i].iFragment;

		test.Start(_L("Open channel"));
		TUint channelCookie=0;
		r = session.ChannelOpen(pslId, channelCookie);
		test.Printf(_L("Open channel %d, cookie recived = 0x%08x\n"), pslId, channelCookie);
		test_KErrNone(r);

		test.Next(_L("Create Dma request"));

		TUint reqCookie=0;
		r = session.RequestCreate(channelCookie, reqCookie);
		test.Printf(_L("cookie recived = 0x%08x\n"), reqCookie );
		test_KErrNone(r);

		if(doFrag)
			{
			test.Next(_L("Fragment request"));
			const TInt size = 128 * KKilo;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);
			r = session.FragmentRequest(reqCookie, transferArgs);
			test_KErrNone(r);
			}

		test.Next(_L("Destroy Dma request"));
		r = session.RequestDestroy(reqCookie);
		test_KErrNone(r);

		test.Next(_L("Channel close"));
		r = session.ChannelClose(channelCookie);
		test_KErrNone(r);
		test.End();
		}

	test.Next(_L("Close session"));
	RTest::CloseHandleAndWaitForDestruction(session);

	test.End();
	}

TInt E32Main()
	{
	__UHEAP_MARK;
	test.Title();

	gHelpRequested = EFalse;
	TInt r;

	// Create the new trap-cleanup mechanism
	CTrapCleanup* cleanup = CTrapCleanup::New();

	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}

	// Process the command line parameters for batch/etc
	TRAPD(err, ProcessCommandLineL());
	if (err != KErrNone)
		{
		User::Panic(_L("DMA test run memory failure"), KErrNoMemory);
		}

	if (gHelpRequested)
		{
		PrintUsage();
		User::Leave(-2);	// nothing to do!
		}
	test.Start(_L("Loading test LDD"));
	//load either the new test ldd, d_dma2.ldd,
	//or d_dma2_compat.ldd - an ldd linked against
	//the old DMA framework
	_LIT(KDma, "D_DMA2.LDD");
	r = User::LoadLogicalDevice(KDma);
	const TBool dma2Loaded = ((r == KErrNone) || (r == KErrAlreadyExists));

	_LIT(KDma2Compat, "D_DMA2_COMPAT.LDD");
	r = User::LoadLogicalDevice(KDma2Compat);
	const TBool dma2CompatLoaded = ((r == KErrNone) || (r == KErrAlreadyExists));

	if (!(dma2Loaded || dma2CompatLoaded))
		{
		test.Printf(_L("Hardware DMA test driver not found - will run tests on simulated DMAC only\n"));
		}
	else if (dma2Loaded && !dma2CompatLoaded)
		{
		test.Printf(_L("Loaded %S\n"), &KDma);
		}
	else if (!dma2Loaded && dma2CompatLoaded)
		{
		test.Printf(_L("Loaded %S\n"), &KDma2Compat);
		}
	else
		{
		test.Printf(_L("The ROM contains %S and %S - only one should be present\n"), &KDma, &KDma2Compat);
		TEST_FAULT;
		}

	const TBool dmaHwPresent = (dma2Loaded || dma2CompatLoaded);

	_LIT(KDma2Sim, "D_DMA2_SIM.LDD");

	r = User::LoadLogicalDevice(KDma2Sim);
	const TBool dma2SimLoaded = ((r == KErrNone) || (r == KErrAlreadyExists));
	if (dma2SimLoaded)
		{
		test.Printf(_L("Loaded %S\n"), &KDma2Sim);
		}
	else
		{
		test.Printf(_L("Failed to load %S, r=%d\n"), &KDma2Sim, r);
		test(EFalse);
		}

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	RTest::CloseHandleAndWaitForDestruction(l);

	__KHEAP_MARK;

	if (gSelfTest) //Run self tests if specified on command line
		{
		SelfTests();
		}

	RunSimDMATests();
	if (dmaHwPresent)
		{
		RunDMATests();
		}

	// Wait for the supervisor thread to run and perform asynchronous
	// cleanup, so that kernel heap space will be freed
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);
	__KHEAP_MARKEND;

	if(dmaHwPresent)
		{
		r = User::FreeLogicalDevice(KTestDmaLddNameHw);
		test_KErrNone(r);
		}
	r = User::FreeLogicalDevice(KTestDmaLddNameSim);
	test_KErrNone(r);

	test.End();
	test.Close();

	delete cleanup;

	__UHEAP_MARKEND;
	return 0;
	}
