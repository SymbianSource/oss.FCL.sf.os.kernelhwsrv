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
// e32\nkernsmp\x86\nccpu.cpp
// 
//

#include <x86.h>
#include <apic.h>


const TLinAddr KWarmResetTrampolineAddr = 0x467;

TLinAddr ApTrampolinePage = 0;		// overridden in multiple memory model with linear address

extern "C" void nanowait(TUint32 aNanoseconds);
void cmos_write(TUint32 val, TUint32 addr);
void SetupApInitInfo(volatile SApInitInfo&);
void _ApMain();

TInt WakeAP(TInt aAPICID)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("WakeAP %d", aAPICID));
	read_apic_reg(SIVR);
	write_apic_reg(ESR, 0);
	read_apic_reg(ESR);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("Asserting INIT"));

	//Turn INIT on target chip
	write_apic_reg(ICRH, aAPICID<<24);

	// Send IPI
	write_apic_reg(ICRL, 0xC500);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("Waiting for send to finish..."));
	TInt timeout = 0;
	TUint32 send_status;
	TUint32 accept_status;
	do	{
		__KTRACE_OPT(KBOOT,DEBUGPRINT("+"));
		nanowait(100000);
		send_status = read_apic_reg(ICRL) & 0x1000;
		} while (send_status && (++timeout < 1000));

	nanowait(10000000);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("Deasserting INIT"));

	//	Target chip
	write_apic_reg(ICRH, aAPICID<<24);

	//	Send IPI
	write_apic_reg(ICRL, 0x8500);

	__KTRACE_OPT(KBOOT,DEBUGPRINT("Waiting for send to finish..."));
	timeout = 0;
	do	{
		__KTRACE_OPT(KBOOT,DEBUGPRINT("+"));
		nanowait(100000);
		send_status = read_apic_reg(ICRL) & 0x1000;
		} while (send_status && (++timeout < 1000));

	/*
	 * Should we send STARTUP IPIs ?
	 *
	 * Determine this based on the APIC version.
	 * If we don't have an integrated APIC, don't send the STARTUP IPIs.
	 */
//	if (APIC_INTEGRATED(apic_version[phys_apicid]))
	TInt num_starts = 2;
//	else
//		num_starts = 0;

	// Run STARTUP IPI loop.
//	maxlvt = get_maxlvt();

	TInt j;
	for (j = 1; j <= num_starts; j++)
		{
		__KTRACE_OPT(KBOOT,DEBUGPRINT("Sending STARTUP %d",j));
		read_apic_reg(SIVR);
		write_apic_reg(ESR, 0);
		read_apic_reg(ESR);

		// target chip
		write_apic_reg(ICRH, aAPICID<<24);

		// send startup IPI
		write_apic_reg(ICRL, (0x600 | (KApBootPage>>12)));

		// give other CPU time to accept it
		nanowait(300000);

		__KTRACE_OPT(KBOOT,DEBUGPRINT("Waiting for send to finish..."));
		timeout = 0;
		do	{
			__KTRACE_OPT(KBOOT,DEBUGPRINT("+"));
			nanowait(100000);
			send_status = read_apic_reg(ICRL) & 0x1000;
			} while (send_status && (++timeout < 1000));

		// give other CPU time to accept it
		nanowait(300000);

		/*
		 * Due to the Pentium erratum 3AP.
		 */
//		if (maxlvt > 3) {
//			read_apic_reg(APIC_SPIV);
//			write_apic_reg(APIC_ESR, 0);
//		}
		accept_status = (read_apic_reg(ESR) & 0xEF);
		if (send_status || accept_status)
			break;
		}
	__KTRACE_OPT(KBOOT,DEBUGPRINT("After startup"));

	if (send_status)
		__KTRACE_OPT(KBOOT,DEBUGPRINT("APIC never delivered???"));
	if (accept_status)
		__KTRACE_OPT(KBOOT,DEBUGPRINT("APIC delivery error %x", accept_status));

	return (send_status | accept_status);
	}



TInt NKern::BootAP(volatile SAPBootInfo* aInfo)
	{
	__KTRACE_OPT(KBOOT,DEBUGPRINT("NKern::BootAP %08x %08x+%x", aInfo->iCpu, aInfo->iInitStackBase, aInfo->iInitStackSize));

	cmos_write(0xa, 0xf);

	TUint8* t = (TUint8*)(KWarmResetTrampolineAddr + ApTrampolinePage);
	TUint cs = KApBootPage>>4;
	*t++ = 0;	// IP low
	*t++ = 0;	// IP high
	*t++ = (TUint8)cs;
	*t++ = cs>>8;

	volatile SApInitInfo& a = *(volatile SApInitInfo*)KApBootPage;
	TCpuPages& cp=X86::CpuPage();
	SetupApInitInfo(a);
	memcpy((TAny*)a.iTempGdt, cp.iGdt, sizeof(cp.iGdt));
	a.iTempGdtr = TUint64(KApBootPage + _FOFF(SApInitInfo,iTempGdt))<<16 | TUint64(KSmpGdtSize*sizeof(SX86Des)-1);
	a.iRgs.iCs = RING0_CS;
	a.iRgs.iEip = (TLinAddr)&_ApMain;
	a.iBootFlag = 0;
	a.iBootFlag2 = 0;
	a.iLinAddr = (TLinAddr)&a;

	a.iStackBase = aInfo->iInitStackBase;
	a.iStackSize = aInfo->iInitStackSize;
	a.iRgs.iEsp = a.iStackBase + a.iStackSize;
	a.iExtra = (TAny*)aInfo;

	TInt r = WakeAP(aInfo->iCpu);
	if (r!=0)
		return KErrGeneral;

	TInt timeout = 500;
	while (--timeout)
		{
		nanowait(1000000);
		if (a.iBootFlag == KBootFlagMagic-1)
			break;
		__chill();
		}
	__KTRACE_OPT(KBOOT, DEBUGPRINT("iBootFlag=%08x",a.iBootFlag));
	if (timeout==0)
		return KErrTimedOut;

	__e32_atomic_add_ord32(&a.iBootFlag, TUint32(-1));

	NKern::DisableAllInterrupts();
	while (a.iBootFlag2==0)
		{}
	__e32_io_completion_barrier();
	a.iBootFlag2 = 2;
	__e32_io_completion_barrier();
	a.iBPTimestamp = X86::Timestamp();
	__e32_io_completion_barrier();
	a.iBootFlag2 = 3;
	__e32_io_completion_barrier();
	while (a.iBootFlag2==3)
		{}
	__e32_io_completion_barrier();
	NKern::EnableAllInterrupts();

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
	volatile SApInitInfo& a = *(volatile SApInitInfo*)KApBootPage;
	NKern::DisableAllInterrupts();
	a.iBootFlag2 = 1;
	__e32_io_completion_barrier();
	while (a.iBootFlag2==1)
		{}
	__e32_io_completion_barrier();
	a.iAPTimestamp = X86::Timestamp();
	__e32_io_completion_barrier();
	while (a.iBootFlag2!=3)
		{}
	__e32_io_completion_barrier();
	TUint64 bpt = a.iBPTimestamp;
	TUint64 apt = a.iAPTimestamp;
	TUint64 delta = bpt - apt;
	aSS->iSSX.iTimestampOffset.i64 = delta;
	TUint64 now = NKern::Timestamp();
	__KTRACE_OPT(KBOOT,DEBUGPRINT("APT=0x%lx BPT=0x%lx Delta=0x%lx", apt, bpt, delta));
	__e32_io_completion_barrier();
	a.iBootFlag2 = 4;
	NKern::EnableAllInterrupts();
	t->iLastActivationTime.i64 = now;
	aSS->iLastTimestamp.i64 = now;
	}



