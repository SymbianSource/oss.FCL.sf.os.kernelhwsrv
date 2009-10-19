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
#include <e32std_private.h>
#include <e32panic.h>

extern "C" {

// Import these functions from drtrvct.dll.
IMPORT_C int __rt_raise(int signal, int type);
IMPORT_C int __rt_exit(int aReturnCode);

int raise(int signal)
    { 
    return __rt_raise(signal, 0); 
    }

EXPORT_C void abort(int signal)
    { 
    __rt_raise(signal, 1); 
    __rt_exit(1);
    }

EXPORT_C void __cxa_pure_virtual()
    {
    User::Panic( _L("Pure virtual"), EPureVirtualCalled );
    }

} // extern "C"


//
// Create the type_info object for void.
//

extern char * _ZTVN10__cxxabiv123__fundamental_type_infoE;
extern const char * const  $Sub$$_ZTSv = "v";

__asm void __rt_exporter_dummy()
{
    EXTERN _ZTVN10__cxxabiv123__fundamental_type_infoE
    EXTERN _ZTSv
    EXPORT |$Sub$$_ZTIv|

|$Sub$$_ZTIv|

    DCD _ZTVN10__cxxabiv123__fundamental_type_infoE
    DCD _ZTSv
}

