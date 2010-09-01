// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\power.cpp
// Generic power management
// 
//

#include <kernel/kpower.h>
#include <kernel/kern_priv.h>
#include "execs.h"
#include "msgqueue.h"

// #define _DEBUG_POWER


/******************************************************************************
 * Power Manager - a Power Model implementation
 ******************************************************************************/
#ifndef __X86__
DPowerController* TPowerController::ThePowerController = 0;
#endif

class DPowerManager : public DPowerModel
	{
public:
	// from DPowerModel
	void CpuIdle();
	void RegisterUserActivity(const TRawEvent& anEvent);
	TInt PowerHalFunction(TInt aFunction, TAny* a1, TAny* a2);
	void AbsoluteTimerExpired();
	void SystemTimeChanged(TInt anOldTime, TInt aNewTime);
	TSupplyStatus MachinePowerStatus();

public:

	static DPowerManager* New();

	TInt EnableWakeupEvents(TPowerState);
	void DisableWakeupEvents();
	void RequestWakeupEventNotification(TRequestStatus*);
	void CancelWakeupEventNotification();
	TInt PowerDown();

	void AppendHandler(DPowerHandler*);
	void RemoveHandler(DPowerHandler*);
	void WakeupEvent();

	DPowerController*	iPowerController;
	DBatteryMonitor*	iBatteryMonitor;
	DPowerHal*			iPowerHal;

	TInt				iTotalCurrent;

private:

	DPowerManager();

	// Acquire the feature lock
	// Called in CS
	void Lock()
		{
		Kern::MutexWait(*iFeatureLock);
		}

	// Release the feature lock
	// Called in CS
	void Unlock()
		{
		Kern::MutexSignal(*iFeatureLock);
		}
//
// Currently not used
//
// #ifdef _DEBUG
//	TBool IsLocked()
		// used in assertions only
//		{ return iFeatureLock->iCleanup.iThread == TheCurrentThread; }
// #endif
	
	void NotifyWakeupEvent(TInt aReason);

	DMutex*			iFeatureLock;
	DThread*		iClient;				// protected by the system lock
	TClientRequest*	iRequest;				// protected by the system lock
	DPowerHandler*	iHandlers;				// protected by the feature lock
	};

static DPowerManager* PowerManager;

class DummyPowerController : public DPowerController
	// Used when the plaform doesn't support power management
	{	 
public: // from DPowerController
	void CpuIdle() {} 
	void EnableWakeupEvents() { WakeupEvent(); }
	void DisableWakeupEvents() {}
	void AbsoluteTimerExpired() {}
	void PowerDown(TTimeK /* aWakeupTime */) { if (iTargetState == EPwOff) __PM_PANIC("Can't power off"); }
	};

TInt PowerModelInit()
	{
#ifdef _DEBUG_POWER
	TInt maskIndex = KPOWER / 32;
	TUint32 bitMask = 1 << (KPOWER % 32);
	TheSuperPage().iDebugMask[maskIndex] |= bitMask;
#endif
	__KTRACE_OPT2(KBOOT,KPOWER,Kern::Printf("PowerModelInit()"));
	DPowerManager* pm = DPowerManager::New();
	if (!pm)
		return KErrNoMemory;
	PowerManager = pm;
	return KErrNone;
	}

_LIT(KPowerMgrMutexName, "PwrMgrLock");

DPowerManager* DPowerManager::New()
	{
	DPowerManager* self = new DPowerManager();
	if (!self)
		return NULL;
	if (Kern::MutexCreate(self->iFeatureLock, KPowerMgrMutexName, KMutexOrdPowerMgr) != KErrNone)
		return NULL; // Do not cleanup since kernel will panic eventually
	self->iPowerController = new DummyPowerController();
	if (self->iPowerController == NULL)
		return NULL; // Do not cleanup since kernel will panic eventually
	TInt r = Kern::CreateClientRequest(self->iRequest);
	if (r != KErrNone)
		return NULL; // Do not cleanup since kernel will panic eventually
	return self;
	}

DPowerManager::DPowerManager() 
	{
	}

void DPowerManager::CpuIdle()
	{ // from DPowerModel
	__PM_ASSERT(iPowerController);
	iPowerController->CpuIdle();
	}

void DPowerManager::RegisterUserActivity(const TRawEvent&)
	{ // from DPowerModel
	}	

void DPowerManager::SystemTimeChanged(TInt aOldTime, TInt aNewTime)
	{ // from DPowerModel
	if (iBatteryMonitor)
		iBatteryMonitor->SystemTimeChanged(aOldTime, aNewTime);
	}

TSupplyStatus DPowerManager::MachinePowerStatus()
	{ // from DPowerModel
	TSupplyStatus s;
	if (iBatteryMonitor)
		s = iBatteryMonitor->MachinePowerStatus();
	else
		s = EGood;
	return s;
	}

TInt DPowerManager::PowerHalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{ // from DPowerModel
	__KTRACE_OPT(KPOWER,Kern::Printf(">DPowerManager::PowerHalFunction() func=0x%x, a1=0x%x, a2=0x%x", aFunction, a1, a2));
	TInt r;
	if (iPowerHal)
		r = iPowerHal->PowerHalFunction(aFunction, a1, a2);
	else
		r = KErrNotSupported;

	__KTRACE_OPT(KPOWER,Kern::Printf("<DPowerManager::PowerHalFunction() return 0x%x", r));
	return r;
	}

void DPowerManager::AbsoluteTimerExpired()
	{ // from DPowerModel  
	if (iPowerController->iTargetState != EPwActive)
		iPowerController->AbsoluteTimerExpired();
	}

// Called in CS
void DPowerManager::AppendHandler(DPowerHandler* aPh)
	{ // called by drivers (power handler)
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::AppendHandler('%lS')", &aPh->iName));
	__ASSERT_CRITICAL;
	//Check to ensure that handler is not added multiple times and not part of any other list
	__PM_ASSERT(!(aPh->iPrev) && !(aPh->iNext));
	Lock();
	//Create circular doubly linked power handler list
	if(iHandlers == NULL)
		{ //Empty list, create first entry
		iHandlers = aPh;
		aPh->iNext = iHandlers;
		aPh->iPrev = iHandlers;
		}
	else
		{ //Append to end of the list.
		aPh->iNext = iHandlers->iNext;
		iHandlers->iNext->iPrev = aPh;
		aPh->iPrev = iHandlers;
		iHandlers->iNext = aPh;
		iHandlers = aPh;
		}
	Unlock();
	}	

// Called in CS
void DPowerManager::RemoveHandler(DPowerHandler* aPh)
	{ // called by drivers (power handler)
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::RemoveHandler('%lS')", &aPh->iName));
	__ASSERT_CRITICAL;
	Lock();
	__PM_ASSERT(aPh);
	
	if ((iHandlers == NULL) //If the list is empty
		|| (aPh->iNext == NULL || aPh->iPrev == NULL)) //or entry already removed then return immediately
		{
		aPh->iPrev = aPh->iNext = NULL;
		Unlock();
		return;
		}

	DPowerHandler *temp = aPh->iPrev;
	if(aPh == temp) //Only one node in the list.
		iHandlers = NULL;
	else
		{ //Remove the entry from list
		temp->iNext = aPh->iNext;
		temp = aPh->iNext;
		temp->iPrev = aPh->iPrev;
		if(aPh == iHandlers) //If the node is the last node in the list then make iHandler point to last one.
			iHandlers = aPh->iPrev;
		}
	//Make next and prev pointers to NULL
	aPh->iPrev = aPh->iNext = NULL;
	Unlock();
	}	

void DPowerManager::WakeupEvent()
	{ // called by power controller  
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::WakeupEvent()"));
	if (iPowerController->iTargetState != EPwActive)
		{
		NKern::ThreadEnterCS();
		NotifyWakeupEvent(KErrNone);
		NKern::ThreadLeaveCS();
		}
	}

TInt DPowerManager::EnableWakeupEvents(TPowerState aState)
	{ // called by ExecHandler
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::EnableWakeupEvents(0x%x)", aState));
	if ((aState < 0) || (EPwLimit <= aState)) 
		return KErrArgument;
	if (aState == EPwActive)
		return KErrArgument;
	Lock();
	if (iPowerController->iTargetState != EPwActive)
		iPowerController->DisableWakeupEvents();
	iPowerController->iTargetState = aState;
	iPowerController->EnableWakeupEvents();
	Unlock();
	return KErrNone;
	}	

void DPowerManager::DisableWakeupEvents()
	{ // called by ExecHandler
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::DisableWakeupEvents()"));
	Lock();
	if (iPowerController->iTargetState != EPwActive)
		{
		iPowerController->iTargetState = EPwActive;
		iPowerController->DisableWakeupEvents();
		}
	Unlock();
	}	

void DPowerManager::RequestWakeupEventNotification(TRequestStatus* aStatus)
	{ // called by ExecHandler
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::RequestWakeupEventNotification()"));
	Lock();		// we aquire this lock to avoid new requests while in PowerDown
	NKern::LockSystem();
	if (iClient || iRequest->SetStatus(aStatus) != KErrNone)
		Kern::RequestComplete(aStatus, KErrInUse);
	else
		{
		iClient = TheCurrentThread;
		iClient->Open();
		}
	NKern::UnlockSystem();
	Unlock();
	__KTRACE_OPT(KPOWER,Kern::Printf("<PowerManger::RequestWakeupEventNotification()"));
	}	

void DPowerManager::NotifyWakeupEvent(TInt aReason)
	{ // private
	NKern::LockSystem();
	DThread* client = iClient;
	if (!client)
		{
		NKern::UnlockSystem();
		return;
		}
	iClient = NULL;
	Kern::QueueRequestComplete(client, iRequest, aReason);
	NKern::UnlockSystem();
	client->Close(NULL);
	}

void DPowerManager::CancelWakeupEventNotification()
	{ // called by ExecHandler
	__KTRACE_OPT(KPOWER,Kern::Printf("PowerManger::CancelWakeupEventNotification()"));
	NKern::ThreadEnterCS();
	NotifyWakeupEvent(KErrCancel);
	NKern::ThreadLeaveCS();
	}

// Called in CS
TInt DPowerManager::PowerDown()
	{ // called by ExecHandler 
	__KTRACE_OPT(KPOWER,Kern::Printf(">PowerManger::PowerDown(0x%x) Enter", iPowerController->iTargetState));
	__ASSERT_CRITICAL;

	Lock();


	if (iPowerController->iTargetState == EPwActive)
		{
		Unlock();
		return KErrNotReady;
		}

    __PM_ASSERT(iHandlers);
	NFastSemaphore sem(0);
	DPowerHandler* ph = iHandlers;
	//Power down in reverse order of handle registration.
	do
		{
#ifdef _DEBUG_POWER
		__PM_ASSERT(!(ph->iStatus & DPowerHandler::EDone));
#endif
		ph->iSem = &sem;
		ph->PowerDown(iPowerController->iTargetState);
#ifdef _DEBUG_POWER
		__PM_ASSERT(!(ph->iStatus & DPowerHandler::EDone));
		NKern::FSWait(&sem);	// power down drivers one after another to simplify debug
		__PM_ASSERT(!ph->iSem);
		__PM_ASSERT(ph->iStatus & EDone);
		ph->iStatus &= ~EDone;
#endif
		ph = ph->iPrev;
		}while(ph != iHandlers);

#ifndef _DEBUG_POWER
	ph = iHandlers;
	do
		{
		NKern::FSWait(&sem);
		ph = ph->iPrev;
		}while(ph != iHandlers);
#endif

	TTickQ::Wait();

	iPowerController->PowerDown(K::SecondQ->WakeupTime());
	__PM_ASSERT(iPowerController->iTargetState != EPwOff);
	iPowerController->iTargetState = EPwActive;

	K::SecondQ->WakeUp();
	TTickQ::Signal();

	ph = iHandlers->iNext;
	//Power up in same order of handle registration.
	do
		{
#ifdef _DEBUG_POWER
		__PM_ASSERT(!(ph->iStatus & DPowerHandler::EDone));
#endif
		ph->iSem = &sem;
		ph->PowerUp();
#ifdef _DEBUG_POWER
		__PM_ASSERT(!(ph->iStatus & DPowerHandler::EDone));
		NKern::FSWait(&sem);	// power down drivers one after another to simplify debug
		__PM_ASSERT(!ph->iSem);
		__PM_ASSERT(ph->iStatus & EDone);
		ph->iStatus &= ~EDone;
#endif
		ph = ph->iNext;
		}while(ph != iHandlers->iNext);

#ifndef _DEBUG_POWER
	ph = iHandlers->iNext;
	do
		{
		NKern::FSWait(&sem);
		ph = ph->iNext;
		}while(ph != iHandlers->iNext);
#endif

	// complete wakeup notification request if any
	NotifyWakeupEvent(KErrNone); 

	Unlock();

	__KTRACE_OPT(KPOWER,Kern::Printf("<PowerManger::PowerDown() Leave"));

	return KErrNone;
	}	

/******************************************************************************
 * Power Handlers
 ******************************************************************************/



/**
Constructor.
		
@param aName The power handler's debug-purpose symbolic name.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C DPowerHandler::DPowerHandler(const TDesC& aName) : iName(aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerHandler::DPowerHandler");
	}



/**
Destructor.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C DPowerHandler::~DPowerHandler()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerHandler::~DPowerHandler");
	SetCurrentConsumption(0);
	}



/**
Registers the power handler with the power manager.
	
After registration, the power manager notifies the device driver about
power state transitions by calling PowerDown() & PowerUp() in this power
handler's interface.

Note that the implementation of Add() acquires an internal lock (a DMutex),
which is also held when the power manager calls PowerDown() and PowerUp(). 
This means that the device driver cannot hold a lock over Add() calls if
the same lock is acquired by PowerDown() and PowerUp() implementations.

You can find an example of synchronization between outgoing Add()
and incoming PowerDown() & PowerUp() in e32/drivers/ecomm/d_comm.cpp.

@see DPowerHandler::Remove()

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerHandler::Add()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerHandler::Add");
	PowerManager->AppendHandler(this);
	}




/**
De-registers the power handler.
	
After the call to this function completes, the power manager stops notifying
the device driver about kernel power state transitions.

Note that the implementation of Remove() acquires an internal lock (a DMutex),
which is also held when the power manager calls PowerDown() and PowerUp(). 
This means that the device driver cannot hold a lock over Remove() calls if
the same lock is acquired by PowerDown() and PowerUp() implementations.

You can find an example of synchronization between outgoing Remove()
and incoming PowerDown() & PowerUp() in e32/drivers/ecomm/d_comm.cpp.

@see DPowerHandler::Add()

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerHandler::Remove()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerHandler::Remove");
	PowerManager->RemoveHandler(this);
	}


void DPowerHandler::Done()
	{ // private
#ifdef _DEBUG_POWER
	__PM_ASSERT(!(iStatus & EDone));
	iStatus |= EDone;
#endif
	NKern::Lock();
	__KTRACE_OPT(KPOWER,Kern::Printf("DPowerHandler::Done('%lS') sem=0x%x", &iName, iSem));
	NFastSemaphore* sem = (NFastSemaphore*)__e32_atomic_swp_ord_ptr(&iSem, 0);
	if (sem)
		sem->Signal();
	NKern::Unlock();
	}




/**
Signals power up completion.

A device driver calls this function precisely once in response to a PowerUp() call.
It can be called by any thread at any time after PowerUp() has been entered.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerHandler::PowerUpDone()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DPowerHandler::PowerUpDone");
	Done();
	}



/**
Signals power down completion.

A device driver calls this function precisely once in response to a PowerDown() call.
It can be called by any thread at any time after PowerDown() has been entered.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerHandler::PowerDownDone()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DPowerHandler::PowerDownDone");
	Done();
	}



/**	@deprecated, no replacement	*/
EXPORT_C void DPowerHandler::DeltaCurrentConsumption(TInt aDelta)
	{
	__e32_atomic_add_ord32(&iCurrent, aDelta);
	__PM_ASSERT(iCurrent >= 0);
	__e32_atomic_add_ord32(&PowerManager->iTotalCurrent, aDelta);
	__PM_ASSERT(PowerManager->iTotalCurrent >= 0);
	}

/**	@deprecated, no replacement	*/
EXPORT_C void DPowerHandler::SetCurrentConsumption(TInt aCurrent)
	{
	__PM_ASSERT(aCurrent >= 0);
	TInt old = (TInt)__e32_atomic_swp_ord32(&iCurrent, aCurrent);
	TInt delta = aCurrent - old;
	__e32_atomic_add_ord32(&PowerManager->iTotalCurrent, delta);
	__PM_ASSERT(PowerManager->iTotalCurrent >= 0);
	}

/******************************************************************************
 * Power Controller
 ******************************************************************************/




/**
Default constructor.

Typically a variant implementation creates a power controller object at
variant initialization time.

Only one power controller can be created.
		
The object can never be destroyed.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C DPowerController::DPowerController()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerController::DPowerController");
	iTargetState = EPwActive;
#ifndef __X86__
	iResourceControllerData.iResourceController = NULL;
	iResourceControllerData.iClientId = -1;
#endif
	}

/**
Registers this power controller object with the power manager.

The power manager can only use the power controller after registration.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerController::Register()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerController::Register");
	__KTRACE_OPT2(KBOOT,KPOWER,Kern::Printf("DPowerController::Register()"));
	delete PowerManager->iPowerController;
	PowerManager->iPowerController = this;
#ifndef __X86__
	TPowerController::ThePowerController = this;
#endif
	K::PowerModel = PowerManager;
	}

#ifndef __X86__
/**
Registers resource controller with power controller
This function is called only by resource controller to register itself with power controller
*/
EXPORT_C TInt DPowerController::RegisterResourceController(DBase* aController, TInt aClientId)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("DPowerController::Register()"));
    if(iResourceControllerData.iResourceController)
        return KErrAlreadyExists;
    //Store the Resource controller pointer
    iResourceControllerData.iResourceController = aController;
    //Store the client Id generated by Resource controller for Power controller to use.
    iResourceControllerData.iClientId = aClientId;
    return KErrNone;
	}

/**
  Interface class to give access to the power controller to base port components.
*/
EXPORT_C DPowerController* TPowerController::PowerController()
	{
    return TPowerController::ThePowerController;
	}
#endif

/**
Signals a wakeup event.

The power controller must signal wakeup events when enabled,
through EnableWakeupEvents() and DisableWakeupEvents(), and 
when iTargetState is not equal to EPwActive.

@see DPowerController::iTargetState

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void DPowerController::WakeupEvent()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPowerController::WakeupEvent");
	PowerManager->WakeupEvent();
	}


/******************************************************************************
 * Battery Monitor
 ******************************************************************************/

EXPORT_C DBatteryMonitor::DBatteryMonitor()
	{
	}

EXPORT_C void DBatteryMonitor::Register()
	{
	__KTRACE_OPT2(KBOOT,KPOWER,Kern::Printf("DBatteryMonitor::Register()"));
	PowerManager->iBatteryMonitor = this;
	}

/******************************************************************************
 * Power Hal
 ******************************************************************************/

EXPORT_C DPowerHal::DPowerHal()
	{
	}

EXPORT_C void DPowerHal::Register()
	{
	__KTRACE_OPT2(KBOOT,KPOWER,Kern::Printf("DPowerHal::Register()"));
	PowerManager->iPowerHal = this;
	}

/******************************************************************************
 * Kern
 ******************************************************************************/

/** Checks power status
	@return ETrue if status is good, EFalse otherwise
*/

EXPORT_C TBool Kern::PowerGood()
	{
	return K::PowerGood;
	}

/******************************************************************************
 * Exec Handlers
 ******************************************************************************/

TInt ExecHandler::PowerEnableWakeupEvents(TPowerState aState)
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Power::EnableWakeupEvents")))
		K::UnlockedPlatformSecurityPanic();
	NKern::ThreadEnterCS();	
	TInt r = PowerManager->EnableWakeupEvents(aState);
	NKern::ThreadLeaveCS();
	return r;
	}	

void ExecHandler::PowerDisableWakeupEvents()
	{ 
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Power::DisableWakeupEvents")))
		K::UnlockedPlatformSecurityPanic();
	NKern::ThreadEnterCS();	
	PowerManager->DisableWakeupEvents();
	NKern::ThreadLeaveCS();
	}	

void ExecHandler::PowerRequestWakeupEventNotification(TRequestStatus* aStatus)
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Power::RequestWakeupEventNotification")))
		K::UnlockedPlatformSecurityPanic();
	NKern::ThreadEnterCS();
	PowerManager->RequestWakeupEventNotification(aStatus);
	NKern::ThreadLeaveCS();
	}	

void ExecHandler::PowerCancelWakeupEventNotification()
	{ 
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Power::CancelWakeupEventNotification")))
		K::UnlockedPlatformSecurityPanic();
	PowerManager->CancelWakeupEventNotification();
	}	

TInt ExecHandler::PowerDown()
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Power::PowerDown")))
		K::UnlockedPlatformSecurityPanic();
	NKern::ThreadEnterCS();
	TInt r = PowerManager->PowerDown();
	NKern::ThreadLeaveCS();
	return r;
	}

