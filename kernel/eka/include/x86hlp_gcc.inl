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
// e32\common\x86\x86hlp_gcc.inl
// If there are no exports then GCC 3.4.x does not generate a .reloc 
// section, without which rombuild can't relocate the .code section
// to its ROM address. Your ROM then goes boom early in the boot sequence.
// This unused export forces the PE to be generated with a .reloc section.
// 
//

EXPORT_C void __ignore_this_export()
	{
	}

static void DivisionByZero()
	{
	asm("int 0");
	}

extern "C" {

void __NAKED__ _alloca()
{
	// GCC passes the param in eax and expects no return value
	asm("pop ecx");
	asm("sub esp, eax");
	asm("push ecx");
	asm("ret");
}

void __NAKED__ _allmul()
//
// Multiply two 64 bit integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = arg 1
//		[esp+12], [esp+16] = arg 1
// Return result in edx:eax
// Remove arguments from stack
//
	{
	asm("mov eax, [esp+4]");		// eax = low1
	asm("mul dword ptr [esp+16]");	// edx:eax = low1*high2
	asm("mov ecx, eax");			// keep low 32 bits of product
	asm("mov eax, [esp+8]");		// eax = high1
	asm("mul dword ptr [esp+12]");	// edx:eax = high1*low2
	asm("add ecx, eax");			// accumulate low 32 bits of product
	asm("mov eax, [esp+4]");		// eax = low1
	asm("mul dword ptr [esp+12]");	// edx:eax = low1*low2
	asm("add edx, ecx");			// add cross terms to high 32 bits
	asm("ret");
	}

void __NAKED__ udiv64_divby0()
	{
	asm("int 0");					// division by zero exception
	asm("ret");
	}

__NAKED__ /*LOCAL_C*/ void UDiv64()
	{
	// unsigned divide edx:eax by edi:esi
	// quotient in ebx:eax, remainder in edi:edx
	// ecx, ebp, esi also modified
	asm("test edi, edi");
	asm("jnz short UDiv64a");			// branch if divisor >= 2^32
	asm("test esi, esi");
	asm("jz %a0": : "i"(&DivisionByZero)); // if divisor=0, branch to error routine
	asm("mov ebx, eax");				// ebx=dividend low
	asm("mov eax, edx");				// eax=dividend high
	asm("xor edx, edx");				// edx=0
	asm("div esi");						// quotient high now in eax
	asm("xchg eax, ebx");				// quotient high in ebx, dividend low in eax
	asm("div esi");						// quotient now in ebx:eax, remainder in edi:edx
	asm("ret");
	asm("UDiv64e:");
	asm("xor eax, eax");				// set result to 0xFFFFFFFF
	asm("dec eax");
	asm("jmp short UDiv64f");
	asm("UDiv64a:");
	asm("js short UDiv64b");			// skip if divisor msb set
	asm("bsr ecx, edi");				// ecx=bit number of divisor msb - 32
	asm("inc cl");
	asm("push edi");					// save divisor high
	asm("push esi");					// save divisor low
	asm("shrd esi, edi, cl");			// shift divisor right so that msb is bit 31
	asm("mov ebx, edx");				// dividend into ebx:ebp
	asm("mov ebp, eax");
	asm("shrd eax, edx, cl");			// shift dividend right same number of bits
	asm("shr edx, cl");
	asm("cmp edx, esi");				// check if approx quotient will be 2^32
	asm("jae short UDiv64e");			// if so, true result must be 0xFFFFFFFF
	asm("div esi");						// approximate quotient now in eax
	asm("UDiv64f:");
	asm("mov ecx, eax");				// into ecx
	asm("mul edi");						// multiply approx. quotient by divisor high
	asm("mov esi, eax");				// ls dword into esi, ms into edi
	asm("mov edi, edx");
	asm("mov eax, ecx");				// approx. quotient into eax
	asm("mul dword ptr [esp]");			// multiply approx. quotient by divisor low
	asm("add edx, esi");				// edi:edx:eax now equals approx. quotient * divisor
	asm("adc edi, 0");
	asm("xor esi, esi");
	asm("sub ebp, eax");				// subtract dividend - approx. quotient *divisor
	asm("sbb ebx, edx");
	asm("sbb esi, edi");
	asm("jnc short UDiv64c");			// if no borrow, result OK
	asm("dec ecx");						// else result is one too big
	asm("add ebp, [esp]");				// and add divisor to get correct remainder
	asm("adc ebx, [esp+4]");
	asm("UDiv64c:");
	asm("mov eax, ecx");				// result into ebx:eax, remainder into edi:edx
	asm("mov edi, ebx");
	asm("mov edx, ebp");
	asm("xor ebx, ebx");
	asm("add esp, 8");					// remove temporary values from stack
	asm("ret");
	asm("UDiv64b:");
	asm("mov ebx, 1");
	asm("sub eax, esi");				// subtract divisor from dividend
	asm("sbb edx, edi");
	asm("jnc short UDiv64d");			// if no borrow, result=1, remainder in edx:eax
	asm("add eax, esi");				// else add back
	asm("adc edx, edi");
	asm("dec ebx");						// and decrement quotient
	asm("UDiv64d:");
	asm("mov edi, edx");				// remainder into edi:edx
	asm("mov edx, eax");
	asm("mov eax, ebx");				// result in ebx:eax
	asm("xor ebx, ebx");
	asm("ret");
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
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("mov eax, [esp+16]");
	asm("mov edx, [esp+20]");
	asm("mov esi, [esp+24]");
	asm("mov edi, [esp+28]");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov ecx, edx");
	asm("mov edx, ebx");
	asm("mov ebx, edi");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
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
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("mov eax, [esp+16]");
	asm("mov edx, [esp+20]");
	asm("mov esi, [esp+24]");
	asm("mov edi, [esp+28]");
	asm("test edx, edx");
	asm("jns alldrvm_dividend_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("alldrvm_dividend_nonnegative:");
	asm("test edi, edi");
	asm("jns alldrvm_divisor_nonnegative");
	asm("neg edi");
	asm("neg esi");
	asm("sbb edi, 0");
	asm("alldrvm_divisor_nonnegative:");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov ebp, [esp+20]");
	asm("mov ecx, edx");
	asm("xor ebp, [esp+28]");
	asm("mov edx, ebx");
	asm("mov ebx, edi");
	asm("jns alldrvm_quotient_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("alldrvm_quotient_nonnegative:");
	asm("cmp dword ptr [esp+20], 0");
	asm("jns alldrvm_rem_nonnegative");
	asm("neg ebx");
	asm("neg ecx");
	asm("sbb ebx, 0");
	asm("alldrvm_rem_nonnegative:");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
	}

//__NAKED__ void _aulldiv()
__NAKED__ void __udivdi3 ()
//
// Divide two 64 bit unsigned integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("push ebx");
	asm("mov eax, [esp+20]");
	asm("mov edx, [esp+24]");
	asm("mov esi, [esp+28]");
	asm("mov edi, [esp+32]");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov edx, ebx");
	asm("pop ebx");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
	}


__NAKED__ void __divdi3()

//
// Divide two 64 bit signed integers returning a 64 bit result
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("push ebx");
	asm("mov eax, [esp+20]");
	asm("mov edx, [esp+24]");
	asm("mov esi, [esp+28]");
	asm("mov edi, [esp+32]");
	asm("test edx, edx");
	asm("jns divdi_dividend_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("divdi_dividend_nonnegative:");
	asm("test edi, edi");
	asm("jns divdi_divisor_nonnegative");
	asm("neg edi");
	asm("neg esi");
	asm("sbb edi, 0");
	asm("divdi_divisor_nonnegative:");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov ecx, [esp+24]");
	asm("mov edx, ebx");
	asm("xor ecx, [esp+32]");
	asm("jns divdi_quotient_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("divdi_quotient_nonnegative:");
	asm("pop ebx");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
	}

__NAKED__ void __umoddi3()
//
// Divide two 64 bit unsigned integers and return 64 bit remainder
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("push ebx");
	asm("mov eax, [esp+20]");
	asm("mov edx, [esp+24]");
	asm("mov esi, [esp+28]");
	asm("mov edi, [esp+32]");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov eax, edx");
	asm("mov edx, edi");
	asm("pop ebx");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
	}

__NAKED__ void __moddi3()
//
// Divide two 64 bit signed integers and return 64 bit remainder
// On entry:
//		[esp+4], [esp+8] = dividend
//		[esp+12], [esp+16] = divisor
// Return result in edx:eax
// Remove arguments from stack
//
	{
	asm("push ebp");
	asm("push edi");
	asm("push esi");
	asm("push ebx");
	asm("mov eax, [esp+20]");
	asm("mov edx, [esp+24]");
	asm("mov esi, [esp+28]");
	asm("mov edi, [esp+32]");
	asm("test edx, edx");
	asm("jns dividend_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("dividend_nonnegative:");
	asm("test edi, edi");
	asm("jns divisor_nonnegative");
	asm("neg edi");
	asm("neg esi");
	asm("sbb edi, 0");
	asm("divisor_nonnegative:");
	asm("call %a0": : "i"(&UDiv64));
	asm("mov eax, edx");
	asm("mov edx, edi");
	asm("cmp dword ptr [esp+24], 0");
	asm("jns rem_nonnegative");
	asm("neg edx");
	asm("neg eax");
	asm("sbb edx, 0");
	asm("rem_nonnegative:");
	asm("pop ebx");
	asm("pop esi");
	asm("pop edi");
	asm("pop ebp");
	asm("ret");
	}

__NAKED__ void _allshr()
//
// Arithmetic shift right EDX:EAX by CL
//
	{
	asm("cmp cl, 64");
	asm("jae asr_count_ge_64");
	asm("cmp cl, 32");
	asm("jae asr_count_ge_32");
	asm("shrd eax, edx, cl");
	asm("sar edx, cl");
	asm("ret");
	asm("asr_count_ge_32:");
	asm("sub cl, 32");
	asm("mov eax, edx");
	asm("cdq");
	asm("sar eax, cl");
	asm("ret");
	asm("asr_count_ge_64:");
	asm("sar edx, 32");
	asm("mov eax, edx");
	asm("ret");
	}

__NAKED__ void _allshl()
//
// shift left EDX:EAX by CL
//
	{
	asm("cmp cl, 64");
	asm("jae lsl_count_ge_64");
	asm("cmp cl, 32");
	asm("jae lsl_count_ge_32");
	asm("shld edx, eax, cl");
	asm("shl eax, cl");
	asm("ret");
	asm("lsl_count_ge_32:");
	asm("sub cl, 32");
	asm("mov edx, eax");
	asm("xor eax, eax");
	asm("shl edx, cl");
	asm("ret");
	asm("lsl_count_ge_64:");
	asm("xor edx, edx");
	asm("xor eax, eax");
	asm("ret");
	}

__NAKED__ void _aullshr()
//
// Logical shift right EDX:EAX by CL
//
	{
	asm("cmp cl, 64");
	asm("jae lsr_count_ge_64");
	asm("cmp cl, 32");
	asm("jae lsr_count_ge_32");
	asm("shrd eax, edx, cl");
	asm("shr edx, cl");
	asm("ret");
	asm("lsr_count_ge_32:");
	asm("sub cl, 32");
	asm("mov eax, edx");
	asm("xor edx, edx");
	asm("shr eax, cl");
	asm("ret");
	asm("lsr_count_ge_64:");
	asm("xor edx, edx");
	asm("xor eax, eax");
	asm("ret");
	}

}
