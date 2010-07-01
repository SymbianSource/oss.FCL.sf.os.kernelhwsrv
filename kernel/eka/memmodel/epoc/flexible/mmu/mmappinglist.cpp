// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <plat_priv.h>
#include "mm.h"
#include "mmu.h"

#include "mmappinglist.h"
#include "mmapping.h"


TSpinLock TMappingListBase::iSpinLock(TSpinLock::EOrderGenericIrqHigh3);


//
// TMappingList
//

void TMappingList::Add(DMemoryMappingBase* aMapping)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());
	aMapping->iLink.InsertBefore(this);
	}


void TMappingList::Remove(DMemoryMappingBase* aMapping)
	{
	__NK_ASSERT_DEBUG(LockIsHeld());
	aMapping->iLink.Remove();
	}


// Utility methods

void TMappingList::RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB)
	{
	Lock();
	TMappingListIter iter;
	DMemoryMappingBase* mapping = iter.Start(*this);
	while(mapping)
		{		
		TUint start = aIndex-mapping->iStartIndex;
		if(start < mapping->iSizeInPages && !mapping->IsPinned())
			{
			// remap the page in the mapping...
			mapping->Open();
			TUint mapInstanceCount = mapping->MapInstanceCount();
			Unlock();
			mapping->RemapPage(aPageArray, aIndex, mapInstanceCount, aInvalidateTLB);
			mapping->AsyncClose();
			}
		else
			{
			// the mapping doesn't contain the page or is a pinned mapping and therefore doesn't own
			// any page tables...
			Unlock();
			}
		Lock();
		mapping = iter.Next();
		}
	iter.Finish();
	Unlock();
	}


//
// TMappingListIter
//

DMemoryMappingBase* TMappingListIter::Start(TMappingList& aList)
	{
	TRACE2(("TMappingListIter::Start"));
	__NK_ASSERT_DEBUG(LockIsHeld());
	return Next(&aList);
	}


void TMappingListIter::Finish()
	{
	TRACE2(("TMappingListIter::Finish"));
	__NK_ASSERT_DEBUG(LockIsHeld());
	Remove();
	}


DMemoryMappingBase* TMappingListIter::Next(TMappingListBase* aPrev)
	{
	// avoid live-lock by only flashing iSpinLock after a number of attempts greater
	// than the number of CPU cores, that way each core is guaranteed to make some headway...
	const TUint KFlashPeriod = KMaxCpus+1;
	__ASSERT_COMPILE(KFlashPeriod<10); // make sure limit is sensible, if not, will need more complex live-lock avoidance
	TUint flash = 0;

	TMappingListBase* prev = aPrev;
	TMappingListBase* next = prev->iNext;
	for(;;)
		{
		TUint type = next->iType;

		// at end of list?
		if(type==EHead)
			{
			// yes, so put iterator back at end...
			prev->iNext = this;
			next->iPrev = this;
			iNext = next;
			iPrev = prev;
			// return NULL...
			TRACE2(("TMappingListIter::Next returns 0"));
			return 0;
			}

		// move past next...
		prev = next;
		next = next->iNext;

		// have we found a mapping?
		if(type==ELink)
			{
			// yes, so insert iterator after it...
			prev->iNext = this;
			next->iPrev = this;
			iNext = next;
			iPrev = prev;
			// return mapping...
			DMemoryMappingBase* mapping = (DMemoryMappingBase*)((TLinAddr)prev-_FOFF(DMemoryMappingBase,iLink));
			TRACE2(("TMappingListIter::Next returns 0x%08x",mapping));
			return mapping;
			}

		// move on to next...
		TRACE2(("TMappingListIter::Next skip %d",next->iType));

		// but first see if we need to flash lock...
		if(++flash>=KFlashPeriod)
			{
			// we need to flash the lock, but first put iterator back in list so we don't loose our place...
			prev->iNext = this;
			next->iPrev = this;
			iNext = next;
			iPrev = prev;
			// flash lock...
			__SPIN_FLASH_IRQ(iSpinLock);
			flash = 0;
			// remove iterator again...
			next = iNext;
			prev = iPrev;
			prev->iNext = next;
			next->iPrev = prev;
			}

		// loop back and try next
		}
	}


DMemoryMappingBase* TMappingListIter::Next()
	{
	__NK_ASSERT_DEBUG(LockIsHeld());

	TMappingListBase* next = iNext;
	TMappingListBase* prev = iPrev;
	TUint type = next->iType;

	// at end of list?
	if(type==EHead)
		{
		// yes, return NULL...
		TRACE2(("TMappingListIter::Next returns 0"));
		return 0;
		}

	// unlink iterator...
	prev->iNext = next;
	next->iPrev = prev;

	// have we found a mapping?
	if(type==ELink)
		{
		// yes, so insert iterator after it...
		prev = next;
		next = next->iNext;
		prev->iNext = this;
		next->iPrev = this;
		iNext = next;
		iPrev = prev;
		// return mapping...
		DMemoryMappingBase* mapping = (DMemoryMappingBase*)((TLinAddr)prev-_FOFF(DMemoryMappingBase,iLink));
		TRACE2(("TMappingListIter::Next returns 0x%08x",mapping));
		return mapping;
		}

	// handle more complicated situations...
	return Next(next);
	}



