// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Kernel Performance Logger test
// Uses helper LDD for performing actual logging from the kernel side.
// See 
// 
//

/**
 @file
*/


#include <e32std.h>
#include <e32std_private.h>
#include <e32cons.h>
#include <e32test.h>
#include <e32math.h>


#include "t_perflogger.h"
#include "t_perflogger_drv.h"


//-------------------------------------------------------------------------------------

//-- logging subcategoty test tags
const TUint8    KLogSubCat_UserThread   = 0x11;
const TUint8    KLogSubCat_KernelThread = 0x12;
const TUint8    KLogSubCat_ISR          = 0x13;
const TUint8    KLogSubCat_IDFC         = 0x14;


//-------------------------------------------------------------------------------------

RTest   test(_L("T_PerfLogger"));
TInt64  rndSeed;

//-------------------------------------------------------------------------------------

/**
Print out the logging record.
@param aTraceLayout unrolled trace record structure
*/
void  PrintTraceRecord(const TTraceLayout& aTraceLayout)
	{
    TBuf<256> printBuf;
    
    printBuf.Format(_L("Record: Size:%d, Flags:0x%02x, Cat:%d, SubCat:%d "), aTraceLayout.iSize, aTraceLayout.iFlags, aTraceLayout.iCategory, aTraceLayout.iSubCategory);
	
    TUint flags = aTraceLayout.iFlags;
	
    if(flags&(BTrace::EHeader2Present))
        printBuf.AppendFormat(_L("Header2:0x%08x "), aTraceLayout.iHeader2);
	
    if(flags & (BTrace::ETimestampPresent))
        printBuf.AppendFormat(_L("Timestamp:0x%x "), aTraceLayout.iTimestamp);
	
    if(flags & (BTrace::ETimestamp2Present))
        printBuf.AppendFormat(_L("Timestamp2:0x%x "), aTraceLayout.iTimestamp2);
	
    if(flags & (BTrace::EContextIdPresent))
        printBuf.AppendFormat(_L("Context:0x%08x "), aTraceLayout.iContext);
	
    if(flags & (BTrace::EPcPresent))
        printBuf.AppendFormat(_L("PC: 0x%08x "),  aTraceLayout.iPC);
	
    if(flags & (BTrace::EExtraPresent))
        printBuf.AppendFormat(_L("Extra: 0x%08x "),  aTraceLayout.iExtra);
	
    printBuf.Append(_L("| "));
    for(TInt i=0; i< aTraceLayout.iDataWords; ++i)
		{
        printBuf.AppendFormat(_L(" 0x%08x"), aTraceLayout.ipData[i]);    
		}
    
    printBuf.Append(_L("\n"));
    test.Printf(printBuf);
	
	}

//-------------------------------------------------------------------------------------

/**
	Get logging trace from the kernel trace engine and optionally check if it corresponds to the 
	TTestLogCtrl structure fields. Actually, the logging trace in our case shall be the result of the 
	test helper LDD worr, that in turn, is controlled by the data in TTestLogCtrl structure.

  @param  aTrace          ref. to the kernel trace engine LDD.
  @param  apControlStruct if not NULL obtained trace fields checked against fields of this structure.
  
*/
void GetParseRecordData(RBTrace& aTrace, const TTestLogCtrl* apControlStruct = NULL)
	{
    TUint8*         record;
    TTraceLayout    traceLayout;
	TInt    		nRecords = 0;
	
	for(TInt i=0; ;++i)
		{
        //-- get raw trace record
        TInt dataSize = aTrace.GetData(record);
		
        if(i != 0 && !dataSize)
            break; //-- no more data
        
        //-- check if we get log data at all at the very beginning
        if(i == 0 && !dataSize)
			{
			if(!apControlStruct)
				{//-- it's ok, no check
                break;
				}
            else
				{//-- panic if there is no log data, but we are required to get some.
                if(apControlStruct->iLogsNum > 0)
					{
                    test.Printf(_L("GetParseRecordData() No trace data found!\n"));
                    test(0);
					}
				}
			}
        
		
        TUint8* end = record+dataSize;
        TUint8* currPos = record;
		TUint nBytes = 0;
		
        //-- parser the record, print out fields and optionally check the correspondence to the fields of the control structure.
        for(; currPos<end; currPos+=nBytes)
			{
            
            nBytes = ParseTraceRecord(currPos ,traceLayout);
            test(nBytes >0 );
            
            //-- skip possible loggings that we didn't make
            if(traceLayout.iCategory != BTrace::EKernPerfLog)
                continue;
            
            ++nRecords;
			
            //-- print the record out
            PrintTraceRecord(traceLayout);

			//-- check the obtained record structure
			if(apControlStruct)
				{
				test(traceLayout.iCategory    == apControlStruct->iCategory);
				test(traceLayout.iSubCategory == apControlStruct->iSubCategory);
				
				if(traceLayout.iDataWords > 1)  //-- we have at least 1 word of user data (1 is for TickCount)
					test(traceLayout.ipData[0] == apControlStruct->iUserData);
				
				if(traceLayout.iDataWords > 2)  //-- we have 2 words of user data (1 is for TickCount)
					test(traceLayout.ipData[1] == apControlStruct->iUserData2);
				
				}
            
			}
				
		//-- release data buffer.
		aTrace.DataUsed();
		}
		
	//-- check number of trace records obtained
	if(apControlStruct)
		{
		test(nRecords == apControlStruct->iLogsNum);
		}

	}


//---------------------------------------------------------------------------------

/**
	Parse logging record, converting it from the raw representation to the unrolled data 
	structure.

	@param  apRecord     points to the buffer with the raw trace record
	@param  aTraceLayout here parsed record will bre returned
  
    @return sise of the record, bytes
*/
TUint  ParseTraceRecord(const TUint8* apRecord, TTraceLayout& aTraceLayout)
	{
    aTraceLayout.iSize          = apRecord[BTrace::ESizeIndex];    
    aTraceLayout.iFlags         = apRecord[BTrace::EFlagsIndex];
    aTraceLayout.iCategory      = apRecord[BTrace::ECategoryIndex];
    aTraceLayout.iSubCategory   = apRecord[BTrace::ESubCategoryIndex];
	
    const TUint flags= aTraceLayout.iFlags;   
	TInt size = aTraceLayout.iSize;
    const TUint32* pData = (const TUint32*)(apRecord+4);
	
    size -= 4; //-- count header
	
    //-- process header flags and appropriate fields of the record
    if(flags&(BTrace::EHeader2Present))
		{
        aTraceLayout.iHeader2 = *pData++;   
        size -= sizeof(TUint32);
        test(size >= 0);
		}
	
    if(flags & (BTrace::ETimestampPresent))
		{
        aTraceLayout.iTimestamp = *pData++;
        size -= sizeof(TUint32);
        test(size >= 0);
		}
	
    if(flags & (BTrace::ETimestamp2Present))
		{
        aTraceLayout.iTimestamp2 = *pData++;
        size -= sizeof(TUint32);
        test(size >= 0);
		}   
	
	if(flags & (BTrace::EContextIdPresent))
		{
		aTraceLayout.iContext = *pData++;
		size -= sizeof(TUint32);
        test(size >= 0);
		}
	
	if(flags & (BTrace::EPcPresent))
		{
		aTraceLayout.iPC = *pData++;	
		size -= sizeof(TUint32);
        test(size >= 0);
		}
	
	if(flags & (BTrace::EExtraPresent))
		{
		aTraceLayout.iExtra = *pData++;	
		size -= sizeof(TUint32);
        test(size >= 0);
		}
	
    //-- process user data if present
    test(size >= 0);
    test(size % sizeof(TUint32) == 0);
	
    aTraceLayout.iDataWords = size / sizeof(TUint32);
    aTraceLayout.ipData = (aTraceLayout.iDataWords!=0) ? pData : NULL;
	
    return aTraceLayout.iSize;
	}

//---------------------------------------------------------------------------------
//! @SYMTestCaseID          KBASE-T_PERFLOGGER-0055
//! @SYMTestType            UT
//! @SYMPREQ                PREQ1030
//! @SYMTestCaseDesc        Tests logging from different contexts: user thread, ISR, Kernel thread and IDFC. 
//!                         The main functionality is performed by test heller LDD, "d_perflogger_test" that acually calls the logging function from different contexts 
//!                         following the commands from user application.
//!
//! @SYMTestActions         0   setup the data structures that specify the logging parameters.
//!                         1   call RKPLoggerTestHelper::MakeLogFromUserThread() this is a synchronous operation. Performs logging from the user thread context.
//!                         1.1 Parse the log record data and ensure that the logged information matches the data we passed to the helper LDD.
//!
//!                         2   call RKPLoggerTestHelper::MakeLogFromISR() this is asynchronous operation. Performs logging from the ISR context.
//!                         2.1 Wait for operation to complete, validate the completion status.
//!                         2.2 Parse the log record data and ensure that the logged information matches the data we passed to the helper LDD.
//!
//!
//!                         3   call RKPLoggerTestHelper::MakeLogFromDFC() this is asynchronous operation. Performs logging from the Kernel thread context (DFC).
//!                         3.1 Wait for operation to complete, validate the completion status.
//!                         3.2 Parse the log record data and ensure that the logged information matches the data we passed to the helper LDD.
//!
//!                         4   call RKPLoggerTestHelper::MakeLogFromIDFC() this is asynchronous operation. Performs logging from the IDFC.
//!                         4.1 Wait for operation to complete, validate the completion status.
//!                         4.2 Parse the log record data and ensure that the logged information matches the data we passed to the helper LDD.
//!
//!                         5.  Make scattered overlapped logging from ISR, DFC & IDFC simultaneously, ensure that this work.
//!
//! @SYMTestExpectedResults return if the performace logger behaves as expected, panic otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------------------------------------------
void TestPerfLogger(RBTrace &aTrace)
	{
    TInt    nRes;
    
    const TUint32   KLogUserData  = 0xBAAABEEF; //-- just for testing
    const TUint32   KLogUserData2 = 0x11FFDDCC; //-- just for testing
	
	
    test.Next(_L("testing kernel performance logger functionality\n"));
	
    RKPLoggerTestHelper testHelperLDD;  //-- helper test driver
    CleanupClosePushL(testHelperLDD);
	
    nRes = testHelperLDD.Open(TVersion(1,0,1)); //-- open test helper ldd for a logger
    test(nRes == KErrNone);
	
    //---
    TTestLogCtrl   logCtrl_UserThread;
    TTestLogCtrl   logCtrl_ISR;
    TTestLogCtrl   logCtrl_DFC;
    TTestLogCtrl   logCtrl_IDFC;
	
	
    TRequestStatus rqStatLogFromISR;
    TRequestStatus rqStatLogFromDFC;
    TRequestStatus rqStatLogFromIDFC;
	
	
    //-- setup log control structures
    logCtrl_UserThread.iLogsNum     = 3;
    logCtrl_UserThread.iCategory    = BTrace::EKernPerfLog;
    logCtrl_UserThread.iSubCategory = KLogSubCat_UserThread;
    logCtrl_UserThread.iUserData    = KLogUserData;
    logCtrl_UserThread.iUserData2   = KLogUserData2;
	
	
    logCtrl_ISR.iLogsNum        = 3;
    logCtrl_ISR.iCategory       = BTrace::EKernPerfLog;
    logCtrl_ISR.iSubCategory    = KLogSubCat_ISR;
    logCtrl_ISR.iUserData       = KLogUserData;
    logCtrl_ISR.iUserData2      = KLogUserData2;
	
	
    logCtrl_DFC.iLogsNum        = 3;
    logCtrl_DFC.iCategory       = BTrace::EKernPerfLog;
    logCtrl_DFC.iSubCategory    = KLogSubCat_KernelThread;
    logCtrl_DFC.iUserData       = KLogUserData;
    logCtrl_DFC.iUserData2      = KLogUserData2;
	
	
    logCtrl_IDFC.iLogsNum       = 3;
    logCtrl_IDFC.iCategory      = BTrace::EKernPerfLog;
    logCtrl_IDFC.iSubCategory   = KLogSubCat_IDFC;
    logCtrl_IDFC.iUserData      = KLogUserData;
    logCtrl_IDFC.iUserData2     = KLogUserData2;
	
	
	
    test.Printf(_L("Testing logging from user thread\n"));
    //============================================================
    //-- 1. make logging from user thread, this is a synchronous operation
    nRes = testHelperLDD.MakeLogFromUserThread(logCtrl_UserThread);
    test(nRes == KErrNone);
	
    //-- 1.1 check the results
    GetParseRecordData(aTrace, &logCtrl_UserThread);
	
	
    test.Printf(_L("Testing logging from ISR\n"));
    //============================================================
    //-- 2. make logging from ISR, this is asynchronous operation
    testHelperLDD.MakeLogFromISR(rqStatLogFromISR, logCtrl_ISR);
    User::WaitForRequest(rqStatLogFromISR);
    test(rqStatLogFromISR.Int() == KErrNone);
	
    //-- 2.1 check the results
    GetParseRecordData(aTrace, &logCtrl_ISR);
	
    test.Printf(_L("Testing logging from DFC\n"));
    //============================================================
    //-- 3. make logging from DFC kennel thread, this is asynchronous operation
    testHelperLDD.MakeLogFromDFC(rqStatLogFromDFC, logCtrl_DFC);
    User::WaitForRequest(rqStatLogFromDFC);
    test(rqStatLogFromDFC.Int() == KErrNone);
	
    //-- 3.1 check the results
    GetParseRecordData(aTrace, &logCtrl_DFC);
	
    test.Printf(_L("Testing logging from IDFC\n"));
    //============================================================
    //-- 4. make logging from IDFC, this is asynchronous operation
    testHelperLDD.MakeLogFromIDFC(rqStatLogFromIDFC, logCtrl_IDFC);
    User::WaitForRequest(rqStatLogFromIDFC);
    test(rqStatLogFromIDFC.Int() == KErrNone);
	
    //-- 4.1 check the results
    GetParseRecordData(aTrace, &logCtrl_IDFC);
	
	
    test.Printf(_L("Testing overlapped logging from ISR, DFC & IDFC\n"));
    //============================================================
    //-- 5. make logging from ISR, DFC and IDFC simultaneously
	
    //-- use random numbers for number and period of loggings  
    logCtrl_ISR.iLogsNum        = URnd(16)+1;
    logCtrl_ISR.iLogPeriodTick  = URnd(20)+1;
	
    logCtrl_DFC.iLogsNum        = URnd(16)+1;
    logCtrl_DFC.iLogPeriodTick  = URnd(20)+1;
	
    logCtrl_IDFC.iLogsNum       = URnd(16)+1;
    logCtrl_IDFC.iLogPeriodTick = URnd(20)+1;
	
    //-- start logging
    testHelperLDD.MakeLogFromISR(rqStatLogFromISR, logCtrl_ISR);
    testHelperLDD.MakeLogFromDFC(rqStatLogFromDFC, logCtrl_DFC);
    testHelperLDD.MakeLogFromIDFC(rqStatLogFromIDFC, logCtrl_IDFC);
	
    //-- wait for logging to finish
    User::WaitForRequest(rqStatLogFromISR);
    User::WaitForRequest(rqStatLogFromDFC);
    User::WaitForRequest(rqStatLogFromIDFC);
	
    GetParseRecordData(aTrace);
	
	
    CleanupStack::PopAndDestroy(1); //-- testHelperLDD
	
}


//---------------------------------------------------------------------------------
//! @SYMTestCaseID          KBASE-T_PERFLOGGER-0056
//! @SYMTestType            UT
//! @SYMPREQ                PREQ1030
//! @SYMTestCaseDesc        Test of PERF_LOG0, PERF_LOG1, PERF_LOG macros performed by helper LDD
//! @SYMTestActions         Calls helper LDD API that in turn implies using PERF_LOG0, PERF_LOG1, PERF_LOG macros by helper LDD
//! @SYMTestExpectedResults return if the performace logger behaves as expected, panic otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------------------------------------------
void TestMacros(RBTrace &aTrace)
	{
    TInt    nRes;
    
    const TUint32   KLogUserData  = 0xBAAABEEF; //-- just for testing
    const TUint32   KLogUserData2 = 0x11FFDDCC; //-- just for testing
	
	
    test.Next(_L("Unit test for different macros\n"));
	
    RKPLoggerTestHelper testHelperLDD;  //-- helper test driver
    CleanupClosePushL(testHelperLDD);
	
    nRes = testHelperLDD.Open(TVersion(1,0,1)); //-- open test helper ldd for a logger
    test(nRes == KErrNone);
	
    //---
    TTestLogCtrl   logCtrl;
	
    //-- setup log control structures
    logCtrl.iLogsNum     = 1;
    logCtrl.iCategory    = BTrace::EKernPerfLog;
    logCtrl.iSubCategory = KLogSubCat_UserThread;
    logCtrl.iUserData    = KLogUserData;
    logCtrl.iUserData2   = KLogUserData2;
	
    //-- make logging from user thread using different macros, PERF_LOG0, PERF_LOG1, PERF_LOG 
    //-- see the helper driver
    nRes = testHelperLDD.TestDifferentMacros(logCtrl);
    test(nRes == KErrNone);
	
    //-- print out the results
    GetParseRecordData(aTrace);
	
	
    CleanupStack::PopAndDestroy(1); //-- testHelperLDD
	
	}

//---------------------------------------------------------------------------------
void MainL(void)
	{
    test.Start(_L("Kern Perf Logger tests"));
    Initialise();
	
	
    RBTrace trace;
	TInt error = trace.Open();
    test(error == KErrNone);
	
	trace.Empty();
	trace.SetFilter(BTrace::EThreadIdentification,0);
	
    
    
    //-- actually, for hardware platforms, the testing category and trace enabling 
    //-- may be set up in appropriate "header.iby" file
    trace.SetMode(RBTrace::EEnable);
    trace.SetFilter(BTrace::EKernPerfLog, ETrue);
    
    //-- unit-test for PERF_LOG macros
    TestMacros(trace);
	
    //-- functionality test
    TestPerfLogger(trace); 
	
    trace.Close();
	
    Finalise();
	test.End();
	}


//---------------------------------------------------------------------------------
//! @SYMTestCaseID          KBASE-T_PERFLOGGER-0162
//! @SYMTestType            UT
//! @SYMPREQ                PREQ1030
//! @SYMTestCaseDesc        Load test helper LDD and check the result
//! @SYMTestActions         load specified LDD by calling User::LoadLogicalDevice()
//! @SYMTestExpectedResults return if helper LDD is loaded OK, panic otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------------------------------------------
void Initialise()
	{
    rndSeed = Math::Random();
    
    //-- load device drivers 
    TInt nRes;
	
    test.Printf(_L("Loading test helper LDD...\n"));
    nRes = User::LoadLogicalDevice(KPLoggerHelperTestDrv);
    test(nRes == KErrNone || nRes == KErrAlreadyExists);
	}

/** Finalise environment */
void Finalise(void)
	{
	}



/**
    Main 	
*/
GLDEF_C TInt E32Main() 
	{
	CTrapCleanup* cleanup=CTrapCleanup::New() ; // get clean-up stack
    
	TRAPD(r,MainL());
	
    delete cleanup ; // destroy clean-up stack
	
    return r;
	
	}

//-------------------------------------------------------------------------------------

TUint URnd(TUint aMin, TUint aMax)
	{
    test(aMin < aMax);
    TUint uRes;
	
    do
		{
        uRes = Math::Rand(rndSeed) % aMax;
		}while(uRes < aMin);
		
		return uRes;
	}

TUint URnd(TUint aMax)
	{
    return Math::Rand(rndSeed) % aMax;
	}











