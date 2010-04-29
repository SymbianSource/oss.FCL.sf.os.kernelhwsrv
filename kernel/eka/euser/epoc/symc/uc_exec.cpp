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
// e32\euser\epoc\win32\uc_exec.cpp
// 
//


//#define __GEN_USER_EXEC_CODE__

#include "../win32/uc_std.h"
#include <e32svr.h>
#include <emulator.h>

//
#include <stdlib.h>
#include <stdio.h>

/*
Symbian compatibility executive panics
*/
enum TSymcExecPanic
	{
	ESymcExecPanicNotSupported,
	ESymcExecPanicHeapAlreadyExists,
	ESymcExecPanicCreateHeapFailed,
	ESymcExecPanicNotUsed
	};

void Panic(TInt aReason)
	{
	_LIT(KCategory,"SYMC-Exec");
	User::Panic(KCategory,aReason);
	}


const TInt KTrapStackSize=256;

/*
TODO: should we use CObject?
*/
class TThread
	{
public:
	
public:
	RSemaphore iRequestSemaphore;
	CActiveScheduler* iActiveScheduler; //Current active scheduler for this thread. Used.
	TTrapHandler* iHandler; //This is our cleanup stack. Used.
	//No idea why we need that trap stack
	//TTrap* iTrapStack[KTrapStackSize];
	//TInt iTrapCount;
	};

/*
TODO: should we use CObject?
Object used to store process globals for our pseudo kernel.
That's typically going to be a singleton.
*/
class TProcess
	{
public:
	void CreateHeap();
	void Free();

public:
	RHeap* iAllocator;
	TAny* iBase;
	TThread iThread; //Single thread for now
	};


void TProcess::CreateHeap()
	{
	//iThread.iTrapCount=0;
	//Define the size of our heap
	const TInt KHeapMaxSize=1024*1024*10; // 10 Mo for now
	__ASSERT_ALWAYS(iAllocator==NULL && iBase==NULL,Panic(ESymcExecPanicHeapAlreadyExists));	
	iBase=malloc(KHeapMaxSize);
	__ASSERT_ALWAYS(iBase!=NULL,Panic(ESymcExecPanicCreateHeapFailed));	
	//TODO: is there anyway we could use variable size heap?
	iAllocator=UserHeap::FixedHeap(iBase,KHeapMaxSize);
	__ASSERT_ALWAYS(iAllocator!=NULL,Panic(ESymcExecPanicCreateHeapFailed));	
	}

void TProcess::Free()
	{
	free(iBase);
	}



TProcess gProcess;



//


typedef TInt (__fastcall *TDispatcher)(TInt, TInt*);
TInt __fastcall LazyDispatch(TInt aFunction, TInt* aArgs);

#pragma data_seg(".data2")
#ifdef __VC32__
#pragma bss_seg(".data2")
#endif
static TDispatcher TheDispatcher = &LazyDispatch;
#pragma data_seg()
#ifdef __VC32__
#pragma bss_seg()
#endif

TInt __fastcall LazyDispatch(TInt aFunction, TInt* aArgs)
	{
	Panic(ESymcExecPanicNotSupported);
	//
	HINSTANCE kernel = GetModuleHandleA("ekern.dll");
	//HINSTANCE kernel = GetModuleHandleA("ekern.exe");
	if (kernel)
		{
		TDispatcher dispatcher = (TDispatcher)Emulator::GetProcAddress(kernel, (LPCSTR)1);
		if (dispatcher)
			{
			TheDispatcher = dispatcher;
			return dispatcher(aFunction, aArgs);
			}
		}
	ExitProcess(101);
	return 0;
	}

#include <u32exec.h>

/******************************************************************************
 * Slow executive calls with preprocessing or extra arguments
 ******************************************************************************/

__NAKED__ TInt Exec::SessionSend(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server.
//
	{
	__DISPATCH(EExecSessionSend|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::SessionSendSync(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server using thread's dedicated message slot.
//
	{
	__DISPATCH(EExecSessionSendSync|EXECUTIVE_SLOW)
	}


__NAKED__ TInt Exec::MessageIpcCopy(TInt /*aHandle*/, TInt /*aParam*/, SIpcCopyInfo& /*aInfo*/, TInt /*anOffset*/)
//
// Perform a descriptor-to-descriptor IPC copy
//
	{

	__DISPATCH(EExecMessageIpcCopy|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::BTraceOut(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecBTraceOut|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::BTraceOutBig(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecBTraceOutBig|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::UTraceOut(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecUTraceOut|EXECUTIVE_SLOW)
	}

EXPORT_C TBool BTrace::Out(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = 0;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86

	if((TUint)aDataSize>8u)
		{
		if((TUint)aDataSize>KMaxBTraceDataArray+4u)
			return Exec::BTraceOutBig(a0,a1,ext,aDataSize);
		a0 += 4;
		aDataSize -= 4;
		ext.iA2 = *((TUint32*&)aData)++;
		ext.iA3 = (TUint32)aData;
		return Exec::BTraceOut(a0,a1,ext,aDataSize);
		}

	if((TUint)aDataSize>4u)
		ext.iA3 = ((TUint32*)aData)[1];
	if(aDataSize)
		ext.iA2 = ((TUint32*)aData)[0];
	a0 += aDataSize;
	aDataSize = 0;
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFiltered(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutFilteredX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutFilteredN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = 0;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86

	if((TUint)aDataSize>8u)
		{
		if((TUint)aDataSize>KMaxBTraceDataArray+4u)
			return Exec::BTraceOutBig(a0,a1,ext,aDataSize);
		a0 += 4;
		aDataSize -= 4;
		ext.iA2 = *((TUint32*&)aData)++;
		ext.iA3 = (TUint32)aData;
		return Exec::BTraceOut(a0,a1,ext,aDataSize);
		}

	if((TUint)aDataSize>4u)
		ext.iA3 = ((TUint32*)aData)[1];
	if(aDataSize)
		ext.iA2 = ((TUint32*)aData)[0];
	a0 += aDataSize;
	aDataSize = 0;
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredPcFormatBig(TUint32 aHeader, TUint32 aModuleUid, TUint32 aPc, TUint16 aFormatId, const TAny* aData, TInt aDataSize)
	{
 	BTrace::SExecExtension ext;
	aHeader |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = aFormatId;
	ext.iA3 = (TUint32)aData;
	ext.iPc = aPc;

	if((TUint)aDataSize>KMaxBTraceDataArray)
		return Exec::UTraceOut(aHeader,aModuleUid,ext,aDataSize);
	aHeader += 4;
	return Exec::BTraceOut(aHeader,aModuleUid,ext,aDataSize);
	}

__NAKED__ void ExecRequestComplete(TInt aHandle, TRequestStatus*& aStatus, TInt aReason)
	{
	//TODO: look our thread per handle
	*aStatus=aReason;
	gProcess.iThread.iRequestSemaphore.Signal();
	}




EXPORT_C void RThread::RequestComplete(TRequestStatus*& aStatus, TInt aReason) const
/**
Signals this thread that an asynchronous request originating from this thread,
is complete.

The request is associated with the specified request status object supplied
by this thread.

Typically, the caller of this function is the service provider responsible
for satisfying the request made by this thread.

The request is completed with the completion code passed in aReason. This
value is copied into this thread's request status, *aStatus, before signalling
this thread's request semaphore.

The meaning of the completion code is a matter of convention to be decided
between the service provider and this thread.

In a client-server situation, completion of a request takes place in the context
of the server thread, but the pointer is interpreted in the address space
of the client.

It is often the case in client-server situations that the client and the server
are in the same address space (i.e. the same process).

Setting the pointer to the request status to NULL is a convenience, not all
servers need it.

@param aStatus A reference to a pointer to the request status originally
               supplied by this thread. This is a pointer into this thread's
               address space, which may be different to the thread currently
               executing (this code). On return, the pointer to the request
               status is set to NULL.

@param aReason The completion code of this request.
*/
	{
	ExecRequestComplete(iHandle,aStatus,aReason);
	}



/**
Signal this threads request semaphore.

This is similar to RThread::RequestComplete() except that no TRequestStatus object
is modified.

May only be used to signal a thread in the same process as the callers.

@panic KERN-EXEC 46 if the thread is not in the same process as the callers
*/
EXPORT_C void RThread::RequestSignal() const
	{
	Exec::ThreadRequestSignal(iHandle);
	}



void ExitCurrentThread(TExitType aType, TInt aReason, const TDesC8* aCategory)
	{
	Exec::ThreadKill(KCurrentThreadHandle, aType, aReason, aCategory);
	}


//
#ifndef __GEN_USER_EXEC_CODE__



__EXECDECL__ void Exec::WaitForAnyRequest()
	{
	FAST_EXEC0(EFastExecWaitForAnyRequest);
	}

__EXECDECL__ RAllocator* Exec::Heap()
	{	
	//FAST_EXEC0(EFastExecHeap);
	return gProcess.iAllocator;
	}

__EXECDECL__ RAllocator* Exec::HeapSwitch(RAllocator*)
	{
	FAST_EXEC1(EFastExecHeapSwitch);
	}

__EXECDECL__ TTrapHandler* Exec::PushTrapFrame(TTrap* aTrap)
	{
	Panic(ESymcExecPanicNotUsed);
	return NULL;
	//FAST_EXEC1(EFastExecPushTrapFrame);
	//ASSERT(gProcess.iThread.iTrapCount<=KTrapStackSize);
	//gProcess.iThread.iTrapStack[gProcess.iThread.iTrapCount++]=aTrap;
	//return gProcess.iThread.iHandler;
	}

__EXECDECL__ TTrap* Exec::PopTrapFrame()
	{
	Panic(ESymcExecPanicNotUsed);
	return NULL;
	//FAST_EXEC0(EFastExecPopTrapFrame);
	//ASSERT(gProcess.iThread.iTrapCount>0);
	//return gProcess.iThread.iTrapStack[gProcess.iThread.iTrapCount--];
	}

__EXECDECL__ CActiveScheduler* Exec::ActiveScheduler()
	{
	//FAST_EXEC0(EFastExecActiveScheduler);
	return gProcess.iThread.iActiveScheduler;
	}

__EXECDECL__ void Exec::SetActiveScheduler(CActiveScheduler* aActiveScheduler)
	{
	//FAST_EXEC1(EFastExecSetActiveScheduler);
	gProcess.iThread.iActiveScheduler=aActiveScheduler;
	}

__EXECDECL__ TTimerLockSpec Exec::LockPeriod()
	{
	FAST_EXEC0(EFastExecLockPeriod);
	}

__EXECDECL__ TTrapHandler* Exec::TrapHandler()
	{
	//FAST_EXEC0(EFastExecTrapHandler);
	return gProcess.iThread.iHandler;
	}

__EXECDECL__ TTrapHandler* Exec::SetTrapHandler(TTrapHandler* aHandler)
	{	
	//FAST_EXEC1(EFastExecSetTrapHandler);
	TTrapHandler* prev=gProcess.iThread.iHandler;
	gProcess.iThread.iHandler=aHandler;
	return prev;
	}

__EXECDECL__ TUint32 Exec::DebugMask()
	{
	FAST_EXEC0(EFastExecDebugMask);
	}

__EXECDECL__ TUint32 Exec::DebugMaskIndex(TUint)
	{
	FAST_EXEC1(EFastExecDebugMaskIndex);
	}

__EXECDECL__ void Exec::SetDebugMask(TUint32)
	{
	FAST_EXEC1(EFastExecSetDebugMask);
	}

__EXECDECL__ TUint32 Exec::FastCounter()
	{
	FAST_EXEC0(EFastExecFastCounter);
	}

__EXECDECL__ TUint32 Exec::NTickCount()
	{
	FAST_EXEC0(EFastExecNTickCount);
	}

EXPORT_C __EXECDECL__ void UserSvr::LockRamDrive()
	{
	FAST_EXEC0(EFastExecLockRamDrive);
	}

EXPORT_C __EXECDECL__ void UserSvr::UnlockRamDrive()
	{
	FAST_EXEC0(EFastExecUnlockRamDrive);
	}

EXPORT_C __EXECDECL__ TLinAddr UserSvr::RomHeaderAddress()
	{
	FAST_EXEC0(EFastExecRomHeaderAddress);
	}

EXPORT_C __EXECDECL__ TLinAddr UserSvr::RomRootDirectoryAddress()
	{
	FAST_EXEC0(EFastExecRomRootDirectoryAddress);
	}

__EXECDECL__ void Exec::SetReentryPoint(TLinAddr)
	{
	FAST_EXEC1(EFastExecSetReentryPoint);
	}

__EXECDECL__ TUint32 Exec::KernelConfigFlags()
	{
	FAST_EXEC0(EFastExecKernelConfigFlags);
	}

__EXECDECL__ TInt Exec::UTCOffset()
	{
	FAST_EXEC0(EFastExecUTCOffset);
	}

__EXECDECL__ TInt Exec::GetGlobalUserData(TInt)
	{
	FAST_EXEC1(EFastExecGetGlobalUserData);
	}

EXPORT_C __EXECDECL__ TBool BTrace::CheckFilter(TUint32)
	{
	FAST_EXEC1(EFastExecCheckFilter);
	}

__EXECDECL__ TInt Exec::ObjectNext(TObjectType, TBuf8<KMaxFullName>&, TFindHandle&)
	{
	SLOW_EXEC3(EExecObjectNext);
	}

__EXECDECL__ TUint8* Exec::ChunkBase(TInt)
	{
	SLOW_EXEC1(EExecChunkBase);
	}

__EXECDECL__ TInt Exec::ChunkSize(TInt)
	{
	SLOW_EXEC1(EExecChunkSize);
	}

__EXECDECL__ TInt Exec::ChunkMaxSize(TInt)
	{
	SLOW_EXEC1(EExecChunkMaxSize);
	}

__EXECDECL__ TUint Exec::HandleAttributes(TInt)
	{
	SLOW_EXEC1(EExecHandleAttributes);
	}

__EXECDECL__ TUint Exec::TickCount()
	{
	SLOW_EXEC0(EExecTickCount);
	}

__EXECDECL__ void Exec::LogicalDeviceGetCaps(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecLogicalDeviceGetCaps);
	}

__EXECDECL__ TBool Exec::LogicalDeviceQueryVersionSupported(TInt, const TVersion&)
	{
	SLOW_EXEC2(EExecLogicalDeviceQueryVersionSupported);
	}

__EXECDECL__ TBool Exec::LogicalDeviceIsAvailable(TInt, TInt, const TDesC8*, const TDesC8*)
	{
	SLOW_EXEC4(EExecLogicalDeviceIsAvailable);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::LocaleExports(TAny*, TLibraryFunction*)
	{
	SLOW_EXEC2(EExecLocaleExports);
	}

__EXECDECL__ TInt Exec::ChannelRequest(TInt, TInt, TAny*, TAny*)
	{
	SLOW_EXEC4(EExecChannelRequest);
	}

__EXECDECL__ TUint32 Exec::MathRandom()
	{
	SLOW_EXEC0(EExecMathRandom);
	}

__EXECDECL__ void Exec::IMB_Range(TAny*, TUint)
	{
	SLOW_EXEC2(EExecIMBRange);
	}

__EXECDECL__ TInt Exec::ResetMachine(TMachineStartupType)
	{
	SLOW_EXEC1(EExecResetMachine);
	}

__EXECDECL__ TLibraryFunction Exec::LibraryLookup(TInt, TInt)
	{
	SLOW_EXEC2(EExecLibraryLookup);
	}

__EXECDECL__ void Exec::LibraryFileName(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecLibraryFileName);
	}

EXPORT_C __EXECDECL__ TInt UserSvr::ExecuteInSupervisorMode(TSupervisorFunction, TAny*)
	{
	SLOW_EXEC2(EExecExecuteInSupervisorMode);
	}

__EXECDECL__ void Exec::MutexWait(TInt)
	{
	SLOW_EXEC1(EExecMutexWait);
	}

__EXECDECL__ void Exec::MutexSignal(TInt)
	{
	SLOW_EXEC1(EExecMutexSignal);
	}

__EXECDECL__ TInt Exec::ProcessId(TInt)
	{
	SLOW_EXEC1(EExecProcessId);
	}

__EXECDECL__ void Exec::DllFileName(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecDllFileName);
	}

__EXECDECL__ void Exec::ProcessResume(TInt)
	{
	SLOW_EXEC1(EExecProcessResume);
	}

__EXECDECL__ void Exec::ProcessFileName(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecProcessFileName);
	}

__EXECDECL__ void Exec::ProcessCommandLine(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecProcessCommandLine);
	}

__EXECDECL__ TExitType Exec::ProcessExitType(TInt)
	{
	SLOW_EXEC1(EExecProcessExitType);
	}

__EXECDECL__ TInt Exec::ProcessExitReason(TInt)
	{
	SLOW_EXEC1(EExecProcessExitReason);
	}

__EXECDECL__ void Exec::ProcessExitCategory(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecProcessExitCategory);
	}

__EXECDECL__ TProcessPriority Exec::ProcessPriority(TInt)
	{
	SLOW_EXEC1(EExecProcessPriority);
	}

__EXECDECL__ TInt Exec::ProcessSetPriority(TInt, TProcessPriority)
	{
	SLOW_EXEC2(EExecProcessSetPriority);
	}

__EXECDECL__ TUint Exec::ProcessFlags(TInt)
	{
	SLOW_EXEC1(EExecProcessFlags);
	}

__EXECDECL__ void Exec::ProcessSetFlags(TInt, TUint, TUint)
	{
	SLOW_EXEC3(EExecProcessSetFlags);
	}

__EXECDECL__ TInt Exec::SemaphoreWait(TInt, TInt)
	{
	SLOW_EXEC2(EExecSemaphoreWait);
	}

__EXECDECL__ void Exec::SemaphoreSignal1(TInt)
	{
	SLOW_EXEC1(EExecSemaphoreSignal1);
	}

__EXECDECL__ void Exec::SemaphoreSignalN(TInt, TInt)
	{
	SLOW_EXEC2(EExecSemaphoreSignalN);
	}

__EXECDECL__ void Exec::ServerReceive(TInt, TRequestStatus&, TAny*)
	{
	SLOW_EXEC3(EExecServerReceive);
	}

__EXECDECL__ void Exec::ServerCancel(TInt)
	{
	SLOW_EXEC1(EExecServerCancel);
	}

__EXECDECL__ void Exec::SetSessionPtr(TInt, const TAny*)
	{
	SLOW_EXEC2(EExecSetSessionPtr);
	}

__EXECDECL__ TInt Exec::ThreadId(TInt)
	{
	SLOW_EXEC1(EExecThreadId);
	}

__EXECDECL__ TInt Exec::SessionShare(TInt&, TInt)
	{
	SLOW_EXEC2(EExecSessionShare);
	}

__EXECDECL__ void Exec::ThreadResume(TInt)
	{
	SLOW_EXEC1(EExecThreadResume);
	}

__EXECDECL__ void Exec::ThreadSuspend(TInt)
	{
	SLOW_EXEC1(EExecThreadSuspend);
	}

__EXECDECL__ TThreadPriority Exec::ThreadPriority(TInt)
	{
	SLOW_EXEC1(EExecThreadPriority);
	}

__EXECDECL__ void Exec::ThreadSetPriority(TInt, TThreadPriority)
	{
	SLOW_EXEC2(EExecThreadSetPriority);
	}

__EXECDECL__ TProcessPriority Exec::ThreadProcessPriority(TInt)
	{
	SLOW_EXEC1(EExecThreadProcessPriority);
	}

__EXECDECL__ void Exec::ThreadSetProcessPriority(TInt, TProcessPriority)
	{
	SLOW_EXEC2(EExecThreadSetProcessPriority);
	}

__EXECDECL__ TUint Exec::ThreadFlags(TInt)
	{
	SLOW_EXEC1(EExecThreadFlags);
	}

__EXECDECL__ void Exec::ThreadSetFlags(TInt, TUint, TUint)
	{
	SLOW_EXEC3(EExecThreadSetFlags);
	}

__EXECDECL__ TInt Exec::ThreadRequestCount(TInt)
	{
	SLOW_EXEC1(EExecThreadRequestCount);
	}

__EXECDECL__ TExitType Exec::ThreadExitType(TInt)
	{
	SLOW_EXEC1(EExecThreadExitType);
	}

__EXECDECL__ TInt Exec::ThreadExitReason(TInt)
	{
	SLOW_EXEC1(EExecThreadExitReason);
	}

__EXECDECL__ void Exec::ThreadExitCategory(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecThreadExitCategory);
	}

__EXECDECL__ void Exec::TimerCancel(TInt)
	{
	SLOW_EXEC1(EExecTimerCancel);
	}

__EXECDECL__ void Exec::TimerAfter(TInt, TRequestStatus&, TInt)
	{
	SLOW_EXEC3(EExecTimerAfter);
	}

__EXECDECL__ void Exec::TimerAt(TInt, TRequestStatus&, TUint32, TUint32)
	{
	SLOW_EXEC4(EExecTimerAt);
	}

__EXECDECL__ void Exec::TimerLock(TInt, TRequestStatus&, TTimerLockSpec)
	{
	SLOW_EXEC3(EExecTimerLock);
	}

__EXECDECL__ TInt Exec::ChangeNotifierLogon(TInt, TRequestStatus&)
	{
	SLOW_EXEC2(EExecChangeNotifierLogon);
	}

__EXECDECL__ TInt Exec::ChangeNotifierLogoff(TInt)
	{
	SLOW_EXEC1(EExecChangeNotifierLogoff);
	}

__EXECDECL__ void Exec::RequestSignal(TInt)
	{
	SLOW_EXEC1(EExecRequestSignal);
	}

__EXECDECL__ void Exec::HandleName(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecHandleName);
	}

__EXECDECL__ void Exec::HandleFullName(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecHandleFullName);
	}

__EXECDECL__ void Exec::HandleInfo(TInt, THandleInfo*)
	{
	SLOW_EXEC2(EExecHandleInfo);
	}

__EXECDECL__ void Exec::HandleCount(TInt, TInt&, TInt&)
	{
	SLOW_EXEC3(EExecHandleCount);
	}

__EXECDECL__ void Exec::After(TInt, TRequestStatus&)
	{
	SLOW_EXEC2(EExecAfter);
	}

__EXECDECL__ void Exec::At(const EXEC_TIME&, TRequestStatus&)
	{
	SLOW_EXEC2(EExecAt);
	}

__EXECDECL__ void Exec::MessageComplete(TInt, TInt)
	{
	SLOW_EXEC2(EExecMessageComplete);
	}

__EXECDECL__ void Exec::MessageCompleteWithHandle(TInt, TInt)
	{
	SLOW_EXEC2(EExecMessageCompleteWithHandle);
	}

__EXECDECL__ void Exec::TransferSession(TInt, TInt)
	{
	SLOW_EXEC2(EExecTransferSession);
	}

__EXECDECL__ TInt Exec::TimeNow(EXEC_TIME&, TInt&)
	{
	SLOW_EXEC2(EExecTimeNow);
	}

__EXECDECL__ TInt Exec::TimeNowSecure(EXEC_TIME&, TInt&)
	{
	SLOW_EXEC2(EExecTimeNowSecure);
	}

__EXECDECL__ TInt Exec::SetUTCTimeAndOffset(const EXEC_TIME&, TInt, TUint, TUint)
	{
	SLOW_EXEC4(EExecSetUTCTimeAndOffset);
	}

__EXECDECL__ TInt Exec::SetMachineConfiguration(const TDesC8&)
	{
	SLOW_EXEC1(EExecSetMachineConfiguration);
	}

__EXECDECL__ void Exec::CaptureEventHook()
	{
	SLOW_EXEC0(EExecCaptureEventHook);
	}

__EXECDECL__ void Exec::ReleaseEventHook()
	{
	SLOW_EXEC0(EExecReleaseEventHook);
	}

__EXECDECL__ void Exec::RequestEvent(TRawEventBuf&, TRequestStatus&)
	{
	SLOW_EXEC2(EExecRequestEvent);
	}

__EXECDECL__ void Exec::RequestEventCancel()
	{
	SLOW_EXEC0(EExecRequestEventCancel);
	}

__EXECDECL__ TInt Exec::AddEvent(const TRawEvent&)
	{
	SLOW_EXEC1(EExecAddEvent);
	}

__EXECDECL__ TAny* Exec::DllTls(TInt aHandle, TInt aDllUid)
	{
	//SLOW_EXEC2(EExecDllTls);
	if (aHandle==-1 && aDllUid==-1)
		{
		//No TGlobalDestructorFunc ATM
		return NULL;
		}
	else
		{
		//No sure what to do here
		__BREAKPOINT();
		}

	}

__EXECDECL__ TInt Exec::HalFunction(TInt, TInt, TAny*, TAny*)
	{
	SLOW_EXEC4(EExecHalFunction);
	}

__EXECDECL__ void Exec::WsRegisterThread()
	{
	SLOW_EXEC0(EExecWsRegisterThread);
	}

__EXECDECL__ void Exec::FsRegisterThread()
	{
	SLOW_EXEC0(EExecFsRegisterThread);
	}

__EXECDECL__ TInt Exec::ProcessCommandLineLength(TInt)
	{
	SLOW_EXEC1(EExecProcessCommandLineLength);
	}

__EXECDECL__ void Exec::TimerInactivity(TInt, TRequestStatus&, TInt)
	{
	SLOW_EXEC3(EExecTimerInactivity);
	}

__EXECDECL__ TInt Exec::UserInactivityTime()
	{
	SLOW_EXEC0(EExecUserInactivityTime);
	}

__EXECDECL__ void Exec::ResetInactivityTime()
	{
	SLOW_EXEC0(EExecResetInactivityTime);
	}

__EXECDECL__ void Exec::DebugPrint(TAny*, TInt)
	{
	SLOW_EXEC2(EExecDebugPrint);
	}

__EXECDECL__ TInt Exec::BreakPoint()
	{
	SLOW_EXEC0(EExecBreakPoint);
	}

__EXECDECL__ TInt Exec::ProfileStart(TInt)
	{
	SLOW_EXEC1(EExecProfileStart);
	}

__EXECDECL__ TInt Exec::ProfileEnd(TInt)
	{
	SLOW_EXEC1(EExecProfileEnd);
	}

__EXECDECL__ TExceptionHandler Exec::ExceptionHandler(TInt)
	{
	SLOW_EXEC1(EExecExceptionHandler);
	}

__EXECDECL__ TInt Exec::SetExceptionHandler(TInt, TExceptionHandler, TUint32)
	{
	SLOW_EXEC3(EExecSetExceptionHandler);
	}

__EXECDECL__ void Exec::ModifyExceptionMask(TInt, TUint32, TUint32)
	{
	SLOW_EXEC3(EExecModifyExceptionMask);
	}

__EXECDECL__ TInt Exec::RaiseException(TInt, TExcType)
	{
	SLOW_EXEC2(EExecRaiseException);
	}

__EXECDECL__ TInt Exec::IsExceptionHandled(TInt, TExcType, TBool)
	{
	SLOW_EXEC3(EExecIsExceptionHandled);
	}

__EXECDECL__ TInt Exec::ProcessGetMemoryInfo(TInt, TModuleMemoryInfo&)
	{
	SLOW_EXEC2(EExecProcessGetMemoryInfo);
	}

__EXECDECL__ TInt Exec::LibraryGetMemoryInfo(TInt, TModuleMemoryInfo&)
	{
	SLOW_EXEC2(EExecLibraryGetMemoryInfo);
	}

__EXECDECL__ TInt Exec::MachineConfiguration(TDes8&, TInt&)
	{
	SLOW_EXEC2(EExecMachineConfiguration);
	}

__EXECDECL__ TInt Exec::SetMemoryThresholds(TInt, TInt)
	{
	SLOW_EXEC2(EExecSetMemoryThresholds);
	}

__EXECDECL__ void Exec::LibraryType(TInt, TUidType&)
	{
	SLOW_EXEC2(EExecLibraryType);
	}

__EXECDECL__ void Exec::ProcessType(TInt, TUidType&)
	{
	SLOW_EXEC2(EExecProcessType);
	}

__EXECDECL__ TInt Exec::ChunkBottom(TInt)
	{
	SLOW_EXEC1(EExecChunkBottom);
	}

__EXECDECL__ TInt Exec::ChunkTop(TInt)
	{
	SLOW_EXEC1(EExecChunkTop);
	}

__EXECDECL__ void Exec::ThreadContext(TInt, TDes8&)
	{
	SLOW_EXEC2(EExecThreadContext);
	}

__EXECDECL__ TInt Exec::ThreadCreate(const TDesC8&, TOwnerType, SThreadCreateInfo8&)
	{
	SLOW_EXEC3(EExecThreadCreate);
	}

__EXECDECL__ TInt Exec::FindHandleOpen(TOwnerType, const TFindHandle&)
	{
	SLOW_EXEC2(EExecFindHandleOpen);
	}

__EXECDECL__ TInt Exec::HandleClose(TInt)
	{
	SLOW_EXEC1(EExecHandleClose);
	}

__EXECDECL__ TInt Exec::ChunkCreate(TOwnerType, const TDesC8*, TChunkCreate&)
	{
	SLOW_EXEC3(EExecChunkCreate);
	}

__EXECDECL__ TInt Exec::ChunkAdjust(TInt, TInt, TInt, TInt)
	{
	SLOW_EXEC4(EExecChunkAdjust);
	}

__EXECDECL__ TInt Exec::OpenObject(TObjectType, const TDesC8&, TOwnerType)
	{
	SLOW_EXEC3(EExecOpenObject);
	}

__EXECDECL__ TInt Exec::HandleDuplicate(TInt, TOwnerType, TInt&)
	{
	SLOW_EXEC3(EExecHandleDuplicate);
	}

__EXECDECL__ TInt Exec::MutexCreate(const TDesC8*, TOwnerType)
	{
	SLOW_EXEC2(EExecMutexCreate);
	}

__EXECDECL__ TInt Exec::SemaphoreCreate(const TDesC8* aName, TInt aCount, TOwnerType aType)
	{
	//SLOW_EXEC3(EExecSemaphoreCreate);
#ifdef _WINDOWS
	HANDLE semaphore = CreateSemaphore( 
		NULL,								// default security attributes
		aCount,
		KMaxTInt,
		//TODO: use the name
		NULL);								// unnamed mutex

	if (semaphore)
		{
		//success
		return (TInt)semaphore;
		}

	//failure
	return NULL;
#else
	//TODO: pthread implementation
	Panic(ESymcExecPanicNotSupported);
#endif
	}

__EXECDECL__ TInt Exec::ThreadOpenById(TUint, TOwnerType)
	{
	SLOW_EXEC2(EExecThreadOpenById);
	}

__EXECDECL__ TInt Exec::ProcessOpenById(TUint, TOwnerType)
	{
	SLOW_EXEC2(EExecProcessOpenById);
	}

__EXECDECL__ void Exec::ThreadKill(TInt aThreadHandle, TExitType aType, TInt aReason, const TDesC8* aCategory)
	{
	//SLOW_EXEC4(EExecThreadKill);
	if (aThreadHandle!=KCurrentThreadHandle)
		{
		//Not sure how to do that yet
		__BREAKPOINT();
		return;
		}

	if (aType==EExitPanic)
		{
		//Display message
#ifdef _WINDOWS
		TBuf8<256> buf;
		buf.Copy(*aCategory);	
		char errstr[256]; sprintf(errstr, "Category: %s\nReason: %d",buf.PtrZ(),aReason);
		MessageBoxA(NULL,errstr, "PANIC", MB_OK | MB_ICONERROR);	
#endif
		__BREAKPOINT();
		}
	
	exit(aType);
	}

__EXECDECL__ void Exec::ThreadLogon(TInt, TRequestStatus*, TBool)
	{
	SLOW_EXEC3(EExecThreadLogon);
	}

__EXECDECL__ TInt Exec::ThreadLogonCancel(TInt, TRequestStatus*, TBool)
	{
	SLOW_EXEC3(EExecThreadLogonCancel);
	}

__EXECDECL__ TInt Exec::DllSetTls(TInt, TInt, TAny*)
	{
	SLOW_EXEC3(EExecDllSetTls);
	}

__EXECDECL__ void Exec::DllFreeTls(TInt)
	{
	SLOW_EXEC1(EExecDllFreeTls);
	}

__EXECDECL__ TInt Exec::ThreadRename(TInt, const TDesC8&)
	{
	SLOW_EXEC2(EExecThreadRename);
	}

__EXECDECL__ TInt Exec::ProcessRename(TInt, const TDesC8&)
	{
	SLOW_EXEC2(EExecProcessRename);
	}

__EXECDECL__ void Exec::ProcessKill(TInt, TExitType, TInt, const TDesC8*)
	{
	SLOW_EXEC4(EExecProcessKill);
	}

__EXECDECL__ void Exec::ProcessLogon(TInt, TRequestStatus*, TBool)
	{
	SLOW_EXEC3(EExecProcessLogon);
	}

__EXECDECL__ TInt Exec::ProcessLogonCancel(TInt, TRequestStatus*, TBool)
	{
	SLOW_EXEC3(EExecProcessLogonCancel);
	}

__EXECDECL__ TInt Exec::ThreadProcess(TInt)
	{
	SLOW_EXEC1(EExecThreadProcess);
	}

__EXECDECL__ TInt Exec::ServerCreate(const TDesC8*, TInt)
	{
	SLOW_EXEC2(EExecServerCreate);
	}

__EXECDECL__ TInt Exec::ServerCreateWithOptions(const TDesC8*, TInt, TInt, TInt)
	{
	SLOW_EXEC4(EExecServerCreateWithOptions);
	}

__EXECDECL__ TInt Exec::SessionCreate(const TDesC8&, TInt, const TSecurityPolicy*, TInt)
	{
	SLOW_EXEC4(EExecSessionCreate);
	}

__EXECDECL__ TInt Exec::SessionCreateFromHandle(TInt, TInt, const TSecurityPolicy*, TInt)
	{
	SLOW_EXEC4(EExecSessionCreateFromHandle);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::DeviceLoad(TAny*, TInt)
	{
	SLOW_EXEC2(EExecDeviceLoad);
	}

__EXECDECL__ TInt Exec::DeviceFree(const TDesC8&, TInt)
	{
	SLOW_EXEC2(EExecDeviceFree);
	}

__EXECDECL__ TInt Exec::ChannelCreate(const TDesC8&, TChannelCreateInfo8&, TInt)
	{
	SLOW_EXEC3(EExecChannelCreate);
	}

__EXECDECL__ TInt Exec::TimerCreate()
	{
	SLOW_EXEC0(EExecTimerCreate);
	}

__EXECDECL__ void Exec::TimerHighRes(TInt, TRequestStatus&, TInt)
	{
	SLOW_EXEC3(EExecTimerHighRes);
	}

__EXECDECL__ void Exec::AfterHighRes(TInt, TRequestStatus&)
	{
	SLOW_EXEC2(EExecAfterHighRes);
	}

__EXECDECL__ TInt Exec::ChangeNotifierCreate(TOwnerType)
	{
	SLOW_EXEC1(EExecChangeNotifierCreate);
	}

__EXECDECL__ TInt Exec::UndertakerCreate(TOwnerType)
	{
	SLOW_EXEC1(EExecUndertakerCreate);
	}

__EXECDECL__ TInt Exec::UndertakerLogon(TInt, TRequestStatus&, TInt&)
	{
	SLOW_EXEC3(EExecUndertakerLogon);
	}

__EXECDECL__ TInt Exec::UndertakerLogonCancel(TInt)
	{
	SLOW_EXEC1(EExecUndertakerLogonCancel);
	}

__EXECDECL__ void Exec::KernelHeapDebug(TInt, TInt, TAny*)
	{
	SLOW_EXEC3(EExecKernelHeapDebug);
	}

__EXECDECL__ TInt Exec::ThreadGetCpuTime(TInt, EXEC_INT64&)
	{
	SLOW_EXEC2(EExecThreadGetCpuTime);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::CodeSegCreate(TCodeSegCreateInfo&)
	{
	SLOW_EXEC1(EExecCodeSegCreate);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::CodeSegLoaded(TCodeSegCreateInfo&)
	{
	SLOW_EXEC1(EExecCodeSegLoaded);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::LibraryCreate(TLibraryCreateInfo&)
	{
	SLOW_EXEC1(EExecLibraryCreate);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::CodeSegOpen(TAny*, TInt)
	{
	SLOW_EXEC2(EExecCodeSegOpen);
	}

EXPORT_C __EXECDECL__ void E32Loader::CodeSegClose(TAny*)
	{
	SLOW_EXEC1(EExecCodeSegClose);
	}

EXPORT_C __EXECDECL__ void E32Loader::CodeSegNext(TAny*&, const TFindCodeSeg&)
	{
	SLOW_EXEC2(EExecCodeSegNext);
	}

EXPORT_C __EXECDECL__ void E32Loader::CodeSegInfo(TAny*, TCodeSegCreateInfo&)
	{
	SLOW_EXEC2(EExecCodeSegInfo);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::CodeSegAddDependency(TAny*, TAny*)
	{
	SLOW_EXEC2(EExecCodeSegAddDependency);
	}

EXPORT_C __EXECDECL__ void E32Loader::CodeSegDeferDeletes()
	{
	SLOW_EXEC0(EExecCodeSegDeferDeletes);
	}

EXPORT_C __EXECDECL__ void E32Loader::CodeSegEndDeferDeletes()
	{
	SLOW_EXEC0(EExecCodeSegEndDeferDeletes);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::ProcessCreate(TProcessCreateInfo&, const TDesC8*)
	{
	SLOW_EXEC2(EExecProcessCreate);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::ProcessLoaded(TProcessCreateInfo&)
	{
	SLOW_EXEC1(EExecProcessLoaded);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::CheckClientState(TInt)
	{
	SLOW_EXEC1(EExecCheckLoaderClientState);
	}

EXPORT_C __EXECDECL__ TAny* E32Loader::ThreadProcessCodeSeg(TInt)
	{
	SLOW_EXEC1(EExecThreadProcessCodeSeg);
	}

EXPORT_C __EXECDECL__ void E32Loader::ReadExportDir(TAny*, TLinAddr*)
	{
	SLOW_EXEC2(EExecCodeSegReadExportDir);
	}

__EXECDECL__ TInt E32Loader::WaitDllLock()
	{
	SLOW_EXEC0(EExecWaitDllLock);
	}

__EXECDECL__ TInt E32Loader::ReleaseDllLock()
	{
	SLOW_EXEC0(EExecReleaseDllLock);
	}

__EXECDECL__ TInt E32Loader::LibraryAttach(TInt, TInt&, TLinAddr*)
	{
	SLOW_EXEC3(EExecLibraryAttach);
	}

__EXECDECL__ TInt E32Loader::LibraryAttached(TInt)
	{
	SLOW_EXEC1(EExecLibraryAttached);
	}

__EXECDECL__ TInt E32Loader::StaticCallList(TInt& aEntryPointCount, TLinAddr* /*aUnused*/)
	{
	//SLOW_EXEC2(EExecStaticCallList);
	//SL: We hijack this function for initializing our process see User::InitProcess
	aEntryPointCount=0; //Tell the caller we don't have any DLL entry point
	
	gProcess.CreateHeap();

	return KErrNone;
	}

__EXECDECL__ TInt E32Loader::LibraryDetach(TInt&, TLinAddr*)
	{
	SLOW_EXEC2(EExecLibraryDetach);
	}

__EXECDECL__ TInt E32Loader::LibraryDetached()
	{
	SLOW_EXEC0(EExecLibraryDetached);
	}

__EXECDECL__ TInt Exec::LastThreadHandle()
	{
	//SLOW_EXEC0(EExecLastThreadHandle);
	//Not sure what to do with that returning 0 seams appropriate for now
	return 0;
	}

__EXECDECL__ void Exec::ThreadRendezvous(TInt)
	{
	SLOW_EXEC1(EExecThreadRendezvous);
	}

__EXECDECL__ void Exec::ProcessRendezvous(TInt)
	{
	SLOW_EXEC1(EExecProcessRendezvous);
	}

__EXECDECL__ TInt Exec::MessageGetDesLength(TInt, TInt)
	{
	SLOW_EXEC2(EExecMessageGetDesLength);
	}

__EXECDECL__ TInt Exec::MessageGetDesMaxLength(TInt, TInt)
	{
	SLOW_EXEC2(EExecMessageGetDesMaxLength);
	}

__EXECDECL__ TInt Exec::MessageClient(TInt, TOwnerType)
	{
	SLOW_EXEC2(EExecMessageClient);
	}

__EXECDECL__ TInt Exec::MessageSetProcessPriority(TInt, TProcessPriority)
	{
	SLOW_EXEC2(EExecMessageSetProcessPriority);
	}

__EXECDECL__ void Exec::MessageConstructFromPtr(TInt, TAny*)
	{
	SLOW_EXEC2(EExecMessageConstructFromPtr);
	}

__EXECDECL__ void Exec::MessageKill(TInt, TExitType, TInt, const TDesC8*)
	{
	SLOW_EXEC4(EExecMessageKill);
	}

__EXECDECL__ TInt Exec::MessageOpenObject(TInt, TObjectType, TInt, TOwnerType)
	{
	SLOW_EXEC4(EExecMessageOpenObject);
	}

__EXECDECL__ void Exec::ProcessSecurityInfo(TInt, SSecurityInfo&)
	{
	SLOW_EXEC2(EExecProcessSecurityInfo);
	}

__EXECDECL__ void Exec::ThreadSecurityInfo(TInt, SSecurityInfo&)
	{
	SLOW_EXEC2(EExecThreadSecurityInfo);
	}

__EXECDECL__ void Exec::MessageSecurityInfo(TInt, SSecurityInfo&)
	{
	SLOW_EXEC2(EExecMessageSecurityInfo);
	}

__EXECDECL__ void Exec::CreatorSecurityInfo(SSecurityInfo&)
	{
	SLOW_EXEC1(EExecCreatorSecurityInfo);
	}

__EXECDECL__ void Exec::DisabledCapabilities(SCapabilitySet&)
	{
	SLOW_EXEC1(EExecDisabledCapabilities);
	}

__EXECDECL__ TInt Exec::ChunkSetRestrictions(TInt, TUint)
	{
	SLOW_EXEC2(EExecChunkSetRestrictions);
	}

__EXECDECL__ TInt Exec::MsgQueueCreate(const TDesC8*, TInt, TInt, TOwnerType)
	{
	SLOW_EXEC4(EExecMsgQueueCreate);
	}

__EXECDECL__ TInt Exec::MsgQueueSend(TInt, const TAny*, TInt)
	{
	SLOW_EXEC3(EExecMsgQueueSend);
	}

__EXECDECL__ TInt Exec::MsgQueueReceive(TInt, TAny*, TInt)
	{
	SLOW_EXEC3(EExecMsgQueueReceive);
	}

__EXECDECL__ void Exec::MsgQueueNotifySpaceAvailable(TInt, TRequestStatus&)
	{
	SLOW_EXEC2(EExecMsgQueueNotifySpaceAvailable);
	}

__EXECDECL__ void Exec::MsgQueueCancelSpaceAvailable(TInt)
	{
	SLOW_EXEC1(EExecMsgQueueCancelSpaceAvailable);
	}

__EXECDECL__ void Exec::MsgQueueNotifyDataAvailable(TInt, TRequestStatus&)
	{
	SLOW_EXEC2(EExecMsgQueueNotifyDataAvailable);
	}

__EXECDECL__ void Exec::MsgQueueCancelDataAvailable(TInt)
	{
	SLOW_EXEC1(EExecMsgQueueCancelDataAvailable);
	}

__EXECDECL__ TInt Exec::MsgQueueSize(TInt)
	{
	SLOW_EXEC1(EExecMsgQueueSize);
	}

__EXECDECL__ TInt Exec::PropertyDefine(TUint, TUint, TPropertyInfo*)
	{
	SLOW_EXEC3(EExecPropertyDefine);
	}

__EXECDECL__ TInt Exec::PropertyDelete(TUint, TUint)
	{
	SLOW_EXEC2(EExecPropertyDelete);
	}

__EXECDECL__ TInt Exec::PropertyAttach(TUint, TUint, TOwnerType)
	{
	SLOW_EXEC3(EExecPropertyAttach);
	}

__EXECDECL__ void Exec::PropertySubscribe(TInt, TRequestStatus*)
	{
	SLOW_EXEC2(EExecPropertySubscribe);
	}

__EXECDECL__ void Exec::PropertyCancel(TInt)
	{
	SLOW_EXEC1(EExecPropertyCancel);
	}

__EXECDECL__ TInt Exec::PropertyGetI(TInt, TInt*)
	{
	SLOW_EXEC2(EExecPropertyGetI);
	}

__EXECDECL__ TInt Exec::PropertyGetB(TInt, TUint8*, TInt)
	{
	SLOW_EXEC3(EExecPropertyGetB);
	}

__EXECDECL__ TInt Exec::PropertySetI(TInt, TInt)
	{
	SLOW_EXEC2(EExecPropertySetI);
	}

__EXECDECL__ TInt Exec::PropertySetB(TInt, const TUint8*, TInt)
	{
	SLOW_EXEC3(EExecPropertySetB);
	}

__EXECDECL__ TInt Exec::PropertyFindGetI(TUint, TUint, TInt*)
	{
	SLOW_EXEC3(EExecPropertyFindGetI);
	}

__EXECDECL__ TInt Exec::PropertyFindGetB(TUint, TUint, TUint8*, TInt)
	{
	SLOW_EXEC4(EExecPropertyFindGetB);
	}

__EXECDECL__ TInt Exec::PropertyFindSetI(TUint, TUint, TInt)
	{
	SLOW_EXEC3(EExecPropertyFindSetI);
	}

__EXECDECL__ TInt Exec::PropertyFindSetB(TUint, TUint, TUint8*, TInt)
	{
	SLOW_EXEC4(EExecPropertyFindSetB);
	}

__EXECDECL__ TInt Exec::PowerEnableWakeupEvents(TPowerState)
	{
	SLOW_EXEC1(EExecPowerEnableWakeupEvents);
	}

__EXECDECL__ void Exec::PowerDisableWakeupEvents()
	{
	SLOW_EXEC0(EExecPowerDisableWakeupEvents);
	}

__EXECDECL__ void Exec::PowerRequestWakeupEventNotification(TRequestStatus*)
	{
	SLOW_EXEC1(EExecPowerRequestWakeupEventNotification);
	}

__EXECDECL__ void Exec::PowerCancelWakeupEventNotification()
	{
	SLOW_EXEC0(EExecPowerCancelWakeupEventNotification);
	}

__EXECDECL__ TInt Exec::PowerDown()
	{
	SLOW_EXEC0(EExecPowerDown);
	}

__EXECDECL__ TInt Exec::ProcessSetHandleParameter(TInt, TInt, TInt)
	{
	SLOW_EXEC3(EExecProcessSetHandleParameter);
	}

__EXECDECL__ TInt Exec::ProcessSetDataParameter(TInt, TInt, const TUint8*, TInt)
	{
	SLOW_EXEC4(EExecProcessSetDataParameter);
	}

__EXECDECL__ TInt Exec::ProcessGetHandleParameter(TInt, TObjectType, TOwnerType)
	{
	SLOW_EXEC3(EExecProcessGetHandleParameter);
	}

__EXECDECL__ TInt Exec::ProcessGetDataParameter(TInt, TUint8*, TInt)
	{
	SLOW_EXEC3(EExecProcessGetDataParameter);
	}

__EXECDECL__ TInt Exec::ProcessDataParameterLength(TInt)
	{
	SLOW_EXEC1(EExecProcessDataParameterLength);
	}

__EXECDECL__ TUint Exec::MessageClientProcessFlags(TInt)
	{
	SLOW_EXEC1(EExecMessageClientProcessFlags);
	}

__EXECDECL__ TInt Exec::ThreadStackInfo(TInt, TThreadStackInfo&)
	{
	SLOW_EXEC2(EExecThreadStackInfo);
	}

__EXECDECL__ RAllocator* Exec::ThreadGetHeap(TInt)
	{
	SLOW_EXEC1(EExecThreadGetHeap);
	}

__EXECDECL__ TInt Exec::ThreadAsProcess(TInt, TInt)
	{
	SLOW_EXEC2(EExecThreadAsProcess);
	}

__EXECDECL__ TInt Exec::CondVarCreate(const TDesC8*, TOwnerType)
	{
	SLOW_EXEC2(EExecCondVarCreate);
	}

__EXECDECL__ TInt Exec::CondVarWait(TInt, TInt, TInt)
	{
	SLOW_EXEC3(EExecCondVarWait);
	}

__EXECDECL__ void Exec::CondVarSignal(TInt)
	{
	SLOW_EXEC1(EExecCondVarSignal);
	}

__EXECDECL__ void Exec::CondVarBroadcast(TInt)
	{
	SLOW_EXEC1(EExecCondVarBroadcast);
	}

__EXECDECL__ TInt Exec::PlatSecDiagnostic(TPlatSecDiagnostic*)
	{
	SLOW_EXEC1(EExecPlatSecDiagnostic);
	}

__EXECDECL__ TLinAddr Exec::ExceptionDescriptor(TLinAddr)
	{
	SLOW_EXEC1(EExecExceptionDescriptor);
	}

__EXECDECL__ void Exec::ThreadRequestSignal(TInt)
	{
	//SLOW_EXEC1(EExecThreadRequestSignal);
	//TODO: look our thread per handle
	gProcess.iThread.iRequestSemaphore.Signal();
	}

__EXECDECL__ TBool Exec::MutexIsHeld(TInt)
	{
	SLOW_EXEC1(EExecMutexIsHeld);
	}

__EXECDECL__ TTrapHandler* Exec::LeaveStart()
	{
	//SLOW_EXEC0(EExecLeaveStart);
	return gProcess.iThread.iHandler;
	}

__EXECDECL__ void Exec::LeaveEnd()
	{
	//SLOW_EXEC0(EExecLeaveEnd);	
	}

__EXECDECL__ void Exec::SetDebugMaskIndex(TUint32, TUint)
	{
	SLOW_EXEC2(EExecSetDebugMaskIndex);
	}

__EXECDECL__ TInt Exec::GetModuleNameFromAddress(TAny*, TDes8&)
	{
	SLOW_EXEC2(EExecGetModuleNameFromAddress);
	}

__EXECDECL__ void Exec::NotifyChanges(TUint)
	{
	SLOW_EXEC1(EExecNotifyChanges);
	}

__EXECDECL__ TInt Exec::SetGlobalUserData(TInt, TInt)
	{
	SLOW_EXEC2(EExecSetGlobalUserData);
	}

__EXECDECL__ TInt Exec::SessionSecurityInfo(TInt, SSecurityInfo&)
	{
	SLOW_EXEC2(EExecSessionSecurityInfo);
	}

__EXECDECL__ const TRequestStatus* Exec::MessageClientStatus(TInt)
	{
	SLOW_EXEC1(EExecMessageClientStatus);
	}

__EXECDECL__ TInt Exec::SetFloatingPointMode(TFloatingPointMode, TFloatingPointRoundingMode)
	{
	SLOW_EXEC2(EExecSetFloatingPointMode);
	}

EXPORT_C __EXECDECL__ TBool BTrace::CheckFilter2(TUint32, TUint32)
	{
	SLOW_EXEC2(EExecCheckFilter2);
	}

__EXECDECL__ TAny* Exec::ProcessExeExportData()
	{
	SLOW_EXEC0(EExecProcessExeExportData);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::NotifyIfCodeSegDestroyed(TRequestStatus&)
	{
	SLOW_EXEC1(EExecNotifyIfCodeSegDestroyed);
	}

EXPORT_C __EXECDECL__ TInt E32Loader::GetDestroyedCodeSegInfo(TCodeSegLoaderCookie&)
	{
	SLOW_EXEC1(EExecGetDestroyedCodeSegInfo);
	}

EXPORT_C __EXECDECL__ TInt Exec::SetWin32RuntimeHook(TAny*)
	{
	SLOW_EXEC1(EExecSetWin32RuntimeHook);
	}

__EXECDECL__ TInt Exec::GetBTraceId(TInt)
	{
	SLOW_EXEC1(EExecGetBTraceId);
	}

__EXECDECL__ void Exec::NotifyOnIdle(TRequestStatus*)
	{
	SLOW_EXEC1(EExecNotifyOnIdle);
	}

__EXECDECL__ void Exec::CancelMiscNotifier(TRequestStatus*)
	{
	SLOW_EXEC1(EExecCancelMiscNotifier);
	}

__EXECDECL__ void Exec::NotifyObjectDestruction(TInt, TRequestStatus*)
	{
	SLOW_EXEC2(EExecNotifyObjectDestruction);
	}

__EXECDECL__ void Exec::RegisterTrustedChunk(TInt)
	{
	SLOW_EXEC1(EExecRegisterTrustedChunk);
	}

/*
Notify the kernel a thread is exiting.

@param Exit reason
@return ETrue if this is the last thread in the process, EFalse otherwise.
*/
__EXECDECL__ TBool Exec::UserThreadExiting(TInt aReason)
	{
	//SLOW_EXEC1(EExecUserThreadExiting);
	gProcess.Free();

	return ETrue; //No thread support ATM so we are always the last thread
	}

__EXECDECL__ TBool Exec::ChunkIsPaged(TInt)
	{
	SLOW_EXEC1(EExecChunkIsPaged);
	}

__EXECDECL__ TBool Exec::ProcessDefaultDataPaged(TInt)
	{
	SLOW_EXEC1(EExecProcessDefaultDataPaged);
	}

__EXECDECL__ TUint Exec::MessageClientThreadFlags(TInt)
	{
	SLOW_EXEC1(EExecMessageClientThreadFlags);
	}

__EXECDECL__ TInt Exec::ShPoolCreate(const TShPoolInfo&, TUint)
	{
	SLOW_EXEC2(EExecShPoolCreate);
	}

__EXECDECL__ TInt Exec::ShPoolAlloc(TInt, TUint, SShBufBaseAndSize&)
	{
	SLOW_EXEC3(EExecShPoolAlloc);
	}

__EXECDECL__ void Exec::ShPoolGetInfo(TInt, TShPoolInfo&)
	{
	SLOW_EXEC2(EExecShPoolGetInfo);
	}

__EXECDECL__ TUint Exec::ShPoolFreeCount(TInt)
	{
	SLOW_EXEC1(EExecShPoolFreeCount);
	}

__EXECDECL__ TInt Exec::ShPoolNotification(TInt, TShPoolNotifyType, TUint, TRequestStatus&)
	{
	SLOW_EXEC4(EExecShPoolNotification);
	}

__EXECDECL__ TInt Exec::ShPoolNotificationCancel(TInt, TShPoolNotifyType, TRequestStatus&)
	{
	SLOW_EXEC3(EExecShPoolNotificationCancel);
	}

__EXECDECL__ TInt Exec::ShPoolBufferWindow(TInt, TInt, TBool)
	{
	SLOW_EXEC3(EExecShPoolBufferWindow);
	}

__EXECDECL__ TInt Exec::ShBufMap(TInt, TBool, SShBufBaseAndSize&)
	{
	SLOW_EXEC3(EExecShBufMap);
	}

__EXECDECL__ TInt Exec::ShBufUnMap(TInt)
	{
	SLOW_EXEC1(EExecShBufUnMap);
	}

__EXECDECL__ TInt Exec::ShBufBaseAndSize(TInt, SShBufBaseAndSize&)
	{
	SLOW_EXEC2(EExecShBufBaseAndSize);
	}



#endif


