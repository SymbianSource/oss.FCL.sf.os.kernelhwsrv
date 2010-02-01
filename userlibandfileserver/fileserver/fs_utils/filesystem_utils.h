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

#if !defined(__FILESYSTEM_UTILS_H__)
#define __FILESYSTEM_UTILS_H__

#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif

//#######################################################################################################################################
//#   constants definitions
//#######################################################################################################################################

const TUint KBitsInByteLog2 = 3;
const TUint KBitsInByte = 1<<KBitsInByteLog2;


const TUint16 K1KiloByteLog2 = 10;
const TUint32 K1KiloByte = 1<<K1KiloByteLog2; 
const TUint32 K1MegaByte = 1<<20; 

const TUint32 K1uSec = 1;               ///< 1 misrosecond in TTimeIntervalMicroSeconds32
const TUint32 K1mSec = 1000;            ///< 1 millisecond in TTimeIntervalMicroSeconds32
const TUint32 K1Sec  = 1000*K1mSec;     ///< 1 second in TTimeIntervalMicroSeconds32

//---------------------------------------------------------------------------------------------------------------------------------------

const TUint KDefSectorSzLog2=9;                         ///< Log2 of the default sector size for the media 
const TUint KDefaultSectorSize = 1 << KDefSectorSzLog2; ///< Default sector size for the media, 512 bytes

//#######################################################################################################################################
//#   some useful utility functions
//#######################################################################################################################################

inline TUint32 Pow2(TUint32 aVal);          //-- return 2^aVal
inline TUint32 Pow2_32(TUint32 aVal);       //-- return 2^aVal
inline TUint64 Pow2_64(TUint32 aVal);       //-- return 2^aVal

inline TBool IsPowerOf2(TUint32 aVal);      //-- return ETrue if aVal is a power of 2 
inline TBool IsPowerOf2_64(TUint64 aVal);   //-- return ETrue if aVal is a power of 2 

inline TUint32 RoundDown(TUint32 aVal, TUint32 aGranularityLog2);
inline TUint32 RoundUp(TUint32 aVal, TUint32 aGranularityLog2);

inline TBool BoolXOR(TBool a1, TBool a2);           //-- return Boolean XOR of a1 and a2

inline TUint32 Log2_inline(TUint32 aVal);           //-- Calculates the Log2(aVal)
TUint32 Log2(TUint32 aVal);                         //-- Calculates the Log2(aVal)

inline TUint32 Count1Bits_inline(TUint32 aVal);     //-- counts number of '1' bits in the aVal
TUint32 Count1Bits(TUint32 aVal);                   //-- counts number of '1' bits in the aVal

//-----------------------------------------------------------------------------

TPtrC RemoveTrailingDots(const TDesC& aName); //-- Removes trailing dots from aName. "Name..." -> "Name"


//#######################################################################################################################################
/**
    A class representing a simple abstraction of the 32 bit flags
*/
class T32Bits
{
 public:
    T32Bits() : iData(0) {}

    inline void  Clear();
    inline TBool HasBitsSet() const;
    inline void SetBit(TUint32 aIndex);
    inline TBool operator[](TUint32 aIndex) const;

 private:
    TUint32 iData; ///< 32 bits data
};






#include "filesystem_utils.inl"


#endif //__FILESYSTEM_UTILS_H__












