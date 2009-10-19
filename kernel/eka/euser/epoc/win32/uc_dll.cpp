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
// e32\euser\epoc\win32\uc_dll.cpp
// This file contains the DLL entrypoint
// 
//

#include "u32std.h"

typedef void* HANDLE;

#ifdef __VC32__
#define __FLTUSED
#endif //__VC32__

#ifdef __CW32__

struct SDestructorEntry;
extern SDestructorEntry* DEListHead;

TBool InitCWRuntime();
TBool CleanupCWRuntime();

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// DLL entry point
extern "C"
BOOL WINAPI _Win32DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
	{
	if (fdwReason == DLL_PROCESS_DETACH)
		{
		DEListHead = NULL;		// prevent destruction of static data after process has died
		return CleanupCWRuntime();
		}
	else
		return 1;
	}

#endif //__CW32__

// include the static data definitions
#include "win32crt.h"

// include compiler helpers
#include "x86hlp.inl"

extern "C"
EXPORT_C TInt _E32Dll(TInt aReason)
	{
	if (aReason==KModuleEntryReasonProcessAttach)
		{
#ifdef __CW32__
		if (!InitCWRuntime())
			return KErrGeneral;
		DEListHead = NULL;
#endif //__CW32__
		constructStatics();
		}
	if (aReason==KModuleEntryReasonProcessDetach)
		{
		destroyStatics();
#ifdef __CW32__
		DEListHead = NULL;
#endif //__CW32__
		}
	return KErrNone;
	}
