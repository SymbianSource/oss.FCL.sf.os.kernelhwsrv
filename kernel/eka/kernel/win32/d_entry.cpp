// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\win32\d_entry.cpp
// 
//

#include <kernel/kernel.h>

typedef void* HANDLE;

#ifdef __VC32__
#define __FLTUSED
#endif //__VC32__

#ifdef __CW32__
struct SDestructorEntry;
extern SDestructorEntry* DEListHead;
#endif //__CW32__

// include the static data definitions
#include "win32crt.h"

// include compiler helpers
#include "x86hlp.inl"

GLREF_C TInt KernelModuleEntry(TInt);

#ifdef __CW32__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern "C"
BOOL WINAPI _Win32DllMain(HINSTANCE, DWORD, LPVOID)
	{
	return 1;
	}
#endif

extern "C"
EXPORT_C TInt _E32Dll(TInt aReason)
//
// EPOC Dll entrypoint for device drivers
//
	{
	if (aReason==KModuleEntryReasonProcessDetach)
		{
		destroyStatics();
		return KErrNone;
		}
	if (aReason==KModuleEntryReasonExtensionInit1 || aReason==KModuleEntryReasonProcessAttach)
		{
#ifdef __CW32__
		DEListHead = NULL;
#endif //__CW32__
		constructStatics();
		}
	return KernelModuleEntry(aReason);
	}
