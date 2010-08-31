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
// e32\nkern\x86\ncutils.cpp
// 
//

#include <x86.h>

//#define __DBG_MON_FAULT__
//#define __RAM_LOADED_CODE__
//#define __EARLY_DEBUG__


void InitFpu();

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
	return traceData.iHandler(a0,0,0,a1,a2,a3,0,pc);
	}

EXPORT_C TBool BTrace::OutX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;

	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	return traceData.iHandler(a0,0,context,a1,a2,a3,0,pc);
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
	if(!aDataSize)
		return traceData.iHandler(a0,0,0,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		return traceData.iHandler(a0,0,0,a1,a2,*(TUint32*)aData,0,pc);
	else
		return traceData.iHandler(a0,0,0,a1,a2,(TUint32)aData,0,pc);
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
	if(!aDataSize)
		return traceData.iHandler(a0,0,context,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		return traceData.iHandler(a0,0,context,a1,a2,*(TUint32*)aData,0,pc);
	else
		return traceData.iHandler(a0,0,context,a1,a2,(TUint32)aData,0,pc);
	}

EXPORT_C TBool BTrace::OutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	TUint32 context = ContextId();
	TUint32 pc = (&a0)[-1]; // return address on X86
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	return DoOutBig(a0,a1,aData,aDataSize,context,pc);
	}

EXPORT_C TBool BTrace::OutFiltered(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	SBTraceData& traceData = BTraceData;
	if(!traceData.iFilter[(a0>>BTrace::ECategoryIndex*8)&0xff])
		return FALSE;
	if(!traceData.CheckFilter2(a1))
		return FALSE;

	TUint32 pc = (&a0)[-1]; // return address on X86
	return traceData.iHandler(a0,0,0,a1,a2,a3,0,pc);
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
	return traceData.iHandler(a0,0,context,a1,a2,a3,0,pc);
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
	if(!aDataSize)
		return traceData.iHandler(a0,0,0,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		return traceData.iHandler(a0,0,0,a1,a2,*(TUint32*)aData,0,pc);
	else
		return traceData.iHandler(a0,0,0,a1,a2,(TUint32)aData,0,pc);
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
	if(!aDataSize)
		return traceData.iHandler(a0,0,context,a1,a2,0,0,pc);
	else if(aDataSize<=4)
		return traceData.iHandler(a0,0,context,a1,a2,*(TUint32*)aData,0,pc);
	else
		return traceData.iHandler(a0,0,context,a1,a2,(TUint32)aData,0,pc);
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
	return DoOutBig(a0,a1,aData,aDataSize,context,pc);
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
	TUint irq = NKern::DisableAllInterrupts();

	aOldHandler = BTraceData.iHandler;
	BTraceData.iHandler = aNewHandler;
	TheScheduler.iBTraceHandler = aNewHandler;

	aOldControl = BTraceData.iControl;
	BTraceData.iControl = aNewControl ? aNewControl : BTraceDefaultControl;

	NKern::RestoreInterrupts(irq);
	}


EXPORT_C TInt BTrace::SetFilter(TUint aCategory, TInt aValue)
	{
	if(!IsSupported(aCategory))
		return KErrNotSupported;
	TUint8* filter = BTraceData.iFilter+aCategory;
	TUint oldValue = *filter;
	if(TUint(aValue)<=1u)
		{
		*filter = (TUint8)aValue;
		BTraceContext4(BTrace::EMetaTrace, BTrace::EMetaTraceFilterChange, (TUint8)aCategory | (aValue<<8));
		if(aCategory==ECpuUsage)
			{
			TheScheduler.iCpuUsageFilter = (TUint8)aValue;
			}
		}
	return oldValue;
	}

EXPORT_C SCpuIdleHandler* NKern::CpuIdleHandler()
	{
	return &::CpuIdleHandler;
	}

void NKern::Init0(TAny*)
	{
	InitFpu();
	}

EXPORT_C TUint32 NKern::CpuTimeMeasFreq()
	{
#ifdef MONITOR_THREAD_CPU_TIME
	return NKern::FastCounterFrequency();
#else
	return 0;
#endif
	}

