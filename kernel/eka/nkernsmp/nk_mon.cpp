// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\nk_mon.cpp
// Kernel crash debugger - NKERNSMP platform-independent portion
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_bal.h"
#include <kernel/monitor.h>
#include "nk_priv.h"

void Monitor::DisplaySpinLock(const char* aTitle, TSpinLock* aLock)
	{
	volatile TUint32* p = (volatile TUint32*)aLock;
	Printf("%s %08x %08x\r\n", aTitle, p[0], p[1]);
	}

EXPORT_C void Monitor::DisplayNThreadInfo(NThread* aT)
	{
	DisplayNSchedulableInfo(aT);
	}

extern void DisplayNThreadStackedRegs(Monitor&, SThreadReschedStack&);
extern void DisplaySubSchedulerExt(Monitor& m, TSubScheduler& ss);
extern void DisplaySchedulerExt(Monitor& m, TScheduler& s);

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
	Printf("Next=%08x Prev=%08x Parent=%08x\r\n", aS->iNext, aS->iPrev, aS->iParent);
	Printf("CPUaff=%08x PrefCpu=%02x TCpu=%02x FCpu=%02x\r\n", aS->iCpuAffinity, aS->iPreferredCpu, aS->iTransientCpu, aS->iForcedCpu);
	Printf("PauseCount %02x Susp %1x ACount %02x ActiveState %d\r\n", aS->iPauseCount, aS->iSuspended, aS->iACount, aS->iActiveState);
	DisplaySpinLock("SpinLock", &aS->iSSpinLock);
	Printf("Stopping %02x Events %08x %08x EventState %08x\r\n", aS->iStopping, aS->iEvents.iA.iNext, aS->iEvents.iA.iPrev, aS->iEventState);
	Printf("TotalCpuTime    %08x %08x  RunCount        %08x %08x\r\n", aS->iTotalCpuTime.i32[1], aS->iTotalCpuTime.i32[0], aS->iRunCount.i32[1], aS->iRunCount.i32[0]);
	Printf("TotalActiveTime %08x %08x  LastActivation  %08x %08x\r\n", aS->iTotalActiveTime.i32[1], aS->iTotalActiveTime.i32[0], aS->iLastActivationTime.i32[1], aS->iLastActivationTime.i32[0]);
	Printf("SavedCpuTime    %08x %08x  SavedActiveTime %08x %08x\r\n", aS->iSavedCpuTime.i32[1], aS->iSavedCpuTime.i32[0], aS->iSavedActiveTime.i32[1], aS->iSavedActiveTime.i32[0]);
	Printf("LastRunTime     %08x %08x  LbLink          %08x %08x    LbState %02x\r\n", aS->iLastRunTime.i32[1], aS->iLastRunTime.i32[0], aS->iLbLink.iNext, aS->iLbLink.iPrev, aS->iLbState);

	DUMP_LOAD_BALANCE_INFO(aS);
	volatile TUint32* el = (volatile TUint32*)&aS->iEnumerateLink;
	Printf("EnumLink  %08x %08x\r\n", el[0], el[1]);

	if (g)
		{
		// Thread group
		return;
		}

	Printf("WaitState %02x %02x [%02x %02x] (%08x)\r\n", t->iWaitState.iWtC.iWtStFlags, t->iWaitState.iWtC.iWtObjType,
											t->iWaitState.iWtC.iWtStSpare1, t->iWaitState.iWtC.iWtStSpare2, t->iWaitState.iWtC.iWtObj);
	Printf("BasePri %d MutexPri %d NomPri %d Att=%02x\r\n", t->iBasePri, t->iMutexPri, t->iNominalPri, t->i_ThrdAttr);
	Printf("HeldFM=%08x FMDef=%02x AddrSp=%08x Initial=%d\r\n", t->iHeldFastMutex, t->iFastMutexDefer, t->iAddressSpace, t->iInitial);
	Printf("Time=%d Timeslice=%d ReqCount=%08x\r\n", t->iTime, t->iTimeslice, t->iRequestSemaphore.iCount);
	Printf("SuspendCount=%d CsCount=%d CsFunction=%08x\r\n", t->iSuspendCount, t->iCsCount, t->iCsFunction);
	Printf("LinkedObjType %02x LinkedObj %08x CoreCyc %02x RebalAttr %02x\r\n", t->iLinkedObjType, t->iLinkedObj, t->iCoreCycling, t->iRebalanceAttr);
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
			DisplayNThreadStackedRegs(*this, reg);
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
	m.Printf("LbQ     %08x %08x   LbCtr %02x\r\n", ss.iLbQ.iA.iNext, ss.iLbQ.iA.iPrev, ss.iLbCounter);
	m.Printf("AddrSp  %08x RschdIPIs %08x iNextIPI %08x\r\n", ss.iAddressSpace, ss.iReschedIPIs, ss.iNextIPI);
	m.DisplaySpinLock("ReadyListLock", &ss.iReadyListLock);
	m.Printf("EvtHand %08x %08x InitThrd %08x SLOC %08x %08x\r\n", ss.iEventHandlers.iA.iNext, ss.iEventHandlers.iA.iPrev,
		ss.iInitialThread, I64HIGH(ss.iSpinLockOrderCheck), I64LOW(ss.iSpinLockOrderCheck));
	m.DisplaySpinLock("EventHandlerLock", &ss.iEventHandlerLock);
	m.Printf("LastTmstmp %08x %08x            RschdCount %08x %08x\r\n", ss.iLastTimestamp.i32[1], ss.iLastTimestamp.i32[0], ss.iReschedCount.i32[1], ss.iReschedCount.i32[0]);

	m.Printf("DeferShutdown %4d RdyThrdC %d Uncached %08x\r\n", ss.iDeferShutdown, ss.iRdyThreadCount, ss.iUncached);
	DisplaySubSchedulerExt(m, ss);

	volatile TUint32* ssc = (volatile TUint32*)&ss.iSubSchedScratch[0];
	m.Printf("Scratch 0: %08x  1: %08x  2: %08x  3: %08x\r\n", ssc[ 0], ssc[ 1], ssc[ 2], ssc[ 3]);
	m.Printf("Scratch 4: %08x  5: %08x  6: %08x  7: %08x\r\n", ssc[ 4], ssc[ 5], ssc[ 6], ssc[ 7]);
	m.Printf("Scratch 8: %08x  9: %08x  A: %08x  B: %08x\r\n", ssc[ 8], ssc[ 9], ssc[10], ssc[11]);
	m.Printf("Scratch C: %08x  D: %08x  E: %08x  F: %08x\r\n", ssc[12], ssc[13], ssc[14], ssc[15]);

	TPriListBase* b = (TPriListBase*)&ss;
	m.Printf("PriClassThrdC %4d %4d %4d %4d\r\n", ss.iPriClassThreadCount[0], ss.iPriClassThreadCount[1], ss.iPriClassThreadCount[2], ss.iPriClassThreadCount[3]);
	m.Printf("Present %08x %08x\r\n", b->iPresent[1], b->iPresent[0]);
	TInt k;
	TUint64 p64 = b->iPresent64;
	for (k=KNumPriorities-1; k>=0; --k, p64+=p64)
		{
		if (p64>>63)
			{
			m.Printf("Priority %2d -> %08x\r\n", k, ss.EntryAtPriority(k));
			}
		}
	}

void Monitor::DisplaySchedulerInfo()
	{
	TScheduler* pS=TScheduler::Ptr();
	Printf("SCHEDULER @%08x:\r\n",pS);
	Printf("ProcessHandler    %08x MonitorExcHndlr %08x ReschedHook %08x\r\n",pS->iProcessHandler,pS->iMonitorExceptionHandler,pS->iRescheduleHook);
	Printf("iThreadAcceptCpus %08x  iIpiAcceptCpus %08x    iNumCpus %d\r\n",pS->iThreadAcceptCpus,pS->iIpiAcceptCpus,pS->iNumCpus);
	Printf("iCpusComingUp     %08x  iCpusGoingDown %08x  CCDeferCnt %d\r\n",pS->iCpusComingUp,pS->iCpusGoingDown,pS->iCCDeferCount);
	Printf("iCCSyncCpus       %08x  CCReactiv8Cpus %08x  CCState    %08x\r\n",pS->iCCSyncCpus,pS->iCCReactivateCpus,pS->iCCState);
	Printf("IdleDfcs %08x %08x     CpusNotIdle %08x     IdleGen %02x IdleSpillCpu %02x\r\n",
		pS->iIdleDfcs.iA.iNext, pS->iIdleDfcs.iA.iPrev, pS->iCpusNotIdle, pS->iIdleGeneration, pS->iIdleSpillCpu);
	DisplaySpinLock("IdleSpinLock", &pS->iIdleSpinLock);
	Printf("SYSLOCK @ %08x\r\n",&pS->iLock);
	DisplayNFastMutexInfo(&pS->iLock);

	DisplaySpinLock("EnumerateLock", &pS->iEnumerateLock);
	volatile TUint32* at = (volatile TUint32*)&pS->iAllThreads;
	volatile TUint32* ag = (volatile TUint32*)&pS->iAllGroups;
	Printf("AllThrds %08x %08x       AllGroups %08x %08x\r\n", at[0], at[1], ag[0], ag[1]);
	DisplaySpinLock("IdleBalanceLock", &pS->iIdleBalanceLock);
	DisplaySpinLock("BalanceListLock", &pS->iBalanceListLock);
	Printf("BalList  %08x %08x LastBalanceTime %08x %08x\r\n", pS->iBalanceList.iA.iNext, pS->iBalanceList.iA.iPrev,
									I64HIGH(pS->iLastBalanceTime), I64LOW(pS->iLastBalanceTime));
	Printf("LbCntr   %02x RebalanceDfcQ %08x NeedBal %02x\r\n", pS->iLbCounter, pS->iRebalanceDfcQ, pS->iNeedBal);
	Printf("iCCRequestLevel   %08x  iPoweringOff   %08x  DetachCnt  %08x\r\n",pS->iCCRequestLevel,pS->iPoweringOff,pS->iDetachCount);

	DisplaySchedulerExt(*this, *pS);
	volatile TUint32* sc = (volatile TUint32*)&pS->iSchedScratch[0];
	Printf("Scratch 0: %08x  1: %08x  2: %08x  3: %08x\r\n", sc[ 0], sc[ 1], sc[ 2], sc[ 3]);
	Printf("Scratch 4: %08x  5: %08x  6: %08x  7: %08x\r\n", sc[ 4], sc[ 5], sc[ 6], sc[ 7]);
	Printf("Scratch 8: %08x  9: %08x  A: %08x  B: %08x\r\n", sc[ 8], sc[ 9], sc[10], sc[11]);
	Printf("Scratch C: %08x  D: %08x  E: %08x  F: %08x\r\n", sc[12], sc[13], sc[14], sc[15]);

	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = *pS->iSub[i];
		DisplaySubSchedulerInfo(*this, ss);
		}
	NewLine();
	}

