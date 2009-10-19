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
// e32\euser\epoc\win32\uc_cwhelp.cpp
// This file contains the CodeWarrior specific helper code common to eexe and edll
// 
//

#include <e32std.h>
#include <e32std_private.h>

// Define LAZY_INIT to allow the runtime to construct itself only when necessary.  This uses less
// TLS indicies, but is a more fragile approach.
//
#define LAZY_INIT

extern "C" {

//
// Dummy versions of CodeWarrior runtime functions.
//
// These are only called when we are linked against the DLL version of the runtime, or if these
// functions aren't present in the static runtime.
//
// The original functions are defined in:
//   c:/apps/Metrowerks/OEM3.1/Symbian_Support/MSL/MSL_C/MSL_Win32/Src/ThreadLocalData.c
//   c:/apps/Metrowerks/OEM3.1/Symbian_Support/MSL/MSL_C/MSL_Win32/Src/startup.win32.c
//

#ifdef LAZY_INIT
	
extern int _InitializeThreadDataIndex(void);
extern int *__get_MSL_init_count(void);
	
__declspec(weak) int *__get_MSL_init_count(void)
	{
	return NULL;
	}
	
#else

extern int _CRTStartup();
	
#endif
	
extern void _CleanUpMSL(void);

}

TBool InitCWRuntime()
	{
#ifdef LAZY_INIT
	return ETrue;
#else
	return _CRTStartup();
#endif
	}

TBool CleanupCWRuntime()
	{
#ifdef LAZY_INIT
	int* init_count_ptr = __get_MSL_init_count();
	if (!init_count_ptr)					// if we couldn't link this function, don't attempt cleanup
		return ETrue;		
	if (!_InitializeThreadDataIndex())		// make sure runtime is initialised to known state
		return EFalse;
	if (++(*init_count_ptr) != 1)			// make it look like _CRTStartup was called
		return EFalse;
#else
	if (*__get_MSL_init_count() < 1)		// _CRTStartup should have been called at least once
		return EFalse;
	*__get_MSL_init_count() = 1;			// reset to one so _CleanUpMSL runs
#endif
	_CleanUpMSL();							// call into runtime to do cleanup
	return ETrue;
	}
