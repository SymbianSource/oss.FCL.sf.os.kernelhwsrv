// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\int_svr_calls.cpp
// 
//

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#undef EXPORT_C
#define EXPORT_C
#undef IMPORT_C
#define IMPORT_C
#include <e32std.h>
#include <e32std_private.h>

#ifdef __WINS__
// NB: The following is a straight rip from the win32 uc_exec.cpp
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
	return 0;
	}

// Do the exec stuff in a cpp file for WINS
#define __GEN_USER_EXEC_CODE__
#include <e32svr.h>
#include <u32exec.h>

#undef EXPORT_C
#define EXPORT_C __declspec(dllexport)

// ripped from e32\euser\epoc\x86\uc_exec.cpp

__NAKED__ TInt Exec::SessionSend(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server.
//
	{
	SLOW_EXEC4(EExecSessionSend);
	}

__NAKED__ TInt Exec::SessionSendSync(TInt /*aHandle*/, TInt /*aFunction*/, TAny* /*aPtr*/, TRequestStatus* /*aStatus*/)
//
// Send a blind message to the server using thread's dedicated message slot.
//
	{
	SLOW_EXEC4(EExecSessionSendSync);
	}

EXPORT_C TInt SessionCreate(const TDesC8& aName, TInt aMsgSlots, const TSecurityPolicy* aPolicy, TInt aType)
	{
	return Exec::SessionCreate(aName, aMsgSlots, aPolicy, aType);
	}

EXPORT_C TInt SessionSend(TInt aHandle, TInt aFunction, TAny* aArgs, TRequestStatus* aStatus)
	{
	return Exec::SessionSend(aHandle, aFunction, aArgs, aStatus);
	}

EXPORT_C TInt SessionSendSync(TInt aHandle, TInt aFunction, TAny* aArgs, TRequestStatus* aStatus)
	{
	return Exec::SessionSendSync(aHandle, aFunction, aArgs, aStatus);
	}

EXPORT_C void SetSessionPtr(TInt aHandle, const TAny* aPtr)
	{
	Exec::SetSessionPtr(aHandle, aPtr);
	}
#endif //__WINS__
