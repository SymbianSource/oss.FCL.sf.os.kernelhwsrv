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
// 

/**
    @file
    @internalTechnology
*/

#if !defined(__FILESYSTEM_UTILS_INL__)
#define __FILESYSTEM_UTILS_INL__


//-----------------------------------------------------------------------------
/** @return 2^aVal */
inline TUint32 Pow2(TUint32 aVal)
    {
    ASSERT(aVal<32);
    return 1 << aVal;
    }

//-----------------------------------------------------------------------------
/** @return TUint32 value equals 2^aVal */
inline TUint32 Pow2_32(TUint32 aVal)
    {
    ASSERT(aVal < 32);
    return 1 << aVal;
    }

//-----------------------------------------------------------------------------
/** @return TUint64 value equals 2^aVal */
inline TUint64 Pow2_64(TUint32 aVal)
    {
    ASSERT(aVal < 64);
    return ((TUint64)1) << aVal;
    }

//-----------------------------------------------------------------------------
/**
    Indicates if a number passed in is a power of two
    @return ETrue if aVal is a power of 2 
*/
inline TBool IsPowerOf2(TUint32 aVal)
    {
    if (aVal==0)
        {
        ASSERT(0);
        return EFalse;
        }

    return !(aVal & (aVal-1));
    }

//-----------------------------------------------------------------------------
/**
    Indicates if a number passed in is a power of two
    @return ETrue if aVal is a power of 2 
*/
inline TBool IsPowerOf2_64(TUint64 aVal)
    {
    if (aVal==0)
        {
        ASSERT(0);
        return EFalse;
        }

    return !(aVal & (aVal-1));

    }

//-----------------------------------------------------------------------------

/**
    rounds down the given value to 2^aGranularityLog2
    @param  aVal                input value to round down
    @param  aGranularityLog2    Log2(granularity)
    @return rounded - down value
*/
inline TUint32 RoundDown(TUint32 aVal, TUint32 aGranularityLog2)
{
    ASSERT(aGranularityLog2 < 32);
    return (aVal >> aGranularityLog2) << aGranularityLog2;
}

//-----------------------------------------------------------------------------
/**  
    Rounds up aVal to the 2^aGranularityLog2 
    For example: RoundUp(0x08, 2) == 0x08; RoundUp(0x08, 3) == 0x08; RoundUp(0x08, 4) == 0x10; RoundUp(0x19, 4) == 0x20
    
    @return rounded-up value
*/
inline TUint32 RoundUp(TUint32 aVal, TUint32 aGranularityLog2)
    {
    ASSERT(aGranularityLog2 < 32);
                                         
    if( (aVal & ((1<<aGranularityLog2)-1)) == 0)
        return aVal;    

    aVal >>= aGranularityLog2;
    aVal++;
    aVal <<= aGranularityLog2;

    return aVal;
    }

//-----------------------------------------------------------------------------

/**
    @return Boolean exclusive OR between a1 and a2
    This function should be used on C-style TBool, which is, actually TInt type; Its '0' value means "False" and _any_ non-zero means "True"
    E.g: BoolXor(0x17, 0x4a) == EFalse;
*/
TBool BoolXOR(TBool a1, TBool a2)
    {
    if(!a1 && !a2)        
        return EFalse;
    else if(a1 && a2)
        return EFalse;
    else
        return ETrue;
    }

//-----------------------------------------------------------------------------

/**
    Calculates the log2 of a number
    This is the explicitly inlined version. Extensive using it may result in a code bloat.

    @param aNum Number to calulate the log two of
    @return The log two of the number passed in
*/
inline TUint32 Log2_inline(TUint32 aVal)
    {
    __ASSERT_COMPILE(sizeof(TUint32) == 4);
    ASSERT(aVal);

    TUint32 bitPos=31;

    if(!(aVal >> 16)) {bitPos-=16; aVal<<=16;}
    if(!(aVal >> 24)) {bitPos-=8;  aVal<<=8 ;}
    if(!(aVal >> 28)) {bitPos-=4;  aVal<<=4 ;}
    if(!(aVal >> 30)) {bitPos-=2;  aVal<<=2 ;}
    if(!(aVal >> 31)) {bitPos-=1;}
    
    return bitPos;
    }


//-----------------------------------------------------------------------------
/**
    Calculates number of '1' bits in the aVal
    This is the explicitly inlined version. Extensive using it may result in a code bloat.

    @param aVal some value
    @return number of '1' bits in the aVal
*/
inline TUint32 Count1Bits_inline(TUint32 aVal)
    {
    if(!aVal)
        return 0;

    if(aVal == 0xFFFFFFFF)
        return 32;

    aVal = aVal - ((aVal >> 1) & 0x55555555);
    aVal = (aVal & 0x33333333) + ((aVal >> 2) & 0x33333333);
    aVal = (aVal + (aVal >> 4)) & 0x0f0f0f0f;
    aVal = aVal + (aVal >> 8);
    aVal = aVal + (aVal >> 16);

    return aVal & 0x3f;
    }




//-----------------------------------------------------------------------------

/** clear all bits */
void T32Bits::Clear()
    {
    iData = 0;
    }

/** @return non-0 if at least one of 32 bits is set to '1' */
TBool T32Bits::HasBitsSet() const 
    {
    return iData;
    } 

/** sets bit number "aIndex" to '1' */
void T32Bits::SetBit(TUint32 aIndex)
    {
    ASSERT(aIndex < 32);
    iData |= (1<<aIndex);
    }

/** 
    Get value of the bit number "aIndex". 
    @return 0 if the bit aIndex is '0' non-zero otherwise
*/
TBool T32Bits::operator[](TUint32 aIndex) const
    {
    ASSERT(aIndex < 32);
    return (iData & (1<<aIndex));
    }










#endif //__FILESYSTEM_UTILS_INL__

















