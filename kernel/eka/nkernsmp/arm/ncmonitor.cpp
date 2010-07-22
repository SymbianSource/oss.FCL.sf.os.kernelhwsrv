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
// e32\nkernsmp\arm\ncmonitor.cpp
// Kernel crash debugger - NKERNSMP ARM specific portion
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include <kernel/monitor.h>
#include "nk_priv.h"
#include <arm.h>

SFullArmRegSet& RegSet(TInt aCpu)
	{
	TScheduler* pS = TScheduler::Ptr();
	TSubScheduler& ss = *pS->iSub[aCpu];
	return *ss.iSSX.iRegs;
	}

void DisplayNThreadStackedRegs(Monitor& m, SThreadReschedStack& reg)
	{
	m.Printf("TEEHBR %08x UROTID %08x URWTID %08x  FPEXC %08x\r\n", reg.iTEEHBR, reg.iRWROTID, reg.iRWRWTID, reg.iFpExc);
	m.Printf("   CAR %08x   DACR %08x\r\n", reg.iCar, reg.iDacr);
	m.Printf(" SPARE %08x   SPSR %08x SPRSCH %08x    R15 %08x\r\n", reg.iSpare, reg.iSpsrSvc, reg.iSPRschdFlg, reg.iR15);
	}

void DisplaySubSchedulerExt(Monitor& m, TSubScheduler& ss)
	{
	TSubSchedulerX& x = ss.iSSX;
	m.Printf("Extras[ 0] %08x Extras[ 1] %08x Extras[ 2] %08x i_GblTmrA  %08x\r\n", x.iSSXP[0], x.iSSXP[1], x.iSSXP[2], x.iGlobalTimerAddr);
	m.Printf("i_ScuAddr  %08x i_GicDist  %08x i_GicCpuIf %08x i_LocTmrA  %08x\r\n", x.iScuAddr, x.iGicDistAddr, x.iGicCpuIfcAddr, x.iLocalTimerAddr);
	m.Printf("i_IrqCount %08x i_IrqNest  %08x i_ExcInfo  %08x i_CrashSt  %08x\r\n", x.iIrqCount, x.iIrqNestCount, x.iExcInfo, x.iCrashState);
	m.Printf("i_AbtStkTp %08x i_UndSktTp %08x i_FiqStkTp %08x i_IrqStkTp %08x\r\n", x.iAbtStackTop, x.iUndStackTop, x.iFiqStackTop, x.iIrqStackTop);
	m.Printf("CpuFreqM   %08x CpuFreqS   %08x CpuPeriodM %08x CpuPeriodS %08x\r\n", x.iCpuFreqRI.iR.iM, x.iCpuFreqRI.iR.iX, x.iCpuFreqRI.iI.iM, x.iCpuFreqRI.iI.iX);
	m.Printf("TmrFreqM   %08x TmrFreqS   %08x TmrPeriodM %08x TmrPeriodS %08x\r\n", x.iTimerFreqRI.iR.iM, x.iTimerFreqRI.iR.iX, x.iTimerFreqRI.iI.iM, x.iTimerFreqRI.iI.iX);
	}

void DisplaySchedulerExt(Monitor& m, TScheduler& s)
	{
	TSchedulerX& sx = s.iSX;
	m.Printf("iTimerMax  %08x %08x\r\n", I64HIGH(sx.iTimerMax), I64LOW(sx.iTimerMax));
	m.Printf("iGTmrA     %08x  iScuAddr %08x  iGicDistA %08x  iGicCpuIfcA %08x  iLocTmrA %08x\r\n",
		sx.iGlobalTimerAddr, sx.iScuAddr, sx.iGicDistAddr, sx.iGicCpuIfcAddr, sx.iLocalTimerAddr);
	m.Printf("iGTFreqM   %08x iGTFreqS   %08x iGTPeriodM %08x iGTPeriodS %08x\r\n",
		sx.iGTimerFreqRI.iR.iM, sx.iGTimerFreqRI.iR.iX, sx.iGTimerFreqRI.iI.iM, sx.iGTimerFreqRI.iI.iX);
	m.Printf("iCount0    %08x %08x   iTimestamp0 %08x %08x\r\n",
		I64HIGH(sx.iCount0), I64LOW(sx.iCount0), I64HIGH(sx.iTimestamp0), I64LOW(sx.iTimestamp0));
	}

void DumpRegisters(Monitor& m, SFullArmRegSet& a)
	{
	SNormalRegs& r = a.iN;
	m.Printf("MODE_USR:\r\n");
	m.Printf(" R0=%08x  R1=%08x  R2=%08x  R3=%08x\r\n", r.iR0,  r.iR1,  r.iR2,  r.iR3);
	m.Printf(" R4=%08x  R5=%08x  R6=%08x  R7=%08x\r\n", r.iR4,  r.iR5,  r.iR6,  r.iR7);
	m.Printf(" R8=%08x  R9=%08x R10=%08x R11=%08x\r\n", r.iR8,  r.iR9,  r.iR10, r.iR11);
	m.Printf("R12=%08x R13=%08x R14=%08x R15=%08x\r\n", r.iR12, r.iR13, r.iR14, r.iR15);
	m.Printf("CPSR=%08x\r\n", r.iFlags);
	m.Printf("MODE_FIQ:\r\n");
	m.Printf(" R8=%08x  R9=%08x R10=%08x R11=%08x\r\n",  r.iR8Fiq,  r.iR9Fiq,  r.iR10Fiq, r.iR11Fiq);
	m.Printf("R12=%08x R13=%08x R14=%08x SPSR=%08x\r\n", r.iR12Fiq, r.iR13Fiq, r.iR14Fiq, r.iSpsrFiq);
	m.Printf("MODE_IRQ:\r\n");
	m.Printf("R13=%08x R14=%08x SPSR=%08x\r\n", r.iR13Irq, r.iR14Irq, r.iSpsrIrq);
	m.Printf("MODE_SVC:\r\n");
	m.Printf("R13=%08x R14=%08x SPSR=%08x\r\n", r.iR13Svc, r.iR14Svc, r.iSpsrSvc);
	m.Printf("MODE_ABT:\r\n");
	m.Printf("R13=%08x R14=%08x SPSR=%08x\r\n", r.iR13Abt, r.iR14Abt, r.iSpsrAbt);
	m.Printf("MODE_UND:\r\n");
	m.Printf("R13=%08x R14=%08x SPSR=%08x\r\n", r.iR13Und, r.iR14Und, r.iSpsrUnd);
//	m.Printf("MODE_MON:\r\n");
//	m.Printf("R13=%08x R14=%08x SPSR=%08x\r\n", r.iR13Mon, r.iR14Mon, r.iSpsrMon);

	SAuxiliaryRegs& aux = a.iA;
	m.Printf("TEEHBR=%08x  CPACR=%08x\r\n", aux.iTEEHBR, aux.iCPACR);

	SBankedRegs& b = a.iB[0];
	m.Printf(" SCTLR=%08x  ACTLR=%08x   PRRR=%08x   NMRR=%08x\r\n", b.iSCTLR, b.iACTLR, b.iPRRR, b.iNMRR);
	m.Printf("  DACR=%08x  TTBR0=%08x  TTBR1=%08x  TTBCR=%08x\r\n", b.iDACR, b.iTTBR0, b.iTTBR1, b.iTTBCR);
	m.Printf("  VBAR=%08x FCSEID=%08x CTXIDR=%08x\r\n", b.iVBAR, b.iFCSEIDR, b.iCTXIDR);
	m.Printf("Thread ID     RWRW=%08x   RWRO=%08x   RWNO=%08x\r\n", b.iRWRWTID, b.iRWROTID, b.iRWNOTID);
#ifdef __CPU_HAS_MMU
	extern TUint32 GetMMUID();
	m.Printf(" MMUID %08x\r\n", GetMMUID());
#endif
#ifdef __CPU_HAS_VFP
	m.Printf("FPEXC %08x\r\n", a.iMore[0]);
#endif
#ifdef __CPU_HAS_CACHE_TYPE_REGISTER
	extern TUint32 GetCacheType();
	m.Printf("CTYPE %08x\r\n", GetCacheType());
#endif
	m.NewLine();
	}

EXPORT_C void Monitor::DumpCpuRegisters()
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		Printf("CPU %d\r\n--------------------------------------------------------------------------------\r\n", i);
		SFullArmRegSet& r = RegSet(i);
		DumpRegisters(*this, r);
		NewLine();
		}
	}

EXPORT_C void Monitor::DisplayCpuFaultInfo()
	{
	TScheduler* pS=TScheduler::Ptr();
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		Printf("CPU %d:\r\n", i);
		TSubScheduler& ss = *pS->iSub[i];
		if (!ss.iSSX.iRegs)
			continue;
		SFullArmRegSet& r = *ss.iSSX.iRegs;
		
		Printf("Exc %1d Cpsr=%08x FAR=%08x FSR=%08x\r\n",  r.iExcCode, r.iN.iFlags, r.iB[0].iDFAR, r.iB[0].iDFSR);
		Printf(" R0=%08x  R1=%08x  R2=%08x  R3=%08x\r\n",  r.iN.iR0,  r.iN.iR1,  r.iN.iR2,  r.iN.iR3);
		Printf(" R4=%08x  R5=%08x  R6=%08x  R7=%08x\r\n",  r.iN.iR4,  r.iN.iR5,  r.iN.iR6,  r.iN.iR7);
		Printf(" R8=%08x  R9=%08x R10=%08x R11=%08x\r\n",  r.iN.iR8,  r.iN.iR9,  r.iN.iR10, r.iN.iR11);
		Printf("R12=%08x R13=%08x R14=%08x R15=%08x\r\n",  r.iN.iR12, r.iN.iR13, r.iN.iR14, r.iN.iR15);
		Printf("R13Svc=%08x R14Svc=%08x SpsrSvc=%08x\r\n", r.iN.iR13Svc, r.iN.iR14Svc, r.iN.iSpsrSvc);
		NewLine();
		}
	}

EXPORT_C void Monitor::GetStackPointers(NThread* aThread, TUint& aSupSP, TUint& aUsrSP)
	{
	if (aThread->iCurrent)
		{
		TInt i = aThread->iLastCpu;
		aSupSP = RegSet(i).iN.iR13Svc;
		aUsrSP = RegSet(i).iN.iR13;
		}
	else
		{
		aSupSP = (TUint)aThread->iSavedSP;
		SThreadExcStack* txs = (SThreadExcStack*)((TLinAddr)aThread->iStackBase + (TLinAddr)aThread->iStackSize);
		--txs;
		if (txs->iExcCode != SThreadExcStack::EInit && txs->iExcCode != SThreadExcStack::EStub)
			aUsrSP = txs->iR13usr;
		else
			aUsrSP = 0;
		}
	}
