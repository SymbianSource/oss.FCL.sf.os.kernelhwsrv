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
// e32\nklib\arm\armcchelp.cpp
// 
//

#include "../common.h"
#include <nkern.h>

extern "C" {
EXPORT_C int __aeabi_idiv0 (int return_value)
    {
      FAULT();
      return return_value;
    }

EXPORT_C long long __aeabi_ldiv0 (long long return_value)
    {
      FAULT();
      return return_value;
    }

EXPORT_C int __cxa_pure_virtual()
//
// Gets called for any unreplaced pure virtual methods.
//
	{
#ifdef __STANDALONE_NANOKERNEL__
	__NK_ASSERT_ALWAYS(0);
#else
	Panic(EPureVirtualCalled);
#endif
	return 0;
	}

#ifdef __KERNEL_MODE__

void __cxa_end_catch(){}

void __cxa_begin_catch(){}

void __cxa_rethrow(){}
// std::terminate

void __cxa_call_unexpected() {}

void __aeabi_unwind_cpp_pr0() {}

void __cxa_end_cleanup() {}

#endif
}

