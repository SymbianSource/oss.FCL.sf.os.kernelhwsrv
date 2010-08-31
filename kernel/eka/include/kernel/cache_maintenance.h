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
// eka\include\kernel\cache_maintenance.h
// 
// Contains Kernel's internal API for cache maintenance 

/**
 @file
 @internalComponent
*/

#ifndef __CACHE_MAINTENANCE_H__
#define __CACHE_MAINTENANCE_H__

#include <e32err.h>
#include <nk_cpu.h>

#if defined(__CPU_HAS_CACHE)
#include <platform.h>
#include <mmboot.h>

/*
 * Specifies the number of different cache types/levels in InternalCache class.
 */
#if defined(__CPU_ARMV7)
const TInt KNumCacheInfos=3; 	// ICache, DCache_PoC & DCache_PoU
#else // defined(__CPU_ARMV7)
const TInt KNumCacheInfos=2; 	// ICache & DCache
#endif//else defined(__CPU_ARMV7)

const TInt KCacheInfoI=0;		// InternalCache info for ICache. On ARMv7, this applies to the point-of-unification.
const TInt KCacheInfoD=1;		// InternalCache info for DCache. On ARMv7, this applies to the point-of-coherency.
const TInt KCacheInfoD_PoU=2;	// InternalCache info for ARMv7 DCache for the point-of-unification.

#if !defined(__CPU_MEMORY_TYPE_REMAPPING) && defined(__CPU_ARM)
// These constants must be dthe same as DefaultPRRR & DefaultNMRR in bootcpu.inc
const TUint32 KDefaultPrimaryRegionRemapRegister = 0x000a00a4;
const TUint32 KDefaultNormalMemoryRemapRegister  = 0x00400040;
#endif

/* 
 * Cache info of particular cache type or level.
 */
struct SCacheInfo
	{
	TUint32 iSize;					// Total size in cache lines
	TUint16 iAssoc;					// Associativity
	TUint16 iLineLength;			// Line length in bytes. For multilevel cache, this is minimum length.
	TUint32 iInvalidateThreshold;	// Size threshold for line-by-line Invalidate (in cache lines)
	TUint32 iCleanThreshold;		// Size threshold for line-by-line Clean (in cache lines)
	TUint32 iCleanAndInvalidateThreshold;// Size threshold for line-by-line CleanAndInvalidate (in cache lines)
#if !defined(__CPU_ARMV7)
	TUint iCleanAndInvalidatePtr;	// CleanAndInvalidate pointer
	TUint iCleanAndInvalidateMask;	// Mask to wrap CleanAndInvalidate pointer
#endif
	TUint8 iLineLenLog2;			// log2(iLineLength)
	TUint8 iPreemptBlock;			// Number of cache lines to clean before checking for system lock contention
	inline TUint InvalidateThresholdBytes()
		{ return iInvalidateThreshold<<iLineLenLog2; }
	inline TUint CleanThresholdBytes()
		{ return iCleanThreshold<<iLineLenLog2; }
	inline TUint CleanAndInvalidateThresholdBytes()
		{ return iCleanAndInvalidateThreshold<<iLineLenLog2; }
	inline TUint InvalidateThresholdPages()
		{ return iInvalidateThreshold >> (KPageShift-iLineLenLog2);}
	};

/*
 * A set of static utility functions for internal (MMU controlled) cache memory.
 * Unless  otherwise specified, the following is assumed:
 *  - All DCache maintenance primitives apply to the Point of Coherency.
 *  - All ICache maintenance primitives apply to the Point of Unification.
 *  - Multilevel caches are maintained either:
 * 			- starting from the level closest to CPU, or
 * 			- all level are maintained simultaneously. 
 */
class InternalCache
	{
	friend class CacheMaintenance;
	friend class Cache;

public:	
/*
 * Initializes internal data structure for different cache types/levels.
 * Must be called during Init1 boot phase.
 * @pre 	All internal cache memory is already configured and switched on in bootstrap.
 * 			Single thread environment is assumed (e.g. during boot time).
 */
	static void Init1();
/*
 * @return MMU's cache type register.
 */	
	static TUint32 TypeRegister();
	
/*
 * @return	Internal and external cache attributes (orred TMappingAttributes enums)
 * 			that match aType memory type.
 */
	static TUint32 TypeToCachingAttributes(TMemoryType aType);

#if defined(__CPU_ARMV7)

/*
 * @return MMU's cache level ID register
 */
	static TUint32 LevelIDRegister();

/*
 * @return MMU's cache size ID  register for given cache type & cache level.
 * @arg aType 0 for data or unified cache, 1 for instruction cache.
 * @arg aLevel 0-7 where 0 indicates the closest level to CPU.
 */
	static TUint32 SizeIdRegister(TUint32 aType, TUint32 aLevel);

#endif //defined(__CPU_ARMV7)

#if !defined(__MEMMODEL_MOVING__)
// Moving memory model is aware of cache implementation on ARMv5 and does some direct calls to
// InternalCache class.	
private:	
#endif

/*
 * Invalidates a memory region from cache(s) on all the cores. If ICache is specified in aMask,
 * it also drains branch predictors and instruction pipelines(ISB barrier).
 * If aSize is bigger than invalidate-threshold of any specified cache, it may clean
 * and invalidate entire cache. 
 * @arg See Clean method for details.
 * 
 * @note CacheMaintanance assumes that on H/W with NOT (defined(__CPU_ARMV7) && defined(__SMP__),
 * this will clean and invalidate entire DCache if invalidate threshold is reached.
 */
	static void Invalidate(TUint aMask, TLinAddr aBase, TUint aSize);

/*
 * Drains the buffers in cache memory. On ARMv6 onwards, this operation is known as DSB (Data 
 * Synchronisation Barrier).
 * On SMP, only the buffers of the running core are drained. 
 */
	static void DrainBuffers();

/*
 * Holds thresholds, cache line size,... for different types/levels of cache.
 */ 
	static SCacheInfo Info[KNumCacheInfos];

private:
#if defined(__BROADCAST_CACHE_MAINTENANCE__)

//	__BROADCAST_CACHE_MAINTENANCE__ is specified when cache maintenance has to be broadcasted
//	across all cores on SMP platforms by software.
//	This is only defined on arm11 SMP HW as it doesn't have HW broadcasting any cache maintenance.
//	CORTEX_A9 SMP has H/W broadcasting of line-by-line maintenance, while index/way is not used.

/*
 * Cleans a memory region from cache(s) & drain write buffers (DSB barrier)
 * on a core that executes the call.
 * @arg See Clean method for other details.
 */ 	
	static void LocalClean(TUint aMask, TLinAddr aBase, TUint aSize);

/*
 * Invalidates a memory region from cache(s) on a core that executes the call.
 * @arg See Invalidate method for other details.
 */ 	
	static void LocalInvalidate(TUint aMask, TLinAddr aBase, TUint aSize);

/*
 * Cleans and invalidates a memory region from cache(s) & drain write buffers (DSB barrier)
 * on a core that executes the call.
 * @arg See CleanAndInvalidate method for details.
 */	
	static void LocalCleanAndInvalidate(TUint aMask, TLinAddr aBase, TUint aSize);

#endif //defined(__BROADCAST_CACHE_MAINTENANCE__)

/*
 * Cleans a memory region from cache(s) & drain write buffers (DSB barrier) on all the cores.
 * If aSize is bigger than clean threshold of any specified cache, it may clean entire cache.
 * @arg aMask	Specifies which caches to clean by orring KCacheSelectI (for ICache) and
 * 				KCacheSelectD (for DCache or unified cache).
 * @arg aBase	Linear address of the start of the region to clean.
 * @arg aSize	Size of the region in bytes.
 */ 	
	static void Clean(TUint aMask, TLinAddr aBase, TUint aSize);

#if defined(__CPU_ARMV7)
/*
 * Cleans a memory region from DCache to the Point of Unification & drains write buffers(DSB barrier)
 * on all the cores. If aSize is bigger than clean-to-the-point-to-unification threshold, it
 * may clean the entire cache(s) to the point-of-unification.
 * @arg See Clean method for details.
 */
	static void CleanPoU(TLinAddr aBase, TUint aSize);
#endif	// defined(__CPU_ARMV7)

/*
 * Invalidates a memory region from data and unified cache(s) on all the cores. It either:
 * 		- starts from the level which is the furthest from CPU, or
 * 		- invalidates all levels at once.
 * If aSize is bigger than invalidate-threshold of any specified cache, it may clean
 * and invalidate the entire cache.
 * @arg aBase	Linear address of the start of the region to clean.
 * @arg aSize	Size of the region in bytes.
 */
	static void Invalidate_DCache_Reverse(TLinAddr aBase, TUint aSize);
	
/*
 * Cleans and invalidates a memory region from cache(s) & drain write buffers (DSB barrier) on
 * all the cores. 
 * If ICache is specified in aMask, it drains branch predictor and instruction pipeline(ISB barrier).
 * If aSize is bigger than CleanAndInvalidate threshold of any specified cache, it may clean and
 * invalidate the entire cache(s).
 * @arg See Clean method for details.
 */ 	
	static void CleanAndInvalidate(TUint aMask, TLinAddr aBase, TUint aSize);
	
/*
 * Invalidates a region of memory in instruction cache and drains branch predictor and
 * instruction pipeline(ISB barrier).
 * On SMP arm11mpcore, only the running core is maintained.
 * On SMP ArmV7 onwards, this maintains all the cores. However, ISB barrier applies only
 * to the running core. The caller must ensure ISB is broadcasted by other maens.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void Invalidate_ICache_Region(TLinAddr aBase, TUint aSize);

/*
 * Invalidates entire content of instruction cache(s) and drains branch predictor and
 * instruction pipeline(ISB barrier).
 * On SMP arm11mpcore, only the running core is maintained. 
 * On SMP ArmV7 onwards, this maintains all the cores. However, ISB barrier applies only
 * to the running core. The caller must ensure ISB is broadcasted by other maens.
 */ 
	static void Invalidate_ICache_All();

/*
 * Invalidates a region of memory in data and unified cache(s).
 * On SMP arm11mpcore, only the running core is maintained. 
 * On SMP ArmV7 onwards, this maintains all the cores.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void Invalidate_DCache_Region(TLinAddr aBase, TUint aSize);

/*
 * Cleans a region of memory in data and unified cache(s) and drains write buffers (DSB barrier).
 * On SMP arm11mpcore, only the running core is maintained. 
 * On SMP ArmV7 onwards, this maintains all the cores.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	The size of the region in bytes.
 */
	static void Clean_DCache_Region(TLinAddr aBase, TUint aSize);

#if defined(__CPU_ARMV7)
/*
 * Cleans a region of memory in data and unified cache(s) to the point-of-unification and drains
 * write buffers (DSB barrier).
 * On SMP, it maintains all the cores.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void Clean_PoU_DCache_Region(TLinAddr aBase, TUint aSize);
#endif  //defined(__CPU_ARMV7)
	
/*
 * Cleans the entire content of data and unified caches and drains write buffers (DSB barrier).
 * On SMP, only the running core is maintained. 
 */
	static void Clean_DCache_All();

#if defined(__CPU_ARMV7)
/*
 * Cleans the entire content of data and unified cache(s) to the point-of-unification and drains
 * write buffers (DSB barrier).
 * On SMP, only the running core is maintained. 
 */
	static void Clean_PoU_DCache_All();
#endif //defined(__CPU_ARMV7)
	
/*
 * Cleans and invalidates a region of memory in data and unified cache(s) and drains
 * write buffers (DSB barrier).
 * On SMP arm11mpcore, only the running core is maintained. 
 * On SMP ArmV7 onwards, this maintains all the cores.
 */
	static void CleanAndInvalidate_DCache_Region(TLinAddr aBase, TUint aSize);

/*
 * Cleans and invalidates the entire content of data and unified cache(s) and drains
 * write buffers (DSB barrier)..
 * On SMP, only the running core is maintained.
 * This methos is called during reboot or power down sequence and therefore is not allowed
 * to do Kernel calls that may hold spin lock - such as Kern::Print or precondition checkings.
 */
	static void CleanAndInvalidate_DCache_All();

/*
 * Synchronises the ICache and DCache for instruction execution.
 * Also invalidates the branch predictor array, this is architecture dependant:
 *   ARM7: Invalidates aAddr and aAddr+2 (covering possible THUMB instructions)
 *   ARM6: Invalidates the whole Branch Predictor Array
 *
 * On SMP, only the running core is maintained.
 * 
 * @arg aAddr 32bit aligned virtual address that belongs to the cache line.
 * 
 * NOTE: On SMP this is guaranteed NOT to broadcast to other cores.
 * NOTE: It assumes the same line size for ICache and DCache
 */
	static void IMB_CacheLine(TLinAddr aAddr);

private:	
#if !defined(__CPU_ARMV7)
/* 
 * A primitive that parses the content of cache type MMU register.
 */
	static void ParseCacheSizeInfo(TUint32 aValue, SCacheInfo& aInfo);
#endif	
	
/* 
 * @return The content of Primary Region Remap Register.
 */
	static TUint32 PrimaryRegionRemapRegister();

/*
 * @return The content of Normal Memory Remap Register.
 */
	static TUint32 NormalMemoryRemapRegister();
	
#if defined(__CPU_ARMV7)
	static TInt DmaBufferAlignementLog2;	// Holds the alignement requirement for DMA buffers. 
#endif

#if !defined(__CPU_MEMORY_TYPE_REMAPPING) && defined(__CPU_ARM)
	/**
	 * On platforms with no memory type remapping, we have to simulate TMemoryType values 4-7
	 */
	static TUint32 iPrimaryRegionRemapRegister;
    static TUint32 iNormalMemoryRemapRegister;
#endif
	};

#ifdef __HAS_EXTERNAL_CACHE__
//ARM External Cache register offsets
const TUint ARML2C_AuxiliaryControl = 0x104;
	const TUint ARML2C_WaySize_Mask = 0xe0000;
	const TUint ARML2C_WaySize_Shift = 17;
#if defined (__ARM_PL310_CACHE__)
	const TUint ARML2C_Assoc_Mask = 0x10000;
#else
	const TUint ARML2C_Assoc_Mask = 0x1e000;
	const TUint ARML2C_Assoc_Shift = 13;
#endif
	
const TUint ARML2C_CacheSync = 0x730;

const TUint ARML2C_InvalidateLineByPA = 0x770;
const TUint ARML2C_CleanLineByPA = 0x7b0;
const TUint ARML2C_CleanInvalidateLineByPA = 0x7f0;

const TUint ARML2C_CleanByIndexWay = 0x7b8;
const TUint ARML2C_CleanInvalidateByIndexWay = 0x7f8;

const TUint ARML2C_CleanByWay = 0x7bc;
const TUint ARML2C_InvalidateByWay = 0x77c;
const TUint ARML2C_CleanInvalidateByWay = 0x7fc;
#if defined (__ARM_PL310_CACHE__)
    const TUint ARML2C_WayShift = 28;
#else
    const TUint ARML2C_WayShift = 29;
#endif
    const TUint ARML2C_IndexShift = 5;

/*
 * A set of static utility functions for external cache memory.
 * The following external cache controllers are supported:
 * 	- L210
 *  - L220
 *  - PL310
 */
class ExternalCache
	{
	friend class CacheMaintenance;
	friend class Cache;

public:
/*
 * Initializes internal cache infos. Must be called during Init1 boot phase.
 * @pre 	External cache controller is already configured and started in bootstrap.
 * 			Single thread environment is assumed (e.g. during boot time).
 */
	static void Init1();
private:
/*
 * Cleans a region of memory in cache and drains its buffers.
 * If aSize is bigger than clean threshold, it may clean the entire cache.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void Clean(TLinAddr aBase, TUint aSize);
/*
 * Invalidates a region of memory in cache.
 * If aSize is bigger than invalidate threshold, it may clean and invalidate the entire cache.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void Invalidate(TLinAddr aBase, TUint aSize);

/*	
 * Cleans and invalidates a region of memory in cache and drains its buffers.
 * If aSize is bigger than clean and invalidate threshold, it may clean and invalidate the entire cache.
 * @arg aBase	Linear address of the start of the region.
 * @arg aSize	Size of the region in bytes.
 */
	static void CleanAndInvalidate(TLinAddr aBase, TUint aSize);

/*
 * Cleans a region of contiguous physical memory in cache and drains its buffers.
 * It doesn't check clean threshold.
 * @arg aBase	Physical address of the start of the region.
 * @arg aAddr	Size of the region in bytes.
 */
	static void CleanPhysicalMemory(TPhysAddr aAddr, TUint aSize);

/*	
 * Invalidates a region of contiguous physical memory in cache.
 * It doesn't check invalidate threshold.
 * @arg aBase	Physical address of the start of the region.
 * @arg aAddr	Size of the region in bytes.
 */
	static void InvalidatePhysicalMemory(TPhysAddr aAddr, TUint aSize);
	
/*
 * Clean and invalidates a region of contiguous physical memory in cache and drains its buffers.
 * It doesn't check clean and invalidate threshold.
 * @arg aBase	Physical address of the start of the region.
 * @arg aAddr	Size of the region in bytes.
 */
	static void CleanAndInvalidatePhysicalMemory(TPhysAddr aAddr, TUint aSize);

/*
 * Ensures the entire content of cache is copied back to main memory.
 * On some platforms, it may not invalidate cache content.
 * @pre Interupts are disabled.
 */
	static void AtomicSync();

private:

/*
 * Generic function that cleans and/or invalidates memory region.
 * @arg aBase		Linear address of the start of the region.
 * @arg aSize		Size of the region in bytes.
 * @param aCtrlReg	The address of the register to access in order to trigger the maintenance
 * 					operation. The following values are valid:
 * 						- to invalidate the region:
 *                              ExternalControllerBaseAddress+ARML2C_InvalidateLineByPA
 *						- to clean the region:
 *						-       ExternalControllerBaseAddress+ARML2C_CleanLineByPA
 * 						- to clean and invalidate the region:
 *                              ExternalControllerBaseAddress+ARML2C_CleanInvalidateLineByPA
 */
	static void Maintain_Region(TLinAddr aBase, TUint aSize, TInt* aCtrlReg);

/*
 * Generic function that cleans or clean&invalidates the entire content of cache.
 * @param aCtrlReg	The address of the register to access in order to trigger the maintenance
 * 					operation. The following values are valid:
 *						- to clean:
 *						        ExternalControllerBaseAddress+ARML2C_CleanByIndexWay
 * 						- to clean and invalidate:
 *                              ExternalControllerBaseAddress+ARML2C_CleanInvalidateByWay
 */
	static void Maintain_All(TInt* aCtrlReg);

/*
 * Drains all the buffers in the cache controller.
 */
	static void DrainBuffers();
	
#if defined(__ARM_PL310_CACHE__)
	static TInt Lock();
	static void FlashLock(TInt aIrq);
	static void Unlock(TInt iIrq);
	const static  TUint KMaxCacheLinesPerSpinLock = 10;//Max number of cache lines to maintain while spin lock is held.
	static TSpinLock iLock;
#endif //defined(__ARM_PL310_CACHE__)
	
	static TLinAddr Base;	//Base address of the external cache controller.
	static SCacheInfo Info;
	};
#endif //#ifdef __HAS_EXTERNAL_CACHE__


/*
 * Collector class of cache memory maintenance primitives.
 * They do not maintain TLBs, branch predictor nor CPU pipeline unless specified otherwise.
 * No preconditions are assumed unless specified otherwise.
 * @internalComponent
 */
class CacheMaintenance
	{
public:
/*
 * Initializes internal structures of cache configuration. Must be called during Init1 boot phase.
 *
 * @pre Single thread environment is assumed (e.g. during boot time).
 */
	static void Init1();

/*
 * Maintains cache(s) for a single page of physical memory that is about to change
 * its mapping/caching attributes. Note that the content of the memory may be lost.
 * 
 * The client may call this method either:
 * 	- during the process of invalidating old mapping(s), or
 *  - as background maintenance of free physical memory, or
 * 	- when the physical memory is about to be reused again.
 * 
 * Either this method or PageToPreserveAndReuse should be called for every page that was mapped
 * as normal memory. To check whether memory type is normal, use CacheMaintenance::IsNormal.
 * 
 * The old mapping(s) should be removed before calling this method to ensure
 * no accidental/speculative access occurs afterwards, as it would negate the effect of this
 * procedure on cache memory.
 * 
 * Since linear address is required for aBase input parameter, the caller may need to apply
 * temporary mapping. Memory type of the temporary mapping must be as it is specified
 * by CacheMaintenance::TemporaryMapping. In addition, the page colouring of the
 * old mapping(s) must apply to the temporary mapping.
 * 
 * @arg aBase				Linear address of the page.
 * @arg aOldType			Memory type of the old mapping.
 * @arg aPhysAddr			Physical adress of the page or KPhysAddrInvalid. If known, physical address
 * 							should be always specified (for performance reason).
 * @arg aMask				Orred values of TPageToReuseMask enum:
 * 			EThresholdReached:
 * 							If set, the method will trigger the maintenance on entire cache(s)(as
 * 							opposed to maintenance of only the specified region). This will effectively
 * 							sort out cache maintenance for all free pages waiting for PageToReuse call.
 * 							However, some cache levels may be unaffected by this global maintenance.
 * 							Therefore, the method still has to be called for all freed pages, but
 * 							those that follow should have EPageHasBeenPartiallySynced set in aMask.
 * 			EPageHasBeenPartiallySynced:
 *							Indicates if the page was in the queue for cache maintenance when the
 * 							maintenance of a whole cache(s) is triggered by the previous call
 * 							of this method with EThresholdReached in aMask.
 * 							If true, the method will sort out only those caches not affected by
 * 							the global cache maintenance.
 * 			EOldAndNewMappingMatch:
 * 							Indicates that the old and new caching attributes for the page are the same.
 * 							If true, the method may avoid unnecessary maintenance on some platforms.
 *
 * @return					True if page has been removed from cache memory.
 * 							False if it wasn't because aOldType doesn't require it, or
 * 							EOldAndNewMappingMatch is set on H/W platform where it is safe not
 * 							to remove page from cache if the mapping remains the same.
 */
	static TBool PageToReuse (TLinAddr aBase, TMemoryType aOldType, TPhysAddr aPhysAddr, TInt aMask=0);

/*
 * Indicates whether the number of pages waiting for PageToReuse maintenance is big enough to
 * trigger the maintenance of the entire cache(s) on particular levels. Use this method to decide
 * whether to set EThresholdReached in aMask when PageToReuse is called.
 * 
 * @arg aPageCount	Number of pages waiting in queue for CacheMaintenance::PageToReuse call. 
 * @return			True if aPageCount is big enough to trigger the maintenance of entire cache(s)
 * 					In that case, client may decide to call CacheMaintenance::PageToReuse with 
 * 					EThresholdReached in aMask argument.
 * 
 * Note:			H/W platforms which are not able to maintain entire cache always returns EFalse.
 */
	static TBool IsPageToReuseThresholdReached(TUint aPageCount);

/*
 * Specifies additional argument in aMask when CacheMaintenance::PageToReuse is called.
 */	
	enum TPageToReuseMask
		{
		/*
		 * Indicates that the call of PageToReuse maintenance must trigger the maintenance
		 * of entire cache(s) on particular level(s). The client should set
		 * this only if CacheMaintenance::IsPageToReuseThresholdReached returns ETrue.
		 */
		EThresholdReached = 1,
		/*
		 * Indicates that the page was in the queue for CacheMaintenance::PageToReuse
		 * call when the maintenance of a whole cache(s) is triggered by the previous
		 * call of CacheMaintenance::PageToReuse with EThresholdReached set in aMask.
		 */
		EPageHasBeenPartiallySynced = 2,
		/* 
		 * Indicates that the old and new cacing attributes for the page are the same.
		 */ 
		EOldAndNewMappingMatch = 4,
		};
	
/*	
 * Preserves the content and maintains cache(s) for a single page of physical memory that
 * is about to change its mapping or caching attributes.
 * 
 * The client may call this method either:
 * 	- during the process of invalidating old mapping(s), or
 *  - as background maintenance of free physical memory, or
 * 	- when the physical memory is about to be reused again.
 * 
 * Either PageToReuse or this method should be called for every page that was mapped as normal
 * memory. To check whether memory type is normal, use CacheMaintenance::IsNormal.

 * The old mapping(s) should be removed before calling this method to ensure
 * no accidental/speculative access occurs afterwards, as it would negate the effect of this
 * procedure on cache memory.
 * 
 * Since linear address is required for aBase input parameter, the caller may need to apply
 * temporary mapping. Memory type of the temporary mapping must be as it is specified
 * by CacheMaintenance::TemporaryMapping. In addition, the page colouring of the
 * old mapping(s) must apply to the temporary mapping.
 * 
 * @arg aBase				Linear address of the page.
 * @arg aOldType			Memory type of the old mapping.
 * @arg aPhysAddr			Physical adress of the page or KPhysAddrInvalid.
 */
	 static void PageToPreserveAndReuse(TLinAddr aBase, TMemoryType aOldType, TPhysAddr aPhysAddr);

/*
 * @return	Memory type for the temporary mapping for a physical page when PageToReuse or
 * 			PageToPreserveAndReuse is called.
 */
	static TMemoryType TemporaryMapping();
	
/*
 * Specifies how the source code has been changed when CodeChanged is called.
 */	
	enum TCodeChangedBy
		{
		/*
		 * The content of executable memory is overwritten through cached mapping.
		 */
		ECPUThroughCache,		
		/*
		 * The content of executable memory is overwritten through uncached mapping.
		 */
		ECPUUncached,
		/*
		 * The executable memory region is remapped.
		 */   
		EMemoryRemap,
		/*
		 * Code is changed by code modifier. It is assumed that:
		 *  - the range of modified code is within a single cache line,
		 * 	- code modifier has its own way to broadcast primitives, therefore, any cache
		 * 	  maintenance caused by this call will NOT be broadcasted by S/W.
		 */
		ECodeModifier
		};

/*
 * Maintains cache for newly loaded or changed code.  It also ensures branch predictor & execution
 * pipeline are drained accordingly.
 * Call this method after the code has been changed and before it executes.
 * 
 * The method may generate data abort exception if any part of defined memory region is not valid.
 * 
 * @arg aBase 		Linear address of the start of code. 
 * @arg aSize 		The size of the region (in bytes) whose code has been changed.
 * @arg aChangedBy	Specifies the way source code has been changed. 
 */
	static void CodeChanged(TLinAddr aBase, TUint aSize, TCodeChangedBy aChangedBy = ECPUThroughCache);

/*
 * Ensures the changes in the specified memory region made by CPUs are visible to the
 * external agents/observers.
 *   
 * The method may generate data abort exception if any part of the region is not valid.
 * 
 * @arg aBase 			Linear address of the start of memory region.
 * @arg aSize 			The size of the region in bytes.
 * @arg aMapAttr		The attributes of the region(orred TMappingAttributes enum values).
 * @arg aPhysAddr		Physical address that corresponds to aBase linear address. KPhysAddrInvalid if
 * 						unspecified. Specify this argument only if the region is contiguously mapped.
 * 
 * @pre 				As specified by MASK_THREAD_STANDARD mask. 
 */
	static void MakeCPUChangesVisible(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr = KPhysAddrInvalid);

/*
 * Prepares memory region for the external agents' write access.
 * It ensures that cache doesn't accidentally overwrite physical memory that the external agent is
 * about to write into. CPUs must not rely on the content of the region nor write into it. Once the
 * external writes are completed, CacheMaintenance::MakeExternalChangesVisible must be called.
 * 
 * Note that this will invalidate CPU writes in the region even if no external write occures.
 * 
 * The method may generate data abort exception if any part of the region is not valid.
 * 
 * @arg aBase 			Linear address of the start of memory region.
 * @arg aSize 			The size of the region in bytes.
 * @arg aMapAttr		The attributes of the region(orred TMappingAttributes enum values).
 * @arg aPhysAddr		Physical address that corresponds to aBase linear address. KPhysAddrInvalid if
 * 						unspecified. Specify this argument only if the region is contiguously mapped.
 * 
 * @pre 				As specified by MASK_THREAD_STANDARD mask. 
 */
	static void PrepareMemoryForExternalWrites(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr = KPhysAddrInvalid);

/*
 * Ensures the changes in the specified memory region made by the external agent are visible by CPUs.
 * 
 * The method may generate data abort exception if any part of the region is not valid.
 * 
 * @arg aBase 			Linear address of the start of memory region.
 * @arg aSize 			The size of the region in bytes.
 * @arg aMapAttr		The attributes of the region(orred TMappingAttributes enum values).
 * @arg aPhysAddr		Physical address that corresponds to aBase linear address. KPhysAddrInvalid if
 * 						unspecified. Specify this argument only if the region is contiguously mapped.
 */
	static void MakeExternalChangesVisible(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr = KPhysAddrInvalid);

#if defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)
// The following method maintain cache on page table/directory change.
// Moving memory always model maps page table as write-through memory so
// InternalCache::DrainBuffers is sufficient in that case.
	
/*	
 * Ensures the change in page table is visible by MMU's Page-Table Walk.
 * Client should call this method when a single entry in a page table has been changed.
 * 
 * @arg aBase 			Linear address of page table entry that has been changed.
 * 
 * @see CACHE_MAINTENANCE_PDE_PTE_UPDATED is alternative assembler macro for cia files. 
 */
	inline static void SinglePteUpdated(TLinAddr aAddr);

/*	
 * Ensures the changes in a page table are visible by MMU's Page-Table Walk.
 * Client should call this method when two and more consecutive entries in a page table
 * have been changed.
 * 
 * @arg aBase 			Linear address of the first page table entry that has been changed.
 * @arg aSize 			The size of the region (in bytes) of the altered page table entries.
 */
	inline static void MultiplePtesUpdated(TLinAddr aAddr, TUint aSize);

/*
 * Ensures the change in page directory is visible by MMU's Page-Table Walk.
 * Client should call this method when a single entry in a page directory has been changed.
 * In case of page mapping, it should also ensure that the content of page table pointed by the new
 * value is either initialised or marked as invalid (no random values are allowed).
 * 
 * @arg aBase 			Linear address of page directory entry that has been changed.
 * 
 * @see CACHE_MAINTENANCE_PDE_PTE_UPDATED is alternative assembler macro for cia files. 
 */	
	inline static void SinglePdeUpdated(TLinAddr aAddr);

/*
 * Ensures the change in page directory is visible by MMU's Page-Table Walk.
 * Client should call this method when two and more consecutive entries in a directory table
 * have been changed.
 * In case of page mapping, it should also ensure that the content of page table pointed by the new
 * value is either initialised or marked as invalid (no random values are allowed).
 * 
 * @arg aBase 			Linear address of the first page directory entry that has been changed.
 * @arg aSize 			The size of the region (in bytes) of the altered page table entries.
 */
	inline static void PdesInitialised(TLinAddr aPde, TUint aSize);

#endif //#if defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)

/*
 * @arg	aType	Memory Type
 * @return 		False if memory type is guaranteed not to be normal memory.
 * 				True if memory type may be normal memory.
 * 
 * @note		Normal uncached memory is not held in cache but may use cache buffers.
 */
	static TBool IsNormal(TMemoryType aType);
		
/*
 * @arg	aType	Memory Type
 * @return 		False if memory type is guaranteed not to be cached at any level.
 * 				True if memory type may be cached at any level.
 */
	static TBool IsCached(TMemoryType aType);

#if defined(__MEMMODEL_MOVING__) || defined(__MEMMODEL_MULTIPLE__)

/*
 * Ensures the changes in the specified memory region made by CPUs are visible to the
 * external agents/observers. The region is also removed from the caches.
 * 
 * On multiple memory model, memory region should be unmapped from its original mapping and
 * temporary mapping should be applied, as described in PageToReuse & PageToPreserveAndReuse methods.
 * On moving memory model, call this function before unmappppping occures.
 *   
 * The method may generate data abort exception if any part of the region is not valid.
 * 
 * @arg aBase 			Linear address of the start of memory region.
 * @arg aSize 			The size of the region in bytes.
 * @arg aMapAttr		The attributes of the region(orred TMappingAttributes enum values).
 */
	static void MemoryToPreserveAndReuse(TLinAddr aLinAddr, TUint aSize, TUint32 aMapAttr);

/*
 * Ensures the entire content of physical (VIPT & PIPT) data cache(s) is written down
 * to memory and the cache is emptied.
 */
	static void SyncPhysicalCache_All();

/*
 * @return 	Performance threshold for SyncPhysicalCache_All method in page count.
 * 			If the number of pages to be recommitted is bigger than the threshold,
 * 			the client may decide to use CacheMaintenance::SyncPhysicalCache_All
 * 			instead of CacheMaintenance::PageToReuse.
 */
	inline static TUint SyncAllPerformanceThresholdPages()
	{
#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
	// Clean&Invalidate by Set/Way in pl310 is broken, so we cannot maintain entire cache(s).
	// This will ensure no cache threshold is reached so all cache maitenance will be performed by cache line(s).
	return KMaxTUint;
#else
	return InternalCache::Info[KCacheInfoD].InvalidateThresholdPages();
#endif // #if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
	}

#endif // #if defined(__MEMMODEL_MOVING__) || defined(__MEMMODEL_MULTIPLE__)

#if defined(__MEMMODEL_MOVING__)
//	Moving memory model is based on ARMv5 architecture and requires virtual cache memory to be
//	flushed away on process switch. For that reason, this memory model needs separate sets
//	of primitives for virtual (VIVT) and physical (VIPT or PIPT) cache.

/*
 * Perform any cache/memory synchronisation required prior to a change
 * in virtual to physical address mappings.
 * Enter and return with system locked.
 */	
	static void OnProcessSwitch();
	
/*
 * Maintains virtual cache for a single page of physical memory that is about to change
 * its mapping/caching attributes. It is presumed the memory is fully cached.
 * @arg aBase				Linear address of the page.
 */	
	static void PageToReuseVirtualCache(TLinAddr aBase);

/*
 * Maintains virtual cache for a single page of physical memory that is about to change
 * its mapping/caching attributes. In addition, the content of physical memory is preserved.
 * It is presumed the memory is fully cached.
 * @arg aBase				Linear address of the page.
 */	
	static void PageToPreserveAndReuseVirtualCache(TLinAddr aBase);
	
/*
 * Maintains physical cache for a single page of physical memory that is about to change
 * its mapping/caching attributes. It is presumed the memory is fully cached.
 * @arg aPhysAddr			Physical adress of the page.
 */	
	static void PageToReusePhysicalCache(TPhysAddr aPhysAddr);

#endif // defined(__MEMMODEL_MOVING__)

#if defined(__MEMMODEL_DIRECT__)
/*
 * Maintains cache(s) for a memory region that is about to change its mapping/caching attributes.
 * @arg aBase				Linear address of the page.
 * @arg aSize				The size of the region.
 */
	static void MemoryToReuse (TLinAddr aBase, TUint aSize);
#endif //defined(__MEMMODEL_DIRECT__)

private:

#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
/*
 * Combines inner and outer caching attributes.
 * 
 * @arg aMapAttr		On entry, holds inner and outer caching attributes (orred
 * 						TMappingAttributes enum values).
 * 						On exit, inner caching attribute holds combined inner and outer values,
 * 						while outer caching attribute remains unchanged.
 * 
 * Note: 	On __CPU_CORTEX_A8__ both inner & outer caches are MMU controlled.
 */
	static void CombineCacheAttributes (TUint32& aMapAttr);
#endif
	};


#if  defined(__SMP__) && !defined(__BROADCAST_CACHE_MAINTENANCE__)
//Platforms that rely on H/W broadcast of cache maintenance have to broadcast ISB by softwer. 
#define __BROADCAST_ISB

class T_ISB_IPI : public TGenericIPI
    {
public:
    T_ISB_IPI();
    static void ISBIsr(TGenericIPI*);
    void Do();
    };
#endif  //defined(__SMP__) && !defined(__BROADCAST_CACHE_MAINTENANCE__)

#endif // defined(__CPU_HAS_CACHE)

#endif //#ifndef __CACHE_MAINTENANCE_H__
