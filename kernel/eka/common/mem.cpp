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
// e32\common\mem.cpp
// 
//

#include "common.h"

//This is used in heap.cpp and it is defined here, as a requirement for patchdata
EXPORT_D extern const TInt KHeapMinCellSize = 0;
EXPORT_D extern const TInt KHeapShrinkHysRatio = RHeap::EShrinkRatioDflt;
//NOTE - if these values are changed then the WINS test case value must be updated

#ifndef __MEM_MACHINE_CODED__

extern "C" {

#ifndef __MEMMOVE_MACHINE_CODED__

// See header file e32cmn.h for the in-source documentation.
EXPORT_C TAny* memcpy(TAny* aTrg, const TAny* aSrc, unsigned int aLength)
	{
	return memmove(aTrg, aSrc, aLength);
	}



// See header file e32cmn.h for the in-source documentation.
EXPORT_C TAny* memmove(TAny* aTrg, const TAny* aSrc, unsigned int aLength)
	{
	if (aLength==0)
		return((TUint8*)aTrg);
	TInt aLen32=0;
	TUint32* pT32=(TUint32*)aTrg;
	const TUint32* pS32=(TUint32 *)aSrc;
	if (((TInt)pT32&3)==0 && ((TInt)pS32&3)==0)
		aLen32=aLength>>2;
	TInt aLen8=aLength-(aLen32<<2);
	TUint32* pE32=pT32+aLen32;
	TUint8* pT;
    TUint8* pE;
    TUint8* pS;
	if (aTrg<aSrc)
		{
		pS32=(TUint32*)aSrc;
		while (pT32<pE32)
			*pT32++=(*pS32++);
		pT=(TUint8*)pT32;
		pS=(TUint8*)pS32;
		pE=(TUint8*)aTrg+aLength;
		while (pT<pE)
			*pT++=(*pS++);
		}
	else if (aTrg>aSrc)
		{
		pT=(TUint8*)(pT32+aLen32);
		pE=pT+aLen8;
		pS=(TUint8*)aSrc+aLength;
		while (pE>pT)
			*--pE=(*--pS);
		pS32=(TUint32*)pS;
		while (pE32>pT32)
			*--pE32=(*--pS32);
		}
	return aTrg;
	}

#endif // ! __MEMMOVE_MACHINE_CODED__

// See header file e32cmn.h for the in-source documentation.
EXPORT_C TAny* memclr(TAny* aTrg, unsigned int aLength)
	{
	return memset(aTrg, 0, aLength);
	}



// See header file e32cmn.h for the in-source documentation.
EXPORT_C TAny* memset(TAny* aTrg, TInt aValue, unsigned int aLength)
	{
	TInt aLen32=0;
	TUint32 *pM32=(TUint32 *)aTrg;
	if (((TInt)aTrg&3)==0)
		{
		aLen32=aLength>>2;
		TUint32 *pE32=pM32+aLen32;
		TUint c = aValue & 0xff;
		TUint32 fillChar=c+(c<<8)+(c<<16)+(c<<24);
		while (pM32<pE32)
			*pM32++=fillChar;
		}
	TInt aLen8=aLength-(aLen32<<2);
	TUint8 *pM=(TUint8 *)pM32;
	TUint8 *pE=pM+aLen8;
	while (pM<pE)
		*pM++=TUint8(aValue);
	return aTrg;
	}

} // extern "C"

extern "C" {



// See header file e32cmn.h for the in-source documentation.
EXPORT_C TAny* wordmove(TAny* aTrg, const TAny* aSrc, unsigned int aLength)
	{
	__ASSERT_DEBUG((aLength&3)==0,Panic(EWordMoveLengthNotMultipleOf4));
	__ASSERT_DEBUG((((TUint)aSrc)&3)==0,Panic(EWordMoveSourceNotAligned));
	__ASSERT_DEBUG((((TUint)aTrg)&3)==0,Panic(EWordMoveTargetNotAligned));
	if (aLength==0)
		return((TUint8*)aTrg);
	TInt len=aLength>>2;
	TUint32* pT=(TUint32*)aTrg;
	const TUint32* pS=(const TUint32*)aSrc;
	const TUint32* pE=pS+len;
	if (pT<pS)
		{
		while (pS<pE)
			*pT++=(*pS++);
		}
	else if (pT>pS)
		{
        pT+=len;
		while (pE>pS)
			*--pT=(*--pE);
		}
	return aTrg;
	}




// See header file e32cmn.h for the in-source documentation.
EXPORT_C TInt memcompare(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL)
	{

	__ASSERT_DEBUG(aLeftL>=0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL>=0,Panic(EMemRightNegative));
	const TUint8 *pE=aLeft+Min(aLeftL,aRightL);
    while (aLeft<pE)
		{
		TInt d=(*aLeft++)-(*aRight++);
		if (d!=0)
		    return(d);
		}
    return(aLeftL-aRightL);
	}

} // extern "C"




#if defined(__GCC32__) && !defined(__KERNEL_MODE__)
/**
Compares a block of data at one specified location with a block of data at 
another specified location.

The comparison proceeds on a byte for byte basis, the result of the comparison 
is based on the difference of the first bytes to disagree.

The data at the two locations are equal if they have the same length and content. 
Where the lengths are different and the shorter section of data is the same 
as the first part of the longer section of data, the shorter is considered 
to be less than the longer.

@param aLeft   A pointer to the first (or left) block of 8 bit data
               to be compared.
@param aLeftL  The length of the first (or left) block of data to be compared,  
               i.e. the number of bytes.
@param aRight  A pointer to the second (or right) block of 8 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e. the number of bytes.
               
@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the
        second (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content.
*/
EXPORT_C TInt Mem::Compare(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL)
	{
	memcompare(aLeft, aLeftL, aRight, aRightL);
	}
#endif

#else // __MEM_MACHINE_CODED__

#if defined(_DEBUG) && defined(__CPU_ARM)

GLDEF_C void PanicEWordMoveLengthNotMultipleOf4()
	{
	Panic(EWordMoveLengthNotMultipleOf4);
	}

GLDEF_C void PanicEWordMoveSourceNotAligned()
	{
	Panic(EWordMoveSourceNotAligned);
	}

GLDEF_C void PanicEWordMoveTargetNotAligned()
	{
	Panic(EWordMoveTargetNotAligned);
	}

#endif

#endif // __MEM_MACHINE_CODED__

#ifndef __KERNEL_MODE__
//
// Dummy class for Binary Compatibility purposes
//
class Mem1
	{
public:
	IMPORT_C static TUint8* Copy(TAny* aTrg,const TAny* aSrc,TInt aLength);
	IMPORT_C static TUint8* Move(TAny* aTrg,const TAny* aSrc,TInt aLength);
	IMPORT_C static void Fill(TAny* aTrg,TInt aLength,TChar aChar);
	};
EXPORT_C void Mem1::Fill(TAny* aTrg,TInt aLength,TChar aChar)
	{ Mem::Fill(aTrg,aLength,aChar); }
EXPORT_C TUint8* Mem1::Copy(TAny* aTrg, const TAny* aSrc, TInt aLength)
	{ return Mem::Copy(aTrg, aSrc, aLength); }
EXPORT_C TUint8* Mem1::Move(TAny* aTrg, const TAny* aSrc, TInt aLength)
	{ return Mem::Move(aTrg, aSrc, aLength); }
		
#endif // __KERNEL_MODE__
