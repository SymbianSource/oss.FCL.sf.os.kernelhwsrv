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
// e32\euser\cbase\ub_tim.cpp
// 
//

#include "ub_std.h"

EXPORT_C CTimer::CTimer(TInt aPriority)
	: CActive(aPriority)
/**
Protected constructor with priority.

Use this constructor to set the priority of the active object.

Classes derived from CTimer must define and provide a constructor through 
which the priority of the active object can be passed. Such a constructor 
can call CTimer's constructor in its constructor initialisation list.

@param aPriority The priority of the timer.
*/
	{
	}




EXPORT_C CTimer::~CTimer()
/**
Destructor.

Frees resources prior to destruction. Specifically, it cancels any outstanding 
request and closes the RTimer handle.
*/
	{

	Cancel();
	iTimer.Close();
	}




EXPORT_C void CTimer::At(const TTime &aTime)
/**
Requests an event at a given local time.

This timer completes at the specified time - if the machine is in a 
turned off state at that time, the machine will be turned on again.

Notes:

1. The CTimer' RunL() function will be run as soon as possible after the
   specified  system time.

2. The RunL() may be delayed because the RunL() of another active object, with 
   the deepest nesting-level active scheduler on the same thread, is running 
   when the event occurs: this cannot be avoided, but can be minimised by
   making  all RunL()s of short duration.

3. The RunL() may be delayed because other, higher-priority, active objects are 
   scheduled instead. This can be avoided by making CTimers very high-priority.
   
4. The TTime object should be set to the home time.

@param aTime The local time at which the event is to occur.

@see TTime::HomeTime
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.At(iStatus,aTime);
	SetActive();
	}




EXPORT_C void CTimer::AtUTC(const TTime &aTimeInUTC)
/**
Requests an event at a given UTC time.

This timer completes at the specified time - if the machine is in a 
turned off state at that time, the machine will be turned on again.

Notes:

1. The CTimer' RunL() function will be run as soon as possible after the
   specified  system time.

2. The RunL() may be delayed because the RunL() of another active object, with 
   the deepest nesting-level active scheduler on the same thread, is running 
   when the event occurs: this cannot be avoided, but can be minimised by
   making  all RunL()s of short duration.

3. The RunL() may be delayed because other, higher-priority, active objects are 
   scheduled instead. This can be avoided by making CTimers very high-priority.
   
4. The TTime object should be set to the universal time.

@param aTime The UTC time at which the event is to occur.

@see TTime::UniversalTime
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.AtUTC(iStatus,aTimeInUTC);
	SetActive();
	}




EXPORT_C void CTimer::After(TTimeIntervalMicroSeconds32 anInterval)
/**
Requests an event after an interval.

This timer completes after the specified number of microseconds. The 
"after timer" counter stops during power-down. Therefore, a 5-second timer 
will complete late if the machine is turned off 2 seconds after the request 
is made.

Notes:

1. The CTimer's RunL() function will be run as soon as possible after the
   specified interval.

2. The RunL() may be delayed because the RunL() of another active object, with 
   the deepest nesting-level active scheduler on the same thread, is running 
   when the event occurs: this cannot be avoided, but can be minimised by
   making  all RunL()s of short duration.

3. The RunL() may be delayed because other, higher-priority, active objects are 
   scheduled instead. This can be avoided by making CTimers very high-priority.

@param anInterval Interval after which event is to occur, in microseconds.

@panic USER 87, if anInterval is negative. This is raised by the
       underlying RTimer.
@panic E32USER-CBase 51, if the active object has not been added to an
       active scheduler.
       
@see RTimer
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.After(iStatus,anInterval);
	SetActive();
	}




EXPORT_C void CTimer::Lock(TTimerLockSpec aLock)
/** 
Requests an event on a specified second fraction.

Note that the RunL() function is run exactly on the specified second fraction.

@param aLock The fraction of a second at which the timer completes.
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.Lock(iStatus,aLock);
	SetActive();
	}




EXPORT_C void CTimer::Inactivity(TTimeIntervalSeconds aSeconds)
/**
Requests an event if no activity occurs within the specified interval.

@param aSeconds The time interval.
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.Inactivity(iStatus, aSeconds);
	SetActive();
	}



EXPORT_C void CTimer::HighRes(TTimeIntervalMicroSeconds32 aInterval)
/**
Requests an event after the specified interval to a resolution of 1ms. 
The "HighRes timer" counter stops during power-down (the same as "after timer"). 

@param aInterval  The time interval, in microseconds, after which an event
                  is to occur.
@panic USER 87, if anInterval is negative. This is raised by the
       underlying RTimer.
@panic KERN-EXEC 15, if this function is called while a request for a timer
       event is still outstanding.
*/
	{

	__ASSERT_ALWAYS(IsAdded(),Panic(ETimNotAdded));
	iTimer.HighRes(iStatus, aInterval);
	SetActive();
	}




EXPORT_C void CTimer::ConstructL()
/**
Constructs a new asynchronous timer.

The function must be called before any timer requests (i.e. calls to
RTimer::After() or RTimer::At()) can be made.

Since it is protected, it cannot be called directly by clients of CTimer
derived classes. Typically, a derived class makes a base call to this function
in the second phase of two-phase construction; i.e. the derived class defines 
and implements its own ConstructL() function within which it makes a base 
call to CTimer::ConstructL().
*/
	{

	TInt r=iTimer.CreateLocal();
	if (r!=KErrNone)
		User::Leave(r);
	}




EXPORT_C void CTimer::DoCancel()
//
// Cancel the timer.
//
	{

	iTimer.Cancel();
	}




EXPORT_C CPeriodic *CPeriodic::New(TInt aPriority)
/**
Allocates and constructs a CPeriodic object - non-leaving.

Specify a high priority so the callback function is scheduled as soon as
possible after the timer events complete.

@param aPriority The priority of the active object. If timing is critical, 
                 it should be higher than that of all other active objects
                 owned by the scheduler.
                 
@return Pointer to new CPeriodic object. The object is initialised and added 
        to the active scheduler. This value is NULL if there is insufficient
        memory.
*/
	{

	CPeriodic *pP=new CPeriodic(aPriority);
	if (pP)
		{
		TRAPD(r,pP->ConstructL());
		if (r==KErrNone)
			CActiveScheduler::Add(pP);
		else
			{
			delete pP;
			pP=NULL;
			}
		}
	return pP;
	}




EXPORT_C CPeriodic *CPeriodic::NewL(TInt aPriority)
/**
Allocates and constructs a CPeriodic object - leaving.

Specify a high priority so the callback function is scheduled as soon as
possible after the timer events complete.

@param aPriority The priority of the active object. If timing is critical, 
                 it should be higher than that of all other active objects
                 owned by the scheduler.

@return Pointer to new CPeriodic object. The object is initialised and added 
        to the active scheduler.
                          
@leave KErrNoMemory There is insufficient memory to create the object.
*/
	{

	return((CPeriodic *)User::LeaveIfNull(New(aPriority)));
	}




EXPORT_C CPeriodic::CPeriodic(TInt aPriority)
	: CTimer(aPriority)
/**
Protected constructor with priority.

Use this constructor to set the priority of the active object.

Classes derived from CPeriodic must define and provide a constructor through 
which the priority of the active object can be passed. Such a constructor 
can call CPeriodic's constructor in its constructor initialisation list.

@param aPriority The priority of the timer.
*/
	{
	}




EXPORT_C CPeriodic::~CPeriodic()
/**
Destructor.

Frees resources prior to destruction.
*/
	{
	}




EXPORT_C void CPeriodic::Start(TTimeIntervalMicroSeconds32 aDelay,TTimeIntervalMicroSeconds32 anInterval,TCallBack aCallBack)
/**
Starts generating periodic events.

The event calls the protected RunL() function, 
which in turn calls the function specified by aCallBack. The first event is 
generated after aDelay microseconds; subsequent events are generated regularly 
thereafter at intervals of anInterval microseconds.

The TCallBack contains a function pointer and a TAny* pointer. The function 
will be repeatedly called with the pointer as a parameter.

Once started, periodic events are generated until the CPeriodic object is 
destroyed.

Notes:

1. The callback function will be run as soon as possible after the initial delay, 
   and after each period.

2. The callback may be delayed because the RunL() of another active object, with 
   the deepest nesting-level active scheduler on the same thread, is running 
   when the event occurs: this cannot be avoided, but can be minimised by making 
   all RunL()s of short duration.

3. The callback may be delayed because other, higher-priority, active objects 
   are scheduled instead. This can be avoided by giving the CPeriodic a very 
   high priority.

@param aDelay     The delay from the Start() function to the generation of the 
                  first event, in microseconds.
@param anInterval The interval between events generated after the initial
                  delay, in microseconds.
@param aCallBack  A callback specifying a function to be called when the CPeriodic 
                  is scheduled after a timer event.
                  
@panic E32USER-CBase 52, if anInterval is negative.
@panic E32USER-CBase 53, if aDelay is negative.
*/
	{

	__ASSERT_ALWAYS(anInterval.Int()>=0,Panic(ETimIntervalNegativeOrZero));
	__ASSERT_ALWAYS(aDelay.Int()>=0,Panic(ETimDelayNegative));
	iInterval=anInterval.Int();
	iCallBack=aCallBack;
	After(aDelay);
	}

EXPORT_C void CPeriodic::RunL()
//
// Handle completion by issuing the next request and then calling back.
//
	{

	After(iInterval);
	iCallBack.CallBack();
	}




EXPORT_C CHeartbeat::CHeartbeat(TInt aPriority)
	: CTimer(aPriority)
/**
Protected constructor with a priority. Use this constructor to set the priority 
of the active object.

Classes derived from CHeartbeat must define and provide a constructor through 
which the priority of the active object can be passed. Such a constructor 
can call CHeartbeat's constructor in its constructor initialisation list.

@param aPriority The priority of the timer.
*/
	{}




EXPORT_C CHeartbeat *CHeartbeat::New(TInt aPriority)
/**
Allocates and constructs a CHeartbeat object - non-leaving.

Specify a high priority so the callback function is scheduled as soon as
possible after the timer events complete.

@param aPriority The priority of the active object. If timing is critical, 
                 it should be higher than that of all other active objects
                 owned by the scheduler.
                 
@return Pointer to new CHeartbeat object. The object is initialised and added 
        to the active scheduler. This value is NULL if insufficient memory was
        available.
*/
	{

	CHeartbeat *pP=new CHeartbeat(aPriority);
	if (pP)
		{
		TRAPD(r,pP->ConstructL());
		if (r==KErrNone)
			CActiveScheduler::Add(pP);
		else
			{
			delete pP;
			pP=NULL;
			}
		}
	return pP;
	}




EXPORT_C CHeartbeat *CHeartbeat::NewL(TInt aPriority)
/**
Allocates and constructs a CHeartbeat object - leaving.

Specify a high priority so the callback function is scheduled as soon as
possible after the timer events complete.

@param aPriority The priority of the active object. If timing is critical, 
                 it should be higher than that of all other active objects
                 owned by the scheduler.
                 
@return Pointer to new CHeartbeat object. The object is initialised and added 
        to the active scheduler.
*/
	{

	return((CHeartbeat *)User::LeaveIfNull(New(aPriority)));
	}




EXPORT_C CHeartbeat::~CHeartbeat()
/**
Destructor.

Frees resources prior to destruction.
*/
	{}




EXPORT_C void CHeartbeat::Start(TTimerLockSpec aLock, MBeating *aBeating)
/**
Starts generating heartbeat events. The event results in calls to the Beat() 
and Synchronize() functions specified by aBeating.

The first event is generated on the first fraction of a second corresponding 
to aLock that occurs after Start() has returned; subsequent events are generated 
regularly thereafter at one second intervals on the second fraction specified 
by aLock.

The aBeating mixin must be written by the user. Most of the time, its Beat() 
function is called which trivially updates the tick count. Occasionally, synchronisation 
is lost, and the Synchronize() function is called instead: this must find 
out from the system time how many ticks should have been counted, and update 
things accordingly.

Once started, heartbeat events are generated until the CHeartbeat object is 
destroyed.

@param aLock    The fraction of a second at which the timer completes.
@param aBeating Provides the Beat() and Synchronize() functions.
*/
	{

	iBeating=aBeating;
	iLock=aLock;
	Lock(aLock);
	}




EXPORT_C void CHeartbeat::RunL()
//
// Handle completion
//
	{
	
	TRequestStatus stat=iStatus;
	Lock(iLock);
	if (stat==KErrNone)
		iBeating->Beat();
	else
		iBeating->Synchronize();
	}

