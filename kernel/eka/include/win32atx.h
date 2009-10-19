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
// e32\include\win32atx.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __WIN32ATX_H__
#define __WIN32ATX_H__

#if defined(__VC32__)

#define	MAX_ATEXIT_HANDLERS	255

#ifdef __KERNEL_MODE__
#define PANIC()	Kern::Fault("ATEXIT", __LINE__)
#else
_LIT(KLitAtExitPanic,"ATEXIT");
#define PANIC()	User::Panic(KLitAtExitPanic, __LINE__)
#endif

extern "C" {
typedef void (__cdecl* TAtExit)(void);
static TUint sp=0;
static TAtExit handlers[MAX_ATEXIT_HANDLERS];

int atexit(TAtExit aFunc)
	{
	if (sp>=MAX_ATEXIT_HANDLERS)
		PANIC();
	handlers[sp++]=aFunc;
	return 0;
	}

void __call_atexit_handlers()
	{
	while(sp)
		(*handlers[--sp])();
	}

#pragma data_seg(".CRT$XPU")
TAtExit __xp_a[] = { __call_atexit_handlers };

}

#elif defined(__CW32__)

struct SDestructorEntry
	{
	SDestructorEntry* iNext;
	TAny* iDstrFn;
	TAny* iObj;
	};

SDestructorEntry* DEListHead;

extern "C" {

void* __register_global_object(void* obj, void* dfn, void* entry)
	{
	SDestructorEntry* e = (SDestructorEntry*)entry;
	e->iNext = DEListHead;
	e->iDstrFn = dfn;
	e->iObj = obj;
	DEListHead = e;
	return obj;
	}

__declspec(naked) void __destroy_global_chain(void)
	{
	_asm push ebp
	_asm mov ebp, esp
	_asm push ebx
	_asm lea ebx, DEListHead
	dgc1:
	_asm mov ebx, [ebx]
	_asm test ebx, ebx
	_asm jz dgc0
	_asm mov ecx, [ebx+8]
	_asm call dword ptr [ebx+4]
	_asm jmp dgc1
	dgc0:
	_asm lea ebx, DEListHead
	_asm mov [ebx], 0
	_asm pop ebx
	_asm mov esp, ebp
	_asm pop ebp
	_asm ret
	}

#pragma data_seg(".CRT$XPU")
void (*__xp_a[])(void) = { &__destroy_global_chain };
}

#elif defined(__GCC32__)
// todo: figure out what to do here
#else
#error Unknown X86 compiler
#endif

#endif
