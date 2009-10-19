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
// toplevel initialization/destruction routines for 'user side' code compiled 
// with the ARMEDG compiler. intended for static linking
// 
//

#include "cppinit.h"

NUKE_SYMBOL(__call_ctors(void));
NUKE_SYMBOL(__call_dtors(void));

extern "C" {
// linker symbols 
__weak PFV C$$pi_ctorvec$$Base;
__weak PFV C$$pi_ctorvec$$Limit;
__weak void _fp_init(void);


// This calls each of the compiler constructed functions referenced from pi_ctorvec.
// These functions arrange to 'call' the appropriate constructor for the 'static' instance
// (in fact the call may be inlined). If the class of the instance has a destructor then 
// compiler records that this object needs 'destructing' at 'exit' time. It does this by 
// calling the function __cxa_atexit. We provide our own definition of this.
// 
void __cpp_initialise(void)
    {
    void (*fp_init_fn)(void) = _fp_init;
    if (fp_init_fn) fp_init_fn();
    PFV * ctor_vec = &C$$pi_ctorvec$$Base;
    PFV * ctor_limit = &C$$pi_ctorvec$$Limit;
    for(; ctor_vec < ctor_limit; ctor_vec++) (RELOCATE(ctor_vec, PFV))();
    }
}	  





