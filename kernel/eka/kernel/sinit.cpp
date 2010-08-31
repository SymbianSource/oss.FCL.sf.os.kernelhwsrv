// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\sinit.cpp
//
//

#include <kernel/kern_priv.h>
#include <e32uid.h>
#include <collate.h>
#include <kernel/emi.h>
#include <kernel/sshbuf.h>
#include "platform.h"
#include "securerng.h"

#ifdef __VC32__
    #pragma setlocale("english")
#endif

#ifndef _UNICODE
Non-unicode kernel builds not supported
#endif

_LIT(KLitNull,"Null");
_LIT(KLitSupervisor,"Supervisor");
_LIT(KLitSvHeap,"SvHeap");
_LIT(KLitCodeSegLock,"CodeSegLock");
_LIT(KLitMsgChunk, "MsgChunk");
_LIT(KLitMsgChunkLock, "MsgChunkLock");
const TInt KMaxMsgChunkSize = 0x200000;

_LIT(KDShBufThreadName,"DShBufThread");
const TInt KDShBufThreadPriority = 7; // same priority as normal background thread
const TInt KDfcThread0Priority=27;
const TInt KDfcThread1Priority=48;
const TInt KMaxEventQueue=40;

#ifdef __SMP__
_LIT(KRebalanceName, "LB");
const TInt KRebalancePriority=26;
#endif

TInt SupervisorThread(TAny*);
TInt DebuggerInit();
extern TInt InitialiseEntropyBuffers();

extern const SNThreadHandlers EpocThreadHandlers;

/**
Returns information about how the system started

@return ETrue if the startup reason was a cold start and
		EFalse otherwise
*/
EXPORT_C TBool Kern::ColdStart()
	{
	return K::ColdStart;
	}

#ifdef __SMP__
EXPORT_D extern const TInt KSMPNumCpus = 0; // used in platform specific cinit.cpp to restrict number of CPUs

extern "C" void ApMainGeneric(volatile SAPBootInfo* aInfo)
	{
	DThread* t = (DThread*)aInfo->iArgs[0];
	__KTRACE_OPT(KBOOT,DEBUGPRINT("Initial thread at %08x", t));
	SNThreadCreateInfo info;
	info.iFunction = 0;
	info.iStackBase = (TAny*)aInfo->iInitStackBase;
	info.iStackSize = aInfo->iInitStackSize;
	info.iPriority = 0;
	info.iTimeslice = -1;
	info.iAttributes = t->iNThread.i_ThrdAttr;
	info.iHandlers = &EpocThreadHandlers;
	info.iFastExecTable = (const SFastExecTable*)EpocFastExecTable;
	info.iSlowExecTable = (const SSlowExecTable*)EpocSlowExecTable;
	info.iParameterBlock = (const TUint32*)aInfo;
	info.iParameterBlockSize = 0;
	info.iCpuAffinity = 0;	// nanokernel substitutes with correct CPU number
	info.iGroup = 0;

	TAny* ec = t->iNThread.iExtraContext;
	TInt ecs = t->iNThread.iExtraContextSize;
	NKern::Init(&t->iNThread, info);
	t->iNThread.SetExtraContext(ec, ecs);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("NKern::CurrentThread() = %08x", NKern::CurrentThread()));

	M::Init2AP();
	A::Init2AP();

	__e32_atomic_store_ord32(&aInfo->iArgs[1], 1);

	FOREVER
		{
		NKern::Idle();
		}
	}


void K::InitAP(TInt aCpu, volatile SAPBootInfo* aInfo, TInt aTimeout)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("K::InitAP aCpu=%d aInfo=%08x HwId=%08x aTimeout=%d", aCpu, aInfo, aInfo->iCpu, aTimeout));
	SThreadCreateInfo t;
	TBuf8<8> name(KLitNull);
	name.AppendNum(aCpu);
	memclr(&t, sizeof(t));
	t.iType=EThreadAPInitial;
	t.iName.Set(name);
	t.iTotalSize = sizeof(t);
	DThread* pN1 = NULL;
	TInt r = K::TheKernelProcess->NewThread(pN1, t, NULL, EOwnerProcess);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created idle thread %d, %d", aCpu+1, r));
	if (r!=KErrNone)
		K::Fault(K::EAPInitialThreadCreateFailed);

	aInfo->iInitStackSize = pN1->iSupervisorStackSize;
	aInfo->iInitStackBase = (TLinAddr)pN1->iSupervisorStack;
	aInfo->iMain = &ApMainGeneric;
	aInfo->iArgs[0] = (TAny*)pN1;
	aInfo->iArgs[1] = 0;
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("AP Initial thread at %08x", aInfo->iArgs[0]));

	r = NKern::BootAP(aInfo);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	while (--aTimeout && !__e32_atomic_load_acq32(&aInfo->iArgs[1]))
		{
		Kern::NanoWait(1000000);
		}
	if (!aTimeout)
		K::Fault(K::EInit2APTimeout);
	}
#endif

TInt K::InitialiseMicrokernel()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("K::InitialiseMicrokernel()"));

	// first phase cache/memory model initialisation
	M::Init1();

	// miscellaneous first-phase initialisation
	// this enables interrupts
	A::Init1();

	TProcessCreateInfo kpinfo;
	TAny* kern_stack_addr;
	TAny* kern_heap_addr;
	P::KernelInfo(kpinfo, kern_stack_addr, kern_heap_addr);
	__KTRACE_OPT(KBOOT,Kern::Printf("kern_heap_addr %08x, kern_stack_addr %08x",kern_heap_addr,kern_stack_addr));
	__KTRACE_OPT(KBOOT,Kern::Printf("HeapSizeMin %08x",kpinfo.iHeapSizeMin));

	// set up the kernel heap as a fixed heap
	RHeapK* kheap = RHeapK::FixedHeap(kern_heap_addr, kpinfo.iHeapSizeMin);
	K::Allocator = kheap;
	__KTRACE_OPT(KBOOT,Kern::Printf("Created kernel fixed heap at %08x, size %08x",K::Allocator,kpinfo.iHeapSizeMin));

	// create the initial thread and process
	DProcess* pP=P::NewProcess();
	pP->iPriority=EProcPrioritySystemServer3;
	DThread* pT=NULL;

	SThreadCreateInfo t;
	t.iType=EThreadInitial;
	t.iSupervisorStack=kern_stack_addr;
	t.iSupervisorStackSize=kpinfo.iStackSize;
	t.iName.Set(KLitNull);
	t.iTotalSize = sizeof(t);
	TInt r=pP->GetNewThread(pT, t);
	__KTRACE_OPT(KBOOT,Kern::Printf("Get initial thread, %d",r));
	if (r!=KErrNone)
		return r;

	SNThreadCreateInfo ni;
	ni.iHandlers = &EpocThreadHandlers;
	ni.iFastExecTable=(const SFastExecTable*)EpocFastExecTable;
	ni.iSlowExecTable=(const SSlowExecTable*)EpocSlowExecTable;
	ni.iParameterBlock=NULL;
	ni.iParameterBlockSize=0;
	ni.iStackBase=kern_stack_addr;
	ni.iStackSize=kpinfo.iStackSize;
#ifdef __SMP__
	ni.iCpuAffinity=0;
	ni.iGroup = 0;
#endif
	TAny* ec = pT->iNThread.iExtraContext;
	TInt ecs = pT->iNThread.iExtraContextSize;
	NKern::Init(&pT->iNThread,ni);
	__KTRACE_OPT(KBOOT,Kern::Printf("Done NKern::Init"));
	pT->iNThread.SetExtraContext(ec, ecs);

	r=pT->Create(t);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created initial thread, %d",r));
	if (r!=KErrNone)
		return r;
	r=pP->Create(ETrue, kpinfo, NULL);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created initial process, %d",r));
	if (r!=KErrNone)
		return r;
	K::TheNullThread=pT;
	K::TheKernelProcess=pP;
	K::SvMsgQ=new TDfcQue;
	if (!K::SvMsgQ)
		return KErrNoMemory;
	K::AsyncFreeDfc.SetDfcQ(K::SvMsgQ);
	K::AsyncChangeNotifierDfc.SetDfcQ(K::SvMsgQ);
	DCodeSeg::KernelCleanupDfc.SetDfcQ(K::SvMsgQ);
	TClientRequest::DeadClientCleanupDfc.SetDfcQ(K::SvMsgQ);

	// add the initial thread to the initial process
	pP->AddThread(*pT);

	// second phase memory model initialisation
	M::Init2();
	A::Init2();

	// create the object containers
	r=K::CreateObjectContainers();
	__KTRACE_OPT(KBOOT,Kern::Printf("Created object containers, %d",r));
	if (r!=KErrNone)
		return r;

	// add the initial thread to container
	r=K::AddObject(pT,EThread);
	__KTRACE_OPT(KBOOT,Kern::Printf("Added initial thread, %d",r));
	if (r!=KErrNone)
		return r;

	// add the initial process to container
	r=K::AddObject(pP,EProcess);
	__KTRACE_OPT(KBOOT,Kern::Printf("Added initial process, %d",r));
	if (r!=KErrNone)
		return r;

	SChunkCreateInfo cinfo;
	DChunk* pC=NULL;
	TLinAddr dataSectionBase=0;
	TInt kheap_offset = TInt(kern_heap_addr) - TInt(kpinfo.iDataRunAddress);
	TInt init_kernel_size = kheap_offset + kpinfo.iHeapSizeMin;
	TInt total_kernel_size = Kern::RoundToChunkSize(kheap_offset + kpinfo.iHeapSizeMax);
	pP->iDataBssRunAddress=kpinfo.iDataRunAddress;
	pP->iDataBssStackChunk=NULL;	// kernel doesn't have separate data chunk
	__KTRACE_OPT(KBOOT,Kern::Printf("kpinfo.iTotalDataSize = %08x",kpinfo.iTotalDataSize));
	__KTRACE_OPT(KBOOT,Kern::Printf("kpinfo.iStackSize     = %08x",kpinfo.iStackSize));
	__KTRACE_OPT(KBOOT,Kern::Printf("kheap_offset          = %08x",kheap_offset));
	__KTRACE_OPT(KBOOT,Kern::Printf("init_kernel_size      = %08x",init_kernel_size));
	__KTRACE_OPT(KBOOT,Kern::Printf("kpinfo.iHeapSizeMax   = %08x",kpinfo.iHeapSizeMax));
	__KTRACE_OPT(KBOOT,Kern::Printf("total_kernel_size     = %08x",total_kernel_size));

	// create the kernel heap chunk
	// this includes kernel .data, .bss, initial thread stack as well as kernel heap
	cinfo.iGlobal=EFalse;
	cinfo.iAtt=TChunkCreate::ENormal;
	cinfo.iForceFixed=EFalse;
	cinfo.iOperations=0;
	cinfo.iType=EKernelData;
	cinfo.iMaxSize=total_kernel_size;
	cinfo.iPreallocated=0;
	cinfo.iName.Set(KLitSvHeap);
	cinfo.iOwner=pP;
	r=pP->NewChunk(pC,cinfo,dataSectionBase);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvHeap, %d",r));
	if (r!=KErrNone)
		return r;
	r=M::InitSvHeapChunk(pC, init_kernel_size);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KMEMTRACE,Kern::Printf("MT:A %d %x %x %O",NTickCount(), pC, pC->iSize, pC));
#ifdef BTRACE_CHUNKS
	BTraceContext12(BTrace::EChunks,BTrace::EChunkMemoryAllocated,pC,0,pC->iSize);
#endif

	// create the kernel heap interlock mutex
	r = kheap->CreateMutex();
	if (r!=KErrNone)
		return r;

	// create a chunk to hold supervisor-mode stacks
	r=M::InitSvStackChunk();
	if (r!=KErrNone)
		return r;

	// mutate the kernel heap into a chunk heap
	kheap->Mutate(kheap_offset, kpinfo.iHeapSizeMax);

	// Intialise debugger
	r= DebuggerInit();
	if (r!=KErrNone)
		return r;

	// create a chunk to hold all RMessageKs
	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.iGlobal = EFalse;
	cinfo.iAtt = TChunkCreate::ENormal;
	cinfo.iForceFixed = EFalse;
	cinfo.iOperations = SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
	cinfo.iType = EKernelMessage;
	cinfo.iMaxSize = Kern::RoundToChunkSize(KMaxMsgChunkSize);
	cinfo.iName.Set(KLitMsgChunk);
	cinfo.iOwner = pP;
#ifdef	__EPOC32__
	cinfo.iMapAttr = EMapAttrSupRw | EMapAttrCachedMax; // Full caching (not supported in emulator)
#endif

	pC = NULL;
	TLinAddr msgChunkAddress = 0;
	r = pP->NewChunk(pC, cinfo, msgChunkAddress);
	__KTRACE_OPT(KBOOT, Kern::Printf("NewChunk(MsgChunk) returned %d, address %08X", r, msgChunkAddress));
	if (r != KErrNone)
		return r;
	K::MsgInfo.iChunk = pC;
	K::MsgInfo.iBase = (TUint8*)msgChunkAddress;
	K::MsgInfo.iMaxSize = pC->iMaxSize;
	K::MsgInfo.iCurrSize = pC->iSize;
	__KTRACE_OPT(KBOOT, Kern::Printf("Created MsgChunk, base %08X, size %d, max size %08X",
		K::MsgInfo.iBase, K::MsgInfo.iCurrSize, K::MsgInfo.iMaxSize));

	// create the msg chunk mutex
	r = K::MutexCreate(K::MsgInfo.iMsgChunkLock, KLitMsgChunkLock, NULL, EFalse, KMutexOrdGeneral0);
	if (r != KErrNone)
		return r;

	// Create a DPowerModel object and setup K::PowerModel to point to.
	r = PowerModelInit();
	if (r!=KErrNone)
		return r;

	// create the code segment mutex
	r=K::MutexCreate(DCodeSeg::CodeSegLock, KLitCodeSegLock, NULL, EFalse, KMutexOrdCodeSegLock);
	if (r!=KErrNone)
		return r;

	// Create request object for implementation of NotifyIfCodeSegDestroyed
	r = Kern::CreateClientRequest(DCodeSeg::DestructNotifyRequest);
	if (r!=KErrNone)
		return r;

#ifdef __EMI_SUPPORT__
	// Create EMI a thread and Semaphore
	r = EMI::Init();
	if (r!=KErrNone)
		return r;
#endif

	pP->Loaded(kpinfo);

	K::TheMiscNotifierMgr.Init2();

#ifdef __SMP__
	if (!(TheSuperPage().KernelConfigFlags() & EKernelConfigDisableAPs))
		A::InitAPs();
#endif

	// create the supervisor thread
	t.iType=EThreadSupervisor;
	t.iFunction=SupervisorThread;
	t.iPtr=NULL;
	t.iInitialThreadPriority=EThrdPriorityMuchMore;
	t.iName.Set(KLitSupervisor);
	t.iSupervisorStack=NULL;
	t.iSupervisorStackSize=0;	// zero means use default value
	DThread* pN=NULL;
	r=pP->NewThread(pN, t, NULL, EOwnerProcess);
	__KTRACE_OPT(KBOOT,Kern::Printf("Created supervisor thread, %d",r));
	if (r!=KErrNone)
		return r;

	pN->iFlags |= KThreadFlagSystemPermanent; // supervisor thread can't exit for any reason

	K::SvThread=pN;
	K::TheKernelThread=pN;
	K::SvMsgQ->iThread=&pN->iNThread;
	K::SvBarrierQ.SetDfcQ(K::SvMsgQ);

	// clear IPC V1 available flag
	TheSuperPage().SetKernelConfigFlags(TheSuperPage().KernelConfigFlags() & ~EKernelConfigIpcV1Available);

	K::Initialising=EFalse;

	// resume the supervisor thread
	Kern::ThreadResume(*pN);

	return KErrNone;
	}

TInt SupervisorThread(TAny*)
	{
	TInt r=K::Init3();
	if (r!=KErrNone)
		K::Fault(K::EInit3Failed);

	P::StartExtensions();

	M::Init4();

#ifdef __SMP__
	TheScheduler.InitLB();
	TheScheduler.StartPeriodicBalancing();
#endif

	K::StartKernelServer();
	return 0;
	}

TInt K::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("K::Init3"));

	// Initialise Hal array
	K::InitHalEntryArray();

	// Third phase MMU initialisation
	M::Init3();

	// Intialize publish and subscribe (depends on demand paging)
	TInt r = PubSubPropertyInit();
	if (r!=KErrNone)
		return r;

	// Create the event queue
	K::CreateEventQueue(KMaxEventQueue);

	// Initialise the DFC system
	r=Kern::DfcQCreate(K::DfcQ0,KDfcThread0Priority);
	if (r!=KErrNone)
		return r;
	DThread* pT = _LOFF(K::DfcQ0->iThread, DThread, iNThread);
	pT->iFlags |= KThreadFlagSystemPermanent;

	r=Kern::DfcQCreate(K::DfcQ1,KDfcThread1Priority);
	if (r!=KErrNone)
		return r;
	pT = _LOFF(K::DfcQ1->iThread, DThread, iNThread);
	pT->iFlags |= KThreadFlagSystemPermanent;

	// create the DfcQ for DShPool
	r = Kern::DfcQInit(&DShPool::iSharedDfcQue,KDShBufThreadPriority,&KDShBufThreadName);
	if (r!=KErrNone)
		return r;

#ifdef __SMP__
	// create thread and DFC queue for load balancing
	TScheduler& s = TheScheduler;
	r = Kern::DfcQCreate(s.iRebalanceDfcQ, KRebalancePriority, &KRebalanceName);
	if (r!=KErrNone)
		return r;
	NThread* nt = TScheduler::LBThread();
	NKern::ThreadSetCpuAffinity(nt, KCpuAffinityAny);
	pT = _LOFF(nt, DThread, iNThread);
	pT->iFlags |= KThreadFlagSystemPermanent;
#endif

	// Initialise the RAM drive
	K::InitNvRam();

	// Start the millisecond timer
	NTimerQ::Init3(K::DfcQ1);

	// Start the system tick timer
	r=K::StartTickQueue();
	if (r!=KErrNone)
		return r;
	
	// Initilize the Secure RNG.
	SecureRNG = new DSecureRNG;
	
	// Initialise entropy buffers for secure RNG
	r=InitialiseEntropyBuffers();
    if (r!=KErrNone)
		return r;

	// Third phase initialisation of ASIC/Variant
	// This enables the system tick and millisecond timer
	A::Init3();

	// Mark the super page signature valid
	P::SetSuperPageSignature();

	return KErrNone;
	}

extern "C" void KernelMain()
	{
	K::Initialising=ETrue;
	__KTRACE_OPT(KBOOT,Kern::Printf("KernelMain"));

	// Zeroth phase initialisation of ASIC and VARIANT
	P::CreateVariant();

	NKern::Init0(K::VariantData[0]);

	TInt r=K::InitialiseMicrokernel();
	if (r!=KErrNone)
		K::Fault(K::EInitMicrokernelFailed);

	__KTRACE_OPT(KBOOT,Kern::Printf("Null thread running..."));
	__KTRACE_OPT(KBOOT,Kern::Printf("Null calling Idle()"));
	FOREVER
		{
		NKern::Idle();
		}
	}

LOCAL_C TInt PowerHal(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	if (K::PowerModel)
		return K::PowerModel->PowerHalFunction(aFunction,a1,a2);
	if (aFunction==EPowerHalSupplyInfo)
		{
		TSupplyInfoV1 info;
//		info.iMainBatteryInsertionTime;
		info.iMainBatteryStatus=EGood;
//		info.iMainBatteryInUseMicroSeconds=0;
//		info.iCurrentConsumptionMilliAmps;
//		info.iMainBatteryConsumedMilliAmpSeconds;
		info.iMainBatteryMilliVolts=3000;
		info.iMainBatteryMaxMilliVolts=3000;
		info.iBackupBatteryStatus=EGood;
		info.iBackupBatteryMilliVolts=3000;
		info.iBackupBatteryMaxMilliVolts=3000;
		info.iExternalPowerPresent=ETrue;
//		info.iExternalPowerInUseMicroSeconds;
//		info.iFlags;
		Kern::InfoCopy(*(TDes8*)a1,TPtrC8((const TUint8*)&info, sizeof(info)));
		return KErrNone;
		}
	else if (aFunction==EPowerHalTestBootSequence)
		return 0;
	return KErrNotSupported;
	}

LOCAL_C TInt kernelHal(TAny*, TInt aFunc, TAny* a1, TAny* a2)
	{
	return K::KernelHal(aFunc,a1,a2);
	}

LOCAL_C TInt variantHal(TAny*, TInt aFunc, TAny* a1, TAny* a2)
	{
	return A::VariantHal(aFunc,a1,a2);
	}

void K::InitHalEntryArray()
	{
	TInt arraySize=KMaxHalGroups*sizeof(SHalEntry2);
	K::HalEntryArray=(SHalEntry2*)Kern::AllocZ(arraySize);
	K::HalEntryArray[0].iFunction=kernelHal;
	K::HalEntryArray[1].iFunction=variantHal;
	K::HalEntryArray[3].iFunction=PowerHal;
	}

TInt DebuggerInit()
//
// Initialise the debugger API
//
	{
	Kern::SuperPage().iDebuggerInfo=NULL;
	return KErrNone;
	}
