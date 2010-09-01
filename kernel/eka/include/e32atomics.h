/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* e32/include/e32atomics.h
* 
*
*/



#ifndef __E32ATOMICS_H__
#define __E32ATOMICS_H__
#include <e32def.h>

/**	@file e32atomics.h
	@publishedAll
	@prototype
*/


/*
Versions needed:
	WINS/WINSCW		Use X86 locked operations. Assume Pentium or above CPU (CMPXCHG8B available)
	X86				For Pentium and above use locked operations
					For 486 use locked operations for 8, 16, 32 bit. For 64 bit must disable interrupts.
					NOTE: 486 not supported at the moment
	ARMv4/ARMv5		Must disable interrupts.
	ARMv6			LDREX/STREX for 8, 16, 32 bit. For 64 bit must disable interrupts (maybe).
	ARMv6K/ARMv7	LDREXB/LDREXH/LDREX/LDREXD
*/

#ifdef __cplusplus
extern "C" {
#endif

IMPORT_C void		__e32_memory_barrier();												/* Barrier guaranteeing ordering of memory accesses */
IMPORT_C void		__e32_io_completion_barrier();										/* Barrier guaranteeing ordering and completion of memory accesses */

/* Atomic operations on 8 bit quantities */
IMPORT_C TUint8		__e32_atomic_load_acq8(const volatile TAny* a);						/* read 8 bit acquire semantics */
IMPORT_C TUint8		__e32_atomic_store_rel8(volatile TAny* a, TUint8 v);				/* write 8 bit, return v, release semantics */
IMPORT_C TUint8		__e32_atomic_store_ord8(volatile TAny* a, TUint8 v);				/* write 8 bit, return v, full fence */
IMPORT_C TUint8		__e32_atomic_swp_rlx8(volatile TAny* a, TUint8 v);					/* write 8 bit, return original, relaxed */
IMPORT_C TUint8		__e32_atomic_swp_acq8(volatile TAny* a, TUint8 v);					/* write 8 bit, return original, acquire */
IMPORT_C TUint8		__e32_atomic_swp_rel8(volatile TAny* a, TUint8 v);					/* write 8 bit, return original, release */
IMPORT_C TUint8		__e32_atomic_swp_ord8(volatile TAny* a, TUint8 v);					/* write 8 bit, return original, full fence */
IMPORT_C TBool		__e32_atomic_cas_rlx8(volatile TAny* a, TUint8* q, TUint8 v);		/* if (*a==*q) {*a=v; return TRUE;} else {*q=*a; return FALSE;} */
IMPORT_C TBool		__e32_atomic_cas_acq8(volatile TAny* a, TUint8* q, TUint8 v);
IMPORT_C TBool		__e32_atomic_cas_rel8(volatile TAny* a, TUint8* q, TUint8 v);
IMPORT_C TBool		__e32_atomic_cas_ord8(volatile TAny* a, TUint8* q, TUint8 v);
IMPORT_C TUint8		__e32_atomic_add_rlx8(volatile TAny* a, TUint8 v);					/* *a += v; return original *a; */
IMPORT_C TUint8		__e32_atomic_add_acq8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_add_rel8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_add_ord8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_and_rlx8(volatile TAny* a, TUint8 v);					/* *a &= v; return original *a; */
IMPORT_C TUint8		__e32_atomic_and_acq8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_and_rel8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_and_ord8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_ior_rlx8(volatile TAny* a, TUint8 v);					/* *a |= v; return original *a; */
IMPORT_C TUint8		__e32_atomic_ior_acq8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_ior_rel8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_ior_ord8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_xor_rlx8(volatile TAny* a, TUint8 v);					/* *a ^= v; return original *a; */
IMPORT_C TUint8		__e32_atomic_xor_acq8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_xor_rel8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_xor_ord8(volatile TAny* a, TUint8 v);
IMPORT_C TUint8		__e32_atomic_axo_rlx8(volatile TAny* a, TUint8 u, TUint8 v);		/* *a = (*a & u) ^ v; return original *a; */
IMPORT_C TUint8		__e32_atomic_axo_acq8(volatile TAny* a, TUint8 u, TUint8 v);
IMPORT_C TUint8		__e32_atomic_axo_rel8(volatile TAny* a, TUint8 u, TUint8 v);
IMPORT_C TUint8		__e32_atomic_axo_ord8(volatile TAny* a, TUint8 u, TUint8 v);
IMPORT_C TUint8		__e32_atomic_tau_rlx8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TUint8		__e32_atomic_tau_acq8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v);
IMPORT_C TUint8		__e32_atomic_tau_rel8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v);
IMPORT_C TUint8		__e32_atomic_tau_ord8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v);
IMPORT_C TInt8		__e32_atomic_tas_rlx8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TInt8		__e32_atomic_tas_acq8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v);
IMPORT_C TInt8		__e32_atomic_tas_rel8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v);
IMPORT_C TInt8		__e32_atomic_tas_ord8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v);

/* Atomic operations on 16 bit quantities */
IMPORT_C TUint16	__e32_atomic_load_acq16(const volatile TAny* a);					/* read 16 bit acquire semantics */
IMPORT_C TUint16	__e32_atomic_store_rel16(volatile TAny* a, TUint16 v);				/* write 16 bit, return v, release semantics */
IMPORT_C TUint16	__e32_atomic_store_ord16(volatile TAny* a, TUint16 v);				/* write 16 bit, return v, full fence */
IMPORT_C TUint16	__e32_atomic_swp_rlx16(volatile TAny* a, TUint16 v);				/* write 16 bit, return original, relaxed */
IMPORT_C TUint16	__e32_atomic_swp_acq16(volatile TAny* a, TUint16 v);				/* write 16 bit, return original, acquire */
IMPORT_C TUint16	__e32_atomic_swp_rel16(volatile TAny* a, TUint16 v);				/* write 16 bit, return original, release */
IMPORT_C TUint16	__e32_atomic_swp_ord16(volatile TAny* a, TUint16 v);				/* write 16 bit, return original, full fence */
IMPORT_C TBool		__e32_atomic_cas_rlx16(volatile TAny* a, TUint16* q, TUint16 v);	/* if (*a==*q) {*a=v; return TRUE;} else {*q=*a; return FALSE;} */
IMPORT_C TBool		__e32_atomic_cas_acq16(volatile TAny* a, TUint16* q, TUint16 v);
IMPORT_C TBool		__e32_atomic_cas_rel16(volatile TAny* a, TUint16* q, TUint16 v);
IMPORT_C TBool		__e32_atomic_cas_ord16(volatile TAny* a, TUint16* q, TUint16 v);
IMPORT_C TUint16	__e32_atomic_add_rlx16(volatile TAny* a, TUint16 v);				/* *a += v; return original *a; */
IMPORT_C TUint16	__e32_atomic_add_acq16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_add_rel16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_add_ord16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_and_rlx16(volatile TAny* a, TUint16 v);				/* *a &= v; return original *a; */
IMPORT_C TUint16	__e32_atomic_and_acq16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_and_rel16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_and_ord16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_ior_rlx16(volatile TAny* a, TUint16 v);				/* *a |= v; return original *a; */
IMPORT_C TUint16	__e32_atomic_ior_acq16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_ior_rel16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_ior_ord16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_xor_rlx16(volatile TAny* a, TUint16 v);				/* *a ^= v; return original *a; */
IMPORT_C TUint16	__e32_atomic_xor_acq16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_xor_rel16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_xor_ord16(volatile TAny* a, TUint16 v);
IMPORT_C TUint16	__e32_atomic_axo_rlx16(volatile TAny* a, TUint16 u, TUint16 v);		/* *a = (*a & u) ^ v; return original *a; */
IMPORT_C TUint16	__e32_atomic_axo_acq16(volatile TAny* a, TUint16 u, TUint16 v);
IMPORT_C TUint16	__e32_atomic_axo_rel16(volatile TAny* a, TUint16 u, TUint16 v);
IMPORT_C TUint16	__e32_atomic_axo_ord16(volatile TAny* a, TUint16 u, TUint16 v);
IMPORT_C TUint16	__e32_atomic_tau_rlx16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TUint16	__e32_atomic_tau_acq16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v);
IMPORT_C TUint16	__e32_atomic_tau_rel16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v);
IMPORT_C TUint16	__e32_atomic_tau_ord16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v);
IMPORT_C TInt16		__e32_atomic_tas_rlx16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TInt16		__e32_atomic_tas_acq16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v);
IMPORT_C TInt16		__e32_atomic_tas_rel16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v);
IMPORT_C TInt16		__e32_atomic_tas_ord16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v);

/* Atomic operations on 32 bit quantities */
IMPORT_C TUint32	__e32_atomic_load_acq32(const volatile TAny* a);					/* read 32 bit acquire semantics */
IMPORT_C TUint32	__e32_atomic_store_rel32(volatile TAny* a, TUint32 v);				/* write 32 bit, return v, release semantics */
IMPORT_C TUint32	__e32_atomic_store_ord32(volatile TAny* a, TUint32 v);				/* write 32 bit, return v, full fence */
IMPORT_C TUint32	__e32_atomic_swp_rlx32(volatile TAny* a, TUint32 v);				/* write 32 bit, return original, relaxed */
IMPORT_C TUint32	__e32_atomic_swp_acq32(volatile TAny* a, TUint32 v);				/* write 32 bit, return original, acquire */
IMPORT_C TUint32	__e32_atomic_swp_rel32(volatile TAny* a, TUint32 v);				/* write 32 bit, return original, release */
IMPORT_C TUint32	__e32_atomic_swp_ord32(volatile TAny* a, TUint32 v);				/* write 32 bit, return original, full fence */
IMPORT_C TBool		__e32_atomic_cas_rlx32(volatile TAny* a, TUint32* q, TUint32 v);	/* if (*a==*q) {*a=v; return TRUE;} else {*q=*a; return FALSE;} */
IMPORT_C TBool		__e32_atomic_cas_acq32(volatile TAny* a, TUint32* q, TUint32 v);
IMPORT_C TBool		__e32_atomic_cas_rel32(volatile TAny* a, TUint32* q, TUint32 v);
IMPORT_C TBool		__e32_atomic_cas_ord32(volatile TAny* a, TUint32* q, TUint32 v);
IMPORT_C TUint32	__e32_atomic_add_rlx32(volatile TAny* a, TUint32 v);				/* *a += v; return original *a; */
IMPORT_C TUint32	__e32_atomic_add_acq32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_add_rel32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_add_ord32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_and_rlx32(volatile TAny* a, TUint32 v);				/* *a &= v; return original *a; */
IMPORT_C TUint32	__e32_atomic_and_acq32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_and_rel32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_and_ord32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_ior_rlx32(volatile TAny* a, TUint32 v);				/* *a |= v; return original *a; */
IMPORT_C TUint32	__e32_atomic_ior_acq32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_ior_rel32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_ior_ord32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_xor_rlx32(volatile TAny* a, TUint32 v);				/* *a ^= v; return original *a; */
IMPORT_C TUint32	__e32_atomic_xor_acq32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_xor_rel32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_xor_ord32(volatile TAny* a, TUint32 v);
IMPORT_C TUint32	__e32_atomic_axo_rlx32(volatile TAny* a, TUint32 u, TUint32 v);		/* *a = (*a & u) ^ v; return original *a; */
IMPORT_C TUint32	__e32_atomic_axo_acq32(volatile TAny* a, TUint32 u, TUint32 v);
IMPORT_C TUint32	__e32_atomic_axo_rel32(volatile TAny* a, TUint32 u, TUint32 v);
IMPORT_C TUint32	__e32_atomic_axo_ord32(volatile TAny* a, TUint32 u, TUint32 v);
IMPORT_C TUint32	__e32_atomic_tau_rlx32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TUint32	__e32_atomic_tau_acq32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v);
IMPORT_C TUint32	__e32_atomic_tau_rel32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v);
IMPORT_C TUint32	__e32_atomic_tau_ord32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v);
IMPORT_C TInt32		__e32_atomic_tas_rlx32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TInt32		__e32_atomic_tas_acq32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v);
IMPORT_C TInt32		__e32_atomic_tas_rel32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v);
IMPORT_C TInt32		__e32_atomic_tas_ord32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v);

/* Atomic operations on 64 bit quantities */
IMPORT_C TUint64	__e32_atomic_load_acq64(const volatile TAny* a);					/* read 64 bit acquire semantics */
IMPORT_C TUint64	__e32_atomic_store_rel64(volatile TAny* a, TUint64 v);				/* write 64 bit, return v, release semantics */
IMPORT_C TUint64	__e32_atomic_store_ord64(volatile TAny* a, TUint64 v);				/* write 64 bit, return v, full fence */
IMPORT_C TUint64	__e32_atomic_swp_rlx64(volatile TAny* a, TUint64 v);				/* write 64 bit, return original, relaxed */
IMPORT_C TUint64	__e32_atomic_swp_acq64(volatile TAny* a, TUint64 v);				/* write 64 bit, return original, acquire */
IMPORT_C TUint64	__e32_atomic_swp_rel64(volatile TAny* a, TUint64 v);				/* write 64 bit, return original, release */
IMPORT_C TUint64	__e32_atomic_swp_ord64(volatile TAny* a, TUint64 v);				/* write 64 bit, return original, full fence */
IMPORT_C TBool		__e32_atomic_cas_rlx64(volatile TAny* a, TUint64* q, TUint64 v);	/* if (*a==*q) {*a=v; return TRUE;} else {*q=*a; return FALSE;} */
IMPORT_C TBool		__e32_atomic_cas_acq64(volatile TAny* a, TUint64* q, TUint64 v);
IMPORT_C TBool		__e32_atomic_cas_rel64(volatile TAny* a, TUint64* q, TUint64 v);
IMPORT_C TBool		__e32_atomic_cas_ord64(volatile TAny* a, TUint64* q, TUint64 v);
IMPORT_C TUint64	__e32_atomic_add_rlx64(volatile TAny* a, TUint64 v);				/* *a += v; return original *a; */
IMPORT_C TUint64	__e32_atomic_add_acq64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_add_rel64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_add_ord64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_and_rlx64(volatile TAny* a, TUint64 v);				/* *a &= v; return original *a; */
IMPORT_C TUint64	__e32_atomic_and_acq64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_and_rel64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_and_ord64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_ior_rlx64(volatile TAny* a, TUint64 v);				/* *a |= v; return original *a; */
IMPORT_C TUint64	__e32_atomic_ior_acq64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_ior_rel64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_ior_ord64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_xor_rlx64(volatile TAny* a, TUint64 v);				/* *a ^= v; return original *a; */
IMPORT_C TUint64	__e32_atomic_xor_acq64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_xor_rel64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_xor_ord64(volatile TAny* a, TUint64 v);
IMPORT_C TUint64	__e32_atomic_axo_rlx64(volatile TAny* a, TUint64 u, TUint64 v);		/* *a = (*a & u) ^ v; return original *a; */
IMPORT_C TUint64	__e32_atomic_axo_acq64(volatile TAny* a, TUint64 u, TUint64 v);
IMPORT_C TUint64	__e32_atomic_axo_rel64(volatile TAny* a, TUint64 u, TUint64 v);
IMPORT_C TUint64	__e32_atomic_axo_ord64(volatile TAny* a, TUint64 u, TUint64 v);
IMPORT_C TUint64	__e32_atomic_tau_rlx64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TUint64	__e32_atomic_tau_acq64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v);
IMPORT_C TUint64	__e32_atomic_tau_rel64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v);
IMPORT_C TUint64	__e32_atomic_tau_ord64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v);
IMPORT_C TInt64		__e32_atomic_tas_rlx64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v);	/* if (*a>=t) *a+=u else *a+=v; return original *a; */
IMPORT_C TInt64		__e32_atomic_tas_acq64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v);
IMPORT_C TInt64		__e32_atomic_tas_rel64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v);
IMPORT_C TInt64		__e32_atomic_tas_ord64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v);

/*	Atomic operations on pointers
	These are implemented as macro definitions over the 32 or 64 bit operations
*/
/*	IMPORT_C TAny*		__e32_atomic_load_acq_ptr(const volatile TAny* a);												*/
#define	__e32_atomic_load_acq_ptr(a)		((TAny*)__e32_atomic_load_acq32(a))
/*	IMPORT_C TAny*		__e32_atomic_store_rel_ptr(volatile TAny* a, const volatile TAny* v);							*/
#define	__e32_atomic_store_rel_ptr(a,v)		((TAny*)__e32_atomic_store_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_store_ord_ptr(volatile TAny* a, const volatile TAny* v);							*/
#define	__e32_atomic_store_ord_ptr(a,v)		((TAny*)__e32_atomic_store_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_swp_rlx_ptr(volatile TAny* a, const volatile TAny* v);								*/
#define	__e32_atomic_swp_rlx_ptr(a,v)		((TAny*)__e32_atomic_swp_rlx32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_swp_acq_ptr(volatile TAny* a, const volatile TAny* v);								*/
#define	__e32_atomic_swp_acq_ptr(a,v)		((TAny*)__e32_atomic_swp_acq32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_swp_rel_ptr(volatile TAny* a, const volatile TAny* v);								*/
#define	__e32_atomic_swp_rel_ptr(a,v)		((TAny*)__e32_atomic_swp_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_swp_ord_ptr(volatile TAny* a, const volatile TAny* v);								*/
#define	__e32_atomic_swp_ord_ptr(a,v)		((TAny*)__e32_atomic_swp_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TBool		__e32_atomic_cas_rlx_ptr(volatile TAny* a, const volatile TAny** q, const volatile TAny* v);	*/
#define	__e32_atomic_cas_rlx_ptr(a,q,v)		(__e32_atomic_cas_rlx32(a,(T_UintPtr*)(q),(T_UintPtr)(v)))
/*	IMPORT_C TBool		__e32_atomic_cas_acq_ptr(volatile TAny* a, const volatile TAny** q, const volatile TAny* v);	*/
#define	__e32_atomic_cas_acq_ptr(a,q,v)		(__e32_atomic_cas_acq32(a,(T_UintPtr*)(q),(T_UintPtr)(v)))
/*	IMPORT_C TBool		__e32_atomic_cas_rel_ptr(volatile TAny* a, const volatile TAny** q, const volatile TAny* v);	*/
#define	__e32_atomic_cas_rel_ptr(a,q,v)		(__e32_atomic_cas_rel32(a,(T_UintPtr*)(q),(T_UintPtr)(v)))
/*	IMPORT_C TBool		__e32_atomic_cas_ord_ptr(volatile TAny* a, const volatile TAny** q, const volatile TAny* v);	*/
#define	__e32_atomic_cas_ord_ptr(a,q,v)		(__e32_atomic_cas_ord32(a,(T_UintPtr*)(q),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_add_rlx_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_add_rlx_ptr(a,v)		((TAny*)__e32_atomic_add_rlx32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_add_acq_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_add_acq_ptr(a,v)		((TAny*)__e32_atomic_add_acq32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_add_rel_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_add_rel_ptr(a,v)		((TAny*)__e32_atomic_add_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_add_ord_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_add_ord_ptr(a,v)		((TAny*)__e32_atomic_add_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_and_rlx_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_and_rlx_ptr(a,v)		((TAny*)__e32_atomic_and_rlx32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_and_acq_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_and_acq_ptr(a,v)		((TAny*)__e32_atomic_and_acq32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_and_rel_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_and_rel_ptr(a,v)		((TAny*)__e32_atomic_and_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_and_ord_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_and_ord_ptr(a,v)		((TAny*)__e32_atomic_and_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_ior_rlx_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_ior_rlx_ptr(a,v)		((TAny*)__e32_atomic_ior_rlx32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_ior_acq_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_ior_acq_ptr(a,v)		((TAny*)__e32_atomic_ior_acq32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_ior_rel_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_ior_rel_ptr(a,v)		((TAny*)__e32_atomic_ior_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_ior_ord_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_ior_ord_ptr(a,v)		((TAny*)__e32_atomic_ior_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_xor_rlx_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_xor_rlx_ptr(a,v)		((TAny*)__e32_atomic_xor_rlx32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_xor_acq_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_xor_acq_ptr(a,v)		((TAny*)__e32_atomic_xor_acq32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_xor_rel_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_xor_rel_ptr(a,v)		((TAny*)__e32_atomic_xor_rel32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_xor_ord_ptr(volatile TAny* a, T_UintPtr v);										*/
#define	__e32_atomic_xor_ord_ptr(a,v)		((TAny*)__e32_atomic_xor_ord32(a,(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_axo_rlx_ptr(volatile TAny* a, T_UintPtr u, T_UintPtr v);							*/
#define	__e32_atomic_axo_rlx_ptr(a,u,v)		((TAny*)__e32_atomic_axo_rlx32(a,(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_axo_acq_ptr(volatile TAny* a, T_UintPtr u, T_UintPtr v);							*/
#define	__e32_atomic_axo_acq_ptr(a,u,v)		((TAny*)__e32_atomic_axo_acq32(a,(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_axo_rel_ptr(volatile TAny* a, T_UintPtr u, T_UintPtr v);							*/
#define	__e32_atomic_axo_rel_ptr(a,u,v)		((TAny*)__e32_atomic_axo_rel32(a,(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_axo_ord_ptr(volatile TAny* a, T_UintPtr u, T_UintPtr v);							*/
#define	__e32_atomic_axo_ord_ptr(a,u,v)		((TAny*)__e32_atomic_axo_ord32(a,(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_tau_rlx_ptr(volatile TAny* a, const volatile TAny* t, T_UintPtr u, T_UintPtr v);	*/
#define	__e32_atomic_tau_rlx_ptr(a,t,u,v)	((TAny*)__e32_atomic_tau_rlx32(a,(T_UintPtr)(t),(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_tau_acq_ptr(volatile TAny* a, const volatile TAny* t, T_UintPtr u, T_UintPtr v);	*/
#define	__e32_atomic_tau_acq_ptr(a,t,u,v)	((TAny*)__e32_atomic_tau_acq32(a,(T_UintPtr)(t),(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_tau_rel_ptr(volatile TAny* a, const volatile TAny* t, T_UintPtr u, T_UintPtr v);	*/
#define	__e32_atomic_tau_rel_ptr(a,t,u,v)	((TAny*)__e32_atomic_tau_rel32(a,(T_UintPtr)(t),(T_UintPtr)(u),(T_UintPtr)(v)))
/*	IMPORT_C TAny*		__e32_atomic_tau_ord_ptr(volatile TAny* a, const volatile TAny* t, T_UintPtr u, T_UintPtr v);	*/
#define	__e32_atomic_tau_ord_ptr(a,t,u,v)	((TAny*)__e32_atomic_tau_ord32(a,(T_UintPtr)(t),(T_UintPtr)(u),(T_UintPtr)(v)))

/*	Miscellaneous utility functions
*/
IMPORT_C TInt		__e32_find_ms1_32(TUint32 v);		/* return bit number of most significant 1, -1 if argument zero */
IMPORT_C TInt		__e32_find_ls1_32(TUint32 v);		/* return bit number of least significant 1, -1 if argument zero */
IMPORT_C TInt		__e32_bit_count_32(TUint32 v);		/* return number of bits with value 1 */
IMPORT_C TInt		__e32_find_ms1_64(TUint64 v);		/* return bit number of most significant 1, -1 if argument zero */
IMPORT_C TInt		__e32_find_ls1_64(TUint64 v);		/* return bit number of least significant 1, -1 if argument zero */
IMPORT_C TInt		__e32_bit_count_64(TUint64 v);		/* return number of bits with value 1 */

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif	/* __E32ATOMICS_H__ */
