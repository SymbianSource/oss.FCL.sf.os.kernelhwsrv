// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\common\win32\seh.cpp
// 
//

#include "seh.h"

// Fill in the blank types for TWin32SEHTrap
#define __WIN32_SEH_TYPES_KNOWN__
#define __UnknownWindowsType1 EXCEPTION_RECORD
#define __UnknownWindowsType2 CONTEXT

// Pretend we're tools to avoid clashes with Win32 headers
#define __TOOLS__
#define __IN_SEH_CPP__
#include <e32cmn.h>
#include <e32cmn_private.h>

#include <emulator.h>

#include <e32panic.h>
GLREF_C void Panic(TCdtPanic);

// magic value denoting the end of the SEH handler list
static const TWin32SEHTrap* const KFencePost = (TWin32SEHTrap*)-1;

//
// Class TWin32SEHTrap
//

#ifdef __KERNEL_MODE__

extern DWORD CallFinalSEHHandler(EXCEPTION_RECORD* aException, CONTEXT* aContext)
	{
	// Get the final SEH entry on the chain
	TWin32SEHTrap* finalHandler = TWin32SEHTrap::IterateForFinal();

	// Call the handler - ignoring return value
	(void)(*finalHandler->ExceptionHandler())(aException, finalHandler, aContext);

	// Explicitly tell Win32 the exception has been handled
	return ExceptionContinueExecution;
	}

TWin32SEHTrap* TWin32SEHTrap::IterateForFinal()
	{
	TWin32SEHTrap* p = (TWin32SEHTrap*)Tib()->ExceptionList;

	// Iterate through the SEH chain to find the final SEH record that we wish to skip to
    for (; p && p!=KFencePost && p->iPrevExceptionRegistrationRecord!=KFencePost; p=p->iPrevExceptionRegistrationRecord)
		{}
	return p;
	}

TWin32SEHExceptionHandler* TWin32SEHTrap::ExceptionHandler()
	{
	return iExceptionHandler;
	}

#else // !__KERNEL_MODE__
#include <u32exec.h>


extern "C" void trap_check(TWin32SEHTrap* a)
	{
	TWin32SEHTrap* p = a->iPrevExceptionRegistrationRecord;
	if (p && p!=KFencePost && a->iExceptionHandler == p->iExceptionHandler && a->iExceptionHandler == &TWin32SEHTrap::ExceptionHandler)
		Exec::PushTrapFrame((TTrap*)p);
	else
		Exec::PushTrapFrame(0);
	}

extern "C" void untrap_check()
	{
	// search back for consecutive TWin32SEHTrap and remember the second one
	TWin32SEHTrap* p = (TWin32SEHTrap*)Tib()->ExceptionList;
	TWin32SEHTrap* q = 0;
	TWin32SEHTrap* s = 0;
	if (p && p!=KFencePost)
		{
		for(;;)
			{
			q = p->iPrevExceptionRegistrationRecord;
			if (!q || q==KFencePost)
				break;
			if (p->iExceptionHandler == &TWin32SEHTrap::ExceptionHandler && q->iExceptionHandler == &TWin32SEHTrap::ExceptionHandler)
				{
				s = q;
				break;
				}
			p = q;
			}
		}
	Exec::PushTrapFrame((TTrap*)s);
	}

// Use assembler to ensure no extra SEH frame is created by the compiler
UEXPORT_C __NAKED__ void TWin32SEHTrap::Trap()
	{
	_asm mov eax, fs:[0]
	_asm mov [ecx], eax
	_asm mov fs:[0], ecx
	_asm push ecx
	_asm call trap_check
	_asm pop ecx
	_asm ret
	}

extern "C" void panic_chain_corrupt()
	{
	Panic(EWin32SEHChainCorrupt);
	}

// Use assembler to ensure no extra SEH frame is created by the compiler
UEXPORT_C __NAKED__ void TWin32SEHTrap::UnTrap()
	{
	_asm mov eax, fs:[0]
	_asm cmp eax, ecx
	_asm ja untrap_0
	_asm jb untrap_error
	_asm mov eax, [ecx]
	_asm mov fs:[0], eax
	_asm xor eax, eax
	_asm mov [ecx], eax
	_asm call untrap_check
untrap_0:
	_asm ret
untrap_error:
	_asm jmp panic_chain_corrupt
	}

UEXPORT_C TWin32SEHTrap::TWin32SEHTrap()
	:	iPrevExceptionRegistrationRecord(NULL),
		iExceptionHandler(&ExceptionHandler)
	{
	}

// Handler called whilst Win32 is walking the SEH chain
DWORD TWin32SEHTrap::ExceptionHandler(EXCEPTION_RECORD* aException, TWin32SEHTrap* /*aRegistrationRecord*/, CONTEXT* aContext)
	{
	if (aException->ExceptionCode != EXCEPTION_MSCPP)
		{
		return Emulator::Win32SEHException(aException, aContext);
		}
	else
		{
		return ExceptionContinueSearch;
		}
	}

#if defined(__LEAVE_EQUALS_THROW__) && defined(__WINS__)
extern "C" TWin32SEHTrap* pop_trap_frame()
	{
	return (TWin32SEHTrap*)Exec::PopTrapFrame();
	}

extern "C" void leave_end()
	{
	Exec::LeaveEnd();
	}


EXPORT_C __NAKED__ TInt XLeaveException::GetReason() const
	{
	_asm push ecx
	_asm call pop_trap_frame
	_asm test eax, eax
	_asm jz no_nested_trap

	// eax points to TWin32SEHTrap to be restored
	// if current exception record is above eax on the stack just restore eax
	_asm cmp eax, esp
	_asm jbe nested_trap_error
	_asm mov edx, fs:[0]
	_asm cmp eax, edx
	_asm ja nested_trap_insert_in_middle
	_asm je no_nested_trap	// we haven't been unwound after all

	// check we eventually reach current exception record from eax
	_asm mov ecx, eax
	_asm mov edx, [eax+4]	// &TWin32SEHTrap::ExceptionHandler
nested_trap_check:
	_asm mov ecx, [ecx]
	_asm cmp ecx, 0
	_asm jz nested_trap_error
	_asm cmp ecx, 0ffffffffh
	_asm jz nested_trap_error
	_asm cmp ecx, fs:[0]
	_asm jz nested_trap_check_ok
	_asm cmp edx, [ecx+4]	// all intervening entries should be TWin32SEHTrap
	_asm jz nested_trap_check
	_asm jmp nested_trap_error

	// other SEH handlers have been added after we were unwound so we need to insert eax 'in the middle'
nested_trap_insert_in_middle:
	_asm cmp eax, [edx]
	_asm je no_nested_trap	// eax is still in the chain
	_asm jb nested_trap_insert_in_middle_found
	_asm mov edx, [edx]
	_asm jmp nested_trap_insert_in_middle

nested_trap_insert_in_middle_found:
	_asm mov ecx, [edx]		// first SEH above eax on stack
	_asm cmp ecx, 0ffffffffh
	_asm je nested_trap_error	// reached end of SEH list
	_asm push edx

	// ECX should be reachable from EAX
	_asm mov edx, eax
nested_trap_insert_in_middle_check:
	_asm mov edx, [edx]
	_asm cmp ecx, edx
	_asm je nested_trap_insert_in_middle_ok
	_asm cmp edx, 0ffffffffh
	_asm je nested_trap_error	// reached end of SEH list
	_asm test edx, edx
	_asm jnz nested_trap_insert_in_middle_check
	_asm jmp nested_trap_error

nested_trap_insert_in_middle_ok:
	_asm pop edx
	_asm mov [edx], eax		// insert eax back into chain
	_asm jmp no_nested_trap

nested_trap_check_ok:
	_asm mov fs:[0], eax	// reinstall nested trap SEH handlers

no_nested_trap:
	_asm call untrap_check	// check for other nested TRAPs
	_asm call leave_end
	_asm pop ecx
	_asm mov eax, [ecx]XLeaveException.iR
	_asm ret

nested_trap_error:
	_asm jmp panic_chain_corrupt
	}
#endif	// defined(__LEAVE_EQUALS_THROW__) && defined(__WINS__)

#endif //__KERNEL_MODE__
