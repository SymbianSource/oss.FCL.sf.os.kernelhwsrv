// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test suite to check whether the device has good enough entropy sources or not.
// More the no. of sources, high the quality of the random numbers (least predictable).
// 
//

/**
 @file
 @internalTechnology
*/

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <hal.h>
#include <e32math.h>
#include "d_entropysources.h"

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-entropysources-2703
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifies that entropy is contributed to the Secure RNG
//! @SYMPREQ					PREQ211
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	TestReseed: tests that the interval between RNG reseeds is less than KMaxReseedTime, unless the platform is 
//!         known not to have a viable entropy source.
//! 		
//! 
//! @SYMTestExpectedResults
//! 	1.	Properties checked:
//! 		1) checks that there is a valid entropy source contrbuting entropy data.
//!         2) checks that the entropy collection framework is functioning correctly..
//! 	
//---------------------------------------------------------------------------------------------------------------------

// In a worst case scenario, we expect the reseed to happen within ten minutes.
// This is approximately 1/10 of the actual reseed intervel time (97.8 rounded to => 98).
// ((2^24 (reseed intervel) * 350 (micro seconds per request)) / 1000000) / 60 => 97.8 minutes.
//
const TTimeIntervalMicroSeconds32 KMaxReseedTime = 10 * 60 * 1000000;

REntropySources Ldd;

LOCAL_D RTest test(_L("T_ENTROPYSOURCES"));

LOCAL_C void TestReseed()
    {    
    test.Next(_L("Loading test driver"));
    TInt r = User::LoadLogicalDevice(KEntropySourcesName);
    test(r==KErrNone || r==KErrAlreadyExists);
    
    test.Next(_L("Opening the logical channel"));
    test_KErrNone(Ldd.Open());
    
	RTimer timer;
	test_KErrNone(timer.CreateLocal());

	test.Next(_L("Testing reseed interval"));
	// Wait for two reseeds, so we can prove the interval between them is ok
	for (TInt i=0; i<2; ++i)
		{
		// Request notification of the next reseed, with a timeout
		test.Next(_L("Requesting reseed notification"));
		TRequestStatus reseed, timeout;
		Ldd.ReseedTest(reseed);
		timer.After(timeout, KMaxReseedTime);

		// Prod the RNG to make sure it's not idle, then wait for an event
		test.Next(_L("Prod RNG and wait"));
		Math::Random();
		User::WaitForRequest(reseed, timeout);

		// Check we didn't time out and cancel the timer
		test_Equal(KRequestPending, timeout.Int());
		timer.Cancel();
		User::WaitForRequest(timeout);
		test_Equal(KErrCancel, timeout.Int());
		test_KErrNone(reseed.Int());
		}
	
	Ldd.Close();

	User::FreeLogicalDevice(KEntropySourcesName);
    }

LOCAL_C TBool HardwareRNGPresent()
    {
    TInt muid = 0;
    const TInt r = HAL::Get(HAL::EMachineUid, muid);
    if (r != KErrNone) return EFalse;;
    return ((muid != HAL::EMachineUid_X86PC) &&
            (muid != HAL::EMachineUid_NE1_TB) &&
            (muid != HAL::EMachineUid_OmapH6) &&
            (muid != HAL::EMachineUid_OmapZoom) &&
            (muid != HAL::EMachineUid_Win32Emulator));
    }

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test periodic entropy source"));

	if(HardwareRNGPresent())	
	    {
	    __UHEAP_MARK;
        
		TestReseed();

        __UHEAP_MARKEND;	    
	    }
	else
	    {
	    test.Printf(_L("Test skipped, platform is known not to have a periodic entropy source\n"));
	    }
	test.End();
	test.Close();
	
	return 0;
	}
