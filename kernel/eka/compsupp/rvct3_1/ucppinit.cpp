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
// This file is part of usrt.lib and ksrt.lib.
// 
//


extern "C" {

// This calls each of the compiler constructed functions referenced from pi_ctorvec.
// These functions arrange to 'call' the appropriate constructor for the 'static' instance
// (in fact the call may be inlined). If the class of the instance has a destructor then 
// compiler records that this object needs 'destructing' at 'exit' time. It does this by 
// calling the function __cxa_atexit. We provide our own definition of this.
// 


__asm void __cpp_initialize__aeabi_()
    {
    CODE32

    IMPORT |SHT$$INIT_ARRAY$$Base|  [WEAK]
    IMPORT |SHT$$INIT_ARRAY$$Limit| [WEAK]

    // Export std::nothrow from here.
    EXPORT    _ZSt7nothrow

    STMFD    r13!,{r3-r5,r14}

    LDR      r4,base
    LDR      r5,limit
    CMP      r4,r5
    
    // Exit if the array is empty.
    LDMEQFD  r13!,{r3-r5,pc}

loop
	LDR      r0,[r4,#0]
    ADD      r0,r0,r4
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

base
    DCD    |SHT$$INIT_ARRAY$$Base|
limit
    DCD    |SHT$$INIT_ARRAY$$Limit|

// cheat - defining this here saves a whole 4 bytes!!! - value is never used
_ZSt7nothrow
    }


} // extern "C"

