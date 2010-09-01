// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Purpose: Kernel-side tracking of debug agent information associated
// with each process being debugged.
// 
//

#ifndef D_DEBUG_AGENT_H
#define D_DEBUG_AGENT_H

#include <rm_debug_api.h>
#include "d_driver_event_info.h"

/**
* Handles events from the kernel, filters them according to the debug agent's requests, 
* and signals these events to the user side in FIFO-style. 
* @see TKernelEventAction
* @see TEventInfo
*/
class DDebugAgent : public DBase
{
public:
	static DDebugAgent* New(TUint64);
	~DDebugAgent();

	TInt SetEventAction(Debug::TEventType aEvent, Debug::TKernelEventAction aEventAction);
	void GetEvent(TClientDataRequest<Debug::TEventInfo>* aAsyncGetValueRequest, DThread* aClientThread);
	TInt EventAction(Debug::TEventType aEvent);

	TInt CancelGetEvent(void);
	void NotifyEvent(const TDriverEventInfo& aEventInfo);
	TUint64 Id();

protected:
	DDebugAgent(TUint64 aId);
	TInt Construct();

private:
	void QueueEvent(const TDriverEventInfo& aEventInfo);
	TBool BufferEmpty() const;
	TBool BufferFull() const;
	TBool BufferCanStoreEvent() const;
	TBool BufferAtCriticalLevel() const;
	void IncrementHeadPosition(void);
	void IncrementTailPosition(void);
	TInt NumberOfEmptySlots() const;
	void LockEventQueue(void);
	void UnlockEventQueue(void);

private:

	TUint64	iId;
	Debug::TKernelEventAction iEventActions[Debug::EEventsLast];

	/**
	* Object used to write events back to DSS thread
	* @see TEventInfo
	*/
	TClientDataRequest<Debug::TEventInfo>* iRequestGetEventStatus;

	DThread* iClientThread;

	/** 
	* Ring buffer of pending events. Access to it is controlled by 
	* @see iEventQueueLock
	*/
	RArray<TDriverEventInfo> iEventQueue;

	/**
	* Ring buffer head. Points to the next empty slot in iEventQueue
	* @see iEventQueue
	*/
	TInt iHead;	

	/**
	* Ring buffer tail. Points to the oldest full slot in iEventQueue
	* @see iEventQueue 
	*/
	TInt iTail;

	/** 
	* Control access to event queue.
	* @see iEventQueue
	*/
	DSemaphore* iEventQueueLock;

	/**
	* Keeps track of how many free slots are available in the event queue.
	* @see iEventQueue
	*/
	TInt iFreeSlots;

	/**
	* Boolean to indicate if we have told the agent that we are ignoring trace events
	* @see QueueEvent
	*/
	TBool iIgnoringTrace;
	
	/**
	* Used to control the delivery of events to the client so that only 
	* when more requests than deliveries have taken place can we deliver the 
	* next event
	* 
	* Incremented when a request for event takes place
	* @see GetEvent
	* 
	* Decremented when an event is delivered. 
	* @see NotifyEvent
	* 
	* Cleared when event requests are cancelled
	* @see CancelGetEvent
	* 
	*/
	TInt   iEventBalance;

	/**
	* Length of kernel-event queue.
	* This is a power of two for efficiency when using the 
	* remainder operator
	* @see DDebugAgent::iEventQueue
	*/
	static const TUint KNumberOfEventsToQueue = 128;

	/**
	* This determines the number of events at which we stop accepting 
	* low priority events into the event queue.
	* @see DDebugAgent::BufferAtCriticalLevel
	* @see DDebugAgent::iEventQueue
	*/
	static const TUint KCriticalBufferSize = 64;

};

#endif // D_DEBUG_AGENT_H

