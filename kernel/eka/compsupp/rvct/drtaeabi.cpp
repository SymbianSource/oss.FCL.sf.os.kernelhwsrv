// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is part of drtaeabi.dll.
// 
//


#include <e32std.h>
#include <e32panic.h>

extern "C" {

EXPORT_C void abort()
    { 
    User::RaiseException(EExcAbort);
    }

EXPORT_C void __cxa_pure_virtual()
    {
    User::Panic( _L("Pure virtual"), EPureVirtualCalled );
    }

#if __ARMCC_VERSION < 300000 
IMPORT_C void* __get_typeid(void*);
EXPORT_C void* __ARM_get_typeid(void* p)
	{
	return __get_typeid(p);
	}
#endif

} // extern "C"


//
// Create the type_info object for void.
//

extern const char * const  $Sub$$_ZTSv = "v";

__asm void __rt_exporter_dummy()
	{
	IMPORT _ZTVN10__cxxabiv123__fundamental_type_infoE [DYNAMIC]
	IMPORT _ZTSv

    #if __ARMCC_VERSION < 300000
	EXPORT |$Sub$$_ZTIv|
    #else
	EXPORT |$Sub$$_ZTIv| [DYNAMIC]
    #endif

|$Sub$$_ZTIv|

	DCD _ZTVN10__cxxabiv123__fundamental_type_infoE
	DCD _ZTSv
	}

#if __ARMCC_VERSION > 400000
asm void __symbian_prevent_export()
	{
    IMPORT __rt_uread4
    IMPORT __rt_uread8
    IMPORT __rt_uwrite4
    IMPORT __rt_uwrite8
    IMPORT _ll_mul
    IMPORT _ll_scmp
    IMPORT _ll_sdiv
    IMPORT _ll_shift_l
    IMPORT _ll_sshift_r
    IMPORT _ll_ucmp
    IMPORT _ll_udiv
    IMPORT _ll_udiv_donemoving
    IMPORT _ll_ushift_r

    IMPORT __ARM_array_delete_general
    IMPORT __ARM_array_new_general
    IMPORT __ARM_vec_cleanup_rethrow
	}
#endif

