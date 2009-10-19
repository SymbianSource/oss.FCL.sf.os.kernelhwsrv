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
// e32test\nkernsa\nirqx.cpp
// 
//

#include <nk_irq.h>

#ifdef __X86__
#include <apic.h>
#endif

class NIrqXTest : public NIrqX
	{
public:
	NIrqXTest();
	void Kick();
	void Set(TBool aLevel);
public:
	static void DoEoi(NIrq* aIrq);
	static void DoEnable(NIrq* aIrq);
	static void DoDisable(NIrq* aIrq);
	static void DoSetCpu(NIrq* aIrq, TUint32 aMask);
	static void DoInit(NIrq* aIrq);
	static TBool DoPending(NIrq* aIrq);
	static void DoWait(NIrq* aIrq);
public:
	TSpinLock			iLock;
	NIrq*				iIrq;
	TUint32				iCpuMask;
	TUint8				iEnabled;
	TUint8				iPending;
	TUint8				iLevel;
	};

NIrqXTest::NIrqXTest()
	{
	iIrq = 0;
	iCpuMask = 0x1;
	iEnabled = 0;
	iPending = 0;
	iLevel = 0;
	}

void NIrqXTest::Set(TBool aLevel)
	{
	TBool active = FALSE;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TUint32 f = iIrq->iStaticFlags;
	if (f & NIrq::ELevel)
		{
		if (f & NIrq::EPolarity)
			{
			active = aLevel;
			}
		else
			{
			active = !aLevel;
			}
		}
	else
		{
		if (f & NIrq::EPolarity)
			{
			active = aLevel && !iLevel;
			}
		else
			{
			active = !aLevel && iLevel;
			}
		}
	iLevel = (TUint8)(aLevel ? 1 : 0);
	if (active && iEnabled)
		Kick();
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	}

#if defined (__X86__)
__NAKED__ void NIrqXTest::Kick()
	{
	_asm mov al, 1
	_asm lock xchg al, [ecx]NIrqXTest.iPending
	_asm cmp al, 0
	_asm jne short kick0
	_asm mov eax, [ecx]NIrqXTest.iCpuMask
	_asm shl eax, 24
	_asm jz short kick0	// no CPUs, so nothing to do
	_asm mov ds:[X86_LOCAL_APIC_BASE + X86_LOCAL_APIC_OFFSET_ICRH], eax
	_asm mov eax, [ecx]NIrqXTest.iIrq
	_asm mov eax, [eax]NIrq.iVector
	_asm or eax, 0x4800
	_asm mov ds:[X86_LOCAL_APIC_BASE + X86_LOCAL_APIC_OFFSET_ICRL], eax
	_asm kick0:
	_asm ret
	}
#endif

void NIrqXTest::DoEoi(NIrq* aIrq)
	{
	NIrqXTest* pX = (NIrqXTest*)iX;
	TInt irq = __SPIN_LOCK_IRQSAVE(pX->iLock);
	if (pX->iPending)
		{
		pX->iPending = 0;
		TUint32 f = aIrq->iStaticFlags;
		if (f & NIrq::ELevel)
			{
			TUint active_level = (f & NIrq::EPolarity) ? 1 : 0;
			if (pX->iLevel==active_level && pX->iEnabled)
				pX->Kick();
			}
		}
	__SPIN_UNLOCK_IRQRESTORE(pX->iLock,irq);
	}

void NIrqXTest::DoEnable(NIrq* aIrq)
	{
	}

void NIrqXTest::DoDisable(NIrq* aIrq)
	{
	}

void NIrqXTest::DoSetCpu(NIrq* aIrq, TUint32 aMask)
	{
	}

void NIrqXTest::DoInit(NIrq* aIrq)
	{
	iIrq = aIrq;
	}

TBool NIrqXTest::DoPending(NIrq* aIrq)
	{
	}

void NIrqXTest::DoWait(NIrq* aIrq)
	{
	}

