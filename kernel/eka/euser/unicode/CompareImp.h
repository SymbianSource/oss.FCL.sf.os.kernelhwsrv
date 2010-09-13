// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __COMPAREIMP_H__
#define __COMPAREIMP_H__

#include <e32std.h>
#include <unicode.h>

//Forward declarations
class TUTF32Iterator;

//////////////////////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////////////////////

inline TChar UTF16ToChar(const TText16* a);
TBool IsBaseCharacter(TChar);

TBool MatchSectionFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm);

TBool FindMatchSectionFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm);

TBool MatchStringFolded(const TText16* aCandidateStringStart, const TText16* aCandidateStringEnd,
                        const TText16* aSearchTermStart, const TText16* aSearchTermEnd);

TInt LocateMatchStringFolded(const TText16* aCandidateStringStart, const TText16* aCandidateStringEnd,
                             const TText16* aSearchTermStart, const TText16* aSearchTermEnd);

TInt FindFolded(TUTF32Iterator& aCandidateString, TUTF32Iterator& aSearchTerm);

TInt CompareFolded(const TUTF32Iterator& aLeft, const TUTF32Iterator& aRight);

TInt CombineAsMuchAsPossible(const TDesC16& aDes, TChar& aCombined);

TBool DecomposeChar(TChar aCh, TPtrC16& aResult);

inline void SkipCombiningCharacters(TUTF32Iterator& aUTF32It);

/**
Converts UTF16 into UTF32, ignoring non-characters and
unpaired surrogates and combining paired surrogates.
@internalComponent
*/
class TUTF32Iterator
	{
public:
	enum TStartsWithValidCharacter { EStartsWithValidCharacter };
	inline TUTF32Iterator();
	inline explicit TUTF32Iterator(const TText16* aSingleton);
	inline TUTF32Iterator(const TText16* aStart, const TText16* aEnd);
	inline TUTF32Iterator(const TText16* aStart, const TText16* aEnd, TStartsWithValidCharacter);

	inline TUTF32Iterator CurrentAsIterator() const;
	inline TBool AtEnd() const;
	inline void Next();
	inline TChar Current() const;
	TBool LocateFoldedBaseCharacter(TChar aChar);
	inline const TText16* CurrentPosition() const;
	inline TInt Length() const;
	inline TInt operator[](TInt) const;
	inline void SetStart(const TText16*);
private:
	const TText16* iStart;
	const TText16* iEnd;
	TChar iCurrent;
	};

//////////////////////////////////////////////////////////////////////////////////////////////
// FOLDING
//////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
class TFoldedDecompIterator
	{
public:
	inline TFoldedDecompIterator();
	explicit TFoldedDecompIterator(const TUTF32Iterator&);
	inline void Set(const TUTF32Iterator&);
	inline TBool AtEnd() const;
	TBool AtEndOrWildcard() const;
	TBool EnterFoldedSequence();
	TBool StrictEnterFoldedSequence();
	inline TBool IsInFoldedSequence() const;
	TBool CurrentIsBaseFoldedFromCombiner() const;
	inline TChar Current() const;
	TBool Match(TChar aCode);
	TBool Match(TFoldedDecompIterator& aThat);
	void Next();
	inline TUTF32Iterator BaseIterator() const;
private:
	TUTF32Iterator iOriginal;
	TUTF32Iterator iFolded;
	};

/**
Sorts sequences of combining characters with non-zero combining classes into
order of their combining classes.
@internalComponent
*/
class TFoldedSortedDecompIterator
	{
public:
	inline TFoldedSortedDecompIterator();
	TInt Set(TFoldedDecompIterator &aBase);
	void Set();
	inline TBool AtEnd() const;
	inline TChar Current() const;
	void Next();

private:
	TFoldedDecompIterator iStart; // Starting code.	
	TInt iLength; // Length in decomposed codes.
	TFoldedDecompIterator iCurrent; // Current code.
	TInt iCurrentCount; // Number of decomposed codes iCurrent is past iStart
	TInt iCurrentClass; // Current class being searched for.
	TInt iRemaining; // Number of Next()s left
	};

/**
Iterator that outputs canonically decomposed folded strings.
This is much slower than using the matching functions, so should only
be used where an ordering is required.
@internalComponent
*/
class TFoldedCanonicalIterator
	{
public:
	TFoldedCanonicalIterator(const TUTF32Iterator&);
	inline TBool AtEnd() const;
	inline TChar Current() const;
	void Next(const TUnicodeDataSet* aCharDataSet);
private:
	TFoldedDecompIterator iBase;
	TFoldedSortedDecompIterator iSorted;
	};


//////////////////////////////////////////////////////////////////////////////////////////////
// COLLATION
//////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
class TDecompositionIterator
	{
public:
	inline TDecompositionIterator();
	void Set(const TUTF32Iterator&);
	explicit TDecompositionIterator(const TUTF32Iterator&);
	inline TBool AtEnd() const;
	inline TChar Current() const;
	void Next();
	const TText16* CurrentPosition() const;
private:
	TUTF32Iterator iBase;
	TUTF32Iterator iDecomposition;
	};

/**
@internalComponent
*/
class TCanonicalDecompositionIterator
	{
public:
	inline TCanonicalDecompositionIterator();
	void Set(const TUTF32Iterator&);
	inline TBool AtEnd() const;
	inline TChar Current() const;
	void Next();
	const TText16* CurrentPositionIfAtCharacter() const;
	TBool IsInOpenSequence() const;
private:
	TDecompositionIterator iBase;
	// iBase.CurrentPosition() before the last move
	const TText16* iLastPosition;
	// If iCurrent is active, iCurrentCombiningClass
	// is nonzero, and represents the combining class
	// of the character it points to.
	TInt iCurrentCombiningClass;
	// contains true if more characters added to the end may change
	// the characters currently being output
	TBool iInOpenSequence;
	// Iterator that looks for characters to be sorted.
	TDecompositionIterator iCurrent;
	};

/**
Iterator that gives the canonically decomposed form of
its input, and allows a limited amount of look-ahead (i.e.
peeking further into the decomposition without moving
the iterator)
@internalComponent
*/
class TCanonicalDecompositionIteratorCached
	{
public:
	void Set(const TUTF32Iterator&);
	inline TBool AtEnd() const;
	// Advance aOffset characters.
	void Next(TInt aOffset);
	// Get the character at the position of the iterator plus aOffset steps.
	// Returns FFFF if we are looking too far ahead.
	TChar Get(TInt aOffset);
	// If the current position in the original string is representable
	// as a pointer into it and we know what it is, return it.
	const TText16* CurrentPositionIfAtCharacter() const;
private:
	// KMaxLookAhead must be a power of 2
	enum { KMaxLookAhead = 8 };
	TCanonicalDecompositionIterator iBase;
	struct TCache
		{
		TChar iChar;
		const TText16* iPos;
		};
	TCache iCache[KMaxLookAhead + 1];
	TInt iCacheStart;
	TInt iCacheSize;
	};

#include "CompareImp.inl"

#endif //__COMPAREIMP_H__
