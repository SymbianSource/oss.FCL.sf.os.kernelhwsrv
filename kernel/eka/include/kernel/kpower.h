// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\kpower.h
// Public header for power management
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


#ifndef __K32POWER_H__
#define __K32POWER_H__

#include <e32power.h>
#include <kernel/kernel.h>

/**
@internalComponent
*/
#define __PM_ASSERT(aCond) \
	__ASSERT_DEBUG( (aCond), \
		( \
			Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" __FILE__ "' Line: %d\n", __LINE__), \
			Kern::Fault("Power Management", 0) \
		) )

/**
@internalComponent
*/
#define __PM_PANIC(aMsg) \
	(\
		Kern::Printf("PANIC:'" aMsg "';\nFile: '" __FILE__ "' Line: %d\n", __LINE__), \
		Kern::Fault("Power Management", 0) \
	)




/**
@publishedPartner
@released

Interface and support functions for a power controller implementation.
	  
A power controller implementation depends on the specific power management
hardware and is typically variant-dependent.

The class defines the interface that any power controller implementation
must provide to the generic kernel-side power manager. 
It also provides the power controller with an API to the power manager.
*/
class DPowerController : public DBase
	{
public:
	IMPORT_C DPowerController();
	IMPORT_C void Register();
	IMPORT_C void Register(TUint aShutdownTimeoutMs);
	IMPORT_C void WakeupEvent();
#ifndef __X86__
	IMPORT_C TInt RegisterResourceController(DBase* aController, TInt aClientId);
protected:
    struct SResourceControllerData
		{
        DBase* iResourceController;
        TInt iClientId;
		}iResourceControllerData;
#endif
public:


	/**
	The target power state of the last, possibly still not completed,
	kernel transition.
	*/
	volatile TPowerState	iTargetState;
public:


	/**
	Puts the CPU into the Idle mode.
	*/
	virtual void CpuIdle() = 0;

	
	/**
	Enables wakeup events.

	When called, iTargetState is guaranteed NOT to be equal to EPwActive.

	After this call, and until a DisableWakeupEvents() or PowerDown() call,
	the power controller must track and signal wakeup events corresponding
	to iTargetState. 
	
	@see DPowerController::iTargetState
	@see TPowerState
	*/
	virtual void EnableWakeupEvents() = 0;

	
	/**
	Disables wakeup events.

	When called, iTargetState is guaranteed to be equal to EPwActive.

	After this call, the power controller must stop signalling wakeup events.
	
    @see DPowerController::iTargetState
   	@see TPowerState
	*/
	virtual void DisableWakeupEvents() = 0;
	
	
	/**
	Notifies an absolute timer expiration.

	The	power controller implementation must call WakeupEvent() if absolute
	timer expiration is currently tracking wakeup events.
	*/
	virtual void AbsoluteTimerExpired() = 0;
	
	
	/**
	Puts the CPU into the low power state.

	When called, iTargetState is guaranteed NOT to be equal to EPwActive.

	If iTargetState is EPwStandby, the power controller will put
	the hardware into standby.

	If at least one wakeup event has been detected since the last
	call to EnableWakeupEvents(), then PowerDown() returns immediately;
	otherwise, PowerDown() returns when a wakeup event occurs.

	When PowerDown() returns, wakeup events must be considered as disabled.

	If iTargetState is EPwOff, then PowerDown() must never return.
	Typically, it turns the platform off, but may perform any other
	platform-specific action such as system reboot.

	@param	aWakeupTime If not zero, specifies the system time when
	                    the system will wakeup.
	                    
	@see DPowerController::iTargetState
    @see TPowerState
	*/
	virtual void PowerDown(TTimeK aWakeupTime) = 0;

	/**
	Registers resources of interest for Idle with Resource Manager

	Function also provided for power controller to perform other operations if required.
	*/
	virtual TInt DoRegisterResourceController()
		{
		return KErrNone;
		}
	};

#ifndef __X86__
/**
@internalComponent
@prototype 9.5
*/
class TPowerController
	{
public:
	IMPORT_C static DPowerController* PowerController();
public:
	static DPowerController* ThePowerController;
	};
#endif

/**
@internalComponent
*/
class DBatteryMonitor
	{
public:
	IMPORT_C DBatteryMonitor();
	IMPORT_C void Register();
public:
	virtual TSupplyStatus MachinePowerStatus() = 0;
	virtual void SystemTimeChanged(TInt anOldTime, TInt aNewTime) = 0;
	};

/**
@internalComponent
*/
class DPowerHal : public DBase
	{
public:
	IMPORT_C DPowerHal();
	IMPORT_C void Register();
public:
	virtual TInt PowerHalFunction(TInt aFunction, TAny* a1, TAny* a2) = 0;
	};


/**
@publishedPartner
@released

Interface and support functions for a device driver's power-handler.
	  
There is typically one power handler object per peripheral. The object
is typically implemented by	the peripheral's device driver.

The class defines the interface that the driver must provide to
the generic kernel-side power manager.
		
It also provides the driver with an API to the kernel-side power manager.
*/
class DPowerHandler : public DBase
	{
public: // from DBase
	IMPORT_C ~DPowerHandler();
public:
	IMPORT_C DPowerHandler(const TDesC& aName);
	IMPORT_C void Add();
	IMPORT_C void Remove();
	IMPORT_C void PowerUpDone();
	IMPORT_C void PowerDownDone();
	/**	@deprecated, no replacement	*/
	IMPORT_C void SetCurrentConsumption(TInt aCurrent);
	/**	@deprecated, no replacement	*/
	IMPORT_C void DeltaCurrentConsumption(TInt aCurrent);
public: 
	/**
	Requests peripheral power down.

	The power manager calls PowerDown() during a transition to standby
	or power off.
	
	The driver must signal the completion of peripheral power down to
	the power manager by calling PowerDownDone().
	Note that PowerDownDone() can be called from the path of PowerDown(),
	as well as asynchronously by another thread before or
	after PowerDown() returns.

    Note that the implementation of Add() & Remove() acquires
    an internal lock (a DMutex), which is also held when the power manager
    calls PowerDown(). This means that the device driver cannot hold a lock
    over Add() & Remove() calls if the same lock is acquired
    by PowerDown() implementations.

    You can find an example of synchronization between outgoing Add() & Remove()
    and incoming PowerDown() in e32/drivers/ecomm/d_comm.cpp.

	@param aState the target power state; can be EPwStandby or EPwOff only
	*/
	virtual void PowerDown(TPowerState aState) = 0;

	/**
	Notifies the peripheral of system power up.

	The power manager calls PowerUp() during a transition from standby.
	
	It is up to the device driver's policy whether to power up the periphiral or not.
	The driver must signal the completion of the operation to the power manager by calling PowerUpDone().
	Note that PowerUpDone() can be called from the path of PowerUp(), as well as asynchronously by another
	thread before or after PowerUp() returns.

    Note that the implementation of Add() & Remove() acquires
    an internal lock (a DMutex), which is also held when the power manager
    calls PowerUp(). This means that the device driver cannot hold a lock
    over Add() & Remove() calls if the same lock is acquired
    by PowerUp() implementations.

    You can find an example of synchronization between outgoing Add() & Remove()
    and incoming PowerUp() in e32/drivers/ecomm/d_comm.cpp.
	*/
	virtual void PowerUp() = 0;

private:
	friend class DPowerManager;
	
	typedef TUint8 TStatus;
	enum { EDone = 0x01 };

	void Wait();
	void Done();

	const TDesC&	iName;
	DPowerHandler*	iNext;
	DPowerHandler*	iPrev;
	NFastSemaphore*	iSem;
	TStatus			iStatus;
	TUint8			i_DPowerHandler_Spare[3];
	TInt			iCurrent;
	};




/**
@publishedPartner
@released

The recommended interface for objects that represent shared power sources.
	
The objects representing shared power sources are typically implemented by
the variant and used by device drivers.

We recommend that these objects implement the MPowerInput interface.
*/
class MPowerInput
	{
public:


	/**
	Signals that the power source is in use.

	Typically, a driver calls this function when it needs the source
	to be powered on.
		
	A typical implementation associates a counter with the object.
	The initial counter's value is 0. Use() increments the counter and, if
	the counter's value changes from 0 to 1, powers on the source.    
	*/
	virtual void Use() = 0;
	
	
	/**
	Signals that the power source is not in use.

	Typically, a driver calls this function when it no longer needs
	the source to be powered on.

	A typical implementation associates a counter with the object.
	The initial counter's value is 0. While the implementation of
	Use() would increment the counter, Release() would decrement it.
	If the counter's value changes from 1 to 0, Release() powers off
	the source.
	*/
	virtual void Release() = 0;
	};

//
// Kernel private
//

/**
@internalAll
*/
class DPowerModel : public DBase
	{
public:
	virtual void AbsoluteTimerExpired() = 0;
	virtual void RegisterUserActivity(const TRawEvent& anEvent) = 0;
public:
	virtual void CpuIdle() = 0;
public:
	virtual void SystemTimeChanged(TInt anOldTime, TInt aNewTime) = 0;
	virtual TSupplyStatus MachinePowerStatus() = 0;
public:
	virtual TInt PowerHalFunction(TInt aFunction, TAny* a1, TAny* a2) = 0;
	};

TInt PowerModelInit();

#endif

