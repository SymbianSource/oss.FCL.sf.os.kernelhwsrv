// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_power.cpp
// Overview:
// Test power down and wakeup event notification
// API Information:
// Power::PowerDown(), Power::RequestWakeupEventNotification()
// Details:
// - Arm a wakeup timer, enable wakeup events, enter standby mode, get woken up.
// - Test RequestWakeupEventNotification(): arm a timer, request notification,
// wait for timer to expire, verify event has been notified, issue another 
// notification request, disable wakeup events, arm another timer, verify 
// wakeup event has not been notified, cancel the notification request and
// close the timer.
// - Verify that a slave process panics because it hasn't and capability.
// - Confirm the number of open handles and pending requests are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32power.h>
#include <e32test.h>
#include <e32kpan.h>
#include <f32file.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

LOCAL_D RTest test(_L(" T_POWER "));

void SetAbsoluteTimeout(RTimer& aTimer, TUint aUs, TRequestStatus& aStatus)
	{
	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalMicroSeconds(aUs);
	aTimer.At(aStatus, wakeup);
	}

void PowerTests()
	{
	test.Next(_L("test PowerDown()"));

	TInt r = Power::PowerDown();
	test (r == KErrNotReady);

	for (int i = 0; i < 4; ++i)
		{
		test.Printf(_L(" %d "), i);
		// Arm an absolute timer wakeup event after 5 sec
		TRequestStatus absstatus;
		RTimer abstimer;
		r = abstimer.CreateLocal();
		test (r == KErrNone);
		SetAbsoluteTimeout(abstimer, 5000000, absstatus); // 5 sec
		// Go to standby 
		r = Power::EnableWakeupEvents(EPwStandby);
		test (r == KErrNone);
		r = Power::PowerDown();
		test (r == KErrNone);
		User::WaitForRequest(absstatus);
		abstimer.Close();
		}
	test.Printf(_L(" OK\n"));

	test.Next(_L("test RequestWakeupEventNotification()"));

		{
		TInt r = Power::EnableWakeupEvents(EPwActive);
		test (r == KErrArgument);

		// Request wakup event notification and enable wakeup events
		TRequestStatus status;
		Power::RequestWakeupEventNotification(status);
		test(status.Int() == KRequestPending);
		r = Power::EnableWakeupEvents(EPwStandby);
		test (r == KErrNone);
		// Arm an absolute timer wakeup event
		TRequestStatus absstatus;
		RTimer abstimer;
		r = abstimer.CreateLocal();
		test (r == KErrNone);
		SetAbsoluteTimeout(abstimer, 100000, absstatus); // 100ms
		// Wait for the timer
		User::WaitForRequest(absstatus);
		test(absstatus.Int() == KErrNone);
		// Wakup event has to be already notified
		test(status.Int() == KErrNone);
		User::WaitForRequest(status);	// collect it
		// Issue another notification request
		Power::RequestWakeupEventNotification(status);
		test(status.Int() == KRequestPending);
		// Disable wakeup events
		Power::DisableWakeupEvents();
		// Arm another absolute timer wakeup event
		SetAbsoluteTimeout(abstimer, 100000, absstatus); // 100ms
		// Wait for the timer
		User::WaitForRequest(absstatus);
		test(absstatus.Int() == KErrNone);
		// Wakeup event has not to be notified
		test(status.Int() == KRequestPending);
		// Cancel the notification request
		Power::CancelWakeupEventNotification();
		test(status.Int() == KErrCancel);
		User::WaitForRequest(status);	// collect it
		// Cancel again just for fun ...
		Power::CancelWakeupEventNotification();
		test(status.Int() == KErrCancel);

		abstimer.Close();
		}
	}

_LIT(KSecuritySlavePath, "t_power_slave.exe");

void ExecSlave(TUint aArg)
	{
	RProcess proc;
	TInt r = proc.Create(KSecuritySlavePath, TPtrC((TUint16*) &aArg, sizeof(aArg)/sizeof(TUint16)));
	test(r == KErrNone);
	TRequestStatus status;
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	// The slave must panic
	test_Equal(EExitPanic, proc.ExitType());
	test_Equal(EPlatformSecurityTrap, proc.ExitReason());
	CLOSE_AND_WAIT(proc);
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing"));

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	//
	// Perform the number of iterations specifed by the command line argument.
	//
	// If no arguments - perform two iterations
	//
	TInt iter = 2;
	TInt len = User::CommandLineLength();
	if (len)
		{
		// Copy the command line in a buffer
		HBufC* hb = HBufC::NewMax(len);
		test(hb != NULL);
		TPtr cmd((TUint16*) hb->Ptr(), len);
		User::CommandLine(cmd);
		// Extract the number of iterations
		TLex l(cmd);
		TInt i;
		TInt r = l.Val(i);
		if (r == KErrNone)
			iter = i;
		else
			// strange command - silently ignore
			{} 
		delete hb;
		}

	test.Printf(_L("Go for %d iterations\n"), iter);

	while (iter--)
		{
		// Remember the number of open handles. Just for a sanity check ....
		TInt start_thc, start_phc;
		RThread().HandleCount(start_phc, start_thc);

		PowerTests();

		test.Start(_L("test platform security"));
		// The slave process must panic because it hasn't any capability 
		if(!PlatSec::IsCapabilityEnforced(ECapabilityPowerMgmt))
			test.Printf(_L("TESTS NOT RUN - PowerMgmt capability isn't enforced on system"));
		else
			{
			test.Next(_L("PowerDown()"));
			ExecSlave(0);
			test.Next(_L("EnableWakeupEvents()"));
			ExecSlave(1);
			test.Next(_L("DisableWakeupEvents()"));
			ExecSlave(2);
			test.Next(_L("RequestWakeupEventNotification()"));
			ExecSlave(3);
			test.Next(_L("CancelWakeupEventNotification()"));
			ExecSlave(4);
			}
		test.End();

		// Sanity check for open handles
		TInt end_thc, end_phc;
		RThread().HandleCount(end_phc, end_thc);
		test(start_thc == end_thc);
		test(start_phc == end_phc);
			// and also for pending requests ...
		test(RThread().RequestCount() == 0);
		}

	test.End();

	return KErrNone;
	}
