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
// e32\nkern\arm\ncthrd.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#define __INCLUDE_REG_OFFSETS__
#include <arm.h>

const TInt KNThreadMinStackSize = 0x100;	// needs to be enough for interrupt + reschedule stack

// Called by a thread when it first runs
extern void __StartThread();

// Called by a thread which has been forced to exit
// Interrupts off here, kernel unlocked
extern void __DoForcedExit();

void NThreadBase::SetEntry(NThreadFunction aFunc)
	{
	TUint32* sp=(TUint32*)iSavedSP;
	sp[SP_R5]=(TUint32)aFunc;
	}

TInt NThread::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	// Assert ParameterBlockSize is not negative and is a multiple of 8 bytes
	__NK_ASSERT_ALWAYS((aInfo.iParameterBlockSize&0x80000007)==0);

	__NK_ASSERT_ALWAYS(aInfo.iStackBase && aInfo.iStackSize>=aInfo.iParameterBlockSize+KNThreadMinStackSize);
	TInt r=NThreadBase::Create(aInfo,aInitial);
	if (r!=KErrNone)
		return r;
	if (!aInitial)
		{
		TUint32* sp=(TUint32*)(iStackBase+iStackSize-aInfo.iParameterBlockSize);
		TUint32 r6=(TUint32)aInfo.iParameterBlock;
		if (aInfo.iParameterBlockSize)
			{
			wordmove(sp,aInfo.iParameterBlock,aInfo.iParameterBlockSize);
			r6=(TUint32)sp;
			}
		*--sp=(TUint32)__StartThread;		// PC
		*--sp=0;							// R11
		*--sp=0;							// R10
		*--sp=0;							// R9
		*--sp=0;							// R8
		*--sp=0;							// R7
		*--sp=r6;							// R6
		*--sp=(TUint32)aInfo.iFunction;		// R5
		*--sp=(TUint32)this;				// R4
		*--sp=0x13;							// SPSR_SVC
		*--sp=0;							// R14_USR
		*--sp=0;							// R13_USR
#ifdef __CPU_ARM_USE_DOMAINS
		*--sp=Arm::DefaultDomainAccess;		// DACR
#endif
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
		*--sp=Arm::DefaultCoprocessorAccess;	// CAR
#endif
#ifdef __CPU_HAS_VFP
		*--sp=VFP_FPEXC_THRD_INIT;			// FPEXC
#endif
#ifdef __CPU_HAS_CP15_THREAD_ID_REG
		*--sp=0;							// TID
#endif
#ifdef __CPU_SUPPORT_THUMB2EE
		*--sp=0;							// ThumbEE Base
#endif
		iSavedSP=(TLinAddr)sp;
		}
	else
		{
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
#ifdef __CPU_HAS_VFP
#ifdef __CPU_XSCALE__
		Arm::ModifyCar(0, 0x0c00);			// enable CP10, CP11
#else
		Arm::ModifyCar(0, 0x00f00000);		// full access to CP10, CP11
#endif
#endif
		Arm::DefaultCoprocessorAccess = Arm::Car();
#endif
		NKern::EnableAllInterrupts();
		}
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
	if (iUserContextType != NThread::EContextNone)
		{
		NThread::TUserContextType t = ((NThread*)this)->UserContextType();
		switch (t)
			{
		case NThread::EContextUserInterrupt:
			t = NThread::EContextUserInterruptDied;
			break;
		case NThread::EContextSvsrInterrupt1:
			t = NThread::EContextSvsrInterrupt1Died;
			break;
		case NThread::EContextSvsrInterrupt2:
			t = NThread::EContextSvsrInterrupt2Died;
			break;
		case NThread::EContextWFAR:
			t = NThread::EContextWFARDied;
			break;
		default:
			// NOP
			break;
			}
		iUserContextType = t;
		}
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
	CHECK_PRECONDITIONS(MASK_KERNEL_LOCKED,"NThreadBase::OnExit");				
	if (iUserContextType == NThread::EContextUndefined)
		iUserContextType = NThread::EContextExec;
	}

void NThreadBase::ForceExit()
	{
	TUint32* sp=(TUint32*)iSavedSP;
	sp[SP_PC]=(TUint32)__DoForcedExit;
	}

void DumpExcInfo(TArmExcInfo& a)
	{
	DEBUGPRINT("Exc %1d Cpsr=%08x FAR=%08x FSR=%08x",a.iExcCode,a.iCpsr,a.iFaultAddress,a.iFaultStatus);
	DEBUGPRINT(" R0=%08x  R1=%08x  R2=%08x  R3=%08x",a.iR0,a.iR1,a.iR2,a.iR3);
	DEBUGPRINT(" R4=%08x  R5=%08x  R6=%08x  R7=%08x",a.iR4,a.iR5,a.iR6,a.iR7);
	DEBUGPRINT(" R8=%08x  R9=%08x R10=%08x R11=%08x",a.iR8,a.iR9,a.iR10,a.iR11);
	DEBUGPRINT("R12=%08x R13=%08x R14=%08x R15=%08x",a.iR12,a.iR13,a.iR14,a.iR15);
	DEBUGPRINT("R13Svc=%08x R14Svc=%08x SpsrSvc=%08x",a.iR13Svc,a.iR14Svc,a.iSpsrSvc);
	DEBUGPRINT("Thread %T, KernCSLocked=%d",TheScheduler.iCurrentThread,TheScheduler.iKernCSLocked);
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

#define CONTEXT_ELEMENT_UNDEFINED(val)						\
	{														\
	TArmContextElement::EUndefined,							\
	val														\
	}

#define CONTEXT_ELEMENT_EXCEPTION(reg)						\
	{														\
	TArmContextElement::EOffsetFromStackTop,				\
	(- (-sizeof(TArmExcInfo)+_FOFF(TArmExcInfo,reg)) )>>2	\
	}

#define CONTEXT_ELEMENT_FROM_SP(offset)						\
	{														\
	TArmContextElement::EOffsetFromSp,						\
	offset													\
	}

#define CONTEXT_ELEMENT_FROM_STACK_TOP(offset)				\
	{														\
	TArmContextElement::EOffsetFromStackTop,				\
	offset													\
	}

#define CONTEXT_ELEMENT_SP_PLUS(offset)						\
	{														\
	TArmContextElement::ESpPlusOffset,						\
	offset													\
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
	CONTEXT_ELEMENT_EXCEPTION(iR13),
	CONTEXT_ELEMENT_EXCEPTION(iR14),
	CONTEXT_ELEMENT_EXCEPTION(iR15),
	CONTEXT_ELEMENT_EXCEPTION(iCpsr),
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
	CONTEXT_ELEMENT_FROM_STACK_TOP(6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(4),
	CONTEXT_ELEMENT_FROM_STACK_TOP(3),
	CONTEXT_ELEMENT_FROM_SP(SP_R4),
	CONTEXT_ELEMENT_FROM_SP(SP_R5),
	CONTEXT_ELEMENT_FROM_SP(SP_R6),
	CONTEXT_ELEMENT_FROM_SP(SP_R7),
	CONTEXT_ELEMENT_FROM_SP(SP_R8),
	CONTEXT_ELEMENT_FROM_SP(SP_R9),
	CONTEXT_ELEMENT_FROM_SP(SP_R10),
	CONTEXT_ELEMENT_FROM_SP(SP_R11),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8),		// interrupted CPSR
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been asynchronously killed after being
// preempted by interrupt while in user mode.

const TArmContextElement ContextTableUserInterruptDied[] =
	{
	CONTEXT_ELEMENT_FROM_STACK_TOP(6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(4),
	CONTEXT_ELEMENT_FROM_STACK_TOP(3),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8),		// interrupted CPSR
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been preempted by an interrupt while in
// supervisor mode in the SWI handler either before the return address was
// saved or after the registers were restored.

const TArmContextElement ContextTableSvsrInterrupt1[] =
	{
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+2),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+3),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+4),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+5),
	CONTEXT_ELEMENT_FROM_SP(SP_R4),
	CONTEXT_ELEMENT_FROM_SP(SP_R5),
	CONTEXT_ELEMENT_FROM_SP(SP_R6),
	CONTEXT_ELEMENT_FROM_SP(SP_R7),
	CONTEXT_ELEMENT_FROM_SP(SP_R8),
	CONTEXT_ELEMENT_FROM_SP(SP_R9),
	CONTEXT_ELEMENT_FROM_SP(SP_R10),
	CONTEXT_ELEMENT_FROM_SP(SP_R11),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+6),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+6),  // r15 = r12
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been asynchronously killed while in the situation
// described above (see ContextTableSvsrInterrupt1).

const TArmContextElement ContextTableSvsrInterrupt1Died[] =
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
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been preempted by an interrupt while in
// supervisor mode in the SWI handler after the return address was saved.

const TArmContextElement ContextTableSvsrInterrupt2[] =
	{
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+2),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+3),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+4),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+5),
	CONTEXT_ELEMENT_FROM_SP(SP_R4),
	CONTEXT_ELEMENT_FROM_SP(SP_R5),
	CONTEXT_ELEMENT_FROM_SP(SP_R6),
	CONTEXT_ELEMENT_FROM_SP(SP_R7),
	CONTEXT_ELEMENT_FROM_SP(SP_R8),
	CONTEXT_ELEMENT_FROM_SP(SP_R9),
	CONTEXT_ELEMENT_FROM_SP(SP_R10),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_FROM_SP(SP_NEXT+USER_MEMORY_GUARD_SAVE_WORDS+6),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads which have been asynchronously killed while in the situation
// described above (see ContextTableSvsrInterrupt2).

const TArmContextElement ContextTableSvsrInterrupt2Died[] =
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
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for non-dying threads blocked on their request semaphore.

const TArmContextElement ContextTableWFAR[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_SP(SP_R4),
	CONTEXT_ELEMENT_FROM_SP(SP_R5),
	CONTEXT_ELEMENT_FROM_SP(SP_R6),
	CONTEXT_ELEMENT_FROM_SP(SP_R7),
	CONTEXT_ELEMENT_FROM_SP(SP_R8),
	CONTEXT_ELEMENT_FROM_SP(SP_R9),
	CONTEXT_ELEMENT_FROM_SP(SP_R10),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_SP(SP_SPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for threads killed asynchronously while blocked on their request
// semaphore.

const TArmContextElement ContextTableWFARDied[] =
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
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_SP(SP_SPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

const TArmContextElement ContextTableExec[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_STACK_TOP(10),
	CONTEXT_ELEMENT_FROM_STACK_TOP(9),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8),
	CONTEXT_ELEMENT_FROM_STACK_TOP(7),
	CONTEXT_ELEMENT_FROM_STACK_TOP(6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(4),
	CONTEXT_ELEMENT_FROM_STACK_TOP(3),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_UNDEFINED(EUserMode),  // can't get flags so just use 'user mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used to retrieve a thread's kernel side context.
// Used for kernel threads.
const TArmContextElement ContextTableKernel[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_SP(SP_R4),			// r4 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R5),			// r5 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R6),			// r6 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R7),			// r7 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R8),			// r8 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R9),			// r9 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R10),		// r10 before reschedule
	CONTEXT_ELEMENT_FROM_SP(SP_R11),		// r11 before reschedule
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_SP_PLUS(SP_NEXT),		// supervisor stack pointer before reschedule
	CONTEXT_ELEMENT_UNDEFINED(0),			// supervisor lr is unknown
	CONTEXT_ELEMENT_FROM_SP(SP_PC),			// return address from reschedule
	CONTEXT_ELEMENT_UNDEFINED(ESvcMode),	// can't get flags so just use 'supervisor mode'
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for non dying threads which are in a user callback while returning
// from having been preempted by an interrupt while in user mode.  

const TArmContextElement ContextTableUserIntrCallback[] =
	{
	CONTEXT_ELEMENT_FROM_STACK_TOP(6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(4),
	CONTEXT_ELEMENT_FROM_STACK_TOP(3),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+9),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+8),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+7),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+4),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+3),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8+USER_MEMORY_GUARD_SAVE_WORDS+2),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8),		// interrupted CPSR
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

// Table used for non-dying threads which are in a user callback while returning
// from being blocked on their request semaphore.

const TArmContextElement ContextTableWFARCallback[] =
	{
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_STACK_TOP(11),
	CONTEXT_ELEMENT_FROM_STACK_TOP(10),
	CONTEXT_ELEMENT_FROM_STACK_TOP(9),
	CONTEXT_ELEMENT_FROM_STACK_TOP(8),
	CONTEXT_ELEMENT_FROM_STACK_TOP(7),
	CONTEXT_ELEMENT_FROM_STACK_TOP(6),
	CONTEXT_ELEMENT_FROM_STACK_TOP(5),
	CONTEXT_ELEMENT_FROM_STACK_TOP(2),
	CONTEXT_ELEMENT_UNDEFINED(0),
	CONTEXT_ELEMENT_FROM_SP(SP_R13U),
	CONTEXT_ELEMENT_FROM_SP(SP_R14U),
	CONTEXT_ELEMENT_FROM_STACK_TOP(1),
	CONTEXT_ELEMENT_FROM_SP(SP_SPSR),
	CONTEXT_ELEMENT_UNDEFINED(0),
	};

const TArmContextElement* const ThreadUserContextTables[] =
	{
	ContextTableUndefined, // EContextNone
	ContextTableException,
	ContextTableUndefined,
	ContextTableUserInterrupt,
	ContextTableUserInterruptDied,
	ContextTableSvsrInterrupt1,
	ContextTableSvsrInterrupt1Died,
	ContextTableSvsrInterrupt2,
	ContextTableSvsrInterrupt2Died,
	ContextTableWFAR,
	ContextTableWFARDied,
	ContextTableExec,
	ContextTableKernel,
	ContextTableUserIntrCallback,
	ContextTableWFARCallback,
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


#ifndef __USER_CONTEXT_TYPE_MACHINE_CODED__
extern TBool RescheduledAfterInterrupt(TUint32 /*aAddr*/);

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
	// Dying thread? use context saved earlier by kernel
	if (iCsFunction == ECSExitInProgress)
		return (TUserContextType)iUserContextType;

	// Check for EContextNone and EContextException
	// Also EContextUserIntrCallback and EContextWFARCallback
	if(iUserContextType<=EContextException || iUserContextType==EContextUserIntrCallback
			|| iUserContextType==EContextWFARCallback)
		return (TUserContextType)iUserContextType;

	// Getting current thread context? must be in exec call as exception
	// and dying thread cases were tested above.
	if (this == NCurrentThread())
		return EContextExec;

	// Check what caused the thread to enter supervisor mode
	TUint32* sst=(TUint32*)((TUint32)iStackBase+(TUint32)iStackSize);
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TInt n=sst-sp;						// number of words on the supervisor stack
	TUint32 resched_ret=sp[SP_PC];		// return address from reschedule
	if (RescheduledAfterInterrupt(resched_ret))
		{
		// thread was preempted due to an interrupt
		// interrupt and reschedule will have pushed 20+EXTRA words onto the stack
		if ((sp[SP_NEXT]&EMaskMode)==EUserMode)   // interrupted mode = user?
			return NThread::EContextUserInterrupt;
		if (n<(30+EXTRA_WORDS))	// n<30 if interrupt occurred in exec call entry before r3-r10 saved
			{					// or after r3-r10 restored
			if (n==(20+EXTRA_WORDS))
				{
				// interrupt before return address, r11 were saved or after registers restored
				return EContextSvsrInterrupt1;
				}
			else
				{
				// interrupt after return address, r11 saved
				return EContextSvsrInterrupt2;
				}
			}
		// thread was interrupted in supervisor mode
		// return address and r3-r11 were saved
		}

	// Transition to supervisor mode must have been due to a SWI
	if (n==(15+EXTRA_WORDS))
		{
		// thread must have blocked doing Exec::WaitForAnyRequest
		return EContextWFAR;
		}

	// Thread must have been in a SLOW or UNPROTECTED Exec call
	return EContextExec;
	}

#endif // __USER_CONTEXT_TYPE_MACHINE_CODED__

// Enter and return with kernel locked
void NThread::GetContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask, const TArmContextElement* aContextTable)
	{
	TUint32* sp  = (TUint32*)iSavedSP;
	TUint32* st  = (TUint32*)((TUint32)iStackBase+(TUint32)iStackSize);
	TArmReg* out = (TArmReg*)(&aContext);
	TBool currentThread = (NCurrentThread() == this);
	
	aAvailRegistersMask = 0;
	if (iNState == EDead)
		{// This thread's stack may no longer exist so just exit.
		return;
		}

	// Copy available context into provided structure.
	for (TInt i = 0; i<KArmRegisterCount; ++i)
		{
		TInt v = aContextTable[i].iValue;
		TInt t = aContextTable[i].iType;
		if(!currentThread && t==TArmContextElement::EOffsetFromSp) 
			{
			// thread has been preempted, it is safe to fetch its context
			// from the info saved in Reschedule().
			v = sp[v];
			aAvailRegistersMask |= (1<<i);
			}
		else if(t==TArmContextElement::EOffsetFromStackTop)
			{
			v = st[-v];
			aAvailRegistersMask |= (1<<i);
			}
		else if(!currentThread && t==TArmContextElement::ESpPlusOffset)
			{
			v = (TInt)(sp+v);
			aAvailRegistersMask |= (1<<i);
			}
		out[i] = v;
		}

	// Getting context of current thread? some values can be fetched directly
	// from the registers if they are not available from the stack.
	if (currentThread && aContextTable[EArmSp].iType == TArmContextElement::EOffsetFromSp)
		{
		Arm::GetUserSpAndLr(out+EArmSp);
		aAvailRegistersMask |= (1<<EArmSp) | (1<<EArmLr);
		}
	}

// Enter and return with kernel locked
void NThread::GetUserContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask)
	{
	TUserContextType type=UserContextType();
	NThread::GetContext(aContext, aAvailRegistersMask, UserContextTables()[type]);
	}

// Enter and return with kernel locked
void NThread::GetSystemContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask)
	{
	NThread::GetContext(aContext, aAvailRegistersMask, UserContextTables()[EContextKernel]);
	}

// Enter and return with kernel locked
void NThread::SetUserContext(const TArmRegSet& aContext)
	{
	if (iNState == EDead)
		{// This thread's stack may no longer exist so just exit.
		return;
		}
	TUserContextType type=UserContextType();
	const TArmContextElement* c = NThread::UserContextTables()[type];
	TUint32* sp  = (TUint32*)iSavedSP;
	TUint32* st  = (TUint32*)((TUint32)iStackBase+(TUint32)iStackSize);
	TArmReg* in = (TArmReg*)(&aContext);
	TBool currentThread = (NCurrentThread() == this);

	// Check that target thread is in USR mode, and update only the flags part of the PSR
	TUint32 tFlags = 0;
	TUint32* tFlagsPtr = &tFlags;
	TUint32 flagsCtxValue = c[EArmFlags].iValue;
	switch (c[EArmFlags].iType)						// describes how to interpret flagsCtxValue
	{
	case TArmContextElement::EUndefined:
		// Flags register not saved; not necessarily an error, but we can't update the flags
		tFlags = flagsCtxValue;						// use mode bits of flagsCtxValue itself
		break;

	case TArmContextElement::EOffsetFromStackTop:
		// Flags register saved, flagsCtxValue is offset from ToS
		tFlagsPtr = &st[-flagsCtxValue];
		break;

	case TArmContextElement::EOffsetFromSp:
		// Flags register saved, flagsCtxValue is offset from SP
		if (!currentThread)
			tFlagsPtr = &sp[flagsCtxValue];
		else
			{
			// This can only occur when the thread is exiting. Therefore,
			// we allow it, but the changed values will never be used.
			tFlags = 0x10;
			}
		break;

	default:
		// Assertion below will fail with default value ...
		;
	}

	tFlags = *tFlagsPtr;							// retrieve saved flags
	__NK_ASSERT_ALWAYS((tFlags & 0x1f) == 0x10);	// target thread must be in USR mode
	const TUint32 writableFlags = 0xF80F0000;		// NZCVQ.......GE3-0................
	tFlags &= ~writableFlags;
	tFlags |= in[EArmFlags] & writableFlags;
	*tFlagsPtr = tFlags;							// update saved flags

	// Copy provided context into stack if possible
	for (TInt i = 0; i<KArmRegisterCount; ++i)
		{
		// The Flags were already processed above, and we don't allow
		// changing the DACR, so we can just skip these two index values
		if (i == EArmFlags || i == EArmDacr)
			continue;

		TInt v = c[i].iValue;
		TInt t = c[i].iType;
		if(!currentThread && t==TArmContextElement::EOffsetFromSp) 
			{
			// thread has been preempted, it is safe to change context
			// saved in Reschedule().
			sp[v] = in[i];
			}
		if(t==TArmContextElement::EOffsetFromStackTop)
			st[-v] = in[i];
		}

	// Current thread? some values can be loaded straight into the registers
	// if they haven't been stored on the stack yet.
	if (currentThread && c[EArmSp].iType == TArmContextElement::EOffsetFromSp)
		Arm::SetUserSpAndLr(in+EArmSp);
	}

// Modify a non-running thread's user stack pointer
// Enter and return with kernel locked
void NThread::ModifyUsp(TLinAddr aUsp)
	{
	// Check what caused the thread to enter supervisor mode
	TUint32* sst=(TUint32*)((TUint32)iStackBase+(TUint32)iStackSize);
	if (iSpare3)
		{
		// exception caused transition to supervisor mode
		TArmExcInfo& e=((TArmExcInfo*)sst)[-1];
		e.iR13=aUsp;
		return;
		}
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	sp[SP_R13U]=aUsp;
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
	TArmRegSet& a=*(TArmRegSet*)aContext;
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
	TArmRegSet& a=*(TArmRegSet*)aContext;
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
	TArmRegSet& a=*(TArmRegSet*)aContext;
	NKern::Lock();
	aThread->SetUserContext(a);
	NKern::Unlock();
	}

/** @internalComponent */
void NKern::ThreadModifyUsp(NThread* aThread, TLinAddr aUsp)
	{
	NKern::Lock();
	aThread->ModifyUsp(aUsp);
	NKern::Unlock();
	}

#ifdef __CPU_ARM_USE_DOMAINS
TUint32 NThread::Dacr()
	{
	if (this==TheScheduler.iCurrentThread)
		return Arm::Dacr();
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 dacr=sp[SP_DACR];
	NKern::Unlock();
	return dacr;
	}

void NThread::SetDacr(TUint32 aDacr)
	{
	if (this==TheScheduler.iCurrentThread)
		Arm::SetDacr(aDacr);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	sp[SP_DACR]=aDacr;
	NKern::Unlock();
	}

TUint32 NThread::ModifyDacr(TUint32 aClearMask, TUint32 aSetMask)
	{
	if (this==TheScheduler.iCurrentThread)
		return Arm::ModifyDacr(aClearMask,aSetMask);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 dacr=sp[SP_DACR];
	sp[SP_DACR]=(dacr&~aClearMask)|aSetMask;
	NKern::Unlock();
	return dacr;
	}
#endif

#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
void NThread::SetCar(TUint32 aCar)
	{
	if (this==TheScheduler.iCurrentThread)
		Arm::SetCar(aCar);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	sp[SP_CAR]=aCar;
	NKern::Unlock();
	}
#endif



/** Get the saved coprocessor access register value for a thread

@return The saved value of the CAR, 0 if CPU doesn't have CAR
@pre	Don't call from ISR

@publishedPartner
@released
 */
EXPORT_C TUint32 NThread::Car()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NThread::Car");				
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
	if (this==TheScheduler.iCurrentThread)
		return Arm::Car();
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 car=sp[SP_CAR];
	NKern::Unlock();
	return car;
#else
	return 0;
#endif
	}



/** Modify the saved coprocessor access register value for a thread
	Does nothing if CPU does not have CAR.

@param	aClearMask	Mask of bits to clear	(1 = clear this bit)
@param	aSetMask	Mask of bits to set		(1 = set this bit)
@return The original saved value of the CAR, 0 if CPU doesn't have CAR
@pre	Don't call from ISR

@publishedPartner
@released
 */
EXPORT_C TUint32 NThread::ModifyCar(TUint32 aClearMask, TUint32 aSetMask)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NThread::ModifyCar");				
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
	if (this==TheScheduler.iCurrentThread)
		return Arm::ModifyCar(aClearMask,aSetMask);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 car=sp[SP_CAR];
	sp[SP_CAR]=(car&~aClearMask)|aSetMask;
	NKern::Unlock();
	return car;
#else
	return 0;
#endif
	}

#ifdef __CPU_HAS_VFP
void NThread::SetFpExc(TUint32 aVal)
	{
	if (this==TheScheduler.iCurrentThread)
		Arm::SetFpExc(aVal);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	sp[SP_FPEXC]=aVal;
	NKern::Unlock();
	}
#endif



/** Get the saved VFP FPEXC register value for a thread

@return The saved value of FPEXC, 0 if VFP not present
@pre	Don't call from ISR

@publishedPartner
@released
 */
EXPORT_C TUint32 NThread::FpExc()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NThread::FpExc");				
#ifdef __CPU_HAS_VFP
	if (this==TheScheduler.iCurrentThread)
		return Arm::FpExc();
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 r=sp[SP_FPEXC];
	NKern::Unlock();
	return r;
#else
	return 0;
#endif
	}



/** Modify the saved VFP FPEXC register value for a thread
	Does nothing if VFP not present

@param	aClearMask	Mask of bits to clear	(1 = clear this bit)
@param	aSetMask	Mask of bits to set		(1 = set this bit)
@return The original saved value of FPEXC, 0 if VFP not present
@pre	Don't call from ISR

@publishedPartner
@released
 */
EXPORT_C TUint32 NThread::ModifyFpExc(TUint32 aClearMask, TUint32 aSetMask)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"NThread::ModifyFpExc");				
#ifdef __CPU_HAS_VFP
	if (this==TheScheduler.iCurrentThread)
		return Arm::ModifyFpExc(aClearMask,aSetMask);
	NKern::Lock();
	TUint32* sp=(TUint32*)iSavedSP;		// saved supervisor stack pointer
	TUint32 r=sp[SP_FPEXC];
	sp[SP_FPEXC]=(r&~aClearMask)|aSetMask;
	NKern::Unlock();
	return r;
#else
	return 0;
#endif
	}

