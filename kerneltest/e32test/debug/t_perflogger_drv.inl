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
//

/**
 @file
*/

#ifndef T_PERFLOGGER_DRV_INL__
#define T_PERFLOGGER_DRV_INL__


//-----------------------------------------------------------------------------------

TTestLogCtrl::TTestLogCtrl()
{
    iLogsNum = -1;       //-- invalid default value
    iLogPeriodTick = 10; //-- 10 ms by default

    iCategory=0x88;
    iSubCategory=0x99;
    
    iUserData  = 0xBEEEBEEE;
    iUserData2 = 0xAABBCCDD;
    
}

//-----------------------------------------------------------------------------------

#ifndef __KERNEL_MODE__

/**
    Open LDD.
    @param  aVer version required
    @return System-wide error code
*/
TInt RKPLoggerTestHelper::Open(const TVersion& aVer)
{ 
    return DoCreate(KPLoggerHelperTestDrv, aVer, KNullUnit, NULL, NULL, EOwnerProcess);
}

/**
    Cancel all pending requests and close LDD
*/
void RKPLoggerTestHelper::Close()
{
    CancelLogFromISR();
    CancelLogFromIDFC();
    CancelLogFromDFC();
    RBusLogicalChannel::Close();
}


/**
    @return Current LDD version    
*/
TVersion RKPLoggerTestHelper::VersionRequired() const
{ 
    return TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber);
}



/**
    Synchronous request to make a number of logging from user thread.
    Logging is performen in the context of the user caller thread

    @param  aLogCtrl    control parameters for logging
    @return             System wide error code
*/
TInt RKPLoggerTestHelper::MakeLogFromUserThread(const TTestLogCtrl& aLogCtrl)
{
    return DoControl(EDoLogFromUserThread, (TAny*)&aLogCtrl);
}


/**
    test different PERF_LOG macros, logging from the user thread. Just unit test
    @param  aLogCtrl    control parameters for logging
    @return             System wide error code
*/
TInt RKPLoggerTestHelper::TestDifferentMacros(const TTestLogCtrl& aLogCtrl)
{
    return DoControl(EDoTestMacros, (TAny*)&aLogCtrl);
}


/**
    Asynchronous request to make a number of logging from ISR. NTimer is used for this purpose.

    @param  aRqStat     request status, will be completed when all done.
    @param  aLogCtrl    control parameters for logging
*/
void RKPLoggerTestHelper::MakeLogFromISR(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl)
{
    DoRequest(EDoLogFromISR, aRqStat, (TAny*)&aLogCtrl);
}

/** cancel ISR logging request */
void RKPLoggerTestHelper::CancelLogFromISR()
{
    DoCancel(EDoLogFromISR);
}

/**
    Asynchronous request to make a number of logging from IDFC. NTimer is used for this purpose.

    @param  aRqStat     request status, will be completed when all done.
    @param  aLogCtrl    control parameters for logging
*/
void RKPLoggerTestHelper::MakeLogFromIDFC(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl)
{
    DoRequest(EDoLogFromIDFC, aRqStat, (TAny*)&aLogCtrl);
}

/** cancel IDFC logging request */
void RKPLoggerTestHelper::CancelLogFromIDFC()
{
    DoCancel(EDoLogFromIDFC);
}

/**
    Asynchronous request to make a number of logging from DFC. NTimer is used for this purpose.

    @param  aRqStat     request status, will be completed when all done.
    @param  aLogCtrl    control parameters for logging
*/
void RKPLoggerTestHelper::MakeLogFromDFC(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl)
{
    DoRequest(EDoLogFromDFC, aRqStat, (TAny*)&aLogCtrl);
}

/** cancel DFC logging request */
void RKPLoggerTestHelper::CancelLogFromDFC()
{
    DoCancel(EDoLogFromDFC);
}


#endif  //__KERNEL_MODE__



#endif  //T_PERFLOGGER_DRV_INL__




