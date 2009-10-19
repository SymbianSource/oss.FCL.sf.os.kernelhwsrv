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
// navienginebsp\ne1_tb\test\interrupts\t_ints.cpp
//
// Overview:
// user-side test for Crazy-Interrupts functionality
//
//
#include <e32test.h>
#include <e32cmn.h>
#include "d_crazyints.h"

_LIT(testName,"t_crazyints");
_LIT(KLddFileName, "d_crazyints.ldd");
_LIT(KLddFileNameRoot, "d_crazyints");

// global data..
GLDEF_D RTest IntsTest(testName);
GLDEF_D RBusIntTestClient IntsTestLdd; // Channel to kernel-side proxy

EXPORT_C TInt E32Main()
	{
	IntsTest.Title();
	IntsTest.Start(_L("Test crazy interrupts"));

	// Load the Test Driver
	TInt r = User::LoadLogicalDevice(KLddFileName);
	if(r != KErrNone && r != KErrAlreadyExists)
		{
		// fail..
		IntsTest.Printf(_L("\nFailed to load the device  driver, r=%d"), r);
		IntsTest(EFalse);
		}

	// Open the driver
	TBufC<11> proxyName(KLddFileNameRoot);
	r = IntsTestLdd.Open(proxyName);
	if(r == KErrNone)
		{
		// run the test..
		r = IntsTestLdd.TestCrazyInts();

		if(r == KErrNotSupported)
			IntsTest.Printf(_L("CrazyInterrupts are not enabled\n\r"));
		}
	else
		{
		IntsTest.Printf(_L("Failed to open the driver (r=%d)"), r);
		if(r == KErrNotSupported)
			{
			IntsTest.Printf(_L(": Not SMP platform..\n"), r);
			}
		else
			{
			IntsTest(r); // fail..
			}
		}

	// Close the driver
	IntsTestLdd.Close();

	// unload the driver
	User::FreeLogicalDevice(KLddFileName);

	// check results..
	IntsTest((r == KErrNone) || (r == KErrNotSupported));

	IntsTest.End();

	return KErrNone;
	}

