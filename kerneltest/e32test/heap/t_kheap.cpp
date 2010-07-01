// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\t_kheap.cpp
// Overview:
// Test clean-up after kernel heap allocation failures.
// API Information:
// RProcess, RThread, RServer, RMutex, RChunk, RSemaphore, RTimer.
// Details:
// - Tests allocation of kernel objects in low memory conditions and checks for
// leaks (OOM testing).  The following objects are tested, with all
// combinations of attributes where applicable:
// - thread-relative timer.
// - local, global semaphore to the process, thread.
// - local, global mutex to the current process, thread.
// - server, handle to a file server session.
// - handle to a change notifier, a thread death notifier.
// - logical device driver, XIP and non-XIP (on hardware).
// - logical channel (hardware only excluding integrator).
// - chunks
// - chunks containg heaps (ie User::HeapChunk)
// - threads
// - Test that the kernel correctly zeros memory on allocation and reallocation
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
//
//

//! @file
//! @SYMTestCaseID  			KBASE-T_KHEAP-0163
//! @SYMREQ 					N/A
//! @SYMTestPriority 		High
//! @SYMTestActions 			Test allocation of Kernel objects in low memory conditions.
//! @SYMTestExpectedResults 	Test runs until this message is emitted: RTEST: SUCCESS : T_KHEAP test completed O.K.
//! @SYMTestType 			UT
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>
#include <hal.h>
#include <e32svr.h>
#include <f32dbg.h>
#include "d_kheap.h"

RTest test(_L("T_KHEAP"));
RLoader LoaderSession;

#ifdef _DEBUG
_LIT(KTestLdd0FileName, "D_LDD.LDD");
_LIT(KTestLdd1FileName, "D_LDD_RAM.LDD");
_LIT(KTestLddName, "Test");

#include "../mmu/mmudetect.h"
#include "../mmu/d_memorytest.h"

TInt gPageSize;
TBool LargeChunkOK=EFalse;
const TInt KLargeChunk=0x80000000;
const TInt KOwnerThread=0x40000000;
const TInt KMaxKernelAllocations=1024;
_LIT(KName,"TestObj");
typedef TInt (*TTestFunction)(TInt);

TUint32 WaitABit;

RMemoryTestLdd TestLdd;
RKHeapDevice KHeapDevice;

void TraceOn()
	{
	User::SetDebugMask(0xefdfffff);
	}

void TraceOff()
	{
	User::SetDebugMask(0x80000000);
	}

TInt TestTimer(TInt)
	{
	RTimer t;
	TInt r=t.CreateLocal();
	if (r==KErrNone)
		t.Close();
	return r;
	}

TInt TestLocalSem(TInt a)
	{
	TOwnerType ot=(TOwnerType)a;
	RSemaphore s;
	TInt r=s.CreateLocal(0, ot);
	if (r==KErrNone)
		s.Close();
	return r;
	}

TInt TestGlobalSem(TInt a)
	{
	TOwnerType ot=(TOwnerType)a;
	RSemaphore s;
	TInt r=s.CreateGlobal(KName, 0, ot);
	if (r==KErrNone)
		s.Close();
	return r;
	}

TInt TestLocalMutex(TInt a)
	{
	TOwnerType ot=(TOwnerType)a;
	RMutex m;
	TInt r=m.CreateLocal(ot);
	if (r==KErrNone)
		m.Close();
	return r;
	}

TInt TestGlobalMutex(TInt a)
	{
	TOwnerType ot=(TOwnerType)a;
	RMutex m;
	TInt r=m.CreateGlobal(KName, ot);
	if (r==KErrNone)
		m.Close();
	return r;
	}

TInt TestServer(TInt)
	{
	RServer2 s;
	TInt r=s.CreateGlobal(KName);
	if (r==KErrNone)
		s.Close();
	return r;
	}

TInt TestSession(TInt)
	{
	RFs fs;
	TInt r=fs.Connect();
	if (r==KErrNone)
		{
		r=fs.ShareAuto();
		fs.Close();
		}
	User::After(WaitABit);	// allow asynchronous cleanup to happen
	return r;
	}

TInt TestChangeNotifier(TInt)
	{
	RChangeNotifier n;
	TInt r=n.Create();
	if (r==KErrNone)
		n.Close();
	return r;
	}

TInt TestUndertaker(TInt)
	{
	RUndertaker u;
	TInt r=u.Create();
	if (r==KErrNone)
		u.Close();
	return r;
	}

TInt TestLogicalDevice(TInt aDevice)
	{
	const TDesC* fileName = NULL;
	const TDesC* objName = &KTestLddName();
	User::FreeLogicalDevice(*objName);
	switch (aDevice)
		{
		case 0:
			fileName = &KTestLdd0FileName();
			break;
		case 1:
			fileName = &KTestLdd1FileName();
			break;
		default:
			test(0);
		}
	TInt r = User::LoadLogicalDevice(*fileName);
	test_KErrNone(LoaderSession.CancelLazyDllUnload());	// make sure transient loader session has been destroyed
	if (r==KErrNone)
		{
		r = User::FreeLogicalDevice(*objName);
		test_KErrNone(r);
		}
	return r;
	}

TInt TestLogicalChannel(TInt)
	{
	RKHeapDevice d;
	TInt r=d.Open();
	if (r==KErrNone)
		d.Close();
	return r;
	}

TInt TestChunk(TInt att)
	{
	TChunkCreateInfo createInfo;
	TInt maxSize = (att & KLargeChunk)? (33*1048576) : gPageSize;
	createInfo.SetOwner((att & KOwnerThread)? EOwnerThread : EOwnerProcess);

	if (att & TChunkCreate::EGlobal)
		createInfo.SetGlobal(KName());

	switch (att & (TChunkCreate::ENormal | TChunkCreate::EDoubleEnded | TChunkCreate::EDisconnected))
		{
		case TChunkCreate::ENormal:
			createInfo.SetNormal(gPageSize, maxSize);
			break;
		case TChunkCreate::EDoubleEnded:
			createInfo.SetDoubleEnded(0, gPageSize, maxSize);
			break;
		case TChunkCreate::EDisconnected:
			createInfo.SetDisconnected(0, gPageSize, maxSize);
			break;
		default:
			test(EFalse);
			break;
		}
	RChunk c;
	TInt r=c.Create(createInfo);
	if (r==KErrNone)
		c.Close();
	return r;
	}

TInt TestHeap(TInt att)
	{
	if (att < 2)
		{
		RHeap* h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x10000, 1, 4, att);
		if (h)
			h->Close();
		return h ? KErrNone : KErrNoMemory;
		}
	RChunk c;
	TInt r = c.CreateLocal(0, 0x100000);
	if (r != KErrNone)
		return r;
	RHeap* h = UserHeap::ChunkHeap(c, 0x1000, 1, 0, 4, att&1, (att&2) ? UserHeap::EChunkHeapDuplicate : 0);
	if ((att & 2) || !h)
		c.Close();
	if (h)
		h->Close();
	return h ? KErrNone : KErrNoMemory;
	}

TInt TestThreadFunction(TAny* a)
	{
	TInt x=(TInt)a;
	return (x+1)*(x+1);
	}

TInt TestThread(TInt att)
//
// bit 0 -> EOwnerThread
// bit 1 -> use name
// bit 2 -> let it run
// bit 3 -> use own heap
//
	{
	TOwnerType ot=(att&1)?EOwnerThread:EOwnerProcess;
	const TDesC* name=(att&2)?&KName():&KNullDesC();
	TBool run=att&4;
	TBool ownheap=att&8;
	RThread t;
	TInt r=0;
	if (ownheap)
		r=t.Create(*name, TestThreadFunction, 0x1000, 0x1000, 0x10000, (TAny*)att, ot);
	else
		r=t.Create(*name, TestThreadFunction, 0x1000, NULL, (TAny*)att, ot);
	if (r!=KErrNone)
		{
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
		return r;
		}
	t.SetPriority(EPriorityMore);
	TRequestStatus s;
	t.Logon(s);
	if (run)
		t.Resume();
	else
		t.Kill((att+1)*(att+1));
	User::WaitForRequest(s);
	if (s==KErrNoMemory)		// if logon failed due to OOM ...
		User::After(WaitABit);	// ... allow thread to terminate before checking exit type
	test(t.ExitType()==EExitKill);
	r=s.Int();
	if (r>=0)
		{
		test(r==(att+1)*(att+1));
		r=KErrNone;
		}
	t.Close();
	User::After(WaitABit);		// let supervisor run - can't use destruct notifier since it involves memory allocation
	return r;
	}

void DoTest(TTestFunction aFunc, TInt aParam)
	{
	test.Printf(_L("DoTest f=%08x p=%08x\n"),aFunc,aParam);
	__KHEAP_MARK;
	__KHEAP_FAILNEXT(1);
	test_Equal(KErrNoMemory, (*aFunc)(aParam));
	test_KErrNone(UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0));
	__KHEAP_MARKEND;
	__KHEAP_RESET;
	__KHEAP_MARK;
	test_KErrNone((*aFunc)(aParam));
	test_KErrNone(UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0));
	__KHEAP_MARKEND;

	TInt i;
	TInt r=KErrNoMemory;
	for (i=0; i<KMaxKernelAllocations && r==KErrNoMemory; i++)
		{
		__KHEAP_MARK;
		__KHEAP_FAILNEXT(i);
		r=(*aFunc)(aParam);
		test_KErrNone(UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0));
		__KHEAP_MARKEND;
		__KHEAP_RESET;
		}
	test.Printf(_L("Took %d tries\n"),i);
	test_KErrNone(r);
	}

_LIT(KLddName, "ECOMM");
#ifdef __EPOC32__
_LIT(KPddName, "EUART");
#else
_LIT(KPddName, "ECDRV");
#endif
TInt LoadDeviceDrivers()
//
// Load ECOMM.LDD and all PDDs with name EUART?.PDD
//
	{
	TInt c=0;
	TInt r;
	TInt i;
	TFileName n=KPddName();
	TInt p=n.Length();
	for (i=-1; i<10; ++i)
		{
		if (i==0)
			n.Append('0');
		else if (i>0)
			n[p]=TText('0'+i);
		r=User::LoadPhysicalDevice(n);
		if (r==KErrNone || r==KErrAlreadyExists)
			{
			c++;
			test.Printf(_L("Loaded PDD %S\n"),&n);
			}
		}
	r=User::LoadLogicalDevice(KLddName);
	if (r==KErrNone || r==KErrAlreadyExists)
		{
		c+=255;
		test.Printf(_L("Loaded LDD %S\n"),&KLddName());
		}
	return c;
	}

void TestKernAllocZerosMemory()
	{
	test.Next(_L("Kern::Alloc memory initialisation"));
	test_KErrNone(TestLdd.Open());
	__KHEAP_MARK;
	TInt r=TestLdd.TestAllocZerosMemory();
	__KHEAP_MARKEND;
	if (r)
		test.Printf(_L("TestAllocZerosMemory returned %08x\n"), r);
	test_KErrNone(r);
	__KHEAP_MARK;
	r=TestLdd.TestReAllocZerosMemory();
	__KHEAP_MARKEND;
	if (r)
		test.Printf(_L("TestReAllocZerosMemory returned %08x\n"), r);
	test_KErrNone(r);
	}

TInt TestCreateSharedChunk(TInt)
	{
	TInt r = KHeapDevice.CreateSharedChunk();
	User::After(WaitABit);	// let supervisor run - can't use destruct notifier since it involves memory allocation
	return r;
	}
#ifdef __EPOC32__
TInt TestCreateHwChunk(TInt a)
	{
	TInt r = KHeapDevice.CreateHwChunk();
	User::After(WaitABit);	// let supervisor run - can't use destruct notifier since it involves memory allocation
	return r;
	}
#endif

GLDEF_C TInt E32Main()
//
// Test kernel alloc heaven with all out of memory possibilities
//
	{

/*	Objects
 *	Thread	       tested here
 *	Process	 tested by loader tests
 *	Chunk		 tested here
 *	Library		 tested by loader tests
 *	Semaphore tested here
 *	Mutex		 tested here
 *	Server		 tested here
 *	Session	 tested here
 *	LDev		 tested by loader tests (to do)
 *	PDev		 tested by loader tests (to do)
 *	LChan		 tested here
 *	ChangeNot	 tested here
 *	Undertaker	 tested here
 */

	test.Title();
	test.Start(_L("Testing kernel OOM handling"));

	TInt factor = UserSvr::HalFunction(EHalGroupVariant, EVariantHalTimeoutExpansion, 0, 0);
	if (factor<=0)
		factor = 1;
	if (factor>1024)
		factor = 1024;
	WaitABit = 200000 * (TUint32)factor;

#ifdef __EPOC32__	// no WINS serial drivers yet
	test.Next(_L("Load comms drivers"));
	test(LoadDeviceDrivers()>=256);
#endif
	test_KErrNone(UserHal::PageSizeInBytes(gPageSize));

	// Keep a session to the loader
	TInt r;
	r = LoaderSession.Connect();
	test_KErrNone(r);

	// Turn off lazy dll unloading
	test_KErrNone(LoaderSession.CancelLazyDllUnload());

	if (TestChunk(KLargeChunk) == KErrNone)
		{
		LargeChunkOK = ETrue;
		}
	test.Next(_L("Load/open d_kheap test driver"));
	r = User::LoadLogicalDevice(KHeapTestDriverName);
	test( r==KErrNone || r==KErrAlreadyExists);
	if( KErrNone != (r=KHeapDevice.Open()) )
		{
		User::FreeLogicalDevice(KHeapTestDriverName);
		test.Printf(_L("Could not open LDD"));
		test(0);
		}

	test.Next(_L("Timer"));
	DoTest(TestTimer, 0);
	test.Next(_L("Local semaphore"));
	DoTest(TestLocalSem, EOwnerProcess);
	test.Next(_L("Global semaphore"));
	DoTest(TestGlobalSem, EOwnerProcess);
	test.Next(_L("Local semaphore"));
	DoTest(TestLocalSem, EOwnerThread);
	test.Next(_L("Global semaphore"));
	DoTest(TestGlobalSem, EOwnerThread);
	test.Next(_L("Local mutex"));
	DoTest(TestLocalMutex, EOwnerProcess);
	test.Next(_L("Global mutex"));
	DoTest(TestGlobalMutex, EOwnerProcess);
	test.Next(_L("Local mutex"));
	DoTest(TestLocalMutex, EOwnerThread);
	test.Next(_L("Global mutex"));
	DoTest(TestGlobalMutex, EOwnerThread);
	test.Next(_L("Server"));
	DoTest(TestServer, 0);
	test.Next(_L("Session"));
	DoTest(TestSession, 0);
	test.Next(_L("Change notifier"));
	DoTest(TestChangeNotifier, 0);
	test.Next(_L("Undertaker"));
	DoTest(TestUndertaker, 0);
	test.Next(_L("Logical Device (XIP)"));
	DoTest(TestLogicalDevice, 0);
#ifdef __EPOC32__
	test.Next(_L("Logical Device (Non-XIP)"));
	//The first time a non-XIP LDD is loaded , Kernel permanently allocates some memory.
	//The next line makes sure it happens before we start testing OOM condition.
	TestLogicalDevice(1);
	//Further non-XIP LDDs loadings must not leak the memory.
	DoTest(TestLogicalDevice, 1);

	// Temporary hack to avoid clash with debug output on Integrator
	TInt muid;
	r = HAL::Get(HAL::EMachineUid, muid);
	test_KErrNone(r);
	if (muid != HAL::EMachineUid_Integrator)
		{
		test.Next(_L("Logical Channel"));
		r = User::LoadLogicalDevice(KHeapTestDriverName);
		test(r==KErrNone || r==KErrAlreadyExists);
		DoTest(TestLogicalChannel, 0);
		}
#endif
	TUint32 att;
	TUint32 attlim=LargeChunkOK?0x100:0x80;
	test.Next(_L("Chunk"));
	for (att=0; att<attlim; ++att)
		{
		if ((att&0x0f)>2)
			continue;
		TInt arg=att&~0xc0;
		if (att&0x40) arg|=KOwnerThread;
		if (att&0x80) arg|=KLargeChunk;
		DoTest(TestChunk, arg);
		}

	test.Next(_L("Heap"));
	for (att=0; att<8; ++att)
		{
		if (att==2 || att==3)
			continue;
		DoTest(TestHeap, att);
		}

	test.Next(_L("Thread"));
	for (att=0; att<16; ++att)
		{
		DoTest(TestThread, att);
		}

	TestKernAllocZerosMemory();

	test.Next(_L("Shared Chunk"));
	DoTest(TestCreateSharedChunk, 0);
#ifdef __EPOC32__
	test.Next(_L("Hw Chunk"));
	DoTest(TestCreateHwChunk, 0);
#endif

	test.Next(_L("Close/unload d_kheap test driver"));
	KHeapDevice.Close();
	User::FreeLogicalDevice(KHeapTestDriverName);
	LoaderSession.Close();
	test.End();
	return 0;
	}
#else
GLDEF_C TInt E32Main()
//
// _KHEAP_SETFAIL etc. not available in release mode, so don't test
//
	{

	test.Title();
	test.Start(_L("No tests in release mode"));
	test.End();
	return 0;
	}
#endif


