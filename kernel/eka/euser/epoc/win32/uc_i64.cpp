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
// e32\euser\epoc\win32\uc_i64.cpp
// 
//

#include "u32std.h"
#include <e32math.h>

#pragma warning ( disable : 4100 )	// unreferenced formal parameter
#pragma warning ( disable : 4414 )  // short jump to function converted to near

extern "C" void UDiv64();




EXPORT_C __NAKED__ void Math::Mul64(Int64 /*aX*/, Int64 /*aY*/, Int64& /*aOutH*/, Uint64& /*aOutL*/)
/**
Multiply aX by aY to generate a 128 bit result.

The high order 64 bits of this calculation are stored in aOutH,
and the low order 64 bits are stored in aOutL.

@param aX     The first 64-bit operand.
@param aY     The second 64-bit operand.
@param aOutH  The high order 64 bits of the result.
@param aOutL  The low order  64 bits of the result.
*/
	{
	_asm mov eax, [esp+4]
	_asm mul dword ptr [esp+12]		// edx:eax = x0*y0
	_asm push edi
	_asm push esi
	_asm push ebx					// [esp+16]=&aX, [esp+24]=&aY, [esp+32]=&aOutH, [esp+36]=&aOutL
	_asm mov ecx, eax
	_asm mov ebx, edx				// ebx:ecx = x0*y0
	_asm mov eax, [esp+16]
	_asm mul dword ptr [esp+28]		// edx:eax = x0*y1
	_asm xor esi, esi
	_asm add ebx, eax
	_asm adc esi, edx				// esi:ebx:ecx = x0*y
	_asm mov eax, [esp+20]			// eax=x1
	_asm imul dword ptr [esp+28]	// edx:eax = x1*y1
	_asm mov edi, edx
	_asm add esi, eax
	_asm adc edi, 0					// partial result in edi:esi:ebx:ecx
	_asm cmp dword ptr [esp+28], 0	// y<0 ?
	_asm jns mul64_ypos
	_asm sub esi, [esp+16]			// if so, subtract x0<<64
	_asm sbb edi, 0
	mul64_ypos:
	_asm mov eax, [esp+20]			// eax=x1
	_asm cmp eax, 0					// x<0 ?
	_asm jns mul64_xpos
	_asm sub esi, [esp+24]			// if so, subtract y0<<64
	_asm sbb edi, 0
	mul64_xpos:
	_asm mul dword ptr [esp+24]		// edx:eax = x1*y0
	_asm add ebx, eax
	_asm mov eax, [esp+32]			// eax=&aOutH
	_asm adc esi, edx
	_asm mov edx, [esp+36]			// edx=&aOutL
	_asm adc edi, 0					// full result now in edi:esi:ebx:ecx
	_asm mov [eax], esi
	_asm mov [eax+4], edi			// store high 64
	_asm mov [edx], ecx
	_asm mov [edx+4], ebx			// store low 64
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm ret
	}




EXPORT_C __NAKED__ void Math::UMul64(Uint64 /*aX*/, Uint64 /*aY*/, Uint64& /*aOutH*/, Uint64& /*aOutL*/)
/**
Multiply aX by aY to generate a 128 bit result.

The high order 64 bits of this calculation are stored in aOutH,
and the low order 64 bits are stored in aOutL.

@param aX     The first 64-bit operand.
@param aY     The second 64-bit operand.
@param aOutH  The high order 64 bits of the result.
@param aOutL  The low order  64 bits of the result.
*/
	{
	_asm mov eax, [esp+4]
	_asm mul dword ptr [esp+12]		// edx:eax = x0*y0
	_asm push edi
	_asm push esi
	_asm push ebx					// [esp+16]=&aX, [esp+24]=&aY, [esp+32]=&aOutH, [esp+36]=&aOutL
	_asm mov ecx, eax
	_asm mov ebx, edx				// ebx:ecx = x0*y0
	_asm mov eax, [esp+16]
	_asm mul dword ptr [esp+28]		// edx:eax = x0*y1
	_asm xor esi, esi
	_asm add ebx, eax
	_asm adc esi, edx				// esi:ebx:ecx = x0*y
	_asm mov eax, [esp+20]			// eax=x1
	_asm mul dword ptr [esp+28]		// edx:eax = x1*y1
	_asm mov edi, edx
	_asm add esi, eax
	_asm adc edi, 0					// partial result in edi:esi:ebx:ecx
	_asm mov eax, [esp+20]
	_asm mul dword ptr [esp+24]		// edx:eax = x1*y0
	_asm add ebx, eax
	_asm mov eax, [esp+32]			// eax=&aOutH
	_asm adc esi, edx
	_asm mov edx, [esp+36]			// edx=&aOutL
	_asm adc edi, 0					// full result now in edi:esi:ebx:ecx
	_asm mov [eax], esi
	_asm mov [eax+4], edi			// store high 64
	_asm mov [edx], ecx
	_asm mov [edx+4], ebx			// store low 64
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm ret
	}




EXPORT_C __NAKED__ Int64 Math::DivMod64(Int64 /*aDividend*/, Int64 /*aDivisor*/, Int64& /*aRemainder*/)
/**
Divides aDividend by aDivisor.

The quotient is returned, and the remainder is stored in aRemainder.
The remainder has same sign as the dividend.

@param aDividend The 64-bit dividend.
@param aDivisor  The 64-bit divisor.
@param aRemainder The 64-bit remainder.

@return The 64-bit quotient.
*/
	{
	_asm mov eax, [esp+4]
	_asm mov edx, [esp+8]			// edx:eax = dividend
	_asm cmp edx, 0
	_asm jns divmod64_0
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	divmod64_0:						// edx:eax = ABS(dividend)
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm push ebp
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]			// edi:esi = dividend
	_asm cmp edi, 0
	_asm jns divmod64_1
	_asm neg edi
	_asm neg esi
	_asm sbb edi, 0					// edi:esi = ABS(dividend)
	divmod64_1:
	_asm call UDiv64				// do division, quotient in ebx:eax remainder in edi:edx
	_asm xchg ebx, edx				// quotient in edx:eax, remainder in edi:ebx
	_asm mov ecx, [esp+24]			// ecx=dividend high
	_asm xor ecx, [esp+32]			// ecx=dividend high ^ divisor high
	_asm jns divmod64_2
	_asm neg edx
	_asm neg eax
	_asm sbb edx, 0
	divmod64_2:						// edx:eax = quotient with correct sign
	_asm cmp dword ptr [esp+24], 0
	_asm jns divmod64_3
	_asm neg edi
	_asm neg ebx
	_asm sbb edi, 0
	divmod64_3:						// edi:ebx = remainder with correct sign
	_asm mov ecx, [esp+36]			// ecx=&aRemainder
	_asm mov [ecx], ebx
	_asm mov [ecx+4], edi
	_asm pop ebp
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm ret
	}




EXPORT_C __NAKED__ Uint64 Math::UDivMod64(Uint64 /*aDividend*/, Uint64 /*aDivisor*/, Uint64& /*aRemainder*/)
/**
Divides aDividend by aDivisor.

The quotient is returned, and the remainder is stored in aRemainder.

@param aDividend The 64-bit dividend.
@param aDivisor  The 64-bit divisor.
@param aRemainder The 64-bit remainder.

@return The 64-bit quotient.
*/
	{
	_asm mov eax, [esp+4]
	_asm mov edx, [esp+8]			// edx:eax = dividend
	_asm push edi
	_asm push esi
	_asm push ebx
	_asm push ebp
	_asm mov esi, [esp+28]
	_asm mov edi, [esp+32]			// edi:esi = dividend
	_asm call UDiv64				// do division, quotient in ebx:eax remainder in edi:edx
	_asm xchg ebx, edx				// quotient in edx:eax, remainder in edi:ebx
	_asm mov ecx, [esp+36]			// ecx=&aRemainder
	_asm mov [ecx], ebx
	_asm mov [ecx+4], edi
	_asm pop ebp
	_asm pop ebx
	_asm pop esi
	_asm pop edi
	_asm ret
	}

#pragma warning ( default : 4100 )
#pragma warning ( default : 4414 )
