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
// e32\euser\epoc\win32\uc_exec.cpp
// 
//

#define __GEN_USER_EXEC_CODE__

#include "uc_std.h"
#include <e32svr.h>
#include <emulator.h>

typedef TInt (__fastcall *TDispatcher)(TInt, TInt*);
TInt __fastcall LazyDispatch(TInt aFunction, TInt* aArgs);

#pragma data_seg(".data2")
#ifdef __VC32__
#pragma bss_seg(".data2")
#endif
static TDispatcher TheDispatcher = &LazyDispatch;
#pragma data_seg()
#ifdef __VC32__
#pragma bss_seg()
#endif

TInt __fastcall LazyDispatch(TInt aFunction, TInt* aArgs)
	{
	HINSTANCE kernel = GetModuleHandleA("ekern.exe");
	if (kernel)
		{
		TDispatcher dispatcher = (TDispatcher)Emulator::GetProcAddress(kernel, (LPCSTR)1);
		if (dispatcher)
			{
			TheDispatcher = dispatcher;
			return dispatcher(aFunction, aArgs);
			}
		}
	ExitProcess(101);
	return 0;
	}

#include <u32exec.h>

/******************************************************************************
 * Slow executive calls with preprocessing or extra arguments
 ******************************************************************************/

__NAKED__ TInt Exec::SessionSend(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server.
//
	{
	__DISPATCH(EExecSessionSend|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::SessionSendSync(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server using thread's dedicated message slot.
//
	{
	__DISPATCH(EExecSessionSendSync|EXECUTIVE_SLOW)
	}


__NAKED__ TInt Exec::MessageIpcCopy(TInt /*aHandle*/, TInt /*aParam*/, SIpcCopyInfo& /*aInfo*/, TInt /*anOffset*/)
//
// Perform a descriptor-to-descriptor IPC copy
//
	{

	__DISPATCH(EExecMessageIpcCopy|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::BTraceOut(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecBTraceOut|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::BTraceOutBig(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecBTraceOutBig|EXECUTIVE_SLOW)
	}

__NAKED__ TInt Exec::UTraceOut(TUint32 /*a0*/, TUint32 /*a1*/, const BTrace::SExecExtension& /*aExtension*/, TInt /*aDataSize*/)
	{
	__DISPATCH(EExecUTraceOut|EXECUTIVE_SLOW)
	}

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

__NAKED__ void ExecRequestComplete(TInt /*aHandle*/, TRequestStatus*& /*aStatus*/, TInt /*aReason*/)
	{
	_asm mov ecx, [esp+8]			// ecx = TRequestStatus**
	_asm xor eax, eax				//
	_asm lock xchg eax, [ecx]		// eax=TRequestStatus*, zero TRequestStatus*
	_asm cmp eax, 0					//
	_asm je ExecRequestComplete_ret
	_asm mov ecx, [esp+12]			// ecx = aReason
	_asm mov [eax], ecx				// store aReason in request status
	__DISPATCH(EExecThreadRequestSignal|EXECUTIVE_SLOW)
	_asm ExecRequestComplete_ret: ret
	}




EXPORT_C void RThread::RequestComplete(TRequestStatus*& aStatus, TInt aReason) const
/**
Signals this thread that an asynchronous request originating from this thread,
is complete.

The request is associated with the specified request status object supplied
by this thread.

Typically, the caller of this function is the service provider responsible
for satisfying the request made by this thread.

The request is completed with the completion code passed in aReason. This
value is copied into this thread's request status, *aStatus, before signalling
this thread's request semaphore.

The meaning of the completion code is a matter of convention to be decided
between the service provider and this thread.

In a client-server situation, completion of a request takes place in the context
of the server thread, but the pointer is interpreted in the address space
of the client.

It is often the case in client-server situations that the client and the server
are in the same address space (i.e. the same process).

Setting the pointer to the request status to NULL is a convenience, not all
servers need it.

@param aStatus A reference to a pointer to the request status originally
               supplied by this thread. This is a pointer into this thread's
               address space, which may be different to the thread currently
               executing (this code). On return, the pointer to the request
               status is set to NULL.

@param aReason The completion code of this request.
*/
	{
	ExecRequestComplete(iHandle,aStatus,aReason);
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



void ExitCurrentThread(TExitType aType, TInt aReason, const TDesC8* aCategory)
	{
	Exec::ThreadKill(KCurrentThreadHandle, aType, aReason, aCategory);
	}

