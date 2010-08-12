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
// e32test\power\t_domain_slave.cpp
// 
//

#include <e32power.h>
#include <e32test.h>
#include <domainmember.h>
#include <domainmanager.h>
#include <e32panic.h>
#include <e32debug.h>

LOCAL_D RTest test(_L(" T_DOMAIN_SLAVE "));

// This will be run in its own thread as part of test #1. It should get killed when trying to connect 
// to the manager without appropriate caps set
TInt IncorrectClient(TAny*)
{
    	RDmDomain domain;
		TInt r = domain.Connect(KDmIdRoot);

		RDmDomainManager manager;
		r = manager.Connect();

        return(r);
}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing"));

//	test.Next(_L("test security"));

	// Get arguments from the command line
	TInt len = User::CommandLineLength();
	test (len);
	TInt size = len * sizeof(TUint16);
	test (size == sizeof(TInt));
	TInt arg;
	TPtr cmd((TUint16*) &arg, len);
	User::CommandLine(cmd);

	TInt expected_result = PlatSec::IsCapabilityEnforced(ECapabilityPowerMgmt) ? KErrPermissionDenied : KErrNone;

	switch(arg)
		{
	case 0:
		{
        // This is the original t_domain_slave test, minus the panicking parts which now get
        // tested as case 1.

        test.Next(_L("test security -- 0"));       

		RDmDomain domain;
		TInt r = domain.Connect(KDmIdRoot);
		test (r == expected_result);

		break;
		}
    case 1:
        {

        test.Next(_L("test security -- 1")); 
        
        TBool jit = User::JustInTime();

        User::SetJustInTime(EFalse);

        _LIT(KPanicThread, "PanicThread");

        RThread testThread;

        TInt tt=testThread.Create(KPanicThread, IncorrectClient, KDefaultStackSize, 
            NULL, NULL);

        test (KErrNone == tt);

        TRequestStatus tStatus;
  //      testThread.Logon(tStatus);

        RUndertaker deathChecker;
        TInt dcOK = deathChecker.Create();

        test (KErrNone == dcOK);

        TInt nextDeadThread;

        deathChecker.Logon(tStatus, nextDeadThread);

        // threads are created in a suspended state. calling resume here starts the thread.
        testThread.Resume();
        User::WaitForRequest(tStatus);

        // If thread suicided for the correct reason --> successful test
        // NB. KErrPermissionDenied means that the server refused the 
        // connection because of incorrect capabilities

        RThread corpse;
        corpse.SetHandle(nextDeadThread);

        RDebug::Printf("Subthread exit type: %d", corpse.ExitType() );

        RDebug::Printf("Subthread exit reason: %d",corpse.ExitReason() );

        test (corpse.ExitType() == EExitKill);

        test (corpse.ExitReason() == KErrPermissionDenied);

        corpse.Close();
  
        // close the RUndertaker and test thread
        deathChecker.Close();
		CLOSE_AND_WAIT(testThread);

        User::SetJustInTime(jit);

		break;
        }
	default:
		User::Panic(_L("USER"), EInvariantFalse);
		break;
		}

	test.End();

	return KErrNone;
	}
