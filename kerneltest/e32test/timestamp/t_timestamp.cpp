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
//  
// This test checks the accuracy if NKern::Timestamp (in SMP systems) or
// NKern::Fastcounter (in unicore), when low power modes are being selected
// during idle.
// Platforms requiring this test need to provide a d_timestamp.pdd which
// implements the functions required to test the accurary. If no pdd is 
// supplied the test is skipped and claims to succeed.
// Overview:

//-------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_timestamp-2706
//! @SYMTestCaseDesc			verifying that NKern::Timestamp/FastCounter works correctly 
//!                             accross low power modes
//! @SYMPREQ					417-52765
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 1. This test harness first gets information about NKern::Timestamp or
//!     NKern::FastCounter in particular its frequency. It also obtains
//!     the following constants: Number of timer to run the test, number
//!     of retries before giving up if no low power modes are detected,
//!     and acceptable error percent. Length in nanokernel ticks of each cycle
//! 2. Then it measures the timestamp before and after a time interval 
//!    controlled by NTimerIntervalInS (constant is in seconds)
//!    2.1 If in period of KTimerInterval a low power mode was entered and 
//!        the timer underlying NKern::Timestamp had to be restated then before
//!        and after time is stored. The cycle is considered successful.
//! 3. If KNMaxentries occur with no successful entries the test fails
//! 4. If any valid entry has an interval as measured with NKern::Timestamp that
//!    is outside KNErrPercent of the same interval as measured with nanokernel 
//!    ticks then the test fails
//! 5. If KNValidRuns valid cycles have an acceptable error the test succeeds
//! 
//! @SYMTestExpectedResults 
//!     test passed
//-------------------------------------------------------------------------------------------

#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <hal.h>
#include <u32hal.h>
#include <e32svr.h>
#include "d_timestamp.h"

_LIT(KTimestampTestLddFileName,"D_TIMESTAMP.LDD");
_LIT(KTimestampTestPddFileName,"D_TIMESTAMP.PDD");

RTest test(_L("T_TIMESTAMP"));

LOCAL_C void UnloadDrivers()
	{
    test.Printf(_L("Unloading LDD\n"));

	TInt r=User::FreeLogicalDevice(RTimestampTest::Name());
    test_KErrNone(r);
    
	TName pddName(RTimestampTest::Name());
	_LIT(KPddWildcardExtension,".*");
	pddName.Append(KPddWildcardExtension);
	TFindPhysicalDevice findPD(pddName);
	TFullName findResult;
	r=findPD.Next(findResult);
	while (r==KErrNone)
		{
        test.Printf(_L("Unloading PDD: %S\n"),&findResult);
		r=User::FreePhysicalDevice(findResult);
        test_KErrNone(r);
		findPD.Find(pddName); // Reset the find handle now that we have deleted something from the container.
		r=findPD.Next(findResult);
		} 
	}


GLDEF_C TInt E32Main()
    {

    TBool dontFail = (User::CommandLineLength()!=0);
    
    test.Title();

    test.Start(_L("Timestamp accuracy test"));
    

    TInt r;
    TRequestStatus st;
    
    r = User::LoadPhysicalDevice(KTimestampTestPddFileName);
    if (KErrNotFound == r)
        {
        test.Printf(_L("No timestamp pdd, test skipped\n"));
        test.End(); // skip test if this platform does not supply a PDD
        return 0;
        }
    

	r=User::LoadLogicalDevice(KTimestampTestLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
    
    RTimestampTest ldd;
    r = ldd.Open();
    test_KErrNone(r);
    
    test.Next(_L("Get timestamp frequency"));
    STimestampTestConfig info;
    r = ldd.Config(info);
    test_KErrNone(r);
    

    TUint retries = 0;
    TUint validruns = 0;
    
    test.Next(_L("Get nanotick frequency"));
	TInt tickPeriod = 0;
	r = HAL::Get(HAL::ENanoTickPeriod, tickPeriod);
    test_KErrNone(r);
    test.Printf(_L(" tick period in uS== %d\n"), tickPeriod);

    TInt ticks = info.iTimerDurationS*1000000/tickPeriod;
    TUint64 expected = info.iTimerDurationS*info.iFreq;
    TUint64 acceptError = info.iErrorPercent*expected/100;
    
    test.Printf(_L("running at %dHz for %d interations, each lasting %d seconds and with %d retries\n"),
                info.iFreq,
                info.iIterations,
                info.iTimerDurationS,
                info.iRetries
        );
    
    test.Printf(_L("expecting %lu with up to %lu error\n"),expected,acceptError);

    test.Next(_L("test timer interval"));
    STimestampResult result;
    STimestampResult* validResults = new STimestampResult[info.iIterations];
    memset(&validResults[0],0,sizeof(validResults));
    ldd.Start(st,ticks);
    User::WaitForRequest(st);
    test_KErrNone(st.Int());
    
    FOREVER
        {
        ldd.WaitOnTimer(st,result);
        User::WaitForRequest(st);
        test_KErrNone(st.Int());
        TUint64 error = (result.iDelta>expected) ? result.iDelta-expected : expected - result.iDelta;

        if (error < acceptError)
            {
            test.Printf(_L("Got %lu expected %lu, LPM Entered:%d, error %lu is OK \n"),
                        result.iDelta,expected,result.iLPMEntered,error);
            }
        else 
            {
            test.Printf(_L("Got %lu expected %lu, LPM Entered:%d, error %lu is BAD\n"),
                        result.iDelta,expected,result.iLPMEntered,error);
            if (!dontFail) 
                {
                delete [] validResults;
                ldd.Close();
                UnloadDrivers();
                test(error < acceptError);
                }
            }

        if (result.iLPMEntered)
            {
            retries = 0;
            validResults[validruns] = result;
            if (++validruns==info.iIterations) break;
            }
        else
            {
            retries++;
            if (retries==info.iRetries) 
                {
                test.Printf(_L("several retries with no power mode entry ... aborting ...\n"));
                ldd.Close();
                delete [] validResults;
                UnloadDrivers();
                test_Compare(retries,<,info.iRetries);
                }
            
            }
        }

    delete [] validResults;
    ldd.Close();
    UnloadDrivers();
    test.End();
	return(0);
    }
