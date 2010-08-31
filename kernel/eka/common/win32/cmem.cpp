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
// e32\common\win32\cmem.cpp
//
//

#include "common.h"

#ifdef __MEMMOVE_MACHINE_CODED__

extern "C" {

// See header file e32cmn.h for the in-source documentation.
EXPORT_C __NAKED__ TAny* memmove(TAny* , const TAny* , unsigned int)
	{
	_asm push      ebx							; // Save used registers
	_asm push      esi
	_asm push      edi
	_asm push      ebp

	_asm cmp       dword ptr [esp+0x1c],0x0		; // Is aLength == 0?
	_asm mov       eax,dword ptr [esp+0x14]		; // Ptr to destination
	_asm mov       ebx,dword ptr [esp+0x18]		; // Ptr to source
	_asm je        End							; // aLength is 0, just return

	_asm mov       ecx,eax						; // Copy destination
	_asm xor       ebp,ebp						; // ebp = 0
	_asm test      ecx,0x3						; // Dest word aligned?
	_asm mov       edx,ebx						; // Copy ptr to source
	_asm jne       Misaligned					; // No
	_asm test      edx,0x3						; // Source word aligned?
	_asm jne       Misaligned					; // No
	_asm mov       ebp,dword ptr [esp+0x1c]		; // ebp = aLength
	_asm shr       ebp,0x2						; // ebp = aLength in words

Misaligned:

	_asm lea       edx,dword ptr [ebp*4+0x0]	; // edx = aLength in words
	_asm sal       ebp,0x2						; // ebp = aLength in bytes
	_asm add       ebp,ecx						; // Point to end of destination
	_asm mov       edi,dword ptr [esp+0x1c]		; // Get number of bytes to copy
	_asm sub       edi,edx						; // Find remainder (aLength % 3)
	_asm cmp       eax,ebx						; // Dest >= source?
	_asm mov       edx,ebp						; // Ptr to end of destination
	_asm jae       DoDescendingCopy				; // Yes, copy downwards

	_asm jmp       AscendingCopy				; // No, copy upwards

AscendingCopyLoop:

	_asm mov       ebp,dword ptr [ebx]			; // Get a word
	_asm mov       dword ptr [ecx],ebp			; // And store it
	_asm add       ebx,0x4						; // Increment source by a word
	_asm add       ecx,0x4						; // Increment destination by a word

AscendingCopy:

	_asm cmp       ecx,edx						; // Still data to copy?
	_asm jb        AscendingCopyLoop			; // Yes

	_asm mov       ebp,eax						; // Copy ptr to destination
	_asm add       ebp,dword ptr [esp+0x1c]		; // Point to end of destination
	_asm jmp       CopyRemainder				; // Copy left over (aLength % 3) bytes

CopyRemainderLoop:

	_asm movzx     edx,byte ptr [ebx]			; // Get a byte
	_asm mov       byte ptr [ecx],dl			; // And store it
	_asm inc       ebx							; // Increment source by a byte
	_asm inc       ecx							; // Increment destination by a byte

CopyRemainder:

	_asm cmp       ecx,ebp						; // Any remaining bytes to copy?
	_asm jb        CopyRemainderLoop			; // Yes, go do it

	_asm jmp       End							; // All done

DoDescendingCopy:

	_asm cmp       eax,ebx						; // Still data to copy?
	_asm jbe       End							; // No, all done

	_asm lea       esi,dword ptr [edi+ebp]		; // Get ptr to end of destination
	_asm mov       edi,ebx						; // Get ptr to source
	_asm add       edi,dword ptr [esp+0x1c]		; // Point to end of source
	_asm jmp       DescendingCopyRemainder		; // Copy copy some data

DescendingCopyRemainderLoop:

	_asm dec       edi							; // Decrement source by a byte
	_asm dec       esi							; // Decrement dest by a byte
	_asm movzx     ebx,byte ptr [edi]			; // Get a byte
	_asm mov       byte ptr [esi],bl			; // And store it

DescendingCopyRemainder:

	_asm cmp       esi,ebp						; // Still data to copy?
	_asm ja        DescendingCopyRemainderLoop	; // Yes, go do it

	_asm jmp       DescendingCopy				; // Go copy the bulk of the data

DescendingCopyLoop:

	_asm sub       edi,0x4						; // Decrement source by a word
	_asm sub       edx,0x4						; // Decrement dest by a word
	_asm mov       ebx,dword ptr [edi]			; // Get a word
	_asm mov       dword ptr [edx],ebx			; // And store it

DescendingCopy:

	_asm cmp       edx,ecx						; // Still data to copy
	_asm ja        DescendingCopyLoop			; // Yes, go do it

End:

	_asm pop       ebp							; // Restore used registers
	_asm pop       edi
	_asm pop       esi
	_asm pop       ebx
	_asm ret
	}

// See header file e32cmn.h for the in-source documentation.
EXPORT_C __NAKED__ TAny* memcpy(TAny* , const TAny* , unsigned int)
	{
	__asm jmp (memmove);						; // memmove() will perform the same function
	}
}

#endif // defined(__MEMMOVE_MACHINE_CODED__)
