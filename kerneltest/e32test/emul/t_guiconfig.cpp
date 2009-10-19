// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_guiconfig.cpp
// Overview:
// Test the kernel-side emulator GUI configuration API.
// API Information:
// RBusLogicalChannel, DLogicalChannel, WinsGui::CurrentConfiguration.
// Details:
// - Test retrieving the current emulator GUI configuration.
// The following emulator configuration (.ini) files can be used with
// this test to setup multiple configurations. Switch from one
// configuration to the next by clicking in the upper left corner of
// the emulator window, then run the test with a different configuration
// selected.
// >>> \epoc32\data\epoc.ini <<<
// configuration c0.ini
// configuration c1.ini
// configuration c2.ini
// configuration c3.ini
// >>> \epoc32\data\c0.ini <<<
// OnActivation 0 EKeyScreenDimension0
// EmulatorControl NextConfig rect 1,1 30,30
// windowtitle ZERO
// >>> \epoc32\data\c1.ini <<<
// OnActivation 0 EKeyScreenDimension1
// EmulatorControl NextConfig rect 1,1 30,30
// windowtitle ONE
// >>> \epoc32\data\c2.ini <<<
// OnActivation 0 EKeyScreenDimension2
// EmulatorControl NextConfig rect 1,1 30,30
// windowtitle TWO
// >>> \epoc32\data\c3.ini <<<
// OnActivation 0 EKeyScreenDimension3
// EmulatorControl NextConfig rect 1,1 30,30
// windowtitle THREE
// Platforms/Drives/Compatibility:
// Only available on the emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_guiconfig.h"

_LIT(KLddFileName, "d_guiconfig.ldd");

GLDEF_D RGuiConfigTest ldd;
GLDEF_D RTest test(_L("Emulator GUI configuration"));

void DoTest(const TDesC& aLddFileName)
	{
	TInt r;
    test.Start(_L("Loading LDD"));

	__KHEAP_MARK;

	r=User::LoadLogicalDevice(aLddFileName);
	test.Printf(_L("Returned %d\n"), r);
    test(r==KErrNone);

	test.Next(_L("Opening device driver"));
	test(ldd.Open()==KErrNone);

	test.Next(_L("Test retrieving current GUI configuration"));
	test.Printf(_L("Current emulator configuration is: %d"), ldd.GetConfig());

	test.Next(_L("Test generating key event"));
	test(ldd.GenerateKeyEvent()==KErrNone);

	TRequestStatus status;
	test.Console()->Read(status);
	User::WaitForRequest(status);

	if (test.Console()->KeyCode() == EKeyScreenDimension0)
		{
		test.Printf(_L("EKeyScreenDimension0 (0x%04x)\r\n"), test.Console()->KeyCode());
		// wait for the key-up code
		test.Console()->Read(status);
		User::WaitForRequest(status);

		test(test.Console()->KeyCode() == EKeyScreenDimension0);
		}
	else if (test.Console()->KeyCode()==EKeyScreenDimension1)
		{
		test.Printf(_L("EKeyScreenDimension1 (0x%04x)\r\n"), test.Console()->KeyCode());
		// wait for the key-up code
		test.Console()->Read(status);
		User::WaitForRequest(status);

		test(test.Console()->KeyCode() == EKeyScreenDimension1);
		}
	else if (test.Console()->KeyCode() == EKeyScreenDimension2)
		{
		test.Printf(_L("EKeyScreenDimension2 (0x%04x)\r\n"), test.Console()->KeyCode());
		// wait for the key-up code
		test.Console()->Read(status);
		User::WaitForRequest(status);

		test(test.Console()->KeyCode() == EKeyScreenDimension2);
		}
	else if (test.Console()->KeyCode() == EKeyScreenDimension3)
		{
		test.Printf(_L("EKeyScreenDimension3 (0x%04x)\r\n"), test.Console()->KeyCode());
		// wait for the key-up code
		test.Console()->Read(status);
		User::WaitForRequest(status);

		test(test.Console()->KeyCode() == EKeyScreenDimension3);
		}
	else
		{
		test.Printf(_L("Key: %5d (0x%04x)\r\n"),test.Console()->KeyCode(),test.Console()->KeyCode());
		test(EFalse);
		}

	test.Next(_L("Close"));
	ldd.Close();

	test.Next(_L("Unload"));
	r = RGuiConfigTest::Unload();
	test(r==KErrNone);

	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test LDD static data
//
    {
	test.Title();
	test.Start(_L("Test emulator GUI configuration API"));

	DoTest(KLddFileName);

	test.End();
	return(KErrNone);
    }

