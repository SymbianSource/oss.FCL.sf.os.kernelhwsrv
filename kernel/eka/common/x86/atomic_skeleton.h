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
// e32\common\x86\atomic_skeleton.h
// 
//

/**
 Read an 8/16/32 bit quantity with acquire semantics
 
 @param	a	Address of data to be read - must be naturally aligned
 @return		The value read
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_load_acq)(const volatile TAny* /*a*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
#ifdef __BARRIERS_NEEDED__
	asm("lock add dword ptr [esp], 0 ");
#endif
	asm("ret ");
	}


/** Write an 8/16/32 bit quantity with release semantics

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The value written
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_store_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __D_REG__ ", [esp+8] ");
	asm("mov " __A_REG__ ", " __D_REG__ );
	asm(__LOCK__ "xchg [ecx], " __D_REG__ );
	asm("ret ");
	}


/** Write an 8/16/32 bit quantity with full barrier semantics

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The value written
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_store_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_store_rel);
	}


/** Write an 8/16/32 bit quantity to memory and return the original value of the memory.
	Relaxed ordering.

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_swp_rlx)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_swp_ord);
	}


/** Write an 8/16/32 bit quantity to memory and return the original value of the memory.
	Acquire semantics.

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_swp_acq)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_swp_ord);
	}


/** Write an 8/16/32 bit quantity to memory and return the original value of the memory.
	Release semantics.

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_swp_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_swp_ord);
	}


/** Write an 8/16/32 bit quantity to memory and return the original value of the memory.
	Full barrier semantics.

	@param	a	Address of data to be written - must be naturally aligned
	@param	v	The value to be written
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_swp_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [esp+8] ");
	asm(__LOCK__ "xchg [ecx], " __A_REG__ );
	asm("ret ");
	}


/** 8/16/32 bit compare and swap, relaxed ordering.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of data to be written - must be naturally aligned
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__fname__(__e32_atomic_cas_rlx)(volatile TAny* /*a*/, __TUintX__* /*q*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_cas_ord);
	}


/** 8/16/32 bit compare and swap, acquire semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of data to be written - must be naturally aligned
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__fname__(__e32_atomic_cas_acq)(volatile TAny* /*a*/, __TUintX__* /*q*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_cas_ord);
	}


/** 8/16/32 bit compare and swap, release semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of data to be written - must be naturally aligned
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__fname__(__e32_atomic_cas_rel)(volatile TAny* /*a*/, __TUintX__* /*q*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_cas_ord);
	}


/** 8/16/32 bit compare and swap, full barrier semantics.

	Atomically performs the following operation:
		if (*a == *q)	{ *a = v; return TRUE; }
		else			{ *q = *a; return FALSE; }

	@param	a	Address of data to be written - must be naturally aligned
	@param	q	Address of location containing expected value
	@param	v	The new value to be written if the old value is as expected
	@return		TRUE if *a was updated, FALSE otherwise
*/
EXPORT_C __NAKED__ TBool		__fname__(__e32_atomic_cas_ord)(volatile TAny* /*a*/, __TUintX__* /*q*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov eax, [esp+8] ");
	asm("mov " __D_REG__ ", [esp+12] ");
	asm("mov " __A_REG__ ", [eax] ");
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 2f ");
	asm("mov eax, 1 ");
	asm("ret ");
	asm("2: ");
	asm("mov edx, [esp+8] ");
	asm("mov [edx], " __A_REG__ );
	asm("xor eax, eax ");
	asm("ret ");
	}


/** 8/16/32 bit atomic add, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_add_rlx)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_add_ord);
	}


/** 8/16/32 bit atomic add, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_add_acq)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_add_ord);
	}


/** 8/16/32 bit atomic add, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_add_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_add_ord);
	}


/** 8/16/32 bit atomic add, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv + v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be added
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_add_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [esp+8] ");
	asm(__LOCK__ "xadd [ecx], " __A_REG__ );
	asm("ret ");
	}


/** 8/16/32 bit atomic bitwise logical AND, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_and_rlx)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_and_ord);
	}


/** 8/16/32 bit atomic bitwise logical AND, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_and_acq)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_and_ord);
	}


/** 8/16/32 bit atomic bitwise logical AND, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_and_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_and_ord);
	}


/** 8/16/32 bit atomic bitwise logical AND, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv & v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ANDed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_and_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+8] ");
	asm("and " __D_REG__ ", " __A_REG__ );
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


/** 8/16/32 bit atomic bitwise logical inclusive OR, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_ior_rlx)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_ior_ord);
	}


/** 8/16/32 bit atomic bitwise logical inclusive OR, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_ior_acq)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_ior_ord);
	}


/** 8/16/32 bit atomic bitwise logical inclusive OR, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_ior_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_ior_ord);
	}


/** 8/16/32 bit atomic bitwise logical inclusive OR, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv | v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be ORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_ior_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+8] ");
	asm("or " __D_REG__ ", " __A_REG__ );
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


/** 8/16/32 bit atomic bitwise logical exclusive OR, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_xor_rlx)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_xor_ord);
	}


/** 8/16/32 bit atomic bitwise logical exclusive OR, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_xor_acq)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_xor_ord);
	}


/** 8/16/32 bit atomic bitwise logical exclusive OR, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_xor_rel)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_xor_ord);
	}


/** 8/16/32 bit atomic bitwise logical exclusive OR, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = oldv ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	v	The value to be XORed with *a
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_xor_ord)(volatile TAny* /*a*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+8] ");
	asm("xor " __D_REG__ ", " __A_REG__ );
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


/** 8/16/32 bit atomic bitwise universal function, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_axo_rlx)(volatile TAny* /*a*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_axo_ord);
	}


/** 8/16/32 bit atomic bitwise universal function, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_axo_acq)(volatile TAny* /*a*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_axo_ord);
	}


/** 8/16/32 bit atomic bitwise universal function, release semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_axo_rel)(volatile TAny* /*a*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_axo_ord);
	}


/** 8/16/32 bit atomic bitwise universal function, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; *a = (oldv & u) ^ v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	u	The value to be ANDed with *a
	@param	v	The value to be XORed with (*a&u)
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_axo_ord)(volatile TAny* /*a*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+8] ");
	asm("and " __D_REG__ ", " __A_REG__ );
	asm("xor " __D_REG__ ", [esp+12] ");
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


/** 8/16/32 bit threshold and add, unsigned, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_tau_rlx)(volatile TAny* /*a*/, __TUintX__ /*t*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_tau_ord);
	}


/** 8/16/32 bit threshold and add, unsigned, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_tau_acq)(volatile TAny* /*a*/, __TUintX__ /*t*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_tau_ord);
	}


/** 8/16/32 bit threshold and add, unsigned, release semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_tau_rel)(volatile TAny* /*a*/, __TUintX__ /*t*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	__redir__(__e32_atomic_tau_ord);
	}


/** 8/16/32 bit threshold and add, unsigned, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (unsigned compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TUintX__	__fname__(__e32_atomic_tau_ord)(volatile TAny* /*a*/, __TUintX__ /*t*/, __TUintX__ /*u*/, __TUintX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+12] ");
	asm("cmp " __A_REG__ ", [esp+8] ");
	asm("jae short 2f ");
	asm("mov " __D_REG__ ", [esp+16] ");
	asm("2: ");
	asm("add " __D_REG__ ", " __A_REG__ );
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


/** 8/16/32 bit threshold and add, signed, relaxed ordering.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TIntX__	__fname__(__e32_atomic_tas_rlx)(volatile TAny* /*a*/, __TIntX__ /*t*/, __TIntX__ /*u*/, __TIntX__ /*v*/)
	{
	__redir__(__e32_atomic_tas_ord);
	}


/** 8/16/32 bit threshold and add, signed, acquire semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TIntX__	__fname__(__e32_atomic_tas_acq)(volatile TAny* /*a*/, __TIntX__ /*t*/, __TIntX__ /*u*/, __TIntX__ /*v*/)
	{
	__redir__(__e32_atomic_tas_ord);
	}


/** 8/16/32 bit threshold and add, signed, release semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TIntX__	__fname__(__e32_atomic_tas_rel)(volatile TAny* /*a*/, __TIntX__ /*t*/, __TIntX__ /*u*/, __TIntX__ /*v*/)
	{
	__redir__(__e32_atomic_tas_ord);
	}


/** 8/16/32 bit threshold and add, signed, full barrier semantics.

	Atomically performs the following operation:
		oldv = *a; if (oldv>=t) *a=oldv+u else *a=oldv+v; return oldv;

	@param	a	Address of data to be updated - must be naturally aligned
	@param	t	The threshold to compare *a to (signed compare)
	@param	u	The value to be added to *a if it is originally >= t
	@param	u	The value to be added to *a if it is originally < t
	@return		The original value of *a
*/
EXPORT_C __NAKED__ __TIntX__	__fname__(__e32_atomic_tas_ord)(volatile TAny* /*a*/, __TIntX__ /*t*/, __TIntX__ /*u*/, __TIntX__ /*v*/)
	{
	asm("mov ecx, [esp+4] ");
	asm("mov " __A_REG__ ", [ecx] ");
	asm("1: ");
	asm("mov " __D_REG__ ", [esp+12] ");
	asm("cmp " __A_REG__ ", [esp+8] ");
	asm("jge short 2f ");
	asm("mov " __D_REG__ ", [esp+16] ");
	asm("2: ");
	asm("add " __D_REG__ ", " __A_REG__ );
	asm(__LOCK__ "cmpxchg [ecx], " __D_REG__ );
	asm("jne short 1b ");
	asm("ret ");
	}


