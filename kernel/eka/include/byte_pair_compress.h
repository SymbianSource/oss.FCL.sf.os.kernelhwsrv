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
// e32\include\byte_pair_compress.h
// This header file contains both the prototype and implementation for the byte pair compressor.
// This is done to facilitate sharing the code between the e32 test code and the tools.  The
// implementation is only included if the macro BYTE_PAIR_COMPRESS_INCLUDE_IMPLEMENTATION is
// defined.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __BYTE_PAIR_COMPRESS_H__
#define __BYTE_PAIR_COMPRESS_H__

#include <e32std.h>

/**
 * @internalTechnology
 *
 * Compress up to 4096 bytes of data usign the byte-pair compression algorithm.
 *
 * @param aDest Destination buffer, must be at least four times the size of the source data.
 * @param aSrc  Source data to compress.
 * @param aSize The size of the source data.
 */
extern TInt BytePairCompress(TUint8* aDest, TUint8* aSrc, TInt aSize);

#ifdef BYTE_PAIR_COMPRESS_INCLUDE_IMPLEMENTATION

const TInt BlockSize = 0x1000;
TInt ExtraSave = 0;

TUint16 PairCount[0x10000];
TUint16 PairBuffer[BlockSize*2];

TUint16 GlobalPairs[0x10000] = {0};
TUint16 GlobalTokenCounts[0x100] = {0};

TUint16 ByteCount[0x100+4];

void CountBytes(TUint8* data, TInt size)
	{
	memset(ByteCount,0,sizeof(ByteCount));
	TUint8* dataEnd = data+size;
	while(data<dataEnd)
		++ByteCount[*data++];
	}

inline void ByteUsed(TInt b)
	{
	ByteCount[b] = 0xffff;
	}

int TieBreak(int b1,int b2)
	{
	return -ByteCount[b1]-ByteCount[b2];
	}

TInt MostCommonPair(TInt& pair, TUint8* data, TInt size, TInt minFrequency, TInt marker)
	{
	memset(PairCount,0,sizeof(PairCount));
	TUint8* dataEnd = data+size-1;
	TInt pairsFound = 0;
	TInt lastPair = -1;
	while(data<dataEnd)
		{
		TInt b1 = *data++;
		if(b1==marker)
			{
			// skip marker and following byte
			lastPair = -1;
			++data;
			continue;
			}
		TInt b2 = *data;
		if(b2==marker)
			{
			// skip marker and following byte
			lastPair = -1;
			data+=2;
			continue;
			}
		TInt p = (b2<<8)|b1;
		if(p==lastPair)
			{
			// ensure a pair of identical bytes don't get double counted
			lastPair = -1;
			continue;
			}
		lastPair = p;
		++PairCount[p];
		if(PairCount[p]==minFrequency)
			PairBuffer[pairsFound++] = (TUint16)p;
		}

	TInt bestCount = -1;
	TInt bestPair = -1;
	TInt bestTieBreak = 0;
	TInt p;
	while(pairsFound--)
		{
		p = PairBuffer[pairsFound];
		TInt f=PairCount[p];
		if(f>bestCount)
			{
			bestCount = f;
			bestPair = p;
			bestTieBreak = TieBreak(p&0xff,p>>8);
			}
		else if(f==bestCount)
			{
			TInt tieBreak = TieBreak(p&0xff,p>>8);
			if(tieBreak>bestTieBreak)
				{
				bestCount = f;
				bestPair = p;
				bestTieBreak = tieBreak;
				}
			}
		}
	pair = bestPair;
	return bestCount;
	}

TInt LeastCommonByte(TInt& byte)
	{
	TInt bestCount = 0xffff;
	TInt bestByte = -1;
	for(TInt b = 0; b<0x100; b++)
		{
		TInt f = ByteCount[b];
		if(f<bestCount)
			{
			bestCount = f;
			bestByte = b;
			}
		}
	byte = bestByte;
	return bestCount;
	}

TInt BytePairCompress(TUint8* dst, TUint8* src, TInt size)
	{
	TInt originalSize = size;
	TUint8* dst2 = dst+size*2;
	TUint8* in = src;
	TUint8* out = dst;

	TUint8 tokens[0x100*3];
	TInt tokenCount = 0;

	CountBytes(in,size);

	TInt marker = -1;
	TInt overhead = 1+3+LeastCommonByte(marker);
	ByteUsed(marker);

	TUint8* inEnd = in+size;
	TUint8* outStart = out;
	while(in<inEnd)
		{
		TInt b=*in++;
		if(b==marker)
			*out++ = (TUint8)b;
		*out++ = (TUint8)b;
		}
	size = out-outStart;

	TInt outToggle = 1;
	in = dst;
	out = dst2;

	for(TInt r=256; r>0; --r)
		{
		TInt byte;
		TInt byteCount = LeastCommonByte(byte);
		TInt pair;
		TInt pairCount = MostCommonPair(pair,in,size,overhead+1,marker);
		TInt saving = pairCount-byteCount;
		if(saving<=overhead)
			break;

		overhead = 3;
		if(tokenCount>=32)
			overhead = 2;

		TUint8* d=tokens+3*tokenCount;
		++tokenCount;
		*d++ = (TUint8)byte;
		ByteUsed(byte);
		*d++ = (TUint8)pair;
		ByteUsed(pair&0xff);
		*d++ = (TUint8)(pair>>8);
		ByteUsed(pair>>8);
		++GlobalPairs[pair];

		inEnd = in+size;
		outStart = out;
		while(in<inEnd)
			{
			TInt b=*in++;
			if(b==marker)
				{
				*out++ = (TUint8)marker;
				b = *in++;
				}
			else if(b==byte)
				{
				*out++ = (TUint8)marker;
				--byteCount;
				}
			else if(b==(pair&0xff) && in<inEnd && *in==(pair>>8))
				{
				++in;
				b = byte;
				--pairCount;
				}
			*out++ = (TUint8)b;
			}
		ASSERT(!byteCount);
		ASSERT(!pairCount);
		size = out-outStart;

		outToggle ^= 1;
		if(outToggle)
			{
			in = dst;
			out = dst2;
			}
		else
			{
			in = dst2;
			out = dst;
			}
		}

	// sort tokens with a bubble sort...
	for(TInt x=0; x<tokenCount-1; x++)
		for(TInt y=x+1; y<tokenCount; y++)
			if(tokens[x*3]>tokens[y*3])
				{
				TInt z = tokens[x*3];
				tokens[x*3] = tokens[y*3];
				tokens[y*3] = (TUint8)z;
				z = tokens[x*3+1];
				tokens[x*3+1] = tokens[y*3+1];
				tokens[y*3+1] = (TUint8)z;
				z = tokens[x*3+2];
				tokens[x*3+2] = tokens[y*3+2];
				tokens[y*3+2] = (TUint8)z;
				}

	// check for not being able to compress...
	if(size>originalSize)
		{
		*dst++ = 0; // store zero token count
		memcpy(dst,src,originalSize); // store original data
		return originalSize+1;
		}

	// make sure data is in second buffer (dst2)
	if(in!=dst2)
		memcpy(dst2,dst,size);

	// store tokens...
	TUint8* originalDst = dst;
	*dst++ = (TUint8)tokenCount;
	if(tokenCount)
		{
		*dst++ = (TUint8)marker;
		if(tokenCount<32)
			{
			memcpy(dst,tokens,tokenCount*3);
			dst += tokenCount*3;
			}
		else
			{
			TUint8* bitMask = dst;
			memset(bitMask,0,32);
			dst += 32;
			TUint8* d=tokens;
			do
				{
				TInt t=*d++;
				bitMask[t>>3] |= (1<<(t&7));
				*dst++ = *d++;
				*dst++ = *d++;
				}
			while(--tokenCount);
			}
		}
	// store data...
	memcpy(dst,dst2,size);
	dst += size;

	// get stats...
	++GlobalTokenCounts[tokenCount];

	// return total size of compressed data...
	return dst-originalDst;
	}

#endif
#endif
