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
// e32\kernel\x86\cinit.cpp
// 
//

#include <x86_mem.h>
#include <e32uid.h>

TInt A::CreateVariant(const TAny* aFile, TInt aMode)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::CreateVariant aFile=%08x aMode=%08x", aFile, aMode));
	const TRomImageHeader& img = *(const TRomImageHeader*)aFile;
	TVariantInitialise2 f = (TVariantInitialise2)*((TLinAddr*)img.iExportDir); // ordinal 1
	__KTRACE_OPT(KBOOT,Kern::Printf("Calling Initialise %08x", f));
	X86::TheAsic = (Asic*)(*f)(0);
	__KTRACE_OPT(KBOOT,Kern::Printf("Initialise returned %08x", X86::TheAsic));
	if (!X86::TheAsic)
		return KErrGeneral;
	if (aMode)
		{
		TInt i;
		for (i=0; i<31; ++i)
			{
			if (aMode & (1<<i))
				{
				__KTRACE_OPT(KBOOT,Kern::Printf("Calling VariantInitialise with %d", i+1));
				TAny* p = (*f)(i+1);
				K::VariantData[i] = p;
				__KTRACE_OPT(KBOOT,Kern::Printf("VariantInitialise returns %08x", p));
				}
			}
		}
	return KErrNone;
	}

void A::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init1()"));

	// first phase ASIC/Variant initialisation
	X86::TheAsic->Init1();
	PP::NanoWaitCal=X86::TheAsic->NanoWaitCalibration();
	X86_NanoWaitCal=PP::NanoWaitCal;

	TMachineStartupType t=X86::TheAsic->StartupReason();
	if (t==EStartupWarmReset || t==EStartupPowerFail || t==EStartupKernelFault)
		{
		if (!P::CheckSuperPageSignature())
			t=EStartupColdReset;
		}
	TheSuperPage().iStartupReason=t;
	switch (t)
		{
	case EStartupCold:
	case EStartupColdReset:
	case EStartupNewOs:
	case EStartupSafeReset:
		K::ColdStart=ETrue;
		break;

	case EStartupPowerFail:
	case EStartupWarmReset:
	case EStartupKernelFault:
		K::ColdStart=EFalse;
		break;

	default:
		PP::Panic(PP::EInvalidStartupReason);
		}

	// initialise the interrupt/exception handlers
	// doesn't enable interrupts - NKern::Init() does that
	X86::Init1Interrupts();

	NTimerQ::Init1(X86::TheAsic->MsTickPeriod());
	}

void A::Init2()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init2()"));
	}

#ifdef __SMP__
void A::InitAPs()
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("A::InitAPs()"));
	TInt timeout = 500;
	TInt i;
#ifdef KPANIC
	if (TheSuperPage().iDebugMask[0] & ~KPANIC)
		timeout = 30000;
#endif
	SCpuBootData* pInfo = X86::CpuBootData;
	memclr(pInfo, sizeof(X86::CpuBootData));
	X86::TheAsic->GetCpuBootData(pInfo);
	TInt a = 0;
	for (i=1; i<KMaxCpus; ++i)
		{
		SCpuBootData& info = pInfo[i];
		if (info.iPresent)
			{
			SX86APBootInfo binfo;
			memclr(&binfo,sizeof(binfo));
			binfo.iCpu = info.iAPICID;
			M::GetAPBootInfo(i, &binfo);
			K::InitAP(i, &binfo, timeout);
			++a;
			}
		}
	// check to see if it may be a dodgy laptop and try and init the second CPU
	if ((a == 0) && (Kern::SuperPage().iDebugPort == KNullDebugPort))
		{
		__KTRACE_OPT(KBOOT, Kern::Printf("A::InitAPs() - assuming laptop and starting 2nd CPU"));
		SX86APBootInfo binfo;
		memclr(&binfo,sizeof(binfo));
		binfo.iCpu = 1;
		M::GetAPBootInfo(1, &binfo);
		K::InitAP(1, &binfo, timeout);
		}
	}

void A::Init2AP()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init2AP()"));
	X86::TheAsic->Init2AP();
	}
#endif

GLREF_C void InitDummyMsgQ();
void A::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init3()"));

	InitDummyMsgQ();
	X86::TheAsic->Init3();
	}


