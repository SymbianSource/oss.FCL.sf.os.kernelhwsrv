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
// e32\common\x86\atomics.cpp
// 
//

#include <e32atomics.h>
#include <cpudefs.h>

/*
Versions needed:
	WINS/WINSCW		Use X86 locked operations. Assume Pentium or above CPU (CMPXCHG8B available)
	X86				For Pentium and above use locked operations
					For 486 use locked operations for 8, 16, 32 bit. For 64 bit must disable interrupts.
					NOTE: 486 not supported at the moment
	ARMv4/ARMv5		Must disable interrupts.
	ARMv6			LDREX/STREX for 8, 16, 32 bit. For 64 bit must disable interrupts (maybe).
	ARMv6K/ARMv7	LDREXB/LDREXH/LDREX/LDREXD

Need both kernel side and user side versions
*/

#if	defined(__SMP__) || !defined(__EPOC32__)
#define	__BARRIERS_NEEDED__
#define	__LOCK__	lock
#else
#define	__LOCK__
#endif


extern "C" {

#undef	__TUintX__
#undef	__TIntX__
#undef	__fname__
#undef	__A_REG__
#undef	__C_REG__
#undef	__D_REG__
#define	__TUintX__		TUint32
#define	__TIntX__		TInt32
#define	__fname__(x)	x##32
#define	__A_REG__		eax
#define	__C_REG__		ecx
#define	__D_REG__		edx
#include "atomic_skeleton.h"

#undef	__TUintX__
#undef	__TIntX__
#undef	__fname__
#undef	__A_REG__
#undef	__C_REG__
#undef	__D_REG__
#define	__TUintX__		TUint16
#define	__TIntX__		TInt16
#define	__fname__(x)	x##16
#define	__A_REG__		ax
#define	__C_REG__		cx
#define	__D_REG__		dx
#include "atomic_skeleton.h"

#undef	__TUintX__
#undef	__TIntX__
#undef	__fname__
#undef	__A_REG__
#undef	__C_REG__
#undef	__D_REG__
#define	__TUintX__		TUint8
#define	__TIntX__		TInt8
#define	__fname__(x)	x##8
#define	__A_REG__		al
#define	__C_REG__		cl
#define	__D_REG__		dl
#include "atomic_skeleton.h"

#undef	__TUintX__
#undef	__TIntX__
#undef	__fname__
#undef	__A_REG__
#undef	__C_REG__
#undef	__D_REG__

/** Full memory barrier for explicit memory accesses

*/
EXPORT_C __NAKED__ void __e32_memory_barrier()
	{
#ifdef __BARRIERS_NEEDED__
	_asm lock add dword ptr [esp], 0
#endif
	_asm ret
	}


/** Barrier guaranteeing completion as well as ordering

*/
EXPORT_C __NAKED__ void __e32_io_completion_barrier()
	{
	_asm push ebx
	_asm cpuid
	_asm pop ebx
	_asm ret
	}


/** Find the most significant 1 in a 32 bit word

	@param	v	The word to be scanned
	@return		The bit number of the most significant 1 if v != 0
				-1 if v == 0
*/
EXPORT_C __NAKED__ TInt __e32_find_ms1_32(TUint32 /*v*/)
	{
	_asm bsr eax, [esp+4]
	_asm jnz short done
	_asm mov eax, 0ffffffffh
done:
	_asm ret
	}


/** Find the least significant 1 in a 32 bit word

	@param	v	The word to be scanned
	@return		The bit number of the least significant 1 if v != 0
				-1 if v == 0
*/
EXPORT_C __NAKED__ TInt __e32_find_ls1_32(TUint32 /*v*/)
	{
	_asm bsf eax, [esp+4]
	_asm jnz short done
	_asm mov eax, 0ffffffffh
done:
	_asm ret
	}


/** Count the number of 1's in a 32 bit word

	@param	v	The word to be scanned
	@return		The number of 1's
*/
EXPORT_C __NAKED__ TInt __e32_bit_count_32(TUint32 /*v*/)
	{
	_asm mov eax, [esp+4]
	_asm mov edx, eax
	_asm and eax, 0aaaaaaaah
	_asm and edx, 055555555h
	_asm shr eax, 1
	_asm add eax, edx
	_asm mov edx, eax
	_asm and eax, 0cccccccch
	_asm and edx, 033333333h
	_asm shr eax, 2
	_asm add eax, edx
	_asm mov edx, eax
	_asm shr eax, 4
	_asm add eax, edx
	_asm and eax, 00f0f0f0fh
	_asm add al, ah
	_asm mov dl, al
	_asm shr eax, 16
	_asm add al, ah
	_asm xor ah, ah
	_asm add al, dl
	_asm ret
	}


/** Find the most significant 1 in a 64 bit word

	@param	v	The word to be scanned
	@return		The bit number of the most significant 1 if v != 0
				-1 if v == 0
*/
EXPORT_C __NAKED__ TInt __e32_find_ms1_64(TUint64 /*v*/)
	{
	_asm bsr eax, [esp+8]
	_asm jnz short mswnz
	_asm bsr eax, [esp+4]
	_asm jnz short lswnz
	_asm mov eax, 0ffffffffh
mswnz:
	_asm or eax, 32
lswnz:
	_asm ret
	}


/** Find the least significant 1 in a 64 bit word

	@param	v	The word to be scanned
	@return		The bit number of the least significant 1 if v != 0
				-1 if v == 0
*/
EXPORT_C __NAKED__ TInt __e32_find_ls1_64(TUint64 /*v*/)
	{
	_asm bsf eax, [esp+4]
	_asm jnz short lswnz
	_asm bsf eax, [esp+8]
	_asm jnz short mswnz
	_asm mov eax, 0ffffffffh
mswnz:
	_asm or eax, 32
lswnz:
	_asm ret
	}


/** Count the number of 1's in a 64 bit word

	@param	v	The word to be scanned
	@return		The number of 1's
*/
EXPORT_C __NAKED__ TInt __e32_bit_count_64(TUint64 /*v*/)
	{
	_asm mov eax, [esp+4]
	_asm mov edx, [esp+8]

	_asm mov ecx, eax
	_asm and eax, 0aaaaaaaah
	_asm and ecx, 055555555h
	_asm shr eax, 1
	_asm add eax, ecx			/* 16 groups of 2 bits, count=0,1,2 */
	_asm mov ecx, eax
	_asm and eax, 0cccccccch
	_asm and ecx, 033333333h
	_asm shr eax, 2
	_asm add ecx, eax			/* 8 groups of 4 bits, count=0...4 */

	_asm mov eax, edx
	_asm and eax, 0aaaaaaaah
	_asm and edx, 055555555h
	_asm shr eax, 1
	_asm add eax, edx			/* 16 groups of 2 bits, count=0,1,2 */
	_asm mov edx, eax
	_asm and eax, 0cccccccch
	_asm and edx, 033333333h
	_asm shr eax, 2
	_asm add eax, edx			/* 8 groups of 4 bits, count=0...4 */

	_asm add eax, ecx			/* 8 groups of 4 bits, count=0...8 */
	_asm mov edx, eax
	_asm and eax, 0f0f0f0f0h
	_asm and edx, 00f0f0f0fh
	_asm shr eax, 4
	_asm add eax, edx			/* 4 groups of 8 bits, count=0...16 */
	_asm add al, ah
	_asm mov dl, al
	_asm shr eax, 16
	_asm add al, ah
	_asm xor ah, ah
	_asm add al, dl
	_asm ret
	}




/** Read a 64 bit word with acquire semantics

	@param	a	Address of word to be read - must be a multiple of 8
	@return		The value read
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_load_acq64(const volatile TAny* /*a*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]
	_asm mov eax, 0badbeefh
	_asm mov edx, eax
	_asm mov ebx, eax
	_asm mov ecx, eax
	_asm __LOCK__ cmpxchg8b [edi]
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** Write a 64 bit word with release semantics

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The value written
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_store_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]
	_asm mov ebx, [esp+16]
	_asm mov ecx, [esp+20]
	_asm mov eax, [edi]
	_asm mov edx, [edi+4]
	_asm retry:
	_asm __LOCK__ cmpxchg8b [edi]
	_asm jne short retry
	_asm mov eax, ebx
	_asm mov edx, ecx
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** Write a 64 bit word with full barrier semantics

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The value written
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_store_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_store_rel64
	}


/** Write a 64 bit word to memory and return the original value of the memory.
	Relaxed ordering.

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_swp_rlx64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_swp_ord64
	}


/** Write a 64 bit word to memory and return the original value of the memory.
	Acquire semantics.

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_swp_acq64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_swp_ord64
	}


/** Write a 64 bit word to memory and return the original value of the memory.
	Release semantics.

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_swp_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_swp_ord64
	}


/** Write a 64 bit word to memory and return the original value of the memory.
	Full barrier semantics.

	@param	a	Address of word to be written - must be a multiple of 8
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_swp_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]
	_asm mov ebx, [esp+16]
	_asm mov ecx, [esp+20]
	_asm mov eax, [edi]
	_asm mov edx, [edi+4]
	_asm retry:
	_asm __LOCK__ cmpxchg8b [edi]
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit compare and swap, relaxed ordering.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of word to be written - must be a multiple of 8
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__e32_atomic_cas_rlx64(volatile TAny* /*a*/, TUint64* /*q*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_cas_ord64
	}


/** 64 bit compare and swap, acquire semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of word to be written - must be a multiple of 8
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__e32_atomic_cas_acq64(volatile TAny* /*a*/, TUint64* /*q*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_cas_ord64
	}


/** 64 bit compare and swap, release semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of word to be written - must be a multiple of 8
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__e32_atomic_cas_rel64(volatile TAny* /*a*/, TUint64* /*q*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_cas_ord64
	}


/** 64 bit compare and swap, full barrier semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of word to be written - must be a multiple of 8
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__e32_atomic_cas_ord64(volatile TAny* /*a*/, TUint64* /*q*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm push esi
	_asm mov edi, [esp+16]				// edi = a
	_asm mov esi, [esp+20]				// esi = q
	_asm mov ebx, [esp+24]				// ecx:ebx = v
	_asm mov ecx, [esp+28]
	_asm mov eax, [esi]					// edx:eax = *q
	_asm mov edx, [esi+4]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==*q) *a=v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short cas_fail
	_asm mov eax, 1
	_asm pop esi
	_asm pop edi
	_asm pop ebx
	_asm ret
	_asm cas_fail:
	_asm mov [esi], eax					// *q = edx:eax
	_asm mov [esi+4], edx
	_asm xor eax, eax
	_asm pop esi
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit atomic add, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_add_rlx64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_add_ord64
	}


/** 64 bit atomic add, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_add_acq64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_add_ord64
	}


/** 64 bit atomic add, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_add_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_add_ord64
	}


/** 64 bit atomic add, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_add_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, eax
	_asm mov ecx, edx
	_asm add ebx, [esp+16]				// ecx:ebx = oldv + v
	_asm adc ecx, [esp+20]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==oldv) *a=oldv+v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit atomic bitwise logical AND, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_and_rlx64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_and_ord64
	}


/** 64 bit atomic bitwise logical AND, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_and_acq64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_and_ord64
	}


/** 64 bit atomic bitwise logical AND, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_and_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_and_ord64
	}


/** 64 bit atomic bitwise logical AND, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_and_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, eax
	_asm mov ecx, edx
	_asm and ebx, [esp+16]				// ecx:ebx = oldv & v
	_asm and ecx, [esp+20]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==oldv) *a=oldv&v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit atomic bitwise logical inclusive OR, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_ior_rlx64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_ior_ord64
	}


/** 64 bit atomic bitwise logical inclusive OR, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_ior_acq64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_ior_ord64
	}


/** 64 bit atomic bitwise logical inclusive OR, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_ior_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_ior_ord64
	}


/** 64 bit atomic bitwise logical inclusive OR, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_ior_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, eax
	_asm mov ecx, edx
	_asm or ebx, [esp+16]				// ecx:ebx = oldv | v
	_asm or ecx, [esp+20]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==oldv) *a=oldv|v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit atomic bitwise logical exclusive OR, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_xor_rlx64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_xor_ord64
	}


/** 64 bit atomic bitwise logical exclusive OR, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_xor_acq64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_xor_ord64
	}


/** 64 bit atomic bitwise logical exclusive OR, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_xor_rel64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_xor_ord64
	}


/** 64 bit atomic bitwise logical exclusive OR, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_xor_ord64(volatile TAny* /*a*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, eax
	_asm mov ecx, edx
	_asm xor ebx, [esp+16]				// ecx:ebx = oldv ^ v
	_asm xor ecx, [esp+20]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==oldv) *a=oldv^v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit atomic bitwise universal function, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_axo_rlx64(volatile TAny* /*a*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_axo_ord64
	}


/** 64 bit atomic bitwise universal function, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_axo_acq64(volatile TAny* /*a*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_axo_ord64
	}


/** 64 bit atomic bitwise universal function, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_axo_rel64(volatile TAny* /*a*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_axo_ord64
	}


/** 64 bit atomic bitwise universal function, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of word to be updated - must be a multiple of 8
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_axo_ord64(volatile TAny* /*a*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, eax
	_asm mov ecx, edx
	_asm and ebx, [esp+16]				// ecx:ebx = oldv & u
	_asm and ecx, [esp+20]
	_asm xor ebx, [esp+24]				// ecx:ebx = (oldv & u) ^ v
	_asm xor ecx, [esp+28]
	_asm __LOCK__ cmpxchg8b [edi]		// if (*a==oldv) *a=(oldv&u)^v, ZF=1 else edx:eax=*a, ZF=0
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit threshold and add, unsigned, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_tau_rlx64(volatile TAny* /*a*/, TUint64 /*t*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_tau_ord64
	}


/** 64 bit threshold and add, unsigned, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_tau_acq64(volatile TAny* /*a*/, TUint64 /*t*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_tau_ord64
	}


/** 64 bit threshold and add, unsigned, release semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_tau_rel64(volatile TAny* /*a*/, TUint64 /*t*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm jmp __e32_atomic_tau_ord64
	}


/** 64 bit threshold and add, unsigned, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TUint64	__e32_atomic_tau_ord64(volatile TAny* /*a*/, TUint64 /*t*/, TUint64 /*u*/, TUint64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, edx
	_asm cmp eax, [esp+16]				// eax - t.low, CF=borrow
	_asm sbb ebx, [esp+20]				// CF = borrow from (oldv - t)
	_asm jnc short use_u				// no borrow means oldv>=t so use u
	_asm mov ebx, [esp+32]				// ecx:ebx = v
	_asm mov ecx, [esp+36]
	_asm jmp short use_v
	_asm use_u:
	_asm mov ebx, [esp+24]				// ecx:ebx = u
	_asm mov ecx, [esp+28]
	_asm use_v:
	_asm add ebx, eax					// ecx:ebx = oldv + u or v
	_asm adc ecx, edx
	_asm __LOCK__ cmpxchg8b [edi]
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}


/** 64 bit threshold and add, signed, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TInt64	__e32_atomic_tas_rlx64(volatile TAny* /*a*/, TInt64 /*t*/, TInt64 /*u*/, TInt64 /*v*/)
	{
	_asm jmp __e32_atomic_tas_ord64
	}


/** 64 bit threshold and add, signed, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TInt64	__e32_atomic_tas_acq64(volatile TAny* /*a*/, TInt64 /*t*/, TInt64 /*u*/, TInt64 /*v*/)
	{
	_asm jmp __e32_atomic_tas_ord64
	}


/** 64 bit threshold and add, signed, release semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TInt64	__e32_atomic_tas_rel64(volatile TAny* /*a*/, TInt64 /*t*/, TInt64 /*u*/, TInt64 /*v*/)
	{
	_asm jmp __e32_atomic_tas_ord64
	}


/** 64 bit threshold and add, signed, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ TInt64	__e32_atomic_tas_ord64(volatile TAny* /*a*/, TInt64 /*t*/, TInt64 /*u*/, TInt64 /*v*/)
	{
	_asm push ebx
	_asm push edi
	_asm mov edi, [esp+12]				// edi = a
	_asm mov eax, [edi]					// edx:eax = oldv
	_asm mov edx, [edi+4]
	_asm retry:
	_asm mov ebx, edx
	_asm cmp eax, [esp+16]				// eax - t.low, CF=borrow
	_asm sbb ebx, [esp+20]				// SF=sign, OF=overflow from (oldv - t)
	_asm jge short use_u				// SF==OF (GE condition) means oldv>=t so use u
	_asm mov ebx, [esp+32]				// ecx:ebx = v
	_asm mov ecx, [esp+36]
	_asm jmp short use_v
	_asm use_u:
	_asm mov ebx, [esp+24]				// ecx:ebx = u
	_asm mov ecx, [esp+28]
	_asm use_v:
	_asm add ebx, eax					// ecx:ebx = oldv + u or v
	_asm adc ecx, edx
	_asm __LOCK__ cmpxchg8b [edi]
	_asm jne short retry
	_asm pop edi
	_asm pop ebx
	_asm ret
	}

} // extern "C"
