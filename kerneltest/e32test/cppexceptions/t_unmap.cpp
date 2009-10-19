// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\cppexceptions\t_unmap.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>
#include <e32def.h>
#include <e32def_private.h>

#include "d_unmap.h"

_LIT(KTestName, "t_unmap");
_LIT(KTestThreadName, "t_unmap test thread");
_LIT(KNopThreadName, "nop [DLL unload checking] thread");
_LIT(KTUnmapPanic, "t_unmap");
_LIT(KThread, "Thread");

_LIT(KUnhandledExcCategory, "KERN-EXEC");
const TInt KUnhandledExcReason = 3;

enum TUnmapPanic
	{
	EPanickingThread = 123456789
	};

RTest test(KTestName);

RTest testThreadA(KTestThreadName);
RTest testThreadB(KTestThreadName);
RTest testThreadC(KTestThreadName);
RTest testThreadD(KTestThreadName);
RTest testThreadE(KTestThreadName);
RTest testThreadF(KTestThreadName);
RTest testThreadG(KTestThreadName);
RTest testThreadH(KTestThreadName);

RTest testThreadI(KTestThreadName);
RTest testThreadJ(KTestThreadName);
RTest testThreadK(KTestThreadName);

RSemaphore Thread1Semaphore;
RSemaphore Thread2Semaphore;

RSemaphore FinishedOpSemaphore;

RLibrary ThreadALibraryHandle;
RLibrary ThreadBLibraryHandle;
RLibrary ThreadCLibraryHandle;
RLibrary ThreadDLibraryHandle;
RLibrary ThreadELibraryHandle;
RLibrary ThreadFLibraryHandle;
RLibrary ThreadGLibraryHandle;
RLibrary ThreadHLibraryHandle;
RLibrary ThreadILibraryHandle;
RLibrary ThreadJLibraryHandle;
RLibrary ThreadKLibraryHandle;

TBool CheckKernelHeap;

void TestThreads();

TInt ThreadA(TAny*);
TInt ThreadB(TAny*);
TInt ThreadC(TAny*);
TInt ThreadD(TAny*);
TInt ThreadE(TAny*);
TInt ThreadF(TAny*);
TInt ThreadG(TAny*);
TInt ThreadH(TAny*);
TInt ThreadI(TAny*);
TInt ThreadJ(TAny*);
TInt ThreadK(TAny*);

TInt DoThreadAL(TAny*);
TInt DoThreadBL(TAny*);
TInt DoThreadCL(TAny*);
TInt DoThreadDL(TAny*);
TInt DoThreadEL(TAny*);
TInt DoThreadFL(TAny*);
TInt DoThreadGL(TAny*);
TInt DoThreadHL(TAny*);
TInt DoThreadIL(TAny*);
TInt DoThreadJL(TAny*);
TInt DoThreadKL(TAny*);

struct STestThreadInfo
	{
	TThreadFunction	iThreadFn;
	TExitType		iExitType;
	TInt			iMappedSignals;
	TInt			iLeaveSignals;
	};

static STestThreadInfo const TheThreadArray[] =
	{
		{ &ThreadA, EExitPanic, 0, 0 },
		{ &ThreadB, EExitKill, 0, 0 },
		{ &ThreadC, EExitPanic, 1, 0 },
		{ &ThreadD, EExitPanic, 2, 0 },
		{ &ThreadE, EExitKill, 2, 2 },
		{ &ThreadF, EExitPanic, 2, 1 },
		{ &ThreadG, EExitKill, 3, 1 },
		{ &ThreadH, EExitKill, 1, 1 },
		{ &ThreadI, EExitKill, 1, 3 },
		{ &ThreadJ, EExitPanic, 1, 2 },
		{ &ThreadK, EExitPanic, 1, 1 }
	};

struct SNopThreadInfo
	{
	TLibraryFunction iFunc;
#ifdef __WINS__
	TInt32 iOriginalContents;
#endif
	};

static SNopThreadInfo NopThreadInfo;

static const TInt TheThreadCount = (sizeof(TheThreadArray) / sizeof(STestThreadInfo));

static const TInt KHeapSize = 0x2000;

TInt E32Main()
   	{
	// Turn off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();
	
	test.Start(_L("Check code seg unmapping over User::Leave()/C++ exceptions."));

	__UHEAP_MARK;
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, TestThreads());

		test.Printf(_L("Returned %d, expected %d\n"), r, KErrNone);
		test(r == KErrNone);
   		}

	delete cleanup;
   	//
   	__UHEAP_MARKEND;

	test.End();

   	return r;
   	}

TInt NopThread(TAny*)
	{
#ifdef __WINS__
	TInt32 current = *(TInt*)NopThreadInfo.iFunc;
	if (current != NopThreadInfo.iOriginalContents)
		current = *(TInt32*)NULL; // cause panic
#endif
	TInt r = NopThreadInfo.iFunc();
	if (r)
		return KErrNone;
	else
		return KErrGeneral;
	}

void TestLoadWhileUnload();

void TestThreads()
	{
	__KHEAP_MARK;

	test.Next(_L("Create synchronisation semaphores"));
	TInt r = Thread1Semaphore.CreateLocal(0);
	test(r == KErrNone);

	r = Thread2Semaphore.CreateLocal(0);
	test(r == KErrNone);

	r = FinishedOpSemaphore.CreateLocal(0);
	test(r == KErrNone);

	// Turn off JIT [threads can panic to test exit cleanup]
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	TInt count, count2;
	
	// Do kernel heap checking
	CheckKernelHeap = ETrue;

	test.Next(_L("Run threads on their own"));
	for (count = 0; count < TheThreadCount ; ++count)
		{
		// Set up descriptor for thread's name
		TBuf16<7> name(KThread);
		name.Append('A' + count);

		// Create thread
		RThread thread;
		TInt r = thread.Create(
			name,
			TheThreadArray[count].iThreadFn,
			KDefaultStackSize,
			KHeapSize,
			KHeapSize,
			&Thread1Semaphore
		);
		test(r == KErrNone);

		// Set up notification of thread's death
		TRequestStatus status;
		thread.Logon(status);

		// Load library
		RLibrary library;
		r = library.Load(KLeavingDll);
		test(r == KErrNone);

		// Remember the address of the NOP function
		NopThreadInfo.iFunc = library.Lookup(2);
#ifdef __WINS__
		NopThreadInfo.iOriginalContents = *(TInt32*)NopThreadInfo.iFunc;
#endif

		// Run thread
		thread.Resume();

		// Wait until it has an open handle to the library
		FinishedOpSemaphore.Wait();

		// Close our handle to the library
		library.Close();

		// Check library is still loaded
		for (count2 = 0; count2 < TheThreadArray[count].iMappedSignals; ++count2)
			{
			// Tell it we're ready to go
			Thread1Semaphore.Signal();

			// Wait for it to finish next step
			FinishedOpSemaphore.Wait();

			// Create NOP thread to call NOP function to check DLL still loaded
			RThread nopThread;
			r = nopThread.Create(
				KNopThreadName,
				NopThread,
				KDefaultStackSize,
				KHeapSize,
				KHeapSize,
				NULL
			);
			test(r == KErrNone);

			// Set up notification of thread's death
			TRequestStatus nopStatus;
			nopThread.Logon(nopStatus);

			// Run thread
			nopThread.Resume();

			// Wait for it to die
			User::WaitForRequest(nopStatus);

			// Check the exit info
			test(nopThread.ExitType() == EExitKill);
			test(nopThread.ExitReason() == KErrNone);

			// Close thread handle
			CLOSE_AND_WAIT(nopThread);
			}

		// Check User::Leave() library unloading behaviour
		for (count2 = 0; count2 < TheThreadArray[count].iLeaveSignals; ++count2)
			{
			// Tell it we're ready to go
			Thread1Semaphore.Signal();

			// Wait for it to finish next step
			FinishedOpSemaphore.Wait();

			// Create NOP thread to call NOP function to check whether DLL is still loaded
			RThread nopThread;
			r = nopThread.Create(
				KNopThreadName,
				NopThread,
				KDefaultStackSize,
				KHeapSize,
				KHeapSize,
				NULL
			);
			test(r == KErrNone);

			// Set up notification of thread's death
			TRequestStatus nopStatus;
			nopThread.Logon(nopStatus);

			// Run thread
			nopThread.Resume();

			// Wait for it to die
			User::WaitForRequest(nopStatus);

			// Check the exit info
#ifdef __LEAVE_EQUALS_THROW__
			test(nopThread.ExitType() == EExitKill);
			test(nopThread.ExitReason() == KErrGeneral);
#else //!__LEAVE_EQUALS_THROW__
			test(nopThread.ExitType() == EExitPanic);
			test(nopThread.ExitCategory() == KUnhandledExcCategory);
			test(nopThread.ExitReason() == KUnhandledExcReason);
#endif //__LEAVE_EQUALS_THROW__

			// Close thread handle
			CLOSE_AND_WAIT(nopThread);
			}

		// Tell it we're ready to go again
		Thread1Semaphore.Signal();

		if (TheThreadArray[count].iExitType == EExitKill)
			{
			// Wait for it to finish last step
			FinishedOpSemaphore.Wait();

			User::After(100000);	// let supervisor run

			// Create NOP thread to call NOP function to check DLL is unloaded
			RThread nopThread;
			r = nopThread.Create(
				KNopThreadName,
				NopThread,
				KDefaultStackSize,
				KHeapSize,
				KHeapSize,
				NULL
			);
			test(r == KErrNone);

			// Set up notification of thread's death
			TRequestStatus nopStatus;
			nopThread.Logon(nopStatus);

			// Run thread
			nopThread.Resume();

			// Wait for it to die
			User::WaitForRequest(nopStatus);

			// Check the exit info
			test(nopThread.ExitType() == EExitPanic);
			test(nopThread.ExitCategory() == KUnhandledExcCategory);
			test(nopThread.ExitReason() == KUnhandledExcReason);

			// Close thread handle
			CLOSE_AND_WAIT(nopThread);

			// Let main thread die now
			Thread1Semaphore.Signal();
			}

		// Wait for thread to exit
		User::WaitForRequest(status);

		// Check the exit type & category
		test(thread.ExitType() == TheThreadArray[count].iExitType);

		// Check category & reason, if appropriate
		if (thread.ExitType() == EExitPanic)
			{
			test(thread.ExitCategory() == KTUnmapPanic);
			test(thread.ExitReason() == EPanickingThread);
			}

		// Close thread handle
		thread.Close();
		}

	// Turn off kernel heap checking
	CheckKernelHeap = EFalse;

	test.Next(_L("Run threads against each other"));
	for (count = 0; count < TheThreadCount ; ++count)
		{
		for (count2 = 0; count2 < TheThreadCount ; ++count2)
			{
			// Can't run the same threads back to back
			if (count == count2)
				{
				continue;
				}

			// Set up descriptors for threads' names
			_LIT(KFirstThread, " - 1");
			_LIT(KSecondThread, " - 2");
			TBuf16<11> name(KThread);
			TBuf16<11> name2(KThread);
			name.Append('A' + count);
			name.Append(KFirstThread);
			name2.Append('A' + count2);
			name2.Append(KSecondThread);

			// Create threads
			RThread thread;
			TInt r = thread.Create(
				name,
				TheThreadArray[count].iThreadFn,
				KDefaultStackSize,
				KHeapSize,
				KHeapSize,
				&Thread1Semaphore
			);
			test(r == KErrNone);

			RThread thread2;
			r = thread2.Create(
				name2,
				TheThreadArray[count2].iThreadFn,
				KDefaultStackSize,
				KHeapSize,
				KHeapSize,
				&Thread2Semaphore
			);
			test(r == KErrNone);

			// Set up notification of threads' death
			TRequestStatus status, status2;
			thread.Logon(status);
			thread2.Logon(status2);

			// Run first thread
			thread.Resume();

			// Wait until just before it's closed the library handle
			FinishedOpSemaphore.Wait();

			// Run second thread
			thread2.Resume();

			// Wait until just before it's closed the library handle
			FinishedOpSemaphore.Wait();

			// Tell first thread we're ready to go
			TInt signals =	TheThreadArray[count].iMappedSignals +
							TheThreadArray[count].iLeaveSignals +
							((TheThreadArray[count].iExitType == EExitPanic) ? 1 : 2);
			Thread1Semaphore.Signal(signals);

			// Eat up 'FinishedOp' signals
			while(--signals>0)
				FinishedOpSemaphore.Wait();

			// Wait for it to finish
			User::WaitForRequest(status);

			// Check the exit type & category of the first thread
			test(thread.ExitType() == TheThreadArray[count].iExitType);

			// Check category & reason of the first thread, if appropriate
			if (thread.ExitType() == EExitPanic)
				{
				test(thread.ExitCategory() == KTUnmapPanic);
				test(thread.ExitReason() == EPanickingThread);
				}

			// Tell second thread we're ready to go
			signals = TheThreadArray[count2].iMappedSignals +
					  TheThreadArray[count2].iLeaveSignals +
					  ((TheThreadArray[count2].iExitType == EExitPanic) ? 1 : 2);
			Thread2Semaphore.Signal(signals);

			// Eat up 'FinishedOp' signals
			while(--signals>0)
				FinishedOpSemaphore.Wait();

			// Wait for it to finish
			User::WaitForRequest(status2);

			// Check the exit type & category of the second thread
			test(thread2.ExitType() == TheThreadArray[count2].iExitType);

			// Check category & reason of the second thread, if appropriate
			if (thread2.ExitType() == EExitPanic)
				{
				test(thread2.ExitCategory() == KTUnmapPanic);
				test(thread2.ExitReason() == EPanickingThread);
				}

			// Close thread handles
			CLOSE_AND_WAIT(thread);
			CLOSE_AND_WAIT(thread2);
			}
		}

	// Test two processes at once to deal with race conditions
	test.Printf(_L("Create two processes at once to map the same library\n"));
	RSemaphore procSem1, procSem2;
	test(procSem1.CreateGlobal(KNullDesC, 0)==KErrNone);
	test(procSem2.CreateGlobal(KNullDesC, 0)==KErrNone);
	RProcess proc1, proc2;
	test(proc1.Create(_L("T_UNMAP2"), KNullDesC)==KErrNone);
	test(proc2.Create(_L("T_UNMAP2"), KNullDesC)==KErrNone);
	test(proc1.SetParameter(1, procSem1)==KErrNone);
	test(proc1.SetParameter(2, procSem2)==KErrNone);
	test(proc2.SetParameter(1, procSem2)==KErrNone);
	test(proc2.SetParameter(2, procSem1)==KErrNone);
	TRequestStatus proc1stat, proc2stat;
	proc1.Logon(proc1stat);
	proc2.Logon(proc2stat);
	test.Printf(_L("Start processes\n"));
	proc1.Resume();
	proc2.Resume();
	test.Printf(_L("Wait for them to exit\n"));
	User::WaitForRequest(proc1stat);
	test(proc1.ExitType() == EExitKill);
	test(proc1.ExitReason() == KErrNone);
	User::WaitForRequest(proc2stat);
	test(proc2.ExitType() == EExitKill);
	test(proc2.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(proc1);
	CLOSE_AND_WAIT(proc2);
	procSem1.Close();
	procSem2.Close();

	// Test load while unload
	TestLoadWhileUnload();

	// Restore JIT setting
	User::SetJustInTime(jit);

	// Close synchronisation semaphores
	Thread1Semaphore.Close();
	Thread2Semaphore.Close();
	FinishedOpSemaphore.Close();

	__KHEAP_MARKEND;
	}

// Test loading a library while another thread is unloading it in an unwind
void TestLoadWhileUnload()
	{
	// Set up descriptor for thread's name
	TBuf16<7> name(KThread);
	name.Append('H');

	// Create thread
	RThread thread;
	TInt r = thread.Create(
		name,
		ThreadH,
		KDefaultStackSize,
		KHeapSize,
		KHeapSize,
		&Thread1Semaphore
	);
	test(r == KErrNone);

	// Set up notification of thread's death
	TRequestStatus status;
	thread.Logon(status);

	// Run thread
	thread.Resume();

	// Wait until it has an open handle to the library
	FinishedOpSemaphore.Wait();

	// Tell it to go ahead and leave
	Thread1Semaphore.Signal();

	// Wait for it to start unwinding
	FinishedOpSemaphore.Wait();

	// Tell it to go ahead and close the library handle
	Thread1Semaphore.Signal();

	// Wait for it to have closed the library
	FinishedOpSemaphore.Wait();

	// Load library
	RLibrary library;
	r = library.Load(KLeavingDll);
	test(r == KErrNone);

	// Remember the address of the NOP function
	NopThreadInfo.iFunc = library.Lookup(2);
#ifdef __WINS__
	NopThreadInfo.iOriginalContents = *(TInt32*)NopThreadInfo.iFunc;
#endif

	User::After(100000);	// let supervisor run

	// Check User::Leave() library unloading behaviour
	for (TInt i = 0; i < 2; ++i)
		{
		// Create NOP thread to call NOP function to check whether DLL is still loaded
		RThread nopThread;
		r = nopThread.Create(
			KNopThreadName,
			NopThread,
			KDefaultStackSize,
			KHeapSize,
			KHeapSize,
			NULL
		);
		test(r == KErrNone);

		// Set up notification of thread's death
		TRequestStatus nopStatus;
		nopThread.Logon(nopStatus);

		// Run thread
		nopThread.Resume();

		// Wait for it to die
		User::WaitForRequest(nopStatus);

		// Check the exit info
		test(nopThread.ExitType() == EExitKill);
		test(nopThread.ExitReason() == KErrNone);

		// Close thread handle
		CLOSE_AND_WAIT(nopThread);

		// Tell it we're ready to go
		Thread1Semaphore.Signal();

		// Wait for it to finish next step
		if (i==0)
			FinishedOpSemaphore.Wait();
		}

	// Wait for thread to exit
	User::WaitForRequest(status);

	// Check the exit type & category
	test(thread.ExitType() == EExitKill);

	// Close thread handle
	CLOSE_AND_WAIT(thread);

	// Close our handle to the library
	library.Close();

	// Create NOP thread to call NOP function to check DLL is unloaded
	RThread nopThread;
	r = nopThread.Create(
		KNopThreadName,
		NopThread,
		KDefaultStackSize,
		KHeapSize,
		KHeapSize,
		NULL
	);
	test(r == KErrNone);

	// Set up notification of thread's death
	TRequestStatus nopStatus;
	nopThread.Logon(nopStatus);

	// Run thread
	nopThread.Resume();

	// Wait for it to die
	User::WaitForRequest(nopStatus);

	// Check the exit info
	test(nopThread.ExitType() == EExitPanic);
	test(nopThread.ExitCategory() == KUnhandledExcCategory);
	test(nopThread.ExitReason() == KUnhandledExcReason);

	// Close thread handle
	CLOSE_AND_WAIT(nopThread);
	}	

//
// Cleanup operations
//

void Checkpoint(TAny* aSemaphore)
	{
	if(aSemaphore)
		{
		FinishedOpSemaphore.Signal();
		static_cast<RSemaphore*>(aSemaphore)->Wait();
		}
	}

void DieDieDie(TAny* aSemaphore)
	{
	// Check-point before panicking
	Checkpoint(aSemaphore);
	
	User::Panic(KTUnmapPanic, EPanickingThread);
	}

void PauseLeaving(TAny* aSemaphore)
	{
	Checkpoint(aSemaphore);
	}

void TrapLeave(TAny* aSemaphore)
	{
	TRAP_IGNORE(
		{
		CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

		Checkpoint(aSemaphore);

		User::Leave(KErrGeneral);

		CleanupStack::Pop(); // pause op
		}
	);

	Checkpoint(aSemaphore);
	}

void TrapLeaveAndDie(TAny* aSemaphore)
	{
	TRAP_IGNORE(
		{
		CleanupStack::PushL(TCleanupItem(&DieDieDie, aSemaphore));

		Checkpoint(aSemaphore);

		User::Leave(KErrGeneral);

		CleanupStack::Pop(); //DieDieDie op
		}
	);
	}

void TrapLeaveAndClose_ThreadE(TAny* aSemaphore)
	{
	CleanupStack::Pop(&ThreadELibraryHandle);

	TRAP_IGNORE(
		{
		CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

		CleanupClosePushL(ThreadELibraryHandle);

		CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

		Checkpoint(aSemaphore);

		User::Leave(KErrGeneral);

		CleanupStack::Pop(); //pre-close pause op

		CleanupStack::Pop(&ThreadELibraryHandle);

		CleanupStack::Pop(); //post-close pause op
		}
	);

	Checkpoint(aSemaphore);
	}

void TrapLeaveCloseAndDie_ThreadF(TAny* aSemaphore)
	{
	CleanupStack::Pop(&ThreadFLibraryHandle);

	TRAP_IGNORE(
		{
		CleanupStack::PushL(TCleanupItem(&DieDieDie, aSemaphore));

		CleanupClosePushL(ThreadFLibraryHandle);

		CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

		Checkpoint(aSemaphore);

		User::Leave(KErrGeneral);

		CleanupStack::Pop(); //pre-close pause op

		CleanupStack::Pop(&ThreadFLibraryHandle);

		CleanupStack::Pop(); //DieDieDie op
		}
	);
	}


/**
Here's a list of interesting things that could happen to a thread which
has an open handle to library on cleanup stack:

a)	Panicks
b)	Closes handle normally
c)	Leaves and panicks before closing handle
d)	Recursively leaves and panicks before closing handle
e)	Recursively leaves and closes handle in recursive leave
f)	Recursively leaves and panicks in recursive leave, after closing handle
g)	Recursively leaves and returns to first leave without closing handle; first leave closes handle
h)	Leaves and closes handle
i)	Leaves and closes handle, then recursively leaves
j)	Leaves and closes handle, then recursively leaves and panicks in recursive leave
k)	Leaves and panicks after closing handle, but before leave completes

Other ideas yet to be done:

l)	TRAPs a leave, then closes handle
m)	TRAPs a recusive leave, then closes handle

The thread functions below correspond to these.

These are the ways a library's code seg can be held open by a process:

a)	Open handle to the library
b)	Open reference to code seg because the last reference to the library was closed during a leave and
	the process has not gone leave-idle

We then test both these by testing at extra points during the sequences above that
the code segments are either mapped or unmapped, as appropriate.
*/

TInt ThreadA(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadA) RTest(KNullDesC);
	testThreadA.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadAL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadA.Printf(_L("A: Returned %d, expected %d\n"), r, KErrNone);
		testThreadA(r == KErrNone);
   		}

	delete cleanup;
   	//
	testThreadA.End();
	testThreadA.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

   	return r;
	}

TInt DoThreadAL(TAny* aSemaphore)
	{
	testThreadA.Printf(_L("A: Loading DLL.\n"));
	User::LeaveIfError(ThreadALibraryHandle.Load(KLeavingDll));

    testThreadA.Printf(_L("A: Pushing cleanup item to kill this thread!\n"));
	CleanupStack::PushL(TCleanupItem(&DieDieDie, aSemaphore));

	testThreadA.Printf(_L("A: Cleaning up Panic operation.\n"));
	CleanupStack::PopAndDestroy(); // DieDieDie op

	ThreadALibraryHandle.Close();

	return KErrNone;
	}

TInt ThreadB(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadB) RTest(KNullDesC);
	testThreadB.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadBL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadB.Printf(_L("B: Returned %d, expected %d\n"), r, KErrNone);
		testThreadB(r == KErrNone);
   		}

	delete cleanup;
   	//
	testThreadB.End();
	testThreadB.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadBL(TAny* aSemaphore)
	{
    testThreadB.Printf(_L("B: Loading DLL.\n"));
	User::LeaveIfError(ThreadBLibraryHandle.Load(KLeavingDll));

    testThreadB.Printf(_L("B: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadBLibraryHandle);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);
	
	testThreadB.Printf(_L("B: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadBLibraryHandle);

	return KErrNone;
	}

TInt ThreadC(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadC) RTest(KNullDesC);
	testThreadC.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadCL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadC.Printf(_L("C: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadC(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadC.End();
	testThreadC.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadCL(TAny* aSemaphore)
	{
    testThreadC.Printf(_L("C: Loading DLL.\n"));
	User::LeaveIfError(ThreadCLibraryHandle.Load(KLeavingDll));

    testThreadC.Printf(_L("C: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadCLibraryHandle);

	testThreadC.Printf(_L("C: Pushing cleanup item to kill this thread before closing handle!\n"));
	CleanupStack::PushL(TCleanupItem(&DieDieDie, aSemaphore));

    testThreadC.Printf(_L("C: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadCLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);
	
	testThreadC.Printf(_L("C: Calling leaving function.\n"));
	(*leaving)();

	testThreadC.Printf(_L("C: Cleaning up Panic operation.\n"));
	CleanupStack::Pop(aSemaphore); // DieDieDie op

	testThreadC.Printf(_L("C: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadCLibraryHandle);

	return KErrNone;
	}

TInt ThreadD(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadD) RTest(KNullDesC);
	testThreadD.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadDL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadD.Printf(_L("D: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadD(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadD.End();
	testThreadD.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadDL(TAny* aSemaphore)
	{
    testThreadD.Printf(_L("D: Loading DLL.\n"));
	User::LeaveIfError(ThreadDLibraryHandle.Load(KLeavingDll));

    testThreadD.Printf(_L("D: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadDLibraryHandle);

	testThreadD.Printf(_L("D: Pushing cleanup item to recursively leave and then kill this thread before closing handle!\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeaveAndDie, aSemaphore));

    testThreadD.Printf(_L("D: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadDLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadD.Printf(_L("D: Calling leaving function.\n"));
	(*leaving)();

	testThreadD.Printf(_L("D: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadDLibraryHandle);

	testThreadD.Printf(_L("D: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(aSemaphore); // recursive leave op

	return KErrNone;
	}

TInt ThreadE(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadE) RTest(KNullDesC);
	testThreadE.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadEL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadE.Printf(_L("E: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadE(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadE.End();
	testThreadE.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadEL(TAny* aSemaphore)
	{
    testThreadE.Printf(_L("E: Loading DLL.\n"));
	User::LeaveIfError(ThreadELibraryHandle.Load(KLeavingDll));

    testThreadE.Printf(_L("E: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadELibraryHandle);

	testThreadE.Printf(_L("E: Pushing cleanup item to recursively leave and then close the handle in the recursive leave\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeaveAndClose_ThreadE, aSemaphore));

    testThreadE.Printf(_L("E: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadELibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadE.Printf(_L("E: Calling leaving function.\n"));
	(*leaving)();

	testThreadE.Printf(_L("E: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(aSemaphore); // recursive leave op

	// NB: library handle removed from cleanup stack

	return KErrNone;
	}

TInt ThreadF(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadF) RTest(KNullDesC);
	testThreadF.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadFL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadF.Printf(_L("F: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadF(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadF.End();
	testThreadF.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadFL(TAny* aSemaphore)
	{
    testThreadF.Printf(_L("F: Loading DLL.\n"));
	User::LeaveIfError(ThreadFLibraryHandle.Load(KLeavingDll));

    testThreadF.Printf(_L("F: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadFLibraryHandle);

	testThreadF.Printf(_L("F: Pushing cleanup item to recursively leave and then panic in recursive leave after closing the library handle\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeaveCloseAndDie_ThreadF, aSemaphore));

    testThreadF.Printf(_L("F: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadFLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadF.Printf(_L("F: Calling leaving function.\n"));
	(*leaving)();

	testThreadF.Printf(_L("F: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(aSemaphore); // recursive leave op

	// NB: library handle removed from cleanup stack

	return KErrNone;
	}

TInt ThreadG(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadG) RTest(KNullDesC);
	testThreadG.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadGL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadG.Printf(_L("G: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadG(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadG.End();
	testThreadG.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadGL(TAny* aSemaphore)
	{
    testThreadG.Printf(_L("G: Loading DLL.\n"));
	User::LeaveIfError(ThreadGLibraryHandle.Load(KLeavingDll));

    testThreadG.Printf(_L("G: Pushing cleanup item to synchronise after closing library handle.\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadG.Printf(_L("G: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadGLibraryHandle);

	testThreadG.Printf(_L("G: Pushing cleanup item to recursively leave, doing nothing, before closing handle\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeave, aSemaphore));

    testThreadG.Printf(_L("G: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadGLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadG.Printf(_L("G: Calling leaving function.\n"));
	(*leaving)();

	testThreadG.Printf(_L("G: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(aSemaphore); // trap leave op

	testThreadG.Printf(_L("G: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadGLibraryHandle);

	return KErrNone;
	}

TInt ThreadH(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadH) RTest(KNullDesC);
	testThreadH.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadHL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadH.Printf(_L("H: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadH(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadH.End();
	testThreadH.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadHL(TAny* aSemaphore)
	{
    testThreadH.Printf(_L("H: Loading DLL.\n"));
	User::LeaveIfError(ThreadHLibraryHandle.Load(KLeavingDll));

    testThreadH.Printf(_L("H: Pushing cleanup item to synchronise after closing library handle.\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadH.Printf(_L("H: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadHLibraryHandle);

	testThreadH.Printf(_L("H: Pushing cleanup item to synchronise during leave\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadH.Printf(_L("H: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadHLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadH.Printf(_L("H: Calling leaving function.\n"));
	(*leaving)();

	testThreadH.Printf(_L("H: Cleaning up leave pausing operation.\n"));
	CleanupStack::Pop(aSemaphore); // pause leave op

	testThreadH.Printf(_L("H: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadHLibraryHandle);

	return KErrNone;
	}

TInt ThreadI(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadI) RTest(KNullDesC);
	testThreadI.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadIL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadI.Printf(_L("I: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadI(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadI.End();
	testThreadI.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadIL(TAny* aSemaphore)
	{
    testThreadI.Printf(_L("I: Loading DLL.\n"));
	User::LeaveIfError(ThreadILibraryHandle.Load(KLeavingDll));

	testThreadI.Printf(_L("I: Pushing cleanup item to recursively leave, doing nothing, after closing handle\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeave, aSemaphore));

    testThreadI.Printf(_L("I: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadILibraryHandle);

	testThreadI.Printf(_L("I: Pushing cleanup item to synchronise during leave\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadI.Printf(_L("I: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadILibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadI.Printf(_L("I: Calling leaving function.\n"));
	(*leaving)();

	testThreadI.Printf(_L("I: Cleaning up leave pausing operation.\n"));
	CleanupStack::Pop(aSemaphore); // pause leave op

	testThreadI.Printf(_L("I: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadILibraryHandle);

	testThreadI.Printf(_L("I: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(); // trap leave op

	return KErrNone;
	}

TInt ThreadJ(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadJ) RTest(KNullDesC);
	testThreadJ.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadJL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadJ.Printf(_L("J: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadJ(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadJ.End();
	testThreadJ.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadJL(TAny* aSemaphore)
	{
    testThreadJ.Printf(_L("J: Loading DLL.\n"));
	User::LeaveIfError(ThreadJLibraryHandle.Load(KLeavingDll));

	testThreadJ.Printf(_L("J: Pushing cleanup item to recursively leave and panic, after closing handle\n"));
	CleanupStack::PushL(TCleanupItem(&TrapLeaveAndDie, aSemaphore));

    testThreadJ.Printf(_L("J: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadJLibraryHandle);

	testThreadJ.Printf(_L("J: Pushing cleanup item to synchronise during leave\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadJ.Printf(_L("J: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadJLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadJ.Printf(_L("J: Calling leaving function.\n"));
	(*leaving)();

	testThreadJ.Printf(_L("J: Cleaning up leave pausing operation.\n"));
	CleanupStack::Pop(aSemaphore); // pause leave op

	testThreadJ.Printf(_L("J: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadJLibraryHandle);

	testThreadJ.Printf(_L("J: Cleaning up recursive leave operation.\n"));
	CleanupStack::Pop(); // leave and die op

	return KErrNone;
	}

TInt ThreadK(TAny* aSemaphore)
	{
	__UHEAP_MARK;
	if (CheckKernelHeap)
		{
		__KHEAP_MARK;
		}

	new (&testThreadK) RTest(KNullDesC);
	testThreadK.Start(KNullDesC);
   	//
   	CTrapCleanup* cleanup = CTrapCleanup::New();
   	TInt r = KErrNoMemory;
   	if (cleanup)
   		{
		TRAP(r, DoThreadKL(aSemaphore));

		// Check-point after closing the library handle
		Checkpoint(aSemaphore);

		testThreadK.Printf(_L("K: Returned %d, expected %d\n"), r, KErrGeneral);
		testThreadK(r == KErrGeneral);

		r = KErrNone;
   		}

	delete cleanup;
   	//
	testThreadK.End();
	testThreadK.Close();

	if (CheckKernelHeap)
		{
		User::After(100000);	// let supervisor run
		__KHEAP_MARKEND;
		}
   	__UHEAP_MARKEND;

	return r;
	}

TInt DoThreadKL(TAny* aSemaphore)
	{
    testThreadK.Printf(_L("K: Loading DLL.\n"));
	User::LeaveIfError(ThreadKLibraryHandle.Load(KLeavingDll));

	testThreadK.Printf(_L("K: Pushing cleanup item to panic, after closing handle\n"));
	CleanupStack::PushL(TCleanupItem(&DieDieDie, aSemaphore));

    testThreadK.Printf(_L("K: Pushing handle to dynamically loaded DLL onto the cleanup stack.\n"));
	CleanupClosePushL(ThreadKLibraryHandle);

	testThreadK.Printf(_L("K: Pushing cleanup item to synchronise during leave\n"));
	CleanupStack::PushL(TCleanupItem(&PauseLeaving, aSemaphore));

    testThreadK.Printf(_L("K: Looking up leaving function.\n"));
	TLibraryFunction leaving = ThreadKLibraryHandle.Lookup(1);
	User::LeaveIfNull((TAny*)leaving);

	// Check-point whilst holding the open library handle
	Checkpoint(aSemaphore);

	testThreadK.Printf(_L("K: Calling leaving function.\n"));
	(*leaving)();

	testThreadK.Printf(_L("K: Cleaning up leave pausing operation.\n"));
	CleanupStack::Pop(aSemaphore); // pause leave op

	testThreadK.Printf(_L("K: Cleaning up DLL handle.\n"));
	CleanupStack::PopAndDestroy(&ThreadKLibraryHandle);

	testThreadK.Printf(_L("K: Cleaning up panic operation.\n"));
	CleanupStack::Pop(); // die op

	return KErrNone;
	}
