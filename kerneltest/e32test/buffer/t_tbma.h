// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_tbma.h
// 
//

#ifndef __T_TBMA_H__
#define __T_TBMA_H__

#include <e32test.h>
#include <e32atomics.h>

#define TBMA_FAULT()	TBmaFault(__LINE__)
#define __ALLOC(x)		User::Alloc(x)

extern void TBmaFault(TInt aLine);

extern RTest test;

// Copied from klib.h
class TBitMapAllocator
	{
public:
	IMPORT_C static TBitMapAllocator* New(TInt aSize, TBool aState);
	IMPORT_C TBitMapAllocator(TInt aSize, TBool aState);
	inline TInt Avail() const {return iAvail;}
	IMPORT_C TInt Alloc();
	IMPORT_C void Free(TInt aPos);
	IMPORT_C void Alloc(TInt aStart, TInt aLength);
	IMPORT_C void Free(TInt aStart, TInt aLength);
	IMPORT_C TUint SelectiveAlloc(TInt aStart, TInt aLength);
	IMPORT_C void SelectiveFree(TInt aStart, TInt aLength);
	IMPORT_C TBool NotFree(TInt aStart, TInt aLength) const;
	IMPORT_C TBool NotAllocated(TInt aStart, TInt aLength) const;
	IMPORT_C TInt AllocList(TInt aLength, TInt* aList);
	IMPORT_C TInt AllocConsecutive(TInt aLength, TBool aBestFit) const;
	IMPORT_C TInt AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit) const;
	IMPORT_C TInt AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength) const;
	IMPORT_C TInt AllocAligned(TInt aLength, TInt aAlign, TInt aBase, TBool aBestFit, TInt& aCarry, TInt& aRunLength, TUint aOffset) const;
	IMPORT_C void CopyAlignedRange(const TBitMapAllocator* aA, TInt aFirst, TInt aLen);
public:
	TInt iAvail;			/**< @internalComponent */
	TUint32* iCheckFirst;	/**< @internalComponent */
	TInt iSize;				/**< @internalComponent */
	TUint32 iMap[1];		/**< @internalComponent */	// extend
	};

class TBmaList
	{
public:
	static TBmaList* New(TInt aNumBmas);
	static TBmaList* New(const TBitMapAllocator& aBma, TInt aNumSplits, VA_LIST aList);
	TBmaList();
	~TBmaList();
	TInt AllocConsecutiveFF(TInt aLength);
	TInt AllocConsecutiveBF(TInt aLength);
	TInt AllocAlignedFF(TInt aLength, TInt aAlign);
	TInt AllocAlignedBF(TInt aLength, TInt aAlign);
public:
	TInt iNumBmas;
	TBitMapAllocator** iBmaList;
	TInt* iBaseList;
	};

struct SRange
	{
	TInt iBase;
	TInt iLength;
	};

#endif
