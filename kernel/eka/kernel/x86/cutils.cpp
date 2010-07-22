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
// e32\kernel\x86\cutils.cpp
// 
//

//#define __DBG_MON_FAULT__
//#define __RAM_LOADED_CODE__
//#define __EARLY_DEBUG__

#include <x86_mem.h>
#include <kernel/cache.h>

#ifdef __SMP__
#include <apic.h>
TSpinLock DbgSpinLock(TSpinLock::EOrderBTrace);
#endif


TInt A::VariantHal(TInt aFunction, TAny* a1, TAny* a2)
	{
	return X86::TheAsic->VariantHal(aFunction,a1,a2);
	}

#ifdef __EARLY_DEBUG__
void DebugOutputChar(TUint aChar);
#else
static void DebugOutputChar(TUint aChar)
//
// Transmit one character from the UART (for debugging only)
//
	{
	if (X86::TheAsic)
		X86::TheAsic->DebugOutput(aChar);
	}
#endif

void A::DebugPrint(const TText* aPtr, TInt aLen, TBool aNewLine)
//
// Send the string to kernel trace port (UART)
//
	{
	const TText* pE=aPtr+aLen;
	if (aPtr!=pE)
		{
#ifdef __SMP__
		TInt irq = DbgSpinLock.LockIrqSave();
		if (aNewLine)
			{
			DebugOutputChar(NKern::CurrentCpu()+0x30);
			DebugOutputChar(0x3a);
			}
#endif
		do
			DebugOutputChar(*aPtr++);
		while(aPtr<pE);

		if (aNewLine)
			{
			DebugOutputChar(0x0d);
			DebugOutputChar(0x0a);
			}
#ifdef __SMP__
		DbgSpinLock.UnlockIrqRestore(irq);
#endif
		}
	}

EXPORT_C Asic* Arch::TheAsic()
	{
	return X86::TheAsic;
	}

TPtr8 A::MachineConfiguration()
	{
	return X86::TheAsic->MachineConfiguration();
	}

TInt A::SystemTimeInSecondsFrom2000(TInt& aTime)
	{
	return X86::TheAsic->SystemTimeInSecondsFrom2000(aTime);
	}

TInt A::SetSystemTimeInSecondsFrom2000(TInt aTime)
	{
	return X86::TheAsic->SetSystemTimeInSecondsFrom2000(aTime);
	}


extern "C" void NKCrashHandler(TInt aPhase, const TAny* a0, TInt a1)
	{
	if (aPhase==0)
		{
		Cache::CpuRetires();
		return;
		}
	Cache::KernelRetires();
#ifdef __SMP__
	SFullX86RegSet* regs = &(((SCpuData*)SubScheduler().iSSX.iTss)->iRegs);
#else
	SFullX86RegSet* regs = &X86_Regs;
#endif
	if (a1 != K::ESystemException)
		TheSuperPage().iKernelExcInfo.iCodeAddress = (TAny*)regs->iEip;
	K::DoFault(a0,a1);
	}


void A::StartCrashDebugger(const TAny* a0, TInt a1)
	{
#ifdef __SMP__
	SFullX86RegSet* regs = &(((SCpuData*)SubScheduler().iSSX.iTss)->iRegs);
#else
	SFullX86RegSet* regs = &X86_Regs;
#endif
	regs->iFaultCategory = (TAny*)a0;
	regs->iFaultReason = a1;

	typedef void (*PVFVP)(TAny*);
	PVFVP f = (PVFVP)PP::MonitorEntryPoint[0];
	if (f)
		(*f)(regs);
	FOREVER;
	}


/** 
Restart the system. 
On hardware targets this calls the Restart Vector in the ROM Header.
Note, aMode is set to zero when this function is used by Kern::Fault()

@param aMode Argument passed to the restart routine. The meaning of this value
depends on the bootstrap implementation. 		
*/
EXPORT_C void Kern::Restart(TInt aMode)
	{
	TLinAddr f = (TLinAddr)&TheRomHeader().iRestartVector;
#ifdef __SMP__
	NKern::DisableAllInterrupts();
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TheSubSchedulers[i].iSSX.iCrashState = f;
		TheSubSchedulers[i].iSSX.iExcInfo = (TAny*)aMode;
		}
	write_apic_reg(ICRH, 0);
	write_apic_reg(ICRL, 0xC4400);	// send NMI to all processors other than this one - will call vector
#ifdef __GCC32__
	asm("int 2");
#else
	_asm int 2						// fake NMI on this processor
#endif
	for(;;) {}
#else
	(*(void (*)(TInt))f)(aMode);
#endif
	}


TInt K::FloatingPointTypes(TUint32& aTypes)
	{
	aTypes = EFpTypeNone;
	return KErrNotSupported;
	}
	

extern "C" EXPORT_C void pagecpy(TAny* aTrg, const TAny* aSrc)
	{
	memcpy(aTrg, aSrc, KPageSize);
	}

extern "C" void fastcpy(TAny* aTrg, const TAny* aSrc, TUint8 aPower)
	{
	memcpy(aTrg, aSrc, 1<<aPower);
	}

#ifdef __MEMMODEL_MULTIPLE__
#include <mmubase.h>
TInt MmuBase::MoveKernelStackPage(DChunk* /*aChunk*/, TUint32 /*aOffset*/, TPhysAddr /*aOld*/, TPhysAddr& /*aNew*/, TUint /*aBlockZoneId*/, TBool /*aBlockRest*/)
	{
	NKern::UnlockSystem();
	return KErrNotSupported;
	}
#endif

TInt K::FloatingPointSystemId(TUint32& /*aSysId*/)
	{
	return KErrNotSupported;
	}
