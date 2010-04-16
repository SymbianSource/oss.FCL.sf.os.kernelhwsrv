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
// e32\include\nkernsmp\arm\arm_tmr.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef	__ARM_TMR_H__
#define	__ARM_TMR_H__
#include <e32def.h>

#ifdef	__STANDALONE_NANOKERNEL__
#undef	__IN_KERNEL__
#define	__IN_KERNEL__
#endif

#if !defined(__CPU_ARM11MP__) && !defined(__CPU_CORTEX_A9__)
#error	Unknown local timer
#endif

// Local timer looks the same on ARM11MP and Cortex A9
struct ArmLocalTimer
	{
	volatile TUint32	iTimerLoad;				// 00 Timer reload value (write also writes counter)
	volatile TUint32	iTimerCount;			// 04 Timer instantaneous count value
	volatile TUint32	iTimerCtrl;				// 08 Timer control register
	volatile TUint32	iTimerIntStatus;		// 0C Timer interrupt status register
	volatile TUint32	i_Spare1[4];			// 10 unused
	volatile TUint32	iWatchdogLoad;			// 20 Watchdog reload value (write also writes counter)
	volatile TUint32	iWatchdogCount;			// 24 Watchdog instantaneous count value
	volatile TUint32	iWatchdogCtrl;			// 28 Watchdog control register
	volatile TUint32	iWatchdogIntStatus;		// 2C Watchdog interrupt status register
	volatile TUint32	iWatchdogResetSent;		// 30 Watchdog reset sent register
	volatile TUint32	iWatchdogDisable;		// 34 Watchdog disable register
	volatile TUint32	i_Spare2[50];			// 38 unused
	};

__ASSERT_COMPILE(sizeof(ArmLocalTimer)==0x100);

// These bits apply to both timer and watchdog control registers
enum TArmTimerCtrl
	{
	E_ArmTmrCtrl_Enable			=1u,		// when set, timer counts down
	E_ArmTmrCtrl_Reload			=2u,		// when set, timer reloads on reaching zero
	E_ArmTmrCtrl_IntEn			=4u,		// when set enables timer interrupt
	E_ArmTmrCtrl_WD				=8u,		// set when in watchdog mode (watchdog only, can write to 1 but not 0)
	E_ArmTmrCtrl_PrescaleShift	=8u,
	E_ArmTmrCtrl_PrescaleMask	=0xff00u,	// bits 8-15 = prescale value - divides by (P+1)
											// input to prescaler is PERIPHCLK (=CPUCLK/2 on NE1, CPUCLK/N in general, N>=2)
	E_ArmTmrCtrl_Prescale64		=0x3f00u,	// value to prescale by 64 (matches cycle counter prescaler)
	};

enum TArmTimerIntStatus
	{
	E_ArmTmrIntStatus_Event		=1u			// set when timer counter reaches zero, write 1 to clear
	};

enum TArmTimerWRS
	{
	E_ArmTmrWRS_ResetSent		=1u			// set if the watchdog caused a reset, write 1 to clear
	};

enum TArmTimerWDDisable
	{
	E_ArmTmrWDD_1				=0x12345678u,	// to disable watchdog, write this ...
	E_ArmTmrWDD_2				=0x87654321u,	// ... then this with no intervening writes
	};


#ifdef	__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK

// r1p0 and later A9s have an additional Global Timer
struct ArmGlobalTimer
	{
	volatile TUint32	iTimerCountLow;			// 00 Timer counter low word
	volatile TUint32	iTimerCountHigh;		// 04 Timer counter high word
	volatile TUint32	iTimerCtrl;				// 08 Timer control register
	volatile TUint32	iTimerStatus;			// 0C Timer status register
	volatile TUint32	iComparatorLow;			// 10 Comparator value low word (per-CPU register)
	volatile TUint32	iComparatorHigh;		// 14 Comparator value high word (per-CPU register)
	volatile TUint32	iComparatorInc;			// 18 Comparator autoincrement value (per-CPU register)
	volatile TUint32	i_Spare2[57];			// 1C unused
	};

__ASSERT_COMPILE(sizeof(ArmGlobalTimer)==0x100);

// Global Timer Control Register Bits
enum TArmGlobalTimerCtrl
	{
	E_ArmGTmrCtrl_TmrEnb		=1u,			// when set, timer counts up
	E_ArmGTmrCtrl_CmpEnb		=2u,			// when set, comparator matching is enabled (per-CPU)
	E_ArmGTmrCtrl_IntEn			=4u,			// when set enables comparator match interrupt (per-CPU)
	E_ArmGTmrCtrl_AutoInc		=8u,			// when set enables comparator auto increment (per-CPU)
	E_ArmGTmrCtrl_PrescaleShift	=8u,
	E_ArmGTmrCtrl_PrescaleMask	=0xff00u,		// bits 8-15 = prescale value - divides by (P+1)
												// input to prescaler is PERIPHCLK (=CPUCLK/2 on NE1, CPUCLK/N in general, N>=2)
	};

enum TArmGlobalTimerStatus
	{
	E_ArmGTmrStatus_Event		=1u				// set when timer count value matches comparator value (per-CPU)
	};

#endif




#endif	// 	__ARM_TMR_H__
