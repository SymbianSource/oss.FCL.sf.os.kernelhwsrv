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
// e32\euser\unicode\unicode.cpp
// The implementation of the base-level Unicode character classification functions. These are members of
// a class called TUnicode that contains a Unicode value.
// 
//

#include <unicode.h>
#include "CompareImp.h"

static const TUnicodeData TheDefaultUnicodeData =
	{ TChar::ECnCategory, TChar::EOtherNeutral, 0, 0, 0, TUnicodeData::ENonNumeric };


// Declarations for tables held in unitable.cpp and used by unicode.cpp.
#ifndef __KERNEL_MODE__
extern const TStandardUnicodeDataSet TheStandardUnicodeDataSet[];
extern const TUnicodePlane ThePlanes[17];
#endif


// Fill in a TChar::TCharInfo structure with category information about the character.
void TUnicode::GetInfo(TChar::TCharInfo& aInfo,const TUnicodeDataSet *aOverridingDataSet) const
	{
	const TUnicodeData& data = GetData(aOverridingDataSet);
	aInfo.iCategory = (TChar::TCategory)data.iCategory;
	aInfo.iBdCategory = (TChar::TBdCategory)data.iBdCategory;
	aInfo.iCombiningClass = data.iCombiningClass;
	aInfo.iLowerCase = iCode;
	aInfo.iUpperCase = iCode;
	aInfo.iTitleCase = iCode;
	if (data.iFlags & TUnicodeData::EHasLowerCase)
		aInfo.iLowerCase = GetLowerCase(data);
	if (data.iFlags & TUnicodeData::EHasUpperCase)
		aInfo.iUpperCase = GetUpperCase(data);
	if (data.iFlags & TUnicodeData::EHasTitleCase)
		aInfo.iTitleCase = GetTitleCase(data);
	aInfo.iMirrored = data.iFlags & TUnicodeData::EMirrored;
	if (data.iFlags & TUnicodeData::ENumericFlags)
		aInfo.iNumericValue = GetNumericValue(data);
	else
		aInfo.iNumericValue = -1;
	}

/*
Get the data describing a character. If "aOverridingDataSet" is non-null, look in that
data set before searching the standard data set.
*/
const TUnicodeData& TUnicode::GetData(const TUnicodeDataSet *aOverridingDataSet) const
	{
	const TUnicodeData *result = NULL;
	if (aOverridingDataSet)
		result = GetDataFromDataSet(*aOverridingDataSet);
	if (result == NULL)
		{
		if (0xFFFF >= iCode)
			{
			// optimize for BMP characters (plane 0)
			TInt index = TheStandardUnicodeDataSet[0].iIndex1[iCode >> 4];
			if (index & 0x8000) // high bit set means all values in block have the same value, and it's in the index
				index &= ~0x8000;
			else
				index = TheStandardUnicodeDataSet[0].iIndex2[index + (iCode & 0x000F)];
			return TheStandardUnicodeDataSet[0].iData[index];
			}
		else
			{
			// for non-BMP characters (plane 1-16)
			TInt plane = (iCode >> 16);
			if (plane > 16)
				{
				// for now we have no data for values above U+10FFFF
				return TheDefaultUnicodeData;
				}
			TInt codesPerBlock = ThePlanes[plane].iCodesPerBlock;
			TInt maskForCodePoint = ThePlanes[plane].iMaskForCodePoint;
			
			TInt low16bit = (iCode & 0xFFFF);
			TInt index = TheStandardUnicodeDataSet[plane].iIndex1[low16bit >> codesPerBlock];
			if (index & 0x8000) // high bit set means all values in block have the same value, and it's in the index
				index &= ~0x8000;
			else
				index = TheStandardUnicodeDataSet[plane].iIndex2[index + (low16bit & maskForCodePoint)];
			return TheStandardUnicodeDataSet[plane].iData[index];
			}
		}

	return *result;
	}

/*
Given a character data set, get the data referring to this character.
Return NULL if no data is available in this data set.
*/
const TUnicodeData *TUnicode::GetDataFromDataSet(const TUnicodeDataSet& aDataSet) const
	{
	// Perform a binary chop to find the range containing this character.
	TInt n = aDataSet.iRanges;
	const TUnicodeDataRange *base = aDataSet.iRange;
	const TUnicodeDataRange *last = base + n - 1;
	const TUnicodeDataRange *r = base;

	while (n > 1)
		{
		TInt pivot = n / 2;
		r += pivot;
		if (iCode < r->iRangeStart)									// it's before this range
			n = pivot;
		else if (r < last && iCode >= r[1].iRangeStart)				// it's after this range
			{
			base = r + 1;
			n -= pivot + 1;
			}
		else														// it's in this range
			break;
		r = base;
		}

	if (r->iIndex >= 0)
		return &aDataSet.iData[r->iIndex];		// index >= 0: data available
	else
		return NULL;							// index < 0: no data available
	}

EXPORT_C TChar::TCategory TUnicode::GetCategory(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return (TChar::TCategory)GetData(aOverridingDataSet).iCategory;
	}

TChar::TBdCategory TUnicode::GetBdCategory(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return (TChar::TBdCategory)GetData(aOverridingDataSet).iBdCategory;
	}

TInt TUnicode::GetCombiningClass(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetData(aOverridingDataSet).iCombiningClass;
	}

EXPORT_C TUint TUnicode::GetLowerCase(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetLowerCase(GetData(aOverridingDataSet));
	}

EXPORT_C TUint TUnicode::GetUpperCase(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetUpperCase(GetData(aOverridingDataSet));
	}

TUint TUnicode::GetLowerCase(const TUnicodeData& aData) const
	{
	if (aData.iFlags & TUnicodeData::EHasLowerCase)
		return iCode + aData.iCaseOffset;
	else
		return iCode;
	}

TUint TUnicode::GetUpperCase(const TUnicodeData& aData) const
	{
	if (aData.iFlags & TUnicodeData::EHasUpperCase)
		return iCode - aData.iCaseOffset;
	else
		return iCode;
	}

TUint TUnicode::GetTitleCase(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetTitleCase(GetData(aOverridingDataSet));
	}

TUint TUnicode::GetTitleCase(const TUnicodeData& aData) const
	{
	// Handle the very few characters with distinct title case variants.
	if (aData.iFlags & TUnicodeData::EHasTitleCase)
		{
		// If the character has no upper case variant add one to get the title case form.
		if (!(aData.iFlags & TUnicodeData::EHasUpperCase))
			return iCode + 1;
		// If the character has no lower case variant subtract one to get the title case form.
		if (!(aData.iFlags & TUnicodeData::EHasLowerCase))
			return iCode - 1;
		// Both upper and lower case forms exist so the character itself must be title case.
		return iCode;
		}

	// All other characters have title case forms that are the same as their upper case forms.
	return GetUpperCase(aData);
	}

TBool TUnicode::IsMirrored(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetData(aOverridingDataSet).iFlags & TUnicodeData::EMirrored;
	}

TInt TUnicode::GetNumericValue(const TUnicodeDataSet *aOverridingDataSet) const
	{
	return GetNumericValue(GetData(aOverridingDataSet));
	}

/*
Return the integer numeric value of this character.
Return -1 if the character is not numeric, or -2 if it has a fractional value.
*/
TInt TUnicode::GetNumericValue(const TUnicodeData& aData) const
	{
	switch (aData.iFlags & TUnicodeData::ENumericFlags)
		{
		case TUnicodeData::ENonNumeric: return -1;
		case TUnicodeData::ESmallNumeric: return (iCode + aData.iDigitOffset) & 0xFF;
		case TUnicodeData::EFiveHundred: return 500;
		case TUnicodeData::EOneThousand: return 1000;
		case TUnicodeData::EFiveThousand: return 5000;
		case TUnicodeData::ETenThousand: return 10000;
		case TUnicodeData::EHundredThousand: return 100000;
		case TUnicodeData::EFraction: return -2;
		default: return -1; // we should never come here
		}
	}

struct TWidthInfo
	{
	TUint iStart;
	TUint iEnd;
	TChar::TCjkWidth iWidth;
	};

static const TWidthInfo TheWidthInfoTable[] =
	{
	{ 0x0020, 0x007F, TChar::ENarrow },
	{ 0x00A2, 0x00A4, TChar::ENarrow },
	{ 0x00A5, 0x00A7, TChar::ENarrow },
	{ 0x00AF, 0x00B0, TChar::ENarrow },
	{ 0x00B1, 0x1100, TChar::ENeutralWidth },
	{ 0x1100, 0x1160, TChar::EWide },
	{ 0x1160, 0x2E80, TChar::ENeutralWidth },
	{ 0x2E80, 0xD7A4, TChar::EWide },
	{ 0xF900, 0xFA2E, TChar::EWide },
	{ 0xFE30, 0xFE6C, TChar::EWide },
	{ 0xFF01, 0xFF5F, TChar::EFullWidth },
	{ 0xFF61, 0xFFDD, TChar::EHalfWidth },
	{ 0xFFE0, 0xFFE7, TChar::EFullWidth },
	{ 0xFFE8, 0xFFEF, TChar::EHalfWidth },
	{ 0x20000, 0x2A6DF, TChar::EWide },		// CJK Unified Ideographs Extension B
	{ 0x2F800, 0x2FA1F, TChar::EWide },		// CJK Unified Ideographs Supplement
	};

const TInt TheWidthInfos = sizeof(TheWidthInfoTable) / sizeof(TheWidthInfoTable[0]);

/*
Get the notional width used by East Asian encoding systems. No check is made that the character is assigned.
No separate 'ambiguous width' is returned; ambiguous characters are treated as neutral except for those
in the CJK range, which are treated as wide. This is a big simplification, but the cost of an exhaustive table
is too great to justify at the moment.
*/
TChar::TCjkWidth TUnicode::GetCjkWidth() const
	{
	const TWidthInfo* w = TheWidthInfoTable;
	for (TInt i = 0; i < TheWidthInfos; i++, w++)
		if (iCode >= w->iStart && iCode < w->iEnd)
			return w->iWidth;
	return TChar::ENeutralWidth;
	}

/*
Convert a Unicode character into a form most likely to be equal to another character, while
still preserving the essential meaning of the character. Possible folding operations include
converting to lower case (TChar::EFoldCase), stripping accents (TChar::EFoldAccents) and others.
The flag value has a default, TChar::EFoldStandard, which performs the folding operations done
by calling Fold functions with no flags argument, and there is also TChar::EFoldAll,
which performs all possible folding operations.

Note that the difference between folding and collation is that folding is
	*	character-based
	*	biased towards yielding equality where possible
while collation is
	*	string-based
	*	designed to yield a non-equal ordering

Typically, folding will be used when searching for a match, while collation will be used when
sorting a list.
*/
EXPORT_C TUint TUnicode::Fold(TInt aFlags,const TUnicodeDataSet *aOverridingDataSet) const
	{
	TUint result = iCode;

	/*
	Fold CJK width variants. This only applies to characters 0xFF00 and above so we can use
	a built-in table.
	*/
	if (result >= 0xFF00 && (aFlags & TChar::EFoldWidth))
		result = CjkWidthFoldTable[result & 0xFF];

	/*
	If the character is <= 0x00FF and the flags include folding case and stripping accents,
	and there is no overriding character data, we can use the built-in fold table.
	*/
	const TUnicodeData* data = NULL;
	if (aOverridingDataSet)
		data = GetDataFromDataSet(*aOverridingDataSet);
	if (data == NULL && result < 256 &&
		(aFlags & (TChar::EFoldCase | TChar::EFoldAccents)) == (TChar::EFoldCase | TChar::EFoldAccents))
		return FoldTable[result];

	/*
	Other characters have to be dealt with laboriously.
	The first operations are those that, if successful, tell us that nothing more
	need be done. If a value is folded to a space or a digit or converted to Katakana
	it cannot have anything else done to it.
	*/
	if (aFlags & TChar::EFoldKana)
		{
		if ((result >= 0x3041 && result <= 0x3094) || result == 0x309D || result == 0x309E)
			return result += 0x0060;
		}
	if (data == NULL)
		data = &GetData(NULL);
	if (aFlags & TChar::EFoldSpaces)
		{
		if (data->iCategory == TChar::EZsCategory)
			return 0x0020;
		}
	if (aFlags & TChar::EFoldDigits)
		{
		TInt n = GetNumericValue(*data);
		if (n >= 0 && n <= 9)
			return 0x0030 + n;
		}

	/*
	The final operations are the relatively rare and expensive ones (after the special
	case dealt with above) of accent removal and case conversion.
	*/
	if ((aFlags & TChar::EFoldAccents) && (result < 0x2000))
		{
		/*
		Throw away characters other than the first if all are accents. For the moment these
		are defined as characters in the range 0x0300..0x0361. This definition may need
		to be modified; or I may decide to store a flag in the decomposition table indicating
		whether or not the decomposition consists of base + accent(s).
		*/
		TPtrC16 decomposition;
		if (::DecomposeChar(iCode, decomposition))
			{
			TBool all_accents = TRUE;			
			for (TInt i = 1; all_accents && i < decomposition.Length(); ++i)
				{
				if (decomposition[i] < 0x0300 || decomposition[i] > 0x0361)
					all_accents = FALSE;
				}
			if (all_accents)
				result = decomposition[0];
			}
		}

	if (aFlags & TChar::EFoldCase)
		{
		if (aOverridingDataSet == NULL && result < 256)
			result = FoldTable[result];
		else
			result = TUnicode(result).GetLowerCase(aOverridingDataSet);
		}
	
	return result;
	}

/*
Compare two Unicode strings naively by Unicode value. This is NOT the same as a comparison
of null-terminated strings; the strings can contain null characters (Unicode 0x0000) and they
compare greater than no character. This means that the string { 0x0001 0x0000 } always comes
after the string { 0x0001 }.

This function exists to make it easier to search tables of Unicode strings (like the composition
buffer) using the binary chop method. It is also used by READTYPE when sorting the compose table.

The return values are: 0 for equality, < 0 if aString1 < aString2, > 0 if aString1 > aString2.
*/
TInt TUnicode::Compare(const TUint16 *aString1,TInt aLength1,const TUint16 *aString2,TInt aLength2)
	{
	for (TInt i = 0; i < aLength1 || i < aLength2; i++, aString1++, aString2++)
		{
		TInt x = i < aLength1 ? *aString1 : -1;
		TInt y = i < aLength2 ? *aString2 : -1;
		if (x != y)
			return x - y;
		}
	return 0;
	}

