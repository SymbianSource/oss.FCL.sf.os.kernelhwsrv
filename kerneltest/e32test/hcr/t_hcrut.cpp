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
#include <drivers/hcr.h>

#include "hcr_hai.h" // for SSettingC structure
#include "hcr_uids.h"

using namespace HCR;

#include "d_hcrut.h"

#include "d_hcrsim_testdata.h"

#define __TRACE_LINE__()	test.Printf(_L("%d\n"),__LINE__)

RTest test(_L("T_HCRUT"));

_LIT(KTestDriver,"d_hcrut");

RHcrTest HcrTest;


void TestBoundaryFindSettingsInCategory()
    {
    TInt32 firstEl;
    TInt32 lastEl;


    //Test low end boundary conditions in the compiled repository, first and 
    //end elements don't belong to the repository
    TUint32 TestCat = KHCRUID_TestCategory1;
    TInt NumInTestCat = 15;
    
    test_KErrNone(HcrTest.Test_FindCompiledSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);

    // Load file repository data for the boundary test condition
    const TText * fileBc0 = (const TText *)"filerepos_bc0.dat";
    test_KErrNone(HcrTest.Test_SwitchFileRepository(fileBc0));

    // Test the low end boundary conditions in the file repository.
    // First element in the category is also the first element in the
    // repository
    TestCat = 0x00000001;
    NumInTestCat = 4;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);

    // Test the high end boundary conditions in the file repository.
    // Last element in the category is also the last element in the
    // repository
    TestCat = 0xFFFFFFFF;
    NumInTestCat = 4;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);



    //Load relevant coreImg repository
    const TText * fileBc1 = (const TText *)"filerepos_bc1.dat";
    test_KErrNone(HcrTest.Test_SwitchFileRepository(fileBc1));


    //Test the low end boundary condition when first element of the 
    //repository does not belong to the requested category, so first element
    // in TestBc1Cat0 is second in the repository.
    TestCat = 0x2;
    NumInTestCat = 4;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);



    //Test the high end boundary condition when last element in the 
    // repository does not belong to TestBc1Cat1 in opposite to the previous
    //element which part of this category.
    TestCat = 0x10000002;
    NumInTestCat = 7;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);


    //The required category has only one element and it's first setting
    //in the repository
    TestCat = 0x01;
    NumInTestCat = 1;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);

    //The required category has only one element and it's the last setting
    //in the repository
    TestCat = 0xFFFFFFFF;
    NumInTestCat = 1;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);


    //Standard use case, all elements of the category are situated somewhere
    //in the middle of the repository
    TestCat = 0x10000002;
    NumInTestCat = 7;

    test_KErrNone(HcrTest.Test_FindFileSettingsInCategory(TestCat,
            &firstEl, &lastEl));
    test_Equal((lastEl - firstEl + 1), NumInTestCat);
    }

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
	test.Next(_L("Check Integrity test"));
#ifdef __WINS__
	test.Printf(_L("Skipped on the emulator"));
#else
	test_KErrNone( HcrTest.Test_CheckIntegrity());	
#endif // __WINS__

	test.Next(_L("Check Content test"));
#ifdef __WINS__
	test.Printf(_L("Skipped on the emulator"));
#else
	test_KErrNone( HcrTest.Test_CheckContent());	
#endif // __WINS__

	
	test.Next(_L("Switch repository test"));
	test_KErrNone(HcrTest.Test_SwitchRepository());
	
	test.Next(_L("Invoke FindSettingsInCategory boundary conditon tests"));
	TestBoundaryFindSettingsInCategory();
		
	// Wait for idle + async cleanup (waits for DKernelEventHandler to go away)
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

    User::After(5000000);
    

    return KErrNone;
    }


TInt TRepositoryGetWordValueUnitTests ()
	{
	test.Next(_L("Unit tests for HCR::TRepositoryFile::GetWordValue && HCR::TRepositoryCompiled::GetWordValue"));

	SSettingC* setting;
	for(setting = SettingsList; setting < SettingsList + sizeof(SettingsList) / sizeof(SSettingC); ++setting)
		{
		TCategoryUid category = setting->iName.iId.iCat;
		TElementId key = setting->iName.iId.iKey;
		TInt type = setting->iName.iType;
		test_KErrNone( HcrTest.Test_TRepositoryGetWordValue(category, key, type));
	
		}

	// Wait for idle + async cleanup (waits for DKernelEventHandler to go away)
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

    return KErrNone;
    }

TInt NegativeTestsLargeValues ()
	{
	test.Next(_L("Negative tests for HCR::TRepository::GetLargeValues"));

	TInt expectedError = KErrArgument;

	test_KErrNone( HcrTest.Test_NegativeTestsLargeValues(expectedError));

	// Wait for idle + async cleanup (waits for DKernelEventHandler to go away)
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

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

	NegativeTestsLargeValues();
	TRepositoryGetWordValueUnitTests();

	test.Next(_L("=== Close LDD"));
	HcrTest.Close();
	
	r = User::FreeLogicalDevice(RHcrTest::Name());
    test_KErrNone(r);

	test.End();
	
	return(KErrNone);
    }

