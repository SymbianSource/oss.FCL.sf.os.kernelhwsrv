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
// e32\kernel\win32\cache.cpp
// 
//

#include <win32.h>
#include <kernel/cache.h>

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
