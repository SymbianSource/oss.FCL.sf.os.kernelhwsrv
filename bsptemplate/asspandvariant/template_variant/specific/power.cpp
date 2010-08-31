// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\power.cpp
// Template Power Management
// (see also variant.cpp for a discussion on Sleep modes and xyin.cpp for example
// of usage of Resource Manager and Peripheral self power down and interaction
// with Power Controller for Wakeup Events)
// 
//

#include "template_power.h"

static TemplateResourceManager TheResourceManager;

DTemplatePowerController* TTemplatePowerController::iPowerController;


//-/-/-/-/-/-/-/-/-/ class DTemplatePowerController /-/-/-/-/-/-/-/-/-/

DTemplatePowerController::DTemplatePowerController()
	{
	Register();			// register Power Controller with Power Manager
	TTemplatePowerController::RegisterPowerController(this);
	}

void DTemplatePowerController::CpuIdle()
	{
	Arch::TheAsic()->Idle();
	}

void DTemplatePowerController::EnableWakeupEvents()
	{
	//
	// TO DO: (mandatory)
	//
	// Enable tracking of wake-up events directly in hardware. If the hardware is controlled by a Driver
	// or Extension, may need to disable interrupts and preemption around the code that accesses the hardware
	// and set up a flag which the Driver/Extension code need to read before modifying the state of that piece
	// of hardware. Note in that case the Driver/Extension may need to link to this Library.
	//

	//
	// EXAMPLE ONLY
	// In this example we simply assume that the driver will call the Power Controller every time a 
	// wakeup event occurr. It is up to the Power Controller to know if it is tracking them or not.
	// We also assume that if a wakeup event occurrs when the CPU is in Standby, this will automatically
	// bring it back from that state.
	iWakeupEventsOn = ETrue;	// start tracking wakeup events
	}

void DTemplatePowerController::DisableWakeupEvents()
	{
	//
	// TO DO: (mandatory)
	//
	// Disable tracking of wake-up events directly in hardware or if the hardware is controlled by a Driver or
	// Extension need to set up a flag which the Driver/Extension reads whenever the event occurs, in order to
	// find out if it needs to deliver notification to the Power Controller
	//
	iWakeupEventsOn = EFalse;	// stop tracking wakeup events
	}

void DTemplatePowerController::AbsoluteTimerExpired()
	{
	if (iTargetState == EPwStandby && iWakeupEventsOn)
		{
		iWakeupEventsOn = EFalse;		// one occurred, no longer track wakeup events
		WakeupEvent();
		}
	}

void DTemplatePowerController::PowerDown(TTimeK aWakeupST)	
	{
	if (iTargetState == EPwStandby)
		{
		//
		// TO DO: (mandatory)
		//
		// Converts between the Wakeup time in System Time units as passed in to this function and a Wakeup
		// time in RTC units. The following code is given as an example how to convert between System time units
		// RTC time units on a system with a 32 bit RTC timer and which is incremented on a second interval:
		//
		TUint32 wakeupRTC;
		if (aWakeupST)
			{
			TUint32 nowRTC = TTemplate::RtcData();
			TTimeK nowST = Kern::SystemTime();
			__KTRACE_OPT(KPOWER,Kern::Printf("system time: now = 0x%lx(us) wakeup = 0x%lx(us)", nowST, aWakeupST));
			if (aWakeupST < nowST)
				return;
			Int64 deltaSecs = (aWakeupST - nowST) / 1000000;
			if (deltaSecs <= 0)
				return;
			if (deltaSecs + (Int64)nowRTC > (Int64)(KMaxTInt - 2))
				wakeupRTC = (KMaxTInt - 2); // RTC can't wrap around during standby
			else
				wakeupRTC = nowRTC + deltaSecs;
			__KTRACE_OPT(KPOWER,Kern::Printf("RTC: now = %d(s) wakeup = %d(s)", nowRTC, wakeupRTC));
			}
		else
			wakeupRTC = 0;
		//
		// TO DO: (optional)
		//
		// It then uses the calculated value to program the RTC to wakeup the System at the Wakeup
		// time ans sets the CPU and remaining hardware to go to the correponding low power mode. When the 
		// state of the Core and Core Peripherals is not preserved in this mode the following is usually 
		// required:
		//	- save current Core state (current Mode, banked registers for each Mode and Stack Pointer for 
		//	  both current and User Modes
		//	- save MMU state: Control Register, TTB and Domain Access Control
		//	- Flush Dta Cache and drain Write Buffer
		//	- save Core Peripherals state: Interrupt Controller, Pin Function, Bus State and Clock settings
		// SDRAM should be put in self refresh mode. Peripheral devices involved in detection of Wakeup events
		// should be left powered.
		// The Tick timer should be disabled and the current count of this and other System timers shall be
		// saved.
		// On wakeing up the state should be restored from the save state as above. SDRAM shall be brought back
		// under CPU control, The Tick count shall be restored and timers re-enabled.

		// We assume that if a wakeup event occurrs when the CPU is in Standby, this will automatically
		// bring it back from that state. Therefore we stop tracking wakeup events as the Power Manager will
		// complete any pending notifications anyway. When the driver delivers its notification, we just ignore
		// it.

		if(wakeupRTC)
			{
			//Handle this if needed
			}

		iWakeupEventsOn = EFalse;		// tracking of wakeup events is now done in hardware
		}
	else
		{
		Kern::Restart(0x80000000);
		}
	}

//-/-/-/-/-/-/-/-/-/ class TTemplatePowerController /-/-/-/-/-/-/-/-/-/

EXPORT_C TemplateResourceManager* TTemplatePowerController::ResourceManager()
	{
	return &TheResourceManager;
	}


EXPORT_C void TTemplatePowerController::WakeupEvent()
	{
	if(!iPowerController)
		__PM_PANIC("Power Controller not present");
	else if(iPowerController->iWakeupEventsOn)
		{
		iPowerController->iWakeupEventsOn=EFalse;		// one occurred, no longer track wakeup events
		iPowerController->WakeupEvent();
		}
	}

//-/-/-/-/-/-/-/-/-/ class TemplateResourceManager /-/-/-/-/-/-/-/-/-/

void TemplateResourceManager::InitResources()
	{
	//
	// TO DO: (optional)
	//
	// Initialise any power resources required by the platform and not initialised in the Bootstrap
	//
	}

//-/-/-/-/-/-/-/-/-/ interface for shared resources /-/-/-/-/-/-/-/-/-/

void SharedBinaryResource1::Use()
	{
	NKern::Lock();		// lock Kernel as shared resource is likely to be modified from different threads
	if (iCount++ == 0)
		{
		//
		// TO DO: (optional)
		//
		// Modify hardware register bit or bits to switch the resource On. If the resource
		// can be accessed from an ISR need to disable/enable interrupts around it.
		//
		NKern::Unlock();
		//
		// TO DO: (optional)
		//
		// If the resource is asynchronous may need to sleep or block the thread until the change is complete
		//
		}
	else
		NKern::Unlock();
	}

void SharedBinaryResource1::Release()
	{
	NKern::Lock();
	__PM_ASSERT(iCount);
	if (--iCount == 0)
		{
		//
		// TO DO: (optional)
		//
		// Modify hardware register bit or bits to switch the resource Off. If the resource
		// can be accessed from an ISR need to disable/enable interrupts around it.
		//
		NKern::Unlock();
		//
		// TO DO: (optional)
		//
		// If the resource is asynchronous may need to sleep or block the thread until the change is complete
		//
		}
	else
		NKern::Unlock();
	}

TUint SharedBinaryResource1::GetCount()
	{
	return iCount;
	}

SharedMultilevelResource1::SharedMultilevelResource1()
	//
	// TO DO: (optional)
	//
	// May need to initialise current level and the Id of its owner if these have been initialised in the Bootstrap
	//
	// : iCurrentLevel(/* a level for this resource as initialised in the Bootstrap */),
	//	 iCurrentLevelOwnerId(/* the Id of the requester of this resource that requires the initial value */)
	{
	}

void SharedMultilevelResource1::IncreaseToLevel(TUint aLevel, TInt aRequester)
	{
	//
	// Drivers should use this API if they wish to request a level higher than the previous level they required 
	// Drivers should keep track of the level they require and be disciplined
	//
	NKern::Lock();
	__PM_ASSERT(aLevel<Levels[aRequester]);
	Levels[aRequester]=aLevel;
	if(aLevel > iCurrentLevel)			// need to increase the level
		{
		// if(aLevel <= MAXLEVEL)
		//	aLevel = MAXLEVEL;
		iCurrentLevel = aLevel;
		iCurrentLevelOwnerId = aRequester;
		//
		// TO DO: (optional)
		//
		// Modify hardware register bits to set the level of the resource to aLevel
		NKern::Unlock();
		//
		// TO DO: (optional)
		//
		// If the resource is asynchronous may need to sleep or block the thread until the change is complete
		//
		}
	else
		NKern::Unlock();
	}

void SharedMultilevelResource1::ReduceToLevel(TUint aLevel, TInt aRequester)
	{
	//
	// Drivers should use this API if they wish to request a level higher than the previous level they required 
	//
	NKern::Lock();
	__PM_ASSERT(aLevel>Levels[aRequester]);

	Levels[aRequester]=aLevel;
	if(aLevel < iCurrentLevel && aRequester == iCurrentLevelOwnerId)	// the holder of the current level as lowered its request
		{
		FindMaxLevel(&iCurrentLevel, &iCurrentLevelOwnerId);			// find max level required and the ID of its holder
		//
		// TO DO: (optional)
		//
		// Modify hardware register bits to set the level of the resource to iCurrentLevel
		NKern::Unlock();
		//
		// TO DO: (optional)
		//
		// If the resource is asynchronous may need to sleep or block the thread until the change is complete
		//
		}
	else
		NKern::Unlock();
	}

TUint SharedMultilevelResource1::GetResourceLevel()
	{
	return iCurrentLevel;
	}

void SharedMultilevelResource1::FindMaxLevel(TUint* aLevel, TInt* aId)
	{
	//
	// TO DO: (optional)
	//
	// Place your clever array search algorithm here...
	// return max level and id of owner
	}

TInt BinaryPowerInit();		// the Symbian example Battery Monitor and Power HAL handling

GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{
	if(aReason==KModuleEntryReasonVariantInit0)
		{
		//
		// TO DO: (optional)
		//
		// Start the Resource Manager earlier so that Variant and other extension could make use of Power Resources
		//
		__KTRACE_OPT(KPOWER, Kern::Printf("Starting Template Resource controller"));
		new(&TheResourceManager)TemplateResourceManager;
		TheResourceManager.InitResources();
		return KErrNone;
		}
	else if(aReason==KModuleEntryReasonExtensionInit0)
		{
		__KTRACE_OPT(KPOWER, Kern::Printf("Starting Template power controller"));
		//
		// TO DO: (optional)
		//
		// Start the Kernel-side Battery Monitor and hook a Power HAL handling function.
		// Symbian provides example code for both of the above in \e32\include\driver\binpower.h
		// You may want to write your own versions.
		// The call below starts the example Battery Monitor and hooks the example Power HAL handling function
		// At the end we return an error to make sure that the entry point is not called again with
		// KModuleEntryReasonExtensionInit1 (which would call the constructor of TheResourceManager again)
		//
		TInt r = BinaryPowerInit();
		if (r!= KErrNone)
			__PM_PANIC("Can't initialise Binary Power model");
		DTemplatePowerController* c = new DTemplatePowerController();
		if(c)
			return KErrGeneral;
		else
			__PM_PANIC("Can't create Power Controller");
		}
	else if(aReason==KModuleEntryReasonExtensionInit1)
		{
		// does not get called...
		}
	return KErrArgument;
	}
