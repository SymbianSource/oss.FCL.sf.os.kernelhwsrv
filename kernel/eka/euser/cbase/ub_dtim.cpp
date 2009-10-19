// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\cbase\ub_dtim.cpp
// 
//

#include "ub_std.h"

/**
Creates a new timed event queue with the specified active object priority.

@param aPriority The priority of this active object.

@return On successful return, a pointer to the queue of timed events.

@publishedAll
@released
*/
EXPORT_C CDeltaTimer* CDeltaTimer::NewL(TInt aPriority)
	{
	TTimeIntervalMicroSeconds32 tickPeriod;
	UserHal::TickPeriod(tickPeriod);

	CDeltaTimer* timer = new(ELeave) CDeltaTimer(aPriority, tickPeriod.Int());

	TInt err = timer->iTimer.CreateLocal();

	if (err)
		{
		delete timer;
		User::Leave(err);
		}

	CActiveScheduler::Add(timer);

	return timer;
	}

/**
Creates a new timed event queue with the specified active object priority, and 
the specified timer granularity.

@param aPriority    The priority of this active object.
@param aGranularity Ignored.  The resolution of the timer is the tick period.

@return On successful return, a pointer to the queue of timed events.

@publishedAll
@deprecated
*/
EXPORT_C CDeltaTimer* CDeltaTimer::NewL(TInt aPriority, TTimeIntervalMicroSeconds32 /*aGranularity*/)
	{
	return CDeltaTimer::NewL(aPriority);
	}

/**
Constructor taking an active object priority value.

The constructor sets this active object's priority value through a call to 
the base class constructor in its c'tor list.

@param aPriority    The priority of this active object.
@param aTickPeriod  The period of a tick on the system.

@internalComponent
@released
*/
CDeltaTimer::CDeltaTimer(TInt aPriority, TInt aTickPeriod)
	: CActive(aPriority), iTickPeriod(aTickPeriod)
	{
	}

/**
Adds a new timed event entry into the timed event queue.
	
@param aTimeInMicroSeconds The interval from the present time when the timed 
	                       event entry is to expire.
@param aEntry              The timed event entry encapsulating the call back that
                           is to be called when this timed event entry expires.

@publishedAll
@released
*/
EXPORT_C void CDeltaTimer::Queue(TTimeIntervalMicroSeconds32 aTimeInMicroSeconds, TDeltaTimerEntry& aEntry)
	{
	QueueLong(TTimeIntervalMicroSeconds(MAKE_TINT64(0, aTimeInMicroSeconds.Int())), aEntry);
	}

/**
Adds a new timed event entry into the timed event queue.
	
@param aTimeInMicroSeconds The interval from the present time when the timed 
	                       event entry is to expire.
@param aEntry              The timed event entry encapsulating the call back that
                           is to be called when this timed event entry expires.

@return KErrNone if sucessful, KErrOverflow if the interval is too great or negative.

@publishedAll
@released
*/
EXPORT_C TInt CDeltaTimer::QueueLong(TTimeIntervalMicroSeconds aTimeInMicroSeconds, TDeltaTimerEntry& aEntry)
	{
	const TInt64 timeInTicks = (aTimeInMicroSeconds.Int64() + iTickPeriod - 1) / iTickPeriod;

	TInt timeInTicks32 = I64LOW(timeInTicks);

	// We are using deltas on tick values, hence using maximum signed number of ticks
	if (I64HIGH(timeInTicks) || (timeInTicks32 < 0))
		{
		return KErrOverflow;
		}

	// Make sure we queue for at least one tick
	if (timeInTicks32 == 0)
		{
		timeInTicks32 = 1;
		}
	
	// Calculate tick count for new entry
	aEntry.iLink.iTickCount = Exec::TickCount() + timeInTicks32;

	// Add this entry at the right spot
	iQueue.Add(aEntry.iLink);

	// Queue the timer, re-queuing if we've added to the head of the queue
	Activate(&aEntry.iLink == iQueue.First());
	
	return KErrNone;
	}

/**
Issues a new RTimer request, if there is no outstanding request and the queue 
is not empty.

@internalComponent
@released
*/
void CDeltaTimer::Activate(TBool aRequeueTimer)
//
// Queue a request on the timer.
//
	{
	if (IsActive())
		{
		if (aRequeueTimer)
			{
			Cancel();
			}
		else
			{
			return;
			}		
		}

	if (!iQueue.IsEmpty() && !iQueueBusy)
		{
		SetActive();

		const TInt ticksToWait = iQueue.First()->iTickCount - Exec::TickCount();

		if (ticksToWait > 0)
			{
			iTimer.AfterTicks(iStatus, ticksToWait);
			}
		else
			{
			TRequestStatus* status = &iStatus;
			User::RequestComplete(status, KErrNone);
			}
		}
	}

/**
Deals with an RTimer completion event.

The function inspects the timed event entry at the head of the queue, and 
reduces its timer value by the appropriate amount. If this timed event is 
now found to have expired, the call back function is called, and the timed 
event entry removed from the queue.

If the timed event entry has not expired, it remains at the head of the queue.

The function issues a new RTimer request, using the timer granularity value 
as the time interval.

@see RTimer
@see CActive

@internalComponent
@released
*/
void CDeltaTimer::RunL()
//
// Call all zero delta callbacks
	{
	// Queue busy
	iQueueBusy = ETrue;

	// Whilst the list of expired timers is being processed, time will pass and
	// the tick count may have increased such that there are now more expired
	// timers.  Loop until we have either emptied the queue or can wait for a
	// timer exipration in the future.
	while (!iQueue.IsEmpty())
		{
		// Calculate how long till first timer expires
		const TUint tickCount = Exec::TickCount();

		// If the first timer is yet to expire, wait some more
		if (((TInt)(iQueue.First()->iTickCount - tickCount)) > 0)
			{
			break;
			}

		// Remove entry before callback to prevent re-entrancy issues
		TTickCountQueLink* entry = iQueue.RemoveFirst();

		// Iterate through the timers we know have expired based on the
		// last calculation of delta
		while (entry)
			{
			// Make callback.  This could go reentrant on Queue[Long]() or Remove().
			reinterpret_cast<TDeltaTimerEntry*>(
				PtrSub(
					entry,
					_FOFF(TDeltaTimerEntry, iLink)
				))
			->iCallBack.CallBack();

			// Remove the next expired entry, if any
			entry = iQueue.RemoveFirst(tickCount);
			}
		}

	// Queue idle
	iQueueBusy = EFalse;

	// Requeue timer
	Activate();
	}
	
/**
Implements cancellation of an oustanding RTimer request.

@internalComponent
@released
*/
void CDeltaTimer::DoCancel()
	{
	iTimer.Cancel();
	}

/**
Removes the specified timed event entry from the timer queue.

@param aEntry The timed event entry.

@publishedAll
@released
*/
EXPORT_C void CDeltaTimer::Remove(TDeltaTimerEntry& aEntry)
	{
	// Remove the specified entry from the list
	aEntry.iLink.Deque();
	}

/**
Destructor.

Frees all resources before destruction of the object. Specifically, it cancels 
any outstanding timer requests generated by the RTimer object and then deletes 
all timed event entries from the timed event queue.

@see RTimer

@publishedAll
@released
*/
CDeltaTimer::~CDeltaTimer()
	{
	Cancel();

	while (!iQueue.IsEmpty())
		{
		iQueue.First()->Deque();
		}

	iTimer.Close();
	}
