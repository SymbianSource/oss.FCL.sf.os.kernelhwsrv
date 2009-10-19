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
// f32\sfile\sf_pgcompr.h
// 
//

#ifndef __SF_PGCOMPR_H__
#define __SF_PGCOMPR_H__

#include <e32base.h>
#include<f32file.h>
//#include "sf_std.h"
#include <f32image.h>
#include "sf_image.h"
#include <e32uid.h>
#include <e32rom.h>
#include "sf_cache.h"


const TUint KBytePairPageSize = 4096;

// Buffer sized for at least 8 compressed pages - a noncompressible page is one byte larger
const TUint KReadNumberOfPages = 8;
const TUint KPagesBufferSize = (KBytePairPageSize + 1) * KReadNumberOfPages;

typedef TUint8* (*TMemoryMoveFunction)(TAny* aTrg,const TAny* aSrc,TInt aLength); 

struct IndexTableHeader
	{
	TInt	iSizeOfData;					// Includes the index and compressed pages
	TInt	iDecompressedSize;
	TUint16	iNumberOfPages;
	};

// sizeof(IndexTableHeader) returns the wrong value due to rounding/padding, so calculate it
const TUint KIndexTableHeaderSize = sizeof(TInt) + sizeof(TInt) + sizeof(TUint16);

NONSHARABLE_CLASS(CBytePairReader) : public CBase
	{
public:
	static CBytePairReader* NewLC(TUint8* aBuffer, TUint32 aLength);
	CBytePairReader(TUint8* aBuffer, TUint32 aLength);

	virtual TUint DecompressPagesL(TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMoveFn);
	void GetPageOffsetsL(TInt32 aInitialOffset, TInt& aPageCount, TInt32*& aPageStarts);
	TUint GetPageL(TUint aPageNum, TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMoveFn);		
	virtual void SeekForwardL(TUint aBytes);

protected:
	virtual void ReadInTableL();
	void ReleaseTable();

	IndexTableHeader iHeader;
	TUint16* iIndexTable;
	TUint8* iNextPage;
	TUint iBytesLeft;
	TUint8 iPageBuf[KBytePairPageSize];
	};

NONSHARABLE_CLASS(CBytePairFileReader) : public CBytePairReader
	{
public:
	static CBytePairFileReader* NewLC(RFile& aFile);
	CBytePairFileReader(RFile& aFile);
	~CBytePairFileReader();

	virtual TUint DecompressPagesL(TUint8* aTarget, TInt aLength, TMemoryMoveFunction aMemMovefn);
	virtual void SeekForwardL(TUint aBytes);

protected:
	virtual void ReadInTableL();

	RFile& iFile;
	TUint8 iBuffer[KPagesBufferSize];
	};

#endif // __SF_PGCOMPR_H__
