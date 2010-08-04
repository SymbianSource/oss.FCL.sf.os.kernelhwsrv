// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Mike Kinghan, mikek@symbian.org, for Symbian Foundation
//
// Contributors:
//
// Description:
// kernelhwsrv/kernel/eka/common/arm/gccehlp.cpp
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

EXPORT_C void __cxa_guard_abort() {}

EXPORT_C void __cxa_guard_acquire() {}

EXPORT_C void __cxa_guard_release() {}

#endif
}

//
// The global new operator.
//
#include <kernel/kernel.h>

EXPORT_C TAny* operator new[](TUint aSize, const std::nothrow_t&) __NO_THROW
	{
	return Kern::Alloc(aSize);
	}

EXPORT_C TAny* operator new(TUint aSize, const std::nothrow_t&) __NO_THROW
	{
	return Kern::Alloc(aSize);
	}



__NAKED__ void __rt_exporter_dummy(void)
	{
// Ensure that "vtable for __cxxabiv1::__si_class_type_info" is available from ekern.exe
// ** This is almost certainly just creating an instance of the symbol, without supplying the 
// ** required functionality!

#define COMM_SYMBOL(x)  asm(".comm " x ",4")

COMM_SYMBOL("_ZTVN10__cxxabiv117__class_type_infoE");     // vtable for __cxxabiv1::__class_type_info
COMM_SYMBOL("_ZTVN10__cxxabiv120__si_class_type_infoE");  // vtable for __cxxabiv1::__si_class_type_info
COMM_SYMBOL("_ZTVN10__cxxabiv121__vmi_class_type_infoE"); // vtable for __cxxabiv1::__vmi_class_type_info


// Implementations exist as "hidden" in libgcc.a, so we need to pull them in and reveal them
// ** This version will completely fail to do that, but creates junk to be exported
#define IMPORT_HIDDEN(x)  asm(".comm " x ",4");

// unaligned-funcs.c
IMPORT_HIDDEN("__aeabi_uread4");
IMPORT_HIDDEN("__aeabi_uread8");
IMPORT_HIDDEN("__aeabi_uwrite4");
IMPORT_HIDDEN("__aeabi_uwrite8");

	}

