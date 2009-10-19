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

// TSubScheduler member data
#define	i_IrqCount			iExtras[9]		// count of interrupts handled
#define	i_ExcInfo			iExtras[10]		// pointer to exception info for crash debugger
#define	i_CrashState		iExtras[11]		// 0=normal, 1=this CPU faulted, 2=this CPU has received an NMI and halted
#define	i_APICID			iExtras[12]		// Local APIC ID for this CPU (starts at -1)
#define	i_IrqNestCount		iExtras[13]		// IRQ nest count for this CPU (starts at -1)
#define	i_IrqStackTop		iExtras[14]		// Top of IRQ stack for this CPU
#define	i_Tss				iExtras[15]		// Address of TSS for this CPU
#define	i_TimerMultF		iExtras[16]		// Timer frequency / Max Timer frequency * 2^32
#define	i_TimerMultI		iExtras[17]		// Max Timer frequency / Timer frequency * 2^24
#define	i_CpuMult			iExtras[18]		// CPU frequency / Max CPU frequency * 2^32
#define	i_TimestampOffset	iExtras[20]		// 64 bit value to add to CPU TSC to give NKern::Timestamp()
#define	i_TimestampOffsetL	iExtras[20]		// 
#define	i_TimestampOffsetH	iExtras[21]		// 

// TScheduler member data
#define	i_TimerMax		iExtras[16]		// Maximum per-CPU timer frequency (after prescaling)

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
	inline void Stillborn()
		{}
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



// End of file
#endif
