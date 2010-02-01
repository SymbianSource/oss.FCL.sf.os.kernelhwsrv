// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\t_pagemove.cpp

//
//--------------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_PAGEMOVE-0572
//! @SYMTestType			UT
//! @SYMPREQ				PREQ308
//! @SYMTestCaseDesc		Test physical page moving
//!							t_pagemove loads and opens the logical device driver ("D_PAGEMOVE.LDD"). 
//!							Following this, it requests that the driver attempt to move  
//!							various kinds of pages directly. 
//!
//!							API Information:
//!								RBusLogicalChannel
//!
//!							Platforms/Drives/Compatibility:
//!								Hardware only. No defrag support on emulator. 
//!
//! @SYMTestActions			1  -  Move regular local data pages
//! 						2  -  Move regular global data pages
//! 						3  -  Move DLL writable static data pages
//! 						4  -  Move user self-modifying code chunk pages
//! 						5  -  Move RAM drive pages
//!							6  -  Move kernel heap pages (*********DISABLED************)
//! 						7  -  Move kernel stack pages
//! 						8  -  Move kernel code pages
//! 						9  -  Move regular code pages
//! 						10 -  Move code whilst the page is being modified
//! 						11 -  Move code (async) whilst the page is being modified
//! 						12 -  Move ROM locale DLL pages
//! 						13 -  Move RAM locale DLL pages
//! 						14 -  Moving pages whilst they are being virtually pinned and unpinned.
//! 						15 -  Moving pages whilst they are being physically pinned and unpinned.
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//--------------------------------------------------------------------------------------------------
//
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include <e32uid.h>
#include <e32hal.h>
#include <e32std.h>
#include <e32std_private.h>
#include <dptest.h>
#include "d_pagemove.h"
#include "t_pagemove_dll.h"
#include "t_pmwsd.h"
#include "..\mmu\mmudetect.h"
#include "..\debug\d_codemodifier.h"
#include "..\mmu\d_memorytest.h"

//#define _DEBUG_MSG
#ifdef _DEBUG_MSG
#define _R_PRINTF(x) 	RDebug::Printf(x)
#define _T_PRINTF(x)	test.Printf(x)
#else
#define _R_PRINTF(x)
#define _T_PRINTF(x)
#endif

LOCAL_D RTest test(_L("T_PAGEMOVE"));

_LIT(ELOCL_DEFAULT, "");
_LIT(ELOCLUS, "T_LOCLUS_RAM");
_LIT(ELOCLUS_ROM, "T_LOCLUS");
LOCAL_C TInt E32TestLocale(TInt);

RCodeModifierDevice Device;
extern TInt TestCodeModFunc();

extern TInt Increment(TInt);
extern TUint Increment_Length();
extern TInt Decrement(TInt);
extern TUint Decrement_Length();
typedef TInt (*PFI)(TInt);

LOCAL_C void StartCodeModifierDriver();
LOCAL_C void StopCodeModifierDriver();
LOCAL_C TInt TestCodeModification(RPageMove &);
LOCAL_C TInt TestCodeModificationAsync(RPageMove& pagemove);


const TPtrC KLddFileName=_L("D_PAGEMOVE.LDD");
TInt Repitions=4000;

TInt PageSize;
TUint NumberOfCpus;

volatile TBool ThreadDie;

TBool gDataPagingSupported;
TBool gRomPagingSupported;
TBool gCodePagingSupported;
TBool gPinningSupported;

// This executable is ram loaded (see mmp file) so this function will do fine
// as a test of RAM-loaded code.
TInt RamLoadedFunction()
	{
	return KArbitraryNumber;
	}

struct SPinThreadArgs
	{
	TLinAddr iLinAddr;
	TTestFunction iTestFunc;
	RThread iParentThread;
	User::TRealtimeState iRealtimeState;
	};


void StartThreads(	TUint aNumThreads, RThread* aThreads, TRequestStatus* aStatus, 
					TThreadFunction aThreadFunc, SPinThreadArgs& aThreadArgs)
	{
	for (TUint i = 0; i < aNumThreads; i++)
		{
		test_KErrNone(aThreads[i].Create(KNullDesC, aThreadFunc, KDefaultStackSize, NULL, &aThreadArgs));
		aThreads[i].Logon(aStatus[i]);
		TRequestStatus threadInitialised;
		aThreads[i].Rendezvous(threadInitialised);
		aThreads[i].Resume();
		_T_PRINTF(_L("wait for child\n"));
		User::WaitForRequest(threadInitialised);
		test_KErrNone(threadInitialised.Int());
		}
	}

void EndThreads(TUint aNumThreads, RThread* aThreads, TRequestStatus* aStatus)
	{
	for (TUint i = 0; i < aNumThreads; i++)
		{
		User::WaitForRequest(aStatus[i]);
		test_Equal(EExitKill, aThreads[i].ExitType());
		test_KErrNone(aThreads[i].ExitReason());
		aThreads[i].Close();
		}
	}


void Reschedule(TInt64& aSeed)
	{
	if (NumberOfCpus == 1)
		{
		TInt rand = Math::Rand(aSeed);
		if ((rand & 0x5) == 5)
			User::AfterHighRes(rand & 0x7);
		}
	}

TInt ReadWriteByte(TAny* aParam)
	{
	SPinThreadArgs* args = (SPinThreadArgs*)aParam;
	volatile TUint8* byte = (volatile TUint8*)args->iLinAddr;
	TInt64 seed = Math::Random()*Math::Random();

	test_KErrNone(User::SetRealtimeState(args->iRealtimeState));

	// Ensure the the parentThread has moved the page at least once
	// before we start accessing it.
	TRequestStatus status;
	args->iParentThread.Rendezvous(status);
	RThread::Rendezvous(KErrNone);
	_R_PRINTF("wait for parent");
	User::WaitForRequest(status);
	_R_PRINTF("acesssing page");

	FOREVER
		{
		*byte = *byte;
		Reschedule(seed);
		if (ThreadDie)
			break;
		}
	return KErrNone;
	}


TInt RunCodeThread(TAny* aParam)
	{
	TInt64 seed = Math::Random()*Math::Random();
	SPinThreadArgs* args = (SPinThreadArgs*)aParam;

	test_KErrNone(User::SetRealtimeState(args->iRealtimeState));

	// Ensure the the parentThread has moved the page at least once
	// before we start accessing it.
	TRequestStatus status;
	args->iParentThread.Rendezvous(status);
	RThread::Rendezvous(KErrNone);
	_R_PRINTF("wait for parent");
	User::WaitForRequest(status);
	_R_PRINTF("acesssing page");

	FOREVER
		{
		TInt r = args->iTestFunc();
		if (r != KArbitraryNumber)
			return KErrGeneral;
		Reschedule(seed);
		if (ThreadDie)
			break;
		}
	return KErrNone;
	}


TInt VirtualPinPage(TAny* aParam)
	{
	TInt64 seed = Math::Random()*Math::Random();
	SPinThreadArgs* args = (SPinThreadArgs*)aParam;
	RMemoryTestLdd ldd;
	test_KErrNone(ldd.Open());

	test_KErrNone(ldd.CreateVirtualPinObject());

	TBool firstRun = ETrue;
	FOREVER
		{
		// Pin the page of aParam.
		test_KErrNone(ldd.PinVirtualMemory(args->iLinAddr, PageSize));
		if (firstRun)
			{// On the first run ensure that the page is definitely pinned when
			// the parent thread first attempts to move it.
			TRequestStatus status;
			args->iParentThread.Rendezvous(status);
			RThread::Rendezvous(KErrNone);
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			firstRun = EFalse;
			}
		Reschedule(seed);
		test_KErrNone(ldd.UnpinVirtualMemory());
		if (ThreadDie)
			break;
		}
	test_KErrNone(ldd.DestroyVirtualPinObject());
	ldd.Close();
	return KErrNone;
	}


TInt PhysicalPinPage(TAny* aParam)
	{
	TInt64 seed = Math::Random()*Math::Random();
	SPinThreadArgs* args = (SPinThreadArgs*)aParam;

	RMemoryTestLdd ldd;
	test_KErrNone(ldd.Open());

	test_KErrNone(ldd.CreatePhysicalPinObject());

	TBool firstRun = ETrue;
	FOREVER
		{
		// Pin the page of aParam, use a read only pinning so that pinning code 
		// doesn't return KErrAccessDenied as writable mappings not allowed on code.
		test_KErrNone(ldd.PinPhysicalMemoryRO(args->iLinAddr, PageSize));
		if (firstRun)
			{// On the first run ensure that the page is definitely pinned when
			// the parent thread first attempts to move it.
			TRequestStatus status;
			args->iParentThread.Rendezvous(status);
			RThread::Rendezvous(KErrNone);
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			firstRun = EFalse;
			}
		Reschedule(seed);
		test_KErrNone(ldd.UnpinPhysicalMemory());
		if (ThreadDie)
			break;
		}
	test_KErrNone(ldd.DestroyPhysicalPinObject());
	ldd.Close();
	return KErrNone;
	}

TInt ModifyCodeThread(TAny* aParam)
	{
	SPinThreadArgs* args = (SPinThreadArgs*)aParam;
	TUint8* p = (TUint8*)args->iLinAddr;
	PFI func = (PFI)p;

	// Ensure the the parentThread has moved the page at least once
	// before we start accessing it.
	TRequestStatus status;
	args->iParentThread.Rendezvous(status);
	RThread::Rendezvous(KErrNone);
	_R_PRINTF("wait for parent");
	User::WaitForRequest(status);
	_R_PRINTF("modifiying page");

	while (!ThreadDie)
		{
		Mem::Copy(p, (TAny*)&Increment, Increment_Length());
		User::IMB_Range(p, p+Increment_Length());
		test_Equal(8, func(7));

		Mem::Copy(p, (TAny*)&Decrement, Decrement_Length());
		User::IMB_Range(p, p+Decrement_Length());
		test_Equal(6, func(7));
		}
	return KErrNone;
	}


enum TMovingPinStage
	{
	ENoPinning,
	EVirtualPinning,
	EPhysicalPinning,
	EMovingPinStages,
	};

void TestUserData(RPageMove& pagemove, TUint8* array, TInt size, TBool aPagedData=EFalse)
	{
	_T_PRINTF(_L("Fill the array with some data\n"));
	for (TInt i=0; i<size; i++) array[i] = i*i;

	TUint8* firstpage = (TUint8*)_ALIGN_DOWN((TLinAddr)array, PageSize);
	RThread thread;
	thread.Open(RThread().Id());
	SPinThreadArgs threadArgs;
	threadArgs.iLinAddr = (TLinAddr)array;
	threadArgs.iParentThread = thread;
	threadArgs.iRealtimeState = User::ERealtimeStateOff;

	TMovingPinStage endStage = EMovingPinStages;
	if (!gPinningSupported)
		endStage = EVirtualPinning;

	for (TUint state = ENoPinning; state < (TUint)endStage; state++)
		{
		TThreadFunction threadFunc = NULL;
		switch (state)
			{
			case ENoPinning:
				test.Printf(_L("Attempt to move pages while they are being modified\n"));
				threadFunc = &ReadWriteByte;
				break;
			case EVirtualPinning:
				test.Printf(_L("Attempt to move pages while they are being virtually pinned\n"));
				threadFunc = &VirtualPinPage;
				break;
			case EPhysicalPinning:
				test.Printf(_L("Attempt to move pages while they are being physically pinned\n"));
				threadFunc = &PhysicalPinPage;
				break;
			}
		ThreadDie = EFalse;
		TUint numThreads = (NumberOfCpus > 1) ? NumberOfCpus - 1 : 1;
		RThread* userDataThread = new RThread[numThreads];
		TRequestStatus* s = new TRequestStatus[numThreads];
		StartThreads(numThreads, userDataThread, s, threadFunc, threadArgs);

		_T_PRINTF(_L("Move first array page repeatedly\n"));
		TBool success=EFalse;
		TUint inuse = 0;
		*(volatile TUint8*)array = *array;	// Ensure the page of the first entry is paged in for the first move.
		for (TInt i=0; i < Repitions*2; i++)
			{
			TInt r = pagemove.TryMovingUserPage(firstpage, ETrue);
			if (i == 0)
				{// If this is the first run allow the pinning threads to 
				// unpin the memory now that we've definitely done at least 
				// one page move with the page pinned.
				_T_PRINTF(_L("signal to child\n"));
				RThread::Rendezvous(KErrNone);
				}
			switch (r)
				{
				case KErrInUse:
					inuse++;
					break;
				case KErrArgument:
					// The page was paged out, this should only happen for paged data.
					test(aPagedData);
					break;
				default:
					test_KErrNone(r);
					success=ETrue;
					break;
				}
			}
		// Can't guarantee that for paged data the page and its page tables will
		// be paged in, in most cases it will be at least once.
		// Pinning the page should always return KErrInUse except for virtually 
		// pinned non-paged memory as virtual pinning is a nop for unpaged memory.
		test.Printf(_L("inuse test removed; inuse %d\n"),inuse);
		//test(inuse || aPagedData || state == EVirtualPinning);
		test(success || state == EPhysicalPinning);

		ThreadDie = ETrue;
		EndThreads(numThreads, userDataThread, s);

		_T_PRINTF(_L("Validate page data\n"));
		for (TInt i=0; i<size; i++)
			test_Equal((TUint8)(i*i), array[i]);
		}
	thread.Close();
	}


void TestMovingCode(RPageMove& aPagemove, TTestFunction aFunc, TBool aPaged=EFalse)
	{
	TUint8* firstpage = (TUint8*)_ALIGN_DOWN((TLinAddr)aFunc, PageSize);
	RThread thread;
	thread.Open(RThread().Id());
	SPinThreadArgs threadArgs;
	threadArgs.iLinAddr = (TLinAddr)firstpage;
	threadArgs.iTestFunc = aFunc;
	threadArgs.iParentThread = thread;
	threadArgs.iRealtimeState = User::ERealtimeStateOff;

	TMovingPinStage endStage = EMovingPinStages;
	if (!gPinningSupported)
		endStage = EVirtualPinning;

	for (TUint state = ENoPinning; state < (TUint)endStage; state++)
		{
		TThreadFunction threadFunc = NULL;
		switch (state)
			{
			case ENoPinning:
				test.Printf(_L("Attempt to move pages while they are being executed\n"));
				threadFunc = &RunCodeThread;
				test_Equal(KArbitraryNumber, aFunc()); // Ensure the page is paged in.
				break;
			case EVirtualPinning:
				test.Printf(_L("Attempt to move pages while they are being virtually pinned\n"));
				threadFunc = &VirtualPinPage;
				break;
			case EPhysicalPinning:
				test.Printf(_L("Attempt to move pages while they are being physically pinned\n"));
				threadFunc = &PhysicalPinPage;
				break;
			}
		ThreadDie = EFalse;
		TUint numThreads = (NumberOfCpus > 1) ? NumberOfCpus - 1 : 1;
		RThread* codeRunThread = new RThread[numThreads];
		TRequestStatus* s = new TRequestStatus[numThreads];
		StartThreads(numThreads, codeRunThread, s, threadFunc, threadArgs);

		_T_PRINTF(_L("Move first code page repeatedly\n"));
		test_Equal(KArbitraryNumber, aFunc());	
		TBool inuse=EFalse, success=EFalse;
		for (TInt i=0; i < Repitions; i++)
			{
			TInt r = aPagemove.TryMovingUserPage(firstpage, ETrue);
			if (i == 0)
				{// If this is the first run allow the pinning threads to 
				// unpin the memory now that we've definitely done at least 
				// one page move with the page pinned.
				_T_PRINTF(_L("signal to child\n"));
				RThread::Rendezvous(KErrNone);
				}
			switch (r)
				{
				case KErrInUse:
					inuse=ETrue;
					break;
				case KErrArgument:
					// The page was paged out, this should only happen for paged code.
					test(aPaged);
					break;
				default:
					test_KErrNone(r);
					success=ETrue;
					break;
				}
			}
		// Physical pinning or adding a new pinning while a page is being moved
		// should prevent code pages being moved.
		switch (state)
		{
			case ENoPinning :			
				test(!inuse || aPaged);	// Stealing may get KErrInUse but this should only happen for paged code.
			case EVirtualPinning :
				test(success);
				break;
			case EPhysicalPinning :
				break;
		}

		ThreadDie = ETrue;
		EndThreads(numThreads, codeRunThread, s);

		_T_PRINTF(_L("Validate page data\n"));
		test_Equal(KArbitraryNumber, aFunc());		
		}
	thread.Close();
	}


void TestMovingRealtime(RPageMove& aPagemove, TUint8* aArray, TInt aSize, TTestFunction aFunc, TBool aCode, TBool aPaged=EFalse)
	{
	TThreadFunction threadFunc;
	TLinAddr pageAddr;
	RThread thread;
	TUint8* firstpage;
	thread.Open(RThread().Id());
	SPinThreadArgs threadArgs;
	threadArgs.iParentThread = thread;
	if (aCode)
		{
		pageAddr = (TLinAddr)aFunc;
		firstpage = (TUint8*)_ALIGN_DOWN(pageAddr, PageSize);
		threadArgs.iLinAddr = (TLinAddr)firstpage;
		threadFunc = RunCodeThread;
		threadArgs.iTestFunc = aFunc;
		test_Equal(KArbitraryNumber, aFunc());
		}
	else
		{
		pageAddr = (TLinAddr)aArray;
		firstpage = (TUint8*)_ALIGN_DOWN(pageAddr, PageSize);
		threadArgs.iLinAddr = (TLinAddr)aArray;
		threadFunc = ReadWriteByte;
		_T_PRINTF(_L("Fill the array with some data\n"));
		for (TInt i=0; i<aSize; i++) aArray[i] = i*i;
		}

	RMemoryTestLdd ldd;

	TMovingPinStage endStage = EMovingPinStages;
	if (gPinningSupported)
		{
		test_KErrNone(ldd.Open());
		test_KErrNone(ldd.CreateVirtualPinObject());
		test_KErrNone(ldd.CreatePhysicalPinObject());
		}
	else
		endStage = EVirtualPinning;

	for (TUint state = ENoPinning; state < (TUint)endStage; state++)
		{
		switch (state)
			{
			case ENoPinning:
				test.Printf(_L("Attempt to move pages while they are being accessed\n"));
				break;
			case EVirtualPinning:
				test.Printf(_L("Attempt to move pages while they are virtually pinned\n"));
				test_KErrNone(ldd.PinVirtualMemory((TLinAddr)firstpage, PageSize));

				break;
			case EPhysicalPinning:
				test.Printf(_L("Attempt to move pages while they are physically pinned\n"));
				test_KErrNone(ldd.PinPhysicalMemoryRO((TLinAddr)firstpage, PageSize));
				break;
			}
		for (	TUint realtimeState = User::ERealtimeStateOff; 
				realtimeState <= User::ERealtimeStateWarn; 
				realtimeState++)
			{
			ThreadDie = EFalse;
			RThread accessThread;
			TRequestStatus s;
			threadArgs.iRealtimeState = (User::TRealtimeState)realtimeState;
			test_KErrNone(accessThread.Create(_L("Realtime Thread"), threadFunc, KDefaultStackSize, NULL, &threadArgs));
			accessThread.Logon(s);
			TRequestStatus threadInitialised;
			accessThread.Rendezvous(threadInitialised);
			accessThread.Resume();

			_T_PRINTF(_L("wait for child\n"));
			User::WaitForRequest(threadInitialised);
			test_KErrNone(threadInitialised.Int());

			_T_PRINTF(_L("Move page repeatedly\n"));
			TBool success=EFalse, pagedOut=EFalse;
			TUint inuse=0;
			if (aCode)
				{
				test_Equal(KArbitraryNumber, aFunc());
				}
			else
				{
				*(volatile TUint8*)aArray = *aArray;
				}

			for (TInt i=0; i < Repitions; i++)
				{
				TInt r = aPagemove.TryMovingUserPage(firstpage, ETrue);
				if (i == 0)
					{
					_T_PRINTF(_L("signal to child\n"));
					RThread::Rendezvous(KErrNone);
					}
				switch (r)
					{
					case KErrInUse:
						inuse++;
						break;
					case KErrArgument:
						// The page was paged out, this should only happen for paged code.
						test(aPaged);
						pagedOut = ETrue;
						break;
					default:
						test_KErrNone(r);
						success=ETrue;
						break;
					}
				}
			ThreadDie = ETrue;
			User::WaitForRequest(s);
			test.Printf(_L("inuse %d\n"),inuse);
			switch (state)
				{
				case ENoPinning :
					test(success);
					if (EExitPanic == accessThread.ExitType())
						{
						test(accessThread.ExitCategory()==_L("KERN-EXEC"));
						test_Equal(EIllegalFunctionForRealtimeThread, accessThread.ExitReason());
						test(aPaged && realtimeState == User::ERealtimeStateOn);
						}
					else
						{
						test_Equal(EExitKill,accessThread.ExitType());
						test_KErrNone(accessThread.ExitReason());
						}
					// Ensure the page is paged in before we attempt to move it again with a different realtime state.
					if (aCode)
						{
						test_Equal(KArbitraryNumber, aFunc());
						}
					else
						{
						*(volatile TUint8*)aArray = *aArray;
						}
					break;				
				case EVirtualPinning :
					test(!aCode || !inuse);
					test(success);
					test(!pagedOut);
					test_Equal(EExitKill,accessThread.ExitType());
					test_KErrNone(accessThread.ExitReason());
					break;
				case EPhysicalPinning :
					test(!success);
					break;
				}
			accessThread.Close();
			}
		if (gPinningSupported)
			{
			// Unpin any pinned memory.
			test_KErrNone(ldd.UnpinVirtualMemory());
			test_KErrNone(ldd.UnpinPhysicalMemory());
			}

		_T_PRINTF(_L("Validate page data\n"));
		if (aCode)
			{
			test_Equal(KArbitraryNumber, aFunc());
			}
		else
			{
			for (TInt i=0; i<aSize; i++)
				test_Equal((TUint8)(i*i), aArray[i]);
			}
			
		}
	if (gPinningSupported)
		{
		test_KErrNone(ldd.DestroyVirtualPinObject());
		test_KErrNone(ldd.DestroyPhysicalPinObject());
		ldd.Close();
		}
	thread.Close();
	}

// Only commits and decommits the first page as that is the only page that is being moved.
// Plus this ensures the page table and page directories of the chunk are always allocated
// and therefore prevents Epoc::LinearToPhysical() from crashing the system.
TInt CommitDecommit(TAny* aParam)
	{
	RChunk* chunk = (RChunk*) aParam;
	volatile TUint8* byte = chunk->Base();
	FOREVER
		{
		*byte = *byte;
		User::AfterHighRes(0);
		TInt r = chunk->Decommit(0, PageSize);
		if (r != KErrNone)
			return r;
		User::AfterHighRes(0);
		r = chunk->Commit(0, PageSize);
		if (r != KErrNone)
			return r;
		}
	}

void TestCommitDecommit(RPageMove& pagemove, RChunk& aChunk)
	{
	test.Printf(_L("Attempt to move a page while it is being committed and decommited\n"));
	RThread thread;
	TRequestStatus s;
	test_KErrNone(thread.Create(_L("CommitDecommit"), &CommitDecommit, KDefaultStackSize, NULL, (TAny*)&aChunk));
	thread.Logon(s);
	thread.SetPriority(EPriorityMore);
	thread.Resume();

	TUint8* firstpage=(TUint8*)_ALIGN_DOWN((TLinAddr)aChunk.Base(), PageSize);
	for (TInt i=0; i < Repitions; i++)
		{
		TInt r = pagemove.TryMovingUserPage(firstpage, ETrue);
		// Allow all valid return codes as we are only testing that this doesn't 
		// crash the kernel and the page could be commited, paged out or decommited
		// at any one time.
		test_Value(r, r <= KErrNone);
		}

	thread.Kill(KErrNone);
	User::WaitForRequest(s);
	test_Equal(EExitKill,thread.ExitType());
	test_KErrNone(thread.ExitReason());
	thread.Close();
	}


void TestPageTableDiscard(RPageMove& pagemove, TUint8* array, TUint size)
	{
	_T_PRINTF(_L("Fill the array with some data\n"));
	for (TUint i=0; i<size; i++) array[i] = i*i;

	TUint8* firstpage = (TUint8*)_ALIGN_DOWN((TLinAddr)array, PageSize);
	RThread thread;
	thread.Open(RThread().Id());
	SPinThreadArgs threadArgs;
	threadArgs.iLinAddr = (TLinAddr)array;
	threadArgs.iParentThread = thread;
	threadArgs.iRealtimeState = User::ERealtimeStateOff;

	TMovingPinStage endStage = EMovingPinStages;
	if (!gPinningSupported)
		endStage = EVirtualPinning;
	
	for (TUint pageTableInfo = 0; pageTableInfo < 2; pageTableInfo++)
		{
		for (TUint state = ENoPinning; state < (TUint)endStage; state++)
			{
			TThreadFunction threadFunc = NULL;
			if (!pageTableInfo)
			{
			switch (state)
				{
				case ENoPinning:
					test.Printf(_L("Attempt to move page tables whilst the pages they map are being modified\n"));
					threadFunc = &ReadWriteByte;
					break;
				case EVirtualPinning:
					test.Printf(_L("Attempt to move page tables whilst the pages they map are being virtually pinned\n"));
					threadFunc = &VirtualPinPage;
					break;
				case EPhysicalPinning:
					test.Printf(_L("Attempt to move page tables whilst the pages they map are being physically pinned\n"));
					threadFunc = &PhysicalPinPage;
					break;
				}
			}
			else
			{
			switch (state)
				{
				case ENoPinning:
					test.Printf(_L("Attempt to move page table infos whilst pages they refer to are being modified\n"));
					threadFunc = &ReadWriteByte;
					break;
				case EVirtualPinning:
					test.Printf(_L("Attempt to move page table infos whilst pages they refer to are being virtually pinned\n"));
					threadFunc = &VirtualPinPage;
					break;
				case EPhysicalPinning:
					test.Printf(_L("Attempt to move page table infos whilst pages they refer to are being physically pinned\n"));
					threadFunc = &PhysicalPinPage;
					break;
				}
			}
			ThreadDie = EFalse;
			TUint numThreads = (NumberOfCpus > 1) ? NumberOfCpus - 1 : 1;
			RThread* threads = new RThread[numThreads];
			TRequestStatus* s = new TRequestStatus[numThreads];
			StartThreads(numThreads, threads, s, threadFunc, threadArgs);

			_T_PRINTF(_L("Move first array page repeatedly\n"));
			TUint inuse = 0;
			for (TInt i=0; i < Repitions; i++)
				{
				TInt r;
				if (!pageTableInfo)
					r = pagemove.TryMovingPageTable(firstpage);
				else
					r = pagemove.TryMovingPageTableInfo(firstpage);					
				if (i == 0)
					{// If this is the first run allow the pinning threads to 
					// unpin the memory now that we've definitely done at least 
					// one page move with the page pinned.
					_T_PRINTF(_L("signal to child\n"));
					RThread::Rendezvous(KErrNone);
					}
				switch (r)
					{
					case KErrInUse:
						inuse++;
						break;
					case KErrNotFound:
						// The page table or page table info page was paged out.
						break;
					default:
						test_KErrNone(r);
						break;
					}
				}
			test.Printf(_L("inuse %d\n"),inuse);
			// A virtually pinned page should always return KErrInUse at least once.
			test(state != EVirtualPinning || inuse);

			ThreadDie = ETrue;
			EndThreads(numThreads, threads, s);

			_T_PRINTF(_L("Validate page data\n"));
			for (TUint i=0; i<size; i++)
				test_Equal((TUint8)(i*i), array[i]);
			}
		}
	thread.Close();
	}

// Basic testing of moving rom pages.
void TestMovingRom(RPageMove& aPageMove)
	{
	TUint8* pPage=(TUint8*)User::Alloc(PageSize);
	test(pPage!=NULL);

	TUint romHdr = UserSvr::RomHeaderAddress();

	if (gPinningSupported)
		{
		// Pin an unpaged rom page to get the physical address of the rom page.
		// Pinning unpaged rom actually does nothing except return the physical 
		// address of the page.
		RMemoryTestLdd ldd;
		test_KErrNone(ldd.Open());
		test_KErrNone(ldd.CreatePhysicalPinObject());

		// Save contents of rom page.
		Mem::Move(pPage,(TAny*)romHdr,PageSize);

		test_KErrNone(ldd.PinPhysicalMemoryRO(romHdr, PageSize));
		test_KErrNone(ldd.UnpinPhysicalMemory());

		// Now move the page, d_memorytest saves the address of the pinned page
		// depsite it being unpinned.
		// Will get KErrArgument as rom pages don't have an SPageInfo so memory
		// model doesn't treat rom as though they are in ram, which in most cases 
		// they are.
		test_Equal(KErrArgument, ldd.MovePinnedPhysicalMemory(0));

		test_KErrNone(Mem::Compare((TUint8*)romHdr,PageSize,pPage,PageSize));
		test_KErrNone(ldd.DestroyPhysicalPinObject());
		ldd.Close();
		}

	if (gRomPagingSupported)
		{
		// Use paged part of rom for testing
		TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
		test(romHeader->iPageableRomStart);
		TUint romAddr = (TUint)((TUint8*)romHeader + romHeader->iPageableRomStart + 64 * PageSize);

		// We will use the 64th pagable rom page so check that it exists.
		test(romHeader->iPageableRomSize >= 65 * PageSize);
	
		// Page in the rom page and save it contents.
		Mem::Move(pPage,(TAny*)romAddr,PageSize);
		// This will actually discard the page not move it.
		test_KErrNone(aPageMove.TryMovingUserPage(pPage));

		test_KErrNone(Mem::Compare((TUint8*)romAddr,PageSize,pPage,PageSize));
		}
	}


void TestMovingCodeChunk(RPageMove& pagemove, RChunk aChunk, TBool aPagedData)
	{
	TUint8* p = aChunk.Base();

	TUint8* firstpage = (TUint8*)_ALIGN_DOWN((TLinAddr)p, PageSize);
	RThread thread;
	thread.Open(RThread().Id());
	SPinThreadArgs threadArgs;
	threadArgs.iLinAddr = (TLinAddr)p;
	threadArgs.iParentThread = thread;

	test.Printf(_L("Attempt to move pages while they are being executed and modified\n"));
	ThreadDie = EFalse;
	RThread modCodeThread;
	TRequestStatus s;
	test_KErrNone(modCodeThread.Create(_L("User Data thread"), &ModifyCodeThread, KDefaultStackSize, NULL, &threadArgs));
	modCodeThread.Logon(s);
	TRequestStatus threadInitialised;
	modCodeThread.Rendezvous(threadInitialised);
	modCodeThread.Resume();

	_T_PRINTF(_L("wait for child\n"));
	User::WaitForRequest(threadInitialised);
	test_KErrNone(threadInitialised.Int());

	_T_PRINTF(_L("Move code chunk page repeatedly\n"));
	TBool success=EFalse;
	*(volatile TUint8*)p = *p; // Ensure the page of the first entry is paged in for the first move.
	for (TInt i=0; i < Repitions; i++)
		{
		TInt r = pagemove.TryMovingUserPage(firstpage, ETrue);
		if (i == 0)
			{// If this is the first run allow the modifying thread to run now 
			// we've done one move.
			_T_PRINTF(_L("signal to child\n"));
			RThread::Rendezvous(KErrNone);
			}
		switch (r)
			{
			case KErrInUse:
				break;
			case KErrArgument:
				// The page was paged out, this should only happen for paged data.
				test(aPagedData);
				break;
			default:
				test_KErrNone(r);
				success=ETrue;
				break;
			}
		}
	test(success);

	ThreadDie = ETrue;
	User::WaitForRequest(s);
	test_Equal(EExitKill,modCodeThread.ExitType());
	test_KErrNone(modCodeThread.ExitReason());
	modCodeThread.Close();

	thread.Close();
	}

GLDEF_C TInt E32Main()
    {
	test.Title();
	if (!HaveMMU())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test_KErrNone(UserHal::PageSizeInBytes(PageSize));

	// Determine which types of paging are supported
	TUint32 attrs = DPTest::Attributes();
	gRomPagingSupported = (attrs & DPTest::ERomPaging) != 0;
	gCodePagingSupported = (attrs & DPTest::ECodePaging) != 0;
	gDataPagingSupported = (attrs & DPTest::EDataPaging) != 0;

	// Does this memory model support pinning.
	TInt mm = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	gPinningSupported = mm >= EMemModelTypeFlexible;

	RPageMove pagemove;
	test.Next(_L("Open test LDD"));
	test_KErrNone(pagemove.Open());

	// Determine whether this is a smp device.
	NumberOfCpus = pagemove.NumberOfCpus();
	if (NumberOfCpus > 1)
		Repitions = 1000;	// SMP system therefore likely to get KErrInUse in less repitions.

	test.Next(_L("Attempting to move regular local data pages"));
		{
		const TInt size=16384;
		TUint8* array = new TUint8[size];
		test_NotNull(array);

		TestUserData(pagemove, array, size);

		_T_PRINTF(_L("Walk heap\n"));
		User::Check();

		delete [] array;
		}

	test.Next(_L("Attempting to move regular global coarse data pages"));
		{
		const TInt size=1<<20;	// Make this chunk multiple of 1MB so it is a coarse memory object on FMM
		RChunk chunk;
		test_KErrNone(chunk.CreateDisconnectedGlobal(_L("Dave"), 0, size, size));
		TUint8* array = chunk.Base();

		TestUserData(pagemove, array, size);
		TestMovingRealtime(pagemove, array, size, NULL, EFalse);
		TestCommitDecommit(pagemove, chunk);

		chunk.Close();
		}

	if (gDataPagingSupported)
		{
		test.Next(_L("Attempting to move demand paged fine local user data pages"));
		const TInt size=16384;
		TChunkCreateInfo createInfo;
		createInfo.SetDisconnected(0, size, size);
		createInfo.SetPaging(TChunkCreateInfo::EPaged);
		RChunk chunk;
		test_KErrNone(chunk.Create(createInfo));
		TUint8* array = chunk.Base();

		TestUserData(pagemove, array, size, ETrue);
		TestMovingRealtime(pagemove, array, size, NULL, EFalse, ETrue);
		TestPageTableDiscard(pagemove, array, size);
		TestCommitDecommit(pagemove, chunk);
		chunk.Close();

		test.Next(_L("Attempting to move demand paged coarse global user data pages"));
		const TInt sizeCoarse = 1 << 20; // Make this chunk multiple of 1MB so it is a coarse memory object on FMM
		TChunkCreateInfo createInfoCoarse;
		createInfoCoarse.SetDisconnected(0, sizeCoarse, sizeCoarse);
		createInfoCoarse.SetGlobal(_L("Dave"));
		createInfoCoarse.SetPaging(TChunkCreateInfo::EPaged);
		RChunk chunkCoarse;
		test_KErrNone(chunkCoarse.Create(createInfoCoarse));
		array = chunkCoarse.Base();

		TestUserData(pagemove, array, sizeCoarse, ETrue);
		TestMovingRealtime(pagemove, array, sizeCoarse, NULL, EFalse, ETrue);
		TestPageTableDiscard(pagemove, array, sizeCoarse);
		TestCommitDecommit(pagemove, chunkCoarse);
		chunkCoarse.Close();
		}

	test.Next(_L("Attempting to move DLL writable static data pages"));
		{
		const TInt size=16384;
		TUint8* array = DllWsd::Address();

		TestUserData(pagemove, array, size);
		}

	test.Next(_L("Attempting to move user self-mod code chunk page when IMB'ing and executing"));
	RChunk codeChunk;
	test_KErrNone(codeChunk.CreateLocalCode(PageSize,PageSize));
	TestMovingCodeChunk(pagemove, codeChunk, EFalse);
	codeChunk.Close();

	if (gDataPagingSupported)
		{
		test.Next(_L("Attempting to move paged user self-mod code chunk page when IMB'ing and executing"));
		TChunkCreateInfo createInfo;
		createInfo.SetCode(PageSize, PageSize);
		createInfo.SetPaging(TChunkCreateInfo::EPaged);

		RChunk pagedCodeChunk;
		test_KErrNone(pagedCodeChunk.Create(createInfo));
		TestMovingCodeChunk(pagemove, pagedCodeChunk, ETrue);
		pagedCodeChunk.Close();
		}

	test.Next(_L("Attempting to move RAM drive"));
	if ((MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeMultiple)
		{
		for (TInt i=0; i<Repitions; i++)
			test_KErrNone(pagemove.TryMovingUserPage((TAny*)0xA0000000));
		}
	else if ((MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeMoving)
		{
		for (TInt i=0; i<Repitions; i++)
			test_KErrNone(pagemove.TryMovingUserPage((TAny*)0x40000000));
		}
	else if ((MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeFlexible)
		{
		// do nothing, RAM drive is not special
		}
	else
		{
		test.Printf(_L("Don't know where the RAM drive is!"));
		test(0);
		}
	
#if 0
	test.Next(_L("Attempting to move kernel heap pages"));
	for (TInt i=0; i<Repitions; i++)
		test_KErrNone(pagemove.TryMovingKHeap());
#endif

	if ((MemModelAttributes()&EMemModelTypeMask) != EMemModelTypeFlexible)
		{// Only the moving and multiple memory models move kernel stack pages.
		test.Next(_L("Attempting to move kernel stack pages"));
		for (TInt i=0; i<Repitions; i++)
			test_KErrNone(pagemove.TryMovingKStack());
		}

	test.Next(_L("Attempting to move ROM pages"));
	TestMovingRom(pagemove);

	test.Next(_L("Attempting to move kernel code pages"));
	for (TInt i=0; i<Repitions; i++)
		test_KErrNone(pagemove.TryMovingKCode());

	test.Next(_L("Attempting to move regular code pages"));
	TestMovingCode(pagemove, RamLoadedFunction);
	TestMovingRealtime(pagemove, NULL, 0, RamLoadedFunction, ETrue, EFalse);

	if (gCodePagingSupported)
		{
		test.Next(_L("Attempting to move demand paged code pages"));
		TestMovingCode(pagemove, DllTestFunction, ETrue);
		TestMovingRealtime(pagemove, NULL, 0, DllTestFunction, ETrue, ETrue);
		}

	/* Setup CodeModifier Test Driver */
	StartCodeModifierDriver();
	test(KErrNone==Device.InitialiseCodeModifier(/* Max break points */ 5 ));

	test.Next(_L("Attempting to move code page being modified\n"));
	test_KErrNone(TestCodeModification(pagemove));

	test.Next(_L("Attempting to move code (async) while page being modified"));
	test_KErrNone(TestCodeModificationAsync(pagemove));

	StopCodeModifierDriver();
	
	test.Next(_L("Attempting to move ROM Locale DLL Page"));
	test_KErrNone(E32TestLocale(1));

	test.Next(_L("Attempting to move RAM Locale DLL Page"));
	test_KErrNone(E32TestLocale(0));

	test.Next(_L("Close test LDD"));
	pagemove.Close();
	User::FreeLogicalDevice(KLddFileName);

	test.End();
	return(KErrNone);
    }


void testUS(const TLocale& aLocale)
{
	test.Printf(_L("Test US\n"));

	test(aLocale.CountryCode()==1);
	test(aLocale.DateFormat()==EDateAmerican);
	test(aLocale.TimeFormat()==ETime12);
	test(aLocale.CurrencySymbolPosition()==ELocaleBefore);
	test(aLocale.CurrencySpaceBetween()==FALSE);
	test(aLocale.CurrencyDecimalPlaces()==2);
	test(aLocale.CurrencyNegativeInBrackets()==EFalse);
	test(aLocale.CurrencyTriadsAllowed()==TRUE);
	test(aLocale.ThousandsSeparator()==',');
	test(aLocale.DecimalSeparator()=='.');
	test(aLocale.DateSeparator(0)==0);
	test(aLocale.DateSeparator(1)=='/');
	test(aLocale.DateSeparator(2)=='/');
	test(aLocale.DateSeparator(3)==0);
	test(aLocale.TimeSeparator(0)==0);
	test(aLocale.TimeSeparator(1)==':');
	test(aLocale.TimeSeparator(2)==':');
	test(aLocale.TimeSeparator(3)==0);
	test(aLocale.AmPmSymbolPosition()==TRUE);
	test(aLocale.AmPmSpaceBetween()==TRUE);
	test(aLocale.HomeDaylightSavingZone()==EDstNorthern);
	test(aLocale.WorkDays()==0x1f);
	test(aLocale.StartOfWeek()==ESunday);
	test(aLocale.ClockFormat()==EClockAnalog);
	test(aLocale.UnitsGeneral()==EUnitsImperial);
	test(aLocale.UnitsDistanceShort()==EUnitsImperial);
	test(aLocale.UnitsDistanceLong()==EUnitsImperial);
}


void testUK(const TLocale& aLocale)
{
//#ifdef __WINS__
	test(aLocale.CountryCode()==44);
	test(aLocale.DateFormat()==EDateEuropean);
	test(aLocale.TimeFormat()==ETime12);
	test(aLocale.CurrencySymbolPosition()==ELocaleBefore);
	test(aLocale.CurrencySpaceBetween()==FALSE);
	test(aLocale.CurrencyDecimalPlaces()==2);
	test(aLocale.CurrencyNegativeInBrackets()==EFalse);
	test(aLocale.CurrencyTriadsAllowed()==TRUE);
	test(aLocale.ThousandsSeparator()==',');
	test(aLocale.DecimalSeparator()=='.');
	test(aLocale.DateSeparator(0)==0);
	test(aLocale.DateSeparator(1)=='/');
	test(aLocale.DateSeparator(2)=='/');
	test(aLocale.DateSeparator(3)==0);
	test(aLocale.TimeSeparator(0)==0);
	test(aLocale.TimeSeparator(1)==':');
	test(aLocale.TimeSeparator(2)==':');
	test(aLocale.TimeSeparator(3)==0);
	test(aLocale.AmPmSymbolPosition()==TRUE);
	test(aLocale.AmPmSpaceBetween()==TRUE);
	test(aLocale.HomeDaylightSavingZone()==EDstEuropean);
	test(aLocale.WorkDays()==0x1f);
	test(aLocale.StartOfWeek()==EMonday);
	test(aLocale.ClockFormat()==EClockAnalog);
	test(aLocale.UnitsGeneral()==EUnitsImperial);
	test(aLocale.UnitsDistanceShort()==EUnitsImperial);
	test(aLocale.UnitsDistanceLong()==EUnitsImperial);
//#endif
}


void testChangeLocale(TInt isrom)
{
	TLocale locale;
	
#ifdef __WINS__
//We get a power-change notification 1 second after switch-on
//So we wait for a second on WINS.
//Should we fix this bug??
	User::After(1000000);
#endif
	RChangeNotifier notifier;
	TInt res=notifier.Create();
	test(res==KErrNone);
	TRequestStatus stat;
	res=notifier.Logon(stat);
	test(res==KErrNone);
	//initial pattern of stat is already tested by t_chnot

	res=notifier.Logon(stat);
	test(res==KErrNone);
	test(stat==KRequestPending);
	if (isrom == 0) 
		{
		test.Printf(_L("Change to RAM US Locale\n")); 	
		res=UserSvr::ChangeLocale(ELOCLUS);
		}
	else
		{
		test.Printf(_L("Change to ROM US Locale\n")); 	
		res=UserSvr::ChangeLocale(ELOCLUS_ROM);
		}
	test.Printf(_L("res=%d\n"),res);
	test(res==KErrNone);
	test(stat.Int() & EChangesLocale);
	res=notifier.Logon(stat);
	test(res==KErrNone);
	test(stat==KRequestPending);
	
	locale.Refresh();
	testUS(locale);
}


LOCAL_C void LocaleLanguageGet(SLocaleLanguage& locale)
{
	TPckg<SLocaleLanguage> localeLanguageBuf(locale);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleLanguageKey, localeLanguageBuf);
	test(r == KErrNone || r == KErrNotFound);
}

LOCAL_C TInt E32TestLocale(TInt isrom)
{
	TInt r;
	TAny *LocaleAddr;
	TLocale locale;
	
	/* Setup the US Locale DLL and ensure the Locale got modified (testUS) */
	testChangeLocale(isrom);
 
	/* Now get a pointer to some data in the DLL. This will be used to move a
	** page from the dll 
	*/
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	LocaleAddr = (TAny *) localeLanguage.iDateSuffixTable;
	test(LocaleAddr != NULL);

	RPageMove pagemove;
	r=pagemove.Open();
	test_KErrNone(r);

	r=pagemove.TryMovingLocaleDll(LocaleAddr);
	
	if (isrom == 0) 
		{
		test_KErrNone(r);
		}
	else
		{
		// When the locale is in rom it is in the unpaged part of rom and 
		// Epoc::LinearToPhysical() won't be able to find the address.
		test_Equal(KErrArgument, r)
		}

	test.Printf(_L("Locale Test: Page move done\n"));

	/* Test US again. The kernel should have cached the locale informaton, so this will not
	 * really be testing the pagmove.
	 */
	locale.Refresh();
	testUS(locale);
	
	/* Reload the Default Locale */
	test.Printf(_L("Locale Test: Change to UK Default\n"));
	r=UserSvr::ChangeLocale(ELOCL_DEFAULT);	
	test(r==KErrNone);
	locale.Refresh();
	testUK(locale);	

	/* This will ACTUALLY test the page which was moved by making the kernel reload the Locale
	 * information from the DLL. 
	 */
	if (isrom == 0) 
		{
		test.Printf(_L("RAM Locale Test: Change to US Again\n"));
		r=UserSvr::ChangeLocale(ELOCLUS);	
		}
	else
		{
		test.Printf(_L("ROM Locale Test: Change to US Again\n"));
		r=UserSvr::ChangeLocale(ELOCLUS_ROM);	
		}


	test(r==KErrNone);
	locale.Refresh();
	testUS(locale);

	/* Reset the Locale to the default */
	r=UserSvr::ChangeLocale(ELOCL_DEFAULT);	
	test(r==KErrNone);
	locale.Refresh();
	testUK(locale);	
	return(KErrNone);
}

LOCAL_C void StartCodeModifierDriver()
	{
	test.Printf(_L("Start CodeModifier Driver\n"));
	TInt r = User::LoadLogicalDevice(KCodeModifierName);
	test( r==KErrNone || r==KErrAlreadyExists);
	if((r = Device.Open())!=KErrNone)	
		{
		User::FreeLogicalDevice(KCodeModifierName);
		test.Printf(_L("Could not open LDD"));
		test(0);
		}
	}


LOCAL_C void StopCodeModifierDriver()
	{

	test.Printf(_L("Stop Code Modifier Driver\n"));
	test(KErrNone==Device.CloseCodeModifier());
	Device.Close();
	User::FreeLogicalDevice(KCodeModifierName);
	}


LOCAL_C void TestCodeSetupDrive(RThread &thread)
{
	/* The CodeModifier driver (look in ../debug/d_codemodifier) takes two threads, we just use the
	** first one */
	test(KErrNone==Device.ThreadId(0, thread.Id()));
}


LOCAL_C TUint GetCodeData(TInt *CodePtr, TInt& Ignore, TInt& FirstJump, TInt& SecondJump)
	{ 
	TUint ModAddr;

	Ignore     = *CodePtr++;
	ModAddr    = (TUint)CodePtr;
	FirstJump  = *CodePtr++;
	SecondJump = *CodePtr++;
	return ModAddr;
	}

LOCAL_C TInt TestCodeModification(RPageMove &pagemove)
	{
	TInt Ignore; 
	TUint ModAddr;
	TInt FirstJump;
	TInt SecondJump;
	RThread thread;
	
	ModAddr = GetCodeData((TInt *)TestCodeModFunc, Ignore, FirstJump, SecondJump); 
	
	test.Printf(_L("User Test code Returns = %d\n"), TestCodeModFunc());
	test.Printf(_L("Ignore = %x First Jump = %x Second = %x \n"), Ignore, FirstJump, SecondJump);
	
	TestCodeSetupDrive(thread);

	for (TInt i=0; i<Repitions * 10; i++)
		{
		
		TInt r=Device.WriteCode(0, ModAddr,SecondJump,sizeof(TInt));
		test_KErrNone(r);
		r = TestCodeModFunc();
		test (2 == r);

		test_KErrNone(pagemove.TryMovingUserPage((TAny*)TestCodeModFunc));

		r = Device.RestoreCode(0, ModAddr);
		test_KErrNone(r);
		r = TestCodeModFunc();
		test (1 == r);
		
		test_KErrNone(pagemove.TryMovingUserPage((TAny*)TestCodeModFunc));

		}

	test.Printf(_L("User Test code = %d\n"), TestCodeModFunc());
	return KErrNone;
	}

LOCAL_C int TestCodeAsync(TAny *NotUsed)
	{
	TInt Ignore; 
	TUint ModAddr;
	TInt FirstJump;
	TInt SecondJump;

	ModAddr = GetCodeData((TInt *)TestCodeModFunc, Ignore, FirstJump, SecondJump); 

	FOREVER
		{
		TInt r = Device.WriteCode(0, ModAddr,SecondJump,sizeof(TInt));
		test_KErrNone(r);
		
		r = TestCodeModFunc();
		test (2 == r);

		r = Device.RestoreCode(0, ModAddr);
	
		test_KErrNone(r);
		r = TestCodeModFunc();
		test (1 == r);
		User::AfterHighRes(10);
		}
	}

/* 
 * Creates a Thread that modifies its code in a tight loop while the main
 * thread moves the functions page around
 */
LOCAL_C TInt TestCodeModificationAsync(RPageMove& pagemove)
	{
	TInt ret;
	RThread CodeThread;
	TRequestStatus s;

	
	/* Create the Thread to modify the code segment */
	test_KErrNone(CodeThread.Create(_L("TestCodeAsync"), TestCodeAsync, KDefaultStackSize, NULL, NULL));
	CodeThread.Logon(s);
	CodeThread.SetPriority(EPriorityMore);
	CodeThread.Resume();

	TestCodeSetupDrive(CodeThread);

	/* Loop trying to move the code page while the thread (CodeThread) modifies it */
	for (TInt i=0; i<Repitions; i++)
		{
		test_KErrNone(pagemove.TryMovingUserPage((TAny*)TestCodeModFunc));
		}

	CodeThread.Kill(KErrNone);
	User::WaitForRequest(s);
	test_Equal(EExitKill, CodeThread.ExitType());
	test_KErrNone(CodeThread.ExitReason());
	CodeThread.Close();

	ret = TestCodeModFunc();
	test(ret == 1 || ret == 2);

	return KErrNone;
	}
