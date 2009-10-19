// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\arm\ncirq.cpp
//
//

/**
 @file
 @internalTechnology
*/

#include "nk_priv.h"
#include "nk_plat.h"
#include <nk_irq.h>
#include <arm.h>
#include <arm_gic.h>
#include <arm_scu.h>
#include <arm_tmr.h>

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

/******************************************************************************
 * ARM Generic Interrupt Controller
 ******************************************************************************/

class ArmGic
	{
public:
	static void Enable(TInt aIndex);
	static void Disable(TInt aIndex);
	static TBool IsEnabled(TInt aIndex);
	static void SetPending(TInt aIndex);
	static void ClearPending(TInt aIndex);
	static TBool IsPending(TInt aIndex);
	static TBool IsActive(TInt aIndex);
	static TBool SetNonSecure(TInt aIndex, TBool aNonSecure);
	static TUint32 Priority(TInt aIndex);
	static TUint32 SetPriority(TInt aIndex, TUint32 aPri);
	static TUint32 Dest(TInt aIndex);
	static TUint32 ModifyDest(TInt aIndex, TUint32 aClear, TUint32 aSet);
	static TUint32 Config(TInt aIndex);
	static TUint32 ModifyConfig(TInt aIndex, TUint32 aClear, TUint32 aSet);

	static void Dump();
	static void DumpCpuIfc();
public:
	static TSpinLock	ArmGicLock;
	static TInt			LSPI;
	static TInt			Domains;
	static TInt			NumCpus;
	static TInt			NumLines;
	static TUint32		PriMask;
	static TUint32		PriSpc;
	static TUint32		MinPri;
	};

TSpinLock ArmGic::ArmGicLock(TSpinLock::EOrderBTrace);
TInt ArmGic::LSPI;
TInt ArmGic::Domains;
TInt ArmGic::NumCpus;
TInt ArmGic::NumLines;
TUint32 ArmGic::PriMask;
TUint32 ArmGic::PriSpc;
TUint32	ArmGic::MinPri;

void ArmGic::Enable(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	GIC_DIST.iEnableSet[aIndex>>5] = mask;
	arm_dsb();
	}

void ArmGic::Disable(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	GIC_DIST.iEnableClear[aIndex>>5] = mask;
	arm_dsb();
	}

TBool ArmGic::IsEnabled(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	return GIC_DIST.iEnableSet[aIndex>>5] & mask;
	}

void ArmGic::SetPending(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	GIC_DIST.iPendingSet[aIndex>>5] = mask;
	arm_dsb();
	}

void ArmGic::ClearPending(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	GIC_DIST.iPendingClear[aIndex>>5] = mask;
	arm_dsb();
	}

TBool ArmGic::IsPending(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	return GIC_DIST.iPendingSet[aIndex>>5] & mask;
	}

TBool ArmGic::IsActive(TInt aIndex)
	{
	TUint32 mask = 1u << (aIndex&31);
	return GIC_DIST.iActive[aIndex>>5] & mask;
	}

TUint32 ArmGic::Dest(TInt aIndex)
	{
	TUint32 reg = GIC_DIST.iTarget[aIndex>>2];
	reg >>= ((aIndex&3)<<3);
	reg &= 0xff;
	return reg;
	}

TUint32 ArmGic::ModifyDest(TInt aIndex, TUint32 aClear, TUint32 aSet)
	{
	aClear &= 0xff;
	aSet &= 0xff;
	TInt shift = (aIndex&3)<<3;
	aClear <<= shift;
	aSet <<= shift;
	volatile TUint32& reg = GIC_DIST.iTarget[aIndex>>2];
	TInt irq = __SPIN_LOCK_IRQSAVE(ArmGicLock);
	TUint32 old = reg;
	reg = (old &~ aClear) | aSet;
	arm_dsb();
	__SPIN_UNLOCK_IRQRESTORE(ArmGicLock, irq);
	old >>= shift;
	return old & 0xff;
	}

TUint32 ArmGic::Config(TInt aIndex)
	{
	TInt shift = (aIndex&15)<<1;
	TUint32 x = GIC_DIST.iConfig[aIndex>>4];
	x >>= shift;
	return x & 3;
	}

TUint32 ArmGic::ModifyConfig(TInt aIndex, TUint32 aClear, TUint32 aSet)
	{
	aClear &= 3;
	aSet &= 3;
	TInt shift = (aIndex&15)<<1;
	aClear <<= shift;
	aSet <<= shift;
	volatile TUint32& reg = GIC_DIST.iConfig[aIndex>>4];
	TInt irq = __SPIN_LOCK_IRQSAVE(ArmGicLock);
	TUint32 old = reg;
	reg = (old &~ aClear) | aSet;
	arm_dsb();
	__SPIN_UNLOCK_IRQRESTORE(ArmGicLock, irq);
	old >>= shift;
	return old & 3;
	}

TBool ArmGic::SetNonSecure(TInt aIndex, TBool aNonSecure)
	{
	TUint32 mask = 1u << (aIndex & 31);
	volatile TUint32& reg = GIC_DIST.iIntSec[aIndex>>5];
	TInt irq = __SPIN_LOCK_IRQSAVE(ArmGicLock);
	TUint32 old = reg;
	reg = aNonSecure ? (old | mask) : (old &~ mask);
	arm_dsb();
	__SPIN_UNLOCK_IRQRESTORE(ArmGicLock, irq);
	return old & mask;
	}

TUint32 ArmGic::Priority(TInt aIndex)
	{
	TInt shift = (aIndex&3)<<3;
	TUint32 x = GIC_DIST.iPriority[aIndex>>2];
	x >>= shift;
	return x & 0xff;
	}

TUint32 ArmGic::SetPriority(TInt aIndex, TUint32 aPri)
	{
	aPri &= 0xff;
	TInt shift = (aIndex&3)<<3;
	TUint32 clear = 0xffu << shift;
	aPri <<= shift;
	volatile TUint32& reg = GIC_DIST.iPriority[aIndex>>2];
	TInt irq = __SPIN_LOCK_IRQSAVE(ArmGicLock);
	TUint32 old = reg;
	reg = (old &~ clear) | aPri;
	arm_dsb();
	__SPIN_UNLOCK_IRQRESTORE(ArmGicLock, irq);
	old >>= shift;
	return old & 0xff;
	}

void ArmGic::Dump()
	{
#ifdef KBOOT
	__KTRACE_OPT(KBOOT,DEBUGPRINT("GIC iCtrl=%08x iType=%08x", GIC_DIST.iCtrl, GIC_DIST.iType));
	TInt n = ArmGic::NumLines;
	TInt i;
	for (i=0; i<n; i++)
		{
		TUint32 cfg = Config(i);
		TUint32 pri = Priority(i);
		TUint32 dest = Dest(i);
		TUint32 enabled = IsEnabled(i) ? 1 : 0;
		TUint32 pending = IsPending(i) ? 1 : 0;
		TUint32 active = IsActive(i) ? 1 : 0;
		const char* cat = (i<16) ? "SW" : (i<32) ? "PP" : "SP";
		__KTRACE_OPT(KBOOT,DEBUGPRINT("%3d: %2s cfg=%1d pri=%02x dest=%02x E%1d P%1d A%1d",
			i, cat, cfg, pri, dest, enabled, pending, active));
		}
#endif
	}

void ArmGic::DumpCpuIfc()
	{
#ifdef KBOOT
	GicCpuIfc& C = GIC_CPU_IFC;
	__KTRACE_OPT(KBOOT,DEBUGPRINT("IFC iCtrl=%08x iPriMask=%08x iBinaryPoint=%08x", C.iCtrl, C.iPriMask, C.iBinaryPoint));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("IFC Running=%08x HighestP=%08x", C.iRunningPri, C.iHighestPending));
#endif
	}


void NIrq::HwEoi()
	{
	if (iX && iX->iEoiFn)
		(*iX->iEoiFn)(this);
	else
		{
		GIC_CPU_IFC.iEoi = iVector;

#if defined(SMP_CRAZY_INTERRUPTS) && !defined(__STANDALONE_NANOKERNEL__)
		// change the target CPU for the next Interrupt
		if ((TInt)TheSuperPage().KernelConfigFlags() & EKernelConfigSMPCrazyInterrupts)
			{
			TInt cpu = NKern::CurrentCpu() + 1;
			if(cpu >= NKern::NumberOfCpus())
				cpu = 0;
			ArmGic::ModifyDest(iVector, 0xffu, 1u << cpu);
			}
		else
			arm_dsb();

#else
		arm_dsb();
#endif
		}
	}

void NIrq::HwEnable()
	{
	if (iX && iX->iEnableFn)
		(*iX->iEnableFn)(this);
	else
		{
		ArmGic::Enable(iVector);
		}
	}

void NIrq::HwDisable()
	{
	if (iX && iX->iDisableFn)
		(*iX->iDisableFn)(this);
	else
		{
		ArmGic::Disable(iVector);
		}
	}

void NIrq::HwSetCpu(TInt aCpu)
	{
	if (iX && iX->iSetCpuFn)
		(*iX->iSetCpuFn)(this, 1u<<aCpu);
	else
		{
		ArmGic::ModifyDest(iVector, 0xffu, 1u<<aCpu);
		}
	}

void NIrq::HwSetCpuMask(TUint32 aMask)
	{
	if (iX && iX->iSetCpuFn)
		(*iX->iSetCpuFn)(this, aMask);
	else
		{
		ArmGic::ModifyDest(iVector, 0xffu, aMask);
		}
	}

void NIrq::HwInit()
	{
	if (iX && iX->iInitFn)
		(*iX->iInitFn)(this);
	else
		{
		__KTRACE_OPT(KBOOT,DEBUGPRINT("NIrq %02x HwInit", iIndex));
		TUint32 clear = E_GicDistICfgEdge;
		TUint32 set = 0;
		if (!(iStaticFlags & ELevel))
			set = E_GicDistICfgEdge;
		ArmGic::ModifyConfig(iVector, clear, set);
		}
	}

TBool NIrq::HwPending()
	{
	if (iX && iX->iPendingFn)
		return (*iX->iPendingFn)(this);
	return ArmGic::IsPending(iVector) || ArmGic::IsActive(iVector);
	}

void NIrq::HwWaitCpus()
	{
	if (iX && iX->iWaitFn)
		(*iX->iWaitFn)(this);
	}

void NIrq::HwInit0()
	{
	__KTRACE_OPT(KBOOT, DEBUGPRINT("NIrq::HwInit0"));

	// Need to set up addresses of GIC_DIST, GIC_CPU_IFC, SCU and LOCAL_TIMER

	GicDistributor& D = GIC_DIST;
	GicCpuIfc& C = GIC_CPU_IFC;
	D.iCtrl = 0;
	C.iCtrl = 0;
	arm_dsb();
	TUint32 type = D.iType;
	__KTRACE_OPT(KBOOT, DEBUGPRINT("GIC iType = %08x", type));
	ArmGic::LSPI = (type & E_GicDistType_LSPIMask) >> E_GicDistType_LSPIShift;
	ArmGic::Domains = (type & E_GicDistType_Domains) ? 2 : 1;
	ArmGic::NumCpus = ((type & E_GicDistType_CPUNMask) >> E_GicDistType_CPUNShift) + 1;
	ArmGic::NumLines = ((type & E_GicDistType_ITMask) + 1) << 5;
	__KTRACE_OPT(KBOOT, DEBUGPRINT("GIC LSPI=%d Domains=%d NumCpus=%d NumLines=%d",
						ArmGic::LSPI, ArmGic::Domains, ArmGic::NumCpus, ArmGic::NumLines));
	TInt i;
	for (i=0; i<32; ++i)
		D.iEnableClear[i] = 0xffffffffu;	// disable all interrupts
	arm_dsb();
	for (i=0; i<32; ++i)
		D.iPendingClear[i] = 0xffffffffu;	// clear any pending interrupts
	arm_dsb();
	D.iPriority[0] = 0xffffffffu;
	ArmGic::PriMask = D.iPriority[0] & 0xffu;
	ArmGic::PriSpc = (~ArmGic::PriMask + 1) & 0xffu;
	ArmGic::MinPri = ArmGic::PriMask - ArmGic::PriSpc;
	__KTRACE_OPT(KBOOT, DEBUGPRINT("PriMask=%02x PriSpc=%02x MinPri=%02x", ArmGic::PriMask, ArmGic::PriSpc, ArmGic::MinPri));
	TUint32 x = ArmGic::MinPri;
	x |= (x<<8);
	x |= (x<<16);
	for (i=0; i<256; ++i)
		D.iPriority[i] = x;		// set all interrupts to minimum active priority
	x = 0x01010101u;
	for (i=0; i<256; ++i)
		D.iTarget[i] = x;		// set all interrupts to target this CPU
	x = 0xAAAAAAAAu;			// config value for SW interrupts (rising edge, N-N)
	D.iConfig[0] = x;			// set config for 0-15
	x = 0x28000000u;			// 31=0b00, 30=29=0b10
	D.iConfig[1] = x;			// set config for 16-31
	x = 0xAAAAAAAAu;			// config value for SW interrupts (rising edge, N-N)
	for (i=2; i<64; ++i)
		D.iConfig[i] = x;		// set default value for other interrupts
	arm_dsb();
	ArmGic::Dump();
	}

void NIrq::HwInit1()
	{
	__KTRACE_OPT(KBOOT, DEBUGPRINT("NIrq::HwInit1"));

	// elevate priority of CRASH_IPI to highest level
	ArmGic::SetPriority(CRASH_IPI_VECTOR, 0);

	GicDistributor& D = GIC_DIST;
	GicCpuIfc& C = GIC_CPU_IFC;
	C.iCtrl = 0;
	C.iPriMask = ArmGic::PriMask;	// unmask all interrupts
	C.iBinaryPoint = 0;
	arm_dsb();
	C.iCtrl = E_GicDistCtrl_Enable;	// enable this CPU's interrupt controller interface
	arm_dsb();
	D.iCtrl = E_GicDistCtrl_Enable;	// enable the global interrupt distributor
	arm_dsb();

	// Enable timeslice timer interrupt
	ArmLocalTimer& T = LOCAL_TIMER;
	T.iTimerCtrl = 0;
	T.iTimerIntStatus = E_ArmTmrIntStatus_Event;
	ArmGic::ClearPending(TIMESLICE_VECTOR);
	arm_dsb();
	ArmGic::Enable(TIMESLICE_VECTOR);
	arm_dsb();
	T.iTimerLoad = KMaxTUint32;				// timer wraps to 0xffffffff after reaching 0
	arm_dsb();
	T.iTimerCount = (TUint32)KMaxTInt32;	// timer starts at 0x7fffffff (initial thread doesn't timeslice)
	arm_dsb();

	ArmGic::DumpCpuIfc();
	}

void NIrq::HwInit2AP()
	{
	__KTRACE_OPT(KBOOT, DEBUGPRINT("NIrq::HwInit2AP"));

	// Must set up interrupts 0-31 separately for each CPU
	GicDistributor& D = GIC_DIST;
	TInt i;
	TUint32 x = ArmGic::MinPri;
	x |= (x<<8);
	x |= (x<<16);
	for (i=0; i<32; ++i)
		D.iPriority[i] = x;		// set all interrupts to minimum active priority
	x = 0xAAAAAAAAu;			// config value for SW interrupts (rising edge, N-N)
	D.iConfig[0] = x;			// set config for 0-15
	x = 0x28000000u;			// 31=0b00, 30=29=0b10
	D.iConfig[1] = x;			// set config for 16-31
	arm_dsb();
	ArmGic::Dump();

	// elevate priority of CRASH_IPI to highest level
	ArmGic::SetPriority(CRASH_IPI_VECTOR, 0);

	GicCpuIfc& C = GIC_CPU_IFC;
	C.iCtrl = 0;
	C.iPriMask = ArmGic::PriMask;	// unmask all interrupts
	C.iBinaryPoint = 0;
	arm_dsb();
	C.iCtrl = E_GicDistCtrl_Enable;
	arm_dsb();

	// Enable timeslice timer interrupt
	ArmLocalTimer& T = LOCAL_TIMER;
	T.iTimerCtrl = 0;
	T.iTimerIntStatus = E_ArmTmrIntStatus_Event;
	ArmGic::ClearPending(TIMESLICE_VECTOR);
	arm_dsb();
	ArmGic::Enable(TIMESLICE_VECTOR);
	arm_dsb();
	T.iTimerLoad = KMaxTUint32;				// timer wraps to 0xffffffff after reaching 0
	arm_dsb();
	T.iTimerCount = (TUint32)KMaxTInt32;	// timer starts at 0x7fffffff (initial thread doesn't timeslice)
	arm_dsb();

	ArmGic::DumpCpuIfc();
	}

