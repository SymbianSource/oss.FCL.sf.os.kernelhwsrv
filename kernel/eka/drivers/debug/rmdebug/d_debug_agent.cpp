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

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/kernel.h> 
#include <kernel/kern_priv.h>
#include <nk_trace.h>
#include <arm.h>

#include "d_process_tracker.h"
#include "debug_logging.h"

#include "d_debug_agent.h"
#include "debug_utils.h"

using namespace Debug;

#define NUMBER_OF_EVENTS_TO_QUEUE 100
#define CRITICAL_BUFFER_SIZE (NUMBER_OF_EVENTS_TO_QUEUE - 50)

// ctor
DDebugAgent::DDebugAgent(TUint64 aId)
: iId(aId),
  iEventInfo(NULL),
  iEventQueue(NUMBER_OF_EVENTS_TO_QUEUE, 0),
  iRequestGetEventStatus(NULL),
  iClientThread(0),
  iHead(0),
  iTail(0),
  iIgnoringTrace(EFalse)
	{
	LOG_MSG("DDebugAgent::DDebugAgent() ");

	// Initialize all the Event Actions to Ignore
	for(TInt i=0; i<EEventsLast; i++)
		{
		iEventActions[i] = EActionIgnore;
		}
	}

DDebugAgent* DDebugAgent::New(TUint64 aId)
	{
	LOG_MSG("DDebugAgent::New()");
	DDebugAgent* agent = new DDebugAgent(aId);
	if(agent == NULL)
		{
		return (NULL);
		}
	if(KErrNone != agent->Construct())
		{
		delete agent;
		return (NULL);
		}
	return agent;
	}

TInt DDebugAgent::Construct()
	{
	// Empty the event queue
	LOG_MSG("DDebugAgent::Construct()");
	TDriverEventInfo emptyEvent;

	for (TInt i=0; i<NUMBER_OF_EVENTS_TO_QUEUE; i++)
		{
		TInt err = iEventQueue.Append(emptyEvent);
		if (KErrNone != err)
			{
			LOG_MSG("Error appending blank event entry");
			return err;
			}
		}
	return KErrNone;
	}


// dtor
DDebugAgent::~DDebugAgent()
	{
	iEventQueue.Reset();
	}

// Associate an action with a particular kernel event
TInt DDebugAgent::SetEventAction(TEventType aEvent, TKernelEventAction aEventAction)
	{
	// Valid Event?
	if (aEvent >= EEventsLast)
		{
		LOG_MSG2("DDebugAgent::EventAction: Bad Event number %d",aEvent);
		return KErrArgument;
		}

	iEventActions[aEvent] = aEventAction;

	return KErrNone;
	}

/* Get the aEventAction associated with aEvent
 *
 * Returns : aEventAction (always +ve), or KErrArgument.
 */
TInt DDebugAgent::EventAction(TEventType aEvent)
	{
	// Validate the Event id
	if (aEvent >= EEventsLast)
		{
		LOG_MSG2("DDebugAgent::EventAction: Bad Event number %d",aEvent);
		return KErrArgument;
		}

	// Return the action associated with this event
	return iEventActions[aEvent];
	}

// Obtain the details of the latest kernel event (if it exists) and place the details in aEventInfo
// If there is no event in the queue for this process+agent combination, store the details
// so that it can be notified later when an event actually occurs.
//
// @param aAsyncGetValueRequest - TClientDataRequest object used for pinning user memory
// @param aEventInfo - Address of TEventInfo structure to place event data when available
// @param aClientThread - The ThreadId of the requesting user-side process. In this case the DSS.
void DDebugAgent::GetEvent(TClientDataRequest<TEventInfo>* aAsyncGetValueRequest, TEventInfo* aEventInfo, DThread* aClientThread)
	{
	iClientThread = aClientThread;

	if (BufferEmpty())
		{
		LOG_MSG("no events available");

		// store the pointer so we can modify it later
		iEventInfo = (TEventInfo *)aEventInfo;
		iRequestGetEventStatus = aAsyncGetValueRequest;
		return;
		}

	LOG_MSG("Event available");

	// returning the event to the client
	TInt err = iEventQueue[iTail].WriteEventToClientThread(aAsyncGetValueRequest,iClientThread);
	if (KErrNone != err)
		{
		LOG_MSG2("Error writing event info: %d", err);
		return;
		}

	// signal the DSS thread
	Kern::QueueRequestComplete(iClientThread, aAsyncGetValueRequest, KErrNone);

	iEventQueue[iTail].Reset();

	// move to the next slot
	IncrementPosition(iTail);
	}

// Stop waiting for an event to occur. This means events will be placed in the iEventQueue
// until GetEvent is called.
TInt DDebugAgent::CancelGetEvent(void)
	{
	Kern::QueueRequestComplete(iClientThread, iRequestGetEventStatus, KErrCancel);
	iEventInfo = NULL;
    iRequestGetEventStatus = 0;
	iClientThread = 0;

	return KErrNone;
	}

// Signal a kernel event to the user-side DSS when it occurs, or queue it for later
// if the user-side has not called GetEvent (see above).
//
// @param aEventInfo - the details of the event to queue.
void DDebugAgent::NotifyEvent(const TDriverEventInfo& aEventInfo)
	{
	LOG_MSG("DDebugAgent::NotifyEvent()");
	// Action depends on the TKernelEvent type in aEventInfo.iType
	
	// Added to fix the pass by value issue seen in Coverity.  
	// Function is changed to pass by reference but temp object is explicitly created.
	TDriverEventInfo eventInfo = aEventInfo;

	if(aEventInfo.iEventType >= EEventsLast)
		{
		// unknown event type so return
		return;
		}

	TKernelEventAction action = iEventActions[eventInfo.iEventType];

	switch (action)
		{
		case EActionSuspend:
			{
			LOG_MSG("DDebugAgent::NotifyEvent() Suspend thread");
			DThread* currentThread = &Kern::CurrentThread();
			switch(eventInfo.iEventType)
				{
				case EEventsAddLibrary:
				case EEventsRemoveLibrary:
					currentThread = DebugUtils::OpenThreadHandle(eventInfo.iThreadId);
					if(currentThread)
						{
						currentThread->Close(NULL);
						}
					break;
				default:
					break;
				}
			TInt err = TheDProcessTracker.SuspendThread(currentThread, eventInfo.FreezeOnSuspend());
			if(!( (err == KErrNone) || (err == KErrAlreadyExists) ))
				{
				// Is there anything we can do in the future to deal with this error having happened?
				LOG_MSG2("DDebugAgent::NotifyEvent() Problem while suspending thread: %d", err);
				}

			// now drop through to the continue case, which typically notifies
			// the debug agent of the event
			}
		case EActionContinue:
			LOG_MSG("DDebugAgent::NotifyEvent() Continue");

			// Tell the user about this event
			if (iEventInfo && iClientThread)
			{
				LOG_MSG("Completing event\r\n");

				// returning the event to the client
				TInt err = eventInfo.WriteEventToClientThread(iRequestGetEventStatus,iClientThread);
				if (KErrNone != err)
				{
					LOG_MSG2("Error writing event info: %d", err);
				}

				// clear this since we've completed the request
				iEventInfo = NULL;

				// signal the debugger thread
				Kern::QueueRequestComplete(iClientThread, iRequestGetEventStatus, KErrNone);
			}
			else
			{
				LOG_MSG("Queuing event\r\n");

				QueueEvent(eventInfo);

			}
			break;

		case EActionIgnore:
		default:
			LOG_MSG("DDebugAgent::NotifyEvent() fallen through to default case");
			// Ignore everything we don't understand.
			return;
		}

	}

// Used to identify which Debug Agent this DDebugAgent is associated with.
TUint64 DDebugAgent::Id(void)
	{
	return iId;
	}

// Used to add an event to the event queue for this debug agent
void DDebugAgent::QueueEvent(TDriverEventInfo& aEventInfo)
	{
	// Have we caught the tail?
	if(BufferFull())
		{
		return;
		}
	
	//check to see if we wish to ignore this event - we dump trace events as they are lower priority than the other events
	if(BufferAtCriticalLevel())
		{
		if(aEventInfo.iEventType == EEventsUserTrace)
			{
			if(!iIgnoringTrace)
				{
				//if this is the first time we are ignoring trace events, we need to issue a EEventsUserTracesLost event
				aEventInfo.Reset();
				aEventInfo.iEventType = EEventsUserTracesLost;
				
				iIgnoringTrace = ETrue;
				}
			else
				{
				//otherwise, ignore this event
				return;
				}
			}
		}
	else
		{
		//reset the iIgnoringTrace flag as we are not at critical level
		iIgnoringTrace = EFalse; 
		}	

	// only one space left so store a EEventsBufferFull event
	if(!BufferCanStoreEvent())
		{
		aEventInfo.Reset();
		aEventInfo.iEventType = EEventsBufferFull;
		}

	__NK_ASSERT_DEBUG(iEventQueue[iHead].iEventType == EEventsUnknown); // we think there is space but the slot is not marked empty

	// Insert the event into the ring buffer at iHead
	iEventQueue[iHead] = aEventInfo;
	IncrementPosition(iHead);
	}

// Checks whether the event queue is empty
TBool DDebugAgent::BufferEmpty() const
	{
	return (NumberOfEmptySlots() == NUMBER_OF_EVENTS_TO_QUEUE);
	}

// Checks whether the event queue is full
TBool DDebugAgent::BufferFull() const
	{
	return (NumberOfEmptySlots() == 0);
	}

// Checks whether there is room in the event queue to store an event (i.e. at least two free slots)
TBool DDebugAgent::BufferCanStoreEvent() const
	{
	return (NumberOfEmptySlots() > 1);
	}

//This looks to see if the buffer is close to being full and should only accept higher priority debug events (user trace is the only low priority event) 
TBool DDebugAgent::BufferAtCriticalLevel() const
	{
	return (NumberOfEmptySlots() < NUMBER_OF_EVENTS_TO_QUEUE - CRITICAL_BUFFER_SIZE);
	}

// increments aPosition, wrapping at NUMBER_OF_EVENTS_TO_QUEUE if necessary
void DDebugAgent::IncrementPosition(TInt& aPosition)
	{
	aPosition = (aPosition + 1) % NUMBER_OF_EVENTS_TO_QUEUE;
	}

// finds the number of empty slots in the event queue
TInt DDebugAgent::NumberOfEmptySlots() const
	{
	if(iHead < iTail)
		{
		return (iTail - iHead) - 1;
		}
	// iHead >= iTail
	return NUMBER_OF_EVENTS_TO_QUEUE - (iHead - iTail);
	}

