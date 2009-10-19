// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_msmain.cpp
//
//

#include <e32def.h>
#include <e32cmn.h>
#include <e32base.h>
#include <f32file.h>
#include <e32debug.h>
#include <e32test.h>

#include "rfsydebug.h"
#include "rusbhostsession.h"
#include "rusbotgsession.h"
#include "cmsdrive.h"
#include "tmslog.h"

/*
OTG Platform configuration 
Define for H4 and undefine for Navi Engine 
*/
#define _OTG_CONFIG 

extern RTest test;
extern RFs fsSession;

CMsDrive* msDrive;

extern void CallTestsL();


GLDEF_C void MainL()
    {
    __MSFNSLOG

    TInt err;
    CleanupClosePushL(fsSession);
    User::LeaveIfError(fsSession.Connect());

    msDrive = CMsDrive::NewL();
    CleanupStack::PushL(msDrive);

    RUsbOtgSession usbOtgSession;
    err = usbOtgSession.Connect();
    User::LeaveIfError(err);
    CleanupClosePushL(usbOtgSession);
    test.Printf(_L("USB OTG Server Connected\n"));


#ifdef _OTG_CONFIG
    TBool deviceInserted = usbOtgSession.DeviceInserted();
    test.Printf(_L("hub inserted = %d\n"), deviceInserted);

    while (!deviceInserted)
        {
        TBool change;
        TRequestStatus status;
        test.Printf(_L("Waiting for hub inserted event...\n"));
        usbOtgSession.NotifyChange(change, status);
    
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());

        TBool deviceInserted = usbOtgSession.DeviceInserted();
        test.Printf(_L("hub inserted = %d\n"), deviceInserted);
        }
#else
    TBool deviceInserted = ETrue;
#endif




    RUsbHostSession usbHostSession;
    err = usbHostSession.Connect();
    User::LeaveIfError(err);
    CleanupClosePushL(usbHostSession);
    test.Printf(_L("USB Host Server Connected\n"));

    err = usbHostSession.Start();
    User::LeaveIfError(err);
    test.Printf(_L("USB Host Server Started\n"));


    if (deviceInserted)
        {
        TBool driveFound = EFalse;

        do
            {
            TRequestStatus status;
            fsSession.NotifyChange(ENotifyAll, status);

            // trigger fileserver to update drive lists
            TDriveList driveList;
            User::LeaveIfError(fsSession.DriveList(driveList));

            test.Printf(_L("Waiting for File System change notifcation...\r\n"));
            User::WaitForRequest(status);
            test.Printf(_L("NotifyChange status=%d\r\n"), status.Int());

            driveFound = msDrive->GetUsbDeviceL();
            } while (!driveFound);

        msDrive->SetSessionPathL();

        if (!msDrive->DrivePresent())
            {
            test.Printf(_L("Media Not present !\n"));
            User::Leave(KErrNotReady);
            }

        test.Printf(_L("#--> ==== All mount enhancements are disabled ====\n"));
        RFsyDebug fsyDebug(msDrive->DriveNumber());
        fsyDebug.DisableAll();

        CallTestsL();
        }
    else
        {
        test.Printf(_L("Hub not inserted!\n"));
        test(EFalse);
        }

    
    test.Printf(_L("Destroying sessions...\n"));
    test.Printf(_L("Destroying USB Host session...\n"));
    CleanupStack::PopAndDestroy(&usbHostSession);
    // Wait for thread to die
    User::After(1000 * 1000 * 4);

    test.Printf(_L("Destroying USB OTG session...\n"));
    CleanupStack::PopAndDestroy(&usbOtgSession);
    test.Printf(_L("Sessions Destroyed.\n"));
    // Wait for thread to die
    User::After(1000 * 1000 * 4);

    CleanupStack::PopAndDestroy(msDrive);
    CleanupStack::PopAndDestroy();  // fsSession
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
    test(err == KErrNone);

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

