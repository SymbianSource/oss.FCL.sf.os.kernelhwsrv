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
// e32\include\e32cmn.h
// 
//

#ifndef __E32CMN_H__
#define __E32CMN_H__
#include <e32const.h>

extern "C" {
/**
@publishedAll
@released

A Nanokernel utility function that compares two memory buffers for equality.

The two buffers are considered equal only if:

1. the buffers have the same length

and
 
2. the binary content of both buffers is the same.

@param aLeft     The start address of the first buffer in the comparison.
@param aLeftLen  The length of the first buffer in the comparison.
@param aRight    The start address of the second buffer in the comparison.
@param aRightLen The length of the second buffer in the comparison.

@return Zero if both buffers are equal; non-zero, otherwise.

@panic USER 88        In debug mode only, if aLeftL is negative, 
                      and the function is called on the user side.
@panic KERN-COMMON 88 In debug mode only, if aLeftL is negative,
                      and the function is called on the kernel side.
@panic USER 89        In debug mode only, if aRightL is negative, 
                      and the function is called on the user side.
@panic KERN-COMMON 89 In debug mode only, if aRightL is negative,
                      and the function is called on the kernel side.
*/
IMPORT_C TInt memcompare(const TUint8* aLeft, TInt aLeftLen, const TUint8* aRight, TInt aRightLen);




/**
@publishedAll
@released

A Nanokernel utility function that moves (copies) bytes in memory.

The function assumes that the addresses are aligned on word boundaries,
and that the length value is a multiple of 4.

@param aTrg    The target address.
@param aSrc    The source address.
@param aLength The number of bytes to be moved.

@return The target address.

@panic USER 91        In debug mode only, if aLength is not a multiple of 4,
                      and the function is called on the user side.
@panic KERN-COMMON 91 In debug mode only, if aLength is not a multiple of 4,
                      and the function is called on the kernel side.
@panic USER 92        In debug mode only, if aSrc is not aligned on a word boundary,
                      and the function is called on the user side.
@panic KERN-COMMON 92 In debug mode only, if aSrc is not aligned on a word boundary,
                      and the function is called on the kernel side.
@panic USER 93        In debug mode only, if aTrg is not aligned on a word boundary,
                      and the function is called on the user side.
@panic KERN-COMMON 93 In debug mode only, if aTrg is not aligned on a word boundary,
                      and the function is called on the kernel side.
*/
IMPORT_C TAny* wordmove(TAny* aTrg, const TAny* aSrc, unsigned int aLength);




/**
@publishedAll
@released

A Nanokernel utility function that sets the specified number of bytes
to binary zero.

@param aTrg    The start address.
@param aLength The number of bytes to be set.

@return The target address.
*/
IMPORT_C TAny* memclr(TAny* aTrg, unsigned int aLength);
}




#ifndef __TOOLS__
extern "C" {
/**
@publishedAll
@released

A Nanokernel utility function that sets all of the specified number of bytes to
the specified fill value.

@param aTrg    The start address.
@param aValue  The fill value (the first or junior byte).
@param aLength The number of bytes to be set.

@return The target address.
*/
	IMPORT_C TAny* memset(TAny* aTrg, TInt aValue, unsigned int aLength);




/**
@publishedAll
@released

A Nanokernel utility function that copies bytes in memory.

@param aTrg    The target address.
@param aSrc    The source address.
@param aLength The number of bytes to be moved.

@return The target address.
*/
	IMPORT_C TAny* memcpy(TAny* aTrg, const TAny* aSrc, unsigned int aLength);




/**
@publishedAll
@released

A Nanokernel utility function that moves (copies) bytes in memory.

@param aTrg    The target address.
@param aSrc    The source address.
@param aLength The number of bytes to be moved.

@return The target address.
*/
	IMPORT_C TAny* memmove(TAny* aTrg, const TAny* aSrc, unsigned int aLength);
}
#else
#include <string.h>
#endif




/** 
@publishedAll
@released

Tests whether the specified value is less than or equal to the
specified upper limit.

@param aVal   The value to be tested.
@param aLimit The upper limit.

@return True, if the value is less than or equal to the specified upper limit;
        false, otherwise.
*/
inline TInt Lim(TInt aVal,TUint aLimit)
	{return(((TUint)aVal)<=aLimit);}




/** 
@publishedAll
@released

Tests whether the specified value is strictly less than the
specified upper limit.

@param aVal   The value to be tested.
@param aLimit The upper limit.

@return True, if the value is strictly less than the specified upper limit;
        false, otherwise.
*/
inline TInt LimX(TInt aVal,TUint aLimit)
	{return(((TUint)aVal)<aLimit);}




/** 
@publishedAll
@released

Returns the smaller of two values.

@param aLeft  The first value to be compared.
@param aRight The second value to be compared.

@return The smaller value.
*/
template <class T>
inline T Min(T aLeft,T aRight)
	{return(aLeft<aRight ? aLeft : aRight);}




/**
@publishedAll
@released

Returns the smaller of two objects, where the right hand object is a treated
as a TInt for the  purpose of comparison.

@param aLeft  The first value to be compared.
@param aRight The second value to be compared.

@return The smaller value.
*/
template <class T>
inline T Min(T aLeft,TUint aRight)
	{return(aLeft<(TInt)aRight ? aLeft : (T)aRight);}




/** 
@publishedAll
@released

Returns the larger of two values.

@param aLeft  The first value to be compared.
@param aRight The second value to be compared.

@return The larger value.
*/
template <class T>
inline T Max(T aLeft,T aRight)
	{return(aLeft<aRight ? aRight : aLeft);}




/**
@publishedAll
@released

Returns the larger of two objects, where the right hand object is a treated
as a TInt for the  purpose of comparison.

@param aLeft  The first value to be compared.
@param aRight The second value to be compared.

@return The larger value.
*/
template <class T>
inline T Max(T aLeft,TUint aRight)
	{return(aLeft<(TInt)aRight ? (TInt)aRight : aLeft);}




/**
@publishedAll
@released

Returns an absolute value.

@param aVal The source value.

@return The absolute value
*/
template <class T>
inline T Abs(T aVal)
	{return(aVal<0 ? -aVal : aVal);}




/** 
@publishedAll
@released

Determines whether a specified value lies within a defined range of values.

@param aMin The lower value of the range.
@param aVal The value to be compared.
@param aMax The higher value of the range.

@return True, if the specified value lies within the range; false, otherwise.
*/
template <class T>
inline TBool Rng(T aMin,T aVal,T aMax)
	{return(aVal>=aMin && aVal<=aMax);}




/**
@publishedAll
@released

Adds a value to a pointer.

@param aPtr Pointer to an object of type T.
@param aVal The value to be added.

@return The resulting pointer value, as a pointer to a type T.
*/
template <class T,class S>
inline T* PtrAdd(T* aPtr,S aVal)
	{return((T*)(((TUint8*)aPtr)+aVal));}




/**
@publishedAll
@released

Subtracts a value from a pointer.

@param aPtr Pointer to an object of type T.
@param aVal The value to be added.

@return The resulting pointer value, as a pointer to a type T.
*/
template <class T,class S>
inline T* PtrSub(T* aPtr,S aVal)
	{return((T*)(((TUint8*)aPtr)-aVal));}




/**
@publishedAll
@released

Aligns the specified value onto a 2-byte boundary.

@param aValue The value to be aligned.

@return The aligned value. 
*/
template <class T>
inline T Align2(T aValue)
	{return((T)((((TUint)aValue)+sizeof(TUint16)-1)&~(sizeof(TUint16)-1)));}




/**
@publishedAll
@released

Aligns the specified value onto a 4-byte boundary.

@param aValue The value to be aligned.

@return The aligned value. 
*/
template <class T>
inline T Align4(T aValue)
	{return((T)((((TUint)aValue)+sizeof(TUint32)-1)&~(sizeof(TUint32)-1)));}




/**
@publishedAll
@released

A templated class which encapsulates a reference to an object within a wrapper.

The wrapper object can be passed to a function as a value type. This allows 
a reference to be passed to a function as a value type.

This wrapper object is commonly termed a value reference.
*/
template <class T>
class TRefByValue
	{
public:
	inline TRefByValue(T& aRef);
	inline operator T&();
private:
	TRefByValue& operator=(TRefByValue aRef);
private:
	T &iRef;
	};




#if !defined (__KERNEL_MODE__)
class TDesC16;	// forward declaration for TChar member functions
class TPtrC16;	// forward declaration for TChar member functions
#endif




/**
@publishedAll
@released

Holds a character value and provides a number of utility functions to
manipulate it and test its properties.

For example, there are functions to convert the character 
to uppercase and test whether or not it is a control character.

The character value is stored as a 32-bit unsigned integer. The shorthand 
"TChar value" is used to describe the character value wrapped by a TChar 
object.

TChar can be used to represent Unicode values outside plane 0 (that is, the 
extended Unicode range from 0x10000 to 0xFFFFF). This differentiates it from 
TText which can only be used for 16-bit Unicode character values.

@see TText
*/
class TChar
	{
public:

	
    /**
    General Unicode character category.

    The high nibble encodes the major category (Mark, Number, etc.) and a low 
    nibble encodes the subdivisions of that category.

    The category codes can be used in three ways:
    
    (i) as unique constants: there is one for each Unicode category, with a
    name of the form
    @code
    E<XX>Category
    @endcode
    where
    @code
    <XX>
    @endcode
    is the category name given by
    the Unicode database (e.g., the constant ELuCategory is used for lowercase
    letters, category Lu);
    
    (ii) as numbers in certain ranges: letter categories are all <= EMaxLetterCategory;
    
    (iii) as codes in which the upper nibble gives the category group
    (e.g., punctuation categories all yield TRUE for
    the test (category & 0xF0) ==EPunctuationGroup).
    */
	enum TCategory
		{
        /**
        Alphabetic letters.
	
        Includes ELuCategory, ELlCategory and ELtCategory.
        */
		EAlphaGroup = 0x00,								
        
        
        /**
        Other letters.
	
        Includes ELoCategory.
        */
		ELetterOtherGroup = 0x10,						
        
        
        /**
        Letter modifiers.
	
        Includes ELmCategory.
        */
		ELetterModifierGroup = 0x20,					
        
        
        /**
        Marks group.
	
        Includes EMnCategory, EMcCategory and EMeCategory.
        */
		EMarkGroup = 0x30,
        
        
        /**
        Numbers group.
	
	    Includes ENdCategory, ENlCategory and ENoCategory.
	    */
		ENumberGroup = 0x40,
        
        
        /**
        Punctuation group.
	
	    IncludesEPcCategory, PdCategory, EpeCategory, EPsCategory and EPoCategory.
	    */
		EPunctuationGroup = 0x50,
        
        
        /**
        Symbols group.
	
        Includes ESmCategory, EScCategory, ESkCategory and ESoCategory.
        */
		ESymbolGroup = 0x60,
        
        
        /**
        Separators group.
	
        Includes EZsCategory, EZlCategory and EZlpCategory.
        */
		ESeparatorGroup = 0x70,
        
        
        /**
        Control, format, private use, unassigned.
	
     	Includes ECcCategory, ECtCategory, ECsCategory,
     	ECoCategory and ECnCategory.
     	*/
		EControlGroup = 0x80,
	    
	    
	    /**
	    The highest possible groups category.
	    */
		EMaxAssignedGroup = 0xE0,
        
        
        /**
        Unassigned to any other group.
        */
		EUnassignedGroup = 0xF0,


        /**
        Letter, Uppercase.
        */
		ELuCategory = EAlphaGroup | 0,					
        
        
        /**
        Letter, Lowercase.
        */
		ELlCategory = EAlphaGroup | 1,					
	    
	    
	    /**
	    Letter, Titlecase.
	    */
		ELtCategory = EAlphaGroup | 2,					
     	
     	
     	/**
     	Letter, Other.
     	*/
		ELoCategory = ELetterOtherGroup | 0,			
	    
	    
	    /**
	    The highest possible (non-modifier) letter category.
	    */
		EMaxLetterCategory = ELetterOtherGroup | 0x0F,	

	    /**
	    Letter, Modifier.
	    */
		ELmCategory = ELetterModifierGroup | 0,			
	    
	    
	    /**
	    The highest possible letter category.
	    */
		EMaxLetterOrLetterModifierCategory = ELetterModifierGroup | 0x0F, 

	    /**
	    Mark, Non-Spacing
	    */
		EMnCategory = EMarkGroup | 0,					
        
        
        /**
        Mark, Combining.
        */
		EMcCategory = EMarkGroup | 1,					
        
        
        /**
        Mark, Enclosing.
        */
		EMeCategory = EMarkGroup | 2,					
        
        
        /**
        Number, Decimal Digit.
        */
		ENdCategory = ENumberGroup | 0,					
        
        
        /**
        Number, Letter.
        */
		ENlCategory = ENumberGroup | 1,					
        
        
        /**
        Number, Other.
        */
		ENoCategory = ENumberGroup | 2,					
        
        
        /**
        Punctuation, Connector.
        */
		EPcCategory = EPunctuationGroup | 0,			
        
        
        /**
        Punctuation, Dash.
        */
		EPdCategory = EPunctuationGroup | 1,			
        
        
        /**
        Punctuation, Open.
        */
		EPsCategory = EPunctuationGroup | 2,			
        
        
        /**
        Punctuation, Close.
        */
		EPeCategory = EPunctuationGroup | 3,
		
		
		/**
		Punctuation, Initial Quote
		*/			
		EPiCategory = EPunctuationGroup | 4,			
		
		
		/**
		Punctuation, Final Quote
		*/
		EPfCategory = EPunctuationGroup | 5,			
        
        
        /**
        Punctuation, Other.
        */
		EPoCategory = EPunctuationGroup | 6,			
        
        
        /**
        Symbol, Math.
        */
		ESmCategory = ESymbolGroup | 0,					
        
        
        /**
        Symbol, Currency.
        */
		EScCategory = ESymbolGroup | 1,					
        
        
        /**
        Symbol, Modifier.
        */
		ESkCategory = ESymbolGroup | 2,					
        
        
        /**
        Symbol, Other.
        */
		ESoCategory = ESymbolGroup | 3,					
        
        
        /**
        The highest possible graphic character category.
        */
		EMaxGraphicCategory = ESymbolGroup | 0x0F,		


        /**
        Separator, Space.
        */
		EZsCategory = ESeparatorGroup | 0,				


        /**
        The highest possible printable character category.
        */
		EMaxPrintableCategory = EZsCategory,			


        /**
        Separator, Line.
        */
		EZlCategory = ESeparatorGroup | 1,				


        /**
        Separator, Paragraph.
        */
		EZpCategory = ESeparatorGroup | 2,				


        /**
        Other, Control.
        */
		ECcCategory = EControlGroup | 0,				


        /**
        Other, Format.
        */
		ECfCategory = EControlGroup | 1,				


        /**
        The highest possible category for assigned 16-bit characters; does not
        include surrogates, which are interpreted as pairs and have no meaning
        on their own.
        */
		EMaxAssignedCategory = EMaxAssignedGroup | 0x0F,
														

        /**
        Other, Surrogate.
        */
		ECsCategory = EUnassignedGroup | 0,				
        
        
        /**
        Other, Private Use.
        */
		ECoCategory = EUnassignedGroup | 1,				
        
        
        /**
        Other, Not Assigned.
        */
		ECnCategory = EUnassignedGroup | 2				
		};

	
    /**
    The bi-directional Unicode character category.

    For more information on the bi-directional algorithm, see Unicode Technical 
    Report No. 9 available at: http://www.unicode.org/unicode/reports/tr9.
    */
	enum TBdCategory
		{
	    /**
	    Left to right.
	    */
		ELeftToRight,				// L Left-to-Right 
	   
	   
	    /**
	    Left to right embedding.
	    */
		ELeftToRightEmbedding,		// LRE Left-to-Right Embedding 
	   
	   
	    /**
	    Left-to-Right Override.
	    */
		ELeftToRightOverride,		// LRO Left-to-Right Override 
	   
	   
	    /**
	    Right to left.
	    */
		ERightToLeft,				// R Right-to-Left 
	   
	   
	    /**
	    Right to left Arabic.
	    */
		ERightToLeftArabic,			// AL Right-to-Left Arabic 
	   
	   
	    /**
	    Right to left embedding.
	    */
		ERightToLeftEmbedding,		// RLE Right-to-Left Embedding 
	   
	   
	    /**
	    Right-to-Left Override.
	    */
		ERightToLeftOverride,		// RLO Right-to-Left Override 
	   
	   
	    /**
	    Pop Directional Format.
	    */
		EPopDirectionalFormat,		// PDF Pop Directional Format 
	   
	   
	    /**
	    European number.
	    */
		EEuropeanNumber,			// EN European Number 
	   
	   
	    /**
	    European number separator.
	    */
		EEuropeanNumberSeparator,	// ES European Number Separator 
	   
	   
	    /**
	    European number terminator.
	    */
		EEuropeanNumberTerminator,	// ET European Number Terminator 
	   
	   
	    /**
	    Arabic number.
	    */
		EArabicNumber,				// AN Arabic Number 
	   
	   
	    /**
	    Common number separator.
	    */
		ECommonNumberSeparator,		// CS Common Number Separator 
	   
	   
	    /**
	    Non Spacing Mark.
	    */
		ENonSpacingMark,			// NSM Non-Spacing Mark 
	   
	   
	    /**
	    Boundary Neutral.
	    */
		EBoundaryNeutral,			// BN Boundary Neutral 
	   
	   
	    /**
	    Paragraph Separator.
	    */
		EParagraphSeparator,		// B Paragraph Separator 
	   
	   
	    /**
	    Segment separator.
	    */
		ESegmentSeparator,			// S Segment Separator 

		
		/**
		Whitespace
		*/
		EWhitespace,				// WS Whitespace 


	    /**
	    Other neutrals; all other characters: punctuation, symbols.
	    */
		EOtherNeutral				// ON Other Neutrals 
		};


	/**
    Notional character width as known to East Asian (Chinese, Japanese,
    Korean (CJK)) coding systems.
    */
	enum TCjkWidth
		{
	    /**
	    Includes 'ambiguous width' defined in Unicode Technical Report 11: East Asian Width
	    */
		ENeutralWidth,			
	    
	    
	    /**
	    Character which occupies a single cell.
	    */
		EHalfWidth,				// other categories are as defined in the report
        
        
        /**
        Character which occupies 2 cells.
        */
		EFullWidth,
        
        
        /**
        Characters that are always narrow and have explicit full-width
        counterparts. All of ASCII is an example of East Asian Narrow
        characters.
        */
		ENarrow,
	    
	    /**
	    Characters that are always wide. This category includes characters that
	    have explicit half-width counterparts.
	    */
		EWide
		};


	/**
	@deprecated
    
    Encoding systems used by the translation functions.
    */
  	enum TEncoding
  		{
  		/**
  		The Unicode encoding.
  		*/
  		EUnicode,
        
        
        /**
        The shift-JIS encoding (used in Japan).
        */
  		EShiftJIS		
  		};


	/**
	Flags defining operations to be performed using TChar::Fold().
	
	The flag values are passed to the Fold() funtion.

	@see TChar::Fold
	*/
	enum
		{
		/**
		Convert characters to their lower case form if any.
		*/
		EFoldCase = 1,			


		/**
		Strip accents
     	*/
		EFoldAccents = 2,		


		/**
		Convert digits representing values 0..9 to characters '0'..'9'
     	*/
		EFoldDigits = 4,		


		/**
		Convert all spaces (ordinary, fixed-width, ideographic, etc.) to ' '
     	*/
		EFoldSpaces = 8,		


		/**
		Convert hiragana to katakana.
     	*/
		EFoldKana = 16,			


		/**
	    Fold fullwidth and halfwidth variants to their standard forms
     	*/
		EFoldWidth = 32,		


		/**
		Perform standard folding operations, i.e.those done by Fold() with no argument
     	*/
		EFoldStandard = EFoldCase | EFoldAccents | EFoldDigits | EFoldSpaces,


        /**
        Perform all possible folding operations
        */
		EFoldAll = -1	
		};


	struct TCharInfo
    /**
    A structure to hold information about a Unicode character.
    
    An object of this type is passed to TChar::GetInfo().
 
    @see TChar::GetInfo
    */
		{
	    /**
	    General category.
	    */
		TCategory iCategory;				
        
        
        /**
        Bi-directional category.
        */
		TBdCategory iBdCategory;			
        
        
        /**
        Combining class: number (currently) in the range 0..234
        */
		TInt iCombiningClass;				
        
        
        /**
        Lower case form.
        */
		TUint iLowerCase;					
        
        
        /**
        Upper case form.
        */
		TUint iUpperCase;					
        
        
        /**
        Title case form.
        */
		TUint iTitleCase;					
        
        
        /**
        True, if the character is mirrored.
        */
		TBool iMirrored;					
        
        
        /**
        Integer numeric value: -1 if none, -2 if a fraction.
        */
		TInt iNumericValue;					
		};

	inline TChar();
	inline TChar(TUint aChar);
	inline TChar& operator-=(TUint aChar);
	inline TChar& operator+=(TUint aChar);
	inline TChar operator-(TUint aChar);
	inline TChar operator+(TUint aChar);
	inline operator TUint() const;
#ifndef __KERNEL_MODE__
	inline void Fold();
	inline void LowerCase();
	inline void UpperCase();
	inline TBool Eos() const;
	IMPORT_C TUint GetUpperCase() const;
	IMPORT_C TUint GetLowerCase() const;
	IMPORT_C TBool IsLower() const;
	IMPORT_C TBool IsUpper() const;
	IMPORT_C TBool IsAlpha() const;
	IMPORT_C TBool IsDigit() const;
	IMPORT_C TBool IsAlphaDigit() const;
	IMPORT_C TBool IsHexDigit() const;
	IMPORT_C TBool IsSpace() const;
	IMPORT_C TBool IsPunctuation() const;
	IMPORT_C TBool IsGraph() const;
	IMPORT_C TBool IsPrint() const;
	IMPORT_C TBool IsControl() const;
	inline void Fold(TInt aFlags);
	inline void TitleCase();
	IMPORT_C TUint GetTitleCase() const;
	IMPORT_C TBool IsTitle() const;
	IMPORT_C TBool IsAssigned() const;
	IMPORT_C void GetInfo(TCharInfo& aInfo) const;
	IMPORT_C TCategory GetCategory() const;
	IMPORT_C TBdCategory GetBdCategory() const;
	IMPORT_C TInt GetCombiningClass() const;
	IMPORT_C TBool IsMirrored() const;
	IMPORT_C TInt GetNumericValue() const;
	IMPORT_C TCjkWidth GetCjkWidth() const;
	IMPORT_C static TBool Compose(TUint& aResult,const TDesC16& aSource);
	IMPORT_C TBool Decompose(TPtrC16& aResult) const;

protected:
	inline void SetChar(TUint aChar);
#endif
private:
	TUint iChar;
	__DECLARE_TEST;
	};

#include <e32des8.h>
#ifndef __KERNEL_MODE__
#include <e32des16.h>
#endif




#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
#define __Size (sizeof(TUint)/sizeof(TUint16))
/**
@publishedAll
@released

Defines a build-independent non-modifiable descriptor.

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see TDesC8
@see TDesC16
*/
typedef TDesC16 TDesC;




/**
@publishedAll
@released

Defines a build-independent non-modifiable pointer descriptor.

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see TPtrC8
@see TPtrC16
*/
typedef TPtrC16 TPtrC;




/**
@publishedAll
@released

Defines a build-independent modifiable descriptor.

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see TDes8
@see TDes16
*/
typedef TDes16 TDes;




/**
@publishedAll
@released

Defines a build-independent modifiable pointer descriptor.

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see TPtr8
@see TPtr16
*/
typedef TPtr16 TPtr;




#ifndef __KERNEL_MODE__
/**
@publishedAll
@released

Defines a build-independent heap descriptor. 

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see HBufC8
@see HBufC16
*/
typedef HBufC16 HBufC;




/** 
@publishedAll
@released

Defines a build-independent descriptor overflow handler.

A 16-bit build variant is generated for a Unicode, non-kernel
mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see TDes8Overflow
@see TDes16Overflow
*/
typedef TDes16Overflow TDesOverflow;


/** 
@publishedAll
@released

Defines a build-independent resizable buffer descriptor.

A 16-bit build variant is generated for a Unicode, non-kernel mode build.

A build-independent type should always be used unless an explicit 8-bit 
or 16-bit type is required.

@see RBuf8
@see RBuf16
*/
typedef RBuf16 RBuf;

#endif
#else
#define __Size (sizeof(TUint)/sizeof(TUint8))




/**
@publishedAll
@released

Defines a build-independent non-modifiable descriptor.

An 8-bit build variant is generated for a non-Unicode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see TDesC8
@see TDesC16
*/
typedef TDesC8 TDesC;




/**
@publishedAll
@released

Defines a build-independent non-modifiable pointer descriptor.

An 8-bit build variant is generated for a non-Unicode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see TPtrC8
@see TPtrC16
*/
typedef TPtrC8 TPtrC;




/**
@publishedAll
@released

Defines a build-independent modifiable descriptor.

An 8-bit build variant is generated for a non-Unicode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see TDes8
@see TDes16
*/
typedef TDes8 TDes;




/**
@publishedAll
@released

Defines a build-independent modifiable pointer descriptor.

An 8-bit build variant is generated for a non-Unicode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see TPtr8
@see TPtr16
*/
typedef TPtr8 TPtr;
#ifndef __KERNEL_MODE__




/**
@publishedAll
@released

Defines a build-independent heap descriptor.

An 8-bit build variant is generated for a non-Unicode, non-kernel
mode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see HBufC8
@see HBufC16
*/
typedef HBufC8 HBufC;




/**
@publishedAll
@released

Defines a build-independent descriptor overflow handler. 

An 8-bit build variant is generated for a non-Unicode, non-kernel
mode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see TDes8Overflow
@see TDes16Overflow
*/
typedef TDes8Overflow TDesOverflow;


/**
@publishedAll
@released

Defines a build-independent resizable buffer descriptor.

An 8-bit build variant is generated for a non-Unicode, non-kernel mode build.

This build-independent type should always be used unless an explicit 8-bit 
or 16-bit build variant is required.

@see RBuf8
@see RBuf16
*/
typedef RBuf8 RBuf;

#endif
#endif


#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
typedef TBufCBase16 TBufCBase;
#else
typedef TBufCBase8 TBufCBase;
#endif

/**
@publishedAll
@released

A build-independent non-modifiable buffer descriptor.

This is a descriptor class which provides a buffer of fixed length for
containing and accessing TUint16 or TUint8 data, depending on the build.

The class intended for instantiation. The data that the descriptor represents 
is part of the descriptor object itself.

The class is templated, based on an integer value which defines the size of 
the descriptor's data area.

The data is intended to be accessed, but not modified; however, it can be 
completely replaced using the assignment operators of this class. The base 
class provides the functions through which the data is accessed.

This class derives from TBufCBase16 for a Unicode, non-kernel build, but
derives from TBufCBase8 for a non-Unicode build.

@see TDesC
@see TDesC8
@see TDesC16
@see TPtr
@see TPtr8
@see TPtr16
@see TBufC8
@see TBufC16
*/
template <TInt S>
#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
class TBufC : public TBufCBase16
#else
class TBufC : public TBufCBase8
#endif
	{
public:
	inline TBufC();
	inline TBufC(const TText* aString);
	inline TBufC(const TDesC& aDes);
	inline TBufC<S>& operator=(const TText* aString);
	inline TBufC<S>& operator=(const TDesC& aDes);
	inline TPtr Des();
private:
	TText iBuf[__Align(S)];
	};



/**
@publishedAll
@released

A build-independent modifiable buffer descriptor.

This is a descriptor class which provides a buffer of fixed length for
containing, accessing and manipulating TUint16 or TUint8 data, depending
on the build.

The class is intended for instantiation. The data that the descriptor represents 
is part of the descriptor object itself.

The class is templated, based on an integer value which determines the size 
of the data area created as part of the buffer descriptor object; this is 
also the maximum length of the descriptor.

The data is intended to be both accessed and modified. The base classes provide 
the functions through which the data is accessed.

This class derives from TBufCBase16 for a Unicode, non-kernel build, but
derives from TBufCBase8 for a non-Unicode build.

@see TDesC
@see TDesC8
@see TDesC16
@see TDes
@see TDes8
@see TDes16
@see TPtr
@see TPtr8
@see TPtr16
*/
template <TInt S>
#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
class TBuf : public TBufBase16
#else
class TBuf : public TBufBase8
#endif
	{
public:
	inline TBuf();
	inline explicit TBuf(TInt aLength);
	inline TBuf(const TText* aString);
	inline TBuf(const TDesC& aDes);
	inline TBuf<S>& operator=(const TText* aString);
	inline TBuf<S>& operator=(const TDesC& aDes);
	inline TBuf<S>& operator=(const TBuf<S>& aBuf);
private:
	TText iBuf[__Align(S)];
	};




/**
@publishedAll
@released

Value reference used in operator TLitC::__TRefDesC().

@see TRefByValue
*/
typedef TRefByValue<const TDesC> __TRefDesC;




/**
@publishedAll
@released

Encapsulates literal text.

This is always constructed using an _LIT macro.

This class is build independent; i.e. for a non-Unicode build, an 8-bit build
variant is generated; for a Unicode build, a 16 bit build variant is generated.

The class has no explicit constructors. See the _LIT macro definition.
*/
template <TInt S>
class TLitC
	{
public:
    /**
    @internalComponent
    */
	enum {BufferSize=S-1};
	inline const TDesC* operator&() const;
	inline operator const TDesC&() const;
	inline const TDesC& operator()() const;
	inline operator const __TRefDesC() const;
public:
#if !defined(_UNICODE) || defined(__KERNEL_MODE__)

    /**
    @internalComponent
    */
	typedef TUint8 __TText;
#elif defined(__GCC32__)

    /**
    @internalComponent
    */
	typedef wchar_t __TText;
#elif defined(__VC32__)

	/**
    @internalComponent
    */
	typedef TUint16 __TText;

#elif defined(__CW32__)

    /**
    @internalComponent
    */
	typedef TUint16 __TText;
#elif !defined(__TText_defined)
#error  no typedef for __TText
#endif
public:
    /**
    @internalComponent
    */
	TUint iTypeLength;

    /**
    @internalComponent
    */
	__TText iBuf[__Align(S)];
	};


/**
@publishedAll
@released

Defines an empty or null literal descriptor.

This is the build independent form.
An 8 bit build variant is generated for a non-Unicode build;
a 16 bit build variant is generated for a Unicode build.
*/
_LIT(KNullDesC,"");



/**
@publishedAll
@released

Defines an empty or null literal descriptor for use with 8-bit descriptors.
*/
_LIT8(KNullDesC8,"");
#ifndef __KERNEL_MODE__



/**
@publishedAll
@released

Defines an empty or null literal descriptor for use with 16-bit descriptors
*/
_LIT16(KNullDesC16,"");
#endif




/**
@publishedAll
@released

Packages a non-modifiable pointer descriptor which represents an object of 
specific type.

The template parameter defines the type of object.

The object represented by the packaged pointer descriptor is accessible through 
the package but cannot be changed. */
template <class T>
class TPckgC : public TPtrC8
	{
public:
	inline TPckgC(const T& aRef);
	inline const T& operator()() const;
private:
	TPckgC<T>& operator=(const TPckgC<T>& aRef);
	};




/**
@publishedAll
@released

Packages a modifiable pointer descriptor which represents an object of specific 
type.

The template parameter defines the type of object.

The object represented by the packaged pointer descriptor is accessible through 
the package.
*/
template <class T>
class TPckg : public TPtr8
	{
public:
	inline TPckg(const T& aRef);
	inline T& operator()();
private:
	TPckg<T>& operator=(const TPckg<T>& aRef);
	};




/**
@publishedAll
@released

Packages an object into a modifiable buffer descriptor.

The template parameter defines the type of object to be packaged.

The package provides a type safe way of transferring an object or data structure 
which is contained within a modifiable buffer descriptor. Typically, a package 
is used for passing data via inter thread communication.

The contained object is accessible through the package.
*/
template <class T>
class TPckgBuf : public TAlignedBuf8<sizeof(T)>
	{
public:
	inline TPckgBuf();
	inline TPckgBuf(const T& aRef);
	inline TPckgBuf& operator=(const TPckgBuf<T>& aRef);
	inline T& operator=(const T& aRef);
	inline T& operator()();
	inline const T& operator()() const;
	};




/**
@publishedAll
@released

Defines a modifiable buffer descriptor that can contain the name of a reference 
counting object.

@see TBuf
@see CObject
*/
typedef TBuf<KMaxName> TName;


/**
@publishedAll
@released

Defines a modifiable buffer descriptor that can contain the full name of a 
reference counting object.

@see TBuf
@see CObject
*/
typedef TBuf<KMaxFullName> TFullName;



/**
@publishedAll
@released

Defines a modifiable buffer descriptor to contain the category name identifying
the cause of thread or process termination. The buffer takes a maximum length
of KMaxExitCategoryName.

@see RThread::ExitCategory
@see RThread::ExitCategory
*/
typedef TBuf<KMaxExitCategoryName> TExitCategoryName;



/**
@publishedAll
@released

A buffer that can contain the name of a file.
The name can have a maximum length of KMaxFileName
(currently 256 but check the definition of KMaxFileName).

@see KMaxFileName
*/
typedef TBuf<KMaxFileName> TFileName;



/**
@publishedAll
@released

A buffer that can contain the name of a path.
The name can have a maximum length of KMaxPath
(currently 256 but check the definition of KMaxPath).

@see KMaxPath
*/
typedef TBuf<KMaxPath> TPath;




/**
@publishedAll
@released

Version name type.

This is a buffer descriptor with a maximum length of KMaxVersionName.
A TVersion object returns the formatted character representation of its version
information in a descriptor of this type.

@see TVersion
*/
typedef TBuf<KMaxVersionName> TVersionName;




/**
@publishedAll
@released

Defines a modifiable buffer descriptor for the text form of the UID.
The descriptor has a maximum length of KMaxUidName and is used to contain
the standard text format returned by the function TUid::Name().

@see TUid::Name
*/
typedef TBuf<KMaxUidName> TUidName;




/**
@publishedAll
@released

Defines a null UID
*/
#define KNullUid TUid::Null()




/**
@publishedAll
@released

A globally unique 32-bit number.
*/
class TUid
	{
public:
#ifndef __KERNEL_MODE__
	IMPORT_C TBool operator==(const TUid& aUid) const;
	IMPORT_C TBool operator!=(const TUid& aUid) const;
	IMPORT_C TUidName Name() const;
#endif
	static inline TUid Uid(TInt aUid);
	static inline TUid Null();
public:
	/**
	The 32-bit integer UID value.
	*/
	TInt32 iUid;
	};




/**
@publishedAll
@released

Encapsulates a set of three unique identifiers (UIDs) which, in combination, 
identify a system object such as a GUI application or a DLL. The three
component UIDs are referred to as UID1, UID2 and UID3.

An object of this type is referred to as a compound identifier or a UID type.
*/
class TUidType
	{
public:
#ifndef __KERNEL_MODE__
	IMPORT_C TUidType();
	IMPORT_C TUidType(TUid aUid1);
	IMPORT_C TUidType(TUid aUid1,TUid aUid2);
	IMPORT_C TUidType(TUid aUid1,TUid aUid2,TUid aUid3);
	IMPORT_C TBool operator==(const TUidType& aUidType) const;
	IMPORT_C TBool operator!=(const TUidType& aUidType) const;
	IMPORT_C const TUid& operator[](TInt anIndex) const;
	IMPORT_C TUid MostDerived() const;
	IMPORT_C TBool IsPresent(TUid aUid) const;
	IMPORT_C TBool IsValid() const;
private:
#endif
	TUid iUid[KMaxCheckedUid];
	};




/**
A class used to represent the Secure ID of a process or executable image.

Constructors and conversion operators are provided to enable conversion
of this class to and from both TUint32 and TUid objects.

Because this class has non-default constructors, compilers will not initialise
this objects at compile time, instead code will be generated to construct the object
at run-time. This is wastefull, and Symbian OS DLLs are not permitted to have
such uninitialised data. To overcome these problems a macro is provided to construct
a const object which behaves like a TSecureId. This is _LIT_SECURE_ID.
This macro should be used where it is desirable to define const TSecureId objects,
like in header files. E.g. Instead of writing:
@code
	const TSecureId MyId=0x1234567
@endcode
use
@code
	_LIT_SECURE_ID(MyId,0x1234567)
@endcode

@publishedAll
@released

@see _LIT_SECURE_ID
*/
class TSecureId
	{
public:
	inline TSecureId();
	inline TSecureId(TUint32 aId);
	inline operator TUint32() const;
	inline TSecureId(TUid aId);
	inline operator TUid() const;
public:
	TUint32 iId;
	};




/**
A class used to represent the Vendor ID of a process or executable image

Constructors and conversion operators are provided to enable conversion
of this class to and from both TUint32 and TUid objects.

Because this class has non-default constructors, compilers will not initialise
this objects at compile time, instead code will be generated to construct the object
at run-time. This is wastefull, and Symbian OS DLLs are not permitted to have
such uninitialised data. To overcome these problems a macro is provided to construct
a const object which behaves like a TSecureId. This is _LIT_VENDOR_ID.
This macro should be used where it is desirable to define const TSecureId objects,
like in header files. E.g. Instead of writing:
@code
	const TVendorId MyId=0x1234567
@endcode
use
@code
	_LIT_VENDOR_ID(MyId,0x1234567)
@endcode

@publishedAll
@released

@see _LIT_VENDOR_ID
*/
class TVendorId
	{
public:
	inline TVendorId();
	inline TVendorId(TUint32 aId);
	inline operator TUint32() const;
	inline TVendorId(TUid aId);
	inline operator TUid() const;
public:
	TUint32 iId;
	};



/**
Structure for compile-time definition of a secure ID
@internalComponent
*/
class SSecureId
	{
public:
	inline const TSecureId* operator&() const;
	inline operator const TSecureId&() const;
	inline operator TUint32() const;
	inline operator TUid() const;
public:
	TUint32 iId;
	};


	
	
/**
Structure for compile-time definition of a vendor ID
@internalComponent
*/
class SVendorId
	{
public:
	inline const TVendorId* operator&() const;
	inline operator const TVendorId&() const;
	inline operator TUint32() const;
	inline operator TUid() const;
public:
	TUint32 iId;
	};




/**
Macro for compile-time definition of a secure ID
@param name Name to use for secure ID
@param value Value of secure ID
@publishedAll
@released
*/
#define _LIT_SECURE_ID(name,value) const SSecureId name={value}




/**
Macro for compile-time definition of a vendor ID
@param name Name to use for vendor ID
@param value Value of vendor ID
@publishedAll
@released
*/
#define _LIT_VENDOR_ID(name,value) const SVendorId name={value}




/**
@publishedAll
@released

Contains version information.

A version is defined by a set of three numbers:

1. the major version number, ranging from 0 to 127, inclusive

2. the minor version number, ranging from 0 to 99 inclusive

3. the build number, ranging from 0 to 32767 inclusive.

The class provides a constructor for setting all three numbers.
It also provides a member function to build a character representation of
this information in a TVersionName descriptor.

@see TVersionName
*/
class TVersion
	{
public:
	IMPORT_C TVersion();
	IMPORT_C TVersion(TInt aMajor,TInt aMinor,TInt aBuild);
	IMPORT_C TVersionName Name() const;
public:
    /**
    The major version number.
    */
	TInt8 iMajor;


    /**
    The minor version number.
    */
	TInt8 iMinor;

	
	/**
	The build number.
	*/
	TInt16 iBuild;
	};




/**
@publishedAll
@released

Indicates the completion status of a request made to a service provider.

When a thread makes a request, it passes a request status as a parameter. 
On completion, the provider signals the requesting thread's request semaphore 
and stores a completion code in the request status. Typically, this is KErrNone 
or one of the other system-wide error codes.

This class is not intended for user derivation.
*/
class TRequestStatus
	{
public:
	inline TRequestStatus();
	inline TRequestStatus(TInt aVal);
	inline TInt operator=(TInt aVal);
	inline TBool operator==(TInt aVal) const;
	inline TBool operator!=(TInt aVal) const;
	inline TBool operator>=(TInt aVal) const;
	inline TBool operator<=(TInt aVal) const;
	inline TBool operator>(TInt aVal) const;
	inline TBool operator<(TInt aVal) const;
	inline TInt Int() const;
private:
	enum
		{
		EActive				= 1,  //bit0
		ERequestPending		= 2,  //bit1
		};
	TInt iStatus;
	TUint iFlags;
	friend class CActive;
	friend class CActiveScheduler;
	friend class CServer2;
	};




class TSize;
/**
@publishedAll
@released

Stores a two-dimensional point in Cartesian co-ordinates.

Its data members (iX and iY) are public and can be manipulated directly, or 
by means of the functions provided. Functions are provided to set and manipulate 
the point, and to compare points for equality.
*/
class TPoint
	{
public:
#ifndef __KERNEL_MODE__
	enum TUninitialized { EUninitialized };
	/**
	Constructs default point, initialising its iX and iY members to zero.
	*/
	TPoint(TUninitialized) {}
	inline TPoint();
	inline TPoint(TInt aX,TInt aY);
	IMPORT_C TBool operator==(const TPoint& aPoint) const;
	IMPORT_C TBool operator!=(const TPoint& aPoint) const;
	IMPORT_C TPoint& operator-=(const TPoint& aPoint);
	IMPORT_C TPoint& operator+=(const TPoint& aPoint);
	IMPORT_C TPoint& operator-=(const TSize& aSize);
	IMPORT_C TPoint& operator+=(const TSize& aSize);
	IMPORT_C TPoint operator-(const TPoint& aPoint) const;
	IMPORT_C TPoint operator+(const TPoint& aPoint) const;
	IMPORT_C TPoint operator-(const TSize& aSize) const;
	IMPORT_C TPoint operator+(const TSize& aSize) const;
	IMPORT_C TPoint operator-() const;
	IMPORT_C void SetXY(TInt aX,TInt aY);
	IMPORT_C TSize AsSize() const;
#endif
public:
	/**
	The x co-ordinate.
	*/
	TInt iX;
	/**
	The y co-ordinate.
	*/
	TInt iY;
	};




/**
@publishedAll
@prototype

Stores a three-dimensional point in Cartesian or polar co-ordinates.
Its data members (iX, iY and iZ) are public and can be manipulated directly.

*/
class TPoint3D
	{
public:
#ifndef __KERNEL_MODE__
	enum TUninitialized { EUninitialized };

	/**
	TUninitialized Constructor
	*/
	TPoint3D(TUninitialized) {}
	/**
	Constructs default TPoint3D, initialising its iX , iY and iZ members to zero.
	*/
	inline TPoint3D();
	/**
	Constructs  TPoint3D with the specified x,y  and z co-ordinates.
	*/
	inline TPoint3D(TInt aX,TInt aY,TInt aZ);
	/** 
	Copy Construct from TPoint , initialises Z co-ordinate to  Zero
	*/
	inline TPoint3D(const  TPoint& aPoint);

	IMPORT_C TBool operator==(const TPoint3D& aPoint3D) const;
	IMPORT_C TBool operator!=(const TPoint3D& aPoint3D) const;

	IMPORT_C TPoint3D& operator-=(const TPoint3D& aPoint3D);
	IMPORT_C TPoint3D& operator-=(const TPoint& aPoint);

	IMPORT_C TPoint3D& operator+=(const TPoint3D& aPoint3D);	
	IMPORT_C TPoint3D& operator+=(const TPoint& aPoint);

	IMPORT_C TPoint3D operator-(const TPoint3D& aPoint3D) const;
	IMPORT_C TPoint3D operator-(const TPoint& aPoint) const;	

	IMPORT_C TPoint3D operator+(const TPoint3D& aPoint3D) const;
	IMPORT_C TPoint3D operator+(const TPoint& aPoint) const;
	/**
    Unary minus operator. The operator returns the negation of this Point3D 
	*/
	IMPORT_C TPoint3D operator-() const;
	
	/**
	Set Method to set the xyz co-ordinates of TPoint3D
	*/
	IMPORT_C void SetXYZ(TInt aX,TInt aY,TInt aZ);
	
	/**
	TPoint3D from TPoint, sets the Z co-ordinate to  Zero
	*/
	IMPORT_C void SetPoint(const TPoint& aPoint);

	/**
	Returns TPoint from TPoint3D
	*/
	IMPORT_C TPoint AsPoint() const;
#endif
public:
	/**
	The x co-ordinate.
	*/
	TInt iX;
	/**
	The y co-ordinate.
	*/
	TInt iY;
	/**
	The z co-ordinate.
	*/
	TInt iZ;
	};



/**
@internalTechnology
@prototype For now, only intended to be used by TRwEvent and the Windows Server

Stores the angular spherical coordinates (Phi,Theta) of a three-dimensional point.

Its data members (iPhi, iTheta) are public and can be manipulated directly.
*/
class TAngle3D
	{
public:
	/**
	The Phi co-ordinate (angle between X-axis and the line that links the projection of the point on the X-Y plane and the origin).
	*/
	TInt iPhi;
	/**
	The Theta co-ordinate (angle between the Z-axis and the line that links the point and the origin).
	*/
	TInt iTheta;
	};

	
/**
@publishedAll
@released

Stores a two-dimensional size as a width and a height value.

Its data members are public and can be manipulated directly, or by means of 
the functions provided.
*/
class TSize
	{
public:
#ifndef __KERNEL_MODE__
	enum TUninitialized { EUninitialized };
	/**
	Constructs the size object with its iWidth and iHeight members set to zero.
	*/
	TSize(TUninitialized) {}
	inline TSize();
	inline TSize(TInt aWidth,TInt aHeight);
	IMPORT_C TBool operator==(const TSize& aSize) const;
	IMPORT_C TBool operator!=(const TSize& aSize) const;
	IMPORT_C TSize& operator-=(const TSize& aSize);
	IMPORT_C TSize& operator-=(const TPoint& aPoint);
	IMPORT_C TSize& operator+=(const TSize& aSize);
	IMPORT_C TSize& operator+=(const TPoint& aPoint);
	IMPORT_C TSize operator-(const TSize& aSize) const;
	IMPORT_C TSize operator-(const TPoint& aPoint) const;
	IMPORT_C TSize operator+(const TSize& aSize) const;
	IMPORT_C TSize operator+(const TPoint& aPoint) const;
	IMPORT_C TSize operator-() const;
	IMPORT_C void SetSize(TInt aWidth,TInt aHeight);
	IMPORT_C TPoint AsPoint() const;
#endif
public:
	/**
	The width of this TSize object.
	*/
	TInt iWidth;
	/**
	The height of this TSize object.
	*/
	TInt iHeight;
	};




/**
@publishedAll
@released

Information about a kernel object.

This type of object is passed to RHandleBase::HandleInfo(). The function 
fetches information on the usage of the kernel object associated with that 
handle and stores the information in the THandleInfo object.

The class contains four data members and no explicitly defined function
members.
*/
class THandleInfo
	{
public:
	/**
	The number of times that the kernel object is open in the current process.
	*/
	TInt iNumOpenInProcess;
	
	/**
	The number of times that the kernel object is open in the current thread.
	*/
	TInt iNumOpenInThread;
	
	/**
	The number of processes which have a handle on the kernel object.
	*/
	TInt iNumProcesses;
	
	/**
	The number of threads which have a handle on the kernel object.
	*/
	TInt iNumThreads;
	};




/**
@internalComponent
*/
class TFindHandle
	{
public:
	inline TFindHandle();
	inline TInt Handle() const;
#ifdef __KERNEL_MODE__
	inline TInt Index() const;
	inline TInt UniqueID() const;
	inline TUint64 ObjectID() const;
	inline void Set(TInt aIndex, TInt aUniqueId, TUint64 aObjectId);
#else
protected:
	inline void Reset();
#endif
private:
	TInt iHandle;
	TInt iSpare1;
	TInt iObjectIdLow;
	TInt iObjectIdHigh;
	};



class RThread;
class TFindHandleBase;
class TFindSemaphore;
/**
@publishedAll
@released

A handle to an object.

The class encapsulates the basic behaviour of a handle, hiding the
handle-number which identifies the object which the handle represents.

The class is abstract in the sense that a RHandleBase object is never
explicitly instantiated. It is always a base class to a concrete handle class;
for example, RSemaphore, RThread, RProcess, RCriticalSection etc.
*/
class RHandleBase
	{
public:
    /**
    @publishedAll
    @released

	Read/Write attributes for the handle.
    */
    enum TAttributes
		{
		EReadAccess=0x1,
		EWriteAccess=0x2,
		EDirectReadAccess=0x4,
		EDirectWriteAccess=0x8,
		};
public:
	inline RHandleBase();
	inline TInt Handle() const;
	inline void SetHandle(TInt aHandle);
	inline TInt SetReturnedHandle(TInt aHandleOrError);	
	static void DoExtendedClose();
#ifndef __KERNEL_MODE__
	IMPORT_C void Close();
	IMPORT_C TName Name() const;
	IMPORT_C TFullName FullName() const;
	IMPORT_C void FullName(TDes& aName) const;
	IMPORT_C void SetHandleNC(TInt aHandle);
	IMPORT_C TInt Duplicate(const RThread& aSrc,TOwnerType aType=EOwnerProcess);
	IMPORT_C void HandleInfo(THandleInfo* anInfo);
	IMPORT_C TUint Attributes() const;
	IMPORT_C TInt BTraceId() const;
	IMPORT_C void NotifyDestruction(TRequestStatus& aStatus);	/**< @internalTechnology */
protected:
	inline RHandleBase(TInt aHandle);
	IMPORT_C TInt Open(const TFindHandleBase& aHandle,TOwnerType aType);
	static TInt SetReturnedHandle(TInt aHandleOrError,RHandleBase& aHandle);
	TInt OpenByName(const TDesC &aName,TOwnerType aOwnerType,TInt aObjectType);
#endif
private:
	static void DoExtendedCloseL();
protected:
	TInt iHandle;
	};




class RMessagePtr2;
/**
@publishedAll
@released

A handle to a semaphore.

The semaphore itself is a Kernel side object.

As with all handles, they should be closed after use. RHandleBase provides 
the necessary Close() function, which should be called when the handle is 
no longer required.

@see RHandleBase::Close
*/
class RSemaphore : public RHandleBase
	{
public:
#ifndef __KERNEL_MODE__
	inline TInt Open(const TFindSemaphore& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateLocal(TInt aCount,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateGlobal(const TDesC& aName,TInt aCount,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt OpenGlobal(const TDesC& aName,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	IMPORT_C void Wait();
	IMPORT_C TInt Wait(TInt aTimeout);	// timeout in microseconds
	IMPORT_C void Signal();
	IMPORT_C void Signal(TInt aCount);
#endif
	};




/**
@publishedAll
@released

A fast semaphore.

This is a layer over a standard semaphore, and only calls into the kernel side
if there is contention.
*/
class RFastLock : public RSemaphore
	{
public:
	inline RFastLock();
	IMPORT_C TInt CreateLocal(TOwnerType aType=EOwnerProcess);
	IMPORT_C void Wait();
	IMPORT_C void Signal();
private:
	TInt iCount;
	};




/**
@publishedAll
@released

A read-write lock.

This is a lock for co-ordinating readers and writers to shared resources.
It is designed to allow multiple concurrent readers.
It is not a kernel side object and so does not inherit from RHandleBase.
*/
class RReadWriteLock
	{
public:
	enum TReadWriteLockPriority
		{
		/** Pending writers always get the lock before pending readers */
		EWriterPriority,
		/** Lock is given alternately to pending readers and writers */
		EAlternatePriority,
		/** Pending readers always get the lock before pending writers - beware writer starvation! */
		EReaderPriority,
		};
	enum TReadWriteLockClientCategoryLimit
		{
		/** Maximum number of clients in each category: read locked, read lock pending, write lock pending */
		EReadWriteLockClientCategoryLimit = KMaxTUint16
		};

public:
	inline RReadWriteLock();
	IMPORT_C TInt CreateLocal(TReadWriteLockPriority aPriority = EWriterPriority);
	IMPORT_C void Close();

	IMPORT_C void ReadLock();
	IMPORT_C void WriteLock();
	IMPORT_C TBool TryReadLock();
	IMPORT_C TBool TryWriteLock();
	IMPORT_C TBool TryUpgradeReadLock();
	IMPORT_C void DowngradeWriteLock();
	IMPORT_C void Unlock();

private:
	RReadWriteLock(const RReadWriteLock& aLock);
	RReadWriteLock& operator=(const RReadWriteLock& aLock);

	TInt UnlockWriter();
	TInt UnlockAlternate();
	TInt UnlockReader();

private:
	volatile TUint64 iValues; // Bits 0-15: readers; bit 16: writer; bits 32-47: readersPending; bits 48-63: writersPending
	TReadWriteLockPriority iPriority;
	RSemaphore iReaderSem;
	RSemaphore iWriterSem;
	TUint32 iSpare[4]; // Reserved for future development
	};




/**
@publishedAll
@released

The user-side handle to a logical channel.

The class provides functions that are used to open a channel
to a device driver, and to make requests. A device driver provides
a derived class to give the user-side a tailored interface to the driver.
*/
class RBusLogicalChannel : public RHandleBase
	{
public:
	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
protected:
	inline TInt DoCreate(const TDesC& aDevice, const TVersion& aVer, TInt aUnit, const TDesC* aDriver, const TDesC8* anInfo, TOwnerType aType=EOwnerProcess, TBool aProtected=EFalse);
	IMPORT_C void DoCancel(TUint aReqMask);
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus);
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TAny* a1);
	IMPORT_C void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TAny* a1,TAny* a2);
	IMPORT_C TInt DoControl(TInt aFunction);
	IMPORT_C TInt DoControl(TInt aFunction,TAny* a1);
	IMPORT_C TInt DoControl(TInt aFunction,TAny* a1,TAny* a2);
	inline TInt DoSvControl(TInt aFunction) { return DoControl(aFunction); }
	inline TInt DoSvControl(TInt aFunction,TAny* a1) { return DoControl(aFunction, a1); }
	inline TInt DoSvControl(TInt aFunction,TAny* a1,TAny* a2) { return DoControl(aFunction, a1, a2); }
private:
	IMPORT_C TInt DoCreate(const TDesC& aDevice, const TVersion& aVer, TInt aUnit, const TDesC* aDriver, const TDesC8* aInfo, TInt aType);
private:
	// Padding for Binary Compatibility purposes
	TInt iPadding1;
	TInt iPadding2;
	};




/**
@internalComponent

Base class for memory allocators.
*/
// Put pure virtual functions into a separate base class so that vptr is at same
// place in both GCC98r2 and EABI builds.
class MAllocator
	{
public:
	virtual TAny* Alloc(TInt aSize)=0;
	virtual void Free(TAny* aPtr)=0;
	virtual TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0)=0;
	virtual TInt AllocLen(const TAny* aCell) const =0;
	virtual TInt Compress()=0;
	virtual void Reset()=0;
	virtual TInt AllocSize(TInt& aTotalAllocSize) const =0;
	virtual TInt Available(TInt& aBiggestBlock) const =0;
	virtual TInt DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL)=0;
	virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)=0;
	};




/**
@publishedAll
@released

Base class for heaps.
*/
class RAllocator : public MAllocator
	{
public:


    /**
    A set of heap allocation failure flags.
    
    This enumeration indicates how to simulate heap allocation failure.

    @see RAllocator::__DbgSetAllocFail()
    */
	enum TAllocFail {
                    /**
                    Attempts to allocate from this heap fail at a random rate;
                    however, the interval pattern between failures is the same
                    every time simulation is started.
                    */
	                ERandom,
	                
	                
                  	/**
                  	Attempts to allocate from this heap fail at a random rate.
                  	The interval pattern between failures may be different every
                  	time the simulation is started.
                  	*/
	                ETrueRandom,
	                
	                
                    /**
                    Attempts to allocate from this heap fail at a rate aRate;
                    for example, if aRate is 3, allocation fails at every
                    third attempt.
                    */
	                EDeterministic,

	                
	                /**
	                Cancels simulated heap allocation failure.
	                */
	                ENone,
	                
	                
	                /**
	                An allocation from this heap will fail after the next aRate - 1 
					allocation attempts. For example, if aRate = 1 then the next 
					attempt to allocate from this heap will fail.
	                */
	                EFailNext,
	                
	                /**
	                Cancels simulated heap allocation failure, and sets
	                the nesting level for all allocated cells to zero.
	                */
	                EReset,

                    /**
                    aBurst allocations from this heap fail at a random rate;
                    however, the interval pattern between failures is the same
                    every time the simulation is started.
                    */
	                EBurstRandom,
	                
	                
                  	/**
                  	aBurst allocations from this heap fail at a random rate.
                  	The interval pattern between failures may be different every
                  	time the simulation is started.
                  	*/
	                EBurstTrueRandom,
	                
	                
                    /**
                    aBurst allocations from this heap fail at a rate aRate.
                    For example, if aRate is 10 and aBurst is 2, then 2 allocations
					will fail at every tenth attempt.
                    */
	                EBurstDeterministic,

	                /**
	                aBurst allocations from this heap will fail after the next aRate - 1 
					allocation attempts have occurred. For example, if aRate = 1 and 
					aBurst = 3 then the next 3 attempts to allocate from this heap will fail.
	                */
	                EBurstFailNext,

					/**
					Use this to determine how many times the current debug 
					failure mode has failed so far.
					@see RAllocator::__DbgCheckFailure()
					*/
					ECheckFailure,
	                };
	                
	                
    /**
    Heap debug checking type flag.
    */
	enum TDbgHeapType {
                      /**
                      The heap is a user heap.
                      */
	                  EUser,
	                  
                      /**
                      The heap is the Kernel heap.
                      */	                  
	                  EKernel
	                  };
	                  
	                  
	enum TAllocDebugOp {ECount, EMarkStart, EMarkEnd, ECheck, ESetFail, ECopyDebugInfo, ESetBurstFail};
	
	
	/**
	Flags controlling reallocation.
	*/
	enum TReAllocMode {
	                  /**
	                  A reallocation of a cell must not change
	                  the start address of the cell.
	                  */
	                  ENeverMove=1,
	                  
	                  /**
	                  Allows the start address of the cell to change
	                  if the cell shrinks in size.
	                  */
	                  EAllowMoveOnShrink=2
	                  };
	                  
	                  
	enum TFlags {ESingleThreaded=1, EFixedSize=2, ETraceAllocs=4, EMonitorMemory=8,};
	struct SCheckInfo {TBool iAll; TInt iCount; const TDesC8* iFileName; TInt iLineNum;};
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
	struct SRAllocatorBurstFail {TInt iBurst; TInt iRate; TInt iUnused[2];};
#endif
	enum {EMaxHandles=32};

public:
	inline RAllocator();
#ifndef __KERNEL_MODE__
	IMPORT_C TInt Open();
	IMPORT_C void Close();
	IMPORT_C TAny* AllocZ(TInt aSize);
	IMPORT_C TAny* AllocZL(TInt aSize);
	IMPORT_C TAny* AllocL(TInt aSize);
	IMPORT_C TAny* AllocLC(TInt aSize);
	IMPORT_C void FreeZ(TAny*& aCell);
	IMPORT_C TAny* ReAllocL(TAny* aCell, TInt aSize, TInt aMode=0);
	IMPORT_C TInt Count() const;
	IMPORT_C TInt Count(TInt& aFreeCount) const;
#endif
	UIMPORT_C void Check() const;
	UIMPORT_C void __DbgMarkStart();
	UIMPORT_C TUint32 __DbgMarkEnd(TInt aCount);
	UIMPORT_C TInt __DbgMarkCheck(TBool aCountAll, TInt aCount, const TDesC8& aFileName, TInt aLineNum);
	inline void __DbgMarkCheck(TBool aCountAll, TInt aCount, const TUint8* aFileName, TInt aLineNum);
	UIMPORT_C void __DbgSetAllocFail(TAllocFail aType, TInt aRate);
	UIMPORT_C void __DbgSetBurstAllocFail(TAllocFail aType, TUint aRate, TUint aBurst);
	UIMPORT_C TUint __DbgCheckFailure();
protected:
	UIMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
#ifndef __KERNEL_MODE__
	IMPORT_C virtual void DoClose();
#endif
protected:
	TInt iAccessCount;
	TInt iHandleCount;
	TInt* iHandles;
	TUint32 iFlags;
	TInt iCellCount;
	TInt iTotalAllocSize;
	};




class UserHeap;
/**
@publishedAll
@released

Represents the default implementation for a heap.

The default implementation uses an address-ordered first fit type algorithm.

The heap itself is contained in a chunk and may be the only occupant of the 
chunk or may share the chunk with the program stack.

The class contains member functions for allocating, adjusting, freeing individual 
cells and generally managing the heap.

The class is not a handle in the same sense that RChunk is a handle; i.e. 
there is no Kernel object which corresponds to the heap.
*/
class RHeap : public RAllocator
	{
public:
    /**
    The structure of a heap cell header for a heap cell on the free list.
    */
	struct SCell {
	             /**
	             The length of the cell, which includes the length of
	             this header.
	             */
	             TInt len; 
	             
	             
	             /**
	             A pointer to the next cell in the free list.
	             */
	             SCell* next;
	             };


	/**
    The structure of a heap cell header for an allocated heap cell in a debug build.
    */             
	struct SDebugCell {
	                  /**
	                  The length of the cell, which includes the length of
                      this header.
	                  */
	                  TInt len;
	                  
	                  
	                  /**
	                  The nested level.
	                  */
	                  TInt nestingLevel;
	                  
	                  
	                  /**
	                  The cumulative number of allocated cells
	                  */
	                  TInt allocCount;
	                  };

	/**
    @internalComponent
    */
	struct SHeapCellInfo { RHeap* iHeap; TInt iTotalAlloc;	TInt iTotalAllocSize; TInt iTotalFree; TInt iLevelAlloc; SDebugCell* iStranded; };

	/**
	@internalComponent
	*/
	struct _s_align {char c; double d;};

	/** 
	The default cell alignment.
	*/
	enum {ECellAlignment = sizeof(_s_align)-sizeof(double)};
	
	/**
	Size of a free cell header.
	*/
	enum {EFreeCellSize = sizeof(SCell)};


#ifdef _DEBUG
    /**
    Size of an allocated cell header in a debug build.
    */
	enum {EAllocCellSize = sizeof(SDebugCell)};
#else
    /**
    Size of an allocated cell header in a release build.
    */
	enum {EAllocCellSize = sizeof(SCell*)};
#endif


    /**
    @internalComponent
    */
	enum TDebugOp {EWalk=128};
	
	
    /**
    @internalComponent
    */
	enum TCellType
		{EGoodAllocatedCell, EGoodFreeCell, EBadAllocatedCellSize, EBadAllocatedCellAddress,
		EBadFreeCellAddress, EBadFreeCellSize};

		
    /**
    @internalComponent
    */
	enum TDebugHeapId {EUser=0, EKernel=1};
    
    /**
    @internalComponent
    */
    enum TDefaultShrinkRatios {EShrinkRatio1=256, EShrinkRatioDflt=512};

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
	/**
    @internalComponent
    */
#else
private:
#endif
    typedef void (*TWalkFunc)(TAny*, TCellType, TAny*, TInt);

public:
	UIMPORT_C virtual TAny* Alloc(TInt aSize);
	UIMPORT_C virtual void Free(TAny* aPtr);
	UIMPORT_C virtual TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
	UIMPORT_C virtual TInt AllocLen(const TAny* aCell) const;
#ifndef __KERNEL_MODE__
	UIMPORT_C virtual TInt Compress();
	UIMPORT_C virtual void Reset();
	UIMPORT_C virtual TInt AllocSize(TInt& aTotalAllocSize) const;
	UIMPORT_C virtual TInt Available(TInt& aBiggestBlock) const;
#endif
	UIMPORT_C virtual TInt DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL);
protected:
	UIMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
public:
	UIMPORT_C RHeap(TInt aMaxLength, TInt aAlign=0, TBool aSingleThread=ETrue);
	UIMPORT_C RHeap(TInt aChunkHandle, TInt aOffset, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign=0, TBool aSingleThread=EFalse);
	UIMPORT_C TAny* operator new(TUint aSize, TAny* aBase) __NO_THROW;
	inline void operator delete(TAny* aPtr, TAny* aBase);
	inline TUint8* Base() const;
	inline TInt Size() const;
	inline TInt MaxLength() const;
	inline TInt Align(TInt a) const;
	inline const TAny* Align(const TAny* a) const;
	inline TBool IsLastCell(const SCell* aCell) const;
	inline void Lock() const;
	inline void Unlock() const;
	inline TInt ChunkHandle() const;
protected:
	inline RHeap();
	void Initialise();
	SCell* DoAlloc(TInt aSize, SCell*& aLastFree);
	void DoFree(SCell* pC);
	TInt TryToGrowHeap(TInt aSize, SCell* aLastFree);
	inline void FindFollowingFreeCell(SCell* aCell, SCell*& pPrev, SCell*& aNext);
	TInt TryToGrowCell(SCell* pC, SCell* pP, SCell* pE, TInt aSize);
	TInt Reduce(SCell* aCell);
	UIMPORT_C SCell* GetAddress(const TAny* aCell) const;
	void CheckCell(const SCell* aCell) const;
	void Walk(TWalkFunc aFunc, TAny* aPtr);
	static void WalkCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
	TInt DoCountAllocFree(TInt& aFree);
	TInt DoCheckHeap(SCheckInfo* aInfo);
	void DoMarkStart();
	TUint32 DoMarkEnd(TInt aExpected);
	void DoSetAllocFail(TAllocFail aType, TInt aRate);
	TBool CheckForSimulatedAllocFail();
	inline TInt SetBrk(TInt aBrk);
	inline TAny* ReAllocImpl(TAny* aPtr, TInt aSize, TInt aMode);
	void DoSetAllocFail(TAllocFail aType, TInt aRate, TUint aBurst);
protected:
	TInt iMinLength;
	TInt iMaxLength;
	TInt iOffset;
	TInt iGrowBy;
	TInt iChunkHandle;
	RFastLock iLock;
	TUint8* iBase;
	TUint8* iTop;
	TInt iAlign;
	TInt iMinCell;
	TInt iPageSize;
	SCell iFree;
protected:
	TInt iNestingLevel;
	TInt iAllocCount;
	TAllocFail iFailType;
	TInt iFailRate;
	TBool iFailed;
	TInt iFailAllocCount;
	TInt iRand;
	TAny* iTestData;

	friend class UserHeap;
	};





class OnlyCreateWithNull;

/** @internalTechnology */
typedef void (OnlyCreateWithNull::* __NullPMF)();

/** @internalTechnology */
class OnlyCreateWithNull
	{
public:
	inline OnlyCreateWithNull(__NullPMF /*aPointerToNull*/) {}
	};

/**
@publishedAll
@released

A handle to a message sent by the client to the server.

A server's interaction with its clients is channelled through an RMessagePtr2
object, which acts as a handle to a message sent by the client.
The details of the original message are kept by the kernel allowing it enforce
correct usage of the member functions of this class.

@see RMessage2
*/
class RMessagePtr2
	{
public:
	inline RMessagePtr2();
	inline TBool IsNull() const;
	inline TInt Handle() const;
#ifndef __KERNEL_MODE__
	IMPORT_C void Complete(TInt aReason) const;
	IMPORT_C void Complete(RHandleBase aHandle) const;
	IMPORT_C TInt GetDesLength(TInt aParam) const;
	IMPORT_C TInt GetDesLengthL(TInt aParam) const;
	IMPORT_C TInt GetDesMaxLength(TInt aParam) const;
	IMPORT_C TInt GetDesMaxLengthL(TInt aParam) const;
	IMPORT_C void ReadL(TInt aParam,TDes8& aDes,TInt aOffset=0) const;
	IMPORT_C void ReadL(TInt aParam,TDes16 &aDes,TInt aOffset=0) const;
	IMPORT_C void WriteL(TInt aParam,const TDesC8& aDes,TInt aOffset=0) const;
	IMPORT_C void WriteL(TInt aParam,const TDesC16& aDes,TInt aOffset=0) const;
	IMPORT_C TInt Read(TInt aParam,TDes8& aDes,TInt aOffset=0) const;
	IMPORT_C TInt Read(TInt aParam,TDes16 &aDes,TInt aOffset=0) const;
	IMPORT_C TInt Write(TInt aParam,const TDesC8& aDes,TInt aOffset=0) const;
	IMPORT_C TInt Write(TInt aParam,const TDesC16& aDes,TInt aOffset=0) const;
	IMPORT_C void Panic(const TDesC& aCategory,TInt aReason) const;
	IMPORT_C void Kill(TInt aReason) const;
	IMPORT_C void Terminate(TInt aReason) const;
	IMPORT_C TInt SetProcessPriority(TProcessPriority aPriority) const;
	inline   void SetProcessPriorityL(TProcessPriority aPriority) const;
	IMPORT_C TInt Client(RThread& aClient, TOwnerType aOwnerType=EOwnerProcess) const;
	inline   void ClientL(RThread& aClient, TOwnerType aOwnerType=EOwnerProcess) const;
	IMPORT_C TUint ClientProcessFlags() const;
	IMPORT_C const TRequestStatus* ClientStatus() const;
	IMPORT_C TBool ClientIsRealtime() const;
	
	/**
	Return the Secure ID of the process which sent this message.

	If an intended use of this method is to check that the Secure ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RMessagePtr2& message;
		TInt error = message.SecureId()==KRequiredSecureId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RMessagePtr2& message;
		static _LIT_SECURITY_POLICY_S0(mySidPolicy, KRequiredSecureId);
		TBool pass = mySidPolicy().CheckPolicy(message);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_S0

	@return The Secure ID.

	@publishedAll
	@released
	*/
	IMPORT_C TSecureId SecureId() const;

	/**
	Return the Vendor ID of the process which sent this message.

	If an intended use of this method is to check that the Vendor ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RMessagePtr2& message;
		TInt error = message.VendorId()==KRequiredVendorId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RMessagePtr2& message;
		static _LIT_SECURITY_POLICY_V0(myVidPolicy, KRequiredVendorId);
		TBool pass = myVidPolicy().CheckPolicy(message);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_V0

	@return The Vendor ID.
	@publishedAll
	@released
	*/
	IMPORT_C TVendorId VendorId() const;

	/**
	Check if the process which sent this message has a given capability.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability The capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds the capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if process which sent this message has the capability, EFalse otherwise.
	@publishedAll
	@released
	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool HasCapability(TCapability aCapability, const char* aDiagnostic=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool HasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline TBool HasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

 	/**
	Check if the process which sent this message has a given capability.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will not leave even though the
	check failed.

 	@param aCapability The capability to test.
 	@param aDiagnosticMessage A string that will be emitted along with any diagnostic message
 								that may be issued if the test finds the capability is not present.
 								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
 								which enables it to be easily removed from the system.
 	@leave KErrPermissionDenied, if the process does not have the capability.
 	@publishedAll
 	@released
 	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
 	inline void HasCapabilityL(TCapability aCapability, const char* aDiagnosticMessage=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
 	inline void HasCapabilityL(TCapability aCapability, OnlyCreateWithNull aDiagnosticMessage=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline void HasCapabilityL(TCapability aCapability, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	/**
	Check if the process which sent this message has both of the given capabilities.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability1 The first capability to test.
	@param aCapability2 The second capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds a capability is not present.
								This string should be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the process which sent this message has both the capabilities, EFalse otherwise.
	@publishedAll
	@released
	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool HasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull aDiagnostic=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline TBool HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

 	/**
	Check if the process which sent this message has both of the given capabilities.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will not leave even though the
	check failed.

 	@param aCapability1 The first capability to test.
 	@param aCapability2 The second capability to test.
 	@param aDiagnosticMessage A string that will be emitted along with any diagnostic message
 								that may be issued if the test finds a capability is not present.
 								This string should be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
 								which enables it to be easily removed from the system.
 	@leave KErrPermissionDenied, if the process does not have the capabilities.
 	@publishedAll
 	@released
 	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline void HasCapabilityL(TCapability aCapability1, TCapability aCapability2, const char* aDiagnosticMessage=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline void HasCapabilityL(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull aDiagnosticMessage=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline void HasCapabilityL(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	/**
	@deprecated Use SecureId()
	*/
	inline TUid Identity() const { return SecureId(); }
#endif

private:
	// Implementations of functions with diagnostics
	IMPORT_C TBool DoHasCapability(TCapability aCapability, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability, TCapability aCapability2, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability, TCapability aCapability2) const;

protected:
	TInt iHandle;
	};
inline TBool operator==(RMessagePtr2 aLeft,RMessagePtr2 aRight);
inline TBool operator!=(RMessagePtr2 aLeft,RMessagePtr2 aRight);

class CSession2;

#define __IPC_V2_PRESENT__

/**
@publishedAll
@released

An object that encapsulates the details of a client request.
*/
class RMessage2 : public RMessagePtr2
	{
	friend class CServer2;
public:

    /**
    Defines internal message types.
    */
	enum TSessionMessages {
	                      /**
	                      A message type used internally that means connect.
	                      */
	                      EConnect=-1,
	                      
	                      /**
                          A message type used internally that means disconnect.
	                      */
	                      EDisConnect=-2
	                      };
public:
	inline RMessage2();
#ifndef __KERNEL_MODE__
	IMPORT_C explicit RMessage2(const RMessagePtr2& aPtr);
	void SetAuthorised() const; 
	void ClearAuthorised() const;
	TBool Authorised() const;
#endif
	inline TInt Function() const;
	inline TInt Int0() const;
	inline TInt Int1() const;
	inline TInt Int2() const;
	inline TInt Int3() const;
	inline const TAny* Ptr0() const;
	inline const TAny* Ptr1() const;
	inline const TAny* Ptr2() const;
	inline const TAny* Ptr3() const;
	inline CSession2* Session() const;
protected:
    
    /**
    The request type.
    */
	TInt iFunction;
	
	/**
	A copy of the message arguments.
	*/
	TInt iArgs[KMaxMessageArguments];
private:
	TInt iSpare1;
protected:
    /**
    @internalComponent
    */
	const TAny* iSessionPtr;
private:
	mutable TInt iFlags;// Currently only used for *Authorised above
	TInt iSpare3;		// Reserved for future use

	friend class RMessage;
	};




/**
@publishedAll
@released

Defines an 8-bit modifiable buffer descriptor to contain passwords when dealing
with password security support in a file server session.

The descriptor takes a maximum length of KMaxMediaPassword.

@see KMaxMediaPassword
*/
typedef TBuf8<KMaxMediaPassword> TMediaPassword;	// 128 bit



/**
@publishedPartner
@prototype
A configuration flag for the shared chunk buffer configuration class (used by the multimedia device drivers). This being
set signifies that a buffer offset list follows the buffer configuration class. This list holds the offset of each buffer.
*/
const TUint KScFlagBufOffsetListInUse=0x00000001;

/**
@publishedPartner
@prototype
A configuration flag for the shared chunk buffer configuration class (used by the multimedia device drivers). This being
set is a suggestion that the shared chunk should be configured leaving guard pages around each buffers.
*/
const TUint KScFlagUseGuardPages=0x00000002;

/**
@publishedPartner
@prototype
The shared chunk buffer configuration class (used by the multimedia device drivers). This is used to hold information
on the current buffer configuration within a shared chunk.
*/
class TSharedChunkBufConfigBase
	{
public:	
	inline TSharedChunkBufConfigBase();
public:
	/** The number of buffers. */
	TInt iNumBuffers;
	/** The size of each buffer in bytes. */
	TInt iBufferSizeInBytes;
	/** Reserved field. */
	TInt iReserved1;
	/** Shared chunk buffer flag settings. */
	TUint iFlags;
	};


/** Maximum size of capability set

@internalTechnology
*/
const TInt KCapabilitySetMaxSize = (((TInt)ECapability_HardLimit + 7)>>3);

/** Maximum size of any future extension to TSecurityPolicy

@internalTechnology
*/
const TInt KMaxSecurityPolicySize = KCapabilitySetMaxSize + 3*sizeof(TUint32);


/** Class representing an arbitrary set of capabilities.

This class can only contain capabilities supported by the current OS version.

@publishedAll
@released
*/
class TCapabilitySet
	{
public:
	inline TCapabilitySet();
	inline TCapabilitySet(TCapability aCapability);
	IMPORT_C TCapabilitySet(TCapability aCapability1, TCapability aCapability2);
	IMPORT_C void SetEmpty();
	inline void Set(TCapability aCapability);
	inline void Set(TCapability aCapability1, TCapability aCapability2);
	IMPORT_C void SetAllSupported();
	IMPORT_C void AddCapability(TCapability aCapability);
	IMPORT_C void RemoveCapability(TCapability aCapability);
	IMPORT_C void Union(const TCapabilitySet&  aCapabilities);
	IMPORT_C void Intersection(const TCapabilitySet& aCapabilities);
	IMPORT_C void Remove(const TCapabilitySet& aCapabilities);
	IMPORT_C TBool HasCapability(TCapability aCapability) const;
	IMPORT_C TBool HasCapabilities(const TCapabilitySet& aCapabilities) const;

	/**
	Make this set consist of the capabilities which are disabled on this platform.
	@internalTechnology
	*/
	IMPORT_C void SetDisabled();
	/**
	@internalComponent
	*/
	TBool NotEmpty() const;

private:
	TUint32 iCaps[KCapabilitySetMaxSize / sizeof(TUint32)];
	};

#ifndef __SECURITY_INFO_DEFINED__
#define __SECURITY_INFO_DEFINED__
/**
@internalTechnology
 */
struct SCapabilitySet
	{
	enum {ENCapW=2};

	inline void AddCapability(TCapability aCap1) {((TCapabilitySet*)this)->AddCapability(aCap1);}
	inline void Remove(const SCapabilitySet& aCaps) {((TCapabilitySet*)this)->Remove(*((TCapabilitySet*)&aCaps));}
	inline TBool NotEmpty() const {return ((TCapabilitySet*)this)->NotEmpty();}

	inline const TUint32& operator[] (TInt aIndex) const { return iCaps[aIndex]; }
	inline TUint32& operator[] (TInt aIndex) { return iCaps[aIndex]; }

	TUint32 iCaps[ENCapW];
	};

/**
@internalTechnology
 */
struct SSecurityInfo
	{
	TUint32	iSecureId;
	TUint32	iVendorId;
	SCapabilitySet iCaps;	// Capabilities re. platform security
	};

#endif

/** Define this macro to reference the set of all capabilities.
	@internalTechnology
*/
#ifdef __REFERENCE_ALL_SUPPORTED_CAPABILITIES__

extern const SCapabilitySet AllSupportedCapabilities;

#endif	//__REFERENCE_ALL_SUPPORTED_CAPABILITIES__

/** Define this macro to include the set of all capabilities.
	@internalTechnology
*/
#ifdef __INCLUDE_ALL_SUPPORTED_CAPABILITIES__

/** The set of all capabilities.
	@internalTechnology
*/
const SCapabilitySet AllSupportedCapabilities = {
		{
		ECapability_Limit<32  ? (TUint32)((1u<<(ECapability_Limit&31))-1u) : 0xffffffffu
		,
		ECapability_Limit>=32 ? (TUint32)((1u<<(ECapability_Limit&31))-1u) : 0u
		}
	};

#endif	// __INCLUDE_ALL_SUPPORTED_CAPABILITIES__

#ifndef __KERNEL_MODE__
class RProcess;
class RThread;
class RMessagePtr2;
class RSessionBase;
#else
class DProcess;
class DThread;
#endif

/** Class representing all security attributes of a process or DLL.
	These comprise a set of capabilities, a Secure ID and a Vendor ID.

@publishedAll
@released
*/
class TSecurityInfo
	{
public:
	inline TSecurityInfo();
#ifdef __KERNEL_MODE__
	IMPORT_C TSecurityInfo(DProcess* aProcess);
	IMPORT_C TSecurityInfo(DThread* aThread);
#else
	IMPORT_C TSecurityInfo(RProcess aProcess);
	IMPORT_C TSecurityInfo(RThread aThread);
	IMPORT_C TSecurityInfo(RMessagePtr2 aMesPtr);
	inline void Set(RProcess aProcess);
	inline void Set(RThread aThread);
	inline void Set(RMessagePtr2 aMsgPtr);
	TInt Set(RSessionBase aSession); /**< @internalComponent */
	inline void SetToCurrentInfo();
	IMPORT_C void SetToCreatorInfo();
#endif //__KERNEL_MODE__
public:
	TSecureId		iSecureId;	/**< Secure ID */
	TVendorId		iVendorId;	/**< Vendor ID */
	TCapabilitySet	iCaps;		/**< Capability Set */
	};


/** Class representing a generic security policy

This class can specify a security policy consisting of either:

-#	A check for between 0 and 7 capabilities
-#	A check for a given Secure ID along with 0-3 capabilities
-#	A check for a given Vendor ID along with 0-3 capabilities

If multiple capabilities are specified, all of them must be present for the
security check to succeed ('AND' relation).

The envisaged use case for this class is to specify access rights to an object
managed either by the kernel or by a server but in principle owned by a client
and usable in a limited way by other clients. For example
- Publish and Subscribe properties
- DBMS databases

In these cases the owning client would pass one (or more) of these objects to
the server to specify which security checks should be done on other clients
before allowing access to the object.

To pass a TSecurityPolicy object via IPC, a client should obtain a descriptor
for the object using Package() and send this. When a server receives this descriptor
it should read the descriptor contents into a TSecurityPolicyBuf and then
Set() should be used to create a policy object from this.

Because this class has non-default constructors, compilers will not initialise
this object at compile time, instead code will be generated to construct the object
at run-time. This is wasteful - and Symbian OS DLLs are not permitted to have
such uninitialised data. To overcome these problems a set of macros are provided to
construct a const object which behaves like a TSecurityPolicy. These are:

_LIT_SECURITY_POLICY_C1 through _LIT_SECURITY_POLICY_C7,
_LIT_SECURITY_POLICY_S0 through _LIT_SECURITY_POLICY_S3 and
_LIT_SECURITY_POLICY_V0 through _LIT_SECURITY_POLICY_V3.

Also, the macros _LIT_SECURITY_POLICY_PASS and _LIT_SECURITY_POLICY_FAIL are provided
in order to allow easy construction of a const object which can be used as a
TSecuityPolicy which always passes or always fails, respectively.

If a security policy object is needed to be embedded in another class then the
TStaticSecurityPolicy structure can be used. This behaves in the same way as a
TSecurityPolicy object but may be initialised at compile time.

@see TStaticSecurityPolicy
@see TSecurityPolicyBuf
@see _LIT_SECURITY_POLICY_PASS
@see _LIT_SECURITY_POLICY_FAIL
@see _LIT_SECURITY_POLICY_C1
@see _LIT_SECURITY_POLICY_C2 
@see _LIT_SECURITY_POLICY_C3 
@see _LIT_SECURITY_POLICY_C4 
@see _LIT_SECURITY_POLICY_C5 
@see _LIT_SECURITY_POLICY_C6 
@see _LIT_SECURITY_POLICY_C7 
@see _LIT_SECURITY_POLICY_S0 
@see _LIT_SECURITY_POLICY_S1 
@see _LIT_SECURITY_POLICY_S2 
@see _LIT_SECURITY_POLICY_S3 
@see _LIT_SECURITY_POLICY_V0 
@see _LIT_SECURITY_POLICY_V1 
@see _LIT_SECURITY_POLICY_V2 
@see _LIT_SECURITY_POLICY_V3 

@publishedAll
@released
*/
class TSecurityPolicy
	{
public:
	enum TSecPolicyType 
		{
		EAlwaysFail=0,
		EAlwaysPass=1,
		};
		
public:
	inline TSecurityPolicy();
	IMPORT_C TSecurityPolicy(TSecPolicyType aType);
	IMPORT_C TSecurityPolicy(TCapability aCap1, TCapability aCap2 = ECapability_None, TCapability aCap3 = ECapability_None);
	IMPORT_C TSecurityPolicy(TCapability aCap1, TCapability aCap2, TCapability aCap3, TCapability aCap4, TCapability aCap5 = ECapability_None, TCapability aCap6 = ECapability_None, TCapability aCap7 = ECapability_None);
	IMPORT_C TSecurityPolicy(TSecureId aSecureId, TCapability aCap1 = ECapability_None, TCapability aCap2 = ECapability_None, TCapability aCap3 = ECapability_None);
	IMPORT_C TSecurityPolicy(TVendorId aVendorId, TCapability aCap1 = ECapability_None, TCapability aCap2 = ECapability_None, TCapability aCap3 = ECapability_None);
	IMPORT_C TInt Set(const TDesC8& aDes);
	IMPORT_C TPtrC8 Package() const;

#ifdef __KERNEL_MODE__

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool CheckPolicy(DProcess* aProcess, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(DThread* aThread, const char* aDiagnostic=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool CheckPolicy(DProcess* aProcess, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(DThread* aThread, OnlyCreateWithNull aDiagnostic=NULL) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

#else // !__KERNEL_MODE__

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool CheckPolicy(RProcess aProcess, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RThread aThread, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic=0) const;
	inline TBool CheckPolicyCreator(const char* aDiagnostic=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool CheckPolicy(RProcess aProcess, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RThread aThread, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicyCreator(OnlyCreateWithNull aDiagnostic=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline TBool CheckPolicy(RProcess aProcess, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RThread aThread, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicyCreator(OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	TInt CheckPolicy(RSessionBase aSession) const; /**< @internalComponent */

#endif //__KERNEL_MODE__

	TBool Validate() const;

private:
#ifdef __KERNEL_MODE__
	IMPORT_C TBool DoCheckPolicy(DProcess* aProcess, const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicy(DProcess* aProcess) const;
	IMPORT_C TBool DoCheckPolicy(DThread* aThread, const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicy(DThread* aThread) const;
#else // !__KERNEL_MODE__
	IMPORT_C TBool DoCheckPolicy(RProcess aProcess, const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicy(RProcess aProcess) const;
	IMPORT_C TBool DoCheckPolicy(RThread aThread, const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicy(RThread aThread) const;
	IMPORT_C TBool DoCheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicy(RMessagePtr2 aMsgPtr) const;
	IMPORT_C TBool DoCheckPolicyCreator(const char* aDiagnostic) const;
	IMPORT_C TBool DoCheckPolicyCreator() const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	TBool DoCheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic) const;
#endif //__REMOVE_PLATSEC_DIAGNOSTICS__
	TBool DoCheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing) const;
#endif //__KERNEL_MODE__

public:
	/** Constants to specify the type of TSecurityPolicy objects.
	*/
	enum TType
		{
		ETypeFail=0,	/**< Always fail*/
		ETypePass=1,	/**< Always pass*/
		ETypeC3=2,		/**< Up to 3 capabilities*/
		ETypeC7=3,		/**< Up to 7 capabilities*/
		ETypeS3=4,		/**< Secure ID and up to 3 capabilities*/
		ETypeV3=5,		/**< Vendor ID and up to 3 capabilities*/

		/** The number of possible TSecurityPolicy types
		This is intended for internal Symbian use only.
		@internalTechnology
		*/
		ETypeLimit

		// other values may be added to indicate expanded policy objects (future extensions)
		};
protected:
	TBool CheckPolicy(const SSecurityInfo& aSecInfo, SSecurityInfo& aMissing) const;
private:
	void ConstructAndCheck3(TCapability aCap1, TCapability aCap2, TCapability aCap3);
private:
	TUint8 iType;
	TUint8 iCaps[3];				// missing capabilities are set to 0xff
	union
		{
		TUint32 iSecureId;
		TUint32 iVendorId;
		TUint8 iExtraCaps[4];		// missing capabilities are set to 0xff
		};
	friend class TCompiledSecurityPolicy;
	};

/** Provides a TPkcgBuf wrapper for a descriptorised TSecurityPolicy.  This a
suitable container for passing a security policy across IPC.
@publishedAll
@released
*/
typedef TPckgBuf<TSecurityPolicy> TSecurityPolicyBuf;


/** Structure for compile-time initialisation of a security policy.

This structure behaves in the same way as a TSecurityPolicy object but has
the advantage that it may be initialised at compile time. E.g.
the following line defines a security policy 'KSecurityPolictReadUserData'
which checks ReadUserData capability.

@code
_LIT_SECURITY_POLICY_C1(KSecurityPolictReadUserData,ECapabilityReadUserData)
@endcode

Or, an array of security policies may be created like this:
@code
static const TStaticSecurityPolicy MyPolicies[] = 
	{
	_INIT_SECURITY_POLICY_C1(ECapabilityReadUserData),
	_INIT_SECURITY_POLICY_PASS(),
	_INIT_SECURITY_POLICY_S0(0x1234567)
	}
@endcode

This class should not be initialised directly, instead one of the following
macros should be used:

-	_INIT_SECURITY_POLICY_PASS
-	_INIT_SECURITY_POLICY_FAIL
-	_INIT_SECURITY_POLICY_C1
-	_INIT_SECURITY_POLICY_C2
-	_INIT_SECURITY_POLICY_C3
-	_INIT_SECURITY_POLICY_C4
-	_INIT_SECURITY_POLICY_C5
-	_INIT_SECURITY_POLICY_C6
-	_INIT_SECURITY_POLICY_C7
-	_INIT_SECURITY_POLICY_S0
-	_INIT_SECURITY_POLICY_S1
-	_INIT_SECURITY_POLICY_S2
-	_INIT_SECURITY_POLICY_S3
-	_INIT_SECURITY_POLICY_V0
-	_INIT_SECURITY_POLICY_V1
-	_INIT_SECURITY_POLICY_V2
-	_INIT_SECURITY_POLICY_V3
-	_LIT_SECURITY_POLICY_PASS
-	_LIT_SECURITY_POLICY_FAIL
-	_LIT_SECURITY_POLICY_C1
-	_LIT_SECURITY_POLICY_C2
-	_LIT_SECURITY_POLICY_C3
-	_LIT_SECURITY_POLICY_C4
-	_LIT_SECURITY_POLICY_C5
-	_LIT_SECURITY_POLICY_C6
-	_LIT_SECURITY_POLICY_C7
-	_LIT_SECURITY_POLICY_S0
-	_LIT_SECURITY_POLICY_S1
-	_LIT_SECURITY_POLICY_S2
-	_LIT_SECURITY_POLICY_S3
-	_LIT_SECURITY_POLICY_V0
-	_LIT_SECURITY_POLICY_V1
-	_LIT_SECURITY_POLICY_V2
-	_LIT_SECURITY_POLICY_V3

@see TSecurityPolicy
@publishedAll
@released
*/
struct TStaticSecurityPolicy
	{
	inline const TSecurityPolicy* operator&() const;
	inline operator const TSecurityPolicy&() const;
	inline const TSecurityPolicy& operator()() const;

#ifndef __KERNEL_MODE__
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool CheckPolicy(RProcess aProcess, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RThread aThread, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic=0) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic=0) const;
	inline TBool CheckPolicyCreator(const char* aDiagnostic=0) const;
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool CheckPolicy(RProcess aProcess, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RThread aThread, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull aDiagnostic=NULL) const;
	inline TBool CheckPolicyCreator(OnlyCreateWithNull aDiagnostic=NULL) const;
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline TBool CheckPolicy(RProcess aProcess, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RThread aThread, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
	inline TBool CheckPolicyCreator(OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#endif // !__KERNEL_MODE__

	TUint32 iA;	/**< @internalComponent */
	TUint32 iB;	/**< @internalComponent */
	};

	
/**
A dummy enum for use by the CAPABILITY_AS_TUINT8 macro
@internalComponent
*/
enum __invalid_capability_value {};

/**
A macro to cast a TCapability to a TUint8.

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param cap The capability value
@internalComponent
*/
#define CAPABILITY_AS_TUINT8(cap)											\
	((TUint8)(int)(															\
		(cap)==ECapability_None												\
		? (__invalid_capability_value(*)[1])(ECapability_None)								\
		: (__invalid_capability_value(*)[((TUint)(cap+1)<=(TUint)ECapability_Limit)?1:2])(cap)	\
	))


/**
A macro to construct a TUint32 from four TUint8s.  The TUint32 is in BigEndian
ordering useful for class layout rather than number generation.

@param i1 The first TUint8
@param i2 The second TUint8
@param i3 The third TUint8
@param i4 The fourth TUint8
@internalComponent
*/
#define FOUR_TUINT8(i1,i2,i3,i4) \
	(TUint32)(				\
		(TUint8)i1 		 | 	\
		(TUint8)i2 << 8  | 	\
		(TUint8)i3 << 16 | 	\
		(TUint8)i4 << 24	\
	)


/** Macro for compile-time initialisation of a security policy object that
always fails.  That is, checks against this policy will always fail,
irrespective of the security attributes of the item being checked.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().
@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_FAIL \
	{ 																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypeFail,									\
		(TUint8)0xff,														\
		(TUint8)0xff,														\
		(TUint8)0xff														\
	),																		\
	(TUint32)0xffffffff														\
	}


/** Macro for compile-time definition of a security policy object that always
fails.  That is, checks against this policy will always fail, irrespective of
the security attributes of the item being checked.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().
@param	n	Name to use for policy object
@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_FAIL(n) const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_FAIL


/** Macro for compile-time initialisation of a security policy object that 
always passes.  That is, checks against this policy will always pass,
irrespective of the security attributes of the item being checked.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().
@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_PASS \
	{ 																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypePass,									\
		(TUint8)0xff,														\
		(TUint8)0xff,														\
		(TUint8)0xff														\
	),																		\
	(TUint32)0xffffffff														\
	}


/** Macro for compile-time definition of a security policy object that always
passes.  That is, checks against this policy will always pass, irrespective of
the security attributes of the item being checked.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().
@param	n	Name to use for policy object
@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_PASS(n) const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_PASS


/** Macro for compile-time initialisation of a security policy object
The policy will check for seven capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)
@param	c6	The sixth capability to check (enumerator of TCapability)
@param	c7	The seventh capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C7(c1,c2,c3,c4,c5,c6,c7) \
	{ 																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypeC7,									\
		CAPABILITY_AS_TUINT8(c1),											\
		CAPABILITY_AS_TUINT8(c2),											\
		CAPABILITY_AS_TUINT8(c3)											\
	),																		\
	FOUR_TUINT8(															\
		CAPABILITY_AS_TUINT8(c4),											\
		CAPABILITY_AS_TUINT8(c5),											\
		CAPABILITY_AS_TUINT8(c6),											\
		CAPABILITY_AS_TUINT8(c7)											\
	)																		\
	}


/** Macro for compile-time definition of a security policy object
The policy will check for seven capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)
@param	c6	The sixth capability to check (enumerator of TCapability)
@param	c7	The seventh capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C7(n,c1,c2,c3,c4,c5,c6,c7)						\
	const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_C7(c1,c2,c3,c4,c5,c6,c7)


/** Macro for compile-time initialisation of a security policy object
The policy will check for six capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)
@param	c6	The sixth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C6(c1,c2,c3,c4,c5,c6)  \
	_INIT_SECURITY_POLICY_C7(c1,c2,c3,c4,c5,c6,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for six capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)
@param	c6	The sixth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C6(n,c1,c2,c3,c4,c5,c6)  \
	_LIT_SECURITY_POLICY_C7(n,c1,c2,c3,c4,c5,c6,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for five capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C5(c1,c2,c3,c4,c5)  \
	_INIT_SECURITY_POLICY_C7(c1,c2,c3,c4,c5,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for five capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)
@param	c5	The fifth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C5(n,c1,c2,c3,c4,c5)  \
	_LIT_SECURITY_POLICY_C7(n,c1,c2,c3,c4,c5,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for four capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C4(c1,c2,c3,c4)  \
	_INIT_SECURITY_POLICY_C7(c1,c2,c3,c4,ECapability_None,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for four capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)
@param	c4	The fourth capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C4(n,c1,c2,c3,c4)  \
	_LIT_SECURITY_POLICY_C7(n,c1,c2,c3,c4,ECapability_None,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C3(c1,c2,c3)									\
	{ 																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypeC3,									\
		CAPABILITY_AS_TUINT8(c1),											\
		CAPABILITY_AS_TUINT8(c2),											\
		CAPABILITY_AS_TUINT8(c3)											\
	),																		\
	(TUint32)0xffffffff														\
	}


/** Macro for compile-time definition of a security policy object
The policy will check for three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C3(n,c1,c2,c3)									\
	const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_C3(c1,c2,c3)


/** Macro for compile-time initialisation of a security policy object
The policy will check for two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C2(c1,c2)  \
	_INIT_SECURITY_POLICY_C3(c1,c2,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C2(n,c1,c2)  \
	_LIT_SECURITY_POLICY_C3(n,c1,c2,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	c1	The first capability to check (enumerator of TCapability)


@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_C1(c1)  \
	_INIT_SECURITY_POLICY_C3(c1,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning will be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	c1	The first capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_C1(n,c1)  \
	_LIT_SECURITY_POLICY_C3(n,c1,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a secure ID and three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_S3(sid,c1,c2,c3)								\
	{																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypeS3,									\
		CAPABILITY_AS_TUINT8(c1),											\
		CAPABILITY_AS_TUINT8(c2),											\
		CAPABILITY_AS_TUINT8(c3)											\
	),																		\
	(TUint32)(sid)															\
	}


/** Macro for compile-time definition of a security policy object
The policy will check for a secure ID and three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_S3(n,sid,c1,c2,c3)								\
	const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_S3(sid,c1,c2,c3)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a secure ID and two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_S2(sid,c1,c2)  \
	_INIT_SECURITY_POLICY_S3(sid,c1,c2,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a secure ID and two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_S2(n,sid,c1,c2)  \
	_LIT_SECURITY_POLICY_S3(n,sid,c1,c2,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a secure ID and one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_S1(sid,c1)  \
	_INIT_SECURITY_POLICY_S3(sid,c1,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a secure ID and one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	sid	The SID value to check for
@param	c1	The first capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_S1(n,sid,c1)  \
	_LIT_SECURITY_POLICY_S3(n,sid,c1,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a secure ID.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

@param	sid	The SID value to check for

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_S0(sid)  \
	_INIT_SECURITY_POLICY_S3(sid,ECapability_None,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a secure ID.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

@param	n	Name to use for policy object
@param	sid	The SID value to check for

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_S0(n,sid)  \
	_LIT_SECURITY_POLICY_S3(n,sid,ECapability_None,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a vendor ID and three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_V3(vid,c1,c2,c3)								\
	{																		\
	FOUR_TUINT8(															\
		(TUint8)TSecurityPolicy::ETypeV3,									\
		CAPABILITY_AS_TUINT8(c1),											\
		CAPABILITY_AS_TUINT8(c2),											\
		CAPABILITY_AS_TUINT8(c3)											\
	),																		\
	(TUint32)(vid)															\
	}


/** Macro for compile-time definition of a security policy object
The policy will check for a vendor ID and three capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)
@param	c3	The third capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_V3(n,vid,c1,c2,c3)								\
	const TStaticSecurityPolicy n = _INIT_SECURITY_POLICY_V3(vid,c1,c2,c3)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a vendor ID and two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_V2(vid,c1,c2)  \
	_INIT_SECURITY_POLICY_V3(vid,c1,c2,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a vendor ID and two capabilities.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)
@param	c2	The second capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_V2(n,vid,c1,c2)  \
	_LIT_SECURITY_POLICY_V3(n,vid,c1,c2,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a vendor ID and one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_V1(vid,c1)  \
	_INIT_SECURITY_POLICY_V3(vid,c1,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a vendor ID and one capability.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

If an invlid capability value is specified then, dependant on the compiler,
a compile time error or warning be produced which includes the label
"__invalid_capability_value"

@param	n	Name to use for policy object
@param	vid	The VID value to check for
@param	c1	The first capability to check (enumerator of TCapability)

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_V1(n,vid,c1)  \
	_LIT_SECURITY_POLICY_V3(n,vid,c1,ECapability_None,ECapability_None)


/** Macro for compile-time initialisation of a security policy object
The policy will check for a vendor ID.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

@param	vid	The VID value to check for

@publishedAll
@released
*/
#define _INIT_SECURITY_POLICY_V0(vid)  \
	_INIT_SECURITY_POLICY_V3(vid,ECapability_None,ECapability_None,ECapability_None)


/** Macro for compile-time definition of a security policy object
The policy will check for a vendor ID.

The object declared has an implicit conversion to const TSecurityPolicy&.
Taking the address of the object will return a const TSecurityPolicy*.
Explicit conversion to const TSecurityPolicy& may be effected by using the
function call operator n().

@param	n	Name to use for policy object
@param	vid	The VID value to check for

@publishedAll
@released
*/
#define	_LIT_SECURITY_POLICY_V0(n,vid)  \
	_LIT_SECURITY_POLICY_V3(n,vid,ECapability_None,ECapability_None,ECapability_None)



#ifdef __KERNEL_MODE__
class DThread;
class RMessageK;
#endif
class TPlatSecDiagnostic;

/**
Class containing Platform Security related methods
@internalTechnology
*/
class PlatSec
	{
#ifndef __KERNEL_MODE__
public:
	/**
	Tests whether a given Platform Security capability is enforced by the system.

	Capabilities may not be enforced for several reasons:
	-#	The capability has been explicitly disabled on this system
		by use of the PlatSecDisabledCaps configuration parameter
	-#	Platform Security checks have been globally disabled
		by use of the EPlatSecEnforcement configuration parameter	     
	-#	The capability value is unknown. I.e. Is not part of the set of supported
		capabilities. See TCapabilitySet::SetAllSupported().

	@param aCapability The capability to test
	@return A non-zero value if the capability is enforced, zero if it is not.

	@publishedAll
	@released
	*/
	IMPORT_C static TBool IsCapabilityEnforced(TCapability aCapability);

	/**
	An enumeration used with PlatSecSetting()
	@see PlatSecSetting()
	@publishedAll
	@test
	*/
	enum TConfigSetting
		{
		EPlatSecEnforcement, /**< Used to request the value of the PlatSecEnforcement setting */
		EPlatSecDiagnotics,  /**< Used to request the value of the PlatSecDiagnotics setting */
		EPlatSecProcessIsolation,  /**< Used to request the value of the PlatSecProcessIsolation setting */
		EPlatSecEnforceSysBin,  /**< Used to request the value of the PlatSecEnforceSysBin setting */
		EPlatSecLocked,  /**< Used to request the value of the PlatSecLocked setting */
		};

	/**
	A test function to return the state of a given Platform Security configuration setting.
	@param aSetting An enumerated value representing the required setting
	@return A value representing the setting. 0 represents 'OFF', 1 represents 'ON'
			Other values may be returned for some settings, these exceptions are documented
			in the description for individual enumerations of TConfigSetting.
	@see TConfigSetting
	@publishedAll
	@test
	*/
	IMPORT_C static TInt ConfigSetting(TConfigSetting aSetting);

#endif // Not __KERNEL_MODE__

	//
	// All methods below here are internalTechnology
	//

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
public:
	/** @internalTechnology */
	static inline TInt LoaderCapabilityViolation(const TDesC8& aImporterName, const TDesC8& aFileName, const SCapabilitySet& aMissingCaps);
#ifdef __KERNEL_MODE__
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(const DProcess* aViolatingProcess, TCapability aCapability, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(const DThread* aViolatingThread, TCapability aCapability, const char* aContextText);
	/** @internalTechnology */
	static inline TInt SecureIdCheckFail(const DProcess* aViolatingProcess, TSecureId aSid, const char* aContextText);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(const DProcess* aProcess, const SSecurityInfo& aMissing, const char* aContextText);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(const DThread* aProcess, const SSecurityInfo& aMissing, const char* aContextText);
	/** @internalTechnology */
	static inline TInt ProcessIsolationFail(const char* aContextText);
	/** @internalTechnology */
	static inline TInt ProcessIsolationIPCFail(RMessageK* aMessage, const char* aContextText);
#else // !__KERNEL_MODE__
	/** @internalTechnology */
	static inline TInt LoaderCapabilityViolation(RProcess aLoadingProcess, const TDesC8& aFileName, const SCapabilitySet& aMissingCaps);
	/** @internalTechnology */
	static inline TInt CreatorCapabilityCheckFail(TCapability aCapability, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CreatorCapabilityCheckFail(const TCapabilitySet& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(TInt aHandle, TCapability aCapability, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(TInt aHandle, const TCapabilitySet& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(TInt aHandle, const SSecurityInfo& aMissing, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(RMessagePtr2 aMessage, TCapability aCapability, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(RMessagePtr2 aMessage, const TCapabilitySet& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(RMessagePtr2 aMessage, const SSecurityInfo& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(RSessionBase aSession, const SSecurityInfo& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CreatorPolicyCheckFail(const SSecurityInfo& aMissingCaps, const char* aContextText);
	/** @internalTechnology */
	static inline TInt CreatorCapabilityCheckFail(TCapability aCapability);
	/** @internalTechnology */
	static inline TInt CreatorCapabilityCheckFail(const TCapabilitySet& aMissingCaps);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(TInt aHandle, TCapability aCapability);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(TInt aHandle, const TCapabilitySet& aMissingCaps);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(TInt aHandle, const SSecurityInfo& aMissing);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(RMessagePtr2 aMessage, TCapability aCapability);
	/** @internalTechnology */
	static inline TInt CapabilityCheckFail(RMessagePtr2 aMessage, const TCapabilitySet& aMissingCaps);
	/** @internalTechnology */
	static inline TInt PolicyCheckFail(RMessagePtr2 aMessage, const SSecurityInfo& aMissingCaps);
	/** @internalTechnology */
	static inline TInt CreatorPolicyCheckFail(const SSecurityInfo& aMissingCaps);
#endif //__KERNEL_MODE__

private:
	UIMPORT_C static TInt EmitDiagnostic(TPlatSecDiagnostic& aDiagnostic, const char* aContextText);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
#ifndef __KERNEL_MODE__
private:
	IMPORT_C static TInt EmitDiagnostic(TPlatSecDiagnostic& aDiagnostic, const char* aContextText);
#endif // !__KERNEL_MODE__
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

public:
	/** @internalTechnology */
	UIMPORT_C static TInt EmitDiagnostic();
	};


#define KMaxSerialNumLength 64
typedef TBuf8<KMaxSerialNumLength> TMediaSerialNumber;


/**
@publishedAll
@released

Contains information about the code and data sections belonging to a process.

@see RProcess::GetMemoryInfo
*/
class TProcessMemoryInfo
	{
public:
    /**
    The code base address (.text).
    */
	TUint32 iCodeBase;

	
    /**
    The size of the code section (.text).
    */
	TUint32 iCodeSize;
	
	
    /**
    The base address of the constant data section (.radata).
    */
	TUint32 iConstDataBase;
	
	
    /**
    The size of the constant data section (.radata).
    */

	TUint32 iConstDataSize;
	
	
    /**
    The base address of the initialised data section (.data).
    */
	TUint32 iInitialisedDataBase;
	
	
    /**
    The size of the initialised data section (.data).
    */
	TUint32 iInitialisedDataSize;

	
    /**
    The base address of the uninitialised data section (.bss).
    */
	TUint32 iUninitialisedDataBase;

	
    /**
    The size of the uninitialised data section (.bss).
    */
	TUint32 iUninitialisedDataSize;
	};




/**
@publishedAll
@released

Defines a more useful synonym for TProcessMemoryInfo.
*/
typedef TProcessMemoryInfo TModuleMemoryInfo;	// more accurate name - remove old one later




#ifndef __KERNEL_MODE__
class CBase;
/**
@publishedAll
@released

Generic array.

This class defines a generic array which can be constructed by any of the
following templated concrete arrays:

1. CArrayFixFlat<class T>

2. CArrayFixSeg<class T>

3. CArrayVarFlat<class T>

4. CArrayVarSeg<class T>

5. CArrayPakFlat<class T>

6. RArray<class T>

7. RPointerArray<class T>

and also by the following template specialisation classes:

1. RArray<TInt>

2. RArray<TUint>

It allows a degree of polymorphism amongst the array classes. It permits the 
operator[] and the Count() member functions of an array to be invoked without 
knowing which array class has been used to construct that array.

TArray allows access to elements of an array but does not permit changes to 
those elements. 

Use the Array() member function of an array to construct and return
a TArray<class T> object for that array.

A TArray<class T> type object is not intended to be constructed explicitly 
by user code.

@see CArrayFixFlat
@see CArrayFixSeg
@see CArrayVarFlat
@see CArrayVarSeg
@see CArrayPakFlat
@see RArray
@see RPointerArray
@see RArray<TInt>
@see RArray<TUint>
*/
template <class T>
class TArray
	{
public:
	inline TArray(TInt (*aCount)(const CBase* aPtr),const TAny*(*anAt)(const CBase* aPtr,TInt anIndex),const CBase* aPtr);
	inline TInt Count() const;
	inline const T& operator[](TInt anIndex) const;
private:
	const CBase* iPtr;
	TInt (*iCount)(const CBase* aPtr);
	const TAny*(*iAt)(const CBase* aPtr,TInt anIndex);
	};
#endif




/**
@publishedAll
@released

Defines a function type used by a TIdentityRelation object. 

A function of this type implements an algorithm for determining whether
two objects match.

@see TIdentityRelation
*/
typedef TBool (*TGeneralIdentityRelation)(const TAny*, const TAny*);




/**
@publishedAll
@released

Defines a function type used by a TLinearOrder object

A function of this type implements an algorithm that determines
the order of two objects.

@see TLinearOrder
*/
typedef TInt (*TGeneralLinearOrder)(const TAny*, const TAny*);




/**
@publishedAll
@released

A templated class which packages a function that determines whether two
objects of a given class type match. During linear search operations the search
term is always passed as the first argument and the second argument is an
element of the array being searched.

A TIdentityRelation<T> object is constructed and passed as a parameter to 
member functions of the array classes RArray<T> and RPointerArray<T>.

@see RArray
@see RPointerArray
*/
template <class T>
class TIdentityRelation
	{
public:
	inline TIdentityRelation();
	inline TIdentityRelation( TBool (*anIdentity)(const T&, const T&) );
	inline operator TGeneralIdentityRelation() const;
private:
	inline static TBool EqualityOperatorCompare(const T& aLeft, const T& aRight);
private:
	TGeneralIdentityRelation iIdentity;
	};



/**
@publishedAll
@released

A set of common identity relations for frequently occurring types.

@see RArray
@see RPointerArray
@see RHashSet
@see RPtrHashSet
@see RHashMap
@see RPtrHashMap
*/
class DefaultIdentity
	{
public:
	IMPORT_C static TBool Integer(const TInt&, const TInt&);
	IMPORT_C static TBool Des8(const TDesC8&, const TDesC8&);
	IMPORT_C static TBool Des16(const TDesC16&, const TDesC16&);
	IMPORT_C static TBool IntegerPtr(TInt* const&, TInt* const&);
	IMPORT_C static TBool Des8Ptr(TDesC8* const&, TDesC8* const&);
	IMPORT_C static TBool Des16Ptr(TDesC16* const&, TDesC16* const&);
	};




/**
@publishedAll
@released

A templated class which packages a function that determines the order of two 
objects of a given class type. During binary search operations the search term
is always passed as the first argument and the second argument is an element
of the array being searched.

A TLinearOrder<T> object is constructed and passed as a parameter to member 
functions of the array classes RArray<T> and RPointerArray<T>.

@see RArray
@see RPointerArray
*/
template <class T>
class TLinearOrder
	{
public:
	inline TLinearOrder( TInt(*anOrder)(const T&, const T&) );
	inline operator TGeneralLinearOrder() const;
private:
	TGeneralLinearOrder iOrder;
	};


/*
@publishedAll
@released

A set of values that tell array search functions which array element is to be
returned when there are duplicate elements in the array.

These values are used by RArray, RPointerArray, RArray<TInt>,
and RArray<TUint> search functions. 

Examples of functions that take
these enum values are: RPointerArray::SpecificFindInOrderL(),
and RArray::SpecificFindInSignedKeyOrder().

@see RArray
@see RPointerArray
@see RArray<TInt>
@see RArray<TUint>
*/
enum TArrayFindMode
	{
	/**
	Indicates that any element in a block of duplicate elements can be
	returned by a search function.
	
	Note that using this mode, there can be no guarantee that the element
	returned by the search functions will be the same if the size of the array
	changes between successive calls to those functions.
	*/
	EArrayFindMode_Any = 0,
	
	/**
	Indicates that the first element in a block of duplicate elements
	is returned.
	*/
	EArrayFindMode_First = 1,

	/**
	Indicates that the first element after the last element in a block
	of duplicate elements is returned.
	*/
	EArrayFindMode_Last = 2,
    
    /**
    @internalTechnology
    */
	EArrayFindMode_Limit = 3
	};


/**
@internalComponent

Base class used in the derivation of RPointerArray, RArray<TInt>,
and RArray<TUint>. 

The base class is inherited privately.

The class is internal and is not intended for use.
*/
class RPointerArrayBase
	{
protected:
	IMPORT_C RPointerArrayBase();
	IMPORT_C RPointerArrayBase(TInt aGranularity);
	IMPORT_C RPointerArrayBase(TInt aMinGrowBy, TInt aFactor);
	IMPORT_C void Close();
	IMPORT_C TInt Count() const;
	inline void ZeroCount() {iCount=0;}
	inline TAny** Entries() {return iEntries;}
	IMPORT_C TAny*& At(TInt anIndex) const;
	IMPORT_C TInt Append(const TAny* anEntry);
	IMPORT_C TInt Insert(const TAny* anEntry, TInt aPos);
	IMPORT_C void Remove(TInt anIndex);
	IMPORT_C void Compress();
	IMPORT_C void Reset();
	IMPORT_C TInt Find(const TAny* anEntry) const;
	IMPORT_C TInt Find(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const;
	IMPORT_C TInt FindReverse(const TAny* aEntry) const;
	IMPORT_C TInt FindReverse(const TAny* aEntry, TGeneralIdentityRelation aIdentity) const;
	IMPORT_C TInt FindIsqSigned(TInt anEntry) const;
	IMPORT_C TInt FindIsqUnsigned(TUint anEntry) const;
	IMPORT_C TInt FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder) const;
	IMPORT_C TInt FindIsqSigned(TInt anEntry, TInt aMode) const;
	IMPORT_C TInt FindIsqUnsigned(TUint anEntry, TInt aMode) const;
	IMPORT_C TInt FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TInt aMode) const;
	IMPORT_C TInt InsertIsqSigned(TInt anEntry, TBool aAllowRepeats);
	IMPORT_C TInt InsertIsqUnsigned(TUint anEntry, TBool aAllowRepeats);
	IMPORT_C TInt InsertIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TBool aAllowRepeats);
	IMPORT_C TInt BinarySearchSigned(TInt anEntry, TInt& anIndex) const;
	IMPORT_C TInt BinarySearchUnsigned(TUint anEntry, TInt& anIndex) const;
	IMPORT_C TInt BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder) const;
	IMPORT_C TInt BinarySearchSigned(TInt anEntry, TInt& anIndex, TInt aMode) const;
	IMPORT_C TInt BinarySearchUnsigned(TUint anEntry, TInt& anIndex, TInt aMode) const;
	IMPORT_C TInt BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder, TInt aMode) const;
#ifndef __KERNEL_MODE__
	IMPORT_C RPointerArrayBase(TAny** aEntries, TInt aCount);
	IMPORT_C void GranularCompress();
	IMPORT_C TInt DoReserve(TInt aCount);
	IMPORT_C void HeapSortSigned();
	IMPORT_C void HeapSortUnsigned();
	IMPORT_C void HeapSort(TGeneralLinearOrder anOrder);
	IMPORT_C static TInt GetCount(const CBase* aPtr);
	IMPORT_C static const TAny* GetElementPtr(const CBase* aPtr, TInt aIndex);
#endif
private:
	TInt Grow();
private:
	TInt iCount;
	TAny** iEntries;
	TInt iAllocated;
	TInt iGranularity;	// positive means linear, negative means exponential growth
	TInt iSpare1;
	TInt iSpare2;
	};




/**
@publishedAll
@released

A simple and efficient array of pointers to objects.

The elements of the array are pointers to instances of a class; this class
is specified as the template parameter T.

The class offers standard array behaviour which includes insertion, appending 
and sorting of pointers.

Derivation from RPointerArrayBase is private.
*/
template <class T>
class RPointerArray : private RPointerArrayBase
	{
public:
	inline RPointerArray();
	inline explicit RPointerArray(TInt aGranularity);
	inline RPointerArray(TInt aMinGrowBy, TInt aFactor);
	inline void Close();
	inline TInt Count() const;
	inline T* const& operator[](TInt anIndex) const;
	inline T*& operator[](TInt anIndex);
	inline TInt Append(const T* anEntry);
	inline TInt Insert(const T* anEntry, TInt aPos);
	inline void Remove(TInt anIndex);
	inline void Compress();
	inline void Reset();
	void ResetAndDestroy();
	inline TInt Find(const T* anEntry) const;
	inline TInt Find(const T* anEntry, TIdentityRelation<T> anIdentity) const;
	template <class K>
	inline TInt Find(const K& aKey, TBool (*apfnCompare)(const K* k, const T& t)) const
	/**
	Finds the first object pointer in the array which matches aKey using
	the comparison algorithm provided by apfnCompare.
	
	The find operation always starts at the low index end of the array. There 
	is no assumption about the order of objects in the array.

	@param aKey The key of type K to be compared with the elements of the array using apfnCompare.
	@param apfnCompare A function defining the identity relation between the
			object pointers in the array, and their keys of type K.  The
			function returns true if k and t match based on this relationship.
	
	@return The index of the first matching object pointer within the array.
			KErrNotFound, if no suitable object pointer can be found.
	*/
		{ return RPointerArrayBase::Find((T*)&aKey,*(TIdentityRelation<T>*)&apfnCompare); }		
	inline TInt FindReverse(const T* anEntry) const;
	inline TInt FindReverse(const T* anEntry, TIdentityRelation<T> anIdentity) const;
	template <class K>
	inline TInt FindReverse(const K& aKey, TInt (*apfnMatch)(const K* k, const T& t)) const
	/**
	Finds the first object pointer in the array which matches aKey using
	the comparison algorithm provided by apfnCompare.
	
	The find operation always starts at the high index end of the array. There 
	is no assumption about the order of objects in the array.

	@param aKey The key of type K to be compared with the elements of the array using apfnMatch.
	@param apfnMatch A function defining the identity relation between the
			object pointers in the array, and their keys of type K.  The
			function returns true if k and t match based on this relationship.
	
	@return The index of the first matching object pointer within the array.
			KErrNotFound, if no suitable object pointer can be found.
	*/

		{ return RPointerArrayBase::FindReverse((T*)&aKey,*(TIdentityRelation<T>*)&apfnMatch); } 				
	inline TInt FindInAddressOrder(const T* anEntry) const;
	inline TInt FindInOrder(const T* anEntry, TLinearOrder<T> anOrder) const;
	inline TInt FindInAddressOrder(const T* anEntry, TInt& anIndex) const;
	inline TInt FindInOrder(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const;
	template <class K>
	inline TInt FindInOrder(const K& aKey, TInt (*apfnCompare)(const K* k, const T& t)) const
	/**
	Finds the object pointer in the array whose object matches the specified
	key, (Using the relationship defined within apfnCompare) using a binary search
	technique and an ordering algorithm.

	The function assumes that existing object pointers in the array are ordered 
	so that the objects themselves are in object order as determined by an algorithm 
	supplied by the caller and packaged as a TLinearOrder<T>.

	@param aKey The key of type K to be compared with the elements of the array using apfnCompare.
	@param apfnCompare A function which defines the order that the array was sorted,
		 where in it aKey (via the defined relationship) should fit, and if the key is present. 
	
	@return The index of the matching object pointer within the array.
			KErrNotFound, if no suitable object pointer can be found.
	*/	
		{ return RPointerArrayBase::FindIsq((T*)&aKey,*(TLinearOrder<T>*)&apfnCompare); }
	inline TInt SpecificFindInAddressOrder(const T* anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrder(const T* anEntry, TLinearOrder<T> anOrder, TInt aMode) const;
	inline TInt SpecificFindInAddressOrder(const T* anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt SpecificFindInOrder(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const;
	inline TInt InsertInAddressOrder(const T* anEntry);
	inline TInt InsertInOrder(const T* anEntry, TLinearOrder<T> anOrder);
	inline TInt InsertInAddressOrderAllowRepeats(const T* anEntry);
	inline TInt InsertInOrderAllowRepeats(const T* anEntry, TLinearOrder<T> anOrder);
#ifndef __KERNEL_MODE__
	inline void AppendL(const T* anEntry);
	inline void InsertL(const T* anEntry, TInt aPos);
	inline TInt FindL(const T* anEntry) const;
	inline TInt FindL(const T* anEntry, TIdentityRelation<T> anIdentity) const;
	inline TInt FindReverseL(const T* anEntry) const;
	inline TInt FindReverseL(const T* anEntry, TIdentityRelation<T> anIdentity) const;
	inline TInt FindInAddressOrderL(const T* anEntry) const;
	inline TInt FindInOrderL(const T* anEntry, TLinearOrder<T> anOrder) const;
	inline void FindInAddressOrderL(const T* anEntry, TInt& anIndex) const;
	inline void FindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const;
	inline TInt SpecificFindInAddressOrderL(const T* anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrderL(const T* anEntry, TLinearOrder<T> anOrder, TInt aMode) const;
	inline void SpecificFindInAddressOrderL(const T* anEntry, TInt& anIndex, TInt aMode) const;
	inline void SpecificFindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const;
	inline void InsertInAddressOrderL(const T* anEntry);
	inline void InsertInOrderL(const T* anEntry, TLinearOrder<T> anOrder);
	inline void InsertInAddressOrderAllowRepeatsL(const T* anEntry);
	inline void InsertInOrderAllowRepeatsL(const T* anEntry, TLinearOrder<T> anOrder);

	inline RPointerArray(T** aEntries, TInt aCount);
	inline void GranularCompress();
	inline TInt Reserve(TInt aCount);
	inline void ReserveL(TInt aCount);
	inline void SortIntoAddressOrder();
	inline void Sort(TLinearOrder<T> anOrder);
	inline TArray<T*> Array() const;
#endif
	};



/**
@publishedAll
@released

Array of raw pointers.

The array is a simple and efficient specialized array of TAny pointers offering
standard array behaviour.

The derivation from RPointerArrayBase is private.
*/
TEMPLATE_SPECIALIZATION class RPointerArray<TAny> : private RPointerArrayBase
	{
public:
	inline RPointerArray();
	inline explicit RPointerArray(TInt aGranularity);
	inline RPointerArray(TInt aMinGrowBy, TInt aFactor);
	inline void Close();
	inline TInt Count() const;
	inline TAny* const& operator[](TInt anIndex) const;
	inline TAny*& operator[](TInt anIndex);
	inline TInt Append(const TAny* anEntry);
	inline TInt Insert(const TAny* anEntry, TInt aPos);
	inline void Remove(TInt anIndex);
	inline void Compress();
	inline void Reset();
	inline TInt Find(const TAny* anEntry) const;
	inline TInt FindReverse(const TAny* anEntry) const;
	inline TInt FindInAddressOrder(const TAny* anEntry) const;
	inline TInt FindInAddressOrder(const TAny* anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInAddressOrder(const TAny* anEntry, TInt aMode) const;
	inline TInt SpecificFindInAddressOrder(const TAny* anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt InsertInAddressOrder(const TAny* anEntry);
	inline TInt InsertInAddressOrderAllowRepeats(const TAny* anEntry);
#ifndef __KERNEL_MODE__
	inline void AppendL(const TAny* anEntry);
	inline void InsertL(const TAny* anEntry, TInt aPos);
	inline TInt FindL(const TAny* anEntry) const;
	inline TInt FindReverseL(const TAny* anEntry) const;
	inline TInt FindInAddressOrderL(const TAny* anEntry) const;
	inline void FindInAddressOrderL(const TAny* anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInAddressOrderL(const TAny* anEntry, TInt aMode) const;
	inline void SpecificFindInAddressOrderL(const TAny* anEntry, TInt& anIndex, TInt aMode) const;
	inline void InsertInAddressOrderL(const TAny* anEntry);
	inline void InsertInAddressOrderAllowRepeatsL(const TAny* anEntry);

	inline RPointerArray(TAny** aEntries, TInt aCount);
	inline void GranularCompress();
	inline void SortIntoAddressOrder();
	inline TArray<TAny*> Array() const;
#endif
	};



/**
@internalComponent

Base class used in the derivation of RArray.

The base class is inherited privately.

The class is internal and is not intended for use.
*/
class RArrayBase
	{
protected:
	IMPORT_C RArrayBase(TInt anEntrySize);
	IMPORT_C RArrayBase(TInt anEntrySize, TInt aGranularity);
	IMPORT_C RArrayBase(TInt anEntrySize, TInt aGranularity, TInt aKeyOffset);
	IMPORT_C RArrayBase(TInt anEntrySize, TInt aMinGrowBy, TInt aKeyOffset, TInt aFactor);
	IMPORT_C RArrayBase(TInt aEntrySize,TAny* aEntries, TInt aCount);
	IMPORT_C void Close();
	IMPORT_C TInt Count() const;
	IMPORT_C TAny* At(TInt anIndex) const;
	IMPORT_C TInt Append(const TAny* anEntry);
	IMPORT_C TInt Insert(const TAny* anEntry, TInt aPos);
	IMPORT_C void Remove(TInt anIndex);
	IMPORT_C void Compress();
	IMPORT_C void Reset();
	IMPORT_C TInt Find(const TAny* anEntry) const;
	IMPORT_C TInt Find(const TAny* anEntry, TGeneralIdentityRelation anIdentity) const;
	IMPORT_C TInt FindReverse(const TAny* aEntry) const;
	IMPORT_C TInt FindReverse(const TAny* aEntry, TGeneralIdentityRelation aIdentity) const;
	IMPORT_C TInt FindIsqSigned(const TAny* anEntry) const;
	IMPORT_C TInt FindIsqUnsigned(const TAny* anEntry) const;
	IMPORT_C TInt FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder) const;
	IMPORT_C TInt FindIsqSigned(const TAny* anEntry, TInt aMode) const;
	IMPORT_C TInt FindIsqUnsigned(const TAny* anEntry, TInt aMode) const;
	IMPORT_C TInt FindIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TInt aMode) const;
	IMPORT_C TInt InsertIsqSigned(const TAny* anEntry, TBool aAllowRepeats);
	IMPORT_C TInt InsertIsqUnsigned(const TAny* anEntry, TBool aAllowRepeats);
	IMPORT_C TInt InsertIsq(const TAny* anEntry, TGeneralLinearOrder anOrder, TBool aAllowRepeats);
	IMPORT_C TInt BinarySearchSigned(const TAny* anEntry, TInt& anIndex) const;
	IMPORT_C TInt BinarySearchUnsigned(const TAny* anEntry, TInt& anIndex) const;
	IMPORT_C TInt BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder) const;
	IMPORT_C TInt BinarySearchSigned(const TAny* anEntry, TInt& anIndex, TInt aMode) const;
	IMPORT_C TInt BinarySearchUnsigned(const TAny* anEntry, TInt& anIndex, TInt aMode) const;
	IMPORT_C TInt BinarySearch(const TAny* anEntry, TInt& anIndex, TGeneralLinearOrder anOrder, TInt aMode) const;
#ifndef __KERNEL_MODE__
	IMPORT_C void GranularCompress();
	IMPORT_C TInt DoReserve(TInt aCount);
	IMPORT_C void HeapSortSigned();
	IMPORT_C void HeapSortUnsigned();
	IMPORT_C void HeapSort(TGeneralLinearOrder anOrder);
	IMPORT_C static TInt GetCount(const CBase* aPtr);
	IMPORT_C static const TAny* GetElementPtr(const CBase* aPtr, TInt aIndex);
#endif
private:
	TInt Grow();
private:
	TInt iCount;
	TAny* iEntries;
	TInt iEntrySize;
	TInt iKeyOffset;
	TInt iAllocated;
	TInt iGranularity;	// positive means linear, negative means exponential growth
	TInt iSpare1;
	TInt iSpare2;
	};




/**
@publishedAll
@released

A simple and efficient array of fixed length objects.

The elements of the array are instances of a class; this class is specified
as the template parameter T.

The array offers standard array behaviour which includes insertion, appending 
and sorting of elements.

Note:

1. where possible, this class should be used in preference to
   CArrayFixFlat<classT>.

2. the derivation from RArrayBase is private.

3. for performance reasons, RArray stores objects in the array as
   word (4 byte) aligned quantities. This means that some member functions
   do not work when RArray is instantiated for classes of less than 4 bytes
   in size, or when the class's alignment requirement is not 4.
   Be aware that it is possible to get an unhandled exception on hardware
   that enforces strict alignment.
   
   The affected functions are:
   
   3.1 the constructor: RArray(TInt, T*, TInt)
   
   3.2 Append(const T&)
   
   3.3 Insert(const T&, TInt)
   
   3.4 the [] operator, and then using the pointer to iterate through
       the array as you would with a C array.
*/
template <class T>
class RArray : private RArrayBase
	{
public:
	inline RArray();
	inline explicit RArray(TInt aGranularity);
	inline RArray(TInt aGranularity, TInt aKeyOffset);
	inline RArray(TInt aMinGrowBy, TInt aKeyOffset, TInt aFactor);
	inline RArray(TInt aEntrySize,T* aEntries, TInt aCount);
	inline void Close();
	inline TInt Count() const;
	inline const T& operator[](TInt anIndex) const;
	inline T& operator[](TInt anIndex);
	inline TInt Append(const T& anEntry);
	inline TInt Insert(const T& anEntry, TInt aPos);
	inline void Remove(TInt anIndex);
	inline void Compress();
	inline void Reset();
	inline TInt Find(const T& anEntry) const;
	inline TInt Find(const T& anEntry, TIdentityRelation<T> anIdentity) const;
	template <class K>
	inline TInt Find(const K& aKey, TBool (*apfnCompare)(const K* k, const T& t)) const
	/**
	Finds the first object in the array which matches aKey using
	the comparison algorithm provided by apfnCompare.
	
	The find operation always starts at the low index end of the array. There 
	is no assumption about the order of objects in the array.

	@param aKey The key of type K to be compared with the elements of the array using apfnCompare.
	@param apfnCompare A function defining the identity relation between the
			object in the array, and their keys of type K.  The function
			returns true if k and t match based on this relationship.
	
	@return The index of the first matching object within the array.
			KErrNotFound, if no suitable object can be found.
	*/
		{ return RArrayBase::Find((T*)&aKey,*(TIdentityRelation<T>*)&apfnCompare); }
	inline TInt FindReverse(const T& anEntry) const;
	inline TInt FindReverse(const T& anEntry, TIdentityRelation<T> anIdentity) const;
	template <class K>
	inline TInt FindReverse(const K& aKey, TInt (*apfnMatch)(const K* k, const T& t)) const 
	/**
	Finds the first object in the array which matches aKey using the comparison
	algorithm provided by apfnCompare.
	
	The find operation always starts at the high index end of the array. There 
	is no assumption about the order of objects in the array.

	@param aKey The key of type K to be compared with the elements of the array using apfnMatch.
	@param apfnMatch A function defining the identity relation between the
			object in the array, and their keys of type K.  The	function
			returns true if k and t match based on this relationship.
	
	@return The index of the first matching object within the array.
			KErrNotFound, if no suitable object can be found.
	*/	
		{ return RArrayBase::FindReverse((T*)&aKey,*(TIdentityRelation<T>*)&apfnMatch); }		
	inline TInt FindInSignedKeyOrder(const T& anEntry) const;
	inline TInt FindInUnsignedKeyOrder(const T& anEntry) const;
	inline TInt FindInOrder(const T& anEntry, TLinearOrder<T> anOrder) const;
	inline TInt FindInSignedKeyOrder(const T& anEntry, TInt& anIndex) const;
	inline TInt FindInUnsignedKeyOrder(const T& anEntry, TInt& anIndex) const;
	inline TInt FindInOrder(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const;
	template <class K>
	inline TInt FindInOrder(const K& aKey, TInt (*apfnCompare)(const K* k, const T& t)) const
	/**
	Finds the object in the array whose object matches the specified
	key, (Using the relationship defined within apfnCompare) using a binary search
	technique and an ordering algorithm.

	The function assumes that existing objects in the array are ordered so
	that the objects themselves are in object order as determined by an algorithm 
	supplied by the caller and packaged as a TLinearOrder<T>.

	@param aKey The key of type K to be compared with the elements of the array using apfnCompare.
	@param apfnCompare A function which defines the order that the array was sorted,
		 where in it aKey (via the defined relationship) should fit, and if the key is present. 
	
	@return The index of the matching object within the array.
			KErrNotFound, if no suitable object can be found.
	*/	

		{ return RArrayBase::FindIsq((T*)&aKey,*(TLinearOrder<T>*)&apfnCompare); }
	inline TInt SpecificFindInSignedKeyOrder(const T& anEntry, TInt aMode) const;
	inline TInt SpecificFindInUnsignedKeyOrder(const T& anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrder(const T& anEntry, TLinearOrder<T> anOrder, TInt aMode) const;
	inline TInt SpecificFindInSignedKeyOrder(const T& anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt SpecificFindInUnsignedKeyOrder(const T& anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt SpecificFindInOrder(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const;
	inline TInt InsertInSignedKeyOrder(const T& anEntry);
	inline TInt InsertInUnsignedKeyOrder(const T& anEntry);
	inline TInt InsertInOrder(const T& anEntry, TLinearOrder<T> anOrder);
	inline TInt InsertInSignedKeyOrderAllowRepeats(const T& anEntry);
	inline TInt InsertInUnsignedKeyOrderAllowRepeats(const T& anEntry);
	inline TInt InsertInOrderAllowRepeats(const T& anEntry, TLinearOrder<T> anOrder);
#ifndef __KERNEL_MODE__
	inline void AppendL(const T& anEntry);
	inline void InsertL(const T& anEntry, TInt aPos);
	inline TInt FindL(const T& anEntry) const;
	inline TInt FindL(const T& anEntry, TIdentityRelation<T> anIdentity) const;
	inline TInt FindReverseL(const T& anEntry) const;
	inline TInt FindReverseL(const T& anEntry, TIdentityRelation<T> anIdentity) const;
	inline TInt FindInSignedKeyOrderL(const T& anEntry) const;
	inline TInt FindInUnsignedKeyOrderL(const T& anEntry) const;
	inline TInt FindInOrderL(const T& anEntry, TLinearOrder<T> anOrder) const;
	inline void FindInSignedKeyOrderL(const T& anEntry, TInt& anIndex) const;
	inline void FindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex) const;
	inline void FindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const;
	inline TInt SpecificFindInSignedKeyOrderL(const T& anEntry, TInt aMode) const;
	inline TInt SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrderL(const T& anEntry, TLinearOrder<T> anOrder, TInt aMode) const;
	inline void SpecificFindInSignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const;
	inline void SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const;
	inline void SpecificFindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const;
	inline void InsertInSignedKeyOrderL(const T& anEntry);
	inline void InsertInUnsignedKeyOrderL(const T& anEntry);
	inline void InsertInOrderL(const T& anEntry, TLinearOrder<T> anOrder);
	inline void InsertInSignedKeyOrderAllowRepeatsL(const T& anEntry);
	inline void InsertInUnsignedKeyOrderAllowRepeatsL(const T& anEntry);
	inline void InsertInOrderAllowRepeatsL(const T& anEntry, TLinearOrder<T> anOrder);

	inline void GranularCompress();
	inline TInt Reserve(TInt aCount);
	inline void ReserveL(TInt aCount);
	inline void SortSigned();
	inline void SortUnsigned();
	inline void Sort(TLinearOrder<T> anOrder);
	inline TArray<T> Array() const;
#endif
	};




/**
@publishedAll
@released

A simple and efficient specialized array of signed integers offering standard 
array behaviour.

Note that derivation from RPointerArrayBase is private.
*/
TEMPLATE_SPECIALIZATION class RArray<TInt> : private RPointerArrayBase
	{
public:
	inline RArray();
	inline explicit RArray(TInt aGranularity);
	inline RArray(TInt aMinGrowBy, TInt aFactor);
	inline void Close();
	inline TInt Count() const;
	inline const TInt& operator[](TInt anIndex) const;
	inline TInt& operator[](TInt anIndex);
	inline TInt Append(TInt anEntry);
	inline TInt Insert(TInt anEntry, TInt aPos);
	inline void Remove(TInt anIndex);
	inline void Compress();
	inline void Reset();
	inline TInt Find(TInt anEntry) const;
	inline TInt FindReverse(TInt anEntry) const;
	inline TInt FindInOrder(TInt anEntry) const;
	inline TInt FindInOrder(TInt anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInOrder(TInt anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrder(TInt anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt InsertInOrder(TInt anEntry);
	inline TInt InsertInOrderAllowRepeats(TInt anEntry);
#ifndef __KERNEL_MODE__
	inline void AppendL(TInt anEntry);
	inline void InsertL(TInt anEntry, TInt aPos);
	inline TInt FindL(TInt anEntry) const;
	inline TInt FindReverseL(TInt anEntry) const;
	inline TInt FindInOrderL(TInt anEntry) const;
	inline void FindInOrderL(TInt anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInOrderL(TInt anEntry, TInt aMode) const;
	inline void SpecificFindInOrderL(TInt anEntry, TInt& anIndex, TInt aMode) const;
	inline void InsertInOrderL(TInt anEntry);
	inline void InsertInOrderAllowRepeatsL(TInt anEntry);

	inline RArray(TInt* aEntries, TInt aCount);
	inline void GranularCompress();
	inline TInt Reserve(TInt aCount);
	inline void ReserveL(TInt aCount);
	inline void Sort();
	inline TArray<TInt> Array() const;
#endif
	};




/**
@publishedAll
@released

Array of unsigned integers.

The array is a simple and efficient specialized array of unsigned integers 
offering standard array behaviour.

The derivation from RPointerArrayBase is private.
*/
TEMPLATE_SPECIALIZATION class RArray<TUint> : private RPointerArrayBase
	{
public:
	inline RArray();
	inline explicit RArray(TInt aGranularity);
	inline RArray(TInt aMinGrowBy, TInt aFactor);
	inline void Close();
	inline TInt Count() const;
	inline const TUint& operator[](TInt anIndex) const;
	inline TUint& operator[](TInt anIndex);
	inline TInt Append(TUint anEntry);
	inline TInt Insert(TUint anEntry, TInt aPos);
	inline void Remove(TInt anIndex);
	inline void Compress();
	inline void Reset();
	inline TInt Find(TUint anEntry) const;
	inline TInt FindReverse(TUint anEntry) const;
	inline TInt FindInOrder(TUint anEntry) const;
	inline TInt FindInOrder(TUint anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInOrder(TUint anEntry, TInt aMode) const;
	inline TInt SpecificFindInOrder(TUint anEntry, TInt& anIndex, TInt aMode) const;
	inline TInt InsertInOrder(TUint anEntry);
	inline TInt InsertInOrderAllowRepeats(TUint anEntry);
#ifndef __KERNEL_MODE__
	inline void AppendL(TUint anEntry);
	inline void InsertL(TUint anEntry, TInt aPos);
	inline TInt FindL(TUint anEntry) const;
	inline TInt FindReverseL(TUint anEntry) const;
	inline TInt FindInOrderL(TUint anEntry) const;
	inline void FindInOrderL(TUint anEntry, TInt& anIndex) const;
	inline TInt SpecificFindInOrderL(TUint anEntry, TInt aMode) const;
	inline void SpecificFindInOrderL(TUint anEntry, TInt& anIndex, TInt aMode) const;
	inline void InsertInOrderL(TUint anEntry);
	inline void InsertInOrderAllowRepeatsL(TUint anEntry);

	inline RArray(TUint* aEntries, TInt aCount);
	inline void GranularCompress();
	inline TInt Reserve(TInt aCount);
	inline void ReserveL(TInt aCount);
	inline void Sort();
	inline TArray<TUint> Array() const;
#endif
	};

#ifndef __LEAVE_EQUALS_THROW__

class TTrapHandler;

/**
@internalComponent
*/
class TTrap
	{
public:
#ifndef __KERNEL_MODE__
	IMPORT_C TInt Trap(TInt& aResult);
	IMPORT_C static void UnTrap();
#endif
public:
	enum {EMaxState=0x10};
public:
	TInt iState[EMaxState];
	TTrap* iNext;
	TInt* iResult;
	TTrapHandler* iHandler;
	};



/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.

Use this macro as a C++ statement.

_r must be a TInt which has already been declared; if any of the
C++ statements _s leaves, then the leave code is returned in _r,
otherwise _r is set to KErrNone.

_s can consist of multiple C++ statements; in theory, _s can consist
of any legal C++ code but in practice, such statements consist of simple
function calls, e.g. Foo() or an assignment of some value to the result of
a function call, e.g. functionValue=GetFoo().

A cleanup stack is constructed for the set of C++ statements _s.
If any function in _s leaves, objects pushed to the cleanup stack are
cleaned-up. In addition, if any of the C++ statements in _s leaves,
then remaining C++ code in _s is not executed and any variables which
are assigned within that remaining code are not defined.

@param _r An lvalue, convertible to TInt&, which will receive the result of
          any User::Leave() executed within _s or, if no leave occurred,
          it will be set to KErrNone. The value of _r on entry is not used.

@param _s C++ statements which will be executed under a trap harness.

@see TRAPD
*/
#define TRAP(_r,_s) {TTrap __t;if (__t.Trap(_r)==0){_s;TTrap::UnTrap();}}

/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.

Use this macro in the same way as you would TRAP, except that the
variable _r is defined as part of the macro (and is therefore valid for the
rest of the block in which the macro occurs). Often, this saves a line of code.

@param _r A name, which will be declared as a TInt, and will receive the result
          of any User::Leave() executed within _s or, if no leave occurred, it
          will be set to KErrNone. After the macro, _r remains in scope until
          the end of its enclosing block.

@param _s C++ statements which will be executed under a trap harness.

@see TRAP
*/
#define TRAPD(_r,_s) TInt _r;{TTrap __t;if (__t.Trap(_r)==0){_s;TTrap::UnTrap();}}

/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.
Any leave code generated is ignored.

Use this macro as a C++ statement.

This macro is functionally equivalent to:
@code
	TInt x;
	TRAP(x,_s)
@endcode
or
@code
	TRAPD(x,_s)
@endcode
where the value in 'x' is not used by any subsequent code.

_s can consist of multiple C++ statements; in theory, _s can consist
of any legal C++ code but in practice, such statements consist of simple
function calls, e.g. Foo() or an assignment of some value to the result of
a function call, e.g. functionValue=GetFoo().

A cleanup stack is constructed for the set of C++ statements _s.
If any function in _s leaves, objects pushed to the cleanup stack are
cleaned-up. In addition, if any of the C++ statements in _s leaves,
then remaining C++ code in _s is not executed and any variables which
are assigned within that remaining code are not defined.

@param _s C++ statements which will be executed under a trap harness.

@see TRAPD
@see TRAP
*/
#define TRAP_IGNORE(_s) {TInt _ignore;TTrap __t;if (__t.Trap(_ignore)==0){_s;TTrap::UnTrap();}}


#else //__LEAVE_EQUALS_THROW__

#ifdef __WINS__
/** @internalComponent */
#define __WIN32SEHTRAP		TWin32SEHTrap __trap; __trap.Trap();
/** @internalComponent */
#define __WIN32SEHUNTRAP	__trap.UnTrap();
IMPORT_C void EmptyFunction();
#define __CALL_EMPTY_FUNCTION	EmptyFunction();   
#else // !__WINS__
#define __WIN32SEHTRAP
#define __WIN32SEHUNTRAP
#define __CALL_EMPTY_FUNCTION
#endif //__WINS__

/** 
This macro is used by the TRAP and TRAPD macros and provides a means
of inserting code into uses of these.

This macro is invoked before any 'trapped' code is called, and it should be
redefined to do whatever task is required. E.g. this code:

@code
    #undef TRAP_INSTRUMENTATION_START
    #define TRAP_INSTRUMENTATION_START DoMyLoging(__LINE__)
@endcode

Will cause all subsequent uses of the TRAP macros to behave in an
equivalent way to:

@code
    DoMyLoging(__LINE__)
    TRAP(r,SomeCodeL());
@endcode


@publishedPartner
@released

@see TRAP
@see TRAPD
*/
#define TRAP_INSTRUMENTATION_START



/** 
This macro is used by the TRAP and TRAPD macros and provides a means
of inserting code into uses of these.

This macro is invoked if the 'trapped' code did not Leave.
E.g. this code:

@code
    #undef TRAP_INSTRUMENTATION_NOLEAVE
    #define TRAP_INSTRUMENTATION_NOLEAVE DoMyLoging(__LINE__)
@endcode

Will cause all subsequent uses of the TRAP macros to behave in an
equivalent way to:

@code
    TRAP(r,SomeCodeL());
    if(r==KErrNone) DoMyLoging(__LINE__);
@endcode


@param aLine The line number in the C++ source file where the TRAP or TRAPD
             macro was used.

@publishedPartner
@released

@see TRAP
@see TRAPD
*/
#define TRAP_INSTRUMENTATION_NOLEAVE


/** 
This macro is used by the TRAP and TRAPD macros and provides a means
of inserting code into uses of these.

This macro is invoked if the 'trapped' code did Leave. E.g. this code:

@code
    #undef TRAP_INSTRUMENTATION_LEAVE
    #define TRAP_INSTRUMENTATION_LEAVE(aResult) DoMyLoging(aResult,__LINE__)
@endcode

Will cause all subsequent uses of the TRAP macros to behave in an
equivalent way to:

@code
    TRAP(r,SomeCodeL());
    if(r!=KErrNone) DoMyLoging(r,__LINE__);
@endcode


@param aResult  A reference to the result value used in the TRAP macro.


@publishedPartner
@released

@see TRAP
@see TRAPD
*/
#define TRAP_INSTRUMENTATION_LEAVE(aResult)



/** 
This macro is used by the TRAP and TRAPD macros and provides a means
of inserting code into uses of these.

This macro is invoked after the 'trapped' code is called, regardless of whether
or not it did Leave.  It should be redefined to do whatever task is
required. E.g. this code:

@code
    #undef TRAP_INSTRUMENTATION_END
    #define TRAP_INSTRUMENTATION_END DoMyLoging(__LINE__)
@endcode

Will cause all subsequent uses of the TRAP macros to behave in an
equivalent way to:

@code
    TRAP(r,SomeCodeL());
    DoMyLoging(__LINE__)
@endcode


@publishedPartner
@released

@see TRAP
@see TRAPD
*/
#define TRAP_INSTRUMENTATION_END



/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.

Use this macro as a C++ statement.

_r must be a TInt which has already been declared; if any of the
C++ statements _s leaves, then the leave code is returned in _r,
otherwise _r is set to KErrNone.

_s can consist of multiple C++ statements; in theory, _s can consist
of any legal C++ code but in practice, such statements consist of simple
function calls, e.g. Foo() or an assignment of some value to the result of
a function call, e.g. functionValue=GetFoo().

A cleanup stack is constructed for the set of C++ statements _s.
If any function in _s leaves, objects pushed to the cleanup stack are
cleaned-up. In addition, if any of the C++ statements in _s leaves,
then remaining C++ code in _s is not executed and any variables which
are assigned within that remaining code are not defined.

@param _r An lvalue, convertible to TInt&, which will receive the result of
          any User::Leave() executed within _s or, if no leave occurred,
          it will be set to KErrNone. The value of _r on entry is not used.

@param _s C++ statements which will be executed under a trap harness.

@see TRAPD
*/

/*__CALL_EMPTY_FUNCTION(call to a function with an empty body) was added as a 
workaround to a compiler bug (mwccsym2 - winscw ) which caused an incorrect 
trap handler to be invoked when multiple nested TRAP's were present and 
User::Leave(..) was called. */

#define TRAP(_r, _s)										\
	{														\
	TInt& __rref = _r;										\
	__rref = 0;												\
	{ TRAP_INSTRUMENTATION_START; }							\
	try	{													\
		__WIN32SEHTRAP										\
		TTrapHandler* ____t = User::MarkCleanupStack();		\
		_s;													\
		User::UnMarkCleanupStack(____t);					\
		{ TRAP_INSTRUMENTATION_NOLEAVE; }					\
		__WIN32SEHUNTRAP									\
		}													\
	catch (XLeaveException& l)								\
		{													\
		__rref = l.GetReason();								\
		{ TRAP_INSTRUMENTATION_LEAVE(__rref); }				\
		}													\
	catch (...)												\
		{													\
		User::Invariant();									\
		}													\
	__CALL_EMPTY_FUNCTION									\
	{ TRAP_INSTRUMENTATION_END; }							\
	}


/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.

Use this macro in the same way as you would TRAP, except that the
variable _r is defined as part of the macro (and is therefore valid for the
rest of the block in which the macro occurs). Often, this saves a line of code.

@param _r A name, which will be declared as a TInt, and will receive the result
          of any User::Leave() executed within _s or, if no leave occurred, it
          will be set to KErrNone. After the macro, _r remains in scope until
          the end of its enclosing block.

@param _s C++ statements which will be executed under a trap harness.

@see TRAP
*/

/*__CALL_EMPTY_FUNCTION(call to a function with an empty body) was added as a 
workaround to a compiler bug (mwccsym2 - winscw ) which caused an incorrect 
trap handler to be invoked when multiple nested TRAP's were present and 
User::Leave(..) was called. */


#define TRAPD(_r, _s)										\
	TInt _r;												\
	{														\
	_r = 0;													\
	{ TRAP_INSTRUMENTATION_START; }							\
	try	{													\
		__WIN32SEHTRAP										\
		TTrapHandler* ____t = User::MarkCleanupStack();		\
		_s;													\
		User::UnMarkCleanupStack(____t);					\
		{ TRAP_INSTRUMENTATION_NOLEAVE; }					\
		__WIN32SEHUNTRAP									\
		}													\
	catch (XLeaveException& l)								\
		{													\
		_r = l.GetReason();									\
		{ TRAP_INSTRUMENTATION_LEAVE(_r); }					\
		}													\
	catch (...)												\
		{													\
		User::Invariant();									\
		}													\
	__CALL_EMPTY_FUNCTION									\
	{ TRAP_INSTRUMENTATION_END; }							\
	}

/**
@publishedAll
@released

Executes the set of C++ statements _s under a trap harness.
Any leave code generated is ignored.

Use this macro as a C++ statement.

This macro is functionally equivalent to:
@code
	TInt x;
	TRAP(x,_s)
@endcode
or
@code
	TRAPD(x,_s)
@endcode
where the value in 'x' is not used by any subsequent code.

Use this macro as a C++ statement.

_s can consist of multiple C++ statements; in theory, _s can consist
of any legal C++ code but in practice, such statements consist of simple
function calls, e.g. Foo() or an assignment of some value to the result of
a function call, e.g. functionValue=GetFoo().

A cleanup stack is constructed for the set of C++ statements _s.
If any function in _s leaves, objects pushed to the cleanup stack are
cleaned-up. In addition, if any of the C++ statements in _s leaves,
then remaining C++ code in _s is not executed and any variables which
are assigned within that remaining code are not defined.

@param _s C++ statements which will be executed under a trap harness.

@see TRAPD
@see TRAP
*/

/*__CALL_EMPTY_FUNCTION(call to a function with an empty body) was added as a 
workaround to a compiler bug (mwccsym2 - winscw ) which caused an incorrect 
trap handler to be invoked when multiple nested TRAP's were present and 
User::Leave(..) was called. */

#define TRAP_IGNORE(_s)										\
	{														\
	{ TRAP_INSTRUMENTATION_START; }							\
	try	{													\
		__WIN32SEHTRAP										\
		TTrapHandler* ____t = User::MarkCleanupStack();		\
		_s;													\
		User::UnMarkCleanupStack(____t);					\
		{ TRAP_INSTRUMENTATION_NOLEAVE; }					\
		__WIN32SEHUNTRAP									\
		}													\
	catch (XLeaveException& l)								\
		{													\
		l.GetReason();										\
		{ TRAP_INSTRUMENTATION_LEAVE(l.Reason()); }			\
		}													\
	catch (...)												\
		{													\
		User::Invariant();									\
		}													\
	__CALL_EMPTY_FUNCTION									\
	{ TRAP_INSTRUMENTATION_END; }							\
	}


#endif //__LEAVE_EQUALS_THROW__

/* The macro __SYMBIAN_STDCPP_SUPPORT__ is defined when building a StdC++ target.
 * In this case, operator new and operator delete below should not be declared
 * to avoid clashing with StdC++ declarations.
 */ 

#ifndef __SYMBIAN_STDCPP_SUPPORT__

#ifndef __OPERATOR_NEW_DECLARED__

/* Some operator new and operator delete overloads may be declared in compiler
 * pre-include files.
 *
 * __OPERATOR_NEW_DECLARED__ is #defined if they are, so that we can avoid
 * re-declaring them here.
 */

#define __OPERATOR_NEW_DECLARED__

/**
@publishedAll
@released
*/
GLREF_C TAny* operator new(TUint aSize) __NO_THROW;

/**
@publishedAll
@released
*/
GLREF_C TAny* operator new(TUint aSize,TUint anExtraSize) __NO_THROW;

/**
@publishedAll
@released
*/
GLREF_C void operator delete(TAny* aPtr) __NO_THROW;

#ifndef __OMIT_VEC_OPERATOR_NEW_DECL__
/**
@publishedAll
@released
*/
GLREF_C TAny* operator new[](TUint aSize) __NO_THROW;

/**
@publishedAll
@released
*/
GLREF_C void operator delete[](TAny* aPtr) __NO_THROW;
#endif // !__OMIT_VEC_OPERATOR_NEW_DECL__

#endif // !__OPERATOR_NEW_DECLARED__

#endif // !__SYMBIAN_STDCPP_SUPPORT__

/**
@publishedAll
@released
*/
inline TAny* operator new(TUint aSize, TAny* aBase) __NO_THROW;

/**
@publishedAll
@released
*/
inline void operator delete(TAny* aPtr, TAny* aBase) __NO_THROW;

#ifndef __PLACEMENT_VEC_NEW_INLINE
/**
@publishedAll
@released
*/
inline TAny* operator new[](TUint aSize, TAny* aBase) __NO_THROW;

/**
@publishedAll
@released
*/
inline void operator delete[](TAny* aPtr, TAny* aBase) __NO_THROW;

#endif // !__PLACEMENT_VEC_NEW_INLINE

#if !defined(__BOOL_NO_TRUE_TRAP__)

/**
@publishedAll
@released
*/
TBool operator==(TTrue,volatile const TBool);

/**
@publishedAll
@released
*/
TBool operator==(volatile const TBool,TTrue);

/**
@publishedAll
@released
*/
TBool operator!=(TTrue,volatile const TBool);

/**
@publishedAll
@released
*/
TBool operator!=(volatile const TBool,TTrue);
#endif




/**
@publishedAll
@released

A Version 2 client/server class that clients use to package 
the arguments to be sent to a server.

The object can package up to 4 arguments together with information about each
argument's type, width and accessibility; it is also possible for
the package to contain zero arguments. In addition to the default constructor,
the class has four templated constructors, allowing an object of this type to
be constructed for 0, 1, 2, 3 or 4 arguments.

Internally, the arguments are stored in a simple TInt array.
Consecutive arguments in a constructor's parameter list are put into
consecutive slots in the array. The Set() overloaded functions can be used
to set argument values into specific slots within this array.
*/
class TIpcArgs
	{
public:
    /**
    @internalComponent
    
    Argument types; some of these may be ORed together to specify
	type, accessibility, and width.
    */
	enum TArgType
		{
		EUnspecified = 0,                         /**< Type not specified.*/
		EHandle = 1,                              /**< Handle type.*/
		EFlagDes = 4,                             /**< Descriptor type.*/
		EFlagConst = 2,                           /**< Read only type.*/
		EFlag16Bit = 1,                           /**< 16 bit rather than 8 bit.*/
		EDes8 = EFlagDes,                         /**< 8 bit read/write descriptor.*/
		EDes16 = EFlagDes|EFlag16Bit,             /**< 16 bit read/write descriptor.*/
		EDesC8 = EFlagDes|EFlagConst,             /**< 8 bit read only descriptor.*/
		EDesC16 = EFlagDes|EFlagConst|EFlag16Bit, /**< 16 bit read only descriptor.*/
		};


    /**
    @internalComponent
	*/
	enum 
		{
		KBitsPerType	= 3, 		/**< Number of bits of type information used for each of the 4 arguments.*/
		KPinArgShift	= KBitsPerType*KMaxMessageArguments,	/**< Bit number of the start of the pin flags. */
		KPinArg0		= 1<<(KPinArgShift+0),	/**< Set to pin argument at index 0.*/
		KPinArg1		= 1<<(KPinArgShift+1),	/**< Set to pin argument at index 1.*/
		KPinArg2		= 1<<(KPinArgShift+2),	/**< Set to pin argument at index 2.*/
		KPinArg3		= 1<<(KPinArgShift+3),	/**< Set to pin argument at index 3.*/
		KPinMask 		= 0xf<<KPinArgShift,	/**< The bits used for the pinning attributes of each argument.*/
		};
	
	
	/**
	Indicates a Null argument.
	*/
	enum TNothing {
	              /**
	              An enum value that can be used to indicate an empty or
	              unused argument to a server. For example:
	
                  @code
                  TIpcArgs args(arg1, TIpcArgs::ENothing, arg2);
                  @endcode
    
                  This argument will have an undefined value when the server
                  receives the message.
	              */
	              ENothing
	              };
public:
    /**
    Default constructor.
    
    An argument package constructed using this constructor has no arguments;
    however, arguments can subsequently be set into this argument package object
    using the Set() member functions.
    */
	inline TIpcArgs()
		:iFlags(0)
		{}
		
		
    /**
    A templated constructor that constructs the argument package; it takes
    1 argument.
    
    @param a0 An argument of general class type T0 to be contained by
              this object.
    */		
	template <class T0>
	inline explicit TIpcArgs(T0 a0)
		{
		Assign(iArgs[0],a0);
		iFlags=(Type(a0)<<(0*KBitsPerType));
		}
		
		
    /**
    A templated constructor that constructs the argument package; it takes
    2 arguments.
    
    @param a0 An argument of general class type T0 to be contained by
              this object.
    @param a1 An argument of general class type T1 to be contained by
              this object.
    */		
	template <class T0,class T1>
	inline TIpcArgs(T0 a0,T1 a1)
		{
		Assign(iArgs[0],a0);
		Assign(iArgs[1],a1);
		iFlags=(Type(a0)<<(0*KBitsPerType))|(Type(a1)<<(1*KBitsPerType));
		}
				
		
    /**
    A templated constructor that constructs the argument package; it takes
    3 arguments.
    
    @param a0 An argument of general class type T0 to be contained by
              this object.
    @param a1 An argument of general class type T1 to be contained by
              this object.
    @param a2 An argument of general class type T2 to be contained by
              this object.
    */		
	template <class T0,class T1,class T2>
	inline TIpcArgs(T0 a0,T1 a1,T2 a2)
		{
		Assign(iArgs[0],a0);
		Assign(iArgs[1],a1);
		Assign(iArgs[2],a2);
		iFlags=(Type(a0)<<(0*KBitsPerType))|(Type(a1)<<(1*KBitsPerType))|(Type(a2)<<(2*KBitsPerType));
		}


    /**
    A templated constructor that constructs the argument package; it takes
    4 arguments.
    
    @param a0 An argument of general class type T0 to be contained by
              this object.
    @param a1 An argument of general class type T1 to be contained by
              this object.
    @param a2 An argument of general class type T2 to be contained by
              this object.
    @param a3 An argument of general class type T3 to be contained by
              this object.
    */		
	template <class T0,class T1,class T2,class T3>
	inline TIpcArgs(T0 a0,T1 a1,T2 a2,T3 a3)
		{
		Assign(iArgs[0],a0);
		Assign(iArgs[1],a1);
		Assign(iArgs[2],a2);
		Assign(iArgs[3],a3);
		iFlags=(Type(a0)<<(0*KBitsPerType))|(Type(a1)<<(1*KBitsPerType))|(Type(a2)<<(2*KBitsPerType))|(Type(a3)<<(3*KBitsPerType));
		}
	//
	inline void Set(TInt aIndex,TNothing);
	inline void Set(TInt aIndex,TInt aValue);
	inline void Set(TInt aIndex,const TAny* aValue);
	inline void Set(TInt aIndex,RHandleBase aValue);
	inline void Set(TInt aIndex,const TDesC8* aValue);
#ifndef __KERNEL_MODE__
	inline void Set(TInt aIndex,const TDesC16* aValue);
#endif
	inline void Set(TInt aIndex,TDes8* aValue);
#ifndef __KERNEL_MODE__
	inline void Set(TInt aIndex,TDes16* aValue);
#endif

	inline TIpcArgs& PinArgs(TBool aPinArg0=ETrue, TBool aPinArg1=ETrue, TBool aPinArg2=ETrue, TBool aPinArg3=ETrue);
private:
	inline static TArgType Type(TNothing);
	inline static TArgType Type(TInt);
	inline static TArgType Type(const TAny*);
	inline static TArgType Type(RHandleBase aValue);
	inline static TArgType Type(const TDesC8*);
#ifndef __KERNEL_MODE__
	inline static TArgType Type(const TDesC16*);
#endif
	inline static TArgType Type(TDes8*);
#ifndef __KERNEL_MODE__
	inline static TArgType Type(TDes16*);
#endif
	//
	inline static void Assign(TInt&,TNothing);
	inline static void Assign(TInt& aArg,TInt aValue);
	inline static void Assign(TInt& aArg,const TAny* aValue);
	inline static void Assign(TInt& aArg,RHandleBase aValue);
	inline static void Assign(TInt& aArg,const TDesC8* aValue);
#ifndef __KERNEL_MODE__
	inline static void Assign(TInt& aArg,const TDesC16* aValue);
#endif
	inline static void Assign(TInt& aArg,TDes8* aValue);
#ifndef __KERNEL_MODE__
	inline static void Assign(TInt& aArg,TDes16* aValue);
#endif
public:
    
    /**
    The location where the message arguments are stored.
    
    There is no reason to access this data member directly and it should be
    considered as internal.
    */
	TInt iArgs[KMaxMessageArguments];
	
	/**
	The location where the flag bits describing the argument types are stored.
	
	The symbolic values describing the argument types are internal to Symbian,
	and there is therefore no reason to access this data member directly.
	It should be considered as internal.
	*/
	TInt iFlags;
	};

// Structures for passing 64 bit integers and doubles across GCC/EABI boundaries

/**
@internalComponent
*/
struct SInt64
	{
public:
	inline SInt64();
	inline SInt64(Int64 a);
	inline SInt64& operator=(Int64 a);
	inline operator Int64() const;
public:
	TUint32 iData[2];	// little endian
	};

/**
@internalComponent
*/
struct SUint64
	{
public:
	inline SUint64();
	inline SUint64(Uint64 a);
	inline SUint64& operator=(Uint64 a);
	inline operator Uint64() const;
public:
	TUint32 iData[2];	// little endian
	};

/**
@internalComponent
*/
struct SDouble
	{
public:
	inline SDouble();
	inline SDouble(TReal a);
	inline SDouble& operator=(TReal a);
	inline operator TReal() const;
public:
	TUint32 iData[2];	// always little endian
	};

/**
@publishedAll
@released

Stores information about a thread's stack.

Note, on the emulator, the memory between iLimit and the thread's current stack pointer
may not actually be committed.

@see RThread::StackInfo()
*/
class TThreadStackInfo
	{
public:
    /**
    The address which the stack pointer would contain if the stack were empty.
    */
	TLinAddr iBase;
	
	/**
	The address which the stack pointer would contain if the stack were full,
    (The lowest valid address).
	*/
	TLinAddr iLimit;
	
	/**
	The limit value for the stack if it were expanded to its maximum size.
    
    Currently expanding stacks is not supported so iExpandLimit==iLimit
	*/
	TLinAddr iExpandLimit;
	};




#ifdef __SUPPORT_CPP_EXCEPTIONS__
/**
@internalComponent
@released

The class used to implement User::Leave in term of throw and TRAP in terms of catch.

*/
class XLeaveException
	{
public:
	inline XLeaveException() {}
	inline XLeaveException(TInt aReason) {iR = aReason;}
	inline TInt Reason() const {return iR;}
	IMPORT_C TInt GetReason() const;
private:
#if __ARMCC_VERSION >= 220000
	// From rvct 2.2 onwards we want the class impedimenta to be shared, so create a key function.
	// Unfortunately we can't make this the key function the dtor since this would make it impossible for existing 2.1 
	// derived binaries to be 'BC' with 2.2 binaries (in the general case (which I wont attempt to describe coz its
	// too complex) so its best to be safe). As a clue: if 2.1 is used to compile with a key function its not possible 
	// for catch handlers to work :-( (see the old code).
	virtual void ForceKeyFunction();	
#endif
private:
#if __ARMCC_VERSION < 220000
	TAny* iVtable;							// reserve space for vtable
#endif	
	TInt iR;
	};

// The standard header file <exception> defines the following guard macro for EDG and CW, VC++, GCC respectively.
// The guard below is ugly. It will surely come back and bite us unless we resolve the whole issue of standard headers
// when we move to supporting Standard C++.

// The macro __SYMBIAN_STDCPP_SUPPORT__ is defined when building a StdC++ target.
// In this case, we include the StdC++ specification <exception> rather than declaring uncaught_exception.
 
#ifdef __SYMBIAN_STDCPP_SUPPORT__
	#include <stdapis/stlportv5/exception>
#elif !defined(_EXCEPTION) && !defined(_EXCEPTION_) && !defined(__EXCEPTION__)
// Declare standard C++ functions relating to exceptions here
namespace std {
#if defined(__VC32__) || defined(__CW32__)
  bool uncaught_exception();
#else
  IMPORT_C bool uncaught_exception();
#endif
  void terminate(void);
  void unexpected(void);
  typedef void (*terminate_handler)();
  terminate_handler set_terminate(terminate_handler h) throw();
  typedef void (*unexpected_handler)();
  unexpected_handler set_unexpected(unexpected_handler h) throw();
}

#endif
#endif //__SUPPORT_CPP_EXCEPTIONS__

#ifdef __WINS__

#ifndef __WIN32_SEH_TYPES_KNOWN__
class __UnknownWindowsType1;
class __UnknownWindowsType2;
#endif

class TWin32SEHTrap;

/**
 * Typedef for the SEH handler function
 * @internalComponent
 */
typedef TUint32 (TWin32SEHExceptionHandler)(__UnknownWindowsType1* aExceptionRecord, TWin32SEHTrap* aRegistrationRecord, __UnknownWindowsType2* aContext);

/**
 * @internalComponent
 */
class TWin32SEHTrap
	{
private:
	// Prevent copy/assign
    TWin32SEHTrap(TWin32SEHTrap const &);
    TWin32SEHTrap& operator=(TWin32SEHTrap const &);

#ifdef __KERNEL_MODE__
//
// Kernel-side functions for nkern exception handler
//
public:
	/** Find final exception handler in SEH chain */
	static TWin32SEHTrap* IterateForFinal();

	/** Access exception handler */
	TWin32SEHExceptionHandler* ExceptionHandler();

private:

#else // !__KERNEL_MODE__
//
// User-side functions for use in TRAP(...)
//
public:
	UIMPORT_C TWin32SEHTrap();

public:
	/** Add object to SEH chain */
	UIMPORT_C void Trap();

	/** Remove object from SEH chain */
	UIMPORT_C void UnTrap();

#ifndef __IN_SEH_CPP__
private:
#endif
	/** Handle Win32 exceptions */
	static TUint32 ExceptionHandler(__UnknownWindowsType1* aException, TWin32SEHTrap* aRegistrationRecord, __UnknownWindowsType2* aContext);

#endif //__KERNEL_MODE__

	//
	// NB: This is really an _EXCEPTION_REGISTRATION_RECORD
	//
    TWin32SEHTrap*					iPrevExceptionRegistrationRecord;	/** Link to previous SEH record */
	TWin32SEHExceptionHandler*		iExceptionHandler;					/** SEH handler function */

private:
	TUint32 iPadding[254];	// discourage the compiler from putting this in reused function parameter space
	};

#else // !__WINS__

#ifdef __X86__
/**
 * @internalComponent
 */
class TWin32SEHTrap
	{
public:
	UIMPORT_C TWin32SEHTrap();
	UIMPORT_C void Trap();
	UIMPORT_C void UnTrap();
	};
#endif //__X86__
#endif //__WINS__

/**
@internalTechnology
 */
struct TEmulatorImageHeader
	{
	TUid iUids[KMaxCheckedUid];
	TProcessPriority iPriority;
	SSecurityInfo iS;
	TUint32 iSpare1;
	TUint32 iSpare2;
	TUint32 iModuleVersion;
	TUint32 iFlags;
	};

// forward declaration of shareable data buffers pool infomation
class TShPoolInfo;

#include <e32cmn.inl>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32cmn_private.h>
#endif

#endif //__E32CMN_H__

