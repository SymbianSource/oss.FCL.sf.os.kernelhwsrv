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
// e32\kernel\x86\ckernel.cpp
// 
//

#include <x86_mem.h>

#ifdef __SMP__
#include <apic.h>
#endif

#define iMState		iWaitLink.iSpare1
#define iExiting	iWaitLink.iSpare2

DX86PlatThread* TheCurrentFpuThread;

void DoFpuContextSwitch(DX86PlatThread*);
void SaveFpuContext(DX86PlatThread*);
void LoadFpuContext(DX86PlatThread*);

/********************************************
 * Thread
 ********************************************/

DX86PlatThread::DX86PlatThread()
	{
	}

DX86PlatThread::~DX86PlatThread()
	{
	DThread::Destruct();
	}


TInt DX86PlatThread::Context(TDes8& aDes)
	{
#ifdef __SMP__
	TX86RegSet& s=*(TX86RegSet*)aDes.Ptr();
	aDes.SetLength(sizeof(s));
	TInt r=KErrNone;
	if (iThreadType!=EThreadUser || this==TheCurrentThread)
		r=KErrAccessDenied;
	else if (iExiting)
		r=KErrDied;
	else
		{
		TUint32 unused;
		NKern::ThreadGetUserContext(&iNThread,&s,unused);
		}
	return r;
#else
	return KErrNotSupported;
#endif
	}

DProcess* P::NewProcess()
	{
	return new DX86PlatProcess;
	}

TInt DX86PlatProcess::GetNewThread(DThread*& aThread, SThreadCreateInfo& anInfo)
//
// Create a new DX86PlatThread object
// If aThread=NULL on entry, allocate it on the kernel heap,
// otherwise do in-place construction.
//
	{
	DX86PlatThread* pT=(DX86PlatThread*)aThread;
	if (pT)
		new (pT) DX86PlatThread;
	else
		{
		pT=new DX86PlatThread;
		if (!pT)
			return KErrNoMemory;
		aThread=(DThread*)pT;
		}
	pT->iOwningProcess=this;
	return KErrNone;
	}

TExcType VectorToType(TInt aVector)
	{
	switch (aVector)
		{
		case 0:
			return EExcIntegerDivideByZero;
		case 3:
			return EExcBreakPoint;
		case 4:
			return EExcIntegerOverflow;
		case 5:
			return EExcBoundsCheck;
		case 6:
			return EExcInvalidOpCode;
		case 8:
			return EExcDoubleFault;
		case 11:
			return EExcAccessViolation;
		case 12:
			return EExcStackFault;
		case 13:
			return EExcAccessViolation;
		case 14:
			return EExcPageFault;
		case 16:
			return EExcFloatDivideByZero;
		case 17:
			return EExcAlignment;
		default:
			return EExcGeneral;
		}
	}

GLREF_C void DumpExcInfo(TX86ExcInfo& a);

void __fastcall PushExcInfoOnUserStack(TX86ExcInfo*, TInt);

void Exc::Dispatch(TAny* aPtr, NThread*)
	{
#ifdef __DEMAND_PAGING__
	TInt paged = M::DemandPagingFault(aPtr);
	if(paged==KErrNone)
		return;
#endif
	TX86ExcInfo* pR=(TX86ExcInfo*)aPtr;
	TInt cpl=pR->iCs & 3;
	DX86PlatThread* pT=(DX86PlatThread*)TheCurrentThread;
	if (cpl==0 && pT->iMagicExcHandler && (*pT->iMagicExcHandler)(pR)==0)
		{
		// magic handler has handled the exception
		return;
		}
	TExcTrap* xt=pT->iExcTrap;
	if (xt)
		{
		// current thread wishes to handle exceptions
		(*xt->iHandler)(xt,pT,aPtr);
		}
	if (NKern::HeldFastMutex())	// thread held fast mutex when exception occurred
		Exc::Fault(aPtr);
		
	NKern::LockSystem();
	if (pT->iThreadType==EThreadUser && cpl==3)
		{
		TExcType type=VectorToType(pR->iExcId);
		if (pT->IsExceptionHandled(type))
			{
			pT->iFlags |= KThreadFlagLastChance;
			NKern::UnlockSystem();

			// tweak context to call exception handler
			__KTRACE_OPT(KSCRATCH,DumpExcInfo(*pR));
			PushExcInfoOnUserStack(pR, type);
			pR->iEip = (TUint32)pT->iOwningProcess->iReentryPoint;
			pR->iEbx = KModuleEntryReasonException;
			__KTRACE_OPT(KSCRATCH,DumpExcInfo(*pR));
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

//	__KTRACE_OPT(KPANIC,DumpExcInfo(*pR));
	DumpExcInfo(*pR);

	// panic current thread
	K::PanicKernExec(ECausedException);
	}

EXPORT_C void Exc::Fault(TAny* aPtr)
	{
	TX86ExcInfo *pR=(TX86ExcInfo*)aPtr;
	TExcInfo e;
	e.iCodeAddress=(TAny*)pR->iEip;
	e.iDataAddress=(TAny*)pR->iFaultAddress;
	e.iExtraData=(TInt)pR->iExcErrorCode;
//	__KTRACE_OPT(KPANIC,DumpExcInfo(*pR));
	DumpExcInfo(*pR);
	TheSuperPage().iKernelExcId=pR->iExcId;
	TheSuperPage().iKernelExcInfo=e;
	NKern::DisableAllInterrupts();
#ifdef __SMP__
	SubScheduler().iSSX.iExcInfo = aPtr;
#else
	TheScheduler.i_ExcInfo = aPtr;
#endif
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
}


void DX86PlatThread::DoExit2()
	{
	}


#ifdef _DEBUG
extern "C" void __FaultIpcClientNotNull()
	{
	K::Fault(K::EIpcClientNotNull);
	}
#endif
