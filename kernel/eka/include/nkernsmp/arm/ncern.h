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
// e32\include\nkernsmp\arm\ncern.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef __NCERN_H__
#define __NCERN_H__

#ifdef __FIQ_IS_UNCONTROLLED__
#define	__ASM_CLI()							CPSIDI			/* Disable all interrupts */
#define	__ASM_STI()							CPSIEI			/* Enable all interrupts */
#define	__ASM_CLI1()						CPSIDI			/* Disable IRQ only */
#define	__ASM_STI1()						CPSIEI			/* Enable IRQ only */
#define	__ASM_CLI2()										/* Disable FIQ only */
#define	__ASM_STI2()										/* Enable FIQ only */

#define	__ASM_CLI_MODE(mode)				CPSIDIM(mode)	/* Disable all interrupts and change mode */
#define	__ASM_STI_MODE(mode)				CPSIEIM(mode)	/* Enable all interrupts and change mode */
#define	__ASM_CLI1_MODE(mode)				CPSIDIM(mode)	/* Disable IRQ only and change mode */
#define	__ASM_STI1_MODE(mode)				CPSIEIM(mode)	/* Enable IRQ only and change mode */
#define	__ASM_CLI2_MODE(mode)				CPSCHM(mode)	/* Disable FIQ only and change mode */
#define	__ASM_STI2_MODE(mode)				CPSCHM(mode)	/* Enable FIQ only and change mode */

#else
#define	__ASM_CLI()							CPSIDIF			/* Disable all interrupts */
#define	__ASM_STI()							CPSIEIF			/* Enable all interrupts */
#define	__ASM_CLI1()						CPSIDI			/* Disable IRQ only */
#define	__ASM_STI1()						CPSIEI			/* Enable IRQ only */
#define	__ASM_CLI2()						CPSIDF			/* Disable FIQ only */
#define	__ASM_STI2()						CPSIEF			/* Enable FIQ only */

#define	__ASM_CLI_MODE(mode)				CPSIDIFM(mode)	/* Disable all interrupts and change mode */
#define	__ASM_STI_MODE(mode)				CPSIEIFM(mode)	/* Enable all interrupts and change mode */
#define	__ASM_CLI1_MODE(mode)				CPSIDIM(mode)	/* Disable IRQ only and change mode */
#define	__ASM_STI1_MODE(mode)				CPSIEIM(mode)	/* Enable IRQ only and change mode */
#define	__ASM_CLI2_MODE(mode)				CPSIDFM(mode)	/* Disable FIQ only and change mode */
#define	__ASM_STI2_MODE(mode)				CPSIEFM(mode)	/* Enable FIQ only and change mode */
#endif

/** Information needed to boot an AP (ARM specific)

@internalTechnology
*/
struct SArmAPBootInfo : public SAPBootInfo
	{
	TLinAddr	iAPBootLin;			// linear address of AP boot page (uncached)
	T_UintPtr	iAPBootPhys;		// physical address of AP boot page (uncached)
	TLinAddr	iAPBootCodeLin;		// linear address of AP boot code (part of bootstrap)
	T_UintPtr	iAPBootCodePhys;	// physical address of AP boot code (part of bootstrap)
	T_UintPtr	iAPBootPageDirPhys;	// physical address of AP boot page directory
	TLinAddr	iInitR13Fiq;		// initial value for R13_fiq
	TLinAddr	iInitR13Irq;		// initial value for R13_irq
	TLinAddr	iInitR13Abt;		// initial value for R13_abt
	TLinAddr	iInitR13Und;		// initial value for R13_und
	};

typedef void (*TDetachComplete)(void);

struct SPerCpuUncached
	{
	volatile TUint32	iDetachCount;		// Number of times core has detached from SMP cluster
	volatile TUint32	iAttachCount;		// Number of times core has reattached to SMP cluster
	volatile TBool		iPowerOffReq;		// TRUE if core needs to be powered off
	volatile TBool		iPowerOnReq;		// TRUE if core needs to be powered on
	TDetachComplete		iDetachCompleteFn;	// idle handler jumps to this to request power down if necessary
											// after cleaning and disabling caches, detaching from SMP cluster
											// and saving state required to bring the core back up again
	volatile TUint32	iDetachCompleteCpus;
	};

union UPerCpuUncached
	{
	SPerCpuUncached		iU;
	volatile TUint64	i__Dummy[8];
	};

__ASSERT_COMPILE(sizeof(SPerCpuUncached) <= 8*sizeof(TUint64));

/** Timer frequency specification

Stores a frequency as a fraction of a (separately stored) maximum.
The frequency must be at least 1/256 of the maximum.

@internalTechnology
@prototype
*/
struct STimerMult
	{
	TUint32		iFreq;						// frequency as a fraction of maximum possible, multiplied by 2^32
	TUint32		iInverse;					// 2^24/(iFreq/2^32) = 2^56/iFreq
	};

/** Function to power up a CPU
@publishedPartner
@prototype
*/
typedef void (*TCpuPowerUpFn)(TInt aCpu, SPerCpuUncached* aU);

/** Function to power down a CPU
@publishedPartner
@prototype
*/
typedef void (*TCpuPowerDownFn)(TInt aCpu, SPerCpuUncached* aU);

/** Variant interface block
@internalTechnology
@prototype
*/
struct SVariantInterfaceBlock : public SInterfaceBlockBase
	{
	TUint64		iMaxCpuClock;				// maximum possible CPU clock frequency on this system
	TUint16		iTimerGap1;
	TUint16		iTimerGap2;
	TUint32		iMaxTimerClock;				// maximum possible local timer clock frequency
	TLinAddr	iScuAddr;					// address of SCU
	TLinAddr	iGicDistAddr;				// address of GIC Distributor
	TLinAddr	iGicCpuIfcAddr;				// address of GIC CPU interface (must be same for all CPUs)
	TLinAddr	iLocalTimerAddr;			// address of per-CPU timer (must be same for all CPUs)
	TLinAddr	iGlobalTimerAddr;			// address of global timer if it exists
	volatile STimerMult*	iTimerMult[KMaxCpus];	// timer[i] frequency / iMaxTimerClock * 2^32
	volatile TUint32*		iCpuMult[KMaxCpus];		// CPU[i] frequency / iMaxCpuClock * 2^32
	UPerCpuUncached*		iUncached[KMaxCpus];	// Pointer to uncached memory for each CPU
	TCpuPowerUpFn			iCpuPowerUpFn;			// function used to power up a retired CPU (NULL if core control not supported)
	TCpuPowerDownFn			iCpuPowerDownFn;		// function used to power down a CPU (NULL if power down done within idle handler itself)
	};

// End of file
#endif
