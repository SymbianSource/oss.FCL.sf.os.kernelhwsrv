// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <e32test.h>

#include "bm_suite.h"

class RTLatency : public BMProgram
	{

	struct Measurement
		{
		RBMChannel::TMode	iMode;
		TPtrC				iName;
		TBool				iForcePSwitch;

		Measurement(RBMChannel::TMode aMode, const TDesC& aName, TBool aForcePSwitch = EFalse) : 
			iMode(aMode), iName(aName), iForcePSwitch(aForcePSwitch) {}
		};

public :
	RTLatency() : BMProgram(_L("Real-Time Latency"))
		{}
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);

	static TBMResult	iResults[];
	static Measurement	iMeasurements[];

	void Perform(RBMChannel::TMode, TBMResult*, TBMUInt64 aIter, TBool aForcePSwitch);
	static TInt Child(TAny*);
	};

RTLatency::Measurement		RTLatency::iMeasurements[] =
	{ 
		Measurement(RBMChannel::EInterruptLatency, _L("Interrupt Latency")),
		Measurement(RBMChannel::EKernelPreemptionLatency, _L("Kernel Thread Preemption Latency (Idle)")),
		Measurement(RBMChannel::EKernelPreemptionLatency, _L("Kernel Thread Preemption Latency(Busy)"), ETrue),
		Measurement(RBMChannel::EUserPreemptionLatency, _L("User Thread Preemption Latency (Idle)")),
		Measurement(RBMChannel::EUserPreemptionLatency, _L("User Thread Preemption Latency (Busy)"), ETrue),
		Measurement(RBMChannel::ENTimerJitter, _L("NTimer Jitter")),
		Measurement(RBMChannel::ETimerStampOverhead, _L("Getting Time Stamp Overhead"))
	};
TBMResult	RTLatency::iResults[sizeof(RTLatency::iMeasurements)/sizeof(RTLatency::iMeasurements[0])];

static RTLatency rtLatency;

struct ChildArgs : public TBMSpawnArgs
	{
	RSemaphore iSem;
	ChildArgs () : TBMSpawnArgs(RTLatency::Child, KBMPriorityLow, ETrue, sizeof(*this))
		{
		TInt r = KErrNone;
		do
			{		//"Bm_SuiteSemaphore" is created/deleted a number of times during the test.
			if (r)	//If kernel did not clean up the previous instance, wait a sec and retry.
				User::After(1000000);
			r = iSem.CreateGlobal(_L("Bm_SuiteSemaphore"), 0);
			}
		while(KErrAlreadyExists == r);

		BM_ERROR(r, r == KErrNone);
		}
	void ChildOpen()
		{	
		iSem.Duplicate(iParent);
		}
	~ChildArgs ()
		{
		iSem.Close();
		}
	};

//
// Child process entry point.
//
TInt RTLatency::Child(TAny* ptr)
	{
	ChildArgs* ca = (ChildArgs*) ptr;
		// get handles to all objects shared with the parent
	ca->ChildOpen();
		// signal the parent that the child is ready
	ca->iSem.Signal();
		// loop forever
	for(;;) {}
	}

void RTLatency::Perform(RBMChannel::TMode aMode, TBMResult* aResult, TBMUInt64 aIter, TBool aForcePSwitch)
	{
	ChildArgs ca;
	MBMChild* child = NULL;
	if (aForcePSwitch)
		{
			// spawn a busy running low-pririty child process
		child = rtLatency.SpawnChild(&ca);
		ca.iSem.Wait();
		}

	RBMChannel ch;
	TInt r = ch.Open(aMode);
	if(r==KErrInUse)
		return; // Assume that resources are being used for other forms of latency testing
	BM_ERROR(r, r == KErrNone);
	while(aIter--) 
		{
			// request an interrupt
		ch.RequestInterrupt();
		TBMTicks ticks;
			// wait for the result. At this point the child becomes running.
		ch.Result(&ticks);
		aResult->Cumulate(ticks);
		}
	ch.Close();

	if (aForcePSwitch)
		{
		child->Kill();
		child->WaitChildExit();
		}
	}

					
TBMResult* RTLatency::Run(TBMUInt64 aIter, TInt* aCount)
	{
	TInt count = sizeof(iResults)/sizeof(iResults[0]);

	for (TInt i = 0; i < count; ++i)
		{
		iResults[i].Reset(iMeasurements[i].iName);
		Perform(iMeasurements[i].iMode, &iResults[i], aIter, iMeasurements[i].iForcePSwitch);
		iResults[i].Update();
		}
	
	*aCount = count;
	return iResults;
	}

void AddrtLatency()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&rtLatency;
	bmSuite->Next()=next;
	}
