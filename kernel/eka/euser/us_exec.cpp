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
// e32\euser\us_exec.cpp
// 
//

#include "us_std.h"
#include "us_data.h"
#include <e32des8_private.h>
#include <e32kpan.h>
#include <unicode.h>
#include <videodriver.h>
#include "CompareImp.h"
#include <e32atomics.h>

#include "locmapping.h"

#ifdef __VC32__
  #pragma setlocale("english")
#endif

_LIT(KLitSpace, " ");
_LIT(KLitOpeningBracket, "(");
_LIT(KLitMinusSign, "-");
_LIT(KLitZeroPad, "0");

#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
_LIT(KFindLan, "elocl_lan.");
_LIT(KFindReg, "elocl_reg.");
_LIT(KFindCol, "elocl_col.");
_LIT(KLoc, "elocl.");
#endif

// Private use area ranges of printable/non-printable characters.
// This is a sorted list of numbers indicating the ranges in which characters
// are printable and non-printable. The elements 0, 2, 4... are the first
// characters of printable ranges and The elements 1, 3, 5... are the first
// characters of non-printable ranges
// We will assume that anything in the End User Sub-area is printable.
static const TInt PUAPrintableRanges[] =
	{
	0xE000, 0xF6D9,		// End user area + unassigned corporate use area
	0xF6DB, 0xF6DC,		// Replacement for character not in font
	0xF6DE, 0xF700,		// various EIKON and Agenda symbols
	0x10000, KMaxTInt	// everything else printable
	};

static TBool IsPUAPrintable(TInt aChar)
	{
	if (0x110000 <= aChar)
		return 0;	// non-characters not printable
	TInt i = 0;
	while (PUAPrintableRanges[i] <= aChar)
		++i;
	return i & 1;
	}




EXPORT_C TBool User::JustInTime()
/**
Tests whether just-in-time debugging is on or off.

The function is used by the Kernel, on the Emulator, to decide whether to do
just-in-time debugging for panics. The function applies to the current process.

Unless overridden by calling User::SetJustInTime(EFalse), just-in-time debugging 
is on by default.

@return True, if just-in-time debugging is on. False otherwise.
@see RProcess::JustInTime
*/
	{

	return RProcess().JustInTime();
	}




EXPORT_C void User::SetJustInTime(const TBool aBoolean)
/**
Sets just-in-time debugging for this process on or off.

While the function can be called by code running on both the Emulator and ARM,
it only has an effect on the Emulator. Turning just-in-time debugging off
prevents the debug Emulator closing down when a panic occurs.

By default, just-in-time debugging is on.

Note that the emulator handles panics in the nomal manner, i.e. by killing 
the thread.

@param aBoolean ETrue, if just-in-time debugging is to be set on. EFalse, 
                if just-in-time debugging is to be set off.
                EFalse causes _asm 3 calls to be disabled.
@see RProcess::SetJustInTime
*/
	{

	RProcess().SetJustInTime(aBoolean);
	}


extern const LCharSet* GetLocaleDefaultCharSet();
extern const LCharSet* GetLocalePreferredCharSet();

// Convert to folded.
EXPORT_C TUint User::Fold(TUint aChar)
/**
@deprecated

Folds the specified character.

Folding converts the character to a form which can be used in tolerant
comparisons without control over the operations performed. Tolerant comparisons
are those which ignore character differences like case and accents.

The result of folding a character depends on the locale and on whether this 
is a UNICODE build or not.

Note that for a non-UNICODE build, if the binary value of the character aChar
is greater than or equal to 0x100, then the character returned is the same as
the character passed to the function.

@param aChar The character to be folded.

@return The folded character.

@see TChar::Fold()
*/
	{
	// ASCII chars excluding 'i's can be handled by naive folding
	if (aChar < 0x80 && aChar != 'I')
		return (aChar >= 'A' && aChar <= 'Z') ? (aChar | 0x0020) : aChar;
	else
		return TUnicode(aChar).Fold(TChar::EFoldStandard,GetLocaleCharSet()->iCharDataSet);
	}




// Convert to a folded version, specifying the folding methods.
EXPORT_C TUint User::Fold(TUint aChar,TInt aFlags)
/**
Folds the character according to a specified folding method.

@param aChar  The character to be folded.
@param aFlags A set of flags defining the folding method. They are:

              TChar::EFoldCase, convert characters to their lower case form,
              if any;
              
              TChar::EFoldAccents, strip accents;
              
              TChar::EFoldDigits, convert digits representing values 0..9 to
              characters '0'..'9';
              
              TChar::EFoldSpaces, convert all spaces (ordinary, fixed-width,
              ideographic, etc.) to ' ';
              
              TChar::EFoldKana, convert hiragana to katakana;
              
              TChar::EFoldWidth, fold full width and half width variants to
              their standard forms;
              
              TChar::EFoldAll, use all of the above folding methods.
              
@return The folded character.
@see TChar::Fold()
*/
	{
	return TUnicode(aChar).Fold(aFlags,GetLocaleCharSet()->iCharDataSet);
	}




// Convert to collated.
EXPORT_C TUint User::Collate(TUint aChar)
/**
Converts the character to its collated form.

Collating is the process of removing differences between characters that are 
deemed unimportant for the purposes of ordering characters. The result of 
the conversion depends on the locale and on whether this is a UNICODE build 
or not.

Note that for a non UNICODE build, if the binary value of the character aChar
is greater than or equal to 0x100, then the character returned is the same as
the character passed to the function.

@param aChar The character to be folded.

@return The converted character.
*/
	{
	return TUnicode(aChar).Fold(TChar::EFoldStandard,GetLocaleCharSet()->iCharDataSet);
	}




// Convert to lower case.
EXPORT_C TUint User::LowerCase(TUint aChar)
/**
Converts the specified character to lower case.

The result of the conversion depends on the locale and on whether this is
a UNICODE build or not.

Note that for a non-UNICODE build, if the binary value of the character
aChar is greater than or equal to 0x100, then the character returned is
the same as the character passed to the function.

@param aChar The character to be converted to lower case.

@return The lower case character.
*/
	{
	// ASCII chars excluding 'i's can be handled by naive folding
	if (aChar < 0x80 && aChar != 'I')
		return (aChar >= 'A' && aChar <= 'Z') ? (aChar | 0x0020) : aChar;
	else
		return TUnicode(aChar).GetLowerCase(GetLocaleCharSet()->iCharDataSet);
	}




// Convert to upper case.
EXPORT_C TUint User::UpperCase(TUint aChar)
/**
Converts a specified character to upper case.

The result of the conversion depends on the locale and on whether this is
a UNICODE build or not.

Note that for a non UNICODE build, if the binary value of the character aChar
is greater than or equal to 0x100, then the character returned is the same as
the character passed to the function.

@param aChar The character to be converted to upper case.

@return The upper case character.
*/
	{
	// ASCII chars excluding 'i's can be handled by naive folding
	if (aChar < 0x80 && aChar != 'i')
		return (aChar >= 'a' && aChar <= 'z') ? (aChar & ~0x0020) : aChar;
	else
		return TUnicode(aChar).GetUpperCase(GetLocaleCharSet()->iCharDataSet);
	}




// Return the title case version of a character, which is the case of composite characters like Dz.
EXPORT_C TUint User::TitleCase(TUint aChar)
/**
Converts a specified character to its title case version.

@param aChar The character to be converted.

@return The converted character.
*/
	{
	return TUnicode(aChar).GetTitleCase(GetLocaleCharSet()->iCharDataSet);
	}




EXPORT_C TUint TChar::GetUpperCase() const
/**
Gets the character value after conversion to uppercase or the character's 
own value, if no uppercase form exists.

The character object itself is not changed.

@return The character value after conversion to uppercase.
*/
	{
	return User::UpperCase(iChar);
	}




EXPORT_C TUint TChar::GetLowerCase() const
/**
Gets the character value after conversion to lowercase or the character's 
own value, if no lowercase form exists.

The character object itself is not changed.

@return The character value after conversion to lowercase.
*/
	{
	return User::LowerCase(iChar);
	}




EXPORT_C TUint TChar::GetTitleCase() const
/**
Gets the character value after conversion to titlecase or the character's 
own value, if no titlecase form exists.

The titlecase form of a character is identical to its uppercase form unless 
a specific titlecase form exists.

@return The value of the character value after conversion to titlecase form.
*/
	{
	return User::TitleCase(iChar);
	}




EXPORT_C TBool TChar::IsLower() const
/**
Tests whether the character is lowercase.

@return True, if the character is lowercase; false, otherwise.
*/
	{
	return GetCategory() == TChar::ELlCategory;
	}




EXPORT_C TBool TChar::IsUpper() const
/**
Tests whether the character is uppercase.

@return True, if the character is uppercase; false, otherwise.
*/
	{
	return GetCategory() == TChar::ELuCategory;
	}



// Return TRUE if the character is title case, which is the case of composite characters like Dz.
EXPORT_C TBool TChar::IsTitle() const
/**
Tests whether this character is in titlecase.

@return True, if this character is in titlecase; false, otherwise.
*/
	{
	return GetCategory() == TChar::ELtCategory;
	}




EXPORT_C TBool TChar::IsAlpha() const
/**
Tests whether the character is alphabetic.

For Unicode, the function returns TRUE for all letters, including those from 
syllabaries and ideographic scripts. The function returns FALSE for letter-like 
characters that are in fact diacritics. Specifically, the function returns 
TRUE for categories: ELuCategory, ELtCategory, ELlCategory, and ELoCategory; 
it returns FALSE for all other categories including ELmCategory.

@return True, if the character is alphabetic; false, otherwise.

@see TChar::IsAlphaDigit()
@see TChar::TCategory
*/
	{
	return GetCategory() <= TChar::EMaxLetterOrLetterModifierCategory;
	}




EXPORT_C TBool TChar::IsDigit() const
/**
Tests whether the character is a standard decimal digit.

For Unicode, this function returns TRUE only
for the digits '0'...'9' (U+0030...U+0039), 
not for other digits in scripts like Arabic, Tamil, etc.

@return True, if the character is a standard decimal digit; false, otherwise.

@see TChar::GetCategory()
@see TChar::GetNumericValue
*/
	{
	return iChar >= '0' && iChar <= '9'; // standard decimal digits only
	}




EXPORT_C TBool TChar::IsAlphaDigit() const
/**
Tests whether the character is alphabetic or a decimal digit.

It is identical to (IsAlpha()||IsDigit()).

@return True, if the character is alphabetic or a decimal digit; false, otherwise.

@see TChar::IsAlpha()
@see TChar::IsDigit()
*/
	{
	TInt cat = (TInt)GetCategory();
	return cat <= TChar::EMaxLetterOrLetterModifierCategory ||
		   (iChar < 256 && cat == TChar::ENdCategory);	// accept any letter, but accept only standard digits
	}




EXPORT_C TBool TChar::IsHexDigit() const
/** 
Tests whether the character is a hexadecimal digit (0-9, a-f, A-F).

@return True, if the character is a hexadecimal digit; false, otherwise.
*/
	{
	/*
	The following code will actually run faster than the non-Unicode version, which needs
	to call the Exec function.
	*/
	return iChar <= 'f' && iChar >= '0' &&
		   (iChar <= '9' || iChar >= 'a' || (iChar >= 'A' && iChar <= 'F'));	// only standard hex digits will do
	}




EXPORT_C TBool TChar::IsSpace() const
/**
Tests whether the character is a white space character.

White space includes spaces, tabs and separators.

For Unicode, the function returns TRUE for all characters in the categories: 
EZsCategory, EZlCategory and EZpCategory, and also for the characters 0x0009 
(horizontal tab), 0x000A (linefeed), 0x000B (vertical tab), 0x000C (form feed), 
and 0x000D (carriage return).

@return True, if the character is white space; false, otherwise.

@see TChar::TCategory
*/
	{
	/*
	The Unicode characters 0009 .. 000D (tab, linefeed, vertical tab, formfeed, carriage return)
	have the category Cc (control); however, we want to avoid breaking traditional programs
	by getting IsSpace() to return TRUE for them.
	*/
	return (iChar <= 0x000D && iChar >= 0x0009) ||
		   (GetCategory() & 0xF0) == TChar::ESeparatorGroup;
	}




EXPORT_C TBool TChar::IsPunctuation() const
/**
Tests whether the character is a punctuation character.

For Unicode, punctuation characters are any character in the categories:
EPcCategory, EPdCategory, EPsCategory, EPeCategory, EPiCategory,
EPfCategory, EPoCategory.

@return True, if the character is punctuation; false, otherwise.

@see TChar::TCategory
*/
	{
	return (GetCategory() & 0xF0) == TChar::EPunctuationGroup;
	}




EXPORT_C TBool TChar::IsGraph() const
/**
Tests whether the character is a graphic character.

For Unicode, graphic characters include printable characters but not the space 
character. Specifically, graphic characters are any character except those 
in categories: EZsCategory,EZlCategory,EZpCategory, ECcCategory,ECfCategory,
ECsCategory, ECoCategory, and ,ECnCategory.

Note that for ISO Latin-1, all alphanumeric and punctuation characters are 
graphic.

@return True, if the character is a graphic character; false, otherwise.

@see TChar::TCategory
*/
	{
	TUint type = TUnicode(iChar).GetCategory(0);
	return type <= TChar::EMaxGraphicCategory ||
		(type == TChar::ECoCategory && IsPUAPrintable(iChar));
	}




EXPORT_C TBool TChar::IsPrint() const
/**
Tests whether the character is a printable character.

For Unicode, printable characters are any character except those in categories: 
ECcCategory, ECfCategory, ECsCategory, ECoCategory and ECnCategory.

Note that for ISO Latin-1, all alphanumeric and punctuation characters, plus 
space, are printable.

@return True, if the character is printable; false, otherwise.

@see TChar::TCategory
*/
	{
	TUint type = TUnicode(iChar).GetCategory(0);
	return type <= TChar::EMaxPrintableCategory ||
		(type == TChar::ECoCategory && IsPUAPrintable(iChar));
	}




EXPORT_C TBool TChar::IsControl() const
/**
Tests whether the character is a control character.

For Unicode, the function returns TRUE for all characters in the categories: 
ECcCategory, ECfCategory, ECsCategory, ECoCategory and ECnCategoryCc.

@return True, if the character is a control character; false, otherwise.

@see TChar::TCategory
*/
	{
	return GetCategory() == TChar::ECcCategory;
	}




EXPORT_C TBool TChar::IsAssigned() const
/**
Tests whether this character has an assigned meaning in the Unicode encoding.

All characters outside the range 0x0000 - 0xFFFF are unassigned and there 
are also many unassigned characters within the Unicode range.

Locales can change the assigned/unassigned status of characters. This means 
that the precise behaviour of this function is locale-dependent.

@return True, if this character has an assigned meaning; false, otherwise.
*/
	{
	return GetCategory() <= TChar::EMaxAssignedCategory;
	}




EXPORT_C void TChar::GetInfo(TCharInfo& aInfo) const
/** 
Gets this character;s standard category information. 

This includes everything except its CJK width and decomposition, if any.

@param aInfo On return, contains the character's standard category information.
*/
	{
	TUnicode(iChar).GetInfo(aInfo,GetLocaleCharSet()->iCharDataSet);
	}




EXPORT_C TChar::TCategory TChar::GetCategory() const
/**
Gets this character's Unicode category.

@return This character's Unicode category.
*/
	{
	//for unicode non private user area just use the default charset
	if (iChar>=0xE000 && iChar<=0xF8FF)
		return TUnicode(iChar).GetCategory(GetLocaleCharSet()->iCharDataSet);
	else
		return TUnicode(iChar).GetCategory(GetLocaleDefaultCharSet()->iCharDataSet);
	}




EXPORT_C TChar::TBdCategory TChar::GetBdCategory() const
/**
Gets the bi-directional category of a character.

For more information on the bi-directional algorithm, see Unicode Technical 
Report No. 9 available at: http://www.unicode.org/unicode/reports/tr9/.

@return The character's bi-directional category.
*/
	{
	return TUnicode(iChar).GetBdCategory(GetLocaleCharSet()->iCharDataSet);
	}




EXPORT_C TInt TChar::GetCombiningClass() const
/**
Gets this character's combining class.

Note that diacritics and other combining characters have non-zero combining 
classes.

@return The combining class.
*/
	{
	//for unicode non private user area just use the default charset
	if (iChar>=0xE000 && iChar<=0xF8FF)
		return TUnicode(iChar).GetCombiningClass(GetLocaleCharSet()->iCharDataSet);
	else
		return TUnicode(iChar).GetCombiningClass(GetLocaleDefaultCharSet()->iCharDataSet);
	}




EXPORT_C TBool TChar::IsMirrored() const
/**
Tests whether this character has the mirrored property.

Mirrored characters, like ( ) [ ] < >, change direction according to the
directionality of the surrounding characters. For example, an opening
parenthesis 'faces right' in Hebrew or Arabic, and to say that 2 < 3 you would
have to say that 3 > 2, where the '>' is, in this example, a less-than sign to
be read right-to-left.

@return True, if this character has the mirrored property; false, otherwise.
*/
	{
	return TUnicode(iChar).IsMirrored(GetLocaleCharSet()->iCharDataSet);
	}




EXPORT_C TInt TChar::GetNumericValue() const
/**
Gets the integer numeric value of this character.

Numeric values need not be in the range 0..9; the Unicode character set
includes various other numeric characters such as the Roman and Tamil numerals
for 500, 1000, etc.

@return The numeric value: -1 if the character has no integer numeric 
        value,-2 if the character has a fractional numeric value.
*/
	{
	return TUnicode(iChar).GetNumericValue(GetLocaleCharSet()->iCharDataSet);
	}




EXPORT_C TChar::TCjkWidth TChar::GetCjkWidth() const
/**
Gets the Chinese, Japanese, Korean (CJK) notional width.

Some display systems used in East Asia display characters on a grid of
fixed-width character cells like the standard MSDOS display mode.

Some characters, e.g. the Japanese katakana syllabary, take up a single
character cell and some characters, e.g., kanji, Chinese characters used in
Japanese, take up two. These are called half-width and full-width characters.
This property is fixed and cannot be overridden for particular locales.

For more information on returned widths, see Unicode Technical Report 11 on 
East Asian Width available at: http://www.unicode.org/unicode/reports/tr11/

@return The notional width of an east Asian character.
*/
	{
	return TUnicode(iChar).GetCjkWidth();
	}




/**
Composes a string of Unicode characters to produce a single character result.

For example, 0061 ('a') and 030A (combining ring above) compose to give 00E5 
('a' with ring above).

A canonical decomposition is a relationship between a string of characters -  
usually a base character and one or more diacritics - and a composed character. 
The Unicode standard requires that compliant software treats composed
characters identically with their canonical decompositions. The mappings used
by these functions are fixed and cannot be overridden for particular locales.

@param aResult If successful, the composed character value. If unsuccessful, 
               this value contains 0xFFFF.
@param aSource String of source Unicode characters.

@return True, if the compose operation is successful in combining the entire
		sequence of characters in the descriptor into a single compound
		character; false, otherwise.
*/

EXPORT_C TBool TChar::Compose(TUint& aResult,const TDesC16& aSource)
	{
	aResult = 0xFFFF;
	if(aSource.Length() > 0)
		{
		TChar combined;
		if(::CombineAsMuchAsPossible(aSource, combined) == aSource.Length())
			{
			aResult = (TUint)combined;
			return ETrue;
			}
		}
	return EFalse;
	}




/**
Maps this character to its maximal canonical decomposition.

For example, 01E1 ('a' with dot above and macron) decomposes into 0061 ('a') 
0307 (dot) and 0304 (macron).

Note that this function is used during collation, as performed by
the Mem::CompareC() function, to convert the compared strings to their maximal
canonical decompositions.

@param aResult If successful, the descriptor represents the canonical decomposition 
               of this character. If unsuccessful, the descriptor is empty.
               
@return True if decomposition is successful; false, otherwise.

@see Mem::CompareC()
@see TChar::Compose()
*/
EXPORT_C TBool TChar::Decompose(TPtrC16& aResult) const
	{
	return ::DecomposeChar(iChar, aResult);
	}




EXPORT_C TInt TFindChunk::Next(TFullName &aResult)
/**
Finds the full name of the next chunk which matches the match pattern.

@param aResult A reference to a TBuf descriptor with a defined maximum length. 
               If a matching chunk is found, its full name is set into
               this descriptor.
               If no matching chunk is found, the descriptor length is set
               to zero.
               
@return KErrNone, if a matching chunk is found;
        KErrNotFound otherwise.
*/
	{
	return NextObject(aResult,EChunk);
	}





EXPORT_C TUint8 * RChunk::Base() const
/**
Gets a pointer to the base of the chunk's reserved region.

@return A pointer to the base of the chunk's reserved region.
*/
	{

	return(Exec::ChunkBase(iHandle));
	}




EXPORT_C TInt RChunk::Size() const
/**
Gets the current size of this chunk's committed region.

@return The size of the chunk's committed region.
*/
	{

	return(Exec::ChunkSize(iHandle));
	}




EXPORT_C TInt RChunk::Bottom() const
/**
Gets the offset of the bottom of the double ended chunk's committed region 
from the base of the chunk's reserved region.

Note that the lowest valid address in a double ended chunk is the sum of the 
base of the chunk's reserved region plus the value of Bottom().

@return The offset of the bottom of the chunk's committed region from the 
        base of the chunk's reserved region.
*/
	{

	return(Exec::ChunkBottom(iHandle));
	}




EXPORT_C TInt RChunk::Top() const
/**
Gets the offset of the top of the double ended chunk's committed region 
from the base of the chunk's reserved region.

Note that the highest valid address in a double ended chunk is the the sum 
of the base of the chunk's reserved region plus the value of Top() - 1.

@return The offset of the top of the chunk's committed region from the base 
        of the chunk's reserved region.
*/
	{

	return(Exec::ChunkTop(iHandle));
	}


EXPORT_C TInt RChunk::MaxSize() const
/**
Gets the maximum size of this chunk.

This maximum size of this chunk is set when the chunk is created.

@return The maximum size of this chunk.
*/
	{

	return(Exec::ChunkMaxSize(iHandle));
	}

/**
Finds the full name of the next LDD factory object which matches the match pattern.

@param aResult A reference to a TBuf descriptor with a defined maximum length. 
               If a matching LDD factory object is found, its full name is set into
               this descriptor.
               If no matching LDD factory object is found, the descriptor length is set
               to zero.
               
@return KErrNone, if a matching LDD factory object is found;
        KErrNotFound otherwise.
*/
EXPORT_C TInt TFindLogicalDevice::Next(TFullName &aResult)
	{
	return NextObject(aResult,ELogicalDevice);
	}

/**
Finds the full name of the next PDD factory object which matches the match pattern.

@param aResult A reference to a TBuf descriptor with a defined maximum length. 
               If a matching PDD factory object is found, its full name is set into
               this descriptor.
               If no matching PDD factory object is found, the descriptor length is set
               to zero.
               
@return KErrNone, if a matching PDD factory object is found;
        KErrNotFound otherwise.
*/
EXPORT_C TInt TFindPhysicalDevice::Next(TFullName &aResult)
	{
	return NextObject(aResult,EPhysicalDevice);
	}

/**
Gets the device capabilities.

@param aDes	A descriptor into which capability's information is to be written.
*/
EXPORT_C void RDevice::GetCaps(TDes8 &aDes) const
	{

	Exec::LogicalDeviceGetCaps(iHandle,aDes);
	}

/**
Checks if a device supports a particular version.

@param aVer	The requested device version.

@return	ETrue if supported, EFalse if not.
*/
EXPORT_C TBool RDevice::QueryVersionSupported(const TVersion &aVer) const
	{

	return(Exec::LogicalDeviceQueryVersionSupported(iHandle,aVer));
	}

/**
Checks if a specified unit number, additional info and a specific PDD is supported.

@param aUnit			The requested unit number.
@param aPhysicalDevice	The requested PDD name.
@param anInfo			The additional information.

@return ETrue if supported, EFalse if not. 
*/
EXPORT_C TBool RDevice::IsAvailable(TInt aUnit, const TDesC* aPhysicalDevice, const TDesC8* anInfo) const
	{
	TInt r;
	if(aPhysicalDevice)
		{
		TBuf8<KMaxKernelName> physicalDevice;
		physicalDevice.Copy(*aPhysicalDevice);
		r = Exec::LogicalDeviceIsAvailable(iHandle,aUnit,(TDesC8*)&physicalDevice,anInfo);
		}
	else
		r = Exec::LogicalDeviceIsAvailable(iHandle,aUnit,(TDesC8*)NULL,anInfo);

	return r;
	}


/**
Queues an asynchronous request for the device driver, taking no parameters.
 
The request is handled on the kernel-side by the logical channel's
DLogicalChannelBase::Request().

Outstanding requests can be cancelled by calling DoCancel().

@param aReqNo   A number identifying the request to the logical channel. 
@param aStatus  The request status object for this request.     
*/
EXPORT_C void RBusLogicalChannel::DoRequest(TInt aReqNo,TRequestStatus &aStatus)
	{

	TAny *a[2];
	a[0]=NULL;
	a[1]=NULL;
	aStatus=KRequestPending;
	Exec::ChannelRequest(iHandle,~aReqNo,&aStatus,&a[0]);
	}




/**
Queues an asynchronous request for the device driver, taking one parameter.
 
The request is handled on the kernel-side by the logical channel's
DLogicalChannelBase::Request().

Outstanding requests can be cancelled by calling DoCancel().

@param aReqNo   A number identifying the request to the logical channel. 
@param aStatus  The request status object for this request.
@param a1       A 32-bit value passed to the kernel-side. Its meaning depends
                on the device driver requirements.           
*/
EXPORT_C void RBusLogicalChannel::DoRequest(TInt aReqNo,TRequestStatus &aStatus,TAny *a1)
	{

	TAny *a[2];
	a[0]=a1;
	a[1]=NULL;
	aStatus=KRequestPending;
	Exec::ChannelRequest(iHandle,~aReqNo,&aStatus,&a[0]);
	}




/**
Queues an asynchronous request for the device driver, taking two parameters.
 
The request is handled on the kernel-side by the logical channel's
DLogicalChannelBase::Request().

Outstanding requests can be cancelled by calling DoCancel().

@param aReqNo   A number identifying the request to the logical channel. 
@param aStatus  The request status object for this request.
@param a1       A 32-bit value passed to the kernel-side. Its meaning depends
                on the device driver requirements.           
@param a2       A 32-bit value passed to the kernel-side. Its meaning depends
                on the device driver requirements.           
*/
EXPORT_C void RBusLogicalChannel::DoRequest(TInt aReqNo,TRequestStatus &aStatus,TAny *a1,TAny *a2)
	{

	TAny *a[2];
	a[0]=a1;
	a[1]=a2;
	aStatus=KRequestPending;
	Exec::ChannelRequest(iHandle,~aReqNo,&aStatus,&a[0]);
	}




/**
Cancels one or more outstanding asynchronous requests.

All outstanding requests complete with KErrCancel.

@param aRequestMask A set of bits identifying the requests to be cancelled.
                    Each bit can be used to identify a separate outstanding
                    request. It is up to the driver to define how the bits map
                    to those outstanding requests.
*/
EXPORT_C void RBusLogicalChannel::DoCancel(TUint aRequestMask)
	{

	Exec::ChannelRequest(iHandle,KMaxTInt,(TAny*)aRequestMask,0);
	}




/**
Makes a synchronous request to the device driver, taking no parameters.

This function does not return until the request has completed, successfully
or otherwise.

@param aFunction A number identifying the request.

@return KErrNone, if successful; otherwise one of the other system-wide
        error codes.
        The value returned depends on the implementation of the device driver.
*/
EXPORT_C TInt RBusLogicalChannel::DoControl(TInt aFunction)
	{

	return Exec::ChannelRequest(iHandle,aFunction,NULL,NULL);
	}




/**
Makes a synchronous request to the device driver, taking one parameter.

This function does not return until the request has completed, successfully
or otherwise.

@param aFunction A number identifying the request.
@param a1        A 32-bit value passed to the kernel-side. Its meaning depends
                 on the device driver requirements.           

@return KErrNone, if successful; otherwise one of the other system-wide
        error codes.
        The value returned depends on the implementation of the device driver.
*/
EXPORT_C TInt RBusLogicalChannel::DoControl(TInt aFunction,TAny *a1)
	{

	return Exec::ChannelRequest(iHandle,aFunction,a1,NULL);
	}




/**
Makes a synchronous request to the device driver, taking two parameters.

This function does not return until the request has completed, successfully
or otherwise.

@param aFunction A number identifying the request.
@param a1        A 32-bit value passed to the kernel-side. Its meaning depends
                 on the device driver requirements.           
@param a2        A 32-bit value passed to the kernel-side. Its meaning depends
                 on the device driver requirements.           

@return KErrNone, if successful; otherwise one of the other system-wide
        error codes.
        The value returned depends on the implementation of the device driver.
*/
EXPORT_C TInt RBusLogicalChannel::DoControl(TInt aFunction,TAny *a1,TAny *a2)
	{

	return Exec::ChannelRequest(iHandle,aFunction,a1,a2);
	}




EXPORT_C void User::WaitForAnyRequest()
/**
Waits for any asynchronous request to complete.

The current thread waits on its request semaphore.

The function completes, and control returns to the caller when the current 
thread's request semaphore is signalled by any of the service providers which 
handle these asynchronous requests.

The request status of all outstanding asynchronous requests must be examined 
to determine which request is complete.

@see TRequestStatus
*/
	{

	Exec::WaitForAnyRequest();
	}




EXPORT_C void User::WaitForRequest(TRequestStatus &aStatus)
/**
Waits for a specific asynchronous request to complete.

The current thread waits on its request semaphore.

The function completes and control returns to the caller when the current 
thread's request semaphore is signalled by the service provider handling the 
request associated with aStatus. Before signalling, the service provider sets 
an appropriate value in aStatus, other than KRequestPending.

Note that if other asynchronous requests complete before the one associated
with aStatus, the request semaphore is adjusted so that knowledge of their
completion is not lost. In this a case, a subsequent call to
User::WaitForAnyRequest() or User::WaitForRequest() will complete and return
immediately.

@param aStatus A reference to the request status object associated with the 
               specific asynchronous request.
               
@see KRequestPending
*/
	{

	TInt i=(-1);
	do
		{
		i++;
		Exec::WaitForAnyRequest();
		} while (aStatus==KRequestPending);
	if (i)
		Exec::RequestSignal(i);
	}




EXPORT_C void User::WaitForRequest(TRequestStatus &aStatus1,TRequestStatus &aStatus2)
/**
Waits for either of two specific asynchronous requests to complete.

The current thread waits on its request semaphore.

The function completes and control returns to the caller when the current 
thread's request semaphore is signalled by either the service provider handling 
the request associated with aStatus1 or the service provider handling the 
request associated with aStatus2. Before signalling, the completing service 
provider sets an appropriate value in the status object, other
than KRequestPending.

Note that if other asynchronous requests complete before the ones associated
with aStatus1 and aStatus2, the request semaphore is adjusted so that knowledge
of their completion is not lost. In this a case, a subsequent call to
User::WaitForAnyRequest() or User::WaitForRequest() will complete and return
immediately.

@param aStatus1 A reference to the request status object associated with the 
                first specific asynchronous request.
@param aStatus2 A reference to the request status object associated with the 
                second specific asynchronous request.

@see KRequestPending                
*/
	{

	TInt i=(-1);
	do
		{
		i++;
		Exec::WaitForAnyRequest();
		} while (aStatus1==KRequestPending && aStatus2==KRequestPending);
	if (i)
		Exec::RequestSignal(i);
	}




EXPORT_C void User::WaitForNRequest(TRequestStatus * aStatusArray[], TInt aNum)
/**
 Waits for any one of  specific asynchronous requests to complete.
  
The current thread waits on its request semaphore.

The function completes and control returns to the caller when the current 
thread's request semaphore is signalled by the service provider handling 
the request associated with any member of aStatusArray[]. Before signalling, 
the completing service provider sets an appropriate value in the status object, 
other than KRequestPending.
 
Note that if other asynchronous requests complete before the ones associated
with aStatusArray the request semaphore is adjusted so that knowledge
of their completion is not lost. In this a case, a subsequent call to
User::WaitForAnyRequest() or User::WaitForRequest() will complete and return
immediately. 
@param aStatusArray[] 	An array of pointers to the request status objects
@param TInt aNum    	The size of aStatusArray[]
*/
	{
     	TRequestStatus* aptr;
     	TBool m = ETrue;
     	TInt i = (-1);
     	do
		{
	 	i++;
        	Exec::WaitForAnyRequest();
        	for(TInt j = 0; j<aNum; j++)
        		{
         		aptr =  aStatusArray[j];
         		if(aptr)
         			{
         			if(aptr->Int()!= KRequestPending)
         				{	
         				m = EFalse;	
         				break;
         				}
         			}
        		}
     		}while(m);
	if(i)
		Exec::RequestSignal(i);	
	}




EXPORT_C TInt TFindLibrary::Next(TFullName &aResult)
/**
Finds the next DLL whose full name matches the match pattern.

If a DLL with a matching name is found, the function copies the full name of
the DLL into the descriptor aResult.

@param aResult A buffer for the fullname of the DLL. This is a template
               specialisation of TBuf defining a modifiable buffer descriptor
               taking a maximum length of KMaxFullName.
               If no matching DLL is found, the descriptor length is
               set to zero. 
               
@return KErrNone, if a matching DLL is found;
        KErrNotFound, otherwise.
*/
	{
	return NextObject(aResult,ELibrary);
	}




EXPORT_C TLibraryFunction RLibrary::Lookup(TInt anOrdinal) const
/**
Gets a pointer to the function at the specified ordinal within this DLL.

@param anOrdinal The ordinal of the required function in this DLL.
                 This value must be positive.

@return A pointer to the function at position anOrdinal in this DLL.
        The value is NULL if there is no function at that ordinal. 
        
@panic USER 116 if anOrdinal is negative
*/
	{
	__ASSERT_ALWAYS(anOrdinal>=0,Panic(EBadLookupOrdinal));
	return (Exec::LibraryLookup(iHandle,anOrdinal));
	}



EXPORT_C TFileName RLibrary::FileName() const
/**
Gets the name of the DLL's file.

@return The DLL's filname.
*/
	{

	TFileName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxFileName, KMaxFileName);
	Exec::LibraryFileName(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C TUidType RLibrary::Type() const
/**
Gets this DLL's UID type.

The UID type is a property of a Symbian OS file; for a DLL, its value is set
during the building of that DLL.

@return The UID type of this DLL. Note that the first TUid component of
        the TUidType has the value KDynamicLibraryUid.
*/
	{

	TUidType u;
	Exec::LibraryType(iHandle,u);
	return(u);
	}




EXPORT_C TInt RLibrary::GetRamSizes(TInt& aCodeSize, TInt& aConstDataSize)
/**
Gets the current size of the code and the const data for this DLL.

This function can be called on a RAM loaded DLL or a ROM based DLL.

@param aCodeSize      The current size of the code for a RAM loaded DLL.
                      This is zero for a ROM based DLL.

@param aConstDataSize The current size of the const data for a RAM loaded DLL.
                      This is zero for a ROM based DLL.

@return KErrNone if successful, otherwise one of the system-wide error codes. 
*/
	{
	TModuleMemoryInfo info;
	TInt r=Exec::LibraryGetMemoryInfo(iHandle,info);
	if (r==KErrNone)
		{
		aCodeSize=info.iCodeSize;
		aConstDataSize=info.iConstDataSize;
		}
	return r;
	}




/**
Sets the home time to a specified time value.

@param aTime A reference to a time representation object containing the time 
             value.
             
@return KErrNone if successful or one of the system-wide error codes.

@deprecated Set the time using User::SetUTCTime if the UTC time is known;
			otherwise, use the timezone server to set the time.

@capability WriteDeviceData
*/
EXPORT_C TInt User::SetHomeTime(const TTime &aTime)
	{
	return(Exec::SetUTCTimeAndOffset(aTime.Int64(),0,ETimeSetTime|ETimeSetLocalTime,0));
	}

/**
Sets the secure home time to a specified time value.

@param aTime A reference to a time representation object containing the 
			 secure time value.
             
@return KErrNone if successful or one of the system-wide error codes.

@capability TCB
@capability WriteDeviceData
*/
EXPORT_C TInt User::SetHomeTimeSecure(const TTime &aTime)
	{
	return(Exec::SetUTCTimeAndOffset(aTime.Int64(),0,ETimeSetTime|ETimeSetLocalTime|ETimeSetSecure,0));
	}



/**
Sets the UTC time to a specified time value.

@param aUTCTime A reference to a time representation object containing the time 
                value.
             
@return KErrNone if successful or one of the system-wide error codes.

@capability WriteDeviceData
*/
EXPORT_C TInt User::SetUTCTime(const TTime &aUTCTime)
	{
	return(Exec::SetUTCTimeAndOffset(aUTCTime.Int64(),0,ETimeSetTime,0));
	}

/**
Sets the secure UTC time to a specified time value.

@param aUTCTime A reference to a time representation object containing the secure time 
                value.
             
@return KErrNone if successful or one of the system-wide error codes.

@capability TCB
@capability WriteDeviceData
*/
EXPORT_C TInt User::SetUTCTimeSecure(const TTime &aUTCTime)
	{
	return(Exec::SetUTCTimeAndOffset(aUTCTime.Int64(),0,ETimeSetTime|ETimeSetSecure,0));
	}

/**
Gets the UTC offset - the difference between UTC and the current local time
due to any time zones and daylight savings time that may be in effect. A positive
offset indicates a time ahead of UTC, a negative offset indicates a time behind UTC.

@return The UTC offset, in seconds.
*/
EXPORT_C TTimeIntervalSeconds User::UTCOffset()
	{
	return(TTimeIntervalSeconds(Exec::UTCOffset()));
	}


/**
Sets the UTC offset to the given number of seconds. This should include both time
zone differences and the effect of any applicable daylight savings time.
A positive offset indicates a time ahead of UTC, a negative offset indicates a time
behind UTC.

@param aOffset The UTC offset, in seconds.

@capability WriteDeviceData
*/
EXPORT_C void User::SetUTCOffset(TTimeIntervalSeconds aOffset)
	{
	Exec::SetUTCTimeAndOffset(0,aOffset.Int(),ETimeSetOffset,0);
	}


/**
Sets the UTC time and UTC offset to the specified values, atomically. This is equivalent
to calling both SetUTCTime and SetUTCOffset, but without the possibility of an incorrect
time being observed between the two calls. If the operation is not successful, an error
code will be returned and both the time and offset will be left unchanged.

@param aUTCTime A reference to a time representation object containing the time 
                value.
@param aOffset The UTC offset, in seconds.
             
@return KErrNone if successful or one of the system-wide error codes.

@capability WriteDeviceData
*/
EXPORT_C TInt User::SetUTCTimeAndOffset(const TTime &aUTCTime, TTimeIntervalSeconds aOffset)
	{
	return(Exec::SetUTCTimeAndOffset(aUTCTime.Int64(),aOffset.Int(),ETimeSetTime|ETimeSetOffset,0));
	}


/**
Gets the current tick count.

The period between ticks is usually 1/64 second, but may be hardware dependent.

@return The machine dependent tick count.
*/
EXPORT_C TUint User::TickCount()
	{

	return(Exec::TickCount());
	}




EXPORT_C TTimeIntervalSeconds User::InactivityTime()
/**
Gets the time since the last user activity.

@return The time interval.
*/
	{

	return TTimeIntervalSeconds(Exec::UserInactivityTime());
	}




/**
Resets all user inactivity timers.
*/
EXPORT_C void User::ResetInactivityTime()
	{
	Exec::ResetInactivityTime();
	}




/**
Gets the nanokernel tick count.

This is the current value of the machine's millisecond tick counter.

On the emulator the resolution defaults to 5 milliseconds; however
you can change it to N milliseconds when you launch the emulator
from the command line by specifying -Dtimerresolution=N as a parameter
to epoc.exe, for example:
@code
epoc.exe -Dtimerresolution=3
@endcode

On most hardware the resolution is about 1 millisecond.

You can get the nanokernel tick period in microseconds by calling
into the Hardware Abstraction Layer:

@code
TInt nanokernel_tick_period;
HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);
@endcode

@return The nanokernel tick count.
*/
EXPORT_C TUint32 User::NTickCount()
	{

	return Exec::NTickCount();
	}




/**
Gets the fast counter.

This is the current value of the machine's high resolution timer.  If a high
resolution timer is not available, it uses the millisecond timer instead.

The freqency of this counter can be determined by reading the HAL attribute
EFastCounterFrequency.

This function is intended for use in profiling and testing; it should not be
used in production code. User::NTickCount() should be used instead.

This is because the implementation of the FastCounter is platform-specific:
its frequency can be anywhere from a few KHz to many MHz. It may also not
be activated when needed, since it is expensive in terms of clock cycles and
battery life, and use of a platform-specific API may be necessary to enable
it.

@return The fast counter value.

@see User::NTickCount()
*/
EXPORT_C TUint32 User::FastCounter()
	{

	return Exec::FastCounter();
	}




EXPORT_C TTimerLockSpec User::LockPeriod()
/**
Returns which of the periods the clock is currently in.

@return The fraction of a second at which the timer completes.
*/
	{

	return(Exec::LockPeriod());
	}




EXPORT_C TName RHandleBase::Name() const
/**
Gets the name of the handle.

@return The name of the handle.
*/
	{

	TName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxName, KMaxName);
	Exec::HandleName(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C TFullName RHandleBase::FullName() const
/**
Gets the full name of the handle.

Note: This method is stack consuming (it takes 512 bytes on stack to execute).
For an alternative way to obtain the full name of the object, see RHandleBase::FullName(TDes& aName) const.

@see RHandleBase::FullName(TDes& aName) const
@return The full name of the handle.
*/
	{

	TFullName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxFullName, KMaxFullName);
	Exec::HandleFullName(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C void RHandleBase::FullName(TDes& aName) const
/**
Gets the full name of the handle.

@param aName On return, contains the full name of the handle.

@panic KERN-EXEC 35, If full name of the handler is longer that the maximum length of aName descriptor.
					 To avoid this, the maximum length of aName should be at least KMaxFullName.
@see KMaxFullName
*/
	{

	// Kernel will copy string in n8, whose data lives in the upper half of aName desciptor data
	TPtr8 n8(((TUint8*)aName.Ptr()) + aName.MaxLength(), aName.MaxLength());
	Exec::HandleFullName(iHandle,n8);
	aName.Copy(n8); // Expands 8bit descriptor into 16bit unicode descriptor.
	}




EXPORT_C void RHandleBase::HandleInfo(THandleInfo* anInfo)
/**
Gets information about the handle.

@param anInfo A pointer to a THandleInfo object supplied by the caller;
              on return, contains the handle information. 
*/
	{

	Exec::HandleInfo(iHandle,anInfo);
	}

EXPORT_C TInt RHandleBase::BTraceId() const
/**
Returns a unique object identifier for use with BTrace
*/
	{
	return Exec::GetBTraceId(iHandle);
	}



EXPORT_C TUint RHandleBase::Attributes() const
//
// Get handle attributes
//
	{

	return Exec::HandleAttributes(iHandle);
	}




EXPORT_C TInt User::AllocLen(const TAny *aCell)
/**
Gets the length of the specified allocated heap cell.

The cell is assumed to be in the current thread's heap.

@param aCell A pointer to the allocated cell whose length
             is to be fetched.

@return The length of the allocated cell.
*/
	{

	return(GetHeap()->AllocLen(aCell));
	}




EXPORT_C TAny* User::Alloc(TInt aSize)
/**
Allocates a cell of specified size from the current thread's heap.

If there is insufficient memory available on the heap from which to allocate a cell 
of the required size, the function returns NULL.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's 
             heap.
             
@return A pointer to the allocated cell. NULL, if there is insufficient memory 
        available.
        
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
	{

	return(GetHeap()->Alloc(aSize));
	}




EXPORT_C TAny* User::AllocL(TInt aSize)
/**
Allocates a cell of specified size from the current thread's heap, and leaves 
if there is insufficient memory in the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's 
             heap.

@return A pointer to the allocated cell.

@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
	{

	return(GetHeap()->AllocL(aSize));
	}




EXPORT_C TAny *User::AllocLC(TInt aSize)
/**
Allocates a cell of specified size from the current thread's default heap, and,
if successful, places a pointer to the cell onto the cleanup stack.

The function leaves if there is insufficient memory in the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's
             default heap.
             
@return A pointer to the allocated cell.

@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
	{

	return(GetHeap()->AllocLC(aSize));
	}




EXPORT_C TAny* User::AllocZ(TInt aSize)
/**
Allocates a cell of specified size from the current thread's default heap,
and clears it to binary zeroes.

If there is insufficient memory available on the heap from which to allocate a cell 
of the required size, the function returns NULL.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's 
             default heap.
             
@return A pointer to the allocated cell. NULL, if there is insufficient memory 
        available.
        
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
	{

	return GetHeap()->AllocZ(aSize);
	}




EXPORT_C TAny* User::AllocZL(TInt aSize)
/**
Allocates a cell of specified size from the current thread's default heap,
clears it to binary zeroes, and leaves if there is insufficient memory in
the heap.

The resulting size of the allocated cell may be rounded up to a value greater 
than aSize, but is guaranteed to be not less than aSize.

@param aSize The size of the cell to be allocated from the current thread's 
             heap.

@return A pointer to the allocated cell.

@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling Alloc(-1) raises this panic.
*/
	{

	return GetHeap()->AllocZL(aSize);
	}




EXPORT_C TInt User::Available(TInt &aBiggestBlock)
/**
Gets the total free space currently available on the current thread's 
default heap, and the space available in the largest free block.

The space available represents the total space which can be allocated.

Note that compressing the heap may reduce the total free space available and the space 
available in the largest free block.

@param aBiggestBlock On return, contains the space available in the largest
                     free block on the current thread's default heap.
                     
@return The total free space currently available on the current thread's heap.
*/
	{

	return(GetHeap()->Available(aBiggestBlock));
	}




EXPORT_C void User::Check()
/**
Checks the validity of the current thread's default heap.

The function walks through the list of allocated cells and the list of free
cells checking that the heap is consistent and complete.

@panic USER 47 if any corruption is found, specifically	a bad allocated
               heap cell size.
@panic USER 48 if any corruption is found, specifically a bad allocated
               heap cell address.
@panic USER 49 if any corruption is found, specifically a bad free heap
               cell address.
*/
	{

	GetHeap()->Check();
	}




EXPORT_C void User::Free(TAny *aCell)
/**
Frees the specified cell and returns it to the current thread's default heap.

@param aCell A pointer to a valid cell to be freed. If NULL this function 
             call will be ignored.
             
@panic USER 42, if aCell is not NULL and does not point to a valid cell.
*/
	{

	if (aCell)
		GetHeap()->Free(aCell);
	}




EXPORT_C void User::FreeZ(TAny * &aCell)
/**
Frees the specified cell, returns it to the current thread's default heap, and resets 
the pointer to NULL.

@param aCell A reference to a pointer to a valid cell to be freed. If NULL 
             this function call will be ignored.
             
@panic USER 42, if aCell is not NULL and does not point to a valid cell.             
*/
	{

	if (aCell)
		GetHeap()->FreeZ(aCell);
	}




EXPORT_C TAny* User::ReAlloc(TAny* aCell, TInt aSize, TInt aMode)
/**
Increases or decreases the size of an existing cell in the current
thread's heap.

If the cell is being decreased in size, then it is guaranteed not to move,
and the function returns the pointer originally passed in aCell. Note that the
length of the cell will be the same if the difference between the old size
and the new size is smaller than the minimum cell size.

If the cell is being increased in size, i.e. aSize is bigger than its
current size, then the function tries to grow the cell in place.
If successful, then the function returns the pointer originally
passed in aCell. If unsuccessful, then:
-# if the cell cannot be moved, i.e. aMode has the ENeverMove bit set, then
   the function returns NULL.
-# if the cell can be moved, i.e. aMode does not have the ENeverMove bit set,
   then the function tries to allocate a new replacement cell, and, if
   successful, returns a pointer to the new cell; if unsuccessful, it
   returns NULL.

Note that in debug mode, the function returns NULL if the cell cannot be grown
in place, regardless of whether the ENeverMove bit is set.

If the reallocated cell is at a different location from the original cell, then
the content of the original cell is copied to the reallocated cell.

If the supplied pointer, aCell is NULL, then the function attempts to allocate
a new cell, but only if the cell can be moved, i.e. aMode does not have
the ENeverMove bit set.

Note the following general points:
- If reallocation fails, the content of the original cell is preserved.
- The resulting size of the re-allocated cell may be rounded up to a value
  greater than aSize, but is guaranteed to be not less than aSize.
 
@param aCell A pointer to the cell to be reallocated. This may be NULL.

@param aSize The new size of the cell. This may be bigger or smaller than the
             size of the original cell. The value can also be zero, but this is
             interpreted as a request for a cell of minimum size; the net
             effect is the same as if the caller had explicitly requested
             a cell of minimum size.
             Note that the minimum size of a heap cell is device dependent.
             
@param aMode Flags controlling the reallocation. The only bit which has any
             effect on this function is that defined by the enumeration
             ENeverMove of the enum RAllocator::TReAllocMode.
             If this is set, then any successful reallocation guarantees not
             to have changed the start address of the cell.
             By default, this parameter is zero.

@return A pointer to the reallocated cell. This may be the same as the original
        pointer supplied through aCell. NULL if there is insufficient memory to
        reallocate the cell, or to grow it in place.

@panic USER 42, if aCell is not NULL, and does not point to a valid cell.
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling ReAlloc(someptr,-1) raises this panic.

@see RAllocator::TReAllocMode
*/
	{

	return GetHeap()->ReAlloc(aCell, aSize, aMode);
	}




EXPORT_C TAny* User::ReAllocL(TAny* aCell, TInt aSize, TInt aMode)
/**
Increases or decreases the size of an existing cell, and leaves 
if there is insufficient memory in the current thread's default heap.

If the cell is being decreased in size, then it is guaranteed not to move,
and the function returns the pointer originally passed in aCell. Note that the
length of the cell will be the same if the difference between the old size
and the new size is smaller than the minimum cell size.

If the cell is being increased in size, i.e. aSize is bigger than its
current size, then the function tries to grow the cell in place.
If successful, then the function returns the pointer originally
passed in aCell. If unsuccessful, then:
-# if the cell cannot be moved, i.e. aMode has the ENeverMove bit set, then
   the function leaves.
-# if the cell can be moved, i.e. aMode does not have the ENeverMove bit set,
   then the function tries to allocate a new replacement cell, and, if
   successful, returns a pointer to the new cell; if unsuccessful, it
   leaves.

Note that in debug mode, the function leaves if the cell cannot be grown
in place, regardless of whether the ENeverMove bit is set.

If the reallocated cell is at a different location from the original cell, then
the content of the original cell is copied to the reallocated cell.

If the supplied pointer, aCell is NULL, then the function attempts to allocate
a new cell, but only if the cell can be moved, i.e. aMode does not have
the ENeverMove bit set.

Note the following general points:
- If reallocation fails, the content of the original cell is preserved.
- The resulting size of the re-allocated cell may be rounded up to a value
  greater than aSize, but is guaranteed to be not less than aSize.

@param aCell A pointer to the cell to be reallocated. This may be NULL.

@param aSize The new size of the cell. This may be bigger or smaller than the
             size of the original cell. The value can also be zero, but this is
             interpreted as a request for a cell of minimum size; the net
             effect is the same as if the caller had explicitly requested
             a cell of minimum size.
             Note that the minimum size of a heap cell is device dependent.
             
@param aMode Flags controlling the reallocation. The only bit which has any
             effect on this function is that defined by the enumeration
             ENeverMove of the enum RAllocator::TReAllocMode.
             If this is set, then any successful reallocation guarantees not
             to have changed the start address of the cell.
             By default, this parameter is zero.

@return A pointer to the reallocated cell. This may be the same as the original
        pointer supplied through aCell.

@panic USER 42, if aCell is not NULL, and does not point to a valid cell.
@panic USER 47, if the maximum unsigned value of aSize is greater
                than or equal to KMaxTInt/2. For example,
                calling ReAlloc(someptr,-1) raises this panic.

@see RAllocator::TReAllocMode
*/
	{

	return GetHeap()->ReAllocL(aCell, aSize, aMode);
	}




EXPORT_C RAllocator& User::Allocator()
/**
Gets the current thread's default current heap.

@return The current heap.
*/
	{

	return *GetHeap();
	}		




EXPORT_C TInt User::AllocSize(TInt &aTotalAllocSize)
/**
Gets the total number of cells allocated on the current thread's default heap, 
and the total space allocated to them.

@param aTotalAllocSize On return, contains the total space allocated to
                       the cells.
                       
@return The number of cells currently allocated on the current thread's heap.
*/
	{

	return(GetHeap()->AllocSize(aTotalAllocSize));
	}




EXPORT_C TInt User::CountAllocCells()
/**
Gets the total number of cells allocated on the current thread's default heap.


@return The number of cells allocated on the current thread's default user heap.
*/
	{
	return(GetHeap()->Count());
	}  




EXPORT_C TInt User::CountAllocCells(TInt &aFreeCount)
/**
Gets the the total number of cells allocated, and the number of free cells, 
on the current thread's default heap.

@param aFreeCount On return, contains the number of free cells 
                  on the current thread's default heap.

@return The number of cells allocated on the current thread's default heap.
*/
	{

	return(GetHeap()->Count(aFreeCount));
	}




EXPORT_C RAllocator* User::SwitchAllocator(RAllocator* aA)
/**
Changes the current thread's heap.
	
@param aA A pointer to the new heap handle.

@return A pointer to the old heap handle.
*/
	{
	
#ifdef __USERSIDE_THREAD_DATA__
	// Just cache the pointer user-side.  We still need to let the kernel know what's going on so
	// the heap can be cleaned up correctly later.
	LocalThreadData()->iHeap=aA;
#endif
	return Exec::HeapSwitch(aA);
	}

// The suffix table
const TText16* const __DefaultDateSuffixTable[KMaxSuffixes] =
	{
	_S16("st"),_S16("nd"),_S16("rd"),_S16("th"),_S16("th"),
	_S16("th"),_S16("th"),_S16("th"),_S16("th"),_S16("th"),
	_S16("th"),_S16("th"),_S16("th"),_S16("th"),_S16("th"),
	_S16("th"),_S16("th"),_S16("th"),_S16("th"),_S16("th"),
	_S16("st"),_S16("nd"),_S16("rd"),_S16("th"),_S16("th"),
	_S16("th"),_S16("th"),_S16("th"),_S16("th"),_S16("th"),
	_S16("st")
	};

// The day names
const TText16* const __DefaultDayTable[KMaxDays] =
	{
	_S16("Monday"),
	_S16("Tuesday"),
	_S16("Wednesday"),
	_S16("Thursday"),
	_S16("Friday"),
	_S16("Saturday"),
	_S16("Sunday")
	};

// The abbreviated day names
const TText16* const __DefaultDayAbbTable[KMaxDays] =
	{
	_S16("Mon"),
	_S16("Tue"),
	_S16("Wed"),
	_S16("Thu"),
	_S16("Fri"),
	_S16("Sat"),
	_S16("Sun")
	};

// The month names
const TText16* const __DefaultMonthTable[KMaxMonths] =
	{
	_S16("January"),
	_S16("February"),
	_S16("March"),
	_S16("April"),
	_S16("May"),
	_S16("June"),
	_S16("July"),
	_S16("August"),
	_S16("September"),
	_S16("October"),
	_S16("November"),
	_S16("December")
	};

// The abbreviated month names
const TText16* const __DefaultMonthAbbTable[KMaxMonths] =
	{
	_S16("Jan"),
	_S16("Feb"),
	_S16("Mar"),
	_S16("Apr"),
	_S16("May"),
	_S16("Jun"),
	_S16("Jul"),
	_S16("Aug"),
	_S16("Sep"),
	_S16("Oct"),
	_S16("Nov"),
	_S16("Dec")
	};

// The am/pm strings
const TText16* const __DefaultAmPmTable[KMaxAmPms] =
	{
	_S16("am"),
	_S16("pm")
	};

const TText16* const __DefaultLMsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S16("Retry"),								// Button 1
	_S16("Stop"),									// Button 2
	_S16("Put the disk back"),					// Put the card back - line1
	_S16("or data will be lost"),					// Put the card back - line2
	_S16("Batteries too low"),					// Low power - line1
	_S16("Cannot complete write to disk"),		// Low power - line2
	_S16("Disk error - cannot complete write"),	// Disk error - line1
	_S16("Retry or data will be lost"),			// Disk error - line2
// SoundDriver
	_S16("Chimes"),								// Chimes
	_S16("Rings"),								// Rings
	_S16("Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S16("Internal"),								// Internal
	_S16("External(01)"),							// External(01)
	_S16("External(02)"),							// External(02)
	_S16("External(03)"),							// External(03)
	_S16("External(04)"),							// External(04)
	_S16("External(05)"),							// External(05)
	_S16("External(06)"),							// External(06)
	_S16("External(07)"),							// External(07)
	_S16("External(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S16("Socket(01)"),							// Socket(01)
	_S16("Socket(02)"),							// Socket(02)
	_S16("Socket(03)"),							// Socket(03)
	_S16("Socket(04)")							// Socket(04)
	};

LOCAL_C void LocaleLanguageGet(SLocaleLanguage& locale)
	{
	TPckg<SLocaleLanguage> localeLanguageBuf(locale);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleLanguageKey, localeLanguageBuf);
	__ASSERT_DEBUG(r == KErrNone || r == KErrNotFound, Panic(EBadLocaleParameter));
	if(r == KErrNotFound)
		{
		locale.iLanguage			= ELangEnglish;
		locale.iDateSuffixTable		= (const TText16*)__DefaultDateSuffixTable;
		locale.iDayTable			= (const TText16*)__DefaultDayTable;
		locale.iDayAbbTable			= (const TText16*)__DefaultDayAbbTable;
		locale.iMonthTable			= (const TText16*)__DefaultMonthTable;
		locale.iMonthAbbTable		= (const TText16*)__DefaultMonthAbbTable;
		locale.iAmPmTable			= (const TText16*)__DefaultAmPmTable;
		locale.iMsgTable			= (const TText16* const*)__DefaultLMsgTable;
		}
	}

LOCAL_C void LocaleSettingsGet(SLocaleLocaleSettings& locale)
	{
	TPckg<SLocaleLocaleSettings> localeSettingsBuf(locale);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleDataExtraKey, localeSettingsBuf);
	__ASSERT_DEBUG(r == KErrNone || r == KErrNotFound, Panic(EBadLocaleParameter));
	if(r == KErrNotFound)
		{
		Mem::Copy(&locale.iCurrencySymbol[0], _S16("\x00a3"), sizeof(TText16) << 2);
		locale.iLocaleExtraSettingsDllPtr = NULL;
		}
	}

LOCAL_C void LocaleTimeDateFormatGet(SLocaleTimeDateFormat& locale)
	{
	TPckg<SLocaleTimeDateFormat> localeTimeDateFormatBuf(locale);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleTimeDateFormatKey, localeTimeDateFormatBuf);
	__ASSERT_DEBUG(r == KErrNone || r == KErrNotFound, Panic(EBadLocaleParameter));
	if(r == KErrNotFound)
		{
		Mem::Copy(&locale.iShortDateFormatSpec[0], _S16("%F%*D/%*M/%Y"), sizeof(TText16) * 13);
		Mem::Copy(&locale.iLongDateFormatSpec[0], _S16("%F%*D%X %N %Y"), sizeof(TText16) * 14);
		Mem::Copy(&locale.iTimeFormatSpec[0], _S16("%F%*I:%T:%S %*A"), sizeof(TText16) * 16);
		locale.iLocaleTimeDateFormatDllPtr = NULL;
		}
	}

EXPORT_C void TDayName::Set(TDay aDay)
/**
Re-retrieves the current locale's text for the specified day of the week.

@param aDay Identifies the day of the week.

@panic USER 184, if the specified day is outside the permitted range.
*/
	{
	
	__ASSERT_ALWAYS(aDay>=EMonday && aDay<=ESunday,Panic(EBadLocaleParameter));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iDayTable))[aDay]);
	}




EXPORT_C void TDayNameAbb::Set(TDay aDay)
/**
Re-retrieves the current locale's abbreviated text for the specified day of 
the week.

@param aDay Identifies the day of the week.

@panic USER 184, if the specified day is outside the permitted range.
*/
	{

	__ASSERT_ALWAYS(aDay>=EMonday && aDay<=ESunday,Panic(EBadLocaleParameter));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iDayAbbTable))[aDay]);
	}




EXPORT_C void TMonthName::Set(TMonth aMonth)
/**
Re-retrieves the current locale's text for the specified month.

@param aMonth Identifies the month.

@panic USER 184, if the specified month is outside the permitted range.
*/
	{

	__ASSERT_ALWAYS(aMonth>=EJanuary && aMonth<=EDecember,Panic(EBadLocaleParameter));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iMonthTable))[aMonth]);
	}




EXPORT_C void TMonthNameAbb::Set(TMonth aMonth)
/**
Re-retrieves the current locale's abbreviated text for the specified month.

@param aMonth Identifies the month.

@panic USER 184, if the specified month is outside the permitted range.
*/
	{

	__ASSERT_ALWAYS(aMonth>=EJanuary && aMonth<=EDecember,Panic(EBadLocaleParameter));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iMonthAbbTable))[aMonth]);
	}




EXPORT_C void TDateSuffix::Set(TInt aSuffix)
/**
Re-retrieves the current locale's date suffix text for the specified day of 
the month.

@param aSuffix A value identifying the day of the month. The value can 
               range from 0 to 30 so that the first day of the month is
               identified by 0, the second day by 1 etc.
                   
@panic USER 69, if aDateSuffix is outside the range 0 to 30.
*/
	{

	__ASSERT_ALWAYS(aSuffix>=0 && aSuffix<KMaxSuffixes,Panic(ETLoclSuffixOutOfRange));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iDateSuffixTable))[aSuffix]);
	}




EXPORT_C void TAmPmName::Set(TAmPm aSelector)
/**
Re-retrieves the current locale's text for identifying time before or after 
noon as identified by the specified selector.

@param aSelector The am/pm selector.

@panic USER 69, if aDateSuffix is outside the range 0 to 30.
*/
	{

	__ASSERT_ALWAYS(aSelector==EAm || aSelector==EPm,Panic(ETLoclSuffixOutOfRange));
	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	Copy((reinterpret_cast<const TText* const*>(localeLanguage.iAmPmTable))[aSelector]);
	}




EXPORT_C void TCurrencySymbol::Set()
/**
Re-retrieves the current locale's currency symbol(s).
*/
	{
	SLocaleLocaleSettings locale;
	LocaleSettingsGet(locale);
	Copy(&locale.iCurrencySymbol[0]);
	}




EXPORT_C void TShortDateFormatSpec::Set()
/**
Sets the contents of the short date format specification from the system-wide 
settings.
*/
	{
	SLocaleTimeDateFormat locale;
	LocaleTimeDateFormatGet(locale);
	Copy(&locale.iShortDateFormatSpec[0]);
	}




EXPORT_C void TLongDateFormatSpec::Set()
/**
Sets the contents of the long date format specification from the system-wide 
settings.
*/
	{
	SLocaleTimeDateFormat locale;
	LocaleTimeDateFormatGet(locale);
	Copy(&locale.iLongDateFormatSpec[0]);
	}




EXPORT_C void TTimeFormatSpec::Set()
/**
Sets the contents of the time string format specification from the system-wide
settings.
*/
	{
	SLocaleTimeDateFormat locale;
	LocaleTimeDateFormatGet(locale);
	Copy(&locale.iTimeFormatSpec[0]);
	}




EXPORT_C TInt User::SetCurrencySymbol(const TDesC& aSymbol)
/**
Sets the system wide currency symbol.

On successful return from this function, a call to the Set() member function 
of a TCurrencySymbol object fetches the new currency symbol.

@capability WriteDeviceData

@param aSymbol A reference to the descriptor containing the currency symbol 
               to be set.
               
@return KErrNone if successful, otherwise one of the other system wide error codes.

@panic USER 119, if the length of aSymbol is greater than KMaxCurrencySymbol. 

@see TCurrencySymbol
@see TCurrencySymbol::Set()
@see KMaxCurrencySymbol
*/
	{

	TExtendedLocale locale;
	return locale.SetCurrencySymbol(aSymbol);
	}




EXPORT_C TLanguage User::Language()
/**
Gets the language of the current locale.

@return One of the TLanguage enumerators identifying the language of the
        current locale.
*/
	{

	SLocaleLanguage localeLanguage;
	LocaleLanguageGet(localeLanguage);
	return localeLanguage.iLanguage;
	}

EXPORT_C TRegionCode User::RegionCode()
	{
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
	TLocale locale;
	locale.Refresh();	
	return static_cast<TRegionCode>(locale.RegionCode());
#else
	return static_cast<TRegionCode>(0);
#endif
	}


EXPORT_C TLocale::TLocale()
/**
Default constructor.

It constructs the object with the system's locale settings.

A single copy of the locale information is maintained by the system. This 
copy may be refreshed under application control with TLocale::Refresh(), and 
the settings may be saved to the system with TLocale::Set(). However, the 
settings are never updated by the system apart from under application control. 
This enables applications to guarantee that consistent locale information 
is used.

@see TLocale::Refresh()
@see TLocale::Set()
*/
	{

	Refresh();
	}


const TUint8 __DefaultDateSeparator[KMaxDateSeparators] = { 0, '/', '/', 0 };
const TUint8 __DefaultTimeSeparator[KMaxTimeSeparators] = { 0, ':', ':', 0 };

void TLocale::SetDefaults()
	{
	iCountryCode = 44;
	iUniversalTimeOffset = 0;
	iDateFormat = EDateEuropean;
	iTimeFormat = ETime12;
	iCurrencySymbolPosition = ELocaleBefore;
	iCurrencySpaceBetween = EFalse;
	iCurrencyDecimalPlaces = 2;
	iNegativeCurrencyFormat = TNegativeCurrencyFormat(EFalse);
	iCurrencyTriadsAllowed = ETrue;
	iThousandsSeparator = ',';
	iDecimalSeparator = '.';
	TInt i=0;
	for(; i<KMaxDateSeparators; i++)
		iDateSeparator[i] = __DefaultDateSeparator[i];
	for(i=0; i<KMaxTimeSeparators; i++)
		iTimeSeparator[i] = __DefaultTimeSeparator[i];
	iAmPmSymbolPosition = ELocaleAfter;
	iAmPmSpaceBetween = ETrue;
	iHomeDaylightSavingZone = EDstEuropean;
	iWorkDays = 0x1f;
	iStartOfWeek = EMonday;
	iClockFormat = EClockAnalog;
	iUnitsGeneral = EUnitsImperial;
	iUnitsDistanceLong =  EUnitsImperial;
	iUnitsDistanceShort =  EUnitsImperial;
	iExtraNegativeCurrencyFormatFlags = 0;
	iLanguageDowngrade[0] = ELangNone;
	iLanguageDowngrade[1] = ELangNone;
	iLanguageDowngrade[2] = ELangNone;
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
	iRegionCode = ERegGBR;
#else
	iRegionCode = 0;
#endif
	iDigitType = EDigitTypeWestern;
	iDeviceTimeState = TDeviceTimeState(EDeviceUserTime);
	}

EXPORT_C void TLocale::Refresh()
/**
Refreshes the contents of this object with the system's locale settings.
*/
	{

	
	TPckg<TLocale> localeDataBuf(*this);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleDataKey, localeDataBuf);
	__ASSERT_DEBUG(r == KErrNone || r == KErrNotFound, Panic(EBadLocaleParameter));
	if(r == KErrNone)
		{
		iUniversalTimeOffset = Exec::UTCOffset();
		iDaylightSaving = 0;
		}
	else if(r == KErrNotFound)
			{
			SetDefaults();
			}
	}




EXPORT_C TInt TLocale::Set() const
/**
Transfers the locale settings from this object to the system. Note that
the timezone offset and daylight savings flags are ignored as setting these
through TLocale is no longer supported.

After this function has been called, other applications may use the new
settings for newly-constructed TLocale objects,
or if they use TLocale::Refresh(), to refresh their settings from
the system copy.

@capability WriteDeviceData

@return KErrNone if successful, otherwise one of the other system wide error codes.

@see TLocale::Refresh()
*/
	{
	TPckg<TLocale> localeDataBuf(*this);
	TInt r = RProperty::Set(KUidSystemCategory, KLocaleDataKey, localeDataBuf);
	if(r == KErrNone)
		{
		Exec::NotifyChanges(EChangesLocale);
		}
	return r;
	}

TInt TExtendedLocale::DoLoadLocale(const TDesC& aLocaleDllName, TLibraryFunction* aExportList)
	{
	RLoader loader;
	TInt r = loader.LoadLocale(aLocaleDllName, aExportList);
	return r;
	}

#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
void TExtendedLocale::DoUpdateLanguageSettingsV2(TLibraryFunction* aExportList)
	{
	iLocale.iDigitType = EDigitTypeWestern;
	iLocale.iLanguageDowngrade[0] = ELangNone;
	iLocale.iLanguageDowngrade[1] = ELangNone;
	iLocale.iLanguageDowngrade[2] = ELangNone;
	
	iLanguageSettings.iLanguage = (TLanguage)aExportList[FnLanguageV2]();
	iLanguageSettings.iDateSuffixTable = (const TText*)aExportList[FnDateSuffixTableV2]();
	iLanguageSettings.iDayTable = (const TText*)aExportList[FnDayTableV2]();
	iLanguageSettings.iDayAbbTable = (const TText*)aExportList[FnDayAbbTableV2]();
	iLanguageSettings.iMonthTable = (const TText*)aExportList[FnMonthTableV2]();
	iLanguageSettings.iMonthAbbTable = (const TText*)aExportList[FnMonthAbbTableV2]();
	iLanguageSettings.iAmPmTable = (const TText*)aExportList[FnAmPmTableV2]();
	iLanguageSettings.iMsgTable = (const TText16* const*)aExportList[FnMsgTableV2]();
	
	TDigitType digitType = (TDigitType)aExportList[FnDigitTypeV2]();	
	iLocale.SetDigitType(digitType);

	TLanguage* languageDowngrade = (TLanguage*)aExportList[FnLanguageDowngradeTableV2]();
	iLocale.SetLanguageDowngrade(0,*(languageDowngrade));
	iLocale.SetLanguageDowngrade(1,*(languageDowngrade+1));
	iLocale.SetLanguageDowngrade(2,*(languageDowngrade+2));
	}

void TExtendedLocale::DoUpdateLocaleSettingsV2(TLibraryFunction* aExportList)
	{
	
	Mem::Copy(&iLocaleExtraSettings.iCurrencySymbol[0], (const TAny*)aExportList[FnCurrencySymbolV2](), sizeof(TText) * (KMaxCurrencySymbol+1));
	iLocaleExtraSettings.iLocaleExtraSettingsDllPtr = (TAny*)aExportList[FnCurrencySymbolV2]();
	Mem::Copy(&iLocaleTimeDateFormat.iShortDateFormatSpec[0], (const TAny*)aExportList[FnShortDateFormatSpecV2](), sizeof(TText) * (KMaxShortDateFormatSpec+1));
	Mem::Copy(&iLocaleTimeDateFormat.iLongDateFormatSpec[0], (const TAny*)aExportList[FnLongDateFormatSpecV2](), sizeof(TText) * (KMaxLongDateFormatSpec+1)) ;
	Mem::Copy(&iLocaleTimeDateFormat.iTimeFormatSpec[0], (const TAny*)aExportList[FnTimeFormatSpecV2](), sizeof(TText) * (KMaxTimeFormatSpec+1));
	iLocaleTimeDateFormat.iLocaleTimeDateFormatDllPtr = (TAny*)aExportList[FnCurrencySymbolV2]();
	
	iLocale.iExtraNegativeCurrencyFormatFlags=0x80000000;
	
	typedef void (*TLibFn)(TLocale*);
	((TLibFn)aExportList[FnLocaleDataV2])(&iLocale);
	
	if (iLocale.iExtraNegativeCurrencyFormatFlags&0x80000000)
		iLocale.iExtraNegativeCurrencyFormatFlags=0;		
	}
#endif
	
void TExtendedLocale::DoUpdateLanguageSettings(TLibraryFunction* aExportList)
	{
	iLanguageSettings.iLanguage = (TLanguage)aExportList[FnLanguage]();
	iLanguageSettings.iDateSuffixTable = (const TText*)aExportList[FnDateSuffixTable]();
	iLanguageSettings.iDayTable = (const TText*)aExportList[FnDayTable]();
	iLanguageSettings.iDayAbbTable = (const TText*)aExportList[FnDayAbbTable]();
	iLanguageSettings.iMonthTable = (const TText*)aExportList[FnMonthTable]();
	iLanguageSettings.iMonthAbbTable = (const TText*)aExportList[FnMonthAbbTable]();
	iLanguageSettings.iAmPmTable = (const TText*)aExportList[FnAmPmTable]();
	iLanguageSettings.iMsgTable = (const TText16* const*)aExportList[FnMsgTable]();
	}

void TExtendedLocale::DoUpdateLocaleSettings(TLibraryFunction* aExportList)
	{
	Mem::Copy(&iLocaleExtraSettings.iCurrencySymbol[0], (const TAny*)aExportList[FnCurrencySymbol](), sizeof(TText) * (KMaxCurrencySymbol+1));
	iLocaleExtraSettings.iLocaleExtraSettingsDllPtr = (TAny*)aExportList[FnDateSuffixTable]();
	}

void TExtendedLocale::DoUpdateTimeDateFormat(TLibraryFunction* aExportList)
	{
	Mem::Copy(&iLocaleTimeDateFormat.iShortDateFormatSpec[0], (const TAny*)aExportList[FnShortDateFormatSpec](), sizeof(TText) * (KMaxShortDateFormatSpec+1));
	Mem::Copy(&iLocaleTimeDateFormat.iLongDateFormatSpec[0], (const TAny*)aExportList[FnLongDateFormatSpec](), sizeof(TText) * (KMaxLongDateFormatSpec+1)) ;
	Mem::Copy(&iLocaleTimeDateFormat.iTimeFormatSpec[0], (const TAny*)aExportList[FnTimeFormatSpec](), sizeof(TText) * (KMaxTimeFormatSpec+1));
	iLocaleTimeDateFormat.iLocaleTimeDateFormatDllPtr = (TAny*)aExportList[FnDateSuffixTable]();
	}

/**
Default constructor.

It constructs an empty object

This is an empty copy of TExtendedLocale. To get the system locale you can
use TExtendedLocale::LoadSystemSettings. The current settings may be saved to the system
with TLocale::SaveSystemSettings().

@see TExtendedLocale::LoadSystemSettings
@see TExtendedLocale::SaveSystemSettings
*/
EXPORT_C TExtendedLocale::TExtendedLocale()
	: iLocale(0)
	{

	Mem::FillZ(&iLanguageSettings, sizeof(TExtendedLocale) - sizeof(TLocale));
	}

/**
Load system wide locale settings

It initialises this TExtendedLocale with the system wide locale settings.
The settings stored in the TExtendedLocale are overwritten with the system
wide locale.

@see TExtendedLocale::SaveSystemSettings
*/
EXPORT_C void TExtendedLocale::LoadSystemSettings()
	{
	LocaleLanguageGet(iLanguageSettings);
	LocaleSettingsGet(iLocaleExtraSettings);
	LocaleTimeDateFormatGet(iLocaleTimeDateFormat);
	iDefaultCharSet = GetLocaleCharSet();
	iPreferredCharSet = GetLocalePreferredCharSet();
	iLocale.Refresh();
	}

/**
Make the current locale information system wide

It overwrites the system wide locale information with the locale information
stored in this TExtendedLocale.
This will generate a notification for system locale changes.
In case of an error, the locale might be in an unconsistent state.

@capability WriteDeviceData

@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
EXPORT_C TInt TExtendedLocale::SaveSystemSettings()
	{

	TPckg<SLocaleLanguage> localeLanguageBuf(iLanguageSettings);
	TInt r = RProperty::Set(KUidSystemCategory, KLocaleLanguageKey, localeLanguageBuf);
	if(r != KErrNone)
		return r;

	TPckg<SLocaleLocaleSettings> localeSettingsBuf(iLocaleExtraSettings);
	r = RProperty::Set(KUidSystemCategory, KLocaleDataExtraKey, localeSettingsBuf);
	if(r != KErrNone)
		return r;

	TPckg<SLocaleTimeDateFormat> localeTimeDateFormatBuf(iLocaleTimeDateFormat);
	r = RProperty::Set(KUidSystemCategory, KLocaleTimeDateFormatKey, localeTimeDateFormatBuf);
	if(r != KErrNone)
		return r;

	r = Exec::SetGlobalUserData(ELocaleDefaultCharSet, (TInt)iDefaultCharSet);
	if(r != KErrNone)
		return r;

	r = Exec::SetGlobalUserData(ELocalePreferredCharSet, (TInt)iPreferredCharSet);

	if(r == KErrNone)
		{
		iLocale.Set();
		}

	return r;
	}

#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
TInt TExtendedLocale::CheckLocaleDllName(const TDesC& aLocaleDllName, TInt& languageID)
	{
	languageID = 0;
	
	if(aLocaleDllName.Find(KLoc) == KErrNotFound)
		return KErrNotFound;	

	TInt len = aLocaleDllName.Length() - 6;  //6 is the length of KLoc.
	TPtrC ptr = aLocaleDllName.Right(len);
	for(TInt i =0; i< len; i++)
		{
		if(ptr[i] >= '0' && ptr[i] <= '9')
			{
			languageID = languageID*10 + (ptr[i] - '0');
			}
		else
			{
			languageID = 0;
			return KErrNotFound;
			}
		}
	return KErrNone;	
	}

//add file extension, such as "elocl_lan" will be "elocl_lan.012"
void TExtendedLocale::AddExtension(TDes& aFileName, TInt aExtension)
	{			
	if (aExtension < 10)
		{
		aFileName.AppendNum(0);
		aFileName.AppendNum(0);
		aFileName.AppendNum(aExtension);
		}
	else if (aExtension < 100)
		{
		aFileName.AppendNum(0);
		aFileName.AppendNum(aExtension);
		}
	else
		{
		aFileName.AppendNum(aExtension);
		}	
	return;	
	}
#endif
	
/**
Loads a locale Dll and get the locale information

It loads a locale DLL and it initialises the contents of this TExtendedLocale
with the locale information stored in the DLL. The locale information is only
stored in this TExtendedLocale. If you want to set the system wide settings with
the locale information in the DLL, you can call TExtendedLocale::SaveSystemSettings
after calling this function.

@param aLocaleDllName The name of the locale DLL to be loaded
@return KErrNone if successful, KErrNotSupported if the DLL name matches the pattern
of a new-style locale library, system wide error otherwise

@see TExtendedLocale::SaveSystemSettings 
*/
EXPORT_C TInt TExtendedLocale::LoadLocale(const TDesC& aLocaleDllName)
	{
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
	if (aLocaleDllName.Find(KFindLan) != KErrNotFound || aLocaleDllName.Find(KFindReg) != KErrNotFound || aLocaleDllName.Find(KFindCol) != KErrNotFound)
		{
		return KErrNotSupported; // Only try to load old-style locale libraries
		}
	TLibraryFunction data[KNumLocaleExports];
	TInt r = DoLoadLocale(aLocaleDllName, &data[0]);
	if(r == KErrNone)
		{
		iLocale.iExtraNegativeCurrencyFormatFlags=0x80000000;
	  	iLocale.iLanguageDowngrade[0] = ELangNone;
		iLocale.iLanguageDowngrade[1] = ELangNone;
  		iLocale.iLanguageDowngrade[2] = ELangNone;
	  	iLocale.iDigitType = EDigitTypeWestern;

		typedef void (*TLibFn)(TLocale*);
		((TLibFn)data[FnLocaleData])(&iLocale);

		//Locale daylightsavings unchanged - we have travelled through space, not time
		if (iLocale.iExtraNegativeCurrencyFormatFlags&0x80000000)
			iLocale.iExtraNegativeCurrencyFormatFlags=0;		
		
		DoUpdateLanguageSettings(&data[0]);
		DoUpdateLocaleSettings(&data[0]);
		DoUpdateTimeDateFormat(&data[0]);

		iPreferredCharSet = (const LCharSet*)data[FnCharSet]();
		iDefaultCharSet = iPreferredCharSet;
		return r;
		}
	else if(r == KErrNotFound)
	    {
	    TInt lan = 0;
		TInt reg = 0;
		TInt col = 0;
	     TInt languageID = -1;
	     TInt err = CheckLocaleDllName(aLocaleDllName, languageID);
	     if (err != KErrNone)
	         return err;
	      
	     TInt i = 0;
	     while (i < KLocMapLength)  //binary search later
	         {
	         if ((LocaleMapping[i].iOldLocaleId) == languageID)
	             {
	             lan = LocaleMapping[i].iNewLocaleID[0];
	             reg = LocaleMapping[i].iNewLocaleID[1];
	             col = LocaleMapping[i].iNewLocaleID[2];
	             break;
	             }   
	         i++;
	         }
	     if(i == KLocMapLength)
	        return KErrNotFound;
	     
	     TBuf<15> lanptr = KFindLan();
	     TBuf<15> regptr = KFindReg();
	     TBuf<15> colptr = KFindCol();   
	     AddExtension(lanptr, lan);
	     AddExtension(regptr, reg);
	     AddExtension(colptr, col);  
	     err = LoadLocale(lanptr, regptr, colptr);   
	     
	     return err; 
	    }
	return r;
#else
	TLibraryFunction data[KNumLocaleExports];
	TInt r = DoLoadLocale(aLocaleDllName, &data[0]);
	if(r == KErrNone)
		{
		iLocale.iExtraNegativeCurrencyFormatFlags=0x80000000;
	  	iLocale.iLanguageDowngrade[0] = ELangNone;
		iLocale.iLanguageDowngrade[1] = ELangNone;
  		iLocale.iLanguageDowngrade[2] = ELangNone;
	  	iLocale.iDigitType = EDigitTypeWestern;

		typedef void (*TLibFn)(TLocale*);
		((TLibFn)data[FnLocaleData])(&iLocale);

		//Locale daylightsavings unchanged - we have travelled through space, not time
		if (iLocale.iExtraNegativeCurrencyFormatFlags&0x80000000)
			iLocale.iExtraNegativeCurrencyFormatFlags=0;		
		
		DoUpdateLanguageSettings(&data[0]);
		DoUpdateLocaleSettings(&data[0]);
		DoUpdateTimeDateFormat(&data[0]);

		iPreferredCharSet = (const LCharSet*)data[FnCharSet]();
		iDefaultCharSet = iPreferredCharSet;
		}
	return r;
#endif
	}

/**
Loads locale data from three locale dlls, which are language, region, and collation locale dlls

It loads three locale DLLs and it initialises the contents of this TExtendedLocale
with the locale information stored in the DLLs. The locale information is only
stored in this TExtendedLocale. If you want to set the system wide settings with
the locale information in the DLL, you can call TExtendedLocale::SaveSystemSettings
after calling this function.

If the function fails then it will call LoadSystemSettings() to return its members
to a known, good state.

@param aLanguageLocaleDllName The name of the language locale DLL to be loaded
@param aRegionLocaleDllName The name of the region locale DLL to be loaded
@param aCollationLocaleDllName The name of the collation locale DLL to be loaded

@return KErrNone if successful, system wide error if not

@see TExtendedLocale::SaveSystemSettings 
*/
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
EXPORT_C TInt TExtendedLocale::LoadLocale(const TDesC& aLanguageLocaleDllName, 
		const TDesC& aRegionLocaleDllName, 
		const TDesC& aCollationLocaleDllName)
	{
	TInt err = LoadLocaleAspect(aLanguageLocaleDllName);

	if(err == KErrNone)
		err = LoadLocaleAspect(aRegionLocaleDllName);

	if(err == KErrNone)
		err = LoadLocaleAspect(aCollationLocaleDllName);

	if(err != KErrNone)
		LoadSystemSettings();	

	return err;	
	}
#else
EXPORT_C TInt TExtendedLocale::LoadLocale(const TDesC& /*aLanguageLocaleDllName*/, 
        const TDesC& /*aRegionLocaleDllName*/, 
        const TDesC& /*aCollationLocaleDllName*/)
    {
    return KErrNotSupported;
    }
#endif

/**
Loads a DLL and get some locale information

It loads the specified locale DLL and depending on the aAspectGroup it overwrites
locale information in this TExtendedLocale with the locale information stored in the
DLL. aAspectGroup is a bitmap of TLocaleAspect values specifying what to be overwritten.
The locale information is only stored in this TExtendedLocale. If you want to set the
system wide settings with the locale information in the DLL, you can call
TExtendedLocale::SaveSystemSettings after calling this function.

@param aAspectGroup A bitmap of TLocaleAspect values specifying what to be overwritten in
					this TExtendedLocale. (eg.: ELocaleLanguageSettings | ELocaleTimeDateSettings)
@param aLocaleDllName The name of the locale DLL to be loaded

@return KErrNone if the method is successful, a system wide error code if not

@see TLocaleAspect
@see TExtendedLocale::SaveSystemSettings
*/
EXPORT_C TInt TExtendedLocale::LoadLocaleAspect(TUint aAspectGroup, const TDesC& aLocaleDllName)
	{
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
	TLibraryFunction data[KNumLocaleExports];
	TInt r = DoLoadLocale(aLocaleDllName, &data[0]);
	if(r == KErrNone)
		{
		if(aAspectGroup & ELocaleLanguageSettings)
			{
			DoUpdateLanguageSettings(&data[0]);
			}
		if(aAspectGroup & ELocaleCollateSetting)
			{
			iPreferredCharSet = (const LCharSet*)data[FnCharSet]();
			iDefaultCharSet = iPreferredCharSet;
			}
		if(aAspectGroup & ELocaleLocaleSettings)
			{
			DoUpdateLocaleSettings(&data[0]);
			}
		if(aAspectGroup & ELocaleTimeDateSettings)
			{
			DoUpdateTimeDateFormat(&data[0]);
			}
		return r;
		}
	
	else if (r == KErrNotFound)
	    {
	    TInt lan = 0;
		TInt reg = 0;
		TInt col = 0;
	    TInt languageID = -1;
	    TInt err = CheckLocaleDllName(aLocaleDllName, languageID);
	    if(err != KErrNone)
	        return err;
	    
	    TInt i = 0;
	    while (i < KLocMapLength)
	        {
	        if ((LocaleMapping[i].iOldLocaleId) == languageID)
	            {
	            lan = LocaleMapping[i].iNewLocaleID[0];
	            reg = LocaleMapping[i].iNewLocaleID[1];
	            col = LocaleMapping[i].iNewLocaleID[2];
	            break;
	            }   
	        i++;
	        }
	    if(i == KLocMapLength)
	        return KErrNotFound;
	    
	    TBuf<15> lanptr = KFindLan();
	    TBuf<15> regptr = KFindReg();
	    TBuf<15> colptr = KFindCol();   
	    AddExtension(lanptr, lan);
	    AddExtension(regptr, reg);
	    AddExtension(colptr, col);  
	    
	    switch (aAspectGroup)
	        {
	        case ELocaleCollateSetting:
	            {
	            err = LoadLocaleAspect(colptr);
	            break;          
	            }
	        case ELocaleLocaleSettings:
	            {
	            err = LoadLocaleAspect(regptr);
	            break;
	            }
	            
	        case ELocaleLanguageSettings:
	            {
	            err = LoadLocaleAspect(lanptr);
	            break;
	            }       
	        }
	    return err;
	    }
	
	return r;
#else
	TLibraryFunction data[KNumLocaleExports];
	TInt r = DoLoadLocale(aLocaleDllName, &data[0]);
	if(r == KErrNone)
		{
		if(aAspectGroup & ELocaleLanguageSettings)
			{
			DoUpdateLanguageSettings(&data[0]);
			}
		if(aAspectGroup & ELocaleCollateSetting)
			{
			iPreferredCharSet = (const LCharSet*)data[FnCharSet]();
			iDefaultCharSet = iPreferredCharSet;
			}
		if(aAspectGroup & ELocaleLocaleSettings)
			{
			DoUpdateLocaleSettings(&data[0]);
			}
		if(aAspectGroup & ELocaleTimeDateSettings)
			{
			DoUpdateTimeDateFormat(&data[0]);
			}
		}
	return r;
#endif
	}

/**
Loads a DLL and get some locale information

It loads the specified locale DLL, and it overwrites
locale information in this TExtendedLocale with the locale information stored in the
DLL. The locale information is only stored in this TExtendedLocale. If you want to set the
system wide settings with the locale information in the DLL, you can call
TExtendedLocale::SaveSystemSettings after calling this function.

@param aLocaleDllName The name of the locale DLL to be loaded

@return KErrNone if the method is successful, a system wide error code if not

@see TExtendedLocale::SaveSystemSettings
*/
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
EXPORT_C TInt TExtendedLocale::LoadLocaleAspect(const TDesC& aLocaleDllName)
	{
	TLibraryFunction data[KNumLocaleExports];
	
	TInt result = aLocaleDllName.Find(KFindReg);
	if(result != KErrNotFound)
		{
		result = DoLoadLocale(aLocaleDllName, &data[0]);
		if(result == KErrNone)
			{
			DoUpdateLocaleSettingsV2(&data[0]);
			return result;
			}
		}
	
	result= aLocaleDllName.Find(KFindLan);
	if(result != KErrNotFound)
		{
		result = DoLoadLocale(aLocaleDllName, &data[0]);
		if(result == KErrNone)
			{
			DoUpdateLanguageSettingsV2(&data[0]);	
			return result;
			}
		}
	
	result = aLocaleDllName.Find(KFindCol);
	if(result != KErrNotFound)
		{
		result = DoLoadLocale(aLocaleDllName, &data[0]);
		if(result == KErrNone)
			{
			iPreferredCharSet = (const LCharSet*)data[1]();
			iDefaultCharSet = iPreferredCharSet;
			return result;
			}
		}
	
	return KErrNotFound;	
	}
#else
EXPORT_C TInt TExtendedLocale::LoadLocaleAspect(const TDesC& /*aLocaleDllName*/)
    {
    return KErrNotSupported;    
    }	
#endif

/**
Sets the currency symbol

It sets the currency symbol. The maximum lenght for the currency symbol is
KMaxCurrencySymbol. Trying to pass a descriptor longer than that, will result
in a panic.

@param aSymbol The new currency symbol

@panic USER 119, if the length of aSymbol is greater than KMaxCurrencySymbol.

@capability WriteDeviceData

@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
EXPORT_C TInt TExtendedLocale::SetCurrencySymbol(const TDesC &aSymbol)
	{
	__ASSERT_ALWAYS(aSymbol.Length()<=KMaxCurrencySymbol,::Panic(ECurrencySymbolOverflow));
	
	LocaleSettingsGet(iLocaleExtraSettings);
	Mem::Copy(&iLocaleExtraSettings.iCurrencySymbol[0], aSymbol.Ptr(), aSymbol.Length()*sizeof(TText) );    
	iLocaleExtraSettings.iCurrencySymbol[aSymbol.Length()] = 0;
	TInt r = RProperty::Set(KUidSystemCategory, KLocaleDataExtraKey, TPckg<SLocaleLocaleSettings>(iLocaleExtraSettings));
	return r;
	}

/**
Returns the name of the DLL containing the given bits of locale information

Given the bits of locale information specified in aLocaleDataSet, it returns the name
of the locale DLL storing the information. TExtendedLocale can contain information from
different DLLs.

@param aLocaleDataSet The TLocaleAspect specifying a group of locale properties
@param aDllName The descriptor that will contain the name of DLL containing the specifying
				bits of locale information (valid if the method is successful)

@return KErrNone if successful, system wide error otherwise
*/
EXPORT_C TInt TExtendedLocale::GetLocaleDllName(TLocaleAspect aLocaleDataSet, TDes& aDllName)
	{
 	TBuf8<KMaxFullName> buf;
	TAny* ptr = 0;
	switch(aLocaleDataSet)
		{
		case ELocaleLanguageSettings:
			ptr = (TAny*)iLanguageSettings.iDateSuffixTable;
			break;
		case ELocaleCollateSetting:
			ptr = (TAny*)iPreferredCharSet;
			break;
		case ELocaleLocaleSettings:
			ptr = (TAny*)iLocaleExtraSettings.iLocaleExtraSettingsDllPtr;
			break;
		case ELocaleTimeDateSettings:
			ptr = (TAny*)iLocaleTimeDateFormat.iLocaleTimeDateFormatDllPtr;
			break;
		}
 	TInt r = Exec::GetModuleNameFromAddress(ptr, buf);
 	if (r == KErrNone)
		{
 		aDllName.Copy(buf);
 		}
 	return r;
	}

/**
Get the Currency Symbol from SLocaleLocaleSettings object

@return TPtrC Pointer holding the Currency Symbol
*/
EXPORT_C TPtrC TExtendedLocale::GetCurrencySymbol()
	{
	TPtrC outCurrencySymbolPtr(iLocaleExtraSettings.iCurrencySymbol);
	return outCurrencySymbolPtr;
	}
	
/**
Get the Long Date Format from SLocaleTimeDateFormat object

@return TPtrC Pointer holding the Long Date Format
*/
EXPORT_C TPtrC TExtendedLocale::GetLongDateFormatSpec()
	{
	TPtrC outLongDateFormatPtr(iLocaleTimeDateFormat.iLongDateFormatSpec);
	return outLongDateFormatPtr;
	}
	
/**
Get the Short Date Format from SLocaleTimeDateFormat object

@return TPtrC Pointer holding the Short Date Format
*/
EXPORT_C TPtrC TExtendedLocale::GetShortDateFormatSpec()
	{
	TPtrC outShortDateFormatPtr(iLocaleTimeDateFormat.iShortDateFormatSpec);
	return outShortDateFormatPtr;
	}
	
/**
Get the Time Format from SLocaleTimeDateFormat object

@return TPtrC Pointer holding the Time Format
*/
EXPORT_C TPtrC TExtendedLocale::GetTimeFormatSpec()
	{
	TPtrC outTimeFormatPtr(iLocaleTimeDateFormat.iTimeFormatSpec);
	return outTimeFormatPtr;
	}

EXPORT_C TInt UserSvr::LocalePropertiesSetDefaults()
	{
	_LIT_SECURITY_POLICY_C1(KLocaleWritePolicy,ECapabilityWriteDeviceData);
	_LIT_SECURITY_POLICY_PASS(KLocaleReadPolicy);

	TInt r = RProperty::Define(KUidSystemCategory, KLocaleLanguageKey, RProperty::EByteArray, KLocaleReadPolicy, KLocaleWritePolicy, sizeof(TPckg<SLocaleLanguage>));
	if(r != KErrNone && r != KErrAlreadyExists)
		return r;
	
	SLocaleLanguage localeLanguage;
	localeLanguage.iLanguage			= ELangEnglish;
	localeLanguage.iDateSuffixTable		= (const TText16*)__DefaultDateSuffixTable;
	localeLanguage.iDayTable			= (const TText16*)__DefaultDayTable;
	localeLanguage.iDayAbbTable			= (const TText16*)__DefaultDayAbbTable;
	localeLanguage.iMonthTable			= (const TText16*)__DefaultMonthTable;
	localeLanguage.iMonthAbbTable		= (const TText16*)__DefaultMonthAbbTable;
	localeLanguage.iAmPmTable			= (const TText16*)__DefaultAmPmTable;
	localeLanguage.iMsgTable			= (const TText16* const*)__DefaultLMsgTable;
	r = RProperty::Set(KUidSystemCategory, KLocaleLanguageKey, TPckg<SLocaleLanguage>(localeLanguage));
	if(r != KErrNone)
		return r;

	r = RProperty::Define(KUidSystemCategory, KLocaleDataKey, RProperty::EByteArray, KLocaleReadPolicy, KLocaleWritePolicy, sizeof(TPckg<TLocale>));
	if(r != KErrNone && r != KErrAlreadyExists)
		return r;

	TLocale locale(0);
	locale.SetDefaults();

	r = RProperty::Set(KUidSystemCategory, KLocaleDataKey, TPckg<TLocale>(locale));
	if(r != KErrNone)
		return r;

	r = RProperty::Define(KUidSystemCategory, KLocaleDataExtraKey, RProperty::EByteArray, KLocaleReadPolicy, KLocaleWritePolicy, sizeof(TPckg<SLocaleLocaleSettings>));
	if(r != KErrNone && r != KErrAlreadyExists)
		return r;

	SLocaleLocaleSettings localeSettings;
	Mem::Copy(&localeSettings.iCurrencySymbol[0], _S16("\x00a3"), sizeof(TText16) * 2);

	r = RProperty::Set(KUidSystemCategory, KLocaleDataExtraKey, TPckg<SLocaleLocaleSettings>(localeSettings));
	if(r != KErrNone)
		return r;

	r = RProperty::Define(KUidSystemCategory, KLocaleTimeDateFormatKey, RProperty::EByteArray, KLocaleReadPolicy, KLocaleWritePolicy, sizeof(TPckg<SLocaleLocaleSettings>));
	if(r != KErrNone && r != KErrAlreadyExists)
		return r;

	SLocaleTimeDateFormat localeTimeDateFormat;
	Mem::Copy(&localeTimeDateFormat.iShortDateFormatSpec[0], _S16("%F%*D/%*M/%Y"), sizeof(TText16) * 13);
	Mem::Copy(&localeTimeDateFormat.iLongDateFormatSpec[0], _S16("%F%*D%X %N %Y"), sizeof(TText16) * 14);
	Mem::Copy(&localeTimeDateFormat.iTimeFormatSpec[0], _S16("%F%*I:%T:%S %*A"), sizeof(TText16) * 16);

	r = RProperty::Set(KUidSystemCategory, KLocaleTimeDateFormatKey, TPckg<SLocaleTimeDateFormat>(localeTimeDateFormat));
	if(r != KErrNone)
		return r;

	TInt charSet = (TInt)GetLocaleDefaultCharSet();
	r = Exec::SetGlobalUserData(ELocaleDefaultCharSet, charSet);
	if(r != KErrNone)
		return r;

	r = Exec::SetGlobalUserData(ELocalePreferredCharSet, charSet);

	return r;
	}


// TOverflowHandler class created to handle the descriptor overflow in TLoacle::FormatCurrency
NONSHARABLE_CLASS(TOverflowHandler) : public TDesOverflow
	{
	void Overflow(TDes& aDes);
	};

void TOverflowHandler::Overflow(TDes&)
	{
	Panic(ETDes16Overflow);
	}




EXPORT_C void TLocale::FormatCurrency(TDes& aText, TInt aAmount)
/**
Renders a currency value as text, based on the locale's currency and numeric 
format settings. 

These settings include the currency symbol, the symbol's position and the 
way negative values are formatted.

@param aText   On return, contains the currency value as text, formatted
               according to the locale's currency format settings.
@param aAmount The currency value to be formatted.

@panic USER 11, if aText is not long enough to hold the formatted value.
*/
	{
	TOverflowHandler overflowHandler;
	FormatCurrency(aText,overflowHandler,aAmount);   
	}




EXPORT_C void TLocale::FormatCurrency(TDes& aText, TInt64 aAmount)
/**
Renders a currency value as text, based on the locale's currency and numeric 
format settings. 

These settings include the currency symbol, the symbol's position and the 
way negative values are formatted.

@param aText   On return, contains the currency value as text, formatted
               according to the locale's currency format settings.
@param aAmount The currency value to be formatted.

@panic USER 11, if aText is not long enough to hold the formatted value.
*/
	{
	TOverflowHandler overflowHandler;
	FormatCurrency(aText,overflowHandler, aAmount);
	}




EXPORT_C void TLocale::FormatCurrency(TDes& aText, TDesOverflow& aOverflowHandler, TInt aAmount)
/**
Renders a currency value as text, based on the locale's currency and numeric 
format settings. 

These settings include the currency symbol, the symbol's position and the 
way negative values are formatted. If aText is not long enough to hold the 
formatted currency value, the overflow handler's Overflow() function is called.

@param aText            On return, contains the currency value as text,
                        formatted according to the locale's currency format
                        settings.
@param aOverflowHandler An object derived from TDesOverflow which handles
                        descriptor overflows.
@param aAmount          The currency value to be formatted.
*/
	{
	TInt64 aLongerInt(aAmount);
	FormatCurrency(aText, aOverflowHandler, aLongerInt); 
	}




EXPORT_C void TLocale::FormatCurrency(TDes& aText, TDesOverflow& aOverflowHandler, TInt64 aAmount)
/**
Renders a currency value as text, based on the locale's currency and numeric 
format settings. 

These settings include the currency symbol, the symbol's position and the 
way negative values are formatted. If aText is not long enough to hold the 
formatted currency value, the overflow handler's Overflow() function is called.

@param aText            On return, contains the currency value as text,
                        formatted according to the locale's currency format
                        settings.
@param aOverflowHandler An object derived from TDesOverflow which handles
                        descriptor  overflows.
@param aAmount          The currency value to be formatted.
*/
	{
	// aAmount is in cents (or equivalent) rather than dollars (or equivalent)
	const TBool amountIsNegative=(aAmount<0);
	if (amountIsNegative)
		{
		aAmount=-aAmount;
		}
	aText.Num(aAmount, EDecimal);
	const TInt currencyDecimalPlaces=CurrencyDecimalPlaces();
	TInt positionOfDecimalSeparator=aText.Length();
	if (currencyDecimalPlaces>0)
		{
		while (positionOfDecimalSeparator <= currencyDecimalPlaces)
			{
			if (aText.Length() == aText.MaxLength())
				{
				aOverflowHandler.Overflow(aText);
				return;
				}
			aText.Insert(0,KLitZeroPad);
			++positionOfDecimalSeparator;
			}
		positionOfDecimalSeparator=aText.Length();
		positionOfDecimalSeparator-=currencyDecimalPlaces;
		TBuf<1> decimalSeparator;
		decimalSeparator.Append(DecimalSeparator());
		if (aText.Length() == aText.MaxLength())
			{
			aOverflowHandler.Overflow(aText);
			return;
			}
		aText.Insert(positionOfDecimalSeparator, decimalSeparator);
		}
	if (CurrencyTriadsAllowed())
		{
		TBuf<1> thousandsSeparator;
		thousandsSeparator.Append(ThousandsSeparator());
		TInt numberOfThousandsSeparator = positionOfDecimalSeparator/3;
		if ((aText.Length()+numberOfThousandsSeparator) > aText.MaxLength())
			{
			aOverflowHandler.Overflow(aText);
			return;
			}
		for (TInt i=positionOfDecimalSeparator-3; i>0; i-=3)
			{
			aText.Insert(i, thousandsSeparator);
			}
		}
	TInt positionToInsertCurrencySymbol = 0; 
	switch (CurrencySymbolPosition())
		{
		case ELocaleBefore:
			{
			if ((amountIsNegative) && (NegativeCurrencySymbolOpposite()))
				{
				positionToInsertCurrencySymbol=aText.Length();
				}
			else
				positionToInsertCurrencySymbol=0;
			}
			break;
		case ELocaleAfter:
			{
			if ((amountIsNegative) && (NegativeCurrencySymbolOpposite()))
				{
				positionToInsertCurrencySymbol=0;
				}
			else
				positionToInsertCurrencySymbol=aText.Length();
			}
			break;
		default:
			Panic(ETRegionOutOfRange);
			break;
		}
	if (CurrencySpaceBetween())
		{
		if (aText.Length() == aText.MaxLength())
			{
			aOverflowHandler.Overflow(aText);
			return;
			}
		if ((amountIsNegative) && (NegativeLoseSpace()))
			{
			// don't add the space
			}
		else
			{
			aText.Insert(positionToInsertCurrencySymbol, KLitSpace); 
			if (positionToInsertCurrencySymbol>0)
				{
				++positionToInsertCurrencySymbol;
				}
			}
		}
	TCurrencySymbol theCurrencySymbol;
	if ((aText.Length()+theCurrencySymbol.Length()) > aText.MaxLength())
		{
		aOverflowHandler.Overflow(aText);
		return;
		}
	aText.Insert(positionToInsertCurrencySymbol,theCurrencySymbol);
	if (amountIsNegative)
		{
		TInt positionToInsertInterveningMinusSign = 0;
		if ((CurrencySpaceBetween()) && !(NegativeLoseSpace()))
			{
			if (positionToInsertCurrencySymbol>0)
				{
				positionToInsertInterveningMinusSign = positionToInsertCurrencySymbol-1;
				}
			else
				{
				positionToInsertInterveningMinusSign = theCurrencySymbol.Length()+1;
				}
			}
		else
			{
			if (positionToInsertCurrencySymbol>0)
				{
				positionToInsertInterveningMinusSign = positionToInsertCurrencySymbol;
				}
			else
				{
				positionToInsertInterveningMinusSign = theCurrencySymbol.Length();
				}
			}
		switch (NegativeCurrencyFormat())
			{
			case EInBrackets:
				{
				if ((aText.Length()+2) > aText.MaxLength())
					{
					aOverflowHandler.Overflow(aText);
					return;
					}
				aText.Insert(0, KLitOpeningBracket);
				aText.Append(')');
				}
				break;
			case ELeadingMinusSign:
				{
				if (aText.Length() == aText.MaxLength())
					{
					aOverflowHandler.Overflow(aText);
					return;
					}
				aText.Insert(0, KLitMinusSign);
				}
				break;
			case ETrailingMinusSign:
				{
				if (aText.Length() == aText.MaxLength())
					{
					aOverflowHandler.Overflow(aText);
					return;
					}
				aText.Append(KLitMinusSign);
				}
				break;
			case EInterveningMinusSign:
				{
				if (aText.Length() == aText.MaxLength())
					{
					aOverflowHandler.Overflow(aText);
					return;
					}
				aText.Insert(positionToInsertInterveningMinusSign, KLitMinusSign);
				}
				break;
			default:
				Panic(ETRegionOutOfRange);
				break;
			}
		}
	}
	

EXPORT_C void TLocaleMessageText::Set(TLocaleMessage aMsgNo)
//
// Get some text from Locale
//
	{
	if(TUint(aMsgNo) < ELocaleMessages_LastMsg)
		{
		SLocaleLanguage localeLanguage;
		LocaleLanguageGet(localeLanguage);
		Copy((reinterpret_cast<const TText* const*>(localeLanguage.iMsgTable))[aMsgNo]);
		}
	else
		SetLength(0);
	}




EXPORT_C TInt TFindServer::Next(TFullName &aResult)
/**
Gets the full name of the next server which matches the match pattern.

@param aResult A reference to a descriptor with a defined maximum length.
               If a matching server is found, its full name is set into
               this descriptor. If no matching server is found,
               the descriptor length is set to zero.
               
@return KErrNone if a matching server is found, KErrNotFound otherwise.
*/
	{
	return NextObject(aResult,EServer);
	}




EXPORT_C void RServer2::Receive(RMessage2& aMessage, TRequestStatus &aStatus)
//
// Receive a message from the server asynchronously.
//
	{

	aStatus=KRequestPending;
	Exec::ServerReceive(iHandle, aStatus, &aMessage);
	}

EXPORT_C void RServer2::Cancel()
//
// Cancel a pending message receive.
//
	{

	Exec::ServerCancel(iHandle);
	}




EXPORT_C TInt TFindMutex::Next(TFullName &aResult)
/**
Finds the next global mutex whose full name matches the match pattern.

If a global mutex with a matching name is found, the function copies its full 
name into the specified descriptor. It also saves the find-handle associated 
with the global mutex into the TFindHandleBase part of this object.

@param aResult A reference to a descriptor with a defined maximum length. 
               If a matching global mutex is found, its full name is set
               into this descriptor. 
               If no matching global mutex is found, the descriptor length
               is set to zero.
               
@return KErrNone if a matching global mutex is found;
        KErrNotFound otherwise.
*/
	{
	return NextObject(aResult,EMutex);
	}




/**
Acquire the mutex, waiting for it to become free if necessary.

This function checks if the mutex is currently held. If not the mutex is marked
as held by the current thread and the call returns immediately. If the mutex is
held by another thread the current thread will suspend until the mutex becomes
free. If the mutex is already held by the current thread a count is maintained
of how many times the thread has acquired the mutex.
*/
EXPORT_C void RMutex::Wait()
	{
	Exec::MutexWait(iHandle, 0);
	}




/**
Acquire the mutex if it is currently free, but don't wait for it.

This function checks if the mutex is currently held. If not the mutex is marked
as held by the current thread and the call returns immediately indicating
success. If the mutex is held by another thread the call returns immediately
indicating failure. If the mutex is already held by the current thread a count
is maintained of how many times the thread has acquired the mutex.

@return	KErrNone if the mutex was acquired
		KErrTimedOut if the mutex could not be acquired
        KErrGeneral if the semaphore is being reset, i.e the semaphore
        is about to  be deleted.
*/
EXPORT_C TInt RMutex::Poll()
	{
	return Exec::MutexWait(iHandle, -1);
	}




/**
Acquire the mutex, if necessary waiting up to a specified maximum amount of time
for it to become free.

This function checks if the mutex is currently held. If not the mutex is marked
as held by the current thread and the call returns immediately. If the mutex is
held by another thread the current thread will suspend until the mutex becomes
free or until the specified timeout period has elapsed. If the mutex is already
held by the current thread a count is maintained of how many times the thread
has acquired the mutex.

@param aTimeout The timeout value in microseconds

@return KErrNone if the mutex was acquired successfully.
        KErrTimedOut if the timeout has expired.
        KErrGeneral if the mutex is being reset, i.e the mutex
        is about to  be deleted.
        KErrArgument if aTimeout is negative;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RMutex::Wait(TInt aTimeout)
	{
	if (aTimeout>=0)
		return Exec::MutexWait(iHandle, aTimeout);
	return KErrArgument;
	}




/**
Release the mutex.

This function decrements the count of how many times the current thread has
acquired this mutex. If the count is now zero the mutex is marked as free and,
if any other threads are waiting for the mutex to become free, the highest
priority among those is made ready to run. However the mutex is not marked as
held by any thread - the thread which has just been awakened must actually run
in order to acquire the mutex.

@pre The mutex must previously have been acquired by the current thread calling
Wait().

@panic KERN-EXEC 1 If the mutex has not previously been acquired by the current
thread calling Wait().
*/
EXPORT_C void RMutex::Signal()
	{

	Exec::MutexSignal(iHandle);
	}



/**
Test if this mutex is held by the current thread.
@return True if the current thread has waited on the mutex, false otherwise.
*/
EXPORT_C TBool RMutex::IsHeld()
	{
	return Exec::MutexIsHeld(iHandle);
	}


/** Wait on a condition variable

This call releases the specified mutex then atomically blocks the current
thread on this condition variable. The atomicity here is with respect to the
condition variable and mutex concerned. Specifically if the condition variable
is signalled at any time after the mutex is released then this thread will be
awakened. Once the thread has awakened it will reacquire the specified mutex
before this call returns (except in the case where the condition variable has
been deleted).

The usage pattern for this is as follows:

@code
	mutex.Wait();
	while(!CONDITION)
		condvar.Wait(mutex);
	STATEMENTS;
	mutex.Signal();
@endcode

where CONDITION is an arbitrary condition involving any number of user-side
variables whose integrity is protected by the mutex.

It is necessary to loop while testing the condition because there is **no** guarantee
that the condition has been satisfied when the condition variable is signalled.
Different threads may be waiting on different conditions or the condition may
have already been absorbed by another thread. All that can be said is that the
thread will awaken whenever something happens which **might** affect the condition.

It needs to be stressed that if:

@code
condvar.Wait(mutex);
@endcode

completes, it does not necessarily mean that the condition is yet satisfied, hence the necessity for the loop.

@param	aMutex		The mutex to be released and reacquired.
@return	KErrNone	if the condition variable has been signalled.
		KErrInUse	if another thread is already waiting on this condition
					variable in conjunction with a different mutex.
		KErrGeneral	if the condition variable is deleted.

@pre	The specified mutex is held by the current thread.
@post	The specified mutex is held by the current thread unless the return
		value is KErrGeneral in which case the condition variable no longer
		exists.

@panic	KERN-EXEC 0 if either the condition variable or mutex handles are not
		valid.
@panic	KERN-EXEC 54 if the current thread does not hold the specified mutex.

@see	RCondVar::Signal()
@see	RCondVar::Broadcast()
*/
EXPORT_C TInt RCondVar::Wait(RMutex& aMutex)
	{
	return Exec::CondVarWait(iHandle, aMutex.Handle(), 0);
	}



/** Wait on a condition variable with timeout

This is the same as RCondVar::Wait(RMutex) except that there is a time limit on
how long the current thread will block while waiting for the condition variable.

@param	aMutex		The mutex to be released and reacquired.
@param	aTimeout	The maximum time to wait in microseconds.
					0 means no maximum.
@return	KErrNone	if the condition variable has been signalled.
		KErrInUse	if another thread is already waiting on this condition
					variable in conjunction with a different mutex.
		KErrGeneral	if the condition variable is deleted.
		KErrTimedOut if the timeout expired before the condition variable was
					signalled.

@pre	The specified mutex is held by the current thread.
@post	The specified mutex is held by the current thread unless the return
		value is KErrGeneral in which case the condition variable no longer
		exists.

@panic	KERN-EXEC 0 if either the condition variable or mutex handles are not
		valid.
@panic	KERN-EXEC 54 if the current thread does not hold the specified mutex.

@see	RCondVar::Wait(RMutex)
*/
EXPORT_C TInt RCondVar::TimedWait(RMutex& aMutex, TInt aTimeout)
	{
	return Exec::CondVarWait(iHandle, aMutex.Handle(), aTimeout);
	}


/** Signal a condition variable

This unblocks a single thread which is currently blocked on the condition
variable. The highest priority waiting thread which is not explicitly suspended
will be the one unblocked. If there are no threads currently waiting this call
does nothing.

It is not required that any mutex is held when calling this function but it is
recommended that the mutex associated with the condition variable is held since
otherwise a race condition can result from the condition variable being signalled
just after the waiting thread testing the condition and before it calls Wait().

*/
EXPORT_C void RCondVar::Signal()
	{
	Exec::CondVarSignal(iHandle);
	}



/** Broadcast to a condition variable

This unblocks all threads which are currently blocked on the condition
variable. If there are no threads currently waiting this call does nothing.

It is not required that any mutex is held when calling this function but it is
recommended that the mutex associated with the condition variable is held since
otherwise a race condition can result from the condition variable being signalled
just after the waiting thread testing the condition and before it calls Wait().

*/
EXPORT_C void RCondVar::Broadcast()
	{
	Exec::CondVarBroadcast(iHandle);
	}




EXPORT_C TInt TFindProcess::Next(TFullName &aResult)
/**
Gets the full name of the next process which matches the match pattern.

@param aResult A reference to a TBuf descriptor with a defined maximum length.
               If a matching process is found, its full name is set into
               this descriptor. If no matching process is found, the descriptor
               length is set to zero.

@return KErrNone if successful, otherwise one of the other  system-wide error
        codes.
*/
	{
	return NextObject(aResult,EProcess);
	}




EXPORT_C TUidType RProcess::Type() const
/**
Gets the Uid type associated with the process. 

@return A reference to a TUidType object containing the process type.
*/
	{

	TUidType u;
	Exec::ProcessType(iHandle,u);
	return(u);
	}




EXPORT_C TProcessId RProcess::Id() const
/**
Gets the Id of this process.

@return The Id of this process.
*/
	{

	return TProcessId( (TUint)Exec::ProcessId(iHandle) );
	}




EXPORT_C void RProcess::Resume()
/**
Makes the first thread in the process eligible for execution.

@panic KERN-EXEC 32 if the process is not yet fully loaded.

@see RThread::Resume()
*/
	{

	Exec::ProcessResume(iHandle);
	}



EXPORT_C TFileName RProcess::FileName() const
/**
Gets a copy of the full path name of the loaded executable on which this 
process is based.

This is the file name which is passed to the Create() member function of this 
RProcess.

@return A TBuf descriptor with a defined maximum length containing the full 
        path name of the file.
        
@see RProcess::Create()
*/
	{

	TFileName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxFileName, KMaxFileName);
	Exec::ProcessFileName(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C void User::CommandLine(TDes &aCommand)
/**
Gets a copy of the data which is passed as an argument to the thread function
of the current process's main thread when it is first scheduled to run.

@param aCommand A modifiable descriptor supplied by the caller into which the
                argument data is put. The descriptor must be big enough to
                contain the expected data, otherwise the function raises
                a panic.

@see User::CommandLineLength()
*/
	{
	TPtr8 aCommand8((TUint8*)aCommand.Ptr(),aCommand.MaxLength()<<1);
	Exec::ProcessCommandLine(KCurrentProcessHandle,aCommand8);
	aCommand.SetLength(aCommand8.Length()>>1);
	}




EXPORT_C TInt User::CommandLineLength()
/**
Gets the length of the data which is passed as an argument to the thread
function of the current process's main thread when it is first scheduled
to run.

@return The length of the argument data.
*/
	{
	return Exec::ProcessCommandLineLength(KCurrentProcessHandle);
	}




EXPORT_C TExitType RProcess::ExitType() const
/**
Tests whether the process has ended and, if it has ended, return how it ended.

This information allows the caller to distinguish between normal termination 
and a panic.

@return An enumeration whose enumerators describe how the process has ended.
*/
	{

	return(Exec::ProcessExitType(iHandle));
	}




EXPORT_C TInt RProcess::ExitReason() const
/** 
Gets the specific reason associated with the end of this process.

The reason number together with the category name is a way of distinguishing
between different causes of process termination.

If the process has panicked, this value is the panic number. If the process 
has ended as a result of a call to Kill(), then the value is the one supplied 
by Kill().

If the process has not ended, then the returned value is zero.

@return The reason associated with the end of the process.

@see RProcess::Kill()
*/
	{

	return(Exec::ProcessExitReason(iHandle));
	}




EXPORT_C TExitCategoryName RProcess::ExitCategory() const
/**
Gets the name of the category associated with the end of the process.

The category name together with the reason number is a way of distinguishing
between different causes of process termination.

If the process has panicked, the category name is the panic category name; 
for example, E32USER-CBase or KERN-EXEC. If the process has ended as a result 
of a call to Kill(), then the category name is Kill.

If the process has not ended, then the category name is empty, i.e. the length 
of the category name is zero.

@return A descriptor with a defined maximum length containing the 
        name of the category associated with the end of the process.
        
@see RProcess::Kill()
*/
	{

	TExitCategoryName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxExitCategoryName, KMaxExitCategoryName);
	Exec::ProcessExitCategory(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C TProcessPriority RProcess::Priority() const
/**
Gets the priority of this process.

@return One of the TProcessPriority enumerator values.
*/
	{

	return(Exec::ProcessPriority(iHandle));
	}




EXPORT_C TInt RProcess::SetPriority(TProcessPriority aPriority) const
/**
Sets the priority of this process to one of the values defined by theTProcessPriority 
enumeration. The priority can be set to one of the four values:

EPriorityLow

EPriorityBackground

EPriorityForeground

EPriorityHigh

The absolute priority of all threads owned by the process (and all threads 
owned by those threads etc.) are re-calculated.

Notes:

The priority values EPriorityWindowServer, EPriorityFileServer, EPriorityRealTimeServer 
and EPrioritySupervisor are internal to Symbian OS and any attempt to explicitly 
set any of these priority values causes a KERN-EXEC 14 panic to be raised.

Any attempt to set the priority of a process which is protected and is different 
from the process owning the thread which invokes this function, causes a KERN-EXEC 
1 panic to be raised.

A process can set its own priority whether it is protected or not.

@param aPriority The priority value.

@return KErrNone, if successful; otherwise one of the other system-wide
        error codes.

*/
	{

	return Exec::ProcessSetPriority(iHandle,aPriority);
	}




/**
Tests whether "Just In Time" debugging is enabled or not for this process.

@return True, when "Just In Time" debugging is enabled. False otherwise.
@see RProcess::SetJustInTime
*/

EXPORT_C TBool RProcess::JustInTime() const
	{

	return (Exec::ProcessFlags(iHandle) & KProcessFlagJustInTime) != 0;
	}


/**
Enables or disables "Just In Time" debugging for this process.
This will only have an effect when running on the emulator.

"Just In Time" debugging catches a thread just before it executes a panic
or exception routine.  Capturing a thread early, before it is terminated, 
allows the developer to more closely inspect what went wrong, before the 
kernel removes the thread.  In some cases, the developer can modify context,
program counter, and variables to recover the thread.  This is only possible
on the emulator.

By default, "Just In Time" debugging is enabled.

@param aBoolean ETrue, to enable just-in-time debugging.
				EFalse, to disable just-in-time debugging.
*/
EXPORT_C void RProcess::SetJustInTime(TBool aState) const
	{

	TUint32 set = aState ? KProcessFlagJustInTime : 0;
	Exec::ProcessSetFlags(iHandle, KProcessFlagJustInTime, set);
	}




EXPORT_C TInt RProcess::SecureApi(TInt /*aState*/)
	{
	return ESecureApiOn;
	}

EXPORT_C TInt RProcess::DataCaging(TInt /*aState*/)
	{
	return EDataCagingOn;
	}



EXPORT_C TInt RProcess::GetMemoryInfo(TModuleMemoryInfo& aInfo) const
/**
Gets the size and base address of the code and various data
sections of the process.

The run addresses are also returned.

@param aInfo On successful return, contains the base address and size of the 
             sections.
             
@return KErrNone, if successful; otherwise one of the other system wide error 
        codes.
*/
	{

	return Exec::ProcessGetMemoryInfo(iHandle,aInfo);
	}
	

EXPORT_C TAny* RProcess::ExeExportData(void)
/**
Retrieves pointer to named symbol export data from the current process, i.e. the
process calling this function.
             
@return Pointer to export data when it is present, NULL if export data not found
@internalTechnology
@released
*/
	{

	return Exec::ProcessExeExportData();
	}



EXPORT_C TInt TFindSemaphore::Next(TFullName &aResult)
/**
Finds the next global semaphore whose full name matches the match pattern.

If a global semaphore with a matching name is found, the function copies its 
full name into the descriptor aResult. It also saves the find-handle associated 
with the global semaphore into the TFindHandleBase part of this TFindSemaphore 
object.

@param aResult A reference to a TBuf descriptor with a defined maximum length. 
               If a matching global semaphore is found, its full name is set
               into this descriptor. 
               If no matching global semaphore is found, the descriptor length
               is set to zero. 
               
@return KErrNone if a matching global semaphore is found;
        KErrNotFound otherwise.
*/
	{
	return NextObject(aResult,ESemaphore);
	}




/**
Waits for a signal on the semaphore.

The function decrements the semaphore count by one and returns immediately 
if it is zero or positive.

If the semaphore count is negative after being decremented, the calling thread is 
added to a queue of threads maintained by this semaphore.

The thread waits until the semaphore is signalled. More than one thread can 
be waiting on a particular semaphore at a time. When there are multiple threads 
waiting on a semaphore, they are released in priority order.

If the semaphore is deleted, all threads waiting on that semaphore are released.
*/
EXPORT_C void RSemaphore::Wait()
	{
	Exec::SemaphoreWait(iHandle, 0);
	}


/**
Waits for a signal on the semaphore, or a timeout.

@param aTimeout The timeout value in microseconds

@return KErrNone if the wait has completed normally.
        KErrTimedOut if the timeout has expired.
        KErrGeneral if the semaphore is being reset, i.e the semaphore
        is about to  be deleted.
        KErrArgument if aTimeout is negative;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RSemaphore::Wait(TInt aTimeout)
	{
	if (aTimeout>=0)
		return Exec::SemaphoreWait(iHandle, aTimeout);
	return KErrArgument;
	}


/**
Acquires the semaphore if that is possible without waiting.

@return KErrNone if the semaphore was acquired successfully
        KErrTimedOut if the semaphore could not be acquired
        KErrGeneral if the semaphore is being reset, i.e the semaphore
        is about to  be deleted.
*/
EXPORT_C TInt RSemaphore::Poll()
	{
	return Exec::SemaphoreWait(iHandle, -1);
	}


EXPORT_C void RSemaphore::Signal()
/**
Signals the semaphore once.

The function increments the semaphore count by one. If the count was negative 
before being incremented, the highest priority thread waiting on the semaphore's queue 
of threads is removed from that queue and, provided that it is not suspended 
for any other reason, is marked as ready to run.
*/
	{

	Exec::SemaphoreSignal1(iHandle);
	}




EXPORT_C void RSemaphore::Signal(TInt aCount)
/**
Signals the semaphore one or more times.

The function increments the semaphore count. If the count was negative 
before being incremented, the highest priority thread waiting on the semaphore's queue 
of threads is removed from that queue and, provided that it is not suspended 
for any other reason, is marked as ready to run.

@param aCount The number of times the semaphore is to be signalled.
*/
	{

	__ASSERT_ALWAYS(aCount>=0,Panic(ESemSignalCountNegative));
	Exec::SemaphoreSignalN(iHandle,aCount);
	}



#ifndef __CPU_ARM
/**
Acquire the lock, if necessary waiting up to a specified maximum amount of time
for it to become free.

This function checks if the lock is currently held. If not the lock is marked
as held by the current thread and the call returns immediately. If the lock is
held by another thread the current thread will suspend until the lock becomes
free or until the specified timeout period has elapsed.

@param aTimeout The timeout value in microseconds

@return KErrNone if the lock was acquired successfully.
        KErrTimedOut if the timeout has expired.
        KErrGeneral if the lock is being reset, i.e the lock
        is about to  be deleted.
        KErrArgument if aTimeout is negative;
        otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RFastLock::Wait(TInt aTimeout)
	{
	if (aTimeout<=0)
		return KErrArgument;
	TInt orig = __e32_atomic_add_acq32(&iCount, TUint32(-1));
	if (orig == 0)
		return KErrNone;
	FOREVER
		{
		TInt r = Exec::SemaphoreWait(iHandle, aTimeout);
		if (r != KErrTimedOut)	// got lock OK or lock deleted
			return r;
		// Before we can return KErrTimedOut we must increment iCount (since we
		// previously decremented it in anticipation of acquiring the lock.
		// However we must not increment iCount if it would become zero, since
		// the semaphore will have been signalled (to counterbalance the Wait()
		// which timed out and thus never happened). This would result in two
		// threads being able to acquire the lock simultaneously - one by
		// decrementing iCount from 0 to -1 without looking at the semaphore,
		// and the other by decrementing iCount from -1 to -2 and then absorbing
		// the spurious semaphore signal.
		orig = __e32_atomic_tas_ord32(&iCount, -1, 0, 1);	// don't release lock completely
		if (orig < -1)
			return KErrTimedOut;	// count corrected - don't need to touch semaphore
		// lock is actually free at this point, try again to claim it
		aTimeout = 1;
		}
	}
#endif

EXPORT_C RCriticalSection::RCriticalSection()
	: iBlocked(1)
/**
Default constructor.
*/
	{}




EXPORT_C void RCriticalSection::Close()
/**
Closes the handle to the critical section.

As a critical section object is implemented using a semaphore, this has the 
effect of closing the handle to the semaphore.
*/
	{

	RSemaphore::Close();
	}




EXPORT_C void RCriticalSection::Wait()
/**
Waits for the critical section to become free.

If no other thread is in the critical section, control returns immediately 
and the current thread can continue into the section.

If another thread is already in the critical section, the current thread is 
marked as waiting (on a semaphore); the current thread is added to a queue
of threads maintained by this critical section.
*/
	{

	if (__e32_atomic_add_acq32(&iBlocked, KMaxTUint32) != 1)
		RSemaphore::Wait();
	}




EXPORT_C void RCriticalSection::Signal()
/**
Signals an exit from the critical section.

A thread calls this function when it exits from the critical section.
The first eligible thread waiting on the critical section's queue of threads
is removed from that queue and, provided that it is not suspended for any other
reason, is marked as ready to run. That thread will, therefore, be the next to
proceed into the critical section.
*/
	{

	__ASSERT_ALWAYS(iBlocked<1,Panic(ECriticalSectionStraySignal));
	if (TInt(__e32_atomic_add_rel32(&iBlocked, 1)) < 0)
		RSemaphore::Signal();
	}




EXPORT_C TInt TFindThread::Next(TFullName &aResult)
/**
Gets the full name of the next global thread which matches the match pattern.

@param aResult A reference to a TBuf descriptor with a defined maximum length.
               If a matching global thread is found, its full name is set into
               this descriptor. If no matching global thread is found,
               the descriptor length is set to zero.

@return KErrNone if successful, otherwise one of the other system-wide error
                 codes.
*/
	{
	return NextObject(aResult,EThread);
	}




EXPORT_C TThreadId RThread::Id() const
/**
Gets the Id of this thread.

@return The Id of this thread.
*/
	{
#ifdef __USERSIDE_THREAD_DATA__
	if(iHandle==KCurrentThreadHandle)
		return TThreadId( LocalThreadData()->iThreadId );
#endif // __USERSIDE_THREAD_DATA__

	return TThreadId( (TUint)Exec::ThreadId(iHandle) );
	}




EXPORT_C void RThread::HandleCount(TInt& aProcessHandleCount, TInt& aThreadHandleCount) const
/**
Gets the number of handles open in this thread, and the number of handles open 
in the process which owns this thread.

@param aProcessHandleCount On return, contains the number of handles open in
                           the process which owns this thread.
@param aThreadHandleCount  On return, contains the number of handles open in
                           this thread.
*/
	{

	Exec::HandleCount(iHandle,aProcessHandleCount,aThreadHandleCount);
	}




EXPORT_C TExceptionHandler User::ExceptionHandler()
/**
Gets a pointer to the exception handler for the current thread.

@return A pointer to the exception handler.
*/
	{
	return(Exec::ExceptionHandler(KCurrentThreadHandle));
	}




EXPORT_C TInt User::SetExceptionHandler(TExceptionHandler aHandler,TUint32 aMask)
/**
Sets a new exception handler for the current thread. Note that the handler is not
guaranteed to receive floating point exceptions (KExceptionFpe) when a hardware
floating point implementation is in use - see User::SetFloatingPointMode for
hardware floating point modes and whether they cause user-trappable exceptions.

@param aHandler The new exception handler.
@param aMask    One or more flags defining the exception categories which
                the handler can handle.

@return KErrNone if successful, otherwise another of the system-wide error codes.

@see KExceptionAbort
@see KExceptionKill
@see KExceptionUserInterrupt
@see KExceptionFpe
@see KExceptionFault
@see KExceptionInteger
@see KExceptionDebug
*/
	{
	return(Exec::SetExceptionHandler(KCurrentThreadHandle, aHandler, aMask));
	}




EXPORT_C void User::ModifyExceptionMask(TUint32 aClearMask, TUint32 aSetMask)
/**
Changes the set of exceptions which the current thread's exception
handler can deal with.

aClearMask is the set of flags defining the set of exceptions which the
exception handler no longer deals with, while aSetMask is the set of flags
defining the new set of exceptions to be set.

Flag clearing is done before flag setting.

@param  aClearMask One or more flags defining the exceptions which the current
                   exception handler no longer deals with.
@param  aSetMask   One or more flags defining the new set of exceptions which
                   the current exception handler is to deal with.
*/
	{
	Exec::ModifyExceptionMask(KCurrentThreadHandle, aClearMask, aSetMask);
	}




_LIT(KLitUserExec, "USER-EXEC");
EXPORT_C TInt User::RaiseException(TExcType aType)
/**
Raises an exception of a specified type on the current thread.

If the thread has an exception handler to handle this type of exception,
then it is called.

If the thread has no exception handler to handle this type of exception, then
the function raises a USER-EXEC 3 panic.

Note that exception handlers are executed in the context of the thread on which
the exception is raised; control returns to the point of the exception. 

@param aType The type of exception.

@return KErrNone if successful, otherwise another of the system-wide
        error codes.
*/
	{
	if (Exec::IsExceptionHandled(KCurrentThreadHandle,aType,ETrue))
		{
		Exec::ThreadSetFlags(KCurrentThreadHandle, 0, KThreadFlagLastChance);
		TUint32 type = aType;
		User::HandleException(&type);
		}
	else
		User::Panic(KLitUserExec, ECausedException);
	return KErrNone;
	}




EXPORT_C TBool User::IsExceptionHandled(TExcType aType)
/**
Tests whether the specified exception type can be handled by the
current thread.

@param aType The type of exception.

@return True, if the thread has an exception handler which can handle
        an exception of type aType.
        False, if the thread has no exception handler or the thread has
        an exception handler which cannot handle the exception defined
        by aType.
*/
	{
	return (Exec::IsExceptionHandled(KCurrentThreadHandle,aType,EFalse));
	}




EXPORT_C void RThread::Context(TDes8 &aDes) const
/**
Gets the register contents of this thread.

@param aDes On return, contains the register contents, starting with R0.
*/
	{

	Exec::ThreadContext(iHandle,aDes);
	}




EXPORT_C void RThread::Resume() const
/**
Makes the thread eligible for execution.

After a thread is created, it is put into a suspended state; the thread is 
not eligible to run until Resume() is called.

This function must also be called to resume execution of this thread after 
it has been explicitly suspended by a call to Suspend().

Note that when a thread is created, it is given the priority EPriorityNormal 
by default. The fact that a thread is initially put into a suspended state 
means that the thread priority can be changed by calling the thread's
SetPriority() member function before the thread is started by a call to Resume().
*/
	{

	Exec::ThreadResume(iHandle);
	}




EXPORT_C void RThread::Suspend() const
/**
Suspends execution of this thread.

The thread is not scheduled to run until a subsequent call to Resume() is made.
*/
	{

	Exec::ThreadSuspend(iHandle);
	}




EXPORT_C TThreadPriority RThread::Priority() const
/**
Gets the priority of this thread.

@return The priority.
*/
	{

	return(Exec::ThreadPriority(iHandle));
	}




EXPORT_C void RThread::SetProcessPriority(TProcessPriority aPriority) const
/**
Sets the priority of the process which owns this thread to one of the values 
defined by the TProcessPriority enumeration.

The priority can be set to one of the four values:

EPriorityLow

EPriorityBackground

EPriorityForeground

EPriorityHigh

The absolute priority of all threads owned by the process (and all threads 
owned by those threads etc.) are re-calculated.

Note:

The use of the priority values EPriorityWindowServer, EPriorityFileServer, 
EPriorityRealTimeServer and EPrioritySupervisor is restricted to Symbian OS, 
and any attempt to explicitly set any of these priority values raises a KERN-EXEC 
14 panic.

@param aPriority The priority value.

@deprecated Not allowed on threads in a different process.
			Replace with RProcess::SetPriority or RMessagePtr2::SetProcessPriority
*/
	{

	Exec::ThreadSetProcessPriority(iHandle,aPriority);
	}




EXPORT_C TProcessPriority RThread::ProcessPriority() const
/**
Gets the priority of the process which owns this thread.

@return The process priority.
*/
	{

	return(Exec::ThreadProcessPriority(iHandle));
	}




EXPORT_C void RThread::SetPriority(TThreadPriority aPriority) const
/**
Sets the priority of the thread to one of the values defined by
the TThreadPriority enumeration. 

The resulting absolute priority of the thread depends on the value of aPriority 
and the priority of the owning process.

Use of the priority value EPriorityNull is restricted to Symbian OS, and any 
attempt to explicitly set this value causes a KERN-EXEC 14 panic to be raised.

@param aPriority The priority value.

@capability ProtServ if aPriority is EPriorityAbsoluteRealTime1 or higher

@panic KERN-EXEC 14, if aPriority is invalid or set to EPriorityNull
@panic KERN-EXEC 46, if aPriority is EPriorityAbsoluteRealTime1 or higher
                     and calling process does not have ProtServ capability
*/
	{

	Exec::ThreadSetPriority(iHandle,aPriority);
	}




EXPORT_C User::TCritical User::Critical(RThread aThread)
/**
Gets the critical state associated with the specified thread.

@param aThread The thread whose critical state is to be retrieved.

@return The critical state.

@see User::SetCritical()
*/
	{
	TUint32 flags = Exec::ThreadFlags(aThread.Handle());
	if (flags & KThreadFlagSystemPermanent)
		return ESystemPermanent;
	if (flags & KThreadFlagSystemCritical)
		return ESystemCritical;
	if (flags & KThreadFlagProcessPermanent)
		return EProcessPermanent;
	if (flags & KThreadFlagProcessCritical)
		return EProcessCritical;
	return ENotCritical;
	}




EXPORT_C User::TCritical User::Critical()
/**
Gets the critical state associated with the current thread.

@return The critical state.
	
@see User::SetCritical()
*/
	{
	RThread me;
	return User::Critical(me);
	}




/**
Sets up or changes the effect that termination of the current
thread has, either on its owning process, or on the whole system.

The precise effect of thread termination is defined by 
the following specific values of the TCritical enum:
- ENotCritical
- EProcessCritical
- EProcessPermanent
- ESystemCritical
- ESystemPermanent

Notes:
-# The enum value EAllThreadsCritical cannot be set using this function. It is
associated with a process, not a thread, and, if appropriate, should be set
using User::SetProcessCritical().
-# The states associated with ENotCritical, EProcessCritical,
EProcessPermanent, ESystemCritical and ESystemPermanent
are all mutually exclusive, i.e. the thread can only be in one of these states
at any one time

@param aCritical The state to be set. 

@return KErrNone, if successful;
        KErrArgument, if EAllThreadsCritical is passed - this is a 
        state associated with a process, and
        you use User::SetProcessCritical() to set it.

@capability ProtServ if aCritical==ESystemCritical or ESystemPermanent

@see User::Critical()
@see User::ProcessCritical()
@see User::SetProcessCritical()
*/
EXPORT_C TInt User::SetCritical(TCritical aCritical)
	{
	const TUint32 clear =	KThreadFlagSystemPermanent | KThreadFlagSystemCritical |
							KThreadFlagProcessPermanent | KThreadFlagProcessCritical;
	TUint32 set;
	switch (aCritical)
		{
		case ENotCritical:		set = 0;							break;
		case EProcessCritical:	set = KThreadFlagProcessCritical;	break;
		case EProcessPermanent:	set = KThreadFlagProcessPermanent;	break;
		case ESystemCritical:	set = KThreadFlagSystemCritical;	break;
		case ESystemPermanent:	set = KThreadFlagSystemPermanent;	break;
		default:													return KErrArgument;
		}
	Exec::ThreadSetFlags(KCurrentThreadHandle, clear, set);
	return KErrNone;
	}




EXPORT_C TInt User::SetRealtimeState(TRealtimeState aState)
	{
	const TUint32 clear =	KThreadFlagRealtime | KThreadFlagRealtimeTest;
	TUint32 set;
	switch (aState)
		{
		case ERealtimeStateOff:		set = 0;											break;
		case ERealtimeStateOn:		set = KThreadFlagRealtime;							break;
		case ERealtimeStateWarn:	set = KThreadFlagRealtime|KThreadFlagRealtimeTest;	break;
		default:																		return KErrArgument;
		}
	Exec::ThreadSetFlags(KCurrentThreadHandle, clear, set);
	return KErrNone;
	}




EXPORT_C User::TCritical User::ProcessCritical(RProcess aProcess)
/**
Gets the critical state associated with the specified process.

@param aProcess The process whose critical state is to be retrieved.

@return The critical state.

@see User::SetProcessCritical()
*/
	{
	TUint32 flags = Exec::ProcessFlags(aProcess.Handle());
	if (flags & KProcessFlagSystemPermanent)
		return ESystemPermanent;
	if (flags & KProcessFlagSystemCritical)
		return ESystemCritical;
	if (flags & (KThreadFlagProcessPermanent | KThreadFlagProcessCritical))
		return EAllThreadsCritical;
	return ENotCritical;
	}




EXPORT_C User::TCritical User::ProcessCritical()
/**
Gets the critical state associated with the current process.

@return The critical state.
	
@see User::SetProcessCritical()
*/
	{
	RProcess me;
	return User::ProcessCritical(me);
	}




EXPORT_C TInt User::SetProcessCritical(TCritical aCritical)
/**
Sets up or changes the effect that termination of subsequently created threads
will have, either on the owning process, or on the whole system.

It is important to note that we are not referring to threads that have already
been created, but threads that will be created subsequent to a call to this function.

The precise effect of thread termination is defined by the following specific
values of the TCritical enum: 
- ENotCritical
- EAllThreadsCritical
- ESystemCritical
- ESystemPermanent

Notes:
-# The enum values EProcessCritical and EProcessPermanent cannot be set using
this function. They are states associated with
a thread, not a process, and, if appropriate, should be set
using User::SetCritical().
-# The states associated with ENotCritical, EAllThreadsCritical,
ESystemCritical and ESystemPermanent are all mutually exclusive, i.e. the
process can only be in one of these states at any one time.

@param aCritical The state to be set. 

@return KErrNone, if successful;
        KErrArgument, if either EProcessCritical or EProcessPermanent
        is passed - these are states associated with a thread, and
        you use User::SetCritical() to set them.

@capability ProtServ if aCritical==ESystemCritical or ESystemPermanent

@see User::ProcessCritical()
@see User::SetCritical()
@see User::Critical()
*/
	{
	const TUint32 clear =	KProcessFlagSystemPermanent | KProcessFlagSystemCritical |
							KThreadFlagProcessPermanent | KThreadFlagProcessCritical;
	TUint32 set;
	switch (aCritical)
		{
		case ENotCritical:			set = 0;							break;
		case EAllThreadsCritical:	set = KThreadFlagProcessCritical;	break;
		case ESystemCritical:		set = KProcessFlagSystemCritical;	break;
		case ESystemPermanent:		set = KProcessFlagSystemPermanent|KProcessFlagSystemCritical;	break;
		default:														return KErrArgument;
		}
	Exec::ProcessSetFlags(KCurrentProcessHandle, clear, set);
	return KErrNone;
	}




EXPORT_C TBool User::PriorityControl()
/**
Tests whether the current process allows other processes to switch its priority 
between 'foreground' and 'background'.

@return True, if the current process allows other processes to switch its priority;
        false, otherwise.
*/
	{
	return Exec::ProcessFlags(KCurrentProcessHandle) & KProcessFlagPriorityControl;
	}




EXPORT_C void User::SetPriorityControl(TBool aEnable)
/**
Allows the current process to choose to have its priority switched by another
process between 'foreground' and 'background'.

By default a process does not allow this.

@param aEnable If ETrue, allows other processes to switch the current process's
               priority.
               If EFalse, prevents other processes from switching the current
               process's priority.
*/
	{
	TUint32 set = aEnable ? KProcessFlagPriorityControl : 0;
	Exec::ProcessSetFlags(KCurrentProcessHandle, KProcessFlagPriorityControl, set);
	}



EXPORT_C TInt RThread::RequestCount() const
/**
Gets this thread's request semaphore count.

The request semaphore is created when a thread is created, and is used to 
support asynchronous requests.

A negative value implies that this thread is waiting for at least
one asynchronous request to complete.

@return This thread's request semaphore count.
*/
	{

	return(Exec::ThreadRequestCount(iHandle));
	}




EXPORT_C TExitType RThread::ExitType() const
/**
Tests whether the thread has ended and, if it has ended, return how it ended.

This information allows the caller to distinguish between normal termination 
and a panic.

@return An enumeration whose enumerators describe how the thread has ended.
*/
	{

	return(Exec::ThreadExitType(iHandle));
	}




EXPORT_C TInt RThread::ExitReason() const
/**
Gets the specific reason associated with the end of this thread.

The reason number together with the category name is a way of distinguishing 
between different causes of thread termination.

If the thread has panicked, this value is the panic number. If the thread 
has ended as a result of a call to Kill(), then the value is the one supplied 
by Kill().

If the thread is still alive, then the returned value is zero.

@return The reason associated with the end of the thread.
*/
	{

	return(Exec::ThreadExitReason(iHandle));
	}




EXPORT_C TExitCategoryName RThread::ExitCategory() const
/**
Gets the name of the category associated with the end of the thread.

The category name together with the reason number is a way of distinguishing
between different causes of thread termination.

If the thread has panicked, the category name is the panic category name; 
for example, E32USER-CBase or KERN-EXEC. If the thread has ended as a result 
of call to Kill(), then the category name is Kill.

If the thread has not ended, then the category name is empty, i.e. the length 
of the category name is zero.

@return A TBuf descriptor with a defined maximum length containing the name 
        of the category associated with the end of the thread.

@see TBuf
*/
	{
	TExitCategoryName n;
	TPtr8 n8(((TUint8*)n.Ptr()) + KMaxExitCategoryName, KMaxExitCategoryName);
	Exec::ThreadExitCategory(iHandle,n8);
	n.Copy(n8);
	return(n);
	}




EXPORT_C TInt RThread::StackInfo(TThreadStackInfo& aInfo) const
/**
Gets information about a thread's user mode stack.

@param aInfo The TThreadStackInfo object to write the stack infomation to.

@return KErrNone, if sucessful;
		KErrGeneral, if the thread doesn't have a user mode stack,
		or it has terminated.

@see TThreadStackInfo
*/
	{
	return(Exec::ThreadStackInfo(iHandle,aInfo));
	}




EXPORT_C TInt RThread::GetCpuTime(TTimeIntervalMicroSeconds& aCpuTime) const
/**
Gets the CPU usage for this thread.

This function is not supported on version 8.0b or 8.1b, and returns
KErrNotSupported.  From 9.1 onwards it may be supported if the kernel has been
compiled with the MONITOR_THREAD_CPU_TIME macro defined.

@param aCpuTime A reference to a time interval object supplied by the caller. 
                                
@return KErrNone - if thread CPU time is available.

        KErrNotSupported - if this feature is not supported on this
        version or build of the OS.
*/
	{
	return Exec::ThreadGetCpuTime(iHandle, (TInt64&)aCpuTime.Int64());
	}




EXPORT_C void User::After(TTimeIntervalMicroSeconds32 aInterval)
/**
Suspends the current thread until a specified time interval has expired.

The resolution of the timer depends on the hardware, but is normally 
1 Symbian OS tick (approximately 1/64 second).

@param aInterval The time interval for which the current thread is to be 
                  suspended, in microseconds.
                  
@panic USER 86, if the time interval is negative.
*/
	{

	__ASSERT_ALWAYS(aInterval.Int()>=0,::Panic(EExecAfterTimeNegative));
	TRequestStatus s=KRequestPending;
	Exec::After(aInterval.Int(),s);
	User::WaitForRequest(s);
	}




EXPORT_C void User::AfterHighRes(TTimeIntervalMicroSeconds32 aInterval)
/**
Suspends the current thread until a specified time interval has expired to
a resolution of 1ms .

@param aInterval The time interval for which the current thread is to be 
                  suspended, in microseconds.
                  
@panic USER 86, if the time interval is negative.
*/
	{

	__ASSERT_ALWAYS(aInterval.Int()>=0,::Panic(EExecAfterTimeNegative));
	TRequestStatus s=KRequestPending;
	Exec::AfterHighRes(aInterval.Int(),s);
	User::WaitForRequest(s);
	}




EXPORT_C TInt User::At(const TTime &aTime)
/**
Suspends the current thread until the specified absolute time, in the current time zone.

If the machine is off at that time, the machine will be turned on again.

@param aTime The absolute time, in the current time zone, until which the current thread is to
             be suspended.

@return On completion, contains the status of the request to suspend the
        current thread:

        KErrNone - suspension of the current thread completed normally at 
        the requested time.

        KErrAbort - suspension of the current thread was aborted 
        because the system time changed.

        KErrUnderflow - the requested completion time is in the past.

        KErrOverFlow - the requested completion time is too far in the future.
*/
	{

	TRequestStatus s=KRequestPending;
	TInt64 time=aTime.Int64();
	time -= ((TInt64)User::UTCOffset().Int()) * 1000000;
	Exec::At(time,s);
	User::WaitForRequest(s);
	return(s.Int());
	}




EXPORT_C void RTimer::Cancel()
/**
Cancels any outstanding request for a timer event.

Any outstanding timer event completes with KErrCancel.
*/
	{

	Exec::TimerCancel(iHandle);
	}




EXPORT_C void RTimer::After(TRequestStatus &aStatus,TTimeIntervalMicroSeconds32 aInterval)
//
// Request a relative timer.
//
/**
Requests an event after the specified interval.

The counter for this type of request stops during power-down.
A 5 second timer will complete late if, for example, the machine is turned off
2 seconds after the request is made.

@param aStatus    On completion, contains the status of the request.
                  This is KErrNone if the timer completed normally at the
                  requested time, otherwise another of the
                  system-wide error codes.

@param aInterval  The time interval, in microseconds, after which an event
                  is to occur.

@panic USER 87, if aInterval is negative.
@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{

	__ASSERT_ALWAYS(aInterval.Int()>=0,::Panic(ERTimerAfterTimeNegative));
	aStatus=KRequestPending;
	Exec::TimerAfter(iHandle,aStatus,aInterval.Int());
	}




EXPORT_C void RTimer::AfterTicks(TRequestStatus& aStatus, TInt aTicks)
//
// Request a relative timer in system ticks.
//
/**
Requests an event after the specified interval.

The counter for this type of request stops during power-down.
A 5 tick timer will complete late if, for example, the machine is turned off
2 ticks after the request is made.

@param aStatus    On completion, contains the status of the request.
                  This is KErrNone if the timer completed normally at the
                  requested time, otherwise another of the
                  system-wide error codes.

@param aTicks     The time interval, in system ticks, after which an event
                  is to occur.

@panic USER 87, if aTicks is negative.
@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{
	__ASSERT_ALWAYS(aTicks >= 0, ::Panic(ERTimerAfterTimeNegative));
	aStatus = KRequestPending;
	Exec::TimerAfter(iHandle, aStatus, -aTicks);
	}




EXPORT_C void RTimer::HighRes(TRequestStatus &aStatus,TTimeIntervalMicroSeconds32 aInterval)
//
// Request a relative timer to a resolution of 1ms.
//
/**
Requests an event after the specified interval to a resolution of 1ms. 
The "HighRes timer" counter stops during power-down (the same as "after timer"). 

@param aStatus    On completion, contains the status of the request.
                  This is KErrNone if the timer completed normally at the
                  requested time, otherwise another of the
                  system-wide error codes.

@param aInterval  The time interval, in microseconds, after which an event
                  is to occur.

@panic USER 87, if aInterval is negative.
@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{

	__ASSERT_ALWAYS(aInterval.Int()>=0,::Panic(ERTimerAfterTimeNegative));
	aStatus=KRequestPending;
	Exec::TimerHighRes(iHandle,aStatus,aInterval.Int());
	}




/**
Requests an event at a specified time after the last expiry of the this
timer object, to a resolution of 1ms. If the last usage of this timer object
was not via either this function or RTimer::HighRes(), this call behaves the
same as RTimer::HighRes().
The "HighRes timer" counter stops during power-down (the same as "after timer"). 

@param aStatus    On completion, contains the status of the request.
                  This is KErrNone if the timer completed normally at the
                  requested time, otherwise another of the
                  system-wide error codes. In particular KErrArgument indicates
				  that the requested expiry time has already passed.

@param aInterval  The time interval, in microseconds, after which an event
                  is to occur, measured from the last expiry time (or intended
				  expiry time in the case where the timer was cancelled) of this
				  timer object.
				  Note that the interval is allowed to be negative. To see why
				  this might be useful consider the following sequence of timer
				  operations:
					1. Timer expires at time T
					2. AgainHighRes(1000000) - timer is queued for T + 1 sec
					3. Cancel() - timer is not queued but last scheduled expiry
									is still at T + 1 second
					4. AgainHighRes(-500000)
					5. Timer expires at time T + 0.5 second

@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
EXPORT_C void RTimer::AgainHighRes(TRequestStatus &aStatus,TTimeIntervalMicroSeconds32 aInterval)
	{
	aStatus=KRequestPending;
	Exec::TimerAgainHighRes(iHandle,aStatus,aInterval.Int());
	}




EXPORT_C void RTimer::At(TRequestStatus &aStatus,const TTime &aTime)
//
// Request an absolute timer.
//
/**
Requests an event at a given system time (in the current time zone).

If the machine is off at that time, it is automatically turned on.

@param aStatus On completion, contains the status of the request:
               KErrNone, the timer completed normally at the requested time;
               KErrCancel, the timer was cancelled;
               KErrAbort, the timer was aborted because the system time changed;
               KErrUnderflow, the requested completion time is in the past;
               KErrOverFlow, the requested completion time is too far in the future;
@param aTime   The time at which the timer will expire.

@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{

	aStatus=KRequestPending;
	TInt64 time=aTime.Int64();
	time -= ((TInt64)User::UTCOffset().Int()) * 1000000;
	Exec::TimerAt(iHandle,aStatus,I64LOW(time),I64HIGH(time));
	}




EXPORT_C void RTimer::AtUTC(TRequestStatus &aStatus,const TTime &aUTCTime)
//
// Request an absolute timer in UTC time.
//
/**
Requests an event at a given UTC time.

If the machine is off at that time, it is automatically turned on.

@param aStatus On completion, contains the status of the request:
               KErrNone, the timer completed normally at the requested time;
               KErrCancel, the timer was cancelled;
               KErrAbort, the timer was aborted because the system time changed;
               KErrUnderflow, the requested completion time is in the past;
               KErrOverFlow, the requested completion time is too far in the future;
@param aTime   The time at which the timer will expire.

@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{

	aStatus=KRequestPending;
	Exec::TimerAt(iHandle,aStatus,I64LOW(aUTCTime.Int64()),I64HIGH(aUTCTime.Int64()));
	}




EXPORT_C void RTimer::Lock(TRequestStatus &aStatus,TTimerLockSpec aLock)
//
// Request an absolute timer.
//
/**
Requests an event on a specified second fraction.

@param aStatus On completion, contains the status of the request:
               KErrGeneral, the first time this is called;
               KErrNone, the timer completed normally at the requested time;
               KErrCancel, the timer was cancelled;
               KErrAbort, the timer was aborted because the system time changed;
               KErrUnderflow, the requested completion time is in the past;
               KErrOverFlow, the requested completion time is too far in the future.
@param aLock   The fraction of a second at which the timer completes.

@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{
	aStatus=KRequestPending;
	Exec::TimerLock(iHandle,aStatus,aLock);
	}


/**
Requests an event to be triggered when aSeconds is exactly, (ie not greater or 
less than), the time elapsed (to the nearest second) since the last user activity.
If the event trigger time has been "missed", instead of triggering late,
the timer waits for the next user activity, to try and satisfy the condition.

That is to say, if there was user activity within the last aSeconds,
the event will be triggered after aSeconds of continuous inactivity following that activity.
Otherwise, if there has been no such activity within this time, an event is
triggered after aSeconds of continuous inactivity following the next user activity
in the future.

It follows from this, that you can request an event directly after the next
user activity by supplying a time interval of zero.


@param aStatus  On completion, contains the status of the request:
                KErrNone, the timer completed normally;
                KErrCancel, the timer was cancelled;
                KErrArgument, if aSeconds is less then zero;
                KErrOverflow, if aSecond reaches its limit (which is platform specific but greater then one and a half day).
@param aSeconds The time interval in seconds.

@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
EXPORT_C void RTimer::Inactivity(TRequestStatus &aStatus, TTimeIntervalSeconds aSeconds)
	{
	aStatus=KRequestPending;
	Exec::TimerInactivity(iHandle, aStatus, aSeconds.Int());
	}




EXPORT_C TInt RChangeNotifier::Logon(TRequestStatus& aStatus) const
/**
Issues a request for notification when changes occur in the environment. 

A switch in locale, or crossing over past midnight, are examples of changes
that are reported.

When a change in the environment occurs, the request completes and the 
TRquestStatus object will contain one or more of the bit values defined
by the TChanges enum.

Alternatively, if an outstanding request is cancelled by a call to
this handle's LogonCancel() member function, then the request completes
with a KErrCancel.

Note that if this is the first notification request after creation of
the change notifier, then this request completes immediately.

@param aStatus A reference to the request status object.

@return KErrInUse, if there is an outstanding request; KErrNone otherwise.

@see TChanges
@see RChangeNotifier::Logon()
*/
	{
	
	aStatus=KRequestPending;
	return(Exec::ChangeNotifierLogon(iHandle,aStatus));
	}




EXPORT_C TInt RChangeNotifier::LogonCancel() const
/**
Cancels an outstanding change notification request.

@return KErrGeneral, if there is no outstanding request; KErrNone otherwise.

@see RChangeNotifier::Logon()
*/
	{
	
	return(Exec::ChangeNotifierLogoff(iHandle));
	}




EXPORT_C void UserSvr::CaptureEventHook()
//
// Capture the event hook
//
	{

	Exec::CaptureEventHook();
	}

EXPORT_C void UserSvr::ReleaseEventHook()
//
// Release the event hook
//
	{

	Exec::ReleaseEventHook();
	}

EXPORT_C void UserSvr::RequestEvent(TRawEventBuf &anEvent,TRequestStatus &aStatus)
//
// Request the next event.
//
	{

	aStatus=KRequestPending;
	Exec::RequestEvent(anEvent,aStatus);
	}

EXPORT_C void UserSvr::RequestEventCancel()
//
// Cancel the event request.
//
	{

	Exec::RequestEventCancel();
	}

/**
Add an event to the queue.

@param anEvent The raw hardware event to be added to the event queue.
@return KErrNone, if successful; KErrPermissionDenied, if the caller has 
insufficient capability; otherwise, one of the other system-wide error codes.

@capability SwEvent
@capability PowerMgmt for ESwitchOff, ERestartSystem, ECaseOpen and ECaseClose
*/
EXPORT_C TInt UserSvr::AddEvent(const TRawEvent& anEvent)
	{

    return(Exec::AddEvent(anEvent));
	}

EXPORT_C void UserSvr::ScreenInfo(TDes8 &anInfo)
//
// Get the screen info.
//
	{

	Exec::HalFunction(EHalGroupDisplay,EDisplayHalScreenInfo,(TAny*)&anInfo,NULL);
	}

#ifdef __USERSIDE_THREAD_DATA__

EXPORT_C TAny* UserSvr::DllTls(TInt aHandle)
//
// Return the value of the Thread Local Storage variable.
//
	{
	return LocalThreadData()->DllTls(aHandle, KDllUid_Default);
	}

EXPORT_C TAny* UserSvr::DllTls(TInt aHandle, TInt aDllUid)
//
// Return the value of the Thread Local Storage variable.
//
	{
	return LocalThreadData()->DllTls(aHandle, aDllUid);
	}

#else

EXPORT_C TAny* UserSvr::DllTls(TInt aHandle)
//
// Return the value of the Thread Local Storage variable.
//
	{

	return Exec::DllTls(aHandle, KDllUid_Default);
	}

EXPORT_C TAny* UserSvr::DllTls(TInt aHandle, TInt aDllUid)
//
// Return the value of the Thread Local Storage variable.
//
	{

	return Exec::DllTls(aHandle, aDllUid);
	}

#endif

EXPORT_C void UserSvr::DllFileName(TInt aHandle, TDes& aFileName)
//
// Return the filename of this dll
//
	{
	TBuf8<KMaxFileName> n8;
	Exec::DllFileName(aHandle, n8);
	aFileName.Copy(n8);
	}

EXPORT_C TBool UserSvr::TestBootSequence()
//
// Is the machine being booted by the test department?
//
    {

	return Exec::HalFunction(EHalGroupPower,EPowerHalTestBootSequence,NULL,NULL);
    }

/**
Register whether the W/S takes care of turning the screen on
*/
EXPORT_C void UserSvr::WsRegisterSwitchOnScreenHandling(TBool aState)
    {

	Exec::HalFunction(EHalGroupDisplay,EDisplayHalWsRegisterSwitchOnScreenHandling,(TAny*)aState,NULL);
    }

EXPORT_C void UserSvr::WsSwitchOnScreen()
//
// W/S switch on the screen
//
    {

	Exec::HalFunction(EHalGroupDisplay,EDisplayHalWsSwitchOnScreen,NULL,NULL);
    }


EXPORT_C TUint32 UserSvr::DebugMask()
/**
Return the kernel debug mask at index 0
*/
    {
	return Exec::DebugMask();
    }


EXPORT_C TUint32 UserSvr::DebugMask(TUint aIndex)
/**
Return the kernel debug mask at the given index position

@param aIndex An index of which 32 bit mask word is to be accessed
*/
    {
	return Exec::DebugMaskIndex(aIndex);
    }



EXPORT_C TTrapHandler *User::TrapHandler()
/**
Gets a pointer to the current thread's trap handler.

Note that TTrapHandler is an abstract base class; a trap handler must be
implemented as a derived class.

@return A pointer to the current thread's trap handler, if any. NULL, if no 
        pre-existing trap handler is set.
*/
	{

	return GetTrapHandler();
	}




EXPORT_C TTrapHandler *User::SetTrapHandler(TTrapHandler *aHandler)
/**
Sets the current thread's trap handler and returns a pointer to any pre-existing 
trap handler.

Pass a NULL pointer to this function to clear the trap handler.

The trap handler works with the TRAP mechanism to handle the effects of a 
leave.

Note that TTrapHandler is an abstract base class; a trap handler must be
implemented as a derived class.

@param aHandler A pointer to the trap handler which is to be installed as 
                the current thread's trap handler.
                
@return A pointer to the current thread's pre-existing trap handler, if any. 
        NULL, if no pre-existing trap handler is set.
        
@see TRAP
@see TRAPD
*/
	{

	TTrapHandler* prev;
#if defined(__USERSIDE_THREAD_DATA__) && defined(__LEAVE_EQUALS_THROW__)
	prev = LocalThreadData()->iTrapHandler;
#else
	prev = Exec::SetTrapHandler(aHandler);
#endif
#ifdef __USERSIDE_THREAD_DATA__
	LocalThreadData()->iTrapHandler = aHandler;
#endif
	return prev;	
	}

#ifndef __LEAVE_EQUALS_THROW__
EXPORT_C TTrapHandler* User::MarkCleanupStack()
/**
If there's a TTrapHandler installed marks the cleanup stack and returns 
the TTrapHandler for subsequent use in UnMarkCleanupStack. 

Only intended for use in the defintion of TRAP and TRAPD and only when 
User::Leave is defined in terms of THROW.

@return A pointer to the current thread's pre-existing trap handler, if any. 
        NULL, if no pre-existing trap handler is set.

@see TRAP
@see TRAPD
*/
	{ 
	return (TTrapHandler*)0; 
	}


EXPORT_C void User::UnMarkCleanupStack(TTrapHandler* /*aHandler*/)
/**
If passed a non-null TTrapHandler unmarks the cleanup stack.

Only intended for use in the defintion of TRAP and TRAPD and only when 
User::Leave is defined in terms of THROW.

@see TRAP
@see TRAPD
*/
	{}

#else

EXPORT_C TTrapHandler* User::MarkCleanupStack()
/**
If there's a TTrapHandler installed marks the cleanup stack and returns 
the TTrapHandler for subsequent use in UnMarkCleanupStack. 

Only intended for use in the defintion of TRAP and TRAPD and only when 
User::Leave is defined in terms of THROW.

@return A pointer to the current thread's pre-existing trap handler, if any. 
        NULL, if no pre-existing trap handler is set.

@see TRAP
@see TRAPD
*/
	{

	TTrapHandler* pH = GetTrapHandler();
	if (pH)
		pH->Trap();
	return pH;
	}

EXPORT_C void User::UnMarkCleanupStack(TTrapHandler* aHandler)
/**
If passed a non-null TTrapHandler unmarks the cleanup stack.

Only intended for use in the defintion of TRAP and TRAPD and only when 
User::Leave is defined in terms of THROW.

@see TRAP
@see TRAPD
*/
	{

	if (aHandler)
		aHandler->UnTrap();
	}

#endif


EXPORT_C TInt User::Beep(TInt aFrequency,TTimeIntervalMicroSeconds32 aDuration)
/**
Makes a beep tone with a specified frequency and duration.

This function should not be used. It exists to maintain compatibility with
older versions of Symban OS.
*/
	{

	return Exec::HalFunction(EHalGroupSound,ESoundHalBeep,(TAny*)aFrequency,(TAny*)aDuration.Int());
	}




// Unused, exists only for BC reasons
EXPORT_C TInt UserSvr::HalGet(TInt, TAny*)
	{
	return KErrNotSupported;
	}

// Unused, exists only for BC reasons
EXPORT_C TInt UserSvr::HalSet(TInt, TAny*)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt UserSvr::HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2)
	{

	return Exec::HalFunction(aGroup, aFunction, a1, a2);
	}

EXPORT_C TInt UserSvr::HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2, TInt aDeviceNumber)
	{

	return Exec::HalFunction(aGroup | (aDeviceNumber<<16), aFunction, a1, a2);
	}

/**
@capability WriteDeviceData
*/
EXPORT_C TInt UserSvr::SetMemoryThresholds(TInt aLowThreshold, TInt aGoodThreshold)
	{
	return Exec::SetMemoryThresholds(aLowThreshold,aGoodThreshold);
	}

/**
@deprecated
@internalAll
@return EFalse
*/
EXPORT_C TBool UserSvr::IpcV1Available()
	{
	return EFalse;
	}



EXPORT_C void User::SetDebugMask(TUint32 aVal)
/**
Sets the debug mask.

@param aVal A set of bit values as defined in nk_trace.h
*/
	{
	Exec::SetDebugMask(aVal);
	}

EXPORT_C void User::SetDebugMask(TUint32 aVal, TUint aIndex)
/**
Sets the debug mask at the given index

@param aVal A set of bit values as defined in nk_trace.h
@param aIndex An index of which 32 bit mask word is to be accessed
*/
	{
	Exec::SetDebugMaskIndex(aVal, aIndex);
	}


/**
Gets machine information.

@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead.
*/
EXPORT_C TInt UserHal::MachineInfo(TDes8& anInfo)
    {
	TInt bufLength=anInfo.MaxLength();
	__ASSERT_ALWAYS(bufLength==sizeof(TMachineInfoV2) || bufLength==sizeof(TMachineInfoV1),Panic(ETDes8BadDescriptorType));

	// assemble a TMachineInfoV1 buffer
	TMachineInfoV2* info=&((TMachineInfoV2Buf&)anInfo)();
	// Variant stuff
	TVariantInfoV01Buf infoBuf;
	TInt r = Exec::HalFunction(EHalGroupVariant, EVariantHalVariantInfo, (TAny*)&infoBuf, NULL);
	if (KErrNone != r) return r;			// must always be implemented!
	TVariantInfoV01& variantInfo = infoBuf();

	info->iRomVersion=variantInfo.iRomVersion;
	info->iMachineUniqueId=variantInfo.iMachineUniqueId;
	info->iLedCapabilities=variantInfo.iLedCapabilities;
	info->iProcessorClockInKHz=variantInfo.iProcessorClockInKHz;
	info->iSpeedFactor=variantInfo.iSpeedFactor;

	// Video driver stuff
	TVideoInfoV01Buf vidinfoBuf;
	r = Exec::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, (TAny*)&vidinfoBuf, NULL);
	if (KErrNone == r)
		{
		TVideoInfoV01& vidinfo = vidinfoBuf();
		info->iDisplaySizeInPixels=vidinfo.iSizeInPixels;
		info->iPhysicalScreenSize=vidinfo.iSizeInTwips;
		}
	else								// no display driver
		{
		info->iDisplaySizeInPixels.iWidth=0;
		info->iDisplaySizeInPixels.iHeight=0;
		info->iPhysicalScreenSize.iWidth=0;
		info->iPhysicalScreenSize.iHeight=0;
		}
	TInt colors = 0;
	r = Exec::HalFunction(EHalGroupDisplay, EDisplayHalColors, &colors, NULL);
	info->iMaximumDisplayColors=(KErrNone == r)?colors:0;
	TInt val;
	info->iBacklightPresent= (KErrNone == Exec::HalFunction(EHalGroupDisplay, EDisplayHalBacklightOn, &val, NULL));

	// Pointing device stuff
	TDigitiserInfoV01Buf xyinfoBuf;
	r = Exec::HalFunction(EHalGroupDigitiser, EDigitiserHalXYInfo, (TAny*)&xyinfoBuf, NULL);
	if (KErrNone == r)
		{
		info->iXYInputType=EXYInputPointer;					// XY is Digitiser
		TDigitiserInfoV01& xyinfo = xyinfoBuf();
		info->iXYInputSizeInPixels=xyinfo.iDigitiserSize;
		info->iOffsetToDisplayInPixels=xyinfo.iOffsetToDisplay;
		}
	else
		{
		TMouseInfoV01Buf mouseinfoBuf;
		r = Exec::HalFunction(EHalGroupMouse, EMouseHalMouseInfo, (TAny*)&mouseinfoBuf, NULL);
		if (KErrNone == r)
			{
			info->iXYInputType=EXYInputMouse;				// XY is Mouse
			TMouseInfoV01& mouseinfo = mouseinfoBuf();
			info->iXYInputSizeInPixels=mouseinfo.iMouseAreaSize;
			info->iOffsetToDisplayInPixels=mouseinfo.iOffsetToDisplay;
			}
		else
			{
			info->iXYInputType=EXYInputNone;				// no XY
			info->iXYInputSizeInPixels.iWidth=0;
			info->iXYInputSizeInPixels.iHeight=0;
			info->iOffsetToDisplayInPixels.iX=0;
			info->iOffsetToDisplayInPixels.iY=0;
			}
		}

	// Keyboard stuff
	TKeyboardInfoV01Buf kbdinfoBuf;
	info->iKeyboardPresent= (KErrNone == Exec::HalFunction(EHalGroupKeyboard, EKeyboardHalKeyboardInfo, (TAny*)&kbdinfoBuf, NULL));

	// Unused, obsolete parameters
	info->iKeyboardId=0;
	info->iDisplayId=0;
	if(bufLength==sizeof(TMachineInfoV2))
		{
		// assemble a TMachineInfoV2 buffer
		info->iLanguageIndex=0;
		info->iKeyboardIndex=0;
		}

	anInfo.SetLength(bufLength);

    return KErrNone;
    }

/**
Gets memory information.

@see HAL::Get()

@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes EMemoryRAM, EMemoryRAMFree or EMemoryROM.
*/
EXPORT_C TInt UserHal::MemoryInfo(TDes8& anInfo)
    {
    return Exec::HalFunction(EHalGroupKernel,EKernelHalMemoryInfo,(TAny*)&anInfo,NULL);
    }

/**
Gets ROM configuration information.

@publishedPartner
@deprecated No replacement.
*/
EXPORT_C TInt UserHal::RomInfo(TDes8& anInfo)
    {
    return Exec::HalFunction(EHalGroupKernel,EKernelHalRomInfo,(TAny*)&anInfo,NULL);
    }




/**
Gets drive information.

@param anInfo A package buffer (TPckgBuf) containing a TDriveInfoV1 structure.
              On return, this structure will contain the drive information.
	
@return KErrNone 

@see TDriveInfoV1Buf
@see TDriveInfoV1
@see TPckgBuf
*/
EXPORT_C TInt UserHal::DriveInfo(TDes8& anInfo)
    {
	TDriveInfoV1Buf8 anInfo8;
    TInt r = Exec::HalFunction(EHalGroupMedia,EMediaHalDriveInfo,(TAny*)&anInfo8,NULL);
	TDriveInfoV18& driveInfo8 = anInfo8();
	TDriveInfoV1* driveInfo = NULL;
	switch(((SBuf8*)&anInfo)->length>>KShiftDesType8) //type
		{
		case EPtr:
			 driveInfo = &((TPckg<TDriveInfoV1>&)anInfo)();
			 break;
		case EBuf:		
			 driveInfo = &((TDriveInfoV1Buf&)anInfo)();
			 break;
		default:
			__ASSERT_ALWAYS(EFalse,Panic(ETDes8BadDescriptorType));
		}

	// A compile time assert to make sure that this function is examined if TDriveInfoV1
	// structure changes
	extern int TDriveInfoV1_structure_assert[(
		_FOFF(TDriveInfoV1,iRegisteredDriveBitmask)+4 == sizeof(TDriveInfoV1)
		&&
		sizeof(TDriveInfoV1) == 816
		)?1:-1];
	(void)TDriveInfoV1_structure_assert;

	// Set length to size of old EKA1 TDriveInfoV1 (Will Panic if not big enough)
	TInt len = (TUint)_FOFF(TDriveInfoV1,iRegisteredDriveBitmask);
	anInfo.SetLength(len);

	// Fill in info for old EKA1 TDriveInfoV1
	driveInfo->iTotalSupportedDrives = driveInfo8.iTotalSupportedDrives;
	driveInfo->iTotalSockets = driveInfo8.iTotalSockets;
	driveInfo->iRuggedFileSystem = driveInfo8.iRuggedFileSystem;
	TInt index;
	for(index=0;index<KMaxLocalDrives;index++)
		driveInfo->iDriveName[index].Copy(driveInfo8.iDriveName[index]);
	for(index=0;index<KMaxPBusSockets;index++)
		driveInfo->iSocketName[index].Copy(driveInfo8.iSocketName[index]);

	// If anInfo is big enough then set new EKA2 members of TDriveInfoV1
	if((TUint)anInfo.MaxLength()>=(TUint)sizeof(TDriveInfoV1))
		{
		anInfo.SetLength(sizeof(TDriveInfoV1));
		driveInfo->iRegisteredDriveBitmask = driveInfo8.iRegisteredDriveBitmask;
		}
	return r;
    }




/**
Gets the startup reason.

@see HAL::Get() 

@publishedPartner
@deprecated Use HAL::Get() from the HAL library instead with attributes ESystemStartupReason.
*/
EXPORT_C TInt UserHal::StartupReason(TMachineStartupType& aReason)
    {
    return Exec::HalFunction(EHalGroupKernel,EKernelHalStartupReason,(TAny*)&aReason,NULL);
    }




/**
Gets the reason why the kernel last faulted.

@param aReason An integer that, on return, contains the reason code describing
               why the kernel faulted. This is the fault number passed 
               in a call to Kern::Fault().

@return KErrNone

@see Kern::Fault()
*/
EXPORT_C TInt UserHal::FaultReason(TInt &aReason)
	{

	return Exec::HalFunction(EHalGroupKernel,EKernelHalFaultReason,(TAny *)&aReason,NULL);
	}




/**
Gets the exception Id that describes the type of fault when
the kernel last faulted.

The Id is the value contained in TArmExcInfo::iExcCode.
 
@param anId An integer that, on return, contains the exception Id.

@return KErrNone

@see TArmExcInfo::iExcCode
@see TArmExcInfo
*/
EXPORT_C TInt UserHal::ExceptionId(TInt &anId)
	{

	return Exec::HalFunction(EHalGroupKernel,EKernelHalExceptionId, (TAny *)&anId, NULL);
	}



/**
Gets the available exception information that describes the last kernel fault.

@param aInfo A TExcInfo structure that, on return, contains the available
             exception information.
             
@return KErrNone

@see TExcInfo        
*/
EXPORT_C TInt UserHal::ExceptionInfo(TExcInfo &aInfo)
	{

	return Exec::HalFunction(EHalGroupKernel,EKernelHalExceptionInfo, (TAny *)&aInfo, NULL);
	}




/**
Gets the page size for this device.

@param anId An integer that, on return, contains the page size, in bytes,
            for this device.

@return KErrNone
*/
EXPORT_C TInt UserHal::PageSizeInBytes(TInt& aSize)
    {

    return Exec::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,(TAny*)&aSize,NULL);
    }




/**
Switches the  device off.

@return KErrNone, if successful; KErrPermissionDenied, if the calling process
        has insufficient capability.

@capability PowerMgmt
*/
EXPORT_C TInt UserHal::SwitchOff()
    {
	if(!RProcess().HasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by UserHal::SwitchOff")))
		return KErrPermissionDenied;
	TInt r = Power::EnableWakeupEvents(EPwStandby);
	if(r!=KErrNone)
		return r;
	TRequestStatus s;
	Power::RequestWakeupEventNotification(s);
	Power::PowerDown();
	User::WaitForRequest(s);
	return s.Int();
//	return Exec::HalFunction(EHalGroupPower,EPowerHalSwitchOff,NULL,NULL);
	}




/**
Sets the calibration data for the digitiser (i.e. XY) input device.

@param aCalibration The calibration data.

@return KErrNone, if successful; KErrPermissionDenied, if the calling process
        has insufficient capability.

@see TDigitizerCalibration

@capability WriteDeviceData
*/
EXPORT_C TInt UserHal::SetXYInputCalibration(const TDigitizerCalibration& aCalibration)
    {
    return Exec::HalFunction(EHalGroupDigitiser,EDigitiserHalSetXYInputCalibration,(TAny*)&aCalibration,NULL);
    }




/**
Gets the points on the display that the user should point to in order
to calibrate the digitiser (i.e. XY) input device.

@param aCalibration A TDigitizerCalibration object that, on return, contains
       the appropriate information.
       
@return KerrNone, if successful; otherwise one of the other system wide
        error codes. 
*/
EXPORT_C TInt UserHal::CalibrationPoints(TDigitizerCalibration& aCalibration)
    {

    return Exec::HalFunction(EHalGroupDigitiser,EDigitiserHalCalibrationPoints,(TAny*)&aCalibration,NULL);
    }




/**
Gets the platform tick period.

@param aTime The tick period in microseconds.

@return KErrNone, if successful; otherwise one of the other system wide
        error codes.
*/
EXPORT_C TInt UserHal::TickPeriod(TTimeIntervalMicroSeconds32 &aTime)
    {

    return Exec::HalFunction(EHalGroupKernel,EKernelHalTickPeriod,(TAny*)&aTime,NULL);
    }



/**
Saves the current digitiser (i.e. XY) input device calibration data.

@return KErrNone, if successful; otherwise one of the other system wide
        error codes, e.g. KErrNotSupported.
*/
EXPORT_C TInt UserHal::SaveXYInputCalibration()
    {

    return Exec::HalFunction(EHalGroupDigitiser,EDigitiserHalSaveXYInputCalibration,NULL,NULL);
    }




/**
Restores the digitiser (i.e. XY) input device calibration data.

@param aType A TDigitizerCalibration object that, on return, contains
       the calibration data.

@return KErrNone, if successful; KErrPermissionDenied, if the calling process
        has insufficient capability; otherwise one of the other system wide
        error codes, e.g. KErrNotSupported.

@capability WriteDeviceData
*/
EXPORT_C TInt UserHal::RestoreXYInputCalibration(TDigitizerCalibrationType aType)
    {
    return Exec::HalFunction(EHalGroupDigitiser,EDigitiserHalRestoreXYInputCalibration,(TAny*)aType,NULL);
	}




/**
Gets the machine configuration.

@param aConfig On return contains the machine configuration data.
@param aSize   On return, contains the size of the data.

@return KErrNone, if sucessful, otherwise one of the other system-wide
        error codes.

@capability ReadDeviceData
*/
EXPORT_C TInt User::MachineConfiguration(TDes8& aConfig,TInt& aSize)
    {
	return(Exec::MachineConfiguration(aConfig,aSize));
    }




EXPORT_C TInt RDebug::Print(TRefByValue<const TDesC> aFmt,...)
//
// Print to the comms port
//
    {

	TestOverflowTruncate overflow;
	// coverity[var_decl]
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> buf;
	// coverity[uninit_use_in_call]
	TRAP_IGNORE(buf.AppendFormatList(aFmt,list,&overflow)); // ignore leave in TTimeOverflowLeave::Overflow()
#ifdef _UNICODE
	TPtr8 p(buf.Collapse());
	Exec::DebugPrint((TAny*)&p, 0);
#else
	Exec::DebugPrint((TAny*)&buf, 0);
#endif
	return 0;
    }

class TestOverflowTruncate8 : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8& /*aDes*/) {}
	};

EXPORT_C void RDebug::Printf(const char* aFmt, ...)
//
// Print to the comms port
//
    {

	TestOverflowTruncate8 overflow;
	// coverity[var_decl]
	VA_LIST list;
	VA_START(list,aFmt);
	TPtrC8 fmt((const TText8*)aFmt);
	TBuf8<0x100> buf;
	// coverity[uninit_use_in_call]
	TRAP_IGNORE(buf.AppendFormatList(fmt,list,&overflow));	
	Exec::DebugPrint((TAny*)&buf, 0);
    }

EXPORT_C void RDebug::RawPrint(const TDesC8& aDes)
	{
	Exec::DebugPrint((TAny*)&aDes, 1);
	}

EXPORT_C void RDebug::RawPrint(const TDesC16& aDes)
//
// Print to the comms port
//
    {
	TBuf8<0x100> aDes8;
	if(aDes.Length()>0x100)
		{
		TPtrC ptr(aDes.Ptr(), 0x100);
		aDes8.Copy(ptr);
		}
	else
		aDes8.Copy(aDes);
	Exec::DebugPrint((TAny*)&aDes8, 1);
	}

EXPORT_C TUint32 Math::Random()
/**
Gets 32 random bits from the kernel's random number generator.

The returned random data may or may not be cryptographically secure but should be of a high quality for
non-cryptographic purposes.

This function uses a cryptographically strong random number generator to generate the random data, which can
be slower than insecure generators. If security is not important, a faster generator may be used such as
Math::Rand().

@return The 32 random bits.
*/
	{

	return Exec::MathRandom();
	}

EXPORT_C void Math::Random(TDes8& aRandomValue)
/**
Fills the provided descriptor with random data up to its current length. The number of random bytes required
should be specified by setting the length of the descriptor that is passed to this function.

The returned random data may or may not be cryptographically secure but should be of a high quality for
non-cryptographic purposes.

This function uses a cryptographically strong random number generator to generate the random data, which can
be slower than insecure generators. If security is not important, a faster generator may be used such as
Math::Rand().

@param aRandomValue on return, the descriptor is filled with the requested number of random bytes.
*/
	{
	Exec::MathSecureRandom(aRandomValue);
    }


EXPORT_C void Math::RandomL(TDes8& aRandomValue)
/**
Fills the provided descriptor with random data up to its current length. The number of random bytes required
should be specified by setting the length of the descriptor that is passed to the function.

If the returned random data cannot be guaranteed to be cryptographically secure, the function will leave with
KErrNotReady to indicate that the returned data should not be used for cryptographic purposes.

The security strength of the cryptographically strong random number generator is 256 bits.

@param aRandomValue  on return, the descriptor is filled with the requested number of random bytes.

@leave KErrNotReady  if the returned random data cannot be guaranteed to be cryptographically secure.
*/
	{
	User::LeaveIfError(Exec::MathSecureRandom(aRandomValue));
	}

EXPORT_C TUint32 Math::RandomL()
/**
Gets 32 random bits from the kernel's random number generator.

If the returned random data could not be guaranteed to be cryptographically secure, the function will instead
leave with KErrNotReady to indicate that data was not available.

The security strength of the cryptographically strong random number generator is 256 bits.

@leave KErrNotReady  if no data was returned as it could not be guaranteed to be cryptographically secure.

@return The 32 random bits.
*/
	{
	TBuf8<sizeof(TUint32)> randomBuffer;
	randomBuffer.SetMax();
	User::LeaveIfError(Exec::MathSecureRandom(randomBuffer));
	return *(TUint32*)(randomBuffer.Ptr());
	}


EXPORT_C void User::IMB_Range(TAny* aStart, TAny* aEnd)
/**
Does the necessary preparations to guarantee correct execution of code in the 
specified virtual address range.

The function assumes that this code has been loaded or modified by user code.
Calling this function against uncommitted memory region is considered as S/W
bug and may generate exception on some memory models.

The specified addresses are associated with a user writable code chunk as 
created by RChunk::CreateLocalCode().

The function cleans the data cache to ensure that written data has been
committed to main memory and then flushes the instruction cache and branch
target buffer (BTB) to ensure that the code is loaded from main memory when
it is executed. 
The Kernel uses the size of the range specified to decide whether to clean/flush 
line-by-line or to simply clean/flush the entire cache.

@param aStart The start virtual address of the region.
@param aEnd   The end virtual address of the region. This location is not within 
              the region.
              
@see RChunk::CreateLocalCode()
@see UserHeap::ChunkHeap()
*/
	{

	Exec::IMB_Range(aStart,(TUint32)aEnd-(TUint32)aStart);
	}




/**
Sets the specified handle into the specified environment data slot
for this process.

The APPARC framework (class CApaApplication etc.) uses some of the slots internally,
so programs that use this framework should ensure that they only use slots available
for public use.

@param aSlot   An index that identifies the environment data slot.
               This is a value relative to zero;
               i.e. 0 is the first item/slot.
               This can range from 0 to 15.
@param aHandle The handle to be passed to this process.

@return KErrNone, always.

@panic KERN-EXEC 46 if this function is called by a thread running
                    in a process that is not the creator of this process, or
                    the handle is not local.
@panic KERN-EXEC 51 if aSlot is negative or is greater than or equal to
                    the value of KArgIndex. 
@panic KERN-EXEC 52 if the specified slot is already in use.

@see CApaApplication
@see CApaCommandLine::EnvironmentSlotForPublicUse()
*/
EXPORT_C TInt RProcess::SetParameter(TInt aSlot, RHandleBase aHandle)
	{
	return Exec::ProcessSetHandleParameter(iHandle, aSlot, aHandle.Handle());
	}




/**
Sets the specified 16-bit descriptor data into the specified environment
data slot for this process.

The APPARC framework (class CApaApplication etc.) uses some of the slots internally,
so programs that use this framework should ensure that they only use slots available
for public use.

@param aSlot   An index that identifies the environment data slot.
               This is a value relative to zero;
               i.e. 0 is the first item/slot.
               This can range from 0 to 15.
@param aDes    The 16-bit descriptor containing data be passed to this process.

@return KErrNone, if successful, otherwise one of the other system
        wide error codes.

@panic KERN-EXEC 46 if this function is called by a thread running
                    in a process that is not the creator of this process.
@panic KERN-EXEC 51 if aSlot is negative or is greater than or equal to
                    the value of KArgIndex. 
@panic KERN-EXEC 52 if the specified slot is already in use.
@panic KERN-EXEC 53 if the length of data passed is negative.

@see CApaApplication
@see CApaCommandLine::EnvironmentSlotForPublicUse()
*/
EXPORT_C TInt RProcess::SetParameter(TInt aSlot, const TDesC16& aDes)
	{
	return Exec::ProcessSetDataParameter(iHandle, aSlot, (const TUint8*)aDes.Ptr(), 2*aDes.Length());
	}




/**
Sets the specified 8-bit descriptor data into the specified environment
data slot for this process.

The APPARC framework (class CApaApplication etc.) uses some of the slots internally,
so programs that use this framework should ensure that they only use slots available
for public use.

@param aSlot   An index that identifies the environment data slot.
               This is a value relative to zero;
               i.e. 0 is the first item/slot.
               This can range from 0 to 15.
@param aDes    The 8-bit descriptor containing data be passed to this process.

@return KErrNone, if successful, otherwise one of the other system
        wide error codes.

@panic KERN-EXEC 46 if this function is called by a thread running
                    in a process that is not the creator of this process.
@panic KERN-EXEC 51 if aSlot is negative or is greater than or equal to
                    the value of KArgIndex. 
@panic KERN-EXEC 52 if the specified slot is already in use.
@panic KERN-EXEC 53 if the length of data passed is negative.

@see CApaApplication
@see CApaCommandLine::EnvironmentSlotForPublicUse()
*/
EXPORT_C TInt RProcess::SetParameter(TInt aSlot, const TDesC8& aDes)
	{
	return Exec::ProcessSetDataParameter(iHandle, aSlot, aDes.Ptr(), aDes.Length());
	}




/**
Sets the specfied sub-session into the specified environment
data slot for this process.

The APPARC framework (class CApaApplication etc.) uses some of the slots internally,
so programs that use this framework should ensure that they only use slots available
for public use.

@param aSlot    An index that identifies the environment data slot.
                This is a value relative to zero;
                i.e. 0 is the first item/slot.
                This can range from 0 to 15.
@param aSession The sub-session.

@return KErrNone, if successful, otherwise one of the other system
        wide error codes.

@panic KERN-EXEC 46 if this function is called by a thread running
                    in a process that is not the creator of this process.
@panic KERN-EXEC 51 if aSlot is negative or is greater than or equal to
                    the value of KArgIndex. 
@panic KERN-EXEC 52 if the specified slot is already in use.
@panic KERN-EXEC 53 if the length of data passed is negative.

@see CApaApplication
@see CApaCommandLine::EnvironmentSlotForPublicUse()
*/
EXPORT_C TInt RProcess::SetParameter(TInt aSlot, const RSubSessionBase& aSession)
	{
	TInt handle = aSession.SubSessionHandle();
	return Exec::ProcessSetDataParameter(iHandle, aSlot, (const TUint8*)&handle, sizeof(handle));
	}




/**
Sets the specfied integer value into the specified environment
data slot for this process.

The APPARC framework (class CApaApplication etc.) uses some of the slots internally,
so programs that use this framework should ensure that they only use slots available
for public use.

@param aSlot   An index that identifies the environment data slot.
               This is a value relative to zero;
               i.e. 0 is the first item/slot.
               This can range from 0 to 15.
@param aData   The integer value.

@return KErrNone, if successful, otherwise one of the other system
        wide error codes.

@panic KERN-EXEC 46 if this function is called by a thread running
                    in a process that is not the creator of this process.
@panic KERN-EXEC 51 if aSlot is negative or is greater than or equal to
                    the value of KArgIndex. 
@panic KERN-EXEC 52 if the specified slot is already in use.
@panic KERN-EXEC 53 if the length of data passed is negative.

@see CApaApplication
@see CApaCommandLine::EnvironmentSlotForPublicUse()
*/
EXPORT_C TInt RProcess::SetParameter(TInt aSlot, TInt aData)
	{
	return Exec::ProcessSetDataParameter(iHandle, aSlot, (TUint8*)&aData, sizeof(aData));
	}




EXPORT_C TInt User::GetTIntParameter(TInt aSlot,  TInt& aData)
/**
Gets the specified environment data item belonging to the
current process; this is assumed to be a 32 bit value.

Environment data may be stored in the process and is passed to a child process
on creation of that child process.

On successful return from this function, the data item is deleted from
the process. 

@param aSlot An index that identifies the data item.
             This is an index whose value is relative to zero;
             i.e. 0 is the first item/slot.
             This can range from 0 to 15, i.e. there are 16 slots. 

@param aData On sucessful return, contains the environment data item.

@return KErrNone, if successful;
        KErrNotFound, if there is no data; 
        KErrArgument, if the data is not binary data, or the data item in the
                      process is longer than 32 bits.
                      
@panic KERN-EXEC 51, if aSlot is negative or is greater than or equal to 16.                                   
*/
	{
	TInt ret = Exec::ProcessGetDataParameter(aSlot, (TUint8*)&aData, sizeof(TInt));
	if (ret < 0)
		return ret;
	return KErrNone;
	}




EXPORT_C TInt User::ParameterLength(TInt aSlot)
/**
Gets the length of the specified item of environment data belonging to the
current process.

Environment data may be stored in the process and is passed to a child process
on creation of that child process.

@param aSlot An index that identifies the data item whose length is to be
             retrieved. This is an index whose value is relative to zero;
             i.e. 0 is the first item/slot.
             This can range from 0 to 15, i.e. there are 16 slots.  
             
@return KErrNotFound, if there is no data; 
        KErrArgument, if the data is not binary data;
        The length of the data item.
             
@panic KERN-EXEC 51, if aSlot is negative or is greater than or equal to 16.             
*/
	{
	TInt ret = Exec::ProcessDataParameterLength(aSlot);
	return ret;
	}




EXPORT_C TInt User::GetDesParameter(TInt aSlot, TDes8& aDes)
/**
Gets the specified environment data item belonging to the
current process; this is assumed to be an 8-bit descriptor.

Environment data may be stored in the process and is passed to a child process
on creation of that child process.

On successful return from this function, the data item is deleted from
the process. 

@param aSlot An index that identifies the data item.
             This is an index whose value is relative to zero;
             i.e. 0 is the first item/slot.
             This can range from 0 to 15, i.e. there are 16 slots. 

@param aDes  On sucessful return, contains the environment data item; the
             length of the descriptor is set to the length of the data item.

@return KErrNone, if successful;
        KErrNotFound, if there is no data; 
        KErrArgument, if the data is not binary data, or the data item in the
                      process is longer than the maximum length of aDes.
                      
@panic KERN-EXEC 51, if aSlot is negative or is greater than or equal to 16.                                   
*/
	{
	TInt ret = Exec::ProcessGetDataParameter(aSlot, (TUint8*)aDes.Ptr(), aDes.MaxLength());
	if (ret < 0)
		return ret;
	aDes.SetLength(ret);
	return KErrNone;
	}




EXPORT_C TInt User::GetDesParameter(TInt aSlot, TDes16& aDes)
/**
Gets the specified environment data item belonging to the
current process; this is assumed to be an 16-bit descriptor.

Environment data may be stored in the process and is passed to a child process
on creation of that child process.

On successful return from this function, the data item is deleted from
the process. 

@param aSlot An index that identifies the data item.
             This is an index whose value is relative to zero;
             i.e. 0 is the first item/slot.
             This can range from 0 to 15, i.e. there are 16 slots. 

@param aDes  On sucessful return, contains the environment data item; the
             length of the descriptor is set to the length of the data item.

@return KErrNone, if successful;
        KErrNotFound, if there is no data; 
        KErrArgument, if the data is not binary data, or the data item in the
                      process is longer than the maximum length of aDes.
                      
@panic KERN-EXEC 51, if aSlot is negative or is greater than or equal to 16.                                   
*/
	{
	TInt ret = Exec::ProcessGetDataParameter(aSlot, (TUint8*)aDes.Ptr(), 2*aDes.MaxLength());
	if (ret < 0)
		return ret;
	aDes.SetLength(ret/2);
	return KErrNone;
	}

/**
Gets the linear address of the exception descriptor for the code module in which
a specified code address resides.

@param	aCodeAddress The code address in question.
@return	The address of the exception descriptor, or zero if there is none.

*/
EXPORT_C TLinAddr UserSvr::ExceptionDescriptor(TLinAddr aCodeAddress)
	{
	return Exec::ExceptionDescriptor(aCodeAddress);
	}

EXPORT_C TInt User::SetFloatingPointMode(TFloatingPointMode aMode, TFloatingPointRoundingMode aRoundingMode)
/**
Sets the hardware floating point mode for the current thread. This does not affect
software floating point calculations. The rounding mode can also be set. New threads created
by this thread will inherit the mode, thus to set the mode for a whole process, call this
method before you create any new threads.

@param aMode         The floating point calculation mode to use.
@param aRoundingMode The floating point rounding mode to use, defaults to nearest.

@return KErrNone if successful, KErrNotSupported if the hardware does not support the
        chosen mode, or there is no floating point hardware present.

@see TFloatingPointMode
@see TFloatingPointRoundingMode
*/
	{
	return(Exec::SetFloatingPointMode(aMode, aRoundingMode));
	}


EXPORT_C TUint32 E32Loader::PagingPolicy()
/**
	Accessor function returns the code paging policy, as defined at ROM build time.

	@return					Code paging policy only.  This function applies
							EKernelConfigCodePagingPolicyMask to the config flags
							before returning the value.
 */
	{
	return Exec::KernelConfigFlags() & EKernelConfigCodePagingPolicyMask;
	}


/** Queue a notifier to detect system idle

@internalTechnology
@prototype
*/
EXPORT_C void User::NotifyOnIdle(TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	Exec::NotifyOnIdle(&aStatus);
	}


/** Cancel a miscellaneous notification requested by this thread

Cancels a currently outstanding notification for system idle or object
deletion.

@internalTechnology
@prototype
*/
EXPORT_C void User::CancelMiscNotifier(TRequestStatus& aStatus)
	{
	Exec::CancelMiscNotifier(&aStatus);
	}


/** Queue a notifier to detect destruction of this object

To cancel the notifier, use User::CancelMiscNotifier().

@internalTechnology
@prototype
*/
EXPORT_C void RHandleBase::NotifyDestruction(TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	Exec::NotifyObjectDestruction(iHandle, &aStatus);
	}

