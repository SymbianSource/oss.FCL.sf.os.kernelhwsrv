// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// with the ARMCC EABI compiler. intended for static linking
// 
//

extern "C" {

// This calls each of the compiler constructed functions referenced from pi_ctorvec.
// These functions arrange to 'call' the appropriate constructor for the 'static' instance
// (in fact the call may be inlined). If the class of the instance has a destructor then 
// compiler records that this object needs 'destructing' at 'exit' time. It does this by 
// calling the function __cxa_atexit. We provide our own definition of this.
// 
// This is the EABI compliant version - it uses .init_array rather than C$$pi_ctorvec
// which means we need to do via assembler :-(

#ifdef __thumb
  __asm void __cpp_initialize__aeabi_(void)
    {
    CODE16
#ifndef _NO_FP
    IMPORT    _fp_init [WEAK]
#endif
    IMPORT    |.init_array$$Base| [WEAK]
    IMPORT    |.init_array$$Limit| [WEAK]

    // export std::nothrow from here
    EXPORT    _ZSt7nothrow

#ifndef _NO_FP
    LDR      r0,fpinit
#endif
    PUSH     {r4-r6,r14}
#ifndef _NO_FP
    CMP      r0,#0
    BEQ      skip
    BL       _fp_init  ;
skip
#endif
    LDR      r4,base
    LDR      r5,limit
    B        check
loop
    LDR      r0,[r4,#0]
    ADD      r0,r0,r4

    BLX      r0
    ADD      r4,#4
check
    CMP      r4,r5
    BCC      loop
    POP      {r4-r6,pc}

#ifndef _NO_FP
fpinit
    DCD    _fp_init
#endif

// cheat - saved a whole 4 bytes!!! - value is never user
_ZSt7nothrow
base
    DCD    |.init_array$$Base|
limit
    DCD    |.init_array$$Limit|

    }

#else
  __asm void __cpp_initialize__aeabi_(void)
    {
    CODE32
#ifndef _NO_FP
    IMPORT    _fp_init [WEAK]
#endif
    IMPORT    |.init_array$$Base| [WEAK]
    IMPORT    |.init_array$$Limit| [WEAK]

    // export std::nothrow from here
    EXPORT    _ZSt7nothrow

#ifndef _NO_FP
    LDR	     r0, fpinit
#endif
    STMFD    r13!,{r3-r5,r14}
#ifndef _NO_FP
    CMP      r0, #0
    BEQ	     skip
    BL	     _fp_init
skip
#endif
    LDR      r4,base
    LDR      r5,limit
    CMP      r4,r5
    LDMEQFD  r13!,{r3-r5,pc}
loop
    LDR      r0,[r4,#0]
    ADR      r14,ret
    ADD      r0,r0,r4
    MOV      pc,r0
ret
    ADD      r4,r4,#4
    CMP      r4,r5
    BNE      loop
    LDMFD    r13!,{r3-r5,pc}

#ifndef _NO_FP
fpinit
    DCD    _fp_init
#endif

base
    DCD    |.init_array$$Base|
limit
    DCD    |.init_array$$Limit|

// cheat - defining this here saves a whole 4 bytes!!! - value is never user
_ZSt7nothrow

    }
#endif

}

