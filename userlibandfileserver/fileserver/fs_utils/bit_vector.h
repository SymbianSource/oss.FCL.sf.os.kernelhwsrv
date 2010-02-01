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

#if !defined(__FILESYSTEM_UTILS_BIT_VECTOR__)
#define __FILESYSTEM_UTILS_BIT_VECTOR__

#if !defined(__FILESYSTEM_UTILS_H__)
#include "filesystem_utils.h"
#endif


//#######################################################################################################################################

/**
    This class represents a bit vector i.e. an array of bits. Vector size can be from 1 to 2^32 bits.
    This class can be created on a stack (but needs to be placed into cleanup stack) or in a heap with the help of its factory methods Create/CreateL
*/
class RBitVector
    {
 public:
    
    RBitVector(); //-- Creates an empty vector. see Create() methods for memory allocation
   ~RBitVector(); 
    
    void Close(); 
    
    TInt Create(TUint32 aNumBits);
    void CreateL(TUint32 aNumBits);

    inline TUint32 Size() const;

    //-- single bit manipulation methods
    inline TBool operator[](TUint32 aIndex) const;
    inline void SetBit(TUint32 aIndex);
    inline void ResetBit(TUint32 aIndex);
    inline void InvertBit(TUint32 aIndex);
    inline void SetBitVal(TUint32 aIndex, TBool aVal);
    
    void Fill(TBool aVal);
    void Fill(TUint32 aIndexFrom, TUint32 aIndexTo, TBool aVal);

    void Invert();
   
    TBool operator==(const RBitVector& aRhs) const; 
    TBool operator!=(const RBitVector& aRhs) const;

    //-- logical operations between 2 vectors. 
    void And(const RBitVector& aRhs);
    void Or (const RBitVector& aRhs);
    void Xor(const RBitVector& aRhs);

    TBool Diff(const RBitVector& aRhs, TUint32& aDiffIndex) const;
    
    TUint32 Num1Bits() const;
    TUint32 Num1Bits(TUint32 aIndexFrom, TUint32 aIndexTo) const;

    TUint32 Num0Bits() const;


    /** Bit search specifiers */
    enum TFindDirection
        {
        ELeft,      ///< Search from the given position to the left (towards lower index)
        ERight,     ///< Search from the given position to the right (towards higher index)
        ENearestL,  ///< Search in both directions starting from the given position; in the case of the equal distances return the position to the left
        ENearestR   ///< Search in both directions starting from the given position; in the case of the equal distances return the position to the right

        //-- N.B the current position the search starts with isn't included to the search.
        };

    TBool Find(TUint32& aStartPos, TBool aBitVal, TFindDirection aDir) const;

    /** panic codes */
    enum TPanicCode
        {
        EIndexOutOfRange,       ///< index out of range
        EWrondFindDirection,    ///< a value doesn't belong to TFindDirection
        ESizeMismatch,          ///< Size mismatch for binary operators
        ENotInitialised,        ///< No memory allocated for the array
        ENotImplemented,        ///< functionality isn't implemented

        EDataAlignment,         ///< wrong data alignment when importing / exporting raw data
        };

 protected:
    
    //-- these are outlawed. Can't use them because memory allocator can leave and we don't have conthrol on it  in these methods. 
    RBitVector(const RBitVector& aRhs);            
    RBitVector& operator=(const RBitVector& aRhs); 

    void* operator new(TUint); //-- disable creating objects on heap.
    void* operator new(TUint, void*);
    //-------------------------------------

  
    void Panic(TPanicCode aPanicCode) const;

    inline TUint32 WordNum(TUint32 aBitPos)  const;
    inline TUint32 BitInWord(TUint32 aBitPos) const;

 protected:
    //-- special interface to acecess raw internal data. It's protected. Derive appropriate class from this one if you wan to use it
    void  DoImportData(TUint32 aStartBit, TUint32 aNumBits, const TAny* apData);
    void  DoExportData(TUint32 aStartBit, TUint32 aNumBits, TDes8& aData) const;


 private:
    TBool FindToRight(TUint32& aStartPos, TBool aBitVal) const;
    TBool FindToLeft (TUint32& aStartPos, TBool aBitVal) const;
    TBool FindNearest(TUint32& aStartPos, TBool aBitVal, TBool aToLeft) const;
   
    inline TUint32 MaskLastWord(TUint32 aVal) const; 
    inline TBool ItrLeft(TUint32& aIdx) const;
    inline TBool ItrRight(TUint32& aIdx) const;


 protected:

    TUint32   iNumBits; ///< number of bits in the vector
    TUint32*  ipData;   ///< pointer to the data 
    TUint32   iNumWords;///< number of 32-bit words that store bits
    };


//#######################################################################################################################################
//#   inline functions area
//#######################################################################################################################################


//--------------------------------------------------------------------------------------------------------------------------------- 
//-- class RBitVector

/** @return size of the vector (number of bits) */
inline TUint32 RBitVector::Size() const
    {
    return iNumBits;
    } 

/**
    Get a bit by index
    
    @param aIndex  index in a bit vector
    @return 0 if the bit at pos aIndex is 0, not zero otherwise
    @panic EIndexOutOfRange if aIndex is out of range
*/
inline TBool RBitVector::operator[](TUint32 aIndex) const
    {
    __ASSERT_ALWAYS(aIndex < iNumBits, Panic(EIndexOutOfRange));
    return (ipData[WordNum(aIndex)] & (1<<BitInWord(aIndex)));
    }

/**
    Set a bit at pos aIndex to '1'
    @param aIndex  index in a bit vector
    @panic EIndexOutOfRange if aIndex is out of range
*/
inline void RBitVector::SetBit(TUint32 aIndex)
    {
    __ASSERT_ALWAYS(aIndex < iNumBits, Panic(EIndexOutOfRange));
    ipData[WordNum(aIndex)] |= (1<<BitInWord(aIndex));
    }

/**
    Set a bit at pos aIndex to '0'
    @param aIndex  index in a bit vector
    @panic EIndexOutOfRange if aIndex is out of range
*/
inline void RBitVector::ResetBit(TUint32 aIndex)
    {
    __ASSERT_ALWAYS(aIndex < iNumBits, Panic(EIndexOutOfRange));
    ipData[WordNum(aIndex)] &= ~(1<<BitInWord(aIndex));
    }

/**
    Invert a bit at pos aIndex
    @param aIndex  index in a bit vector
    @panic EIndexOutOfRange if aIndex is out of range
*/
inline void RBitVector::InvertBit(TUint32 aIndex)
    {
    __ASSERT_ALWAYS(aIndex < iNumBits, Panic(EIndexOutOfRange));
    ipData[WordNum(aIndex)] ^= (1<<BitInWord(aIndex));
    }

/**
    Set bit value at position aIndex
    @param aIndex  index in a bit vector
    @panic EIndexOutOfRange if aIndex is out of range
*/
inline void RBitVector::SetBitVal(TUint32 aIndex, TBool aVal)
    {
    if(aVal) 
        SetBit(aIndex);
    else 
        ResetBit(aIndex);
    }


inline TUint32 RBitVector::MaskLastWord(TUint32 aVal) const
    {
    const TUint32 shift = (32-(iNumBits & 0x1F)) & 0x1F;
    return (aVal << shift) >> shift; //-- mask unused high bits
    }

inline TUint32 RBitVector::WordNum(TUint32 aBitPos)  const
    {
    return aBitPos >> 5;
    }

inline TUint32 RBitVector::BitInWord(TUint32 aBitPos) const 
    {
    return aBitPos & 0x1F;
    }



#endif //__FILESYSTEM_UTILS_BIT_VECTOR__













