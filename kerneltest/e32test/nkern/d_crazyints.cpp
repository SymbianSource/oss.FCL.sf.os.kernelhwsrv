// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkern\d_crazyints.cpp
//
// Overview:
// kernel-side test (driver) for Crazy-Interrupts functionality
//

#include <platform.h>
#include <nk_priv.h>
#include <kernel/kern_priv.h>
#include "d_crazyints.h"


const TInt KMaxNumChannels = 1; // we only support one client at the time

_LIT(KLddRootName,"d_crazyints");
_LIT(KIntTestThreadName,"InterruptsTest");

// Constructor
DDeviceIntsTest::DDeviceIntsTest()
	{
	iParseMask = 0;
	iUnitsMask = 0;  // No info, no PDD, no Units
	iVersion = TVersion(KIntTestMajorVersionNumber,
	        KIntTestMinorVersionNumber, KIntTestBuildVersionNumber);
	}

DDeviceIntsTest::~DDeviceIntsTest()
	{
	}

// Install the device driver.
TInt DDeviceIntsTest::Install()
	{
	return (SetName(&KLddRootName));
	}

void DDeviceIntsTest::GetCaps(TDes8& aDes) const
	{
	TPckgBuf<TCapsProxyClient> b;
	b().version = TVersion(KIntTestMajorVersionNumber, KIntTestMinorVersionNumber, KIntTestBuildVersionNumber);
	Kern::InfoCopy(aDes, b);
	}

// Create a channel on the device.
TInt DDeviceIntsTest::Create(DLogicalChannelBase*& aChannel)
	{
	if (iOpenChannels >= KMaxNumChannels)
		return KErrOverflow;

	aChannel = new DChannIntsTest;
	return aChannel ? KErrNone : KErrNoMemory;
	}

DChannIntsTest::DChannIntsTest()
#ifdef __SMP__
	: iTimer(Handler, this)
#endif
	{
	iClient = &Kern::CurrentThread();
	// Increase the DThread's ref count so that it does not close without us
	((DObject*)iClient)->Open();
	}

DChannIntsTest::~DChannIntsTest()
	{
	iDfcQue->Destroy();
	// decrement the DThread's reference count
	Kern::SafeClose((DObject*&) iClient, NULL);
	}

TInt DChannIntsTest::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	TInt r = Kern::DynamicDfcQCreate(iDfcQue, KIntTestThreadPriority, KIntTestThreadName);

	if(r == KErrNone)
		{
		SetDfcQ(iDfcQue);
		iMsgQ.Receive();
		}
#ifdef __SMP__
	return r;
#else
	return KErrNotSupported;
#endif
	}

void DChannIntsTest::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*) aMsg;
	TInt id = m.iValue;

	if (id == ECloseMsg)
		{
		iMsgQ.iMessage->Complete(KErrNone, EFalse);
		return;
		}
	else if (id == KMaxTInt)
		{
		m.Complete(KErrNone, ETrue);
		return;
		}

	if (id < 0)
		{
		TRequestStatus* pS = (TRequestStatus*) m.Ptr0();
		TInt r = DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		if (r != KErrNone)
			{
			Kern::RequestComplete(iClient, pS, r);
			}
		m.Complete(KErrNone, ETrue);
		}
	else
		{
		TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(r, ETrue);
		}
	}

#ifdef __SMP__
TBool DChannIntsTest::CrazyInterruptsEnabled()
	{
	// check, if crazy-interrupts are enabled..
	TSuperPage& sp = Kern::SuperPage();
	return (sp.KernelConfigFlags() & EKernelConfigSMPCrazyInterrupts);
	}

// Isr handler - it will check, if previously was run on the same core..
void DChannIntsTest::Handler (TAny *aParam)
	{
	__KTRACE_OPT(KTESTFAST,Kern::Printf("Handler\n"));
	DChannIntsTest *a = (DChannIntsTest*) aParam;

	// clear the flag for the core that we're now executing..
	TUint curr_core = NKern::CurrentCpu();
	a->iStatus &= ~(1u << curr_core);

	// decrement the counter..and kick the timer again - as required..
	TInt cnt = __e32_atomic_add_acq32(&a->iRunCount, ~(0u));
	if(cnt >= 1)
		a->iTimer.Again(KTimerWaitValue);
	}

// This test checks if the timer handler run in ISR context executes on all available
// CPU cores when crazy interrupts are enabled.
TInt DChannIntsTest::TestCrazyInts()
	{
	__KTRACE_OPT(KTESTFAST,Kern::Printf("TestCrazyInts"));

	if(!CrazyInterruptsEnabled())
		return KErrNotSupported;

	// Check how many CPUs we have in the system -and set corresponding flags in
	// iStatus. As an indication, that ISRs are run on a different cores all these flags
	// should be cleared in the Handler.
	TUint status = 0;
	for(TInt i = 0; i < NKern::NumberOfCpus(); ++i)
		{
		status |= 1u << i;
		}
	iStatus = status;

	// Kick-off a Timer to run it's handler in a ISR context..
	iRunCount = KNumOfTimes;
	iTimer.OneShot(KTimerWaitValue);

	//busy wait until it all completes..
	while (__e32_atomic_load_acq32(&iRunCount) > 0)
		NKern::Sleep(1);

	if(iStatus)
		{
		if((iStatus ^ status) == 1)
			{
			// we must be running on the single core version, whilst the kernel flag is set
			// this might happen, if the Kernel (nkern) was not compiled with SMP_CRAZY_INTERRUPTS defined
			// check, if variant.mmh for this platform contains:  macro SMP_CRAZY_INTERRUPTS
			Kern::Printf("Interrupts running only on CPU0 and EKernelConfigSMPCrazyInterrupts flag is set \n\r is this - wrong configuration? ", iStatus);
			}
		else
			Kern::Printf("ISR was not executed on all CPUs..(%x)", iStatus);

		return KErrGeneral;
		}

	return KErrNone;
	}
#endif

// to handle synchronous requests from the client
TInt DChannIntsTest::DoControl(TInt aId, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	switch (aId)
		{
		case RBusIntTestClient::ETestCrazyInts:
			{
#ifdef __SMP__
			r = TestCrazyInts();
#else
			r = KErrNotSupported;
#endif
			break;
			}

		default:
			{
			__KTRACE_OPT(KTESTFAST,Kern::Printf("DChannIntsTest::DoControl():Unrecognized value for aId=0x%x\n", aId));
			r = KErrArgument;
			break;
			}
		}
	return r;
	}

// to handle asynchronous requests from the client
TInt DChannIntsTest::DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	__KTRACE_OPT(KTESTFAST,Kern::Printf("DChannIntsTest::DoRequest(aId=0x%x, aStatus=0x%x, a1=0x%x, a2=0x%x\n",
					aId, aStatus, a1, a2));

	TInt r = KErrNone;
	switch (aId)
		{
		default:
			{
			__KTRACE_OPT(KTESTFAST,Kern::Printf("DChannIntsTest::DoRequest(): unrecognized value for aId=0x%x\n", aId));
			r = KErrArgument;
			break;
			}
		}
	return r;
	}

// LDD entry point
DECLARE_STANDARD_LDD()
	{
	return new DDeviceIntsTest;
	}

