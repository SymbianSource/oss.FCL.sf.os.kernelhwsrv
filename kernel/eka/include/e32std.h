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
// e32\include\e32std.h
// 
//

#ifndef __E32STD_H__
#define __E32STD_H__

#ifdef __KERNEL_MODE__
#error !! Including e32std.h in kernel code !!
#endif

#include <e32cmn.h>

/**
@publishedAll
@released
*/
class TFunctor
	{
public:
	IMPORT_C virtual void operator()() =0;
	};

/**
@publishedAll
@released

Encapsulates a general call-back function.

The class encapsulates:

1. a pointer to a function which takes an argument of type TAny* and returns 
   a TInt.

2. a pointer which is passed to the function every time it is called.
   The pointer can point to any object. It can also be NULL.

The callback function can be a static function of a class,
e.g. TInt X::Foo(TAny *) or it can be a function which is not a member of
any class, e.g. TInt Foo(TAny *).

When used with the CIdle and the CPeriodic classes, the callback function
is intended to be called repeatedly; the encapsulated pointer is passed on
each call. Typically, the pointer refers to an object which records the state
of the task across each call. When used with CIdle, the callback function
should also return a true (non-zero) value if it is intended to be called
again, otherwise it should return a false (zero) value.

@see CIdle
@see CPeriodic
*/
class TCallBack
	{
public:
	inline TCallBack();
	inline TCallBack(TInt (*aFunction)(TAny* aPtr));
	inline TCallBack(TInt (*aFunction)(TAny* aPtr),TAny* aPtr);
	inline TInt CallBack() const;
public:
	
	/**
	A pointer to the callback function.
	*/
	TInt (*iFunction)(TAny* aPtr);
	
	
	/**
	A pointer that is passed to the callback function when
	the function is called.
	*/
	TAny* iPtr;
	};




/**
@publishedAll
@released

An object embedded within a class T so that objects of type T can form part 
of a singly linked list.

A link object encapsulates a pointer to the next link object in the list.

@see TSglQue
*/
class TSglQueLink
	{
#if defined _DEBUG
public:
	inline TSglQueLink() : iNext(NULL)
	/**
	An explicitly coded default constructor that is only defined for DEBUG builds.
	
	It sets the pointer to the next link object to NULL.
	
	@see iNext
	*/
	{}
#endif
private:
	IMPORT_C void Enque(TSglQueLink* aLink);
public:
	/**
	A pointer to the next link object in the list.
	*/
	TSglQueLink* iNext;
	friend class TSglQueBase;
	};




/**
@publishedAll
@released

A base class that provides implementation for the link object of a doubly
linked list.

It also encapsulates pointers both to the next and the previous link 
objects in the doubly linked list.

The class is abstract and is not intended to be instantiated.

@see TDblQueLink
*/
class TDblQueLinkBase
	{
public:
	inline TDblQueLinkBase() : iNext(NULL)
	/**
	Default constructor.
	
	It sets the pointer to the next link object to NULL.
	
	@see iNext
	*/
	{}
	IMPORT_C void Enque(TDblQueLinkBase* aLink);
	IMPORT_C void AddBefore(TDblQueLinkBase* aLink);
public:
	/**
	A pointer to the next link object in the list.
	*/
	TDblQueLinkBase* iNext;
	
	/**
	A pointer to the previous link object in the list.
	*/
	TDblQueLinkBase* iPrev;
	};




/**
@publishedAll
@released

An object embedded within a class T so that objects of type T can form part 
of a doubly linked list.
*/
class TDblQueLink : public TDblQueLinkBase
	{
public:
	IMPORT_C void Deque();
	};




/**
@publishedAll
@released

An object embedded within a class T so that objects of type T can form part
of an ordered doubly linked list.

Objects are added to the doubly linked list in descending priority order.
*/
class TPriQueLink : public TDblQueLink
	{
public:
	/**
	The priority value.

    Objects are added to the doubly linked list in descending order of this value.
	*/
	TInt iPriority;
	};




/**
@publishedAll
@released

An object embedded within a class T so that objects of type T can form part 
of a delta doubly linked list.
*/
class TDeltaQueLink : public TDblQueLinkBase
	{
public:
	/**
	The delta value.
	*/
	TInt iDelta;
	};




/**
@publishedAll
@released

An object embedded within a class T so that objects of type T can form part 
of a doubly linked list sorted by tick count.
*/
class TTickCountQueLink : public TDblQueLink
	{
public:
	/**
	The tick count.
	*/
	TUint iTickCount;
	};




/**
@publishedAll
@released

A base class that provides implementation for the singly linked list header. 

It also encapsulates the offset value of a link object.

The class is abstract and is not intended to be instantiated.

@see TSglQue
*/
class TSglQueBase
	{
public:
	IMPORT_C TBool IsEmpty() const;
	IMPORT_C void SetOffset(TInt aOffset);
	IMPORT_C void Reset();
protected:
	IMPORT_C TSglQueBase();
	IMPORT_C TSglQueBase(TInt aOffset);
	IMPORT_C void DoAddFirst(TAny* aPtr);
	IMPORT_C void DoAddLast(TAny* aPtr);
	IMPORT_C void DoRemove(TAny* aPtr);
protected:
	/**
	A pointer to the first element in the list.
	*/
	TSglQueLink* iHead;
	
	/**
	A pointer to the last element in the list.
	*/
	TSglQueLink* iLast;
	
	/**
	The offset of a component link object within elements that form the list.
	*/
	TInt iOffset;
private:
	TSglQueBase(const TSglQueBase& aQue);
	TSglQueBase &operator=(const TSglQueBase& aQue);
	friend class TSglQueIterBase;
	};




/**
@publishedAll
@released

A base class that provides implementation for the doubly linked list header. 

It also encapsulates the offset value of a link object.

The class is abstract and is not intended to be instantiated.

@see TDblQue
*/
class TDblQueBase
	{
public:
	IMPORT_C TBool IsEmpty() const;
	IMPORT_C void SetOffset(TInt aOffset);
	IMPORT_C void Reset();
protected:
	IMPORT_C TDblQueBase();
	IMPORT_C TDblQueBase(TInt aOffset);
	IMPORT_C void DoAddFirst(TAny* aPtr);
	IMPORT_C void DoAddLast(TAny* aPtr);
	IMPORT_C void DoAddPriority(TAny* aPtr);
	IMPORT_C void __DbgTestEmpty() const;
protected:
	/**
	The head, or anchor point of the queue.
	*/
	TDblQueLink iHead;
	
	/**
	The offset of a component link object within elements that form the list.
	*/
	TInt iOffset;
private:
	TDblQueBase(const TDblQueBase& aQue);
	TDblQueBase& operator=(const TDblQueBase& aQue);
	friend class TDblQueIterBase;
	};




/**
@publishedAll
@released

A base class that provides implementation for the TDeltaQue template class.

The class is abstract and is not intended to be instantiated.

@see TDeltaQue
*/
class TDeltaQueBase : public TDblQueBase
	{
public:
	IMPORT_C TBool CountDown();
	IMPORT_C TBool CountDown(TInt aValue);
	IMPORT_C TBool FirstDelta(TInt& aValue);
	IMPORT_C void Reset();
protected:
	IMPORT_C TDeltaQueBase();
	IMPORT_C TDeltaQueBase(TInt aOffset);
	IMPORT_C void DoAddDelta(TAny* aPtr,TInt aDelta);
	IMPORT_C void DoRemove(TAny* aPtr);
	IMPORT_C TAny* DoRemoveFirst();
protected:
	/**
	Pointer to the delta value in the first link element.
	*/
	TInt* iFirstDelta;
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for managing a singly linked 
list.

It also acts as the head of the list, maintaining the pointers into the list.

The template parameter defines the type of element that forms the singly linked 
list and is the class that acts as host to the link object.

@see TSglQueLink
*/
template <class T>
class TSglQue : public TSglQueBase
	{
public:
	inline TSglQue();
	inline explicit TSglQue(TInt aOffset);
	inline void AddFirst(T& aRef);
	inline void AddLast(T& aRef);
	inline TBool IsFirst(const T* aPtr) const;
	inline TBool IsLast(const T* aPtr) const;
	inline T* First() const;
	inline T* Last() const;
	inline void Remove(T& aRef);
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for managing a doubly linked 
list. 

It also acts as the head of the list, maintaining the pointers into the list.

The template parameter defines the type of element that forms the doubly linked 
list and is the class that acts as host to the link object.

@see TDblQueLink
*/
template <class T>
class TDblQue : public TDblQueBase
	{
public:
	inline TDblQue();
	inline explicit TDblQue(TInt aOffset);
	inline void AddFirst(T& aRef);
	inline void AddLast(T& aRef);
	inline TBool IsHead(const T* aPtr) const;
	inline TBool IsFirst(const T* aPtr) const;
	inline TBool IsLast(const T* aPtr) const;
	inline T* First() const;
	inline T* Last() const;
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for managing a doubly linked
list in which the elements are added in descending priority order.

Priority is defined by the value of the TPriQueLink::iPriority member of
the link element.

The template parameter defines the type of element that forms the doubly linked
list and is the class that acts as host to the link object.

@see TPriQueLink
@see TPriQueLink::iPriority
*/
template <class T>
class TPriQue : public TDblQueBase
	{
public:
	inline TPriQue();
	inline explicit TPriQue(TInt aOffset);
	inline void Add(T& aRef);
	inline TBool IsHead(const T* aPtr) const;
	inline TBool IsFirst(const T* aPtr) const;
	inline TBool IsLast(const T* aPtr) const;
	inline T* First() const;
	inline T* Last() const;
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for managing a doubly linked 
list in which elements represent values which are increments, or deltas, on 
the value represented by a preceding element.

The list is ordered so that the head of the queue represents a nominal zero 
point.

The delta value of a new element represents its 'distance' from the nominal 
zero point. The new element is added into the list, and the delta values of 
adjacent elements (and of the new element, if necessary) are adjusted, so 
that the sum of all deltas, up to and including the new element, is the same 
as the new element's intended 'distance' from the nominal zero point.

A common use for a list of this type is as a queue of timed events, where 
the delta values represent the intervals between the events.

The delta value is defined by the value of the TDeltaQueLink::iDelta member 
of the link element.

The template parameter defines the type of element that forms the doubly linked 
list and is the class that acts as host to the link object.

@see TDeltaQueLink
@see TDeltaQueLink::iDelta
*/
template <class T>
class TDeltaQue : public TDeltaQueBase
	{
public:
	inline TDeltaQue();
	inline explicit TDeltaQue(TInt aOffset);
	inline void Add(T& aRef,TInt aDelta);
	inline void Remove(T& aRef);
	inline T* RemoveFirst();
	};




// Forward declaration
class TTickCountQueLink;

/**
@internalComponent
@released

A class that provides the behaviour for managing a doubly linked list
in which elements are added in order of the time until their tick count.

A common use for a list of this type is as a queue of timed events, where 
the tick counts are the expiry times of the events.

The tick count is defined by the value of the TTickCountQueLink::iTickCount
member of the link element.

@see TTickCountQueLink
@see TTickCountQueLink::iTickCount
*/
class TTickCountQue : public TDblQueBase
	{
public:
	TTickCountQue();
	void Add(TTickCountQueLink& aRef);
	TTickCountQueLink* First() const;
	TTickCountQueLink* RemoveFirst();
	TTickCountQueLink* RemoveFirst(TUint aTickCount);
	};




/**
@publishedAll
@released

A base class that provides implementation for the singly linked list iterator. 

It also encapsulates a pointer to the current link link list element.

The class is abstract and is not intended to be instantiated.
*/
class TSglQueIterBase
	{
public:
	IMPORT_C void SetToFirst();
protected:
	IMPORT_C TSglQueIterBase(TSglQueBase& aQue);
	IMPORT_C TAny* DoPostInc();
	IMPORT_C TAny* DoCurrent();
	IMPORT_C void DoSet(TAny* aLink);
protected:
	TInt iOffset;
	TSglQueLink* iHead;
	TSglQueLink* iNext;
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for iterating through a set of 
singly linked list elements.

The template parameter defines the type of element that forms the singly linked 
list. The class defined in the template parameter contains the link object.
*/
template <class T>
class TSglQueIter : public TSglQueIterBase
	{
public:
	inline TSglQueIter(TSglQueBase& aQue);
	inline void Set(T& aLink);
	inline operator T*();
	inline T* operator++(TInt);
	};




/**
@publishedAll
@released

A base class that provides implementation for the doubly linked list iterator.

It also encapsulates a pointer to the current link list element.

The class is abstract and is not intended to be instantiated.
*/
class TDblQueIterBase
	{
public:
	IMPORT_C void SetToFirst();
	IMPORT_C void SetToLast();
protected:
	IMPORT_C TDblQueIterBase(TDblQueBase& aQue);
	IMPORT_C TAny* DoPostInc();
	IMPORT_C TAny* DoPostDec();
	IMPORT_C TAny* DoCurrent();
	IMPORT_C void DoSet(TAny* aLink);
protected:
	/**
	The offset of a component link object within elements that form the list.
	*/
	TInt iOffset;
	
	/**
	Pointer to the anchor for the list.
	*/
	TDblQueLinkBase* iHead;
	
	/**
	Pointer to the current element.
	*/
	TDblQueLinkBase* iNext;
	};




/**
@publishedAll
@released

A templated class that provides the behaviour for iterating through a set of 
doubly linked list elements.

The template parameter defines the type of element that forms the doubly linked 
list. The class defined in the template parameter contains the link object.
*/
template <class T>
class TDblQueIter : public TDblQueIterBase
	{
public:
	inline TDblQueIter(TDblQueBase& aQue);
	inline void Set(T& aLink);
	inline operator T*();
	inline T* operator++(TInt);
	inline T* operator--(TInt);
	};




/**
@publishedAll
@released

Governs the type of comparison to be made between descriptor keys or between 
text keys.

@see TKeyArrayFix
@see TKeyArrayVar
@see TKeyArrayPak
*/
enum TKeyCmpText
	{
	/**
	For a Unicode build, this is the same as ECmpNormal16.
	For a non-Unicode build, this is the same as ECmpNormal8.
	
	Using the build independent names (i.e. TPtrC, TPtr, TBufC, TBuf or TText) 
	allows the compiler to chose the correct variant according to the build.
	*/
	ECmpNormal,
	
	
	/**
	For descriptor keys, the key is assumed to be the 8 bit variant, derived
	from TDesc8. A simple comparison is done between the content of the
	descriptors; the data is not folded and collation rules are not applied for
	the purpose of the comparison.
	
	For text keys, the key is assumed to be the 8 bit variant, of type TText8. 
	A normal comparison is done between the text data; the data is not folded 
	and collation rules are not applied for the purpose of the comparison.
	*/
	ECmpNormal8,
	
	
	/**
	For descriptor keys, the key is assumed to be the 16 bit variant, derived
	from TDesc16. A simple comparison is done between the content of the
	descriptors; the data is not folded and collation rules are not applied for
	the purpose of the comparison.
	
	For text keys, the key is assumed to be the 16 bit variant, of type
	TText16. A normal comparison is done between the text data; the data is
	not folded 	and collation rules are not applied for the purpose of the
	comparison.
	*/
	ECmpNormal16,
	
	
	/**
	For a Unicode build, this is the same as EcmpFolded16.
    For a non-Unicode build, this is the same as EcmpFolded8.

    Using the build independent names (i.e. TPtrC, TPtr, TBufC, TBuf or TText)
    allows the compiler to chose the correct variant according to the build. 
	*/
	ECmpFolded,
	
	
	/**
	For descriptor keys, the key is assumed to be the 8 bit variant,
	derived from TDesc8. The descriptor contents are folded for the purpose
	of the comparison.

    For text keys, the key is assumed to be the 8 bit variant, of type
    TText8. The text data is folded for the purpose of the comparison.
	*/
	ECmpFolded8,
	
	
	/**
	For descriptor keys, the key is assumed to be the 16 bit variant,
	derived from TDesc16. The descriptor contents are folded for the purpose
	of the comparison.

    For text keys, the key is assumed to be the 16 bit variant, of type
    TText16. The text data is folded for the purpose of the comparison.
	*/
	ECmpFolded16,
	
	
	/**
	For a Unicode build, this is the same as EcmpCollated16.
    For a non-Unicode build, this is the same as EcmpCollated8.

    Using the build independent names (i.e. TPtrC, TPtr, TBufC, TBuf or TText)
    allows the compiler to chose the correct variant according to the build.
	*/
	ECmpCollated,
	
	
	/**
	For descriptor keys, the key is assumed to be the 8 bit variant,
	derived from TDesc8. Collation rules are applied for the purpose of
	the comparison.

    For text keys, the key is assumed to be the 8 bit variant, of type 
    TText8. Collation rules are applied for the purpose of the comparison.
	*/
	ECmpCollated8,
	
	
	/**
	For descriptor keys, the key is assumed to be the 16 bit variant,
	derived from TDesc16. Collation rules are applied for the purpose of
	the comparison.

    For text keys, the key is assumed to be the 16 bit variant,
    of type TText16. Collation rules are applied for the purpose of
    the comparison.
	*/
	ECmpCollated16
	};




/**
@publishedAll
@released

Governs the type of comparison to be made between numeric keys.

@see TKeyArrayFix
@see TKeyArrayVar
@see TKeyArrayPak
*/
enum TKeyCmpNumeric
	{
	/**
	The key is assumed to be of type TInt8.
	*/
	ECmpTInt8=((ECmpCollated16+1)<<1),
	
	
	/**
	The key is assumed to be of type TInt16.
	*/
	ECmpTInt16,
	
	
	/**
	The key is assumed to be of type TInt32.
	*/
	ECmpTInt32,
	
	
	/**
	The key is assumed to be of type TInt.
	*/
	ECmpTInt,
	
	
	/**
	The key is assumed to be of type TUint8.
	*/
	ECmpTUint8,
	
	
	/**
	The key is assumed to be of type TUint16.
	*/
	ECmpTUint16,
	
	
	/**
	The key is assumed to be of type TUint32.
	*/
	ECmpTUint32,
	
	
	/**
	The key is assumed to be of type TUint.
	*/
	ECmpTUint,
	
	
	/**
	The key is assumed to be of type TInt64.
	*/
	ECmpTInt64
	};




/**
@publishedAll
@released

Defines the characteristics of a key used to access the elements of an array.

The class is abstract and cannot be instantiated. A derived class must be 
defined and implemented.

The classes TKeyArrayFix, TKeyArrayVar and TKeyArrayPak, derived from TKey, 
are already supplied to implement keys for the fixed length element, variable 
length element and packed arrays.

A derived class would normally be written to define the characteristics of 
a key for a non standard array.

@see TKeyArrayFix
@see TKeyArrayVar
@see TKeyArrayPak
*/
class TKey
	{
public:
	inline void SetPtr(const TAny* aPtr);
	IMPORT_C virtual TInt Compare(TInt aLeft,TInt aRight) const;
	IMPORT_C virtual TAny* At(TInt anIndex) const;
protected:
	IMPORT_C TKey();
	IMPORT_C TKey(TInt aOffset,TKeyCmpText aType);
	IMPORT_C TKey(TInt aOffset,TKeyCmpText aType,TInt aLength);
	IMPORT_C TKey(TInt aOffset,TKeyCmpNumeric aType);
protected:
	TInt iKeyOffset;
	TInt iKeyLength;
	TInt iCmpType;
	const TAny* iPtr;
	};

/**
@publishedAll
@released

Defines the basic behaviour for swapping two elements of an array.

The class is abstract. A derived class must be defined and implemented to 
use the functionality.

A derived class can define how to swap two elements of an array. In practice, 
this means providing an implementation for the virtual function Swap().

To support this, the derived class is also likely to need a pointer to the 
array itself and suitable constructors and/or other member functions to set 
such a pointer.
*/
class TSwap
	{
public:
	IMPORT_C TSwap();
	IMPORT_C virtual void Swap(TInt aLeft,TInt aRight) const;
	};




/**
@publishedAll
@released

Folds a specified character and provides functions to fold additional
characters after construction of the object.

Folding converts the character to a form which can be used in tolerant
comparisons without control over the operations performed. Tolerant comparisons
are those which ignore character differences like case and accents. 

Note that folding is locale-independent behaviour. It is also important to 
note that there can be no guarantee that folding is in any way culturally 
appropriate, and should not be used for matching characters in
natural language.

@see User::Fold
*/
class TCharF : public TChar
	{
public:
	inline TCharF(TUint aChar);
	inline TCharF(const TChar& aChar);
	inline TCharF& operator=(TUint aChar);
	inline TCharF& operator=(const TChar& aChar);
	};




/**
@publishedAll
@released

Converts a specified character to lower case and provides functions to convert 
additional characters after construction of the object.
*/
class TCharLC : public TChar
	{
public:
	inline TCharLC(TUint aChar);
	inline TCharLC(const TChar& aChar);
	inline TCharLC& operator=(TUint aChar);
	inline TCharLC& operator=(const TChar& aChar);
	};




/**
@publishedAll
@released

Converts a specified character to upper case and provides functions to convert 
additional characters after construction of the object.
*/
class TCharUC : public TChar
	{
public:
	inline TCharUC(TUint aChar);
	inline TCharUC(const TChar& aChar);
	inline TCharUC& operator=(TUint aChar);
	inline TCharUC& operator=(const TChar& aChar);
	};



/**
@publishedAll
@released

Defines the character representation of a real number type such
as a TReal or a TRealX.

An object of this type is used by functions that convert real values to
character format, for example, the descriptor functions:
Num(), AppendNum() and Format().

There are three constructors for constructing a suitable object.
The data members of the class, however, are public and can be
explicitly set after construction.
*/
class TRealFormat
	{
public:
	IMPORT_C TRealFormat();
	IMPORT_C TRealFormat(TInt aWidth);
	IMPORT_C TRealFormat(TInt aWidth,TInt aDecimalPlaces);
public:
    /**
    Governs the format of the character representation of the real number.

    This is set to one of the defined format types.

    One or more of the defined format flags can subsequently be ORed into this member.
    
    @see KRealFormatFixed
    @see KRealFormatExponent
    @see KRealFormatGeneral
    @see KRealFormatNoExponent
    @see KRealFormatCalculator
    @see KExtraSpaceForSign
    @see KAllowThreeDigitExp
    @see KDoNotUseTriads
    @see KGeneralLimit
    @see KUseSigFigs
    */
	TInt iType;
	
	
	/**
	Defines the maximum number of characters required to represent the number.
	*/
	TInt iWidth;
	
	
	/**
	Defines either the number of characters to be used to represent the decimal
	portion of the number, or the maximum number of significant digits in
	the character representation of the number.

    The interpretation depends on the chosen format as defined by iType.
    
	@see TRealFormat::iType
	*/
	TInt iPlaces;
	
	
	/**
	Defines the character to be used to separate the integer portion of
	a number representation from its decimal portion.

    In general, the character used for this purpose is a matter of local
    convention. The TLocale::DecimalSeparator() function can supply the
    desired character.

    @see TLocale
	*/
	TChar iPoint;
	
	
	/**
	Defines the character to be used to delimit groups of three digits in
	the integer part of the number.

    In general, the character used for this purpose is a matter of local
    convention. The TLocale::ThousandsSeparator() function can supply the
    desired character.

    @see TLocale
	*/
	TChar iTriad;
	
	
	/**
	Defines the threshold number of digits above which triad separation is to
	occur. A value of zero disables triad separation and no triad separation
	character (i.e. the character held in iTriad) is inserted into the
	resulting character representation regardless of the number of characters.

    For example, a value of 1 causes the number 1000 to be represented by the
    characters "1,000" whereas a value of 4 causes the same number to be
    represented by the characters "1000" (This assumes the ‘,’ triad separation
    character).

    Note that no triad separation occurs if the flag KDoNotUseTriads is set in
    the iType data member.

	@see TRealFormat::iTriad
	@see KDoNotUseTriads
	*/
	TInt iTriLen;
	};




/**
@publishedAll
@released

Defines the extraction mark used by the TLex8 class to indicate the current 
lexical element being analysed.

In practice, objects of this type are accessed through the TLexMark typedef.

@see TLexMark
@see TLex8
*/
class TLexMark8
	{
public:
	inline TLexMark8();
private:
	inline TLexMark8(const TUint8* aString);
	const TUint8* iPtr;
	friend class TLex8;
	__DECLARE_TEST;
	};




class TRealX;
/**
@publishedAll
@released

Provides general string-parsing functions suitable for numeric format
conversions and syntactical-element parsing.

The class is the 8-bit variant for non-Unicode strings and 8-bit wide
characters.

An instance of this class stores a string, maintaining an extraction mark 
to indicate the current lexical element being analysed and a pointer to the 
next character to be examined.

Objects of this type are normally accessed through the build independent type 
TLex.

@see TLex
*/
class TLex8
	{
public:
	IMPORT_C TLex8();
	inline TLex8(const TUint8* aString);
	inline TLex8(const TDesC8& aDes);
	inline TLex8& operator=(const TUint8* aString);
	inline TLex8& operator=(const TDesC8& aDes);
	inline TBool Eos() const;
	inline void Mark(TLexMark8& aMark) const;
	inline void Mark();
	IMPORT_C void Inc();
	IMPORT_C void Inc(TInt aNumber);
	IMPORT_C TChar Get();
	IMPORT_C TChar Peek() const;
	IMPORT_C void UnGet();
	inline void UnGetToMark();
	IMPORT_C void UnGetToMark(const TLexMark8 aMark);
	IMPORT_C void SkipSpace();
	inline void SkipAndMark(TInt aNumber);
	IMPORT_C void SkipAndMark(TInt aNumber, TLexMark8& aMark);
	inline void SkipSpaceAndMark();
	IMPORT_C void SkipSpaceAndMark(TLexMark8& aMark);
	IMPORT_C void SkipCharacters();
	inline TInt TokenLength() const;
	IMPORT_C TInt TokenLength(const TLexMark8 aMark) const;
	IMPORT_C TPtrC8 MarkedToken() const;
	IMPORT_C TPtrC8 MarkedToken(const TLexMark8 aMark) const;
	IMPORT_C TPtrC8 NextToken();
	IMPORT_C TPtrC8 Remainder() const;
	IMPORT_C TPtrC8 RemainderFromMark() const;
	IMPORT_C TPtrC8 RemainderFromMark(const TLexMark8 aMark) const;
	IMPORT_C TInt Offset() const;
	inline TInt MarkedOffset() const;
	IMPORT_C TInt MarkedOffset(const TLexMark8 aMark) const;
	IMPORT_C TInt Val(TInt8& aVal);
	IMPORT_C TInt Val(TInt16& aVal);
	IMPORT_C TInt Val(TInt32& aVal);
	IMPORT_C TInt Val(TInt64& aVal);
	inline TInt Val(TInt& aVal);
	IMPORT_C TInt Val(TUint8& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TUint16& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TUint32& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TInt64& aVal, TRadix aRadix);
	inline TInt Val(TUint& aVal,TRadix aRadix=EDecimal);
	IMPORT_C TInt BoundedVal(TInt32& aVal,TInt aLimit);
	IMPORT_C TInt BoundedVal(TInt64& aVal, const TInt64& aLimit);
	IMPORT_C TInt BoundedVal(TUint32& aVal,TRadix aRadix,TUint aLimit);
	IMPORT_C TInt BoundedVal(TInt64& aVal, TRadix aRadix, const TInt64& aLimit);
	IMPORT_C TInt Val(TReal32& aVal);
	IMPORT_C TInt Val(TReal32& aVal,TChar aPoint);
	IMPORT_C TInt Val(TReal64& aVal);
	IMPORT_C TInt Val(TReal64& aVal,TChar aPoint);
	inline void Assign(const TLex8& aLex);
	IMPORT_C void Assign(const TUint8* aString);
	IMPORT_C void Assign(const TDesC8& aDes);
	TInt Val(TRealX& aVal);
	TInt Val(TRealX& aVal, TChar aPoint);

	/** @deprecated Use BoundedVal(TInt32& aVal,TInt aLimit) */
	inline TInt Val(TInt32& aVal,TInt aLimit) { return BoundedVal(aVal,aLimit); };

	/** @deprecated Use BoundedVal(TInt64& aVal,const TInt64& aLimit) */
	inline TInt Val(TInt64& aVal,const TInt64& aLimit) { return BoundedVal(aVal,aLimit); };

	/** @deprecated Use BoundedVal(TUint32& aVal,TRadix aRadix,TUint aLimit) */
	inline TInt Val(TUint32& aVal,TRadix aRadix,TUint aLimit) { return BoundedVal(aVal,aRadix,aLimit); };

	/** @deprecated Use BoundedVal(TInt64& aVal,TRadix aRadix,const TInt64& aLimit) */
	inline TInt Val(TInt64& aVal,TRadix aRadix,const TInt64& aLimit) { return BoundedVal(aVal,aRadix,aLimit); };
private:
	void Scndig(TInt& aSig, TInt& aExp, TUint64& aDl);
	void ScndigAfterPoint(TInt& aSig, TUint64& aDl);
	void ValidateMark(const TLexMark8 aMark) const;
private:
	const TUint8* iNext;
	const TUint8* iBuf;
	const TUint8* iEnd;
	TLexMark8 iMark;
	__DECLARE_TEST;
	};




/**
@publishedAll
@released

Defines the extraction mark used by the TLex16 class to indicate the current 
lexical element being analysed.

In practice, objects of this type are accessed through the TLexMark typedef.

@see TLexMark
@see TLex16
*/
class TLexMark16
	{
public:
	inline TLexMark16();
private:
	inline TLexMark16(const TUint16* aString);
	const TUint16* iPtr;
	friend class TLex16;	
	__DECLARE_TEST;
	};




/**
@publishedAll
@released

Provides general string-parsing functions suitable for numeric format
conversions and syntactical-element parsing. 

The class is the 16-bit variant for Unicode strings and 16-bit wide
characters.

An instance of this class stores a string, maintaining an extraction mark 
to indicate the current lexical element being analysed and a pointer to the 
next character to be examined.

Objects of this type are normally accessed through the build independent type 
TLex.

@see TLex
*/
class TLex16
	{
public:
	IMPORT_C TLex16();
	inline TLex16(const TUint16* aString);
	inline TLex16(const TDesC16& aDes);
	inline TLex16& operator=(const TUint16* aString);
	inline TLex16& operator=(const TDesC16& aDes);
	inline TBool Eos() const;
	inline void Mark();
	inline void Mark(TLexMark16& aMark) const;
	IMPORT_C void Inc();
	IMPORT_C void Inc(TInt aNumber);
	IMPORT_C TChar Get();
	IMPORT_C TChar Peek() const;
	IMPORT_C void UnGet();
	inline void UnGetToMark();
	IMPORT_C void UnGetToMark(const TLexMark16 aMark);
	IMPORT_C void SkipSpace();
	inline void SkipAndMark(TInt aNumber);
	IMPORT_C void SkipAndMark(TInt aNumber, TLexMark16& aMark);
	IMPORT_C void SkipSpaceAndMark(TLexMark16& aMark);
	inline void SkipSpaceAndMark();
	IMPORT_C void SkipCharacters();
	inline TInt TokenLength() const;
	IMPORT_C TInt TokenLength(const TLexMark16 aMark) const;
	IMPORT_C TPtrC16 MarkedToken() const;
	IMPORT_C TPtrC16 MarkedToken(const TLexMark16 aMark) const;
	IMPORT_C TPtrC16 NextToken();
	IMPORT_C TPtrC16 Remainder() const;
	IMPORT_C TPtrC16 RemainderFromMark() const;
	IMPORT_C TPtrC16 RemainderFromMark(const TLexMark16 aMark) const;
	IMPORT_C TInt Offset() const;
	inline TInt MarkedOffset() const;
	IMPORT_C TInt MarkedOffset(const TLexMark16 aMark) const;
	IMPORT_C TInt Val(TInt8& aVal);
	IMPORT_C TInt Val(TInt16& aVal);
	IMPORT_C TInt Val(TInt32& aVal);
	IMPORT_C TInt Val(TInt64& aVal);
	inline TInt Val(TInt& aVal);
	IMPORT_C TInt Val(TUint8& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TUint16& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TUint32& aVal,TRadix aRadix);
	IMPORT_C TInt Val(TInt64& aVal, TRadix aRadix);
//	inline TInt Val(TInt64& aVal, TRadix aRadix) {return Val(aVal,aRadix);}
	inline TInt Val(TUint& aVal,TRadix aRadix=EDecimal);
	IMPORT_C TInt BoundedVal(TInt32& aVal,TInt aLimit);
	IMPORT_C TInt BoundedVal(TInt64& aVal, const TInt64& aLimit);
	IMPORT_C TInt BoundedVal(TUint32& aVal,TRadix aRadix,TUint aLimit);
	IMPORT_C TInt BoundedVal(TInt64& aVal, TRadix aRadix, const TInt64& aLimit);
	IMPORT_C TInt Val(TReal32& aVal);
	IMPORT_C TInt Val(TReal32& aVal,TChar aPoint);
	IMPORT_C TInt Val(TReal64& aVal);
	IMPORT_C TInt Val(TReal64& aVal,TChar aPoint);
	inline void Assign(const TLex16& aLex);
	IMPORT_C void Assign(const TUint16* aString);
	IMPORT_C void Assign(const TDesC16& aDes);		
	TInt Val(TRealX& aVal);
	TInt Val(TRealX& aVal, TChar aPoint);

	/** @deprecated Use BoundedVal(TInt32& aVal,TInt aLimit) */
	inline TInt Val(TInt32& aVal,TInt aLimit) { return BoundedVal(aVal,aLimit); };

	/** @deprecated Use BoundedVal(TInt64& aVal,const TInt64& aLimit) */
	inline TInt Val(TInt64& aVal,const TInt64& aLimit) { return BoundedVal(aVal,aLimit); };

	/** @deprecated Use BoundedVal(TUint32& aVal,TRadix aRadix,TUint aLimit) */
	inline TInt Val(TUint32& aVal,TRadix aRadix,TUint aLimit) { return BoundedVal(aVal,aRadix,aLimit); };

	/** @deprecated Use BoundedVal(TInt64& aVal,TRadix aRadix,const TInt64& aLimit) */
	inline TInt Val(TInt64& aVal,TRadix aRadix,const TInt64& aLimit) { return BoundedVal(aVal,aRadix,aLimit); };
private:
	void Scndig(TInt& aSig, TInt& aExp, TUint64& aDl);
	void ValidateMark(const TLexMark16 aMark) const;
private:
	const TUint16* iNext;
	const TUint16* iBuf;
	const TUint16* iEnd;
	TLexMark16 iMark;
	__DECLARE_TEST;
	};




#if defined(_UNICODE)
/**
@publishedAll
@released

Provides access to general string-parsing functions suitable for numeric format 
conversions and syntactical-element parsing.

It maps directly to either a TLex16 for a Unicode build or a TLex8 for a non-Unicode 
build.

The build independent type should always be used unless an explicit 16 bit 
or 8 bit build variant is required.

@see TLex16
@see TLex8
*/
typedef TLex16 TLex;




/**
@publishedAll
@released

Defines the extraction mark used by the TLex classes to indicate the current 
lexical element being analysed. 

It maps directly to either a TLexMark16 for a Unicode build or a TLexMark8 
for a non-Unicode build.

The build independent type should always be used unless an explicit 16 bit 
or 8 bit build variant is required.
*/
typedef TLexMark16 TLexMark;




#else



/**
@publishedAll
@released

Provides access to general string-parsing functions suitable for numeric format 
conversions and syntactical-element parsing.

It maps directly to either a TLex16 for a Unicode build or a TLex8 for a non-Unicode 
build.

The build independent type should always be used unless an explicit 16 bit 
or 8 bit build variant is required.

@see TLex16
@see TLex8
*/
typedef TLex8 TLex;




/**
@publishedAll
@released

Defines the extraction mark used by the TLex classes to indicate the current 
lexical element being analysed. 

It maps directly to either a TLexMark16 for a Unicode build or a TLexMark8 
for a non-Unicode build.

The build independent type should always be used unless an explicit 16 bit 
or 8 bit build variant is required.
*/
typedef TLexMark8 TLexMark;
#endif




/**
@publishedAll
@released

Packages a Uid type together with a checksum.

@see TUidType
*/
class TCheckedUid
	{
public:
	IMPORT_C TCheckedUid();
	IMPORT_C TCheckedUid(const TUidType& aUidType);
	IMPORT_C TCheckedUid(const TDesC8& aPtr);
	IMPORT_C void Set(const TUidType& aUidType);
	IMPORT_C void Set(const TDesC8& aPtr);
	IMPORT_C TPtrC8 Des() const;
	inline const TUidType& UidType() const;
protected:
	IMPORT_C TUint Check() const;
private:
	TUidType iType;
	TUint iCheck;
	};




/**
@publishedAll
@released

A date and time object in which the individual components are accessible in
human-readable form.

The individual components are: year, month, day, hour, minute,
second and microsecond.

These components are stored as integers and all except the year are checked for
validity when a TDateTime is constructed or assigned new values.

This class only supports getting and setting the entire date/time or any component 
of it. It does not support adding or subtracting intervals to or from a time. 
For functions which manipulate times, use class TTime.

@see TTime
*/
class TDateTime
	{
public:
	inline TDateTime();
	IMPORT_C TDateTime(TInt aYear,TMonth aMonth,TInt aDay,TInt aHour,TInt aMinute, TInt aSecond,TInt aMicroSecond);
	IMPORT_C TInt Set(TInt aYear,TMonth aMonth,TInt aDay,TInt aHour,TInt aMinute, TInt aSecond,TInt aMicroSecond);
	IMPORT_C TInt SetYear(TInt aYear);
	IMPORT_C TInt SetYearLeapCheck(TInt aYear);
	IMPORT_C TInt SetMonth(TMonth aMonth);
	IMPORT_C TInt SetDay(TInt aDay);
	IMPORT_C TInt SetHour(TInt aHour);
	IMPORT_C TInt SetMinute(TInt aMinute);
	IMPORT_C TInt SetSecond(TInt aSecond);
	IMPORT_C TInt SetMicroSecond(TInt aMicroSecond);
	inline TInt Year() const;
	inline TMonth Month() const;
	inline TInt Day() const;
	inline TInt Hour() const;
	inline TInt Minute() const;
	inline TInt Second() const;
	inline TInt MicroSecond() const;
private:
	TInt iYear;
	TMonth iMonth;
	TInt iDay;
	TInt iHour;
	TInt iMinute;
	TInt iSecond;
	TInt iMicroSecond;
	};




/**
@publishedAll
@released

Represents a time interval of a millionth of a second stored as
a 64-bit integer. 

It supports the initialisation, setting and getting of an interval and provides
standard comparison operations. Objects of this class can be added to and
subtracted from TTime objects.

@see TTime
*/
class TTimeIntervalMicroSeconds
	{
public:
	inline TTimeIntervalMicroSeconds();
	inline TTimeIntervalMicroSeconds(const TInt64& aInterval);
	inline TTimeIntervalMicroSeconds& operator=(const TInt64& aInterval);
	inline TBool operator==(const TTimeIntervalMicroSeconds& aInterval) const;
	inline TBool operator!=(const TTimeIntervalMicroSeconds& aInterval) const;
	inline TBool operator>=(const TTimeIntervalMicroSeconds& aInterval) const;
	inline TBool operator<=(const TTimeIntervalMicroSeconds& aInterval) const;
	inline TBool operator>(const TTimeIntervalMicroSeconds& aInterval) const;
	inline TBool operator<(const TTimeIntervalMicroSeconds& aInterval) const;
	inline const TInt64& Int64() const;
private:
	TInt64 iInterval;
	};




/**
@publishedAll
@released

Provides a base class for all time interval classes using
a 32-bit representation. 

It supports retrieving the interval and provides various operations for
comparing intervals. Its concrete derived classes can be added to and
subtracted from a TTime.

The comparison operators simply compare the integer representations of the 
two intervals. They do not take account of different time interval units. 
So, for example, when comparing for equality an interval of three hours with 
an interval of three days, the result is true.

@see TTime
*/
class TTimeIntervalBase
	{
public:
	inline TBool operator==(TTimeIntervalBase aInterval) const;
	inline TBool operator!=(TTimeIntervalBase aInterval) const;
	inline TBool operator>=(TTimeIntervalBase aInterval) const;
	inline TBool operator<=(TTimeIntervalBase aInterval) const;
	inline TBool operator>(TTimeIntervalBase aInterval) const;
	inline TBool operator<(TTimeIntervalBase aInterval) const;
	inline TInt Int() const;
protected:
	inline TTimeIntervalBase();
	inline TTimeIntervalBase(TInt aInterval);
protected:
	TInt iInterval;
	};




/**
@publishedAll
@released

Represents a microsecond time interval stored in 32 rather than 64 bits.

Its range is +-2147483647, which is +-35 minutes, 47 seconds. Comparison and 
interval retrieval functions are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalMicroSeconds32 : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalMicroSeconds32();
	inline TTimeIntervalMicroSeconds32(TInt aInterval);
	inline TTimeIntervalMicroSeconds32& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in seconds.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.

The range of values which it can represent is +-2147483647, which is equal to
+-24855 days (approximately 68 years).
*/
class TTimeIntervalSeconds : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalSeconds();
	inline TTimeIntervalSeconds(TInt aInterval);
	inline TTimeIntervalSeconds& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in minutes.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalMinutes : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalMinutes();
	inline TTimeIntervalMinutes(TInt aInterval);
	inline TTimeIntervalMinutes& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in hours.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalHours : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalHours();
	inline TTimeIntervalHours(TInt aInterval);
	inline TTimeIntervalHours& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in days.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalDays : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalDays();
	inline TTimeIntervalDays(TInt aInterval);
	inline TTimeIntervalDays& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in months.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalMonths : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalMonths();
	inline TTimeIntervalMonths(TInt aInterval);
	inline TTimeIntervalMonths& operator=(TInt aInterval);
	};




/**
@publishedAll
@released

Represents a time interval in years.

Comparison and interval retrieval functions 
are provided by the base class TTimeIntervalBase.
*/
class TTimeIntervalYears : public TTimeIntervalBase
	{
public:
	inline TTimeIntervalYears();
	inline TTimeIntervalYears(TInt aInterval);
	inline TTimeIntervalYears& operator=(TInt aInterval);
	};
	
	
	
/**
@publishedAll
@released

An enumeration one or both of whose enumerator values may be returned
by TTime::Parse().

@see TTime::Parse
*/
enum {
     /**
     Indicates that a time is present.
     
     @see TTime::Parse
     */
     EParseTimePresent=0x1,
     /**
     Indicates that a date is present.
     
     @see TTime::Parse
     */
     EParseDatePresent=0x2
     };



class TLocale;
/**
@publishedAll
@released

Stores and manipulates the date and time. 

It represents a date and time as a number of microseconds since midnight, 
January 1st, 1 AD nominal Gregorian. BC dates are represented by negative 
TTime values. A TTime object may be constructed from a TInt64, a TDateTime 
a string literal, or by default, which initialises the time to an arbitrary 
value. To access human-readable time information, the TTime may be converted 
from a TInt64 into a TDateTime, which represents the date and time as seven 
numeric fields and provides functions to extract these fields. Alternatively, 
to display the time as text, the time may be formatted and placed into a
descriptor using a variety of formatting commands and which may or may not
honour the system's locale settings. The conversion between time and text may
be performed the other way around, so that a descriptor can be parsed and
converted into a TTime value.

In addition to setting and getting the date and time and converting between 
text and time, TTime provides functions to get intervals between times and 
standard comparison and arithmetic operators which enable time intervals to 
be added or subtracted to or from the time.

@see TInt64
@see TDateTime
*/
class TTime
	{
public:
	inline TTime();
	inline TTime(const TInt64& aTime);
	IMPORT_C TTime(const TDesC& aString);
	IMPORT_C TTime(const TDateTime& aDateTime);
	inline TTime& operator=(const TInt64& aTime);
	IMPORT_C TTime& operator=(const TDateTime& aDateTime);
	IMPORT_C void HomeTime();
	IMPORT_C void UniversalTime();
	IMPORT_C TInt Set(const TDesC& aString);
	IMPORT_C TInt HomeTimeSecure();
	IMPORT_C TInt UniversalTimeSecure();

	IMPORT_C TDateTime DateTime() const;
	IMPORT_C TTimeIntervalMicroSeconds MicroSecondsFrom(TTime aTime) const;
	IMPORT_C TInt SecondsFrom(TTime aTime,TTimeIntervalSeconds& aInterval) const;
	IMPORT_C TInt MinutesFrom(TTime aTime,TTimeIntervalMinutes& aInterval) const;
	IMPORT_C TInt HoursFrom(TTime aTime,TTimeIntervalHours& aInterval) const;
	IMPORT_C TTimeIntervalDays DaysFrom(TTime aTime) const;
	IMPORT_C TTimeIntervalMonths MonthsFrom(TTime aTime) const;
	IMPORT_C TTimeIntervalYears YearsFrom(TTime aTime) const;

	IMPORT_C TInt DaysInMonth() const;
	IMPORT_C TDay DayNoInWeek() const;
	IMPORT_C TInt DayNoInMonth() const;
	IMPORT_C TInt DayNoInYear() const;
	IMPORT_C TInt DayNoInYear(TTime aStartDate) const;
	IMPORT_C TInt WeekNoInYear() const;
	IMPORT_C TInt WeekNoInYear(TTime aStartDate) const;
	IMPORT_C TInt WeekNoInYear(TFirstWeekRule aRule) const;
	IMPORT_C TInt WeekNoInYear(TTime aStartDate,TFirstWeekRule aRule) const;
	IMPORT_C void FormatL(TDes& aDes,const TDesC& aFormat) const;
	IMPORT_C void FormatL(TDes& aDes,const TDesC& aFormat,const TLocale& aLocale) const;
	IMPORT_C void RoundUpToNextMinute();
	IMPORT_C TInt Parse(const TDesC& aDes,TInt aCenturyOffset=0);

	IMPORT_C TTime operator+(TTimeIntervalYears aYear) const;
	IMPORT_C TTime operator+(TTimeIntervalMonths aMonth) const;
	IMPORT_C TTime operator+(TTimeIntervalDays aDay) const;
	IMPORT_C TTime operator+(TTimeIntervalHours aHour) const;
	IMPORT_C TTime operator+(TTimeIntervalMinutes aMinute) const;
	IMPORT_C TTime operator+(TTimeIntervalSeconds aSecond) const;  	
	IMPORT_C TTime operator+(TTimeIntervalMicroSeconds aMicroSecond) const;
	IMPORT_C TTime operator+(TTimeIntervalMicroSeconds32 aMicroSecond) const;
	IMPORT_C TTime operator-(TTimeIntervalYears aYear) const;
	IMPORT_C TTime operator-(TTimeIntervalMonths aMonth) const;
	IMPORT_C TTime operator-(TTimeIntervalDays aDay) const;
	IMPORT_C TTime operator-(TTimeIntervalHours aHour) const;
	IMPORT_C TTime operator-(TTimeIntervalMinutes aMinute) const;
	IMPORT_C TTime operator-(TTimeIntervalSeconds aSecond) const;  	
	IMPORT_C TTime operator-(TTimeIntervalMicroSeconds aMicroSecond) const;
	IMPORT_C TTime operator-(TTimeIntervalMicroSeconds32 aMicroSecond) const;
	IMPORT_C TTime& operator+=(TTimeIntervalYears aYear);
	IMPORT_C TTime& operator+=(TTimeIntervalMonths aMonth);
	IMPORT_C TTime& operator+=(TTimeIntervalDays aDay);
	IMPORT_C TTime& operator+=(TTimeIntervalHours aHour);
	IMPORT_C TTime& operator+=(TTimeIntervalMinutes aMinute);
	IMPORT_C TTime& operator+=(TTimeIntervalSeconds aSecond);	
	IMPORT_C TTime& operator+=(TTimeIntervalMicroSeconds aMicroSecond);
	IMPORT_C TTime& operator+=(TTimeIntervalMicroSeconds32 aMicroSecond);
	IMPORT_C TTime& operator-=(TTimeIntervalYears aYear);
	IMPORT_C TTime& operator-=(TTimeIntervalMonths aMonth);
	IMPORT_C TTime& operator-=(TTimeIntervalDays aDay);
	IMPORT_C TTime& operator-=(TTimeIntervalHours aHour);
	IMPORT_C TTime& operator-=(TTimeIntervalMinutes aMinute);
	IMPORT_C TTime& operator-=(TTimeIntervalSeconds aSecond);	
	IMPORT_C TTime& operator-=(TTimeIntervalMicroSeconds aMicroSecond);
	IMPORT_C TTime& operator-=(TTimeIntervalMicroSeconds32 aMicroSecond);
	inline TBool operator==(TTime aTime) const;
	inline TBool operator!=(TTime aTime) const;
	inline TBool operator>=(TTime aTime) const;
	inline TBool operator<=(TTime aTime) const;
	inline TBool operator>(TTime aTime) const;
	inline TBool operator<(TTime aTime) const;
	inline const TInt64& Int64() const;
private:
	static TTime Convert(const TDateTime& aDateTime);
private:
	TInt64 iTime;
	__DECLARE_TEST;
	};




/**
@publishedAll
@released

A utility class whose functions may be used by the other date/time related 
classes.
*/
class Time
	{
public:
	IMPORT_C static TTime NullTTime();
	IMPORT_C static TTime MaxTTime();
	IMPORT_C static TTime MinTTime();
	IMPORT_C static TInt DaysInMonth(TInt aYear, TMonth aMonth);
	IMPORT_C static TBool IsLeapYear(TInt aYear);
	IMPORT_C static TInt LeapYearsUpTo(TInt aYear);
	};




/**
@publishedAll
@released

Gets a copy of the current locale's full text name for a day of the week.

After construction or after a call to Set(), the copy of the text can be accessed 
and manipulated using the standard descriptor member functions provided by 
the base class.

@see KMaxDayName
*/
class TDayName : public TBuf<KMaxDayName>
	{
public:
	IMPORT_C TDayName();
	IMPORT_C TDayName(TDay aDay);
	IMPORT_C void Set(TDay aDay);
	};




/**
@publishedAll
@released

Gets a copy of the current locale's abbreviated text name for a day of the 
week.

After construction or after a call to Set(), the copy of the abbreviated text 
can be accessed and manipulated using the standard descriptor member functions 
provided by the base class.

The abbreviated day name cannot be assumed to be one character. In English, 
it is 3 characters (Mon, Tue, Wed etc.), but the length can vary from locale 
to locale, with a maximum length of KMaxDayNameAbb.

@see KMaxDayNameAbb
*/
class TDayNameAbb : public TBuf<KMaxDayNameAbb>
	{
public:
	IMPORT_C TDayNameAbb();
	IMPORT_C TDayNameAbb(TDay aDay);
	IMPORT_C void Set(TDay aDay);
	};




/**
@publishedAll
@released

Gets a copy of the current locale's full text name for a month.

After construction or after a call to Set(), the copy of the text can be accessed 
and manipulated using the standard descriptor member functions provided by 
the base class.

@see KMaxMonthName
*/
class TMonthName : public TBuf<KMaxMonthName>
	{
public:
	IMPORT_C TMonthName();
	IMPORT_C TMonthName(TMonth aMonth);
	IMPORT_C void Set(TMonth aMonth);
	};




/**
@publishedAll
@released

Gets a copy of the current locale's abbreviated text name for a month.

After construction or after a call to Set(), the copy of the abbreviated text 
can be accessed and manipulated using the standard descriptor member functions 
provided by the base class.

@see KMaxMonthNameAbb
*/
class TMonthNameAbb : public TBuf<KMaxMonthNameAbb>
	{
public:
	IMPORT_C TMonthNameAbb();
	IMPORT_C TMonthNameAbb(TMonth aMonth);
	IMPORT_C void Set(TMonth aMonth);
	};




/**
@publishedAll
@released

Gets a copy of the current locale's date suffix text for a specific day in 
the month.

The text is the set of characters which can be appended to dates of the month 
(e.g. in English, st for 1st, nd for 2nd etc).

After construction or after a call to Set(), the copy of the suffix text can 
be accessed and manipulated using the standard descriptor member functions 
provided by the base class.
*/
class TDateSuffix : public TBuf<KMaxSuffix>
	{
public:
	IMPORT_C TDateSuffix();
	IMPORT_C TDateSuffix(TInt aDateSuffix);
	IMPORT_C void Set(TInt aDateSuffix);
	};




/**
@publishedAll
@released

Current locale's am/pm text

This class retrieves a copy of the current locale's text identifying time 
before and after noon. In English, this is am and pm.

After construction or after a call to Set(), the copy of the text can be accessed 
and manipulated using the standard descriptor member functions provided by 
the base class.
*/
class TAmPmName : public TBuf<KMaxAmPmName>
	{
public:
	IMPORT_C TAmPmName();
	IMPORT_C TAmPmName(TAmPm aSelector);
	IMPORT_C void Set(TAmPm aSelector);
	};




/**
@publishedAll
@released

Gets a copy of the currency symbol(s) in use by the current locale.

After construction or after a call to TCurrencySymbol::Set(), the copy of 
the currency symbol(s) can be accessed and manipulated using the standard 
descriptor member functions provided by the base class.
*/
class TCurrencySymbol : public TBuf<KMaxCurrencySymbol>
	{
public:
	IMPORT_C TCurrencySymbol();
	IMPORT_C void Set();
	};




/**
@publishedAll
@released

Contains a format list that defines the short date format.

An instance of this class should be passed as the second argument
to TTime::FormatL().
The string does not include any time components. The content of the long 
date format specification is taken from the system-wide settings.

For example, in the English locale, the short date format would be something
like 14/1/2000.

This class is used as follows:

@code
TTime now;
now.HomeTime();
TBuf<KMaxShortDateFormatSpec*2> buffer;
now.FormatL(buffer,TShortDateFormatSpec());
@endcode

@see KMaxShortDateFormatSpec
@see TTime::FormatL
*/
class TShortDateFormatSpec : public TBuf<KMaxShortDateFormatSpec> // to be passed into TTime::FormatL
	{
public:
	IMPORT_C TShortDateFormatSpec();
	IMPORT_C void Set();
	};




/**
@publishedAll
@released

Contains a format list that defines the long date format.

An instance of this class should be passed as the second argument
to TTime::FormatL(). 
The string does not include any time components. The content of the long 
date format specification is taken from the system-wide settings.

For example, in the English locale, the long date format would be
something like 14th January 2000.

This class is used as follows:

@code
TTime now;
now.HomeTime();
TBuf<KMaxLongDateFormatSpec*2> buffer;
now.FormatL(buffer,TLongDateFormatSpec());
@endcode

@see KMaxLongDateFormatSpec
@see TTime::FormatL
*/
class TLongDateFormatSpec : public TBuf<KMaxLongDateFormatSpec> // to be passed into TTime::FormatL
	{
public:
	IMPORT_C TLongDateFormatSpec();
	IMPORT_C void Set();
	};




/**
@publishedAll
@released

Contains a format list that defines the time string format. 

An instance of this class should be passed as the second argument
to TTime::FormatL().
The string does not include any time components. The content of the time format 
specification is taken from the system-wide settings.

This class is used as follows:

@code
TTime now;
now.HomeTime();
TBuf<KMaxTimeFormatSpec*2> buffer;
now.FormatL(buffer,TTimeFormatSpec());
@endcode

@see KMaxTimeFormatSpec
@see TTime::FormatL
*/
class TTimeFormatSpec : public TBuf<KMaxTimeFormatSpec> // to be passed into TTime::FormatL
	{
public:
	IMPORT_C TTimeFormatSpec();
	IMPORT_C void Set();
	};




/**
@publishedAll
@released

Sets and gets the system's locale settings.

Symbian OS maintains the locale information internally. On
construction, this object is initialized with the system information
for all locale items.
*/
class TLocale
	{
public:
		
    /**
    Indicates how negative currency values are formatted.
    */
	enum TNegativeCurrencyFormat
		{
	    /**
	    A minus sign is inserted before the currency symbol and value.
	    */
		ELeadingMinusSign,

		/**
		The currency value and symbol are enclosed in brackets (no minus sign
		is used).
		*/
		EInBrackets, //this one must be non-zero for binary compatibility with the old TBool TLocale::iCurrencyNegativeInBrackets which was exposed in the binary interface because it was accessed via *inline* functions
			
	    /**
	    A minus sign is inserted after the currency symbol and value.
        */
		ETrailingMinusSign,
		
        /**
        A minus sign is inserted between the currency symbol and the value.
        */
		EInterveningMinusSign
		};
		
	/**
	Flags for negative currency values formatting
	*/
	enum 
		{
		/** 
		If this flag is set and the currency value being formatted is negative,
		if there is a space between the currency symbol and the value,
		that space is lost. 
		*/
		EFlagNegativeLoseSpace = 0x00000001,
		
		/**   
		If this flag is set and the currency value being formatted is negative,
		the position of the currency symbol is placed in the opposite direction 
		from the position set for the positive currency value. 
		*/
		EFlagNegativeCurrencySymbolOpposite=0x00000002
		};
	/** Indicates how the device universal time is maintained */
	enum TDeviceTimeState
		{
		/** Universal time is maintained by the device RTC and the user selection 
		of the locale of the device indicating offset from GMT and daylight saving*/
		EDeviceUserTime,

		/** Universal time and offset from GMT is supplied by the mobile network
		and maintained by device RTC */
		ENITZNetworkTimeSync
		};
public:
	IMPORT_C TLocale();
	inline TLocale(TInt);
	IMPORT_C void Refresh();
	IMPORT_C TInt Set() const;
	IMPORT_C void FormatCurrency(TDes& aText, TInt aAmount);
	IMPORT_C void FormatCurrency(TDes& aText, TInt64 aAmount);
	IMPORT_C void FormatCurrency(TDes& aText, TDesOverflow& aOverflowHandler, TInt aAmount); 
	IMPORT_C void FormatCurrency(TDes& aText, TDesOverflow& aOverflowHandler, TInt64 aAmount); 
	
	inline TInt CountryCode() const;
	inline void SetCountryCode(TInt aCode);
	inline TTimeIntervalSeconds UniversalTimeOffset() const;
	inline TDateFormat DateFormat() const;
	inline void SetDateFormat(TDateFormat aFormat);
	inline TTimeFormat TimeFormat() const;
	inline void SetTimeFormat(TTimeFormat aFormat);
	inline TLocalePos CurrencySymbolPosition() const;
	inline void SetCurrencySymbolPosition(TLocalePos aPos);
	inline TBool CurrencySpaceBetween() const;
	inline void SetCurrencySpaceBetween(TBool aSpace);
	inline TInt CurrencyDecimalPlaces() const;
	inline void SetCurrencyDecimalPlaces(TInt aPlaces);
	inline TBool CurrencyNegativeInBrackets() const;        // These two functions are deprecated
	inline void SetCurrencyNegativeInBrackets(TBool aBool); // They are here to maintain compatibility. Use the New functions -> NegativeCurrencyFormat setter/getter. 
 	inline TBool CurrencyTriadsAllowed() const;  
	inline void SetCurrencyTriadsAllowed(TBool aBool);
	inline TChar ThousandsSeparator() const;
	inline void SetThousandsSeparator(const TChar& aChar);
	inline TChar DecimalSeparator() const;
	inline void SetDecimalSeparator(const TChar& aChar);
	inline TChar DateSeparator(TInt aIndex) const;
	inline void SetDateSeparator(const TChar& aChar,TInt aIndex);
	inline TChar TimeSeparator(TInt aIndex) const;
	inline void SetTimeSeparator(const TChar& aChar,TInt aIndex);
	inline TBool AmPmSpaceBetween() const;
	inline void SetAmPmSpaceBetween(TBool aSpace);
	inline TLocalePos AmPmSymbolPosition() const;
	inline void SetAmPmSymbolPosition(TLocalePos aPos);
	inline TUint DaylightSaving() const;
	inline TBool QueryHomeHasDaylightSavingOn() const;
	inline TDaylightSavingZone HomeDaylightSavingZone() const;
	inline TUint WorkDays() const;
	inline void SetWorkDays(TUint aMask);
	inline TDay StartOfWeek() const;
	inline void SetStartOfWeek(TDay aDay);
	inline TClockFormat ClockFormat() const;
	inline void SetClockFormat(TClockFormat aFormat);
	inline TUnitsFormat UnitsGeneral() const;
	inline void SetUnitsGeneral(TUnitsFormat aFormat);
	inline TUnitsFormat UnitsDistanceShort() const;
	inline void SetUnitsDistanceShort(TUnitsFormat aFormat);
	inline TUnitsFormat UnitsDistanceLong() const;
	inline void SetUnitsDistanceLong(TUnitsFormat aFormat);
	inline TNegativeCurrencyFormat NegativeCurrencyFormat() const;
	inline void SetNegativeCurrencyFormat(TNegativeCurrencyFormat aNegativeCurrencyFormat);
	inline TBool NegativeLoseSpace() const;
	inline void SetNegativeLoseSpace(TBool aBool);
	inline TBool NegativeCurrencySymbolOpposite() const;
	inline void SetNegativeCurrencySymbolOpposite(TBool aBool);
	inline TLanguage LanguageDowngrade(TInt aIndex) const;	 // 0 <= aIndex < 3
	inline void SetLanguageDowngrade(TInt aIndex, TLanguage aLanguage);
	inline TDigitType DigitType() const;
	inline void SetDigitType(TDigitType aDigitType);
	inline TDeviceTimeState DeviceTime() const;
 	inline void SetDeviceTime(TDeviceTimeState aState);
 	inline TInt RegionCode() const;

	void SetDefaults(); /**< @internalComponent */

private:
	friend class TExtendedLocale;
private:
	TInt iCountryCode;
	TTimeIntervalSeconds iUniversalTimeOffset;
	TDateFormat iDateFormat;
	TTimeFormat iTimeFormat;
	TLocalePos iCurrencySymbolPosition;
	TBool iCurrencySpaceBetween;
	TInt iCurrencyDecimalPlaces;
	TNegativeCurrencyFormat iNegativeCurrencyFormat; //	replaced TBool iCurrencyNegativeInBrackets
	TBool iCurrencyTriadsAllowed;
	TChar iThousandsSeparator;
	TChar iDecimalSeparator;
	TChar iDateSeparator[KMaxDateSeparators];
	TChar iTimeSeparator[KMaxTimeSeparators];
	TLocalePos iAmPmSymbolPosition;
	TBool iAmPmSpaceBetween;
	TUint iDaylightSaving;
	TDaylightSavingZone iHomeDaylightSavingZone;
	TUint iWorkDays;
	TDay iStartOfWeek;
	TClockFormat iClockFormat;
	TUnitsFormat iUnitsGeneral;
	TUnitsFormat iUnitsDistanceShort;
	TUnitsFormat iUnitsDistanceLong;
	TUint iExtraNegativeCurrencyFormatFlags;
	TUint16 iLanguageDowngrade[3];
	TUint16 iRegionCode;
	TDigitType iDigitType;
 	TDeviceTimeState iDeviceTimeState;
 	TInt iSpare[0x1E];
	};

/** 
@publishedAll
@released

TLocaleAspect

Enumeration used with TExtendedLocale::LoadLocaleAspect to select which
locale information is to be replaced from the contents of the Locale
DLL being loaded.

ELocaleLanguageSettings - Replaces everything that should change with
                          language selection e.g. Month names, Day names,
                          etc,

ELocaleLocaleSettings - Replaces the currently selected currency symbol,
                        TLocale settings, and FAT utility functions

ELocaleTimeAndDateSettings - Replaces the current time and date display
                             format settings.

ELocaleCollateSettings - Replaces the "system" preferred Charset
                         (because that's where the collation table
                         is!). The "Default" charset will remain
                         unchanged until after the next power
                         off/on cycle
*/
enum TLocaleAspect
	{
	ELocaleLanguageSettings = 0x01,
	ELocaleCollateSetting = 0x02,
	ELocaleLocaleSettings = 0x04,
	ELocaleTimeDateSettings = 0x08,
	};

/**
@internalComponent
*/
struct SLocaleLanguage
	{
	TLanguage 		iLanguage;
	const TText*	iDateSuffixTable;
	const TText*	iDayTable;
	const TText*	iDayAbbTable;
	const TText*	iMonthTable;
	const TText*	iMonthAbbTable;
	const TText*	iAmPmTable;
	const TText16* const*	iMsgTable;
	};

/**
@internalComponent
*/
struct SLocaleLocaleSettings
	{
	TText	iCurrencySymbol[KMaxCurrencySymbol+1];
	TAny*	iLocaleExtraSettingsDllPtr;
	};

/**
@internalComponent
*/
struct SLocaleTimeDateFormat
	{
	TText	iShortDateFormatSpec[KMaxShortDateFormatSpec+1];
	TText	iLongDateFormatSpec[KMaxLongDateFormatSpec+1];
	TText	iTimeFormatSpec[KMaxTimeFormatSpec+1];
	TAny*	iLocaleTimeDateFormatDllPtr;
	};

struct LCharSet;

/**
@publishedAll
@released

Extended locale class

This class holds a collection of locale information. It contains a TLocale internally.
It has methods to load a locale DLL and to set the system wide locale information.

*/
class TExtendedLocale
	{
public:

	// Default constructor, create an empty instance
	IMPORT_C TExtendedLocale();

	// Initialise to (or restore from!) current system wide locale
	// settings
	IMPORT_C void LoadSystemSettings();
	
	// Overwrite current system wide locale settings with the current
	// contents of this TExtendedLocale
	IMPORT_C TInt SaveSystemSettings();

	//load a complete locale data from a single locale dll
	IMPORT_C TInt LoadLocale(const TDesC& aLocaleDllName);
	
	//load a complete locale data from three locale dlls, which are language lcoale dll, region locale dll, and collation locale dll.
	IMPORT_C TInt LoadLocale(const TDesC& aLanguageLocaleDllName, const TDesC& aRegionLocaleDllName, const TDesC& aCollationLocaleDllName);	
	
	// Load an additional Locale DLL and over-ride a selected subset
	// (currently ELocaleLanguageSettings to select an alternative set
	// of language specific text strings, ELocaleCollateSetting to
	// select a new system collation table,
	// ELocaleOverRideMatchCollationTable to locally select an
	// alternative collation order for matching text strings, or
	// ELocaleOverRideSortCollationTable for ordering text strings)
	// of settings with its contents
	IMPORT_C TInt LoadLocaleAspect(TUint aAspectGroup, const TDesC& aLocaleDllName);
	
	//load a locale aspect from a locale dll. 
	//Such as load language locale aspect from locale language dll; 
	//load region locale aspect from locale region dll; 
	//load collation locale aspect from locale collation dll. 
	//There are in all three aspect, which are langauge, region, and collation.
	IMPORT_C TInt LoadLocaleAspect(const TDesC& aLocaleDllName);

	// Set the currency Symbol
	IMPORT_C TInt SetCurrencySymbol(const TDesC &aSymbol);

	// Get the name of the DLL holding the data for a particular set
	// of Locale properties
	IMPORT_C TInt GetLocaleDllName(TLocaleAspect aLocaleDataSet, TDes& aDllName);

	// Get the preferred collation method.
	// Note that some Charsets may contain more than one Collation
	// method (e.g "dictionary" v "phonebook" ordering) so an optional
	// index parameter can be used to select between them
	IMPORT_C TCollationMethod GetPreferredCollationMethod(TInt index = 0) ;
	
	//Get the Currency Symbol
	IMPORT_C TPtrC GetCurrencySymbol();
	
	//Get the Long Date Format
	IMPORT_C TPtrC GetLongDateFormatSpec();
	
	//Get the Short Date Format
	IMPORT_C TPtrC GetShortDateFormatSpec();
	
	//Get the Time Format
	IMPORT_C TPtrC GetTimeFormatSpec();

	// Retrieve a reference to the encapsulated TLocale
	inline TLocale*	GetLocale();

private:

	TInt DoLoadLocale(const TDesC& aLocaleDllName, TLibraryFunction* aExportList);
	void DoUpdateLanguageSettings(TLibraryFunction* aExportList);
	void DoUpdateLocaleSettings(TLibraryFunction* aExportList);
	void DoUpdateTimeDateFormat(TLibraryFunction* aExportList);
	
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL	
	void DoUpdateLanguageSettingsV2(TLibraryFunction* aExportList);
	void DoUpdateLocaleSettingsV2(TLibraryFunction* aExportList);		
	TInt CheckLocaleDllName(const TDesC& aLocaleDllName, TInt& languageID);
	void AddExtension(TDes& aFileName, TInt aExtension);	
#endif

private:

	TLocale					iLocale;
	SLocaleLanguage			iLanguageSettings;
	SLocaleLocaleSettings	iLocaleExtraSettings;
	SLocaleTimeDateFormat	iLocaleTimeDateFormat;
	const LCharSet*			iDefaultCharSet;
	const LCharSet*			iPreferredCharSet;
	};




/**
@publishedAll
@released

Geometric rectangle.

The class represents a rectangle whose sides are parallel with the axes of 
the co-ordinate system. 

The co-ordinates of the top-left and bottom-right corners are used to set 
the dimensions of the rectangle. The bottom right co-ordinate is outside the 
rectangle. Thus TRect(TPoint(2,2),TSize(4,4)) is equal
to TRect(TPoint(2,2),TPoint(6,6)), 
and in both cases you get a 4x4 pixel rectangle on the screen.

Functions are provided to initialise and manipulate the rectangle and to extract 
information about it.
*/
class TRect
	{
public:
	enum TUninitialized { EUninitialized };
	/**
	Constructs a default rectangle.
	
	This initialises the co-ordinates of its top 
	left and bottom right corners to (0,0).
	*/
	TRect(TUninitialized) {}
	IMPORT_C TRect();
	IMPORT_C TRect(TInt aAx,TInt aAy,TInt aBx,TInt aBy);
	IMPORT_C TRect(const TPoint& aPointA,const TPoint& aPointB);
	IMPORT_C TRect(const TPoint& aPoint,const TSize& aSize);
	IMPORT_C TRect(const TSize& aSize);
	IMPORT_C TBool operator==(const TRect& aRect) const;
	IMPORT_C TBool operator!=(const TRect& aRect) const;
	IMPORT_C void SetRect(TInt aAx,TInt aAy,TInt aBx,TInt aBy);
	IMPORT_C void SetRect(const TPoint& aPointTL,const TPoint& aPointBR);
	IMPORT_C void SetRect(const TPoint& aPoint,const TSize& aSize);
	IMPORT_C void Move(TInt aDx,TInt aDy);
	IMPORT_C void Move(const TPoint& aOffset);
	IMPORT_C void Resize(TInt aDx,TInt aDy);
	IMPORT_C void Resize(const TSize& aSize);
	IMPORT_C void Shrink(TInt aDx,TInt aDy);
	IMPORT_C void Shrink(const TSize& aSize);
	IMPORT_C void Grow(TInt aDx,TInt aDy);
	IMPORT_C void Grow(const TSize& aSize);
	IMPORT_C void BoundingRect(const TRect& aRect);
	IMPORT_C TBool IsEmpty() const;
	IMPORT_C TBool Intersects(const TRect& aRect) const;
	IMPORT_C void Intersection(const TRect& aRect);
	IMPORT_C void Normalize();
	IMPORT_C TBool Contains(const TPoint& aPoint) const;
	IMPORT_C TSize Size() const;
	IMPORT_C TInt Width() const;
	IMPORT_C TInt Height() const;
	IMPORT_C TBool IsNormalized() const;
	IMPORT_C TPoint Center() const;
	IMPORT_C void SetSize(const TSize& aSize);
	IMPORT_C void SetWidth(TInt aWidth);
	IMPORT_C void SetHeight(TInt aHeight);
private:
	void Adjust(TInt aDx,TInt aDy);
public:
	/**
	The x and y co-ordinates of the top left hand corner of the rectangle.
	*/
	TPoint iTl;
	
	/**
	The x and y co-ordinates of the bottom right hand corner of the rectangle.
	*/
	TPoint iBr;
	};




/**
@publishedAll
@released

Clipping region - abstract base class. 

This abstract base class represents a 2-dimensional area which is used by 
Graphics, the graphics window server, and the text window server to define 
regions of the display which need to be updated, or regions within which all 
operations must occur. 

A TRegion is defined in terms of an array of TRects and the more complex the 
region, the more TRects are required to represent it.

A clipping region initially has space allocated for five rectangles.
If manipulations result in a region which requires more than this, an attempt
is made to allocate more rectangles. If this cannot be done, an error flag
is set, and all subsequent operations involving the region have no effect
(except possibly to propagate the error flag to other regions).
The CheckError() member function allows 
the error flag to be tested; Clear() can be used to clear it.

The redraw logic of application programs may use the TRegion in various ways:

1. minimally, they pass it to the graphics context as the clipping region; when 
   a graphics context is activated to a window, the clipping region is set up 
   automatically

2. if they wish to avoid redrawing objects which are outside the general area 
   of the region, they may use TRegion::BoundingRect() to return the rectangle 
   which bounds the clipping region, and draw only primitives that lie within 
   that rectangle

3. if they wish to exercise finer control, they may extract the individual rectangles 
   that comprise the clipping region using Operator[]().

Application programs may also manipulate clipping regions in order to constrain 
parts of their redrawing to narrower areas of the screen than the clipping 
region offered by the window server. To do this, functions that allow clipping 
region manipulation may be used; for example, adding or removing rectangles 
or finding the intersection or union of two regions.
*/
class TRegion
	{
public:
	inline TInt Count() const;
	inline const TRect* RectangleList() const;
	inline TBool CheckError() const;
	IMPORT_C TBool IsEmpty() const;
	IMPORT_C TRect BoundingRect() const;
	IMPORT_C const TRect& operator[](TInt aIndex) const;
	IMPORT_C void Copy(const TRegion& aRegion);
	IMPORT_C void AddRect(const TRect& aRect);
	IMPORT_C void SubRect(const TRect& aRect,TRegion* aSubtractedRegion=NULL);
	IMPORT_C void Offset(TInt aXoffset,TInt aYoffset);
	IMPORT_C void Offset(const TPoint& aOffset);
	IMPORT_C void Union(const TRegion& aRegion);
	IMPORT_C void Intersection(const TRegion& aRegion,const TRegion& aRegion2);
	IMPORT_C void Intersect(const TRegion& aRegion);
	IMPORT_C void SubRegion(const TRegion& aRegion,TRegion* aSubtractedRegion=NULL);
	IMPORT_C void ClipRect(const TRect& aRect);
	IMPORT_C void Clear();
	IMPORT_C void Tidy();
	IMPORT_C TInt Sort();
	IMPORT_C TInt Sort(const TPoint& aOffset);
	IMPORT_C void ForceError();
	IMPORT_C TBool IsContainedBy(const TRect& aRect) const;
	IMPORT_C TBool Contains(const TPoint& aPoint) const;
	IMPORT_C TBool Intersects(const TRect& aRect) const;	
protected:
	IMPORT_C TRect* RectangleListW();
	IMPORT_C TRegion(TInt aAllocedRects);
	inline TRegion();
	TBool SetListSize(TInt aCount);
	void AppendRect(const TRect& aRect);
	void DeleteRect(TRect* aRect);
	void AppendRegion(TRegion& aRegion);
	void MergeRect(const TRect& aRect, TBool aEnclosed);
	void SubtractRegion(const TRegion &aRegion,TRegion *aSubtractedRegion=NULL);
	void ShrinkRegion();
	TRect* ExpandRegion(TInt aCount);
protected:
	TInt iCount;
	TBool iError;
	TInt iAllocedRects;
protected:
	enum {ERRegionBuf=0x40000000};
	};




/**
@publishedAll
@released

Expandable region.

This class provides for the construction and destruction of a TRegion, including 
a granularity for expanding the region. A region;s granularity represents 
the number of memory slots allocated when the object is created, and the number 
of new memory slots allocated each time an RRegion is expanded beyond the 
number of free slots. The default granularity is five.
*/
class RRegion : public TRegion
	{
private:
	enum {EDefaultGranularity=5};
protected:
	IMPORT_C RRegion(TInt aBuf,TInt aGran);
public:
	IMPORT_C RRegion();
	IMPORT_C RRegion(TInt aGran);
	IMPORT_C RRegion(const RRegion& aRegion);
	IMPORT_C RRegion(const TRect& aRect,TInt aGran=EDefaultGranularity);
	IMPORT_C RRegion(TInt aCount,TRect* aRectangleList,TInt aGran=EDefaultGranularity);
	IMPORT_C void Close();
	IMPORT_C void Destroy();
	inline TInt CheckSpare() const;
private:
	TInt iGranularity;
	TRect* iRectangleList;
	friend class TRegion;
	};




/**
@publishedAll
@released

Region with pre-allocated buffer. 

This class provides the functionality of an RRegion, but in addition, for 
optimisation purposes, uses a buffer containing pre-allocated space for as 
many rectangles as are specified in the granularity. 

When this buffer is full, cell allocation takes place as for an RRegion, and 
the RRegionBuf effectively becomes an RRegion. In this case, the region does 
not revert to using the buffer, even if the region were to shrink so that 
the buffer could, once again, contain the region. When the region is no longer 
required, call Close(), defined in the base class RRegion, to free up all 
memory.
*/
template <TInt S>
class RRegionBuf : public RRegion
	{
public:
	inline RRegionBuf();
	inline RRegionBuf(const RRegion& aRegion);
	inline RRegionBuf(const RRegionBuf<S>& aRegion);
	inline RRegionBuf(const TRect& aRect);
private:
	TInt8 iRectangleBuf[S*sizeof(TRect)];
	};




/**
@publishedAll
@released

A fixed size region.

The region consists of a fixed number of rectangles; this number is specified 
in the templated argument. The region cannot be expanded to contain more than 
this number of rectangles. If an attempt is made to do so, the region's 
error flag is set, and the region is cleared.

Note that when adding a rectangle to a region, if that rectangle overlaps 
an existing rectangle, the operation causes more than one rectangle to be 
created.
*/
template <TInt S>
class TRegionFix : public TRegion
	{
public:
	inline TRegionFix();
	inline TRegionFix(const TRect& aRect);
	inline TRegionFix(const TRegionFix<S>& aRegion);
private:
	TInt8 iRectangleBuf[S*sizeof(TRect)];
	};




/**
@publishedAll
@released

Base class for searching for global kernel objects.

This is the base class for a number of classes which are used to find specific 
types of global kernel object such as semaphores, threads and mutexes;
TFindSemaphore, TFindThread and TFindMutex are typical examples of such
derived classes.

The class implements the common behaviour, specifically, the storage of the 
match pattern which is used to search for object names.

This class is not intended to be explicitly instantiated; it has public
constructors but they are part of the class implementation and are described
for information only.
*/
class TFindHandleBase : public TFindHandle
	{
public:
	IMPORT_C TFindHandleBase();
	IMPORT_C TFindHandleBase(const TDesC& aMatch);
	IMPORT_C void Find(const TDesC& aMatch);
protected:
	TInt NextObject(TFullName& aResult,TInt aObjectType);
private:
	
	/**
	The full name of the last kernel side object found.
	*/
	TFullName iMatch;
	};




/**
@publishedAll
@released

Finds all global semaphores whose full names match a specified pattern.

The match pattern can be set into the TFindSemaphore object at construction; 
it can also be changed at any time after construction by using the Find() 
member function of the TFindHandleBase base class.

After construction, the Next() member function can be used repeatedly to find 
successive global semaphores whose full names match the current pattern.

A successful call to Next() means that a matching global semaphore has been 
found. To open a handle on this semaphore, call the RSemaphore::Open() function 
and pass a reference to this TFindSemaphore.

Pattern matching is part of descriptor behaviour.

@see TFindHandleBase::Find
@see TFindSemaphore::Next
@see RSemaphore::Open
@see TDesC16::Match
@see TDesC8::Match
*/
class TFindSemaphore : public TFindHandleBase
	{
public:
	inline TFindSemaphore();
	inline TFindSemaphore(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};




/**
@publishedAll
@released

Finds all global mutexes whose full names match a specified pattern.

The match pattern can be set into the object at construction; it can also 
be changed at any time after construction by using the Find() member function 
of the base class.

After construction, the Next() member function may be used repeatedly to find 
successive global mutexes whose full names match the current pattern.

A successful call to Next() means that a matching global mutex has been found. 
To open a handle on this mutex, call the Open() member function of RMutex 
and pass a reference to this TFindMutex object.

Pattern matching is part of descriptors behaviour.

@see TFindHandleBase::Find
@see TFindMutex::Next
@see RMutex::Open
@see TDesC16::Match
@see TDesC8::Match
*/
class TFindMutex : public TFindHandleBase
	{
public:
	inline TFindMutex();
	inline TFindMutex(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};




/**
@publishedAll
@released

Searches for all global chunks by pattern matching against the names of (Kernel 
side) chunk objects.

The match pattern can be set into this object at construction; it can also 
be changed at any time after construction by using TFindHandleBase::Find().

After construction, call TFindChunk::Next() repeatedly to find successive 
chunks whose names match the current pattern. A successful call
to TFindChunk::Next() means that a matching chunk has been found.

@see TFindHandleBase
*/
class TFindChunk : public TFindHandleBase
	{
public:
	inline TFindChunk();
	inline TFindChunk(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};





/**
@publishedAll
@released

Searches for threads by pattern matching against the names
of thread objects.

The match pattern can be set into this object at construction; it can also be
changed at any time after construction by using TFindHandleBase::Find().

After construction, call TFindThread::Next() repeatedly to find successive
threads whose names match the current pattern.
A successful call to TFindThread::Next() means that a matching thread has
been found. To open a handle on this thread, call RThread::Open() and pass
a reference to this TFindThread.

@see RThread
*/
class TFindThread : public TFindHandleBase
	{
public:
	inline TFindThread();
	inline TFindThread(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};




/**
@publishedAll
@released

Searches for processes by pattern matching against the names
of process objects.

The match pattern can be set into this object at construction; it can also be
changed at any time after construction by using TFindHandleBase::Find().

After construction, call TFindProcess::Next() repeatedly to find successive
processes whose names match the current pattern.
A successful call to TFindProcess::Next() means that a matching process has
been found. To open a handle on this process, call RProcess::Open() and pass
a reference to this TFindProcess.

@see RProcess
*/
class TFindProcess : public TFindHandleBase
	{
public:
	inline TFindProcess();
	inline TFindProcess(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};



/**
@publishedAll
@released

Searches for LDD factory objects by pattern matching against the names of 
 LDD factory objects.

An LDD factory object is an instance of a DLogicalDevice derived class. 

The match pattern can be set into this object at construction; it can also 
be changed at any time after construction by using TFindHandleBase::Find().

After construction, call TFindLogicalDevice::Next() repeatedly to find successive 
LDD factory objects whose names match the current pattern. A successful call to 
TFindLogicalDevice::Next() means that a matching LDD factory object has been found.

The name of an LDD factory object is set by its Install() member function as 
part of the construction process.
*/
class TFindLogicalDevice : public TFindHandleBase
	{
public:
	inline TFindLogicalDevice();
	inline TFindLogicalDevice(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};

/**
@publishedAll
@released

Searches for PDD factory objects by pattern matching against the names of
PDD factory objects.

A PDD factory object is an instance of a DPhysicalDevice derived class. 

The match pattern can be set into this object at construction; it can also be 
changed at any time after construction by using TFindHandleBase::Find().

After construction, call TFindPhysicalDevice::Next() repeatedly to find successive 
PDD factory objects whose names match the current pattern. A successful call to 
TFindPhysicalDevice::Next() means that a matching PDD factory object has been found.

The name of a PDD factory object is set by its Install() member function as part 
of the construction process.
*/
class TFindPhysicalDevice : public TFindHandleBase
	{
public:
	inline TFindPhysicalDevice();
	inline TFindPhysicalDevice(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};





/**
@publishedAll
@released

Searches for servers by pattern matching against the names of kernel side
server objects.

The match pattern can be set into this object at construction; it can also
be changed at any time after construction by using the TFindHandleBase::Find()
base class.

After construction, call TFindServer::Next() repeatedly to find successive
servers whose names match the current pattern.
A successful call to TFindServer::Next() means that a matching server
has been found.
*/
class TFindServer : public TFindHandleBase
	{
public:
	inline TFindServer();
	inline TFindServer(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};




/**
@publishedAll
@released

Searches for DLLs whose full names match a specified pattern.

The match pattern is set at construction but can also be changed at any time
after construction by using TFindHandleBase::Find().

After construction, use TFindLibrary::Next() to repeatedly find successive DLLs
whose names match the current pattern. A successful call to
TFindLibrary::Next() means that a matching DLL has been found.
*/
class TFindLibrary : public TFindHandleBase
	{
public:
	inline TFindLibrary();
	inline TFindLibrary(const TDesC& aMatch);
	IMPORT_C TInt Next(TFullName& aResult);
	};



/**
@publishedAll
@released

User side handle to an LDD factory object, an instance of a DLogicalDevice 
derived class.

The LDD factory object is a Kernel side object which is constructed on the 
Kernel heap when the logical device is opened using User::LoadLogicalDevice(). 
The handle allows the User side to get information about the logical device.

To use the device, a thread must create and use an instance of an 
RBusLogicalChannel derived class.

*/
class RDevice : public RHandleBase
	{
public:
	inline TInt Open(const TFindLogicalDevice& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(const TDesC& aName,TOwnerType aType=EOwnerProcess);
	IMPORT_C void GetCaps(TDes8& aDes) const;
	IMPORT_C TBool QueryVersionSupported(const TVersion& aVer) const;
	IMPORT_C TBool IsAvailable(TInt aUnit, const TDesC* aPhysicalDevice, const TDesC8* anInfo) const;
	};

/**
@publishedAll
@released

Asynchronous timer services. 

Five types of asynchronous request are supported by the class:

1. Requesting an event after a specified interval

2. Requesting an event at a specified system time

3. Requesting a timer event on a specific second fraction

4. Requesting an event if an interval elapses with no user activity.

5. Requesting an event after a specified interval, to a resolution of 1ms.
   
Each of these requests can be cancelled.

The timer exists from its creation, following a call to RTimer::CreateLocal(),
until it is destroyed by a call to the Close() member function of the base
class RHandleBase.

This class is ultimately implemented in terms of the nanokernel tick, and
therefore the granularity of the generated events is limited to the period of
this timer.  This is variant specific, but is usually 1 millisecond.

Note that the CTimer active object uses an RTimer.
*/
class RTimer : public RHandleBase
	{
public:
	IMPORT_C TInt CreateLocal();
	IMPORT_C void Cancel();
	IMPORT_C void After(TRequestStatus& aStatus,TTimeIntervalMicroSeconds32 anInterval);
	IMPORT_C void AfterTicks(TRequestStatus &aStatus, TInt aTicks);
	IMPORT_C void At(TRequestStatus& aStatus,const TTime& aTime);
	IMPORT_C void AtUTC(TRequestStatus& aStatus,const TTime& aUTCTime);
	IMPORT_C void Lock(TRequestStatus& aStatus,TTimerLockSpec aLock);
	IMPORT_C void Inactivity(TRequestStatus& aStatus, TTimeIntervalSeconds aSeconds);
	IMPORT_C void HighRes(TRequestStatus& aStatus,TTimeIntervalMicroSeconds32 anInterval);
	};




/**
@publishedAll
@released

A handle to a dynamically loadable DLL.

The class is not intended for user derivation.
*/
class RLibrary : public RHandleBase
	{
public:
	IMPORT_C void Close();
	IMPORT_C TInt Load(const TDesC& aFileName, const TUidType& aType);
	IMPORT_C TInt Load(const TDesC& aFileName, const TDesC& aPath=KNullDesC);
	IMPORT_C TInt Load(const TDesC& aFileName, const TDesC& aPath, const TUidType& aType);
	IMPORT_C TInt Load(const TDesC& aFileName, const TDesC& aPath, const TUidType& aType, TUint32 aModuleVersion);
	IMPORT_C TInt LoadRomLibrary(const TDesC& aFileName, const TDesC& aPath);
	IMPORT_C TLibraryFunction Lookup(TInt anOrdinal) const;
	IMPORT_C TUidType Type() const;
	IMPORT_C TFileName FileName() const;
	IMPORT_C TInt GetRamSizes(TInt& aCodeSize, TInt& aConstDataSize);
	IMPORT_C TInt Init(); /**< @internalTechnology */
public:
	/**
	Class representing information about an executable binary, (DLL or EXE).
	@internalTechnology
	*/
	struct TInfo
		{
		TUint32 iModuleVersion;			/**< Version number */
		TUidType iUids;					/**< UIDs */
		TSecurityInfo iSecurityInfo;	/**< Security Info */
		};

	/**
	Class representing information about an executable binary, (DLL or EXE), version 2.
	@internalTechnology
	*/
	struct TInfoV2 : public TInfo
		{
		TUint8 iHardwareFloatingPoint;	/**< Which hardware floating point used, from TFloatingPointType */
		enum TDebugAttributes
		{
			EDebugAllowed = 1<<0, ///< Flags set if executable may be debugged.
			ETraceAllowed = 1<<1 ///< Flags set if executable may be traced.
		};
		TUint8 iDebugAttributes; ///< Bitmask of values from enum TDebugAttributes
		TUint8 iSpare[6];
		};

	/**
	Type representing a TInfo struct packaged as a descriptor.
	@internalTechnology
	*/
	typedef TPckgBuf<TInfo> TInfoBuf;

	/**
	Type representing a TInfo struct packaged as a descriptor, version 2.
	@internalTechnology
	*/
	typedef TPckgBuf<TInfoV2> TInfoBufV2;

	/**
	@internalTechnology
	*/
	enum TRequiredImageHeaderSize
		{
#ifdef __WINS__
		/**
		Size of header data which should be passed to GetInfoFromHeader()
		*/
		KRequiredImageHeaderSize = KMaxTInt
#else
		KRequiredImageHeaderSize = 9*1024
#endif
		};

	IMPORT_C static TInt GetInfoFromHeader(const TDesC8& aHeader, TDes8& aInfoBuf);

	/**
	@internalTechnology
	@deprecated Use TInfo
	*/
	struct SInfo
		{
		TUint32 iModuleVersion;
		TUidType iUids;
		SSecurityInfo iS;
		};

	/**
	@internalTechnology
	@deprecated Use TInfoBuf
	*/
	typedef TPckgBuf<SInfo> SInfoBuf;

	/**
	@internalTechnology
	*/
	IMPORT_C static TInt GetInfo(const TDesC& aFileName, TDes8& aInfoBuf);
private:
	TInt InitL();
	};




/**
@publishedAll
@released

A handle to a critical section.

A critical section itself is a kernel object, and is implemented using
a semaphore. The class RCriticalSection inherits privately from RSemaphore
as a matter of implementation and this is, in effect, equivalent to using
a semaphore.

The public functions of RSemaphore are not part of the public API of this 
class.

As with all handles, they should be closed after use. This class provides 
the necessary Close() function, which should be called when the handle is 
no longer required.

@see RHandleBase::Close
*/
class RCriticalSection : private RSemaphore
	{
public:
	IMPORT_C RCriticalSection();
	IMPORT_C TInt CreateLocal(TOwnerType aType=EOwnerProcess);
	IMPORT_C void Close();
	IMPORT_C void Wait();
	IMPORT_C void Signal();
	inline TBool IsBlocked() const;
private:
	TInt iBlocked;
	};



/**
@publishedAll
@released

A handle to a mutex.

The mutex itself is a kernel side object.

Handles should be closed after use. RHandleBase provides the necessary Close() 
function which should be called when the handle is no longer required.

@see RHandleBase::Close
*/
class RMutex : public RHandleBase
	{
public:
	inline TInt Open(const TFindMutex& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateLocal(TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateGlobal(const TDesC& aName,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt OpenGlobal(const TDesC& aName,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	IMPORT_C void Wait();
	IMPORT_C void Signal();
	IMPORT_C TBool IsHeld();
	};



/**
@publishedAll
@released

A handle to a condition variable.

The condition variable itself is a kernel side object.

Handles should be closed after use. RHandleBase provides the necessary Close() 
function which should be called when the handle is no longer required.

@see RHandleBase::Close
*/
class RCondVar : public RHandleBase
	{
public:
	IMPORT_C TInt CreateLocal(TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateGlobal(const TDesC& aName, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt OpenGlobal(const TDesC& aName, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(RMessagePtr2 aMessage, TInt aParam, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Wait(RMutex& aMutex);
	IMPORT_C TInt TimedWait(RMutex& aMutex, TInt aTimeout);	// timeout in microseconds
	IMPORT_C void Signal();
	IMPORT_C void Broadcast();
	};



class UserHeap;
class TChunkCreate;
struct TChunkCreateInfo;
/**
@publishedAll
@released

A handle to a chunk.

The chunk itself is a kernel side object.
*/
class RChunk : public RHandleBase
	{
public:
	/**	
	Set of flags used by SetRestrictions().
	
	@see RChunk::SetRestrictions
	*/
	enum TRestrictions
		{
		/** Prevent Adjust, Commit, Allocate and Decommit */
		EPreventAdjust = 0x01,
		};
public:
	inline TInt Open(const TFindChunk& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateLocal(TInt aSize,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateLocalCode(TInt aSize,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateGlobal(const TDesC& aName,TInt aSize,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateDoubleEndedLocal(TInt aInitialBottom, TInt aInitialTop,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateDoubleEndedGlobal(const TDesC& aName,TInt aInitialBottom,TInt aInitialTop,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateDisconnectedLocal(TInt aInitialBottom, TInt aInitialTop,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt CreateDisconnectedGlobal(const TDesC& aName,TInt aInitialBottom,TInt aInitialTop,TInt aMaxSize,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Create(TChunkCreateInfo& aCreateInfo);
	IMPORT_C TInt SetRestrictions(TUint aFlags);
	IMPORT_C TInt OpenGlobal(const TDesC& aName,TBool isReadOnly,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,TBool isReadOnly,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Adjust(TInt aNewSize) const;
	IMPORT_C TInt AdjustDoubleEnded(TInt aBottom, TInt aTop) const;
	IMPORT_C TInt Commit(TInt anOffset, TInt aSize) const;
	IMPORT_C TInt Allocate(TInt aSize) const;
	IMPORT_C TInt Decommit(TInt anOffset, TInt aSize) const;
	IMPORT_C TInt Unlock(TInt aOffset, TInt aSize);	/**< @internalTechnology */
	IMPORT_C TInt Lock(TInt aOffset, TInt aSize);	/**< @internalTechnology */
	IMPORT_C TUint8* Base() const;
	IMPORT_C TInt Size() const;
	IMPORT_C TInt Bottom() const;
	IMPORT_C TInt Top() const;
	IMPORT_C TInt MaxSize() const;
	inline TBool IsReadable() const;
	inline TBool IsWritable() const;
	IMPORT_C TBool IsPaged() const;
private:
	friend class UserHeap;
	};


/**
This structure specifies the type and properties of the chunk to be created.  It
is passed as a parameter to the RChunk::Create() method.

@publishedAll
@released
*/
struct TChunkCreateInfo
	{
public :
	/**
	Currently supported version numbers
	@internalComponent
	*/
	enum TChunkCreateVersions
		{
		EVersion0,
		ESupportedVersions,
		};

	friend class RChunk;

	/**
	Attributes that specify whether the chunk to be created	is data paged or not.
	*/
	enum TChunkPagingAtt
		{
		EUnspecified,	/**< The chunk will use the creating process's paging attributes.*/
		EPaged,			/**< The chunk will be data paged.*/
		EUnpaged,		/**< The chunk will not be data paged.*/
		};

	IMPORT_C TChunkCreateInfo();
	IMPORT_C void SetNormal(TInt aInitialSize, TInt aMaxSize);
	IMPORT_C void SetCode(TInt aInitialSize, TInt aMaxSize);
	IMPORT_C void SetDoubleEnded(TInt aInitialBottom, TInt aInitialTop, TInt aMaxSize);
	IMPORT_C void SetDisconnected(TInt aInitialBottom, TInt aInitialTop, TInt aMaxSize);
	IMPORT_C void SetOwner(TOwnerType aType);
	IMPORT_C void SetGlobal(const TDesC& aName);
	IMPORT_C void SetClearByte(TUint8 aClearByte);
	IMPORT_C void SetPaging(const TChunkPagingAtt aPaging);
	IMPORT_C void SetReadOnly();
	void SetThreadHeap(TInt aInitialSize, TInt aMaxSize, const TDesC& aName);

	/**
	For use by file server only.
	@internalTechnology
	*/
	IMPORT_C void SetCache(TInt aMaxSize);
protected :
	/** The version number of this TChunkCreateInfo.
	@internalComponent
	*/
	TUint iVersionNumber;
	/** The type of the chunk to be created.
	@internalComponent
	*/
	TUint iType;
	/** Specify if chunk is global or not.
	@internalComponent
	*/
	TBool iGlobal;
	/**	The maximum size in bytes of the chunk to be created.
	@internalComponent
	*/
	TInt iMaxSize;
	/** An enumeration whose enumerators define the ownership of this chunk 
		handle. If not explicitly specified, EOwnerProcess is taken as default.
	@internalComponent
	*/
	TOwnerType iOwnerType;
	/**	A pointer to a descriptor containing the name to be assigned to  
		global chunks. The length of the descriptor must be no greater than 
		that allowed for a TKName type.  Must be NULL for local chunks.
	@internalComponent
	*/
	const TDesC* iName;
	/** The offset of the bottom of the region to commit to the chunk on 
		creation from the base of the chunk's reserved region.
		This is only used for double ended and disconnected chunks.
	@internalComponent
	*/
	TInt iInitialBottom;
	/** The offset of the top of the region to commit to the chunk on 
		creation from the base of the chunk's reserved region.
		This is only used for double ended and disconnected chunks.
	@internalComponent
	*/
	TInt iInitialTop;
	/**	Attributes to the chunk to be created should have.
		Should be set from one or more the values in TChunkCreate::TChunkCreateAtt.
	@internalComponent
	*/
	TUint iAttributes;
	/** The byte to clear all the memory committed to the chunk to.
	@internalComponent
	*/
	TUint8 iClearByte; 
	/** @internalComponent*/
	TUint8 iSpare1[3];
	/** @internalComponent*/
	TUint iSpare2;
	};

/**
This structure specifies the type and properties of the user heap to be created.  It
is passed as a parameter to the UserHeap::Create() method.

@publishedAll
@released
*/
struct TChunkHeapCreateInfo
	{
public:
	/**
	Currently supported version numbers
	@internalComponent
	*/
	enum TChunkHeapCreateVersions
		{
		EVersion0,
		ESupportedVersions,
		};

	/**
	Attributes that specify whether the chunk heap to be created is data paged or not.
	*/
	enum TChunkHeapPagingAtt
		{
		EUnspecified,	/**< The chunk heap will use the creating process's paging attributes.*/
		EPaged,			/**< The chunk heap will be data paged.*/
		EUnpaged,		/**< The chunk heap will not be data paged.*/
		};

	friend class UserHeap;

	IMPORT_C TChunkHeapCreateInfo(TInt aMinLength, TInt aMaxLength);
	IMPORT_C void SetCreateChunk(const TDesC* aName);
	IMPORT_C void SetUseChunk(const RChunk aChunk);
	inline void SetSingleThread(TBool aSingleThread);
	inline void SetAlignment(TInt aAlign);
	inline void SetGrowBy(TInt aGrowBy);
	inline void SetOffset(TInt aOffset);
	inline void SetMode(TUint aMode);
	inline void SetPaging(const TChunkHeapPagingAtt aPaging);

protected:
	/** The version number of this TChunkHeapCreateInfo.
	@internalComponent
	*/
	TUint iVersionNumber;
	/** The minimum size for the heap.
	@internalConponent
	*/
	TInt iMinLength;
	/** The maximum size for the heap.
	@internalConponent
	*/
	TInt iMaxLength;
	/** The alignment of the heap.
	@internalComponent
	*/
	TInt iAlign;
	/** The grow by value of the heap.
	@internalComponent
	*/
	TInt iGrowBy;
	/** The single thread value of the heap.
	@internalComponent
	*/
	TBool iSingleThread;
	/** The offset from the base of the chunk to the start of the heap.
	@internalComponent
	*/
	TInt iOffset;
	/** The paging attributes of the chunk.
	@internalComponent
	*/
	TChunkHeapPagingAtt iPaging;
	/** The mode flags for the heap.
	@internalComponent
	*/
	TUint iMode;
	/** The name of the chunk to be created for the heap.
	@internalComponent
	*/
	TDesC* iName;
	/** The chunk to use for the heap.
	@internalComponent
	*/
	RChunk iChunk;
	/**@internalComponent*/
	TInt iSpare[5];
	};

struct SStdEpocThreadCreateInfo;
/**
@publishedAll
@released

A set of static functions for constructing fixed length heaps and local or 
global heaps.

@see RHeap
@see RChunk
*/
class UserHeap
	{
public:
	/**
	Flags to control the heap creation.
	*/
	enum TChunkHeapCreateMode
		{
		/** On successful creation of the heap this switches the calling thread to
			use the new heap.
		*/
		EChunkHeapSwitchTo	= 0x1,
		/** On successful creation of the heap this causes the handle to the heap
			to be duplicated.
		*/ 
		EChunkHeapDuplicate	= 0x2,

		/** @internalComponent*/
		EChunkHeapMask = EChunkHeapSwitchTo | EChunkHeapDuplicate,
		};
	IMPORT_C static RHeap* FixedHeap(TAny* aBase, TInt aMaxLength, TInt aAlign=0, TBool aSingleThread=ETrue);
	IMPORT_C static RHeap* ChunkHeap(const TDesC* aName, TInt aMinLength, TInt aMaxLength, TInt aGrowBy=1, TInt aAlign=0, TBool aSingleThread=EFalse);
	IMPORT_C static RHeap* ChunkHeap(RChunk aChunk, TInt aMinLength, TInt aGrowBy=1, TInt aMaxLength=0, TInt aAlign=0, TBool aSingleThread=EFalse, TUint32 aMode=0);
	IMPORT_C static RHeap* OffsetChunkHeap(RChunk aChunk, TInt aMinLength, TInt aOffset, TInt aGrowBy=1, TInt aMaxLength=0, TInt aAlign=0, TBool aSingleThread=EFalse, TUint32 aMode=0);
	IMPORT_C static RHeap* ChunkHeap(const TChunkHeapCreateInfo& aCreateInfo);
	IMPORT_C static TInt SetupThreadHeap(TBool aNotFirst, SStdEpocThreadCreateInfo& aInfo);
	IMPORT_C static TInt CreateThreadHeap(SStdEpocThreadCreateInfo& aInfo, RHeap*& aHeap, TInt aAlign=0, TBool aSingleThread=EFalse);
	};




/**
@publishedAll
@released

Encapsulates the Id of a kernel object.
*/
class TObjectId
	{
public:
	inline TObjectId();
	inline TObjectId(TUint64 anId);
	inline TUint64 Id() const;
	inline operator TUint() const;
	inline TBool operator==(TObjectId aId) const;
	inline TBool operator!=(TObjectId aId) const;
private:
	TUint64 iId;
	};




/**
@publishedAll
@released

Encapsulates the Id of a thread.

An object of this type is not explicitly constructed in open code,
but is returned by the Id() member function of a thread handle,
an RThread type.

@see RThread
*/
class TThreadId : public TObjectId
	{
public:
	inline TThreadId();
	inline TThreadId(TUint64 anId);
	};




/**
This structure specifies the type and properties of the thread to be created.  It
is passed as a parameter to the RThread::Create() method.

@publishedAll
@released
*/
struct TThreadCreateInfo
	{
public :
	/**
	Currently supported version numbers
	@internalComponent
	*/
	enum TThreadCreateVersions
		{
		EVersion0,
		ESupportedVersions,
		};

	/**
	Attributes that specify whether the stack and heap of the thread to be created
	are data paged or not.
	*/
	enum TThreadPagingAtt
		{
		EUnspecified,	/**< The thread will use the creating process's paging attributes.*/
		EPaged,			/**< The thread will data page its stack and heap.*/
		EUnpaged,		/**< The thread will not data page its stack and heap.*/
		};

	friend class RThread;

	IMPORT_C TThreadCreateInfo(	const TDesC &aName, TThreadFunction aFunction, 
								TInt aStackSize, TAny* aPtr);
	IMPORT_C void SetCreateHeap(TInt aHeapMinSize, TInt aHeapMaxSize);
	IMPORT_C void SetUseHeap(const RAllocator* aHeap);
	IMPORT_C void SetOwner(const TOwnerType aOwner);
	IMPORT_C void SetPaging(const TThreadPagingAtt aPaging);

protected:
	/**	The version number of this TChunkCreateInfo.
		@internalComponent
	*/
	TUint iVersionNumber;
	/**	The Name to be given to the thread to be created
		@internalComponent
	*/
	const TDesC* iName;
	/**	The function this thread will execute.
		@internalComponent
	*/
	TThreadFunction iFunction;
	/**	The size of the stack of the thread to be created.
		@internalComponent
	*/
	TInt iStackSize;
	/** The parameter to be passed to the function of the thread to be created.
		@internalComponent
	*/
	TAny* iParameter;
	/** The owner of the thread to be created.
		@internalComponent
	*/
	TOwnerType iOwner;
	/**	The heap for the thread to be created to use.
		NULL if a new heap is to be created.
		@internalComponent
	*/
	RAllocator* iHeap;
	/**	Minimum size of any heap to be created for the thread.
		@internalComponent*/
	TInt iHeapMinSize;
	/**	Maximum size of any heap to be created for the thread.
		@internalComponent
	*/
	TInt iHeapMaxSize;
	/** The attributes of the thread
		@internalComponent
	*/
	TUint iAttributes;
	/**@internalComponent*/
	TUint32 iSpare[6];
	};

class RProcess;


/**
@publishedAll
@released

A handle to a thread.

The thread itself is a kernel object.
*/
class RThread : public RHandleBase
	{
public:
	inline RThread();
	IMPORT_C TInt Create(const TDesC& aName, TThreadFunction aFunction, TInt aStackSize, TInt aHeapMinSize, TInt aHeapMaxSize, TAny *aPtr, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Create(const TDesC& aName, TThreadFunction aFunction, TInt aStackSize, RAllocator* aHeap, TAny* aPtr, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Create(const TThreadCreateInfo& aCreateInfo);
	IMPORT_C TInt Open(const TDesC& aFullName, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TThreadId aID, TOwnerType aType=EOwnerProcess);
	IMPORT_C TThreadId Id() const;
	IMPORT_C void Resume() const;
	IMPORT_C void Suspend() const;
	/**
	@publishedAll
	@deprecated Use User::RenameThread() instead
	*/
	inline static TInt RenameMe(const TDesC& aName);

	IMPORT_C void Kill(TInt aReason);
	IMPORT_C void Terminate(TInt aReason);
	IMPORT_C void Panic(const TDesC& aCategory,TInt aReason);
	IMPORT_C TInt Process(RProcess& aProcess) const;
	IMPORT_C TThreadPriority Priority() const;
	IMPORT_C void SetPriority(TThreadPriority aPriority) const;
	IMPORT_C TProcessPriority ProcessPriority() const;
	IMPORT_C void SetProcessPriority(TProcessPriority aPriority) const;
	IMPORT_C TInt RequestCount() const;
	IMPORT_C TExitType ExitType() const;
	IMPORT_C TInt ExitReason() const;
	IMPORT_C TExitCategoryName ExitCategory() const;
	IMPORT_C void RequestComplete(TRequestStatus*& aStatus,TInt aReason) const;
	IMPORT_C void RequestSignal() const;
	IMPORT_C void Logon(TRequestStatus& aStatus) const;
	IMPORT_C TInt LogonCancel(TRequestStatus& aStatus) const;
	IMPORT_C void HandleCount(TInt& aProcessHandleCount, TInt& aThreadHandleCount) const;
	IMPORT_C void Context(TDes8& aDes) const;
	IMPORT_C TInt StackInfo(TThreadStackInfo& aInfo) const;
	IMPORT_C TInt GetCpuTime(TTimeIntervalMicroSeconds& aCpuTime) const;
	inline TInt Open(const TFindThread& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C void Rendezvous(TRequestStatus& aStatus) const;
	IMPORT_C TInt RendezvousCancel(TRequestStatus& aStatus) const;
	IMPORT_C static void Rendezvous(TInt aReason);

	/**
	Return the Secure ID of the process to which the thread belongs.

	If an intended use of this method is to check that the Secure ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RThread& thread;
		TInt error = thread.SecureId()==KRequiredSecureId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RThread& thread;
		static _LIT_SECURITY_POLICY_S0(mySidPolicy, KRequiredSecureId);
		TBool pass = mySidPolicy().CheckPolicy(thread);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_S0

	@return The Secure ID.
	@publishedAll
	@released
	*/
	IMPORT_C TSecureId SecureId() const;

	/**
	Return the Vendor ID of the process to which the thread belongs.

	If an intended use of this method is to check that the Vendor ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RThread& thread;
		TInt error = thread.VendorId()==KRequiredVendorId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RThread& thread;
		static _LIT_SECURITY_POLICY_V0(myVidPolicy, KRequiredVendorId);
		TBool pass = myVidPolicy().CheckPolicy(thread);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_V0

	@return The Vendor ID.
	@publishedAll
    @released
	*/
	IMPORT_C TVendorId VendorId() const;

	/**
	Check if the process to which the thread belongs has a given capability

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability The capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds the capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the process to which the thread belongs has the capability, EFalse otherwise.
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
	Check if the process to which the thread belongs has both of the given capabilities

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability1 The first capability to test.
	@param aCapability2 The second capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds a capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the process to which the thread belongs has both the capabilities, EFalse otherwise.
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
	inline TBool HasCapability(TCapability aCapability, TCapability aCapability2, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	/** Function only temporarily supported to aid migration to process emulation...

	@publishedAll
	@deprecated Use process emulation instead
	*/
	inline TInt Create(const TDesC& aName,TThreadFunction aFunction,TInt aStackSize,TAny* aPtr,RLibrary* aLibrary,RHeap* aHeap, TInt aHeapMinSize,TInt aHeapMaxSize,TOwnerType aType);

private:
	// Implementations of functions with diagnostics
	IMPORT_C TBool DoHasCapability(TCapability aCapability, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability1, TCapability aCapability2) const;
	};

/**
@publishedAll
@deprecated
*/
inline TInt RThread::Create(const TDesC& /*aName*/,TThreadFunction /*aFunction*/,TInt /*aStackSize*/,TAny* /*aPtr*/,RLibrary* /*aLibrary*/,RHeap* /*aHeap*/, TInt /*aHeapMinSize*/,TInt /*aHeapMaxSize*/,TOwnerType /*aType*/)
	{return KErrNotSupported; }



/**
@publishedAll
@released

Encapsulates the Id of a process.

An object of this type is not explicitly constructed in open code,
but is returned by the Id() member function of a process handle,
an RProcess type.

@see RProcess
*/
class TProcessId : public TObjectId
	{
public:
	inline TProcessId();
	inline TProcessId(TUint64 anId);
	};




class RSubSessionBase;

/** 
@publishedAll
@released

A handle to a process.

The process itself is a kernel object.
*/
class RProcess : public RHandleBase
	{
public:
	inline RProcess();
	IMPORT_C TInt Create(const TDesC& aFileName,const TDesC& aCommand,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Create(const TDesC& aFileName,const TDesC& aCommand,const TUidType &aUidType, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(const TDesC& aName,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TProcessId aId,TOwnerType aType=EOwnerProcess);
	IMPORT_C TUidType Type() const;
	IMPORT_C TProcessId Id() const;
	/**
	@publishedAll
	@deprecated Use User::RenameProcess() instead
	*/
	inline static TInt RenameMe(const TDesC& aName);

	IMPORT_C void Kill(TInt aReason);
	IMPORT_C void Terminate(TInt aReason);
	IMPORT_C void Panic(const TDesC& aCategory,TInt aReason);
	IMPORT_C void Resume();
	IMPORT_C TFileName FileName() const;
	IMPORT_C TExitType ExitType() const;
	IMPORT_C TInt ExitReason() const;
	IMPORT_C TExitCategoryName ExitCategory() const;
	IMPORT_C TProcessPriority Priority() const;
	IMPORT_C TInt SetPriority(TProcessPriority aPriority) const;
    IMPORT_C TBool JustInTime() const;
    IMPORT_C void SetJustInTime(TBool aBoolean) const; 
	IMPORT_C void Logon(TRequestStatus& aStatus) const;
	IMPORT_C TInt LogonCancel(TRequestStatus& aStatus) const;
	IMPORT_C TInt GetMemoryInfo(TModuleMemoryInfo& aInfo) const;
	inline TInt Open(const TFindProcess& aFind,TOwnerType aType=EOwnerProcess);
	IMPORT_C void Rendezvous(TRequestStatus& aStatus) const;
	IMPORT_C TInt RendezvousCancel(TRequestStatus& aStatus) const;
	IMPORT_C static void Rendezvous(TInt aReason);
	IMPORT_C TBool DefaultDataPaged() const;

	/**
	Return the Secure ID of the process.

	If an intended use of this method is to check that the Secure ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RProcess& process;
		TInt error = process.SecureId()==KRequiredSecureId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RProcess& process;
		static _LIT_SECURITY_POLICY_S0(mySidPolicy, KRequiredSecureId);
		TBool pass = mySidPolicy().CheckPolicy(process);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_S0

	@return The Secure ID.
	@publishedAll
	@released
	*/
	IMPORT_C TSecureId SecureId() const;

	/**
	Return the Vendor ID of the process.

	If an intended use of this method is to check that the Vendor ID is
	a given value, then the use of a TSecurityPolicy object should be
	considered. E.g. Instead of something like:

	@code
		RProcess& process;
		TInt error = process.VendorId()==KRequiredVendorId ? KErrNone : KErrPermissionDenied;
	@endcode

	this could be used;

	@code
		RProcess& process;
		static _LIT_SECURITY_POLICY_V0(myVidPolicy, KRequiredVendorId);
		TBool pass = myVidPolicy().CheckPolicy(process);
	@endcode

	This has the benefit that the TSecurityPolicy::CheckPolicy methods are
	configured by the system wide Platform Security configuration. I.e. are
	capable of emitting diagnostic messages when a check fails and/or the
	check can be forced to always pass.

	@see TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic) const
	@see _LIT_SECURITY_POLICY_V0

	@return The Vendor ID.
	@publishedAll
    @released
	*/
	IMPORT_C TVendorId VendorId() const;

	/**
	Check if the process has a given capability

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability The capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds the capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the process has the capability, EFalse otherwise.
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
	Check if the process has both of the given capabilities

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability1 The first capability to test.
	@param aCapability2 The second capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds a capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the process has both the capabilities, EFalse otherwise.
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
	inline TBool HasCapability(TCapability aCapability, TCapability aCapability2, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	IMPORT_C TInt SetParameter(TInt aIndex,  RHandleBase aHandle);
	IMPORT_C TInt SetParameter(TInt aSlot, const RSubSessionBase& aSession);
	IMPORT_C TInt SetParameter(TInt aSlot, const TDesC16& aDes);
	IMPORT_C TInt SetParameter(TInt aSlot, const TDesC8& aDes);
	IMPORT_C TInt SetParameter(TInt aSlot, TInt aData);
	inline RProcess(TInt aHandle);

	/**
	@deprecated Use RProcess::SecureId() instead
	*/
	inline TUid Identity() const { return SecureId(); }

	/**
	Legacy Platform Security development and migration support
	@internalAll
	@deprecated No replacement
	*/
	enum TSecureApi { ESecureApiOff, ESecureApiOn, ESecureApiQuery }; // __SECURE_API__ remove this

	/**
	Legacy Platform Security development and migration support
	@internalAll
	@deprecated No replacement
	*/
	IMPORT_C TInt SecureApi(TInt aState); // __SECURE_API__ remove this

	/**
	Legacy Platform Security development and migration support
	@internalAll
	@deprecated No replacement
	*/
	enum TDataCaging { EDataCagingOff, EDataCagingOn, EDataCagingQuery}; // __DATA_CAGING__ __SECURE_API__ remove this

	/**
	Legacy Platform Security development and migration support
	@internalAll
	@deprecated No replacement
	*/
	IMPORT_C TInt DataCaging(TInt aState); // __DATA_CAGING__ __SECURE_API__ remove this
	

	IMPORT_C TInt CreateWithStackOverride(const TDesC& aFileName,const TDesC& aCommand, const TUidType &aUidType, TInt aMinStackSize, TOwnerType aType);

	IMPORT_C static TAny* ExeExportData(void);

private:
	// Implementations of functions with diagnostics
	IMPORT_C TBool DoHasCapability(TCapability aCapability, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const;
	IMPORT_C TBool DoHasCapability(TCapability aCapability1, TCapability aCapability2) const;
	};









/**
@internalTechnology
*/
class RServer2 : public RHandleBase
	{
public:
	IMPORT_C TInt CreateGlobal(const TDesC& aName);
	IMPORT_C TInt CreateGlobal(const TDesC& aName, TInt aMode);
	IMPORT_C TInt CreateGlobal(const TDesC& aName, TInt aMode, TInt aRole, TInt aOpts);
	IMPORT_C void Receive(RMessage2& aMessage,TRequestStatus& aStatus);
	IMPORT_C void Receive(RMessage2& aMessage);
	IMPORT_C void Cancel();
	};




/**
@publishedAll
@released

Client-side handle to a session with a server.

This is the client-side interface through which communication with the server
is channelled.

Clients normally define and implement a derived class to provide
a richer interface.
*/
class RSessionBase : public RHandleBase
	{
	friend class RSubSessionBase;
public:
    /**
    Indicates whether or not threads in the process are automatically attached
    to the session when passed as a parameter to the Share() function.
    */
	enum TAttachMode {EExplicitAttach,EAutoAttach};
public:
	/**
	Creates a session that can be shared by other threads in the current
    process.
    
    After calling this function the session object may be used by threads other
    than than the one that created it.
    
    Note that this can only be done with servers that mark their sessions
    as sharable.
    
    @return	KErrNone, if the session is successfully shared;
	        KErrNoMmemory, if the attempt fails for lack of memory.

    @panic	KERN-EXEC 23 The session cannot be shared.
    
    @see CServer2
    @see RSessionBase::ShareProtected()
    @see CServer2::TServerType
	*/
	inline TInt ShareAuto()	{ return DoShare(EAutoAttach); }


    /**
    Creates a session handle that can be be passed via IPC to another process
    as well as being shared by other threads in the current process.
    
    After calling this function the session object may be used by threads other
    than than the one that created it.

    Note that this can only be done with servers that mark their sessions
    as globally sharable.
    
    @return	KErrNone, if the session is successfully shared;
	        KErrNoMmemory, if the attempt fails for lack of memory.
   
    @panic	KERN-EXEC 23 The session cannot be shared.
    
    @see CServer2
    @see RSessionBase::ShareAuto()
    @see CServer2::TServerType
    */
	inline TInt ShareProtected() { return DoShare(EAutoAttach|KCreateProtectedObject); }


	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(RMessagePtr2 aMessage,TInt aParam,const TSecurityPolicy& aServerPolicy,TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	IMPORT_C TInt Open(TInt aArgumentIndex, const TSecurityPolicy& aServerPolicy, TOwnerType aType=EOwnerProcess);
	inline TInt SetReturnedHandle(TInt aHandleOrError);
	IMPORT_C TInt SetReturnedHandle(TInt aHandleOrError,const TSecurityPolicy& aServerPolicy);
protected:
	inline TInt CreateSession(const TDesC& aServer,const TVersion& aVersion);
	IMPORT_C TInt CreateSession(const TDesC& aServer,const TVersion& aVersion,TInt aAsyncMessageSlots);
	IMPORT_C TInt CreateSession(const TDesC& aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TIpcSessionType aType,const TSecurityPolicy* aPolicy=0, TRequestStatus* aStatus=0);
	inline TInt CreateSession(RServer2 aServer,const TVersion& aVersion);
	IMPORT_C TInt CreateSession(RServer2 aServer,const TVersion& aVersion,TInt aAsyncMessageSlots);
	IMPORT_C TInt CreateSession(RServer2 aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TIpcSessionType aType,const TSecurityPolicy* aPolicy=0, TRequestStatus* aStatus=0);
	inline static TInt SetReturnedHandle(TInt aHandleOrError,RHandleBase& aHandle);

	/**
	@deprecated Use CreateSession(const TDesC& aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TIpcSessionType aType,const TSecurityPolicy* aPolicy=0, TRequestStatus* aStatus=0);
	*/
	inline TInt CreateSession(const TDesC& aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TRequestStatus* aStatus)
		{ return CreateSession(aServer, aVersion, aAsyncMessageSlots, EIpcSession_Unsharable, (TSecurityPolicy*)0, aStatus); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs) const;
	inline void SendReceive(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus) const;
	inline TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;
	inline TInt Send(TInt aFunction) const;
	inline void SendReceive(TInt aFunction,TRequestStatus& aStatus) const;
	inline TInt SendReceive(TInt aFunction) const;
private:
	IMPORT_C TInt DoSend(TInt aFunction,const TIpcArgs* aArgs) const;
	IMPORT_C void DoSendReceive(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus& aStatus) const;
	IMPORT_C TInt DoSendReceive(TInt aFunction,const TIpcArgs* aArgs) const;
	TInt SendAsync(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus* aStatus) const;
	TInt SendSync(TInt aFunction,const TIpcArgs* aArgs) const;
	IMPORT_C TInt DoShare(TInt aAttachMode);
	TInt DoConnect(const TVersion &aVersion,TRequestStatus* aStatus);
	};




/**
@publishedAll
@released

Client-side handle to a sub-session. 

It represents a client-side sub-session, and has a corresponding sub-session
object on the server-side.

Clients normally define and implement a derived class to provide a richer
interface. In particular, a derived class should:

1. provide a function to create a new sub-session with the server;
   this should call CreateSubSession().

2. provide a function to close the current sub-session;
   this should call CloseSubSession().

A session must already exist with a server before a client can establish
any sub-sessions.
*/
class RSubSessionBase
	{
public:
	inline TInt SubSessionHandle() const;
protected:
	inline RSubSessionBase();
	IMPORT_C const RSessionBase Session() const;
	inline TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs);
	inline TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction);
	IMPORT_C TInt CreateAutoCloseSubSession(RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs);
	IMPORT_C void CloseSubSession(TInt aFunction);
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs) const;
	inline void SendReceive(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus) const;
	inline TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;
	inline TInt Send(TInt aFunction) const;
	inline void SendReceive(TInt aFunction,TRequestStatus& aStatus) const;
	inline TInt SendReceive(TInt aFunction) const;
private:
	IMPORT_C TInt DoCreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs* aArgs);
	IMPORT_C TInt DoSend(TInt aFunction,const TIpcArgs* aArgs) const;
	IMPORT_C void DoSendReceive(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus& aStatus) const;
	IMPORT_C TInt DoSendReceive(TInt aFunction,const TIpcArgs* aArgs) const;
	TInt DoCreateSubSession(RSessionBase& aSession,TInt aFunction,const TIpcArgs* aArgs, TBool aAutoClose);
private:
	RSessionBase iSession;
	TInt iSubSessionHandle;
	};




/**
@publishedAll
@released

Base class that provides an implementation for the templated
RRef class.

@see RRef
*/
class RRefBase
	{
public:
	IMPORT_C void Free();
protected:
	inline RRefBase();
	inline RRefBase(const RRefBase& aRef);
	IMPORT_C void DoAlloc(const TAny* aPtr,TInt aSize);
	IMPORT_C void DoAllocL(const TAny* aPtr,TInt aSize);
	IMPORT_C void Copy(const RRefBase& aRef);
private:
	IMPORT_C void operator=(const RRefBase& aRef);
protected:
	TInt* iPtr;
	};




/**
@publishedAll
@released

Contains, or packages, a copy of an instance of another class.

The template parameter defines the type of the contained object.

The contained object is held in allocated memory, and can be accessed
through the member selection and dereference operators.
*/
template <class T>
class RRef : public RRefBase
	{
public:
	inline RRef();
	inline RRef(const RRef<T>& anObject);
	inline void operator=(const RRef<T>& anObject);
	inline T* operator->();
	inline operator T*();
	inline void Alloc(const T& anObject);
	inline void Alloc(const T& anObject,TInt aSize);
	inline void AllocL(const T& anObject);
	inline void AllocL(const T& anObject,TInt aSize);
	};




/**
@publishedAll
@released

A handle to a change notifier. 

The change notifier itself is a kernel object.
*/
class RChangeNotifier : public RHandleBase
	{
public:
	IMPORT_C TInt Create();
	IMPORT_C TInt Logon(TRequestStatus& aStatus) const;
	IMPORT_C TInt LogonCancel() const;
	};




/**
@publishedAll
@released

Handle to a thread death notifier. 

The notifier allows threads to be notified of the death of another thread. 

The thread-death notifier itself is a kernel object.
*/
class RUndertaker : public RHandleBase
	{
public:
	IMPORT_C TInt Create();
	IMPORT_C TInt Logon(TRequestStatus& aStatus,TInt& aThreadHandle) const;
	IMPORT_C TInt LogonCancel() const;
	};





class HBufC16;
/**
@publishedAll
@released

A handle to a session with the extended notifier server that provides support
for plug-in notifiers.

The interface allows engines or other low level components
to communicate with the UI.
*/
class RNotifier : public RSessionBase
	{
public:
	IMPORT_C RNotifier();
	IMPORT_C TInt Connect();
	IMPORT_C void Close();
	IMPORT_C TInt StartNotifier(TUid aNotifierUid,const TDesC8& aBuffer);
	IMPORT_C TInt StartNotifier(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C TInt StartNotifier(TUid aNotifierDllUid,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C TInt CancelNotifier(TUid aNotifierUid);
	IMPORT_C TInt UpdateNotifier(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C void UpdateNotifierAndGetResponse(TRequestStatus& aRs,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C void StartNotifierAndGetResponse(TRequestStatus& aRs,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C void StartNotifierAndGetResponse(TRequestStatus& aRs,TUid aNotifierDllUid,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	IMPORT_C TInt UnloadNotifiers(TUid aNotifierUid);
	IMPORT_C TInt LoadNotifiers(TUid aNotifierUid);
	IMPORT_C void Notify(const TDesC& aLine1,const TDesC& aLine2,const TDesC& aBut1,const TDesC& aBut2,TInt& aButtonVal,TRequestStatus& aStatus);
	IMPORT_C void NotifyCancel();
	IMPORT_C TInt InfoPrint(const TDesC& aDes);
private:
	TPtr8 iButtonVal;
	HBufC16* iCombinedBuffer;
	};

/**
@publishedAll
@released

Abstract class that defines a handler to work with the TRAP mechanism.

Symbian OS provides a trap handler and this class does not normally need to be
used or accessed directly by applications and third party code.
*/
class TTrapHandler
	{
public:
	IMPORT_C TTrapHandler();
	
	/**
	Called when a TRAP is invoked.
	*/
	IMPORT_C virtual void Trap()=0;
	
	/**
	Called when a function exits a TRAP without leaving.
    */
	IMPORT_C virtual void UnTrap()=0;
	
	/**
	Called when a function within a TRAP leaves.

    @param aValue The leave value.
	*/
	IMPORT_C virtual void Leave(TInt aValue)=0;
	};




struct TCollationMethod; // forward declaration




/**
@publishedAll
@released

Contains a set of static functions which perform manipulation of
data in memory.

The arguments passed to the functions of this class are pointers to memory 
locations and length values. These functions are, therefore, not normally 
used in open code but are suitable for implementing data manipulation for 
other classes. Typically the interface provided by such classes is typesafe 
and hides this direct memory to memory manipulation.
*/
class Mem
	{
public:
	inline static TUint8* Copy(TAny* aTrg, const TAny* aSrc, TInt aLength);
	inline static TUint8* Move(TAny* aTrg, const TAny* aSrc, TInt aLength);
	inline static void Fill(TAny* aTrg, TInt aLength, TChar aChar);
	inline static void FillZ(TAny* aTrg, TInt aLength);
#ifndef __GCC32__
	inline static TInt Compare(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL);
#else
	IMPORT_C static TInt Compare(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL);
#endif

	IMPORT_C static TInt Compare(const TUint16* aLeft, TInt aLeftL, const TUint16* aRight, TInt aRightL);
	IMPORT_C static TInt CompareF(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL);
	IMPORT_C static TInt CompareF(const TUint16* aLeft, TInt aLeftL, const TUint16* aRight, TInt aRightL);
	IMPORT_C static TInt CompareC(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL);
	IMPORT_C static TInt CompareC(const TUint16* aLeft, TInt aLeftL, const TUint16* aRight, TInt aRightL);
	IMPORT_C static TInt CompareC(const TUint16* aLeft, TInt aLeftL, const TUint16* aRight, TInt aRightL,
								  TInt aMaxLevel, const TCollationMethod* aCollationMethod);
	IMPORT_C static TInt CollationMethods();
	IMPORT_C static TUint CollationMethodId(TInt aIndex);
	IMPORT_C static const TCollationMethod* CollationMethodByIndex(TInt aIndex);
	IMPORT_C static const TCollationMethod* CollationMethodById(TUint aId);
	IMPORT_C static const TCollationMethod* GetDefaultMatchingTable();
	IMPORT_C static void Swap(TAny* aPtr1, TAny* aPtr2, TInt aLength);
	IMPORT_C static void Crc(TUint16& aCrc, const TAny* aPtr, TInt aLength);
	IMPORT_C static void Crc32(TUint32& aCrc, const TAny* aPtr, TInt aLength);
	};





/**
@publishedAll
@released

Set of static user functions.

These functions are related to a number of System component APIs.

The majority of the functions are related to either the current thread, or 
its heap. Examples in this category include User::Exit(), which causes the 
thread to terminate, and User::Alloc(), which allocates memory from the current 
thread's heap.

Some of these functions are equivalent to functions in the RThread or RHeap 
classes. In these cases, the User function is a convenient way to access the 
function without first having to get a handle to the current thread.

Functions are also provided to support debugging of memory leaks. These function 
calls can be written explicitly or can be generated using a corresponding 
macro - the advantage of using a macro is that the function call is only 
generated for debug builds.

A final category of functions, which includes User::BinarySearch() and User::QuickSort(), 
are just useful functions which have no other natural home.

@see RThread
@see RHeap
*/
class User : public UserHeap
    {
public:
    // Execution control
	IMPORT_C static void InitProcess();			/**< @internalComponent */
    IMPORT_C static void Exit(TInt aReason);
    IMPORT_C static void Panic(const TDesC& aCategory,TInt aReason);
    IMPORT_C static void HandleException(TAny* aInfo);	/**< @internalComponent */
    // Cleanup support
    IMPORT_C static void Leave(TInt aReason);
    IMPORT_C static void LeaveNoMemory();
    IMPORT_C static TInt LeaveIfError(TInt aReason);
    IMPORT_C static TAny* LeaveIfNull(TAny* aPtr);
    inline static const TAny* LeaveIfNull(const TAny* aPtr);
    IMPORT_C static TTrapHandler* SetTrapHandler(TTrapHandler* aHandler);
    IMPORT_C static TTrapHandler* TrapHandler();
    IMPORT_C static TTrapHandler* MarkCleanupStack();   /**< @internalComponent */
    IMPORT_C static void UnMarkCleanupStack(TTrapHandler* aHandler);   /**< @internalComponent */
	IMPORT_C static void LeaveEnd();	/**< @internalComponent */
    // Infoprint
    IMPORT_C static TInt InfoPrint(const TDesC& aDes);
    // Asynchronous service support
    IMPORT_C static void RequestComplete(TRequestStatus*& aStatus,TInt aReason);
    IMPORT_C static void WaitForAnyRequest();
    IMPORT_C static void WaitForRequest(TRequestStatus& aStatus); 
    IMPORT_C static void WaitForRequest(TRequestStatus& aStatus1,TRequestStatus& aStatus2);
    IMPORT_C static void WaitForNRequest(TRequestStatus *aStatusArray[], TInt aNum);
    // User heap management
    IMPORT_C static TInt AllocLen(const TAny* aCell); 
    IMPORT_C static TAny* Alloc(TInt aSize);
    IMPORT_C static TAny* AllocL(TInt aSize); 
    IMPORT_C static TAny* AllocLC(TInt aSize);
    IMPORT_C static TAny* AllocZ(TInt aSize);
    IMPORT_C static TAny* AllocZL(TInt aSize); 
    IMPORT_C static TInt AllocSize(TInt& aTotalAllocSize); 
    IMPORT_C static TInt Available(TInt& aBiggestBlock); 
    IMPORT_C static TInt CountAllocCells();
    IMPORT_C static TInt CountAllocCells(TInt& aFreeCount); 
    IMPORT_C static void Free(TAny* aCell);
    IMPORT_C static void FreeZ(TAny*& aCell); 
    IMPORT_C static RAllocator& Allocator();
    inline static RHeap& Heap();
    IMPORT_C static TAny* ReAlloc(TAny* aCell, TInt aSize, TInt aMode=0);
    IMPORT_C static TAny* ReAllocL(TAny* aCell, TInt aSize, TInt aMode=0);
    IMPORT_C static RAllocator* SwitchAllocator(RAllocator* aAllocator);
	inline static RHeap* SwitchHeap(RAllocator* aHeap);
	IMPORT_C static TInt CompressAllHeaps();
    // Synchronous timer services
    IMPORT_C static void After(TTimeIntervalMicroSeconds32 aInterval);
    IMPORT_C static TInt At(const TTime& aTime);
    IMPORT_C static void AfterHighRes(TTimeIntervalMicroSeconds32 aInterval);
    // Set time and deal with timezones
    IMPORT_C static TInt SetHomeTime(const TTime& aTime);
    IMPORT_C static TInt SetHomeTimeSecure(const TTime& aTime);
	IMPORT_C static TInt SetUTCTime(const TTime& aUTCTime);
	IMPORT_C static TInt SetUTCTimeSecure(const TTime& aUTCTime);
	IMPORT_C static TTimeIntervalSeconds UTCOffset();
	IMPORT_C static void SetUTCOffset(TTimeIntervalSeconds aOffset);
	IMPORT_C static TInt SetUTCTimeAndOffset(const TTime& aUTCTime, TTimeIntervalSeconds aOffset);
    // Set locale information
    IMPORT_C static TInt SetCurrencySymbol(const TDesC& aSymbol);
	// Set floating point mode
	IMPORT_C static TInt SetFloatingPointMode(TFloatingPointMode aMode, TFloatingPointRoundingMode aRoundingMode=EFpRoundToNearest);
	// Timers
	IMPORT_C static TUint TickCount();
	IMPORT_C static TUint32 NTickCount();
	IMPORT_C static TTimerLockSpec LockPeriod();
	IMPORT_C static TTimeIntervalSeconds InactivityTime();
	IMPORT_C static void ResetInactivityTime();
	IMPORT_C static TUint32 FastCounter();
	// Atomic operations
	IMPORT_C static TInt LockedInc(TInt& aValue);
	IMPORT_C static TInt LockedDec(TInt& aValue);
	IMPORT_C static TInt SafeInc(TInt& aValue);
	IMPORT_C static TInt SafeDec(TInt& aValue);
    // Beep
    IMPORT_C static TInt Beep(TInt aFrequency,TTimeIntervalMicroSeconds32 aDuration); 
    // Information
    IMPORT_C static TInt IsRomAddress(TBool& aBool,TAny* aPtr);
    // Algorithms
    IMPORT_C static TInt BinarySearch(TInt aCount,const TKey& aKey,TInt& aPos);
    IMPORT_C static TInt QuickSort(TInt aCount,const TKey& aKey,const TSwap& aSwap);
    // Language-dependent character functions 
    IMPORT_C static TLanguage Language();
    IMPORT_C static TRegionCode RegionCode();
    IMPORT_C static TUint Collate(TUint aChar); 
    IMPORT_C static TUint Fold(TUint aChar); 
    IMPORT_C static TUint LowerCase(TUint aChar); 
    IMPORT_C static TUint UpperCase(TUint aChar); 
	IMPORT_C static TUint Fold(TUint aChar,TInt aFlags);
	IMPORT_C static TUint TitleCase(TUint aChar);
    // C-style string length
    IMPORT_C static TInt StringLength(const TUint8* aString); 
    IMPORT_C static TInt StringLength(const TUint16* aString);
    // Device management
    IMPORT_C static TInt FreeLogicalDevice(const TDesC& aDeviceName); 
	IMPORT_C static TInt FreePhysicalDevice(const TDesC& aDriverName); 
    IMPORT_C static TInt LoadLogicalDevice(const TDesC& aFileName); 
    IMPORT_C static TInt LoadPhysicalDevice(const TDesC& aFileName); 
    // Version information
    IMPORT_C static TBool QueryVersionSupported(const TVersion& aCurrent,const TVersion& aRequested);
    IMPORT_C static TVersion Version();
    // Machine configuration
    IMPORT_C static TInt SetMachineConfiguration(const TDesC8& aConfig);
    IMPORT_C static TInt MachineConfiguration(TDes8& aConfig,TInt& aSize);
    // Debugging support
    IMPORT_C static void SetDebugMask(TUint32 aVal);
    IMPORT_C static void SetDebugMask(TUint32 aVal, TUint aIndex);
    IMPORT_C static void SetJustInTime(const TBool aBoolean); 
    IMPORT_C static void Check();
    IMPORT_C static void Invariant();
    IMPORT_C static TBool JustInTime();
    IMPORT_C static void __DbgMarkStart(TBool aKernel);
    IMPORT_C static void __DbgMarkCheck(TBool aKernel, TBool aCountAll, TInt aCount, const TUint8* aFileName, TInt aLineNum);
    IMPORT_C static TUint32 __DbgMarkEnd(TBool aKernel, TInt aCount);
    IMPORT_C static void __DbgSetAllocFail(TBool aKernel, RAllocator::TAllocFail aFail, TInt aRate);
    IMPORT_C static void __DbgSetBurstAllocFail(TBool aKernel, RAllocator::TAllocFail aFail, TUint aRate, TUint aBurst);
	IMPORT_C static TUint __DbgCheckFailure(TBool aKernel);
	IMPORT_C static void PanicUnexpectedLeave(); /**< @internalComponent */
    // Name Validation
    IMPORT_C static TInt ValidateName(const TDesC& aName);
	// Instruction Memory Barrier
	IMPORT_C static void IMB_Range(TAny* aStart, TAny* aEnd);
	//
	IMPORT_C static TInt CommandLineLength();
	IMPORT_C static void CommandLine(TDes &aCommand);
	IMPORT_C static TExceptionHandler ExceptionHandler();
	IMPORT_C static TInt SetExceptionHandler(TExceptionHandler aHandler,TUint32 aMask);
	IMPORT_C static void ModifyExceptionMask(TUint32 aClearMask, TUint32 aSetMask);
	IMPORT_C static TInt RaiseException(TExcType aType);
	IMPORT_C static TBool IsExceptionHandled(TExcType aType);

	/**
	A set of values that defines the effect that terminating a thread 
	has, either on its owning process or on the whole system.
	
	A thread is said to be critical if its owning process or the entire system
	terminates when the thread itself terminates. 
	
	You pass one of these values to the functions:
	- User::SetCritical()
	- User::SetProcessCritical()
	
	The meaning of a value when passed to one function is different to
	its meaning when passed the other function. See the description of each
	individual value.
			
	@see User::SetCritical()
	@see User::SetProcessCritical()
	*/
	enum TCritical {
	
	
	               /**
                   This value can be passed to both:
                   - User::SetCritical(), which means that the current thread
                   is no longer critical, i.e. termination of the current
                   thread will no longer cause termination of the current thread's
                   owning process (i.e. the current process) or a reboot of the system.
                   - User::SetProcessCritical(), which means that threads
                   subsequently created in the current thread's owning
                   process (i.e. the current process) will no longer cause termination of that
                   process or a reboot of the system. Note, however, that existing
                   threads are NOT affected when you call this function.
                   
                   @see User::SetCritical()
                   @see User::SetProcessCritical()
                   */
                   ENotCritical, 
                   
                                      
                   /**
                   This value can only be passed to User::SetCritical() and
                   affects the current thread only.
                   
                   It means that the owning process (i.e.the current process)
                   terminates if:
                   - the current thread is terminated.
                   - the current thread panics.
                   
                   @see User::SetCritical()
                   */	
	               EProcessCritical,

	               
	               /**
                   This value can only be passed to User::SetCritical() and
                   affects the current thread only.
                   
                   It means that the owning process (i.e.the current process)
                   terminates if the current thread terminates for any reason.
                   
                   @see User::SetCritical()
                   */
	               EProcessPermanent,
	               
	               
	               /**
	               This value can only be passed to User::SetProcessCritical() and
                   affects any new threads created in the current process.
	               
	               It means that the current process terminates if:
	               - any new thread subsequently created in the current process is terminated.
	               - any new thread subsequently created in the current process panics.
	               .
	               Note, however, that existing threads in the current process
	               are NOT affected when you call User::SetProcessCritical()
	               with this value.
	               	               
	               @see EProcessCritical
                   @see User::SetProcessCritical()
	               */
	               EAllThreadsCritical,
	                	                
	                
	               /**
	               This value can be passed to both: User::SetCritical() and
	               User::SetProcessCritical().
                   
                   When passed to User::SetCritical(), it means that
                   the entire system is rebooted if:
                   - the current thread is terminated.
                   - the current thread panics.
                   
                   When passed to User::SetProcessCritical(), it means that
                   the entire system is rebooted if:
                   - any new thread subsequently created in the current process is terminated.
                   - any new thread subsequently created in the current process panics.
                   - the process itself is terminated
                   - the process itself panics
	               
	               Note:
                   -# existing threads in the current process are NOT affected when you
                   call User::SetProcessCritical() with this value.
                   -# Only a process with 'Protected Server' capability can set a
                   thread to system-critical.
                   
                   @see User::SetCritical()
                   @see User::SetProcessCritical()
	               */
	               ESystemCritical,
	               
	               
	               /**
	               This value can be passed to both: User::SetCritical()
	               and User::SetProcessCritical().
                   
                   When passed to User::SetCritical(), it means that
                   the entire system is rebooted if the current thread
                   exits for any reason.
                   
                   When passed to User::SetProcessCritical(), it means that
                   the entire system is rebooted if any new thread 
                   subsequently created in the current process exits
                   for any reason, or if the process itself exits for any reason.
	               
	               Note:
                   -# existing threads in the current process are NOT affected when you
                   call User::SetProcessCritical() with this value.
                   -# Only a process with 'Protected Server' capability can set a
                   thread to system-permanent.
                   
                   @see User::SetCritical()
                   @see User::SetProcessCritical()
	               */
	               ESystemPermanent
	               };
	IMPORT_C static TCritical Critical();
	IMPORT_C static TCritical Critical(RThread aThread);
	IMPORT_C static TInt SetCritical(TCritical aCritical);
	IMPORT_C static TCritical ProcessCritical();
	IMPORT_C static TCritical ProcessCritical(RProcess aProcess);
	IMPORT_C static TInt SetProcessCritical(TCritical aCritical);
	IMPORT_C static TBool PriorityControl();
	IMPORT_C static void SetPriorityControl(TBool aEnable);

	/**
	A threads realtime state.
	Some non-realtime behaviour can be detected by the kernel. When it does so,
	action is taken depending on the thread state:
	-	ERealtimeStateOff - no action.
	-	ERealtimeStateOn - the the thread will be panicked with KERN-EXEC 61 (EIllegalFunctionForRealtimeThread).
	-	ERealtimeStateWarn - no action. However, if the kernel trace flag KREALTIME is enabled
							 then tracing will be emitted as if the thread state was ERealtimeStateOn.
	@publishedPartner
	@released
	*/
	enum TRealtimeState
		{
		ERealtimeStateOff,	/**< Thread is not realtime */
		ERealtimeStateOn,	/**< Thread is realtime */
		ERealtimeStateWarn	/**< Thread is realtime but doesn't want this enforced */
		};

	/**
	Set the current threads realtime state.
	@see TRealtimeState
	@param aState The state
	@return KErrNone if successful. KErrArgument if aState is invalid.
	@publishedPartner
	@released
	*/
	IMPORT_C static TInt SetRealtimeState(TRealtimeState aState);

	/**
	Return the Secure ID of the process that created the current process.
	@return The Secure ID.
	@publishedAll
	@released
	*/
	IMPORT_C static TSecureId CreatorSecureId();

	/**
	Return the Vendor ID of the process that created the current process.
	@return The Vendor ID.
	@publishedAll
	@released
	*/
	IMPORT_C static TVendorId CreatorVendorId();

	/**
	Check if the process that created the current process has a given capability

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability The capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds the capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the creator process has the capability, EFalse otherwise.
	@publishedAll
	@released
	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline static TBool CreatorHasCapability(TCapability aCapability, const char* aDiagnostic=0);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline static TBool CreatorHasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic=NULL);
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline static TBool CreatorHasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress);
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	/**
	Check if the process that created the current process has both of the given capabilities

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

	@param aCapability1 The first capability to test.
	@param aCapability2 The second capability to test.
	@param aDiagnostic A string that will be emitted along with any diagnostic message
								that may be issued if the test finds a capability is not present.
								This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
								which enables it to be easily removed from the system.
	@return ETrue if the creator process has both the capabilities, EFalse otherwise.
	@publishedAll
	@released
	*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline static TBool CreatorHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic=0);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline static TBool CreatorHasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull aDiagnostic=NULL);
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	// For things using KSuppressPlatSecDiagnostic
	inline static TBool CreatorHasCapability(TCapability aCapability, TCapability aCapability2, OnlyCreateWithNull aDiagnostic, OnlyCreateWithNull aSuppress);
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	IMPORT_C static TInt ParameterLength(TInt aSlot);
	IMPORT_C static TInt GetTIntParameter(TInt aSlot, TInt& aData);
	IMPORT_C static TInt GetDesParameter(TInt aSlot, TDes8& aDes);
	IMPORT_C static TInt GetDesParameter(TInt aSlot, TDes16& aDes);
	IMPORT_C static TInt RenameThread(const TDesC &aName);
	IMPORT_C static TInt RenameProcess(const TDesC &aName);
	/*
	User::Identity() has been deprecated and is available for backward
	compatibility purposes only.

	Use RProcess().SecureId() instead.
    
	@deprecated
	*/
	inline static TUid Identity() { return RProcess().SecureId(); }

	/*
	User::CreatorIdentity() has been deprecated and is available for backward
	compatibility purposes only.

	Use CreatorSecureId() instead.
	
	@deprecated
	*/
	static inline TUid CreatorIdentity() { return CreatorSecureId(); }

	IMPORT_C static void NotifyOnIdle(TRequestStatus& aStatus);			/**< @internalTechnology */
	IMPORT_C static void CancelMiscNotifier(TRequestStatus& aStatus);	/**< @internalTechnology */
private:
	// Implementations of functions with diagnostics
	IMPORT_C static TBool DoCreatorHasCapability(TCapability aCapability, const char* aDiagnostic);
	IMPORT_C static TBool DoCreatorHasCapability(TCapability aCapability);
	IMPORT_C static TBool DoCreatorHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic);
	IMPORT_C static TBool DoCreatorHasCapability(TCapability aCapability1, TCapability aCapability2);
	};




class ExecHandler;

/**
@internalComponent
@removed
*/
typedef void (*TTlsCleanupHandler)(TAny*);		//don't use

/**
@publishedAll
@released

A collection of static functions involved in managing access to
thread-local storage. 

Thread-local storage is a single machine word of static writable memory.
The scope of this machine word is the thread, which means that there is one
word per thread. The word is only accessible to code running in a DLL.

In practice, this word is almost always used to hold a pointer to allocated
memory; this makes that memory available to all DLL code running on behalf
of the same thread.

Note that DLL code running on behalf of one thread does not see the same word when
running on behalf of another thread. 

The class in not intended for user derivation.
*/
class Dll
	{
public:
	static TInt SetTls(TAny* aPtr);
	static TAny* Tls();
	static void FreeTls();
	static void FileName(TFileName &aFileName);
	};




#ifndef __TOOLS__
/**
@publishedAll
@released

A thin wrapper class for C++ arrays allowing automatic checking of index values 
to ensure that all accesses are legal. 

The class also supports the deletion of objects.

The class is templated, based on a class type and an integer value. The class 
type defines the type of object contained in the array; the integer value 
defines the size (dimension) of the array.

A wrapper object can be:

1. embedded in objects allocated on the heap.

2. used on the program stack.
*/
template <class T,TInt S> 
class TFixedArray
	{
	typedef TFixedArray<T,S> ThisClass;
public:
	inline TFixedArray();
	inline TFixedArray(const T* aList, TInt aLength);
	//
	inline void Copy(const T* aList, TInt aLength);
	inline void Reset();		// zero fill
	inline void DeleteAll();
	//
	inline TInt Count() const;
	inline TInt Length() const;
	// Accessors - debug range checking
	inline T& operator[](TInt aIndex);
	inline const T& operator[] (TInt aIndex) const;
	// Accessors - always range checking
	inline T& At(TInt aIndex);
	inline const T& At(TInt aIndex) const;
	// Provides pointers to the beginning and end of the array
	inline T* Begin();
	inline T* End();
	inline const T* Begin() const;
	inline const T* End() const;
	//
	inline TArray<T> Array() const;
protected:
	inline static TBool InRange(TInt aIndex);
	inline static TInt CountFunctionR(const CBase* aThis);
	inline static const TAny* AtFunctionR(const CBase* aThis,TInt aIndex);
protected:
	T iRep[S];
	};




/**
@publishedAll
@released
*/
#define DECLARE_ROM_ARRAY( AName, AData, AType ) \
   	const TFixedArray<AType,(sizeof(AData)/sizeof((AData)[0]))>& \
            AName = *(reinterpret_cast<const TFixedArray<AType, \
                           (sizeof(AData)/sizeof((AData)[0]))>* > (AData))
#endif

// Global leaving operator new
/**
@publishedAll
@released
*/
inline TAny* operator new(TUint aSize, TLeave);
/**
@publishedAll
@released
*/
inline TAny* operator new(TUint aSize, TLeave, TUint aExtraSize);
#if !defined(__VC32__) || defined (__MSVCDOTNET__)
/**
@publishedAll
@released
*/
inline TAny* operator new[](TUint aSize, TLeave);
#endif


#ifdef __LEAVE_EQUALS_THROW__
/** Macro to assert in all builds that code does not leave

@param	_s	C++ statements to be executed which should not leave
@panic	USER 194 if the code being checked does leave

@publishedAll
@released
*/
#define	__ASSERT_ALWAYS_NO_LEAVE(_s)	\
	{														\
	try	{													\
		TTrapHandler* ____t = User::MarkCleanupStack();		\
		_s;													\
		User::UnMarkCleanupStack(____t);					\
		}													\
	catch (XLeaveException& /*l*/)							\
		{													\
		User::PanicUnexpectedLeave();						\
		}													\
	catch (...)												\
		{													\
		User::Invariant();									\
		}													\
	}

#else
/** Macro to assert in all builds that code does not leave

@param	_s	C++ statements to be executed which should not leave
@panic	USER 194 if the code being checked does leave

@publishedAll
@released
*/
#define	__ASSERT_ALWAYS_NO_LEAVE(_s)	\
	{									\
	TInt _r;							\
	TTrap _t;							\
	if (_t.Trap(_r) == 0)				\
		{								\
		_s;								\
		TTrap::UnTrap();				\
		}								\
	else								\
		User::PanicUnexpectedLeave();	\
	}
#endif

/** Macro to assert in debug builds that code does not leave

@param	_s	C++ statements to be executed which should not leave
@panic	USER 194 if the code being checked does leave

@publishedAll
@released
*/
#ifdef _DEBUG
#define	__ASSERT_DEBUG_NO_LEAVE(_s)		__ASSERT_ALWAYS_NO_LEAVE(_s)
#else
#define	__ASSERT_DEBUG_NO_LEAVE(_s)		{ _s; }
#endif



// Inline methods
#include <e32std.inl>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32std_private.h>
#endif

#endif

