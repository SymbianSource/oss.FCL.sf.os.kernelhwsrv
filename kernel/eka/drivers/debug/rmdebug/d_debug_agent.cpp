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

#include "d_debug_agent.inl"

using namespace Debug;

// ctor
DDebugAgent::DDebugAgent(TUint64 aId) :
	iId(aId),
	iRequestGetEventStatus(NULL),
	iClientThread(0),
	iEventQueue(KNumberOfEventsToQueue, 0),
	iHead(0),
	iTail(0),
	iEventQueueLock(NULL),
	iFreeSlots(KNumberOfEventsToQueue),
	iIgnoringTrace(EFalse),
	iEventBalance(0)
	{
	LOG_MSG2("DDebugAgent::DDebugAgent(), this=0x%x ", this);

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

	// Use a semaphore to serialise access
	TInt err = Kern::SemaphoreCreate(agent->iEventQueueLock, _L("RM_DebugAgentQueueLock"), 1 /* Initial count */);
	if (err != KErrNone)
		return NULL;

	return agent;
	}

/** Standard contructor.
 * Fills event queue with empty events
 * @return : standard system error code
 */
TInt DDebugAgent::Construct()
	{
	// Empty the event queue
	LOG_MSG("DDebugAgent::Construct()");
	TDriverEventInfo emptyEvent;
	TInt err = KErrNone;

	for (TInt i=0; i<KNumberOfEventsToQueue; i++)
		{
		err = iEventQueue.Append(emptyEvent);
		if (err != KErrNone)
			{
			LOG_MSG("Error appending blank event entry");
			return err;
			}
		}

	err = Kern::CreateClientDataRequest(iRequestGetEventStatus);
	if(err != KErrNone)
		{
		LOG_MSG("Error creating TClientDataRequest");
		return err;
		}

	LOG_MSG2("DDebugAgent::Construct() iRequestGetEventStatus=0x%08x", iRequestGetEventStatus);

	return err;
	}

// dtor
DDebugAgent::~DDebugAgent()
	{
	iEventQueue.Reset();

	if (iEventQueueLock)
		iEventQueueLock->Close(NULL);
	
	if(iRequestGetEventStatus)
		Kern::DestroyClientRequest(iRequestGetEventStatus);
	
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

/** Get the aEventAction associated with aEvent
 *
 * @return : aEventAction (always +ve), or KErrArgument.
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

/** Obtain the details of the latest kernel event (if it exists) and place the details in aEventInfo
 * If there is no event in the queue for this process+agent combination, store the details
 * so that it can be notified later when an event actually occurs.
 * 
 * @param aAsyncGetValueRequest - TClientDataRequest object used for pinning user memory
 * @param aClientThread - The ThreadId of the requesting user-side process. In this case the DSS.
 */
void DDebugAgent::GetEvent(TClientDataRequest<TEventInfo>* aAsyncGetValueRequest, DThread* aClientThread)
	{
	LockEventQueue();

	iRequestGetEventStatus->Reset();
	TInt err = iRequestGetEventStatus->SetStatus( aAsyncGetValueRequest->StatusPtr() );
	if (err != KErrNone)
		{
		LOG_MSG2("Error :iRequestGetEventStatus->SetStatus ret %d", err);
		return;
		}
	
	iRequestGetEventStatus->SetDestPtr( aAsyncGetValueRequest->DestPtr() );

	iEventBalance++;
	
	LOG_MSG4("DDebugAgent::GetEvent: this=0x%08x, iRequestGetEventStatus=0x%08x, iEventBalance=%d", 
		this, iRequestGetEventStatus, iEventBalance );
	
	iClientThread = aClientThread;
	
	if (BufferEmpty())
		{
		LOG_MSG2("Event buffer empty, iEventBalance=%d", iEventBalance);		
		UnlockEventQueue();
		return;
		}

	LOG_MSG2("Event already available at queue pos=%d", iTail);

	// returning the event to the client
	err = iEventQueue[iTail].WriteEventToClientThread(iRequestGetEventStatus,iClientThread);
	if (err != KErrNone)
		{
		LOG_MSG2("Error writing event info: %d", err);
		UnlockEventQueue();
		return;
		}

	// signal the DSS thread
	Kern::QueueRequestComplete(iClientThread, iRequestGetEventStatus, KErrNone);
	iEventBalance--;

	iEventQueue[iTail].Reset();

	// move to the next slot
	IncrementTailPosition();

	UnlockEventQueue();
	}

/**
 * Stop waiting for an event to occur. This means events will be placed 
 * in the iEventQueue (by setting iEventBalance to 0) until GetEvent is called. 
 */ 
TInt DDebugAgent::CancelGetEvent(void)
	{
	LOG_MSG2("DDebugAgent::CancelGetEvent. iEventBalance=%d. > QueueRequestComplete", iEventBalance);
	Kern::QueueRequestComplete(iClientThread, iRequestGetEventStatus, KErrCancel);
	iEventBalance=0;
	iClientThread = 0;
	return KErrNone;
	}

/** Signal a kernel event to the user-side DSS when it occurs, or queue it for later
 * if the user-side has not called GetEvent (see above).
 * 
 * @param aEventInfo - the details of the event to queue.
 */
void DDebugAgent::NotifyEvent(const TDriverEventInfo& aEventInfo)
	{

	if(aEventInfo.iEventType >= EEventsLast)
		{
		LOG_MSG3("DDebugAgent::NotifyEvent(),iEventType %d, this=0x%x. Ignoring since > EEventsLast", aEventInfo.iEventType, this);
		return;
		}

	LockEventQueue();

	DThread* currentThread = &Kern::CurrentThread();
	
	LOG_MSG5("DDebugAgent::NotifyEvent(), iEventType %d, this=0x%x currThrd=0x%08x, iEventBalance=%d",
		aEventInfo.iEventType, this, currentThread, iEventBalance );
	TKernelEventAction action = iEventActions[aEventInfo.iEventType];

	switch (action)
		{
		case EActionSuspend:
			{
			LOG_MSG("DDebugAgent::NotifyEvent() Suspend thread");

			switch(aEventInfo.iEventType)
				{
				case EEventsAddLibrary:
				case EEventsRemoveLibrary:
					currentThread = DebugUtils::OpenThreadHandle(aEventInfo.iThreadId);
					if(currentThread)
						{
						currentThread->Close(NULL);
						}
					break;
				default:
					break;
				}
			TInt err = TheDProcessTracker.SuspendThread(currentThread, aEventInfo.FreezeOnSuspend());
			if((err != KErrNone) && (err != KErrAlreadyExists))
				{
				// Is there anything we can do in the future to deal with this error having happened?
				LOG_MSG2("DDebugAgent::NotifyEvent() Problem while suspending thread: %d", err);
				}

			// now drop through to the continue case, which typically notifies
			// the debug agent of the event
			}
		case EActionContinue:
			{
			// Queue this event
			QueueEvent(aEventInfo);

			// Tell the user about the oldest event in the queue
			if ( iClientThread )
				{
				if( iRequestGetEventStatus && (iEventBalance > 0) )
					{
					// Fill the event data
					TInt err = iEventQueue[iTail].WriteEventToClientThread(iRequestGetEventStatus,iClientThread);
					if (err != KErrNone)
						{
						LOG_MSG2("Error writing event info: %d", err);
						}

					// signal the debugger thread 
					LOG_MSG4("> QueueRequestComplete iRequestGetEventStatus=0x%08x, iEventBalance=%d, iTail=%d",
						iRequestGetEventStatus->iStatus, iEventBalance, iTail );
					Kern::QueueRequestComplete(iClientThread, iRequestGetEventStatus, KErrNone);

					iEventBalance--;

					iEventQueue[iTail].Reset();

					// move to the next slot
					IncrementTailPosition();
					}
				else
					{
					if( !iRequestGetEventStatus )
						{
						LOG_MSG("iRequestGetEventStatus is NULL so not signalling client" );
						}
					else
						{
						LOG_MSG2("Queued event. iEventBalance=%d (unbalanced event requests vs notifications)", 
							iEventBalance );
						}
					}
				}
			else
				{
				 LOG_MSG("DDebugAgent::NotifyEvent() : Not informing client since its thread is NULL");
				}
			break;
			}
		case EActionIgnore:
		default:
			LOG_EVENT_MSG("DDebugAgent::NotifyEvent() fallen through to default case");
			// Ignore everything we don't understand.

		}

	UnlockEventQueue();

	}

// Used to identify which Debug Agent this DDebugAgent is associated with.
TUint64 DDebugAgent::Id(void)
	{
	return iId;
	}

/**
 * Used to add an event to the event queue for this debug agent if event 
 * queue is not at critical level. If it is at critical and it is trace event, 
 * we start ignoring trace events and insert a lost trace event.
 * If the buffer cannot store an event, only insert a buffer full event.
 * @see EEventsBufferFull
 * @see EEventsUserTracesLost
 * @see TDriverEventInfo
 * @see iEventQueue
 */
void DDebugAgent::QueueEvent(const TDriverEventInfo& aEventInfo)
	{
	// Have we caught the tail?
	if(BufferFull())
		{
		LOG_MSG("DDebugAgent::QueueEvent : BufferFull. Not queueing");
		return;
		}

	// Assert if we think there is space but the slot is not marked empty
	__NK_ASSERT_DEBUG(iEventQueue[iHead].iEventType == EEventsUnknown);

	const TBool bufferAtCritical = BufferAtCriticalLevel();

	if(!bufferAtCritical)
		{
		//reset the iIgnoringTrace flag as we are not at 
		//critical level and can store event
		iIgnoringTrace = EFalse; 
		
		// Insert the event into the ring buffer at iHead
		iEventQueue[iHead] = aEventInfo;
		IncrementHeadPosition();
		}
	else if(bufferAtCritical && BufferCanStoreEvent())
		{
		LOG_MSG("DDebugAgent::QueueEvent : BufferCritical");
		if(aEventInfo.iEventType == EEventsUserTrace)
			{
			if(!iIgnoringTrace)
				{
				//if this is the first time we are ignoring trace events, 
				//we need to issue a EEventsUserTracesLost event
				iEventQueue[iHead].Reset();
				iEventQueue[iHead].iEventType = EEventsUserTracesLost;
				IncrementHeadPosition();

				iIgnoringTrace = ETrue;
				}
			else
				{
				//otherwise, ignore this event
				LOG_MSG("DDebugAgent::QueueEvent : Ignore EEventsUserTrace event");
				}
			}
		else
			{
			// Store the event since its not a trace event
			iEventQueue[iHead] = aEventInfo;
			IncrementHeadPosition();
			}
		}
	else
		{
		//At critical level and cannot store new events, so 
		//only one space left. Store a EEventsBufferFull event
		LOG_MSG("DDebugAgent::QueueEvent : Event Buffer Full, ignoring event");
		iEventQueue[iHead].Reset();
		iEventQueue[iHead].iEventType = EEventsBufferFull;
		IncrementHeadPosition();
		}
	}

// End of file - d_debug_agent.cpp
