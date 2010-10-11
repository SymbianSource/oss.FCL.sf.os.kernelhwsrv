
// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/rpmb/t_rpmb.cpp
// LDD for testing RPMB kernel extension
// 
//

/**
 @file
 @internalComponent
 @prototype
*/

#define	__E32TEST_EXTENSION__
#include <e32test.h>
#include "t_rpmb.h"

RTest test(_L("t_rpmb"));
RTestRpmb	RpmbTestDeviceDriver;

/******************************************************************************
 * Main
 ******************************************************************************/
TInt E32Main()
	{
#if !defined(__WINS__)
	test.Title();
	test.Start(_L("Opening device driver"));
	TInt r;
	r = User::LoadLogicalDevice(KRpmbTestLddName);
	test_Value(r,r==KErrNone||r==KErrAlreadyExists);
	r = RpmbTestDeviceDriver.Open();
	test_KErrNone(r);
	test.Next(_L("Execute RPMB tests"));	
	r = RpmbTestDeviceDriver.RunTests();
	test_KErrNone(r);
	RpmbTestDeviceDriver.Close();
	r = User::FreeLogicalDevice(KRpmbTestLddName);
	test_KErrNone(r);
	test.End();
#else
test.Printf(_L("This test does not run on emulator. \n"));
#endif
	return 0;
	}

