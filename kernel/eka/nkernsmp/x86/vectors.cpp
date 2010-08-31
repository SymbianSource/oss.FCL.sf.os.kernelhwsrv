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
// e32\nkernsmp\x86\vectors.cpp
// 
//

#include <x86.h>
#include <apic.h>
#include <nk_irq.h>
#include "vectors.h"

TUint32 __tr();
void __ltr(TInt);

extern "C" void _irqdebug(TUint a)
	{
	if (a>=0x30)
		return;
	__KTRACE_OPT(KSCHED2,DEBUGPRINT("!%02x",a));
	}

extern "C" void IrqStartTrace(TUint32 aVector)
	{
	__ACQUIRE_BTRACE_LOCK();
	BTraceData.iHandler(BTRACE_HEADER(8,BTrace::ECpuUsage,BTrace::EIrqStart),0,0,aVector,0,0,0,0);
	__RELEASE_BTRACE_LOCK();
	}

extern "C" void IrqEndTrace()
	{
	__ACQUIRE_BTRACE_LOCK();
	BTraceData.iHandler(BTRACE_HEADER(4,BTrace::ECpuUsage,BTrace::EIrqEnd),0,0,0,0,0,0,0);
	__RELEASE_BTRACE_LOCK();
	}


/** Register the global IRQ handler
	Called by the base port at boot time to bind the top level IRQ dispatcher
	to the X86 common IRQ handler. Should not be called at any other time.

	The handler specified will be called with IRQs disabled. ESP will point
	to the top of the interrupt stack. On entry to the handler EAX will point
	to a block of saved registers, as follows:

	[EAX+00h] = saved ESI
	[EAX+04h] = saved EDI
	[EAX+08h] = saved EDX
	[EAX+0Ch] = saved ECX
	[EAX+10h] = saved EAX
	[EAX+14h] = saved ES
	[EAX+18h] = saved DS
	[EAX+1Ch] = interrupt vector number
	[EAX+20h] = return EIP
	[EAX+24h] = return CS
	[EAX+28h] = return EFLAGS
	[EAX+2Ch] = return ESP if interrupt occurred while CPL>0
	[EAX+30h] = return SS if interrupt occurred while CPL>0

	The handler should preserve all registers other than EAX, ECX, EDX
	and should return using a standard RET instruction.

	@param	aHandler The address of the top level IRQ dispatcher routine
 */
EXPORT_C void X86::SetIrqHandler(TLinAddr aHandler)
	{
	X86_IrqHandler = aHandler;
	}


/** Return the address immediately after the end of the interrupt stack.

	@return Interrupt Stack Base + Interrupt Stack Size
 */
EXPORT_C TLinAddr X86::IrqStackTop(TInt aCpu)
	{
	TLinAddr a = 0;
	if (aCpu>=0 && aCpu<KMaxCpus)
		a = TLinAddr(TheSubSchedulers[aCpu].iSSX.iIrqStackTop);
	else
		{
		TInt irq = NKern::DisableAllInterrupts();
		a = TLinAddr(SubScheduler().iSSX.iIrqStackTop);
		NKern::RestoreInterrupts(irq);
		}
	return a;
	}

void SetTrapGate(SX86Des* aEntry, PFV aHandler, TInt aDPL)
	{
	aEntry->iLow=(KRing0CS<<16)|(TUint32(aHandler)&0xffff);
	aEntry->iHigh=(TUint32(aHandler)&0xffff0000) | 0x8f00 | (aDPL<<13);
	}

void SetInterruptGate(SX86Des* aEntry, PFV aHandler, TInt aDPL)
	{
	aEntry->iLow=(KRing0CS<<16)|(TUint32(aHandler)&0xffff);
	aEntry->iHigh=(TUint32(aHandler)&0xffff0000) | 0x8e00 | (aDPL<<13);
	}

void SetTssDescriptor(SX86Des* aEntry, TX86Tss* aTss)
	{
	TUint addr3=TUint(aTss)>>24;
	TUint addr2=(TUint(aTss)>>16)&0xff;
	TUint addr01=TUint(aTss)&0xffff;
	aEntry->iLow=(addr01<<16)|(sizeof(TX86Tss)-1);
	aEntry->iHigh=(addr3<<24)|0x00108900|addr2;
	}

void X86::Init1Interrupts()
//
// Initialise the interrupt and exception vector handlers.
//
	{
//	TheIrqHandler=0;	// done by placing TheIrqHandler, TheFiqHandler in .bss
	__KTRACE_OPT(KBOOT,DEBUGPRINT(">X86::Init1Interrupts()"));

	TCpuPages& cp=X86::CpuPage();
	memclr(cp.iIdt, KIdtSize*sizeof(SX86Des));
	TInt i;
	for (i=0; i<(TInt)(sizeof(TheExcVectors)/sizeof(PFV)); i++)
		{
		if (i==0x03 || i==0x20 || i==0x21)
			SetInterruptGate(cp.iIdt+i, TheExcVectors[i], 3);
		else if (i<0x20 && i!=0x02)
			SetInterruptGate(cp.iIdt+i, TheExcVectors[i], 0);
		if (i==0x02 || i>=0x27)
			SetInterruptGate(cp.iIdt+i, TheExcVectors[i], 0);
		}
	for (i=0; i<KMaxCpus; ++i)
		{
		SCpuData& cd = cp.iCpuData[i];
		memclr(&cd, sizeof(cd) - sizeof(cd.iIrqStack));
		memset(cd.iIrqStack, 0xab+i*2, sizeof(cd.iIrqStack));

		TUint32 esp = (TUint32)cd.iIrqStack + sizeof(cd.iIrqStack);
		cd.iTss.iCR3 = get_cr3();
		cd.iTss.iSs0 = KRing0DS;
		cd.iTss.iEsp0 = esp;
		SetTssDescriptor(&cp.iGdt[5+i], &cd.iTss);

		TSubScheduler& ss = TheSubSchedulers[i];
		ss.iSSX.iIrqNestCount = (TLinAddr)(-1);
		ss.iSSX.iIrqStackTop = (TLinAddr)esp;
		ss.iSSX.iTss = &cd.iTss;
		}

	X86::DefaultCR0 = get_cr0();
	NIrq::HwInit1();

	__ltr(TSS_SELECTOR(0));
	__lidt(cp.iIdt, KIdtSize);
	__KTRACE_OPT(KBOOT,DEBUGPRINT("<X86::Init1Interrupts()"));
	}


extern "C" void ExcFault(TAny*);
void __X86ExcFault(TAny* aInfo)
	{
	ExcFault(aInfo);
	}

