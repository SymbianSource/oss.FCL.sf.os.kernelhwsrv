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
// USB Mass Storage Application - also used as an improvised boot loader mechanism
// 
//



/**
 @file
*/

#include <e32test.h>
#include <f32file.h>
#include <d32usbdi_hubdriver.h>

#include "rusbhostmsdevice.h"
#include "rextfilesystem.h"
#include "cusbmsmountmanager.h"
#include "cusbhost.h"
#include "appdebug.h"


_LIT(KTxtApp,"USBHOSTMSAPP");

LOCAL_D RTest test(_L("MSAPP HOST TEST"));


GLDEF_C void RunAppL()
    {
	test.Title();
	test.Start(_L("Mass Storage Host Tests"));

    CUsbHost* host = CUsbHost::NewL();

    host->OpenHubL();

	test.Printf(_L("Build up bus events. Press any key to start consuming"));
	test.Getch();

    TInt deviceCount = 0;
    for (;;)
        {
        test.Next(_L("Wait for device attach"));
        RUsbHubDriver::TBusEvent::TEvent event = host->WaitForBusEvent();

        if (event == RUsbHubDriver::TBusEvent::EDeviceAttached)
            {
            /* Jungo stack has attached the device */
            TUint32 token = host->OpenDeviceL();
            host->MountDeviceL();
            deviceCount++;
            __PRINT1(_L("%d device(s) attached"), deviceCount);
            }
        else if (event == RUsbHubDriver::TBusEvent::EDeviceRemoved)
            {
            host->DismountDeviceL();
            host->CloseDeviceL();

            __PRINT1(_L("%d device(s) attached"), deviceCount);

            if (--deviceCount == 0)
                {
                break;
                }
            }

        else
            {
            // nothing to do
            }
        }


    test.Printf(_L("Press a key to dismount\n"));

    test.Getch();

    host->DismountAllFileSystemsL();
    host->CloseAllDevicesL();
    host->CloseHubL();

    delete host;

	// 1 sec delay for sessions to stop
	User::After(1000000);

    test.End();
    test.Close();
    }



GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
    test(cleanup != NULL);
	TRAPD(error, RunAppL());
	__ASSERT_ALWAYS(!error, User::Panic(KTxtApp, error));
	delete cleanup;
	__UHEAP_MARKEND;
	return 0;
	}
