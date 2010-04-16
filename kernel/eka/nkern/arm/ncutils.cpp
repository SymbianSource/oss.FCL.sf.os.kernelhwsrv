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
// e32\nkern\arm\ncutils.cpp
// 
//

#include <arm.h>
#include "../../include/kernel/kernboot.h"

extern "C" {
SFullArmRegSet ArmRegs;
}

#ifdef _DEBUG
void FastMutexNestAttempt()
	{
	FAULT();
	}

void FastMutexSignalError()
	{
	FAULT();
	}
#endif

void NKern::Init0(TAny*)
	{
	ArmRegs.iExcCode = -1;
	TheScheduler.i_Regs = &ArmRegs;
	}

GLDEF_C TUint32 IrqReturnAddress()
	{
	TStackInfo& stackInfo =  ((SSuperPageBase*)::SuperPageAddress)->iStackInfo;
	return ((TUint32)stackInfo.iIrqStackBase) + stackInfo.iIrqStackSize  - sizeof(TUint32);
	}

/** Register the global IRQ handler
	Called by the base port at boot time to bind the top level IRQ dispatcher
	to the ARM IRQ vector. Should not be called at any other time.

	The handler specified will be called in mode_irq with IRQs disabled and
	FIQs enabled. R0-R3, R12 and the return address from the interrupt will
	be on the top of the mode_irq stack. R14_irq will point to the kernel's
	IRQ postamble routine, which will run IDFCs and reschedule if necessary.
	R13_irq will point to the top of the mode_irq stack and will be 8-byte aligned.
	The handler should preserve all registers other than R0-R3, R12, R14_irq
	and should return to the address in R14_irq.

	@param	aHandler The address of the top level IRQ dispatcher routine
 */
EXPORT_C void Arm::SetIrqHandler(TLinAddr aHandler)
	{
	ArmInterruptInfo.iIrqHandler=aHandler;
	}

/** Register the global FIQ handler
	Called by the base port at boot time to bind the top level FIQ dispatcher
	to the ARM FIQ vector. Should not be called at any other time.

	The handler specified will be called in mode_fiq with both IRQs and FIQs
	disabled. The return address from the interrupt will be on the top of the
	mode_fiq stack. R14_fiq will point to the kernel's FIQ postamble routine,
	which will run IDFCs and reschedule if necessary.
	R13_fiq will point to the top of the mode_fiq stack and will be 4 modulo 8.
	The handler should preserve all registers other than R8_fiq-R12_fiq and
	R14_fiq	and should return to the address in R14_fiq.

	@param	aHandler The address of the top level FIQ dispatcher routine
 */
EXPORT_C void Arm::SetFiqHandler(TLinAddr aHandler)
	{
	ArmInterruptInfo.iFiqHandler=aHandler;
	}

/** Register the global Idle handler
	Called by the base port at boot time to register a handler containing a pointer to
	a function that is called by the Kernel when each core reaches idle.
	Should not be called at any other time.

	@param	aHandler Pointer to idle handler function
	@param	aPtr Idle handler function argument
*/
EXPORT_C void Arm::SetIdleHandler(TCpuIdleHandlerFn aHandler, TAny* aPtr)
	{
	ArmInterruptInfo.iCpuIdleHandler.iHandler = aHandler;
	ArmInterruptInfo.iCpuIdleHandler.iPtr = aPtr;
	ArmInterruptInfo.iCpuIdleHandler.iPostambleRequired = EFalse;
	}

extern void initialiseState();
void Arm::Init1Interrupts()
//
// Initialise the interrupt and exception vector handlers.
//
	{
//	TheIrqHandler=0;	// done by placing TheIrqHandler, TheFiqHandler in .bss
//	TheFiqHandler=0;
	
	initialiseState();
	}

extern "C" void __ArmVectorReset()
//
// Reset
//
	{

	FAULT();
	}

extern "C" void __ArmVectorReserved()
//
// Reserved
//
	{

	FAULT();
	}


TInt BTraceDefaultControl(BTrace::TControl /*aFunction*/, TAny* /*aArg1*/, TAny* /*aArg2*/)
	{
	return KErrNotSupported;
	}


EXPORT_C void BTrace::SetHandlers(BTrace::THandler aNewHandler, BTrace::TControlFunction aNewControl, BTrace::THandler& aOldHandler, BTrace::TControlFunction& aOldControl)
	{
	TUint irq = NKern::DisableAllInterrupts();
	
	aOldHandler = BTraceData.iHandler;
	BTraceData.iHandler = aNewHandler;
	ArmInterruptInfo.iBTraceHandler = aNewHandler;
	TheScheduler.iBTraceHandler = aNewHandler;
	
	aOldControl = BTraceData.iControl;
	BTraceData.iControl = aNewControl ? aNewControl : BTraceDefaultControl;
	
	NKern::RestoreInterrupts(irq);
	}


EXPORT_C TInt BTrace::SetFilter(TUint aCategory, TInt aValue)
	{
	if(!IsSupported(aCategory))
		return KErrNotSupported;
	TUint irq = NKern::DisableAllInterrupts();
	TUint8* filter = BTraceData.iFilter+aCategory;
	TUint oldValue = *filter;
	if(TUint(aValue)<=1u)
		{
		*filter = (TUint8)aValue;
		BTraceContext4(BTrace::EMetaTrace, BTrace::EMetaTraceFilterChange, (TUint8)aCategory | (aValue<<8));
		if(aCategory==ECpuUsage)
			{
			ArmInterruptInfo.iCpuUsageFilter = aValue;
			TheScheduler.iCpuUsageFilter = aValue;
			}
		if (aCategory == EFastMutex)
			{
			// This is done because of the optimization in ncsched.cia for
			// ARMv5 (check if lock is free (cmp) && filter is enabled (cmpeq))
			TheScheduler.iFastMutexFilter = aValue ? 1 : 0;
			}
		}
	NKern::RestoreInterrupts(irq);
	return oldValue;
	}

EXPORT_C SCpuIdleHandler* NKern::CpuIdleHandler()
	{
	return &ArmInterruptInfo.iCpuIdleHandler;
	}

EXPORT_C TUint32 NKern::CpuTimeMeasFreq()
	{
#ifdef MONITOR_THREAD_CPU_TIME
	return NKern::FastCounterFrequency();
#else
	return 0;
#endif
	}
