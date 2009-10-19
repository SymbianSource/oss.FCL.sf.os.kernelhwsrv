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
// e32\kernel\win32\cexec.cpp
// 
//

#include <win32.h>
#include <kernel/cache.h>
#include "memmodel.h"

GLREF_C TInt CalcKernelHeapUsed();
void GetLatencyValues(TInt aMode, TInt& aCount, TInt* aDest);
void KernMsgTest();
void InvalidExecHandler();
void InvalidFastExec();
void Win32PreprocessHandler(TInt* aArgs, TUint32 aFlags);

#define __GEN_KERNEL_EXEC_CODE__

#include "execs.h"


/***********************************************************************************
 * User-side executive handlers
 ***********************************************************************************/

TInt ExecHandler::BreakPoint()
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::Breakpoint"));
	return 0;
	}

TInt ExecHandler::ProfileStart( TInt __DEBUG_ONLY(aProfile))
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ProfileStart(%d)",
		aProfile));
	return 0;
	}

TInt ExecHandler::ProfileEnd( TInt __DEBUG_ONLY(aProfile))
	{
	// Don't implement without considering Platform Security!
	__KTRACE_OPT(KEXEC,Kern::Printf("ExecHandler::ProfileEnd(%d)",
		aProfile));
	return 0;
	}

/***********************************************************************************
 * Exec dispatch code
 ***********************************************************************************/

void InvalidExecHandler()
	{
	K::PanicKernExec(EInvalidSystemCall);
	}

void InvalidFastExec()
	{
	NKern::Unlock();		// On Win32, fast exec calls hold the preemption lock
	InvalidExecHandler();
	}

/***************************************************************************
 * Look up a handle in the current thread or process handles array
 * On entry aFlags=0xY000000X, where bits 0-3 indicate the type of object referenced.
 * Return address of object referenced, or Panic if handle invalid
 * Enter and leave with system locked
 ***************************************************************************/

void Win32PreprocessHandler(TInt* aArgs, TUint32 aFlags)
	{
	DThread* t = TheCurrentThread;
	aFlags &= 0x3f;
	if (aFlags >= EIpcMessageD)
		{
		RMessageK* m = RMessageK::MessageK(aArgs[0], t);
		if (m == NULL || (aFlags != EIpcMessageD && m->iFunction == RMessage2::EDisConnect))
			K::PanicCurrentThread(EBadMessageHandle);
		if (aFlags == EIpcClient)
			aArgs[0] = TInt(m->iClient);
		return;
		}
	DObject* obj = Kern::ObjectFromHandle(t, aArgs[0], TObjectType(TInt(aFlags)-1));
	if (obj)
		aArgs[0] = TInt(obj);
	else
		K::PanicCurrentThread(EBadHandle);
	}

/**	
@pre    No fast mutex can be held.
@pre	Call in a thread context.
@pre	Kernel must be unlocked
@pre	interrupts enabled
*/
EXPORT_C TInt Kern::HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2)
//
// Execute a HAL function
//
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::HalFunction(TInt, TInt, TAny*, TAny*)");
	return ExecHandler::HalFunction(aGroup, aFunction, a1, a2);
	}

/**	
@pre    No fast mutex can be held.
@pre	Call in a thread context.
@pre	Kernel must be unlocked
@pre	interrupts enabled
*/
EXPORT_C TInt Kern::HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2, TInt aDeviceNumber)
//
// Execute a HAL function
//
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::HalFunction(TInt, TInt, TAny*, TAny*, TInt)");
	return ExecHandler::HalFunction(aGroup | (aDeviceNumber<<16), aFunction, a1, a2);
	}


TInt ExecHandler::SetFloatingPointMode(TFloatingPointMode /*aMode*/, TFloatingPointRoundingMode /*aRoundingMode*/)
	{
	return KErrNotSupported;
	}


TInt ExecHandler::SetWin32RuntimeHook(TAny* aFunc)
	{
	DWin32Process* process = (DWin32Process*)TheCurrentThread->iOwningProcess;
	if (process->iWin32RuntimeHook)
		return KErrAlreadyExists;
	process->iWin32RuntimeHook = (TWin32RuntimeHook)aFunc;
	process->CallRuntimeHook(EWin32RuntimeProcessAttach);
	return KErrNone;
	}
