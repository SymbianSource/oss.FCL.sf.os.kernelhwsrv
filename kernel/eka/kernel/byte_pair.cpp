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
// e32\kernel\byte_pair.cpp
// 
//

#include <e32cia.h>
#include "u32std.h"

TInt BytePairDecompress(TUint8* dst, TInt dstSize, TUint8* src, TInt srcSize, TUint8*& srcNext)
	{
	TUint8* dstStart = dst;
	TUint8* dstEnd = dst+dstSize;
	TUint8* srcEnd = src+srcSize;

	TUint32 LUT[0x100/2];
	TUint8* LUT0 = (TUint8*)LUT;
	TUint8* LUT1 = LUT0+0x100;

	TUint8 stack[0x100];
	TUint8* stackStart = stack+sizeof(stack);
	TUint8* sp = stackStart;

	TUint32 marker = ~0u;
	TInt numTokens;
	TUint32 p1;
	TUint32 p2;

	TUint32* l = (TUint32*)LUT;
	TUint32 b = 0x03020100;
	TUint32 step = 0x04040404;
	do
		{
		*l++ = b;
		b += step;
		}
	while(b>step);

	if(src>=srcEnd)
		goto error;
	numTokens = *src++;
	if(numTokens)
		{
		if(src>=srcEnd)
			goto error;
		marker = *src++;
		LUT0[marker] = (TUint8)~marker;

		if(numTokens<32)
			{
			TUint8* tokenEnd = src+3*numTokens;
			if(tokenEnd>srcEnd)
				goto error;
			do
				{
				TInt b = *src++;
				TInt p1 = *src++;
				TInt p2 = *src++;
				LUT0[b] = (TUint8)p1;
				LUT1[b] = (TUint8)p2;
				}
			while(src<tokenEnd);
			}
		else
			{
			TUint8* bitMask = src;
			src += 32;
			if(src>srcEnd)
				goto error;
			TInt b=0;
			do
				{
				TUint8 mask = bitMask[b>>3];
				if(mask&(1<<(b&7)))
					{
					if(src>=srcEnd)
						goto error;
					TInt p1 = *src++;
					if(src>=srcEnd)
						goto error;
					TInt p2 = *src++;
					LUT0[b] = (TUint8)p1;
					LUT1[b] = (TUint8)p2;		
					--numTokens;
					}
				++b;
				}
			while(b<0x100);
			if(numTokens)
				goto error;
			}
		}

	if(src>=srcEnd)
		goto error;
	b = *src++;
	if(dst>=dstEnd)
		goto error;
	p1 = LUT0[b];
	if(p1!=b)
		goto not_single;
next:
	if(src>=srcEnd)
		goto done_s;
	b = *src++;
	*dst++ = (TUint8)p1;
	if(dst>=dstEnd)
		goto done_d;
	p1 = LUT0[b];
	if(p1==b)
		goto next;

not_single:
	if(b==marker)
		goto do_marker;

do_pair:
	p2 = LUT1[b];
	b = p1;
	p1 = LUT0[b];
	if(sp<=stack)
		goto error;
	*--sp = (TUint8)p2;

recurse:
	if(b!=p1)
		goto do_pair;

	if(sp==stackStart)
		goto next;
	b = *sp++;
	*dst++ = (TUint8)p1;
	if(dst>=dstEnd)
		goto error;
	p1 = LUT0[b];
	goto recurse;

do_marker:
	if(src>=srcEnd)
		goto error;
	p1 = *src++;
	goto next;

error:
	srcNext = 0;
	return KErrCorrupt;

done_s:
	*dst++ = (TUint8)p1;
	srcNext = src;
	return dst-dstStart;

done_d:
	--src;
	srcNext = src;
	return dst-dstStart;
	}


