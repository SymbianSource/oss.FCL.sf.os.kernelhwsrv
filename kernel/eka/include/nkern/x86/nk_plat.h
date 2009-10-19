// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkern\x86\nk_plat.h
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

#define IRQ_STACK_SIZE	1024

//#define __SCHEDULER_MACHINE_CODED__
//#define __DFC_MACHINE_CODED__
//#define __MSTIM_MACHINE_CODED__
//#define __PRI_LIST_MACHINE_CODED__
//#define __FAST_SEM_MACHINE_CODED__
//#define __FAST_MUTEX_MACHINE_CODED__

// TScheduler member data
#define	i_ExcInfo			iExtras[15]		// pointer to exception info for crash debugger

class TX86RegSet;
class NThread : public NThreadBase
	{
public:
	TInt Create(SNThreadCreateInfo& anInfo, TBool aInitial);
	inline void Stillborn()
		{}
	void GetUserContext(TX86RegSet& aContext);
	void ModifyUsp(TLinAddr aUsp);
public:
	TUint32	i_NThread_Pad1;
	TUint64	iCoprocessorState[64];	// state of FPU, SSE, SSE2
	};

__ASSERT_COMPILE(!(_FOFF(NThread,iCoprocessorState)&7));

// Positions of registers on stack, relative to saved SP
struct SThreadStack
	{
	TUint32 iCR0;
	TUint32 iEbx;
	TUint32 iEsi;
	TUint32 iEdi;
	TUint32 iEbp;
	TUint32 iGs;
	TUint32 iFs;
	TUint32 iReschedFlag;
	TUint32 iEip;
	};

extern "C" {
GLREF_D TUint32 X86_IrqStack[IRQ_STACK_SIZE/4];
GLREF_D TLinAddr X86_IrqHandler;
GLREF_D TInt X86_IrqNestCount;
GLREF_D SCpuIdleHandler CpuIdleHandler;
GLREF_D TBool X86_UseGlobalPTEs;
GLREF_D TUint64 DefaultCoprocessorState[64];
}


/** Ensure the ordering of explicit memory writes

	On x86 this is a no-op
*/
#define	wmb()
#define smp_wmb()

/** Ensure the ordering of explicit memory accesses

	On x86 any instruction with the LOCK prefix does this
*/
#ifdef __GCC32__
#define	mb()	__asm__ __volatile__("lock add dword ptr [esp], 0" : : : "memory")
#else
#define	mb()	do { _asm lock add dword ptr [esp], 0 } while (0)
#endif
#define smp_mb()

// End of file
#endif
