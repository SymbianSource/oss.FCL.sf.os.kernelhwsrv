// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkern\nklib.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __NKLIB_H__
#define __NKLIB_H__
#include <e32err.h>
#include <nk_cpu.h>

#ifndef __KERNEL_MODE__
#error Including kernel header in user code
#endif

#if defined(__GCC32__)




/**
@publishedPartner
@released

64-bit signed integer type.
*/
typedef long long Int64;




/**
@publishedPartner
@released
	
64-bit unsigned integer type.	
*/
typedef unsigned long long Uint64;




#elif defined(__VC32__)
typedef __int64 Int64;
typedef unsigned __int64 Uint64;
#elif defined(__CW32__)
#pragma longlong on
typedef long long Int64;
typedef unsigned long long Uint64;
#endif




/**
@publishedPartner
@released
	
Defines a 64-bit time value. 
*/
typedef Int64 TTimeK;


/**
@internalComponent
*/
union TUint64HL
	{
	TUint64		i64;
	TUint32		i32[2];
	};


/**
@internalComponent

Ratio represented = iM*2^iX
e.g. 1.0 has iM=0x80000000, iX=-31
*/
struct SRatio
	{
	void Set(TUint32 aInt, TInt aDivisorExp=0);		// set this ratio to aInt/2^aDivisorExp
	TInt Reciprocal();								// this = 1/this
	TInt Mult(TUint32& aInt32);						// Multiply aInt32 by this ratio
//	TInt Mult(TUint64& aInt64);						// Multiply aInt64 by this ratio

	TUint32		iM;		// mantissa, normalised so bit 31=1
	TInt16		iX;		// -exponent.
	TUint8		iSpare1;
	TUint8		iSpare2;
	};

/**
@internalComponent

Ratio and inverse ratio
*/
struct SRatioInv
	{
	void Set(const SRatio* aR);

	SRatio		iR;
	SRatio		iI;
	};


#if defined(__VC32__) || defined(__CW32__)
extern "C"
/** @internalComponent */
__NORETURN__ void abort();
#endif

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
// Global placement operator new
/** @internalComponent */
inline TAny* operator new(TUint /*aSize*/, TAny* aBase) __NO_THROW
	{return aBase;}

// Global placement operator delete
/** @internalComponent */
inline void operator delete(TAny* /*aPtr*/, TAny* /*aBase*/) __NO_THROW
	{}
#endif //__PLACEMENT_NEW_INLINE

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
// Global placement operator new[]
/** @internalComponent */
inline TAny* operator new[](TUint /*aSize*/, TAny* aBase) __NO_THROW
	{return aBase;}

// Global placement operator delete[]
/** @internalComponent */
inline void operator delete[](TAny* /*aPtr*/, TAny* /*aBase*/) __NO_THROW
	{}
#endif //__PLACEMENT_VEC_NEW_INLINE

/******************************************************************************
 *
 * SIMPLE DOUBLY-LINKED CIRCULAR LIST
 *
 ******************************************************************************/

/**
	Macro to offset a SDblQueLink pointer back to the base of a class containing it
	@publishedPartner
	@released
*/
#define _LOFF(p,T,f) ((T*)(((TUint8*)(p))-_FOFF(T,f)))

#ifdef _DEBUG

/** @internalComponent */
#define KILL_LINK_VALUE (SDblQueLink*)0xdfdfdfdf

/** @internalComponent */
#define KILL_LINK(l)	(l)->iNext=(l)->iPrev=KILL_LINK_VALUE

#else

#define KILL_LINK(l)

#endif


#ifdef __ARMCC__
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif


/**
@publishedPartner
@released

An object that forms part of a doubly linked list.

SDblQueLink can also be embedded within another object so that that object
can form part of the doubly linked list.

@see SDblQue
*/
struct SDblQueLink
	{
	
#ifdef _DEBUG
    /**
    Default constructor; only defined for debug builds.
    
    It initialises the link pointers.
    */
	FORCE_INLINE SDblQueLink() {iNext=iPrev=NULL;}
#endif


    /**
    Removes this link item from the doubly linked list.
    
    @return A pointer to this link item.
    */
	FORCE_INLINE SDblQueLink* Deque()
		{
		SDblQueLink* next = iNext;
		SDblQueLink* prev = iPrev;
		next->iPrev=prev;
		prev->iNext=next;
		KILL_LINK(this);
		return this;
		}


    /**
    Inserts this link item into the list so that it precedes the specified link item.
    
    @param aL A pointer to the link item which is to follow this link item.
    */
	FORCE_INLINE void InsertBefore(SDblQueLink* aL)
		{
		SDblQueLink* prev = aL->iPrev;
		iNext=aL;
		iPrev=prev;
		prev->iNext=this;
		aL->iPrev=this;
		}
	
		
	/**
	Inserts this link item into the list so that it follows the specified link item.
    
    @param aL A pointer to the link item which is to precede this link item.
    */
	FORCE_INLINE void InsertAfter(SDblQueLink* aL)
		{
		SDblQueLink* next = aL->iNext;
		iPrev=aL;
		iNext=next;
		next->iPrev=this;
		aL->iNext=this;
		}
	
	
	/**
	Tests whether this is the only link item in the list.
	
	@return True, if this is the only link item in the list; false, otherwise.
    */
	FORCE_INLINE TBool Alone() const
		{ return (iNext==iPrev); }
    
    
    /**
    Pointer to the next link item in the list.
    */
	SDblQueLink* iNext;
	
	/**
    Pointer to the previous link item in the list.
    */
	SDblQueLink* iPrev;
	};




/**
@publishedPartner
@released

Anchor for a doubly linked list of SDblQueLink items.

@see SDblQueLink
*/
struct SDblQue
	{
	
	
	/**
	Default constructor.
	*/
	FORCE_INLINE SDblQue()
		{ iA.iNext=iA.iPrev=&iA; }
		
	
	/**
	Moves link items from the specified list onto this list, and clears the specified list
	
	@param aQ The source linked list. This list must not be empty.
	*/	
	inline SDblQue(SDblQue* aQ, TInt)		// move entries from aQ onto this queue and clear aQ - aQ must not be empty
		{ new (this) SDblQue(*aQ); iA.iNext->iPrev=&iA; iA.iPrev->iNext=&iA; new (aQ) SDblQue; }
		
		
	/**
	Tests whether this doubly linked list is empty.
	
	@return True, if the list is empty; false, otherwise.
	*/
	FORCE_INLINE TBool IsEmpty() const
		{ return (iA.iNext==&iA); }
	
		
    /**
    Gets a pointer to the first item in this doubly linked list.
    
    @return A pointer to the first item.
    */		
	FORCE_INLINE SDblQueLink* First() const
		{ return iA.iNext; }
	
		
    /**
    Gets a pointer to the last item in this doubly linked list.
    
    @return A pointer to the last item.
    */		
	FORCE_INLINE SDblQueLink* Last() const
		{ return iA.iPrev; }
	
		
	/**
	Adds the specified link item onto the end of this doubly linked list.
	
	@param aL A pointer to the link item to be added.
	*/
	FORCE_INLINE void Add(SDblQueLink* aL)
		{
		SDblQueLink* prev = iA.iPrev;
		aL->iNext=&iA;
		aL->iPrev=prev;
		prev->iNext=aL;
		iA.iPrev=aL;
		}
	
		
	/**
	Adds the specified link item onto the front of this doubly linked list.
	
	@param aL A pointer to the link item to be added.
	*/
	FORCE_INLINE void AddHead(SDblQueLink* aL)
		{
		SDblQueLink* next = iA.iNext;
		aL->iNext=next;
		aL->iPrev=&iA;
		next->iPrev=aL;
		iA.iNext=aL;
		}
	
		
	/**
    Removes the last link item from the linked list and adds it to the front
    of the list. 
	*/
	inline void Rotate()
		{ SDblQueLink* pL=iA.iPrev; pL->Deque(); AddHead(pL); }
		
		
	/**
	Gets the first link item in the linked list.
	
	@return The first link item in the list; NULL, if the list is empty.
	*/
	inline SDblQueLink* GetFirst()
		{ if (IsEmpty()) return NULL; else return First()->Deque(); }


	/**
	Gets the last link item in the linked list.
	
	@return The last link item in the list; NULL, if the list is empty.
	*/
	inline SDblQueLink* GetLast()
		{ if (IsEmpty()) return NULL; else return Last()->Deque(); }


	/**
	Appends entries from the specified linked list onto this list, and clears
	the specified link list anchor.
	
	@param aQ The source linked list.
	*/
	inline void MoveFrom(SDblQue* aQ)	// append entries from aQ onto this queue and clear aQ
		{ if (!aQ->IsEmpty())
			{iA.iPrev->iNext=aQ->iA.iNext; aQ->iA.iNext->iPrev=iA.iPrev; iA.iPrev=aQ->iA.iPrev; iA.iPrev->iNext=&iA; new (aQ) SDblQue; }
		}


    /**
    The anchor point for the doubly linked list.
    */
	SDblQueLink iA;
	};




/******************************************************************************
 *
 * ITERABLE DOUBLY-LINKED CIRCULAR LIST
 *
 ******************************************************************************/

/**
@internalComponent

An object that forms part of an iterable doubly linked list.

SIterDQLink can also be embedded within another object so that that object
can form part of the doubly linked list.

@see SIterDQ
*/
struct SIterDQ;
struct SIterDQIterator;
struct SIterDQLink
	{
	
    /**
    Default constructor; only defined for debug builds.
    
    It initialises the link pointers.
    */
	FORCE_INLINE SIterDQLink() {iNext=iPrev=0;}

	enum
		{
		ENonAddressMask=3u,
		EIterator=1u,
		EAnchor=2u,
		};

	FORCE_INLINE SIterDQLink* Next() const
		{ return (SIterDQLink*)(iNext & ~ENonAddressMask); }

	FORCE_INLINE SIterDQLink* Prev() const
		{ return (SIterDQLink*)(iPrev & ~ENonAddressMask); }

	FORCE_INLINE TBool IsObject() const
		{ return !(iNext & ENonAddressMask); }

	FORCE_INLINE TBool IsIterator() const
		{ return iNext & EIterator; }

	FORCE_INLINE TBool IsAnchor() const
		{ return iNext & EAnchor; }

	FORCE_INLINE void SetNext(SIterDQLink* aNext)
		{ iNext = (iNext & ENonAddressMask) | (TUintPtr(aNext) & ~ENonAddressMask); }

	FORCE_INLINE void SetPrev(SIterDQLink* aPrev)
		{ iPrev = (iPrev & ENonAddressMask) | (TUintPtr(aPrev) & ~ENonAddressMask); }

    /**
    Removes this link item from the doubly linked list.
    
    @return A pointer to this link item.
    */
	FORCE_INLINE SIterDQLink* Deque()
		{
		SIterDQLink* next = Next();
		SIterDQLink* prev = Prev();
		next->SetPrev(prev);
		prev->SetNext(next);
#ifdef _DEBUG
		SetNext((SIterDQLink*)4);
		SetPrev((SIterDQLink*)4);
#endif
		return this;
		}


    /**
    Inserts this link item into the list so that it precedes the specified link item.
    
    @param aL A pointer to the link item which is to follow this link item.
    */
	FORCE_INLINE void InsertBefore(SIterDQLink* aL)
		{
		SIterDQLink* prev = aL->Prev();
		SetNext(aL);
		SetPrev(prev);
		prev->SetNext(this);
		aL->SetPrev(this);
		}
	
		
	/**
	Inserts this link item into the list so that it follows the specified link item.
    
    @param aL A pointer to the link item which is to precede this link item.
    */
	FORCE_INLINE void InsertAfter(SIterDQLink* aL)
		{
		SIterDQLink* next = aL->Next();
		SetPrev(aL);
		SetNext(next);
		next->SetPrev(this);
		aL->SetNext(this);
		}
	
	
	/**
	Tests whether this is the only link item in the list.
	
	@return True, if this is the only link item in the list; false, otherwise.
    */
	FORCE_INLINE TBool Alone() const
		{ return (iNext==iPrev); }
    
private:
	/**
	Bits 2-31 = Address of the next link item in the list.
	Bit 0 = 1 for iterator, 0 for object
	*/
	TUintPtr iNext;

	/**
	Bits 2-31 = Address of the previous link item in the list.
	Bit 0 = 1 for iterator, 0 for object
	*/
	TUintPtr iPrev;

	friend struct SIterDQ;
	friend struct SIterDQIterator;
	};




/**
@internalComponent

Anchor for an iterable circular doubly linked list of SIterDQLink items.

@see SIterDQLink
*/
struct SIterDQ
	{
	
	/**
	Default constructor.
	*/
	FORCE_INLINE SIterDQ()
		{ iA.iNext = iA.iPrev = TUintPtr(&iA)|SIterDQLink::EAnchor; }
		
	
	/**
	Moves link items from the specified list onto this list, and clears the specified list
	
	@param aQ The source linked list. This list must not be empty.
	*/	
	inline SIterDQ(SIterDQ* aQ, TInt)		// move entries from aQ onto this queue and clear aQ - aQ must not be empty
		{ iA.iNext=aQ->iA.iNext; iA.iPrev=aQ->iA.iPrev; First()->SetPrev(&iA); Last()->SetNext(&iA); new (aQ) SIterDQ; }
		
		
	/**
	Tests whether this doubly linked list is empty.
	
	@return True, if the list is empty; false, otherwise.
	*/
	FORCE_INLINE TBool IsEmpty() const
		{ return (iA.iNext &~ SIterDQLink::ENonAddressMask) == TUintPtr(&iA); }
	
		
    /**
    Gets a pointer to the first item in this doubly linked list.
    
    @return A pointer to the first item.
    */		
	FORCE_INLINE SIterDQLink* First() const
		{ return iA.Next(); }
	
		
    /**
    Gets a pointer to the last item in this doubly linked list.
    
    @return A pointer to the last item.
    */		
	FORCE_INLINE SIterDQLink* Last() const
		{ return iA.Prev(); }
	
		
	/**
	Adds the specified link item onto the end of this doubly linked list.
	
	@param aL A pointer to the link item to be added.
	*/
	FORCE_INLINE void Add(SIterDQLink* aL)
		{
		aL->InsertBefore(&iA);
		}
	
		
	/**
	Adds the specified link item onto the front of this doubly linked list.
	
	@param aL A pointer to the link item to be added.
	*/
	FORCE_INLINE void AddHead(SIterDQLink* aL)
		{
		aL->InsertAfter(&iA);
		}
	
		
	/**
	Gets the first link item in the linked list.
	
	@return The first link item in the list; NULL, if the list is empty.
	*/
	inline SIterDQLink* GetFirst()
		{ if (IsEmpty()) return NULL; else return First()->Deque(); }


	/**
	Gets the last link item in the linked list.
	
	@return The last link item in the list; NULL, if the list is empty.
	*/
	inline SIterDQLink* GetLast()
		{ if (IsEmpty()) return NULL; else return Last()->Deque(); }


	/**
	Appends entries from the specified linked list onto this list, and clears
	the specified link list anchor.
	
	@param aQ The source linked list.
	*/
	inline void MoveFrom(SIterDQ* aQ)	// append entries from aQ onto this queue and clear aQ
		{ if (!aQ->IsEmpty())
			{
			SIterDQLink* last = Last();		// last current
			SIterDQLink* fx = aQ->First();	// first extra
			SIterDQLink* lx = aQ->Last();	// last extra
			last->SetNext(fx);
			fx->SetPrev(last);
			iA.SetPrev(lx);
			lx->SetNext(&iA);
			new (aQ) SIterDQ;
			}
		}

private:
    /**
    The anchor point for the doubly linked list.
    */
	SIterDQLink	iA;
	};


#ifdef __VC32__
#pragma warning( disable : 4127 )	// conditional expression is constant
#endif

/**
@internalComponent

Iterator for an iterable circular doubly linked list of SIterDQLink items.

@see SIterDQLink
@see SIterDQ
*/
struct SIterDQIterator : public SIterDQLink
	{

	/**
	Default constructor.

	Iterator starts out not attached to any queue
	*/
	FORCE_INLINE SIterDQIterator()
		{ iNext = iPrev = SIterDQLink::EIterator; }

	/**
	Destructor ensures iterator detached before destruction
	*/
	FORCE_INLINE ~SIterDQIterator()
		{
#ifdef _DEBUG
		if (iNext != SIterDQLink::EIterator) { __crash(); }
#endif
		}

	/**
	Detach the iterator if it is currently attached to a queue
	*/
	FORCE_INLINE void Detach()
		{ if (Next()) {Deque(); SetNext(0);} }

	/**
	Attach the iterator to a queue at the beginning.
	*/
	FORCE_INLINE void Attach(SIterDQ* aQ)
		{
#ifdef _DEBUG
		if (iNext != SIterDQLink::EIterator) { __crash(); }
#endif
		aQ->AddHead(this);
		}

	/**
	Step the iterator over the next object.
	Return KErrNone if we stepped over an object.
	Return KErrEof if we reached the end of the list.
	Return KErrGeneral if we stepped over aMaxSteps other iterators.
	In first case aObj is set to point to the object stepped over.
	In other cases aObj is set to NULL.
	*/
	TInt Step(SIterDQLink*& aObj, TInt aMaxSteps=0);	// 0 means use default value

	};

#ifdef __VC32__
#pragma warning( default : 4127 )	// conditional expression is constant
#endif



/******************************************************************************
 *
 * ORDERED DOUBLY-LINKED CIRCULAR LIST
 *
 ******************************************************************************/

/**
@publishedPartner
@released

An object that forms part of a doubly linked list arranged
in descending key order.

@see SOrdQue
*/
struct SOrdQueLink : public SDblQueLink
	{
	
	
	/**
	The key value used to order the link item.
	*/
	TInt iKey;
	};


/**
@publishedPartner
@released

Anchor for a doubly linked list of SOrdQueLink items.

The items in this linked list are in descending key order.

@see SOrdQueLink
*/
struct SOrdQue : public SDblQue
	{
	
	
	/**
	Adds the specified link item into this doubly linked list so that
	the list remains in descending key order.
	
	@param aL A pointer to the link item to be added.
	*/
	inline void Add(SOrdQueLink* aL)
		{
		SOrdQueLink* pQ=(SOrdQueLink*)iA.iNext;
		TInt k=aL->iKey;
		while(pQ!=&iA && (pQ->iKey>=k)) pQ=(SOrdQueLink*)pQ->iNext;
		aL->InsertBefore(pQ);
		}
	};



/******************************************************************************
 *
 * DELTA-ORDERED DOUBLY-LINKED CIRCULAR LIST
 *
 ******************************************************************************/

/**
@publishedPartner
@released

An object that forms part of a doubly linked list arranged
in 'delta' order.

The item represents some value that is an increment, or delta,
on the value represented by a preceding element.

@see SDeltaQue
*/
struct SDeltaQueLink : public SDblQueLink
	{
	/**
	The delta value.
	*/
	TInt iDelta;
	};




/**
@publishedPartner
@released

Anchor for a doubly linked list of SDeltaQueLink items.

An item in this linked list represents a value that is an increment,
or a delta, on the value represented by a preceding element.
The list is ordered so that the head of the queue represents a nominal zero point.

@see SDeltaQueLink
*/
struct SDeltaQue : public SDblQue
	{
	
	
	/**
	Gets the delta value of the first link item in the list.

    @return The delta value.
	*/
	inline TInt FirstDelta() const
		{return ((SDeltaQueLink*)First())->iDelta;}
		
		
    /**
    Decrements the delta value of the first item in the list by the specified value.

    @param aCount The amount by which the delta value is to be reduced.

    @return True, if the resulting delta value is negative or zero;
            false, if the value is positive.
    */		
	inline TBool CountDown(TInt aCount)
		{SDeltaQueLink& l=*(SDeltaQueLink*)First(); return((l.iDelta-=aCount)<=0);}
		
	
	/**
	Adds the specified list item, having the specified 'distance' from
	the nominal zero point, into the list.

    The item is added into the list, the adjacent delta values are adjusted,
    and a suitable delta value assigned to the new item so that
    the new item is at the specified 'distance' from the nominal zero point.

    @param aL     The item to be inserted. 
    @param aDelta The 'distance' of the item from the nominal zero point.
	*/
	inline void Add(SDeltaQueLink* aL, TInt aDelta)
		{
		SDeltaQueLink* pQ=(SDeltaQueLink*)iA.iNext;
		while(pQ!=&iA && aDelta>=pQ->iDelta)
			{ aDelta-=pQ->iDelta; pQ=(SDeltaQueLink*)pQ->iNext; }
		aL->iDelta=aDelta;
		aL->InsertBefore(pQ);
		if (pQ!=&iA) pQ->iDelta-=aDelta;
		}
				
		
	/**
	Removes the specified link item from the list.
	
	The delta value of the item following the removed item is adjusted
	so that its 'distance' from the nominal zero point remains the same.
	
	@param aL The list item to be removed.
	
	@return A pointer to the item removed from the queue.
	*/
	inline SDeltaQueLink* Remove(SDeltaQueLink* aL)
		{
		if (aL->iNext!=&iA)
			{
			SDeltaQueLink& next=*(SDeltaQueLink*)aL->iNext;
			next.iDelta+=aL->iDelta;
			}
		return (SDeltaQueLink*)aL->Deque();
		}
		
		
    /**
    Removes the first item from the linked list if its delta value
    is zero or negative.
    
    @return A pointer to the item removed from the linked list.
            This is NULL, if the first element has a positive delta value,
            and has not been removed from the list.
    */		
	inline SDeltaQueLink* RemoveFirst()
		{
		SDeltaQueLink& l=*(SDeltaQueLink*)First();
		if (l.iDelta<=0)
			return Remove(&l);
		return NULL;
		}
	};



/******************************************************************************
 *
 * O(1) PRIORITY ORDERED LIST
 *
 ******************************************************************************/

/**
@publishedPartner
@released

An object that forms part of a TPriList, priority ordered lists.

@see TPriListBase
@see TPriList
*/
class TPriListLink : public SDblQueLink
	{
public:

    
    /**
    Default constructor.
    
    Sets the priority value to zero.
    */
	inline TPriListLink() : iPriority(0) {}
	
	
    /**
    Constructor.
    
    Sets the priority to the specified value.
    
    @param aPriority The priority value.
    */
	inline TPriListLink(TInt aPriority) : iPriority((TUint8)aPriority) {}
	
	
	/**
	Tests whether this is a solitary link item.
	
	@return True, if this is a solitary link item; false, otherwise. 
	*/
	inline TBool Alone() const
		{ return (iNext==(SDblQueLink*)this); }
public:

    /** 
    The priority value.
    */
	TUint8 iPriority;
	
	/**
	Reserved for future use.
	*/
	TUint8 iSpare1;
	
		
	/**
	Reserved for future use.
	*/
	TUint8 iSpare2;
	
		
	/**
	Reserved for future use.
	*/
	TUint8 iSpare3;
	};




/**
@publishedPartner
@released

Base class for a TPriList, priority ordered lists.

@see TPriListLink
@see TPriList
*/
class TPriListBase
	{
public:
	IMPORT_C TPriListBase(TInt aNumPriorities);
	IMPORT_C TInt HighestPriority();
	IMPORT_C TPriListLink* First();
	IMPORT_C void Add(TPriListLink* aLink);
	IMPORT_C void AddHead(TPriListLink* aLink);
	IMPORT_C void Remove(TPriListLink* aLink);
	IMPORT_C void ChangePriority(TPriListLink* aLink, TInt aNewPriority);
	
	/**
	Tests whether there are any non-empty lists.
		
	@return True, if there are non-empty lists; false, if all lists are empty.
	*/
	inline TBool NonEmpty() const
		{ return iPresent[0]|iPresent[1]; }
		
	/**
	Tests whether there are any non-empty lists.
		
	@return True, if all lists are empty
	*/
	inline TBool IsEmpty() const
		{ return !iPresent[0] && !iPresent[1]; }
		
	/**
	Tests whether any linked list with priority greater than p is non-empty.

	@param p The priority value (0-63).

	@return True, if any list with priority greater than p is non-empty; false, otherwise.	
	*/
	inline TBool operator>(TInt p) const
		{ return ((p<32) ? (iPresent[1] | (iPresent[0]>>p)>>1) : (iPresent[1]>>(p-32))>>1 ); }
public:

    /**
    64-bit mask to indicate which list is non-empty.

    Bit n in the mask is set if and only if the linked list for priority n is non-empty.
    */
	union
		{
		TUint iPresent[2];
		TUint64 iPresent64;
		};
	
	/**
	Pointer to the first linked list.
	*/
	SDblQueLink* iQueue[1];
	};




template<class T, int n>
/**
@publishedPartner
@released

Anchor for a collection of doubly linked lists, where each list
corresponds to a priority value.

The lists are ordered by priority value, but items within
a list are in chronological order.

The number of lists is defined by the template integer parameter,
and each item in each list is of a class type defined by the template class parameter.
The number of lists must be between 1 and 64 inclusive.

@see TPriListLink
*/
class TPriList : public TPriListBase
	{
public:
    /**
    Constructor.
    */
	inline TPriList() : TPriListBase(n) {}
	
	
	/**
	Finds the highest priority item present on a priority list.
	If multiple items at the same priority are present, return the first to be
	added in chronological order.

	@return	a pointer to the item or NULL if the list is empty.
	*/
	inline T* First() { return (T*)TPriListBase::First(); }
private:
	SDblQueLink* iExtraQueues[n-1];
	};



/** Base for variant interface block
@internalTechnology
@prototype
*/
struct SInterfaceBlockBase
	{
	TUint32	iVer;	// version number
	TUint32	iSize;	// size in bytes
	};
#endif
