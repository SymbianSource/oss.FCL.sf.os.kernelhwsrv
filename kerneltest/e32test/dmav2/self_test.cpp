/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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

void RDmaSession::SelfTest(TBool aSimulatedDmac)
	{
	test.Start(_L("Simple transfer test"));

	RDmaSession session;
	TInt r = KErrUnknown;
	if (aSimulatedDmac)
		{
		test.Next(_L("Open session (simulated DMA)"));
		r = session.OpenSim();
		}
	else
		{
		test.Next(_L("Open session"));
		r = session.Open();
		}

	test_KErrNone(r);

	test.Next(_L("Get test info"));
	TDmaV2TestInfo testInfo;
	r = session.GetTestInfo(testInfo);
	test_KErrNone(r);

	if(gVerboseOutput)
	{
	Print(testInfo);
	}

	// Self test just needs 1 channel
	// The real test will test all available ones
	test.Next(_L("Select test channel"));
	TUint testChannel = 0;
	if(testInfo.iMaxSbChannels > 0)
		{
		testChannel = testInfo.iSbChannels[0];
		}
	else if(testInfo.iMaxDbChannels > 0)
		{
		testChannel = testInfo.iDbChannels[0];
		}
	else if(testInfo.iMaxSgChannels > 0)
		{
		testChannel = testInfo.iSgChannels[0];
		}
	else
		{
		test.Printf(_L("Driver exposes no channels to test"));
		test(EFalse);
		}

	test.Printf(_L("using PSL cookie %d (0x%08x)\n"), testChannel, testChannel);
	test.Next(_L("Open channel"));
	TUint channelCookie=0;
	r = session.ChannelOpen(testChannel, channelCookie);
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
	r = session.RequestCreateOld(channelCookie, reqCookie, 32 * KKilo);
	test.Printf(_L("cookie recived = 0x%08x\n"), reqCookie);
	test_KErrNone(r);

	if(newPil)
		{
		test.Next(_L("Create Dma request (with new-style callback)"));
		TUint reqCookieNewStyle=0;
		r = session.RequestCreate(channelCookie, reqCookieNewStyle);
		test.Printf(_L("cookie recived = 0x%08x\n"), reqCookieNewStyle );
		test_KErrNone(r);

		if(!aSimulatedDmac)
			{
			test.Next(_L("Fragment for ISR callback"));
			const TInt size = 128 * KKilo;
			TDmaTransferArgs transferArgs(0, size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);
			r = session.FragmentRequest(reqCookieNewStyle, transferArgs);
			test_KErrNone(r);

			TIsrRequeArgs reque;
			test.Next(_L("Queue ISR callback - with default re-queue"));
			r = session.QueueRequestWithRequeue(reqCookieNewStyle, &reque, 1);
			test_KErrNone(r);
			}

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

	if(!aSimulatedDmac)
		{
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

	CAP_TEST(none, KTestCapSet, ERun);
	CAP_TEST(pauseRequired, KTestCapSet, EFail);
	CAP_TEST(pauseRequired_skip, KTestCapSet, ESkip);
	CAP_TEST(pauseNotWanted, KTestCapSet, ERun);
	CAP_TEST(hwDesNotWanted, KTestCapSet, EFail);	
	CAP_TEST(hwDesNotWanted_skip, KTestCapSet, ESkip);
	CAP_TEST(hwDesWanted, KTestCapSet, ERun);

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
	test.Start(_L("Unit test of TTestCase::TestCaseValid\n"));

// Create a TTestCase with paramaters CAP1 and CAP2
// call TTestCase::TestCaseValid against CAPSET
// Expected result is EXPCT
#define TEST_TEST_CASE(CAP1, CAP2, CAPSET, EXPCT)\
	{\
	test.Next(_L(#CAP1 L", " L ## #CAP2 L" -- Against: " L ## #CAPSET L", Expect: " L ## #EXPCT));\
	TTestCase testCase(NULL, EFalse, CAP1, CAP2);\
	testCase.iChannelCaps[0] = (CAP1);\
	TResult t = testCase.TestCaseValid(CAPSET);\
	test_Equal(EXPCT, t);\
	}

	TEST_TEST_CASE(pauseRequired, hwDesNotWanted, KTestCapSet, EFail);
	TEST_TEST_CASE(pauseRequired_skip, hwDesNotWanted, KTestCapSet, EFail);
	TEST_TEST_CASE(pauseRequired_skip, hwDesNotWanted_skip, KTestCapSet, ESkip);
	TEST_TEST_CASE(pauseNotWanted, hwDesNotWanted_skip, KTestCapSet, ESkip);
	TEST_TEST_CASE(pauseNotWanted, hwDesWanted, KTestCapSet, ERun);
    TEST_TEST_CASE(pauseNotWanted, none, KTestCapSet, ERun);

	TEST_TEST_CASE(pauseNotWanted, capAboveV1, KDmacTestCapsV1, ESkip);
	TEST_TEST_CASE(pauseNotWanted, capAboveV1, KDmacTestCapsV2, ERun);

	TEST_TEST_CASE(pauseNotWanted, capBelowV2, KDmacTestCapsV1, ERun);
	TEST_TEST_CASE(pauseNotWanted, capBelowV2, KDmacTestCapsV2, ESkip);

	// contradictory requirements
	TEST_TEST_CASE(capAboveV1, capBelowV2, KDmacTestCapsV2, ESkip);
	TEST_TEST_CASE(capBelowV2, capAboveV1, KDmacTestCapsV2, ESkip);

	TEST_TEST_CASE(capAboveV1, capBelowV2, KDmacTestCapsV1, ESkip);
	TEST_TEST_CASE(capBelowV2, capAboveV1, KDmacTestCapsV1, ESkip);

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
	test.Start(_L("SelfTest of TCallbackRecord"));

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

	// The mean of these numbers is 10
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

	test.Next(_L("Test IsFilled()"));
	TUint8 buffer[] = {0,0,0,0};
	TAddrRange range((TUint)buffer, 4);
	test(range.IsFilled(0));
	buffer[3] = 1;
	test(!range.IsFilled(0));
	buffer[2] = 1;
	buffer[1] = 1;
	buffer[0] = 1;
	test(range.IsFilled(1));

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

void TIsrRequeArgsSet::SelfTest()
	{
	test.Start(_L("Selftest of TIsrRequeArgsSet"));

	TUint size = 0x1000;
	TDmaTransferArgs tferArgs(0, 1*size, size, KDmaMemAddr, KDmaSyncAuto, KDmaRequestCallbackFromIsr);

	TIsrRequeArgs requeArgArray[] = {
		TIsrRequeArgs(),									// Repeat
		TIsrRequeArgs(KPhysAddrInvalidUser, 2*size, 0),		// Change destination
		TIsrRequeArgs(),									// Repeat
		TIsrRequeArgs(3*size, KPhysAddrInvalidUser, 0),		// Change source
		TIsrRequeArgs(),									// Repeat
	};
	TIsrRequeArgsSet argSet(requeArgArray, ARRAY_LENGTH(requeArgArray));

	test.Next(_L("Test that Substitute updates transfer args in order"));
	argSet.Substitute(tferArgs);

	TAddressParms expectedFinal(3*size, 2*size, size);
	if(!(expectedFinal == argSet.iRequeArgs[4]))
		{
		TBuf<0x100> out;

		out += _L("substitue: ");
		GetAddrParms(tferArgs).AppendString(out);
		test.Printf(out);

		out.Zero();
		out += _L("\nexpected final: ");
		expectedFinal.AppendString(out);
		test.Printf(out);

		out.Zero();
		out += _L("\nactual: ");
		argSet.iRequeArgs[4].AppendString(out);
		test.Printf(out);

		test(EFalse);
		}

	TIsrRequeArgs requeArgArray2[] = {
		TIsrRequeArgs(),									// Repeat
		TIsrRequeArgs(KPhysAddrInvalidUser, 2*size, 0),		// Change destination
		TIsrRequeArgs(KPhysAddrInvalidUser, 1*size, 0),		// Change destination back
	};
	argSet = TIsrRequeArgsSet(requeArgArray2, ARRAY_LENGTH(requeArgArray2));

	test.Next(_L("CheckRange(), negative"));

	test(!argSet.CheckRange(0, (2 * size) - 1, tferArgs));
	test(!argSet.CheckRange(0, (2 * size) + 1, tferArgs));
	test(!argSet.CheckRange(0, (2 * size), tferArgs));

	test(!argSet.CheckRange(1 ,(3 * size), tferArgs));
	test(!argSet.CheckRange(1 ,(3 * size) + 1, tferArgs));

	test(!argSet.CheckRange(1 * size , 2 * size, tferArgs));

	test.Next(_L("CheckRange(), positive"));
	test(argSet.CheckRange(0, 3 * size, tferArgs));
	test(argSet.CheckRange(0, 3 * size+1, tferArgs));
	test(argSet.CheckRange(0, 4 * size, tferArgs));


	test.End();
	}

void RArrayCopyTestL()
	{
	test.Start(_L("Selftest of RArray CopyL"));

	RArray<TInt> orig;
	TInt i;													// VC++
	for(i=0; i<10; i++)
		{
		orig.AppendL(i);
		}

	RArray<TInt> newArray;
	CopyL(orig, newArray);

	test_Equal(10, newArray.Count());

	for(i=0; i<10; i++)
		{
		test_Equal(orig[i], newArray[i])
		}

	orig.Close();
	newArray.Close();
	test.End();
	}

void RArrayInsertLTest()
	{
	test.Start(_L("Selftest of RArray InsertL"));

	RArray<TInt> array;
	TInt numbers[10] = {0,1,2,3,4,5,6,7,8,9};
	ArrayAppendL(array, &numbers[0], numbers + ARRAY_LENGTH(numbers));

	test_Equal(10, array.Count());
	for(TInt i=0; i<10; i++)
		{
		test_Equal(numbers[i], array[i])
		}

	array.Close();
	test.End();
	}

/**
Run check buffers on the supplied TAddressParms array
*/
TBool DoTferParmTestL(const TAddressParms* aParms, TInt aCount, TBool aAllowRepeat, TBool aPositive)
	{
	_LIT(KPositive, "positive");
	_LIT(KNegative, "negative");
	test.Printf(_L("CheckBuffers %S test: %d args, repeats allowed %d\n"),
			(aPositive ? &KPositive : &KNegative), aCount, aAllowRepeat);
	RArray<const TAddressParms> array;
	ArrayAppendL(array, aParms, aParms + aCount);
	TPreTransferIncrBytes preTran;
	TBool r = preTran.CheckBuffers(array, aAllowRepeat);
	array.Close();
	return r;
	}

void TPreTransferIncrBytes::SelfTest()
	{
	// Test that TPreTransferIncrBytes::CheckBuffers can identify
	// overlapping buffers
	test.Start(_L("Selftest of TPreTransferIncrBytes"));

// Macro generates test for 2 element array
#define TPARM_TEST2(EXPECT, ALLOW_REPEAT, EL0, EL1)\
		{\
		TAddressParms set[2] = {EL0, EL1}; \
		const TBool r = DoTferParmTestL(set, 2, ALLOW_REPEAT, EXPECT);\
		test_Equal(EXPECT, r);\
		}

// Generate positive 2 element test
#define TPARM_TEST2_POSITIVE(ALLOW_REPEAT, EL0, EL1) TPARM_TEST2(ETrue, ALLOW_REPEAT, EL0, EL1)
// Generate negative 2 element test
#define TPARM_TEST2_NEG(ALLOW_REPEAT, EL0, EL1) TPARM_TEST2(EFalse, ALLOW_REPEAT, EL0, EL1)

// Macro generates test for 3 element array
#define TPARM_TEST3(EXPECT, ALLOW_REPEAT, EL0, EL1, EL2)\
		{\
		TAddressParms set[3] = {EL0, EL1, EL2}; \
		const TBool r = DoTferParmTestL(set, 3, ALLOW_REPEAT, EXPECT);\
		test_Equal(EXPECT, r);\
		}

// Generate positive 3 element test
#define TPARM_TEST3_POSITIVE(ALLOW_REPEAT, EL0, EL1, EL2) TPARM_TEST3(ETrue, ALLOW_REPEAT, EL0, EL1, EL2)
// Generate negative 3 element test
#define TPARM_TEST3_NEG(ALLOW_REPEAT, EL0, EL1, EL2) TPARM_TEST3(EFalse, ALLOW_REPEAT, EL0, EL1, EL2)

	TPARM_TEST2_POSITIVE(EFalse, TAddressParms(0,16,16), TAddressParms(32, 48, 16));
	TPARM_TEST2_POSITIVE(ETrue, TAddressParms(0, 16, 16), TAddressParms(0, 16, 16)); // both overlap (repeat allowed)

	TPARM_TEST2_NEG(EFalse, TAddressParms(0,16,16), TAddressParms(24, 40, 16)); // second source depends on first destination
	TPARM_TEST2_NEG(EFalse, TAddressParms(0,16,16), TAddressParms(16, 0, 16)); // second dest overwrites first source
	TPARM_TEST2_NEG(EFalse, TAddressParms(0, 16, 16), TAddressParms(0, 16, 16)); // both overlap (repeat not allowed)
	TPARM_TEST2_NEG(ETrue, TAddressParms(0, 16, 16), TAddressParms(0, 20, 16)); // exact repeat allowed, but overlap is only partial
	TPARM_TEST2_NEG(ETrue, TAddressParms(0, 16, 16), TAddressParms(32, 16, 16)); // exact repeat allowed, but 2nd overwrites first dest


	TPARM_TEST3_POSITIVE(EFalse, TAddressParms(0,16,16), TAddressParms(32, 48, 16), TAddressParms(64, 128, 64)); // no overlaps
	TPARM_TEST3_POSITIVE(ETrue, TAddressParms(0, 16, 16), TAddressParms(0, 16, 16), TAddressParms(0, 16, 16)); // all overlap (repeat allowed)
	TPARM_TEST3_POSITIVE(EFalse, TAddressParms(0,16,16), TAddressParms(0, 32, 16), TAddressParms(0, 48, 16)); // no overlaps (1 src to 3 dsts)

	TPARM_TEST3_NEG(EFalse, TAddressParms(0,16,16), TAddressParms(128, 256, 128), TAddressParms(24, 40, 16)); // 3rd source depends on first destination
	TPARM_TEST3_NEG(EFalse, TAddressParms(0,16,16), TAddressParms(128, 256, 128), TAddressParms(16, 0, 16)); // 3rd dest overwrites first source
	TPARM_TEST3_NEG(EFalse, TAddressParms(0, 16, 16), TAddressParms(0, 16, 16), TAddressParms(0, 16, 16)); // all overlap (repeat not allowed)
	test.Next(_L("CheckBuffers(RArray<TAddressParms>)"));
	}

void SelfTests()
	{
	test.Next(_L("Running framework unit tests"));
#ifndef __WINS__
	// Cannot connect real driver on Emulator - only
	// simulator
	RDmaSession::SelfTest(EFalse);
#endif
	RDmaSession::SelfTest(ETrue);
	TDmaCapability::SelfTest();
	TTestCase::SelfTest();
	TTransferIter::SelfTest();
	TCallbackRecord::SelfTest();
	CDmaBmFragmentation::SelfTest();
	TAddrRange::SelfTest();
	TAddressParms::SelfTest();
	TIsrRequeArgsSet::SelfTest();
	RArrayCopyTestL();
	RArrayInsertLTest();
	TPreTransferIncrBytes::SelfTest();
	test.End();
	test.Close();
	}
