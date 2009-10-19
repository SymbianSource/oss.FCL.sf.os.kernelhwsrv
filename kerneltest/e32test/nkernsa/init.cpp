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
// e32test\nkernsa\init.cpp
// 
//

#include <nktest/nkutils.h>

extern "C" {
TLinAddr RomHeaderAddress;
TLinAddr SuperPageAddress;
TBool InitialThreadDefined;
}

NThread* InitialThread;
TDfcQue* TimerDfcQ;
TDfcQue* CleanupDfcQ;
NThread* MainThread;

#ifdef __SMP__
const char* NullNames[KMaxCpus] =
	{
	"Null0",
	"Null1",
	"Null2",
	"Null3",
	"Null4",
	"Null5",
	"Null6",
	"Null7"
	};
#endif

extern "C" void StartSystemTimer();
extern "C" void Hw_Init1();
extern "C" void Hw_InitAPs();
extern "C" void Hw_Init3();
extern "C" TLinAddr __initial_stack_base();
extern "C" TInt __initial_stack_size();

extern void Main(TAny*);

extern "C" void KernelMain()
	{
	KPrintf("KernelMain()");
	__CHECKPOINT();

	Hw_Init1();
	NTimerQ::Init1(__timer_period());
	SetupMemoryAllocator();

	InitialThread = new NThread;
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("Initial thread at %08x", InitialThread));
	SNThreadCreateInfo info;

	info.iFunction = 0;
	info.iStackBase = (TAny*)__initial_stack_base();
	info.iStackSize = __initial_stack_size();
	info.iPriority = 0;
	info.iTimeslice = -1;
	info.iAttributes = 0;
	info.iHandlers = 0;
	info.iFastExecTable = 0;
	info.iSlowExecTable = 0;
	info.iParameterBlock = 0;
	info.iParameterBlockSize = 0;
#ifdef __SMP__
	info.iCpuAffinity = 0;
	info.iGroup = 0;
#endif

	NKern::Init(InitialThread, info);
#ifdef __SMP__
	InitialThread->iNThreadBaseSpare8 = (TUint32)NullNames[NKern::CurrentCpu()];
#else
	InitialThread->iSpare8 = (TUint32)"Null";
#endif
	InitialThreadDefined = ETrue;

	KPrintf("NKern::CurrentThread() = %08x", NKern::CurrentThread());
	KPrintf("TR=%x",__cpu_id());

#ifdef __SMP__
	Hw_InitAPs();
#endif

	CleanupDfcQ = CreateDfcQ("Cleanup", 17, KCpuAffinityAny);
	TimerDfcQ = CreateDfcQ("Timer", 48);
	NTimerQ::Init3(TimerDfcQ);
	Hw_Init3();

	init_fast_counter();

	MainThread = CreateThread("Main", &Main, 16, 0, 0, ETrue, KTimeslice);

	KPrintf("spinning ...");
	FOREVER
		{
		NKern::Idle();
		}
	}

#ifdef __SMP__
extern "C" void ApMainGeneric(volatile SAPBootInfo* aInfo)
	{
	NThread* t = (NThread*)aInfo->iArgs[0];
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("Initial thread at %08x", t));
	SNThreadCreateInfo info;

	info.iFunction = 0;
	info.iStackBase = (TAny*)aInfo->iInitStackBase;
	info.iStackSize = aInfo->iInitStackSize;
	info.iPriority = 0;
	info.iTimeslice = -1;
	info.iAttributes = 0;
	info.iHandlers = 0;
	info.iFastExecTable = 0;
	info.iSlowExecTable = 0;
	info.iParameterBlock = (const TUint32*)aInfo;
	info.iParameterBlockSize = 0;
	info.iCpuAffinity = 0;	// nanokernel substitutes with correct CPU number
	info.iGroup = 0;

	NKern::Init(t, info);
	t->iNThreadBaseSpare8 = (TUint32)NullNames[NKern::CurrentCpu()];

	KPrintf("NKern::CurrentThread() = %08x\n", NKern::CurrentThread());

	__e32_atomic_store_ord32(&aInfo->iArgs[1], 1);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("AP status=%08x", __cpu_status_reg()));
	KPrintf("AP TR=%x",__cpu_id());

	FOREVER
		{
		NKern::Idle();
		}
	}
#endif

TBool BTrace::IsSupported(TUint aCategory)
	{
	if(aCategory>255)
		return EFalse;
	return ETrue;
	}

