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
// e32\memmodel\emul\win32\zrom.cpp
// Emulator implementation of the ROM related parts of the system
// 
//

#include "plat_priv.h"
#include <property.h>
#include <emulator.h>
#include "memmodel.h"

const char* KVariantDllName="ecust.dll";

TInt P::DefaultInitialTime()
	{
//
// Not used on emulator
//
	return KErrGeneral;
	}

TInt P::InitSystemTime()
	{
//
//  Initialise system time
//	Return the initial time in seconds from 00:00:00 01-01-2000
//
	TUint dummy;
	K::SetSystemTimeAndOffset(0, 0, 0, dummy, ETimeSetOffset|ETimeSetNoTimeUpdate);

	__KTRACE_OPT(KBOOT,Kern::Printf("Use Host time to set system time"));
    TInt seconds;
	A::SystemTimeInSecondsFrom2000(seconds);
	return seconds;
	}


void P::CreateVariant()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("CreateVariant"));
	HINSTANCE var = LoadLibraryA(KVariantDllName);
	if (var)
		{
		TLibraryEntry entryPoint=(TLibraryEntry)Emulator::GetProcAddress(var, "_E32Dll");
		if (entryPoint)
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Found variant"));
			// Call the entry point (global constructors)
			__KTRACE_OPT(KBOOT,Kern::Printf("Calling entrypoint %08x",entryPoint));
			TInt r=entryPoint(KModuleEntryReasonVariantInit0);
			__KTRACE_OPT(KBOOT,Kern::Printf("Entrypoint returned %d",r));
			if (r<0)
				Kern::Fault("VariantEntry",r);

			// Initialise and create the variant object
			r=A::CreateVariant(var, r);
			if (r<0)
				Kern::Fault("VariantInit",r);
			__KTRACE_OPT(KBOOT,Kern::Printf("Variant installed"));

			BTrace::Init0(); // we have to do this after variant has initialised the SuperPage
			return;
			}
		}
	Kern::Fault("NoVariant",0);
	}


TInt StartExtension(const char* aExtension)
	{
	HINSTANCE ext;
 	HMODULE mh;
 	if (*aExtension == '?')
 		{
 		ext = LoadLibraryA(++aExtension);
 		if (!ext)
 			{
 			__KTRACE_OPT(KBOOT, Kern::Printf("Optional extension \"%s\" not found", aExtension));
 			return KErrNone;
 			}
 		}
 	else
 		{
 		ext = LoadLibraryA(aExtension);
 		if (!ext)
 			return KErrNotFound;
 		}		
 	mh = GetModuleHandleA(aExtension);	
	TLibraryEntry entryPoint=(TLibraryEntry)Emulator::GetProcAddress(ext, "_E32Dll");
	if (!entryPoint)
		return KErrArgument;
	TInt r=entryPoint(KModuleEntryReasonExtensionInit0);
	if (r!=KErrNone)
		{
		__KTRACE_OPT(KBOOT,Kern::Printf("Extension %s already started", aExtension));
		return KErrNone;
		}
	DCodeSeg::Wait();
	r=MM::RegisterModule(mh);
	DCodeSeg::Signal();
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KBOOT,Kern::Printf("Calling entrypoint of %s extension ", aExtension));
	return entryPoint(KModuleEntryReasonExtensionInit1);
	}


void P::StartExtensions()
	{
	HMODULE mh = GetModuleHandleA(KVariantDllName);
	TInt r = MM::RegisterModule(mh);
	const char* extension = NULL;
	if (r==KErrNone)
		extension = Property::GetString("Extension");
	if (extension)
		{
		for (;;)
			{
			char name[128];
			const char* end = strchr(extension, ';');
			if (!end)
				break;
			strncpy(name, extension, end-extension);
			name[end-extension] = '\0';
			if (strchr(name, '.') == NULL)
				strcat(name, ".dll");
			r = StartExtension(name);
			if (r != KErrNone)
				break;
			extension = end+1;
			}
		if (r==KErrNone)
			r = StartExtension(extension);
		}
	if (r != KErrNone)
		K::Fault(K::EStartExtensionsFailed);
	}
