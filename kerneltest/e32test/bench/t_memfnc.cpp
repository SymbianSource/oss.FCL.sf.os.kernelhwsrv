// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_memfnc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>

const TInt Len64K = 64*1024;

volatile extern TInt count;
volatile extern TInt iters;
extern TInt8* trg;
extern TInt8* src;
extern TInt8 trgoffset;
extern TInt8 srcoffset;
extern TInt len;

GLREF_C TInt PurgeCache()
	{
    TInt8* purgetrg;
    TInt8* purgesrc;

    purgetrg=(TInt8*)User::Alloc(Len64K);
    purgesrc=(TInt8*)User::Alloc(Len64K);

	Mem::Copy(purgetrg,purgesrc,Len64K);

	User::Free(purgetrg);
	User::Free(purgesrc);
	return 0;
	}

TInt MemBaseline(TAny*)
	{
	FOREVER
		{		
		for (iters=0; iters<8192; )
			{
			// eight off, unrolled for accuracy
			iters++;
			iters++;
			iters++;
			iters++;

			iters++;
			iters++;
			iters++;
			iters++;
			}

		count++;
		}
	}

TInt MemCopy(TAny*)
	{
	FOREVER
		{
		Mem::Copy(trgoffset+trg,srcoffset+src,len);
		count++;
		}
	}

TInt MemFill(TAny*)
	{
	FOREVER
		{
		Mem::Fill(trgoffset+trg,len,42);
		count++;
		}
	}

TInt MemSwap(TAny*)
	{
	FOREVER
		{
		Mem::Swap(trgoffset+trg,srcoffset+src,len);
		count++;
		}
	}

/// For lengths smaller than the allocated size of 64K.  Pointers are stepped
/// though RAM so that we never hit the cache.  Stepping is done backwards to
/// foil any automatic preparation for loading of the next cache line.
TInt MemCopyUncached(TAny*)
	{
	TUint step = (len + 31) & (~31);	// length rounded up to cache line size

	TInt8* slimit = src + 2*Len64K - step + srcoffset;
	TInt8* tlimit = trg + 2*Len64K - step + trgoffset;

	TInt8* s = slimit;
	TInt8* t = tlimit;
	
	FOREVER
		{
		Mem::Copy(t,s,len);
		s -= step;
		t -= step;
		if (s < src)
			{
			s = slimit;
			t = tlimit;
			}
		count++;
		}
	}

// See comments for MemCopyUncached above
TInt MemFillUncached(TAny*)
	{
	TUint step = (len + 31) & (~31);	// length rounded up to cache line size

	TInt8* tlimit = trg + 2*Len64K - step + trgoffset;
	TInt8* t = tlimit;
	
	FOREVER
		{
		Mem::Fill(t,len,42);
		t -= step;
		if (t < trg)
			t = tlimit;
		count++;
		}
	}

TInt WordMove(TAny*)
	{
	FOREVER
		{
		wordmove(trgoffset+trg,srcoffset+src,len);
		count++;
		}
	}

// See comments for MemCopyUncached above
TInt WordMoveUncached(TAny*)
	{
	TUint step = (len + 31) & (~31);	// length rounded up to cache line size

	TInt8* slimit = src + 2*Len64K - step + srcoffset;
	TInt8* tlimit = trg + 2*Len64K - step + trgoffset;

	TInt8* s = slimit;
	TInt8* t = tlimit;
	
	FOREVER
		{
		wordmove(t,s,len);
		s -= step;
		t -= step;
		if (s < src)
			{
			s = slimit;
			t = tlimit;
			}
		count++;
		}
	}
