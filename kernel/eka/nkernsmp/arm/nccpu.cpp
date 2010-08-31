// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\arm\nccpu.cpp
// 
//

#include <arm.h>
#include <arm_gic.h>
#include <arm_scu.h>
#include <arm_tmr.h>


struct SAPBootPage : public SFullArmRegSet
	{
	volatile T_UintPtr	iAPBootPtr[KMaxCpus];
	volatile T_UintPtr	iBootFlags;
	volatile T_UintPtr	iBootFlags2;
	volatile T_UintPtr	iBootFlags3;
	volatile T_UintPtr	iBootFlags4;
	volatile TUint64	iBPTimestamp;
	volatile TUint64	iAPTimestamp;

	TUint32				i_SAPBootPage_Spare[624];

	UPerCpuUncached		iPerCpu[KMaxCpus];
	};

__ASSERT_COMPILE(sizeof(SAPBootPage)==4096);


extern "C" void _ApEntry();
extern "C" void KickCpu(volatile T_UintPtr* aPtr, T_UintPtr aRegsPhys);
extern void DumpFullRegSet(SFullArmRegSet& a);

TInt NKern::BootAP(volatile SAPBootInfo* aInfo)
	{
	volatile SArmAPBootInfo& a = *(volatile SArmAPBootInfo*)aInfo;
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NKern::BootAP %d %08x+%x", a.iCpu, a.iInitStackBase, a.iInitStackSize));

	T_UintPtr bp_phys = a.iAPBootPhys;
	T_UintPtr BootFlagsPhys = bp_phys + (T_UintPtr)_FOFF(SAPBootPage,iBootFlags);
	volatile SAPBootPage& bootPage = *(volatile SAPBootPage*)a.iAPBootLin;
	Arm::SaveState((SFullArmRegSet&)bootPage);
	NKern::EnableAllInterrupts();	// Arm::SaveState() disables interrupts

	TLinAddr BootFlagsLin = (TLinAddr)&bootPage.iBootFlags;
	TLinAddr init_sp = a.iInitStackBase + a.iInitStackSize;

	volatile T_UintPtr* pB = (volatile T_UintPtr*)BootFlagsLin;
	*pB = 0;
	pB[1] = 0;
	pB[2] = 0;
	pB[3] = 0;

	__KTRACE_OPT(KBOOT,DEBUGPRINT("BootFlagsPhys=%08x BootFlagsLin=%08x RegsPhys=%08x RegsLin=%08x",
		BootFlagsPhys, BootFlagsLin, bp_phys, &bootPage));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("SCU: iCtrl=%08x iConfig=%08x iCpuStat=%08x", SCU.iCtrl, SCU.iConfig, SCU.iCpuStatus));

	bootPage.iN.iR13Abt		= a.iInitR13Abt;
	bootPage.iN.iR13Und		= a.iInitR13Und;
	bootPage.iN.iR13Irq		= a.iInitR13Irq;
	bootPage.iN.iR13Fiq		= a.iInitR13Fiq;
	bootPage.iN.iR13Svc		= init_sp;
	bootPage.iN.iFlags		= 0x1D3;
	bootPage.iN.iR1			= (TUint32)a.iAPBootPageDirPhys;
	bootPage.iN.iR2			= TUint32(a.iAPBootCodeLin) - TUint32(a.iAPBootCodePhys);
	bootPage.iN.iR3			= (TUint32)a.iAPBootLin;
	bootPage.iN.iR4			= (TUint32)aInfo;
	bootPage.iN.iR5			= BootFlagsPhys;
	bootPage.iN.iR6			= BootFlagsLin;
	bootPage.iN.iR7			= a.iInitStackSize;
	bootPage.iN.iR15		= (TLinAddr)&_ApEntry;
	bootPage.iB[0].iRWRWTID	= 0;
	bootPage.iB[0].iRWROTID	= 0;
	bootPage.iB[0].iRWNOTID	= 0;

	__KTRACE_OPT(KBOOT,DumpFullRegSet((SFullArmRegSet&)bootPage));

	arm_dsb();	// ensure writes to uncached memory visible

	KickCpu(&bootPage.iAPBootPtr[a.iCpu], bp_phys);

	TUint32 n = TUint32(TheScheduler.iVIB->iMaxCpuClock >> 3);
	n = -n;
	TUint32 b = 0;
	do	{
		++n;
		b = *pB;
		} while(b!=init_sp && n!=0);
	arm_dsb();

	__KTRACE_OPT(KBOOT,DEBUGPRINT("BootFlag=%08x %08x %08x %08x n=%d", b, n, pB[1], pB[2], pB[3]));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("SCU: iCtrl=%08x iConfig=%08x iCpuStat=%08x", SCU.iCtrl, SCU.iConfig, SCU.iCpuStatus));
	if (n==0)
		return KErrTimedOut;

#if defined(__NKERN_TIMESTAMP_USE_LOCAL_TIMER__)
#error Use of local timer for NKern::Timestamp() no longer supported
#endif

	return KErrNone;
	}

void InitTimestamp(TSubScheduler* aSS, SNThreadCreateInfo& aInfo)
	{
	NThread* t = (NThread*)aSS->iCurrentThread;
	t->iActiveState = 1;
	if (aSS->iCpuNum == 0)
		{
		aSS->iLastTimestamp.i64 = 0;
		t->iLastActivationTime.i64 = 0;
		return;
		}
	TUint64 ts = 0;
#if defined(__NKERN_TIMESTAMP_USE_LOCAL_TIMER__)
#error Use of local timer for NKern::Timestamp() no longer supported
#endif
	ts = NKern::Timestamp();
	aSS->iLastTimestamp.i64 = ts;
	t->iLastActivationTime.i64 = ts;
	}

