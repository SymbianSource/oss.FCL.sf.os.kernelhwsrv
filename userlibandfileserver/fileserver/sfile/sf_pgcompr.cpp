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
// f32\sfile\sf_pgcompr.cpp
// 
//

#include <f32file.h>
#include "sf_std.h"
#include "sf_ldr.h"
#include <f32image.h>
#include "sf_image.h"
#include <e32uid.h>
#include <e32rom.h>
#include "sf_cache.h"

#include "sf_pgcompr.h"

extern TInt BytePairDecompress(TUint8* /*dst*/, TInt /*dstSize*/, TUint8* /*src*/, TInt /*srcSize*/, TUint8*& /*srcNext*/);


// CBytePairReader - reading from in-memory buffer


CBytePairReader* CBytePairReader::NewLC(TUint8* aBuffer, TUint32 aLength)
	{
	CBytePairReader* reader = new (ELeave) CBytePairReader(aBuffer, aLength);
	CleanupStack::PushL(reader);
	return reader;
	}


CBytePairReader::CBytePairReader(TUint8* aBuffer, TUint32 aLength)
	: iNextPage(aBuffer), iBytesLeft(aLength)
	{
	}


void CBytePairReader::SeekForwardL(TUint aBytes)
	{
	if (iBytesLeft < aBytes)
		LEAVE_FAILURE(KErrCorrupt);
	iNextPage += aBytes;
	iBytesLeft -= aBytes;
	}


void CBytePairReader::ReadInTableL()
	{
	if (KIndexTableHeaderSize > iBytesLeft)
		LEAVE_FAILURE(KErrCorrupt);
	Mem::Copy(&iHeader, iNextPage, KIndexTableHeaderSize);
	iNextPage += KIndexTableHeaderSize;
	iBytesLeft -= KIndexTableHeaderSize;
	
	__IF_DEBUG(Printf("numberOfPages:%d", iHeader.iNumberOfPages));
	
	TUint size = iHeader.iNumberOfPages * sizeof(TUint16);
	if (size > iBytesLeft)
		LEAVE_FAILURE(KErrCorrupt);
	// coverity[buffer_alloc]
	iIndexTable = new (ELeave) TUint16[size/sizeof(TUint16)];
	Mem::Copy(iIndexTable, iNextPage, size);
	iNextPage += size;
	iBytesLeft -= size;
	} 


void CBytePairReader::ReleaseTable()
	{
	delete[] iIndexTable;
	iIndexTable = NULL;
	}


TUint CBytePairReader::GetPageL(TUint aPageNum, TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMoveFn)
	{
	if (iIndexTable[aPageNum] > iBytesLeft)
		LEAVE_FAILURE(KErrCorrupt);
	iBytesLeft -= iIndexTable[aPageNum];
	aLength = Min(aLength, KBytePairPageSize);

	TInt size;
	TUint8* nextPage;

	if (aMemMoveFn)
		size = BytePairDecompress(iPageBuf, aLength, iNextPage, iIndexTable[aPageNum], nextPage);
	else
		size = BytePairDecompress(aTarget, aLength, iNextPage, iIndexTable[aPageNum], nextPage);
		
	User::LeaveIfError(size);
	if (size != aLength)
		LEAVE_FAILURE(KErrCorrupt);
	if (iNextPage + iIndexTable[aPageNum] != nextPage)
		LEAVE_FAILURE(KErrCorrupt);

	// If a memmove() was provided, use that to copy the data to its final target
	if (aMemMoveFn)
		aMemMoveFn(aTarget, iPageBuf, size);

	iNextPage = nextPage;
	return size;
	}


TUint CBytePairReader::DecompressPagesL(TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMoveFn)
	{
	TUint decompressedSize = 0;

	ReadInTableL();
	
	for (TUint curPage = 0; curPage < iHeader.iNumberOfPages; ++curPage)
		{
		TUint size = GetPageL(curPage, aTarget, aLength, aMemMoveFn);
		
		decompressedSize += size;
		aTarget += size;
		aLength -= size;
		
		__IF_DEBUG(Printf("decomp page size:%d\n", size ));
		} 
	
	__IF_DEBUG(Printf("decompressedSize:%d", decompressedSize));

	ReleaseTable();

	return decompressedSize;
	}


void CBytePairReader::GetPageOffsetsL(TInt32 aInitialOffset, TInt& aPageCount, TInt32*& aPageOffsets)
	{
	ReadInTableL();
	aPageCount = iHeader.iNumberOfPages;
	aPageOffsets = new (ELeave) TInt32[aPageCount+1];

	TInt bytes = aInitialOffset + KIndexTableHeaderSize + aPageCount * sizeof(TUint16);
	for (TInt i = 0; i < aPageCount; ++i)
		{
		aPageOffsets[i] = bytes;
		bytes += iIndexTable[i];
		}
	aPageOffsets[aPageCount] = bytes;

	ReleaseTable();
	}


// CBytePairFileReader - reading from file


CBytePairFileReader* CBytePairFileReader::NewLC(RFile& aFile)
	{
	CBytePairFileReader* reader = new (ELeave) CBytePairFileReader(aFile);
	CleanupStack::PushL(reader);
	return reader;
	}


CBytePairFileReader::CBytePairFileReader(RFile& aFile)
	: CBytePairReader(NULL, 0), iFile(aFile)
	{
	}	


CBytePairFileReader::~CBytePairFileReader()
	{
	ReleaseTable();
	}


void CBytePairFileReader::SeekForwardL(TUint aBytes)
	{
	TInt bytes = aBytes;
	User::LeaveIfError(iFile.Seek(ESeekCurrent, bytes));
	}
	

void CBytePairFileReader::ReadInTableL()
	{
	TPtr8 header((TUint8*)&iHeader, KIndexTableHeaderSize);
	User::LeaveIfError(iFile.Read(header, KIndexTableHeaderSize));
	if (header.Length() != (TInt)KIndexTableHeaderSize)
		LEAVE_FAILURE(KErrCorrupt);
	
	__IF_DEBUG(Printf("numberOfPages:%d", iHeader.iNumberOfPages));
	
	TInt size = iHeader.iNumberOfPages * sizeof(TUint16);
	iIndexTable = new (ELeave) TUint16[size/sizeof(TUint16)];
	TPtr8 indexTable((TUint8*)iIndexTable, size);
	User::LeaveIfError(iFile.Read(indexTable, size));
	if (indexTable.Length() != size)
		LEAVE_FAILURE(KErrCorrupt);
	} 


TUint CBytePairFileReader::DecompressPagesL(TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMoveFn)
	{
	TUint decompressedSize = 0;

	ReadInTableL();
	
	TUint curPage = 0;
	while (curPage < iHeader.iNumberOfPages)
		{
		TUint bytes = 0;
		TUint pages = 0;
		while (curPage + pages < iHeader.iNumberOfPages &&
			   bytes + iIndexTable[curPage+pages] < sizeof(iBuffer))
			{
			bytes += iIndexTable[curPage+pages];
			++pages;
			}
		if(!bytes)
			LEAVE_FAILURE(KErrCorrupt);
		TPtr8 data(iBuffer, bytes);
		User::LeaveIfError(iFile.Read(data, bytes));
		if (data.Length() != (TInt)bytes)
			LEAVE_FAILURE(KErrCorrupt);
		iNextPage = iBuffer;
		iBytesLeft = bytes;

		for (; pages; ++curPage, --pages)
			{
			TUint size = GetPageL(curPage, aTarget, aLength, aMemMoveFn);
		
			decompressedSize += size;
			aTarget += size;
			aLength -= size;
		
			__IF_DEBUG(Printf("decomp page size:%d\n", size ));
			}
		} 
	
	__IF_DEBUG(Printf("decompressedSize:%d", decompressedSize));

	ReleaseTable();

	return decompressedSize;
	}
