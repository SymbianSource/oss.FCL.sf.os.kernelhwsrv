/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* This file contains unit tests for the test framework itself.
* They should be run if changes have been made to
* to the user side test framework code ie. anything in the dmav2
* directory other than the d_* driver code, or test_cases.cpp
*
*/

#include "d_dma2.h"
#include "u32std.h"
#include "t_dma2.h"
#include "cap_reqs.h"

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32debug.h>
#include <e32svr.h>

static RTest test(_L("t_dma2 test framework tests"));

void RDmaSession::SelfTest()
	{
	test.Start(_L("Simple transfer test"));
	test.Next(_L("Open session"));
	RDmaSession session;
	TInt r = session.Open();
	test_KErrNone(r);

	test.Next(_L("Get test info"));
	TDmaV2TestInfo testInfo;
	r = session.GetTestInfo(testInfo);
	test_KErrNone(r);

	if(gVerboseOutput)
	{
	Print(testInfo);
	}

	test.Next(_L("Channel open"));
	TUint channelCookie=0;
	r = session.ChannelOpen(16, channelCookie);
	test.Printf(_L("cookie recived = 0x%08x\n"), channelCookie);
	test_KErrNone(r);

	test.Next(_L("Get Channel caps"));
	SDmacCaps channelCaps;
	r = session.ChannelCaps(channelCookie, channelCaps);
	test_KErrNone(r);
	if(gVerboseOutput)
	{
	PRINT(channelCaps.iChannelPriorities);
	PRINT(channelCaps.iChannelPauseAndResume);
	PRINT(channelCaps.iAddrAlignedToElementSize);
	PRINT(channelCaps.i1DIndexAddressing);
	PRINT(channelCaps.i2DIndexAddressing);
	PRINT(channelCaps.iSynchronizationTypes);
	PRINT(channelCaps.iBurstTransactions);
	PRINT(channelCaps.iDescriptorInterrupt);
	PRINT(channelCaps.iFrameInterrupt);
	PRINT(channelCaps.iLinkedListPausedInterrupt);
	PRINT(channelCaps.iEndiannessConversion);
	PRINT(channelCaps.iGraphicsOps);
	PRINT(channelCaps.iRepeatingTransfers);
	PRINT(channelCaps.iChannelLinking);
	PRINT(channelCaps.iHwDescriptors);
	PRINT(channelCaps.iSrcDstAsymmetry);
	PRINT(channelCaps.iAsymHwDescriptors);
	PRINT(channelCaps.iBalancedAsymSegments);
	PRINT(channelCaps.iAsymCompletionInterrupt);
	PRINT(channelCaps.iAsymDescriptorInterrupt);
	PRINT(channelCaps.iAsymFrameInterrupt);
	PRINT(channelCaps.iReserved[0]);
	PRINT(channelCaps.iReserved[1]);
	PRINT(channelCaps.iReserved[2]);
	PRINT(channelCaps.iReserved[3]);
	PRINT(channelCaps.iReserved[4]);	
	}

	test.Next(_L("Get extended Channel caps (TDmacTestCaps)"));
	TDmacTestCaps extChannelCaps;
	r = session.ChannelCaps(channelCookie, extChannelCaps);
	test_KErrNone(r);
	test.Printf(_L("PIL version = %d\n"), extChannelCaps.iPILVersion);

	const TBool newPil = (extChannelCaps.iPILVersion > 1);

	test.Next(_L("Create Dma request - max fragment size 32K"));
	TUint reqCookie=0;
	r = session.RequestCreate(channelCookie, reqCookie, 32 * KKilo);
	test.Printf(_L("cookie recived = 0x%08x\n"), reqCookie);
	test_KErrNone(r);

	if(newPil)
		{
		test.Next(_L("Create Dma request (with new-style callback)"));
		TUint reqCookieNewStyle=0;
		r = session.RequestCreateNew(channelCookie, reqCookieNewStyle);
		test.Printf(_L("cookie recived = 0x%08x\n"), reqCookieNewStyle );
		test_KErrNone(r);

		test.Next(_L("Fragment for ISR callback"));
		const TInt size = 128 * KKilo;
		TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);
		r = session.FragmentRequest(reqCookieNewStyle, transferArgs);
		test_KErrNone(r);

		TIsrRequeArgs reque;
		test.Next(_L("Queue ISR callback - with default re-queue"));
		r = session.QueueRequestWithRequeue(reqCookieNewStyle, &reque, 1);
		test_KErrNone(r);

		test.Next(_L("Destroy new-style Dma request"));
		r = session.RequestDestroy(reqCookieNewStyle);
		test_KErrNone(r);

		test.Next(_L("Attempt to destroy request again "));
		r = session.RequestDestroy(reqCookieNewStyle);
		test_Equal(KErrNotFound, r);
		}

	test.Next(_L("Open chunk handle"));
	RChunk chunk;
	r = session.OpenSharedChunk(chunk);
	test_KErrNone(r);
	if(gVerboseOutput)
	{
	test.Printf(_L("chunk base = 0x%08x\n"), chunk.Base());
	test.Printf(_L("chunk size = %d\n"), chunk.Size());
	}
	test(chunk.IsWritable());
	test(chunk.IsReadable());

	test.Next(_L("Fragment(old style)"));
	const TInt size = 128 * KKilo;
	TInt i;
	for(i = 0; i<10; i++)
		{
		TUint64 time = 0;
		TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr);
		r = session.FragmentRequestOld(reqCookie, transferArgs, &time);
		test_KErrNone(r);
		if(gVerboseOutput)
			{
			test.Printf(_L("%lu us\n"), time);
			}
	}

	test.Next(_L("Queue"));
	TRequestStatus status;

	for(i = 0; i<10; i++)
		{
		TUint64 time = 0;
		r = session.QueueRequest(reqCookie, status, 0, &time);
		User::WaitForRequest(status);
		test_KErrNone(r);
		if(gVerboseOutput)
			{
			test.Printf(_L("%lu us\n"), time);
			}
		}

	if(newPil)
		{
		test.Next(_L("Fragment(new style)"));
		TDmaTransferArgs transferArgs;
		transferArgs.iSrcConfig.iAddr = 0;
		transferArgs.iDstConfig.iAddr = size;
		transferArgs.iSrcConfig.iFlags = KDmaMemAddr;
		transferArgs.iDstConfig.iFlags = KDmaMemAddr;
		transferArgs.iTransferCount = size;

		for(i = 0; i<10; i++)
			{
			TUint64 time = 0;
			r = session.FragmentRequest(reqCookie, transferArgs, &time);
			test_KErrNone(r);
			if(gVerboseOutput)
				{
				test.Printf(_L("%lu us\n"), time);
				}
			}
		}

	test.Next(_L("Queue"));
	TCallbackRecord record;
	r = session.QueueRequest(reqCookie, &record);
	test_KErrNone(r);

	test.Next(_L("check TCallbackRecord record"));
	if(gVerboseOutput)
	{
	record.Print();
	}
	const TCallbackRecord expected(TCallbackRecord::EThread, 1);
	if(!(record == expected))
		{
		test.Printf(_L("TCallbackRecords did not match"));
		if(gVerboseOutput)
			{
			test.Printf(_L("expected:"));
			expected.Print();
			}
		TEST_FAULT;
		}

	test.Next(_L("Destroy Dma request"));
	r = session.RequestDestroy(reqCookie);
	test_KErrNone(r);

	test.Next(_L("Close chunk handle"));
	chunk.Close();

	test.Next(_L("Channel close"));
	r = session.ChannelClose(channelCookie);
	test_KErrNone(r);

	test.Next(_L("Channel close (same again)"));
	r = session.ChannelClose(channelCookie);
	test_Equal(KErrNotFound, r);

	test.Next(_L("Close session"));
	RTest::CloseHandleAndWaitForDestruction(session);

	test.End();

	}

const SDmacCaps KTestCapSet =
	{6,										// TInt iChannelPriorities;
	 EFalse,								// TBool iChannelPauseAndResume;
	 ETrue,									// TBool iAddrAlignedToElementSize;
	 EFalse,								// TBool i1DIndexAddressing;
	 EFalse,								// TBool i2DIndexAddressing;
	 KDmaSyncSizeElement | KDmaSyncSizeFrame |
	 KDmaSyncSizeBlock,					   // TUint iSynchronizationTypes;
	 KDmaBurstSize4 | KDmaBurstSize8,	   // TUint iBurstTransactions;
	 EFalse,							   // TBool iDescriptorInterrupt;
	 EFalse,							   // TBool iFrameInterrupt;
	 EFalse,							   // TBool iLinkedListPausedInterrupt;
	 EFalse,							   // TBool iEndiannessConversion;
	 0,									   // TUint iGraphicsOps;
	 ETrue,								   // TBool iRepeatingTransfers;
	 EFalse,							   // TBool iChannelLinking;
	 ETrue,								   // TBool iHwDescriptors;
	 EFalse,							   // TBool iSrcDstAsymmetry;
	 EFalse,							   // TBool iAsymHwDescriptors;
	 EFalse,							   // TBool iBalancedAsymSegments;
	 EFalse,							   // TBool iAsymCompletionInterrupt;
	 EFalse,							   // TBool iAsymDescriptorInterrupt;
	 EFalse,							   // TBool iAsymFrameInterrupt;
	 {0, 0, 0, 0, 0}					   // TUint32 iReserved[5];
	};

const TDmacTestCaps KDmacTestCapsV1(KTestCapSet, 1);
const TDmacTestCaps KDmacTestCapsV2(KTestCapSet, 2);

void TDmaCapability::SelfTest()
	{
	test.Start(_L("Unit test_Value of TDmaCapability::CompareToDmaCaps\n"));

	{
	test.Next(_L("ENone\n"));
	TResult t = none.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == ERun);
	}

	{
	test.Next(_L("EChannelPauseAndResume - wanted\n"));
	TResult t = pauseRequired.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == EFail);
	}
	{
	test.Next(_L("EChannelPauseAndResume - wanted - Allow skip\n"));
	TResult t = pauseRequired_skip.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == ESkip);
	}
	{
	test.Next(_L("EChannelPauseAndResume - not wanted\n"));
	TResult t = pauseNotWanted.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == ERun);
	}

	{
	test.Next(_L("EHwDescriptors - not wanted\n"));
	TResult t = hwDesNotWanted.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == EFail);
	}

	{
	test.Next(_L("EHwDescriptors - not wanted - Allow skip\n"));
	TResult t = hwDesNotWanted_skip.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == ESkip);
	}

	{
	test.Next(_L("EHwDescriptors - wanted\n"));
	TResult t = hwDesWanted.CompareToDmaCaps(KTestCapSet);
	test_Value(t, t == ERun);
	}


//TODO use this macro for the above tests

// Note: The construction of the test description message
// is horribly confusing. The _L macro will make the
// *first* string token wide, but not the next two.
// Therefore these must be made wide or compilier
// will complain about concatination of narrow and wide string
// literals
#define CAP_TEST(CAP, CAPSET, EXPCT)\
	{\
	test.Next(_L(#CAP L" against " L ## #CAPSET));\
	TResult t = (CAP).CompareToDmaCaps(CAPSET);\
	test_Equal(EXPCT, t);\
	}


	CAP_TEST(capEqualV1, KDmacTestCapsV1, ERun);
	CAP_TEST(capEqualV2, KDmacTestCapsV2, ERun);
	CAP_TEST(capEqualV1, KDmacTestCapsV2, ESkip);
	CAP_TEST(capEqualV2, KDmacTestCapsV1, ESkip);
	CAP_TEST(capEqualV2Fatal, KDmacTestCapsV1, EFail);

	CAP_TEST(capAboveV1, KDmacTestCapsV2, ERun);
	CAP_TEST(capBelowV2, KDmacTestCapsV1, ERun);
	CAP_TEST(capAboveV1, KDmacTestCapsV1, ESkip);
	CAP_TEST(capBelowV2, KDmacTestCapsV2, ESkip);

	test.End();
	}

void TTestCase::SelfTest()
	{
	//TODO should use macros for these tests
	test.Start(_L("Unit test of TTestCase::TestCaseValid\n"));

	TTestCase testCase(NULL, EFalse, pauseRequired, hwDesNotWanted);
	test.Next(_L("pauseRequired, hwDesNotWanted\n"));
	TResult t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == EFail);

	test.Next(_L("pauseRequired_skip, hwDesNotWanted\n"));
	testCase.iChannelCaps[0] = pauseRequired_skip;
	t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == EFail);

	test.Next(_L("pauseRequired_skip, hwDesNotWanted_skip\n"));
	testCase.iChannelCaps[1] = hwDesNotWanted_skip;
	t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == ESkip);

	test.Next(_L("pauseNotWanted, hwDesNotWanted_skip\n"));
	testCase.iChannelCaps[0] = pauseNotWanted;
	t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == ESkip);

	test.Next(_L("pauseNotWanted, hwDesWanted\n"));
	testCase.iChannelCaps[1] = hwDesWanted;
	t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == ERun);

	test.Next(_L("pauseNotWanted\n"));
	testCase.iChannelCaps[1] = none;
	t = testCase.TestCaseValid(KTestCapSet);
	test_Value(t, t == ERun);

	test.Next(_L("pauseNotWanted + V1 PIL required\n"));
	testCase.iChannelCaps[1] = capAboveV1;
	test.Next(_L("Against KDmacTestCapsV1"));
	t = testCase.TestCaseValid(KDmacTestCapsV1);
	test_Equal(ESkip, t);
	test.Next(_L("Against KDmacTestCapsV2"));
	t = testCase.TestCaseValid(KDmacTestCapsV2);
	test_Equal(ERun, t);

	test.Next(_L("pauseNotWanted + >V1 PIL required\n"));
	testCase.iChannelCaps[1] = capBelowV2;
	test.Next(_L("Against KDmacTestCapsV1"));
	t = testCase.TestCaseValid(KDmacTestCapsV1);
	test_Equal(ERun, t);
	test.Next(_L("Against KDmacTestCapsV2"));
	t = testCase.TestCaseValid(KDmacTestCapsV2);
	test_Equal(ESkip, t);

	test.End();
	test.Close();
	}


void TTransferIter::SelfTest()
	{
	test.Start(_L("No skip"));

	const TUint8 src[9] = {
			1 ,2, 3,
			4, 5, 6,
			7, 8, 9
	};

	const TUint32 addr = (TUint32)src;
	const TUint elementSize = 1;
	const TUint elementSkip = 0;
	const TUint elementsPerFrame = 3;
	const TUint frameSkip = 0;
	const TUint framesPerTransfer = 3;
	TDmaTransferConfig cfg(addr, elementSize, elementsPerFrame, framesPerTransfer,
			elementSkip, frameSkip, KDmaMemAddr
			);

	TTransferIter iter(cfg, 0);
	TTransferIter end;
	TInt i;
	for(i = 0; i<9; i++, ++iter)
		{
		test_Equal(src[i],*iter);
		};


	test.Next(_L("90 degree rotation"));
	// Now imagine that we wanted to perform a rotation
	// as we write, so that we wrote out the following

	const TUint8 expected[9] = {
		7, 4, 1,
		8, 5, 2,
		9, 6, 3
	};

	TUint8 dst[9] = {0};
	TDmaTransferConfig dst_cfg(cfg);
	dst_cfg.iAddr = (TUint32)&dst[2];
	dst_cfg.iElementSkip = 2;
	dst_cfg.iFrameSkip = -8;

	TTransferIter dst_iter(dst_cfg, 0);
	for(i=0; dst_iter != end; i++, ++dst_iter)
		{
		TEST_ASSERT(i<9);
		*dst_iter=src[i];
		};

	for(i=0; i<9; i++)
		{
		test_Equal(expected[i],dst[i]);
		}
	}

void TCallbackRecord::SelfTest()
	{
	test.Start(_L("SeltTest of TCallbackRecord"));

	test.Next(_L("create default TCallbackRecord record, record2"));
	TCallbackRecord record;
	const TCallbackRecord record2;
	if(gVerboseOutput)
	{
	test.Next(_L("Print record"));
	record.Print();
	}

	test.Next(_L("test (record == record2)"));
	if(!(record == record2))
		{
		if(gVerboseOutput)
			{
			record2.Print();
			}
		TEST_FAULT;
		}

	//A series of callback masks
	//Note these combinations do not necessarily represent
	//possible callback combinations
	TUint callbacks[]  =
		{
		EDmaCallbackDescriptorCompletion,
		EDmaCallbackDescriptorCompletion,
		EDmaCallbackDescriptorCompletion,
		EDmaCallbackDescriptorCompletion,
		EDmaCallbackFrameCompletion_Src,
		EDmaCallbackFrameCompletion_Dst,
		EDmaCallbackDescriptorCompletion_Src | EDmaCallbackDescriptorCompletion_Dst,
		EDmaCallbackDescriptorCompletion_Src | EDmaCallbackFrameCompletion_Src | EDmaCallbackLinkedListPaused_Dst,
		EDmaCallbackRequestCompletion | EDmaCallbackRequestCompletion_Src,
		EDmaCallbackDescriptorCompletion_Dst
		};
	test.Next(_L("Feed a series of callback masks in to record"));
	const TInt length = ARRAY_LENGTH(callbacks);
	for(TInt i = 0; i < length; i++)
		{
		record.ProcessCallback(callbacks[i], EDmaResultOK);
		}
	
	if(gVerboseOutput)
	{
	test.Next(_L("Print record"));
	record.Print();
	}

	test.Next(_L("test GetCount"));
	test_Equal(1, record.GetCount(EDmaCallbackRequestCompletion));
	test_Equal(1, record.GetCount(EDmaCallbackRequestCompletion_Src));
	test_Equal(0, record.GetCount(EDmaCallbackRequestCompletion_Dst));
	test_Equal(4, record.GetCount(EDmaCallbackDescriptorCompletion));
	test_Equal(2, record.GetCount(EDmaCallbackDescriptorCompletion_Src));
	test_Equal(2, record.GetCount(EDmaCallbackDescriptorCompletion_Dst));
	test_Equal(0, record.GetCount(EDmaCallbackFrameCompletion));
	test_Equal(2, record.GetCount(EDmaCallbackFrameCompletion_Src));
	test_Equal(1, record.GetCount(EDmaCallbackFrameCompletion_Dst));
	test_Equal(0, record.GetCount(EDmaCallbackLinkedListPaused));
	test_Equal(0, record.GetCount(EDmaCallbackLinkedListPaused_Src));
	test_Equal(1, record.GetCount(EDmaCallbackLinkedListPaused_Dst));

	test.Next(_L("test expected == record"));
	const TCallbackRecord expected(TCallbackRecord::EThread, 1, 1, 0, 4, 2, 2, 0, 2, 1, 0, 0, 1);
	if(!(expected == record))
		{
		if(gVerboseOutput)
			{
			expected.Print();
			}
		TEST_FAULT;
		}

	test.Next(_L("modify record: test expected != record"));
	record.SetCount(EDmaCallbackFrameCompletion, 10);
	if(expected == record)
		{
		if(gVerboseOutput)
			{
			expected.Print();
			}
		TEST_FAULT;
		}

	test.Next(_L("test Reset()"));
	record.Reset();
	test(record == record2);

	test.End();
	}

void CDmaBenchmark::SelfTest()
	{
	test.Start(_L("SelfTest of CDmaBenchmark"));
	test.Next(_L("MeanResult()"));

	TUint64 results[] = {8, 12, 1, 19, 3, 17, 10};
	const TInt count = ARRAY_LENGTH(results);

	CDmaBmFragmentation fragTest(_L("SelfTest"), count, TDmaTransferArgs(), 0);

	for(TInt i = 0; i < count; i++)
		{
		fragTest.iResultArray.Append(results[i]);
		}
	test_Equal(10, fragTest.MeanResult());

	test.End();
	}

void TAddrRange::SelfTest()
	{
	test.Start(_L("SelfTest of TAddrRange"));
	TAddrRange a(0, 8);
	TAddrRange b(8, 8);

	test_Equal(7, a.End());
	test_Equal(15, b.End());

	test(!a.Overlaps(b));
	test(!b.Overlaps(a));
	test(a.Overlaps(a));
	test(b.Overlaps(b));

	TAddrRange c(7, 2);
	test_Equal(8, c.End());

	test(a.Overlaps(c));
	test(c.Overlaps(a));
	test(b.Overlaps(c));
	test(c.Overlaps(b));

	TAddrRange d(0, 24);
	test(a.Overlaps(d));
	test(d.Overlaps(a));

	test(b.Overlaps(d));
	test(d.Overlaps(b));

	test(d.Contains(d));

	test(d.Contains(a));
	test(!a.Contains(d));

	test(d.Contains(b));
	test(!b.Contains(d));

	test(!a.Contains(b));
	test(!b.Contains(a));
	test.End();
	}

void TAddressParms::SelfTest()
	{
	test.Start(_L("SelfTest of TAddressParms"));
	const TAddressParms pA(0, 32, 8);
	test(pA == pA);
	test(pA.Overlaps(pA));

	const TAddrRange rA(4, 8);
	const TAddrRange rB(16, 8);
	const TAddrRange rC(28, 8);
	const TAddrRange rD(4, 32);

	test(pA.Overlaps(rA));
	test(!pA.Overlaps(rB));
	test(pA.Overlaps(rC));
	test(pA.Overlaps(rD));

	const TAddressParms pB(8, 16, 8);
	test(!(pA == pB));
	test(!(pB == pA));
	test(!pA.Overlaps(pB));
	test(!pB.Overlaps(pA));

	const TAddressParms pC(8, 28, 8);
	test(pC.Overlaps(pA));
	test(pC.Overlaps(pB));

	const TAddressParms pD(0, 128, 64);
	test(pD.Overlaps(pA));
	test(pD.Overlaps(pB));
	test(pD.Overlaps(pC));
	test.End();
	}

void SelfTests()
	{
	test.Next(_L("Running framework unit tests"));
	RDmaSession::SelfTest();
	TDmaCapability::SelfTest();
	TTestCase::SelfTest();
	TTransferIter::SelfTest();
	TCallbackRecord::SelfTest();
	CDmaBmFragmentation::SelfTest();
	TAddrRange::SelfTest();
	TAddressParms::SelfTest();
	test.End();
	test.Close();
	}
