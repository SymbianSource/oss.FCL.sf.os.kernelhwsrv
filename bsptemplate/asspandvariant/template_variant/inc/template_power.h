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
// template\template_variant\inc\template_power.h
// Template Power Management Header
// (see also assp.cpp for a discussion on Sleep modes and xyin.cpp for example
// of usage of Resource Manager and Peripheral self power down and interaction
// with Power Controller for Wakeup Events)
// 
//

#ifndef __PM_STD_H__
#define __PM_STD_H__
#include <kernel/kpower.h>
#include "variant.h"

//
// TO DO: (mandatory)
//
// Definition of the DPowerController derived class
//
NONSHARABLE_CLASS(DTemplatePowerController) : public DPowerController
	{
public: // from DPowerController
	void CpuIdle();
	void EnableWakeupEvents();
	void AbsoluteTimerExpired();
	void DisableWakeupEvents();
	void PowerDown(TTimeK aWakeupTime);
public:
	DTemplatePowerController();
private:
	void DoStandby(TBool aTimed, TUint32 aWakeupRTC);
public:
	TBool iWakeupEventsOn;
	};

//
// TO DO: (optional)
//
// If you need to access to the Power Controller from Drivers/Extensions/Variant 
// or access to Resource Manager then define an accessor class as below
//

class TemplateResourceManager;

class TTemplatePowerController
	{
public:
	// to allow Variant/Drivers/Extensions access to Resource Manager
	IMPORT_C static TemplateResourceManager* ResourceManager();
	// used by drivers/extensions to signal a wakeup event to Power Controller
	IMPORT_C static void WakeupEvent();

	inline static void RegisterPowerController(DTemplatePowerController* aPowerController);
private:
	static DTemplatePowerController* iPowerController;
	};

NONSHARABLE_CLASS(SharedBinaryResource1) : public MPowerInput
	{
public: // from MPowerInput
	void Use();
	void Release();
public:
	TUint GetCount();		// gets current usage count
private:
	TUint	iCount;
	};

// Prototype class for Multilevel resources

class MSharedMultilevelResource			// Multilevel Shared Input
	{
public:
	virtual void IncreaseToLevel(TUint aLevel, TInt aRequester) = 0;
	virtual void ReduceToLevel(TUint aLevel, TInt aRequester) = 0;
	virtual TUint GetResourceLevel() = 0;
	};

// EXAMPLE ONLY
const TUint KSharers = 3;

NONSHARABLE_CLASS(SharedMultilevelResource1) : public MSharedMultilevelResource
	{
public:
	enum TRequesterId
		{
		//
		// Identify all possible users of this resource
		//
		ERequesterX = 0,
		ERequesterY = 1,
		ERequesterZ = 2
		};
	SharedMultilevelResource1();
public:	// from MSharedMultilevelResource
	void IncreaseToLevel(TUint aLevel, TInt aRequester);
	void ReduceToLevel(TUint aLevel, TInt aRequester);
	TUint GetResourceLevel();
private:
	void FindMaxLevel(TUint* aLevel, TInt* aId);
private:
	TUint	Levels[KSharers];		// one element per user of resource
	TUint	iCurrentLevel;
	TInt	iCurrentLevelOwnerId;
	};

//
// TO DO: (optional)
//
// The Resource Manager class
//
NONSHARABLE_CLASS(TemplateResourceManager)
	{
public:
	enum TResource			// a list of controllable resources (e.g clocks, voltages, power lines)
		{
		SynchBinResourceUsedByZOnly,
		AsynchBinResourceUsedByZOnly,
		//	... other non-shared binary resources, synchronous or asynchronous
		BinResourceSharedByZAndY,
		//	...	other shared binary resources, synchronous or asynchronous
		SynchMlResourceUsedByXOnly,
		AsynchMlResourceUsedByXOnly,
		//	... other non-shared multilevel resources, synchronous or asynchronous
		MlResourceSharedByXAndW,
		//	...	other shared multilevel resources, synchronous or asynchronous
		};
	enum TSleepModes
		{
		Snooze,
		// ...
		Sleep,
		// ...
		DeepSleep,
		// ...
		Coma
		};

	void InitResources();									// initialises power Resources not initialised by Bootstrap

	inline TSleepModes MapSleepMode(TInt aSleepPeriod);

	// interface for non-shared shared resources

	inline void Modify(TResource aResource, TBool aOnOff);			// for non-shared binary resources
	inline void ModifyToLevel(TResource aResource, TInt aLevel);	// for non-shared multilevel resources
	// the following functions may be used by Drivers/Extensions or the Idle routine to 
	// determine what resources are On or Off or their levels
	inline TBool GetResourceState(TResource aResource);			// for non-shared binary resources
	inline TUint GetResourceLevel(TResource aResource);			// for non-shared multilevel resources 
public:
	// interface for shared resources

	SharedBinaryResource1 iSharedBResource1;
	//	...	other shared Binary resources, synchronous or asynchronous
	inline SharedBinaryResource1* SharedBResource1();

	SharedMultilevelResource1 iSharedMlResource1;
	//	...	other shared Multilevel resources
	inline SharedMultilevelResource1* SharedMlResource1();
	};

#include "template_power.inl"

#endif
