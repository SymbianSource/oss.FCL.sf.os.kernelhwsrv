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
// e32\euser\epoc\arm\uc_dll.cpp
// This file contains the DLL entrypoint
// 
//

#include "u32std.h"


extern "C" {

#if defined(__GCC32__)

typedef void (*PFV)();
extern PFV __CTOR_LIST__[];
extern PFV __DTOR_LIST__[];

GLDEF_C TInt _E32Dll_Body(TInt aReason)
	{
	if (aReason==KModuleEntryReasonProcessAttach)
		{
		TUint i=1;
		while (__CTOR_LIST__[i])
			(*__CTOR_LIST__[i++])();
		}
	else if (aReason==KModuleEntryReasonProcessDetach)
		{
		TUint i=1;
		while (__DTOR_LIST__[i])
			(*__DTOR_LIST__[i++])();
		}
	return 0;
	}

#elif defined(__EABI__)

#ifdef __GCCE__
// Workaround for bug #3560, http://developer.symbian.org/bugs/show_bug.cgi?id=3560
static int AddressIsInCodeSegment(void *addr)
{
	void * code_seg_base;
	void * code_seg_limit;
	asm(".extern Image$$ER_RO$$Base");
	asm(".extern Image$$ER_RO$$Limit");
	asm("code_seg_limit:");
	asm(".word Image$$ER_RO$$Limit");
	asm("code_seg_base:");
	asm(".word Image$$ER_RO$$Base");
	asm("ldr %[result], code_seg_base" : [result] "=r" (code_seg_base));
	asm("ldr %[result], code_seg_limit" : [result] "=r" (code_seg_limit));
	return addr >= code_seg_base && addr < code_seg_limit;		
}
#endif

void __DLL_Export_Table__(void);
void __cpp_initialize__aeabi_(void);
__WEAK__ void run_static_dtors(void);

GLDEF_C TInt _E32Dll_Body(TInt aReason)
	{
	if (aReason==KModuleEntryReasonProcessAttach)
		{
		__DLL_Export_Table__();
		__cpp_initialize__aeabi_();
		}
	else if (aReason==KModuleEntryReasonProcessDetach)
		{
#if defined(__ARMCC__)
		int call_static_dtors = (int)run_static_dtors;
		if (call_static_dtors) run_static_dtors();
#elif defined(__GCCE__)
// Workaround for bug #3560, http://developer.symbian.org/bugs/show_bug.cgi?id=3560
		if (AddressIsInCodeSegment((void *)run_static_dtors)) run_static_dtors();
#else
#error What compiler?
#endif
		return KErrNone;
		}
	return 0;
	}

#else
#error not supported
#endif

}	// extern "C"

