// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include <e32def.h>
#include <e32def_private.h>
#include "TestCaseFactory.h"
#include "debugmacros.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcasefactoryTraces.h"
#endif


	
RTestFactory& RTestFactory::Instance()
	{
	static RTestFactory singleton;
	return singleton;
	}
	
	
RTestFactory::~RTestFactory()
	{
	}
	
	
RTestFactory::RTestFactory()
:	iTestCases(TStringIdentity::Hash,TStringIdentity::Id)
	{
	}
	
	
void RTestFactory::RegisterTestCase(const TDesC& aTestCaseId,TCreationMethod aCreationMethod)
	{
	TStringIdentity key(aTestCaseId);
	TInt err(Instance().iTestCases.Insert(key,aCreationMethod));
	if (err != KErrNone)
		{
		// Log that a test case could not be registered due to err
		OstTraceExt1(TRACE_NORMAL, RTESTFACTORY_REGISTERTESTCASE_DUP01, "Test case '%S' could not be registered with test case factory",aTestCaseId);
		}
	else
		{
		RTestFactory::TCreationMethod* creatorFunction = Instance().iTestCases.Find(key);
		if (creatorFunction == NULL)
			{
			OstTraceExt1(TRACE_NORMAL, RTESTFACTORY_REGISTERTESTCASE_DUP02, "<Error> Test case '%S' did not register",aTestCaseId);
			ListRegisteredTestCases();
			}
		else
			{
			OstTraceExt1(TRACE_NORMAL, RTESTFACTORY_REGISTERTESTCASE_DUP03, "Test case '%S' registered in factory",aTestCaseId);
			}
		}
	}


TBool RTestFactory::TestCaseExists(const TDesC& aTestCaseId)
	{
	RTestFactory::TCreationMethod creatorFunction;
	TStringIdentity key(aTestCaseId);
	
	creatorFunction = REINTERPRET_CAST(RTestFactory::TCreationMethod, Instance().iTestCases.Find(key));
	return (NULL != creatorFunction);
	}


/* Returns the test ID (name) of test at offset aIndex in the MAP
 */
void RTestFactory::GetTestID(TInt aIndex, TBuf<KTestCaseIdLength> &aTestID)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(RTESTFACTORY_GETTESTID);
	    }

	RFactoryMap::TIter it(Instance().iTestCases);
	
	TInt count(0);
	for (count=0; count<Instance().iTestCases.Count(); count++)
		{
		it.NextKey();
		if (count == aIndex)
			{
			TStringIdentity k(*it.CurrentKey());
			TBuf<64> *p = REINTERPRET_CAST(TBuf<64>*, &k);
			
			aTestID.Copy( *p);	
			return;
			}
		}
	// Error: Case not found!
	User::Leave(-2);
	}
	

/* Return the ordinal value of the Test ID (numeric portion)
 */
TInt TestIDValue(const TDesC & aTestID)
	{
	TUint16 value;
	TBuf<KTestCaseIdLength> id;
	id = aTestID.Right(4);
	TLex  lex(id);
	if (KErrNone == lex.Val(value, EDecimal))
		return(value);
	return(-1);	
	}

/* Print the test IDs in numerical order
 * Returns a sorted array of strings containing all of the test IDs
 */	
void RTestFactory::ListRegisteredTestCases(RPointerArray<HBufC> & aTestCaseNameArr)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(RTESTFACTORY_LISTREGISTEREDTESTCASES);
	    }
	RFactoryMap::TIter it(Instance().iTestCases);
	TInt count(0);
	TInt cases(Instance().iTestCases.Count());
	
	test.Printf(_L("------ F A C T O R Y -------\n"));
	OstTrace0(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES_DUP01, "------ F A C T O R Y -------\n");
	
	it.Reset();
	for (count=0; count<Instance().iTestCases.Count(); count++)
		{
		TStringIdentity k(*it.NextKey());
		TBuf<KTestCaseIdLength> *p = REINTERPRET_CAST(TBuf<KTestCaseIdLength>*, &k); // pointer to the test ID
		TBool placed(EFalse);
		TInt  pos(0);
		TInt  val(0);
		
		// build the sorted list
		while (!placed)
			{
			val = TestIDValue(*p);
			if (aTestCaseNameArr.Count()==pos) 
				{ //array empty or reached end
				HBufC* testIdentity = HBufC::NewLC(KTestCaseIdLength);
				*testIdentity = *p;
				aTestCaseNameArr.Append(testIdentity);
				CleanupStack::Pop(testIdentity);
				placed = ETrue;
				}
			else
				{
				if ( val < TestIDValue(*aTestCaseNameArr[pos]) )
					{
					HBufC* testIdentity = HBufC::NewLC(KTestCaseIdLength);
					*testIdentity = *p;
					aTestCaseNameArr.Insert(testIdentity, pos);
					placed = ETrue;
					CleanupStack::Pop(testIdentity);
					}
				else
					{
					pos++;
					}
				}
			}
		}
	// print it
	for (count=0; count<aTestCaseNameArr.Count(); count++)
		{
		test.Printf(_L("% 2d: %S\n"), count, aTestCaseNameArr[count]);
		OstTraceExt2(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES_DUP02, "% 2d: %S\n", count, *aTestCaseNameArr[count]);
		}
	
	test.Printf(_L("----------------------------\n"));
	OstTrace0(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES_DUP03, "----------------------------\n");
	}


CTestCaseRoot* RTestFactory::CreateTestCaseL(const TDesC& aTestCaseId)
	{
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(RTESTFACTORY_CREATETESTCASEL);
	    }
	RTestFactory::TCreationMethod creatorFunction = NULL;
	TInt err(KErrNone);
	TStringIdentity key(aTestCaseId);
	
	TRAP(err,creatorFunction = Instance().iTestCases.FindL(key));
	if (err != KErrNone)
		{
		// Test case is not present in the factory therefore test cannot support specified test case
		OstTraceExt2(TRACE_NORMAL, RTESTFACTORY_CREATETESTCASEL_DUP01, "<Error %d> Test case '%S' not supported",err,aTestCaseId);
		ListRegisteredTestCases();
		User::Leave(err);
		}

	OstTraceExt1(TRACE_NORMAL, RTESTFACTORY_CREATETESTCASEL_DUP02, "Creating test case '%S'",aTestCaseId);
		
	// Call the creator function to create the test case object
	return creatorFunction(gSemiAutomated);
	}
	

