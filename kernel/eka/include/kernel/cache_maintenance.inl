// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\include\kernel\cache_maintenance.inl
// 
// Contains Kernel's internal API of cache maintenance 

/**
 @file
 @internalComponent
*/

#ifndef __CACHE_MAINTENANCE_INL__
#define __CACHE_MAINTENANCE_INL__

#if defined(__CPU_HAS_CACHE)

#include <e32err.h>
#include "cache_maintenance.h"
#include <platform.h>
#include <nk_cpu.h>
#include <mmboot.h>

#if defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)

#if defined(GNUC) && !defined(__MARM_ARM4__)
#define  __VOLATILE__    volatile
#else
#define __VOLATILE__
#endif


#if defined(__CPU_ARMV6)
	#define CLEAN_PTE_REGION(aAddr, aSize) InternalCache::Clean_DCache_Region(aAddr, aSize)
	#define PTE_CLEAN_REG "c10"
#else //(__CPU_ARMV6)
	// On ARMv7 onwards, page tables have to be cleaned to the Point of Unification
	// to be visible to page-table walk.
#if defined (__FLUSH_PT_INTO_RAM__)
    // However, if page table entries have to be visible in main memory, we have to clean to the
    // Point of Coherency.
    #define CLEAN_PTE_REGION(aAddr, aSize) InternalCache::Clean_DCache_Region(aAddr, aSize)
    #define PTE_CLEAN_REG "c10"
#else
    // Ordinary ARMv7 case, where page table change is pushed down to the Point of Unification.
    #define CLEAN_PTE_REGION(aAddr, aSize) InternalCache::Clean_PoU_DCache_Region(aAddr, aSize)
    #define PTE_CLEAN_REG "c11"
#endif // else defined (__FLUSH_PT_INTO_RAM__)
#endif // else (__CPU_ARMV6)

FORCE_INLINE void CacheMaintenance::MultiplePtesUpdated(TLinAddr aPte, TUint aSize)
	{
	#if defined(__CPU_X86)
		//Empty
	#elif defined(__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !defined(__CPU_PAGE_TABLES_FULLY_CACHED)
		(void)aPte;
		(void)aSize;
		__e32_io_completion_barrier();
	#else	// (__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !(__CPU_PAGE_TABLES_FULLY_CACHED)
		CLEAN_PTE_REGION((TLinAddr)aPte, aSize); // clean cache lines & drain write buffer
	#endif	// else (__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !(__CPU_PAGE_TABLES_FULLY_CACHED)
#if defined (__FLUSH_PT_INTO_RAM__)
	//See SinglePteUpdated for details.
	ExternalCache::Clean(aPte, aSize);
#endif // (__FLUSH_PT_INTO_RAM__)
	}

FORCE_INLINE void CacheMaintenance::PdesInitialised(TLinAddr aPde, TUint aSize)
	{
	CacheMaintenance::MultiplePtesUpdated(aPde, aSize);
	}

FORCE_INLINE void CacheMaintenance::SinglePteUpdated(TLinAddr aPte)
	{
	#if defined(__CPU_X86)
		//Empty
	#elif defined(__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !defined(__CPU_PAGE_TABLES_FULLY_CACHED)
		(void)aPte;
		__e32_io_completion_barrier();
	#else // (__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !(__CPU_PAGE_TABLES_FULLY_CACHED)
		#ifdef __GNUC__
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, " PTE_CLEAN_REG ", 1 " : : "r"(aPte));	// clean cache line
		#elif defined(__ARMCC__)
			asm("mcr p15, 0, aPte, c7, " PTE_CLEAN_REG ", 1 ");		// clean cache line
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
		__e32_io_completion_barrier();
	#endif //else (__CPU_SUPPORTS_PAGE_TABLE_WALK_TO_L1_CACHE) || !(__CPU_PAGE_TABLES_FULLY_CACHED)

#if defined (__FLUSH_PT_INTO_RAM__)
	// This will ensure that the page tables/dirs are updated in main memory.
	// This is only necessary when another part of the system accesses the page
	// tables separately (e.g. another processor using the same page tables out
	// of main memory), and is not necessary on standard platforms.
	// Either __ARM_L210_CACHE__ or __ARM_L220_CACHE__ must also be defined
	ExternalCache::Clean(aPte, 4);
#endif
	}

FORCE_INLINE void CacheMaintenance::SinglePdeUpdated(TLinAddr aPde)
	{
	CacheMaintenance::SinglePteUpdated(aPde);
	}

#endif //#if defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)

#endif //defined(__CPU_HAS_CACHE)

#endif //#ifndef __CACHE_MAINTENANCE_INL__
