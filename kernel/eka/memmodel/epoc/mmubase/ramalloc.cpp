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
// e32\memmodel\epoc\mmubase\ramalloc.cpp
// 
//

/**
 @file
 @internalComponent
*/
//#define __VERIFY_LEASTMOVDIS

#include <plat_priv.h>
#include <ramalloc.h>
#include <e32btrace.h>

#ifndef __MEMMODEL_FLEXIBLE__
#include <mmubase.inl>
#else
#include "mdefrag.inl"
#endif //__MEMMODEL_FLEXIBLE__

DRamAllocator* DRamAllocator::New()
	{
	return new DRamAllocator;
	}

DRamAllocator* DRamAllocator::New(const SRamInfo& aInfo, const SRamZone* aZoneInfo, TRamZoneCallback aZoneCallback)
	{
	DRamAllocator* pA=New();
	if (!pA)
		Panic(ECreateNoMemory);
	// If this fails then it won't return but panic
	pA->Create(aInfo,aZoneInfo, aZoneCallback);
	return pA;
	}

void DRamAllocator::Panic(TPanic aPanic)
	{
	Kern::Fault("RAM-ALLOC", aPanic);
	}

#ifdef KMMU
void HexDump32(const TAny* a, TInt n, const char* s)
	{
	const TUint32* p=(const TUint32*)a;
	Kern::Printf(s);
	TInt i=0;
	while(n)
		{
		TBuf8<80> b;
		b.AppendNumFixedWidth(i,EHex,4);
		b.Append(':');
		TInt m=Min(n,4);
		n-=m;
		i+=m;
		while(m--)
			{
			b.Append(' ');
			b.AppendNumFixedWidth(*p++,EHex,8);
			}
		Kern::Printf("%S",&b);
		}
	}

void HexDump8(const TAny* a, TInt n, const char* s)
	{
	const TUint8* p=(const TUint8*)a;
	Kern::Printf(s);
	TInt i=0;
	while(n)
		{
		TBuf8<80> b;
		b.AppendNumFixedWidth(i,EHex,4);
		b.Append(':');
		TInt m=Min(n,16);
		n-=m;
		i+=m;
		while(m--)
			{
			b.Append(' ');
			b.AppendNumFixedWidth(*p++,EHex,2);
			}
		Kern::Printf("%S",&b);
		}
	}

void DRamAllocator::DebugDump()
	{
	Kern::Printf("PageSize=%08x PageShift=%d",KPageSize,KPageShift);
	Kern::Printf("Total Pages=%x Total Free=%x",iTotalRamPages,iTotalFreeRamPages);
	Kern::Printf("Number of zones=%d, PowerState=%016lx",iNumZones,iZonePwrState);
	Kern::Printf("PhysAddrBase=%08x, PhysAddrTop=%08x",iPhysAddrBase,iPhysAddrTop);

	TUint i = 0;
	Kern::Printf("Zone Info:");
	for (; i<iNumZones; ++i)
		{
		SZone& z=iZones[i];
		TBitMapAllocator& b = *(z.iBma[KBmaAllPages]);
		Kern::Printf("%x: Avail %x Size %x Phys %08x PhysEnd %08x ID %08x FreePage %x Pref %02x",i,b.iAvail,b.iSize,
										z.iPhysBase, z.iPhysEnd, z.iId,z.iFreePages, z.iPref);
		Kern::Printf("Allocated Unknown %x Fixed %x Movable %x Discardable %x",iZones[i].iAllocPages[EPageUnknown],iZones[i].iAllocPages[EPageFixed],
										iZones[i].iAllocPages[EPageMovable],iZones[i].iAllocPages[EPageDiscard]);
		}

	Kern::Printf("Zone pref order:");
	SDblQueLink* link = iZonePrefList.First();
	for (; link != &iZonePrefList.iA; link = link->iNext)
		{
		SZone& zone = *_LOFF(link, SZone, iPrefLink);
		Kern::Printf("ID0x%x rank0x%x", zone.iId, zone.iPrefRank);
		}
	SZone& zone = *_LOFF(iZoneLeastMovDis, SZone, iPrefLink);
	Kern::Printf("iZoneLeastMovDis ID 0x%x rank 0x%x", zone.iId, iZoneLeastMovDisRank);
	}
#endif

TInt CountBanks(const SRamBank* aBankList)
	{
	TInt banks=0;
	for (; aBankList->iSize; ++banks, ++aBankList);
	return banks;
	}

TUint32 TotalBankSize(const SRamBank* aBankList)
	{
	TUint32 size=0;
	for (; aBankList->iSize; ++aBankList)
		size+=aBankList->iSize;
	return size;
	}

/**
Count how many zones have been specified and do some basic checks on their layout:
	Zones must be distinct, i.e. not overlap
	Zone ID must be unique
	Zones must be page size aligned
	Zones must be big enough to cover all of the allocatable RAM
The end of the list is indicated by a SRamZone.iSize==0. 
@param aZones The list of RAM zones to be setup
*/
void DRamAllocator::CountZones(const SRamZone* aZones)
	{
	TUint32 totalSize = 0;
	TUint32 pageMask = KPageSize-1;
	// Check zones don't overlap each other and while running through the zones
	// calculate how many there are
	const SRamZone* pCurZ = aZones;
	for (; pCurZ->iSize != 0; pCurZ++)
		{
		// Verify zone addresses and alignment
		TUint32 curEnd = pCurZ->iBase + pCurZ->iSize - 1;
		__KTRACE_OPT(KMMU,Kern::Printf("curBase %x curEnd %x pageMask %x",pCurZ->iBase,curEnd,pageMask));
		if (curEnd <= pCurZ->iBase || (((curEnd + 1) | pCurZ->iBase) & pageMask))
			{
			Panic(EZonesAlignment);
			}
		
		if (pCurZ->iId == KRamZoneInvalidId)
			{
			Panic(EZonesIDInvalid);
			}
		// Check the flags are not set to invalid values
		if (pCurZ->iFlags & KRamZoneFlagInvalid)
			{
			Panic(EZonesFlagsInvalid);
			}

		iNumZones++;
		if (iNumZones > KMaxRamZones)
			{// Too many zones specified
			Panic(EZonesTooNumerousOrFew);
			}
		totalSize += pCurZ->iSize;
		
		// Verify this zone doesn't overlap any of the previous zones' address space
		const SRamZone* pTmpZ = aZones;
		for (; pTmpZ < pCurZ; pTmpZ++)
			{
			TUint32 tmpEnd = pTmpZ->iBase + pTmpZ->iSize - 1;
			if (tmpEnd >= pCurZ->iBase && pTmpZ->iBase <= curEnd)
				{
				Panic(EZonesNotDistinct);
				}
			if(pTmpZ->iId == pCurZ->iId)
				{
				Panic(EZonesIDNotUnique);
				}
			}
		}
	__KTRACE_OPT(KMMU,Kern::Printf("iNumZones=%d, totalSize=%x",iNumZones,totalSize));
	if (!iNumZones)
		{// no zones specified
		Panic(EZonesTooNumerousOrFew);
		}

	// Together all of the zones should cover the whole of the RAM
	if (totalSize>>KPageShift < iTotalRamPages)
		{
		Panic(EZonesIncomplete);
		}
	}


/**
Get the zone from the ID
@param aId ID of zone to find
@return Pointer to the zone if zone of matching ID found, NULL otherwise
*/
SZone* DRamAllocator::ZoneFromId(TUint aId) const
	{
	SZone* pZ = iZones;
	const SZone* const pEndZone = iZones + iNumZones;
	for (; pZ < pEndZone; pZ++)
		{
		if (aId == pZ->iId)
			{
			return pZ;
			}
		}
	return NULL;
	}

/** Retrieve the physical base address and number of pages in the specified zone.

@param	aZoneId	The ID of the zone
@param 	aPhysBaseAddr	Receives the base address of the zone
@param	aNumPages	Receives the number of pages in the zone

@return KErrNone if zone found, KErrArgument if zone couldn't be found
*/
TInt DRamAllocator::GetZoneAddress(TUint aZoneId, TPhysAddr& aPhysBase, TUint& aNumPages)
	{
	SZone* zone = ZoneFromId(aZoneId);
	if (zone == NULL)
		{
		return KErrArgument;
		}
	aPhysBase = zone->iPhysBase;
	aNumPages = zone->iPhysPages;
	return KErrNone;
	}

#ifdef __MEMMODEL_FLEXIBLE__
/**
@param aAddr The address of page to find the zone of
@param aOffset The page offset from the start of the zone that the page is in
*/
SZone* DRamAllocator::GetZoneAndOffset(TPhysAddr aAddr, TInt& aOffset)
	{
	// Get the zone from the SPageInfo of the page at aAddr
	SPageInfo* pageInfo = SPageInfo::SafeFromPhysAddr(aAddr);
	if (pageInfo == NULL)
		{
		return NULL;
		}

	// Perform a binary search for the RAM zone, we know aAddr is within a RAM 
	// zone as pageInfo != NULL.
	SZone* left = iZones;
	SZone* mid = iZones + (iNumZones>>1);
	SZone* top = iZones + iNumZones - 1;

	while (mid->iPhysEnd < aAddr || mid->iPhysBase > aAddr)
		{
		if (mid->iPhysEnd < aAddr)
			left = mid + 1;
		else
			top = mid - 1;
		mid = left + ((top - left) >> 1);
		__ASSERT_DEBUG(left <= top && mid <= top && mid >= left, Panic(EAllocRamPagesInconsistent));
		}
	__ASSERT_DEBUG(mid->iPhysBase <= aAddr && mid->iPhysEnd >= aAddr, Panic(EAllocRamPagesInconsistent));
	aOffset = (aAddr - mid->iPhysBase) >> KPageShift;
	__ASSERT_DEBUG((TUint)aOffset < mid->iPhysPages, Panic(EAllocRamPagesInconsistent));
	return mid;
	}
#else
/**
@param aAddr The address of page to find the zone of
@param aOffset The page offset from the start of the zone that the page is in
*/
SZone* DRamAllocator::GetZoneAndOffset(TPhysAddr aAddr, TInt& aOffset)
	{
	// Get the zone from the SPageInfo of the page at aAddr
	SPageInfo* pageInfo = SPageInfo::SafeFromPhysAddr(aAddr);
	if (pageInfo == NULL)
		{
		return NULL;
		}
	SZone* z = iZones + pageInfo->Zone();
	aOffset = (aAddr - z->iPhysBase) >> KPageShift;
	__ASSERT_DEBUG((TUint)aOffset < z->iPhysPages, Panic(EAllocRamPagesInconsistent));
	return z;
	}
#endif
/**
@param aId ID of zone to get page count for
@param aPageData store for page counts
@return KErrNone if zone found, KErrArgument otherwise
*/
TInt DRamAllocator::GetZonePageCount(TUint aId, SRamZonePageCount& aPageData)
	{
	// Search for the zone of ID aId
	const SZone* zone = ZoneFromId(aId);
	if (zone == NULL)
		{
		return KErrArgument;
		}
	aPageData.iFreePages = zone->iFreePages;
	aPageData.iUnknownPages = zone->iAllocPages[EPageUnknown];
	aPageData.iFixedPages = zone->iAllocPages[EPageFixed];
	aPageData.iMovablePages = zone->iAllocPages[EPageMovable];
	aPageData.iDiscardablePages = zone->iAllocPages[EPageDiscard];

	return KErrNone;
	}


/** Update the count of free and allocated pages for the zone with
@param aZone The index of the zone whose counts are being updated
@param aCount The no of pages being allocated
@param aType The type of the pages being allocated
*/
void DRamAllocator::ZoneAllocPages(SZone* aZone, TUint32 aCount, TZonePageType aType)
	{
#ifdef _DEBUG
	TUint32 free = aZone->iFreePages - aCount;
	TUint32 alloc = aZone->iAllocPages[aType] + aCount;
	TUint32 total_alloc = 	aZone->iAllocPages[EPageUnknown] +
							aZone->iAllocPages[EPageDiscard] + 
							aZone->iAllocPages[EPageMovable] + 
							aZone->iAllocPages[EPageFixed] + aCount;
	if (free > aZone->iFreePages || 
		alloc < aZone->iAllocPages[aType] ||
		free + total_alloc != aZone->iPhysPages ||
		iTotalFreeRamPages > iTotalRamPages)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("TotalFree %x TotalPages %x",iTotalFreeRamPages, iTotalRamPages));
		__KTRACE_OPT(KMMU,Kern::Printf("ZoneFreePages - aCount %x free %x, alloc %x",aCount,free,alloc));	// counts rolled over
		__KTRACE_OPT(KMMU,Kern::Printf("Alloc Unk %x Fx %x Mv %x Dis %x",aZone->iAllocPages[EPageUnknown], 
					aZone->iAllocPages[EPageFixed],	aZone->iAllocPages[EPageMovable],aZone->iAllocPages[EPageDiscard]));
		Panic(EZonesCountErr);
		}
	__KTRACE_OPT(KMMU2,Kern::Printf("ZoneFreePages - aCount %x free %x, alloc %x",aCount,free,alloc));
	__KTRACE_OPT(KMMU2,Kern::Printf("Alloc Unk %x Fx %x Mv %x Dis %x",aZone->iAllocPages[EPageUnknown], 
					aZone->iAllocPages[EPageFixed],	aZone->iAllocPages[EPageMovable],aZone->iAllocPages[EPageDiscard]));

	if (!iContiguousReserved)
		{
		__ASSERT_DEBUG(free == (TUint32)aZone->iBma[KBmaAllPages]->iAvail, Panic(EAllocRamPagesInconsistent));
		TBitMapAllocator& bmaType = *(aZone->iBma[(aType != EPageUnknown)? aType : EPageFixed]);
		TUint allocPages;
		if (aType == EPageFixed || aType == EPageUnknown)
			allocPages = aZone->iAllocPages[EPageUnknown] + aZone->iAllocPages[EPageFixed];
		else
			allocPages = aZone->iAllocPages[aType];
		allocPages += aCount;
		__NK_ASSERT_DEBUG(aZone->iPhysPages - bmaType.iAvail == allocPages);
		__NK_ASSERT_DEBUG((TUint)bmaType.iAvail >= aZone->iFreePages - aCount);

//#define _FULL_VERIFY_TYPE_BMAS
#ifdef _FULL_VERIFY_TYPE_BMAS
		TUint offset = 0;
		TUint matchedPages = 0;
		TInt r = KErrNone;
		while (offset < aZone->iPhysPages && r == KErrNone)
			{
			r = NextAllocatedPage(aZone, offset, EPageTypes);
			if (bmaType.NotFree(offset, 1))
				{
				matchedPages++;
				}
			offset++;
			}
		__NK_ASSERT_DEBUG(matchedPages == allocPages);
#endif
		}
#endif

	// Update counts
	aZone->iAllocPages[aType] += aCount;
	aZone->iFreePages -= aCount;
	aZone->iFlags &= ~KRamZoneFlagMark;	// clear the mark as this zone is active

	// Check if power state of zone needs to be changed
	if (iZonePowerFunc && !(iZonePwrState & (((TUint64)1) << aZone - iZones)))
		{//zone no longer empty so call variant to power RAM zone up if necessary
		iZonePwrState |= (((TUint64)1) << aZone - iZones);

		if (iZoneCallbackInitSent)
			{
			TInt ret = (*iZonePowerFunc)(ERamZoneOp_PowerUp, (TAny*)aZone->iId, (TUint*)&iZonePwrState);
			if (ret != KErrNone && ret != KErrNotSupported)
				{
				Panic(EZonesCallbackErr);
				}
			CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "DRamAllocator::ZoneAllocPages");
			}
		}

	// Re-order the zone preference list so that a RAM zone with more immovable pages 
	// is more preferable and secondary to that a RAM zone that is not empty is more
	// preferable than one that is empty.
	while (&aZone->iPrefLink != iZonePrefList.First())
		{
		SZone* prevZ = _LOFF(aZone->iPrefLink.iPrev, SZone, iPrefLink);
		__NK_ASSERT_DEBUG(K::Initialising || prevZ->iPrefRank == aZone->iPrefRank - 1);
		if (prevZ->iPref == aZone->iPref && 
			(prevZ->iAllocPages[EPageFixed] + prevZ->iAllocPages[EPageUnknown] < 
			aZone->iAllocPages[EPageFixed] + aZone->iAllocPages[EPageUnknown] ||
			prevZ->iFreePages == prevZ->iPhysPages))
			{
			__KTRACE_OPT(KMMU, Kern::Printf("a - Reorder aZone 0x%x free 0x%x before prevZ 0x%x free 0x%x", aZone->iId, aZone->iFreePages, prevZ->iId, prevZ->iFreePages));
			// Make this RAM zone more preferable.
			aZone->iPrefLink.Deque();
			aZone->iPrefLink.InsertBefore(&prevZ->iPrefLink);
			aZone->iPrefRank--;
			prevZ->iPrefRank++;

			if (iZoneLeastMovDis == &prevZ->iPrefLink)
				{// Ensure iZoneLeastMovDisRank is kept up to date.
				iZoneLeastMovDisRank = prevZ->iPrefRank;
				}
			if (iZoneLeastMovDis == &aZone->iPrefLink)
				{// Ensure iZoneLeastMovDisRank is kept up to date.
				iZoneLeastMovDisRank = aZone->iPrefRank;		
				// aZone was the least preferable with movable and/or discardable so is it still?		
				if (prevZ->iAllocPages[EPageMovable] || prevZ->iAllocPages[EPageDiscard])
					{// prevZ is now the least preferable RAM zone with movable and/or discardable.
					iZoneLeastMovDis = &prevZ->iPrefLink; 
					iZoneLeastMovDisRank = prevZ->iPrefRank;
					__KTRACE_OPT(KMMU, Kern::Printf("aa - iZoneleastInUse ID 0x%x", (_LOFF(iZoneLeastMovDis, SZone, iPrefLink))->iId));
					}
				__KTRACE_OPT(KMMU, Kern::Printf("iZoneLeastMovDisRank 0x%x", iZoneLeastMovDisRank));
				}
			}
		else
			{
			break;
			}
		}

	// Now that the preference list has been re-ordered check whether
	// iZoneLeastMovDis needs updating.
	if (aType >= EPageMovable && iZoneLeastMovDisRank < aZone->iPrefRank)
		{
		iZoneLeastMovDis = &aZone->iPrefLink;
		iZoneLeastMovDisRank = aZone->iPrefRank;
		__KTRACE_OPT(KMMU, Kern::Printf("a - iZoneleastInUse ID 0x%x", (_LOFF(iZoneLeastMovDis, SZone, iPrefLink))->iId));
		}
	__NK_ASSERT_DEBUG(	K::Initialising || 
						iZoneLeastMovDisRank == _LOFF(iZoneLeastMovDis, SZone, iPrefLink)->iPrefRank);
#ifdef __VERIFY_LEASTMOVDIS
	if (!K::Initialising)
		VerifyLeastPrefMovDis();
#endif
	}


/** Update the count of free and allocated pages for the zone with
@param aZone The index of the zone whose counts are being updated
@param aCount The no of pages being freed
@param aType The type of the pages being freed
*/
void DRamAllocator::ZoneFreePages(SZone* aZone, TUint32 aCount, TZonePageType aType)
	{
#ifdef _DEBUG
	TUint32 alloc = aZone->iAllocPages[aType] - aCount;
	TUint32 free = aZone->iFreePages + aCount;
	TUint32 total_alloc = 	aZone->iAllocPages[EPageUnknown] +
							aZone->iAllocPages[EPageDiscard] + 
							aZone->iAllocPages[EPageMovable] + 
							aZone->iAllocPages[EPageFixed] - aCount;
	if (free < aZone->iFreePages ||
		alloc > aZone->iAllocPages[aType] ||
		free + total_alloc != aZone->iPhysPages ||
		iTotalFreeRamPages > iTotalRamPages)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("TotalFree %x TotalPages %x",iTotalFreeRamPages, iTotalRamPages));
		__KTRACE_OPT(KMMU,Kern::Printf("ZoneFreePages - aCount %x free %x, alloc %x",aCount,free,alloc));	// counts rolled over
		__KTRACE_OPT(KMMU,Kern::Printf("Alloc Unk %x Fx %x Mv %x Dis %x",aZone->iAllocPages[EPageUnknown], 
					aZone->iAllocPages[EPageFixed],	aZone->iAllocPages[EPageMovable],aZone->iAllocPages[EPageDiscard]));
		Panic(EZonesCountErr);
		}
	__KTRACE_OPT(KMMU2,Kern::Printf("ZoneFreePages - aCount %x free %x, alloc %x",aCount,free,alloc));
	__KTRACE_OPT(KMMU2,Kern::Printf("Alloc Unk %x Fx %x Mv %x Dis %x",aZone->iAllocPages[EPageUnknown], 
					aZone->iAllocPages[EPageFixed],	aZone->iAllocPages[EPageMovable],aZone->iAllocPages[EPageDiscard]));

	if (!iContiguousReserved)
		{
		__ASSERT_DEBUG(free == (TUint32)aZone->iBma[KBmaAllPages]->iAvail, Panic(EAllocRamPagesInconsistent));
		TBitMapAllocator& bmaType = *(aZone->iBma[(aType != EPageUnknown)? aType : EPageFixed]);
		TUint allocPages;
		if (aType == EPageFixed || aType == EPageUnknown)
			allocPages = aZone->iAllocPages[EPageUnknown] + aZone->iAllocPages[EPageFixed];
		else
			allocPages = aZone->iAllocPages[aType];
		allocPages -= aCount;
		__NK_ASSERT_DEBUG(aZone->iPhysPages - bmaType.iAvail == allocPages);
		__NK_ASSERT_DEBUG((TUint)bmaType.iAvail >= aZone->iFreePages + aCount);

#ifdef _FULL_VERIFY_TYPE_BMAS
		TUint offset = 0;
		TUint matchedPages = 0;
		TInt r = KErrNone;
		while(offset < aZone->iPhysPages && r == KErrNone)
			{
			r = NextAllocatedPage(aZone, offset, EPageTypes);
			if (bmaType.NotFree(offset, 1))
				{
				matchedPages++;
				}
			offset++;
			}
		__NK_ASSERT_DEBUG(matchedPages == allocPages);
#endif
		}
#endif

	// Update counts
	aZone->iAllocPages[aType] -= aCount;
	aZone->iFreePages += aCount;
	aZone->iFlags &= ~KRamZoneFlagMark;	// clear the mark as this zone is active

	// Check if power state of zone needs to be changed.
	//	Don't update iZonePwrState when a zone is being cleared to then be 
	//	claimed as it shouldn't be powered off as it's about to be used.
	if (iZonePowerFunc && !(aZone->iFlags & KRamZoneFlagClaiming) &&
		aZone->iFreePages == aZone->iPhysPages)
		{// Zone is empty so call variant to power down RAM zone if desirable.
		TUint64 pwrMask = ~(((TUint64)1) << aZone - iZones);
		iZonePwrState &= pwrMask;

		// Don't invoke callback until Init callback sent.
		if (iZoneCallbackInitSent)
			{
			TInt ret = (*iZonePowerFunc)(ERamZoneOp_PowerDown, (TAny*)aZone->iId, (TUint*)&iZonePwrState);
			if (ret != KErrNone && ret != KErrNotSupported)
				{
				Panic(EZonesCallbackErr);
				}
			CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "DRamAllocator::ZoneFreePages");
			}
		}

	// Re-order the zone preference list so that a RAM zone with more immovable pages 
	// is more preferable and secondary to that a RAM zone that is not empty is more
	// preferable than one that is empty.
	while (&aZone->iPrefLink != iZonePrefList.Last())
		{
		SZone* nextZ = _LOFF(aZone->iPrefLink.iNext, SZone, iPrefLink);
		__NK_ASSERT_DEBUG(K::Initialising || nextZ->iPrefRank == aZone->iPrefRank + 1);
		if (nextZ->iPref == aZone->iPref && 
			(nextZ->iAllocPages[EPageFixed] + nextZ->iAllocPages[EPageUnknown] >
			aZone->iAllocPages[EPageFixed] + aZone->iAllocPages[EPageUnknown] ||
			(nextZ->iFreePages != nextZ->iPhysPages &&
			aZone->iFreePages == aZone->iPhysPages)))
			{
			__KTRACE_OPT(KMMU, Kern::Printf("f - Reorder aZone 0x%x free 0x%x after nextZ 0x%x free 0x%x", aZone->iId, aZone->iFreePages, nextZ->iId, nextZ->iFreePages));
			// Make this RAM zone less preferable.
			aZone->iPrefLink.Deque();
			aZone->iPrefLink.InsertAfter(&nextZ->iPrefLink);
			aZone->iPrefRank++;
			nextZ->iPrefRank--;

			if (iZoneLeastMovDis == &aZone->iPrefLink)
				{// Ensure iZoneLeastMovDisRank is kept up to date.
				iZoneLeastMovDisRank = aZone->iPrefRank;
				}
			if (iZoneLeastMovDis == &nextZ->iPrefLink)
				{// Ensure iZoneLeastMovDisRank is kept up to date.
				iZoneLeastMovDisRank = nextZ->iPrefRank;
				if (aZone->iAllocPages[EPageMovable] || aZone->iAllocPages[EPageDiscard])
					{// aZone is now the least preferable RAM zone with movable and/or discardable.
					iZoneLeastMovDis = &aZone->iPrefLink;
					iZoneLeastMovDisRank = aZone->iPrefRank;
					__KTRACE_OPT(KMMU, Kern::Printf("aa - iZoneleastInUse ID 0x%x", (_LOFF(iZoneLeastMovDis, SZone, iPrefLink))->iId));
					}
				__KTRACE_OPT(KMMU, Kern::Printf("iZoneLeastMovDis Rank 0x%x", iZoneLeastMovDisRank));
				}
			}
		else
			{
			break;
			}
		}
	if (&aZone->iPrefLink == iZoneLeastMovDis && 
		!aZone->iAllocPages[EPageMovable] && !aZone->iAllocPages[EPageDiscard])
		{// This RAM zone no longer has movable or discardable and therefore it 
		// is also no longer the least preferable RAM zone with movable and/or 
		// discardable.
		SZone* zonePrev;
		do 
			{
			iZoneLeastMovDis = iZoneLeastMovDis->iPrev;
			iZoneLeastMovDisRank--;
			if (iZoneLeastMovDis == iZonePrefList.First())
				{// This the most preferable RAM zone so can't go any further.
				break;
				}
			zonePrev = _LOFF(iZoneLeastMovDis, SZone, iPrefLink);
			__KTRACE_OPT(KMMU, Kern::Printf("f - iZoneLeastMovDis 0x%x", zonePrev->iId));
			}
		while (!zonePrev->iAllocPages[EPageMovable] && !zonePrev->iAllocPages[EPageDiscard]);

	__NK_ASSERT_DEBUG(	K::Initialising || 
						iZoneLeastMovDisRank == _LOFF(iZoneLeastMovDis, SZone, iPrefLink)->iPrefRank);

#ifdef __VERIFY_LEASTMOVDIS
		if (!K::Initialising)
			VerifyLeastPrefMovDis();
#endif
		}
	}


/** Calculate the physical address order of the zones and temporally store
	the order in aZoneAddrOrder
*/
inline void DRamAllocator::SortRamZones(const SRamZone* aZones, TUint8* aZoneAddrOrder)
	{
	const SRamZone* const endZone = aZones + iNumZones;
	const SRamZone* zone = aZones;
	for (; zone < endZone; zone++)
		{
		// zoneIdx is the number of zones that have a lower base address than the 
		// current zone and therefore it is the address index of the current zone
		TInt zoneIdx = 0;
		// search for any zones of lower base address
		const SRamZone* zone2 = aZones;
		for (; zone2 < endZone; zone2++)
			{
			if (zone2->iBase < zone->iBase)
				{
				zoneIdx++; // have another zone of lower base address
				}
			}
		aZoneAddrOrder[zoneIdx] = zone - aZones;
		}
	}


/** Initialise SPageInfos for all pages in this zone with the 
index of the zone.
@param aZone The zone the pages to be initialised are in
*/
inline TUint DRamAllocator::InitSPageInfos(const SZone* aZone)
	{
	TUint pagesUpdated = 0;
	if (aZone->iPhysBase > iPhysAddrTop || aZone->iPhysEnd < iPhysAddrBase)
		{// None of the zone is in allocatable RAM
		return pagesUpdated;
		}

	// Mark each allocatable page in this zone with the index of the zone
#ifndef __MEMMODEL_FLEXIBLE__
	TUint8 zoneIndex = aZone - iZones;
#endif
	TPhysAddr addr = aZone->iPhysBase;
	for (; addr <= aZone->iPhysEnd; addr += KPageSize)
		{
		SPageInfo* pi = SPageInfo::SafeFromPhysAddr(addr);
		if (pi)
			{
#ifndef __MEMMODEL_FLEXIBLE__	// The FMM doesn't store zone indices in SPageInfos.
			pi->SetZone(zoneIndex);
#endif
			pagesUpdated++;
			}
		}
	return pagesUpdated;
	}

/** HAL Function for the RAM allocator.
*/
TInt DRamAllocator::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{
	switch(aFunction)
		{
		case ERamHalGetZoneCount:
			{
			kumemput32(a1, &iNumZones, sizeof(iNumZones));
			return KErrNone;
			}

		case ERamHalGetZoneConfig:
			{
			TUint zoneIndex = (TUint)a1;
			if (zoneIndex < iNumZones)
				{
				SZone* pZone = iZones + zoneIndex;
				struct SRamZoneConfig config;
				NKern::ThreadEnterCS();
				M::RamAllocLock(); // get mutex to ensure consistent set of values are read...
				config.iZoneId         = pZone->iId;
				config.iZoneIndex      = zoneIndex;		
				config.iPhysBase       = pZone->iPhysBase;
				config.iPhysEnd        = pZone->iPhysEnd;
				config.iPhysPages      = pZone->iPhysPages;
				config.iPref		   = pZone->iPref;
				config.iFlags		   = pZone->iFlags;
				M::RamAllocUnlock();
				NKern::ThreadLeaveCS();
				kumemput32(a2,&config,sizeof(config));
				return KErrNone;
				}
			return KErrNotFound;
			}
		
		case ERamHalGetZoneUtilisation:
			{
			TUint zoneIndex = (TUint)a1;
			if (zoneIndex < iNumZones)
				{
				SZone* pZone = iZones + zoneIndex;
				struct SRamZoneUtilisation config;
				NKern::ThreadEnterCS();
				M::RamAllocLock(); // get mutex to ensure consistent set of values are read...
				config.iZoneId			 = pZone->iId;
				config.iZoneIndex		 = zoneIndex;
				config.iPhysPages		 = pZone->iPhysPages;
				config.iFreePages		 = pZone->iFreePages;
				config.iAllocUnknown	 = pZone->iAllocPages[EPageUnknown];
				config.iAllocFixed		 = pZone->iAllocPages[EPageFixed];
				config.iAllocMovable	 = pZone->iAllocPages[EPageMovable];
				config.iAllocDiscardable = pZone->iAllocPages[EPageDiscard];
				config.iAllocOther		 = 0;
				M::RamAllocUnlock();
				NKern::ThreadLeaveCS();
				kumemput32(a2,&config,sizeof(config));
				return KErrNone;
				}
			return KErrNotFound;
			}

		default:
			{
			return KErrNotSupported;
			}
		}
	}

/**
Setup the ram allocator with information of the RAM available in the system that 
comes from the bootstrap/superpage.  This is intended to be called from 
DRamAllocator::New().
@internalComponent
@see DRamAllocator::New()
@param aInfo Two lists of SRamBanks for available and reserved banks in RAM, respectively
@param aZones A list of the ram zones in the system and their configuration/preferences
@param aZoneCallback Pointer to a base port call back function that will be invoked by this class
*/
void DRamAllocator::Create(const SRamInfo& aInfo, const SRamZone* aZones, TRamZoneCallback aZoneCallback)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::Create"));

	// SZone::iBma array assumes this and KBmaAllPages can't be the same as any 
	// allocatable page type.
	__ASSERT_COMPILE(EPageFixed < KPageImmovable && EPageUnknown < KPageImmovable &&
					EPageDiscard >= KPageImmovable && EPageMovable >= KPageImmovable &&
					KBmaAllPages != EPageFixed && KBmaAllPages != EPageMovable && 
					KBmaAllPages != EPageDiscard);
	// NoAllocOfPageType() requires this
	__ASSERT_COMPILE(	KRamZoneFlagNoFixed == 1 << (EPageFixed - KPageTypeAllocBase) && 
						KRamZoneFlagNoMovable == 1 << (EPageMovable - KPageTypeAllocBase) &&
						KRamZoneFlagNoDiscard == 1 << (EPageDiscard - KPageTypeAllocBase));
						
	// SZone::iPhysEnd and iPhysAddrTop rely on this when checking contiguous zones etc.
	__ASSERT_COMPILE(KPageShift != 0);

	///////////////////////////////////////////////////////////////////////////
	//	Determine where all the allocatable RAM pages are, using the SRamBank
	//	data passed to the kernel by the bootstrap
	//////////////////////////////////////////////////////////////////////////
	TUint num_boot_banks=CountBanks(aInfo.iBanks);
	TUint32 total_ram_size=TotalBankSize(aInfo.iBanks);
	__KTRACE_OPT(KMMU,Kern::Printf("#banks from bootstrap=%d",num_boot_banks));
	__KTRACE_OPT(KMMU,Kern::Printf("Total size=%08x",total_ram_size));
	iTotalRamPages=total_ram_size>>KPageShift;
 	// Assume all pages are allocated as unknown for now
	iTotalFreeRamPages = 0;
	__KTRACE_OPT(KMMU,Kern::Printf("Total size=%08x, total pages=%08x",total_ram_size,iTotalRamPages));

	iPhysAddrBase=aInfo.iBanks[0].iBase;
	const SRamBank& last_boot_bank=aInfo.iBanks[num_boot_banks-1];
	iPhysAddrTop = last_boot_bank.iBase + last_boot_bank.iSize - 1;
	__KTRACE_OPT(KMMU,Kern::Printf("PA base=%08x, PA top=%08x",iPhysAddrBase,iPhysAddrTop));

	__ASSERT_DEBUG(iPhysAddrTop > iPhysAddrBase, Panic(ECreateInvalidRamBanks));

	
	///////////////////////////////////////////////////////////////////////////
	//	Determine how many zones are required and allocate all the 
	//	data structures that will be required, permanent one first then
	//	temporary ones to avoid kernel heap fragmentation.
	///////////////////////////////////////////////////////////////////////////
	// Stop any RAM zone callback operations until the initial one has been sent
	iZoneCallbackInitSent = EFalse;
	if (aZones)
		{
		CountZones(aZones);
		iZonePowerFunc = aZoneCallback;
		}
	else
		{// maximum number of zone is number of non-coalesced boot banks
		iNumZones = num_boot_banks;
		// No zones specified so don't worry about invoking callback function
		iZonePowerFunc = NULL;
		}
	
	// Permenant heap allocation #1 - may be resized if no zones specified
	__KTRACE_OPT(KMMU,Kern::Printf("iNumZones=%d", iNumZones));
	iZones = (SZone*)Kern::AllocZ(iNumZones*sizeof(SZone));
	if (!iZones)
		{
		Panic(ECreateNoMemory);
		}

	///////////////////////////////////////////////////////////////////////////
	//	Coalesce contiguous boot banks
	///////////////////////////////////////////////////////////////////////////
	SRamBank* physBanks = (SRamBank*)Kern::Alloc(num_boot_banks*sizeof(SRamBank));
	if (!physBanks)
		{
		Panic(ECreateNoMemory);
		}
	SRamBank* coalescedBank = physBanks;
	const SRamBank* const lastBank = aInfo.iBanks + num_boot_banks;
	TPhysAddr currentBase = aInfo.iBanks->iBase;
	TPhysAddr currentEnd = aInfo.iBanks->iBase + aInfo.iBanks->iSize;
	const SRamBank* nextBank = aInfo.iBanks + 1;
	for (; nextBank <= lastBank; ++nextBank)
		{
		// Create new bank if the next bank isn't contiguous or if 
		// it is the last bank
		if (nextBank == lastBank || nextBank->iBase != currentEnd)
			{
			coalescedBank->iBase = currentBase;
			coalescedBank->iSize = currentEnd - currentBase;
			// Mark all the SPageInfos for the pages in this bank as unused.
			// Needs to be done here to allow SPageInfo::SafeFromPhysAddr to work
			// which is used by InitSPageInfos()
			SPageInfo* pi = SPageInfo::FromPhysAddr(coalescedBank->iBase);
			SPageInfo* piBankEnd = pi + (coalescedBank->iSize >> KPageShift);
			for (; pi < piBankEnd; pi++)
				{
				pi->SetUnused();
				}
			++coalescedBank;
			__KTRACE_OPT(KMMU, Kern::Printf("Coalesced bank: %08x-%08x", currentBase, currentEnd));
			currentBase = nextBank->iBase;
			currentEnd = currentBase + nextBank->iSize;
			}
		else
			{
			currentEnd += nextBank->iSize;
			}
		}
	TUint num_coalesced_banks = coalescedBank - physBanks;
	__KTRACE_OPT(KMMU, Kern::Printf("#Coalesced banks: %d", num_coalesced_banks));

	///////////////////////////////////////////////////////////////////////////
	//	Initialise the SZone objects and mark all the SPageInfos with the index 
	//	of zone they are in.
	//////////////////////////////////////////////////////////////////////////
	// Assume everything is off so base port will get notification every time the 
	// a new zone is required during the rest of boot process.
	if (aZones != NULL)
		{		
		SZone* newZone = iZones;	// pointer to zone being created

		// Create and fill zoneAddrOrder with address ordered indices to aZones
		TUint8* zoneAddrOrder = (TUint8*)Kern::Alloc(iNumZones);
		if (!zoneAddrOrder)
			{
			Panic(ECreateNoMemory);
			}
		SortRamZones(aZones, zoneAddrOrder);

		// Now go through each SRamZone in address order initialising the SZone 
		// objects.
		TUint i = 0;
		TUint totalZonePages = 0;
		for (; i < iNumZones; i++)
			{
			const SRamZone& ramZone = *(aZones + zoneAddrOrder[i]);
			newZone->iPhysBase = ramZone.iBase;
			newZone->iPhysEnd = ramZone.iBase + ramZone.iSize - 1;
			newZone->iPhysPages = ramZone.iSize >> KPageShift;
			newZone->iAllocPages[EPageUnknown] = newZone->iPhysPages;
			newZone->iId = ramZone.iId;
			newZone->iPref = ramZone.iPref;
			newZone->iFlags = ramZone.iFlags;
			totalZonePages += InitSPageInfos(newZone);
			newZone++;
			}

		// iZones now points to all the SZone objects stored in address order
		Kern::Free(zoneAddrOrder);
		if (totalZonePages != iTotalRamPages)
			{// The zones don't cover all of the allocatable RAM.
			Panic(EZonesIncomplete);
			}
		}
	else
		{
		iNumZones = num_coalesced_banks;
		iZones = (SZone*)Kern::ReAlloc((TAny*)iZones, iNumZones*sizeof(SZone));
		if (iZones == NULL)
			{
			Panic(ECreateNoMemory);
			}
		// Create a zone for each coalesced boot bank
		SRamBank* bank = physBanks;
		SRamBank* bankEnd = physBanks + num_coalesced_banks;
		SZone* zone = iZones;
		for (; bank < bankEnd; bank++, zone++)
			{
			zone->iPhysBase = bank->iBase;
			zone->iPhysEnd = bank->iBase + bank->iSize - 1;
			zone->iPhysPages = bank->iSize >> KPageShift;
			zone->iAllocPages[EPageUnknown] = zone->iPhysPages;
			zone->iId = (TUint)bank; // doesn't matter what it is as long as it is unique
			InitSPageInfos(zone);
			}
		}
	// Delete the coalesced banks as no longer required
	Kern::Free(physBanks);

	//////////////////////////////////////////////////////////////////////////
	//	Create each zones' bit map allocator now as no temporary heap 
	// 	cells still allocated at this point.
	///////////////////////////////////////////////////////////////////////////
	const SZone* const endZone = iZones + iNumZones;
	SZone* zone = iZones;
	for (; zone < endZone; zone++)
		{// Create each BMA with all pages allocated as unknown.
		for (TUint i = 0; i < EPageTypes; i++)
			{
			// Only mark the all pages bma and fixed/unknown bma as allocated.
			TBool notAllocated = (i >= (TUint)EPageMovable);
			zone->iBma[i] = TBitMapAllocator::New(zone->iPhysPages, notAllocated);
			if (!zone->iBma[i])
				{
				Panic(ECreateNoMemory);
				}
			}
		}

	///////////////////////////////////////////////////////////////////////////
	// Unallocate each page in each bank so that it can be allocated when required.
	// Any page that exists outside a bank will remain allocated as EPageUnknown
	// and will therefore not be touched by the allocator.
	//////////////////////////////////////////////////////////////////////////
	// Temporarily fill preference list so SetPhysicalRamState can succeed
#ifdef _DEBUG
	// Block bma verificaitons as bma and alloc counts aren't consistent yet.
	iContiguousReserved = 1;
#endif
	const SZone* const lastZone = iZones + iNumZones;
	zone = iZones;
	for (; zone < lastZone; zone++)
		{
		iZonePrefList.Add(&zone->iPrefLink);
		}
	const SRamBank* const lastPhysBank = aInfo.iBanks + num_boot_banks;
	const SRamBank* bank = aInfo.iBanks;
	for (; bank < lastPhysBank; bank++)
		{// Free all the pages in this bank.
		SetPhysicalRamState(bank->iBase, bank->iSize, ETrue, EPageUnknown);
		}
#ifdef _DEBUG
	// Only now is it safe to enable bma verifications
	iContiguousReserved = 0;
#endif

	///////////////////////////////////////////////////////////////////////////
	//	Sort the zones by preference and create a preference ordered linked list
	///////////////////////////////////////////////////////////////////////////
	zone = iZones;
	for (; zone < lastZone; zone++)
		{// clear all the zones from the preference list as not in preference order
		zone->iPrefLink.Deque();
		}
	SZone** prefOrder = (SZone**)Kern::AllocZ(iNumZones * sizeof(SZone*));
	if (!prefOrder)
		{
		Panic(ECreateNoMemory);
		}
	zone = iZones;
	for(; zone < lastZone; zone++)
		{
		TInt lowerZones = 0;
		// Find how many zones that have a lower preference than this one
		const SZone* zone2 = iZones;
		for (; zone2 < lastZone; zone2++)
			{
			if (zone->iPref > zone2->iPref ||
				zone->iPref == zone2->iPref && zone->iFreePages > zone2->iFreePages)
				{
				lowerZones++;
				}
			}
		while (prefOrder[lowerZones] != 0)
			{// Zone(s) of this preference and size already exist so 
			 // place this one after it/them
			lowerZones++;
			}
		prefOrder[lowerZones] = zone;
		}
	// Fill preference ordered linked list
	SZone** const lastPref = prefOrder + iNumZones;
	SZone** prefZone = prefOrder;
	TUint prefRank = 0;
	for (; prefZone < lastPref; prefZone++, prefRank++)
		{
		SZone& zone = **prefZone;
		iZonePrefList.Add(&zone.iPrefLink);
		zone.iPrefRank = prefRank;
		}
	Kern::Free(prefOrder); // Remove temporary allocation

	///////////////////////////////////////////////////////////////////////////
	// 	Now mark any regions reserved by the base port as allocated and not 
	//	for use by the RAM allocator.
	///////////////////////////////////////////////////////////////////////////
	const SRamBank* pB = lastBank + 1;	// first reserved block specifier
	for (; pB->iSize; ++pB)
		{
		__KTRACE_OPT(KMMU, Kern::Printf("Reserve physical block %08x+%x", pB->iBase, pB->iSize));
		TInt r = SetPhysicalRamState(pB->iBase, pB->iSize, EFalse, EPageFixed);
		__KTRACE_OPT(KMMU, Kern::Printf("Reserve returns %d", r));
		if (r!=KErrNone)
			{
			Panic(ECreateInvalidReserveBank);
			}
#ifdef BTRACE_KERNEL_MEMORY
		BTrace8(BTrace::EKernelMemory, BTrace::EKernelMemoryDrvPhysAlloc, pB->iSize, pB->iBase);
		Epoc::DriverAllocdPhysRam += pB->iSize;
#endif
#ifndef __MEMMODEL_FLEXIBLE__ // Mmu::Init2Common() handles this in FMM.
		// Synchronise the SPageInfo with any blocks that were reserved by
		// marking any reserved regions as locked
		TPhysAddr physAddrEnd = pB->iBase + pB->iSize;
		TPhysAddr physAddr = pB->iBase;
		for(; physAddr < physAddrEnd; physAddr += KPageSize)
			{
			SPageInfo* pi = SPageInfo::FromPhysAddr(physAddr);
			pi->Lock();
			}
#endif
		}

	//////////////////////////////////////////////////////////////////////////
	// Now that we have have the RAM zone preference list and know how many
	// allocatable pages there are, set iZoneLeastMovDis to be the RAM zone 
	// that will be used when half of the RAM is in use. This a boot up 
	// optimisation to reduce the amount of moving and/or discarding fixed page 
	// allocations will have to make during boot.
	//////////////////////////////////////////////////////////////////////////
	TUint halfAllocatablePages = iTotalFreeRamPages >> 1;
	TUint pages = 0;
	SDblQueLink* link = &iZonePrefList.iA;
	do
		{
		link = link->iNext;
		__NK_ASSERT_DEBUG(link != &iZonePrefList.iA);
		SZone& zonePages = *_LOFF(link, SZone, iPrefLink);
		pages += zonePages.iFreePages;
		}
	while(pages < halfAllocatablePages);
	iZoneLeastMovDis = link;
	iZoneLeastMovDisRank = _LOFF(link, SZone, iPrefLink)->iPrefRank;

	// Reset general defrag links.
	iZoneGeneralPrefLink = NULL;
	iZoneGeneralTmpLink = NULL;

	__KTRACE_OPT(KMMU,DebugDump());
	}


void DRamAllocator::MarkPagesAllocated(TPhysAddr aAddr, TInt aCount, TZonePageType aType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::MarkPagesAllocated(%x+%x)",aAddr,aCount));

	M::RamAllocIsLocked();

	// Don't allow unknown pages to be allocated, saves extra 'if' when 
	// creating bmaType.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	__ASSERT_DEBUG(	!(TUint32(aAddr) & (KPageSize - 1)) &&
					(TUint32(aAddr) < TUint32(iPhysAddrTop)) && 
					(TUint32(aAddr) >= TUint32(iPhysAddrBase))&&
					(TUint32((aCount << KPageShift) -1 + aAddr) <= TUint32(iPhysAddrTop)),
					Panic(EDoMarkPagesAllocated1));

	iTotalFreeRamPages-=aCount;
	// Find the 1st zone the 1st set of allocations belong to
	TInt offset = 0;
	SZone* pZ = GetZoneAndOffset(aAddr,offset);
	if (pZ == NULL)
		{//aAddr not in RAM
		Panic(EDoMarkPagesAllocated1);
		}
	while(aCount)
		{
		TBitMapAllocator& bmaAll = *(pZ->iBma[KBmaAllPages]);
		TBitMapAllocator& bmaType = *(pZ->iBma[aType]);
		TInt count = Min(bmaAll.iSize - offset, aCount);
		bmaAll.Alloc(offset, count);
		bmaType.Alloc(offset, count);
		ZoneAllocPages(pZ, count, aType);
		aCount -= count;

		// If spanning zones then ensure the next zone is contiguous.
		__ASSERT_DEBUG(!aCount || ((pZ + 1)->iPhysBase != 0 && ((pZ + 1)->iPhysBase - 1) == pZ->iPhysEnd), Panic(EDoMarkPagesAllocated1));

		pZ++; 		// zones in physical address order so move to next one
		offset = 0;	// and reset offset to start of the zone
		}
	}


TInt DRamAllocator::MarkPageAllocated(TPhysAddr aAddr, TZonePageType aType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DRamAllocator::MarkPageAllocated %08x",aAddr));

	M::RamAllocIsLocked();

	// Don't allow unknown pages to be allocated, saves extra 'if' when 
	// creating bmaType.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	TInt n;
	SZone* z=GetZoneAndOffset(aAddr,n);
	if (!z)
		{
		return KErrArgument;
		}
	__KTRACE_OPT(KMMU2,Kern::Printf("Zone index %d page index %04x",z-iZones,n));
	TBitMapAllocator& bmaAll = *(z->iBma[KBmaAllPages]);
	TBitMapAllocator& bmaType = *(z->iBma[aType]);
	if (bmaAll.NotFree(n,1))
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Page already allocated"));
		return KErrAlreadyExists;			// page is already allocated
		}
	bmaAll.Alloc(n,1);
	if (bmaType.NotAllocated(n,1))
		bmaType.Alloc(n,1);
#ifdef _DEBUG
	else // Allow this page to already be reserved in bmaType as AllocContiguousRam() may have done this.
		__NK_ASSERT_DEBUG(aType == EPageFixed);
#endif
	--iTotalFreeRamPages;
	ZoneAllocPages(z, 1, aType);
	__KTRACE_OPT(KMMU,Kern::Printf("Total free RAM pages now = %d",iTotalFreeRamPages));

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocMarkAllocated, aType, aAddr);
#endif
	return KErrNone;
	}


TInt DRamAllocator::FreeRamPage(TPhysAddr aAddr, TZonePageType aType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FreeRamPage %08x",aAddr));

	M::RamAllocIsLocked();

#ifdef _DEBUG
#ifndef __MEMMODEL_FLEXIBLE__
	// Check lock counter of the page
	if (aAddr != KPhysAddrInvalid)
		{
		SPageInfo* pi =  SPageInfo::SafeFromPhysAddr(aAddr);
		if(pi && pi->LockCount())
			Panic(EFreeingLockedPage);
		}
#endif
	// Don't allow unknown pages to be freed, saves extra 'if' when 
	// creating bmaType.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);
#endif
	
	TInt n;
	SZone* z=GetZoneAndOffset(aAddr,n);
	if (!z)
		{
		return KErrArgument;
		}
	__KTRACE_OPT(KMMU2,Kern::Printf("Zone index %d page index %04x",z-iZones,n));
	TBitMapAllocator& bmaAll = *(z->iBma[KBmaAllPages]);
	TBitMapAllocator& bmaType = *(z->iBma[aType]);

	bmaType.Free(n);
	if (iContiguousReserved && aType != EPageFixed && z->iBma[EPageFixed]->NotFree(n, 1))
		{// This page has been reserved by AllocContiguous() so don't free it
		// but allocate it as fixed.
		ZoneFreePages(z, 1, aType);
		ZoneAllocPages(z, 1, EPageFixed);
		}
	else
		{
		bmaAll.Free(n);
		++iTotalFreeRamPages;
		ZoneFreePages(z, 1, aType);	
		}
#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocFreePage, aType, aAddr);
#endif
	return KErrNone;
	}


void DRamAllocator::FreeRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("FreeRamPages count=%08x",aNumPages));

	M::RamAllocIsLocked();

#if defined(_DEBUG) && !defined(__MEMMODEL_FLEXIBLE__)
	// Check lock counter for each page that is about to be freed.
	TInt pageNum = aNumPages;
	TPhysAddr* pageList = aPageList;
	while (pageNum--)
		{
		TPhysAddr pa = *pageList++;
		if (pa == KPhysAddrInvalid)
			continue;
		SPageInfo* pi =  SPageInfo::SafeFromPhysAddr(pa);
		if(pi && pi->LockCount())
			Panic(EFreeingLockedPage);
		}
#endif
	
	while(aNumPages--)
		{
		TPhysAddr first_pa = *aPageList++;
		if (first_pa == KPhysAddrInvalid)
			{
			continue;
			}
		TInt ix;
		SZone* z = GetZoneAndOffset(first_pa,ix);
		if (!z)
			{
			continue;
			}
		TBitMapAllocator& bmaAll = *(z->iBma[KBmaAllPages]);
		TInt zp_rem = bmaAll.iSize - ix;
		__KTRACE_OPT(KMMU,Kern::Printf("1st PA=%08x Zone %d index %04x",first_pa,z-iZones,ix));
		TInt n = 1;
		TPhysAddr pa = first_pa + KPageSize;
		while (--zp_rem && aNumPages && *aPageList==pa)
			{
			++n;
			--aNumPages;
			++aPageList;
			pa += KPageSize;
			}
		__KTRACE_OPT(KMMU2,Kern::Printf("%d consecutive pages, zp_rem=%x, %d remaining pages",n,zp_rem,aNumPages));
		TBitMapAllocator& bmaType = *(z->iBma[aType]);
		bmaType.Free(ix,n);

		if (iContiguousReserved && aType != EPageFixed)
			{// See if a page has been reserved by AllocContiguous() in this range.
			TUint pagesFreed = 0;
			TUint allocStart = ix;
			TUint freeOffset = ix;
			TUint endOffset = ix + n - 1;
			while (freeOffset <= endOffset)
				{
				TUint runLength =  NextAllocatedRun(z, allocStart, endOffset, EPageFixed);
				if (allocStart > freeOffset)
					{
					TUint freed = allocStart - freeOffset;
					bmaAll.Free(freeOffset, freed);
					pagesFreed += freed;
					}
				allocStart += runLength;
				freeOffset = allocStart;
				}
			iTotalFreeRamPages += pagesFreed;
			ZoneFreePages(z, n, aType);
			ZoneAllocPages(z, n - pagesFreed, EPageFixed);
			}
		else
			{
			bmaAll.Free(ix,n);
			iTotalFreeRamPages += n;
			ZoneFreePages(z, n, aType);
			}
#ifdef BTRACE_RAM_ALLOCATOR
		BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocFreePages, aType, n, first_pa);
#endif
		}
#ifdef BTRACE_RAM_ALLOCATOR
	BTrace0(BTrace::ERamAllocator, BTrace::ERamAllocFreePagesEnd);
#endif
	}


/**
	Attempt to clear upto the required amount of discardable or movable pages
	from the RAM zone.

	@param aZone			The RAM zone to clear.
	@param aRequiredPages	The maximum number of pages to clear.
*/
void DRamAllocator::ZoneClearPages(SZone& aZone, TUint aRequiredPages)
	{
	__KTRACE_OPT(KMMU, 
		Kern::Printf("ZoneClearPages: ID 0x%x, req 0x%x", aZone.iId, aRequiredPages));
	// Discard the required number of discardable pages.
	TUint offset = 0;
	for (; aRequiredPages; offset++)
		{
		TInt r = NextAllocatedPage(&aZone, offset, EPageDiscard);
		if (r != KErrNone)
			break;
		if (iContiguousReserved && aZone.iBma[EPageFixed]->NotFree(offset, 1))
			{
			offset++;
			continue;
			}
		TPhysAddr physAddr = (offset << KPageShift) + aZone.iPhysBase;
		TInt discarded = M::DiscardPage(physAddr, aZone.iId, M::EMoveDisMoveDirty);
		if (discarded == KErrNone)
			{// The page was successfully discarded.
			aRequiredPages--;
			}
		}
	// Move the required number of movable pages.
	for (offset = 0; aRequiredPages; offset++)
		{
		TInt r = NextAllocatedPage(&aZone, offset, EPageMovable);
		if (r != KErrNone)
			break;
		if (iContiguousReserved && aZone.iBma[EPageFixed]->NotFree(offset, 1))
			{
			offset++;
			continue;
			}
		TPhysAddr physAddr = (offset << KPageShift) + aZone.iPhysBase;
		TPhysAddr newAddr = KPhysAddrInvalid;
		if (M::MovePage(physAddr, newAddr, aZone.iId, 0) == KErrNone)
			{// The page was successfully moved.
#ifdef _DEBUG
			TInt newOffset = 0;
			SZone* newZone = GetZoneAndOffset(newAddr, newOffset);
			__NK_ASSERT_DEBUG(newZone != &aZone);
#endif
			aRequiredPages--;
			}
		}
	}

/** Attempt to allocate pages into a particular zone.  Pages will not
	always be contiguous.

	@param aPageList On return it will contain the addresses of any allocated pages
	@param aZone The zone to allocate from
	@param aNumPages The number of pages to allocate
	@param aType The type of pages to allocate
	@return The number of pages that were allocated
*/
TUint32 DRamAllocator::ZoneFindPages(TPhysAddr*& aPageList, SZone& aZone, TUint32 aNumPages, TZonePageType aType)
	{
	// Don't allow unknown pages to be allocated, saves extra 'if' when 
	// creating bmaType.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	TBitMapAllocator& bmaAll = *aZone.iBma[KBmaAllPages];
	TBitMapAllocator& bmaType = *(aZone.iBma[aType]);
	TPhysAddr zpb = aZone.iPhysBase;
	TInt got = bmaAll.AllocList(aNumPages, (TInt*)aPageList);
	if (got)
		{
		TPhysAddr* pE = aPageList + got;
		while(aPageList < pE)
			{
			TInt ix = *aPageList;
			*aPageList++ = zpb + (ix << KPageShift);
			__KTRACE_OPT(KMMU,Kern::Printf("Got page @%08x",zpb + (ix << KPageShift)));

			// Mark the page allocated on the page type bit map.
			bmaType.Alloc(ix, 1);
			}
		ZoneAllocPages(&aZone, got, aType);
#ifdef BTRACE_RAM_ALLOCATOR
		BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocRamPages, aType, got, *(pE-got));
#endif
		}
	return got;
	}

/**
Allocate discontiguous pages.  

Fixed pages are always allocated into the most preferable RAM zone that has free,
movable or discardable pages in it.  This is to avoid fixed pages being placed 
in the less preferred RAM zones.

Movable and discardable pages are allocated into the RAM zones currently in use.
An empty RAM zone will only be used (switched on) if there are not enough free 
pages in the in use RAM zones.  The pages will be allocated from the least 
preferable RAM to be in use after the allocation to the more preferred RAM zones.

If a valid zone is specified in aBlockedZoneId then that RAM zone will not be
allocated into.  Also, if aBlockedZoneId and aBlockRest is set then the allocation
will stop if aBlockZoneId

@param aPageList 	On success, will contain the address of each allocated page
@param aNumPages 	The number of the pages to allocate
@param aType 		The type of the pages to allocate
@param aBlockedZoneId	The ID of the RAM zone that shouldn't be allocated into.  
						The default value has no effect.
@param aBlockRest 	Set to ETrue to stop this allocation using any currently empty 
					RAM zones, EFalse to allow empty RAM zones to be used. Only
					effects movable and discardable allocations.

@return 0 on success, the number of extra pages required to fulfill the request on failure.
*/
TInt DRamAllocator::AllocRamPages(TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType, TUint aBlockedZoneId, TBool aBlockRest)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocRamPages 0x%x type%d",aNumPages, aType));

	M::RamAllocIsLocked();

	// Should never allocate unknown pages.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	TPhysAddr* pageListBase = aPageList;
	TUint32 numMissing = aNumPages;

	if ((TUint)aNumPages > iTotalFreeRamPages)
		{// Not enough free pages to fulfill this request so return the amount required
		return aNumPages - iTotalFreeRamPages;
		}

	if (aType == EPageFixed)
		{// Currently only a general defrag operation should set this and it won't
		// allocate fixed pages.
		__NK_ASSERT_DEBUG(!aBlockRest);

		// Search through each zone in preference order until all pages allocated or
		// have reached the end of the preference list
		SDblQueLink* link = iZonePrefList.First();
		while (numMissing && link != &iZonePrefList.iA)
			{
			SZone& zone = *_LOFF(link, SZone, iPrefLink);
			// Get the link to next zone before any potential reordering.
			// Which would occur if previous zone is same preference and has
			// more free space after this allocation.
			link = link->iNext;

			if (zone.iId == aBlockedZoneId || NoAllocOfPageType(zone, aType))
				{// The flags disallow aType pages or all pages.
				__KTRACE_OPT(KMMU2, Kern::Printf("ARP Flags 0x%08x", zone.iFlags));
				continue;
				}

			numMissing -= ZoneFindPages(aPageList, zone, numMissing, aType);
			__KTRACE_OPT(KMMU, Kern::Printf("zone.iId 0x%x", zone.iId));

			if (numMissing && 
				(zone.iAllocPages[EPageMovable] || zone.iAllocPages[EPageDiscard]))
				{// Not all the required pages where allocated and there are still some
				// movable and discardable pages in this RAM zone.
				ZoneClearPages(zone, numMissing);

				// Have discarded and moved everything required or possible so
				// now allocate into the pages just freed.
				numMissing -= ZoneFindPages(aPageList, zone, numMissing, aType);
				}
			}
		}
	else
		{
		// Determine if there are enough free pages in the RAM zones in use.
		TUint totalFreeInUse = 0;
		SDblQueLink* link = iZoneLeastMovDis;
		for(; link != &iZonePrefList.iA; link = link->iPrev)
			{
			SZone& zone = *_LOFF(link, SZone, iPrefLink);
			if (zone.iId == aBlockedZoneId || NoAllocOfPageType(zone, aType) ||
				(aBlockRest && (zone.iFlags & KRamZoneFlagGenDefragBlock)))
				{// The blocked RAM zone or flags disallow aType pages or all pages
				__KTRACE_OPT(KMMU2, Kern::Printf("ARP Flags 0x%08x", zone.iFlags));
				continue;
				}
			totalFreeInUse += zone.iFreePages;
			}

		if (aBlockRest && totalFreeInUse < (TUint)aNumPages)
			{// Allocating as part of a general defragmentation and
			// can't allocate without using a RAM zone less preferable than
			// the current least prefeable RAM zone with movable and/or 
			// discardable.
			__NK_ASSERT_DEBUG(numMissing);
			goto exit;
			}
		
		SDblQueLink* leastClearable = iZoneLeastMovDis;
		while (totalFreeInUse < (TUint)aNumPages)
			{// The amount of free pages in the RAM zones with movable 
			// and/or discardable isn't enough.
			leastClearable = leastClearable->iNext;
			if (leastClearable == &iZonePrefList.iA)
				{// There are no more RAM zones to allocate into.
				__NK_ASSERT_DEBUG(numMissing);
				goto exit;
				}
			SZone& zone = *_LOFF(leastClearable, SZone, iPrefLink);
			if (zone.iId == aBlockedZoneId || NoAllocOfPageType(zone, aType))
				{// The flags disallow aType pages or all pages
				__KTRACE_OPT(KMMU2, Kern::Printf("ARP Flags 0x%08x", zone.iFlags));
				continue;
				}
			totalFreeInUse += zone.iFreePages;
			}
		// Now that we know exactly how many RAM zones will be required do
		// the allocation. To reduce fixed allocations having to clear RAM 
		// zones, allocate from the least preferable RAM to be used
		// to the most preferable RAM zone.
		link = leastClearable;
		while (numMissing)
			{
			__NK_ASSERT_DEBUG(link != &iZonePrefList.iA);
			SZone& zone = *_LOFF(link, SZone, iPrefLink);
			// Update the link before any reordering so we don't miss a RAM zone.
			link = link->iPrev;

			if (zone.iId == aBlockedZoneId || NoAllocOfPageType(zone, aType) ||
				(aBlockRest && (zone.iFlags & KRamZoneFlagGenDefragBlock)))
				{// The blocked RAM zone or flags disallow aType pages or all pages
				__KTRACE_OPT(KMMU2, Kern::Printf("ARP Flags 0x%08x", zone.iFlags));
				continue;
				}

			numMissing -= ZoneFindPages(aPageList, zone, numMissing, aType);
			__KTRACE_OPT(KMMU, Kern::Printf("zone.iId 0x%x", zone.iId));
			}
		__NK_ASSERT_DEBUG(!numMissing);
		}

exit:
	// Update here so any call to FreeRamPages doesn't upset count
	aNumPages -= numMissing; //set to number of pages that are allocated
	iTotalFreeRamPages -= aNumPages;

	if (numMissing)
		{// Couldn't allocate all required pages so free those that were allocated
		FreeRamPages(pageListBase, aNumPages, aType);
		}
#ifdef BTRACE_RAM_ALLOCATOR
	else
		{
		BTrace0(BTrace::ERamAllocator, BTrace::ERamAllocRamPagesEnd);
		}
#endif
	return numMissing;
	}


/**
Attempt to allocate discontiguous pages from the specified RAM zone.

NOTE - This method only obeys the KRamZoneFlagNoAlloc and KRamZoneFlagClaiming 
flags and not the others.
But as currently only EFixed pages will be allocated using this method that is
the desired behaviour.

@param aZoneIdList 	An array of the IDs of the RAM zones to allocate from.
@param aZoneIdCount	The number of IDs in aZoneIdList.
@param aPageList 	On success, will contain the address of each allocated page.
@param aNumPages 	The number of the pages to allocate.
@param aType 		The type of the pages to allocate.

@return KErrNone on success, KErrNoMemory if allocation couldn't succeed or 
the RAM zone has the KRamZoneFlagNoAlloc flag set, KErrArgument if a zone of
aZoneIdList doesn't exist or aNumPages is greater than the total pages in the zone.
*/
TInt DRamAllocator::ZoneAllocRamPages(TUint* aZoneIdList, TUint aZoneIdCount, TPhysAddr* aPageList, TInt aNumPages, TZonePageType aType)
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(aType == EPageFixed);


	__KTRACE_OPT(KMMU,Kern::Printf("ZoneAllocRamPages 0x%x zones 0x%x",aNumPages, aZoneIdCount));

	TInt r = KErrNone;
	TUint* zoneIdPtr = aZoneIdList;
	TUint* zoneIdEnd = zoneIdPtr + aZoneIdCount;
	TUint numMissing = aNumPages;
	TUint physicalPages = 0;
	TPhysAddr* pageListBase = aPageList;

	// Always loop through all the RAM zones so that if an invalid ID is specified
	// it is always detected whether all the specified RAM zones were required 
	// for the allocation or not.
	for(; zoneIdPtr < zoneIdEnd; zoneIdPtr++)
		{
		SZone* zone = ZoneFromId(*zoneIdPtr);

		if (zone == NULL)
			{// Invalid zone ID.
			r = KErrArgument;
			break;
			}

		physicalPages += zone->iPhysPages;

		if (zone->iFlags & (KRamZoneFlagNoAlloc|KRamZoneFlagClaiming))
			{// If this RAM zone can't be allocated into then skip it.
			continue;
			}

		numMissing -= ZoneFindPages(aPageList, *zone, numMissing, aType);

		if (numMissing && aType == EPageFixed)
			{// Remove up to required number of pages from the RAM zone 
			// and reattempt the allocation.
			ZoneClearPages(*zone, numMissing);
			numMissing -= ZoneFindPages(aPageList, *zone, numMissing, aType);
			}
		}

	// Update iTotalFreeRamPages here so that if allocation doesn't succeed then
	// FreeRamPages() will keep it consistent.
	TUint numAllocated = aNumPages - numMissing;
	iTotalFreeRamPages -= numAllocated;

	if (r == KErrArgument || physicalPages < (TUint)aNumPages)
		{// Invalid zone ID or the number of pages requested is too large.
		// This should fail regardless of whether the allocation failed or not.
		FreeRamPages(pageListBase, numAllocated, aType);
		return KErrArgument;
		}

	if (numMissing)
		{// Couldn't allocate all required pages so free those that were allocated
		FreeRamPages(pageListBase, numAllocated, aType);
		return KErrNoMemory;
		}

	// Have allocated all the required pages.
#ifdef BTRACE_RAM_ALLOCATOR
	BTrace0(BTrace::ERamAllocator, BTrace::ERamAllocZoneRamPagesEnd);
#endif
	return KErrNone;
	}


/**
Will return zones one at a time in the following search patterns until a suitable
zone has been found or it is determined that there is no suitable zone:
	- preference order
	- address order
Before the first call for a new search sequence must set:
		iZoneTmpAddrIndex = -1;
		iZoneTmpPrefLink = iZonePrefList.First();

@param aZone On return this will be a pointer to the next zone to search.  
@param aState The current search state, i.e. which of the zone orderings to follow.
It will be updated if necessary by this function.
@param aType The type of page to be allocated.
@param aBlockedZoneId The ID of a RAM zone to not allocate into.
@param aBlockRest ETrue if allocation should fail as soon as a blocked zone is reached, 
EFalse otherwise. (Currently not used)
@return ETrue a sutiable zone is found, EFalse when the allocation is not possible.
*/
TBool DRamAllocator::NextAllocZone(SZone*& aZone, TZoneSearchState& aState, TZonePageType aType, TUint aBlockedZoneId, TBool aBlockRest)
	{
	TUint currentState = aState;
	TBool r = EFalse;

	for (; currentState < EZoneSearchEnd; currentState++)
		{
		if (currentState == EZoneSearchAddr)
			{
			iZoneTmpAddrIndex++;
			for (; iZoneTmpAddrIndex < (TInt)iNumZones; iZoneTmpAddrIndex++)
				{
				aZone = iZones + iZoneTmpAddrIndex;
				if (aBlockedZoneId != aZone->iId && !NoAllocOfPageType(*aZone, aType))
					{
					r = ETrue;
					goto exit;
					}				
				}
			}
		else
			{
			while(iZoneTmpPrefLink != &iZonePrefList.iA)
				{
				aZone = _LOFF(iZoneTmpPrefLink, SZone, iPrefLink);
				iZoneTmpPrefLink = iZoneTmpPrefLink->iNext; // Update before any re-ordering
				if (aBlockedZoneId != aZone->iId && !NoAllocOfPageType(*aZone, aType))
					{
					r = ETrue;
					goto exit;
					}
				}
			}
		}
exit:
	__NK_ASSERT_DEBUG((r && currentState < EZoneSearchEnd) || (!r && currentState == EZoneSearchEnd));

	aState = (TZoneSearchState)currentState;
	return r;
	}


#if !defined(__MEMMODEL_MULTIPLE__) && !defined(__MEMMODEL_MOVING__)
TUint DRamAllocator::BlockContiguousRegion(TPhysAddr aAddrBase, TUint aNumPages)
	{
	// Shouldn't be asked to block zero pages, addrEndPage would be wrong if we did.
	__NK_ASSERT_DEBUG(aNumPages);
	TPhysAddr addr = aAddrBase;
	TPhysAddr addrEndPage = aAddrBase + ((aNumPages - 1) << KPageShift);
	TInt tmpOffset;
	SZone* endZone = GetZoneAndOffset(addrEndPage, tmpOffset);
	SZone* tmpZone;
	TUint totalUnreserved = aNumPages;
	do
		{
		tmpZone = GetZoneAndOffset(addr, tmpOffset);
		__NK_ASSERT_DEBUG(tmpZone != NULL);
		TUint runLength = 	(addrEndPage < tmpZone->iPhysEnd)? 
							((addrEndPage - addr) >> KPageShift) + 1: 
							tmpZone->iPhysPages - tmpOffset;
		TUint reserved = tmpZone->iBma[KBmaAllPages]->SelectiveAlloc(tmpOffset, runLength);
		if (reserved)
			{
#ifdef _DEBUG
			TUint runEnd = tmpOffset + runLength;
			TUint free = 0;
			for (TUint i = tmpOffset; i < runEnd; i++)
				if (tmpZone->iBma[EPageMovable]->NotAllocated(i,1) && tmpZone->iBma[EPageDiscard]->NotAllocated(i,1))
					free++;
			__NK_ASSERT_DEBUG(free == reserved);
#endif
			ZoneAllocPages(tmpZone, reserved, EPageFixed);
			iTotalFreeRamPages -= reserved;
			totalUnreserved -= reserved;
			}
		tmpZone->iBma[EPageFixed]->Alloc(tmpOffset, runLength);
		addr = tmpZone->iPhysEnd + 1;
		}
	while (tmpZone != endZone);
	return totalUnreserved;
	}


FORCE_INLINE void DRamAllocator::UnblockSetAllocRuns(	TUint& aOffset1, TUint& aOffset2, 
														TUint aRunLength1, TUint aRunLength2, 
														TUint& aAllocLength, TUint& aAllocStart)
	{
	aAllocStart = aOffset1;
	aAllocLength = aRunLength1;
	aOffset1 += aAllocLength;
	if  (aOffset1 == aOffset2)
		{
		aAllocLength += aRunLength2;
		aOffset2 += aRunLength2;
		aOffset1 = aOffset2;
		}
	} 	


void DRamAllocator::UnblockContiguousRegion(TPhysAddr aAddrBase, TUint aNumPages)
	{
	// Shouldn't be asked to unblock zero pages, addrEndPage would be wrong if we did.
	__NK_ASSERT_DEBUG(aNumPages);
	TPhysAddr addr = aAddrBase;
	TPhysAddr addrEndPage = aAddrBase + ((aNumPages - 1) << KPageShift);
	TInt tmpOffset;
	SZone* endZone = GetZoneAndOffset(addrEndPage, tmpOffset);
	SZone* tmpZone;
	do
		{
		tmpZone = GetZoneAndOffset(addr, tmpOffset);
		__NK_ASSERT_DEBUG(tmpZone != NULL);
		TUint runLength = 	(addrEndPage < tmpZone->iPhysEnd)? 
							((addrEndPage - addr) >> KPageShift) + 1: 
							tmpZone->iPhysPages - tmpOffset;
		TUint unreserved = 0;
		TUint runEnd = tmpOffset + runLength - 1;
		TUint freeOffset = tmpOffset;
		TUint discardOffset = freeOffset;
		TUint movableOffset = freeOffset;
		__KTRACE_OPT(KMMU2, Kern::Printf("freeOff %d, runEnd %d", freeOffset, runEnd));
		while (freeOffset <= runEnd)
			{
			TUint discardRun;
			TUint movableRun;
			discardRun = NextAllocatedRun(tmpZone, discardOffset, runEnd, EPageDiscard);
			movableRun = NextAllocatedRun(tmpZone, movableOffset, runEnd, EPageMovable);
			TUint allocLength;
			TUint allocStart;
			__KTRACE_OPT(KMMU2, Kern::Printf("disOff %d len %d movOff %d len %d", discardOffset, discardRun, movableOffset, movableRun));
			if (discardOffset < movableOffset)
				UnblockSetAllocRuns(discardOffset, movableOffset, discardRun, movableRun, allocLength, allocStart);
			else
				UnblockSetAllocRuns(movableOffset, discardOffset, movableRun, discardRun, allocLength, allocStart);

			if (allocStart > freeOffset)
				{
				unreserved += allocStart - freeOffset;
				tmpZone->iBma[KBmaAllPages]->Free(freeOffset, allocStart - freeOffset);
				__NK_ASSERT_DEBUG(	!tmpZone->iBma[EPageMovable]->NotFree(freeOffset, allocStart - freeOffset) && 
									!tmpZone->iBma[EPageDiscard]->NotFree(freeOffset, allocStart - freeOffset));
				}
			__KTRACE_OPT(KMMU2, Kern::Printf("disOff %d len %d movOff %d len %d start %d len %d", discardOffset, discardRun, movableOffset, movableRun, allocStart, allocLength));
			freeOffset = allocStart + allocLength;
			__KTRACE_OPT(KMMU2, Kern::Printf("freeOff %d", freeOffset));
			}
		tmpZone->iBma[EPageFixed]->Free(tmpOffset, runLength);
		ZoneFreePages(tmpZone, unreserved, EPageFixed);
		iTotalFreeRamPages += unreserved;
		addr = tmpZone->iPhysEnd + 1;
		}
	while (tmpZone != endZone);
	}


TUint DRamAllocator::CountPagesInRun(TPhysAddr aAddrBase, TPhysAddr aAddrEndPage, TZonePageType aType)
	{
	__NK_ASSERT_DEBUG(aAddrBase <= aAddrEndPage);
	TUint totalAllocated = 0;
	TPhysAddr addr = aAddrBase;
	TUint tmpOffset;
	SZone* endZone = GetZoneAndOffset(aAddrEndPage, (TInt&)tmpOffset);
	SZone* tmpZone;
	do
		{
		tmpZone = GetZoneAndOffset(addr, (TInt&)tmpOffset);
		__NK_ASSERT_DEBUG(tmpZone != NULL);
		TUint runLength = 	(aAddrEndPage < tmpZone->iPhysEnd)? 
							((aAddrEndPage - addr) >> KPageShift) + 1: 
							tmpZone->iPhysPages - tmpOffset;
		TUint runEnd = tmpOffset + runLength - 1;
		while (tmpOffset <= runEnd)
			{
			TUint run = NextAllocatedRun(tmpZone, tmpOffset, runEnd, aType);
			totalAllocated += run;
			tmpOffset += run;
			}
		addr = tmpZone->iPhysEnd + 1;
		}
	while (tmpZone != endZone);
	return totalAllocated;
	}


TInt DRamAllocator::ClearContiguousRegion(TPhysAddr aAddrBase, TPhysAddr aZoneBase, TUint aNumPages, TInt& aOffset, TUint aUnreservedPages)
	{
	TPhysAddr addr = aAddrBase;
	TPhysAddr addrEndPage = aAddrBase + ((aNumPages -1 )<< KPageShift);
	TInt contigOffset = 0;
	SZone* contigZone = GetZoneAndOffset(addr, contigOffset);
	TUint unreservedPages = aUnreservedPages;
	for (; addr <= addrEndPage; addr += KPageSize, contigOffset++)
		{
		if (contigZone->iPhysEnd < addr)
			{
			contigZone = GetZoneAndOffset(addr, contigOffset);
			__NK_ASSERT_DEBUG(contigZone != NULL);
			}

		__NK_ASSERT_DEBUG(contigZone != NULL);
		__NK_ASSERT_DEBUG(contigZone->iBma[EPageFixed]->NotFree(contigOffset, 1));
		__NK_ASSERT_DEBUG(SPageInfo::SafeFromPhysAddr(addr) != NULL);

		if (unreservedPages > iTotalFreeRamPages)
			{// May need to discard some pages so there is free space for the 
			// pages in the contiguous run to be moved to.
			TUint requiredPages = unreservedPages - iTotalFreeRamPages;
			if (requiredPages)
				{// Ask the pager to get free some pages.
				M::GetFreePages(requiredPages);

				// The ram alloc lock may have been flashed so ensure that we still have
				// enough free ram to complete the allocation.
				TUint remainingPages = ((addrEndPage - addr) >> KPageShift) + 1;
				unreservedPages = remainingPages - CountPagesInRun(addr, addrEndPage, EPageFixed);
				if (unreservedPages > iTotalFreeRamPages + M::NumberOfFreeDpPages())
					{// Not enough free space and not enough freeable pages.
					return KErrNoMemory;
					}
				}
			}

		TInt r = M::MoveAndAllocPage(addr, EPageFixed);
		if (r != KErrNone)
			{// This page couldn't be moved or discarded so 
			// restart the search the page after this one.
			__KTRACE_OPT(KMMU2, Kern::Printf("ContigMov fail contigOffset 0x%x r %d", contigOffset, r));
			aOffset = (addr < aZoneBase)? 0 : contigOffset + 1;
			return r;
			}
		__NK_ASSERT_DEBUG(contigZone->iBma[EPageFixed]->NotFree(contigOffset, 1));
		__NK_ASSERT_DEBUG(contigZone->iBma[KBmaAllPages]->NotFree(contigOffset, 1));
		__NK_ASSERT_DEBUG(contigZone->iBma[EPageDiscard]->NotAllocated(contigOffset, 1));
		__NK_ASSERT_DEBUG(contigZone->iBma[EPageMovable]->NotAllocated(contigOffset, 1));
		}

	// Successfully cleared the contiguous run
	return KErrNone;
	}


/**
Search through the zones for the requested contiguous RAM, first in preference 
order then, if that fails, in address order.

No support for non-fixed pages as this will discard and move pages if required.

@param aNumPages The number of contiguous pages to find
@param aPhysAddr Will contain the base address of any contiguous run if found
@param aAlign Alignment specified as the alignment shift
in preference ordering.  EFalse otherwise.

@return KErrNone on success, KErrNoMemory otherwise
*/	
TInt DRamAllocator::AllocContiguousRam(TUint aNumPages, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam size %08x align %d",aNumPages,aAlign));

	M::RamAllocIsLocked();

	if ((TUint)aNumPages > iTotalFreeRamPages + M::NumberOfFreeDpPages())
		{// Not enough free space and not enough freeable pages.
		return KErrNoMemory;
		}
	if (aNumPages > iTotalFreeRamPages)
		{// Need to discard some pages so there is free space for the pages in 
		// the contiguous run to be moved to.
		TUint requiredPages = aNumPages - iTotalFreeRamPages;
		if (!M::GetFreePages(requiredPages))
			return KErrNoMemory;
		}

	TInt alignWrtPage = Max(aAlign - KPageShift, 0);
	TUint32 alignmask = (1u << alignWrtPage) - 1;

	// Attempt to find enough pages searching in preference order first then
	// in address order
	TZoneSearchState searchState = EZoneSearchPref;
	SZone* zone;
	SZone* prevZone = NULL;
	TInt carryAll = 0;		// Carry for all pages bma, clear to start new run.
	TInt carryImmov = 0;	// Carry for immovable pages bma, clear to start new run.
	TInt base = 0;
	TInt offset = 0;
	iZoneTmpAddrIndex = -1;
	iZoneTmpPrefLink = iZonePrefList.First();
	while (NextAllocZone(zone, searchState, EPageFixed, KRamZoneInvalidId, EFalse))
		{
		// Be sure to start from scratch if zone not contiguous with previous zone
		if (prevZone && (zone->iPhysBase == 0 || (zone->iPhysBase - 1) != prevZone->iPhysEnd))
			{
			carryAll = 0;
			carryImmov = 0;
			}
		prevZone = zone;
		TBitMapAllocator& bmaAll = *(zone->iBma[KBmaAllPages]);
		base = TInt(zone->iPhysBase >> KPageShift);
		TInt runLength;
		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: base=%08x carryAll=%08x offset=%08x", base, carryAll, offset));
		offset = bmaAll.AllocAligned(aNumPages, alignWrtPage, base, EFalse, carryAll, runLength);
		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: offset=%08x", offset));

		if (offset >= 0)
			{
			// Have found enough contiguous pages so return address of physical page
			// at the start of the region
			aPhysAddr = TPhysAddr((base + offset - carryAll + alignmask) & ~alignmask) << KPageShift;
			MarkPagesAllocated(aPhysAddr, aNumPages, EPageFixed);

			__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %08x",aPhysAddr));
#ifdef BTRACE_RAM_ALLOCATOR
			BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocContiguousRam, EPageFixed, aNumPages, aPhysAddr);
#endif
			return KErrNone;
			}
		// No run found when looking in just the free pages so see if this
		// RAM zone could be used if pages where moved or discarded.
		TBitMapAllocator& bmaImmov = *(zone->iBma[EPageFixed]);
		offset = 0;	// Clear so searches whole of fixed BMA on the first pass.
		do
			{
			__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: base=%08x carryImmov=%08x offset=%08x", base, carryImmov, offset));
			offset = bmaImmov.AllocAligned(aNumPages, alignWrtPage, base, EFalse, carryImmov, runLength, offset);
			__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: offset=%08x", offset));
			if (offset >= 0)
				{// Have found a run in immovable page bma so attempt to clear
				// it for the allocation.
				TPhysAddr addrBase = TPhysAddr((base + offset - carryImmov + alignmask) & ~alignmask) << KPageShift;
				__KTRACE_OPT(KMMU2, Kern::Printf(">AllocContig fix run 0x%08x - 0x%08x 0x%x", addrBase, addrBase + (aNumPages << KPageShift), TheCurrentThread));
				
				// Block the contiguous region from being allocated.
				iContiguousReserved++;
				TUint unreservedPages = BlockContiguousRegion(addrBase, aNumPages);
				TInt clearRet = ClearContiguousRegion(addrBase, zone->iPhysBase, aNumPages, offset, unreservedPages);
				if (clearRet == KErrNone)
					{// Cleared all the required pages.
					// Return address of physical page at the start of the region.
					iContiguousReserved--;
					aPhysAddr = addrBase;
					__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %08x",aPhysAddr));
#ifdef BTRACE_RAM_ALLOCATOR
					BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocContiguousRam, EPageFixed, aNumPages, aPhysAddr);
#endif
					__KTRACE_OPT(KMMU2, Kern::Printf("<AllocContig suc run 0x%08x - 0x%08x 0x%x", addrBase, addrBase + (aNumPages << KPageShift), TheCurrentThread));
					return KErrNone;
					}
				else
					{
					// Unblock the contiguous region.
					UnblockContiguousRegion(addrBase, aNumPages);
					iContiguousReserved--;
					__KTRACE_OPT(KMMU2, Kern::Printf("ContigMov fail offset 0x%x carryImmov %x", 
														offset, carryImmov));
					// Can't rely on RAM zone preference ordering being
					// the same so clear carrys and restart search from
					// within the current RAM zone or skip onto the next 
					// one if at the end of this one.
					carryImmov = 0;
					carryAll = 0;
					__KTRACE_OPT(KMMU2, Kern::Printf("<AllocContigfail run 0x%08x - 0x%08x 0x%x", addrBase, addrBase + (aNumPages << KPageShift), TheCurrentThread));
					if (clearRet == KErrNoMemory)
						{// There are no longer enough free or discardable pages to 
						// be able to fulfill this allocation.
						return KErrNoMemory;
						}
					}
				}
			}
		// Keep searching immovable page bma of the current RAM zone until 
		// gone past end of RAM zone or no run can be found.
		while (offset >= 0 && (TUint)offset < zone->iPhysPages);
		}
	return KErrNoMemory;
	}

#else

/**
Search through the zones for the requested contiguous RAM, first in preference 
order then, if that fails, in address order.

No support for non-fixed pages as this will discard and move pages if required.

@param aNumPages The number of contiguous pages to find
@param aPhysAddr Will contain the base address of any contiguous run if found
@param aAlign Alignment specified as the alignment shift

@return KErrNone on success, KErrNoMemory otherwise
*/	
TInt DRamAllocator::AllocContiguousRam(TUint aNumPages, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam size %08x align %d",aNumPages,aAlign));

	M::RamAllocIsLocked();

	TInt alignWrtPage = Max(aAlign - KPageShift, 0);
	TUint32 alignmask = (1u << alignWrtPage) - 1;

	// Attempt to find enough pages searching in preference order first then
	// in address order
	TZoneSearchState searchState = EZoneSearchPref;
	SZone* zone;
	SZone* prevZone = NULL;
	TInt carryAll = 0;		// Carry for all pages bma, clear to start new run.
	TInt carryImmov = 0;	// Carry for immovable pages bma, clear to start new run.
	TInt base = 0;
	TInt offset = 0;
	iZoneTmpAddrIndex = -1;
	iZoneTmpPrefLink = iZonePrefList.First();
	while (NextAllocZone(zone, searchState, EPageFixed, KRamZoneInvalidId, EFalse))
		{
		// Be sure to start from scratch if zone not contiguous with previous zone
		if (prevZone && (zone->iPhysBase == 0 || (zone->iPhysBase - 1) != prevZone->iPhysEnd))
			{
			carryAll = 0;
			carryImmov = 0;
			}
		prevZone = zone;
		TBitMapAllocator& bmaAll = *(zone->iBma[KBmaAllPages]);
		base = TInt(zone->iPhysBase >> KPageShift);
		TInt runLength;
		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: base=%08x carryAll=%08x offset=%08x", base, carryAll, offset));
		offset = bmaAll.AllocAligned(aNumPages, alignWrtPage, base, EFalse, carryAll, runLength);
		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: offset=%08x", offset));

		if (offset >= 0)
			{// Have found enough contiguous pages so return address of physical page
			 // at the start of the region
			aPhysAddr = TPhysAddr((base + offset - carryAll + alignmask) & ~alignmask) << KPageShift;
			MarkPagesAllocated(aPhysAddr, aNumPages, EPageFixed);

			__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %08x",aPhysAddr));
#ifdef BTRACE_RAM_ALLOCATOR
			BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocContiguousRam, EPageFixed, aNumPages, aPhysAddr);
#endif
			return KErrNone;
			}
		else
			{// No run found when looking in just the free pages so see if this
			// RAM zone could be used if pages where moved or discarded.
			if (aNumPages > KMaxFreeableContiguousPages)
				{// Can't move or discard any pages so move on to next RAM zone 
				// taking any run at the end of this RAM zone into account.
				carryImmov = 0;
				continue;
				}
			TBitMapAllocator& bmaImmov = *(zone->iBma[EPageFixed]);
			offset = 0;	// Clear so searches whole of fixed BMA on the first pass.
			do
				{
				__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: base=%08x carryImmov=%08x offset=%08x", base, carryImmov, offset));
				offset = bmaImmov.AllocAligned(aNumPages, alignWrtPage, base, EFalse, carryImmov, runLength, offset);
				__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: offset=%08x", offset));
				if (offset >= 0)
					{// Have found a run in immovable page bma so attempt to clear
					// it for the allocation.
					TPhysAddr addrBase = TPhysAddr((base + offset - carryImmov + alignmask) & ~alignmask) << KPageShift;
					TPhysAddr addrEnd = addrBase + (aNumPages << KPageShift);
					
					// Block the RAM zones containing the contiguous region
					// from being allocated into when pages are moved or replaced.
					TPhysAddr addr = addrBase;
					TInt tmpOffset;
					SZone* tmpZone = GetZoneAndOffset(addr, tmpOffset);
					while (addr < addrEnd-1)
						{
						tmpZone->iFlags |= KRamZoneFlagTmpBlockAlloc;
						addr = tmpZone->iPhysEnd;
						tmpZone++;
						}

					addr = addrBase;
					TInt contigOffset = 0;
					SZone* contigZone = GetZoneAndOffset(addr, contigOffset);
					for (; addr != addrEnd; addr += KPageSize, contigOffset++)
						{
						if (contigZone->iPhysEnd < addr)
							{
							contigZone = GetZoneAndOffset(addr, contigOffset);
							__NK_ASSERT_DEBUG(contigZone != NULL);
							}
						// This page shouldn't be allocated as fixed, only movable or discardable.
						__NK_ASSERT_DEBUG(contigZone != NULL);
						__NK_ASSERT_DEBUG(contigZone->iBma[EPageFixed]->NotAllocated(contigOffset, 1));
						__NK_ASSERT_DEBUG(SPageInfo::SafeFromPhysAddr(addr) != NULL);

						TPhysAddr newAddr;
						TInt moveRet = M::MovePage(addr, newAddr, contigZone->iId, 0);
						if (moveRet != KErrNone && moveRet != KErrNotFound)
							{// This page couldn't be moved or discarded so 
							// restart the search the page after this one.
							__KTRACE_OPT(KMMU2, 
										Kern::Printf("ContigMov fail offset %x moveRet %d addr %x carryImmov %x", 
										offset, moveRet, addr, carryImmov));
							// Can't rely on RAM zone preference ordering being
							// the same so clear carrys and restart search from
							// within the current RAM zone or skip onto the next 
							// one if at the end of this one.
							carryImmov = 0;
							carryAll = 0;
							offset = (addr < zone->iPhysBase)? 0 : contigOffset + 1;
							__KTRACE_OPT(KMMU2, Kern::Printf("ContigMov fail offset %x", offset));
							break;
							}
						}
					// Unblock the RAM zones containing the contiguous region. 
					TPhysAddr flagAddr = addrBase;
					tmpZone = GetZoneAndOffset(flagAddr, tmpOffset);
					while (flagAddr < addrEnd-1)
						{
						tmpZone->iFlags &= ~KRamZoneFlagTmpBlockAlloc;
						flagAddr = tmpZone->iPhysEnd;
						tmpZone++;
						}

					if (addr == addrEnd)
						{// Cleared all the required pages so allocate them.
						// Return address of physical page at the start of the region.
						aPhysAddr = addrBase;
						MarkPagesAllocated(aPhysAddr, aNumPages, EPageFixed);

						__KTRACE_OPT(KMMU,Kern::Printf("AllocContiguousRam returns %08x",aPhysAddr));
#ifdef BTRACE_RAM_ALLOCATOR
						BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocContiguousRam, EPageFixed, aNumPages, aPhysAddr);
#endif
						return KErrNone;
						}
					}
				}
			// Keep searching immovable page bma of the current RAM zone until 
			// gone past end of RAM zone or no run can be found.
			while (offset >= 0 && (TUint)offset < zone->iPhysPages);
			}
		}
	return KErrNoMemory;
	}
#endif // !defined(__MEMODEL_MULTIPLE__) || !defined(__MEMODEL_MOVING__)


/**
Attempt to allocate the contiguous RAM from the specified zone.

NOTE - This method only obeys the KRamZoneFlagNoAlloc and KRamZoneFlagClaiming 
flags and not the others.
But as currently only EFixed pages will be allocated using this method that is
the desired behaviour.

@param aZoneIdList 	An array of the IDs of the RAM zones to allocate from.
@param aZoneIdCount	The number of the IDs listed by aZoneIdList.
@param aSize 		The number of contiguous bytes to find
@param aPhysAddr 	Will contain the base address of the contiguous run if found
@param aAlign 		Alignment specified as the alignment shift

@return KErrNone on success, KErrNoMemory if allocation couldn't succeed or 
the RAM zone has the KRamZoneFlagNoAlloc flag set.  KErrArgument if a zone of
aZoneIdList exists or if aSize is larger than the size of the zone.
*/	
TInt DRamAllocator::ZoneAllocContiguousRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ZoneAllocContiguousRam zones 0x%x size 0x%08x align %d",aZoneIdCount, aSize, aAlign));

	M::RamAllocIsLocked();


	TUint numPages = (aSize + KPageSize - 1) >> KPageShift;
	TInt carry = 0; // must be zero as this is always the start of a new run
	TInt alignWrtPage = Max(aAlign - KPageShift, 0);
	TUint32 alignmask = (1u << alignWrtPage) - 1;
	TInt offset = -1;
	TInt base = 0;
	
	TUint physPages = 0;
	TUint* zoneIdPtr = aZoneIdList;
	TUint* zoneIdEnd = aZoneIdList + aZoneIdCount;
	SZone* prevZone = NULL;
	for (; zoneIdPtr < zoneIdEnd; zoneIdPtr++)
		{
		SZone* zone = ZoneFromId(*zoneIdPtr);
		if (zone == NULL)
			{// Couldn't find zone of this ID or it isn't large enough
			return KErrArgument;
			}
		physPages += zone->iPhysPages;

		if (offset >= 0 ||
			(zone->iFlags & (KRamZoneFlagNoAlloc|KRamZoneFlagClaiming)))
			{// Keep searching through the RAM zones if the allocation
			// has succeeded, to ensure the ID list is always fully verified or
			// if this zone is currently blocked for further allocations.
			continue;
			}

		// Be sure to start from scratch if zone not contiguous with previous zone
		if (prevZone && (zone->iPhysBase == 0 || (zone->iPhysBase - 1) != prevZone->iPhysEnd))
			{
			carry = 0;
			}
		prevZone = zone;

		TInt len;
		TBitMapAllocator& bmaAll = *(zone->iBma[KBmaAllPages]);
		base = TInt(zone->iPhysBase >> KPageShift);

		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: aBase=%08x aCarry=%08x", base, carry));
		offset = bmaAll.AllocAligned(numPages, alignWrtPage, base, EFalse, carry, len);
		__KTRACE_OPT(KMMU,Kern::Printf("AllocAligned: offset=%08x", offset));
		}

	if (physPages < numPages)
		{// The allocation requested is too large for the specified RAM zones.
		return KErrArgument;
		}

	if (offset < 0)
		{// The allocation failed.
		return KErrNoMemory;
		}

	// Have found enough contiguous pages so mark the pages allocated and 
	// return address of physical page at the start of the region.
	aPhysAddr = TPhysAddr((base + offset - carry + alignmask) & ~alignmask) << KPageShift;
	MarkPagesAllocated(aPhysAddr, numPages, EPageFixed);

	__KTRACE_OPT(KMMU,Kern::Printf("ZoneAllocContiguousRam returns %08x",aPhysAddr));
#ifdef BTRACE_RAM_ALLOCATOR
	BTrace12(BTrace::ERamAllocator, BTrace::ERamAllocZoneContiguousRam, EPageFixed, numPages, aPhysAddr);
#endif
	return KErrNone;
	}


/**
Attempt to set the specified contiguous block of RAM pages to be either 
allocated or free.

@param aBase The base address of the RAM to update.
@param aSize The number of contiguous bytes of RAM to update.
@param aState Set to ETrue to free the RAM, EFalse to allocate the RAM.
@param aType The type of the pages being updated.

@return KErrNone on success, KErrArgument if aBase is an invalid address, 
KErrGeneral if a page being marked free is already free,
KErrInUse if the page being marked allocated is already allocated.
*/
TInt DRamAllocator::SetPhysicalRamState(TPhysAddr aBase, TInt aSize, TBool aState, TZonePageType aType)
	{
	M::RamAllocIsLocked();

	__KTRACE_OPT(KMMU,Kern::Printf("SetPhysicalRamState(%08x,%x,%d)",aBase,aSize,aState?1:0));
	TUint32 pageMask = KPageSize-1;
	aSize += (aBase & pageMask);
	aBase &= ~pageMask;
	TInt npages = (aSize + pageMask) >> KPageShift;
	__KTRACE_OPT(KMMU,Kern::Printf("Rounded base %08x npages=%x",aBase,npages));
	TInt baseOffset;
	SZone* baseZone = GetZoneAndOffset(aBase, baseOffset);
	if (!baseZone || (TUint32)aSize > (iPhysAddrTop - aBase + 1))
		{
		return KErrArgument;
		}
	SZone* zone = baseZone;
	SZone* zoneEnd = iZones + iNumZones;
	TPhysAddr base = aBase;
	TInt pagesLeft = npages;
	TInt offset = baseOffset;
	TInt pageCount = -1;
	__KTRACE_OPT(KMMU2,Kern::Printf("Zone %x page index %x z=%08x zE=%08x n=%x base=%08x",zone->iId, offset, zone, zoneEnd, pagesLeft, base));
	for (; 	pagesLeft && zone < zoneEnd; ++zone)
		{
		if (zone->iPhysBase + (offset << KPageShift) != base)
			{// Zone not contiguous with current run of page, so have been 
			// asked to set the state of non-existent pages.
			return KErrArgument;
			}

		TBitMapAllocator& bmaAll = *(zone->iBma[KBmaAllPages]);
		TInt zp_rem = bmaAll.iSize - offset;
		pageCount = Min(pagesLeft, zp_rem);
		__KTRACE_OPT(KMMU2,Kern::Printf("Zone %x pages %x+%x base %08x", zone->iId, offset, pageCount, base));
		if(aState)
			{
			if(bmaAll.NotAllocated(offset, pageCount))
				{
				return KErrGeneral;
				}
			}
		else
			{
			if(bmaAll.NotFree(offset, pageCount))
				{
				return KErrInUse;
				}
			}
		pagesLeft -= pageCount;
		offset = 0;
		base += (TPhysAddr(pageCount) << KPageShift);
		}
	if (pagesLeft)
		{
		return KErrArgument;	// not all of the specified range exists
		}

	iTotalFreeRamPages += (aState ? npages : -npages);
	zone = baseZone;
	offset = baseOffset;
	for (pagesLeft = npages; pagesLeft; pagesLeft -= pageCount)
		{
		TBitMapAllocator& bmaAll = *(zone->iBma[KBmaAllPages]);
		// Unknown and fixed pages share a bit map.
		TBitMapAllocator& bmaType = *(zone->iBma[(aType != EPageUnknown)? aType : EPageFixed]);
		TInt zp_rem = bmaAll.iSize - offset;
		pageCount = Min(pagesLeft, zp_rem);
		if (aState)
			{
			bmaAll.Free(offset, pageCount);
			bmaType.Free(offset, pageCount);
			ZoneFreePages(zone, pageCount, aType);
			}
		else
			{
			bmaAll.Alloc(offset, pageCount);
			bmaType.Alloc(offset, pageCount);
			ZoneAllocPages(zone, pageCount, aType);
			}
		__KTRACE_OPT(KMMU2,Kern::Printf("Zone %d pages %x+%x base %08x",zone-iZones, offset, pageCount, base));
		++zone;
		offset = 0;
		}
	return KErrNone;
	}

/** Update the allocated page counts for the zone that is page is allocated into.

@param aAddr The physical address of the page
@param aOldPageType The type the page was allocated as
@param aNewPageType The type the page is changing to
*/
void DRamAllocator::ChangePageType(SPageInfo* aPageInfo, TZonePageType aOldType, TZonePageType aNewType)
	{

	TInt offset;
	SZone* zone = GetZoneAndOffset(aPageInfo->PhysAddr(), offset);
#ifdef _DEBUG
// ***********	System lock may be held while this is invoked so don't do********
// ***********	anything too slow and definitely don't call zone callback********
	M::RamAllocIsLocked();
	CHECK_PRECONDITIONS((MASK_THREAD_CRITICAL) & ~MASK_NO_FAST_MUTEX, "DRamAllocator::ChangePageType");

	// Get zone page is in and on debug builds check that it is allocated
	if (zone == NULL || zone->iBma[KBmaAllPages]->NotAllocated(offset, 1))
		{
		Panic(EAllocRamPagesInconsistent);
		}

	// Check if adjusting counts is valid, i.e. won't cause a roll over
	if (zone->iAllocPages[aOldType] - 1 > zone->iAllocPages[aOldType] ||
		zone->iAllocPages[aNewType] + 1 < zone->iAllocPages[aNewType])
		{
		__KTRACE_OPT(KMMU, Kern::Printf("ChangePageType Alloc Unk %x Fx %x Mv %x Dis %x",zone->iAllocPages[EPageUnknown],
					zone->iAllocPages[EPageFixed], zone->iAllocPages[EPageMovable],zone->iAllocPages[EPageDiscard]));
		Panic(EZonesCountErr);
		}
#endif

	// Update the counts and bmas
	zone->iAllocPages[aOldType]--;
	zone->iBma[aOldType]->Free(offset);
	zone->iAllocPages[aNewType]++;
	zone->iBma[aNewType]->Alloc(offset, 1);

	__KTRACE_OPT(KMMU2, Kern::Printf("ChangePageType Alloc Unk %x Fx %x Mv %x Dis %x",zone->iAllocPages[EPageUnknown],
					zone->iAllocPages[EPageFixed], zone->iAllocPages[EPageMovable],zone->iAllocPages[EPageDiscard]));
#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocChangePageType, aNewType, aPageInfo->PhysAddr());
#endif
	}

/**
Get the next page in this zone that is allocated after this one.

@param aZone	The zone to find the next allocated page in.
@param aOffset	On entry this is the offset from which the next allocated
				page in the zone should be found, on return it will be the offset 
				of the next allocated page.
@return KErrNone if a next allocated page could be found, KErrNotFound if no more pages in
the zone after aOffset are allocated, KErrArgument if aOffset is outside the zone.
*/
TInt DRamAllocator::NextAllocatedPage(SZone* aZone, TUint& aOffset, TZonePageType aType) const
	{
	const TUint KWordAlignMask = KMaxTUint32 << 5;

	M::RamAllocIsLocked();

	__NK_ASSERT_DEBUG(aZone - iZones < (TInt)iNumZones);
	// Makes things simpler for bma selection.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	TUint zoneEndOffset = aZone->iPhysPages - 1;
	if (aOffset > zoneEndOffset)
		{// Starting point is outside the zone
		return KErrArgument;
		}

	TUint wordIndex = aOffset >> 5;
	TUint endWordIndex = zoneEndOffset >> 5;

	// Select the BMA to search, 
	TUint bmaIndex = (aType == EPageTypes)? KBmaAllPages : aType;
	TUint32* map = &(aZone->iBma[bmaIndex]->iMap[wordIndex]);
	TUint32* mapEnd = &(aZone->iBma[bmaIndex]->iMap[endWordIndex]);
	TUint32 bits = *map;

	// Set bits for pages before 'offset' (i.e. ones we want to ignore)...
	bits |= ~(KMaxTUint32 >> (aOffset & ~KWordAlignMask));

	// Find the first bit map word from aOffset in aZone with allocated pages
	while (bits == KMaxTUint32 && map < mapEnd)
		{
		bits = *++map;
		}

	if (map == mapEnd)
		{// Have reached the last bit mask word so set the bits that are
		//  outside of the zone so that they are ignored.
		bits |= (KMaxTUint32 >> (zoneEndOffset & ~KWordAlignMask)) >> 1;
		}

	if (bits == KMaxTUint32)
		{// No allocated pages found after aOffset in aZone.
		return KErrNotFound;
		}

	// Now we have bits with allocated pages in it so determine the exact 
	// offset of the next allocated page
	TInt msOne = __e32_find_ms1_32(~bits);
	__NK_ASSERT_DEBUG(msOne >= 0);	// Must have at least one allocated page in the word.
	TUint msOneOffset = 31 - msOne;
	aOffset = ((map - aZone->iBma[bmaIndex]->iMap) << 5) + msOneOffset;
	return KErrNone;
	}


/**
Get the next run of pages in this zone that are allocated after aOffset.

@param aZone	The zone to find the next allocated page in.
@param aOffset	On entry this is the offset from which the next allocated
				page in the zone should be found, on return it will be the offset 
				of the next allocated page.
@param aEndOffset The last offset within this RAM zone to check for allocated runs.
@return The length of any run found.
*/
TInt DRamAllocator::NextAllocatedRun(SZone* aZone, TUint& aOffset, TUint aEndOffset, TZonePageType aType) const
	{
	const TUint KWordAlignMask = KMaxTUint32 << 5;

	M::RamAllocIsLocked();

	__NK_ASSERT_DEBUG(aZone - iZones < (TInt)iNumZones);
	// Makes things simpler for bma selection.
	__NK_ASSERT_DEBUG(aType != EPageUnknown);

	if (aOffset > aEndOffset)
		{// UnblockContiguous() has already searched the whole range for this page type.
		return 0;
		}

	TUint wordIndex = aOffset >> 5;
	TUint endWordIndex = aEndOffset >> 5;

	// Select the BMA to search, 
	TUint bmaIndex = (aType == EPageTypes)? KBmaAllPages : aType;
	TUint32* map = &(aZone->iBma[bmaIndex]->iMap[wordIndex]);
	TUint32* mapEnd = &(aZone->iBma[bmaIndex]->iMap[endWordIndex]);
	TUint32 bits = *map;

	// Set bits for pages before 'offset' (i.e. ones we want to ignore)...
	bits |= ~(KMaxTUint32 >> (aOffset & ~KWordAlignMask));

	// Find the first bit map word from aOffset in aZone with allocated pages
	while (bits == KMaxTUint32 && map < mapEnd)
		{
		bits = *++map;
		}

	if (map == mapEnd)
		{// Have reached the last bit mask word so set the bits that are
		//  outside of the range so that they are ignored.
		bits |= (KMaxTUint32 >> (aEndOffset & ~KWordAlignMask)) >> 1;
		}

	if (bits == KMaxTUint32)
		{// No allocated pages found in the range.
		aOffset = aEndOffset + 1;
		return 0;
		}

	// Now we have bits with allocated pages in it so determine the exact 
	// offset of the next allocated page
	TInt msOne = __e32_find_ms1_32(~bits);
	__NK_ASSERT_DEBUG(msOne >= 0);	// Must have at least one allocated page in the word.
	TUint msOneOffset = 31 - msOne;
	aOffset = ((map - aZone->iBma[bmaIndex]->iMap) << 5) + msOneOffset;
	TUint32* runWord = map;

	if (map < mapEnd && __e32_bit_count_32(~bits) == msOne + 1)
		{// The whole of the region in this word is allocated.
		// Find the next word which isn't completely allocated within the range.
		do
			{
			bits = *++map;
			}
		while (!bits && map < mapEnd);
		}

	// Clear any bits before the run so can get next free from __e32_find_msl_32().
	if (runWord == map)
		bits &= KMaxTUint32 >> (aOffset & ~KWordAlignMask);
	TInt msFree = __e32_find_ms1_32(bits);
	__NK_ASSERT_DEBUG(msFree >= 0 || map == mapEnd);
	TUint msFreeOffset = (msFree >= 0)? 31 - msFree : 32;
	TUint endIndex = map - aZone->iBma[bmaIndex]->iMap;
	TUint runEnd = (endIndex << 5) + msFreeOffset;
	if (runEnd > aEndOffset + 1)	// Ensure we don't go past the range.
		runEnd = aEndOffset + 1;
	__NK_ASSERT_DEBUG(runEnd > aOffset);

	return runEnd - aOffset;
	}


/**
See if any of the least preferable RAM zones can be emptied.  If they can then 
initialise the allocator for a general defragmentation operation.

Stage 0 of the general defrag is to ensure that there are enough free 
pages in the more preferable RAM zones to be in use after the general defrag
for the movable page allocations.  This is achieved by discarding the 
required amount of discardable pages from the more preferable RAM zones
to be in use after the general defrag.


@parm 	aInitialStage			On return this will contain the stage the general 
								defragmentation should begin at.  I.e. if no RAM 
								zones can be cleared then just perform the final
								tidying stage.
@param 	aRequiredToBeDiscarded	On return this will contain the number of 
								discardable pages that need to be discarded 
								from the RAM zones to be in use after the 
								general defrag.
@return Pointer to the RAM zone object that may potentially have pages
		discarded by the general defrag.  This will be NULL if no suitable 
		RAM zone could be found.
*/
SZone* DRamAllocator::GeneralDefragStart0(TGenDefragStage& aStage, TUint& aRequiredToBeDiscarded)
	{
#ifdef _DEBUG
	if (!K::Initialising) 
		{
		M::RamAllocIsLocked();
#ifdef __VERIFY_LEASTMOVDIS
		VerifyLeastPrefMovDis();
#endif
		}
	// Any previous general defrag operation must have ended.
	__NK_ASSERT_DEBUG(iZoneGeneralPrefLink == NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralTmpLink == NULL);
#endif

	if (iNumZones == 1)
		{
		// Only have one RAM zone so a defrag can't do anything.
		return NULL;
		}

	// Determine how many movable or discardable pages are required to be allocated.
	TUint requiredPagesDis = 0;
	TUint requiredPagesMov = 0;
	TUint firstClearableInUseRank = 0;
	SDblQueLink* link = iZoneLeastMovDis;
	do
		{
		SZone& zone = *_LOFF(link, SZone, iPrefLink);
		requiredPagesDis += zone.iAllocPages[EPageDiscard];
		requiredPagesMov += zone.iAllocPages[EPageMovable];
		
		if (!firstClearableInUseRank && 
			(zone.iAllocPages[EPageMovable] || zone.iAllocPages[EPageDiscard]) &&
			!zone.iAllocPages[EPageFixed] && !zone.iAllocPages[EPageUnknown])
			{// This is the least preferable RAM zone that is has movable or 
			// discardable but may be clearable as it has no immovable pages.
			firstClearableInUseRank = zone.iPrefRank;
			}

		// Reset KRamZoneFlagGenDefrag flag bit for each RAM zone to be defraged.
		zone.iFlags &= ~(KRamZoneFlagGenDefrag | KRamZoneFlagGenDefragBlock);

		link = link->iPrev;
		}
	while (link != &iZonePrefList.iA);

	// Adjust the number of discardable pages for those that are freeable.
	// Dirty pages will be moved rather than discarded so they are not freeable
	// and we must make sure that we have enough space in zones for these dirty 
	// paged pages.
	__NK_ASSERT_DEBUG(requiredPagesDis >= (TUint)M::NumberOfFreeDpPages());
	requiredPagesDis -= M::NumberOfFreeDpPages();
	TUint totalDirtyPagesDis = M::NumberOfDirtyDpPages();
	if (requiredPagesDis < totalDirtyPagesDis)
		requiredPagesDis = totalDirtyPagesDis;

	// Determine which is the least preferable RAM zone that needs to be 
	// in use for required number of movable and discardable pages.
	TUint onlyPagesDis = 0;		// Number of pages in RAM zones for discard only.
	TUint onlyPagesMov = 0;		// Number of pages in RAM zones for movable only.
	TUint totalPagesDis = 0;	// Total pages found so far for discardable pages.
	TUint totalPagesMov = 0;	// Total pages found so far for movable pages.
	TUint totalCurrentDis = 0;	// Number of allocated discardable pages found in 
								// RAM zones to be in use after the general defrag.
	TUint totalCurrentMov = 0;	// Number of allocated movable pages found in 
								// RAM zones to be in use after the general defrag.
	TUint totalCurrentFree = 0; // The current number of free pages in the RAM zones
								// to be in use after the general defrag.
	iZoneGeneralPrefLink = &iZonePrefList.iA;
	while (iZoneGeneralPrefLink != iZoneLeastMovDis && 
			(requiredPagesMov > totalPagesMov ||
			requiredPagesDis > totalPagesDis))
		{
		iZoneGeneralPrefLink = iZoneGeneralPrefLink->iNext;
		SZone& zone = *_LOFF(iZoneGeneralPrefLink, SZone, iPrefLink);
		// Update the current totals.
		totalCurrentDis += zone.iAllocPages[EPageDiscard];
		totalCurrentMov += zone.iAllocPages[EPageMovable];
		totalCurrentFree += zone.iFreePages;

		TBool onlyAllocDis = NoAllocOfPageType(zone, EPageMovable);
		TBool onlyAllocMov = NoAllocOfPageType(zone, EPageDiscard);
		if (!onlyAllocMov || !onlyAllocDis)
			{// Either movable, discardable or both can be allocated in this zone.
			TUint zonePagesFree = zone.iFreePages;
			TUint zonePagesDis = zone.iAllocPages[EPageDiscard];
			TUint zonePagesMov = zone.iAllocPages[EPageMovable];
			// Total pages in this RAM zone that can be used for either 
			// discardable or movable pages.
			TUint zonePagesGen = zonePagesDis + zonePagesMov + zonePagesFree;
			if (onlyAllocMov)
				{
				if (requiredPagesDis > totalPagesDis)
					{// No further discardable pages can be allocated into
					// this RAM zone but consider any that already are.
					TUint usedPages = Min(	(TInt)zonePagesDis,
											requiredPagesDis - totalPagesDis);
					totalPagesDis += usedPages;
					zonePagesDis -= usedPages;
					}
				TUint zoneOnlyMov = zonePagesDis + zonePagesMov + zonePagesFree;
				onlyPagesMov += zoneOnlyMov;
				totalPagesMov += zoneOnlyMov;
				__KTRACE_OPT(KMMU2, Kern::Printf("onlyMov ID%x tot %x", 
									zone.iId, zoneOnlyMov));
				zonePagesGen = 0;	// These pages aren't general purpose.
				}
			if (onlyAllocDis)
				{
				if (requiredPagesMov > totalPagesMov)
					{// No further movable pages can be allocated into
					// this RAM zone but consider any that already are.
					TUint usedPages = Min(	(TInt)zonePagesMov,
											requiredPagesMov - totalPagesMov);
					totalPagesMov += usedPages;
					zonePagesMov -= usedPages;
					}
				TUint zoneOnlyDis = zonePagesDis + zonePagesMov + zonePagesFree;
				onlyPagesDis +=	zoneOnlyDis;
				totalPagesDis += zoneOnlyDis;
				__KTRACE_OPT(KMMU2, Kern::Printf("onlyDis ID%x tot %x", 
									zone.iId, zoneOnlyDis));
				zonePagesGen = 0;	// These pages aren't general purpose.
				}

			if (requiredPagesDis > totalPagesDis)
				{// Need some discardable pages so first steal any spare 
				// movable pages for discardable allocations.
				if (totalPagesMov > requiredPagesMov)
					{// Use any spare movable pages that can also be 
					// used for discardable allocations for discardable.
					__NK_ASSERT_DEBUG(onlyPagesMov);
					TUint spareMovPages = Min((TInt)(totalPagesMov - onlyPagesMov),
												totalPagesMov - requiredPagesMov);
					totalPagesMov -= spareMovPages;
					totalPagesDis += spareMovPages;
					__KTRACE_OPT(KMMU2, Kern::Printf("genDis Mov ID%x used%x", 
										zone.iId, spareMovPages));
					}
				if (requiredPagesDis > totalPagesDis)
					{
					// Need more discardable pages but only grab those required.
					TUint usedPages = Min(	(TInt) zonePagesGen, 
											requiredPagesDis - totalPagesDis);
					totalPagesDis += usedPages;
					zonePagesGen -= usedPages;
					__KTRACE_OPT(KMMU2, Kern::Printf("genDis ID%x used%x", 
										zone.iId, usedPages));
					}
				}
			if (requiredPagesMov > totalPagesMov)
				{// Need some movable pages so first steal any spare 
				// discardable pages for movable allocations.
				if (totalPagesDis > requiredPagesDis)
					{// Use any spare discardable pages that can also be 
					// used for movable allocations for movable.
					__NK_ASSERT_DEBUG(onlyPagesDis);
					TUint spareDisPages = Min((TInt)(totalPagesDis - onlyPagesDis),
												totalPagesDis - requiredPagesDis);
					totalPagesDis -= spareDisPages;
					totalPagesMov += spareDisPages;
					__KTRACE_OPT(KMMU2, Kern::Printf("genMov Dis ID%x used%x", 
										zone.iId, spareDisPages));
					}
				if (requiredPagesMov > totalPagesMov)
					{// Still need some movable pages so grab them from this zone.
					// Just grab all of the general pages left as discard pages will
					// have already grabbed some if it had needed to.
					totalPagesMov += zonePagesGen;
					__KTRACE_OPT(KMMU2, Kern::Printf("genMov ID%x used%x", 
										zone.iId, zonePagesGen));
					}
				}	
			}
		}

	__KTRACE_OPT(KMMU, Kern::Printf("gen least in use ID 0x%x", 
				(_LOFF(iZoneGeneralPrefLink, SZone, iPrefLink))->iId));
	__NK_ASSERT_DEBUG(_LOFF(iZoneGeneralPrefLink, SZone, iPrefLink)->iPrefRank <= 
						iZoneLeastMovDisRank);

	if (iZoneGeneralPrefLink != iZoneLeastMovDis &&
		firstClearableInUseRank > _LOFF(iZoneGeneralPrefLink, SZone, iPrefLink)->iPrefRank)
		{// We can reduce the number of RAM zones in use so block all the RAM
		// zones not to be in use after the defrag from being allocated into 
		// by the general defrag.
		link = iZoneLeastMovDis;
		while (link != iZoneGeneralPrefLink)
			{
			SZone& zone = *_LOFF(link, SZone, iPrefLink);
			zone.iFlags |= KRamZoneFlagGenDefragBlock;
			link = link->iPrev;
			}

		// Determine how many pages will need to be discarded to allow general 
		// defrag to succeed in using the minimum RAM zones required.
		if (requiredPagesDis > totalCurrentDis)
			{// Need to replace some discardable pages in RAM zones to be 
			// cleared with pages in the RAM zones to be in use after the 
			// general defrag.
			__NK_ASSERT_DEBUG(totalCurrentFree >= requiredPagesDis - totalCurrentDis);
			totalCurrentFree -= requiredPagesDis - totalCurrentDis;
			}
		TUint totalForMov = totalCurrentFree + totalCurrentMov;
		if (requiredPagesMov > totalForMov)
			{// Need to discard some pages from the least preferable RAM zone to be
			// in use after the general for the movable pages to be moved to.
			aRequiredToBeDiscarded = requiredPagesMov - totalForMov;
			__NK_ASSERT_DEBUG(aRequiredToBeDiscarded <= totalCurrentDis);
			__NK_ASSERT_DEBUG(totalCurrentDis - aRequiredToBeDiscarded >= requiredPagesDis);
			}

		// This stage should discard pages from the least preferable RAM zones
		// to be in use after the general defrag to save the pages having to
		// be moved again by the final stage.
		iZoneGeneralStage = EGenDefragStage0;
		aStage = EGenDefragStage1;	// Defrag::GeneralDefrag() requires this.
		iZoneGeneralTmpLink = iZoneGeneralPrefLink;
		return GeneralDefragNextZone0();
		}

	// General defrag can't clear any RAM zones so jump to tidying stage.
	aStage = EGenDefragStage2;
	iZoneGeneralStage = EGenDefragStage2;
	return NULL;
	}


/**
Find the next RAM zone that is suitable for stage 0 of a general defrag.
This should only be called after a preceeding call to 
DRamAllocator::GeneralDefragStart0().

This goes through the RAM zones from the least preferable to be in use
after the general defrag to the most preferable RAM zone. It will 
return each time it finds a RAM zone with discardable pages allocated into it.

@return Pointer to the RAM zone object that may potentially have pages
		discarded by the general defrag.  This will be NULL if no suitable 
		RAM zone could be found.
*/
SZone* DRamAllocator::GeneralDefragNextZone0()
	{
	M::RamAllocIsLocked();
	// Any previous general defrag operation must have ended.
	__NK_ASSERT_DEBUG(iZoneGeneralPrefLink != NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralTmpLink != NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralStage == EGenDefragStage0);

	while (iZoneGeneralTmpLink != &iZonePrefList.iA)
		{
		SZone* zone = _LOFF(iZoneGeneralTmpLink, SZone, iPrefLink);

		// Save the RAM zone that is currently more preferable than this one
		// before any reordering.
		iZoneGeneralTmpLink = iZoneGeneralTmpLink->iPrev;

		if (zone->iFlags & KRamZoneFlagGenDefrag)
			{// This zone has been selected for a general defrag already.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext0 zone ID 0x%x already defraged", 
						zone->iId));
			return NULL;
			}
		zone->iFlags |= KRamZoneFlagGenDefrag;
		if (zone->iAllocPages[EPageDiscard])
			{
			// A RAM zone that may have pages discarded by a general defrag has been found.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext0 zone ID 0x%x", zone->iId));
			return zone;
			}
		}
	return NULL;
	}


/**
Initialise this stage of a general defrag operation which will attempt
to clear all the RAM zones not to be in use once the general defrag
has completed.

@return Pointer to the RAM zone object that may potentially be cleared
		by the general defrag.  This will be NULL if no suitable 
		RAM zone could be found.
*/
SZone* DRamAllocator::GeneralDefragStart1()
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(iNumZones == 1 || iZoneGeneralPrefLink != NULL);


	if (iNumZones == 1)
		{// On a device with one RAM zone can't do any defrag so return NULL.
		return NULL;
		}

	// Clear general defrag flags of each RAM zone to be defraged.
	SDblQueLink* link = iZoneGeneralPrefLink;
	for (; link != &iZonePrefList.iA; link = link->iPrev)
		{
		SZone& zone = *_LOFF(link, SZone, iPrefLink);
		zone.iFlags &= ~KRamZoneFlagGenDefrag;
		}
	
	// Flags cleared so now to start this stage from least preferable RAM zone
	// currently in use.
	iZoneGeneralTmpLink = iZoneLeastMovDis;
	iZoneGeneralStage = EGenDefragStage1;
	return GeneralDefragNextZone1();
	}


/**
Find the next RAM zone that is suitable for stage 1 of a general defrag.
This should only be called after a preceeding call to 
DRamAllocator::GeneralDefragStart1().

This goes through the RAM zones from the least preferable currently 
with movable or discardable pages allocated into it to the least 
preferable RAM zone that is to be in use after the general defrag.
It will return each time it finds a RAM zone with movable and/or 
discardable pages allocated into it.

@return Pointer to the RAM zone object that may potentially be cleared by a 
		general defrag.  This will be NULL if no suitable zone could be found.
*/
SZone* DRamAllocator::GeneralDefragNextZone1()
	{
	M::RamAllocIsLocked();
	// Any previous general defrag operation must have ended.
	__NK_ASSERT_DEBUG(iZoneGeneralPrefLink != NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralTmpLink != NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralStage == EGenDefragStage1);


	// If we hit the target least preferable RAM zone to be in use once 
	// the defrag has completed then stop this stage of the general defrag.

	// Should never skip past iZoneGeneralPrefLink.
	__NK_ASSERT_DEBUG(iZoneGeneralTmpLink != &iZonePrefList.iA);

	while (iZoneGeneralTmpLink != iZoneGeneralPrefLink)
		{
		SZone* zone = _LOFF(iZoneGeneralTmpLink, SZone, iPrefLink);

		// Save the RAM zone that is currently more preferable than this one
		// before any reordering.
		iZoneGeneralTmpLink = iZoneGeneralTmpLink->iPrev;

		if (zone->iFlags & KRamZoneFlagGenDefrag)
			{// This zone has been selected for a general defrag already.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext1 zone ID 0x%x already defraged", 
						zone->iId));
			return NULL;
			}
		zone->iFlags |= KRamZoneFlagGenDefrag;
		if (zone->iAllocPages[EPageMovable] || zone->iAllocPages[EPageDiscard])
			{
			// A RAM zone that may be cleared by a general defrag has been found.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext1 zone ID 0x%x", zone->iId));
			return zone;
			}
		}
	__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext1 reached general target"));
	return NULL;
	}


/**
Initialise stage 2 of a general defrag operation.

Stage 2 creates room for fixed pages allocations in the more preferable RAM 
zones in use by moving pages into the least preferable RAM zones in use.

@return Pointer to the RAM zone object that may potentially be cleared of
		movable and discardable pages by the general defrag.  This will be 
		NULL if no suitable zone could be found.
*/
SZone* DRamAllocator::GeneralDefragStart2()
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(iNumZones == 1 || iZoneGeneralPrefLink != NULL);


	if (iNumZones == 1)
		{// On a device with one RAM zone can't do any defrag so return NULL.
		return NULL;
		}

	// Clear general defrag flags of each RAM zone to be defraged.
	SDblQueLink* link = iZoneLeastMovDis;
	for (; link != &iZonePrefList.iA; link = link->iPrev)
		{
		SZone& zone = *_LOFF(link, SZone, iPrefLink);
		zone.iFlags &= ~(KRamZoneFlagGenDefrag | KRamZoneFlagGenDefragBlock);
		}
	
	// Flags cleared so now to start 2nd stage from most preferable RAM zone.
	iZoneGeneralTmpLink = iZonePrefList.First();
	iZoneGeneralStage = EGenDefragStage2;
	return GeneralDefragNextZone2();
	}


/**
Find the next RAM zone that is suitable for this stage of general defrag.
This should only be called after a preceeding call to 
DRamAllocator::GeneralDefragStart2().

This goes through the RAM zones from the most preferable to the least 
preferable RAM zone that has movable and/or discardable pages allocated
into it.  It will return each time it finds a RAM zone with movable and/or 
discardable pages allocated into it.

@return Pointer to the RAM zone object that may potentially be cleared of
		movable and discardable pages by the general defrag.  This will be 
		NULL if no suitable zone could be found.
*/
SZone* DRamAllocator::GeneralDefragNextZone2()
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(iZoneGeneralTmpLink != NULL);
	__NK_ASSERT_DEBUG(iZoneGeneralStage == EGenDefragStage2);


	while (iZoneGeneralTmpLink != iZoneLeastMovDis)
		{
		SZone* zone = _LOFF(iZoneGeneralTmpLink, SZone, iPrefLink);

		// Save the RAM zone that is currently less preferable than this one
		// before any reordering.
		iZoneGeneralTmpLink = iZoneGeneralTmpLink->iNext; 

		if (zone->iFlags & KRamZoneFlagGenDefrag)
			{// This zone has been selected for a general defrag already.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext2 zone ID 0x%x already defraged", zone->iId));
			return NULL;
			}
		zone->iFlags |= KRamZoneFlagGenDefrag | KRamZoneFlagGenDefragBlock;
		if (zone->iAllocPages[EPageMovable] || zone->iAllocPages[EPageDiscard])
			{// A RAM zone that may be cleared by a general defrag has been found.
			__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext2 zone ID 0x%x", zone->iId));
			return zone;
			}
		}
	__KTRACE_OPT(KMMU, Kern::Printf("GenDefragNext2 reached general target"));
	return NULL;
	}

/**
Inform the allocator that a general defragmentation operation has completed.

*/
void DRamAllocator::GeneralDefragEnd()
	{
#ifdef _DEBUG
	if (!K::Initialising) 
		{
		M::RamAllocIsLocked();
#ifdef __VERIFY_LEASTMOVDIS
		VerifyLeastPrefMovDis();
#endif
		}
#endif
	// Reset the general defrag preference link as it is no longer required.
	iZoneGeneralPrefLink = NULL;
	iZoneGeneralTmpLink = NULL;
	}


/**
Calculate the number of free pages in all the RAM zones to be in use
once the general defragmentation operation has completed.

@param aType The type of free pages to find in the higher priority zones.
@return The number of free pages in the RAM zones intended to be in use 
after the general defrag operation has completed.
*/
TUint DRamAllocator::GenDefragFreePages(TZonePageType aType) const
	{
	M::RamAllocIsLocked();

	if (iZoneGeneralStage == EGenDefragStage2)
		{// Second stage of general defrag where don't have to empty the RAM zone.
		return KMaxTUint;
		}
	TUint totalFree = 0;
	SDblQueLink* link = iZoneGeneralPrefLink;
	for (; link != &iZonePrefList.iA; link = link->iPrev)
		{
		SZone& zone = *_LOFF(link, SZone, iPrefLink);
		if (NoAllocOfPageType(zone, aType) ||
			zone.iFlags & KRamZoneFlagGenDefragBlock)
			{
			continue;
			}
		// This zone has free space for this type of page
		totalFree += zone.iFreePages;
		}
	return totalFree;
	}


/** Mark the RAM zone as being claimed to stop any further allocations.
@param aZone The zone to stop allocations to.

@pre RamAlloc mutex held.
@post RamAlloc mutex held.
*/
void DRamAllocator::ZoneClaimStart(SZone& aZone)
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(!(aZone.iFlags & KRamZoneFlagClaiming));

	aZone.iFlags |= KRamZoneFlagClaiming;

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocZoneFlagsModified, aZone.iId, aZone.iFlags);
#endif
	}


/** Mark the RAM zone as not being claimed to allow allocations.
@param aZone The zone to allow allocations into.

@pre RamAlloc mutex held.
@post RamAlloc mutex held.
*/
void DRamAllocator::ZoneClaimEnd(SZone& aZone)
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(aZone.iFlags & KRamZoneFlagClaiming);

	aZone.iFlags &= ~KRamZoneFlagClaiming;

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocZoneFlagsModified, aZone.iId, aZone.iFlags);
#endif
	}

/** Mark the RAM zone so that any allocation or frees from it can be detected.
Useful for defragging.
@param aZone The zone to mark.
@pre RamAlloc mutex held
@post RamAlloc mutex held
*/
void DRamAllocator::ZoneMark(SZone& aZone)
	{
	M::RamAllocIsLocked();
	__NK_ASSERT_DEBUG(!(aZone.iFlags & KRamZoneFlagMark));

	aZone.iFlags |= KRamZoneFlagMark;

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocZoneFlagsModified, aZone.iId, aZone.iFlags);
#endif
	}

/** Unmark the RAM zone.
Useful for defragging.
@param aZone The zone to mark.
@return ETrue if the RAM zone is inactive, EFalse otherwise.
@pre RamAlloc mutex held
@post RamAlloc mutex held
*/
TBool DRamAllocator::ZoneUnmark(SZone& aZone)
	{
	M::RamAllocIsLocked();

	TInt r = aZone.iFlags & KRamZoneFlagMark;
	aZone.iFlags &= ~KRamZoneFlagMark;

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocZoneFlagsModified, aZone.iId, aZone.iFlags);
#endif
	return r;
	}

/** Determine whether it is OK to allocate the specified page type
to the RAM zone.

This should be used by all functions that search through the zones when
attempting to allocate pages.

@return ETrue if this page type shouldn't be allocated into the RAM zone,
EFalse if it is OK to allocate that page type into the RAM zone.
*/
TBool DRamAllocator::NoAllocOfPageType(SZone& aZone, TZonePageType aType) const
	{
	TUint8 flagMask = 1 << (aType - KPageTypeAllocBase);
	return 	(aZone.iFlags & (KRamZoneFlagClaiming|KRamZoneFlagNoAlloc|KRamZoneFlagTmpBlockAlloc)) ||
			(aZone.iFlags & flagMask);
	}


/** Updates the flags of the specified RAM zone.

@param aId			The ID of the RAM zone to modify.
@param aClearFlags	The bit flags to clear.
@param aSetFlags	The bit flags to set.

@return KErrNone on success, KErrArgument if the RAM zone of aId not found or
aSetMask contains invalid flags.

@pre RamAlloc mutex held
@post RamAlloc mutex held
*/
TInt DRamAllocator::ModifyZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask)
	{
	M::RamAllocIsLocked();

	SZone* zone = ZoneFromId(aId);
	if (zone == NULL || (aSetMask & KRamZoneFlagInvalid))
		{// aId invalid or an invalid flag bit was requested to be set.
		return KErrArgument;
		}
	zone->iFlags &= ~aClearMask;
	zone->iFlags |= aSetMask;

	__KTRACE_OPT(KMMU, Kern::Printf("Zone %x Flags %x", zone->iId, zone->iFlags));

#ifdef BTRACE_RAM_ALLOCATOR
	BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocZoneFlagsModified, zone->iId, zone->iFlags);
#endif
	return KErrNone;
	}


/** Invoke the RAM zone call back function to inform the variant of the RAM zones
in use so far by the system.
This is designed to only be invoked once during boot in MmuBase::Init2()
*/
void DRamAllocator::InitialCallback()
	{
	__NK_ASSERT_DEBUG(iZoneCallbackInitSent == EFalse);
	if (iZonePowerFunc)
		{
		TInt ret = (*iZonePowerFunc)(ERamZoneOp_Init, NULL, (TUint*)&iZonePwrState);
		if (ret != KErrNone && ret != KErrNotSupported)
			{
			Panic(EZonesCallbackErr);
			}
		CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "DRamAllocator::InitialCallback");
		}
	iZoneCallbackInitSent = ETrue;
	}


#ifdef BTRACE_RAM_ALLOCATOR
/**
Structure for outputing zone information to BTrace that couldn't be fit into first
2 words of the BTraceN call
*/
struct TRamAllocBtraceZone
	{
	TUint32 iId;
	TUint8 iPref;
	TUint8 iFlags;
	TUint16 iReserved;
	};

/**
This will be invoked when BTrace starts logging BTrace::ERamAllocator category 
traces.
It outputs the zone configuration and the base addresses of any contiguous block
of allocated pages.
*/
void DRamAllocator::DoBTracePrime(void)
	{
	M::RamAllocIsLocked();
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "DRamAllocator::SendInitialBtraceLogs");

	// Output the zone information
	TRamAllocBtraceZone bZone;
	BTrace4(BTrace::ERamAllocator, BTrace::ERamAllocZoneCount, iNumZones);
	const SZone* zone = iZones;
	const SZone* const  endZone = iZones + iNumZones;
	for (; zone < endZone; zone++)
		{
		bZone.iId = zone->iId;
		bZone.iPref = zone->iPref;
		bZone.iFlags = zone->iFlags;
		BTraceN(BTrace::ERamAllocator, BTrace::ERamAllocZoneConfig, zone->iPhysPages,
				zone->iPhysBase, &bZone, sizeof(TRamAllocBtraceZone));
		}

	// Search through zones and output each contiguous region of allocated pages
	for (zone = iZones; zone < endZone; zone++)
		{
		if (zone->iFreePages != zone->iPhysPages)
			{
			TInt pageCount = 0;
			TInt totalPages = 0;
			TUint32 runStart = 0;
			while ((TUint)totalPages != zone->iPhysPages - zone->iFreePages)
				{
				// find set of contiguous pages that have been allocated
				// runStart will be set to first page of allocated run if one found
				for (;runStart < zone->iPhysPages && zone->iBma[KBmaAllPages]->NotAllocated(runStart,1);	runStart++);

				// find last allocated page of this run
				TUint32 runEnd = runStart + 1;
				for (;runEnd < zone->iPhysPages && zone->iBma[KBmaAllPages]->NotFree(runEnd,1); runEnd++);
				
				pageCount = runEnd - runStart;
				if (pageCount > 0)
					{// have a run of allocated pages so output BTrace
					TPhysAddr baseAddr = (runStart << KPageShift) + zone->iPhysBase;
					__KTRACE_OPT(KMMU2, Kern::Printf("offset %x physBase %x pages %x baseAddr %08x",runStart, zone->iPhysBase, pageCount, baseAddr));
					BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocBootAllocation, pageCount, baseAddr);
					runStart += pageCount;
					totalPages += pageCount;
					}
				}
			}
		}
	BTrace0(BTrace::ERamAllocator, BTrace::ERamAllocBootAllocationEnd);
	}
#endif // BTRACE_RAM_ALLOCATOR

TInt DRamAllocator::ClaimPhysicalRam(TPhysAddr aBase, TInt aSize)
	{
	TInt ret = SetPhysicalRamState(aBase,aSize,EFalse, EPageFixed);
#ifdef BTRACE_RAM_ALLOCATOR
	if (ret == KErrNone)
		{
		BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocClaimRam, aSize, aBase);
		}
#endif
	return ret;
	}

TInt DRamAllocator::FreePhysicalRam(TPhysAddr aBase, TInt aSize)
	{
	TInt ret = SetPhysicalRamState(aBase,aSize,ETrue, EPageFixed); 
#ifdef BTRACE_RAM_ALLOCATOR
	if (ret == KErrNone)
		{
		BTrace8(BTrace::ERamAllocator, BTrace::ERamAllocFreePhysical, aSize, aBase);
		}
#endif
	return ret;
	}


TInt DRamAllocator::FreeRamInBytes()
	{
	return iTotalFreeRamPages<<KPageShift;
	}

TUint DRamAllocator::FreeRamInPages()
	{
	return iTotalFreeRamPages;
	}

TUint DRamAllocator::TotalPhysicalRamPages()
	{
	return iTotalRamPages;
	}

#ifdef __VERIFY_LEASTMOVDIS
void DRamAllocator::VerifyLeastPrefMovDis()
	{
	// Shouldn't have any movable or discardable pages in any RAM
	// zone less preferable than iZoneLeastMovDis
	SDblQueLink* tmpLink = iZoneLeastMovDis->iNext;
	while (tmpLink != &iZonePrefList.iA)
		{
		SZone& zone = *_LOFF(tmpLink, SZone, iPrefLink);
		if (zone.iAllocPages[EPageMovable] != 0 ||
			zone.iAllocPages[EPageDiscard] != 0)
			{
			DebugDump();
			__NK_ASSERT_DEBUG(0);
			}
		tmpLink = tmpLink->iNext;
		}
	}
#endif
