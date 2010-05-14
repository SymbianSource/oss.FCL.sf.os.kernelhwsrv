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
// e32test\demandpaging\t_threadcreate.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"

#include "t_dpcmn.h"

enum
	{
	EUnspecified, 
	EPaged, 
	EUnpaged,
	};

_LIT(KGlobalThreadName, "gThreadGlobal");

RSemaphore gSem1;
RSemaphore gSem2;
TBool gStackPaged;
TUint8* gStackPtr = NULL;

struct SThreadPagedInfo
	{
	TBool iHeapPaged;
	TBool iStackPaged;
	};

TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}

TInt TestThreadFunction(TAny* aPtr)
	{
	for (TInt i = 0; i<2; i++)
		{
		if (i == 1)
			{
			User::SetRealtimeState(User::ERealtimeStateOn);
			RDebug::Printf("aPtr %x",aPtr);
			ReadByte((TUint8*)aPtr);
			}
		}
	return KErrNone;
	}

//
// IsStackPaged
//
// Determine whether the stack is paged by flushing the cache and attempting
// to read a byte that has been paged out
//
//
TInt IsStackPaged(const TUint8* aPtr)
	{
	RThread thread;
	TInt r;
	r = thread.Create(KNullDesC, TestThreadFunction, 0x1000, NULL, (TAny*)aPtr);
	if (r != KErrNone)
		{
		return r;
		}

	TRequestStatus status;
	thread.Logon(status);
	if(status.Int() != KRequestPending)
		{
		return KErrGeneral;
		}
	thread.Resume();
	User::WaitForRequest(status);
	if (thread.ExitType() == EExitPanic &&
		thread.ExitCategory() == _L("KERN-EXEC") &&
		thread.ExitReason() == EIllegalFunctionForRealtimeThread)
		{
		gStackPaged = ETrue;
		}
	else
		{ 
		r = thread.ExitReason();
		if(r != KErrNone)
			return r;

		if (EExitKill != thread.ExitType())
			return KErrGeneral;
		gStackPaged = EFalse;
		}	
	thread.Close();
	if (!gStackPaged)
		{
		RDebug::Printf("    %08x present", aPtr);
		}
	else
		{
		RDebug::Printf("    %08x not present", aPtr);
		}
	return r;
	}

/**
Thread that just returns the data paging attributes of the thread.
*/
TInt ThreadFunc(TAny* aThreadInfo)
	{
	SThreadPagedInfo& info = *(SThreadPagedInfo*)aThreadInfo;
	RHeap& heap = User::Heap();
	RChunk chunk;
	chunk.SetHandle(((TestHybridHeap&) heap).ChunkHandle());
	info.iHeapPaged = chunk.IsPaged();
	gStackPtr = (TUint8*)&chunk;
	RDebug::Printf("&chunk %x",&chunk);
	gSem1.Signal();
	gSem2.Wait();
	info.iStackPaged = gStackPaged;
	return KErrNone;
	}

TInt DummyFunction(TAny*)
	{
	return KErrNone;
	}

TInt PanicThreadCreate(TAny* aCreateInfo)
	{
	RThread thread;
	TThreadCreateInfo createInfo((*(TThreadCreateInfo*) aCreateInfo));
	thread.Create(createInfo);
	return KErrGeneral; // Should never reach here
	}

//
// CheckHeapStackPaged
//
// Using the TThreadCreateInfo used to create the cheap, determine 
// whether the stack and the heap are paged or not
//
//
TInt CheckHeapStackPaged(TThreadCreateInfo& aCreateInfo, TInt aPaged, SThreadPagedInfo& aPagedInfo, TBool aUseProcessHeap = EFalse)
	{
	RThread thread;
	TBool paged;
	switch (aPaged)
		{
		case EUnspecified:
			test.Printf(_L("Testing gProcessPaged\n"));
			paged = gProcessPaged;
			break;

		case EPaged:
			test.Printf(_L("Testing Paged\n"));
			aCreateInfo.SetPaging(TThreadCreateInfo::EPaged);
			paged = ETrue;
			break;

		case EUnpaged:
			test.Printf(_L("Testing Unpaged\n"));
			aCreateInfo.SetPaging(TThreadCreateInfo::EUnpaged);
			paged = EFalse;
			break;
		}


	test_KErrNone(thread.Create(aCreateInfo));
	
	// Disable JIT debugging.
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);

	TRequestStatus status;
	thread.Logon(status); 
	
	thread.Resume();
	
	gSem1.Wait();		
	DPTest::FlushCache();
	TInt r = IsStackPaged(gStackPtr);
	test_KErrNone(r);
	gSem2.Signal();

	User::WaitForRequest(status);
	test (EExitKill == thread.ExitType());
	test(KErrNone == status.Int());
	
	test(KErrNone == thread.ExitReason());

	if (thread.ExitType() == EExitPanic)
		{
		test(thread.ExitCategory()==_L("USER"));
		}
			
	CLOSE_AND_WAIT(thread);

	// Put JIT debugging back to previous status.
	User::SetJustInTime(justInTime);

	UpdatePaged(paged);
	if (aUseProcessHeap)
		{// If using existing thread heap, heap will take the process paging status
		test_Equal(gProcessPaged, aPagedInfo.iHeapPaged);
		}
	else
		{
		test_Equal(paged, aPagedInfo.iHeapPaged);
		}
	test_Equal(paged, aPagedInfo.iStackPaged);
	return KErrNone;
	}

//
// TestThreadCreate
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_THREADHEAPCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		TThreadCreateInfo tests
//!							Verify the thread heap creation implementation
//! @SYMTestActions	
//! 1.	Call TThreadCreateInfo::TThreadCreateInfo() with valid parameters. 
//! 	Following this call RThread::Create()
//! 2.	Call TThreadCreateInfo::TThreadCreateInfo() with an invalid stack size. 
//!		Following this call RThread::Create()
//! 3.	Call TThreadCreateInfo::SetCreateHeap() with an invalid min heap size. 
//!		Following this call RThread::Create()
//! 4.	Call TThreadCreateInfo::SetCreateHeap() with an invalid max heap size. 
//!		Following this call RThread::Create()
//! 5.	Call TThreadCreateInfo::SetCreateHeap() with minHeapSize. > maxHeapSize 
//!		Following this call RThread::Create()
//! 6.	Call TThreadCreateInfo::SetUseHeap() specifying NULL. Following this call RThread::Create()
//! 7.	Call TThreadCreateInfo::SetOwner() with aOwner set to EOwnerProcess. 
//!		Following this call RThread::Create()
//! 8.	Call TThreadCreateInfo::SetOwner() with aOwner set to EOwnerThread. 
//!		Following this call RThread::Create()
//! 9.	Call TThreadCreateInfo::SetPaging() with aPaging set to unspecified. 
//!		Following this call RThread::Create() and check the paging status of the thread
//! 10.	Call TThreadCreateInfo::SetPaging() with aPaging set to EPaged. 
//!		Following this call RThread::Create() and check the paging status of the thread
//! 11.	Call TThreadCreateInfo::SetPaging() with aPaging set to EUnpaged. 
//!		Following this call RThread::Create() and check the paging status of the thread
//!
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestThreadCreate()
	{
	TInt r;
	test.Start(_L("Test RThread::Create() (New Heap)"));
		{
		RThread thread;
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize, NULL);
		createInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		r = thread.Create(createInfo);
		test_KErrNone(r);
		test_KErrNone(TestThreadExit(thread, EExitKill, KErrNone));
		thread.Close();
		}

	test.Next(_L("Test RThread::Create() - invalid stack size"));
		{
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, -1 , NULL);
		createInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		
		RThread threadPanic;
		test_KErrNone(threadPanic.Create(_L("Panic UserHeap"), PanicThreadCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));
		test_KErrNone(TestThreadExit(threadPanic, EExitPanic, EThrdStackSizeNegative));
		}

	test.Next(_L("Test RThread::Create() - invalid min heap size"));
		{
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize , NULL);
		createInfo.SetCreateHeap(-1, KMinHeapSize);
		
		RThread threadPanic;
		test_KErrNone(threadPanic.Create(_L("Panic UserHeap"), PanicThreadCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(threadPanic, EExitPanic, EThrdHeapMinTooSmall));
		}

	test.Next(_L("Test RThread::Create() - invalid max heap size"));
		{
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize , NULL);
		createInfo.SetCreateHeap(KMinHeapSize, -1);
		
		RThread threadPanic;
		test_KErrNone(threadPanic.Create(_L("Panic UserHeap"), PanicThreadCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(threadPanic, EExitPanic, EThrdHeapMaxLessThanMin));
		}

	test.Next(_L("Test RThread::Create() - min heap size > max heap size"));
		{
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize , NULL);
		createInfo.SetCreateHeap(KMinHeapSize << 1, KMinHeapSize);
		
		RThread threadPanic;
		test_KErrNone(threadPanic.Create(_L("Panic UserHeap"), PanicThreadCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(threadPanic, EExitPanic, EThrdHeapMaxLessThanMin));
		}

	test.Next(_L("Test TThreadCreateInfo::SetUseHeap() "));
		{
		RThread thread;
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize, NULL);
		createInfo.SetUseHeap(NULL);
		r = thread.Create(createInfo);
		test_KErrNone(r);
		test_KErrNone(TestThreadExit(thread, EExitKill, KErrNone));
		thread.Close();
		}

	test.Next(_L("Test TThreadCreateInfo::SetOwner(EOwnerProcess) "));
		{
		RThread thread;
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize, NULL);
		createInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		createInfo.SetOwner(EOwnerProcess);
		r = thread.Create(createInfo);
		test_KErrNone(r);
		test_KErrNone(TestThreadExit(thread, EExitKill, KErrNone));
		thread.Close();
		}


	test.Next(_L("Test TThreadCreateInfo::SetOwner(EOwnerThread) "));
		{
		RThread thread;
		TThreadCreateInfo createInfo(KGlobalThreadName, DummyFunction, KDefaultStackSize, NULL);
		createInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		createInfo.SetOwner(EOwnerThread);
		r = thread.Create(createInfo);
		test_KErrNone(r);
		test_KErrNone(TestThreadExit(thread, EExitKill, KErrNone));
		thread.Close();
		}



	gSem1.CreateLocal(0);
	gSem2.CreateLocal(0);
	test.Next(_L("Test Thread paging (New Heap)"));
		{		
		TBool aPaged = gProcessPaged;
		SThreadPagedInfo pagedInfo;
		test.Printf(_L("Testing gProcessPaged: aPaged = %x\n"), aPaged);
		TThreadCreateInfo createInfo(	KGlobalThreadName, ThreadFunc, KDefaultStackSize,
										(TAny*)&pagedInfo);
		createInfo.SetCreateHeap(KMinHeapSize, KMinHeapSize);
		
		test_KErrNone(CheckHeapStackPaged(createInfo, EUnspecified, pagedInfo));
		test_KErrNone(CheckHeapStackPaged(createInfo, EPaged, pagedInfo));
		test_KErrNone(CheckHeapStackPaged(createInfo, EUnpaged, pagedInfo));
		}


	test.Next(_L("Test RThread::Create() (Existing Heap)"));
		{
		SThreadPagedInfo pagedInfo;
		TThreadCreateInfo createInfo(	KGlobalThreadName, ThreadFunc, KDefaultStackSize, 
										(TAny*)&pagedInfo);
		createInfo.SetUseHeap(NULL);
		
		test_KErrNone(CheckHeapStackPaged(createInfo, EUnspecified, pagedInfo, ETrue));
		test_KErrNone(CheckHeapStackPaged(createInfo, EPaged, pagedInfo, ETrue));
		test_KErrNone(CheckHeapStackPaged(createInfo, EUnpaged, pagedInfo, ETrue));
		}
	test.End();
	}



TInt TestingTThreadCreate()
	{
	test.Printf(_L("Test TThreadCreateInfo\n"));
	TestThreadCreate();

	return 0;
	}
