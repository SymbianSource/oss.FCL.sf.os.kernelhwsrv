// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\x86\ncmonitor.cpp
// Kernel crash debugger - NKERN ARM specific portion
// 
//

#include "nk_priv.h"
#include <x86.h>
#include <kernel/monitor.h>


void DisplayNThreadPlatformSpecific(Monitor& m, NThread* pT)
	{
	m.Printf("ExcInUserMode=%02x\r\n",pT->iSpare3);
	if (pT != TScheduler::Ptr()->iCurrentThread)
		{
		TUint32* pS=(TUint32*)pT->iSavedSP;
		SThreadStack reg;
		MTRAPD(r,wordmove(&reg,pS,sizeof(SThreadStack)));
		if (r==KErrNone)
			{
			m.Printf("EBX %08x ESI %08x EDI %08x EBP %08x\r\n",reg.iEbx,reg.iEsi,reg.iEdi,reg.iEbp);
			m.Printf("EIP %08x CR0 %08x  FS %08x  GS %08x\r\n",reg.iEip,reg.iCR0,reg.iFs,reg.iGs);
			}
		}
	}

void Monitor::DumpCpuRegisters()
	{
	SFullX86RegSet& r=*(SFullX86RegSet*)iRegs;
	Printf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\r\n",r.iEax,r.iEbx,r.iEcx,r.iEdx);
	Printf("ESP=%08x EBP=%08x ESI=%08x EDI=%08x\r\n",r.iEsp,r.iEbp,r.iEsi,r.iEdi);
	Printf(" CS=%08x EIP=%08x EFL=%08x  SS=%08x\r\n",r.iCs,r.iEip,r.iEflags,r.iSs);
	Printf(" DS=%08x  ES=%08x  FS=%08x  GS=%08x\r\n",r.iDs,r.iEs,r.iFs,r.iGs);
	Printf("IrqNest=%08x\r\n",r.iIrqNestCount);
	NewLine();
	}

void Monitor::DisplayCpuFaultInfo()
	{
	TScheduler* pS = TScheduler::Ptr();
	TAny* p = pS->i_ExcInfo;
	if (!p)
		return;
	TX86ExcInfo& a=*(TX86ExcInfo*)p;
	Printf("Exc %02x EFLAGS=%08x FAR=%08x ErrCode=%08x\r\n",a.iExcId,a.iEflags,a.iFaultAddress,a.iExcErrorCode);
	Printf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\r\n",a.iEax,a.iEbx,a.iEcx,a.iEdx);
	Printf("ESP=%08x EBP=%08x ESI=%08x EDI=%08x\r\n",a.iEsp,a.iEbp,a.iEsi,a.iEdi);
	Printf(" CS=%08x EIP=%08x  DS=%08x  SS=%08x\r\n",a.iCs,a.iEip,a.iDs,a.iSs);
	Printf(" ES=%08x  FS=%08x  GS=%08x\r\n",a.iEs,a.iFs,a.iGs);
	if (a.iCs&3)
		{
		Printf("SS3=%08x ESP3=%08x\r\n",a.iSs3,a.iEsp3);
		}
	}

EXPORT_C void Monitor::GetStackPointers(NThread* aThread, TUint& aSupSP, TUint& aUsrSP)
	{
	TScheduler* pS = TScheduler::Ptr();
	if (aThread == pS->iCurrentThread)
		aSupSP = ((SFullX86RegSet*)iRegs)->iEsp;
	else
		aSupSP = (TUint)aThread->iSavedSP;

	// User SP is in the exception info on the top of the supervisor stack
	TUint stackTop = (TUint)aThread->iStackBase + aThread->iStackSize;
	TX86ExcInfo* excRegs = (TX86ExcInfo*)(stackTop-sizeof(TX86ExcInfo));
	aUsrSP = excRegs->iEsp3;
	}
