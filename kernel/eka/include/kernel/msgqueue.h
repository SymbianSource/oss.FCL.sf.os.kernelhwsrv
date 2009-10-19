// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\msgqueue.h
// 
//

#ifndef __MSGQUEUE_H__
#define __MSGQUEUE_H__
#include <kernel/kernel.h>
#include "nk_priv.h"

/**
@internalComponent
*/
class DMsgQueue : public DObject
	{
public:
	enum TQueueState {EEmpty, EPartial, EFull};
	enum {KMaxLength = 256};
public:
	~DMsgQueue();
	TInt Create(DObject* aOwner, const TDesC* aName, TInt aMsgLength, TInt aSlotCount, TBool aVisible = ETrue);
	TInt Send(const TAny* aPtr, TInt aLength);
	TInt Receive(TAny* aPtr, TInt aLength);
	void NotifySpaceAvailable(TRequestStatus* aStatus);
	void NotifyDataAvailable(TRequestStatus* aStatus);
	void CancelSpaceAvailable();
	void CancelDataAvailable();
	TInt MessageSize() const;
private:
	enum TNotification {ESpaceAvailable, EDataAvailable};
	void CompleteRequestIfPending(DThread*& aThread, TClientRequest* aRequest, TInt aCompletionVal);
	void RequestNotification(TNotification aNotification, TRequestStatus* aStatus, DThread*& aThread, TClientRequest* aRequest);
private:
	TUint8* iMsgPool;			//pointer to the block of memory used for the slots
	TUint8* iFirstFreeSlot;		//first free slot in the msg pool unless iState == EFull
	TUint8* iFirstFullSlot;		//first full slot in the pool unless iState == EEmpty
	TUint8* iEndOfPool;			//address after the allocated pool.  
	DThread* iThreadWaitingOnSpaceAvail;	//thread waiting for space available notification
	DThread* iThreadWaitingOnDataAvail;		//thread waiting for data available notification
	TClientRequest* iDataAvailRequest;
	TClientRequest* iSpaceAvailRequest;
	TUint16 iMaxMsgLength;		//max message length in bytes
	TUint8 iState;				//whether the queue is empty, full or in between
	TUint8 iSpare;
public:
	friend class Monitor;
	};

#endif

