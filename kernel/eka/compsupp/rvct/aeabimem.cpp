// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This file is part of drtaeabi.dll.
// 
//

#include <e32cmn.h>


// Call our implementations of memcpy/move/set/clr rather than the ARM supplied
// ones.
// 
// Note that the AEABI switched the order of arg2 and arg3 to save an instruction
// when calling 'memset' from 'memclr'.
//
// Prototypes are:
//   void __aeabi_memset8(TAny* aTrg, unsigned int aLength, TInt aValue);
//   void __aeabi_memset4(TAny* aTrg, unsigned int aLength, TInt aValue);
//   void __aeabi_memset(TAny* aTrg, unsigned int aLength, TInt aValue);

extern "C" __asm void make_mem_functions_call_euser_versions()
	{
	CODE32

	IMPORT memset  [DYNAMIC]
	IMPORT memclr  [DYNAMIC]
	IMPORT memcpy  [DYNAMIC]
	IMPORT memmove [DYNAMIC]

	EXPORT __aeabi_memset   [DYNAMIC]
	EXPORT __aeabi_memset4  [DYNAMIC]
	EXPORT __aeabi_memset8  [DYNAMIC]
	EXPORT __aeabi_memclr   [DYNAMIC]
	EXPORT __aeabi_memclr4  [DYNAMIC]
	EXPORT __aeabi_memclr8  [DYNAMIC]
	EXPORT __aeabi_memcpy   [DYNAMIC]
	EXPORT __aeabi_memcpy4  [DYNAMIC]
	EXPORT __aeabi_memcpy8  [DYNAMIC]
	EXPORT __aeabi_memmove  [DYNAMIC]
	EXPORT __aeabi_memmove4 [DYNAMIC]
	EXPORT __aeabi_memmove8 [DYNAMIC]

__aeabi_memset
__aeabi_memset4
__aeabi_memset8
	mov r3, r1
	mov r1, r2
	mov r2, r3
	b memset

__aeabi_memclr8
__aeabi_memclr4
__aeabi_memclr
	b memclr

__aeabi_memcpy8
__aeabi_memcpy4
__aeabi_memcpy
	b memcpy

__aeabi_memmove8
__aeabi_memmove4
__aeabi_memmove
	b memmove
	}

