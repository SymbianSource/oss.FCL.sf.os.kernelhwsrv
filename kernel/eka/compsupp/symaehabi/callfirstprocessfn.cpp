// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\compsupp\symaehabi\callfirstprocessfn.cpp
// 
//

#include "unwind_env.h"
#include "unwinder.h"
#include "symbian_support.h"
#include <e32panic.h>

#ifdef __LEAVE_EQUALS_THROW__
_LIT(KLitUser,"USER");
#endif

TInt E32Main();

#ifdef __KERNEL_MODE__
#error This file should not be compiled in kernel mode
#endif

extern "C" {

void __cpp_initialize__aeabi_();
void __DLL_Export_Table__();

TInt CallThrdProcEntry(TInt (*aFn)(void*), void* aPtr, TInt aNotFirst)
	{
	TCppRTExceptionsGlobals aExceptionGlobals;

	if (!aNotFirst)
		{
		// Init statics for implicitly linked DLLs
		User::InitProcess();

		// Init statics for EXE
		//pick up export table if we're an exexp
		__DLL_Export_Table__();
		__cpp_initialize__aeabi_();
		}
#ifdef __LEAVE_EQUALS_THROW__
	TInt r = KErrNone;

	try {
		r = aNotFirst ? (*aFn)(aPtr) : E32Main();
		}
	catch (XLeaveException&)
		{
		User::Panic(KLitUser, EUserLeaveWithoutTrap);
		}

	return r;
#else
	return aNotFirst ? (*aFn)(aPtr) : E32Main();
#endif
	}
}
