// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dma\t_dma.cpp

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

// SelfTest Option
_LIT(KArgSelfTest, "/SELFTEST");  
_LIT(KArgSelfTest2, "/S");		  

//Verbose Option
_LIT(KArgVerboseOutput, "/VERBOSE"); 
_LIT(KArgVerboseOutput2, "/V");	     
  

TBool gHelpRequested;   // print usage 
TBool gVerboseOutput;   // enable verbose output 
TBool gSelfTest;        // run SelfTest 

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
	TInt r = iDmaSession.Open();
	TEST_ASSERT(r == KErrNone);
	r = iDmaSession.OpenSharedChunk(iChunk);
	TEST_ASSERT(r == KErrNone);
	}

void CDmaTest::CloseDmaSession()
	{
	iChunk.Close();
	iDmaSession.Close();
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
			iDmaSession.RequestCreateNew(iChannelSessionCookie, iRequestSessionCookie, iMaxFragmentSize);
		}
	else
		{
		if(gVerboseOutput)
			{
			RDebug::Printf("Calling Old Request API\n");
			}
		iActual.iRequestResult.iCreate =
			iDmaSession.RequestCreate(iChannelSessionCookie, iRequestSessionCookie, iMaxFragmentSize);
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
// CDmaBenchmark
//////////////////////////////////////////////////////////////////////

CDmaBenchmark::CDmaBenchmark(const TDesC& aName, TInt aIterations, const TResultSet& aExpectedResults, const TDmaTransferArgs& aTransferArgs, TUint aMaxFragmentSize)
	:CSingleTransferTest(aName, aIterations, aTransferArgs, aExpectedResults, aMaxFragmentSize, NULL, NULL)
	{
	UseNewDmaApi(EFalse);
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

	//TODO this will be handled by the ctor later
	iResultArray.Close();

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


//////////////////////////////////////////////////////////////////////
// CMultiTransferTest
//////////////////////////////////////////////////////////////////////

//TODO
// Add pre and post transfer for CMultiTransferTest
CMultiTransferTest::CMultiTransferTest(const TDesC& aName, TInt aIterations, const TDmaTransferArgs* aTransferArgs,
		const TResultSet* aResultSets, TInt aCount)
	: CDmaTest(aName, aIterations, NULL, NULL), iTransferArgs(aTransferArgs), iTransferArgsCount(aCount), iNewDmaApi(ETrue),
	iChannelSessionCookie(0), iExpectedArray(aResultSets), iPauseWhileQueuing(EFalse)
	{}

CMultiTransferTest::CMultiTransferTest(const CMultiTransferTest& aOther)
	: CDmaTest(aOther), iTransferArgs(aOther.iTransferArgs), iTransferArgsCount(aOther.iTransferArgsCount),
	iNewDmaApi(aOther.iNewDmaApi),
	iExpectedArray(aOther.iExpectedArray), iPauseWhileQueuing(aOther.iPauseWhileQueuing)
	//const cast is required because their isn't a ctor taking const
	//array values
	//TODO iRequestCookies(const_cast<TUint*>(&aOther.iRequestCookies[0]), aOther.iRequestCookies.Count())
	{
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
		// Since all transfers will use the same channel,
		// they all get the same result
		// Arguably, iChannelOpenResult doesn't
		// belong TResultSet
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
			r = iDmaSession.RequestCreateNew(iChannelSessionCookie, cookie);
			}
		else
			{
			r = iDmaSession.RequestCreate(iChannelSessionCookie, cookie);
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

//TODO support test setup for CMultiTransferTest
void CMultiTransferTest::PreTransferSetup()
	{
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

/*
//TODO will need to support buffer checking of the trasnfers
TBool CIsrRequeTest::Result()
	{
	return CSingleTransferTest::Result();
	}
*/

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

//TODO
//this check will not deal correctly transfers were subsequent
//requeues overlap
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
	//TODO could make use of Fixup() method
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
	//TODO check for overlap

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
Check that the destination of each TAddressParms does not overlap with
any previous source or destination or that if it does the whole transfer
matches.
This is so that successive transfers do not overwrite the destinations or
sources of preceeding ones.
Exactly matching transfers are allowed to test the case that a repeat
transfer is required - though it can't then be determined just from
looking at the buffers that the repeat was successful
*/
TBool TPreTransferIncrBytes::CheckBuffers(const RArray<const TAddressParms> aTransferParams) const
	{
	const TInt count = aTransferParams.Count();

	for(TInt i=1; i<count; i++)
		{
		const TAddressParms& current = aTransferParams[i];
		for(TInt j=0; j<i; j++)
			{
			const TAddressParms& previous = aTransferParams[j];
			const TBool ok = !previous.Overlaps(current.DestRange()) || current == previous;
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
	const TInt bytesTransfered = (
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
		TEST_FAULT;
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

		if(testCase.iConcurrentTest)
			RDebug::Printf("== Begin concurrent test run ==");

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
				dmaTest->Announce();
				if(testCase.iConcurrentTest)
					{
					//Add test to array to be run concurrently
					TInt r = concurrentTests.Append(dmaTest);
					TEST_ASSERT(r == KErrNone);
					}
				else
					{
					//Run test in this thread
					(*dmaTest)();
					//TTestThread(
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

		const TInt count = concurrentTests.Count();
		if(count>0)
			{
			MultipleTestRun(concurrentTests);
			for(TInt i=0; i<count; i++)
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
	test.Printf(_L("  /V  or /VERBOSE    = Control test output\n"));
	test.Printf(_L("  /S  or /SELFTEST   = Run DMA self test\n"));
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
	testRunner.AddTestCases(TestArray);

	test.Next(_L("call TTestRunner::RunTests()\n"));
	testRunner.RunTests();

	test.End();
	}

TInt E32Main()
	{
	__UHEAP_MARK;
	//__KHEAP_MARK;
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
		//TODO how can we distinguish this case from a platform where
		//dma is supposed to be supported but the dma test ldd is
		//missing?
		test.Printf(_L("DMA not supported - test skipped\n"));
		return 0;
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

	ApiTests();

	RunDMATests();

	__KHEAP_MARKEND;

	r = User::FreeLogicalDevice(KTestDmaLddName);
	test_KErrNone(r);
	test.End();
	test.Close();

	delete cleanup;

	//__KHEAP_MARKEND;
	__UHEAP_MARKEND;
	return 0;
	}
