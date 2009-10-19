// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\x86\ncmonitor.cpp
// Kernel crash debugger - NKERN X86 specific portion
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"
#include <x86.h>
#include <kernel/monitor.h>


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
			Printf("CR0 %08x  RsF %08x  EIP %08x  RSN %08x\r\n", reg.iCR0, reg.iReschedFlag, reg.iEip, reg.iReason);
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
	m.Printf("\r\nSUBSCHEDULER %d @%08x:\r\n",ss.iCpuNum,&ss);
	m.Printf("CurrentThread=%08x\r\n", ss.iCurrentThread);
	m.Printf("DFCS %08x %08x CPU# %08x CPUmask %08x\r\n", ss.iDfcs.iA.iNext, ss.iDfcs.iA.iPrev, ss.iCpuNum, ss.iCpuMask);
	m.Printf("KLCount %d DFCPending %02x ReschedNeeded %02x InIDFC %02x\r\n", ss.iKernLockCount, ss.iDfcPendingFlag, ss.iRescheduleNeededFlag, ss.iInIDFC);
	m.Printf("AddressSpace=%08x ReschedIPIs=%08x iNextIPI %08x\r\n", ss.iAddressSpace, ss.iReschedIPIs, ss.iNextIPI);
	volatile TUint32* pL = (volatile TUint32*)&ss.iReadyListLock;
	m.Printf("SpinLock %08x %08x %08x %08x\r\n", pL[0], pL[1], pL[2], pL[3]);
	m.Printf("Extras[ 0] %08x Extras[ 1] %08x Extras[ 2] %08x Extras[ 3] %08x\r\n", ss.iExtras[0], ss.iExtras[1], ss.iExtras[2], ss.iExtras[3]);
	m.Printf("Extras[ 4] %08x Extras[ 5] %08x Extras[ 6] %08x Extras[ 7] %08x\r\n", ss.iExtras[4], ss.iExtras[5], ss.iExtras[6], ss.iExtras[7]);
	m.Printf("Extras[ 8] %08x i_IrqCount %08x i_ExcInfo  %08x i_CrashSt  %08x\r\n", ss.iExtras[8], ss.i_IrqCount, ss.i_ExcInfo, ss.i_CrashState);
	m.Printf("i_APICID   %08x i_IrqNestC %08x i_IrqStkTp %08x i_Tss      %08x\r\n", ss.i_APICID, ss.i_IrqNestCount, ss.i_IrqStackTop, ss.i_Tss);
	m.Printf("i_TmrMultF %08x i_TmrMultI %08x i_CpuMult  %08x Extras[13] %08x\r\n", ss.i_TimerMultF, ss.i_TimerMultI, ss.i_CpuMult, ss.iExtras[19]);
	m.Printf("i_TmOffL   %08x i_TmOffH   %08x Extras[16] %08x Extras[17] %08x\r\n", ss.iExtras[20], ss.iExtras[21], ss.iExtras[22], ss.iExtras[23]);
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

SFullX86RegSet& RegSet(TInt aCpu)
	{
	TScheduler* pS=TScheduler::Ptr();
	TSubScheduler& ss = *pS->iSub[aCpu];
	SCpuData* cpudata = (SCpuData*)ss.i_Tss;
	return cpudata->iRegs;
	}

TX86Tss& TSS(TInt aCpu)
	{
	TScheduler* pS=TScheduler::Ptr();
	TSubScheduler& ss = *pS->iSub[aCpu];
	return *(TX86Tss*)ss.i_Tss;
	}

void Monitor::DumpCpuRegisters()
	{
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		SFullX86RegSet& r = RegSet(i);
		Printf("CPU %d:\r\n", i);
		Printf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\r\n",r.iEax,r.iEbx,r.iEcx,r.iEdx);
		Printf("ESP=%08x EBP=%08x ESI=%08x EDI=%08x\r\n",r.iEsp,r.iEbp,r.iEsi,r.iEdi);
		Printf(" CS=%08x EIP=%08x EFL=%08x  SS=%08x\r\n",r.iCs,r.iEip,r.iEflags,r.iSs);
		Printf(" DS=%08x  ES=%08x  FS=%08x  GS=%08x\r\n",r.iDs,r.iEs,r.iFs,r.iGs);
		Printf("IrqNest=%08x\r\n",r.iIrqNestCount);
		NewLine();
		}
	}

void Monitor::DisplayCpuFaultInfo()
	{
	TScheduler* pS=TScheduler::Ptr();
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		Printf("CPU %d:\r\n", i);
		TSubScheduler& ss = *pS->iSub[i];
		if (!ss.i_ExcInfo)
			continue;
		TX86ExcInfo& a = *(TX86ExcInfo*)ss.i_ExcInfo;
		Printf("Exc %02x EFLAGS=%08x FAR=%08x ErrCode=%08x\r\n",a.iExcId,a.iEflags,a.iFaultAddress,a.iExcErrorCode);
		Printf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\r\n",a.iEax,a.iEbx,a.iEcx,a.iEdx);
		Printf("ESP=%08x EBP=%08x ESI=%08x EDI=%08x\r\n",a.iEsp,a.iEbp,a.iEsi,a.iEdi);
		Printf(" CS=%08x EIP=%08x  DS=%08x  SS=%08x\r\n",a.iCs,a.iEip,a.iDs,a.iSs);
		Printf(" ES=%08x  FS=%08x  GS=%08x\r\n",a.iEs,a.iFs,a.iGs);
		if (a.iCs&3)
			{
			Printf("SS3=%08x ESP3=%08x\r\n",a.iSs3,a.iEsp3);
			}
		NewLine();
		}
	}

EXPORT_C void Monitor::GetStackPointers(NThread* aThread, TUint& aSupSP, TUint& aUsrSP)
	{
	TScheduler* pS = TScheduler::Ptr();
	TBool user_done = FALSE;
	if (aThread->iCurrent)
		{
		TInt i = aThread->iLastCpu;
		if (RegSet(i).iCs & 3)
			aSupSP = RegSet(i).iEsp;
		else
			{
			aUsrSP = RegSet(i).iEsp;
			aSupSP = TSS(i).iEsp0;
			user_done = TRUE;
			}
		}
	else
		aSupSP = (TUint)aThread->iSavedSP;

	if (!user_done)
		{
		// User SP is in the exception info on the top of the supervisor stack
		TUint stackTop = (TUint)aThread->iStackBase + aThread->iStackSize;
		TX86ExcInfo* excRegs = (TX86ExcInfo*)(stackTop - sizeof(TX86ExcInfo));
		aUsrSP = excRegs->iEsp3;
		}
	}
