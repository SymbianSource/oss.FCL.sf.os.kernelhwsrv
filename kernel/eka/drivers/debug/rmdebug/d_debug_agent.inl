// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Inline methods for debug agent class
//


/**
 @file
 @internalComponent
 @released
*/

#ifndef D_DEBUG_AGENT_INL
#define D_DEBUG_AGENT_INL


/**
 Checks whether the event queue is empty
*/
inline TBool DDebugAgent::BufferEmpty() const
	{
	return (NumberOfEmptySlots() == KNumberOfEventsToQueue);
	}

/**
 Checks whether the event queue is full
*/
inline TBool DDebugAgent::BufferFull() const
	{
	return (NumberOfEmptySlots() == 0);
	}

/**
 Checks whether there is room in the event queue to store an event
*/
inline TBool DDebugAgent::BufferCanStoreEvent() const
	{
	return (NumberOfEmptySlots() > 0);
	}

/**
 This looks to see if the buffer is close to being full and should only
 accept higher priority debug events (user trace is the only low priority event) 
*/
inline TBool DDebugAgent::BufferAtCriticalLevel() const
	{
	return (NumberOfEmptySlots() < KNumberOfEventsToQueue - KCriticalBufferSize);
	}

/**
 Increments Head position, wrapping at KNumberOfEventsToQueue if necessary
*/
inline void DDebugAgent::IncrementHeadPosition(void)
	{
	iHead = (iHead + 1) % KNumberOfEventsToQueue;

	iFreeSlots--;
	}

/**
 Increments Tail position, wrapping at KNumberOfEventsToQueue if necessary
*/
inline void DDebugAgent::IncrementTailPosition(void)
	{
	iTail = (iTail + 1) % KNumberOfEventsToQueue;

	iFreeSlots++;
}

/**
 Returns the number of free slots in the event queue
*/
inline TInt DDebugAgent::NumberOfEmptySlots() const
	{
	return iFreeSlots;
	}

/**
 Lock access to this agent's event queue
*/
inline void DDebugAgent::LockEventQueue(void)
	{
	Kern::SemaphoreWait(*iEventQueueLock);
	}

/**
 Release the lock on this agent's event queue
*/
inline void DDebugAgent::UnlockEventQueue(void)
	{
	Kern::SemaphoreSignal(*iEventQueueLock);
	}


#endif	// D_DEBUG_AGENT_INL
