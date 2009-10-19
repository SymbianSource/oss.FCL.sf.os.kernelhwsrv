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
// e32test\power\t_switchoff.cpp
// 
//

#include <e32event.h>
#include <e32event_private.h>
#include <e32svr.h>
#include <e32test.h>

LOCAL_D RTest test(_L(" T_SWITCHOFF "));

void SetAbsoluteTimeout(RTimer& aTimer, TUint aUs, TRequestStatus& aStatus)
	{
	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalMicroSeconds(aUs);
	aTimer.At(aStatus, wakeup);
	}

void SwittchOffTests()
	{
	test.Next(_L("test sending ESwitchOff event"));

	for (int i = 0; i < 4; ++i)
		{
		test.Printf(_L(" %d "), i);
		// Arm an absolute timer wakeup event after 5 sec
		TRequestStatus absstatus;
		RTimer abstimer;
		TInt r = abstimer.CreateLocal();
		test (r == KErrNone);
		SetAbsoluteTimeout(abstimer, 5000000, absstatus); // 5 sec

		// Go to standby through sending a ESwitchOff event (uses domain manager)
		TRawEvent e;
		e.Set(TRawEvent::ESwitchOff);
		r = UserSvr::AddEvent(e);
		test (r == KErrNone);
		User::WaitForRequest(absstatus);
		abstimer.Close();
		}
	test.Printf(_L(" OK\n"));
	}


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing"));

	SwittchOffTests();

	test.End();

	return KErrNone;
	}
