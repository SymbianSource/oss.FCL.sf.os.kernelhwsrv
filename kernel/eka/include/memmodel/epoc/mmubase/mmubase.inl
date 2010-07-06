// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\epoc\mmubase\mmubase.inl
// 
//

#ifndef __MMUBASE_INL__
#define __MMUBASE_INL__

#include <mmboot.h>

const TInt KPageInfosPerPageShift = KPageShift-KPageInfoShift;
const TInt KPageInfosPerPage = 1<<KPageInfosPerPageShift;
const TInt KNumPageInfoPagesShift = 32-KPageShift-KPageInfosPerPageShift;
const TInt KNumPageInfoPages = 1<<KNumPageInfoPagesShift;

inline SPageInfo* SPageInfo::FromPhysAddr(TPhysAddr aAddress)
	{
	return ((SPageInfo*)KPageInfoLinearBase)+(aAddress>>KPageShift);
	}

inline TPhysAddr SPageInfo::PhysAddr()
	{
	return ((TPhysAddr)this)<<KPageInfosPerPageShift;
	}


//
// M class implementations of ram allocator and defrag methods
// Make these inline as they are just memory model independent interface
// for the ram allocator and defrag classes.
//

inline TInt M::PageShift()
	{
	return KPageShift;
	}


inline void M::RamAllocLock()
	{
	MmuBase::Wait();
	}


inline void M::RamAllocUnlock()
	{
	MmuBase::Signal();
	}


inline void M::RamAllocIsLocked()
	{
#ifdef _DEBUG
	if (!K::Initialising) 
		__ASSERT_MUTEX(MmuBase::RamAllocatorMutex);
#endif
	}


inline TUint M::NumberOfFreeDpPages()
	{
	return Mmu::Get().NumberOfFreeDpPages();
	}


inline TUint M::NumberOfDirtyDpPages()
	{// This memory model doesn't support data paging so can't get dirty paged pages.
	return 0;
	}


inline TInt M::MovePage(TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TUint aMoveDisFlags)
	{
	return Mmu::Get().MovePage(	aOld, aNew, aBlockZoneId, 
								(aMoveDisFlags & M::EMoveDisBlockRest)!=0);
	}


inline TInt M::DiscardPage(TPhysAddr aAddr, TUint aBlockZoneId, TUint aMoveDisFlags)
	{
	return Mmu::Get().DiscardPage(	aAddr, aBlockZoneId, 
									(aMoveDisFlags & M::EMoveDisBlockRest)!=0);
	}
#endif


