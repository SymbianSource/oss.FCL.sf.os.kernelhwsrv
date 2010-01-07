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
// e32\include\e32std.inl
// 
//

// Global leaving operator new
inline TAny* operator new(TUint aSize, TLeave)
	{return User::AllocL(aSize);}
inline TAny* operator new(TUint aSize, TLeave, TUint aExtraSize)
	{return User::AllocL(aSize + aExtraSize);}
#if !defined(__VC32__) || defined (__MSVCDOTNET__)
inline TAny* operator new[](TUint aSize, TLeave)
	{return User::AllocL(aSize);}
#endif




// class Mem
inline TUint8* Mem::Copy(TAny* aTrg, const TAny* aSrc, TInt aLength)
/**
Copies data from a source location to a target location and returns a pointer 
to the end of the copied data.
	
The source and target areas can overlap.
	
The copy operation is optimised so that if both source and target locations 
are aligned on a word boundary, the operation performs the copy on a word 
by word basis.
	
@param aTrg    A pointer to the target location for the copy operation. 
@param aSrc    A pointer to the source location for the copy operation. 
@param aLength The number of bytes to be copied. This value must not
               be negative. 

@return A pointer to a location aLength bytes beyond aTrg (i.e. the location 
        aTrg+aLength).

@panic USER 90 In debug builds only, if aLength is negative. 
*/
	{ return (TUint8*)memmove(aTrg, aSrc, aLength) + aLength; }




inline TUint8* Mem::Move(TAny* aTrg, const TAny* aSrc, TInt aLength)
/**
Moves a block of data from a source location to a target location and returns 
a pointer to the end of the moved data.
	
The source and target areas can overlap.
	
Both source and target locations must be aligned on a word boundary. 
The specified length must also be a multiple of 4.
	
@param aTrg    A pointer to the target location for the move operation. This 
               pointer must be word aligned. 
@param aSrc    A pointer to the source location for the move operation. This
               pointer must be word aligned.
@param aLength The number of bytes to be copied. This value must be a multiple 
               of 4.
			   
@return A pointer to a location aLength bytes beyond aTrg (i.e. the location 
        aTrg+aLength).

@panic USER 93 In debug builds only, if aTrg is not word aligned.
@panic USER 92 In debug builds only, if aSrc is not word aligned.
@panic USER 91 In debug builds only, if aLength is not a multiple of 4.
*/
	{ return (TUint8*)wordmove(aTrg, aSrc, aLength) + aLength; }




inline void Mem::Fill(TAny* aTrg, TInt aLength, TChar aChar)
/**
Fills a specified block of data with a specified character, replacing
any existing content.
	
The function assumes that the fill character is a non-Unicode character.
	
@param aTrg    A pointer to the location where filling is to start. 
@param aLength The number of bytes to be filled. This value must not
               be negative. 
@param aChar   The fill character.

@panic USER 95 In debug builds only, if aLength is negative.  
*/
	{ memset(aTrg, (TInt)(aChar.operator TUint()), aLength); }




inline void Mem::FillZ(TAny* aTrg,TInt aLength)
/**
Fills a specified block of data with binary zeroes (i.e. 0x00), replacing any 
existing content.
	
@param aTrg    A pointer to the location where filling is to start. 
@param aLength The number of bytes to be filled. This value must not
               be negative. 
	
@panic USER 95 In debug builds only, if aLength is negative.  
*/
	{ memclr(aTrg, aLength); }




#if !(defined(__GCC32__) && defined(__MARM__))
inline TInt Mem::Compare(const TUint8* aLeft, TInt aLeftL, const TUint8* aRight, TInt aRightL)
/**
Compares a block of data at one specified location with a block of data at 
another specified location.

The comparison proceeds on a byte for byte basis, the result of the comparison 
is based on the difference of the first bytes to disagree.

The data at the two locations are equal if they have the same length and content. 
Where the lengths are different and the shorter section of data is the same 
as the first part of the longer section of data, the shorter is considered 
to be less than the longer.

@param aLeft   A pointer to the first (or left) block of 8 bit data
               to be compared.
@param aLeftL  The length of the first (or left) block of data to be compared,  
               i.e. the number of bytes.
@param aRight  A pointer to the second (or right) block of 8 bit data to be 
               compared.
@param aRightL The length of the second (or right) block of data to be compared 
               i.e. the number of bytes.
               
@return Positive, if the first (or left) block of data is greater than the 
        second (or right) block of data.
        Negative, if the first (or left) block of data is less than the
        second (or right) block of data.
        Zero, if both the first (or left) and second (or right) blocks of data
        have the same length and the same content.
*/
	{ return memcompare(aLeft, aLeftL, aRight, aRightL); }
#endif




// class RHeap
inline TInt RHeap::SetBrk(TInt aBrk)
	{ return ((RChunk*)&iChunkHandle)->Adjust(aBrk); }




// class TChar
#ifndef __KERNEL_MODE__
inline void TChar::SetChar(TUint aChar)
	{iChar=aChar;}




inline void TChar::Fold()
/**
Converts the character to a form which can be used in tolerant comparisons 
without control over the operations performed. 

Tolerant comparisons are those which ignore character differences like case 
and accents.

This function can be used when searching for a string in a text file or a 
file in a directory. Folding performs the following conversions: converts 
to lowercase, strips accents, converts all digits representing the values 
0..9 to the ordinary digit characters '0'..'9', converts all spaces (standard, 
non-break, fixed-width, ideographic, etc.) to the ordinary space character 
(0x0020), converts Japanese characters in the hiragana syllabary to katakana, 
and converts East Asian halfwidth and fullwidth variants to their ordinary 
forms. You can choose to perform any subset of these operations by using the 
other function overload.

@see User::Fold
*/
	{iChar=User::Fold(iChar);}




inline void TChar::LowerCase()
/**
Converts the character to its lowercase form.

Characters lacking a lowercase form are unchanged.

@see User::LowerCase
*/
	{iChar=User::LowerCase(iChar);}




inline void TChar::UpperCase()
/**
Converts the character to its uppercase form.

Characters lacking an uppercase form are unchanged.

@see User::UpperCase
*/
	{iChar=User::UpperCase(iChar);}




#ifdef _UNICODE
inline void TChar::Fold(TInt aFlags)
/**
Converts the character to a form which can be used in tolerant comparisons 
allowing selection of the specific fold operations to be performed.

@param aFlags Flags which define the operations to be performed. The values 
              are defined in the enum beginning with EFoldCase.

@see TChar::EFoldCase
@see User::Fold
*/
	{iChar=User::Fold(iChar,aFlags);}




inline void TChar::TitleCase()
/**
Converts the character to its titlecase form.

The titlecase form of a character is identical to its uppercase form unless 
a specific titlecase form exists. Characters lacking a titlecase form are 
unchanged.
*/
	{iChar=User::TitleCase(iChar);}
#endif




inline TBool TChar::Eos() const
/**
Tests whether the character is the C/C++ end-of-string character - 0.

@return True, if the character is 0; false, otherwise.
*/
	{return(iChar==0);}
#endif // _UNICODE




// Class TCallBack
inline TCallBack::TCallBack()
/**
Default constructor.
	
Sets the function pointer to Null.
*/
	{iFunction=NULL;}




inline TCallBack::TCallBack(TInt (*aFunction)(TAny *aPtr))
	: iFunction(aFunction),iPtr(NULL)
/**
Constructs the callback object with the specified callback function.

@param aFunction A pointer to the callback function. It takes an argument of
                 type TAny* and returns a TInt.
				 It must be either a static member of a class or a function
				 which is not a member of any class. 
*/
	{}




inline TCallBack::TCallBack(TInt (*aFunction)(TAny *aPtr),TAny *aPtr)
	: iFunction(aFunction),iPtr(aPtr)
/**
Constructs the callback object with the specified callback function and
a pointer to any object.

@param aFunction A pointer to the callback function. It takes an argument of
                 type TAny* and returns a TInt.
				 It must be either a static member of a class or a function
				 which is not a member of any class. 
@param aPtr      A pointer which is always passed to the callback function.
*/
	{}




/**
Calls the callback function.
	
@return The value returned by the callback function. The meaning of this value
        depends entirely on the context in which the callback function
        is called.
        For example, when used with the CIdle class, a false (zero) value
        indicates that the callback function should not be 	called again.
        As another example, when used with the CPeriodic class, the return
        value is ignored and is irrelevant in that context.

@see CIdle
@see CPeriodic        
*/
inline TInt TCallBack::CallBack() const
	{ return (iFunction ? (*iFunction)(iPtr) : 0); }




// Class TSglQue
template <class T>
inline TSglQue<T>::TSglQue()
/**
Constructs an empty list header and sets the offset value of the link object 
to zero.

In practice, never assume that the offset of the link object from the start 
of a list element is zero, even if the link object is declared as the first 
data member in the list element class.

If this default constructor is used, then call the SetOffset() function of 
the base class to ensure that the offset value is set correctly.

@see TSglQueBase::SetOffset
*/
	{}




template <class T>
inline TSglQue<T>::TSglQue(TInt aOffset)
	: TSglQueBase(aOffset)
/**
Constructs an empty list header and sets the offset of the link object to the 
specified value.

@param aOffset The offset of the link object from the start of a list element. 
                The macro _FOFF can be used to calculate this value.
				
@panic USER 75, if aOffset is not divisible by four.

@see _FOFF
*/
	{}




template <class T>
inline void TSglQue<T>::AddFirst(T &aRef)
/**
Inserts the specified list element at the front of the singly linked list.

If the list is not empty, the specified element becomes the first in the list. 
What was previously the first element becomes the second in the list.

@param aRef The list element to be inserted at the front of the singly linked 
            list.
*/
	{DoAddFirst(&aRef);}




template <class T>
inline void TSglQue<T>::AddLast(T &aRef)
/**
Inserts the specified list element at the back of the singly linked list.

If the list is not empty, the specified element becomes the last in the list. 
What was previously the last element becomes the next to last element in the 
list.

@param aRef The list element to be inserted at the back of the singly linked 
            list.
*/
	{DoAddLast(&aRef);}




template <class T>
inline TBool TSglQue<T>::IsFirst(const T *aPtr) const
/**
Tests whether the specified element is the first in the singly linked list.

@param aPtr A pointer to the element whose position in the list is to be
            checked.

@return True, if the element is the first in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iHead);}




template <class T>
inline TBool TSglQue<T>::IsLast(const T *aPtr) const
/**
Tests whether the specified element is the last in the singly linked list.

@param aPtr A pointer to the element whose position in the list is 
            to be checked.

@return True, if the element is the last in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iLast);}




template <class T>
inline T *TSglQue<T>::First() const
/**
Gets a pointer to the first list element in the singly linked list.

@return A pointer to the first list element in the singly linked list. If 
        the list is empty, this pointer is not necessarily NULL and must not
		be assumed to point to a valid object.
*/
	{return(PtrSub((T *)iHead,iOffset));}




template <class T>
inline T *TSglQue<T>::Last() const
/**
Gets a pointer to the last list element in the singly linked list.

@return A pointer to the last list element in the singly linked list. If the 
        list is empty, this pointer is not necessarily NULL and must not be
		assumed to point to a valid object.
*/
	{return(PtrSub((T *)iLast,iOffset));}




template <class T>
inline void TSglQue<T>::Remove(T &aRef)
/**
Removes the specified element from the singly linked list.

The singly linked list must not be empty.

@param aRef A list element to be removed from the singly linked list.

@panic USER 76, if the element to be removed is not in the list
*/
	{DoRemove(&aRef);}




// Class TDblQue
template <class T>
inline TDblQue<T>::TDblQue()
/**
Constructs an empty list header and sets the offset value of the link object 
to zero.

In practice, never assume that the offset of the link object from the start 
of a list element is zero, even if the link object is declared as the first 
data member in the list element class.

If this default constructor is used, then call the SetOffset() function of 
the base class to ensure that the offset value is set correctly.

@see TDblQueBase::SetOffset()
*/
	{}




template <class T>
inline TDblQue<T>::TDblQue(TInt aOffset)
	: TDblQueBase(aOffset)
/**
Constructs an empty list header and sets the offset of the link object to the 
specified value.

@param aOffset The offset of the link object from the start of a list element. 
                The macro _FOFF can be used to calculate this value.
				
@panic USER 78. if aOffset is not divisble by 4.
				  
@see _FOFF
*/
	{}




template <class T>
inline void TDblQue<T>::AddFirst(T &aRef)
/**
Inserts the specified list element at the front of the doubly linked list.

If the list is not empty, the specified element becomes the first in the list. 
What was previously the first element becomes the second in the list.

@param aRef The list element to be inserted at the front of the doubly linked 
            list.
*/
	{DoAddFirst(&aRef);}




template <class T>
inline void TDblQue<T>::AddLast(T &aRef)
/**
Inserts the specified list element at the back of the doubly linked list.

If the list is not empty, the specified element becomes the last in the list. 
What was previously the last element becomes the next to last element in the 
list.

@param aRef The list element to be inserted at the back of the doubly linked 
            list.
*/
	{DoAddLast(&aRef);}




template <class T>
inline TBool TDblQue<T>::IsHead(const T *aPtr) const
/**
Tests whether the end of a list has been reached.

A doubly linked list is circular; in following the chain of elements in a 
list (e.g. using the iterator operator++ or operator--), the chain eventually 
reaches the end of the list and aPtr corresponds to the header (although it 
will not point to a valid T object).

@param aPtr The pointer value to be checked. 

@return True, if the end of the list has been reached. False, if the end of 
        the list has not been reached; aPtr points to an element in the list.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)&iHead);}




template <class T>
inline TBool TDblQue<T>::IsFirst(const T *aPtr) const
/**
Tests whether the specified element is the first in the doubly linked list.

@param aPtr A pointer to the element whose position in the list is to be checked.

@return True, if the element is the first in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iHead.iNext);}




template <class T>
inline TBool TDblQue<T>::IsLast(const T *aPtr) const
/**
Tests whether the specified element is the last in the doubly linked list.

@param aPtr A pointer to the element whose position in the list is to be checked.

@return True, if the element is the last in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iHead.iPrev);}




template <class T>
inline T *TDblQue<T>::First() const
/**
Gets a pointer to the first list element in the doubly linked list.

@return A pointer to the first list element in the doubly linked list. If 
        the list is empty, this pointer is not necessarily NULL and must not
		be assumed to point to a valid object.
*/
	{
#if defined (_DEBUG)
	__DbgTestEmpty();
#endif
    return(PtrSub((T *)iHead.iNext,iOffset));
    }




template <class T>
inline T *TDblQue<T>::Last() const
/**
Gets a pointer to the last list element in the doubly linked list.

@return A pointer to the last list element in the doubly linked list. If the 
        list is empty, this pointer is not necessarily NULL and must not be assumed 
        to point to a valid object.
*/
	{
#if defined (_DEBUG)
	__DbgTestEmpty();
#endif
	return(PtrSub((T *)iHead.iPrev,iOffset));
	}




// Class TPriQue
template <class T>
inline TPriQue<T>::TPriQue()
/**
Default constructor.

Constructs an empty list header and sets the offset value of the link
object to zero.

In practice, never assume that the offset of the link object from the start
of a list element is zero, even if the link object is declared as the first
data member in the list element class.

If this default constructor is used, then call the SetOffset() function of
the base class to ensure that the offset value is set correctly.

@see TDblQueBase::SetOffset
*/
	{}




template <class T>
inline TPriQue<T>::TPriQue(TInt aOffset)
	: TDblQueBase(aOffset)
/**
Constructs an empty list header and sets the offset of the link object
to the specified value.

@param aOffset The offset of the link object from the start of a list element.
                The macro _FOFF can be used to calculate this value.
				
@panic USER 78 if aOffset is not divisible by four.		  
*/
	{}




template <class T>
inline void TPriQue<T>::Add(T &aRef)
/**
Inserts the specified list element in descending priority order.

If there is an existing list element with the same priority, then the new
element is added after the existing element.

@param aRef The list element to be inserted.
*/
	{DoAddPriority(&aRef);}




template <class T>
inline TBool TPriQue<T>::IsHead(const T *aPtr) const
/**
Tests whether the end of a list has been reached.

A doubly linked list is circular; in following the chain of elements in a list
(e.g. using the iterator operator++ or operator--), the chain eventually
reaches the end of the list and aPtr corresponds to the header (although it
will not point to a valid T object).

@param aPtr The pointer value to be checked.

@return True, if the end of the list has been reached. False, if the end of the
        list has not been reached; aPtr points to an element in the list.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)&iHead);}




template <class T>
inline TBool TPriQue<T>::IsFirst(const T *aPtr) const
/**
Tests whether the specified element is the first in the linked list.

@param aPtr A pointer to the element whose position in the list is to
            be checked.

@return True, if the element is the first in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iHead.iNext);}




template <class T>
inline TBool TPriQue<T>::IsLast(const T *aPtr) const
/**
Tests whether the specified element is the last in the linked list.

@param aPtr A pointer to the element whose position in the list is to
            be checked.

@return True, if the element is the last in the list; false, otherwise.
*/
	{return(PtrAdd(aPtr,iOffset)==(T *)iHead.iPrev);}




template <class T>
inline T *TPriQue<T>::First() const
/**
Gets a pointer to the first list element in the linked list.

@return A pointer to the first list element in the linked list.
        If the list is empty, this pointer is not necessarily NULL and must
		not be assumed to point to a valid object.
*/
	{return(PtrSub((T *)iHead.iNext,iOffset));}




template <class T>
inline T *TPriQue<T>::Last() const
/**
Gets a pointer to the last list element in the linked list.

@return A pointer to the last list element in the linked list.
        If the list is empty, this pointer is not necessarily NULL and must
		not be assumed to point to a valid object.
*/
	{return(PtrSub((T *)iHead.iPrev,iOffset));}




// Class TDeltaQue
template <class T>
inline TDeltaQue<T>::TDeltaQue()
/**
Constructs an empty list header and sets the offset value of the link object 
to zero.

In practice, never assume that the offset of the link object from the start 
of a list element is zero, even if the link object is declared as the first 
data member in the list element class.

If this default constructor is used, then call the TDblQueBase::SetOffset() 
function in the base class to ensure that the offset value is set correctly.

TDeltaQueBase::iFirstDelta is set to NULL.

@see TDblQueBase::SetOffset
*/
	{}




template <class T>
inline TDeltaQue<T>::TDeltaQue(TInt aOffset)
	: TDeltaQueBase(aOffset)
/**
Constructs an empty list header and sets the offset of the link object to the 
specified value.

TDeltaQueBase::iFirstDelta is set to NULL.

@param aOffset The offset of the link object from the start of a list element. 
                The macro _FOFF can be used to calculate this value. 

@panic USER 78, if aOffset is not divisible by four.
				  
@see _FOFF
*/
	{}




template <class T>
inline void TDeltaQue<T>::Add(T &aRef,TInt aDelta)
/**
Adds the specified list element, having the specified 'distance' from the
nominal zero point, into the list.

The element is added into the list, the adjacent delta values adjusted, and 
a suitable delta value assigned to the new element, so that the new element 
is at the specified 'distance' from the nominal zero point.

@param aRef   The list element to be inserted.
@param aDelta The 'distance' from the nominal zero point.
*/
	{DoAddDelta(&aRef,aDelta);}




template <class T>
inline void TDeltaQue<T>::Remove(T &aRef)
/**
Removes the specified list element from the linked list.

The delta value of the element following the removed element is adjusted
so that its 'distance' from the nominal zero point remains the same.

@param aRef The list element to be removed.
*/
	{DoRemove(&aRef);}




template <class T>
inline T *TDeltaQue<T>::RemoveFirst()
/**
Removes the first list element from the linked list if its delta value is zero 
or negative.

@return A pointer to the element removed from the linked list. This is NULL, 
        if the first element has a positive delta value.
*/
	{return((T *) DoRemoveFirst());}




// Class TSglQueIter
template <class T>
inline TSglQueIter<T>::TSglQueIter(TSglQueBase &aQue)
	: TSglQueIterBase(aQue)
/**
Constructs the iterator for the specified singly linked list.

The iterator can be constructed whether or not the list contains any elements.

If the list does contain elements, the iterator pointer is set to the first one.

If the list has no elements, the iterator pointer is not set and the conversion 
operator T*() and the post increment operator ++ subsequently return NULL. 
Once elements have been added to the list, use either the
TSglQueIter::Set() function or the TSglQueIterBase::SetToFirst() function to set the 
iterator pointer.

@param aQue A reference to a singly linked list header.

@see TSglQueIter::Set
@see TSglQueIterBase::SetToFirst
*/
	{}




template <class T>
inline void TSglQueIter<T>::Set(T &aLink)
/**
Sets the iterator to point to a specific element in the list.

This function can be used to alter the pointer at any time during the iterator's 
existence. The referenced element must be in the list, otherwise the result 
is undefined.

@param aLink A reference to the element from where iteration is to continue.
*/
	{DoSet(&aLink);}




template <class T>
inline TSglQueIter<T>::operator T *()
/**
Gets a pointer to the iterator’s current element.

The operator is normally used implicitly; e.g. some member functions of the
list header class TSglQue require a pointer to an element (of type class T)
as a parameter, but in practice an iterator is often passed instead.
This operator performs the necessary conversion.
*/
	{return((T *)DoCurrent());}




template <class T>
inline T *TSglQueIter<T>::operator++(TInt)
/**
Gets a pointer to the iterator's current element and then sets the iterator 
to point to the next element.

Repeated use of this operator allows successive elements to be accessed.

@return A pointer to the current list element, if the iterator points to an 
        element. NULL, if the iterator does not point to an element; i.e. the
		iterator pointer has reached the end of the list.
*/
	{return((T *)DoPostInc());}




// Class TDblQueIter
template <class T>
inline TDblQueIter<T>::TDblQueIter(TDblQueBase &aQue)
	: TDblQueIterBase(aQue)
/**
Constructs the iterator for the specified doubly linked list

The iterator can be constructed whether or not the list contains any elements.

If the list does contain elements, the iterator pointer is set to the first one.

If the list has no elements, the iterator pointer is not set and the conversion 
operator T*(), the post increment operator++() and the post decrement operator 
--() subsequently return NULL. Once elements have been added to the list, use 
either the TDblQueIter::Set() function, the TDblQueIterBase::SetToFirst() 
function or the TDblQueIterBase::SetToLast() function to set the iterator 
pointer.

@param aQue A reference to a doubly linked list header.

@see TDblQueIter::Set
@see TDblQueIterBase::SetToFirst
@see TDblQueIterBase::SetToLast
*/
	{}




template <class T>
inline void TDblQueIter<T>::Set(T &aLink)
/**
Sets the iterator to point to a specific element in the list.

This function can be used to alter the pointer at any time during
the iterator's existence. The referenced element must be in the list,
otherwise the result is undefined.

@param aLink A reference to the element from where iteration is to continue.
*/
	{DoSet(&aLink);}




template <class T>
inline TDblQueIter<T>::operator T *()
/**
Gets a pointer to the iterator’s current element.

The operator is normally used implicitly; e.g. some member functions of the
list header class TDblQue require a pointer to an element (of type class T)
as a parameter but in practice, an iterator is often passed instead.
This operator performs the necessary conversion.

@return A pointer to the current element, if the iterator points to an element
        in the list. NULL, if the iterator does not point to an element;
		i.e. the iterator pointer has previously reached the end of the list
		(see operator++) or the start of the list (see operator--) or
		the list is empty. 
*/
	{return((T *) DoCurrent());}




template <class T>
inline T *TDblQueIter<T>::operator++(TInt)
/**
Gets a pointer to the iterator's current element and then sets the iterator 
to point to the next element.

Repeated use of this operator allows successive 
elements to be accessed in the forwards direction.

@return A pointer to the current list element, if the iterator points to an 
        element. NULL, if the iterator does not point to an element;
		i.e. the iterator pointer has reached the end of the list.
*/
	{return((T *) DoPostInc());}




template <class T>
inline T *TDblQueIter<T>::operator--(TInt)
/**
Gets a pointer to the iterator's current element and then sets the iterator 
to point to the previous element.

Repeated use of this operator allows successive 
elements to be accessed in the backwards direction.

@return A pointer to the current element, if the iterator points to an element. 
        NULL, if the iterator does not point to an element; i.e. the iterator
		pointer has reached the beginning of the list.
*/
	{return((T *) DoPostDec());}




// Class TKey
inline void TKey::SetPtr(const TAny *aPtr)
/**
Sets the pointer to a sample element whose key is to be used for comparison.
	
The element can be in an existing array or it can be located anywhere in
addressable memory.
	
The At() member function supplied by a derived class must return a pointer 
to this sample element's key when passed an index value of KIndexPtr.
	
SetPtr() must be called before calling User::BinarySearch() because this algorithm 
uses the key of this sample element as the basis for searching the array.
	
@param aPtr A pointer to a sample element.
*/
	{iPtr=aPtr;}




// Class TCharF
inline TCharF::TCharF(TUint aChar)
	: TChar(User::Fold(aChar))
/**
Constructs this 'fold character' object and initialises it with the specified 
value.

@param aChar The initialisation value.
*/
	{}




inline TCharF::TCharF(const TChar& aChar)
	: TChar(User::Fold(aChar))
/**
Constructs this 'fold character' object and initialises it with the value of 
the TChar object aChar.

@param aChar The character object to use as the initialisation value.
*/
	{}




inline TCharF& TCharF::operator=(TUint aChar)
/**
Assigns an unsigned integer value to the 'fold character' object.

@param aChar The value to assign.

@return A reference to this 'fold character' object.
*/
	{SetChar(User::Fold(aChar));return(*this);}




inline TCharF& TCharF::operator=(const TChar& aChar)
/**
Assigns the specified character object to this 'fold character' object.

@param aChar The character object to assign.

@return A reference to this 'fold character' object.
*/
	{SetChar(User::Fold(aChar));return(*this);}




// Class TCharLC
inline TCharLC::TCharLC(TUint aChar)
	: TChar(User::LowerCase(aChar))
/**
Constructs this 'character to lower case' object and initialises it with the 
specified value.

@param aChar The initialisation value.

*/
	{}




inline TCharLC::TCharLC(const TChar& aChar)
	: TChar(User::LowerCase(aChar))
/**
Constructs this 'character to lower case' object and initialises it with the 
value of the TChar object aChar.

@param aChar The character object to use as the initialisation value.
*/
	{}




inline TCharLC& TCharLC::operator=(TUint aChar)
/**
Assigns an unsigned integer value to the 'character to lower case' object.

@param aChar The value to assign.

@return A reference to this 'character to lower case' object.
*/
	{SetChar(User::LowerCase(aChar));return(*this);}




inline TCharLC& TCharLC::operator=(const TChar& aChar)
/**
Assigns the specified character object to this 'character to lower case'
object.

@param aChar The character object to assign.

@return A reference to this 'character to lower case' object.
*/
	{SetChar(User::LowerCase(aChar));return(*this);}




// Class TCharUC
inline TCharUC::TCharUC(TUint aChar)
	: TChar(User::UpperCase(aChar))
/**
Constructs this 'character to upper case' object and initialises it with the 
specified value.

@param aChar The initialisation value.
*/
	{}




inline TCharUC::TCharUC(const TChar& aChar)
	: TChar(User::UpperCase(aChar))
/**
Constructs this 'character to upper case' object and initialises it with the 
value of the TChar object aChar.

@param aChar The character object to use as the initialisation value.
*/
	{}




inline TCharUC& TCharUC::operator=(TUint aChar)
/**
Assigns an unsigned integer value to the 'character to upper case'  object.

@param aChar The value to assign.

@return A reference to this 'character to upper case'  object.
*/
	{SetChar(User::UpperCase(aChar));return(*this);}




inline TCharUC& TCharUC::operator=(const TChar& aChar)
/**
Assigns the specified character object to this 'character to upper case' 
object.

@param aChar The character object to assign.

@return A reference to this 'character to upper case'  object.
*/
	{SetChar(User::UpperCase(aChar));return(*this);}




// Class TDateTime
inline TDateTime::TDateTime()
	: iYear(1980),
	  iMonth(EJanuary), 
	  iDay(1),
	  iHour(0),
	  iMinute(0),
	  iSecond(0),
	  iMicroSecond(0)
/**
Constructs an uninitialised TDateTime object.
*/
	{}           




inline TInt TDateTime::Year() const
/**
Gets the year component of the date/time.

A negative value indicates a BC date.

@return The year
*/
	{return(iYear);}




inline TMonth TDateTime::Month() const
/**
Gets the month component of the date/time.

@return The month. EJanuary to EDecember. Offset from zero, so add one before 
        displaying the month number.
*/
	{return(iMonth);}




inline TInt TDateTime::Day() const
/**
Gets the day component of the date/time.

@return The day. Offset from zero, so add one before displaying the day number.
*/
	{return(iDay);}




inline TInt TDateTime::Hour() const
/**
Gets the hour component of the date/time.

@return The hour.
*/
	{return(iHour);}




inline TInt TDateTime::Minute() const
/**
Gets the minute component of the date/time.

@return The minute.
*/
	{return(iMinute);}




inline TInt TDateTime::Second() const
/**
Gets the second component of the date/time.

@return The second.
*/
	{return(iSecond);}




inline TInt TDateTime::MicroSecond() const
/**
Gets the microsecond component of the date/time.

@return The microsecond.
*/
	{return(iMicroSecond);}

// Class TTimeIntervalMicroSeconds




inline TTimeIntervalMicroSeconds::TTimeIntervalMicroSeconds()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalMicroSeconds::TTimeIntervalMicroSeconds(const TInt64& aInterval)
	: iInterval(aInterval)
/**
Constructs the object with the specified 64-bit interval value.

@param aInterval The 64-bit interval value with which the object is to be
                 initialised.
*/
	{}




inline TTimeIntervalMicroSeconds& TTimeIntervalMicroSeconds::operator=(const TInt64& aInterval)
/**
Assigns a 64-bit integer value to this object.

@param aInterval The 64-bit integer interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




inline TBool TTimeIntervalMicroSeconds::operator==(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is equal to the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if the two time intervals are equal. False otherwise.
*/
	{return(iInterval==aInterval.iInterval);}




inline TBool TTimeIntervalMicroSeconds::operator!=(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is not equal to the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if the two time intervals are not equal. False otherwise.
*/
	{return(iInterval!=aInterval.iInterval);}




inline TBool TTimeIntervalMicroSeconds::operator>=(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is greater than or equal to the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is greater than or equal to the specified
        time interval. False otherwise.
*/
	{return(iInterval>=aInterval.iInterval);}




inline TBool TTimeIntervalMicroSeconds::operator<=(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is less than or equal to the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is less than or equal to the specified
        time interval. False otherwise.
*/
	{return(iInterval<=aInterval.iInterval);}




inline TBool TTimeIntervalMicroSeconds::operator>(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is greater than the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is greater than the specified
        time interval. False otherwise.
*/
	{return(iInterval>aInterval.iInterval);}




inline TBool TTimeIntervalMicroSeconds::operator<(const TTimeIntervalMicroSeconds& aInterval) const
/**
Tests whether this TTimeIntervalMicroSeconds object is less than the
specified TTimeIntervalMicroSeconds object.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is less than the specified
        time interval. False otherwise.
*/
	{return(iInterval<aInterval.iInterval);}




inline const TInt64& TTimeIntervalMicroSeconds::Int64() const
/**
Gets the time interval as a 64-bit integer value.

@return This 64-bit integer time interval value.
*/
	{return(iInterval);}




// Class TTimeIntervalBase
inline TTimeIntervalBase::TTimeIntervalBase()
/**
Default constructor.
*/
	{}




inline TTimeIntervalBase::TTimeIntervalBase(TInt aInterval)
	: iInterval(aInterval)
/**
Constructor taking an interval value.

@param aInterval The interval value.
*/
	{}




inline TBool TTimeIntervalBase::operator==(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is the same as the specified time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if the two time intervals are equal. False otherwise.
*/
	{return(iInterval==aInterval.iInterval);}




inline TBool TTimeIntervalBase::operator!=(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is not the same as the specified
time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if the two time intervals differ. False otherwise.
*/
	{return(iInterval!=aInterval.iInterval);}




inline TBool TTimeIntervalBase::operator>=(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is greater than or equal to the
specified time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is greater than or equal to the specified
        time interval. False otherwise.
*/
	{return(iInterval>=aInterval.iInterval);}




inline TBool TTimeIntervalBase::operator<=(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is less than or equal to the
specified time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is less than or equal to the specified
        time interval. False otherwise.
*/
	{return(iInterval<=aInterval.iInterval);}




inline TBool TTimeIntervalBase::operator>(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is greater than the specified time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is greater than the specified
        time interval. False otherwise.
*/
	{return(iInterval>aInterval.iInterval);}




inline TBool TTimeIntervalBase::operator<(TTimeIntervalBase aInterval) const
/**
Tests whether this time interval is less than the specified time interval.

@param aInterval The time interval to be compared with this time interval.

@return True if this time interval is less than the specified
        time interval. False otherwise.
*/
	{return(iInterval<aInterval.iInterval);}




inline TInt TTimeIntervalBase::Int() const
/** 
Gets the time interval as a 32 bit integer.

@return The time interval as a 32 bit integer.
*/
	{return(iInterval);}




// Class TTimeIntervalMicroSeconds32
inline TTimeIntervalMicroSeconds32::TTimeIntervalMicroSeconds32()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalMicroSeconds32::TTimeIntervalMicroSeconds32(TInt aInterval)
    : TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalMicroSeconds32& TTimeIntervalMicroSeconds32::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalSeconds
inline TTimeIntervalSeconds::TTimeIntervalSeconds()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalSeconds::TTimeIntervalSeconds(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalSeconds& TTimeIntervalSeconds::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalMinutes
inline TTimeIntervalMinutes::TTimeIntervalMinutes()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalMinutes::TTimeIntervalMinutes(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalMinutes& TTimeIntervalMinutes::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalHours
inline TTimeIntervalHours::TTimeIntervalHours()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}
inline TTimeIntervalHours::TTimeIntervalHours(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalHours& TTimeIntervalHours::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalDays
inline TTimeIntervalDays::TTimeIntervalDays()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalDays::TTimeIntervalDays(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalDays& TTimeIntervalDays::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalMonths
inline TTimeIntervalMonths::TTimeIntervalMonths()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalMonths::TTimeIntervalMonths(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalMonths& TTimeIntervalMonths::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTimeIntervalYears
inline TTimeIntervalYears::TTimeIntervalYears()
/**
Default constructor.

Constructs an uninitialised object.
*/
	{}




inline TTimeIntervalYears::TTimeIntervalYears(TInt aInterval)
	: TTimeIntervalBase(aInterval)
/**
Constructs the object with the specified interval value.

@param aInterval The interval value with which the object is to be initialised.
*/
	{}




inline TTimeIntervalYears& TTimeIntervalYears::operator=(TInt aInterval)
/**
Assigns a value to this object.

@param aInterval The interval value to be assigned.

@return A reference to this object.
*/
	{iInterval=aInterval;return(*this);}




// Class TTime
inline TTime::TTime()
/**
Default constructor.

The object is initialised to an arbitrary value.
*/
	{}




inline TTime::TTime(const TInt64& aTime)
	: iTime(aTime)
/**
Constructs the object from a 64-bit microsecond value.

@param aTime Microsecond value to which to initialise the TTime object.
*/
	{}




inline TTime &TTime::operator=(const TInt64& aTime)
/**
Assigns a value contained in a 64-bit integer to this TTime object.

@param aTime Microsecond value which to assign to the TTime object.

@return This TTime object.
*/
	{iTime=aTime;return(*this);}




inline TBool TTime::operator==(TTime aTime) const
/**
Tests whether two date/times are equal.

@param aTime The time to be compared with this TTime.

@return True if the two TTimes are equal. False if not.
*/
	{return(iTime==aTime.iTime);}




inline TBool TTime::operator!=(TTime aTime) const
/**
Tests whether two date/times are not equal.

@param aTime The date/time to be compared with this TTime.

@return True if the two TTimes are different. False if the same.
*/
	{return(iTime!=aTime.iTime);}




inline TBool TTime::operator>=(TTime aTime) const
/**
Tests whether this date/time is later than or the same as the
specified date/time.

@param aTime The date/time to be compared with this date/time.

@return True if this date/time is later than or the same as the
        specified date/time. False otherwise.
*/
	{return(iTime>=aTime.iTime);}




inline TBool TTime::operator<=(TTime aTime) const
/**
Tests whether this date/time is earlier than or the same as the
specified date/time.

@param aTime The date/time to be compared with this date/time.

@return True if this date/time is earlier than or the same as the
        specified date/time. False otherwise.
*/
	{return(iTime<=aTime.iTime);}




inline TBool TTime::operator>(TTime aTime) const
/**
Tests whether this date/time is later than the specified date/time.

@param aTime The date/time to be compared with this date/time.

@return True if this date/time is later than the specified date/time.
        False otherwise.
*/
	{return(iTime>aTime.iTime);}




inline TBool TTime::operator<(TTime aTime) const
/**
Tests whether this date/time is earlier than the specified date/time.

@param aTime The date/time to be compared with this date/time.

@return True if this date/time is earlier than the specified date/time.
        False otherwise.
*/
	{return(iTime<aTime.iTime);}




inline const TInt64& TTime::Int64() const
/**
Gets the 64 bit integer representation of this TTime obect.

@return The 64 bit integer representation.
*/
	{return(iTime);}




// Class TLexMark8
inline TLexMark8::TLexMark8()
	: iPtr(NULL)
/**
Default constructor.
*/
	{}




inline TLexMark8::TLexMark8(const TUint8 *aString) 
	: iPtr(aString)
	{}




// Class TLex8
inline TLex8::TLex8(const TUint8 *aString)
/**
Constructs the object with a pointer to a string.

The extraction mark and next character members are initialised to point
to the start of the string.

@param aString String to be assigned.
*/
	{Assign(TPtrC8(aString));}




inline TLex8::TLex8(const TDesC8 &aDes)
/**
Constructs the object with a descriptor.

The extraction mark and next character 
members are initialised to point to the start of the string.

@param aDes Descriptor to be assigned by reference.
*/
	{Assign(aDes);}




inline TLex8& TLex8::operator=(const TUint8* aString)
/**
Allows strings to be assigned to a TLex8.

@param aString String to be assigned to the TLex8. 

@return TLex8 descriptor.
*/
	{Assign(TPtrC8(aString));return(*this);}




inline TLex8& TLex8::operator=(const TDesC8& aBuf)
/**
Allows descriptors to be assigned to a TLex8.

@param aBuf Descriptor to be assigned to the TLex8.

@return TLex8 descriptor.
*/
	{Assign(aBuf);return(*this);}




inline TBool TLex8::Eos() const
/**
Tests whether the next character position is at the end of the string.

@return True if at end of string, false otherwise.
*/
	{return(iNext==iEnd);}




inline void TLex8::Mark()
/**
Sets the TLex8's next character position to its extraction mark.
*/
	{Mark(iMark);}




inline void TLex8::Mark(TLexMark8& aMark) const
/**
Sets the supplied extraction mark to the TLex8's next character position.

@param aMark On return, this is set to the next character position.
*/
	{aMark.iPtr=iNext;}




inline void TLex8::UnGetToMark()
/**
Sets the next character position to the current extraction mark position.

@panic USER 63, if the extraction mark is before the start or beyond the end
       of the string.
*/
    {UnGetToMark(iMark);}




inline void TLex8::SkipAndMark(TInt aNumber)
/**
Moves the next character position a specified number of characters. 
  
@param aNumber Number of characters to skip.

@panic USER 61, if the skip moves the next character position either to before
       the start or beyond the end of the string.
*/
    {SkipAndMark(aNumber,iMark);}




inline void TLex8::SkipSpaceAndMark()
/**
Moves the next character position past any white space and copies it to the 
TLex8's extraction mark.

Stops if at the end of the string.
*/
    {SkipSpaceAndMark(iMark);}




inline TInt TLex8::TokenLength() const
/**
Gets the length of the token.

This is the difference between the next character 
position and the extraction mark.

@return Length of the token.
*/
	{return(iNext-iMark.iPtr);}




inline TInt TLex8::MarkedOffset() const
/**
Gets the offset of the extraction mark from the start of the string.

@return The offset of the extraction mark.
*/
    {return(iMark.iPtr-iBuf);}




inline TInt TLex8::Val(TInt &aVal)
/**
Parses the string to extract a signed integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{return(Val((TInt32&)aVal));}




inline TInt TLex8::Val(TUint &aVal,TRadix aRadix)
/**
Parses the string to extract an unsigned integer, using the specified radix.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number. The default radix
              for this function overload is decimal.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{return(Val((TUint32&)aVal,aRadix));}




inline void TLex8::Assign(const TLex8& aLex)
/**
Assigns a string to this object from another TLex8 object.

@param aLex The object to be assigned.
*/
	{new(this) TLex8(aLex);}




// Class TLexMark16
inline TLexMark16::TLexMark16()
	: iPtr(NULL)
/**
Default constructor.
*/
	{}




inline TLexMark16::TLexMark16(const TUint16 *aString) 
	: iPtr(aString)
	{}




// Class TLex16
inline TLex16::TLex16(const TUint16 *aString)
/**
Constructs the object with a pointer to a string.

The extraction mark and next character members are initialised to point
to the start of the string.

@param aString String to be assigned.
*/
	{Assign(TPtrC16(aString));}




inline TLex16::TLex16(const TDesC16 &aDes)
/**
Constructs the object with a descriptor.

The extraction mark and next character 
members are initialised to point to the start of the string.

@param aDes Descriptor to be assigned by reference.
*/
	{Assign(aDes);}



inline TLex16& TLex16::operator=(const TUint16* aString)
/** 
Allows strings to be assigned to a TLex16.

@param aString String to be assigned to the TLex16. 

@return TLex16 descriptor.
*/
	{Assign(TPtrC16(aString));return(*this);}




inline TLex16& TLex16::operator=(const TDesC16& aBuf)
/**
Allows descriptors to be assigned to a TLex16.

@param aBuf Descriptor to be assigned to the TLex16.

@return TLex8 descriptor.
*/
	{Assign(aBuf);return(*this);}




inline TBool TLex16::Eos() const
/**
Tests whether the next character position is at the end of the string.

@return True if at end of string, false otherwise.
*/
	{return(iNext==iEnd);}




inline void TLex16::Mark(TLexMark16& aMark) const
/**
Sets the supplied extraction mark to the TLex16's next character position.

@param aMark On return, set to the next character position.
*/
	{aMark.iPtr=iNext;}




inline void TLex16::Mark()
/**
Sets the TLex16's next character position to its extraction mark.
*/
	{iMark.iPtr=iNext;}




inline void TLex16::UnGetToMark()
/**
Sets the next character position to the current extraction mark position.

@panic USER 68, if the specified mark is before the start or beyond the end
       of the string.
*/
    {UnGetToMark(iMark);}




inline void TLex16::SkipAndMark(TInt aNumber)
/**
Moves the next character position a specified number of characters.

@param aNumber Number of characters to skip. 

@panic USER 68, if the skip moves the next character position either to before
       the start or beyond the end of the string.
*/
    {SkipAndMark(aNumber,iMark);}




inline void TLex16::SkipSpaceAndMark()
/**
Moves the next character position past any white space and copies it to the 
TLex16's extraction mark.

Stops if at the end of the string.
*/
    {SkipSpaceAndMark(iMark);}




inline TInt TLex16::TokenLength() const
/**
Gets the length of the token.

This is the difference between the next character 
position and the extraction mark.

@return Length of the token.
*/
	{return(iNext-iMark.iPtr);}




inline TInt TLex16::MarkedOffset() const
/**
Gets the offset of the extraction mark from the start of the string.

@return The offset of the extraction mark.
*/
    {return(iMark.iPtr-iBuf);}




inline TInt TLex16::Val(TInt &aVal)
/**
Parses the string to extract a signed integer.

@param aVal On return, contains the extracted integer.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{return(Val((TInt32&)aVal));}




inline TInt TLex16::Val(TUint &aVal,TRadix aRadix)
/**
Parses the string to extract an unsigned integer, using the specified radix.

@param aVal   On return, contains the extracted integer.
@param aRadix The radix to use when converting the number. The default radix
              for this function overload is decimal.

@return KErrNone if successful.
        KErrGeneral if the next character position is initially at the end of the string
        or no valid characters found initially.
        KErrOverflow if there is sign overflow, i.e. converted value greater than limit.
        If error codes KErrGeneral or KErrOverflow are returned, the object's
        members are left unaltered.
*/
	{return(Val((TUint32&)aVal,aRadix));}




inline void TLex16::Assign(const TLex16& aLex)
/**
Assigns a string to this object from another TLex16 object.

@param aLex The object to be assigned.
*/
	{new(this) TLex16(aLex);}




// Class TLocale
inline TLocale::TLocale(TInt)
	{}

inline TInt TLocale::RegionCode() const
	{return(iRegionCode);}
inline TInt TLocale::CountryCode() const
/**
Gets the code which is used to select country-specific locale data.

The country code is the code used as the international dialling prefix.
This code is also used to identify a country by the dialling software.
	
@return The country code.
*/
	{return(iCountryCode);}




inline void TLocale::SetCountryCode(TInt aCode)
/**
Sets the value which is used to select country-specific locale data.

This value can be retrieved by using TLocale::CountryCode(). The country code
is the code used as the international dialling prefix. This code is also used
to identify a country by the dialling software.
	
@param aCode The country code.

@see TLocale::CountryCode
*/
	{iCountryCode=aCode;}




inline TTimeIntervalSeconds TLocale::UniversalTimeOffset() const
/**
Gets the locale's universal time offset.
	
@return Offset in seconds from universal time. Time zones east of universal 
	    time have positive offsets. Time zones west of universal time have negative 
	    offsets.

@deprecated Use User::UTCOffset to get the current offset inclusive of daylight
			savings time. This function returns the same value, for compatibility.
*/
	{return(iUniversalTimeOffset);}




inline TDateFormat TLocale::DateFormat() const
/**
Gets the date format.
	
@return The date format.
*/
	{return(iDateFormat);}




inline void TLocale::SetDateFormat(TDateFormat aFormat)
/**
Sets the date format.
	
@param aFormat The date format to be used.
*/
	{iDateFormat=aFormat;}




inline TTimeFormat TLocale::TimeFormat() const
/**
Gets the time format (12 or 24 hour).
	
@return The time format.
*/
	{return(iTimeFormat);}




inline void TLocale::SetTimeFormat(TTimeFormat aFormat)
/**
Sets the time format (12 or 24 hour).
	
@param aFormat The time format.
*/
	{iTimeFormat=aFormat;}




inline TLocalePos TLocale::CurrencySymbolPosition() const
/**
Gets the currency symbol position.
	
For negative currency values, this position may be
reversed using SetNegativeCurrencySymbolOpposite().
	
@return The currency symbol position.

@see TLocale::SetNegativeCurrencySymbolOpposite
*/
	{return(iCurrencySymbolPosition);}




inline void TLocale::SetCurrencySymbolPosition(TLocalePos aPos)
/**
Sets the currency symbol position.
	
@param aPos The currency symbol position.
*/
	{iCurrencySymbolPosition=aPos;}




inline TBool TLocale::CurrencySpaceBetween() const
/**
Gets whether or not a space is inserted between the currency symbol and the 
currency value.
	
For negative currency values, the space can be removed using SetNegativeLoseSpace().
	
@return True if a space is inserted; false if not.

@see TLocale::SetNegativeLoseSpace
*/
	{return(iCurrencySpaceBetween);}




inline void TLocale::SetCurrencySpaceBetween(TBool aSpace)
/**
Sets whether a space is inserted between the currency symbol and the currency 
amount.
	
@param aSpace ETrue if a space is inserted; EFalse if not.
*/
	{iCurrencySpaceBetween=aSpace;}




inline TInt TLocale::CurrencyDecimalPlaces() const
/**
Gets the number of decimal places to which currency values are set.
	
@return The number of decimal places.
*/
	{return(iCurrencyDecimalPlaces);}




inline void TLocale::SetCurrencyDecimalPlaces(TInt aPlaces)
/**
Sets the number of decimal places to which currency values should be set.
	
@param aPlaces The number of decimal places.
*/
	{iCurrencyDecimalPlaces=aPlaces;}




inline TBool TLocale::CurrencyNegativeInBrackets() const
/**
@deprecated

Gets whether negative currency values are enclosed in brackets rather than 
being preceded by a minus sign. 
	
This is deprecated, use NegativeCurrencyFormat() instead.
	
@return True if negative currency is enclosed in brackets and has no minus 
        sign; false if negative currency has a minus sign and is not enclosed
		in brackets.

@see TLocale::NegativeCurrencyFormat
*/
	{return((TBool)iNegativeCurrencyFormat);}			




inline void TLocale::SetCurrencyNegativeInBrackets(TBool aBool)
/** 
@deprecated

Sets whether negative currency values are enclosed in brackets rather than
being preceded by a minus sign.
	
This is deprecated, use SetNegativeCurrencyFormat() instead.
	
@param aBool ETrue, if a negative currency value must be enclosed in brackets 
	         without a minus sign; EFalse, if a negative currency value is
			 preceded by a minus sign without any enclosing brackets.

@see TLocale::SetNegativeCurrencyFormat
*/
	{iNegativeCurrencyFormat=(aBool)?EInBrackets:ELeadingMinusSign;}




inline TBool TLocale::CurrencyTriadsAllowed() const
/**
Gets whether triads are allowed in currency values. Triads are groups of 
three digits separated by the thousands separator.
	
@return True if triads are allowed; false if not.
*/
	{return(iCurrencyTriadsAllowed);}




inline void TLocale::SetCurrencyTriadsAllowed(TBool aBool)
/**
Sets whether triads are allowed in currency values.
	
@param aBool ETrue if triads are allowed; EFalse if triads not allowed.
*/
	{iCurrencyTriadsAllowed=aBool;}




inline TChar TLocale::ThousandsSeparator() const
/**
Gets the character used to separate groups of three digits to the left of 
the decimal separator.
	
A thousands separator character is only displayed in currency values if currency 
triads are allowed.
	
@return The character used as the thousands separator.
*/
	{return(iThousandsSeparator);}




inline void TLocale::SetThousandsSeparator(const TChar& aChar)
/**
Sets the character to be used to separate groups of three digits to the left 
of the decimal separator.
	
A thousands separator character is only displayed in currency values if currency 
triads are allowed.
	
@param aChar The character to be used as the thousands separator.
*/
	{iThousandsSeparator=aChar;}




inline TChar TLocale::DecimalSeparator() const
/**
Gets the character used to separate a whole number from its fractional part.
	
@return The character used as the decimal separator.
*/
	{return(iDecimalSeparator);}




inline void TLocale::SetDecimalSeparator(const TChar& aChar)
/**
Sets the character to be used to separate a whole number from its fractional 
part.
	
@param aChar The character to be used as the decimal separator.
*/
	{iDecimalSeparator=aChar;}




inline TChar TLocale::DateSeparator(TInt aIndex) const
/**
Gets one of the four characters used to separate the day, month and year 
components of the date.
	
If the four separators are represented by S0, S1, S2 and S3 and the three 
date components are represented by XX, YY and ZZ, then the separators are 
located: S0 XX S1 YY S2 ZZ S3.
	
@param aIndex An index indicating which of the four separators is being accessed. 
              This must be a value between zero and three inclusive.

@return A date separator character as determined by the value of aIndex.
*/
	{return(iDateSeparator[aIndex]);}




inline void TLocale::SetDateSeparator(const TChar& aChar,TInt aIndex)
/**
Sets one of the four characters used to separate the day, month and year
components of the date.
	
If the four separators are represented by S0, S1, S2 and S3 and the three 
date components are represented by XX, YY and ZZ, then the separators are 
located: S0 XX S1 YY S2 ZZ S3.
	
@param aChar  A date separator character to be used.
@param aIndex An index indicating which of the four separators is being accessed. 
	          This must be a value between zero and three inclusive.
*/
	{__ASSERT_DEBUG(aIndex>=0 && aIndex<KMaxDateSeparators,User::Invariant());
	iDateSeparator[aIndex]=aChar;}




inline TChar TLocale::TimeSeparator(TInt aIndex) const
/**
Gets one of the four characters used to separate the hour, second and minute 
components of the time.
	
If the four separators are represented by S0, S1, S2 and S3 and the three 
time components are represented by XX, YY and ZZ, then the separators are 
located: S0 XX S1 YY S2 ZZ S3.
	
@param aIndex An index indicating which of the four separators is being
              accessed. This must be a value between zero and three inclusive.

@return A time separator character as determined by the value of aIndex.
*/

	{return(iTimeSeparator[aIndex]);}




inline void TLocale::SetTimeSeparator(const TChar& aChar,TInt aIndex)
/**
Sets one of the four characters used to separate the hour, minute and second 
components of the date.
	
If the four separators are represented by S0, S1, S2 and S3 and the three 
time components are represented by XX, YY and ZZ, then the separators are 
located: S0 XX S1 YY S2 ZZ S3.
	
@param aChar  A time separator character to be used.
@param aIndex An index indicating which of the four separators is being accessed. 
	          This must be a value between zero and three inclusive.
*/
	{__ASSERT_DEBUG(aIndex>=0 && aIndex<KMaxTimeSeparators,User::Invariant());
	iTimeSeparator[aIndex]=aChar;}




inline TLocalePos TLocale::AmPmSymbolPosition() const
/**
Gets the am/pm text position (before or after the time value).

@return The am/pm text position (0 before, 1 after).
*/
	{return(iAmPmSymbolPosition);}




inline void TLocale::SetAmPmSymbolPosition(TLocalePos aPos)
/**
Sets the am/pm text position (before or after the time value).
	
@param aSpace The am/pm text position (0 before, 1 after).
*/
	{iAmPmSymbolPosition=aPos;}




inline TBool TLocale::AmPmSpaceBetween() const
/**
Tests whether or not a space is inserted between the time and the preceding 
or trailing am/pm text.
	
@return True if a space is inserted between the time and am/pm text; false 
        if not.
*/
	{return(iAmPmSpaceBetween);}




inline void TLocale::SetAmPmSpaceBetween(TBool aSpace)
/**
Sets whether a space is inserted between the time and the preceding or trailing 
am/pm text.
	
@param aPos ETrue if a space is inserted between the time and am/pm text; 
            EFalse otherwise.
*/
	{iAmPmSpaceBetween=aSpace;}




inline TUint TLocale::DaylightSaving() const
/**
Gets the zones in which daylight saving is in effect.
	
If daylight saving is in effect, one hour is added to the time.
	
Use TLocale::QueryHomeHasDaylightSavingOn() to find out whether daylight saving 
is in effect for the home city. This is because the daylight saving setting 
for the home city may differ from that of the zone in which home is located.
	
@return A bit mask in which the three least significant bits are defined, 
        indicating which of the three daylight saving zones are adjusted for
		daylight saving. These bits represent:
		Northern (non-European countries in the northern hemisphere),
		Southern (southern hemisphere),
		and European.

@see TLocale::QueryHomeHasDaylightSavingOn
@see TDaylightSavingZone

@deprecated Use the timezone server to retrieve information on timezones and DST.
			This method will always indicate that DST is inactive, in order to
			preserve compatibility.
*/
	{return(iDaylightSaving);} 




inline TBool TLocale::QueryHomeHasDaylightSavingOn() const
/**
Tests whether or not daylight saving is set for the home city.
	
@return True if home daylight saving is set; false if not.

@deprecated Use the timezone server to retrieve information on timezones and DST.
			This method will always indicate that DST is inactive, in order to
			preserve compatibility.
*/
	{return((iHomeDaylightSavingZone|EDstHome) & iDaylightSaving);}




inline TDaylightSavingZone TLocale::HomeDaylightSavingZone() const
/**
Gets the daylight saving zone in which the home city is located.
	
@return The daylight saving zone in which the home city is located.

@deprecated Use the timezone server to retrieve information on timezones and DST.
*/
	{return(iHomeDaylightSavingZone);}




inline TUint TLocale::WorkDays() const
/**
Gets a bit mask representing the days of the week which are considered as 
working days.
	
@return A bit mask of seven bits indicating (by being set) which days are 
        workdays. The least significant bit corresponds to Monday, the next bit to 
	    Tuesday and so on.
*/
	{return(iWorkDays);}




inline void TLocale::SetWorkDays(TUint aMask)
/**
Sets the days of the week which are considered as working days.
	
@param aMask A bit mask of seven bits indicating (by being set) which days 
             are workdays. The least significant bit corresponds to Monday, the
			 next bit is Tuesday and so on.
*/
	{iWorkDays=aMask;}




inline TDay TLocale::StartOfWeek() const
/**
Gets the day which is considered the first day of the week.
	
@return The first day of the week.
*/
	{return(iStartOfWeek);}




inline void TLocale::SetStartOfWeek(TDay aDay)
/**
Sets the day which is considered to be the first day of the week.
	
@param aDay The first day of the week.
*/
	{iStartOfWeek=aDay;}




inline TClockFormat TLocale::ClockFormat() const
/**
Gets the clock display format.
	
@return The clock display format.
*/
	{return(iClockFormat);}




inline void TLocale::SetClockFormat(TClockFormat aFormat)
/**
Sets the clock display format.
	
@param aFormat The clock display format.
*/
	{iClockFormat=aFormat;}




inline TUnitsFormat TLocale::UnitsGeneral() const
/**
Gets the general units of measurement.

This function should be used when both short and long distances use the
same units of measurement.
	
@return General units of measurement.
*/
	{return(iUnitsGeneral);}




inline void TLocale::SetUnitsGeneral(TUnitsFormat aFormat)
/**
Sets the general units of measurement.
This function should be used when both short and long distances use the
same units of measurement.
	
@param aFormat General units of measurement.
*/
	{iUnitsGeneral=aFormat;}




inline TUnitsFormat TLocale::UnitsDistanceShort() const
/**
Gets the units of measurement for short distances.

Short distances are those which would normally be represented by either
metres and centimetres or feet and inches.
	
@return Units of measurement for short distances.
*/
	{return(iUnitsDistanceShort);}




inline void TLocale::SetUnitsDistanceShort(TUnitsFormat aFormat)
/**
Sets the units of measurement for short distances.

Short distances are those which would normally be represented by either
metres and centimetres or feet and inches.
	
@param aFormat Units of measurement for short distances.
*/
	{iUnitsDistanceShort=aFormat;}




inline TUnitsFormat TLocale::UnitsDistanceLong() const
/**
Gets the units of measurement for long distances.

Long distances are those which would normally be represented by either
miles or kilometres.
	
@return Units of measurement for long distances.
*/
	{return(iUnitsDistanceLong);}




inline void TLocale::SetUnitsDistanceLong(TUnitsFormat aFormat)
/**
Sets the units of measurement for long distances.

Long distances are those which would normally be represented by either
miles or kilometres.
	
@param aFormat Units of measurement for long distances.
*/
	{iUnitsDistanceLong=aFormat;}




inline void TLocale::SetNegativeCurrencyFormat(TLocale::TNegativeCurrencyFormat aNegativeCurrencyFormat)
/**
Sets the negative currency format.
	
@param aNegativeCurrencyFormat How negative currency values are formatted.
*/
	{iNegativeCurrencyFormat = aNegativeCurrencyFormat;}




inline TLocale::TNegativeCurrencyFormat TLocale::NegativeCurrencyFormat() const
/**
Gets the negative currency format.
	
@return How negative currency values are formatted.
*/
	{return(iNegativeCurrencyFormat);}




inline TBool TLocale::NegativeLoseSpace() const
/**
Gets whether negative currency values lose the space between the currency 
symbol and the value.
	
@return True, if negative currency values lose the space between the value 
	    and the symbol; false, if not.
*/
	{ 
	if((iExtraNegativeCurrencyFormatFlags|EFlagNegativeLoseSpace)==iExtraNegativeCurrencyFormatFlags)
		return ETrue;
	else
		return EFalse;
	}




inline void TLocale::SetNegativeLoseSpace(TBool aBool)
/**
Sets whether negative currency values lose the space between the currency symbol 
and the value.
	
@param aBool ETrue to set a flag which indicates that negative currency values 
	         should lose the space between the value and the symbol. EFalse to unset it.
*/
	{
	if(aBool)
		iExtraNegativeCurrencyFormatFlags |= EFlagNegativeLoseSpace;
	else
		iExtraNegativeCurrencyFormatFlags &= ~EFlagNegativeLoseSpace;
	}




inline TBool TLocale::NegativeCurrencySymbolOpposite() const
/**
Gets whether in negative currency values, the position of the currency symbol 
is set to be the opposite of the position used for non-negative values (before 
or after the value, as set by SetCurrencySymbolPosition()).
	
@return True, if the currency symbol position for negative currency values 
	    is the opposite of the position set by SetCurrencySymbolPosition();
		false, otherwise.

@see TLocale::SetCurrencySymbolPosition
*/
	{
	if((iExtraNegativeCurrencyFormatFlags|EFlagNegativeCurrencySymbolOpposite)==iExtraNegativeCurrencyFormatFlags)
		return ETrue;
	else
		return EFalse;
	}




inline void TLocale::SetNegativeCurrencySymbolOpposite(TBool aBool)
/**
Sets whether the position of the currency symbol for negative currency values 
should be the opposite of the position used for non-negative values (before 
or after the value, as set by SetCurrencySymbolPosition()).
	
@param aBool ETrue to set the position of the currency symbol in negative 
             currency values to be the opposite of the position as set
			 using SetCurrencySymbolPosition(). EFalse to leave the
			 position unchanged.

@see TLocale::SetCurrencySymbolPosition
*/
	{
	if (aBool)
		iExtraNegativeCurrencyFormatFlags |= EFlagNegativeCurrencySymbolOpposite;
	else
		iExtraNegativeCurrencyFormatFlags &= ~EFlagNegativeCurrencySymbolOpposite;
	}




inline TLanguage TLocale::LanguageDowngrade(TInt aIndex) const
/**
Gets the language that is stored at the specified index into the customisable 
part of the language downgrade path.
	
The second, third and fourth languages in the language downgrade path can 
be customised. These can be enquired using this function. The first language 
in the path is always the language of the current locale, as returned by User::Language(). 
	
The languages in the downgrade path are used in turn by the BaflUtils::NearestLanguageFile() 
function to find the best matching language-specific version of a language-neutral 
filename.
	
The full language downgrade path can be retrieved using BaflUtils::GetDowngradePath().
	
@param aIndex An index into the customisable part of the language downgrade 
              path. Between zero and two inclusive.

@return The language at the specified index.

@see BaflUtils::NearestLanguageFile
@see BaflUtils::GetDowngradePath
*/
	{
	__ASSERT_DEBUG(0 <= aIndex && aIndex < 3, User::Invariant());
	return static_cast<TLanguage>(iLanguageDowngrade[aIndex]);
	}




inline void TLocale::SetLanguageDowngrade(TInt aIndex, TLanguage aLanguage)
/**
Sets a language in the customisable part of the language downgrade path.
	
@param aIndex    An index into the customisable part of the path at which to 
	             add the language, a value between zero and two.
@param aLanguage The language to add. ELangNone is considered to be the last 
	             language in the path, no more will be searched, so can be used
				 to specify that no language downgrade is required.

@see BaflUtils::NearestLanguageFile
@see BaflUtils::GetDowngradePath
*/
	{
	__ASSERT_DEBUG(0 <= aIndex && aIndex < 3, User::Invariant());
	iLanguageDowngrade[aIndex] = static_cast<TUint16>(aLanguage);
	}




/**
Gets the number mode stored in the locale.

@return The number mode for the locale.
*/
inline TDigitType TLocale::DigitType() const
	{ return iDigitType; }




/**
Sets the number mode for the locale. 

@param aDigitType The number mode to be set.
*/
inline void TLocale::SetDigitType(TDigitType aDigitType)
	{ iDigitType=aDigitType; }




/**
Sets the device time state.

@param aState The device time state. 

@deprecated Use the timezone server to coordinate automatic time adjustment.
*/
inline void TLocale::SetDeviceTime(TDeviceTimeState aState)
   	{
   	iDeviceTimeState=aState;
   	}
   

/**
Get the pointer to the TLocale object contained in this extended locale.

@return Pointer to the TLocale object. 
*/
inline TLocale*	TExtendedLocale::GetLocale()
	{ return &iLocale; }


/**
Gets the device time state.

@return The device time state.

@deprecated Use the timezone server to coordinate automatic time adjustment.
*/
inline TLocale::TDeviceTimeState TLocale::DeviceTime() const
   	{
   	return iDeviceTimeState;
   	}


// Class TFindSemaphore
inline TFindSemaphore::TFindSemaphore()
    : TFindHandleBase()
/**
Constructs the object with a default match pattern.

The default match pattern, as implemented by the base class, is the single 
character "*".

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

@see TFindHandleBase::Find
*/
    {}




inline TFindSemaphore::TFindSemaphore(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs this object with the specified match pattern.

A new match pattern can be set after construction by
calling TFindHandleBase::Find().

Note that after construction, the object contains a copy of the supplied
match pattern; the source descriptor can, therefore, be safely discarded.

@param aMatch A reference to the descriptor containing the match pattern. 

@see TFindHandleBase::Find
*/
    {}




// Class TFindMutex
inline TFindMutex::TFindMutex()
    : TFindHandleBase()
/**
Constructs this object with a default match pattern.

The default match pattern, as implemented by the base class, is the single 
character "*".

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

@see TFindHandleBase::Find
*/
    {}




inline TFindMutex::TFindMutex(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs this object with the specified match pattern.

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

After construction, the object contains a copy of the supplied match pattern; 
the source descriptor can, therefore, be safely discarded.

@param aMatch The match pattern.

@see TFindHandleBase::Find
*/
    {}




// Class TFindChunk
inline TFindChunk::TFindChunk()
    : TFindHandleBase()
/**
Constructs this object with a default match pattern.

The default match pattern, as implemented by the base class, is
the single character "*".

A new match pattern can be set after construction by
calling TFindHandleBase::Find().

@see TFindHandleBase
*/
    {}




inline TFindChunk::TFindChunk(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs the object with the specified match pattern.

A new match pattern can be set after construction by
calling TFindHandleBase::Find().

@param aMatch The match pattern.

@see TFindHandleBase
*/
    {}




// Class TFindThread
inline TFindThread::TFindThread()
    : TFindHandleBase()
/**
Constructs this object with a default match pattern.

The default match pattern, as implemented by the base class,
is the single character *.

A new match pattern can be set after construction
by calling TFindHandleBase::Find().

@see TFindHandleBase::Find
*/
    {}




inline TFindThread::TFindThread(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs this object with the specified match pattern.

A new match pattern can be set after construction
by calling the TFindHandleBase::Find().

@see TFindHandleBase::Find
*/
    {}




// Class TFindProcess
inline TFindProcess::TFindProcess()
    : TFindHandleBase()
/**
Constructs this object with a default match pattern.

The default match pattern, as implemented by the base class,
is the single character *.

A new match pattern can be set after construction
by calling TFindHandleBase::Find().

@see TFindHandleBase::Find
*/
    {}




inline TFindProcess::TFindProcess(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs this object with the specified match pattern.

A new match pattern can be set after construction
by calling the TFindHandleBase::Find().

@see TFindHandleBase::Find
*/
    {}




// Class TFindLogicalDevice
/**
Constructs the LDD factory object with a default match pattern.

The default match pattern, as implemented by the base class, is the single 
character "*".

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

@see TFindHandleBase::Find
*/
inline TFindLogicalDevice::TFindLogicalDevice()
    : TFindHandleBase()
    {}

/**
Constructs the LDD factory object with a specified match pattern.

A new match pattern can be set after construction by calling
TFindHandleBase::Find().

@param aMatch The match pattern.

@see TFindHandleBase::Find
*/
inline TFindLogicalDevice::TFindLogicalDevice(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
    {}

// Class TFindPhysicalDevice
/**
Constructs the PDD factory object with a default match pattern.

The default match pattern, as implemented by the base class, is the single 
character "*".

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

@see TFindHandleBase::Find
*/
inline TFindPhysicalDevice::TFindPhysicalDevice()
    : TFindHandleBase()
    {}

/**
Constructs the PDD factory object with a specified match pattern.

A new match pattern can be set after construction by calling
TFindHandleBase::Find().

@param aMatch The match pattern.

@see TFindHandleBase::Find
*/
inline TFindPhysicalDevice::TFindPhysicalDevice(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
    {}





// Class TFindServer
inline TFindServer::TFindServer()
    : TFindHandleBase()
/**
Constructs the object with a default match pattern.

The default match pattern, as implemented by the base class, is the single 
character "*".

A new match pattern can be set after construction by calling the Find() member 
function of the TFindHandleBase base class.

@see TFindHandleBase::Find
*/
    {}




inline TFindServer::TFindServer(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs the object with a specified match pattern.

A new match pattern can be set after construction by calling
TFindHandleBase::Find().

@param aMatch The match pattern.

@see TFindHandleBase::Find
*/
    {}




// Class TFindLibrary
inline TFindLibrary::TFindLibrary()
    : TFindHandleBase()
/**
Constructs this object with a default match pattern.

The default match pattern is the single character ‘*’ and is implemented by
the base class TFindHandleBase.
*/
    {}




inline TFindLibrary::TFindLibrary(const TDesC &aMatch)
    : TFindHandleBase(aMatch)
/**
Constructs this object with the specified match pattern.

@param aMatch The descriptor containing the match pattern. 
*/
    {}




// Class RDevice
/**
Opens a handle to an LDD factory object found using a TFindLogicalDevice object.

A TFindLogicalDevice object is used to find all LDD factory objects whose full names match 
a specified pattern.

@param aFind A reference to the object which is used to find the LDD factory object.
@param aType An enumeration whose enumerators define the ownership of this 
             LDD factory object handle. If not explicitly specified, EOwnerProcess is
			 taken as default.

@return KErrNone if successful, otherwise one of the other system wide error 
        codes.
*/
inline TInt RDevice::Open(const TFindLogicalDevice& aFind,TOwnerType aType)
	{return(RHandleBase::Open(aFind,aType));}




// Class RCriticalSection
inline TBool RCriticalSection::IsBlocked() const
/**
Tests whether the critical section is occupied by any thread.

@return True, if the critical section is occupied by another thread. False, 
        otherwise.
*/
	{return(iBlocked!=1);}




// Class RMutex
inline TInt RMutex::Open(const TFindMutex& aFind,TOwnerType aType)
/**
Opens a handle to the global mutex found using a TFindMutex object.

A TFindMutex object is used to find all global mutexes whose full names match 
a specified pattern.

By default, any thread in the process can use this instance of RMutex to access 
the mutex. However, specifying EOwnerThread as the second parameter to this 
function, means that only the opening thread can use this instance of RMutex 
to access the mutex; any other thread in this process that wants to access 
the mutex must either duplicate the handle or use OpenGlobal() again.

@param aFind A reference to the object which is used to find the mutex.
@param aType An enumeration whose enumerators define the ownership of this 
             mutex handle. If not explicitly specified, EOwnerProcess is
			 taken as default. 

@return KErrNone if successful, otherwise one of the other system wide error 
        codes.
*/
	{return(RHandleBase::Open(aFind,aType));}




// Class RChunk
inline TInt RChunk::Open(const TFindChunk& aFind,TOwnerType aType)
/**
Opens a handle to the global chunk found using a TFindChunk object.

A TFindChunk object is used to find all chunks whose full names match
a specified pattern. 

By default, ownership of this chunk handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aFind A reference to the TFindChunk object used to find the chunk.
@param aType An enumeration whose enumerators define the ownership of this 
             chunk handle. If not explicitly specified, EOwnerProcess is
			 taken as default.

@return KErrNone if successful, otherwise another of the system error codes.
*/
	{return(RHandleBase::Open(aFind,aType));}




inline TBool RChunk::IsReadable() const
/**
Tests whether the chunk is mapped into its process address space.

@return True, if the chunk is readable; false, otherwise.
*/
	{return (Attributes()&RHandleBase::EDirectReadAccess); }




inline TBool RChunk::IsWritable() const
/**
Tests whether the chunk mapped into its process address space and is writable.

@return True, if the chunk is writable; false, otherwise.
*/
	{return (Attributes()&RHandleBase::EDirectWriteAccess); }




// Class TObjectId
inline TObjectId::TObjectId()
/**
Default constructor.
*/
	{}




inline TObjectId::TObjectId(TUint64 aId)
	: iId(aId)
/**
Constructor taking an unsigned integer value.

@param aId The value of the object id.
*/
	{}




inline TUint64 TObjectId::Id() const
/**
Return the ID as a 64 bit integer
*/
	{ return iId; }




inline TObjectId::operator TUint() const
/**
Conversion operator invoked by the compiler when a TObjectId type is passed
to a function that is prototyped to take a TUint type.

@see TUint
*/
	{ return TUint(iId); }




inline TBool TObjectId::operator==(TObjectId aId) const
/**
Tests whether this thread Id is equal to the specified Id.

@param aId The thread Id to be compared with this thread Id.

@return True, if the thread Ids are equal; false otherwise.
*/
	{return iId==aId.iId;}




inline TBool TObjectId::operator!=(TObjectId aId) const
/**
Tests whether this thread Id is unequal to the specified thread Id.

@param aId The thread Id to be compared with this thread Id.

@return True, if the thread Ids are unequal; false otherwise.
*/
	{return iId!=aId.iId;}




// Class TThreadId
inline TThreadId::TThreadId()
	: TObjectId()
/**
Default constructor.
*/
	{}




inline TThreadId::TThreadId(TUint64 aId)
	: TObjectId(aId)
/**
Constructor taking an unsigned integer value.

@param aId The value of the thread id.
*/
	{}




// Class RThread
inline RThread::RThread()
	: RHandleBase(KCurrentThreadHandle)
/**
Default constructor.

The constructor exists to initialise private data within this handle; it does 
not create the thread object.

Specifically, it sets the handle-number to the value KCurrentThreadHandle.
In effect, the constructor creates a default thread handle.
*/
	{}




inline TInt RThread::Open(const TFindThread& aFind,TOwnerType aType)
/**
Opens a handle to the thread found by pattern matching a name.

A TFindThread object is used to find all threads whose full names match a 
specified pattern. 

By default, ownership of this thread handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aFind A reference to the TFindThread object used to find the thread.
@param aType An enumeration whose enumerators define the ownership of this 
             thread handle. If not explicitly specified, EOwnerProcess is
			 taken as default.

@return KErrNone if successful, otherwise one of the other system-wide error codes.
*/
	{return(RHandleBase::Open(aFind,aType));}




#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TBool RThread::HasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability, aDiagnostic);
	}

inline TBool RThread::HasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability1, aCapability2, aDiagnostic);
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

// Only available to NULL arguments
inline TBool RThread::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}

inline TBool RThread::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
// For things using KSuppressPlatSecDiagnostic
inline TBool RThread::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool RThread::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability1, aCapability2, KSuppressPlatSecDiagnosticMagicValue);
	}

#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

// Class TProcessId
inline TProcessId::TProcessId()
	: TObjectId()
/**
Default constructor.
*/
	{}




inline TProcessId::TProcessId(TUint64 aId)
	: TObjectId(aId)
/**
Constructor taking an unsigned integer value.

@param aId The value of the process id.
*/
	{}




// Class RProcess
inline RProcess::RProcess()
	: RHandleBase(KCurrentProcessHandle)
/** 
Default constructor.

The constructor exists to initialise private data within this handle; it does 
not create the process object.

Specifically, it sets the handle-number to the value KCurrentProcessHandle.
In effect, the constructor creates a default process handle.
*/
	{}




inline RProcess::RProcess(TInt aHandle)
	: RHandleBase(aHandle)
/**
Constructor taking a handle number.

@param aHandle The handle number to be used to construct this RProcess handle.
*/
	{}




inline TInt RProcess::Open(const TFindProcess& aFind,TOwnerType aType)
/**
Opens a handle to the process found by pattern matching a name.

A TFindProcess object is used to find all processes whose full names match 
a specified pattern. 

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aFind A reference to the TFindProcess object used to find the process.
@param aType An enumeration whose enumerators define the ownership of this 
             process handle. If not explicitly specified, EOwnerProcess is taken
			 as default.

@return KErrNone if successful, otherwise one of the other system-wide error codes.
*/
	{return(RHandleBase::Open(aFind,aType));}




#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TBool RProcess::HasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability, aDiagnostic);
	}

inline TBool RProcess::HasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability1, aCapability2, aDiagnostic);
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

// Only available to NULL arguments
inline TBool RProcess::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}

inline TBool RProcess::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
// For things using KSuppressPlatSecDiagnostic
inline TBool RProcess::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool RProcess::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability1, aCapability2, KSuppressPlatSecDiagnosticMagicValue);
	}

#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__





// Class RSessionBase


/**
Creates a session with a server, specifying no message slots.

It should be called as part of session initialisation in the derived class.

Message slots are not pre-allocated for the session but are taken from
a system-wide pool allowing up to 255 asynchronous messages to be outstanding.
This raises a risk of failure due to lack of memory and, therefore, this mode
of operation is not viable for sessions that make guarantees about the failure
modes of asynchonous services.

@param aServer  The name of the server with which a session is to
                be established.
@param aVersion The lowest version of the server with which this client
                is compatible

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
inline TInt RSessionBase::CreateSession(const TDesC& aServer,const TVersion& aVersion)
	{return CreateSession(aServer,aVersion,-1,EIpcSession_Unsharable,NULL,0);}




/**
Creates a session with a server, specifying no message slots.

It should be called as part of session initialisation in the derived class.

Message slots are not pre-allocated for the session but are taken from
a system-wide pool allowing up to 255 asynchronous messages to be outstanding.
This raises a risk of failure due to lack of memory and, therefore, this mode
of operation is not viable for sessions that make guarantees about the failure
modes of asynchonous services.

@param aServer  A handle to a server with which a session is to be established.
@param aVersion The lowest version of the server with which this client
                is compatible

@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
inline TInt RSessionBase::CreateSession(RServer2 aServer,const TVersion& aVersion)
	{return CreateSession(aServer,aVersion,-1,EIpcSession_Unsharable,NULL,0);}




/**
Issues a blind request to the server with the specified function number,
and arguments.

A blind request is one where the server does not issue a response
to the client.

@param aFunction The function number identifying the request.
@param aArgs     A set of up to 4 arguments and their types to be passed
                 to the server.

@return KErrNone, if the send operation is successful;
        KErrServerTerminated, if the server no longer present;
        KErrServerBusy, if there are no message slots available;
        KErrNoMemory, if there is insufficient memory available.

@panic  USER 72 if the function number is negative.
*/
inline TInt RSessionBase::Send(TInt aFunction,const TIpcArgs& aArgs) const
	{return DoSend(aFunction,&aArgs);}




/**
Issues an asynchronous request to the server with the specified function
number and arguments. 

The completion status of the request is returned via the request
status object, aStatus. 

@param aFunction The function number identifying the request.
@param aArgs     A set of up to 4 arguments and their types to be passed
                 to the server.
@param aStatus   The request status object used to contain the completion status
                 of the request.
                 
@panic  USER 72  if the function number is negative.                 
*/
inline void RSessionBase::SendReceive(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus) const
	{DoSendReceive(aFunction,&aArgs,aStatus);}




/**
Issues a synchronous request to the server with the specified function number
and arguments.

@param aFunction The function number identifying the request.
@param aArgs     A set of up to 4 arguments and their types to be passed
                 to the server.

@return KErrNone, if the send operation is successful;
        KErrServerTerminated, if the server no longer present;
        KErrServerBusy, if there are no message slots available;
        KErrNoMemory, if there is insufficient memory available.

@panic  USER 72  if the function number is negative.
*/
inline TInt RSessionBase::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{return DoSendReceive(aFunction,&aArgs);}




/**
Issues a blind request to the server with the specified function number,
but with no arguments.

A blind request is one where the server does not issue a response
to the client.

@param aFunction The function number identifying the request.

@return KErrNone, if the send operation is successful;
        KErrServerTerminated, if the server no longer present;
        KErrServerBusy, if there are no message slots available;
        KErrNoMemory, if there is insufficient memory available.

@panic  USER 72 if the function number is negative.
*/
inline TInt RSessionBase::Send(TInt aFunction) const
	{return DoSend(aFunction,NULL);}




/**
Issues an asynchronous request to the server with the specified function
number, but with no arguments.

The completion status of the request is returned via the request
status object, aStatus. 

@param aFunction The function number identifying the request.
@param aStatus   The request status object used to contain the completion
                 status of the request.
                 
@panic  USER 72  if the function number is negative.                 
*/
inline void RSessionBase::SendReceive(TInt aFunction,TRequestStatus& aStatus) const
	{ DoSendReceive(aFunction,NULL,aStatus);}




/**
Sets the handle-number of this handle to the specified 
value.

The function can take a (zero or positive) handle-number,
or a (negative) error number.

If aHandleOrError represents a handle-number, then the handle-number of this handle
is set to that value.
If aHandleOrError represents an error number, then the handle-number of this handle is set to zero
and the negative value is returned.

@param aHandleOrError A handle-number, if zero or positive; an error value, if negative.

@return KErrNone, if aHandle is a handle-number; the value of aHandleOrError, otherwise.
*/
inline TInt RSessionBase::SetReturnedHandle(TInt aHandleOrError)
	{ return RHandleBase::SetReturnedHandle(aHandleOrError);}




inline TInt RSessionBase::SetReturnedHandle(TInt aHandleOrError,RHandleBase& aHandle)
	{ return RHandleBase::SetReturnedHandle(aHandleOrError,aHandle);}
/**
Issues a synchronous request to the server with the specified function number,
but with no arguments.

@param aFunction The function number identifying the request.

@return KErrNone, if the send operation is successful;
        KErrServerTerminated, if the server no longer present;
        KErrServerBusy, if there are no message slots available;
        KErrNoMemory, if there is insufficient memory available.

@panic  USER 72  if the function number is negative.
*/
inline TInt RSessionBase::SendReceive(TInt aFunction) const
	{return DoSendReceive(aFunction,NULL);}




// Class RSubSessionBase
inline RSubSessionBase::RSubSessionBase()
	: iSubSessionHandle(0)
/**
Default constructor
*/
	{}




inline TInt RSubSessionBase::SubSessionHandle() const
/**
Gets the sub-session handle number.

This number is automatically passed to the server when making requests and is
used to identify the appropriate server-side sub-session.

@return The sub-session handle number.
*/
	{return iSubSessionHandle;}




/**
Creates a new sub-session within an existing session.

@param aSession    The session to which this sub-session will belong.
@param aFunction   The opcode specifying the requested service; the server should interpret this as a request to create a sub-session.
@param aArgs       The message arguments.   

@return            KErrNone if successful, otherwise one of the system-wide error codes.
*/
inline TInt RSubSessionBase::CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs)
	{ return DoCreateSubSession(aSession,aFunction,&aArgs); }



	
/**
Creates a new sub-session within an existing session.

This variant sends no message arguments to the server.

@param aSession    The session to which this sub-session will belong.
@param aFunction   The opcode specifying the requested service; the server should interpret this as a request to create a sub-session.

@return            KErrNone if successful, otherwise one of the system-wide error codes.
*/
inline TInt RSubSessionBase::CreateSubSession(const RSessionBase& aSession,TInt aFunction)
	{ return DoCreateSubSession(aSession,aFunction,NULL); }



	
/**
Sends a blind message to the server - no reply is expected.

A set of message arguments is passed that can be used to specify client
addresses, which the server can use to read from and write to the client
address space.

Note that this function can fail if there are no available message-slots, either
in the system wide pool (if this is being used), or in the session reserved pool
(if this is being used). If the client request is synchronous, then always use
the synchronous variant of SendReceive(); this is guaranteed to reach the server.

@param aFunction	The opcode specifying the requested service.
@param aArgs		The message arguments.
                    
@return				KErrNone if successful, otherwise one of the system-wide error codes.                    
*/
inline TInt RSubSessionBase::Send(TInt aFunction,const TIpcArgs& aArgs) const
	{return DoSend(aFunction,&aArgs);}



	
/**
Sends a message to the server and waits asynchronously for the reply.

An opcode specifies the service required.
A set of message arguments is passed that can be used to specify client addresses,
which the server can use to read from and write to the client address space.

Note that this function can fail if there are no available message-slots, 
either in the system wide pool (if this is being used), or in the session
reserved pool (if this is being used). If the client request is synchronous,
then always use the synchronous variant of SendReceive();
this is guaranteed to reach the server.

@param aFunction	The opcode specifying the requested service.
@param aArgs		The message arguments.
@param aStatus	    A request status which indicates the completion status of the asynchronous request.
*/
inline void RSubSessionBase::SendReceive(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus) const
	{DoSendReceive(aFunction,&aArgs,aStatus);}



	
/**
Sends a message to the server and waits synchronously for a reply.

An opcode specifies the service required.
A set of message arguments is passed that can be used to specify client addresses,
which the server can use to read from and write to the client address space.

Note that this function will only fail if the server itself fails or environmental
errors occur in the server. All requests made using this function are guaranteed to
reach the server. This means that all synchronous client requests (typically those
that return void) should be routed through this synchronous variant of SendReceive().

@param aFunction	The opcode specifying the requested service.
@param aArgs		The message arguments.

@return				KErrNone if successful, otherwise one of the system-wide error codes. 
*/
inline TInt RSubSessionBase::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{return DoSendReceive(aFunction,&aArgs);}
	


	
/**
Sends a blind message to the server - no reply is expected.

This variant sends no message arguments to the server.

@param aFunction	The opcode specifying the requested service.
                    
@return				KErrNone if successful, otherwise one of the system-wide error codes. 
*/
inline TInt RSubSessionBase::Send(TInt aFunction) const
	{return DoSend(aFunction,NULL);}



	
/**
Sends a message to the server and waits asynchronously for the reply.

An opcode specifies the service required.
This variant sends no message arguments to the server.

@param aFunction	The opcode specifying the requested service.
@param aStatus	    A request status which indicates the completion status of the asynchronous request.
*/
inline void RSubSessionBase::SendReceive(TInt aFunction,TRequestStatus& aStatus) const
	{ DoSendReceive(aFunction,NULL,aStatus);}



	
/**
Sends a message to the server and waits synchronously for a reply.

An opcode specifies the service required.
This variant sends no message arguments to the server.

@param aFunction	The opcode specifying the requested service.

@return				KErrNone if successful, otherwise one of the system-wide error codes. 
*/
inline TInt RSubSessionBase::SendReceive(TInt aFunction) const
	{return DoSendReceive(aFunction,NULL);}




// Class RRefBase

/**
Default constructor.
*/
inline RRefBase::RRefBase()
	: iPtr(NULL)
	{}



/**
Copy constructor.

@param aRef A reference to the object to be copied.
*/
inline RRefBase::RRefBase(const RRefBase &aRef)
	{Copy(aRef);}




// Class RRef


/**
Default constructor.
*/
template <class T>
inline RRef<T>::RRef()
	{}



/**
Copy constructor.

The constructor frees any existing contained object, and takes ownership of
the object owned by anObject. 

@param anObject A reference to another 'reference' object.
                On return from this constructor, anObject may be safely
                orphaned if it lives on the program stack.
*/
template <class T>
inline RRef<T>::RRef(const RRef<T> &anObject)
	{Copy(anObject);}




/**
Assignment operator.

The constructor frees any existing contained object, and takes ownership of
the object owned by anObject. 

@param anObject A reference to another 'reference' object.
                On return from this constructor, anObject may be safely
                orphaned if it lives on the program stack.
*/
template <class T>
inline void RRef<T>::operator=(const RRef<T> &anObject)
	{Copy(anObject);}




/**
Gets a pointer to the contained object.

@return A pointer to the contained object
*/
template <class T>
inline T *RRef<T>::operator->()
	{return((T *)iPtr);}




/**
Gets a pointer to the contained object.

@return A pointer to the contained object
*/
template <class T>
inline RRef<T>::operator T*()
	{return((T *)iPtr);}




/**
Creates a copy of the specified object, which is to be contained by
this reference object.

The amount of memory set aside to contain the object is defined by the size
of the object

@param anObject The object to be packaged up by this reference object.
*/
template <class T>
void RRef<T>::Alloc(const T &anObject)
	{DoAlloc(&anObject,sizeof(T));}




/**
Creates a copy of the specified object, which is to be contained by
this reference object.

The amount of memory set aside to contain the object is defined by aSize.

@param anObject The object to be packaged up by this reference object.
@param aSize    The amount of memory to be set aside to contain the object.
                You must make sure that this is big enough.
*/
template <class T>
void RRef<T>::Alloc(const T &anObject,TInt aSize)
	{DoAlloc(&anObject,aSize);}




/**
Creates a copy of the specified object, which is to be contained by
this reference object, and leaves on failure.

The amount of memory set aside to contain the object is defined by the size
of the object

@param anObject The object to be packaged up by this reference object.
*/
template <class T>
void RRef<T>::AllocL(const T &anObject)
	{DoAllocL(&anObject,sizeof(T));}




/**
Creates a copy of the specified object, which is to be contained by
this reference object, and leaves on failure.

The amount of memory set aside to contain the object is defined by aSize.

@param anObject The object to be packaged up by this reference object.
@param aSize    The amount of memory to be set aside to contain the object.
                You must make sure that this is big enough.
*/
template <class T>
void RRef<T>::AllocL(const T &anObject,TInt aSize)
	{DoAllocL(&anObject,aSize);}




// Class TRegion
inline TBool TRegion::CheckError() const
/** 
Tests whether the region's error flag is set.

The error flag may be set:

1. when an attempt to allocate more memory for the region fails

2. if an attempt is made to expand a fixed size region beyond its allocated
   size

3. if ForceError() has been called.

Use Clear() to unset the error flag, clear the region and free all allocated 
memory.

@return True, if the error flag is set; false, otherwise. 

@see TRegion::ForceError
@see TRegion::Clear
*/
	{return(iError);}




inline TInt TRegion::Count() const
/**
Gets the number of rectangles in this region.

@return The number of rectangles.
*/
	{return(iCount);}




inline const TRect *TRegion::RectangleList() const
/**
Gets a pointer to the array of rectangles defining this region.

@return Pointer to the array of rectangles. Note that array is a standard 
        C++ array, i.e. a concatenated set of TRect objects. Use Count() to
		get the number of rectangles.

@see TRegion::Count
*/
	{return(((TRegion *)this)->RectangleListW());}




inline TRegion::TRegion()
	{}




// Class RRegion
inline TInt RRegion::CheckSpare() const
/**
Gets the number of free memory slots in the region.

This is the number of slots which have been allocated, minus the number in 
use.

@return The number of free memory slots in the region.
*/
	{return(iAllocedRects-iCount);}




// Class TRegionFix
template <TInt S>
inline TRegionFix<S>::TRegionFix() : TRegion(-S)
/**
Constructs a default fixed size region.
*/
	{}




template <TInt S>
inline TRegionFix<S>::TRegionFix(const TRect &aRect) : TRegion(-S)
/**
Constructs a fixed size region with a TRect.

@param aRect Rectangle to be added to the newly constructed region.
*/
	{AddRect(aRect);}




template <TInt S>
inline TRegionFix<S>::TRegionFix(const TRegionFix<S> &aRegion)
/**
Copy constructor.

@param aRegion The TRegionFix object to be copied.
*/
	{*this=aRegion;}




template <TInt S>
inline RRegionBuf<S>::RRegionBuf() : RRegion(-S&(~ERRegionBuf),S)
/**
Constructs a default object.

The granularity is the value of the template parameter.
*/
	{}



template <TInt S>
inline RRegionBuf<S>::RRegionBuf(const RRegion &aRegion) 
/**
Constructs this object from the specified RRegion.

@param aRegion The region to assign to this RRegionBuf.
*/
	{*this=aRegion;}




template <TInt S>
inline RRegionBuf<S>::RRegionBuf(const TRect &aRect) : RRegion(-S&(~ERRegionBuf),S)
/**
Constructs an RRegionBuf with a TRect.

Its granularity is initialised to the value contained in the template argument.
The resulting region consists of the specified single rectangle.

@param aRect The single rectangle with which to initialise the region.
*/
	{AddRect(aRect);}




template <TInt S>
inline RRegionBuf<S>::RRegionBuf(const RRegionBuf<S> &aRegion)
/**
Copy constructs from an existing RRegionBuf object.

@param aRegion The RRegionBuf to be copied.
*/
    {*this=aRegion;}




// enum TTimerLockSpec
inline TTimerLockSpec &operator++(TTimerLockSpec &aLock)
	{
	return aLock=((aLock==ETwelveOClock) ? EOneOClock : (TTimerLockSpec)((TInt)aLock+1));
	}
inline TTimerLockSpec operator++(TTimerLockSpec &aLock, TInt)
	{
	TTimerLockSpec l=aLock;
	aLock=((aLock==ETwelveOClock) ? EOneOClock : (TTimerLockSpec)((TInt)aLock+1));
	return l;
	}




// Class TCheckedUid
inline const TUidType& TCheckedUid::UidType() const
/**
Gets the Uid type contained in this object.

@return The Uid type.
*/
    {return(iType);}




// Array deletion support, uses CBase deletion (virtual d'tor) for all C-classes
template <class T>
/**	@internalComponent
*/
void _DeleteArray(T** aBegin,T** aEnd)
	{for (;;) if (aBegin<aEnd) delete *aBegin++; else return;}

template <class T>
/**	@internalComponent
*/
struct _ArrayUtil
	{
	static inline void Delete(T* aBegin,T* aEnd,CBase*)
		{::_DeleteArray((CBase**)aBegin,(CBase**)aEnd);}
	static inline void Delete(T* aBegin,T* aEnd,TAny*)
		{::_DeleteArray(aBegin,aEnd);}
	static inline void Delete(T* aArray,TInt aCount)
		{Delete(aArray,aArray+aCount,*aArray);}
	};




#ifndef __TOOLS__
// Template class TFixedArray
IMPORT_C void PanicTFixedArray();




template <class T,TInt S>
inline TFixedArray<T,S>::TFixedArray()
/**
Default constructor.

Constructs an uninitialised C++ array.
*/
	{}




template <class T,TInt S>
inline void TFixedArray<T,S>::Copy(const T* aList,TInt aLength)
/**
Copies the specified set of contiguous objects into the C++ array.

The copy operation starts at the beginning of the array, replacing
any existing data.

@param aList   A pointer to a set of contiguous objects. 
@param aLength The number of contiguous objects to be copied. This value must
               not be negative and must not be greater than the size of the
			   array as defined by the integer template parameter.

@panic USER 133, in a debug build only, if aLength is negative or is greater
       than the size of the array as defined by the integer template parameter.
*/
	{__ASSERT_DEBUG(TUint(aLength)<=TUint(S),PanicTFixedArray());Mem::Copy(iRep,aList,aLength*sizeof(T));}




template <class T,TInt S>
inline TFixedArray<T,S>::TFixedArray(const T* aList,TInt aLength)
/**
Constructs a C++ array initialised with the specified objects.

@param aList   A pointer to a set of contiguous objects. 
@param aLength The number of contiguous objects to be copied. This value must
               not be negative and must not be greater than the size of the
			   array as defined by the integer template parameter.

@panic USER 133, in a debug build only, if aLength is negative or is greater
       than the size of the array as defined by the integer template parameter.
*/
	{Copy(aList,aLength);}




template <class T,TInt S>
inline void TFixedArray<T,S>::Reset()
/**
Fills every element of the array with binary zeroes.
*/
	{Mem::FillZ(iRep,sizeof(iRep));}




template <class T,TInt S>
inline TInt TFixedArray<T,S>::Count() const
/**
Gets the size of the array.

For any instance of this class, the array size 
is fixed and has the same value as the integer template parameter.

@return The size of the array.
*/
	{return S;}




template <class T,TInt S>
inline TInt TFixedArray<T,S>::Length() const
/**
Gets the size of an array element, in bytes.

@return The size of an array element, in bytes.
*/
	{return sizeof(T);}




template <class T,TInt S>
inline TBool TFixedArray<T,S>::InRange(TInt aIndex)
	{return TUint(aIndex)<S;}




template <class T,TInt S>
inline T& TFixedArray<T,S>::operator[](TInt aIndex)
/**
Gets a reference to the specified element within the C++ array.

@param aIndex The position of the element within the array. This is an offset value; 
              a zero value refers to the first element in the array. This value must be 
              greater than or equal to zero and less than the size of the array.

@return A reference to an element of the array.

@panic USER 133, in a debug build only, if aIndex is negative or greater than or equal to the size
       of the array as defined by the integer template parameter.
*/
	{__ASSERT_DEBUG(InRange(aIndex),PanicTFixedArray());return iRep[aIndex];}




template <class T,TInt S>
inline const T& TFixedArray<T,S>::operator[](TInt aIndex) const
/**
Gets a const reference to the specified element within the C++ array.

@param aIndex The position of the element within the array. This is an offset value; 
              a zero value refers to the first element in the array. This value must be 
              greater than or equal to zero and less than the size of the array.

@return A const reference to an element of the array; the element cannot be 
        changed through this reference.

@panic USER 133, in a debug build only, if aIndex is negative or greater than or equal to the size
       of the array as defined by the integer template parameter.
*/
	{return CONST_CAST(ThisClass&,*this)[aIndex];}




template <class T,TInt S>
inline T& TFixedArray<T,S>::At(TInt aIndex)
/**
Gets a reference to the specified element within the C++ array.

@param aIndex The position of the element within the array. This is an offset value; 
              a zero value refers to the first element in the array. This value must be 
              greater than or equal to zero and less than the size of the array.

@return A reference to an element of the array.

@panic USER 133, if aIndex is negative or greater than or equal to the size
       of the array as defined by the integer template parameter.
*/
	{__ASSERT_ALWAYS(InRange(aIndex),PanicTFixedArray());return iRep[aIndex];}




template <class T,TInt S>
inline const T& TFixedArray<T,S>::At(TInt aIndex) const
/**
Gets a const reference to the specified element within the C++ array.

@param aIndex The position of the element within the array. This is an offset value; 
              a zero value refers to the first element in the array. This value must be 
              greater than or equal to zero and less than the size of the array.

@return A const reference to an element of the array; the element cannot be 
        changed through this reference.

@panic USER 133, in a debug build only, if aIndex is negative or greater than or equal to the size
       of the array as defined by the integer template parameter.
*/
	{return CONST_CAST(ThisClass&,*this).At(aIndex);}




template <class T,TInt S>
inline T* TFixedArray<T,S>::Begin()
/**
Gets a pointer to the first element of the array.

@return A pointer to the first element of the array.
*/
	{return &iRep[0];}




template <class T,TInt S>
inline T* TFixedArray<T,S>::End()
/**
Gets a pointer to the first byte following the end of the array.

@return A pointer to the first byte following the end of the array.
*/
	{return &iRep[S];}




template <class T,TInt S>
inline const T* TFixedArray<T,S>::Begin() const
/**
Gets a pointer to the first element of the array.

@return A pointer to a const element of the array. 
*/
	{return &iRep[0];}




template <class T,TInt S>
inline const T* TFixedArray<T,S>::End() const
/**
Gets a pointer to the first byte following the end of the array.

@return A pointer to the const first byte following the end of the array.
*/
	{return &iRep[S];}




template <class T,TInt S>
inline TInt TFixedArray<T,S>::CountFunctionR(const CBase*)
	{return S;}




template <class T,TInt S>
inline const TAny* TFixedArray<T,S>::AtFunctionR(const CBase* aThis,TInt aIndex)
	{return &REINTERPRET_CAST(const ThisClass&,*aThis)[aIndex];}




template <class T,TInt S>
inline TArray<T> TFixedArray<T,S>::Array() const
/**
Creates and returns a generic array for this C++ array.

@return A generic array for this C++ array.
*/
	{return TArray<T>(CountFunctionR,AtFunctionR,REINTERPRET_CAST(const CBase*,this));}




template <class T,TInt S>
inline void TFixedArray<T,S>::DeleteAll()
/**
Invokes the delete operator on every member of the array.

The function can only be used for arrays of pointers to CBase derived objects.

If the array is to be used after a call to this function, it is good practice 
to call TFixedArray<class T,TInt S>::Reset() to set all array elements to 
NULL.
*/
	{_ArrayUtil<T>::Delete(iRep,S);}
#endif




// class User

inline RHeap* User::SwitchHeap(RAllocator* aHeap)
/**
Changes the current thread's heap.
	
@param aHeap A pointer to the new heap handle.

@return A pointer to the old heap handle.
*/
	{ return (RHeap*)SwitchAllocator(aHeap); }




inline RHeap& User::Heap()
/**
Gets a reference to the handle to the current thread's heap.
	
@return A reference to the handle to the current thread's heap.
*/
	{ return (RHeap&)Allocator(); }




#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TBool User::CreatorHasCapability(TCapability aCapability, const char* aDiagnostic)
	{
	return DoCreatorHasCapability(aCapability, aDiagnostic);
	}

inline TBool User::CreatorHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic)
	{
	return DoCreatorHasCapability(aCapability1, aCapability2, aDiagnostic);
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

// Only available to NULL arguments
inline TBool User::CreatorHasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/)
	{
	return DoCreatorHasCapability(aCapability);
	}

inline TBool User::CreatorHasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/)
	{
	return DoCreatorHasCapability(aCapability1, aCapability2);
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
// For things using KSuppressPlatSecDiagnostic
inline TBool User::CreatorHasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/)
	{
	return DoCreatorHasCapability(aCapability, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool User::CreatorHasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/)
	{
	return DoCreatorHasCapability(aCapability1, aCapability2, KSuppressPlatSecDiagnosticMagicValue);
	}

#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__


inline const TAny* User::LeaveIfNull(const TAny* aPtr)
/**
Leaves with the reason code KErrNoMemory, if the specified pointer is NULL. 

If the pointer is not NULL, the function simply returns with the value of 
the pointer.

Used to check pointers to const objects.

@param aPtr The pointer to be tested.

@return If the function returns, the value of aPtr.
*/
	{ return (const TAny*)LeaveIfNull((TAny*)aPtr); }

/** Sets this TSecurityInfo to the security attributes of this process. */
inline void TSecurityInfo::SetToCurrentInfo()
	{ new (this) TSecurityInfo(RProcess()); }

/** Constructs a TSecurityInfo using the security attributes of aProcess */
inline void TSecurityInfo::Set(RProcess aProcess)
	{ new (this) TSecurityInfo(aProcess); }

/** Constructs a TSecurityInfo using the security attributes of the process
owning aThread 
*/
inline void TSecurityInfo::Set(RThread aThread)
	{ new (this) TSecurityInfo(aThread); }

/** Constructs a TSecurityInfo using the security attributes of the process
which sent the message aMsgPtr */
inline void TSecurityInfo::Set(RMessagePtr2 aMsgPtr)
	{ new (this) TSecurityInfo(aMsgPtr); }

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aProcess, aDiagnostic);
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aThread, aDiagnostic);
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aMsgPtr, aDiagnostic);
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aMissing A TSecurityInfo object which this method fills with any capabilities or IDs
				it finds to be missing. 
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object

@internalComponent
*/
inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aMsgPtr, aMissing, aDiagnostic);
	}

/** Checks this policy against the platform security attributes of this process' creator.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of this process' creator, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicyCreator(const char* aDiagnostic) const
	{
	return DoCheckPolicyCreator(aDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic) const
	{
	return (&(*this))->CheckPolicy(aProcess, aDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic) const
	{
	return (&(*this))->CheckPolicy(aThread, aDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr, aDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic)
@internalComponent
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr, aMissing, aDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicyCreator(const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicyCreator(const char* aDiagnostic) const
	{
	return (&(*this))->CheckPolicyCreator(aDiagnostic);
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RProcess aProcess, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aProcess);
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RThread aThread, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aThread);
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aMsgPtr);
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aMissing A TSecurityInfo object which this method fills with any capabilities or IDs
				it finds to be missing. 
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object

@internalComponent
*/
inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aMsgPtr, aMissing);
	}

/** Checks this policy against the platform security attributes of this process' creator.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of this process' creator, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicyCreator(OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicyCreator();
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
inline TBool TSecurityPolicy::CheckPolicy(RProcess aProcess, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoCheckPolicy(aProcess, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool TSecurityPolicy::CheckPolicy(RThread aThread, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoCheckPolicy(aThread, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoCheckPolicy(aMsgPtr, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoCheckPolicy(aMsgPtr, aMissing, KSuppressPlatSecDiagnosticMagicValue);
	}

inline TBool TSecurityPolicy::CheckPolicyCreator(OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoCheckPolicyCreator(KSuppressPlatSecDiagnosticMagicValue);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/**
@see TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RProcess aProcess, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return (&(*this))->CheckPolicy(aProcess);
	}

/**
@see TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RThread aThread, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return (&(*this))->CheckPolicy(aThread);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic)
@internalComponent
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr, aMissing);
	}

/**
@see TSecurityPolicy::CheckPolicyCreator(const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicyCreator(OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return (&(*this))->CheckPolicyCreator();
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/**
@see TSecurityPolicy::CheckPolicy(RProcess aProcess, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RProcess aProcess, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return (&(*this))->CheckPolicy(aProcess, KSuppressPlatSecDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RThread aThread, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RThread aThread, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return (&(*this))->CheckPolicy(aThread, KSuppressPlatSecDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr, KSuppressPlatSecDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic)
@internalComponent
*/
inline TBool TStaticSecurityPolicy::CheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return (&(*this))->CheckPolicy(aMsgPtr, aMissing, KSuppressPlatSecDiagnostic);
	}

/**
@see TSecurityPolicy::CheckPolicyCreator(const char* aDiagnostic)
*/
inline TBool TStaticSecurityPolicy::CheckPolicyCreator(OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return (&(*this))->CheckPolicyCreator(KSuppressPlatSecDiagnostic);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#endif //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__



#ifndef __KERNEL_MODE__

/**
Appends an object pointer onto the array.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be appended.
*/
template <class T>
inline void RPointerArray<T>::AppendL(const T* anEntry)
	{ User::LeaveIfError(Append(anEntry));}


/**
Inserts an object pointer into the array at the specified position.

The function leaves with one of the system wide error codes, if
the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be inserted.
@param aPos    The position within the array where the object pointer is to be 
               inserted. The position is relative to zero, i.e. zero implies
			   that a pointer is inserted at the beginning of the array.

@panic USER 131, if aPos is negative, or is greater than the number of object
       pointers currently in the array.
*/
template <class T>
inline void RPointerArray<T>::InsertL(const T* anEntry, TInt aPos)
	{ User::LeaveIfError(Insert(anEntry,aPos)); }


/**
Finds the first object pointer in the array which matches the specified object 
pointer, using a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@return The index of the first matching object pointer within the array.
@leave KErrNotFound, if no matching object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindL(const T* anEntry) const
	{ return User::LeaveIfError(Find(anEntry));}


/**
Finds the first object pointer in the array whose object matches the specified 
object, using a sequential search and a matching algorithm.

The algorithm for determining whether two class T objects match is provided 
by a function supplied by the caller.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry    The object pointer to be found.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T objects match.

@return The index of the first matching object pointer within the array.
@leave  KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindL(const T* anEntry, TIdentityRelation<T> anIdentity) const
	{ return User::LeaveIfError(Find(anEntry, anIdentity));}


/**
Finds the last object pointer in the array which matches the specified object 
pointer, using a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@return The index of the last matching object pointer within the array.
@leave KErrNotFound, if no matching object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindReverseL(const T* anEntry) const
	{ return User::LeaveIfError(FindReverse(anEntry));}


/**
Finds the last object pointer in the array whose object matches the specified 
object, using a sequential search and a matching algorithm.

The algorithm for determining whether two class T objects match is provided 
by a function supplied by the caller.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry    The object pointer to be found.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T objects match.

@return The index of the last matching object pointer within the array.
@leave  KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindReverseL(const T* anEntry, TIdentityRelation<T> anIdentity) const
	{ return User::LeaveIfError(FindReverse(anEntry, anIdentity));}


/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that object pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.

@return The index of the matching object pointer within the array
@leave KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindInAddressOrderL(const T* anEntry) const
	{ return User::LeaveIfError(FindInAddressOrder(anEntry));}


/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an algorithm 
supplied by the caller and packaged as a TLinearOrder<T>.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return The index of the matching object pointer within the array.

@leave KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline TInt RPointerArray<T>::FindInOrderL(const T* anEntry, TLinearOrder<T> anOrder) const
	{ return User::LeaveIfError(FindInOrder(anEntry, anOrder));}


/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that object pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anIndex A reference to a TInt into which the
               function puts an index value: If the function does not leave,
               this is the index of the matching object pointer within the
               array. If the function leaves with KErrNotFound, this is the
               index of the first object pointer within the array which
               logically follows after anEntry.

@leave KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline void RPointerArray<T>::FindInAddressOrderL(const T* anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInAddressOrder(anEntry, anIndex)); }


/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an
algorithm supplied by the caller and packaged as a TLinearOrder<T>.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an
               index value:
               If the function does not leave, this is the index of the
               matching object pointer within the array. 
               If the function leaves with KErrNotFound, this is the index of
               the first object pointer in the array whose object is bigger
               than the entry being searched for - if no objects pointed to in
               the array are bigger, then the index value is the same as the
               total number of object pointers in the array.

@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@leave         KErrNotFound, if no suitable object pointer can be found.
*/
template <class T>
inline void RPointerArray<T>::FindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const
	{ User::LeaveIfError(FindInOrder(anEntry, anIndex, anOrder)); }


/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that object pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param	anEntry The object pointer to be found.
@param	aMode   Specifies whether to find the first match, the last match or
                any match, as defined by one of the TArrayFindMode enum values.

@return If there is a matching element, the array index of a matching element -  what
        the index refers to depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows the
        last matching element - if the last matching element is also the last element of the array,
        then the index value is the same as the total number of elements in the array.
        
@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline TInt RPointerArray<T>::SpecificFindInAddressOrderL(const T* anEntry, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInAddressOrder(anEntry, aMode));}


/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

In the case that there is more than one matching element finds the first, last
or any match as specified by the value of aMode.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an algorithm 
supplied by the caller and packaged as a TLinearOrder<T> type.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.

@return If there is a matching element, the array index of a matching
        element -  what the index refers to depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of the array, then
        the index value is the same as the total number of elements in the array.

@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline TInt RPointerArray<T>::SpecificFindInOrderL(const T* anEntry, TLinearOrder<T> anOrder, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInOrder(anEntry, anOrder, aMode));}


/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

Where there is more than one matching element, it finds the first, the last or
any matching element as specified by the value of aMode.

The function assumes that object pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the  index
               of the first element in the array that is bigger than the element being
               searched for - if no elements in the array are bigger, then the index
               value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends
               on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by
               one of the TArrayFindMode enum values.

@leave  KErrNotFound, if no suitable object pointer can be found.

@see TArrayFindMode
*/
template <class T>
inline void RPointerArray<T>::SpecificFindInAddressOrderL(const T* anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInAddressOrder(anEntry, anIndex, aMode)); }


/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

Where there is more than one matching element, it finds the first, the last or any
matching element as specified by the value of aMode.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an
algorithm supplied by the caller and packaged as a TLinearOrder<T>.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is
               the index of the first element in the array that is bigger than
               the element being searched for - if no elements in the array are bigger,
               then the index value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends
               on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element of
               the array, then the index value is the same as the total number of elements in the array.

@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by
               one of the TArrayFindModeenum values.

@leave  KErrNotFound, if no suitable object pointer can be found.

@see TArrayFindMode
*/
template <class T>
inline void RPointerArray<T>::SpecificFindInOrderL(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInOrder(anEntry, anIndex, anOrder, aMode)); }


/**
Inserts an object pointer into the array in address order.

No duplicate entries are permitted.
The function assumes that existing object pointers within the array are in 
address order.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be inserted.
*/
template <class T>
inline void RPointerArray<T>::InsertInAddressOrderL(const T* anEntry)
	{ User::LeaveIfError(InsertInAddressOrder(anEntry)); }


/**
Inserts an object pointer into the array so that the object itself is in object 
order.

The algorithm for determining the order of two class T objects is provided 
by a function supplied by the caller.

No duplicate entries are permitted.

The function assumes that the array is ordered so that the referenced objects 
are in object order.

The function leaves with one of the system wide error codes, if the operation fails.

Note that the array remains unchanged following an attempt to insert a duplicate
entry.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
*/
template <class T>
inline void RPointerArray<T>::InsertInOrderL(const T* anEntry, TLinearOrder<T> anOrder)
	{ User::LeaveIfError(InsertInOrder(anEntry, anOrder)); }


/**
Inserts an object pointer into the array in address order, allowing duplicates.

If the new object pointer is a duplicate of an existing object pointer in 
the array, then the new pointer is inserted after the existing one. If more 
than one duplicate object pointer already exists in the array, then any new 
duplicate pointer is inserted after the last one.

The function assumes that existing object pointers within the array are in 
address order.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be inserted.
*/
template <class T>
inline void RPointerArray<T>::InsertInAddressOrderAllowRepeatsL(const T* anEntry)
	{ User::LeaveIfError(InsertInAddressOrderAllowRepeats(anEntry)); }


/**
Inserts an object pointer into the array so that the object itself is in object 
order, allowing duplicates

The algorithm for determining the order of two class T objects is provided 
by a function supplied by the caller.

If the specified object is a duplicate of an existing object, then the new 
pointer is inserted after the pointer to the existing object. If more than 
one duplicate object already exists, then the new pointer is inserted after 
the pointer to the last one.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The object pointer to be inserted. 
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
*/
template <class T>
inline void RPointerArray<T>::InsertInOrderAllowRepeatsL(const T* anEntry, TLinearOrder<T> anOrder)
	{ User::LeaveIfError(InsertInOrderAllowRepeats(anEntry, anOrder)); }



/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of object pointers specified. Adding new object pointers to the array 
does not result in a re-allocation of memory until the the total number of 
pointers exceeds the specified count.

@param	aCount	The number of object pointers for which space should be reserved
@leave KErrNoMemory	If the requested amount of memory could not be allocated
*/
template <class T>
inline void RPointerArray<T>::ReserveL(TInt aCount)
	{ User::LeaveIfError(RPointerArrayBase::DoReserve(aCount)); }



// Specialization for RPointerArray<TAny>

/**
Appends an pointer onto the array.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be appended.
*/
inline void RPointerArray<TAny>::AppendL(const TAny* anEntry)
	{ User::LeaveIfError(Append(anEntry));}


/**
Inserts an pointer into the array at the specified position.

The function leaves with one of the system wide error codes, if
the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be inserted.
@param aPos    The position within the array where the pointer is to be 
               inserted. The position is relative to zero, i.e. zero implies
			   that a pointer is inserted at the beginning of the array.

@panic USER 131, if aPos is negative, or is greater than the number of object
       pointers currently in the array.
*/
inline void RPointerArray<TAny>::InsertL(const TAny* anEntry, TInt aPos)
	{ User::LeaveIfError(Insert(anEntry,aPos)); }


/**
Finds the first pointer in the array which matches the specified pointer, using
a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be found.
@return The index of the first matching pointer within the array.
@leave KErrNotFound, if no matching pointer can be found.
*/
inline TInt RPointerArray<TAny>::FindL(const TAny* anEntry) const
	{ return User::LeaveIfError(Find(anEntry));}


/**
Finds the last pointer in the array which matches the specified pointer, using
a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be found.
@return The index of the last matching pointer within the array.
@leave KErrNotFound, if no matching pointer can be found.
*/
inline TInt RPointerArray<TAny>::FindReverseL(const TAny* anEntry) const
	{ return User::LeaveIfError(FindReverse(anEntry));}


/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

The function assumes that pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be found.

@return The index of the matching pointer within the array
@leave KErrNotFound, if no suitable pointer can be found.
*/
inline TInt RPointerArray<TAny>::FindInAddressOrderL(const TAny* anEntry) const
	{ return User::LeaveIfError(FindInAddressOrder(anEntry));}


/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

The function assumes that pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be found.
@param anIndex A reference to a TInt into which the
               function puts an index value: If the function does not leave,
			   this is the index of the matching pointer within the array. If the
			   function leaves with KErrNotFound, this is the index of the last
			   pointer within the array which logically precedes
			   anEntry.

@leave KErrNotFound, if no suitable pointer can be found.
*/
inline void RPointerArray<TAny>::FindInAddressOrderL(const TAny* anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInAddressOrder(anEntry, anIndex)); }


/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param	anEntry The pointer to be found.
@param	aMode   Specifies whether to find the first match, the last match or
                any match, as defined by one of the TArrayFindMode enum values.

@return If there is a matching element, the array index of a matching element -  what
        the index refers to depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows the
        last matching element - if the last matching element is also the last element of the array,
        then the index value is the same as the total number of elements in the array.
        
@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
inline TInt RPointerArray<TAny>::SpecificFindInAddressOrderL(const TAny* anEntry, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInAddressOrder(anEntry, aMode));}


/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

Where there is more than one matching element, it finds the first, the last or
any matching element as specified by the value of aMode.

The function assumes that pointers in the array are in address order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the  index
               of the first element in the array that is bigger than the element being
               searched for - if no elements in the array are bigger, then the index
               value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends
               on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by
               one of the TArrayFindMode enum values.

@leave  KErrNotFound, if no suitable pointer can be found.

@see TArrayFindMode
*/
inline void RPointerArray<TAny>::SpecificFindInAddressOrderL(const TAny* anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInAddressOrder(anEntry, anIndex, aMode)); }


/**
Inserts an pointer into the array in address order.

No duplicate entries are permitted.  The function assumes that existing pointers
within the array are in address order.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be inserted.
*/
inline void RPointerArray<TAny>::InsertInAddressOrderL(const TAny* anEntry)
	{ User::LeaveIfError(InsertInAddressOrder(anEntry)); }


/**
Inserts an pointer into the array in address order, allowing duplicates.

If the new pointer is a duplicate of an existing pointer in the array, then the
new pointer is inserted after the existing one. If more than one duplicate
pointer already exists in the array, then any new duplicate pointer is inserted
after the last one.

The function assumes that existing pointers within the array are in address
order.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The pointer to be inserted.
*/
inline void RPointerArray<TAny>::InsertInAddressOrderAllowRepeatsL(const TAny* anEntry)
	{ User::LeaveIfError(InsertInAddressOrderAllowRepeats(anEntry)); }


/**
Apends an object onto the array.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry    A reference to the object of type class T to be appended.
*/
template <class T>
inline void RArray<T>::AppendL(const T& anEntry)
	{ User::LeaveIfError(Append(anEntry));}


/**
Inserts an object into the array at a specified position.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The class T object to be inserted.
@param aPos    The position within the array where the object is to
               be inserted. The position is relative to zero, i.e. zero
			   implies that an object is inserted at the beginning of
			   the array.
			   
@panic USER 131, if aPos is negative or is greater than the number of objects
       currently in the array.
*/
template <class T>
inline void RArray<T>::InsertL(const T& anEntry, TInt aPos)
	{ User::LeaveIfError(Insert(anEntry, aPos));}


/**
Finds the first object in the array which matches the specified object using 
a sequential search.

Matching is based on the comparison of a TInt value at the key offset position 
within the objects.

For classes which define their own equality operator (==), the alternative method
FindL(const T& anEntry, TIdentityRelation<T> anIdentity) is recommended.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the first matching object within the array. 
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindL(const T& anEntry) const
	{ return User::LeaveIfError(Find(anEntry));}


/**
Finds the first object in the array which matches the specified object using 
a sequential search and a matching algorithm.

The algorithm for determining whether two class T type objects match is provided 
by a function supplied by the caller.

Such a function need not be supplied if an equality operator (==) is defined for class T. 
In this case, default construction of anIdentity provides matching.

See Find(const T& anEntry, TIdentityRelation<T> anIdentity) for more details.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry    A reference to an object of type class T to be used
                  for matching.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T type objects match.

@return The index of the first matching object within the array.
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindL(const T& anEntry, TIdentityRelation<T> anIdentity) const
	{ return User::LeaveIfError(Find(anEntry, anIdentity));}


/**
Finds the last object in the array which matches the specified object using 
a sequential search.

Matching is based on the comparison of a TInt value at the key offset position 
within the objects.

For classes which define their own equality operator (==), the alternative method
FindReverseL(const T& anEntry, TIdentityRelation<T> anIdentity) is recommended.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the last matching object within the array. 
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindReverseL(const T& anEntry) const
	{ return User::LeaveIfError(FindReverse(anEntry));}


/**
Finds the last object in the array which matches the specified object using 
a sequential search and a matching algorithm.

The algorithm for determining whether two class T type objects match is provided 
by a function supplied by the caller.

Such a function need not be supplied if an equality operator (==) is defined for class T. 
In this case, default construction of anIdentity provides matching.

See Find(const T& anEntry, TIdentityRelation<T> anIdentity) for more details.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry    A reference to an object of type class T to be used
                  for matching.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T type objects match.

@return The index of the last matching object within the array.
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindReverseL(const T& anEntry, TIdentityRelation<T> anIdentity) const
	{ return User::LeaveIfError(FindReverse(anEntry, anIdentity));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in signed 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the matching object within the array.
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindInSignedKeyOrderL(const T& anEntry) const
	{ return User::LeaveIfError(FindInSignedKeyOrder(anEntry));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in unsigned 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the matching object within the array.
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindInUnsignedKeyOrderL(const T& anEntry) const
	{ return User::LeaveIfError(FindInUnsignedKeyOrder(anEntry));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T>.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return The index of the matching object within the array.
@leave  KErrNotFound if no matching object can be found.
*/
template <class T>
inline TInt RArray<T>::FindInOrderL(const T& anEntry, TLinearOrder<T> anOrder) const
{ return User::LeaveIfError(FindInOrder(anEntry, anOrder));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in signed 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains an index value of the matching object within the array.
               If the function leaves with KErrNotFound,this is the index of the
               first element in the array whose key is bigger than the key of the
               element being sought. If there are no elements in the array with
               a bigger key, then the index value is the same as the total 
               number of elements in the array.
@leave KErrNotFound, if no matching object can be found.
*/
template <class T>
inline void RArray<T>::FindInSignedKeyOrderL(const T& anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInSignedKeyOrder(anEntry, anIndex));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in unsigned 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains an index value of the matching object within the array. 
               If the function leaves with KErrNotFound,  this is the index of the
               first element in the array whose key is bigger than the key of the
               element being sought. If there are no elements in the array with
               a bigger key, then the index value is the same as the total 
               number of elements in the array.
@leave  KErrNotFound, if no matching object can be found.
*/
template <class T>
inline void RArray<T>::FindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInUnsignedKeyOrder(anEntry, anIndex));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T>.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains the index value of the matching object within the array
               If the function leaves with KErrNotFound, this is the index of
               the first element in the array that is bigger than the element
               being searched for - if no elements in the array are bigger,
               then the index value is the same as the total number of elements
               in the array.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@leave  KErrNotFound if no matching object can be found.
*/
template <class T>
inline void RArray<T>::FindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const
	{ User::LeaveIfError(FindInOrder(anEntry, anIndex, anOrder));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by a signed 32-bit word
(the key) embedded in each array element. In the case that there is more than
one matching element, finds the first, last or any match as specified.

The function assumes that existing objects within the array are in signed 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param	aMode  Specifies whether to find the first match, the last match or
               any match, as defined by one of the TArrayFindMode enum values.

@return The array index of a matching element - what the index refers to
        depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline TInt RArray<T>::SpecificFindInSignedKeyOrderL(const T& anEntry, TInt aMode) const
{ return User::LeaveIfError(SpecificFindInSignedKeyOrder(anEntry, aMode));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by an unsigned 32-bit word
(the key) embedded in each array element. In the case that there is more than
one matching element, finds the first, last or any match as specified.

The function assumes that existing objects within the array are in unsigned 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param	aMode  Specifies whether to find the first match, the last match or any
        match, as defined by one of the TArrayFindMode enum values.

@return The array index of a matching element -  what the index refers to
        depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element
        of the array, then the index value is the same as the total number of elements in the array.
        
@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline TInt RArray<T>::SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInUnsignedKeyOrder(anEntry, aMode));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

Where there is more than one matching element, it finds the first, the last or
any matching element as specified by the value of aMode.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T> type.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.

@return The array index of a matching element -  what the index refers to
        depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element
        of the array, then the index value is the same as the total number of elements in the array.
        
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline TInt RArray<T>::SpecificFindInOrderL(const T& anEntry, TLinearOrder<T> anOrder, TInt aMode) const
{ return User::LeaveIfError(SpecificFindInOrder(anEntry, anOrder, aMode));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by a signed 32-bit word
(the key) embedded in each array element. In the case that there is more than
one matching element, finds the first, last or any match as specified.

The function assumes that existing objects within the array are in signed 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the
               value of aMode. If there is no matching element in the array,
               then this is the index of the first element in the array that
               is bigger than the element being searched for - if no elements
               in the array are bigger, then the index value is the same as
               the total number of elements in the array.
               If there is a matching element, then what the index refers to
               depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements
               in the array.
@param aMode   Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.
               
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline void RArray<T>::SpecificFindInSignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInSignedKeyOrder(anEntry, anIndex, aMode));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by an unsigned 32-bit word
(the key) embedded in each array element. In the case that there is more than
one matching element, finds the first, last or any match as specified.

The function assumes that existing objects within the array are in unsigned 
key order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the
               value of aMode. If there is no matching element in the array,
               then this is the index of the first element in the array that
               is bigger than the element being searched for - if no elements
               in the array are bigger, then the index value is the same as
               the total number of elements in the array. If there is a matching
               element, then what the index refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
@param aMode   Specifies whether to find the first match, the last match or any match,
               as defined by one of the  TArrayFindMode enum values.
               
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline void RArray<T>::SpecificFindInUnsignedKeyOrderL(const T& anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInUnsignedKeyOrder(anEntry, anIndex, aMode));}


/**
Finds the object in the array which matches the specified object using a binary 
search technique and a specified ordering algorithm.

Where there is more than one matching element, it finds the first, the last or
any matching element as specified by the value of aMode.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T> type.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the value
               of aMode. If there is no matching element in the array, then this is
               the  index of the first element in the array that is bigger than the
               element being searched for - if no elements in the array are bigger,
               then the index value is the same as the total number of elements
               in the array. If there is a matching element, then what the index
               refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
               
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param aMode   Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.
               
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
template <class T>
inline void RArray<T>::SpecificFindInOrderL(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInOrder(anEntry, anIndex, anOrder, aMode));}


/**
Inserts an object into the array in ascending signed key order.

The order of two class T type objects is based on comparing a TInt value
located at the key offset position within the class T object.

No duplicate entries are permitted.

The function leaves with one of the system wide error codes, if the operation fails.

Note that the array remains unchanged following an attempt to insert a duplicate entry.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
*/
template <class T>
inline void RArray<T>::InsertInSignedKeyOrderL(const T& anEntry)
	{ User::LeaveIfError(InsertInSignedKeyOrder(anEntry));}


/**
Inserts an object into the array in ascending unsigned key order, not allowing 
duplicate entries.

The order of two class T type objects is based on comparing a TUint value 
located at the key offset position within the class T object. 

The function leaves with one of the system wide error codes, if the operation fails.

Note that the array remains unchanged following an attempt to insert a duplicate entry.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
*/
template <class T>
inline void RArray<T>::InsertInUnsignedKeyOrderL(const T& anEntry)
	{ User::LeaveIfError(InsertInUnsignedKeyOrder(anEntry));}


/**
Inserts an object of into the array in object order.

The algorithm for determining the order of two class T type objects is provided 
by a function supplied by the caller.

No duplicate entries are permitted.

The function assumes that existing objects within the array are in object 
order.

The function leaves with one of the system wide error codes, if the operation fails.

Note that the array remains unchanged following an attempt to insert a duplicate entry.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
*/
template <class T>
inline void RArray<T>::InsertInOrderL(const T& anEntry, TLinearOrder<T> anOrder)
	{ User::LeaveIfError(InsertInOrder(anEntry, anOrder));}


/**
Inserts an object into the array in ascending signed key order,
allowing duplicates.

The order of two class T type objects is based on comparing a TInt value
located at the key offset position within the class T object. 

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then any new duplicate object is inserted after 
the last one.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
*/
template <class T>
inline void RArray<T>::InsertInSignedKeyOrderAllowRepeatsL(const T& anEntry)
	{ User::LeaveIfError(InsertInSignedKeyOrderAllowRepeats(anEntry));}


/**
Inserts an object into the array in ascending unsigned key order, allowing 
duplicates.

The order of two class T type objects is based on comparing a TUint value 
located at the key offset position within the class T object. 

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then any new duplicate object is inserted after 
the last one.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
*/
template <class T>
inline void RArray<T>::InsertInUnsignedKeyOrderAllowRepeatsL(const T& anEntry)
	{ User::LeaveIfError(InsertInUnsignedKeyOrderAllowRepeats(anEntry));}


/**
Inserts an object into the array in object order, allowing duplicates.

The algorithm for determining the order of two class T type objects is provided 
by a function supplied by the caller.

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then anEntry is inserted after the last one.

The function assumes that existing objects within the array are in object 
order.

The function leaves with one of the system wide error codes, if the operation fails.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry A reference to the object of type class T to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
*/
template <class T>
inline void RArray<T>::InsertInOrderAllowRepeatsL(const T& anEntry, TLinearOrder<T> anOrder)
	{ User::LeaveIfError(InsertInOrderAllowRepeats(anEntry, anOrder));}



/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of objects specified. Adding new objects to the array 
does not result in a re-allocation of memory until the the total number of 
objects exceeds the specified count.

@param	aCount	The number of objects for which space should be reserved
@leave KErrNoMemory	If the requested amount of memory could not be allocated
*/
template <class T>
inline void RArray<T>::ReserveL(TInt aCount)
	{ User::LeaveIfError(RArrayBase::DoReserve(aCount)); }




/**
Appends a signed integer onto the array.

The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The signed integer to be appended.
*/
inline void RArray<TInt>::AppendL(TInt anEntry)
	{ User::LeaveIfError(Append(anEntry));}


/**
Inserts a signed integer into the array at the specified position.
	
The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The signed integer to be inserted.
@param aPos    The position within the array where the signed integer is to be 
	           inserted. The position is relative to zero, i.e. zero implies
			   that an entry is inserted at the beginning of the array.
		   
@panic USER 131, if aPos is negative, or is greater than the number of entries
       currently in the array.
*/
inline void RArray<TInt>::InsertL(TInt anEntry, TInt aPos)
	{ User::LeaveIfError(Insert(anEntry, aPos));}


/**
Finds the first signed integer in the array which matches the specified signed 
integer using a sequential search.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of entries in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The signed integer to be found.

@return The index of the first matching signed integer within the array.
@leave  KErrNotFound, if no matching entry can be found.
*/
inline TInt RArray<TInt>::FindL(TInt anEntry) const
	{ return User::LeaveIfError(Find(anEntry));}


/**
Finds the last signed integer in the array which matches the specified signed 
integer using a sequential search.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of entries in the array.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The signed integer to be found.

@return The index of the last matching signed integer within the array.
@leave  KErrNotFound, if no matching entry can be found.
*/
inline TInt RArray<TInt>::FindReverseL(TInt anEntry) const
	{ return User::LeaveIfError(FindReverse(anEntry));}


/**
Finds the signed integer in the array that matches the specified signed integer 
using a binary search technique.

The function assumes that the array is in signed integer order.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The signed integer to be found.

@return The index of the matching signed integer within the array.
@leave  KErrNotFound, if no match can be found.
*/
inline TInt RArray<TInt>::FindInOrderL(TInt anEntry) const
	{ return User::LeaveIfError(FindInOrder(anEntry));}


/**
Finds the signed integer in the array that matches the specified signed integer
using a binary search technique.
	
The function assumes that the array is in signed integer order.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The signed integer to be found.
@param anIndex A reference to a signed integer into which the
               function puts an index value: If the function returns ,
               this is the index of the matching signed integer within the
               array. If the function leaves with KErrNotFound, this is the
               index of the first signed integer within the array that is
               bigger than the signed integer being searched for - if no
               signed integers within the array are bigger, then the index
               value is the same as the total number of signed integers
               within the array.
@leave  KErrNotFound if no  match can be found.
*/
inline void RArray<TInt>::FindInOrderL(TInt anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInOrder(anEntry, anIndex));}


/**
Finds the signed integer in the array that matches the specified signed integer 
using a binary search technique.

Where there is more than one matching element, it finds the first, last or any
matching element  as specified by the value of aMode.
	
The function assumes that the array is in signed integer order.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The signed integer to be found.
@param aMode   Specifies whether to find the first match, the last match or
               any match, as defined by one of the TArrayFindMode enum values.

@return The array index of a matching element - what the index refers to
        depends on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element
        of the array, then the index value is the same as the total number of elements in the array.
        
@leave  KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
inline TInt RArray<TInt>::SpecificFindInOrderL(TInt anEntry, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInOrder(anEntry, aMode));}


/**
Finds the signed integer in the array that matches the specified signed integer
using a binary search technique.

Where there is more than one matching element, it finds the first, last or any
matching element  as specified by the value of aMode.

The function assumes that the array is in signed integer order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The signed integer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is
               the  index of the first element in the array that is bigger
               than the element being searched for - if no elements in the
               array are bigger, then the index value is the same as the total
               number of elements in the array. If there is a matching element,
               then what the index refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined
               by one of the TArrayFindMode enum values.
               
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
inline void RArray<TInt>::SpecificFindInOrderL(TInt anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInOrder(anEntry, anIndex, aMode));}


/**
Inserts a signed integer into the array in signed integer order.

No duplicate entries are permitted.

The function assumes that existing entries within the array are in signed 
integer order.

The function leaves with one of the system wide error codes, if the operation fails.

Note that the array remains unchanged following an attempt to insert a duplicate entry. 

NOTE: This function is NOT AVAILABLE to code running on the kernel side.

@param anEntry The signed integer to be inserted.
*/
inline void RArray<TInt>::InsertInOrderL(TInt anEntry)
	{ User::LeaveIfError(InsertInOrder(anEntry));}


/**
Inserts a signed integer into the array in signed integer order,
allowing duplicates.

If anEntry is a duplicate of an existing entry in the array, then the new 
signed integer is inserted after the existing one. If more than one duplicate 
entry already exists in the array, then any new duplicate signed integer is 
inserted after the last one.

The function assumes that existing entries within the array are in signed 
integer order.

The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The signed integer to be inserted.
*/
inline void RArray<TInt>::InsertInOrderAllowRepeatsL(TInt anEntry)
	{ User::LeaveIfError(InsertInOrderAllowRepeats(anEntry));}



/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of integers specified. Adding new integers to the array 
does not result in a re-allocation of memory until the the total number of 
integers exceeds the specified count.

@param	aCount	The number of integers for which space should be reserved
@leave KErrNoMemory	If the requested amount of memory could not be allocated
*/
inline void RArray<TInt>::ReserveL(TInt aCount)
	{ User::LeaveIfError(RPointerArrayBase::DoReserve(aCount)); }




/**
Appends an unsigned integer onto the array.
	
The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be appended.
*/
inline void RArray<TUint>::AppendL(TUint anEntry)
	{ User::LeaveIfError(Append(anEntry));}


/**
Inserts an unsigned integer into the array at the specified position.
	
The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry  The unsigned integer to be inserted.
@param aPos     The position within the array where the unsigned integer is to 
	            be inserted. The position is relative to zero, i.e. zero
				implies that an entry is inserted at the beginning of
				the array.
			
@panic USER 131, if aPos is negative, or is greater than the number of entries
       currently in the array.
*/
inline void RArray<TUint>::InsertL(TUint anEntry, TInt aPos)
	{ User::LeaveIfError(Insert(anEntry, aPos));}


/**
Finds the first unsigned integer in the array which matches the specified
value, using a sequential search.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of entries in the array.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be found.
@return The index of the first matching unsigned integer within the array.
@leave  KErrNotFound, if no matching entry can be found.
*/
inline TInt RArray<TUint>::FindL(TUint anEntry) const
	{ return User::LeaveIfError(Find(anEntry));}


/**
Finds the last unsigned integer in the array which matches the specified
value, using a sequential search.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of entries in the array.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be found.
@return The index of the last matching unsigned integer within the array.
@leave  KErrNotFound, if no matching entry can be found.
*/
inline TInt RArray<TUint>::FindReverseL(TUint anEntry) const
	{ return User::LeaveIfError(FindReverse(anEntry));}


/**
Finds the unsigned integer in the array which matches the specified value, 
using a binary search technique.
	
The functions assume that existing entries within the array are in unsigned 
integer order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The unsigned integer to be found.

@return The index of the matching unsigned integer within the array;
@leave  KErrNotFound, if no matching entry can be found.
*/
inline TInt RArray<TUint>::FindInOrderL(TUint anEntry) const
	{ return User::LeaveIfError(FindInOrder(anEntry));}


/**
Finds the unsigned integer in the array which matches the specified value, 
using a binary search technique.

If the index cannot be found, the function returns the index of the last
unsigned integer within the array which logically precedes anEntry.
The functions assume that existing entries within the array are in unsigned 
integer order.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The unsigned integer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an index
               value of the matching unsigned integer within the array. 
               If the function leaves with KErrNotFound, this is the index of the
               first unsigned integer within the array that is bigger than the
               unsigned integer being searched for - if no unsigned integers within
               the array are bigger, then the index value is the same as the
               total number of unsigned integers within the array.

@leave  KErrNotFound, if no matching entry can be found.
*/
inline void RArray<TUint>::FindInOrderL(TUint anEntry, TInt& anIndex) const
	{ User::LeaveIfError(FindInOrder(anEntry, anIndex));}


/**
Finds the unsigned integer in the array that matches the specified unsigned integer 
using a binary search technique.

In the case that there is more than one matching element, finds the first, last or any
match as specified.
	
The function assumes that the array is in unsigned integer order.

NOTE: This function is NOT AVAILABLE to code running on the kernel side.
	
@param anEntry The unsigned integer to be found.
@param aMode   Specifies whether to find the first match, the last match or 
               any match, as defined by one of the TArrayFindMode enum values.

@return The array index of a matching element - what the index refers to depends
        on the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element
        of the array, then the index value is the same as the total number of elements in the array.
        
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
inline TInt RArray<TUint>::SpecificFindInOrderL(TUint anEntry, TInt aMode) const
	{ return User::LeaveIfError(SpecificFindInOrder(anEntry, aMode));}


/**
Finds the unsigned integer in the array that matches the specified unsigned integer
using a binary search technique.

Where there is more than one matching element, it finds the first, last or
any matching element as specified by the value of aMode.

The function assumes that the array is in unsigned integer order.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the
               index of the first element in the array that is bigger than the element
               being searched for - if no elements in the array are bigger, then
               the index value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends on
               the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element of the array,
               then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by
               one of the TArrayFindMode enum values.
@leave KErrNotFound if no matching entry exists.

@see TArrayFindMode
*/
inline void RArray<TUint>::SpecificFindInOrderL(TUint anEntry, TInt& anIndex, TInt aMode) const
	{ User::LeaveIfError(SpecificFindInOrder(anEntry, anIndex, aMode));}


/**
Inserts an unsigned integer into the array in unsigned integer order.

No duplicate entries are permitted.

The function assumes that existing entries within the array are in unsigned 
integer order.

The function leaves with one of the system wide error codes, if the operation fails.
	
Note that the array remains unchanged following an attempt to insert a duplicate entry.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be inserted.
*/
inline void RArray<TUint>::InsertInOrderL(TUint anEntry)
	{ User::LeaveIfError(InsertInOrder(anEntry));}


/**
Inserts an unsigned integer into the array in unsigned integer order, allowing 
duplicates.

If the new integer is a duplicate of an existing entry in the array, then 
the new unsigned integer is inserted after the existing one. If more than 
one duplicate entry already exists in the array, then any new duplicate
unsigned integer is inserted after the last one.
	
The function assumes that existing entries within the array are in unsigned 
integer order.

The function leaves with one of the system wide error codes, if the operation fails.
	
NOTE: This function is NOT AVAILABLE to code running on the kernel side.	
	
@param anEntry The unsigned integer to be inserted.
*/
inline void RArray<TUint>::InsertInOrderAllowRepeatsL(TUint anEntry)
	{ User::LeaveIfError(InsertInOrderAllowRepeats(anEntry));}



/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of integers specified. Adding new integers to the array 
does not result in a re-allocation of memory until the the total number of 
integers exceeds the specified count.

@param	aCount	The number of integers for which space should be reserved
@leave KErrNoMemory	If the requested amount of memory could not be allocated
*/
inline void RArray<TUint>::ReserveL(TInt aCount)
	{ User::LeaveIfError(RPointerArrayBase::DoReserve(aCount)); }



// class TChunkHeapCreateInfo
/**
Sets single thread property of the chunk heap.

This overrides any previous call to TChunkHeapCreateInfo::SetSingleThread()
for this TChunkHeapCreateInfo object.

@param aSingleThread	ETrue when the chunk heap is to be single threaded,
						EFalse otherwise.
*/
inline void TChunkHeapCreateInfo::SetSingleThread(const TBool aSingleThread)
	{
	iSingleThread = aSingleThread;
	}


/**
Sets alignment of the cells of the chunk heap to be created.

This overrides any previous call to TChunkHeapCreateInfo::SetAlignment()
for this TChunkHeapCreateInfo object.

@param aAlignment	The alignment of the heap cells.
*/
inline void TChunkHeapCreateInfo::SetAlignment(TInt aAlign)
	{
	iAlign = aAlign;
	}


/**
Sets the increments to the size of the host chunk.  If the supplied value is 
less than KMinHeapGrowBy, it is discarded and the value KMinHeapGrowBy is 
used instead.

This overrides any previous call to TChunkHeapCreateInfo::SetGrowBy()
for this TChunkHeapCreateInfo object.

@param aGrowBy	The increment to the size of the host chunk.
*/
inline void TChunkHeapCreateInfo::SetGrowBy(TInt aGrowBy)
	{
	iGrowBy = aGrowBy;
	}


/**
Sets the offset from the base of the host chunk to the start of the heap.

This overrides any previous call to TChunkHeapCreateInfo::SetOffset()
for this TChunkHeapCreateInfo object.

@param aOffset	The offset in bytes.
*/
inline void TChunkHeapCreateInfo::SetOffset(TInt aOffset)
	{
	iOffset = aOffset;
	}


/**
Sets the mode flags of the chunk heap.

This overrides any previous call to TChunkHeapCreateInfo::SetMode()
for this TChunkHeapCreateInfo object.

@param aMode	The mode flags for the chunk heap to be created, this should be
				one or more of the values from TChunkHeapCreateMode.
*/
inline void TChunkHeapCreateInfo::SetMode(TUint aMode)
	{
	iMode = aMode;
	}


/**
Sets the paging attribute of the chunk heap to be created.

This overrides any previous call to TChunkHeapCreateInfo::SetPaging()
for this TChunkHeapCreateInfo object.

@param aPaging	The paging attribute for the chunk heap to be created.
*/
inline void TChunkHeapCreateInfo::SetPaging(const TChunkHeapPagingAtt aPaging)
	{
	iPaging = aPaging;
	}


/**
Sets the priority of the client's process.

@param aPriority The priority value.
*/
inline void RMessagePtr2::SetProcessPriorityL(TProcessPriority aPriority) const
	{ User::LeaveIfError(SetProcessPriority(aPriority));}


/**
Opens a handle on the client thread.

@param aClient    On successful return, the handle to the client thread.
@param aOwnerType An enumeration whose enumerators define the ownership of
                  the handle. If not explicitly specified,
                  EOwnerProcess is taken as default.
*/
inline void RMessagePtr2::ClientL(RThread& aClient, TOwnerType aOwnerType) const
	{ User::LeaveIfError(Client(aClient, aOwnerType));}


#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TBool RMessagePtr2::HasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability, aDiagnostic);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability, const char* aDiagnosticMessage) const
	{
	if (!HasCapability(aCapability, aDiagnosticMessage))
		{
		User::Leave(KErrPermissionDenied);
		}
	}

inline TBool RMessagePtr2::HasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	return DoHasCapability(aCapability1, aCapability2, aDiagnostic);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability1, TCapability aCapability2, const char* aDiagnosticMessage) const
	{
	if (!HasCapability(aCapability1, aCapability2, aDiagnosticMessage))
		{
		User::Leave(KErrPermissionDenied);
		}
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

// Only available to NULL arguments
inline TBool RMessagePtr2::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability, OnlyCreateWithNull /*aDiagnosticMessage*/) const
	{
	if (!DoHasCapability(aCapability))
		{
		User::Leave(KErrPermissionDenied);
		}
	}

inline TBool RMessagePtr2::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnosticMessage*/) const
	{
	if (!DoHasCapability(aCapability1, aCapability2))
		{
		User::Leave(KErrPermissionDenied);
		}
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
// For things using KSuppressPlatSecDiagnostic
inline TBool RMessagePtr2::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability, KSuppressPlatSecDiagnosticMagicValue);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	if (!DoHasCapability(aCapability, KSuppressPlatSecDiagnosticMagicValue))
		{
		User::Leave(KErrPermissionDenied);
		}
	}

inline TBool RMessagePtr2::HasCapability(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	return DoHasCapability(aCapability1, aCapability2, KSuppressPlatSecDiagnosticMagicValue);
	}

inline void RMessagePtr2::HasCapabilityL(TCapability aCapability1, TCapability aCapability2, OnlyCreateWithNull /*aDiagnostic*/, OnlyCreateWithNull /*aSuppress*/) const
	{
	if (!DoHasCapability(aCapability1, aCapability2, KSuppressPlatSecDiagnosticMagicValue))
		{
		User::Leave(KErrPermissionDenied);
		}
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TInt RThread::RenameMe(const TDesC& aName)
	{ return User::RenameThread(aName); }
inline TInt RProcess::RenameMe(const TDesC& aName)
	{ return User::RenameProcess(aName); }


#endif // !__KERNEL_MODE__

#ifdef __SUPPORT_CPP_EXCEPTIONS__
// The standard header file <exception> defines the following guard macro for EDG and CW, VC++, GCC respectively.
// The guard below is ugly. It will surely come back and bite us unless we resolve the whole issue of standard headers
// when we move to supporting Standard C++.

// The macro __SYMBIAN_STDCPP_SUPPORT__ is defined when building a StdC++ target.
// In this case, we wish to avoid defining uncaught_exception below since it clashes with the StdC++ specification 
#if !defined(_EXCEPTION) && !defined(_EXCEPTION_) && !defined(__EXCEPTION__) && !defined(__SYMBIAN_STDCPP_SUPPORT__)

#if defined(__VC32__) && !defined(_CRTIMP_PURE)

	// Declare MS EH runtime functions
	bool __uncaught_exception(void);

#if _MSC_VER >= 1200
	__declspec(noreturn) void terminate(void);
	__declspec(noreturn) void unexpected(void);
#else
	void terminate(void);
	void unexpected(void);
#endif

	typedef void (*terminate_handler)();
	terminate_handler set_terminate(terminate_handler h) throw();
	typedef void (*unexpected_handler)();
	unexpected_handler set_unexpected(unexpected_handler h) throw();

namespace std {
#ifdef __MSVCDOTNET__
	inline bool uncaught_exception(void) { return ::__uncaught_exception(); }
#else // !__MSVCDOTNET__
	// MS KB242192: BUG: uncaught_exception() Always Returns False
	inline bool uncaught_exception(void) { return false; }
#endif //__MSVCDOTNET__
	inline void terminate(void) { ::terminate(); }
	inline void unexpected(void) { ::unexpected(); }
	inline terminate_handler set_terminate(terminate_handler h) throw() { return ::set_terminate(h); }
	inline unexpected_handler set_unexpected(unexpected_handler h) throw() { return ::set_unexpected(h); }
}

#endif // extract from MSVC headers

#ifdef __CW32__

	extern "C" bool __uncaught_exception(void);

namespace std {
#if __MWERKS__ > 0x3200
	inline bool uncaught_exception(void) { return ::__uncaught_exception(); }
#else
	// no uncaught_exception() implementation on CW 2.4.7
	inline bool uncaught_exception(void) { return false; }
#endif
}

#endif // extract from CW headers

#endif // <exception> header guard

#endif //__SUPPORT_CPP_EXCEPTIONS__
