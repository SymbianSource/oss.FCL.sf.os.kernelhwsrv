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
// eka\kernel\arm\cache.cpp
// 
//

#include <arm.h>
#include <kernel/cache.h>
#include "cache_maintenance.h"

EXPORT_C void Cache::IMB_Range(TLinAddr aBase, TUint aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::IMB_Range");
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::IMB_Range %08x+%08x",aBase,aSize));
	CacheMaintenance::CodeChanged(aBase,aSize);
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaWrite(TLinAddr aBase, TUint aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryBeforeDmaWrite");				
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryBeforeDmaWrite %08x+%08x",aBase,aSize));
	CacheMaintenance::MakeCPUChangesVisible(aBase, aSize, EMapAttrCachedMax);
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaWrite(TLinAddr aBase, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryBeforeDmaWrite");				
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryDeforeDmaWrite %08x+%08x attr=%08x",aBase,aSize,aMapAttr));
	CacheMaintenance::MakeCPUChangesVisible(aBase, aSize, aMapAttr);
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaRead(TLinAddr aBase, TUint aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryBeforeDmaRead");				
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryBeforeDmaRead %08x+%08x",aBase,aSize));
	CacheMaintenance::PrepareMemoryForExternalWrites(aBase, aSize, EMapAttrCachedMax);
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaRead(TLinAddr aBase, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryBeforeDmaRead");				
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryBeforeDmaRead %08x+%08x attr=%08x",aBase,aSize,aMapAttr));
	CacheMaintenance::PrepareMemoryForExternalWrites(aBase, aSize, aMapAttr);
	}

EXPORT_C void Cache::SyncMemoryAfterDmaRead(TLinAddr aBase, TUint aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryAfterDmaRead");
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryAfterDmaRead %08x+%08x",aBase,aSize));
	CacheMaintenance::MakeExternalChangesVisible(aBase, aSize, EMapAttrCachedMax);
	}

EXPORT_C void Cache::SyncMemoryAfterDmaRead(TLinAddr aBase, TUint aSize, TUint32 aMapAttr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Cache::SyncMemoryAfterDmaRead");
	__KTRACE_OPT(KMMU,Kern::Printf("Cache::SyncMemoryBeforeDmaRead %08x+%08x attr=%08x",aBase,aSize,aMapAttr));
	CacheMaintenance::MakeExternalChangesVisible(aBase, aSize, aMapAttr);
	}

EXPORT_C void Cache::AtomicSyncMemory()
	{
	// This methos is called during reboot or power down sequence and therefore is not allowed
	// to do Kernel calls that may hold spin lock - such as Kern::Print or precondition checkings.
	// CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"Cache::AtomicSyncMemory");				
	// __KTRACE_OPT(KMMU,Kern::Printf("Cache::AtomicSyncMemory"));

	InternalCache::CleanAndInvalidate_DCache_All();

#if defined(__HAS_EXTERNAL_CACHE__)
	ExternalCache::AtomicSync();
#endif //__HAS_EXTERNAL_CACHE__
	}

EXPORT_C void Cache::CpuRetires()
	{
	// This methos is called during reboot or power down sequence and therefore is not allowed
	// to do Kernel calls that may hold spin lock - such as Kern::Print or precondition checkings.
	// CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"Cache::CpuRetires");				

	InternalCache::CleanAndInvalidate_DCache_All();

#if  !defined(__SMP__) & defined(__HAS_EXTERNAL_CACHE__)
	ExternalCache::AtomicSync();
#endif	
	}

EXPORT_C void Cache::KernelRetires()
	{
	// This methos is called during reboot or power down sequence and therefore is not allowed
	// to do Kernel calls that may hold spin lock - such as Kern::Print or precondition checkings.
	// CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"Cache::KernelRetires");				

#if  defined(__SMP__) & defined(__HAS_EXTERNAL_CACHE__)
	ExternalCache::AtomicSync();
#endif
	}

EXPORT_C TInt Cache::GetThresholds(TCacheThresholds& aThresholds, TUint aCacheType)
	{
	SCacheInfo* info;
	switch (aCacheType)
		{
		case KCacheSelectI: 	info = &InternalCache::Info[KCacheInfoI]; break;
		case KCacheSelectD: 	info = &InternalCache::Info[KCacheInfoD]; break;
#if defined(__CPU_ARMV7)
		case KCacheSelectD_IMB: info = &InternalCache::Info[KCacheInfoD_PoU]; break;
#endif	//(__CPU_ARMV7)
#if defined (__HAS_EXTERNAL_CACHE__)
		case KCacheSelect_L2: 	info = &ExternalCache::Info; break;
#endif //(__HAS_EXTERNAL_CACHE__)
		default: 				return KErrNotSupported;
		}
	aThresholds.iPurge = info->InvalidateThresholdBytes();
	aThresholds.iClean = info->CleanThresholdBytes();
	aThresholds.iFlush = info->CleanAndInvalidateThresholdBytes();

	return KErrNone;
	}

EXPORT_C TInt Cache::SetThresholds(const TCacheThresholds& aThresholds, TUint aCacheType)
	{
	SCacheInfo* info;
	switch (aCacheType)
		{
		case KCacheSelectI: 	info = &InternalCache::Info[KCacheInfoI]; break;
		case KCacheSelectD: 	info = &InternalCache::Info[KCacheInfoD]; break;
#if defined(__CPU_ARMV7)
		case KCacheSelectD_IMB: info = &InternalCache::Info[KCacheInfoD_PoU]; break;
#endif	// (__CPU_ARMV7)
#if defined (__HAS_EXTERNAL_CACHE__)
		case KCacheSelect_L2: 	info = &ExternalCache::Info; break;
#endif //(__HAS_EXTERNAL_CACHE__)
		default: 				return KErrNotSupported;
		}
	info->iInvalidateThreshold = aThresholds.iPurge >> info->iLineLenLog2;
	info->iCleanThreshold = aThresholds.iClean >> info->iLineLenLog2;
	info->iCleanAndInvalidateThreshold = aThresholds.iFlush >> info->iLineLenLog2;

	return KErrNone;
	}

EXPORT_C TUint Cache::DmaBufferAlignment()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD, "Cache::DmaBufferAlignment");
	__KTRACE_OPT(KMMU, Kern::Printf("Cache::DmaBufferAlignment"));
#if defined(__CPU_ARMV7)
	TUint len = 1<<InternalCache::DmaBufferAlignementLog2;
#else
	TUint len = InternalCache::Info[KCacheInfoD].iLineLength;	
#endif
#ifdef __HAS_EXTERNAL_CACHE__
	TUint ext_len = ExternalCache::Info.iLineLength;
	len = (ext_len > len) ? ext_len : len;
#endif
	return len;
	}
