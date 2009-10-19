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
// e32\kernel\x86\k_entry.cpp
// 
//

#include <kernel/kernel.h>
#include <x86_mem.h>
#include <kernel/klib.h>

// include the static data definitions
#define __FLTUSED
#include "win32crt.h"
#include "nwdl.h"


extern "C" void KernelMain();

extern "C"
int atexit(void (__cdecl *)(void))
	{
	return 0;
	}

extern "C" {

void _E32Startup_Body(TLinAddr aRomHeader, TLinAddr aSuperPage)
//
// The main startup program
// aRomHeader is address of ROM header passed in by bootstrap
// aSuperPage is address of super page passed in by bootstrap
//
	{
	PP::InitSuperPageFromRom(aRomHeader, aSuperPage);
//	A::DebugPrint(_L8("_E32Startup"));

	Kern::Printf("construct statics!");
	constructStatics();
	Kern::Printf("done construct statics!");
//	Kern::Printf("RomHeaderAddress=%08x, SuperPageAddress=%08x", RomHeaderAddress, SuperPageAddress);
	KernelMain();
	}

#ifdef __GCC32__
int __cxa_pure_virtual()
//
// Gets called for any unreplaced pure virtual methods.
//
	{
	KL::Panic(KL::EPureCall);
	return 0;
	}
#endif

}
