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
// e32\euser\us_func.cpp
// 
//

#include "us_std.h"
//#define __DEBUG_IMAGE__ 1
#if defined(__DEBUG_IMAGE__) && defined (__EPOC32__)
#include "e32svr.h" 
#define __IF_DEBUG(t) {RDebug debug;debug.t;}
#else
#define __IF_DEBUG(t)
#endif
#ifdef _UNICODE
#include <collate.h>
#include "CompareImp.h"
#endif

#include "us_data.h"

typedef union
	{
	TAny *tany;
	TText8 *ttext8;
	TText16 *ttext16;
	TDesC8 *tdesc8;
	TDesC16 *tdesc16;
	TInt8 *tint8;
	TInt16 *tint16;
	TInt32 *tint32;
	TInt64 *tint64;
	TInt *tint;
	TUint8 *tuint8;
	TUint16 *tuint16;
	TUint32 *tuint32;
	TUint *tuint;
	} UPTR;

const TUint crcTab[256] =
    {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,
	0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,
	0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,0x2462,
	0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,
	0xe5ee,0xf5cf,0xc5ac,0xd58d,0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,
	0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,0x48c4,0x58e5,
	0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,
	0x9969,0xa90a,0xb92b,0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,0x6ca6,0x7c87,0x4ce4,
	0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,
	0x8d68,0x9d49,0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,
	0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,0x9188,0x81a9,0xb1ca,0xa1eb,
	0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,
	0x6067,0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,
	0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,
	0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,
	0x16b0,0x6657,0x7676,0x4615,0x5634,0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,
	0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,0xcb7d,
	0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,
	0x0af1,0x1ad0,0x2ab3,0x3a92,0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,
	0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,0xef1f,0xff3e,
	0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,
	0x3eb2,0x0ed1,0x1ef0
    };

const TInt KQDepth=150; // Maximum queue depth

LOCAL_C TUint checkSum(const TAny *aPtr)
//
// Checksum every other byte
//
	{

	const TUint8* pB=(const TUint8*)aPtr;
	const TUint8* pE=pB+(KMaxCheckedUid*sizeof(TUid));
	TUint8 buf[(KMaxCheckedUid*sizeof(TUid))>>1];
	TUint8* pT=(&buf[0]);
	while (pB<pE)
		{
		*pT++=(*pB);
		pB+=2;
		}
	TUint16 crc=0;
	Mem::Crc(crc,&buf[0],(KMaxCheckedUid*sizeof(TUid))>>1);
	return(crc);
	}
	
LOCAL_C TInt partit(TInt n,TInt m,const TKey &aKey,const TSwap &aSwap)
//
// Partition n elements of array stating from element m, 
// return element no. of partition point. 
//
	{

	TInt i=m-1;
	TInt j=i+n;
	TInt pivlin=j;
	aSwap.Swap((j+m)>>1,j);
	while (i<j) 
		{
		for (i++;aKey.Compare(i,pivlin)<0;i++)
			{
			}
		for (--j;j>i;j--)
			{
			if (aKey.Compare(j,pivlin)<=0)
				break;
			}
		if (i<j)
			aSwap.Swap(i,j);
		}
	aSwap.Swap(i,m+n-1);
	return(i);
	}

static const TCollationMethod TheCollationMethod[] =
	{
		{
		KUidBasicCollationMethod,				// this is the standard unlocalised method
		NULL,									// null means use the standard table
		NULL,									// there's no override table
		0										// the flags are standard
		}
	};

static const TCollationDataSet TheCollationDataSet =
	{
	TheCollationMethod,
	1
	};

// The one and only locale character set object.
const LCharSet TheCharSet =
	{
	NULL,
	&TheCollationDataSet
	};

const LCharSet* GetLocaleDefaultCharSet()
	{
	return &TheCharSet;
	}

const LCharSet* GetLocaleCharSet()
	{
	const LCharSet* charSet = (const LCharSet*)Exec::GetGlobalUserData(ELocaleDefaultCharSet);
	if(charSet)
		return charSet;
	return &TheCharSet;
	}

const LCharSet* GetLocalePreferredCharSet()
	{
	const LCharSet* charSet = (const LCharSet*)Exec::GetGlobalUserData(ELocalePreferredCharSet);
	if(charSet)
		return charSet;
	return &TheCharSet;
	}

EXPORT_C TCollationMethod TExtendedLocale::GetPreferredCollationMethod(TInt aIndex)
/**
Get the preferred collation method for the preferred charset.

Note that some charsets may contain more than one collation
method (e.g "dictionary" v "phonebook" ordering) so an optional
index parameter can be used to select between them.

@param aIndex Optional parameter specifying the index of the collation 
              method in the locale to get. This is the responsibility of 
			  the caller to make sure that the index is less than the total 
			  number of collation methods in the preferred charset in this locale.

@return TCollationMethod representing the requested collation method.

@panic USER 185 In both debug and release builds, if either the current charset 
       is not set up or aIndex is greater than or equal to the total number of 
	   collation methods in the preferred charset in this locale.

@see TCollationMethod
*/
	{
	__ASSERT_ALWAYS(iPreferredCharSet && (TUint(aIndex)<=TUint(iPreferredCharSet->iCollationDataSet->iMethods)), Panic(EBadLocaleParameter));
	return iPreferredCharSet->iCollationDataSet->iMethod[aIndex];
	}

extern const TUint8 __FoldCollTab8[256];

EXPORT_C TInt Mem::CompareF(const TUint8 *aLeft,TInt aLeftL,const TUint8 *aRight,TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location, using the standard folding method appropriate 
to the current locale.

@param aLeft   A pointer to the first (or left) block of 8 bit data to be
               compared.
@param aLeftL  The length of the first (or left) block of data to be compared, 
               i.e. the number of bytes.
@param aRight  A pointer to the second (or right) block of 8 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be
               compared, i.e. the number of bytes.

@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the second
        (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content. 
        
@see Mem::Compare
*/
	{

	__ASSERT_DEBUG(aLeftL>=0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL>=0,Panic(EMemRightNegative));
	const TUint8 *pE=aLeft+Min(aLeftL,aRightL);
	const TUint8* table=__FoldCollTab8;
    while (aLeft<pE)
		{
		TUint l=*aLeft++;
		TUint r=*aRight++;
		l = table[l];
		r = table[r];
		TInt d=l-r;
		if (d!=0)
		    return(d);
		}
    return(aLeftL-aRightL);
	}




EXPORT_C TInt Mem::CompareC(const TUint8 *aLeft,TInt aLeftL,const TUint8 *aRight,TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location using the standard collation method appropriate 
to the current locale.

@param aLeft   A pointer to the first (or left) block of 8 bit data to be
               compared.
@param aLeftL  The length of the first (or left) block of data to be compared 
               i.e. the number of bytes.
@param aRight  A pointer to the second (or right) block of 8 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e. the number of bytes.

@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the second
        (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content.
        
@see Mem::Compare
@deprecated
*/
	{
	return Mem::CompareF(aLeft, aLeftL, aRight, aRightL);
	}




EXPORT_C TInt Mem::Compare(const TUint16 *aLeft,TInt aLeftL,const TUint16 *aRight,TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location.

The comparison proceeds on a double-byte for double-byte basis, the result 
of the comparison is based on the difference of the first pair of bytes to 
disagree.

The data at the two locations are equal if they have the same length and content. 
Where the lengths are different and the shorter section of data is the same 
as the first part of the longer section of data, the shorter is considered 
to be less than the longer.

@param aLeft   A pointer to the first (or left) block of 16 bit data to be
               compared.
@param aLeftL  The length of the first (or left) block of data to be compared 
               i.e. the number of double-bytes. 
@param aRight  A pointer to the second (or right) block of 16 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e. the number of double-bytes.

@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the second
        (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content.
*/
	{

	__ASSERT_DEBUG(aLeftL>=0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL>=0,Panic(EMemRightNegative));
	const TUint16 *pE=aLeft+Min(aLeftL,aRightL);
    while (aLeft<pE)
		{
		TInt d=(*aLeft++)-(*aRight++);
		if (d!=0)
		    return(d);
		}
    return(aLeftL-aRightL);
	}




EXPORT_C TInt Mem::CompareF(const TUint16 *aLeft,TInt aLeftL,const TUint16 *aRight,TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location, using the standard folding method appropriate 
to the current locale.

@param aLeft   A pointer to the first (or left) block of 16 bit data to be
               compared.
@param aLeftL  The length of the first (or left) block of data to be compared 
               i.e. the number of double-bytes.
@param aRight  A pointer to the second (or right) block of 16 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e the number of double-bytes.

@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the second
        (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content.
        
@see Mem::Compare
*/
	{
	__ASSERT_DEBUG(aLeftL >= 0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL >= 0,Panic(EMemRightNegative));

	const TText16* aLeftEnd = aLeft + aLeftL;
	const TText16* aRightEnd = aRight + aRightL;
	
	const TUint8* table=__FoldCollTab8;

	while (aLeft != aLeftEnd)
		{
		if (aRight == aRightEnd)
			return 1;

		TUint l = *aLeft;
		TUint r = *aRight;

		// check if character is Ascii, if so treat as Ascii
		if (l < 128 && r < 128)
			{
			l = table[l];
			r = table[r];

			if (r != l)
				return l-r;
			
			aLeft++;
			aRight++;
			}
		// covers Unicode characters...
		else
			{
			TUTF32Iterator leftIt(aLeft, aLeftEnd);
			TUTF32Iterator rightIt(aRight, aRightEnd);
			return ::CompareFolded(leftIt, rightIt);
			}
		}

	return aRight == aRightEnd? 0:-1;
	}
	




EXPORT_C TInt Mem::CompareC(const TUint16 *aLeft,TInt aLeftL,const TUint16 *aRight,TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location using the standard collation method appropriate 
to the current locale.

@param aLeft   A pointer to the first (or left) block of 16 bit data to be
               compared.
@param aLeftL  The length of the first (or left) block of data to be compared 
               i.e. the number of double-bytes).
@param aRight  A pointer to the second (or right) block of 16 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e. the number of double-bytes. 
               
@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the second
        (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content. 
        
@see Mem::Compare
*/
	{
	__ASSERT_DEBUG(aLeftL>=0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL>=0,Panic(EMemRightNegative));
#ifdef _UNICODE
	TCollate c(GetLocaleCharSet());
	return c.Compare(aLeft,aLeftL,aRight,aRightL);
#else
	const TUint16 *pE=aLeft+Min(aLeftL,aRightL);
    while (aLeft<pE)
		{
		TInt d=User::Collate(*aLeft++)-User::Collate(*aRight++);
		if (d!=0)
		    return(d);
		}
    return(aLeftL-aRightL);
#endif
	}




#ifdef _UNICODE
EXPORT_C TInt Mem::CompareC(const TUint16* aLeft,TInt aLeftL,const TUint16* aRight,TInt aRightL,
							TInt aMaxLevel,const TCollationMethod* aCollationMethod)
/**
Compares a block of data at one location with a block of data at another location 
using the specified collation method and collating to the specified maximum 
collation level.

If no collation method is supplied, a default method, appropriate to the current 
locale, is used.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an explicit 
16 bit descriptor is used.

@param aLeft            A pointer to the first (or left) block of 16 bit data
                        to be compared.
@param aLeftL           The length of the first (or left) block of data to be
                        compared. This is the number of double-bytes. 
@param aRight           A pointer to the second (or right) block of 16 bit data
                        to be compared.
@param aRightL          The length of the second (or right) block of data to be
                        compared. This is the number of double-bytes. 
@param aMaxLevel        The maximum collation level. 
@param aCollationMethod A pointer to the collation method or NULL. 

@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and their contents
        are the same.
*/
	{
	__ASSERT_DEBUG(aLeftL>=0,Panic(EMemLeftNegative));
	__ASSERT_DEBUG(aRightL>=0,Panic(EMemRightNegative));
	if (aCollationMethod == NULL)
		{
		TCollate c(GetLocaleCharSet());
		return c.Compare(aLeft,aLeftL,aRight,aRightL,aMaxLevel);
		}
	else
		{
		TCollate c(*aCollationMethod);
		return c.Compare(aLeft,aLeftL,aRight,aRightL,aMaxLevel);
		}
	}
#endif




#ifdef _UNICODE
EXPORT_C TInt Mem::CollationMethods()
/**
Gets the number of collation methods in this locale.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an
explicit 16 bit descriptor is used.

@return The number of collation methods.
*/
	{
	return GetLocaleCharSet()->iCollationDataSet->iMethods;
	}
#endif




#ifdef _UNICODE
EXPORT_C TUint Mem::CollationMethodId(TInt aIndex)
/**
Gets the Uid associated with the specified collation method.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an
explicit 16 bit descriptor is used.

@param aIndex An index into the set of collation methods in thie locale. This 
              value is relative to zero; i.e. a zero value refers to the first
              collation method. This value must not be negative, and must be
              less than the total number of collation methods in this locale.

@return The Uid of the collation method.

@panic USER 132 In debug builds only, if aIndex is negative or is greater than
       or equal to the total number of collation methods in this locale.
*/
	{
	const TCollationDataSet* s = GetLocaleCharSet()->iCollationDataSet;
	__ASSERT_DEBUG(aIndex >= 0 && aIndex < s->iMethods,Panic(EBadCollationRulesIndex));
	return s->iMethod[aIndex].iId;
	}
#endif




#ifdef _UNICODE
EXPORT_C const TCollationMethod* Mem::CollationMethodByIndex(TInt aIndex)
/**
Gets the collation method identified by the specified index.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an
explicit 16 bit descriptor is used.

@param aIndex An index into the set of collation methods in this locale. This 
              value is relative to zero; i.e. a zero value refers to the first
              collation method. This value must not be negative, and must be
              less than the total number of collation methods in this locale.
              
@return A pointer to the collation method.

@panic USER 132 In debug builds only, if aIndex is negative or is greater than
       or equal to the total number of collation methods in this locale.
*/
	{
	const TCollationDataSet* s = GetLocaleCharSet()->iCollationDataSet;
	__ASSERT_DEBUG(aIndex >= 0 && aIndex < s->iMethods,Panic(EBadCollationRulesIndex));
	return &s->iMethod[aIndex];
	}
#endif




#ifdef _UNICODE
EXPORT_C const TCollationMethod* Mem::CollationMethodById(TUint aId)
/**
Gets the collation method identified by the specified Uid.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an
explicit 16 bit descriptor is used.

@param aId The Uid of a collation method in the set of collation methods in 
           this locale. 
           
@return A pointer to the collation method.
*/
	{
	const TCollationDataSet* set = GetLocaleCharSet()->iCollationDataSet;
	const TCollationMethod* method = set->iMethod;
	const TCollationMethod* m = method;
	int methods = set->iMethods;
	for (int i = 0; i < methods; i++, m++)
		if (m->iId == aId)
			{
			method = m;
			break;
			}
	return method;
	}
#endif

#ifdef _UNICODE
EXPORT_C const TCollationMethod* Mem::GetDefaultMatchingTable()
/**
Gets the collation method specific for matching purpose.

This function is only defined for 16 bit (Unicode) build variants. This means 
that the function is not defined for 8 bit build variants, even when an
explicit 16 bit descriptor is used.

@return A pointer to the collation method
*/
	{
	const TCollationDataSet* set=GetLocaleCharSet()->iCollationDataSet;
	const TCollationMethod* method=set->iMethod;
	const TCollationMethod* m= method;
	int methods = set->iMethods;
	for (; methods-->0; m++)
		if (m->iFlags & TCollationMethod::EMatchingTable)
			{
			method=m;
			break;
			}
	return method;
	}
#endif


#if !defined(__MEM_MACHINE_CODED__)
EXPORT_C void Mem::Swap(TAny *aPtr1,TAny *aPtr2,TInt aLength)
/**
Swaps a number of bytes of data between two specified locations.

The source and target areas can overlap.

@param aPtr1   A pointer to the first location taking part in the swap. 
@param aPtr2   A pointer to second location taking part in the swap. 
@param aLength The number of bytes to be swapped between the two locations. 
               This value must not be negative.

@panic USER 94 In debug builds only, if aLength is negative.
*/
	{

	__ASSERT_DEBUG(aLength>=0,Panic(EMemSwapLengthNegative));
	if (aPtr1==aPtr2)
		return;
	TUint8 *pT=(TUint8 *)aPtr1;
	TUint8 *pE=pT+aLength;
	TUint8 *pS=(TUint8 *)aPtr2;
	while (pT<pE)
		{
		TUint b=(*pT);
		*pT++=(*pS);
		*pS++=(TUint8)b;
		}
	}
#endif




EXPORT_C void Mem::Crc(TUint16& aCrc,const TAny* aPtr,TInt aLength)
/**
Performs a CCITT CRC checksum on the specified data.

On return from this function, the referenced 16 bit integer contains the checksummed 
value.

@param aCrc    A reference to a 16 bit integer to contain the checksummed value. 
@param aPtr    A pointer to the start of the data to be checksummed. 
@param aLength The length of the data to be checksummed.
*/
	{

	const TUint8* pB=(const TUint8*)aPtr;
	const TUint8* pE=pB+aLength;
	TUint crc=aCrc;
    while (pB<pE)
		crc=(crc<<8)^crcTab[((crc>>8)^*pB++)&0xff];
	aCrc=(TUint16)crc;
	}




EXPORT_C TInt User::StringLength(const TUint8 *aString)
/**
Gets the length of a C style, null terminated, string of single-byte valued 
characters.

The length does not include the null terminator.

@param aString A pointer to the single byte valued, null terminated, string.

@return The length of the string.
*/
	{

	const TUint8 *pS=aString;
	while (*pS)
		pS++;
	return(pS-aString);
	}




EXPORT_C TInt User::StringLength(const TUint16 *aString)
/**
Gets the length of a C style, null terminated, string of double-byte valued 
characters.

The length does not include the null terminator.

@param aString A pointer to the double-byte valued, null terminated, string.

@return The length of the string.
*/
	{

	const TUint16 *pS=aString;
	while (*pS)
		pS++;
	return(pS-aString);
	}




EXPORT_C void User::Panic(const TDesC &aCategory,TInt aReason)
/**
Panics the current thread, specifying a category name and panic number.

Keep the length of the category name small; it is limited to 16 characters.

@param aCategory A reference to the descriptor containing the text that defines 
                 the category for this panic.
@param aReason   The panic number.
*/
	{

	__IF_DEBUG(Print(_L("User::Panic %S %d\n"),&aCategory,aReason));
	TPtrC cat16(aCategory.Ptr(),Min(KMaxExitCategoryName,aCategory.Length()));
	TBuf8<KMaxExitCategoryName> cat;
	cat.Copy(cat16);
	ExitCurrentThread(EExitPanic,aReason,&cat);
	}




TBool TEntryPointList::AlreadyCalled(TLinAddr aEP)
	{
	TInt i;

	// if we find it in the list before the current one
	// then it was already called, return true
	for (i=0; i<iCurrentEP; ++i)
		if (iEPs[i] == aEP)
			return ETrue;

	// if it *is* the current one (i==iCurrentEP here)
	// then we're in some kind of hideous cycle. There is no
	// way to resolve this that isn't wrong, but some people
	// may depend on it anyway. The safest thing to do is to
	// just claim the EP is already called and hope that
	// the constructors aren't actually dependent on each other.
	if (iEPs[i] == aEP)
		return ETrue;
		
	// if we find it *after* the current one then it's not
	// already been called, but we need to call it now
	// rather than when we get that far back up the stack,
	// so set that copy to -1.
	for (++i; i<iNumEPs; ++i)
		if (iEPs[i] == aEP)
			iEPs[i] = 0xFFFFFFFFU;

	// if this is not the top of the stack recurse, otherwise
	// return false so the current load will call it
	if (iPrevList)
		return iPrevList->AlreadyCalled(aEP);
	else
		return EFalse;
	}


TInt TEntryPointList::CallEPs()
	{
	// The TLS entry for KNestedEntryPointCallKey is the head
	// of a linked list of TEntryPointList objects. Add this one
	// to the front of the list. iPrevList will end up NULL if
	// there is no nested load yet.
	// Only one thread per process can be doing a load at a time
	// because of the DLL lock, so using TLS for this is fine.
	iPrevList=(TEntryPointList*)UserSvr::DllTls(KNestedEntryPointCallKey, KDllUid_Special);
	TInt r=UserSvr::DllSetTls(KNestedEntryPointCallKey, KDllUid_Special, this);
	if (r != KErrNone)
		return r;

	// call each entry point unless it's already been called higher
	// in the nested loading process, or it's been set to -1 because
	// it was called lower in the nested load.
	for (iCurrentEP=0; iCurrentEP<iNumEPs; ++iCurrentEP)
		{
		TLinAddr ep=iEPs[iCurrentEP];
		if (ep != 0xFFFFFFFFU && (!iPrevList || !iPrevList->AlreadyCalled(ep)))
			{
			TLibraryEntry f=(TLibraryEntry)ep;
			r = (*f)(KModuleEntryReasonProcessAttach);
			if (r != KErrNone)
				break;
			}
		}

	// Take this object off the list before returning
	UserSvr::DllSetTls(KNestedEntryPointCallKey, KDllUid_Special, iPrevList);

	return r;
	}




void CallStaticEntryPoints(TBool aInit)
	{
	TEntryPointList eplist;
	eplist.iNumEPs=KMaxLibraryEntryPoints;
	TInt r=E32Loader::StaticCallList(eplist.iNumEPs, eplist.iEPs);
	if (r!=KErrNone)
		return;
	eplist.iNumEPs -= 1; // last EP is always process entry point
	if (aInit)
		{
		eplist.CallEPs();
		}
	else
		{
		for (TInt i=eplist.iNumEPs-1; i>=0; --i)
			{
			TLibraryEntry f=(TLibraryEntry)eplist.iEPs[i];
			(*f)(KModuleEntryReasonProcessDetach);
			}
		}
	}




EXPORT_C void User::InitProcess()
/**
@internalAll
*/
	{
	CallStaticEntryPoints(ETrue);
	E32Loader::StaticCallsDone();
	}




EXPORT_C void User::Exit(TInt aReason)
/**
Terminates the current thread, specifying a reason.
All child threads are terminated and all resources are cleaned up.

If the current thread is the main thread in a process, the process is also 
terminated.

@param aReason The reason code.
*/
	{
	// Notify kernel that thread is exiting
	TBool lastThread = Exec::UserThreadExiting(aReason);
	if (lastThread)
		{
		// Call global destructors if we're the last thread in the process
		TGlobalDestructorFunc func = (TGlobalDestructorFunc)UserSvr::DllTls(KGlobalDestructorTlsKey, KDllUid_Special);
		if (func)
			{
			func();
			CallStaticEntryPoints(EFalse);
			}
		}
	
	FOREVER
		{
		TInt h=Exec::LastThreadHandle();
		if (h==0)
			break;
		if (Exec::HandleClose(h)>0)
			RHandleBase::DoExtendedClose();
		}

#ifdef __USERSIDE_THREAD_DATA__
	LocalThreadData()->Close();
#endif
	
	ExitCurrentThread(EExitKill,aReason,NULL);
	}




EXPORT_C TInt User::QuickSort(TInt aCount,const TKey &aKey,const TSwap &aSwap)
//
// Routine sorts a set of records into the order defined by the key aKey.
// There are aCount records to sort, each record is numbered, the first is
// record 0, the last record aCount-1.
// Each time the quicksort algorithm needs to compare two records it calls
//     aKey.Compare(TInt n,TInt m).
// where n and m (both type TUint) are the record no.s of the two records to compare.
// The compare routine should return
//     >0 if record(n) > record(m)
//      0 if record(n) == record(m)
//     <0 if record(n) < record(m)
// Each time the quicksort algorithm needs to exchange two records it calls
//     aSwap.Swap(n,m)
// where n and m (both type TUint) are the record numbers of the two records to 
// exchange.
// The swap routine should exchange the positions of records n and m so that
// the value of record m becomes the former value of record n and visa versa.
//
/**
Quick sorts array elements.

It is used by the standard Symbian OS arrays having 
CArrayFixBase, CArrayVarBase or CArrayPakBase in their class hierarchy in 
the implementation of their sort functions. The function can be used by other 
arrays. 

The function returns KErrNone if the operation is successful otherwise it 
returns KErrGeneral.

@param aCount The number of elements in the array.
@param aKey   A reference to a suitably initialised TKey derived object.
@param aSwap  A reference to a suitably initialised TSwap derived object.

@return KErrNone if the operation is successful; KErrGeneral otherwise.

@panic USER 96, if aCount is negative.
*/
	{
	TInt *parts_sp; // Stack pointer for partitions todo
	TInt m; // First element of partition
	TInt n; // No. of elements in partition
	TInt d1,d2; // Temporary variables
	TInt division_point; // Seperation point of partitions
	TInt parts_todo[KQDepth]; // Stack pairs are <n,base>

	__ASSERT_ALWAYS(aCount>=0,::Panic(ESortCountNegative));
	if (aCount<=1) 
		return(KErrNone); // Discard trivial sorts
	parts_sp=(&parts_todo[0]); // Reset partitions to do stack
	m=0; // Initial limits, first element
	n=aCount; // No_elm elements to do
	do  {
		while (n>1 && parts_sp<(&parts_todo[KQDepth-2]))
			{
			division_point=partit(n,m,aKey,aSwap);
			d1=division_point-m;
			d2=m+n-division_point-1;
			if (d1<d2)
				{
				// Less elements in first partition, do it first
				// Stack bigger partition for later
				*(parts_sp++)=d2;
				*(parts_sp++)=division_point+1;
				n=d1;
				}
			else
				{
				// Less elements in second partition,do it first
				// Stack bigger partition for later
				*(parts_sp++)=d1;
				*(parts_sp++)=m;                      
				n=d2;
				m=division_point+1;
				}
			}
		if (parts_sp>=&parts_todo[KQDepth-2]) 
			return(KErrGeneral); // Stack overflow
		m=(*(--parts_sp));
		n=(*(--parts_sp)); // Unstack next partit to do
		} while (parts_sp>=(&parts_todo[0])); // Stop on stack underflow
	return(KErrNone);
	}




EXPORT_C TInt User::BinarySearch(TInt aCount,const TKey &aKey,TInt &aPos)
//
// Perform a binary search on any array. aKey.Compare() will be
// called to lexically compare a record in the array with the
// value being searched for. The second index to aKey.Compare() will
// always be KIndexPtr, meaning the value being compared. The result
// returned will be 0 if a match is found and >0 if no match is found.
// The index of the matching record or the index of the record logically
// following the value being searched for will be returned in aPos.
//
/**
Performs a binary search for an array element containing a specified key.

It can be used on any kind of array where elements can be identified by key. 
It is used by the standard Symbian OS arrays having CArrayFix, CArrayVar or 
CArrayPak in their class hierarchy in the implementation of the various
functions for inserting, deleting and finding elements by key. The function
can be used by other arrays. 

The function returns a zero value if the search is successful and a non-zero 
value otherwise.

If the search is successful, the function puts the position (i.e. the index) 
of the element into aPos. If the search is unsuccessful, then the function 
puts into aPos the position of the first element in the array whose key is 
greater than the search key.

If the array is empty, i.e. aCount is zero, then the search is unsuccessful 
and aPos is not defined.

@param aCount The number of elements in the array.
@param aKey   A reference to a suitably initialised TKey derived object.
              In particular, the object will have been initialised with
              a pointer to a sample element containing the search key.
@param aPos   If the element is found, the reference is set to the position of 
              that element within the array. The position is relative to zero,
              (i.e. the first element in the array is at position 0).
              If the element is not found and the array is not empty, then
              the reference is set to the position of the first element in
              the array with a key which is greater than the search key. 
              If the element is not found and the array is empty, then the
              reference is undefined.

@return Zero, if the element with the specified key is found. Non-zero, if 
the element with the specified key is not found.

@panic USER 97, if aCount is negative.
*/
	{

	__ASSERT_ALWAYS(aCount>=0,::Panic(EBinarySearchCountNegative));
    TInt mid=0;
    TInt r=(-1);
    if (aCount)
        {
        TInt low=0;
        TInt high=aCount-1;
        while (low<=high)
            {
            mid=(low+high)>>1;
            if ((r=aKey.Compare(mid,KIndexPtr))==0)
                break;
            if (r<0)
                low=mid+1;
            else
                high=mid-1;
            }
        }
	if (r<0)
		mid++;
    aPos=mid;
    return(r);
	}




EXPORT_C TVersion User::Version()
/**
Retrieves the E32 component version number, which is the kernel architecture version number.  
For example for EKA2 the major part of the version number will be 2.

@return  The E32 component version number.
*/
	{

	return(TVersion(KE32MajorVersionNumber,KE32MinorVersionNumber,KE32BuildVersionNumber));
	}




EXPORT_C void User::Invariant()
/**
Panics the current thread with a USER 0 panic.

Typically, this is called when a test for a class invariant fails, i.e. when
a test which checks that the internal data of an object is
self-consistent, fails.

Such tests are almost always done in debug builds, commonly using
the __ASSERT_DEBUG macro.
*/
	{

	::Panic(EInvariantFalse);
	}




EXPORT_C TBool User::QueryVersionSupported(const TVersion &aCurrent,const TVersion &aRequested)
/**
Compares two version objects and returns true if the test version is less 
than the current version.

Version information is encapsulated by a TVersion type object and consists
of a major version number, a minor version number and a build number.

The function returns true if one of the following conditions is true:

1. the test major version is strictly less than the current major version

2. the test major version is equal to the current major version and the test 
   minor version is less than or equal to the current minor version

If neither condition is true, the function returns false.

@param aCurrent   A reference to the current version against which aRequested 
                  is compared.
@param aRequested A reference to the test version to be compared
                  against aCurrent.
                  
@return True, if one or both conditions are true. False otherwise.
*/
	{

	if (aRequested.iMajor<aCurrent.iMajor || (aRequested.iMajor==aCurrent.iMajor && aRequested.iMinor<=aCurrent.iMinor))
		return(ETrue);
	return(EFalse);
	}




EXPORT_C TKey::TKey()
/**
Protected default constructor.

This constructor prevents TKey objects from being constructed directly.
*/
	{}




EXPORT_C TKey::TKey(TInt anOffset,TKeyCmpText aType)
	: iKeyOffset(anOffset),iCmpType(ECmpCollated16+aType+1)
/**
Constructs the characteristics of a descriptor type key.

This constructor should be called by the corresponding derived class
constructor that takes the same arguments. Typically, the derived class
constructor calls this constructor in its constructor initialization list.

No length value is passed as this is implied by the type of key.

Note that the constructor sets the offset value into the protected data member 
iKeyOffset.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two descriptor keys.
                
@panic USER 98, if anOffset is negative.
                
@see TKeyCmpText
*/
	{

	__ASSERT_ALWAYS(iKeyOffset>=0,Panic(EKeyOffsetNegative));
	}




EXPORT_C TKey::TKey(TInt anOffset,TKeyCmpText aType,TInt aLength)
	: iKeyOffset(anOffset),iKeyLength(aLength),iCmpType(aType)
/**
Constructs the characteristics of a text key.

This constructor should be called by the corresponding derived class
constructor that takes the same arguments. Typically, the derived class
constructor calls this constructor in its constructor initialization list.

Note that the constructor sets the offset value into the protected data member 
iKeyOffset.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two text keys.
@param aLength  The length of the text key.

@panic USER 98, if anOffset is negative.

@see TKeyCmpText
*/
	{

	__ASSERT_ALWAYS(iKeyOffset>=0,Panic(EKeyOffsetNegative));
	}




EXPORT_C TKey::TKey(TInt anOffset,TKeyCmpNumeric aType)
	: iKeyOffset(anOffset),iCmpType(aType)
/**
Constructs the characteristics of a numeric key.

This constructor should be called by the corresponding derived class
constructor that takes the same arguments. Typically, the derived class
constructor calls this constructor in its constructor initialization list.

No length value is passed as this is implied by the type of key.

Note that the constructor sets the offset value into the protected data member 
iKeyOffset.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of the numeric key.

@panic USER 98, if anOffset is negative.

@see TKeyCmpNumeric
*/
	{

	__ASSERT_ALWAYS(iKeyOffset>=0,Panic(EKeyOffsetNegative));
	}




EXPORT_C TInt TKey::Compare(TInt aLeft,TInt aRight) const
/**
Compares the keys of two array elements.

This function is called by User::BinarySearch() and User::QuickSort().

The position of the elements are identified by the specified index values. 
The default implementation uses the At() virtual function to convert the index 
values into pointers to the elements themselves.

The default implementation also uses:

1. the TDesC comparison functions to compare descriptor type keys

2. the Mem functions to compare text type keys

3. numeric comparison for numeric type keys.

@param aLeft  The index of an array element participating in the comparison, 
              designated the left element.
@param aRight The index of an array element participating in the comparison, 
              designated the right element.
              
@return Zero, if the two keys are equal;
        negative, if the left key is less than the right key;
        positive, if the left key is greater than the right key.
        
@see User::BinarySearch()
@see User::QuickSort()
@see TDesC
@see Mem
*/
	{

	UPTR left;
	left.tany=At(aLeft);
	UPTR right;
	// coverity[returned_pointer]
	right.tany=At(aRight);
	TInt r=(-1);
	switch (iCmpType)
		{
#if !defined(_UNICODE)
	case ECmpNormal:
#endif
	case ECmpNormal8:
		r=Mem::Compare(left.ttext8,iKeyLength,right.ttext8,iKeyLength);
		break;
#if defined(_UNICODE)
	case ECmpNormal:
#endif
	case ECmpNormal16:
		r=Mem::Compare(left.ttext16,iKeyLength,right.ttext16,iKeyLength);
		break;
#if !defined(_UNICODE)
	case ECmpFolded:
#endif
	case ECmpFolded8:
		r=Mem::CompareF(left.ttext8,iKeyLength,right.ttext8,iKeyLength);
		break;
#if defined(_UNICODE)
	case ECmpFolded:
#endif
	case ECmpFolded16:
		r=Mem::CompareF(left.ttext16,iKeyLength,right.ttext16,iKeyLength);
		break;
#if !defined(_UNICODE)
	case ECmpCollated:
#endif
	case ECmpCollated8:
		r=Mem::CompareC(left.ttext8,iKeyLength,right.ttext8,iKeyLength);
		break;
#if defined(_UNICODE)
	case ECmpCollated:
#endif
	case ECmpCollated16:
		r=Mem::CompareC(left.ttext16,iKeyLength,right.ttext16,iKeyLength);
		break;
#if !defined(_UNICODE)
	case ECmpCollated16+ECmpNormal+1:
#endif
	case ECmpCollated16+ECmpNormal8+1:
		r=left.tdesc8->Compare(*right.tdesc8);
		break;
#if defined(_UNICODE)
	case ECmpCollated16+ECmpNormal+1:
#endif
	case ECmpCollated16+ECmpNormal16+1:
		r=left.tdesc16->Compare(*right.tdesc16);
		break;
#if !defined(_UNICODE)
	case ECmpCollated16+ECmpFolded+1:
#endif
	case ECmpCollated16+ECmpFolded8+1:
		r=left.tdesc8->CompareF(*right.tdesc8);
		break;
#if defined(_UNICODE)
	case ECmpCollated16+ECmpFolded+1:
#endif
	case ECmpCollated16+ECmpFolded16+1:
		r=left.tdesc16->CompareF(*right.tdesc16);
		break;
#if !defined(_UNICODE)
	case ECmpCollated16+ECmpCollated+1:
#endif
	case ECmpCollated16+ECmpCollated8+1:
		r=left.tdesc8->CompareC(*right.tdesc8);
		break;
#if defined(_UNICODE)
	case ECmpCollated16+ECmpCollated+1:
#endif
	case ECmpCollated16+ECmpCollated16+1:
		r=left.tdesc16->CompareC(*right.tdesc16);
		break;
	case ECmpTInt:
		if (*left.tint==*right.tint)
			r=0;
		else if (*left.tint>*right.tint)
			r=1;
		break;	   
	case ECmpTUint:
		if (*left.tuint==*right.tuint)
			r=0;
		else if (*left.tuint>*right.tuint)
			r=1;
		break;	   
	case ECmpTInt8:
		if (*left.tint8==*right.tint8)
			r=0;
		else if (*left.tint8>*right.tint8)
			r=1;
		break;	   
	case ECmpTUint8:
		if (*left.tuint8==*right.tuint8)
			r=0;
		else if (*left.tuint8>*right.tuint8)
			r=1;
		break;
	case ECmpTInt16:
		if (*left.tint16==*right.tint16)
			r=0;
		else if (*left.tint16>*right.tint16)
			r=1;
		break;	   
	case ECmpTUint16:
		if (*left.tuint16==*right.tuint16)
			r=0;
		else if (*left.tuint16>*right.tuint16)
			r=1;
		break;
	case ECmpTInt32:
		if (*left.tint32==*right.tint32) 
			r=0;
		else if (*left.tint32>*right.tint32) 
			r=1;
		break;	   
	case ECmpTUint32:
		if (*left.tuint32==*right.tuint32)
			r=0;
		else if (*left.tuint32>*right.tuint32)
			r=1;
		break;
	case ECmpTInt64:
		if (*left.tint64==*right.tint64) 
			r=0;
		else if (*left.tint64>*right.tint64) 
			r=1;
		break;	   
		}
	return(r);
	}




EXPORT_C TAny* TKey::At(TInt /*anIndex*/) const
/**
Gets a pointer to the key of a specified array element.
	
The default implementation raises a USER 35 panic.
	
The function is called by TKey::Compare() to compare the keys of two elements.
	
The implementation provided by a derived class must convert the index to a 
pointer to the key within the corresponding element. The implementation depends 
on the design of the array but, as general rule, use the index value to get 
a pointer to the corresponding element and then add the TKey protected data 
member iKeyOffset to this pointer to get a pointer to the key itself.
	
By convention, the index value is relative to zero; i.e. a zero value refers 
to the first element in the array. By this convention, the index can take 
any value between zero and the number of elements within the array minus one.
	
The function must also handle the special index value KIndexPtr. When this 
value is passed, the function should return a pointer to the key within the 
sample element. A pointer to the sample element is held in the protected data 
member iPtr and can be set up using SetPtr().
	
The implementation of this function also assumes that the derived class has 
a pointer to the array itself or has a function for finding it.
	
@param anIndex The index of the array element or the special index value KIndexPtr.

@return An untyped pointer to the key within the specified array element or 
        an untyped pointer to the key within the sample element, if KIndexPtr
        is passed as an argument.

@panic USER 35, if no replacement function has been provided by a derived class.
        
@see TKey::Compare
@see TKey::SetPtr
@see KIndexPtr
*/
	{

	Panic(ETFuncTKeyVirtualAt);
	return(NULL);
	}




EXPORT_C TSwap::TSwap()
/**
Default constructor.

The constructor has an empty implementation.
*/
	{}




EXPORT_C void TSwap::Swap(TInt /*aLeft*/,TInt /*aRight*/) const
/**
Swaps two elements of an array.
	
This function is called by User::QuickSort().
	
The default implementation raises a USER 36 panic.
	
In general, the class must provide a way of translating the indexes representing 
the two elements into pointers to the elements themselves. The Mem::Swap() 
utility function can then be used to swap the two elements. This implies that 
the derived class must contain a pointer to the array itself and have access 
to other information about the array, such as the length of elements.
	
By convention, the index value is relative to zero; i.e. a zero value refers 
to the first element in the array.
	
@param aLeft  The index of an array element participating in the swap
@param aRight The index of an array element participating in the swap

@panic USER 36, if no replacement function has been provided by a derived class.

@see User::QuickSort
@see Mem::Swap
*/
	{

	Panic(ETFuncTSwapVirtualSwap);
	}




EXPORT_C TVersion::TVersion()
/**
Default constructor.

It sets the major, minor and build numbers to zero.
*/
	: iMajor(0),iMinor(0),iBuild(0)
	{}




EXPORT_C TVersion::TVersion(TInt aMajor,TInt aMinor,TInt aBuild)
/**
Constructs the object with the specified major version number, the minor
version number and the build number.

Note that the constructor does not check that the values passed are within
the specified ranges. As the parameters are TInt types, care must be taken to
ensure that values passed do not exceed the specified maxima, otherwise they
will be interpreted as negative values.

@param aMajor The major version number. This must be a number in the
              range 0 to 127.
@param aMinor The minor version number. This must be a number in the
              range 0 to 99.
@param aBuild The build number. This must be a number in the range 0 to 32,767.

*/
	: iMajor((TInt8)aMajor),iMinor((TInt8)aMinor),iBuild((TInt16)aBuild)
	{}




EXPORT_C TVersionName TVersion::Name() const
/**
Gets a descriptor buffer containing the formatted character representation
of the version information.

The general format of the representation is: xxx.yy(zzzzz)

where:

1. xxx is the major version number; depending on the value, this may have
   a length of one, two or three characters.

2. yy is the minor version number; this is always two characters, padded
   with a leading zero, if necessary. 

3. zzzzz is the build number; depending on the value, this may have a length
   of one to 5 characters.

Note that if the object is constructed with values that exceed the permitted
range, they will appear negative in their formatted character representation.

@return A buffer descriptor containing the formatted character representation.
*/
	{

	TVersionName v;
	v.AppendNum(iMajor);
	v.Append(TChar('.'));
	v.AppendNumFixedWidth(iMinor,EDecimal,2);
	v.Append(TChar('('));
	v.AppendNum(iBuild);
	v.Append(TChar(')'));
//	v.Format(_L("%d.%02d(%d)"),iMajor,iMinor,iBuild);
	return(v);
	}




/**
Signals the current thread that the asynchronous request associated with the 
specified request status object is complete.

This function is used to complete an asynchronous request originating in the 
same thread as the code that is currently executing. If a request originates 
in another thread, then executing code must use RThread::RequestComplete() 
to signal the completion of that request.

The request is completed with the completion code passed in aReason. This 
value is copied into the request status, pointed to by aStatus, before 
signalling the current thread's request semaphore.

The meaning of the completion code passed in aReason is a matter of convention 
to be decided between the service requester and the service provider.

@param aStatus A reference to a pointer to the request status object. This 
               is a pointer into the current thread's address space. 
               On return, the pointer to the request status is set to NULL.
               Note that setting the pointer to NULL is a convenience, 
               not all servers need it, and is done before 
               the function returns.
               
@param aReason The completion code of this request.

@see  RThread::RequestComplete
*/
EXPORT_C void User::RequestComplete(TRequestStatus * &aStatus,TInt aReason)
	{
	*aStatus=KRequestPending;
	RThread().RequestComplete(aStatus,aReason);
	}




EXPORT_C TLdrInfo::TLdrInfo()
//
// Constructor
//
	{
	memclr(this, sizeof(TLdrInfo));
	iRequestedVersion = KModuleVersionWild;
	}

EXPORT_C TPtrC8 TCodeSegCreateInfo::RootName() const
	{
	return iFileName.Mid(iRootNameOffset,iRootNameLength);
	}

EXPORT_C TBool TUid::operator==(const TUid& aUid) const
/**
Compares two UIDs for equality.

@param aUid The UID to be compared with this UID.

@return True, if the two UIDs are equal; false otherwise. 
*/
	{

	return(iUid==aUid.iUid);
	}




EXPORT_C TBool TUid::operator!=(const TUid& aUid) const
/**
Compares two UIDs for inequality.

@param aUid The UID to be compared with this UID.

@return True, if the two UIDs are unequal; false otherwise. 
*/
	{

	return(iUid!=aUid.iUid);
	}




EXPORT_C TUidName TUid::Name() const
/**
Generates and returns the standard text form of the UID.

The resulting text has the form:

@code
[12345678]
@endcode

The function always generates 10 characters, where the first and last characters 
are open and close square brackets enclosing exactly 8 hexadecimal digits 
(padded to the left with zeroes, if necessary).

@return A modifiable descriptor containing the standard text format of the 
        UID.
*/
	{

	TUidName n;
	n.Append(TChar('['));
	n.AppendNumFixedWidth(iUid,EHex,8);
	n.Append(TChar(']'));
//	n.Format(_L("[%08x]"),iUid);
	return(n);
	}




EXPORT_C TUidType::TUidType()
/**
Default constructor.

Creates a UID type, and sets all three component UIDs to KNullUid.
*/
    {

	Mem::FillZ(this,sizeof(TUidType));
    }




EXPORT_C TUidType::TUidType(TUid aUid1)
/**
Constructor that creates a UID type and sets the UID1 component
to the specified value.

The UID2 and UID3 components are set to KNullUid.

@param aUid1 Value for UID1.
*/
    {


	Mem::FillZ(this,sizeof(TUidType));
    iUid[0]=aUid1;
    }




EXPORT_C TUidType::TUidType(TUid aUid1,TUid aUid2)
/**
Constructor that creates a UID type and sets the UID1 and UID2 components
to the specified values. 

The UID3 component is set to KNullUid.

@param aUid1 Value for UID1. 
@param aUid2 Value for UID2.
*/
    {

    iUid[0]=aUid1;
    iUid[1]=aUid2;
    iUid[2]=KNullUid;
    }




EXPORT_C TUidType::TUidType(TUid aUid1,TUid aUid2,TUid aUid3)
/**
Constructor that creates a UID type and sets all three UID components
to the specified values.

@param aUid1 Value for UID1.
@param aUid2 Value for UID2.
@param aUid3 Value for UID3.
*/
    {


    iUid[0]=aUid1;
    iUid[1]=aUid2;
    iUid[2]=aUid3;
    }




EXPORT_C TBool TUidType::operator==(const TUidType& aUidType) const
/**
Compares this UID type for equality with the specified UID type.

@param aUidType The UID type to be compared. 

@return True, if each component UID is equal to the corresponding component 
        UID in the specified UID type; false, otherwise.
*/
    {

    return(iUid[0]==aUidType.iUid[0] &&
           iUid[1]==aUidType.iUid[1] &&
           iUid[2]==aUidType.iUid[2]);
    }




EXPORT_C TBool TUidType::operator!=(const TUidType& aUidType) const
/** 
Compares this UID type for inequality with the specified UID type.

@param aUidType The UID type to be compared.

@return True, if any component UID is not equal to the corresponding component 
UID in the specified UID type; false otherwise.
*/
    {


    return(!(*this==aUidType));
    }




EXPORT_C const TUid& TUidType::operator[](TInt aIndex) const
/**
Gets the UID component as identified by the specified index.

@param aIndex Index value indicating which UID component to return.
               0 specifies UID1,
               1 specifies UID2,
               2 specifies UID3.

@return A reference to the required UID component.

@panic USER 37 if aIndex is not in the range 0 to 2, inclusive.
*/
    {

	__ASSERT_ALWAYS(aIndex>=0 && aIndex<KMaxCheckedUid,Panic(ETFuncUidTypeBadIndex));
	return(iUid[aIndex]);
    }




EXPORT_C TUid TUidType::MostDerived() const
/**
Gets the most derived UID.

Taking the three UID components as a hierarchy with UID1 being the most general, 
UID2 being more specific than UID1 and UID3 being more specific than UID2, 
then the function returns:

UID3, if UID3 is not KNullUid.

UID2, if UID2 is not KNullUid.

UID1, otherwise

@return The most derived UID.

@see KNullUid
*/
    {

    if (iUid[2]!=KNullUid)
        return(iUid[2]);
    if (iUid[1]!=KNullUid)
        return(iUid[1]);
    return(iUid[0]);
    }




EXPORT_C TBool TUidType::IsPresent(TUid aUid) const
/**
Tests if any of the component UIDs are equal to the specified UID.

@param aUid The UID to be tested.

@return True, if any one of the component UIDs are the same as the specified 
        UID; false, if none of the component UIDs are the same.
*/
    {

	return(iUid[0]==aUid || iUid[1]==aUid || iUid[2]==aUid);
    }




EXPORT_C TBool TUidType::IsValid() const
/**
Tests the object for a valid (non-KNullUid) UID type.

@return True, if at least one of the component UIDs is not KNullUid; false, 
        if all component UIDs are KNullUid.

@see KNullUid
*/
    {

    return(MostDerived()!=KNullUid);
    }




EXPORT_C TCheckedUid::TCheckedUid()
/**
Default constructor.

Initialises the object to binary zeroes.
*/
	{

	Mem::FillZ(this,sizeof(TCheckedUid));
	}




EXPORT_C TCheckedUid::TCheckedUid(const TUidType& aUidType)
/**
Constructor taking an existing Uid type.

The constructor calculates a checksum.

@param aUidType The Uid type to be packaged.
*/
	{

    Set(aUidType);
    }




EXPORT_C TCheckedUid::TCheckedUid(const TDesC8& aPtr)
/**
Constructor taking an existing TCheckedUid object encapsulated within
a descriptor.

The checksum is recalculated and must match the checksum value passed in the 
encapsulated TCheckedUid object, otherwise the content of this object is reset 
to binary zeroes.

@param aPtr A pointer to a descriptor containing an existing TCheckedUid object. 
                        
@panic USER 38 If the length of the descriptor is not the same as the size 
       of a TCheckedUid object.
*/
	{

	Set(aPtr);
	}




EXPORT_C void TCheckedUid::Set(const TUidType& aUidType)
/**
Sets the specified Uid type to be packaged, and recalculates the checksum.

@param aUidType The Uid type to be packaged.
*/
	{

    iType=aUidType;
    iCheck=Check();
    }




EXPORT_C void TCheckedUid::Set(const TDesC8& aPtr)
/**
Sets an existing TCheckedUid object encapsulated within a descriptor.

The checksum is recalculated and must match the checksum value passed in the 
encapsulated TCheckedUid object, otherwise the content of this object is reset 
to binary zeroes.

@param aPtr A pointer to a descriptor containing an existing
            TCheckedUid object.

@panic USER 38 If the length of the descriptor is not the same as the size 
       of a TCheckedUid object.
*/
	{

	__ASSERT_ALWAYS(aPtr.Length()==sizeof(TCheckedUid),Panic(ETFuncCheckedUidBadSet));
	Mem::Move(this,aPtr.Ptr(),sizeof(TCheckedUid));
	if (iCheck!=Check())
		Mem::FillZ(this,sizeof(TCheckedUid));
	}




EXPORT_C TPtrC8 TCheckedUid::Des() const
/**
Gets a pointer descriptor to represent this object's data.

@return The pointer descriptor for this object's data. The descriptor's length
        is the same as the length of a TCheckedUid object.
*/
	{

	return(TPtrC8((const TUint8*)this,sizeof(TCheckedUid)));
	}




EXPORT_C TUint TCheckedUid::Check() const
/**
Calculates the checksum of the UIDs.

@return The checksum.
*/
	{

	return((checkSum(((TUint8*)this)+1)<<16)|checkSum(this));
	}




EXPORT_C TInt User::InfoPrint(const TDesC& aDes)
/**
Invokes the notifier server to display a text message on the screen for a short 
time. 

@param aDes A reference to the descriptor containing the text to be sent to 
            the notifier server.
            
@return KErrNone if successful, otherwise one of the system-wide error codes.

@see RNotifier
*/
	{

	RNotifier notif;
	TInt r=notif.Connect();
	if (r!=KErrNone)
		return(KErrGeneral);
	r=notif.InfoPrint(aDes);
	notif.Close();
	return(r);
	}

static const TUint32 CrcTab32[256] =
	{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};


/**
Performs a CCITT CRC-32 checksum on the specified data.

On return from this function, the referenced 32 bit integer contains the CRC
value.

@param aCrc		A reference to a 32 bit integer to contain the CRC value. 
@param aPtr		A pointer to the start of the data to be checksummed. 
@param aLength	The length of the data to be checksummed.
*/
EXPORT_C void Mem::Crc32(TUint32& aCrc, const TAny* aPtr, TInt aLength)
	{
	const TUint8* p = (const TUint8*)aPtr;
	const TUint8* q = p + aLength;
	TUint32 crc = aCrc;
	while (p < q)
		crc = (crc >> 8) ^ CrcTab32[(crc ^ *p++) & 0xff];
	aCrc = crc;
	}
