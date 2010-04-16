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
// e32\include\kernel\arm\arm.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __ARM_KERN_H__
#define __ARM_KERN_H__

#ifdef __STANDALONE_NANOKERNEL__
#include <e32cmn.h>
#include <nk_priv.h>
#include <nk_plat.h>
#include <kernel/arm/arm_types.h>
#else
#include <e32const.h>
#include <plat_priv.h>
#include <assp.h>
#include <kernel/arm/arm_types.h>
#endif

const TLinAddr KArmResetVectorAddr=0x00;			/**< @internalTechnology */
const TLinAddr KArmUndefVectorAddr=0x04;			/**< @internalTechnology */
const TLinAddr KArmSwiVectorAddr=0x08;				/**< @internalTechnology */
const TLinAddr KArmPrefetchAbortVectorAddr=0x0C;	/**< @internalTechnology */
const TLinAddr KArmDataAbortVectorAddr=0x10;		/**< @internalTechnology */
const TLinAddr KArmReservedVectorAddr=0x14;			/**< @internalTechnology */
const TLinAddr KArmIrqVectorAddr=0x18;				/**< @internalTechnology */
const TLinAddr KArmFiqVectorAddr=0x1C;				/**< @internalTechnology */

/**
@internalTechnology
*/
enum TArmVectors
	{
	EArmVectorUndef,EArmVectorSwi,
	EArmVectorAbortPrefetch,EArmVectorAbortData,
	EArmVectorReserved,
	EArmVectorIrq,EArmVectorFiq
	};

/**
@publishedPartner
@released
*/
enum TArmFlags
	{
	ECpuThumb=0x00000020,	// THUMB state
	ECpuNoFiq=0x00000040,	// FIQ interrupt mask
	ECpuNoIrq=0x00000080,	// IRQ interrupt mask
#if defined(__CPU_ARMV6) || defined(__CPU_ARMV7)
	ECpuAf=0x00000100,		// Imprecise abort mask
	ECpuEf=0x00000200,		// Load/store endianness select
	ECpuGE0f=0x00010000,	// SIMD condition flag 0
	ECpuGE1f=0x00020000,	// SIMD condition flag 1
	ECpuGE2f=0x00040000,	// SIMD condition flag 2
	ECpuGE3f=0x00080000,	// SIMD condition flag 3
#endif
#if defined(__CPU_HAS_JAZELLE) || defined(__CPU_ARMV6) || defined(__CPU_ARMV7)
	ECpuJf=0x01000000,		// JAVA state
#endif
#if defined(__ENHANCED_DSP_INSTRUCTIONS) || defined(__CPU_ARMV6)
	ECpuQf=0x08000000,		// sticky overflow flag
#endif
	ECpuVf=0x10000000,
	ECpuCf=0x20000000,
	ECpuZf=0x40000000,
	ECpuNf=(TInt)0x80000000
	};

// ARM instruction fields
const TInt KArmMBit=0x08000000;			/**< @internalComponent */ // Multiple/single transfer
const TInt KArmPBit=0x01000000;			/**< @internalComponent */ // pre/post indexing
const TInt KArmUBit=0x00800000;			/**< @internalComponent */ // down/up
const TInt KArmSBit=0x00400000;			/**< @internalComponent */ // S bit in LDM/STM
const TInt KArmWBit=0x00200000;			/**< @internalComponent */ // Writeback/no writeback
const TInt KArmLBit=0x00100000;			/**< @internalComponent */ // Load/store

const TInt KArmRnPos=16;				/**< @internalComponent */
const TInt KArmRdPos=12;				/**< @internalComponent */
const TInt KArmRmPos=0;					/**< @internalComponent */
const TInt KArmRnMask=(0xf<<KArmRnPos);	/**< @internalComponent */
const TInt KArmRdMask=(0xf<<KArmRdPos);	/**< @internalComponent */
const TInt KArmRmMask=(0xf<<KArmRmPos);	/**< @internalComponent */

#ifdef __CPU_THUMB
// THUMB instruction fields
const TInt KThumbUnusualRegPos=8;		/**< @internalComponent */
const TInt KThumbUsualRmPos=6;			/**< @internalComponent */
const TInt KThumbUsualRnPos=3;			/**< @internalComponent */
const TInt KThumbUsualRdPos=0;			/**< @internalComponent */
const TInt KThumbImm5Pos=6;				/**< @internalComponent */
const TInt KThumbImm8Pos=0;				/**< @internalComponent */

const TInt KThumbUnusualRegMask=(0x7<<KThumbUnusualRegPos);	/**< @internalComponent */
const TInt KThumbUsualRmMask=(0x7<<KThumbUsualRmPos);		/**< @internalComponent */
const TInt KThumbUsualRnMask=(0x7<<KThumbUsualRnPos);		/**< @internalComponent */
const TInt KThumbUsualRdMask=(0x7<<KThumbUsualRdPos);		/**< @internalComponent */
const TInt KThumbImm5Mask=(0x1f<<KThumbImm5Pos);			/**< @internalComponent */
const TInt KThumbImm8Mask=(0xff<<KThumbImm8Pos);			/**< @internalComponent */

const TInt KThumbLBit=0x0800;			/**< @internalComponent */
#endif

//
// information passed to the exception handler
//
/**
@publishedPartner
@released
*/
enum TArmExcCode
	{
	EArmExceptionPrefetchAbort=0,
	EArmExceptionDataAbort=1,
	EArmExceptionUndefinedOpcode=2,
	};

/**
@publishedPartner
@released
*/
#ifdef __SMP__
class TArmExcInfo
	{
public:
	TArmReg iFaultAddress;
	TArmReg iFaultStatus;
	TArmReg iSpsrSvc;
	TArmReg	i_TArmExcInfo_Pad1;
	TArmReg	iR13Svc;
	TArmReg	iR14Svc;
	TArmReg iR0;
	TArmReg iR1;
	TArmReg iR2;
	TArmReg iR3;
	TArmReg iR4;
	TArmReg iR5;
	TArmReg iR6;
	TArmReg iR7;
	TArmReg iR8;
	TArmReg iR9;
	TArmReg iR10;
	TArmReg iR11;
	TArmReg iR12;
	TArmReg iR13;		// sp_usr
	TArmReg iR14;		// lr_usr
	TArmReg	iExcCode;	// 0=prefetch abort, 1=data abort, 2=undefined instruction
	TArmReg iR15;
	TArmReg iCpsr;
	};
#else
class TArmExcInfo
	{
public:
	TArmReg iCpsr;
	TInt	iExcCode;	// 0=prefetch abort, 1=data abort, 2=undefined instruction
	TArmReg	iR13Svc;
	TArmReg iR4;
	TArmReg iR5;
	TArmReg iR6;
	TArmReg iR7;
	TArmReg iR8;
	TArmReg iR9;
	TArmReg iR10;
	TArmReg iR11;
	TArmReg iR14Svc;
	TArmReg iFaultAddress;
	TArmReg iFaultStatus;
	TArmReg iSpsrSvc;
	TArmReg iR13;		// sp_usr
	TArmReg iR14;		// lr_usr
	TArmReg iR0;
	TArmReg iR1;
	TArmReg iR2;
	TArmReg iR3;
	TArmReg iR12;
	TArmReg iR15;
	};
#endif

extern "C" {
extern TLinAddr SuperPageAddress;
}

/**
Miscellaneous ARM CPU related functions

@publishedPartner
@released
*/
class Arm
	{
public:
	enum {EDebugPortJTAG=42};								/**< @internalComponent */
public:
	static void Init1Interrupts();							/**< @internalComponent */
	static void GetUserSpAndLr(TAny* /*aReg[2]*/);			/**< @internalComponent */
	static void SetUserSpAndLr(TAny* /*aReg[2]*/);			/**< @internalComponent */
public:
	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static void SetIrqHandler(TLinAddr aHandler);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static void SetFiqHandler(TLinAddr aHandler);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static void SetIdleHandler(TCpuIdleHandlerFn aHandler, TAny* aPtr);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static TInt DebugOutJTAG(TUint aChar);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static TInt DebugInJTAG(TUint32& aRxData);

	/**
	@internalTechnology
	*/
	IMPORT_C static TLinAddr IrqReturnAddress();

	static void SaveState(SFullArmRegSet& a);				/**< @internalComponent */
	static void UpdateState(SFullArmRegSet& aF, TArmExcInfo& aX);			/**< @internalComponent */
	static void RestoreState(SFullArmRegSet& a);							/**< @internalComponent */
	static TArmReg* Reg(SFullArmRegSet& a, TInt aRegNum, TArmReg aMode);	/**< @internalComponent */
public:
#ifndef __STANDALONE_NANOKERNEL__
	static Asic* TheAsic;									/**< @internalComponent */
#endif
#ifdef __CPU_HAS_VFP
	static TUint32 VfpDefaultFpScr;							/**< @internalComponent */
#ifdef __SMP__
	static NThread* VfpThread[KMaxCpus];					/**< @internalComponent */
#else
	static NThread* VfpThread[1];							/**< @internalComponent */
#endif // __SMP__
#endif // __CPU_HAS_VFP
	typedef TInt (*TVfpBounceHandler)(TArmExcInfo* aPtr);	/**< @internalComponent */
	static TVfpBounceHandler VfpBounceHandler;				/**< @internalComponent */
#ifdef __CPU_ARM_USE_DOMAINS
	static TUint32 Dacr();									/**< @internalComponent */
	static void SetDacr(TUint32 aDacr);						/**< @internalComponent */
	static TUint32 ModifyDacr(TUint32 aClearMask, TUint32 aSetMask);		/**< @internalComponent */
	static TUint32 DefaultDomainAccess;						/**< @internalComponent */
#endif
	IMPORT_C static TUint32 Car();							/**< @internalComponent */
	IMPORT_C static TUint32 ModifyCar(TUint32 aClearMask, TUint32 aSetMask);	/**< @internalComponent */
#ifdef __CPU_HAS_COPROCESSOR_ACCESS_REG
	static void SetCar(TUint32 aCar);						/**< @internalComponent */
	static TUint32 DefaultCoprocessorAccess;				/**< @internalComponent */
#endif
	IMPORT_C static TUint32 FpExc();						/**< @internalComponent */
	IMPORT_C static TUint32 ModifyFpExc(TUint32 aClearMask, TUint32 aSetMask);	/**< @internalComponent */
	IMPORT_C static TUint32 FpScr();						/**< @internalComponent */
	IMPORT_C static TUint32 ModifyFpScr(TUint32 aClearMask, TUint32 aSetMask);	/**< @internalComponent */
	IMPORT_C static void SetVfpBounceHandler(TVfpBounceHandler aHandler);		/**< @internalComponent */
	IMPORT_C static void SetVfpDefaultFpScr(TUint32 aFpScr);					/**< @internalComponent */
#ifdef __CPU_HAS_VFP
	static void SetFpExc(TUint32 aCar);						/**< @internalComponent */
#ifdef __VFP_V3
	static TBool NeonPresent();								/**< @internalComponent */
#endif
#endif
#ifdef __CPU_HAS_MMU
	static TBool MmuActive();								/**< @internalComponent */
	static TUint32 MmuTTBR0();								/**< @internalComponent */
#endif
	};

#ifdef __FIQ_RESERVED_FOR_SECURE_STATE__
#undef __FIQ_IS_UNCONTROLLED__
#define __FIQ_IS_UNCONTROLLED__
#endif

/**
A mask indicating all interrupts under control of Symbian OS.
@internalComponent
*/
#ifdef __FIQ_IS_UNCONTROLLED__
const TInt KAllInterruptsMask = 0x80;
#else
const TInt KAllInterruptsMask = 0xc0;
#endif

// DEBUG-mode definitions
#ifdef _DEBUG
/**
@publishedPartner
@released
*/
#define ASM_DEBUG4(fn,a0,a1,a2,a3)	asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mov r2,"  # a2 );				\
									asm("mov r3,"  # a3 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_DEBUG3(fn,a0,a1,a2)		asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mov r2,"  # a2 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_DEBUG2(fn,a0,a1)		asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_DEBUG1(fn,a0)			asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_DEBUG0(fn)				asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_CDEBUG4(cc,fn,a0,a1,a2,a3)	asm("b" #cc " .+40 ");			\
									asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mov r2,"  # a2 );				\
									asm("mov r3,"  # a3 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_CDEBUG3(cc,fn,a0,a1,a2)	asm("b" #cc " .+36 ");				\
									asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mov r2,"  # a2 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_CDEBUG2(cc,fn,a0,a1)	asm("b" #cc " .+32 ");				\
									asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mov r1,"  # a1 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_CDEBUG1(cc,fn,a0)		asm("b" #cc " .+28 ");				\
									asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mov r0,"  # a0 );				\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");

/**
@publishedPartner
@released
*/
#define ASM_CDEBUG0(cc,fn)			asm("b" #cc " .+24 ");				\
									asm("stmfd sp!, {r0-r5,ip,lr} ");	\
									asm("mrs r4, cpsr ");				\
									asm("mov r5, sp ");					\
									asm("bic sp, sp, #4 ");				\
									asm("bl __DebugMsg"#fn );			\
									asm("mov sp, r5 ");					\
									asm("msr cpsr, r4 ");				\
									asm("ldmfd sp!, {r0-r5,ip,lr} ");
#else
#define ASM_DEBUG4(fn,a0,a1,a2,a3)
#define ASM_DEBUG3(fn,a0,a1,a2)
#define ASM_DEBUG2(fn,a0,a1)
#define ASM_DEBUG1(fn,a0)
#define ASM_DEBUG0(fn)
#define ASM_CDEBUG4(cc,fn,a0,a1,a2,a3)
#define ASM_CDEBUG3(cc,fn,a0,a1,a2)
#define ASM_CDEBUG2(cc,fn,a0,a1)
#define ASM_CDEBUG1(cc,fn,a0)
#define ASM_CDEBUG0(cc,fn)
#endif


#if defined(_DEBUG) && !defined(__STANDALONE_NANOKERNEL__) && (defined(__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)||defined(__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
//Checks of kernel preconditions
/**
@internalComponent
*/

//Kernel must be locked.
#define	ASM_CHECK_PRECONDITIONS(mask)	asm("stmfd	sp!, {r0-r4,r12,lr} ");						\
										asm("mov	r4, sp ");									\
										asm("bic	sp, sp, #4 ");								\
										asm("mov	r0, #%a0" : : "i" ((mask)&0x000000ff));		\
										asm("orr	r0, r0, #%a0" : : "i" ((mask)&0x0000ff00));	\
										asm("orr	r0, r0, #%a0" : : "i" ((mask)&0x00ff0000));	\
										asm("orr	r0, r0, #%a0" : : "i" ((mask)&0xff000000));	\
										asm("mov	r1, #0 ");									\
										asm("sub	r2, pc, #40 ");								\
										asm("bl "	CSM_CFUNC(CheckPreconditions));				\
										asm("cmp	r0, #0 ");									\
										asm("mov	sp, r4 ");									\
										asm("ldmfd	sp!, {r0-r4,r12,lr} ");						\
										asm("addeq	pc, pc, #0 ");								\
										__ASM_CRASH()
		

#else
#define	ASM_CHECK_PRECONDITIONS(mask)
#endif


#if defined(__DEMAND_PAGING__) && defined(_DEBUG)

extern "C" void ASMCheckPagingSafe(TLinAddr, TLinAddr, TLinAddr, TUint);
extern "C" void ASMCheckDataPagingSafe(TLinAddr, TLinAddr, TLinAddr, TUint);

#define ASM_ASSERT_PAGING_SAFE			asm("stmdb	sp!, {r0-r3,r12,lr} ");						\
										asm("sub	r0, pc, #12 ");								\
										asm("mov	r1, lr ");									\
										asm("mov	r2, #0 ");									\
										asm("mov	r3, #%a0 " : : "i" ((TInt)KMaxTUint));		\
										asm("bl "	CSM_CFUNC(ASMCheckPagingSafe));				\
										asm("ldmia	sp!, {r0-r3,r12,lr} ");

#define ASM_ASSERT_DATA_PAGING_SAFE		asm("stmdb	sp!, {r0-r3,r12,lr} ");						\
										asm("sub	r0, pc, #12 ");								\
										asm("mov	r1, lr ");									\
										asm("mov	r2, #0 ");									\
										asm("mov	r3, #%a0" : : "i" ((TInt)KMaxTUint));		\
										asm("bl "	CSM_CFUNC(ASMCheckDataPagingSafe));			\
										asm("ldmia	sp!, {r0-r3,r12,lr} ");

#else

#define ASM_ASSERT_PAGING_SAFE
#define ASM_ASSERT_DATA_PAGING_SAFE

#endif



#endif
