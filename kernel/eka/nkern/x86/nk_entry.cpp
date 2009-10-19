// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\x86\nk_entry.cpp
// 
//

#include <x86.h>

extern "C" {
extern TAny* malloc(TUint32 aSize);
extern void free(TAny* aCell);
}

// include the static data definitions
#define __FLTUSED
#define __USE_MALLOC__
#include "win32crt.h"
#include "nwdl.h"


extern "C" void HwInit0();
extern "C" void KernelMain();

extern "C"
int atexit(void (__cdecl *)(void))
	{
	return 0;
	}

extern "C" {

extern TLinAddr RomHeaderAddress;
extern TLinAddr SuperPageAddress;


void _E32Startup_Body(TLinAddr aRomHeader, TLinAddr aSuperPage)
//
// The main startup program
// aRomHeader is address of ROM header passed in by bootstrap
// aSuperPage is address of super page passed in by bootstrap
//
	{
	RomHeaderAddress = aRomHeader;
	SuperPageAddress = aSuperPage;

	HwInit0();

	KPrintf("RomHeaderAddress = %08x", RomHeaderAddress);
	KPrintf("SuperPageAddress = %08x", SuperPageAddress);
	KPrintf("Calling global constructors...");
	constructStatics();
	KPrintf("Calling KernelMain()...");
	KernelMain();
	}
}

