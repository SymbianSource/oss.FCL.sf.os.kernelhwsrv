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
// e32\common\arm\atomic_8_16_32_exec.h
// 8, 16 and 32 bit user side atomic operations on V5 processors
// These work by calling Exec functions.
// 
//

#include "atomic_ops.h"

#ifdef __CPU_ARMV6
#error 8/16/32 bit atomic operations on ARMv6 must use LDREX/STREX
#else
// Write paging not supported, so atomics can assume unpaged memory, so FAST exec OK

#if __DATA_SIZE__==8
#define	__ATOMIC_EXEC__(op)		FAST_EXEC1_NR(EFastExecFastAtomic##op##8)
#elif __DATA_SIZE__==16
#define	__ATOMIC_EXEC__(op)		FAST_EXEC1_NR(EFastExecFastAtomic##op##16)
#elif __DATA_SIZE__==32
#define	__ATOMIC_EXEC__(op)		FAST_EXEC1_NR(EFastExecFastAtomic##op##32)
#else
#error Invalid data size
#endif

#endif

#ifdef __BARRIERS_NEEDED__
#error Barriers not supported on V6/V5, only V6K/V7
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
#if defined(__OP_ADD__)
	asm("stmfd sp!, {r0-r1} ");		// iA=a, i0=v, i1, i2 unused
	asm("mov r0, sp ");
	__ATOMIC_EXEC__(Add);
	asm("add sp, sp, #8 ");
#else
#if defined(__OP_SWP__)
	asm("mov r2, r1 ");				// XOR mask = v
	asm("mov r1, #0 ");				// AND mask = 0
#elif defined(__OP_AND__)
	asm("mov r2, #0 ");				// AND mask = v, XOR mask = 0
#elif defined(__OP_IOR__)
	asm("mov r2, r1 ");				// XOR mask = v
	asm("mvn r1, r1 ");				// AND mask = ~v
#elif defined(__OP_XOR__)
	asm("mov r2, r1 ");				// XOR mask = v
	asm("mvn r1, #0 ");				// AND mask = 0xFFFFFFFF
#endif
	asm("stmfd sp!, {r0-r2} ");		// iA=a, i0=AND mask, i1=XOR mask, i2 unused
	asm("mov r0, sp ");
	__ATOMIC_EXEC__(Axo);
	asm("add sp, sp, #12 ");
#endif
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
	asm("stmfd sp!, {r0-r2} ");		// iA=a, iQ=q, i1=v, i2 unused
	asm("mov r0, sp ");
	__ATOMIC_EXEC__(Cas);
	asm("add sp, sp, #12 ");
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
	asm("stmfd sp!, {r0-r2} ");		// iA=a, i0=u, i1=v, i2 unused
	asm("mov r0, sp ");
	__ATOMIC_EXEC__(Axo);
	asm("add sp, sp, #12 ");
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
	asm("stmfd sp!, {r0-r3} ");		// iA=a, i0=t, i1=u, i2=v
	asm("mov r0, sp ");
#if	defined(__OP_TAU__)
	__ATOMIC_EXEC__(Tau);
#elif	defined(__OP_TAS__)
	__ATOMIC_EXEC__(Tas);
#endif
	asm("add sp, sp, #16 ");
	__JUMP(,lr);
	}


#endif

#undef __ATOMIC_EXEC__

// Second inclusion undefines temporaries
#include "atomic_ops.h"
