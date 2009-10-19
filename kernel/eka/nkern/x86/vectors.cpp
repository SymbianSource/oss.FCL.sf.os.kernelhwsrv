// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkern\x86\vectors.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <x86.h>
#include "vectors.h"

#ifdef _DEBUG
#define __CHECK_LOCK_STATE__
#endif

void __X86VectorIrq();
void __X86VectorExc();
void __X86ExcFault(TAny*);


/** Register the global IRQ handler
	Called by the base port at boot time to bind the top level IRQ dispatcher
	to the X86 common IRQ handler. Should not be called at any other time.

	The handler specified will be called with IRQs disabled. ESP will point
	to the top of the interrupt stack. On entry to the handler EAX will point
	to a block of saved registers, as follows:

	[EAX+00h] = saved EDX
	[EAX+04h] = saved ECX
	[EAX+08h] = saved EAX
	[EAX+0Ch] = saved ES
	[EAX+10h] = saved DS
	[EAX+14h] = interrupt vector number
	[EAX+18h] = return EIP
	[EAX+1Ch] = return CS
	[EAX+20h] = return EFLAGS
	[EAX+24h] = return ESP if interrupt occurred while CPL>0
	[EAX+28h] = return SS if interrupt occurred while CPL>0

	The handler should preserve all registers other than EAX, ECX, EDX
	and should return using a standard RET instruction.

	@param	aHandler The address of the top level IRQ dispatcher routine
 */
EXPORT_C void X86::SetIrqHandler(TLinAddr aHandler)
	{
	X86_IrqHandler=aHandler;
	}


/** Return the address immediately after the end of the interrupt stack.

	@return Interrupt Stack Base + Interrupt Stack Size
 */
EXPORT_C TLinAddr X86::IrqStackTop(TInt /*aCpu*/)
	{
	return TLinAddr(X86_IrqStack) + IRQ_STACK_SIZE;
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
	memset(X86_IrqStack,0xaa,IRQ_STACK_SIZE);

#ifndef __STANDALONE_NANOKERNEL__
	TStackInfo& stackInfo =  TheSuperPage().iStackInfo;
	stackInfo.iIrqStackBase = X86_IrqStack;
	stackInfo.iIrqStackSize = IRQ_STACK_SIZE;
#endif
	
	TCpuPage& cp=X86::CpuPage();
	memclr(cp.iIdt, KIdtSize*sizeof(SX86Des));
	TInt i;
	for (i=0; i<64; i++)
		{
		if (i==0x03 || i==0x20 || i==0x21)
			SetTrapGate(cp.iIdt+i, TheExcVectors[i], 3);
		else if (i<0x20)
			SetTrapGate(cp.iIdt+i, TheExcVectors[i], 0);
		if (i>=0x30)
			SetInterruptGate(cp.iIdt+i, TheExcVectors[i], 0);
		}
	X86_IrqNestCount=-1;
	X86::DefaultCR0=get_cr0();
	memclr(&cp.iTss,sizeof(TX86Tss));
	cp.iTss.iCR3=get_cr3();
	cp.iTss.iSs0=KRing0DS;
	cp.iTss.iEsp0=get_esp();
	SetTssDescriptor(&cp.iGdt[5],&cp.iTss);
	X86_TSS_Ptr=&cp.iTss;
	__lidt(cp.iIdt,KIdtSize);
	__KTRACE_OPT(KBOOT,DEBUGPRINT("<X86::Init1Interrupts()"));
	}


/**	Return the current processor context type (thread, IDFC or interrupt)

	@return	A value from NKern::TContext enumeration (but never EEscaped)
	@pre	Any context

	@see	NKern::TContext
 */
EXPORT_C TInt NKern::CurrentContext()
	{
	if (X86_IrqNestCount >= 0)
		return NKern::EInterrupt;
	if (TheScheduler.iInIDFC)
		return NKern::EIDFC;
	return NKern::EThread;
	}

extern "C" void ExcFault(TAny*);
void __X86ExcFault(TAny* aInfo)
	{
	ExcFault(aInfo);
	}

