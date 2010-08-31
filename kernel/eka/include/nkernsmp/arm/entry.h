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
// e32/include/nkernsmp/arm/entry.h
// 
//

#include <arm_gic.h>
#include <arm_tmr.h>


extern "C" {

extern void __ArmVectorReset();
extern void __ArmVectorUndef();
extern void __ArmVectorSwi();
extern void __ArmVectorAbortPrefetch();
extern void __ArmVectorAbortData();
extern void __ArmVectorReserved();
extern void __ArmVectorIrq();
extern void __ArmVectorFiq();

#define __DECLARE_UNDEFINED_INSTRUCTION_HANDLER		asm(".word __ArmVectorUndef ")
#define __DECLARE_PREFETCH_ABORT_HANDLER			asm(".word __ArmVectorAbortPrefetch ")
#define __DECLARE_DATA_ABORT_HANDLER				asm(".word __ArmVectorAbortData ")

#ifdef BTRACE_CPU_USAGE
extern void btrace_irq_entry(TInt);
extern void btrace_fiq_entry();
#endif

extern void handle_crash_ipi();
extern void handle_indirect_powerdown_ipi();

#ifdef _DEBUG
extern void __DebugMsgIrq(TUint aIrqNumber);
#endif

/* NOTE: We must ensure that this code goes at the beginning of the kernel image.
*/
__NAKED__ void __this_must_go_at_the_beginning_of_the_kernel_image()
	{
	asm("ldr	pc, __reset_vector ");		// 00 = Reset vector
	asm("ldr	pc, __undef_vector ");		// 04 = Undefined instruction vector
	asm("ldr	pc, __swi_vector ");		// 08 = SWI vector
	asm("ldr	pc, __pabt_vector ");		// 0C = Prefetch abort vector
	asm("ldr	pc, __dabt_vector ");		// 10 = Data abort vector
	asm("ldr	pc, __unused_vector ");		// 14 = unused
	asm("b		HandleIrq ");				// 18 = IRQ vector
											// 1C = FIQ vector, code in situ
	/*** FIQ entry point ******************************************************/
	asm("ldr	r12, __ArmInterrupt ");
#ifdef BTRACE_CPU_USAGE
	asm("ldr	r11, __BTraceCpuUsageFilter ");
#endif
	asm("sub	lr, lr, #4 ");
	asm("ldr	r10, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iFiqHandler)); // r10 points to FIQ handler
	asm("str	lr, [sp, #-4]! ");
	// we assume FIQ handler preserves r0-r7 but not r8-r12
	// hence must be assembler, so stack misalignment OK
#if defined(__CPU_ARM_HAS_WORKING_CLREX)
	CLREX
#elif defined(__CPU_ARM_HAS_LDREX_STREX)
	STREX(8,14,13);						// dummy STREX to reset exclusivity monitor
#endif
#ifdef __USER_MEMORY_GUARDS_ENABLED__
	USER_MEMORY_GUARD_ON(,lr,r8);
	asm("str lr, [sp, #-4]! ");
#endif
#ifdef BTRACE_CPU_USAGE
	asm("ldrb	r8, [r11] ");
	asm("ldr	lr, _ArmVectorFiq ");
	asm("cmp	r8, #0 ");
	asm("bne	btrace_fiq");
	__JUMP(,	r10);					// jump to FIQ handler

	asm("btrace_fiq: ");				// call trace handler before fiq handler...
	asm("stmfd	sp!, {r0-r3,r12,lr} ");
	asm("adr	lr, btrace_fiq_return ");
	asm("ldr	pc, __btrace_fiq_entry ");
	asm("btrace_fiq_return: ");
	asm("ldmfd	sp!, {r0-r3,r12,lr} ");
	__JUMP(,	r10);					// jump to FIQ handler
#endif
	asm("ldr lr, _ArmVectorFiq ");
	__JUMP(,r10);			// jump to FIQ handler


	/*** Nested IRQ register save *********************************************/
	asm("nested_irq: ");
	SRSDBW(MODE_SYS);					// save return address and return CPSR to interrupt stack
	CPSCHM(MODE_SYS);					// mode_sys, IRQs off
	asm("stmfd	sp!, {r0-r12,lr} ");	// save R0-R12,R14 from system mode
	GET_RWNO_TID(,r4);
	asm("b nested_irq_rejoin ");

	/*** IRQ entry point ******************************************************/
	asm("HandleIrq: ");
	asm("mrs	r13, spsr ");
	asm("sub	lr, lr, #4 ");
	asm("and	r13, r13, #0x1f ");
	asm("cmp	r13, #0x1f ");			// interrupted mode_sys?
	asm("beq	nested_irq ");			// yes -> nested interrupt
	SRSDBW(MODE_SVC);					// save return address and return CPSR to supervisor stack
	__ASM_CLI_MODE(MODE_SVC);			// mode_svc, IRQs and FIQs off
	asm("sub	sp, sp, #%a0" : : "i" _FOFF(SThreadExcStack,iR15));
	asm("stmia	sp, {r0-r14}^ ");		// save R0-R12, R13_usr, R14_usr
	asm("mov	r1, #%a0" : : "i" ((TInt)SThreadExcStack::EIrq));
#if defined(__CPU_ARM_HAS_WORKING_CLREX)
	CLREX
#elif defined(__CPU_ARM_HAS_LDREX_STREX)
	STREX(12, 0, 13);					// dummy STREX to reset exclusivity monitor
#endif
	GET_RWNO_TID(,r4);
	asm("mov	r5, sp ");
	asm("str	r1, [sp, #%a0]" : : "i" _FOFF(SThreadExcStack,iExcCode));	// word describing exception type
	__ASM_STI2_MODE(MODE_SYS);			// mode_sys, IRQs off, FIQs on
	asm("ldr	sp, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iIrqStackTop));
	USER_MEMORY_GUARD_ON(,r8,r0);		// r8 = original DACR if user memory guards in use

	asm("nested_irq_rejoin: ");
	asm("ldr	r0, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iIrqCount));
	asm("ldr	r7, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iIrqNestCount));
	asm("ldr	r12, __ArmInterrupt ");
	asm("ldr	r10, _ArmVectorIrq ");
	asm("add	r0, r0, #1 ");
	asm("add	r7, r7, #1 ");
	__DATA_MEMORY_BARRIER_Z__(r2);		// ensure memory accesses in interrupted code are observed before
										// the writes to i_IrqCount, i_IrqNestCount
	asm("str	r0, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iIrqCount));		// increment i_IrqCount
	asm("ldr	r11, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iIrqHandler));		// address if IRQ handler
	asm("ldr	r6, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iGicCpuIfcAddr));
	asm("str	r7, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iIrqNestCount));	// increment i_IrqNestCount

	asm("1: ");
#ifdef BTRACE_CPU_USAGE
	asm("ldr	r2, __BTraceCpuUsageFilter ");
#endif
	asm("mov	r1, #%a0" : : "i" ((TInt)E_GicIntId_Spurious+1));
	asm("ldr	r0, [r6, #%a0]" : : "i" _FOFF(GicCpuIfc, iAck));		// r0 = number of interrupt to service
#ifdef BTRACE_CPU_USAGE
	asm("ldrb	r2, [r2] ");
#endif
	asm("sub	r1, r1, #1 ");
	asm("cmp	r0, r1 ");				// any more interrupts pending?
	asm("beq	2f ");					// if not, branch out
#ifdef BTRACE_CPU_USAGE
	asm("cmp	r2, #0 ");
	asm("beq	9f ");
	asm("stmfd	sp!, {r0-r3} ");
	asm("adr	lr, btrace_irq_return ");
	asm("ldr	pc, __btrace_irq_entry ");
	asm("btrace_irq_return: ");
	asm("ldmfd	sp!, {r0-r3} ");
	asm("9: ");
#endif	// BTRACE_CPU_USAGE
	ASM_DEBUG1(_longjump_Irq,r0);
	asm("adr	lr, 1b ");
	asm("tst	r0, #0x3e0 ");			// check for interrupt numbers 0-31
	asm("beq	3f ");					// branch out if so
	__JUMP(,r11);						// jump to dispatcher, R0 = interrupt number, return to 1:
										// dispatcher acknowledges interrupt

	// No more interrupts pending - jump to postamble in the kernel
	// R4->TSubScheduler at this point, R5->saved registers on SVC stack if not nested IRQ
	// R6->GIC CPU interface
	asm("2: ");
	__JUMP(,r10);

	// Kernel IPI
	asm("3: ");
	asm("and	r2, r0, #31 ");			// r2 = interrupt number 0...31
	asm("cmp	r2, #%a0" : : "i" ((TInt)TIMESLICE_VECTOR));
	asm("beq	do_timeslice_irq ");
	asm("cmp	r2, #%a0" : : "i" ((TInt)RESCHED_IPI_VECTOR));
	asm("beq	do_resched_ipi ");
	asm("cmp	r2, #%a0" : : "i" ((TInt)GENERIC_IPI_VECTOR));
	asm("beq	do_generic_ipi ");
	asm("cmp	r2, #%a0" : : "i" ((TInt)TRANSFERRED_IRQ_VECTOR));
	asm("beq	do_transferred_ipi ");
	asm("cmp	r2, #%a0" : : "i" ((TInt)INDIRECT_POWERDOWN_IPI_VECTOR));
	asm("beq	do_indirect_powerdown_ipi ");
	asm("cmp	r2, #15 ");
	__JUMP(hi,	r11);					// if >15 but not TIMESLICE_VECTOR, call dispatcher

	// else assume CRASH_IPI
	asm("str	r0, [r6, #%a0]" : : "i" _FOFF(GicCpuIfc, iEoi));		// acknowledge interrupt
	__DATA_SYNC_BARRIER_Z__(r1);
	asm("ldr	r1, __HandleCrashIPI ");
	__JUMP(,	r1);					// CRASH IPI, so crash

	// TIMESLICE, RESCHED or TRANSFERRED
	asm("do_timeslice_irq: ");
	asm("ldr	r2, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iSSX.iLocalTimerAddr));
	asm("mov	r1, #1 ");
	asm("str	r1, [r2, #%a0]" : : "i" _FOFF(ArmLocalTimer, iTimerIntStatus));	// clear timer event flag
	asm("do_resched_ipi: ");
	asm("mov	r1, #1 ");
	asm("strb	r1, [r4, #%a0]" : : "i" _FOFF(TSubScheduler, iRescheduleNeededFlag));
	asm("do_transferred_ipi: ");
	asm("str	r0, [r6, #%a0]" : : "i" _FOFF(GicCpuIfc, iEoi));		// acknowledge interrupt
	__DATA_SYNC_BARRIER_Z__(r1);		// ensure writes to i_IrqCount, i_IrqNestCount, iRescheduleNeededFlag complete before SEV
										// also ensure EOI is written before we return from the interrupt
	ARM_SEV;							// kick any CPUs waiting for us to enter the ISR
	asm("b		1b ");

	asm("do_indirect_powerdown_ipi: ");
	asm("str	r0, [r6, #%a0]" : : "i" _FOFF(GicCpuIfc, iEoi));		// acknowledge interrupt
	__DATA_SYNC_BARRIER_Z__(r1);		// ensure writes to i_IrqCount, i_IrqNestCount, iRescheduleNeededFlag complete before SEV
										// also ensure EOI is written before we return from the interrupt
	ARM_SEV;							// kick any CPUs waiting for us to enter the ISR
	asm("stmfd	sp!, {r0-r3,r12,lr} ");
	asm("bl		call_ipd_handler ");
	asm("ldmfd	sp!, {r0-r3,r12,lr} ");
	asm("b		1b ");

	// GENERIC_IPI
	asm("do_generic_ipi: ");
	asm("ldr	r2, _GenericIPIIsr ");
	asm("str	r0, [r6, #%a0]" : : "i" _FOFF(GicCpuIfc, iEoi));		// acknowledge interrupt
	asm("mov	r0, r4 ");				// r0->SubScheduler
	__DATA_SYNC_BARRIER_Z__(r1);
	__JUMP(,	r2);

	asm("__DebugMsg_longjump_Irq: ");
	asm("ldr	pc, _dmIrq ");

	asm("call_ipd_handler: ");
	asm("ldr	pc, __handle_ipd_ipi ");

	asm("__reset_vector:");
	asm(".word	__ArmVectorReset "); 
	asm("__undef_vector:");
	__DECLARE_UNDEFINED_INSTRUCTION_HANDLER;
	asm("__swi_vector:");
	asm(".word	__ArmVectorSwi "); 
	asm("__pabt_vector:");
	__DECLARE_PREFETCH_ABORT_HANDLER;
	asm("__dabt_vector:");
	__DECLARE_DATA_ABORT_HANDLER;
	asm("__unused_vector:");
	asm(".word	__ArmVectorReserved ");

	asm("__ArmInterrupt: ");
	asm(".word	ArmInterruptInfo ");
	asm("_ArmVectorIrq: ");
	asm(".word	__ArmVectorIrq ");
	asm("_GenericIPIIsr: ");
	asm(".word generic_ipi_isr ");
	asm("_ArmVectorFiq: ");
	asm(".word	__ArmVectorFiq ");
	asm("__HandleCrashIPI: ");
	asm(".word handle_crash_ipi ");
#ifdef BTRACE_CPU_USAGE
	asm("__BTraceCpuUsageFilter: ");
	asm(".word	%a0" : : "i" ((TInt)&BTraceData.iFilter[BTrace::ECpuUsage]));
	asm("__btrace_irq_entry: ");
	asm(".word btrace_irq_entry ");
	asm("__btrace_fiq_entry: ");
	asm(".word btrace_fiq_entry ");
#endif
	asm("_dmIrq: ");
	asm(".word __DebugMsgIrq ");
	asm("__handle_ipd_ipi: ");
	asm(".word handle_indirect_powerdown_ipi ");
	}
}

