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
// e32\kernel\win32\cutils.cpp
// 
//

#include "memmodel.h"
#include <winbase.h>
#include <emulator.h>
#include <property.h>

DJitCrashHandler* Emul::TheJitHandler;
const char* JustInTime;

TInt A::VariantHal(TInt aFunction, TAny* a1, TAny* a2)
	{
	return Emul::TheAsic->VariantHal(aFunction,a1,a2);
	}

void A::DebugPrint(const TText* aPtr, TInt aLen, TBool)
//
// Send the string to the debug output
// mutate LF to CRLF and append CRLF if no trailing CR/LF
//
	{
	const TInt maxSize = 1024;
	TBuf8<maxSize+3> out;
	const TUint8* p = aPtr;
	const TUint8* e = p + aLen;
	TText8* o = (TText8*)out.Ptr();
	TText8* oLimit = o+maxSize;
	TBool CR = EFalse;
	TInt c = -1;
	while (p < e)
		{
		c = *p++;
		switch (c)
			{
		case '\r':
			CR = ETrue;
			break;
		case '\n':
			CR = ETrue;
			// fall through
		default:
			if (CR)
				{
				CR = EFalse;
				*o++ = '\r';
				if(o>=oLimit)
					break;
				}
			*o++ = TUint8(c);
			if(o>=oLimit)
				break;
			}
		}
	if (c != '\n')
		{
		*o++ = '\r';
		*o++ = '\n';
		}
	else if (CR)
		*o++ = '\r';
	*o = '\0';
	out.SetLength(o - out.Ptr());
	Emul::TheAsic->DebugPrint(out);
	}

void KPrintf(const char* aFmt, ...)
//
// Nanokernel formatted print.
//
	{
	TBuf8<256> printBuf;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(printBuf,aFmt,list);
	K::TextTrace(printBuf,EKernelTrace);
	}

EXPORT_C Asic* Arch::TheAsic()
	{
	return Emul::TheAsic;
	}

TPtr8 A::MachineConfiguration()
	{
	return Emul::TheAsic->MachineConfiguration();
	}

TInt A::CallSupervisorFunction(TSupervisorFunction aFunction, TAny* aPtr)
//
// Execute an F32 function
//
	{
	return aFunction(aPtr);
	}

TInt A::SystemTimeInSecondsFrom2000(TInt& aTime)
	{
	return Emul::TheAsic->SystemTimeInSecondsFrom2000(aTime);
	}

TInt A::SetSystemTimeInSecondsFrom2000(TInt aTime)
	{
	return Emul::TheAsic->SetSystemTimeInSecondsFrom2000(aTime);
	}


/**
Default implementation for Kern::NanoWait, which waits for the specified time.

@param anInterval The time to wait in nanoseconds.
*/
void K::DoNanoWait(TUint32 anInterval)
	{
	const TUint32 KLoopOverhead = 35000; // 35us

	LARGE_INTEGER v;
	QueryPerformanceCounter(&v);
	LARGE_INTEGER q;
	QueryPerformanceFrequency(&q);
	q.QuadPart = (q.QuadPart * (anInterval - KLoopOverhead) / 1000000000) + v.QuadPart;
	if (anInterval > KLoopOverhead)
		{
		do
			{
			QueryPerformanceCounter(&v);
			} while (v.QuadPart < q.QuadPart);
		}
	}

LOCAL_C const TText8* ThreadPanicBreakPoint(const TDesC8& aPanic, TInt aVal)
//
// The current thread has been panic'd
//
	{
	TBuf8<256> msg(aPanic);
	msg.Append(':');
	msg.AppendNum(aVal);
	msg.Append('\0');
	const TText8* m = msg.Ptr();
	__BREAKPOINT();
	return m;
	}

LOCAL_C TBool DisplayErrorDialog(Asic::TError aType, const TDesC8& aPanic, TInt aVal)
	{
	// Re-enable interrupts while we put up the error dialog
	NKern::EnableAllInterrupts();
	TBool result = Emul::TheAsic->ErrorDialog(aType, aPanic, aVal);
	NKern::DisableAllInterrupts();
	return result;
	}


extern "C" void NKCrashHandler(TInt aPhase, const TAny* a0, TInt a1)
	{
	if (aPhase==0)
		{
		return;
		}
#ifdef __SMP__
#error SMP on emulator
#endif
	K::DoFault(a0,a1);
	}

void A::StartCrashDebugger(const TAny* a0, TInt a1)
	{
	const TDesC8& msg = *(const TDesC8*)a0;
	if (IsDebuggerPresent()
		|| _stricmp(JustInTime,"debug") == 0
		|| (_stricmp(JustInTime,"query") == 0 && DisplayErrorDialog(Asic::EFatalError, msg, a1)))
		ThreadPanicBreakPoint(msg, a1);
	// Interrupts are disabled here, so we can't be suspended while exiting the process
	ExitProcess(100);
	}

TUint DJitCrashHandler::HandleEvent(TKernelEvent aEvent, TAny* /*a1*/, TAny* /*a2*/, TAny* /*aThis*/)
	{
	DThread* pT = TheCurrentThread;
	if (aEvent==EEventKillThread && pT->iExitType == EExitPanic 
			&& (pT->iOwningProcess->iFlags & KProcessFlagJustInTime))
		{
		TBuf8<16> cat;
		cat.Copy(pT->iExitCategory);
		// If JustInTime requests a 'query' box provide a friendly box
		if (IsDebuggerPresent()
			|| _stricmp(JustInTime,"debug") == 0
			|| (_stricmp(JustInTime,"query") == 0 && Emul::TheAsic->ErrorDialog(Asic::EApplicationError, cat, pT->iExitReason)))
			ThreadPanicBreakPoint(cat, pT->iExitReason);
		}
	return ERunNext;
	}

/**	Increments atomically a counter that's initially positive

	It checks the previous CPU mode and if it's User, it only applies user permissions when accessing aValue.
	It doesn't increment if aValue is negative or zero.

	@param aValue Reference to the counter to increment
	@return Previous value of counter 
*/
EXPORT_C TInt Kern::KUSafeInc(TInt& aValue)
	{
	return __e32_atomic_tas_ord32(&aValue, 1, 1, 0);
	}

/**	Decrements atomically a counter that's initially positive

	It checks the previous CPU mode and if it's User, it only applies user permissions when accessing aValue.
	It doesn't decrement if aValue is negative or zero.

	@param aValue Reference to the counter to decrement
	@return Previous value of counter 
*/
EXPORT_C TInt Kern::KUSafeDec(TInt& aValue)
	{
	return __e32_atomic_tas_ord32(&aValue, 1, -1, 0);
	}

TInt K::FloatingPointTypes(TUint32& aTypes)
	{
	aTypes = EFpTypeNone;
	return KErrNotSupported;
	}

/**	Returns the current state of interrupts
	
	@param	aRequest indicates which state of interrupts should be returned
			
	@return if aRequest is ETrue, function returns ETrue if interrupts are enabled and EFalse otherwise
			if aRequest is EFalse, function returns ETrue if interrupts are disabled and EFalse otherwise
*/
TBool InterruptsStatus(TBool aRequest)
	{
	return Interrupt.InterruptsStatus(aRequest);
	}

TInt K::FloatingPointSystemId(TUint32& /*aSysId*/)
	{
	return KErrNotSupported;
	}
