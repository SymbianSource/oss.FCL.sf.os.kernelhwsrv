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
// Overview:
// Test and benchmark kernel-side utility operations  
// API Information:
// RBusLogicalChannel			
// Details:
// - Create a list of benchmark modules and start running them one by one;
// each module contains a set of measurement units, each unit runs for a fixed
// amount of time in a series of iterations; the results, minimum, maximum and
// average times are displayed on the screen;
// The tests use a high resolution timer implemented kernel side in a device
// driver.
// - The test contains the following benchmark modules:
// - Real-time latency module measures:
// - interrupt latency by calculating the time taken from when an
// interrupt is generated until the ISR starts
// - kernel thread latency by calculating the time taken from an ISR
// scheduling a DFC to signal the kernel thread until the kernel thread
// starts running
// - kernel thread latency as above while a CPU intensive low priority
// user thread runs at the same time
// - user thread latency by calculating the time taken from an ISR 
// scheduling a DFC to signal the user thread until the user thread
// starts running
// - user thread latency as above while a CPU intensive low priority
// user thread runs at the same time
// - NTimer period jitter by calculating the actual period as the delta
// between two consecutive NTimer callbacks that store the current time;
// the jitter is the difference between the actual period and a theoretical
// period.
// - timer overhead by calculating the delta of time between two consecutive
// timestamps requested from the high precision timer implemented in the
// device driver; the calls are made from kernel side code
// - Overhead module measures:
// - timer overhead by calculating the delta of time between two consecutive
// timestamps requested from the high precision timer implemented in the
// device driver; the calls are made from user side code
// - Synchronization module measures: 
// - mutex passing, local mutex contention, remote mutex contention, 
// local semaphore latency, remote semaphore latency, 
// local thread semaphore latency, remote thread semaphore latency.
// - Client-server framework module measures:
// - For local high priority, local low priority, remote high priority 
// and remote low priority: connection request latency, connection
// reply latency, request latency, request response time, reply latency.
// - Threads modules measures:
// - Thread creation latency, thread creation suicide, thread suicide,
// thread killing, setting per thread data, getting per thread data.
// - Properties module measures:
// - Local int notification latency, remote int notification latency, 
// local byte(1) notification latency, remote byte(1) notification latency, 
// local byte(8) notification latency, remote byte(8) notification latency, 
// local byte(512) notification latency, remote byte(512) notification latency, 
// int set overhead, byte(1) set overhead, byte(8) set overhead, byte(512) set 
// overhead, int get overhead, byte(1) get overhead, byte(8) get overhead, 
// byte(512) get overhead. 
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "bm_suite.h"
#include <e32svr.h>
#include <u32hal.h>

//
// The default value of the time allocated for one benchmark program.  
//
static TInt KBMSecondsPerProgram = 30;
//
// The initial number of iterations to estimate the acctual number of iteration. 
//
static TInt KBMCalibrationIter = 64;

//
// Global handle to high-resolution timer. 
//
RBMTimer bmTimer;
//
// The head of the benchmark programs' list
//
BMProgram* bmSuite; 
//
// Global handle to the kernel side benchmark utilty API 
//
static RBMDriver bmDriver;

TBMResult::TBMResult(const TDesC& aName) : iName(aName) 
	{
	Reset();
	}

void TBMResult::Reset()
	{
	::bmTimer.Period(&iMinTicks);
	iMaxTicks = 0;
	iCumulatedTicks = 0;
	iCumulatedIterations = 0;
	iIterations = 0;
	iMin = 0;
	iMax = 0;
	iAverage = 0;
	}

void TBMResult::Reset(const TDesC& aName)
	{
	Reset();
	iName.Set(aName);
	}

void TBMResult::Cumulate(TBMTicks aTicks)
{
	if (aTicks < iMinTicks) iMinTicks = aTicks;
	if (iMaxTicks < aTicks) iMaxTicks = aTicks;

	iCumulatedTicks += aTicks;
	if (iCumulatedIterations < KHeadSize)
		{
		iHeadTicks[iCumulatedIterations] = aTicks;
		}
		// use the array as a circular buufer to store last KTailSize results
		// (would not really know which one was actually the last)
	iTailTicks[iCumulatedIterations % KTailSize] = aTicks;
	++iCumulatedIterations;
	
}


void TBMResult::Cumulate(TBMTicks aTicks, TBMUInt64 aIter)
{
	iCumulatedIterations += aIter;
	iCumulatedTicks += aTicks;
}

void TBMResult::Update()
{
	if (iCumulatedIterations == 0) return;
	iIterations = iCumulatedIterations;
	::bmTimer.TicksToNs(&iMinTicks, &iMin);
	::bmTimer.TicksToNs(&iMaxTicks, &iMax);
	TBMTicks averageTicks = iCumulatedTicks/TBMUInt64(iCumulatedIterations);
	::bmTimer.TicksToNs(&averageTicks, &iAverage);
	TInt i;
	for (i = 0; i < KHeadSize; ++i) 
		{
		::bmTimer.TicksToNs(&iHeadTicks[i], &iHead[i]);
		}
	for (i = 0; i < KTailSize; ++i) 
		{
		::bmTimer.TicksToNs(&iTailTicks[i], &iTail[i]);
		}
	}

inline TBMNs TTimeIntervalMicroSecondsToTBMNs(TTimeIntervalMicroSeconds us)
	{
	return BMUsToNs(*(TBMUInt64*)&us);
	}

TBMNs TBMTimeInterval::iStampPeriodNs;
TBMTicks TBMTimeInterval::iStampPeriod;

void TBMTimeInterval::Init()
	{
	::bmTimer.Period(&iStampPeriod); 
	::bmTimer.TicksToNs(&iStampPeriod, &iStampPeriodNs);
}

void TBMTimeInterval::Begin()
	{
	//
	// Order is important: read first low-precision timer, then the high-precision one. 
	// Therefore, two high-precision timer reads will be accounted in the low-precision interval,
	// that's better than the opposite.
	//
	iTime.HomeTime();
	::bmTimer.Stamp(&iStamp);
	}

TBMNs TBMTimeInterval::EndNs()
	{
	//
	// Now, in the reverse order
	//
	TBMTicks stamp;
	::bmTimer.Stamp(&stamp);
	TTime time;	
	time.HomeTime();
	TBMNs ns = TTimeIntervalMicroSecondsToTBMNs(time.MicroSecondsFrom(iTime));
	//
	// If the interval fits in the high-precision timer period we can use it;
	// otherwise, use the low-precision timer.
	//
	if (ns < iStampPeriodNs)
		{
		stamp = TBMTicksDelta(iStamp, stamp);
		::bmTimer.TicksToNs(&stamp, &ns);
		}
	return ns;
	}

TBMTicks TBMTimeInterval::End()
	{
	//
	// The same as the previous one but returns ticks
	//

	TBMTicks stamp;
	::bmTimer.Stamp(&stamp);
	TTime time;	
	time.HomeTime();
	TBMNs ns = TTimeIntervalMicroSecondsToTBMNs(time.MicroSecondsFrom(iTime));
	if (ns < iStampPeriodNs)
		{
		stamp = TBMTicksDelta(iStamp, stamp);
		}
	else
		{
			// multiply first - privileging precision to improbable overflow.
		stamp = (ns * iStampPeriod) / iStampPeriodNs; 
		}
	return stamp;
	}

TInt BMProgram::SetAbsPriority(RThread aThread, TInt aNewPrio)
	{
	TInt aOldPrio=0;
	TInt r = ::bmDriver.SetAbsPriority(aThread, aNewPrio, &aOldPrio);
	BM_ERROR(r, r == KErrNone);
	return aOldPrio;
	}

const TInt TBMSpawnArgs::KMagic = 0xdeadbeef;

TBMSpawnArgs::TBMSpawnArgs(TThreadFunction aChildFunc, TInt aChildPrio, TBool aRemote, TInt aSize)
	{
	iMagic = KMagic;
	iParentId = RThread().Id();
		// get a thread handle meaningful in the context of any other thread. 
		// (RThread() doesn't work since contextual!)
	TInt r = iParent.Open(iParentId);
	BM_ERROR(r, r == KErrNone);
	iRemote = aRemote;
	iChildFunc = aChildFunc;
	iChildPrio = aChildPrio;
	iSize = aSize;
	}

TBMSpawnArgs::~TBMSpawnArgs()
	{
	iParent.Close();
	}

//
// An object of CLocalChild class represents a "child" thread created by its "parent" thread 
// in the parent's process through BmProgram::SpawnChild() interface.
//
// CLocalChild class is typically used (invoked) by the parent's thread. 
//
class CLocalChild : public CBase, public MBMChild
	{
private:
	BMProgram*		iProg;
public:
	RThread			iChild;
	TRequestStatus	iExitStatus;

	CLocalChild(BMProgram* aProg)
		{
		iProg = aProg;
		}
	
	virtual void WaitChildExit();
	virtual void Kill();
	};

void CLocalChild::Kill()
	{
	iChild.Kill(KErrCancel);
	}

void CLocalChild::WaitChildExit()
	{
	User::WaitForRequest(iExitStatus);
	CLOSE_AND_WAIT(iChild);
	//
	// Lower the parent thread priority and then restore the current one 
	// to make sure that the kernel-side thread destruction DFC had a chance to complete.
	//
	TInt prio = BMProgram::SetAbsPriority(RThread(), iProg->iOrigAbsPriority);
	BMProgram::SetAbsPriority(RThread(), prio);
	delete this;
	}

//
// Local (i.e. sharing the parent's process) child's entry point
//
TInt LocalChildEntry(void* ptr)
	{
	TBMSpawnArgs* args = (TBMSpawnArgs*) ptr;
	args->iChildOrigPriority = BMProgram::SetAbsPriority(RThread(), args->iChildPrio);
	return args->iChildFunc(args);
	}

MBMChild* BMProgram::SpawnLocalChild(TBMSpawnArgs* args)
	{
	CLocalChild* child = new CLocalChild(this);
	BM_ERROR(KErrNoMemory, child);
	TInt r = child->iChild.Create(KNullDesC, ::LocalChildEntry, 0x2000, NULL, args);
	BM_ERROR(r, r == KErrNone);
	child->iChild.Logon(child->iExitStatus);
	child->iChild.Resume();
	return child;
	}

//
// An object of CRemoteChild class represents a "child" thread created by its "parent" thread 
// as a separate process through BmProgram::SpawnChild() interface.
//
// CRemoteChild class is typically used (invoked) by the parent's thread. 
//
class CRemoteChild : public CBase, public MBMChild
	{
private:
	BMProgram*		iProg;
public:
	RProcess		iChild;
	TRequestStatus	iExitStatus;

	CRemoteChild(BMProgram* aProg)
		{
		iProg = aProg;
		}

	virtual void WaitChildExit();
	virtual void Kill();
	};

void CRemoteChild::Kill()
	{
	iChild.Kill(KErrCancel);
	}

void CRemoteChild::WaitChildExit()
	{
	User::WaitForRequest(iExitStatus);
	CLOSE_AND_WAIT(iChild);
	//
	// Lower the parent thread priority and then restore the current one 
	// to make sure that the kernel-side thread destruction DFC had a chance to complete.
	//
	TInt prio = BMProgram::SetAbsPriority(RThread(), iProg->iOrigAbsPriority);
	BMProgram::SetAbsPriority(RThread(), prio);
	delete this;
	}

//
// Remote (i.e. running in its own process) child's entry point.
// Note that the child's process entry point is still E32Main() process (see below)
//
TInt ChildMain(TBMSpawnArgs* args)
	{
	args->iChildOrigPriority = BMProgram::SetAbsPriority(RThread(), args->iChildPrio);
		// get a handle to the parent's thread in the child's context.
	TInt r = args->iParent.Open(args->iParentId);
	BM_ERROR(r, r == KErrNone);
	return args->iChildFunc(args);
	}

MBMChild* BMProgram::SpawnRemoteChild(TBMSpawnArgs* args)
	{
	CRemoteChild* child = new CRemoteChild(this);
	BM_ERROR(KErrNoMemory, child);
	//
	// Create the child process and pass args as a UNICODE command line.
	// (we suppose that the args size is multiple of sizeof(TUint16))
	//
	BM_ASSERT((args->iSize % sizeof(TUint16)) == 0);
	TInt r = child->iChild.Create(RProcess().FileName(), TPtrC((TUint16*) args, args->iSize/sizeof(TUint16)));
	BM_ERROR(r, (r == KErrNone) );
	child->iChild.Logon(child->iExitStatus);
	child->iChild.Resume();
	return child;
	}

MBMChild* BMProgram::SpawnChild(TBMSpawnArgs* args)
	{
	MBMChild* child;
	if (args->iRemote)
		{
		child = SpawnRemoteChild(args);
		}
	else
		{
		child = SpawnLocalChild(args);
		}
	return child;
	}

//
// The benchmark-suite entry point.
//
GLDEF_C TInt E32Main()
	{
	RTest test(_L("Benchmark Suite"));
	test.Title();

	AddProperty();
	AddThread();
	AddIpc();
	AddSync();
	AddOverhead();
	AddrtLatency();

	TInt r = User::LoadPhysicalDevice(KBMPddFileName);
	BM_ERROR(r, (r == KErrNone) || (r == KErrAlreadyExists));

	r = User::LoadLogicalDevice(KBMLddFileName);
	BM_ERROR(r, (r == KErrNone) || (r == KErrAlreadyExists));

	r = ::bmTimer.Open();
	BM_ERROR(r, (r == KErrNone));

	r = ::bmDriver.Open();
	BM_ERROR(r, (r == KErrNone));

	TBMTimeInterval::Init();

	TInt seconds = KBMSecondsPerProgram;

	TInt len = User::CommandLineLength();
	if (len)
		{
		//
		// Copy the command line in a buffer
		//
		TInt size = len * sizeof(TUint16);
		HBufC8* hb = HBufC8::NewMax(size);
		BM_ERROR(KErrNoMemory, hb);
		TPtr cmd((TUint16*) hb->Ptr(), len);
		User::CommandLine(cmd);
		//
		// Check for the TBMSpawnArgs magic number.
		//
		TBMSpawnArgs* args = (TBMSpawnArgs*) hb->Ptr();	
		if (args->iMagic == TBMSpawnArgs::KMagic)
			{
			//
			// This is a child process -  call it's entry point
			//
			return ::ChildMain(args);
			}
		else
			{
			//
			// A real command line - the time (in seconds) for each benchmark program.
			//
			TLex l(cmd);
			r = l.Val(seconds);
			if (r != KErrNone)
				{
				test.Printf(_L("Usage: bm_suite <seconds>\n"));
				BM_ERROR(r, 0);
				}
			}
		delete hb;
		}

	{
	TBMTicks ticks = 1;
	TBMNs ns;
	::bmTimer.TicksToNs(&ticks, &ns);
	test.Printf(_L("High resolution timer tick %dns\n"), TInt(ns));
	test.Printf(_L("High resolution timer period %dms\n"), BMNsToMs(TBMTimeInterval::iStampPeriodNs));
	}

	test.Start(_L("Performance Benchmark Suite"));

	BMProgram* prog = ::bmSuite;
	while (prog) {
		//
		// For each program from the benchmark-suite's list
		//

		//
		// Remember the number of open handles. Just for a sanity check ....
		//
		TInt start_thc, start_phc;
		RThread().HandleCount(start_phc, start_thc);

		test.Printf(_L("%S\n"), &prog->Name());
	
		//
		// A benchmark-suite's thread can run at any of three possible absolute priorities:
		//		KBMPriorityLow, KBMPriorityMid and KBMPriorityHigh.
		// The main thread starts individual benchmark programs at KBMPriorityMid
		//
		prog->iOrigAbsPriority = BMProgram::SetAbsPriority(RThread(), KBMPriorityMid);

		//
		// First of all figure out how many iteration would be required to run this program
		// for the given number of seconds.
		//
		TInt count;
		TBMNs ns = 0;
		TBMUInt64 iter = KBMCalibrationIter;
		for (;;) 
			{
			TBMTimeInterval ti;
			ti.Begin();
			prog->Run(iter, &count);
			ns = ti.EndNs();
				// run at least 100ms (otherwise, could be too much impricise ...)
			if (ns > BMMsToNs(100)) break;
			iter *= 2;
			}
		test.Printf(_L("%d iterations in %dms\n"), TInt(iter), BMNsToMs(ns));
		iter = (BMSecondsToNs(seconds) * iter) / ns;
		test.Printf(_L("Go for %d iterations ...\n"), TInt(iter));

		//
		// Now the real run ...
		//
		TBMResult* results = prog->Run(iter, &count);

			// Restore the original priority
		BMProgram::SetAbsPriority(RThread(), prog->iOrigAbsPriority);

		//
		// Now print out the results
		//
		for (TInt i = 0; i < count; ++i) 
			{
			if (results[i].iMax)
				{
				test.Printf(_L("%S. %d iterations; Avr: %dns; Min: %dns; Max: %dns\n"), 
							&results[i].iName, TInt(results[i].iIterations),
							TInt(results[i].iAverage), TInt(results[i].iMin), TInt(results[i].iMax));

				TInt j;
				BM_ASSERT((TBMResult::KHeadSize % 4) == 0);
				test.Printf(_L("Head:"));
				for (j = 0; j < TBMResult::KHeadSize; j += 4)
					{
					test.Printf(_L(" %d %d %d %d "), 
								TInt(results[i].iHead[j]), TInt(results[i].iHead[j+1]), 
								TInt(results[i].iHead[j+2]), TInt(results[i].iHead[j+3]));
					}
				test.Printf(_L("\n"));

				BM_ASSERT((TBMResult::KTailSize % 4) == 0);
				test.Printf(_L("Tail:"));
				for (j = 0; j < TBMResult::KTailSize; j += 4)
					{
					test.Printf(_L(" %d %d %d %d "), 
								TInt(results[i].iTail[j]), TInt(results[i].iTail[j+1]), 
								TInt(results[i].iTail[j+2]), TInt(results[i].iTail[j+3]));
					}
				test.Printf(_L("\n"));
				}
			else
				{
				test.Printf(_L("%S. %d iterations; Avr: %dns\n"), 
							&results[i].iName, TInt(results[i].iIterations), TInt(results[i].iAverage));
				}

			}

		//
		// Sanity check for open handles
		//
		TInt end_thc, end_phc;
		RThread().HandleCount(end_phc, end_thc);
		BM_ASSERT(start_thc == end_thc);
		BM_ASSERT(start_phc == end_phc);
			// and also for pending requests ...
		BM_ASSERT(RThread().RequestCount() == 0);

		prog = prog->Next();
//
//		This can be used to run forever ...
//
//		if (prog == NULL)
//			prog = ::bmSuite;
//
	}
	
	test.End();

	::bmDriver.Close();
	::bmTimer.Close();
	return 0;
	}


void bm_assert_failed(char* aCond, char* aFile, TInt aLine)
	{
	RTest test(_L("Benchmark Suite Assert Failed"));
	test.Title();

	TPtrC8 fd((TUint8*)aFile);
	TPtrC8 cd((TUint8*)aCond);

	HBufC* fhb = HBufC::NewMax(fd.Length());
	test(fhb != 0);
	HBufC* chb = HBufC::NewMax(cd.Length());
	test(chb != 0);

	fhb->Des().Copy(fd);
	chb->Des().Copy(cd);

	test.Printf(_L("Assertion %S failed;  File: %S; Line %d;\n"), chb, fhb, aLine);
	test(0);
	}

void bm_error_detected(TInt aError, char* aCond, char* aFile, TInt aLine)
	{
	RTest test(_L("Benchmark Suite Error Detected"));
	test.Title();

	TPtrC8 fd((TUint8*)aFile);
	TPtrC8 cd((TUint8*)aCond);

	HBufC* fhb = HBufC::NewMax(fd.Length());
	test(fhb != 0);
	HBufC* chb = HBufC::NewMax(cd.Length());
	test(chb != 0);

	fhb->Des().Copy(fd);
	chb->Des().Copy(cd);

	test.Printf(_L("Error: %d; Cond: %S; File: %S; Line %d;\n"), aError, chb, fhb, aLine);
	test(0);
	}
