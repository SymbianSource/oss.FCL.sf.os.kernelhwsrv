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
// e32\euser\us_lex16.cpp
// 
//

#include "us_std.h"




EXPORT_C TLex16::TLex16()
	: iNext(NULL),iBuf(NULL),iEnd(NULL),iMark(NULL)
/** 
Default cosntructor.

Constructs a TLex16, initialising its members to NULL.
*/
	{}




EXPORT_C void TLex16::Assign(const TUint16 *aString)
/**
Assigns a string to this object from another string.

@param aString A pointer to a string to be assigned.
*/
	{

	iMark.iPtr=iNext=iBuf=aString;
	iEnd=iBuf+User::StringLength(aString);
	}




EXPORT_C void TLex16::Assign(const TDesC16 &aDes)
/**
Assigns a string to this object from a descriptor.

@param aDes The descriptor to be assigned.
*/
	{

	iMark.iPtr=iNext=iBuf=aDes.Ptr();
	iEnd=iBuf+aDes.Length();
	}




EXPORT_C void TLex16::UnGet()
/**
Decrements the next character position, allowing the previously "got" character 
to be re-read.

@panic USER 64, if the previous character is before the start of the string.
*/
	{

	__ASSERT_ALWAYS(iNext>iBuf,Panic(ETLex16UnGetUnderflow));
	iNext--;
	}




EXPORT_C void TLex16::UnGetToMark(const TLexMark16 aMark)
/**
Sets the next character position to the supplied extraction mark position.

@param aMark Mark to copy to the next character position.

@panic USER 68, if the extraction mark is before the start or beyond the end
       of the string.
*/
	{

	ValidateMark(aMark);
	iNext=aMark.iPtr;
	}




EXPORT_C void TLex16::Inc(TInt aNumber)
/**
Increments the next character position by aNumber.

@param aNumber The number of characters to increment the next character
               position by.

@panic USER 65, if the increment puts the next character position before the
                start or beyond the end of the string.               
*/
	{

	iNext+=aNumber;
	__ASSERT_ALWAYS(iNext>=iBuf && iNext<=iEnd,Panic(ETLex16IncOutOfRange));
	}




EXPORT_C void TLex16::Inc()
/**
Increments to the next character position.

@panic USER 65, if the increment puts the next character position before the
                start or beyond the end of the string.               
*/
	{

	if (!Eos())
		iNext++;
	__ASSERT_ALWAYS(iNext<=iEnd,Panic(ETLex16IncOutOfRange));
	}




EXPORT_C TChar TLex16::Get()
/**
Gets the next character in the string and increments the next
character position.

@return Next character to be read. 0 if at the end of the string.
*/
	{

	if (Eos())
		return(0);
	return(*iNext++);
	}




EXPORT_C TChar TLex16::Peek() const
/**
Shows the next character to be returned by Get().

@return Character to be returned by the next call to Get(). 0 if at the end of the 
        string.
        
@see TLex16::Get        
*/
	{

	if (Eos())
		return(0);
	return(*iNext);
	}




EXPORT_C void TLex16::SkipSpace()
/**
Moves the next character position past any white space (space or separator). 

Stops if at the end of string.
*/
	{

	while (!Eos() && Peek().IsSpace())
		iNext++;
	}




EXPORT_C void TLex16::SkipAndMark(TInt aNumber,TLexMark16& aMark)
/**
Moves the next character position a specified number of characters and copies 
it to the specified extraction mark.

@param aNumber Number of characters to skip.
@param aMark   On return, set to the next character position.

@panic USER 66, if the skip moves the next character position either to before
       the start or beyond the end of the string.
*/
	{

	iNext+=aNumber;
	__ASSERT_ALWAYS(iNext>=iBuf && iNext<=iEnd,Panic(ETLex16SkipOutOfRange));
	Mark(aMark);
	}




EXPORT_C void TLex16::SkipSpaceAndMark(TLexMark16& aMark)
/**
Moves the next character position past any white space and copies it to the 
specified extraction mark.

Stops if at the end of the string.

@param aMark On return, contains a reference to the next character position.
*/
	{

	SkipSpace();
	Mark(aMark);
	}




EXPORT_C void TLex16::SkipCharacters()
/**
Moves the next character position to the next white space (space or separator). 

Stops if at the end of the string.
*/
	{

	while (!Eos() && !Peek().IsSpace())
		iNext++;
	}




EXPORT_C TInt TLex16::TokenLength(const TLexMark16 aMark) const
/**
Gets the length of the token starting at the specified extraction mark.

@param aMark Extraction mark indicating the start of the token.

@return Length of the token.

@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.       
*/
	{

	ValidateMark(aMark);
	return (iNext-aMark.iPtr);
	}




EXPORT_C TPtrC16 TLex16::MarkedToken(const TLexMark16 aMark) const
/**
Extracts the token, starting at the specified mark

@param aMark Extraction mark indicating the start of the token.

@return Extracted token.

@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.       
*/
	{

	return(TPtrC16(aMark.iPtr,TokenLength(aMark)));
	}




EXPORT_C TPtrC16 TLex16::MarkedToken() const
//
// Extract the internally marked token
// there is the assumption here that iMark is always valid
/**
Extracts the marked token.

Note that the function assumes that the current extraction mark is valid.

@return Extracted token. 

@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.       
*/
	{

	return(TPtrC16(iMark.iPtr,TokenLength()));
	}




EXPORT_C TPtrC16 TLex16::NextToken()
/**
Strips any white space and extracts the next token.

@return Extracted token.
*/
	{
	TLexMark16 mark;

	SkipSpaceAndMark(mark);
	SkipCharacters();
	return(TPtrC16(mark.iPtr,iNext-mark.iPtr));
	}




EXPORT_C TPtrC16 TLex16::Remainder() const
/**
Gets a descriptor containing all the text from the next character position 
to the end of the string.

@return Text from the next character position onwards.

@panic USER 17, if the value of (next character position - extraction mark) is
       negative.
*/
	{

	return(TPtrC16(iNext,iEnd-iNext));
	}




EXPORT_C TPtrC16 TLex16::RemainderFromMark(const TLexMark16 aMark) const
/**
Gets a descriptor containing all the text from the specified extraction mark 
to the end of the string.

@param aMark Extraction mark indicating where remaining text starts.

@return Text from the specified extraction mark onwards.

@panic USER 17, if the value of (next character position - extraction mark) is
       negative.
@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.
*/
	{

	ValidateMark(aMark);
	return(TPtrC16(aMark.iPtr,iEnd-aMark.iPtr));
	}




EXPORT_C TPtrC16 TLex16::RemainderFromMark() const
//
// Return the remainder from iMark
// There is an assumption here that the internal mark will always be valid
//
/**
Gets a descriptor containing all the text from the extraction mark to the end 
of the string.

The function assumes that the current extraction mark is valid.

@return Text from the extraction mark onwards.
*/
	{

	return(TPtrC16(iMark.iPtr,iEnd-iMark.iPtr));
	}




EXPORT_C TInt TLex16::Offset() const
//
// Return the offset from iNext to the lex start.
//
/**
Gets the offset of the next character position from the start of the string.

@return Offset of next character position.
*/
	{

	return((TUint)(iNext-iBuf));
	}




EXPORT_C TInt TLex16::MarkedOffset(const TLexMark16 aMark) const
/**
Gets the offset of the specified extraction mark from the start of the string.

@param aMark Extraction mark.

@return The offset of the extraction mark.

@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.
*/
	{

	ValidateMark(aMark);
	return((TUint)(aMark.iPtr-iBuf));
	}




EXPORT_C TInt TLex16::BoundedVal(TUint32 &aVal,TRadix aRadix,TUint aLimit)
/**
Parses the string to extract a 32-bit unsigned integer, using the
specified radix, and checks that it is within the specified limit.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.
@param aLimit The upper limit.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TUint l=aLimit/aRadix;
	TUint v=0;
	TUint d=0;
	TLexMark16 mark(iNext);
	while (!Eos())
		{
		TChar c=Peek();
		if (!c.IsHexDigit())
			break;
		c=Get();
		if (c.IsAlpha())
			{
			c.UpperCase();
			c-=('A'-10);
			}
		else
			c-='0';
		if (c>=(TUint)aRadix)
			{
			iNext--;
			break;
			}
		if (v>l)
			{
			UnGetToMark(mark);
			return(KErrOverflow);
			}
		TUint o=v;
		v*=aRadix;
		v+=c;
		if (o>v)
			{
			UnGetToMark(mark);
			return(KErrOverflow);
			}
		d++;
		}
	if (d==0)
		{
		UnGetToMark(mark);
		return(KErrGeneral);
		}
	if (v>aLimit)
		{
		UnGetToMark(mark);
		return(KErrOverflow);
		}
	aVal=v;
	return(KErrNone);
	}




EXPORT_C TInt TLex16::BoundedVal(TInt32 &aVal,TInt aLimit)
/**
Parses the string to extract a 32-bit signed integer, and checks that it is
within the specified limit.

@param aVal   On return, contains the extracted integer.
@param aLimit The upper limit.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	if (Eos())
		return(KErrGeneral);
	TUint lim=aLimit;
	TLexMark16 mark(iNext);
	TUint s=FALSE;
	TChar c=Peek();
	if (c=='-')
		{
		lim++;
		s++;
		Inc();
		}
	else if (c=='+')
		Inc();
	TUint32 v;
	TInt r=BoundedVal(v,EDecimal,lim);
	if (r==KErrNone)
		{
		if (v>lim)
			r=KErrOverflow;
		else if (s)
			aVal=(-((TInt32)v));
		else
			aVal=v;
		}
	if (r!=KErrNone)
		UnGetToMark(mark);
	return(r);
	}




EXPORT_C TInt TLex16::BoundedVal(TInt64& aVal, TRadix aRadix, const TInt64& aLimit)
/**
Parses the string to extract a 64-bit signed integer, using the
specified radix, and checks that it is within the specified limit.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.
@param aLimit The upper limit.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{
	TUint64 rad = aRadix;
	TUint64 lim = static_cast<TUint64>(aLimit);

	lim /= rad;

	TUint64 v = 0;
	TUint digit=0;
	TLexMark16 mark(iNext);
	while (!Eos())
		{
		TChar c=Peek(); 
		if (!c.IsHexDigit())
			break;	  
		c=Get(); 
		if (c.IsAlpha())
			{
			c.UpperCase();
			c-=('A'-10);
			}
		else
			c-='0';
		if (c >= rad)
			{
			iNext--;
			break;
			}
		if (v > lim)
			{
			UnGetToMark(mark);
			return(KErrOverflow);
			}
		TUint64 old = v;
		v*=rad;
		v+=c;
		if (old > v)
			{
			UnGetToMark(mark);
			return(KErrOverflow);
			}
		digit++;
		}
	if (digit==0)
		{
		UnGetToMark(mark);
		return(KErrGeneral);
		}
	if (v > static_cast<TUint64>(aLimit))
		{
		UnGetToMark(mark);
		return(KErrOverflow);
		}
	aVal = static_cast<TInt64>(v);
	return(KErrNone);
	}




EXPORT_C TInt TLex16::BoundedVal(TInt64& aVal, const TInt64& aLimit)
/**
Parses the string to extract a 64-bit signed integer, and checks that it
is within the specified limit.

@param aVal   On return, contains the extracted integer.
@param aLimit The upper limit.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	if (Eos())
		return(KErrGeneral);
	TInt64 lim = aLimit;
	TLexMark16 mark(iNext);
	TBool s=EFalse;
	TChar c=Peek();
	if (c=='-')
		{
		lim++;
		s++;
		Inc();
		}
	else if (c=='+')
		Inc();
	TInt64 v;
	TInt r=BoundedVal(v,EDecimal,lim);
	if (r==KErrNone)
		{
		if (v>lim)
			r=KErrOverflow;
		else if (s)
			aVal=(-v);
		else
			aVal=v;
		}
	if (r!=KErrNone)
		UnGetToMark(mark);
	return(r);
	}




EXPORT_C TInt TLex16::Val(TInt8 &aVal)
/**
Parses the string to extract a signed 8-bit integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TInt32 v;
	TInt r=BoundedVal(v,0x7fu);
	if (r==KErrNone)
		aVal=(TInt8)v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TInt16 &aVal)
/**
Parses the string to extract a signed 16-bit integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TInt32 v;
	TInt r=BoundedVal(v,0x7fffu);
	if (r==KErrNone)
		aVal=(TInt16)v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TInt32 &aVal)
/**
Parses the string to extract a signed 32-bit integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TInt32 v;
	TInt r=BoundedVal(v,0x7fffffffu);
	if (r==KErrNone)
		aVal=v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TInt64& aVal)
/**
Parses the string to extract a signed 64-bit integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TChar c=Peek();
	if (c=='-' || c=='+')
		Inc();
	TInt r;
	if (c=='-')
		{
		r=BoundedVal(aVal, EDecimal, UI64LIT(0x8000000000000000));
		if (r==KErrNone)
			aVal=-aVal;
		}
	else
		r=BoundedVal(aVal, UI64LIT(0x7fffffffffffffff));
	return(r);
	}




EXPORT_C TInt TLex16::Val(TUint8 &aVal,TRadix aRadix)
/**
Parses the string to extract an 8-bit unsigned integer, using the
specified radix.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TUint32 v;
	TInt r=BoundedVal(v,aRadix,0xffu);
	if (r==KErrNone)
		aVal=(TUint8)v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TUint16 &aVal,TRadix aRadix)
/**
Parses the string to extract a 16-bit unsigned integer, using the
specified radix.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TUint32 v;
	TInt r=BoundedVal(v,aRadix,0xffffu);
	if (r==KErrNone)
		aVal=(TUint16)v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TUint32 &aVal,TRadix aRadix)
/**
Parses the string to extract a 32-bit unsigned integer, using the
specified radix.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	TUint32 v;
	TInt r=BoundedVal(v,aRadix,0xffffffffu);
	if (r==KErrNone)
		aVal=v;
	return(r);
	}




EXPORT_C TInt TLex16::Val(TInt64& aVal, TRadix aRadix)
/**
Parses the string to extract a 64-bit integer (treated as unsigned), using the
specified radix.

The specified radix is one of binary, octal, decimal, or hexadecimal.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{

	return BoundedVal(aVal,aRadix,TInt64(UI64LIT(0xffffffffffffffff)));
	}




void TLex16::ValidateMark(const TLexMark16 aMark) const
//
// Asserts that the mark is valid for this lex
//
	{
	__ASSERT_ALWAYS(aMark.iPtr>=iBuf && aMark.iPtr<=iEnd,Panic(ETLex16MarkOutOfRange));
	}


