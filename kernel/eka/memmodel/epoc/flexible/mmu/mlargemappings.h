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
//   Support for mapping memory with section mappings and large pages.
//
//   This adds the following new classes:
//   
//     DLargeMappedMemory - a subclass of DCoarseMemory that holds information about which areas of
//                          memory are contiguous, and contains the page tables to use to map the
//                          memory.
//
//     DLargeMapping	  - a subclass of DCoarseMapping used to map areas of a DLargeMappedMemory
//     						object.
//
//   todo: currently only section mappings are supported.
//

/**
 @file
 @internalComponent
*/

#ifndef MLAGEMAPPINGS_H
#define MLAGEMAPPINGS_H

#include "mobject.h"
#include "mmapping.h"

// todo: Think of a better name than DLargeMappedMemory for a coarse memory object that supports
// large mappings

/**
A coarse memory object that supports mappings larger than 4K pages.

All the contraints of coarse memory objects also apply to large memory objects.

Fine memory mappings (DFineMapping) may also be attached to this memory object but these will be
mapped using 4K pages and won't benefit from page table sharing.
*/
class DLargeMappedMemory : public DCoarseMemory
	{
public:
	/**
	Create a new DLargeMappedMemory object.

	@param aManager		The manager object for this memory.
	@param aSizeInPages Size of the memory object, in number of pages.  (Must represent an exact
						'chunk' size.)
	@param aAttributes	Bitmask of values from enum #TMemoryAttributes.
	@param aCreateFlags	Bitmask of option flags from enum #TMemoryCreateFlags.

	@return The newly created DLargeMemory or the null pointer if there was insufficient memory.
	*/
	static DLargeMappedMemory* New(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	
private:
	DLargeMappedMemory(DMemoryManager* aManager, TUint aSizeInPages, TMemoryAttributes aAttributes, TMemoryCreateFlags aCreateFlags);
	
public:
	// from DMemoryObject...
	virtual ~DLargeMappedMemory();
	virtual TInt ClaimInitialPages(TLinAddr aBase, TUint aSize, TMappingPermissions aPermissions, TBool aAllowGaps, TBool aAllowNonRamPages);
	virtual TInt MapPages(RPageArray::TIter aPages);
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TBool aInvalidateTLB);
	virtual void UnmapPages(RPageArray::TIter aPages, TBool aDecommitting);
	virtual void RestrictPages(RPageArray::TIter aPages, TRestrictPagesType aRestriction);
	virtual DMemoryMapping* CreateMapping(TUint aIndex, TUint aCount);
	
public:
	TBool IsChunkContiguous(TInt aChunkIndex);
private:
	void SetChunkContiguous(TInt aChunkIndex, TBool aIsContiguous);

	TUint32 iContiguousState[1];
	};


/**
A memory mapping to map a 'chunk' aligned region of a DLargeMappedMemory object into an address
space, which allows use of mappings larger than a single page.

Currently this only supports section mapping, and only then when the memory is initially mapped in
sections by the bootstrap.
*/
class DLargeMapping: public DCoarseMapping
	{
public:
	DLargeMapping();
	
private:
	// from DMemoryMappingBase...
	virtual TInt DoMap();
	virtual void RemapPage(TPhysAddr& aPageArray, TUint aIndex, TUint aMapInstanceCount, TBool aInvalidateTLB);
	virtual TInt PageIn(RPageArray::TIter aPages, TPinArgs& aPinArgs, TUint aMapInstanceCount);
	virtual TBool MovingPageIn(TPhysAddr& aPageArrayPtr, TUint aIndex);
	
	// from DMemoryMapping...
	virtual TPte* FindPageTable(TLinAddr aLinAddr, TUint aMemoryIndex);
	};


#endif
