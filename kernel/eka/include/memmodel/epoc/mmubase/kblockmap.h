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
// e32\include\memmodel\epoc\mmubase\kblockmap.h
// Kernel-side functionality for processing block maps
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KBLOCKMAP_H__
#define __KBLOCKMAP_H__

#include <e32ldr.h>
#include <e32ldr_private.h>

/**
The kernel-side representation of a block map.
*/
class TBlockMap
	{
public:
	TBlockMap();
	~TBlockMap();
	
	/**
	Initialise and populate kernel-side representation from a user-side block map

	@param aBlockMapInfo    	The user-side block map info structure.
	
	@param aBlockMapEntries 	Pointer to a buffer containg the user-side block map entries.
	                            This object takes ownership of the buffer.
	                        
	@param aBlockMapEntriesSize The size of the user-side block map entries in bytes.

	@param aReadUnitShift       Log2 of the paging device's read unit size.
	
	@param aDataLengthInFile 	The length of the (possibly compressed) code in the file.
	*/
	TInt Initialise(const SBlockMapInfoBase& aBlockMapInfo,
					TBlockMapEntryBase* aBlockMapEntries,
                    TInt aBlockMapEntriesSize,
					TInt aReadUnitShift,
					TInt aDataLengthInFile);

	/**
	A function supplied to Read that is called to read the actual data.
	
	@param aArg1   		An argument parameter passed to read.
	@param aArg2   		Another argument parameter passed to read.
	@param aBuffer 		The address of the buffer to read the data into.
	@param aBlockNumber The block number to read.
	@param aBlockCount 	The number of blocks to read.
	*/
	typedef TInt (*TReadFunc)(TAny* aArg1, TAny* aArg2, TLinAddr aBuffer, TInt aBlockNumber, TInt aBlockCount);

	/**
	Read data from the file described by the block map into a buffer.

	@param aBuffer   	  The buffer into which to read the data.
	@param aPos	   	 	  The offset from the start of the data at which to read.
	@param aLength   	  The length of data to read in bytes.
	@param aReadUnitShift Log2 of the paging device's read unit size.
	@param aReadFunc 	  The function to call to read the blocks of data.
	@param aArg1   		  An argument parameter passed to read.
	@param aArg2   		  Another argument parameter passed to read.
	
	@return The offset into the buffer at which the data starts, or one of the system-wide error
	codes.
	*/
	TInt Read(TLinAddr aBuffer, TInt aPos, TInt aLength, TInt aReadUnitShift, TReadFunc aReadFunc, TAny* aArg1, TAny* aArg2) const;

	/**
	A contiguous area of media containing (possibly compressed) code.
	*/
	struct SExtent
		{
		TInt iDataOffset;		// position in file from, counting from start of code data
		TUint iBlockNumber;		// block number containg this position
		};

	inline TInt Count() const;
	inline const SExtent& Extent(TInt aIndex) const;
	inline TInt DataLength() const;

	/**
	Print out the contents of this object for debugging purposes.
	This method is only implemented in debug builds.
	*/
	void Dump() const;

private:
	
	TInt FindFirstExtent(TInt aPos) const;

private:

	TInt iDataLength;
	TInt iExtentCount;
	SExtent* iExtents;
	};

inline TInt TBlockMap::Count() const
	{
	return iExtentCount;
	}

inline const TBlockMap::SExtent& TBlockMap::Extent(TInt aIndex) const
	{
	return iExtents[aIndex];
	}

inline TInt TBlockMap::DataLength() const
	{
	return iDataLength;
	}

#endif
