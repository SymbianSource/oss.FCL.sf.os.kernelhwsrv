// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// testcaseb2broot.cpp
// @internalComponent
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest header
#include <e32ver.h>
#include <e32def.h>
#include <e32def_private.h>
#include <d32otgdi.h>		// OTGDI header
#include <d32usbc.h>		// USBCC header
#include "testcaseroot.h"
#include "b2bwatchers.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "b2bwatchersTraces.h"
#endif



void CNotifyWatcherBase::RunL()
	{
	DisplayEvent(); 
	iHandler.HandleEvent(iWatchType, GetEventValue()); // report the event upwards

	IssueAgain(); 
	SetActive();
	}


CNotifyCollector::CNotifyCollector(TRequestStatus &aStatus)  : iStatusStep(aStatus) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CNOTIFYCOLLECTOR_CNOTIFYCOLLECTOR);
	    }
	TTimeIntervalDays oneday(1);
	iTimeStarted.HomeTime();
	iTimeStarted += (oneday); // force all durations to produce a negative (invalid) value
	}


/***************************************************************
 *  ~CNotifyCollector
 */ 
CNotifyCollector::~CNotifyCollector()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CNOTIFYCOLLECTOR_DCNOTIFYCOLLECTOR);
	    }
	
	ClearAllEvents(); // free event arrays

	iNotifyObjects.Close();
	iFailureEvents.Close();
	iRequiredEvents.Close();
	iReceivedEvents.Close();
	}


/* The test-case calls this to clear the events stored.
 * Both the expected and already recieved events get cleared, this method 
 * is typically called at the start of each test-step
 */
void CNotifyCollector::ClearAllEvents(TBool aClearRecieved/*=ETrue*/, TBool aClearRequired /*=ETrue*/)
	{
	if (aClearRequired)
		{
		iRequiredEvents.Reset();
		iFailureEvents.Reset();
		}
	if (aClearRecieved)
		{
		iReceivedEvents.Reset();
		}
	}


/* Creates and starts all 4 observers
 * Note: The Watchdog does not get started, because it needs an interval
 */
void CNotifyCollector::CreateObserversL(COtgRoot &aOtgDriver)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CNOTIFYCOLLECTOR_CREATEOBSERVERSL);
	    }
	TInt watchType;
	ASSERT(aOtgDriver.LddLoaded());
	
	for (watchType=EWatcherTimeouts; watchType < EWatcherInvalid; watchType++)
		{
		CNotifyWatcherBase *pWatcher=0;
		switch ((TWatcherNotifyType )watchType)
			{
			case EWatcherTimeouts:
				pWatcher = COtgWatchdogWatcher::NewL(*this,  EWatcherTimeouts, aOtgDriver);
				break;
			case EWatcherState:
				pWatcher = COtgStateWatcher::NewL(*this , EWatcherState, aOtgDriver);
				break;
			case EWatcherEvent:
				pWatcher = COtgEventWatcher::NewL(*this, EWatcherEvent, aOtgDriver);
				break;
			case EWatcherMessage:
				pWatcher = COtgMessageWatcher::NewL(*this, EWatcherMessage, aOtgDriver);
				break;
			case EWatcherPeripheralState:
				pWatcher = CPeripheralStateWatcher::NewL(*this, EWatcherPeripheralState, aOtgDriver);
				break;
			case EWatcherAConnectionIdle:
				pWatcher = CAConnectionIdleWatcher::NewL(*this, EWatcherAConnectionIdle, aOtgDriver);
				break;

			}
		// the TRequest object is added to scheduler in it's own constructor
		
		// add it to our list so we can kill them after the test.
		iNotifyObjects.Append(pWatcher);

		
		// start all watchers, except for the watchdog
		if (watchType != EWatcherTimeouts)
			{
			pWatcher->StartWatching(-1);
			}
		}
	test.Printf(_L("\n"));
	OstTrace0(TRACE_NORMAL, CNOTIFYCOLLECTOR_CREATEOBSERVERSL_DUP02, "\n");
	}


/* NOTE: OTG must still be loaded or else we cannot cancel the outstanding event watches here!
 */
void CNotifyCollector::DestroyObservers()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CNOTIFYCOLLECTOR_DESTROYOBSERVERS);
	    }
	
	// Free the Watchers
	for (TInt idx=0; idx < iNotifyObjects.Count(); idx++)
		{
		LOG_VERBOSE2(_L(".. %d .."), idx);		
		if(gVerboseOutput)
		    {
		    OstTrace1(TRACE_VERBOSE, CNOTIFYCOLLECTOR_DESTROYOBSERVERS_DUP01, ".. %d ..", idx);		
		    }
		delete iNotifyObjects[idx];	// they will call their own Cancel() methods
		}
	iNotifyObjects.Close();	
	}


void CNotifyCollector::AddRequiredNotification(TWatcherNotifyType aType, TInt aValue)
	{
	AddRequiredOrFailureNotification(aType, aValue, EFalse);
	}

void CNotifyCollector::	AddFailureNotification(const TWatcherNotifyType aType, TInt aValue)
	{
	AddRequiredOrFailureNotification(aType, aValue, ETrue);
	}

/* Checks that a watcher for the event exists, and then adds it to a list of events required for a PASS condition
 * The timout event does not get added to this list.
 * If the parameter is a time, the timer gets started
 * If aEventMeansFailure is set True, then the reception of the event will cause the current test step to fail
 */	
void CNotifyCollector::AddRequiredOrFailureNotification(TWatcherNotifyType aType, TInt aValue, TBool aEventMeansFailure)
	{
	CNotifyWatcherBase *pWatcher=0;
	TInt index=0;
	TBuf<MAX_DSTRLEN> aDescription;	

	// print a usefull debug message
	switch (aType)
		{
		case EWatcherTimeouts:
			break;
		case EWatcherState:
			COtgRoot::OtgStateString(static_cast<RUsbOtgDriver::TOtgState>(aValue), aDescription);
			LOG_VERBOSE3(_L("AddRequiredNotification() State %d '%S' wanted\n"), aValue, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CNOTIFYCOLLECTOR_ADDREQUIREDORFAILURENOTIFICATION, "AddRequiredNotification() State %d '%S' wanted\n", aValue, aDescription);
			    }
			break;
		case EWatcherEvent:
			COtgRoot::OtgEventString(static_cast<RUsbOtgDriver::TOtgEvent>(aValue), aDescription);
			LOG_VERBOSE3(_L("AddRequiredNotification() Event %d '%S' wanted\n"), aValue, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CNOTIFYCOLLECTOR_ADDREQUIREDORFAILURENOTIFICATION_DUP01, "AddRequiredNotification() Event %d '%S' wanted\n", aValue, aDescription);
			    }
			break;
		case EWatcherMessage:
			COtgRoot::OtgMessageString(static_cast<RUsbOtgDriver::TOtgMessage>(aValue), aDescription);
			LOG_VERBOSE3(_L("AddRequiredNotification() Message %d '%S' wanted\n"), aValue, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CNOTIFYCOLLECTOR_ADDREQUIREDORFAILURENOTIFICATION_DUP02, "AddRequiredNotification() Message %d '%S' wanted\n", aValue, aDescription);
			    }
			break;
		case EWatcherPeripheralState:
			COtgRoot::PeripheralStateString(static_cast<TUint>(aValue), aDescription);
			LOG_VERBOSE3(_L("AddRequiredNotification() Peripheral State %d '%S' wanted\n"), aValue, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CNOTIFYCOLLECTOR_ADDREQUIREDORFAILURENOTIFICATION_DUP03, "AddRequiredNotification() Peripheral State %d '%S' wanted\n", aValue, aDescription);
			    }
			break;
		case EWatcherAConnectionIdle:
			COtgRoot::AConnectionIdleString(static_cast<RUsbOtgDriver::TOtgConnection>(aValue), aDescription);
			LOG_VERBOSE3(_L("AddRequiredNotification() AConnectionIdle %d '%S' wanted\n"), aValue, &aDescription);
			if(gVerboseOutput)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CNOTIFYCOLLECTOR_ADDREQUIREDORFAILURENOTIFICATION_DUP04, "AddRequiredNotification() AConnectionIdle %d '%S' wanted\n", aValue, aDescription);
			    }
			break;

		}
	
	// Find the watcher if possible
	while (index < iNotifyObjects.Count())
		{

		TEST_ASSERTION(iNotifyObjects[index]!=NULL, _L("iNotifyObjects element gone!"));

		if (iNotifyObjects[index]->GetType() == aType)
			{
			pWatcher = iNotifyObjects[index];
			break;
			}
		index++;
		}
	
	TEST_ASSERTION(pWatcher!=NULL, _L("pWatcher=0!"));
	if (aType == EWatcherTimeouts)	
		{ // other watchers are already running, but we start the timer now
		pWatcher->StartWatching(aValue);
		}
	else
		{ // timeouts are not added to the Q
		TOtgObservedEvent evt(aType, aValue);	
		if(aEventMeansFailure)
			{
			iFailureEvents.Append(evt);
			}
		else
			{
			iRequiredEvents.Append(evt);
			}
		}
	// flag as pending
	iStatusStep = KRequestPending;
	iTimeStarted.HomeTime();
	}


/* Return the number of milliseconds since the last call to AddRequiredNotification()
 */
TInt CNotifyCollector::DurationElapsed()
	{
	TTime TimeEnd;
	TInt Millisec;

	TimeEnd.HomeTime();
	TTimeIntervalMicroSeconds ivlMicro(TimeEnd.MicroSecondsFrom(iTimeStarted));
	Millisec = (TInt)(ivlMicro.Int64())/1000;	// USB times are in uSec, but in ms for the user layer

	if (Millisec < 0)
		Millisec = -1; // TRUE for when the Notifiers are not yet being used.
	return(Millisec);				
	}


/* Search for an event in the received Q
 * @return :TRUE if the specified event has been received
 */
TBool CNotifyCollector::EventReceivedAlready(const TOtgObservedEvent& aEvent)
	{
	for (TInt idx=0; idx < iReceivedEvents.Count(); idx++)
		if (iReceivedEvents[idx] == aEvent)
			return(ETrue);
	return(EFalse);
	}

/* Search for an event in the failure event queue
 * @return :TRUE if the specified event does denote a failure
 */
TBool CNotifyCollector::IsFailureEvent(TOtgObservedEvent &aEvent)
	{
	for (TInt idx=0; idx < iFailureEvents.Count(); idx++)
		if (iFailureEvents[idx] == aEvent)
			return(ETrue);
	return(EFalse);
	}

/* @return 0 if the watcher has not yet been created. (for instance early in the test)
 */
CNotifyWatcherBase* CNotifyCollector::GetWatcher(TWatcherNotifyType aType)
	{
	CNotifyWatcherBase *pWatcher=0;
	// Find the watcher 
	TInt index=0;
	
	while (index < iNotifyObjects.Count())
		{

		TEST_ASSERTION(iNotifyObjects[index]!=NULL, _L("iNotifyObjects element gone!"));

		if (iNotifyObjects[index]->GetType() == aType)
			{
			pWatcher = iNotifyObjects[index];
			break;
			}
		index++;
		}

	return(pWatcher);
	}


/* Process the event. The OTG watchers are responsible for renewing themselves
 * but the Timer event does not renew
 */
void CNotifyCollector::HandleEvent(TWatcherNotifyType aType, TInt aValue)
	{
	if (aType == EWatcherTimeouts)
		{
		test.Printf(_L("Step timed out..(%dms).\n\n"), GetWatcher(aType)->GetEventValue());
		OstTrace1(TRACE_NORMAL, CNOTIFYCOLLECTOR_HANDLEEVENT, "Step timed out..(%dms).\n\n", GetWatcher(aType)->GetEventValue());
		CompleteStep(KTestCaseWatchdogTO);
		return;
		}
	
	TOtgObservedEvent evt(aType, aValue);
	TInt start=0;
	iReceivedEvents.Append(evt);  // store incomming evt

	//	Check to see whether the event denotes a failure for this event
	if (IsFailureEvent(evt))
		{
		test.Printf(_L("This event denotes failure for this test\n"));
		OstTrace0(TRACE_NORMAL, CNOTIFYCOLLECTOR_HANDLEEVENT_DUP01, "This event denotes failure for this test\n");
		CompleteStep(KTestCaseFailureEventReceived);
		return;
		}
	
	if (iRequiredEvents.Count())
		{
		// itterate all required events, search for each one in the incomming events list
		while (start< iRequiredEvents.Count())
			{
				
				if (!EventReceivedAlready(iRequiredEvents[start]))
					return;	// missing still, continue
			start++; 
			}
		// found all the required events
		LOG_VERBOSE1(_L("Found all.\n"));
		if(gVerboseOutput)
		    {
		    OstTrace0(TRACE_VERBOSE, CNOTIFYCOLLECTOR_HANDLEEVENT_DUP03, "Found all.\n");
		    }
		CompleteStep(KErrNone);
		}
	else
		{
		test.Printf(_L("Warning : No required events!\n"));
		OstTrace0(TRACE_NORMAL, CNOTIFYCOLLECTOR_HANDLEEVENT_DUP04, "Warning : No required events!\n");
		}	
	}

//	Complete the test step's TRequestStatus (checking it is currently KRequestPending
//	to try and avoid multiple completions).
//
void CNotifyCollector::CompleteStep(TInt aCompletionCode)
	{
	if(iStatusStep.Int() != KRequestPending)
		{
		test.Printf(_L("Can't complete step - not KRequestPending!\n"));
		OstTrace0(TRACE_NORMAL, CNOTIFYCOLLECTOR_COMPLETESTEP, "Can't complete step - not KRequestPending!\n");
		}
	else
		{
		TRequestStatus *StatusStepPtr = &iStatusStep;
		User::RequestComplete(StatusStepPtr, aCompletionCode);
		}
	}

/****************************************************************************
 * COtg Watchdog Watcher
 */
COtgWatchdogWatcher *COtgWatchdogWatcher::NewL(MOtgNotificationHandler &wdHandler, 
												const TWatcherNotifyType aWatchType, 
												COtgRoot &aOtgRoot)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CNOTIFYCOLLECTOR_COMPLETESTEP_DUP01);
	    }
	COtgWatchdogWatcher* self = new (ELeave) COtgWatchdogWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


void COtgWatchdogWatcher::ConstructL()
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGWATCHDOGWATCHER_CONSTRUCTL);
	    }
	
	iTimer.CreateLocal();
	iIntervalMs = -1;
	}


void COtgWatchdogWatcher::StepExpired(TInt aInterval) 
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGWATCHDOGWATCHER_STEPEXPIRED);
	    }
	iHandler.HandleEvent(EWatcherTimeouts, aInterval) ; 
	}


void COtgWatchdogWatcher::RunL()
	{ 
	StepExpired(iIntervalMs); 
	}


void COtgWatchdogWatcher::StartTimer(TInt aIntervalMs)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGWATCHDOGWATCHER_STARTTIMER);
	    }

	iIntervalMs = aIntervalMs;	// save value for printing latter 
	if (IsActive()) //cancel the last timer we set, this is easier than cancelling it in each test-step
		{
		iTimer.Cancel();
		User::WaitForRequest(iStatus); // swallow it
		iTimer.After(iStatus, aIntervalMs*1000);
		}
	else
		{
		iTimer.After(iStatus, aIntervalMs*1000);
		SetActive();
		}
	LOG_VERBOSE2(_L("wd Timer %dms\n"), aIntervalMs)
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, COTGWATCHDOGWATCHER_STARTTIMER_DUP01, "wd Timer %dms\n", aIntervalMs);
	    }
	}	


/****************************************************************************
 * OTG Event/State/Message Watchers
 */
COtgMessageWatcher* COtgMessageWatcher::NewL(MOtgNotificationHandler &wdHandler, 
										const TWatcherNotifyType aWatchType, 
										COtgRoot &aOtgRoot)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGMESSAGEWATCHER_NEWL);
	    }
	COtgMessageWatcher* self = new (ELeave) COtgMessageWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


void COtgMessageWatcher::DisplayEvent()
	{
	TBuf<MAX_DSTRLEN> aDescription;	
	iOtgRoot.OtgMessageString(iMessage, aDescription);
	test.Printf(_L("Received Message %d '%S'\n"), iMessage, &aDescription);
	OstTraceExt2(TRACE_NORMAL, COTGMESSAGEWATCHER_DISPLAYEVENT, "Received Message %d '%S'\n", iMessage, aDescription);
	}


COtgStateWatcher* COtgStateWatcher::NewL(MOtgNotificationHandler &wdHandler, 
										const TWatcherNotifyType aWatchType, 
										COtgRoot &aOtgRoot)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(COTGSTATEWATCHER_NEWL);
	    }
	COtgStateWatcher* self = new (ELeave) COtgStateWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


void COtgStateWatcher::DisplayEvent() 
	{
	TBuf<MAX_DSTRLEN> aDescription;	
	iOtgRoot.OtgStateString(iState, aDescription);
	test.Printf(_L("Received State %d '%S'\n"), iState, &aDescription);
	OstTraceExt2(TRACE_NORMAL, COTGSTATEWATCHER_DISPLAYEVENT_DUP01, "Received State %d '%S'\n", iState, aDescription);
	}


COtgEventWatcher* COtgEventWatcher::NewL(MOtgNotificationHandler &wdHandler, 
										const TWatcherNotifyType aWatchType, 
										COtgRoot &aOtgRoot)
	{
	COtgEventWatcher* self = new (ELeave) COtgEventWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void COtgEventWatcher::DisplayEvent()
	{
	TBuf<MAX_DSTRLEN> aDescription;	
	iOtgRoot.OtgEventString(iEvent, aDescription);
	test.Printf(_L("Received Event %d '%S'\n"), iEvent, &aDescription);
	OstTraceExt2(TRACE_NORMAL, COTGEVENTWATCHER_DISPLAYEVENT, "Received Event %d '%S'\n", iEvent, aDescription);
	}

CPeripheralStateWatcher* CPeripheralStateWatcher::NewL(MOtgNotificationHandler &wdHandler, 
														const TWatcherNotifyType aWatchType, 
														COtgRoot &aOtgRoot)
	{
	CPeripheralStateWatcher* self = new (ELeave) CPeripheralStateWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CPeripheralStateWatcher::DisplayEvent()
	{
	TBuf<MAX_DSTRLEN> aDescription;	
	iOtgRoot.PeripheralStateString(iPeripheralState, aDescription);
	test.Printf(_L("Peripheral State %d '%S'\n"), iPeripheralState, &aDescription);
	OstTraceExt2(TRACE_NORMAL, CPERIPHERALSTATEWATCHER_DISPLAYEVENT, "Peripheral State %u '%S'\n", iPeripheralState, aDescription);
	}

CAConnectionIdleWatcher* CAConnectionIdleWatcher::NewL(MOtgNotificationHandler &wdHandler, 
														const TWatcherNotifyType aWatchType, 
														COtgRoot &aOtgRoot)
	{
	CAConnectionIdleWatcher* self = new (ELeave) CAConnectionIdleWatcher(wdHandler, aWatchType, aOtgRoot);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CAConnectionIdleWatcher::RunL()
	{
	//	We need to override the RunL for this event type, as
	//	the semantics of the asynchronous function are somewhat
	//	different to the rest of the ones being serviced by 
	//	CNotifyWatcherBase.
	//
	//	In the case of QueueOtgConnectionNotification, the value
	//	passed in is updated *immediately* to reflect the current
	//	activity or otherwise of the connection (unlike the other
	//	async functions which update the value on completion).
	//	The completion in the case of QueueOtgConnectionNotification
	//	is used to indicate that the value has changed, and
	//	another request should be queued to pick up this new value.
	//
	//	The practical upshot of this is that the IssueAgain needs
	//	to happen before the event is displayed and handled...
	
	IssueAgain(); 
	DisplayEvent(); 
	iHandler.HandleEvent(iWatchType, GetEventValue()); // report the event upwards
	SetActive();
	}


void CAConnectionIdleWatcher::DisplayEvent()
	{
	TBuf<MAX_DSTRLEN> aDescription;	
	iOtgRoot.AConnectionIdleString(iAConnectionIdle, aDescription);
	test.Printf(_L("AConnectionIdle %d '%S'\n"), iAConnectionIdle, &aDescription);
	OstTraceExt2(TRACE_NORMAL, CACONNECTIONIDLEWATCHER_DISPLAYEVENT, "AConnectionIdle %d '%S'\n", iAConnectionIdle, aDescription);
	}
