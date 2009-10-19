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
// e32\include\e32base.inl
// 
//

// Class CBase
inline TAny* CBase::operator new(TUint aSize, TAny* aBase) __NO_THROW
/**
Initialises the object to binary zeroes.

@param aSize The size of the derived class. This parameter is specified
             implicitly by C++ in all circumstances in which a derived
			 class is allocated.
@param aBase Indicates a base address which is returned as the object's
             address. 

@return A pointer to the base address.
*/
	{ Mem::FillZ(aBase, aSize); return aBase; }




inline TAny* CBase::operator new(TUint aSize) __NO_THROW
/**
Allocates the object from the heap and then initialises its contents
to binary zeroes.

@param aSize The size of the derived class. This parameter is specified
             implicitly by C++ in all circumstances in which a derived class
			 is allocated.

@return      A pointer to the allocated object; NULL if memory could not
             be allocated.
*/
	{ return User::AllocZ(aSize); }




inline TAny* CBase::operator new(TUint aSize, TLeave)
/**
Allocates the object from the heap and then initialises its contents
to binary zeroes.

@param aSize  The size of the derived class. This parameter is specified
              implicitly by C++ in all circumstances in which a derived class
			  is allocated.

@return       A pointer to the allocated object; the TLeave parameter indicates
              that the operation leaves if allocation fails with out-of-memory.
*/
	{ return User::AllocZL(aSize); }




inline TAny* CBase::operator new(TUint aSize, TUint aExtraSize) __NO_THROW
/**
Allocates the object from the heap and then initialises its contents
to binary zeroes.

Use of this overload is rare.

@param aSize  The size of the derived class. This parameter is specified
              implicitly by C++ in all circumstances in which a derived class
			  is allocated.

@param aExtraSize Indicates additional size beyond the end of the base class.

@return      A pointer to the allocated object; NULL if memory could not
             be allocated.
*/
	{ return User::AllocZ(aSize + aExtraSize); }




inline TAny* CBase::operator new(TUint aSize, TLeave, TUint aExtraSize)
/**
Allocates the object from the heap and then initialises its contents
to binary zeroes.

Use of this overload is rare.

@param aSize  The size of the derived class. This parameter is specified
              implicitly by C++ in all circumstances in which a derived class
			  is allocated.

@param aExtraSize Indicates additional size beyond the end of the base class.

@return      A pointer to the allocated object; the TLeave parameter indicates
              that the operation leaves if allocation fails with out-of-memory.
*/
	{ return User::AllocZL(aSize + aExtraSize); }




// Class CBufBase
inline TInt CBufBase::Size() const
/**
Gets the number of data bytes in the buffer.

Note that the number of heap bytes used by the buffer may be greater than its
size, because there is typically extra room to allow for expansion. Use the
Compress() function to reduce the extra allocation as much as possible.

@return The number of data bytes in the buffer.
*/
	{return(iSize);}




// Class CBufFlat
inline TInt CBufFlat::Capacity() const
/**
Gets the size to which the buffer may expand without re-allocation.

@return The size of the allocated cell associated with the buffer. This is 
        the maximum size the buffer may be expanded to without re-allocation.
*/
	{return(iMaxSize);}




// Class CArrayFixBase
inline TInt CArrayFixBase::Count() const
/**
Gets the number of elements held in the array.

@return The number of array elements
*/
	{return(iCount);}




inline TInt CArrayFixBase::Length() const
/**
Gets the length of an element.

@return The length of an element of type class T.
*/
	{return(iLength);}




// Template class CArrayFix
template <class T>
inline CArrayFix<T>::CArrayFix(TBufRep aRep,TInt aGranularity)
	: CArrayFixBase(aRep,sizeof(T),aGranularity)
/**
@internalComponent
*/
	{}




template <class T>
inline const T &CArrayFix<T>::operator[](TInt anIndex) const
/**
Gets a const reference to the element located at the specified position 
within the array.

Note that if a pointer to the returned referenced class T object is taken,
be aware that the pointer value becomes invalid once elements have been added
to, or removed from the array. Always refresh the pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.
			   
@return A const reference to the required element.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(*((const T *)CArrayFixBase::At(anIndex)));}




template <class T>
inline T &CArrayFix<T>::operator[](TInt anIndex)
/**
Gets a non-const reference to the element located at the specified position 
within the array.

Note that if a pointer to the returned referenced class T object is taken,
be aware that the pointer value becomes invalid once elements have been added
to, or removed from the array. Always refresh the pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.
			   
@return A non-const reference to the required element.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(*((T *)CArrayFixBase::At(anIndex)));}




template <class T>
inline const T &CArrayFix<T>::At(TInt anIndex) const
/** 
Gets a const reference to the element located at the specified position 
within the array.

Note that if a pointer to the returned referenced class T object is taken,
be aware that the pointer value becomes invalid once elements have been added
to, or removed from the array. Always refresh the pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.
			   
@return A const reference to the required element.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(*((const T *)CArrayFixBase::At(anIndex)));}




template <class T>
inline const T *CArrayFix<T>::End(TInt anIndex) const
/**
Gets a pointer to the (const) first byte following the end of the 
contiguous region containing the element at the specified position within
the array.

For arrays implemented using flat buffers, the pointer always points to the 
first byte following the end of the buffer.

For arrays implemented using segmented buffers, the pointer always points 
to the first byte following the end of the segment which contains the element.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element
			   in the array.
			   
@return A pointer to the constant byte following the end of the contiguous 
        region. 

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return((const T *)CArrayFixBase::End(anIndex));}




template <class T>
inline const T *CArrayFix<T>::Back(TInt anIndex) const
/**
Gets a pointer to the (const) beginning of a contiguous region.

For arrays implemented using flat buffers, the function always returns a
pointer to the beginning of the buffer.

For arrays implemented using segmented buffers, the function returns a pointer 
to the beginning of the segment for all elements in that segment except the 
first. If the element at position anIndex is the first in a segment, then 
the function returns a pointer the beginning of the previous segment.

For the first element in the array, the function returns a NULL pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element
			   in the array.
			   
@return A pointer to the (const) beginning of the contiguous region.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return((const T *)CArrayFixBase::Back(anIndex));}




template <class T>
inline T &CArrayFix<T>::At(TInt anIndex)
/**
Gets a non-const reference to the element located at the specified position 
within the array.

Note that if a pointer to the returned referenced class T object is taken,
be aware that the pointer value becomes invalid once elements have been added
to, or removed from the array. Always refresh the pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.
			   
@return A non-const reference to the required element.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(*((T *)CArrayFixBase::At(anIndex)));}




template <class T>
inline T *CArrayFix<T>::End(TInt anIndex)
/**
Gets a pointer to the first byte following the end of the contiguous region 
containing the element at the specified position within the array.

For arrays implemented using flat buffers, the pointer always points to the 
first byte following the end of the buffer.

For arrays implemented using segmented buffers, the pointer always points 
to the first byte following the end of the segment which contains the element.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element
			   in the array.
			   
@return A pointer to the byte following the end of the contiguous region.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(((T *)CArrayFixBase::End(anIndex)));}




template <class T>
inline T *CArrayFix<T>::Back(TInt anIndex)
/**
Gets a pointer to the beginning of a contiguous region.

For arrays implemented using flat buffers, the function always returns a pointer 
to the beginning of the buffer.

For arrays implemented using segmented buffers, the function returns a pointer 
to the beginning of the segment for all elements in that segment except the 
first. If the element at position anIndex is the first in a segment, then 
the function returns a pointer the beginning of the previous segment.

For the first element in the array, the function returns a NULL pointer.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element
			   in the array.
			   
@return A pointer to the beginning of the contiguous region.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
                         number of objects currently within the array.
*/
	{return(((T *)CArrayFixBase::Back(anIndex)));}




template <class T>
inline void CArrayFix<T>::AppendL(const T &aRef)
/**
Appends a single element onto the end of the array.

@param aRef A reference to the class T element to be appended.

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.
*/
	{CArrayFixBase::InsertL(Count(),&aRef);}




template <class T>
inline void CArrayFix<T>::AppendL(const T *aPtr,TInt aCount)
/**
Appends one or more elements onto the end of the array.

@param aPtr   A pointer to a contiguous set of type <class T> objects to be
              appended.
@param aCount The number of contiguous objects of type <class T> located at 
              aPtr to be appended.
			  
@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@panic E32USER-CBase 23, if aCount is negative.
*/
	{CArrayFixBase::InsertL(Count(),aPtr,aCount);}




template <class T>
inline void CArrayFix<T>::AppendL(const T &aRef,TInt aReplicas)
/**
Appends replicated copies of an element onto the end of the array.

@param aRef      A reference to the <class T> object to be replicated and appended.
@param aReplicas The number of copies of the aRef element to be appended. 

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@panic E32USER-CBase 28 if aReplicas is negative.
*/
	{CArrayFixBase::InsertRepL(Count(),&aRef,aReplicas);}




template <class T>
inline T &CArrayFix<T>::ExpandL(TInt anIndex)
/**
Expands the array by one element at the specified position.

It:

1. expands the array by one element at the specified position

2. constructs a new element at that position

3. returns a reference to the new element.

All existing elements from position anIndex to the end of the array are moved 
up, so that the element originally at position anIndex is now at position 
anIndex + 1 etc.

The new element of type class T is constructed at position anIndex, using 
the default constructor of that class.

@param anIndex The position within the array where the array is to be expanded 
               and the new class T object is to be constructed. 

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@return A reference to the newly constructed class T object at position 
        anIndex within the array.

@panic E32USER-CBase 21 if anIndex is negative or greater than the number
                        of elements currently in the array.
*/
	{return(*new(CArrayFixBase::ExpandL(anIndex)) T);}




template <class T>
inline T &CArrayFix<T>::ExtendL()
/**
Expands the array by one element at the end of the array.

It:

1. expands the array by one element at the end of the array, i.e. at position 
   CArrayFixBase::Count()

2. constructs a new element at that position

3. returns a reference to the new element.

The new element of type class T is constructed at the end of the array, 
using the default constructor of that class.

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@return A reference to the newly constructed class T object at the end of 
        the array.

@see CArrayFixBase::Count
*/
	{return(*new(CArrayFixBase::ExpandL(Count())) T);}




template <class T>
inline TInt CArrayFix<T>::Find(const T &aRef,TKeyArrayFix &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of
keys, using a sequential search.

The array is searched sequentially for an element whose key matches the key of the
supplied class T object. The search starts with the first element in the array.

Note that where an array has elements with duplicate keys, the function only
supplies the position of the first element in the array with that key.

@param aRef    A reference to an object of type class T whose key is used
               for comparison.
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A reference to a TInt supplied by the caller. On return, if the
               element is found, the reference is set to the position of that
			   element within the array. The position is relative to zero, 
			   (i.e. the first element in the array is at position 0).
			   If the element is not found and the array is not empty, then
			   the value of the reference is set to the number of elements in
			   the array. 
			   If the element is not found and the array is empty, then the
			   reference is set to zero.

@return Zero, if the element with the specified key is found. 
        Non-zero, if the element with the specified key is not found.
*/
	{return(CArrayFixBase::Find(&aRef,aKey,anIndex));}




template <class T>
inline TInt CArrayFix<T>::FindIsq(const T &aRef,TKeyArrayFix &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of 
keys, using a binary search technique.

The array is searched, using a binary search technique, for an element whose 
key matches the key of the supplied class T object.

The array must be in key order.

Note that where an array has elements with duplicate keys, the function cannot
guarantee which element, with the given key value, it will return, except that
it will find one of them.

@param aRef    A reference to an object of type class T whose key is used 
               for comparison. 
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A reference to a TInt supplied by the caller. On return, if the
               element is found, the reference is set to the position of that
			   element within the array. The position is relative to zero,
			   (i.e. the first element in the array is at position 0).
			   If the element is not found and the array is not empty, then the
			   reference is set to the position of the first element in the
			   array with a key which is greater than the key of the 
               object aRef.
			   If the element is not found and the array is empty, then the 
               reference is set to zero.

@return Zero, if the element with the specified key is found.
        Non-zero, if the element with the specified key is not found.
*/
	{return(CArrayFixBase::FindIsq(&aRef,aKey,anIndex));}




template <class T>
inline void CArrayFix<T>::InsertL(TInt anIndex,const T &aRef)
/**
Inserts an element into the array at the specified position.

Note that passing a value of anIndex which is the same as the current number
of elements in the array, has the effect of appending the element.

@param anIndex The position within the array where the element is to be
               inserted. The position is relative to zero, i.e. zero implies
			   that elements are inserted at the beginning of the array. 
			   
@param aRef    A reference to the class T object to be inserted into the array 
               at position anIndex.

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@panic E32USER-CBase 21 if anIndex is negative, or is greater than the number
       of elements currently in the array.
*/
	{CArrayFixBase::InsertL(anIndex,&aRef);}




template <class T>
inline void CArrayFix<T>::InsertL(TInt anIndex,const T *aPtr,TInt aCount)
/**
Inserts one or more elements into the array at the specified position.

The objects to be added must all be contiguous.

Note that passing a value of anIndex which is the same as the current number
of elements in the array, has the effect of appending the element.

@param anIndex The position within the array where the elements are to be 
               inserted. The position is relative to zero, i.e. zero implies
			   that elements are inserted at the beginning of the array.
			   
@param aPtr    A pointer to the first of the contiguous elements of type 
               class T to be inserted into the array at position anIndex.

@param aCount  The number of contiguous elements of type class T located at 
               aPtr to be inserted into the array. 

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.

@panic E32USER-CBase 21  if anIndex is negative or is greater than the number
                         of elements currently in the array.
@panic E32USER-CBase 23  if aCount is negative.
*/
	{CArrayFixBase::InsertL(anIndex,aPtr,aCount);}




template <class T>
inline void CArrayFix<T>::InsertL(TInt anIndex,const T &aRef,TInt aReplicas)
/**
Inserts replicated copies of an element into the array at the specified
position.

Note that passing a value of anIndex which is the same as the current number
of elements in the array, has the effect of appending the element.


@param anIndex   The position within the array where elements are to be
                 inserted. The position is relative to zero, i.e. zero implies
				 that elements are inserted at the beginning of the array.
				 
@param aRef      A reference to the class T object to be replicated and
                 inserted into the array at position anIndex. 

@param aReplicas The number of copies of the aRef element to be inserted into 
                 the array.
				 
@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.
	   
@panic E32USER-CBase 21, if anIndex is negative or is greater than the number
                         of elements currently in the array.
@panic E32USER-CBase 28, if aReplicas is negative.
*/
	{CArrayFixBase::InsertRepL(anIndex,&aRef,aReplicas);}




template <class T>
inline TInt CArrayFix<T>::InsertIsqL(const T &aRef,TKeyArrayFix &aKey)
/**
Inserts a single element into the array at a position determined by a key.

The array MUST already be in key sequence (as defined by the key), otherwise 
the position of the new element is unpredictable, or duplicates may occur.

Elements with duplicate keys are not permitted.

@param aRef A reference to the element of type <class T> to be inserted into 
            the array.
@param aKey A reference to a key object defining the properties of the key. 

@return The position within the array of the newly inserted element. 

@leave KErrAlreadyExists An element with the same key already exists within 
       the array. NB the array MUST already be in key sequence, otherwise
	   the function may insert a duplicate and fail to leave with
	   this value.
@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.
*/
	{return(CArrayFixBase::InsertIsqL(&aRef,aKey));}




template <class T>
inline TInt CArrayFix<T>::InsertIsqAllowDuplicatesL(const T &aRef,TKeyArrayFix &aKey)
/**
Inserts a single element into the array at a position determined by a key, 
allowing duplicates.

The array MUST already be in key sequence (as defined by the key), otherwise 
the position of the new element is unpredictable.

If the new element's key is a duplicate of an existing element's key, then 
the new element is positioned after the existing element.

@param aRef A reference to the element of type <class T> to be inserted into 
            the array.
@param aKey A reference to a key object defining the properties of the key. 

@return The position within the array of the newly inserted element.

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call.
*/
	{return(CArrayFixBase::InsertIsqAllowDuplicatesL(&aRef,aKey));}




template <class T>
inline void CArrayFix<T>::ResizeL(TInt aCount)
/**
Changes the size of the array so that it contains the specified number
of elements.

The following describes the effects of calling this function:

1. If aCount is less than the current number of elements in the array, then the 
   array is shrunk. The elements at positions aCount and above are discarded. 
   The array buffer is not compressed.

2. If aCount is greater than the current number of elements in the array, then 
   the array is extended.

3. New elements are replicated copies of an object of type <class T>,
   constructed using the default constructor of that class.

The new elements are positioned after the existing elements in the array.

The function may attempt to expand the array buffer. If there is insufficient 
memory available, the function leaves. The leave code is one of the system 
wide error codes. If the function leaves, the array is left in the state it 
was in before the call.

@param aCount The number of elements the array is to contain after the resizing 
              operation. 
			  
@panic E32USER-CBase 24, if aCount is negative.
*/
	{TUint8 b[sizeof(T)]; new(&b[0]) T; CArrayFixBase::ResizeL(aCount,&b[0]);}




template <class T>
inline void CArrayFix<T>::ResizeL(TInt aCount,const T &aRef)
/**
Changes the size of the array so that it contains the specified number
of elements.

The following describes the effects of calling this function:

1. If aCount is less than the current number of elements in the array, then the 
   array is shrunk. The elements at positions aCount and above are discarded. 
   The array buffer is not compressed.

2. If aCount is greater than the current number of elements in the array, then 
   the array is extended.

3. New elements are replicated copies of aRef.

The new elements are positioned after the existing elements in the array.

The function may attempt to expand the array buffer. If there is insufficient 
memory available, the function leaves. The leave code is one of the system 
wide error codes. If the function leaves, the array is left in the state it 
was in before the call.

@param aCount The number of elements the array is to contain after the resizing 
              operation.

@param aRef   A reference to an object of type <class T>, copies of which are
              used as the new elements of the array, if the array is extended.

@panic E32USER-CBase 24, if aCount is negative.
*/
	{CArrayFixBase::ResizeL(aCount,&aRef);}




template <class T>
inline const TArray<T> CArrayFix<T>::Array() const
/**
Constructs and returns a TArray<T> object.

@return A TArray<T> object representing this array.
*/
	{return(TArray<T>(CountR,AtR,this));}




inline CArrayFix<TAny>::CArrayFix(TBufRep aRep,TInt aRecordLength,TInt aGranularity)
	: CArrayFixBase(aRep,aRecordLength,aGranularity)
/**
@internalComponent
*/
	{}




inline const TAny *CArrayFix<TAny>::At(TInt anIndex) const
/**
Gets a pointer to the untyped element located at the specified position 
within the array.
	
@param anIndex The position of the element within the array. The position 
	           is relative to zero; i.e. zero implies the first element in the
			   array. 
			   
@return A pointer to the untyped element located at position anIndex within 
	    the array. 

@panic E32User-CBase 21, if anIndex is negative or is greater than or equal
       to the number of objects currently within the array.
*/
	{return(CArrayFixBase::At(anIndex));}




inline const TAny *CArrayFix<TAny>::End(TInt anIndex) const
/**
Gets a pointer to the first byte following the end of the contiguous region 
containing the element at the specfied position within the array.
	
For flat buffers, the pointer always points to the first byte following the 
end of the buffer.
	
For segmented buffers, the pointer always points to the first byte following 
the end of the segment which contains the element.
	
@param anIndex The position of the element within the array. The position 
	           is relative to zero; i.e. zero implies the first element in
			   the array.

@return A pointer to the byte following the end of the contiguous region.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayFixBase::End(anIndex));}




inline const TAny *CArrayFix<TAny>::Back(TInt anIndex) const
/**
Gets a pointer to the beginning of a contiguous region.
	
For flat buffers, the function always returns a pointer to the beginning of 
the buffer.
	
For segmented buffers, the function returns a pointer to the beginning of 
the segment for all elements in that segment except the first. If the element 
at the specified position is the first in a segment, then the function returns 
a pointer the beginning of the previous segment.
	
For the first element in the array, the function returns a NULL pointer.
	
@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in the array.
	
@return A pointer to the beginning of the contiguous region.

@panic E32User-CBase 21, if anIndex is negative or is greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayFixBase::Back(anIndex));}




inline TAny *CArrayFix<TAny>::At(TInt anIndex)
/**
Gets a pointer to the untyped element located at the specified position 
within the array.
	
@param anIndex The position of the element within the array. The position 
	           is relative to zero; i.e. zero implies the first element in the
			   array. 
			   
@return A pointer to the untyped element located at position anIndex within 
	    the array. 

@panic E32User-CBase 21, if anIndex is negative or is greater than or equal
       to the number of objects currently within the array.
*/
	{return(CArrayFixBase::At(anIndex));}




inline TAny *CArrayFix<TAny>::End(TInt anIndex)
/**
Gets a pointer to the first byte following the end of the contiguous region 
containing the element at the specfied position within the array.
	
For flat buffers, the pointer always points to the first byte following the 
end of the buffer.
	
For segmented buffers, the pointer always points to the first byte following 
the end of the segment which contains the element.
	
@param anIndex The position of the element within the array. The position 
	           is relative to zero; i.e. zero implies the first element in
			   the array.

@return A pointer to the byte following the end of the contiguous region.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayFixBase::End(anIndex));}




inline TAny *CArrayFix<TAny>::Back(TInt anIndex)
/**
Gets a pointer to the beginning of a contiguous region.
	
For flat buffers, the function always returns a pointer to the beginning of 
the buffer.
	
For segmented buffers, the function returns a pointer to the beginning of 
the segment for all elements in that segment except the first. If the element 
at the specified position is the first in a segment, then the function returns 
a pointer the beginning of the previous segment.
	
For the first element in the array, the function returns a NULL pointer.
	
@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in the array.
	
@return A pointer to the beginning of the contiguous region.

@panic E32User-CBase 21, if anIndex is negative or is greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayFixBase::Back(anIndex));}




inline void CArrayFix<TAny>::AppendL(const TAny *aPtr)
/**
Appends the specified untyped element onto the end of the array.
	
@param aPtr A pointer to an untyped element to be appended. 

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
	   there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call. 
*/
	{CArrayFixBase::InsertL(Count(),aPtr);}




inline void CArrayFix<TAny>::AppendL(const TAny *aPtr,TInt aCount)
/**
Appends one or more untyped elements onto the end of the array.
	
@param aPtr   A pointer to the first of the contiguous untyped elements to be 
	          appended. 
@param aCount The number of contiguous elements located at aPtr to be appended. 

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
       there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call. 
@panic E32USER-CBase 23, if aCount is negative.
*/
	{CArrayFixBase::InsertL(Count(),aPtr,aCount);}




inline TAny *CArrayFix<TAny>::ExtendL()
/**
Expands the array by the length of one element at the end of the array and 
returns a pointer to this new location.
	
As elements are untyped, no construction is possible and the content of the 
new location remains undefined.

@return A pointer to the new element location at the end of the array.

@leave KErrNoMemory The function may attempt to expand the array buffer. If 
	   there is insufficient memory available, the function leaves, in which
	   case the array is left in the state it was in before the call. 
*/
	{return(CArrayFixBase::ExpandL(Count()));}




// Template class CArrayFixFlat
template <class T>
inline CArrayFixFlat<T>::CArrayFixFlat(TInt aGranularity)
	: CArrayFix<T>((TBufRep)CBufFlat::NewL,aGranularity)
/**
Constructs a flat array of fixed length objects with the specified granularity.

The length of all array elements is the length of the class passed as the 
template parameter. The length must be non-zero.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array. 
  
@panic E32USER-CBase 17, if the length of the class implied by the template parameter is zero.

@panic E32USER-CBase 18, if aGranularity is not positive.
*/
	{}




template <class T>
inline void CArrayFixFlat<T>::SetReserveL(TInt aCount)
/**
Reserves space in the array buffer.

If necessary, the array buffer is allocated or re-allocated so that it can
accommodate the specified number of elements.

After a successful call to this function, elements can be added to the array 
and the process is guaranteed not to fail for lack of memory - provided the 
total number of elements does not exceed the number specified in this function.

This function does not increase the number of elements in the array; i.e. 
the member function CArrayFixBase::Count() returns the same value both before 
and after a call to CArrayFixFlat::SetReserveL().

@param aCount The total number of elements for which space is to be reserved. 
	
@panic E32USER-CBase 27, if aCount is less than the current number of elements
                         in the array.
*/
	{this->SetReserveFlatL(aCount);}




inline CArrayFixFlat<TAny>::CArrayFixFlat(TInt aRecordLength,TInt aGranularity)
	: CArrayFix<TAny>((TBufRep)CBufFlat::NewL,aRecordLength,aGranularity)
/**
Constructs a flat array of fixed length objects with the specified granularity 
to contain elements of the specified length.
	
Note that no memory is allocated to the array buffer by this constructor.
	
@param aRecordLength The length of the elements of this fixed length array. 
@param aGranularity  The granularity of the array.
	
@panic E32USER-CBase 17, if aRecordLength is not positive.
@panic E32USER-CBase 18, if aGranularity is  not positive.
*/
	{}




inline void CArrayFixFlat<TAny>::SetReserveL(TInt aCount)
/**
Reserves space in the array buffer.

If necessary, the array buffer is allocated or re-allocated so that it can
accommodate the specified number of elements.
	
After a successful call to this function, elements can be added to the array 
and the process is guaranteed not to fail for lack of memory - provided the 
total number of elements does not exceed the specified number.
	
This function does not increase the number of elements in the array; i.e. 
the member function CArrayFixBase::Count() returns the same value both before 
and after a call to this function.
	
@param aCount The total number of elements for which space is to be reserved. 

@panic E32USER-CBase 27, if aCount is less than the current number of elements
                         in the array.
*/
	{SetReserveFlatL(aCount);}




inline void CArrayFixFlat<TInt>::SetReserveL(TInt aCount)
/**
Reserves space in the array buffer.
	
If necessary, the array buffer is allocated or re-allocated so that it can 
accommodate the specified number of TInt elements.
	
After a successful call to this function, elements can be added to the array 
and the process is guaranteed not to fail for lack of memory - provided the 
total number of elements does not exceed the specified number.
	
This function does not increase the number of elements in the array; i.e. 
the member function CArrayFixBase::Count() returns the same value both before 
and after a call to this function.
	
@param aCount The total number of elements for which space is to be reserved. 

@panic E32USER-CBase 27, if aCount is less than the current number of elements
                         in the array.
*/
	{SetReserveFlatL(aCount);}




inline void CArrayFixFlat<TUid>::SetReserveL(TInt aCount)
/**
Reserves space in the array buffer.
	
If necessary, the array buffer is allocated or re-allocated so that it can 
accommodate the specified number of TUid elements.
	
After a successful call to this function, elements can be added to the array 
and the process is guaranteed not to fail for lack of memory - provided the 
total number of elements does not exceed the specified number.
	
This function does not increase the number of elements in the array; i.e. 
the member function CArrayFixBase::Count() returns the same value both before 
and after a call to this function.
	
@param aCount The total number of elements for which space is to be reserved. 

@panic E32USER-CBase 27, if aCount is less than the current number of elements
                         in the array.
*/
	{SetReserveFlatL(aCount);}




// Template class CArrayFixSeg
template <class T>
inline CArrayFixSeg<T>::CArrayFixSeg(TInt aGranularity)
	: CArrayFix<T>((TBufRep)CBufSeg::NewL,aGranularity)
/**
Constructs a segmented array of fixed length objects with the specified
granularity.

The length of all array elements is the length of the class passed as the 
template parameter. The length must be non-zero.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array. 

@panic E32USER-CBase 17, if the length of the class implied by the template
                         parameter is zero.
@panic E32USER-CBase 18, if aGranularity is not positive.
*/
	{}




inline CArrayFixSeg<TAny>::CArrayFixSeg(TInt aRecordLength,TInt aGranularity)
	: CArrayFix<TAny>((TBufRep)CBufSeg::NewL,aRecordLength,aGranularity)
/**
Constructs a segmented array of fixed length objects with the specified
granularity to contain elements of the specified length.
	
Note that no memory is allocated to the array buffer by this constructor.
	
@param aRecordLength The length of the elements of this array.

@param aGranularity The granularity of the array.

@panic E32USER-CBase 17, if aRecordLength is not positive.
@panic E32USER-CBase 18, if aGranularity is not positive.
*/
	{}




// Template class CArrayPtr
template <class T>
inline CArrayPtr<T>::CArrayPtr(TBufRep aRep,TInt aGranularity)
	: CArrayFix<T*>(aRep,aGranularity)
/**
@internalComponent
*/
	{}




template <class T>
void CArrayPtr<T>::ResetAndDestroy()
/**
Destroys all objects whose pointers form the elements of the array, before 
resetting the array.

The destructor of each class T object is called before the objects themselves 
are destroyed.

If the array is not empty, this member function must be called before the 
array is deleted to prevent the CBase derived objects from being orphaned 
on the heap.

Note that each call to this function results in a small, but non-trivial,
amount of code being generated.
*/
	{
	for (TInt i=0;i<this->Count();++i)
		delete this->At(i);
	this->Reset();
	}




// Template class CArrayPtrFlat
template <class T>
inline CArrayPtrFlat<T>::CArrayPtrFlat(TInt aGranularity)
	: CArrayPtr<T>((TBufRep)CBufFlat::NewL,aGranularity)
/** 
Constructs a flat array of pointers with specified granularity.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array. 

@panic E32USER-CBase 18, if aGranularity is not positive.
*/
	{}




template <class T>
inline void CArrayPtrFlat<T>::SetReserveL(TInt aCount)
/**
Reserves space in the array buffer.

If necessary, the array buffer is allocated or re-allocated so that it can
accommodate the specified number of elements.

After a successful call to this function, elements can be added to the array 
and the process is guaranteed not to fail for lack of memory - provided the 
total number of elements does not exceed the specified number.

This function does not increase the number of elements in the array; i.e. 
the member function CArrayFixBase::Count() returns the same value both before 
and after a call to this function.

@param aCount The total number of elements for which space is to be reserved. 

@panic E32USER-CBase 27, if aCount is less than the current number of elements
       in the array.
*/
	{this->SetReserveFlatL(aCount);}




// Template class CArrayPtrSeg
template <class T>
inline CArrayPtrSeg<T>::CArrayPtrSeg(TInt aGranularity)
	: CArrayPtr<T>((TBufRep)CBufSeg::NewL,aGranularity)
/**
Constructs a segmented array of pointers with specified granularity.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array. 

@panic E32USER-CBase 18, if aGranularity is not positive.
*/
	{}




// Class CArrayVarBase
inline TInt CArrayVarBase::Count() const
/**
Gets the number of elements held in the array.

@return The number of array elements.
*/
	{return(iCount);}




// Template class CArrayVar
template <class T>
inline CArrayVar<T>::CArrayVar(TBufRep aRep,TInt aGranularity)
	: CArrayVarBase(aRep,aGranularity)
/**
@internalComponent
*/
	{}




template <class T>
inline const T &CArrayVar<T>::operator[](TInt anIndex) const
/** 
Gets a reference to the const element located at the specified position 
within the array.

The compiler uses this variant of the function if the returned reference is 
used in an expression where it cannot be modified.

@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.
			   
@return A const reference to the element located at position anIndex within 
        the array. 

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(*((const T *)CArrayVarBase::At(anIndex)));}




template <class T>
inline T &CArrayVar<T>::operator[](TInt anIndex)
/**
Gets a reference to the element located at the specified position within 
the array.

The compiler uses this variant of the function if the returned reference is 
used in an expression where it can be modified.

@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.


@return A reference to the non-const element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(*((T *)CArrayVarBase::At(anIndex)));}




template <class T>
inline const T &CArrayVar<T>::At(TInt anIndex) const
/** 
Gets a reference to the const element located at the specified position 
within the array.

The compiler uses this variant of the function if the returned reference is 
used in an expression where it cannot be modified.

@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.
			   
@return A const reference to the element located at position anIndex within 
        the array. 

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(*((const T *)CArrayVarBase::At(anIndex)));}



template <class T>
inline T &CArrayVar<T>::At(TInt anIndex)
/**
Gets a reference to the element located at the specified position within 
the array.

The compiler uses this variant of the function if the returned reference is 
used in an expression where it can be modified.

@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.

@return A reference to the non-const element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(*((T *)CArrayVarBase::At(anIndex)));}




template <class T>
inline void CArrayVar<T>::AppendL(const T &aRef,TInt aLength)
/**
Appends an element of a specified length onto the array.

@param aRef    A reference to the <class T> element to be appended.
@param aLength The length of the element to be appended.

@leave KErrNoMemory The function always attempts to allocate a cell to
       contain the new element and may also attempt to expand the array buffer.
	   If there is insufficient memory available, the function leaves, in which
	   case, the array is left in the state it was in before the call.

@panic E32USER-CBase 30, if aLength is negative.
*/
	{CArrayVarBase::InsertL(Count(),&aRef,aLength);}




template <class T>
inline T &CArrayVar<T>::ExpandL(TInt anIndex,TInt aLength)
/**
Expands the array by one element of specified length at the specified position. 

It:

1. expands the array by one element position anIndex

2. constructs a new element of specified length at that position

3. returns a reference to the new element.

All existing elements from position anIndex to the end of the array are moved 
up, so that the element originally at position anIndex is now at position 
anIndex + 1 etc.

The new element of type <class T> and length aLength is constructed at position 
anIndex, using the default constructor of that class.

@param anIndex The position within the array where the array is to be expanded 
               and the new <class T> object is to be constructed.
			   
@param aLength The length of the new element. 

@return A reference to the newly constructed <class T> object at position 
        anIndex within the array.

@leave KErrNoMemory The function always attempts to allocate a cell to contain 
       the new element and may also attempt to expand the array buffer. If there 
       is insufficient memory available, the function leaves, in which case, the 
       array is left in the state it was in before the call.

@panic E32USER-CBase 21, if anIndex is negative or is greater than the number
       of elements currently in the array.
@panic E32USER-CBase 30, if aLength is negative.
*/
	{return(*new(CArrayVarBase::ExpandL(anIndex,aLength)) T);}




template <class T>
inline T &CArrayVar<T>::ExtendL(TInt aLength)
/**
Expands the array by one element of specified length at the end of the array. 

It:

1. expands the array by one element at the end of the array, i.e. at position 
   CArrayVarBase::Count()

2. constructs a new element of specified length at that position.

3. returns a reference to the new element.

The new element of type <class T> is constructed at the end of the array, 
using the default constructor of that class.

@param aLength The length of the new element.

@return A reference to the newly constructed <class T> object at the end of 
        the array.

@leave KErrNoMemory The function always attempts to allocate a cell to contain 
       the new element and may also attempt to expand the array buffer. If there 
       is insufficient memory available, the function leaves, in which case, the 
       array is left in the state it was in before the call.
  
@panic E32USER-CBase 30, if aLength is negative.
*/
	{return(*new(CArrayVarBase::ExpandL(Count(),aLength)) T);}




template <class T>
inline TInt CArrayVar<T>::Find(const T &aRef,TKeyArrayVar &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of 
keys, using a sequential search.

The array is searched sequentially for an element whose key matches the key 
of the supplied object. The search starts with the first element in the array.

Note that where an array has elements with duplicate keys, the function only
supplies the position of the first element in the array with that key.

@param aRef    A reference to an object of type <class T> whose key is used 
               for comparison.
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A TInt supplied by the caller. On return, if the element is
               found, this is set to the position of that element
			   within the array. The position is relative to zero, (i.e. 
               the first element in the array is at position 0).
			   If the element is not found or the array is empty, then
			   this is undefined.

@return Zero, if the element with the specified key is found. Non-zero, if 
        the element with the specified key is not found.
*/
	{return(CArrayVarBase::Find(&aRef,aKey,anIndex));}




template <class T>
inline TInt CArrayVar<T>::FindIsq(const T &aRef,TKeyArrayVar &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of 
keys, using a binary search technique.

The array is searched, using a binary search technique, for an element whose 
key matches the key of the supplied <class T> object.

The array must be in key order.

Note that where an array has elements with duplicate keys, the function cannot
guarantee which element, with the given key value, it will return, except that
it will find one of them.

@param aRef    A reference to an object of type <class T> whose key is used 
               for comparison.
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A TInt supplied by the caller. On return, if the element is
               found, this is set to the position  of that element within
			   the array. The position is relative to zero, (i.e. 
               the first element in the array is at position zero).
			   If the element is not found and the array is not empty, then
			   this is set to the position of the first element in the array
			   with a key which is greater than the key of the object aRef.
			   If the element is not found and the array is empty, then 
               this is undefined.

@return Zero, if the element with the specified key is found or the array is 
        empty. Non-zero, if the element with the specified key is not found.
*/
	{return(CArrayVarBase::FindIsq(&aRef,aKey,anIndex));}




template <class T>
inline void CArrayVar<T>::InsertL(TInt anIndex,const T &aRef,TInt aLength)
/**
Inserts an element of a specified length into the array at the specified
position.

Note that passing a value of anIndex which is the same as the current number
of elements in the array, has the effect of appending that element.

@param anIndex The position within the array where the element is to be
               inserted. The position is relative to zero, i.e. zero implies
			   that elements are inserted at the beginning of the array.
@param aRef    A reference to the <class T> object to be inserted into
               the array.
@param aLength The length of the element to be inserted into the array. 

@leave KErrNoMemory The function always attempts to allocate a cell to contain 
       the new element and may also attempt to expand the array buffer. If
	   there is insufficient memory available, the function leaves, in which
	   case, the array is left in the state it was in before the call.

@panic E32USER-CBase 21, if anIndex is negative or is greater than the number
       of objects currently in the array.
@panic E32USER-CBase 30, if aLength is is negative.
*/
	{CArrayVarBase::InsertL(anIndex,&aRef,aLength);}




template <class T>
inline TInt CArrayVar<T>::InsertIsqL(const T &aRef,TInt aLength,TKeyArrayVar &aKey)
/**
Inserts a single element of a specified length into the array at a position 
determined by a key.

The array MUST already be in key sequence (as defined by the key), otherwise 
the position of the new element is unpredictable, or duplicates may occur.

Elements with duplicate keys are not permitted.

@param aRef    A reference to the element of type <class T> to be inserted into 
               the array.
@param aLength The length of the new element of type <class T> to be inserted 
               into the array.
@param aKey    A reference to a key object defining the properties of the key.

@return The position within the array of the newly inserted element.

@leave KErrAlreadyExists An element with the same key already exists within 
       the array. NB the array MUST already be in key sequence, otherwise
	   the function may insert a duplicate and fail to leave with
	   this value.
@leave KErrNoMemory The function always attempts to allocate a cell to contain 
       the new element and may also attempt to expand the array buffer. If
	   there is insufficient memory available, the function leaves, in which
	   case, the array is left in the state it was in before the call.
*/
	{return(CArrayVarBase::InsertIsqL(&aRef,aLength,aKey));}




template <class T>
inline TInt CArrayVar<T>::InsertIsqAllowDuplicatesL(const T &aRef,TInt aLength,TKeyArrayVar &aKey)
/**
Inserts a single element of a specified length into the array at a position 
determined by a key, allowing duplicates.

The array MUST already be in key sequence, otherwise the position of the 
new element is unpredictable.

Elements with duplicate keys are permitted. If the new element's key is a 
duplicate of an existing element's key, then the new element is positioned 
after the existing element.

@param aRef    A reference to the element of type <class T> to be inserted
               into the array.
@param aLength The length of the new element to be inserted into the array. 
@param aKey    A reference to a key object defining the properties of the key. 

@return The position within the array of the newly inserted element.

@leave KErrNoMemory The function always attempts to allocate a cell to contain 
       the new element and may also attempt to expand the array buffer. If
	   there is insufficient memory available, the function leaves, in which
	   case, the array is left in the state it was in before the call.
*/
	{return(CArrayVarBase::InsertIsqAllowDuplicatesL(&aRef,aLength,aKey));}




template <class T>
inline const TArray<T> CArrayVar<T>::Array() const
/**
Constructs and returns a TArray<T> object.

@return A TArray<T> object for this array.
*/
	{return(TArray<T>(CountR,AtR,this));}




inline const TAny *CArrayVar<TAny>::At(TInt anIndex) const
/**
Returns a pointer to the untyped element located at the specified position 
within the array.
	
The compiler uses this variant of the function if the returned pointer is 
used in an expression where it cannot be modified.
	
@param anIndex The position of the element within the array, relative to zero; 
	           i.e. zero implies the first element.

@return A pointer to the const element located at position anIndex within the 
        array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(CArrayVarBase::At(anIndex));}




inline CArrayVar<TAny>::CArrayVar(TBufRep aRep,TInt aGranularity)
	: CArrayVarBase(aRep,aGranularity)
/**
Constructs a variable array with the specified granularity and buffer
organization.
	
Note that no memory is allocated to the array buffer by this constructor.
	
@param aRep         A pointer to a function used to expand the array buffer.
                    The organisation of the array buffer is implied by the
					choice of this function.
			        For a flat array buffer, pass (TBufRep)CBufFlat::NewL.
			        For a segmented array buffer, pass (TBufRep)CBufSeg::NewL. 
@param aGranularity The granularity of the array. 

@panic E32USER-CBase 19, if aGranularity is not positive.
*/
	{}




inline TAny *CArrayVar<TAny>::At(TInt anIndex)
/**
Returns a pointer to the untyped element located at the specified position 
within the array.
	
The compiler uses this variant of the function if the returned pointer is 
used in an expression where it can be modified.
	
@param anIndex The position of the element within the array, relative to zero; 
	           i.e. zero implies the first element.

@return A pointer to the non-const element located at position anIndex within the 
        array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of objects currently within the array.
*/
	{return(CArrayVarBase::At(anIndex));}




inline void CArrayVar<TAny>::AppendL(const TAny *aPtr,TInt aLength)
/**
Appends an untyped element of specified length onto the end of the array.
	
@param aPtr    A pointer to an untyped element to be appended. 
@param aLength The length of the untyped element.
*/
	{CArrayVarBase::InsertL(Count(),aPtr,aLength);}




inline TAny *CArrayVar<TAny>::ExtendL(TInt aLength)
/**
Extends the array by one element of specified length at the end of the array,
i.e. at position CArrayVarBase::Count(), and returns a pointer to the new
element location.

As elements are untyped, no construction is possible and the content of the
new location remains undefined.

Note that the function always attempts to allocate a cell to contain the new
element and may also attempt to expand the array buffer. If there is
insufficient memory available, the function leaves.
The leave code is one of the system wide error codes.
If the function leaves, the array is left in the state it was in before
the call. 

@param aLength The length of the new element.

@return A pointer to the new element location at the end of the array. 
*/
	{return(CArrayVarBase::ExpandL(Count(),aLength));}




// Template class CArrayVarFlat
template <class T>
inline CArrayVarFlat<T>::CArrayVarFlat(TInt aGranularity)
	: CArrayVar<T>((TBufRep)CBufFlat::NewL,aGranularity)
/**
Constructs a variable flat array with specified granularity.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array. 

@panic E32USER-CBase 19, if aGranularity is not positive.
*/
	{}




// Template class CArrayVarSeg
template <class T>
inline CArrayVarSeg<T>::CArrayVarSeg(TInt aGranularity)
	: CArrayVar<T>((TBufRep)CBufSeg::NewL,aGranularity)
/**
Constructs a variable segmented array with specified granularity.

Note that no memory is allocated to the array buffer by this constructor.

@param aGranularity The granularity of the array.

@panic E32USER-CBase 19, if aGranularity is not positive.
*/
	{}




// Class CArrayPakBase
inline TInt CArrayPakBase::Count() const
/**
Gets the number of elements held in the array.

@return The number of array elements.
*/
	{return(iCount);}




// Template class CArrayPak
template <class T>
inline CArrayPak<T>::CArrayPak(TBufRep aRep,TInt aGranularity)
	: CArrayPakBase(aRep,aGranularity)
/**
@internalComponent
*/
	{}




template <class T>
inline const T &CArrayPak<T>::operator[](TInt anIndex) const
/**
Gets a reference to the element located at the specified position within the 
array.

The compiler uses this variant of the function when the returned reference 
is used in an expression where it cannot be modified.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.  
		
@return A const reference to the element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       number of objects currently within the array.
*/
	{return(*((const T *)CArrayPakBase::At(anIndex)));}




template <class T>
inline T &CArrayPak<T>::operator[](TInt anIndex)
/**
Gets a reference to the element located at the specified position within the 
array.

The compiler uses this variant of the function when the returned reference 
is used in an expression where it can be modified.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.  
		
@return A non-const reference to the element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       number of objects currently within the array.
*/
	{return(*((T *)CArrayPakBase::At(anIndex)));}




template <class T>
inline const T &CArrayPak<T>::At(TInt anIndex) const
/**
Gets a reference to the element located at the specified position within the 
array.

The compiler uses this variant of the function when the returned reference 
is used in an expression where it cannot be modified.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.  
		
@return A const reference to the element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       number of objects currently within the array.
*/
	{return(*((const T *)CArrayPakBase::At(anIndex)));}




template <class T>
inline T &CArrayPak<T>::At(TInt anIndex)
/**
Gets a reference to the element located at the specified position within the 
array.

The compiler uses this variant of the function when the returned reference 
is used in an expression where it can be modified.

@param anIndex The position of the element within the array. The position 
               is relative to zero; i.e. zero implies the first element in
			   the array.  
		
@return A non-const reference to the element located at position anIndex within 
        the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       number of objects currently within the array.
*/
	{return(*((T *)CArrayPakBase::At(anIndex)));}




template <class T>
inline void CArrayPak<T>::AppendL(const T &aRef,TInt aLength)
/**
Appends an element of a specified length onto the array.

@param aRef    A reference to the class T element to be appended.
@param aLength The length of the element to be appended.

@leave KErrNoMemory The function attempted to allocate from the heap and there 
       is insufficient memory available. In this case, the array is left in
	   the state it was in before the call.

@panic E32USER-CBase 30, if aLength is negative.
*/
	{CArrayPakBase::InsertL(Count(),&aRef,aLength);}




template <class T>
inline T &CArrayPak<T>::ExpandL(TInt anIndex,TInt aLength)
/**
Expands the array by one element of specified length at the specified position. 

It:

1. expands the array by one element at the specified position.

2. constructs a new element of specified length at that position.

3. returns a reference to the new element.

All existing elements from position anIndex to the end of the array are moved 
up, so that the element originally at position anIndex is now at position 
anIndex + 1 etc.

The new element of type <class T> and length aLength is constructed at position 
anIndex, using the default constructor of that class.

@param anIndex The position within the array where the array is to be expanded 
               and the new <class T> object is to be constructed. 
@param aLength The length of the new element.

@return A reference to the newly constructed <class T> object at position 
        anIndex within the array.

@leave KErrNoMemory The function attempted to allocate from the heap and there 
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.

@panic E32USER-CBase 21, if anIndex is negative or greater than the number of
       elements currently in the array.
@panic E32USER-CBase 30, if aLength is negative.
*/
	{return(*new(CArrayPakBase::ExpandL(anIndex,aLength)) T);}




template <class T>
inline T &CArrayPak<T>::ExtendL(TInt aLength)
/**
Expands the array by one element of specified length at the end of the array. 

It:

1. expands the array by one element at the end of the array, i.e. at position 
   CArrayPakbase::Count().

2. constructs a new element of length aLength at that position.

3. returns a reference to the new element.

The new element of type <class T> is constructed at the end of the array, 
using the default constructor of that class.

@param aLength The length of the new element.

@return A reference to the newly constructed <class T> object at the end of 
        the array.

@leave KErrNoMemory The function attempted to allocate from the heap and there 
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.

@panic E32USER-CBase 30, if aLength is negative.
*/
	{return(*new(CArrayPakBase::ExpandL(Count(),aLength)) T);}




template <class T>
inline TInt CArrayPak<T>::Find(const T &aRef,TKeyArrayPak &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of 
keys, using a sequential search.

The array is searched sequentially for an element whose key matches the key 
of the supplied <class T> object. The search starts with the first element 
in the array.

Note that where an array has elements with duplicate keys, the function only
supplies the position of the first element in the array with that key.

@param aRef    A reference to an object of type <class T> whose key is used 
               for comparison.
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A reference to a TInt supplied by the caller. On return, if the
               element is found, this is set to the position 
               of that element within the array. The position is relative to zero, (i.e. 
               the first element in the array is at position 0).
			   If the element is not found or the array is empty, then this is undefined.

@return Zero, if the element with the specified key is found. Non-zero, if 
        the element with the specified key is not found.
*/
	{return(CArrayPakBase::Find(&aRef,aKey,anIndex));}




template <class T>
inline TInt CArrayPak<T>::FindIsq(const T &aRef,TKeyArrayPak &aKey,TInt &anIndex) const
/**
Finds the position of an element within the array, based on the matching of 
keys, using a binary search technique.

The array is searched, using a binary search technique, for an element whose 
key matches the key of the supplied <class T> object.

The array must be in key order.

Note that where an array has elements with duplicate keys, the function cannot
guarantee which element, with the given key value, it will return, except that it
will find one of them.

@param aRef    A reference to an object of type <class T> whose key is used 
               for comparison.
@param aKey    A reference to a key object defining the properties of the key.
@param anIndex A reference to a TInt supplied by the caller. On return, if the
               element is found, this is set to the position of that element
			   within the array. The position is relative to zero, (i.e. 
               the first element in the array is at position 0).
			   If the element is not found and the array is not empty, then
			   this is set to the position of the first element in the array
			   with a key which is greater than the key of the object aRef.
			   If the element is not found and the array is empty, then this 
               is undefined.

@return Zero, if the element with the specified key is found or the array is 
        empty.
		Non-zero, if the element with the specified key is not found.
*/
	{return(CArrayPakBase::FindIsq(&aRef,aKey,anIndex));}




template <class T>
inline void CArrayPak<T>::InsertL(TInt anIndex,const T &aRef,TInt aLength)
/** 
Inserts an element of a specified length into the array at the specified
position.

@param anIndex The position within the array where the element is to be
               inserted. The position is relative to zero, i.e. zero implies
			   that elements are inserted at the beginning of the array.
@param aRef    A reference to the class T object to be inserted into
               the array.
@param aLength The length of the element to be inserted into the array.

@leave KErrNoMemory The function attempted to expand the array buffer and there
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.

@panic E32USER-CBase 21, if anIndex is negative or greater than the number of
       objects currently in the array.
@panic E32USER-CBase 30, if aLength is negative.
*/
	{CArrayPakBase::InsertL(anIndex,&aRef,aLength);}




template <class T>
inline TInt CArrayPak<T>::InsertIsqL(const T &aRef,TInt aLength,TKeyArrayPak &aKey)
/**
Inserts a single element of a specified length into the array at a position 
determined by a key.

The array MUST already be in key sequence (as defined by the key), otherwise 
the position of the new element is unpredictable, or duplicates may occur.

Elements with duplicate keys are not permitted.

@param aRef    A reference to the element of type <class T> to be inserted into 
               the array.
@param aLength The length of the new element of type <class T> to be inserted 
               into the array.
@param aKey    A reference to a key object defining the properties of the key.

@return The position within the array of the newly inserted element.

@leave KErrAlreadyExists An element with the same key already exists within 
       the array. NB the array MUST already be in key sequence, otherwise
	   the function may insert a duplicate and fail to leave with
	   this value.
@leave KErrNoMemory The function attempted to expand the array buffer and there 
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.
*/
	{return(CArrayPakBase::InsertIsqL(&aRef,aLength,aKey));}




template <class T>
inline TInt CArrayPak<T>::InsertIsqAllowDuplicatesL(const T &aRef,TInt aLength,TKeyArrayPak &aKey)
/**
Inserts a single element of a specified length into the array at a position 
determined by a key, allowing duplicates.

The array MUST already be in key sequence, otherwise the position of the 
new element is unpredictable.

Elements with duplicate keys are permitted. If the new element's key is a 
duplicate of an existing element's key, then the new element is positioned 
after the existing element.

@param aRef    A reference to the element of type <class T> to be inserted into
               the array.
@param aLength The length of the new element to be inserted into the array.
@param aKey    A reference to a key object defining the properties of the key.

@return The position within the array of the newly inserted element. 

@leave KErrNoMemory The function attempted to expand the array buffer and there 
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.
*/
	{return(CArrayPakBase::InsertIsqAllowDuplicatesL(&aRef,aLength,aKey));}




template <class T>
inline const TArray<T> CArrayPak<T>::Array() const
/**
Constructs and returns a TArray<T> object.

@return A Tarray<T> object for this array.
*/
	{return(TArray<T>(CountR,AtR,this));}




inline CArrayPak<TAny>::CArrayPak(TBufRep aRep,TInt aGranularity)
	: CArrayPakBase(aRep,aGranularity)
/**
Constructs a variable array with the specified granularity and
buffer organisation.
	
Note that no memory is allocated to the array buffer by this constructor.
	
@param aRep         A pointer to a function used to expand the array buffer. 
                    The organisation of the array buffer is implied by the
					choice of this function.
	                For a flat array buffer, pass (TBufRep)CBufFlat::NewL.
			        For a segmented array buffer, pass (TBufRep)CBufSeg::NewL. 
@param aGranularity The granularity of the array. 

@panic E32USER-CBase 19, if aGranularity is not positive.
*/
	{}




inline const TAny *CArrayPak<TAny>::At(TInt anIndex) const
/**
Gets a pointer to the untyped element located at the specified position 
within the array.
	
The compiler uses this variant of the function if the returned reference is 
used in an expression where that reference cannot be modified.
	
@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.

@return A pointer to the const element located at position anIndex within 
	    the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayPakBase::At(anIndex));}




inline TAny *CArrayPak<TAny>::At(TInt anIndex)
/**
Gets a pointer to the untyped element located at the specified position 
within the array.
	
The compiler uses this variant of the function if the returned reference is 
used in an expression where that reference can be modified.
	
@param anIndex The position of the element within the array, relative to zero; 
               i.e. zero implies the first element.

@return A pointer to the non-const element located at position anIndex within 
	    the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to
       the number of objects currently within the array.
*/
	{return(CArrayPakBase::At(anIndex));}




inline void CArrayPak<TAny>::AppendL(const TAny *aPtr,TInt aLength)
/**
Appends the untyped element of the specified length onto the end of the array.
	
@param aPtr    A pointer to an untyped element to be appended. 
@param aLength The length of the untyped element. 

@leave KErrNoMemory The function attempted to expand the array buffer and there 
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.
*/
	{CArrayPakBase::InsertL(Count(),aPtr,aLength);}




inline TAny *CArrayPak<TAny>::ExtendL(TInt aLength)
/**
Expands the array by one element of the specified length at the end of
the array, and returns a pointer to this new location.

As elements are untyped, no construction is possible and the content of
the new location remains undefined.

@param aLength  The length of the new element.

@return A pointer to the new element location at the end of the array.

@leave KErrNoMemory The function attempted to expand the array buffer and there
       is insufficient memory available. In this case, the array is left in the
	   state it was in before the call.
*/
	{return(CArrayPakBase::ExpandL(Count(),aLength));}




// Template class CArrayPakFlat
template <class T>
inline CArrayPakFlat<T>::CArrayPakFlat(TInt aGranularity)
	: CArrayPak<T>((TBufRep)CBufFlat::NewL,aGranularity)
/**
Constructs a packed flat array with specified granularity.

Note that no memory is allocated to the array buffer by this  constructor.

@param aGranularity The granularity of the array.

@panic E32USER-CBase 20, if aGranularity is not positive.
*/
	{}




// Class CObject
inline TInt CObject::UniqueID() const
/**
Gets this reference counting object's unique ID.

The unique ID is an integer which is a property of the object container. It 
forms part of the identity of all reference counting objects and is the same 
value for all reference counting objects held within the same object container.

@return This reference counting object's unique ID.

@see CObjectCon
*/
	{return(iContainer->UniqueID());}




inline TInt CObject::AccessCount() const
/**
Gets the number of open references to this reference counting object.

@return The number of open references.
*/
	{return(iAccessCount);}




inline void CObject::Inc()
/**
Adds one to the reference count.

This function is called by the default implementation of the Open() member 
function of this class.

@see CObject::Open
*/
	{iAccessCount++;}




inline void CObject::Dec()
/**
Subtracts one from the reference count.

This function is called by the default implementation of the Close() member 
function of this class.

@see CObject::Close
*/
	{iAccessCount--;}




inline CObject * CObject::Owner() const
/**
Gets a pointer to the reference counting object which owns this
reference counting object.

@return A pointer to the owning reference counting object. This is NULL, if 
        there is no owner.
*/
	{return(iOwner);}




inline void CObject::SetOwner(CObject *anOwner)
/**
Sets the owner of this reference counting object.

If this reference counting object already has an owner, then all knowledge 
of that owner is lost.

@param anOwner A pointer to the reference counting object which is to be the 
               new owner of this reference counting object.
*/
	{iOwner=anOwner;}




// class CObjectIx
inline TInt CObjectIx::Count() const
/**
Gets the number greater then the last slot number used to hold valid CObject pointer.
The input argument of CObject* CObjectIx::operator[]() must be less then the number returned by this method.

@return The number greater then the last used slot.

@see CObjectIx::ActiveCount
@see CObjectIx::operator[]
*/
	{return iHighWaterMark;}




inline TInt CObjectIx::ActiveCount() const
/**
Gets the current number of reference counting objects held by this
object index.

@return The current number.
*/
	{return iNumEntries;}



// class CObjectCon
inline TInt CObjectCon::UniqueID() const
/**
Gets this object container's unique ID.

@return The unique ID value.
*/
	{return iUniqueID;}




inline TInt CObjectCon::Count() const
/**
Gets the number of reference counting objects in this object container.

@return The number of objects.
*/
	{return iCount;}





// class CCirBufBase
inline TInt CCirBufBase::Count() const
/**
Gets the current number of objects in this circular buffer.

@return The number of objects in this circular buffer.
        This value can never be greater than the maximum capacity.
*/
	{return(iCount);}




inline TInt CCirBufBase::Length() const
/**
Gets the maximum capacity of this circular buffer.

The capacity is the maximum number of elements that the buffer can hold.

Use SetLengthL() to change the capacity of the circular buffer.

@return The maximum capacity of this circular buffer.

@see CCirBufBase::SetLengthL
*/
	{return(iLength);}




// Template class CCirBuf
template <class T>
inline CCirBuf<T>::CCirBuf()
	: CCirBufBase(sizeof(T))
/**
Default C++ constructor.

The size of each object in the buffer is fixed and is the length of the class 
passed as the template parameter. 

@panic E32USER-CBase 72, if the length of the template class is zero.
*/
	{}




template <class T>
inline TInt CCirBuf<T>::Add(const T *aPtr)
/**
Adds a single object to the circular buffer.

The object is of type class T and is only added if there is space available.

@param aPtr A pointer to the object of type class T to be added to the circular 
            buffer.

@return 1 if the object is successfully added. 0 if the object cannot be added 
        because the circular buffer is full.

@panic E32USER-CBase 74, if a call to CCirBufBase::SetLengthL() has not been
                         made before calling this function.

@see CCirBufBase::SetLengthL
*/
	{return(DoAdd((const TUint8 *)aPtr));}




template <class T>
inline TInt CCirBuf<T>::Add(const T *aPtr,TInt aCount)
/**
Adds multiple objects to the circular buffer.

The function attempts to add aCount objects of type class T. The objects are
only added if there is space available.

@param aPtr   A pointer to a set of contiguous objects of type class T to be 
              added to the circular buffer.

@param aCount The number of objects to be added to the circular buffer.

@return The number of objects successfully added to the buffer. This value 
        may be less than the number requested and can range from 0 to aCount. 

@panic E32USER-CBase 74, if a call to CCirBufBase::SetLengthL() has not been
                         made before calling this function.
@panic E32USER-CBase 75, if aCount is not a positive value. 

@see CCirBufBase::SetLengthL
*/
	{return(DoAdd((const TUint8 *)aPtr,aCount));}




template <class T>
inline TInt CCirBuf<T>::Remove(T *aPtr)
/**
Removes a single object from the circular buffer.

An object can only be removed if there are objects in the buffer.

A binary copy of the object is made to aPtr.

@param aPtr A pointer to an object of type class T supplied by the caller.

@return 1 if an object is successfully removed. 0 if an object cannot be removed 
        because the circular buffer is empty.
*/
	{return(DoRemove((TUint8 *)aPtr));}




template <class T>
inline TInt CCirBuf<T>::Remove(T *aPtr,TInt aCount)
/**
Removes multiple objects from the circular buffer.

The function attempts to remove aCount objects of type class T.
Objects can only be removed if there are objects in the buffer

A binary copy of the objects is made to aPtr.

@param aPtr   A pointer to contiguous memory able to hold aCount class T objects, 
              supplied by the caller. 

@param aCount The number of objects to be removed from the circular buffer.

@return The number of objects successfully removed from the buffer. This value
        may be less than the number requested, and can range from 0 to aCount.

@panic E32USER-CBase 76, if aCount is not a positive value.
*/
	{return(DoRemove((TUint8 *)aPtr,aCount));}




// Class CActive
inline TBool CActive::IsActive() const
/**
Determines whether the active object has a request outstanding.

A request is outstanding when:

1. it has been issued

2. it has not been cancelled

3. it servicing has not yet begun.

@return True, if a request is outstanding; false, otherwise.
*/
	{return(iStatus.iFlags&TRequestStatus::EActive);}




inline TBool CActive::IsAdded() const
/**
Determines whether the active object has been added to the active scheduler's 
list of active objects.

If the active object has not been added to a scheduler, it cannot handle the 
completion of any request. No request should be issued until the active object 
has been added to a scheduler because completion of that request generates 
what appears to be a stray signal.

Use the active object function Deque() to remove the active object from the 
scheduler.

@return True, if the active object has been added to an active scheduler; 
        false, otherwise. 

@see CActive::Deque
*/
	{return(iLink.iNext!=NULL);}




inline TInt CActive::Priority() const
/**
Gets the priority of the active object.

@return The active object's priority value.
*/
	{return iLink.iPriority;}




// class CDeltaTimer
inline TDeltaTimerEntry::TDeltaTimerEntry(const TCallBack& aCallback)
/**
Constructor specifying a general callback.

@param aCallback The callback to be called when this timed event entry expires.
*/
	{iCallBack=aCallback;}




inline TDeltaTimerEntry::TDeltaTimerEntry()
/**
Default constructor.
*/
	{}




inline void TDeltaTimerEntry::Set(TCallBack& aCallback)
/**
Sets the specified callback.

@param aCallback The callback to be called when this timed event entry expires.
*/
	{iCallBack=aCallback;}




/**
Gets a reference to the server's current message.

@return The current message that contains the client request details.
*/
inline const RMessage2 &CServer2::Message() const
	{return iMessage;}



	
/**
Gets the server active object that handles messages for this session.

This is the instance of the CServer2 derived class that created
this session object.

@return The server active object.
*/
inline const CServer2 *CSession2::Server() const
	{return iServer;}




// Class CAsyncOneShot
inline RThread& CAsyncOneShot::Thread()
/**
Gets a handle to the current thread.

@return The handle to the current thread.
*/
	{ return iThread; }




// Class CActiveScheduler
inline TInt CActiveScheduler::Level() const
/**
@deprecated Use the StackDepth() function instead.

Gets the scheduler's level of nestedness.

@return The level of nestedness.

@see StackDepth()
*/
	{return StackDepth();}




// Class CActiveSchedulerWait
inline TBool CActiveSchedulerWait::IsStarted() const
/**
Reports whether this CActiveSchedulerWait object is currently started.

Note: a CActiveSchedulerWait object itself becomes "stopped" as
soon as AsyncStop() is called, and can be started again immediately if
required (but this would start a new nested level of the scheduler).

@return True if the scheduling loop is active; false, otherwise.

@see CActiveSchedulerWait::Start
@see CActiveSchedulerWait::AsyncStop
*/
	{return iLoop != NULL;}




// Class CleanupStack
#ifdef _DEBUG
inline void CleanupStack::Pop(TAny* aExpectedItem)
/**
Pops an object from the top of the cleanup stack.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function checks that the specified item is at the top 
   of the cleanup stack before attempting to pop it; an E32USER-CBase 90 panic 
   is raised if the check fails.

2  In a release build, the function just pops the object which is at the top 
   of the cleanup stack; no checking is done.

@param aExpectedItem A pointer to the item expected to be at the top of the 
                     cleanup stack. In a release build, this parameter
					 is not used.
*/
	{ CleanupStack::Check(aExpectedItem); CleanupStack::Pop(); }




inline void CleanupStack::Pop(TInt aCount, TAny* aLastExpectedItem)
/**
Pops the specified number of objects from the top of the cleanup stack.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function pops (aCount-1) items from the cleanup stack, 
   and then checks that the specified item is the next one on the cleanup stack 
   before attempting to pop it; an E32USER-CBase 90 panic is raised if the
   check fails.

2. In a release build, the function just pops aCount items from the cleanup stack; 
   no checking is done.

@param aCount            The number of items top be popped from
                         the cleanup stack.
@param aLastExpectedItem A pointer to the item expected to be at the top of 
                         the cleanup stack, after (aCount-1) items have been
						 popped. In a release build, this parameter is
						 not used.
*/
	{
	if (--aCount)
		CleanupStack::Pop(aCount);
	CleanupStack::Check(aLastExpectedItem);
	CleanupStack::Pop();
	}




inline void CleanupStack::PopAndDestroy(TAny* aExpectedItem)
/**
Pops an object from the top of the cleanup stack, and cleans it up.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function checks that the specified item is at the top 
   of the cleanup stack before attempting to pop and clean it up;
   an E32USER-CBase 90 panic is raised if the check fails.

2. In a release build, the function just pops and cleans up the object at 
   the top of the cleanup stack; no checking is done.

@param aExpectedItem A pointer to the item expected to be at the top of the 
                     cleanup stack. In a release build, this parameter is
					 not used.
*/
	{ CleanupStack::Check(aExpectedItem); CleanupStack::PopAndDestroy(); }




inline void CleanupStack::PopAndDestroy(TInt aCount, TAny* aLastExpectedItem)
/**
Pops the specified number of objects from the top of the cleanup stack, and 
cleans them up.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function pops and cleans up (aCount-1) items from the 
   cleanup stack, and then checks that the specified item is the next one on 
   the cleanup stack before attempting to pop it and clean it up;
   an E32USER-CBase  90 panic is raised if the check fails.

2. In a release build, the function just pops and cleans up aCount items from 
   the cleanup stack; no checking is done.

@param aCount            The number of items top be popped from the
                         cleanup stack, and cleaned up.
@param aLastExpectedItem A pointer to the item expected to be at the top of 
                         the cleanup stack, after (aCount-1) items have been
						 popped and cleaned up. In a release build, this 
						 parameter is not used.
*/
	{
	if (--aCount)
		CleanupStack::PopAndDestroy(aCount);
	CleanupStack::Check(aLastExpectedItem);
	CleanupStack::PopAndDestroy();
	}
#else
inline void CleanupStack::Pop(TAny*)
/**
Pops an object from the top of the cleanup stack.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function checks that the specified item is at the top 
   of the cleanup stack before attempting to pop it; an E32USER-CBase 90 panic 
   is raised if the check fails.

2  In a release build, the function just pops the object which is at the top 
   of the cleanup stack; no checking is done.

@param aExpectedItem A pointer to the item expected to be at the top of the 
                     cleanup stack. In a release build, this parameter
					 is not used.
*/
	{ CleanupStack::Pop(); }




inline void CleanupStack::Pop(TInt aCount, TAny*)
/**
Pops the specified number of objects from the top of the cleanup stack.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function pops (aCount-1) items from the cleanup stack, 
   and then checks that the specified item is the next one on the cleanup stack 
   before attempting to pop it; an E32USER-CBase 90 panic is raised if the
   check fails.

2. In a release build, the function just pops aCount items from the cleanup stack; 
   no checking is done.

@param aCount            The number of items top be popped from
                         the cleanup stack.
@param aLastExpectedItem A pointer to the item expected to be at the top of 
                         the cleanup stack, after (aCount-1) items have been
						 popped. In a release build, this parameter is
						 not used.
*/
	{ CleanupStack::Pop(aCount); }




inline void CleanupStack::PopAndDestroy(TAny*)
/**
Pops an object from the top of the cleanup stack, and cleans it up.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function checks that the specified item is at the top 
   of the cleanup stack before attempting to pop and clean it up;
   an E32USER-CBase 90 panic is raised if the check fails.

2. In a release build, the function just pops and cleans up the object at 
   the top of the cleanup stack; no checking is done.

@param aExpectedItem A pointer to the item expected to be at the top of the 
                     cleanup stack. In a release build, this parameter is
					 not used.
*/
	{ CleanupStack::PopAndDestroy(); }




inline void CleanupStack::PopAndDestroy(TInt aCount, TAny*)
/**
Pops the specified number of objects from the top of the cleanup stack, and 
cleans them up.

The function has two modes of operation, depending on whether it is part of 
a debug build or a release build.

1. In a debug build, the function pops and cleans up (aCount-1) items from the 
   cleanup stack, and then checks that the specified item is the next one on 
   the cleanup stack before attempting to pop it and clean it up;
   an E32USER-CBase  90 panic is raised if the check fails.

2. In a release build, the function just pops and cleans up aCount items from 
   the cleanup stack; no checking is done.

@param aCount            The number of items top be popped from the
                         cleanup stack, and cleaned up.
@param aLastExpectedItem A pointer to the item expected to be at the top of 
                         the cleanup stack, after (aCount-1) items have been
						 popped and cleaned up. In a release build, this 
						 parameter is not used.
*/
	{ CleanupStack::PopAndDestroy(aCount); }
#endif




// Class TCleanupItem
inline TCleanupItem::TCleanupItem(TCleanupOperation anOperation)
	: iOperation(anOperation)
/**
Constructs the object with a cleanup operation.

@param anOperation  A cleanup operation which will be invoked by the pop and
                    destroy action resulting from a subsequent call to
					CleanupStack::PopAndDestroy().
*/
	{}




inline TCleanupItem::TCleanupItem(TCleanupOperation anOperation,TAny *aPtr)
	: iOperation(anOperation), iPtr(aPtr)
/**
Constructs the object with a cleanup operation and a pointer to the object
to be cleaned up.

@param anOperation A cleanup operation which will be invoked by the pop
                   and destroy action resulting from a subsequent call to
				   CleanupStack::PopAndDestroy().

@param aPtr        A pointer to an object which is the target of the
                   cleanup operation.
*/
	{}




// Class TCleanupTrapHandler
inline CCleanup &TCleanupTrapHandler::Cleanup()
	{return(*iCleanup);}

// Class TAutoClose
template <class T>
inline TAutoClose<T>::~TAutoClose()
/**
Destructor.

The implementation calls Close() on iObj, the instance of the template class.
*/
#ifdef __LEAVE_EQUALS_THROW__
	{if (!std::uncaught_exception()) iObj.Close();}
#else
	{iObj.Close();}
#endif




template <class T>
inline void TAutoClose<T>::PushL()
/**
Pushes a cleanup item onto the cleanup stack, so that Close() is called on the 
templated class object, iObj, if a leave occurs.
*/
	{CleanupStack::PushL(TCleanupItem(Close, (TAny *)&iObj));}




template <class T>
inline void TAutoClose<T>::Pop()
/**
Pops a single cleanup item from the cleanup stack.
*/
	{CleanupStack::Pop();}




template <class T>
void TAutoClose<T>::Close(TAny *aObj)
	{((T *)aObj)->Close();}




// Template class CleanupDelete
template <class T>
inline void CleanupDelete<T>::PushL(T* aPtr)
/**
Creates a TCleanupItem for the specified object.

The cleanup operation is the private static function Delete() of this class, which
deletes the specified object.

@param aPtr The object for which a TCleanupItem is to be constructed.
*/
	{CleanupStack::PushL(TCleanupItem(&Delete,aPtr));}




template <class T>
void CleanupDelete<T>::Delete(TAny *aPtr)
/**
The cleanup operation to be performed.

@param aPtr A pointer to the object for which clean up is to be performed. 
            The implementation deletes this object.
*/
	{delete STATIC_CAST(T*,aPtr);}




// See header file e32base.h for in-source comment.
template <class T>
inline void CleanupDeletePushL(T* aPtr)
	{CleanupDelete<T>::PushL(aPtr);}




// Template class CleanupArrayDelete
template <class T>
inline void CleanupArrayDelete<T>::PushL(T* aPtr)
/**
Creates a TCleanupItem for the specified array.

The cleanup operation is the private static function ArrayDelete() of
this class, which deletes the specified array.

@param aPtr The array of class T type objects for which a TCleanupItem is
            to be constructed.
*/
	{CleanupStack::PushL(TCleanupItem(&ArrayDelete,aPtr));}




template <class T>
void CleanupArrayDelete<T>::ArrayDelete(TAny *aPtr)
/**
The cleanup operation to be performed.

@param aPtr A pointer to the array for which clean up is to be performed. 
            The implementation deletes this array.
*/
	{delete [] STATIC_CAST(T*,aPtr);}




// See header file e32base.h for in-source comment.
template <class T>
inline void CleanupArrayDeletePushL(T* aPtr)
	{CleanupArrayDelete<T>::PushL(aPtr);}




// Template class CleanupClose
template <class T>
inline void CleanupClose<T>::PushL(T& aRef)
/**
Creates a TCleanupItem for the specified object.

The cleanup operation is the private static function Close() of this class.

@param aRef The object for which a TCleanupItem is to be constructed.
*/
	{CleanupStack::PushL(TCleanupItem(&Close,&aRef));}




template <class T>
void CleanupClose<T>::Close(TAny *aPtr)
/**
The cleanup operation to be performed.

@param aPtr A pointer to the object for which clean up is to be performed. 
            The implementation calls Close() on this object.
*/
	{(STATIC_CAST(T*,aPtr))->Close();}




// See header file e32base.h for in-source comment.
template <class T>
inline void CleanupClosePushL(T& aRef)
	{CleanupClose<T>::PushL(aRef);}




// Template class CleanupRelease
template <class T>
inline void CleanupRelease<T>::PushL(T& aRef)
/**
Creates a TCleanupItem for the specified object.

The cleanup operation is the private static function Release() of this class.

@param aRef The object for which a TCleanupItem is to be constructed.
*/
	{CleanupStack::PushL(TCleanupItem(&Release,&aRef));}




template <class T>
void CleanupRelease<T>::Release(TAny *aPtr)
/**
The cleanup operation to be performed.

@param aPtr A pointer to the object for which clean up is to be performed. 
            The implementation calls Release() on this object.
*/
	{(STATIC_CAST(T*,aPtr))->Release();}




// See header file e32base.h for in-source comment.
template <class T>
inline void CleanupReleasePushL(T& aRef)
	{CleanupRelease<T>::PushL(aRef);}



