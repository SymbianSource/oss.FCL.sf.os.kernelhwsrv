// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_khal.cpp
// Overview:
// Test class Kern HAL APIs
// API Information:
// Kern::AddHalEntry(), Kern::RemoveHalEntry(), Kern::FindHalEntry()
// Details:
// - Adds a new HAL handler for EHalGroupPlatformSpecific2
// - Tries to add handler for an existing group
// - Calls the installed handler
// - Tests Find API to find the installed handler 
// - Removes the handler and tries to remove some fixed HAL group
// - Tries to find removed handler
// - Tries to call removed handler
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32hal.h>
#include <u32hal.h>
#include <e32svr.h>
#include "d_khal.h"
#include <hal.h>

RLddKHalTest gLdd;
GLDEF_D RTest test(_L("T_KHAL - class Kern HAL API test"));


TBool gEntryForDevice0Registered; // indicator whether the device0 got registered
TBool gEntryForDeviceXRegistered; // indicator whether the deviceX got registered
TInt gTestDeviceNumber;			  // device number that was registered


void TestAddHalHandler()
	{
	TInt r;

	// Try to register PlatformSpecific2 handler for Device 0
	test.Next(_L("Register new HAL handler - device 0"));
	r=gLdd.AddHalEntryDevice0();
	// we dont want to test() the value, since it might actually not be able to register the
	// default device, if one already exists in the system
	if (r==KErrNone)
		{
		gEntryForDevice0Registered=ETrue;
		}
	else
		{
		gEntryForDevice0Registered=EFalse;
		test.Printf(_L("Couldn't register handler for device 0 (r=%d). Some test cases will not be executed"),r);
		}
	// lets try again for device0 if succeeded for the first time, should return error
	if (gEntryForDevice0Registered)
		{
		test.Next(_L("Try to register new HAL handler again for Device0"));
		test_Equal(KErrInUse,gLdd.AddHalEntryDevice0());
		}

	// Try to register PlatformSpecific2 handler for Device X. Trying for multiple devices, so
	// it's highly unlikely that baseport has added this many devices for this group.
	test.Next(_L("Register new HAL handler - device X"));
	
	r=gLdd.AddHalEntryDeviceX();
	if (r==KErrNone)
		{
		gEntryForDeviceXRegistered = ETrue;
		}
	else
		{
		gEntryForDeviceXRegistered = EFalse;
		test.Printf(_L("FAILED to register any device for EHalGroupPlatformSpecific2 (r=%d). Some test cases will not be executed"),r);
		}

    test_Value(r, (r==KErrNone || r==KErrInUse)); // this should not fail, but if it does, print indication

	if (gEntryForDeviceXRegistered)
		{
		gTestDeviceNumber=gLdd.GetRegisteredDeviceNumber();
		test(gTestDeviceNumber != KErrNotFound);
		}

	test.Next(_L("Try to register new HAL handler for fixed group (EHalGroupKernel) - should not be possible"));
	test_Equal(KErrArgument,gLdd.AddHalEntryForExistingFixed());
	}

void TestCallHalHandler()
	{
	test.Next(_L("Call HAL handler function - device 0"));
	if (gEntryForDevice0Registered)
		{
		test_KErrNone(UserSvr::HalFunction(EHalGroupPlatformSpecific2,RLddKHalTest::ETestHalFunc,0,0));
		}
	else
		{
		test.Printf(_L("Didn't try to call handler for device 0, since it wasn't registered in the beginning"));
		}

	test.Next(_L("Call HAL handler function - device X"));
	if (gEntryForDeviceXRegistered)
		{
		test_KErrNone(UserSvr::HalFunction(EHalGroupPlatformSpecific2,RLddKHalTest::ETestHalFunc,0,0,gTestDeviceNumber));
		}
	else
		{
		test.Printf(_L("Didn't try to call handler for device x, since it wasn't registered in the beginning"));
		}
	}

void TestCallRemovedHalHandler()
	{
	TInt r;

	test.Next(_L("Call removed HAL handler function - device 0"));
	if (gEntryForDevice0Registered)
		{
		r=UserSvr::HalFunction(EHalGroupPlatformSpecific2,RLddKHalTest::ETestHalFunc,0,0);
		test_Compare(r, !=, KErrNone);
		}
	else
		{
		test.Printf(_L("Didn't try to call removed handler for device 0, since it wasn't registered in the beginning"));
		}

	test.Next(_L("Call removed HAL handler function - device X"));
	if (gEntryForDeviceXRegistered)
		{
		r=UserSvr::HalFunction(EHalGroupPlatformSpecific2,RLddKHalTest::ETestHalFunc,0,0,gTestDeviceNumber);
		test_Compare(r, !=, KErrNone);
		}
	else
		{
		test.Printf(_L("Didn't try to call removed handler for device x, since it wasn't registered in the beginning"));
		}
	}

void TestFindHalHandler()
	{
	test.Next(_L("Try with one parameter find (device 0) for own handler"));
	if (gEntryForDevice0Registered)
		{
		test_KErrNone(gLdd.FindHalEntryDevice0());
		}
	else
		{
		test.Printf(_L("Didn't try to find handler for device 0, since it wasn't registered in the beginning"));
		}

	test.Next(_L("Try with one parameter find (device 0) for an existing group"));
	test_KErrNone(gLdd.FindHalEntryDevice0Other()); // should find because trying to get EHalGroupKernel, which must exist

	test.Next(_L("Try with two parameter find (device x)"));
	if (gEntryForDeviceXRegistered)
		{
		test_KErrNone(gLdd.FindHalEntryDeviceX()); // should find because handler for device x has been registered
		}
	else
		{
		test.Printf(_L("Didn't try to find handler for device X, since it wasn't registered in the beginning"));
		}
	}

void TestRemoveHalHandler()
	{
	test.Next(_L("Remove HAL handler - device 0"));
	if (gEntryForDevice0Registered)
		{
		test_KErrNone(gLdd.RemoveHalEntryDevice0());
		}
	else
		{
		test.Printf(_L("Didn't try to remove handler for device 0, since it wasn't registered in the beginning"));
		}

	test.Next(_L("Remove HAL handler - device X"));
	if (gEntryForDeviceXRegistered)
		{
		test_KErrNone(gLdd.RemoveHalEntryDeviceX());
		}
	else
		{
		test.Printf(_L("Didn't try to remove handler for device x, since it wasn't registered in the beginning"));
		}

	test.Next(_L("Remove fixed HAL handler (EHalGroupKernel) - should not be possible"));
	test_Equal(KErrArgument,gLdd.RemoveHalEntryExistingFixed());
	}

void TestFindRemovedHalHandler()
	{
	test.Next(_L("Try with one parameter find (device 0) for removed handler"));
	if (gEntryForDevice0Registered)
		{
		test_Equal(KErrNotFound,gLdd.FindHalEntryDevice0());
		}
	else
		{
		test.Printf(_L("didn't try to find removed HAL handler for device 0 since it wasn't registered in the beginning"));
		}

	test.Next(_L("Try with two parameter find (device x) for removed handler"));
	if (gEntryForDeviceXRegistered)
		{
		test_Equal(KErrNotFound,gLdd.FindHalEntryDeviceX());
		}
	else
		{
		test.Printf(_L("didn't try to find removed HAL handler for device X since it wasn't registered in the beginning"));
		}
	}

void LoadDeviceDriver()
	{
	test_KErrNone(User::LoadLogicalDevice(KLddName));
	test_KErrNone(gLdd.Open());
	}

void UnLoadDeviceDriver()
	{
	gLdd.Close();
	test_KErrNone(User::FreeLogicalDevice(KLddName));
	}


GLDEF_C TInt E32Main()
//
// Test Kern HAL API
//
	{
	test.Title();

	test.Start(_L("Test class Kern HAL API functions"));
	// load the driver
	LoadDeviceDriver();

	// add handlers for default device (0) and Device X
	TestAddHalHandler();
	
	// call handlers that were managed to register
	TestCallHalHandler();

	// test find APIs
	TestFindHalHandler();

	// test removal of HAL handlers
	TestRemoveHalHandler();

	// test find APIs for removed handlers
	TestFindRemovedHalHandler();

	// try to call removed handlers
	TestCallRemovedHalHandler();

	// unload the driver
	UnLoadDeviceDriver();

	test.End();
	test.Close();
 	return(KErrNone);
    }
