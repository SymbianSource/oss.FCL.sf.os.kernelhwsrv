// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\earlyextension\t_testextension.cpp
// 
//

#include <e32test.h>
#include <hal.h>
#include "d_testearlyextension.h"

RLddEarlyExtensionTest lddChan;
GLDEF_D RTest test(_L("LDD tests"));
_LIT(KLddFileName, "D_TESTEARLYEXTENSION.LDD");

GLDEF_C TInt E32Main()
    {
	test.Title();
	test.Start(_L("Testing early extension...\n"));
	//Load logical device
	TInt r = User::LoadLogicalDevice(KLddFileName);
	test((r == KErrNone) || (r == KErrAlreadyExists));
	//Open the channel
	r = lddChan.Open();
    test(r==KErrNone || r==KErrAlreadyExists);
	Int64 earlyExtTime = 0, extTime = 0;
	//Get system time stamps 
	r = lddChan.Test_getSystemTimeStamps(earlyExtTime, extTime);
    test(r == KErrNone);
	//Compare the time stamps for correctness. 
	//Time stamps got in early extension should be less than (valuewise) the one got in normal extension entry point.
	if(earlyExtTime > extTime)
		{
		test.Printf(_L("Early Extension time stamp %ld is greater than extension time stamp %ld\n"), earlyExtTime, extTime);
		test(0);
		}
	test.Printf(_L("Time stamps are as expected!!!\n"));
	test.Printf(_L("Closing the channel\n"));
	lddChan.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);
	test(r==KErrNone);
	User::After(100000);
	test.End();
	test.Close();
	return r;
	}
