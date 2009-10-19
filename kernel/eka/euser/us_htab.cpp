// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/euser/us_htab.cpp
// 
//

#include "us_std.h"
#include <e32hashtab.h>

const TUint KDefaultIndexBits = 4;
const TUint KMaxIndexBits = 28;

extern TUint32 DefaultIntegerHash(const TAny*);
extern TUint32 DefaultStringHash(const TUint8*, TInt);
extern TUint32 DefaultWStringHash(const TUint16*, TInt);

#define	_DEBUG_HASH_TABLE
#ifndef	_DEBUG
#undef	_DEBUG_HASH_TABLE
#endif

#define __PANIC(x) Panic(x)

EXPORT_C RHashTableBase::RHashTableBase(TGeneralHashFunction32 aHash, TGeneralIdentityRelation aId, TInt aElementSize, TInt aKeyOffset)
	:	iHashFunc(aHash),
		iIdFunc(aId),
		iIndexBits(TUint8(KDefaultIndexBits)),
		iGeneration(EGen0),
		iPad0(0),
		iElements(0),
		iCount(0),
		iPad1(0),
		iPad2(0)
	{
	__ASSERT_ALWAYS(aHash!=NULL, __PANIC(EHashTableNoHashFunc));
	__ASSERT_ALWAYS(aId!=NULL, __PANIC(EHashTableNoIdentityRelation));
	__ASSERT_ALWAYS(aElementSize>0, __PANIC(EHashTableBadElementSize));
	__ASSERT_ALWAYS(aKeyOffset==0 || TUint(aKeyOffset-4)<(TUint)Min(252,aElementSize-4), __PANIC(EHashTableBadKeyOffset));
	iElementSize = aElementSize;
	iKeyOffset = (TUint8)aKeyOffset;	// 0 means ptr at offset 4
	iEmptyCount = 0;
	SetThresholds();
	}

void RHashTableBase::SetThresholds()
	{
	TUint32 max = 1u << iIndexBits;
	if (iIndexBits == KMaxIndexBits)
		iUpperThreshold = KMaxTUint;
	else
		iUpperThreshold = (max>>1) + (max>>2);	// 3/4 of max
	if (iIndexBits == KDefaultIndexBits)
		iLowerThreshold = 0;
	else
		iLowerThreshold = max >> 2;				// 1/4 of max

	// clean table if <1/8 of entries empty
	iCleanThreshold = max>>3;
	}

EXPORT_C void RHashTableBase::Close()
	{
	User::Free(iElements);
	new (this) RHashTableBase(iHashFunc, iIdFunc, iElementSize, iKeyOffset);
	}

EXPORT_C TInt RHashTableBase::Count() const
	{
	return (TInt)iCount;
	}

EXPORT_C TAny* RHashTableBase::Find(const TAny* aKey, TInt aOffset) const
	{
	if (!iElements)
		return NULL;
	TUint32 hash = (*iHashFunc)(aKey);
	TUint32 ix = hash >> (32 - iIndexBits);		// top bits of hash used as initial index
	hash = (hash &~ EStateMask) | iGeneration;
	TUint32 mask = (1u << iIndexBits) - 1;		// iIndexBits 1's
	TUint32 step = (hash >> 1) & mask;			// iIndexBits-1 LSBs of hash followed by 1
	FOREVER
		{
		const SElement* e = ElementC(ix);
		if (e->iHash==hash && (*iIdFunc)(aKey, GetKey(e)))
			{
			if (aOffset >= 0)
				return ((TUint8*)e) + aOffset;
			return *(TAny**)((TUint8*)e - aOffset);
			}
		if (e->IsEmpty())
			break;
		ix = (ix + step) & mask;
		}
	return NULL;
	}

EXPORT_C TAny* RHashTableBase::FindL(const TAny* aKey, TInt aOffset) const
	{
	TAny* p = Find(aKey, aOffset);
	if (!p)
		User::Leave(KErrNotFound);
	return p;
	}

TInt RHashTableBase::Insert(const TAny* aKey, TAny*& aElement)
	{
	TInt r = KErrNone;
	TUint32 max = 1u << iIndexBits;
	if (!iElements)
		{
		iElements = User::AllocZ(max * iElementSize);
		if (!iElements)
			return KErrNoMemory;
		iEmptyCount = max;
		}
	else if (iCount > iUpperThreshold)
		{
		r = ExpandTable(iIndexBits+1);
		if (iEmptyCount>1)
			r = KErrNone;	// doesn't matter if expand fails unless there is only one empty slot left
		max = 1u << iIndexBits;
		}
	else if (iEmptyCount < iCleanThreshold)
		ReformTable(iIndexBits);

	TUint32 hash = (*iHashFunc)(aKey);
	TUint32 ix = hash >> (32 - iIndexBits);
	TUint32 mask = max - 1;
	hash = (hash &~ EStateMask) | iGeneration;
	TUint32 step = (hash >> 1) & mask;			// iIndexBits-1 LSBs of hash followed by 1
	SElement* e = 0;
	SElement* d = 0;
	FOREVER
		{
		e = Element(ix);
		if (e->IsEmpty())
			break;
		if (e->IsDeleted())
			{
			if (!d)
				d = e;
			}
		else if (e->iHash==hash && (*iIdFunc)(aKey, GetKey(e)))
			{
			aElement = e;
			return KErrNone;	// duplicate so always succeed
			}
		ix = (ix + step) & mask;
		}
	if (d)
		e = d;		// if we can reuse a deleted slot, always succeed
	else
		{
		if (r!=KErrNone)
			return r;	// new slot needed - if we failed to expand, fail the request here
		--iEmptyCount;
		}
	e->iHash = hash;
	aElement = e;
	++iCount;
	return KErrNone;
	}

EXPORT_C TInt RHashTableBase::PtrInsert(const TAny* aKey, const TAny* aValue)
	{
	const TAny** e;
	TInt r = Insert(aKey, (TAny*&)e);
	if (r==KErrNone)
		{
		e[1] = aKey;
		if (iElementSize>=12)
			e[2] = aValue;
		}
	return r;
	}

EXPORT_C void RHashTableBase::PtrInsertL(const TAny* aKey, const TAny* aValue)
	{
	const TAny** e;
	User::LeaveIfError(Insert(aKey, (TAny*&)e));
	e[1] = aKey;
	if (iElementSize>=12)
		e[2] = aValue;
	}

EXPORT_C TInt RHashTableBase::ValueInsert(const TAny* aKey, TInt aKeySize, const TAny* aValue, TInt aValueOffset, TInt aValueSize)
	{
	TUint8* e;
	TInt r = Insert(aKey, (TAny*&)e);
	if (r==KErrNone)
		{
		memcpy(e+iKeyOffset, aKey, aKeySize);
		if (aValue)
			memcpy(e+aValueOffset, aValue, aValueSize);
		}
	return r;
	}

EXPORT_C void RHashTableBase::ValueInsertL(const TAny* aKey, TInt aKeySize, const TAny* aValue, TInt aValueOffset, TInt aValueSize)
	{
	TUint8* e;
	User::LeaveIfError(Insert(aKey, (TAny*&)e));
	memcpy(e+iKeyOffset, aKey, aKeySize);
	if (aValue)
		memcpy(e+aValueOffset, aValue, aValueSize);
	}

EXPORT_C TInt RHashTableBase::Remove(const TAny* aKey)
	{
	SElement* e = (SElement*)Find(aKey);
	if (!e)
		return KErrNotFound;
	e->SetDeleted();
	if (--iCount == 0)
		{
		Close();
		return KErrNone;
		}
	if (iCount < iLowerThreshold)
		ShrinkTable();
	return KErrNone;
	}

void RHashTableBase::ReformTable(TUint aNewIndexBits)
	{
	if (!iElements)
		return;
	TUint32 max = 1u << iIndexBits;
	TUint32 newmax = 1u << aNewIndexBits;
	TUint32 newmask = newmax - 1;
	TUint32 ix = 0;
	TUint32 newsh = 32 - aNewIndexBits;
	iGeneration ^= 1;	// change generation so we know which entries have been updated
	for (; ix < max; ++ix)
		{
		SElement* e = Element(ix);
		if (e->IsEmpty())
			continue;		// skip empty entries
		if (e->IsDeleted())
			{
			e->SetEmpty();	// mark deleted entries as empty
			continue;
			}
		if ((e->iHash & EStateMask) == iGeneration)	// entry has been processed so leave it alone
			continue;
		TUint32 pos = e->iHash >> newsh;
		if (pos == ix)
			{
			e->iHash ^= 1;		// entry is in first position for its hash so leave it there
			continue;
			}
		TUint32 step = (e->iHash >> 1) & newmask;
		FOREVER
			{
			SElement* d = Element(pos);
			if (d->IsEmptyOrDeleted())
				{
				memcpy(d, e, iElementSize);
				d->iHash &= ~EStateMask;
				d->iHash |= iGeneration;	// mark it as processed
				e->SetEmpty();				// remove old entry
				break;
				}
			if ((d->iHash & EStateMask) != iGeneration)
				{
				if (pos == ix)
					{
					e->iHash ^= 1;		// entry is already in correct position so leave it there
					break;
					}
				if ((d->iHash >> newsh) == pos)
					{
					// candidate for replacement is in correct position so leave it and look elsewhere
					d->iHash ^= 1;
					}
				else
					{
					Mem::Swap(d, e, iElementSize);	// switch entries
					d->iHash ^= 1;		// mark entry as processed
					--ix;				// process current position again
					break;
					}
				}
			pos = (pos + step) & newmask;
			}
		}
	iIndexBits = (TUint8)aNewIndexBits;
	iEmptyCount = newmax - iCount;
	SetThresholds();
#ifdef _DEBUG_HASH_TABLE
	VerifyReform();
#endif
	}

#ifdef _DEBUG_HASH_TABLE
void RHashTableBase::VerifyReform()
	{
	TUint32 dcount;
	ConsistencyCheck(&dcount);
	__ASSERT_ALWAYS(dcount==0, __PANIC(EHashTableDeletedEntryAfterReform));
	}
#endif

EXPORT_C void RHashTableBase::ConsistencyCheck(TUint32* aDeleted, TUint32* aComparisons, TUint32 aChainLimit, TUint32* aChainInfo)
	{
#ifdef _DEBUG_HASH_TABLE
	TUint32 count = 0;
	TUint32 dcount = 0;
	TUint32 ecount = 0;
	TUint32 max = 1u << iIndexBits;
	TUint32 mask = max - 1;
	TUint32 sh = 32 - iIndexBits;
	TUint32 ix = 0;
	TUint32 cmp = 0;
	if (aChainInfo)
		memclr(aChainInfo, aChainLimit*sizeof(TUint32));
	if (iElements)
		{
		for (ix = 0; ix < max; ++ix)
			{
			SElement* e = Element(ix);
			if (e->IsEmpty())
				{
				++ecount;
				continue;
				}
			if (e->IsDeleted())
				{
				++dcount;
				continue;
				}
			++count;
			__ASSERT_ALWAYS((e->iHash & EStateMask) == iGeneration, __PANIC(EHashTableBadGeneration));
			TUint32 hash = (*iHashFunc)(GetKey(e));
			hash = (hash &~ EStateMask) | iGeneration;
			__ASSERT_ALWAYS(e->iHash == hash, __PANIC(EHashTableBadHash));

			TUint32 pos = hash >> sh;
			TUint32 step = (hash >> 1) & mask;
			SElement* f = 0;
			TUint32 cl = 0;
			FOREVER
				{
				f = Element(pos);
				if (f->IsEmpty())
					{
					f = 0;
					break;
					}
				++cl;
				if (!f->IsDeleted() && f->iHash==hash)
					{
					++cmp;
					if (e==f || (*iIdFunc)(GetKey(e), GetKey(f)))
						break;
					}
				pos = (pos + step) & mask;
				}
			__ASSERT_ALWAYS(e==f, __PANIC(EHashTableEntryLost));
			if (aChainInfo && cl<aChainLimit)
				++aChainInfo[cl];
			}
		}
	if (aDeleted)
		*aDeleted = dcount;
	if (aComparisons)
		*aComparisons = cmp;
	__ASSERT_ALWAYS(iCount==count, __PANIC(EHashTableCountWrong));
	__ASSERT_ALWAYS(iEmptyCount==ecount, __PANIC(EHashTableEmptyCountWrong));
#else
	if (aDeleted)
		*aDeleted = KMaxTUint;
	if (aComparisons)
		*aComparisons = KMaxTUint;
	if (aChainInfo)
		memclr(aChainInfo, aChainLimit*sizeof(TUint32));
#endif
	}

void RHashTableBase::ShrinkTable()
	{
	ReformTable(iIndexBits - 1);
	TUint32 max = 1u << iIndexBits;
	iElements = User::ReAlloc(iElements, max * iElementSize);
	}

TInt RHashTableBase::ExpandTable(TInt aNewIndexBits)
	{
	TUint32 newmax = 1u << aNewIndexBits;
	if (!iElements)
		{
		iElements = User::AllocZ(newmax * iElementSize);
		if (!iElements)
			return KErrNoMemory;
		iIndexBits = (TUint8)aNewIndexBits;
		iEmptyCount = newmax;
		SetThresholds();
		return KErrNone;
		}
	TUint32 max = 1u << iIndexBits;
	TAny* p = User::ReAlloc(iElements, newmax * iElementSize);
	if (!p)
		return KErrNoMemory;
	iElements = p;
	memclr(Element(max), (newmax-max)*iElementSize);
	ReformTable(aNewIndexBits);
	return KErrNone;
	}

EXPORT_C TInt RHashTableBase::Reserve(TInt aCount)
	{
	__ASSERT_ALWAYS((TUint)aCount<0x40000000u, __PANIC(EHashTableBadReserveCount));
	TInt new_ixb = iIndexBits;
	TUint grow_threshold = iUpperThreshold;
	while (TUint(aCount) > grow_threshold)
		{
		grow_threshold <<= 1;
		++new_ixb;
		}
	// Expand the table if it isn't large enough to fit aCount elements in it
	// or if the table hasn't yet been created, create it with ExpandTable
	if (new_ixb > TInt(iIndexBits) || !iElements)
		{
		return ExpandTable(new_ixb);
		}
	return KErrNone;
	}

EXPORT_C void RHashTableBase::ReserveL(TInt aCount)
	{
	User::LeaveIfError(Reserve(aCount));
	}

EXPORT_C THashTableIterBase::THashTableIterBase(const RHashTableBase& aTable)
	:	iTbl(aTable), iIndex(-1), iPad1(0), iPad2(0)
	{
	}

EXPORT_C void THashTableIterBase::Reset()
	{
	iIndex = -1;
	}

EXPORT_C const TAny* THashTableIterBase::Next(TInt aOffset)
	{
	TInt max = 1 << iTbl.iIndexBits;
	if (!iTbl.iElements)
		return NULL;
	__ASSERT_DEBUG(iIndex>=-1 && iIndex<=max, __PANIC(EHashTableIterNextBadIndex));
	if (iIndex < max)
		++iIndex;
	for(; iIndex < max; ++iIndex)
		{
		const RHashTableBase::SElement* e = iTbl.ElementC(iIndex);
		if (!e->IsEmptyOrDeleted())
			{
			if (aOffset >= 0)
				return (TUint8*)e + aOffset;
			return *(const TAny**)((TUint8*)e - aOffset);
			}
		}
	return NULL;
	}

EXPORT_C const TAny* THashTableIterBase::Current(TInt aOffset) const
	{
	TInt max = 1 << iTbl.iIndexBits;
	if (!iTbl.iElements || iIndex<0 || iIndex>=max)
		return NULL;
	const RHashTableBase::SElement* e = iTbl.ElementC(iIndex);
	__ASSERT_DEBUG(!e->IsEmptyOrDeleted(), __PANIC(EHashTableIterCurrentBadIndex));
	if (aOffset >= 0)
		return (TUint8*)e + aOffset;
	return *(const TAny**)((TUint8*)e - aOffset);
	}

EXPORT_C void THashTableIterBase::RemoveCurrent()
	{
	TInt max = 1 << iTbl.iIndexBits;
	if (!iTbl.iElements || iIndex<0 || iIndex>=max)
		return;
	RHashTableBase& tbl = (RHashTableBase&)iTbl;
	RHashTableBase::SElement* e = tbl.Element(iIndex);
	__ASSERT_DEBUG(!e->IsEmptyOrDeleted(), __PANIC(EHashTableIterCurrentBadIndex));

	// mark entry as deleted but don't shrink the array since that will mess up the iteration
	e->SetDeleted();
	if (--tbl.iCount == 0)
		{
		memclr(tbl.iElements, max * tbl.iElementSize);
		tbl.iEmptyCount = max;
		tbl.iGeneration = RHashTableBase::EGen0;
		}
	}

/**
@publishedAll
@released

Calculate a 32 bit hash from an 8 bit descriptor.

@param	aDes	The descriptor to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C TUint32 DefaultHash::Des8(const TDesC8& aDes)
	{
	return DefaultStringHash(aDes.Ptr(), aDes.Length());
	}


/**
@publishedAll
@released

Calculate a 32 bit hash from a 16 bit descriptor.

@param	aDes	The descriptor to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C TUint32 DefaultHash::Des16(const TDesC16& aDes)
	{
	return DefaultWStringHash(aDes.Ptr(), aDes.Size());
	}


/**
@publishedAll
@released

Calculate a 32 bit hash from a TInt pointer.

@param	aPtr	The TInt pointer to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C TUint32 DefaultHash::IntegerPtr(TInt* const& aPtr)
	{
	return Integer((TInt)aPtr);
	}

/**
@publishedAll
@released

Calculate a 32 bit hash from a TDesC8 pointer.

@param	aPtr	The TDesC8 pointer to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C TUint32 DefaultHash::Des8Ptr(TDesC8* const& aPtr)
	{
	return Integer((TInt)aPtr);
	}

/**
@publishedAll
@released

Calculate a 32 bit hash from a TDesC16 pointer.

@param	aPtr	The TDesC16 pointer to be hashed.
@return			The calculated 32 bit hash value.
*/
EXPORT_C TUint32 DefaultHash::Des16Ptr(TDesC16* const& aPtr)
	{
	return Integer((TInt)aPtr);
	}

/**
@publishedAll
@released

Compare two integers for equality.

@param	aA	The first integer to be compared
@param	aB	The second integer to be compared
@return		ETrue if the arguments are equal, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::Integer(const TInt& aA, const TInt& aB)
	{
	return aA == aB;
	}


/**
@publishedAll
@released

Compare two 8 bit descriptors for exact binary equality.

@param	aA	The first integer to be compared
@param	aB	The second integer to be compared
@return		ETrue if the arguments are identical, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::Des8(const TDesC8& aA, const TDesC8& aB)
	{
	return aA == aB;
	}


/**
@publishedAll
@released

Compare two 16 bit descriptors for exact binary equality.

@param	aA	The first integer to be compared
@param	aB	The second integer to be compared
@return		ETrue if the arguments are identical, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::Des16(const TDesC16& aA, const TDesC16& aB)
	{
	return aA == aB;
	}

/**
@publishedAll
@released

Compare two TInt pointers for equality.

@param	aA	The first pointer to be compared
@param	aB	The second pointer to be compared
@return		ETrue if the arguments are equal, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::IntegerPtr(TInt* const& aA,TInt* const& aB)
	{
	return aA == aB;
	}

/**
@publishedAll
@released

Compare two TDesC8 pointers for equality.

@param	aA	The first pointer to be compared
@param	aB	The second pointer to be compared
@return		ETrue if the arguments are equal, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::Des8Ptr(TDesC8* const& aA,TDesC8* const& aB)
	{
	return aA == aB;
	}

/**
@publishedAll
@released

Compare two TDesC16 pointers for equality.

@param	aA	The first pointer to be compared
@param	aB	The second pointer to be compared
@return		ETrue if the arguments are equal, EFalse otherwise.
*/
EXPORT_C TBool DefaultIdentity::Des16Ptr(TDesC16* const& aA,TDesC16* const& aB)
	{
	return aA == aB;
	}
