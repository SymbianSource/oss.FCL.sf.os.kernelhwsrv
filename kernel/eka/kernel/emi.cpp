// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



#include <kernel/emi.h>
#include <kernel/kern_priv.h>

#ifdef __EMI_SUPPORT__
#ifdef __SMP__
#error EMI incompatible with SMP
#endif

// For use with DThread::iEmiFlags
// const TUint8 KThdCreated   = 0; 
const TUint8 KThdStarting     = 1;
const TUint8 KThdRunning      = 2;
const TUint8 KThdExiting      = 3;
const TUint8 KThdEMIStateMask = 3;
const TUint8 KThdUseOldExit   = 4;
const TUint8 KThdReStart      = 8;

// Static inishalization
TThreadExitMonitor   EMI::ExitMonitor    = NULL;
TThreadExitMonitor   EMI::OldExitMonitor = NULL;
TThreadStartMonitor  EMI::StartMonitor   = NULL;
TInt                 EMI::Generation     = 0;
NFastSemaphore*      EMI::ExitSemaphore  = NULL;
NTimer*              EMI::AfterIdleTimer = NULL;
EMI::TAfterIdleState EMI::AfterIdleState = ENone;
TInt                 EMI::AfterIdleDelay = 0;




/** 
@internalComponent
*/
TInt EMI::Init()
	{
	K::TheNullThread->iEmiFlags= KThdRunning;
	
	ExitSemaphore = new NFastSemaphore();
	if (!ExitSemaphore)
		return KErrNoMemory;

	TheScheduler.iSigma = new NThread();
	return (TheScheduler.iSigma == NULL) ? KErrNoMemory : KErrNone;
	}




//EXPORT_C TInt EMI::TaskEventLogging(TBool aLogging, TInt aLogSize, TThreadStartMonitor aStartMonitor, TThreadExitMonitor aExitMonitor)
/**
Starts or stops the scheduling event logging system.

The function can be called repeatedly to change the configuration of the scheduling event
logging system at any time.

It take pointers to two functions, known as the 'start monitor' and
the 'exit monitor'. 

The 'start monitor' is called when a Symbian OS thread, (a DThread), starts
executing, i.e. after the thread is initially resumed. It should be used
to initialise any data associated with the thread, and to set the thread
as loggable, through a call to SetThreadLoggable(), if scheduling event logging is required.

The 'exit monitor' is called when, and if, a Symbian OS thread, (a DThread) exits.
It should be used to free any resources associated with	the thread.

The initial state of the 'start monitor' and the 'exit monitor' is NULL.
Changing from this joint initial state, by setting one or both, causes all threads
to be initialised to a non-loggable state.

TaskEventLogging() should be protected from possible parallel
invocation, as it does not act atomically.

@param aLogging

	If true, scheduling event logging is enabled.  If false, scheduling event logging is disabled,
	but non-scheduling events can still be logged. The initial state is false.


@param aLogSize

	The number of entries in the internal event log.  If the event buffer is
	filled up, oldest events are deleted to make way for new events and the
	"event lost" flag is set on the event following the deleted event.  A
	value of 0 indicates no event log and no event logging is possible in this
	state.  If an event log already exists, and this value is a different
	size, then all events in the old log will be lost. 

	Changing the event size may result in a memory allocation.  On failure,
	this function returns KErrNoMemory, with logging turned off, and no event
	buffer. Monitors are left as they where before the call.


@param aStartMonitor

	A pointer to the function to be called when a new DThread is started. A value of 
	NULL means that no 'start monitor' function will be called.

	Once this function has been set, it is called for each DThread that
	already exists.  This includes the Idle thread, the Supervisor thread,
	and	DFC threads.  The function is also called for the Sigma thread, which
	accounts for time spent in non-loggable scheduled entities.  Such entities
	would include DThreads that are not fully initialised, or are being
	deleted.

	On return, the 'start monitor' function should return 0 for success or
	a negative value to indicate failure. Note that a failure here may result
	in the creation of the thread failing. Threads that are already running remain running.

@param aExitMonitor

	A pointer to the function to be called when (and if) a DThread
	exits. A value of NULL means that no 'exit monitor' function will be called.

	Before the exit monitor is called, the monitoring of the thread is 
	disabled, i.e. it is set to non-loggable, and all events for the thread are deleted
	from the event log. In reality, the deleted event records are altered so that
	they no longer make any reference to the deleted thread, 
	but instead refer to the Sigma thread. (This gives rise to the possibility of 
	seeing the Sigma thread switching to the Sigma thread) Note that a 
	non-visible thread will still trigger this routine. 

@pre Calling thread must be in a critical section. 
@pre Interrupts must be enabled. 
@pre Kernel must be unlocked. 
@pre No fast mutex can be held. 
@pre Call in a thread context. 
@pre Can be used in a device driver. 

@return KErrNone on success; KErrNoMemory on allocation failure; 
        KErrNotSupported if EMI is not compiled into the kernel.
*/
EXPORT_C TInt EMI::TaskEventLogging(TBool aLogging, TInt aLogSize, TThreadStartMonitor aStartMonitor, TThreadExitMonitor aExitMonitor)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"EMI::TaskEventLogging");	
	TInt error = TaskEventLogging(aLogging, aLogSize);
	if (error == KErrNone)
		ThreadMonitors(aStartMonitor, aExitMonitor);
	return error;
	}




/**
This is equivalent to calling the 4 parameter version of TaskEventLogging(), 
but with the last two parameters remaining unchanged.

@param aLogging If true, scheduling event logging is enabled.  If false,
                scheduling event logging is disabled, but non-scheduling events
                can still be logged. The initial state is false.
                
@param aLogSize The number of entries in the internal event log.  If the event buffer is
	            filled up, oldest events are deleted to make way for new events and the
	            "event lost" flag is set on the event following the deleted event.  A
	            value of 0 indicates no event log and no event logging is possible in this
	            state.  If an event log already exists, and this value is a different
	            size, then all events in the old log will be lost. 

	            Changing the event size may result in a memory allocation.  On failure,
	            this function returns KErrNoMemory, with logging turned off, and no event
	            buffer. Monitors are left as they where before the call.

@return KErrNone on success; KErrNoMemory on allocation failure; 
        KErrNotSupported if EMI is not compiled into the kernel.
        
@see TaskEventLogging()
*/
EXPORT_C TInt EMI::TaskEventLogging(TBool aLogging, TInt aLogSize)
	{

	// Alloc/DeAlloc event log.
	// ------------------------  
	// The event log is a ring buffer where iBufferStart is the first valid
	// record, and iBufferEnd is the last.  There is a floating empty slot.
	// To allow for this, one more record the aLogSize is allocated.
	// iBufferTail = next record to be read from.
	// iBufferHead = next record to be written to.

	TInt oldSize;
	TAny* eventBuffer;

	if (TheScheduler.iBufferStart)
		oldSize =(((TInt) TheScheduler.iBufferEnd -
			  (TInt) TheScheduler.iBufferStart) / sizeof(TTaskEventRecord));
	else
		oldSize = 0;

	if (aLogSize != oldSize)
		{
		// Disable logging
		TheScheduler.iLogging = EFalse;
		// Disable buffer use
		eventBuffer = TheScheduler.iBufferStart;
		NKern::Lock();
		TheScheduler.iBufferStart = NULL;
		Generation++;
		NKern::Unlock();

		if (oldSize > 0)
			Kern::Free(eventBuffer);
			
		if (aLogSize == 0)
			{
			aLogging = EFalse;

			TheScheduler.iBufferEnd = NULL;
			eventBuffer = NULL;
			}
		else if (aLogSize > 0)
			{
			aLogSize *= sizeof(TTaskEventRecord);
			eventBuffer = Kern::Alloc(aLogSize + sizeof(TTaskEventRecord));	// +1 empty slot
			if (eventBuffer != NULL)
				TheScheduler.iBufferEnd = (TAny*) ((TInt) eventBuffer + aLogSize);
			else
				{
				TheScheduler.iBufferEnd = NULL;
				TheScheduler.iLogging = EFalse;
				return KErrNoMemory;
				}
			TheScheduler.iBufferTail = eventBuffer;
			TheScheduler.iBufferHead = eventBuffer;
			}
		else
			{
			K::Fault(K::EBadLogSize);
			}

		//Re-enable buffer use;
		TheScheduler.iBufferStart = eventBuffer;
		}

	// Set Logging 
	// -----------

	TheScheduler.iLogging = aLogging;

	return KErrNone;
	}


/**
This is equivalent to calling the 4 parameter version of TaskEventLogging(), 
but with the first two parameters remaining unchanged.

@param aStartMonitor A pointer to the new 'start monitor' function.

@param aExitMonitor  A pointer to the new 'exit monitor' function.

@see TaskEventLogging()

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre ernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C void EMI::ThreadMonitors(TThreadStartMonitor aStartMonitor, TThreadExitMonitor aExitMonitor)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"EMI::ThreadMonitors");
	TInt threadNo;

	// Clear loggable
	// -------------- 

	DObjectCon & threadList = *(K::Containers[EThread]);

	// if monitors changed from initial state,
	// iterate thread list unsetting loggable flag.
	if ((StartMonitor == NULL) && (ExitMonitor == NULL)
			&& ((aStartMonitor != NULL) || (aExitMonitor != NULL)))
		{
		threadList.Wait();

		for (TInt i = threadList.Count()-1; i >=0; i--)
			((DThread*) threadList[i])->iNThread.ModifyAttributes(KThreadAttLoggable, 0);

		threadList.Signal();
		}

	// Set Monitors
	// ------------ 

	if ((StartMonitor == aStartMonitor) || (aStartMonitor == NULL))
		{
		StartMonitor = aStartMonitor;
		ExitMonitor = aExitMonitor;
		if ((aStartMonitor == NULL))
			{
			threadList.Wait();
			threadNo = threadList.Count();
			while (threadNo--)
				((DThread*) threadList[threadNo])->iEmiStartMonitor=NULL;
			threadList.Signal();
			}
		}
	else
		{
		TUint currentId = 0;
		TInt count;
		DThread* theThread;

		if (ExitMonitor == aExitMonitor)
			{
			StartMonitor = aStartMonitor;
			}
		else
			{
			// Newly created theads will use new monitors
			// but current threads can use old monitor until
			// new start monitor is called.

			threadList.Wait();

			NKern::Lock();
			OldExitMonitor = ExitMonitor;
			for (threadNo = threadList.Count()-1; threadNo >=0 ; threadNo--)
				{
				((DThread*) threadList[threadNo])->iEmiFlags |= KThdUseOldExit;
				NKern::PreemptionPoint();
				}
			StartMonitor = aStartMonitor;
			ExitMonitor = aExitMonitor;
			NKern::Unlock();

			// Unlocking and relocking acts as Premption point. 
			threadList.Signal();
			}

		// The Sigma thread;
		StartMonitor(TheScheduler.iSigma);

		threadList.Wait();

		// Iterate thead list, calling start monitor.
		theThread = (DThread *) threadList[0];
		currentId = theThread->iId;
		threadNo = -1;
		FOREVER
			{
			if (theThread->iId != currentId)
				{
				// from start find next thread with id>current
				count = threadList.Count();
				for (threadNo = 0; threadNo < count; threadNo++)
					{
					theThread = (DThread*) threadList[threadNo];
					if (theThread->iId > currentId)
						break;
					}

				if (theThread->iId > currentId)	// if found,				  
					currentId = theThread->iId;
				else
					break;

				}
			else
				{
				// move onto next thread
				threadNo++;
				if (threadNo >= threadList.Count())
					break;
				else
					{
					theThread = (DThread*) threadList[threadNo];
					currentId = theThread->iId;
					}

				}
			
			NKern::Lock();
			if (theThread->iEmiStartMonitor == (TAny*) StartMonitor)
				{
				theThread->iEmiFlags &= KThdEMIStateMask;	// Clear KThdUseOldExit
				NKern::Unlock();
				}
			else
				{
				if ((theThread->iEmiFlags & KThdEMIStateMask) != KThdRunning)
					{
					if ((theThread->iEmiFlags & KThdEMIStateMask) == KThdStarting)	// Old start running					  
						theThread->iEmiStartMonitor = (TAny*) StartMonitor;
					
					theThread->iEmiFlags &= KThdEMIStateMask;	// Clear KThdUseOldExit
					NKern::Unlock();
					}
				else
					{
					// Use new exit monitor, moniter running.					   
					theThread->iEmiFlags = KThdRunning 	| KThdReStart;	// & !KThdUseOldExit
					NKern::Unlock();
					threadList.Signal();
					StartMonitor(&(theThread->iNThread));
					theThread->iEmiStartMonitor = (TAny*) StartMonitor;
					NKern::Lock();
					if ((theThread->iEmiFlags & KThdEMIStateMask) == KThdExiting)
						{		// It must be waiting for us to finish.
						ExitSemaphore->Signal();
						theThread->iEmiFlags = KThdExiting;	// Clear flags
						}
					else
						theThread->iEmiFlags = KThdRunning;	// Clear flags					  
					NKern::Unlock();
					threadList.Wait();

					//Resync theadNo with theThread, after unlocked period
					if (threadNo >= threadList.Count())	//count could drop														
						threadNo = 0;
					theThread = (DThread*) threadList[threadNo];
					}
				}
			}		

		OldExitMonitor = NULL;	// Transfer compleated.	  
		threadList.Signal();
		}
	}


/**
@internalComponent
*/
void EMI::CallStartHandler(DThread * aDThread)
	{
	TThreadStartMonitor startMonitor;
	TInt result;

	NKern::ThreadEnterCS();
	NKern::Lock();
	aDThread->iEmiStartMonitor = (TAny*) StartMonitor;
	
	if (OldExitMonitor == ExitMonitor)
		aDThread->iEmiFlags |= KThdStarting;
	else
		aDThread->iEmiFlags = KThdStarting;

	do
		{
		startMonitor = (TThreadStartMonitor) aDThread->iEmiStartMonitor;
		NKern::Unlock();

		if (startMonitor != NULL)
			{
			result = startMonitor (&(aDThread->iNThread));
			if (result < 0)
				{
				NKern::ThreadLeaveCS();
				Kern::Exit(result);
				}
			}

		NKern::Lock();
		}
	while ((TAny*) startMonitor != aDThread->iEmiStartMonitor);

	aDThread->iEmiFlags &= KThdUseOldExit;  // Flag must be preserved as could
	aDThread->iEmiFlags |= KThdRunning; // be about to run diffrent start monitor.
										// if loop above executed, flag is unset.
	
	NKern::Unlock();
	NKern::ThreadLeaveCS();
	}


/**
@internalComponent
*/
void EMI::CallExitHandler(DThread* aDThread)
	{
	TTaskEventRecord* rec;
	TInt currentGeneration;
	TAny* nThread = (TAny*) &(aDThread->iNThread);
	TThreadExitMonitor exitMonitor;

	// Set thread non-loggable
	((NThread*) nThread)->ModifyAttributes(KThreadAttLoggable, 0);

	// Clear event log
	NKern::Lock();
	if (TheScheduler.iBufferStart != NULL)
		{
		currentGeneration = Generation;
		rec = (TTaskEventRecord*) TheScheduler.iBufferHead;

		while ((rec != TheScheduler.iBufferTail) && (currentGeneration == Generation))
			{
			rec--;
			if (rec < TheScheduler.iBufferStart)
				rec = (TTaskEventRecord*) TheScheduler.iBufferEnd;

			if (rec->iPrevious == nThread)
				rec->iPrevious = TheScheduler.iSigma;
			if (rec->iNext == nThread)
				rec->iNext = TheScheduler.iSigma;

			NKern::PreemptionPoint();
			}
		}

	if ((aDThread->iEmiFlags & KThdEMIStateMask) != KThdRunning)
		{
		// Thread died before it started running - dont call VEMs function.
		aDThread->iEmiFlags	|= KThdExiting;	
		NKern::Unlock();
		}
	else
		{
		
		aDThread->iEmiFlags	|= KThdExiting;	// No further calls to Start monitor.

		
		 __ASSERT_COMPILE(KThdExiting==KThdEMIStateMask);
		
		if ((aDThread->iEmiFlags & KThdUseOldExit) != 0)
			exitMonitor = OldExitMonitor;
		else
			exitMonitor = ExitMonitor;

		if ((aDThread->iEmiFlags & KThdReStart) != 0)
			{							// monitor already running
			ExitSemaphore->SetOwner(NULL);
			ExitSemaphore->Wait();
			}

		NKern::Unlock();

		// Call VEMs function
		if (exitMonitor != NULL)
			exitMonitor (&(aDThread->iNThread));
		}
	}

/**
Fills the specified task event record from the event log, before the event is
deleted.

Note that if the event log becomes full, the addition of a further event (before any 
subsequent event get) results in the oldest event in the buffer being 
deleted.  In this case, the next oldest event record will have its  
"Events lost before this event" flag set to true.  If bit 1 of the flags 
("Previous thread now waiting") is set, it means that the previous thread 
is blocked and  is no longer ready to run.  It could be waiting on a timer,
mutex, semaphore or similar.  If this flag is false it would mean that either 
the thread's time slice expired, or it was pre-empted by a higher priority
thread. 

@param  aRecord The record to be filled.

@return EFalse, if the event queue is empty; ETrue, otherwise.

@see TTaskEventRecord

@pre Do not call from an ISR
*/
EXPORT_C TBool EMI::GetTaskEvent(TTaskEventRecord& aRecord)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"EMI::GetTaskEvent");
	NKern::Lock();
	if (TheScheduler.iBufferStart)
		{
		if (TheScheduler.iBufferTail != TheScheduler.iBufferHead)
			{ 						// if not empty		   
			aRecord = *((TTaskEventRecord*) TheScheduler.iBufferTail);
			TheScheduler.iBufferTail = ((TTaskEventRecord*) TheScheduler.iBufferTail) + 1;
			if (TheScheduler.iBufferTail > TheScheduler.iBufferEnd)
				TheScheduler.iBufferTail = TheScheduler.iBufferStart;
			NKern::Unlock();
			return ETrue;
			}
		}
	NKern::Unlock();
	return EFalse;
	}


/**
Allows custom events to be added into the event log.

See TTaskEventRecord for the record structure.

-# TTaskEventRecord::iType should be set to a value greater then 127.
-# Bit 0 of TTaskEventRecord::iFlags should be set to zero.
-# TTaskEventRecord::iUserState and TTaskEventRecord::iTime will be overridden
   by the actual UserState and Time when the event is added to the event log.
-# TTaskEventRecord::iPrevious and TTaskEventRecord::iNext are intended to point
   to NThreads, so that the helper tools can provide additional information about
   the threads involved, but the fields can also be used to hold pointers to non-NThread objects.
   If used in this alternative way, care should be used to ensure that there is no address
   collision with the NThread records in the system.

Note: Custom events will follow the same rules as scheduling events in that
the event could be deleted (due to buffer over run), or the previous event
may be deleted setting the flag in this one.  As a result such records should
never be relied upon, and no memory leakage or similar side effect should 
occur if the events are lost.  

Also for events within the buffer, as with scheduling events, if the thread 
indicated by iPrevious or iNext terminates, then this thread reference will be
removed, (to prevent dereferencing of an invalid pointer) and replaced with a
pointer to the Sigma thread.

@param aRecord The record to be copied into the event log.

@return EFalse on failure, indicating no event buffer is allocated; ETrue, otherwise.

@see TTaskEventRecord

@pre Do not call from an ISR
*/
EXPORT_C TBool EMI::AddTaskEvent(TTaskEventRecord& aRecord)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR,"EMI::AddTaskEvent");
	NKern::Lock();
	if (TheScheduler.iBufferStart)
		{
		*((TTaskEventRecord*) TheScheduler.iBufferHead) = aRecord;
		((TTaskEventRecord*) TheScheduler.iBufferHead)->iUserState = TheScheduler.iEmiState;
		((TTaskEventRecord*) TheScheduler.iBufferHead)->iTime = NKern::FastCounter();

		TheScheduler.iBufferHead = ((TTaskEventRecord*) TheScheduler.iBufferHead) + 1;
		if (TheScheduler.iBufferHead > TheScheduler.iBufferEnd)
			TheScheduler.iBufferHead = TheScheduler.iBufferStart;

		if (TheScheduler.iBufferTail == TheScheduler.iBufferHead)
			{ 						// overflow, move on read pointer - event lost!
			TheScheduler.iBufferTail = ((TTaskEventRecord*) TheScheduler.iBufferTail) + 1;
			if (TheScheduler.iBufferTail > TheScheduler.iBufferEnd)
				TheScheduler.iBufferTail = TheScheduler.iBufferStart;

			((TTaskEventRecord*) TheScheduler.iBufferTail)->iFlags |= KTskEvtFlag_EventLost;
			}
		NKern::Unlock();
		return ETrue;
		}
	NKern::Unlock();
	return EFalse;
	}

#ifdef __WINS__

/**
@internalComponent
*/
void EMI_AddTaskSwitchEvent(TAny* aPrevious, TAny* aNext)
	{
	if (TheScheduler.iLogging)
		{
		TUint8 state = ((NThread*) aPrevious)->iSpare1;	//iNState

		if ((((NThread*) aPrevious)->Attributes() & KThreadAttLoggable) == 0)
			aPrevious = TheScheduler.iSigma;
		if ((((NThread*) aNext)->Attributes() & KThreadAttLoggable) == 0)
			aNext = TheScheduler.iSigma;

		if (aPrevious != aNext)
			{
			TTaskEventRecord& rec = *((TTaskEventRecord*) TheScheduler.iBufferHead);

			rec.iType = 0;
			rec.iFlags = (state == NThread::EReady) ? (TUint8) 0 :KTskEvtFlag_PrevWaiting;
			rec.iUserState = TheScheduler.iEmiState;
			rec.iTime = NKern::FastCounter();
			rec.iPrevious = aPrevious;
			rec.iNext = aNext;

			TheScheduler.iBufferHead = ((TTaskEventRecord *) TheScheduler.iBufferHead) + 1;
			if (TheScheduler.iBufferHead > TheScheduler.iBufferEnd)
				TheScheduler.iBufferHead = TheScheduler.iBufferStart;

			if (TheScheduler.iBufferTail == TheScheduler.iBufferHead)
				{						// overflow, move on read pointer - event lost!
				TheScheduler.iBufferTail = ((TTaskEventRecord*) TheScheduler.iBufferTail) + 1;
				if (TheScheduler.iBufferTail > TheScheduler.iBufferEnd)
					TheScheduler.iBufferTail = TheScheduler.iBufferStart;

				((TTaskEventRecord*) TheScheduler.iBufferTail)->iFlags |= KTskEvtFlag_EventLost;
				}
			}
		}
	}

/**
@internalComponent
*/
void EMI_CheckDfcTag(TAny* aNext)
	{
	TUint result = ((NThread*) aNext)->iTag & TheScheduler.iEmiMask;
	
	if (result!=0)
		{
		TheScheduler.iEmiDfcTrigger |= result;
		TheScheduler.iEmiDfc->Add();
		}
	}



#endif




/**
Gets a pointer to the idle thread.

This can be compared with an NThread pointer returned from
EMI::GetTaskEvent() to determine whether the system was idling.

@return A pointer to the idle thread.
*/
EXPORT_C NThread* EMI::GetIdleThread()
	{
	return &(K::TheNullThread->iNThread);
	}




/**
Gets a pointer to the Sigma thread.

This can be compared with an NThread pointer returned from
EMI::GetTaskEvent() to determine whether the system was busy
with activity associated with the Sigma thread.

@return A pointer to the Sigma thread.
*/
EXPORT_C NThread* EMI::GetSigmaThread()
	{
	return TheScheduler.iSigma;
	}




/**
Sets energy management scheme (VEMS) data into the specified thread.

This function allows implementors of energy management schemes to store
any data associated with the thread. The field is used solely by scheme
implementors, and has no restriction in use.

Note that this data must be cleaned up before the 'exit monitor' completes.

The function sets the NThread::iVmsdata field to the value of the aVemsData pointer.

@param aThread The thread to be altered.
@param aVemsData The pointer to be set into the NThread::iVemsData field.

@see EMI::GetThreadVemsData()
*/
EXPORT_C void EMI::SetThreadVemsData(NThread* aThread, TAny* aVemsData)
	{
	aThread->iVemsData = aVemsData;
	}



/**
Gets energy management scheme (VEMS) data from the specified thread.

The function returns the NThread::iVmsdata field.

@param aThread The thread whose data is to be queried.

@return The pointer returned from the NThread::iVemsData field.

@see EMI::SetThreadVemsData()
*/
EXPORT_C TAny* EMI::GetThreadVemsData(NThread* aThread)
	{
	return aThread->iVemsData;
	}




/**
Sets the loggable attribute in the specified NThread.

If this field is true then 
scheduling events involving this thread will be logged, and may be discovered 
with GetTaskEvent().  This is used to protect the system from trying to 
interact with a partially defined DThread.  This will be initialised to 
false, and set back to false during deletion of the thread.  The energy management scheme (VEMS)
can set this to true from the 'start monitor', after any associated data has
been initialised.  During any phase in which the loggable attribute is false, 
the thread's activity will be considered to be Sigma thread activity.  This 
behaviour prevents partially defined DThreads' activity appearing in the 
event log before the thread apparently started or after it terminated.  The 
Sigma thread's Loggable attribute is ignored, and is treated as if always
being true.

@param aThread   The thread to be altered.
@param aLoggable Value for the loggable flag.

@see EMI::GetThreadLoggable()
*/
EXPORT_C void EMI::SetThreadLoggable(NThread* aThread, TBool aLoggable)
	{
	if (aLoggable)
		aThread->ModifyAttributes(0, KThreadAttLoggable);
	else
		aThread->ModifyAttributes(KThreadAttLoggable, 0);
	}




/**
Gets the state of the Loggable attribute in the NThread.

@param aThread The thread to be queried.

@return The state of the Loggable attribute in the NThread.

@see EMI::SetThreadLoggable()
*/
EXPORT_C TBool EMI::GetThreadLoggable(NThread* aThread)
	{
	return (aThread->Attributes() & KThreadAttLoggable) != 0;
	}




/**
Sets the NThread::iTag field in the specified NThread.

This field is ANDed with the scheduler's EMI mask when the thread is scheduled.
A nonzero result will trigger a DFC to be scheduled.

@param aThread	The thread to be altered.
@param aTag		The tag to be set.

@see EMI::SetDfc()
@see EMI::SetMask()
@see EMI::GetMask()
@see EMI::GetThreadTag()
*/
EXPORT_C void EMI::SetThreadTag(NThread* aThread, TUint32 aTag)
	{
	aThread->iTag = aTag;
	}




/**
Gets the NThread::iTag field from the specified NThread. 

This tag is ANDed with the scheduler's EMI mask when the thread is scheduled.
A nonzero result will trigger a DFC to be scheduled.

@param aThread The thread to be queried.

@return The value of the NThread::iTag field.

@see EMI::SetDfc()
@see EMI::SetMask()
@see EMI::GetMask()
@see EMI::SetThreadTag()
*/
EXPORT_C TUint32 EMI::GetThreadTag(NThread* aThread)
	{
	return aThread->iTag;
	}



/**
Sets the scheduler's EMI mask.

During scheduling, this mask is ANDed with the NThread::iTag field in the NThread 
about to be scheduled.  If the result is nonzero, a DFC is scheduled. This 
function can be called anywhere and does not need to be called from a 
critical section. 

@param aMask The mask to be used.

@see EMI::SetDfc()
@see EMI::SetThreadTag()
@see EMI::GetThreadTag()
@see EMI::GetMask()
*/
EXPORT_C void EMI::SetMask(TUint32 aMask)
	{
	TheScheduler.iEmiMask = aMask;

	}




/**
Gets the scheduler's EMI mask.

During scheduling, this mask is ANDed with the NThread::iTag field in the NThread 
about to be scheduled.  If the result is nonzero, a DFC is scheduled. This 
function can be called anywhere and does not need to be called from a 
critical section. 

@return The mask value.

@see EMI::SetDfc()
@see EMI::SetThreadTag()
@see EMI::GetThreadTag()
@see EMI::SetMask()
*/
EXPORT_C TUint32 EMI::GetMask()
	{
	return TheScheduler.iEmiMask;
	}




/**
Sets the the scheduler's EMI mask, and the DFC to be scheduled.

During scheduling, this mask is ANDed with the NThread::iTag field in the NThread 
about to be scheduled.  If the result is nonzero, then the specified DFC 
is scheduled. This method may be called anywhere and does not need to be 
called from a critical section.

@param aDfc The DFC to be used.
@param aMask The mask to be used.

@see EMI::SetThreadTag()
@see EMI::SetMask()
*/
EXPORT_C void EMI::SetDfc(TDfc* aDfc, TUint32 aMask)
	{
	TheScheduler.iEmiDfc = aDfc;
	TheScheduler.iEmiMask = aMask;
	}




/**
Returns the logical AND of the NThread::iTag field of the thread which triggered the 
DFC with the mask prevailing at the time.

As is normal with DFCs, if more than one DFC triggering event occurs before the DFC gets
to run, then only one DFC will actually run.  In this case, this function returns the logical OR of 
the corresponding values for all the events. The accumulated trigger tag is 
cleared by this function, so calling it twice in succession will return zero 
the second time.

@return The tag bits that caused the last DFC.
*/
EXPORT_C TUint32 EMI::GetDfcTriggerTag()
	{
	return __e32_atomic_swp_ord32(&TheScheduler.iEmiDfcTrigger, 0);
	}




/**
Sets the user state variable, as reported in the event log. 

It is expected that this variable will be used to remember the clock 
frequency at the time when the event was logged.  This method may be called 
anywhere and does not need to be called from a critical section. 

@param aState The new user state value.

@see EMI::GetState()
*/
EXPORT_C void EMI::SetState(TUint32 aState)
	{
	TheScheduler.iEmiState = aState;
	}




/**
Gets the user state variable, as reported in the event log.

This method may be called anywhere and does not need to be called from a critical section.

@return The user state variable.

@see EMI::SetState()
*/
EXPORT_C TUint32 EMI::GetState()
	{
	return TheScheduler.iEmiState;
	}




/**
@internalComponent
*/
void AfterIdleCallback(TAny* aSem)
	{
	NKern::Lock();
	EMI::AfterIdleState = EMI::ENone;
	((NFastSemaphore*) aSem)->Signal();
						//release semaphore
	NKern::Unlock();
	}




/**
This call waits for the time interval aDelay (specified in milliseconds)
to pass, unless the CPU goes idle following the call with only one event in
the queue, representing the context switch from the current thread to the idle
thread. In this case the timer is paused before the CPU goes idle and resumed
when it wakes up.

For this to function correctly, EMI::EnterIdle() and EMI::LeaveIdle() must be called by
the base port from the idle function.

@param aDelay The time interval to be delayed.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void EMI::AfterIdle(TInt aDelay)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"EMI::AfterIdle");
	NFastSemaphore mySem;
	// Set new timer, callback to release semephore
	NTimer myTimer(&AfterIdleCallback, &mySem);
	NKern::Lock();
	myTimer.OneShot(aDelay, ETrue);
	// Set AfterIdle Timer pointer to point at new timer. Mark as runnung.  
	// (If this is running, any existing AfterIdele call should act as normall timer)
	AfterIdleState = EWaiting;
	AfterIdleTimer = &myTimer;
	AfterIdleDelay = aDelay;

	// Wait for Timer to release semaphore.
	mySem.SetOwner(NULL);
	mySem.Wait();
	NKern::Unlock();
	}


/**
This function must be called by the base port at the start of the idle 
function, just after interrupts have been disabled.
*/
EXPORT_C void EMI::EnterIdle()
	{
	if (AfterIdleState == EWaiting)
		{
		if (AfterIdleTimer->iState > NTimer::EIdle)
			{
			TInt sizeDelay = (TInt) TheScheduler.iBufferHead - (TInt) TheScheduler.iBufferTail;

			if (sizeDelay && TheScheduler.iLogging)
				sizeDelay = ((sizeDelay != sizeof(TTaskEventRecord)) &&
						 ((TheScheduler.iBufferHead != TheScheduler.iBufferStart)
						  || (TheScheduler.iBufferTail != TheScheduler.iBufferEnd)));

			if (!sizeDelay)
				{
				AfterIdleTimer->Cancel();
				AfterIdleState = EHeld;
				}
			else
				AfterIdleState = ENone;
			}
		else
			AfterIdleState = ENone;
		}
	}

/**
This function must be called by the base port at the end of the idle function,
just before interrupts are re-enabled.
*/
EXPORT_C void EMI::LeaveIdle()
	{
	if (AfterIdleState == EHeld)
		{
		AfterIdleTimer->OneShot(AfterIdleDelay, ETrue);
		AfterIdleState = ENone;
		}
	}

#else	// EMI not supported
		// EMI Stubs - for compatability

EXPORT_C TInt	 EMI::TaskEventLogging(TBool, TInt, TThreadStartMonitor, TThreadExitMonitor)
															{ return KErrNotSupported; }
EXPORT_C TInt	 EMI::TaskEventLogging (TBool, TInt)		{ return KErrNotSupported; }
EXPORT_C void	 EMI::ThreadMonitors (TThreadStartMonitor, TThreadExitMonitor)	{}
EXPORT_C TBool	EMI::GetTaskEvent (TTaskEventRecord&)		{ return EFalse; }
EXPORT_C TBool	EMI::AddTaskEvent (TTaskEventRecord&)		{ return EFalse; }
EXPORT_C NThread* EMI::GetIdleThread ()						{ return NULL; }
EXPORT_C NThread* EMI::GetSigmaThread ()					{ return NULL; }
EXPORT_C void	 EMI::SetThreadVemsData (NThread*, TAny*)	{}
EXPORT_C TAny*	EMI::GetThreadVemsData (NThread*)			{ return NULL; }
EXPORT_C void	 EMI::SetThreadLoggable (NThread*, TBool)	{}
EXPORT_C TBool	EMI::GetThreadLoggable (NThread*)			{ return EFalse; }
EXPORT_C void	 EMI::SetThreadTag (NThread*, TUint32)		{}
EXPORT_C TUint32  EMI::GetThreadTag (NThread*)				{ return 0; }
EXPORT_C void	 EMI::SetMask (TUint32)						{}
EXPORT_C TUint32  EMI::GetMask ()							{ return 0; }
EXPORT_C void	 EMI::SetDfc (TDfc *, TUint32)				{}
EXPORT_C TUint32  EMI::GetDfcTriggerTag ()					{ return 0; }
EXPORT_C void	 EMI::SetState (TUint32)					{}
EXPORT_C TUint32  EMI::GetState ()							{ return 0; }
EXPORT_C void	 EMI::AfterIdle (TInt)						{}
EXPORT_C void	 EMI::EnterIdle ()							{}
EXPORT_C void	 EMI::LeaveIdle ()							{}

#endif
