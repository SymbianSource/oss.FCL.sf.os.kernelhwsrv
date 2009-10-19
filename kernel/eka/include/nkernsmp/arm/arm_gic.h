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
// e32\include\nkernsmp\arm\arm_gic.h
// Register definitions for ARM Generic Interrupt Controller
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef	__ARM_GIC_H__
#define	__ARM_GIC_H__
#include <e32def.h>

#ifdef	__STANDALONE_NANOKERNEL__
#undef	__IN_KERNEL__
#define	__IN_KERNEL__
#endif

enum TGicIntId
	{
	E_GicIntId_Soft0		=0,				// IDs 0-15 are for software triggered IPIs
	E_GicIntId_Soft1		=1,
	E_GicIntId_Soft2		=2,
	E_GicIntId_Soft3		=3,
	E_GicIntId_Soft4		=4,
	E_GicIntId_Soft5		=5,
	E_GicIntId_Soft6		=6,
	E_GicIntId_Soft7		=7,
	E_GicIntId_Soft8		=8,
	E_GicIntId_Soft9		=9,
	E_GicIntId_Soft10		=10,
	E_GicIntId_Soft11		=11,
	E_GicIntId_Soft12		=12,
	E_GicIntId_Soft13		=13,
	E_GicIntId_Soft14		=14,
	E_GicIntId_Soft15		=15,

	E_GicIntId_Private0		=16,			// IDs 16-31 are for private peripherals
	E_GicIntId_Private1		=17,
	E_GicIntId_Private2		=18,
	E_GicIntId_Private3		=19,
	E_GicIntId_Private4		=20,
	E_GicIntId_Private5		=21,
	E_GicIntId_Private6		=22,
	E_GicIntId_Private7		=23,
	E_GicIntId_Private8		=24,
	E_GicIntId_Private9		=25,
	E_GicIntId_Private10	=26,
	E_GicIntId_Private11	=27,
	E_GicIntId_Private12	=28,
	E_GicIntId_Private13	=29,
	E_GicIntId_Private14	=30,
	E_GicIntId_Private15	=31,

	E_GicIntId_Normal0		=32,			// first normal interrupt ID

	E_GicIntId_NormalLast	=1019,			// last possible normal interrupt ID
	E_GicIntId_Reserved0	=1020,			// reserved interrupt ID
	E_GicIntId_Reserved1	=1021,			// reserved interrupt ID
	E_GicIntId_NS			=1022,			// only nonsecure interrupts are serviceable
	E_GicIntId_Spurious		=1023			// no interrupts are serviceable
	};

struct GicDistributor
	{
	volatile TUint32	iCtrl;				// 000 Control register
	volatile TUint32	iType;				// 004 Type register
	volatile TUint32	iImpId;				// 008 Implementor Identification register (not on MPCore)
	volatile TUint32	i_Skip_1[29];		// 00C unused
	volatile TUint32	iIntSec[32];		// 080 Interrupt Security register (not on MPCore) (1 bit per interrupt)
	volatile TUint32	iEnableSet[32];		// 100 Enable set register (1 bit per interrupt)
	volatile TUint32	iEnableClear[32];	// 180 Enable clear register (1 bit per interrupt)
	volatile TUint32	iPendingSet[32];	// 200 Pending set register (1 bit per interrupt)
	volatile TUint32	iPendingClear[32];	// 280 Pending clear register (1 bit per interrupt)
	volatile TUint32	iActive[32];		// 300 Active status register (1 bit per interrupt)
	volatile TUint32	i_Skip_2[32];		// 380 unused
	volatile TUint32	iPriority[256];		// 400 Interrupt priority register (8 bits per interrupt)
	volatile TUint32	iTarget[256];		// 800 Interrupt target CPUs register (8 bits per interrupt)
	volatile TUint32	iConfig[64];		// C00 Interrupt configuration register (2 bits per interrupt)
	volatile TUint32	iImpDef[64];		// D00 Implementation defined registers
											// = Interrupt line level on MPCore
	volatile TUint32	i_Skip_3[64];		// E00 unused
	volatile TUint32	iSoftIrq;			// F00 Software triggered interrupt register
	volatile TUint32	i_Skip_4[51];		// F04 unused
	volatile TUint32	iIdent[12];			// FD0 Identification registers
	};

__ASSERT_COMPILE(sizeof(GicDistributor)==0x1000);

struct GicCpuIfc
	{
	volatile TUint32	iCtrl;				// 00 Control register
	volatile TUint32	iPriMask;			// 04 Priority mask register
	volatile TUint32	iBinaryPoint;		// 08 Binary point register
	volatile TUint32	iAck;				// 0C Interrupt acknowledge register
	volatile TUint32	iEoi;				// 10 End of interrupt register
	volatile TUint32	iRunningPri;		// 14 Running priority register
	volatile TUint32	iHighestPending;	// 18 Highest pending interrupt register
	volatile TUint32	iNSBinaryPoint;		// 1C Aliased nonsecure binary point register (not on MPCore)
	volatile TUint32	i_Skip_1[8];		// 20 unused
	volatile TUint32	iImpDef[36];		// 40 Implementation defined (not present on MPCore)
	volatile TUint32	i_Skip_2[11];		// D0 unused
	volatile TUint32	iImpId;				// FC Implementor Identification register (not on MPCore)
	};

__ASSERT_COMPILE(sizeof(GicCpuIfc)==0x100);

enum TGicDistCtrl
	{
	E_GicDistCtrl_Enable	=1,				// Enable interrupt distributor
	};

enum TGicDistType
	{
	E_GicDistType_ITShift	=0u,			// bits 0-4 = number of sets of 32 interrupts supported
	E_GicDistType_ITMask	=0x1fu,
	E_GicDistType_CPUNShift	=5u,			// bits 5-7 = number of CPUs supported - 1
	E_GicDistType_CPUNMask	=0xe0u,
	E_GicDistType_Domains	=0x400u,		// set if two security domains supported
	E_GicDistType_LSPIShift	=11u,			// bits 11-15 = number of lockable shared peripheral interrupts
	E_GicDistType_LSPIMask	=0xf800u,
	};

enum TGicDistIntConfig
	{
	E_GicDistICfg1N			=1u,			// if set use 1-N model else use N-N model
											// peripheral interrupts support only 1-N model, s/w interrupts N-N
											// 1-N means the interrupt is cleared by the first CPU to accept it
	E_GicDistICfgEdge		=2u,			// if set, rising edge triggered, else active high level triggered
	};

enum TGicDistSoftIrqDestType
	{
	E_GicDestTypeList		=0u,			// send to all CPUs in list (bit mask)
	E_GicDestTypeOthers		=1u,			// send to all CPUs other than self
	E_GicDestTypeSelf		=2u,			// send to self only
	E_GicDestTypeRsvd		=3u
	};

// Compile word to generate IPI
// dt = destination type, dl = bit mask of destination CPUs, id = interrupt ID (0-15)
#define	GIC_SOFT_IRQ_WORD(dt,dl,id)		((TUint32(dt)<<24)|(TUint32(dl)<<16)|(TUint32(id)))

#define	GIC_IPI_SELF(id)				GIC_SOFT_IRQ_WORD(E_GicDestTypeSelf, 0, id)
#define	GIC_IPI_OTHERS(id)				GIC_SOFT_IRQ_WORD(E_GicDestTypeOthers, 0, id)
#define	GIC_IPI(dl,id)					GIC_SOFT_IRQ_WORD(E_GicDestTypeList, dl, id)


#endif	// 	__ARM_GIC_H__
