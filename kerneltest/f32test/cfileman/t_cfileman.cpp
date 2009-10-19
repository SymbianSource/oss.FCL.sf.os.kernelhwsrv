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
// f32test\cfileman\t_cfileman.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>

#include "t_cfileman_cases.h"

static RArray<TUint> gFailedTestCases;

RTest test(_L("T_CFILEMAN"));

/*
 * Prints failure notification for failed test cases during test running period
 */ 
void DoLogTestCaseFailure(const TTestParamAll& aParam)
	{
	const TUint testCaseId = aParam.iTestCaseID; 
	test.Printf(_L("Test Failure: Case %d !\n"), testCaseId);
	gFailedTestCases.Append(testCaseId);
	test.Printf(_L("Print out directory contents:\n"));
	PrintDir(aParam.iSrcPrsPath, *aParam.iSrcDrvChar);
	PrintDir(aParam.iSrcCmpPath, *aParam.iSrcDrvChar);
	if (aParam.iAPI == ECFMMove || aParam.iAPI == ECFMRename || aParam.iAPI == ECFMCopy || aParam.iAPI == ECFMCopyHandle)
		{
		PrintDir(aParam.iTrgPrsPath, *aParam.iTrgDrvChar);
		PrintDir(aParam.iTrgCmpPath, *aParam.iTrgDrvChar);
		}
	}

/*
 * Overall test results logging module, prints out the failing test cases with their ID
 */
void DoPrintTestResults()
	{
	// if no failure found
	if (gFailedTestCases.Count() == 0)
		{
		test.Printf(_L("All test cases have passed!\n"));
		return;
		}

	test.Printf(_L("Test failure(s) found in following case(s):\n"));
	for (TInt i = 0; i < gFailedTestCases.Count(); i++)
		{
		test.Printf(_L("Test Case: %u"), gFailedTestCases[i]);
		}
	test(EFalse);
	}

/*
 * Presetting module, presets initial source, target and comparing direcotries.
 * @param	aParam	test param that contains all information about a test case
 */
void DoPresettings(const TTestParamAll& aParam)
	{

	// Setup source files
	TFileName path = aParam.iSrcPrsPath;
	path[0] = (TUint16)*aParam.iSrcDrvChar;
	
	if(path[0] == (TUint8)gDriveToTest || path[0] == (TUint8)gFixedDriveValid)
		{
		SetupDirFiles(path, aParam.iSrcPrsFiles);

		// setup source cmp files
		path = aParam.iSrcCmpPath;
		path[0] = (TUint16)*aParam.iSrcDrvChar;
		SetupDirFiles(path, aParam.iSrcCmpFiles);
		}
	
		if (aParam.iAPI == ECFMMove || aParam.iAPI == ECFMRename || aParam.iAPI == ECFMCopy || aParam.iAPI == ECFMCopyHandle)
			{
			// setup trg files
			path = aParam.iTrgPrsPath;
			path[0] = (TUint16)*aParam.iTrgDrvChar;
			
			if(path[0]== (TUint8)gDriveToTest)
				{
				SetupDirFiles(path, aParam.iTrgPrsFiles);
			
				// setup trg cmp files
				path = aParam.iTrgCmpPath;
				path[0] = (TUint16)*aParam.iTrgDrvChar;
				SetupDirFiles(path, aParam.iTrgCmpFiles);
				}
			}
	}

/*
 * Test execution module
 * @param	aParam	test param that contains all information about a test case
 * @panic	USER:84	if return codes do not match the expected values. 	
 */

typedef TBuf<350> TTestFileName; 
void DoCmdExecution(const TTestParamAll& aParam)
	{
	TTestFileName srcCmd = aParam.iSrcCmdPath;
	if (srcCmd.Length() > 0)
		{
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	else
		{
		srcCmd= gSessionPath;
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	TInt r = KErrNone;
	switch(aParam.iAPI)
		{
		case ECFMDelete:
			if (!gAsynch)
				{
				r = gFileMan->Delete(srcCmd, aParam.iSwitch);
				test_Equal(r, aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->Delete(srcCmd, aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
		break;
		
	    case ECFMRmDir:
			if (!gAsynch)
				{
				r = gFileMan->RmDir(srcCmd);
				test_Equal(r , aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->RmDir(srcCmd, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
		break;
		case ECFMAttribs:
			if (!gAsynch)
				{
				r = gFileMan->Attribs(srcCmd, aParam.iSetAttribs, aParam.iClearAttribs, aParam.iSetModified, aParam.iSwitch);
				test_Equal(r , aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->Attribs(srcCmd, aParam.iSetAttribs, aParam.iClearAttribs, aParam.iSetModified, aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
			break;
		case ECFMMove:
			{
			TTestFileName trgCmd = aParam.iTrgCmdPath;
			if (trgCmd.Length() > 0)
				{
				trgCmd[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				trgCmd= gSessionPath;
				}
			if (!gAsynch)
				{
				r = gFileMan->Move(srcCmd, trgCmd, aParam.iSwitch);
				test_Equal(r , aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->Move(srcCmd, trgCmd, aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
			break;
			}
		case ECFMCopy:
			{
			TTestFileName trgCmd = aParam.iTrgCmdPath;
			if (trgCmd.Length() > 0)
				{
				trgCmd[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				trgCmd= gSessionPath;
				}
			if (!gAsynch)
				{
				r = gFileMan->Copy(srcCmd, trgCmd, aParam.iSwitch);
				test_Equal(r , aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->Copy(srcCmd, trgCmd,aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
			break;
			}
		case ECFMRename:
			{
			TTestFileName trgCmd = aParam.iTrgCmdPath;
			if (trgCmd.Length() > 0)
				{
				trgCmd[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				trgCmd= gSessionPath;
				}
			if (!gAsynch)
				{
				r = gFileMan->Rename(srcCmd, trgCmd, aParam.iSwitch);
				test_Equal(r , aParam.iSyncReturn);
				}
			else
				{
				r = gFileMan->Rename(srcCmd, trgCmd, aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				}
			break;
			}
		case ECFMCopyHandle:
		 {
			TTestFileName trgCmd = aParam.iTrgCmdPath;
			if (trgCmd.Length() > 0)
				{
				trgCmd[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				trgCmd= gSessionPath;
				} 
			
			if (!gAsynch)
				{
				RFile tryfile;
				TInt ret = 0;
				ret = tryfile.Open(TheFs, srcCmd, EFileRead|EFileWrite);
				test_Equal(ret , KErrNone);
				r = gFileMan->Copy(tryfile, trgCmd, aParam.iSwitch);
				test_Equal(r , aParam.iSyncReturn);
				tryfile.Close();
				}
			else
				{
				RFile tryfile;
				TInt ret = 0;
				ret = tryfile.Open(TheFs, srcCmd, EFileRead|EFileWrite);
				test(ret == KErrNone);
				r = gFileMan->Copy(tryfile, trgCmd, aParam.iSwitch, gStat);
				User::WaitForRequest(gStat);
				test_Equal(r , aParam.iAsyncReturn);
				test(gStat == aParam.iAsyncStatus);
				tryfile.Close();
				}
		}
		default:
		break;

		}
			
	}

/*
 * Result verification module.
 * @param	aParam	test param that contains all information about a test case
 * @return	ETrue	if test results in expected behaviour, i.e. test passes
 * 			EFalse	if test results in unexpected behaviour, i.e. test fails
 */
TBool DoResultsVerification(const TTestParamAll& aParam)
	{
	TFileName srcPath = aParam.iSrcPrsPath;
	srcPath[0] = (TUint16)*aParam.iSrcDrvChar;
	TFileName srcCmpPath = aParam.iSrcCmpPath;
	srcCmpPath[0] = (TUint16)*aParam.iSrcDrvChar;
	
	if ((*aParam.iSrcDrvChar == gDriveToTest))
		{
		TBool rel = CompareL(srcPath, srcCmpPath);
		if(!rel)
			return EFalse;
		}
	
		if (aParam.iAPI == ECFMMove || aParam.iAPI == ECFMRename || aParam.iAPI == ECFMCopy || aParam.iAPI == ECFMCopyHandle)
			{
			TFileName trgPath = aParam.iTrgPrsPath;
			trgPath[0] = (TUint16)*aParam.iTrgDrvChar;
			TFileName trgCmpPath = aParam.iTrgCmpPath;
			trgCmpPath[0] = (TUint16)*aParam.iTrgDrvChar;
			if ((*aParam.iTrgDrvChar == gDriveToTest))
				{
				TBool rel = CompareL(trgPath, trgCmpPath);
				if(!rel)
					return rel;
				}
			
			}	
	return ETrue;
	}

/*
 * Search test cases by the index of the array of test case group, overloaded version for basic unitary cases.
 * @param 	aIdx		the test case index in search
 * @param	aBasicUnitaryTestCaseGroup		the input test group, should always be gBasicUnitaryTestCases[]
 * @param	aTestCaseFound		contains params of the test case found by the test case Id.
 * @return	KErrNone	if only one test case on the id is found
 * 			KErrNotFound	if no test case is found
 */
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseUnitaryBasic aBasicUnitaryTestCaseGroup[], TTestParamAll& aTestCaseFound)
	{
	if (aBasicUnitaryTestCaseGroup[aIdx].iBasic.iTestCaseID != 0)
		{
		aTestCaseFound.iTestCaseID 	= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iTestCaseID;
		aTestCaseFound.iAPI 		= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iAPI;
		aTestCaseFound.iSwitch		= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iSwitch;
		aTestCaseFound.iSyncReturn	= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iSyncReturn;
		aTestCaseFound.iAsyncReturn	= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iAsyncReturn;
		aTestCaseFound.iAsyncStatus	= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iAsyncStatus;

		aTestCaseFound.iSrcDrvChar	= aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iDrvChar;
		aTestCaseFound.iSrcCmdPath.Set(aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmdPath);
		aTestCaseFound.iSrcPrsPath.Set(aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsPath);
		aTestCaseFound.iSrcPrsFiles = aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsFiles;
		aTestCaseFound.iSrcCmpPath.Set(aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmpPath);
		aTestCaseFound.iSrcCmpFiles = aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmpFiles;
		}
	else
		return KErrNotFound;
	return KErrNone;
		
	}

/*
 * Search test cases by the index of the array of test case group, overloaded version for basic binary cases.
 * @param 	aIdx		the test case index in search
 * @param	aBasicUnitaryTestCaseGroup		the input test group, should always be gBasicBinaryTestCases[]
 * @param	aTestCaseFound		contains params of the test case found by the test case Id.
 * @return	KErrNone	if only one test case on the id is found
 * 			KErrNotFound	if no test case is found
 */
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseBinaryBasic aBasicBinaryTestCaseGroup[], TTestParamAll& aTestCaseFound)
	{
	if (aBasicBinaryTestCaseGroup[aIdx].iBasic.iTestCaseID != 0)
		{
		aTestCaseFound.iTestCaseID 	= aBasicBinaryTestCaseGroup[aIdx].iBasic.iTestCaseID;
		aTestCaseFound.iAPI 		= aBasicBinaryTestCaseGroup[aIdx].iBasic.iAPI;
		aTestCaseFound.iSwitch		= aBasicBinaryTestCaseGroup[aIdx].iBasic.iSwitch;
		aTestCaseFound.iSyncReturn	= aBasicBinaryTestCaseGroup[aIdx].iBasic.iSyncReturn;
		aTestCaseFound.iAsyncReturn	= aBasicBinaryTestCaseGroup[aIdx].iBasic.iAsyncReturn;
		aTestCaseFound.iAsyncStatus	= aBasicBinaryTestCaseGroup[aIdx].iBasic.iAsyncStatus;

		aTestCaseFound.iSrcDrvChar	= aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iDrvChar;
		aTestCaseFound.iSrcCmdPath.Set(aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmdPath);
		aTestCaseFound.iSrcPrsPath.Set(aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsPath);
		aTestCaseFound.iSrcPrsFiles = aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsFiles;
		aTestCaseFound.iSrcCmpPath.Set(aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmpPath);
		aTestCaseFound.iSrcCmpFiles = aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmpFiles;

		aTestCaseFound.iTrgDrvChar	= aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iDrvChar;
		aTestCaseFound.iTrgCmdPath.Set(aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iCmdPath);
		aTestCaseFound.iTrgPrsPath.Set(aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iPrsPath);
		aTestCaseFound.iTrgPrsFiles = aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iPrsFiles;
		aTestCaseFound.iTrgCmpPath.Set(aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iCmpPath);
		aTestCaseFound.iTrgCmpFiles = aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iCmpFiles;
		}
	else
		{
		return KErrNotFound;
		}
	return KErrNone;
	}

/*
 * Search test cases by test case Id, overloaded version for Basic unitary cases.
 * @param 	aCaseId		the test case Id in search
 * @param	aBasicUnitaryTestCaseGroup		the input test group, should always be gBasicUnitaryTestCases[]
 * @param	aTestCaseFound		contains params of the test case found by the test case Id.
 * @return	KErrNone	if only one test case on the id is found
 * 			KErrAlreadyExists	if more than one test cases found by the test case Id
 * 			KErrNotFound	if no test case is found
 */
TInt SearchTestCaseByTestCaseId(TUint aCaseId, const TTestCaseUnitaryBasic aBasicUnitaryTestCaseGroup[], TTestParamAll& aTestCaseFound)
	{
	TBool found = EFalse;
	TInt rel = KErrNone;

	// Scan through the test group by array index
	for(TInt i = 0; rel == KErrNone; i++)
		{
		rel = SearchTestCaseByArrayIdx(i, aBasicUnitaryTestCaseGroup, aTestCaseFound);
		if(aTestCaseFound.iTestCaseID == aCaseId)
			{
			// If more than one test cases found, return error 
			if(found)
				{
				return KErrAlreadyExists;
				}
			found = ETrue;
			}
		}

	if (!found)
		{
		return KErrNotFound;
		}
	return KErrNone;
	}

/*
 * Do all basic binary test cases defined in gBasicUnitaryTestCases[]
 */
void DoAllBasicUnitaryTests(const TTestCaseUnitaryBasic aBasicUnitaryTestCaseGroup[])
	{
	TTestParamAll nextTestCase;
	TInt i = 0;
	
	while (SearchTestCaseByArrayIdx(i, aBasicUnitaryTestCaseGroup, nextTestCase) == KErrNone)
		{
		TTime startTime;
		TTime endTime;
		startTime.HomeTime();
		DoPresettings(nextTestCase);
		DoCmdExecution(nextTestCase);
		if (!DoResultsVerification(nextTestCase))
			{
			DoLogTestCaseFailure(nextTestCase);
			}
		else
			{
			test.Printf(_L("Test ID %d passed \n"),nextTestCase.iTestCaseID);
			}
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken(0);
		timeTaken = endTime.MicroSecondsFrom(startTime);
		TInt time2=0;
		time2=I64LOW(timeTaken.Int64()/1000);
		test.Printf(_L("Time Taken by testid %d = %d mS \n"),nextTestCase.iTestCaseID,time2);		
		++i;

		}
	}

/*
 * Do all basic binary test cases defined in gBasicBinaryTestCases[]
 */
void DoAllBasicBinaryTests(const TTestCaseBinaryBasic aBasicBinaryTestCaseGroup[])
	{
	TTestParamAll nextTestCase;
	TInt i = 0;
	while (SearchTestCaseByArrayIdx(i, aBasicBinaryTestCaseGroup, nextTestCase) == KErrNone)
		{
		TTime startTime;
		TTime endTime;
		startTime.HomeTime();
		DoPresettings(nextTestCase);
		DoCmdExecution(nextTestCase);
		if (!DoResultsVerification(nextTestCase))
			{
			DoLogTestCaseFailure(nextTestCase);
			}
		else
			{
			test.Printf(_L("Test ID %d passed \n"),nextTestCase.iTestCaseID);
			}
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken(0);
		timeTaken = endTime.MicroSecondsFrom(startTime);

		TInt time2=0;
		time2=I64LOW(timeTaken.Int64()/1000);
		test.Printf(_L("Time Taken by test id %d = %d mS \n"),nextTestCase.iTestCaseID,time2);
		++i;
		}
	}

// Future work: provide command arguement parsing faclity so that users
//  can choose specific test case(s) in ranges
//  can choose specific API(s), switches, configurations, etc. for testing
/*
 * Main testing control unit
 */
void TestMain()
	{
//The __PERFTEST__ macro is for future use when a tests are setup to run on a performance machine 
//which will be enabled to run for both WINSCW and ARMV5
#ifndef __PERFTEST__
	//Tests are enabled to run for WINSCW only on the below specified drives due to the time constraint.
	if((gDriveToTest == 'C') || (gDriveToTest == 'X') || (gDriveToTest == 'Y'))
		{
		DoAllBasicUnitaryTests(gBasicUnitaryTestCases);
		DoAllBasicBinaryTests(gBasicBinaryTestCases);
		}
	else
		{
		test.Printf(_L("Drive %C: is not supported for this configuration, see test logs for supported configuration details"),gDriveToTest);
		}
#endif
	
#ifdef __PERFTEST__
		DoAllBasicUnitaryTests(gBasicUnitaryTestCases);
		DoAllBasicBinaryTests(gBasicBinaryTestCases);
#endif	
	}


/*
 * Initialise test, do all tests in both sync and async mode. 
*/
void CallTestsL()
	{
	InitialiseL();
	
	CreateTestDirectory(_L("\\F32-TST\\T_CFILEMAN\\"));

	gAsynch=EFalse;	
	test.Next(_L("Synchronous tests ..."));
	TestMain();
	
	DeleteTestDirectory();

	CreateTestDirectory(_L("\\F32-TST\\T_CFILEMAN\\"));
	gAsynch=ETrue;
	test.Next(_L("Asynchronous tests ..."));
	TestMain();

	DoPrintTestResults();
	Cleanup();
	DeleteTestDirectory();
	}
