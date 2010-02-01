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
// Hardware Configuration Respoitory Test Application
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <u32hal.h>
#include <hal.h>
#include <hal_data.h>
#include "d_hcrut.h"

#define __TRACE_LINE__()	test.Printf(_L("%d\n"),__LINE__)

RTest test(_L("T_HCRUT"));

_LIT(KTestDriver,"d_hcrut");
	
RHcrTest HcrTest;

//---------------------------------------------
//! @SYMTestCaseID 
//! @SYMTestType
//! @SYMPREQ 
//! @SYMTestCaseDesc
//! @SYMTestActions 
//!
//! @SYMTestExpectedResults 
//!		
//! @SYMTestPriority 
//! @SYMTestStatus
//---------------------------------------------
TInt TestBasics ()
	{
	test.Next(_L("Switch repository test"));

	test_KErrNone( HcrTest.Test_SwitchRepository());

	// Wait for idle + async cleanup (waits for DKernelEventHandler to go away)
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

    User::After(1000000);

    return KErrNone;
    }




GLDEF_C TInt E32Main()
    {
	TInt r;
	    
    test.Title();
	test.Start(_L("=== HCR Test Suite"));


	test.Next(_L("=== Open test LDD"));
	r = User::LoadLogicalDevice(KTestDriver);
	test_Assert((r==KErrNone || r==KErrAlreadyExists),void (0));
		
	r = HcrTest.Open();
	test_KErrNone(r);
    // Do test cases
    //
    TestBasics();

	test.Next(_L("=== Close LDD"));
	HcrTest.Close();
	
	r = User::FreeLogicalDevice(RHcrTest::Name());
    test_KErrNone(r);

	test.End();
	
	return(KErrNone);
    }

