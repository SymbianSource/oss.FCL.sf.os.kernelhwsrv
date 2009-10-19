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
// Bootstrap Shadow Memory Region Test Application
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <u32hal.h>
#include <hal.h>
#include <hal_data.h>
#include "d_smr.h"

#define __TRACE_LINE__()	test.Printf(_L("%d\n"),__LINE__)

RTest test(_L("T_SMR"));

RSMRTest SMRTest;


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
TInt TestSMRIBPtr (TBool aEnforce)
    {
    /* These tests assume that the Core Loader does not load and user data areas
	* as they are too large (>8Mb). Leaving the SMRIB undefined (==0) on NE1 and
	* H4. 
    */
    
	/* Print out the SMRIB Pointer in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_CheckSMRIBPtr"));   
    test.Printf(_L("\n"));
	test_KErrNone( SMRTest.Test_CheckSMRIBPtr(aEnforce)); 

    return KErrNone;
    }
    
TInt TestSMRAccess (TBool aEnforce)
    {
    /* These tests assume that the Core Loader has been modified to treat the
    *  user data partitions as SMR partitions and load them into SMR memory area
    *  and updated teh SMRIB. Two SMRs are expected, each of 4Mb in size.    
    */
    
    
	/* Print out the SMRIB in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_PrintSMRIB"));   
    test.Printf(_L("\n"));
    test_KErrNone( SMRTest.Test_PrintSMRIB(aEnforce));
    
	/* Print out the SMRIB and access and dump 1Kb of memory in each SMR. 
    *  When enforce, zero SMR entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_AccessAllSMRs"));   
    test.Printf(_L("\n"));
	test_KErrNone( SMRTest.Test_AccessAllSMRs(aEnforce));

	/* Print out the SMRIB in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  

    return KErrNone;
    }
    

TInt TestSMRAccessAndRAMFree (TBool aEnforce)
    {
    /* These tests assume that the Core Loader has been modified to treat the
    *  user data partitions as SMR partitions and load them into SMR memory area
    *  and updated teh SMRIB. Two SMRs are expected, each of 4Mb in size.    
    */
    
    
	/* Print out the SMRIB in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_PrintSMRIB"));   
    test.Printf(_L("\n"));
    test_KErrNone( SMRTest.Test_PrintSMRIB(aEnforce));
    
	/* Print out the SMRIB and access and dump 1Kb of memory in each SMR. 
    *  When enforce, zero SMR entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_AccessAllSMRs"));   
    test.Printf(_L("\n"));
	test_KErrNone( SMRTest.Test_AccessAllSMRs(aEnforce));

	/* Print out the SMRIB in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_FreeHalfSMR1PhysicalRam"));   
    test.Printf(_L("\n"));
    test_KErrNone( SMRTest.Test_FreeHalfSMR1PhysicalRam(aEnforce));

	/* Print out the SMRIB in the Super Page. 
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_FreeAllSMR2PhysicalRam"));   
    test.Printf(_L("\n"));
    test_KErrNone( SMRTest.Test_FreeAllSMR2PhysicalRam(aEnforce));



	/* Print out the SMRIB in the Super Page to see how it has been modified.
    *  When enforce, zero entries will lead to a test failure.    
    */	  
    test.Next(_L("SMR Test: Test_PrintSMRIB"));   
    test.Printf(_L("\n"));
    test_KErrNone( SMRTest.Test_PrintSMRIB(aEnforce));

    return KErrNone;
    }


GLDEF_C TInt E32Main()
    {
	TInt r;
	TBuf<256> args;
	TInt arglen = 0;
	TBool enforce = EFalse;
	TBool getch = EFalse;
	TInt testcase = 0;
	
    test.Title();

	test.Start(_L("=== SMR Test Suite"));

	arglen = User::CommandLineLength();
	if (arglen > 0)
	{
		/** Process command arguments */
		test_Compare(arglen,<,256);	
		User::CommandLine(args);
		
		test.Printf(_L("Arguments: %S\n"), &args);
		
		if (args.Find(_L("-1")) >= 0)
			testcase = 1;
		if (args.Find(_L("-2")) >= 0)
			testcase = 2;
		if (args.Find(_L("-3")) >= 0)
			testcase = 3;
		if (args.Find(_L("-4")) >= 0)
			testcase = 4;
			
		if (args.Find(_L("-e")) >= 0)
			enforce = ETrue;

		if (args.Find(_L("-g")) >= 0)
			getch = ETrue;
		
		if (args.Find(_L("-h")) >= 0)
			{
			test.Printf(_L("usage: t_smr [-1|-2|-3|-4] [-e] [-h] [-g]\n"));
			goto done;
			}
	}
		
	TInt muid = 0;   
   	r = HAL::Get(HAL::EMachineUid, muid);
	test_KErrNone(r);
   	if ((muid != HALData::EMachineUid_NE1_TB) && (testcase != 1))
   		{
   		test.Printf (_L("Testing skipped as test only applies to NaviEngine platform\n"));
   		goto done;
   		}

	test.Next(_L("=== Open test LDD"));
	r = User::LoadLogicalDevice(RSMRTest::Name());
	test_Assert((r==KErrNone || r==KErrAlreadyExists),void (0));
		
	r = SMRTest.Open();
	test_KErrNone(r);


    // Do test cases
    //
    if (testcase == 1) 
    	{
    	TestSMRAccess(enforce);
    	}
    else if (testcase == 2)
    	{
    	TestSMRAccessAndRAMFree(enforce);
		}
	else if (testcase == 3)
		{
    	TestSMRIBPtr(enforce);		
		}
	else if (testcase == 4)
		{
    	TestSMRIBPtr(enforce);		
		}
	else
		{
		test.Printf (_L("Just open and closing test driver, no test case argument supplied."));
		}

	test.Next(_L("=== Close LDD"));
	SMRTest.Close();
	
	r = User::FreeLogicalDevice(RSMRTest::Name());
    test_KErrNone(r);

done:
	test.End();
	
	if (getch)
		{
		test.Printf(_L("Press any key to continue...\n"));
		(void) test.Getch();
		}
	return(0);
    }

