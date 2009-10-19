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
// template\template_variant\inc\template_power.inl
// Template Power Management Inline file
// -/-/-/-/-/-/-/-/-/ class TTemplatePowerController /-/-/-/-/-/-/-/-/-/
// 
//

inline void TTemplatePowerController::RegisterPowerController(DTemplatePowerController* aPowerController)
	{iPowerController = aPowerController;}

//-/-/-/-/-/-/-/-/-/ class TemplateResourceManager /-/-/-/-/-/-/-/-/-/

inline TemplateResourceManager::TSleepModes TemplateResourceManager::MapSleepMode(TInt aSleepPeriod)
	{
	//
	// TO DO: (optional)
	//
	// Investigate what resources are On or Off, or used at what level and with the expected duration
	// of Sleep map this to platform-specific Sleep mode
	//
	return Snooze;
	}

inline void TemplateResourceManager::Modify(TResource aResource, TBool aOnOff)
	{
	//
	// TO DO: (optional)
	//
	// This function is used to modify non-shared binary resources 
	// The following is an EXAMPLE ONLY:
	//
	switch(aResource)
		{
	case SynchBinResourceUsedByZOnly:
		NKern::Lock();
		//
		// TO DO: (optional)
		//
		// Modify hardware register bit or bits to switch the resource On or Off as defined by aOnOff
		// If the resource is only accessed by a driver and not from an ISR, there's no need to stop
		// preemption. If it can be accessed from an ISR need to disable/enable interrupts around it.
		//
		NKern::Unlock();
		break;

	case AsynchBinResourceUsedByZOnly:
		//
		// TO DO: (optional)
		//
		// Modify hardware register bit or bits to switch the resource On or Off as defined by aOnOff
		// and then wait until it has been modified.
		// If the waits is only a few uS you could consider spinning, If it is considerable larger then
		// you may need to use Kern::PollingWait passing a polling function, a pointer to a owning
		// object a poll period in milliseconds and a maximum number of attempts. This will sleep
		// the driver thread so if your driver is multithreaded and the resource can be accessed
		// from more than one thread you may need to lock accesses to it with a fast Mutex.
		// The completion of the change may be indicated by an interrupt: you still need to guarantee
		// that the resource is not accessed until the change takes place.
		//
		break;
	default:
		break;
		}
	}

inline void TemplateResourceManager::ModifyToLevel(TResource aResource, TInt aLevel)
	{
	//
	// TO DO: (optional)
	//
	// This function is used to modify non-shared multilevel resources
	// The following is an EXAMPLE ONLY:
	//
	switch(aResource)
		{
	case SynchMlResourceUsedByXOnly:
		NKern::Lock();
		//
		// TO DO: (optional)
		//
		// Modify hardware register bits to set the level of the resource to aLevel
		// If the resource is only accessed by a driver and not from an ISR, there's no need to stop
		// preemption. If it can be accessed from an ISR need to disable/enable interrupts around it.
		//
		NKern::Unlock();
		break;

	case AsynchMlResourceUsedByXOnly:
		//
		// TO DO: (optional)
		//
		// Modify hardware register bits to set the level of the resource to aLevel
		// and then wait until it has been modified.
		// If the waits is only a few uS you could consider spinning, If it is considerable larger then
		// you may need to use Kern::PollingWait passing a polling function, a pointer to a owning
		// object a poll period in milliseconds and a maximum number of attempts. This will sleep
		// the driver thread so if your driver is multithreaded and the resource can be accessed
		// from more than one thread you may need to lock accesses to it with a fast Mutex.
		// The completion of the change may be indicated by an interrupt: you still need to guarantee
		// that the resource is not accessed until the change takes place.
		//
		break;
	default:
		break;
		}
	}

inline TBool TemplateResourceManager::GetResourceState(TResource aResource)
	{
	//
	// TO DO: (optional)
	//
	// Read from hardware (or from follower variable) and return the state of non-shared binary resource
	// EXAMPLE ONLY
	//
	return(EFalse);
	}


inline TUint TemplateResourceManager::GetResourceLevel(TResource aResource)
	{
	//
	// TO DO: (optional)
	//
	// Read from hardware (or from follower variable) and return the level of non-shared multilevel resource
	//
	// EXAMPLE ONLY
	//
	return(0);
	}

inline SharedBinaryResource1* TemplateResourceManager::SharedBResource1()
	{return &iSharedBResource1;}

inline SharedMultilevelResource1* TemplateResourceManager::SharedMlResource1()
	{return &iSharedMlResource1;}
