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
// e32\euser\epoc\win32\uc_trp.cpp
// 
//

#include <u32exec.h>
#include <e32panic.h>
#include "uc_std.h"

GLREF_C void Panic(TCdtPanic);

#ifndef __LEAVE_EQUALS_THROW__

// __thiscall - parameter is expected to be removed by this function
EXPORT_C TInt TTrap::Trap(TInt &aResult)
//
// Save the enter frame state and return 0.
//
	{
	// compiler generates push ebp; mov ebp,esp here
	// hence correct action for Leave() is to restore ebp then mov esp,ebp; pop ebp; ret 4
	aResult=KErrNone;
	iResult=(&aResult);
	_asm mov eax, this
	_asm mov [eax], ebx
	_asm mov [eax+4], esi
	_asm mov [eax+8], edi
	_asm mov [eax+12], ebp
	_asm mov [eax+16], ds
	_asm mov [eax+20], es
	_asm mov [eax+24], fs
	_asm mov [eax+28], gs
	_asm mov edx, [ebp]
	_asm mov [eax+32], edx
	_asm mov edx, [ebp+4]
	_asm mov [eax+36], edx
	TTrapHandler* h = Exec::PushTrapFrame(this);
	if (h != NULL)
		h->Trap();
	return(0);
	}



// __cdecl - this function will leave parameter on the stack
// but it will return to the instruction after the call to TTrap::Trap()
// and this expects a parameter to be popped from the stack
// so we must return with a ret 4, rather than leaving the compiler to generate a ret 0
EXPORT_C void User::Leave(TInt aReason)
/**
Leaves the currently executing function, unwinds the call stack, and returns
from the most recently entered trap harness.

@param aReason The value returned from the most recent call to TRAP or TRAPD.
               This is known as the reason code and, typically, it gives the
               reason for the environment or user error causing this leave
               to occur.
              
@see TRAP
@see TRAPD              
*/
	{

	TTrap *pT=Exec::PopTrapFrame();
    if (!pT)
        ::Panic(EUserLeaveWithoutTrap);
	TTrapHandler *pH=pT->iHandler;
	*pT->iResult=aReason;
	if (pH!=NULL)
		pH->Leave(aReason);
	_asm mov eax, pT
	_asm mov ebp, [eax+12]
	_asm mov esp, ebp
	_asm mov ebx, [eax]
	_asm mov esi, [eax+4]
	_asm mov edi, [eax+8]
	_asm mov ds, [eax+16]
	_asm mov es, [eax+20]
	_asm mov fs, [eax+24]
	_asm mov gs, [eax+28]
	_asm mov edx, [eax+32]
	_asm mov [ebp], edx
	_asm mov edx, [eax+36]
	_asm mov [ebp+4], edx
	_asm mov eax, 1
	_asm pop ebp
	_asm ret 4
	}

#endif // !__LEAVE_EQUALS_THROW__
