// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\win32\ncthrd.cpp
//
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"
#include <emulator.h>

#if		defined(__CW32__) && defined(__MWERKS__) && (__MWERKS__ < 0x3200)
// Early versions didn't support try/except :(
#error	"This compiler is no longer supported, because it doesn't provide C++ exception generation and handling"
#endif

extern "C" void ExcFault(TAny*);

// initial Win32 thread stack size
const TInt KInitialStackSize = 0x1000;

// maximum size of the parameter block passed to a new thread
const TInt KMaxParameterBlock = 512;

// data passed to new thread to enable hand-off of the parameter block
struct SCreateThread
	{
	const SNThreadCreateInfo* iInfo;
	NFastMutex iHandoff;
	};


/** Set some global properties of the emulator
	Called by the Win32 base port during boot.

	@param	aTrace TRUE means trace Win32 thread ID for every thread created
	@param	aSingleCpu TRUE means lock the emulator process to a single CPU

	@internalTechnology
 */
EXPORT_C void NThread::SetProperties(TBool aTrace, TInt aSingleCpu)
	{
	Win32TraceThreadId = aTrace;
	Win32SingleCpu = aSingleCpu;
	}



void NThread__HandleException(TWin32ExcInfo aExc)
//
// Final stage NKern exception handler.
//
// The first stage of exception processing (in ExceptionHandler()) entered the
// kernel and locked it, so we have to undo those two operations before returning.
// However, if the kernel was already locked when the exception occurred, it is
// a fatal condition and the system will be faulted.
//
// Note that the parameter struct is passed by value, this allows for direct
// access to the exception context created on the call stack by NThread::Exception().
//
	{
	NKern::Unlock();
	if (TheScheduler.iKernCSLocked)
		ExcFault(&aExc);

	// Complete the exception data. Note that the call to EnterKernel() in
	// ExceptionHandler() will have incremented iInKernel after the exception
	// occurred.
	NThread* me = static_cast<NThread*>(TheScheduler.iCurrentThread);
	__NK_ASSERT_DEBUG(me->iInKernel);
	aExc.iFlags = me->iInKernel == 1 ? 0 : TWin32ExcInfo::EExcInKernel;
	aExc.iHandler = NULL;

	// run NThread exception handler in 'kernel' mode
	me->iHandlers->iExceptionHandler(&aExc, me);
	LeaveKernel();

	// If a 'user' handler is set by the kernel handler, run it
	if (aExc.iHandler)
		aExc.iHandler(aExc.iParam[0], aExc.iParam[1]);
	}

__NAKED__ void NThread::Exception()
//
// Trampoline to nKern exception handler
// Must preserve all registers in the structure defined by TWin32Exc
//
// This is an intermediate layer, to which control has been diverted by
// NThread::ExceptionHandler(). It constructs a TWin32Exc structure on
// the stack and passes it NThread__ExceptionHandler().
//
// At this point we are no longer in Win32 exception context.
//
	{
	// this is the TWin32Exc structure
	__asm push Win32ExcAddress			// save return address followed by EBP first to help debugger
	__asm push ebp
	__asm mov ebp, esp
	__asm push cs
	__asm pushfd
	__asm push gs
	__asm push fs
	__asm push es
	__asm push ds
	__asm push ss
	__asm push edi
	__asm push esi
	__asm lea esi, [ebp+8]
	__asm push esi						// original esp
	__asm push ebx
	__asm push edx
	__asm push ecx
	__asm push eax
	__asm push Win32ExcDataAddress
	__asm push Win32ExcCode
	__asm sub esp, 20					// struct init completed by NThread__HandleException()

	__asm call NThread__HandleException

	__asm add esp, 28
	__asm pop eax
	__asm pop ecx
	__asm pop edx
	__asm pop ebx
	__asm pop esi						// original ESP - ignore
	__asm pop esi
	__asm pop edi
	__asm pop ebp						// original SS - ignore
	__asm pop ds
	__asm pop es
	__asm pop fs
	__asm pop gs
	__asm popfd
	__asm pop ebp						// original CS - ignore
	__asm pop ebp
	__asm ret
	}

// From e32/commmon/win32/seh.cpp
extern DWORD CallFinalSEHHandler(EXCEPTION_RECORD* aException, CONTEXT* aContext);

extern void DivertHook();

DWORD NThread::ExceptionHandler(EXCEPTION_RECORD* aException, CONTEXT* aContext)
//
// Win32 exception handler for EPOC threads
//
// This is the outermost wrapper, called from ExceptionFilter() of via manual
// interception of the Win32 exception mechanism if using a really old compiler
//
	{
	if (aException->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
		// Hardcoded breakpoint
		//
		// Jump directly to NT's default unhandled exception handler which will
		// either display a dialog, directly invoke the JIT debugger or do nothing
		// dependent upon the AeDebug and ErrorMode registry settings.
		//
		// Note this handler is always installed on the SEH chain and is always
		// the last handler on this chain, as it is installed by NT in kernel32.dll
		// before invoking the Win32 thread function.
		return CallFinalSEHHandler(aException, aContext);
		}

	// deal with conflict between preemption and diversion
	// the diversion will have been applied to the pre-exception context, not
	// the current context, and thus will get 'lost'. Wake-up of a pre-empted
	// thread with a diversion will not unlock the kernel, so we need to deal
	// with the possibility that the kernel may be locked if a diversion exists
	NThread& me = *static_cast<NThread*>(TheScheduler.iCurrentThread);
	if (me.iDiverting && me.iDivertFn)
		{
		// The thread is being forced to exit - run the diversion outside of Win32 exception handler
		__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
		aContext->Eip = (TUint32)&DivertHook;
		}
	else
		{
		if (me.iDiverting)
			{
			// The thread is being prodded to pick up its callbacks.  This will happen when the
			// exception handler calls LeaveKernel(), so we can remove the diversion
			__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
			if (aException->ExceptionAddress == &DivertHook)
				aException->ExceptionAddress = me.iDivertReturn;
			me.iDivertReturn = NULL;
			me.iDiverting = EFalse;
			EnterKernel(TRUE);
			}
		else
			{
			EnterKernel();
			TheScheduler.iKernCSLocked = 1;	// prevent pre-emption
			}

		// If the kernel was already locked, this will be detected in the next stage handler
		// (NThread::Exception()), which we arrange to run outside the Win32 exception context
		Win32ExcAddress = aException->ExceptionAddress;
		Win32ExcDataAddress = (TAny*)aException->ExceptionInformation[1];
		Win32ExcCode = aException->ExceptionCode;
		aContext->Eip = (TUint32)&Exception;
		}

	return ExceptionContinueExecution;
	}

LONG WINAPI NThread::ExceptionFilter(EXCEPTION_POINTERS* aExc)
//
// Filter wrapper for main Win32 exception handler
//
	{
	LONG ret = EXCEPTION_CONTINUE_SEARCH;

	switch (ExceptionHandler(aExc->ExceptionRecord, aExc->ContextRecord))
		{
	case ExceptionContinueExecution:
		ret = EXCEPTION_CONTINUE_EXECUTION;
		break;
		
	case ExceptionContinueSearch:
	default:
		break;
		}

	return ret;
	}


DWORD WINAPI NThread::StartThread(LPVOID aParam)
//
// Win32 thread function for nKern threads.
//
// The thread first enters this function after the nScheduler has resumed
// it, following the context switch induced by the hand-off mutex.
//
// The parameter block for this thread needs to be copied into its
// own context, before releasing the mutex and handing control back to
// the creating thread.
//
	{
	SCreateThread* init = static_cast<SCreateThread*>(aParam);
	NThread& me = *static_cast<NThread*>(init->iHandoff.iHoldingThread);
	me.iWinThreadId = GetCurrentThreadId();
	SchedulerRegister(me);
#ifdef BTRACE_FAST_MUTEX
	BTraceContext4(BTrace::EFastMutex, BTrace::EFastMutexWait, &init->iHandoff);
#endif
	NKern::Unlock();

	// intercept win32 exceptions in a debuggabble way
	__try
		{
		// save the thread entry point and parameter block
		const SNThreadCreateInfo& info = *init->iInfo;
		NThreadFunction threadFunction = info.iFunction;
		TUint8 parameterBlock[KMaxParameterBlock];
		TAny* parameter = (TAny*)info.iParameterBlock;

		if (info.iParameterBlockSize)
			{
			__NK_ASSERT_DEBUG(TUint(info.iParameterBlockSize) <= TUint(KMaxParameterBlock));
			memcpy(parameterBlock, info.iParameterBlock, info.iParameterBlockSize);
			parameter = parameterBlock;
			}


		// Calculate stack base
		me.iUserStackBase = (((TLinAddr)&parameterBlock) + 0xfff) & ~0xfff;

		// some useful diagnostics for debugging
		if (Win32TraceThreadId)
			KPrintf("Thread %T created @ 0x%x - Win32 Thread ID 0x%x", init->iHandoff.iHoldingThread, init->iHandoff.iHoldingThread, GetCurrentThreadId());

#ifdef MONITOR_THREAD_CPU_TIME
		me.iLastStartTime = 0;  // Don't count NThread setup in cpu time
#endif

		// start-up complete, release the handoff mutex, which will re-suspend us
		NKern::FMSignal(&init->iHandoff);

		// thread has been resumed: invoke the thread function
		threadFunction(parameter);
		}
	__except (ExceptionFilter(GetExceptionInformation()))
		{
		// Do nothing - filter does all the work and hooks into EPOC
		// h/w exception mechanism if necessary by thread diversion
		}

	NKern::Exit();
	return 0;
	}



/**
 * Set the Win32 thread priority based on the thread type.
 * Interrupt/Event threads must be able to preempt normal
 * nKern threads, so they get a higher (Win32) priority.
 */
static void SetPriority(HANDLE aThread, TEmulThreadType aType)
	{
	TInt p;

	switch (aType)
		{
	default:
		FAULT();

	case EThreadEvent:
		p = THREAD_PRIORITY_ABOVE_NORMAL;
		break;

	case EThreadNKern:
		p = THREAD_PRIORITY_NORMAL;
		break;
		}

	CheckedSetThreadPriority(aThread, p);
	SetThreadPriorityBoost(aThread, TRUE);		// disable priority boost (for NT)
	}

/**	Create a Win32 thread for use in the emulator.

	@param	aType Type of thread (Event or NKern) - determines Win32 priority
	@param	aThreadFunc Entry point of thread
	@param	aPtr Argument passed to entry point
	@param	aRun TRUE if thread should be resumed immediately
	@return	The Win32 handle to the thread, 0 if an error occurred

	@pre    Call either in thread context.
	@pre	Do not call from bare Win32 threads.

	@see TEmulThreadType
 */
EXPORT_C HANDLE CreateWin32Thread(TEmulThreadType aType, LPTHREAD_START_ROUTINE aThreadFunc, LPVOID aPtr, TBool aRun)
	{
	__NK_ASSERT_DEBUG(!TheScheduler.iCurrentThread || NKern::CurrentContext() == NKern::EThread);

	__LOCK_HOST;
	
	DWORD id;
	HANDLE handle = CreateThread(NULL , KInitialStackSize, aThreadFunc, aPtr, CREATE_SUSPENDED, &id);
	if (handle)
		{
		SetPriority(handle, aType);
		if (aRun)
			ResumeThread(handle);
		}
	return handle;
	}

static HANDLE InitThread()
//
// Set up the initial thread and return the thread handle
//
	{
	HANDLE p = GetCurrentProcess();
	HANDLE me;
	DWORD r = DuplicateHandle(p, GetCurrentThread(), p, &me, 0, FALSE, DUPLICATE_SAME_ACCESS);
	__NK_ASSERT_ALWAYS(r != 0);						// r is zero on error
	SetPriority(me, EThreadNKern);
	return me;
	}

TInt NThread::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	iWinThread = NULL;
	iWinThreadId = 0;
	iScheduleLock = NULL;
	iInKernel = 1;
	iDivertFn = NULL;
	iWakeup = aInitial ? ERelease : EResumeLocked;	// mark new threads as created (=> win32 suspend)

	TInt r = NThreadBase::Create(aInfo, aInitial);
	if (r != KErrNone)
		return r;

	// the rest has to be all or nothing, we must complete it
	iScheduleLock = CreateEventA(NULL, FALSE, FALSE, NULL);
	if (iScheduleLock == NULL)
		return Emulator::LastError();

	if (aInitial)
		{
		iWinThread = InitThread();
		FastCounterInit();
#ifdef MONITOR_THREAD_CPU_TIME
		iLastStartTime = NKern::FastCounter();
#endif
		iUserStackBase = (((TLinAddr)&r) + 0xfff) & ~0xfff; // base address of stack
		SchedulerInit(*this);
		return KErrNone;
		}

	// create the thread proper
	//
	SCreateThread start;
	start.iInfo = &aInfo;
	iWinThread = CreateWin32Thread(EThreadNKern, &StartThread, &start, FALSE);
	if (iWinThread == NULL)
		{
		r = Emulator::LastError();
		CloseHandle(iScheduleLock);
		return r;
		}

#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification, BTrace::ENanoThreadCreate, this);
#endif
	// switch to the new thread to hand over the parameter block
	NKern::Lock();
	ForceResume();	// mark the thread as ready
	// give the thread ownership of the handoff mutex
	start.iHandoff.iHoldingThread = this;
	iHeldFastMutex = &start.iHandoff;
	Suspend(1);		// will defer as holding a fast mutex (implicit critical section)
	// do the hand-over
	start.iHandoff.Wait();
	start.iHandoff.Signal();
	NKern::Unlock();

	return KErrNone;
	}



void NThread::Diverted()
//
// This function is called in the context of a thread that is being diverted.
// This can be for either of two reasons: if iDivertFn has been set, that
// function will be called and is not expected to return i.e.  it should force
// the thread to exit (in this case, the thread may already be in the kernel,
// but only at a place where it's safe for it to be killed).  Otherwise, the
// thread must be in user mode, and it's forced to make a null trip through
// the kernel, causing it to run pending user-mode callbacks on the way out.
//
// On entry, the kernel is locked and interrupts enabled
//
	{
	NThread& me = *static_cast<NThread*>(TheScheduler.iCurrentThread);
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
	__NK_ASSERT_ALWAYS(me.iDiverting);
	NThread::TDivert divertFn = me.iDivertFn;
	me.iDivertFn = NULL;
	me.iDivertReturn = NULL;
	me.iDiverting = EFalse;

	EnterKernel(TRUE);

	if (divertFn)
		divertFn();					// does not return

	NKern::Unlock();
	LeaveKernel();
	}

void NThread__Diverted()
	{
	NThread::Diverted();
	}

__NAKED__ void DivertHook()
	{
	// The stack frame is set up like this:
	//
	//		| return address |
	//		| frame pointer  |
	//		| flags			 |
	//		| saved eax		 |
	//		| saved ecx		 |
	//      | saved edx		 |
	//
	__asm push eax					// reserve word for return address
	__asm push ebp
	__asm mov ebp, esp
	__asm pushfd
	__asm push eax
	__asm push ecx
	__asm push edx
	__asm mov eax, [TheScheduler.iCurrentThread]
	__asm mov eax, [eax]NThread.iDivertReturn
	__asm mov [esp + 20], eax		// store return address
	__asm call NThread__Diverted
	__asm pop edx
	__asm pop ecx
	__asm pop eax
	__asm popfd
	__asm pop ebp
	__asm ret
	}


void NThread::ApplyDiversion()
//
// Arrange that the thread will be diverted when next it runs.
// This can be for either of two reasons: if iDivertFn has been set,
// that function will be called and is not expected to return i.e.
// it should force the thread to exit. Otherwise, the thread will
// make a null trip through the kernel, causing it to run pending
// user-mode callbacks on the way out.
//
// This uses the Win32 CONTEXT functions to change the thread's PC
// so that execution restarts at DivertHook ...
//
	{
	// Called with interrupts disabled and kernel locked
	__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked == 1);
	__NK_ASSERT_ALWAYS(iDivertReturn == NULL || iDiverting);

	if (iDiverting)
		return;

	CONTEXT c;
	c.ContextFlags = CONTEXT_CONTROL;
	CheckedGetThreadContext(iWinThread, &c);
	iDivertReturn = (TAny*)c.Eip;
	c.Eip = (TUint32)&DivertHook;
	c.ContextFlags = CONTEXT_CONTROL;
	CheckedSetThreadContext(iWinThread, &c);
	iDiverting = ETrue;
	}

void NThread::Divert(TDivert aDivertFn)
//
// Arrange that the thread will exit by calling aDivertFn when next
// it runs.  The diversion function will be called with the kernel
// locked and interrupts enabled.  It is not expected to return.
//
	{
	iDivertFn = aDivertFn;
	if (iWakeup == EResume)
		iWakeup = EResumeDiverted;
	else
		__NK_ASSERT_ALWAYS(iWakeup == ERelease);
	}

void NThread::ExitSync()
//
// Diversion used to terminate 'stillborn' threads.
// On entry, kernel is locked, interrupts are enabled and we hold an interlock mutex
//
	{
	NThreadBase& me = *TheScheduler.iCurrentThread;
	me.iHeldFastMutex->Signal();	// release the interlock
	me.iNState = EDead;				// mark ourselves as dead which will take thread out of scheduler
	TheScheduler.Remove(&me);
	RescheduleNeeded();
	TScheduler::Reschedule();	// this won't return
	FAULT();
	}

void NThread::Stillborn()
//
// Called if the new thread creation was aborted - so it will not be killed in the usual manner
//
// This function needs to exit the thread synchronously as on return we will destroy the thread control block
// Thus we need to use an interlock that ensure that the target thread runs the exit handler before we continue
//
	{
	// check if the Win32 thread was created
	if (!iWinThread)
		return;

	NKern::Lock();
	Divert(&ExitSync);
	ForceResume();
	// create and assign mutex to stillborn thread
	NFastMutex interlock;
	interlock.iHoldingThread = this;
	iHeldFastMutex = &interlock;
	interlock.Wait();			// interlock on thread exit handler
	interlock.Signal();
	NKern::Unlock();
	}

void NThread::ExitAsync()
//
// Diversion used to terminate 'killed' threads.
// On entry, kernel is locked and interrupts are enabled
//
	{
	NThreadBase& me = *TheScheduler.iCurrentThread;
	__NK_ASSERT_ALWAYS(static_cast<NThread&>(me).iInKernel > 0);
	me.iCsCount = 0;
	me.Exit();
	}

inline void NThread::DoForceExit()
	{
	__NK_ASSERT_DEBUG(TheScheduler.iKernCSLocked);
	Divert(&ExitAsync);
	}

void NThreadBase::ForceExit()
//
// Called to force the thread to exit when it resumes
//
	{
	static_cast<NThread*>(this)->DoForceExit();
	}

void NThreadBase::OnExit()
	{
	}

void NThreadBase::OnKill()
	{
	}

