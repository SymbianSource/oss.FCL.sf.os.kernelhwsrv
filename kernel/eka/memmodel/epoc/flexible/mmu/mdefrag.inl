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
// Description: mdefrag.inl
//				In line utility functions used by the defragbase.cpp and ramalloc.cpp

/**
 @file
 @internalComponent
*/

#include "mm.h"
#include "mmu.h"
#include "mpager.h"


inline TInt M::PageShift()
	{
	return KPageShift;
	}


inline void M::RamAllocLock()
	{
	RamAllocLock::Lock();
	}


inline void M::RamAllocUnlock()
	{
	RamAllocLock::Unlock();
	}


inline void M::RamAllocIsLocked()
	{
#ifdef _DEBUG
	if (!K::Initialising) 
		__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
#endif
	}


inline TInt M::DiscardPage(TPhysAddr aAddr, TUint aBlockZoneId, TUint aMoveDisFlags)
	{
	TPhysAddr newAddr;
	return MovePage(aAddr, newAddr, aBlockZoneId, aMoveDisFlags);
	}


inline TUint M::NumberOfFreeDpPages()
	{
	return ThePager.NumberOfFreePages();
	}


inline TUint M::NumberOfDirtyDpPages()
	{
	return ThePager.NumberOfDirtyPages();
	}


inline void M::RamZoneClaimed(SZone* aZone)
	{
	TheMmu.AllocatedPhysicalRam(aZone->iPhysBase, 
								aZone->iPhysPages,
								(Mmu::TRamAllocFlags)EMemAttStronglyOrdered);
	}


inline TBool M::GetFreePages(TUint aNumPages)
	{
	return ThePager.GetFreePages(aNumPages);
	}
