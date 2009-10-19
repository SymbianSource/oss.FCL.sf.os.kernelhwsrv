// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\random.cpp
// 
//

#include <kernel/kern_priv.h>

//
// Generate nth random number using the following algorithm
//
// X[n] = ((X[n-j] rotl r1) + (X[n-k] rotl r2)) modulo 2^b               
//
// k=17, j=10, r1=5, r2=3
//

const TInt KBufferSize=17;
const TInt KBufferSizeInBits=KBufferSize*32;
const TInt KPointer2Offset=10;  // Must be less than KBufferSize

TUint32 RandomBuffer[KBufferSize];
TInt RandomPointer=1;
TInt RandomSaltPointer=KBufferSizeInBits-1;

inline TUint32 RotateLeft5(TUint32 aVal)
	{ return (aVal << 5) | (aVal >> 27); }

inline TUint32 RotateLeft3(TUint32 aVal)
	{ return (aVal << 3) | (aVal >> 29); }

void K::Randomize()
//
// Initialise the random pool
//
	{
	TUint64 seed=K::TickQ->iRtc;
	TInt i;
	for (i=0; i<KBufferSize; i++)
		{
		RandomBuffer[i] = TUint32(seed);
		seed= ((seed<<5) + (seed>>59)) + 97;
		}
	NKern::LockSystem();
	for (i=0; i<50; i++)
		Kern::Random();
	NKern::UnlockSystem();
	}
	

/**
	Generate the next random number.

	@return The generated random number.

	@pre Kernel Lock must not be held.
	@pre No fast mutex should be held
	@pre Interrupts should be enabled
	@pre Can be used in a device driver.
*/
EXPORT_C TUint32 Kern::Random()
	{
	TBool alreadyLocked = TheScheduler.iLock.HeldByCurrentThread();
	if (!alreadyLocked)
		NKern::LockSystem();
	TInt p1 = RandomPointer;
	if(--p1<0)
		p1 = KBufferSize-1;
	RandomPointer = p1;
	TInt p2 = p1+KPointer2Offset;
	if(p2>KBufferSize-1)
		p2 -= KBufferSize-1;
	TUint32 r = RandomBuffer[p1] = RotateLeft5(RandomBuffer[p2])+RotateLeft3(RandomBuffer[p1]);
	if (!alreadyLocked)
		NKern::UnlockSystem();
	return r;
	}

/**
	Adds a bit to the random pool used to generate random numbers.
	This method should be used by any sources of entropy to improve the quality of
	random number generation.

	@param aBitOfSalt The least significant bit of this value is added to the random pool.

  	@pre Can be used in a device driver.
*/
EXPORT_C void Kern::RandomSalt(TUint32 aBitOfSalt)
	{
	TInt p=RandomSaltPointer; // protect RandomSaltPointer from re-entrantrancy
	if (aBitOfSalt&=1)
		{
		TInt word=p >> 5;
		TInt bit=p & 0x1f;
		RandomBuffer[word] ^= aBitOfSalt << bit;
		}
	if (--p<0)
		p=KBufferSizeInBits-1;
	RandomSaltPointer=p;
	}

