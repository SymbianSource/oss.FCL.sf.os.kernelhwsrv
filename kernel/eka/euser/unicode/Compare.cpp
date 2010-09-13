// Copyright (c) 2001-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Folding and decomposition implementation
// 
//

#include "FoldDecomp.inl"
#include "CompareImp.h"
#include "u32std.h"

#define ARRAY_LENGTH(a) (static_cast<TInt>(sizeof(a)/sizeof(a[0])))

////////////////////////////////////////////////////////////////////////////////////////////
// Global functions
////////////////////////////////////////////////////////////////////////////////////////////

/**
Is a character a base character (ETrue) or a combiner (EFalse)?
For now, we will treat all control characters as base characters.
@internalComponent
*/
TBool IsBaseCharacter(TChar a)
	{
	if(a < 0x220)
        {
		// These Unicode characters are all assigned
		// and none of them is a combining character
		return ETrue;
        }
	return (a.GetCategory() & 0xFFF0) - TChar::EMarkGroup;
	}

/**
@internalComponent
*/
inline TInt GetDecompositionIndex(TChar a)
	{
    TInt i = DecompositionHashStart(a);
	TUint32 v = KUnicodeToIndexHash[i];
	if (!v)
		return -1;
	if ((v & 0xFFFFF) != a)
		{
        TInt step = DecompositionHashStep(a);
		do	{
			i = (i + step) & KDecompositionHashBitmask;
			v = KUnicodeToIndexHash[i];
			if (!v)
				return -1;
			} while ((v & 0xFFFFF) != a);
		}
// it is important that this is an unsigned shift
	return static_cast<TInt>(v >> 20);
	}

/**
Will not work if an invalid index is passed.
@internalComponent
*/
static TUTF32Iterator GetFoldedDecomposition(TInt aIndex)
	{
	TInt singletonIndex = aIndex - (sizeof(KNonSingletonFolds)/sizeof(KNonSingletonFolds[0])/2);
	if (0 <= singletonIndex)
		return TUTF32Iterator(KSingletonFolds + singletonIndex);
	const TText* start = KNonSingletonFolds + aIndex * 2;
	if (*start != KLongD)
		return TUTF32Iterator(start, start + 2,
			TUTF32Iterator::EStartsWithValidCharacter);
	TInt longDecompIndex = start[1];
	start = KLongDecompositions + (longDecompIndex & 0xFFF);
	return TUTF32Iterator(start, start + (longDecompIndex >> 12) + 3,
		TUTF32Iterator::EStartsWithValidCharacter);
	}

/**
@internalComponent
*/
static TChar GetFirstFoldedChar(TChar a)
	{
    TInt index = ::GetDecompositionIndex(a);
    return index < 0? a : ::GetFoldedDecomposition(index).Current();
	}

/**
@internalComponent
*/
static TBool FirstFoldedCodeIsNot(TInt aNonSurrogate, TInt aFoldedNonSurrogate)
	{
    TInt index = ::GetDecompositionIndex(aNonSurrogate);
	if (index < 0)
		return aNonSurrogate - aFoldedNonSurrogate;
	TInt singletonIndex = index - (sizeof(KNonSingletonFolds)/sizeof(KNonSingletonFolds[0])/2);
	if (0 < singletonIndex)
		return KSingletonFolds[singletonIndex] - aFoldedNonSurrogate;
	const TText* start = KNonSingletonFolds + index * 2;
	if (start[0] == KLongD)
		start = KLongDecompositions + (start[1] & 0xFFF);
	return *start - aFoldedNonSurrogate;
	}

/**
@internalComponent
*/
static TInt GetClass(TFoldedDecompIterator& a)
	{
	ASSERT(!a.AtEnd());
	a.EnterFoldedSequence();
	TChar ch = a.Current();
	TInt cl = ch.GetCombiningClass();
	if (cl == 0)
		// Assume starters have been folded from ypogegrammeni
		cl = 240;
	return cl;
	}

////////////////////////////////////////////////////////////////////////////////////////////
// TUTF32Iterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
Locates a base character in a string using a folded comparision. Will not find combining 
characters, nor will it consider Korean combining Jamo to be equivalent to Hangul.
@internalComponent
*/
TBool TUTF32Iterator::LocateFoldedBaseCharacter(TChar aChar)
	{
	// A quick shortcut for simple rejections
	if (aChar < 0xFFFF)
		{
        while (iStart != iEnd && *iStart < 0xD800 && ::FirstFoldedCodeIsNot(*iStart, aChar))
			++iStart;
		if (iStart != iEnd)
			{
            iCurrent = ::UTF16ToChar(iStart);
			if (iCurrent == 0xFFFF)
				Next();
			}
		}
	while (!AtEnd())
		{
        TChar foldedChar = ::GetFirstFoldedChar(iCurrent);
		if (aChar == foldedChar)
			return ETrue;
		Next();
		}
	return EFalse;
	}

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedDecompIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
TFoldedDecompIterator::TFoldedDecompIterator(const TUTF32Iterator& a)
	{
	Set(a);
	}

/**
@internalComponent
*/
TBool TFoldedDecompIterator::AtEndOrWildcard() const
	{
	// neither '?' nor '*' have decomposition sequences, so we can assume that
	// if we are pointing at one or the other, we don't need to check if we
	// are in a decomposition sequence or not.
	return iOriginal.AtEnd() || iOriginal.Current() == '?' || iOriginal.Current() == '*';
	}

/**
@internalComponent
*/
TBool TFoldedDecompIterator::EnterFoldedSequence()
	{
	ASSERT(!AtEnd());
	return !IsInFoldedSequence() && StrictEnterFoldedSequence();
	}

/**
Enter folded sequence, assuming that we are not already in one
@internalComponent
*/
TBool TFoldedDecompIterator::StrictEnterFoldedSequence()
	{
	ASSERT(!AtEnd());
	ASSERT(!IsInFoldedSequence());
    TInt index = ::GetDecompositionIndex(iOriginal.Current());
	if (index < 0)
		return EFalse;
	iFolded = ::GetFoldedDecomposition(index);
	return ETrue;
	}

/**
An iota might have folded from a combining ypogegrammeni.
If the current character is a base character, this function will
detect whether it was folded from a combining character (and
therefore does not constitute a grapheme boundary).
@internalComponent
*/
TBool TFoldedDecompIterator::CurrentIsBaseFoldedFromCombiner() const
	{
	if (!IsInFoldedSequence())
		return EFalse;
	// The only character that does this is Ypogerammeni, which folds to iota
	if (iFolded.Current() != 0x3B9)
		return EFalse;
	// If the unfolded character is a combiner then it cannot contain an iota,
	// so it must be an ypogegrammeni that has been folded to one.
	// This will catch 0x345, the ypogegrammeni itself.
    if (!::IsBaseCharacter(iOriginal.Current()))
		return ETrue;
	// Otherwise the base character will be at the start of the decomposition
	// sequence. We will assume that it is folded from a genuine iota if and
	// only if there is an iota at the start of the folded decomposition
	// sequence. (In theory there might be an iota with a combining
	// ypogegrammeni, but in practice this will not occur).
	TInt index = ::GetDecompositionIndex(iOriginal.Current());
	ASSERT(0 <= index);
	TUTF32Iterator folded = ::GetFoldedDecomposition(index);
	return folded.Current() != 0x3B9;
	}

/** 
Move past this code if it matches unfolded or folded 
@internalComponent
*/
TBool TFoldedDecompIterator::Match(TChar aCode)
	{
	ASSERT(!AtEnd());
	if (!IsInFoldedSequence())
		{
		if (aCode == iOriginal.Current())
			{
			iOriginal.Next();
			return ETrue;
			}
		if (!StrictEnterFoldedSequence())
			return EFalse;
		}
	ASSERT(IsInFoldedSequence());
	if (aCode == iFolded.Current())
		{
		iFolded.Next();
		if (iFolded.AtEnd())
			iOriginal.Next();
		return ETrue;
		}
	return EFalse;
	}

/** 
Move this and argument past matching character. 
@internalComponent
*/
TBool TFoldedDecompIterator::Match(TFoldedDecompIterator& aThat)
	{
	ASSERT(!AtEnd());
	if (!IsInFoldedSequence())
		{
		if ( aThat.Match(iOriginal.Current()) )
			{
			iOriginal.Next();
			return ETrue;
			}
		if (!StrictEnterFoldedSequence())
			return EFalse;
		}
	ASSERT(IsInFoldedSequence());
	if ( aThat.Match(iFolded.Current()) )
		{
		iFolded.Next();
		if (iFolded.AtEnd())
			iOriginal.Next();
		return ETrue;
		}
	return EFalse;
	}

/** 
@internalComponent
*/
void TFoldedDecompIterator::Next()
	{
	ASSERT(!AtEnd());
	if (IsInFoldedSequence())
		{
		iFolded.Next();
		if (IsInFoldedSequence())
			return;
		}
	iOriginal.Next();
	}

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedSortedDecompIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
Set this iterator to iterate over the next combining characters.
Iotas in folded sequences are assumed to be character class 240
(combining ypogegrammeni). Next() must be used once before
the first call to Current(), as long as AtEnd() returns false.
@param aBase Sets the start of the iteration. This value is advanced to the
             end of the iteration.
@return The number of codes in the iteration.
@internalComponent
*/
TInt TFoldedSortedDecompIterator::Set(TFoldedDecompIterator &aBase)
	{
	iStart = aBase;
	TInt length = 0;
	iCurrentClass = 256;

	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;

	// Find the next starter (i.e. character with combining class 0).
	// We must not count iota folded from ypogegrammeni.
	// Ypogegrammeni has combining class 240.
	// Iota has combining class 0.
	// Also we will be searching for the character with the smallest
	// combining class to start at.
	while (!aBase.AtEnd())
		{
		aBase.EnterFoldedSequence();
		TChar ch = aBase.Current();
		TInt cl = TUnicode(TUint(ch)).GetCombiningClass(charDataSet);
		if (cl == 0)
			{
			if (aBase.CurrentIsBaseFoldedFromCombiner())
				cl = 240;
			else
				break;
			}
		if (cl < iCurrentClass)
			{
			iCurrentClass = cl;
			iCurrent = aBase;
			iCurrentCount = length;
			}
		++length;
		aBase.Next();
		}
	iRemaining = length;
	iLength = length;
	ASSERT(iLength == 0 || iCurrentClass < 256);
	return length;
	}

/** 
Set this iterator so that AtEnd() returns ETrue. 
@internalComponent
*/
void TFoldedSortedDecompIterator::Set()
	{
	iRemaining = 0;
	}

/** 
@internalComponent
*/
void TFoldedSortedDecompIterator::Next()
	{
	ASSERT(!AtEnd());
	--iRemaining;
	while (++iCurrentCount != iLength)
		{
		iCurrent.Next();
        if (::GetClass(iCurrent) == iCurrentClass)
			return;
		}
	// We have not found any more of the same class, so we will look
	// for the smallest one larger.
	TInt minClass = 256;
	TFoldedDecompIterator searcher(iStart);
	TInt searchCount = 0;
	while (searchCount != iLength)
		{
        TInt cl = ::GetClass(searcher);
		if (iCurrentClass < cl && cl < minClass)
			{
			minClass = cl;
			iCurrentCount = searchCount;
			iCurrent = searcher;
			}
		++searchCount;
		searcher.Next();
		}
	iCurrentClass = minClass;
	ASSERT(minClass < 256 || AtEnd());
	}

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedCanonicalIterator
////////////////////////////////////////////////////////////////////////////////////////////

/** 
@internalComponent
*/
TFoldedCanonicalIterator::TFoldedCanonicalIterator(const TUTF32Iterator& a)
	{
	iBase.Set(a);
	iSorted.Set();
    if(!iBase.AtEnd())
        {
	    iBase.EnterFoldedSequence();
	    if (iBase.Current().GetCombiningClass() != 0 || iBase.CurrentIsBaseFoldedFromCombiner())
            {
		    iSorted.Set(iBase);
            }
        }
	}

/** 
@internalComponent
*/
void TFoldedCanonicalIterator::Next(const TUnicodeDataSet* aCharDataSet)
	{
	ASSERT(!iBase.AtEnd() || !iSorted.AtEnd());
	if (!iSorted.AtEnd())
		{
		iSorted.Next();
		return;
		}
	iBase.Next();
    if(!iBase.AtEnd())
        {
	    iBase.EnterFoldedSequence();
 	    if (TUnicode(TUint(iBase.Current())).GetCombiningClass(aCharDataSet) != 0 || iBase.CurrentIsBaseFoldedFromCombiner())
           {
		    iSorted.Set(iBase);
            }
        }
	}

////////////////////////////////////////////////////////////////////////////////////////////
// Folding - Global functions and structures
////////////////////////////////////////////////////////////////////////////////////////////

/** 
@internalComponent
*/
struct TEndTester 
    { 
    typedef enum {EGenuine, EWildcard} TType;

    inline TEndTester(TType aType) :
        iType(aType)
        {
        }

    inline TBool operator()(const TFoldedDecompIterator& a) const
        {
        return iType == EGenuine ? a.AtEnd() : a.AtEndOrWildcard();
        } 

private:
    TType iType;

    };

/**
Consumes as much of aCandidate as matches aSearchTerm up to the next '?' or
'*' wildcard or the end of aSearchTerm.
It is assumed that the search term begins with a base character.
Returns true if and only if the whole search term was matched
with a whole number of UTF16s in the candidate string.
On return of ETrue the candidate string iterator will have consumed the
matching characters the search term will have had all its matching characters
consumed.
@internalComponent
*/
TBool ConsumeFoldedMatch(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm,
                         const TEndTester& aEndTester)
	{
	TBool assumeBase = ETrue;
	TFoldedDecompIterator st(aSearchTerm);
	TFoldedDecompIterator cs(aCandidateString);
	while (!aEndTester(st))
		{
		if (cs.AtEnd())
			return EFalse;
		if (st.Match(cs))
			{
			assumeBase = EFalse;
			if (aEndTester(st))
				{
                // We have a match...
                if (cs.IsInFoldedSequence())
                    // but it was against only part of a character
                    // in the original string, so we fail.
                    return EFalse;
				aCandidateString = cs.BaseIterator();
				aSearchTerm = st.BaseIterator();
				return ETrue;
				}
			continue;
			}
		// did not match. We need to re-order canonically.
		if (assumeBase)
			// The first characters did not match.. do not bother
			// to re-order.
			return EFalse;
		TFoldedSortedDecompIterator csc;
		TInt cscLength = csc.Set(cs);
		if (cscLength < 2)
			// If there are less than two characters to be reordered,
			// there is no hope of getting a match by re-ordering
			return EFalse;
		TFoldedSortedDecompIterator stc;
		if (cscLength != stc.Set(st))
			// if the strings have differing numbers of characters,
			// there can be no match
			return EFalse;
		while (!stc.AtEnd())
			{
			ASSERT(!csc.AtEnd());
			if (stc.Current() != csc.Current())
				// well, we tried.
				return EFalse;
			stc.Next();
			csc.Next();
			}
		}
	// If the candidate string is in a folded sequence, then
	// we should not accept the match, as we require all matches
	// to be for a whole number of characters in the original string.
	if (cs.IsInFoldedSequence())
		return EFalse;
	aCandidateString = cs.BaseIterator();
	aSearchTerm = st.BaseIterator();
	return ETrue;
	}

/** 
@internalComponent
*/
TBool ConsumeFoldedMatchWholeGraphemes(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm, 
                                       const TEndTester& aEndTester)
	{
    if (!::ConsumeFoldedMatch(aCandidateString, aSearchTerm, aEndTester))
		return EFalse;
    return aCandidateString.AtEnd() || ::IsBaseCharacter(aCandidateString.Current());
	}

/** 
@internalComponent
*/
static TBool ConsumeGrapheme(TUTF32Iterator& a)
	{
	if (a.AtEnd())
		return EFalse;
	a.Next();
    while (!a.AtEnd() && !::IsBaseCharacter(a.Current()))
		a.Next();
	return ETrue;
	}

/** 
@internalComponent
*/
TBool MatchSectionFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm)
	{
    TEndTester endTester(TEndTester::EWildcard);
    while(::ConsumeFoldedMatchWholeGraphemes(aCandidateString, aSearchTerm, endTester))
		{
		if (aSearchTerm.AtEnd() || aSearchTerm.Current() == '*')
			return ETrue;
		ASSERT(aSearchTerm.Current() == '?');
		aSearchTerm.Next();
        if (!::ConsumeGrapheme(aCandidateString))
			return EFalse;
		}
	return EFalse;
	}

/** 
@internalComponent
*/
TBool FindMatchSectionFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm)
	{
	// match as many graphemes as there are leading ?s
	while (!aSearchTerm.AtEnd()
		&& aSearchTerm.Current() != '*' && aSearchTerm.Current() == '?')
		{
        if (!::ConsumeGrapheme(aCandidateString))
			return EFalse;
		aSearchTerm.Next();
		}
	if (aSearchTerm.AtEnd() || aSearchTerm.Current() == '*')
		return ETrue;
    TChar firstCharOfSearchTerm = ::GetFirstFoldedChar(aSearchTerm.Current());
	TUTF32Iterator savedST(aSearchTerm);
	while (aCandidateString.LocateFoldedBaseCharacter(firstCharOfSearchTerm))
		{
		TUTF32Iterator savedCS = aCandidateString;
		if (::MatchSectionFolded(aCandidateString, aSearchTerm))
			return ETrue;
		aSearchTerm = savedST;
		aCandidateString = savedCS;
		aCandidateString.Next();
		}
	return EFalse;
	}

/** 
@internalComponent
*/
TBool MatchStringFolded(const TText16* aCandidateStringStart, const TText16* aCandidateStringEnd,
                        const TText16* aSearchTermStart, const TText16* aSearchTermEnd)
	{
	TUTF32Iterator candidate(aCandidateStringStart, aCandidateStringEnd);
	TUTF32Iterator searchTerm(aSearchTermStart, aSearchTermEnd);
	// First section of search term must match exactly at the start of the
	// candidate string.
	if (!::MatchSectionFolded(candidate, searchTerm))
		return EFalse;

	// If there was only one section, it must match the whole candidate string.
	if (searchTerm.AtEnd())
		return candidate.AtEnd();

	while (!searchTerm.AtEnd())
		{
		searchTerm.Next();
		if (!::FindMatchSectionFolded(candidate, searchTerm))
			return EFalse;
		}

	// The last section must match exactly at the end of the candidate string.
	if (candidate.AtEnd())	// shortcut
		return ETrue;
	const TText16* searchTermLastSection = aSearchTermEnd;
	while (searchTermLastSection != aSearchTermStart
		&& searchTermLastSection[-1] != '*')
		--searchTermLastSection;
	if (searchTermLastSection == aSearchTermEnd)
		// last section is null, so trivially matches
		return ETrue;
	// Count graphemes by counting the number of base characters.
	// The first one is assumed to start a grapheme.
	TInt graphemeCount = 1;
	for (const TText16* s = searchTermLastSection + 1; s != aSearchTermEnd; ++s)
		{
        if (::IsBaseCharacter(*s))
			++graphemeCount;
		}
	// Count this many graphemes back in the candidate string
	const TText16* candidateLastSection = aCandidateStringEnd;
	while (graphemeCount != 0
		&& candidateLastSection != aCandidateStringStart)
		{
        if (::IsBaseCharacter(*--candidateLastSection))
			--graphemeCount;
		}
	TUTF32Iterator last(candidateLastSection, aCandidateStringEnd);
	TUTF32Iterator st(searchTermLastSection, aSearchTermEnd);
	return ::MatchSectionFolded(last, st);
	}

/** 
Implementation of MatchF
(slow if there is a match: MatchStringFolded is better, but does not return
the position of the match)
@internalComponent
*/
TInt LocateMatchStringFolded(const TText16* aCandidateStringStart, const TText16* aCandidateStringEnd,
                             const TText16* aSearchTermStart, const TText16* aSearchTermEnd)
	{
	// Quick shortcut for simple non-match
	if (aSearchTermStart != aSearchTermEnd && *aSearchTermStart != '*')
		{
		if (aCandidateStringStart == aCandidateStringEnd)
			return KErrNotFound;
		// We won't bother with non-characters and surrogate pairs.
		if (*aSearchTermStart != '?'
			&& *aSearchTermStart < 0xD800
			&& *aCandidateStringStart < 0xD800
            && ::GetFirstFoldedChar(*aSearchTermStart) != ::GetFirstFoldedChar(*aCandidateStringStart))
			return KErrNotFound;
		}
    if (!::MatchStringFolded(aCandidateStringStart, aCandidateStringEnd,
		aSearchTermStart, aSearchTermEnd))
		return KErrNotFound;
	// find where it matches
	while (aSearchTermStart != aSearchTermEnd && *aSearchTermStart == '*')
		++aSearchTermStart;
	const TText16* end = aSearchTermStart;
	while (end != aSearchTermEnd && *end != '*')
		++end;
	// To preserve existing behaviour, a search term consisting of nothing
	// but stars is considered to match at 0.
	if (end == aSearchTermStart)
		return 0;
	for (const TText16* csSection = aCandidateStringStart;
		csSection <= aCandidateStringEnd; ++csSection)
		{
		TUTF32Iterator cs(csSection, aCandidateStringEnd);
		TUTF32Iterator st(aSearchTermStart, end);
        if (::MatchSectionFolded(cs, st))
			{
			// If this match must match exactly at the end, we must keep
			// going.
			// This could be a lot faster, with some optimizations.
			if (end == aSearchTermEnd)
				{
				if (!cs.AtEnd())
					continue;
				}
			return csSection - aCandidateStringStart;
			}
        // Because this function is using TUTF32Iterator, which means the
        // original author want to support surrogate. Take it as a defect and
        // fix it, while do not define a new LocateMatchStringFoldedSurrogate().
        if (TChar::IsSurrogate(*csSection))
        	++csSection;
		}
	// this should never happen!
	ASSERT(0);
	return KErrNotFound;
	}

/** 
Implementation of FindF
@internalComponent
*/
TInt FindFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm)
	{
    //Empty aSearchTerm string? - Then return 0 - keep the new implementation functionally 
    //compatible with the old one.
    if(aSearchTerm.Length() == 0)
        {
        return 0;
        }
	const TText16* candidateStartPosition = aCandidateString.CurrentPosition();
    TChar firstCharOfSearchTerm = ::GetFirstFoldedChar(aSearchTerm.Current());
	TUTF32Iterator savedST(aSearchTerm);
	while (aCandidateString.LocateFoldedBaseCharacter(firstCharOfSearchTerm))
		{
		TUTF32Iterator savedCS = aCandidateString;
        TEndTester endTester(TEndTester::EGenuine);
        if (::ConsumeFoldedMatchWholeGraphemes(aCandidateString, aSearchTerm, endTester))
			return savedCS.CurrentPosition() - candidateStartPosition;
		aSearchTerm = savedST;
		aCandidateString = savedCS;
		aCandidateString.Next();
		}
	return KErrNotFound;
	}

/** 
Implementation of boolean CompareF
@internalComponent
*/
TBool EqualsFolded(TUTF32Iterator& aLeft, TUTF32Iterator& aRight)
	{
    TEndTester endTester(TEndTester::EGenuine);
    if (::ConsumeFoldedMatchWholeGraphemes(aLeft, aRight, endTester))
		return aLeft.AtEnd();
	return EFalse;
	}

/** 
Implementation of tri-state CompareF
@internalComponent
*/
TInt CompareFolded(const TUTF32Iterator& aLeft, const TUTF32Iterator& aRight)
	{
	TFoldedCanonicalIterator left(aLeft);
	TFoldedCanonicalIterator right(aRight);

	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;

	while (!left.AtEnd())
		{
		if (right.AtEnd())
			return 1;
		TChar cr = right.Current();
		TChar cl = left.Current();
		if (cr != cl)
			return cl - cr;
        right.Next(charDataSet);
        left.Next(charDataSet);
		}
	return right.AtEnd()? 0 : -1;
	}

////////////////////////////////////////////////////////////////////////////////////////////
// Composition/Decomposition - Global functions and structures
////////////////////////////////////////////////////////////////////////////////////////////

/** 
@internalComponent
*/
static TUTF32Iterator IndexToNonSingletonDecomposition(TInt aIndex)
	{
	const TText* start = KNonSingletonDecompositions + aIndex * 2;
	if (*start != KLongD)
		return TUTF32Iterator(start, start + 2, TUTF32Iterator::EStartsWithValidCharacter);
	TInt longDecompIndex = start[1];
	start = KLongDecompositions + (longDecompIndex & 0xFFF);
	return TUTF32Iterator(start, start + (longDecompIndex >> 12) + 3, TUTF32Iterator::EStartsWithValidCharacter);
	}

/** 
Come up with a decomposition for the current value pointed at by the iterator
@internalComponent
*/
static TBool Decompose(const TUTF32Iterator& aUTF32It, TUTF32Iterator& aOutIt)
	{
    TInt index = ::GetDecompositionIndex(aUTF32It.Current());
	TInt singletonIndex = index - (sizeof(KNonSingletonDecompositions)/sizeof(KNonSingletonDecompositions[0])/2);
	const TInt SizeOfSingletonTable = sizeof(KSingletonDecompositions)/sizeof(KSingletonDecompositions[0]);
	if(index < 0 || SizeOfSingletonTable <= singletonIndex)
        {
        aOutIt = aUTF32It.CurrentAsIterator();
		return EFalse;//There is not any valid decomposition
        }
	if(0 <= singletonIndex)
        {
        // KSingletonDecompositions contains some items that come from "ShortDecompsLongFolds".
        // "ShortDecompsLongFolds" contains some characters that have no composition, but have "long fold".
        // This basically specific to non-BMP characters with fold also outside BMP.
        // For example:
        // 10400;DESERET CAPITAL LETTER LONG I;Lu;0;L;;;;;N;;;;10428;
        // In Unicode 5.0.0, totally, there are 40 similar characters, which are U+10400-U+10427.
        if (KSingletonDecompositions[singletonIndex] == 0xFFFF)
        	return EFalse;
		aOutIt = TUTF32Iterator(KSingletonDecompositions + singletonIndex);
        }
    else
        {
        aOutIt = ::IndexToNonSingletonDecomposition(index);
        }
    return ETrue;//Valid decomposition
	}

/** 
@internalComponent
*/
static TUTF32Iterator CharToUTF32Iterator(TChar aChar, TDes16& aBuf)
	{
	aBuf.Zero();
	if (aChar < 0x10000)
		aBuf.Append(aChar);
	else
		{
		aBuf.Append((aChar >> 10) + 0xD7C0);
		aBuf.Append((aChar & 0x3FF) + 0xDC00);
		}
	const TText16* t = aBuf.Ptr();
	return TUTF32Iterator(t, t + aBuf.Length());
	}

/** 
@internalComponent
*/
TBool DecomposeChar(TChar aCh, TPtrC16& aResult)
    {
    aResult.Set(NULL, 0);
    TBuf16<2> srcBuf;
    TUTF32Iterator it = ::CharToUTF32Iterator(aCh, srcBuf);
    TBool res = ::Decompose(it, it);
    if(res)
        {
        aResult.Set(it.CurrentPosition(), it.Length());
        }
    return res;
    }

/** 
Turn an index into the hash table known to point to a non-singleton
decomposition into that decomposition.
@internalComponent
*/
static TUTF32Iterator HashIndexToDecomposition(TInt aIndex)
	{
	TUint32 v = KUnicodeToIndexHash[aIndex];
	ASSERT(v != 0);
	TInt index = static_cast<TInt>(v >> 20);
	ASSERT(index < ARRAY_LENGTH(KNonSingletonDecompositions)/2);
    return ::IndexToNonSingletonDecomposition(index);
	}

/**
Takes a start and (one past the) end index into KCompostionMapping
and a number of UTF16 characters (aLengthSoFar). All of the compositions
within the range must have their first aLengthSoFar UTF16 characters
matching.

On entry, if aStart == aEnd then there is no possibility of a match so return
immediately with EFalse. To continue, aStart must be strictly less than aEnd.

Afterwards, aStart and aEnd will be narrowed to all those compositions
where the aLengthSoFar'th UTF16 character matches aNextCharacter.
No further compositions existing is indicated by aStart == aEnd.

@return ETrue if the composition at aStart is exactly of length aLengthSoFar + 1.
@internalComponent
*/
static TBool RefineComposition(TInt& aStart, TInt& aEnd, TInt aLengthSoFar, TInt aNextCharacter)
	{
	if (aStart == aEnd)
		return EFalse;
	ASSERT((TUint)aStart < (TUint)aEnd);
 	ASSERT((TUint)aEnd <= (TUint)ARRAY_LENGTH(KCompositionMapping));
    TUTF32Iterator startIterator(::HashIndexToDecomposition(KCompositionMapping[aStart]));
	if (startIterator.Length() == aLengthSoFar)
		++aStart;

	// Find a single example of a decomposition that is suitable
	TInt mid;
	TUTF32Iterator midIt;
	for (;;)
		{
		if (aStart == aEnd)
			return EFalse;
		mid = aStart + ((aEnd - aStart) >> 1);
        midIt = ::HashIndexToDecomposition(KCompositionMapping[mid]);
		ASSERT(aLengthSoFar < midIt.Length());
		TInt midItChar = midIt[aLengthSoFar];
		if (midItChar < aNextCharacter)
			aStart = mid + 1;
		else if (aNextCharacter < midItChar)
			aEnd = mid;
		else
			{
			startIterator = midIt;
			break;
			}
		}

	// FInd the first decomposition that does not match
	TInt start2 = mid + 1;
	while (start2 != aEnd)
		{
		ASSERT(start2 < aEnd);
		TInt mid2 = start2 + ((aEnd - start2) >> 1);
        midIt = ::HashIndexToDecomposition(KCompositionMapping[mid2]);
		ASSERT(aLengthSoFar < midIt.Length());
		TInt midItChar = midIt[aLengthSoFar];
		ASSERT(aNextCharacter <= midItChar);
		if (aNextCharacter < midItChar)
			aEnd = mid2;
		else
			start2 = mid2 + 1;
		}

	// Find the first decomposition that matches
	while (aStart != mid)
		{
		ASSERT(aStart < mid);
		TInt mid2 = aStart + ((mid - aStart) >> 1);
        midIt = ::HashIndexToDecomposition(KCompositionMapping[mid2]);
		ASSERT(aLengthSoFar < midIt.Length());
		TInt midItChar = midIt[aLengthSoFar];
		ASSERT(midItChar <= aNextCharacter);
		if (midItChar < aNextCharacter)
			aStart = mid2 + 1;
		else
			{
			startIterator = midIt;
			mid = mid2;
			}
		}

	return startIterator.Length() == (aLengthSoFar + 1);
	}

/** 
@internalComponent
*/
static TBool RefineCompositionUTF32(TInt& aStart, TInt& aEnd, TInt& aLengthSoFar, TChar aChar)
	{
	if (aChar < 0x10000)
        return ::RefineComposition(aStart, aEnd, aLengthSoFar++, aChar);
    ::RefineComposition(aStart, aEnd, aLengthSoFar++, (aChar >> 10) + 0xD7C0);
	if (aStart == aEnd)
		return EFalse;
    return ::RefineComposition(aStart, aEnd, aLengthSoFar++, (aChar & 0x3FF) + 0xDC00);
	}

/**
Combine as many of the characters presented as possible into a single
character.
@return The number of characters successfully combined.
@param aCombined If a nonzero value is returned, this contains
            the character that is that number of characters from the start of
            aDes combined.
@internalComponent
*/
TInt CombineAsMuchAsPossible(const TDesC16& aDes, TChar& aCombined)
	{
	TInt start = 0;
	TInt end = sizeof(KCompositionMapping)/sizeof(KCompositionMapping[0]);
	TInt length = 0;
	TInt bestIndex = 0;
	TInt bestLength = 0;
	const TText16* ptr = aDes.Ptr();
	TUTF32Iterator input(ptr, ptr + aDes.Length());
	while (!input.AtEnd())
		{
        if (::RefineCompositionUTF32(start, end, length, input.Current()))
			{
			bestIndex = start;
			bestLength = length;
			}
		input.Next();
		}
	if (bestLength == 0)
		return 0;
	aCombined = KUnicodeToIndexHash[KCompositionMapping[bestIndex]] & 0xFFFFF;
	return bestLength;
	}

//////////////////////////////////////////////////////////////////////////////////////////////
// COLLATION
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
// TDecompositionIterator class

/** 
@internalComponent
*/
void TDecompositionIterator::Set(const TUTF32Iterator& a)
	{
	iBase = a;
	if (!iBase.AtEnd())
        {
        (void)::Decompose(iBase, iDecomposition);
        }
	}

/** 
@internalComponent
*/
TDecompositionIterator::TDecompositionIterator(const TUTF32Iterator& a)
	{
	Set(a);
	}

/** 
@internalComponent
*/
void TDecompositionIterator::Next()
	{
	ASSERT(!iBase.AtEnd() && !iDecomposition.AtEnd());
	iDecomposition.Next();
	if (!iDecomposition.AtEnd())
		return;
	iBase.Next();
	if (!iBase.AtEnd())
        {
        (void)::Decompose(iBase, iDecomposition);
        }
	}

/** 
@internalComponent
*/
const TText16* TDecompositionIterator::CurrentPosition() const
	{
	return iBase.CurrentPosition();
	}

/** 
Find out the length and minimum combining class of
the current run of characters of nonzero combining class.
aMinClass and aMinClassPos are not written to if the return
value is 0.
aEndOfRun is written to with the final position of the iteration
if 0 is returned and aEndOfRun is non-null
@internalComponent
*/
static TInt ReorderingRun(TInt& aMinClass, TDecompositionIterator& aMinClassPos,
                          const TDecompositionIterator& aStart, TBool* aOpenSequence, 
                          TInt aMaxDisallowedClass = 0, TDecompositionIterator* aEndOfRun = 0)
	{
	TInt comclass = aStart.AtEnd()? 0 : aStart.Current().GetCombiningClass();
	if (comclass == 0)
		{
		if (aEndOfRun)
			*aEndOfRun = aStart;
		if (aOpenSequence)
			*aOpenSequence = aStart.AtEnd();
		return 0;
		}
	aMinClass = 256;
	TDecompositionIterator i = aStart;
	TInt count = 0;
	while (comclass != 0)
		{
		if (aMaxDisallowedClass < comclass)
			{
			if (comclass < aMinClass)
				{
				aMinClass = comclass;
				aMinClassPos = i;
				}
			++count;
			}
		i.Next();
		comclass = i.AtEnd()? 0 : i.Current().GetCombiningClass();
		}
	if (count == 0 && aEndOfRun)
		*aEndOfRun = i;
	if (aOpenSequence)
		*aOpenSequence = i.AtEnd();
	return count;
	}

//////////////////////////////////////////////////////////////////////////////////////////////
// TCanonicalDecompositionIterator class

/** 
@internalComponent
*/
void TCanonicalDecompositionIterator::Set(const TUTF32Iterator& a)
	{
	iBase.Set(a);
	iLastPosition = 0;
	if (ReorderingRun(iCurrentCombiningClass, iCurrent, iBase, &iInOpenSequence) < 2)
		iCurrentCombiningClass = 0;
	}

/** 
@internalComponent
*/
void TCanonicalDecompositionIterator::Next()
	{
	iLastPosition = iBase.CurrentPosition();
	if (iCurrentCombiningClass == 0)
		{
		iBase.Next();
		if (ReorderingRun(iCurrentCombiningClass, iCurrent, iBase, &iInOpenSequence) < 2)
			iCurrentCombiningClass = 0;
		return;
		}
	// Find the next character in the run with the same combining class
	iCurrent.Next();
	TInt curclass = iCurrent.AtEnd()? 0 : iCurrent.Current().GetCombiningClass();
	while (curclass != 0)
		{
		if (curclass == iCurrentCombiningClass)
			// success
			return;
		iCurrent.Next();
		curclass = iCurrent.AtEnd()? 0 : iCurrent.Current().GetCombiningClass();
		}
	// There are none left in the current class. Find out what the next one is.
	if (0 == ReorderingRun(iCurrentCombiningClass, iCurrent, iBase, 0, iCurrentCombiningClass, &iBase))
		iCurrentCombiningClass = 0;
	}

/** 
@internalComponent
*/
const TText16* TCanonicalDecompositionIterator::CurrentPositionIfAtCharacter() const
	{
	if (iCurrentCombiningClass != 0)
		return 0;
	const TText16* p = iBase.CurrentPosition();
	return iLastPosition == p? 0 : p;
	}

/** 
@internalComponent
*/
TBool TCanonicalDecompositionIterator::IsInOpenSequence() const
	{
	return iInOpenSequence;
	}

//////////////////////////////////////////////////////////////////////////////////////////////
// TCanonicalDecompositionIteratorCached class

/** 
@internalComponent
*/
void TCanonicalDecompositionIteratorCached::Set(const TUTF32Iterator& a)
	{
	iBase.Set(a);
	iCacheStart = 0;
	iCacheSize = 0;
	}

/** 
@internalComponent
*/
void TCanonicalDecompositionIteratorCached::Next(TInt aOffset)
	{
	ASSERT(0 <= aOffset);
	ASSERT(0 <= iCacheSize);
	ASSERT(0 != iCacheSize || !iBase.AtEnd());
	if (aOffset <= iCacheSize)
		{
		iCacheSize -= aOffset;
		iCacheStart = (iCacheStart + aOffset) & (KMaxLookAhead - 1);
		return;
		}
	aOffset -= iCacheSize;
	iCacheSize = 0;
	while (aOffset != 0)
		{
		iBase.Next();
		--aOffset;
		}
	}

/** 
Get the character at the position of the iterator plus aOffset steps.
Returns -1 if we are looking too far ahead.
@internalComponent
*/
TChar TCanonicalDecompositionIteratorCached::Get(TInt aOffset)
	{
	// should be assert debug: there is a chance this could go off with
	// bad collation tables
	ASSERT(aOffset <= KMaxLookAhead);
	while (iCacheSize <= aOffset)
		{
		if (iBase.AtEnd())
			return TChar(static_cast <TUint> (-1));
		TInt cachePos = (iCacheStart + iCacheSize) & (KMaxLookAhead - 1);
		iCache[cachePos].iChar = iBase.Current();
		iCache[cachePos].iPos = iBase.CurrentPositionIfAtCharacter();
		++iCacheSize;
		iBase.Next();
		}
	return iCacheSize == aOffset? iBase.Current() : iCache[(iCacheStart + aOffset) & (KMaxLookAhead - 1)].iChar;
	}

/** 
If the current position in the original string is representable
as a pointer into it and we know what it is, return it.
@internalComponent
*/
const TText16* TCanonicalDecompositionIteratorCached::CurrentPositionIfAtCharacter() const
	{
	if(iCacheSize == 0)
		{
		return iBase.CurrentPositionIfAtCharacter();
		}
	return iCache[iCacheStart & (KMaxLookAhead - 1)].iPos;
	}

