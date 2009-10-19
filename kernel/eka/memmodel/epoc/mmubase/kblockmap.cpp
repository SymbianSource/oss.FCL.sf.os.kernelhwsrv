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
// e32\memmodel\epoc\mmubase\kblockmap.cpp
// 
//

#include <memmodel/epoc/mmubase/kblockmap.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32atomics.h>

#undef ASSERT

#ifdef __KERNEL_MODE__

#include <kernel/kernel.h>
#define TRACECLASS Kern
#undef TRACE
#define ASSERT(x) __NK_ASSERT_DEBUG(x)
#define FREE(x) Kern::Free(x)

#define RETURN_ERROR(err, traceMsg)													 \
	{																				 \
	__KTRACE_OPT(KPAGING,Kern::Printf("DP: %s, returning error %d", traceMsg, err)); \
	return err;																		 \
	}

#define TRACE_FATAL(x) __KTRACE_OPT(KPANIC, Kern::x)

#else

#include <e32debug.h>
#include <e32test.h>
#define TRACECLASS RDebug
extern RTest test;
#define ASSERT(x) test(x);
#define FREE(x) User::Free(x)

#define RETURN_ERROR(err, traceMsg)								  \
	{															  \
	RDebug::Printf("DP: %s, returning error %d", traceMsg, err); \
	return err;													  \
	}

#define TRACE_FATAL(x) RDebug::x

#endif

TInt TBlockMap::Initialise(const SBlockMapInfoBase& aBlockMapInfo,
						   TBlockMapEntryBase* aBlockMapEntries,
						   TInt aBlockMapEntriesSize,
						   TInt aReadUnitShift,
						   TInt aDataLengthInFile)
	{
	ASSERT(iExtents == NULL);

	if (aBlockMapEntriesSize % sizeof(TBlockMapEntryBase) != 0)
		RETURN_ERROR(KErrArgument, "Size of block map is not multiple of entry size");
	iExtentCount = aBlockMapEntriesSize / sizeof(TBlockMapEntryBase);

	TInt blockShift = __e32_find_ms1_32(aBlockMapInfo.iBlockGranularity);
	if ((1u << blockShift) != aBlockMapInfo.iBlockGranularity)
		RETURN_ERROR(KErrArgument, "Block granularity not a power of two");
	if (blockShift < aReadUnitShift)
		RETURN_ERROR(KErrArgument, "Block size must be greater than or equal to read unit size");
	TInt blockScaling = blockShift - aReadUnitShift;
	TInt readUnitMask = (1 << aReadUnitShift) - 1;

	if ((aBlockMapInfo.iStartBlockAddress & ((1 << aReadUnitShift) - 1)) != 0)
		RETURN_ERROR(KErrArgument, "Block zero address not a multiple of read unit size");
	TUint blockZeroNumber = (TUint)(aBlockMapInfo.iStartBlockAddress >> aReadUnitShift);  // offset of block zero from start of partition

	if (aBlockMapInfo.iBlockStartOffset >= aBlockMapInfo.iBlockGranularity)
		RETURN_ERROR(KErrArgument, "Block start offset must be less than block size");

	if (aDataLengthInFile <= 0)
		RETURN_ERROR(KErrArgument, "Length of code data in file must be greater than zero"); 
	iDataLength = aDataLengthInFile;
	
	// Process block map data into kernel-side reprsentation
	TInt dataOffset = -(TInt)(aBlockMapInfo.iBlockStartOffset & readUnitMask);
	SExtent entry;
	for (TInt i = 0 ; i < iExtentCount ; ++i)
		{
		const TBlockMapEntryBase& data = aBlockMapEntries[i];
		entry.iDataOffset = dataOffset;
		entry.iBlockNumber = (data.iStartBlock << blockScaling) + blockZeroNumber;
		dataOffset += data.iNumberOfBlocks << blockShift;
		if (i == 0)
			{
			TInt adjustStartBlock = aBlockMapInfo.iBlockStartOffset >> aReadUnitShift;
			entry.iBlockNumber += adjustStartBlock;
			dataOffset -= adjustStartBlock << aReadUnitShift;
			}
		(SExtent&)data = entry;
		}
	
	if (dataOffset < iDataLength)
		RETURN_ERROR(KErrArgument, "Block map too short");

	// Take ownership of buffer
	iExtents = (SExtent*) aBlockMapEntries;

	return KErrNone;
	}

TBlockMap::TBlockMap()
	: iExtents(NULL)
	{
	}

TBlockMap::~TBlockMap()
	{
	FREE(iExtents);
	}

TInt TBlockMap::FindFirstExtent(TInt aPos) const
	{
	if (aPos < 0 || aPos >= iDataLength)
		return KErrArgument;
	RArray<SExtent> extents(sizeof(SExtent), iExtents, iExtentCount);
	SExtent findEntry = { aPos, 0 };
	TInt i = -1;
	extents.SpecificFindInSignedKeyOrder(findEntry, i, EArrayFindMode_Last);
	--i;
	if (i < 0 || i >= iExtentCount)
		return KErrArgument;
	return i;
	}

TInt TBlockMap::Read(TLinAddr aBuffer, TInt aPos, TInt aLength, TInt aReadUnitShift, TReadFunc aReadFunc, TAny* aArg1, TAny* aArg2) const
	{
	TInt dataOffset = aPos;
	TInt remain = aLength;
	TInt readUnitMask = (1 << aReadUnitShift) - 1;
	
	TInt i = FindFirstExtent(dataOffset);
	if (i < 0)
		return i;
	TInt bufferStart = (dataOffset - Extent(i).iDataOffset) & readUnitMask;  // start of page in buffer
	TInt bufferOffset = 0;
	while (remain > 0)
		{
 		if (i >= Count())
			return KErrArgument;
		
		const SExtent& extent = Extent(i);
		TInt blockStartOffset = dataOffset - extent.iDataOffset;
		TInt blockNumber = extent.iBlockNumber + (blockStartOffset >> aReadUnitShift);
		TInt nextExtentStart = i == Count() - 1 ? iDataLength : Extent(i + 1).iDataOffset;
		TInt dataSize = Min(remain, nextExtentStart - dataOffset);
		TInt blockCount = (dataSize + (blockStartOffset & readUnitMask) + readUnitMask) >> aReadUnitShift;
		
		TInt r = aReadFunc(aArg1, aArg2, aBuffer + bufferOffset, blockNumber, blockCount);
		if (r != KErrNone)
			{
			TRACE_FATAL(Printf("TBlockMap::Read: error reading media at %08x + %x: %d", blockNumber << aReadUnitShift, blockCount << aReadUnitShift, r));
			return r;
			}

		bufferOffset += blockCount << aReadUnitShift;
		dataOffset += dataSize;
		remain -= dataSize;
		++i;
		}
	
	return bufferStart;
	}

#ifdef _DEBUG

void TBlockMap::Dump() const
	{
	TRACECLASS::Printf("TBlockMap:");
	for (TInt i = 0 ; i < Count() ; ++i)
		{
		TInt nextExtentStart = i == Count() - 1 ? iDataLength : Extent(i + 1).iDataOffset;
		TRACECLASS::Printf("  %d: %08x -> %08x: %08x", i, Extent(i).iDataOffset, nextExtentStart, Extent(i).iBlockNumber);
		}
	}

#endif
