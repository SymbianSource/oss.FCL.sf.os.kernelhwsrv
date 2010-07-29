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
// Mike Kinghan, mikek@symbian.org, for Symbian Foundation.
//
// Description:
// This file is part of usrt.lib and ksrt.lib.
// 
//


extern "C" __NAKED__ void __cpp_initialize__aeabi_()
    {
    asm(".code 32");
	asm(".weak SHT$$INIT_ARRAY$$Base");
	asm(".weak SHT$$INIT_ARRAY$$Limit");
	asm(".global _ZSt7nothrow");     
	asm("STMFD    r13!,{r3-r5,r14}");
	asm("LDR      r4,base");
	asm("LDR      r5,limit");
	asm("CMP      r4,r5");
    // Exit if the array is empty.
    asm("LDMEQFD  r13!,{r3-r5,pc}");
	asm("loop:");
    asm("LDR      r0,[r4,#0]");
    asm("ADD      r0,r0,r4");
#ifdef __MARM_ARMV4__
    asm("ADR      r14,ret");
    asm("MOV      pc,r0");
#else
    asm("BLX      r0");
#endif
	asm("ret:");
    asm("ADD      r4,r4,#4");
    asm("CMP      r4,r5");
    asm("BNE      loop");
    asm("LDMFD    r13!,{r3-r5,pc}");
	asm("base:");
    asm(".word SHT$$INIT_ARRAY$$Base");
	asm("limit:");
    asm(".word SHT$$INIT_ARRAY$$Limit");
	asm("_ZSt7nothrow:");
    }

