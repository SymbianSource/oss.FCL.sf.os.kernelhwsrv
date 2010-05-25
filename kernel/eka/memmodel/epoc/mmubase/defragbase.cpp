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
// e32\memmodel\epoc\mmubase\defragbase.cpp
// 
//

/**
 @file
 @internalComponent
*/
#include <kernel/kern_priv.h>
#include <defrag.h>
#include <ramalloc.h>

#ifndef __MEMMODEL_FLEXIBLE__
#include <mmubase.inl>
#else
#include "mdefrag.inl"
#endif //__MEMMODEL_FLEXIBLE__

// Maximum number of times to attempt to defrag a particular zone.
const TUint KDefragMaxRetries = 5;

const TInt KDefragIdlingThreadPriority = 27;

Defrag* Defrag::TheDefrag = NULL;

_LIT(KDefragDfcThreadName, "DefragDFC");

void Defrag::Panic(TPanic aPanic)
	{
	Kern::Fault("DEFRAG",aPanic);
	}


Defrag::Defrag()
	{
	}


void Defrag::Init3(DRamAllocator* aRamAllocator)
	{
	TheDefrag = this;

	TInt r = Kern::DfcQInit(&iTaskQ, KDefragIdlingThreadPriority, &KDefragDfcThreadName);
	if (r!=KErrNone)
		Panic(EDfcQInitFailed);

	iRamAllocator = aRamAllocator;
	}


/**
Move the movable pages in this zone into higher priority zones.

@param aZone The zone to clear movable pages from
@param aBestEffort Set to ETrue to always keep clearing pages even if all can't 
be moved or other threads allocate into the zone.
@param aRequest The request object containing the defrag parameters.
@pre RamAlloc mutex held.
@post RamAlloc mutex held.
*/
TInt Defrag::ClearMovableFromZone(SZone& aZone, TBool aBestEffort, TRamDefragRequest* aRequest)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ClearMovableFromZone: ID %x ", aZone.iId));

	TUint offset = 0;
	TPhysAddr newAddr;
	TBool zoneActive = EFalse;
	const TUint moveDisFlags = (aBestEffort)? 0 : M::EMoveDisBlockRest | M::EMoveDisMoveDirty;
	TInt ret = iRamAllocator->NextAllocatedPage(&aZone, offset, EPageMovable);

	// if not best effort mode keep moving pages unless someone allocates into 
	// the zone or an unmovable page is hit.
	// If in best effort mode then attempt to move all allocated pages in 
	// the zone regardless.
	while (	aZone.iAllocPages[EPageMovable] != 0 && ret == KErrNone) 
		{
		if (aRequest->PollForCancel())
			{
			__KTRACE_OPT(KMMU, Kern::Printf("ClearMovableFromZone: cancelled"));
			return KErrCancel;
			}
		
		TPhysAddr addr = (offset << M::PageShift()) + aZone.iPhysBase;

		if (!aBestEffort && 
			(aZone.iAllocPages[EPageMovable] > iRamAllocator->GenDefragFreePages(EPageMovable) ||
			zoneActive))
			{
			__KTRACE_OPT(KMMU, Kern::Printf("ClearMovableFromZone: memory too low or zone active addr %x", addr));
			return KErrNoMemory;
			}
		TInt moved = M::MovePage(addr, newAddr, aZone.iId, moveDisFlags);
		if (moved != KErrNone)
			{// Couldn't move the page so stop as we can't clear the zone
			if (!aBestEffort)
				{
				__KTRACE_OPT(KMMU, Kern::Printf("ClearMovableFromZone exit: move fail zone %x addr %x", aZone.iId, addr));
				return moved;
				}
			}
		// Flash RAM alloc mutex to allow other allocations
		iRamAllocator->ZoneMark(aZone);
		M::RamAllocUnlock();
		M::RamAllocLock();
		zoneActive = !iRamAllocator->ZoneUnmark(aZone);
		offset++;
		ret = iRamAllocator->NextAllocatedPage(&aZone, offset, EPageMovable);
		}
	__KTRACE_OPT(KMMU, Kern::Printf("ClearMovableFromZone: ret %d off %x", ret, offset));
	return KErrNone;
	}


/**
Discard as many RAM cache pages from the specified zone as possible in one pass.
The method will return when no more pages could be discarded.

@param aZone 		The zone to clear of cache pages.
@param aBestEffort 	Set to ETrue continue clearing the zone even if more allocations
					are made into the zone or if it isn't possible to clear all the 
					cache pages from the zone.  Otherwise set to EFalse.
@param aRequest 	The request object containing the defrag parameters.
@param aMaxDiscard	A pointer to the maximum number of discardable pages to discard.
					Set to NULL if there is no maximum.
@pre RamAlloc mutex held.
@post RamAlloc mutex held.
*/
TInt Defrag::ClearDiscardableFromZone(SZone& aZone, TBool aBestEffort, TRamDefragRequest* aRequest, TUint* aMaxDiscard)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: ID %x ", aZone.iId));

	TUint offset = 0;
	TBool zoneActive = EFalse;
	TInt ret = iRamAllocator->NextAllocatedPage(&aZone, offset, EPageDiscard);
	const TUint moveDisFlags = (aBestEffort)? 0 : M::EMoveDisBlockRest | M::EMoveDisMoveDirty;


	while (	aZone.iAllocPages[EPageDiscard] != 0 && ret == KErrNone && 
			(!aMaxDiscard || *aMaxDiscard))
		{
		if (aRequest->PollForCancel())
			{
			__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: cancelled"));
			return KErrCancel;
			}

		TPhysAddr addr = aZone.iPhysBase + (offset << M::PageShift());

		// When running is best effort mode keep clearing pages whatever.  If not in 
		// best effort stop the defrag if can't remove all the discardable pages 
		// without reducing the cache beyond its minimum size or someone is 
		// allocating into this zone or if it is active.
		if (!aBestEffort && 
			(iRamAllocator->GenDefragFreePages(EPageDiscard) + M::NumberOfFreeDpPages() < aZone.iAllocPages[EPageDiscard] ||
			zoneActive))
			{
			__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: memory too low or zone active addr %x", addr));
			return KErrNoMemory;
			}

		TInt discardRet = M::DiscardPage(addr, aZone.iId, moveDisFlags);
		if (discardRet == KErrNone)
			{// Page was discarded successfully.
			if (aMaxDiscard)
				(*aMaxDiscard)--;
			}
		else
			{
			if (!aBestEffort)
				{// Page couldn't be discarded and this is a general defrag so stop.
				__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: page discard fail addr %x r %d", addr, discardRet));
				return discardRet;
				}
			}

		// Give someone else ago on the RAM alloc mutex.
		iRamAllocator->ZoneMark(aZone);
		M::RamAllocUnlock();
		M::RamAllocLock();
		zoneActive = !iRamAllocator->ZoneUnmark(aZone);
		offset++;
		ret = iRamAllocator->NextAllocatedPage(&aZone, offset, EPageDiscard);
		}
	__KTRACE_OPT(KMMU, Kern::Printf("ClearDiscardableFromZone: ret %d off %x", ret, offset));
	return KErrNone;
	}


/**
Attempt to remove as many pages as possible from the zone.

If there are not enough free zones to move pages to or there are fixed pages in the zone it 
will not be possible to completely clear the zone, in this case this method will return 
KErrNoMemory.

@param aZone		The zone to be cleared
@param aMaxRetries	The maximum number of passes to run through the zone
@param aRequest		The request object containing the defrag parameters.
@return KErrNone if zone cleared, KErrNoMemory if some pages still allocated in 
the zone (see above), KErrCancel if the defrag operation was cancelled.

@pre RamAlloc mutex held.
@post RamAlloc mutex held.
*/
TInt Defrag::ClearZone(SZone& aZone, TUint aMaxRetries, TRamDefragRequest* aRequest)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ClearZone ID%x retry %d", aZone.iId, aMaxRetries));

	// Attempt to clear all pages from the zone.
	// Keep retrying until no more progress is being made or the retry limit 
	// has been reached
	TUint retryCount = 0;
	for (; 	aZone.iPhysPages != aZone.iFreePages && 
			retryCount < aMaxRetries; 
			retryCount++)
		{
		TUint prevFreePages = aZone.iFreePages;

		// Discard all discardable pages in the zone
		if (ClearDiscardableFromZone(aZone, ETrue, aRequest) == KErrCancel)
			{// Defrag has been cancelled
			return KErrCancel;
			}

		// Remove all the movable pages in the zone
		if (ClearMovableFromZone(aZone, ETrue, aRequest) == KErrCancel)
			{// Defrag has been cancelled
			return KErrCancel;
			}

		if (prevFreePages >= aZone.iFreePages)
			{// i.e. the number of free pages didn't increase so give up
			break;
			}
		}
	if (aZone.iPhysPages != aZone.iFreePages)
		{// Zone couldn't be completely cleared
		return KErrNoMemory;
		}
	return KErrNone;
	}


/**
Perform a general defragmentation of the RAM zones.  Attempt to clear as many
of the lowest preference zones of allocated pages as possible while still respecting
the thresholds and still allowing other allocations to occur.

@param aRequest A TRamDefragRequest object with the iMaxPages member set to 
the maximum number of pages to clear during the defrag operation.

@return KErrNone when done, KErrCancel when the defrag was cancelled.
*/
TInt Defrag::GeneralDefrag(TRamDefragRequest* aRequest)
	{
	TInt ret = KErrNone;
	TUint defragCount = 0;
	TUint stage = EGenDefragStage1;

	// Acquire RAM alloc mutex
	M::RamAllocLock();

	// Determine which stage the general defrag should begin at.
	TUint requiredToDiscard = 0;
	SZone* zone = iRamAllocator->GeneralDefragStart0((TGenDefragStage&)stage, requiredToDiscard);

	// First stage is to clear any discardable pages that are required for
	// movable pages to be allocated into.
	if (aRequest->iMaxPages && aRequest->iMaxPages < requiredToDiscard)
		{// Can't discard the required amount of pages without hitting the 
		// max pages limit so no point continuing.
		__KTRACE_OPT(KMMU, Kern::Printf("GeneralDefrag exit - zone %x max %x requiredToDiscard %x", 
						zone->iId, aRequest->iMaxPages,	requiredToDiscard));
		goto exit;
		}
	while (zone != NULL && requiredToDiscard)
		{		
		TUint prevRequired = requiredToDiscard;
		if (ClearDiscardableFromZone(*zone, EFalse, aRequest, &requiredToDiscard) == KErrCancel)
			{// Defrag cancelled
			ret = KErrCancel;
			goto exit;
			}
		defragCount += prevRequired - requiredToDiscard;
		zone = iRamAllocator->GeneralDefragNextZone0();
		}
	for (; stage < EGenDefragStageEnd; stage++)
		{

		SZone* zone = NULL;
		// Initialise the allocator for the current general defrag stage.
		if (stage == EGenDefragStage1)
			zone = iRamAllocator->GeneralDefragStart1();
		else
			zone = iRamAllocator->GeneralDefragStart2();

		while (zone != NULL)
			{
			if (zone->iAllocPages[EPageMovable] > iRamAllocator->GenDefragFreePages(EPageMovable) ||
				(aRequest->iMaxPages && 
				zone->iAllocPages[EPageMovable] + zone->iAllocPages[EPageDiscard] > aRequest->iMaxPages - defragCount))
				{// Not enough free pages in the more preferable RAM zone(s) or would hit the iMaxPages limit.
				__KTRACE_OPT(KMMU, Kern::Printf("GeneralDefrag exit - zone %x max %x defrag %x", 
								zone->iId, aRequest->iMaxPages,
								zone->iAllocPages[EPageMovable] + zone->iAllocPages[EPageDiscard] + defragCount));
				break;
				}

			// Discard all discardable pages in the zone.
			defragCount += zone->iAllocPages[EPageDiscard];
			if (ClearDiscardableFromZone(*zone, EFalse, aRequest) == KErrCancel)
				{// Defrag cancelled
				ret = KErrCancel;
				goto exit;
				}
			if (zone->iAllocPages[EPageDiscard])
				{// Couldn't discard all the discardable pages so no point continuing.
				__KTRACE_OPT(KMMU, Kern::Printf("GeneralDefrag exit - zone%x Discardable %x", zone->iId, zone->iAllocPages[EPageDiscard]));
				break;
				}

			// Should only have movable pages left in the zone now so shift them 
			// to the higher preference zones.
			defragCount += zone->iAllocPages[EPageMovable];
			if (ClearMovableFromZone(*zone, EFalse, aRequest) == KErrCancel)
				{// Defrag cancelled
				ret = KErrCancel;
				goto exit;
				}
			if (zone->iAllocPages[EPageMovable])
				{// Couldn't move all the movable pages so no point continuing.
				__KTRACE_OPT(KMMU, Kern::Printf("GeneralDefrag exit - zone%x Movable %x", zone->iId, zone->iAllocPages[EPageMovable]));
				break;
				}
			// Get the next RAM zone to be defraged.
			if (stage == EGenDefragStage1)
				zone = iRamAllocator->GeneralDefragNextZone1();
			else
				zone = iRamAllocator->GeneralDefragNextZone2();
			}
		}
exit:
	iRamAllocator->GeneralDefragEnd();
	M::RamAllocUnlock();
	return ret;
	}


/**
Claim a RAM zone by removing all the pages from it and then allocating it as fixed. 

This method may return the following error codes:
- KErrCancel: The call was cancelled, 
- KErrArgument: The specified zone could not be found,
- KErrNoMemory: ClaimRamZone failed; there may be no free zones to move pages to, there may be fixed pages in the zone which can not be moved.

@param aRequest A TRamDefragRequest object with the iId member set to the ID of 
the zone to empty. On success the iPhysAddr member will contain the physical 
base address of the zone that has been claimed.

@return  KErrNone if successful or one of the errors described above.
*/
TInt Defrag::ClaimRamZone(TRamDefragRequest* aRequest)
	{
	TInt ret = KErrNoMemory;

	// Acquire RAM alloc mutex
	M::RamAllocLock();

	SZone* zone = iRamAllocator->ZoneFromId(aRequest->iId);
	if (zone == NULL)
		{// can't find zone
		M::RamAllocUnlock();
		__KTRACE_OPT(KMMU, Kern::Printf("ClaimZone exit - no zone %x", aRequest->iId));
		return KErrArgument;
		}
	
	// Mark the zone as restricted for future allocations
	iRamAllocator->ZoneClaimStart(*zone);

	if (zone->iAllocPages[EPageUnknown] != 0)
		{// Can't ever empty this zone so stop
		__KTRACE_OPT(KMMU, Kern::Printf("ClaimZone exit - zone%x unk%x", zone->iId, zone->iAllocPages[EPageUnknown]));
		goto exit;
		}

	// Attempt to clear all pages from the zone.
	ret = ClearZone(*zone, KDefragMaxRetries, aRequest);

	if (ret == KErrNone)
		{// The zone is empty so claim it
		__KTRACE_OPT(KMMU, Kern::Printf("ClaimZone success - zone%x", zone->iId));
#ifdef BTRACE_RAM_ALLOCATOR
		BTrace4(BTrace::ERamAllocator, BTrace::ERamAllocClaimZone, zone->iId);
#endif

#if defined(BTRACE_KERNEL_MEMORY) && !defined(__MEMMODEL_FLEXIBLE__)
		TUint size = zone->iPhysPages << M::PageShift();
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, size, zone->iPhysBase);
		Epoc::DriverAllocdPhysRam += size;
#endif

		iRamAllocator->MarkPagesAllocated(zone->iPhysBase, zone->iPhysPages, EPageFixed);
		*(aRequest->iPhysAddr) = zone->iPhysBase;
		ret = KErrNone;
		M::RamZoneClaimed(zone);
		}
exit:
	// Release RAM alloc mutex and allow the zone to be allocated into
	iRamAllocator->ZoneClaimEnd(*zone);
	M::RamAllocUnlock();
	return ret;
	}


/**
Empty a RAM zone by removing as many pages as possible from it.

This method may return the following errors:
- KErrCancel: The defrag was cancelled, 
- KErrArgument: The specified zone couldn't be found,
- KErrNoMemory: The zone could not be completely cleared, there may be not enough free zones to move pages to, there may be are fixed pages in the zone.

@param aRequest A TRamDefragRequest object with the iId member set to the ID 
of the zone to empty.

@return KErrNone: Zone emptied or one of the above error codes.
*/
TInt Defrag::EmptyRamZone(TRamDefragRequest* aRequest)
	{
	TInt ret = KErrNone;

	// Acquire RAM alloc mutex
	M::RamAllocLock();

	SZone* zone = iRamAllocator->ZoneFromId(aRequest->iId);
	if (zone == NULL)
		{// can't find zone
		ret = KErrArgument;
		goto exit;
		}

	// Attempt to clear all the pages from the zone
	ret = ClearZone(*zone, KDefragMaxRetries, aRequest);

exit:
	// Release RAM alloc mutex
	M::RamAllocUnlock();
	return ret;
	}


void Defrag::DefragTask(TAny* aArg)
	{
	Defrag& d = *Defrag::TheDefrag;
	TRamDefragRequest* task = (TRamDefragRequest*)aArg;

	TInt r = Kern::SetThreadPriority(task->iThreadPriority, NULL);
	if (r!=KErrNone)
		{
		task->Complete(r);
		return;
		}
	
	d.iDefragPriority = task->iThreadPriority;


	if (task->PollForCancel())
		{
		__KTRACE_OPT(KMMU, Kern::Printf("DefragTask: cancelled"));
		r = KErrCancel;
		goto exit;
		}

	switch (task->iOp)
		{
	case Epoc::ERamDefrag_DefragRam:
		r = d.GeneralDefrag(task);
		break;
	case Epoc::ERamDefrag_EmptyRamZone:
		r = d.EmptyRamZone(task);
		break;
	case Epoc::ERamDefrag_ClaimRamZone:
		r = d.ClaimRamZone(task);
		break;
	default:
		r = KErrNotSupported;
		break;
		}

exit:
	task->Complete(r);
	Kern::SetThreadPriority(KDefragIdlingThreadPriority, NULL);
	}

/**
Constructor for TRamDefragRequest.

@publishedPartner
@released
*/
EXPORT_C TRamDefragRequest::TRamDefragRequest()
	: TAsyncRequest(Defrag::DefragTask, &Defrag::TheDefrag->iTaskQ, 0)
	{
	}


/**
Performs a general defragmentation of RAM. Attempts to free/move as many
pages from the lowest preference RAM zones as possible.

@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.
@param aMaxPages 	The maximum number of pages to move or discard during defragmentation. 
					Zero implies no limit.

@return KErrNone if successful, or KErrArgument if the parameters given are invalid.

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::DefragRam(TInt aPriority, TInt aMaxPages)
	{
	if (aMaxPages < 0 || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_DefragRam;
	iMaxPages = aMaxPages;
	SetupPriority(aPriority);
	return SendReceive();
	}


/**
Performs a general defragmentation of RAM. Attempts to free/move as many
pages from the lowest preference RAM zones as possible. 
The function returns immediately.
When the operation is complete (or cancelled), aSem is signalled.

@param aSem			The fast semaphore to signal on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.
@param aMaxPages 	The maximum number of pages to move or discard during defragmentation. 
					Zero implies no limit.

@return KErrNone if successful, or a system-wide error code.

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::DefragRam(NFastSemaphore* aSem, TInt aPriority, TInt aMaxPages)
	{
	if (aMaxPages < 0 || !aSem || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_DefragRam;
	iMaxPages = aMaxPages;
	SetupPriority(aPriority);
	Send(aSem);
	return KErrNone;
	}


/**
Performs a general defragmentation of RAM. Attempts to free or move as many
pages from the lowest preference RAM zones as possible. 
The function returns immediately.
When the operation is complete (or cancelled), aDfc is enqueued.

@param aDfc			The DFC to enqueue on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.
@param aMaxPages 	The maximum number of pages to move or discard during defragmentation. 
					Zero implies no limit.

@return KErrNone if successful, or a system-wide error code.

@see TDfc

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::DefragRam(TDfc* aDfc, TInt aPriority, TInt aMaxPages)
	{
	if (aMaxPages < 0 || !aDfc || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_DefragRam;
	iMaxPages = aMaxPages;
	SetupPriority(aPriority);
	Send(aDfc);
	return KErrNone;
	}


/**
Removes as many pages from the specified RAM zone as possible.

This method may return the following errors:
- KErrCancel: The defrag was cancelled, 
- KErrArgument: The specified zone couldn't be found, or the parameters are invalid,
- KErrNoMemory: The zone could not be completely cleared, there may be not enough free zones to move pages to, there may be fixed pages in the zone.

@param aId			The ID of the RAM zone to empty.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if successful, see above for errors returned by this method.

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, TInt aPriority)
	{
	if (aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_EmptyRamZone;
	iId = aId;
	SetupPriority(aPriority);
	return SendReceive();
	}


/**
Removes as many pages from the specified RAM zone as possible. The function returns immediately. 
When the operation is complete (or cancelled) aSem is signalled. The result of the request can 
be found by calling TRamDefragRequest::Result(); the following may be returned:
- KErrCancel: The defrag was cancelled, 
- KErrArgument: The specified zone couldn't be found,
- KErrNoMemory: The zone could not be completely cleared, there may be not enough free zones to move pages to, there may be fixed pages in the zone.

@param aId			The ID of the RAM zone to empty.
@param aSem			The fast semaphore to signal on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if request sent or KErrArgument on invalid parameters

@see NFastSemaphore

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, NFastSemaphore* aSem, TInt aPriority)
	{
	if (!aSem || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_EmptyRamZone;
	iId = aId;
	SetupPriority(aPriority);
	Send(aSem);
	return KErrNone;
	}


/**
Removes as many pages from the specified RAM zone as possible. The function returns immediately.
When the operation is complete (or cancelled) aDfc is enqueued. The result of the request can be 
found by calling TRamDefragRequest::Result(); the following may be returned:
- KErrCancel: The defrag was cancelled, 
- KErrArgument: The specified zone couldn't be found,
- KErrNoMemory: The zone could not be completely cleared, there may be not enough free zones to move pages to, there may be fixed pages in the zone.

@param aId			The ID of the RAM zone to empty.
@param aDfc			The DFC to enqueue on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if request sent or KErrArgument on invalid parameters

@see TDfc

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, TDfc* aDfc, TInt aPriority)
	{
	if (!aDfc || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_EmptyRamZone;
	iId = aId;
	SetupPriority(aPriority);
	Send(aDfc);
	return KErrNone;
	}


/**
Attempts to claim the whole of the specified RAM zone.

This method may return the following error codes:
- KErrCancel: The call was cancelled, 
- KErrArgument: aPriority was out of scope or the specified zone could not be found,
- KErrNoMemory: ClaimRamZone failed; may be no free zones to move pages to, there may be fixed pages in the zone which can not be moved.

@param aId			The ID of the RAM zone to claim.
@param aPhysAddr	On success, this holds the base address of the claimed RAM zone
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if successful, or a system-wide error code, see above.

@see TPhysAddr

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TInt aPriority)
	{
	if (aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_ClaimRamZone;
	iId = aId;
	iPhysAddr = &aPhysAddr;
	SetupPriority(aPriority);
	return SendReceive();
	}


/**
Attempts to claim the whole of the specified RAM zone. 
The function returns immediately. When the operation is complete (or cancelled) 
aSem is signalled. The result of the request can be found by calling 
TRamDefragRequest::Result(); the following may be returned:
- KErrNone: The zone was claimed,
- KErrCancel: The call was cancelled, 
- KErrArgument: The specified zone could not be found,
- KErrNoMemory: ClaimRamZone failed; there may be no free zones to move pages to, there may be fixed pages in the zone which can not be moved.

@param aId			The ID of the RAM zone to claim.
@param aPhysAddr	On success, this holds the base address of the claimed RAM zone
@param aSem			The fast semaphore to signal on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if the request was sent or KErrArgument if parameters were invalid.

@see TPhysAddr
@see NFastSemaphore

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, NFastSemaphore* aSem, TInt aPriority)
	{
	if (!aSem || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_ClaimRamZone;
	iId = aId;
	iPhysAddr = &aPhysAddr;
	SetupPriority(aPriority);
	Send(aSem);
	return KErrNone;
	}


/**
Attempts to claim the whole of the specified RAM zone. The function returns immediately.
When the operation is complete (or cancelled) aDfc is enqueued. The result of the request 
can be found by calling TRamDefragRequest::Result(); the following may be returned:
- KErrNone: The zone was claimed,
- KErrCancel: The call was cancelled, 
- KErrArgument: The specified zone could not be found,
- KErrNoMemory: ClaimRamZone failed; there may be no free zones to move pages to, there may be fixed pages in the zone which can not be moved.

@param aId			The ID of the RAM zone to claim.
@param aPhysAddr	On success, this holds the base address of the claimed RAM zone
@param aDfc			The DFC to enqueue on completion of the operation.
@param aPriority	The thread priority for the defragmentation.
					TRamDefragRequest::KInheritPriority to use the priority of the caller.

@return KErrNone if the request was sent or KErrArgument if parameters were invalid.

@see TPhysAddr
@see TDfc

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TDfc* aDfc, TInt aPriority)
	{
	if (!aDfc || aPriority < -1 || aPriority >= KNumPriorities)
		return KErrArgument;
	
	iOp = Epoc::ERamDefrag_ClaimRamZone;
	iId = aId;
	iPhysAddr = &aPhysAddr;
	SetupPriority(aPriority);
	Send(aDfc);
	return KErrNone;
	}


/**
Retrieves the result of the last request. This value is only valid if notification of
completion has been received (via DFC callback or by waiting on the semaphore).

@return KErrNone if the last request was successful, or a system-wide error code.

@publishedPartner
@released
*/
EXPORT_C TInt TRamDefragRequest::Result()
	{
	return iResult;
	}


/**
Cancel the request. If the operation has already started, it terminates at the
next opportunity. This function has no effect if no request has been made or if 
the request has already finished.

@publishedPartner
@released
*/
EXPORT_C void TRamDefragRequest::Cancel()
	{
	TAsyncRequest::Cancel();
	}

void TRamDefragRequest::SetupPriority(TInt aPriority)
	{
	if (aPriority == KInheritPriority)
		iThreadPriority = NCurrentThread()->iPriority;
	else
		iThreadPriority = aPriority;

	const TUint KPriorityDivisor = (TUint) ((KNumPriorities + KNumDfcPriorities - 1) / KNumDfcPriorities);
	SetPriority(iThreadPriority / KPriorityDivisor);
	}
