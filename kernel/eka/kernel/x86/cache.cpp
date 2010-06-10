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
// e32\kernel\x86\cache.cpp
// 
//

#include <x86.h>
#include <kernel/cache.h>
#include "cache_maintenance.h"

#ifdef __CPU_HAS_CACHE

#define CACHEFAULT()	CacheFault(__LINE__)

void CacheFault(TInt aLine)
	{
	Kern::Fault("CacheFault",aLine);
	}

EXPORT_C void Cache::IMB_Range(TLinAddr, TUint)
	{
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaWrite(TLinAddr, TUint)
	{
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaRead(TLinAddr, TUint)
	{
	}

EXPORT_C void Cache::SyncMemoryAfterDmaRead(TLinAddr, TUint)
	{
	}

EXPORT_C void Cache::AtomicSyncMemory()
	{
	}

EXPORT_C void Cache::CpuRetires()
	{
	}

EXPORT_C void Cache::KernelRetires()
	{
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaWrite(TLinAddr, TUint, TUint32)
	{
	}

EXPORT_C void Cache::SyncMemoryBeforeDmaRead(TLinAddr, TUint, TUint32)
	{
	}

EXPORT_C void Cache::SyncMemoryAfterDmaRead(TLinAddr, TUint, TUint32)
	{
	}

EXPORT_C TInt Cache::GetThresholds(TCacheThresholds&, TUint)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt Cache::SetThresholds(const TCacheThresholds&, TUint)
	{
	return KErrNotSupported;
	}

EXPORT_C TUint Cache::DmaBufferAlignment()
	{
	return 1;
	}

void CacheMaintenance::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("CacheMaintenance::Init1"));
	}

TBool CacheMaintenance::PageToReuse (TLinAddr, TMemoryType, TPhysAddr, TInt)
	{
	return EFalse;
	}

void CacheMaintenance::PageToPreserveAndReuse (	TLinAddr, TMemoryType, TPhysAddr aPhysAddr)
	{
	}

TMemoryType CacheMaintenance::TemporaryMapping()
	{
	return EMemAttNormalCached;
	}

void CacheMaintenance::CodeChanged(TLinAddr, TUint, TCodeChangedBy)
	{
	}

void CacheMaintenance::MakeCPUChangesVisible(TLinAddr, TUint, TUint32, TPhysAddr)
	{
	}

void CacheMaintenance::PrepareMemoryForExternalWrites(TLinAddr, TUint, TUint32, TPhysAddr)
	{
	}

void CacheMaintenance::MakeExternalChangesVisible(TLinAddr, TUint, TUint32, TPhysAddr)
	{
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

#if defined(__MEMMODEL_MULTIPLE__)

void CacheMaintenance::MemoryToPreserveAndReuse(TLinAddr, TUint, TUint32)
	{
	}

void CacheMaintenance::SyncPhysicalCache_All()
	{
	}
#endif //defined(__MEMMODEL_MULTIPLE__)

TUint32 InternalCache::TypeToCachingAttributes(TMemoryType)
	{
	return EMapAttrCachedMax;
	}
#endif //__CPU_HAS_CACHE
