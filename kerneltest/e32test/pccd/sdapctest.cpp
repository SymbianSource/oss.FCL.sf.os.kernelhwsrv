// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL " http://www.eclipse.org/legal/epl-v10.html ".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:


#include <drivers/d_sdapc.h>

#define __E32TEST_EXTENSION__

#include <e32std.h>
#include <e32test.h>
#include <hal.h>

LOCAL_D RTest test(_L("SDAPCTEST"));


// This test is intended to simply load and free the associated LDD,
// since the required funcionality is contained in the channel creation method (client registration, PSU locking)
// and destructor (deregistration and unlocking).
// 
// The waits are intended to allow a generous sample time for logging of KTRACE from the PSU code
// with the driver loaded/unloaded, to verify that it is behaving as expected.

GLDEF_C TInt E32Main()
	{
#if !defined(__WINS__)
	test.Title();
		
    RSDAuxiliaryPowerControlAPI TheDriver;

	test.Start(_L("SDAPCTEST - Main Test"));

	// Only test on platforms with SDIO support in all ROM configurations
	TInt machineuid;
    HAL::Get(HAL::EMachineUid, machineuid);
    if(machineuid != HAL::EMachineUid_OmapH2)
	    {
        test.Printf(_L("Test not supported on this platform\n"));
        }
	else
		{
		TInt err = KErrGeneral;

	    err = User::LoadLogicalDevice(_L("D_SDAPC"));
		test.Printf(_L("Value of err is %d\n"), err);
		test_Value(err, err==KErrNone || err==KErrAlreadyExists);
    	        
		err = TheDriver.Open(0,TheDriver.VersionRequired());
		test_KErrNone(err);

		test.Printf(_L("Wait for 10 seconds with SD auxiliary power-control driver loaded...\n"));
		User::After(10000000);
    	        
		TheDriver.Close();
        		    
		err = User::FreeLogicalDevice(_L("D_SDAPC"));
		test.Printf(_L("Value of err is %d\n"), err);

		test.Printf(_L("Wait for 10 seconds without SD auxiliary power-control driver loaded...\n"));

		User::After(10000000);
		}
		
	test.End();
#else
	test.Printf(_L("This test does not run on emulator.\n"));
#endif
	return(KErrNone);
	}


