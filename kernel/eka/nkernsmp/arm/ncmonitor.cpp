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
	return *(SFullArmRegSet*)ss.i_AbtStackTop;
	}

void Monitor::DisplaySpinLock(const char* aTitle, TSpinLock* aLock)
	{
	volatile TUint32* p = (volatile TUint32*)aLock;
	Printf("%s %08x %08x\r\n", aTitle, p[0], p[1]);
	}

EXPORT_C void Monitor::DisplayNThreadInfo(NThread* aT)
	{
	DisplayNSchedulableInfo(aT);
	}

void Monitor::DisplayNSchedulableInfo(NSchedulable* aS)
	{
	NThread* t = aS->iParent ? (NThread*)aS : 0;
	NThreadGroup* g = aS->iParent ? 0 : (NThreadGroup*)aS;

	if (t && aS->iParent==aS)
		Printf("NThread @%08x Pri %d\r\n",aS,aS->iPriority);
	else if (t)
		Printf("NThread @%08x (G:%08x) Pri %d\r\n",aS,aS->iParent,aS->iPriority);
	else
		Printf("NThreadGroup @%08x Pri %d\r\n",aS,aS->iPriority);
	Printf("Rdy=%02x Curr=%02x LastCpu=%d CpuChg=%02x FrzCpu=%d\r\n", aS->iReady, aS->iCurrent, aS->iLastCpu, aS->iCpuChange, aS->iFreezeCpu);
	Printf("Next=%08x Prev=%08x Parent=%08x CPUaff=%08x\r\n", aS->iNext, aS->iPrev, aS->iParent, aS->iCpuAffinity);
	Printf("PauseCount %02x Susp %1x\r\n", aS->iPauseCount, aS->iSuspended);
	DisplaySpinLock("SpinLock", &aS->iSSpinLock);
	Printf("Stopping %02x Events %08x %08x EventState %08x\r\n", aS->iStopping, aS->iEvents.iA.iNext, aS->iEvents.iA.iPrev, aS->iEventState);
	Printf("TotalCpuTime %08x %08x RunCount %08x %08x\r\n", aS->iTotalCpuTime32[1], aS->iTotalCpuTime32[0], aS->iRunCount32[1], aS->iRunCount32[0]);

	if (g)
		{
		// Thread group
		return;
		}

	Printf("WaitState %02x %02x [%02x %02x] (%08x)\r\n", t->iWaitState.iWtC.iWtStFlags, t->iWaitState.iWtC.iWtObjType,
											t->iWaitState.iWtC.iWtStSpare1, t->iWaitState.iWtC.iWtStSpare2, t->iWaitState.iWtC.iWtObj);
	Printf("BasePri %d MutexPri %d Att=%02x\r\n", t->iBasePri, t->iMutexPri, t->i_ThrdAttr);
	Printf("HeldFM=%08x FMDef=%02x AddrSp=%08x\r\n", t->iHeldFastMutex, t->iFastMutexDefer, t->iAddressSpace);
	Printf("Time=%d Timeslice=%d ReqCount=%08x\r\n", t->iTime, t->iTimeslice, t->iRequestSemaphore.iCount);
	Printf("SuspendCount=%d CsCount=%d CsFunction=%08x\r\n", t->iSuspendCount, t->iCsCount, t->iCsFunction);
	Printf("LinkedObjType %02x LinkedObj %08x\r\n", t->iLinkedObjType, t->iLinkedObj);
	Printf("SavedSP=%08x WaitLink:%08x %08x %d\r\n", t->iSavedSP, t->iWaitLink.iNext, t->iWaitLink.iPrev, t->iWaitLink.iPriority);
	Printf("iNewParent=%08x iExtraContext=%08x, iExtraContextSize=%08x\r\n", t->iNewParent, t->iExtraContext, t->iExtraContextSize);
	Printf("iUserModeCallbacks=%08x iNThreadBaseSpare6=%08x\r\n", t->iUserModeCallbacks, t->iNThreadBaseSpare6);
	Printf("iNThreadBaseSpare7=%08x iNThreadBaseSpare8=%08x iNThreadBaseSpare9=%08x\r\n", t->iNThreadBaseSpare7, t->iNThreadBaseSpare8, t->iNThreadBaseSpare9);
	if (!aS->iCurrent)
		{
		TUint32* pS=(TUint32*)t->iSavedSP;
		SThreadReschedStack reg;
		MTRAPD(r,wordmove(&reg,pS,sizeof(SThreadReschedStack)));
		if (r==KErrNone)
			{
			Printf("TEEHBR %08x UROTID %08x URWTID %08x  FPEXC %08x\r\n", reg.iTEEHBR, reg.iRWROTID, reg.iRWRWTID, reg.iFpExc);
			Printf("   CAR %08x   DACR %08x\r\n", reg.iCar, reg.iDacr);
			Printf(" SPARE %08x   SPSR %08x SPRSCH %08x    R15 %08x\r\n", reg.iSpare, reg.iSpsrSvc, reg.iSPRschdFlg, reg.iR15);
			}
		}
	NewLine();
	}

void Monitor::DisplayNFastSemInfo(NFastSemaphore* pS)
	{
	if (pS->iCount >= 0)
		Printf("NFastSemaphore @ %08x Count %d OwningThread %08x\r\n",pS,pS->iCount,pS->iOwningThread);
	else
		Printf("NFastSemaphore @ %08x Count %08x (%08x) OwningThread %08x\r\n",pS,pS->iCount,pS->iCount<<2,pS->iOwningThread);
	}

void Monitor::DisplayNFastMutexInfo(NFastMutex* aM)
	{
	Printf("NFastMutex @ %08x HoldingThread %08x iWaitQ Pri Mask %08x %08x\r\n", aM, aM->iHoldingThread, aM->iWaitQ.iPresent[1], aM->iWaitQ.iPresent[0]);
	DisplaySpinLock("SpinLock", &aM->iMutexLock);
	}

void DisplaySubSchedulerInfo(Monitor& m, TSubScheduler& ss)
	{
	m.Printf("\r\nSUBSCHEDULER %d @%08x:\r\n", ss.iCpuNum, &ss);
	m.Printf("CurrentThread=%08x\r\n", ss.iCurrentThread);
	m.Printf("IDFCs   %08x %08x    CPU# %08x CPUmask %08x\r\n", ss.iDfcs.iA.iNext, ss.iDfcs.iA.iPrev, ss.iCpuNum, ss.iCpuMask);
	m.Printf("ExIDFCs %08x %08x CurIDFC %08x PendFlg %08x\r\n", ss.iExIDfcs.iA.iNext, ss.iExIDfcs.iA.iPrev, ss.iCurrentIDFC, *(TUint32*)&ss.iRescheduleNeededFlag);
	m.DisplaySpinLock("ExIDfcLock", &ss.iExIDfcLock);
	m.Printf("KLCount %d InIDFC %02x EvPend %02x\r\n", ss.iKernLockCount, ss.iInIDFC, ss.iEventHandlersPending);
	m.Printf("AddrSp  %08x RschdIPIs %08x iNextIPI %08x\r\n", ss.iAddressSpace, ss.iReschedIPIs, ss.iNextIPI);
	m.DisplaySpinLock("ReadyListLock", &ss.iReadyListLock);
	m.Printf("EvtHand %08x %08x InitThrd %08x SLOC %08x %08x\r\n", ss.iEventHandlers.iA.iNext, ss.iEventHandlers.iA.iPrev,
		ss.iInitialThread, I64HIGH(ss.iSpinLockOrderCheck), I64LOW(ss.iSpinLockOrderCheck));
	m.DisplaySpinLock("EventHandlerLock", &ss.iEventHandlerLock);
	m.Printf("Extras[ 0] %08x Extras[ 1] %08x Extras[ 2] %08x Extras[ 3] %08x\r\n", ss.iExtras[0], ss.iExtras[1], ss.iExtras[2], ss.iExtras[3]);
	m.Printf("i_ScuAddr  %08x i_GicDist  %08x i_GicCpuIf %08x i_LocTmrA  %08x\r\n", ss.i_ScuAddr, ss.i_GicDistAddr, ss.i_GicCpuIfcAddr, ss.i_LocalTimerAddr);
	m.Printf("i_IrqCount %08x i_IrqNest  %08x i_ExcInfo  %08x i_CrashSt  %08x\r\n", ss.i_IrqCount, ss.i_IrqNestCount, ss.i_ExcInfo, ss.i_CrashState);
	m.Printf("i_AbtStkTp %08x i_UndSktTp %08x i_FiqStkTp %08x i_IrqStkTp %08x\r\n", ss.i_AbtStackTop, ss.i_UndStackTop, ss.i_FiqStackTop, ss.i_IrqStackTop);
	m.Printf("i_TmrMultF %08x i_TmrMultI %08x i_CpuMult  %08x Extras[13] %08x\r\n", ss.i_TimerMultF, ss.i_TimerMultI, ss.i_CpuMult, ss.iExtras[19]);
	m.Printf("i_LstTmrSt %08x i_TstmpErr %08x i_MaxCorr  %08x i_TimerGap %08x\r\n", ss.iExtras[20], ss.iExtras[21], ss.iExtras[22], ss.iExtras[23]);
	m.Printf("iLastTimestamp %08x %08x   iReschedCount %08x %08x\r\n", ss.iLastTimestamp32[1], ss.iLastTimestamp32[0], ss.iReschedCount32[1], ss.iReschedCount32[0]);
	}

void Monitor::DisplaySchedulerInfo()
	{
	TScheduler* pS=TScheduler::Ptr();
	Printf("SCHEDULER @%08x:\r\n",pS);
	Printf("ProcessHandler=%08x MonitorExceptionHandler=%08x RescheduleHook=%08x\r\n",pS->iProcessHandler,pS->iMonitorExceptionHandler,pS->iRescheduleHook);
	Printf("iActiveCpus1=%08x, iActiveCpus2=%08x, iNumCpus=%d\r\n",pS->iActiveCpus1,pS->iActiveCpus2,pS->iNumCpus);
	Printf("SYSLOCK @ %08x\r\n",&pS->iLock);
	DisplayNFastMutexInfo(&pS->iLock);
	DisplaySpinLock("IdleSpinLock", &pS->iIdleSpinLock);
	Printf("IdleDfcs %08x %08x CpusNotIdle %08x IdleGeneration %02x IdleSpillCpu %02x\r\n",
		pS->iIdleDfcs.iA.iNext, pS->iIdleDfcs.iA.iPrev, pS->iCpusNotIdle, pS->iIdleGeneration, pS->iIdleSpillCpu);
	Printf("Extras  0: %08x  1: %08x  2: %08x  3: %08x\r\n",pS->iExtras[0],pS->iExtras[1],pS->iExtras[2],pS->iExtras[3]);
	Printf("Extras  4: %08x  5: %08x  6: %08x  7: %08x\r\n",pS->iExtras[4],pS->iExtras[5],pS->iExtras[6],pS->iExtras[7]);
	Printf("Extras  8: %08x  9: %08x  A: %08x  B: %08x\r\n",pS->iExtras[8],pS->iExtras[9],pS->iExtras[10],pS->iExtras[11]);
	Printf("Extras  C: %08x  D: %08x  E: %08x  F: %08x\r\n",pS->iExtras[12],pS->iExtras[13],pS->iExtras[14],pS->iExtras[15]);
	Printf("Extras 10: %08x 11: %08x 12: %08x 13: %08x\r\n",pS->iExtras[16],pS->iExtras[17],pS->iExtras[18],pS->iExtras[19]);
	Printf("Extras 14: %08x 15: %08x 16: %08x 17: %08x\r\n",pS->iExtras[20],pS->iExtras[21],pS->iExtras[22],pS->iExtras[23]);

	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = *pS->iSub[i];
		DisplaySubSchedulerInfo(*this,ss);
		}
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
		if (!ss.i_Regs)
			continue;
		SFullArmRegSet& r = *(SFullArmRegSet*)ss.i_Regs;
		
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
//	TScheduler* pS = TScheduler::Ptr();
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
