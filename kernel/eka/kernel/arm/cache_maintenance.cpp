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
// eka\kernel\arm\cache_maintenance.cpp
// 
//

#include <arm.h>
#include "cache_maintenance.h"
#include <kernel/cache.h>

#define CACHEFAULT()	CacheFault(__LINE__)

void CacheFault(TInt aLine)
	{
	Kern::Fault("cache_maintenance.cpp",aLine);
	}

SCacheInfo InternalCache::Info[KNumCacheInfos];
#if defined(__CPU_ARMV7)
TInt InternalCache::DmaBufferAlignementLog2; 
#endif

#if !defined(__CPU_MEMORY_TYPE_REMAPPING)
TUint32 InternalCache::iPrimaryRegionRemapRegister;
TUint32 InternalCache::iNormalMemoryRemapRegister;
#endif


void CacheMaintenance::Init1()
	{
	InternalCache::Init1();
#ifdef __HAS_EXTERNAL_CACHE__
	ExternalCache::Init1();
#endif
	}

TBool CacheMaintenance::PageToReuse (TLinAddr aLinAddr, TMemoryType aOldType, TPhysAddr aPhysAddr,TInt aMask)
	{
	TBool pageRemovedFromCache = EFalse; //Return value will indicate if page is removed from caches.
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PageToReuse va:%08x type:%x pa:%08x mask:%d",
		aLinAddr,aOldType, aPhysAddr, aMask));
	__ASSERT_DEBUG((aLinAddr&KPageMask)==0, CACHEFAULT());
#if defined(__MEMMODEL_MOVING__)
	// Moving memory model is not supposed to pass EOldAndNewMappingMatch as it maintains
	// cache on decommit (when new mapping is not known).
	__ASSERT_DEBUG((aMask & EOldAndNewMappingMatch)==0, CACHEFAULT());
#endif	
	if (aMask & EThresholdReached)
		{
		#if  defined(__CPU_ARMV7) && defined(__SMP__)
		CACHEFAULT(); // Cannot CleanAndInvalidate L1 cache on such platforms. 
		#endif
		// Force Clean&Invalidate of entire internal DCache. Do not call
		// InternalCache::CleanAndInvalidate_DCache_All directly as it maintains
		// only the running core (while Invalidate is broadcasted if necessary).
		InternalCache::Invalidate(KCacheSelectD, aLinAddr, TUint(-1));
		pageRemovedFromCache = ETrue;
		}

	switch(aOldType)
		{
		case EMemAttStronglyOrdered:
            break;

		case EMemAttDevice:
            //Device memory reads/writes are not combined nor reordered but can still be buffered.
            InternalCache::DrainBuffers();
            break;

		case EMemAttNormalUncached:
			InternalCache::DrainBuffers();
		#if defined (__ARM_PL310_CACHE__)
			ExternalCache::DrainBuffers();			
		#endif
			break;

		case EMemAttNormalCached:
		default:// Platform specific TMemoryType-s are treated as fully cached. No attempt has been
				// made to optimize their maintenance as they are unlikely to be reused frequently.
			{
			// Sort out internal cache...
			
			if (aMask & (EPageHasBeenPartiallySynced | EThresholdReached))
				{
				// Do nothing as this page has been already invalidated from internal cache by 
				// this or a previous call because of EThresholdReached in aMask.
				}
			else
				{
			#if defined (__CPU_D_CACHE_HAS_COLOUR) && defined(__CPU_CACHE_PHYSICAL_TAG)
				// Do not check EOldAndNewMappingMatch.  Even if it is set, the colour of the
				// old and new mapping can still be different.
				InternalCache::Invalidate(KCacheSelectD, aLinAddr, KPageSize);
				InternalCache::DrainBuffers();
				pageRemovedFromCache = ETrue;
			#else
				if (aMask & EOldAndNewMappingMatch)
					{
					// VIPT & PIPT cache memory with no colour restriction doesn't need to
					// be maintained if memory attributes are unchanged on remapping.
					}
				else
					{
					InternalCache::Invalidate(KCacheSelectD, aLinAddr, KPageSize);
					InternalCache::DrainBuffers();
					pageRemovedFromCache = ETrue;
					}
			#endif // else defined (__CPU_D_CACHE_HAS_COLOUR) || defined(__MEMMODEL_MOVING__)
				}

			// Sort out external cache...
		
		#if defined (__HAS_EXTERNAL_CACHE__)
			if (aMask & EOldAndNewMappingMatch)
				{
				//External cache is PIPT, so there is nothing to be done here.
				}
			else
				{
				if (aPhysAddr == KPhysAddrInvalid)
					ExternalCache::Invalidate(aLinAddr, KPageSize);
				else
					ExternalCache::InvalidatePhysicalMemory(aPhysAddr, KPageSize);
				}
		#endif //defined (__HAS_EXTERNAL_CACHE__)
			}//end of default case
		}//end of switch

	return pageRemovedFromCache;
	}

TBool CacheMaintenance::IsPageToReuseThresholdReached(TUint aPageCount)
	{
#if  defined(__CPU_ARMV7) && defined(__SMP__)
	// There is no reliable way to broadcast maintenance of entire cache on ARMv7 SMP.
	(void)aPageCount;
	return EFalse;
#else
	TBool ret = EFalse;
	if (aPageCount >= InternalCache::Info[KCacheInfoD].InvalidateThresholdPages())
		ret = ETrue;
	return ret;
#endif
	}
	
void CacheMaintenance::PageToPreserveAndReuse (	TLinAddr aLinAddr, TMemoryType aOldType, TPhysAddr aPhysAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PageToPreserveAndReuse va:%08x type:%x pa:%08x",
		aLinAddr,aOldType, aPhysAddr));
	__ASSERT_DEBUG((aLinAddr&KPageMask)==0, CACHEFAULT());
	switch(aOldType)
		{
		case EMemAttStronglyOrdered:
            break;

		case EMemAttDevice:
            //Device memory reads/writes are not combined nor reordered but can still be buffered.
            InternalCache::DrainBuffers();
            break;
		
		case EMemAttNormalUncached:
			InternalCache::DrainBuffers();
#ifdef __HAS_EXTERNAL_CACHE__
			ExternalCache::DrainBuffers();			
#endif
			break;
		
		case EMemAttNormalCached:
		default:// Platform specific TMemoryType-s are treated as fully cached. No attempt has been
				// made to optimize their maintenance as they are unlikely to be reused frequently.
			InternalCache::CleanAndInvalidate(KCacheSelectD, aLinAddr, KPageSize);
#ifdef __HAS_EXTERNAL_CACHE__
			if (aPhysAddr == KPhysAddrInvalid)
				ExternalCache::CleanAndInvalidate(aLinAddr, KPageSize);
			else
				ExternalCache::CleanAndInvalidatePhysicalMemory(aPhysAddr, KPageSize);
#endif
		}//end of switch
	}

TMemoryType CacheMaintenance::TemporaryMapping()
	{
#if defined (__CPU_CORTEX_A8__)
	// This is the fix for problem with cortex_a8 (rev 1.3) that freezes if we maintain cache with
	// uncached attributes. Speculative read is not implemented on cortex_a8 so this should be safe to do this.
	return EMemAttNormalCached;
#elif defined(__CPU_ARMV7)
	return EMemAttNormalUncached;	// due to speculative read
#else
	return EMemAttNormalCached;    // armv6 requires this
#endif	
	}

void CacheMaintenance::CodeChanged(TLinAddr aBase, TUint aSize, TCodeChangedBy aChangedBy)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::CodeChanged %08x+%08x by %d",aBase,aSize, aChangedBy));
	switch (aChangedBy)
		{
		case ECPUThroughCache:
#if defined(__CPU_ARMV7)
			InternalCache::CleanPoU(aBase,aSize);
#else
			InternalCache::Clean(KCacheSelectD,aBase,aSize);
#endif
			InternalCache::Invalidate(KCacheSelectI,aBase,aSize);
			break;

		case ECPUUncached:
			InternalCache::DrainBuffers();
			InternalCache::Invalidate(KCacheSelectI,aBase,aSize);
			break;

		case EMemoryRemap:
			InternalCache::Invalidate(KCacheSelectI,aBase,aSize);
			break;
			
		case ECodeModifier:
			{
		#ifdef _DEBUG
			TInt lineSize = InternalCache::Info[KCacheInfoD].iLineLength;
			// Ensure Dcache * ICache have the same cache line size.
            // Or else, rewrite some InternalCache primitives.
			__ASSERT_DEBUG(lineSize == InternalCache::Info[KCacheInfoI].iLineLength, CACHEFAULT());
			TInt lineMask = lineSize-1;
			// Ensure the region is within a single cache line.
			__ASSERT_DEBUG( (aBase&~lineMask) == ((aBase+aSize-1)&~lineMask), CACHEFAULT());
		#endif			
			InternalCache::IMB_CacheLine(aBase);
			break;
			}
		default:
			CACHEFAULT();
		}	
	}

void CacheMaintenance::MakeCPUChangesVisible(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"CacheMaintenance::MakeCPUChangesVisible");				
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::MakeCPUChangesVisible %08x+%08x attr=%08x pa=%08x",
			aBase,aSize,aMapAttr,aPhysAddr));

#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
	CombineCacheAttributes(aMapAttr);
#endif	
	
	switch(aMapAttr&EMapAttrL1CacheMask)
		{
	case EMapAttrFullyBlocking:
		return;

	case EMapAttrBufferedNC:
        //Device memory reads/writes are not combined nor reordered but can still be buffered.
		InternalCache::DrainBuffers();
		return;

	case EMapAttrBufferedC:
		InternalCache::DrainBuffers();
		#if defined (__ARM_PL310_CACHE__)
		// PL310 cotroller buffers normal memory regardless of external cache attributes.
		ExternalCache::DrainBuffers();  
		#endif
		return;
		
	case EMapAttrL1Uncached:
	case EMapAttrCachedWTRA:
	case EMapAttrCachedWTWA:
	case EMapAttrAltCacheWTRA:
	case EMapAttrAltCacheWTWA:
		InternalCache::DrainBuffers();
		break;

	default:
		InternalCache::Clean(KCacheSelectD,aBase,aSize);
		break;
		}

#ifdef __HAS_EXTERNAL_CACHE__

#if defined(__ARM_L2_CACHE_WT_MODE)	
	// All cache lines are WT. No need to clean them. Check if memory is buffered.
	#if defined (__ARM_PL310_CACHE__)
	// PL310 cotroller buffers normal memory regardless of external cache attributes.
	ExternalCache::DrainBuffers();
	#else //defined (__ARM_PL310_CACHE__)
	if (aMapAttr&EMapAttrL2CacheMask)
		ExternalCache::DrainBuffers();
	#endif // else defined (__ARM_PL310_CACHE__)
#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
	// On ARMv5, all cache lines are WB. We have to clean even the memory marked in page table as WT.
	if (aMapAttr&EMapAttrL2CacheMask)
		{
		if (aPhysAddr == KPhysAddrInvalid)
			ExternalCache::Clean(aBase, aSize);
		else
			ExternalCache::CleanPhysicalMemory(aPhysAddr, aSize);
		}
#else //neither defined(__ARM_L2_CACHE_WT_MODE) nor defined(__CPU_ARM9XXX). This is ARM6&7 case
	// This is normal case where we flush write buffer for WT memory and 
	// clean cache lines for WB memory
	switch (aMapAttr&EMapAttrL2CacheMask)
		{
		case EMapAttrL2Uncached:
			#if defined (__ARM_PL310_CACHE__)
			// PL310 cotroller buffers normal memory regardless of external cache attributes.
			ExternalCache::DrainBuffers();		
			#endif // defined (__ARM_PL310_CACHE__)
			break; 
		case EMapAttrL2CachedWTRA:
		case EMapAttrL2CachedWTWA:
			ExternalCache::DrainBuffers();
			break;
		default:					
			if (aPhysAddr == KPhysAddrInvalid)
				ExternalCache::Clean(aBase, aSize);
			else
				ExternalCache::CleanPhysicalMemory(aPhysAddr, aSize);
			break;
		}
#endif // (__ARM_L2_CACHE_WT_MODE) #elif (__CPU_ARM9XXX__)
	
#endif //__HAS_EXTERNAL_CACHE__
	}

void CacheMaintenance::PrepareMemoryForExternalWrites(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"CacheMaintenance::PrepareMemoryForExternalWrites");				
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PrepareMemoryForExternalWrites %08x+%08x attr=%08x pa=%08x",aBase,aSize,aMapAttr,aPhysAddr));
	
#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
	CombineCacheAttributes(aMapAttr);
#endif	

	switch(aMapAttr&EMapAttrL1CacheMask)
		{
	case EMapAttrFullyBlocking:
		return;	// No action required for this type of memory.

	case EMapAttrBufferedNC:
        //Device memory reads/writes are not combined nor reordered but can still be buffered.
		InternalCache::DrainBuffers();
		return;

	case EMapAttrBufferedC:
		InternalCache::DrainBuffers();
		#if defined (__ARM_PL310_CACHE__)
		// PL310 cotroller buffers normal memory regardless of external cache attributes.
		ExternalCache::DrainBuffers();  
		#endif
		return;
		
	case EMapAttrL1Uncached:
	case EMapAttrCachedWTRA:
	case EMapAttrCachedWTWA:
	case EMapAttrAltCacheWTRA:
	case EMapAttrAltCacheWTWA:
		InternalCache::DrainBuffers(); // in case of (__ARM_PL310_CACHE__), it must be followed by ExternalCache::DrainBuffers below.
		break;

	default:
		InternalCache::Invalidate(KCacheSelectD,aBase,aSize);
		InternalCache::DrainBuffers(); //// in case of (__ARM_PL310_CACHE__), it must be followed by ExternalCache::DrainBuffers below.
		break;
		}

#ifdef __HAS_EXTERNAL_CACHE__
#if defined(__ARM_L2_CACHE_WT_MODE)	
	// All cache lines are in WT mode. Check if memory is buffered.
	#if defined (__ARM_PL310_CACHE__)
	// PL310 cotroller buffers normal memory regardless of external cache attributes.
	ExternalCache::DrainBuffers();
	#else //defined (__ARM_PL310_CACHE__)
	if (aMapAttr&EMapAttrL2CacheMask)
		ExternalCache::DrainBuffers();
#endif // else defined (__ARM_PL310_CACHE__)
#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
	// On ARMv5, all cache lines are WB. We have to Invalidate even if the memory is marked in page tables as WT.
	if (aMapAttr&EMapAttrL2CacheMask)
		{
		if (aPhysAddr == KPhysAddrInvalid)
			ExternalCache::Invalidate(aBase, aSize);
		else
			ExternalCache::InvalidatePhysicalMemory(aPhysAddr, aSize);
		}
	ExternalCache::DrainBuffers();
#else //not ARM92xx
	// This is normal case where treat memory according to cache attributes.
	switch (aMapAttr&EMapAttrL2CacheMask)
		{
		case EMapAttrL2Uncached:
			#if defined (__ARM_PL310_CACHE__)
			// PL310 cotroller buffers normal memory regardless of external cache attributes.
			ExternalCache::DrainBuffers();		
			#endif // defined (__ARM_PL310_CACHE__)
			break; 
		case EMapAttrL2CachedWTRA:
		case EMapAttrL2CachedWTWA:
			// By definition, WT memory is written back (through write/flush/eviction buffers)
			// into main memory as soon as its content in cache line is changed. Therefore,
			// once write buffers are empty, there will be no cache-to-memory transactions any more. 			
			ExternalCache::DrainBuffers();
			break;
		default:
			// WB memory must be Invalidated as cache eviction may generate cache-to-memory transfer.
			if (aPhysAddr == KPhysAddrInvalid)
				ExternalCache::Invalidate(aBase, aSize);
			else
				ExternalCache::InvalidatePhysicalMemory(aPhysAddr, aSize);
			ExternalCache::DrainBuffers();
			break;
		}
#endif  // (__ARM_L2_CACHE_WT_MODE) #elif (__CPU_ARM920T__) || (__CPU_ARM925T__) || (__CPU_ARM926J__)
	
#endif  // defined __HAS_EXTERNAL_CACHE__
	}

void CacheMaintenance::MakeExternalChangesVisible(TLinAddr aBase, TUint aSize, TUint32 aMapAttr, TPhysAddr aPhysAddr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"CacheMaintenance::MakeExternalChangesVisible");
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::MakeExternalChangesVisible %08x+%08x attr=%08x pa=%08x",aBase,aSize,aMapAttr,aPhysAddr));

#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
	CombineCacheAttributes(aMapAttr);
#endif	
	
#ifdef __HAS_EXTERNAL_CACHE__
	if (aMapAttr&EMapAttrL2CacheMask)
		{
		if (aPhysAddr == KPhysAddrInvalid)
			ExternalCache::Invalidate(aBase, aSize);
		else
			ExternalCache::InvalidatePhysicalMemory(aPhysAddr, aSize);
		}	
#endif  // __HAS_EXTERNAL_CACHE__
	
	switch(aMapAttr&EMapAttrL1CacheMask)
		{
	case EMapAttrFullyBlocking:
	case EMapAttrBufferedNC:
	case EMapAttrBufferedC:
	case EMapAttrL1Uncached:
		break;				//Not in internal cache.

	default:
		// Any kind of caching requires invalidation.
		// For multilevel internal caches, do it in reverse order.
		InternalCache::Invalidate_DCache_Reverse(aBase,aSize);
		break;
		}
	}

TBool CacheMaintenance::IsNormal(TMemoryType aType)
	{
	switch (aType)
		{
		case EMemAttStronglyOrdered:
		case EMemAttDevice:
			return EFalse;
		default:
			return ETrue;
		}
	}

TBool CacheMaintenance::IsCached(TMemoryType aType)
	{
	switch (aType)
		{
		case EMemAttStronglyOrdered:
		case EMemAttDevice:
		case EMemAttNormalUncached:
			return EFalse;
		default:
			return ETrue;
		}
	}

#if defined(__MEMMODEL_MOVING__) || defined(__MEMMODEL_MULTIPLE__)

void CacheMaintenance::MemoryToPreserveAndReuse(TLinAddr aBase, TUint aSize, TUint32 aMapAttr)
	{
	//Note: PL310 is not supported on these memory models.
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::MemoryToPreserveAndReuse %08x+%08x attr=%08x",aBase,aSize,aMapAttr));

#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
	CombineCacheAttributes(aMapAttr);
#endif	
	
	switch(aMapAttr&EMapAttrL1CacheMask)
		{
	case EMapAttrFullyBlocking:
		return;

	case EMapAttrBufferedNC:
	case EMapAttrBufferedC:
		InternalCache::DrainBuffers();
		return;

	case EMapAttrL1Uncached:
	case EMapAttrCachedWTRA:
	case EMapAttrCachedWTWA:
	case EMapAttrAltCacheWTRA:
	case EMapAttrAltCacheWTWA:
		InternalCache::DrainBuffers();
		break;

	default:
		InternalCache::CleanAndInvalidate(KCacheSelectD,aBase,aSize);
		break;
		}

#ifdef __HAS_EXTERNAL_CACHE__

#if defined(__ARM_L2_CACHE_WT_MODE)	
	// All cache lines are WT. No need to clean them. Check if memory is buffered.
	if (aMapAttr&EMapAttrL2CacheMask)
		ExternalCache::DrainBuffers();
#elif defined(__CPU_ARM920T__) || defined(__CPU_ARM925T__) || defined(__CPU_ARM926J__)
	// On ARMv5, all cache lines are WB. We have to clean even the memory marked in page table as WT.
	if (aMapAttr&EMapAttrL2CacheMask)
		ExternalCache::CleanAndInvalidate(aBase, aSize);
#else //neither defined(__ARM_L2_CACHE_WT_MODE) nor defined(__CPU_ARM9XXX).
	switch (aMapAttr&EMapAttrL2CacheMask)
		{
		case EMapAttrL2Uncached:
			break; 
		case EMapAttrL2CachedWTRA:
		case EMapAttrL2CachedWTWA:
			ExternalCache::DrainBuffers();
			break;
		default:					
			ExternalCache::CleanAndInvalidate(aBase, aSize);
			break;
		}
#endif // (__ARM_L2_CACHE_WT_MODE) #elif (__CPU_ARM9XXX__)
	
#endif //__HAS_EXTERNAL_CACHE__
	}

void CacheMaintenance::SyncPhysicalCache_All()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::SyncPhysicalCache_All"));
#ifdef __CPU_CACHE_PHYSICAL_TAG
	InternalCache::CleanAndInvalidate_DCache_All();
#endif //__CPU_CACHE_PHYSICAL_TAG
#ifdef __HAS_EXTERNAL_CACHE__
	ExternalCache::Maintain_All((TInt*)(ExternalCache::Base+ARML2C_CleanInvalidateByIndexWay));
#endif //__HAS_EXTERNAL_CACHE__
	}
#endif // defined(__MEMMODEL_MOVING__) || defined(__MEMMODEL_MULTIPLE__)


#if defined(__MEMMODEL_MOVING__)

void CacheMaintenance::PageToReuseVirtualCache(TLinAddr aLinAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PageToReuseVirtualCache %08x",aLinAddr));
	__ASSERT_DEBUG((aLinAddr&KPageMask)==0, CACHEFAULT());
	InternalCache::Invalidate_DCache_Region(aLinAddr, KPageSize);
	}

void CacheMaintenance::PageToPreserveAndReuseVirtualCache(TLinAddr aLinAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PageToPreserveAndReuseVirtualCache %08x",aLinAddr));
	__ASSERT_DEBUG((aLinAddr&KPageMask)==0, CACHEFAULT());
	InternalCache::CleanAndInvalidate_DCache_Region(aLinAddr, KPageSize);
	}

void CacheMaintenance::PageToReusePhysicalCache(TPhysAddr aPhysAddr)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::PageToReusePhysicalCache %08x",aPhysAddr));
	__ASSERT_DEBUG((aPhysAddr&KPageMask)==0, CACHEFAULT());
#ifdef __HAS_EXTERNAL_CACHE__
	ExternalCache::InvalidatePhysicalMemory(aPhysAddr, KPageSize);
#endif
	}

#endif //#if defined(__MEMMODEL_MOVING__)

#if defined(__MEMMODEL_DIRECT__)
	void CacheMaintenance::MemoryToReuse(TLinAddr aBase, TUint aSize)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("CacheMaintenance::MemoryToReuse %08x+%08x",aBase,aSize));
		InternalCache::Invalidate(KCacheSelectI|KCacheSelectD, aBase, aSize);
		}
#endif //defined(__MEMMODEL_DIRECT__)


#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)
void CacheMaintenance::CombineCacheAttributes(TUint32& aMapAttr)
	{
	TUint innerAttr = (aMapAttr & EMapAttrL1CacheMask)>>12; //defined from 1000h to f000h
	TUint outerAttr = (aMapAttr & EMapAttrL2CacheMask)>>16; //defined from 10000h to f0000h
	if (outerAttr> innerAttr) //replace inner with outer attributes if later are bigger.
		aMapAttr = (aMapAttr & ~EMapAttrL1CacheMask) | (outerAttr << 12);
	}
#endif	//#if defined (__CPU_OUTER_CACHE_IS_INTERNAL_CACHE)

//*********InternalCache**************/

#if defined(__CPU_ARMV7)
#ifdef _DEBUG
void DumpCacheInfo(SCacheInfo& aCache)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("LineLength %04x LineLenLog2 %02x",aCache.iLineLength,aCache.iLineLenLog2));
	__KTRACE_OPT(KMMU,Kern::Printf("PurgeThreshold %04x CleanThreshold %04x FlushThreshold %04x",aCache.iInvalidateThreshold,aCache.iCleanThreshold,aCache.iCleanAndInvalidateThreshold));
	}
#endif //def _DEBUG

struct SCacheLevelInfo
	{
	TInt iNumSets;
	TInt iAssoc;
	TInt iLineLenLog2;
	TInt iSize;
	};

void ParseCacheLevelInfo(TInt aCacheSizeIDReg, SCacheLevelInfo& aCacheLevelInfo)
	{
	aCacheLevelInfo.iNumSets = ((aCacheSizeIDReg>>13)& 0x7fff)+1;
	aCacheLevelInfo.iAssoc =   ((aCacheSizeIDReg>>3)& 0x3ff)+1;
	aCacheLevelInfo.iLineLenLog2 =((aCacheSizeIDReg & 0x7)+4);//+2 (+2 as we count in bytes)
	aCacheLevelInfo.iSize = aCacheLevelInfo.iNumSets * aCacheLevelInfo.iAssoc * (1<<aCacheLevelInfo.iLineLenLog2);
	}

void InternalCache::Init1()
	{
	SCacheLevelInfo cli;
	
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::Init1"));
	TUint32 ctr=TypeRegister();
	__KTRACE_OPT(KMMU,Kern::Printf("CTR:%x",ctr));
	TUint32 clr=LevelIDRegister();
	__KTRACE_OPT(KMMU,Kern::Printf("CLR:%x",clr));
	TInt LoC = (clr>>24)&7;	//The number of levels to be purged/clean to Point-to-Coherency
#if  defined(__SMP__)
	// LoUIS seems to be not implemented on cortex_a9. 
	//TInt LoU = (clr>>21)&7;	//The number of levels to be purged/clean to Point-to-Unification in LoUIS field
	TInt LoU = (clr>>27)&7; //The number of levels to be purged/clean to Point-to-Unification in LoUU field
#else
	TInt LoU = (clr>>27)&7; //The number of levels to be purged/clean to Point-to-Unification in LoUU field
#endif
	TInt dataPoCNum=0, dataPoUNum=0, codePoUNum=0; 			//The number of levels for caches
	TInt dataPoCCount=0, dataPoUCount=0, codePoUCount=0;	//Cache size counters used to calculate thresholds
	Info[KCacheInfoI].iLineLenLog2=Info[KCacheInfoD_PoU].iLineLenLog2=Info[KCacheInfoD].iLineLenLog2=9; //this is max. possible value for iLineLenLog2
	DmaBufferAlignementLog2 = 0;
	
	TInt level;
	for (level=0;level<LoC;level++)
		{
		TInt type = (clr >> (level*3)) & 7; //000:NoCache 001:ICache 010:DCache 011:Both 100:Unified
		__KTRACE_OPT(KMMU,Kern::Printf("type:%x",type));
		__ASSERT_ALWAYS(type<=4,CACHEFAULT());
		
		if (type==0)		// No Cache. Also no cache below this level
			break;
		
		if(type & 1) 	// Instruction Cache
			{
			TInt csr = SizeIdRegister(1,level);
			ParseCacheLevelInfo(csr, cli);
			__KTRACE_OPT(KMMU,Kern::Printf("I-CACHE L%d:%x, numsets:%xh, assoc:%xh, lineLog2:%xh, size=%xh",
								level+1, csr, cli.iNumSets, cli.iAssoc, cli.iLineLenLog2, cli.iSize));
			if (level<LoU)
				{
				codePoUNum++;
				codePoUCount+=cli.iSize;
				if (Info[KCacheInfoI].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoI].iLineLenLog2=cli.iLineLenLog2;
				}
			}
			
		if(type & 2) 	// Data Cache
			{
			TInt csr = SizeIdRegister(0,level);
			ParseCacheLevelInfo(csr, cli);
			__KTRACE_OPT(KMMU,Kern::Printf("D-CACHE L%d:%x, numsets:%xh, assoc:%xh, lineLog2:%xh, size=%xh",
								level+1, csr, cli.iNumSets, cli.iAssoc, cli.iLineLenLog2, cli.iSize));
			dataPoCNum++;
			dataPoCCount+=cli.iSize;
			if (DmaBufferAlignementLog2<cli.iLineLenLog2) DmaBufferAlignementLog2=cli.iLineLenLog2;
			if (Info[KCacheInfoD].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoD].iLineLenLog2=cli.iLineLenLog2;

			if (level<LoU)
				{
				dataPoUNum++;
				dataPoUCount+=cli.iSize;
				if (Info[KCacheInfoD_PoU].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoD_PoU].iLineLenLog2=cli.iLineLenLog2;
				}
			
			}

		if(type & 4) 	// Unified Cache
			{
			TInt csr = SizeIdRegister(0,level);
			ParseCacheLevelInfo(csr, cli);
			__KTRACE_OPT(KMMU,Kern::Printf("U-CACHE L%d:%x, numsets:%xh, assoc:%xh, lineLog2:%xh, size=%xh",
								level+1, csr, cli.iNumSets, cli.iAssoc, cli.iLineLenLog2, cli.iSize));
			dataPoCNum++;
			dataPoCCount+=cli.iSize;
			if (DmaBufferAlignementLog2<cli.iLineLenLog2) DmaBufferAlignementLog2=cli.iLineLenLog2;
			if (Info[KCacheInfoD].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoD].iLineLenLog2=cli.iLineLenLog2;

			if (LoU<level)
				{
				codePoUNum++;
				codePoUCount+=cli.iSize;
				if (Info[KCacheInfoI].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoI].iLineLenLog2=cli.iLineLenLog2;

				dataPoUNum++;
				dataPoUCount+=cli.iSize;
				if (Info[KCacheInfoD_PoU].iLineLenLog2>cli.iLineLenLog2) Info[KCacheInfoD_PoU].iLineLenLog2=cli.iLineLenLog2;
				}
			}
		}
	
	__ASSERT_ALWAYS(codePoUNum && dataPoUNum && dataPoCNum,CACHEFAULT());
	//effective cache sizes used to calculate thresholds.
	codePoUCount/=codePoUNum;
	dataPoUCount/=dataPoUNum;
	dataPoCCount/=dataPoCNum;
		
	//Calculate line lenghts
	Info[KCacheInfoI].iLineLength = 1 << Info[KCacheInfoI].iLineLenLog2;
	Info[KCacheInfoD_PoU].iLineLength = 1 << Info[KCacheInfoD_PoU].iLineLenLog2;
	Info[KCacheInfoD].iLineLength = 1 << Info[KCacheInfoD].iLineLenLog2;
		
	//Calculate thresholds (counted in cache lines)
	Info[KCacheInfoI].iInvalidateThreshold = Info[KCacheInfoI].iCleanThreshold = Info[KCacheInfoI].iCleanAndInvalidateThreshold = (8 * codePoUCount) >> Info[KCacheInfoI].iLineLenLog2;
	Info[KCacheInfoD_PoU].iInvalidateThreshold = (4 * dataPoUCount) >> Info[KCacheInfoD_PoU].iLineLenLog2;
	Info[KCacheInfoD_PoU].iCleanThreshold = Info[KCacheInfoD_PoU].iCleanAndInvalidateThreshold = (3 * dataPoUCount) >> Info[KCacheInfoD_PoU].iLineLenLog2;
	Info[KCacheInfoD].iInvalidateThreshold = (4 * dataPoCCount) >> Info[KCacheInfoD].iLineLenLog2;
	Info[KCacheInfoD].iCleanThreshold = Info[KCacheInfoD].iCleanAndInvalidateThreshold = (3 * dataPoCCount) >> Info[KCacheInfoD].iLineLenLog2;

#ifdef _DEBUG
	__KTRACE_OPT(KMMU,Kern::Printf("Instr Cache:"));
	DumpCacheInfo(Info[KCacheInfoI]);
	__KTRACE_OPT(KMMU,Kern::Printf("Data PoU Cache:"));
	DumpCacheInfo(Info[KCacheInfoD_PoU]);
	__KTRACE_OPT(KMMU,Kern::Printf("Data PoC Cache:"));
	DumpCacheInfo(Info[KCacheInfoD]);
#endif
	}


#else // defined(__CPU_ARMV7)

#ifdef _DEBUG
void DumpCacheInfo(SCacheInfo& aCache)
	{
	SCacheInfo& c=aCache;
	__KTRACE_OPT(KBOOT,Kern::Printf("Size %04x LineLength %04x Assoc %04x",c.iSize,c.iLineLength,c.iAssoc));
	__KTRACE_OPT(KBOOT,Kern::Printf("InvalidateThreshold %04x CleanThreshold %04x CleanAndInvalidateThreshold %04x",c.iInvalidateThreshold,c.iCleanThreshold,c.iCleanAndInvalidateThreshold));
	__KTRACE_OPT(KBOOT,Kern::Printf("LineLenLog2 %02x PreemptBlock %02x",c.iLineLenLog2,c.iPreemptBlock));
	__KTRACE_OPT(KBOOT,Kern::Printf("CleanAndInvalidatePtr %08x CleanAndInvalidateMask %08x",c.iCleanAndInvalidatePtr,c.iCleanAndInvalidateMask));
	}
#endif // _DEBUG

void InternalCache::ParseCacheSizeInfo(TUint32 aValue, SCacheInfo& aInfo)
// Parse a size info field in the ARM cache type register
	{
	TUint sz=(aValue>>6)&7;
	TUint assoc=(aValue>>3)&7;
	TUint M=aValue & 4;
	TUint len=aValue&3;
	if (!M || assoc)
		{
		// cache present
		aInfo.iLineLenLog2=len+3;
		aInfo.iLineLength=(TUint16)(1u<<(len+3));
		aInfo.iSize=(TUint16)((0x40u<<sz)>>len);
		aInfo.iAssoc=(TUint16)(1u<<assoc);
		if (M)
			{
			aInfo.iSize+=((aInfo.iSize)>>1);
			aInfo.iAssoc+=((aInfo.iAssoc)>>1);
			}
		}
	
	//This is required for ARMv5 that CleanAndInvalidatees all cache on process switch
	aInfo.iPreemptBlock = (K::MaxMemCopyInOneGo/aInfo.iLineLength)<<1;
	aInfo.iPreemptBlock = (K::MaxMemCopyInOneGo/aInfo.iLineLength)<<1;
	}

void InternalCache::Init1()
// NOTE: K::MaxMemCopyInOneGo must be initialised before this function is called
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("InternalCache::Init1"));
	TUint32 ctr=TypeRegister();
	TUint32 ctype=(ctr>>25)&0x0f;
	TUint32 S=ctr&0x01000000;
	TUint32 Dsize=(ctr>>12)&0xfff;
	TUint32 Isize=ctr&0xfff;
#if defined(__CPU_ARMV6)
//	TUint32 alias=ctr&0x800800;	// check for 16K alias restriction
	__ASSERT_ALWAYS(ctype==6 || ctype==14,CACHEFAULT());	// writethrough only on V6? don't be silly...
#else
	__ASSERT_ALWAYS(ctype==2 || ctype==6 || ctype==7 || ctype==14,CACHEFAULT());
#endif

	__ASSERT_ALWAYS(S,CACHEFAULT());
	ParseCacheSizeInfo(Isize,Info[KCacheInfoI]);
	ParseCacheSizeInfo(Dsize,Info[KCacheInfoD]);

	// Calculate thresholds from cache size/type
	Info[KCacheInfoI].iInvalidateThreshold=8*Info[KCacheInfoI].iSize;
	Info[KCacheInfoI].iCleanThreshold=8*Info[KCacheInfoI].iSize;
	Info[KCacheInfoI].iCleanAndInvalidateThreshold=8*Info[KCacheInfoI].iSize;
	Info[KCacheInfoD].iInvalidateThreshold=4*Info[KCacheInfoD].iSize;
	Info[KCacheInfoD].iCleanThreshold=3*Info[KCacheInfoD].iSize;
	Info[KCacheInfoD].iCleanAndInvalidateThreshold=3*Info[KCacheInfoD].iSize;

	Info[KCacheInfoI].iCleanAndInvalidateMask=Info[KCacheInfoI].iSize*Info[KCacheInfoI].iLineLength/Info[KCacheInfoI].iAssoc;
	TUint x=__e32_find_ms1_32(256u/Info[KCacheInfoI].iAssoc);
	Info[KCacheInfoI].iCleanAndInvalidatePtr=0x01000000u<<x;
	
	Info[KCacheInfoD].iCleanAndInvalidateMask=Info[KCacheInfoD].iSize*Info[KCacheInfoD].iLineLength/Info[KCacheInfoD].iAssoc;
	x=__e32_find_ms1_32(256u/Info[KCacheInfoD].iAssoc);
	Info[KCacheInfoD].iCleanAndInvalidatePtr=0x01000000u<<x;

#ifdef _DEBUG
		__KTRACE_OPT(KBOOT,Kern::Printf("ICache:\n"));
		DumpCacheInfo(Info[KCacheInfoD]);
		__KTRACE_OPT(KBOOT,Kern::Printf("DCache:\n"));
		DumpCacheInfo(Info[KCacheInfoD]);
#endif
#if !defined(__CPU_MEMORY_TYPE_REMAPPING)
	// Simulate PRRR & NRRR MMU registers on platforms without memory remap feature.  
	// "Magic" numbers come from the description of ARM core's remap registers.  
	TUint32 platformSpecificMappings = TheSuperPage().iPlatformSpecificMappings;
	iPrimaryRegionRemapRegister	= KDefaultPrimaryRegionRemapRegister | ((platformSpecificMappings&0x3f)<<10);
	iNormalMemoryRemapRegister = KDefaultNormalMemoryRemapRegister | (platformSpecificMappings&0xfc00fc00);	
#endif //!defined(__CPU_MEMORY_TYPE_REMAPPING)
	}

#if !defined(__CPU_MEMORY_TYPE_REMAPPING)
TUint32 InternalCache::PrimaryRegionRemapRegister()
    {
    return iPrimaryRegionRemapRegister;
    }

TUint32 InternalCache::NormalMemoryRemapRegister()
    {
    return iNormalMemoryRemapRegister;
    }
#endif //!defined(__CPU_MEMORY_TYPE_REMAPPING)

#endif //else defined(__CPU_ARMV7)

#ifdef __BROADCAST_CACHE_MAINTENANCE__

void InternalCache::LocalInvalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::LocalInvalidate %x %08x+%08x",aMask,aBase,aSize));
#else
void InternalCache::Invalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::Invalidate %x %08x+%08x",aMask,aBase,aSize));
#endif
	if(!aSize)
		return;

	if (aMask&KCacheSelectD)
		{
    #if  defined(__CPU_ARMV7) && defined(__SMP__)
		// Do not check threshold, as we cannot maintain entire DCaches on SMP ARMv7 H/W
		// because there is no available/reliable way to do that.
        Invalidate_DCache_Region(aBase,aSize);
    #else		
		if (aSize>=Info[KCacheInfoD].InvalidateThresholdBytes())
			CleanAndInvalidate_DCache_All();
		else
			Invalidate_DCache_Region(aBase,aSize);
    #endif //defined(__CPU_ARMV7) && defined(__SMP__)
        }
	
	if (aMask&KCacheSelectI)
		{
		if (aSize>=Info[KCacheInfoI].InvalidateThresholdBytes())
			Invalidate_ICache_All();
		else
			Invalidate_ICache_Region(aBase,aSize);
	    
        #if defined(__BROADCAST_ISB)
		T_ISB_IPI isbipi;
		isbipi.Do();
        #endif  // __BROADCAST_ISB
		}
	}

void InternalCache::Invalidate_DCache_Reverse(TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::Invalidate_DCache_Reverse %x %08x+%08x",aBase,aSize));
	if(!aSize)
		return;
#if  defined(__CPU_ARMV7)
	// Do not check threshold here because:
	// 	- There is no relible way to broadcast CleanAndInvalidate_DCache_All
	//  - CleanAndInvalidate_DCache_All maintains cache starting from the level which
	//    is the closest to CPU.
	
	// On ARMv7, this maintains all levels of cache at the same time.
	Invalidate_DCache_Region(aBase,aSize);
#else //defined(__CPU_ARMV7)
	
	//There are no multilevel internal caches, so the ordinary Invalidate will do here.
	Invalidate(KCacheSelectD, aBase, aSize);
#endif // else defined(__CPU_ARMV7)
	}

#ifdef __BROADCAST_CACHE_MAINTENANCE__
void InternalCache::LocalClean(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::LocalClean %x %08x+%08x",aMask,aBase,aSize));
#else
void InternalCache::Clean(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::Clean %x %08x+%08x",aMask,aBase,aSize));
#endif
	if(!aSize)
		{
		DrainBuffers();
		return;
		}

#if  defined(__CPU_ARMV7) && defined(__SMP__)
	// Do not check threshold, as we cannot maintain entire DCaches on SMP ARMv7 H/W,
	// as there is no available/reliable way to do that.
	if (aMask&KCacheSelectD)
		Clean_DCache_Region(aBase,aSize);
#else //defined(__CPU_ARMV7) && defined(__SMP__)
	if (aMask&KCacheSelectD)
		{
		if (aSize>=Info[KCacheInfoD].CleanThresholdBytes())
			{
			Clean_DCache_All();
			}
		else
			Clean_DCache_Region(aBase,aSize);
		}
#endif // else defined(__CPU_ARMV7) && defined(__SMP__)	
	}

#ifdef __BROADCAST_CACHE_MAINTENANCE__
void InternalCache::LocalCleanAndInvalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::LocalCleanAndInvalidate %x %08x+%08x",aMask,aBase,aSize));
#else
void InternalCache::CleanAndInvalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::CleanAndInvalidate %x %08x+%08x",aMask,aBase,aSize));
#endif
	if(!aSize)
		{
		DrainBuffers();
		return;
		}

	if (aMask&KCacheSelectD)
		{
#if  defined(__CPU_ARMV7) && defined(__SMP__)
	// Do not check threshold, as we cannot maintain entire DCaches on SMP ARMv7 H/W,
	// as there is no available/reliable way to do that.
		CleanAndInvalidate_DCache_Region(aBase,aSize);
#else
		if (aSize>=Info[KCacheInfoD].CleanAndInvalidateThresholdBytes())
			{
			CleanAndInvalidate_DCache_All();
			}
		else
			CleanAndInvalidate_DCache_Region(aBase,aSize);
#endif //else defined(__CPU_ARMV7) && defined(__SMP__)
		}
	if (aMask&KCacheSelectI)
		{
		if (aSize>=Info[KCacheInfoI].CleanAndInvalidateThresholdBytes())
			Invalidate_ICache_All();
		else
			Invalidate_ICache_Region(aBase,aSize);
	    
		#if defined(__BROADCAST_ISB)
		T_ISB_IPI isbipi;
		isbipi.Do();
		#endif  // __BROADCAST_ISB
		}
	}

#if defined(__CPU_ARMV7)
void InternalCache::CleanPoU(TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::CleanPoU %08x+%08x",aBase,aSize));
	if(!aSize)
		{
		DrainBuffers();
		return;
		}

#if  defined(__SMP__)
	// Do not check threshold, as we cannot maintain entire DCaches on SMP ARMv7 H/W,
	// as there is no available/reliable way to do that.
	Clean_PoU_DCache_Region(aBase,aSize);
#else //defined(__SMP__)
	if (aSize>=Info[KCacheInfoD_PoU].CleanThresholdBytes())
		{
		Clean_PoU_DCache_All();
		}
	else
		Clean_PoU_DCache_Region(aBase,aSize);
#endif //else defined(__SMP__)
	}
#endif // defined(__CPU_ARMV7)	


TUint32 InternalCache::TypeToCachingAttributes(TMemoryType aType)
	{	
	TUint32 attr = 0;
	switch (aType)
		{
		case EMemAttStronglyOrdered:attr = EMapAttrFullyBlocking;break;
		case EMemAttDevice:			attr = EMapAttrBufferedNC;	 break;
		case EMemAttNormalUncached: attr = EMapAttrL1Uncached;   break;
		case EMemAttNormalCached:	attr = EMapAttrCachedMax;	 break;

		case EMemAttKernelInternal4:
		case EMemAttPlatformSpecific5:
		case EMemAttPlatformSpecific6:
		case EMemAttPlatformSpecific7:
			{
			// The mapping of these types are defined in bootstrap.
			// Read remap registers and set mapping attributes accordingly.
			// "Magic" numbers come from the description of ARM core's remap registers.  
			TInt prrr = PrimaryRegionRemapRegister();
			prrr = (prrr >> (2*aType)) & 3; //prrr value the for given mapping
			switch (prrr)
				{
				case 0: // strongly ordered
					attr = EMapAttrFullyBlocking; break;
				case 1:	// device
					attr = EMapAttrBufferedNC; break;
				case 2: // normal
					{
					TInt nmrr = NormalMemoryRemapRegister(); 
					switch ((nmrr >> (2*aType)) & 3)		//inner cache
						{
						case 0: attr = EMapAttrL1Uncached;  break;
						case 1: attr = EMapAttrL1CachedMax; break;
						case 2: attr = EMapAttrCachedWTRA;  break;
						case 3: attr = EMapAttrCachedWBRA;  break;
						}
					switch ((nmrr >> (2*aType + 16)) & 3)	//outer cache
						{
						case 0: attr |= EMapAttrL2Uncached;  break;
						case 1: attr |= EMapAttrL2CachedMax; break;
						case 2: attr |= EMapAttrL2CachedWTRA;break;
						case 3: attr |= EMapAttrL2CachedWBRA;break;
						}
					}
					break;
				default:
					CACHEFAULT(); //unsupported value (3) in Primary Region Remap Register
				}
			}
			break;
		default: 
			CACHEFAULT(); //invalid value of aType
		}
	__KTRACE_OPT(KMMU,Kern::Printf("InternalCache::TypeToCachingAttributes type:%x ret:%08x",aType,attr));
	return attr;
	}

#ifdef __BROADCAST_CACHE_MAINTENANCE__
class TCacheBroadcast : public TGenericIPI
	{
public:
	typedef void (*TCacheFn)(TUint,TLinAddr,TUint);
public:
	TCacheBroadcast(TCacheFn aFn, TUint aMask, TLinAddr aBase, TUint aSize);
	static void Isr(TGenericIPI*);
	void DoIsrOp();
	void DoThreadOp();
public:
	TCacheFn		iFn;
	TUint			iMask;
	TLinAddr		iBase;
	TUint			iSize;
	};

TCacheBroadcast::TCacheBroadcast(TCacheFn aFn, TUint aMask, TLinAddr aBase, TUint aSize)
	:	iFn(aFn), iMask(aMask), iBase(aBase), iSize(aSize)
	{
	}

void TCacheBroadcast::Isr(TGenericIPI* aPtr)
	{
	TCacheBroadcast& a = *(TCacheBroadcast*)aPtr;
	(*a.iFn)(a.iMask, a.iBase, a.iSize);
	}

void TCacheBroadcast::DoIsrOp()
	{
	NKern::Lock();
	QueueAllOther(&Isr);
	Isr(this);
	WaitCompletion();
	NKern::Unlock();
	}

void TCacheBroadcast::DoThreadOp()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TCacheBroadcast::DoThreadOp");
	TCoreCycler cycler;
	while (cycler.Next()==KErrNone)
		{
		Isr(this);
		}
	}

void InternalCache::Invalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::Invalidate %x %08x+%08x",aMask,aBase,aSize));
	if (!aSize) return;
	TCacheBroadcast cb(&InternalCache::LocalInvalidate, aMask, aBase, aSize);
	if (aSize<=256 && aBase>=0x80000000u && aSize<=~aBase)
		{
		cb.DoIsrOp();
		return;
		}
	cb.DoThreadOp();
	}

void InternalCache::Clean(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::Clean %x %08x+%08x",aMask,aBase,aSize));
	if (!aSize) return;
	TCacheBroadcast cb(&InternalCache::LocalClean, aMask, aBase, aSize);
	if (aSize<=256 && aBase>=0x80000000u && aSize<=~aBase)
		{
		cb.DoIsrOp();
		return;
		}
	cb.DoThreadOp();
	}

void InternalCache::CleanAndInvalidate(TUint aMask, TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::CleanAndInvalidate %x %08x+%08x",aMask,aBase,aSize));
	if (!aSize) return;
	TCacheBroadcast cb(&InternalCache::LocalCleanAndInvalidate, aMask, aBase, aSize);
	if (aSize<=256 && aBase>=0x80000000u && aSize<=~aBase)
		{
		cb.DoIsrOp();
		return;
		}
	cb.DoThreadOp();
	}
#endif	//	__BROADCAST_CACHE_MAINTENANCE__

#if  defined(__BROADCAST_ISB)
T_ISB_IPI::T_ISB_IPI()
    {
    }

void T_ISB_IPI::Do()
    {
    NKern::Lock();
    QueueAllOther(&ISBIsr);
    ISBIsr(this);
    WaitCompletion();
    NKern::Unlock();
    }
#endif //defined(__BROADCAST_ISB)


