// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\x86\ncirq.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include "nk_priv.h"
#include "nk_plat.h"
#include <nk_irq.h>
#include <apic.h>

#ifdef _DEBUG
#define DMEMDUMP(base,size)	DbgMemDump((TLinAddr)base,size)
void DbgMemDump(TLinAddr aBase, TInt aSize)
	{
	TInt off;
	const TUint8* p=(const TUint8*)aBase;
	NKern::Lock();
	for (off=0; off<aSize; off+=16, p+=16)
		{
		DEBUGPRINT("%08x: %02x %02x %02x %02x  %02x %02x %02x %02x | %02x %02x %02x %02x  %02x %02x %02x %02x",
			p,		p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}
	NKern::Unlock();
	}
#else
#define DMEMDUMP(base,size)
#endif

#define	IO_APIC_BASE			0xFEC00000
#define	IO_APIC_REGSEL_OFFSET	0x00
#define	IO_APIC_IOWIN_OFFSET	0x10

#define	IO_APIC_REG_ID			0x00
#define	IO_APIC_REG_VER			0x01
#define	IO_APIC_REG_ARB			0x02

#define IO_APIC_CTRL_IMASK			0x10000		// 1 = masked
#define	IO_APIC_CTRL_LEVEL			0x08000		// 1 = level triggered, 0 = edge
#define	IO_APIC_CTRL_REMOTE_IRR		0x04000		//
#define	IO_APIC_CTRL_INTPOL_LOW		0x02000		// 1 = active low
#define	IO_APIC_CTRL_DELIVS			0x01000		//
#define	IO_APIC_CTRL_DESTMOD		0x00800		// 1 = logical, 0 = physical
#define	IO_APIC_CTRL_DELMOD_MASK	0x00700
#define	IO_APIC_CTRL_DELMOD_FIXED	0x00000
#define	IO_APIC_CTRL_DELMOD_LOWP	0x00100
#define	IO_APIC_CTRL_DELMOD_SMI		0x00200
#define	IO_APIC_CTRL_DELMOD_NMI		0x00400
#define	IO_APIC_CTRL_DELMOD_INIT	0x00500
#define	IO_APIC_CTRL_DELMOD_EXTINT	0x00700
#define	IO_APIC_CTRL_INTVEC_MASK	0x000FF


/******************************************************************************
 * IO APIC
 ******************************************************************************/

#define IO_APIC_SELECT(x)		((void)(*(volatile TUint32*)(iAddr + IO_APIC_REGSEL_OFFSET) = (x)))
#define	IO_APIC_REG				(*(volatile TUint32*)(iAddr + IO_APIC_IOWIN_OFFSET))

class TIoApic
	{
public:
	TIoApic(TLinAddr aAddr);
	TUint32 Id();
	TUint32 Ver();
	TUint32 Arb();
	TUint32 Dest(TInt aIndex);
	TUint32 Control(TInt aIndex);
	TUint32 ModifyDest(TInt aIndex, TUint32 aNewDest);
	TUint32 ModifyControl(TInt aIndex, TUint32 aClear, TUint32 aSet);

	void Dump();
public:
	TSpinLock iLock;
	TLinAddr iAddr;
	};

TIoApic TheIoApic(IO_APIC_BASE);

TIoApic::TIoApic(TLinAddr aAddr)
	: iLock(TSpinLock::EOrderBTrace)
	{
	iAddr = aAddr;
	}

TUint32 TIoApic::Id()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(IO_APIC_REG_ID);
	TUint32 x = IO_APIC_REG;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x;
	}

TUint32 TIoApic::Ver()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(IO_APIC_REG_VER);
	TUint32 x = IO_APIC_REG;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x;
	}

TUint32 TIoApic::Arb()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(IO_APIC_REG_ARB);
	TUint32 x = IO_APIC_REG;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x;
	}

TUint32 TIoApic::Dest(TInt aIndex)
	{
	TUint32 reg = 2*aIndex + 0x11;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(reg);
	TUint32 x = IO_APIC_REG;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x>>24;
	}

TUint32 TIoApic::Control(TInt aIndex)
	{
	TUint32 reg = 2*aIndex + 0x10;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(reg);
	TUint32 x = IO_APIC_REG;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x;
	}

TUint32 TIoApic::ModifyDest(TInt aIndex, TUint32 aNewDest)
	{
	TUint32 reg = 2*aIndex + 0x11;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(reg);
	TUint32 x = IO_APIC_REG;
	IO_APIC_REG = (x&0x00ffffffu) | (aNewDest<<24);
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x>>24;
	}

TUint32 TIoApic::ModifyControl(TInt aIndex, TUint32 aClear, TUint32 aSet)
	{
	TUint32 reg = 2*aIndex + 0x10;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	IO_APIC_SELECT(reg);
	TUint32 x = IO_APIC_REG;
	x &= ~aClear;
	x |= aSet;
	IO_APIC_SELECT(reg);
	IO_APIC_REG = x;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return x;
	}

void TIoApic::Dump()
	{
	TUint32 id = Id();
	TUint32 ver = Ver();
	TUint32 arb = Arb();
	__KTRACE_OPT(KBOOT,DEBUGPRINT("IOAPIC ID=%08x VER=%08x ARB=%08x", id, ver, arb));

	TInt max = (ver>>16)&0xff;
	TInt i;
	for (i=0; i<=max; ++i)
		{
		TUint32 dest = Dest(i);
		TUint32 ctrl = Control(i);
		__KTRACE_OPT(KBOOT,DEBUGPRINT("IOAPIC[%02x] DEST=%02x CTRL=%08x", i, dest, ctrl));
		}
	}


void NIrq::HwEoi()
	{
	if (iX && iX->iEoiFn)
		(*iX->iEoiFn)(this);
	else
		{
		volatile TUint32* const apic_eoi = (volatile TUint32*)(X86_LOCAL_APIC_BASE + X86_LOCAL_APIC_OFFSET_EOI);
		*apic_eoi = 0;
		}
	}

void NIrq::HwEnable()
	{
	if (iX && iX->iEnableFn)
		(*iX->iEnableFn)(this);
	else
		{
//		if ((iStaticFlags & ELevel) || (iIState & ERaw))
			TheIoApic.ModifyControl(iIndex, IO_APIC_CTRL_IMASK, 0);
		}
	}

void NIrq::HwDisable()
	{
	if (iX && iX->iDisableFn)
		(*iX->iDisableFn)(this);
	else
		{
		if ((iStaticFlags & ELevel) || (iIState & ERaw))
			TheIoApic.ModifyControl(iIndex, 0, IO_APIC_CTRL_IMASK);
		}
	}

void NIrq::HwSetCpu(TInt aCpu)
	{
	if (iX && iX->iSetCpuFn)
		(*iX->iSetCpuFn)(this, 1u<<aCpu);
	else
		{
		TheIoApic.ModifyDest(iIndex, 1u<<aCpu);
		}
	}

void NIrq::HwSetCpuMask(TUint32 aMask)
	{
	if (iX && iX->iSetCpuFn)
		(*iX->iSetCpuFn)(this, aMask);
	else
		{
		TheIoApic.ModifyDest(iIndex, aMask);
		}
	}

void NIrq::HwInit()
	{
	if (iX && iX->iInitFn)
		(*iX->iInitFn)(this);
	else
		{
		__KTRACE_OPT(KBOOT,DEBUGPRINT("NIrq %02x HwInit", iIndex));
		TUint32 clear = IO_APIC_CTRL_INTVEC_MASK;
		TUint32 set = iVector & IO_APIC_CTRL_INTVEC_MASK;
		set |= IO_APIC_CTRL_IMASK;
		if (iStaticFlags & ELevel)
			set |= (IO_APIC_CTRL_LEVEL /*| IO_APIC_CTRL_IMASK*/);
		else
			clear |= (IO_APIC_CTRL_LEVEL /*| IO_APIC_CTRL_IMASK*/);
		if (iStaticFlags & EPolarity)
			clear |= IO_APIC_CTRL_INTPOL_LOW;
		else
			set |= IO_APIC_CTRL_INTPOL_LOW;
		TheIoApic.ModifyControl(iIndex, clear, set);
		TheIoApic.Dump();
		}
	}

TBool NIrq::HwPending()
	{
	if (iX && iX->iPendingFn)
		return (*iX->iPendingFn)(this);
	return FALSE;
	}

void NIrq::HwWaitCpus()
	{
	if (iX && iX->iWaitFn)
		(*iX->iWaitFn)(this);
	}

void NIrq::HwInit0()
	{
	TheIoApic.Dump();
	TInt n = 1 + (TheIoApic.Ver() >> 16);
	TInt i;
	for (i=0; i<n; ++i)
		{
		TheIoApic.ModifyControl(i,
			IO_APIC_CTRL_DELMOD_MASK | IO_APIC_CTRL_INTVEC_MASK | IO_APIC_CTRL_LEVEL | IO_APIC_CTRL_INTPOL_LOW,
			IO_APIC_CTRL_DESTMOD | IO_APIC_CTRL_IMASK | 0xff);
		TheIoApic.ModifyDest(i, 0x01);
		if (i>15)
			{
			TheIoApic.ModifyControl(i, 0, IO_APIC_CTRL_LEVEL | IO_APIC_CTRL_INTPOL_LOW);
			}
		}
	TheIoApic.Dump();
	}

void NIrq::HwInit1()
	{
	write_apic_reg(SIVR, 0x300 | SPURIOUS_INTERRUPT_VECTOR);
	write_apic_reg(DIVCNF, 10);				// APIC timer clock divide by 128 (bus clock freq / 128)
	write_apic_reg(LVTTMR, 0x10000|TIMESLICE_VECTOR);
	write_apic_reg(DFR, 0xf0000000u);		// set flat logical destination mode
	write_apic_reg(LDR, 0x01000000u);		// this CPU will be selected by logical destination with bit 0 set
	}

void NIrq::HwInit2AP()
	{
	TInt cpu = NKern::CurrentCpu();
	write_apic_reg(SIVR, 0x300 | SPURIOUS_INTERRUPT_VECTOR);
	write_apic_reg(DIVCNF, 10);
	write_apic_reg(LVTTMR, 0x10000|TIMESLICE_VECTOR);
	write_apic_reg(DFR, 0xf0000000u);		// set flat logical destination mode
	write_apic_reg(LDR, 0x01000000u<<cpu);	// this CPU will be selected by logical destination with bit n set
	}
