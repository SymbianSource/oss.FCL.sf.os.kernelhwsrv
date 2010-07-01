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
// e32\kernel\arm\ckernel.cpp
// 
//

#include <arm_mem.h>
#include <arm_vfp.h>

#define iMState		iWaitLink.iSpare1
#define iExiting	iWaitLink.iSpare2

GLREF_C void __ArmVectorReset();
GLREF_C void __ArmVectorUndef();
GLREF_C void __ArmVectorSwi();
GLREF_C void __ArmVectorAbortPrefetch();
GLREF_C void __ArmVectorAbortData();
GLREF_C void __ArmVectorReserved();
GLREF_C void __ArmVectorIrq();
GLREF_C void __ArmVectorFiq();

extern "C" void ExcFault(TAny* aExcInfo);

/********************************************
 * Thread
 ********************************************/

DArmPlatThread::~DArmPlatThread()
	{
	DThread::Destruct();
	}

TInt DArmPlatThread::Context(TDes8& aDes)
	{
	TArmRegSet& s=*(TArmRegSet*)aDes.Ptr();
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
	}

DProcess* P::NewProcess()
	{
	return new DArmPlatProcess;
	}

#ifdef __CPU_HAS_VFP
#ifdef __VFP_V3
const TInt KVfpContextSize = (1 + (32 * 2)) * 4;	// FPSCR + 32 dword registers
#else
const TInt KVfpContextSize = (3 + (16 * 2)) * 4;	// FPSCR + FPINST + FPINST2 + 16 dword registers
#endif
#endif
TInt DArmPlatProcess::GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo)
//
// Create a new DArmPlatThread object
// If aThread=NULL on entry, allocate it on the kernel heap,
// otherwise do in-place construction.
//
	{
	DArmPlatThread* pT=(DArmPlatThread*)aThread;
	if (!pT)
		{
		TInt size = sizeof(DArmPlatThread);
#ifdef __CPU_HAS_VFP
		size += KVfpContextSize;
#endif
		__KTRACE_OPT(KTHREAD,Kern::Printf("GetNewThread size=%04x",size));
		pT = (DArmPlatThread*)Kern::AllocZ(size);
		}
	if (!pT)
		return KErrNoMemory;
	new (pT) DArmPlatThread;
	aThread = pT;
	pT->iOwningProcess=this;

#ifdef __CPU_HAS_VFP
	pT->iNThread.iExtraContext = (TUint8*)pT + sizeof(DArmPlatThread);
	*(TUint32*)(pT->iNThread.iExtraContext) = Arm::VfpDefaultFpScr;
	// Inherit parent VFP FPSCR value if applicable
	if ((TInt)aInfo.iType != (TInt)EThreadInitial)
		{
		if (pT->iOwningProcess == Kern::CurrentThread().iOwningProcess)
			{
			if (Arm::FpExc() & VFP_FPEXC_EN)
				{
				*(TUint32*)(pT->iNThread.iExtraContext) = Arm::FpScr() & VFP_FPSCR_MODE_MASK;
				}
			else
				{
				*(TUint32*)(pT->iNThread.iExtraContext) = *(TUint32*)(Kern::CurrentThread().iNThread.iExtraContext);
				}
			}
		}
#endif

	return KErrNone;
	}

extern void DumpExcInfo(TArmExcInfo& a);
void DumpExcInfoX(TArmExcInfo& a)
	{
	DumpExcInfo(a);
	NThread* nthread = NCurrentThread();
	if (nthread == NULL)
		Kern::Printf("No current thread");
	else
		{
		DThread* thread = Kern::NThreadToDThread(NCurrentThread());
		if (thread)
			{
			TFullName thread_name;
			thread->TraceAppendFullName(thread_name, EFalse);
			Kern::Printf("Thread full name=%S", &thread_name);
			Kern::Printf("Thread ID=%d, KernCSLocked=%d",TheCurrentThread->iId,NKern::KernelLocked());
			}
		else
			Kern::Printf("Thread N/A, KernCSLocked=%d",NKern::KernelLocked());
		}
	}

extern void DumpFullRegSet(SFullArmRegSet& a);
extern void GetUndefinedInstruction(TArmExcInfo* /*aContext*/);
extern void PushExcInfoOnUserStack(TArmExcInfo*, TInt);
extern "C" {
extern SFullArmRegSet DefaultRegSet;
}

#ifdef __CPU_HAS_VFP
GLREF_C TInt HandleVFPOperation(TAny* aPtr);
#endif

void Exc::Dispatch(TAny* aPtr, NThread*)
	{
#ifdef __CPU_ARM_ABORT_MODEL_UPDATED
#error Processors implementing the 'Base Register Updated' Abort Model are no longer supported
#endif

	TArmExcInfo* pR=(TArmExcInfo*)aPtr;
	TInt mode=pR->iCpsr & EMaskMode;

	TBool faultHandled = EFalse;

#ifdef __DEMAND_PAGING__
	faultHandled |= M::DemandPagingFault(aPtr) == KErrNone;
#endif

	if(!faultHandled)
		faultHandled |= M::RamDefragFault(aPtr) == KErrNone;

	if(faultHandled)
		{
#ifdef __ATOMIC64_USE_SLOW_EXEC__
		if (mode==ESvcMode && pR->iExcCode==EArmExceptionDataAbort && IsMagicAtomic64(pR->iR15))
			{
			// Magic atomic instruction so return to next instruction to stop any 
			// writes to memory being executed and ensure interrupts are enabled.
			pR->iR15 += 4;
			pR->iCpsr &= ~KAllInterruptsMask;
			}
#endif
		return;
		}

	if (mode==ESvcMode && pR->iExcCode==EArmExceptionDataAbort && IsMagic(pR->iR15))
		{
		// skip instruction that caused the exception, set the zero flag and place faulted address in r12
		__KTRACE_OPT(KPANIC,DumpExcInfoX(*pR));
		pR->iR15 += 4;
		pR->iCpsr |= ECpuZf;
		pR->iR12 = pR->iFaultAddress;
		return;
		}

	DThread* pT=TheCurrentThread;
	TExcTrap* xt=pT->iExcTrap;
	if (xt)
		{
		__KTRACE_OPT(KPANIC,DumpExcInfoX(*pR));
		// current thread wishes to handle exceptions
		(*xt->iHandler)(xt,pT,aPtr);
		}

	if (NKern::HeldFastMutex())	// thread held fast mutex when exception occurred
		Exc::Fault(aPtr);

#ifdef __CPU_HAS_VFP
	if (pR->iExcCode==EArmExceptionUndefinedOpcode)
		{
		// Get the undefined instruction
		GetUndefinedInstruction(pR);

		const TUint32 opcode = pR->iFaultAddress;
		TInt cpnum = -1;

#ifdef __SUPPORT_THUMB_INTERWORKING
		if (!(pR->iCpsr & ECpuThumb)) {
#endif
		// check for coprocessor instructions
		//		10987654321098765432109876543210
		// CDP:	cond1110op1 CRn CRd cp_#op20CRm
		// LDC: cond110PUNW1Rn  CRd cp_#offset
		// STC: cond110PUNW0Rn  CRd cp_#offset
		// MRC: cond1110op11CRn Rd  cp_#op21CRm
		// MCR: cond1110op10CRn Rd  cp_#op21CRm
		// ext:	cond11000x0xRn  CRd cp_#offset
		//CDP2:	11111110xxxxxxxxxxxxxxxxxxx0xxxx
		//LDC2: 1111110xxxx1xxxxxxxxxxxxxxxxxxxx
		//STC2: 1111110xxxx0xxxxxxxxxxxxxxxxxxxx
		//MRC2:	11111110xxx1xxxxxxxxxxxxxxx1xxxx
		//MCR2:	11111110xxx0xxxxxxxxxxxxxxx1xxxx
		//MRRC: cond11100101Rn  Rd  cp_#opc CRm
		//MCRR: cond11100100Rn  Rd  cp_#opc CRm
		//
		//NEON data processing:
		//      1111001xxxxxxxxxxxxxxxxxxxxxxxxx
		//NEON element/structure load/store:
		//      11110100xxx0xxxxxxxxxxxxxxxxxxxx
		//
		// Coprocessor instructions have 2nd hex digit (bits 24-27) C,D,E.
		// The coprocessor number is in bits 8-11.
		// NEON instructions have 1st hex digits F2, F3, or F4x where x is even
		// No coprocessor number, route to cp 10

		TUint32 hex2 = (opcode>>24) & 0x0f;

		if (hex2==0xc || hex2==0xd || hex2==0xe)
			cpnum=(opcode>>8)&0x0f;
#ifdef __CPU_ARMV7
		else if ((opcode>>28)==0xf && (hex2==2 || hex2==3 || (hex2==4 && ((opcode>>20)&1)==0)))
			cpnum=VFP_CPID_S;
#endif

#ifdef __SUPPORT_THUMB_INTERWORKING
		}
#endif
#ifdef __CPU_ARMV7
	else
		{
		// Check for coprocessor instructions (thumb mode, so only first halfword)
		//			5432109876543210 5432109876543210
		//   CDP:	11101110op1 CRn  CRd cp_#op20CRm
		//   LDC:	1110110PUNW1Rn   CRd cp_#offset
		//   STC:	1110110PUNW0Rn   CRd cp_#offset
		//   MRC:	11101110op11CRn  Rd  cp_#op21CRm
		//   MCR:	11101110op10CRn  Rd  cp_#op21CRm
		//	CDP2:	11111110xxxxxxxx xxxxxxxxxx0xxxxx
		//	LDC2:	1111110xxxx1xxxx xxxxxxxxxxxxxxxx
		//	STC2:	1111110xxxx0xxxx xxxxxxxxxxxxxxxx
		//	MRC2:	11111110xxx1xxxx xxxxxxxxxx1xxxxx
		//	MCR2:	11111110xxx0xxxx xxxxxxxxxx1xxxxx
		//  MRRC:	111011100101Rn   Rd  cp_#opc CRm
		//  MCRR:	111011100100Rn   Rd  cp_#opc CRm
		//
		// Operations starting 1111 are not currently valid for VFP/NEON
		// but are handled here in case of future development or 
		// alternative coprocessors
		//
		// NEON data processing:
		//			111x1111xxxxxxxx xxxxxxxxxxxxxxxx
		// NEON element/structure load/store:
		//			11111001xxx0xxxx xxxxxxxxxxxxxxxx
		//
		// Coprocessor instructions have first hex digit E or F
		// and second C, D or E
		// The coprocessor number is in bits 8-11 of the second halfword
		// NEON instructions have first 2 hex digits EF, FF or F9
		// No coprocessor number, route to cp 10

		const TUint32 hex12 = opcode >> 8;

		if ((hex12 & 0xe0) == 0xe0)
			{
			const TUint32 hex2 = hex12 & 0xf;
			if (hex2 == 0xc || hex2 == 0xd || hex2 == 0xe)
				{
				TArmExcInfo nextInstruction = *pR;
				nextInstruction.iR15 += 2;
				GetUndefinedInstruction(&nextInstruction);
				cpnum = (nextInstruction.iFaultAddress >> 8) & 0x0f;
				}
			else
				{
				if (hex12 == 0xef || hex12 == 0xf9 || hex12 == 0xff)
					cpnum = VFP_CPID_S;
				}
			}
		}
#endif // __CPU_ARMV7

		if (cpnum >= 0)
			{
			__KTRACE_OPT(KEVENT,Kern::Printf("VFP Instruction %08x", opcode));
			TInt r = HandleVFPOperation(pR);
			if (r==KErrNone)
				return;
			__KTRACE_OPT(KEVENT,Kern::Printf("VFP Instruction returned %d", r));
			}
		}
#endif // __CPU_HAS_VFP

	NKern::LockSystem();
	if (pT->iThreadType==EThreadUser && mode==EUserMode)
		{
		TExcType type=(TExcType)12;
		if (pT->IsExceptionHandled(type))
			{
			pT->iFlags |= KThreadFlagLastChance;
			NKern::UnlockSystem();

			// tweak context to call exception handler
			PushExcInfoOnUserStack(pR, type);
			pR->iR15 = (TUint32)pT->iOwningProcess->iReentryPoint;
			pR->iR4 = KModuleEntryReasonException;
#ifdef __SUPPORT_THUMB_INTERWORKING
			pR->iCpsr &= ~ECpuThumb;
#endif
			return;
			}
		}
	// Fault system before attempting to signal kernel event handler as going to
	// crash anyway so no point trying to handle the event.  Also, stops
	// D_EXC hanging system on crash of critical/permanent thread.
    if (pT->iFlags & (KThreadFlagSystemCritical|KThreadFlagSystemPermanent))
		Exc::Fault(aPtr);
	NKern::UnlockSystem();	

	TUint m = DKernelEventHandler::Dispatch(EEventHwExc, pR, NULL);
	if (m & (TUint)DKernelEventHandler::EExcHandled)
		return;

//	__KTRACE_OPT(KPANIC,DumpExcInfoX(*pR));
	DumpExcInfoX(*pR);

	// panic current thread
	K::PanicKernExec(ECausedException);
	}

EXPORT_C void Exc::Fault(TAny* aExcInfo)
	{
#ifdef __SMP__
	TSubScheduler* ss = &SubScheduler();
	if (!ss)
		ss = &TheSubSchedulers[0];
	ss->iSSX.iExcInfo = aExcInfo;
	SFullArmRegSet* a = ss->iSSX.iRegs;
	if (!a)
		a = &DefaultRegSet;
#else
	TheScheduler.i_ExcInfo = aExcInfo;
	SFullArmRegSet* a = (SFullArmRegSet*)TheScheduler.i_Regs;
#endif
	if (aExcInfo)
		{
		Arm::SaveState(*a);
		Arm::UpdateState(*a, *(TArmExcInfo*)aExcInfo);
		}
	TExcInfo e;
	e.iCodeAddress = (TAny*)a->iN.iR15;
	e.iDataAddress = (TAny*)a->iB[0].iDFAR;
	e.iExtraData = (TInt)a->iB[0].iDFSR;
//	__KTRACE_OPT(KPANIC,DumpExcInfoX(*pR));
	DumpFullRegSet(*a);
	TheSuperPage().iKernelExcId = a->iExcCode;
	TheSuperPage().iKernelExcInfo = e;
	Kern::Fault("Exception", K::ESystemException);
	}

extern "C" void ExcFault(TAny* aExcInfo)
	{
	Exc::Fault(aExcInfo);
	}

void DArmPlatThread::DoExit2()
	{
#ifdef __CPU_HAS_VFP
	for (TInt cpu = 0; cpu < NKern::NumberOfCpus(); cpu++)
		{
		// Ensure that if this thread object is re-used then it gets a fresh context
		if (Arm::VfpThread[cpu] == &iNThread)
			Arm::VfpThread[cpu] = NULL;
#ifndef __SMP__
		Arm::ModifyFpExc(VFP_FPEXC_EX | VFP_FPEXC_EN, 0); // Disable VFP here for unicore
#endif
		}
#endif
	}


/** Sets the function used to handle bounces from VFP hardware.

Used by a VFP coprocessor support kernel extension to register its
bounce handler.

@publishedPartner
@released
 */
EXPORT_C void Arm::SetVfpBounceHandler(TVfpBounceHandler aHandler)
	{
	Arm::VfpBounceHandler = aHandler;
	}


/** Sets the default value of FPSCR in the VFP hardware.

Used by a VFP coprocessor support kernel extension to enable a
better default mode than RunFast.

@publishedPartner
@released
 */
EXPORT_C void Arm::SetVfpDefaultFpScr(TUint32 aFpScr)
	{
#ifdef __CPU_HAS_VFP
	VfpDefaultFpScr = aFpScr;
#endif
	}


#ifdef __CPU_HAS_VFP

#ifndef __SMP__
extern void DoSaveVFP(void*);
#endif
extern void DoRestoreVFP(const void*);

GLDEF_C TInt HandleVFPOperation(TAny* aPtr)
	{
	NThread* pC = NCurrentThread();

	if (Arm::FpExc() & VFP_FPEXC_EN)
		{
		// Coprocessor already enabled so it must be a real exception
		if (Arm::VfpBounceHandler)
			return Arm::VfpBounceHandler((TArmExcInfo*)aPtr);
		else
			return KErrGeneral;	
		}

	NKern::Lock();

	// Enable access for this thread, clear any exceptional condition
	TUint32 oldFpExc = Arm::ModifyFpExc(VFP_FPEXC_EX, VFP_FPEXC_EN);

#ifndef __SMP__
	if (Arm::VfpThread[0] != pC)
		{
		// Only for unicore - SMP explicitly saves the current context and disables VFP
		// when a thread is descheduled in case it runs on a different core next time
		if (Arm::VfpThread[0])
			{
			DoSaveVFP(Arm::VfpThread[0]->iExtraContext);
			Arm::VfpThread[0]->ModifyFpExc(VFP_FPEXC_EN, 0); // Take access away from previous thread
			}
		DoRestoreVFP(pC->iExtraContext);	// Restore this thread's context
		Arm::VfpThread[0] = pC;
		}
#else
	const TInt currentCpu = NKern::CurrentCpu();
	if (Arm::VfpThread[currentCpu] != pC)
		{
		DoRestoreVFP(pC->iExtraContext);	// Restore this thread's context
		Arm::VfpThread[currentCpu] = pC;
		for (TInt cpu = 0; cpu < NKern::NumberOfCpus(); cpu++)
			{
			if (cpu != currentCpu)
				{
				TUint32 pCcopy = (TUint32)pC;
				__e32_atomic_cas_rlx32(&Arm::VfpThread[cpu], &pCcopy, NULL);
				}
			}
		}
#endif

	// Put FPEXC back how it was in case there was a pending exception, but keep enable bit on
	Arm::SetFpExc(oldFpExc | VFP_FPEXC_EN);
	NKern::Unlock();

	return KErrNone;
	}
#endif // __CPU_HAS_VFP

#ifdef _DEBUG
extern "C" void __FaultIpcClientNotNull()
	{
	K::Fault(K::EIpcClientNotNull);
	}
#endif
