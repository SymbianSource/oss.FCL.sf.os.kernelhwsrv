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
// e32test\nkernsa\testutils.cpp
// 
//

#include <nktest/nkutils.h>

extern "C" {
TUint32 random(TUint32* aSeed)
	{
	TUint32 x = aSeed[0];
	TUint32 r3 = x >> 1;
	r3 |= (aSeed[1] << 31);
	aSeed[1] = x & 1;
	r3 ^= (x << 12);
	x = r3 ^ (r3 >> 20);
	aSeed[0] = x;
	return x;
	}

void setup_block(TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	TInt i;
	for (i=1; i<aNumWords; ++i)
		*++aBlock = random(seed);
	}

void setup_block_cpu(TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	aBlock[1] = NKern::CurrentCpu();
	TInt i;
	for (i=2; i<aNumWords; ++i)
		aBlock[i] = random(seed) ^ NKern::CurrentCpu();
	}

TBool verify_block(const TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	TInt i;
	for (i=1; i<aNumWords; ++i)
		{
		TUint32 x = random(seed);
		if (aBlock[i] != x)
			{
			KPrintf("Verify block failed: expected %08x got %08x", x, aBlock[i]);
			return FALSE;
			}
		}
	return TRUE;
	}

TBool verify_block_no_trace(const TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	TInt i;
	for (i=1; i<aNumWords; ++i)
		{
		TUint32 x = random(seed);
		if (aBlock[i] != x)
			return FALSE;
		}
	return TRUE;
	}

TInt verify_block_cpu(const TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	TUint32 cpu = aBlock[1];
	TInt i;
	for (i=2; i<aNumWords; ++i)
		{
		TUint32 x = random(seed);
		TUint32 y = aBlock[i] ^ x;
		if (y != cpu)
			{
			KPrintf("Verify block with CPU failed: expected %08x got %08x (XOR %08x)", x, aBlock[i], y);
			return -1;
			}
		}
	return cpu;
	}

TInt verify_block_cpu_no_trace(const TUint32* aBlock, TInt aNumWords)
	{
	TUint32 seed[2];
	seed[0] = aBlock[0];
	seed[1] = 0;
	TUint32 cpu = aBlock[1];
	TInt i;
	for (i=2; i<aNumWords; ++i)
		{
		TUint32 x = random(seed);
		TUint32 y = aBlock[i] ^ x;
		if (y != cpu)
			return -1;
		}
	return cpu;
	}
}


CircBuf* CircBuf::New(TInt aSlots)
	{
	CircBuf* p = new CircBuf();
	p->iSlotCount = aSlots;
	p->iGetIndex = 0;
	p->iPutIndex = 0;
	p->iBufBase = (TUint32*)malloc(aSlots*sizeof(TUint32));
	if (!p->iBufBase)
		{
		delete p;
		p = 0;
		}
	return p;
	}

CircBuf::CircBuf()
	:	iLock(TSpinLock::EOrderGenericIrqLow1)
	{
	}

CircBuf::~CircBuf()
	{
	free(iBufBase);
	}

TInt CircBuf::TryGet(TUint32& aOut)
	{
	TInt r = KErrUnderflow;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	if (iGetIndex != iPutIndex)
		{
		aOut = iBufBase[iGetIndex++];
		if (iGetIndex == iSlotCount)
			iGetIndex = 0;
		r = KErrNone;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return r;
	}

TInt CircBuf::TryPut(TUint32 aIn)
	{
	TInt r = KErrOverflow;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TInt nextPut = iPutIndex + 1;
	if (nextPut == iSlotCount)
		nextPut = 0;
	if (iGetIndex != nextPut)
		{
		iBufBase[iPutIndex] = aIn;
		iPutIndex = nextPut;
		r = KErrNone;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return r;
	}

TUint32 CircBuf::Get()
	{
	TUint32 x;
	while(TryGet(x)!=KErrNone)
		NKern::Sleep(1);
	return x;
	}

void CircBuf::Put(TUint32 aIn)
	{
	while(TryPut(aIn)!=KErrNone)
		NKern::Sleep(1);
	}

void CircBuf::Reset()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	iPutIndex = iGetIndex = 0;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	}

TInt CircBuf::Count()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TInt r = iPutIndex - iGetIndex;
	if (r < 0)
		r += iSlotCount;
	__SPIN_UNLOCK_IRQRESTORE(iLock,irq);
	return r;
	}
