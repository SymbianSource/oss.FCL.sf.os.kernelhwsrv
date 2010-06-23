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
// e32\common\array.cpp
// 
//

#include "common.h"
#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#endif

const TInt KDefaultPtrArrayGranularity		=8;
const TInt KPtrArrayMaxGranularity			=0x10000000;
const TInt KDefaultSimpleArrayGranularity	=8;
const TInt KSimpleArrayMaxGranularity		=0x10000000;
const TInt KSimpleArrayMaxEntrySize			=640;	// allow room for a unicode TFullName
const TInt KMaxArrayGrowBy					=65535;
const TInt KMinArrayFactor					=257;
const TInt KMaxArrayFactor					=32767;

EXPORT_C RPointerArrayBase::RPointerArrayBase()
	: iCount(0), iEntries(NULL), iAllocated(0), iGranularity(KDefaultPtrArrayGranularity), iSpare1(0), iSpare2(0)
	{}

EXPORT_C RPointerArrayBase::RPointerArrayBase(TInt aGranularity)
	: iCount(0), iEntries(NULL), iAllocated(0), iGranularity(aGranularity), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(aGranularity>0 && aGranularity<=KPtrArrayMaxGranularity,
		Panic(EBadArrayGranularity));
	}

EXPORT_C RPointerArrayBase::RPointerArrayBase(TInt aMinGrowBy, TInt aFactor)
	: iCount(0), iEntries(NULL), iAllocated(0), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(aMinGrowBy>0 && aMinGrowBy<=KMaxArrayGrowBy, Panic(EBadArrayMinGrowBy));
	__ASSERT_ALWAYS(aFactor>=KMinArrayFactor && aFactor<=KMaxArrayFactor, Panic(EBadArrayFactor));
	iGranularity = aMinGrowBy | (aFactor << 16) | 0x80000000;
	}

#ifndef __KERNEL_MODE__
EXPORT_C RPointerArrayBase::RPointerArrayBase(TAny** aEntries, TInt aCount)
	: iCount(aCount), iEntries(aEntries), iAllocated(aCount), iGranularity(aCount), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(aCount>0,Panic(EBadArrayCount));
	}
#endif

EXPORT_C void RPointerArrayBase::Close()
	{
	iCount=0;
	STD_CLASS::Free(iEntries);
	iEntries=NULL;
	iAllocated=0;
	}

EXPORT_C TInt RPointerArrayBase::Count() const
	{
	return iCount;
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TAny*& RPointerArrayBase::At(TInt anIndex) const
	{
	__ASSERT_ALWAYS((anIndex>=0 && anIndex<iCount),Panic(EBadArrayIndex));
	return iEntries[anIndex];
	}
#else
GLDEF_C void PanicBadArrayIndex()
	{
	Panic(EBadArrayIndex);
	}
#endif

TInt CalculateArraySizeAfterGrow(TInt aOrigSize, TInt aGranularity, TInt aLimit)
	{
	if (aGranularity >= 0)
		{
		if (aOrigSize > aLimit - aGranularity)
			return KErrNoMemory;	// array can't be >2GB
		return aOrigSize + aGranularity;
		}
	TUint minStep = (TUint)(aGranularity & 65535);
	TUint factor = TUint(aGranularity & 0x7fff0000) >> 16;
	Uint64 na64 = aOrigSize;
	na64 *= (Uint64)factor;
	na64 += 128;
	na64 >>= 8;
	Uint64 min_na64 = (Uint64)aOrigSize + (Uint64)minStep;
	if (min_na64 > na64)
		na64 = min_na64;
	if (na64 > (Uint64)aLimit)
		return KErrNoMemory;	// array can't be >2GB
	return (TInt)na64;
	}

TInt CalculateArraySizeAfterShrink(TInt aOrigSize, TInt aGranularity, TInt aUsed)
	{
	TInt step = aGranularity;
	if (step < 0)
		step &= 65535;
	if (aOrigSize - aUsed < step)
		return aOrigSize;
	aUsed += step - 1;
	aUsed /= step;
	aUsed *= step;
	return aUsed;
	}

TInt RPointerArrayBase::Grow()
	{
	TInt newAlloc = CalculateArraySizeAfterGrow(iAllocated, iGranularity, KMaxTInt >> 2);
	if (newAlloc < 0)
		return newAlloc;
	TAny** pA = (TAny**)STD_CLASS::ReAlloc(iEntries, newAlloc*sizeof(TAny*));
	if (!pA)
		return KErrNoMemory;
	iEntries = pA;
	iAllocated = newAlloc;
	return KErrNone;
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TInt RPointerArrayBase::Append(const TAny* anEntry)
	{
	if (iCount==iAllocated)
		{
		TInt r=Grow();
		if (r!=KErrNone)
			return r;
		}
	iEntries[iCount++]=(TAny*)anEntry;
	return KErrNone;
	}
#endif

EXPORT_C TInt RPointerArrayBase::Insert(const TAny* anEntry, TInt aPos)
	{
	__ASSERT_ALWAYS((aPos>=0 && aPos<=iCount),Panic(EBadArrayPosition));
	if (iCount==iAllocated)
		{
		TInt r=Grow();
		if (r!=KErrNone)
			return r;
		}
	TInt entries=iCount-aPos;
	if (entries!=0)
		wordmove(iEntries+aPos+1,iEntries+aPos,entries*sizeof(TAny*));
	iCount++;
	iEntries[aPos]=(TAny*)anEntry;
	return KErrNone;
	}

EXPORT_C void RPointerArrayBase::Remove(TInt anIndex)
	{
	__ASSERT_ALWAYS((anIndex>=0 && anIndex<iCount),Panic(EBadArrayIndex));
	TInt entries=iCount-anIndex-1;
	if (entries!=0)
		wordmove(iEntries+anIndex,iEntries+anIndex+1,entries*sizeof(TAny*));
	iCount--;
	}

EXPORT_C void RPointerArrayBase::Compress()
	{
	if (iCount)
		iEntries=(TAny**)STD_CLASS::ReAlloc(iEntries,iCount*sizeof(TAny*)); // can't fail
	else
		{
		STD_CLASS::Free(iEntries);
		iEntries=NULL;
		}
	iAllocated=iCount;
	}

#ifndef __KERNEL_MODE__
EXPORT_C void RPointerArrayBase::GranularCompress()
	{
	TInt newAlloc = CalculateArraySizeAfterShrink(iAllocated, iGranularity, iCount);
	if (newAlloc == iAllocated)
		return;
	if (newAlloc)
		iEntries=(TAny**)STD_CLASS::ReAlloc(iEntries,newAlloc*sizeof(TAny*)); // can't fail
	else
		{
		STD_CLASS::Free(iEntries);
		iEntries=NULL;
		}
	iAllocated=newAlloc;
	}

EXPORT_C TInt RPointerArrayBase::DoReserve(TInt aCount)
	{
	__ASSERT_ALWAYS(aCount>=0, Panic(EArrayBadReserveCount));
	if (aCount <= iAllocated)
		return KErrNone;	// if allocated space is already sufficient, nothing to do

	const TInt KLimit = TInt(0x80000000u / sizeof(TAny*));
	if (aCount >= KLimit)
		return KErrNoMemory;

	TAny** pA = (TAny**)STD_CLASS::ReAlloc(iEntries, aCount*sizeof(TAny*));
	if (!pA)
		return KErrNoMemory;
	iEntries = pA;
	iAllocated = aCount;
	return KErrNone;
	}
#endif

EXPORT_C void RPointerArrayBase::Reset()
	{
	iCount=0;
	STD_CLASS::Free(iEntries);
	iEntries=NULL;
	iAllocated=0;
	}

EXPORT_C TInt RPointerArrayBase::BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder) const
	{
	return BinarySearch(anEntry, anIndex, anOrder, EArrayFindMode_Any);
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TInt RPointerArrayBase::Find(const TAny* anEntry) const
	{
	TInt i;
	for (i=0; i<iCount; i++)
		{
		if (iEntries[i]==anEntry)
			return i;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RPointerArrayBase::Find(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const
	{
	TInt i;
	for (i=0; i<iCount; i++)
		{
		if ((*anIdentity)(anEntry,iEntries[i]))
			return i;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RPointerArrayBase::BinarySearchSigned(TInt anEntry, TInt& anIndex) const
	{
	return BinarySearchSigned(anEntry, anIndex, EArrayFindMode_Any);
	}

EXPORT_C TInt RPointerArrayBase::BinarySearchSigned(TInt anEntry, TInt& anIndex, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TInt m=(l+r)>>1;
		TInt h=(TInt)iEntries[m];
		if (anEntry==h)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (anEntry>h)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RPointerArrayBase::BinarySearchUnsigned(TUint anEntry, TInt& anIndex) const
	{
	return BinarySearchUnsigned(anEntry, anIndex, EArrayFindMode_Any);
	}

EXPORT_C TInt RPointerArrayBase::BinarySearchUnsigned(TUint anEntry, TInt& anIndex, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TUint m=(l+r)>>1;
		TUint h=(TUint)iEntries[m];
		if (anEntry==h)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (anEntry>h)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RPointerArrayBase::BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TInt m=(l+r)>>1;
		TInt k=(*anOrder)(anEntry,iEntries[m]);
		if (k==0)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (k>0)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RPointerArrayBase::FindIsqSigned(TInt anEntry) const
	{
	return FindIsqSigned(anEntry, EArrayFindMode_Any);
	}

EXPORT_C TInt RPointerArrayBase::FindIsqUnsigned(TUint anEntry) const
	{
	return FindIsqUnsigned(anEntry, EArrayFindMode_Any);
	}

EXPORT_C TInt RPointerArrayBase::FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder) const
	{
	return FindIsq(anEntry, anOrder, EArrayFindMode_Any);
	}

EXPORT_C TInt RPointerArrayBase::FindIsqSigned(TInt anEntry, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearchSigned(anEntry,i,aMode);
	return (r==KErrNone)?i:r;
	}

EXPORT_C TInt RPointerArrayBase::FindIsqUnsigned(TUint anEntry, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearchUnsigned(anEntry,i,aMode);
	return (r==KErrNone)?i:r;
	}

EXPORT_C TInt RPointerArrayBase::FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearch(anEntry,i,anOrder,aMode);
	return (r==KErrNone)?i:r;
	}
#else
extern "C" void PanicBadArrayFindMode()
	{
	Panic(EBadArrayFindMode);
	}
#endif


EXPORT_C TInt RPointerArrayBase::FindReverse(const TAny* anEntry) const
	{
	TInt i=iCount;
	while (i--)
		{
		if (iEntries[i]==anEntry)
			return i;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RPointerArrayBase::FindReverse(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const
	{
	TInt i=iCount;
	while (i--)
		{
		if ((*anIdentity)(anEntry,iEntries[i]))
			return i;
		}
	return KErrNotFound;
	}


EXPORT_C TInt RPointerArrayBase::InsertIsqSigned(TInt anEntry, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearchSigned(anEntry,i,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

EXPORT_C TInt RPointerArrayBase::InsertIsqUnsigned(TUint anEntry, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearchUnsigned(anEntry,i,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

EXPORT_C TInt RPointerArrayBase::InsertIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearch(anEntry,i,anOrder,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

#ifndef __ARRAY_MACHINE_CODED__
void HeapSortUnsigned(TUint* aEntries,TInt aCount)
	{
	TInt ss = aCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			TUint si;
			if (sh!=0)
				{
				--sh;
				si = aEntries[sh];
				}
			else
				{
				--ss;
				si = aEntries[ss];
				aEntries[ss]=aEntries[0];
				if (ss==1)
					{
					aEntries[0]=si;
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			FOREVER
				{
				jj = (jj+1)<<1;
				if (jj>=ss || TUint(aEntries[jj-1])>TUint(aEntries[jj]) )
					--jj;
				if (jj>=ss || TUint(aEntries[jj])<=si)
					break;
				aEntries[ii]=aEntries[jj];
				ii = jj;
				}
			aEntries[ii]=si;
			}
		}
	}
#endif // !__ARRAY_MACHINE_CODED__


#ifndef __KERNEL_MODE__
#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C void RPointerArrayBase::HeapSortSigned()
	{
	TInt ss = iCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			TInt si;
			if (sh!=0)
				{
				--sh;
				si = (TInt)iEntries[sh];
				}
			else
				{
				--ss;
				si = (TInt)iEntries[ss];
				iEntries[ss]=iEntries[0];
				if (ss==1)
					{
					iEntries[0]=(TAny*)si;
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			FOREVER
				{
				jj = (jj+1)<<1;
				if (jj>=ss || TInt(iEntries[jj-1])>TInt(iEntries[jj]) )
					--jj;
				if (jj>=ss || TInt(iEntries[jj])<=si)
					break;
				iEntries[ii]=iEntries[jj];
				ii = jj;
				}
			iEntries[ii]=(TAny*)si;
			}
		}
	}

EXPORT_C void RPointerArrayBase::HeapSortUnsigned()
	{
	::HeapSortUnsigned((TUint*)iEntries,iCount);
	}

EXPORT_C void RPointerArrayBase::HeapSort(TGeneralLinearOrder anOrder)
	{
	TInt ss = iCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			TAny* si;
			if (sh!=0)
				{
				--sh;
				si = iEntries[sh];
				}
			else
				{
				--ss;
				si = iEntries[ss];
				iEntries[ss]=iEntries[0];
				if (ss==1)
					{
					iEntries[0]=si;
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			FOREVER
				{
				jj = (jj+1)<<1;
				if (jj>=ss || (*anOrder)(iEntries[jj-1],iEntries[jj])>0 )
					--jj;
				if (jj>=ss || (*anOrder)(iEntries[jj],si)<=0 )
					break;
				iEntries[ii]=iEntries[jj];
				ii = jj;
				}
			iEntries[ii]=si;
			}
		}
	}
#endif

EXPORT_C TInt RPointerArrayBase::GetCount(const CBase* aPtr)
	{
	return ((RPointerArrayBase*)aPtr)->Count();
	}

EXPORT_C const TAny* RPointerArrayBase::GetElementPtr(const CBase* aPtr, TInt aIndex)
	{
	return &(((RPointerArrayBase*)aPtr)->At(aIndex));
	}
#endif	// __KERNEL_MODE__

EXPORT_C RArrayBase::RArrayBase(TInt anEntrySize)
	:	iCount(0), iEntries(NULL), iKeyOffset(0), iAllocated(0),
		iGranularity(KDefaultSimpleArrayGranularity), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(anEntrySize>0 && anEntrySize<=KSimpleArrayMaxEntrySize,Panic(EBadArrayEntrySize));
	iEntrySize=(anEntrySize+(TInt)sizeof(TInt)-1)&~((TInt)sizeof(TInt)-1);
	}

EXPORT_C RArrayBase::RArrayBase(TInt anEntrySize, TInt aGranularity)
	:	iCount(0), iEntries(NULL), iKeyOffset(0), iAllocated(0),
		iGranularity(aGranularity), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(anEntrySize>0 && anEntrySize<=KSimpleArrayMaxEntrySize,Panic(EBadArrayEntrySize));
	__ASSERT_ALWAYS(aGranularity>0 && (aGranularity*anEntrySize)<=KSimpleArrayMaxGranularity, Panic(EBadArrayGranularity));
	iEntrySize=(anEntrySize+(TInt)sizeof(TInt)-1)&~((TInt)sizeof(TInt)-1);
	}

EXPORT_C RArrayBase::RArrayBase(TInt aEntrySize,TAny* aEntries, TInt aCount)
	:	iCount(aCount), iEntries(aEntries), iKeyOffset(0), iAllocated(aCount),
		iGranularity(KDefaultSimpleArrayGranularity), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(aEntrySize>0 && aEntrySize<=KSimpleArrayMaxEntrySize,Panic(EBadArrayEntrySize));
	__ASSERT_ALWAYS(aCount>0,Panic(EBadArrayCount));
	iEntrySize=(aEntrySize+(TInt)sizeof(TInt)-1)&~((TInt)sizeof(TInt)-1);
	}

EXPORT_C RArrayBase::RArrayBase(TInt anEntrySize, TInt aGranularity, TInt aKeyOffset)
	:	 iCount(0), iEntries(NULL), iKeyOffset(aKeyOffset), iAllocated(0),
		iGranularity(aGranularity), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(anEntrySize>0 && anEntrySize<=KSimpleArrayMaxEntrySize,Panic(EBadArrayEntrySize));
	__ASSERT_ALWAYS(aGranularity>0 && (aGranularity*anEntrySize)<=KSimpleArrayMaxGranularity, Panic(EBadArrayGranularity));
	__ASSERT_ALWAYS(aKeyOffset>=0 && (aKeyOffset&3)==0 && aKeyOffset<anEntrySize, Panic(EBadArrayKeyOffset));
	iEntrySize=(anEntrySize+(TInt)sizeof(TInt)-1)&~((TInt)sizeof(TInt)-1);
	}

EXPORT_C RArrayBase::RArrayBase(TInt aEntrySize, TInt aMinGrowBy, TInt aKeyOffset, TInt aFactor)
	:	 iCount(0), iEntries(NULL), iKeyOffset(aKeyOffset), iAllocated(0), iSpare1(0), iSpare2(0)
	{
	__ASSERT_ALWAYS(aEntrySize>0 && aEntrySize<=KSimpleArrayMaxEntrySize, Panic(EBadArrayEntrySize));
	__ASSERT_ALWAYS(aKeyOffset>=0 && (aKeyOffset&3)==0 && aKeyOffset<aEntrySize, Panic(EBadArrayKeyOffset));
	__ASSERT_ALWAYS(aMinGrowBy>0 && aMinGrowBy<=KMaxArrayGrowBy, Panic(EBadArrayMinGrowBy));
	__ASSERT_ALWAYS(aFactor>=KMinArrayFactor && aFactor<=KMaxArrayFactor, Panic(EBadArrayFactor));
	iEntrySize=(aEntrySize+(TInt)sizeof(TInt)-1)&~((TInt)sizeof(TInt)-1);
	iGranularity = aMinGrowBy | (aFactor << 16) | 0x80000000;
	}

EXPORT_C void RArrayBase::Close()
	{
	iCount=0;
	STD_CLASS::Free(iEntries);
	iEntries=NULL;
	iAllocated=0;
	}

EXPORT_C TInt RArrayBase::Count() const
	{
	return iCount;
	}

EXPORT_C void RArrayBase::SetKeyOffset(TInt aKeyOffset)
	{
	__ASSERT_ALWAYS(TUint(aKeyOffset)<TUint(iEntrySize) && (aKeyOffset&3)==0, Panic(EBadArrayKeyOffset));
	iKeyOffset = aKeyOffset;
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TAny* RArrayBase::At(TInt anIndex) const
	{
	__ASSERT_ALWAYS((anIndex>=0 && anIndex<iCount),Panic(EBadArrayIndex));
	return (((TUint8*)iEntries)+anIndex*iEntrySize);
	}
#endif

TInt RArrayBase::Grow()
	{
	TInt newAlloc = CalculateArraySizeAfterGrow(iAllocated, iGranularity, KMaxTInt/iEntrySize);
	if (newAlloc < 0)
		return newAlloc;
	TAny** pA = (TAny**)STD_CLASS::ReAlloc(iEntries, newAlloc*iEntrySize);
	if (!pA)
		return KErrNoMemory;
	iEntries = pA;
	iAllocated = newAlloc;
	return KErrNone;
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TInt RArrayBase::Append(const TAny* anEntry)
	{
	if (iCount==iAllocated)
		{
		TInt r=Grow();
		if (r!=KErrNone)
			return r;
		}
	wordmove((TUint8*)iEntries+iCount*iEntrySize, anEntry, iEntrySize);
	iCount++;
	return KErrNone;
	}
#endif

EXPORT_C TInt RArrayBase::Insert(const TAny* anEntry, TInt aPos)
	{
	__ASSERT_ALWAYS((aPos>=0 && aPos<=iCount),Panic(EBadArrayPosition));
	if (iCount==iAllocated)
		{
		TInt r=Grow();
		if (r!=KErrNone)
			return r;
		}
	TUint8 *pS=(TUint8*)iEntries+aPos*iEntrySize;
	TUint8 *pD=pS+iEntrySize;
	TInt entries=iCount-aPos;
	if (entries!=0)
		wordmove(pD,pS,entries*iEntrySize);
	wordmove(pS,anEntry,iEntrySize);
	iCount++;
	return KErrNone;
	}

EXPORT_C void RArrayBase::Remove(TInt anIndex)
	{
	__ASSERT_ALWAYS((anIndex>=0 && anIndex<iCount),Panic(EBadArrayIndex));
	TUint8 *pD=(TUint8*)iEntries+anIndex*iEntrySize;
	TUint8 *pS=pD+iEntrySize;
	TInt entries=iCount-anIndex-1;
	if (entries!=0)
		wordmove(pD,pS,entries*iEntrySize);
	iCount--;
	}

EXPORT_C void RArrayBase::Compress()
	{
	if (iCount)
		iEntries=STD_CLASS::ReAlloc(iEntries,iCount*iEntrySize); // can't fail
	else
		{
		STD_CLASS::Free(iEntries);
		iEntries=NULL;
		}
	iAllocated=iCount;
	}

#ifndef __KERNEL_MODE__
EXPORT_C void RArrayBase::GranularCompress()
	{
	TInt newAlloc = CalculateArraySizeAfterShrink(iAllocated, iGranularity, iCount);
	if (newAlloc == iAllocated)
		return;
	if (newAlloc)
		iEntries=STD_CLASS::ReAlloc(iEntries,newAlloc*iEntrySize); // can't fail
	else
		{
		STD_CLASS::Free(iEntries);
		iEntries=NULL;
		}
	iAllocated=newAlloc;
	}

EXPORT_C TInt RArrayBase::DoReserve(TInt aCount)
	{
	__ASSERT_ALWAYS(aCount>=0, Panic(EArrayBadReserveCount));
	if (aCount <= iAllocated)
		return KErrNone;	// if allocated space is already sufficient, nothing to do

	Int64 size = Int64(aCount) * Int64(iEntrySize);
	if (size > Int64(KMaxTInt))
		return KErrNoMemory;

	TAny** pA = (TAny**)STD_CLASS::ReAlloc(iEntries, aCount*iEntrySize);
	if (!pA)
		return KErrNoMemory;
	iEntries = pA;
	iAllocated = aCount;
	return KErrNone;
	}
#endif

EXPORT_C void RArrayBase::Reset()
	{
	iCount=0;
	STD_CLASS::Free(iEntries);
	iEntries=NULL;
	iAllocated=0;
	}

EXPORT_C TInt RArrayBase::BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder) const
	{
	return BinarySearch(anEntry, anIndex, anOrder, EArrayFindMode_Any);
	}

#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C TInt RArrayBase::Find(const TAny* anEntry) const
	{
	TUint8 *pS=(TUint8*)iEntries+iKeyOffset;
	TInt match=*(TInt*)((TUint8*)anEntry+iKeyOffset);
	TInt i;
	for (i=0; i<iCount; i++)
		{
		TInt key=*(TInt*)pS;
		if (key==match)
			return i;
		pS+=iEntrySize;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RArrayBase::Find(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const
	{
	TUint8 *pS=(TUint8*)iEntries;
	TInt i;
	for (i=0; i<iCount; i++)
		{
		if ((*anIdentity)(anEntry,pS))
			return i;
		pS+=iEntrySize;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RArrayBase::BinarySearchSigned(const TAny* anEntry, TInt& anIndex) const
	{
	return BinarySearchSigned(anEntry, anIndex, EArrayFindMode_Any);
	}

EXPORT_C TInt RArrayBase::BinarySearchSigned(const TAny* anEntry, TInt& anIndex, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TInt match=*(TInt*)((TUint8*)anEntry+iKeyOffset);
	TUint8* pK=(TUint8*)iEntries+iKeyOffset;
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TInt m=(l+r)>>1;
		TInt h=*(TInt*)(pK+m*iEntrySize);
		if (match==h)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (match>h)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RArrayBase::BinarySearchUnsigned(const TAny* anEntry, TInt& anIndex) const
	{
	return BinarySearchUnsigned(anEntry, anIndex, EArrayFindMode_Any);
	}

EXPORT_C TInt RArrayBase::BinarySearchUnsigned(const TAny* anEntry, TInt& anIndex, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TUint match=*(TUint*)((TUint8*)anEntry+iKeyOffset);
	TUint8* pK=(TUint8*)iEntries+iKeyOffset;
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TInt m=(l+r)>>1;
		TUint h=*(TUint*)(pK+m*iEntrySize);
		if (match==h)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (match>h)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RArrayBase::BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder, TInt aMode) const
	{
	__ASSERT_DEBUG(TUint(aMode)<TUint(EArrayFindMode_Limit), Panic(EBadArrayFindMode));
	TInt l=0;
	TInt r=iCount;
	TInt ret = KErrNotFound;
	while(r>l)
		{
		TInt m=(l+r)>>1;
		TInt k=(*anOrder)(anEntry,(TUint8*)iEntries+m*iEntrySize);
		if (k==0)
			{
			if (aMode == EArrayFindMode_Any)
				{
				anIndex=m;
				return KErrNone;
				}
			ret = KErrNone;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (k>0)
			l=m+1;
		else
			r=m;
		}
	anIndex=r;
	return ret;
	}

EXPORT_C TInt RArrayBase::FindIsqSigned(const TAny* anEntry) const
	{
	return FindIsqSigned(anEntry, EArrayFindMode_Any);
	}

EXPORT_C TInt RArrayBase::FindIsqUnsigned(const TAny* anEntry) const
	{
	return FindIsqUnsigned(anEntry, EArrayFindMode_Any);
	}

EXPORT_C TInt RArrayBase::FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder) const
	{
	return FindIsq(anEntry, anOrder, EArrayFindMode_Any);
	}

EXPORT_C TInt RArrayBase::FindIsqSigned(const TAny* anEntry, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearchSigned(anEntry,i,aMode);
	return (r==KErrNone)?i:r;
	}

EXPORT_C TInt RArrayBase::FindIsqUnsigned(const TAny* anEntry, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearchUnsigned(anEntry,i,aMode);
	return (r==KErrNone)?i:r;
	}

EXPORT_C TInt RArrayBase::FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TInt aMode) const
	{
	TInt i;
	TInt r=BinarySearch(anEntry,i,anOrder,aMode);
	return (r==KErrNone)?i:r;
	}
#endif

EXPORT_C TInt RArrayBase::FindReverse(const TAny* anEntry) const
	{
	TUint8 *pS=(TUint8*)iEntries+(iCount-1)*iEntrySize+iKeyOffset;
	TInt match=*(TInt*)((TUint8*)anEntry+iKeyOffset);
	TInt i=iCount;
	while(i--)
		{
		TInt key=*(TInt*)pS;
		if (key==match)
			return i;
		pS-=iEntrySize;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RArrayBase::FindReverse(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const
	{
	TUint8 *pS=(TUint8*)iEntries+(iCount-1)*iEntrySize;
	TInt i=iCount;
	while (i--)
		{
		if ((*anIdentity)(anEntry,pS))
			return i;
		pS-=iEntrySize;
		}
	return KErrNotFound;
	}

EXPORT_C TInt RArrayBase::InsertIsqSigned(const TAny* anEntry, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearchSigned(anEntry,i,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

EXPORT_C TInt RArrayBase::InsertIsqUnsigned(const TAny* anEntry, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearchUnsigned(anEntry,i,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

EXPORT_C TInt RArrayBase::InsertIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TBool aAllowRepeats)
	{
	TInt i;
	TInt mode = aAllowRepeats ? EArrayFindMode_Last : EArrayFindMode_Any;
	TInt r=BinarySearch(anEntry,i,anOrder,mode);
	if (r==KErrNotFound || aAllowRepeats)
		return Insert((const TAny*)anEntry,i);
	return KErrAlreadyExists;
	}

#ifndef __KERNEL_MODE__
#ifndef __ARRAY_MACHINE_CODED__
EXPORT_C void RArrayBase::HeapSortSigned()
	{
	TUint32 si[KSimpleArrayMaxEntrySize/4];
	TInt ss = iCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			if (sh!=0)
				{
				--sh;
				wordmove(si,(TUint8*)iEntries+sh*iEntrySize,iEntrySize);
				}
			else
				{
				--ss;
				wordmove(si,(TUint8*)iEntries+ss*iEntrySize,iEntrySize);
				wordmove((TUint8*)iEntries+ss*iEntrySize,iEntries,iEntrySize);
				if (ss==1)
					{
					wordmove(iEntries,si,iEntrySize);
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			TInt sikey=*(TInt*)((TUint8*)si+iKeyOffset);
			FOREVER
				{
				jj = (jj+1)<<1;
				TUint8* pKey=((TUint8*)iEntries+jj*iEntrySize+iKeyOffset);
				if (jj>=ss || (*(TInt*)(pKey-iEntrySize))>*(TInt*)pKey )
					{
					--jj;
					pKey-=iEntrySize;
					}
				if (jj>=ss || *(TInt*)pKey<=sikey)
					break;
				wordmove((TUint8*)iEntries+ii*iEntrySize,(TUint8*)iEntries+jj*iEntrySize,iEntrySize);
				ii = jj;
				}
			wordmove((TUint8*)iEntries+ii*iEntrySize,si,iEntrySize);
			}
		}
	}

EXPORT_C void RArrayBase::HeapSortUnsigned()
	{
	TUint32 si[KSimpleArrayMaxEntrySize/4];
	TInt ss = iCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			if (sh!=0)
				{
				--sh;
				wordmove(si,(TUint8*)iEntries+sh*iEntrySize,iEntrySize);
				}
			else
				{
				--ss;
				wordmove(si,(TUint8*)iEntries+ss*iEntrySize,iEntrySize);
				wordmove((TUint8*)iEntries+ss*iEntrySize,iEntries,iEntrySize);
				if (ss==1)
					{
					wordmove(iEntries,si,iEntrySize);
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			TUint sikey=*(TUint*)((TUint8*)si+iKeyOffset);
			FOREVER
				{
				jj = (jj+1)<<1;
				TUint8* pKey=((TUint8*)iEntries+jj*iEntrySize+iKeyOffset);
				if (jj>=ss || (*(TUint*)(pKey-iEntrySize))>*(TUint*)pKey )
					{
					--jj;
					pKey-=iEntrySize;
					}
				if (jj>=ss || *(TUint*)pKey<=sikey)
					break;
				wordmove((TUint8*)iEntries+ii*iEntrySize,(TUint8*)iEntries+jj*iEntrySize,iEntrySize);
				ii = jj;
				}
			wordmove((TUint8*)iEntries+ii*iEntrySize,si,iEntrySize);
			}
		}
	}

EXPORT_C void RArrayBase::HeapSort(TGeneralLinearOrder anOrder)
	{
	TUint32 si[KSimpleArrayMaxEntrySize/4];
	TInt ss = iCount;
	if (ss>1)
		{
		TInt sh = ss>>1;
		FOREVER
			{
			if (sh!=0)
				{
				--sh;
				wordmove(si,(TUint8*)iEntries+sh*iEntrySize,iEntrySize);
				}
			else
				{
				--ss;
				wordmove(si,(TUint8*)iEntries+ss*iEntrySize,iEntrySize);
				wordmove((TUint8*)iEntries+ss*iEntrySize,iEntries,iEntrySize);
				if (ss==1)
					{
					wordmove(iEntries,si,iEntrySize);
					break;
					}
				}
			TInt ii = sh;
			TInt jj = sh;
			FOREVER
				{
				jj = (jj+1)<<1;
				TUint8* pJJ=((TUint8*)iEntries+jj*iEntrySize);
				if (jj>=ss || (*anOrder)(pJJ-iEntrySize,pJJ)>0)
					{
					--jj;
					pJJ-=iEntrySize;
					}
				if (jj>=ss || (*anOrder)(pJJ,si)<=0)
					break;
				wordmove((TUint8*)iEntries+ii*iEntrySize,(TUint8*)iEntries+jj*iEntrySize,iEntrySize);
				ii = jj;
				}
			wordmove((TUint8*)iEntries+ii*iEntrySize,si,iEntrySize);
			}
		}
	}
#endif

EXPORT_C TInt RArrayBase::GetCount(const CBase* aPtr)
	{
	return ((RArrayBase*)aPtr)->Count();
	}

EXPORT_C const TAny* RArrayBase::GetElementPtr(const CBase* aPtr, TInt aIndex)
	{
	return ((RArrayBase*)aPtr)->At(aIndex);
	}
#endif	// __KERNEL_MODE__

