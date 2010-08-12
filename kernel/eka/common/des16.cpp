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
// e32\common\des16.cpp
// 
//

#include "common.h"
#include <e32des16_private.h>
#include <collate.h>
#ifdef _UNICODE
#include <unicode.h>
#include "collateimp.h"
#endif
#include "CompareImp.h"


#define __CHECK_ALIGNMENT(p,c) __ASSERT_DEBUG(!(TUint(p)&1),Des16Panic(c))

enum TDes16Panic
	{
	ETDesC16ConstructCString=0,
	ETDesC16ConstructBufLength=1,
	ETDesC16ConstructBufLengthMax=2,
	ETDesC16FindPtrLen=3,
	ETDesC16FindFPtrLen=4,
	ETDesC16FindCPtrLen=5,
	ETBufCBase16CopyStringMax=6,
	EHBufC16AssignCString=7,
	ETDes16AssignCString=8,
	ETDes16CopyCString=9,
	ETDes16CopyBufLength=10,
	ETDes16AppendBufLength=11,
	ETDes16RepeatBufLength=12,
	ETDes16AppendJustify1=13,
	ETDes16AppendJustify2=14,
	ETPtrC16SetBufLength=15,
	ETPtr16SetBufLengthMax=16,
	ETDesC16Invariant=17,
	ETDesC16Ptr=18
	};

#ifdef _DEBUG
_LIT(KLitDes16Align,"Des16Align");
void Des16Panic(TDes16Panic aPanic)
	{
	PANIC_CURRENT_THREAD(KLitDes16Align,aPanic);
	}
#endif

inline TUint16* memCopy(TUint16* aPtr, const TUint16* aSrc, TInt aLength)
	{
	return (TUint16 *)Mem::Copy(aPtr,aSrc,aLength<<1);
	}

#if !defined( __DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
inline TInt StringLength(const TUint16* aPtr)
	{
	const TUint16* p = aPtr;
	while (*p)
		++p;
	return p-aPtr;
	}
#endif

inline TDesC16::TDesC16(TInt aType,TInt aLength)
	:iLength(aLength|(aType<<KShiftDesType16))
	{}
inline TInt TDesC16::Type() const
//
// Return the descriptor type
//
	{
	return(iLength>>KShiftDesType16);
	}

inline TDes16::TDes16(TInt aType,TInt aLength,TInt aMaxLength)
	: TDesC16(aType,aLength),iMaxLength(aMaxLength)
	{}

// Class TBufCBase16
inline TBufCBase16::TBufCBase16(TInt aLength)
	:TDesC16(EBufC,aLength)
	{}
inline TUint16* TBufCBase16::WPtr() const
	{return const_cast<TUint16*>(Ptr());}

#if !defined( __DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TPtr16::TPtr16(TBufCBase16& aLcb, TInt aMaxLength)
	: TDes16(EBufCPtr,aLcb.Length(),aMaxLength),iPtr((TUint16*)&aLcb)
	{
	__ASSERT_DEBUG(aLcb.Length()<=aMaxLength,Panic(ETDes16LengthOutOfRange));
	}
#endif

#if !defined( __DES16_MACHINE_CODED__)
/**
Gets a pointer to the data represented by the descriptor.

The data cannot be changed through the returned pointer.

@return A pointer to the data
*/
EXPORT_C const TUint16 *TDesC16::Ptr() const
	{

	const TUint16* p=NULL;
	switch (Type())
		{
	case EBufC:
		p=(&((SBufC16 *)this)->buf[0]); break;
	case EPtrC:
		p=(((SPtrC16 *)this)->ptr); break;
	case EPtr:
		p=(((SPtr16 *)this)->ptr); break;
	case EBuf:
		p=(&((SBuf16 *)this)->buf[0]); break;
	case EBufCPtr:
		p=(&((SBufCPtr16 *)this)->ptr->buf[0]); break;
	default:
		Panic(ETDes16BadDescriptorType);
		}
	__CHECK_ALIGNMENT(p,ETDesC16Ptr);
	return p;
	}

EXPORT_C const TUint16 &TDesC16::AtC(TInt anIndex) const
//
// Return a reference to the character in the buffer.
//
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<Length(),Panic(ETDes16IndexOutOfRange));
	return(Ptr()[anIndex]);
	}




/**
Compares this descriptor's data with the specified descriptor's data.

The comparison proceeds on a double-byte for double byte basis.
The result of the comparison is based on the difference of the first pair
of bytes to disagree.

Two descriptors are equal if they have the same length and content. Where 
two descriptors have different lengths and the shorter descriptor's data 
matches the first part of the longer descriptor's data, the shorter is
considered to be less than the longer.

@param aDes The 16-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive. if this descriptor is greater than the specified descriptor.
        Negative. if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
*/
EXPORT_C TInt TDesC16::Compare(const TDesC16 &aDes) const
	{
	return MEM_COMPARE_16(Ptr(),Length(),aDes.Ptr(),aDes.Length());
	}




/**
Compares this descriptor's folded data with the specified descriptor's folded 
data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for comparing strings in natural language; 
use CompareC() for this.

@param aDes The 16-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if both descriptors have the same length and the their contents
        are the same.
        
@see TDesC16::Compare()
*/
EXPORT_C TInt TDesC16::CompareF(const TDesC16 &aDes) const
	{

	return(Mem::CompareF(Ptr(),Length(),aDes.Ptr(),aDes.Length()));
	}




/**
Compares this descriptor's data with the specified descriptor's data using 
the standard collation method appropriate to the current locale.

@param aDes The 16-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if the content of both descriptors match.
        
@see TDesC16::Compare()
*/
EXPORT_C TInt TDesC16::CompareC(const TDesC16 &aDes) const
	{

	return(Mem::CompareC(Ptr(),Length(),aDes.Ptr(),aDes.Length()));
	}
#endif




#ifdef _UNICODE
/**
Compares this descriptor's data with the specified descriptor's data to the 
specified maximum collation level and using the specified collation method.

If no collation method is supplied, a default method is used that uses a
locale-independent collation table. This means that sorting and matching will
not be based on the current locale.

This function is only defined for 16-bit (Unicode) build variants. This means 
that the function is not defined for 8-bit build variants, even when an
explicit 16-bit descriptor is used.

Strings may match even if the lengths of their respective descriptors are 
different.

@param aDes             The 16-bit non-modifable descriptor whose data is to
                        be compared with this descriptor's data.
                          
@param aMaxLevel        The maximum collation level. This is an integer with 
                        values: 0, 1, 2 or 3, which, effectively, determines
                        how 'tight' the matching should be. Level 3 is always
                        used if the aim is to sort strings.
                 
@param aCollationMethod A pointer to the collation method or NULL. Collation 
                        methods can be retrieved by calls to
                        Mem::CollationMethodByIndex()
                        and Mem::CollationMethodById(). 
                        Specifying NULL means that the default method is used.
                        
@return Positive, if this descriptor is greater than the specified descriptor. 
        Negative, if this descriptor is less than the specified descriptor.
        Zero, if the content of both descriptors match.
        
@see Mem::CollationMethodByIndex()
@see Mem::CollationMethodById()
@see TDesC16::Compare()
*/
EXPORT_C TInt TDesC16::CompareC(const TDesC16& aDes, TInt aMaxLevel, const TCollationMethod* aCollationMethod) const
	{
	return Mem::CompareC(Ptr(),Length(),aDes.Ptr(),aDes.Length(),aMaxLevel,aCollationMethod);
	}

/**
Get the normalized decomposed form of this 16 bit descriptor
@return A pointer to the 16-bit heap buffer containing normalized decomposed form
		if creation is successful
@leave KErrNoMemory if not enough memory to construct the output buffer
*/
EXPORT_C HBufC16* TDesC16::GetNormalizedDecomposedFormL() const
	{
	//pre create a buffer with of size Length
	TInt strLength=Length();
	HBufC16* outBuf=HBufC16::NewL(strLength);

	TUTF32Iterator input(Ptr(),Ptr()+strLength);
	TCanonicalDecompositionIterator output;
	output.Set(input);

	TInt currentAllocateCount=0;
	TUint16* outPtr = (TUint16* )outBuf->Des().Ptr();
	TInt preAllocateCount= outBuf->Des().MaxLength();

	for (;!output.AtEnd();output.Next(),currentAllocateCount++)
		{
		if (currentAllocateCount>=preAllocateCount)
			{
			const TInt KMaxGrowSize=16;
			HBufC16* newOutBuf = outBuf->ReAlloc(preAllocateCount+KMaxGrowSize);
			if(!newOutBuf)
				{
				delete outBuf;
				User::Leave(KErrNoMemory);
				}
			outBuf = newOutBuf;
			outPtr = (TUint16* )outBuf->Des().Ptr();
			preAllocateCount = outBuf->Des().MaxLength();
			}
		outPtr[currentAllocateCount] = (TUint16)(TUint)(output.Current());
		}
	// update the length of the buffer...
	outBuf->Des().SetLength(currentAllocateCount);

	//compress the unused slot
	if (currentAllocateCount<preAllocateCount)
		outBuf = outBuf->ReAlloc(currentAllocateCount); // can't fail to shrink memory
	return outBuf;
	}

/**
Get the folded decomposed form of this 16 bit descriptor
@return A pointer to the 16-bit heap buffer containing folded decomposed form
		if creation is succesful
@leave KErrNoMemory if not enough memory to construct the output buffer
*/	
EXPORT_C HBufC16* TDesC16::GetFoldedDecomposedFormL() const
	{
	//pre create a buffer with of size Length
	TInt strLength=Length();
	HBufC16* outBuf=HBufC16::NewL(strLength);

	TUTF32Iterator input(Ptr(),Ptr()+strLength);
	TFoldedCanonicalIterator output (input);

	TInt currentAllocateCount=0;
	TUint16* outPtr = (TUint16* )outBuf->Des().Ptr();
	TInt preAllocateCount= outBuf->Des().MaxLength();
	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;
	for (;!output.AtEnd();output.Next(charDataSet),currentAllocateCount++)
		{
		if (currentAllocateCount>=preAllocateCount)
			{
			const TInt KMaxGrowSize=16;
			HBufC16* newOutBuf = outBuf->ReAlloc(preAllocateCount+KMaxGrowSize);
			if(!newOutBuf)
				{
				delete outBuf;
				User::Leave(KErrNoMemory);
				}
			outBuf = newOutBuf;
			outPtr = (TUint16* )outBuf->Des().Ptr();
			preAllocateCount = outBuf->Des().MaxLength();
			}
		outPtr[currentAllocateCount] = (TUint16)(TUint)(output.Current());
		}
	// update the length of the buffer...
	outBuf->Des().SetLength(currentAllocateCount);

	//compress the unused slot
	if (currentAllocateCount<preAllocateCount)
		outBuf = outBuf->ReAlloc(currentAllocateCount); // can't fail to shrink memory
	return outBuf;
	}

//some utils function
static void ResetAndDestroyArray(TAny* aPtr)
	{
	(STATIC_CAST(RPointerArray<HBufC8>*,aPtr))->ResetAndDestroy();
	}

/**
Get the collation keys of this 16 bit descriptor for a given collation level
If no collation method is supplied, a default method is used that uses a
locale-independent collation table. 
@param aMaxLevel        The maximum collation level. This is an integer with 
                        values: 0, 1, 2 or 3. Level 3 is always
                        used if the aim is to sort strings.
@param aCollationMethod A pointer to the collation method or NULL. Collation 
                        methods can be retrieved by calls to
                        Mem::CollationMethodByIndex()
                        and Mem::CollationMethodById(). 
                        Specifying NULL means that the default method is used.
@return A pointer to the 8-bit heap buffer containing the collation keys if
		creation is succesful
@leave KErrNoMemory if not enough memory to construct the output buffer		
*/	
EXPORT_C HBufC8* TDesC16::GetCollationKeysL(TInt aMaxLevel,const TCollationMethod* aCollationMethod) const
	{
	// Clamp the maximum level of the comparison.
	if(aMaxLevel < 0)
		aMaxLevel = 0;
	if(aMaxLevel > 3)
		aMaxLevel = 3;
	
	RPointerArray<HBufC8> levelBuffer;
 	CleanupStack::PushL(TCleanupItem(ResetAndDestroyArray, &levelBuffer));
  	TInt strLength=Length();
 	TInt outputBufferSize=0;
 	
  	//preallocate some initial buffer size as it is not possible to determine the max buffer
  	//required as a character can be further decomposed and each character can possibly
  	//have up to 8 collation keys and this limit might still change.
	for (TInt i=0;i<=aMaxLevel;i++)
		{
		TInt levelKeySize=TCollationKey::MaxSizePerKey(i);
		HBufC8* buffer=HBufC8::NewLC(strLength*levelKeySize);
		levelBuffer.AppendL(buffer);
		CleanupStack::Pop();
		outputBufferSize+=levelKeySize;
		}
	TCollationMethod clm;
	
	//if collationMethod is NULL, use the default one
	if (aCollationMethod==NULL)
		{
		clm=*(GetLocaleCharSet()->iCollationDataSet->iMethod);
		}
	else
		{
		clm = *aCollationMethod;			
		}
	//if the main table is empty use the standard table
	if (clm.iMainTable==NULL)
		clm.iMainTable=StandardCollationMethod();	
	
	TCollationValueIterator tvi(clm);	
  	TUTF32Iterator input(Ptr(),Ptr()+strLength);	
 	tvi.SetSourceIt(input);

	//Expand the buffer by 16 bytes if buffer is exhausted
	const TInt KMaxBufferGrowSize=16;
	TInt currentKeyCount=0;
	TInt preAllocateCount=strLength;
	TCollationKey collateKey;
	for (;tvi.GetCurrentKey(collateKey);tvi.Increment(),currentKeyCount++)
		{
		for (TInt i=0;i<=aMaxLevel;i++)
			{
			if (currentKeyCount==preAllocateCount)
				levelBuffer[i]=levelBuffer[i]->ReAllocL((currentKeyCount+KMaxBufferGrowSize)*TCollationKey::MaxSizePerKey(i));
			
			collateKey.AppendToDescriptor(levelBuffer[i]->Des(),i);
			}
		if (currentKeyCount==preAllocateCount)
			preAllocateCount+=KMaxBufferGrowSize;
		}
	//append the level separator which is a "\x00\x00" for level 0 and \x00 for other level
	outputBufferSize=(outputBufferSize*currentKeyCount)+(aMaxLevel==0?0:4+(aMaxLevel-1)*2);
	HBufC8* outputResult=HBufC8::NewL(outputBufferSize);
	TPtr8 outputResultPtr(outputResult->Des());
	for (TInt count=0;count<=aMaxLevel;count++)
		{
		outputResultPtr.Append(*levelBuffer[count]);
		if (count!=aMaxLevel)
			{
			if (count==0)
				outputResultPtr.Append(0);
			outputResultPtr.Append(0);
			}
		}
	CleanupStack::PopAndDestroy();
	return outputResult;	
	}	

#endif

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
       
@panic  USER 17 if aLenS is negative. 
*/
EXPORT_C TInt TDesC16::Find(const TUint16 *pS,TInt aLenS) const
	{

	if (!aLenS)
		return(0);
	__ASSERT_ALWAYS(aLenS>0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(pS,ETDesC16FindPtrLen);
	const TUint16 *pB=Ptr();
	TInt aLenB=Length();
	const TUint16 *pC=pB-1;			// using pre-increment addressing
	TInt i=aLenB-aLenS;
	if (i>=0)
		{
		const TUint16* pEndS=pS+aLenS-1;		// using pre-increment addressing
		const TUint16 *pEndB=pB+i;			// using pre-increment addressing
		TUint s=*pS;
		for (;;)
			{
			do
				{
				if (pC==pEndB)
					return KErrNotFound;
				} while (*++pC!=s);
			const TUint16 *p1=pC;
			const TUint16 *p2=pS;
			do
				{
				if (p2==pEndS)
					return (pC-pB);
				} while (*++p1==*++p2);
			}
		}
	return(KErrNotFound);
	}




/**
Searches for the first occurrence of the specified data sequence within this 
descriptor.

Searching always starts at the beginning of this descriptor's 
data.

@param aDes The 16-bit non-modifiable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
*/
EXPORT_C TInt TDesC16::Find(const TDesC16 &aDes) const
	{

	return(Find(aDes.Ptr(),aDes.Length()));
	}




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

@panic USER 17 if aLenS is negative

@see TDesC16::FindC()
*/
EXPORT_C TInt TDesC16::FindF(const TUint16 *pS,TInt aLenS) const
	{
	__CHECK_ALIGNMENT(pS,ETDesC16FindFPtrLen);
	TUTF32Iterator candidateStrIt(Ptr(), Ptr() + Length());
	TUTF32Iterator searchTermIt(pS, pS + aLenS);
	return ::FindFolded(candidateStrIt, searchTermIt);
	}




/**
Searches for the first occurrence of the specified folded data sequence within 
this descriptor's folded data.

Searching always starts at the beginning of this descriptor's data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for finding strings in natural language; 
use FindC() for this.

@param aDes The 16-bit non-modifable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
        Zero, if the length of the search data sequence is zero.
        
@see TDesC16::FindC()
*/
EXPORT_C TInt TDesC16::FindF(const TDesC16 &aDes) const
	{
	TUTF32Iterator candidateStrIt(Ptr(), Ptr() + Length());
	TUTF32Iterator searchTermIt(aDes.Ptr(), aDes.Ptr() + aDes.Length());
	return ::FindFolded(candidateStrIt, searchTermIt);
	}




/**
Searches for the first occurrence of the specified collated data sequence within
this descriptor's collated data.

Searching always starts at the beginning of this descriptor's data. The function
uses the standard collation method appropriate to the current locale.

@param aText   A pointer to a location containing the data sequence to be
               searched for.             
@param aLength The length of the data sequence to be searched for.

@return The offset of the data sequence from the beginning of this descriptor's data.
        KErrNotFound, if the data sequence cannot be found. 

@panic USER 17 if aLength is negative.
*/
EXPORT_C TInt TDesC16::FindC(const TUint16 *aText,TInt aLength) const
	{
	__CHECK_ALIGNMENT(aText,ETDesC16FindCPtrLen);

	TCollate c(GetLocaleCharSet(),TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase);
	return c.Find(Ptr(),Length(),aText,aLength,1);
	}




/**
Searches for the first occurrence of the specified collated data sequence
within this descriptor's collated data to the specified maximum collation
level.

@param aText            A pointer to a location containing the data sequence to
                        be searched for.             
                        
@param aLength          The length of the data sequence to be searched for.
                          
@param aMaxLevel        The maximum collation level. This is an integer with 
                        values: 0, 1, 2 or 3, which, effectively, determines
                        how 'tight' the matching should be. Level 3 is always
                        used if the aim is to sort strings.
                       
@return The offset of the data sequence from the beginning of this descriptor's data.
        KErrNotFound, if the data sequence cannot be found. 
*/
EXPORT_C TInt TDesC16::FindC(const TUint16 *aText,TInt aLength, TInt aMaxLevel) const

	{
	__CHECK_ALIGNMENT(aText,ETDesC16FindCPtrLen);

	TCollate c(GetLocaleCharSet(),TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase);
	return c.Find(Ptr(),Length(),aText,aLength,aMaxLevel);
	}




/**
Searches for the first occurrence of the specified collated data sequence 
within this descriptor's collated data.

Searching always starts at the beginning of this descriptor's data. The
function uses the standard collation method appropriate to the current
locale.

@param aDes The 16-bit non-modifable descriptor containing the data sequence 
            to be searched for. 
            
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
*/
EXPORT_C TInt TDesC16::FindC(const TDesC16 &aDes) const
	{

	return(FindC(aDes.Ptr(),aDes.Length()));
	}

/**
Searches for the first occurrence of the specified collated data sequence 
within this descriptor's collated data.

Searching always starts at the beginning of this descriptor's data. The
function uses the standard collation method appropriate to the current
locale.

@param aDes             The 16-bit non-modifable descriptor containing the data sequence 
                        to be searched for.

@param aLengthFound     A refernce to the maximal length of the match found in the candidate 
                        string. KErrNotFound, if the data sequence cannot be found. 

@param aCollationMethod A pointer to the collation method or NULL. Collation 
                        methods can be retrieved by calls to
                        Mem::CollationMethodByIndex()
                        and Mem::CollationMethodById(). 
                        Specifying NULL means that the default method is used.

@param aMaxLevel        The maximum collation level. This is an integer with 
                        values: 0, 1, 2 or 3, which, effectively, determines
                        how 'tight' the matching should be. Level 3 is always
                        used if the aim is to sort strings.
                          
@return The offset of the data sequence from the beginning of this descriptor's 
        data. KErrNotFound, if the data sequence cannot be found.
*/
EXPORT_C TInt TDesC16::FindC(const TDesC16 &aDes, TInt &aLengthFound, const TCollationMethod &aMethod, TInt aMaxLevel) const
	{
	TCollate c(aMethod);
	return c.Find(Ptr(),Length(),aDes.Ptr(),aDes.Length(),aLengthFound,aMaxLevel); 
	}


#ifdef _UNICODE
LOCAL_C const TText* convTable(TMatchType aType)
	{
	switch (aType)
		{
		case EMatchFolded:											  // at present, folding is...
		case EMatchCollated: return (const TText *)(TChar::EFoldStandard); // ...the same as collation: use all folding methods
		default: return 0;
		}
	}
#else
LOCAL_C const TText* convTable(TMatchType aType)
	{
	switch (aType)
		{
		case EMatchFolded: return Locl::FoldTable();
		case EMatchCollated: return Locl::CollTable();
		default: return NULL;
		}
	}
#endif

inline TUint conv(const TUint16* aStr,const TText *aConv, const TUnicodeDataSet* aCharDataSet)
//
// If aConv is not NULL then convert the character.
//
	{

#ifdef _UNICODE
	if (aConv)
		return TUnicode(*aStr).Fold((TInt)aConv, aCharDataSet);
	else
		return *aStr;
#else
	TUint c=*aStr;
	return aConv && c<0x100 ? aConv[c] : c;
#endif
	}

inline TUint lookup(const TUint16* aStr,const TText *aConv)
	{
#ifdef _UNICODE
	return TUnicode(*aStr).Fold((TInt)aConv,GetLocaleCharSet()->iCharDataSet);
#else
	TUint c=*aStr;
	return c<0x100 ? aConv[c] : c;
#endif
	}

// Surrogate-aware version of lookup() above.
// aChar can be over 0xFFFF.
inline TUint lookup2(const TUint aChar, const TText *aConv)
	{
	return TUnicode(aChar).Fold((TInt)aConv, GetLocaleCharSet()->iCharDataSet);
	}

TInt DoMatch16(const TDesC16 &aLeftD,const TDesC16 &aRightD,TMatchType aType)
	{
	const TText* table=convTable(aType);
	const TUint16* pRight=aRightD.Ptr();
	const TUint16* pM=pRight-1;				// pre-increment addressing
	const TUint16* pP=pM+aRightD.Length();
	const TUint16* pLeft=aLeftD.Ptr()-1;		// pre-increment addressing
	const TUint16* pB=pLeft;	
	const TUint16* pE=pB+aLeftD.Length();


	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;

	// Match any pattern up to the first star
	TUint c;
	for (;;)
		{
		if (pM==pP)		// exhausted the pattern
			return pB==pE ? 0 : KErrNotFound;
		TUint c=conv(++pM,table, charDataSet);
		if (c==KMatchAny)
			break;
		if (pB==pE)			// no more input
			return KErrNotFound;
		if (c!=conv(++pB,table, charDataSet) && c!=KMatchOne)	// match failed
			return KErrNotFound;
		}
	// reached a star
	if (pM==pP)
		return 0;
	TInt r=pM==pRight ? -1 : 0;
	for (;;)
		{
		c=conv(++pM,table, charDataSet);
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
			if (table)
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
			const TUint16* pb=pB;
			const TUint16* pm=pM;
			for (;;)
				{
				if (pm<pP)
					{
					TUint cc=conv(++pm,table, charDataSet);
					if (cc==KMatchAny)
						{	// sub-match successful, back to main loop
						r+=(r>=0 ? 0 : pB-pLeft);
						pB=pb;
						pM=pm;
						goto star;
						}
					if (pb==pE)
						return KErrNotFound;	// no more input
					if (cc!=conv(++pb,table, charDataSet) && cc!=KMatchOne)
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



EXPORT_C TDesC16::TPrefix TDesC16::HasPrefixC(const TDesC16& aPossiblePrefix,
	TInt aLevel, const TCollationMethod* aCollationMethod) const
/**
Compares aPossiblePrefix against the start of the descriptor, using a
collated comparison.

@param aLevel The maximum level at which to perform the collation.

              0: Only check character identities.
       
              1: Check accents as well.
       
              2: Check case as well.
          
              3: Check Unicode values.
       
              Currently only level 0 is supported.
       
@param aCollationMethod The collation method to be used for the matching.

@return  EIsPrefix, if aPossiblePrefix can be extended to
         be equivalent to the text at the start of this descriptor.
         EIsNotPrefix if aPossiblePrefix cannot
         be extended to be equivalent to the text at the start of this descriptor.
         EMightBePrefix if it currently does not seem to be a prefix, but it
         is possible that it could be extended to become equivalent to text at
         the start of this descriptor.
         EMightBePrefix is returned in cases where it would be expensive
         to determine for sure.
*/	
	{
	const TText16* lp = aPossiblePrefix.Ptr();
	const TText16* rp = Ptr();
	TInt ll = aPossiblePrefix.Length();
	TInt rl = Length();
	TInt r;
	if (!aCollationMethod)
		{
		TCollate c(GetLocaleCharSet());
		r = c.Compare(rp, rl, lp, ll, aLevel);
		}
	else
		{
		TCollate c(*aCollationMethod);
		r = c.Compare(rp, rl, lp, ll, aLevel);
		}
	return r == 1 || r == 0? EIsPrefix : EIsNotPrefix;
	}

EXPORT_C TInt TDesC16::Match(const TDesC16 &aDes) const
/**
Searches this descriptor's data for a match with the match pattern supplied 
in the specified descriptor.

The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.

@param aDes A 16-bit non-modifable descriptor containing the match pattern. 

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
*/
	{

	return DoMatch16(*this,aDes,EMatchNormal);

	}

EXPORT_C TInt TDesC16::MatchF(const TDesC16 &aDes) const
/**
Searches this descriptor's folded data for a match with the folded match pattern 
supplied in the specified descriptor.

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

@param aDes A 16-bit non-modifable descriptor containing the match pattern. 

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
        
@see TDesC16::MatchC()
*/
	{
	const TText16* csp = Ptr();
	const TText16* stp = aDes.Ptr();
	return LocateMatchStringFolded(csp, csp + Length(), stp, stp + aDes.Length());
	}

EXPORT_C TInt TDesC16::MatchC(const TDesC16 &aPattern) const
/**
Searches this descriptor's collated data for a match with the collated match 
pattern supplied in the specified descriptor.

The function uses the standard collation method appropriate to
the current locale.
	
The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.
	
@param aPattern A 16-bit non-modifable descriptor containing the match pattern.

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
*/
	{
	TCollationMethod m=*Mem::GetDefaultMatchingTable();
	m.iFlags |= (TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase);
	TCollate c(m);
	return c.Match(Ptr(), Length(), aPattern.Ptr(), aPattern.Length(), 0, '?', '*');
	}

/**
Searches this descriptor's collated data for a match with the collated match 
pattern supplied in the specified descriptor.

The function uses the standard collation method appropriate to
the current locale.
	
The match pattern can contain the wildcard characters specified by aWildChar and aWildSequenceChar 
parameters, where aWildSequenceChar matches zero or more consecutive occurrences of any character and 
aWildChar matches a single occurrence of any character.

@param aPattern A 16-bit non-modifable descriptor containing the match pattern.
@param aWildChar Wild card character which may be specified for aSearchTerm.
@param aWildSequenceChar Wild card sequence character which may be specified for aSearchTerm.
@param aEscapeChar The escape character, or 0 if there is to be none. The escape character removes any 
 				   special meaning from the subsequent character. For example, if the escape, wild card 
 				   and wild sequence characters are \, ? And * respectively, the search term "\?\*\\" matches 
 				   only the candidate string "?*\";
@param aMaxLevel Determines the tightness of the collation. At level 0, only
                 character identities are distinguished. At level 1 accents are
                 distinguished as well. At level 2 case is distinguishes as well. At
                 level 3 all valid different Unicode characters are considered different.
@param aCollationMethod A pointer to the collation method or NULL. Collation methods can be retrieved by calls to
				 Mem::CollationMethodByIndex() and Mem::CollationMethodById(). 
				 Specifying NULL means that the default method is used.

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
*/
EXPORT_C TInt TDesC16::MatchC(const TDesC16 &aPattern, TInt aWildChar, TInt aWildSequenceChar, 
							  TInt aEscapeChar, TInt aMaxLevel, const TCollationMethod* aCollationMethod) const
	{
	TCollationMethod m(aCollationMethod ? *aCollationMethod : *Mem::GetDefaultMatchingTable());
	m.iFlags |= (TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase);
	TCollate c(m);
	return c.Match(Ptr(), Length(), aPattern.Ptr(), aPattern.Length(), aMaxLevel, aWildChar, aWildSequenceChar, aEscapeChar);
	}


/**
Searches this descriptor's collated data for a match with the collated match 
pattern supplied in the specified descriptor.

The function uses the standard collation method appropriate to
the current locale.
	
The match pattern can contain the wildcard characters specified by aWildChar and aWildSequenceChar 
parameters, where aWildSequenceChar matches zero or more consecutive occurrences of any character and 
aWildChar matches a single occurrence of any character.

@param aPattern A 16-bit non-modifable descriptor containing the match pattern.
@param aFlags Flags providing advanced control of the collation algorithm @see TCollationFlag
@param aWildChar Wild card character which may be specified for aSearchTerm. Defaulted to '?' if omitted.
@param aWildSequenceChar Wild card sequence character which may be specified for aSearchTerm.
						 Defaulted to '*' if omitted.
@param aEscapeChar The escape character, or 0 if there is to be none. The escape character removes any 
 				   special meaning from the subsequent character. For example, if the escape, wild card 
 				   and wild sequence characters are \, ? And * respectively, the search term "\?\*\\" matches 
 				   only the candidate string "?*\".  Defaulted to 0 if omitted.
@param aMaxLevel Determines the tightness of the collation. Defaulted to 3 if omitted. At level 0, only
                 character identities are distinguished. At level 1 accents are
                 distinguished as well. At level 2 case is distinguishes as well. At
                 level 3 all valid different Unicode characters are considered different.
@param aCollationMethod A pointer to the collation method. Collation methods can be retrieved by calls to
				 Mem::CollationMethodByIndex(), Mem::CollationMethodById() or by custom defined name.
				 Flags can be set on definition of the custom TCollationMethod, or by const_cast-ing
				 the returned pointer and setting the iFlags field directly. @see TCollationMethod
@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
*/
EXPORT_C TInt TDesC16::MatchC(const TDesC16 &aPattern, const TCollationMethod* aCollationMethod,
							  TInt aMaxLevel, TInt aWildChar, TInt aWildSequenceChar, TInt aEscapeChar) const
	{
	TCollate c(*aCollationMethod);
	return c.Match(Ptr(), Length(), aPattern.Ptr(), aPattern.Length(), aMaxLevel, aWildChar, aWildSequenceChar, aEscapeChar);
	}


#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C TInt TDesC16::Locate(TChar aChar) const
/**
Searches for the first occurrence of a character within this descriptor's 
data.

The search starts at the beginning of the data, i.e. at the leftmost 
position.

@param aChar The character to be found. 

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	const TUint16 *pBuf=Ptr();
	const TUint16 *pB=pBuf-1;
	const TUint16 *pE=pB+Length();
	do
		{
		if (pB==pE)
			return KErrNotFound;
		} while (TUint(*++pB)!=aChar);
	return pB-pBuf;
	}
#endif

LOCAL_C TInt DoLocateF16(const TDesC16& aDes,TUint aChar)
//
// Locate character aChar in the descriptor folded.
//
	{
#ifdef _UNICODE
	const TText* table = convTable(EMatchFolded);
	TUint16 aChar16 = (TUint16)aChar;
	aChar = lookup(&aChar16,table);
#else
	const TText* table=Locl::FoldTable;
	if (aChar<0x100)
		aChar=table[aChar];
#endif
	const TUint16 *pBuf=aDes.Ptr();
	const TUint16 *pB=pBuf-1;
	const TUint16 *pE=pB+aDes.Length();
	do
		{
		if (pB==pE)
			return KErrNotFound;
		} while (lookup(++pB,table)!=aChar);
	return pB-pBuf;
	}

EXPORT_C TInt TDesC16::LocateF(TChar aChar) const
/**
Searches for the first occurrence of a folded character within this
descriptor's folded data.

The search starts at the beginning of the data, i.e. at the leftmost 
position.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The character to be found.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	return DoLocateF16(*this,aChar);

	}

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C TInt TDesC16::LocateReverse(TChar aChar) const
/**
Searches for the first occurrence of a character within this descriptor's 
data, searching from the end of the data.

The search starts at the rightmost position.

@param aChar The character to be found.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	TInt len=Length();
	if (len==0)
		return(KErrNotFound);
	const TUint16 *pB=Ptr();
	const TUint16 *pE=pB+len-1;
	while (pE>=pB)
		{
		if (TUint(*pE)==aChar)
			break;
		pE--;
		}
	return(pE<pB ? KErrNotFound : pE-pB);
	}
#endif

EXPORT_C TInt TDesC16::LocateReverseF(TChar aChar) const
/**
Searches for the first occurrence of a folded character within this descriptor's 
folded data, searching from the end of the data.

The search starts at the rightmost position.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The character to be found 
@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
*/
	{

	TInt len=Length();
	if (len==0)
		return(KErrNotFound);
	const TUint16 *pB=Ptr();
	const TUint16 *pE=pB+len-1;
	aChar.Fold();
	while (pE>=pB)
		{
		TCharF c(*pE);
		if (c==aChar)
			break;
		pE--;
		}
	return(pE<pB ? KErrNotFound : pE-pB);
	}

EXPORT_C HBufC16 *TDesC16::Alloc() const
/**
Creates a new 16-bit heap descriptor and initialises it with a copy of this 
descriptor's data.

@return A pointer to the new 16-bit heap descriptor, if creation is successful. 
        NULL, if creation of the descriptor fails.
*/
	{

	HBufC16 *pH=HBufC16::New(Length());
	if (pH)
		*pH=(*this);
	return(pH);
	}

EXPORT_C HBufC16 *TDesC16::AllocL() const
/**
Creates a new 16-bit heap descriptor and initialises it with a copy of this 
descriptor's data.

The function leaves, if creation of the descriptor fails.

@return A pointer to the 16-bit heap descriptor, if creation is successful.
*/
	{

	HBufC16 *pH=HBufC16::NewL(Length());
	*pH=(*this);
	return(pH);
	}

EXPORT_C HBufC16 *TDesC16::AllocLC() const
/**
Creates a new 16-bit heap descriptor, initialises it with a copy of this 
descriptor's data, and puts a pointer to the descriptor onto the cleanup stack.

The function leaves, if creation of the descriptor fails.

@return A pointer to the 16-bit heap descriptor, if creation is successful. 
        The pointer is also put onto the cleanup stack.
*/
	{

	HBufC16 *pH=HBufC16::NewLC(Length());
	*pH=(*this);
	return(pH);
	}

#if !defined(__DES16_MACHINE_CODED__)

EXPORT_C TPtrC16 TDesC16::Left(TInt aLength) const
/**
Extracts the leftmost part of the data. 

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the leftmost part of the data.

@param aLength The length of the data to be extracted. If this value is 
               greater than the length of the descriptor, the function 
               extracts the whole of the descriptor.
               
@return The 16-bit non-modifiable pointer descriptor representing the leftmost 
        part of the data.

@panic USER 10 if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16PosOutOfRange));
	return(TPtrC16(Ptr(),Min(aLength,Length())));
	}

EXPORT_C TPtrC16 TDesC16::Right(TInt aLength) const
/**
Extracts the rightmost part of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the rightmost part of the data.

@param aLength The length of data to be extracted. If this value
               is greater than the length of the descriptor, the function
               extracts the whole of the descriptor. 
               
@return The 16-bit non-modifiable pointer descriptor representing the rightmost 
        part of the data.

@panic USER 10 if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16PosOutOfRange));
	TInt len=Length();
	if (aLength>len)
		aLength=len;
    return(TPtrC16(Ptr()+len-aLength,aLength));
	}

EXPORT_C TPtrC16 TDesC16::Mid(TInt aPos) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by the length of the 
remainder of the data starting from the specified position.

@param aPos The starting position of the data to be extracted. This is an 
            offset value; a zero value refers to the leftmost data position.
             
@return The 16-bit non-modifiable pointer descriptor representing the specified 
        portion of the data.

@panic USER 10  if aPos is negative or aPos is greater than the
                length of the descriptor.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes16PosOutOfRange));
    return(TPtrC16(Ptr()+aPos,len-aPos));
	}

EXPORT_C TPtrC16 TDesC16::Mid(TInt aPos,TInt aLength) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a non-modifiable 
pointer descriptor to represent the defined portion.

The portion is identified by its starting position and by its length.

@param aPos    The starting position of the data to be extracted. This is an 
               offset value; a zero value refers to the leftmost data position. 
@param aLength The length of data to be extracted.

@return The 16-bit non-modifiable pointer descriptor representing the specified 
        portion of the data.
        
@panic USER 10  if aPos is negative or aPos plus aLength is greater than the
                length of the descriptor.
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && (aPos+aLength)<=Length(),Panic(ETDes16PosOutOfRange));
    return(TPtrC16(Ptr()+aPos,aLength));
	}

#endif  // !defined(__DES16_MACHINE_CODED__)


/**
 * A helper function, which moves a pointer one Unicode character forward.
 * 
 * @aStart points to the head of the string to process.
 * @aEnd   points to the end of the string. Note that aEnd points to the first
 *         16-bit unit after the string. That is, the string length (i.e, count
 *         of 16-bit units) is (aEnd-aStart).
 * 
 * On return,
 *      if find valid character, then return KErrNone, with aNewStart pointing
 *          to the 16-bit unit after the found character;
 *      if meet corrupt surrogate before find a valid character, then return
 *          KErrCorruptSurrogateFound, with aNewStart pointing to the corrupt surrogate;
 *      if meet aEnd before find a valid character, then return KErrNotFound.
 * 
 * @return KErrNone if ok;
 *         KErrNotFound if get to aEnd;
 *         KErrCorruptSurrogateFound if meet corrupt surrogate.
 */
TInt ProceedOneCharacter(const TText16* aStart, const TText16* aEnd, TText16*& aNewStart, TUint& aCurrentChar)
	{
	if (!aStart || !aEnd || aStart>=aEnd)
		return KErrNotFound;
	if (!TChar::IsSurrogate(aStart[0]))
		{
		aCurrentChar = aStart[0];
		aNewStart = const_cast<TText16*> (aStart + 1);
		return KErrNone;
		}
	else if (TChar::IsHighSurrogate(aStart[0]))
		{
		if (aEnd < aStart + 2)
			return KErrCorruptSurrogateFound;
		if (!TChar::IsLowSurrogate(aStart[1]))
			{
			aNewStart = const_cast<TText16*> (aStart + 2);
			return KErrCorruptSurrogateFound;
			}
		aCurrentChar = TChar::JoinSurrogate(aStart[0], aStart[1]);
		aNewStart = const_cast<TText16*> (aStart + 2);
		return KErrNone;
		}
	else
		{
		aNewStart = const_cast<TText16*> (aStart);
		return KErrCorruptSurrogateFound;
		}
	}

/**
 * A helper function, which moves a pointer one or more Unicode characters forward.
 * 
 * This function starts from aStart, stops when one of below conditions matched:
 *   1) 16-bit position >= (aEnd - aStart);
 *   2) 16-bit position >= aMaxInt16Position;
 *   3) character position >= aMaxCharacterPosition;
 * 
 * Specify a huge integer (say KMaskDesLength16) for aMaxInt16Position or 
 * aMaxCharacterPosition to indicate unlimited 16-bit position or character 
 * position.
 * 
 * When return, aOutInt16Position, aOutCharacterPosition and aLastChar will 
 *              indicate the same one character, whose 
 *              16-bit position <= aMaxInt16Position, and 
 *              character position <= aMaxCharacterPosition.
 * 
 * @return KErrNone if no error found;
 *         KErrNotFound if get to aEnd before find wanted position; or,
 *                      if aMaxIntPosition<=0 or aMaxCharacterPosition<=0;
 *         KErrCorruptSurrogateFound if meet corrupt surrogate.
 */
TInt ProceedMultiCharacters(const TText16* aStart, const TText16* aEnd,
							const TInt aMaxInt16Position, const TInt aMaxCharacterPosition,
							TInt& aOutInt16Position, TInt& aOutCharacterPosition, TUint& aLastChar)
	{
	TText16 *next;
	TInt status = KErrNotFound;
	aOutInt16Position = 0;
	aOutCharacterPosition = 0;
	while (aOutInt16Position <= aMaxInt16Position && aOutCharacterPosition <= aMaxCharacterPosition)
		{
		status = ::ProceedOneCharacter(aStart+aOutInt16Position, aEnd, next, aLastChar);
		if (status == KErrNotFound || status == KErrCorruptSurrogateFound)
			return status;
		if (next - aStart > aMaxInt16Position || aOutInt16Position == aMaxInt16Position || aOutCharacterPosition == aMaxCharacterPosition)
			{
			return status;
			}
		aOutInt16Position = (next - aStart);
		++aOutCharacterPosition;
		}
	return status;
	}

EXPORT_C TInt TDesC16::FindCorruptSurrogate() const
/**
Look for the first corrupt surrogate in the descriptor.

@return The 16-bit position of the first corrupt surrogate. KErrNotFound, if 
        not found.
*/
	{
	// Do not use TUTF32Iterator, because it hides some characters, including corrupt surrogate.
	TInt strLength = Length();

	const TText16* start = Ptr();
	const TText16* end = Ptr() + strLength;
	TInt int16Pos;
	TInt charPos;
	TUint lastChar;
	TInt status = ::ProceedMultiCharacters(start, end, KMaskDesLength16, KMaskDesLength16, int16Pos, charPos, lastChar);
	if (status == KErrCorruptSurrogateFound)
		return int16Pos;
	return KErrNotFound;
	}

EXPORT_C TInt TDesC16::Locate2(TChar aChar) const
/**
The surrogate aware version of Locate().

Searches for the first occurrence of a character within this descriptor's 
data.

The search starts at the beginning of the data, i.e. at the leftmost 
position.

@param aChar The Unicode character to be found. Can be inside or outside BMP.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
        KErrCorruptSurrogateFound, if meet corrupt surrogate in the searching.

@see TDesC16::Locate()
*/
	{
	TInt strLength = Length();
	const TText16* start = Ptr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		if (status != KErrNone)
			return status;
		if (currentChar == aChar)
			return int16Index;
		int16Index = (next - start);
		}
	}

LOCAL_C TInt DoLocateF16_2(const TDesC16& aDes, TUint aChar)
// Surrogate-aware version of DoLocateF16().
// Locate character aChar in the descriptor folded.
	{
	const TText* table = convTable(EMatchFolded);
	TUint aChar32 = aChar;
	aChar = lookup2(aChar32, table);
	
	// find aChar in aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	while (status == KErrNone)
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
        if (status != KErrNone)
            break;
		if (lookup2(currentChar, table) == aChar)
			return int16Index;
		int16Index = (next - start);
		}
	return status;
	}

EXPORT_C TInt TDesC16::LocateF2(TChar aChar) const
/**
The surrogate aware version of LocateF().

Searches for the first occurrence of a folded character within this
descriptor's folded data.

The search starts at the beginning of the data, i.e. at the leftmost 
position.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The Unicode character to be found. Can be inside or outside BMP.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
        KErrCorruptSurrogateFound, if meet corrupt surrogate in the searching.

@see TDesC16::LocateF()
*/
	{
	return DoLocateF16_2(*this, aChar);
	}

/**
 * Proceed backward from aEnd toward aStart by one character.
 * 
 * @aStart points to the first 16-bit unit in a descriptor.
 * @aEnd   points to the 16-bit unit after the last one. So, count of 16-bit 
 *         units to process is (aEnd-aStart).
 * 
 * On return,
 *      if valid character found, then return KErrNone, with aNewEnd pointing 
 *          to the character found;
 *      if meet corrupt surrogate before find a valid character, then return 
 *          KErrCorruptSurrogateFound, with aNewStart point to the corrupt
 *          surrogate;
 *      if aStart met, then return KErrNotFound.
 * 
 * @return KErrNone if ok;
 *         KErrNotFound if get to aStart;
 *         KErrCorruptSurrogateFound if meet corrupt surrogate.
 */
TInt RecedeOneCharacter(const TText16* aStart, const TText16* aEnd, TText16*& aNewEnd, TUint& aCurrentChar)
	{
	if (!aStart || !aEnd || aStart>=aEnd)
		return KErrNotFound;
	if (!TChar::IsSurrogate(aEnd[-1]))
		{
		aCurrentChar = aEnd[-1];
		aNewEnd = const_cast<TText16*> (aEnd - 1);
		return KErrNone;
		}
	else if (TChar::IsLowSurrogate(aEnd[-1]))
		{
		if (aEnd < aStart + 2)
			return KErrNotFound;
		if (!TChar::IsHighSurrogate(aEnd[-2]))
			{
			aNewEnd = const_cast<TText16*> (aEnd - 2);
			return KErrCorruptSurrogateFound;
			}
		aCurrentChar = TChar::JoinSurrogate(aEnd[-2], aEnd[-1]);
		aNewEnd = const_cast<TText16*> (aEnd - 2);
		return KErrNone;
		}
	else
		{
		aNewEnd = const_cast<TText16*> (aEnd);
		return KErrCorruptSurrogateFound;
		}
	}

EXPORT_C TInt TDesC16::LocateReverse2(TChar aChar) const
/**
The surrogate aware version of LocateReverse().

Searches for the first occurrence of a character within this descriptor's 
data, searching from the end of the data.

The search starts at the rightmost position.

@param aChar The Unicode character to be found. Can be inside or outside BMP.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
        KErrCorruptSurrogateFound, if meet corrupt surrogate in the searching.

@see TDesC16::LocateReverse()
*/
	{
	TInt strLength = Length();
	const TText16* start = Ptr();
	TText16* newEnd;
	TUint currentChar;
	TInt int16Index = strLength;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::RecedeOneCharacter(start, start+int16Index, newEnd, currentChar);
		if (status != KErrNone)
		    return status;
		int16Index = (newEnd - start);
		if (currentChar == aChar)
			return int16Index;
		}
	}

EXPORT_C TInt TDesC16::LocateReverseF2(TChar aChar) const
/**
The surrogate aware version of LocateReverseF().

Searches for the first occurrence of a folded character within this descriptor's 
folded data, searching from the end of the data.

The search starts at the rightmost position.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for searching strings in natural language.

@param aChar The Unicode character to be found. Can be inside or outside BMP.

@return The offset of the character position from the beginning of the data.
        KErrNotFound, if no matching character can be found.
        KErrCorruptSurrogateFound, if meet corrupt surrogate in the searching.

@see TDesC16::LocateReverseF()
*/
	{
	TInt strLength = Length();
	const TText16* start = Ptr();
	TText16* newEnd;
	TUint currentChar;
	TInt int16Index = strLength;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::RecedeOneCharacter(start, start+int16Index, newEnd, currentChar);
		if (status != KErrNone)
		    return status;
		int16Index = (newEnd - start);
		TCharF c(currentChar);
		if (c == aChar)
		    return int16Index;
		}
	}

inline TUint conv2(TUint aChar, const TText *aConv, const TUnicodeDataSet* aCharDataSet)
// Surrogate-aware version of conv().
// If aConv is not NULL then convert the character.
	{
	if (aConv)
		return TUnicode(aChar).Fold((TInt)aConv, aCharDataSet);
	else
		return aChar;
	}

// Surrogate-aware version of DoMatch16().
// This helper function uses the same search algorithm as DoMatch16().
TInt DoMatch16_2(const TDesC16 &aLeftD, const TDesC16 &aRightD, TMatchType aType)
	{
	const TText* table=convTable(aType);
	const TUint16* const pRight=aRightD.Ptr();
	const TUint16* pM=pRight-1;						// pre-increment addressing
	const TUint16* const pP=pM+aRightD.Length();
	const TUint16* const pLeft=aLeftD.Ptr()-1;		// pre-increment addressing
	const TUint16* pB=pLeft;	
	const TUint16* pB2=pLeft;						// always points to current char; pB2==pB or pB-1
	const TUint16* const pE=pB+aLeftD.Length();

	// Note: pM and pB always point to the int16 unit before the character to handle.
	//       so, pM[0] and pB[0] may be a low surrogate.
	//       but, pM[1] and pB[1] must be start of a character.
	// Note: pB2 always points to current character being handled.
	//       pB2 is used to generated return value.
	//       if pB[0] is low surrogate, then pB2=pB-1;
	//       if pB[0] is BMP, then pB2=pB.
	//
	// A 'diagram' shows the pointers:
	//
	// before search:
	//     left:       ############################
	//                ^                           ^
	//             pLeft/pB/pB2                   pE
	//
	//     right:      ############################
	//                ^^                          ^
	//              pM  pRight                    pP
	//
	//
	// after several iterations (C is the next character going to be checked):
    //     left:       ###############C############
    //                ^              ^            ^
    //             pLeft             pB/pB2       pE
    //
    //     right:      ##########C#################
    //                 ^        ^                 ^
    //                 pRight   pM                pP
	//

	const TUnicodeDataSet* charDataSet = GetLocaleCharSet()->iCharDataSet;

	// Match any pattern up to the first star
	TUint c;
	TInt status;
	TText* newStart;
	for (;;)
		{
		status = ::ProceedOneCharacter(pM+1, pP+1, newStart, c);
		if (status == KErrCorruptSurrogateFound)
		    return KErrCorruptSurrogateFound;
		if (status == KErrNotFound)		// exhausted the pattern
			return pB==pE ? 0 : KErrNotFound;
		pM = newStart - 1;
		c = conv2(c, table, charDataSet);
		if (c==KMatchAny)
			break;
		if (pB==pE)			// no more input
			return KErrNotFound;
		TUint c2;
		pB2 = pB + 1;
		status = ::ProceedOneCharacter(pB+1, pE+1, newStart, c2);
        if (status == KErrCorruptSurrogateFound)
            return KErrCorruptSurrogateFound;
		pB = newStart - 1;
		if (c != conv2(c2, table, charDataSet) && c != KMatchOne)	// match failed
			return KErrNotFound;
		}
	// reached a star
	if (pM==pP)
		return 0;
	TInt r=pM==pRight ? -1 : 0;		// r = how many int16 has been matched in candidate (aLeftD)
	for (;;)
		{
		status = ::ProceedOneCharacter(pM+1, pP+1, newStart, c);
        if (status == KErrCorruptSurrogateFound)
            return KErrCorruptSurrogateFound;
		pM = newStart - 1;
		c = conv2(c, table, charDataSet);
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
			TUint dummyC;
			pB2 = pB + 1;
			status = ::ProceedOneCharacter(pB+1, pE+1, newStart, dummyC);
	        if (status == KErrCorruptSurrogateFound)
	            return KErrCorruptSurrogateFound;
			pB = newStart - 1;
			if (r < 0)
				r -= (newStart - pB2);	// back r by 1 or 2, depending on dummyC is BMP or non-BMP.
			continue;
			}
	// Matching a non-wild character
		for (;;)
			{
			if (table)
				{
				TUint c2;
				for (;;)
					{
					pB2 = pB + 1;
					status = ::ProceedOneCharacter(pB+1, pE+1, newStart, c2);
			        if (status == KErrCorruptSurrogateFound)
			            return KErrCorruptSurrogateFound;
					pB = newStart - 1;
					if (lookup2(c2, table) == c)
						break;
					if (pB==pE)				// no more input
						return KErrNotFound;
					}
				}
			else
				{
				TUint c2;
				for (;;)
					{
					pB2 = pB + 1;
					status = ::ProceedOneCharacter(pB+1, pE+1, newStart, c2);
			        if (status == KErrCorruptSurrogateFound)
			            return KErrCorruptSurrogateFound;
					pB = newStart - 1;
					if (c2 == c)
						break;
					if (pB==pE)				// no more input
						return KErrNotFound;
					}
				}
			// Try to match up to the next star
			const TUint16* pb=pB;
			const TUint16* pm=pM;
			for (;;)
				{
				if (pm<pP)
					{
					TUint cc;
					status = ::ProceedOneCharacter(pm+1, pP+1, newStart, cc);
			        if (status == KErrCorruptSurrogateFound)
			            return KErrCorruptSurrogateFound;
					pm = newStart - 1;
					cc = conv2(cc, table, charDataSet);
					if (cc==KMatchAny)
						{	// sub-match successful, back to main loop
						r+=(r>=0 ? 0 : pB2-pLeft);
						pB=pb;
						pM=pm;
						goto star;
						}
					if (pb==pE)
						return KErrNotFound;	// no more input
					TUint cc2;
					status = ::ProceedOneCharacter(pb+1, pE+1, newStart, cc2);
			        if (status == KErrCorruptSurrogateFound)
			            return KErrCorruptSurrogateFound;
					pb = newStart - 1;
					if (cc != conv2(cc2, table, charDataSet) && cc != KMatchOne)
						break;	// sub-match failed, try next input character
					}
				else if (pb==pE)	// end of matching pattern
					{
					return r+(r>=0 ? 0 : pB2-pLeft);	// end of input, so have a match
					}
				else
					break;		// try next input character
				}
			}
		}
	}

EXPORT_C TInt TDesC16::Match2(const TDesC16 &aDes) const
/**
The surrogate aware version of Match().

Searches this descriptor's data for a match with the match pattern supplied 
in the specified descriptor.

The match pattern can contain the wildcard characters "*" and "?", where "*" 
matches zero or more consecutive occurrences of any character and "?" matches 
a single occurrence of any character.

Note that there is no 'escape character', which means that it is not possible
to match either the "*" character itself or the "?" character itself using
this function.

@param aDes A 16-bit non-modifable descriptor containing the match pattern.

@return If a match is found, the offset within this descriptor's data where 
        the match first occurs. KErrNotFound, if there is no match.
        KErrCorruptSurrogateFound, if meet corrupt surrogate in the searching.

@see TDesC16::Match()
*/
	{
	return DoMatch16_2(*this, aDes, EMatchNormal);
	}

#if !defined( __DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TBufCBase16::TBufCBase16()
//
// Constructor
//
	: TDesC16(EBufC,0)
	{}
#endif

#if !defined( __DES16_MACHINE_CODED_HWORD__) | defined(__EABI_CTORS__)
EXPORT_C TBufCBase16::TBufCBase16(const TUint16 *aString,TInt aMaxLength)
//
// Constructor
//
	: TDesC16(EBufC,0)
	{
	Copy(aString,aMaxLength);
	}
#endif

#if !defined( __DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TBufCBase16::TBufCBase16(const TDesC16 &aDes,TInt aMaxLength)
//
// Constructor
//
	: TDesC16(EBufC,0)
	{
	Copy(aDes,aMaxLength);
	}
#endif

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C void TBufCBase16::Copy(const TUint16 *aString,TInt aMaxLength)
//
// Copy from a string.
//
	{

	__CHECK_ALIGNMENT(aString,ETBufCBase16CopyStringMax);
	TInt len=STRING_LENGTH_16(aString);
	__ASSERT_ALWAYS(len<=aMaxLength,Panic(ETDes16Overflow));
	memCopy(WPtr(),aString,len);
	DoSetLength(len);
	}
#endif

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TBufCBase16::Copy(const TDesC16 &aDes,TInt aMaxLength)
//
// Copy from a descriptor.
//
	{

	TInt len=aDes.Length();
	__ASSERT_ALWAYS(len<=aMaxLength,Panic(ETDes16Overflow));
	memCopy(WPtr(),aDes.Ptr(),len);
	DoSetLength(len);
	}
#endif

inline HBufC16::HBufC16(TInt aLength)
	:TBufCBase16(aLength)
	{}

EXPORT_C HBufC16 *HBufC16::New(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 16-bit heap descriptor.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger than
                  requested.

@return A pointer to the new 16-bit heap descriptor. NULL, if the 16-bit heap 
        descriptor cannot be created.
        
@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	return new(STD_CLASS::Alloc(_FOFF(HBufC16,iBuf[aMaxLength]))) HBufC16(0);
	}

EXPORT_C HBufC16 *HBufC16::NewL(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 16-bit heap descriptor, and leaves 
on failure.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested.
                  
@return A pointer to the new 16-bit heap descriptor. The function leaves, if 
        the new 16-bit heap descriptor cannot be created.
        

@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	return static_cast<HBufC16*>(User::LeaveIfNull(New(aMaxLength)));
	}

EXPORT_C HBufC16 *HBufC16::NewLC(TInt aMaxLength)
/**
Creates, adds a pointer onto the cleanup stack and returns a pointer to, a 
new 16-bit heap descriptor; leaves on failure.

The heap descriptor is empty and its length is zero.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested.
                  
@return A pointer to the new 16-bit heap descriptor. The function leaves, if 
        the new 16-bit heap descriptor cannot be created.
        
@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	HBufC16* buf=NewL(aMaxLength);
	CleanupStack::PushL(buf);
	return buf;
	}

EXPORT_C HBufC16 *HBufC16::NewMax(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 16-bit heap descriptor.

No data is assigned into the new descriptor but its length
is set to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested. This also means that the resulting maximum
                  length of the descriptor may be greater than its length.
@return A pointer to the new 16-bit heap descriptor. NULL, if the new 16-bit 
        heap descriptor cannot be created.

@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	return new(STD_CLASS::Alloc(_FOFF(HBufC16,iBuf[aMaxLength]))) HBufC16(aMaxLength);
	}

EXPORT_C HBufC16 *HBufC16::NewMaxL(TInt aMaxLength)
/**
Creates, and returns a pointer to, a new 16-bit heap descriptor;
leaves on failure.

No data is assigned into the new descriptor but its length is set
to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger
                  than requested. This also means that the resulting 
                  maximum length of the descriptor may be greater than its length.
                  
@return A pointer to the new 16-bit heap descriptor. The function leaves, if 
        the new 16-bit heap descriptor cannot be created.

@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	return static_cast<HBufC16*>(User::LeaveIfNull(NewMax(aMaxLength)));
	}

EXPORT_C HBufC16 *HBufC16::NewMaxLC(TInt aMaxLength)
/**
Creates, adds a pointer onto the cleanup stack and returns a pointer to, a 
new 16-bit heap descriptor; leaves on failure.

No data is assigned into the new descriptor but its length
is set to aMaxLength.

Data can, subsequently, be assigned into it using the assignment operators.

@param aMaxLength The requested maximum length of the descriptor. Note that 
                  the resulting heap cell size and, therefore, the resulting
                  maximum length of the descriptor may be larger than requested.

@return A pointer to the new 16-bit heap descriptor. This is also put onto 
        the cleanup stack. The function leaves, if the new 16-bit heap descriptor 
        cannot be created.

@panic USER 18 if aMaxLength is negative.

@see HBufC16::operator=()
*/
	{
	HBufC16* buf=NewMaxL(aMaxLength);
	CleanupStack::PushL(buf);
	return buf;
	}

EXPORT_C HBufC16 &HBufC16::operator=(const TUint16 *aString)
/**
Copies data into this 16-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or
last re-allocated.

@param aString A pointer to a zero-terminated string.

@return A reference to this 16-bit heap descriptor.

@panic USER 11 if the length of the string, excluding the zero terminator,
               is greater than the maximum length of this (target) descriptor.
*/
	{

	__CHECK_ALIGNMENT(aString,EHBufC16AssignCString);
	Copy(aString,(STD_CLASS::AllocLen(this)-sizeof(TDesC16))/sizeof(TUint16));
	return(*this);
	}

EXPORT_C HBufC16 &HBufC16::operator=(const TDesC16 &aDes)
/**
Copies data into this 16-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or
last re-allocated.

@param aDes A 16-bit non-modifiable descriptor.

@return A reference to this 16-bit heap descriptor.

@panic USER 11  if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor
*/
	{

	Copy(aDes,(STD_CLASS::AllocLen(this)-sizeof(TDesC16))/sizeof(TUint16));
	return(*this);
	}

EXPORT_C HBufC16 *HBufC16::ReAlloc(TInt aMaxLength)
/**
Expands or contracts this 16-bit heap descriptor.

This is done by:

1. creating a new heap descriptor.

2. copying the original data into the new descriptor.

3. deleting the original descriptor.

@param aMaxLength The new requested maximum length of the descriptor.
                  Note that the resulting heap cell size and, therefore,
                  the resulting maximum length of the descriptor may be
                  larger than requested.
                  
@return A pointer to the new expanded or contracted 16-bit heap descriptor -  
        the original descriptor is deleted. NULL, if the new 16-bit heap descriptor 
        cannot be created - the original descriptor remains unchanged.

@panic USER 14 if aMaxLength is less than the length of the existing data.
@panic USER 18 if aMaxLength is negative.
*/
	{

	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	__ASSERT_ALWAYS(Length()<=aMaxLength,Panic(ETDes16ReAllocTooSmall));
	return((HBufC16 *)STD_CLASS::ReAlloc(this,(aMaxLength*sizeof(TUint16))+sizeof(TDesC16)));
	}

EXPORT_C HBufC16 *HBufC16::ReAllocL(TInt aMaxLength)
/**
Expands or contracts this 16-bit heap descriptor; leaves on failure.

This is done by:

1. creating a new heap descriptor.

2. copying the original data into the new descriptor.

3. deleting the original descriptor.

@param aMaxLength The new requested maximum length of the descriptor.
                  Note that the resulting heap cell size and, therefore,
                  the resulting maximum length of the descriptor may be
                  larger than requested.
                  
@return A pointer to the new expanded or contracted 16-bit heap descriptor -  
        the original descriptor is deleted. The function leaves, if the new
        16-bit heap descriptor cannot be created - the original descriptor
        remains unchanged.

@panic USER 14 if aMaxLength is less than the length of the existing data.
@panic USER 18 if aMaxLength is negative.
*/
	{

	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	__ASSERT_ALWAYS(Length()<=aMaxLength,Panic(ETDes16ReAllocTooSmall));
	return((HBufC16 *)STD_CLASS::ReAllocL(this,(aMaxLength*sizeof(TUint16))+sizeof(TDesC16)));
	}

EXPORT_C TPtr16 HBufC16::Des()
/**
Creates and returns a 16-bit modifiable pointer descriptor for the data
represented by this 16-bit heap descriptor.

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

Note that it is a common mistake to use Des() to create a TDesC16& reference. 
While not incorrect, it is simpler and much more efficient to simply dereference 
the heap descriptor.

@return A 16-bit modifiable pointer descriptor representing the data in this 
        16-bit heap descriptor.
*/
	{

	return DoDes((STD_CLASS::AllocLen(this)-sizeof(TDesC16))/sizeof(TUint16));
	}

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TDes16::SetLength(TInt aLength)
/**
Sets the length of the data represented by the descriptor to the
specified value.

@param aLength The new length of the descriptor.

@panic USER 11  if aLength is negative or is greater than the maximum length of
                this (target) descriptor.
*/
	{

	__ASSERT_ALWAYS(TUint(aLength)<=TUint(MaxLength()),Panic(ETDes16Overflow));
	DoSetLength(aLength);
	if (Type()==EBufCPtr)
		((SBufCPtr16 *)this)->ptr->length=aLength; // Relies on iType==0 for an TBufC
  	}

EXPORT_C void TDes16::SetMax()
/**
Sets the length of the data to the maximum length of the descriptor.
*/
	{

	SetLength(iMaxLength);
	}
#endif

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C void TDes16::Copy(const TUint16 *aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@panic USER 11  if the length of aString, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
	{

	__CHECK_ALIGNMENT(aString,ETDes16CopyCString);
	TInt len=STRING_LENGTH_16(aString);
	SetLength(len);
    memCopy(WPtr(),aString,len);
	}
#endif

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TDes16::Copy(const TUint16 *aBuf,TInt aLength)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aBuf    The start address of data to be copied. 
@param aLength The length of data to be copied.

@panic USER 11  if aLength is negative or is greater than maximum length
                of this (target) descriptor.
*/
	{

	__CHECK_ALIGNMENT(aBuf,ETDes16CopyBufLength);
	SetLength(aLength);
    memCopy(WPtr(),aBuf,aLength);
	}

EXPORT_C void TDes16::Copy(const TDesC16 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length
                of this (target) descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
    memCopy(WPtr(),aDes.Ptr(),len);
	}
#endif

EXPORT_C void TDes16::Copy(const TDesC8 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8 bit non modifiable descriptor. 

@panic USER 11  if the length of aDes is greater than the maximum
                length of this (target) descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint8 *pS=aDes.Ptr();
	const TUint8 *pE=pS+len;
	TUint16 *pT=WPtr();
	while (pS<pE)
		*pT++=(*pS++);
	}

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C void TDes16::Append(TChar aChar)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aChar A single character to be appended. The length of the descriptor 
             is incremented by one.
             
@panic USER 11  if the resulting new length of this descriptor is greater than
                its maximum length.
*/
	{

	TInt len=Length();
	TUint16 *pB=WPtr()+len;
	SetLength(len+1);
	*pB++=(TUint16)aChar;
	}
#endif

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TDes16::Append(const TUint16 *aBuf,TInt aLength)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aBuf    A pointer to the data to be copied.
@param aLength The length of data to be copied.

@panic USER 11  if the resulting new length of this descriptor is greater than
                its maximum length.
@panic USER 17  if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(aBuf,ETDes16AppendBufLength);
	TInt len=Length();
	SetLength(len+aLength);
    memCopy(WPtr()+len,aBuf,aLength);
	}

EXPORT_C void TDes16::Append(const TDesC16 &aDes)
/**
Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

@param aDes A 16-bit non modifiable descriptor whose data is to be appended.

@panic USER 11  if the resulting new length of this descriptor is greater than
                its maximum length.
*/
	{

	TInt len=Length();
    TInt n=aDes.Length();
	SetLength(len+n);
    memCopy(WPtr()+len,aDes.Ptr(),n);
	}
#endif

EXPORT_C void TDes16::Swap(TDes16 &aDes)
/**
Swaps the data represented by this descriptor with the data represented by 
the specified descriptor.

The lengths of both descriptors are also swapped to reflect the change.

Note that each descriptor must be capable of accommodating the contents of
the other descriptor.

Each descriptor must be capable of accommodating the contents of the other 
descriptor. If the maximum length of either descriptor is smaller than the 
length of the other descriptor, then the function raises a USER 11 panic.

@param aDes The 16-bit modifiable descriptor whose data is to be swapped with 
            the data of this descriptor.
            
@panic USER 11  if the maximum length of either descriptor is smaller than the 
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
	TUint16 *pL=WPtr();
	TUint16 *pR=aDes.WPtr();
	while (s--)
		{
		TChar a=(*pL);
		*pL++=(*pR);
		*pR++=(TUint16)a;
		}
	while (l--)
		*pR++=(*pL++);
	while (r--)
		*pL++=(*pR++);
	}

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C void TDes16::Fill(TChar aChar)
/**
Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled from the beginning up to its current length. The 
descriptor's length does not change. It is not filled to its maximum length.

@param aChar The fill character.
*/
	{

	TUint16 *pB=WPtr();
	TUint16 *pE=pB+Length();
	while (pB<pE)
		*pB++=(TUint16)aChar;
	}
#endif

EXPORT_C void TDes16::Fill(TChar aChar,TInt aLength)
/** 
Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled with the specified number of characters,
and its length is changed to reflect this.

@param aChar   The fill character.
@param aLength The new length of the descriptor and the number of fill characters 
               to be copied into it. 
               
@panic USER 11  if aLength is negative or is greater than the maximum length
                of this descriptor.
*/
	{

	SetLength(aLength);
	Fill(aChar);
	}

EXPORT_C void TDes16::AppendFill(TChar aChar,TInt aLength)
/**
Appends and fills this descriptor with the specified character.

The descriptor is appended with the specified number of characters.
and its length is changed to reflect this.

@param aChar   The fill character. 
@param aLength The number of fill characters to be appended.

@panic USER 11  if aLength is negative, or the resulting length of this
                descriptor is greater than its maximum length.
*/
	{

	TInt len=Length();
	TUint16 *pB=WPtr()+len;
	SetLength(len+aLength);
	TUint16 *pE=pB+aLength;
	while (pB<pE)
		*pB++=(TUint16)aChar;
	}

#ifndef __DES16_MACHINE_CODED_HWORD__
EXPORT_C void TDes16::ZeroTerminate()
/**
Appends a zero terminator onto the end of this descriptor's data.

The length of the descriptor is not changed. It must, however, be strictly less than 
the descriptor's maximum length. 
This condition guarantees that there is sufficient space for the zero terminator.

@panic USER 11  if the descriptor's length is not strictly less than its
                maximum length.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(len<MaxLength(),Panic(ETDes16Overflow));
	WPtr()[len]=0;
	}

EXPORT_C const TUint16 *TDes16::PtrZ()
/**
Appends a zero terminator onto the end of this descriptor's data and returns 
a pointer to the data.

The length of the descriptor is not changed. It must be strictly less than 
the descriptor's maximum length. 
This condition guarantees that there is sufficient space for the
zero terminator.

@return A pointer to the descriptor's zero terminated data.

@panic USER 11  if the descriptor's length is not strictly less than its
                maximum length.
*/
	{

	ZeroTerminate();
	return(Ptr());
	}
#endif

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TDes16::Zero()
/**
Sets the length of the data to zero.
*/
	{

	SetLength(0);
	}
#endif

#ifndef __DES16_MACHINE_CODED__
EXPORT_C void TDes16::FillZ()
/**
Fills the descriptor's data area with binary zeroes, i.e.0x0000, replacing any 
existing data.

The descriptor is filled from the beginning up to its current length. The 
descriptor's length does not change. It is not filled to its maximum length.
*/
	{

    memclr(WPtr(),Length()*2);
	}
#endif

EXPORT_C void TDes16::FillZ(TInt aLength)
/**
Fills the descriptor's data area with binary zeroes, i.e. 0x0000, replacing any 
existing data, and changes its length.

The descriptor is filled with the specified number of binary zeroes.
The descriptor's length is changed to reflect this.

@param aLength The new length of the descriptor and the number of binary zeroes
               to be copied into it. 
               
@panic USER 11  if aLength is negative, or is greater than the maximum length
                of this descriptor.
*/
	{

	SetLength(aLength);
    FillZ();
	}

EXPORT_C void TDes16::Fold()
/**
Performs folding on the content of this descriptor.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.
*/
	{

	TUint16 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TCharF c(*pB);
		*pB++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::Collate()
/**
Performs collation on the content of this descriptor.
*/
	{

	TUint16 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TChar c=User::Collate(*pB);
		*pB++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::LowerCase()
/**
Converts the content of this descriptor to lower case.

Conversion is implemented as appropriate to the current locale.
*/
	{

	TUint16 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TCharLC c(*pB);
		*pB++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::UpperCase()
/**
Converts the content of this descriptor to upper case.

Conversion is implemented as appropriate to the current locale.
*/
	{

	TUint16 *pB=WPtr();
	TInt len=Length();
	while (len--)
		{
		TCharUC c(*pB);
		*pB++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::Capitalize()
/**
Capitalises the content of this descriptor.

Capitalisation is implemented as appropriate to the current locale.
*/
	{

	TUint16 *pB=WPtr();
	TInt len=Length();
	if (len--)
		{
		*pB=(TUint16)User::TitleCase(*pB);
		++pB;
		while (len--)
			{
			*pB=(TUint16)User::LowerCase(*pB);
			++pB;
			}
		}
	}

EXPORT_C void TDes16::CopyF(const TDesC16 &aDes)
/**
Copies and folds data from the specified descriptor into this descriptor replacing 
any existing data.

The length of this descriptor is set to reflect the new 
data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	TUint16 *pT=WPtr();
	while (len--)
		{
		TCharF c(*pS++);
		*pT++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::CopyC(const TDesC16 &aDes)
/**
Copies and collates data from the specified descriptor
into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	TUint16 *pT=WPtr();
	while (len--)
		{
		TChar c=User::Collate(*pS++);
		*pT++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::CopyLC(const TDesC16 &aDes)
/**
Copies text from the specified descriptor and converts it to lower case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to lower case is implemented as appropriate to the current locale.

@param aDes A 16-bit non modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	TUint16 *pT=WPtr();
	while (len--)
		{
		TCharLC c(*pS++);
		*pT++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::CopyUC(const TDesC16 &aDes)
/**
Copies text from the specified descriptor and converts it to upper case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to upper case is implemented as appropriate to the current locale.

@param aDes A 16-bit non modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	TUint16 *pT=WPtr();
	while (len--)
		{
		TCharUC c(*pS++);
		*pT++=(TUint16)c;
		}
	}

EXPORT_C void TDes16::CopyCP(const TDesC16 &aDes)
/**
Copies text from the specified descriptor and capitalises it before putting 
it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Capitalisation is implemented as appropriate to the current locale.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.
*/
	{

	TInt len=aDes.Length();
	SetLength(len);
	const TUint16 *pS=aDes.Ptr();
	TUint16 *pT=WPtr();
	if (len--)
		{
		TChar c(*pS++);
#ifdef _UNICODE
		c.TitleCase();
#else
		c.UpperCase();
#endif
		*pT++=(TUint16)c;
		while (len--)
			{
			TCharLC c=(*pS++);
			*pT++=(TUint16)c;
			}
		}
	}

EXPORT_C void TDes16::Repeat(const TUint16 *aBuf,TInt aLength)
/**
Copies data with repetition into this descriptor, from a memory location
specified by pointer, replacing any existing data.

Copying proceeds until this descriptor is filled up to its current length. 
If it cannot contain a whole number of copies of the source data, then the 
last copy is truncated.

@param aBuf    A pointer to data to be repeatedly copied. 
@param aLength The length of data to be copied. 

@panic USER 17  if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(aBuf,ETDes16RepeatBufLength);
	TUint16 *pB=WPtr();
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

EXPORT_C void TDes16::Repeat(const TDesC16 &aDes)
/**
Copies data with repetition into this descriptor, from another descriptor,
replacing any existing data.

Copying proceeds until this descriptor is filled up to its current length. 
If it cannot contain a whole number of copies of the source data, then the 
last copy is truncated.

@param aDes A 16-bit non modifiable descriptor whose data is to be repeatedly 
            copied.
*/
	{

	Repeat(aDes.Ptr(),aDes.Length());
	}

EXPORT_C void TDes16::Trim()
/**
Deletes leading and trailing whitespace characters from the descriptor's data.

The length of the descriptor is reduced to reflect the loss of the whitespace characters.

@see TDes16::TrimLeft()
@see TDes16::TrimRight()
*/
	{

	TrimLeft();
	TrimRight();
	}

EXPORT_C void TDes16::TrimAll()
/**
Deletes leading and trailing whitespace characters from the descriptor's data 
and replaces each contiguous set of whitespace characters within the data by one 
whitespace character.

The length of the descriptor is reduced to reflect the loss of the whitespace
characters.

@see TDes16::Trim()
*/
	{

	TrimLeft();
	TrimRight();
	TUint16 *pBuf=(TUint16 *)Ptr();
	TUint16 *pSrc=pBuf;
	TUint16 *pDst=pBuf;
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

EXPORT_C void TDes16::TrimLeft()
/**
Deletes leading whitespace characters from the descriptor's data.

All whitespace characters up to, but not including the first
non-whitespace character, are deleted.

The length of the descriptor is reduced to reflect the loss
of the whitespace characters.
*/
	{

	const TUint16 *pBuf=Ptr();
	const TUint16 *pB=pBuf;
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

EXPORT_C void TDes16::TrimRight()
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
	const TUint16 *pB=Ptr()+len-1;
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

EXPORT_C void TDes16::Insert(TInt aPos,const TDesC16 &aDes)
/**
Inserts data into this descriptor.

The length of this descriptor is changed to reflect the extra data.

@param aPos The position within the data where insertion is to start. This 
            is an offset value; a zero value refers to the leftmost data
            position.
            
@param aDes A 16-bit non modifiable descriptor whose data is to be inserted.

@panic USER 10  if aPos is negative or is greater than the length of this
                descriptor.
@panic USER 11  if the resulting length of this descriptor is greater than its
                maximum length.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes16PosOutOfRange));
	TInt s=aDes.Length();
	__ASSERT_ALWAYS((len+s)<=MaxLength(),Panic(ETDes16Overflow));
	TUint16 *pB=WPtr();
	memCopy(pB+aPos+s,pB+aPos,len-aPos);
	memCopy(pB+aPos,aDes.Ptr(),aDes.Length());
	SetLength(len+s);
	}

EXPORT_C void TDes16::Delete(TInt aPos,TInt aLength)
/**
Deletes data from this descriptor.

The length of this descriptor is changed to reflect the loss of data.

@param aPos    The position within the data where deletion is to start. This 
               is an offset value; a zero value refers to the leftmost data
               position. 
            
@param aLength The length of data to be deleted. If necessary, the function 
               adjusts this value to ensure that no data beyond the end of
               the descriptor data area is deleted.

@panic USER 10  if aPos is negative or is greater than the length of this
                descriptor.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes16PosOutOfRange));
	TInt d=Min(len-aPos,aLength);
	TUint16 *pB=WPtr();
	memCopy(pB+aPos,pB+aPos+d,len-aPos-d);
	SetLength(len-d);
	}

EXPORT_C void TDes16::Replace(TInt aPos,TInt aLength,const TDesC16 &aDes)
/**
Replaces data in this descriptor.

The specified length can be different to the length of the replacement data.
The length of this descriptor changes to reflect the change of data.

@param aPos    The position within the data where replacement is to start. 
               This is an offset value; a zero value refers to the leftmost
               data position. 
            
@param aLength The length of data to be replaced.

@param aDes    The source 16-bit non modifiable descriptor whose data is to
               replace the target descriptor's data at aPos.

@panic USER  8  if aLength is negative or the sum of aLength and aPos is
                greater than the length of this descriptor.
               
@panic USER 10  if aPos is negative or is greater than the length of this
                descriptor.
                
@panic USER 11  if the resulting length of this descriptor is greater than its
                maximum length.
                
@panic USER 16  if the length of the source descriptor aDes is negative or is
                greater than the maximum length of this target descriptor,                
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes16PosOutOfRange));
	__ASSERT_ALWAYS(aLength>=0 && aPos+aLength<=len,Panic(ETDes16LengthOutOfRange));
	TInt s=aDes.Length();
	TInt maxlen=MaxLength();
	__ASSERT_ALWAYS(s>=0 && s<=maxlen,Panic(ETDes16RemoteLengthOutOfRange));
	__ASSERT_ALWAYS((len+s-aLength)<=maxlen,Panic(ETDes16Overflow));
	TUint16 *pB=WPtr();
	memCopy(pB+aPos+s,pB+aPos+aLength,len-aPos-aLength);
	memCopy(pB+aPos,aDes.Ptr(),s);
	SetLength(len+s-aLength);
	}

EXPORT_C void TDes16::Justify(const TDesC16 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Copies data into this descriptor and justifies it, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The target area is considered to be an area of specified width positioned at
the beginning of this descriptor's data area. Source data is copied into, and
aligned within this target area according to the specified alignment
instruction.

If the length of the target area is larger than the length of the source, then
spare space within the target area is padded with the fill character.

@param aDes        A 16-bit non-modifiable descriptor containing the source data.
                   The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and 
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth).

@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area

@param aFill       The fill character used to pad the target area. 

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    Zero();
    AppendJustify(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
    }

EXPORT_C void TDes16::AppendJustify(const TDesC16 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends data onto the end of this descriptor's data and justifies it.
	
The source of the appended data is an existing descriptor.
	
The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within this target area according to the specified alignment instruction.
	
If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.
		
@param aDes        A 16-bit non-modifiable descriptor containing the source
                   data. The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth). 
	
@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
	               re-set to the length of the data source.
	
@param anAlignment The alignment of the data within the target area. 
	
@param aFill       The fill character used to pad the target area.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    AppendJustify(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes16::AppendJustify(const TDesC16 &aDes,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill)
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

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

    AppendJustify(aDes.Ptr(),aLength,aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes16::AppendJustify(const TUint16 *aString,TInt aWidth,TAlign anAlignment,TChar aFill)
/**
Appends a zero terminated string onto the end of this descriptor's data and 
justifies it.

The zero terminator is not copied.

The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within, this target area according to the specified alignment instruction.

If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.

@param aString     A pointer to a zero terminated string The length of the data 
                   to be copied is the smaller of: the length of the string (excluding the zero 
                   terminator), the width of the target area (only if this is not the explicit 
                   negative value KDefaultJustifyWidth). 
                    
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is re-set to the length of the 
                   zero terminated string (excluding the zero terminator).
                    
@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
*/
	{

 	__CHECK_ALIGNMENT(aString,ETDes16AppendJustify1);
	AppendJustify(aString,STRING_LENGTH_16(aString),aWidth,anAlignment,aFill);
    } 

EXPORT_C void TDes16::AppendJustify(const TUint16 *aString,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill)
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
               
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source. 
               
@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.
                
@panic USER 17  if aLength is negative.  
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(aString,ETDes16AppendJustify2);
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
	memCopy(WPtr()+offset+r,aString,aLength);
	}

EXPORT_C void TDes16::NumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth)
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
*/
	{

	Zero();
	AppendNumFixedWidth(aVal,aRadix,aWidth);
	}

EXPORT_C void TDes16::NumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth)
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
@param aWidth The number of characters: to be used to contain the conversion, 
              to be copied into this descriptor.
              
@see TDes16::Format()
*/
	{

    Zero();
    AppendNumFixedWidthUC(aVal,aRadix,aWidth);
    }

EXPORT_C void TDes16::Num(TInt64 aVal)
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

EXPORT_C void TDes16::Num(TUint64 aVal, TRadix aRadix)
/**
Converts the specified 64 bit unsigned integer into a character representation 
based on the specified number system and copies the conversion into this
descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.
	
When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.
	
@param aVal   The 64 bit integer value. This is treated as an unsigned
              value for all builds. 
@param aRadix The number system representation for the 64 bit integer.
*/
	{

	Zero();
	AppendNum(aVal,aRadix);
	}

EXPORT_C void TDes16::NumUC(TUint64 aVal, TRadix aRadix)
/**
Converts the specified 64 bit unsigned integer into a character representation 
based on the specified number system and copies the conversion into this
descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.

@param aVal   The 64 bit integer value. This is always treated as an unsigned
              value for all builds. 
@param aRadix The number system representation for the 64 bit integer. If no 
              explicit value is specified, then EDecimal is the default.
*/
	{

	Zero();
	AppendNumUC(aVal,aRadix);
	}

void TDes16::DoPadAppendNum(TInt l, TInt aW, const TUint8* p)
	{
	__ASSERT_DEBUG( ((l|(TInt)p)&1)==0, Panic(EDes16PadAppendBadAlign));
	l>>=1;
	if (aW<=0)
		{
		Append((const TUint16*)p, l);
		return;
		}
	TInt l0 = Length();
	SetLength(l0 + aW);
	TUint16* d = WPtr() + l0;
	for (; aW>l; --aW) *d++ = (TUint16)'0';
	memcpy(d, p, aW*2);
	}

EXPORT_C void TDes16::AppendNumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth)
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

	TBuf16<32> buf;
	buf.Num(aVal,aRadix);
	if (buf.Length()>=aWidth)
		Append(buf.Left(aWidth));
	else
		{
		TInt i=aWidth-buf.Length();
		while(i--)
			Append(TChar('0'));
		Append(buf);
		}
	}

#ifndef __DES16_MACHINE_CODED__
EXPORT_C TPtr16 TDes16::LeftTPtr(TInt aLength) const
/**
Extracts the leftmost part of the data. 

The function does not cut or remove any data but constructs a modifiable pointer
descriptor to represent the leftmost part of the data.

@param aLength The length of the data to be extracted. If this value is 
               greater than the length of the descriptor, the function 
               extracts the whole of the descriptor.
               
@return The 16-bit modifiable pointer descriptor representing the leftmost part
		of the data.

@panic USER 10  if aLength is negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16PosOutOfRange));
	TInt len = Min(aLength,Length());
	return(TPtr16((TUint16*)Ptr(),len,len));
	}

EXPORT_C TPtr16 TDes16::RightTPtr(TInt aLength) const
/**
Extracts the rightmost part of the data.

The function does not cut or remove any data but constructs a modifiable pointer
descriptor to represent the rightmost part of the data.

@param aLength The length of data to be extracted. If this value
               is greater than the length of the descriptor, the function
               extracts the whole of the descriptor. 
               
@return The 16-bit modifiable pointer descriptor representing the rightmost part
		of the data.

@panic USER 10  if aLength is negative.
*/
	{

	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16PosOutOfRange));
	TInt len=Length();
	if (aLength>len)
		aLength=len;
	return(TPtr16((TUint16*)Ptr()+len-aLength,aLength,aLength));
	}

EXPORT_C TPtr16 TDes16::MidTPtr(TInt aPos) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a modifiable pointer
descriptor to represent the defined portion.

The portion is identified by its starting position and by the length of the 
remainder of the data starting from the specified position.

@param aPos The starting position of the data to be extracted. This is an 
            offset value; a zero value refers to the leftmost data position.
             
@return The 16-bit modifiable pointer descriptor representing the specified 
        portion of the data.

@panic USER 10  if aPos is negative or aPos is greater than the
                length of the descriptor.
*/
	{

	TInt len=Length();
	__ASSERT_ALWAYS(aPos>=0 && aPos<=len,Panic(ETDes16PosOutOfRange));
	return(TPtr16((TUint16*)Ptr()+aPos,len-aPos,len-aPos));
	}

EXPORT_C TPtr16 TDes16::MidTPtr(TInt aPos,TInt aLength) const
/**
Extracts a portion of the data.

The function does not cut or remove any data but constructs a modifiable pointer
descriptor to represent the defined portion.

The portion is identified by its starting position and by its length.

@param aPos    The starting position of the data to be extracted. This is an 
               offset value; a zero value refers to the leftmost data position. 
@param aLength The length of data to be extracted.

@return The 16-bit modifiable pointer descriptor representing the specified
		portion of the data.
        
@panic USER 10  if aPos is negative or aPos plus aLength is greater than the
                length of the descriptor.
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && (aPos+aLength)<=Length(),Panic(ETDes16PosOutOfRange));
	return(TPtr16((TUint16*)Ptr()+aPos,aLength,aLength));
	}
#endif

EXPORT_C void TDes16::AppendNumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth)
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
@param aWidth The number of characters: to be used to contain the conversion, 
              to be appended to this descriptor.
*/
	{

	TBuf16<32> buf;
	buf.NumUC(aVal,aRadix);
	if (buf.Length()>=aWidth)
		Append(buf.Left(aWidth));
	else
		{
		TInt i=aWidth-buf.Length();
		while(i--)
			Append(TChar('0'));
		Append(buf);
		}
	}

EXPORT_C void TDes16::AppendNum(TInt64 aVal)
/**
Converts the 64-bit signed integer into a decimal character representation 
and appends the conversion onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.

If the integer is negative, the character representation is prefixed by a 
minus sign.

@param aVal The 64-bit signed integer value.
*/
	{

	if (aVal<0)
		{
		Append('-');
		aVal=-aVal;
		}
	AppendNum(static_cast<TUint64>(aVal), EDecimal);
	}

void TDes16::DoAppendNum(TUint64 aVal, TRadix aRadix, TUint aA, TInt aW)
//
// Convert a TUint64 into the descriptor.
//
	{

	TUint16 buf[APPEND_BUF_SIZE_64];
	TUint8* p = (TUint8*)(buf + APPEND_BUF_SIZE_64);
	TInt l = __DoConvertNum(aVal, aRadix, aA|256, p);
	// coverity[overrun-local]
	DoPadAppendNum(l, aW, p);
	}

EXPORT_C void TDes16::AppendNum(TUint64 aVal, TRadix aRadix)
/**
Converts the specified 64 bit integer into a character representation 
based on the specified number system and appends the conversion onto the end 
of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.
	
When a hexadecimal conversion is specified, hexadecimal characters are in 
lower case.
	
@param aVal   The 64 bit integer value. This is always treated as an unsigned
              value. 
@param aRadix The number system representation for the 64 bit integer.
*/
	{
	DoAppendNum(aVal, aRadix, 'a', 0);
	}

EXPORT_C void TDes16::AppendNumUC(TUint64 aVal,TRadix aRadix)
/**
Converts the specified 64 bit integer into a character representation 
based on the specified number system and appends the conversion onto the end 
of this descriptor's data.

The length of this descriptor is incremented to reflect the new content.
	
When a hexadecimal conversion is specified, hexadecimal characters are in 
upper case.
	
@param aVal   The 64 bit integer value. This is always treated as an unsigned
              value. 
@param aRadix The number system representation for the 64 bit integer. If no 
              explicit value is specified, then EDecimal is the default.
*/

	{
	DoAppendNum(aVal, aRadix, 'A', 0);
	}

EXPORT_C void TDes16::Format(TRefByValue<const TDesC16> aFmt,...)
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

@panic USER 11  if the  resulting length of text in this descriptor exceeds
                the descriptor's maximum length.
@panic USER 12  if the format string has incorrect syntax.

@see TDes16::Num()
@see TDes16::NumUC()
*/
	{

    VA_LIST list;
    VA_START(list,aFmt);
	// coverity[uninit_use_in_call]
    FormatList(aFmt,list);
    }

EXPORT_C void TDes16::FormatList(const TDesC16 &aFmt,VA_LIST aList)
/**
Formats and copies text into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The behaviour of this function is the same as Format(). In practice, it is 
better and easier to use Format(), passing a variable number of arguments 
as required by the format string.

@param aFmt  The descriptor containing the format string.
@param aList A pointer to an argument list.

@see TDes16::Format()
@see VA_LIST
*/
	{

	Zero();
	AppendFormatList(aFmt,aList);
	}

EXPORT_C void TDes16::AppendFormat(TRefByValue<const TDesC16> aFmt,TDes16Overflow *aOverflowHandler,...)
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
2  raises a USER 11 panic, if no overflow handler is supplied.

As much literal text as possible will have been copied into this descriptor
and this descriptor will have reached its maximum length.

Text converted from a trailing argument is appended as a complete string.
If an attempt to append this string fails because the resulting length
of this descriptor would exceed its maximum length, then the function:

1. calls the Overflow() member function of the overflow handler, if an overflow
   handler is supplied
   
2  raises a USER 11 panic, if no overflow handler is supplied.
  
None of the generated text is appended and length of this descriptor
may be less than the maximum.

@param aFmt             The 16-bit non-modifiable descriptor containing the
                        format string. The TRefByValue class provides a
                        constructor which takes a TDesC16 type. 

@param aOverflowHandler A pointer to the overflow handler. 

@param ...              A variable number of arguments to be converted to text
                        as dictated by the format string. 

@panic USER 11  if the length of the descriptor exceeds its maximum length and
                no overflow handler has been supplied.
@panic USER 12  if the format string has incorrect syntax.

@see TDes16::Format()
@see TDes16Overflow::Overflow()
*/
	{

	VA_LIST list;
	VA_START(list, aOverflowHandler);
	// coverity[uninit_use_in_call]
	AppendFormatList(aFmt,list,aOverflowHandler);
	}

EXPORT_C void TDes16::AppendFormat(TRefByValue<const TDesC16> aFmt,...)
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

@param aFmt The 16-bit non-modifiable descriptor containing the
            format string. The TRefByValue class provides a
            constructor which takes a TDesC16 type. 

@param ...  A variable number of arguments to be converted to text
            as dictated by the format string. 

@panic USER 11  if the  resulting length of text in this descriptor exceeds
                the descriptor's maximum length.
@panic USER 12  if the format string has incorrect syntax.

@see TDes16::Format()
*/
	{

    VA_LIST list;
    VA_START(list,aFmt);
    AppendFormatList(aFmt,list);
    }

EXPORT_C void TDes16::Append2(TChar aChar)
/**
The surrogate aware version of Append().

Appends data onto the end of this descriptor's data.

The length of this descriptor is incremented to reflect the new content. The
length will be increased by 1 if aChar is inside BMP or 2 if aChar is outside
BMP.

@param aChar A single character to be appended. Can be inside or outside BMP.

@panic USER 11  if the resulting new length of this descriptor is greater than
                its maximum length.

@panic USER 217 if corrupt surrogate found in aChar. This functions will not
                validate already existing surrogate in the descriptor.

@see TDes16::Append()
*/
	{
	__ASSERT_ALWAYS(TChar::IsSupplementary(aChar) || !TChar::IsSurrogate((TText16)aChar), Panic(ECorruptSurrogateFound));

	TInt len = Length();
	TUint16 *pB = WPtr() + len;
	if (TChar::IsSupplementary(aChar))
		{
		SetLength(len + 2);
		*pB++ = TChar::GetHighSurrogate(aChar);
		*pB = TChar::GetLowSurrogate(aChar);
		}
	else
		{
		SetLength(len + 1);
		*pB = (TText16)aChar;
		}
	}

EXPORT_C void TDes16::Fill2(TChar aChar)
/**
The surrogate aware version of Fill().

Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled from the beginning up to its current length. The 
descriptor's length does not change. It is not filled to its maximum length.
If aChar is supplementary character, and available space to fill is odd in
16-bit unit, then the last 16-bit unit will be filled with high surrogate, 
and the length will keep unchanged.

@param aChar The fill character. Can be inside or outside BMP.

@see TDes16::Fill()
*/
	{
	TUint16 *pB = WPtr();
	TUint16 *pE = pB + Length();
	if (!TChar::IsSupplementary(aChar))
		{
		while (pB < pE)
			*pB++ = (TUint16)aChar;
		}
	else
		{
		while (pB < pE - 1)
			{
			*pB++ = TChar::GetHighSurrogate(aChar);
			*pB++ = TChar::GetLowSurrogate(aChar);
			}
		// fill the last 16-bit unit
		if (pB < pE)
		    *pB++ = TChar::GetHighSurrogate(aChar);
		}
	}

EXPORT_C void TDes16::Fill2(TChar aChar, TInt aLength)
/**
The surrogate aware version of Fill().

Fills the descriptor's data area with the specified character, replacing any 
existing data.

The descriptor is filled with the specified number of characters,
and its length is changed to reflect this.

If aChar is supplementary character, and available space to fill is odd in
16-bit unit, then the last 16-bit unit will be left unchanged.

@param aChar   The fill character. Can be inside or outside BMP.
@param aLength The new length of the descriptor.

@panic USER 11  if aLength is negative or is greater than the maximum length
                of this descriptor.

@panic USER 217 if corrupt surrogate found in aChar. These functions will not 
                validate already existing surrogate in the descriptor.

@see TDes16::Fill()
*/
	{
	__ASSERT_ALWAYS(TChar::IsSupplementary(aChar) || !TChar::IsSurrogate((TText16)aChar), Panic(ECorruptSurrogateFound));

	SetLength(aLength);
	Fill2(aChar);
	}

EXPORT_C void TDes16::AppendFill2(TChar aChar, TInt aLength)
/**
The surrogate aware version of AppendFill().

Appends and fills this descriptor with the specified character.

The descriptor is appended with the specified number of characters, and its
length is changed to reflect this.

If aChar is supplementary character, and available space to fill is odd in 
16-bit unit, then the last 16-bit unit will be filled with high surrogate.

@param aChar   The fill character. Can be inside or outside BMP.
@param aLength The length of additional space to append into.

@panic USER 11  if aLength is negative, or the resulting length of this
                descriptor is greater than its maximum length.

@panic USER 217 if corrupt surrogate found in aChar. These functions will not 
                validate already existing surrogate in the descriptor.

@see TDes16::AppendFill()
*/
	{
	__ASSERT_ALWAYS(TChar::IsSupplementary(aChar) || !TChar::IsSurrogate((TText16)aChar), Panic(ECorruptSurrogateFound));

	TInt len=Length();
	TUint16 *pB=WPtr()+len;
	SetLength(len+aLength);
	TUint16 *pE=pB+aLength;
	if (!TChar::IsSupplementary(aChar))
		{
		while (pB < pE)
			*pB++ = (TUint16)aChar;
		}
	else
		{
		while (pB < pE - 1)
			{
			*pB++ = TChar::GetHighSurrogate(aChar);
			*pB++ = TChar::GetLowSurrogate(aChar);
			}
        // fill the last 16-bit unit
        if (pB < pE)
            *pB++ = TChar::GetHighSurrogate(aChar);
		}
	}

EXPORT_C void TDes16::Justify2(const TDesC16 &aDes, TInt aWidth, TAlign anAlignment, TChar aFill)
/**
The surrogate aware version of Justify().

Copies data into this descriptor and justifies it, replacing any existing data.

The length of this descriptor is set to reflect the new data.

The target area is considered to be an area of specified width positioned at
the beginning of this descriptor's data area. Source data is copied into, and
aligned within this target area according to the specified alignment
instruction.

If the length of the target area is larger than the length of the source, then
spare space within the target area is padded with the fill character.

@param aDes        A 16-bit non-modifiable descriptor containing the source data.
                   The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and 
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth).

@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area

@param aFill       The fill character used to pad the target area. Can be
                   inside or outside BMP.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.

@panic USER 217 if corrupt surrogate found in the parameters or in the 
                descriptor.

@see TDes16::Justify()
*/
	{
    Zero();
    AppendJustify2(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
	}

EXPORT_C void TDes16::AppendJustify2(const TDesC16 &aDes, TInt aWidth, TAlign anAlignment, TChar aFill)
/**
The surrogate aware version of AppendJustify.

Appends data onto the end of this descriptor's data and justifies it.
    
The source of the appended data is an existing descriptor.
    
The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within this target area according to the specified alignment instruction.
    
If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.
        
@param aDes        A 16-bit non-modifiable descriptor containing the source
                   data. The length of the data to be copied is the smaller of:
                   the length of the source descriptor, and
                   the width of the target area (only if this is not the
                   explicit negative value KDefaultJustifyWidth). 
    
@param aWidth      The width of the target area. If this has the specific
                   negative value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.
    
@param anAlignment The alignment of the data within the target area. 
    
@param aFill       The fill character used to pad the target area. Can be
                   inside or outside BMP.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.

@panic USER 217 if corrupt surrogate found in the parameters or in the 
                descriptor.

@see TDes16::AppendJustify()
*/
	{
    AppendJustify2(aDes.Ptr(),aDes.Length(),aWidth,anAlignment,aFill);
	}

EXPORT_C void TDes16::AppendJustify2(const TDesC16 &aDes, TInt aLength, TInt aWidth, TAlign anAlignment, TChar aFill)
/**
The surrogate aware version of AppendJustify.

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
                   The length of data to be copied must not be  greater than
                   the length of the source descriptor. Note that this
                   condition is not automatically tested. 
                   
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source.

@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area. Can be
                   inside or outside BMP.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.

@panic USER 217 if corrupt surrogate found in the parameters or in the 
                descriptor.

@see TDes16::AppendJustify()
*/
	{
    AppendJustify2(aDes.Ptr(),aLength,aWidth,anAlignment,aFill);
	}

EXPORT_C void TDes16::AppendJustify2(const TUint16 *aString, TInt aWidth, TAlign anAlignment, TChar aFill)
/**
The surrogate aware version of AppendJustify.

Appends a zero terminated string onto the end of this descriptor's data and 
justifies it.

The zero terminator is not copied.

The target area is considered to be an area of specified width, immediately 
following this descriptor's existing data. Source data is copied into, and 
aligned within, this target area according to the specified alignment instruction.

If the length of the target area is larger than the length of the source, 
then spare space within the target area is padded with the fill character.

@param aString     A pointer to a zero terminated string The length of the data 
                   to be copied is the smaller of: the length of the string (excluding the zero 
                   terminator), the width of the target area (only if this is not the explicit 
                   negative value KDefaultJustifyWidth). 
                    
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is re-set to the length of the 
                   zero terminated string (excluding the zero terminator).
                    
@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area. Can be
                   inside or outside BMP.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.

@panic USER 217 if corrupt surrogate found in the parameters or in the 
                descriptor.

@see TDes16::AppendJustify()
*/
	{
 	__CHECK_ALIGNMENT(aString,ETDes16AppendJustify1);
	AppendJustify2(aString,STRING_LENGTH_16(aString),aWidth,anAlignment,aFill);
	}

EXPORT_C void TDes16::AppendJustify2(const TUint16 *aString, TInt aLength, TInt aWidth, TAlign anAlignment, TChar aFill)
/**
The surrogate aware version of AppendJustify.

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
               
@param aWidth      The width of the target area. If this has the specific negative 
                   value KDefaultJustifyWidth, then the width is
                   re-set to the length of the data source. 
               
@param anAlignment The alignment of the data within the target area. 

@param aFill       The fill character used to pad the target area. Can be
                   inside or outside BMP.

@panic USER 11  if the resulting length of this descriptor is greater than
                its maximum length or aWidth has a negative value other 
                than KDefaultJustifyWidth.

@panic USER 17  if aLength is negative.
  
@panic USER 217 if corrupt surrogate found in the parameters or in the 
                descriptor.

@see TDes16::AppendJustify()
*/
	{
	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(aString,ETDes16AppendJustify2);
	if (aWidth==KDefaultJustifyWidth)
		aWidth=aLength;
	if (aLength>aWidth)
		aLength=aWidth;
	TInt offset=Length();
	AppendFill2(aFill,aWidth);
	TInt r=aWidth-aLength;
	if (anAlignment==ECenter)
		r>>=1;
	else if (anAlignment==ELeft)
		r=0;
	memCopy(WPtr()+offset+r,aString,aLength);
	}

EXPORT_C void TDes16::Fold2()
/**
The surrogate aware version of Fold().

Performs folding on the content of this descriptor.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.

@panic USER 217 if corrupt surrogate found in the descriptor.

@see TDes16::Fold()
*/
	{
	TInt strLength = Length();
	TText16* start = WPtr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		TCharF c(currentChar);
		// at present, c and currentChar always in the same plane
		if (TChar::IsSupplementary(c))
			{
			start[int16Index] = TChar::GetHighSurrogate(c);
			start[int16Index+1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			start[int16Index] = (TText16)c;
			}
		int16Index = (next - start);
		}
	}

EXPORT_C void TDes16::Collate2()
/**
The surrogate aware version of Collate().

Performs collation on the content of this descriptor.

@panic USER 217 if corrupt surrogate found in the descriptor.

@see TDes16::Collate()
*/
	{
	TInt strLength = Length();
	TText16* start = WPtr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		TChar c = User::Collate(currentChar);
		// at present, c and currentChar always in the same plane
		if (TChar::IsSupplementary(c))
			{
			start[int16Index] = TChar::GetHighSurrogate(c);
			start[int16Index+1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			start[int16Index] = (TText16)c;
			}
		int16Index = (next - start);
		}
	}

EXPORT_C void TDes16::LowerCase2()
/**
The surrogate aware version of LowerCase().

Converts the content of this descriptor to lower case.

Conversion is implemented as appropriate to the current locale.

@panic USER 217 if corrupt surrogate found in the descriptor.

@see TDes16::LowerCase()
*/
	{
	TInt strLength = Length();
	TText16* start = WPtr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		TCharLC c(currentChar);
		// at present, c and currentChar always in the same plane
		if (TChar::IsSupplementary(c))
			{
			start[int16Index] = TChar::GetHighSurrogate(c);
			start[int16Index+1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			start[int16Index] = (TText16)c;
			}
		int16Index = (next - start);
		}
	}

EXPORT_C void TDes16::UpperCase2()
/**
The surrogate aware version of UpperCase().

Converts the content of this descriptor to upper case.

Conversion is implemented as appropriate to the current locale.

@panic USER 217 if corrupt surrogate found in the descriptor.

@see TDes16::UpperCase()
*/
	{
	TInt strLength = Length();
	TText16* start = WPtr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		TCharUC c(currentChar);
		// at present, c and currentChar always in the same plane
		if (TChar::IsSupplementary(c))
			{
			start[int16Index] = TChar::GetHighSurrogate(c);
			start[int16Index+1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			start[int16Index] = (TText16)c;
			}
		int16Index = (next - start);
		}
	}

EXPORT_C void TDes16::Capitalize2()
/**
The surrogate aware version of Capitalize().

Capitalises the content of this descriptor.

Capitalisation is implemented as appropriate to the current locale.

@panic USER 217 if corrupt surrogate found in the descriptor.

@see TDes16::Capitalize()
*/
	{
	TInt strLength = Length();
	TText16* start = WPtr();
	const TText16* end = Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	
	// the first character: title case
	status = ::ProceedOneCharacter(start, end, next, currentChar);
	__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
	TChar c = User::TitleCase(currentChar);
	// at present, c and currentChar always in the same plane
	if (TChar::IsSupplementary(c))
		{
		start[0] = TChar::GetHighSurrogate(c);
		start[1] = TChar::GetLowSurrogate(c);
		}
	else
		{
		start[0] = (TText16)c;
		}
	int16Index = (next - start);
	
	// following characters: lower case
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		TChar c = User::LowerCase(currentChar);
		// at present, c and currentChar always in the same plane
		if (TChar::IsSupplementary(c))
			{
			start[int16Index] = TChar::GetHighSurrogate(c);
			start[int16Index+1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			start[int16Index] = (TText16)c;
			}
		int16Index = (next - start);
		}
	}

EXPORT_C void TDes16::CopyF2(const TDesC16 &aDes)
/**
The surrogate aware version of CopyF().

Copies and folds data from the specified descriptor into this descriptor replacing 
any existing data.

The length of this descriptor is set to reflect the new 
data.

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used when dealing with strings in natural
language.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.

@panic USER 217 if corrupt surrogate found in aDes or in the descriptor.

@see TDes16::CopyF()
*/
	{
	TText16* pT = WPtr();
	TInt len = 0;
	const TInt maxLen = MaxLength();
	
	// iterate through aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		int16Index = (next - start);
		TCharF c(currentChar);
		if (TChar::IsSupplementary(c))
			{
			len += 2;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-2] = TChar::GetHighSurrogate(c);
			pT[len-1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			++len;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-1] = (TText16)c;
			}
		}
	SetLength(len);
	}

EXPORT_C void TDes16::CopyC2(const TDesC16 &aDes)
/**
The surrogate aware version of CopyC().

Copies and collates data from the specified descriptor
into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.

@panic USER 217 if corrupt surrogate found in aDes or in the descriptor.

@see TDes16::CopyC()
*/
	{
	TText16* pT = WPtr();
	TInt len = 0;
	const TInt maxLen = MaxLength();
	
	// iterate through aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		int16Index = (next - start);
		TChar c = User::Collate(currentChar);
		if (TChar::IsSupplementary(c))
			{
			len += 2;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-2] = TChar::GetHighSurrogate(c);
			pT[len-1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			++len;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-1] = (TText16)c;
			}
		}
	SetLength(len);
	}

EXPORT_C void TDes16::CopyLC2(const TDesC16 &aDes)
/**
The surrogate aware version of CopyLC().

Copies text from the specified descriptor and converts it to lower case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to lower case is implemented as appropriate to the current locale.

@param aDes A 16-bit non modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.

@panic USER 217 if corrupt surrogate found in aDes or in the descriptor.

@see TDes16::CopyLC()
*/
	{
	TText16* pT = WPtr();
	TInt len = 0;
	const TInt maxLen = MaxLength();
	
	// iterate through aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		int16Index = (next - start);
		TCharLC c(currentChar);
		if (TChar::IsSupplementary(c))
			{
			len += 2;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-2] = TChar::GetHighSurrogate(c);
			pT[len-1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			++len;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-1] = (TText16)c;
			}
		}
	SetLength(len);
	}

EXPORT_C void TDes16::CopyUC2(const TDesC16 &aDes)
/**
The surrogate aware version of CopyUC().

Copies text from the specified descriptor and converts it to upper case before 
putting it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Conversion to upper case is implemented as appropriate to the current locale.

@param aDes A 16-bit non modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.

@panic USER 217 if corrupt surrogate found in aDes or in the descriptor.

@see TDes16::CopyUC()
*/
	{
	TText16* pT = WPtr();
	TInt len = 0;
	const TInt maxLen = MaxLength();
	
	// iterate through aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		int16Index = (next - start);
		TCharUC c(currentChar);
		if (TChar::IsSupplementary(c))
			{
			len += 2;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-2] = TChar::GetHighSurrogate(c);
			pT[len-1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			++len;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-1] = (TText16)c;
			}
		}
	SetLength(len);
	}

EXPORT_C void TDes16::CopyCP2(const TDesC16 &aDes)
/**
The surrogate aware version of CopyCP().

Copies text from the specified descriptor and capitalises it before putting 
it into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

Capitalisation is implemented as appropriate to the current locale.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11  if the length of aDes is greater than the maximum length of
                this target descriptor.

@panic USER 217 if corrupt surrogate found in aDes or in the descriptor.

@see TDes16::CopyCP()
*/
	{
	TText16* pT = WPtr();
	TInt len = 0;
	const TInt maxLen = MaxLength();
	
	// iterate through aDes
	TInt strLength = aDes.Length();
	const TText16* start = aDes.Ptr();
	const TText16* end = aDes.Ptr() + strLength;
	TText16* next;
	TUint currentChar;
	TInt int16Index = 0;
	TInt status = KErrNone;
	
	// first character: title case
	status = ::ProceedOneCharacter(start, end, next, currentChar);
	__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
	int16Index = (next - start);
	TChar c(currentChar);
	c.TitleCase();
	if (TChar::IsSupplementary(c))
		{
		len += 2;
		__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
		pT[len-2] = TChar::GetHighSurrogate(c);
		pT[len-1] = TChar::GetLowSurrogate(c);
		}
	else
		{
		++len;
		__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
		pT[len-1] = (TText16)c;
		}
	
	// following characters: lower case
	FOREVER
		{
		status = ::ProceedOneCharacter(start+int16Index, end, next, currentChar);
		__ASSERT_ALWAYS(status != KErrCorruptSurrogateFound, Panic(ECorruptSurrogateFound));
		if (status == KErrNotFound)
			break;
		int16Index = (next - start);
		TCharLC c(currentChar);
		if (TChar::IsSupplementary(c))
			{
			len += 2;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-2] = TChar::GetHighSurrogate(c);
			pT[len-1] = TChar::GetLowSurrogate(c);
			}
		else
			{
			++len;
			__ASSERT_ALWAYS(len<=maxLen, Panic(ETDes16Overflow));
			pT[len-1] = (TText16)c;
			}
		}
	SetLength(len);
	}


#if !defined(__DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TPtrC16::TPtrC16()
	: TDesC16(EPtrC,0),iPtr(0)
/**
Constructs an empty 16-bit non-modifiable pointer descriptor.

It represents no data and its length is zero.

The non-modifiable pointer descriptor can, subsequently, be set to represent 
data.

@see TPtrC16::Set()
*/
	{}

EXPORT_C TPtrC16::TPtrC16(const TDesC16 &aDes)
	: TDesC16(EPtrC,aDes.Length()),iPtr(aDes.Ptr())
/** 
Constructs the 16-bit non-modifiable pointer descriptor from any existing
descriptor.

It is set to point to the same data and is given the same length as the source
descriptor.

@param aDes A reference to a 16-bit non-modifiable descriptor.
*/
	{}
#endif

#if !defined(__DES16_MACHINE_CODED_HWORD__) | defined(__EABI_CTORS__)
EXPORT_C TPtrC16::TPtrC16(const TUint16 *aString)
	: TDesC16(EPtrC,STRING_LENGTH_16(aString)),iPtr(aString)
/**
Constructs the 16-bit non-modifiable pointer descriptor to point to a zero 
terminated string, whether in RAM or ROM.

The length of the descriptor is set to the length of the zero terminated string, 
excluding the zero terminator.

@param aString A pointer to a zero terminated string.
*/
	{
	__CHECK_ALIGNMENT(aString,ETDesC16ConstructCString);
	}
#endif

#if !defined(__DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TPtrC16::TPtrC16(const TUint16 *aBuf,TInt aLength)
	: TDesC16(EPtrC,aLength),iPtr(aBuf)
/**
Constructs the 16-bit non-modifiable pointer descriptor to point to the 
specified location in memory, whether in RAM or ROM.

The length of the descriptor is set to the specified length.

@param aBuf    A pointer to the location that the descriptor is to represent.
@param aLength The length of the descriptor.This value must be non-negative.

@panic USER 17  if aLength is negative.
*/
	{
	__ASSERT_ALWAYS(aLength>=0,Panic(ETDes16LengthNegative));
	__CHECK_ALIGNMENT(aBuf,ETDesC16ConstructBufLength);
	}

EXPORT_C TPtr16::TPtr16(TUint16 *aBuf,TInt aMaxLength)
	: TDes16(EPtr,0,aMaxLength),iPtr(aBuf)
/**
Constructs the 16-bit modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor is set to zero, and its maximum length is set to
the specified value.

@param aBuf       A pointer to the location that the descriptor is to represent.
@param aMaxLength The maximum length of the descriptor.

@panic USER 18  if aMaxLength is negative.
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	__CHECK_ALIGNMENT(aBuf,ETDesC16ConstructBufLengthMax);
	}

EXPORT_C TPtr16::TPtr16(TUint16 *aBuf,TInt aLength,TInt aMaxLength)
	: TDes16(EPtr,aLength,aMaxLength),iPtr(aBuf)
/**
Constructs the 16-bit modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor and its maximum length are set to the specified
values.

@param aBuf       A pointer to the location that the descriptor is to represent.
@param aLength    The length of the descriptor. 
@param aMaxLength The maximum length of the descriptor.

@panic USER 8   if aLength is negative, or is greater than the descriptor's 
                maximum length,
                
@panic USER 18  if aMaxLength is negative.
*/
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	__ASSERT_ALWAYS(TUint(aLength)<=TUint(aMaxLength),Panic(ETDes16LengthOutOfRange));
	__CHECK_ALIGNMENT(aBuf,ETDesC16ConstructBufLengthMax);
	}

EXPORT_C TBufBase16::TBufBase16(TInt aMaxLength)
	:TDes16(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	}

EXPORT_C TBufBase16::TBufBase16(TInt aLength,TInt aMaxLength)
	:TDes16(EBuf,aLength,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	__ASSERT_ALWAYS(TUint(aLength)<=TUint(aMaxLength),Panic(ETDes16LengthOutOfRange));
	}
#endif

#if !defined( __DES16_MACHINE_CODED_HWORD__) | defined(__EABI_CTORS__)
EXPORT_C TBufBase16::TBufBase16(const TUint16* aString,TInt aMaxLength)
	:TDes16(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	Copy(aString);
	}
#endif

#if !defined( __DES16_MACHINE_CODED__) | defined(__EABI_CTORS__)
EXPORT_C TBufBase16::TBufBase16(const TDesC16& aDes,TInt aMaxLength)
	:TDes16(EBuf,0,aMaxLength)
	{
	__ASSERT_DEBUG(aMaxLength>=0,Panic(ETDes16MaxLengthNegative));
	Copy(aDes);
	}
#endif

EXPORT_C void TDesC16::__DbgTestInvariant() const
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
	if (Ptr() != NULL) // TPtr and TPtrC can be null
 		__CHECK_ALIGNMENT(Ptr(),ETDesC16Invariant);
#endif
    }

EXPORT_C void TPtrC16::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC16::__DbgTestInvariant(); // Test base class
	if (Type()!=EPtrC)
		User::Invariant();
#endif
	}

EXPORT_C void TDes16::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC16::__DbgTestInvariant(); // Test base class
	if (Length()>MaxLength() || !(Type()==EPtr || Type()==EBufCPtr || Type()==EBuf))
		User::Invariant();
#endif
	}

EXPORT_C void HBufC16::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDesC16::__DbgTestInvariant(); // Test base class
	if (Length()>(TInt)(STD_CLASS::AllocLen(this)-sizeof(TDesC16)) || Type()!=EBufC)
		User::Invariant();
#endif
	}

EXPORT_C void TPtr16::__DbgTestInvariant() const
//
// Test that the class obeys its invariant.
//
    {

#if defined(_DEBUG)
	TDes16::__DbgTestInvariant(); // Test base class
	if (!(Type()==EPtr || Type()==EBufCPtr))
		User::Invariant();
#endif
	}

/** Collapse all characters from 16 to 8 bits

@return 8-bit pointer descriptor to transformed text
*/
EXPORT_C TPtr8 TDes16::Collapse()
	{
	TInt l = Length();
	TInt ml = MaxLength();
	TText8* d = (TText8*)Ptr();
	TText8* d0 = d;
	const TText16* s = (const TText16*)d;
	const TText16* sE = s + l;
	while (s < sE)
		*d++ = (TText8)*s++;
	return TPtr8(d0, l, ml*sizeof(TText));
	}

// Truncate literal string to fit into descriptor
EXPORT_C void TDes16IgnoreOverflow::Overflow(TDes16& /*aDes*/)
	{}

#ifndef __KERNEL_MODE__

/**
Default constructor.

Constructs a zero-length 16-bit resizable buffer descriptor.

Note that the object owns no allocated memory.
*/
EXPORT_C RBuf16::RBuf16()
	:TDes16(EPtr,0,0),iEPtrType(NULL)
	{
	// Zero-length RBuf16 is of type EPtr with NULL pointer.
	}




/**
Constructor.
			
Constructs a 16-bit resizable buffer descriptor, transferring ownership of the
specified heap descriptor to this object.

@param aHBuf The heap descriptor to be transferred to this object. This pointer
             can be NULL, which means that a zero length 16-bit resizable
             buffer	descriptor is constructed, and the object will not own any
             allocated memory.
*/
EXPORT_C RBuf16::RBuf16(HBufC16* aHBuf)
	{
	if(aHBuf)
		//Create EBufCPtr type descriptor that points to aHBuf
		new(this) TPtr16(aHBuf->Des());
	else
		//Create zero-length RBuf16. It is EPtr type of descriptor that points to NULL.
		new(this) RBuf16();
	}




/**
Protected constructor.
*/
EXPORT_C RBuf16::RBuf16(TInt aType,TInt aLength,TInt aMaxLength)
	:TDes16(aType,aLength,aMaxLength)
	{
	}




/**
Transfers ownership of the specified 16-bit resizable buffer descriptor's 
buffer to this object.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aRBuf The source 16-bit resizable buffer. The ownership of this
             object's buffer is to be transferred.

@see RBuf16::Close()
*/
EXPORT_C void RBuf16::Assign(const RBuf16& aRBuf)
	{
	Mem::Copy(this, &aRBuf, sizeof(*this)); 
	__TEST_INVARIANT;
	}




/**
Assigns ownership of the specified allocated memory to this object.

The allocated memory forms the buffer for this descriptor. The current length
of the descriptor is set to zero.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aHeapCell  The allocated memory to be assigned to this object. This
                  pointer can be NULL, which means that a zero length 16-bit
                  resizable buffer descriptor is created.
@param aMaxLength The maximum length of the descriptor.

@panic USER 8 If the specified maximum length is greater then the size of
              the allocated heap cell, or the specified maximum length
              is NOT zero when the pointer to the heap cell is NULL.
              
@see TDesC16::Length()
@see TDes16::MaxLength()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::Assign(TUint16 *aHeapCell,TInt aMaxLength)
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
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aHeapCell  The allocated memory to be assigned to this object.
@param aLength	  The length of the descriptor.
@param aMaxLength The maximum length of the descriptor.

@panic USER 8 If the specified maximum length is greater then the size of
              the allocated heap cell, or the specified length is greater then
              the specified	maximum length, or the specified maximum length
              is NOT zero when the pointer to the heap cell is NULL.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::Assign(TUint16 *aHeapCell,TInt aLength,TInt aMaxLength)
	{
	__ASSERT_ALWAYS(aLength<=aMaxLength, Panic(ETDes16LengthOutOfRange));
	if(aHeapCell)
		{
		__ASSERT_ALWAYS(User::AllocLen(aHeapCell) >= aMaxLength * (TInt)sizeof(TUint16), Panic(ETDes16LengthOutOfRange));
		//Create EPtr type descriptor that points to aHeapCell
		new(this) TPtr16(aHeapCell,aLength,aMaxLength); 
		}
	else
		{
		__ASSERT_ALWAYS(aMaxLength == 0, Panic(ETDes16LengthOutOfRange));
		//Create zero-length RBuf. It is EPtr type of descriptor that points to NULL.
		new(this) RBuf16();
		}
	__TEST_INVARIANT;
	}




/**
Transfers ownership of the specified heap descriptor to this object.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aHBuf  The heap descriptor to be transferred to this object.
              This pointer can be NULL, which means that a zero length
              16-bit resizable buffer descriptor is created.

@see RBuf16::Close()
*/
EXPORT_C void RBuf16::Assign(HBufC16* aHBuf)
	{
	new(this) RBuf16(aHBuf);
	}




/**
Swaps the content of two 16-bit resizable buffer descriptors.

@param aRBuf The 16-bit resizable buffer descriptor whose contents are to be
             swapped with this one.
*/
EXPORT_C void RBuf16::Swap(RBuf16& aRBuf)
	{
	Mem::Swap(this,&aRBuf,sizeof(*this));
	}




/**
Creates a 16-bit resizable buffer descriptor.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

The current length of the descriptor is set to zero. The maximum length of
the descriptor is set to the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength  The maximum length of the descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient	memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see RBuf16::Close()
*/
EXPORT_C TInt RBuf16::Create(TInt aMaxLength)
	{
	if (aMaxLength)
		{
		//Allocate memory
		TUint16* buf=(TUint16*)User::Alloc(aMaxLength*sizeof(TUint16));
		if(!buf) return KErrNoMemory;
		iEPtrType = buf;
		}
	else
		iEPtrType = NULL; //Zero-length descriptor.


	//Create EPtr type descriptor.
	new(this) RBuf16(EPtr,0,aMaxLength);
	__TEST_INVARIANT;
	return KErrNone;
	}




/**
Creates 16-bit resizable buffer descriptor, and leaves on failure.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

The current length of the descriptor is set to zero. The maximum length of
the descriptor is set to the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength The maximum length of the descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::CreateL(TInt aMaxLength)
	{
	User::LeaveIfError(Create(aMaxLength));
	}




/**
Creates a 16-bit resizable buffer descriptor.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

Both the current length and the maximum length of the descriptor are set to
the specified value.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength  The length and the maximum length of the descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see RBuf16::Close()
*/
EXPORT_C TInt RBuf16::CreateMax(TInt aMaxLength)
	{
	TInt err=Create(aMaxLength); 
	if(err==KErrNone)
		SetMax(); 
	return err;
	}




/**
Creates a 16-bit resizable buffer descriptor, and leaves on failure.

The function allocates sufficient memory to contain descriptor data up to
the specified maximum length.

Both the current length and the maximum length of the descriptor are set to
the specified value. 

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any pre-existing owned
allocated memory.  If this descriptor does already own allocated memory,
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aMaxLength The length and the maximum length of the descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::CreateMaxL(TInt aMaxLength)
	{
	User::LeaveIfError(CreateMax(aMaxLength));
	}




/**
Creates a 16-bit resizable buffer descriptor to contain a copy of the
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
allocated memory, RBuf16::Close() should be invoked on this descriptor before 
this function is invoked.

@param aDes Source descriptor to be copied into this object.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see TDes16::Copy()
@see RBuf16::Close()
*/
EXPORT_C TInt RBuf16::Create(const TDesC16& aDes)
	{
	return Create(aDes,aDes.Length());
	}





/**
Creates a 16-bit resizable buffer descriptor to contain a copy of the specified
(source) descriptor, and leaves on failure.
 
The function allocates sufficient memory so that this descriptor's maximum
length is the same as the length of the source descriptor.Both the current
length and the maximum length of this descriptor are set to the length
of the source descriptor.

The data contained in the source descriptor is copied into this descriptor.

Note that the function assumes that this descriptor does not already own any
allocated memory. It does not check, nor does it free any
pre-existing owned allocated memory.  If this descriptor does already own 
allocated memory, RBuf16::Close() should be invoked on this descriptor before 
this function is invoked.

@param aDes Source descriptor to be copied into this object.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see TDes16::Copy()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::CreateL(const TDesC16& aDes)
	{
	CreateL(aDes,aDes.Length());
	}




/**
Creates a 16-bit resizable buffer descriptor to contain a copy of the
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
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aDes Source descriptor to be copied into this object.
            
@param aMaxLength The maximum length of this descriptor.

@return KErrNone, if successful; KErrNoMemory, if there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see TDes16::Copy()
@see RBuf16::Close()
*/
EXPORT_C TInt RBuf16::Create(const TDesC16& aDes,TInt aMaxLength)
	{
	TInt err=Create(aMaxLength);
	if(err==KErrNone) 
		Copy(aDes.Left(aMaxLength));
	return err;
	}




/**
Creates a 16-bit resizable buffer descriptor to contain a copy of the specified
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
RBuf16::Close() should be invoked on this descriptor before this function is
invoked.

@param aDes Source descriptor to be copied into this object.
            
@param aMaxLength The maximum length of this descriptor.

@leave KErrNoMemory If there is insufficient memory.

@see TDesC16::Length()
@see TDes16::MaxLength()
@see TDes16::Copy()
@see RBuf16::Close()
*/
EXPORT_C void RBuf16::CreateL(const TDesC16& aDes,TInt aMaxLength)
	{
	CreateL(aMaxLength);
	Copy(aDes.Left(aMaxLength));
	}




/**
Resizes this 16-bit resizable buffer descriptor.

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

@panic USER 14 If the new maximum length is less then the current descriptor length.
*/
EXPORT_C TInt RBuf16::ReAlloc(TInt aMaxLength)
	{
	__ASSERT_ALWAYS(Length()<=aMaxLength, Panic(ETDes16ReAllocTooSmall));
	__TEST_INVARIANT;

	if (!aMaxLength)				//Reallocation to zero length
		{
		User::Free(iEPtrType);	//Free memory 
		new (this) RBuf16();		//Create zero-length RBuf
		return KErrNone;
		}

	if (!iMaxLength)				//Reallocation from zero length
		return Create(aMaxLength); 

	switch(Type())
		{
		case EPtr:
			{
			TUint16* buf = (TUint16*)User::ReAlloc(iEPtrType,aMaxLength*sizeof(TUint16));
			if(!buf) return KErrNoMemory;
			iEPtrType = buf;
			iMaxLength = aMaxLength;
			break;
			}
		case EBufCPtr:
			{
			HBufC16* hbufc = iEBufCPtrType->ReAlloc(aMaxLength);
			if(!hbufc) return KErrNoMemory;
			Assign(hbufc);
			break;
			}
		}

	__TEST_INVARIANT;
	return KErrNone;
	}




/**
Resizes this 16-bit resizable buffer descriptor, leaving on failure.

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

@panic USER 14 If the new maximum length is less then the current descriptor length.
*/
EXPORT_C void RBuf16::ReAllocL(TInt aMaxLength)
	{
	User::LeaveIfError(ReAlloc(aMaxLength));
	}




/**
Deallocates memory assigned to this object, and re-initializes the object as
a zero-length descriptor.
*/
EXPORT_C void RBuf16::Close() 
	{
	User::Free(iEPtrType); 
	//Create zero-length RBuf. It is EPtr type of descriptor that points to NULL.
	new(this) RBuf16();
	}




/**
Pushes a cleanup item for this object onto the cleanup stack.

The effect of this is to cause Close() to be called on this 16-bit resizable
buffer descriptor, when CleanupStack::PopAndDestroy() is called at some later time.

@code
...
RBuf16 x;
....
x.CleanupClosePushL();
...
CleanupStack::PopAndDestroy();
...
@endcode

@see RBuf16::Close()
*/
EXPORT_C void RBuf16::CleanupClosePushL()
	{
	::CleanupClosePushL(*this);
	}




/**
Tests that the class obeys its invariant.
*/
EXPORT_C void RBuf16::__DbgTestInvariant() const
	{
#ifdef _DEBUG
	TDes16::__DbgTestInvariant();
	switch(Type())
		{
	case EPtr:
		if (iEPtrType)
			{
			__ASSERT_DEBUG(User::AllocLen(iEPtrType) >= iMaxLength*(TInt)sizeof(TUint16), Panic(EInvariantFalse));
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


#if defined(__DES16_MACHINE_CODED__) || defined(__EABI__)
GLDEF_C void Des16PanicBadDesType()
	{
	Panic(ETDes16BadDescriptorType);
	}

GLDEF_C void Des16PanicPosOutOfRange()
	{
	Panic(ETDes16PosOutOfRange);
	}
#endif

#ifdef __DES16_MACHINE_CODED__
GLDEF_C void Des16PanicLengthNegative()
	{
	Panic(ETDes16LengthNegative);
	}

GLDEF_C void Des16PanicMaxLengthNegative()
	{
	Panic(ETDes16MaxLengthNegative);
	}

GLDEF_C void Des16PanicLengthOutOfRange()
	{
	Panic(ETDes16LengthOutOfRange);
	}

GLDEF_C void Des16PanicDesOverflow()
	{
	Panic(ETDes16Overflow);
	}

GLDEF_C void Des16PanicDesIndexOutOfRange()
	{
	Panic(ETDes16IndexOutOfRange);
	}
#endif

