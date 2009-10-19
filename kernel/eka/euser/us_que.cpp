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
// e32\euser\us_que.cpp
// 
//

#include "us_std.h"

EXPORT_C void TSglQueLink::Enque(TSglQueLink* aLink)
//
// Enque this after aLink.
//
	{

	iNext=aLink->iNext;
	aLink->iNext=this;
	}




EXPORT_C void TDblQueLinkBase::Enque(TDblQueLinkBase* aLink)
/** 
Inserts this link object after the specified link object.

The specified link object must already be in the doubly linked list.

The function cannot be used to insert a list element into the beginning or 
end of a doubly linked list; this is handled by the TDblQue::AddFirst() 
and TDblQue::AddLast() functions.

@param aLink A pointer to the link object embedded within the list element 
             to which this link object is to be connected. It must not be NULL.
              
@see TDblQue
*/
	{

	iNext=aLink->iNext;
	iPrev=aLink;
	aLink->iNext->iPrev=this;
	aLink->iNext=this;
	}




EXPORT_C void TDblQueLinkBase::AddBefore(TDblQueLinkBase* aLink)
/**
Inserts this link object before the specified link object.

The specified link object must already be in the doubly linked list.

The function cannot be used to insert a list element into the beginning or 
end of a doubly linked list; this is handled by the TDblQue::AddFirst() 
and TDblQue::AddLast() functions.

@param aLink A pointer to the link object embedded within the list element 
             to which this link object is to be connected. It must not be NULL.
              
@see TDblQue
*/
	{

	iNext=aLink;
	iPrev=aLink->iPrev;
	aLink->iPrev->iNext=this;
	aLink->iPrev=this;
	}




EXPORT_C void TDblQueLink::Deque()
/**
Removes this link object from the doubly linked list.

In effect, this removes the list element that acts as host to this link object
from the doubly linked list.

The link object can be any in the doubly linked list.

It is safe to use this method on an object which has already been removed from the list.

@post iNext member is set to NULL
*/
	{

	if (iNext)
		{
	    iPrev->iNext=iNext;
		iNext->iPrev=iPrev;
		iNext=NULL;
		}
	}




EXPORT_C TSglQueBase::TSglQueBase()
	: iHead(NULL),iLast((TSglQueLink*)&iHead),iOffset(0)
/**
Default constructor.

It sets:

1. iHead to Null.

2. iLast to point to the head  of queue.

3. iOffset to zero.

@see iHead
@see iLast
@see iOffset
*/
	{}




EXPORT_C TSglQueBase::TSglQueBase(TInt aOffset)
	: iHead(NULL),iLast((TSglQueLink*)&iHead),iOffset(aOffset)
/**
Constructor with specified offset.

It sets:

1. iHead to Null

2. iLast to point to the head of queue.

3. iOffset to the specified value.

@param aOffset The offset of a link object within an element.

@panic USER 75, if aOffset is not divisible by four

@see iHead
@see iLast
@see iOffset
*/
	{

	__ASSERT_ALWAYS(iOffset%4==0,Panic(ESQueOffsetNotAligned));
	}




EXPORT_C TBool TSglQueBase::IsEmpty() const
/**
Tests whether the singly linked list is empty, i.e. has no list elements.

@return True, if the singly linked list is empty; false, otherwise.
*/
	{

	return(iHead==NULL);
	}




EXPORT_C void TSglQueBase::SetOffset(TInt aOffset)
/**
Sets the offset of the link object from the start of a singly linked
list element.

@param aOffset The offset of the link object from the start of a singly linked 
               list element.
               
@panic USER 75, if aOffset is not divisible by four.              

@see TSglQue
*/
	{

	__ASSERT_ALWAYS(iOffset%4==0,Panic(ESQueOffsetNotAligned));
	iOffset=aOffset;
	}





EXPORT_C void TSglQueBase::Reset()
/**
Empties the singly linked list.

After a call to this function, there are no elements queued from the header; 
the elements are orphaned. Special care must be taken when list elements are 
CBase derived objects, i.e. are allocated on the heap.
*/
	{
	
	iHead=NULL;
	iLast=(TSglQueLink*)&iHead;
	}





EXPORT_C void TSglQueBase::DoAddFirst(TAny* aPtr)
/**
Implements the insertion of a list element at the front of the singly linked 
list.

This function is called by TSglQue::AddFirst().

@param aPtr An untyped pointer to the element to be inserted.

@see TSglQue::AddFirst
*/
	{

	TSglQueLink* pL=PtrAdd((TSglQueLink*)aPtr,iOffset);
	pL->Enque((TSglQueLink*)&iHead);
	if (iLast==(TSglQueLink*)(&iHead))
		iLast=pL;
	}




EXPORT_C void TSglQueBase::DoAddLast(TAny* aPtr)
/**
Implements the insertion of a list element at the back of the singly linked 
list.

This function is called by TSglQue::AddLast().

@param aPtr An untyped pointer to the element to be inserted.

@see TSglQue::AddLast
*/
	{

	TSglQueLink* pL=PtrAdd((TSglQueLink*)aPtr,iOffset);
	pL->Enque(iLast);
	iLast=pL;
	}




EXPORT_C void TSglQueBase::DoRemove(TAny* aPtr)
/**
Implements the removal of a list element from the singly linked list.

This function is called by TSglQue::Remove().

@param aPtr An untyped pointer to the element to be removed.

@see TSglQue::Remove
*/
	{

	TSglQueLink* pP=(TSglQueLink*)(&iHead);
	TSglQueLink* pL=PtrAdd((TSglQueLink*)aPtr,iOffset);
	TSglQueLink* pN=pP->iNext;
	while (pN)
		{
		if (pN==pL)
			{
			pP->iNext=pN->iNext;
			if (iLast==pL)
				{
				iLast=pP;
				if (iLast==NULL)
					iLast=(TSglQueLink*)(&iHead);
				}
			return;
			}
		pP=pN;
		pN=pP->iNext;
		}
	Panic(ESQueLinkNotQueued);
	}




#pragma warning( disable : 4705 )	// statement has no effect
EXPORT_C TDblQueBase::TDblQueBase()
	: iOffset(0)
/**
Default constructor.

It sets: 

1. iHead to point to this object in both the forwards and backwards direction.

2. iOffset to zero.

@see iHead
@see iOffset
*/
	{

	iHead.iNext=iHead.iPrev=(&iHead);
	}




EXPORT_C TDblQueBase::TDblQueBase(TInt aOffset)
	: iOffset(aOffset)
/**
Constructor with specified offset.

It sets:

1. iHead to point to this object in both the forwards and backwards direction.

2. iOffset to the specified value.

@param aOffset The offset of a link object within an element.

@panic USER 78, if aOffset is not divisible by four

@see iHead
@see iOffset
*/
	{

	__ASSERT_ALWAYS(iOffset%4==0,Panic(ETQueOffsetNotAligned));
	iHead.iNext=iHead.iPrev=(&iHead);
	}
#pragma warning( default : 4705 )




EXPORT_C TBool TDblQueBase::IsEmpty() const
/**
Tests whether the doubly linked list is empty, i.e. has no list elements.

@return True, if the doubly linked list is empty; false, otherwise.
*/
	{

	return((const TDblQueLinkBase*)iHead.iNext==(&iHead));
	}




EXPORT_C void TDblQueBase::SetOffset(TInt aOffset)
/**
Sets the offset of the link object from the start of a doubly linked list element.

@param aOffset The offset of the link object from the start of a doubly linked 
               list element.
               
@panic USER 78, if aOffset is not divisible by four.               
               
@see TDblQue
*/
	{

	__ASSERT_ALWAYS(iOffset%4==0,Panic(ETQueOffsetNotAligned));
	iOffset=aOffset;
	}




EXPORT_C void TDblQueBase::Reset()
/**
Empties the doubly linked list.

After a call to this function, there are no elements queued from the header; 
the elements are orphaned. Special care must be taken when list elements are 
CBase derived objects, i.e. are allocated on the heap.
*/
	{
	
	iHead.iNext=iHead.iPrev=(&iHead);
	}




EXPORT_C void TDblQueBase::DoAddFirst(TAny* aPtr)
/**
Implements the insertion of the specified list element at the front of the
doubly linked list.

This function is called by TDblQue::AddFirst().

@param aPtr An untyped pointer to the element to be inserted.

@see TDblQue::AddFirst
*/
	{

	PtrAdd((TDblQueLinkBase*)aPtr,iOffset)->Enque(&iHead);
	}




EXPORT_C void TDblQueBase::DoAddLast(TAny* aPtr)
/**
Implements the insertion of the specified list element at the back of the
doubly linked list.

This function is called by TDblQue::AddLast().

@param aPtr An untyped pointer to the element to be inserted.

@see TDblQue::AddLast*/
	{

	PtrAdd((TDblQueLinkBase*)aPtr,iOffset)->Enque(iHead.iPrev);
	}




EXPORT_C void TDblQueBase::DoAddPriority(TAny* aPtr)
/**
Implements the insertion of the specified list element in priority order.

This function is called by TPriQue::Add().

@param aPtr An untyped pointer to the element to be inserted.

@see TPriQue::Add
*/
	{

	TPriQueLink* pN=(TPriQueLink*)iHead.iNext;
	TPriQueLink* pI=PtrAdd((TPriQueLink*)aPtr,iOffset);
	TInt p=pI->iPriority;
	while (pN!=(TPriQueLink*)&iHead && p<=pN->iPriority)
		pN=(TPriQueLink*)pN->iNext;
	pI->Enque(pN->iPrev);
	}




EXPORT_C TDeltaQueBase::TDeltaQueBase()
	: iFirstDelta(NULL)
/**
Default constructor.

It sets iFirstDelta to NULL.

@see TDeltaQueBase::iFirstDelta
*/
	{
	}




EXPORT_C TDeltaQueBase::TDeltaQueBase(TInt aOffset)
	: TDblQueBase(aOffset),iFirstDelta(NULL)
/**
Constructor with specified offset.

It sets:

1. iFirstDelta to NULL

2. TDblQueBase::iOffset to the specified value, through a call to the
   base class  constructor.

@param aOffset The offset of a link object within an element.

@see TDeltaQueBase::iFirstDelta
@see TDblQueBase::iOffset
*/
	{
	}




EXPORT_C void TDeltaQueBase::Reset()
/**
Empties the doubly linked list, and resets the first delta pointer.
*/
	{
	
	TDblQueBase::Reset();
	iFirstDelta=NULL;
	}




EXPORT_C TBool TDeltaQueBase::FirstDelta(TInt& aValue)
/**
Gets the delta value of the first list element.

@param aValue On return, contsins the delta value of the first element.
              Note that this remains unchanged if there is no first element.
              
@return True, if there is a first element; false, otherwise.
*/
    {
    if (iFirstDelta)
        {
        aValue=*iFirstDelta;
        return(ETrue);
        }
    return(EFalse);
    }




EXPORT_C TBool TDeltaQueBase::CountDown()
/**
Decrements the delta value of the first element by one, and returns true if 
the result is negative or zero.

@return True, if the resulting delta value is negative or zero; false, if 
        the value is positive, or there is no first element.
*/
	{

    return(CountDown(1));
	}




EXPORT_C TBool TDeltaQueBase::CountDown(TInt aValue)
/**
Decrements the delta value of the first element by the specified value, and 
returns true if the result is negative or zero.

@param aValue The amount by which the delta value is to be reduced.

@return True, if the resulting delta value is negative or zero; false, if the 
        value is positive, or there is no first element.
*/
	{

	if (iFirstDelta)
		{
		(*iFirstDelta)-=aValue;
		if (*iFirstDelta<=0)
			return(ETrue);
		}
	return(EFalse);
	}




EXPORT_C void TDeltaQueBase::DoAddDelta(TAny* aPtr,TInt aDelta)
/**
Implements the addition of the specified list element into the list.

This function is called by TDeltaQue::Add().

@param aPtr   Pointer to the list element to be inserted.
@param aDelta The 'distance' from the nominal zero point.

@see TDeltaQue::Add
*/
	{

	TDeltaQueLink* pD=(TDeltaQueLink*)iHead.iNext;
	TDeltaQueLink* pI=PtrAdd((TDeltaQueLink*)aPtr,iOffset);
	while (pD!=(TDeltaQueLink*)&iHead && aDelta>=pD->iDelta)
		{
		aDelta-=pD->iDelta;
		pD=(TDeltaQueLink*)pD->iNext;
		}
	pI->iDelta=aDelta;
	pI->Enque(pD->iPrev);
	if (pI->iNext!=&iHead)
		pD->iDelta-=aDelta;
	iFirstDelta=(&((TDeltaQueLink*)iHead.iNext)->iDelta);
	}




EXPORT_C void TDeltaQueBase::DoRemove(TAny* aPtr)
/**
Implements the removal of the specified list element from the list.

This function is called by TDeltaQue::Remove().

@param aPtr Pointer to the list element to be removed.

@see TDeltaQue::Remove
*/
	{

	TDeltaQueLink* pI=PtrAdd((TDeltaQueLink*)aPtr,iOffset);
	TDeltaQueLink* pN=(TDeltaQueLink*)pI->iNext;
	if (pN!=(TDeltaQueLink*)&iHead)
		pN->iDelta+=pI->iDelta;
	((TDblQueLink*)pI)->Deque();
	iFirstDelta=(iHead.iNext!=(&iHead) ? &((TDeltaQueLink*)iHead.iNext)->iDelta : NULL);
	}




EXPORT_C TAny* TDeltaQueBase::DoRemoveFirst()
/**
Implements the removal of the first list element from the linked list if its 
delta value is zero or negative.

This function is called by TDeltaQue::RemoveFirst().

@return A pointer to the element removed from the linked list. This is NULL, 
        if the first element has a positive delta value.

@see TDeltaQue::RemoveFirst
*/
	{

	TDeltaQueLink* pN=(TDeltaQueLink*)iHead.iNext;
	if (pN!=(TDeltaQueLink*)&iHead && pN->iDelta<=0)
		{
		pN=PtrSub(pN,iOffset);
		DoRemove(pN);
		return(pN);
		}
	return(NULL);
	}




EXPORT_C TSglQueIterBase::TSglQueIterBase(TSglQueBase& aQue)
//
// Cosntructor.
//
	: iOffset(aQue.iOffset),iHead((TSglQueLink*)&aQue.iHead),iNext(aQue.iHead)
	{}




EXPORT_C void TSglQueIterBase::SetToFirst()
/**
Sets the iterator to point to the first element in the singly linked list. 

The function can be called to re-set the pointer at any time during the iterator's 
existence.

The function can be called even if the list has no elements.
*/
	{

	iNext=iHead->iNext;
	}




EXPORT_C void TSglQueIterBase::DoSet(TAny* aLink)
//
// Start the iterator at aLink.
//
	{

	iNext=PtrAdd((TSglQueLink*)aLink,iOffset);
	}

EXPORT_C TAny* TSglQueIterBase::DoPostInc()
//
// Return the current pointer and increment.
//
	{

	TAny* pN=iNext;
	if (pN==NULL)
		return NULL;
	iNext=iNext->iNext;
	return(PtrSub(pN,iOffset));
	}

EXPORT_C TAny* TSglQueIterBase::DoCurrent()
//
// Return the current pointer.
//
	{

	return(iNext==NULL ? NULL : PtrSub((TAny*)iNext,iOffset));
	}




EXPORT_C TDblQueIterBase::TDblQueIterBase(TDblQueBase& aQue)
	: iOffset(aQue.iOffset),iHead(&aQue.iHead),iNext(aQue.iHead.iNext)
/**
Constructs the iterator for the specified doubly linked list.

@param aQue A reference to a doubly linked list header.
*/
	{}




EXPORT_C void TDblQueIterBase::SetToFirst()
/**
Sets the iterator to point to the first element in the doubly linked list. 

The function can be called to re-set the pointer at any time during the
iterator's  existence.

The function can be called even if the list has no elements.
*/
	{

	iNext=iHead->iNext;
	}




EXPORT_C void TDblQueIterBase::SetToLast()
/**
Sets the iterator to point to the last element in the doubly linked list. The 
function can be called to re-set the pointer at any time during the
iterator's existence.

The function can be called even if the list has no elements.
*/
	{

	iNext=iHead->iPrev;
	}




EXPORT_C void TDblQueIterBase::DoSet(TAny* aLink)
/**
Sets the iterator to point to a specific element in the list.

The function is an implementation for TDblQueIter::Set().

@param aLink A pointer to the current list element.

@see TDblQueIter::Set
*/
	{

	iNext=PtrAdd((TDblQueLinkBase*)aLink,iOffset);
	}




EXPORT_C TAny* TDblQueIterBase::DoPostInc()
/**
Gets the current item and then moves to the next item.

The function is an implementation for TDblQueIter::operator++().

@return A pointer to the current list element.

@see TDblQueIter::operator++
*/
	{

	if (iNext==iHead)
		return(NULL);
	__ASSERT_DEBUG((iNext->iNext!=NULL)&&(iNext->iPrev!=NULL),Panic(ETQueLinkHasBeenRemoved));
	TAny* p=PtrSub(iNext,iOffset);
	iNext=iNext->iNext;
	return(p);
	}




EXPORT_C TAny* TDblQueIterBase::DoPostDec()
/**
Gets the current item and then moves to the previous item.

The function is an implementation for TDblQueIter::operator--().

@return A pointer to the current list element.

@see TDblQueIter::operator--
*/
	{

	if (iNext==iHead)
		return(NULL);
	__ASSERT_DEBUG((iNext->iNext!=NULL)&&(iNext->iPrev!=NULL),Panic(ETQueLinkHasBeenRemoved));
	TAny* p=PtrSub(iNext,iOffset);
	iNext=iNext->iPrev;
	return(p);
	}




EXPORT_C TAny* TDblQueIterBase::DoCurrent()
/**
Gets the current item in the queue.

The function is an implementation for TDblQueIter::operator T*().

@return A pointer to the current list element.

@see TDblQueIter::operator T*
*/
	{

	if (iNext==iHead)
		return(NULL);
	__ASSERT_DEBUG((iNext->iNext!=NULL)&&(iNext->iPrev!=NULL),Panic(ETQueLinkHasBeenRemoved));
	return(PtrSub(iNext,iOffset));
	}




EXPORT_C void TDblQueBase::__DbgTestEmpty() const
/**
Tests whether the queue is empty.

The function is implemented as an __ASSERT_DEBUG.

@panic USER 79, if the assertion fails.
*/
	{

	__ASSERT_DEBUG((((TDblQueLink*)iHead.iNext)!=&iHead)&&(((TDblQueLink*)iHead.iPrev)!=&iHead),Panic(ETQueQueueEmpty));
	}

