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
// e32\kernel\arm\cinit.cpp
//
//

#include <arm_mem.h>
#include <e32uid.h>

#ifdef __SMP__
#include <arm_scu.h>
#endif

extern "C" void NKIdle(TUint32 aStage)
	{
	SCpuIdleHandler* cih = NKern::CpuIdleHandler();
#ifdef __SMP__
	TSubScheduler& ss = SubScheduler();
	if (cih && cih->iHandler)
		(*cih->iHandler)(cih->iPtr, aStage, ss.iUncached);
#else
	if (cih && cih->iHandler)
		(*cih->iHandler)(cih->iPtr, aStage);
#endif
	else if (K::PowerModel)
		K::PowerModel->CpuIdle();
	else
		Arm::TheAsic->Idle();
	}

TInt A::CreateVariant(const TAny* aFile, TInt aMode)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::CreateVariant aFile=%08x aMode=%08x", aFile, aMode));
	const TRomImageHeader& img = *(const TRomImageHeader*)aFile;
	TVariantInitialise2 f = (TVariantInitialise2)*((TLinAddr*)img.iExportDir); // ordinal 1
	__KTRACE_OPT(KBOOT,Kern::Printf("Calling Initialise %08x", f));
	Arm::TheAsic = (Asic*)(*f)(0);
	__KTRACE_OPT(KBOOT,Kern::Printf("Initialise returned %08x", Arm::TheAsic));
	if (!Arm::TheAsic)
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
#ifdef __SMP__
		SVariantInterfaceBlock* vib = (SVariantInterfaceBlock*)K::VariantData[0];
		TSuperPage& spg = TheSuperPage();
		for (i=0; i<KMaxCpus; ++i)
			{
			if (!vib->iUncached[i])
				{
				vib->iUncached[i] = (UPerCpuUncached*)(spg.iAPBootPageLin + 0xE00 + (i<<6));
				}
			}
#endif
		}
	return KErrNone;
	}

void A::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init1()"));

	// first phase ASIC/Variant initialisation
	// install any extra coprocessors
	Arm::TheAsic->Init1();
	PP::NanoWaitCal=Arm::TheAsic->NanoWaitCalibration();

	TMachineStartupType t=Arm::TheAsic->StartupReason();
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
	Arm::Init1Interrupts();

	NTimerQ::Init1(Arm::TheAsic->MsTickPeriod());
	}

void A::Init2()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init2()"));
	}

#ifdef __SMP__
IMPORT_D extern const TInt KSMPNumCpus;

void A::InitAPs()
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("A::InitAPs()"));
	TInt ncpus = (SCU.iConfig & E_ArmScuCfg_NCpusMask) + 1;
	if (KSMPNumCpus > 0 && KSMPNumCpus < ncpus)
		ncpus = KSMPNumCpus; // patchable constant reduces number of available CPUs.
	__KTRACE_OPT(KBOOT, Kern::Printf("ncpus=%d", ncpus));
	TInt timeout = 500;
	TInt i;
	TSuperPage& spg = TheSuperPage();
#ifdef KPANIC
	if (spg.iDebugMask[0] & ~KPANIC)
		timeout = 30000;
#endif
	for (i=1; i<ncpus; ++i)
		{
		SArmAPBootInfo info;
		memclr(&info,sizeof(info));
		info.iCpu = i;
		info.iAPBootLin = spg.iAPBootPageLin;
		info.iAPBootPhys = spg.iAPBootPagePhys;
		info.iAPBootCodeLin = ::RomHeaderAddress;
		info.iAPBootCodePhys = spg.iRomHeaderPhys;
		info.iAPBootPageDirPhys = spg.iAPBootPageDirPhys;
		M::GetAPBootInfo(i, &info);
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iAPBootLin=%08x", info.iAPBootLin));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iAPBootPhys=%08x", info.iAPBootPhys));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iAPBootCodeLin=%08x", info.iAPBootCodeLin));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iAPBootCodePhys=%08x", info.iAPBootCodePhys));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iAPBootPageDirPhys=%08x", info.iAPBootPageDirPhys));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iInitR13Fiq=%08x", info.iInitR13Fiq));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iInitR13Irq=%08x", info.iInitR13Irq));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iInitR13Abt=%08x", info.iInitR13Abt));
		__KTRACE_OPT(KBOOT,DEBUGPRINT("iInitR13Und=%08x", info.iInitR13Und));
		K::InitAP(i, &info, timeout);
		}
	}

void A::Init2AP()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("A::Init2AP()"));
	Arm::TheAsic->Init2AP();
	}
#endif

GLREF_C void InitDummyMsgQ();
void A::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf(">A::Init3()"));

	InitDummyMsgQ();
	Arm::TheAsic->Init3();

	__KTRACE_OPT(KBOOT,Kern::Printf("<A::Init3()"));
	}

