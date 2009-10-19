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

#include "mslaballoc.h"



//
// RSlabAllocatorBase
//

RSlabAllocatorBase::RSlabAllocatorBase(TBool aDelayedCleanup)
	:	iFreeCount(0), iReserveCount(0),
		iSpinLock(TSpinLock::EOrderGenericIrqHigh3),
		iDelayedCleanup(aDelayedCleanup), iSlabMap(0), iMemory(0), iMapping(0)
	{
	}


RSlabAllocatorBase::~RSlabAllocatorBase()
	{
	delete iSlabMap;
	MM::MappingDestroy(iMapping);
	MM::MemoryDestroy(iMemory);
	}


TInt RSlabAllocatorBase::Construct(TUint aMaxSlabs, TUint aObjectSize)
	{
	return Construct(aMaxSlabs,aObjectSize,0);
	}


TInt RSlabAllocatorBase::Construct(TUint aMaxSlabs, TUint aObjectSize, TLinAddr aBase)
	{
	TRACE2(("RSlabAllocatorBase::Construct(0x%08x,0x%08x,0x%08x)",aMaxSlabs,aObjectSize,aBase));

	// set sizes...
	iObjectSize = aObjectSize;
	iObjectsPerSlab = ((KPageSize-sizeof(TSlabHeader))/aObjectSize);

	// sanity check arguments...
	__NK_ASSERT_DEBUG(iObjectsPerSlab);
	__NK_ASSERT_DEBUG(aObjectSize>=sizeof(SDblQueLink));
	__NK_ASSERT_DEBUG(aObjectSize%sizeof(TAny*)==0);

	// construct bitmap for slabs...
	iSlabMap = TBitMapAllocator::New(aMaxSlabs,ETrue);
	if(!iSlabMap)
		return KErrNoMemory;

	if(aBase)
		{
		// setup base address, we expect one slab to already be mapped at this address...
		iBase = aBase;

		// initialise first slab...
		iSlabMap->Alloc(0,1);
		InitSlab(iBase);
		}
	else
		{
		// construct memory object for slabs...
		__NK_ASSERT_DEBUG(!iMemory);
		TInt r = MM::MemoryNew(iMemory,EMemoryObjectUnpaged,aMaxSlabs);
		if(r!=KErrNone)
			return r;

		// construct memory mapping for slabs...
		r = MM::MappingNew(iMapping,iMemory,ESupervisorReadWrite,KKernelOsAsid);
		if(r!=KErrNone)
			return r;

		// setup base address...
		iBase = MM::MappingBase(iMapping);
		}

	// done...
	return KErrNone;
	}


TAny* RSlabAllocatorBase::Alloc()
	{
#ifdef _DEBUG
	RamAllocLock::Lock();
	TBool fail = K::CheckForSimulatedAllocFail();
	RamAllocLock::Unlock();
	if(fail)
		return 0;
#endif
	__SPIN_LOCK_IRQ(iSpinLock);

	// check if we need to allocate a new slab...
	if(iFreeCount<=iReserveCount && !NewSlab())
		{
		__SPIN_UNLOCK_IRQ(iSpinLock);
		return 0;
		}

	// get a slab with unused objects...
	TSlabHeader* slab = (TSlabHeader*)iFreeList.iA.iNext;
	__NK_ASSERT_DEBUG(slab!=(TSlabHeader*)&iFreeList.iA.iNext);
#ifdef _DEBUG
	CheckSlab(slab);
#endif

	// get object from slab...
	SDblQueLink* object = (SDblQueLink*)slab->iFreeList.iA.iNext;
	TRACE2(("RSlabAllocatorBase::Alloc got 0x%08x",object));
	object->Deque();
	__NK_ASSERT_DEBUG(slab->iAllocCount<iObjectsPerSlab);
	++slab->iAllocCount;
	--iFreeCount;

	// see if there are uninitialised free objects after the one just allocated...
	if(slab->iHighWaterMark==object)
		{
		SDblQueLink* nextFree = (SDblQueLink*)((TLinAddr)object+iObjectSize);
		if((TAny*)((TLinAddr)nextFree+iObjectSize)<=slab)
			{
			slab->iHighWaterMark = nextFree;
			slab->iFreeList.Add(nextFree);
			}
		}

	// if slab has no more free objects, remove it from the free list...
	if(slab->iFreeList.iA.iNext==&slab->iFreeList.iA)
		slab->Deque();

	__SPIN_UNLOCK_IRQ(iSpinLock);

	return object;
	}


void RSlabAllocatorBase::Free(TAny* aObject)
	{
	TRACE2(("RSlabAllocatorBase::Free(0x%08x)",aObject));

	if(!aObject)
		{
		// nothing to do
		return; 
		}

	__SPIN_LOCK_IRQ(iSpinLock);

	// check object address is valid...
	__NK_ASSERT_DEBUG((TLinAddr)aObject-iBase < iSlabMap->iSize*(TLinAddr)KPageSize); // in range
	__NK_ASSERT_DEBUG(((TLinAddr)aObject&KPageMask)%iObjectSize==0); // aligned correctly
	__NK_ASSERT_DEBUG(((TLinAddr)aObject&KPageMask)<iObjectSize*iObjectsPerSlab); // in slab

	// get slab for object...
	TSlabHeader* slab = (TSlabHeader*)(((TLinAddr)aObject|KPageMask)+1)-1;
#ifdef _DEBUG
	CheckSlab(slab);
#endif

	// if slab didn't previously have any free objects, add it to the free list...
	if(slab->iFreeList.iA.iNext==&slab->iFreeList.iA)
		iFreeList.AddHead(slab);

	// add object to slab's free list...
	slab->iFreeList.AddHead((SDblQueLink*)(TAny*)aObject);
	TUint allocCount = --slab->iAllocCount;
	__NK_ASSERT_DEBUG(allocCount<iObjectsPerSlab);
	++iFreeCount;

	if(!allocCount)
		{
		// if slab is empty, put it on end of free list...
		slab->Deque();
		iFreeList.Add(slab);
		}
	else
		{
		// migrate slab to try and keep fuller slabs near the free list start...
		TSlabHeader* nextSlab = (TSlabHeader*)slab->iNext;
		if(nextSlab!=(SDblQueLink*)&iFreeList && allocCount<=nextSlab->iAllocCount)
			{
			slab->Deque();
			slab->InsertAfter(nextSlab);
			}
		}

#ifdef _DEBUG
	CheckSlab(slab);
#endif

	// check for spare empty slab...
	TSlabHeader* lastSlab = (TSlabHeader*)iFreeList.iA.iPrev;
	if(lastSlab->iNext!=lastSlab->iPrev && lastSlab->iAllocCount==0) // not only slab and it's empty...
		{
		// free up slab...
		if(!iDelayedCleanup)
			{
			// free up slab now, (this also relinquishes iSpinLock)...
			FreeSlab(lastSlab);
			}
		else
			{
			// queue later cleanup...
			__SPIN_UNLOCK_IRQ(iSpinLock);
			iCleanup.Add(CleanupTrampoline,this);
			}
		}
	else
		{
		__SPIN_UNLOCK_IRQ(iSpinLock);
		}
	}


#ifdef _DEBUG

void RSlabAllocatorBase::CheckSlab(TSlabHeader* aSlab)
	{
//	Kern::Printf("CheckSlab %x %x %d",aSlab,aSlab->iHighWaterMark,aSlab->iAllocCount);
	TAny* base = (TAny*)((TLinAddr)aSlab&~KPageMask);
	SDblQueLink* o = aSlab->iFreeList.First();
	TUint max = ((TLinAddr)aSlab->iHighWaterMark-(TLinAddr)base)/iObjectSize+1;
	__NK_ASSERT_DEBUG(aSlab->iAllocCount<=max);
	__NK_ASSERT_DEBUG(max<=iObjectsPerSlab);
	TUint freeCount = max-aSlab->iAllocCount;
	while(freeCount)
		{
//		Kern::Printf("CheckSlab o=%x",o);
		__NK_ASSERT_DEBUG(o>=base);
		__NK_ASSERT_DEBUG(o<=aSlab->iHighWaterMark);
		__NK_ASSERT_DEBUG((((TLinAddr)o-(TLinAddr)base)%iObjectSize)==0);
		o = o->iNext;
		--freeCount;
		}
	__NK_ASSERT_DEBUG(o==&aSlab->iFreeList.iA);
	}

#endif


TBool RSlabAllocatorBase::NewSlab()
	{
	TRACE2(("RSlabAllocatorBase::NewSlab()"));

	for(;;)
		{
		__SPIN_UNLOCK_IRQ(iSpinLock);
		MM::MemoryLock(iMemory);

		if(iAllocatingSlab)
			{
			// we've gone recursive...
			__NK_ASSERT_DEBUG(iFreeCount); // check we still have some reserved objects

			// lie and pretend we've allocated a slab which will allow Alloc() to proceed...
			MM::MemoryUnlock(iMemory);
			__SPIN_LOCK_IRQ(iSpinLock);
			return true;
			}

		iAllocatingSlab = true;

		// still need new slab?
		if(iFreeCount<=iReserveCount)
			{
			// find unused slab...
			TInt i = iSlabMap->Alloc();
			if(i<0)
				break; // out of memory

			// commit memory for slab...
			TInt r = MM::MemoryAlloc(iMemory,i,1);
			if(r!=KErrNone)
				{
				iSlabMap->Free(i);
				break; // error
				}

			// initialise slab...
			TLinAddr page = iBase+(i<<KPageShift);
			InitSlab(page);
			TRACE2(("RSlabAllocatorBase::NewSlab() allocated 0x%08x",(TSlabHeader*)(page+KPageSize)-1));
			}

		iAllocatingSlab = false;

		MM::MemoryUnlock(iMemory);
		__SPIN_LOCK_IRQ(iSpinLock);

		// still need new slab?
		if(iFreeCount>iReserveCount)
			return true; // no, so finish
		}

	// failed...
	iAllocatingSlab = false;
	MM::MemoryUnlock(iMemory);
	__SPIN_LOCK_IRQ(iSpinLock);
	return false;
	}


void RSlabAllocatorBase::InitSlab(TLinAddr aPage)
	{
	TRACE2(("RSlabAllocatorBase::InitSlab(0x%08x)",aPage));

	// header goes at end of slab...
	TSlabHeader* slab = (TSlabHeader*)(aPage+KPageSize)-1;

	// link first object in slab onto the slab's free list...
	SDblQueLink* head = &slab->iFreeList.iA;
	SDblQueLink* first = (SDblQueLink*)aPage;
	head->iNext = first;
	head->iPrev = first;
	first->iPrev = head;
	first->iNext = head;

	// setup rest of slab header...
	slab->iAllocCount = 0;
	slab->iHighWaterMark = first;

	// put new slab at end of free slab list...
	__SPIN_LOCK_IRQ(iSpinLock);
	iFreeList.Add(slab);
	iFreeCount += iObjectsPerSlab;
	__SPIN_UNLOCK_IRQ(iSpinLock);
	}


void RSlabAllocatorBase::FreeSlab(TSlabHeader* aSlab)
	{
	TRACE2(("RSlabAllocatorBase::FreeSlab(0x%08x)",aSlab));

	aSlab->Deque();
	iFreeCount -= iObjectsPerSlab;
	__SPIN_UNLOCK_IRQ(iSpinLock);

	MM::MemoryLock(iMemory);
	TUint i = ((TLinAddr)aSlab-iBase)>>KPageShift;
	MM::MemoryFree(iMemory,i,1);
	iSlabMap->Free(i);
	MM::MemoryUnlock(iMemory);
	}


//
// Cleanup
//

void RSlabAllocatorBase::CleanupTrampoline(TAny* aSelf)
	{
	((RSlabAllocatorBase*)aSelf)->Cleanup();
	}


void RSlabAllocatorBase::Cleanup()
	{
	// free any empty slabs...
	for(;;)
		{
		__SPIN_LOCK_IRQ(iSpinLock);
		TSlabHeader* slab = (TSlabHeader*)iFreeList.iA.iPrev; // get slab from end of list
		if(slab==iFreeList.iA.iNext)
			break; // only slab left, so leave it
		if(slab->iAllocCount!=0)
			break; // slab has allocated objects, so end, (empty slabs are always at end of list)
		FreeSlab(slab);
		}
	__SPIN_UNLOCK_IRQ(iSpinLock);
	}
