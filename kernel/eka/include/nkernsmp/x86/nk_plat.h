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
// e32\include\nkernsmp\x86\nk_plat.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __NK_X86_H__
#define __NK_X86_H__
#include <nk_cpu.h>

class TSubScheduler;
class TScheduler;
struct TX86Tss;

// TSubScheduler member data
struct TSubSchedulerX
	{
	TUint32				iSSXP[9];
	volatile TUint32	iIrqCount;				// count of interrupts handled
	TAny*				iExcInfo;				// pointer to exception info for crash debugger
	volatile TInt		iCrashState;			// 0=normal, 1=this CPU faulted, 2=this CPU has received an NMI and halted
	TUint32				iAPICID;				// Local APIC ID for this CPU (starts at -1)
	volatile TInt		iIrqNestCount;			// IRQ nest count for this CPU (starts at -1)
	TLinAddr			iIrqStackTop;			// Top of IRQ stack for this CPU
	TX86Tss*			iTss;					// Address of TSS for this CPU
	SRatioInv			iCpuFreqRI;				// Ratio of CPU frequency to maximum possible CPU frequency
	SRatioInv			iTimerFreqRI;			// Ratio of CPU local timer frequency to maximum possible

	volatile TUint64HL	iTimestampOffset;		// 64 bit value to add to CPU TSC to give NKern::Timestamp()

	TUint32				iSSXP2[36];
	TUint64				iSSXP3;					// one 64 bit value to guarantee alignment
	};

// TScheduler member data
struct TSchedulerX
	{
	TUint64				iTimerMax;				// Maximum per-CPU timer frequency (after prescaling)
	TUint32				iSXP[30];
	};


#define CRASH_IPI_VECTOR			0x27
#define RESCHED_IPI_VECTOR			0x28
#define TIMESLICE_VECTOR			0x29
#define GENERIC_IPI_VECTOR			0x2A
#define TRANSFERRED_IRQ_VECTOR		0x2E
#define SPURIOUS_INTERRUPT_VECTOR	0x2F

extern "C" TSubScheduler* SubSchedulerLookupTable[256];		// look up subscheduler from APIC ID

#define IRQ_STACK_SIZE	1024

//#define __SCHEDULER_MACHINE_CODED__
//#define __DFC_MACHINE_CODED__
//#define __MSTIM_MACHINE_CODED__
//#define __PRI_LIST_MACHINE_CODED__
//#define __FAST_SEM_MACHINE_CODED__
//#define __FAST_MUTEX_MACHINE_CODED__

class TX86RegSet;
class NThread : public NThreadBase
	{
public:
	TInt Create(SNThreadCreateInfo& anInfo, TBool aInitial);
	void Stillborn();
	void GetUserContext(TX86RegSet& aContext, TUint32& aAvailRegMask);
	void SetUserContext(const TX86RegSet& aContext, TUint32& aRegMask);
	void GetSystemContext(TX86RegSet& aContext, TUint32& aAvailRegMask);
	void CompleteContextSave();
public:
	TUint64	iCoprocessorState[64];	// state of FPU, SSE, SSE2
	};

__ASSERT_COMPILE(!(_FOFF(NThread,iCoprocessorState)&7));


// Positions of registers on stack, relative to saved SP
struct SThreadReschedStack
	{
	TUint32 iCR0;
	TUint32 iReschedFlag;
	TUint32 iEip;
	TUint32 iReason;
	};

// Registers pushed on stack for all exceptions other than slow exec
struct SThreadExcStack
	{
	TUint32	iEcx;
	TUint32	iEdx;
	TUint32	iEbx;
	TUint32	iEsi;
	TUint32	iEdi;
	TUint32	iEbp;
	TUint32	iEax;
	TUint32	iDs;
	TUint32	iEs;
	TUint32	iFs;
	TUint32	iGs;
	TUint32	iVector;
	TUint32	iError;
	TUint32	iEip;
	TUint32	iCs;
	TUint32	iEflags;
	TUint32	iEsp3;		// only if iCs does not indicate CPL=0
	TUint32	iSs3;		// only if iCs does not indicate CPL=0
	};

// Registers pushed on stack for slow exec
struct SThreadSlowExecStack
	{
	TUint32	iEcx;
	TUint32	iEdx;
	TUint32	iEbx;
	TUint32	iEsi;
	TUint32	iEdi;
	TUint32	iEbp;
	TUint32	iEax;
	TUint32	iDs;
	TUint32	iEs;
	TUint32	iFs;
	TUint32	iGs;
	TUint32	iArgs[8];	// space for extra arguments copied from user side
	TUint32	iVector;
	TUint32	iError;
	TUint32	iEip;
	TUint32	iCs;
	TUint32	iEflags;
	TUint32	iEsp3;		// only if iCs does not indicate CPL=0
	TUint32	iSs3;		// only if iCs does not indicate CPL=0
	};

// Top of stack after thread creation for threads with parameter block passed
// by value.
struct SThreadStackStub
	{
	enum {EVector=0xffffffffu};
	TLinAddr iPBlock;	// pointer to parameter block
	TUint32	iVector;
	TUint32	iError;
	TUint32	iEip;
	TUint32	iCs;
	TUint32	iEflags;
	};

// Stack structure at thread creation either at top of stack (if parameter block
// passed by reference) or below parameter block if passed by value.
struct SThreadInitStack
	{
	enum {EVector=0xfffffffeu};
	SThreadReschedStack		iR;
	SThreadExcStack			iX;
	};


extern "C" {
GLREF_D TLinAddr X86_IrqHandler;
GLREF_D SCpuIdleHandler CpuIdleHandler;
GLREF_D TUint32 X86_CPUID;
GLREF_D TBool X86_UseGlobalPTEs;
GLREF_D TUint64 DefaultCoprocessorState[64];
}

/** Ensure the ordering of explicit memory writes

	On x86 this is a no-op
*/
#define	wmb()
#define	smp_wmb()	wmb()

/** Ensure the ordering of explicit memory accesses

	On x86 any instruction with the LOCK prefix does this
*/
#ifdef __GCC32__
#define	mb()	__asm__ __volatile__("lock add dword ptr [esp], 0" : : : "memory")
#else
#define	mb()	do { _asm lock add dword ptr [esp], 0 } while (0)
#endif
#define smp_mb()	mb()


/**
@internalComponent
*/
extern "C" void send_resched_ipis(TUint32 aMask);


// End of file
#endif
