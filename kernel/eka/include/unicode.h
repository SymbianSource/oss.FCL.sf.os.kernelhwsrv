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
// e32\include\unicode.h
// The TUnicode class contains a Unicode value. It is provided for convenience in implementing the
// character attribute retrieval functions. It also contains:
// structures used to store and search the tables of character information:
// when modifying these, please remember that they form part of tables that must be initialised as aggregates,
// so they cannot have constructors, non-public members, base classes or virtual functions. I have used 'struct'
// rather than class to make that clear.
// default constructor that sets the stored Unicode value to 0xFFFF - an invalid character
// constructors and conversion functions for converting between integers and TUnicode objects
// functions to retrieve the categories and attributes
// The categories are explained in 'unicode_fields.txt', which is a key to the fields of the data file
// 'unidata2.txt'; these files are supplied on the CD-ROM that comes with the book 'The Unicode Standard,
// Version 2.0'.
// Because the category constants must be available to users they are defined not here but in the TChar
// class in e32std.h.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/


#ifndef __UNICODE_H__
#define __UNICODE_H__ 1

#include <e32cmn.h>

/*
A structure to contain the raw data about a Unicode character:
it must not have a constructor because an array of these in unitable.cpp is initialised as an aggregate.
*/
struct TUnicodeData
	{
	// bit values for iFlags
	enum
		{
		EHasLowerCase = 1,			// adding the case offset gives the lower case form
		EHasUpperCase = 2,			// subtracting the case offset gives the upper case form
		EHasTitleCase = 4,			// a title case form exists that is distinct from the upper case form
		EMirrored = 8,				// this character is replaced by a mirror-image in right-to-left text
		ENumericFlags = 0x70,		// one of these flags is set if this number has a numeric value
		ENonNumeric = 0x00,			// this character has no numeric value
		ESmallNumeric = 0x10,		// numeric in the range 0..255 (see iDigitOffset)
		EFiveHundred = 0x20,		// numeric with the value 500
		EOneThousand = 0x30,		// numeric with the value 1000
		EFiveThousand = 0x40,		// numeric with the value 5000
		ETenThousand = 0x50,		// numeric with the value 10000
		EHundredThousand = 0x60,	// numeric with the value 100000
		EFraction = 0x70			// numeric with a fractional value
		};

	TUint8 iCategory;					// general category
	TUint8 iBdCategory;					// bidirectional category
	TUint8 iCombiningClass;				// combining class
	TInt8 iDigitOffset;					// if this character has a small numeric value, the difference between the low
										// 8 bits of the character code and the numeric value
	TInt16 iCaseOffset;					// offset to other case; subtract to get upper case, add to get lower
										// case (this makes it more likely that characters
										// differing only by case have the same	data, making the table smaller)
	TUint8 iFlags;						// flags: does this character have a lower case form, etc.
	};

/*
A structure for Unicode plane information.
An array of 17 elements should be defined in unitable.cpp, which is generated
by the readtype tool. All characters in a plane are divided into blocks. All
blocks in a plane have the same block size. Block size can be 2, 4, 8, etc.
Any field in this structure can be calculated from any other field. Such
'redundant' information is just for faster runtime speed.
For example, a plane has block size of 16, which is 2 ^ 4. The code number 
will be 4. The mask for block will be 0xFFF0, which means high 12 bit indicates
block index. The mask for code point will be 0x000F, which means the lower 4
bits indicates index in block.
*/
struct TUnicodePlane
	{
	TUint8 iCodesPerBlock;			// how many bits are used to represent code points (for example if there were 4096 blocks (12 bits), this would be 4 bits)
	TUint16 iMaskForBlock;			// mask of 16 bits for blocks (for example 8 bits would be 0xff00)
	TUint16 iMaskForCodePoint;		// mask of 16 bits for index in block (for example 8 bits would be 0x00ff)
	};

/*
A structure for a range of Unicode characters with the same raw data; must not have a
constructor because an array of these in unitable.cpp is initialised as an aggregate.

@deprecated
*/
struct TUnicodeDataRange
	{
	TUint16 iRangeStart;	// Unicode value of the start of the range of characters
	TInt16 iIndex;			// index into an array of character information structures (-1 means data no available)
	};

/*
A structure to hold a set of overriding character data
*/
struct TUnicodeDataSet
	{
	const TUnicodeData *iData;			// array of character data structures
	const TUnicodeDataRange *iRange;	// array of ranges referring to elements of iData
	TInt iRanges;						// number of elements in the array of ranges
	};

// A structure to hold the standard character data
struct TStandardUnicodeDataSet
	{
	const TUint16* iIndex1;				// first trie index: 4096 elements indexed by high 12 bits of Unicode value
	const TUint16* iIndex2;				// second trie index, indexed by values in iIndex1
	const TUnicodeData *iData;			// array of character data structures, indexed by values in iIndex2, offset
										// by low 4 bits of Unicode value
	};

/*
A class to hold a Unicode character and provide functions for characterisation (e.g., is this character lowercase?)
composition (e.g., create a character from a base character and an accent), and decomposition
(e.g., remove the accent from this character if there is one).
*/
class TUnicode
	{
	public:

	// Constructors
	TUnicode() { iCode = 0xFFFF; }
	TUnicode(TUint c) : iCode(c) {}
	operator TUint() const { return iCode; }

	// Attribute retrieval (functions used by the ExecHandler class, etc., in ekern.dll take IMPORT_C)
	void GetInfo(TChar::TCharInfo& aInfo,const TUnicodeDataSet *aOverridingDataSet) const;
	IMPORT_C TChar::TCategory GetCategory(const TUnicodeDataSet *aOverridingDataSet) const;
	TChar::TBdCategory GetBdCategory(const TUnicodeDataSet *aOverridingDataSet) const;
	TInt GetCombiningClass(const TUnicodeDataSet *aOverridingDataSet) const;
	IMPORT_C TUint GetLowerCase(const TUnicodeDataSet *aOverridingDataSet) const;
	IMPORT_C TUint GetUpperCase(const TUnicodeDataSet *aOverridingDataSet) const;
	TUint GetTitleCase(const TUnicodeDataSet *aOverridingDataSet) const;
	TBool IsMirrored(const TUnicodeDataSet *aOverridingDataSet) const;
	TInt GetNumericValue(const TUnicodeDataSet *aOverridingDataSet) const;
	TChar::TCjkWidth GetCjkWidth() const;
	IMPORT_C TUint Fold(TInt aFlags,const TUnicodeDataSet *aOverridingDataSet) const;
	
	// Utilities
	static TInt Compare(const TUint16 *aString1,TInt aLength1,const TUint16 *aString2,TInt aLength2);

	private:
	const TUnicodeData& GetData(const TUnicodeDataSet *aOverridingDataSet) const;
	const TUnicodeData *GetDataFromDataSet(const TUnicodeDataSet& aDataSet) const;
	TUint GetLowerCase(const TUnicodeData& aData) const;
	TUint GetUpperCase(const TUnicodeData& aData) const;
	TUint GetTitleCase(const TUnicodeData& aData) const;
	TInt GetNumericValue(const TUnicodeData& aData) const;

	TUint iCode; // not TUint16 because values in the extended range from 0x10000 to 0xFFFFF may be used.

	public:
#ifndef __KERNEL_MODE__
	static const TUint16 FoldTable[256];		// fold table (strip accents, fold case) for the range 0..255
	static const TUint16 CjkWidthFoldTable[256];// width fold table (convert from width variants) for range 0xFF00..0xFFFF
#else
	static const TUint16* FoldTable;
	static const TUint16* CjkWidthFoldTable;
#endif
	};

#endif // __UNICODE_H__
