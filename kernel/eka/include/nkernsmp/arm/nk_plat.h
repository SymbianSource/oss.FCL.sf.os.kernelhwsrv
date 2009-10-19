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
// e32\include\nkernsmp\arm\nk_plat.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __NK_ARM_H__
#define __NK_ARM_H__
#include <nk_cpu.h>

// These macros are intended for Symbian use only.
// It may not be possible to build the kernel if any of these macros are undefined
//#define __SCHEDULER_MACHINE_CODED__
//#define __DFC_MACHINE_CODED__
//#define __MSTIM_MACHINE_CODED__
#define __PRI_LIST_MACHINE_CODED__
#define __FAST_SEM_MACHINE_CODED__
#define __FAST_MUTEX_MACHINE_CODED__
#define __NTHREAD_WAITSTATE_MACHINE_CODED__

// TSubScheduler member data
#define	i_ScuAddr			iExtras[4]		// Address of SCU (also in TScheduler)
#define	i_GicDistAddr		iExtras[5]		// Address of GIC Distributor (also in TScheduler)
#define	i_GicCpuIfcAddr		iExtras[6]		// Address of GIC CPU Interface (also in TScheduler)
#define	i_LocalTimerAddr	iExtras[7]		// Address of local timer registers (also in TScheduler)
#define	i_IrqCount			iExtras[8]		// count of interrupts handled
#define	i_IrqNestCount		iExtras[9]		// IRQ nest count for this CPU (starts at -1)
#define	i_ExcInfo			iExtras[10]		// pointer to exception info for crash debugger
#define	i_CrashState		iExtras[11]		// 0=normal, 1=this CPU faulted, 2=this CPU has received an NMI and halted
#define	i_AbtStackTop		iExtras[12]		// Top of ABT stack for this CPU, also used to point to SFullArmRegSet
#define	i_UndStackTop		iExtras[13]		// Top of UND stack for this CPU
#define	i_FiqStackTop		iExtras[14]		// Top of FIQ stack for this CPU
#define	i_IrqStackTop		iExtras[15]		// Top of IRQ stack for this CPU
#define	i_TimerMultF		iExtras[16]		// Timer frequency / Max Timer frequency * 2^32
#define	i_TimerMultI		iExtras[17]		// Max Timer frequency / Timer frequency * 2^24
#define	i_CpuMult			iExtras[18]		// CPU frequency / Max CPU frequency * 2^32
#define	i_LastTimerSet		iExtras[20]		// Value last written to local timer counter
#define	i_TimestampError	iExtras[21]		// Current error in the timestamp
#define	i_MaxCorrection		iExtras[22]		// Maximum correction to timestamp in one go
#define	i_TimerGap			iExtras[23]		// Timestamp ticks taken to read and write local timer counter

#define	i_Regs				iExtras[12]		// Alias for i_AbtStackTop

// TScheduler member data
#define	i_TimerMax			iExtras[16]		// Maximum per-CPU timer frequency (after prescaling)


#define	RESCHED_IPI_VECTOR			0x00
#define	GENERIC_IPI_VECTOR			0x01
#define	TRANSFERRED_IRQ_VECTOR		0x02
#define	CRASH_IPI_VECTOR			0x03	// would really like this to be a FIQ
#define	BOOT_IPI_VECTOR				0x04	// used during boot to handshake with APs
#define RESERVED_IPI_VECTOR_1		0x05	// reserved for future kernel functionality
#define RESERVED_IPI_VECTOR_2		0x06	// reserved for future kernel functionality
#define RESERVED_IPI_VECTOR_3		0x07	// reserved for future kernel functionality

#if defined(__CPU_ARM11MP__)
#define	TIMESLICE_VECTOR			0x1D	// vector 29 is per-CPU timer interrupt
											// vector 30 is per-CPU Watchdog timer when not in watchdog mode
											// vector 31 is external nIRQ local interrupt pin
#elif defined(__CPU_CORTEX_A9__)
#define	TIMESLICE_VECTOR			0x1D	// vector 29 is per-CPU timer interrupt
											// vector 30 is per-CPU Watchdog timer when not in watchdog mode
#else
#error	TIMESLICE_VECTOR not defined
#endif


//extern "C" TSubScheduler* SubSchedulerLookupTable[256];		// look up subscheduler from APIC ID

const TUint32 KNThreadContextFlagThumbBit0=1;

/** Registers saved by the scheduler

Let's just have the same stack layout for all CPUs shall we?
TEEHBR, FpExc may not be used but leave space on the stack for them.

@internalComponent
*/
struct SThreadReschedStack
	{
	TUint32	iFpExc;			// VFP enable
	TUint32	iCar;			// coprocessor access register
	TUint32	iTEEHBR;		// Thumb2-EE Handler Base
	TUint32	iRWROTID;		// User RO Thread ID
	TUint32	iRWRWTID;		// User RW Thread ID
	TUint32	iDacr;			// domain access control
	TUint32	iSpare;
	TUint32	iSpsrSvc;
	TUint32 iSPRschdFlg;	// Stack pointer plus flag indicating reschedule occurred
	TUint32	iR15;			// return address from Reschedule()
	};

/** Registers saved on any exception, interrupt or system call

@internalComponent
*/
struct SThreadExcStack
	{
	enum	TType
		{
		EPrefetch	=0,		// prefetch abort
		EData		=1,		// data abort
		EUndef		=2,		// undefined instruction
		EIrq		=3,		// IRQ interrupt
		EFiq		=4,		// FIQ interrupt
		ESvc		=5,		// SWI
		EInit		=6,		// Thread has never run
		EStub		=7,		// Stub indicating parameter block still on stack
		};

	TUint32	iR0;
	TUint32	iR1;
	TUint32	iR2;
	TUint32	iR3;
	TUint32	iR4;
	TUint32	iR5;
	TUint32	iR6;
	TUint32	iR7;
	TUint32	iR8;
	TUint32	iR9;
	TUint32	iR10;
	TUint32	iR11;
	TUint32	iR12;
	TUint32	iR13usr;		// always user mode R13
	TUint32	iR14usr;		// always user mode R14
	TUint32	iExcCode;
	TUint32	iR15;			// return address
	TUint32	iCPSR;			// return CPSR
	};

/**
@internalComponent
*/
struct SThreadStackStub
	{
	TLinAddr iPBlock;		// pointer to parameter block
	TUint32	iExcCode;		// always EStub
	TUint32	iR15;			// unused
	TUint32	iCPSR;			// unused
	};

/**
@internalComponent
*/
struct SThreadInitStack
	{
	SThreadReschedStack		iR;
	SThreadExcStack			iX;
	};


/**
@internalComponent
*/
struct SThreadIrqStack
	{
	SThreadReschedStack		iR;
	TUint32					iUMGSave;	// User memory guard state (if active)
	TUint32					iR14svc;
	SThreadExcStack			iX;
	};


class TArmContextElement;
class TArmRegSet;

/** ARM-specific part of the nano-thread abstraction.
	@internalComponent
 */
class NThread : public NThreadBase
	{
public:
	TInt Create(SNThreadCreateInfo& aInfo, TBool aInitial);
	inline void Stillborn()
		{}

	/** Value indicating what event caused thread to enter privileged mode.
		@publishedPartner
		@released
	 */
	enum TUserContextType
		{
		EContextNone=0,             /**< Thread has no user context */
		EContextException=1,		/**< Hardware exception while in user mode */
		EContextUndefined,			
		EContextUserInterrupt,		/**< Preempted by interrupt taken in user mode */
		EContextUserInterruptDied,  /**< Killed while preempted by interrupt taken in user mode */	// NOT USED
		EContextSvsrInterrupt1,     /**< Preempted by interrupt taken in executive call handler */
		EContextSvsrInterrupt1Died, /**< Killed while preempted by interrupt taken in executive call handler */	// NOT USED
		EContextSvsrInterrupt2,     /**< Preempted by interrupt taken in executive call handler */	// NOT USED
		EContextSvsrInterrupt2Died, /**< Killed while preempted by interrupt taken in executive call handler */	// NOT USED
		EContextWFAR,               /**< Blocked on User::WaitForAnyRequest() */
		EContextWFARDied,           /**< Killed while blocked on User::WaitForAnyRequest() */		// NOT USED
		EContextExec,				/**< Slow executive call */
		EContextKernel,				/**< Kernel side context (for kernel threads) */
		EContextKernel1,			/**< Kernel side context (for kernel threads) (NKern::Unlock, NKern::PreemptionPoint) */
		EContextKernel2,			/**< Kernel side context (for kernel threads) (NKern::FSWait, NKern::WaitForAnyRequest) */
		EContextKernel3,			/**< Kernel side context (for kernel threads) (Interrupt) */
		EContextKernel4,			/**< Kernel side context (for kernel threads) (Exec::WaitForAnyRequest) */
		};

	IMPORT_C static const TArmContextElement* const* UserContextTables();
	IMPORT_C TUserContextType UserContextType();
	void GetUserContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask);
	void SetUserContext(const TArmRegSet& aContext, TUint32& aRegMask);
	void GetSystemContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask);

	TUint32 Dacr();
	void SetDacr(TUint32 aDacr);
	TUint32 ModifyDacr(TUint32 aClearMask, TUint32 aSetMask);

	void SetCar(TUint32 aDacr);
	IMPORT_C TUint32 Car();
	IMPORT_C TUint32 ModifyCar(TUint32 aClearMask, TUint32 aSetMask);

#ifdef __CPU_HAS_VFP
	void SetFpExc(TUint32 aDacr);
#endif
	IMPORT_C TUint32 FpExc();
	IMPORT_C TUint32 ModifyFpExc(TUint32 aClearMask, TUint32 aSetMask);

	void CompleteContextSave();
	};


struct SArmInterruptInfo
	{
	TLinAddr iIrqHandler;
	TLinAddr iFiqHandler;
	SCpuIdleHandler iCpuIdleHandler;
	};

extern "C" SArmInterruptInfo ArmInterruptInfo;

#if defined(__ARMCC__)
#ifndef __CIA__
inline void mb()
	{
	TUint32 reg = 0;
	asm("mcr p15, 0, reg, c7, c10, 5 ");
	}

inline void arm_dsb()
	{
	TUint32 reg = 0;
	asm("mcr p15, 0, reg, c7, c10, 4 ");
	}

inline void arm_isb()
	{
	TUint32 reg = 0;
	asm("mcr p15, 0, reg, c7, c5, 4 ");
	}
#endif
#elif defined(__GNUC__) || defined(__GCC32__)
#define	mb()	\
	do	{	\
		TUint32 reg = 0;	\
		__asm__ __volatile__("mcr p15, 0, %0, c7, c10, 5" : : "r"(reg) : "memory");	\
		} while(0)

#define	arm_dsb()	\
	do	{	\
		TUint32 reg = 0;	\
		__asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4" : : "r"(reg) : "memory");	\
		} while(0)

#define	arm_isb()	\
	do	{	\
		TUint32 reg = 0;	\
		__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 4" : : "r"(reg) : "memory");	\
		} while(0)
#else
#error Unknown ARM compiler
#endif

#define	smp_mb()	mb()
#define	wmb()		mb()
#define	smp_wmb()	mb()

#ifdef	__IN_KERNEL__
struct ArmScu;
struct GicDistributor;
struct GicCpuIfc;
struct ArmLocalTimer;
#define	SCU			(*(ArmScu*)TheScheduler.i_ScuAddr)
#define	GIC_DIST	(*(GicDistributor*)TheScheduler.i_GicDistAddr)
#define	GIC_CPU_IFC	(*(GicCpuIfc*)TheScheduler.i_GicCpuIfcAddr)
#define	LOCAL_TIMER	(*(ArmLocalTimer*)TheScheduler.i_LocalTimerAddr)
#endif


// End of file
#endif
