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
// eka\include\kernel\cache.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __CACHE_H__
#define __CACHE_H__
#include <e32err.h>
#include <nk_cpu.h>

/**
Cache Thresholds container used to Set/Get cache thresholds from/to Kernel.
@see Cache::GetThresholds
@see Cache::SetThresholds
*/
struct TCacheThresholds
	{
	TUint32 iPurge; /**< Invalidate threshold in bytes*/
	TUint32 iClean; /**< Clean threshold in bytes*/
	TUint32 iFlush; /**< Clean and invalidate threshold in bytes*/
	};

const TUint KCacheSelectI=1; 		/**<Specifies instruction cache.*/ 
const TUint KCacheSelectD=2;		/**<Specifies data cache to the point of coherency.*/
const TUint KCacheSelectAltD=4;		/**<Specifies alternative cache. @deprecated*/
const TUint KCacheSelectD_IMB=8;	/**<Specifies data cache to the point of unification.*/
const TUint KCacheSelect_L2=0x10;	/**<Specifies external cache.*/

/**
A set of Kernel APIs for cache utility functions.
*/
class Cache
	{
public:

	/**
	Synchronises cache(s) for instruction execution in the specified address range.

	The function performs the cache/memory synchronisation required to guarantee
	correct execution of code in the specified virtual address range.

	@param aBase The start of the virtual address range.
	@param aSize The size of the address range.
	 
	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	*/
	IMPORT_C static void IMB_Range(TLinAddr aBase, TUint aSize);

	/**
	Synchronises cache(s) prior to a DMA write (memory to HW DMA transfer) operation.
	It is assumed that the memory region is fully cached.

	The purpose of SyncMemoryBeforeDmaWrite is to ensure that the main memory is
	synchronised with the content of cache memory before the start of DMA write transfer.

	@param aBase The start of the virtual address range.
	@param aSize The size of the address range.
	 
	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	*/
	IMPORT_C static void SyncMemoryBeforeDmaWrite(TLinAddr aBase, TUint aSize);

	/**
	Synchronises cache(s) prior to a DMA read (HW to memory DMA transfer) operation.
	It is assumed that the memory region is fully cached.

	The purpose of SyncMemoryBeforeDmaRead is to make sure that the content of memory
	(that is about to be DMA-ed) won't be destroyed by interaction between cache
	and the main memory. (E.g. by cache eviction or remaining content of write buffers.) 

	@param aBase The start of the virtual address range.
	@param aSize The size of the address range.
	 
	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	*/
	IMPORT_C static void SyncMemoryBeforeDmaRead(TLinAddr aBase, TUint aSize);
	
	/**
	Synchronises cache(s) after a DMA read (HW to memory DMA transfer) operation.
	It is assumed that the memory region is fully cached.

	The purpose of SyncMemoryAfterDmaRead is to make sure that CPU won't read
	old data from cache instead of DMA-ed data from the main memory.

	@param aBase The start of the virtual address range.
	@param aSize The size of the address range.
	 
	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	*/
	IMPORT_C static void SyncMemoryAfterDmaRead(TLinAddr aBase, TUint aSize);
	
	/**
	Synchronises cache(s) with main memory prior to power off or reboot.
	It ensures the content of cache is copied down to the main memory. It doesn't necessarily
	invalidates the content of cache(s).
	On SMP platforms, it only maintains internal cache of the CPU that executes the call.
	Cache memory common to the all cores (like external cache controllers) are also synchronised.
	@pre Interrupts must be disabled.
	@deprecated in TB 10.1 Use Cache::CpuRetires and Cache::KernelRetires
	*/
	IMPORT_C static void AtomicSyncMemory();

	/**
	Synchronises cache(s) of the current CPU with the next memory level (which may also be cache)
	prior to power off or reboot. Caches in other CPUs and those that are shared among them are not affected.

	Shut down or reboot sequence should ensure that the context of cache memories is copied down to the main
	memory prior CPU/cache power off. In order to achieve this goal, the following should be obeyed:
	 - On SMP H/W, any CPU that is about to shut down or reboot should call this method. The very last
	   running CPU should call both this method and Cache::KernelRetires method.
	 - On non-SMP H/W. CPU that is about to shut down or reboot should call this method. Call to
	   Cache::KernelRetires is not required.

	Both Cache::CpuRetires and Cache::KernelRetires assume that Kernel may not be in stable state (as reboot may
	be caused by crash), so no attampt will be made to acquire spin lock or call any other Kernel interface.

	@see Cache::KernelRetires
	@pre Interrupts must be disabled.
	@pre Kernel may not be in stable state.
	*/
	IMPORT_C static void CpuRetires();

	/*
	Synchronises cache(s) that are shared among CPUs with the main memory prior to power off or reboot.
	@see Cache::CpuRetires
	@pre Interrupts must be disabled.
	@pre Kernel may not be in stable state.
	@pre All CPUs other than the current CPU are powered down or their reboot sequence is completed.
	*/
	IMPORT_C static void KernelRetires();
	
	/**
	Synchronises cache(s) prior to a DMA write (memory to HW DMA transfer) operation.

	The purpose of SyncMemoryBeforeDmaWrite is to make sure that the main memory is synchronised 
	with the content of cache memory before DMA transfer from main memory starts.

	@param aBase    The start of the virtual address range.

	@param aSize    The size of the address range.

	@param aMapAttr The mapping attributes with which the address range has been mapped.
	                This is a value constructed from the bit masks in the enumeration
	                TMappingAttributes.

	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.

	@see TMappingAttributes
	*/
	IMPORT_C static void SyncMemoryBeforeDmaWrite(TLinAddr aBase, TUint aSize, TUint32 aMapAttr);

	/**
	Synchronises cache(s) prior to a DMA read ((HW to memory DMA transfer) operation.

	The purpose of SyncMemoryBeforeDmaRead is to make sure that the content of memory
	(that is about to be DMA-ed) won't be destroyed by interaction between cache
	and the main memory. (E.g. by write buffers' flushing or cache eviction.) 

	@param aBase    The start of the virtual address range.

	@param aSize    The size of the address range.

	@param aMapAttr The mapping attributes with which the address range has been mapped.
	                This is a value constructed from the bit masks in the enumeration
	                TMappingAttributes.

	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.

	@see TMappingAttributes
	*/
	IMPORT_C static void SyncMemoryBeforeDmaRead(TLinAddr aBase, TUint aSize, TUint32 aMapAttr);
	
	/**
	Synchronises cache(s) after a DMA read (HW to memory DMA transfer) operation.

	The purpose of SyncMemoryAfterDmaRead is to make sure that CPU won't read
	old data from cache instead of DMA-ed data from the main memory.

	@param aBase The start of the virtual address range.
	@param aSize The size of the address range.
	@param aMapAttr The mapping attributes with which the address range has been mapped.
	                This is a value constructed from the bit masks in the enumeration
	                TMappingAttributes.
	 
	@pre Call in a thread context.
	@pre No fast mutex can be held.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.

	@see TMappingAttributes
	*/
	IMPORT_C static void SyncMemoryAfterDmaRead(TLinAddr aBase, TUint aSize, TUint32 aMapAttr);

	/**
	Prepares physical memory for DMA writing (memory to H/W DMA copy). If the physical memory is the
	subject of RAM defragmentation framework (e.g. if it is mapped in user side space) the driver
	should make sure it is pinned (@see Kern::PinPhysicalMemory) before calling this Kernel service.
	Kern::PinPhysicalMemory also generates input parameters aPages and aColour for this Kernel service.

	@param	aPages		Pointer to the the list of physical memory pages to be prepared for DMA write.
						The exact portion of physical memory to be prepared is defined by aOffset and
						aSize parameters. For example, if aOffset is 0x1800 and aSize is 0x2000, the method
						will sync the last 0x800 bytes of the second page in the list, the whole third page
						and the first 0x800  bytes of the fourth page. (0x1000 page size is assumed).
	@param	aColour		The mapping colour of the first physical page in the list. 
	@param	aOffset		Offset in memory list where DMA transfer should start.
	@param	aSize		The size of the memory in bytes to be sync'ed.
	@param	aMapAttr	Mapping attributes of the the existing mapping. The value is either:
						  - Formed by ORing together values from the TMappingAttributes enumeration as
						    returned by Kern::ChunkCreate interface, or
						  - TMappingAttributes2 object.
						For user memory (always fully cached), EMapAttrCachedMax enum value can be passed.

	@return 			KErrNotSupported on memory models other than flexible.
						KErrNone, on flexible memory model.

	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Can be used in a device driver.
	*/
	IMPORT_C static TInt SyncPhysicalMemoryBeforeDmaWrite(TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);

	/**
	Prepares physical memory for DMA read (H/W to memory DMA copy).
	For all the details @see Cache::SyncPhysicalMemoryBeforeDmaWrite
	*/
	IMPORT_C static TInt SyncPhysicalMemoryBeforeDmaRead (TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);

	/**
	Maintains physical memory after DMA read (H/W to memory DMA copy).
	For all the details @see Cache::SyncPhysicalMemoryBeforeDmaWrite
	*/
	IMPORT_C static TInt SyncPhysicalMemoryAfterDmaRead  (TPhysAddr* aPages, TUint aColour, TUint aOffset, TUint aSize, TUint32 aMapAttr);

/*
 * Gets threshold values for the specified cache.
 * 
 * When Kernel is about to perform cache maintenance operation, the size of the region to act
 * against is compared to the corresponding threshold value:
 *  - When invalidating memory region:
 * 		@code
 * 			if (invalidate region > invalidate threshold)
 * 				clean and invalidate entire cache
 * 			else
 * 				invalidate the specified region
 * 		@endcode
 * 
 *	- When cleaning memory region:
 * 		@code
 * 			if (clean region > clean threshold)
 * 				clean entire cache
 * 			else
 * 				clean the specified region
 * 		@endcode
 * 
 *	- When invalidating and cleaning memory region:
 * 		@code
 * 			if (invalidate and clean region > invalidate and clean threshold)
 * 				invalidate and clean entire cache
 * 			else
 * 				invalidate and clean the specified region
 * 		@endcode
 * 
 * This function returns the current threshold values for the specified type of cache. Threshold
 * values could be changed by Cache::SetThresholds.
 * 
 * @param aCacheType 	Specifies which type of cache the thresholds belong to:
 * 		- KCacheSelectI 		Instruction Cache
 *		- KCacheSelectD 		Data Cache. For ARMv7 platforms, it specifies Point-of-Coherency thresholds.
 * 								These threshold are relevant for DMA related cache maintenance.
 * 		- KCacheSelectD_IMB 	Data Cache for the Point-of-Unification.
 * 								These threshold are relevant for Instruction Memory Barrier (IMB).
 * 		- KCacheSelectAltD 		Alternative Data Cache. This type of cache is depricated on today's platforms.
 * 		- KCacheSelect_L2		External Cache, such as L210, L220 or PL310.
 * @param aThresholds 	If KErrNone is returned, holds thresholds values(in bytes) for the specified cache.
 * 
 * @return 						KErrNone if successfull or KErrNotSupported if aCacheType is not valid on the
 * 								running platform.
 * 
 * @see Cache::SetThresholds
 * @see TCacheThresholds
 * @released 9.3
 */
	IMPORT_C static TInt GetThresholds(TCacheThresholds& aThresholds, TUint aCacheType);

/*
 * Sets threshold values for the specified cache. @See GetThresholds for details.
 * 
 * @param aCacheType 	Specifies which type of cache the thresholds belong to:
 * 		- KCacheSelectI 		Instruction Cache
 *		- KCacheSelectD 		Data Cache. For ARMv7 platforms, it specifies Point-of-Coherency thresholds.
 * 								These threshold are relevant for DMA related cache maintenance.
 * 		- KCacheSelectD_IMB 	Data Cache for the Point-of-Unification.
 * 								These threshold are relevant for Instruction Memory Barrier (IMB).
 * 		- KCacheSelectAltD 		Alternative Data Cache. This type of cache is depricated on today's platforms.
 * 		- KCacheSelect_L2		External Cache, such as L210, L220 or PL310.
 * @param aThresholds 			New threshold values (in bytes) for the cache..
 * 
 * @return 						KErrNone if successfull or KErrNotSupported if aCacheType is not valid on the
 * 								running platform.
 * 
 * @see Cache::GetThresholds
 * @see TCacheThresholds
 * @released 9.3
 */
	IMPORT_C static TInt SetThresholds(const TCacheThresholds& aThresholds, TUint aCacheType);

/*
 * Returns the required alignment for fully cached memory buffer used in DMA transfer.
 * Use this value to separate DMA from non-DMA memory.
 * 
 * Note that a single DMA transfer can still start/stop from any memory location. However,
 * the content of memory just before (if start address is unaligned) or after (if end address
 * is unaligned) DMA buffer may be corrupted.
 * 
 * Here is an example of code that allocates DMA buffer from the heap.
 * 
 * @code
 * 	class DMABufferAlloc
 *		{
 * 	public:
 * 		DMABufferAlloc():iPtr(NULL){};
 * 
 * 		//Return value is guaranteed to be aligned.
 *		TAny* Alloc(TInt aSize)
 * 			{
 * 			TInt alignmentMask = Cache::DmaBufferAlignment()-1;
 * 			NKern::ThreadEnterCS();
 * 
 * 			// Assume that the return value of Kern::Alloc is unaligned.
 *			// Allocate sufficient memory to cover the worst case, for example:
 *			// Alignment = 32, aSize = 2. If (iPtr==31) 64 bytes are required   
 *			iPtr = Kern::Alloc( (aSize+2*alignmentMask) & ~alignmentMask);
 * 
 * 			NKern::ThreadLeaveCS();
 * 
 *			//Return the first aligned location in the allocated buffer.
 *			return (TAny*)(((TInt)iPtr + alignmentMask) & ~alignmentMask);
 *			}
 * 
 *		void Free()
 *			{
 *			NKern::ThreadEnterCS();
 *			Kern::Free(iPtr);
 *			iPtr = NULL;
 *			NKern::ThreadLeaveCS();
 *			}
 *
 * 		~DMABufferAlloc()
 * 			{
 *			if (iPtr)
 *			Free();
 *			};
 * 
 *	private:
 *		TAny* iPtr;
 * 		};
 *	@codeend
 */
	IMPORT_C static TUint DmaBufferAlignment();
	};

#endif // def __CACHE_H__
