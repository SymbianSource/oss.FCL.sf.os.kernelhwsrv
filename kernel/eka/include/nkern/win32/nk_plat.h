// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __NK_WIN32_H__
#define __NK_WIN32_H__

#define _CRTIMP			// we want to use the win32 static runtime library

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>

typedef void (*TExcHandler)(TAny*,TAny*);

struct TWin32ExcInfo
	{
	enum {EExcInKernel = 0x1};
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
	EThreadNKern	// a nKern thread, identified by a NThread control block
	};

class NThread : public NThreadBase
	{
public:
	typedef void (*TDivert)();
	enum TWakeup {ERelease,EResume,EResumeLocked,EIdle,EEscaped,EResumeDiverted};
	enum TSelectCpu {ECpuAll=-1,ECpuSingle=-2};
public:
	TInt Create(SNThreadCreateInfo& aInfo, TBool aInitial);
	void Stillborn();
	void DoForceExit();
//
	IMPORT_C static void Idle();
	IMPORT_C static void SetProperties(TBool aTrace, TInt aCpu);
	TBool WakeUp();
	TBool IsSafeToPreempt();
	void Divert(TDivert aDivert);
	void ApplyDiversion();

private:
	static DWORD WINAPI StartThread(LPVOID aParam);
//
	static void ExitSync();
	static void ExitAsync();
//
	static void Exception();
	static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* aExc);

public:
	static void Diverted();
	// Has to be accessible to code user-side via emulator.lib
	static DWORD ExceptionHandler(EXCEPTION_RECORD* aException, CONTEXT* aContext);

public:
	HANDLE iWinThread;
	DWORD iWinThreadId;
	HANDLE iScheduleLock;	// event used for scheduling interlock
	TBool iDiverted;		// flag to indicate that the thread is being diverted
	TDivert iDivert;		// function to invoke after reschedule, may be null
	TAny* iDivertReturn;    // return address from diversion
	TInt iInKernel;			// flag to indicate if the thread is running 'in' the kernel
	TWakeup iWakeup;		// indicates how to wake up the thread
	TLinAddr iUserStackBase;
	};

IMPORT_C HANDLE CreateWin32Thread(TEmulThreadType aType, LPTHREAD_START_ROUTINE aThreadFunc, LPVOID aPtr, TBool aRun);
IMPORT_C void StartOfInterrupt();
IMPORT_C void EndOfInterrupt();

void SchedulerInit(NThread& aInit);
void SchedulerRegister(NThread& aSelf);
NThread* SchedulerThread();
NThread& CheckedCurrentThread();
void SchedulerLock();
void SchedulerUnlock();
void SchedulerEscape();
void SchedulerReenter();
void Win32FindNonPreemptibleFunctions();

inline void EnterKernel(TBool aCheck=TRUE)
	{
	if (++CheckedCurrentThread().iInKernel==1 && aCheck)
		{
		NThread& t = CheckedCurrentThread();
		__NK_ASSERT_ALWAYS(t.iCsCount==0);
		__NK_ASSERT_ALWAYS(t.iHeldFastMutex==0);
		__NK_ASSERT_ALWAYS(TheScheduler.iKernCSLocked==0);
		}
	}

void LeaveKernel();

IMPORT_C TInt __fastcall Dispatch(TInt aFunction, TInt* aArgs);

typedef TInt (__cdecl *TExecHandler)(TInt,TInt,TInt,TInt);
typedef void (__cdecl *TPreprocessHandler)(TInt*,TUint32);

// Emulator nKern scheduling data
class Win32Interrupt
	{
public:
	void Init();
	TInt Mask();
	void Restore(TInt aLevel);
	void Begin();
	void End();
	inline TBool InInterrupt() const
		{return iInterrupted!=0;}	
	void ForceReschedule();
	inline TBool InterruptsStatus(TBool aRequest) const
	{return aRequest?(iLevel==0):(iLevel!=0);}
private:
	static void Reschedule(TAny*);
private:
	TInt iLock;
	HANDLE iQ;
	DWORD iOwner;
	TInt iLevel;
	TBool iRescheduleOnExit;
	NThread* iInterrupted;
	NThread iScheduler;
	};

extern TBool Win32AtomicSOAW;	// flag to indicate availability of SignalObjectAndWait() API
extern TBool Win32TraceThreadId;
extern TInt Win32SingleCpu;
extern Win32Interrupt Interrupt;

// Emulator nKern exception data
extern TAny* Win32ExcAddress;
extern TAny* Win32ExcDataAddress;
extern TUint Win32ExcCode;

void FastCounterInit();

#endif
