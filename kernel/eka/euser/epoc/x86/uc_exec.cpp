// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\x86\uc_exec.cpp
// 
//

#include <e32svr.h>
#include <u32exec.h>


/******************************************************************************
 * Slow executive calls with preprocessing or extra arguments
 ******************************************************************************/
EXPORT_C TBool BTrace::Out(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	ext.iA2 = 0;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86

	if((TUint)aDataSize>8u)
		{
		if((TUint)aDataSize>KMaxBTraceDataArray+4u)
			return Exec::BTraceOutBig(a0,a1,ext,aDataSize);
		a0 += 4;
		aDataSize -= 4;
		ext.iA2 = *((TUint32*&)aData)++;
		ext.iA3 = (TUint32)aData;
		return Exec::BTraceOut(a0,a1,ext,aDataSize);
		}

	if((TUint)aDataSize>4u)
		ext.iA3 = ((TUint32*)aData)[1];
	if(aDataSize)
		ext.iA2 = ((TUint32*)aData)[0];
	a0 += aDataSize;
	aDataSize = 0;
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFiltered(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutFilteredX(TUint32 a0, TUint32 a1, TUint32 a2, TUint32 a3)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = a3;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,0);
	}

EXPORT_C TBool BTrace::OutFilteredN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = a2;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize)
	{
	BTrace::SExecExtension ext;
	a0 |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = 0;
	ext.iA3 = (TUint32)aData;
	ext.iPc = (&a0)[-1]; // return address on X86

	if((TUint)aDataSize>8u)
		{
		if((TUint)aDataSize>KMaxBTraceDataArray+4u)
			return Exec::BTraceOutBig(a0,a1,ext,aDataSize);
		a0 += 4;
		aDataSize -= 4;
		ext.iA2 = *((TUint32*&)aData)++;
		ext.iA3 = (TUint32)aData;
		return Exec::BTraceOut(a0,a1,ext,aDataSize);
		}

	if((TUint)aDataSize>4u)
		ext.iA3 = ((TUint32*)aData)[1];
	if(aDataSize)
		ext.iA2 = ((TUint32*)aData)[0];
	a0 += aDataSize;
	aDataSize = 0;
	return Exec::BTraceOut(a0,a1,ext,aDataSize);
	}

EXPORT_C TBool BTrace::OutFilteredPcFormatBig(TUint32 aHeader, TUint32 aModuleUid, TUint32 aPc, TUint16 aFormatId, const TAny* aData, TInt aDataSize)
	{
 	BTrace::SExecExtension ext;
	aHeader |= EMissingRecord<<BTrace::EFlagsIndex*8; // overload meaning of this flag to indicate filtered trace
	ext.iA2 = aFormatId;
	ext.iA3 = (TUint32)aData;
	ext.iPc = aPc;

	if((TUint)aDataSize>KMaxBTraceDataArray)
		return Exec::UTraceOut(aHeader,aModuleUid,ext,aDataSize);
	aHeader += 4;
	return Exec::BTraceOut(aHeader,aModuleUid,ext,aDataSize);
	}

/**
Signal this threads request semaphore.

This is similar to RThread::RequestComplete() except that no TRequestStatus object
is modified.

May only be used to signal a thread in the same process as the callers.

@panic KERN-EXEC 46 if the thread is not in the same process as the callers
*/
EXPORT_C void RThread::RequestSignal() const
	{
	Exec::ThreadRequestSignal(iHandle);
	}



