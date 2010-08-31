// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\arm\ncthrd.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#define __INCLUDE_REG_OFFSETS__
#include <arm.h>
#include <arm_gic.h>
#include <arm_scu.h>
#include <arm_tmr.h>
#include <nk_irq.h>

const TInt KNThreadMinStackSize = 0x100;	// needs to be enough for interrupt + reschedule stack

// Called by a thread when it first runs
extern "C" void __StartThread();

// Initialise CPU registers
extern void initialiseState(TInt aCpu, TSubScheduler* aSS);

extern "C" void ExcFault(TAny*);

extern TUint32 __mpid();
extern void InitTimestamp(TSubScheduler* aSS, SNThreadCreateInfo& aInfo);

TInt NThread::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	// Assert ParameterBlockSize is not negative and is a multiple of 8 bytes
	__NK_ASSERT_ALWAYS((aInfo.iParameterBlockSize&0x80000007)==0);
	__NK_ASSERT_ALWAYS(aInfo.iStackBase && aInfo.iStackSize>=aInfo.iParameterBlockSize+KNThreadMinStackSize);
	TInt cpu = -1;
	TSubScheduler* ss = 0;
	new (this) NThread;
	if (aInitial)
		{
		cpu = __e32_atomic_add_ord32(&TheScheduler.iNumCpus, 1);
		aInfo.iCpuAffinity = cpu;
		// OK since we can't migrate yet
		ss = &TheSubSchedulers[cpu];
		ss->iCurrentThread = this;
		ss->iDeferShutdown = 0;
		iRunCount.i64 = UI64LIT(1);
		iActiveState = 1;
		__KTRACE_OPT(KBOOT,DEBUGPRINT("Init: cpu=%d ss=%08x", cpu, ss));
		if (cpu)
			{
			initialiseState(cpu,ss);

			ArmLocalTimer& T = LOCAL_TIMER;
			T.iWatchdogDisable = E_ArmTmrWDD_1;
			T.iWatchdogDisable = E_ArmTmrWDD_2;
			T.iTimerCtrl = 0;
			T.iTimerIntStatus = E_ArmTmrIntStatus_Event;
			T.iWatchdogCtrl = 0;
			T.iWatchdogIntStatus = E_ArmTmrIntStatus_Event;

			NIrq::HwInit2AP();
			T.iTimerCtrl = E_ArmTmrCtrl_IntEn | E_ArmTmrCtrl_Reload | E_ArmTmrCtrl_Enable;

			__e32_atomic_ior_ord32(&TheScheduler.iThreadAcceptCpus, 1<<cpu);
			__e32_atomic_ior_ord32(&TheScheduler.iIpiAcceptCpus, 1<<cpu);
			__e32_atomic_ior_ord32(&TheScheduler.iCpusNotIdle, 1<<cpu);
			__e32_atomic_add_ord32(&TheScheduler.iCCRequestLevel, 1);
			__KTRACE_OPT(KBOOT,DEBUGPRINT("AP MPID=%08x",__mpid()));
			}
		else
			{
			Arm::DefaultDomainAccess = Arm::Dacr();
			Arm::ModifyCar(0, 0x00f00000);		// full access to CP10, CP11
			Arm::DefaultCoprocessorAccess = Arm::Car();
			}
		}
	TInt r=NThreadBase::Create(aInfo,aInitial);
	if (r!=KErrNone)
		return r;
	if (!aInitial)
		{
		aInfo.iPriority = 0;
		TLinAddr stack_top = (TLinAddr)iStackBase + (TLinAddr)iStackSize;
		TLinAddr sp = stack_top;
		TUint32 pb = (TUint32)aInfo.iParameterBlock;
		SThreadStackStub* tss = 0;
		if (aInfo.iParameterBlockSize)
			{
			tss = (SThreadStackStub*)stack_top;
			--tss;
			tss->iExcCode = SThreadExcStack::EStub;
			tss->iR15 = 0;
			tss->iCPSR = 0;
			sp = (TLinAddr)tss;
			sp -= (TLinAddr)aInfo.iParameterBlockSize;
			wordmove((TAny*)sp, aInfo.iParameterBlock, aInfo.iParameterBlockSize);
			pb = (TUint32)sp;
			tss->iPBlock = sp;
			}
		SThreadInitStack* tis = (SThreadInitStack*)sp;
		--tis;
		memclr(tis, sizeof(SThreadInitStack));
		iSavedSP = (TLinAddr)tis;
#ifdef __CPU_HAS_VFP
		tis->iR.iFpExc = VFP_FPEXC_THRD_INIT;
#endif
		tis->iR.iCar = Arm::DefaultCoprocessorAccess;
		tis->iR.iDacr = Arm::DefaultDomainAccess;
		tis->iR.iSpsrSvc = MODE_SVC;
		tis->iR.iSPRschdFlg = TLinAddr(&tis->iX) | 1;
		tis->iR.iR15 = (TUint32)&__StartThread;

		tis->iX.iR0 = pb;
		tis->iX.iR4 = (TUint32)this;
		tis->iX.iR11 = stack_top;
		tis->iX.iExcCode = SThreadExcStack::EInit;
		tis->iX.iR15 = (TUint32)aInfo.iFunction;
		tis->iX.iCPSR = MODE_SVC;
		}
	else
		{
		NKern::EnableAllInterrupts();

#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined(__NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)

		if (cpu == 0) 
			{
			// start global timer if necessary
			ArmGlobalTimer& GT = GLOBAL_TIMER;
			if (!(GT.iTimerCtrl & E_ArmGTmrCtrl_TmrEnb))
				{
				// timer not currently enabled
				GT.iTimerCtrl = 0;
				__e32_io_completion_barrier();
				GT.iTimerStatus = E_ArmGTmrStatus_Event;
				__e32_io_completion_barrier();
				GT.iTimerCountLow = 0;
				GT.iTimerCountHigh = 0;
				__e32_io_completion_barrier();
				GT.iTimerCtrl = E_ArmGTmrCtrl_TmrEnb;	// enable timer with prescale factor of 1
				__e32_io_completion_barrier();
				}
			}
		
#endif

		// start local timer
		ArmLocalTimer& T = LOCAL_TIMER;
		T.iTimerCtrl = E_ArmTmrCtrl_IntEn | E_ArmTmrCtrl_Reload | E_ArmTmrCtrl_Enable;
		// Initialise timestamp
		InitTimestamp(ss, aInfo);
		}
	AddToEnumerateList();
	InitLbInfo();
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::ENanoThreadCreate,this);
#endif
	return KErrNone;
	}


/**	Called from generic layer when thread is killed asynchronously.

	For ARM, save reason for last user->kernel switch (if any) so that user
	context can be accessed from EDebugEventRemoveThread hook.  Must be done
	before forcing the thread to exit as this alters the saved return address
	which is used to figure out where the context is saved.

	@pre kernel locked
	@post kernel locked
 */
void NThreadBase::OnKill()
	{
	}

/**	Called from generic layer when thread exits.

	For ARM, save that if the thread terminates synchronously the last
	user->kernel switch was an exec call.  Do nothing if non-user thread or
	reason already saved in OnKill().

	@pre kernel locked
	@post kernel locked
	@see OnKill
 */
void NThreadBase::OnExit()
	{
	}


void DumpExcInfo(TArmExcInfo& a)
	{
	DEBUGPRINT("Exc %1d Cpsr=%08x FAR=%08x FSR=%08x",a.iExcCode,a.iCpsr,a.iFaultAddress,a.iFaultStatus);
	DEBUGPRINT(" R0=%08x  R1=%08x  R2=%08x  R3=%08x",a.iR0,a.iR1,a.iR2,a.iR3);
	DEBUGPRINT(" R4=%08x  R5=%08x  R6=%08x  R7=%08x",a.iR4,a.iR5,a.iR6,a.iR7);
	DEBUGPRINT(" R8=%08x  R9=%08x R10=%08x R11=%08x",a.iR8,a.iR9,a.iR10,a.iR11);
	DEBUGPRINT("R12=%08x R13=%08x R14=%08x R15=%08x",a.iR12,a.iR13,a.iR14,a.iR15);
	DEBUGPRINT("R13Svc=%08x R14Svc=%08x SpsrSvc=%08x",a.iR13Svc,a.iR14Svc,a.iSpsrSvc);

	TInt irq = NKern::DisableAllInterrupts();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* ct = ss.iCurrentThread;
	TInt inc = TInt(ss.iSSX.iIrqNestCount);
	TInt cpu = ss.iCpuNum;
	TInt klc = ss.iKernLockCount;
	NKern::RestoreInterrupts(irq);
	DEBUGPRINT("Thread %T, CPU %d, KLCount=%d, IrqNest=%d", ct, cpu, klc, inc);
	}

void DumpFullRegSet(SFullArmRegSet& a)
	{
	SNormalRegs& r = a.iN;
	DEBUGPRINT("MODE_USR:");
	DEBUGPRINT(" R0=%08x  R1=%08x  R2=%08x  R3=%08x", r.iR0,  r.iR1,  r.iR2,  r.iR3);
	DEBUGPRINT(" R4=%08x  R5=%08x  R6=%08x  R7=%08x", r.iR4,  r.iR5,  r.iR6,  r.iR7);
	DEBUGPRINT(" R8=%08x  R9=%08x R10=%08x R11=%08x", r.iR8,  r.iR9,  r.iR10, r.iR11);
	DEBUGPRINT("R12=%08x R13=%08x R14=%08x R15=%08x", r.iR12, r.iR13, r.iR14, r.iR15);
	DEBUGPRINT("CPSR=%08x", r.iFlags);
	DEBUGPRINT("MODE_FIQ:");
	DEBUGPRINT(" R8=%08x  R9=%08x R10=%08x R11=%08x",  r.iR8Fiq,  r.iR9Fiq,  r.iR10Fiq, r.iR11Fiq);
	DEBUGPRINT("R12=%08x R13=%08x R14=%08x SPSR=%08x", r.iR12Fiq, r.iR13Fiq, r.iR14Fiq, r.iSpsrFiq);
	DEBUGPRINT("MODE_IRQ:");
	DEBUGPRINT("R13=%08x R14=%08x SPSR=%08x", r.iR13Irq, r.iR14Irq, r.iSpsrIrq);
	DEBUGPRINT("MODE_SVC:");
	DEBUGPRINT("R13=%08x R14=%08x SPSR=%08x", r.iR13Svc, r.iR14Svc, r.iSpsrSvc);
	DEBUGPRINT("MODE_ABT:");
	DEBUGPRINT("R13=%08x R14=%08x SPSR=%08x", r.iR13Abt, r.iR14Abt, r.iSpsrAbt);
	DEBUGPRINT("MODE_UND:");
	DEBUGPRINT("R13=%08x R14=%08x SPSR=%08x", r.iR13Und, r.iR14Und, r.iSpsrUnd);
//	DEBUGPRINT("MODE_MON:");
//	DEBUGPRINT("R13=%08x R14=%08x SPSR=%08x", r.iR13Mon, r.iR14Mon, r.iSpsrMon);

	SAuxiliaryRegs& aux = a.iA;
	DEBUGPRINT("TEEHBR=%08x  CPACR=%08x", aux.iTEEHBR, aux.iCPACR);

	SBankedRegs& b = a.iB[0];
	DEBUGPRINT(" SCTLR=%08x  ACTLR=%08x   PRRR=%08x   NMRR=%08x", b.iSCTLR, b.iACTLR, b.iPRRR, b.iNMRR);
	DEBUGPRINT("  DACR=%08x  TTBR0=%08x  TTBR1=%08x  TTBCR=%08x", b.iDACR, b.iTTBR0, b.iTTBR1, b.iTTBCR);
	DEBUGPRINT("  VBAR=%08x FCSEID=%08x CTXIDR=%08x", b.iVBAR, b.iFCSEIDR, b.iCTXIDR);
	DEBUGPRINT("Thread ID     RWRW=%08x   RWRO=%08x   RWNO=%08x", b.iRWRWTID, b.iRWROTID, b.iRWNOTID);
	DEBUGPRINT("  DFSR=%08x   DFAR=%08x   IFSR=%08x   IFAR=%08x", b.iDFSR, b.iDFAR, b.iIFSR, b.iIFAR);
	DEBUGPRINT(" ADFSR=%08x              AIFSR=%08x", b.iADFSR, b.iAIFSR);
#ifdef __CPU_HAS_VFP
	DEBUGPRINT("FPEXC %08x", a.iMore[0]);
#endif
	DEBUGPRINT("ExcCode %08x", a.iExcCode);
	}


#define CONTEXT_ELEMENT_UNDEFINED(val)							\
	{															\
	TArmContextElement::EUndefined,								\
	val,														\
	0,															\
	0															\
	}

#define CONTEXT_ELEMENT_EXCEPTION(reg)							\
	{															\
	TArmContextElement::EOffsetFromStackTop,					\
	((sizeof(SThreadExcStack)-_FOFF(SThreadExcStack,reg))>>2),	\
	0,															\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED(reg)							\
	{															\
	TArmContextElement::EOffsetFromSp,							\
	(_FOFF(SThreadReschedStack,reg)>>2),						\
	0,															\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED_SP()							\
	{															\
	TArmContextElement::EOffsetFromSpBic3,						\
	(_FOFF(SThreadReschedStack,iSPRschdFlg)>>2),				\
	0,															\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED_SP_PLUS(offset)					\
	{															\
	TArmContextElement::EOffsetFromSpBic3_1,					\
	(_FOFF(SThreadReschedStack,iSPRschdFlg)>>2),				\
	(offset),													\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED_SP_OFFSET(offset)				\
	{															\
	TArmContextElement::EOffsetFromSpBic3_2,					\
	(_FOFF(SThreadReschedStack,iSPRschdFlg)>>2),				\
	(offset),													\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED_IRQ(reg)						\
	{															\
	TArmContextElement::EOffsetFromSpBic3_2,					\
	(_FOFF(SThreadReschedStack,iSPRschdFlg)>>2),				\
	((_FOFF(SThreadIrqStack,reg)-sizeof(SThreadReschedStack))>>2),	\
	0															\
	}

#define CONTEXT_ELEMENT_RESCHED_INIT(reg)						\
	{															\
	TArmContextElement::EOffsetFromSpBic3_2,					\
	(_FOFF(SThreadReschedStack,iSPRschdFlg)>>2),				\
	((_FOFF(SThreadInitStack,reg)-sizeof(SThreadReschedStack))>>2),	\
	0															\
	}


const TArmContextElement ContextTableException[] =
	{
	CONTEXT_ELEMENT_EXCEPTION(iR0),
	CONTEXT_ELEMENT_EXCEPTION(iR1),
	CONTEXT_ELEMENT_EXCEPTION(iR2),
	CONTEXT_ELEMENT_EXCEPTION(iR3),
	CONTEXT_ELEMENT_EXCEPTION(iR4),
	CONTEXT_ELEMENT_EXCEPTION(iR5),
	CONTEXT_ELEMENT_EXCEPTION(iR6),
	CONTEXT_ELEMENT_EXCEPTION(iR7),
	CONTEXT_ELEMENT_EXCEPTION(iR8),
	CONTEXT_ELEMENT_EXCEPTION(iR9),
	CONTEXT_ELEMENT_EXCEPTION(iR10),
	CONTEXT_ELEMENT_EXCEPTION(iR11),
	CONTEXT_ELEMENT_EXCEPTION(iR12),
	CONTEXT_ELEMENT_EXCEPTION(iR13usr),
	CONTEXT_ELEMENT_EXCEPTION(iR14usr),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_EXCEPTION(iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

const TArmContextElement ContextTableUndefined[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for non dying threads which have been preempted by an interrupt
// while in user mode.  
const TArmContextElement ContextTableUserInterrupt[] =
	{
	CONTEXT_ELEMENT_EXCEPTION(iR0),
	CONTEXT_ELEMENT_EXCEPTION(iR1),
	CONTEXT_ELEMENT_EXCEPTION(iR2),
	CONTEXT_ELEMENT_EXCEPTION(iR3),
	CONTEXT_ELEMENT_EXCEPTION(iR4),
	CONTEXT_ELEMENT_EXCEPTION(iR5),
	CONTEXT_ELEMENT_EXCEPTION(iR6),
	CONTEXT_ELEMENT_EXCEPTION(iR7),
	CONTEXT_ELEMENT_EXCEPTION(iR8),
	CONTEXT_ELEMENT_EXCEPTION(iR9),
	CONTEXT_ELEMENT_EXCEPTION(iR10),
	CONTEXT_ELEMENT_EXCEPTION(iR11),
	CONTEXT_ELEMENT_EXCEPTION(iR12),
	CONTEXT_ELEMENT_EXCEPTION(iR13usr),
	CONTEXT_ELEMENT_EXCEPTION(iR14usr),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_EXCEPTION(iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been preempted by an interrupt while in
// supervisor mode in the SWI handler either before the return address was
// saved or after the registers were restored.
const TArmContextElement ContextTableSvsrInterrupt1[] =
	{
	CONTEXT_ELEMENT_EXCEPTION(iR0),
	CONTEXT_ELEMENT_EXCEPTION(iR1),
	CONTEXT_ELEMENT_EXCEPTION(iR2),
	CONTEXT_ELEMENT_EXCEPTION(iR3),
	CONTEXT_ELEMENT_EXCEPTION(iR4),
	CONTEXT_ELEMENT_EXCEPTION(iR5),
	CONTEXT_ELEMENT_EXCEPTION(iR6),
	CONTEXT_ELEMENT_EXCEPTION(iR7),
	CONTEXT_ELEMENT_EXCEPTION(iR8),
	CONTEXT_ELEMENT_EXCEPTION(iR9),
	CONTEXT_ELEMENT_EXCEPTION(iR10),
	CONTEXT_ELEMENT_EXCEPTION(iR11),
	CONTEXT_ELEMENT_EXCEPTION(iR12),
	CONTEXT_ELEMENT_EXCEPTION(iR13usr),
	CONTEXT_ELEMENT_EXCEPTION(iR14usr),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for non-dying threads blocked on their request semaphore.
const TArmContextElement ContextTableWFAR[] =
	{
	CONTEXT_ELEMENT_EXCEPTION(iR0),
	CONTEXT_ELEMENT_EXCEPTION(iR1),
	CONTEXT_ELEMENT_EXCEPTION(iR2),
	CONTEXT_ELEMENT_EXCEPTION(iR3),
	CONTEXT_ELEMENT_EXCEPTION(iR4),
	CONTEXT_ELEMENT_EXCEPTION(iR5),
	CONTEXT_ELEMENT_EXCEPTION(iR6),
	CONTEXT_ELEMENT_EXCEPTION(iR7),
	CONTEXT_ELEMENT_EXCEPTION(iR8),
	CONTEXT_ELEMENT_EXCEPTION(iR9),
	CONTEXT_ELEMENT_EXCEPTION(iR10),
	CONTEXT_ELEMENT_EXCEPTION(iR11),
	CONTEXT_ELEMENT_EXCEPTION(iR12),
	CONTEXT_ELEMENT_EXCEPTION(iR13usr),
	CONTEXT_ELEMENT_EXCEPTION(iR14usr),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_EXCEPTION(iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

const TArmContextElement ContextTableExec[] =
	{
	CONTEXT_ELEMENT_EXCEPTION(iR0),
	CONTEXT_ELEMENT_EXCEPTION(iR1),
	CONTEXT_ELEMENT_EXCEPTION(iR2),
	CONTEXT_ELEMENT_EXCEPTION(iR3),
	CONTEXT_ELEMENT_EXCEPTION(iR4),
	CONTEXT_ELEMENT_EXCEPTION(iR5),
	CONTEXT_ELEMENT_EXCEPTION(iR6),
	CONTEXT_ELEMENT_EXCEPTION(iR7),
	CONTEXT_ELEMENT_EXCEPTION(iR8),
	CONTEXT_ELEMENT_EXCEPTION(iR9),
	CONTEXT_ELEMENT_EXCEPTION(iR10),
	CONTEXT_ELEMENT_EXCEPTION(iR11),
	CONTEXT_ELEMENT_EXCEPTION(iR12),
	CONTEXT_ELEMENT_EXCEPTION(iR13usr),
	CONTEXT_ELEMENT_EXCEPTION(iR14usr),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_EXCEPTION(iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context at the point where
// Reschedule() returns.
// Used for kernel threads.
const TArmContextElement ContextTableKernel[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_RESCHED_SP(),			// supervisor stack pointer before reschedule
	CONTEXT_ELEMENT_UNDEFINED(0),			// supervisor lr is unknown
	CONTEXT_ELEMENT_RESCHED(iR15),			// return address from reschedule
	CONTEXT_ELEMENT_UNDEFINED(ESvcMode),	// can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context at the point where
// NKern::Unlock() or NKern::PreemptionPoint() returns.
// Used for kernel threads.
const TArmContextElement ContextTableKernel1[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(4),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(8),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(12),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(16),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(20),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(24),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(28),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(32),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_RESCHED_SP_PLUS(40),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(36),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(36),
	CONTEXT_ELEMENT_UNDEFINED(ESvcMode),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context at the point where
// NKern::FSWait() or NKern::WaitForAnyRequest() returns.
// Used for kernel threads.
const TArmContextElement ContextTableKernel2[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(4),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(8),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(12),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(16),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(20),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(24),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(28),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(32),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_RESCHED_SP_PLUS(40),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(36),
	CONTEXT_ELEMENT_RESCHED_SP_OFFSET(36),
	CONTEXT_ELEMENT_UNDEFINED(ESvcMode),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context at the point where
// an interrupt taken in supervisor mode returns.
// Used for kernel threads.
const TArmContextElement ContextTableKernel3[] =
	{
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR0),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR1),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR2),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR3),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR4),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR5),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR6),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR7),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR8),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR9),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR10),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR11),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR12),
	CONTEXT_ELEMENT_RESCHED_SP_PLUS((sizeof(SThreadExcStack)+8)),
	CONTEXT_ELEMENT_RESCHED_IRQ(iR14svc),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iR15),
	CONTEXT_ELEMENT_RESCHED_IRQ(iX.iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context at the point where
// Exec::WaitForAnyRequest() returns.
// Used for kernel threads.
const TArmContextElement ContextTableKernel4[] =
	{
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR0),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR1),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR2),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR3),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR4),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR5),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR6),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR7),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR8),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR9),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR10),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR11),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR12),
	CONTEXT_ELEMENT_RESCHED_SP_PLUS(sizeof(SThreadExcStack)),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR15),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iR15),
	CONTEXT_ELEMENT_RESCHED_INIT(iX.iCPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

const TArmContextElement* const ThreadUserContextTables[] =
	{
	ContextTableUndefined,					// EContextNone
	ContextTableException,					// EContextException
	ContextTableUndefined,					// EContextUndefined
	ContextTableUserInterrupt,				// EContextUserInterrupt
	ContextTableUndefined,					// EContextUserInterruptDied (not used)
	ContextTableSvsrInterrupt1,				// EContextSvsrInterrupt1
	ContextTableUndefined,					// EContextSvsrInterrupt1Died (not used)
	ContextTableUndefined,					// EContextSvsrInterrupt2 (not used)
	ContextTableUndefined,					// EContextSvsrInterrupt2Died (not used)
	ContextTableWFAR,						// EContextWFAR
	ContextTableUndefined,					// EContextWFARDied (not used)
	ContextTableExec,						// EContextExec
	ContextTableKernel,						// EContextKernel
	ContextTableKernel1,					// EContextKernel1
	ContextTableKernel2,					// EContextKernel2
	ContextTableKernel3,					// EContextKernel3
	ContextTableKernel4,					// EContextKernel4
	0 // Null terminated
	};

/**  Return table of pointers to user context tables.

	Each user context table is an array of TArmContextElement objects, one per
	ARM CPU register, in the order defined in TArmRegisters.

	The master table contains pointers to the user context tables in the order
	defined in TUserContextType.  There are as many user context tables as
	scenarii leading a user thread to switch to privileged mode.

	Stop-mode debug agents should use this function to store the address of the
	master table at a location known to the host debugger.  Run-mode debug
	agents are advised to use NKern::GetUserContext() and
	NKern::SetUserContext() instead.

	@return A pointer to the master table.  The master table is NULL
	terminated.  The master and user context tables are guaranteed to remain at
	the same location for the lifetime of the OS execution so it is safe the
	cache the returned address.

	@see UserContextType
	@see TArmContextElement
	@see TArmRegisters
	@see TUserContextType
	@see NKern::SetUserContext
	@see NKern::GetUserContext

	@publishedPartner
 */
EXPORT_C const TArmContextElement* const* NThread::UserContextTables()
	{
	return &ThreadUserContextTables[0];
	}


/** Get a value which indicates where a thread's user mode context is stored.

	@return A value that can be used as an index into the tables returned by
	NThread::UserContextTables().

	@pre any context
	@pre kernel locked
	@post kernel locked
 
	@see UserContextTables
	@publishedPartner
 */
EXPORT_C NThread::TUserContextType NThread::UserContextType()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NThread::UserContextType");

	/*
	The SMP nanokernel always saves R0-R12,R13usr,R14usr,ExcCode,PC,CPSR on any
	entry to the kernel, so getting the user context is always the same.
	The only possible problem is an FIQ occurring immediately after any other
	exception, before the registers have been saved. In this case the registers
	saved by the FIQ will be the ones observed and they will be correct except
	that the CPSR value will indicate a mode other than USR, which can be used
	to detect the condition.
	*/
	return EContextException;
	}


// Enter and return with kernel locked
void NThread::GetUserContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask)
	{
	NThread* pC = NCurrentThreadL();
	TSubScheduler* ss = 0;
	if (pC != this)
		{
		AcqSLock();
		if (iWaitState.ThreadIsDead() || i_NThread_Initial)
			{
			RelSLock();
			aAvailRegistersMask = 0;
			return;
			}
		if (iReady && iParent->iReady)
			{
			ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
			ss->iReadyListLock.LockOnly();
			}
		if (iCurrent)
			{
			// thread is actually running on another CPU
			// interrupt that CPU and wait for it to enter interrupt mode
			// this allows a snapshot of the thread user state to be observed
			// and ensures the thread cannot return to user mode
			send_resched_ipi_and_wait(iLastCpu);
			}
		}
	SThreadExcStack* txs = (SThreadExcStack*)(TLinAddr(iStackBase) + TLinAddr(iStackSize));
	--txs;
	if (txs->iExcCode <= SThreadExcStack::EInit)	// if not, thread never entered user mode
		{
		aContext.iR0 = txs->iR0;
		aContext.iR1 = txs->iR1;
		aContext.iR2 = txs->iR2;
		aContext.iR3 = txs->iR3;
		aContext.iR4 = txs->iR4;
		aContext.iR5 = txs->iR5;
		aContext.iR6 = txs->iR6;
		aContext.iR7 = txs->iR7;
		aContext.iR8 = txs->iR8;
		aContext.iR9 = txs->iR9;
		aContext.iR10 = txs->iR10;
		aContext.iR11 = txs->iR11;
		aContext.iR12 = txs->iR12;
		aContext.iR13 = txs->iR13usr;
		aContext.iR14 = txs->iR14usr;
		aContext.iR15 = txs->iR15;
		aContext.iFlags = txs->iCPSR;
		if ((aContext.iFlags & 0x1f) == 0x10)
			aAvailRegistersMask = 0x1ffffu;	// R0-R15,CPSR all valid
		else
			{
			aContext.iFlags = 0x10;			// account for FIQ in SVC case
			aAvailRegistersMask = 0x0ffffu;	// CPSR not valid
			}
		}
	if (pC != this)
		{
		if (ss)
			ss->iReadyListLock.UnlockOnly();
		RelSLock();
		}
	}

class TGetContextIPI : public TGenericIPI
	{
public:
	void Get(TInt aCpu, TArmRegSet& aContext, TUint32& aAvailRegistersMask);
	static void Isr(TGenericIPI*);
public:
	TArmRegSet* iContext;
	TUint32* iAvailRegsMask;
	};

extern "C" TLinAddr get_sp_svc();
extern "C" TLinAddr get_lr_svc();
extern "C" TInt get_kernel_context_type(TLinAddr /*aReschedReturn*/);

void TGetContextIPI::Isr(TGenericIPI* aPtr)
	{
	TGetContextIPI& ipi = *(TGetContextIPI*)aPtr;
	TArmRegSet& a = *ipi.iContext;
	SThreadExcStack* txs = (SThreadExcStack*)get_sp_svc();
	a.iR0 = txs->iR0;
	a.iR1 = txs->iR1;
	a.iR2 = txs->iR2;
	a.iR3 = txs->iR3;
	a.iR4 = txs->iR4;
	a.iR5 = txs->iR5;
	a.iR6 = txs->iR6;
	a.iR7 = txs->iR7;
	a.iR8 = txs->iR8;
	a.iR9 = txs->iR9;
	a.iR10 = txs->iR10;
	a.iR11 = txs->iR11;
	a.iR12 = txs->iR12;
	a.iR13 = TUint32(txs) + sizeof(SThreadExcStack);
	a.iR14 = get_lr_svc();
	a.iR15 = txs->iR15;
	a.iFlags = txs->iCPSR;
	*ipi.iAvailRegsMask = 0x1ffffu;
	}

void TGetContextIPI::Get(TInt aCpu, TArmRegSet& aContext, TUint32& aAvailRegsMask)
	{
	iContext = &aContext;
	iAvailRegsMask = &aAvailRegsMask;
	Queue(&Isr, 1u<<aCpu);
	WaitCompletion();
	}

void GetRegs(TArmRegSet& aContext, TLinAddr aStart, TUint32 aMask)
	{
	TUint32* d = (TUint32*)&aContext;
	const TUint32* s = (const TUint32*)aStart;
	for (; aMask; aMask>>=1, ++d)
		{
		if (aMask & 1)
			*d = *s++;
		}
	}

// Enter and return with kernel locked
void NThread::GetSystemContext(TArmRegSet& aContext, TUint32& aAvailRegsMask)
	{
	aAvailRegsMask = 0;
	NThread* pC = NCurrentThreadL();
	__NK_ASSERT_ALWAYS(pC!=this);
	TSubScheduler* ss = 0;
	AcqSLock();
	if (iWaitState.ThreadIsDead())
		{
		RelSLock();
		return;
		}
	if (iReady && iParent->iReady)
		{
		ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
		ss->iReadyListLock.LockOnly();
		}
	if (iCurrent)
		{
		// thread is actually running on another CPU
		// use an interprocessor interrupt to get a snapshot of the state
		TGetContextIPI ipi;
		ipi.Get(iLastCpu, aContext, aAvailRegsMask);
		}
	else
		{
		// thread is not running and can't start
		SThreadReschedStack* trs = (SThreadReschedStack*)iSavedSP;
		TInt kct = get_kernel_context_type(trs->iR15);
		__NK_ASSERT_ALWAYS(kct>=0);	// couldn't match return address from reschedule
		TLinAddr sp = trs->iSPRschdFlg &~ 3;
		switch (kct)
			{
			case 0:	// thread not yet started
			case 5:	// Exec::WaitForAnyRequest()
				GetRegs(aContext, sp, 0x01fffu);
				aContext.iR13 = sp + sizeof(SThreadExcStack);
				GetRegs(aContext, sp+64, 0x18000u);
				aAvailRegsMask =0x1bfffu;
				break;
			case 1:	// unlock
			case 2:	// preemption point
			case 3:	// NKern::WaitForAnyRequest() or NKern::FSWait()
				GetRegs(aContext, sp+4, 0x08ff0u);
				aContext.iR14 = aContext.iR15;
				aContext.iR13 = sp+40;
				aAvailRegsMask =0x0eff0u;
				break;
			case 4:	// IRQ/FIQ
				GetRegs(aContext, sp+4, 0x04000u);
				GetRegs(aContext, sp+8, 0x01fffu);
				GetRegs(aContext, sp+64, 0x18000u);
				aContext.iR13 = sp + sizeof(SThreadExcStack) + 8;
				aAvailRegsMask =0x1ffffu;
				break;
			default:
				__NK_ASSERT_ALWAYS(0);
			}
		}
	if (ss)
		ss->iReadyListLock.UnlockOnly();
	RelSLock();
	}

// Enter and return with kernel locked
void NThread::SetUserContext(const TArmRegSet& aContext, TUint32& aRegMask)
	{
	NThread* pC = NCurrentThreadL();
	TSubScheduler* ss = 0;
	if (pC != this)
		{
		AcqSLock();
		if (iWaitState.ThreadIsDead() || i_NThread_Initial)
			{
			RelSLock();
			aRegMask = 0;
			return;
			}
		if (iReady && iParent->iReady)
			{
			ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
			ss->iReadyListLock.LockOnly();
			}
		if (iCurrent)
			{
			// thread is actually running on another CPU
			// interrupt that CPU and wait for it to enter interrupt mode
			// this allows a snapshot of the thread user state to be observed
			// and ensures the thread cannot return to user mode
			send_resched_ipi_and_wait(iLastCpu);
			}
		}
	SThreadExcStack* txs = (SThreadExcStack*)(TLinAddr(iStackBase) + TLinAddr(iStackSize));
	--txs;
	aRegMask &= 0x1ffffu;
	if (txs->iExcCode <= SThreadExcStack::EInit)	// if not, thread never entered user mode
		{
		if (aRegMask & 0x0001u)
			txs->iR0 = aContext.iR0;
		if (aRegMask & 0x0002u)
			txs->iR1 = aContext.iR1;
		if (aRegMask & 0x0004u)
			txs->iR2 = aContext.iR2;
		if (aRegMask & 0x0008u)
			txs->iR3 = aContext.iR3;
		if (aRegMask & 0x0010u)
			txs->iR4 = aContext.iR4;
		if (aRegMask & 0x0020u)
			txs->iR5 = aContext.iR5;
		if (aRegMask & 0x0040u)
			txs->iR6 = aContext.iR6;
		if (aRegMask & 0x0080u)
			txs->iR7 = aContext.iR7;
		if (aRegMask & 0x0100u)
			txs->iR8 = aContext.iR8;
		if (aRegMask & 0x0200u)
			txs->iR9 = aContext.iR9;
		if (aRegMask & 0x0400u)
			txs->iR10 = aContext.iR10;
		if (aRegMask & 0x0800u)
			txs->iR11 = aContext.iR11;
		if (aRegMask & 0x1000u)
			txs->iR12 = aContext.iR12;
		if (aRegMask & 0x2000u)
			txs->iR13usr = aContext.iR13;
		if (aRegMask & 0x4000u)
			txs->iR14usr = aContext.iR14;
		if (aRegMask & 0x8000u)
			txs->iR15 = aContext.iR15;
		// Assert that target thread is in USR mode, and update only the flags part of the PSR
		__NK_ASSERT_ALWAYS((txs->iCPSR & 0x1f) == 0x10);
		if (aRegMask & 0x10000u)
			{
			// NZCVQ.......GE3-0................
			const TUint32 writableFlags = 0xF80F0000;
			txs->iCPSR &= ~writableFlags;
			txs->iCPSR |= aContext.iFlags & writableFlags;
			}
		}
	else
		aRegMask = 0;
	if (pC != this)
		{
		if (ss)
			ss->iReadyListLock.UnlockOnly();
		RelSLock();
		}
	}

/** Get (subset of) user context of specified thread.

	The nanokernel does not systematically save all registers in the supervisor
	stack on entry into privileged mode and the exact subset depends on why the
	switch to privileged mode occured.  So in general only a subset of the
	register set is available.

	@param aThread	Thread to inspect.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TArmRegSet structure where the context is
	copied.

	@param aAvailRegistersMask Bit mask telling which subset of the context is
	available and has been copied to aContext (1: register available / 0: not
	available).  Bit 0 stands for register R0.

	@see TArmRegSet
	@see ThreadSetUserContext

	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadGetUserContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadGetUserContext");
	TArmRegSet& a = *(TArmRegSet*)aContext;
	memclr(aContext, sizeof(TArmRegSet));
	NKern::Lock();
	aThread->GetUserContext(a, aAvailRegistersMask);
	NKern::Unlock();
	}

/** Get (subset of) system context of specified thread.
  
	@param aThread	Thread to inspect.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TArmRegSet structure where the context is
	copied.

	@param aAvailRegistersMask Bit mask telling which subset of the context is
	available and has been copied to aContext (1: register available / 0: not
	available).  Bit 0 stands for register R0.

	@see TArmRegSet
	@see ThreadSetUserContext

	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadGetSystemContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadGetSystemContext");
	TArmRegSet& a = *(TArmRegSet*)aContext;
	memclr(aContext, sizeof(TArmRegSet));
	NKern::Lock();
	aThread->GetSystemContext(a, aAvailRegistersMask);
	NKern::Unlock();
	}

/** Set (subset of) user context of specified thread.

	@param aThread	Thread to modify.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TArmRegSet structure containing the context
	to set.  The values of registers which aren't part of the context saved
	on the supervisor stack are ignored.

	@see TArmRegSet
	@see ThreadGetUserContext

  	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadSetUserContext(NThread* aThread, TAny* aContext)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadSetUserContext");
	TArmRegSet& a = *(TArmRegSet*)aContext;
	TUint32 mask = 0x1ffffu;
	NKern::Lock();
	aThread->SetUserContext(a, mask);
	NKern::Unlock();
	}


#ifdef __CPU_HAS_VFP
extern void VfpContextSave(void*);
#endif
/** Complete the saving of a thread's context

This saves the VFP/NEON registers if necessary once we know that we are definitely
switching threads.

@internalComponent
*/
void NThread::CompleteContextSave()
	{
#ifdef __CPU_HAS_VFP
	if (Arm::VfpThread[NKern::CurrentCpu()] == this)
		{
		VfpContextSave(iExtraContext); // Disables VFP
		}
#endif
	}


extern "C" TInt HandleSpecialOpcode(TArmExcInfo* aContext, TInt aType)
	{
	TUint32 cpsr = aContext->iCpsr;
	TUint32 mode = cpsr & 0x1f;
	TUint32 opcode = aContext->iFaultStatus;

	// Coprocessor abort from CP15 or E7FFDEFF -> crash immediately
	if (		(aType==15 && opcode!=0xee000f20)
			||	(aType==32 && opcode==0xe7ffdeff)
			||	(aType==33 && opcode==0xdeff)
		)
		{
		if (mode != 0x10)
			ExcFault(aContext);	// crash instruction in privileged mode
		return 0;	// crash instruction in user mode - handle normally
		}
	if (		(aType==15 && opcode==0xee000f20)
			||	(aType==32 && opcode==0xe7ffdefc)
			||	(aType==33 && opcode==0xdefc)
		)
		{
		// checkpoint
		__KTRACE_OPT(KPANIC,DumpExcInfo(*aContext));
		if (aType==32)
			aContext->iR15 += 4;
		else
			aContext->iR15 += 2;
		return 1;
		}
	return 0;
	}


TInt NKern::QueueUserModeCallback(NThreadBase* aThread, TUserModeCallback* aCallback)
	{
	__e32_memory_barrier();
	if (aCallback->iNext != KUserModeCallbackUnqueued)
		return KErrInUse;
	if (aThread->i_NThread_Initial)
		return KErrArgument;
	TInt result = KErrDied;
	NKern::Lock();
	TUserModeCallback* listHead = aThread->iUserModeCallbacks;
	do	{
		if (TLinAddr(listHead) & 3)
			goto done;	// thread exiting
		aCallback->iNext = listHead;
		} while (!__e32_atomic_cas_ord_ptr(&aThread->iUserModeCallbacks, &listHead, aCallback));
	result = KErrNone;

	if (!listHead)	// if this isn't first callback someone else will have done this bit
		{
		/*
		 * If aThread is currently running on another CPU we need to send an IPI so
		 * that it will enter kernel mode and run the callback.
		 * The synchronization is tricky here. We want to check if the thread is
		 * running and if so on which core. We need to avoid any possibility of
		 * the thread entering user mode without having seen the callback,
		 * either because we thought it wasn't running so didn't send an IPI or
		 * because the thread migrated after we looked and we sent the IPI to
		 * the wrong processor. Sending a redundant IPI is not a problem (e.g.
		 * because the thread is running in kernel mode - which we can't tell -
		 * or because the thread stopped running after we looked)
		 * The following events are significant:
		 * Event A:	Target thread writes to iCurrent when it starts running
		 * Event B: Target thread reads iUserModeCallbacks before entering user
		 *			mode
		 * Event C: This thread writes to iUserModeCallbacks
		 * Event D: This thread reads iCurrent to check if aThread is running
		 * There is a DMB and DSB between A and B since A occurs with the ready
		 * list lock for the CPU involved or the thread lock for aThread held
		 * and this lock is released before B occurs.
		 * There is a DMB between C and D (part of __e32_atomic_cas_ord_ptr).
		 * Any observer which observes B must also have observed A.
		 * Any observer which observes D must also have observed C.
		 * If aThread observes B before C (i.e. enters user mode without running
		 * the callback) it must observe A before C and so it must also observe
		 * A before D (i.e. D reads the correct value for iCurrent).
		 */
		TInt current = aThread->iCurrent;
		if (current)
			{
			TInt cpu = current & NSchedulable::EReadyCpuMask;
			if (cpu != NKern::CurrentCpu())
				send_resched_ipi(cpu);
			}
		}
done:
	NKern::Unlock();
	return result;
	}

