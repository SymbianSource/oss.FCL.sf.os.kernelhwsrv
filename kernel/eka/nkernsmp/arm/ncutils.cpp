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
extern TUint KernCoreStats_EnterIdle(TUint aCore);
extern void KernCoreStats_LeaveIdle(TInt aCookie,TUint aCore);

extern void DetachComplete();
extern void send_irq_ipi(TSubScheduler*, TInt);
}

TInt ClockFrequenciesChanged();


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
	SVariantInterfaceBlock* v = (SVariantInterfaceBlock*)a;
	TheScheduler.iVIB = v;
	__NK_ASSERT_ALWAYS(v && v->iVer==0 && v->iSize==sizeof(SVariantInterfaceBlock));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iVer=%d iSize=%d", v->iVer, v->iSize));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxCpuClock=%08x %08x", I64HIGH(v->iMaxCpuClock), I64LOW(v->iMaxCpuClock)));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxTimerClock=%u", v->iMaxTimerClock));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iScuAddr=%08x", v->iScuAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iGicDistAddr=%08x", v->iGicDistAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iGicCpuIfcAddr=%08x", v->iGicCpuIfcAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iLocalTimerAddr=%08x", v->iLocalTimerAddr));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iGlobalTimerAddr=%08x", v->iGlobalTimerAddr));

	TScheduler& s = TheScheduler;
	s.iSX.iScuAddr = (ArmScu*)v->iScuAddr;
	s.iSX.iGicDistAddr = (GicDistributor*)v->iGicDistAddr;
	s.iSX.iGicCpuIfcAddr = (GicCpuIfc*)v->iGicCpuIfcAddr;
	s.iSX.iLocalTimerAddr = (ArmLocalTimer*)v->iLocalTimerAddr;
	s.iSX.iTimerMax = (v->iMaxTimerClock / 1);		// use prescaler value of 1
#ifdef	__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK
	s.iSX.iGlobalTimerAddr = (ArmGlobalTimer*)v->iGlobalTimerAddr;
	s.iSX.iGTimerFreqRI.Set(v->iGTimerFreqR);
	v->iGTimerFreqR = 0;
#endif

	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = TheSubSchedulers[i];
		ss.iSSX.iCpuFreqRI.Set(v->iCpuFreqR[i]);
		ss.iSSX.iTimerFreqRI.Set(v->iTimerFreqR[i]);

		v->iCpuFreqR[i] = 0;
		v->iTimerFreqR[i] = 0;
		UPerCpuUncached* u = v->iUncached[i];
		ss.iUncached = u;
		u->iU.iDetachCount = 0;
		u->iU.iAttachCount = 0;
		u->iU.iPowerOffReq = FALSE;
		u->iU.iDetachCompleteFn = &DetachComplete;
		}
	v->iFrqChgFn = &ClockFrequenciesChanged;
	__e32_io_completion_barrier();
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

void NKern::DoIdle()
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss = SubScheduler();	// OK since idle thread locked to CPU
	SPerCpuUncached* u0 = &((UPerCpuUncached*)ss.iUncached)->iU;
	TUint32 m = ss.iCpuMask;
	TUint32 retire = 0;
	TBool global_defer = FALSE;
	TBool event_kick = FALSE;
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
	TBool shutdown_check = !((s.iThreadAcceptCpus|s.iCCReactivateCpus) & m);
	if (shutdown_check)
		{
		// check whether this CPU is ready to be powered off
		s.iGenIPILock.LockOnly();
		ss.iEventHandlerLock.LockOnly();
		if ( !((s.iThreadAcceptCpus|s.iCCReactivateCpus) & m) && !ss.iDeferShutdown && !ss.iNextIPI && !ss.iEventHandlersPending)
			{
			for(;;)
				{
				if (s.iCCDeferCount)
					{
					global_defer = TRUE;
					break;
					}
				if (s.iPoweringOff)
					{
					// another CPU might be in the process of powering off
					SPerCpuUncached* u = &((UPerCpuUncached*)s.iPoweringOff->iUncached)->iU;
					if (u->iDetachCount == s.iDetachCount)
						{
						// still powering off so we must wait
						global_defer = TRUE;
						break;
						}
					}
				TUint32 more = s.CpuShuttingDown(ss);
				retire = SCpuIdleHandler::ERetire;
				if (more)
					retire |= SCpuIdleHandler::EMore;
				s.iPoweringOff = &ss;
				s.iDetachCount = u0->iDetachCount;
				break;
				}
			}
		ss.iEventHandlerLock.UnlockOnly();
		s.iGenIPILock.UnlockOnly();
		}
	if (!retire && ss.iCurrentThread->iSavedSP)
		{
		// rescheduled between entry to NKern::Idle() and here
		// go round again to see if any more threads to pull from other CPUs
		__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);	// we aren't idle after all
		s.iIdleSpinLock.UnlockIrq();
		return;
		}
	if (global_defer)
		{
		// Don't WFI if we're only waiting for iCCDeferCount to reach zero or for
		// another CPU to finish powering down since we might not get another IPI.
		__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);	// we aren't idle after all
		s.iIdleSpinLock.UnlockIrq();
		__snooze();
		return;
		}

	// postamble happens here - interrupts cannot be reenabled
	TUint32 arg = orig_cpus_not_idle & ~m;
	if (arg == 0)
		s.AllCpusIdle();
	s.iIdleSpinLock.UnlockOnly();

	TUint cookie = KernCoreStats_EnterIdle((TUint8)ss.iCpuNum);

	arg |= retire;
	NKIdle(arg);

	// interrupts have not been reenabled
	s.iIdleSpinLock.LockOnly();

	if (retire)
		{
		// we just came back from power down
		SPerCpuUncached* u = &((UPerCpuUncached*)ss.iUncached)->iU;
		u->iPowerOnReq = 0;
		__e32_io_completion_barrier();
		s.iGenIPILock.LockOnly();
		ss.iEventHandlerLock.LockOnly();
		s.iIpiAcceptCpus |= m;
		s.iCCReactivateCpus |= m;
		s.iCpusGoingDown &= ~m;
		if (s.iPoweringOff == &ss)
			s.iPoweringOff = 0;
		if (ss.iEventHandlersPending)
			event_kick = TRUE;
		ss.iEventHandlerLock.UnlockOnly();
		s.iGenIPILock.UnlockOnly();
		}

	TUint32 ci = __e32_atomic_ior_ord32(&s.iCpusNotIdle, m);
	if (ArmInterruptInfo.iCpuIdleHandler.iPostambleRequired)
		{
		ArmInterruptInfo.iCpuIdleHandler.iPostambleRequired = FALSE;
		NKIdle(ci|m|SCpuIdleHandler::EPostamble);
		}
	if (ci == 0)
		s.FirstBackFromIdle();

	KernCoreStats_LeaveIdle(cookie, (TUint8)ss.iCpuNum);

	if (retire)
		{
		s.iCCReactivateDfc.RawAdd();	// kick load balancer to give us some work
		if (event_kick)
			send_irq_ipi(&ss, EQueueEvent_Kick);	// so that we will process pending events
		}
	s.iIdleSpinLock.UnlockIrq();	// reenables interrupts
	}

TBool TSubScheduler::Detached()
	{
	SPerCpuUncached* u = &((UPerCpuUncached*)iUncached)->iU;
	return u->iDetachCount != u->iAttachCount;
	}

TBool TScheduler::CoreControlSupported()
	{
	return TheScheduler.iVIB->iCpuPowerUpFn != 0;
	}

void TScheduler::CCInitiatePowerUp(TUint32 aCores)
	{
	TCpuPowerUpFn pUp = TheScheduler.iVIB->iCpuPowerUpFn;
	if (pUp && aCores)
		{
		TInt i;
		for (i=0; i<KMaxCpus; ++i)
			{
			if (aCores & (1u<<i))
				{
				TSubScheduler& ss = TheSubSchedulers[i];
				SPerCpuUncached& u = ((UPerCpuUncached*)ss.iUncached)->iU;
				u.iPowerOnReq = TRUE;
				__e32_io_completion_barrier();
				pUp(i, &u);

				// wait for core to reattach
				while (u.iDetachCount != u.iAttachCount)
					{
					__snooze();
					}
				}
			}
		}
	}

void TScheduler::CCIndirectPowerDown(TAny*)
	{
	TCpuPowerDownFn pDown = TheScheduler.iVIB->iCpuPowerDownFn;
	if (pDown)
		{
		TInt i;
		for (i=0; i<KMaxCpus; ++i)
			{
			TSubScheduler& ss = TheSubSchedulers[i];
			SPerCpuUncached& u = ((UPerCpuUncached*)ss.iUncached)->iU;
			if (u.iPowerOffReq)
				{
				pDown(i, &u);
				__e32_io_completion_barrier();
				u.iPowerOffReq = FALSE;
				__e32_io_completion_barrier();
				}
			}
		}
	}

// Called on any CPU which receives an indirect power down IPI
extern "C" void handle_indirect_powerdown_ipi()
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss = SubScheduler();
	if (s.iIpiAcceptCpus & ss.iCpuMask)
		s.iCCPowerDownDfc.Add();
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
	TUint32 mf32 = TheScheduler.iSX.iTimerMax;
	TUint64 mf(mf32);
	TUint64 ticks = mf*TUint64(aMicroseconds) + UI64LIT(999999);
	ticks /= UI64LIT(1000000);
	if (ticks > TUint64(TInt(KMaxTInt)))
		return KMaxTInt;
	else
		return (TInt)ticks;
	}


#if defined(__NKERN_TIMESTAMP_USE_LOCAL_TIMER__)
	// Assembler
#elif defined(__NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	// Assembler
#elif defined(__NKERN_TIMESTAMP_USE_INLINE_BSP_CODE__)
#define __DEFINE_NKERN_TIMESTAMP_CPP__
#include <variant_timestamp.h>
#undef __DEFINE_NKERN_TIMESTAMP_CPP__
#elif defined(__NKERN_TIMESTAMP_USE_BSP_CALLOUT__)
	// Assembler
#else
#error No definition for NKern::Timestamp()
#endif

/** Get the frequency of counter queried by NKern::Timestamp().

@publishedPartner
@prototype
*/
EXPORT_C TUint32 NKern::TimestampFrequency()
	{
#if defined(__NKERN_TIMESTAMP_USE_LOCAL_TIMER__)
	// Use per-CPU local timer in Cortex A9 or ARM11MP
	return TheScheduler.iSX.iTimerMax;
#elif defined(__NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	// Use global timer in Cortex A9 r1p0
	return TheScheduler.iSX.iTimerMax;
#elif defined(__NKERN_TIMESTAMP_USE_INLINE_BSP_CODE__)
	// Use code in <variant_timestamp.h> supplied by BSP
	return KTimestampFrequency;
#elif defined(__NKERN_TIMESTAMP_USE_BSP_CALLOUT__)
	// Call function defined in variant
#else
#error No definition for NKern::TimestampFrequency()
#endif
	}

/******************************************************************************
 * Notify frequency changes
 ******************************************************************************/

struct SFrequencies
	{
	void Populate();
	void Apply();
	TBool AddToQueue();

	SFrequencies*	iNext;
	TUint32			iWhich;
	SRatioInv		iNewCpuRI[KMaxCpus];
	SRatioInv		iNewTimerRI[KMaxCpus];
	SRatioInv		iNewGTimerRI;
	NFastSemaphore*	iSem;

	static SFrequencies* volatile Head;
	};

SFrequencies* volatile SFrequencies::Head;

TBool SFrequencies::AddToQueue()
	{
	SFrequencies* h = Head;
	do	{
		iNext = h;
		} while(!__e32_atomic_cas_rel_ptr(&Head, &h, this));
	return !h;	// TRUE if list was empty
	}


void SFrequencies::Populate()
	{
	TScheduler& s = TheScheduler;
	TInt cpu;
	iWhich = 0;
	SRatio* ri = (SRatio*)__e32_atomic_swp_ord_ptr(&s.iVIB->iGTimerFreqR, 0);
	if (ri)
		{
		iNewGTimerRI.Set(ri);
		iWhich |= 0x80000000u;
		}
	for (cpu=0; cpu<s.iNumCpus; ++cpu)
		{
		TSubScheduler& ss = *s.iSub[cpu];
		ri = (SRatio*)__e32_atomic_swp_ord_ptr(&s.iVIB->iCpuFreqR[cpu], 0);
		if (ri)
			{
			iNewCpuRI[cpu].Set(ri);
			iWhich |= ss.iCpuMask;
			}
		ri = (SRatio*)__e32_atomic_swp_ord_ptr(&s.iVIB->iTimerFreqR[cpu], 0);
		if (ri)
			{
			iNewTimerRI[cpu].Set(ri);
			iWhich |= (ss.iCpuMask<<8);
			}
		}
	}

#if defined(__NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
extern void ArmGlobalTimerFreqChg(const SRatioInv* /*aNewGTimerFreqRI*/);
#endif

void SFrequencies::Apply()
	{
	if (!iWhich)
		return;
	TScheduler& s = TheScheduler;
	TStopIPI ipi;
	TUint32 stopped = ipi.StopCPUs();
	TInt cpu;
	TUint32 wait = 0;
	for (cpu=0; cpu<s.iNumCpus; ++cpu)
		{
		TSubScheduler& ss = *s.iSub[cpu];
		TUint32 m = 1u<<cpu;
		TUint32 m2 = m | (m<<8);
		if (stopped & m)
			{
			// CPU is running so let it update
			if (iWhich & m2)
				{
				if (iWhich & m)
					ss.iSSX.iNewCpuFreqRI = &iNewCpuRI[cpu];
				if (iWhich & (m<<8))
					ss.iSSX.iNewTimerFreqRI = &iNewTimerRI[cpu];
				ss.iRescheduleNeededFlag = 1;
				wait |= m;
				}
			}
		else
			{
			// CPU is not running so update directly
			if (iWhich & m)
				{
				ss.iSSX.iCpuFreqRI = iNewCpuRI[cpu];
				}
			if (iWhich & (m<<8))
				{
				ss.iSSX.iTimerFreqRI = iNewTimerRI[cpu];
				}
			}
		}
#if defined(__NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	if (iWhich & 0x80000000u)
		{
		ArmGlobalTimerFreqChg(&iNewGTimerRI);
		}
#endif
	ipi.ReleaseCPUs();	// this CPU handled here
	while(wait)
		{
		cpu = __e32_find_ls1_32(wait);
		TSubScheduler& ss = *s.iSub[cpu];
		if (!ss.iSSX.iNewCpuFreqRI && !ss.iSSX.iNewTimerFreqRI)
			wait &= ~ss.iCpuMask;
		__chill();
		}
	}

void TScheduler::DoFrequencyChanged(TAny*)
	{
	SFrequencies* list = (SFrequencies*)__e32_atomic_swp_ord_ptr(&SFrequencies::Head, 0);
	if (!list)
		return;
	list->Populate();
	list->Apply();
	SFrequencies* rev = 0;
	while (list)
		{
		SFrequencies* next = list->iNext;
		list->iNext = rev;
		rev = list;
		list = next;
		}
	while (rev)
		{
		NFastSemaphore* s = rev->iSem;
		rev = rev->iNext;
		NKern::FSSignal(s);
		}
	}

TInt ClockFrequenciesChanged()
	{
	TScheduler& s = TheScheduler;
	NFastSemaphore sem(0);
	SFrequencies f;
	f.iSem = &sem;
	NThread* ct = NKern::CurrentThread();
	NThread* lbt = TScheduler::LBThread();
	NKern::ThreadEnterCS();
	TBool first = f.AddToQueue();
	if (!lbt || lbt == ct)
		TScheduler::DoFrequencyChanged(&s);
	else if (first)
		s.iFreqChgDfc.Enque();
	NKern::FSWait(&sem);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

