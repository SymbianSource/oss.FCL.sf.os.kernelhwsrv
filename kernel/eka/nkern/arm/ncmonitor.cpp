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
// e32\nkern\arm\ncmonitor.cpp
// Kernel crash debugger - NKERN ARM specific portion
// 
//

#define __INCLUDE_REG_OFFSETS__  // for SP_R13U in nk_plat.h

#include <kernel/monitor.h>
#include "nk_priv.h"
#include <arm.h>

void DisplayNThreadPlatformSpecific(Monitor& m, NThread* pT)
	{
	m.Printf("iUserContextType=%02x ExtraContext=%08x ExtraContextSize=%04x\r\n",pT->iSpare3,pT->iExtraContext,pT->iExtraContextSize);
	if (pT != TScheduler::Ptr()->iCurrentThread)
		{
		TUint32* pS=(TUint32*)pT->iSavedSP;
		TUint32 reg[12+EXTRA_STACK_SPACE/4];
		MTRAPD(r,wordmove(reg,pS,48+EXTRA_STACK_SPACE));
		if (r==KErrNone)
			{
			TUint32* pR=reg;
#ifdef __CPU_SUPPORT_THUMB2EE
			m.Printf("TEEHBR %08x ",*pR++);
#endif
#ifdef __CPU_HAS_CP15_THREAD_ID_REG
			m.Printf("RWRWTID %08x ",*pR++);
#endif
#ifdef __CPU_HAS_VFP
			m.Printf("FPEXC %08x ",*pR++);
#endif
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
			m.Printf("CAR %08x ",*pR++);
#endif
#ifdef __CPU_ARM_USE_DOMAINS
			m.Printf("DACR %08x\r\n",*pR++);
#endif
			m.Printf("R13_USR %08x R14_USR %08x SPSR_SVC %08x\r\n",pR[0],pR[1],pR[2]);
			m.Printf(" R4 %08x  R5 %08x  R6 %08x  R7 %08x\r\n",pR[3],pR[4],pR[5],pR[6]);
			m.Printf(" R8 %08x  R9 %08x R10 %08x R11 %08x\r\n",pR[7],pR[8],pR[9],pR[10]);
			m.Printf(" PC %08x\r\n",pR[11]);
			}
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
	SFullArmRegSet& r = *(SFullArmRegSet*)iRegs;
	DumpRegisters(*this, r);
	}

EXPORT_C void Monitor::DisplayCpuFaultInfo()
	{
	TScheduler* pS = TScheduler::Ptr();
	SFullArmRegSet& r = *(SFullArmRegSet*)pS->i_Regs;
	if (TUint(r.iExcCode) > TUint(EArmExceptionUndefinedOpcode))
		return;
	Printf("Exc %1d Cpsr=%08x FAR=%08x FSR=%08x\r\n",  r.iExcCode, r.iN.iFlags, r.iB[0].iDFAR, r.iB[0].iDFSR);
	Printf(" R0=%08x  R1=%08x  R2=%08x  R3=%08x\r\n",  r.iN.iR0,  r.iN.iR1,  r.iN.iR2,  r.iN.iR3);
	Printf(" R4=%08x  R5=%08x  R6=%08x  R7=%08x\r\n",  r.iN.iR4,  r.iN.iR5,  r.iN.iR6,  r.iN.iR7);
	Printf(" R8=%08x  R9=%08x R10=%08x R11=%08x\r\n",  r.iN.iR8,  r.iN.iR9,  r.iN.iR10, r.iN.iR11);
	Printf("R12=%08x R13=%08x R14=%08x R15=%08x\r\n",  r.iN.iR12, r.iN.iR13, r.iN.iR14, r.iN.iR15);
	Printf("R13Svc=%08x R14Svc=%08x SpsrSvc=%08x\r\n", r.iN.iR13Svc, r.iN.iR14Svc, r.iN.iSpsrSvc);
	}

EXPORT_C void Monitor::GetStackPointers(NThread* aThread, TUint& aSupSP, TUint& aUsrSP)
	{
	TScheduler* pS = TScheduler::Ptr();
	if (aThread == pS->iCurrentThread)
		{
		SFullArmRegSet& r = *(SFullArmRegSet*)iRegs;
		aSupSP = r.iN.iR13Svc;
		aUsrSP = r.iN.iR13;
		}
	else
		{
		TUint32* sp = (TUint32*)aThread->iSavedSP;
		aSupSP = (TUint)sp;
		aUsrSP = sp[SP_R13U];
		}
	}
