// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\cbase\ub_tque.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>




// Class TTickCountQue
/**
@internalComponent
@released

Constructs an empty list header
*/
TTickCountQue::TTickCountQue()
	{}




/**
@internalComponent
@released

Adds the specified list element.

The element is added into the list in order of its tick count.

@param aRef The list element to be inserted.
*/
void TTickCountQue::Add(TTickCountQueLink& aRef)
	{
	TTickCountQueLink* currentLink = (TTickCountQueLink*)(iHead.iNext);
	TTickCountQueLink* addLink = &aRef;

	while (	(currentLink != (TTickCountQueLink*)&iHead) &&
			(((TInt)(addLink->iTickCount - currentLink->iTickCount)) >= 0)
		)
		{
		currentLink = (TTickCountQueLink*)currentLink->iNext;
		}

	addLink->Enque(currentLink->iPrev);
	}




/**
@internalComponent
@released

Removes the first list element from the linked list if its tick count
is prior to the current tick count.

@param aTickCount The current tick count.

@return A pointer to the element removed from the linked list. This is NULL 
        if the first element has yet to expire or the queue is empty.
*/
TTickCountQueLink* TTickCountQue::RemoveFirst(TUint aTickCount)
	{
	TTickCountQueLink* firstLink = (TTickCountQueLink*)iHead.iNext;

	if (((TInt)(firstLink->iTickCount - aTickCount)) <= 0)
		{
		return RemoveFirst();
		}
	else
		{
		return NULL;
		}
	}


/**
@internalComponent
@released

Removes the first list element from the linked list, if any.

@return A pointer to the element removed from the linked list. This is NULL, 
        if the queue is empty.
*/
TTickCountQueLink* TTickCountQue::RemoveFirst()
	{
	TTickCountQueLink* firstLink = (TTickCountQueLink*)iHead.iNext;

	if (firstLink != (TTickCountQueLink*)&iHead)
		{
		firstLink->Deque();
		return firstLink;
		}

	return NULL;
	}




/**
@internalComponent
@released

Gets a pointer to the first list element in the doubly linked list.

@return A pointer to the first list element in the doubly linked list. If 
        the list is empty, this pointer is not necessarily NULL and must not
		be assumed to point to a valid object.
*/
TTickCountQueLink* TTickCountQue::First() const
	{
#if defined (_DEBUG)
	__DbgTestEmpty();
#endif
    return((TTickCountQueLink*)iHead.iNext);
    }
