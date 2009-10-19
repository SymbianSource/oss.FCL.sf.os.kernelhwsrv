// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_tldd.cpp
// Overview:
// Test device driver loading and unloading and
// the RBusLogicalChannel hook functionality.
// API Information:
// RBusLogicalChannel, DLogicalChannel, Kern::InstallLogicalDevice.
// Details:
// - Test loading the test logical device driver "D_LDD" and creating a logical
// channel.
// - Test initialisation of LDD data and bss sections
// - Test passing data to driver and back, and storing it in data/bss/heap
// areas
// - Test contructors were called for driver global data
// - Test functionality of Kern::InstallLogicalDevice by asking the test LDD
// to create a second device. Check this device works, and check it goes
// away when freed.
// - Check the return value as KErrInUse when the LDD close method is called 
// before closing the handle.
// - Close the handle and unload the device, verify success.
// - Check that no memory is leaked on the kernel heap
// - Load the logical device driver again and verify that the global variables
// and bss have been reinitialized.
// - Close the handle and unload the device, verify success.
// - Test User::LoadLogicalDevice and User::FreeLogicalDevice APIs, including
// free whilst in use and load whilst already loaded
// - On hardware, repeat all tests with the "D_LDD_RAM" device driver.
// - Check the return value is KErrNotSupported when trying to load invalid 
// logical device driver.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_ldd.h"

_LIT(KLddFileName, "D_LDD.LDD");
_LIT(KLddFileNameBadUid, "D_LDDNS.LDD");

#ifdef __EPOC32__
_LIT(KLddFileNameRam, "D_LDD_RAM.LDD");
#endif

GLDEF_D RLddTest ldd;
GLDEF_D RTest test(_L("LDD tests"));

void DoTest(const TDesC& aLddFileName)
	{
	TInt r;
    test.Start(_L("Loading LDD"));
#ifdef __EPOC32__
	// Need to load/unload before doing __KHEAP_MARK to ensure kernel's CodeChunk has been created
	r=User::LoadLogicalDevice(aLddFileName);
	test.Printf(_L("Returned %d\n"), r);
    test(r==KErrNone);
    test.Next(_L("Unloading LDD"));
	r = RLddTest::UnloadAndWait();
	test.Printf(_L("Returned %d\n"), r);
	test(r==KErrNone);
    test.Next(_L("Loading LDD"));
#endif
	__KHEAP_MARK;
	r=User::LoadLogicalDevice(aLddFileName);
	test.Printf(_L("Returned %d\n"), r);
    test(r==KErrNone);
    test.Next(_L("Opening device driver"));
	test(ldd.Open()==KErrNone);
	test.Next(_L("Test LDD global static function pointer"));
	r=ldd.Test1();
	test.Printf(_L("%x\n"), r);
	test.Next(_L("Test LDD global static data"));
	r=ldd.Test2();
	test.Printf(_L("%x\n"), r);
	test(r==0x100);
	r=ldd.Test2();
	test.Printf(_L("%x\n"), r);
	test(r==0x101);
	r=ldd.Test2();
	test.Printf(_L("%x\n"), r);
	test(r==0x102);
	r=ldd.Test3();
	test.Printf(_L("%x\n"), r);
	test(r==0x103);
	r=ldd.Test3();
	test.Printf(_L("%x\n"), r);
	test(r==0x102);
	test.Next(_L("Test LDD .bss"));
	r=ldd.Test5();
	test.Printf(_L("%x\n"), r);
	test(r==0);
	r=ldd.Test6(299792458);
	test.Printf(_L("%x\n"), r);
	test(r==KErrNone);
	r=ldd.Test5();
	test.Printf(_L("%x\n"), r);
	test(r==299792458);
	test.Next(_L("Test global constructors"));
	TUint32 v = ldd.Test7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==487);
	r = ldd.Test9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);
	ldd.Test8(488);
	v = ldd.Test7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==488);
	r = ldd.Test9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);

	test.Next(_L("Test linked global static function pointer"));
	r=ldd.LinkedTest1();
	test.Printf(_L("%x\n"), r);
	test.Next(_L("Test linked global static data"));
	r=ldd.LinkedTest2();
	test.Printf(_L("%x\n"), r);
	test(r==0x100);
	r=ldd.LinkedTest2();
	test.Printf(_L("%x\n"), r);
	test(r==0x101);
	r=ldd.LinkedTest2();
	test.Printf(_L("%x\n"), r);
	test(r==0x102);
	r=ldd.LinkedTest3();
	test.Printf(_L("%x\n"), r);
	test(r==0x103);
	r=ldd.LinkedTest3();
	test.Printf(_L("%x\n"), r);
	test(r==0x102);
	test.Next(_L("Test linked .bss"));
	r=ldd.LinkedTest5();
	test.Printf(_L("%x\n"), r);
	test(r==0);
	r=ldd.LinkedTest6(299792458);
	test.Printf(_L("%x\n"), r);
	test(r==KErrNone);
	r=ldd.LinkedTest5();
	test.Printf(_L("%x\n"), r);
	test(r==299792458);
	test.Next(_L("Test linked global constructors"));
	v = ldd.LinkedTest7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==487);
	r = ldd.LinkedTest9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);
	ldd.LinkedTest8(488);
	v = ldd.LinkedTest7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==488);
	r = ldd.LinkedTest9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);


	test.Next(_L("Test behaviour of Kern::InstallLogicalDevice"));
	RKInstallTest ki;
	r = ki.Open();
	test(r==KErrNotFound);
	r = ldd.TestKInstall();
	test(r==KErrNone);
	r = ki.Open();
	test(r==KErrNone);
	ki.Close();
	r = User::FreeLogicalDevice(KKInstallLddName);
	test(r==KErrNone);
	r = ki.Open();
	test(r==KErrNotFound);
	
	test.Next(_L("Try to unload with open channel"));
	TInt i;
	for (i=0; i<10; ++i)
		{
		r = RLddTest::UnloadAndWait();
		test.Printf(_L("Returned %d\n"), r);
		test(r==KErrInUse);
		}
	test.Next(_L("Close"));
	ldd.Close();
	test.Next(_L("Unload"));
	r = RLddTest::UnloadAndWait();
	test.Printf(_L("Returned %d\n"), r);
	test(r==KErrNone);
	__KHEAP_MARKEND;
	test.Next(_L("Reload"));
	r=User::LoadLogicalDevice(aLddFileName);
	test.Printf(_L("Returned %d\n"), r);
    test(r==KErrNone);
	test.Next(_L("Open again"));
	test(ldd.Open()==KErrNone);

	test.Next(_L("Test data re-initialised"));
	r=ldd.Test4();
	test.Printf(_L("%x\n"), r);
	test(r==0x100);
	test.Next(_L("Test .bss reinitialised"));
	r=ldd.Test5();
	test.Printf(_L("%x\n"), r);
	test(r==0);
	v = ldd.Test7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==487);
	r = ldd.Test9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);
	
	test.Next(_L("Test linked data re-initialised"));
	r=ldd.LinkedTest4();
	test.Printf(_L("%x\n"), r);
	test(r==0x100);
	test.Next(_L("Test linked .bss reinitialised"));
	r=ldd.LinkedTest5();
	test.Printf(_L("%x\n"), r);
	test(r==0);
	v = ldd.LinkedTest7();
	test.Printf(_L("iInt = %u\n"),v);
	test(v==487);
	r = ldd.LinkedTest9();
	test.Printf(_L("Verify returned %d\n"), r);
	test(r==KErrNone);

	ldd.Close();

	test.Next(_L("Unload"));
	r = RLddTest::Unload();
	test(r==KErrNone);
	r=User::LoadLogicalDevice(aLddFileName);
    test(r==KErrNone);
	r = RLddTest::UnloadAndWait();
	test.Printf(_L("Returned %d\n"), r);
	test(r==KErrNone);

	// Tests for User::LoadLogicalDevice and User::FreeLogicalDevice
	RDevice device;
	test.Next(_L("Load Device"));
	r=User::LoadLogicalDevice(aLddFileName);
    test(r==KErrNone);
	test.Next(_L("Open Device"));
	r=device.Open(KLddName);
    test(r==KErrNone);
	test.Next(_L("Unload whilst in use"));
	r = User::FreeLogicalDevice(KLddName);;
	test(r==KErrInUse);
	test.Next(_L("Close Device"));
	device.Close();
	test.Next(_L("Unload"));
	r = User::FreeLogicalDevice(KLddName);;
	test(r==KErrNone);

	test.Next(_L("Load Device"));
	r=User::LoadLogicalDevice(aLddFileName);
    test(r==KErrNone);
	test.Next(_L("Load Device again"));
	r=User::LoadLogicalDevice(aLddFileName);
    test(r==KErrAlreadyExists);
	test.Next(_L("Unload"));
	r = User::FreeLogicalDevice(KLddName);;
	test(r==KErrNone);
	
	test.End();
	}


void DoTest2(const TDesC& aLddFileName)
	{
    test.Start(_L("Test repeated loading/unloading of RAM-based LDD"));

	// This tests repeated loading and unloading works, and that it always
	// re-loads the LDD code to the same address
	
	TInt func = 0;
	for (TInt i = 0 ; i < 100 ; ++i)
		{
		test(User::LoadLogicalDevice(aLddFileName)==KErrNone);
		test(ldd.Open()==KErrNone);

		if (i == 0)
			func=ldd.Test1();
		else	
			test(func==ldd.Test1());
		
		ldd.Close();
		test(User::FreeLogicalDevice(KLddName)==KErrNone);
		}
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test LDD static data
//
    {
	TInt r;
	test.Title();
	test.Start(_L("Test device driver loading and unloading"));

	DoTest(KLddFileName);
#ifdef __EPOC32__
	DoTest(KLddFileNameRam);
	DoTest2(KLddFileNameRam);
#endif

	r=User::LoadLogicalDevice(KLddFileNameBadUid);
	test(r==KErrNotSupported);

    test.End();
	return(KErrNone);
    }

