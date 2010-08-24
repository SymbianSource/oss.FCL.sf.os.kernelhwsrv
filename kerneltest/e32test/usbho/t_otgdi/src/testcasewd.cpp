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
// testengine.cpp
// @internalComponent
// Test Case watchdog implementation
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <u32std.h> 	// unicode builds
#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32Test.h>	// RTest headder
#include "testcaseroot.h"
#include "TestCasewd.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testcasewdTraces.h"
#endif




const TInt KMagicNumberWDogValid	= 0xF143F00D;
_LIT(KMsgWatchdogPanicd, "Test Case watchdog error");

//
// CTestCaseWatchdog: Timer for any OTG event (async calls) time-outs
//

CTestCaseWatchdog::CTestCaseWatchdog()
    : CTimer(EPriorityUserInput)
    {
    }
    

CTestCaseWatchdog::~CTestCaseWatchdog()
    {
	Cancel();
	iValidConstr = 0;
    }
    

CTestCaseWatchdog* CTestCaseWatchdog::NewL()
    {
    CTestCaseWatchdog *self = new (ELeave) CTestCaseWatchdog();
    CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
    return self;
    }
    

void CTestCaseWatchdog::ConstructL()
    {
	iValidConstr = KMagicNumberWDogValid;
	CTimer::ConstructL();
	CActiveScheduler::Add(this);
    }
    

void CTestCaseWatchdog::RunL()
// Timer request has completed, so notify the timer's owner that we timed out
    {
	if(gVerboseOutput)
	    {
	    OstTraceFunctionEntry0(CTESTCASEWATCHDOG_RUNL);
	    }
	__ASSERT_ALWAYS(iCancelFriendFunc, User::Panic(KMsgWatchdogPanicd, EPanicWatchdogError));
	__ASSERT_ALWAYS(iThisPointer, User::Panic(KMsgWatchdogPanicd, EPanicWatchdogError));
	(*iCancelFriendFunc)(iThisPointer);
	iCancelFriendFunc = NULL;
	}
	

// call back setup
void CTestCaseWatchdog::IssueRequest(TInt aWatchdogIntervalMS, 
		CTestCaseRoot* pRoot, 
		WDCancellerMethod cancelMethod)
	{
	LOG_VERBOSE2(_L("Watchdogging this step for %d ms\n"), aWatchdogIntervalMS);
	if(gVerboseOutput)
	    {
	    OstTrace1(TRACE_VERBOSE, CTESTCASEWATCHDOG_ISSUEREQUEST, "Watchdogging this step for %d ms\n", aWatchdogIntervalMS);
	    }
	if (IsValid())
		{
		Cancel();
		
		iThisPointer = pRoot;				// save caller instance data
		iCancelFriendFunc = cancelMethod;	// save cancel handler
		iMSRequested = aWatchdogIntervalMS;
		After(aWatchdogIntervalMS * 1000);	// convert to uS
		}
	}
	
	
/** IsValid
 A common mistake is to not new() this event source and start using it before instantiation
*/
TBool CTestCaseWatchdog::IsValid()
	{
	// return ETrue if the object is validly constructed
	// This is test code, or else we would ifdef this out
	if (KMagicNumberWDogValid!= iValidConstr)
		{
		
		test.Printf(_L("CTestCaseWatchdog obj not properly constructed!\n"));
		OstTrace0(TRACE_NORMAL, CTESTCASEWATCHDOG_ISVALID, "CTestCaseWatchdog obj not properly constructed!\n");
		return(EFalse);
		}
	return(ETrue);	
	}



