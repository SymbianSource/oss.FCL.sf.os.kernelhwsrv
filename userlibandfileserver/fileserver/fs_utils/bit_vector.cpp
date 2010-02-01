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
//  Collection of common constants, utility functions, etc. for the file server and file systems.
//  Definitions here must be filesystem-agnostic, i.e. generic enougs to be used by every file system
//
//  This is the internal file and must not be exported.

/**
    @file
    @internalTechnology
*/

#include "bit_vector.h"


//#######################################################################################################################################
//#   RBitVector class implementation
//#######################################################################################################################################

const TUint32 K_FFFF = 0xFFFFFFFF; //-- all one bits, beware rigth shifts of signed integers!

RBitVector::RBitVector()
          :iNumBits(0), ipData(NULL), iNumWords(0)
    {
    }


RBitVector::~RBitVector()
    {
    Close();
    }

/**
    Panics.
    @param aPanicCode   a panic code
*/
void RBitVector::Panic(TPanicCode aPanicCode) const
    {
    _LIT(KPanicCat,"RBitVector");
    User::Panic(KPanicCat, aPanicCode);
    }

/** explicitly closes the object and deallocates memory */
void RBitVector::Close()
    {
    iNumBits = 0;
    iNumWords =0;
    User::Free(ipData);
    ipData = NULL;
    }

//-----------------------------------------------------------------------------

/**
    Comparison perator.
    @param  aRhs a vector to compate with.
    @panic ESizeMismatch in the case of different vector sizes
*/
TBool RBitVector::operator==(const RBitVector& aRhs) const
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(iNumBits == aRhs.iNumBits, Panic(ESizeMismatch));


    if(!iNumBits)
        return ETrue; //-- comparing 0-lenght arrays

    if(this == &aRhs)
        {//-- comparing with itself, potential source of errors
        ASSERT(0);
        return ETrue; 
        }
    
    if(iNumWords >= 1)
        {
        const TUint32 cntBytes = (iNumBits >> 5) << 2; //-- bytes to compare
        if(memcompare((const TUint8*)ipData, cntBytes, (const TUint8*)aRhs.ipData, cntBytes))
            return EFalse;
        }

    const TUint32 bitsRest  = iNumBits & 0x1F;
    if(bitsRest)
        {
        const TUint32 mask = K_FFFF >> (32-bitsRest);
        return ( (ipData[iNumWords-1] & mask) == (aRhs.ipData[iNumWords-1] & mask) );
        }
    
    return ETrue;
    }

TBool RBitVector::operator!=(const RBitVector& aRhs) const  
    {
    return ! ((*this) == aRhs);
    } 

//-----------------------------------------------------------------------------

/** The same as Create(), but leaves on error */
void RBitVector::CreateL(TUint32 aNumBits)
    {
    User::LeaveIfError(Create(aNumBits));
    }


/**
    Create the vector with the size of aNumBits bits.
    @return system-wide error codes:
        KErrNoMemory    unable to allocate sufficient amount of memory for the array
        KErrInUse       an attempt to call Create() for non-empty vector. Close it first.
        KErrArgument    invalid aNumBits value == 0
*/
TInt RBitVector::Create(TUint32 aNumBits)
    {

    if(ipData)
        return KErrInUse; //-- array is already in use. Close it first.

    if(!aNumBits)
        return KErrArgument;

    //-- memory is allocated by word (32 bit) quiantities
    const TUint32 numWords = (aNumBits >> 5) + ((aNumBits & 0x1F) > 0 ? 1:0);
    ipData = (TUint32*)User::AllocZ(numWords << 2);

    if(!ipData)
        return KErrNoMemory;

    iNumBits  = aNumBits;
    iNumWords = numWords;

    return KErrNone;
    }


/**
    Fill a bit vector with a given bit value
    @param aVal a bit value
*/
void RBitVector::Fill(TBool aVal)
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    memset(ipData, (aVal ? 0xFF : 0x00), iNumWords << 2);
    }

/** Invert all bits in a bit vector */
void RBitVector::Invert()
{
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    for(TUint32 i=0; i<iNumWords; ++i)
        ipData[i] ^= K_FFFF;
}


/**
    Perform "And" operation between 2 vectors. They shall be the same size.
    @param  aRhs a vector from the right hand side
    @panic ESizeMismatch in the case of different vector sizes
*/
void RBitVector::And(const RBitVector& aRhs)
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(iNumBits == aRhs.iNumBits, Panic(ESizeMismatch));
    for(TUint32 i=0; i<iNumWords; ++i)
        {
        ipData[i] &= aRhs.ipData[i];
        }
    }

/**
    Perform "Or" operation between 2 vectors. They shall be the same size.    
    @param  aRhs a vector from the right hand side
    @panic ESizeMismatch in the case of different vector sizes
*/
void RBitVector::Or(const RBitVector& aRhs)
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(iNumBits == aRhs.iNumBits, Panic(ESizeMismatch));
    for(TUint32 i=0; i<iNumWords; ++i)
        {
        ipData[i] |= aRhs.ipData[i];
        }
    }

/**
    Perform "Xor" operation between 2 vectors. They shall be the same size.    
    @param  aRhs a vector from the right hand side
    @panic ESizeMismatch in the case of different vector sizes
*/
void RBitVector::Xor(const RBitVector& aRhs)
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(iNumBits == aRhs.iNumBits, Panic(ESizeMismatch));
    for(TUint32 i=0; i<iNumWords; ++i)
        {
        ipData[i] ^= aRhs.ipData[i];
        }
    }

//-----------------------------------------------------------------------------
/**
    Fill a range from bit number "aIndexFrom" to "aIndexTo" inclusively with the value of aVal
    
    @param  aIndexFrom  start bit number (inclusive)
    @param  aIndexTo    end bit number (inclusive)
    @param  aVal        the value to be used to fill the range (0s or 1s)
*/
void RBitVector::Fill(TUint32 aIndexFrom, TUint32 aIndexTo, TBool aVal)
    {
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));

    //-- swap indexes if they are not in order
    if(aIndexFrom > aIndexTo)
        {
        const TUint32 tmp = aIndexFrom;
        aIndexFrom = aIndexTo;
        aIndexTo = tmp;
        }

    __ASSERT_ALWAYS((aIndexFrom < iNumBits) && (aIndexTo < iNumBits), Panic(EIndexOutOfRange));

    const TUint32 wordStart = WordNum(aIndexFrom);
    const TUint32 wordTo    = WordNum(aIndexTo);

    if(aVal)
        {//-- filling a range with '1'
        
        TUint32 shift = BitInWord(aIndexFrom);
        const TUint32 mask1 = (K_FFFF >> shift) << shift;

        TUint32 mask2 = K_FFFF;
        shift = 1+BitInWord(aIndexTo);
        if(shift < 32)
            {
            mask2 = ~((mask2 >> shift) << shift);
            }

        if(wordTo == wordStart)
            {//-- a special case, filling is in the same word
            ipData[wordStart] |= (mask1 & mask2);
            }
        else
            {
            ipData[wordStart] |= mask1; 
            ipData[wordTo]    |= mask2;
            
            const TUint32 wholeWordsBetween = wordTo - wordStart - 1; //-- whole words that can be bulk filled

            if(wholeWordsBetween)
                memset(ipData+wordStart+1, 0xFF, wholeWordsBetween << 2);
                            
            }
        }
    else
        {//-- filling a range with '0'
        
        TUint32 shift = BitInWord(aIndexFrom);
        const TUint32 mask1 = ~((K_FFFF >> shift) << shift);

        TUint32 mask2 = 0;
        shift = 1+BitInWord(aIndexTo);
        if(shift < 32)
            {
            mask2 = ((K_FFFF >> shift) << shift);
            }

        if(wordTo == wordStart)
            {//-- a special case, filling is in the same word
            ipData[wordStart] &= (mask1 | mask2);
            }
        else
            {
            ipData[wordStart] &= mask1; 
            ipData[wordTo]    &= mask2;
            
            const TUint32 wholeWordsBetween = wordTo - wordStart - 1; //-- whole words that can be bulk filled

            if(wholeWordsBetween)
                memset(ipData+wordStart+1, 0x00, wholeWordsBetween << 2);
                            
            }
        }

    }

//-----------------------------------------------------------------------------

/**
    Search for a specified bit value ('0' or '1') in the vector from the given position.
    @param  aStartPos   zero-based index; from this position the search will start. This position isn't included to the search.
                        On return may contain a new position if the specified bit is found in specified direction.
    @param  aBitVal     zero or non-zero bit to search.
    @param  aDir        Specifies the search direction

    @return ETrue if the specified bit value is found; aStartPos gets updated.
            EFalse otherwise.

*/
TBool RBitVector::Find(TUint32& aStartPos, TBool aBitVal, TFindDirection aDir) const
    {
    __ASSERT_ALWAYS(aStartPos < iNumBits, Panic(EIndexOutOfRange));
    ASSERT(iNumWords && ipData);

    switch(aDir)
        {
        case ERight:    //-- Search from the given position to the right
            return FindToRight(aStartPos, aBitVal);

        case ELeft:     //-- Search from the given position to the left (towards lower index)
            return FindToLeft(aStartPos, aBitVal);

        case ENearestL: //-- Search for the nearest value in both directions starting from left
            return FindNearest(aStartPos, aBitVal, ETrue);

        case ENearestR: //-- Search for the nearest value in both directions starting from right
            return FindNearest(aStartPos, aBitVal, EFalse);

        default:
            Panic(EWrondFindDirection);
            return EFalse;

        };
    
    }

//-----------------------------------------------------------------------------
/**
    Internal method to look for a given bit value in the right direction.
    see TBool RBitVector::Find(...)
*/
TBool RBitVector::FindToRight(TUint32& aStartPos, TBool aBitVal) const
    {
    if(aStartPos >= iNumBits-1)
        return EFalse; //-- no way to the right

    const TUint32 startPos = aStartPos+1;
    const TUint32 fInvert = aBitVal ? 0 : K_FFFF; //-- invert everything if we are looking for '0' bit

    TUint32 wordNum = WordNum(startPos);
    TUint32 val = ipData[wordNum] ^ fInvert;

    if(wordNum == iNumWords-1)
        {//-- process the last word in the array, some higher bits might not belong to the bit vector
        val = MaskLastWord(val);
        }

    const TUint32 shift = BitInWord(startPos);
    val = (val >> shift) << shift; //-- mask unused low bits

    if(val)
        {//-- there are '1' bits in the current word
        goto found;
        }
    else
        {//-- search in higher words
        wordNum++;

        while(iNumWords-wordNum > 1)
            {
            val = ipData[wordNum] ^ fInvert;
            if(val)
                goto found;

            wordNum++;
            }

        if(wordNum == iNumWords-1)
            {//-- process the last word in the array, some higher bith might not belong to the bit vector
            val = ipData[wordNum] ^ fInvert;
            val = MaskLastWord(val);

            if(val)
                goto found;
            }
        }

    return EFalse; //-- haven't found anything

  found:

    val &= (~val+1); //-- select rightmost bit
    aStartPos = (wordNum << 5)+Log2(val);
    return ETrue;
    }


//-----------------------------------------------------------------------------

/**
    Internal method to look for a given bit value in the left direction.
    see TBool RBitVector::Find(...)
*/
TBool RBitVector::FindToLeft(TUint32& aStartPos, TBool aBitVal) const
{
    if(!aStartPos)
        return EFalse; //-- no way to the left
    
    const TUint32 startPos=aStartPos-1;
    const TUint32 fInvert = aBitVal ? 0 : K_FFFF; //-- invert everything if we are looking for '0' bit

    TUint32 wordNum = WordNum(startPos);
    TUint32 val = ipData[wordNum] ^ fInvert;

    const TUint32 shift = 31-(BitInWord(startPos));
    val = (val << shift) >> shift; //-- mask unused high bits

    if(val)
    {//-- there are '1' bits in the current word
        goto found;
    }
    else
    {//-- search in the lower words
        while(wordNum)
        {
            wordNum--;
            val=ipData[wordNum] ^ fInvert;
            if(val)
                goto found;
        }
    }

    return EFalse; //-- nothing found

 found:
    aStartPos = (wordNum << 5)+Log2(val);
    return ETrue;
}

//-----------------------------------------------------------------------------

/**
    Internal method to look for a given bit value in the both directions.
    see TBool RBitVector::Find(...)
*/
TBool RBitVector::FindNearest(TUint32& aStartPos, TBool aBitVal, TBool aToLeft) const
{
    if(iNumBits < 2)
        return EFalse;

    if(aStartPos == 0)
        return FindToRight(aStartPos, aBitVal);

    if(aStartPos == iNumBits-1)
        return FindToLeft(aStartPos, aBitVal);

    
    const TUint32 fInvert = aBitVal ? 0 : K_FFFF; //-- invert everything if we are looking for '0' bit
    
    TUint32 wordNum = WordNum(aStartPos);
    TUint32 l_Idx; //-- index of the word to the left
    TUint32 r_Idx; //-- index of the word to the right
    
    l_Idx = r_Idx = wordNum;

    TBool   noWayLeft  = (wordNum == 0);            //-- if we are in the first word
    TBool   noWayRight = (wordNum == iNumWords-1);  //-- if we are in the last word

    //-- look in the current word first
    TUint32 val = ipData[wordNum] ^ fInvert;
    
    if(noWayRight)
    {   //-- this is the last word in the array, mask unused high bits in the last word
        val = MaskLastWord(val);
    }

    const TUint32 bitPos = aStartPos & 0x1F;
    val &= ~(1<<bitPos); //-- mask the bit at current position
    
    if(val == 0)
    {//-- no '1' bits in the current word
        noWayLeft  = ItrLeft(l_Idx);
        noWayRight = ItrRight(r_Idx);
    }
    else if(bitPos == 0)
    {
        noWayLeft = ItrLeft(l_Idx); //-- move to the previous word
    }
    else if(bitPos == 31)
    {
        noWayRight = ItrRight(r_Idx); //-- move to the next word
    }
    else
    {//-- look in the current word, in both halves to the left and right from the start position
        
        const TUint32 shift1 = 32-bitPos;
        const TUint32 partLo = (val << shift1) >> shift1; //-- towards lower bits

        const TUint32 shift2 = bitPos+1;
        const TUint32 partHi = (val >> shift2) << shift2; //-- towards higher bits 
        

        if(partLo && !partHi) //-- only lower part has '1' bits   
        {
            aStartPos = (wordNum << 5)+Log2(partLo);
            return ETrue;
        }
        else if(!partLo && partHi) //-- only higher part has '1' bits
        {
            aStartPos = (wordNum << 5)+Log2( (partHi & (~partHi+1)) );
            return ETrue;
        }
        else if(partLo && partHi) //-- both parts contain '1' bits, select the nearest one
        {
            const TUint32 posL = (wordNum << 5)+Log2(partLo);
            const TUint32 posR = (wordNum << 5)+Log2( (partHi & (~partHi+1)) );
        
            ASSERT(aStartPos > posL);
            ASSERT(posR > aStartPos);
            const TUint32 distL = aStartPos-posL;
            const TUint32 distR = posR-aStartPos;

            if(distL < distR)
            {
                aStartPos = posL;
                return ETrue;
            }
            else if(distL > distR)
            {
                aStartPos = posR;
                return ETrue;
            }
            else
            {//-- distL == distR, take into account search priority
                aStartPos = aToLeft ? posL : posR;
                return ETrue;
            }
        }
        else //-- (!partLo && !partHi), nothing in the current word
        {
            ASSERT(0);
        }

    }// if(bitPos > 0 && bitPos < 31)

    //-- now we are processing separate words from both sides of the search position
    for(;;)
    { 
        TUint32 wL = ipData[l_Idx] ^ fInvert;
        TUint32 wR = ipData[r_Idx] ^ fInvert;
        if(r_Idx == iNumWords-1)
        {   //-- this is the last word in the array, mask unused high bits in the last word
            wR = MaskLastWord(wR);
        }

        if(wL && !wR)
        {
            aStartPos = (l_Idx << 5)+Log2(wL);
            return ETrue;
        }
        else if(!wL && wR)
        {
            aStartPos = (r_Idx << 5)+Log2( (wR & (~wR+1)) );
            return ETrue;
        }
        else if(wL && wR)
        {
            const TUint32 posL = (l_Idx << 5)+Log2(wL);
            const TUint32 posR = (r_Idx << 5)+Log2( (wR & (~wR+1)) );
        
            ASSERT(aStartPos > posL);
            ASSERT(posR > aStartPos);
            const TUint32 distL = aStartPos-posL;
            const TUint32 distR = posR-aStartPos;

            if(distL < distR)
            {
                aStartPos = posL;
                return ETrue;
            }
            else if(distL > distR)
            {
                aStartPos = posR;
                return ETrue;
            }
            else
            {//-- distL == distR, take into account search priority
                aStartPos = aToLeft ? posL : posR;
                return ETrue;
            }

        }//else if(wL && wR)


        if(noWayLeft)
        {
            aStartPos = r_Idx << 5;
            return FindToRight(aStartPos, aBitVal);
        }
        else
        {
            noWayLeft  = ItrLeft(l_Idx);
        }

        if(noWayRight)
        {
            aStartPos = l_Idx << 5;
            return FindToLeft(aStartPos, aBitVal);
        }
        else
        {    
            noWayRight = ItrRight(r_Idx);
        }

   }//for(;;)

    //return EFalse;
}

//-----------------------------------------------------------------------------
/**
    Find out if two vectors are different.

    @param  aRhs        vector to compare with
    @param  aDiffIndex  if there is a differene, here will be the number of the first different bit
    @return ETrue if vectors differ, EFalse, if they are identical.
*/
TBool RBitVector::Diff(const RBitVector& aRhs, TUint32& aDiffIndex) const
{
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(iNumBits == aRhs.iNumBits, Panic(ESizeMismatch));
    ASSERT(iNumWords > 0);

    TUint32 diffWord=0;
    TUint32 wordNum=0;

    //-- compare all but the last word in the array
    for(wordNum=0; wordNum < iNumWords-1; ++wordNum)
    {
        diffWord = ipData[wordNum] ^ aRhs.ipData[wordNum];
        if(diffWord)
            break;  //-- found difference
    }

    //-- process the last word in the array
    if(!diffWord)
    {
        diffWord = MaskLastWord(ipData[wordNum]) ^ MaskLastWord(aRhs.ipData[wordNum]);
    }

    if(!diffWord)
        return EFalse; //-- vectors are the same

    //-- calculate the position of the bit that different.
    diffWord &= (~diffWord+1); //-- select rightmost bit
    aDiffIndex = (wordNum << 5)+Log2(diffWord);
    
    return ETrue;
}

//-----------------------------------------------------------------------------

/**
    Iterate to the left (towards lower index) in the array of words ipData

    @param  aIdx index within ipData array to be decremented; if it's possible to move left, it will be decreased
    @return ETrue if there is no way left i.e. aIdx is 0. EFalse otherwise and aIdx decreased.
*/
TBool RBitVector::ItrLeft(TUint32& aIdx) const
{
    if(aIdx == 0)
        return ETrue;
    else
    {
        aIdx--;
        return EFalse;
    }
}

//-----------------------------------------------------------------------------

/**
    Iterate to the right (towards higher index) in the array of words ipData

    @param  aIdx index within ipData array to be incremented; if it's possible to move right, it will be increased
    @return ETrue if there is no way right i.e. aIdx corresponds to the last word. EFalse otherwise and aIdx increased.
*/
TBool RBitVector::ItrRight(TUint32& aIdx) const
{
    if(aIdx < iNumWords-1)
    {
        aIdx++;
        return EFalse;
    }
    else
        return ETrue;
}

//-----------------------------------------------------------------------------

/**
    Import data to the internal bit vector representation.
    Just replaces number of bytes from apData to the ipData.

    @param aStartBit starting bit number. Must have 8-bit alignment.
    @param aNumBits  number of bits to import; granularity: 1 bit, i.e. it can be 177, for example.
    @param apData    pointer to the data (bitstream) to import.

*/
void RBitVector::DoImportData(TUint32 aStartBit, TUint32 aNumBits, const TAny* apData)
{
    ASSERT(aNumBits);
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    
    //-- check parameters granularity. aStartBit must have 8-bit alignment
    __ASSERT_ALWAYS(!(aStartBit & 0x07), Panic(EDataAlignment));


    __ASSERT_ALWAYS(iNumWords && (aStartBit+aNumBits <= iNumBits), Panic(EIndexOutOfRange));

    const TUint     bitsTail = aNumBits & 0x07;
    const TUint32   nBytes = aNumBits >> 3;
 
    if(nBytes)
    {//-- copy full array of bytes
        const TUint32   startByte = aStartBit >> 3;
        Mem::Copy(((TUint8*)ipData) + startByte, apData, nBytes);
    }

    if(bitsTail)
    {//-- we need to copy trailing bits from the input data to the corresponding byte of the internal array
        const TUint8 mask   = (TUint8)(0xFF >> (8-bitsTail));
        const TUint8 orMask = (TUint8)( *((const TUint8*)apData + nBytes) & mask);
        const TUint8 andMask= (TUint8)~mask;

        TUint8* pbData = (TUint8*)ipData + nBytes;
        *pbData &= andMask;
        *pbData |= orMask;
    }

}

//-----------------------------------------------------------------------------

/**
    Export data from the internal bit vector buffer to the external one.

    @param aStartBit starting bit number. Must have 8-bit alignment.
    @param aNumBits  number of bits to export, must comprise the whole byte, i.e. be multiple of 8.
                     The client is responsible for masking extra bits it doesn't need. 
                     Another implication: e.g. if the bitvector consists of 3 bits, this value must be 8.
                     The value of bits 3-7 in the aData[0] will be undefined.

    @param aData     destination data descriptor   
*/
void  RBitVector::DoExportData(TUint32 aStartBit, TUint32 aNumBits, TDes8& aData) const
{
    ASSERT(aNumBits);
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));

    //-- check parameters granularity.
    __ASSERT_ALWAYS(!(aStartBit & 0x07), Panic(EDataAlignment)); //-- aStartBit must have 8-bit alignment
    __ASSERT_ALWAYS(!(aNumBits & 0x07), Panic(EDataAlignment));  //-- number of bits shall comprise a byte

    __ASSERT_ALWAYS(iNumWords && (aStartBit+aNumBits <= (iNumWords << (KBitsInByteLog2+sizeof(TUint32))) ), Panic(EIndexOutOfRange));

    const TUint32 nBytes = aNumBits >> 3;
    const TUint32 startByte = aStartBit >> 3;
    
    aData.SetLength(nBytes);
    aData.Copy(((const TUint8*)ipData) + startByte, nBytes);
}

//-----------------------------------------------------------------------------

/**
    @return number of bits set to '1' in the vector
*/
TUint32 RBitVector::Num1Bits() const
{
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    if(!iNumBits)
        return 0;

    TUint32 cntBits = 0;

    TUint32 wordNum;
    for(wordNum=0; wordNum < iNumWords-1; ++wordNum)
    {
        cntBits += Count1Bits(ipData[wordNum]);
    }

    //-- process the last word, it shall be masked
    cntBits += Count1Bits(MaskLastWord(ipData[wordNum]));
    
    return cntBits;
}

//-----------------------------------------------------------------------------
/**
    @return number of bits set to '0' in the vector
*/
TUint32 RBitVector::Num0Bits() const
{
    return iNumBits - Num1Bits();
}


//-----------------------------------------------------------------------------
/**
    Calculate number of '1' bits in the range from aIndexFrom to aIndexTo inclusively

    @param  aIndexFrom  starting index; bit[aIndexFrom] is included to the search
    @param  aIndexTo    ending index;   bit[aIndexTo] is included to the search
    @return number of bits set to '1' in the slice
*/
TUint32 RBitVector::Num1Bits(TUint32 aIndexFrom, TUint32 aIndexTo) const
{
    __ASSERT_ALWAYS(ipData, Panic(ENotInitialised));
    __ASSERT_ALWAYS(aIndexTo < iNumBits && aIndexFrom <= aIndexTo, Panic(EIndexOutOfRange));

    const TUint32 wordFrom = WordNum(aIndexFrom); //?const
    const TUint32 wordTo   = WordNum(aIndexTo);

    if(wordFrom == wordTo)
    {//-- the same word
        TUint32 word = ipData[wordFrom];

        const TUint32 shMaskR = BitInWord(aIndexFrom);
        word >>= shMaskR; word <<= shMaskR; //-- zero low bits

        const TUint32 shMaskL = 31-BitInWord(aIndexTo);
        word <<= shMaskL;  //-- zero high bits

        return Count1Bits(word);
    } 

    TUint32 bitsCnt = 0;
    TUint32 wordsBetween = wordTo - wordFrom - 1;

    //-- count '1' bits in the termial words
    TUint32 word = ipData[wordFrom];
    const TUint32 shMaskR = BitInWord(aIndexFrom);
    word >>= shMaskR; //-- zero low bits
    bitsCnt += Count1Bits(word);

    word = ipData[wordTo];
    const TUint32 shMaskL = 31-BitInWord(aIndexTo);
    word <<= shMaskL;  //-- zero high bits
    bitsCnt += Count1Bits(word);

    //-- count '1' bits in the words between terminal ones
    TUint32 wordIdx = wordFrom+1;
    while(wordsBetween--)
    {
        bitsCnt += Count1Bits(ipData[wordIdx]);
    }
    

    return bitsCnt;
}






















