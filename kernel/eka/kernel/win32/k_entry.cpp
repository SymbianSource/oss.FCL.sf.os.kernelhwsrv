// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\win32\k_entry.cpp
// 
//

#include <kernel/kernel.h>

// include the static data definitions
#define __FLTUSED
#include "win32crt.h"

// include compiler helpers
#include "x86hlp.inl"

GLREF_C void BootEpoc();

GLDEF_D TBool EmulRunExe;

extern "C"
int atexit(void (__cdecl *)(void))
	{
	return 0;
	}

extern "C"
EXPORT_C void _E32Startup(TBool aRunExe)
//
// Ordinal 1 - used by EXEs to boot EPOC
//
	{
	constructStatics();
	EmulRunExe = aRunExe;
	BootEpoc();
	}
