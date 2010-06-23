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

class TSubScheduler;
class TScheduler;
struct SFullArmRegSet;
struct ArmScu;
struct GicDistributor;
struct GicCpuIfc;
struct ArmLocalTimer;
struct ArmGlobalTimer;

// TSubScheduler member data
struct TSubSchedulerX
	{
	TUint32				iSSXP[3];
	ArmGlobalTimer*		iGlobalTimerAddr;		// Address of global timer registers (also in TScheduler)
	ArmScu*				iScuAddr;				// Address of SCU (also in TScheduler)
	GicDistributor*		iGicDistAddr;			// Address of GIC Distributor (also in TScheduler)
	GicCpuIfc*			iGicCpuIfcAddr;			// Address of GIC CPU Interface (also in TScheduler)
	ArmLocalTimer*		iLocalTimerAddr;		// Address of local timer registers (also in TScheduler)
	volatile TUint32	iIrqCount;				// count of interrupts handled
	volatile TInt		iIrqNestCount;			// IRQ nest count for this CPU (starts at -1)
	TAny*				iExcInfo;				// pointer to exception info for crash debugger
	volatile TInt		iCrashState;			// 0=normal, 1=this CPU faulted, 2=this CPU has received an NMI and halted
	union {
		TLinAddr		iAbtStackTop;			// Top of ABT stack for this CPU, also used to point to SFullArmRegSet
		SFullArmRegSet* iRegs;
		};
	TLinAddr			iUndStackTop;			// Top of UND stack for this CPU
	TLinAddr			iFiqStackTop;			// Top of FIQ stack for this CPU
	TLinAddr			iIrqStackTop;			// Top of IRQ stack for this CPU
	SRatioInv* volatile	iNewCpuFreqRI;			// set when CPU frequency has been changed
	SRatioInv* volatile	iNewTimerFreqRI;		// set when CPU local timer frequency has been changed
	SRatioInv			iCpuFreqRI;				// Ratio of CPU frequency to maximum possible CPU frequency
	SRatioInv			iTimerFreqRI;			// Ratio of CPU local timer frequency to maximum possible

	TUint32				iSSXP2[36];
	TUint64				iSSXP3;					// one 64 bit value to guarantee alignment
	};

// TScheduler member data
struct TSchedulerX
	{
	TUint64				iTimerMax;				// Maximum per-CPU timer frequency (after prescaling)
	TUint32				iSXP[1];
	ArmGlobalTimer*		iGlobalTimerAddr;		// Address of global timer registers (also in TSubScheduler)
	ArmScu*				iScuAddr;				// Address of SCU (also in TSubScheduler)
	GicDistributor*		iGicDistAddr;			// Address of GIC Distributor (also in TSubScheduler)
	GicCpuIfc*			iGicCpuIfcAddr;			// Address of GIC CPU Interface (also in TSubScheduler)
	ArmLocalTimer*		iLocalTimerAddr;		// Address of local timer registers (also in TSubScheduler)

	SRatioInv			iGTimerFreqRI;			// ratio of global timer frequency to maximum possible
	TUint64				iCount0;				// global timer count at last frequency change
	TUint64				iTimestamp0;			// timestamp at last frequency change

	TUint32				iSXP2[16];
	};


#define	RESCHED_IPI_VECTOR				0x00
#define	GENERIC_IPI_VECTOR				0x01
#define	TRANSFERRED_IRQ_VECTOR			0x02
#define	CRASH_IPI_VECTOR				0x03	// would really like this to be a FIQ
#define	BOOT_IPI_VECTOR					0x04	// used during boot to handshake with APs
#define	INDIRECT_POWERDOWN_IPI_VECTOR	0x04	// used to trigger core power down
#define RESERVED_IPI_VECTOR_1			0x05	// reserved for future kernel functionality
#define RESERVED_IPI_VECTOR_2			0x06	// reserved for future kernel functionality
#define IDLE_WAKEUP_IPI_VECTOR			0x07	// for use of Idle handler/Wakeup handler

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
	void Stillborn();

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
#define	SCU				(*TheScheduler.iSX.iScuAddr)
#define	GIC_DIST		(*TheScheduler.iSX.iGicDistAddr)
#define	GIC_CPU_IFC		(*TheScheduler.iSX.iGicCpuIfcAddr)
#define	LOCAL_TIMER		(*TheScheduler.iSX.iLocalTimerAddr)

#ifdef	__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK
#define	GLOBAL_TIMER	(*TheScheduler.iSX.iGlobalTimerAddr)
#endif

#endif


// End of file
#endif
