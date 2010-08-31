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
// e32\include\nkern\nk_cpu.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __NK_CPU_H__
#define __NK_CPU_H__

#include <cpudefs.h>

#ifdef __CPU_ARM
#if defined(__CPU_GENERIC_ARM4__)
	// no cache no MMU
	#define __CPU_ARM_ABORT_MODEL_RESTORED
#endif

#if defined(__CPU_ARM710T__) || defined(__CPU_ARM720T__)
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define __CPU_ARM_ABORT_MODEL_UPDATED
	#define __CPU_WRITE_BUFFER
#endif

#ifdef __CPU_SA1__
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define __CPU_ARM_ABORT_MODEL_RESTORED
	#define __CPU_SPLIT_CACHE
	#define __CPU_SPLIT_TLB
	#define __CPU_WRITE_BUFFER
	#define __CPU_HAS_ALT_D_CACHE
	#define __CPU_WRITE_BACK_CACHE
	#define __CPU_CACHE_FLUSH_BY_DATA_READ
	#define __CPU_HAS_SINGLE_ENTRY_DCACHE_FLUSH
#endif

#if defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define __CPU_ARM_ABORT_MODEL_RESTORED
	#define __CPU_SPLIT_CACHE
	#define __CPU_SPLIT_TLB
	#define __CPU_WRITE_BUFFER
	#define __CPU_WRITE_BACK_CACHE
	#define __CPU_CACHE_FLUSH_BY_WAY_SET_INDEX
	#define __CPU_CACHE_POLICY_IN_PTE
	#define __CPU_HAS_CACHE_TYPE_REGISTER
	#define __CPU_HAS_SINGLE_ENTRY_ITLB_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_ICACHE_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_DCACHE_FLUSH
#endif

#ifdef __CPU_XSCALE__
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define __CPU_ARM_ABORT_MODEL_RESTORED
	#define __CPU_SPLIT_CACHE
	#define __CPU_SPLIT_TLB
	#define __CPU_WRITE_BUFFER
#ifndef __CPU_XSCALE_MANZANO__
	#define __CPU_HAS_ALT_D_CACHE
#endif
	#define __CPU_WRITE_BACK_CACHE
	#define __CPU_CACHE_WRITE_ALLOCATE
#ifdef __CPU_XSCALE_MANZANO__
	#define __CPU_CACHE_FLUSH_BY_WAY_SET_INDEX
#else
	#define __CPU_CACHE_FLUSH_BY_LINE_ALLOC
#endif
	#define __CPU_CACHE_POLICY_IN_PTE
	#define __CPU_HAS_CACHE_TYPE_REGISTER
	#define __CPU_HAS_SINGLE_ENTRY_ITLB_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_ICACHE_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_DCACHE_FLUSH
	#define	__CPU_HAS_BTB
	#define __CPU_USE_MMU_TEX_FIELD
	#define __CPU_HAS_COPROCESSOR_ACCESS_REG
	#define	__CPU_HAS_ACTLR
#endif

#if defined(__CPU_ARM1136__) || defined(__CPU_ARM11MP__) || defined(__CPU_ARM1176__) || defined(__CPU_CORTEX_A8__) || defined(__CPU_CORTEX_A9__)
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define __CPU_CACHE_PHYSICAL_TAG
	#define	__CPU_SUPPORTS_FAST_PROCESS_SWITCH
	#define __CPU_ARM_ABORT_MODEL_RESTORED
	#define __CPU_SPLIT_CACHE

	#if defined(__CPU_CORTEX_A9__) || defined(__CPU_CORTEX_A8__) || defined(__CPU_ARM1136__)
	#define __CPU_SPLIT_TLB
	#endif

    #if defined(__CPU_CORTEX_A8__)
    /* Internal cache controller maintains both inner & outer caches.
     * @internalComponent
     */
    #define __CPU_OUTER_CACHE_IS_INTERNAL_CACHE
    #endif



	#if defined(__CPU_CORTEX_A9__) || defined(__CPU_ARM11MP__)
	#define	__CPU_SUPPORTS_TLBIMVAA
	#endif
	
	#if defined(__CPU_CORTEX_A9__)
	#ifdef __SMP__
//	#define	__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE
	#endif
	#endif

	#if (defined(__CPU_ARM1136__) && defined(__CPU_ARM1136_ERRATUM_399234_FIXED) && !defined(__MEMMODEL_FLEXIBLE__)) || (defined(__CPU_ARM11MP__) && defined (__SMP__) )
	// Page tables on these platforms are either uncached or write through cached.
	#else
	// Page/directory tables are fully cached (write-back) on these platforms. 
	#define __CPU_PAGE_TABLES_FULLY_CACHED
	#endif
		
	#define __CPU_WRITE_BUFFER
	#define __CPU_WRITE_BACK_CACHE
	#define __CPU_CACHE_WRITE_ALLOCATE
	#define __CPU_CACHE_FLUSH_BY_WAY_SET_INDEX
	#define __CPU_CACHE_POLICY_IN_PTE
	#define __CPU_HAS_CACHE_TYPE_REGISTER
	#define __CPU_HAS_SINGLE_ENTRY_ITLB_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_ICACHE_FLUSH
	#define __CPU_HAS_SINGLE_ENTRY_DCACHE_FLUSH
	#define	__CPU_HAS_BTB
	#define __CPU_HAS_COPROCESSOR_ACCESS_REG
	#define	__CPU_HAS_PREFETCH_BUFFER
	#define	__CPU_HAS_ACTLR
	#define	__CPU_HAS_TTBR1
	
	#if !defined(__CPU_ARM1136__)
	#define	__CPU_MEMORY_TYPE_REMAPPING
	#endif

	#if defined(__CPU_ARM11MP__) && defined(__SMP__)
	#define __BROADCAST_CACHE_MAINTENANCE__
	#endif

	#if defined(__CPU_ARM11MP__) || defined(__CPU_ARM1176__)
	#define __CPU_NEEDS_BTAC_FLUSH_AFTER_ASID_CHANGE
	#endif

	#define __CPU_CACHE_HAS_COLOUR
	#define __CPU_I_CACHE_HAS_COLOUR

	#if defined(__CPU_ARM1136__) || defined(__CPU_ARM1176__)
	#define __CPU_D_CACHE_HAS_COLOUR
	#elif defined(__CPU_ARM11MP__)
	// MPCore has physically indexed D cache, so no colour problems
	#else
	// Assume other ARM cores have virtually indexed D cache with broken alias avoidence hardware...
	#define __CPU_D_CACHE_HAS_COLOUR
	#endif


#endif


#ifdef __FIQ_RESERVED_FOR_SECURE_STATE__
#define __FIQ_IS_UNCONTROLLED__
#endif

#if defined(__CPU_MEMORY_TYPE_REMAPPING) || defined(__MEMMODEL_FLEXIBLE__)
#define	__MMU_USE_SYMMETRIC_ACCESS_PERMISSIONS
#endif


#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_353494_FIXED) && defined(__MMU_USE_SYMMETRIC_ACCESS_PERMISSIONS)
#define ERRATUM_353494_MODE_CHANGE(cc,r) FLUSH_BTB(cc,r)
#else
#define ERRATUM_353494_MODE_CHANGE(cc,r)
#endif
	
#ifdef __CPU_HAS_MMU
#define __CPU_ARM_USE_DOMAINS
#endif

#if defined(__ARM_L210_CACHE__) || defined(__ARM_L220_CACHE__)|| defined(__ARM_PL310_CACHE__)
/**
Indicates the presense of external cache controller.
@internalTechnology
*/
#define __HAS_EXTERNAL_CACHE__
#endif

#ifndef __CPU_HAS_MMU
#define	CPWAIT(cc,r) /**< @internalTechnology */
#endif

#include <arm_vfp.h>

// CP15 definitions
#if defined(__CPU_ARM710T__) || defined(__CPU_ARM720T__)
#define FLUSH_DCACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_IDCACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_DTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_ITLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_IDTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");		/**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c7, 1 ");	/**< @internalTechnology */
#define FLUSH_ITLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c7, 1 ");	/**< @internalTechnology */
#define FLUSH_IDTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c7, 1 ");	/**< @internalTechnology */
#define DRAIN_WRITE_BUFFER(cc,r,rd) // this seems dodgy on Windermere and it works without it
#define	CPWAIT(cc,r) /**< @internalTechnology */

#elif defined(__CPU_SA1__)
#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");   /**< @internalTechnology */
#define PURGE_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 ");   /**< @internalTechnology */
#define CLEAN_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 ");  /**< @internalTechnology */
#define FLUSH_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 "); asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 ");/**< @internalTechnology */
#define FLUSH_DTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c6, 0 ");   /**< @internalTechnology */
#define FLUSH_ITLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c5, 0 ");   /**< @internalTechnology */
#define FLUSH_IDTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");   /**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c6, 1 ");/**< @internalTechnology */
#define DRAIN_WRITE_BUFFER(cc,r,rd)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 4 ");
#define	CPWAIT(cc,r) /**< @internalTechnology */

#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");	/**< @internalTechnology */
#define FLUSH_ICACHE_LINE(cc,r,tmp)	asm("mcr"#cc" p15, 0, "#r", c7, c5, 1 ");	/**< @internalTechnology */
#define PURGE_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 ");	/**< @internalTechnology */
#define CLEAN_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 ");	/**< @internalTechnology */
#define CLEAN_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 2 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c14, 1 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c14, 2 ");	/**< @internalTechnology */
#define FLUSH_DTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c6, 0 ");	/**< @internalTechnology */
#define FLUSH_ITLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c5, 0 ");	/**< @internalTechnology */
#define FLUSH_IDTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");	/**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c6, 1 ");/**< @internalTechnology */
#define FLUSH_ITLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c5, 1 ");/**< @internalTechnology */
#define DRAIN_WRITE_BUFFER(cc,r,rd)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 4 ");
#define	CPWAIT(cc,r) /**< @internalTechnology */
#define CACHE_MAINTENANCE_PDE_PTE_UPDATED(r)	DRAIN_WRITE_BUFFER(,r,r);

#elif defined(__CPU_XSCALE__)
//#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");
#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c5, 0; sub"#cc" pc, pc, #4 ");/**< @internalTechnology */ // A step hack
#define FLUSH_ICACHE_LINE(cc,r,tmp)	asm("mcr"#cc" p15, 0, "#r", c7, c5, 1 ");	/**< @internalTechnology */
#ifdef __CPU_XSCALE_MANZANO__
#define PURGE_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 ");	/**< @internalTechnology */
#define CLEAN_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 ");	/**< @internalTechnology */
#define CLEAN_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 2 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c14, 1 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c14, 2 ");	/**< @internalTechnology */
#else
#define PURGE_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 "); asm("nop "); /**< @internalTechnology */	// PXA250 ERRATUM 96
#define CLEAN_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 "); asm("nop ");/**< @internalTechnology */	// PXA250 ERRATUM 96
#define FLUSH_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 "); asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 "); asm("nop "); /**< @internalTechnology */ // PXA250 ERRATUM 96
#define ALLOC_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c2, 5 ");	/**< @internalTechnology */
#endif
#define FLUSH_DTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c6, 0 ");	/**< @internalTechnology */
#define FLUSH_ITLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c5, 0 ");	/**< @internalTechnology */
#define FLUSH_IDTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");	/**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c6, 1 "); asm("nop "); asm ("nop "); /**< @internalTechnology */	// PXA250 ERRATUM 21
#define FLUSH_ITLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c5, 1 "); /**< @internalTechnology */

#ifdef __CPU_XSCALE_MANZANO__
#define DRAIN_WRITE_BUFFER(cc,r,rd)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 4 ");
#else  //__CPU_XSCALE_MANZANO__
// PXA250 ERRATUM 14
#define DRAIN_WRITE_BUFFER(cc,r,rd)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 4 ");	\
									asm("ldr"#cc" "#rd", [pc] ");	\
									asm("add pc, pc, #0 ");	\
									asm(".word %a0" : : "i" ((TInt)&SuperPageAddress));	\
									asm("ldr"#cc" "#rd", ["#rd"] ");	\
									asm("ldr"#cc" "#rd", ["#rd", #%a0]" : : "i" _FOFF(TSuperPage,iUncachedAddress));	\
									asm("ldr"#cc" "#rd", ["#rd"] ");
#endif //else __CPU_XSCALE_MANZANO__
//#define FLUSH_BTB(cc,r)				asm("mcr"#cc" p15, 0, "#r", c7, c5, 6 ");
#define FLUSH_BTB(cc,r)				asm("mcr"#cc" p15, 0, "#r", c7, c5, 6; sub"#cc" pc, pc, #4 "); /**< @internalTechnology */ // A step hack
#define	CPWAIT(cc,r)				asm("mrc"#cc" p15, 0, "#r", c2, c0, 0; mov"#cc" "#r","#r"; sub"#cc" pc, pc, #4 "); /**< @internalTechnology */
#define GET_CAR(cc,r)				asm("mrc"#cc" p15, 0, "#r", c15, c1, 0 "); /**< @internalTechnology */
#define SET_CAR(cc,r)				asm("mcr"#cc" p15, 0, "#r", c15, c1, 0 "); /**< @internalTechnology */

#elif defined(__CPU_ARMV6) // end of elif __CPU_XSCALE

#if !defined(__CPU_ARM1136_ERRATUM_411920_FIXED) && (defined(__CPU_ARM1136__) || defined(__CPU_ARM1176__))
/** @internalTechnology */
#define FLUSH_ICACHE(cc,r,rt)		asm("mrs "#rt", cpsr"); 					\
									CPSIDAIF;									\
									asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");	\
									asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");	\
									asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");	\
									asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 ");	\
									asm("msr cpsr_c, "#rt); 					\
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop"); \
									asm("nop");
									
#else
#define FLUSH_ICACHE(cc,r)			asm("mcr"#cc" p15, 0, "#r", c7, c5, 0 "); /**< @internalTechnology */
#endif // else !(__CPU_ARM1136_ERRATUM_411920_FIXED) && (__CPU_ARM1136__ || __CPU_ARM1176__)
#if defined(__CPU_ARM1136_ERRATUM_371025_FIXED) || !defined(__CPU_ARM1136__)

#if !defined(__CPU_ARM1176_ERRATUM_720013_FIXED) && defined(__CPU_ARM1176__)
#define FLUSH_ICACHE_LINE(cc,r,tmp) asm("mcr"#cc" p15, 0, "#r", c7, c5, 1 ");       \
                                    asm("mcr"#cc" p15, 0, "#r", c7, c5, 1 "); /**< @internalTechnology */
#else
#define FLUSH_ICACHE_LINE(cc,r,tmp)	asm("mcr"#cc" p15, 0, "#r", c7, c5, 1 "); /**< @internalTechnology */
#endif // !defined(__CPU_ARM1176_ERRATUM_720013_FIXED) && defined(__CPU_ARM1176__)

#else // workaround for erratum 371025 of 1136...
/** @internalTechnology */
#define FLUSH_ICACHE_LINE(cc,r,tmp)	asm("orr"#cc" "#tmp", "#r", #0xC0000000 ");		\
									asm("bic"#cc" "#tmp", "#tmp", #1 ");			\
									asm("mcr"#cc" p15, 0, "#tmp", c7, c5, 2 ");		\
									asm("sub"#cc" "#tmp", "#tmp", #0x40000000 ");	\
									asm("mcr"#cc" p15, 0, "#tmp", c7, c5, 2 ");		\
									asm("sub"#cc" "#tmp", "#tmp", #0x40000000 ");	\
									asm("mcr"#cc" p15, 0, "#tmp", c7, c5, 2 ");		\
									asm("sub"#cc" "#tmp", "#tmp", #0x40000000 ");	\
									asm("mcr"#cc" p15, 0, "#tmp", c7, c5, 2 ");
#endif //else (__CPU_ARM1136_ERRATUM_371025_FIXED) || !(__CPU_ARM1136__)

#if !defined(__CPU_ARM1176_ERRATUM_720013_FIXED) && defined(__CPU_ARM1176__)
// It is commented out to ensure it is not used on 1176 cores with 720013 erratum
// #define FLUSH_ICACHE_INDEX(cc,r)    asm("mcr"#cc" p15, 0, "#r", c7, c5, 2 ");
#else
#define FLUSH_ICACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c5, 2 ");	/**< @internalTechnology */
#endif //!defined(__CPU_ARM1176_ERRATUM_720013_FIXED) && defined(__CPU_ARM1176__)
#define PURGE_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c6, 1 ");	/**< @internalTechnology */
#define PURGE_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c6, 2 ");	/**< @internalTechnology */
#define CLEAN_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c10, 1 ");	/**< @internalTechnology */

#define CLEAN_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 2 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_LINE(cc,r)		asm("mcr"#cc" p15, 0, "#r", c7, c14, 1 ");	/**< @internalTechnology */
#define FLUSH_DCACHE_INDEX(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c14, 2 ");	/**< @internalTechnology */
#define FLUSH_ITLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c5, 0 ");	/**< @internalTechnology */
#define FLUSH_DTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c6, 0 ");	/**< @internalTechnology */
#define FLUSH_IDTLB(cc,r)			asm("mcr"#cc" p15, 0, "#r", c8, c7, 0 ");	/**< @internalTechnology */


   // addr must include ASID
#if defined (__CPU_ARM11MP__)
#define FLUSH_ITLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c5, 3 ");	/**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c6, 3 ");	/**< @internalTechnology */
#else //(__CPU_ARM11MP__)
#define FLUSH_ITLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c5, 1 ");	/**< @internalTechnology */
#define FLUSH_DTLB_ENTRY(cc,addr)	asm("mcr"#cc" p15, 0, "#addr", c8, c6, 1 ");	/**< @internalTechnology */
#endif // else (__CPU_ARM11MP__)
#define FLUSH_ITLB_ASID(cc,asid)	asm("mcr"#cc" p15, 0, "#asid", c8, c5, 2 ");	/**< @internalTechnology */
#define FLUSH_DTLB_ASID(cc,asid)	asm("mcr"#cc" p15, 0, "#asid", c8, c6, 2 ");	/**< @internalTechnology */

#define DRAIN_WRITE_BUFFER(cc,r,rd)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 4 ");
#define DATA_MEMORY_BARRIER(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c10, 5 ");
#define	FLUSH_PREFETCH_BUFFER(cc,r)	asm("mcr"#cc" p15, 0, "#r", c7, c5, 4 ");	/**< @internalTechnology */
#define FLUSH_BTB(cc,r)				asm("mcr"#cc" p15, 0, "#r", c7, c5, 6 ");	/**< @internalTechnology */
#define	CPWAIT(cc,r)															/**< @internalTechnology */	// not sure about this
#define GET_CAR(cc,r)				asm("mrc"#cc" p15, 0, "#r", c1, c0, 2 ");	/**< @internalTechnology */
#define SET_CAR(cc,r)				asm("mcr"#cc" p15, 0, "#r", c1, c0, 2 ");	/**< @internalTechnology */

#if defined(__CPU_PAGE_TABLES_FULLY_CACHED)
	#define CACHE_MAINTENANCE_PDE_PTE_UPDATED(r) 	CLEAN_DCACHE_LINE(,r);\
													DRAIN_WRITE_BUFFER(,r,r);
#else
	#define CACHE_MAINTENANCE_PDE_PTE_UPDATED(r)	DRAIN_WRITE_BUFFER(,r,r);
#endif //end of __CPU_PAGE_TABLES_FULLY_CACHED

#elif defined(__CPU_ARMV7) // end of elif (__CPU_ARMV6)

// Define new-style cache/TLB maintenance instructions
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	ICIALLU						asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0,  r0,  c7,  c5, 0 ");			/**< @internalTechnology */
#else
#define	ICIALLU						asm("mcr p15, 0,  r0,  c7,  c5, 0 ");			/**< @internalTechnology */
#endif // end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)
#define	ICIMVAU(r)					asm("mcr p15, 0, "#r", c7,  c5, 1 ");			/**< @internalTechnology */
#define	BPIALL						asm("mcr p15, 0,  r0,  c7,  c5, 6 ");			/**< @internalTechnology */
#define	BPIMVA(r)					asm("mcr p15, 0, "#r", c7,  c5, 7 ");			/**< @internalTechnology */
#define	DCIMVAC(r)					asm("mcr p15, 0, "#r", c7,  c6, 1 ");			/**< @internalTechnology */
#define	DCISW(r)					asm("mcr p15, 0, "#r", c7,  c6, 2 ");			/**< @internalTechnology */
#define	DCCMVAC(r)					asm("mcr p15, 0, "#r", c7, c10, 1 ");			/**< @internalTechnology */
#define	DCCSW(r)					asm("mcr p15, 0, "#r", c7, c10, 2 ");			/**< @internalTechnology */
#define	DCCMVAU(r)					asm("mcr p15, 0, "#r", c7, c11, 1 ");			/**< @internalTechnology */
#define	DCCIMVAC(r)					asm("mcr p15, 0, "#r", c7, c14, 1 ");			/**< @internalTechnology */
#define	DCCISW(r)					asm("mcr p15, 0, "#r", c7, c14, 2 ");			/**< @internalTechnology */

#ifdef __SMP__
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	ICIALLUIS					asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0,  r0,  c7,  c1, 0 ");			/**< @internalTechnology */
#else
#define	ICIALLUIS					asm("mcr p15, 0,  r0,  c7,  c1, 0 ");			/**< @internalTechnology */
#endif //end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)
#define	BPIALLIS					asm("mcr p15, 0,  r0,  c7,  c1, 6 ");			/**< @internalTechnology */
#endif // end of __SMP__

#ifdef __CPU_SPLIT_TLB
#define	ITLBIALL					asm("mcr p15, 0,  r0,  c8,  c5, 0 ");			/**< @internalTechnology */
#define	ITLBIMVA(r)					asm("mcr p15, 0, "#r", c8,  c5, 1 ");			/**< @internalTechnology */
#define	ITLBIASID(r)				asm("mcr p15, 0, "#r", c8,  c5, 2 ");			/**< @internalTechnology */
#define	DTLBIALL					asm("mcr p15, 0,  r0,  c8,  c6, 0 ");			/**< @internalTechnology */
#define	DTLBIMVA(r)					asm("mcr p15, 0, "#r", c8,  c6, 1 ");			/**< @internalTechnology */
#define	DTLBIASID(r)				asm("mcr p15, 0, "#r", c8,  c6, 2 ");			/**< @internalTechnology */
#endif
#define	UTLBIALL					asm("mcr p15, 0,  r0,  c8,  c7, 0 ");			/**< @internalTechnology */
#define	UTLBIMVA(r)					asm("mcr p15, 0, "#r", c8,  c7, 1 ");			/**< @internalTechnology */
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	UTLBIASID(r)				asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0, "#r", c8,  c7, 2 ");			/**< @internalTechnology */
#else
#define	UTLBIASID(r)				asm("mcr p15, 0, "#r", c8,  c7, 2 ");			/**< @internalTechnology */
#endif // end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)

#ifdef __CPU_SUPPORTS_TLBIMVAA
#ifdef __CPU_SPLIT_TLB
#define	ITLBIMVAA(r)				asm("mcr p15, 0, "#r", c8,  c5, 3 ");			/**< @internalTechnology */
#define	DTLBIMVAA(r)				asm("mcr p15, 0, "#r", c8,  c6, 3 ");			/**< @internalTechnology */
#endif // end of __CPU_SPLIT_TLB
#define	UTLBIMVAA(r)				asm("mcr p15, 0, "#r", c8,  c7, 3 ");			/**< @internalTechnology */
#endif // end of __CPU_SUPPORTS_TLBIMVAA

#ifdef __SMP__
#ifdef __CPU_SPLIT_TLB
#define	ITLBIALLIS					asm("mcr p15, 0,  r0,  c8,  c3, 0 ");			/**< @internalTechnology */
#define	ITLBIMVAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 1 ");			/**< @internalTechnology */
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	ITLBIASIDIS(r)				asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#else
#define	ITLBIASIDIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#endif // end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)
#define	DTLBIALLIS					asm("mcr p15, 0,  r0,  c8,  c3, 0 ");			/**< @internalTechnology */
#define	DTLBIMVAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 1 ");			/**< @internalTechnology */
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	DTLBIASIDIS(r)				asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#else
#define	DTLBIASIDIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#endif // end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)
#endif // end of __CPU_SPLIT_TLB
#define	UTLBIALLIS					asm("mcr p15, 0,  r0,  c8,  c3, 0 ");			/**< @internalTechnology */
#define	UTLBIMVAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 1 ");			/**< @internalTechnology */
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
// ARM Cortex-A9 MPCore erratum 571618 workaround
// Execute memory barrier before interruptible CP15 operations
#define	UTLBIASIDIS(r)				asm("mcr p15, 0,  r0,  c7, c10, 5 "); \
									asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#else
#define	UTLBIASIDIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 2 ");			/**< @internalTechnology */
#endif // end of else (__CPU_CORTEX_A9__) && !(__CPU_ARM_A9_ERRATUM_571618_FIXED)

#ifdef __CPU_SUPPORTS_TLBIMVAA
#ifdef __CPU_SPLIT_TLB
#define	ITLBIMVAAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 3 ");			/**< @internalTechnology */
#define	DTLBIMVAAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 3 ");			/**< @internalTechnology */
#endif // end of __CPU_SPLIT_TLB
#define	UTLBIMVAAIS(r)				asm("mcr p15, 0, "#r", c8,  c3, 3 ");			/**< @internalTechnology */
#endif // end of __CPU_SUPPORTS_TLBIMVAA
#endif // end of __SMP__


#define DRAIN_WRITE_BUFFER(cc,r,rd)	__DATA_SYNC_BARRIER__(r)
#define DATA_MEMORY_BARRIER(cc,r)	__DATA_MEMORY_BARRIER__(r)
#define	FLUSH_PREFETCH_BUFFER(cc,r)	__INST_SYNC_BARRIER__(r)					/**< @internalTechnology */
//#define FLUSH_BTB(cc,r)				asm("mcr"#cc" p15, 0, "#r", c7, c5, 6 ");	/**< @internalTechnology */

#define	CPWAIT(cc,r)															/**< @internalTechnology */	// not sure about this
#define GET_CAR(cc,r)				asm("mrc"#cc" p15, 0, "#r", c1, c0, 2 ");	/**< @internalTechnology */
#define SET_CAR(cc,r)				asm("mcr"#cc" p15, 0, "#r", c1, c0, 2 "); \
									__INST_SYNC_BARRIER__(r)										/**< @internalTechnology */

#if !defined(__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) && defined(__CPU_PAGE_TABLES_FULLY_CACHED)
	#define CACHE_MAINTENANCE_PDE_PTE_UPDATED(r)	DCCMVAU(r); \
													__DATA_SYNC_BARRIER__(r);
#else
	#define CACHE_MAINTENANCE_PDE_PTE_UPDATED(r)	__DATA_SYNC_BARRIER__(r);
#endif // end of !(__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) && (__CPU_PAGE_TABLES_FULLY_CACHED)

#endif // end of of elif (__CPU_ARMV7)


/**
CPU_ARM1136_ERRATUM_317041: Bits [4:3] of Translation Table Base address registers (TTBR0, TTBR1)
do not read back correctly, but instead always return 0. 
@internalComponent
@released
*/
#if defined(__CPU_ARM1136__) && defined(__HAS_EXTERNAL_CACHE__) && !defined(__CPU_ARM1136_ERRATUM_317041_FIXED)
#define UPDATE_PW_CACHING_ATTRIBUTES(cc,r) asm("orr"#cc" "#r", "#r", #0x18")
#else
#define UPDATE_PW_CACHING_ATTRIBUTES(cc,r)
#endif  

// Instruction macros

#if defined(__CPU_ARMV6) || defined(__CPU_ARMV7)
#define SRSgen(P,U,W,mode)			asm(".word %a0" : : "i" ((TInt)(0xf84d0500|(P<<24)|(U<<23)|(W<<21)|(mode))));
#define SRSIA(mode)					SRSgen(0,1,0,mode)
#define SRSIAW(mode)				SRSgen(0,1,1,mode)
#define SRSDB(mode)					SRSgen(1,0,0,mode)
#define SRSDBW(mode)				SRSgen(1,0,1,mode)
#define SRSIB(mode)					SRSgen(1,1,0,mode)
#define SRSIBW(mode)				SRSgen(1,1,1,mode)
#define SRSDA(mode)					SRSgen(0,0,0,mode)
#define SRSDAW(mode)				SRSgen(0,0,1,mode)
#define RFEgen(P,U,W,base)			asm(".word %a0" : : "i" ((TInt)(0xf8100a00|(P<<24)|(U<<23)|(W<<21)|(base<<16))));
#define RFEIA(base)					RFEgen(0,1,0,base)
#define RFEIAW(base)				RFEgen(0,1,1,base)
#define RFEDB(base)					RFEgen(1,0,0,base)
#define RFEDBW(base)				RFEgen(1,0,1,base)
#define RFEIB(base)					RFEgen(1,1,0,base)
#define RFEIBW(base)				RFEgen(1,1,1,base)
#define RFEDA(base)					RFEgen(0,0,0,base)
#define RFEDAW(base)				RFEgen(0,0,1,base)
#elif defined(__CPU_XSCALE__) // end of (__CPU_ARMV6) || (__CPU_ARMV7)
#define MAR(acc,RdLo,RdHi)			MCRR(0,0,RdLo,RdHi,acc)
#define MARcc(cc,acc,RdLo,RdHi)		MCRR(cc,0,0,RdLo,RdHi,acc)
#define MRA(acc,RdLo,RdHi)			MRRC(0,0,RdLo,RdHi,acc)
#define MRAcc(cc,acc,RdLo,RdHi)		MRRC(cc,0,0,RdLo,RdHi,acc)
#define MIAgen(cc,acc,Rm,Rs,opc3)	asm(".word %a0" : : "i" ((TInt)0x0e200010|((cc)<<28)|((opc3)<<16)|((Rs)<<12)|((acc)<<5)|(Rm)));
#define MIA(acc,Rm,Rs)				MIAgen(CC_AL,acc,Rm,Rs,0)
#define MIAPH(acc,Rm,Rs)			MIAgen(CC_AL,acc,Rm,Rs,8)
#define MIABB(acc,Rm,Rs)			MIAgen(CC_AL,acc,Rm,Rs,12)
#define MIATB(acc,Rm,Rs)			MIAgen(CC_AL,acc,Rm,Rs,13)
#define MIABT(acc,Rm,Rs)			MIAgen(CC_AL,acc,Rm,Rs,14)
#define MIATT(acc,Rm,Rs)			MIAgen(CC_AL,acc,Rm,Rs,15)
#define MIAcc(cc,acc,Rm,Rs)			MIAgen(cc,acc,Rm,Rs,0)
#define MIAPHcc(cc,acc,Rm,Rs)		MIAgen(cc,acc,Rm,Rs,8)
#define MIABBcc(cc,acc,Rm,Rs)		MIAgen(cc,acc,Rm,Rs,12)
#define MIATBcc(cc,acc,Rm,Rs)		MIAgen(cc,acc,Rm,Rs,13)
#define MIABTcc(cc,acc,Rm,Rs)		MIAgen(cc,acc,Rm,Rs,14)
#define MIATTcc(cc,acc,Rm,Rs)		MIAgen(cc,acc,Rm,Rs,15)
#endif // end of elif (__CPU_XSCALE__)

#ifdef __CPU_ARM_HAS_CPS
#define CPSgen(im,mm,f,mode)		asm(".word %a0" : : "i" ((TInt)(0xf1000000|((im)<<18)|((mm)<<17)|((f)<<6)|(mode))))
#if __ARM_ASSEMBLER_ISA__ >= 6
#define CPSIDAIF					asm("cpsidaif ")
#define CPSIDAI						asm("cpsidai ")
#define CPSIDIF						asm("cpsidif ")
#define CPSIDI						asm("cpsidi ")
#define CPSIDF						asm("cpsidf ")
#define CPSIEAIF					asm("cpsieaif ")
#define CPSIEI						asm("cpsiei ")
#define CPSIEF						asm("cpsief ")
#define CPSIEIF						asm("cpsieif ")
#else
#define CPSIDAIF					CPSgen(3,0,7,0)		// disable all interrupts, leave mode alone
#define CPSIDAI						CPSgen(3,0,6,0)		// disable IRQs, leave mode alone
#define CPSIDIF						CPSgen(3,0,3,0)		// disable IRQs and FIQs, leave mode alone
#define CPSIDI						CPSgen(3,0,2,0)		// disable IRQs, leave mode alone
#define CPSIDF						CPSgen(3,0,1,0)		// disable FIQs, leave mode alone
#define CPSIEAIF					CPSgen(2,0,7,0)		// enable all interrupts, leave mode alone
#define CPSIEI						CPSgen(2,0,2,0)		// enable IRQs, leave mode alone
#define CPSIEF						CPSgen(2,0,1,0)		// enable FIQs, leave mode alone
#define CPSIEIF						CPSgen(2,0,3,0)		// enable IRQs and FIQs, leave mode alone
#endif // end of __ARM_ASSEMBLER_ISA__ >= 6
#define CPSIDAIFM(mode)				CPSgen(3,1,7,mode)	// disable all interrupts and change mode
#define CPSIDIFM(mode)				CPSgen(3,1,3,mode)	// disable all interrupts and change mode
#define CPSIDAIM(mode)				CPSgen(3,1,6,mode)	// disable IRQs and change mode
#define CPSIDIM(mode)				CPSgen(3,1,2,mode)	// disable IRQs and change mode
#define CPSIDFM(mode)				CPSgen(3,1,1,mode)	// disable FIQs and change mode
#define CPSIEAIFM(mode)				CPSgen(2,1,7,mode)	// enable all interrupts and change mode
#define CPSIEIM(mode)				CPSgen(2,1,2,mode)	// enable IRQs and change mode
#define CPSIEFM(mode)				CPSgen(2,1,1,mode)	// enable FIQs and change mode
#define CPSIEIFM(mode)				CPSgen(2,1,3,mode)	// enable IRQs and FIQs, and change mode
#define CPSCHM(mode)				CPSgen(0,1,0,mode)	// change mode, leave interrupt masks alone
#endif // end of __CPU_ARM_HAS_CPS

// Processor modes
#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1b
#define MODE_SYS 0x1f

// Macros for changing processor made and interrupt status
// 
// Two instructions are necessary prior to ARMv6, and these may be interleaved. 
//
// SET_MODE - sets mode and intrrupts status
// SET_INTS - sets interrupts status (requires knowing the current mode at compile time)
// INTS_ON  - enables interrupts (requires the cpsr value be available at run time)
// INTS_OFF - disables interrupts (requires the cpsr value be available at run time)

#ifdef __CPU_ARM_HAS_CPS

#define INTS_ALL_OFF	IDIF
#define INTS_IRQ_OFF	IDI
#define INTS_FIQ_ON		IEF
#define INTS_ALL_ON		IEIF

#define CONCAT2(a,b) 	a##b
#define CONCAT3(a,b,c)	a##b##c

#define SET_MODE_1(rd, newMode, newInts)
#define SET_MODE_2(rd, newMode, newInts)			CONCAT3(CPS, newInts, M)(newMode)

#define SET_INTS_1(rd, currentMode, newInts)		
#define SET_INTS_2(rd, currentMode, newInts)		CONCAT2(CPS, newInts)

#define INTS_ON_1(rd, rCpsr, newInts)				
#define INTS_ON_2(rd, rCpsr, newInts)				CONCAT2(CPS, newInts)

#define INTS_OFF_1(rd, rCpsr, newInts)				
#define INTS_OFF_2(rd, rCpsr, newInts)				CONCAT2(CPS, newInts)

#else	 //	__CPU_ARM_HAS_CPS

#define INTS_ALL_OFF	0xc0
#define INTS_IRQ_OFF	0x80
#define INTS_FIQ_ON		0x80
#define INTS_ALL_ON		0x00

#define SET_MODE_1(rd, newMode, newInts)			asm("mov "#rd", #%a0" : : "i" (newMode | newInts))
#define SET_MODE_2(rd, newMode, newInts)			asm("msr cpsr_c, "#rd)

#define SET_INTS_1(rd, currentMode, newInts)		SET_MODE_1(rd, currentMode, newInts)
#define SET_INTS_2(rd, currentMode, newInts)		SET_MODE_2(rd, currentMode, newInts)

#define INTS_ON_1(rd, rCpsr, newInts)				asm("bic "#rd", "#rCpsr", #%a0" : : "i" (newInts ^ 0xc0))
#define INTS_ON_2(rd, rCpsr, newInts)				asm("msr cpsr_c, "#rd)

#define INTS_OFF_1(rd, rCpsr, newInts)				asm("orr "#rd", "#rCpsr", #%a0" : : "i" (newInts))
#define INTS_OFF_2(rd, rCpsr, newInts)				asm("msr cpsr_c, "#rd)

#endif	 //	end of __CPU_ARM_HAS_CPS

#define SET_MODE(rd, newMode, newInts)				SET_MODE_1(rd, newMode, newInts); SET_MODE_2(rd, newMode, newInts)
#define SET_INTS(rd, currentMode, newInts) 			SET_INTS_1(rd, currentMode, newInts); SET_INTS_2(rd, currentMode, newInts)
#define INTS_ON(rd, rCpsr, newInts) 				INTS_ON_1(rd, rCpsr, newInts); INTS_ON_2(rd, rCpsr, newInts)
#define INTS_OFF(rd, rCpsr, newInts) 				INTS_OFF_1(rd, rCpsr, newInts); INTS_OFF_2(rd, rCpsr, newInts)

#define	__chill()

#ifdef 	__CPU_ARM_HAS_WFE_SEV

extern "C" void __arm_wfe();
extern "C" void __arm_sev();

#define	__snooze()	__arm_wfe()
#define	__holler()	__arm_sev()
#else
#define	__snooze()
#define	__holler()
#endif

#if defined(__SMP__) && !defined(__CPU_ARM_HAS_LDREX_STREX_V6K)
#error SMP not allowed without v6K
#endif
#if defined(__SMP__) && !defined(__CPU_HAS_CP15_THREAD_ID_REG)
#error SMP not allowed without thread ID registers
#endif

#define	__SRATIO_MACHINE_CODED__

#endif	//	end of __CPU_ARM

#if defined(__CPU_X86) && defined(__EPOC32__)
	#define __CPU_HAS_MMU
	#define __CPU_HAS_CACHE
	#define	__CPU_SUPPORTS_FAST_PROCESS_SWITCH

	// Page/directory tables are cached on X86.
	#define __CPU_PAGE_TABLES_FULLY_CACHED

#if defined(__VC32__)
	#define	X86_PAUSE	_asm rep nop
	#define	__chill()	do { _asm rep nop } while(0)
#elif defined(__GCC32__)
	#define	X86_PAUSE	__asm__ __volatile__("pause ");
	#define	__chill()	__asm__ __volatile__("pause ")
#else
#error Unknown x86 compiler
#endif

#define	__snooze()	__chill()
#define	__holler()

#if defined(__cplusplus)
extern "C" {
#endif
#if defined(__VC32__)
extern int _inp(unsigned short);								// input byte (compiler intrinsic)
extern unsigned short _inpw(unsigned short);					// input word (compiler intrinsic)
extern unsigned long _inpd(unsigned short);						// input dword (compiler intrinsic)
extern int _outp(unsigned short, int);							// output byte (compiler intrinsic)
extern unsigned short _outpw(unsigned short, unsigned short);	// output word (compiler intrinsic)
extern unsigned long _outpd(unsigned short, unsigned long);		// output dword (compiler intrinsic)

#pragma intrinsic(_inp, _inpw, _inpd, _outp, _outpw, _outpd)

#define	x86_in8(port)			((TUint8)_inp(port))
#define	x86_in16(port)			((TUint16)_inpw(port))
#define	x86_in32(port)			((TUint32)_inpd(port))
#define	x86_out8(port,data)		((void)_outp((port),(TUint8)(data)))
#define	x86_out16(port,data)	((void)_outpw((port),(TUint16)(data)))
#define	x86_out32(port,data)	((void)_outpd((port),(TUint32)(data)))

#elif defined(__GCC32__) // end of (__VC32__)
inline TUint8 _inpb(TUint16 port)
	{
	TUint8 ret;
	__asm__ __volatile__("in al, dx" : "=a" (ret) : "d" (port));
	return ret;
	}

inline TUint16 _inpw(TUint16 port)
	{
	TUint8 ret;
	__asm__ __volatile__("in ax, dx" : "=a" (ret) : "d" (port));
	return ret;
	}

inline TUint32 _inpd(TUint16 port)
	{
	TUint32 ret;
	__asm__ __volatile__("in eax, dx" : "=a" (ret) : "d" (port));
	return ret;
	}

inline void _outpb(TUint16 port, TUint8 data)
	{
	__asm__ __volatile__("out dx, al" : : "d" (port), "a" (data));
	}

inline void _outpw(TUint16 port, TUint16 data)
	{
	__asm__ __volatile__("out dx, ax" : : "d" (port), "a" (data));
	}

inline void _outpd(TUint16 port, TUint32 data)
	{
	__asm__ __volatile__("out dx, eax" : : "d" (port), "a" (data));
	}

#define	x86_in8(port)			(_inpb(port))
#define	x86_in16(port)			(_inpw(port))
#define	x86_in32(port)			(_inpd(port))
#define	x86_out8(port,data)		(_outpb((port),(TUint8)(data)))
#define	x86_out16(port,data)	(_outpw((port),(TUint16)(data)))
#define	x86_out32(port,data)	(_outpd((port),(TUint32)(data)))

#else // end of elif (__GCC32__)
#error Unknown x86 compiler
#endif
#if defined(__cplusplus)
}
#endif // end of (__VC32__) elif __GCC32__ else

#endif //__CPU_X86 && __EPOC32__


#undef	__USER_MEMORY_GUARDS_ENABLED__
#if	defined(_DEBUG) && !defined(__KERNEL_APIS_DISABLE_USER_MEMORY_GUARDS__)
#if defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)
#if defined(__CPU_ARM)
#define __USER_MEMORY_GUARDS_ENABLED__
#endif
#endif
#endif // end of (_DEBUG) && !(__KERNEL_APIS_DISABLE_USER_MEMORY_GUARDS__)

#ifndef __USER_MEMORY_GUARDS_ENABLED__

#define USER_MEMORY_GUARD_SAVE_WORDS			0
#define USER_MEMORY_DOMAIN						0

#define USER_MEMORY_GUARD_SAVE(save)
#define USER_MEMORY_GUARD_RESTORE(save,temp)
#define USER_MEMORY_GUARD_ON(cc,save,temp)
#define USER_MEMORY_GUARD_OFF(cc,save,temp)
#define USER_MEMORY_GUARD_ON_IF_MODE_USR(temp)
#define USER_MEMORY_GUARD_OFF_IF_MODE_USR(temp)
#define USER_MEMORY_GUARD_ASSERT_ON(temp)
#define USER_MEMORY_GUARD_ASSERT_OFF_IF_MODE_USR(psr)

#else // __USER_MEMORY_GUARDS_ENABLED__

#define USER_MEMORY_GUARD_SAVE_WORDS			2
#define USER_MEMORY_DOMAIN						15
#define	USER_MEMORY_DOMAIN_MASK					(3U << (2*USER_MEMORY_DOMAIN))
#define	USER_MEMORY_DOMAIN_CLIENT				(1U << (2*USER_MEMORY_DOMAIN))

// Save the DACR in the named register
#define USER_MEMORY_GUARD_SAVE(save)											\
	asm("mrc p15, 0, "#save", c3, c0, 0");			/* save<-DACR */

// Restore access to domain 15 (user pages) to the state previously saved
// In this case, 'save' may not be the same register as 'temp'
#define USER_MEMORY_GUARD_RESTORE(save,temp)									\
	asm("mrc p15, 0, "#temp", c3, c0, 0");			/* temp<-DACR */			\
	asm("bic "#temp", "#temp", #%a0" : : "i" USER_MEMORY_DOMAIN_MASK);			\
	asm("and "#save", "#save", #%a0" : : "i" USER_MEMORY_DOMAIN_MASK);			\
	asm("orr "#temp", "#temp", "#save );										\
	asm("mcr p15, 0, "#temp", c3, c0, 0");			/* DACR<-temp */			\
    __INST_SYNC_BARRIER__(temp)

// Disable access to domain 15 (user pages)
// 'save' may be the same register as 'temp', but in that case the use as
// a temporary takes precedence and the value left in 'save' is undefined
#define USER_MEMORY_GUARD_ON(cc,save,temp)										\
	asm("mrc"#cc" p15, 0, "#save", c3, c0, 0");		/* save<-DACR */			\
	asm("bic"#cc" "#temp", "#save", #%a0" : : "i" USER_MEMORY_DOMAIN_MASK);	\
	asm("mcr"#cc" p15, 0, "#temp", c3, c0, 0");		/* DACR<-temp */			\
    __INST_SYNC_BARRIER__(temp)

// Enable access to domain 15 (user pages) as a client
// 'save' may be the same register as 'temp', but in that case the use as
// a temporary takes precedence and the value left in 'save' is undefined
#define USER_MEMORY_GUARD_OFF(cc,save,temp)										\
	asm("mrc"#cc" p15, 0, "#save", c3, c0, 0");		/* save<-DACR */			\
	asm("orr"#cc" "#temp", "#save", #%a0" : : "i" USER_MEMORY_DOMAIN_CLIENT);	\
	asm("mcr"#cc" p15, 0, "#temp", c3, c0, 0");		/* DACR<-temp */			\
    __INST_SYNC_BARRIER__(temp)

// Disable access to domain 15 (user pages) if SPSR indicates mode_usr
// The specified 'temp' register is left with an undefined value
#define USER_MEMORY_GUARD_ON_IF_MODE_USR(temp)									\
	asm("mrs "#temp", spsr");													\
	asm("tst "#temp", #0x0f");													\
	USER_MEMORY_GUARD_ON(eq,temp,temp)

// Enable access to domain 15 (user pages) if SPSR indicates mode_usr
// The specified 'temp' register is left with an undefined value
#define USER_MEMORY_GUARD_OFF_IF_MODE_USR(temp)									\
	asm("mrs "#temp", spsr");													\
	asm("tst "#temp", #0x0f");													\
	USER_MEMORY_GUARD_OFF(eq,temp,temp)

// Assert that access to domain 15 (user pages) is disabled
#define USER_MEMORY_GUARD_ASSERT_ON(temp)										\
	asm("mrc p15, 0, "#temp", c3, c0, 0");		/* temp<-DACR				*/	\
	asm("tst "#temp", #%a0" : : "i" USER_MEMORY_DOMAIN_MASK);					\
	asm("cdpne p15, 0, c0, c0, c0, 0");			/* fault if nonzero			*/

// Assert that access to domain 15 (user pages) is enabled if the value
// in 'psr' says we came from/are going back to user mode
#define USER_MEMORY_GUARD_ASSERT_OFF_IF_MODE_USR(psr)							\
	asm("tst "#psr", #0x0f");					/* check for mode_usr		*/	\
	asm("mrceq p15, 0, "#psr", c3, c0, 0");		/* psr<-DACR				*/	\
	asm("tsteq "#psr", #%a0" : : "i" USER_MEMORY_DOMAIN_MASK);					\
	asm("cdpeq p15, 0, c0, c0, c0, 0");			/* fault if no access		*/

#endif // end of else __USER_MEMORY_GUARDS_ENABLED__

#endif // __NK_CPU_H__
