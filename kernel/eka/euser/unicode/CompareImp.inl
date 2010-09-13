// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @internalComponent
*/
inline TBool IsSurrogate(TText a) 
	{ 
	return 0xD800 == (a & 0xF800); 
	}

/**
@internalComponent
*/
inline TBool IsHighSurrogate(TText a) 
	{ 
	return 0xD800 == (a & 0xFC00); 
	}

/**
@internalComponent
*/
inline TBool IsLowSurrogate(TText a) 
	{ 
	return 0xDC00 == (a & 0xFC00); 
	}

/**
@internalComponent
*/
inline TChar PairSurrogates(TText aHigh, TText aLow)
	{
	return ((aHigh - 0xd7f7) << 10) + aLow;
	}

inline void SkipCombiningCharacters(TUTF32Iterator& aUTF32It)
	{
	while(!aUTF32It.AtEnd() && !::IsBaseCharacter(aUTF32It.Current()))
		{
		aUTF32It.Next();
		}
	}

/**
@internalComponent
*/
inline TChar UTF16ToChar(const TText16* a)
	{
	if (0xD800 <= a[0])
		{
		if (a[0] < 0xE000)
			{
            if (a[0] < 0xDC00 && ::IsLowSurrogate(a[1]))
				{
                TChar c = ::PairSurrogates(a[0], a[1]);
				if ((c & 0xFFFE) != 0xFFFE)
					return c;
				}
			return 0xFFFF;
			}
		if (a[0] == 0xFFFE)
			return 0xFFFF;
		}
	return a[0];
	}


////////////////////////////////////////////////////////////////////////////////////////////
// TUTF32Iterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
inline TUTF32Iterator::TUTF32Iterator() : 
	iStart(0), 
	iEnd(0) 
	{
	}

/**
@internalComponent
*/
inline TUTF32Iterator::TUTF32Iterator(const TText16* aSingleton) : 
	iStart(aSingleton), 
	iEnd(aSingleton + 1), 
	iCurrent(*aSingleton)
	{
	ASSERT((iCurrent & 0xFFFE) != 0xFFFE);
	ASSERT(!::IsSurrogate(*aSingleton));
	}

/**
@internalComponent
*/
inline TUTF32Iterator::TUTF32Iterator(const TText16* aStart, const TText16* aEnd) : 
	iEnd(aEnd)
	{
	SetStart(aStart);
	}

/**
Sets the iteration to begin at aStart.
@param aStart New starting point of iteration.
@internalComponent
*/
inline void TUTF32Iterator::SetStart(const TText16* aStart)
	{
	iStart = aStart;
	if(iStart != iEnd)
		{
		if(::IsSurrogate(iEnd[-1]))
			{
			--iEnd;
			if(iStart == iEnd)
				{
				return;
				}
			}
		iCurrent = ::UTF16ToChar(iStart);
		if(iCurrent == 0xFFFF)
			{
			Next();
			}
		}
	}

/**
@internalComponent
*/
inline TUTF32Iterator::TUTF32Iterator(const TText16* aStart, const TText16* aEnd, TStartsWithValidCharacter) : 
	iStart(aStart), 
	iEnd(aEnd)
	{
	ASSERT(iStart < iEnd);
	if(::IsSurrogate(iEnd[-1]))
		{
		--iEnd;
		}
	iCurrent = ::UTF16ToChar(iStart);
	ASSERT(iCurrent != 0xFFFF);
	}

/**
@internalComponent
*/
inline TUTF32Iterator TUTF32Iterator::CurrentAsIterator() const
	{
	TUTF32Iterator retval(*this);
	ASSERT(iStart != iEnd);
	retval.iEnd = iStart + 1;
	return retval;
	}

/**
@internalComponent
*/
inline TBool TUTF32Iterator::AtEnd() const
	{
	return iEnd == iStart;
	}

/**
@internalComponent
*/
inline TChar TUTF32Iterator::Current() const
	{
	ASSERT(iStart != iEnd);
	ASSERT(iCurrent != 0xFFFF);
	return iCurrent;
	}

/**
@internalComponent
*/
inline void TUTF32Iterator::Next()
	{
	ASSERT(iStart != iEnd);
	while (++iStart != iEnd)
		{
        iCurrent = ::UTF16ToChar(iStart);
		if (iCurrent != 0xFFFF)
			return;
		}
	}

/**
@internalComponent
*/
inline const TText16* TUTF32Iterator::CurrentPosition() const
	{
	return iStart;
	}

/**
@internalComponent
*/
inline TInt TUTF32Iterator::Length() const
	{
	return iEnd - iStart;
	}

/**
@internalComponent
*/
inline TInt TUTF32Iterator::operator[](TInt a) const
	{
	return iStart[a];
	}

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedDecompIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
inline TFoldedDecompIterator::TFoldedDecompIterator() 
	{
	}

/**
@internalComponent
*/
inline TUTF32Iterator TFoldedDecompIterator::BaseIterator() const 
	{ 
	return iOriginal; 
	}

/**
@internalComponent
*/
inline void TFoldedDecompIterator::Set(const TUTF32Iterator& a)
	{
	iOriginal = a;
	}

/**
@internalComponent
*/
TBool TFoldedDecompIterator::AtEnd() const
    {
    return iOriginal.AtEnd();
    }

/**
@internalComponent
*/
inline TBool TFoldedDecompIterator::IsInFoldedSequence() const
	{
	return !iFolded.AtEnd();
	}

/**
@internalComponent
*/
inline TChar TFoldedDecompIterator::Current() const
    {
    ASSERT(!AtEnd());
    return IsInFoldedSequence()? iFolded.Current() : iOriginal.Current();
    }

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedSortedDecompIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
inline TFoldedSortedDecompIterator::TFoldedSortedDecompIterator() 
	{
	}

/** 
@internalComponent
*/
inline TBool TFoldedSortedDecompIterator::AtEnd() const
    {
    return iRemaining == 0;
    }

/** 
@internalComponent
*/
inline TChar TFoldedSortedDecompIterator::Current() const
    {
    ASSERT(!AtEnd());
    return iCurrent.Current();
    }

////////////////////////////////////////////////////////////////////////////////////////////
// TFoldedCanonicalIterator
////////////////////////////////////////////////////////////////////////////////////////////

/** 
@internalComponent
*/
inline TBool TFoldedCanonicalIterator::AtEnd() const
    {
    return iSorted.AtEnd() && iBase.AtEnd();
    }

/** 
@internalComponent
*/
inline TChar TFoldedCanonicalIterator::Current() const
    {
    ASSERT(!iBase.AtEnd() || !iSorted.AtEnd());
    return iSorted.AtEnd() ? iBase.Current() : iSorted.Current();
    }

////////////////////////////////////////////////////////////////////////////////////////////
// TDecompositionIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
inline TDecompositionIterator::TDecompositionIterator() 
	{
	}

/** 
@internalComponent
*/
inline TBool TDecompositionIterator::AtEnd() const
    {
    return iBase.AtEnd();
    }

/** 
@internalComponent
*/
inline TChar TDecompositionIterator::Current() const
    {
    return iDecomposition.Current();
    }

////////////////////////////////////////////////////////////////////////////////////////////
// TCanonicalDecompositionIterator
////////////////////////////////////////////////////////////////////////////////////////////

/**
@internalComponent
*/
inline TCanonicalDecompositionIterator::TCanonicalDecompositionIterator() 
	{
	}

/** 
@internalComponent
*/
inline TBool TCanonicalDecompositionIterator::AtEnd() const
    {
    return iBase.AtEnd();
    }

/** 
@internalComponent
*/
inline TChar TCanonicalDecompositionIterator::Current() const
    {
    return iCurrentCombiningClass? iCurrent.Current() : iBase.Current();
    }

////////////////////////////////////////////////////////////////////////////////////////////
// TCanonicalDecompositionIteratorCached
////////////////////////////////////////////////////////////////////////////////////////////

/** 
@internalComponent
*/
inline TBool TCanonicalDecompositionIteratorCached::AtEnd() const
    {
    return iCacheSize == 0 && iBase.AtEnd();
    }



