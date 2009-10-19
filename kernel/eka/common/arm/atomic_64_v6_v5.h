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
// e32\common\arm\atomic_64_v6_v5.h
// Kernel-side 64 bit atomic operations on V6 or V5 processors
// WARNING: GCC98r2 doesn't align registers so 'v' ends up in R2:R1 not R3:R2
// 
//

#include "atomic_ops.h"

#ifdef __BARRIERS_NEEDED__
#error Barriers not supported on V6/V5, only V6K/V7
#endif

#if defined(__OP_LOAD__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_load_acq64(const volatile TAny* /*a*/)
	{
	// R0=a
	// return value in R1:R0
	ENSURE_8BYTE_ALIGNMENT(0);
	asm("ldmia r0, {r0-r1} ");
	__JUMP(,lr);
	}



#elif defined(__OP_STORE__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_store_rel64(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0 equal to v
	// just fall through to __e32_atomic_store_ord64
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_store_ord64(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0 equal to v
	ENSURE_8BYTE_ALIGNMENT(0);
#ifdef __EABI__
	asm("stmia r0, {r2-r3} ");
	asm("mov r0, r2 ");
	asm("mov r1, r3 ");
#else
	asm("stmia r0, {r1-r2} ");
	asm("mov r0, r1 ");
	asm("mov r1, r2 ");
#endif
	__JUMP(,lr);
	}


#elif defined(__OP_RMW1__)

#if defined(__OP_SWP__)
#define	__DO_PROCESSING__
#elif defined(__OP_ADD__)
#define	__DO_PROCESSING__	asm("adds r2, r2, r0 ");	asm("adcs r3, r3, r1 ");
#elif defined(__OP_AND__)
#define	__DO_PROCESSING__	asm("and r2, r2, r0 ");		asm("and r3, r3, r1 ");
#elif defined(__OP_IOR__)
#define	__DO_PROCESSING__	asm("orr r2, r2, r0 ");		asm("orr r3, r3, r1 ");
#elif defined(__OP_XOR__)
#define	__DO_PROCESSING__	asm("eor r2, r2, r0 ");		asm("eor r3, r3, r1 ");
#endif

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0

#ifndef __EABI__
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
#endif
	asm("mov r12, r0 ");
	ENSURE_8BYTE_ALIGNMENT(12);
	asm("str r4, [sp, #-4]! ");
	__DISABLE_INTERRUPTS__(r4,r1);
	asm("ldmia r12, {r0-r1} ");
	__DO_PROCESSING__
	asm("stmia r12, {r2-r3} ");
	__RESTORE_INTERRUPTS__(r4);
	asm("ldr r4, [sp], #4 ");
	__JUMP(,lr);
	}

#undef __DO_PROCESSING__


#elif defined(__OP_CAS__)

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
	// just fall through to __e32_atomic_*_acq64
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
	asm("stmfd sp!, {r4-r7} ");
	asm("ldmia r1, {r4-r5} ");
	ENSURE_8BYTE_ALIGNMENT(0);
	__DISABLE_INTERRUPTS__(r12,r6);
	asm("ldmia r0, {r6-r7} ");
	asm("cmp r6, r4 ");
	asm("cmpeq r7, r5 ");
	asm("stmeqia r0, {r2-r3} ");
	__RESTORE_INTERRUPTS__(r12);	// flags preserved
	asm("stmneia r1, {r6-r7} ");
	asm("ldmfd sp!, {r4-r7} ");
	asm("moveq r0, #1 ");
	asm("movne r0, #0 ");
	__JUMP(,lr);
	}


#elif defined(__OP_AXO__)
#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
#ifdef __EABI__
	asm("mov r1, sp ");
	asm("stmfd sp!, {r4-r6} ");
	asm("mov r12, r0 ");
	asm("ldmia r1, {r4-r5} ");
#else
	asm("stmfd sp!, {r4-r6} ");
	asm("mov r4, r3 ");
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
	asm("mov r12, r0 ");
	asm("ldr r5, [sp, #12] ");
#endif
	ENSURE_8BYTE_ALIGNMENT(12);
	__DISABLE_INTERRUPTS__(r6,r0);
	asm("ldmia r12, {r0-r1} ");
	asm("and r2, r2, r0 ");
	asm("and r3, r3, r1 ");
	asm("eor r2, r2, r4 ");
	asm("eor r3, r3, r5 ");
	asm("stmia r12, {r2-r3} ");
	__RESTORE_INTERRUPTS__(r6);	// flags preserved
	asm("ldmfd sp!, {r4-r6} ");
	__JUMP(,lr);
	}


#elif defined(__OP_RMW3__)
#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
	// just fall through to __e32_atomic_*_acq64
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
#ifdef __EABI__
	asm("mov r1, sp ");
	asm("stmfd sp!, {r4-r8} ");
	asm("mov r12, r0 ");
	asm("ldmia r1, {r4-r7} ");
#else
	asm("mov r12, sp ");
	asm("stmfd sp!, {r4-r8} ");
	asm("mov r4, r3 ");
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
	asm("ldmia r12, {r5-r7} ");
	asm("mov r12, r0 ");
#endif
	ENSURE_8BYTE_ALIGNMENT(12);
	__DISABLE_INTERRUPTS__(r8,r0);
	asm("ldmia r12, {r0-r1} ");
	asm("subs r2, r0, r2 ");
	asm("sbcs r3, r1, r3 ");
#if	defined(__OP_TAU__)
	asm("movcc r4, r6 ");
	asm("movcc r5, r7 ");
#elif	defined(__OP_TAS__)
	asm("movlt r4, r6 ");
	asm("movlt r5, r7 ");
#endif
	asm("adds r2, r0, r4 ");
	asm("adcs r3, r1, r5 ");
	asm("stmia r12, {r2-r3} ");
	__RESTORE_INTERRUPTS__(r8);	// flags preserved
	asm("ldmfd sp!, {r4-r8} ");
	__JUMP(,lr);
	}

#endif

// Second inclusion undefines temporaries
#include "atomic_ops.h"
