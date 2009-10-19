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
// e32test\nkern\t_implicit.cpp
// Overview:
// Test nanokernel implicit system lock
// API Information:
// NKern::LockSystem and NKern::UnlockSystem
// Details:
// - Load and open the logical device driver ("D_IMPLICIT"). Verify results.
// - The LDD uses three separate threads to test the nanokernel implicit 
// system lock with changing priorities, round-robin and a "dry run".
// - Close the channels and verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "d_implicit.h"
#include <u32hal.h>
#include <e32svr.h>

RTest test(_L("T_IMPLICIT"));

void Wait(TInt aSeconds)
	{
	while (aSeconds>0)
		{
		--aSeconds;
		User::After(1000000);
		test.Printf(_L("."));
		}
	}

TInt Display(const SStats& a)
	{
	test.Printf(_L("\nc1=%6d c2=%6d c3=%6d f=%6d\n"),a.iCount1,a.iCount2,a.iCount3,a.iFailCount);
	return a.iFailCount;
	}

GLDEF_C TInt E32Main()
	{

	test.Title();

	if (UserSvr::HalFunction(EHalGroupKernel, EKernelHalSmpSupported, 0, 0) == KErrNone)
		{
		test.Printf(_L("*********************************\n"));
		test.Printf(_L("*  NOT SUPPORTED ON SMP SYSTEMS *\n"));
		test.Printf(_L("*********************************\n"));
		User::After(2000000);
		return(0);
		}

	test.Start(_L("Load LDD"));
	TInt r=User::LoadLogicalDevice(_L("D_IMPLICIT"));
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Open channel"));
	RImpSysTest impSys;
	r=impSys.Open();
	test(r==KErrNone);

	SStats s;
	test.Next(_L("Test with changing priorities"));
	r=impSys.Start(RImpSysTest::ETestPriority);
	test(r==KErrNone);
	Wait(30);
	r=impSys.Stop(s);
	test(r==KErrNone);
	TInt f1=Display(s);

	test.Next(_L("Test with round-robin"));
	r=impSys.Start(RImpSysTest::ETestRoundRobin);
	test(r==KErrNone);
	Wait(30);
	r=impSys.Stop(s);
	test(r==KErrNone);
	TInt f2=Display(s);

	test.Next(_L("Dry run"));
	r=impSys.Start(RImpSysTest::ETestDummy);
	test(r==KErrNone);
	Wait(30);
	r=impSys.Stop(s);
	test(r==KErrNone);
	TInt f3=Display(s);

	test.Next(_L("Close channel"));
	impSys.Close();

	test(f1==0);
	test(f2==0);
	test(f3==0);

	test.End();
	return KErrNone;
	}
