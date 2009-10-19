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
// e32\common\arm\atomic_64_v6k.h
// 64 bit atomic operations on V6K processors
// 
//

#include "atomic_ops.h"

#if defined(__OP_LOAD__)
extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_load_acq64(const volatile TAny* /*a*/)
	{
	// R0=a
	// return value in R1:R0

	asm("mov r2, r0 ");
	ENSURE_8BYTE_ALIGNMENT(2);
	asm("1: ");
	LDREXD(0,2);						// R1:R0 = oldv
	STREXD(3,0,2);						// try to write back, R3=0 if success
	asm("cmp r3, #0 ");
	asm("bne 1b ");						// failed - retry
	__LOCAL_DATA_MEMORY_BARRIER__(r3);
	__JUMP(,lr);
	}



#elif defined(__OP_STORE__)

#define	__DO_STORE__		\
	asm("mov r12, r0 ");	\
	asm("1: ");				\
	LDREXD(0,12);			\
	STREXD(1,2,12);			\
	asm("cmp r1, #0 ");		\
	asm("bne 1b ");


extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_store_rel64(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
#ifdef __BARRIERS_NEEDED__				// If no barriers, just fall through to __e32_atomic_store_ord64
#ifndef __EABI__
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
#endif
	// R0=a, R3:R2=v
	// return value in R1:R0 equal to v
	ENSURE_8BYTE_ALIGNMENT(0);
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
	__DO_STORE__
	asm("mov r0, r2 ");
	asm("mov r1, r3 ");
	__JUMP(,lr);
#endif
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __e32_atomic_store_ord64(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0 equal to v
#ifndef __EABI__
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
#endif
	ENSURE_8BYTE_ALIGNMENT(0);
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
	__DO_STORE__
	__LOCAL_DATA_MEMORY_BARRIER__(r1);
	asm("mov r0, r2 ");
	asm("mov r1, r3 ");
	__JUMP(,lr);
	}

#undef __DO_STORE__


#elif defined(__OP_RMW1__)

#ifdef __OP_SWP__
#define	__SAVE_REGS__		asm("str r6, [sp, #-4]! ");
#define	__RESTORE_REGS__	asm("ldr r6, [sp], #4 ");
#define	__SOURCE_REG__		2
#define	__DO_PROCESSING__
#else
#define	__SAVE_REGS__		asm("stmfd sp!, {r4-r6} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r6} ");
#define	__SOURCE_REG__		4
#if defined(__OP_ADD__)
#define	__DO_PROCESSING__	asm("adds r4, r0, r2 ");	asm("adcs r5, r1, r3 ");
#elif defined(__OP_AND__)
#define	__DO_PROCESSING__	asm("and r4, r0, r2 ");		asm("and r5, r1, r3 ");
#elif defined(__OP_IOR__)
#define	__DO_PROCESSING__	asm("orr r4, r0, r2 ");		asm("orr r5, r1, r3 ");
#elif defined(__OP_XOR__)
#define	__DO_PROCESSING__	asm("eor r4, r0, r2 ");		asm("eor r5, r1, r3 ");
#endif
#endif

#define __DO_RMW1_OP__				\
	asm("mov r12, r0 ");			\
	ENSURE_8BYTE_ALIGNMENT(0);		\
	asm("1: ");						\
	LDREXD(0,12);					\
	__DO_PROCESSING__				\
	STREXD(6,__SOURCE_REG__,12);	\
	asm("cmp r6, #0 ");				\
	asm("bne 1b ");


extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
#ifndef __EABI__
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
#endif
	__SAVE_REGS__
	__DO_RMW1_OP__
	__RESTORE_REGS__
	__JUMP(,lr);
#endif
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*v*/)
	{
	// R0=a, R3:R2=v
	// return value in R1:R0
#ifndef __EABI__
	asm("mov r3, r2 ");
	asm("mov r2, r1 ");
#endif
	__SAVE_REGS__
	__DO_RMW1_OP__
	__LOCAL_DATA_MEMORY_BARRIER__(r6);
	__RESTORE_REGS__
	__JUMP(,lr);
	}

#undef __SAVE_REGS__
#undef __RESTORE_REGS__
#undef __DO_RMW1_OP__
#undef __SOURCE_REG__
#undef __DO_PROCESSING__


#elif defined(__OP_CAS__)

#define	__SAVE_REGS__		asm("stmfd sp!, {r4-r7} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r7} ");

#define __DO_CAS_OP__				\
	asm("ldmia r1, {r6-r7} ");		\
	ENSURE_8BYTE_ALIGNMENT(0);		\
	asm("1: ");						\
	LDREXD(4,0);					\
	asm("cmp r4, r6 ");				\
	asm("cmpeq r5, r7 ");			\
	asm("bne 2f ");					\
	STREXD(12,2,0);					\
	asm("cmp r12, #0 ");			\
	asm("bne 1b ");					\
	asm("2: ");						\
	asm("stmneia r1, {r4-r5} ");	\
	asm("movne r0, #0 ");			\
	asm("moveq r0, #1 ");


extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__SAVE_REGS__
	__DO_CAS_OP__
	__RESTORE_REGS__
	__JUMP(,lr);
#endif
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

extern "C" EXPORT_C __NAKED__ TBool __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ * /*q*/, __TYPE__ /*v*/)
	{
	// R0=a, R1=q, R3:R2=v
	// return value in R0
	__SAVE_REGS__
	__DO_CAS_OP__
	__LOCAL_DATA_MEMORY_BARRIER__(r12);
	__RESTORE_REGS__
	__JUMP(,lr);
	}

#undef __SAVE_REGS__
#undef __RESTORE_REGS__
#undef __DO_CAS_OP__



#elif defined(__OP_AXO__)

#ifdef __EABI__
#define	__SAVE_REGS__		asm("mov r1, sp "); asm("stmfd sp!, {r4-r8} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r8} ");
#else
#define	__SAVE_REGS__		asm("str r3, [sp, #-4]! "); asm("mov r3, r2 "); asm("mov r2, r1 "); asm("mov r1, sp "); asm("stmfd sp!, {r4-r8} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r8,r12} ");
#endif

#define __DO_AXO_OP__				\
	asm("ldmia r1, {r4-r5} ");		\
	asm("mov r12, r0 ");			\
	ENSURE_8BYTE_ALIGNMENT(0);		\
	asm("1: ");						\
	LDREXD(0,12);					\
	asm("and r6, r0, r2 ");			\
	asm("and r7, r1, r3 ");			\
	asm("eor r6, r6, r4 ");			\
	asm("eor r7, r7, r5 ");			\
	STREXD(8,6,12);					\
	asm("cmp r8, #0 ");				\
	asm("bne 1b ");


#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__SAVE_REGS__
	__DO_AXO_OP__
	__RESTORE_REGS__
	__JUMP(,lr);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=u, [SP+4,0]=v
	// return value in R1:R0
	__SAVE_REGS__
	__DO_AXO_OP__
	__LOCAL_DATA_MEMORY_BARRIER__(r8);
	__RESTORE_REGS__
	__JUMP(,lr);
	}

#undef __SAVE_REGS__
#undef __RESTORE_REGS__
#undef __DO_AXO_OP__


#elif defined(__OP_RMW3__)

#ifdef __EABI__
#define	__SAVE_REGS__		asm("mov r1, sp "); asm("stmfd sp!, {r4-r10} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r10} ");
#else
#define	__SAVE_REGS__		asm("str r3, [sp, #-4]! "); asm("mov r3, r2 "); asm("mov r2, r1 "); asm("mov r1, sp "); asm("stmfd sp!, {r4-r10} ");
#define	__RESTORE_REGS__	asm("ldmfd sp!, {r4-r10,r12} ");
#endif

#if	defined(__OP_TAU__)
#define	__COND_GE__		"cs"
#define	__COND_LT__		"cc"
#elif	defined(__OP_TAS__)
#define	__COND_GE__		"ge"
#define	__COND_LT__		"lt"
#endif

#define __DO_RMW3_OP__				\
	asm("ldmia r1, {r4-r7} ");		\
	asm("mov r12, r0 ");			\
	ENSURE_8BYTE_ALIGNMENT(0);		\
	asm("1: ");						\
	LDREXD(0,12);					\
	asm("subs r8, r0, r2 ");		\
	asm("sbcs r9, r1, r3 ");		\
	asm("mov" __COND_GE__ " r8, r4 ");	\
	asm("mov" __COND_GE__ " r9, r5 ");	\
	asm("mov" __COND_LT__ " r8, r6 ");	\
	asm("mov" __COND_LT__ " r9, r7 ");	\
	asm("adds r8, r8, r0 ");		\
	asm("adcs r9, r9, r1 ");		\
	STREXD(10,8,12);				\
	asm("cmp r10, #0 ");			\
	asm("bne 1b ");


#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rel,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,rlx,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__SAVE_REGS__
	__DO_RMW3_OP__
	__RESTORE_REGS__
	__JUMP(,lr);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,ord,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
#ifdef __BARRIERS_NEEDED__				// If no barriers, all ordering variants collapse to same function
	__LOCAL_DATA_MEMORY_BARRIER_Z__(r12);
#endif
	}

#ifdef __EABI__
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(volatile TAny* /*a*/, __TYPE__ /*t*/, __TYPE__ /*u*/, __TYPE__ /*v*/)
#else
extern "C" EXPORT_C __NAKED__ __TYPE__ __fname__(__OPERATION__,acq,64)(int,int,int,int)
#endif
	{
	// R0=a, R3:R2=t, [SP+4,0]=u, [SP+12,8]=v
	// return value in R1:R0
	__SAVE_REGS__
	__DO_RMW3_OP__
	__LOCAL_DATA_MEMORY_BARRIER__(r10);
	__RESTORE_REGS__
	__JUMP(,lr);
	}

#undef __SAVE_REGS__
#undef __RESTORE_REGS__
#undef __DO_RMW3_OP__
#undef __COND_GE__
#undef __COND_LT__


#endif

// Second inclusion undefines temporaries
#include "atomic_ops.h"
