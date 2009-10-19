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
// e32test\system\t_xprot.cpp
// 
//

#include <e32test.h>
#include "execinfo.h"
#include <u32arm.h>

const TInt KSupervisorAddress=0x80000000;
const TInt KGarbageAddress=0xeeddbbcc;

TInt Handles[MAX_HANDLE];
TInt DataArea[1024];
TBuf8<1024> Buf8;
TBuf<1024> Buf;
TPtrC8 PtrC8(Buf8.Ptr(),10);
TPtrC8 PtrC(Buf.Ptr(),10);
TPtrC8 Ptr8(Buf8.Ptr(),10,1024);
TPtrC8 Ptr(Buf.Ptr(),10,1024);
TPtrC8 Ptr8N(NULL,10,1024);
TPtrC8 PtrN(NULL,10,1024);
TPtrC8 Ptr8I((TUint8*)KGarbageAddress,10,1024);
TPtrC8 PtrI((TText*)KGarbageAddress,10,1024);
TPtrC8 Ptr8S((TUint8*)KSupervisorAddress,10,1024);
TPtrC8 PtrS((TText*)KSupervisorAddress,10,1024);
TInt MessageHandle;

TInt Args[4];

TInt GetHandle(TInt& aState, TInt aHandleType)
	{
	TInt r;
	TInt t=aHandleType?aHandleType-1:ELibrary;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=0xdeaddead; break;
		case 2: r=Handles[EThread]; break;
		case 3: r=Handles[t]; break;
		case 4: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetAnyInt(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=1; break;
		case 2: r=2; break;
		case 3: r=-1; break;
		case 4: r=299792458; break;
		case 5: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetAnyPtr(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)DataArea; break;
		case 4: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetIntPtr(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=((TInt)DataArea)+1; break;
		case 3: r=KSupervisorAddress; break;
		case 4: r=(TInt)DataArea; break;
		case 5: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetDes8(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)&Ptr8N; break;
		case 4: r=(TInt)&Ptr8I; break;
		case 5: r=(TInt)&Ptr8S; break;
		case 6: r=(TInt)&Ptr8; break;
		case 7: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetWDes8(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)&Ptr8N; break;
		case 4: r=(TInt)&Ptr8I; break;
		case 5: r=(TInt)&Ptr8S; break;
		case 6: r=(TInt)&PtrC8; break;
		case 7: r=(TInt)&Ptr8; break;
		case 8: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetDes(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)&PtrN; break;
		case 4: r=(TInt)&PtrI; break;
		case 5: r=(TInt)&PtrS; break;
		case 6: r=(TInt)&Ptr; break;
		case 7: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetWDes(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)&PtrN; break;
		case 4: r=(TInt)&PtrI; break;
		case 5: r=(TInt)&PtrS; break;
		case 6: r=(TInt)&PtrC; break;
		case 7: r=(TInt)&Ptr; break;
		case 8: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetBool(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=1; break;
		case 2: r=299792458; break;
		case 3: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetObjectType(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=EThread; break;
		case 1: r=ELibrary; break;
		case 2: r=ENumObjectType; break;
		case 3: r=20000; break;
		case 4: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetDevUnit(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=1; break;
		case 2: r=20000; break;
		case 3: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetMsgHandle(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=(TInt)DataArea; break;
		case 3: r=KSupervisorAddress; break;
		case 4: r=MessageHandle; break;
		case 5: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetModuleHandle(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=ModuleHandle; break;
		case 3: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetSessionHandlePtr(TInt& aState)
	{
	TInt r;
	switch(aState)
		{
		case 0: r=0; break;
		case 1: r=KGarbageAddress; break;
		case 2: r=KSupervisorAddress; break;
		case 3: r=(TInt)&Handles[ESession]; break;
		case 4: aState=0; return 0;
		}
	++aState;
	return r;
	}

TInt GetArgument(TInt& aState, TInt aArgType)
	{
	if (aArgType==NO_PAR)
		return 0;
	if (aArgType<MAX_HANDLE)
		return GetHandle(aState,aArgType);
	switch (aArgType)
		{
		case ANY_INT: return GetAnyInt(aState);
		case ANY_PTR: return GetAnyPtr(aState);
		case INT_PTR: return GetIntPtr(aState);
		case DES8: return GetDes8(aState);
		case WDES8: return GetWDes8(aState);
		case DES: return GetDes(aState);
		case WDES: return GetWDes(aState);
		case BOOL: return GetBool(aState);
		case OBJECT_TYPE: return GetObjectType(aState);
		case DEV_UNIT: return GetDevUnit(aState);
		case MSG_HANDLE: return GetMsgHandle(aState);
		case MODULE_HANDLE: return GetModuleHandle(aState);
		case SESSION_HANDLE_PTR: return GetSessionHandlePtr(aState);
		}
	User::Panic(_L("UNKNOWN ARG TYPE"),aArgType);
	}

#define FAST_EXEC(n)	asm("swi %a0" : : "i" (n|EXECUTIVE_FAST))
__NAKED__ void DoFastExecCall(TInt /*aExecNum*/)
	{
	asm("ldr r1, __Args ");
	asm("mov ip, lr ");
	asm("ldmfd r1, {r0-r3} ");
	asm("add pc, pc, r0, lsl #2 ");
	asm("nop )";

	FAST_EXEC(EFastExecWaitForAnyRequest);
	FAST_EXEC(EFastExecLanguage);
	FAST_EXEC(EFastExecHeap);
	FAST_EXEC(EFastExecHeapSwitch);
	FAST_EXEC(EFastExecPushTrapFrame);
	FAST_EXEC(EFastExecPopTrapFrame);
	FAST_EXEC(EFastExecActiveScheduler);
	FAST_EXEC(EFastExecSetActiveScheduler);
	FAST_EXEC(EFastExecLockPeriod);
	FAST_EXEC(EFastExecTrapHandler);
	FAST_EXEC(EFastExecSetTrapHandler);
	FAST_EXEC(EFastExecLockedInc);
	FAST_EXEC(EFastExecLockedDec);
	FAST_EXEC(EFastExecDebugMask);
	FAST_EXEC(EFastExecSetDebugMask);
	FAST_EXEC(EFastExecFastCounter);
	FAST_EXEC(EFastExecGetLocaleCharSet);
	FAST_EXEC(EFastExecLockRamDrive);
	FAST_EXEC(EFastExecUnlockRamDrive);
	FAST_EXEC(EFastExecRomHeaderAddress);
	FAST_EXEC(EFastExecRomRootDirAddress);
	FAST_EXEC(EFastExecJustInTime);
	FAST_EXEC(EFastExecSetJustInTime);
	FAST_EXEC(EFastExecBlockThreads);
	FAST_EXEC(EFastExecKernelStartup);
	FAST_EXEC(EFastExecFatUtilityFunctions);
	FAST_EXEC(EFastExecSafeInc);
	FAST_EXEC(EFastExecSafeDec);

	asm("__Args: ");
	asm(".word Args ");
	}

#define SLOW_EXEC(n)	asm("swi %a0" : : "i" (n))
__NAKED__ void DoSlowExecCall(TInt /*aExecNum*/)
	{
	asm("ldr r1, __Args ");
	asm("mov ip, lr ");
	asm("ldmfd r1, {r0-r3} ");
	asm("add pc, pc, r0, lsl #2 ");
	asm("nop )";

	SLOW_EXEC(EExecObjectNext);
	SLOW_EXEC(EExecChunkBase);
	SLOW_EXEC(EExecChunkSize);
	SLOW_EXEC(EExecChunkMaxSize);
	SLOW_EXEC(EExecHandleAttributes);
	SLOW_EXEC(EExecTickCount);
	SLOW_EXEC(EExecLogicalDeviceGetCaps);
	SLOW_EXEC(EExecLogicalDeviceQueryVersionSupported);
	SLOW_EXEC(EExecLogicalDeviceIsAvailable);
	SLOW_EXEC(EExecDllGlobalAlloc);
	SLOW_EXEC(EExecChangeLocale);
	SLOW_EXEC(EExecChannelRequest);
	SLOW_EXEC(EExecMathRandom);
	SLOW_EXEC(EExecChannelControl);
	SLOW_EXEC(EExecResetMachine);
	SLOW_EXEC(EExecLibraryLookup);
	SLOW_EXEC(EExecLibraryEntryPoint);
	SLOW_EXEC(EExecLibraryDllRefTable);
	SLOW_EXEC(EExecStaticCallList);
	SLOW_EXEC(EExecDynamicCallList);
	SLOW_EXEC(EExecLibraryCallList);
	SLOW_EXEC(EExecLibraryFileName);
	SLOW_EXEC(EExecExecuteInSupervisorMode);
	SLOW_EXEC(EExecMutexCount);
	SLOW_EXEC(EExecMutexWait);
	SLOW_EXEC(EExecMutexSignal);
	SLOW_EXEC(EExecProcessId);
	SLOW_EXEC(EExecDllFileName);
	SLOW_EXEC(EExecProcessResume);
	SLOW_EXEC(EExecProcessFileName);
	SLOW_EXEC(EExecProcessCommandLine);
	SLOW_EXEC(EExecProcessExitType);
	SLOW_EXEC(EExecProcessExitReason);
	SLOW_EXEC(EExecProcessExitCategory);
	SLOW_EXEC(EExecProcessPriority);
	SLOW_EXEC(EExecProcessSetPriority);
	SLOW_EXEC(EExecProcessFlags);
	SLOW_EXEC(EExecProcessSetFlags);
	SLOW_EXEC(EExecProcessSetOwner);
	SLOW_EXEC(EExecDllInitialiseData);
	SLOW_EXEC(EExecSemaphoreCount);
	SLOW_EXEC(EExecSemaphoreWait);
	SLOW_EXEC(EExecSemaphoreSignal1);
	SLOW_EXEC(EExecSemaphoreSignalN);
	SLOW_EXEC(EExecDllFreeData);
	SLOW_EXEC(EExecServerReceive);
	SLOW_EXEC(EExecServerCancel);
	SLOW_EXEC(EExecSetSessionPtr);
	SLOW_EXEC(EExecSessionSend);
	SLOW_EXEC(EExecThreadId);
	SLOW_EXEC(EExecSessionShare);
	SLOW_EXEC(EExecThreadResume);
	SLOW_EXEC(EExecThreadSuspend);
	SLOW_EXEC(EExecThreadPriority);
	SLOW_EXEC(EExecThreadSetPriority);
	SLOW_EXEC(EExecThreadProcessPriority);
	SLOW_EXEC(EExecThreadSetProcessPriority);
	SLOW_EXEC(EExecThreadFlags);
	SLOW_EXEC(EExecThreadSetFlags);
	SLOW_EXEC(EExecThreadRequestCount);
	SLOW_EXEC(EExecThreadExitType);
	SLOW_EXEC(EExecThreadExitReason);
	SLOW_EXEC(EExecThreadExitCategory);
	SLOW_EXEC(EExecThreadGetDesLength);
	SLOW_EXEC(EExecThreadGetDesMaxLength);
	SLOW_EXEC(EExecThreadRead8);
	SLOW_EXEC(EExecThreadRead16);
	SLOW_EXEC(EExecThreadWrite8);
	SLOW_EXEC(EExecThreadWrite16);
	SLOW_EXEC(EExecTimerCancel);
	SLOW_EXEC(EExecTimerAfter);
	SLOW_EXEC(EExecTimerAt);
	SLOW_EXEC(EExecTimerLock);
	SLOW_EXEC(EExecChangeNotifierLogon);
	SLOW_EXEC(EExecChangeNotifierLogoff);
	SLOW_EXEC(EExecRequestSignal);
	SLOW_EXEC(EExecMatch8);
	SLOW_EXEC(EExecMatch16);
	SLOW_EXEC(EExecFind8);
	SLOW_EXEC(EExecFind16);
	SLOW_EXEC(EExecLocateF8);
	SLOW_EXEC(EExecLocateF16);
	SLOW_EXEC(EExecHandleName);
	SLOW_EXEC(EExecHandleFullName);
	SLOW_EXEC(EExecHandleInfo);
	SLOW_EXEC(EExecHandleCount);
	SLOW_EXEC(EExecAfter);
	SLOW_EXEC(EExecAt);
	SLOW_EXEC(EExecDayName);
	SLOW_EXEC(EExecDayNameAbb);
	SLOW_EXEC(EExecMonthName);
	SLOW_EXEC(EExecMonthNameAbb);
	SLOW_EXEC(EExecSuffix);
	SLOW_EXEC(EExecAmPmName);
	SLOW_EXEC(EExecCurrencySymbol);
	SLOW_EXEC(EExecLocale);
	SLOW_EXEC(EExecLocaleSet);
	SLOW_EXEC(EExecLocaleMessageText);
	SLOW_EXEC(EExecMessageComplete);
	SLOW_EXEC(EExecTimeNow);
	SLOW_EXEC(EExecSetHomeTime);
	SLOW_EXEC(EExecSetMachineConfiguration);
	SLOW_EXEC(EExecCaptureEventHook);
	SLOW_EXEC(EExecReleaseEventHook);
	SLOW_EXEC(EExecRequestEvent);
	SLOW_EXEC(EExecRequestEventCancel);
	SLOW_EXEC(EExecAddEvent);
	SLOW_EXEC(EExecSessionSendSync);
	SLOW_EXEC(EExecDllGlobalAllocated);
	SLOW_EXEC(EExecDllGlobalRead);
	SLOW_EXEC(EExecDllGlobalWrite);
	SLOW_EXEC(EExecDllTls);
	SLOW_EXEC(EExecHalFunction);
	SLOW_EXEC(EExecSessionAttach);
	SLOW_EXEC(EExecWsRegisterThread);
	SLOW_EXEC(EExecFsRegisterThread);
	SLOW_EXEC(EExecProcessCommandLineLength);
	SLOW_EXEC(EExecTimerInactivity);
	SLOW_EXEC(EExecUserInactivityTime);
	SLOW_EXEC(EExecShortDateFormatSpec);
	SLOW_EXEC(EExecLongDateFormatSpec);
	SLOW_EXEC(EExecTimeFormatSpec);
	SLOW_EXEC(EExecResetInactivityTime);
	SLOW_EXEC(EExecDebugFunction);
	SLOW_EXEC(EExecBreakPoint);
	SLOW_EXEC(EExecProfileStart);
	SLOW_EXEC(EExecProfileEnd);
	SLOW_EXEC(EExecPasswordIsEnabled);
	SLOW_EXEC(EExecPasswordIsValid);
	SLOW_EXEC(EExecExceptionHandler);
	SLOW_EXEC(EExecSetExceptionHandler);
	SLOW_EXEC(EExecModifyExceptionMask);
	SLOW_EXEC(EExecRaiseException);
	SLOW_EXEC(EExecIsExceptionHandled);
	SLOW_EXEC(EExecThreadGetRamSizes);
	SLOW_EXEC(EExecProcessGetRamSizes);
	SLOW_EXEC(EExecLibraryGetRamSizes);
	SLOW_EXEC(EExecMachineConfiguration);
	SLOW_EXEC(EExecWasChunkSetHeapInfo);
	SLOW_EXEC(EExecLibraryType);
	SLOW_EXEC(EExecProcessType);
	SLOW_EXEC(EExecPasswordSetEnabled);
	SLOW_EXEC(EExecPasswordSet);
	SLOW_EXEC(EExecSetCurrencySymbol);
	SLOW_EXEC(EExecProcessSetType);
	SLOW_EXEC(EExecChunkBottom);
	SLOW_EXEC(EExecChunkTop);
	SLOW_EXEC(EExecThreadContext);
	SLOW_EXEC(EExecDllDataInfo);
	SLOW_EXEC(EExecThreadCreate);
	SLOW_EXEC(EExecProcessCreate);
	SLOW_EXEC(EExecProcessLoaded);
	SLOW_EXEC(EExecFindHandleOpen);
	SLOW_EXEC(EExecHandleClose);
	SLOW_EXEC(EExecChunkCreate);
	SLOW_EXEC(EExecChunkAdjust);
	SLOW_EXEC(EExecOpenNamedObject);
	SLOW_EXEC(EExecHandleDuplicate);
	SLOW_EXEC(EExecMutexCreate);
	SLOW_EXEC(EExecSemaphoreCreate);
	SLOW_EXEC(EExecThreadOpenById);
	SLOW_EXEC(EExecProcessOpenById);
	SLOW_EXEC(EExecThreadKill);
	SLOW_EXEC(EExecThreadLogon);
	SLOW_EXEC(EExecThreadLogonCancel);
	SLOW_EXEC(EExecDllSetTls);
	SLOW_EXEC(EExecDllFreeTls);
	SLOW_EXEC(EExecThreadRename);
	SLOW_EXEC(EExecProcessRename);
	SLOW_EXEC(EExecProcessKill);
	SLOW_EXEC(EExecProcessOwner);
	SLOW_EXEC(EExecProcessLogon);
	SLOW_EXEC(EExecProcessLogonCancel);
	SLOW_EXEC(EExecThreadSetInitialParameter);
	SLOW_EXEC(EExecThreadProcess);
	SLOW_EXEC(EExecThreadGetHeap);
	SLOW_EXEC(EExecServerCreate);
	SLOW_EXEC(EExecSessionCreate);
	SLOW_EXEC(EExecLibraryLoadExact);
	SLOW_EXEC(EExecLibraryLoaded);
	SLOW_EXEC(EExecDeviceLoad);
	SLOW_EXEC(EExecDeviceFree);
	SLOW_EXEC(EExecChannelCreate);
	SLOW_EXEC(EExecTimerCreate);
	SLOW_EXEC(EExecDllAddDependency);
	SLOW_EXEC(EExecTimerHighRes);
	SLOW_EXEC(EExecAfterHighRes);
	SLOW_EXEC(EExecChangeNotifierCreate);
	SLOW_EXEC(EExecUndertakerCreate);
	SLOW_EXEC(EExecUndertakerLogon);
	SLOW_EXEC(EExecUndertakerLogonCancel);
	SLOW_EXEC(EExecKernelHeapDebug);
	SLOW_EXEC(EExecThreadGetCpuTime);
	}


void TestExec(const SExecInfo& aInfo, TBool aFast)
	{
	TInt xn=aInfo.iExecNum;
	TInt np=aInfo.iNumParams;
	if (np==0)
		return;		// no parameters
	if (!aFast)
		{
		// don't do some slow execs
		switch (xn)
			{
			// loader group (could be restricted to F32)
			case EExecProcessCreate:
			case EExecProcessLoaded:
			case EExecLibraryLoadExact:
			case EExecLibraryLoaded:
			case EExecDllAddDependency:
				return;
			// descriptor processing (could be moved user side)
			case EExecMatch8:
			case EExecMatch16:
			case EExecLocateF8:
			case EExecLocateF16:
			case EExecFind8:
			case EExecFind16:
				return;

			// we don't want to do this
			case EExecResetMachine:
				return;
			}
		}

	TInt state[4];
	Mem::FillZ(state,sizeof(state));
	}
