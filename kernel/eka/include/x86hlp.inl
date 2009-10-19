// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\common\x86\x86hlp.inl
// 
//

#ifdef __GCC32__
#include "x86hlp_gcc.inl"
#else

/**** MSVC helpers ****/

/*static void DivisionByZero()
	{
	_asm int 0;
	}*/

#pragma warning ( disable : 4414 )  // short jump to function converted to near

extern "C" {
__NAKED__ void _allmul()
//
// Multiply two 64 bit integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = arg 1
//		[esp+12], [esp+16] = arg 1
// Return result in edx:eax
// Remove arguments from stack
//
	{
	_asm mov eax, [esp+4]			// eax = low1
	_asm mul dword ptr [esp+16]		// edx:eax = low1*high2
	_asm mov ecx, eax				// keep low 32 bits of product
	_asm mov eax, [esp+8]			// eax = high1
	_asm mul dword ptr [esp+12]		// edx:eax = high1*low2
	_asm add ecx, eax				// accumulate low 32 bits of product
	_asm mov eax, [esp+4]			// eax = low1
	_asm mul dword ptr [esp+12]		// edx:eax = low1*low2
	_asm add edx, ecx				// add cross terms to high 32 bits
	_asm ret 16
	}

void udiv64_divby0()
	{
	_asm int 0						// division by zero exception
	_asm ret
	}

__NAKED__ void UDiv64()
	{
	// unsigned divide edx:eax by edi:esi
	// quotient in ebx:eax, remainder in edi:edx
	// ecx, ebp, esi also modified
	_asm test edi, edi
	_asm jnz short UDiv64a				// branch if divisor >= 2^32
	_asm test esi, esi
//	_ASM_j(z,DivisionByZero)			// if divisor=0, branch to error routine
	_asm jz udiv64_divby0
	_asm mov ebx, eax					// ebx=dividend low
	_asm mov eax, edx					// eax=dividend high
	_asm xor edx, edx					// edx=0
	_asm div esi						// quotient high now in eax
	_asm xchg eax, ebx					// quotient high in ebx, dividend low in eax
	_asm div esi						// quotient now in ebx:eax, remainder in edi:edx
	_asm ret
	UDiv64e:
	_asm xor eax, eax					// set result to 0xFFFFFFFF
	_asm dec eax
	_asm jmp short UDiv64f
	UDiv64a:
	_asm js short UDiv64b				// skip if divisor msb set
	_asm bsr ecx, edi					// ecx=bit number of divisor msb - 32
	_asm inc cl
	_asm push edi						// save divisor high
	_asm push esi						// save divisor low
	_asm shrd esi, edi, cl				// shift divisor right so that msb is bit 31
	_asm mov ebx, edx					// dividend into ebx:ebp
	_asm mov ebp, eax
	_asm shrd eax, edx, cl				// shift dividend right same number of bits
	_asm shr edx, cl
	_asm cmp edx, esi					// check if approx quotient will be 2^32
	_asm jae short UDiv64e				// if so, true result must be 0xFFFFFFFF
	_asm div esi						// approximate quotient now in eax
	UDiv64f:
	_asm mov ecx, eax					// into ecx
	_asm mul edi						// multiply approx. quotient by divisor high
	_asm mov esi, eax					// ls dword into esi, ms into edi
	_asm mov edi, edx
	_asm mov eax, ecx					// approx. quotient into eax
	_asm mul dword ptr [esp]			// multiply approx. quotient by divisor low
	_asm add edx, esi					// edi:edx:eax now equals approx. quotient * divisor
	_asm adc edi, 0
	_asm xor esi, esi
	_asm sub ebp, eax					// subtract dividend - approx. quotient *divisor
	_asm sbb ebx, edx
	_asm sbb esi, edi
	_asm jnc short UDiv64c				// if no borrow, result OK
	_asm dec ecx						// else result is one too big
	_asm add ebp, [esp]					// and add divisor to get correct remainder
	_asm adc ebx, [esp+4]
	UDiv64c:
	_asm mov eax, ecx					// result into ebx:eax, remainder into edi:edx
	_asm mov edi, ebx
	_asm mov edx, ebp
	_asm xor ebx, ebx
	_asm add esp, 8						// remove temporary values from stack
	_asm ret
	UDiv64b:
	_asm mov ebx, 1
	_asm sub eax, esi					// subtract divisor from dividend
	_asm sbb edx, edi
	_asm jnc short UDiv64d				// if no borrow, result=1, remainder in edx:eax
	_asm add eax, esi					// else add back
	_asm adc edx, edi
	_asm dec ebx						// and decrement quotient
	UDiv64d:
	_asm mov edi, edx					// remainder into edi:edx
	_asm mov edx, eax
	_asm mov eax, ebx					// result in ebx:eax
	_asm xor ebx, ebx
	_asm ret
	}

__NAKED__ void _aulldvrm()
//
// Divide two 64 bit unsigned integers, returning a 64 bit result
// and a 64 bit remainder
//
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
//
// Return (dividend / divisor) in edx:eax
// Return (dividend % divisor) in ebx:ecx
//
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm mov eax, [esp+16]
	_asm mov edx, [esp+20]
	_asm mov esi, [esp+24]
	_asm mov edi, [esp+28]
	_asm call UDiv64
	_asm mov ecx, edx
	_asm mov edx, ebx
	_asm mov ebx, edi
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _alldvrm()
//
// Divide two 64 bit signed integers, returning a 64 bit result
// and a 64 bit remainder
//
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
//
// Return (dividend / divisor) in edx:eax
// Return (dividend % divisor) in ebx:ecx
//
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm mov eax, [esp+16]
	_asm mov edx, [esp+20]
	_asm mov esi, [esp+24]
	_asm mov edi, [esp+28]
	_asm test edx, edx
	_asm jns dividend_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	dividend_nonnegative:
	_asm test edi, edi
	_asm jns divisor_nonnegative
	_asm neg edi
	_asm neg esi
	_asm sbb edi, 0
	divisor_nonnegative:
	_asm call UDiv64
	_asm mov ebp, [esp+20]
	_asm mov ecx, edx
	_asm xor ebp, [esp+28]
	_asm mov edx, ebx
	_asm mov ebx, edi
	_asm jns quotient_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	quotient_nonnegative:
	_asm cmp dword ptr [esp+20], 0
	_asm jns rem_nonnegative
	_asm neg ebx
	_asm neg ecx
	_asm sbb ebx, 0
	rem_nonnegative:
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _aulldiv()
//
// Divide two 64 bit unsigned integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm mov eax, [esp+20]
	_asm mov edx, [esp+24]
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]
	_asm call UDiv64
	_asm mov edx, ebx
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _alldiv()
//
// Divide two 64 bit signed integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm mov eax, [esp+20]
	_asm mov edx, [esp+24]
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]
	_asm test edx, edx
	_asm jns dividend_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	dividend_nonnegative:
	_asm test edi, edi
	_asm jns divisor_nonnegative
	_asm neg edi
	_asm neg esi
	_asm sbb edi, 0
	divisor_nonnegative:
	_asm call UDiv64
	_asm mov ecx, [esp+24]
	_asm mov edx, ebx
	_asm xor ecx, [esp+32]
	_asm jns quotient_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	quotient_nonnegative:
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _aullrem()
//
// Divide two 64 bit unsigned integers and return 64 bit remainder
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm mov eax, [esp+20]
	_asm mov edx, [esp+24]
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]
	_asm call UDiv64
	_asm mov eax, edx
	_asm mov edx, edi
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _allrem()
//
// Divide two 64 bit signed integers and return 64 bit remainder
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	_asm push ebp
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm mov eax, [esp+20]
	_asm mov edx, [esp+24]
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]
	_asm test edx, edx
	_asm jns dividend_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	dividend_nonnegative:
	_asm test edi, edi
	_asm jns divisor_nonnegative
	_asm neg edi
	_asm neg esi
	_asm sbb edi, 0
	divisor_nonnegative:
	_asm call UDiv64
	_asm mov eax, edx
	_asm mov edx, edi
	_asm cmp dword ptr [esp+24], 0
	_asm jns rem_nonnegative
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	rem_nonnegative:
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm pop ebp
	_asm ret 16
	}

__NAKED__ void _allshr()
//
// Arithmetic shift right EDX:EAX by CL
//
	{
	_asm cmp cl, 64
	_asm jae asr_count_ge_64
	_asm cmp cl, 32
	_asm jae asr_count_ge_32
	_asm shrd eax, edx, cl
	_asm sar edx, cl
	_asm ret
	asr_count_ge_32:
	_asm sub cl, 32
	_asm mov eax, edx
	_asm cdq
	_asm sar eax, cl
	_asm ret
	asr_count_ge_64:
	_asm sar edx, 32
	_asm mov eax, edx
	_asm ret
	}

__NAKED__ void _allshl()
//
// shift left EDX:EAX by CL
//
	{
	_asm cmp cl, 64
	_asm jae lsl_count_ge_64
	_asm cmp cl, 32
	_asm jae lsl_count_ge_32
	_asm shld edx, eax, cl
	_asm shl eax, cl
	_asm ret
	lsl_count_ge_32:
	_asm sub cl, 32
	_asm mov edx, eax
	_asm xor eax, eax
	_asm shl edx, cl
	_asm ret
	lsl_count_ge_64:
	_asm xor edx, edx
	_asm xor eax, eax
	_asm ret
	}

__NAKED__ void _aullshr()
//
// Logical shift right EDX:EAX by CL
//
	{
	_asm cmp cl, 64
	_asm jae lsr_count_ge_64
	_asm cmp cl, 32
	_asm jae lsr_count_ge_32
	_asm shrd eax, edx, cl
	_asm shr edx, cl
	_asm ret
	lsr_count_ge_32:
	_asm sub cl, 32
	_asm mov eax, edx
	_asm xor edx, edx
	_asm shr eax, cl
	_asm ret
	lsr_count_ge_64:
	_asm xor edx, edx
	_asm xor eax, eax
	_asm ret
	}


}


#endif 
