// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_realx.cpp
// 
//

#include "u32std.h"
#include <e32math.h>

#pragma warning (disable : 4100)	// unreferenced formal parameter
#pragma warning (disable : 4700)	// local variable 'this' used without
									// having been initialised
#pragma warning ( disable : 4414 )  // short jump to function converted to near


#if defined(__VC32__) && (_MSC_VER==1100)	// untested on MSVC++ > 5.0
// Workaround for MSVC++ 5.0 bug; MSVC incorrectly fixes up conditional jumps
// when the destination is a C++ function.
#define _ASM_j(cond,dest) _asm jn##cond short $+11 _asm jmp dest
#define _ASM_jn(cond,dest) _asm j##cond short $+11 _asm jmp dest
#pragma optimize( "", off )			// stop MSVC murdering the code
#else
#define _ASM_j(cond,dest) _asm j##cond dest
#define _ASM_jn(cond,dest) _asm jn##cond dest
#endif

//
// 64-bit precision floating point routines
// Register storage format:
// edx:ebx=64 bit normalised mantissa
// ecx bits 16-31 = 16-bit exponent, biased by 7FFF
// ecx bit 0 = sign
// ecx bit 8 = rounded-down flag
// ecx bit 9 = rounded-up flag
//
// Memory storage format:
// 3 doublewords per number
// Low 32 bits of mantissa at [addr]
// High 32 bits of mantissa at [addr+4]
// Exponent/flags/sign at [addr+8]
//

LOCAL_C void TRealXPanic(TInt aErr)
	{
	User::Panic(_L("MATHX"),aErr);
	}

__NAKED__ LOCAL_C void TRealXPanicEax(void)
	{
	_asm push eax
	_asm call TRealXPanic
	}

LOCAL_C __NAKED__ void TRealXRealIndefinite(void)
	{
	// return 'real indefinite' NaN in ecx,edx:ebx
	_asm mov ecx, 0xFFFF0001	// exponent=FFFF, sign negative
	_asm mov edx, 0xC0000000	// mantissa=C0000000 00000000
	_asm xor ebx, ebx
	_asm mov eax, -6			// return KErrArgument
	_asm ret
	}

LOCAL_C __NAKED__ void TRealXBinOpNaN(void)
	{
	// generic routine to process NaN's in binary operations
	// destination operand in ecx,edx:eax
	// source operand at [esi]

	_asm mov eax, [esi+8]		// source operand into eax,edi:ebp
	_asm mov edi, [esi+4]
	_asm mov ebp, [esi]
	_asm cmp ecx, 0xFFFF0000	// check if dest is a NaN
	_asm jb short TRealXBinOpNaN1	// if not, swap them
	_asm cmp edx, 0x80000000
	_asm jne short TRealXBinOpNaN2
	_asm test ebx, ebx
	_asm jne short TRealXBinOpNaN2
	TRealXBinOpNaN1:			// swap the operands
	_asm xchg ecx, eax
	_asm xchg edx, edi
	_asm xchg ebx, ebp
	TRealXBinOpNaN2:
	_asm cmp eax, 0xFFFF0000	// check if both operands are NaNs
	_asm jb short TRealXBinOpNaN4	// if not, ignore non-NaN operand
	_asm cmp edi, 0x80000000
	_asm jne short TRealXBinOpNaN3
	_asm test ebp, ebp
	_asm je short TRealXBinOpNaN4
	TRealXBinOpNaN3:			// if both operands are NaN's, compare significands
	_asm cmp edx, edi
	_asm ja short TRealXBinOpNaN4
	_asm jb short TRealXBinOpNaN5
	_asm cmp ebx, ebp
	_asm jae short TRealXBinOpNaN4
	TRealXBinOpNaN5:			// come here if dest is smaller - copy source to dest
	_asm mov ecx, eax
	_asm mov edx, edi
	_asm mov ebx, ebp
	TRealXBinOpNaN4:			// NaN with larger significand is in ecx,edx:ebx
	_asm or edx, 0x40000000		// convert an SNaN to a QNaN
	_asm mov eax, -6			// return KErrArgument
	_asm ret
	}

// Add TRealX at [esi] + ecx,edx:ebx
// Result in ecx,edx:ebx
// Error code in eax
// Note:	+0 + +0 = +0, -0 + -0 = -0, +0 + -0 = -0 + +0 = +0,
//			+/-0 + X = X + +/-0 = X, X + -X = -X + X = +0
__NAKED__ LOCAL_C void TRealXAdd()
	{
	_asm xor ch, ch				// clear rounding flags
	_asm cmp ecx, 0xFFFF0000	// check if dest=NaN or infinity
	_asm jnc addfpsd			// branch if it is
	_asm mov eax, [esi+8]		// fetch sign/exponent of source
	_asm cmp eax, 0xFFFF0000	// check if source=NaN or infinity
	_asm jnc addfpss			// branch if it is
	_asm cmp eax, 0x10000		// check if source=0
	_asm jc addfp0s				// branch if it is
	_asm cmp ecx, 0x10000		// check if dest=0
	_asm jc addfp0d				// branch if it is
	_asm and cl, 1				// clear bits 1-7 of ecx
	_asm and al, 1				// clear bits 1-7 of eax
	_asm mov ch, cl
	_asm xor ch, al				// xor of signs into ch bit 0
	_asm add ch, ch
	_asm or cl, ch				// and into cl bit 1
	_asm or al, ch				// and al bit 1
	_asm xor ch, ch				// clear rounding flags
	_asm mov ebp, [esi]			// fetch source mantissa 0-31
	_asm mov edi, [esi+4]		// fetch source mantissa 32-63
	_asm ror ecx, 16			// dest exponent into cx
	_asm ror eax, 16			// source exponent into ax
	_asm push ecx				// push dest exponent/sign
	_asm sub cx, ax				// cx = dest exponent - source exponent
	_asm je short addfp3b		// if equal, no shifting required
	_asm ja short addfp1		// branch if dest exponent >= source exponent
	_asm xchg ebx, ebp			// make sure edi:ebp contains the mantissa to be shifted
	_asm xchg edx, edi			//
	_asm xchg eax, [esp]		// and larger exponent and corresponding sign is on the stack
	_asm neg cx					// make cx positive = number of right shifts needed
	addfp1:
	_asm cmp cx, 64				// if more than 64 shifts needed
	_asm ja addfp2				// branch to output larger number
	_asm jb addfp3				// branch if <64 shifts
	_asm mov eax, edi			// exactly 64 shifts needed - rounding word=mant high
	_asm test ebp, ebp			// check bits lost
	_asm jz short addfp3a
	_asm or ch, 1				// if not all zero, set rounded-down flag
	addfp3a:
	_asm xor edi, edi			// clear edx:ebx
	_asm xor ebp, ebp
	_asm jmp short addfp5		// finished shifting
	addfp3b:					// exponents equal
	_asm xor eax, eax			// set rounding word=0
	_asm jmp short addfp5
	addfp3:
	_asm cmp cl, 32				// 32 or more shifts needed ?
	_asm jb short addfp4		// skip if <32
	_asm mov eax, ebp			// rounding word=mant low
	_asm mov ebp, edi			// mant low=mant high
	_asm xor edi, edi			// mant high=0
	_asm sub cl, 32				// reduce count by 32
	_asm jz short addfp5		// if now zero, finished shifting
	_asm shrd edi, eax, cl		// shift ebp:eax:edi right by cl bits
	_asm shrd eax, ebp, cl		//
	_asm shr ebp, cl			//
	_asm test edi, edi			// check bits lost in shift
	_asm jz short addfp5		// if all zero, finished
	_asm or ch, 1				// else set rounded-down flag
	_asm xor edi, edi			// clear edx again
	_asm jmp short addfp5		// finished shifting
	addfp4:						// <32 shifts needed now
	_asm xor eax, eax			// clear rounding word initially
	_asm shrd eax, ebp, cl		// shift edi:ebp:eax right by cl bits
	_asm shrd ebp, edi, cl		//
	_asm shr edi, cl			//

	addfp5:
	_asm mov [esp+3], ch		// rounding flag into ch image on stack
	_asm pop ecx				// recover sign and exponent into ecx, with rounding flag
	_asm ror ecx, 16			// into normal position
	_asm test cl, 2				// addition or subtraction needed ?
	_asm jnz short subfp1		// branch if subtraction
	_asm add ebx,ebp			// addition required - add mantissas
	_asm adc edx,edi			//
	_asm jnc short roundfp		// branch if no carry
	_asm rcr edx,1				// shift carry right into mantissa
	_asm rcr ebx,1				//
	_asm rcr eax,1				// and into rounding word
	_asm jnc short addfp5a
	_asm or ch, 1				// if 1 shifted out, set rounded-down flag
	addfp5a:
	_asm add ecx, 0x10000		// and increment exponent

	// perform rounding based on rounding word in eax and rounding flag in ch
	roundfp:
	_asm cmp eax, 0x80000000
	_asm jc roundfp0			// if rounding word<80000000, round down
	_asm ja roundfp1			// if >80000000, round up
	_asm test ch, 1
	_asm jnz short roundfp1		// if rounded-down flag set, round up
	_asm test ch, 2
	_asm jnz short roundfp0		// if rounded-up flag set, round down
	_asm test bl, 1				// else test mantissa lsb
	_asm jz short roundfp0		// round down if 0, up if 1 (round to even)
	roundfp1:					// Come here to round up
	_asm add ebx, 1				// increment mantissa
	_asm adc edx,0				//
	_asm jnc roundfp1a			// if no carry OK
	_asm rcr edx,1				// else shift carry into mantissa (edx:ebx=0 here)
	_asm add ecx, 0x10000		// and increment exponent
	roundfp1a:
	_asm cmp ecx, 0xFFFF0000	// check for overflow
	_asm jae short addfpovfw	// jump if overflow
	_asm mov ch, 2				// else set rounded-up flag
	_asm xor eax, eax			// return KErrNone
	_asm ret

	roundfp0:					// Come here to round down
	_asm cmp ecx, 0xFFFF0000	// check for overflow
	_asm jae short addfpovfw	// jump if overflow
	_asm test eax, eax			// else check if rounding word zero
	_asm jz short roundfp0a		// if so, leave rounding flags as they are
	_asm mov ch, 1				// else set rounded-down flag
	roundfp0a:
	_asm xor eax, eax			// return KErrNone
	_asm ret					// exit

	addfpovfw:					// Come here if overflow occurs
	_asm xor ch, ch				// clear rounding flags, exponent=FFFF
	_asm xor ebx, ebx
	_asm mov edx, 0x80000000	// mantissa=80000000 00000000 for infinity
	_asm mov eax, -9			// return KErrOverflow
	_asm ret

	// exponents differ by more than 64 - output larger number
	addfp2:
	_asm pop ecx				// recover exponent and sign
	_asm ror ecx, 16			// into normal position
	_asm or ch, 1				// set rounded-down flag
	_asm test cl, 2				// check if signs the same
	_asm jz addfp2a
	_asm xor ch, 3				// if not, set rounded-up flag
	addfp2a:
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// signs differ, so must subtract mantissas
	subfp1:
	_asm add ch, ch				// if rounded-down flag set, change it to rounded-up
	_asm neg eax				// subtract rounding word from 0
	_asm sbb ebx, ebp			// and subtract mantissas with borrow
	_asm sbb edx, edi			//
	_asm jnc short subfp2		// if no borrow, sign is correct
	_asm xor cl, 1				// else change sign of result
	_asm shr ch, 1				// change rounding back to rounded-down
	_asm not eax				// negate rounding word
	_asm not ebx				// and mantissa
	_asm not edx				//
	_asm add eax,1				// two's complement negation
	_asm adc ebx,0				//
	_asm adc edx,0				//
	subfp2:
	_asm jnz short subfp3		// branch if edx non-zero at this point
	_asm mov edx, ebx			// else shift ebx into edx
	_asm or edx, edx			//
	_asm jz short subfp4		// if still zero, branch
	_asm mov ebx, eax			// else shift rounding word into ebx
	_asm xor eax, eax			// and zero rounding word
	_asm sub ecx, 0x200000		// decrease exponent by 32 due to shift
	_asm jnc short subfp3		// if no borrow, carry on
	_asm jmp short subfpundflw	// if borrow here, underflow
	subfp4:
	_asm mov edx, eax			// move rounding word into edx
	_asm or edx, edx			// is edx still zero ?
	_asm jz short subfp0		// if so, result is precisely zero
	_asm xor ebx, ebx			// else zero ebx and rounding word
	_asm xor eax, eax			//
	_asm sub ecx, 0x400000		// and decrease exponent by 64 due to shift
	_asm jc short subfpundflw	// if borrow, underflow
	subfp3:
	_asm mov edi, ecx			// preserve sign and exponent
	_asm bsr ecx, edx			// position of most significant 1 into ecx
	_asm neg ecx				//
	_asm add ecx, 31			// cl = 31-position of MS 1 = number of shifts to normalise
	_asm shld edx, ebx, cl		// shift edx:ebx:eax left by cl bits
	_asm shld ebx, eax, cl		//
	_asm shl eax, cl			//
	_asm mov ebp, ecx			// bit count into ebp for subtraction
	_asm shl ebp, 16			// shift left by 16 to align with exponent
	_asm mov ecx, edi			// exponent, sign, rounding flags back into ecx
	_asm sub ecx, ebp			// subtract shift count from exponent
	_asm jc short subfpundflw	// if borrow, underflow
	_asm cmp ecx, 0x10000		// check if exponent 0
	_asm jnc roundfp			// if not, jump to round result, else underflow

	// come here if underflow
	subfpundflw:
	_asm and ecx, 1				// set exponent to zero, leave sign
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm mov eax, -10			// return KErrUnderflow
	_asm ret

	// come here to return zero result
	subfp0:
	_asm xor ecx, ecx			// set exponent to zero, positive sign
	_asm xor edx, edx
	_asm xor ebx, ebx
	addfp0snzd:
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if source=0 - eax=source exponent/sign
	addfp0s:
	_asm cmp ecx, 0x10000		// check if dest=0
	_asm jnc addfp0snzd			// if not, return dest unaltered
	_asm and ecx, eax			// else both zero, result negative iff both zeros negative
	_asm and ecx, 1
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if dest=0, source nonzero
	addfp0d:
	_asm mov ebx, [esi]			// return source unaltered
	_asm mov edx, [esi+4]
	_asm mov ecx, [esi+8]
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if dest=NaN or infinity
	addfpsd:
	_asm cmp edx, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	_asm mov eax, [esi+8]		// eax=second operand exponent
	_asm cmp eax, 0xFFFF0000	// check second operand for NaN or infinity
	_asm jae short addfpsd1		// branch if NaN or infinity
	addfpsd2:
	_asm mov eax, -9			// else return dest unaltered (infinity) and KErrOverflow
	_asm ret
	addfpsd1:
	_asm mov ebp, [esi]			// source mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm xor al, cl				// both operands are infinity - check signs
	_asm test al, 1
	_asm jz short addfpsd2		// if both the same, return KErrOverflow
	_asm jmp TRealXRealIndefinite	// else return 'real indefinite'

	// come here if source=NaN or infinity, dest finite
	addfpss:
	_asm mov ebp, [esi]			// source mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm mov ecx, eax			// if source=infinity, return source unaltered
	_asm mov edx, edi
	_asm mov ebx, ebp
	_asm mov eax, -9			// return KErrOverflow
	_asm ret
	}

// Subtract TRealX at [esi] - ecx,edx:ebx
// Result in ecx,edx:ebx
// Error code in eax
__NAKED__ LOCAL_C void TRealXSubtract()
	{
	_asm xor cl, 1              // negate subtrahend
	_asm jmp TRealXAdd
	}

// Multiply TRealX at [esi] * ecx,edx:ebx
// Result in ecx,edx:ebx
// Error code in eax
__NAKED__ LOCAL_C void TRealXMultiply()
	{
	_asm xor ch, ch				// clear rounding flags
	_asm mov eax, [esi+8]		// fetch sign/exponent of source
	_asm xor cl, al				// xor signs
	_asm cmp ecx, 0xFFFF0000	// check if dest=NaN or infinity
	_asm jnc mulfpsd			// branch if it is
	_asm cmp eax, 0xFFFF0000	// check if source=NaN or infinity
	_asm jnc mulfpss			// branch if it is
	_asm cmp eax, 0x10000		// check if source=0
	_asm jc mulfp0				// branch if it is
	_asm cmp ecx, 0x10000		// check if dest=0
	_asm jc mulfp0				// branch if it is
	_asm push ecx				// save result sign
	_asm shr ecx, 16			// dest exponent into cx
	_asm shr eax, 16			// source exponent into ax
	_asm add eax, ecx			// add exponents
	_asm sub eax, 0x7FFE		// eax now contains result exponent
	_asm push eax				// save it
	_asm mov edi, edx			// save dest mantissa high
	_asm mov eax, ebx			// dest mantissa low -> eax
	_asm mul dword ptr [esi]	// dest mantissa low * source mantissa low -> edx:eax
	_asm xchg ebx, eax			// result dword 0 -> ebx, dest mant low -> eax
	_asm mov ebp, edx			// result dword 1 -> ebp
	_asm mul dword ptr [esi+4]	// dest mant low * src mant high -> edx:eax
	_asm add ebp, eax			// add in partial product to dwords 1 and 2
	_asm adc edx, 0				//
	_asm mov ecx, edx			// result dword 2 -> ecx
	_asm mov eax, edi			// dest mant high -> eax
	_asm mul dword ptr [esi+4]	// dest mant high * src mant high -> edx:eax
	_asm add ecx, eax			// add in partial product to dwords 2, 3
	_asm adc edx, 0				//
	_asm mov eax, edi			// dest mant high -> eax
	_asm mov edi, edx			// result dword 3 -> edi
	_asm mul dword ptr [esi]	// dest mant high * src mant low -> edx:eax
	_asm add ebp, eax			// add in partial product to dwords 1, 2
	_asm adc ecx, edx			//
	_asm adc edi, 0				// 128-bit mantissa product is now in edi:ecx:ebp:ebx
	_asm mov edx, edi			// top 64 bits into edx:ebx
	_asm mov edi, ebx
	_asm mov ebx, ecx			// bottom 64 bits now in ebp:edi
	_asm pop ecx				// recover exponent
	_asm js short mulfp1		// skip if mantissa normalised
	_asm add edi, edi			// else shift left (only one shift will be needed)
	_asm adc ebp, ebp
	_asm adc ebx, ebx
	_asm adc edx, edx
	_asm dec ecx				// and decrement exponent
	mulfp1:
	_asm cmp ebp, 0x80000000	// compare bottom 64 bits with 80000000 00000000 for rounding
	_asm ja short mulfp2		// branch to round up
	_asm jb short mulfp3		// branch to round down
	_asm test edi, edi
	_asm jnz short mulfp2		// branch to round up
	_asm test bl, 1				// if exactly half-way, test LSB of result mantissa
	_asm jz short mulfp4		// if LSB=0, round down (round to even)
	mulfp2:
	_asm add ebx, 1				// round up - increment mantissa
	_asm adc edx, 0
	_asm jnc short mulfp2a
	_asm rcr edx, 1
	_asm inc ecx
	mulfp2a:
	_asm mov al, 2				// set rounded-up flag
	_asm jmp short mulfp5
	mulfp3:						// round down
	_asm xor al, al				// clear rounding flags
	_asm or ebp, edi			// check for exact result
	_asm jz short mulfp5		// skip if exact
	mulfp4:						// come here to round down when we know result inexact
	_asm mov al, 1				// else set rounded-down flag
	mulfp5:						// final mantissa now in edx:ebx, exponent in ecx
	_asm cmp ecx, 0xFFFF		// check for overflow
	_asm jge short mulfp6		// branch if overflow
	_asm cmp ecx, 0				// check for underflow
	_asm jle short mulfp7		// branch if underflow
	_asm shl ecx, 16			// else exponent up to top end of ecx
	_asm mov ch, al				// rounding flags into ch
	_asm pop eax				// recover result sign
	_asm mov cl, al				// into cl
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if overflow
	mulfp6:
	_asm pop eax				// recover result sign
	_asm mov ecx, 0xFFFF0000	// exponent=FFFF
	_asm mov cl, al				// sign into cl
	_asm mov edx, 0x80000000	// set mantissa to 80000000 00000000 for infinity
	_asm xor ebx, ebx
	_asm mov eax, -9			// return KErrOverflow
	_asm ret

	// come here if underflow
	mulfp7:
	_asm pop eax				// recover result sign
	_asm xor ecx, ecx			// exponent=0
	_asm mov cl, al				// sign into cl
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm mov eax, -10			// return KErrUnderflow
	_asm ret

	// come here if either operand zero
	mulfp0:
	_asm and ecx, 1				// set exponent=0, keep sign
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if destination operand NaN or infinity
	mulfpsd:
	_asm cmp edx, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	_asm cmp eax, 0xFFFF0000	// check second operand for NaN or infinity
	_asm jae short mulfpsd1		// branch if NaN or infinity
	_asm cmp eax, 0x10000		// check if second operand zero
	_ASM_j(c,TRealXRealIndefinite)	// if so, return 'real indefinite'
	_asm mov eax, -9			// else return dest (infinity) with xor sign and KErrOverflow
	_asm ret
	mulfpsd1:
	_asm mov ebp, [esi]			// source mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm mov eax, -9			// both operands infinity - return infinity with xor sign
	_asm ret					// and KErrOverflow

	// come here if source operand NaN or infinity, destination finite
	mulfpss:
	_asm mov ebp, [esi]			// source mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm cmp ecx, 0x10000		// source=infinity, check if dest=0
	_ASM_j(c,TRealXRealIndefinite)	// if so, return 'real indefinite'
	_asm or ecx, 0xFFFF0000		// set exp=FFFF, leave xor sign in cl
	_asm mov edx, edi			// set mantissa for infinity
	_asm mov ebx, ebp
	_asm mov eax, -9			// return KErrOverflow
	_asm ret
	}

// Divide 96-bit unsigned dividend EDX:EAX:0 by 64-bit unsigned divisor ECX:EBX
// Assume ECX bit 31 = 1, ie 2^63 <= divisor < 2^64
// Assume the quotient fits in 32 bits
// Return 32 bit quotient in EDI
// Return 64 bit remainder in EBP:ESI
__NAKED__ LOCAL_C void LongDivide(void)
	{
	_asm push edx				// save dividend
	_asm push eax				//
	_asm cmp edx, ecx			// check if truncation of divisor will overflow DIV instruction
	_asm jb short longdiv1		// skip if not
	_asm xor eax, eax			// else return quotient of 0xFFFFFFFF
	_asm dec eax				//
	_asm jmp short longdiv2		//
	longdiv1:
	_asm div ecx				// divide EDX:EAX by ECX to give approximate quotient in EAX
	longdiv2:
	_asm mov edi, eax			// save approx quotient
	_asm mul ebx				// multiply approx quotient by full divisor ECX:EBX
	_asm mov esi, eax			// first partial product into EBP:ESI
	_asm mov ebp, edx			//
	_asm mov eax, edi			// approx quotient back into eax
	_asm mul ecx				// upper partial product now in EDX:EAX
	_asm add eax, ebp			// add to form 96-bit product in EDX:EAX:ESI
	_asm adc edx, 0				//
	_asm neg esi				// remainder = dividend - approx quotient * divisor
	_asm mov ebp, [esp]			// fetch dividend bits 32-63
	_asm sbb ebp, eax			//
	_asm mov eax, [esp+4]		// fetch dividend bits 64-95
	_asm sbb eax, edx			// remainder is now in EAX:EBP:ESI
	_asm jns short longdiv4		// if remainder positive, quotient is correct, so exit
	longdiv3:
	_asm dec edi				// else quotient is too big, so decrement it
	_asm add esi, ebx			// and add divisor to remainder
	_asm adc ebp, ecx			//
	_asm adc eax, 0				//
	_asm js short longdiv3		// if still negative, repeat (requires <4 iterations)
	longdiv4:
	_asm add esp, 8				// remove dividend from stack
	_asm ret					// return with quotient in EDI, remainder in EBP:ESI
	}

// Divide TRealX at [esi] / ecx,edx:ebx
// Result in ecx,edx:ebx
// Error code in eax
__NAKED__ LOCAL_C void TRealXDivide(void)
	{
	_asm xor ch, ch				// clear rounding flags
	_asm mov eax, [esi+8]		// fetch sign/exponent of dividend
	_asm xor cl, al				// xor signs
	_asm cmp eax, 0xFFFF0000	// check if dividend=NaN or infinity
	_asm jnc divfpss			// branch if it is
	_asm cmp ecx, 0xFFFF0000	// check if divisor=NaN or infinity
	_asm jnc divfpsd			// branch if it is
	_asm cmp ecx, 0x10000		// check if divisor=0
	_asm jc divfpdv0			// branch if it is
	_asm cmp eax, 0x10000		// check if dividend=0
	_asm jc divfpdd0			// branch if it is
	_asm push esi				// save pointer to dividend
	_asm push ecx				// save result sign
	_asm shr ecx, 16			// divisor exponent into cx
	_asm shr eax, 16			// dividend exponent into ax
	_asm sub eax, ecx			// subtract exponents
	_asm add eax, 0x7FFE		// eax now contains result exponent
	_asm push eax				// save it
	_asm mov ecx, edx			// divisor mantissa into ecx:ebx
	_asm mov edx, [esi+4]		// dividend mantissa into edx:eax
	_asm mov eax, [esi]
	_asm xor edi, edi			// clear edi initially
	_asm cmp edx, ecx			// compare EDX:EAX with ECX:EBX
	_asm jb short divfp1		// if EDX:EAX < ECX:EBX, leave everything as is
	_asm ja short divfp2		//
	_asm cmp eax, ebx			// if EDX=ECX, then compare ls dwords
	_asm jb short divfp1		// if dividend mant < divisor mant, leave everything as is
	divfp2:
	_asm sub eax, ebx			// else dividend mant -= divisor mant
	_asm sbb edx, ecx			//
	_asm inc edi				// and EDI=1 (bit 0 of EDI is the integer part of the result)
	_asm inc dword ptr [esp]	// also increment result exponent
	divfp1:
	_asm push edi				// save top bit of result
	_asm call LongDivide		// divide EDX:EAX:0 by ECX:EBX to give next 32 bits of result in EDI
	_asm push edi				// save next 32 bits of result
	_asm mov edx, ebp			// remainder from EBP:ESI into EDX:EAX
	_asm mov eax, esi			//
	_asm call LongDivide		// divide EDX:EAX:0 by ECX:EBX to give next 32 bits of result in EDI
	_asm test byte ptr [esp+4], 1	// test integer bit of result
	_asm jnz short divfp4		// if set, no need to calculate another bit
	_asm xor eax, eax			//
	_asm add esi, esi			// 2*remainder into EAX:EBP:ESI
	_asm adc ebp, ebp			//
	_asm adc eax, eax			//
	_asm sub esi, ebx			// subtract divisor to generate final quotient bit
	_asm sbb ebp, ecx			//
	_asm sbb eax, 0				//
	_asm jnc short divfp3		// skip if no borrow - in this case eax=0
	_asm add esi, ebx			// if borrow add back - final remainder now in EBP:ESI
	_asm adc ebp, ecx			//
	_asm adc eax, 0				// eax will be zero after this and carry will be set
	divfp3:
	_asm cmc					// final bit = 1-C
	_asm rcr eax, 1				// shift it into eax bit 31
	_asm mov ebx, edi			// result into EDX:EBX:EAX, remainder in EBP:ESI
	_asm pop edx
	_asm add esp, 4				// discard integer bit (zero)
	_asm jmp short divfp5		// branch to round

	divfp4:						// integer bit was set
	_asm mov ebx, edi			// result into EDX:EBX:EAX
	_asm pop edx				//
	_asm pop eax				// integer part of result into eax (=1)
	_asm stc					// shift a 1 into top end of mantissa
	_asm rcr edx,1				//
	_asm rcr ebx,1				//
	_asm rcr eax,1				// bottom bit into eax bit 31

	// when we get to here we have 65 bits of quotient mantissa in
	// EDX:EBX:EAX (bottom bit in eax bit 31)
	// and the remainder is in EBP:ESI
	divfp5:
	_asm pop ecx				// recover result exponent
	_asm add eax, eax			// test rounding bit
	_asm jnc short divfp6		// branch to round down
	_asm or ebp, esi			// test remainder to see if we are exactly half-way
	_asm jnz short divfp7		// if not, round up
	_asm test bl, 1				// exactly halfway - test LSB of mantissa
	_asm jz short divfp8		// round down if LSB=0 (round to even)
	divfp7:
	_asm add ebx, 1				// round up - increment mantissa
	_asm adc edx, 0
	_asm jnc short divfp7a
	_asm rcr edx, 1				// if carry, shift 1 into mantissa MSB
	_asm inc ecx				// and increment exponent
	divfp7a:
	_asm mov al, 2				// set rounded-up flag
	_asm jmp short divfp9
	divfp6:
	_asm xor al, al				// round down - first clear rounding flags
	_asm or ebp, esi			// test if result exact
	_asm jz short divfp9		// skip if exact
	divfp8:						// come here to round down when we know result is inexact
	_asm mov al, 1				// set rounded-down flag
	divfp9:						// final mantissa now in edx:ebx, exponent in ecx
	_asm cmp ecx, 0xFFFF		// check for overflow
	_asm jge short divfp10		// branch if overflow
	_asm cmp ecx, 0				// check for underflow
	_asm jle short divfp11		// branch if underflow
	_asm shl ecx, 16			// else exponent up to top end of ecx
	_asm mov ch, al				// rounding flags into ch
	_asm pop eax				// recover result sign
	_asm mov cl, al				// into cl
	_asm pop esi				// recover dividend pointer
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if overflow
	divfp10:
	_asm pop eax				// recover result sign
	_asm mov ecx, 0xFFFF0000	// exponent=FFFF
	_asm mov cl, al				// sign into cl
	_asm mov edx, 0x80000000	// set mantissa to 80000000 00000000 for infinity
	_asm xor ebx, ebx
	_asm mov eax, -9			// return KErrOverflow
	_asm pop esi				// recover dividend pointer
	_asm ret

	// come here if underflow
	divfp11:
	_asm pop eax				// recover result sign
	_asm xor ecx, ecx			// exponent=0
	_asm mov cl, al				// sign into cl
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm mov eax, -10			// return KErrUnderflow
	_asm pop esi				// recover dividend pointer
	_asm ret


	// come here if divisor=0, dividend finite
	divfpdv0:
	_asm cmp eax, 0x10000		// check if dividend also zero
	_ASM_j(c,TRealXRealIndefinite)	// if so, return 'real indefinite'
	_asm or ecx, 0xFFFF0000		// else set exponent=FFFF, leave xor sign in cl
	_asm mov edx, 0x80000000	// set mantissa for infinity
	_asm xor ebx, ebx
	_asm mov eax, -41			// return KErrDivideByZero
	_asm ret

	// come here if dividend=0, divisor finite and nonzero
	divfpdd0:
	_asm and ecx, 1				// exponent=0, leave xor sign in cl
	_asm xor eax, eax			// return KErrNone
	_asm ret

	// come here if dividend is a NaN or infinity
	divfpss:
	_asm mov ebp, [esi]			// dividend mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm cmp ecx, 0xFFFF0000	// check divisor for NaN or infinity
	_asm jae short divfpss1		// branch if NaN or infinity
	_asm or ecx, 0xFFFF0000		// infinity/finite - return infinity with xor sign
	_asm mov edx, 0x80000000
	_asm xor ebx, ebx
	_asm mov eax, -9			// return KErrOverflow
	_asm ret
	divfpss1:
	_asm cmp edx, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	_asm jmp TRealXRealIndefinite	// if both operands infinite, return 'real indefinite'

	// come here if divisor is a NaN or infinity, dividend finite
	divfpsd:
	_asm cmp edx, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	_asm and ecx, 1				// dividend is finite, divisor=infinity, so return 0 with xor sign
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm xor eax, eax			// return KErrNone
	_asm ret
	}

// TRealX modulo - dividend at [esi], divisor in ecx,edx:ebx
// Result in ecx,edx:ebx
// Error code in eax
__NAKED__ LOCAL_C void TRealXModulo(void)
	{
	_asm mov eax, [esi+8]		// fetch sign/exponent of dividend
	_asm mov cl, al				// result sign=dividend sign
	_asm xor ch, ch				// clear rounding flags
	_asm cmp eax, 0xFFFF0000	// check if dividend=NaN or infinity
	_asm jnc modfpss			// branch if it is
	_asm cmp ecx, 0xFFFF0000	// check if divisor=NaN or infinity
	_asm jnc modfpsd			// branch if it is
	_asm cmp ecx, 0x10000		// check if divisor=0
	_ASM_j(c,TRealXRealIndefinite)	// if so, return 'real indefinite'
	_asm shr eax, 16			// ax=dividend exponent
	_asm ror ecx, 16			// cx=divisor exponent
	_asm sub ax, cx				// ax=dividend exponent-divisor exponent
	_asm jc modfpdd0			// if dividend exponent is smaller, return dividend
	_asm cmp ax, 64				// check if exponents differ by >= 64 bits
	_asm jnc modfplp			// if so, underflow
	_asm mov ah, 0				// ah bit 0 acts as 65th accumulator bit
	_asm mov ebp, [esi]			// edi:ebp=dividend mantissa
	_asm mov edi, [esi+4]		//
	_asm jmp short modfp2		// skip left shift on first iteration
	modfp1:
	_asm add ebp, ebp			// shift accumulator left (65 bits)
	_asm adc edi, edi
	_asm adc ah, ah
	modfp2:
	_asm sub ebp, ebx			// subtract divisor from dividend
	_asm sbb edi, edx
	_asm sbb ah, 0
	_asm jnc short modfp3		// skip if no borrow
	_asm add ebp, ebx			// else add back
	_asm adc edi, edx
	_asm adc ah, 0
	modfp3:
	_asm dec al					// any more bits to do?
	_asm jns short modfp1		// loop if there are
	_asm mov edx, edi			// result mantissa (not yet normalised) into edx:ebx
	_asm mov ebx, ebp
	_asm or edi, ebx			// check for zero
	_asm jz modfp0				// jump if result zero
	_asm or edx, edx			// check if ms dword zero
	_asm jnz short modfp4
	_asm mov edx, ebx			// if so, shift left by 32
	_asm xor ebx, ebx
	_asm sub cx, 32				// and decrement exponent by 32
	_asm jbe modfpund			// if borrow or exponent zero, underflow
	modfp4:
	_asm mov edi, ecx			// preserve sign and exponent
	_asm bsr ecx, edx			// position of most significant 1 into ecx
	_asm neg ecx				//
	_asm add ecx, 31			// cl = 31-position of MS 1 = number of shifts to normalise
	_asm shld edx, ebx, cl		// shift edx:ebx left by cl bits
	_asm shl ebx, cl			//
	_asm mov ebp, ecx			// bit count into ebp for subtraction
	_asm mov ecx, edi			// exponent & sign back into ecx
	_asm sub cx, bp				// subtract shift count from exponent
	_asm jbe short modfpund		// if borrow or exponent 0, underflow
	_asm rol ecx, 16			// else ecx=exponent:sign
	_asm xor eax, eax			// normal exit, result in ecx,edx:ebx
	_asm ret

	// dividend=NaN or infinity
	modfpss:
	_asm mov ebp, [esi]			// dividend mantissa into edi:ebp
	_asm mov edi, [esi+4]
	_asm cmp edi, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebp, ebp
	_ASM_jn(e,TRealXBinOpNaN)
	_asm cmp ecx, 0xFFFF0000	// check divisor for NaN or infinity
	_ASM_j(b,TRealXRealIndefinite)	// infinity%finite - return 'real indefinite'
	_asm cmp edx, 0x80000000	// check for divisor=infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	_asm jmp TRealXRealIndefinite	// if both operands infinite, return 'real indefinite'

	// divisor=NaN or infinity, dividend finite
	modfpsd:
	_asm cmp edx, 0x80000000	// check for infinity
	_ASM_jn(e,TRealXBinOpNaN)	// branch if NaN
	_asm test ebx, ebx
	_ASM_jn(e,TRealXBinOpNaN)
	// finite%infinity - return dividend unaltered

	modfpdd0:
	_asm mov ebx, [esi]			// normal exit, return dividend unaltered
	_asm mov edx, [esi+4]
	_asm mov ecx, [esi+8]
	_asm xor eax, eax
	_asm ret

	modfp0:
	_asm shr ecx, 16			// normal exit, result 0
	_asm xor eax, eax
	_asm ret

	modfpund:
	_asm shr ecx, 16			// underflow, result 0
	_asm mov eax, -10			// return KErrUnderflow
	_asm ret

	modfplp:
	_asm shr ecx, 16			// loss of precision, result 0
	_asm mov eax, -7			// return KErrTotalLossOfPrecision
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::TRealX()
/**
Constructs a default extended precision object.

This sets the value to zero.
*/
	{
	_asm xor eax, eax
	_asm mov [ecx], eax			// set value to zero
	_asm mov [ecx+4], eax
	_asm mov [ecx+8], eax
	_asm mov eax, ecx			// must return this
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::TRealX(TUint /*aExp*/, TUint /*aMantHi*/, TUint /*aMantLo*/)
/**
Constructs an extended precision object from an explicit exponent and
a 64 bit mantissa.

@param aExp    The exponent 
@param aMantHi The high order 32 bits of the 64 bit mantissa 
@param aMantLo The low order 32 bits of the 64 bit mantissa 
*/
	{
	_asm mov eax, [esp+4]		// eax=aExp
	_asm mov [ecx+8], eax
	_asm mov eax, [esp+8]		// eax=aMantHi
	_asm mov [ecx+4], eax
	_asm mov eax, [esp+12]		// eax=aMantLo
	_asm mov [ecx], eax
	_asm mov eax, ecx			// must return this
	_asm ret 12
	}




__NAKED__ EXPORT_C TInt TRealX::Set(TInt /*aInt*/)
/**
Gives this extended precision object a new value taken
from a signed integer.

@param aInt The signed integer value.

@return KErrNone, always.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return code in eax
	_asm mov edx, [esp+4]		// edx=aInt
	_asm or edx, edx			// test sign/zero
	_asm mov eax, 0x7FFF
	_asm jz short trealxfromint0	// branch if 0
	_asm jns short trealxfromint1	// skip if positive
	_asm neg edx					// take absolute value
	_asm add eax, 0x10000			// sign bit in eax bit 16
	trealxfromint1:
	_asm push ecx					// save this
	_asm bsr ecx, edx				// bit number of edx MSB into ecx
	_asm add eax, ecx				// add to eax to form result exponent
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of shifts to normalise edx
	_asm shl edx, cl				// normalise edx
	_asm pop ecx					// this back into ecx
	_asm ror eax, 16				// sign/exponent into normal positions
	_asm mov [ecx+4], edx			// store mantissa high word
	_asm mov [ecx+8], eax			// store sign/exponent
	_asm xor eax, eax
	_asm mov [ecx], eax				// zero mantissa low word
	_asm ret 4						// return KErrNone
	trealxfromint0:
	_asm mov [ecx], edx
	_asm mov [ecx+4], edx			// store mantissa high word=0
	_asm mov [ecx+8], edx			// store sign/exponent=0
	_asm xor eax, eax				// return KErrNone
	_asm ret 4
	}




__NAKED__ EXPORT_C TInt TRealX::Set(TUint /*aInt*/)
/**
Gives this extended precision object a new value taken from
an unsigned integer.

@param aInt The unsigned integer value.

@return KErrNone, always.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return code in eax
	_asm mov edx, [esp+4]		// edx=aInt
	_asm mov eax, 0x7FFF
	_asm or edx, edx				// test for 0
	_asm jz short trealxfromuint0	// branch if 0
	_asm push ecx					// save this
	_asm bsr ecx, edx				// bit number of edx MSB into ecx
	_asm add eax, ecx				// add to eax to form result exponent
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of shifts to normalise edx
	_asm shl edx, cl				// normalise edx
	_asm pop ecx					// this back into ecx
	_asm shl eax, 16				// exponent into normal position
	_asm mov [ecx+4], edx			// store mantissa high word
	_asm mov [ecx+8], eax			// store exponent
	_asm xor eax, eax
	_asm mov [ecx], eax				// zero mantissa low word
	_asm ret 4						// return KErrNone
	trealxfromuint0:
	_asm mov [ecx], edx
	_asm mov [ecx+4], edx			// store mantissa high word=0
	_asm mov [ecx+8], edx			// store sign/exponent=0
	_asm xor eax, eax				// return KErrNone
	_asm ret 4
	}




__NAKED__ LOCAL_C void TRealXFromTInt64(void)
	{
	// Convert TInt64 in edx:ebx to TRealX in ecx,edx:ebx
	_asm mov eax, 0x7FFF
	_asm or edx, edx				// test sign/zero
	_asm jz short trealxfromtint64a	// branch if top word zero
	_asm jns short trealxfromtint64b
	_asm add eax, 0x10000			// sign bit into eax bit 16
	_asm neg edx					// take absolute value
	_asm neg ebx
	_asm sbb edx, 0
	_asm jz short trealxfromtint64d	// branch if top word zero
	trealxfromtint64b:
	_asm bsr ecx, edx				// ecx=bit number of edx MSB
	_asm add eax, ecx				// add to exponent in eax
	_asm add eax, 32
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of left shifts to normalise
	_asm shld edx, ebx, cl			// shift left to normalise edx:ebx
	_asm shl ebx, cl
	_asm mov ecx, eax				// sign/exponent into ecx
	_asm ror ecx, 16				// and into normal positions
	_asm ret
	trealxfromtint64a:				// come here if top word zero
	_asm or ebx, ebx				// test for bottom word also zero
	_asm jz short trealxfromtint64c	// branch if it is
	trealxfromtint64d:				// come here if top word zero, bottom word not
	_asm mov edx, ebx				// shift edx:ebx left 32
	_asm xor ebx, ebx
	_asm bsr ecx, edx				// ecx=bit number of edx MSB
	_asm add eax, ecx				// add to exponent in eax
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of left shifts to normalise
	_asm shl edx, cl				// normalise
	_asm mov ecx, eax				// sign/exponent into ecx
	_asm ror ecx, 16				// and into normal positions
	_asm ret
	trealxfromtint64c:				// entire number is zero
	_asm xor ecx, ecx
	_asm ret
	}




__NAKED__ EXPORT_C TInt TRealX::Set(const TInt64& /*aInt*/)
/**
Gives this extended precision object a new value taken from
a 64 bit integer.

@param aInt The 64 bit integer value.

@return KErrNone, always.
*/
	{
	// on entry ecx=this, [esp+4]=address of aInt, return code in eax
	_asm push ebx
	_asm push ecx
	_asm mov edx, [esp+12]		// edx=address of aInt
	_asm mov ebx, [edx]
	_asm mov edx, [edx+4]		// edx:ebx=aInt
	_asm call TRealXFromTInt64	// convert to TRealX in ecx,edx:ebx
	_asm pop eax				// eax=this
	_asm mov [eax], ebx			// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm xor eax, eax			// return KErrNone
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ LOCAL_C void __6TRealXi()
	{
	// common function for int to TRealX
	_asm mov edx, [esp+4]		// edx=aInt
	_asm or edx, edx			// test sign/zero
	_asm mov eax, 0x7FFF
	_asm jz short trealxfromint0	// branch if 0
	_asm jns short trealxfromint1	// skip if positive
	_asm neg edx					// take absolute value
	_asm add eax, 0x10000			// sign bit in eax bit 16
	trealxfromint1:
	_asm push ecx					// save this
	_asm bsr ecx, edx				// bit number of edx MSB into ecx
	_asm add eax, ecx				// add to eax to form result exponent
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of shifts to normalise edx
	_asm shl edx, cl				// normalise edx
	_asm pop ecx					// this back into ecx
	_asm ror eax, 16				// sign/exponent into normal positions
	_asm mov [ecx+4], edx			// store mantissa high word
	_asm mov [ecx+8], eax			// store sign/exponent
	_asm xor eax, eax
	_asm mov [ecx], eax				// zero mantissa low word
	_asm mov eax, ecx				// return eax=this
	_asm ret 4
	trealxfromint0:
	_asm mov [ecx], edx
	_asm mov [ecx+4], edx			// store mantissa high word=0
	_asm mov [ecx+8], edx			// store sign/exponent=0
	_asm mov eax, ecx				// return eax=this
	_asm ret 4
	}




__NAKED__ EXPORT_C TRealX::TRealX(TInt /*aInt*/)
/**
Constructs an extended precision object from a signed integer value.

@param aInt The signed integer value.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return eax=this
	_asm jmp __6TRealXi
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator=(TInt /*aInt*/)
/**
Assigns the specified signed integer value to this extended precision object.

@param aInt The signed integer value.

@return A reference to this extended precision object.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return eax=this
	_asm jmp __6TRealXi
	}




__NAKED__ LOCAL_C void __6TRealXui()
	{
	// common function for unsigned int to TRealX
	_asm mov edx, [esp+4]		// edx=aInt
	_asm mov eax, 0x7FFF
	_asm or edx, edx				// test for zero
	_asm jz short trealxfromuint0	// branch if 0
	_asm push ecx					// save this
	_asm bsr ecx, edx				// bit number of edx MSB into ecx
	_asm add eax, ecx				// add to eax to form result exponent
	_asm neg cl
	_asm add cl, 31					// 31-bit number = number of shifts to normalise edx
	_asm shl edx, cl				// normalise edx
	_asm pop ecx					// this back into ecx
	_asm shl eax, 16				// exponent into normal position
	_asm mov [ecx+4], edx			// store mantissa high word
	_asm mov [ecx+8], eax			// store exponent
	_asm xor eax, eax
	_asm mov [ecx], eax				// zero mantissa low word
	_asm mov eax, ecx				// return eax=this
	_asm ret 4
	trealxfromuint0:
	_asm mov [ecx], edx
	_asm mov [ecx+4], edx			// store mantissa high word=0
	_asm mov [ecx+8], edx			// store sign/exponent=0
	_asm mov eax, ecx				// return eax=this
	_asm ret 4
	}




__NAKED__ EXPORT_C TRealX::TRealX(TUint /*aInt*/)
/**
Constructs an extended precision object from an unsigned integer value.

@param aInt The unsigned integer value.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return eax=this
	_asm jmp __6TRealXui
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator=(TUint /*aInt*/)
/**
Assigns the specified unsigned integer value to this extended precision object.

@param aInt The unsigned integer value.

@return A reference to this extended precision object.
*/
	{
	// on entry ecx=this, [esp+4]=aInt, return eax=this
	_asm jmp __6TRealXui
	}




__NAKED__ LOCAL_C void __6TRealXRC6TInt64()
	{
	// common function for TInt64 to TRealX
	_asm push ebx				// preserve ebx
	_asm push ecx				// save this
	_asm mov edx, [esp+12]		// edx=address of aInt
	_asm mov ebx, [edx]
	_asm mov edx, [edx+4]		// edx:ebx=aInt
	_asm call TRealXFromTInt64	// convert to TRealX in ecx,edx:ebx
	_asm pop eax				// eax=this
	_asm mov [eax], ebx			// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm pop ebx				// restore ebx
	_asm ret 4					// return this in eax
	}




__NAKED__ EXPORT_C TRealX::TRealX(const TInt64& /*aInt*/)
/**
Constructs an extended precision object from a 64 bit integer.

@param aInt A reference to a 64 bit integer. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aInt, return eax=this
	_asm jmp __6TRealXRC6TInt64
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator=(const TInt64& /*aInt*/)
/**
Assigns the specified 64 bit integer value to this extended precision object.

@param aInt A reference to a 64 bit integer. 

@return A reference to this extended precision object.
*/
	{
	// on entry ecx=this, [esp+4]=address of aInt, return eax=this
	_asm jmp __6TRealXRC6TInt64
	}




__NAKED__ LOCAL_C void ConvertTReal32ToTRealX(void)
	{
	// Convert TReal32 in edx to TRealX in ecx:edx,ebx
	_asm xor ebx, ebx			// mant low always zero
	_asm mov eax, edx
	_asm shr eax, 23			// exponent now in al, sign in ah bit 0
	_asm test al, al			// check for denormal/zero
	_asm jz short treal32totrealx2	// branch if denormal/zero
	_asm xor ecx, ecx
	_asm mov cl, al
	_asm add ecx, 0x7F80		// bias exponent correctly for TRealX
	_asm cmp al, 0xFF			// check for infinity/NaN
	_asm jnz short treal32totrealx1	// skip if neither
	_asm mov cl, al				// else set TRealX exponent to FFFF
	_asm mov ch, al
	treal32totrealx1:
	_asm shl edx, 8				// left-justify mantissa in edx
	_asm or edx, 0x80000000		// put in implied integer bit
	_asm shl ecx, 16			// exponent into ecx bits 16-31
	_asm mov cl, ah				// sign into ecx bit 0
	_asm ret
	treal32totrealx2:			// come here if exponent 0
	_asm shl edx, 9				// left-justify mantissa in edx (shift out integer bit as well)
	_asm jnz short treal32totrealx3	// jump if denormal
	_asm xor ecx, ecx			// else return 0
	_asm mov cl, ah				// with same sign as input value
	_asm ret
	treal32totrealx3:			// come here if denormal
	_asm bsr ecx, edx			// ecx=bit number of MSB of edx
	_asm neg ecx
	_asm add ecx, 31			// ecx=number of left shifts to normalise edx
	_asm shl edx, cl			// normalise
	_asm neg ecx
	_asm add ecx, 0x7F80			// exponent=7F80-number of shifts
	_asm shl ecx, 16			// exponent into ecx bits 16-31
	_asm mov cl, ah				// sign into ecx bit 0
	_asm ret
	}

__NAKED__ LOCAL_C void ConvertTReal64ToTRealX(void)
	{
	// Convert TReal64 in edx:ebx to TRealX in ecx:edx,ebx
	_asm mov eax, edx
	_asm shr eax, 20
	_asm mov ecx, 0x7FF
	_asm and ecx, eax				// ecx=exponent
	_asm jz short treal64totrealx1	// branch if zero/denormal
	_asm add ecx, 0x7C00			// else bias exponent correctly for TRealX
	_asm cmp ecx, 0x83FF			// check for infinity/NaN
	_asm jnz short treal64totrealx2
	_asm mov ch, cl					// if so, set exponent to FFFF
	treal64totrealx2:
	_asm shl ecx, 16				// exponent into ecx bits 16-31
	_asm mov cl, 11					// number of shifts needed to justify mantissa correctly
	_asm shld edx, ebx, cl			// shift mantissa left
	_asm shl ebx, cl
	_asm or edx, 0x80000000			// put in implied integer bit
	_asm shr eax, 11				// sign bit into al bit 0
	_asm mov cl, al					// into ecx bit 0
	_asm ret
	treal64totrealx1:				// come here if zero/denormal
	_asm mov cl, 12					// number of shifts needed to justify mantissa correctly
	_asm shld edx, ebx, cl			// shift mantissa left
	_asm shl ebx, cl
	_asm test edx, edx				// check for zero
	_asm jnz short treal64totrealx3
	_asm test ebx, ebx
	_asm jnz short treal64totrealx4
	_asm shr eax, 11				// sign bit into eax bit 0, rest of eax=0
	_asm mov ecx, eax				// return 0 result with correct sign
	_asm ret
	treal64totrealx4:				// come here if denormal, edx=0
	_asm mov edx, ebx				// shift mantissa left 32
	_asm xor ebx, ebx
	_asm bsr ecx, edx				// ecx=bit number of MSB of edx
	_asm neg ecx
	_asm add ecx, 31				// ecx=number of left shifts to normalise edx
	_asm shl edx, cl				// normalise
	_asm neg ecx
	_asm add ecx, 0x7BE0			// exponent=7BE0-number of shifts
	_asm shl ecx, 16				// exponent into bits 16-31 of ecx
	_asm shr eax, 11
	_asm mov cl, al					// sign into bit 0 of ecx
	_asm ret
	treal64totrealx3:				// come here if denormal, edx nonzero
	_asm bsr ecx, edx				// ecx=bit number of MSB of edx
	_asm neg ecx
	_asm add ecx, 31				// ecx=number of left shifts to normalise edx:ebx
	_asm shld edx, ebx, cl			// normalise
	_asm shl ebx, cl
	_asm neg ecx
	_asm add ecx, 0x7C00				// exponent=7C00-number of shifts
	_asm shl ecx, 16				// exponent into bits 16-31 of ecx
	_asm shr eax, 11
	_asm mov cl, al					// sign into bit 0 of ecx
	_asm ret
	}




__NAKED__ EXPORT_C TInt TRealX::Set(TReal32 /*aReal*/)
/**
Gives this extended precision object a new value taken from
a single precision floating point number.

@param aReal The single precision floating point value. 

@return KErrNone, if a valid number;
        KErrOverflow, if the number is infinite;
        KErrArgument, if not a number.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4]
	// on exit, error code in eax
	_asm push ebx					// save ebx
	_asm push ecx					// save this
	_asm mov edx, [esp+12]			// aReal into edx
	_asm call ConvertTReal32ToTRealX
	_asm pop eax					// eax=this
	_asm mov [eax], ebx				// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm xor eax, eax				// error code=KErrNone initially
	_asm cmp ecx, 0xFFFF0000		// check for infinity/NaN
	_asm jb short trealxsettreal32a	// if neither, return KErrNone
	_asm mov eax, -9				// eax=KErrOverflow
	_asm cmp edx, 0x80000000			// check for infinity
	_asm je short trealxsettreal32a	// if infinity, return KErrOverflow
	_asm mov eax, -6				// if NaN, return KErrArgument
	trealxsettreal32a:
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C TInt TRealX::Set(TReal64 /*aReal*/)
/**
Gives this extended precision object a new value taken from
a double precision floating point number.

@param aReal The double precision floating point value. 

@return KErrNone, if a valid number;
        KErrOverflow, if the number is infinite;
        KErrArgument, if not a number.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4] (mant low) and [esp+8] (sign/exp/mant high)
	// on exit, error code in eax
	_asm push ebx				// save ebx
	_asm push ecx				// save this
	_asm mov ebx, [esp+12]		// aReal into edx:ebx
	_asm mov edx, [esp+16]
	_asm call ConvertTReal64ToTRealX
	_asm pop eax				// eax=this
	_asm mov [eax], ebx			// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm xor eax, eax				// error code=KErrNone initially
	_asm cmp ecx, 0xFFFF0000		// check for infinity/NaN
	_asm jb short trealxsettreal64a	// if neither, return KErrNone
	_asm mov eax, -9				// eax=KErrOverflow
	_asm cmp edx, 0x80000000			// check for infinity
	_asm jne short trealxsettreal64b	// branch if NaN
	_asm test ebx, ebx
	_asm je short trealxsettreal64a		// if infinity, return KErrOverflow
	trealxsettreal64b:
	_asm mov eax, -6				// if NaN, return KErrArgument
	trealxsettreal64a:
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ LOCAL_C void __6TRealXf()
	{
	// common function for float to TRealX
	_asm push ebx				// save ebx
	_asm push ecx				// save this
	_asm mov edx, [esp+12]		// aReal into edx
	_asm call ConvertTReal32ToTRealX
	_asm pop eax				// eax=this
	_asm mov [eax], ebx			// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C TRealX::TRealX(TReal32 /*aReal*/)
/**
Constructs an extended precision object from
a single precision floating point number.

@param aReal The single precision floating point value.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4]
	// on exit, eax=this
	_asm jmp __6TRealXf
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator=(TReal32 /*aReal*/)
/**
Assigns the specified single precision floating point number to
this extended precision object.

@param aReal The single precision floating point value.

@return A reference to this extended precision object.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4]
	// on exit, eax=this
	_asm jmp __6TRealXf
	}




__NAKED__ LOCAL_C void __6TRealXd()
	{
	// common function for double to TRealX
	_asm push ebx				// save ebx
	_asm push ecx				// save this
	_asm mov ebx, [esp+12]		// aReal into edx:ebx
	_asm mov edx, [esp+16]
	_asm call ConvertTReal64ToTRealX
	_asm pop eax				// eax=this
	_asm mov [eax], ebx			// store result
	_asm mov [eax+4], edx
	_asm mov [eax+8], ecx
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX::TRealX(TReal64 /*aReal*/)
/**
Constructs an extended precision object from
a double precision floating point number.

@param aReal The double precision floating point value.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4] (mant low) and [esp+8] (sign/exp/mant high)
	// on exit, eax=this
	_asm jmp __6TRealXd
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator=(TReal64 /*aReal*/)
/**
Assigns the specified double precision floating point number to
this extended precision object.

@param aReal The double precision floating point value.

@return A reference to this extended precision object.
*/
	{
	// on entry, ecx=this and aReal is in [esp+4] (mant low) and [esp+8] (sign/exp/mant high)
	// on exit, eax=this
	_asm jmp __6TRealXd
	}




__NAKED__ EXPORT_C TRealX::operator TInt() const
/**
Gets the extended precision value as a signed integer value.

The operator returns:

1. zero , if the extended precision value is not a number

2. 0x7FFFFFFF, if the value is positive and too big to fit into a TInt.

3. 0x80000000, if the value is negative and too big to fit into a TInt.
*/
	{
	// on entry ecx=this, return value in eax
	_asm mov edx, [ecx]			// edx=mantissa low
	_asm mov eax, [ecx+4]		// eax=mantissa high
	_asm mov ecx, [ecx+8]		// ecx=exponent/sign
	_asm ror ecx, 16			// exponent into cx
	_asm cmp cx, 0xFFFF
	_asm jz short trealxtoint1	// branch if exp=FFFF
	_asm mov dx, cx
	_asm mov cx, 0x801E
	_asm sub cx, dx				// cx=number of right shifts needed to convert mantissa to int
	_asm jbe short trealxtoint2	// if exp>=801E, saturate result
	_asm cmp cx, 31				// more than 31 shifts needed?
	_asm ja short trealxtoint0	// if so, underflow to zero
	_asm shr eax, cl			// else ABS(result)=eax>>cl
	_asm test ecx, 0x10000		// test sign
	_asm jz short trealxtoint3	// skip if +
	_asm neg eax
	trealxtoint3:
	_asm ret
	trealxtoint1:			// come here if exponent=FFFF
	_asm cmp eax, 0x80000000		// check for infinity
	_asm jnz short trealxtoint0	// if NaN, return 0
	_asm test edx, edx
	_asm jnz short trealxtoint0	// if NaN, return 0
	trealxtoint2:			// come here if argument too big for 32-bit integer
	_asm mov eax, 0x7FFFFFFF
	_asm shr ecx, 17			// sign bit into carry flag
	_asm adc eax, 0				// eax=7FFFFFFF if +, 80000000 if -
	_asm ret					// return saturated value
	trealxtoint0:			// come here if INT(argument)=0 or NaN
	_asm xor eax, eax			// return 0
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::operator TUint() const
/**
Returns the extended precision value as an unsigned signed integer value.

The operator returns:

1. zero, if the extended precision value is not a number

2. 0xFFFFFFFF, if the value is positive and too big to fit into a TUint.

3. zero, if the value is negative and too big to fit into a TUint.
*/
	{
	// on entry ecx=this, return value in eax
	_asm mov edx, [ecx]			// edx=mantissa low
	_asm mov eax, [ecx+4]		// eax=mantissa high
	_asm mov ecx, [ecx+8]		// ecx=exponent/sign
	_asm ror ecx, 16			// exponent into cx
	_asm cmp cx, 0xFFFF
	_asm jz short trealxtouint1	// branch if exp=FFFF
	_asm mov dx, cx
	_asm mov cx, 0x801E
	_asm sub cx, dx				// cx=number of right shifts needed to convert mantissa to int
	_asm jb short trealxtouint2	// if exp>801E, saturate result
	_asm cmp cx, 31				// more than 31 shifts needed?
	_asm ja short trealxtouint0	// if so, underflow to zero
	_asm test ecx, 0x10000		// test sign
	_asm jnz short trealxtouint0	// if -, return 0
	_asm shr eax, cl			// else result=eax>>cl
	_asm ret
	trealxtouint1:			// come here if exponent=FFFF
	_asm cmp eax, 0x80000000		// check for infinity
	_asm jnz short trealxtouint0	// if NaN, return 0
	_asm test edx, edx
	_asm jnz short trealxtouint0	// if NaN, return 0
	trealxtouint2:			// come here if argument too big for 32-bit integer
	_asm mov eax, 0xFFFFFFFF
	_asm shr ecx, 17			// sign bit into carry flag
	_asm adc eax, 0				// eax=FFFFFFFF if +, 0 if -
	_asm ret					// return saturated value
	trealxtouint0:			// come here if INT(argument)=0 or NaN
	_asm xor eax, eax			// return 0
	_asm ret
	}




__NAKED__ LOCAL_C void ConvertTRealXToTInt64(void)
	{
	// Convert TRealX in ecx,edx:ebx to TInt64 in edx:ebx
	_asm ror ecx, 16				// exponent into cx
	_asm cmp cx, 0xFFFF
	_asm jz short trealxtoint64a	// branch if exp=FFFF
	_asm mov ax, cx
	_asm mov cx, 0x803E
	_asm sub cx, ax					// cx=number of right shifts needed to convert mantissa to int
	_asm jbe short trealxtoint64b	// if exp>=803E, saturate result
	_asm cmp cx, 63					// more than 63 shifts needed?
	_asm ja short trealxtoint64z	// if so, underflow to zero
	_asm cmp cl, 31					// more than 31 shifts needed?
	_asm jbe short trealxtoint64d	// branch if not
	_asm sub cl, 32					// cl=shift count - 32
	_asm mov ebx, edx				// shift right by 32
	_asm xor edx, edx
	trealxtoint64d:
	_asm shrd ebx, edx, cl			// shift edx:ebx right by cl to give ABS(result)
	_asm shr edx, cl
	_asm test ecx, 0x10000			// test sign
	_asm jz short trealxtoint64c	// skip if +
	_asm neg edx					// if -, negate
	_asm neg ebx
	_asm sbb edx, 0
	trealxtoint64c:
	_asm ret
	trealxtoint64a:			// come here if exponent=FFFF
	_asm cmp edx, 0x80000000			// check for infinity
	_asm jnz short trealxtoint64z	// if NaN, return 0
	_asm test ebx, ebx
	_asm jnz short trealxtoint64z	// if NaN, return 0
	trealxtoint64b:			// come here if argument too big for 32-bit integer
	_asm mov edx, 0x7FFFFFFF
	_asm mov ebx, 0xFFFFFFFF
	_asm shr ecx, 17				// sign bit into carry flag
	_asm adc ebx, 0					// edx:ebx=7FFFFFFF FFFFFFFF if +,
	_asm adc edx, 0					// or 80000000 00000000 if -
	_asm ret						// return saturated value
	trealxtoint64z:			// come here if INT(argument)=0 or NaN
	_asm xor edx, edx				// return 0
	_asm xor ebx, ebx
	_asm ret
	}




/**
Returns the extended precision value as a 64 bit integer value.

The operator returns:

1. zero, if the extended precision value is not a number

2. 0x7FFFFFFF FFFFFFFF, if the value is positive and too big to fit
   into a TInt64

3. 0x80000000 00000000, if the value is negative and too big to fit
   into a TInt64.
*/
__NAKED__ EXPORT_C TRealX::operator TInt64() const
	{
	// on entry, ecx=this, return value in edx:eax
	_asm push ebx
	_asm mov ebx, [ecx]			// get TRealX value into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call ConvertTRealXToTInt64
	_asm mov eax, ebx			// store low result into eax
	_asm pop ebx
	_asm ret
	}




__NAKED__ LOCAL_C void TRealXGetTReal32(void)
	{
	// Convert TRealX in ecx,edx:ebx to TReal32 in edx
	// Return error code in eax
	_asm cmp ecx, 0xFFFF0000		// check for infinity/NaN
	_asm jnc short trealxgettreal32a
	_asm xor eax, eax
	_asm ror ecx, 16				// exponent into cx
	_asm sub cx, 0x7F80				// cx=result exponent if normalised
	_asm jbe short trealxgettreal32b	// jump if denormal, zero or underflow
	_asm cmp cx, 0xFF				// check if overflow
	_asm jb short trealxgettreal32c	// jump if not
	trealxgettreal32d:			// come here if overflow
	_asm xor edx, edx				// set mantissa=0 to generate infinity
	_asm ror ecx, 16				// ecx back to normal format
	trealxgettreal32a:			// come here if infinity or NaN
	_asm shr edx, 7
	_asm or edx, 0xFF000000			// set exponent to FF
	_asm shr ecx, 1					// sign bit -> carry
	_asm rcr edx, 1					// sign bit -> MSB of result
	_asm mov eax, edx
	_asm shl eax, 9					// test for infinity or NaN
	_asm mov eax, -9				// eax=KErrOverflow
	_asm jz short trealxgettreal32e
	_asm mov eax, -6				// if NaN, eax=KErrArgument
	trealxgettreal32e:
	_asm ret
	trealxgettreal32b:			// come here if exponent<=7F80
	_asm cmp cx, -24				// check for zero or total underflow
	_asm jle short trealxgettreal32z
	_asm neg cl
	_asm inc cl						// cl=number of right shifts to form denormal mantissa
	_asm shrd eax, ebx, cl			// shift mantissa right into eax
	_asm shrd ebx, edx, cl
	_asm shr edx, cl
	_asm or edx, 0x80000000			// set top bit to ensure correct rounding up
	_asm xor cl, cl					// cl=result exponent=0
	trealxgettreal32c:			// come here if result normalised
	_asm cmp dl, 0x80				// check rounding bits
	_asm ja short trealxgettreal32f	// branch to round up
	_asm jb short trealxgettreal32g	// branch to round down
	_asm test ebx, ebx
	_asm jnz short trealxgettreal32f	// branch to round up
	_asm test eax, eax
	_asm jnz short trealxgettreal32f	// branch to round up
	_asm test ecx, 0x01000000			// check rounded-down flag
	_asm jnz short trealxgettreal32f	// branch to round up
	_asm test ecx, 0x02000000			// check rounded-up flag
	_asm jnz short trealxgettreal32g	// branch to round down
	_asm test dh, 1						// else round to even
	_asm jz short trealxgettreal32g		// branch to round down if LSB=0
	trealxgettreal32f:				// come here to round up
	_asm add edx, 0x100					// increment mantissa
	_asm jnc short trealxgettreal32g
	_asm rcr edx, 1
	_asm inc cl							// if carry, increment exponent
	_asm cmp cl, 0xFF					// and check for overflow
	_asm jz short trealxgettreal32d		// branch out if overflow
	trealxgettreal32g:				// come here to round down
	_asm xor dl, dl
	_asm add edx, edx					// shift out integer bit
	_asm mov dl, cl
	_asm ror edx, 8						// exponent->edx bits 24-31, mantissa in 23-1
	_asm test edx, edx					// check if underflow
	_asm jz short trealxgettreal32h		// branch out if underflow
	_asm shr ecx, 17					// sign bit->carry
	_asm rcr edx, 1						// ->edx bit 31, exp->edx bits 23-30, mant->edx bits 22-0
	_asm xor eax, eax					// return KErrNone
	_asm ret
	trealxgettreal32z:				// come here if zero or underflow
	_asm xor eax, eax
	_asm cmp cx, 0x8080					// check for zero
	_asm jz short trealxgettreal32y		// if zero, return KErrNone
	trealxgettreal32h:				// come here if underflow after rounding
	_asm mov eax, -10					// eax=KErrUnderflow
	trealxgettreal32y:
	_asm xor edx, edx
	_asm shr ecx, 17
	_asm rcr edx, 1						// sign bit into edx bit 31, rest of edx=0
	_asm ret
	}




__NAKED__ LOCAL_C void TRealXGetTReal64(void)
	{
	// Convert TRealX in ecx,edx:ebx to TReal64 in edx:ebx
	// Return error code in eax
	// edi, esi also modified
	_asm ror ecx, 16				// exponent into cx
	_asm cmp cx, 0xFFFF				// check for infinity/NaN
	_asm jnc short trealxgettreal64a
	_asm xor eax, eax
	_asm xor edi, edi
	_asm sub cx, 0x7C00				// cx=result exponent if normalised
	_asm jbe short trealxgettreal64b	// jump if denormal, zero or underflow
	_asm cmp cx, 0x07FF				// check if overflow
	_asm jb short trealxgettreal64c	// jump if not
	trealxgettreal64d:			// come here if overflow
	_asm xor edx, edx				// set mantissa=0 to generate infinity
	_asm xor ebx, ebx
	trealxgettreal64a:			// come here if infinity or NaN
	_asm mov cl, 10
	_asm shrd ebx, edx, cl
	_asm shr edx, cl
	_asm or edx, 0xFFE00000			// set exponent to 7FF
	_asm shr ecx, 17				// sign bit -> carry
	_asm rcr edx, 1					// sign bit -> MSB of result
	_asm rcr ebx, 1
	_asm mov eax, edx
	_asm shl eax, 12				// test for infinity or NaN
	_asm mov eax, -9				// eax=KErrOverflow
	_asm jnz short trealxgettreal64n
	_asm test ebx, ebx
	_asm jz short trealxgettreal64e
	trealxgettreal64n:
	_asm mov eax, -6				// if NaN, eax=KErrArgument
	trealxgettreal64e:
	_asm ret
	trealxgettreal64b:			// come here if exponent<=7C00
	_asm cmp cx, -53				// check for zero or total underflow
	_asm jle trealxgettreal64z
	_asm neg cl
	_asm inc cl						// cl=number of right shifts to form denormal mantissa
	_asm cmp cl, 32
	_asm jb trealxgettreal64x
	_asm mov eax, ebx				// if >=32 shifts, do 32 shifts and decrement count by 32
	_asm mov ebx, edx
	_asm xor edx, edx
	trealxgettreal64x:
	_asm shrd edi, eax, cl
	_asm shrd eax, ebx, cl			// shift mantissa right into eax
	_asm shrd ebx, edx, cl
	_asm shr edx, cl
	_asm or edx, 0x80000000			// set top bit to ensure correct rounding up
	_asm xor cx, cx					// cx=result exponent=0
	trealxgettreal64c:			// come here if result normalised
	_asm mov esi, ebx
	_asm and esi, 0x7FF				// esi=rounding bits
	_asm cmp esi, 0x400				// check rounding bits
	_asm ja short trealxgettreal64f	// branch to round up
	_asm jb short trealxgettreal64g	// branch to round down
	_asm test eax, eax
	_asm jnz short trealxgettreal64f	// branch to round up
	_asm test edi, edi
	_asm jnz short trealxgettreal64f	// branch to round up
	_asm test ecx, 0x01000000			// check rounded-down flag
	_asm jnz short trealxgettreal64f	// branch to round up
	_asm test ecx, 0x02000000			// check rounded-up flag
	_asm jnz short trealxgettreal64g	// branch to round down
	_asm test ebx, 0x800					// else round to even
	_asm jz short trealxgettreal64g		// branch to round down if LSB=0
	trealxgettreal64f:				// come here to round up
	_asm add ebx, 0x800					// increment mantissa
	_asm adc edx, 0
	_asm jnc short trealxgettreal64g
	_asm rcr edx, 1
	_asm inc cx							// if carry, increment exponent
	_asm cmp cx, 0x7FF					// and check for overflow
	_asm jz trealxgettreal64d			// branch out if overflow
	trealxgettreal64g:				// come here to round down
	_asm xor bl, bl						// clear rounding bits
	_asm and bh, 0xF8
	_asm mov di, cx						// save exponent
	_asm mov cl, 10
	_asm and edx, 0x7FFFFFFF				// clear integer bit
	_asm shrd ebx, edx, cl				// shift mantissa right by 10
	_asm shr edx, cl
	_asm shl edi, 21					// exponent into edi bits 21-31
	_asm or edx, edi					// into edx bits 21-31
	_asm test edx, edx					// check if underflow
	_asm jnz short trealxgettreal64i
	_asm test ebx, ebx
	_asm jz short trealxgettreal64h		// branch out if underflow
	trealxgettreal64i:
	_asm shr ecx, 17					// sign bit->carry
	_asm rcr edx, 1						// ->edx bit 31, exp->edx bits 20-30, mant->edx bits 20-0
	_asm rcr ebx, 1
	_asm xor eax, eax					// return KErrNone
	_asm ret
	trealxgettreal64z:				// come here if zero or underflow
	_asm xor eax, eax
	_asm cmp cx, 0x8400					// check for zero
	_asm jz short trealxgettreal64y		// if zero, return KErrNone
	trealxgettreal64h:				// come here if underflow after rounding
	_asm mov eax, -10					// eax=KErrUnderflow
	trealxgettreal64y:
	_asm xor edx, edx
	_asm xor ebx, ebx
	_asm shr ecx, 17
	_asm rcr edx, 1						// sign bit into edx bit 31, rest of edx=0, ebx=0
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::operator TReal32() const
/**
Returns the extended precision value as
a single precision floating point value.
*/
	{
	// On entry, ecx=this
	// On exit, TReal32 value on top of FPU stack
	_asm push ebx
	_asm mov ebx, [ecx]			// *this into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXGetTReal32	// Convert to TReal32 in edx
	_asm push edx				// push TReal32 onto stack
	_asm fld dword ptr [esp]	// push TReal32 onto FPU stack
	_asm pop edx
	_asm pop ebx
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::operator TReal64() const
/**
Returns the extended precision value as
a double precision floating point value.
*/
	{
	// On entry, ecx=this
	// On exit, TReal64 value on top of FPU stack
	_asm push ebx
	_asm push esi
	_asm push edi
	_asm mov ebx, [ecx]			// *this into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXGetTReal64	// Convert to TReal32 in edx:ebx
	_asm push edx				// push TReal64 onto stack
	_asm push ebx
	_asm fld qword ptr [esp]	// push TReal64 onto FPU stack
	_asm add esp, 8
	_asm pop edi
	_asm pop esi
	_asm pop ebx
	_asm ret
	}




__NAKED__ EXPORT_C TInt TRealX::GetTReal(TReal32& /*aVal*/) const
/**
Extracts the extended precision value as
a single precision floating point value.

@param aVal A reference to a single precision object which contains
            the result of the operation.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow.
*/
	{
	// On entry, ecx=this, [esp+4]=address of aVal
	// On exit, eax=return code
	_asm push ebx
	_asm mov ebx, [ecx]			// *this into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXGetTReal32
	_asm mov ecx, [esp+8]		// ecx=address of aVal
	_asm mov [ecx], edx			// store result
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::GetTReal(TReal64& /*aVal*/) const
/**
Extracts the extended precision value as
a double precision floating point value.

@param aVal A reference to a double precision object which
            contains the result of the operation.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow.
*/
	{
	// On entry, ecx=this, [esp+4]=address of aVal
	// On exit, eax=return code
	_asm push ebx
	_asm push esi
	_asm push edi
	_asm mov ebx, [ecx]			// *this into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXGetTReal64
	_asm mov ecx, [esp+16]		// ecx=address of aVal
	_asm mov [ecx], ebx			// store result
	_asm mov [ecx+4], edx
	_asm pop edi
	_asm pop esi
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C void TRealX::SetZero(TBool /*aNegative*/)
/**
Sets the value of this extended precision object to zero.

@param aNegative ETrue, the value is a negative zero;
                 EFalse, the value is a positive zero, this is the default.
*/
	{
	_asm mov edx, [esp+4]		// aNegative into edx
	_asm xor eax, eax			// eax=0
	_asm mov [ecx], eax
	_asm mov [ecx+4], eax
	_asm test edx, edx
	_asm jz short setzero1
	_asm inc eax				// eax=1 if aNegative!=0
	setzero1:
	_asm mov [ecx+8], eax		// generate positive or negative zero
	_asm ret 4
	}




__NAKED__ EXPORT_C void TRealX::SetNaN()
/**
Sets the value of this extended precision object to 'not a number'.
*/
	{
	_asm xor eax, eax			// set *this to 'real indefinite'
	_asm mov [ecx], eax
	_asm mov eax, 0xC0000000
	_asm mov [ecx+4], eax
	_asm mov eax, 0xFFFF0001
	_asm mov [ecx+8], eax
	_asm ret
	}




__NAKED__ EXPORT_C void TRealX::SetInfinite(TBool /*aNegative*/)
/**
Sets the value of this extended precision object to infinity.

@param aNegative ETrue, the value is a negative zero;
                 EFalse, the value is a positive zero.
*/
	{
	_asm mov edx, [esp+4]		// aNegative into edx
	_asm mov eax, 0xFFFF0000	// exponent=FFFF, sign=0 initially
	_asm test edx, edx
	_asm jz short setinf1
	_asm inc eax				// sign=1 if aNegative!=0
	setinf1:
	_asm mov [ecx+8], eax		// generate positive or negative infinity
	_asm mov eax, 0x80000000
	_asm mov [ecx+4], eax
	_asm xor eax, eax
	_asm mov [ecx], eax
	_asm ret 4
	}




__NAKED__ EXPORT_C TBool TRealX::IsZero() const
/**
Determines whether the extended precision value is zero.

@return True, if the extended precision value is zero, false, otherwise.
*/
	{
	_asm mov eax, [ecx+8]		// check exponent
	_asm shr eax, 16			// move exponent into ax
	_asm jz short iszero1		// branch if zero
	_asm xor eax, eax			// else return 0
	_asm ret
	iszero1:
	_asm inc eax				// if zero, return 1
	_asm ret
	}




__NAKED__ EXPORT_C TBool TRealX::IsNaN() const
/**
Determines whether the extended precision value is 'not a number'.

@return True, if the extended precision value is 'not a number',
        false, otherwise.
*/
	{
	_asm mov eax, [ecx+8]		// check exponent
	_asm cmp eax, 0xFFFF0000
	_asm jc short isnan0		// branch if not FFFF
	_asm mov eax, [ecx+4]
	_asm cmp eax, 0x80000000		// check for infinity
	_asm jne short isnan1
	_asm mov eax, [ecx]
	_asm test eax, eax
	_asm jne short isnan1
	isnan0:
	_asm xor eax, eax			// return 0 if not NaN
	_asm ret
	isnan1:
	_asm mov eax, 1				// return 1 if NaN
	_asm ret
	}




__NAKED__ EXPORT_C TBool TRealX::IsInfinite() const
/**
Determines whether the extended precision value has a finite value.

@return True, if the extended precision value is finite,
        false, if the value is 'not a number' or is infinite,
*/
	{
	_asm mov eax, [ecx+8]		// check exponent
	_asm cmp eax, 0xFFFF0000
	_asm jc short isinf0		// branch if not FFFF
	_asm mov eax, [ecx+4]
	_asm cmp eax, 0x80000000		// check for infinity
	_asm jne short isinf0
	_asm mov eax, [ecx]
	_asm test eax, eax
	_asm jne short isinf0
	_asm inc eax				// return 1 if infinity
	_asm ret
	isinf0:
	_asm xor eax, eax			// return 0 if not infinity
	_asm ret
	}




__NAKED__ EXPORT_C TBool TRealX::IsFinite() const
/**
Determines whether the extended precision value has a finite value.

@return True, if the extended precision value is finite,
        false, if the value is 'not a number' or is infinite,
*/
	{
	_asm mov eax, [ecx+8]		// check exponent
	_asm cmp eax, 0xFFFF0000	// check for NaN or infinity
	_asm jnc short isfinite0	// branch if NaN or infinity
	_asm mov eax, 1				// return 1 if finite
	_asm ret
	isfinite0:
	_asm xor eax, eax			// return 0 if NaN or infinity
	_asm ret
	}




__NAKED__ EXPORT_C const TRealX& TRealX::operator+=(const TRealX& /*aVal*/)
/**
Adds an extended precision value to this extended precision number.

@param aVal The extended precision value to be added.

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXAdd			// do addition, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return this in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C const TRealX& TRealX::operator-=(const TRealX& /*aVal*/)
/**
Subtracts an extended precision value from this extended precision number. 

@param aVal The extended precision value to be subtracted.

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXSubtract	// do subtraction, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return this in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C const TRealX& TRealX::operator*=(const TRealX& /*aVal*/)
/**
Multiplies this extended precision number by an extended precision value.

@param aVal The extended precision value to be subtracted.

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXMultiply	// do multiplication, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return this in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C const TRealX& TRealX::operator/=(const TRealX& /*aVal*/)
/**
Divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor. 

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
@panic MATHX KErrDivideByZero if the divisor is zero.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXDivide		// do division, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return this in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C const TRealX& TRealX::operator%=(const TRealX& /*aVal*/)
/**
Modulo-divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor. 

@return A reference to this object.

@panic MATHX KErrTotalLossOfPrecision panic if precision is lost.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXModulo		// do modulo, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return this in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




__NAKED__ EXPORT_C TInt TRealX::AddEq(const TRealX& /*aVal*/)
/**
Adds an extended precision value to this extended precision number.

@param aVal The extended precision value to be added.

@return KErrNone, if the operation is successful;
        KErrOverflow,if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXAdd			// do addition, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::SubEq(const TRealX& /*aVal*/)
/**
Subtracts an extended precision value from this extended precision number.

@param aVal The extended precision value to be subtracted.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXSubtract	// do subtraction, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::MultEq(const TRealX& /*aVal*/)
/**
Multiplies this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the multiplier.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXMultiply	// do multiplication, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::DivEq(const TRealX& /*aVal*/)
/**
Divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow;
        KErrDivideByZero, if the divisor is zero. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXDivide		// do division, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::ModEq(const TRealX& /*aVal*/)
/**
Modulo-divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor. 

@return KErrNone, if the operation is successful;
        KErrTotalLossOfPrecision, if precision is lost;
        KErrUnderflow, if the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+20]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXModulo		// do modulo, result in ecx,edx:ebx, error code in eax
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4					// return with error code in eax
	}




__NAKED__ EXPORT_C TRealX TRealX::operator+() const
/**
Returns this extended precision number unchanged.

Note that this may also be referred to as a unary plus operator. 

@return The extended precision number.
*/
	{
	_asm mov eax, [esp+4]		// eax=address to write return value
	_asm mov edx, [ecx]
	_asm mov [eax], edx
	_asm mov edx, [ecx+4]
	_asm mov [eax+4], edx
	_asm mov edx, [ecx+8]
	_asm mov [eax+8], edx
	_asm ret 4					// return address of return value in eax
	}




__NAKED__ EXPORT_C TRealX TRealX::operator-() const
/**
Negates this extended precision number.

This may also be referred to as a unary minus operator.

@return The negative of the extended precision number.
*/
	{
	_asm mov eax, [esp+4]		// eax=address to write return value
	_asm mov edx, [ecx]
	_asm mov [eax], edx
	_asm mov edx, [ecx+4]
	_asm mov [eax+4], edx
	_asm mov edx, [ecx+8]
	_asm xor dl, 1				// change sign bit
	_asm mov [eax+8], edx
	_asm ret 4					// return address of return value in eax
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator++()
/**
Increments this extended precision number by one,
and then returns a reference to it.

This is also referred to as a prefix operator. 

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// pre-increment
	// on entry ecx=this, return this in eax
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, 0x7FFF0000	// set ecx,edx:ebx to 1.0
	_asm mov edx, 0x80000000
	_asm xor ebx, ebx
	_asm call TRealXAdd			// add 1 to *this
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax			// check error code
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// else return this in eax
	_asm pop edi
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret
	}




__NAKED__ EXPORT_C TRealX TRealX::operator++(TInt)
/**
Returns this extended precision number before incrementing it by one.

This is also referred to as a postfix operator. 

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// post-increment
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=dummy int
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov edi, [esp+20]		// address of return value into edi
	_asm mov eax, [ecx]			// copy initial value of *this into [edi]
	_asm mov [edi], eax
	_asm mov eax, [ecx+4]
	_asm mov [edi+4], eax
	_asm mov eax, [ecx+8]
	_asm mov [edi+8], eax
	_asm mov ecx, 0x7FFF0000	// set ecx,edx:ebx to 1.0
	_asm mov edx, 0x80000000
	_asm xor ebx, ebx
	_asm call TRealXAdd			// add 1 to *this
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax			// check error code
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, [esp+20]		// address of return value into eax
	_asm pop edi
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX& TRealX::operator--()
/**
Decrements this extended precision number by one,
and then returns a reference to it.

This is also referred to as a prefix operator. 

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// pre-decrement
	// on entry ecx=this, return this in eax
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, 0x7FFF0001		// set ecx,edx:ebx to -1.0
	_asm mov edx, 0x80000000
	_asm xor ebx, ebx
	_asm call TRealXAdd			// add -1 to *this
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax			// check error code
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// else return this in eax
	_asm pop edi
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret
	}




__NAKED__ EXPORT_C TRealX TRealX::operator--(TInt)
/**
Returns this extended precision number before decrementing it by one.

This is also referred to as a postfix operator. 

@return A reference to this object.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// post-decrement
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=dummy int
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov edi, [esp+20]		// address of return value into edi
	_asm mov eax, [ecx]			// copy initial value of *this into [edi]
	_asm mov [edi], eax
	_asm mov eax, [ecx+4]
	_asm mov [edi+4], eax
	_asm mov eax, [ecx+8]
	_asm mov [edi+8], eax
	_asm mov ecx, 0x7FFF0001		// set ecx,edx:ebx to -1.0
	_asm mov edx, 0x80000000
	_asm xor ebx, ebx
	_asm call TRealXAdd			// add -1 to *this
	_asm mov [esi], ebx			// store result in *this
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax			// check error code
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, [esp+20]		// address of return value into eax
	_asm pop edi
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX TRealX::operator+(const TRealX& /*aVal*/) const
/**
Adds an extended precision value to this extended precision number.

@param aVal The extended precision value to be added. 

@return An extended precision object containing the result.

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXAdd			// do addition, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of return value
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return address of return value in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX TRealX::operator-(const TRealX& /*aVal*/) const
/**
Subtracts an extended precision value from this extended precision number. 

@param aVal The extended precision value to be subtracted. 

@return An extended precision object containing the result. 

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXSubtract	// do subtraction, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of return value
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return address of return value in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX TRealX::operator*(const TRealX& /*aVal*/) const
/**
Multiplies this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the multiplier. 

@return An extended precision object containing the result. 

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXMultiply	// do multiplication, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of return value
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return address of return value in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX TRealX::operator/(const TRealX& /*aVal*/) const
/**
Divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor. 

@return An extended precision object containing the result. 

@panic MATHX KErrOverflow if the operation results in overflow.
@panic MATHX KErrUnderflow if  the operation results in underflow.
@panic MATHX KErrDivideByZero if the divisor is zero.
*/
	{
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXDivide		// do division, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of return value
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return address of return value in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TRealX TRealX::operator%(const TRealX& /*aVal*/) const
/**
Modulo-divides this extended precision number by an extended precision value.

@param aVal The extended precision value to be used as the divisor. 

@return An extended precision object containing the result. 

@panic MATHX KErrTotalLossOfPrecision if precision is lost.
@panic MATHX KErrUnderflow if the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of return value, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXModulo		// do modulo, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of return value
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm test eax, eax
	_ASM_jn(z,TRealXPanicEax)	// panic if error
	_asm mov eax, esi			// return address of return value in eax
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8
	}




__NAKED__ EXPORT_C TInt TRealX::Add(TRealX& /*aResult*/, const TRealX& /*aVal*/) const
/**
Adds an extended precision value to this extended precision number.

@param aResult On return, a reference to an extended precision object
               containing the result of the operation.
@param aVal    The extended precision value to be added. 

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aResult, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXAdd			// do addition, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of aResult
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::Sub(TRealX& /*aResult*/, const TRealX& /*aVal*/) const
/**
Subtracts an extended precision value from this extended precision number.

@param aResult On return, a reference to an extended precision object
               containing the result of the operation.
@param aVal    The extended precision value to be subtracted. 

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aResult, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXSubtract	// do subtraction, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of aResult
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::Mult(TRealX& /*aResult*/, const TRealX& /*aVal*/) const
/**
Multiplies this extended precision number by an extended precision value.

@param aResult On return, a reference to an extended precision object
               containing the result of the operation.
@param aVal    The extended precision value to be used as the multiplier. 

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow. 
*/
	{
	// on entry ecx=this, [esp+4]=address of aResult, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXMultiply	// do multiplication, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of aResult
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::Div(TRealX& /*aResult*/, const TRealX& /*aVal*/) const
/**
Divides this extended precision number by an extended precision value.

@param aResult On return, a reference to an extended precision object
               containing the result of the operation.
@param aVal    The extended precision value to be used as the divisor.

@return KErrNone, if the operation is successful;
        KErrOverflow, if the operation results in overflow;
        KErrUnderflow, if the operation results in underflow;
        KErrDivideByZero, if the divisor is zero.
*/
	{
	// on entry ecx=this, [esp+4]=address of aResult, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXDivide		// do division, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of aResult
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8					// return with error code in eax
	}




__NAKED__ EXPORT_C TInt TRealX::Mod(TRealX& /*aResult*/, const TRealX& /*aVal*/) const
/**
Modulo-divides this extended precision number by an extended precision value.

@param aResult On return, a reference to an extended precision object
               containing the result of the operation.

@param aVal    The extended precision value to be used as the divisor. 

@return KErrNone, if the operation is successful;
        KErrTotalLossOfPrecision, if precision is lost;
        KErrUnderflow, if the operation results in underflow.
*/
	{
	// on entry ecx=this, [esp+4]=address of aResult, [esp+8]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, ecx			// this into esi
	_asm mov ecx, [esp+24]		// address of aVal into ecx
	_asm mov ebx, [ecx]			// aVal into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXModulo		// do modulo, result in ecx,edx:ebx, error code in eax
	_asm mov esi, [esp+20]		// esi=address of aResult
	_asm mov [esi], ebx			// store result
	_asm mov [esi+4], edx
	_asm mov [esi+8], ecx
	_asm pop edi				// restore registers
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 8					// return with error code in eax
	}

// Compare TRealX in ecx,edx:ebx (op1) to TRealX at [esi] (op2)
// Return 1 if op1<op2
// Return 2 if op1=op2
// Return 4 if op1>op2
// Return 8 if unordered
// Return value in eax
__NAKED__ LOCAL_C void TRealXCompare(void)
	{
	_asm cmp ecx, 0xFFFF0000	// check if op1=NaN or infinity
	_asm jc short fpcmp1		// branch if not
	_asm cmp edx, 0x80000000		// check for infinity
	_asm jnz short fpcmpunord	// branch if NaN
	_asm test ebx, ebx
	_asm jz short fpcmp1		// if infinity, process normally
	fpcmpunord:					// come here if unordered
	_asm mov eax, 8				// return 8
	_asm ret
	fpcmp1:						// op1 is not a NaN
	_asm mov eax, [esi+8]		// get op2 into eax,edi:ebp
	_asm mov edi, [esi+4]
	_asm mov ebp, [esi]
	_asm cmp eax, 0xFFFF0000	// check for NaN or infinity
	_asm jc short fpcmp2		// branch if neither
	_asm cmp edi, 0x80000000		// check for infinity
	_asm jnz short fpcmpunord	// branch if NaN
	_asm test ebp, ebp
	_asm jnz short fpcmpunord
	fpcmp2:						// neither operand is a NaN
	_asm cmp ecx, 0x10000		// check if op1=0
	_asm jc short fpcmpop1z		// branch if it is
	_asm cmp eax, 0x10000		// check if op2=0
	_asm jc short fpcmp4		// branch if it is
	_asm xor al, cl				// check if signs the same
	_asm test al, 1
	_asm jnz short fpcmp4		// branch if different
	_asm push ecx
	_asm shr ecx, 16			// op1 exponent into cx
	_asm shr eax, 16			// op2 exponent into ax
	_asm cmp ecx, eax			// compare exponents
	_asm pop ecx
	_asm ja short fpcmp4		// if op1 exp > op2 exp op1>op2 if +ve
	_asm jb short fpcmp5		// if op1 exp < op2 exp op1<op2 if +ve
	_asm cmp edx, edi			// else compare mantissa high words
	_asm ja short fpcmp4
	_asm jb short fpcmp5
	_asm cmp ebx, ebp			// if equal compare mantissa low words
	_asm ja short fpcmp4
	_asm jb short fpcmp5
	fpcmp0:
	_asm mov eax, 2				// numbers exactly equal
	_asm ret
	fpcmp4:						// come here if ABS(op1)>ABS(op2) or if signs different
								// or if op2 zero, op1 nonzero
	_asm mov eax, 4				// return 4 if +ve
	_asm test cl, 1				// check sign
	_asm jz short fpcmp4a		// skip if +
	_asm mov al, 1				// return 1 if -ve
	fpcmp4a:
	_asm ret
	fpcmp5:						// come here if ABS(op1)<ABS(op2)
	_asm mov eax, 1				// return 1 if +ve
	_asm test cl, 1				// check sign
	_asm jz short fpcmp5a		// skip if +
	_asm mov al, 4				// return 4 if -ve
	fpcmp5a:
	_asm ret
	fpcmpop1z:					// come here if op1=0
	_asm cmp eax, 0x10000		// check if op2 also zero
	_asm jc short fpcmp0		// if so, they are equal
	_asm test al, 1				// test sign of op 2
	_asm mov eax, 4				// if -, return 4
	_asm jnz short fpcmpop1z2n	// skip if -
	_asm mov al, 1				// else return 1
	fpcmpop1z2n:
	_asm ret
	}




__NAKED__ EXPORT_C TRealX::TRealXOrder TRealX::Compare(const TRealX& /*aVal*/) const
/**
*/
	{
	// On entry ecx=this, [esp+4]=address of aVal
	_asm push ebx				// save registers
	_asm push ebp
	_asm push esi
	_asm push edi
	_asm mov esi, [esp+20]		// address of aVal into esi
	_asm mov ebx, [ecx]			// *this into ecx,edx:ebx
	_asm mov edx, [ecx+4]
	_asm mov ecx, [ecx+8]
	_asm call TRealXCompare		// result in eax
	_asm pop edi
	_asm pop esi
	_asm pop ebp
	_asm pop ebx
	_asm ret 4
	}




#pragma warning (default : 4100)	// unreferenced formal parameter
#pragma warning (default : 4414)	// short jump converted to near
#pragma warning (default : 4700)	// local variable 'this' used without having been initialised

