// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\win32\ckernel.cpp
// 
//

#include "memmodel.h"
#include <kernel/emi.h>
#include <kernel/kern_priv.h>

#define iMState		iWaitLink.iSpare1
#define iExiting	iWaitLink.iSpare2

#ifdef __LEAVE_EQUALS_THROW__
#include <e32panic.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
_LIT(KLitUser, "USER");
#endif //__LEAVE_EQUALS_THROW__


/********************************************
 * Thread
 ********************************************/

DWin32Thread::~DWin32Thread()
	{
	DThread::Destruct();
	}

void DWin32Thread::DoExit2()
	{
	DWin32Process* process = (DWin32Process*)iOwningProcess;
	process->CallRuntimeHook(EWin32RuntimeThreadDetach);
	}

typedef void (*TWin32ProcessEntry)(TInt aReason, TAny* aInfo);
void DThread::EpocThreadFunction(TAny* aParam)
//
// Called when an EPOC thread starts running
// parameter is the SThreadCreateInfo block
//
	{
	SThreadCreateInfo& info = *static_cast<SThreadCreateInfo*>(aParam);
	DThread& t = *TheCurrentThread;
	__DEBUG_EVENT(EEventStartThread, &t);

#ifdef __EMI_SUPPORT__
	EMI::CallStartHandler(&t);
#endif

	if (t.iThreadType != EThreadUser)
		Kern::Exit(info.iFunction(info.iPtr));
	else
		{
		t.iUserThreadState = EUserThreadRunning;
		LeaveKernel();
		
#ifdef __LEAVE_EQUALS_THROW__
		// intercept uncaught leaves
		try {
#endif

		DWin32Process* process = (DWin32Process*)t.iOwningProcess;
		process->CallRuntimeHook(EWin32RuntimeThreadAttach);

		TWin32ProcessEntry f;
		TInt reason = (t.iFlags & KThreadFlagOriginal) ? KModuleEntryReasonProcessInit : KModuleEntryReasonThreadInit;
		if (reason == KModuleEntryReasonProcessInit)
			f = (TWin32ProcessEntry)t.iOwningProcess->iCodeSeg->iEntryPtVeneer;
		else
			f = (TWin32ProcessEntry)t.iOwningProcess->iReentryPoint;
		(*f)(reason, &info);
		
#ifdef __LEAVE_EQUALS_THROW__
		} catch (XLeaveException&) {
			EnterKernel();
			Kern::PanicCurrentThread(KLitUser, EUserLeaveWithoutTrap);
		}
#endif
		}
	}

TInt DWin32Thread::Context(TDes8& /*aDes*/)
	{
//	TArmRegSet& s=*(TArmRegSet*)aDes.Ptr();
//	TLinAddr* pU=NULL;
//	aDes.SetLength(sizeof(s));
	TInt r=KErrNone;
	if (iThreadType!=EThreadUser || this==TheCurrentThread)
		r=KErrAccessDenied;
	else if (iExiting)
		r=KErrDied;
	else
		r=KErrNotSupported;//NKern::ThreadGetUserContext(&iNThread,&s,availmask);
	return r;
	}


DProcess* P::NewProcess()
	{
	return new DWin32Process;
	}

TInt DWin32Process::GetNewThread(DThread*& aThread, SThreadCreateInfo&)
//
// Create a new DWin32Thread object
// If aThread=NULL on entry, allocate it on the kernel heap,
// otherwise do in-place construction.
//
	{
	if (aThread)
		new (aThread) DWin32Thread;
	else
		{
		aThread=new DWin32Thread;
		if (!aThread)
			return KErrNoMemory;
		}
	aThread->iOwningProcess=this;
	return KErrNone;
	}


// Exception handling

TExcType exceptionType(TUint32 aExceptionCode)
	{
	switch (aExceptionCode)
		{
	case EXCEPTION_ACCESS_VIOLATION:
		return EExcAccessViolation;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return EExcBoundsCheck;
	case EXCEPTION_BREAKPOINT:
		return EExcBreakPoint;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return EExcAlignment;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return EExcFloatDenormal;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return EExcFloatDivideByZero;
	case EXCEPTION_FLT_INEXACT_RESULT:
		return EExcFloatInexactResult;
	case EXCEPTION_FLT_INVALID_OPERATION:
		return EExcFloatInvalidOperation;
	case EXCEPTION_FLT_OVERFLOW:
		return EExcFloatOverflow;
	case EXCEPTION_FLT_STACK_CHECK:
		return EExcFloatStackCheck;
	case EXCEPTION_FLT_UNDERFLOW:
		return EExcFloatUnderflow;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return EExcInvalidOpCode;
	case EXCEPTION_IN_PAGE_ERROR:
		return EExcPageFault;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return EExcIntegerDivideByZero;
	case EXCEPTION_INT_OVERFLOW:
		return EExcIntegerOverflow;
	case EXCEPTION_INVALID_DISPOSITION:
		return EExcAbort;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return EExcDoubleFault;
	case EXCEPTION_PRIV_INSTRUCTION:
		return EExcPrivInstruction;
	case EXCEPTION_SINGLE_STEP:
		return EExcSingleStep;
	case EXCEPTION_STACK_OVERFLOW:
		return EExcStackFault;
		}
	return EExcGeneral;
	}

void Exc::Dispatch(TAny* aPtr, NThread*)
	{
	TWin32ExcInfo *pR=(TWin32ExcInfo*)aPtr;
//
	DThread* pT=TheCurrentThread;
	TExcTrap* xt=pT->iExcTrap;
	if (xt)
		{
		// current thread wishes to handle exceptions
		(*xt->iHandler)(xt,pT,aPtr);
		}
	if ((pR->iFlags&TWin32ExcInfo::EExcInKernel) && NKern::HeldFastMutex())	// thread held fast mutex when exception occurred
		Exc::Fault(aPtr);
	
	NKern::LockSystem();
	if (pT->iThreadType==EThreadUser && !(pR->iFlags&TWin32ExcInfo::EExcInKernel))
		{
		TExcType type=exceptionType(pR->iExcCode);
		pR->iExcType = type;
		if (pT->IsExceptionHandled(type))
			{
			pT->iFlags |= KThreadFlagLastChance;
			NKern::UnlockSystem();
			pR->iHandler = (TExcHandler)pT->iOwningProcess->iReentryPoint;
			pR->iParam[0] = (TAny*)KModuleEntryReasonException;
			pR->iParam[1] = (TAny*)&pR->iExcType;
			return;
			}
		}
	// Fault system before attempting to signal kernel event handler as going 
	// crash anyway so no point trying to handle the event.  Also, stops
	// D_EXC hanging system on crash of critical/permanent thread. (see INC093397)
    if (pT->iFlags & (KThreadFlagSystemCritical|KThreadFlagSystemPermanent))
		Exc::Fault(aPtr);
	NKern::UnlockSystem();
    
	TUint m = DKernelEventHandler::Dispatch(EEventHwExc, pR, NULL);
	if (m & DKernelEventHandler::EExcHandled)
		return;
	
	// panic current thread
	K::PanicKernExec(ECausedException);
	}

EXPORT_C void Exc::Fault(TAny*)
	{
	Kern::Fault("Exception",K::ESystemException);
	}

extern "C" void ExcFault(TAny* aExcInfo)
	{
	Exc::Fault(aExcInfo);
	}

extern "C" {
void DefaultExcTrapHandler(TExcTrap* xt, DThread*, TAny*)
	{
	xt->Exception(KErrBadDescriptor);
	}
	
void IpcExcHandler(TExcTrap* xt, DThread* aThread, TAny* aContext)
	{
	DThread::IpcExcHandler(xt, aThread, aContext);
	}

__NAKED__ void TExcTrap_Trap()
	{
	_asm mov eax, TheScheduler.iCurrentThread
	_asm mov edx, 0
	_asm lea edx, [edx]DThread.iNThread
	_asm sub eax, edx
	_asm mov edx, [esp+4]
	_asm mov [ecx]TExcTrap.iHandler, edx
	_asm mov [eax]DThread.iExcTrap, ecx
	_asm mov [ecx]TExcTrap.iThread, eax
	_asm pop edx
	_asm mov [ecx+0], edx
	_asm mov [ecx+4], ebx
	_asm mov [ecx+8], ebp
	_asm mov [ecx+12], esp
	_asm mov [ecx+16], esi
	_asm mov [ecx+20], edi
	_asm mov [ecx+24], ds
	_asm mov [ecx+28], es
	_asm mov [ecx+32], fs
	_asm mov [ecx+36], gs
	_asm push edx
	_asm xor eax, eax
	_asm ret 4
	}
}

__NAKED__ TInt TIpcExcTrap::Trap(DThread* /*aThread*/)
	{
	// On entry ecx=this, [esp]=return address, [esp+4]=aThread
	// The thread parameter is not used on the emulator and so is ignored
	// __thiscall convention - this function must remove the stack parameter
	_asm pop eax
	_asm pop edx
	_asm lea edx, IpcExcHandler
	_asm push edx
	_asm push eax
	_asm jmp TExcTrap_Trap
	}

EXPORT_C __NAKED__ TInt TExcTrap::Trap()
	{
	// On entry ecx=this, [esp]=return address
	// __thiscall convention
	_asm pop eax
	_asm lea edx, DefaultExcTrapHandler
	_asm push edx
	_asm push eax
	_asm jmp TExcTrap_Trap
	}

EXPORT_C __NAKED__ TInt TExcTrap::Trap(TExcTrapHandler /*aHandler*/)
	{
	// On entry ecx=this, [esp]=return address, [esp+4]=aHandler
	// __thiscall convention - this function must remove the stack parameter
	_asm jmp TExcTrap_Trap
	}

EXPORT_C __NAKED__ void TExcTrap::Exception(TInt /*aResult*/)
	{
	// On entry ecx=this, [esp]=return address (unused), [esp+4]=aResult
	// Need ret 4 since saved esp leaves parameter to TExcTrap::Trap on the stack
#if defined(__VC32__)
	_asm mov edx, [ecx]this.iThread
#elif defined(__CW32__)
 	_asm mov edx, [ecx]TExcTrap.iThread
#endif
	_asm xor eax, eax
	_asm mov [edx]DThread.iExcTrap, eax
	_asm mov eax, TheScheduler.iCurrentThread
	_asm dec dword ptr [eax]NThread.iInKernel
	_asm mov eax, [esp+4]
	_asm mov edx, [ecx+0]
	_asm mov ebx, [ecx+4]
	_asm mov ebp, [ecx+8]
	_asm mov esp, [ecx+12]
	_asm mov esi, [ecx+16]
	_asm mov edi, [ecx+20]
	_asm mov ds, [ecx+24]
	_asm mov es, [ecx+28]
	_asm mov fs, [ecx+32]
	_asm mov gs, [ecx+36]
	_asm push edx
	_asm ret 4
	}
