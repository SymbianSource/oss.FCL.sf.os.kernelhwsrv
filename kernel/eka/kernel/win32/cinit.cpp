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
// e32\kernel\win32\cinit.cpp
// 
//

#include "memmodel.h"
#include <e32uid.h>
#include <property.h>
#include <emulator.h>

extern const char* JustInTime;
TBool DisableWSDWarning;

GLREF_D TBool EmulRunExe;

extern "C" void NKIdle(TUint32 aStage)
	{
	SCpuIdleHandler* cih = NKern::CpuIdleHandler();
	if (cih && cih->iHandler)
		(*cih->iHandler)(cih->iPtr, aStage);
	else if (K::PowerModel)
		K::PowerModel->CpuIdle();
	else
		Emul::TheAsic->Idle();
	}

TInt A::CreateVariant(const TAny* aHandle, TInt /*aMode*/)
//
// Hack! aHandle is not a rom image file, but a win32 HMODULE for the variant DLL
//
	{
	HMODULE h=(HMODULE)aHandle;
	TVariantInitialise f=(TVariantInitialise)Emulator::GetProcAddress(h, (LPCSTR)1);
	__KTRACE_OPT(KBOOT,Kern::Printf("Calling Initialise %08x",f));
	Emul::TheAsic=(*f)(EmulRunExe);
	__KTRACE_OPT(KBOOT,Kern::Printf("Initialise returned %08x",Emul::TheAsic));
	if (!Emul::TheAsic)
		return KErrGeneral;
	return KErrNone;
	}

void A::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init1()"));

	// first phase ASIC/Variant initialisation
	Emul::TheAsic->Init1();

	TheSuperPage().iStartupReason=EStartupColdReset;
	K::ColdStart=ETrue;

	NTimerQ::Init1(Emul::TheAsic->MsTickPeriod());
	}

void A::Init2()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init2()"));
	}

void A::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init3()"));

	Emul::TheAsic->Init3();
	JustInTime = Property::GetString("JustInTime","query");
    DisableWSDWarning = Property::GetBool("DisableWSDWarning",EFalse);
#ifdef _DEBUG
	// install a handler for thread panics to invoke the IDE debugger if
	// JustInTime is not set to 'none'
	if (_stricmp(JustInTime,"none") != 0)
		{
		Emul::TheJitHandler = new DJitCrashHandler;
		if (!Emul::TheJitHandler || Emul::TheJitHandler->Add() != KErrNone)
			K::Fault(K::EJitCrashHandlerCreation);
		}
	
#endif
	}


