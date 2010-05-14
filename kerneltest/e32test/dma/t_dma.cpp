// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dma\t_dma.cpp
// Overview:
// Test the DMA channel functionality.
// API Information:
// RBusLogicalChannel, DLogicalChannelBase, DLogicalDevice
// Details:	
// - Load the DMA LDD, create a critical section, an active scheduler and
// a CPeriodic object.
// - Test one shot single buffer transfers: test simple transfer, request 
// reconfiguration and cancelling. Verify results are as expected.
// - Test one shot double buffer transfers: test simple transfer, request 
// reconfiguration and cancelling. Verify results are as expected.
// - Test streaming single buffer transfers: test simple transfer and
// cancelling. Test that framework behaves correctly if one or more DMA
// interrupts are missed. Verify results are as expected.
// - Test streaming double buffer transfers: test simple transfer and
// cancelling. Test that framework behaves correctly if one or more DMA
// interrupts are missed. Verify results are as expected.
// - Test streaming scatter/gather transfers: test simple transfer and
// cancelling. Test that framework behaves correctly if one or more DMA
// interrupts are missed. Verify results are as expected.
// Platforms/Drives/Compatibility:
// Hardware (Automatic).
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "d_dma.h"
#include <e32debug.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include "u32std.h"

#ifdef __DMASIM__
RTest test(_L("T_DMASIM"));
#else
RTest test(_L("T_DMA"));
#endif

//////////////////////////////////////////////////////////////////////////////
// Mini-framework for running tests either in a single thread or in
// several concurrent ones.

RTestDma::TInfo Info;
TBool JitEnabled;
RCriticalSection TheCriticalSection;						// protect following variables
TInt ThreadCount;											// decremented when tester thread dies
CPeriodic* Bipper;											// display dots during tests to detect lock-ups

// Test macro used inside tester threads
_LIT(KTestFailure, "XTEST");
static void TestPanic(TInt aLine, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	RDebug::Printf("Line %d test failed a1=%08x (%d) a2=%08x (%d) a3=%08x (%d)", aLine, a1, a1, a2, a2, a3, a3);
	RThread().Panic(KTestFailure, aLine);
	}
#define XTEST(e)				if (!(e)) TestPanic(__LINE__, 0, 0, 0)
#define XTEST1(e,a1)			if (!(e)) TestPanic(__LINE__, (a1), 0, 0)
#define XTEST2(e,a1,a2)			if (!(e)) TestPanic(__LINE__, (a1), (a2), 0)
#define XTEST3(e,a1,a2,a3)		if (!(e)) TestPanic(__LINE__, (a1), (a2), (a3))


/**
Specifies a DMA test
@note Have not inherited from CBase so that implicit copy ctors are used
*/
class CTest
	{
public:
	typedef void (*TTestFunction)(RTestDma aChannel, TInt aMaxFragment, TInt aFragmentSize);

	CTest(TTestFunction aFn, TInt aMaxIter)
		:iTestFn(aFn), iChannelId(0), iMaxIter(aMaxIter)
		{}

	virtual ~CTest()
		{}

	TInt RunTest();

	virtual TBool OpenChannel(TInt aDesCount, TInt aMaxFragmentSize=0);

	virtual void AnnounceTest(TDes& aDes)
		{aDes.AppendFormat(_L("Channel Id %d, iMaxIter %d"), iChannelId, iMaxIter);}
	virtual void ReportState(TDes& aDes)
		{aDes.AppendFormat(_L("Channel Id %d, iCurIter %d"), iChannelId, iCurIter);}


	void SetChannelId(TUint32 aChannelId)
		{iChannelId = aChannelId;}

	TInt MaxIter() const {return iMaxIter;}
	TInt CurIter() const {return iCurIter;}

	/**
	@return A copy of this test
	*/	
	virtual	CTest* Clone() const =0;

protected:
	TInt virtual DoRunTest() =0;

	const TTestFunction iTestFn;
	TUint32 iChannelId;
	const TInt iMaxIter;
	TInt iCurIter;
	RTestDma iChannel;
	};	

/**
Specifies a DMA test where the maximum fragmentation is
explicitly limited. This tests that requests are split
in to the number of fragments expected.

This test also requires that physically contiguous buffers
are used. For this reason the product of iMaxFragment and
iMaxFragmentSize should be kept small
*/
class CFragmentationTest : public CTest
	{
public:
	CFragmentationTest(TTestFunction aFn, TInt aMaxIter, TInt aMaxFragment, TInt aMaxFragmentSize)
		: CTest(aFn, aMaxIter), iMaxFragment(aMaxFragment), iMaxFragmentSize(aMaxFragmentSize), iCurFragment(0)
	{}

	TInt virtual DoRunTest();

	virtual void AnnounceTest(TDes& aDes)
		{
		aDes.AppendFormat(_L("CFragmentationTest: Frag count = [1..%d], Max Frag Size = 0x%08x bytes: "), iMaxFragment, iMaxFragmentSize);
		CTest::AnnounceTest(aDes);
		}

	virtual void ReportState(TDes& aDes)
		{
		aDes.AppendFormat(_L("CFragmentationTest: Current Fragment %d: "), iCurFragment);
		CTest::ReportState(aDes);
		}

	CTest* Clone() const
		{return new CFragmentationTest(*this);}

private:
	const TInt iMaxFragment;
	TInt iMaxFragmentSize;
	TInt iCurFragment;
	};

/**
Specifies a DMA test where the maximum fragment size is
not limited - and we do not care how many fragments are
used

- This checks that transfers work correctly with the DMAC's
default fragment size
*/
class CDefaultFragTest : public CTest
	{
public:
	CDefaultFragTest(TTestFunction aFn, TInt aMaxIter, TUint aTotalTransferSize)
		: CTest(aFn, aMaxIter), iTotalTransferSize(aTotalTransferSize)
		{}

	TInt virtual DoRunTest();

	virtual void AnnounceTest(TDes& aDes)
		{
		aDes.AppendFormat(_L("CDefaultFragTest: Transfer = 0x%08x bytes: "), iTotalTransferSize);
		CTest::AnnounceTest(aDes);
		}

	CTest* Clone() const
		{return new CDefaultFragTest(*this);}

	const TInt iTotalTransferSize;
	};

/**
Test that it is possible to close a channel from a callback
*/
class CCloseInCb : public CTest
	{
public:
	CCloseInCb()
		: CTest(NULL, 1), iTransferSize(4 * KKilo)
		{}

	TInt virtual DoRunTest();

	virtual void AnnounceTest(TDes& aDes)
		{
		aDes.AppendFormat(_L("CCloseInCb"));
		CTest::AnnounceTest(aDes);
		}

	CTest* Clone() const
		{return new CCloseInCb(*this);}
private:
	const TInt iTransferSize;

	};

/**
Perform multiple transfers with different fragment counts and with smaller
and smaller fragment size

This checks that the PSL's ISR(s) are properly written, and do not miss interrupts
or notify the PIL spuriously.
*/
class CFragSizeRange : public CTest
	{
public:
	CFragSizeRange(TInt aMaxIter, TInt aFragCount, TInt aInitialFragmentSize, TInt aInnerIteraions)
		: CTest(NULL, aMaxIter), iMaxFragCount(aFragCount), iFragCount(1), iInitialFragmentSize(aInitialFragmentSize),
		iInnerIterations(aInnerIteraions)
		{}

	TInt virtual DoRunTest();

	virtual void AnnounceTest(TDes& aDes)
		{
		aDes.AppendFormat(_L("CFragSizeRange: Fragments %d, intital frag size %d, inner iters %d "), iFragCount, iInitialFragmentSize, iInnerIterations);
		CTest::AnnounceTest(aDes);
		}

	CTest* Clone() const
		{return new CFragSizeRange(*this);}

private:
	/**
	Run the transfer
	*/
	TInt Transfer(TInt aFragSize);


	TInt iMaxFragCount;
	TInt iFragCount;
	const TInt iInitialFragmentSize;
	const TInt iInnerIterations;

	RTimer iTimer;
	};

//
// Active object used to create a tester thread, log on to it and
// interpret its exit status.
//
class CTesterThread : public CActive
	{
public:
	CTesterThread(TInt aIdx, CTest* aTest);
	~CTesterThread()
		{
		delete iTest;
		}
private:
	static TInt ThreadFunction(TAny* aSelf);
	TInt StartThread();
	// from CActive
	virtual void DoCancel();
	virtual void RunL();
private:
	RThread iThread;
	CTest* iTest;
	};


/**
Run the test for iMaxIter iterations
*/
TInt CTest::RunTest()
	{
	TInt r = KErrNone;
	for (iCurIter=0; iCurIter<iMaxIter; ++iCurIter)
		{
		r =  DoRunTest();
		if(KErrNone != r)
			break;
		}
	return r;
	}

/**
Open iChannel

@pre iChannel is not open
@return
   - KErrNotSupported Channel does not exist on DMAC
   - KErrNone Success
   - KErrInUse
*/
TInt CTest::OpenChannel(TInt aDesCount, TInt aMaxFragmentSize)
		{
		ASSERT(!iChannel.Handle());
		const TInt r = iChannel.Open(iChannelId, aDesCount, aMaxFragmentSize);
		if (r == KErrNotSupported)
			return r;
		XTEST1(KErrNone == r || KErrInUse == r, r);
		
		if(KErrInUse == r)
			{
			// Channel is in use.
			RDebug::Printf("\nDMA Channel %d is in use",iChannelId);
			if(0 == iCurIter)
				{
				// Terminate thread by returning this error code KErrInUse.
				return r;
				}
			else
				{
#ifdef __WINS__
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif // __WINS__
				XTEST1(EFalse, iCurIter);
#ifdef __WINS__
#pragma warning( default : 4127 ) // warning C4127: conditional expression is constant
#endif // __WINS__
				}
			}
		return r;
		}



// Spawn thread. Will auto-delete when thread exits.
CTesterThread::CTesterThread(TInt aIdx, CTest* aTest)
	: CActive(EPriorityStandard), iTest(aTest)
	{
	CActiveScheduler::Add(this);
	TBuf<16> name;
	name = _L("TESTER-");
	name.AppendNum(aIdx);
	test(iThread.Create(name, ThreadFunction, 0x2000, NULL, this) == KErrNone);
	iThread.SetPriority(EPriorityLess);
	iThread.Logon(iStatus);
	SetActive();
	iThread.Resume();
	}


TInt CTesterThread::ThreadFunction(TAny* aSelf)
	{
	CTesterThread* self = (CTesterThread*)aSelf;
	return self->StartThread();
	}

TInt CTesterThread::StartThread()
	{
	return iTest->RunTest();
	}



TInt CFragmentationTest::DoRunTest()
	{
	// In case iMaxFragmentSize was larger than suppported (we need to know what fragment
	// size will actually be used)
	iMaxFragmentSize = Min(iMaxFragmentSize, Info.iMaxTransferSize);

	// Open channel with enough descriptors for 3 open DMA
	// requests (see TestStreaming).
	TInt r = OpenChannel(3* iMaxFragment, iMaxFragmentSize);
	if(r != KErrNone)
		return r;

	//we are controlling fragment size, so we know how
	//many to expect
	for (iCurFragment=1; iCurFragment<=iMaxFragment; iCurFragment*=2)
		{
		const TInt size = iCurFragment * ( iMaxFragmentSize & ~Info.iMemAlignMask);
		iTestFn(iChannel, iCurFragment, size);
		}
	iChannel.Close();
	return KErrNone;
	}

TInt CDefaultFragTest::DoRunTest()
	{
	// +1 so we don't underestimate maxFragount for inexact division
	const TUint maxFragCount = (iTotalTransferSize / Info.iMaxTransferSize) +1;

	// Open channel with enough descriptors for 3 open DMA
	// requests (see TestStreaming).
	const TUint descriptorCount = 3 * maxFragCount;

	TInt r = OpenChannel(descriptorCount);
	if(r != KErrNone)
		return r;

	iTestFn(iChannel, 0, iTotalTransferSize);
	
	iChannel.Close();
	return KErrNone;
	}

TInt CCloseInCb::DoRunTest()
	{
	TInt r = KErrNone;
	RTest test(_L("CCloseInCb test"));

	r = OpenChannel(1);
	test_KErrNone(r);

	const TInt KRequest = 0;
	const TInt KSrcBuf = 0;
	const TInt KDestBuf = 1;

	const TInt size = Min(iTransferSize, Info.iMaxTransferSize);

	r = iChannel.AllocBuffer(KSrcBuf, size);
	test_KErrNone(r);
	iChannel.FillBuffer(KSrcBuf, 'A');
	r = iChannel.AllocBuffer(KDestBuf, size);
	test_KErrNone(r);
	iChannel.FillBuffer(KDestBuf, '\0');

	TRequestStatus rs = KRequestPending;
	r = iChannel.Fragment(KRequest, KSrcBuf, KDestBuf, size, &rs);
	test_KErrNone(r);

	// "X" will cause channel to be closed during callback
	r = iChannel.Execute(_L8("QX0"));
	test_KErrNone(r);

	User::WaitForRequest(rs);
	test_KErrNone(rs.Int());

	test(iChannel.CheckBuffer(KDestBuf, 'A'));
	iChannel.FreeAllBuffers();

	test.Close();
	return KErrNone;
	}

TInt CFragSizeRange::DoRunTest()
	{
	const TInt initialFragmentSize = Min(iInitialFragmentSize, Info.iMaxTransferSize);

	TInt r = KErrNone;
	RTest test(_L("CFragSizeRange test"));

	r = iTimer.CreateLocal();
	test_KErrNone(r);


	TInt fragSize = initialFragmentSize;
	TInt step = 0;
	do
		{
		fragSize -= step;

		// Make sure size is aligned
		fragSize = fragSize & ~Info.iMemAlignMask;
		if(fragSize == 0)
			break;

		r = OpenChannel(iMaxFragCount, fragSize);
		test_KErrNone(r);

		for(iFragCount=1; iFragCount <= iMaxFragCount; iFragCount++)
			{
			test.Printf(_L("Chan %d Fragment size %d bytes, %d fragments, %d iters\n"), iChannelId, fragSize, iFragCount, iInnerIterations);
			for(TInt i=0; i<iInnerIterations; i++)
				{
				r = Transfer(fragSize);
				test_KErrNone(r);
				}
			}
		iChannel.Close();
		// Reduce frag size by a quarter each iteration
		step = (fragSize/4);
		} while (step > 0);

	iTimer.Close();

	test.Close();
	return r;
	}

TInt CFragSizeRange::Transfer(TInt aFragmentSize)
	{
	const TInt KRequest = 0;
	const TInt KSrcBuf = 0;
	const TInt KDestBuf = 1;

	const TInt size = aFragmentSize * iFragCount;

	TInt r = iChannel.AllocBuffer(KSrcBuf, size);
	test_KErrNone(r);
	iChannel.FillBuffer(KSrcBuf, 'A');
	r = iChannel.AllocBuffer(KDestBuf, size);
	XTEST2(r == KErrNone, r, size);
	iChannel.FillBuffer(KDestBuf, '\0');

	// Test simple transfer
	TRequestStatus rs = KRequestPending;
	r = iChannel.Fragment(KRequest, KSrcBuf, KDestBuf, size, &rs);
	test_KErrNone(r);

	test(iChannel.FragmentCheck(KRequest, iFragCount));
	r = iChannel.Execute(_L8("Q0"));
	test_KErrNone(r);

	const TInt microSecTimeout = 1000000; // 1s
	TRequestStatus timerStatus;
	iTimer.After(timerStatus, microSecTimeout);

	User::WaitForRequest(rs, timerStatus);
	if(rs.Int() == KRequestPending)
		{
		RDebug::Printf("Chan %d: Transfer timed out!", iChannelId);
		// timed out
		test(EFalse);
		}
	iTimer.Cancel();
	test_KErrNone(rs.Int());
	test(iChannel.CheckBuffer(KDestBuf, 'A'));

	// Queue, then cancel request - Checks
	// that there there is no spurious callback
	// to the PIL
	r = iChannel.Execute(_L8("Q0C"));
	test_KErrNone(r);

	iChannel.FreeAllBuffers();
	return KErrNone;
	}


// Called when thread completed.
void CTesterThread::RunL()
	{
	TExitType et = iThread.ExitType();
	TInt er = iThread.ExitReason();
	TExitCategoryName ec = iThread.ExitCategory();
	TName name = iThread.Name();
	CLOSE_AND_WAIT(iThread);

	switch (et)
		{
	case EExitKill:
		// nothing to do
		break;
	case EExitPanic:
			{
			User::SetJustInTime(JitEnabled);
			TBuf<128> buffer;
			iTest->ReportState(buffer);
			test.Printf(_L("Tester Thread Panic: %S: Test: %S\n"),
						&name, &buffer);
			if (ec.Match(KTestFailure) == 0)
				test.Panic(_L("Test failure line %d"), er);
			else
				test.Panic(_L("Unexpected panic: %S-%d"), &ec, er);
			break;
			}
	default:
		test.Panic(_L("Invalid thread exit type"));
		}

	TheCriticalSection.Wait();
	if (--ThreadCount == 0)
		{
		Bipper->Cancel();
		test.Console()->Printf(_L("\n"));
		CActiveScheduler::Stop();
		}
	TheCriticalSection.Signal();

	// We commit suicide as the alternative (being deleted by
	// RunTest()) implies keeping a list of all instances in
	// RunTest().
	delete this;
	}


void CTesterThread::DoCancel()
	{
	test.Panic(_L("CTesterThread::DoCancel called"));
	}


static TInt Bip(TAny*)
	{
	test.Console()->Printf(_L("."));
	return 0;
	}


// Execute provided test object in one or more tester threads.
void RunTest(TUint32 aChannelIds[], TInt aMaxThread, CTest* aTest)			 
	{
	test_NotNull(aTest);

	if (aMaxThread == 0)
		{
		test.Printf(_L("transfer mode not supported - skipped\n"));
		return;
		}

	test.Printf(_L("Using %d thread(s)\n"), aMaxThread);

	// We don't want JIT debugging here because the tester threads may panic
	JitEnabled = User::JustInTime();
	User::SetJustInTime(EFalse);

	// must be set before spawning threads to avoid premature active scheduler stop
	ThreadCount = aMaxThread;

	TBuf<128> buffer;
	for (TInt i=0; i<aMaxThread; ++i)
		{
		//each CTesterThread needs its own CTest object
		CTest* dmaTest = aTest->Clone();
		test_NotNull(dmaTest);

		dmaTest->SetChannelId(aChannelIds[i]);

		buffer.Zero();
		dmaTest->AnnounceTest(buffer);
		test.Printf(_L("Thread %d: %S\n"), i, &buffer);
		
		test(new CTesterThread(i, dmaTest) != NULL);
		dmaTest = NULL; //ownership transferred to CTesterThread
		}

	const TTimeIntervalMicroSeconds32 KPeriod = 1000000;	// 1s
	Bipper->Start(KPeriod, KPeriod, Bip);

	CActiveScheduler::Start();

	User::SetJustInTime(JitEnabled);
	}


inline void RunSbTest(TInt aMaxThread, CTest* aTest)
	{
	RunTest(Info.iSbChannels, Min(1,Info.iMaxSbChannels), aTest);
	RunTest(Info.iSbChannels, Min(aMaxThread,Info.iMaxSbChannels), aTest);

	//the orginal isn't needed
	delete aTest;
	}

inline void RunDbTest(TInt aMaxThread, CTest* aTest)
	{
	RunTest(Info.iDbChannels, Min(1,Info.iMaxDbChannels), aTest);
	RunTest(Info.iDbChannels, Min(aMaxThread,Info.iMaxDbChannels), aTest);

	//the orginal isn't needed
	delete aTest;
	}

inline void RunSgTest(TInt aMaxThread, CTest* aTest)
	{
	RunTest(Info.iSgChannels, Min(1,Info.iMaxSgChannels), aTest);
	RunTest(Info.iSgChannels, Min(aMaxThread,Info.iMaxSgChannels), aTest);

	//the orginal isn't needed
	delete aTest;
	}
//////////////////////////////////////////////////////////////////////////////

static void GetChannelInfo()
	{
	RTestDma channel;
	test(channel.GetInfo(Info) == KErrNone);
	test(Info.iMaxSbChannels>0 || Info.iMaxDbChannels>0 || Info.iMaxSgChannels>0);
	}


static void TestOneShot(RTestDma aChannel, TInt aFragmentCount, TInt aSize)
	{
	const TInt KRequest = 0;
	const TInt KSrcBuf = 0;
	const TInt KDestBuf1 = 1;
	const TInt KDestBuf2 = 2;

	TInt r = aChannel.AllocBuffer(KSrcBuf, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KSrcBuf, 'A');
	r = aChannel.AllocBuffer(KDestBuf1, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KDestBuf1, '\0');
	r = aChannel.AllocBuffer(KDestBuf2, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KDestBuf2, '\0');

	// Test simple transfer
	TRequestStatus rs;
	r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf1, aSize, &rs);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest, aFragmentCount));
	r = aChannel.Execute(_L8("Q0"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs);
	XTEST1(rs == KErrNone, rs.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf1, 'A'));

	// Test request reconfiguration.
	aChannel.FillBuffer(KDestBuf1, '\0');
	r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf2, aSize, &rs);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest, aFragmentCount));
	r = aChannel.Execute(_L8("Q0"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs);
	XTEST1(rs == KErrNone, rs.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf1, '\0'));			// previous dest unchanged?
	XTEST(aChannel.CheckBuffer(KDestBuf2, 'A'));

	// Test cancelling
	aChannel.FillBuffer(KDestBuf1, '\0');
	r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf1, aSize);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest, aFragmentCount));
	r = aChannel.Execute(_L8("Q0C"));
	XTEST1(r == KErrNone, r);
	// Part of the destination buffer should be unchanged if the
	// cancel occured before the transfer completed.
#ifdef __DMASIM__
	// At least part of the last destination buffer should be
	// unchanged if cancel occured before the transfer completed.
	// Assert only on WINS as real DMACs are too fast.
	XTEST(! aChannel.CheckBuffer(KDestBuf2, 'C'));
#endif

	// Perform another transfer to ensure cancel operation let the
	// framework in a consistent state.
	aChannel.FillBuffer(KDestBuf1, '\0');
	r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf1, aSize, &rs);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest, aFragmentCount));
	r = aChannel.Execute(_L8("Q0"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs);
	XTEST1(rs == KErrNone, rs.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf1, 'A'));

	//
	// Test failure if the underlying DMA kernel extension allows it.
	//
	// As long as only "CancelAllFragments" is supported, it's okay to
	// always fail on the first fragment.
	//

	if (aChannel.FailNext(1) == KErrNone)
		{
		aChannel.FillBuffer(KDestBuf1, '\0');
		r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf1, aSize, &rs);
		XTEST2(r == KErrNone, r, aSize);
		test(aChannel.FragmentCheck(KRequest, aFragmentCount));
		r = aChannel.Execute(_L8("Q0"));
		XTEST1(r == KErrNone, r);
		User::WaitForRequest(rs);
		XTEST1(rs != KErrNone, rs.Int());
		XTEST(! aChannel.CheckBuffer(KDestBuf1, 'A'));
		r = aChannel.Execute(_L8("C"));
		XTEST1(r == KErrNone, r);

		// Perform another transfer to ensure we are still in a
		// consistent state.
		aChannel.FillBuffer(KDestBuf1, '\0');
		r = aChannel.Fragment(KRequest, KSrcBuf, KDestBuf1, aSize, &rs);
		XTEST2(r == KErrNone, r, aSize);
		test(aChannel.FragmentCheck(KRequest, aFragmentCount));
		r = aChannel.Execute(_L8("Q0"));
		XTEST1(r == KErrNone, r);
		User::WaitForRequest(rs);
		XTEST1(rs == KErrNone, rs.Int());
		XTEST(aChannel.CheckBuffer(KDestBuf1, 'A'));
		}

	aChannel.FreeAllBuffers();
	}


static void TestStreaming(RTestDma aChannel, TInt aFragmentCount, TInt aSize)
	{
	const TInt KRequest0 = 0;
	const TInt KRequest1 = 1;
	const TInt KRequest2 = 2;
	const TInt KSrcBuf0 = 0;
	const TInt KSrcBuf1 = 1;
	const TInt KSrcBuf2 = 2;
	const TInt KDestBuf0 = 3;
	const TInt KDestBuf1 = 4;
	const TInt KDestBuf2 = 5;

	//
	// Allocate and initialise source buffers
	//

	TInt r = aChannel.AllocBuffer(KSrcBuf0, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KSrcBuf0, 'A');

	r = aChannel.AllocBuffer(KSrcBuf1, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KSrcBuf1, 'B');

	r = aChannel.AllocBuffer(KSrcBuf2, aSize);
	XTEST2(r == KErrNone, r, aSize);
	aChannel.FillBuffer(KSrcBuf2, 'C');

	//
	// Allocate destination buffers
	//

	r = aChannel.AllocBuffer(KDestBuf0, aSize);
	XTEST2(r == KErrNone, r, aSize);
	r = aChannel.AllocBuffer(KDestBuf1, aSize);
	XTEST2(r == KErrNone, r, aSize);
	r = aChannel.AllocBuffer(KDestBuf2, aSize);
	XTEST2(r == KErrNone, r, aSize);

	//
	// Test simple transfer.
	// (no need to test for request reconfiguration afterwards because
	// this was exercised in the one-shot test case)
	//

	TRequestStatus rs0;
	r = aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize, &rs0);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
	TRequestStatus rs1;
 	r = aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize, &rs1);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
	TRequestStatus rs2;
	r = aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest2, aFragmentCount));

	r = aChannel.Execute(_L8("Q0Q1Q2"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs0);
	XTEST1(rs0 == KErrNone, rs0.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
	User::WaitForRequest(rs1);
	XTEST1(rs1 == KErrNone, rs1.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf1, 'B'));
	User::WaitForRequest(rs2);
	XTEST1(rs2 == KErrNone, rs2.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));

	//
	// Test cancel
	//

	aChannel.FillBuffer(KDestBuf0, '\0');
	aChannel.FillBuffer(KDestBuf1, '\0');
	aChannel.FillBuffer(KDestBuf2, '\0');

	r = aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
 	r = aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
	r = aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest2, aFragmentCount));

	r = aChannel.Execute(_L8("Q0Q1Q2C"));
	XTEST1(r == KErrNone, r);
#ifdef __DMASIM__
	// At least part of the last destination buffer should be
	// unchanged if cancel occured before the transfer completed.
	// Assert only on WINS as real DMACs are too fast.
	XTEST(! aChannel.CheckBuffer(KDestBuf2, 'C'));
#endif

	//
	// Perform another transfer to ensure cancel operation let the
	// framework in a consistent state.
	//

	aChannel.FillBuffer(KDestBuf0, '\0');
	aChannel.FillBuffer(KDestBuf1, '\0');
	aChannel.FillBuffer(KDestBuf2, '\0');
	// Reconfigure last request to enable transfer completion notification
	r = aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
	r = aChannel.Execute(_L8("Q0Q1Q2"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs2);
	XTEST1(rs2 == KErrNone, rs2.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
	XTEST(aChannel.CheckBuffer(KDestBuf1, 'B'));
	XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));

	//
	// Test for proper implementation of UnlinkHwDes() in the PSL.
	//

	aChannel.FillBuffer(KDestBuf0, '\0');
	aChannel.FillBuffer(KDestBuf1, '\0');
	aChannel.FillBuffer(KDestBuf2, '\0');
	// Reconfigure last request to enable transfer completion notification
	r = aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2);
	XTEST2(r == KErrNone, r, aSize);
	test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
	// Queue first request (Q0)
	r = aChannel.Execute(_L8("Q0"));
	// Wait a second, so next request will be queued on its own
	// (instead of being appended to the previous one)
	User::After(1000000);
	// Queue third request (Q2)
	r = aChannel.Execute(_L8("Q2"));
	XTEST1(r == KErrNone, r);
	User::WaitForRequest(rs2);
	XTEST1(rs2 == KErrNone, rs2.Int());
	XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
	// KDestBuf1 should have been left untouched!
	// If we find all B's in KDestBuf1, that means the last descriptor of the
	// first request (Q0) wasn't properly unlinked and still points to the Q1
	// descriptor chain from the previous run.
	XTEST(aChannel.CheckBuffer(KDestBuf1, '\0'));
	XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));

	//
	// Test failure if the underlying DMA kernel extension allows it.
	//
	// As long as only "CancelAllFragments" is supported, it's okay to
	// always fail on the first fragment.
	//

	if (aChannel.FailNext(1) == KErrNone)
		{
		aChannel.FillBuffer(KDestBuf0, '\0');
		aChannel.FillBuffer(KDestBuf1, '\0');
		aChannel.FillBuffer(KDestBuf2, '\0');
		XTEST(aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize, &rs0) == KErrNone);
		test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize) == KErrNone);
		test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize) == KErrNone);
		test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
		XTEST(aChannel.Execute(_L8("Q0Q1Q2")) == KErrNone);
		User::WaitForRequest(rs0);
		XTEST(rs0 != KErrNone);
		XTEST(! aChannel.CheckBuffer(KDestBuf0, 'A'));
		XTEST(! aChannel.CheckBuffer(KDestBuf1, 'B'));
		XTEST(! aChannel.CheckBuffer(KDestBuf2, 'C'));
		XTEST(aChannel.Execute(_L8("C")) == KErrNone);

		// Transfer again to ensure cancel cleaned-up correctly
		aChannel.FillBuffer(KDestBuf0, '\0');
		aChannel.FillBuffer(KDestBuf1, '\0');
		aChannel.FillBuffer(KDestBuf2, '\0');
		XTEST(aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize, &rs0) == KErrNone);
		test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize, &rs1) == KErrNone);
		test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2) == KErrNone);
		test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
		XTEST(aChannel.Execute(_L8("Q0Q1Q2")) == KErrNone);
		User::WaitForRequest(rs0);
		XTEST(rs0 == KErrNone);
		User::WaitForRequest(rs1);
		XTEST(rs1 == KErrNone);
		User::WaitForRequest(rs2);
		XTEST(rs2 == KErrNone);
		XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
		XTEST(aChannel.CheckBuffer(KDestBuf1, 'B'));
		XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));
		}

	//
	// Test that framework behaves correctly if one or more DMA interrupts are
	// missed.
	//

	if (aChannel.MissNextInterrupts(1) == KErrNone)
		{
		aChannel.FillBuffer(KDestBuf0, '\0');
		aChannel.FillBuffer(KDestBuf1, '\0');
		aChannel.FillBuffer(KDestBuf2, '\0');
		XTEST(aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize, &rs0) == KErrNone);
		test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize, &rs1) == KErrNone);
		test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2) == KErrNone);
		test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
		XTEST(aChannel.Execute(_L8("Q0Q1Q2")) == KErrNone);
		User::WaitForRequest(rs0);
		XTEST(rs0 == KErrNone);
		User::WaitForRequest(rs1);
		XTEST(rs1 == KErrNone);
		User::WaitForRequest(rs2);
		XTEST(rs2 == KErrNone);
		XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
		XTEST(aChannel.CheckBuffer(KDestBuf1, 'B'));
		XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));
		}

	if (aChannel.MissNextInterrupts(2) == KErrNone)
		{
		aChannel.FillBuffer(KDestBuf0, '\0');
		aChannel.FillBuffer(KDestBuf1, '\0');
		aChannel.FillBuffer(KDestBuf2, '\0');
		XTEST(aChannel.Fragment(KRequest0, KSrcBuf0, KDestBuf0, aSize, &rs0) == KErrNone);
		test(aChannel.FragmentCheck(KRequest0, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest1, KSrcBuf1, KDestBuf1, aSize, &rs1) == KErrNone);
		test(aChannel.FragmentCheck(KRequest1, aFragmentCount));
		XTEST(aChannel.Fragment(KRequest2, KSrcBuf2, KDestBuf2, aSize, &rs2) == KErrNone);
		test(aChannel.FragmentCheck(KRequest2, aFragmentCount));
		XTEST(aChannel.Execute(_L8("Q0Q1Q2")) == KErrNone);
		User::WaitForRequest(rs0);
		XTEST(rs0 == KErrNone);
		User::WaitForRequest(rs1);
		XTEST(rs1 == KErrNone);
		User::WaitForRequest(rs2);
		XTEST(rs2 == KErrNone);
		XTEST(aChannel.CheckBuffer(KDestBuf0, 'A'));
		XTEST(aChannel.CheckBuffer(KDestBuf1, 'B'));
		XTEST(aChannel.CheckBuffer(KDestBuf2, 'C'));
		}

	aChannel.FreeAllBuffers();
	}


static TBool ParseCmdLine(TBool& aCrashDbg, TInt& aMaxfrag, TInt& aMaxIter, TInt& aMaxchannel, TInt& aMaxFragSize)
//
// The command line. Syntax is:
//
//     t_dma [enableCrashDebugger [aMaxFrag [aMaxIter [aMaxchannel [aMaxFragSize]]]]]
//
	{
	TBuf<256> cmdline;
	User::CommandLine(cmdline);
	TLex lex(cmdline);

	lex.SkipSpace();

	if (lex.Eos())
		return ETrue;
	if (lex.Val(aCrashDbg) != KErrNone)
		return EFalse;
	lex.SkipSpace();
	if (lex.Eos())
		return ETrue;
	if (lex.Val(aMaxfrag) != KErrNone)
		return EFalse;
	lex.SkipSpace();
	if (lex.Eos())
		return ETrue;
	if (lex.Val(aMaxIter) != KErrNone)
		return EFalse;
	lex.SkipSpace();
	if (lex.Eos())
		return ETrue;
	if (lex.Val(aMaxchannel) != KErrNone)
		return EFalse;
	lex.SkipSpace();
	if (lex.Eos())
		return ETrue;

	return lex.Val(aMaxFragSize) == KErrNone;
	}


TInt E32Main()
	{
	test.Title();

	test.Start(_L("Parsing command-line"));
	// Default values when run with empty command-line
	TInt maxfrag = 16; // 5 fragments needed to exercise fully double-buffering state machine
	TInt maxIter = 3;
	TInt maxchannel = KMaxTInt;
	TBool crashDbg = EFalse;
	TInt maxFragSize = 0x4000; //16k

	(void) ParseCmdLine(crashDbg, maxfrag, maxIter, maxchannel, maxFragSize);

	if (crashDbg)
		{
		User::SetCritical(User::ESystemCritical);
		User::SetProcessCritical(User::ESystemCritical);
		}


	TInt r;
#if defined(__DMASIM__) && defined(__WINS__)
	test.Next(_L("Loading DMA simulator"));
	r = User::LoadLogicalDevice(_L("DMASIM.DLL"));
	test(r == KErrNone || r == KErrAlreadyExists);
#endif

	test.Next(_L("Loading test LDD"));
#ifdef __DMASIM__
	r = User::LoadLogicalDevice(_L("D_DMASIM"));
	test(r == KErrNone || r == KErrAlreadyExists);
#else
	//load either the original test ldd, d_dma.ldd,
	//or d_dma_compat.ldd - an ldd providing the same interface
	//but linked against the new MHA dma framework
	_LIT(KDma, "D_DMA.LDD");
	r = User::LoadLogicalDevice(KDma);
	const TBool dmaPresent = (r == KErrNone || r == KErrAlreadyExists);

	_LIT(KDmaCompat, "D_DMA_COMPAT.LDD");
	r = User::LoadLogicalDevice(KDmaCompat);
	const TBool dmaCompatPresent = (r == KErrNone || r == KErrAlreadyExists);

	if (!(dmaPresent || dmaCompatPresent))
		{
		test.Printf(_L("DMA test driver not found - test skipped\n"));
		return 0;
		}
	else if (dmaPresent && !dmaCompatPresent)
		{
		test.Printf(_L("Loaded %S\n"), &KDma);
		}
	else if (!dmaPresent && dmaCompatPresent)
		{
		test.Printf(_L("Loaded %S\n"), &KDmaCompat);
		}
	else
		{
		test.Printf(_L("The ROM contains %S and %S - only one should be present\n"), &KDma, &KDmaCompat);
		test(EFalse);
		}
#endif

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	__UHEAP_MARK;
	__KHEAP_MARK;

	test.Next(_L("Creating critical section"));
	test(TheCriticalSection.CreateLocal() == KErrNone);

	test.Next(_L("Creating active scheduler"));
	CActiveScheduler* pS = new CActiveScheduler;
	test(pS != NULL);
	CActiveScheduler::Install(pS);

	test.Next(_L("Creating bipper"));
	Bipper = CPeriodic::New(CActive::EPriorityStandard);
	test(Bipper != NULL);

	test.Next(_L("Getting channel info"));
	GetChannelInfo();

	test.Next(_L("Test that channel can be closed from callback"));
	test.Next(_L("sb"));
	RunSbTest(maxchannel, new CCloseInCb() );
	test.Next(_L("db"));
	RunDbTest(maxchannel, new CCloseInCb() );
	test.Next(_L("sg"));
	RunSgTest(maxchannel, new CCloseInCb() );

	test.Next(_L("Testing different fragment sizes"));

	const TInt rangeFragSize = 4096;
#ifdef __DMASIM__
	// Use fewer iterations on the emulator
	// since it is slower. Also this test is really
	// intended to find errors in PSL implmentations
	const TInt iterPerFragSize = 1;
#else
	const TInt iterPerFragSize = 10;
#endif
	const TInt rangeMaxFragCount = 5;

	test.Next(_L("sb"));
	RunSbTest(maxchannel, new CFragSizeRange(1, rangeMaxFragCount, rangeFragSize, iterPerFragSize));
	test.Next(_L("db"));
	RunDbTest(maxchannel, new CFragSizeRange(1, rangeMaxFragCount, rangeFragSize, iterPerFragSize));
	test.Next(_L("sg"));
	RunSgTest(maxchannel, new CFragSizeRange(1, rangeMaxFragCount, rangeFragSize, iterPerFragSize));


	// Size for the single transfer test
	TInt totalTransferSize = 64 * KKilo;

	test.Next(_L("Testing one shot single buffer transfer"));
	RunSbTest(maxchannel, new CFragmentationTest(TestOneShot, maxIter, maxfrag, maxFragSize));
	RunSbTest(maxchannel, new CDefaultFragTest(TestOneShot, maxIter, totalTransferSize));

	test.Next(_L("Testing one shot double buffer transfer"));
	RunDbTest(maxchannel, new CFragmentationTest(TestOneShot, maxIter, maxfrag, maxFragSize));
	RunDbTest(maxchannel, new CDefaultFragTest(TestOneShot, maxIter, totalTransferSize));

	test.Next(_L("Testing one shot scatter/gather transfer"));
	RunSgTest(maxchannel, new CFragmentationTest(TestOneShot, maxIter, maxfrag, maxFragSize));
	RunSgTest(maxchannel, new CDefaultFragTest(TestOneShot, maxIter, totalTransferSize));

	test.Next(_L("Testing streaming single buffer transfer"));
	RunSbTest(maxchannel, new CFragmentationTest(TestStreaming, maxIter, maxfrag, maxFragSize));
	RunSbTest(maxchannel, new CDefaultFragTest(TestStreaming, maxIter, totalTransferSize));

	test.Next(_L("Testing streaming double buffer transfer"));
	RunDbTest(maxchannel, new CFragmentationTest(TestStreaming, maxIter, maxfrag, maxFragSize));
	RunDbTest(maxchannel, new CDefaultFragTest(TestStreaming, maxIter, totalTransferSize));

	test.Next(_L("Testing streaming scatter/gather transfer"));
	RunSgTest(maxchannel, new CFragmentationTest(TestStreaming, maxIter, maxfrag, maxFragSize));
	RunSgTest(maxchannel, new CDefaultFragTest(TestStreaming, maxIter, totalTransferSize));

	delete pS;
	delete Bipper;
	TheCriticalSection.Close();

	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	__KHEAP_MARKEND;
	__UHEAP_MARKEND;

	test.End();
	return 0;
	}
