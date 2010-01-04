/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/
// There are 2 reasons why not use existing unicodeconv.cpp:
// 1) "unicode->foreign" in existing unicodeconv.cpp is quite slow, especially
//    for huge code pages (e.g, Asia code pages). See INC127598.
//
// 2) GB18030 has 32-bit code that existing unicodeconv.cpp cannot handle.
//
// The algorithm of this special version unicodeconv.cpp is straightforward:
// 1) foreign->unicode:
//    1.1) 1 byte/2 byte->unicode bmp: use existing mechanism; mapping table in
//              "cp54936_2byte_tounicode.cpp", which is generated with command
//              "perl -w ..\group\FatConversionTable.pl cp54936_2byte.txt".
//
//    1.2) 4 byte->unicode bmp: convert the 4-byte code to a 16-bit index, then
//              search into the mapping table in "cp54936_4byte_tounicode.cpp",
//              which is generated with command
//              "perl -w ..\group\cp54936_4byte_tounicode.pl cp54936_4byte.txt".
//
//    1.3) 4 byte->unicode non-bmp: calculate with formula in this file.
//
// 2) unicode->foreign:
//    2.1) unicode bmp->1/2/4 byte: the huge table in "cp54936_allbmp_fromunicode.cpp"
//              can map directly, which is generated with command
//              "perl -w ..\group\cp54936_allbmp_fromunicode.pl cp54936_2byte.txt cp54936_4byte.txt".
//
//    2.2) unicode non-bmp->4 byte: calculate with formula in this file.
//
// The function cp54936_2byte_tounicode.cpp::TConvDataStruct::
// ConvertSingleUnicode() is not used anymore. It's reserved just because not
// changing the tool FatConversionTable.pl.
//
// About the mapping table "cp54936_2byte.txt" and "cp54936_4byte.txt":
// 1) All Private Used Area (PUA) code points are reserved.
// 2) All GB18030 code points that mapping to undefined Unicode are reserved.
//
//
// About the formula for non-bmp calculation:
// 1) All code points from 0x10000 to 0x10FFFF are supported.
// 2) Code points in 0x10000-0x1FFFF and 0x30000-0x10FFFF are summarized from
//    the GB18030 standard, since the standard does not define the mapping for
//    code points out of 0x20000-0x2FFFF.


#include <e32std.h>
#include <e32def.h>
#include <e32des8.h> 
#include "unicodeconv.h"
#include "cp54936.h"


enum TFccPanic
	{
	EBadForeignCode = 0,
	E4ByteIndexOutOfRange,
	EPanicBadIndices1,
	EInavlidUnicodeValue
	};
void Panic(TFccPanic aPanic)
	{

	User::Panic(_L("FatCharsetConv"),aPanic);
	}


//replacement character to be used when unicode cannot be converted
const TUint8 KForeignReplacement = 0x5F;

const TUint8 KU10000Byte1 = 0x90;
const TUint8 KU10000Byte2 = 0x30;
const TUint8 KU10000Byte3 = 0x81;
const TUint8 KU10000Byte4 = 0x30;

inline TBool IsSupplementary(TUint aChar)
/**
@param aChar The 32-bit code point value of a Unicode character.

@return True, if aChar is supplementary character; false, otherwise.
*/
	{
	return (aChar > 0xFFFF);
	}

inline TBool IsSurrogate(TText16 aInt16)
/**
@return True, if aText16 is high surrogate or low surrogate; false, otherwise.
*/
	{
	return (aInt16 & 0xF800) == 0xD800;
	}

inline TBool IsHighSurrogate(TText16 aInt16)
/**
@return True, if aText16 is high surrogate; false, otherwise.
*/
	{
	return (aInt16 & 0xFC00) == 0xD800;
	}

inline TBool IsLowSurrogate(TText16 aInt16)
/**
@return True, if aText16 is low surrogate; false, otherwise.
*/
	{
	return (aInt16 & 0xFC00) == 0xDC00;
	}

inline TUint JoinSurrogate(TText16 aHighSurrogate, TText16 aLowSurrogate)
/**
Combine a high surrogate and a low surrogate into a supplementary character.

@return The 32-bit code point value of the generated Unicode supplementary
        character.
*/
	{
	return ((aHighSurrogate - 0xD7F7) << 10) + aLowSurrogate;
	}

inline TText16 GetHighSurrogate(TUint aChar)
/**
Retrieve the high surrogate of a supplementary character.

@param aChar The 32-bit code point value of a Unicode character.

@return High surrogate of aChar, if aChar is a supplementary character; 
        aChar itself, if aChar is not a supplementary character.
*/
	{
	return STATIC_CAST(TText16, 0xD7C0 + (aChar >> 10));
	}

inline TText16 GetLowSurrogate(TUint aChar)
/**
Retrieve the low surrogate of a supplementary character.

@param aChar The 32-bit code point value of a Unicode character.

@return Low surrogate of aChar, if aChar is a supplementary character; 
        zero, if aChar is not a supplementary character.
*/
	{
	return STATIC_CAST(TText16, 0xDC00 | (aChar & 0x3FF));
	}

//This function converts from Unicoded characters, to foreign characters and adds them into a descriptor
EXPORT_C void UnicodeConv::ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode)
	{
    UnicodeConv::ConvertFromUnicodeL(aForeign, aUnicode, ETrue);
    }

//This function converts from Unicoded characters, to foreign characters and adds them into a descriptor
EXPORT_C TInt UnicodeConv::ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TBool leaveWhenOverflow)
	{
	const TInt length = aUnicode.Length();
	const TUint16* unicode = aUnicode.Ptr();
	const TUint16* guard = unicode + length;
	
	TUint8* foreign = const_cast<TUint8*>(aForeign.Ptr());
	TUint8* foreignguard = foreign + aForeign.MaxLength();
	
	//loop going through the character of the unicode descriptor
	while (unicode < guard)
		{
		TUint32 unicodeChar = *unicode++;
		if (IsHighSurrogate(unicodeChar))
			{
			if (unicode >= guard || !IsLowSurrogate(*unicode))
				{
				if (foreign >= foreignguard)
					{
                    aForeign.SetLength(foreign-aForeign.Ptr());
					if (leaveWhenOverflow)
						User::Leave(KErrOverflow);
                    else
                    	return KErrOverflow;
					}
				*foreign++ = KForeignReplacement;
				continue;
				}
			unicodeChar = JoinSurrogate(unicodeChar, *unicode++);
			}
		if (IsLowSurrogate(unicodeChar))
			{
			if (foreign >= foreignguard)
				{
				aForeign.SetLength(foreign-aForeign.Ptr());
				if (leaveWhenOverflow)
					User::Leave(KErrOverflow);
				else
					return KErrOverflow;
				}
			*foreign++ = KForeignReplacement;
			continue;
			}
		
		TUint8 b1, b2, b3, b4;		// byte 1,2,3,4 of result GB18030 code.
		TInt count;					// byte count of result GB18030 code; can be 1, 2 or 4.
		
		// unicode to cp54936
		if (IsSupplementary(unicodeChar))
			{
			unicodeChar -= 0x10000;
			b4 = unicodeChar % 10 + KU10000Byte4;
			unicodeChar /= 10;
			b3 = unicodeChar % 126 + KU10000Byte3;
			unicodeChar /= 126;
			b2 = unicodeChar % 10 + KU10000Byte2;
			b1 = unicodeChar / 10 + KU10000Byte1;
			count = 4;
			}
		else
			{
			TUint32 foreignChar;
			foreignChar = KMappingTableUnicodeBmp2CP54936[unicodeChar];
			b1 = ((foreignChar >> 24) & 0xFF);
			b2 = ((foreignChar >> 16) & 0xFF);
			b3 = ((foreignChar >> 8) & 0xFF);
			b4 = (foreignChar & 0xFF);
			count = 1;
			if (b1)
				{
				count = 4;
				}
			else
				{
				__ASSERT_DEBUG(b2==0, Panic(EBadForeignCode));
				if (b3)
					{
					count = 2;
					}
				}
			}
		
		if (foreign + count > foreignguard)
			{
			aForeign.SetLength(foreign-aForeign.Ptr());
            if (leaveWhenOverflow)
            	User::Leave(KErrOverflow);
            else
            	return KErrOverflow;
			}
		if (count == 4)
			{
			*foreign++ = b1;
			*foreign++ = b2;
			}
		if (count >= 2)
			*foreign++ = b3;
		*foreign++ = b4;
		}
	aForeign.SetLength(foreign-aForeign.Ptr());
	return KErrNone;
	}


//This function converts from foreign characters into unicode and adds them into a descriptor
EXPORT_C void UnicodeConv::ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign)
	{
    UnicodeConv::ConvertToUnicodeL(aUnicode, aForeign, ETrue);
    }

//This function converts from foreign characters into unicode and adds them into a descriptor
EXPORT_C TInt UnicodeConv::ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TBool leaveWhenOverflow)
	{
	const TInt foreignLength = aForeign.Length();
	const TUint8* foreign = aForeign.Ptr();
	const TUint8* guard = foreign + foreignLength;
	
	TUint16* unicode = const_cast<TUint16*>(aUnicode.Ptr());
	TUint16* unicodeguard = unicode + aUnicode.MaxLength();
	
	TUint8 b1, b2, b3, b4;
	enum TCodeType
	{
	E1Byte = 0,
	E2Byte,
	E4ByteBmp,
	E4ByteSupplementary,
	EError,
	};
	TCodeType codetype;
	TUint32 unicodeChar;

	//loop going through the characters of the foreign descriptor
	while (foreign < guard)
		{
		// roughly, detect which area the foreign code belongs to
		b1 = *foreign++;
		if (b1 <= 0x7F)
			codetype = E1Byte;
		else if (b1 == 0x80 || b1 > 0xFE)
			codetype = EError;
		else if (foreign >= guard)
			codetype = EError;
		else
			{
			b2 = *foreign++;
			if (b2 >= 0x40 && b2 <= 0xFE && b2 != 0x7F)
				codetype = E2Byte;
			else if (b2 < 0x30 || b2 > 0x39)
				codetype = EError;
			else if (foreign+1 >= guard)
				codetype = EError;
			else
				{
				b3 = *foreign++;
				if (b3 < 0x81 || b3 > 0xFE)
					codetype = EError;
				else
					{
					b4 = *foreign++;
					if (b4 < 0x30 || b4 > 0x39)
						codetype = EError;
					else if (b1 >= 0x81 && b1 <= 0x84)		// 0x81308130-0x8439FE39
						codetype = E4ByteBmp;
					else if (b1 >= 0x90 && b1 <= 0xE3)		// 0x90308130-0xE339FE39
						codetype = E4ByteSupplementary;
					else
						codetype = EError;					// others are reserved
					}
				}
			}
		
		// cp54936 to unicode
		if (codetype == E1Byte)
			{
			unicodeChar = b1;
			}
		else if (codetype == E2Byte)
			{
			// conventional algorithm used in FatCharsetConv
			const TLeadOrSingle* structPtr = TConvDataStruct::KFirstByteConversions + (b1-0x80);
			if (structPtr->iUnicodeIfSingle)
				unicodeChar = structPtr->iUnicodeIfSingle;
			else if (TConvDataStruct::KMinTrailByte <= b2 && b2 <= TConvDataStruct::KMaxTrailByte)
				unicodeChar = TConvDataStruct::KDoubleByteConversions[structPtr->iDoubleByteIndex + (b2 - TConvDataStruct::KMinTrailByte)];
			else
				unicodeChar = 0xFFFD;
			}
		else if (codetype == E4ByteBmp)
			{
			TUint index = (b1-0x81)*12600 + (b2-0x30)*1260 + (b3-0x81)*10 + (b4-0x30);
			__ASSERT_DEBUG(index<39420, Panic(E4ByteIndexOutOfRange));
			unicodeChar = KMappingTable4ByteBmp2Unicode[index];
			}
		else if (codetype == E4ByteSupplementary)
			{
			unicodeChar = 0x10000 + (b1 - KU10000Byte1) * 12600 +
									(b2 - KU10000Byte2) * 1260 +
									(b3 - KU10000Byte3) * 10 +
									(b4 - KU10000Byte4);
			__ASSERT_DEBUG(unicodeChar >= 0x10000 && unicodeChar <= 0x10FFFF, Panic(EInavlidUnicodeValue));
			}
		else
			{
			unicodeChar = 0xFFFD;
			}
		
		// append to output buffer
		if (IsSupplementary(unicodeChar))
			{
			if (unicode + 1 >= unicodeguard)
				{
				aUnicode.SetLength(unicode-aUnicode.Ptr());
				if (leaveWhenOverflow)
					User::Leave(KErrOverflow);
				else
					return KErrOverflow;
				}
			*unicode++ = GetHighSurrogate(unicodeChar);
			*unicode++ = GetLowSurrogate(unicodeChar);
			}
		else
			{
			if (unicode >= unicodeguard)
				{
				aUnicode.SetLength(unicode-aUnicode.Ptr());
                if (leaveWhenOverflow)
                	User::Leave(KErrOverflow);
                else
                	return KErrOverflow;
				}
			*unicode++ = unicodeChar;
			}
		}
	aUnicode.SetLength(unicode-aUnicode.Ptr());
	return KErrNone;
	}

EXPORT_C TBool UnicodeConv::IsLegalShortNameCharacter (TUint aCharacter)
	{
	//1. aCharacter >= 0x0080 
	if (aCharacter>=0x0080)
		{
		// Since all Unicode characters can be mapped to GB18030, so no need to
		// test the converting.
		if (aCharacter <= 0x10FFFF && !IsSurrogate(aCharacter))
			return ETrue;
		else
			return EFalse;
		}

    // For most common cases: 
    // Note: lower case characters are considered legal DOS char here. 
	if ((aCharacter>='a' && aCharacter<='z') || 
	    (aCharacter>='A' && aCharacter<='Z') || 
	    (aCharacter>='0' && aCharacter<='9'))
			{
			return ETrue;
			}
    // Checking for illegal chars: 
    // 2. aCharacter <= 0x20 
    // Note: leading 0x05 byte should be guarded by callers of this function 
    //  as the information of the position of the character is required. 
	if (aCharacter < 0x20)
		return EFalse;
	// Space (' ') is not considered as a legal DOS char here.
	if (aCharacter == 0x20)
		return EFalse;
	
	// 3. 0x20 < aCharacter < 0x80 
    // According to FAT Spec, "following characters are not legal in any bytes of DIR_Name": 
    switch (aCharacter) 
            { 
            case 0x22:        // '"' 
            case 0x2A:        // '*' 
            case 0x2B:        // '+' 
            case 0x2C:        // ',' 
            //case 0x2E:        // '.'   // Although '.' is not allowed in any bytes of DIR_Name, it 
                                         // is a valid character in short file names. 
            case 0x2F:        // '/' 
            case 0x3A:        // ':' 
            case 0x3B:        // ';' 
            case 0x3C:        // '<' 
            case 0x3D:        // '=' 
            case 0x3E:        // '>' 
            case 0x3F:        // '?' 
            case 0x5B:        // '[' 
            case 0x5C:        // '\' 
            case 0x5D:        // ']' 
            case 0x7C:        // '|' 
            	return EFalse; 
            default: 
            	return ETrue; 
            } 
	}		

