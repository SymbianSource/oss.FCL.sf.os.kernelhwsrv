/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#include "kernel/kern_priv.h"
#include <e32btrace.h>
#include "utraceepbussdio.h"


#if !defined(_USE_UTRACE_)

TModuleUid TTraceContext::DefaultModuleUid()
	{
	return EXECUTABLE_DEFAULT_MODULEUID;
	}

#if defined(SYMBIAN_TRACE_EXECUTABLE_INCLUDE)

void Printf(TTraceContext aTraceContext, const char* aFmt, ...)
	{
	TBuf8<KMaxPrintfSize> buf;
	VA_LIST list;
	VA_START(list, aFmt);
	Kern::AppendFormat(buf,aFmt,list);
	BTraceContextPcN(aTraceContext.iClassification, 0, EXECUTABLE_DEFAULT_MODULEUID, KFormatPrintf, buf.Ptr(), buf.Size());
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId)
	{
	BTraceContextPc8(aTraceContext.iClassification, 0, EXECUTABLE_DEFAULT_MODULEUID, aFormatId);	
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aA1)
	{
	BTraceContextPc12(aTraceContext.iClassification, 0, EXECUTABLE_DEFAULT_MODULEUID, aFormatId, aA1);		
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint8* aPtr, TUint32 aLen)
	{
	BTraceContextPcN(aTraceContext.iClassification, 0, EXECUTABLE_DEFAULT_MODULEUID, aFormatId, aPtr, aLen);
	}

#else // SYMBIAN_TRACE_EXECUTABLE_INCLUDE

void Printf(TTraceContext aTraceContext, const char* aFmt, ...)
	{
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId)
	{
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aA1)
	{
	}

void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint8* aPtr, TUint32 aLen)
	{
	}


#endif // SYMBIAN_TRACE_EXECUTABLE_INCLUDE
#endif // _USE_BTRACE_

#if defined(SYMBIAN_TRACE_EXECUTABLE_INCLUDE)
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aArgCount, TUint32 aA1, ...)
	{
	const TUint32 KMaxArgs = 8;
	if (aArgCount > KMaxArgs)
		return;
	TUint32 args[KMaxArgs];
	TInt argLen = aArgCount << 2;
	memcpy(args, &aA1, argLen);
	
	Trace(aTraceContext, aFormatId, reinterpret_cast<TUint8*>(args), argLen);
	}
#else // SYMBIAN_TRACE_EXECUTABLE_INCLUDE
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aArgCount, TUint32 aA1, ...)
	{
	}
#endif
