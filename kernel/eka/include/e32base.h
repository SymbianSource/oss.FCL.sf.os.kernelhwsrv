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
// e32\include\e32base.h
// 
//

#ifndef __E32BASE_H__
#define __E32BASE_H__
#include <e32std.h>

/**
 * Container Base Class
 */
class CBase
/**
@publishedAll
@released

Base class for all classes to be instantiated on the heap.

By convention, all classes derived from CBase have a name beginning with the 
letter 'C'.

The class has two important features:

1. A virtual destructor that allows instances of derived classes to be destroyed 
   and properly cleaned up through a CBase* pointer. All CBase derived objects 
   can be pushed, as CBase* pointers, onto the cleanup stack, and destroyed through 
   a call to CleanupStack::PopAndDestroy().

2. Initialisation of the CBase derived object to binary zeroes through a specific 
   CBase::operator new() - this means that members, whose initial value should 
   be zero, do not have to be initialised in the constructor. This allows safe 
   destruction of a partially-constructed object.

Note that using C++ arrays of CBase-derived types is not recommended, as objects 
in the array will not be zero-initialised (as there is no operator new[] member). 
You should use an array class such as RPointerArray instead for arrays of 
CBase-derived types.

@see CleanupStack
*/
	{
public:
	/**
	Default constructor
	*/
	inline CBase()	{}
	IMPORT_C virtual ~CBase();
	inline TAny* operator new(TUint aSize, TAny* aBase) __NO_THROW;
	inline TAny* operator new(TUint aSize) __NO_THROW;
	inline TAny* operator new(TUint aSize, TLeave);
	inline TAny* operator new(TUint aSize, TUint aExtraSize) __NO_THROW;
	inline TAny* operator new(TUint aSize, TLeave, TUint aExtraSize);
	IMPORT_C static void Delete(CBase* aPtr);
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
private:
	CBase(const CBase&);
	CBase& operator=(const CBase&);
private:
	};
	
	
	
	
class CBufBase : public CBase
/**
@publishedAll
@released

Defines the interface for dynamic buffers. 

The basic functions, InsertL(), Read(), Write(), Delete(), Reset() and Size(), 
transfer data between the buffer and other places, and allow that data to 
be deleted

The ExpandL() and Resize() functions allow some operations to be carried out 
with greater efficiency

A Compress() function frees (back to the heap) any space which may have been 
allocated, but not used

Ptr() and BackPtr() allow look-up of contiguous data from any given position, 
forward or backward
*/
	{
public:
	IMPORT_C ~CBufBase();
	inline TInt Size() const;
	IMPORT_C void Reset();
	IMPORT_C void Read(TInt aPos,TDes8& aDes) const;
	IMPORT_C void Read(TInt aPos,TDes8& aDes,TInt aLength) const;
	IMPORT_C void Read(TInt aPos,TAny* aPtr,TInt aLength) const;
	IMPORT_C void Write(TInt aPos,const TDesC8& aDes);
	IMPORT_C void Write(TInt aPos,const TDesC8& aDes,TInt aLength);
	IMPORT_C void Write(TInt aPos,const TAny* aPtr,TInt aLength);
	IMPORT_C void InsertL(TInt aPos,const TDesC8& aDes);
	IMPORT_C void InsertL(TInt aPos,const TDesC8& aDes,TInt aLength);
	IMPORT_C void InsertL(TInt aPos,const TAny* aPtr,TInt aLength);
	IMPORT_C void ExpandL(TInt aPos,TInt aLength);
	IMPORT_C void ResizeL(TInt aSize);
// Pure virtual
	/**
	Compresses the buffer so as to occupy minimal space.
	
	Normally, you would call this when a buffer has reached its final size,
	or when you know it will not expand again for a while, or when an
	out-of-memory error has occurred and your program is taking measures to
	save space. Compression in these circumstances releases memory for other
	programs to use, but has no adverse effect on performance.
	
	Derived classes provide the implementation.
	
	@see CBufFlat::Compress
	@see CBufSeg::Compress
	*/
    virtual void Compress()=0;
	/**
	Deletes data from the buffer.
	
	Derived classes provide the implementation.
	
	@param aPos    Buffer position where the deletion will begin; must be in the 
                   range zero to (Size() minus the length of the data
                   to be deleted). 
	@param aLength The number of bytes to be deleted; must be non-negative.
		
	@see CBufFlat::Delete
	@see CBufSeg::Delete
	*/
	virtual void Delete(TInt aPos,TInt aLength)=0;
	/**
	Gets a pointer descriptor to represent the data from the specified position to  
	the end of the contiguous region containing that byte.
	
	Derived classes provide the implementation.
		
	@param aPos Buffer position: must be in range zero to Size().
	 
	@return Descriptor representing the data starting at aPos, and whose length
	        indicates the number of contiguous bytes stored in the buffer, 
            forward from that point. The length will be non-zero unless aPos==Size().
              	
	@see CBufFlat::Ptr
	@see CBufSeg::Ptr
	*/
	virtual TPtr8 Ptr(TInt aPos)=0;
	/**
	Gets a pointer descriptor to represent data from just before the specified 
	data byte backward to the beginning of the contiguous region containing 
	that byte.
	
	Derived classes provide the implementation.
	
	@param aPos Buffer position: must be in range zero to Size().
	 
	@return Descriptor representing the back contiguous region. 
	        The address in the descriptor is the pointer to the bytes at the
	        buffer position, unless the buffer position was at the beginning of
	        a non-first segment in the buffer: in this case, the address is a
	        pointer just beyond the last data byte in the previous segment.
	        The length is the number of contiguous bytes from the address
	        backwards to the beginning of the segment.
		
	@see CBufFlat::BackPtr
	@see CBufSeg::BackPtr
	*/
	virtual TPtr8 BackPtr(TInt aPos)=0;
private:
	virtual void DoInsertL(TInt aPos,const TAny* aPtr,TInt aLength)=0;
protected:
	IMPORT_C CBufBase(TInt anExpandSize);
protected:
	TInt iSize;
	TInt iExpandSize;
	};




class CBufFlat : public CBufBase
/**
@publishedAll
@released

Provides a flat storage dynamic buffer.

This class should be used when high-speed pointer lookup is an important
consideration, and you are reasonably confident that the insertion of
data will not fail. 

This class is an implementation of the abstract buffer interface provided 
by CBufBase and uses a single heap cell to contain the data.
*/
	{
public:
	IMPORT_C ~CBufFlat();
	IMPORT_C static CBufFlat* NewL(TInt anExpandSize);
	inline TInt Capacity() const;
	IMPORT_C void SetReserveL(TInt aSize);
	IMPORT_C void Compress();
	IMPORT_C void Delete(TInt aPos,TInt aLength);
	IMPORT_C TPtr8 Ptr(TInt aPos);
	IMPORT_C TPtr8 BackPtr(TInt aPos);
protected:
	IMPORT_C CBufFlat(TInt anExpandSize);
private:
	IMPORT_C void DoInsertL(TInt aPos,const TAny* aPtr,TInt aLength);
private:
	TInt iMaxSize;
	TUint8* iPtr;
	};




class TBufSegLink;
class CBufSeg : public CBufBase
/**
@publishedAll
@released

Provides a segmented dynamic buffer.

This class should be used when the object has a long life-time and an
unpredictable number of insertions, or there is concern about the performance
of insertion and deletion operations into large buffers.

This class is an implementation of the abstract buffer interface provided 
by CBufBase and uses doubly-linked list of heap cells to contain the data; 
each cell containing a segment of the buffer.

Its (private) data members include an anchor for the doubly-linked list, and also a 
reference to the buffer position used by the last operation. This reference 
acts as a cache; if the next operation uses a similar buffer position, then 
calculation of the pointer corresponding to its buffer position is much faster.
*/
	{
public:
	IMPORT_C ~CBufSeg();
	IMPORT_C static CBufSeg* NewL(TInt anExpandSize);
    IMPORT_C void Compress();
	IMPORT_C void Delete(TInt aPos,TInt aLength);
	IMPORT_C TPtr8 Ptr(TInt aPos);
	IMPORT_C TPtr8 BackPtr(TInt aPos);
protected:
	IMPORT_C CBufSeg(TInt anExpandSize);
	void InsertIntoSegment(TBufSegLink* aSeg,TInt anOffset,const TAny* aPtr,TInt aLength);
	void DeleteFromSegment(TBufSegLink* aSeg,TInt anOffset,TInt aLength);
	void FreeSegment(TBufSegLink* aSeg);
    void SetSBO(TInt aPos);
	void AllocSegL(TBufSegLink* aSeg,TInt aNumber);
private:
	IMPORT_C void DoInsertL(TInt aPos,const TAny* aPtr,TInt aLength);
private:
    TDblQue<TBufSegLink> iQue;
	TBufSegLink* iSeg;
	TInt iBase;
	TInt iOffset;
	};




class TKeyArrayFix : public TKey
/**
@publishedAll
@released

Defines the characteristics of a key used to access the elements of arrays 
of fixed length objects.

An object of this type can represent three categories of key, depending on 
the constructor used:

1. a descriptor key 

2. a text key

3. a numeric key.

The Sort(), InsertIsqL(), Find() and FindIsqL() member functions of the CArrayFixFlat 
and CArrayFixSeg class hierarchies need a TKeyArrayFix object as an argument 
to define the location and type of key within an array element.

@see CArrayFixFlat
@see CArrayFixSeg
*/
	{
public:
	IMPORT_C TKeyArrayFix(TInt anOffset,TKeyCmpText aType);
	IMPORT_C TKeyArrayFix(TInt anOffset,TKeyCmpText aType,TInt aLength);
	IMPORT_C TKeyArrayFix(TInt anOffset,TKeyCmpNumeric aType);
protected:
	IMPORT_C virtual void Set(CBufBase* aBase,TInt aRecordLength);
	IMPORT_C TAny* At(TInt anIndex) const;
protected:
	TInt iRecordLength;
	CBufBase* iBase;
	friend class CArrayFixBase;
	};




typedef CBufBase*(*TBufRep)(TInt anExpandSize);
class CArrayFixBase : public CBase
/**
@publishedAll
@released

Base class for arrays of fixed length objects.

It provides implementation and public functions which are common to all arrays
of this type.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	IMPORT_C ~CArrayFixBase();
	inline TInt Count() const;
	inline TInt Length() const;
	IMPORT_C void Compress();
	IMPORT_C void Reset();
	IMPORT_C TInt Sort(TKeyArrayFix& aKey);
	IMPORT_C TAny* At(TInt anIndex) const;
	IMPORT_C TAny* End(TInt anIndex) const;
	IMPORT_C TAny* Back(TInt anIndex) const;
	IMPORT_C void Delete(TInt anIndex);
	IMPORT_C void Delete(TInt anIndex,TInt aCount);
	IMPORT_C TAny* ExpandL(TInt anIndex);
	IMPORT_C TInt Find(const TAny* aPtr,TKeyArrayFix& aKey,TInt& anIndex) const;
	IMPORT_C TInt FindIsq(const TAny* aPtr,TKeyArrayFix& aKey,TInt& anIndex) const;
	IMPORT_C void InsertL(TInt anIndex,const TAny* aPtr);
	IMPORT_C void InsertL(TInt anIndex,const TAny* aPtr,TInt aCount);
	IMPORT_C TInt InsertIsqL(const TAny* aPtr,TKeyArrayFix& aKey);
	IMPORT_C TInt InsertIsqAllowDuplicatesL(const TAny* aPtr,TKeyArrayFix& aKey);
	IMPORT_C void ResizeL(TInt aCount,const TAny* aPtr);
protected:
	IMPORT_C CArrayFixBase(TBufRep aRep,TInt aRecordLength,TInt aGranularity);
	IMPORT_C void InsertRepL(TInt anIndex,const TAny* aPtr,TInt aReplicas);
	IMPORT_C void SetKey(TKeyArrayFix& aKey) const;
	IMPORT_C void SetReserveFlatL(TInt aCount);
	IMPORT_C static TInt CountR(const CBase* aPtr);
	IMPORT_C static const TAny* AtR(const CBase* aPtr,TInt anIndex);
private:
	TInt iCount;
	TInt iGranularity;
	TInt iLength;
	TBufRep iCreateRep;
	CBufBase* iBase;
	};




template <class T>
class CArrayFix : public CArrayFixBase
/**
@publishedAll
@released

A thin templated base class for arrays of fixed length objects. 

The public functions provide standard array behaviour.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	inline CArrayFix(TBufRep aRep,TInt aGranularity);
	inline const T& operator[](TInt anIndex) const;
	inline T& operator[](TInt anIndex);
	inline const T& At(TInt anIndex) const;
	inline const T* End(TInt anIndex) const;
	inline const T* Back(TInt anIndex) const;
	inline T& At(TInt anIndex);
	inline T* End(TInt anIndex);
	inline T* Back(TInt anIndex);
	inline void AppendL(const T& aRef);
	inline void AppendL(const T* aPtr,TInt aCount);
	inline void AppendL(const T& aRef,TInt aReplicas);
	inline T& ExpandL(TInt anIndex);
	inline T& ExtendL();
	inline TInt Find(const T& aRef,TKeyArrayFix& aKey,TInt& anIndex) const;
	inline TInt FindIsq(const T& aRef,TKeyArrayFix& aKey,TInt& anIndex) const;
	inline void InsertL(TInt anIndex,const T& aRef);
	inline void InsertL(TInt anIndex,const T* aPtr,TInt aCount);
	inline void InsertL(TInt anIndex,const T& aRef,TInt aReplicas);
	inline TInt InsertIsqL(const T& aRef,TKeyArrayFix& aKey);
	inline TInt InsertIsqAllowDuplicatesL(const T& aRef,TKeyArrayFix& aKey);
	inline void ResizeL(TInt aCount);
	inline void ResizeL(TInt aCount,const T& aRef);
	inline const TArray<T> Array() const;
	};




TEMPLATE_SPECIALIZATION class CArrayFix<TAny> : public CArrayFixBase
/**
@publishedAll
@released

A template specialisation base class for arrays of fixed length
untyped objects.

The public functions provide standard array behaviour.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	inline CArrayFix(TBufRep aRep,TInt aRecordLength,TInt aGranularity);
	inline const TAny* At(TInt anIndex) const;
	inline const TAny* End(TInt anIndex) const;
	inline const TAny* Back(TInt anIndex) const;
	inline TAny* At(TInt anIndex);
	inline TAny* End(TInt anIndex);
	inline TAny* Back(TInt anIndex);
	inline void AppendL(const TAny* aPtr);
	inline void AppendL(const TAny* aPtr,TInt aCount);
	inline TAny* ExtendL();
	};





template <class T>
class CArrayFixFlat : public CArrayFix<T>
/**
@publishedAll
@released

Array of fixed length objects contained within a flat dynamic buffer.

The elements of the array are instances of the template class T.

The flat dynamic buffer is an instance of a CBufFlat.

The elements can be T or R type objects and must have an accessible default 
constructor.

Note that, where possible, use the RArray<class T> class as this is more
efficient.

@see CBufFlat
@see RArray
*/
	{
public:
	inline explicit CArrayFixFlat(TInt aGranularity);
	inline void SetReserveL(TInt aCount);
	};




TEMPLATE_SPECIALIZATION class CArrayFixFlat<TAny> : public CArrayFix<TAny>
/**
@publishedAll
@released

An array of fixed length untyped objects using a flat dynamic buffer.

The array elements are contained within a CBufFlat.

The class is useful for constructing an array of fixed length buffers, where 
the length is decided at run time.

This class is also useful as a data member of a base class in a thin template 
class/base class pair where the type of the array element is not known until 
the owning thin template class is instantiated.

@see CBufFlat
*/
	{
public:
	inline CArrayFixFlat(TInt aRecordLength,TInt aGranularity);
	inline void SetReserveL(TInt aCount);
	};




TEMPLATE_SPECIALIZATION class CArrayFixFlat<TInt> : public CArrayFix<TInt>
/**
@publishedAll
@released

Template specialisation base class for arrays of TInt types implemented in a 
flat dynamic buffer.

@see TInt 
*/
	{
public:
	IMPORT_C explicit CArrayFixFlat(TInt aGranularity);
	IMPORT_C ~CArrayFixFlat();
	inline void SetReserveL(TInt aCount);
	};




TEMPLATE_SPECIALIZATION class CArrayFixFlat<TUid> : public CArrayFix<TUid>
/**
@publishedAll
@released

Template specialisation base class for arrays of TUid types implemented in a 
flat dynamic buffer.

@see TUid 
*/
	{
public:
	IMPORT_C explicit CArrayFixFlat(TInt aGranularity);
	IMPORT_C ~CArrayFixFlat();
	inline void SetReserveL(TInt aCount);
	};




template <class T>
class CArrayFixSeg : public CArrayFix<T>
/**
@publishedAll
@released

Array of fixed length objects contained within a segmented buffer.

The elements of the array are instances of the template class T.

The segmented buffer is an instance of a CBufSeg.

The elements can be T or R type objects and must have an accessible default 
constructor.

@see CBufSeg
*/
	{
public:
	inline explicit CArrayFixSeg(TInt aGranularity);
	};




TEMPLATE_SPECIALIZATION class CArrayFixSeg<TAny> : public CArrayFix<TAny>
/**
@publishedAll
@released

An array of fixed length untyped objects using a segmented dynamic buffer.
 
The array elements are contained within a CBufSeg.

The class is useful for constructing an array of fixed length buffers, where 
the length is decided at run time.

This class is also useful as a data member of a base class in a thin template 
class/base class pair where the type of the array element is not known until 
the owning thin template class is instantiated.

@see CBufSeg
*/
	{
public:
	inline CArrayFixSeg(TInt aRecordLength,TInt aGranularity);
	};




template <class T>
class CArrayPtr : public CArrayFix<T*>
/**
@publishedAll
@released

A thin templated base class for arrays of pointers to objects.

The public functions contribute to standard array behaviour.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	inline CArrayPtr(TBufRep aRep,TInt aGranularity);
    void ResetAndDestroy();
	};





template <class T>
class CArrayPtrFlat : public CArrayPtr<T>
/**
@publishedAll
@released

Array of pointers to objects implemented using a flat dynamic buffer.

The elements of the array are pointers to instances of the template class T
and are contained within a CBufFlat.

This type of array has the full behaviour of flat arrays but, in addition, 
the CArrayPtr<class T>::ResetAndDestroy() function offers a way of destroying 
all of the objects whose pointers form the elements of the array, before
resetting the array.

Note that where possible, use the RPointerArray<class T> class as this is
more efficient.

@see CBufFlat
@see CArrayPtr::ResetAndDestroy
@see RPointerArray
*/
	{
public:
	inline explicit CArrayPtrFlat(TInt aGranularity);
	inline void SetReserveL(TInt aCount);
	};




template <class T>
class CArrayPtrSeg : public CArrayPtr<T>
/**
@publishedAll
@released

Array of pointers to objects implemented using a segmented dynamic buffer. 

The elements of the array are pointers to instances of the template class T
and are contained within a CBufSeg.

This type of array has the full behaviour of segmented arrays but, in addition, 
the CArrayPtr<class T>::ResetAndDestroy() function offers a way of destroying 
all of the objects whose pointers form the elements of the array before
resetting the array.

@see CBufSeg
@see CArrayPtr::ResetAndDestroy
*/
	{
public:
	inline explicit CArrayPtrSeg(TInt aGranularity);
	};




class TKeyArrayVar : public TKey
/**
@publishedAll
@released

Defines the characteristics of a key used to access the elements of arrays 
of variable length objects.

An object of this type can represent three categories of key, depending on 
the constructor used:

1. a descriptor key 

2. a text key

3. a numeric key.

The Sort(), InsertIsqL(), Find() and FindIsqL() member functions of the CArrayVarFlat 
and CArrayVarSeg class hierarchies need a TKeyArrayVar object as an argument 
to define the location and type of key within an array element.

A TKeyArrayVar object is also required for sorting a packed array. The implementation 
of the SortL() member function of the CArrayPakFlat class constructs a temporary 
CArrayVarFlat object which requires the TKeyArrayVar object.

@see CArrayVarFlat
@see CArrayVarSeg
@see CArrayPakFlat
*/
	{
public:
	IMPORT_C TKeyArrayVar(TInt anOffset,TKeyCmpText aType);
	IMPORT_C TKeyArrayVar(TInt anOffset,TKeyCmpText aType,TInt aLength);
	IMPORT_C TKeyArrayVar(TInt anOffset,TKeyCmpNumeric aType);
protected:
	IMPORT_C virtual void Set(CBufBase* aBase);
	IMPORT_C TAny* At(TInt anIndex) const;
protected:
	CBufBase* iBase;
	friend class CArrayVarBase;
	};





class CArrayVarBase : public CBase
/**
@publishedAll
@released

An implementation base class for variable length arrays. 

It provides implementation and public functions which are common to all
variable length type arrays.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	IMPORT_C ~CArrayVarBase();
	inline TInt Count() const;
	IMPORT_C TInt Length(TInt anIndex) const;
	IMPORT_C void Compress();
	IMPORT_C void Reset();
	IMPORT_C TInt Sort(TKeyArrayVar& aKey);
	IMPORT_C TAny* At(TInt anIndex) const;
	IMPORT_C void Delete(TInt anIndex);
	IMPORT_C void Delete(TInt anIndex,TInt aCount);
	IMPORT_C TAny* ExpandL(TInt anIndex,TInt aLength);
	IMPORT_C TInt Find(const TAny* aPtr,TKeyArrayVar& aKey,TInt& anIndex) const;
	IMPORT_C TInt FindIsq(const TAny* aPtr,TKeyArrayVar& aKey,TInt& anIndex) const;
	IMPORT_C void InsertL(TInt anIndex,const TAny* aPtr,TInt aLength);
	IMPORT_C TInt InsertIsqL(const TAny* aPtr,TInt aLength,TKeyArrayVar& aKey);
	IMPORT_C TInt InsertIsqAllowDuplicatesL(const TAny* aPtr,TInt aLength,TKeyArrayVar& aKey);
protected:
	IMPORT_C CArrayVarBase(TBufRep aRep,TInt aGranularity);
	IMPORT_C void SetKey(TKeyArrayVar& aKey) const;
	IMPORT_C static TInt CountR(const CBase* aPtr);
	IMPORT_C static const TAny* AtR(const CBase* aPtr,TInt anIndex);
private:
	TInt iCount;
	TInt iGranularity;
	TBufRep iCreateRep;
	CBufBase* iBase;
	};




template <class T>
class CArrayVar : public CArrayVarBase
/**
@publishedAll
@released

A thin templated base class for variable length arrays.

The public functions provide standard array behaviour.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	inline CArrayVar(TBufRep aRep,TInt aGranularity);
	inline const T& operator[](TInt anIndex) const;
	inline T& operator[](TInt anIndex);
	inline const T& At(TInt anIndex) const;
	inline T& At(TInt anIndex);
	inline void AppendL(const T& aRef,TInt aLength);
	inline T& ExpandL(TInt anIndex,TInt aLength);
	inline T& ExtendL(TInt aLength);
	inline TInt Find(const T& aRef,TKeyArrayVar& aKey,TInt& anIndex) const;
	inline TInt FindIsq(const T& aRef,TKeyArrayVar& aKey,TInt& anIndex) const;
	inline void InsertL(TInt anIndex,const T& aRef,TInt aLength);
	inline TInt InsertIsqL(const T& aRef,TInt aLength,TKeyArrayVar& aKey);
 	inline TInt InsertIsqAllowDuplicatesL(const T& aRef,TInt aLength,TKeyArrayVar& aKey);
	inline const TArray<T> Array() const;
	};




TEMPLATE_SPECIALIZATION class CArrayVar<TAny> : public CArrayVarBase
/**
@publishedAll
@released

A template specialisation base class for variable length arrays.

The array buffer organisation is defined at construction.

The class is useful for constructing an array of variable length buffers, 
where the length is decided at run time.

This class is also useful as a data member of a base class in a thin template 
class/base class pair, where the type of the array element is not known until 
the owning thin template class is instantiated.
*/
	{
public:
	inline CArrayVar(TBufRep aRep,TInt aGranularity);
	inline const TAny* At(TInt anIndex) const;
	inline TAny* At(TInt anIndex);
	inline void AppendL(const TAny* aPtr,TInt aLength);
	inline TAny* ExtendL(TInt aLength);
	};




template <class T>
class CArrayVarFlat : public CArrayVar<T>
/**
@publishedAll
@released

Array of variable length objects implemented using a flat dynamic buffer.

The elements of the array are instances of the template class T and are
contained within their own heap cells. Pointers to the elements are maintained
within the flat dynamic buffer, a CBufFlat.

The elements can be T or R type objects and must have an accessible default 
constructor. 

@see CBufFlat
*/
	{
public:
	inline explicit CArrayVarFlat(TInt aGranularity);
	};




template <class T>
class CArrayVarSeg : public CArrayVar<T>
/**
@publishedAll
@released

Array of variable length objects implemented using a segmented dynamic buffer. 

The elements of the array are instances of the template class T and are
contained within their own heap cells. Pointers to the elements are maintained
within a segmented dynamic buffer, a CBufSeg.

The elements can be T or R type objects and must have an accessible default 
constructor.

@see CBufSeg
*/
	{
public:
	inline explicit CArrayVarSeg(TInt aGranularity);
	};




class TKeyArrayPak : public TKeyArrayVar
/**
@publishedAll
@released

Defines the characteristics of a key used to access the elements of packed 
arrays.

An object of this type can represent three categories of key, depending on 
the constructor used:

1. a descriptor key 

2. a text key

3. a numeric key.

The InsertIsqL(), Find() and FindIsqL() member functions of the CArrayPakFlat 
class hierarchy need a TKeyArrayPak object as an argument to define the location 
and type of key within an array element.

Note that a TKeyArrayVar object is required for sorting a packed array. The 
implementation of the SortL() member function of the CArrayPakFlat class constructs 
a temporary CArrayVarFlat object which requires the TKeyArrayVar object.

@see CArrayVarSeg
@see CArrayPakFlat
@see TKeyArrayVar
*/
	{
public:
	IMPORT_C TKeyArrayPak(TInt anOffset,TKeyCmpText aType);
	IMPORT_C TKeyArrayPak(TInt anOffset,TKeyCmpText aType,TInt aLength);
	IMPORT_C TKeyArrayPak(TInt anOffset,TKeyCmpNumeric aType);
protected:
	IMPORT_C virtual void Set(CBufBase* aBase);
	IMPORT_C TAny* At(TInt anIndex) const;
private:
	TInt iCacheIndex;
	TInt iCacheOffset;
	friend class CArrayPakBase;
	};




class CArrayPakBase : public CBase
/**
@publishedAll
@released

An implementation base class for all variable length, packed arrays.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	IMPORT_C ~CArrayPakBase();
	inline TInt Count() const;
	IMPORT_C TInt Length(TInt anIndex) const;
	IMPORT_C void Compress();
	IMPORT_C void Reset();
	IMPORT_C void SortL(TKeyArrayVar& aKey);
	IMPORT_C TAny* At(TInt anIndex) const;
	IMPORT_C void Delete(TInt anIndex);
	IMPORT_C void Delete(TInt anIndex,TInt aCount);
	IMPORT_C TAny* ExpandL(TInt anIndex,TInt aLength);
	IMPORT_C TInt Find(const TAny* aPtr,TKeyArrayPak& aKey,TInt& anIndex) const;
	IMPORT_C TInt FindIsq(const TAny* aPtr,TKeyArrayPak& aKey,TInt& anIndex) const;
	IMPORT_C void InsertL(TInt anIndex,const TAny* aPtr,TInt aLength);
	IMPORT_C TInt InsertIsqL(const TAny* aPtr,TInt aLength,TKeyArrayPak& aKey);
	IMPORT_C TInt InsertIsqAllowDuplicatesL(const TAny* aPtr,TInt aLength,TKeyArrayPak& aKey);
protected:
	IMPORT_C CArrayPakBase(TBufRep aRep,TInt aGranularity);
	IMPORT_C void SetKey(TKeyArrayPak& aKey) const;
	IMPORT_C TInt GetOffset(TInt anIndex) const;
	IMPORT_C void BuildVarArrayL(CArrayVarFlat<TAny>*& aVarFlat);
	IMPORT_C static TInt CountR(const CBase* aPtr);
	IMPORT_C static const TAny* AtR(const CBase* aPtr,TInt anIndex);
private:
	TInt iCount;
	TInt iGranularity;
	TBufRep iCreateRep;
	CBufBase* iBase;
	TInt iCacheIndex;
	TInt iCacheOffset;
	};




template <class T>
class CArrayPak : public CArrayPakBase
/**
@publishedAll
@released

A thin templated base class for variable length, packed, arrays.

The public functions provide standard array behaviour.

The class is always derived from and is never instantiated explicitly.
*/
	{
public:
	inline CArrayPak(TBufRep aRep,TInt aGranularity);
	inline const T& operator[](TInt anIndex) const;
	inline T& operator[](TInt anIndex);
	inline const T& At(TInt anIndex) const;
	inline T& At(TInt anIndex);
	inline void AppendL(const T& aRef,TInt aLength);
	inline T& ExpandL(TInt anIndex,TInt aLength);
	inline T& ExtendL(TInt aLength);
	inline TInt Find(const T& aRef,TKeyArrayPak& aKey,TInt& anIndex) const;
	inline TInt FindIsq(const T& aRef,TKeyArrayPak& aKey,TInt& anIndex) const;
	inline void InsertL(TInt anIndex,const T& aRef,TInt aLength);
	inline TInt InsertIsqL(const T& aRef,TInt aLength,TKeyArrayPak& aKey);
	inline TInt InsertIsqAllowDuplicatesL(const T& aRef,TInt aLength,TKeyArrayPak& aKey);
	inline const TArray<T> Array() const;
	};




TEMPLATE_SPECIALIZATION class CArrayPak<TAny> : public CArrayPakBase
/**
@publishedAll
@released

A template specialisation base class for variable length, packed, arrays.

The array buffer organisation is defined at construction.

The class is useful for constructing an array of variable length buffers, 
where the length is decided at run time.

This class is also useful as a data member of a base class in a thin template 
class/base class pair where the type of the array element is not known until 
the owning thin template class is instantiated.
*/
	{
public:
	inline CArrayPak(TBufRep aRep,TInt aGranularity);
	inline const TAny* At(TInt anIndex) const;
	inline TAny* At(TInt anIndex);
	inline void AppendL(const TAny* aPtr,TInt aLength);
	inline TAny* ExtendL(TInt aLength);
	};




template <class T>
class CArrayPakFlat : public CArrayPak<T>
/**
@publishedAll
@released

Array of variable length objects packed into a flat buffer. 

The elements of the array are instances of the template class T and are
contained within a flat dynamic buffer, a CBufFlat.

The elements can be T or R type objects and must have an accessible default 
constructor.

@see CBufFlat
*/
	{
public:
	inline explicit CArrayPakFlat(TInt aGranularity);
	};




class CObjectCon;
class CObject : public CBase
/**
@publishedAll
@released

Implements reference counting to track concurrent references to itself.

An object of this type arranges automatic destruction of itself when the final 
reference is removed.

A reference counting object is any object which has CObject as its base class. 
Constructing a CObject derived type or calling its Open() member function 
adds a reference to that object by adding one to the reference count; calling 
its Close() member function removes a reference by subtracting one from the 
reference count; when the last user of the object calls Close(), the reference 
count becomes zero and the object is automatically destroyed.
*/
	{
public:
	IMPORT_C CObject();
	IMPORT_C ~CObject();
	IMPORT_C virtual TInt Open();
	IMPORT_C virtual void Close();
	IMPORT_C virtual TName Name() const;
	IMPORT_C virtual TFullName FullName() const;
	IMPORT_C TInt SetName(const TDesC* aName);
	IMPORT_C void SetNameL(const TDesC* aName);
	inline CObject* Owner() const;
	inline void SetOwner(CObject* anOwner);
	inline TInt AccessCount() const;
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
protected:
	inline TInt UniqueID() const;
	inline void Inc();
	inline void Dec();
private:
	TInt iAccessCount;
	CObject* iOwner;
	CObjectCon* iContainer;
	HBufC* iName;
	TAny* iSpare1;
	TAny* iSpare2;
	friend class CObjectCon;
	friend class CObjectIx;
	__DECLARE_TEST;
	};

//Forward declaration of SObjectIxRec
struct SObjectIxRec;
	
class CObjectIx : public CBase
/**
@publishedAll
@released

Generates handle numbers for reference counting objects.

This is referred to as an object index.

Adding a reference counting object to an object index is the way in which 
a unique handle number can be generated for that object. A handle number is 
the way in which an object, which is owned or managed by another thread or 
process can be identified.

@see CObject
*/
	{
public:
	enum {
	     /**
	     When ORd into the handle number, indicates that the reference
	     counting object cannot be closed.
	     */
         ENoClose=KHandleNoClose,
         
         
         /**
         When ORed into the handle number, indicates that the handle
         is a local handle.
         */
         ELocalHandle=KHandleFlagLocal
         };
public:
	IMPORT_C static CObjectIx* NewL();
	IMPORT_C ~CObjectIx();
	IMPORT_C TInt AddL(CObject* anObj);
	IMPORT_C void Remove(TInt aHandle);
	IMPORT_C CObject* At(TInt aHandle,TInt aUniqueID);
	IMPORT_C CObject* At(TInt aHandle);
	IMPORT_C CObject* AtL(TInt aHandle,TInt aUniqueID);
	IMPORT_C CObject* AtL(TInt aHandle);
	IMPORT_C TInt At(const CObject* anObject) const;
	IMPORT_C TInt Count(CObject* anObject) const;
	IMPORT_C CObject* operator[](TInt anIndex);
	inline TInt Count() const;
	inline TInt ActiveCount() const;
protected:
	IMPORT_C CObjectIx();
private:
	void UpdateState();
private:
	TInt iNumEntries;		// Number of actual entries in the index
	TInt iHighWaterMark;	// points to at least 1 above the highest active index
	TInt iAllocated;		// Max entries before realloc needed
	TInt iNextInstance;
	SObjectIxRec *iObjects;
	TInt iFree;				// The index of the first free slot or -1.
	TInt iUpdateDisabled;   // If >0, disables HWM update, reorder of the free list and memory shrinking.
	TAny* iSpare1;
	TAny* iSpare2;
	};
//
inline TBool IsLocalHandle(TInt aHandle)
	{return(aHandle&CObjectIx::ELocalHandle);}
inline void SetLocalHandle(TInt &aHandle)
	{aHandle|=CObjectIx::ELocalHandle;}
inline void UnSetLocalHandle(TInt &aHandle)
	{aHandle&=(~CObjectIx::ELocalHandle);}




class CObjectCon : public CBase
/**
@publishedAll
@released

An object container.

An object container acts as a home for a set of related reference counting 
objects.

A reference counting object, a CObject type, must be added to an object
container. Only one instance of a given reference counting object can be
held by an object container, i.e. each object within an object container
must be distinct.

Object containers are constructed by an object container index, a CObjectConIx 
type. 

Note that this class is not intended for user derivation.

@see CObject
@see CObjectConIx
*/
	{
public:
	IMPORT_C static CObjectCon* NewL();
	IMPORT_C ~CObjectCon();
	IMPORT_C void Remove(CObject* anObj);
	IMPORT_C void AddL(CObject* anObj);
	IMPORT_C CObject* operator[](TInt anIndex);
	IMPORT_C CObject* At(TInt aFindHandle) const;
	IMPORT_C CObject* AtL(TInt aFindHandle) const;
	IMPORT_C TInt CheckUniqueFullName(const CObject* anOwner,const TDesC& aName) const;
	IMPORT_C TInt CheckUniqueFullName(const CObject* anObject) const;
	IMPORT_C TInt FindByName(TInt& aFindHandle,const TDesC& aMatch,TName& aName) const;
	IMPORT_C TInt FindByFullName(TInt& aFindHandle,const TDesC& aMatch,TFullName& aFullName) const;
	inline TInt UniqueID() const;
	inline TInt Count() const;
protected:
	IMPORT_C CObjectCon(TInt aUniqueID);
	TBool NamesMatch(const CObject* anObject, const CObject* aCurrentObject) const;
	TBool NamesMatch(const CObject* anObject, const TName& anObjectName, const CObject* aCurrentObject) const;
public:
    /**
    The object container's unique Id value.
    */
	TInt iUniqueID;
private:
	TInt iCount;
	TInt iAllocated;
	CObject** iObjects;
	TAny* iSpare1;
	TAny* iSpare2;
	friend class CObjectConIx;
	};




class CObjectConIx : public CBase
/**
@publishedAll
@released

A container for object containers

This is referred to as a container index.

The class provides the mechanism through which object containers, CObjectCon 
types, are created.

@see CObjectCon
@see CObject
*/
	{
#ifndef	SYMBIAN_ENABLE_SPLIT_HEADERS
protected:
    /**
    @internalComponent
    */
	enum {ENotOwnerID};
#endif
	
public:
	IMPORT_C static CObjectConIx* NewL();
	IMPORT_C ~CObjectConIx();
	IMPORT_C CObjectCon* Lookup(TInt aFindHandle) const;
	IMPORT_C CObjectCon* CreateL();
	IMPORT_C void Remove(CObjectCon* aCon);
protected:
	IMPORT_C CObjectConIx();
	IMPORT_C void CreateContainerL(CObjectCon*& anObject);
private:
	CObjectCon* LookupByUniqueId(TInt aUniqueId) const;
private:
	TInt iCount;
	TInt iAllocated;
	TUint16 iNextUniqueID;
	TUint16 iUniqueIDHasWrapped;
	CObjectCon** iContainers;
	TAny* iSpare1;
	TAny* iSpare2;
	};

// Forward Declaration of TCleanupStackItem
class TCleanupStackItem;




/**
@publishedAll
@released

Defines a function which takes a single argument of type TAny* and returns 
void.

An argument of this type is required by the constructors of a TCleanupItem 
object.
*/
typedef void (*TCleanupOperation)(TAny*);




class TCleanupItem
/**
@publishedAll
@released

Encapsulates a cleanup operation and an object on which the operation
is to be performed.

The class allows cleanup to be more sophisticated than simply deleting objects,
for example, releasing access to some shared resource.
*/
	{
public:
	inline TCleanupItem(TCleanupOperation anOperation);
	inline TCleanupItem(TCleanupOperation anOperation,TAny* aPtr);
private:
	TCleanupOperation iOperation;
	TAny* iPtr;
	friend class TCleanupStackItem;
	};




class CCleanup : public CBase
/**
@publishedAll
@released

Implements the cleanup stack.

An object of this type is created and used by the cleanup stack
interface, CTrapCleanup.
*/
	{
public:
	IMPORT_C static CCleanup* New();
	IMPORT_C static CCleanup* NewL();
	IMPORT_C ~CCleanup();
	IMPORT_C void NextLevel();
	IMPORT_C void PreviousLevel();
	IMPORT_C void PushL(TAny* aPtr);
	IMPORT_C void PushL(CBase* anObject);
	IMPORT_C void PushL(TCleanupItem anItem);
	IMPORT_C void Pop();
	IMPORT_C void Pop(TInt aCount);
	IMPORT_C void PopAll();
	IMPORT_C void PopAndDestroy();
	IMPORT_C void PopAndDestroy(TInt aCount);
	IMPORT_C void PopAndDestroyAll();
	IMPORT_C void Check(TAny* aExpectedItem);
protected:
	IMPORT_C void DoPop(TInt aCount,TBool aDestroy);
	IMPORT_C void DoPopAll(TBool aDestroy);
protected:
	IMPORT_C CCleanup();
protected:
	/**
	Pointer to the bottom of the cleanup stack.
	*/
	TCleanupStackItem* iBase;
	
	
	/**
	Pointer to the top of the cleanup stack.
	*/
	TCleanupStackItem* iTop;
	
	
	/**
	Pointer to the next availaible slot in the cleanup stack.
	*/
	TCleanupStackItem* iNext;
	};




NONSHARABLE_CLASS(TCleanupTrapHandler) : public TTrapHandler
/**
@publishedAll
@released

Implementation for a handler to work with the TRAP mechanism.

This class does not normally need to be used or accessed directly by applications 
and third party code.
*/
	{
public:
	TCleanupTrapHandler();
	virtual void Trap();
	virtual void UnTrap();
	
	virtual void Leave(TInt aValue);
	inline CCleanup& Cleanup();
private:
	CCleanup* iCleanup;
	friend class CTrapCleanup;
	};




template <class T>
class TAutoClose
/**
@publishedAll
@released

Automatically calls Close() on an object when that object goes out of scope.

The behaviour takes advantage of the fact that the compiler automatically 
destroys objects that go out of scope.
*/
	{
public:
	inline ~TAutoClose();
	inline void PushL();
	inline void Pop();
private:
	static void Close(TAny *aObj);
public:
	/**
	An instance of the template class.
	*/
	T iObj;
	};




class CTrapCleanup : public CBase
/**
@publishedAll
@released

Cleanup stack interface. 

The creation and destruction of a cleanup stack is done automatically by GUI 
applications and servers.
*/
	{
public:
	IMPORT_C static CTrapCleanup* New();
	IMPORT_C ~CTrapCleanup();
protected:
	IMPORT_C CTrapCleanup();
private:
	TCleanupTrapHandler iHandler;
	TTrapHandler* iOldHandler;
	};




class CCirBufBase : public CBase
/**
@publishedAll
@released

Base class for circular buffers.

The class is part of the implementation of circular buffers and is never
instantiated. 

The class provides member functions that form part of the interface.
*/
	{
public:
	IMPORT_C ~CCirBufBase();
	inline TInt Count() const;
	inline TInt Length() const;
	IMPORT_C void SetLengthL(TInt aLength);
	IMPORT_C void Reset();
protected:
	IMPORT_C CCirBufBase(TInt aSize);
	IMPORT_C TInt DoAdd(const TUint8* aPtr);
	IMPORT_C TInt DoAdd(const TUint8* aPtr,TInt aCount);
	IMPORT_C TInt DoRemove(TUint8* aPtr);
	IMPORT_C TInt DoRemove(TUint8* aPtr,TInt aCount);
protected:
	TInt iCount;
	TInt iSize;
	TInt iLength;
	TUint8* iPtr;
	TUint8* iPtrE;
	TUint8* iHead;
	TUint8* iTail;
	};




template <class T>
class CCirBuf : public CCirBufBase
/**
@publishedAll
@released

A circular buffer containing objects of a type defined by the
template parameter.
*/
	{
public:
	inline CCirBuf();
#if defined(__VC32__)
	inline ~CCirBuf() {}
#endif
	inline TInt Add(const T* aPtr);
	inline TInt Add(const T* aPtr,TInt aCount);
	inline TInt Remove(T* aPtr);
	inline TInt Remove(T* aPtr,TInt aCount);
	};




class CCirBuffer : public CCirBuf<TUint8>
/**
@publishedAll
@released

Circular buffer of unsigned 8-bit integers. 

The integer values range from 0 to 255.
*/
	{
public:
	IMPORT_C CCirBuffer();
	IMPORT_C ~CCirBuffer();
	IMPORT_C TInt Get();
	IMPORT_C TInt Put(TInt aVal);
	};
//



class CActive : public CBase
/**
@publishedAll
@released

The core class of the active object abstraction.

It encapsulates both the issuing of a request to an asynchronous service provider 
and the handling of completed requests. An application can have one or more 
active objects whose processing is controlled by an active scheduler.
*/
	{
public:

/**
Defines standard priorities for active objects.
*/
enum TPriority
	{
	/**
	A low priority, useful for active objects representing
	background processing.
	*/
	EPriorityIdle=-100,
	
	
	/**
	A priority higher than EPriorityIdle but lower than EPriorityStandard.
	*/
	EPriorityLow=-20,
	
	
	/**
	Most active objects will have this priority.
	*/
	EPriorityStandard=0,


	/**
	A priority higher than EPriorityStandard; useful for active objects
	handling user input.
	*/
	EPriorityUserInput=10,
	
	
	/**
	A priority higher than EPriorityUserInput.
	*/
	EPriorityHigh=20,
	};
public:
	IMPORT_C ~CActive();
	IMPORT_C void Cancel();
	IMPORT_C void Deque();
	IMPORT_C void SetPriority(TInt aPriority);
	inline TBool IsActive() const;
	inline TBool IsAdded() const;
	inline TInt Priority() const;
protected:
	IMPORT_C CActive(TInt aPriority);
	IMPORT_C void SetActive();


    /**
    Implements cancellation of an outstanding request.
	
	This function is called as part of the active object's Cancel().
	
	It must call the appropriate cancel function offered by the active object's 
	asynchronous service provider. The asynchronous service provider's cancel 
	is expected to act immediately.
	
	DoCancel() must not wait for event completion; this is handled by Cancel().
	
	@see CActive::Cancel
	*/
	virtual void DoCancel() =0;


	/**
	Handles an active object's request completion event.
	
	A derived class must provide an implementation to handle the
	completed request. If appropriate, it may issue another request.
	
	The function is called by the active scheduler when a request
	completion event occurs, i.e. after the active scheduler's
	WaitForAnyRequest() function completes.
	
	Before calling this active object's RunL() function, the active scheduler 
	has:
	
	1. decided that this is the highest priority active object with
	   a completed request
	
    2. marked this active object's request as complete (i.e. the request is no 
	   longer outstanding)
	
	RunL() runs under a trap harness in the active scheduler. If it leaves,
	then the active scheduler calls RunError() to handle the leave.
	
	Note that once the active scheduler's Start() function has been called, 
	all user code is run under one of the program's active object's RunL() or 
	RunError() functions.
	
	@see CActiveScheduler::Start
	@see CActiveScheduler::Error
	@see CActiveScheduler::WaitForAnyRequest
	@see TRAPD
	*/
	virtual void RunL() =0;
	IMPORT_C virtual TInt RunError(TInt aError);
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
public:
	
	/**
	The request status associated with an asynchronous request.
	
	This is passed as a parameter to all asynchronous service providers.
	
	The active scheduler uses this to check whether the active object's request 
	has completed.
	
	The function can use the completion code to judge the success or otherwise 
	of the request.
	*/
	TRequestStatus iStatus;
private:
//	TBool iActive;
	TPriQueLink iLink;
	TAny* iSpare;
	friend class CActiveScheduler;
	friend class CServer;
	friend class CServer2;
	};




class CIdle : public CActive
/**
@publishedAll
@released

An active object that performs low-priority processing when no higher-priority 
active objects are ready to run.

An idle time active object together with its associated callback function 
may be used to implement potentially long running background tasks, such as 
spreadsheet recalculation and word processor repagination.
*/
	{
public:
	IMPORT_C static CIdle* New(TInt aPriority);
	IMPORT_C static CIdle* NewL(TInt aPriority);
	IMPORT_C ~CIdle();
	IMPORT_C void Start(TCallBack aCallBack);
protected:
	IMPORT_C CIdle(TInt aPriority);
	IMPORT_C void RunL();
	IMPORT_C void DoCancel();
protected:
	
	/**
	The callback object that encapsulates the background task.
	
	@see Start
	*/
	TCallBack iCallBack;
	};




class CAsyncOneShot : public CActive
/**
@publishedAll
@released

An active object that performs processing that is only performed once.

The active object is intended to be given a low priority, so that it runs 
only when no higher-priority active objects are ready to run. In addition, 
the class ensures that the current thread cannot be closed until the active 
object is destroyed.

The class needs to be derived from to make use of its behaviour, in particular, 
it needs to define and implement a RunL() function.

NB: the constructor creates a process-relative handle to the current thread 
and this is stored within this object. If the thread subsequently dies abnormally, 
then this handle will not be closed, and the thread will not be destroyed 
until the process terminates.

NB: if Call() is called from a different thread (for example, to implement 
a kind of inter-thread communication), a client-specific mechanism must be 
used to ensure that the thread that created this object is still alive.

NB: if the thread that created this object has its own heap and terminates 
abnormally, then the handle stored within this object is lost.

@see CActive::RunL
@see CAsyncOneShot::Call
*/
	{
public:
	IMPORT_C CAsyncOneShot(TInt aPriority);
	IMPORT_C virtual void DoCancel();
	IMPORT_C virtual void Call();
	IMPORT_C virtual ~CAsyncOneShot();
	inline RThread& Thread();
private:
	void Setup();
	RThread iThread;
	};




class CAsyncCallBack : public CAsyncOneShot
/**
@publishedAll
@released

An active object that performs its processing through an associated call back 
function, and which is only performed once.
*/
	{
public:
	IMPORT_C CAsyncCallBack(TInt aPriority);
	IMPORT_C CAsyncCallBack(const TCallBack& aCallBack, TInt aPriority);
	IMPORT_C void Set(const TCallBack& aCallBack);
	IMPORT_C void CallBack();
	IMPORT_C virtual ~CAsyncCallBack();
protected:
	virtual void RunL();
//
protected:
	/**
	The callback object that encapsulates the callback function.
	*/
	TCallBack iCallBack;
	};




class TDeltaTimerEntry
/**
@publishedAll
@released

A timed event entry.

An object of this type is added to a queue of timed events, as represented 
by a CDeltaTimer object. It represents a call back function that is called 
when the associated timed event expires.

@see CDeltaTimer
*/
	{
	friend class CDeltaTimer;
public:
	inline TDeltaTimerEntry(const TCallBack& aCallback);
	inline TDeltaTimerEntry();
	inline void Set(TCallBack& aCallback);
private:
	TCallBack iCallBack; 
	TTickCountQueLink iLink;
	};
	
	
	

class CDeltaTimer : public CActive
/**
@publishedAll
@released

A queue of timed events.

A timed event is a callback function encapsulated by a TDeltaTimerEntry object, 
and is intended to be called when the time interval represented by the event 
expires.

The queue itself is a TDeltaQue list. A timed event entry is added into a 
position in the queue that is determined by the time interval specified for 
that event. Although the time interval for a timed event is specified as an 
interval from the present moment, when added to the queue the implementation 
treats each event as having an interval from the previous timed event (or now).

CDeltaTimer is an active object, driven by an RTimer which is usually set to
expire upon completion of the event at the head of the queue.  If the time to
the next event is too great or an event at the head of the queue has been
removed, the timer may be set to expire prior to the event at the head of the
queue (if any).

When the timer completes, the head of the queue is inspected to see whether
the timed event at the head of the queue has expired.  On expiry, the callback
function represented by that timed event is called, and the timed event entry
is removed from the queue.  The queue then inspects further events for expiry,
calling and removing them as necessary until either the queue is empty or there
is an event in the future to wait for.

Note that the tick period is the minimum time interval for an event and the
granularity of all timings using the queue.  Note that in general, any event
may be called back some time after it has expired and that specifically the
duration of all events will at least be rounded up to a muliple of the tick
period.


@see TDeltaTimerEntry
@see TDeltaQue
@see RTimer
*/
	{
public:
	// Queue management
	IMPORT_C virtual void Queue(TTimeIntervalMicroSeconds32 aTimeInMicroSeconds, TDeltaTimerEntry& aEntry);
	IMPORT_C virtual void Remove(TDeltaTimerEntry& aEntry);
	IMPORT_C TInt QueueLong(TTimeIntervalMicroSeconds aTimeInMicroSeconds, TDeltaTimerEntry& aEntry);

	// Factory functions
	IMPORT_C static CDeltaTimer* NewL(TInt aPriority);
	IMPORT_C static CDeltaTimer* NewL(TInt aPriority, TTimeIntervalMicroSeconds32 aGranularity);

	// Destructor
	~CDeltaTimer();

private:
	// Construction
	CDeltaTimer(TInt aPriority, TInt aTickPeriod);

	// From CActive	
	void DoCancel();
	void RunL();

	// Utility
	void Activate(TBool aRequeueTimer = EFalse);

private:	
	/**
	The asynchronous timer.
	*/
	RTimer iTimer;
	
	/**
	The list of timed event entries.
	*/
	TTickCountQue iQueue;
	
	/**
	The period of a tick count.
	*/
	const TInt iTickPeriod;
	
	/**
	Pseudo-lock on the the queue to avoid reentrancy problems
	*/
	TBool iQueueBusy;
	};




class CTimer : public CActive
/**
@publishedAll
@released

Base class for a timer active object.

This is an active object that uses the asynchronous services provided by RTimer, 
to generate events. These events occur either at a specific time specified 
as a TTime, or after an interval specified in microseconds.

The RunL() virtual member function is called by the active scheduler after 
this event occurs.

To write a class derived from CTimer, first define and implement a constructor 
through which the priority of the CTimer active object can be specified. Then 
define and implement a suitable RunL() function to handle the completion of 
a timer request. This function is not defined by CTimer itself and must, therefore, 
be provided by the derived class.

This class is ultimately implemented in terms of the nanokernel tick, and
therefore the granularity of the generated events is limited to the period of
this timer.  This is variant specific, but is usually 1 millisecond.

Note that the CPeriodic and CHeartbeat classes are derived from CTimer, and 
answer most timing needs.

@see CHeartbeat
@see CPeriodic
@see CHeartbeat
*/
	{
public:
	IMPORT_C ~CTimer();
	IMPORT_C void At(const TTime& aTime);
	IMPORT_C void AtUTC(const TTime& aTimeInUTC);
	IMPORT_C void After(TTimeIntervalMicroSeconds32 anInterval);
	IMPORT_C void Lock(TTimerLockSpec aLock);
	IMPORT_C void Inactivity(TTimeIntervalSeconds aSeconds);
	IMPORT_C void HighRes(TTimeIntervalMicroSeconds32 aInterval);
protected:
	IMPORT_C CTimer(TInt aPriority);
	IMPORT_C void ConstructL();
	IMPORT_C void DoCancel();
private:
	RTimer iTimer;
	};




class CPeriodic : public CTimer
/**
@publishedAll
@released

Periodic timer active object. 

This class generates regular timer events and handles them with a callback 
function. The callback is specified as a parameter to Start().

The callback may not be called immediately after the signal from the timer 
request has been generated, for the following reasons:

1. the RunL() of another active object may be running at the time of the signal

2. other active objects may have a higher priority than the CPeriodic

If timing accuracy is important to your application, you can minimise the 
first problem by ensuring all RunL()s complete quickly, and can eliminate 
the second by giving the CPeriodic a higher priority than any other active 
object. Although it is generally recommended that timer-related active objects 
have a high priority, this will not address the problem of CPeriodic timers 
running behind, because active object scheduling is not pre-emptive.

After a timer signal generated by a CPeriodic, the next signal is requested 
just before running the callback, and this request can be delayed for the 
same reasons that running the callback can be delayed. Therefore, a large 
number N of periods may add up to somewhat more than N times the requested 
period time. If absolute precision is required in tracking time, do not rely 
on counting the number of times the callback is called: read the value of 
the system clock every time you need it.

For many applications, such precision is not required, for example, tick 
counting is sufficiently accurate for controlling time-outs in a communications 
program.

Note that you should be familiar with CActive in order to understand
CPeriodic behaviour, but not necessarily with CTimer.

@see CHeartbeat
*/
	{
public:
	IMPORT_C static CPeriodic* New(TInt aPriority);
	IMPORT_C static CPeriodic* NewL(TInt aPriority);
	IMPORT_C ~CPeriodic();
	IMPORT_C void Start(TTimeIntervalMicroSeconds32 aDelay,TTimeIntervalMicroSeconds32 anInterval,TCallBack aCallBack);
protected:
	IMPORT_C CPeriodic(TInt aPriority);
	IMPORT_C void RunL();
private:
	TTimeIntervalMicroSeconds32 iInterval;
	TCallBack iCallBack;
	};




class MBeating
/**
@publishedAll
@released

Heartbeat timer call-back handling interface.

The interface provides a pair of functions to handle the beating and
synchronisation of heartbeat timers.

The CHeartbeat active object class uses an object implementing the MBeating 
interface.

@see CHeartbeat::Start
*/
	{
public:
	/**
	Handles a regular heartbeat timer event.
	
	This type of event is one where the timer completes in synchronisation
	with the system clock.
	*/
	virtual void Beat() =0;
	
	/**
	Synchronises the heartbeat timer with system clock. 
	
	This function handles a heartbeat timer event where the timer completes out 
	of synchronisation with the system clock, (i.e. one or more heartbeats have 
	been missed).
	*/
	virtual void Synchronize() =0;
	};




class CHeartbeat : public CTimer
/**
@publishedAll
@released

Heatbeat timer.

This class generates regular heartbeat events on a fixed fraction of a second. 
It is more accurate than a CPeriodic timer, because it provides a function 
to restore timer accuracy if it gets out of synchronisation with the system 
clock.

The protected RunL() function is called when the timer completes. The RunL() 
function in turn calls either the MBeating::Beat() or the MBeating::Synchronize() 
functions; MBeating is specified as a parameter to the Start() function 
used to start the heartbeat timer.

The relevant MBeating function may not be called immediately after the signal 
from the timer request has been generated, for the following reasons:

1. the RunL() of another active object may be running at the time of the signal

2. other active objects may have a higher priority than the CHeartbeat

If no heartbeat is missed, then the Beat() function is called.

If one or more heartbeats are missed then the Synchronize() function is called. 
It is important to bear in mind that the machine might be switched off after 
a few beats of the heart, and then Synchronize() will be called several days 
later. It is therefore essential that synchronisation is achieved as quickly 
as possible, rather than trying to catch up a tick at a time. In the context 
of an analogue clock, for instance, the clock should just redraw itself with 
the current time - rather than moving the hands round in steps until the time 
is correct.

CHeartbeat is an active object, derived from CActive (via CTimer). You should 
be familiar with CActive in order to understand CHeartbeat behaviour, but 
not necessarily with CTimer.

@see MBeating
*/
	{
public:
	IMPORT_C static CHeartbeat* New(TInt aPriority);
	IMPORT_C static CHeartbeat* NewL(TInt aPriority);
	IMPORT_C ~CHeartbeat();
	IMPORT_C void Start(TTimerLockSpec aLock,MBeating *aBeating);
protected:
	IMPORT_C CHeartbeat(TInt aPriority);
	IMPORT_C void RunL();
private:
	TTimerLockSpec iLock;
	MBeating *iBeating;
	};
//

class CServer2;




/**
@publishedAll
@released

Represents a session (version 2) for a client thread on the server-side.

A session acts as a channel of communication between the client and the server.
A client thread can have multiple concurrent sessions with a server.

A session can be:
- restricted to the creating thread
- can be shared with other threads in the same process
- can be shared by all threads in the system.

A server must define and implement a derived class. In particular, 
it must provide an implementation for the ServiceL() virtual function.

(Note that this class should be used instead of CSession)
*/
class CSession2 : public CBase
	{
	friend class CServer2;
public:
	IMPORT_C virtual ~CSession2() =0;
private:
	IMPORT_C virtual void CreateL(); // Default method, does nothing
public:
	inline const CServer2* Server() const;
	IMPORT_C void ResourceCountMarkStart();
	IMPORT_C void ResourceCountMarkEnd(const RMessage2& aMessage);
	IMPORT_C virtual TInt CountResources();

    /**
    Handles the servicing of a client request that has been passed
    to the server.

    This function must be implemented in a derived class. The details of
    the request are contained within the message.

	@param aMessage The message containing the details of the client request.
    */
	virtual void ServiceL(const RMessage2& aMessage) =0;
	IMPORT_C virtual void ServiceError(const RMessage2& aMessage,TInt aError);
protected:
	IMPORT_C CSession2();
	IMPORT_C virtual void Disconnect(const RMessage2& aMessage);
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
public:
	IMPORT_C void SetServer(const CServer2* aServer);
    /**
    @internalComponent
    */
	enum TPanicNo {ESesCountResourcesNotImplemented=1,ESesFoundResCountHeaven};

private:
	TInt iResourceCountMark;
	TDblQueLink iLink;
	const CServer2* iServer;
	TAny* iSpare;
	};

/**
@publishedAll
@released

Abstract base class for servers (version 2).

This is an active object. It accepts requests from client threads and forwards
them to the relevant server-side client session. It also handles the creation
of server-side client sessions as a result of requests from client threads.

A server must define and implement a derived class.

(Note that this class should be used instead of CServer)
*/
class CServer2 : public CActive
	{
public:

	/**
	This enumeration defines the maximum sharability of sessions opened
	with this server; for backwards compatibilty, these should be have
	the same values as the corresponding EIpcSessionType enumeration
	*/
	enum TServerType
		{
		EUnsharableSessions					= EIpcSession_Unsharable,
		ESharableSessions					= EIpcSession_Sharable,
		EGlobalSharableSessions				= EIpcSession_GlobalSharable,
		};

public:
	IMPORT_C virtual ~CServer2() =0;
	IMPORT_C TInt Start(const TDesC& aName);
	IMPORT_C void StartL(const TDesC& aName);
	IMPORT_C void ReStart();
	IMPORT_C void SetPinClientDescriptors(TBool aPin);
	
	/**
	Gets a handle to the server.
	
	Note that the RServer2 object is classified as Symbian internal, and its
	member functions cannot be acessed. However, the handle can be passed
	to the RSessionBase::CreateSession() variants that take a server handle.
	
	@return The handle to the server.
	*/
	inline RServer2 Server() const { return iServer; }
protected:
	inline const RMessage2& Message() const;
	IMPORT_C CServer2(TInt aPriority, TServerType aType=EUnsharableSessions);
	IMPORT_C void DoCancel();
	IMPORT_C void RunL();
	IMPORT_C TInt RunError(TInt aError);
	IMPORT_C virtual void DoConnect(const RMessage2& aMessage);
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
private:
    
    /**
    Creates a server-side session object.

    The session represents a communication link between a client and a server,
    and its creation is initiated by the client through a call to one of
    the RSessionBase::CreateSession() variants. 

    A server must provide an implementation, which as a minimum should:

    - check that the version of the server is compatible with the client by
      comparing the client supplied version number against the server's version
      number; it should leave if there is incompatibility.

    - construct and return the server side client session object.

    @param aVersion The version information supplied by the client. 
    @param aMessage Represents the details of the client request that is requesting
                    the creation of the session.
    
    @return A pointer to the newly created server-side session object. 
    
    @see User::QueryVersionSupported()
    */
	IMPORT_C virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const =0;
	void Connect(const RMessage2& aMessage);
	void DoConnectL(const RMessage2& aMessage,CSession2* volatile& aSession);
public:
	IMPORT_C void SetMaster(const CServer2* aServer);

	/**
    @internalComponent
    */
	enum TPanic
		{
		EBadMessageNumber,
		ESessionNotConnected,
		ESessionAlreadyConnected,
		EClientDoesntHaveRequiredCaps,
		};

private:
	TUint8 iSessionType;
	TUint8 iServerRole;
	TUint16 iServerOpts;
	RServer2 iServer;
	RMessage2 iMessage;
	TAny* iSpare;
	TDblQue<CSession2> iSessionQ;
	
protected:
	TDblQueIter<CSession2> iSessionIter;
private:
	void Disconnect(const RMessage2& aMessage);
	static void BadMessage(const RMessage2& aMessage);
	static void NotConnected(const RMessage2& aMessage);
	friend class CPolicyServer;
	};



/**
A security policy framework built on top of the normal CServer2 class.

The two major functions of the Policy Server framework are to check a received
message against a security policy and then to perform an action depending on
the result of this check. The exact behaviour is defined by the contents of
the TPolicy structure given in the constructor for CPolicyServer. 

The processing performed when a server receives a message are describe below.
This should aid understanding of the interaction of the TPolicy structure and
virtual member functions which may be implemented by classes derived from CPolicyServer.

Checking the Security Policy

On receipt of a message, the message function number is used to search the
list of ranges pointed to by TPolicy::iRanges. This yields a range
number R, which is between 0 and TPolicy::iRangeCount-1.
The policy index, X, for this range is then fetched from TPolicy::iElementsIndex[R].
If the message is a Connect message, then X is fetched directly from TPolicy::iOnConnect
instead.

The further action taken is determined by the value of X.
-	If X==TSpecialCase::EAlwaysPass,
	the message is processed as normal; either by passing it to the ServiceL()
	method of a session, or, in the case of a connection message, a new session
	is created.
-	If X==TSpecialCase::ENotSupported,
	the message is completed with KErrNotSupported.
-	If X==TSpecialCase::ECustomCheck,
	a call to the virtual function CustomSecurityCheckL() is made. The implementation
	of this method must return one of the TCustomResult enumerations which determine
	what further action is to be taken:
	-	TCustomResult::EPass 
		The message is processed as normal; either by passing it to the ServiceL()
		method of a session, or, in the case of a connection message, a new session
		is created.
	-	TCustomResult::EFail 
		This causes CheckFailedL() to be called with the action specified by the
		aAction reference given to CustomSecurityCheckL() (This defaults to
		TFailureAction::EFailClient.)
	-	TCustomResult::EAsync 
		The derived class is responsible for further processing of the message,
		the Policy Server framework will do nothing more with it.
-	If X < TSpecialCase::ESpecialCaseHardLimit,
		X is taken as an index into the array of TPolicyElement objects pointed
		to by TPolicy::iElements. The platform security attributes of the process
		which sent the message being processed are checked against the security
		policy specified in this TPolicyElement. If the process possesses all of
		the attributes specified then the message processed as normal. Otherwise,
		CheckFailedL() is called with the action value specified in the TPolicyElement .

Handling Policy Check Failure

The CheckFailedL() method is called when a security check has failed. It performs
an action according to the aAction value given to it:

-	If aAction==TFailureAction::EFailClient, the message is completed with
	KErrPermissionDenied.
-	If aAction==TFailureAction::EPanicClient, the client thread is panicked.
-	If aAction < 0 a call to the virtual function CustomFailureActionL() is made.
	The implementation of this method must return one of the TCustomResult
	enumerations which determine what further action is to be taken:
	-	TCustomResult::EPass 
		The message is processed as normal; either by passing it to the ServiceL()
		method of a session, or, in the case of a connection message, a new session
		is created.
	-	TCustomResult::EFail 
		The message is completed with KErrPermissionDenied.
	-	TCustomResult::EAsync 
		The derived class is responsible for further processing of the message,
		the Policy Server framework will do nothing more with it.

@publishedAll
@released
*/
class CPolicyServer : public CServer2
	{
public:
	/** Enumeration specifying action to take if a security check fails.
	Values >= 0 are handled by CheckFailedL().  Values < 0 are specific to the
	derived implementation of the policy server and will result in a call to
	CustomFailureActionL() if a security check fails.  Attempts to use undefined
	values >= 0 will result in a panic in CheckFailedL().
	*/
	enum TFailureAction
		{
		EFailClient	= 0,	/**< Complete message with KErrPermissionDenied */
		EPanicClient= 1,	/**< Panic client */
		};

	/** Enumeration of acceptable return codes from both of
	CustomSecurityCheckL() and CustomFailureActionL().  Results of EPass or EFail
	are handled by the CPolicyServer framework.  No other action is required on
	the part of the derived implementation.  However, results of EAsync imply
	that the derived implementation will call the appropriate function once the
	result is known.  See CustomSecurityCheckL() and CustomFailureActionL for
	more information.
	*/
	enum TCustomResult
		{
		EPass = 0,	/**< Security check passed. */
		EFail = 1,	/**< Security check failed. */
		EAsync = 2,	/**< Security checking will be performed asynchronously. */
		};

	/** Class specifying a security check and the action to take

	If iAction is >=0 it must be a member of TFailureAction
	If iAction is <0 it is assumed to specify a custom action specific to the
	derived implementation.  In this case, CustomFailureActionL must be implemented
	by the derived class.
	*/
	class TPolicyElement
		{
	public:
		/** Security policy to check against the client which sent a message.

		This class can specify a security policy consisting of either:

		-#	A check for between 0 and 7 capabilities
		-#	A check for a given Secure ID along with 0-3 capabilities
		-#	A check for a given Vendor ID along with 0-3 capabilities

		This member should only be initialised by one of the following macros:

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

		@see TPolicy
		*/
		TStaticSecurityPolicy	 	iPolicy;

		/** Action to take on failure. Either a value from TFailureAction
			or a negative value which has meaning to the CustomFailureActionL()
			method of a derived class.
		*/
		TInt						iAction;	
		};

	/** Special case values which can be used instead of a policy element index
		contained in the array TPolicy::iElementsIndex
	*/
	enum TSpecialCase 
		{
		/** Indicates a custom check should be made by calling CustomSecurityCheckL() */
		ECustomCheck 			=255u,

		/** Indicates that message is requesting an unsupported function.
			The message is completed with KErrNotSupported. */
		ENotSupported			=254u,

		/** Indicates that the message is requesting an unrestricted function
			and therefore should be processed without any further checks. */
		EAlwaysPass				=253u, 

		ESpecialCaseLimit 		=252u, 		/**< @internalTechnology */
		ESpecialCaseHardLimit	=250u		/**< @internalTechnology */
		};

	/** Object specifying which security checks to perform on each request
	number and what action to take if the check fails.  

	Explanations of each of the members of this class are detailed below.

	As explained in CPolicyServer::CPolicyServer, it is important that the
	instance of this class (CPolicyServer::TPolicy) given to the policy
	server constructor, exists for the lifetime of the server. For this
	reason, as well as code size considerations, it is recommended that
	the TPolicy instance is const static data.
	The following code segment shows the recommended way of doing this.
	Further detail on what each of these statements means is given below.

	@code
	const TUint myRangeCount = 4;
	const TInt myRanges[myRangeCount] = 
		{
		0, //range is 0-2 inclusive
		3, //range is 3-6 inclusive
		7, //range is 7
		8, //range is 8-KMaxTInt inclusive
		};
	const TUint8 myElementsIndex[myRangeCount] = 
		{
		1, 								//applies to 0th range (req num: 0-2)
		CPolicyServer::ECustomCheck, 	//applies to 1st range (req num: 3-6)
		0, 								//applies to 2nd range (req num: 7)
		CPolicyServer::ENotSupported,	//applies to 3rd range (req num: 8-KMaxTInt)
		};
	const CPolicyServer::TPolicyElement myElements[] = 
		{
		{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin), CPolicyServer::EFailClient},
		{_INIT_SECURITY_POLICY_C1(ECapabilityLocation), CMyPolicyServer::EQueryUser},
		}
	const CPolicySErver::TPolicy myPolicy =
		{
		CPolicyServer::EAlwaysPass, //specifies all connect attempts should pass
		myRangeCount,					
		myRanges,
		myElementsIndex,
		myElements,
		}
	@endcode
	*/
	class TPolicy
		{
	public:
		/** The index into iElements, or an allowed value of TSpecialCase,
		that is used to check a connection attempt . */
		TUint8 iOnConnect;

		/** Number of ranges in the iRanges array. */
		TUint16 iRangeCount;

		/** A pointer to an array of ordered ranges of request numbers.  Each
		element in this array refers to the starting request number of a range.
		The range of the previous element is up to and including the current
		element minus 1.  Thus an array like:
		@code
		const TInt myRanges[4] = {0, 3, 7, 8};
		@endcode
		means that:
		- the 0th range is 0-2 (inclusive).
		- the 1st range is 3-6 (inclusive).
		- the 2nd range is solely request number 7.
		- the 3rd range is 8-KMaxTInt (inclusive).

		Note that the all possible request numbers must be accounted for.  This
		implies that the first element must be 0.  It also implies that the
		last range goes from the that element to KMaxTint.  Finally, each
		element must be strictly greater than the previous element.  As the
		first element is 0, this clearly implies that iRanges must not contain
		negative elements. 
		*/
		const TInt* iRanges;

		/** A pointer to an array of TUint8 values specifying the appropriate action
		to take for each range in iRanges.  For example, the 0th element of
		iElementsIndex specifies the appropriate action to take for the 0th
		range in iRanges.  As such, iElementsIndex must have precisely the same
		number of elements as iRanges.  
	
		The following rules apply to the value of each element in iElementsIndex:
		-#	Each value must be a valid index into iElements (that is, less than
			the number of elements in iElements) OR a valid value from
			TSpecialCase. 
		-#	Elements' values need not follow any special ordering.
		-#	Elements may repeat values.
		
		Continuing the example from iRanges:
		@code
		const TInt myRanges[4] = {0, 3, 7, 8};
		const TUInt8 myElementsIndex[4] = {
			1, 
			CPolicyServer::ECustomCheck, 
			0, 
			CPolicyServer::ENotSupported
			};
		@endcode
		This means that:
		-#	Requests within the first range of myRanges (request numbers 0-2)
			will be checked against the policy specified by the 1st element of
			iElements.
		-#	Requests with the the second range of myRanges (request numbers
			3-6) require a custom check to determine if they are allowed.  This requires
			derived server implementations to implement CustomSecurityCheckL()
		-#	Requests within the third range of myRanges (request number 7) will
			be checked against the policy specified by the 0th element of iElements.
		-#	Requests within the fourth range of myRanges (request numbers
			8-KMaxTInt) will automatically be completed with KErrNotSupported by
			the policy server framework.
		*/
		const TUint8* iElementsIndex;

		/** A pointer to an array of distinct policy elements.

		Continuing with the previous examples:
		@code
		const TInt myRanges[4] = {0, 3, 7, 8};
		const TUInt8 myElementsIndex[4] = {
			1, 
			CPolicyServer::ECustomCheck, 
			0, 
			CPolicyServer::ENotSupported
			};
		const TPolicyElement iElements[] = {
			{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin), CPolicyServer::EFailClient},
			{_INIT_SECURITY_POLICY_C1(ECapabilityLocation), CMyPolicyServer::EQueryUser}
			}
		@endcode

		The instantiation of iElements specifies that:
		-#	Request numbers 0-2 require the Location capability.  As the
			iAction member of the 1st element specifies a custom action
			(represented by the negative number, CMyPolicyServer::EQueryUser),
			requests without Location will passed to the reimplementation of
			CustomFailureActionL.
		-#	Request number 7 requires the DiskAdmin capability.  Requestors
			without DiskAdmin will have their request completed with
			KErrPermissionDenied.
		*/
		const TPolicyElement* iElements;	
		};

public:
	/** Process an accepted message which has passed its policy check.

	The message is either passed to the ServiceL() method of a session,
	or, in the case of a connection message, a new session is created.
		
	This is called by RunL() to process a message which has passed its security
	check.  If the server implementation returns EAsync from either
	CustomSecurityCheckL() or CustomFailureActionL(), then it is the responsibility
	of the derived server implementation to call ProcessL at a later point if
	the messages passes the asynchronous check.

	This function should only ever be called by derived implementations if
	asynchronous security checks are in use.
	*/
	IMPORT_C void ProcessL(const RMessage2& aMsg);

	/** Called when a security check has failed.  

	The aAction parameter determines the action taken:
	-	If aAction==TFailureAction::EFailClient, the message is completed with
		KErrPermissionDenied.
	-	If aAction==TFailureAction::EPanicClient, the client thread is panicked.
	-	If aAction < 0 a call to the virtual function CustomFailureActionL() is made.

	This function should only ever be called by derived implementations if
	asynchronous security checks are in use. 

	@param	aMsg	 The message which failed its check.
	@param	aAction	 The action to take. (See description.)
	@param 	aMissing A list of the security attributes that were missing from
					 the checked process.  
	*/
	IMPORT_C void CheckFailedL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing);

	/** Called if a leave occurs during processing of a message.  The
	underlying framework ensures that leaves which occur during
	CSession2::ServiceL are passed to CSession2::ServiceError.  Leaves occuring
	prior to this (ie. during CustomSecurityCheckL() or CustomFailureActionL() ) are
	completed with the leave code.

	This function should only ever be called by derived implementations if
	asynchronous security checks are in use.  In this case the RunError() of
	that other active object must call ProcessError().

	@param	aMsg		The message being processed when the leave occurred.
	@param	aError		The leave code.
	*/
	IMPORT_C void ProcessError(const RMessage2& aMsg, TInt aError);

protected:
	/** Construct a policy server

	@param aPriority	Active object priority for this server
	@param aPolicy		Reference to a policy object describing the security checks
						required for each message type.  The server does not make a
						copy of policy, and therefore this object must exist for the
						lifetime of the server.  It is recommended that aPolicy
						is in const static data.
	@param aType		Type of session sharing supported by this server
	*/
	IMPORT_C CPolicyServer(TInt aPriority, const TPolicy& aPolicy, TServerType aType=EUnsharableSessions);

	/** Performs a custom security check.
	Derived server classes must implement this function if any element in
	iElementsIndex has the value CPolicyServer::ECustomCheck.
	Similarly, if CPolicyServer::ECustomCheck is not used, then this function
	can be safely ignored.

	If CPolicyServer::ECustomCheck is used, there are two further cases to consider:
	-#	The custom security check can synchronously decide if the message
		should pass.  In this case, the derived implementation must simply return
		either EPass or EFail depending on the result of the security check.
	-#	The custom security check needs to use asynchronous methods in order
		to determine whether the message should procceed.  In this case, these
		asysnchronous methods should be started and then the EAsync value returned.
		Furthermore, implmentations returning EAsync commit to the following:
		-	If the security check eventually passes, ProcessL() must be called with
			the appropriate message.
		-	If the security check eventually fails, CheckFailedL() must be called
			with that message.
		-	Pending messages on a given session need to be completed and discarded
			if the session is closed.

		IMPORTANT NOTE. When processing a message asynchronously, a copy must be
		made of the RMessage2 object. Saving a refernece or pointer to the original
		message will produce unpredictable defects. This is because the object will
		be reused for the next message that the server receives.
		
	In both cases, synchronous and asynchronous, the derived implementation has the
	option of updating the aAction and/or aMissing parameters if that is
	appropriate.

	@param	aMsg The message to check.
	@param	aAction A reference to the action to take if the security check
			fails. This is either a value from TFailureAction or a negative
			value which has meaning to the CustomFailureActionL() method of
			a derived class.
			The policy server framework gives this value a default of
			EFailClient.  If a derived implementation wishes a
			different value, then it should change this.
	@param 	aMissing A reference to the list of security attributes missing
			from the checked process.  The policy server initialises this
			object to zero (that is a sid of 0, a vid of 0, and no capabilities).
			If derived implementations wish to take advantage of a list of
			missing attributes in their implementation of CustomFailureActionL(),
			then they should set those missing attributes here in
			CustomSecurityCheckL().
	@return A value from TCustomResult.  
	@panic CBase 95 If the default implementation is called.
	*/
	IMPORT_C virtual TCustomResult CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing);

	/** Performs a custom action after the failure of a security check.
	Derived server classes must implement this function if the aAction value
	passed to CheckFailedL() is less than zero.  This can happened if the policy
	specified a negative number in the iAction member of any of the
	TPolicyElements, or, if the derived CustomSecurityCheckL() modified the
	value of aAction prior to returning.
	
	If negative aAction values are used, there are two further cases to consider:
	-#	The custom security check can synchronously decide if the message
		should pass.  In this case, the derived implementation must simply return
		either EPass or EFail depending on the result of the security check.
	-#	The custom security check needs to use asynchronous methods in order
		to determine whether the message should still proceed.  In this case, these
		asysnchronous methods should be started and then the EAsync value returned.
		Furthermore, implmentations returning EAsync commit to the following:
		-	If the security check eventually passes, ProcessL() must be called with
			the appropriate message.
		-	If the security check eventually fails, or if a fatal error condition occurs, 
			including if the previously mentioned call to ProcessL() leaves; 
			then CPolicyServer::ProcessError() should be called passing the message and 
			relevant error code.
		-	Pending messages on a given session need to be completed and discarded 
			if the session is closed.

		IMPORTANT NOTE. When processing a message asynchronously, a copy must be
		made of the RMessage2 object. Saving a refernece or pointer to the original
		message will produce unpredictable defects. This is because the object will
		be reused for the next message that the server receives.

	The default implementation of this function panics the server.

	@param	aMsg The message to check
	@param	aAction The custom failure action requested.
					This is either a value from TFailureAction or a negative
					value which has meaning to the CustomFailureActionL() method of
					a derived class.
	@param 	aMissing A const reference to the list of security attributes missing
			from the checked process.  There are two cases to consider:
			(a) If this message was checked (and failed) by a static policy
				applied by the policy server framework, aMissing will contain a
				list of the security attributes that caused the policy to fail.  An
				completely zeroed aMissing implies that an always fail policy was
				encountered.
			(b) If this message was failed by a custom security check, then
				aMissing will be zeroed unless the CustomSecurityCheckL() method
				filled it in.
	@return A value from TCustomResult.  
	@panic CBase 95 If the default implementation is called.
	*/
	IMPORT_C virtual TCustomResult CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing);

protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
private:
	IMPORT_C virtual void RunL();
	IMPORT_C virtual TInt RunError(TInt aError);
	const CPolicyServer::TPolicyElement* FindPolicyElement(TInt aFn, TUint& aSpecialCase) const;
private:
	const TPolicy& iPolicy;

	};



class CActiveScheduler : public CBase
/**
@publishedAll
@released

Controls the handling of asynchronous requests as represented by
active objects.

An active scheduler is used to schedule the sequence in which active object request
completion events are handled by a single event-handling thread.

An active scheduler can be instantiated and used directly if either:

- the RunL() function of all of its active objects is guaranteed not to leave, or

- each of its active objects implements a suitable RunError() function to provide suitable cleanup

If any of the active scheduler's active objects does not provide a RunError()
function, then a CActiveScheduler derived class must be defined and an implementation
of the Error() function provided to perform the cleanup required.

There is one active scheduler per thread and the static functions provided by the
class always refer to the current active scheduler.

@see CActiveScheduler::Error
@see CActive 
@see CActiveSchedulerWait
*/
	{
	friend class CActiveSchedulerWait;
public:
	struct TLoop;
	typedef TLoop* TLoopOwner;
public:
	IMPORT_C CActiveScheduler();
	IMPORT_C ~CActiveScheduler();
	IMPORT_C static void Install(CActiveScheduler* aScheduler);
	IMPORT_C static CActiveScheduler* Current();
	IMPORT_C static void Add(CActive* aActive);
	IMPORT_C static void Start();
	IMPORT_C static void Stop();
	IMPORT_C static TBool RunIfReady(TInt& aError, TInt aMinimumPriority);
	IMPORT_C static CActiveScheduler* Replace(CActiveScheduler* aNewActiveScheduler);
	IMPORT_C virtual void WaitForAnyRequest();
	IMPORT_C virtual void Error(TInt aError) const;
	IMPORT_C void Halt(TInt aExitCode) const;
	IMPORT_C TInt StackDepth() const;
private:
	class TCleanupBundle
	{
		public:
		CCleanup* iCleanupPtr;
		TInt iDummyInt;
	};
private:
	static void Start(TLoopOwner* aOwner);
	IMPORT_C virtual void OnStarting();
	IMPORT_C virtual void OnStopping();
	IMPORT_C virtual void Reserved_1();
	IMPORT_C virtual void Reserved_2();
	void Run(TLoopOwner* const volatile& aLoop);
	void DoRunL(TLoopOwner* const volatile& aLoop, CActive* volatile & aCurrentObj, TCleanupBundle* aCleanupBundle);
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
protected:
	inline TInt Level() const;	// deprecated
private:
	TLoop* iStack;
	TPriQue<CActive> iActiveQ;
	TAny* iSpare;
	};




class CActiveSchedulerWait : public CBase
/**
@publishedAll
@released

Controls a single scheduling loop in the current active scheduler.

This class provides better control of nested wait loops in the active
scheduler.

Note that a CActiveSchedulerWait object can be used as a data member
inside other CBase derived classes.

@see CActiveScheduler
*/
	{
public:
	IMPORT_C CActiveSchedulerWait();
	IMPORT_C ~CActiveSchedulerWait();
	IMPORT_C void Start();
	IMPORT_C void AsyncStop();
	IMPORT_C void AsyncStop(const TCallBack& aCallMeWhenStopped);
	inline TBool IsStarted() const;
	IMPORT_C TBool CanStopNow() const;
private:
	CActiveScheduler::TLoopOwner iLoop;
	};




class CleanupStack
/**
@publishedAll
@released

A collection of static functions that are used to add resources to and remove 
resources from the cleanup stack.
*/
	{
public:
	IMPORT_C static void PushL(TAny* aPtr);
	IMPORT_C static void PushL(CBase* aPtr);
	IMPORT_C static void PushL(TCleanupItem anItem);
	IMPORT_C static void Pop();
	IMPORT_C static void Pop(TInt aCount);
	IMPORT_C static void PopAndDestroy();
	IMPORT_C static void PopAndDestroy(TInt aCount);
	IMPORT_C static void Check(TAny* aExpectedItem);
	inline static void Pop(TAny* aExpectedItem);
	inline static void Pop(TInt aCount, TAny* aLastExpectedItem);
	inline static void PopAndDestroy(TAny* aExpectedItem);
	inline static void PopAndDestroy(TInt aCount, TAny* aLastExpectedItem);
	};




/**
@publishedAll
@released

A utility class used by the templated function CleanupDeletePushL() to create 
a TCleanupItem item that will perform a delete type operation on
the class T type object.

@see CleanupDeletePushL()
*/
template <class T>
class CleanupDelete
	{
public:
	inline static void PushL(T* aPtr);
private:
	static void Delete(TAny *aPtr);
	};
	
	

	
/**
@publishedAll
@released

Constructs and pushes a TCleanupItem object onto the cleanup stack.

The TCleanupItem encapsulates:

- the pointer aPtr to the object of type class T which is to be cleaned up

- an associated cleanup operation.

The cleanup operation is the private static function Delete() of the templated
class CleanupDelete, and is called as a result of a subsequent call
to CleanupStack::PopAndDestroy().

CleanupDelete::Delete() is passed a pointer to the class T object to be cleaned
up, and the function implements cleanup by deleting the passed object.

An example of its use:

@code
...
CTestOne* one = new (ELeave) CTestOne;
CleanupDeletePushL(one);
...
CleanupStack::PopAndDestroy(); // <--- results in "one" being deleted.
...
@endcode

@param aPtr A pointer to a templated class T type object for which the cleanup item is being created. 

@see TCleanupItem
@see CleanupDelete
@see CleanupStack::PopAndDestroy()
*/
template <class T>
inline void CleanupDeletePushL(T* aPtr);




/**
@publishedAll
@released

A utility class used by the templated function CleanupArrayDeletePushL() to 
create a TCleanupItem item that will perform a delete type operation on an 
array of class T type objects.

@see CleanupArrayDeletePushL()
*/
template <class T>
class CleanupArrayDelete
	{
public:
	inline static void PushL(T* aPtr);
private:
	static void ArrayDelete(TAny *aPtr);
	};




/**
@publishedAll
@released

Constructs and pushes a TCleanupItem object onto the cleanup stack.

The TCleanupItem encapsulates:

- the pointer aPtr to an array of type class T objects to be cleaned up

- an associated cleanup operation.

The cleanup operation is the private static function ArrayDelete() of the
templated class CleanupArrayDelete, and is called as a result of
a subsequent call to CleanupStack::PopAndDestroy().

CleanupArrayDelete::ArrayDelete() is passed a pointer to the array of class T
objects to be cleaned up, and the function implements cleanup by deleting
the passed array using the delete [] operator.

An example of its use:

@code
...
RTestOne* one = new (ELeave) RTestOne [KSomeArraySize];
CleanupArrayDeletePushL(one);
... // Do something with the object.........
CleanupStack::PopAndDestroy(); // <--- results in the array "one" being deleted.
...
@endcode

@param aPtr A pointer to an array of class T type objects for which
            the cleanup item is being created.

@see TCleanupItem
@see CleanupArrayDelete
@see CleanupStack::PopAndDestroy()
*/
template <class T>
inline void CleanupArrayDeletePushL(T* aPtr);




/**
@publishedAll
@released

A utility class used by the templated function CleanupClosePushL() to create 
a TCleanupItem item that will perform a close type operation on
the class T type object.

@see CleanupClosePushL()
*/
template <class T>
class CleanupClose
	{
public:
	inline static void PushL(T& aRef);
private:
	static void Close(TAny *aPtr);
	};
	
	

	
/**
@publishedAll
@released

Constructs and pushes a TCleanupItem object onto the cleanup stack.

The TCleanupItem encapsulates:

1. a reference aRef to the object of type class T which is to be cleaned up

2. an associated cleanup operation.

The cleanup operation is the private static function Close() of the templated
class CleanupClose and is invoked as a result of a subsequent call to
CleanupStack::PopAndDestroy().

CleanupClose::Close() is passed a pointer to the class T object to be cleaned
up, and the function implements cleanup by calling Close() on the passed object.
The class T object must, therefore, define and implement (or inherit)  a Close()
member function.

An example of its use:

@code
class RTestTwo;
	{
public :
    ...
	IMPORT_C void Close();
	...
	}
...
RTestTwo two;
CleanupClosePushL(two);
...
CleanupStack::PopAndDestroy(); // <--- results in Close() being called on "two".
......
@endcode

In practice, this type of cleanup operation is commonly applied to handles
to resources; if such handles are constructed on the program stack, then it is
important that such handles are closed.

@param aRef A reference to a class T type object for which the cleanup item
            is being created.

@see TCleanupItem
@see CleanupClose
@see CleanupStack::PopAndDestroy()
*/
template <class T>
inline void CleanupClosePushL(T& aRef);




/**
@publishedAll
@released

A utility class used by the templated function CleanupReleasePushL() to create 
a TCleanupItem item that will perform a release type operation on
the class T type object.

@see CleanupReleasePushL()
*/
template <class T>
class CleanupRelease
	{
public:
	inline static void PushL(T& aRef);
private:
	static void Release(TAny *aPtr);
	};
	
	

	
/**
@publishedAll
@released

Constructs and pushes a TCleanupItem object onto the cleanup stack.

The TCleanupItem encapsulates:

1. a reference aRef to the object of type class T which is to be cleaned up

2. an associated cleanup operation.

The cleanup operation is the private static function Release() of the
templated class CleanupRelease and is invoked as a result of
a subsequent call to CleanupStack::PopAndDestroy().

CleanupRelease::Release() is passed a pointer to the class T object to be cleaned
up, and the function implements cleanup by calling Release() on the passed object.
The class T object must, therefore, define and implement (or inherit) a Release()
member function.

An example of its use:

@code
class RTestThree;
	{
public :
    ...
	IMPORT_C void Release();
	...
	}
...
RTestThree three;
CleanupReleasePushL(three);
...
CleanupStack::PopAndDestroy(); // <--- results in Release() being called on "three".
......
@endcode

@param aRef A reference to a class T type object for which the cleanup item 
            is being created.

@see TCleanupItem
@see CleanupRelease
@see CleanupStack::PopAndDestroy()
*/
template <class T>
inline void CleanupReleasePushL(T& aRef);




class CConsoleBase;

/**
@publishedPartner
@released
*/
class Console
	{
public:
	IMPORT_C static CConsoleBase* NewL(const TDesC& aTitle,TSize aSize);
	};

#include <e32base.inl>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32base_private.h>
#endif

#endif //__E32BASE_H__
