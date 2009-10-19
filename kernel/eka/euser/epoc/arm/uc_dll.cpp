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

#elif defined(__ARMCC__)

void __DLL_Export_Table__(void);
void __cpp_initialize__aeabi_(void);
__weak void run_static_dtors(void);

GLDEF_C TInt _E32Dll_Body(TInt aReason)
	{
	if (aReason==KModuleEntryReasonProcessAttach)
		{
		__DLL_Export_Table__();
		__cpp_initialize__aeabi_();
		}
	else if (aReason==KModuleEntryReasonProcessDetach)
		{
		int call_static_dtors = (int)run_static_dtors;
		if (call_static_dtors) run_static_dtors();
		return KErrNone;
		}
	return 0;
	}

#else
#error not supported
#endif

}	// extern "C"

