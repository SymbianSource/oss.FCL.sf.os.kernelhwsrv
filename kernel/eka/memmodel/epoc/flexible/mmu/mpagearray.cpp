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

#include "mpagearray.h"
#include "mslaballoc.h"


static RStaticSlabAllocator<RPageArray::TSegment,KPageArraySegmentBase,KPageArraySegmentEnd> PageSegmentAllocator;


//
// RPageArray::TSegment
//

RPageArray::TSegment* RPageArray::TSegment::New()
	{
	__NK_ASSERT_DEBUG(!MmuLock::IsHeld());

	// allocate segment...
	TSegment* s = PageSegmentAllocator.Alloc();
	if(!s)
		return s;

	// initialise segment...
	s->iCounts = 1; // lock count = 1, alloc count = 0
	TPhysAddr* p = s->iPages;
	TPhysAddr* pEnd = p+KPageArraySegmentSize;
	TPhysAddr nullPage = EEmptyEntry;
	do *p++ = nullPage;
	while(p<pEnd);

	return s;
	}


RPageArray::TSegment* RPageArray::TSegment::Delete(TSegment* aSegment)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aSegment->iCounts==0);
#ifdef _DEBUG
	TPhysAddr* p = aSegment->iPages;
	TPhysAddr* pEnd = p+KPageArraySegmentSize;
	do
		{
		TPhysAddr a = *p++;
		if(IsPresent(a))
			{
			Kern::Printf("TSegment Delete with allocated pages! [%d]=0x%08x",p-aSegment->iPages-1,a);
			__NK_ASSERT_DEBUG(0);
			}
		}
	while(p<pEnd);
#endif
	PageSegmentAllocator.Free(aSegment);
	return 0;
	}


FORCE_INLINE void RPageArray::TSegment::Lock(TUint aCount)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__e32_atomic_add_ord32(&iCounts, (TUint32)aCount);
	__NK_ASSERT_DEBUG((iCounts&KPageArraySegmentLockCountMask));
	}


/**
@return True if segment still exists, false if segment was deleted.
*/
TBool RPageArray::TSegment::Unlock(TSegment*& aSegment, TUint aCount)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());

	TSegment* s = aSegment;
	__NK_ASSERT_DEBUG(s);

	TUint oldCounts = (TUint)__e32_atomic_add_ord32(&s->iCounts, (TUint32)-(TInt)aCount);
	__NK_ASSERT_DEBUG(oldCounts&KPageArraySegmentLockCountMask); // alloc count must have been non-zero before decrementing

#ifdef _DEBUG
	if((oldCounts&KPageArraySegmentLockCountMask)==aCount)
		{
		// check alloc count is consistent...
		TUint allocCount = s->iCounts>>KPageArraySegmentAllocCountShift;
		__NK_ASSERT_DEBUG(allocCount<=KPageArraySegmentSize);
		TUint realAllocCount = 0;
		TPhysAddr* p = s->iPages;
		TPhysAddr* pEnd = p+KPageArraySegmentSize;
		do
			{
			if(IsPresent(*p++))
				++realAllocCount;
			}
		while(p<pEnd);
		if(realAllocCount!=allocCount)
			{
			Kern::Printf("TSegment::Unlock alloc count missmatch %u!=%u",realAllocCount,allocCount);
			__NK_ASSERT_DEBUG(0);
			}
		}
#endif

	if(oldCounts>1)
		return oldCounts; // return 'true' to indicate segment still exists

	// delete segment...
	aSegment = 0;
	return (TBool)Delete(s); // returns 'false'
	}


FORCE_INLINE void RPageArray::TSegment::AdjustAllocCount(TInt aDelta)
	{
	__NK_ASSERT_DEBUG((iCounts&KPageArraySegmentLockCountMask));
	__e32_atomic_add_ord32(&iCounts, TUint32(aDelta)<<KPageArraySegmentAllocCountShift);
	}


#ifdef _DEBUG
void RPageArray::TSegment::Dump()
	{
	TUint allocCount = iCounts>>KPageArraySegmentAllocCountShift;
	TUint lockCount = iCounts&KPageArraySegmentLockCountMask;
	Kern::Printf("RPageArray::TSegment[0x%08x]::Dump() allocCount=%d lockCount=%d",this,allocCount,lockCount);
	for(TUint i=0; i<KPageArraySegmentSize; i+=4)
		Kern::Printf("  %08x %08x %08x %08x",iPages[i+0],iPages[i+1],iPages[i+2],iPages[i+3]);
	}
#endif


//
// RPageArray::TIter
//

TUint RPageArray::TIter::Pages(TPhysAddr*& aStart, TUint aMaxCount)
	{
	// MmuLock *may* be needed, depends if segments have been locked

	TUint index = iIndex;
	TUint size = iEndIndex-index;
	if(!size)
		return 0;

	TUint offset = index&KPageArraySegmentMask;
	aStart = iSegments[index>>KPageArraySegmentShift]->iPages+offset;

	TUint n = KPageArraySegmentSize-offset;
	if(n>aMaxCount)
		n = aMaxCount;
	if(n>size)
		n = size;
	return n;
	}


TUint RPageArray::TIter::AddFind(TIter& aPageList)
	{
	TRACE2(("RPageArray::TIter::AddFind range 0x%x..0x%x",iIndex,iEndIndex));

	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	if(index==endIndex)
		{
nothing_found:
		aPageList.iIndex = endIndex;
		aPageList.iEndIndex = endIndex;
		TRACE2(("RPageArray::TIter::AddFind returns 0x%x+0x%x",iEndIndex,0));
		return 0;
		}

	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	TPhysAddr* p;
	TUint limit;

	MmuLock::Lock();

	// scan for empty entries...
	do
		{
		// get segment...
		p = (*pS++)->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		// scan segment...
		do
			{
			TPhysAddr page = *p;
			if(!IsPresent(page))
				goto find_start;
#if _DEBUG
			if(State(page)!=ECommitted)
				{
				Kern::Printf("RPageArray::TIter::AddFind found unexpected page: %x",page);
				__NK_ASSERT_DEBUG(0);
				// *p = (page&~(EStateMask|EVetoed))|ECommitted; // mark page as allocated again
				}
#endif
			++p;
			}
		while(++index<limit);

		MmuLock::Flash();
		}
	while(index<endIndex);

	MmuLock::Unlock();
	goto nothing_found;

find_start:
	TUint startIndex = index;
	// scan for end of empty region...
	for(;;)
		{
		// scan segment...
		do
			{
			if(IsPresent(*p++))
				goto find_end;
			}
		while(++index<limit);
		// check for end...
		if(index>=endIndex)
			break;
		MmuLock::Flash();
		// get next segment...
		p = (*pS++)->iPages;
		TUint nextIndex = index+KPageArraySegmentSize;
		limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		}

find_end:
	MmuLock::Unlock();

	aPageList.iSegments = iSegments;
	aPageList.iIndex = startIndex;
	aPageList.iEndIndex = index;

	iIndex = index;
	TUint n = index-startIndex;
	TRACE2(("RPageArray::TIter::AddFind returns 0x%x+0x%x",startIndex,n));
	return n;
	}


void RPageArray::TIter::Add(TUint aCount, const TPhysAddr* aPages)
	{
	// MmuLock NOT required because...
	// 1. AddStart has ensured all segments are allocated and locked (so they can't be deleted)
	// 2. AddFind returns an unallocated region. This can only be changed by Adding pages
	//    and we only allow one thread to do this at a time (i.e. the thread calling this function.)

	TRACE2(("RPageArray::TIter::Add 0x%x+0x%x",iIndex,aCount));
	__NK_ASSERT_DEBUG(aCount);

	TUint index = iIndex;
	TUint endIndex = index+aCount;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	do
		{
		// get segment...
		TSegment* s = *pS++;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;

		// add pages to segment...
		s->AdjustAllocCount(limit-index);
		do
			{
			__NK_ASSERT_DEBUG((*aPages&KPageMask)==0);
			__NK_ASSERT_DEBUG(!IsPresent(*p)); // AddFind only found not-present entries
			*p++ = *aPages++|ECommitted;
			}
		while(++index<limit);
		}
	while(index<endIndex);

	iIndex = index;
	}


void RPageArray::TIter::AddContiguous(TUint aCount, TPhysAddr aPhysAddr)
	{
	// MmuLock NOT required because...
	// 1. AddStart has ensured all segments are allocated and locked (so they can't be deleted)
	// 2. AddFind returns an unallocated region. This can only be changed by Adding pages
	//    and we only allow one thread to do this at a time (i.e. the thread calling this function.)

	TRACE2(("RPageArray::TIter::AddContiguous 0x%x+0x%x",iIndex,aCount));
	__NK_ASSERT_DEBUG(aCount);
	__NK_ASSERT_DEBUG((aPhysAddr&KPageMask)==0);

	TUint index = iIndex;
	TUint endIndex = index+aCount;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);

	do
		{
		// get segment...
		TSegment* s = *pS++;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;

		// add pages to segment...
		s->AdjustAllocCount(limit-index);
		do
			{
			__NK_ASSERT_DEBUG(!IsPresent(*p)); // AddFind only found not-present entries
			*p++ = aPhysAddr|ECommitted;
			aPhysAddr += KPageSize;
			}
		while(++index<limit);
		}
	while(index<endIndex);

	iIndex = index;
	}


void RPageArray::TIter::Added(TUint aCount, TUint aChanged)
	{
	__NK_ASSERT_DEBUG(aCount);
	__NK_ASSERT_DEBUG(aChanged<=aCount);
	TUint index = iIndex;
	__NK_ASSERT_DEBUG((index>>KPageArraySegmentShift)==((index+aCount-1)>>KPageArraySegmentShift));
	TSegment* s = iSegments[index>>KPageArraySegmentShift];
	__NK_ASSERT_DEBUG(s);
	__NK_ASSERT_DEBUG(s->iCounts&KPageArraySegmentLockCountMask);
	s->AdjustAllocCount(aChanged);
	Skip(aCount);
	}


TUint RPageArray::TIter::Find(TIter& aPageList)
	{
	TRACE2(("RPageArray::TIter::Find range 0x%x..0x%x",iIndex,iEndIndex));

	MmuLock::Lock();
	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;

	// search for first page...
	while(index<endIndex)
		{
		TSegment* s = *pS;
		if(!s)
			index = nextIndex;
		else
			{
			// search segment...
			TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
			TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
			do
				{
				if(RPageArray::IsPresent(*p++))
					goto start_done;
				}
			while(++index<limit);
			}
		// next segment...
		MmuLock::Flash();
		++pS;
		nextIndex = index+KPageArraySegmentSize;
		}
start_done:
	// we can't flash or release the MmuLock until we've Locked the segment we found!
	iIndex = index;

	// search for range of allocated pages...
	while(index<endIndex)
		{
		// check first entry...
		TSegment* s = *pS;
		if(!s)
			break;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		if(!RPageArray::IsPresent(*p++))
			break;

		// segment has pages, lock it...
		s->Lock();

		// scan rest of entries...
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		while(++index<limit)
			if(!RPageArray::IsPresent(*p++))
				goto done;

		// next segment...
		MmuLock::Flash();
		++pS;
		}
done:
	MmuLock::Unlock();

	aPageList.iSegments = iSegments;
	aPageList.iIndex = iIndex;
	aPageList.iEndIndex = index;
	TInt n = index-iIndex;
	TRACE2(("RPageArray::TIter::Find returns 0x%x+0x%x",iIndex,n));
	return n;
	}


void RPageArray::TIter::FindRelease(TUint aCount)
	{
	TUint index = iIndex;
	Skip(aCount);
	RPageArray::Release(iSegments,index,aCount);
	}


TUint RPageArray::TIter::RemoveFind(TIter& aPageList)
	{
	TRACE2(("RPageArray::TIter::RemoveFind range 0x%x..0x%x",iIndex,iEndIndex));

	MmuLock::Lock();

	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;

	// search for first page...
	while(index<endIndex)
		{
		TSegment* s = *pS;
		if(!s)
			index = nextIndex;
		else
			{
			// search segment...
			TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
			TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
			do
				{
				if(State(*p++)>=EDecommitting)
					goto start_done;
				}
			while(++index<limit);
			}

		// next segment...
		MmuLock::Flash();
		++pS;
		nextIndex = index+KPageArraySegmentSize;
		}
start_done:
	// we can't flash or release the MmuLock until we've Locked the segment we found!
	iIndex = index;

	// search for range of allocated pages, marking them EDecommitting...
	while(index<endIndex)
		{
		// check first entry...
		TSegment* s = *pS;
		if(!s)
			break;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TPhysAddr page = *p++;
		if(State(page)<EDecommitting)
			break;

		p[-1] = (page&~EStateMask)|EDecommitting;

		// segment has pages, lock it...
		s->Lock();

		// scan rest of entries...
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		while(++index<limit)
			{
			TPhysAddr page = *p++;
			if(State(page)<EDecommitting)
				goto done;
			p[-1] = (page&~EStateMask)|EDecommitting;
			}

		// next segment...
		MmuLock::Flash();
		++pS;
		}
done:
	MmuLock::Unlock();

	aPageList.iSegments = iSegments;
	aPageList.iIndex = iIndex;
	aPageList.iEndIndex = index;
	TInt n = index-iIndex;
	TRACE2(("RPageArray::TIter::RemoveFind returns 0x%x+0x%x",iIndex,n));
	return n;
	}


TUint RPageArray::TIter::Remove(TUint aMaxCount, TPhysAddr* aPages)
	{
	TRACE2(("RPageArray::TIter::Remove 0x%x..0x%x max=0x%x",iIndex,iEndIndex,aMaxCount));

	__NK_ASSERT_DEBUG(aMaxCount);

	TUint count = 0;
	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	if(index==endIndex)
		return 0;

	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);

	MmuLock::Lock();

	do
		{
		// get segment...
		TSegment* s = *pS++;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;

		// remove pages from segment...
		do
			{
			TPhysAddr page = *p++;
			__NK_ASSERT_DEBUG(State(page)!=EStealing); // can't be stealing as that only happens with the RamAllocLock held, which we should already hold if freeing demand paged pages
			if(State(page)==EDecommitting || State(page)==EDecommitted)
				{
				// remove a page...
				if(page&EUnmapVetoed)
					{
					p[-1] = (page&~(EUnmapVetoed|EStateMask))|EDecommitted; // change to EDecommitted state
					}
				else
					{
					p[-1] = EEmptyEntry;
					s->AdjustAllocCount(-1);
					TPhysAddr pagePhys = page&~KPageMask;
					aPages[count++] = pagePhys;
					TRACE2(("RPageArray::TIter::Remove index=0x%x returns 0x%08x",index,pagePhys));
					if(count>=aMaxCount)
						{
						++index;
						goto done;
						}
					}
				// check not removing managed pages without the RamAllocLock...
				__NK_ASSERT_DEBUG(RamAllocLock::IsHeld()
					|| SPageInfo::FromPhysAddr(page)->Type()!=SPageInfo::EManaged);
				}
			}
		while(++index<limit);

		MmuLock::Flash();
		}
	while(index<endIndex);

done:
	MmuLock::Unlock();
	iIndex = index;
	return count;
	}


void RPageArray::TIter::VetoUnmap()
	{
	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	if(index==endIndex)
		return;

	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);

	MmuLock::Lock();

	do
		{
		// get segment...
		TSegment* s = *pS++;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;

		// veto pages in segment...
		do
			{
			TPhysAddr page = *p++;
			TRACE2(("RPageArray::TIter::Veto() yes/no=%d page=0x%08x",IsPresent(page) && TargetStateIsDecommitted(page),page));
			if(IsPresent(page) && TargetStateIsDecommitted(page))
				p[-1] = page|EUnmapVetoed;
			}
		while(++index<limit);

		MmuLock::Flash();
		}
	while(index<endIndex);

	MmuLock::Unlock();
	}
	

void RPageArray::TIter::VetoRestrict(TBool aPageMoving)
	{
	TUint index = iIndex;
	TUint endIndex = iEndIndex;
	if(index==endIndex)
		return;

	RPageArray::TState operation = 	aPageMoving ? RPageArray::EMoving : RPageArray::ERestrictingNA;

	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);

	MmuLock::Lock();

	do
		{
		// get segment...
		TSegment* s = *pS++;
		TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;

		// veto pages in segment...
		do
			{
			TPhysAddr page = *p++;
			TRACE2(("RPageArray::TIter::VetoRestrict() yes/no=%d page=0x%08x",State(page)==operation,page));
			if(State(page)==operation)
				{
				// to veto a 'restrict page' operation, we put the page back into the committed...
				p[-1] = (page&~EStateMask)|ECommitted;
				}
			}
		while(++index<limit);

		MmuLock::Flash();
		}
	while(index<endIndex);

	MmuLock::Unlock();
	}
	

FORCE_INLINE void RPageArray::TIter::Set(RPageArray::TSegment** aSegments, TUint aIndex, TUint aEndIndex)
	{
	iSegments = aSegments;
	iIndex = aIndex;
	iEndIndex = aEndIndex;
	}


//
// RPageArray
//

void RPageArray::Init2A()
	{
	TInt r = PageSegmentAllocator.Construct();
	__NK_ASSERT_ALWAYS(r==KErrNone);
	}


void RPageArray::Init2B(DMutex* aLock)
	{
	// construct memory object for slabs...
	DMemoryObject* memory;
	TMappingCreateFlags mapFlags = (TMappingCreateFlags)(EMappingCreateFixedVirtual|EMappingCreateReserveAllResources);
	TMemoryAttributes memAttr = EMemoryAttributeStandard;
	TInt r = MM::InitFixedKernelMemory(memory, KPageArraySegmentBase, KPageArraySegmentEnd, KPageSize, EMemoryObjectUnpaged, EMemoryCreateNoWipe, memAttr, mapFlags);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	MM::MemorySetLock(memory,aLock);
	PageSegmentAllocator.SetMemory(memory,1);
	}


RPageArray::RPageArray()
	{
	__NK_ASSERT_DEBUG(!iSegments);
	}


TInt RPageArray::Construct(TUint aMaxPages, TBool aPreallocateMemory)
	{
	iNumSegments = (aMaxPages+KPageArraySegmentMask)>>KPageArraySegmentShift;
	iSegments = (TSegment**)Kern::AllocZ(iNumSegments*sizeof(TSegment*));
	if(!iSegments)
		return KErrNoMemory;

	if(!aPreallocateMemory)
		return KErrNone;

	return PreallocateMemory();
	}


TInt RPageArray::PreallocateMemory()
	{
	MmuLock::Lock();

	__NK_ASSERT_DEBUG(!iPreallocatedMemory);
	iPreallocatedMemory = true;

	TSegment** pS = iSegments;
	TSegment** pGEnd = pS+iNumSegments;
	do
		{
		if(!GetOrAllocateSegment(pS,1))
			{
			iNumSegments = pS-iSegments; // truncate to amount successfully allocated
			MmuLock::Unlock();
			return KErrNoMemory;
			}
		}
	while(++pS<pGEnd);

	MmuLock::Unlock();
	return KErrNone;
	}


RPageArray::~RPageArray()
	{
	TSegment** pS = iSegments;
	if(pS)
		{
		TSegment** pGEnd = pS+iNumSegments;
		if(!iPreallocatedMemory)
			{
			// check all segments have already been deleted...
			while(pS<pGEnd)
				{
#ifdef _DEBUG
				if(*pS)
					(*pS)->Dump();
#endif
				__NK_ASSERT_DEBUG(!*pS);
				++pS;
				}
			}
		else
			{
			MmuLock::Lock();
			while(pS<pGEnd)
				{
				__NK_ASSERT_DEBUG(*pS);
				TSegment::Unlock(*pS);
#ifdef _DEBUG
				if(*pS)
					(*pS)->Dump();
#endif
				__NK_ASSERT_DEBUG(!*pS);
				TRACE2(("RPageArray::~RPageArray delete segment=%d",pS-iSegments));
				++pS;
				if(pS<pGEnd)
					MmuLock::Flash();
				}
			MmuLock::Unlock();
			}

		Kern::Free(iSegments);
		}
	}


RPageArray::TSegment* RPageArray::GetOrAllocateSegment(TSegment** aSegmentEntry, TUint aLockCount)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aLockCount);

	for(;;)
		{
		TSegment* s = *aSegmentEntry;
		if(s)
			{
			s->Lock(aLockCount);
			return s;
			}

		// no segment, so allocate one...
		MmuLock::Unlock();
		s = TSegment::New();
		MmuLock::Lock();
		if(!s)
			return s;

		// if someone else allocated one...
		if(*aSegmentEntry)
			{
			// free the one we created...
			TSegment::Unlock(s); 
			//and retry...
			continue;
			}

		// use new segment...
		TRACE2(("RPageArray::GetOrAllocateSegment new segment=%d",aSegmentEntry-iSegments));
		*aSegmentEntry = s;
		if(--aLockCount)
			s->Lock(aLockCount);
		return s;
		}
	}


TInt RPageArray::Alloc(TUint aIndex, TUint aCount)
	{
	TRACE2(("RPageArray::Alloc(0x%x,0x%x)",aIndex,aCount));
	__NK_ASSERT_DEBUG(aIndex+aCount<=iNumSegments*KPageArraySegmentSize);
	__NK_ASSERT_DEBUG(aIndex+aCount>=aIndex);

	MmuLock::Lock();

	TUint index = aIndex;
	TUint endIndex = aIndex+aCount;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	while(index<endIndex)
		{
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		TUint lockCount = limit-index;
		index = limit;
		TSegment* s = GetOrAllocateSegment(pS++,lockCount);
		if(!s)
			goto no_memory;
		}

	MmuLock::Unlock();
	return KErrNone;

no_memory:
	MmuLock::Unlock();

	// free what we actually alloced...
	endIndex = index&~KPageArraySegmentMask;
	Free(aIndex,endIndex-aIndex);

	return KErrNoMemory;
	}


void RPageArray::Free(TUint aIndex, TUint aCount)
	{
	TRACE2(("RPageArray::Free(0x%x,0x%x)",aIndex,aCount));
	__NK_ASSERT_DEBUG(aIndex+aCount<=iNumSegments*KPageArraySegmentSize);
	__NK_ASSERT_DEBUG(aIndex+aCount>aIndex);

	MmuLock::Lock();

	TUint index = aIndex;
	TUint endIndex = aIndex+aCount;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	while(index<endIndex)
		{
		__NK_ASSERT_DEBUG(*pS);
		TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
		TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
		TSegment::Unlock(*pS,limit-index);
		index = limit;
		++pS;
		}

	MmuLock::Unlock();
	}


TInt RPageArray::AddStart(TUint aIndex, TUint aCount, TIter& aIter, TBool aAllowExisting)
	{
	TRACE2(("RPageArray::AddStart(0x%x,0x%x,?,%d)",aIndex,aCount,aAllowExisting));
	__NK_ASSERT_DEBUG(aIndex+aCount<=iNumSegments*KPageArraySegmentSize);
	__NK_ASSERT_DEBUG(aIndex+aCount>aIndex);

	aIter.Set(iSegments,aIndex,aIndex+aCount);

	MmuLock::Lock();

	TInt r;
	TUint index = aIndex;
	TUint endIndex = aIndex+aCount;
	TSegment** pS = iSegments+(index>>KPageArraySegmentShift);
	while(index<endIndex)
		{
		TSegment* s = *pS;
		if(!s)
			{
			// no segment, so allocate one...
			MmuLock::Unlock();
			s = TSegment::New();
			MmuLock::Lock();
			if(!s)
				goto no_memory;

			// if someone else allocated one
			if(*pS)
				{
				// free the one we created...
				TSegment::Unlock(s); 
				//and retry...
				continue;
				}

			// use new segment...
			TRACE2(("RPageArray::AddStart new segment=%d",pS-iSegments));
			*pS = s;

			// move on...
			index = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
			}
		else
			{
			TUint nextIndex = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
			if(aAllowExisting)
				{
				// just move on to next segment...
				index = (index+KPageArraySegmentSize)&~KPageArraySegmentMask;
				}
			else
				{
				// check page entries are empty...
				TPhysAddr* p = s->iPages+(index&KPageArraySegmentMask);
				TUint limit = (nextIndex<endIndex) ? nextIndex : endIndex;
				do
					{
					if(IsPresent(*p++))
						goto already_exists;
					}
				while(++index<limit);
				}
			// lock segment so that it doesn't go away...
			s->Lock();

			if(index<endIndex)
				MmuLock::Flash();
			}
		++pS;
		}

	// done...
	MmuLock::Unlock();
	return KErrNone;

no_memory:
	r = KErrNoMemory;
	goto error;
already_exists:
	r = KErrAlreadyExists;
error:
	MmuLock::Unlock();

	// unlock any segments that we locked...
	endIndex = index&~KPageArraySegmentMask;
	if(endIndex>aIndex)
		Release(iSegments,aIndex,endIndex-aIndex);

	// return error...
	return r;
	}


void RPageArray::AddEnd(TUint aIndex, TUint aCount)
	{
	Release(iSegments,aIndex,aCount);
	}


void RPageArray::FindStart(TUint aIndex, TUint aCount, TIter& aIter)
	{
	TRACE2(("RPageArray::FindStart(0x%x,0x%x,?)",aIndex,aCount));
	__NK_ASSERT_DEBUG(aIndex+aCount<=iNumSegments*KPageArraySegmentSize);
	__NK_ASSERT_DEBUG(aIndex+aCount>aIndex);

	aIter.Set(iSegments,aIndex,aIndex+aCount);
	}


void RPageArray::Release(TSegment** aSegments, TUint aIndex, TUint aCount)
	{
	__NK_ASSERT_DEBUG(aIndex+aCount>aIndex);

	MmuLock::Lock();

	TSegment** pS = aSegments+(aIndex>>KPageArraySegmentShift);
	TSegment** pGLast = aSegments+((aIndex+aCount-1)>>KPageArraySegmentShift);
	__NK_ASSERT_DEBUG(pS<=pGLast);
	TUint flash = 0;
	do
		{
		MmuLock::Flash(flash,KMaxPagesInOneGo);
		if(TSegment::Unlock(*pS)==0)
			{
			TRACE2(("RPageArray::Release delete segment=%d",pS-aSegments));
			}
		++pS;
		}
	while(pS<=pGLast);

	MmuLock::Unlock();
	}


TPhysAddr* RPageArray::AddPageStart(TUint aIndex, TIter& aPageList)
	{
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	MmuLock::Lock();
	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = GetOrAllocateSegment(pS,1);
	MmuLock::Unlock();

	if(!s)
		return 0;

	aPageList.Set(iSegments,aIndex,aIndex+1);

	return s->iPages+(aIndex&KPageArraySegmentMask);
	}


TPhysAddr* RPageArray::RemovePageStart(TUint aIndex, TIter& aPageList)
	{
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	MmuLock::Lock();

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	if(!s)
		{
		MmuLock::Unlock();
		return 0;
		}

	TPhysAddr* p = s->iPages+(aIndex&KPageArraySegmentMask);
	TPhysAddr page = *p;
	if(State(page)<EDecommitting)
		{
		MmuLock::Unlock();
		return 0;
		}

	*p = (page&~EStateMask)|EDecommitting;

	s->Lock();

	MmuLock::Unlock();

	aPageList.Set(iSegments,aIndex,aIndex+1);

	return p;
	}


TPhysAddr RPageArray::RemovePage(TPhysAddr* aPageEntry)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	TPhysAddr page = *aPageEntry;
	__NK_ASSERT_DEBUG(State(page)!=EStealing); // can't be stealing as that only happens with the RamAllocLock held, which we should already hold if freeing demand paged pages
	if(State(page)==EDecommitting || State(page)==EDecommitted)
		{
		// remove a page...
		if(page&EUnmapVetoed)
			{
			*aPageEntry = (page&~(EUnmapVetoed|EStateMask))|EDecommitted; // change to EDecommitted state
			}
		else
			{
			*aPageEntry = EEmptyEntry;
			return page&~KPageMask;
			}
		// check not removing managed pages without the RamAllocLock...
		__NK_ASSERT_DEBUG(RamAllocLock::IsHeld()
			|| SPageInfo::FromPhysAddr(page)->Type()!=SPageInfo::EManaged);
		}
	return KPhysAddrInvalid;
	}


TPhysAddr* RPageArray::RestrictPageNAStart(TUint aIndex, TIter& aPageList)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	if(!s)
		return 0;

	TPhysAddr* p = s->iPages+(aIndex&KPageArraySegmentMask);
	TPhysAddr page = *p;
	if(State(page) < RPageArray::ERestrictingNA)
		return 0;

	*p = (page&~EStateMask) | RPageArray::ERestrictingNA;

	s->Lock();

	aPageList.Set(iSegments,aIndex,aIndex+1);

	return p;
	}


TPhysAddr* RPageArray::StealPageStart(TUint aIndex, TIter& aPageList)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	__NK_ASSERT_DEBUG(s); // we only steal pages in the live list and these can not go away yet because we hold the RamAllocLock

	TPhysAddr* p = s->iPages+(aIndex&KPageArraySegmentMask);
	TPhysAddr page = *p;

	if(State(page)>EStealing)
		*p = (page&~EStateMask)|EStealing;

	s->Lock();

	aPageList.Set(iSegments,aIndex,aIndex+1);

	return p;
	}


TPhysAddr* RPageArray::MovePageStart(TUint aIndex, TIter& aPageList)
	{
	__NK_ASSERT_DEBUG(RamAllocLock::IsHeld());
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex <= iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	// The segment should always exist for a page that is being moved.
	__NK_ASSERT_DEBUG(s);

	TPhysAddr* p = s->iPages+(aIndex&KPageArraySegmentMask);
	TPhysAddr page = *p;
	if(State(page) <= RPageArray::EMoving)
		return NULL;

	*p = (page & ~EStateMask) | EMoving;
	s->Lock();

	aPageList.Set(iSegments, aIndex, aIndex+1);

	return p;
	}


void RPageArray::ReleasePage(TUint aIndex, TInt aDelta)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	__NK_ASSERT_DEBUG(s); // must exist because FindPageStart/AddPageStart locked it

	__NK_ASSERT_DEBUG(aDelta>=-1 && aDelta<=1);
	if(aDelta)
		s->AdjustAllocCount(aDelta);

	if(TSegment::Unlock(*pS)==0)
		{
		TRACE2(("RPageArray::ReleasePage delete segment=%d",pS-iSegments));
		}
	}


TPhysAddr RPageArray::Page(TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	if(!s)
		return ENotPresent;
	return s->iPages[aIndex&KPageArraySegmentMask];
	}


TPhysAddr* RPageArray::PageEntry(TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	if(!s)
		return NULL;
	return s->iPages + (aIndex & KPageArraySegmentMask);
	}


TUint RPageArray::PagingManagerData(TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);
	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	__NK_ASSERT_DEBUG(s);
	TPhysAddr* p = &s->iPages[aIndex&KPageArraySegmentMask];

	TPhysAddr entry = *p;
	if(IsPresent(entry))
		{
#ifdef _DEBUG
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(entry&~KPageMask);
		if(!pi)
			Kern::Printf("RPageArray::PagingManagerData bad entry 0x%08x",entry);
		__NK_ASSERT_DEBUG(pi);
#else
		SPageInfo* pi = SPageInfo::FromPhysAddr(entry);
#endif
		entry = pi->PagingManagerData();
		}
	__NK_ASSERT_DEBUG((entry&(EFlagsMask|EStateMask))==ENotPresent);

	return entry>>(EFlagsShift+EStateShift);
	}


void RPageArray::SetPagingManagerData(TUint aIndex, TUint aValue)
	{
	aValue = (aValue<<(EFlagsShift+EStateShift))|ENotPresent;

	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);
	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	__NK_ASSERT_DEBUG(s);
	TPhysAddr* p = &s->iPages[aIndex&KPageArraySegmentMask];

	TPhysAddr entry = *p;
	if(!IsPresent(entry))
		*p = aValue;
	else
		{
#ifdef _DEBUG
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(entry&~KPageMask);
		if(!pi)
			Kern::Printf("RPageArray::SetPagingManagerData bad entry 0x%08x",entry);
		__NK_ASSERT_DEBUG(pi);
#else
		SPageInfo* pi = SPageInfo::FromPhysAddr(entry);
#endif
		pi->SetPagingManagerData(aValue);
		}
	}


TPhysAddr RPageArray::PhysAddr(TUint aIndex)
	{
	__NK_ASSERT_DEBUG(MmuLock::IsHeld());
	__NK_ASSERT_DEBUG(aIndex<=iNumSegments*KPageArraySegmentSize);

	TSegment** pS = iSegments+(aIndex>>KPageArraySegmentShift);
	TSegment* s = *pS;
	if(s)
		{
		TPhysAddr page = s->iPages[aIndex&KPageArraySegmentMask];
		if(IsPresent(page))
			{
			return page&~KPageMask;
			}
		}
	return KPhysAddrInvalid;
	}


TInt RPageArray::PhysAddr(TUint aIndex, TUint aCount, TPhysAddr& aPhysicalAddress, TPhysAddr* aPhysicalPageList)
	{
	__NK_ASSERT_DEBUG(aCount);
	MmuLock::Lock();

	TUint32* pageList = aPhysicalPageList;

	// get first page...
	TPhysAddr physStart = PhysAddr(aIndex++);
	if(physStart==KPhysAddrInvalid)
		{
		MmuLock::Unlock();
		return KErrNotFound;
		}
	if(pageList)
		*pageList++ = physStart;

	TUint32 nextPhys = physStart+KPageSize;

	TUint flash = 0;
	while(--aCount)
		{
		MmuLock::Flash(flash,KMaxPagesInOneGo);

		// get next page...
		TPhysAddr phys = PhysAddr(aIndex++);
		if(phys==KPhysAddrInvalid)
			{
			MmuLock::Unlock();
			return KErrNotFound;
			}
		if(pageList)
			*pageList++ = phys;

		// check for contiguity...
		if(phys!=nextPhys)
			nextPhys = KPhysAddrInvalid;
		else
			nextPhys += KPageSize;
		}

	MmuLock::Unlock();

	if(nextPhys==KPhysAddrInvalid)
		{
		// memory is discontiguous...
		if(!aPhysicalPageList)
			return KErrNotFound;
		aPhysicalAddress = KPhysAddrInvalid;
		return 1;
		}
	else
		{
		// memory is contiguous...
		aPhysicalAddress = physStart;
		return KErrNone;
		}
	}


