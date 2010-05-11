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
// e32\include\kernel\kbma.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KBMA_H__
#define __KBMA_H__

#include <e32cmn.h>

/** Simple bitmap allocator

@publishedPartner
@released
*/
class TBitMapAllocator
	{
public:
	IMPORT_C static TBitMapAllocator* New(TInt aSize, TBool aState);
	IMPORT_C TBitMapAllocator(TInt aSize, TBool aState);
	inline TInt Avail() const {return iAvail;}
	IMPORT_C TInt Alloc();
	IMPORT_C TInt AllocFrom(TUint aOffset);
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

#endif
