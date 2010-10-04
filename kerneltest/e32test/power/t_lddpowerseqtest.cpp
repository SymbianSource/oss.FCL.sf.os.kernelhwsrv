// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_lddpowerseqtest.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>
#include "d_lddpowerseqtest.h"
#include <e32power.h>

_LIT(KLddFileName, "D_LDDPOWERSEQTEST.LDD");

RLddTest1 ldd;
GLDEF_D RTest test(_L("T_LDDPOWERSEQTEST"));

void DoTests()
	{
	TInt r;
	TInt timeInterval = 3;
	//time stamp for power up and powerdown of handlers
	TUint time_power1up = 0, time_power1down = 0;
	TUint time_power2up = 10, time_power2down = 0;
	TRequestStatus statuspowerdown1;
	TRequestStatus statuspowerdown2;
	TRequestStatus statuspowerup1;
	TRequestStatus statuspowerup2;
	RTimer timer;
	TRequestStatus tstatus;

	test.Printf(_L("Loading logical device \n"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r == KErrNone);

	test.Printf(_L("Opening of logical device\n"));
	r = ldd.Open();
	test(r == KErrNone);

	//Send asynchronouse request for power status and time stamp during powerdown.
	ldd.Test_power1down(statuspowerdown1, time_power1down);
	ldd.Test_power2down(statuspowerdown2, time_power2down);
	ldd.Test_power1up(statuspowerup1, time_power1up);
	ldd.Test_power2up(statuspowerup2, time_power2up);
	
	//Set the sleep time
	r = ldd.Test_setSleepTime(timeInterval);
	test(r == KErrNone);
        

	r = timer.CreateLocal();
	test (r == KErrNone);

 	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalMicroSeconds(5000000);
	timer.At(tstatus, wakeup);

	test.Printf(_L("Enable wakeup power events to standby\n"));
	r = Power::EnableWakeupEvents(EPwStandby);
	test (r == KErrNone);

	test.Printf(_L("Powerdown\n"));
	r = Power::PowerDown();
	test (r == KErrNone);

	test.Printf(_L("Waiting for power down request completion of handler1\n"));
	User::WaitForRequest(statuspowerdown1);
	test(statuspowerdown1.Int() == KErrNone);

	test.Printf(_L("Waiting for power down request completion of handler1\n"));
	User::WaitForRequest(statuspowerdown2);
	test(statuspowerdown2.Int() == KErrNone);

	test.Printf(_L("Waiting for power up request completion of handler2\n"));
	User::WaitForRequest(statuspowerup1);
	test(statuspowerup1.Int() == KErrNone);

	test.Printf(_L("Waiting for power up request completion of handler2\n"));
	User::WaitForRequest(statuspowerup2);
	test(statuspowerup2.Int() == KErrNone);

	test.Printf(_L("Waiting for time request completion\n"));;
	User::WaitForRequest(tstatus);
	test(tstatus.Int() == KErrNone);

	test.Printf(_L("Set power manager shut down timeout value\n"));
	ldd.Test_setPowerDownTimeout(10);

	test.Printf(_L("Instruct power handler 2 to act dead\n"));
	ldd.Test_power2ActDead();

	test.Printf(_L("Enable wakeup power events to standby\n"));
	r = Power::EnableWakeupEvents(EPwStandby);
	test (r == KErrNone);

	wakeup.HomeTime();
	wakeup += TTimeIntervalMicroSeconds(5000000);
	timer.At(tstatus, wakeup);

	test.Printf(_L("Powerdown\n"));
	r = Power::PowerDown();
	test (r == KErrNone);

	timer.Close();

	test(time_power2down >= time_power1down + timeInterval);
	test(time_power1up >= time_power2up + timeInterval);
	test.Printf(_L("Verified power up and power down sequence -- OK \n"));

	test.Printf(_L("Closing the channel\n"));
	ldd.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);
	User::After(100000);

	test.End();
	test.Close();
	}

GLDEF_C TInt E32Main()
//
// Test LDD power sequence
//
    {
	test.Start(_L("Test power up and power down sequence"));
	DoTests();
 	return(KErrNone);
    }
