#ifndef B2BWATCHERS_H
#define B2BWATCHERS_H
// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// 
//


#include <d32otgdi.h>	// OTGDI thunk headder (under test)
#include <d32usbc.h>
#include "debugmacros.h"
#include "testcasefactory.h"
#include "testpolicy.h"
#include "testcaseroot.h"


// forward:
class CNotifyWatcherBase;	

// types:
enum TWatcherNotifyType
	{
	EWatcherTimeouts	=1,	// step watchdog event
	EWatcherState,			// OTG states
	EWatcherEvent,			// ... events
	EWatcherMessage,		// ... messages
	EWatcherPeripheralState,// Peripheral device state (undefined, attached, powered etc.)
	EWatcherAConnectionIdle,// (A-Device only) whether the connection is idle or busy
	EWatcherInvalid
	};

/* Applicable values in the value member - these are identical to the RUsbOtgDriver enum values, 
* but are passed as TInt to be generic
*	enum TOtgEvent
*		{
*		EEventAPlugInserted		,
*		EEventAPlugRemoved		,
*		EEventVbusRaised		,
*		EEventVbusDropped		,
*		EEventSrpInitiated		,
*		EEventSrpReceived		,
*		EEventHnpEnabled		,
*		EEventHnpDisabled		,
*		EEventHnpSupported		,
*		EEventHnpAltSupported	,
*		EEventRoleChangedToHost	,
*		EEventRoleChangedToDevice,
*		EEventRoleChangedToIdle								
*		};
*
*	enum TOtgState
*		{
*		EStateReset					,
*		EStateAIdle					,
*		EStateAHost					,
*		EStateAPeripheral			,
*		EStateAVbusError			,						
*		EStateBIdle					,
*		EStateBPeripheral			,
*		EStateBHost					
*		};
*
*	enum TOtgMessage
*		{
*		EEventQueueOverflow					,
*		EStateQueueOverflow					,
*		EMessageQueueOverflow				,
*		EMessageBadState					,
*		EMessageStackNotStarted				,
*		EMessageVbusAlreadyRaised			,
*		EMessageSrpForbidden				,
*		EMessageBusControlProblem			,
*		EMessageVbusError					,
*		EMessageSrpTimeout					,
*		EMessageSrpActive					,
*		EMessageSrpNotPermitted				,
*		EMessageHnpNotPermitted				,
*		EMessageHnpNotEnabled				,
*		EMessageHnpNotSuspended				,
*		EMessageVbusPowerUpNotPermitted		,
*		EMessageVbusPowerUpError			,
*		EMessageVbusPowerDownNotPermitted	,
*		EMessageVbusClearErrorNotPermitted	,
*		};
  */	

/*
From d32usbc.h : Device states available from client stack

enum TUsbcDeviceState
	{
	EUsbcDeviceStateUndefined,								// 0
	EUsbcDeviceStateAttached,								// 1
	EUsbcDeviceStatePowered,								// 2
	EUsbcDeviceStateDefault,								// 3
	EUsbcDeviceStateAddress,								// 4
	EUsbcDeviceStateConfigured,								// 5
	EUsbcDeviceStateSuspended,								// 6
 	EUsbcNoState = 0xff										// 255 (used as a place holder)
	};

*/

/* Used to save,encapsulate events so we can validate them at the end of a step
 */
class TOtgObservedEvent
	{
public:
	TOtgObservedEvent() {iValue = EWatcherInvalid;};
	
	TOtgObservedEvent(TWatcherNotifyType aType, TInt aValue) : iType(aType), iValue(aValue) {};
	TBool operator == (const TOtgObservedEvent&other) {return(other.iType==iType && other.iValue==iValue);};
	
	// getters
	TWatcherNotifyType 	GetType() {return(iType);};
	TInt 				GetValue() {return(iValue);};
private:	
	TWatcherNotifyType iType; 
	TInt iValue;
	};


/* OTG generic mixin. fires when otg event/state recieved - the Base class auto re-issues 
 * the request after handling the event by calling IssueAgain()
 */ 
class MOtgNotificationHandler
	{
public:
	virtual void HandleEvent(TWatcherNotifyType aType, TInt aNotificationValue)=0; 
	};


/* Sets up and then Collects notifications the user wanted
 * Become active when the last event we wanted fires
 */
class CNotifyCollector : public MOtgNotificationHandler
	{
public:
	CNotifyCollector(TRequestStatus &aStatus);
	virtual ~CNotifyCollector();

	// Create and start the observers
	void CreateObserversL(COtgRoot &aOtgDriver);
	// Stop and destroy the observers
	void DestroyObservers();
	
	// add an object to the Q
	void AddRequiredNotification(const TWatcherNotifyType aType, TInt aValue);

	//	Add an event to the list of events that indicates an instant failure
	//	(e.g. the B-Device becoming configured during a short-circuit role swap)
	void AddFailureNotification(const TWatcherNotifyType aType, TInt aValue);

	/* Ppecify a step timeout in MILLISECONDS */
	void AddStepTimeout(TInt aTimeoutMs) {
		AddRequiredNotification(EWatcherTimeouts, aTimeoutMs);};
	
	void ClearAllEvents(TBool aClearRecieved =ETrue, TBool aClearRequired =ETrue); 
	
	
	TBool EventReceivedAlready(const TOtgObservedEvent &aEvent);
	
	// from MOtgNotificationHandler
	// ... process the event, if all the needed events are satisfied, complete the object
	void HandleEvent(TWatcherNotifyType aType, TInt aNotificationValue); 
	
	// getters
	CNotifyWatcherBase* GetWatcher(TWatcherNotifyType aType);
	
	TInt DurationElapsed();

private:
	void AddRequiredOrFailureNotification(TWatcherNotifyType aType, TInt aValue, TBool aEventMeansFailure);
	TBool IsFailureEvent(TOtgObservedEvent& aEvent);
	
	void CompleteStep(TInt aCompletionCode);

private:
	TRequestStatus	&iStatusStep;	// KTestCaseWatchdogTO = failure, the test-step waits on this. 

	RArray<CNotifyWatcherBase*> iNotifyObjects;	// observers
	
	RArray<TOtgObservedEvent>   iRequiredEvents;
	RArray<TOtgObservedEvent>	iReceivedEvents;
	RArray<TOtgObservedEvent>	iFailureEvents;
	
	// duration timer
	TTime iTimeStarted;
	};


/* -----------------------------------------------------------------------------------------------
 * OTG Notification handler Base (Generic). We call these notification watchers, because they abstract 
 * OTG Messages, OTG Events, OTG States or a Watchdog timeout
 */
class CNotifyWatcherBase : public CActive
	{
public:
	virtual ~CNotifyWatcherBase() {  };
	
	virtual void StartWatching(TInt aInterval) {  TInt n(aInterval); IssueAgain(); SetActive(); };
	// getter
	TWatcherNotifyType GetType() {return(iWatchType);};		
	
protected:
	CNotifyWatcherBase (MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aRoot) : 
				iOtgRoot(aRoot), 
				iWatchType(aWatchType),
				iHandler(aHandler),	
				CActive(EPriorityStandard) { CActiveScheduler::Add(this); };
	
	// from CActive
		void RunL();
 
	// override in child

public:		
	virtual TInt GetEventValue() =0;
	virtual TInt IssueAgain()    =0; 	// override 
	virtual void DisplayEvent()  =0;		
	
protected:
	MOtgNotificationHandler & iHandler;
	COtgRoot 				&iOtgRoot; 	// driver
	TWatcherNotifyType 		iWatchType; 
	};


//-----------------------------------------------------------------------------------------
// watchdog watcher,
// this watcher converts a CTimer event into a Notification, NOTE: it does not re-Issue the timer
class COtgWatchdogWatcher : public CNotifyWatcherBase
		{
	public:
		static COtgWatchdogWatcher* NewL(MOtgNotificationHandler &wdHandler, 
										const TWatcherNotifyType aWatchType, 
										COtgRoot &aOtgRoot);
		void ConstructL();
		virtual ~COtgWatchdogWatcher() {Cancel();};
		
		void StartTimer(TInt aIntervalMs);
		void StartWatching(TInt aIntervalMs) {StartTimer(aIntervalMs); };
		
 
		
		TInt IssueAgain() { ASSERT(0); return(0);};
		void DoCancel() {iTimer.Cancel();};
		void DisplayEvent() {ASSERT(0); }; // should never fire this
	TInt GetEventValue() {return(iIntervalMs);};
	void StepExpired(TInt aInterval);		
	
protected:
	COtgWatchdogWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) :
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	// from CNotifyWatcherBase
	void RunL();
		
protected:
	TInt 	iIntervalMs;
	RTimer 	iTimer;
	};


/*--------------------------------------------------------------------------------------------
 * STATE Watcher
 * Like the other OTG watchers, this watcher will renew (resubscribe) to the relevant notification
 * again after it has fired.
 */
class COtgStateWatcher : public CNotifyWatcherBase
	{
public:
	static COtgStateWatcher* NewL(MOtgNotificationHandler &wdHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot);
	void ConstructL() {};
	
	virtual ~COtgStateWatcher() { Cancel(); };
	
	TInt IssueAgain() { iOtgRoot.otgQueueOtgStateRequest(iState , iStatus); return(ETrue);};
	void DisplayEvent(); 

	TInt GetEventValue() {return(iState);};
	
	void DoCancel() { iOtgRoot.otgCancelOtgStateRequest();};

protected:
	COtgStateWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) :
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	
	RUsbOtgDriver::TOtgState iState;
	};


//---------------------------------------
// EVENT Watcher
class COtgEventWatcher : public CNotifyWatcherBase
	{
public:
	static COtgEventWatcher* NewL(MOtgNotificationHandler &wdHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot);
	void ConstructL() {};
	virtual ~COtgEventWatcher() { Cancel(); };		
	
	TInt IssueAgain() { iOtgRoot.otgQueueOtgEventRequest(iEvent , iStatus); return(ETrue);};
	void DisplayEvent(); 
	TInt GetEventValue() {return(iEvent);};
	void DoCancel() {  iOtgRoot.otgCancelOtgEventRequest();};

protected:
	COtgEventWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) : 
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	
	RUsbOtgDriver::TOtgEvent iEvent;
	};


//---------------------------------------
// MESSAGE Watcher
class COtgMessageWatcher : public CNotifyWatcherBase
	{
public:
	static COtgMessageWatcher* NewL(MOtgNotificationHandler &wdHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot);
	void ConstructL() {};
	virtual ~COtgMessageWatcher() { Cancel(); };	
	
	TInt IssueAgain() { iOtgRoot.otgQueueOtgMessageRequest(iMessage, iStatus); return(ETrue);};
	void DisplayEvent();
	TInt GetEventValue() {return(iMessage);};
	void DoCancel() { iOtgRoot.otgCancelOtgMessageRequest();};

protected:
	COtgMessageWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) : 
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	
	RUsbOtgDriver::TOtgMessage iMessage;
	};
	
// Client state watcher
class CPeripheralStateWatcher : public CNotifyWatcherBase
	{
public:
	static CPeripheralStateWatcher* NewL(MOtgNotificationHandler &wdHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot);
	void ConstructL() {};
	virtual ~CPeripheralStateWatcher() { Cancel(); };	
	
	TInt IssueAgain() { iOtgRoot.otgQueuePeripheralStateRequest(iPeripheralState, iStatus); return(ETrue);};
	void DisplayEvent();
	TInt GetEventValue() {return(iPeripheralState);};
	void DoCancel() { iOtgRoot.otgCancelPeripheralStateRequest();};

protected:
	CPeripheralStateWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) : 
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	
	TUint iPeripheralState;	//	*usually* to be interpreted as TUsbcDeviceState, see RDevUsbcClient::AlternateDeviceStatusNotify
	};

// Client state watcher
class CAConnectionIdleWatcher : public CNotifyWatcherBase
	{
public:
	static CAConnectionIdleWatcher* NewL(MOtgNotificationHandler &wdHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot);
	void ConstructL() {};
	virtual ~CAConnectionIdleWatcher() { Cancel(); };	
	
	TInt IssueAgain() { iOtgRoot.otgQueueAConnectionIdleRequest(iAConnectionIdle, iStatus); return(ETrue);};
	void DisplayEvent();
	TInt GetEventValue() {return(iAConnectionIdle);};
	void DoCancel() { iOtgRoot.otgCancelAConnectionIdleRequest();};

protected:
	CAConnectionIdleWatcher(MOtgNotificationHandler &aHandler, const TWatcherNotifyType aWatchType, COtgRoot &aOtgRoot) : 
		CNotifyWatcherBase(aHandler, aWatchType, aOtgRoot)
			{};
	
	void RunL();	//	From CActive

	RUsbOtgDriver::TOtgConnection iAConnectionIdle;
	};


#endif // B2BWATCHERS_H
