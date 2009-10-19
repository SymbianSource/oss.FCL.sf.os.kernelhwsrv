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
// e32\nkernsmp\arm\ncutils.cpp
// 
//

#include <arm.h>
#include <arm_gic.h>
#include <arm_scu.h>
#include <arm_tmr.h>
#include <nk_irq.h>

extern "C" {
extern SVariantInterfaceBlock* VIB;
}

/******************************************************************************
 * Spin lock
 ******************************************************************************/
/** Create a spin lock

	@publishedPartner
	@released
*/
EXPORT_C TSpinLock::TSpinLock(TUint aOrder)
	{
	(void)aOrder;
	__NK_ASSERT_DEBUG( (aOrder==EOrderNone) || ((aOrder&0x7f)<0x20) );
	if (aOrder>=0x80 && aOrder!=EOrderNone)
		aOrder -= 0x60;
	aOrder |= 0xFF00u;
	iLock = TUint64(aOrder)<<48;	// byte 6 = 00-1F for interrupt, 20-3F for preemption
									// byte 7 = FF if not held
	}


/******************************************************************************
 * Read/Write Spin lock
 ******************************************************************************/
/** Create a spin lock

	@publishedPartner
	@released
*/
EXPORT_C TRWSpinLock::TRWSpinLock(TUint aOrder)
	{
	(void)aOrder;
	__NK_ASSERT_DEBUG( (aOrder==TSpinLock::EOrderNone) || ((aOrder&0x7f)<0x20) );
	if (aOrder>=0x80 && aOrder!=TSpinLock::EOrderNone)
		aOrder -= 0x60;
	aOrder |= 0xFF00u;
	iLock = TUint64(aOrder)<<48;	// byte 6 = 00-1F for interrupt, 20-3F for preemption
									// byte 7 = FF if not held
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

void NKern::Init0(TAny* a)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("VIB=%08x", a));
	VIB = (SVariantInterfaceBlock*)a;
	__NK_ASSERT_ALWAYS(VIB && VIB->iVer==0 && VIB->iSize==sizeof(SVariantInterfaceBlock));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iVer=%d iSize=%d", VIB->iVer, VIB->iSize));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxCpuClock=%08x %08x", I64HIGH(VIB->iMaxCpuClock), I64LOW(VIB->iMaxCpuClock)));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxTimerClock=%u", VIB->iMaxTimerClock));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iScuAddr=%08x", VIB->iScuAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iGicDistAddr=%08x", VIB->iGicDistAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iGicCpuIfcAddr=%08x", VIB->iGicCpuIfcAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iLocalTimerAddr=%08x", VIB->iLocalTimerAddr));

	TScheduler& s = TheScheduler;
	s.i_ScuAddr = (TAny*)VIB->iScuAddr;
	s.i_GicDistAddr = (TAny*)VIB->iGicDistAddr;
	s.i_GicCpuIfcAddr = (TAny*)VIB->iGicCpuIfcAddr;
	s.i_LocalTimerAddr = (TAny*)VIB->iLocalTimerAddr;
	s.i_TimerMax = (TAny*)(VIB->iMaxTimerClock / 1);		// use prescaler value of 1

	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = TheSubSchedulers[i];
		ss.i_TimerMultF = (TAny*)KMaxTUint32;
		ss.i_TimerMultI = (TAny*)0x01000000u;
		ss.i_CpuMult = (TAny*)KMaxTUint32;
		ss.i_LastTimerSet = (TAny*)KMaxTInt32;
		ss.i_TimestampError = (TAny*)0;
		ss.i_TimerGap = (TAny*)16;
		ss.i_MaxCorrection = (TAny*)64;
		VIB->iTimerMult[i] = (volatile STimerMult*)&ss.i_TimerMultF;
		VIB->iCpuMult[i] = (volatile TUint32*)&ss.i_CpuMult;
		}
	InterruptInit0();
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

extern void initialiseState(TInt aCpu, TSubScheduler* aSS);

void Arm::Init1Interrupts()
//
// Initialise the interrupt and exception vector handlers.
//
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT(">Arm::Init1Interrupts()"));

	TSubScheduler* ss = &TheSubSchedulers[0];
	initialiseState(0, ss);

	ArmLocalTimer& T = LOCAL_TIMER;
	T.iWatchdogDisable = E_ArmTmrWDD_1;
	T.iWatchdogDisable = E_ArmTmrWDD_2;
	T.iTimerCtrl = 0;
	T.iTimerIntStatus = E_ArmTmrIntStatus_Event;
	T.iWatchdogCtrl = 0;
	T.iWatchdogIntStatus = E_ArmTmrIntStatus_Event;

	NIrq::HwInit1();

	__KTRACE_OPT(KBOOT,DEBUGPRINT("<Arm::Init1Interrupts()"));
	}

extern "C" void __ArmVectorReset()
	{
	FAULT();
	}

extern "C" void __ArmVectorReserved()
	{
	FAULT();
	}


TInt BTraceDefaultControl(BTrace::TControl /*aFunction*/, TAny* /*aArg1*/, TAny* /*aArg2*/)
	{
	return KErrNotSupported;
	}


EXPORT_C void BTrace::SetHandlers(BTrace::THandler aNewHandler, BTrace::TControlFunction aNewControl, BTrace::THandler& aOldHandler, BTrace::TControlFunction& aOldControl)
	{
	BTrace::TControlFunction nc = aNewControl ? aNewControl : &BTraceDefaultControl;
	__ACQUIRE_BTRACE_LOCK();
	BTrace::THandler oldh = (BTrace::THandler)__e32_atomic_swp_ord_ptr(&BTraceData.iHandler, aNewHandler);
	BTrace::TControlFunction oldc = (BTrace::TControlFunction)__e32_atomic_swp_ord_ptr(&BTraceData.iControl, nc);
	__RELEASE_BTRACE_LOCK();
	aOldHandler = oldh;
	aOldControl = oldc;
	}


EXPORT_C TInt BTrace::SetFilter(TUint aCategory, TInt aValue)
	{
	if(!IsSupported(aCategory))
		return KErrNotSupported;
	TUint8* filter = BTraceData.iFilter+aCategory;
	TUint oldValue = *filter;
	if(TUint(aValue)<=1u)
		{
		oldValue = __e32_atomic_swp_ord8(filter, (TUint8)aValue);
		BTraceContext4(BTrace::EMetaTrace, BTrace::EMetaTraceFilterChange, (TUint8)aCategory | (aValue<<8));
		}
	return oldValue;
	}

EXPORT_C SCpuIdleHandler* NKern::CpuIdleHandler()
	{
	return &ArmInterruptInfo.iCpuIdleHandler;
	}

TUint32 NKern::IdleGenerationCount()
	{
	return TheScheduler.iIdleGenerationCount;
	}

void NKern::Idle()
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss = SubScheduler();	// OK since idle thread locked to CPU
	TUint32 m = ss.iCpuMask;
	s.iIdleSpinLock.LockIrq();
	TUint32 orig_cpus_not_idle = __e32_atomic_and_acq32(&s.iCpusNotIdle, ~m);
	if (orig_cpus_not_idle == m)
		{
		// all CPUs idle
		if (!s.iIdleDfcs.IsEmpty())
			{
			__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);	// we aren't idle after all
			s.iIdleGeneration ^= 1;
			++s.iIdleGenerationCount;
			s.iIdleSpillCpu = (TUint8)ss.iCpuNum;
			ss.iDfcs.MoveFrom(&s.iIdleDfcs);
			ss.iDfcPendingFlag = 1;
			s.iIdleSpinLock.UnlockIrq();
			NKern::Lock();
			NKern::Unlock();	// process idle DFCs here
			return;
			}
		}

	// postamble happens here - interrupts cannot be reenabled
	s.iIdleSpinLock.UnlockOnly();
	NKIdle(orig_cpus_not_idle & ~m);

	// interrupts have not been reenabled
	s.iIdleSpinLock.LockOnly();
	__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);
	if (ArmInterruptInfo.iCpuIdleHandler.iPostambleRequired)
		{
		ArmInterruptInfo.iCpuIdleHandler.iPostambleRequired = FALSE;
		NKIdle(-1);
		}
	s.iIdleSpinLock.UnlockIrq();	// reenables interrupts
	}


EXPORT_C TUint32 NKern::CpuTimeMeasFreq()
	{
	return NKern::TimestampFrequency();
	}


/**	Converts a time interval in microseconds to thread timeslice ticks

	@param aMicroseconds time interval in microseconds.
	@return Number of thread timeslice ticks.  Non-integral results are rounded up.

 	@pre aMicroseconds should be nonnegative
	@pre any context
 */
EXPORT_C TInt NKern::TimesliceTicks(TUint32 aMicroseconds)
	{
	TUint32 mf32 = (TUint32)TheScheduler.i_TimerMax;
	TUint64 mf(mf32);
	TUint64 ticks = mf*TUint64(aMicroseconds) + UI64LIT(999999);
	ticks /= UI64LIT(1000000);
	if (ticks > TUint64(TInt(KMaxTInt)))
		return KMaxTInt;
	else
		return (TInt)ticks;
	}


/** Get the frequency of counter queried by NKern::Timestamp().

@publishedPartner
@prototype
*/
EXPORT_C TUint32 NKern::TimestampFrequency()
	{
	return (TUint32)TheScheduler.i_TimerMax;
	}

