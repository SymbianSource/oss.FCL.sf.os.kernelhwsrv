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
// e32\include\nkern\win32\nk_plat.h
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef	__NK_WIN32_H__
#define	__NK_WIN32_H__

#define	_CRTIMP			// we want to use the win32 static runtime library

#define	WIN32_LEAN_AND_MEAN
#define	_WIN32_WINNT	0x0400
#include <windows.h>

typedef void (*TExcHandler)(TAny*, TAny*);

struct TWin32ExcInfo
	{
	enum { EExcInKernel = 0x1 };
public:
	TExcHandler iHandler;
	TAny* iParam[2];
	TUint iFlags;
	// ExcType, iExcId, iFaultAddress, iEax, iEcx, iEdx, iEbx, iEsp, iEbp, iEsi, iEdi,
	// iSs, iDs, iEs, iFs, iGs, iEflags, iEip, iCs
	TUint32	iExcType;	// filled in by EPOC
	TUint32 iExcCode;
	TUint32 iExcDataAddress;
	TUint32 iEax;
	TUint32 iEcx;
	TUint32 iEdx;
	TUint32 iEbx;
	TUint32 iEsp;
	TUint32 iEsi;
	TUint32 iEdi;
	TUint32 iSs;
	TUint32 iDs;
	TUint32 iEs;
	TUint32 iFs;
	TUint32 iGs;
	TUint32 iEflags;
	TUint32 iCs;
	TUint32 iEbp;
	TUint32 iEip;
	};

enum TEmulThreadType
	{
	EThreadEvent,	// an 'interrupt' thread, interacts with Win32 events
	EThreadNKern	// an nKern thread, identified by an NThread control block
	};

class NThread : public NThreadBase
	{
public:
	typedef void (*TDivert)();
	enum TWakeup { ERelease, EResume, EResumeLocked, EIdle, EEscaped, EResumeDiverted };
	enum TSelectCpu { ECpuAll = -1, ECpuSingle = -2 };

public:
	TInt Create(SNThreadCreateInfo& aInfo, TBool aInitial);
	void Stillborn();
	void DoForceExit();

	IMPORT_C static void Idle();
	IMPORT_C static void SetProperties(TBool aTrace, TInt aCpu);
	TBool WakeUp();
	TBool IsSafeToPreempt();
	void Divert(TDivert aDivert);
	void ApplyDiversion();

private:
	static DWORD WINAPI StartThread(LPVOID aParam);

	static void ExitSync();
	static void ExitAsync();

	static void Exception();
	static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* aExc);

public:
	static void Diverted();
	// Has to be accessible to code user-side via emulator.lib
	static DWORD ExceptionHandler(EXCEPTION_RECORD* aException, CONTEXT* aContext);

public:
	HANDLE iWinThread;
	DWORD iWinThreadId;
	HANDLE iScheduleLock;		// event used for scheduling interlock
	volatile TInt iInKernel;	// flag to indicate if the thread is running 'in' the kernel
	TWakeup iWakeup;			// indicates how to wake up the thread
	TBool iDiverting;			// flag to indicate that the thread is being diverted
	TDivert iDivertFn;			// function to invoke after reschedule, may be null
	TAny* iDivertReturn;    	// return address from diversion
	TLinAddr iUserStackBase;
	};

IMPORT_C HANDLE CreateWin32Thread(TEmulThreadType aType, LPTHREAD_START_ROUTINE aThreadFunc, LPVOID aPtr, TBool aRun);

void SchedulerInit(NThread& aInit);
void SchedulerRegister(NThread& aSelf);
void SchedulerLock();
void SchedulerUnlock();
void SchedulerEscape();
void SchedulerReenter();
NThread* RunningThread();
NThread& CheckedCurrentThread();

void EnterKernel(TBool aDiversion = FALSE);
void LeaveKernel();
IMPORT_C void StartOfInterrupt();
IMPORT_C void EndOfInterrupt();

void Win32FindNonPreemptibleFunctions();

IMPORT_C TInt __fastcall Dispatch(TInt aFunction, TInt* aArgs);

typedef TInt (__cdecl* TExecHandler)(TInt, TInt, TInt, TInt);
typedef void (__cdecl* TPreprocessHandler)(TInt*, TUint32);

// Emulator nKern scheduling data
class Win32Interrupt
	{
public:
	void Init();
	void BeginInterrupt();
	void EndInterrupt();
	TInt MaskInterrupts(TBool aPreempt);
	void RestoreInterruptMask(TInt aLevel);
	void ForceReschedule();

	inline TBool InInterrupt() const
		{
		return iInterrupted != 0;
		}
	inline TBool InterruptsStatus(TBool aRequest) const
		{
		return aRequest ? (iLevel == 0) : (iLevel != 0);
		}

private:
	static void SchedulerThreadFunction(TAny*);

private:
	CRITICAL_SECTION iCS;			// protects data below
	HANDLE iQ;						// semaphore to wait on

	DWORD iOwner;					// controller of the mask
	TInt iLevel;					// owner's recursion count
	TInt iWaiting;					// number of waiters

	TBool iRescheduleOnExit;
	NThread* iInterrupted;			// the thread preempted by Begin()
	NThread iScheduler;				// the dedicated scheduler thread
	};

extern TBool Win32AtomicSOAW;		// flag to indicate availability of SignalObjectAndWait() API
extern TBool Win32TraceThreadId;
extern TInt Win32SingleCpu;
extern Win32Interrupt Interrupt;

// Emulator nKern exception data
extern TAny* Win32ExcAddress;
extern TAny* Win32ExcDataAddress;
extern TUint Win32ExcCode;

void FastCounterInit();

// Wrappers round Win32 thread control & synchronisation functions ...
inline void CheckedSuspendThread(HANDLE aWinThread)
	{
	TInt suspLevel = (TInt)SuspendThread(aWinThread);
	__NK_ASSERT_ALWAYS(suspLevel >= 0);
	}

inline void CheckedResumeThread(HANDLE aWinThread, BOOL once = EFalse)
	{
	TInt suspLevel = (TInt)ResumeThread(aWinThread);
	__NK_ASSERT_ALWAYS(once ? suspLevel == 1 : suspLevel > 0);				// check thread was previously suspended
	}

inline void CheckedGetThreadContext(HANDLE aWinThread, CONTEXT* aContext)
	{
	DWORD r = GetThreadContext(aWinThread, aContext);
	__NK_ASSERT_ALWAYS(r != 0);
	}

inline void CheckedSetThreadContext(HANDLE aWinThread, CONTEXT* aContext)
	{
	DWORD r = SetThreadContext(aWinThread, aContext);
	__NK_ASSERT_ALWAYS(r != 0);
	}

inline void CheckedSetThreadPriority(HANDLE aWinThread, TInt aPriority)
	{
	DWORD r = SetThreadPriority(aWinThread, aPriority);
	__NK_ASSERT_ALWAYS(r != 0);
	}

inline void CheckedWaitForSingleObject(HANDLE aWaitObject)
	{
	DWORD r = WaitForSingleObject(aWaitObject, INFINITE);
	__NK_ASSERT_ALWAYS(r == WAIT_OBJECT_0);
	}

inline void CheckedSignalObjectAndWait(HANDLE aToWake, HANDLE aToWaitOn)
	{
	DWORD r = SignalObjectAndWait(aToWake, aToWaitOn, INFINITE, FALSE);
	__NK_ASSERT_ALWAYS(r == WAIT_OBJECT_0);
	}

inline void CheckedSetEvent(HANDLE aWaitObject)
	{
	DWORD r = SetEvent(aWaitObject);
	__NK_ASSERT_ALWAYS(r != 0);
	}

inline void CheckedReleaseSemaphore(HANDLE aSemaphore)
	{
	DWORD r = ReleaseSemaphore(aSemaphore, 1, NULL);
	__NK_ASSERT_ALWAYS(r != 0);
	}


#endif	// __NK_WIN32_H__
