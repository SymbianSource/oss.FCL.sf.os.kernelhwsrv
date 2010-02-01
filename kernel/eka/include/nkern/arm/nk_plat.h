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
// e32\include\nkern\arm\nk_plat.h
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
#define __SCHEDULER_MACHINE_CODED__
#define __DFC_MACHINE_CODED__
#define __MSTIM_MACHINE_CODED__
#define __PRI_LIST_MACHINE_CODED__
#define __FAST_SEM_MACHINE_CODED__
#define __FAST_MUTEX_MACHINE_CODED__
#define __USER_CONTEXT_TYPE_MACHINE_CODED__
#define __CLIENT_REQUEST_MACHINE_CODED__

// TScheduler member data
#define	i_Regs				iExtras[14]
#define	i_ExcInfo			iExtras[15]		// pointer to exception info for crash debugger

const TUint32 KNThreadContextFlagThumbBit0=1;

#ifdef __CPU_ARM_USE_DOMAINS
#define DOMAIN_STACK_SPACE	4
#else
#define DOMAIN_STACK_SPACE	0
#endif
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
#define CAR_STACK_SPACE	4
#else
#define CAR_STACK_SPACE	0
#endif
#ifdef __CPU_HAS_VFP
#define VFP_STACK_SPACE	4
#else
#define VFP_STACK_SPACE	0
#endif
#ifdef __CPU_HAS_CP15_THREAD_ID_REG
#define TID_STACK_SPACE	4
#else 
#define TID_STACK_SPACE	0
#endif 
#ifdef __CPU_SUPPORT_THUMB2EE
#define THUMB2EE_STACK_SPACE 4
#else
#define THUMB2EE_STACK_SPACE 0
#endif

#define EXTRA_STACK_SPACE	(DOMAIN_STACK_SPACE+CAR_STACK_SPACE+VFP_STACK_SPACE+TID_STACK_SPACE+THUMB2EE_STACK_SPACE)

#ifdef __INCLUDE_REG_OFFSETS__
// Positions of registers on stack, relative to saved SP
#define EXTRA_WORDS	(EXTRA_STACK_SPACE/4)

#ifdef __CPU_HAS_VFP
#define SP_FPEXC	((THUMB2EE_STACK_SPACE+TID_STACK_SPACE)/4)
#endif
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
#define	SP_CAR		((THUMB2EE_STACK_SPACE+TID_STACK_SPACE+VFP_STACK_SPACE)/4)
#endif
#ifdef __CPU_ARM_USE_DOMAINS
#define SP_DACR		((THUMB2EE_STACK_SPACE+TID_STACK_SPACE+VFP_STACK_SPACE+CAR_STACK_SPACE)/4)
#endif

#define	SP_R13U		EXTRA_WORDS
#define	SP_R14U		(SP_R13U+1)
#define	SP_SPSR		(SP_R13U+2)
#define	SP_R4		(SP_R13U+3)
#define	SP_R5		(SP_R13U+4)
#define	SP_R6		(SP_R13U+5)
#define	SP_R7		(SP_R13U+6)
#define	SP_R8		(SP_R13U+7)
#define	SP_R9		(SP_R13U+8)
#define	SP_R10		(SP_R13U+9)
#define	SP_R11		(SP_R13U+10)
#define	SP_PC		(SP_R13U+11)

#define SP_NEXT		(SP_PC+1)		// first word on stack before reschedule
#endif	// __INCLUDE_REG_OFFSETS__

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
		EContextUserInterruptDied,  /**< Killed while preempted by interrupt taken in user mode */
		EContextSvsrInterrupt1,     /**< Preempted by interrupt taken in executive call handler */
		EContextSvsrInterrupt1Died, /**< Killed while preempted by interrupt taken in executive call handler */
		EContextSvsrInterrupt2,     /**< Preempted by interrupt taken in executive call handler */
		EContextSvsrInterrupt2Died, /**< Killed while preempted by interrupt taken in executive call handler */
		EContextWFAR,               /**< Blocked on User::WaitForAnyRequest() */
		EContextWFARDied,           /**< Killed while blocked on User::WaitForAnyRequest() */
		EContextExec,				/**< Slow executive call */
		EContextKernel,				/**< Kernel side context (for kernel threads) */
		EContextUserIntrCallback,	/**< Blocked/preempted in a user callback on the way back from interrupt */
		EContextWFARCallback,		/**< Blocked/preempted in a user callback on the way back from User::WFAR */
		};

	IMPORT_C static const TArmContextElement* const* UserContextTables();
	IMPORT_C TUserContextType UserContextType();
	inline TInt SetUserContextType()
		{ return iSpare3=UserContextType(); }
	inline void ResetUserContextType()
		{ if(iSpare3>EContextUndefined && iSpare3<EContextUserIntrCallback) iSpare3=EContextUndefined; }
	void GetContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask, const TArmContextElement* aContextTable);
	void GetUserContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask);
	void SetUserContext(const TArmRegSet& aContext);
	void GetSystemContext(TArmRegSet& aContext, TUint32& aAvailRegistersMask);

	void ModifyUsp(TLinAddr aUsp);

#ifdef __CPU_ARM_USE_DOMAINS
	TUint32 Dacr();
	void SetDacr(TUint32 aDacr);
	TUint32 ModifyDacr(TUint32 aClearMask, TUint32 aSetMask);
#endif

#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
	void SetCar(TUint32 aDacr);
#endif
	IMPORT_C TUint32 Car();
	IMPORT_C TUint32 ModifyCar(TUint32 aClearMask, TUint32 aSetMask);

#ifdef __CPU_HAS_VFP
	void SetFpExc(TUint32 aDacr);
#endif
	IMPORT_C TUint32 FpExc();
	IMPORT_C TUint32 ModifyFpExc(TUint32 aClearMask, TUint32 aSetMask);
	};


struct SArmInterruptInfo
	{
	TLinAddr iIrqHandler;
	TLinAddr iFiqHandler;
	TUint8	 iCpuUsageFilter;
	BTrace::THandler iBTraceHandler;
	SCpuIdleHandler iCpuIdleHandler;
	};

extern "C" SArmInterruptInfo ArmInterruptInfo;

#endif
