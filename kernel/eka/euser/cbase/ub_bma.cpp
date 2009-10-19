// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\cbase\ub_bma.cpp
// 
//

#include "ub_std.h"

const TInt KBitsPerInt=32;
const TInt KBitsPerIntMask=(KBitsPerInt-1);
const TInt KBitsPerIntShift=5;


inline TInt FindLeastSignificantZero(register TUint n)
	{
	register TInt i=0;
	n=~n;
	if (n<<16==0) n>>=16, i+=16;
	if (n<<24==0) n>>=8, i+=8;
	if (n<<28==0) n>>=4, i+=4;
	if (n<<30==0) n>>=2, i+=2;
	if (n<<31==0) n>>=1, i+=1;
	return i;
	}

inline TInt FindLeastSignificantZero(register TUint n, TUint aFrom)
	{
	n |= ((1<<aFrom)-1);
	return FindLeastSignificantZero(n);
	}

inline TInt FindLeastSignificantOne(register TUint n)
	{
	register TInt i=0;
	if (n<<16==0) n>>=16, i+=16;
	if (n<<24==0) n>>=8, i+=8;
	if (n<<28==0) n>>=4, i+=4;
	if (n<<30==0) n>>=2, i+=2;
	if (n<<31==0) n>>=1, i+=1;
	return i;
	}

inline TInt FindMostSignificantZero(register TUint n)
	{
	register TInt i=31;
	n=~n;
	if (n<0x00010000) n<<=16, i-=16;
	if (n<0x01000000) n<<=8, i-=8;
	if (n<0x10000000) n<<=4, i-=4;
	if (n<0x40000000) n<<=2, i-=2;
	if (n<0x80000000) n<<=1, i-=1;
	return i;
	}

EXPORT_C CBitMapAllocator::CBitMapAllocator(TInt aSize,TInt aLength)
//
// Constructor
//
	: iAvail(aSize),iSize(aSize),iLength(aLength)
	{
	TInt rem=aSize&KBitsPerIntMask;
	if (rem)
		{
		TInt last=(aSize-1)>>KBitsPerIntShift;
		iMap[last]=0xFFFFFFFFu<<rem;
		}
	}

EXPORT_C CBitMapAllocator::~CBitMapAllocator()
//
// Destructor
//
	{
	}

EXPORT_C CBitMapAllocator *CBitMapAllocator::New(TInt aSize)
//
// Create a new bit map allocator.
//
	{
	__ASSERT_ALWAYS(aSize>0,Panic(EBmaSizeLessOrEqualToZero));
	TInt sz=((aSize+KBitsPerIntMask)>>KBitsPerIntShift)-1;
	return(new(sz*sizeof(TUint)) CBitMapAllocator(aSize,sz+1));
	}

EXPORT_C CBitMapAllocator *CBitMapAllocator::NewL(TInt aSize)
//
// Create a new bit map allocator. Leave on any error.
//
	{
	CBitMapAllocator *pA=New(aSize);
	User::LeaveIfNull(pA);
	return(pA);
	}

EXPORT_C TInt CBitMapAllocator::Alloc()
//
// Allocate the next position.
//
	{
	if (iAvail)
		{
		TUint *pS=iMap;
		TUint *pE=pS+iLength;
		do	{
			register TUint n=*pS++;
			if (n!=0xFFFFFFFFu)
				{
				iAvail--;
				TInt bit=FindLeastSignificantZero(n);
				*--pS=n|(1<<bit);
				return((TInt(pS-iMap)<<KBitsPerIntShift)+bit);
				}
			} while(pS<pE);
		Panic(EBmaInconsistentState);
		}
	return(KErrNoMemory);
	}

EXPORT_C TInt CBitMapAllocator::AllocFrom(TInt aPos)
//
// Allocate the next position after aPos.
//
	{
	__ASSERT_ALWAYS((aPos>=0 && aPos<iSize),Panic(EBmaAllocFromOutOfRange));
	if (iAvail)
		{
		TUint *pS=iMap+(aPos>>KBitsPerIntShift);
		TUint *pE=iMap+iLength;
		TInt start=aPos&KBitsPerIntMask;
		register TUint n;
		if (start)
			{
			n=*pS++ | ~(0xFFFFFFFFu<<start);
			if (n!=0xFFFFFFFFu)
				goto found;
			}
		while(pS<pE)
			{
			n=*pS++;
			if (n!=0xFFFFFFFFu)
				{
found:
				iAvail--;
				TInt bit=FindLeastSignificantZero(n);
				*--pS |= (1<<bit);
				return((TInt(pS-iMap)<<KBitsPerIntShift)+bit);
				}
			}
		}
	return(KErrNoMemory);
	}

EXPORT_C TInt CBitMapAllocator::AllocFromTop()
//
// Allocate the next position.
//
	{
	if (iAvail)
		{
		TUint *pS=iMap;
		TUint *pE=pS+iLength;
		do	{
			register TUint n=*--pE;
			if (n!=0xFFFFFFFFu)
				{
				iAvail--;
				TInt bit=FindMostSignificantZero(n);
				*pE=n|(1<<bit);
				return((TInt(pE-pS)<<KBitsPerIntShift)+bit);
				}
			} while(pE>pS);
		Panic(EBmaInconsistentState);
		}
	return(KErrNoMemory);
	}

EXPORT_C TInt CBitMapAllocator::AllocFromTopFrom(TInt aPos)
//
// Allocate the next position after aPos.
//
	{
	__ASSERT_ALWAYS((aPos>=0 && aPos<iSize),Panic(EBmaAllocFromTopFromOutOfRange));
	if (iAvail)
		{
		TUint *pS=iMap;
		TUint *pE=pS+((aPos+1)>>KBitsPerIntShift);
		TInt start=(aPos+1)&KBitsPerIntMask;
		register TUint n;
		if (start)
			{
			n=*pE | (0xFFFFFFFFu<<start);
			if (n!=0xFFFFFFFFu)
				goto found;
			}
		while(pE>pS)
			{
			n=*--pE;
			if (n!=0xFFFFFFFFu)
				{
found:
				iAvail--;
				TInt bit=FindMostSignificantZero(n);
				*pE|=(1<<bit);
				return((TInt(pE-pS)<<KBitsPerIntShift)+bit);
				}
			}
		}
	return(KErrNoMemory);
	}

EXPORT_C TInt CBitMapAllocator::Alloc(TInt aCount, TInt& aConsecutive)
	{
	__ASSERT_ALWAYS((aCount>0),Panic(EBmaAllocCountNegative));
	TInt initPos;
	if (iAvail)
		{
		TUint *pS=iMap;
		TUint *pE=pS+iLength;
		register TUint n;
		do	{
			n=*pS++;
			if (n!=0xFFFFFFFFu)
				goto found;
			} while(pS<pE);
		Panic(EBmaInconsistentState);
found:
		register TInt c;
		pS--;
		TInt bit=FindLeastSignificantZero(n);
		initPos=(TInt(pS-iMap)<<KBitsPerIntShift)+bit;
		n>>=bit;
		if (n)
			{
			c=FindLeastSignificantOne(n);
			if (aCount<c) c=aCount;
			*pS |= ~(0xFFFFFFFFu<<c)<<bit;
			iAvail-=c;
			aConsecutive=c;
			return initPos;
			}
		c=KBitsPerInt-bit;
		if (c>=aCount)
			{
			c=aCount;
			if (c<KBitsPerInt)
				*pS |= ~(0xFFFFFFFFu<<c)<<bit;
			else
				*pS |= 0xFFFFFFFFu<<bit;
			iAvail-=c;
			aConsecutive=c;
			return initPos;
			}
		c=aCount-c;
		*pS=0xFFFFFFFFu;
		while(++pS<pE && (n=*pS)==0 && c>=KBitsPerInt)
			*pS=0xFFFFFFFFu, c-=KBitsPerInt;
		if (c && pS<pE && n!=0xFFFFFFFFu)
			{
			bit=n?FindLeastSignificantOne(n):KBitsPerInt;
			if (bit>c) bit=c;
			*pS |= ~(0xFFFFFFFFu<<bit);
			c-=bit;
			}
		aConsecutive=aCount-c;
		iAvail-=aConsecutive;
		return initPos;
		}
	aConsecutive=0;
	return KErrNoMemory;
	}

LOCAL_D const TUint AlignedSearchMask[] =
	{0x00000000,0xAAAAAAAA,0xEEEEEEEE,0xFEFEFEFE,0xFFFEFFFE};

EXPORT_C TInt CBitMapAllocator::AllocAligned(TInt anAlignment)
	{
	__ASSERT_ALWAYS((anAlignment>=0 && anAlignment<32),Panic(EBmaAllAlgnOutOfRange));
	if (iAvail==0)
		return KErrNoMemory;
	TUint mask;
	TInt step;
	if (anAlignment>=KBitsPerIntShift)
		{
		mask=0xFFFFFFFEu;
		step=1<<(anAlignment-KBitsPerIntShift);
		}
	else
		{
		mask=AlignedSearchMask[anAlignment];
		step=1;
		}
	TUint *pM=iMap;
	TUint *pE=pM+iLength;
	do	{
		register TUint n=*pM | mask;
		if (n!=0xFFFFFFFFu)
			{
			iAvail--;
			TInt bit=(mask==0xFFFFFFFEu)?0:FindLeastSignificantZero(n);
			*pM |= (1<<bit);
			return((TInt(pM-iMap)<<KBitsPerIntShift)+bit);
			}
		pM+=step;
		} while(pM<pE);
	return KErrNoMemory;
	}

EXPORT_C TInt CBitMapAllocator::AllocAlignedBlock(TInt anAlignment)
	{
	__ASSERT_ALWAYS((anAlignment>=0 && anAlignment<32),Panic(EBmaAllAlgnBOutOfRange));
	if (iAvail==0)
		return KErrNoMemory;
	TInt blocksz=1<<anAlignment;
	TUint mask;
	TUint block;
	TInt step;
	if (anAlignment>=KBitsPerIntShift)
		{
		mask=0xFFFFFFFEu;
		step=1<<(anAlignment-KBitsPerIntShift);
		block=0xFFFFFFFFu;
		}
	else
		{
		mask=AlignedSearchMask[anAlignment];
		step=1;
		block=~(0xFFFFFFFFu<<blocksz);
		}
	TUint *pM=iMap;
	TUint *pE=pM+iLength;
	do	{
		register TUint n=*pM | mask;
		if (n!=0xFFFFFFFFu)
			{
			if (blocksz>=KBitsPerInt)
				{
				n=0;
				TUint *pS=pM+step;
				if (pS<=pE)
					{
					do n|=*pM++; while(pM<pS);
					pM-=step;
					if (n==0)
						{
						iAvail-=blocksz;
						do *pM++=0xFFFFFFFFu; while(pM<pS);
						pM-=step;
						return (TInt(pM-iMap)<<KBitsPerIntShift);
						}
					}
				}
			else
				{
				TInt bit=FindLeastSignificantZero(n);
				mask=block<<bit;
				n=*pM;
				do	{
					if ((n&mask)==0)
						{
						*pM |= mask;
						iAvail-=blocksz;
						return((TInt(pM-iMap)<<KBitsPerIntShift)+bit);
						}
					bit+=blocksz;
					mask<<=blocksz;
					} while(mask);
				}
			}
		pM+=step;
		} while(pM<pE);
	return KErrNoMemory;
	}

EXPORT_C void CBitMapAllocator::AllocAt(TInt aPos)
//
// Allocate a required position.
//
	{
	__ASSERT_ALWAYS(aPos>=0 && aPos<iSize,Panic(EBmaAllocOutOfRange));
	TUint *pM=iMap+(aPos>>KBitsPerIntShift);
	TUint mask=1<<(aPos&KBitsPerIntMask);
	__ASSERT_ALWAYS(!(*pM&mask),Panic(EBmaAllocAtAlreadyAllocated));
	*pM |= mask;
	iAvail--;
	}

EXPORT_C void CBitMapAllocator::AllocAt(TInt aPos, TInt aCount)
	{
	__ASSERT_ALWAYS((aPos>=0 && (aPos+aCount)<=iSize),Panic(EBmaAllocBlkOutOfRange));
	TUint *pM=iMap+(aPos>>KBitsPerIntShift);
	TInt c=aPos&KBitsPerIntMask;
	TUint m;
	if (aCount<(KBitsPerInt-c))
		{
		m=~(0xFFFFFFFFu<<aCount)<<c;
		if (*pM & m)
			Panic(EBmaAllocBlkNotFree);
		*pM |= m;
		iAvail-=aCount;
		return;
		}
	m=0xFFFFFFFFu<<c;
	if (*pM & m)
		Panic(EBmaAllocBlkNotFree);
	*pM++ |= m;
	c=aCount-KBitsPerInt+c;
	while(c>=KBitsPerInt)
		{
		if (*pM)
			Panic(EBmaAllocBlkNotFree);
		*pM++=0xFFFFFFFFu;
		c-=KBitsPerInt;
		}
	if (c)
		{
		m=0xFFFFFFFFu>>(KBitsPerInt-c);
		if (*pM & m)
			Panic(EBmaAllocBlkNotFree);
		*pM++ |= m;
		}
	iAvail-=aCount;
	}

EXPORT_C TInt CBitMapAllocator::ExtractRamPages(TInt aConsecutive,TInt& aPageNo)
	{
	if(iAvail<aConsecutive)
		return KErrNoMemory;

	TUint *pS=iMap;
	TUint *pE=pS+iLength;

	do	{
		register TUint n=*pS++;
		if (n!=0xFFFFFFFFu)
			{
			TInt x = 0;
			do
				{
				TInt bit=FindLeastSignificantZero(n,x);
				TInt pos=(TInt((pS-1)-iMap)<<KBitsPerIntShift)+bit;
				if(pos+aConsecutive > iSize)
					return KErrNoMemory;
				if(IsFree(pos,aConsecutive))
					{
					AllocAt(pos,aConsecutive);
					aPageNo=pos;
					return KErrNone;
					}
				else
					{
					x = bit+2;
					}
				}
			while (x < KBitsPerInt);
			} 
		} while(pS<pE);
	return KErrNoMemory;
	}

EXPORT_C TBool CBitMapAllocator::IsFree(TInt aPos)
//
// Check a required position is available
//
	{
	__ASSERT_ALWAYS(aPos>=0 && aPos<iSize,Panic(EBmaFreeOutOfRange));
	TUint n=iMap[aPos>>KBitsPerIntShift];
	return !(n>>(aPos&KBitsPerIntMask)&1);
	}

EXPORT_C TBool CBitMapAllocator::IsFree(TInt aPos, TInt aCount)
	{
	__ASSERT_ALWAYS((aPos>=0 && (aPos+aCount)<=iSize),Panic(EBmaChkBlkOutOfRange));
	TUint *pM=iMap+(aPos>>KBitsPerIntShift);
	TUint m=*pM++;
	TInt c=aPos&KBitsPerIntMask;
	m>>=c;
	if (aCount<(KBitsPerInt-c))
		{
		return !(m&~(0xFFFFFFFFu<<aCount));
		}
	aCount-=(KBitsPerInt-c);
	while(aCount>=KBitsPerInt)
		{
		m |= *pM++;
		aCount-=KBitsPerInt;
		}
	if (aCount)
		{
		m|=(*pM<<(KBitsPerInt-aCount));
		}
	return(!m);
	}

EXPORT_C void CBitMapAllocator::Free(TInt aPos)
//
// Free a position.
//
	{
	__ASSERT_ALWAYS(aPos>=0 && aPos<iSize,Panic(EBmaFreeOutOfRange));
	TUint *pM=iMap+(aPos>>KBitsPerIntShift);
	TUint mask=1<<(aPos&KBitsPerIntMask);
	__ASSERT_ALWAYS((*pM&mask),Panic(EBmaFreeNotAllocated));
	*pM &= ~mask;
	iAvail++;
	}

EXPORT_C void CBitMapAllocator::Free(TInt aPos, TInt aCount)
	{
	__ASSERT_ALWAYS((aPos>=0 && (aPos+aCount)<=iSize),Panic(EBmaFreeBlkOutOfRange));
	TUint *pM=iMap+(aPos>>KBitsPerIntShift);
	TInt c=aPos&KBitsPerIntMask;
	TUint m;
	if (aCount<(KBitsPerInt-c))
		{
		m=~(0xFFFFFFFFu<<aCount)<<c;
		if ((*pM & m)!=m)
			Panic(EBmaFreeBlkNotAllocated);
		*pM &= ~m;
		iAvail+=aCount;
		return;
		}
	m=0xFFFFFFFFu<<c;
	if ((*pM & m)!=m)
		Panic(EBmaFreeBlkNotAllocated);
	*pM++ &= ~m;
	c=aCount-KBitsPerInt+c;
	while(c>=KBitsPerInt)
		{
		if (*pM!=0xFFFFFFFF)
			Panic(EBmaFreeBlkNotAllocated);
		*pM++=0;
		c-=KBitsPerInt;
		}
	if (c)
		{
		m=0xFFFFFFFFu>>(KBitsPerInt-c);
		if ((*pM & m)!=m)
			Panic(EBmaFreeBlkNotAllocated);
		*pM++ &= ~m;
		}
	iAvail+=aCount;
	}

EXPORT_C TInt CBitMapAllocator::Avail()
//
// Get the available blocks count.
//
	{
	return(iAvail);
	}

EXPORT_C TInt CBitMapAllocator::Size()
//
// Get the size of all available blocks.
//
	{
	return(iSize);
	}

