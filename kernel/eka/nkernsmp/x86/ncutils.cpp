// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\x86\ncutils.cpp
// 
//

#include <x86.h>

//#define __DBG_MON_FAULT__
//#define __RAM_LOADED_CODE__
//#define __EARLY_DEBUG__
void InitFpu();


TUint32 NKern::IdleGenerationCount()
	{
	return TheScheduler.iIdleGenerationCount;
	}

void NKern::DoIdle()
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss = SubScheduler();	// OK since idle thread is locked to CPU
	TUint32 m = ss.iCpuMask;

	s.iIdleSpinLock.LockIrq();	// don't allow any more idle DFCs for now
	TUint32 orig_cpus_not_idle = __e32_atomic_and_ord32(&s.iCpusNotIdle, ~m);
	if (orig_cpus_not_idle == m)
		{
		// all CPUs idle
		if (!s.iIdleDfcs.IsEmpty())
			{
			__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);		// we aren't idle after all
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
	if (ss.iCurrentThread->iSavedSP)
		{
		// rescheduled between entry to NKern::Idle() and here
		// go round again to see if any more threads to pull from other CPUs
		__e32_atomic_ior_ord32(&s.iCpusNotIdle, m);	// we aren't idle after all
		s.iIdleSpinLock.UnlockIrq();
		return;
		}

	s.iIdleSpinLock.UnlockOnly();	// leave interrupts disabled

	NKIdle(0);
	
	}

TUint32 ContextId()
	{
	switch(NKern::CurrentContext())
		{
	case NKern::EThread:
		return (TUint32)NKern::CurrentThread();
	case NKern::EIDFC:
		return 3;
	case NKern::EInterrupt:
		return 2;
	default:
		return 0;
		}
	}

EXPORT_C TBool BTrace::Out(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;

	TUint32 pc = (&a0)[-1]; // return address on X86
	__ACQUIRE_BTRACE_LOCK();
	TBool r = traceData.iHandler(a0,0,0,a1,a2,a3,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;

	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	__ACQUIRE_BTRACE_LOCK();
	TBool r = traceData.iHandler(a0,0,context,a1,a2,a3,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;

	if(TUint(aDataSize)>KMaxBTraceDataArray)
		{
		aDataSize = KMaxBTraceDataArray;
		a0 |= BTrace::ERecordTruncated<<(BTrace::EFlagsIndex*8);
		}
	a0 += aDataSize<<(BTrace::ESizeIndex*8);

	TUint32 pc = (&a0)[-1]; // return address on X86
	TBool r;
	__ACQUIRE_BTRACE_LOCK();
	if (!aDataSize)
		r = traceData.iHandler(a0,0,0,a1,a2,0,0,pc);
	else if (aDataSize<=4)
		r = traceData.iHandler(a0,0,0,a1,a2,*(TUint32*)aData,0,pc);
	else
		r = traceData.iHandler(a0,0,0,a1,a2,(TUint32)aData,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;

	if(TUint(aDataSize)>KMaxBTraceDataArray)
		{
		aDataSize = KMaxBTraceDataArray;
		a0 |= BTrace::ERecordTruncated<<(BTrace::EFlagsIndex*8);
		}
	a0 += aDataSize<<(BTrace::ESizeIndex*8);

	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	TBool r;
	__ACQUIRE_BTRACE_LOCK();
	if(!aDataSize)
		r = traceData.iHandler(a0,0,context,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		r = traceData.iHandler(a0,0,context,a1,a2,*(TUint32*)aData,0,pc);
	else
		r = traceData.iHandler(a0,0,context,a1,a2,(TUint32)aData,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	TBool r = DoOutBig(a0,a1,aData,aDataSize,context,pc);
	return r;
	}

EXPORT_C TBool BTrace::OutFiltered(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;

	TUint32 pc = (&a0)[-1]; // return address on X86
	__ACQUIRE_BTRACE_LOCK();
	TBool r = traceData.iHandler(a0,0,0,a1,a2,a3,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutFilteredX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;

	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	__ACQUIRE_BTRACE_LOCK();
	TBool r = traceData.iHandler(a0,0,context,a1,a2,a3,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutFilteredN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;

	if(TUint(aDataSize)>KMaxBTraceDataArray)
		{
		aDataSize = KMaxBTraceDataArray;
		a0 |= BTrace::ERecordTruncated<<(BTrace::EFlagsIndex*8);
		}
	a0 += aDataSize<<(BTrace::ESizeIndex*8);

	TUint32 pc = (&a0)[-1]; // return address on X86
	TBool r;
	__ACQUIRE_BTRACE_LOCK();
	if(!aDataSize)
		r = traceData.iHandler(a0,0,0,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		r = traceData.iHandler(a0,0,0,a1,a2,*(TUint32*)aData,0,pc);
	else
		r = traceData.iHandler(a0,0,0,a1,a2,(TUint32)aData,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutFilteredNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;

	if(TUint(aDataSize)>KMaxBTraceDataArray)
		{
		aDataSize = KMaxBTraceDataArray;
		a0 |= BTrace::ERecordTruncated<<(BTrace::EFlagsIndex*8);
		}
	a0 += aDataSize<<(BTrace::ESizeIndex*8);

	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	TBool r;
	__ACQUIRE_BTRACE_LOCK();
	if(!aDataSize)
		r = traceData.iHandler(a0,0,context,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		r = traceData.iHandler(a0,0,context,a1,a2,*(TUint32*)aData,0,pc);
	else
		r = traceData.iHandler(a0,0,context,a1,a2,(TUint32)aData,0,pc);
	__RELEASE_BTRACE_LOCK();
	return r;
	}

EXPORT_C TBool BTrace::OutFilteredBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;
	TBool r = DoOutBig(a0,a1,aData,aDataSize,context,pc);
	return r;
	}

EXPORT_C TBool BTrace::OutFilteredPcFormatBig(TUint32 aHeader, TUint32 aModuleUid, TUint32 aPc, TUint16 aFormatId, const TAny* aData, TInt aDataSize)
	{
	return EFalse; //kernel side not implemented yet
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
	return &::CpuIdleHandler;
	}


void NKern::Init0(TAny* a)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("VIB=%08x", a));
	SVariantInterfaceBlock* v = (SVariantInterfaceBlock*)a;
	TheScheduler.iVIB = v;
	__NK_ASSERT_ALWAYS(v && v->iVer==0 && v->iSize==sizeof(SVariantInterfaceBlock));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iVer=%d iSize=%d", v->iVer, v->iSize));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxCpuClock=%08x %08x", I64HIGH(v->iMaxCpuClock), I64LOW(v->iMaxCpuClock)));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iTimestampFreq=%u", v->iTimestampFreq));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("iMaxTimerClock=%u", v->iMaxTimerClock));
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = TheSubSchedulers[i];
		ss.iSSX.iCpuFreqRI.Set(v->iCpuFreqR[i]);
		ss.iSSX.iTimerFreqRI.Set(v->iTimerFreqR[i]);

		ss.iSSX.iTimestampOffset.i64 = 0;
		v->iCpuFreqR[i] = 0;
		v->iTimerFreqR[i] = 0;
		}
	TheScheduler.iSX.iTimerMax = (v->iMaxTimerClock / 128);
	InitFpu();
	InterruptInit0();
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
	TUint32 mf32 = (TUint32)TheScheduler.iSX.iTimerMax;
	TUint64 mf(mf32);
	TUint64 ticks = mf*TUint64(aMicroseconds) + UI64LIT(999999);
	ticks /= UI64LIT(1000000);
	if (ticks > TUint64(TInt(KMaxTInt)))
		return KMaxTInt;
	else
		return (TInt)ticks;
	}

TBool TSubScheduler::Detached()
	{
	return FALSE;
	}

TBool TScheduler::CoreControlSupported()
	{
	return FALSE;
	}

void TScheduler::CCInitiatePowerUp(TUint32 /*aCores*/)
	{
	}

void TScheduler::CCIndirectPowerDown(TAny*)
	{
	}

void TScheduler::DoFrequencyChanged(TAny*)
	{
	}
