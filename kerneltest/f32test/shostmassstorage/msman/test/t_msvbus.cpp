/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#include <e32def.h>
#include <e32cmn.h>
#include <e32base.h>
#include <f32file.h>
#include <e32cons.h>
#include <e32debug.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>


#include "rusbhostsession.h"
#include "rusbotgsession.h"

#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "ttestutils.h"
#include "tmslog.h"

RTest test(_L("T_MSVBUS"));
RFs fsSession;
RUsbOtgSession usbOtgSession;

extern CMsDrive* msDrive;


void Test1L()
    {
    test.Start(_L("Test1"));
    test.Next(_L("Test1"));

    usbOtgSession.BusDrop();
    usbOtgSession.DeviceInserted();

    TTestTimer iTimer;
    // #1
    // wait for suspend

    iTimer.Start();
    TBool usbActive = TTestUtils::WaitForConnectionStateEventL();
    test(usbActive == EFalse);
    iTimer.End();

    // #2
    // wait for suspend
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);

    // #3
    // wait for suspend
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);

    // #4
    // wait for suspend
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);
    test.End();
    }



void CallTestsL()
    {
    CleanupClosePushL(fsSession);
    User::LeaveIfError(fsSession.Connect());

    TTestUtils::WaitForBusEventL();

    Test1L();

    CleanupStack::PopAndDestroy();
    }


GLDEF_C void MainL()
    {
    __MSFNSLOG

    TInt err;

    RUsbHostSession usbHostSession;
    err = usbHostSession.Connect();
    User::LeaveIfError(err);
    CleanupClosePushL(usbHostSession);
    test.Printf(_L("USB Host Server Connected\n"));

    err = usbHostSession.Start();
    User::LeaveIfError(err);
    test.Printf(_L("USB Host Server Started\n"));

    err = usbOtgSession.Connect();
    User::LeaveIfError(err);
    CleanupClosePushL(usbOtgSession);
    test.Printf(_L("USB OTG Server Connected\n"));

    TBool deviceInserted;
    deviceInserted = usbOtgSession.DeviceInserted();
    RDebug::Printf(">>>>>>>>>>>> %d", deviceInserted);

    CallTestsL();

    test.Printf(_L("Destroying sessions...\n"));
    test.Printf(_L("Destroying USB OTG session...\n"));
    CleanupStack::PopAndDestroy(&usbOtgSession);

    test.Printf(_L("Destroying USB Host session...\n"));
    CleanupStack::PopAndDestroy(&usbHostSession);
    test.Printf(_L("Sessions Destroyed.\n"));
    }


GLDEF_C TInt E32Main()
	{
    CTrapCleanup* cleanup = CTrapCleanup::New();
	__UHEAP_MARK;
    test.Title();
    test.Start(_L("Starting tests..."));

	TTime timerC;
	timerC.HomeTime();

	TRAPD(err, MainL());

    if (err != KErrNone)
       {
       RDebug::Print(_L("MainL error: %d\n"),err);
       }

    TTime endTimeC;
    endTimeC.HomeTime();
    TTimeIntervalSeconds timeTakenC;
    err = endTimeC.SecondsFrom(timerC,timeTakenC);
    test(err == KErrNone);
    test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());

	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return KErrNone;
	}
