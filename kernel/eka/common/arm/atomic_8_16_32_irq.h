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
// e32\common\arm\atomic_8_16_32_irq.h
// 8, 16 and 32 bit kernel side atomic operations on V5 or V6 processors
// These work by disabling interrupts
// 
//

#include "atomic_ops.h"

#if defined(__OP_LOAD__)
#error Load operation not defined
#elif defined(__OP_STORE__)
#error Store operation not defined
#elif defined(__OP_RMW1__)

#if defined(__OP_SWP__)
#define	__DO_PROCESSING__
#elif defined(__OP_ADD__)
#define	__DO_PROCESSING__	asm("add r1, r0, r1 ");
#elif defined(__OP_AND__)
#define	__DO_PROCESSING__	asm("and r1, r0, r1 ");
#elif defined(__OP_IOR__)
#define	__DO_PROCESSING__	asm("orr r1, r0, r1 ");
#elif defined(__OP_XOR__)
#define	__DO_PROCESSING__	asm("eor r1, r0, r1 ");
#endif

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
	asm("mov r3, r0 ");
	__DISABLE_INTERRUPTS__(r12,r2);
	__LDR_INST__( ," r0, [r3] ");
	__DO_PROCESSING__
	__STR_INST__( ," r1, [r3] ");
	__RESTORE_INTERRUPTS__(r12);
	__JUMP(,lr);
	}

#undef __DO_PROCESSING__


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
	asm("str r4, [sp, #-4]! ");
	__LDR_INST__( ," r4, [r1] ");
	__DISABLE_INTERRUPTS__(r12,r3);
	__LDR_INST__( ," r3, [r0] ");
	asm("cmp r3, r4 ");
	__STR_INST__(eq," r2, [r0] ");
	__RESTORE_INTERRUPTS__(r12);
	__STR_INST__(ne," r3, [r1] ");
	asm("ldr r4, [sp], #4 ");
	asm("movne r0, #0 ");
	asm("moveq r0, #1 ");
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
	asm("mov r3, r0 ");
	__DISABLE_INTERRUPTS__(r12,r0);
	__LDR_INST__( ," r0, [r3] ");
	asm("and r1, r0, r1 ");
	asm("eor r1, r1, r2 ");
	__STR_INST__( ," r1, [r3] ");
	__RESTORE_INTERRUPTS__(r12);
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
	asm("mov r12, r0 ");
	asm("str r4, [sp, #-4]! ");
	__DISABLE_INTERRUPTS__(r4,r0);
#if	defined(__OP_TAU__)
	__LDR_INST__( ," r0, [r12] ");
	asm("cmp r0, r1 ");
	asm("addcs r1, r0, r2 ");
	asm("addcc r1, r0, r3 ");
#elif	defined(__OP_TAS__)
	__LDRS_INST__( ," r0, [r12] ");
	asm("cmp r0, r1 ");
	asm("addge r1, r0, r2 ");
	asm("addlt r1, r0, r3 ");
#endif
	__STR_INST__( ," r1, [r12] ");
	__RESTORE_INTERRUPTS__(r4);
	asm("ldr r4, [sp], #4 ");
	__JUMP(,lr);
	}


#endif

// Second inclusion undefines temporaries
#include "atomic_ops.h"
