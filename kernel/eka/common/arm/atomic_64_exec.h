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
// e32\common\arm\atomic_64_exec.h
// User-side 64 bit atomic operations on V6 or V5 processors using Exec calls
// WARNING: GCC98r2 doesn't align registers so 'v' ends up in R2:R1 not R3:R2
// 
//

#include "atomic_ops.h"

#ifdef __CPU_ARMV6
// Write paging supported, so atomics must work on paged memory, so SLOW exec needed
#define	__ATOMIC64_EXEC__(op)	SLOW_EXEC1_NR(EExecSlowAtomic##op##64)
#else
// Write paging not supported, so atomics can assume unpaged memory, so FAST exec OK
#define	__ATOMIC64_EXEC__(op)	FAST_EXEC1_NR(EFastExecFastAtomic##op##64)
#endif

#ifdef __BARRIERS_NEEDED__
#error Barriers not supported on V6/V5, only V6K/V7
#endif

#if defined(__OP_LOAD__)
#error LOAD same as kernel side
#elif defined(__OP_STORE__)
#error STORE same as kernel side

#elif defined(__OP_RMW1__)

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
	ENSURE_8BYTE_ALIGNMENT(0);
#if defined(__OP_SWP__)
	asm("mov r1, #0 ");
	asm("mov r12, #0 ");
	asm("stmfd sp!, {r2-r3} ");		// i2 = XOR mask = v
	asm("stmfd sp!, {r1,r12} ");	// i1 = AND mask = 0
	asm("stmfd sp!, {r0-r1} ");		// iA = a
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Axo);
	asm("ldmia sp!, {r0-r1} ");
	asm("add sp, sp, #16 ");
#elif defined(__OP_ADD__)
	asm("stmfd sp!, {r0-r3} ");
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Add);
	asm("ldmia sp!, {r0-r1} ");
	asm("add sp, sp, #8 ");
#elif defined(__OP_AND__)
	asm("mov r1, #0 ");
	asm("mov r12, #0 ");
	asm("stmfd sp!, {r1,r12} ");	// i2 = XOR mask = 0
	asm("stmfd sp!, {r0-r3} ");		// i1 = AND mask = v, iA=a
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Axo);
	asm("ldmia sp!, {r0-r1} ");
	asm("add sp, sp, #16 ");
#elif defined(__OP_IOR__)
	asm("mvn r1, r2 ");				// r12:r1 = ~r3:r2
	asm("mvn r12, r3 ");
	asm("stmfd sp!, {r2-r3} ");		// i2 = XOR mask = v
	asm("stmfd sp!, {r1,r12} ");	// i1 = AND mask = ~v
	asm("stmfd sp!, {r0-r1} ");		// iA = a
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Axo);
	asm("ldmia sp!, {r0-r1} ");
	asm("add sp, sp, #16 ");
#elif defined(__OP_XOR__)
	asm("mvn r1, #0 ");
	asm("mvn r12, #0 ");
	asm("stmfd sp!, {r2-r3} ");		// i2 = XOR mask = v
	asm("stmfd sp!, {r1,r12} ");	// i1 = AND mask = 0xFFFFFFFFFFFFFFFF
	asm("stmfd sp!, {r0-r1} ");		// iA = a
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Axo);
	asm("ldmia sp!, {r0-r1} ");
	asm("add sp, sp, #16 ");
#endif
	__JUMP(,lr);
	}



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
	ENSURE_8BYTE_ALIGNMENT(0);
	asm("stmfd sp!, {r0-r3} ");		// iA=a, iQ=q, i1=v
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Cas);			// returns result in R0
	asm("add sp, sp, #16 ");
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
	ENSURE_8BYTE_ALIGNMENT(0);
#ifdef __EABI__
									// i2 = XOR mask = v already on stack
	asm("stmfd sp!, {r0-r3} ");		// i1 = AND mask = u, iA = a
#else
	asm("stmfd sp!, {r1-r3} ");		// i1 = AND mask = u, i2 = XOR mask = v (high word already on stack)
	asm("stmfd sp!, {r0-r1} ");		// iA = a, dummy word for i0 (unused)
#endif
	asm("mov r0, sp ");
	__ATOMIC64_EXEC__(Axo);
	asm("ldmia sp!, {r0-r1} ");
#ifdef __EABI__
	asm("add sp, sp, #8 ");
#else
	asm("add sp, sp, #12 ");
#endif
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
	ENSURE_8BYTE_ALIGNMENT(0);
#ifdef __EABI__
									// i3 = v already on stack
									// i2 = u already on stack
	asm("stmfd sp!, {r0-r3} ");		// i1 = t, iA = a
#else
	// v and high word of u already on stack
	asm("stmfd sp!, {r1-r3} ");		// i1 = t, i2 = u (high word already on stack)
	asm("stmfd sp!, {r0-r1} ");		// iA = a, dummy word for i0 (unused)
#endif
	asm("mov r0, sp ");
#if	defined(__OP_TAU__)
	__ATOMIC64_EXEC__(Tau);
#elif	defined(__OP_TAS__)
	__ATOMIC64_EXEC__(Tas);
#endif
	asm("ldmia sp!, {r0-r1} ");
#ifdef __EABI__
	asm("add sp, sp, #8 ");
#else
	asm("add sp, sp, #12 ");
#endif
	__JUMP(,lr);
	}

#endif

// Second inclusion undefines temporaries
#include "atomic_ops.h"
