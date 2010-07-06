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
// Kernel crash debugger - NKERNSMP X86 specific portion
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include "nk_priv.h"
#include <x86.h>
#include <kernel/monitor.h>


void DisplayNThreadStackedRegs(Monitor& m, SThreadReschedStack& reg)
	{
	m.Printf("CR0 %08x  RsF %08x  EIP %08x  RSN %08x\r\n", reg.iCR0, reg.iReschedFlag, reg.iEip, reg.iReason);
	}

void DisplaySubSchedulerExt(Monitor& m, TSubScheduler& ss)
	{
	TSubSchedulerX& x = ss.iSSX;
	m.Printf("Extras[ 0] %08x Extras[ 1] %08x Extras[ 2] %08x Extras[ 3] %08x\r\n", x.iSSXP[0], x.iSSXP[1], x.iSSXP[2], x.iSSXP[3]);
	m.Printf("Extras[ 4] %08x Extras[ 5] %08x Extras[ 6] %08x Extras[ 7] %08x\r\n", x.iSSXP[4], x.iSSXP[5], x.iSSXP[6], x.iSSXP[7]);
	m.Printf("Extras[ 8] %08x i_IrqCount %08x i_ExcInfo  %08x i_CrashSt  %08x\r\n", x.iSSXP[8], x.iIrqCount, x.iExcInfo, x.iCrashState);
	m.Printf("i_APICID   %08x i_IrqNestC %08x i_IrqStkTp %08x i_Tss      %08x\r\n", x.iAPICID, x.iIrqNestCount, x.iIrqStackTop, x.iTss);
	m.Printf("CpuFreqM   %08x CpuFreqS   %08x CpuPeriodM %08x CpuPeriodS %08x\r\n", x.iCpuFreqRI.iR.iM, x.iCpuFreqRI.iR.iX, x.iCpuFreqRI.iI.iM, x.iCpuFreqRI.iI.iX);
	m.Printf("TmrFreqM   %08x TmrFreqS   %08x TmrPeriodM %08x TmrPeriodS %08x\r\n", x.iTimerFreqRI.iR.iM, x.iTimerFreqRI.iR.iX, x.iTimerFreqRI.iI.iM, x.iTimerFreqRI.iI.iX);
	m.Printf("TmstampOff %08x %08x            iSSXP2[0]  %08x iSSXP2[1]  %08x\r\n", I64HIGH(x.iTimestampOffset.i64), I64LOW(x.iTimestampOffset.i64), x.iSSXP2[0], x.iSSXP2[1]);
	}

void DisplaySchedulerExt(Monitor& m, TScheduler& s)
	{
	volatile TUint32* sx = (volatile TUint32*)&s.iSX;
	m.Printf("Extras  0: %08x  1: %08x  2: %08x  3: %08x\r\n",sx[0],sx[1],sx[2],sx[3]);
	m.Printf("Extras  4: %08x  5: %08x  6: %08x  7: %08x\r\n",sx[4],sx[5],sx[6],sx[7]);
	m.Printf("Extras  8: %08x  9: %08x  A: %08x  B: %08x\r\n",sx[8],sx[9],sx[10],sx[11]);
	m.Printf("Extras  C: %08x  D: %08x  E: %08x  F: %08x\r\n",sx[12],sx[13],sx[14],sx[15]);
	}

SFullX86RegSet& RegSet(TInt aCpu)
	{
	TScheduler* pS=TScheduler::Ptr();
	TSubScheduler& ss = *pS->iSub[aCpu];
	SCpuData* cpudata = (SCpuData*)ss.iSSX.iTss;
	return cpudata->iRegs;
	}

TX86Tss& TSS(TInt aCpu)
	{
	TScheduler* pS=TScheduler::Ptr();
	TSubScheduler& ss = *pS->iSub[aCpu];
	return *ss.iSSX.iTss;
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
		if (!ss.iSSX.iExcInfo)
			continue;
		TX86ExcInfo& a = *(TX86ExcInfo*)ss.iSSX.iExcInfo;
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
