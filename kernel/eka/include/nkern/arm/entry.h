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
// e32/include/nkern/arm/entry.h
// 
//

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
	asm("ldr r12, __ArmInterrupt ");	// THIS MUST BE AN IDEMPOTENT INSTRUCTION TO AVOID A PROBLEM WITH XSCALE PXA255
	asm("sub lr, lr, #4 ");
	asm("str lr, [sp, #-4]! ");
	// we assume FIQ handler preserves r0-r7 but not r8-r12
	// hence must be assembler, so stack misalignment OK
#if defined(__CPU_ARM_HAS_WORKING_CLREX)
	CLREX
#elif defined(__CPU_ARM_HAS_LDREX_STREX)
	STREX(8,14, 13);		// dummy STREX to reset exclusivity monitor
#endif
#ifdef __USER_MEMORY_GUARDS_ENABLED__
	USER_MEMORY_GUARD_ON(,lr,r8);
	asm("str lr, [sp, #-4]! ");
#endif
#ifdef BTRACE_CPU_USAGE
	asm("ldrb r8, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iCpuUsageFilter));
	asm("ldr lr, _ArmVectorFiq ");
	asm("mov r10, #%a0" : : "i" ((TInt)(BTrace::ECpuUsage<<BTrace::ECategoryIndex*8)+(BTrace::EFiqStart<<BTrace::ESubCategoryIndex*8)) );
	asm("cmp r8,#0");
	asm("bne btrace_fiq");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iFiqHandler)); // call FIQ handler, return to ArmVectorFiq

	asm("btrace_fiq:");		// call trace handler before fiq handler...
	asm("stmdb sp!, {r0-r3} ");
	asm("add r0, r10, #%a0" : : "i" ((TInt)4) ); // add size of trace into header
	asm("mov lr, pc");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iBTraceHandler));
	asm("ldr r12, __ArmInterrupt ");
	asm("ldmia sp!, {r0-r3} ");
#endif	
	asm("ldr lr, _ArmVectorFiq ");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iFiqHandler)); // call FIQ handler, return to ArmVectorFiq

	asm("HandleIrq: ");
	asm("sub lr, lr, #4 ");
	asm("stmfd sp!, {r0-r3,r12,lr} ");
#if defined(__CPU_ARM_HAS_WORKING_CLREX)
	CLREX
#elif defined(__CPU_ARM_HAS_LDREX_STREX)
	STREX(12, 0, 13);	// dummy STREX to reset exclusivity monitor
#endif
#ifdef __USER_MEMORY_GUARDS_ENABLED__
	USER_MEMORY_GUARD_ON(,lr,r12);
	asm("str lr, [sp, #-8]! ");
#endif
	asm("ldr r12, __ArmInterrupt ");
#ifdef BTRACE_CPU_USAGE
	asm("mov r0, #%a0" : : "i" ((TInt)(BTrace::ECpuUsage<<BTrace::ECategoryIndex*8)+(BTrace::EIrqStart<<BTrace::ESubCategoryIndex*8)) );
	asm("add r0, r0, #%a0" : : "i" ((TInt)4) ); // add size of trace into header
	asm("ldrb r1, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iCpuUsageFilter));
	asm("ldr lr, _ArmVectorIrq ");
	asm("cmp r1,#0");
	asm("bne btrace_irq");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iIrqHandler));  // call IRQ handler, return to ArmVectorIrq

	asm("btrace_irq:");	// call trace handler before irq handler...
	asm("mov lr, pc");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iBTraceHandler));
	asm("ldr r12, __ArmInterrupt ");
#endif
	asm("ldr lr, _ArmVectorIrq ");
	asm("ldr pc, [r12,#%a0]" : : "i" _FOFF(SArmInterruptInfo,iIrqHandler)); // call IRQ handler, return to ArmVectorIrq

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
	asm(".word  ArmInterruptInfo ");
	asm("_ArmVectorIrq:");
	asm(".word __ArmVectorIrq"); 
	asm("_ArmVectorFiq:");
	asm(".word __ArmVectorFiq ");
	}
}

