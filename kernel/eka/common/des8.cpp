// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\common\des8.cpp
// 
//

#include "common.h"
#include <e32des8_private.h>
#ifndef __KERNEL_MODE__
#include <collate.h>
#else
#include <kernel/kern_priv.h>
#endif
#include <unicode.h>

// Folding/Collation for 8 bit characters

extern const TUint8 __FoldCollTab8[256];

const TUint8 __FoldCollTab8[256] =
	{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, // 0x00
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, // 0x10
	0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, // 0x20
	0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, // 0x30
	0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67, // 0x40
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, // 0x50
	0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67, // 0x60
	0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, // 0x70
	0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, // 0x80
	0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97, // 0x90
	0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0x20,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7, // 0xa0
	0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7, // 0xb0
	0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0x61,0x61,0x61,0x61,0x61,0x61,0xe6,0x63, // 0xc0
	0x65,0x65,0x65,0x65,0x69,0x69,0x69,0x69,
	0xf0,0x6e,0x6f,0x6f,0x6f,0x6f,0x6f,0xd7, // 0xd0
	0xf8,0x75,0x75,0x75,0x75,0x79,0xfe,0xdf,
	0x61,0x61,0x61,0x61,0x61,0x61,0xe6,0x63, // 0xe0
	0x65,0x65,0x65,0x65,0x69,0x69,0x69,0x69,
	0xf0,0x6e,0x6f,0x6f,0x6f,0x6f,0x6f,0xf7, // 0xf0
	0xf8,0x75,0x75,0x75,0x75,0x79,0xfe,0x79
	};

#ifndef __KERNEL_MODE__
inline TUint8* memCopy(TUint8* aPtr, const TUint8* aSrc, TInt aLength)
//
// Copy 8 bit values.
//
	{

	return Mem::Copy(aPtr, aSrc, aLength);
	}
#endif

#if (defined(__KERNEL_MODE__) && !defined(__DES8_MACHINE_CODED__)) | defined(__EABI_CTORS__)
inline TInt StringLength(const TUint8* aPtr)
	{
	const TUint8* p = aPtr;
	while (*p)
		++p;
	return p-aPtr;
	}
#endif

inline TDesC8::TDesC8(TInt aType,TInt aLength)
	:iLength(aLength|(aType<<KShiftDesType8))
	{}
inline TInt TDesC8::Type() const
//
// Return the descriptor type
//
	{
	return(iLength>>KShiftDesType8);
	}

inline TDes8::TDes8(TInt aType,TInt aLength,TInt aMaxLength)
	: TDesC8(aType,aLength),iMaxLength(aMaxLength)
	{}

// Class TBufCBase8
inline TBufCBase8::TBufCBase8(TInt aLength)
	:TDesC8(EBufC,aLength)
	{}

inline TUint8* TBufCBase8::WPtr() const
	{return const_cast<TUint8*>(Ptr());}


#ifndef __DES8_MACHINE_CODED__
EXPORT_C const TUint8* TDesC8::Ptr() const
/**
Gets a pointer to the data represented by the descriptor.

The data cannot be changed through the returned pointer.

@return A pointer to the data
*/
	{

	switch (Type())
		{
	case EBufC:
		return(&((SBufC8 *)this)->buf[0]);
	case EPtrC:
		return(((SPtrC8 *)this)->ptr);
	case EPtr:
		return(((SPtr8 *)this)->ptr);
	case EBuf:
		return(&((SBuf8 *)this)->buf[0]);
	case EBufCPtr:
		return(&((SBufCPtr8 *)this)->ptr->buf[0]);
		}
	Panic(ETDes8BadDescriptorType);
	return(NULL);
	}

EXPORT_C const TUint8 &TDesC8::AtC(TInt anIndex) const
//
// Return a reference to the character in the buffer.
//
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<Length(),Panic(ETDes8IndexOutOfRange));
	return(Ptr()[anIndex]);
	}

EXPORT_C TInt TDesC8::Compare(const TDesC8 &aDes) const
/**
Compares this descriptor's data with the specified descriptor's data.

The comparison proceeds on a byte for byte basis. The result of the comparison 
is based on the difference of the first bytes to disagree.

Two descriptors are equal if they have the same length and content. Where 
two descriptors have different lengths and the shorter descriptor's data
matches the first part of the longer descriptor's data, the shorter is
considered to be less than the longer.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data.
             
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
*/
	{

	return memcompare(Ptr(), Length(), aDes.Ptr(), aDes.Length());
	}

#ifndef __KERNEL_MODE__
EXPORT_C TInt TDesC8::CompareF(const TDesC8 &aDes) const
/**
Compares this descriptor's folded data with the specified descriptor's folded 
data. 

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for comparing strings in natural language; 
use CompareC() for this.

@param aDes The 8-bit non modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
        
@see TDesC8::Compare()
*/
	{

	return(Mem::CompareF(Ptr(),Length(),aDes.Ptr(),aDes.Length()));
	}

EXPORT_C TInt TDesC8::CompareC(const TDesC8 &aDes) const
/**
Compares this descriptor's data with the specified descriptor's data using 
the standard collation method for narrow text appropriate to the current locale.

@param aDes The 8-bit non modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
        
@see TDesC8::Compare()
@deprecated
*/
	{

	return(Mem::CompareC(Ptr(),Length(),aDes.Ptr(),aDes.Length()));
	}
#endif
#endif

EXPORT_C TInt TDesC8::Find(const TUint8 *pS,TInt aLenS) const
/**
Searches for the first occurrence of the specified data sequence within this 
descriptor.

Searching always starts at the beginning of this descriptor's data.

@param pS    A pointer to a location containing the data sequence to be searched 
             for.
@param aLenS The length of the data sequence to be searched for. This value 
             must not be negative, otherwise the function raises a panic.
             
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
       
@panic  USER 29 if aLenS is negative. 
*/
	{

	if (!aLenS)
		return(0);
	__ASSERT_ALWAYS(aLenS>0,Panic(ETDes8LengthNegative));
	const TUint8 *pB=Ptr();
	TInt aLenB=Length();
	const TUint8 *pC=pB-1;			// using pre-increment addressing
	TInt i=aLenB-aLenS;
	if (i>=0)
		{
		const TUint8* pEndS=pS+aLenS-1;		// using pre-increment addressing
		const TUint8 *pEndB=pB+i;			// using pre-increment addressing
		TUint s=*pS;
		for (;;)
			{
			do
				{
				if (pC==pEndB)
					return KErrNotFound;
				} while (*++pC!=s);
			const TUint8 *p1=pC;
			const TUint8 *p2=pS;
			do
				{
				if (p2==pEndS)
					return (pC-pB);
				} while (*++p1==*++p2);
			}
		}
	return(KErrNotFound);
	}

EXPORT_C TInt TDesC8::Find(const TDesC8 &aDes) const
/**
Searches for the first occurrence of the specified data sequence within this 
descriptor.

Searching always starts at the beginning of this descriptor's data.

@param aDes The 8-bit non modifable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
*/
	{

	return(Find(aDes.Ptr(),aDes.Length()));
	}

const TUint8* convTable(TMatchType aType)
	{
	switch (aType)
		{
		case EMatchFolded:
		case EMatchCollated: return __FoldCollTab8;
		default: return NULL;
		}
	}

inline TUint conv(const TUint8* aStr,const TUint8* aConv)
	{
	TUint c=*aStr;
	return aConv ? aConv[c] : c;
	}


inline TUint lookup(const TUint8* aStr,const TUint8* aConv)
	{
	return aConv[*aStr];
	}

TInt DoMatch8(const TDesC8 &aLeftD,const TDesC8 &aRightD,TMatchType aType)
	{
	const TUint8* table=convTable(aType);
	const TUint8* pRight=aRightD.Ptr();
	const TUint8* pM=pRight-1;				// pre-increment addressing
	const TUint8* pP=pM+aRightD.Length();
	const TUint8* pLeft=aLeftD.Ptr()-1;		// pre-increment addressing
	const TUint8* pB=pLeft;	
	const TUint8* pE=pB+aLeftD.Length();

	// Match any pattern up to the first star
	TUint c;
	for (;;)
		{
		if (pM==pP)		// exhausted the pattern
			return pB==pE ? 0 : KErrNotFound;
		TUint c=conv(++pM,table);
		if (c==KMatchAny)
			break;
		if (pB==pE)			// no more input
			return KErrNotFound;
		if (c!=conv(++pB,table) && c!=KMatchOne)	// match failed
			return KErrNotFound;
		}
	// reached a star
	if (pM==pP)
		return 0;
	TInt r=pM==pRight ? -1 : 0;
	for (;;)
		{
		c=conv(++pM,table);
		if (c==KMatchAny)
			{
star:		if (pM==pP)		// star at end of pattern, always matches
				return Max(r,0);
			if (r<-1)		// skipped some '?', matches at beginning
				r=0;
			continue;
			}
		if (pB==pE)			// no more input
			return KErrNotFound;
		if (c==KMatchOne)
			{				// skip a character in the input
			if (pM==pP)
				return r+((r>=0) ? 0 : (pE-pLeft));
			++pB;
			if (r<0)
				--r;
			continue;
			}
	// Matching a non-wild character
		for (;;)
			{
			if (table)	// pull this test out of the tight loop (10-20% faster)
				{
				while (lookup(++pB,table)!=c)
					{
					if (pB==pE)				// no more input
						return KErrNotFound;
					}
				}
			else
				{
				while (*++pB!=c)
					{
					if (pB==pE)				// no more input
						return KErrNotFound;
					}
				}
			// Try to match up to the next star
			const TUint8* pb=pB;
			const TUint8* pm=pM;
			for (;;)
				{
				if (pm<pP)
					{
					TUint cc=conv(++pm,table);
					if (cc==KMatchAny)
						{	// sub-match successful, back to main loop
						r+=(r>=0 ? 0 : pB-pLeft);
						pB=pb;
						pM=pm;
						goto star;
						}
					if (pb==pE)
						return KErrNotFound;	// no more input
					if (cc!=conv(++pb,table) && cc!=KMatchOne)
						break;	// sub-match failed, try next input character
					}
				else if (pb==pE)	// end of matching pattern
					return r+(r>=0 ? 0 : pB-pLeft);	// end of input, so have a match
				else
					break;		// try next input character
				}
			}
		}
	}

EXPORT_C TInt TDesC8::Match(const TDesC8 &aDes) const
/**
Searches this descriptor's data for a match with the match pattern supplied 
in the specified descriptor.

The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.

@param aDes An 8-bit non-modifable descriptor containing the match pattern.

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
*/
	{

	return DoMatch8(*this,aDes,EMatchNormal);
	}

EXPORT_C TInt TDesC8::MatchF(const TDesC8 &aDes) const
/**
Searches this descriptor's folded data for a match with the folded match 
pattern supplied in the specified descriptor.

The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for matching strings in natural language; 
use MatchC() for this.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.

@param aDes An 8-bit non-modifable descriptor containing the match pattern. 

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match. 

@see TDesC8::MatchC()
*/
	{

	return DoMatch8(*this,aDes,EMatchFolded);
	}

EXPORT_C TInt TDesC8::MatchC(const TDesC8 &aPattern) const
/**
Searches this descriptor's collated data for a match with the collated match 
pattern supplied in the specified descriptor.

The function uses the standard collation method for narrow text appropriate to 
the current locale.
	
The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.
	
@param aPattern An 8-bit non-modifable descriptor containing the match pattern. 

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
@deprecated
*/
	{
#ifndef __KERNEL_MODE__
	return MatchF(aPattern);
#else
	return DoMatch8(*this,aPattern,EMatchCollated);
#endif
	}

#ifndef __KERNEL_MODE__

EXPORT_C TInt TDesC8::FindF(const TUint8 *pS,TInt aLenS) const
/**
Searches for the first occurrence of the specified folded data sequence within 
this descriptor's folded data.

Searching always starts at the beginning of this descriptor's data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for finding strings in natural language; 
use FindC() for this.

@param pS    A pointer to a location containing the data sequence to be
             searched for.
@param aLenS The length of the data sequence to be searched for. This value 
             must not be negative, otherwise the function raises a panic.
             
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found. Zero, if the
        length of the search data sequence is zero.

@panic USER 29 if aLenS is negative

@see TDesC8::FindC()
*/
	{
	if (!aLenS)
		return(0);
	const TUint8* table=convTable(EMatchFolded);
	const TUint8 *pB=Ptr();
	TInt aLenB=Length();
	const TUint8 *pC=pB-1;			// using pre-increment addressing
	TInt i=aLenB-aLenS;
	if (i>=0)
		{
		const TUint8* pEndS=pS+aLenS-1;		// using pre-increment addressing
		const TUint8 *pEndB=pB+i;			// using pre-increment addressing
		TUint s=lookup(pS,table);
		for (;;)
			{
			do
				{
				if (pC==pEndB)
					return KErrNotFound;
				} while (lookup(++pC,table)!=s);
			const TUint8 *p1=pC;
			const TUint8 *p2=pS;
			do
				{
				if (p2==pEndS)
					return (pC-pB);
				} while (lookup(++p1,table)==lookup(++p2,table));
			}
		}
	return(KErrNotFound);
	}

EXPORT_C TInt TDesC8::FindF(const TDesC8 &aDes) const
/**
Searches for the first occurrence of the specified folded data sequence within 
this descriptor's folded data. 

Searching always starts at the beginning of this descriptor's data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for finding strings in natural language; 
use FindC() for this.

@param aDes The 8-bit non-modifable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found. Zero, if the 
        length of the search data sequence is zero.

@see TDesC8::FindC()
*/
	{

	return(FindF(aDes.Ptr(),aDes.Length()));
	}

EXPORT_C TInt TDesC8::FindC(const TUint8* aText,TInt aLength) const
/**
Searches for the first occurrence of the specified collated data sequence within 
this descriptor's collated data.
	
Searching always starts at the beginning of this descriptor's data. The function 
uses the standard collation method for narrow text appropriate to the current 
locale.
	
@param aText   A pointer to a location containing the data sequence to be
               searched for.
@param aLength The length of the data sequence to be searched for.
             
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
      
@panic USER 29 if aLength is negative.
@deprecated
*/
	{
	return FindF(aText, aLength);
	}

EXPORT_C TInt TDesC8::FindC(const TDesC8 &aDes) const
/**
Searches for the first occurrence of the specified collated data sequence within 
this descriptor's collated data.

Searching always starts at the beginning of this descriptor's data. The function 
uses the standard collation method for narrow text appropriate to the current 
locale.

@param aDes The 8-bit non-modifable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
@deprecated
*/
	{

	return(FindC(aDes.Ptr(),aDes.Length()));
	}

EXPORT_C TInt TDesC8::LocateF(TChar aChar) const
/**
Searches for the first occurrence of a folded character within this
descriptor's folded data.

The search starts at the beginning of the data,i.e. at the leftmost position.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The character to be found.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{
	TUint c = User::Fold(aChar);
	if(c>=0x100)
		return KErrNotFound;
	const TUint8 *pBuf=Ptr();
	const TUint8 *pB=pBuf-1;
	const TUint8 *pE=pB+Length();
	const TUint8* table=__FoldCollTab8;
	do
		{
		if (pB==pE)
			return KErrNotFound;
		} while (table[*++pB]!=c);
	return pB-pBuf;
	}
#endif	// __KERNEL_MODE__

#ifndef __DES8_MACHINE_CODED__
EXPORT_C TInt TDesC8::Locate(TChar aChar) const
/**
Searches for the first occurrence of a character within this descriptor's
data.

The search starts at the beginning of the data, i.e. at the leftmost position.

@param aChar The character to be found. 

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	const TUint8 *pBuf=Ptr();
	const TUint8 *pB=pBuf-1;
	const TUint8 *pE=pB+Length();
	do
		{
		if (pB==pE)
			return KErrNotFound;
		} while (*++pB!=aChar);
	return pB-pBuf;
	}
#endif

#ifndef __DES8_MACHINE_CODED__
EXPORT_C TInt TDesC8::LocateReverse(TChar aChar) const
/**
Searches for the first occurrence of a character within this descriptor's data, 
searching from the end of the data.

The search starts at the rightmost position.

@param aChar The character to be found.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	TInt len=Length();
	if (len==0)
		return(KErrNotFound);
	const TUint8 *pB=Ptr();
	const TUint8 *pE=pB+len-1;
	while (pE>=pB)
		{
		if (*pE==aChar)
			break;
		pE--;
		}
	return(pE<pB ? KErrNotFound : pE-pB);
	}
#endif

#ifndef __KERNEL_MODE__
EXPORT_C TInt TDesC8::LocateReverseF(TChar aChar) const
/**
Searches for the first occurrence of a folded character within this descriptor's 
folded data, searching from the end of the data.

The search starts at the rightmost position. 

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The character to be found 

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found
*/
	{

	TInt len=Length();
	if (len==0)
		return(KErrNotFound);
	const TUint8 *pB=Ptr();
	const TUint8 *pE=pB+len-1;
	const TUint8* table=__FoldCollTab8;
	TUint c = table[aChar];
	while (pE>=pB)
		{
		if (table[*pE]==c)
			break;
		pE--;
		}
	return(pE<pB ? KErrNotFound : pE-pB);
	}

EXPORT_C HBufC8 *TDesC8::Alloc() const
/**
Creates a new 8-bit heap descriptor and initialises it with a copy of this 
descriptor's data.

@return A pointer to the new 8 bit heap descriptor, if creation is successful. 
        NULL, if creation of the descriptor fails.
*/
	{

	HBufC8 *pH=HBufC8::New(Length());
	if (pH)
		*pH=(*this);
	return(pH);
	}

EXPORT_C HBufC8 *TDesC8::AllocL() const
/**
Creates a new 8-bit heap descriptor and initialises it with a copy of this 
descriptor's data.

The function leaves, if creation of the descriptor fails.

@return A pointer to the 8-bit heap descriptor, if creation is successful.
*/
	{

	HBufC8 *pH=HBufC8::NewL(Length());
	*pH=(*this);
	return(pH);
	}

EXPORT_C HBufC8 *TDesC8::AllocLC() const
/**
Creates a new 8-bit heap descriptor, initialises it with a copy of this 
descriptor's data, and puts a pointer to the descriptor onto the cleanup stack.

The function leaves, if creation of the descriptor fails.

@return A pointer to the 8 bit heap descriptor, if creation is successful. 
        The pointer is also put onto the cleanup stack.
*/
	{

	HBufC8 *pH=HBufC8::NewLC(Length());
	*pH=(*this);
	return(pH);
	}
#endif	// __KERNEL_MODE__

#if !defined(__DES8_MACHINE_CODED__)

EXPORT_C TPtrC8 TDesC8::Left(TInt aLength) const
/**
Extracts the leftmost part of the data. 

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the leftmost part of the data.

@param aLength The length of the data to be extracted. If this value
               is greater than the length of the descriptor, the function
               extracts the whole of the descriptor.

@return The 8-bit non-modifiable pointer descriptor representing the leftmost 
        part of the data.

@panic USER 22 if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8PosOutOfRange));
	return(TPtrC8(Ptr(),Min(aLength,Length())));
	}

EXPORT_C TPtrC8 TDesC8::Right(TInt aLength) const
/**
Extracts the rightmost part of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the rightmost part of the data.

@param aLength The length of data to be extracted. If this value
               is greater than the length of the descriptor, the function 
               extracts the whole of the descriptor. 
               
@return The 8 bit non-modifiable pointer descriptor representing the rightmost 
        part of the data.

@panic USER 22 if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8PosOutOfRange));
	TInt len=Length();
	if (aLength>len)
		aLength=len;
    return(TPtrC8(Ptr()+len-aLength,aLength));
	}

EXPORT_C TPtrC8 TDesC8::Mid(TInt aPos) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by the length of the 
remainder of the data starting from the specified position.

@param aPos The starting position of the data to be extracted. This is an 
            offset value; a zero value refers to the leftmost data position. 
            
@return The 8-bit non-modifiable pointer descriptor representing the specified 
        portion of the data.
        
@panic USER 22  if aPos is negative or aPos is greater than the
                length of the descriptor.       
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes8PosOutOfRange));
    return(TPtrC8(Ptr()+aPos,len-aPos));
	}

EXPORT_C TPtrC8 TDesC8::Mid(TInt aPos,TInt aLength) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by its length.

@param aPos    The starting position of the data to be extracted. This is an 
               offset value; a zero value refers to the leftmost data position. 
@param aLength The length of data to be extracted.

@return The 8 bit non-modifiable pointer descriptor representing the specified 
        portion of the data.

@panic USER 22  if aPos is negative or aPos plus aLength is greater than the
                length of the descriptor.
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && (aPos+aLength)<=Length(),Panic(ETDes8PosOutOfRange));
    return(TPtrC8(Ptr()+aPos,aLength));
	}

#endif  // !defined(__DES8_MACHINE_CODED__)

#if !defined( __DES8_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TBufCBase8::TBufCBase8()
//
// Constructor
//
	: TDesC8(EBufC,0)
	{}

EXPORT_C TBufCBase8::TBufCBase8(const TUint8 *aString,TInt aMaxLength)
//
// Constructor
//
	: TDesC8(EBufC,0)
	{
	Copy(aString,aMaxLength);
	}

EXPORT_C TBufCBase8::TBufCBase8(const TDesC8 &aDes,TInt aMaxLength)
//
// Constructor
//
	: TDesC8(EBufC,0)
	{
	Copy(aDes,aMaxLength);
	}
#endif 

#ifndef __DES8_MACHINE_CODED__
EXPORT_C void TBufCBase8::Copy(const TUint8 *aString,TInt aMaxLength)
//
// Copy from a string.
//
	{

	TInt len=STRING_LENGTH(aString);
	__ASSERT_ALWAYS(len<=aMaxLength,Panic(ETDes8Overflow));
	memmove(WPtr(), aString, len);
	DoSetLength(len);
	}

EXPORT_C void TBufCBase8::Copy(const TDesC8 &aDes,TInt aMaxLength)
//
// Copy from a descriptor.
//
	{

	TInt len=aDes.Length();
	__ASSERT_ALWAYS(len<=aMaxLength,Panic(ETDes8Overflow));
	memmove(WPtr(), aDes.Ptr(), len);
	DoSetLength(len);
	}
#endif

#ifndef __KERNEL_MODE__
inline HBufC8::HBufC8(TInt aLength)
	:TBufCBase8(aLength)
	{}

EXPORT_C HBufC8 *HBufC8::New(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 8-bit heap descriptor.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested.

@return A pointer to the new 8-bit heap descriptor. NULL, if the 8-bit heap 
        descriptor cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	return new(STD_CLASS::Alloc(_FOFF(HBufC8,iBuf[aMaxLength]))) HBufC8(0);
	}

EXPORT_C HBufC8 *HBufC8::NewL(TInt aMaxLength)
/** 
Creates, and returns a pointer to, a new 8-bit heap descriptor, and leaves 
on failure.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested.

@return A pointer to the new 8 bit heap descriptor. The function leaves, if 
        the new 8-bit heap descriptor cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	return static_cast<HBufC8*>(User::LeaveIfNull(New(aMaxLength)));
	}

EXPORT_C HBufC8 *HBufC8::NewLC(TInt aMaxLength)
/**
Creates, adds a pointer onto the cleanup stack, and returns a pointer to, a 
new 8 bit heap descriptor; leaves on failure.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested.
                  
@return A pointer to the new 8-bit heap descriptor. The function leaves, if 
        the new 8-bit heap descriptor cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	HBufC8* buf=NewL(aMaxLength);
	CleanupStack::PushL(buf);
	return buf;
	}

EXPORT_C HBufC8 *HBufC8::NewMax(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 8-bit heap descriptor.

No data is assigned into the new descriptor but its length
is set to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested. This also means that the resulting maximum
                  length of the descriptor may be greater than its length.

@return A pointer to the new 8-bit heap descriptor. NULL, if the new 8-bit 
        heap descriptor cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	return new(STD_CLASS::Alloc(_FOFF(HBufC8,iBuf[aMaxLength]))) HBufC8(aMaxLength);
	}

EXPORT_C HBufC8 *HBufC8::NewMaxL(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 8-bit heap descriptor;
leaves on failure.

No data is assigned into the new descriptor but its length
is set to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested. This also means that the resulting maximum
                  length of the descriptor may be greater than its length.
                  
@return A pointer to the new 8-bit heap descriptor. The function leaves, if 
        the new 8-bit heap descriptor cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	return static_cast<HBufC8*>(User::LeaveIfNull(NewMax(aMaxLength)));
	}

EXPORT_C HBufC8 *HBufC8::NewMaxLC(TInt aMaxLength)
/**
Creates, adds a pointer onto the cleanup stack and returns a pointer to, a 
new 8-bit heap descriptor; leaves on failure.

No data is assigned into the new descriptor but its length
is set to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger than requested.
                  This also means that the resulting maximum
                  length of the descriptor may be greater than its length.
                  
@return A pointer to the new 8-bit heap descriptor. This is also put onto the 
        cleanup stack. The function leaves, if the new 8-bit heap descriptor
        cannot be created.

@panic USER 30 if aMaxLength is negative.

@see HBufC8::operator=()
*/
	{
	HBufC8* buf=NewMaxL(aMaxLength);
	CleanupStack::PushL(buf);
	return buf;
	}

EXPORT_C HBufC8 &HBufC8::operator=(const TUint8 *aString)
/**
Copies data into this 8-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or
last re-allocated.

@param aString A pointer to a zero-terminated string. 
                 
@return A reference to this 8 bit heap descriptor.

@panic USER 23  if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor,
*/
	{

	Copy(aString,(STD_CLASS::AllocLen(this)-sizeof(TDesC8))/sizeof(TUint8));
	return(*this);
	}

EXPORT_C HBufC8 &HBufC8::operator=(const TDesC8 &aDes)
/**
Copies data into this 8-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or last
re-allocated.

@param aDes An 8-bit non-modifiable descriptor.
            
@return A reference to this 8-bit heap descriptor.

@panic USER 23  if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor
*/
	{

	Copy(aDes,(STD_CLASS::AllocLen(this)-sizeof(TDesC8))/sizeof(TUint8));
	return(*this);
	}

EXPORT_C HBufC8 *HBufC8::ReAlloc(TInt aMaxLength)
/**
Expands or contracts the heap descriptor.

This is done by:

1. creating a new heap descriptor.

2. copying the original data into the new descriptor.

3. deleting the original descriptor.

@param aMaxLength The new requested maximum length of the descriptor. 
                  Note that the resulting heap cell size and, therefore,
                  the resulting maximum length of the descriptor may be
                  larger than requested.

@return A pointer to the new expanded or contracted 8 bit heap descriptor -  
        the original descriptor is deleted. NULL, if the new 8-bit heap descriptor 
        cannot be created - the original descriptor remains unchanged

@panic USER 26  if aMaxLength is less than the length of the existing data.
@panic USER 30  if aMaxLength is negative.
*/
	{

	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	__ASSERT_ALWAYS(Length()<=aMaxLength,Panic(ETDes8ReAllocTooSmall));
	return((HBufC8 *)STD_CLASS::ReAlloc(this,(aMaxLength*sizeof(TUint8))+sizeof(TDesC8)));
	}

EXPORT_C HBufC8 *HBufC8::ReAllocL(TInt aMaxLength)
/**
Expands or contracts the descriptor; leaves on failure.

This is done by:

1. creating a new heap descriptor.

2. copying the original data into the new descriptor.

3. deleting the original descriptor.

@param aMaxLength The new requested maximum length of the descriptor. 
                  Note that the resulting heap cell size and, therefore,
                  the resulting maximum length of the descriptor may be
                  larger than requested.

@return A pointer to the new expanded or contracted 8 bit heap descriptor -  
        the original descriptor is deleted. NULL, if the new 8-bit heap descriptor 
        cannot be created - the original descriptor remains unchanged

@panic USER 26  if aMaxLength is less than the length of the existing data.
@panic USER 30  if aMaxLength is negative.
*/
	{

	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	__ASSERT_ALWAYS(Length()<=aMaxLength,Panic(ETDes8ReAllocTooSmall));
	return((HBufC8 *)STD_CLASS::ReAllocL(this,(aMaxLength*sizeof(TUint8))+sizeof(TDesC8)));
	}

EXPORT_C TPtr8 HBufC8::Des()
/**
Creates and returns an 8-bit modifiable pointer descriptor for the data
represented by this 8-bit heap descriptor.

The content of a heap descriptor normally cannot be altered, other than by 
complete replacement of the data. Creating a modifiable pointer descriptor 
provides a way of changing the data.

The modifiable pointer descriptor is set to point to this heap descriptor's 
data.

The length of the modifiable pointer descriptor is set to the length of this 
heap descriptor.

The maximum length of the modifiable pointer descriptor is set to the length 
of the heap descriptor's buffer. Note that the maximum length is the length 
of the descriptor buffer in the allocated host heap cell; this may be greater 
than the maximum length requested when this descriptor was originally created 
or last re-allocated.

When data is modified through this new pointer descriptor, the lengths of 
both it and this heap descriptor are changed.

Note that it is a common mistake to use Des() to create a TDesC8& reference. 
While not incorrect, it is simpler and much more efficient to simply dereference 
the heap descriptor.

@return An 8-bit modifiable pointer descriptor representing the data in this 
        8-bit heap descriptor.
*/
	{
	return DoDes((STD_CLASS::AllocLen(this)-sizeof(TDesC8))/sizeof(TUint8));
	}
#endif	// __KERNEL_MODE__

#ifndef __DES8_MACHINE_CODED__
EXPORT_C void TDes8::SetLength(TInt aLength)
/**
Sets the length of the data represented by the descriptor to the
specified value.

@param aLength The new length of the descriptor.

@panic USER 23  if alength is negative or is greater than the maximum length of
                this (target) descriptor.
*/
	{

	__ASSERT_ALWAYS(TUint(aLength)<=TUint(MaxLength()),Panic(ETDes8Overflow));
	DoSetLength(aLength);
	if (Type()==EBufCPtr)
		((SBufCPtr8 *)this)->ptr->length=aLength; // Relies on iType==0 for an TBufC
  	}

EXPORT_C void TDes8::SetMax()
/**
Sets the length of the data to the maximum length of the descriptor.
*/
	{

	SetLength(iMaxLength);
	}

EXPORT_C void TDes8::Copy(const TUint8 *aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.
               
@panic USER 23  if the length of aString, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
	{

	TInt len=STRING_LENGTH(aString);
	SetLength(len);
    memmove(WPtr(), aString, len);
	}

EXPORT_C void TDes8::Copy(const TUint8 *aBuf,TInt aLength)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aBuf    The start address of data to be copied.
@param aLength The length of data to be copied.

@panic USER 23  if aLength is greater than the maximum length of
                this (target) descriptor.
*/
	{

	SetLength(aLength);
    memmove(WPtr(), aBuf, aLength);
	}

EXPORT_C void TDes8::Copy(const TDesC8 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit non-modifiable descriptor. The length of the data cannot 
            be greater than the maximum length of the target descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this (target) descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
    memmove(WPtr(), aDes.Ptr(), len);
	}
#endif

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::Copy(const TDesC16 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor. Each double-byte value can 
            only be copied into the corresponding single byte when the
            double-byte value is less than decimal 256. A double-byte value of
            256 or greater cannot be  copied and the corresponding single byte
            is set to a value of decimal 1.
            
@panic USER 23  if the length of the aDes is greater than the maximum length
                of this (target) descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	const TUint16 *pE=pS+len;
	TUint8 *pT=WPtr();
	while (pS<pE)
		{
		TUint c=(*pS++);
		if (c>=0x100)
			c=1;
		*pT++=(TUint8)c;
		}
	}
#endif

#ifndef __DES8_MACHINE_CODED__
EXPORT_C void TDes8::Append(TChar aChar)
/**
Appends a character onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aChar The single character to be appended. The length of the descriptor 
             is incremented by one. The function assumes that the character
             is non-Unicode and that it can be represented by a single byte.

@panic USER 23  if the resulting new length of this descriptor is greater than
                its maximum length.
*/
	{

	TInt len=Length();
	TUint8 *pB=WPtr()+len;
	SetLength(len+1);
	*pB++=(TUint8)aChar;
	}

EXPORT_C void TDes8::Append(const TUint8 *aBuf,TInt aLength)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aBuf    A pointer to the data to be copied.
@param aLength The length of the data to be copied.

@panic USER 23  if the resulting new length of this descriptor is greater than
                its maximum length.
@panic USER 29  if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8LengthNegative));
	TInt len=Length();
	SetLength(len+aLength);
    memmove(WPtr()+len, aBuf, aLength);
	}

EXPORT_C void TDes8::Append(const TDesC8 &aDes)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aDes An 8-bit non-modifiable descriptor whose data is to be appended.

@panic USER 23  if the resulting new length of this descriptor is greater than
                its maximum length.
*/
	{

	TInt len=Length();
    TInt n=aDes.Length();
	SetLength(len+n);
    memmove(WPtr()+len, aDes.Ptr(), n);
	}
#endif

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::Append(const TDesC16 &aDes)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aDes A 16-bit non-modifiable descriptor whose data is to be appended. 
            Each double-byte value can only be appended as a single byte when
            the double-byte value is less than decimal 256. A double-byte value
            of 256 or greater cannot be appended and the corresponding single
            byte is set to a value of decimal 1.

@panic USER 23  if the resulting new length of this descriptor is greater than
                its maximum length.
*/
	{

	TInt len=Length();
	TInt n=aDes.Length();
	const TUint16* pS=aDes.Ptr();
	const TUint16* pE=pS+n;
	TUint8 *pT=WPtr()+len;
	SetLength(len+n);
	while (pS<pE)
		{
		TUint c=(*pS++);
		if (c>=0x100)
			c=1;
		*pT++=(TUint8)c;
		}
	}
#endif

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::Swap(TDes8 &aDes)
/**
Swaps the data represented by this descriptor with the data represented by 
the specified descriptor.

The lengths of both descriptors are swapped to reflect the change.

Note that each descriptor must be capable of accommodating the contents of
the other descriptor.

@param aDes The 8-bit modifiable descriptor whose data is to be swapped with 
            the data of this descriptor.
            
@panic USER 23  if the maximum length of either descriptor is smaller than the 
                length of the other descriptor.
*/
	{

	TInt l=Length();
	TInt r=aDes.Length();
	aDes.SetLength(l);
	SetLength(r);
	TInt s=Min(l,r);
	l-=s;
	r-=s;
	TUint8 *pL=WPtr();
	TUint8 *pR=aDes.WPtr();
	while (s--)
		{
		TChar a=(*pL);
		*pL++=(*pR);
		*pR++=(TUint8)a;
		}
	while (l--)
		*pR++=(*pL++);
	while (r--)
		*pL++=(*pR++);
	}
#endif

#ifndef __DES8_MACHINE_CODED__
EXPORT_C void TDes8::Fill(TChar aChar)
/**
Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled from the beginning up to its current length. The 
descriptor's length does not change. It is not filled to its maximum length.

@param aChar The fill character. The function assumes that the character is 
             non-Unicode, and that it can be represented by a single byte.
*/
	{

    memset(WPtr(), (TInt)(aChar.operator TUint()), Length());
	}
#endif

EXPORT_C void TDes8::Fill(TChar aChar,TInt aLength)
/**
Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled with the specified number of characters.
and its length is changed to reflect this.

@param aChar   The fill character. The function assumes that the character is 
               non-Unicode, and that it can be represented by a single byte. 
@param aLength The new length of the descriptor and the number of fill
               characters to be copied into it.
               
@panic USER 23  if aLength is negative or is greater than the maximum length
                of this descriptor.
*/
	{

	SetLength(aLength);
    memset(WPtr(), (TInt)(aChar.operator TUint()), aLength);
	}

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::AppendFill(TChar aChar,TInt aLength)
/**
Appends and fills this descriptor with the specified character.

The descriptor is appended with the specified number of characters.
and its length is changed to reflect this.

@param aChar   The fill character. The function assumes that the character
               is non-Unicode and that it can be represented by a single byte.
@param aLength The number of fill characters to be appended.

@panic USER 23  if aLength is negative, or the resulting length of this
                descriptor is greater than its maximum length.
*/
	{

	TInt len=Length();
	SetLength(len+aLength);
    memset(WPtr()+len, (TInt)(aChar.operator TUint()), aLength);
	}
#endif

#ifndef __DES8_MACHINE_CODED__
#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::ZeroTerminate()
/**
Appends a zero terminator onto the end of this descriptor's data.

The length of the descriptor is not changed. It must, however, be strictly
less than the descriptor's maximum length. 
This condition guarantees that there is sufficient space for the
zero terminator.

@panic USER 23  if the descriptor's length is not strictly less than its
                maximum length.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(len<MaxLength(),Panic(ETDes8Overflow));
	WPtr()[len]=0;
	}

EXPORT_C const TUint8 *TDes8::PtrZ()
/**
Appends a zero terminator onto the end of this descriptor's data and returns 
a pointer to the data.

The length of the descriptor is not changed. It must, however, be strictly
less than the descriptor's maximum length.
This condition guarantees that there is sufficient space for the
zero terminator.

@return A pointer to the descriptor's zero terminated data.

@panic USER 23  if the descriptor's length is not strictly less than its
                maximum length.
*/
	{

	ZeroTerminate();
	return(Ptr());
	}
#endif

EXPORT_C void TDes8::Zero()
/**
Sets the length of the data to zero.
*/
	{

	SetLength(0);
	}

EXPORT_C void TDes8::FillZ()
/**
Fills the descriptor's data area with binary zeroes, i.e. 0x00, replacing any 
existing data.

The descriptor is filled from the beginning up to its current length. The 
descriptor's length does not change. It is not filled to its maximum length.
*/
	{

    memclr(WPtr(), Length());
	}
#endif

EXPORT_C void TDes8::FillZ(TInt aLength)
/**
Fills the descriptor's data area with binary zeroes, i.e. 0x00, replacing any 
existing data, and changes its length.

The descriptor is filled with the specified number of binary zeroes.
The descriptor's length is changed to reflect this.

@param aLength The new length of the descriptor and the number of binary zeroes
               to be copied into it. 
               
@panic USER 23  if aLength is negative, or is greater than the maximum length
                of this descriptor.
*/
	{

	SetLength(aLength);
    memclr(WPtr(), aLength);
	}

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::Fold()
/**
Performs folding on the content of this descriptor.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.
*/
	{

	TUint8 *pB=WPtr();
	TInt len=Length();
	const TUint8* table=__FoldCollTab8;
	while (len--)
		{
		*pB=table[*pB];
		pB++;
		}
	}

EXPORT_C void TDes8::Collate()
/**
Performs collation on the content of this descriptor.
@deprecated
*/
	{

	TUint8 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TChar c=User::Collate(*pB);
		*pB++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::LowerCase()
/**
Converts the content of this descriptor to lower case.

Conversion is implemented as appropriate to the current locale.
*/
	{

	TUint8 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TCharLC c(*pB);
		*pB++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::UpperCase()
/**
Converts the content of this descriptor to upper case.

Conversion is implemented as appropriate to the current locale.
*/
	{

	TUint8 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TCharUC c(*pB);
		*pB++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::Capitalize()
/**
Capitalises the content of this descriptor.

Capitalisation is implemented as appropriate to the current locale.
*/
	{

	TUint8 *pB=WPtr();
	TInt len=Length();
	if (len--)
		{
		*pB=(TUint8)User::TitleCase(*pB);
		++pB;
		while (len--)
			{
			*pB=(TUint8)User::LowerCase(*pB);
			++pB;
			}
		}
	}

EXPORT_C void TDes8::CopyF(const TDesC8 &aDes)
/**
Copies and folds data from the specified descriptor into this descriptor
replacing any existing data.

The length of this descriptor is set to reflect the new 
data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.

@param aDes An 8-bit non-modifiable descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	TUint8 *pT=WPtr();
	const TUint8* table=__FoldCollTab8;
	while (len--)
		{
		*pT++=table[*pS++];
		}
	}

EXPORT_C void TDes8::CopyC(const TDesC8 &aDes)
/**
Copies and collates data from the specified descriptor 
into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8 bit non-modifiable descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this target descriptor.
@deprecated
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	TUint8 *pT=WPtr();
	while (len--)
		{
		TChar c=User::Collate(*pS++);
		*pT++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::CopyLC(const TDesC8 &aDes)
/**
Copies text from the specified descriptor and converts it to lower case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to lower case is implemented as appropriate to the current locale.

@param aDes An 8-bit non-modifiable descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	TUint8 *pT=WPtr();
	while (len--)
		{
		TCharLC c(*pS++);
		*pT++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::CopyUC(const TDesC8 &aDes)
/**
Copies text from the specified descriptor and converts it to upper case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to upper case is implemented as appropriate to the current locale.

@param aDes An 8-bit non-modifiable descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	TUint8 *pT=WPtr();
	while (len--)
		{
		TCharUC c(*pS++);
		*pT++=(TUint8)c;
		}
	}

EXPORT_C void TDes8::CopyCP(const TDesC8 &aDes)
/**
Copies text from the specified descriptor and capitalises it before putting 
it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Capitalisation is implemented as appropriate to the current locale.

@param aDes An 8-bit non-modifiable descriptor.

@panic USER 23  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	TUint8 *pT=WPtr();
	if (len--)
		{
		TChar c(*pS++);
#ifdef _UNICODE
		c.TitleCase();
#else
		c.UpperCase();
#endif
		*pT++=(TUint8)c;
		while (len--)
			{
			TCharLC c=(*pS++);
			*pT++=(TUint8)c;
			}
		}
	}

EXPORT_C void TDes8::Repeat(const TUint8 *aBuf,TInt aLength)
/**
Copies data with repetition into this descriptor, from a memory location
specified by pointer, replacing any existing data.

Copying proceeds until this descriptor is filled up to its current length. 
If it cannot contain a whole number of copies of the source data, then the 
last copy is truncated.

@param aBuf    A pointer to data to be repeatedly copied. 
@param aLength The length of data to be copied.

@panic USER 29  if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8LengthNegative));
	TUint8 *pB=WPtr();
	TInt len=Length();
	if (len && aLength)
		{
		while (len)
			{
			TInt i=Min(len,aLength);
			pB=memCopy(pB,aBuf,i);
			len-=i;
			}
		}
	}

EXPORT_C void TDes8::Repeat(const TDesC8 &aDes)
/**
Copies data with repetition into this descriptor, from another descriptor,
replacing any existing data.

Copying proceeds until this descriptor is filled up to its current length. 
If it cannot contain a whole number of copies of the source data, then the 
last copy is truncated.

@param aDes An 8-bit non-modifiable descriptor whose data is to be repeatedly 
            copied.
*/
	{

	Repeat(aDes.Ptr(),aDes.Length());
	}

EXPORT_C void TDes8::Trim()
/**
Deletes leading and trailing whitespace characters from the descriptor's data.

The length of the descriptor is reduced to reflect the loss of the whitespace characters.

@see TDes8::TrimLeft()
@see TDes8::TrimRight()
*/
	{

	TrimLeft();
	TrimRight();
	}

EXPORT_C void TDes8::TrimAll()
/**
Deletes leading and trailing whitespace characters from the descriptor's data and 
replaces each contiguous set of whitespace characters within the data by one whitespace 
character.

The length of the descriptor is reduced to reflect the loss of the whitespace 
characters.

@see TDes8::Trim()
*/
	{

	TrimLeft();
	TrimRight();
	TUint8 *pBuf=(TUint8 *)Ptr();
	TUint8 *pSrc=pBuf;
	TUint8 *pDst=pBuf;
	TInt len=Length();
	TInt spaces=0;
	while (len--)
		{
		TChar c=*pSrc;
		if (c.IsSpace())
			{
			if (spaces++==0)
				{
				if (pDst!=pSrc)
					*pDst=*pSrc;
				pDst++;
				}
			}
		else
			{
			spaces=0;
			if (pDst!=pSrc)
				*pDst=*pSrc;
			pDst++;
			}
		pSrc++;
		}
	Delete(pDst-pBuf, pSrc-pDst);
	}

EXPORT_C void TDes8::TrimLeft()
/**
Deletes leading whitespace characters from the descriptor's data.

All whitespace characters up to, but not including the first non-whitespace
character, are deleted.

The length of the descriptor is reduced to reflect the loss of the
whitespace characters.
*/
	{

	const TUint8 *pBuf=Ptr();
	const TUint8 *pB=pBuf;
	TInt len=Length();
	while (len--)
		{
		TChar c=(*pB);
		if (!c.IsSpace())
			break;
		pB++;
		}
	Delete(0,pB-pBuf);
	}

EXPORT_C void TDes8::TrimRight()
/**
Deletes trailing whitespace characters from the descriptor's data.

The process starts on the right hand side of the descriptor's data
and proceeds to the left. 

All whitespace characters up to, but not including the first non-whitespace character, 
are deleted.

The length of the descriptor is reduced to reflect the loss of the whitespace
characters.
*/
	{

	TInt len=Length();
	if (len==0)
		return;
	const TUint8 *pB=Ptr()+len-1;
	TInt s=len;
	while (s)
		{
		TChar c=(*pB--);
		if (!c.IsSpace())
			break;
		s--;
		}
	Delete(s,len-s);
	}

EXPORT_C void TDes8::Insert(TInt aPos,const TDesC8 &aDes)
/**
Inserts data into this descriptor.

The length of this descriptor is changed to reflect the extra data.

@param aPos The position within the data where insertion is to start. This 
            is an offset value; a zero value refers to the leftmost data
            position.
            
@param aDes An 8 bit non modifiable descriptor whose data is to be inserted.

@panic USER 22  if aPos is negative or is greater than the length of this
                descriptor.
@panic USER 23  if the resulting length of this descriptor is greater than its
                maximum length.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes8PosOutOfRange));
	TInt s=aDes.Length();
	__ASSERT_ALWAYS((len+s)<=MaxLength(),Panic(ETDes8Overflow));
	TUint8 *pB=WPtr();
	memmove(pB+aPos+s,pB+aPos,len-aPos);
	memmove(pB+aPos,aDes.Ptr(),aDes.Length());
	SetLength(len+s);
	}

EXPORT_C void TDes8::Delete(TInt aPos,TInt aLength)
/**
Deletes data from this descriptor.

The length of this descriptor is changed to reflect the loss of data.

@param aPos    The position within the data where deletion is to start. This 
               is an offset value; a zero value refers to the leftmost data
               position.
               
@param aLength The length of data to be deleted. If necessary, the function 
               adjusts this value to ensure that no data beyond the end of the
               descriptor data area is deleted.

@panic USER 22  if aPos is negative or is greater than the length of this
                descriptor.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes8PosOutOfRange));
	TInt d=Min(len-aPos,aLength);
	TUint8 *pB=WPtr();
	memmove(pB+aPos,pB+aPos+d,len-aPos-d);
	SetLength(len-d);
	}

EXPORT_C void TDes8::Replace(TInt aPos,TInt aLength,const TDesC8 &aDes)
/**
Replaces data in this descriptor.

The specified length can be different to the length of the replacement data.
The length of this descriptor changes to reflect the change of data.

@param aPos    The position within the data where replacement is to start.
               This is an offset value; a zero value refers to the leftmost
               data position.
               
@param aLength The length of data to be replaced.

@param aDes    The source 8-bit non-modifiable descriptor whose data is to
               replace the target descriptor's data at aPos.

@panic USER 20  if aLength is negative or the sum of aLength and aPos is
                greater than the length of this descriptor.
                
@panic USER 22  if aPos is negative or is greater than the length of this
                descriptor.
                
@panic USER 23  if the resulting length of this descriptor is greater than its
                maximum length.

@panic USER 28  if the length of the source descriptor aDes is negative or is
                greater than the maximum length of this target descriptor,
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes8PosOutOfRange));
	__ASSERT_ALWAYS(aLength>=0 && aPos+aLength<=len,Panic(ETDes8LengthOutOfRange));
	TInt s=aDes.Length();
	TInt maxlen=MaxLength();
	__ASSERT_ALWAYS(s>=0 && s<=maxlen,Panic(ETDes8RemoteLengthOutOfRange));
	__ASSERT_ALWAYS((len+s-aLength)<=maxlen,Panic(ETDes8Overflow));
	TUint8 *pB=WPtr();
	memmove(pB+aPos+s,pB+aPos+aLength,len-aPos-aLength);
	memmove(pB+aPos,aDes.Ptr(),s);
	SetLength(len+s-aLength);
	}

EXPORT_C void TDes8::Justify(const TDesC8 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Copies data into this descriptor and justifies it, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The target area is considered to be an area of specified width positioned at
the beginning of this descriptor's data area. Source data is copied into, and
aligned within this target area according to the specified alignment
instruction.

If the length of the target area is larger than the length of the source, then
spare space within the target area is padded with the fill character.

@param aDes        An 8-bit non-modifiable descriptor containing the source data.
                   The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and 
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth).

@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area

@param aFill       The fill character used to pad the target area. 

@panic USER 23  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    Zero();
    AppendJustify(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
    }

EXPORT_C void TDes8::AppendJustify(const TDesC8 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends data onto the end of this descriptor's data and justifies it.
	
The source of the appended data is an existing descriptor.
	
The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within this target area according to the specified alignment instruction.
	
If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.
		
@param aDes        An 8-bit non-modifiable descriptor containing the source
                   data. The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth). 
	
@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
	               re-set to the length of the data source.
	
@param anAlignment The alignment of the data within the target area. 
	
@param aFill       The fill character used to pad the target area.

@panic USER 23  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    AppendJustify(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes8::AppendJustify(const TDesC8 &aDes,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends data onto the end of this descriptor's data and justifies it.
	
The source of the appended data is an existing descriptor.
	
The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within this target area according to the specified alignment instruction.
	
If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.
	
@param aDes        An 8-bit non-modifiable descriptor containing the source data. 

@param aLength     The length of data to be copied from the source descriptor. 
                   If this is greater than the width of the target area, then
                   the length of data copied is limited to the width.
                   The length of data to be copied must not be 	greater than
                   the length of the source descriptor. Note that this
                   condition is not automatically tested. 
                   
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area.

@panic USER 23  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    AppendJustify(aDes.Ptr(),aLength,aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes8::AppendJustify(const TUint8 *aString,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends a zero terminated string onto the end of this descriptor's data and 
justifies it.

The zero terminator is not copied.

The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within, this target area according to the specified alignment instruction.

If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.

@param aString     A pointer to a zero terminated string. The length of the
                   data to be copied is the smaller of:
                   the length of the string (excluding the zero terminator),
                   and the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth). 

@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width
                   is re-set to the length of the  zero terminated string
                   (excluding the zero terminator).
               
@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area.

@panic USER 23  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    AppendJustify(aString,STRING_LENGTH(aString),aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes8::AppendJustify(const TUint8 *aString,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends data onto the end of this descriptor's data and justifies it.

The source of the appended data is a memory location.

The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within, this target area according to the specified alignment instruction.

If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.

@param aString     A pointer to a source memory location. 

@param aLength     The length of data to be copied. If this is greater than the 
                   width of the target area, then the length of data copied is
                   limited to the width.
                    
@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area.

@panic USER 23  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
                
@panic USER 29  if aLength is negative.               
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8LengthNegative));
	if (aWidth==KDefaultJustifyWidth)
		aWidth=aLength;
	if (aLength>aWidth)
		aLength=aWidth;
	TInt offset=Length();
	AppendFill(aFill,aWidth);
	TInt r=aWidth-aLength;
	if (anAlignment==ECenter)
		r>>=1;
	else if (anAlignment==ELeft)
		r=0;
	memmove(WPtr()+offset+r,aString,aLength);
	}
#endif	// __KERNEL_MODE__

EXPORT_C void TDes8::Num(TInt64 aVal)
//
// Convert a TInt64 to the descriptor.
//
/**
Converts the 64-bit signed integer into a decimal character representation
and copies the conversion into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

If the integer is negative, the character representation is prefixed by a 
minus sign.

@param aVal The 64-bit signed integer value.
*/
	{
	Zero();
	AppendNum(aVal);
	}

EXPORT_C void TDes8::Num(TUint64 aVal, TRadix aRadix)
/**
Converts the specified 64-bit unsigned integer into a character representation 
based on the specified number system and copies the conversion into this descriptor, 
replacing any existing data.

The length of this descriptor is set to reflect the new data.

When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.

@param aVal   The 64-bit integer value. This is treated as an unsigned value. 
@param aRadix The number system representation for the 64-bit integer.
*/
	{
	Zero();
	AppendNum(aVal, aRadix);
	}

EXPORT_C void TDes8::NumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth)
/**
Converts the specified unsigned integer into a fixed width character
representation based on the specified number system and copies the conversion
into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The function generates the exact number of specified characters, either padding 
to the left with character zeroes or discarding low order characters as necessary.

When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.

This function is equivalent to using Format() with parameters which specify:

1. a fixed length target field

2. padding with zero characters, for example "%08x".

When this is the case, always use NumFixedWidth() in preference 
to Format() as it is more efficient.

@param aVal   The unsigned integer value. 
@param aRadix The number system representation for the unsigned integer. 
@param aWidth The number of characters: to be used to contain the conversion, 
              to be copied into this descriptor.
              
@see TDes8::Format()
*/
	{

	Zero();
	AppendNumFixedWidth(aVal,aRadix,aWidth);
	}

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::NumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth)
/**
Converts the specified unsigned integer into a fixed width character
representation based on the specified number system and copies the conversion
into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The function generates the exact number of specified characters, either padding 
to the left with character zeroes or discarding low order characters as
necessary.

When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.

This function is equivalent to using Format() with parameters which specify:

1. a fixed length target field

2. padding with zero characters, for example "%08x".

When this is the case, always use NumFixedWidthUC() in 
preference to Format() as it is more efficient.

@param aVal   The unsigned integer value. 
@param aRadix The number system representation for the unsigned integer. 
@param aWidth The number of characters to be used to contain the conversion,
              and to be copied into this descriptor.

@see TDes8::Format()
*/
	{

    Zero();
    AppendNumFixedWidthUC(aVal,aRadix,aWidth);
    }

EXPORT_C void TDes8::NumUC(TUint64 aVal, TRadix aRadix)	//NOT __KERNEL_MODE__
/**
Converts the specified 64-bit unsigned integer into a character representation 
based on the specified number system and copies the conversion into this descriptor, 
replacing any existing data.

The length of this descriptor is set to reflect the new data.

When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.

@param aVal   The 64-bit integer value. This is treated as an unsigned value. 
@param aRadix The number system representation for the 64-bit integer. If no 
              explicit value is specified, then EDecimal is the default.
*/
	{
	Zero();
	AppendNumUC(aVal,aRadix);
	}
#endif	// __KERNEL_MODE__

EXPORT_C void TDes8::AppendNum(TInt64 aVal)
/**
Converts the 64-bit signed integer into a decimal character representation 
and appends the conversion onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

If the integer is negative, the character representation is prefixed by a 
minus sign.

@param aVal The 64-bit signed integer value.
*/
	{
	if (aVal < 0)
		{
		Append('-');
		aVal = -aVal;
		}

	AppendNum(aVal, EDecimal);
	}

#ifndef __DES_MACHINE_CODED__
GLDEF_C TInt __DoConvertNum(TUint aVal, TRadix aRadix, TUint aA, TUint8*& aDest)
	{
	__KERNEL_CHECK_RADIX(aRadix);
	TUint radix = (TUint)aRadix;
	TUint8* p = aDest;
	TBool out16 = (aA>255);
	aA &= 0xff;
	do	{
		TUint q = aVal/radix;
		TUint c = aVal-q*radix;
		(c>9) ? c+=(aA-10) : c+='0';
		aVal = q;
		if (out16)
			*--p = 0;
		*--p = (TUint8)c;
		} while (aVal);
	TInt l = aDest - p;
	aDest = p;
	return l;
	}

GLDEF_C TInt __DoConvertNum(TUint64 aVal, TRadix aRadix, TUint aA, TUint8*& aDest)
	{
	__KERNEL_CHECK_RADIX(aRadix);
	TUint radix = (TUint)aRadix;
	TUint8* p = aDest;
	TBool out16 = (aA>255);
	TUint8 a = static_cast<TUint8>(aA);
	while (aVal >= UI64LIT(0x100000000))
		{
		TUint8 c = static_cast<TUint8>(aVal % radix);
		aVal /= radix;
		(c > 9) ? c = static_cast<TUint8>(c + a - 10) : c = static_cast<TUint8>(c + '0');
		if (out16)
			*--p = 0;
		*--p = c;
		}
	TInt l = aDest - p;
	aDest = p;
	return l + __DoConvertNum((TUint)aVal, aRadix, aA, aDest);
	}
#endif

void TDes8::DoPadAppendNum(TInt l, TInt aW, const TUint8* p)
	{
	if (aW<=0)
		{
		Append(p, l);
		return;
		}
	TInt l0 = Length();
	SetLength(l0 + aW);
	TUint8* d = WPtr() + l0;
	for (; aW>l; --aW) *d++ = (TUint8)'0';
	memcpy(d, p, aW);
	}

void TDes8::DoAppendNum(TUint64 aVal, TRadix aRadix, TUint aA, TInt aW)
//
// Convert a TUint64 into the descriptor.
//
	{
	TUint8 buf[APPEND_BUF_SIZE_64];
	TUint8* p = buf + APPEND_BUF_SIZE_64;
	TInt l = __DoConvertNum(aVal, aRadix, aA, p);
	// coverity[overrun-local]
	DoPadAppendNum(l, aW, p);
	}

EXPORT_C void TDes8::AppendNum(TUint64 aVal, TRadix aRadix)
/**
Converts the specified 64-bit unsigned integer into a character representation 
based on the specified number system and appends the conversion onto the end 
of this descriptor's data. The length of this descriptor is incremented to 
reflect the new content

When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.

@param aVal   The 64-bit integer value. This is treated as an unsigned value. 
@param aRadix The number system representation for the 64-bit integer.
*/
	{
	DoAppendNum(aVal, aRadix, 'a', 0);
	}

EXPORT_C void TDes8::AppendNumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth)
/**
Converts the specified unsigned integer into a fixed width character
representation based on the specified number system and appends the conversion
onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

The function generates the exact number of specified characters, either padding 
to the left with character zeroes or discarding low order characters as
necessary.

When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.

@param aVal   The unsigned integer value. 
@param aRadix The number system representation for the unsigned integer. 
@param aWidth The number of characters to be used to contain the conversion,
              and to be appended to this descriptor.
*/
	{
	DoAppendNum(aVal, aRadix, 'a', aWidth);
	}

#if (!defined(__DES8_MACHINE_CODED__) && !defined(__KERNEL_MODE__))
EXPORT_C TPtr8 TDes8::LeftTPtr(TInt aLength) const
/**
Extracts the leftmost part of the data. 

The function does not cut or remove any data but constructs a modifiable 
pointer descriptor to represent the leftmost part of the data.

@param aLength The length of the data to be extracted. If this value
               is greater than the length of the descriptor, the function
               extracts the whole of the descriptor.

@return The 8-bit modifiable pointer descriptor representing the leftmost 
        part of the data.

@panic USER 22  if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8PosOutOfRange));
	TInt len=Min(aLength,Length());
	return(TPtr8((TUint8*)Ptr(),len,len));
	}

EXPORT_C TPtr8 TDes8::RightTPtr(TInt aLength) const
/**
Extracts the rightmost part of the data.

The function does not cut or remove any data but constructs a modifiable 
pointer descriptor to represent the rightmost part of the data.

@param aLength The length of data to be extracted. If this value
               is greater than the length of the descriptor, the function 
               extracts the whole of the descriptor. 
               
@return The 8 bit modifiable pointer descriptor representing the rightmost 
        part of the data.

@panic USER 22  if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8PosOutOfRange));
	TInt len=Length();
	if (aLength>len)
		aLength=len;
	return(TPtr8((TUint8*)Ptr()+len-aLength,aLength,aLength));
	}

EXPORT_C TPtr8 TDes8::MidTPtr(TInt aPos) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by the length of the 
remainder of the data starting from the specified position.

@param aPos The starting position of the data to be extracted. This is an 
            offset value; a zero value refers to the leftmost data position. 
            
@return The 8-bit modifiable pointer descriptor representing the specified 
        portion of the data.
        
@panic USER 22  if aPos is negative or aPos is greater than the
                length of the descriptor.       
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes8PosOutOfRange));
	return(TPtr8((TUint8*)Ptr()+aPos,len-aPos,len-aPos));
	}

EXPORT_C TPtr8 TDes8::MidTPtr(TInt aPos,TInt aLength) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by its length.

@param aPos    The starting position of the data to be extracted. This is an 
               offset value; a zero value refers to the leftmost data position. 
@param aLength The length of data to be extracted.

@return The 8 bit modifiable pointer descriptor representing the specified 
        portion of the data.

@panic USER 22  if aPos is negative or aPos plus aLength is greater than the
                length of the descriptor.
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && (aPos+aLength)<=Length(),Panic(ETDes8PosOutOfRange));
	return(TPtr8((TUint8*)Ptr()+aPos,aLength,aLength));
	}
#endif

#ifndef __KERNEL_MODE__
EXPORT_C void TDes8::AppendNumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth)
/**
Converts the specified unsigned integer into a fixed width character
representation based on the specified number system and appends the conversion
onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

The function generates the exact number of specified characters, either
padding to the left with character zeroes or discarding low order characters
as necessary.

When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.

@param aVal   The unsigned integer value. 
@param aRadix The number system representation for the unsigned integer. 
@param aWidth The number of characters to be used to contain the conversion,
              and to be appended to this descriptor.
*/
	{
	DoAppendNum(aVal, aRadix, 'A', aWidth);
	}

EXPORT_C void TDes8::AppendNumUC(TUint64 aVal, TRadix aRadix)
/** Converts the specified 64-bit unsigned integer into a character
representation based on the specified number system and appends the conversion
onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.

@param aVal   The 64-bit integer value. This is treated as an unsigned value. 
@param aRadix The number system representation for the 64-bit integer. If no 
              explicit value is specified, then EDecimal is the default.
*/
	{
	DoAppendNum(aVal, aRadix, 'A', 0);
	}

EXPORT_C void TDes8::Format(TRefByValue<const TDesC8> aFmt,...)
/**
Formats and copies text into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The function takes a format string and a variable number of arguments.
The format string contains literal text embedded with directives for converting
the trailing list of arguments into text.

The embedded directives are character sequences prefixed with the '%' character.
The literal text is simply copied into this descriptor unaltered while
the '%' directives are used to convert successive arguments from the
trailing list.

The resulting stream of literal text and converted arguments is copied into
this descriptor.

The syntax of the embedded directives follows one of four general patterns.

Note that formatting of single numerical values can be achieved more
conveniently using the Num() and NumUC() member functions of this class.

The full description of the syntax of a format string cannot be	included here.
For full details, navigate to the Symbian OS guide, and follow the hierarchy of links:

@code
Symbian OS Guide
	Base
		Using  User Library (E32)
			Buffers and Strings
				Using Descriptors
					How to Use Descriptors
						Format string syntax
@endcode

@param aFmt The descriptor containing the format string.
            The TRefByValue class provides a constructor which takes a
            TDesC8 type.

@param ...  A variable number of arguments to be converted to text as
            dictated by the format string. 

@panic USER 23  if the  resulting length of text in this descriptor exceeds
                the descriptor's maximum length.
@panic USER 24  if the format string has incorrect syntax.

@see TDes8::Num()
@see TDes8::NumUC()
*/
	{

    VA_LIST list;
    VA_START(list,aFmt);
	// coverity[uninit_use_in_call]
    FormatList(aFmt,list);
    }

EXPORT_C void TDes8::FormatList(const TDesC8 &aFmt,VA_LIST aList)
/**
Formats and copies text into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The behaviour of this function is the same as Format(). In practice, it is 
better and easier to use Format(), passing a variable number of arguments 
as required by the format string.

@param aFmt  The descriptor containing the format string.
@param aList A pointer to an argument list.

@see TDes8::Format()
@see VA_LIST
*/
	{

	Zero();
	AppendFormatList(aFmt,aList);
	}

EXPORT_C void TDes8::AppendFormat(TRefByValue<const TDesC8> aFmt,TDes8Overflow *aOverflowHandler,...)
/**
Formats and appends text onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

The function takes a format string and a variable number of arguments.
The format string contains literal text, embedded with directives,
for converting the trailing list of arguments into text.

The embedded directives are character sequences prefixed with the '%' character.
The literal text is simply copied into this descriptor unaltered while
the '%' directives are used to convert successive arguments from the
trailing list. See the description of the Format() function.

Literal text is appended on a character by character basis.
If it results in the length of this descriptor exceeding its maximum length,
then the function:

1. calls the Overflow() member function of the overflow handler, if an overflow
   handler is supplied
2  raises a USER 23 panic, if no overflow handler is supplied.

As much literal text as possible will have been copied into this descriptor
and this descriptor will have reached its maximum length.

Text converted from a trailing argument is appended as a complete string.
If an attempt to append this string fails because the resulting length
of this descriptor would exceed its maximum length, then the function:

1. calls the Overflow() member function of the overflow handler, if an overflow
   handler is supplied
   
2  raises a USER 23 panic, if no overflow handler is supplied.
  
None of the generated text is appended and length of this descriptor
may be less than the maximum.

@param aFmt             The 8-bit non-modifiable descriptor containing the
                        format string. The TRefByValue class provides a
                        constructor which takes a TDesC8 type. 

@param aOverflowHandler A pointer to the overflow handler. 

@param ...              A variable number of arguments to be converted to text
                        as dictated by the format string. 

@panic USER 23  if the length of the descriptor exceeds its maximum length and
                no overflow handler has been supplied.
@panic USER 24  if the format string has incorrect syntax.

@see TDes8::Format()
@see TDes8Overflow::Overflow()
*/
	{

	VA_LIST list;
	VA_START(list, aOverflowHandler);
	// coverity[uninit_use_in_call]
	AppendFormatList(aFmt,list,aOverflowHandler);
	}

EXPORT_C void TDes8::AppendFormat(TRefByValue<const TDesC8> aFmt,...)
/**
Formats and appends text onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

The function takes a format string and a variable number of arguments.
The format string contains literal text, embedded with directives,
for converting the trailing list of arguments into text.

The embedded directives are character sequences prefixed with the '%' character.
The literal text is simply copied into this descriptor unaltered while
the '%' directives are used to convert successive arguments from the
trailing list. See the description of the Format() function.

Literal text is appended on a character by character basis.

Text converted from a trailing argument is appended as a complete string.

@param aFmt The 8-bit non-modifiable descriptor containing the
            format string. The TRefByValue class provides a
            constructor which takes a TDesC8 type. 

@param ...  A variable number of arguments to be converted to text
            as dictated by the format string. 


@panic USER 23  if the  resulting length of text in this descriptor exceeds
                the descriptor's maximum length.
@panic USER 24  if the format string has incorrect syntax.

@see TDes8::Format()
*/
	{

    VA_LIST list;
    VA_START(list,aFmt);
    AppendFormatList(aFmt,list);
    }
#endif	// __KERNEL_MODE__

#if !defined(__DES8_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TPtrC8::TPtrC8()
	: TDesC8(EPtrC,0),iPtr(0)
/**
Default constructor.

Constructs an empty 8-bit non-modifiable pointer descriptor.

It represents no data and its length is zero.

The non-modifiable pointer descriptor can, subsequently, be set to represent 
data.

@see TPtrC8::Set()
*/
	{}

EXPORT_C TPtrC8::TPtrC8(const TDesC8 &aDes)
	: TDesC8(EPtrC,aDes.Length()),iPtr(aDes.Ptr())
/**
Constructs the 8-bit non-modifiable pointer descriptor from any existing
descriptor.

It is set to point to the same data and is given the same length as the source 
descriptor.

@param aDes A reference to an 8bit non-modifiable descriptor.
*/
	{}

EXPORT_C TPtrC8::TPtrC8(const TUint8 *aString)
	: TDesC8(EPtrC,STRING_LENGTH(aString)),iPtr(aString)
/**
Constructs the 8-bit non-modifiable pointer descriptor to point to a zero
terminated string, whether in RAM or ROM.

The length of the descriptor is set to the length of the zero terminated
string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.
*/
	{}

EXPORT_C TPtrC8::TPtrC8(const TUint8 *aBuf,TInt aLength)
	: TDesC8(EPtrC,aLength),iPtr(aBuf)
/**
Constructs the 8-bit non-modifiable pointer descriptor to point to the
specified location in memory, whether in RAM or ROM.

The length of the descriptor is set to the specified length.

@param aBuf    A pointer to the location that the descriptor is to represent.
@param aLength The length of the descriptor. This value must be non-negative.

@panic USER 29  if aLength is negative.
*/
	{
	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes8LengthNegative));
	}

EXPORT_C TPtr8::TPtr8(TUint8 *aBuf,TInt aMaxLength)
	: TDes8(EPtr,0,aMaxLength),iPtr(aBuf)
/**
Constructs the 8-bit modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor is set to zero and its maximum length is set
to the specified value.

@param aBuf       A pointer to the location that the descriptor is to
                  represent.
                  
@param aMaxLength The maximum length of the descriptor.

@panic USER 30  if aMaxLength is negative.
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	}

EXPORT_C TPtr8::TPtr8(TUint8 *aBuf,TInt aLength,TInt aMaxLength)
	: TDes8(EPtr,aLength,aMaxLength),iPtr(aBuf)
/**
Constructs the 8-bit modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor and its maximum length are set to the specified
values.

@param aBuf       A pointer to the location that the descriptor is
                  to represent.
@param aLength    The length of the descriptor.
@param aMaxLength The maximum length of the descriptor.

@panic USER 20  if aLength is negative, or is greater than the descriptor's 
                maximum length,
@panic USER 30  if aMaxLength is negative.
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	__ASSERT_ALWAYS(TUint(aLength)<=TUint(aMaxLength),Panic(ETDes8LengthOutOfRange));
	}

EXPORT_C TPtr8::TPtr8(TBufCBase8 &aLcb,TInt aMaxLength)
	: TDes8(EBufCPtr,aLcb.Length(),aMaxLength),iPtr((TUint8*)&aLcb)
	{
	__ASSERT_DEBUG(aLcb.Length()<=aMaxLength,Panic(ETDes8LengthOutOfRange));
	}

EXPORT_C TBufBase8::TBufBase8(TInt aMaxLength)
	:TDes8(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	}

EXPORT_C TBufBase8::TBufBase8(TInt aLength,TInt aMaxLength)
	:TDes8(EBuf,aLength,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	__ASSERT_ALWAYS(TUint(aLength)<=TUint(aMaxLength),Panic(ETDes8LengthOutOfRange));
	}

EXPORT_C TBufBase8::TBufBase8(const TUint8* aString,TInt aMaxLength)
	:TDes8(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	Copy(aString);
	}

EXPORT_C TBufBase8::TBufBase8(const TDesC8& aDes,TInt aMaxLength)
	:TDes8(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes8MaxLengthNegative));
	Copy(aDes);
	}

#endif

// Truncate literal string to fit into descriptor
EXPORT_C void TDes8IgnoreOverflow::Overflow(TDes8& /*aDes*/)
	{}

#ifndef __KERNEL_MODE__
EXPORT_C void TDesC8::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	switch (Type())
		{
	case EBufC:
	case EPtrC:
	case EPtr:
	case EBuf:
	case EBufCPtr:
		break;
	default:
		User::Invariant();
		}
#endif
    }

EXPORT_C void TPtrC8::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC8::__DbgTestInvariant(); // Test base class
	if (Type()!=EPtrC)
		User::Invariant();
#endif
	}

EXPORT_C void TDes8::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC8::__DbgTestInvariant(); // Test base class
	if (Length()>MaxLength() || !(Type()==EPtr || Type()==EBufCPtr || Type()==EBuf))
		User::Invariant();
#endif
	}

EXPORT_C void HBufC8::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC8::__DbgTestInvariant(); // Test base class
	if (Length()>(TInt)(User::AllocLen(this)-sizeof(TDesC8)) || Type()!=EBufC)
		User::Invariant();
#endif
	}

EXPORT_C void TPtr8::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDes8::__DbgTestInvariant(); // Test base class
	if (!(Type()==EPtr || Type()==EBufCPtr))
		User::Invariant();
#endif
	}

/** Expand all characters from 8 to 16 bits

@return 16-bit pointer descriptor to transformed text

The length of descriptor increased by 2 (length *= 2).

@panic USER 187 if either the descriptor length or the maximum length is odd
or data pointer is not aligned by 2-bytes boundary
*/
EXPORT_C TPtr16 TDes8::Expand()
	{
	TInt l = Length();
	TInt ml = MaxLength();
	const TText8* s0 = Ptr();
	const TText8* s = s0 + l;
	__ASSERT_ALWAYS( !((ml|(TInt)s0)&1), Panic(EDes8ExpandOdd) );
	SetLength(l<<1);
	TText16* d = ((TText16*)s0) + l;
	while (s > s0)
		*--d = *--s;
	return TPtr16(d, l, ml>>1);
	}


/** Collapse all characters from 16 to 8 bits

The length of descriptor truncated by 2 (length /= 2).

@panic USER 188  if either the descriptor length or the maximum length is odd
or data pointer is not aligned by 2-bytes boundary.
*/

EXPORT_C void TDes8::Collapse()
	{
	TInt l = Length();
	TInt ml = MaxLength();
	TText8* d = (TText8*)Ptr();
	__ASSERT_ALWAYS( !((l|ml|(TInt)d)&1), Panic(EDes8CollapseOdd) );
	const TText16* s = (const TText16*)d;
	const TText16* sE = s + (l>>1);
	while (s < sE)
		*d++ = (TText8)*s++;
	SetLength(l>>1);
	}
#else // __KERNEL_MODE__

EXPORT_C TInt TDesC8::CompareF(const TDesC8 &aDes) const
/**
Compares this descriptor's folded data with the specified descriptor's folded 
data. 

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for comparing strings in natural language; 
use CompareC() for this.

@param aDes The 8-bit non modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
        
@see TDesC8::Compare()
*/
	{

	TInt ll = Length();
	TInt rl = aDes.Length();
	TInt r = memicmp(Ptr(), aDes.Ptr(), Min(ll, rl));
	if (r == 0)
		r = ll - rl;
	return r;
	}

#endif	// __KERNEL_MODE__

#ifndef __KERNEL_MODE__

/**
Default constructor.

Constructs a zero-length 8-bit resizable buffer descriptor.

Note that the object owns no allocated memory.
*/
EXPORT_C RBuf8::RBuf8()
	:TDes8(EPtr,0,0),iEPtrType(NULL)
	{
	// Zero-length RBuf8 is of type EPtr with NULL pointer.
	}




/**
Constructor.
			
Constructs an 8-bit resizable buffer descriptor, transferring ownership of the
specified heap descriptor to this object.

@param aHBuf The heap descriptor to be transferred to this object. This pointer
             can be NULL, which means that a zero length 8-bit resizable
             buffer	descriptor is constructed, and the object will not own any
             allocated memory.
*/
EXPORT_C RBuf8::RBuf8(HBufC8* aHBuf)
	{
	if(aHBuf)
		//Create EBufCPtr type descriptor that points to aHBuf
		new(this) TPtr8(aHBuf->Des());
	else
		//Create zero-length RBuf8. It is EPtr type of descriptor that points to NULL.
		new(this) RBuf8();
	}




/**
Protected constructor.
*/
EXPORT_C RBuf8::RBuf8(TInt aType,TInt aLength,TInt aMaxLength)
	:TDes8(aType,aLength,aMaxLength)
	{
	}




/**
Transfers ownership of the specified 8-bit resizable buffer descriptor's 
buffer to this object.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aRBuf The source 8-bit resizable buffer. The ownership of this
             object's buffer is to be transferred.

@see RBuf8::Close()
*/
EXPORT_C void RBuf8::Assign(const RBuf8& aRBuf)
	{
	Mem::Copy(this, &aRBuf, sizeof(RBuf8)); 
	__TEST_INVARIANT;
	}




/**
Assigns ownership of the specified allocated memory to this object.

The allocated memory forms the buffer for this descriptor. The current length
of the descriptor is set to zero.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aHeapCell  The allocated memory to be assigned to this object. This
                  pointer can be NULL, which means that a zero length 8-bit
                  resizable buffer descriptor is created.
@param aMaxLength The maximum length of the descriptor.

@panic USER 20 If the specified maximum length is greater then the size of
               the allocated heap cell, or the specified maximum length
               is NOT zero when the pointer to the heap cell is NULL.
              
@see TDesC8::Length()
@see TDes8::MaxLength()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::Assign(TUint8 *aHeapCell,TInt aMaxLength)
	{
	Assign(aHeapCell,0,aMaxLength);
	}




/**
Assigns ownership of the specified allocated memory to this object.

The allocated memory forms the buffer for this descriptor. The current length
of the descriptor is set to the value of the second parameter.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aHeapCell  The allocated memory to be assigned to this object.
@param aLength	  The length of the descriptor.
@param aMaxLength The maximum length of the descriptor.

@panic USER 20 If the specified maximum length is greater then the size of
               the allocated heap cell, or the specified length is greater then
               the specified maximum length, or the specified maximum length
               is NOT zero when the pointer to the heap cell is NULL.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::Assign(TUint8 *aHeapCell,TInt aLength,TInt aMaxLength)
	{
	__ASSERT_ALWAYS(aLength<=aMaxLength, Panic(ETDes8LengthOutOfRange));
	if(aHeapCell)
		{
		__ASSERT_ALWAYS(User::AllocLen(aHeapCell) >= aMaxLength * (TInt)sizeof(TUint8), Panic(ETDes8LengthOutOfRange));
		//Create EPtr type descriptor that points to aHeapCell
		new(this) TPtr8(aHeapCell,aLength,aMaxLength); 
		}
	else
		{
		__ASSERT_ALWAYS(aMaxLength == 0, Panic(ETDes8LengthOutOfRange));
		//Create zero-length RBuf. It is EPtr type of descriptor that points to NULL.
		new(this) RBuf8();
		}
	__TEST_INVARIANT;
	}




/**
Transfers ownership of the specified heap descriptor to this object.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aHBuf  The heap descriptor to be transferred to this object.
              This pointer can be NULL, which means that a zero length
              8-bit resizable buffer descriptor is created.
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::Assign(HBufC8* aHBuf)
	{
	new(this) RBuf8(aHBuf);
	}




/**
Swaps the content of two 8-bit resizable buffer descriptors.

@param aRBuf The 8-bit resizable buffer descriptor whose contents are to be
             swapped with this one.
*/
EXPORT_C void RBuf8::Swap(RBuf8& aRBuf)
	{
	Mem::Swap(this,&aRBuf,sizeof(*this));
	}




/**
Creates an 8-bit resizable buffer descriptor.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

The current length of the descriptor is set to zero. The maximum length of
the descriptor is set to the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength  The maximum length of the descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient	memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see RBuf8::Close()
*/
EXPORT_C TInt RBuf8::Create(TInt aMaxLength)
	{
	if (aMaxLength)
		{
		//Allocate memory
		TUint8* buf=(TUint8*)User::Alloc(aMaxLength*sizeof(TUint8));
		if(!buf) return KErrNoMemory;
		iEPtrType = buf;
		}
	else
		iEPtrType = NULL; //Zero-length descriptor.


	//Create EPtr type descriptor.
	new(this) RBuf8(EPtr,0,aMaxLength);
	__TEST_INVARIANT;
	return KErrNone;
	}




/**
Creates an 8-bit resizable buffer descriptor, and leaves on failure.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

The current length of the descriptor is set to zero. The maximum length of
the descriptor is set to the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength The length and the maximum length of the descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::CreateL(TInt aMaxLength)
	{
	User::LeaveIfError(Create(aMaxLength));
	}




/**
Creates an 8-bit resizable buffer descriptor.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

Both the current length and the maximum length of the descriptor are set to
the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength  The length and the maximum length of the descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see RBuf8::Close()
*/
EXPORT_C TInt RBuf8::CreateMax(TInt aMaxLength)
	{
	TInt err=Create(aMaxLength); 
	if(err==KErrNone)
		SetMax(); 
	return err;
	}




/**
Creates an 8-bit resizable buffer descriptor, and leaves on failure.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

Both the current length and the maximum length of the descriptor are set to
the specified value. 

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength The length and the maximum length of the descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::CreateMaxL(TInt aMaxLength)
	{
	User::LeaveIfError(CreateMax(aMaxLength));
	}




/**
Creates a 8-bit resizable buffer descriptor to contain a copy of the
specified (source) descriptor.

The function allocates sufficient memory so that this descriptor's maximum
length is the same as the length of the source descriptor. Both the current
length and the maximum length of this descriptor are set to
the length of the source descriptor.
				
The data contained in the source descriptor is copied into this
descriptor.

Note that the function assumes that this descriptor does not
already own any allocated memory. It does not check, nor does it free any
pre-existing owned allocated memory.  If this descriptor does already own 
allocated memory, RBuf8::Close() should be invoked on this descriptor before 
this function is invoked.

@param aDes Source descriptor to be copied into this object.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see TDes8::Copy()
@see RBuf8::Close()
*/
EXPORT_C TInt RBuf8::Create(const TDesC8& aDes)
	{
	return Create(aDes,aDes.Length());
	}




/**
Creates an 8-bit resizable buffer descriptor to contain a copy of the specified
(source) descriptor, and leaves on failure.
 
The function allocates sufficient memory so that this descriptor's maximum
length is the same as the length of the source descriptor.Both the current
length and the maximum length of this descriptor are set to the length
of the source descriptor.

The data contained in the source descriptor is copied into this descriptor.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any
pre-existing owned allocated memory.  If this descriptor does already own 
allocated memory, RBuf8::Close() should be invoked on this descriptor before 
this function is invoked.

@param aDes Source descriptor to be copied into this object.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see TDes8::Copy()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::CreateL(const TDesC8& aDes)
	{
	CreateL(aDes,aDes.Length());
	}




/**
Creates an 8-bit resizable buffer descriptor to contain a copy of the
specified (source) descriptor. 

The function allocates sufficient memory so that this descriptor's maximum length
is the same as the value of the aMaxLength parameter.

The data contained in the source descriptor is copied into this descriptor.
The length of data copied is either

- the length of the source descriptor aDes

or

- the value of the aMaxLength parameter

whichever is the smaller value. The current length of this descriptor is also
set to the smaller value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aDes Source descriptor to be copied into this object.
            
@param aMaxLength The maximum length of this descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see TDes8::Copy()
@see RBuf8::Close()
*/
EXPORT_C TInt RBuf8::Create(const TDesC8& aDes,TInt aMaxLength)
	{
	TInt err=Create(aMaxLength);
	if(err==KErrNone)
		Copy(aDes.Left(aMaxLength));
	return err;
	}




/**
Creates an 8-bit resizable buffer descriptor to contain a copy of the specified
(source) descriptor, and leaves on failure.

The function allocates sufficient memory so that this descriptor's maximum
length is the same as the value of the aMaxLength parameter.

The data contained in the source descriptor is copied into this descriptor.
The length of data copied is either

- the length of the source descriptor aDes

or

- the value of the aMaxLength parameter

whichever is the smaller value. The current length of this descriptor is also
set to the smaller value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf8::Close() should be invoked on this descriptor before this function is
invoked.

@param aDes Source descriptor to be copied into this object.
            
@param aMaxLength The maximum length of this descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC8::Length()
@see TDes8::MaxLength()
@see TDes8::Copy()
@see RBuf8::Close()
*/
EXPORT_C void RBuf8::CreateL(const TDesC8& aDes,TInt aMaxLength)
	{
	CreateL(aMaxLength);
	Copy(aDes.Left(aMaxLength));
	}




/**
Resizes this 8-bit resizable buffer descriptor.

The length and contents of the descriptor are unchanged.

If the buffer descriptor was created from a zero-length heap descriptor
HBufC, this method might leak memory (the heap descriptor is not freed).
It is possible to avoid this by calling the Close() method prior to ReAlloc(),
but this should be done only in this situation (otherwise the buffer contents
will be lost).

For example, add
@code
    if (desc.MaxLength() == 0) desc.Close();
@endcode
before the call to ReAlloc().

@param aMaxLength The new maximum length of the descriptor. This can be zero,
                  which results in a descriptor with zero maximum length and no
                  allocated memory.
                  
@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@panic USER 26 If the new maximum length is less then the current descriptor length.
*/
EXPORT_C TInt RBuf8::ReAlloc(TInt aMaxLength)
	{
	__ASSERT_ALWAYS(Length()<=aMaxLength, Panic(ETDes8ReAllocTooSmall));
	__TEST_INVARIANT;

	if (!aMaxLength)				//Reallocation to zero length
		{
		User::Free(iEPtrType);	//Free memory 
		new (this) RBuf8();			//Create zero-length RBuf
		return KErrNone;
		}

	if (!iMaxLength)				//Reallocation from zero length
		return Create(aMaxLength);

	switch(Type())
		{
		case EPtr:
			{
			TUint8* buf = (TUint8*)User::ReAlloc(iEPtrType,aMaxLength*sizeof(TUint8));
			if(!buf) return KErrNoMemory;
			iEPtrType = buf;
			iMaxLength = aMaxLength;
			break;
			}
		case EBufCPtr:
			{
			HBufC8* hbufc = iEBufCPtrType->ReAlloc(aMaxLength);
			if(!hbufc) return KErrNoMemory;
			Assign(hbufc);
			break;
			}
		}

	__TEST_INVARIANT;
	return KErrNone;
	}




/**
Resizes this 8-bit resizable buffer descriptor, leaving on failure.

The length and contents of the descriptor are unchanged.

If the buffer descriptor was created from a zero-length heap descriptor
HBufC, this method might leak memory (the heap descriptor is not freed).
It is possible to avoid this by calling the Close() method prior to ReAllocL(),
but this should be done only in this situation (otherwise the buffer contents
will be lost).

For example, add
@code
    if (desc.MaxLength() == 0) desc.Close();
@endcode
before the call to ReAlloc().

@param aMaxLength The new maximum length of the descriptor. This can be zero,
                  which results in a descriptor with zero maximum length and no
                  allocated memory.
                  
@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@panic USER 26 If the new maximum length is less then the current descriptor length.
*/
EXPORT_C void RBuf8::ReAllocL(TInt aMaxLength)
	{
	User::LeaveIfError(ReAlloc(aMaxLength));
	}




/**
Deallocates memory assigned to this object, and re-initializes the object as
a zero-length descriptor.
*/
EXPORT_C void RBuf8::Close() 
	{
	User::Free(iEPtrType); 
	//Create zero-length RBuf. It is EPtr type of descriptor that points to NULL.
	new(this) RBuf8();
	}




/**
Pushes a cleanup item for this object onto the cleanup stack.

The effect of this is to cause Close() to be called on this 8-bit resizable
buffer descriptor, when CleanupStack::PopAndDestroy() is called at some later time.

@code
...
RBuf8 x;
....
x.CleanupClosePushL();
...
CleanupStack::PopAndDestroy();
...
@endcode

@see RBuf8::Close()
*/
EXPORT_C void RBuf8::CleanupClosePushL()
	{
	::CleanupClosePushL(*this);
	}




/**
Tests that the class obeys its invariant.
*/
EXPORT_C void RBuf8::__DbgTestInvariant() const
	{
#ifdef _DEBUG
	TDes8::__DbgTestInvariant();
	switch(Type())
		{
	case EPtr:
		if (iEPtrType)
			{
			__ASSERT_DEBUG(User::AllocLen(iEPtrType) >= iMaxLength*(TInt)sizeof(TUint8), Panic(EInvariantFalse));
			}
		break;
	case EBufCPtr:
		iEBufCPtrType->__DbgTestInvariant(); 
		__ASSERT_DEBUG(iEBufCPtrType->Des().MaxLength() == iMaxLength, Panic(EInvariantFalse));
		__ASSERT_DEBUG(iEBufCPtrType->Length() == Length(), Panic(EInvariantFalse));
		break;
	default:
		User::Invariant();
		}
#endif // _DEBUG
	}

#endif	// __KERNEL_MODE__


#if defined(__DES8_MACHINE_CODED__) || defined(__EABI__)
GLDEF_C void Des8PanicBadDesType()
	{
	Panic(ETDes8BadDescriptorType);
	}

GLDEF_C void Des8PanicPosOutOfRange()
	{
	Panic(ETDes8PosOutOfRange);
	}
#endif

#ifdef __DES8_MACHINE_CODED__
GLDEF_C void Des8PanicLengthNegative()
	{
	Panic(ETDes8LengthNegative);
	}

GLDEF_C void Des8PanicMaxLengthNegative()
	{
	Panic(ETDes8MaxLengthNegative);
	}

GLDEF_C void Des8PanicLengthOutOfRange()
	{
	Panic(ETDes8LengthOutOfRange);
	}

GLDEF_C void Des8PanicDesOverflow()
	{
	Panic(ETDes8Overflow);
	}

GLDEF_C void Des8PanicDesIndexOutOfRange()
	{
	Panic(ETDes8IndexOutOfRange);
	}
#endif

