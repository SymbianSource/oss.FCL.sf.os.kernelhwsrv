// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\collate.h
// Definitions needed for Unicode collation.
// Collation is the comparison of two Unicode strings to produce an ordering
// that may be used in a dictionary or other list.
// Collation is implemented using the Standard Unicode Collation algorithm. There
// are four levels of comparison:
// primary: basic character identity
// secondary: accents and diacritics
// tertiary: upper and lower case, and other minor attributes
// quaternary: Unicode character value
// Punctuation is normally ignored but can optionally be taken into account.
// Strings are fully expanded using the standard Unicode canonical expansions before
// they are compared. Thai and Lao vowels are swapped with the following character
// if any.
// EUSER contains the 'basic collation method'. This method assigns the standard Unicode collation key values
// to the characters in the WGL4 repertoire, plus commonly used control characters and fixed-width spaces, plus
// the CJK ideograms (for which the keys can be generated algorithmically). Other characters are collated after
// all the characters for which keys are defined, and ordered by their Unicode values.
// Locales can supply any number of other collation methods. They will usually supply a 'tailoring' of the standard
// method. This is done by using the standard table as the main key table (signalled by placing NULL in
// TCollationMethod::iMainTable) and specifying an override table (TCollationMethod::iOverrideTable).
// Locale-specific collation data resides in ELOCL.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __COLLATE_H__
#define __COLLATE_H__

#ifdef __KERNEL_MODE__
#include <e32cmn.h>
#else
#include <e32std.h>
#endif

//This material is used in the Unicode build only.
#ifdef _UNICODE

/**
Collation key table structure.
@publishedPartner
@released
*/
struct TCollationKeyTable
	{
public:
	/**
	Masks for the various parts of the elements of the iKey array.
	*/
	enum
		{
		ELevel0Mask = 0xFFFF0000,	// primary key - basic character identity
		ELevel1Mask = 0x0000FF00,	// secondary key - accents and diacritics
		ELevel2Mask = 0x000000FC,	// tertiary key - case, etc.
		EIgnoreFlag = 0x2,			// if set, this key is normally ignored
		EStopFlag = 0x1				// if set, this key is the last in a sequence representing a Unicode value or values
		};

	/**
	An array containing all of the keys and strings of keys concatenated
	together. Each key has EStopFlag set only if it is the last key in its
	string. Eack key contains the keys for levels 0, 1 and 2, and a flag
	EIgnoreFlag if the key is usually ignored (for punctuation & spaces
	etc.).
	*/
	const TUint32* iKey;
	/**
	An array of indices into the iKey array. Each element has its high 16
	bits indicating a Unicode value and its low 16 bits indicating an index
	into the iKey array at which its key starts. For surrogate pairs, high
	surrogate code is in index[i]:16-31, and low surrogate code is in 
	index[i+1]:16-31. These two elements are combined to represent a surrogate
	pair. The elements are sorted by Unicode value.
	*/
	const TUint32* iIndex;
	/**
	The size of the iIndex array.
	*/
	TInt iIndices;
	/**
	Concatenated Unicode strings. Each is a strings that is to be converted
	to keys differently from how it would be if each letter were converted
	independently. An example is "ch" in Spanish, which sorts as though it
	were a single letter. Each Unicode string is preceeded by a 16-bit value
	indicating the string's length (in 16-bit). The end of the string is not 
	delimited. A surrogate pair is represented by two ajacent 16-bit values.
	*/
	const TUint16* iString;
	/**
	An array of elements mapping elements of iString to elements of iIndex.
	Each element has its high 16 bits indicating the index of the start of
	an element of iString, and its low 16 bits indicating the corresponding
	element in iIndex. This array is sorted on the string index.
	*/
	const TUint32* iStringIndex;
	/**
	The size of the iStringIndex array.
	*/
	TInt iStringIndices;
	};

/**
Defines a collation method. 

Collation means sorting pieces of text. It needs to take into account characters, 
accents and case; spaces and punctuation are usually ignored. It differs from 
ordinary methods of sorting in that it is locale-dependent - different 
languages use different ordering methods. Additionally, multiple collation 
methods may exist within the same locale.

A collation method provides the collation keys and other data needed to customise 
collation; the Mem and TDesC16 collation functions (e.g. Mem::CompareC()) 
perform the collation. Note that these functions use the standard collation 
method for the current locale - you only need to specify an object of class 
TCollationMethod to customise this collation scheme. Collation methods can 
be retrieved using member functions of the Mem class. Each one has a unique 
identifier.

A collation method specifies a main table of collation keys, and optionally 
an overriding table that contains keys for which the values in the main table 
are overridden. A collation key table (TCollationKeyTable) is the set of collation 
keys: primary (basic character identity), secondary (accents and diacritics) 
and tertiary (case). The quaternary key is the Unicode character values themselves.

The simplest way to customise a collation method is to create a local copy 
of the standard collation method and change it. For example, you could use 
the standard method, but not ignore punctuation and spaces:

@code
TCollationMethod m = *Mem::CollationMethodByIndex(0); // get the standard method
m.iFlags |= TCollationMethod::EIgnoreNone; // dont ignore punctuation and spaces
@endcode

@publishedPartner
@released
*/
struct TCollationMethod
	{
	public:
	/**
	The UID of this collation method.
	*/
	TUint iId;
	
	/**
	The main collation key table; if NULL, use the standard table.
	*/
	const TCollationKeyTable* iMainTable;
	
	/**
	If non-NULL, tailoring for collation keys.
	*/
	const TCollationKeyTable* iOverrideTable;
	enum
		{
		/**
		Don't ignore any keys (punctuation, etc. is normally ignored).
		*/
		EIgnoreNone = 1,
		
		/**
		Reverse the normal order for characters differing only in case
		*/
		ESwapCase = 2,
		
		/**
		Compare secondary keys which represent accents in reverse
		order (from right to left); this is needed for French when comparing
		words that differ only in accents.
		*/
		EAccentsBackwards = 4,	
		
		/**
		Reverse the normal order for characters differing only in whether they
		are katakana or hiragana.
		*/
		ESwapKana = 8,
		
		/**
		Fold all characters to lower case before extracting keys; needed for
		comparison of filenames, for which case is ignored but other
		tertiary (level-2) distinctions are not.
		*/
		EFoldCase = 16,
		
		/** Flag to indicate a collation method for matching purpose 
		This flag is only needed if we wish to specify a particular collation method
		to be used for matching purpose.
		*/
		EMatchingTable = 32,
		
		/** Ignore the check for adjacent combining characters.  A combining
		character effectively changes the character it combines with to something
		else and so a match doesn't occur.  Setting this flag will allow character
		matching regardless of any combining characters.
		*/
		EIgnoreCombining = 64
		};
		
	/**
	Flags.
	
	@see TCollationMethod::EIgnoreNone
	@see TCollationMethod::ESwapCase
	@see TCollationMethod::EAccentsBackwards
	@see TCollationMethod::ESwapKana
	@see TCollationMethod::EFoldCase
	*/
	TUint iFlags;
	};

/**
A collation data set provides any collation methods needed by a locale.
@publishedPartner
@released
*/
struct TCollationDataSet
	{
	public:
	const TCollationMethod* iMethod;
	TInt iMethods;
	};

// Collation method IDs

/**
A collation data set provides any collation methods needed by a locale.
@internalTechnology
@released
*/
const TUint KUidBasicCollationMethod = 0x10004F4E;

/**
A collation data set provides any collation methods needed by a locale.
@internalTechnology
@released
*/
const TUint KUidStandardUnicodeCollationMethod = 0x10004E96;

#ifndef __KERNEL_MODE__

//Forward declarations
class TUTF32Iterator;
struct LCharSet;

/**
Provides low-level collation functions.
@internalComponent
@released
*/
class TCollate
	{
public:
	/**
	Construct a TCollate object based on the collation method specified
	within aCharSet, if any. If there is none, or aCharSet is null, the
	standard collation method will be used. aMask and aFlags provide a
	method for overriding the flags in the collation method: Each flag set
	to 1 in aMask is a flag that will be overridden and set to the
	corresponding flag value in aFlags. Ownership of aCharSet is not passed.
	*/
	TCollate(const LCharSet* aCharSet,TUint aMask = 0,TUint aFlags = 0xFFFFFFFF);
	/**
	Construct a TCollate object based on an already constructed
	TCollationMethod specified in aMethod. Ownership is not passed.
	*/
	TCollate(const TCollationMethod& aMethod);

	enum TComparisonResult
		{
		ELeftComparesLessAndIsNotPrefix = -2,
		ELeftIsPrefixOfRight = -1,
		EStringsIdentical = 0,
		ERightIsPrefixOfLeft = 1,
		ERightComparesLessAndIsNotPrefix = 2
		};

	/**
	Compare the string beginning at aString1 of length aLength1 against the
	string beginning at aString2 of length aLength2.
	aMaxLevel determines the tightness of the collation. At level 0, only
	character identities are distinguished. At level 1 accents are
	distinguished as well. At level 2 case is distinguishes as well. At
	level 3 all valid different Unicode characters are considered different.
	*/
	TComparisonResult Compare(const TUint16* aString1,TInt aLength1,
							  const TUint16* aString2,TInt aLength2,
							  TInt aMaxLevel = 3) const;
	/**
	Find the string beginning at aString2 of length aLength2 in the string
	beginning at aString1 of length aLength1. aMaxLevel determines
	the tightness of the collation, see Compare for details.
	*/
	TInt Find(const TUint16 *aString1,TInt aLength1,const TUint16 *aString2,TInt aLength2,
			  TInt aMaxLevel,TUint aString2WildChar = 0) const;
			  
	TInt Find(const TUint16 *aString1,TInt aLength1,const TUint16 *aString2,TInt aLength2,
		      TInt &aLengthFound,TInt aMaxLevel,TUint aString2WildChar = 0) const;
		      
	/**
	Test if the string beginning at aSearchTerm of length aSearchTermLength
	matches the string beginning at aCandidate of length aCandidateLength.
	aMaxLevel determines the tightness of the collation, see
	Compare for details. The search term may have wild card characters as
	specified by aWildChar (for matching a single grapheme- i.e. character
	and any characters that combine with it, such as accents) and
	aWildSequenceChar (for matching any sequence of whole graphemes). The
	return value is KErrNotFound iff the search term does not match the
	candidate string exactly. To find a match within the candidate string,
	the search term must begin and end with a wild sequence character. If
	the search term does match the candidate string, 0 will be returned,
	unless the first character of the search term is a wild sequence
	character in which case the value returned will be the index into
	aCandidate at which the first non-wild sequence character matched.
	aWildSequenceChar must be a valid (non-surrogate) Unicode character
	below FFFE.
	*/
	TInt Match(const TUint16 *aCandidate, TInt aCandidateLength,
			   const TUint16 *aSearchTerm,TInt aSearchTermLength,
			   TInt aMaxLevel, TUint aWildChar = '?', TUint aWildSequenceChar = '*', TUint aEscapeChar = 0) const;

private:
	/**
	Compare values output from the iterators. After the comparison, if
	ERightIsPrefixOfLeft or EStringsIdentical is returned, then aLeft and
	aRight will be pointing at the next key (at MaxLevel) after the match.
	If right is shown to be a prefix of left, this means that it has been
	checked at all requested levels. If it is reported that the right is a
	prefix of the left, then this will mean also that there are no unmatched
	combining characters on the left.
	*/
	TComparisonResult CompareKeySequences(TUTF32Iterator& aLeft, TUTF32Iterator& aRight,
										  TInt aMaxLevel, TInt aRightStringWildChar, TInt aEscapeChar) const;
	/**
	Finds search term inside candidate string. Returns KErrNotFound if there
	is no match, returns the offset into the candidate string at which the
	search term was found (note that this is the offset from the start of
	the iteration, not from where the iteration was when the function was
	called). If a string was found, the search term iterator is left
	pointing at the end of the search term, and the candidate iterator is
	left pointing just after the matched keys. aMatchPos returns where in
	the candidate string the match was found.
	*/
	TInt FindKeySequence(TUTF32Iterator& aCandidate, TUTF32Iterator& aSearchTerm,
						 TInt aMaxLevel, TInt aWildChar, TInt aEscapeChar, TInt& aLengthFound) const;

private:
	TCollationMethod iMethod;
	};

#endif	// __KERNEL_MODE__

#endif // _UNICODE

#endif // __COLLATE_H__
