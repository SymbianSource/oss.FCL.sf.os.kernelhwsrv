// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\d_kernasmfnc.cpp
// 
//

#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/kern_priv.h>
#include "d_kernbm.h"
#include "d_kernasmbm.h"

TUint8* UserPtr;		// pointer to user-side buffer
TUint8* KernelPtr;	// pointer to kernel-side buffer

NFastMutex* FastMutex;
NFastSemaphore* FastSem;

DThread* Thread;

void DfcFunc(TAny*)
	{
	}

TDfc Dfc(DfcFunc, NULL, Kern::DfcQue0(), 3);

TPriList<TPriListLink, 64> PriList;

_LIT(KObjectName, "This is a valid object name.");

TBitMapAllocator* Bma;
TInt BitmapList[32];
const TUint8* CharData = (const TUint8*) "Here's some char data to compare with memicmp";

// Allocate a 32-byte aligned block
TAny* AllocAligned(TUint aSize)
	{
	TUint mem = (TUint)Kern::Alloc(aSize + 32 + 4);
	if (!mem)
		return NULL;
	TUint* ptr = (TUint*) ALIGN_ADDR(mem + 4);
	ptr[-1] = mem;
	return ptr;
	}

// Free a block allocated by AllocAligned
void FreeAligned(TAny* ptr)
	{
	Kern::Free((TAny*)((TUint*)ptr)[-1]);
	}

TBitMapAllocator* AllocAlignedBMA(TInt aSize, TBool aState)
	{
	TInt nmapw=(aSize+31)>>5;
	TInt memsz=sizeof(TBitMapAllocator)+(nmapw-1)*sizeof(TUint32);
	TBitMapAllocator* pA=(TBitMapAllocator*)AllocAligned(memsz);
	if (pA)
		new(pA) TBitMapAllocator(aSize, aState);
	return pA;
	}

TInt ThreadFunc(TAny* aPtr)
	{
	return KErrNone;
	}

TInt InitData()
	{
	KernelPtr = (TUint8*)AllocAligned(KKernAsmBmBufferSize);
	if (!KernelPtr)
		return KErrNoMemory;
	
	Bma = AllocAlignedBMA(256, ETrue);
	if (!Bma)
		return KErrNoMemory;

	FastSem = (NFastSemaphore*) AllocAligned(sizeof(NFastSemaphore));
	if (!FastSem)
		return KErrNoMemory;
	new (FastSem) NFastSemaphore;
	NKern::FSSetOwner(FastSem, NULL);

	FastMutex = (NFastMutex*) AllocAligned(sizeof(FastMutex));
	if (!FastMutex)
		return KErrNoMemory;
	new (FastMutex) NFastMutex;

	SThreadCreateInfo info;
	info.iType=EThreadSupervisor;
	info.iFunction=ThreadFunc;
	info.iPtr=NULL;
	info.iSupervisorStack=NULL;
	info.iSupervisorStackSize=0;	// zero means use default value
	info.iInitialThreadPriority=NKern::CurrentThread()->iPriority;
	info.iName.Set(_L("bmthread2"));
	info.iTotalSize = sizeof(info);

	NKern::ThreadEnterCS();
	TInt r = Kern::ThreadCreate(info);
	NKern::ThreadLeaveCS();
	if (r != KErrNone)
		return r;
	
	Thread = (DThread*)info.iHandle;
	
	return KErrNone;
	}

void CloseData()
	{
	FreeAligned(KernelPtr); KernelPtr = NULL;
	FreeAligned(Bma); Bma = NULL;
	FreeAligned(FastSem); FastSem = NULL;
	FreeAligned(FastMutex); FastMutex = NULL;
	Kern::ThreadResume(*Thread); Thread = NULL; // thread will now exit
	}

// 1.     Functions that have C++ equivalents

// 1.1    NKern

// 1.1.1  DFCs

DEFINE_BENCHMARK(Dfc_DoEnqueCancel,
				 NKern::Lock(),
				 Dfc.DoEnque(); Dfc.Cancel(),
				 NKern::Unlock());

DEFINE_BENCHMARK(Dfc_AddCancel,
				 NKern::Lock(),
				 Dfc.Add(); Dfc.Cancel(),
				 NKern::Unlock());

// Not exported: TDfc::DoEnqueFinal, TDfc::ThreadFunction

// 1.1.2  Fast mutex

DEFINE_BENCHMARK(NKern_LockUnlockSystem,
				 ,
				 NKern::LockSystem(); NKern::UnlockSystem(),
				 );

DEFINE_BENCHMARK(NFastMutex_WaitSignal,
				 NKern::Lock(),
				 FastMutex->Wait(); FastMutex->Signal(),
				 NKern::Unlock());  // no contention

DEFINE_BENCHMARK(NKern_FMWaitSignal,
				 ,
				 NKern::FMWait(FastMutex); NKern::FMSignal(FastMutex),
				 );  // no contention

// Benchmark that exercises contention for fast mutex
DEFINE_THREADED_BENCHMARK(NFastMutex_ThreadSwitch,
						  -1,
						  Kern::ThreadResume(*iThread2),
						  NKern::FSWait(FastSem);   NKern::FMWait(FastMutex); NKern::FMSignal(FastMutex),
  						  NKern::FMWait(FastMutex); NKern::FSSignal(FastSem); NKern::FMSignal(FastMutex),
  						  );

// Benchmark thread switching with suspend/resume for comparison
DEFINE_THREADED_BENCHMARK(Kern_ThreadSwitch,
						  1,
						  ,
						  Kern::ThreadResume(*iThread2),
						  Kern::ThreadSuspend(*iThread2, 1),
						  Kern::ThreadResume(*iThread2));

// 1.1.3  Fast semaphore

DEFINE_BENCHMARK(NFastSem_Wait,
				 NKern::Lock(); FastSem->SignalN(aParams.iIts * 10),
				 FastSem->Wait(),
				 NKern::Unlock());

DEFINE_BENCHMARK(NFastSem_Signal,
				 NKern::Lock(),
				 FastSem->Signal(),
				 FastSem->Reset(); NKern::Unlock());

DEFINE_BENCHMARK(NFastSem_SignalN,
				 NKern::Lock(),
				 FastSem->SignalN(1),
				 FastSem->Reset(); NKern::Unlock());

DEFINE_BENCHMARK(NFastSem_Reset,
				 NKern::Lock(),
				 FastSem->Reset(),
				 NKern::Unlock());

// NFastSemaphore::WaitCancel not exported

DEFINE_THREADED_BENCHMARK(NFastSem_ThreadSwitch,
						  -1,
						  Kern::ThreadResume(*iThread2),
						  NKern::FSWait(FastSem),
						  NKern::FSSignal(FastSem),
						  );

DEFINE_BENCHMARK(NKern_WaitForAnyReq,
				 NKern::ThreadRequestSignal(NULL, aParams.iIts * 10),
				 NKern::WaitForAnyRequest(),
				 );

DEFINE_BENCHMARK(NKern_ThreadReqSignal,
				 NThread* t = &Thread->iNThread; NKern::Lock(),
				 NKern::ThreadRequestSignal(t),
				 t->iRequestSemaphore.Reset();NKern::Unlock());

DEFINE_BENCHMARK(NKern_ThreadReqSignal2,
				 NThread* t = &Thread->iNThread,
				 NKern::FMWait(FastMutex); NKern::ThreadRequestSignal(t, FastMutex),
				 NKern::Lock(); t->iRequestSemaphore.Reset(); NKern::Unlock(););

// 1.1.4  Timers

DEFINE_BENCHMARK(NTimer_OneShot,
				 NTimer timer,
				 timer.OneShot(100); timer.Cancel(),
				 );

DEFINE_BENCHMARK(NTimer_Again,
				 NTimer timer,
				 timer.Again(100); timer.Cancel(),
				 );

// Not exported:
//	  NTimerQ::Add
//	  NTimerQ::AddFinal
//	  NTimerQ::DfcFn
//	  NTimerQ::Dfc
// Not tested:
//	  NTimerQ::IdleTime

// 1.1.5  Priority list

DEFINE_BENCHMARK(TPriList_AddRemove,
				 TPriListLink link(15),
				 PriList.Add(&link); PriList.Remove(&link),
				 );

DEFINE_BENCHMARK(TPriList_AddRemove2,
				 TPriListLink link(15); TPriListLink link2(15); PriList.Add(&link2),
				 PriList.Add(&link); PriList.Remove(&link),
				 PriList.Remove(&link2));

DEFINE_BENCHMARK(TPriList_First,
				 TPriListLink link(15); PriList.Add(&link),
				 PriList.First(),
				 PriList.Remove(&link));

DEFINE_BENCHMARK(TPriList_HighestPri,
				 TPriListLink link(15); PriList.Add(&link),
				 PriList.HighestPriority(),
				 PriList.Remove(&link));

DEFINE_BENCHMARK(TPriList_ChangePri,
				 TPriListLink link(15); PriList.Add(&link); TInt pri = 15,
				 pri ^= 32; PriList.ChangePriority(&link, pri),
				 PriList.Remove(&link));

// 1.2 Kern

DEFINE_BENCHMARK(Kern_ValidateFullName,
				 ,
				 Kern::ValidateFullName(KObjectName),
				 );

// 1.3. klib

DEFINE_BENCHMARK(TBitMapAlloc_Ctor,
				 ,
				 new (Bma) TBitMapAllocator(256, ETrue),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_AllocFree1,
				 ,
				 Bma->Alloc(17, 119); Bma->Free(17, 119),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_AllocFree2,
				 Bma->Alloc(0, 119),
				 Bma->Free(Bma->Alloc()),
				 Bma->Free(0, 119));

DEFINE_BENCHMARK(TBitMapAlloc_SelectFree,
				 ,
				 Bma->SelectiveFree(0, 153),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_NotFree,
				 ,
				 Bma->NotFree(0, 119),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_NotAlloc,
				 ,
				 Bma->NotAllocated(0, 119),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_AllocList,
				 ,
				 Bma->AllocList(32, BitmapList); Bma->Free(0, 32),
				 );

DEFINE_BENCHMARK(TBitMapAlloc_AllocAligned,
				 Bma->Alloc(0, 35); TInt a = 0; TInt b=0,
				 Bma->AllocAligned(64, 3, 0, EFalse, a, b),
				 Bma->Free(0, 35));

DEFINE_BENCHMARK(Kern_ValidateName,
				 ,
				 Kern::ValidateName(KObjectName),
				 );

DEFINE_BENCHMARK(memicmp,
				 ,
				 memicmp(CharData, CharData, 49),
				 );

// 2.	Functions that have no C++ equivalent

// 2.1. NKern

DEFINE_BENCHMARK(NKern_LockedInc,
				 TInt i,
				 NKern::LockedInc(i),
				 );

DEFINE_BENCHMARK(NKern_LockedDec,
				 TInt i,
				 NKern::LockedDec(i),
				 );

DEFINE_BENCHMARK(NKern_LockedAdd,
				 TInt i,
				 NKern::LockedAdd(i, 0),
				 );

DEFINE_BENCHMARK(NKern_LockedSetClear,
				 TUint32 i,
				 NKern::LockedSetClear(i, 0, 0),
				 );

DEFINE_BENCHMARK(NKern_LockedSetClear8,
				 TUint8 i,
				 NKern::LockedSetClear8(i, 0, 0),
				 );

DEFINE_BENCHMARK(NKern_SafeInc,
				 TInt i = 0,
				 NKern::SafeInc(i),
				 );

DEFINE_BENCHMARK(NKern_SafeDec,
				 TInt i = KMaxTInt,
				 NKern::SafeDec(i),
				 );

DEFINE_BENCHMARK(NKern_SafeSwap,
				 TAny* i,
				 NKern::SafeSwap(0, i),
				 );

DEFINE_BENCHMARK(NKern_SafeSwap8,
				 TUint8 i,
				 NKern::SafeSwap8(0, i),
				 );

DEFINE_BENCHMARK(NKern_LockUnlock,
				 ,
				 NKern::Lock(); NKern::Unlock(),
				 );

DEFINE_BENCHMARK(NKern_DisableInts1,
				 ,
				 NKern::RestoreInterrupts(NKern::DisableInterrupts(1)),
				 );

DEFINE_BENCHMARK(NKern_DisableInts2,
				 ,
				 NKern::RestoreInterrupts(NKern::DisableInterrupts(2)),
				 );

// 2.2 Kern

DEFINE_BENCHMARK(Kern_NanoWait,
				 ,
				 Kern::NanoWait(1000),
				 );  // expect 1uS!

DEFINE_BENCHMARK(Kern_KUSafeInc,
				 umemset(UserPtr, 1, sizeof(TInt)),
				 Kern::KUSafeInc(*(TInt*)UserPtr),
				 );

DEFINE_BENCHMARK(Kern_KUSafeDec,
				 umemset(UserPtr, KMaxTInt8, sizeof(TInt)),
				 Kern::KUSafeDec(*(TInt*)UserPtr),
				 );

DEFINE_BENCHMARK(DThread_ObjectFromHandle,
				 DThread* thread = &Kern::CurrentThread(),
				 thread->ObjectFromHandle(1),
				 );

DEFINE_BENCHMARK(Kern_ObjectFromHandle,
				 DThread* thread = &Kern::CurrentThread(),
				 Kern::ObjectFromHandle(thread, 1, -1),
				 );

DEFINE_BENCHMARK(Kern_KUSafeRead,
				 TUint8 buf[128],
				 Kern::KUSafeRead(UserPtr, buf, 128),
				 );

DEFINE_BENCHMARK(Kern_SafeRead,
				 TUint8 buf[128],
				 Kern::SafeRead(KernelPtr, buf, 128),
				 );

DEFINE_BENCHMARK(Kern_KUSafeWrite,
				 TUint8 buf[128],
				 Kern::KUSafeWrite(UserPtr, buf, 128),
				 );

DEFINE_BENCHMARK(Kern_SafeWrite,
				 TUint8 buf[128],
				 Kern::SafeWrite(KernelPtr, buf, 128),
				 );

DEFINE_BENCHMARK(Kern_SafeRead4,
				 TUint8 buf[128],
				 Kern::SafeRead(KernelPtr, buf, 4),
				 );

DEFINE_BENCHMARK(Kern_SafeRead8,
				 TUint8 buf[128],
				 Kern::SafeRead(KernelPtr, buf, 8),
				 );

// 2.3 klib

DEFINE_BENCHMARK(umemput32_16,
				 ,
				 umemput32(UserPtr, KernelPtr, 16),
				 );

DEFINE_BENCHMARK(umemput32_4,
				 ,
				 umemput32(UserPtr, KernelPtr, 4),
				 );

DEFINE_MEMORY_BENCHMARK(umemput32_64K,
						4,
						KernelPtr,
						UserPtr,
						,
						umemput32(dest, src, 65536),
						);

DEFINE_BENCHMARK(umemput_32,
				 ,
				 umemput(UserPtr, KernelPtr, 32),
				 );

DEFINE_BENCHMARK(umemput_40,
				 ,
				 umemput(UserPtr, KernelPtr, 40),
				 );

DEFINE_MEMORY_BENCHMARK(umemput_64K,
						1,
						KernelPtr,
						UserPtr,
						,
						umemput(dest, src, 65536),
						);

DEFINE_BENCHMARK(umemget32_32,
				 ,
				 umemget32(KernelPtr, UserPtr, 32),
				 );

DEFINE_MEMORY_BENCHMARK(umemget32_64K,
						4,
						UserPtr,
						KernelPtr,
						,
						umemget32(dest, src, 65536),
						);

DEFINE_BENCHMARK(umemget_7,
				 ,
				 umemget(KernelPtr, UserPtr, 7),
				 );

DEFINE_MEMORY_BENCHMARK(umemget_64K,
						1,
						UserPtr,
						KernelPtr,
						,
						umemget(dest, src, 65536),
						);

DEFINE_BENCHMARK(umemset_64K,
				 ,
				 umemset(UserPtr, 23, 65536),
				 );

// Not exported:
//   K::ObjectFromHandle(TInt /*aHandle*/)
//   K::ObjectFromHandle(TInt /*aHandle*/, TInt /*aType*/)
//   ExecHandler::LockedInc
//   ExecHandler::LockedDec
//   ExecHandler::SafeInc
//   ExecHandler::SafeDec
