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
// e32\euser\cbase\ub_act.cpp
// 
//

#include "ub_std.h"
#include "us_data.h"

#ifdef __SMP__
#include <e32atomics.h>
#endif



#pragma warning( disable : 4705 )	// statement has no effect
EXPORT_C CActive::CActive(TInt aPriority)
/**
Constructs the active object with the specified priority.

Derived classes must define and implement a constructor through which the
priority can be specified. A typical implementation calls this active object
constructor through a constructor initialization list.

@param aPriority An integer specifying the priority of this active object.
                 CActive::TPriority defines a standard set of priorities.
*/
	{
	iLink.iPriority=aPriority;
	}
#pragma warning( default : 4705 )




EXPORT_C CActive::~CActive()
/**
Frees resources prior to destruction.

Specifically, it removes this active object from the active scheduler's
list of active objects.

Typically, a derived class calls Cancel() in its destructor.

@panic E32USER-CBase 40 if the active object has an outstanding request when
       the destructor is called,

@see CActive::Cancel
*/
	{
	__ASSERT_ALWAYS(!(iStatus.iFlags&TRequestStatus::EActive),Panic(EReqStillActiveOnDestruct));
	if (IsAdded())
		iLink.Deque();
	}




EXPORT_C void CActive::Cancel()
/**
Cancels the wait for completion of an outstanding request.

If there is no request outstanding, then the function does nothing.

If there is an outstanding request, the function:

1. calls the active object's DoCancel() function, provided by
   the derived class to implement cancellation of the request.

2. waits for the cancelled request to complete; this must complete as fast as
   possible.

3. marks the active object's request as complete (i.e. the request is no longer
   outstanding).

@see CActive::DoCancel
@see CActive::IsActive
@see CActive::~CActive
@see User::WaitForRequest
*/
	{
	if (iStatus.iFlags&TRequestStatus::EActive)
		{
		DoCancel();
		User::WaitForRequest(iStatus);
    	iStatus.iFlags&=~(TRequestStatus::EActive | TRequestStatus::ERequestPending); //iActive=EFalse;
		}
	}




EXPORT_C void CActive::Deque()
/**
Removes the active object from the active scheduler's list of active objects.

Before being removed from the active scheduler's list, the function cancels
any outstanding request.

@see CActive::Cancel
*/
	{
	__ASSERT_ALWAYS(IsAdded(),Panic(EActiveNotAdded));
	Cancel();
	iLink.Deque();
	iLink.iNext=NULL; // Must do this or object cannot be re-queued
	}




EXPORT_C void CActive::SetActive()
/**
Indicates that the active object has issued a request and that
it is now outstanding.

Derived classes must call this function after issuing a request.

A request is automatically marked as complete (i.e. it is no longer
outstanding) by:

1. the active scheduler, immediately before it calls the active object's RunL()
   function.

or

2. the active object within the implementation of the Cancel() function.

E32USER-CBase 46 panics may occur if an active object is set active but
no request is made on its TRequestStatus, or vice-versa. This panic happens
no earlier than the next time that the active scheduler assesses which
objects are ready to run, and may happen much later. This panic is termed 
a 'stray event' because it indicates that some entity has sent an event 
to the active scheduler thread, but this thread is not in a state ready to handle it.

@see CActive::IsActive
@see CActive::RunL
@see CActive::Cancel

@panic E32USER-CBase 42 if this active object is already active
@panic E32USER-CBase 49 if this active object has not been added to the active
       scheduler.
*/
	{
	__ASSERT_ALWAYS(!(iStatus.iFlags&TRequestStatus::EActive),Panic(EReqAlreadyActive));
	__ASSERT_ALWAYS(IsAdded(),Panic(EActiveNotAdded));
	iStatus.iFlags|=TRequestStatus::EActive;
	}




EXPORT_C void CActive::SetPriority(TInt aPriority)
/**
Sets the priority of the active object.

@param aPriority An integer specifying the new priority of this active object.
                 CActive::TPriority defines a standard set of priorities.

@panic E32USER-CBase 50 if this function is called while a request
       is outstanding.
*/
	{
	__ASSERT_ALWAYS(!(iStatus.iFlags&TRequestStatus::EActive),Panic(ESetPriorityActive));
	iLink.iPriority=aPriority;
	if (IsAdded())
		{
		Deque();
		iLink.iNext=NULL; // Make this not added
		CActiveScheduler::Add(this);
		}
	}




EXPORT_C TInt CActive::RunError(TInt aError)
/**
Handles a leave occurring in the request completion event handler RunL().

The active scheduler calls this function if this active object's RunL()
function leaves. This gives this active object the opportunity to perform
any necessary cleanup.

A derived class implementation should handle the leave and return KErrNone.
Returning any other value results in the active scheduler function
CActiveScheduler::Error() being called.

The default implementation simply returns the leave code.

Note that if the active scheduler is to handle the error, a suitably derived
CActiveScheduler::Error() function must be supplied.

@param aError The leave code

@return The default implementation returns aError. A derived class
        implementation should return KErrNone, if it handles the leave;
        otherwise it should return any suitable value to cause the handling
        of the error to be propagated back to the active scheduler.

@see CActiveScheduler::Error
*/
	{
	return aError;
	}




/**
Extension function


*/
EXPORT_C TInt CActive::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CBase::Extension_(aExtensionId, a0, a1);
	}




EXPORT_C CIdle* CIdle::New(TInt aPriority)
/**
Allocates and initialises an Idle time active object and adds it to the active
scheduler.

@param aPriority An integer specifying the priority of this active object.
                 It must be lower than that of all other active objects on
                 the active scheduler.
                 The value CActive::TPriority::EPriorityIdle is recommended.

@return Pointer to the new Idle time active object, or NULL if the object could
        not be created.
*/
	{
	CIdle *pI=new CIdle(aPriority);
	if (pI!=NULL)
		CActiveScheduler::Add(pI);
	return(pI);
	}




EXPORT_C CIdle* CIdle::NewL(TInt aPriority)
/**
Allocates and initialises an Idle time active object, adds it to the active
scheduler, but leaves on failure.

@param aPriority An integer specifying the priority of this active object.
                 It must be lower than that of all other active objects on
                 the active scheduler.
                 The value CActive::TPriority::EPriorityIdle is recommended.

@return Pointer to the new Idle time active object.
*/
	{
	CIdle *pI=new(ELeave) CIdle(aPriority);
	CActiveScheduler::Add(pI);
	return(pI);
	}





EXPORT_C CIdle::CIdle(TInt aPriority)
	: CActive(aPriority)
/**
Protected constructor taking a priority value.

Sets this active object's priority value.

@param aPriority The active object priority value.
*/
	{}




EXPORT_C CIdle::~CIdle()
/**
Frees resources prior to destruction.

Specifically, it cancels any outstanding request.
*/
	{
	Cancel();
	}




EXPORT_C void CIdle::Start(TCallBack aCallBack)
/**
Starts the background task.

The background task is encapsulated in the callback. The function represented
by this callback is called every time this Idle time active object is scheduled
to run.

The callback function should be structured to perform a background task in
many increments, i.e. it should voluntarily relinquish control (i.e. return)
after a suitable time interval to allow other, higher priority events to be
handled.

If the callback function has further work to do, it should return a true value.
This ensures that the active object is scheduled to run again later.

Once the callback function has finally completed its work, it should return
a false value. The active object is then no longer scheduled to run.

@param aCallBack A callback object encapsulating a function which is called
                 when no higher priority active object is ready to run.
*/
	{
	iCallBack=aCallBack;
	iStatus=KRequestPending;
	TRequestStatus *pS=(&iStatus);
	User::RequestComplete(pS,0);
	SetActive();
	}




EXPORT_C void CIdle::RunL()
/**
Handles this idle active object's request completion event.

It is called when nothing of a higher priority can be scheduled.

@see CActive::RunL
*/
	{
	if (iCallBack.CallBack())
		Start(iCallBack);
	}




EXPORT_C void CIdle::DoCancel()
/**
Implements the cancellation of an outstanding request.

This function is called by the active object's Cancel() function.

@see CActive::DoCancel
*/
	{
	}




EXPORT_C void CAsyncOneShot::Call()
/**
Queues this active object to be run once.

@panic E32USER-CBase 2 In debug builds only, if this active object has not
       already been added to the active scheduler.
*/
	{
	__ASSERT_DEBUG(IsAdded(),Panic(ECAsyncOneShotNotAdded));
	TRequestStatus *pS=(&iStatus);
	iStatus = KRequestPending;
	SetActive();
	iThread.RequestComplete(pS,0);
	}




EXPORT_C void CAsyncOneShot::DoCancel()
/**
Implements cancellation of an outstanding request.

The class provides an empty implementation.

This is called by the destructor.
*/
	{
	// Empty
	}




EXPORT_C CAsyncOneShot::CAsyncOneShot(TInt aPriority)
	:CActive(aPriority)
/**
Constructor taking a priority value.

Specifically, the constructor:

1. sets this active object's priority value

2. opens a handle to the current thread to ensure that the thread cannot be
   closed until this CAsyncOneShot object is destroyed

3. adds this active object to the current active scheduler.

@param aPriority The active object priority value. CActive::TPriority defines
                 a standard set of priorities.

@panic E32USER-CBase 93 if the attempt to open a handle to the current thread
       fails.
*/
	{
	Setup();
	}




void CAsyncOneShot::Setup()
//
// ensures that we are added to the Scheduler.
//
	{
	// No error checking was done initially.  As this function is called from
	// the c'tor, there is no way to fix it properly without breaking BC.  So
	// we panic if something goes wrong (should only happen in extreme
	// circumstances if the kernel heap is exhausted or heavily fragmented).
	__ASSERT_ALWAYS(iThread.Duplicate(RThread()) == KErrNone, Panic(EAsyncOneShotSetupFailed));

	// Add ourself to the current active scheduler
	// This is because we might be being used as an inter thread call
	// we need to make sure that we're on the correct scheduler for
	// the RThread were going to duplicate.
	CActiveScheduler::Add(this);
	}




EXPORT_C CAsyncOneShot::~CAsyncOneShot()
/**
Frees resources prior to destruction.

Specifically, it closes the handle to the current thread.

@see CActive::~CActive
*/
	{
	Cancel();
	iThread.Close();
	}




EXPORT_C CAsyncCallBack::CAsyncCallBack(TInt aPriority)
	: CAsyncOneShot(aPriority), iCallBack(NULL)
/**
Constructor taking a priority value.

Specifically, the constructor sets this active object's priority value through
a call to the base class constructor in its ctor list.

No call back is set, which means that it must be set subsequently through
a call to the Set() function.

@param aPriority The active object priority value. CActive::TPriority defines
                 a standard set of priorities.

@see CAsyncCallBack::Set
*/
	{
	}




EXPORT_C CAsyncCallBack::CAsyncCallBack(const TCallBack& aCallBack, TInt aPriority)
	: CAsyncOneShot(aPriority), iCallBack(aCallBack)
/**
Constructor taking a priority value and a callback.

Specifically, the constructor:

1. sets this active object's priority value through a call to the base class
   constructor in its ctor list

2. sets the callback; the function encapsulated by the callback is called when
   this active object is scheduled to run.

@param aCallBack A reference to a callback object encapsulating a function
                 which is called when this active object is ready to run.
                 The constructor takes a copy of this callback object, which
                 means that it can be safely discarded after construction.
@param aPriority The active object priority value.
*/
	{
	}




EXPORT_C CAsyncCallBack::~CAsyncCallBack()
/**
Destructor.
*/
	{
	}




EXPORT_C void CAsyncCallBack::CallBack()
/**
Queues this active object to be run, if it is not already queued.
*/
	{
	if (!IsActive())
		Call();
	}




EXPORT_C void CAsyncCallBack::Set(const TCallBack& aCallBack)
/**
Sets the call back.

@param aCallBack A reference to a callback object encapsulating a function
                 which is called when this active object is ready to run.

@panic E32USER-CBase 1 if the active object is currently active.
*/
	{
	__ASSERT_ALWAYS(!IsActive(), Panic(ECAsyncCBIsActive));
	iCallBack = aCallBack;
	}




void CAsyncCallBack::RunL()
/**
Calls the callback function.

@see TCallBack::CallBack
*/
	{
	iCallBack.CallBack();
	}




struct CActiveScheduler::TLoop
	{
	TLoop* iNext;
	CActiveScheduler::TLoopOwner* iOwner;
	TCallBack iCallback;
	TInt iExitCode;
	};

CActiveScheduler::TLoopOwner* const KLoopNoOwner=reinterpret_cast<CActiveScheduler::TLoopOwner*>(1);
CActiveScheduler::TLoopOwner* const KLoopInactive=0;




EXPORT_C CActiveSchedulerWait::CActiveSchedulerWait()
/**
Default constructor.
*/
	{}





EXPORT_C CActiveSchedulerWait::~CActiveSchedulerWait()
/**
Ensures that the attached scheduler loop, and all nested loops, are stopped
prior to destruction.

@see AsyncStop()
*/
	{
	if (IsStarted())
		AsyncStop();
	}




EXPORT_C void CActiveSchedulerWait::Start()
/**
Starts a new wait loop under the control of the current active scheduler.

Compared with CActiveScheduler::Start(), this object owns control of
the scheduling loop that is started, and that loop can only be stopped
by using this objects AsyncStop() function or the CActiveScheduler::Halt()
function. Start() only returns when either of thos has occurred.

This is the preferred way to start a nested wait loop. Typically, a nested
wait loop is used when the handling of a completed event in an active object
requires processing further events from the other active objects before it
can complete. This is a form of modal processing.

@panic E32USER-CBase 44 if the thread does not have an active scheduler installed.
@panic E32USER-CBase 91 if this object has already been started.

@see CActiveSchedulerWait::AsyncStop
@see CActiveSchedulerWait::IsStarted
@see CActiveScheduler::Start
@see CActiveScheduler::Halt
*/
	{
	__ASSERT_ALWAYS(!IsStarted(), Panic(EActiveSchedulerWaitAlreadyStarted));		// can only start a CActiveSchedulerWait if it isn't already started
	CActiveScheduler::Start(&iLoop);
	}




EXPORT_C void CActiveSchedulerWait::AsyncStop()
/**
Stops the scheduling loop owned by this object.

Note that the corresponding call to Start() only returns once all nested
scheduler loops have stopped.

@panic E32USER-CBase 92 if the wait object has not been started.
*/
	{
	AsyncStop(TCallBack());
	}




EXPORT_C void CActiveSchedulerWait::AsyncStop(const TCallBack& aCallMeWhenStopped)
/**
Stops the scheduling loop owned by this object, specifying a callback.

This version of AsyncStop() provides a callback which is invoked immediately
after the scheduler loop actually stops before the corresponding call
to Start() returns.

Note that the corresponding call to Start() only returns once all nested
scheduler loops have stopped.

@param aCallMeWhenStopped The callback to invoke when the scheduler loop exits.

@panic E32USER-CBase 92 if the wait object has not been started.
 */
	{
	CActiveScheduler::TLoopOwner loop=iLoop;
	__ASSERT_ALWAYS(loop, Panic(EActiveSchedulerWaitNotStarted));		// can only stop a CActiveSchedulerWait if it's started
	__ASSERT_DEBUG(loop->iOwner==&iLoop, User::Invariant());

	loop->iCallback = aCallMeWhenStopped;
	loop->iOwner = KLoopInactive;			// disconnect from owner
	iLoop = 0;
	}




EXPORT_C TBool CActiveSchedulerWait::CanStopNow() const
/**
Reports whether stopping will have immediate effect.

This returns an indication of whether a call to AsyncStop() would be
expected to stop the scheduler loop immediately, or whether it will
have to wait until nested scheduler loops have stopped. This may alter
which version of AsyncStop() you would want to call.

@return Boolean indicating if the scheduling loop would stop immediately.

@panic E32USER-CBase 92 if the wait object has not been started.

@see CActiveSchedulerWait::Start
@see CActiveSchedulerWait::AsyncStop
*/
 	{
	__ASSERT_ALWAYS(IsStarted(), Panic(EActiveSchedulerWaitNotStarted));		// Scheduler must be running
	for (CActiveScheduler::TLoop* loop=GetActiveScheduler()->iStack; loop; loop=loop->iNext)
		{
		if (loop==iLoop)
			return ETrue;
		if (loop->iOwner != KLoopInactive)
			break;
		}
	return EFalse;
	}



EXPORT_C CActiveScheduler::CActiveScheduler()
	: iActiveQ(_FOFF(CActive,iLink))
/**
Constructs an active scheduler.

After construction, the scheduler should be installed.

@see CActiveScheduler::Install
*/
	{}




EXPORT_C CActiveScheduler::~CActiveScheduler()
/**
Frees resources prior to destruction.

Specifically, it removes all active objects from the active scheduler's list
of active objects.

An active scheduler should only be destroyed when the top-level call to Start()
has returned.

@see CActiveScheduler::Start
@see CActiveScheduler::Stop
*/
	{
	while (!iActiveQ.IsEmpty())
		iActiveQ.First()->Deque();
	if (GetActiveScheduler()==this)
		SetActiveScheduler(NULL);
	}




EXPORT_C void CActiveScheduler::Install(CActiveScheduler *aManager)
/**
Installs the specified active scheduler as the current active scheduler.

The installed active scheduler now handles events for this thread.

The current active scheduler can be uninstalled by passing a NULL pointer.

@param aManager A pointer to the active scheduler to be installed.
                If this is NULL, the current active scheduler is uninstalled.

@panic E32USER-CBase 43 if If there is already an installed active scheduler.
*/
	{
	if (aManager!=NULL)
		__ASSERT_ALWAYS(GetActiveScheduler()==NULL,Panic(EReqManagerAlreadyExists));
	SetActiveScheduler(aManager);
	}




EXPORT_C void CActiveScheduler::Add(CActive *aRequest)
/**
Adds the specified active object to the current active scheduler.

An active object can be removed from an active scheduler either by
destroying the active object or by using its Deque() member function.

@param aRequest Pointer to the active object to be added.

@panic E32USER-CBase 41 if the active object aRequest has already been added
       to the current active scheduler.
@panic E32USER-CBase 48 if aRequest is NULL.
@panic E32USER-CBase 44 if the thread does not have an installed
       active scheduler.

@see CActive::Deque
*/
	{
	CActiveScheduler *pS=GetActiveScheduler();
	__ASSERT_ALWAYS(pS!=NULL,Panic(EReqManagerDoesNotExist));
	__ASSERT_ALWAYS(aRequest,Panic(EReqNull));
	__ASSERT_ALWAYS(!aRequest->IsAdded(),Panic(EReqAlreadyAdded));
	pS->iActiveQ.Add(*aRequest);
	}




EXPORT_C void CActiveScheduler::WaitForAnyRequest()
/**
Wait for an asynchronous request to complete.

The default implementation just calls User::WaitForAnyRequest().

Derived classes can replace this. Typically, this would be done to implement
code for maintaining an outstanding request; this would be followed by a call
to User::WaitForAnyRequest().

@see User::WaitForAnyRequest
*/
	{
	User::WaitForAnyRequest();
	}




EXPORT_C void CActiveScheduler::Start()
/**
Starts a new wait loop under the control of the current active scheduler.

At least one active object, with an outstanding request, must be added
to the scheduler before the wait loop is started, otherwise no events
will occur and the thread will hang, or any events that do occur will be
counted as stray signals, raising a panic.

While Start() is executing, user code runs only:

1. in the RunL() function of active objects known to the current active scheduler

2. in the RunError() function of an active object that leaves from its RunL()

3. in the current active scheduler’s Error() function, if an active object’s
   RunError() returns an error code.

Start() returns only when a corresponding Stop() or Halt() is issued.

Although this can be used to start a nested wait loop, this API is deprecated
for that specific functionality, and a CActiveSchedulerWait object should be
used instead.

(Note that a nested wait loop is used when the handling of a completed event
 in an active object requires the processing of further events from the other
 active objects before it can complete. This is a form of modal processing.)

@panic E32USER-CBase 44 if the thread does not have an active
       scheduler installed.

@see CActiveScheduler::Stop
@see CActiveScheduler::Halt
@see CActive::RunL
@see CActive::RunError
@see CActiveScheduler::Error
@see CActiveSchedulerWait
*/
	{
	Start(KLoopNoOwner);
	}




void CActiveScheduler::Start(TLoopOwner* aOwner)
/**
@internalComponent

Start a new nesting level
*/
	{
	CActiveScheduler* pS=GetActiveScheduler();
	__ASSERT_ALWAYS(pS!=NULL, Panic(EReqManagerDoesNotExist));

	// Instantiate the local loop control
	TLoop loop;
	loop.iOwner=aOwner;
	if (aOwner != KLoopNoOwner)
		*aOwner=&loop;
	loop.iNext=pS->iStack;
	pS->iStack=&loop;
	loop.iExitCode=0;

	// Run the scheduler loop
#if 1
	// FIXME!!! Will support old-style leave-from-Error() transiently
	// in order to avoid simultaneous integration requirement.
	// This should be reverted to the conditionally excluded code once
	// fixes have been made elsewhere
	TRAPD(r,pS->Run(loop.iOwner));
	if (r!=KErrNone)
		{
		loop.iExitCode = r;
		TLoopOwner* owner=loop.iOwner;
		if (TUint(owner) > TUint(KLoopNoOwner))
			*owner = NULL;
		}
#else	// fixme
#ifdef _DEBUG
	// catch old-style bad behaviour - leaving from Error()
	TRAPD(r,pS->Run(loop.iOwner));
	__ASSERT_DEBUG(r==KErrNone,User::Invariant());
#else
	pS->Run(loop.iOwner);
#endif
#endif

	pS->iStack=loop.iNext;
	loop.iCallback.CallBack();
	// propagate the exit-code via a leave (yuck, but blame BAFL & co.)
	if (loop.iExitCode)
		User::Leave(loop.iExitCode);
	}

/*
@internalComponent

Dummy Function. This is used as a dummy object to put onto the cleanupstack in order
to check for imbalance in the CActiveScheduler::DoRunL.
 */
void DummyFunc(TAny* /*aPtr*/)
	{}


#ifdef __LEAVE_EQUALS_THROW__
/**
@internalComponent

Start dispatching request completions.

Stop when aLoop becomes 'Inactive'

This version uses the implementation of TRAP/Leave in terms of C++ exceptions.
We have to make sure here that we don't call Active Object's RunError() or Active Scheduler's Error()
while we are still in exception (within 'catch' brackets), as it can lead to nested-exceptions scenario.
It is not fatal by default, but if two nested exceptions are due to OOM condition, RVCT implementation
of exception will run out of emergency buffers and terminate the thread.
*/
void CActiveScheduler::Run(TLoopOwner* const volatile& aLoop)
	{
	CActive * volatile curr_obj = 0;
	TBool leaveException = EFalse;
	TInt exceptionReason = 0;
	do
		{
		try	{
			__WIN32SEHTRAP
			TTrapHandler* t = User::MarkCleanupStack();
			
#ifdef _DEBUG
			//We cache the cleanupstack here do avoid repeated exec calls in DoRunL
			TCleanupTrapHandler *pH=(TCleanupTrapHandler *)GetTrapHandler();
			CCleanup* cleanupPtr=NULL;
			TCleanupBundle cleanupBundle;

			if(pH!=NULL) // test whether there's a CleanupTrapHandler installed
				{
				CCleanup& ccleanup =pH->Cleanup();
				//Store pointer as need the scope of ccleanup increased
				cleanupPtr = &ccleanup; 
				cleanupBundle.iCleanupPtr = cleanupPtr;
				
				//Push a dummy item onto the stack - we check it after the AO's RunL has returned
				//and we check to make sure its still at the top.
				ccleanup.PushL(TCleanupItem(DummyFunc, &(cleanupBundle.iDummyInt)));
				
				DoRunL(aLoop, curr_obj, &cleanupBundle);

				//Dummy Int must (will) be at the top
				//Cleanup our stack
				cleanupPtr->Pop(1);
				} 
			else // no cleanup stack installed
				{
				DoRunL(aLoop, curr_obj, NULL);
				}
			
#else
			DoRunL(aLoop, curr_obj, NULL);
#endif
			
			User::UnMarkCleanupStack(t);
			__WIN32SEHUNTRAP
			return;
			}
		catch (XLeaveException& l)
			{
			Exec::LeaveEnd();
			leaveException = ETrue;
			exceptionReason = l.Reason();
			}
		catch (...)
			{
			User::Invariant();
			}

		if (leaveException)
			{
			if (exceptionReason != KErrNone)
				{
				TInt r = curr_obj->RunError(exceptionReason);
				if (r != KErrNone)
					Error(r);
				}
			leaveException = EFalse;
			}

		} while (aLoop != KLoopInactive);
	}

#else

/**
@internalComponent

Start dispatching request completions.

Stop when aLoop becomes 'Inactive'

This version uses the original implementation of TRAP/Leave.
*/
void CActiveScheduler::Run(TLoopOwner* const volatile& aLoop)
	{
	CActive * volatile curr_obj = 0;
	do
		{
		// explicitly expand the TRAPD macro here to enable single-step debugging
		// of the scheduler loop
		TInt r;
		TTrap trap;
		if (trap.Trap(r)==0)
			{
#ifdef _DEBUG
			//We cache the cleanupstack here do avoid repeated exec calls in DoRunL
			TCleanupTrapHandler *pH=(TCleanupTrapHandler *)GetTrapHandler();
			CCleanup* cleanupPtr=NULL;
			TCleanupBundle cleanupBundle;

			if(pH!=NULL) // test whether there's a CleanupTrapHandler installed
				{
				CCleanup& ccleanup =pH->Cleanup();
				//Store pointer as need the scope of ccleanup increased
				cleanupPtr = &ccleanup; 
				cleanupBundle.iCleanupPtr = cleanupPtr;
				
				//Push a dummy item onto the stack - we check it after the AO's RunL has returned
				//and we check to make sure its still at the top.
				ccleanup.PushL(TCleanupItem(DummyFunc, &(cleanupBundle.iDummyInt)));
				
				DoRunL(aLoop, curr_obj, &cleanupBundle);

				//Dummy Int must (will) be at the top
				//Cleanup our stack
				cleanupPtr->Pop(1);
				} 
			else // no cleanup stack installed
				{
				DoRunL(aLoop, curr_obj, NULL);
				}
#else
			DoRunL(aLoop, curr_obj, NULL);
#endif
			
			TTrap::UnTrap();
			return;		// exit level
			}
		if (r != KErrNone)
			{
			r = curr_obj->RunError(r);
			if (r != KErrNone)
				Error(r);
			}
		} while (aLoop != KLoopInactive);
	}
#endif

#ifndef __CACTIVESCHEDULER_MACHINE_CODED__
/**
@internalComponent

The inner active scheduler loop. This repeatedly waits for a signal and then
dispatches the highest priority ready active object. The loop terminates either
if one of the RunLs stops the current active scheduler level or leaves.

Stop when aLoop becomes 'Inactive'
@panic EClnCheckFailed 90 This will panic when the RunL has left the cleanup stack in an unbalanced state.
*/
#ifdef _DEBUG
void CActiveScheduler::DoRunL(TLoopOwner* const volatile& aLoop, CActive* volatile & aCurrentObj, TCleanupBundle* aCleanupBundlePtr)
#else
void CActiveScheduler::DoRunL(TLoopOwner* const volatile& aLoop, CActive* volatile & aCurrentObj, TCleanupBundle* /*aCleanupBundlePtr*/)
#endif
	{
	TDblQueIter<CActive> q(iActiveQ);
	do
		{
		WaitForAnyRequest();
		q.SetToFirst();
		CActive* pR;
		do
			{
			pR=q++;
			__ASSERT_ALWAYS(pR!=NULL,Panic(EReqStrayEvent));
			//if the line below panics it's either because you made a request but you haven't
			//SetActive the object (pR->iStatus.iFlags&TRequestStatus::EActive==0) or you didn't set the iStatus
			//to KRequestPending (pR->iStatus.iFlags&TRequestStatus::ERequestPending==0)
			__ASSERT_DEBUG(!(pR->iStatus.iFlags&TRequestStatus::EActive)==!(pR->iStatus.iFlags&TRequestStatus::ERequestPending),Panic(EReqStrayEvent));
			} while (!pR->IsActive() || pR->iStatus==KRequestPending);
#ifdef __SMP__
		__e32_memory_barrier();
#endif
		pR->iStatus.iFlags&=~(TRequestStatus::EActive | TRequestStatus::ERequestPending); //pR->iActive=EFalse;
		aCurrentObj = pR;
		pR->RunL();

#ifdef _DEBUG
		if(aCleanupBundlePtr!=NULL)
			{
			//If the following line panics, the RunL left the
			//cleanup stack in an umbalanced state.
			TInt* dummyInt = &(aCleanupBundlePtr->iDummyInt);
			aCleanupBundlePtr->iCleanupPtr->Check(dummyInt);
			}
#endif

		} while (aLoop != KLoopInactive);
	return;		// exit level
	}

#else

extern "C" void PanicStrayEvent()
	{
	Panic(EReqStrayEvent);
	}
#endif




EXPORT_C void CActiveScheduler::Stop()
/**
Stops the wait loop started by the most recent call to Start().

Typically, this is called by the RunL() of one of the scheduler’s active
objects. When this RunL() finishes, the scheduler’s wait loop terminates,
i.e. it does not wait for the completion of the next request.

It will not stop a wait loop started by a call
to CActiveSchedulerWait::Start().

Stop() may also be called from Error().

Note that stopping a nested wait loop is deprecated using this functionality,
use a CActiveSchedulerWait object instead.

@see CActiveSchedulerWait::Start
@see CActive::RunL
@see CActiveSchedulerWait::Error
@see CActiveSchedulerWait::AsyncStop
*/
	{
	CActiveScheduler *pS=GetActiveScheduler();
	__ASSERT_ALWAYS(pS!=NULL,Panic(EReqManagerDoesNotExist));

	for (CActiveScheduler::TLoop* loop=pS->iStack; loop; loop=loop->iNext)
		{
		if (loop->iOwner == KLoopNoOwner)
			{
			loop->iOwner=KLoopInactive;
			return;
			}
		}
	Panic(EReqTooManyStops);
	}




EXPORT_C void CActiveScheduler::Halt(TInt aExitCode) const
/**
Unilaterally terminates the current scheduler loop.

This causes the current scheduler loop to stop, whether it was started
using CActiveSchedulerWait::Start() or CActiveScheduler::Start(). It can
also trigger a leave from Start() if an exit code is provided. If the
current level has already been stopped, then this still records the exit code.

@param aExitCode If non-zero, the reason code reported by Start().
*/
	{
	CActiveScheduler::TLoop* loop=iStack;
	__ASSERT_ALWAYS(loop!=NULL,Panic(EReqTooManyStops));
	TLoopOwner* owner=loop->iOwner;
	if (TUint(owner) > TUint(KLoopNoOwner))
		*owner = NULL;
	loop->iOwner = KLoopInactive;			// disconnect from owner
	loop->iExitCode = aExitCode;
	}




EXPORT_C TInt CActiveScheduler::StackDepth() const
/**
Gets the current number of nested wait loops.

@return The number of nested calls to Start().
*/
	{
	TInt depth=0;
	for (CActiveScheduler::TLoop* loop=iStack; loop; loop=loop->iNext)
		++depth;
	return depth;
	}




EXPORT_C CActiveScheduler* CActiveScheduler::Current()
/**
Gets a pointer to the currently installed active scheduler.

@return A pointer to the active scheduler which is currently installed.
*/
	{
	return GetActiveScheduler();
	}




EXPORT_C void CActiveScheduler::Error(TInt /*aError*/) const
/**
Handles the result of a leave occurring in an active object’s RunL()
function.

An active scheduler always invokes an active object’s RunL()
function under a trap harness.

The default implementation must be replaced.

Any cleanup relevant to the possible causes of leaving should
be performed. If Stop() or Halt() is called from within this function, the
current wait loop terminates. This may be an appropriate response to
catastrophic error conditions.

@param aError The leave code propagated from the active object’s RunL() function

@panic E32USER-CBase 47 if the default implementation is invoked.

@see CActive::RunL
@see CActiveScheduler::Stop
@see CActiveScheduler::Halt
*/
	{
	Panic(EReqActiveObjectLeave);
	}




EXPORT_C TBool CActiveScheduler::RunIfReady(TInt& aError, TInt aMinimumPriority)
/**
@deprecated

Causes the RunL() function of at most one pending active object of priority
aMinimumPriority or greater to be run.

@param aError Error returned by called active object.
@param aMinimumPriority Minimum priority of active object to run.

@return EFalse if no active object's RunL() function was run, i.e. if there
        were no active objects of priority aMinimumPriority or greater pending.
*/
	{
	aError=KErrNone;
	CActiveScheduler* pS=GetActiveScheduler();
	if (pS!=NULL)
		{
		TDblQueIter<CActive> iterator(pS->iActiveQ);
		for (CActive* active=iterator++; (active!=NULL) && (active->Priority()>=aMinimumPriority); active=iterator++)
			{
			if (active->IsActive() && (active->iStatus!=KRequestPending))
				{
				active->iStatus.iFlags&=~(TRequestStatus::EActive | TRequestStatus::ERequestPending); //pR->iActive=EFalse;
				TRAP(aError, active->RunL());
				if (aError!=KErrNone)
					aError=active->RunError(aError);
				return ETrue;
				}
			}
		}
	return EFalse;
	}




EXPORT_C CActiveScheduler* CActiveScheduler::Replace(CActiveScheduler* aNewActiveScheduler)
/**
Allows the current active scheduler to be replaced, while retaining its active
objects.

@param aNewActiveScheduler The new active scheduler.

@return Previous active scheduler.
*/
	{
	__ASSERT_ALWAYS(aNewActiveScheduler!=NULL, Panic(EReqManagerDoesNotExist));
	CActiveScheduler* oldActiveScheduler=GetActiveScheduler();
	__ASSERT_ALWAYS(aNewActiveScheduler!=oldActiveScheduler, Panic(EActiveSchedulerReplacingSelf));
	if (oldActiveScheduler!=NULL)
		{
		// steal all the CActive objects from oldActiveScheduler (without canceling any of them)
		TPriQue<CActive>& oldActiveQ=oldActiveScheduler->iActiveQ;
		TPriQue<CActive>& newActiveQ=aNewActiveScheduler->iActiveQ;
		while (!oldActiveQ.IsEmpty())
			{
			CActive& active=*oldActiveQ.First();
			// call the lower-level function active.iLink.Deque() rather than active.Deque()
			// as the latter would also call active.Cancel() (which we don't want)
			active.iLink.Deque();
			newActiveQ.Add(active);
			}
		}
	SetActiveScheduler(aNewActiveScheduler);
	return oldActiveScheduler;
	}




EXPORT_C void CActiveScheduler::OnStarting()
/**
@removed

Dummy EXPORT for Binary Compatibility reasons.
This method is never called.
*/
	{
	}




EXPORT_C void CActiveScheduler::OnStopping()
/**
@removed

Dummy EXPORT for Binary Compatibility reasons.
This method is never called.
*/
	{
	}



EXPORT_C void CActiveScheduler::Reserved_1()
/**
@internalComponent

Dummy EXPORT for Binary Compatibility reasons.
*/
	{
	}



EXPORT_C void CActiveScheduler::Reserved_2()
/**
@internalComponent

Dummy EXPORT for Binary Compatibility reasons.
*/
	{
	}


/**
Extension function


*/
EXPORT_C TInt CActiveScheduler::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CBase::Extension_(aExtensionId, a0, a1);
	}
