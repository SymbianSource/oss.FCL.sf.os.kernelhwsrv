// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\usbcc\queue.cpp
// Platform independent layer (PIL) of the USB Device controller driver:
// Simple singly linked list + its iterator.
// 
//

/**
 @file queue.cpp
 @internalTechnology
*/

#include <drivers/usbc.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "queueTraces.h"
#endif



void TSglQueLink::Enque(TSglQueLink* aLink)
//
// Enque this after aLink.
//
	{
	iNext = aLink->iNext;
	aLink->iNext = this;
	}


TSglQueBase::TSglQueBase(TInt aOffset)
//
// Constructor
//
	: iHead(NULL), iLast((TSglQueLink*) &iHead), iOffset(aOffset), iElements(0)
	{
	// ESQueOffsetNotAligned
	__ASSERT_ALWAYS((iOffset % 4 == 0), Kern::Fault(KUsbPILPanicCat, __LINE__));
	}


void TSglQueBase::DoAddLast(TAny* aPtr)
//
// Add the object at the end of the queue.
//
	{
	TSglQueLink* pL = PtrAdd((TSglQueLink*) aPtr, iOffset);
	pL->Enque(iLast);
	iLast = pL;
	iElements++;
	__ASSERT_DEBUG((iElements > 0), Kern::Fault(KUsbPILPanicCat, __LINE__));
	}


void TSglQueBase::DoRemove(TAny* aPtr)
//
// Remove the object from the queue.
//
	{
	TSglQueLink* pP = (TSglQueLink*) (&iHead);
	TSglQueLink* pL = PtrAdd((TSglQueLink*) aPtr, iOffset);
	TSglQueLink* pN = pP->iNext;
	while (pN)
		{
		if (pN == pL)
			{
			pP->iNext = pN->iNext;
			if (iLast == pL)
				{
				iLast = pP;
				if (iLast == NULL)
					iLast = (TSglQueLink*) (&iHead);
				}
			iElements--;
			__ASSERT_DEBUG((iElements >= 0), Kern::Fault(KUsbPILPanicCat, __LINE__));
			return;
			}
		pP = pN;
		pN = pP->iNext;
		}
	// This doesn't have to indicate an error (but might):
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FATAL, TSGLQUEBASE_DOREMOVE, 
	        "TSglQueBase::DoRemove: ESQueLinkNotQueued" );
	}


TSglQueIterBase::TSglQueIterBase(TSglQueBase& aQue)
//
// Constructor.
//
	: iOffset(aQue.iOffset), iHead(aQue.iHead), iNext(aQue.iHead)
	{
	}


void TSglQueIterBase::SetToFirst()
//
// Start from the beginning of the que.
//
	{
	iNext = iHead->iNext;
	}


TAny* TSglQueIterBase::DoPostInc()
//
// Return the current pointer and increment.
//
	{
	TAny* pN = iNext;
	if (pN == NULL)
		return NULL;
	iNext = iNext->iNext;
	return PtrSub(pN, iOffset);
	}


//---
