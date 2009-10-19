// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\common\arm\atomic_8_16_v6.h
// 8 and 16 bit atomic operations on V6 processors using LDREX/STREX and shifts
// Must use these user side so that it will work with data paging.
// 
//

#include "atomic_ops.h"

#if defined(__CPU_ARM_HAS_LDREX_STREX_V6K) || !defined(__CPU_ARM_HAS_LDREX_STREX)
#error 8/16 bit atomic operations using LDREX/STREX
#endif

#ifdef __BARRIERS_NEEDED__
#error Barriers not supported on V6/V5, only V6K/V7
#endif

#if __DATA_SIZE__!=8 && __DATA_SIZE__!=16
#error Unsupported data size
#endif

#if defined(__OP_LOAD__)
#error Load operation not defined

#elif defined(__OP_STORE__)
#error Store operation not defined

#elif defined(__OP_RMW1__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=v
	// return value in R0
	asm("bic r12, r0, #3 ");
	asm("and r3, r0, #3 ");
	asm("stmfd sp!, {r4-r5} ");
	asm("movs r4, r3, lsl #3 ");	// r4 = bit offset of data in word
#if __DATA_SIZE__==16
	asm("eor r4, r4, #16 ");		// r4 = right rotate to take hword of interest to top (0->16, 2->0)
	asm("mov r5, r4 ");
#elif __DATA_SIZE__==8
	asm("add r4, r4, #8 ");
	asm("and r4, r4, #31 ");		// right rotate to take byte of interest to top byte (0->8, 1->16, 2->24, 3->0)
	asm("rsb r5, r4, #32 ");
	asm("and r5, r5, #31 ");		// right rotate to take top byte to byte of interest (0->8, 1->16, 2->24, 3->0)
#endif
#if defined(__OP_AND__)
	asm("mvn r1, r1 ");
	asm("mvn r1, r1, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// v in upper bits of r1, 1's in lower bits
#endif
	asm("1: ");
	LDREX(0,12);
	asm("mov r0, r0, ror r4 ");		// rotate so desired data is at top
#if defined(__OP_SWP__)
	asm("orr r2, r1, r0, lsl #%a0" : : "i" (__DATA_SIZE__));
	asm("mov r2, r2, ror #%a0" : : "i" (__DATA_SIZE__));
#elif defined(__OP_ADD__)
	asm("add r2, r0, r1, lsl #%a0" : : "i" (32-__DATA_SIZE__));
#elif defined(__OP_AND__)
	asm("and r2, r0, r1 ");
#elif defined(__OP_IOR__)
	asm("orr r2, r0, r1, lsl #%a0" : : "i" (32-__DATA_SIZE__));
#elif defined(__OP_XOR__)
	asm("eor r2, r0, r1, lsl #%a0" : : "i" (32-__DATA_SIZE__));
#endif
	asm("mov r2, r2, ror r5 ");		// rotate back
	STREX(3,2,12);
	asm("cmp r3, #0 ");
	asm("bne 1b ");
	asm("mov r0, r0, lsr #%a0" : : "i" (32-__DATA_SIZE__));	// shift return value so data is at bottom
	asm("ldmfd sp!, {r4-r5} ");
	__JUMP(,lr);
	}


#elif defined(__OP_CAS__)
extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rel,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rlx,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,ord,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,acq,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R2=v
	// return value in R0
	asm("bic r12, r0, #3 ");
	asm("and r3, r0, #3 ");
	asm("stmfd sp!, {r4-r6} ");
	asm("movs r4, r3, lsl #3 ");	// r4 = bit offset of data in word
	__LDR_INST__( ," r6, [r1] ");	// r6 = expected value
#if __DATA_SIZE__==16
	asm("mov r5, r4 ");				// r5 = right rotate to take bottom hword to hword of interest (0->0, 2->16)
	asm("eor r4, r4, #16 ");		// r4 = right rotate to take hword of interest to top (0->16, 2->0)
#elif __DATA_SIZE__==8
	asm("rsb r5, r4, #32 ");
	asm("and r5, r5, #31 ");		// right rotate to take bottom byte to byte of interest (0->0, 1->24, 2->16, 3->8)
	asm("add r4, r4, #8 ");
	asm("and r4, r4, #31 ");		// right rotate to take byte of interest to top byte (0->8, 1->16, 2->24, 3->0)
#endif
	asm("1: ");
	LDREX(0,12);
	asm("mov r0, r0, ror r4 ");		// rotate so desired data is at top
	asm("cmp r6, r0, lsr #%a0" : : "i" (32-__DATA_SIZE__));		// check desired data against expected value *q
	asm("bne 2f ");					// if no match, give up
	asm("orr r0, r2, r0, lsl #%a0" : : "i" (__DATA_SIZE__));	// substitute value v into bottom bits of r0, replacing original
	asm("mov r0, r0, ror r5 ");		// rotate back (bottom byte/hword goes to byte/hword of interest)
	STREX(3,0,12);
	asm("cmp r3, #0 ");
	asm("bne 1b ");
	asm("2: ");
	asm("movne r0, r0, lsr #%a0" : : "i" (32-__DATA_SIZE__));	// shift return value so data is at bottom
	__STR_INST__(ne, "r0, [r1] ");	// update *q with observed value
	asm("ldmfd sp!, {r4-r6} ");
	asm("movne r0, #0 ");			// if *a not updated return FALSE
	asm("moveq r0, #1 ");			// if *a updated return TRUE
	__JUMP(,lr);
	}


#elif defined(__OP_AXO__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=u, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=u, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=u, R2=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=u, R2=v
	// return value in R0
	asm("and r3, r0, #3 ");
	asm("bic r12, r0, #3 ");
	asm("stmfd sp!, {r4-r5} ");
	asm("mvn r1, r1, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// ~u in upper bits of r1, 1's in lower bits
	asm("movs r4, r3, lsl #3 ");	// r4 = bit offset of data in word
	asm("mov r1, r1, lsr #%a0" : : "i" (32-__DATA_SIZE__));	// ~u in lower bits of r1, zero extended
	asm("1: ");
	LDREX(0,12);
	asm("bic r5, r0, r1, lsl r4 ");	// r5 = r0 & u in relevant bit positions, other bit positions unchanged
	asm("eor r5, r5, r2, lsl r4 ");
	STREX(3,5,12);
	asm("cmp r3, #0 ");
	asm("bne 1b ");
	asm("mov r0, r0, lsr r4 ");		// shift original value so byte/hword of interest is at the bottom
	asm("ldmfd sp!, {r4-r5} ");
#if __DATA_SIZE__==16
	asm("bic r0, r0, #0xff000000 ");	// mask upper 16 bits
	asm("bic r0, r0, #0x00ff0000 ");
#elif __DATA_SIZE__==8
	asm("and r0, r0, #0xff ");			// mask upper 24 bits
#endif
	__JUMP(,lr);
	}


#elif defined(__OP_RMW3__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=t, R2=u, R3=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=t, R2=u, R3=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=t, R2=u, R3=v
	// return value in R0
	// just fall through
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,__DATA_SIZE__)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=t, R2=u, R3=v
	// return value in R0
	asm("bic r12, r0, #3 ");
	asm("stmfd sp!, {r4-r7} ");
	asm("and r4, r0, #3 ");
	asm("movs r4, r4, lsl #3 ");	// r4 = bit offset of data in word
#if __DATA_SIZE__==16
	asm("eor r4, r4, #16 ");		// r4 = right rotate to take hword of interest to top (0->16, 2->0)
	asm("mov r5, r4 ");				// r5 = right rotate to undo r4
#elif __DATA_SIZE__==8
	asm("add r4, r4, #8 ");
	asm("and r4, r4, #31 ");		// right rotate to take byte of interest to top byte (0->8, 1->16, 2->24, 3->0)
	asm("rsb r5, r4, #32 ");
	asm("and r5, r5, #31 ");		// right rotate to take top byte to byte of interest (0->8, 1->16, 2->24, 3->0)
#endif
	asm("1: ");
	LDREX(0,12);
	asm("mov r0, r0, ror r4 ");		// rotate so desired data is at top
#if	defined(__OP_TAU__)
	asm("cmp r1, r0, lsr #%a0" : : "i" (32-__DATA_SIZE__));	// t - byte/hword of interest
	asm("addls r6, r0, r2, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// if t<=*a add u
	asm("addhi r6, r0, r3, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// if t>*a add v
#elif	defined(__OP_TAS__)
	asm("cmp r1, r0, asr #%a0" : : "i" (32-__DATA_SIZE__));	// t - byte/hword of interest
	asm("addle r6, r0, r2, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// if t<=*a add u
	asm("addgt r6, r0, r3, lsl #%a0" : : "i" (32-__DATA_SIZE__));	// if t>*a add v
#endif
	asm("mov r6, r6, ror r5 ");		// rotate back
	STREX(7,6,12);
	asm("cmp r7, #0 ");
	asm("bne 1b ");
	asm("ldmfd sp!, {r4-r7} ");
#if	defined(__OP_TAU__)
	asm("mov r0, r0, lsr #%a0" : : "i" (32-__DATA_SIZE__));	// shift return value so data is at bottom
#elif	defined(__OP_TAS__)
	asm("mov r0, r0, asr #%a0" : : "i" (32-__DATA_SIZE__));	// shift return value so data is at bottom
#endif
	__JUMP(,lr);
	}

#endif

// Second inclusion undefines temporaries
#include "atomic_ops.h"
