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


extern "C" __asm void __cpp_initialize__aeabi_()
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
    #if __ARMCC_VERSION > 300000
    ADD      r0,r0,r4
    #endif

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

_ZSt7nothrow
    }

