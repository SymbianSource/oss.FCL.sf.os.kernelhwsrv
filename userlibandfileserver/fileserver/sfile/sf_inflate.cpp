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
// f32\sfile\sf_inflate.h
// 
//

#include "sf_deflate.h"
#include "sf_ldr.h"

// Class RInflater
//
// The inflation algorithm, complete with huffman decoding

inline CInflater::CInflater(TBitInput& aInput)
	:iBits(&aInput),iEncoding(0),iOut(0)
	{}

void CInflater::ConstructL()
	{
	iEncoding=new(ELeave) TEncoding;
	InitL();
	iLen=0;
	iOut=new(ELeave) TUint8[KDeflateMaxDistance];
	iAvail=iLimit=iOut;
	}

CInflater* CInflater::NewLC(TBitInput& aInput)
	{
	CInflater* self=new(ELeave) CInflater(aInput);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CInflater::~CInflater()
	{
	delete iEncoding;
	delete [] iOut;
	}

TInt CInflater::ReadL(TUint8* aBuffer,TInt aLength, TMemoryMoveFunction aMemMovefn)
	{
	TInt tfr=0;
	for (;;)
		{
		TInt len=Min(aLength,iLimit-iAvail);
		if (len && aBuffer)
			{
			aMemMovefn(aBuffer,iAvail,len);
			aBuffer+=len;
			}
		aLength-=len;
		iAvail+=len;
		tfr+=len;
		if (aLength==0)
			return tfr;
		len=InflateL();
		if (len==0)
			return tfr;
		iAvail=iOut;
		iLimit=iAvail+len;
		}
	}

TInt CInflater::SkipL(TInt aLength)
	{
	return ReadL(0,aLength,Mem::Move);
	}

void CInflater::InitL()
	{
// read the encoding
	Huffman::InternalizeL(*iBits,iEncoding->iLitLen,KDeflationCodes);
// validate the encoding
	if (!Huffman::IsValid(iEncoding->iLitLen,TEncoding::ELitLens) ||
		!Huffman::IsValid(iEncoding->iDistance,TEncoding::EDistances))
		LEAVE_FAILURE(KErrCorrupt);
// convert the length tables into huffman decoding trees
	Huffman::Decoding(iEncoding->iLitLen,TEncoding::ELitLens,iEncoding->iLitLen);
	Huffman::Decoding(iEncoding->iDistance,TEncoding::EDistances,iEncoding->iDistance,KDeflateDistCodeBase);
	}

TInt CInflater::InflateL()
//
// consume all data lag in the history buffer, then decode to fill up the output buffer
// return the number of available bytes in the output buffer. This is only ever less than
// the buffer size if the end of stream marker has been read
//
	{
// empty the history buffer into the output
	TUint8* out=iOut;
	TUint8* const end=out+KDeflateMaxDistance;
	const TUint32* tree=iEncoding->iLitLen;
	if (iLen<0)	// EOF
		return 0;
	if (iLen>0)
		goto useHistory;
//
	while (out<end)
		{
		// get a huffman code
		{
		TInt val=iBits->HuffmanL(tree)-TEncoding::ELiterals;
		if (val<0)
			{
			*out++=TUint8(val);
			continue;			// another literal/length combo
			}
		if (val==TEncoding::EEos-TEncoding::ELiterals)
			{	// eos marker. we're done
			iLen=-1;
			break;
			}
		// get the extra bits for the code
		TInt code=val&0xff;
		if (code>=8)
			{	// xtra bits
			TInt xtra=(code>>2)-1;
			code-=xtra<<2;
			code<<=xtra;
			code|=iBits->ReadL(xtra);
			}
		if (val<KDeflateDistCodeBase-TEncoding::ELiterals)
			{
			// length code... get the code
			if(TUint(code)>TUint(KDeflateMaxLength-KDeflateMinLength))
				{
				CHECK_FAILURE(KErrCorrupt);
				goto error;
				}
			iLen=code+KDeflateMinLength;
			tree=iEncoding->iDistance;
			continue;			// read the huffman code
			}
		// distance code
		if(TUint(code)>TUint(KDeflateMaxDistance-1))
			{
			CHECK_FAILURE(KErrCorrupt);
			goto error;
			}
		iRptr=out-(code+1);
		if (iRptr+KDeflateMaxDistance<end)
			iRptr+=KDeflateMaxDistance;
		if(!iLen)
			{
			CHECK_FAILURE(KErrCorrupt);
			goto error;
			}
		}
useHistory:
		{
		TInt tfr=Min(end-out,iLen);
		iLen-=tfr;
		const TUint8* from=iRptr;
		do
			{
			*out++=*from++;
			if (from==end)
				from-=KDeflateMaxDistance;
			} while (--tfr!=0);
		iRptr=from;
		tree=iEncoding->iLitLen;
		}

		};
	return out-iOut;

error:
	LEAVE_FAILURE(KErrCorrupt);
	return 0;
	}

