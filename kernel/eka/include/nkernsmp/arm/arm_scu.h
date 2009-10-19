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
// e32\include\nkernsmp\arm\arm_scu.h
// Register definitions for ARM Snoop Control Unit
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef	__ARM_SCU_H__
#define	__ARM_SCU_H__
#include <e32def.h>

#ifdef	__STANDALONE_NANOKERNEL__
#undef	__IN_KERNEL__
#define	__IN_KERNEL__
#endif

#if defined(__CPU_ARM11MP__)
struct ArmScu
	{
	volatile TUint32	iCtrl;				// 00 Control register
	volatile TUint32	iConfig;			// 04 Configuration register (RO)
	volatile TUint32	iCpuStatus;			// 08 SCU CPU Status register
	volatile TUint32	iInvalidateAll;		// 0C Invalidate All register (WO)
	volatile TUint32	iPMCtrl;			// 10 Performance Monitor Control register
	volatile TUint32	iMonitorEvents0;	// 14 Monitor Counter Events 0
	volatile TUint32	iMonitorEvents1;	// 18 Monitor Counter Events 1
	volatile TUint32	iMonitorCount0;		// 1C Monitor Counter 0
	volatile TUint32	iMonitorCount1;		// 20 Monitor Counter 1
	volatile TUint32	iMonitorCount2;		// 24 Monitor Counter 2
	volatile TUint32	iMonitorCount3;		// 28 Monitor Counter 3
	volatile TUint32	iMonitorCount4;		// 2C Monitor Counter 4
	volatile TUint32	iMonitorCount5;		// 30 Monitor Counter 5
	volatile TUint32	iMonitorCount6;		// 34 Monitor Counter 6
	volatile TUint32	iMonitorCount7;		// 38 Monitor Counter 7
	volatile TUint32	i_Skip_1[49];		// 3C unused
	};

__ASSERT_COMPILE(sizeof(ArmScu)==0x100);

enum TArmScuCtrl
	{
	E_ArmScuCtrl_Enable			=1u,			// SCU Enable
	E_ArmScuCtrl_AccessShift	=1u,
	E_ArmScuCtrl_AccessMask		=0x1eu,			// bits 1-4 = SCU access control for CPU0-3
	E_ArmScuCtrl_IIAliasShift	=5u,
	E_ArmScuCtrl_IIAliasMask	=0x1e0u,		// bits 5-8 = Interrupt Interface Alias enable for CPU0-3
	E_ArmScuCtrl_PIAliasShift	=9u,
	E_ArmScuCtrl_PIAliasMask	=0x1e00u,		// bits 9-12 = Peripheral Interface Alias enable for CPU0-3
	};

enum TArmScuPMCR
	{
	E_ArmScuPMCR_Enable			=1u,			// 0=all counters disabled
	E_ArmScuPMCR_ResetAll		=2u,			// write 1 resets all counters
	E_ArmScuPMCR_IntEn0			=0x100u,		// Interrupt Enable for MN0
	E_ArmScuPMCR_IntEn1			=0x200u,		// Interrupt Enable for MN1
	E_ArmScuPMCR_IntEn2			=0x400u,		// Interrupt Enable for MN2
	E_ArmScuPMCR_IntEn3			=0x800u,		// Interrupt Enable for MN3
	E_ArmScuPMCR_IntEn4			=0x1000u,		// Interrupt Enable for MN4
	E_ArmScuPMCR_IntEn5			=0x2000u,		// Interrupt Enable for MN5
	E_ArmScuPMCR_IntEn6			=0x4000u,		// Interrupt Enable for MN6
	E_ArmScuPMCR_IntEn7			=0x8000u,		// Interrupt Enable for MN7
	E_ArmScuPMCR_Ovfw0			=0x10000u,		// Overflow Flag for MN0 (write 1 to clear)
	E_ArmScuPMCR_Ovfw1			=0x20000u,		// Overflow Flag for MN1
	E_ArmScuPMCR_Ovfw2			=0x40000u,		// Overflow Flag for MN2
	E_ArmScuPMCR_Ovfw3			=0x80000u,		// Overflow Flag for MN3
	E_ArmScuPMCR_Ovfw4			=0x100000u,		// Overflow Flag for MN4
	E_ArmScuPMCR_Ovfw5			=0x200000u,		// Overflow Flag for MN5
	E_ArmScuPMCR_Ovfw6			=0x400000u,		// Overflow Flag for MN6
	E_ArmScuPMCR_Ovfw7			=0x800000u,		// Overflow Flag for MN7
	};


#elif defined(__CPU_CORTEX_A9__)
struct ArmScu
	{
	volatile TUint32	iCtrl;				// 00 Control register
	volatile TUint32	iConfig;			// 04 Configuration register (RO)
	volatile TUint32	iCpuStatus;			// 08 SCU CPU Power Status register
	volatile TUint32	iInvalidateAll;		// 0C Invalidate All register (WO)
	volatile TUint32	i_Skip_1[12];		// 10-3F unused
	volatile TUint32	i_FSAR;				// 40 Filtering Start Address Register
	volatile TUint32	i_FEAR;				// 44 Filtering End Address Register
	volatile TUint32	i_Skip_2[2];		// 48-4F unused
	volatile TUint32	i_SAC;				// 50 SCU Access Control Register
	volatile TUint32	i_SSAC;				// 54 SCU Secure Access Control Register
	volatile TUint32	i_Skip_3[42];		// 58-FF unused
	};

__ASSERT_COMPILE(sizeof(ArmScu)==0x100);

enum TArmScuCtrl
	{
	E_ArmScuCtrl_Enable			=1u,			// SCU Enable
	E_ArmScuCtrl_AFEnable		=2u,			// SCU Address Filtering Enable
	E_ArmScuCtrl_ParityEnable	=4u,			// SCU Parity Enable
	};

enum TArmScuSAC
	{
	E_ArmScuSAC_CPU0			=1u,			// If set, CPU0 can access SCU registers
	E_ArmScuSAC_CPU1			=2u,			// If set, CPU1 can access SCU registers
	E_ArmScuSAC_CPU2			=4u,			// If set, CPU2 can access SCU registers
	E_ArmScuSAC_CPU3			=8u,			// If set, CPU3 can access SCU registers
	};

enum TArmScuSSAC
	{
	E_ArmScuSSAC_CPU0			=1u,			// If set, CPU0 can access SCU registers in nonsecure state
	E_ArmScuSSAC_CPU1			=2u,			// If set, CPU1 can access SCU registers in nonsecure state
	E_ArmScuSSAC_CPU2			=4u,			// If set, CPU2 can access SCU registers in nonsecure state
	E_ArmScuSSAC_CPU3			=8u,			// If set, CPU3 can access SCU registers in nonsecure state
	E_ArmScuSSAC_Timer0			=16u,			// If set, CPU0 private timer is accessible in nonsecure state
	E_ArmScuSSAC_Timer1			=32u,			// If set, CPU1 private timer is accessible in nonsecure state
	E_ArmScuSSAC_Timer2			=64u,			// If set, CPU2 private timer is accessible in nonsecure state
	E_ArmScuSSAC_Timer3			=128u,			// If set, CPU3 private timer is accessible in nonsecure state
	};

#else
#error	Unknown SCU
#endif

enum TArmScuConfig
	{
	E_ArmScuCfg_NCpusMask		=3u,			// bits0,1 = number of CPUs - 1
	E_ArmScuCfg_CpuSMPShift		=4u,
	E_ArmScuCfg_CpuSMPMask		=0xf0u,			// bits4-7 = CPU0-3 SMP mode indicator
	E_ArmScuCfg_TagShift		=8u,
	E_ArmScuCfg_TagMask			=0xff00u,		// two bits per CPU, tag RAM size = 16KB<<n (n=0,1,2 n=3 reserved)
	};

// Bits 2n,2n+1 of CPU status refer to CPU n
enum TArmScuCPUStatus
	{
	E_ArmScuCpuStat_Normal		=0u,			// normal mode
												// 1 reserved
	E_ArmScuCpuStat_Dormant		=2u,			// dormant mode
	E_ArmScuCpuStat_PowerDown	=3u,			// power down mode
	};

#endif	// 	__ARM_SCU_H__
