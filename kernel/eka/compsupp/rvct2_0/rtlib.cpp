// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// initialize and shutdown (low-level) runtime.
// these are intended to go in user side static runtime lib
// 
//

#include <e32std.h>
#include <e32std_private.h>

extern "C" {
EXPORT_C void __rt_lib_shutdown(void){};

extern void _fp_init(void);
extern void __cpp_initialise(void);

EXPORT_C void __rt_lib_init(void)
    {
    _fp_init();
    __cpp_initialise();
    }
}
