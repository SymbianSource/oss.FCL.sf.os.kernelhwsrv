// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\u32exec.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __U32EXEC_H__
#define __U32EXEC_H__
#include <u32std.h>
#include <cpudefs.h>

#ifdef __KERNEL_MODE__
#define	EXEC_INT64		Int64
#define	EXEC_TIME		TTimeK
#else
#define	EXEC_INT64		TInt64
#define	EXEC_TIME		TInt64
#endif

#define __EXECDECL__	__NAKED__

#if defined(__WINS__)

// Executive call macros for WINS

#define EXECUTIVE_FAST	0x00800000
#define EXECUTIVE_SLOW	0x00000000

#define __DISPATCH(func)			\
	__asm lea	edx, [esp + 0x4]	\
	__asm mov	ecx, (func)			\
	__asm jmp  [TheDispatcher]

#define	FAST_EXEC0(n)	__DISPATCH((n)|EXECUTIVE_FAST)
#define	FAST_EXEC1(n)	__DISPATCH((n)|EXECUTIVE_FAST)
#define	SLOW_EXEC0(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC1(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC2(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC3(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC4(n)	__DISPATCH((n)|EXECUTIVE_SLOW)

#define	KFAST_EXEC0(n)	__DISPATCH((n)|EXECUTIVE_FAST)
#define	KFAST_EXEC1(n)	__DISPATCH((n)|EXECUTIVE_FAST)
#define	KSLOW_EXEC0(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC1(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC2(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC3(n)	__DISPATCH((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC4(n)	__DISPATCH((n)|EXECUTIVE_SLOW)

#elif defined(__CPU_X86)

// Executive call macros for X86
#ifdef __GCC32__
#define	FAST_EXEC0(n)	asm("mov eax, %0": :"i"(n)); \
						asm("int 0x20 \n ret")
#define	FAST_EXEC1(n)	asm("mov eax, %0": :"i"(n)); \
						asm("mov ecx, [esp+4] \n int 0x20 \n ret")
#define	SLOW_EXEC0(n)	asm("mov eax, %0": :"i"(n)); \
						asm("int 0x21 \n ret")
#define	SLOW_EXEC1(n)	asm("mov eax, %0": :"i"(n)); \
						asm("mov ecx, [esp+4] \n int 0x21 \n ret")
#define	SLOW_EXEC2(n)	asm("mov eax, %0": :"i"(n)); \
						asm("mov ecx, [esp+4] \n mov edx, [esp+8] \n int 0x21 \n ret")
#define SLOW_EXEC3(n)	asm("mov eax, %0": :"i"(n)); \
						asm("push ebx         \n" \
							"mov ecx, [esp+8] \n" \
							"mov edx, [esp+12]\n" \
							"mov ebx, [esp+16]\n" \
							"int 0x21         \n" \
							"pop ebx          \n" \
						    "ret")

#define SLOW_EXEC4(n)	asm("mov eax, %0": :"i"(n)); \
						asm("push ebx         \n" \
							"push esi         \n" \
							"mov ecx, [esp+12]\n" \
							"mov edx, [esp+16]\n" \
							"mov ebx, [esp+20]\n" \
							"mov esi, [esp+24]\n" \
							"int 0x21         \n" \
							"pop esi          \n" \
							"pop ebx          \n" \
							"ret")
#else
#define	FAST_EXEC0(n)	_asm mov eax, n _asm int 20h _asm ret
#define	FAST_EXEC1(n)	_asm mov eax, n _asm mov ecx, [esp+4] _asm int 20h _asm ret

#define	SLOW_EXEC0(n)	_asm mov eax, n _asm int 21h _asm ret
#define	SLOW_EXEC1(n)	_asm mov eax, n _asm mov ecx, [esp+4] _asm int 21h _asm ret
#define	SLOW_EXEC2(n)	_asm mov eax, n _asm mov ecx, [esp+4] _asm mov edx, [esp+8] _asm int 21h _asm ret

#define SLOW_EXEC3(n)	_asm mov eax, n			\
						_asm push ebx			\
						_asm mov ecx, [esp+8]	\
						_asm mov edx, [esp+12]	\
						_asm mov ebx, [esp+16]	\
						_asm int 21h			\
						_asm pop ebx			\
						_asm ret

#define SLOW_EXEC4(n)	_asm mov eax, n			\
						_asm push ebx			\
						_asm push esi			\
						_asm mov ecx, [esp+12]	\
						_asm mov edx, [esp+16]	\
						_asm mov ebx, [esp+20]	\
						_asm mov esi, [esp+24]	\
						_asm int 21h			\
						_asm pop esi			\
						_asm pop ebx			\
						_asm ret
#endif

#define	KFAST_EXEC0(n)	FAST_EXEC0(n)
#define	KFAST_EXEC1(n)	FAST_EXEC1(n)
#define	KSLOW_EXEC0(n)	SLOW_EXEC0(n)
#define	KSLOW_EXEC1(n)	SLOW_EXEC1(n)
#define	KSLOW_EXEC2(n)	SLOW_EXEC2(n)
#define KSLOW_EXEC3(n)	SLOW_EXEC3(n)
#define KSLOW_EXEC4(n)	SLOW_EXEC4(n)

#elif defined(__CPU_ARM)

// Executive call macros for ARM

#define EXECUTIVE_FAST	0x00800000
#define EXECUTIVE_SLOW	0x00000000

#define __DISPATCH(n)				\
	asm("swi %a0" : : "i" (n));     \
	__JUMP(,lr);

#define	FAST_EXEC0(n)		__DISPATCH((n)|EXECUTIVE_FAST)
#define	FAST_EXEC1(n)		__DISPATCH((n)|EXECUTIVE_FAST)
#define	SLOW_EXEC0(n)		__DISPATCH((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC1(n)		__DISPATCH((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC2(n)		__DISPATCH((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC3(n)		__DISPATCH((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC4(n)		__DISPATCH((n)|EXECUTIVE_SLOW)

#define __KDISPATCH(n)				\
	asm("stmfd sp!, {ip,lr} ");		\
	asm("swi %a0" : : "i" (n));     \
	__POPRET("ip,");

#define	KFAST_EXEC0(n)		__KDISPATCH((n)|EXECUTIVE_FAST)
#define	KFAST_EXEC1(n)		__KDISPATCH((n)|EXECUTIVE_FAST)
#define	KSLOW_EXEC0(n)		__KDISPATCH((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC1(n)		__KDISPATCH((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC2(n)		__KDISPATCH((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC3(n)		__KDISPATCH((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC4(n)		__KDISPATCH((n)|EXECUTIVE_SLOW)

#define __DISPATCH_NR(n)			\
	asm("swi %a0" : : "i" (n));

#define	FAST_EXEC0_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_FAST)
#define	FAST_EXEC1_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_FAST)
#define	SLOW_EXEC0_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC1_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_SLOW)
#define	SLOW_EXEC2_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC3_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_SLOW)
#define SLOW_EXEC4_NR(n)	__DISPATCH_NR((n)|EXECUTIVE_SLOW)

#define __KDISPATCH_NR(n)			\
	asm("swi %a0" : : "i" (n));

#define	KFAST_EXEC0_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_FAST)
#define	KFAST_EXEC1_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_FAST)
#define	KSLOW_EXEC0_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC1_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_SLOW)
#define	KSLOW_EXEC2_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC3_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_SLOW)
#define KSLOW_EXEC4_NR(n)	__KDISPATCH_NR((n)|EXECUTIVE_SLOW)

#else
#error Unknown CPU
#endif

#ifdef __LEAVE_EQUALS_THROW__
// Hide TTrap to catch unwary uses of the old cleanup
// mechanism at compile time
class TTrap;
#endif //__LEAVE_EQUALS_THROW__

#include <exec_enum.h>
#include <e32btrace.h>
#include <exec_user.h>

#endif
