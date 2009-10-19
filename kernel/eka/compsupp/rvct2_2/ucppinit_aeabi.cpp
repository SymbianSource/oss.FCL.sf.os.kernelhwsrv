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
// The code supports 2.2 both bpabi and non-bpabi. The values in .init_array are
// pointers in both environments, so the library is common to both.
// 
//

extern "C" {

// This calls each of the compiler constructed functions referenced from pi_ctorvec.
// These functions arrange to 'call' the appropriate constructor for the 'static' instance
// (in fact the call may be inlined). If the class of the instance has a destructor then 
// compiler records that this object needs 'destructing' at 'exit' time. It does this by 
// calling the function __cxa_atexit. We provide our own definition of this.
// 

#ifdef __thumb
  __asm void __cpp_initialize__aeabi_(void)
    {
    CODE16
#ifndef _NO_FP
    IMPORT    _fp_init [WEAK]
#endif
    IMPORT    |SHT$$INIT_ARRAY$$Base| [WEAK]
    IMPORT    |SHT$$INIT_ARRAY$$Limit| [WEAK]

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

    BLX      r0
    ADD      r4,#4
check
    CMP      r4,r5
    BCC      loop
    POP      {r4-r6,pc}

#ifndef _NO_FP
    ALIGN 4 
fpinit
    DCD    _fp_init
#endif

// cheat - saved a whole 4 bytes!!! - value is never used
_ZSt7nothrow
base
    DCD    |SHT$$INIT_ARRAY$$Base|
limit
    DCD    |SHT$$INIT_ARRAY$$Limit|

    }

#else
  __asm void __cpp_initialize__aeabi_(void)
    {
    CODE32
#ifndef _NO_FP
    IMPORT    _fp_init [WEAK]
#endif
    IMPORT    |SHT$$INIT_ARRAY$$Base| [WEAK]
    IMPORT    |SHT$$INIT_ARRAY$$Limit| [WEAK]

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
#ifdef __MARM_ARMV4__
    ADR      r14,ret
    MOV      pc,r0
#else
    BLX      r0
#endif
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
    DCD    |SHT$$INIT_ARRAY$$Base|
limit
    DCD    |SHT$$INIT_ARRAY$$Limit|

// cheat - defining this here saves a whole 4 bytes!!! - value is never used
_ZSt7nothrow

    }
#endif

}
