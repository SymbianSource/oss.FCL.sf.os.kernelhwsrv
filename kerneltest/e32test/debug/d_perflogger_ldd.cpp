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
// A helper test driver for testing Kernel Performance Logger implementation.
// 
//

/**
 @file
*/


#include "d_perflogger_ldd.h"
#include <kernperflogger.h>

_LIT(KDFCThreadName,"D_PL_DFC_THREAD");
const TInt KDFCThreadPriority=27;

//-----------------------------------------------------------------------------------


DKPLoggerTestHelperLDD* DKPLoggerTestHelperLDD::pSelf = NULL;	//-- static pointer to the single instance of the logical channel class

//-----------------------------------------------------------------------------------

DLogTicker::DLogTicker() : iTimer(LogTimerCallback,this)  //-- initialize log events generator
	{
    iLogDFC = NULL;
    iRequest = NULL;
    iUserThreadContext = NULL;
	}

DLogTicker::~DLogTicker()
	{
    Cancel(); //-- cancel user request, DFC, timers etc.
	Kern::DestroyClientRequest(iRequest);
    delete  iLogDFC;
	}

//-----------------------------------------------------------------------------------

/**
	Construct ticker object. Creates appropriate TDfc object for dealing with IDFC or DFC

	@param  apUserThreadContext pointer to the user thread context where the request will be completed in
	@param  apDfcQ              pointer to the DFC queue this object will be using.
	@param  aLogContext         specfies the context (ISR, DFC or IDFC) the logging will be made from. Can be NKern::EIDFC, NKern::EThread, NKern::EInterrupt
*/
void DLogTicker::Construct(DThread* aUserThreadContext, TDfcQue* aDfcQ, NKern::TContext aLogContext)
	{
    __NK_ASSERT_DEBUG(aUserThreadContext && aDfcQ);
	
    iLogContext = aLogContext;
    if(aLogContext == NKern::EIDFC)
		{//-- we will deal with IDFC, create appropriate DFC object
        iLogDFC = new TDfc(LogDFC, this);
		}
    else
		{
		if(aLogContext == NKern::EThread || aLogContext == NKern::EInterrupt)
			{//-- we will deal with DFC or ISR
			iLogDFC = new TDfc(LogDFC, this, aDfcQ, 0);
			}
		else
			{//-- wrong value
			__PRINT("#KPLogTest:DLogTicker::Construct() wrong context request !");
			__NK_ASSERT_DEBUG(0);
			}
		}
		
	__NK_ASSERT_ALWAYS(iLogDFC);

	TInt r = Kern::CreateClientRequest(iRequest);
	__NK_ASSERT_ALWAYS(r == KErrNone);
		
	iUserThreadContext = aUserThreadContext; //-- store user thread context co complete requests correctly
//	iLogDFC->SetDfcQ(aDfcQ); //-- attach to the given DFC queue. !!!!DON'T DO THIS FOR IDFC!!!!!
	}

//-----------------------------------------------------------------------------------

/**
	Start the state machine by scheduling a DFC (or IDFC)

	@param  apLogControl    log parameters structure
	@param  apRqStat        pointer to the user request staus object that will be completed when all loggings done.
*/
void DLogTicker::Start(const TTestLogCtrl* aLogControl, TRequestStatus* aRqStat)
	{
    __NK_ASSERT_DEBUG(aLogControl && aRqStat && iLogDFC);
    kumemget32(&iLogControl, aLogControl, sizeof(TTestLogCtrl)); //-- copy control structure from the user side
	
	__NK_ASSERT_DEBUG(iLogControl.iLogsNum>=0);
	__PRINT1("#KPLogTest:DLogTicker::Start() for %d loggings",iLogControl.iLogsNum);

	if (iRequest->SetStatus(aRqStat) != KErrNone)
		{//-- current request is pending, panic client thread
        __PRINT("#KPLogTest:DLogTicker::Start() request is already pending !");
        Kern::PanicCurrentThread(KPLoggerHelperTestDrv, EReqAlreadyPending);
		}
	
    if(iLogContext == NKern::EIDFC)
		{//-- DLogTicker::LogDFC() will be called as IDFC 
        NKern::Lock();   
        iLogDFC->Add();  //-- start
        NKern::Unlock();
		}
    else
		{//-- DLogTicker::LogDFC() will be called as DFC 
        iLogDFC->Enque(); //-- start
		}
	
	}

//-----------------------------------------------------------------------------------

/**
	Complete the user request when all logging done.
	This can be called from 2 concurrent places - DFC & NTimer callback, either of them can complete the request

	@param aCompletionCode request completion code
*/
void DLogTicker::CompleteRequest(TInt aCompletionCode/*=KErrNone*/)
	{
	Kern::QueueRequestComplete(iUserThreadContext, iRequest, aCompletionCode);
	}

//-----------------------------------------------------------------------------------

/**
	Cancel everything
*/
void DLogTicker::Cancel(void)
	{
    CompleteRequest(KErrCancel); //-- cancel user request
	iLogControl.iLogsNum = 0;	 // Prevent DFC from restarting the timer
	iTimer.Cancel();    //-- cancel Timer
	iLogDFC->Cancel(); //-- cancel DFC
	}

//-----------------------------------------------------------------------------------

/**
	Ticker timer callback. Can be called in ISR or DFC context.
	If called in ISR context, makes logging, and schedules a DFC to complete the request if needed.
	If called in DFC context, makes logging for DFC (not for IDFC) mode and completes the request if needed.

	@param  apSelf pointer to the DLogTicker object
*/
void DLogTicker::LogTimerCallback(TAny* aSelf)
	{
    DLogTicker *pSelf = (DLogTicker*)aSelf;
    __NK_ASSERT_DEBUG(pSelf);
    
    TTestLogCtrl& logClontrol = pSelf->iLogControl;
    __NK_ASSERT_DEBUG(logClontrol.iLogsNum >=0);
	
    TInt context = NKern::CurrentContext();
    if(context == NKern::EInterrupt)
		{//-- This callback is from ISR
        
        //-- make logging from ISR context, category field is ignored, it will be FastTrace::EKernPerfLog
        PERF_LOG(logClontrol.iSubCategory, logClontrol.iUserData, logClontrol.iUserData2);
		
        //-- kick DFC, it will probaly complete the request. 
        pSelf->iLogDFC->Add();  
		}
    else
		{//-- this is a DFC callback, but the DFC object could also have been ceated as IDFC
		//-- complete user request here if necessarily.
        if(pSelf->iLogDFC->IsIDFC())
			{//-- the logging will be made in IDFC function.
            if(pSelf->iLogControl.iLogsNum == 0)
				{//-- all done, complete the request here, because we can't do in in IDFC
                pSelf->CompleteRequest();
				}
            else
				{//-- this callback came from IDFC object, kick IDFC in a special way.
                NKern::Lock();
                pSelf->iLogDFC->Add(); 
                NKern::Unlock();
				}
			}
        else
			{//-- this callback came from IDFC object, make logging from DFC context
            //-- category field is ignored, it will be FastTrace::EKernPerfLog
            PERF_LOG(logClontrol.iSubCategory, logClontrol.iUserData, logClontrol.iUserData2);    
			
            pSelf->iLogDFC->Enque(); //-- kick DFC 
			}
        
		}
	
	}

//-----------------------------------------------------------------------------------

/**
	Ticker DFC or IDFC function. Kicks the timer; 
	If is called as IDFC, makes logging in this context and schedules a timer callback in DFC context to complete the request.
	If is called as DFC, schedules timer callback in DFC or ISR context. 

	@param  apSelf pointer to the DLogTicker object
*/
void DLogTicker::LogDFC(TAny* aSelf)
	{
    DLogTicker *pSelf = (DLogTicker*)aSelf;
    __NK_ASSERT_DEBUG(pSelf);
	
    TInt context = NKern::CurrentContext();
    (void)context; //-- avoid warning in release mode
	
    if(pSelf->iLogControl.iLogsNum <= 0)
		{//-- complete user request, all done. The request can also be completed in LogTimerCallback()
		//-- in case if this is a IDFC and callback is a DFC
        pSelf->CompleteRequest();    
		}
    else
		{
        TTestLogCtrl& logClontrol = pSelf->iLogControl;
        logClontrol.iLogsNum--; //-- decrease remaining number of loggings
		
        if(pSelf->iLogDFC->IsIDFC())
			{//-- we are in IDFC context, make logging from here, timer callback won't be IDFC
            __NK_ASSERT_DEBUG(context == NKern::EIDFC);
			
            //-- category field is ignored, it will be FastTrace::EKernPerfLog
            PERF_LOG(logClontrol.iSubCategory, logClontrol.iUserData, logClontrol.iUserData2);
			
            //-- kick the timer to have a callback in a specified time in DFC context
            //-- timer's DFC will complete the request if necessarily.
            pSelf->iTimer.OneShot(logClontrol.iLogPeriodTick, ETrue); 
			}
        else
			{//-- we are in DFC context, kick the timer to have a callback in a specified time in ISR or DFC context
            pSelf->iTimer.OneShot(logClontrol.iLogPeriodTick, !(pSelf->iLogContext == NKern::EInterrupt));
			}
		}
	}


//-----------------------------------------------------------------------------------


//###################################################################################
//#            DKPLoggerTestHelperLDD   class implementation
//###################################################################################

DKPLoggerTestHelperLDD::DKPLoggerTestHelperLDD()

	{
    //-- store the pointer to the current thread for request completion from ISR->DFC
    iClientThread = &Kern::CurrentThread();
	
    __NK_ASSERT_DEBUG(iClientThread);
	
	//-- Open client's user thread, incrementing ref. counter 
	TInt nRes = iClientThread->Open();
	__NK_ASSERT_DEBUG(nRes == KErrNone);
	(void)nRes;//-- avoid warning in release mode


    //-- initialize DFC machinery
    //iDfcQ = Kern::DfcQue0();   //-- attach to the low priority DFC queue
	if (!iDfcQ)
 		{
 		TInt r = Kern::DynamicDfcQCreate(iDfcQ, KDFCThreadPriority, KDFCThreadName);
		if (r!= KErrNone)
			{
			return;
			}

#ifdef CPU_AFFINITY_ANY
		NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);			
#endif
 		}	
    
    iIsrLogTicker.Construct (iClientThread, iDfcQ, NKern::EInterrupt);//-- construct ISR log ticker
    iDfcLogTicker.Construct (iClientThread, iDfcQ, NKern::EThread);   //-- construct DFC log ticker
    iIDfcLogTicker.Construct(iClientThread, iDfcQ, NKern::EIDFC);     //-- construct IDFC log ticker
	}

//-----------------------------------------------------------------------------------

DKPLoggerTestHelperLDD::~DKPLoggerTestHelperLDD()
	{
    __PRINT("#KPLogTest:~DKPLoggerTestHelperLDD()");
	
	iClientThread->Close(NULL);

	if (iDfcQ)
		iDfcQ->Destroy();
		
	pSelf = NULL;  //-- clear the pointer to this class instance
	}

//-----------------------------------------------------------------------------------

/**
	static factory function for the LDD.

	@return pointer to the created (or existing) instance of the class
*/
DKPLoggerTestHelperLDD* DKPLoggerTestHelperLDD::CreateInstance()
	{
    __PRINT("#KPLogTest:DKPLoggerTestHelperLDD::CreateInstance()");
    
    //-- create LDD channel instance
    if(pSelf)
	    {//-- this is a singleton, can't have more than one instance
        __PRINT("#DKPLoggerTestHelperLDD::CreateInstance(): Attempt to create a second instance of a singleton!");
        return pSelf;
		}

	pSelf = new DKPLoggerTestHelperLDD;
	
    if(!pSelf)
		{//-- OOM 
        __PRINT("#KPLogTest:DKPLoggerTestHelperLDD::CreateInstance(): Unable to create class instance !");
		}
	
    return pSelf;
	}

//-----------------------------------------------------------------------------------

/**
	LDD second stage constructor
*/
TInt DKPLoggerTestHelperLDD::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	//-- check if the version aVer is supported
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	
    return KErrNone;
	}

//-----------------------------------------------------------------------------------

/**
	Requests processing function.
	@return     request processing error code.
*/
TInt DKPLoggerTestHelperLDD::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
    TInt nRes = KErrNone;
	
	if (aFunction == KMaxTInt)
		{//-- this is DoCancel()
		
        TUint reqMask = (TUint)a1;
        DoCancel(reqMask);
		return KErrNone;
		}
	
    if(aFunction < 0)
		{//-- This is DoRequest()
        
        //-- extract request parameters
        TRequestStatus* pRqStat=(TRequestStatus*)a1;
        
        TAny* params[2];
        kumemget32(params, a2, sizeof(params));
        
        nRes = DoRequest(~aFunction, pRqStat, params[0], params[1]); 
		}
    else
		{//-- This is DoControl()
        nRes = DoControl(aFunction, a1, a2);
		}
	
	
    return nRes;
	}

//-----------------------------------------------------------------------------------

/**
	Cancel outstanding request(s)
	@param  aReqNumber	request number to cancel
*/
void DKPLoggerTestHelperLDD::DoCancel(TUint aReqNumber)
	{
	
    switch (aReqNumber)
		{
		//-- cancel logging from ISR.
		case RKPLoggerTestHelper::EDoLogFromISR:  
			iIsrLogTicker.Cancel();
			break;
			
			//-- cancel logging from IDFC.
		case RKPLoggerTestHelper::EDoLogFromIDFC: 
			iIDfcLogTicker.Cancel();
			break;
			
			//-- cancel logging from DFC.
		case RKPLoggerTestHelper::EDoLogFromDFC:  
			iDfcLogTicker.Cancel();
			break;
			
		default:
			__PRINT1("#KPLogTest:DKPLoggerTestHelperLDD::DoCancel Cancelling a wrong request number:%d!", aReqMask);
			Kern::PanicCurrentThread(KPLoggerHelperTestDrv, EWrongRequest);
			break;
			
		}
	}

//-----------------------------------------------------------------------------------

/**
	Asynchronous request processing. 

	@param aFunction    request function number, see RKPLoggerTestHelper::TControl
  
    @param apRqStat     pointer to the user's request status object.
    @param apArg1       pointer to the 1st parameter in RKPLoggerTestHelper::DoRequest
    @param apArg2       pointer to the 2nd parameter in RKPLoggerTestHelper::DoRequest
	
	@return request scheduling result, system-wide error code.
	  
*/
TInt DKPLoggerTestHelperLDD::DoRequest(TInt aReqNumber, TRequestStatus* aRqStat, TAny* aArg1, TAny* /*apArg2*/)
	{
    switch (aReqNumber)
		{
		//-- make logging from ISR.
		case RKPLoggerTestHelper::EDoLogFromISR:  
			{   
			__PRINT("#KPLogTest: making loggings from ISR context");
			iIsrLogTicker.Start((const TTestLogCtrl*)aArg1, aRqStat);
			}
			break;
			
			//-- make logging from IDFC.
		case RKPLoggerTestHelper::EDoLogFromIDFC: 
			{   
			__PRINT("#KPLogTest: making loggings from IDFC context");			
			iIDfcLogTicker.Start((const TTestLogCtrl*)aArg1, aRqStat);
			}			
			break;
			
			//-- make logging from DFC.
		case RKPLoggerTestHelper::EDoLogFromDFC:  
			{   
			__PRINT("#KPLogTest: making loggings from DFC context");			
			iDfcLogTicker.Start((const TTestLogCtrl*)aArg1, aRqStat);
			}
			break;
			
			
		default:
			__PRINT1("#KPLogTest:DKPLoggerTestHelperLDD::DoRequest() Wrong request number:%d!", aReqNumber);
			Kern::PanicCurrentThread(KPLoggerHelperTestDrv, EWrongRequest);
			break;
		}
    
    return KErrNone;
	}

//-----------------------------------------------------------------------------------

/**
	Synchronous requests processing. 

	@param aFunction    request function number, see RKernPerfLogger::TControl
	@param apArg1       pointer to the 1st parameter in RKernPerfLogger::DoControl
	@param apArg2       pointer to the 2n parameter in RKernPerfLogger::DoControl 
  
    @return request processing result
*/
TInt DKPLoggerTestHelperLDD::DoControl(TInt aFunction, TAny* aArg1, TAny* /*apArg2*/)
	{
	
    switch (aFunction)
		{
        //-- make test logging from the user thread
        case RKPLoggerTestHelper::EDoLogFromUserThread:
			{    
            kumemget32(&iLogControlUserThread, aArg1, sizeof(iLogControlUserThread)); //-- copy control structure from the user side
            __PRINT1("#KPLogTest: making %d loggings from a user-thread context", iLogControlUserThread.iLogsNum);
            __NK_ASSERT_DEBUG(iLogControlUserThread.iLogsNum >=0 );
			
            //-- This context is actually, a user thread. Make logging from here
            for(TInt i=0; i<iLogControlUserThread.iLogsNum; ++i)
				{
                //-- category field is ignored, it will be FastTrace::EKernPerfLog
                PERF_LOG(iLogControlUserThread.iSubCategory, iLogControlUserThread.iUserData, iLogControlUserThread.iUserData2);
                
                NKern::Sleep(iLogControlUserThread.iLogPeriodTick);
				}
			}
			break;
			
		//-- unit test for different PERF_LOG macros
        case RKPLoggerTestHelper::EDoTestMacros:
			{
            kumemget32(&iLogControlUserThread, aArg1, sizeof(iLogControlUserThread)); //-- copy control structure from the user side
            __PRINT1("#KPLogTest: making %d loggings from a user-thread context, testing different macros", iLogControlUserThread.iLogsNum);
            __NK_ASSERT_DEBUG(iLogControlUserThread.iLogsNum >=0 );
			
            for(TInt i=0; i<iLogControlUserThread.iLogsNum; ++i)
				{
                PERF_LOG0(iLogControlUserThread.iSubCategory);
                PERF_LOG1(iLogControlUserThread.iSubCategory, iLogControlUserThread.iUserData);
                PERF_LOG (iLogControlUserThread.iSubCategory, iLogControlUserThread.iUserData, iLogControlUserThread.iUserData2);
                
                NKern::Sleep(iLogControlUserThread.iLogPeriodTick);
				}
			
			
			}
			break;
			
        default:
			__PRINT1("#KPLogTest:DKPLoggerTestHelperLDD::DoControl() Wrong function number:%d!", aFunction);
			Kern::PanicCurrentThread(KPLoggerHelperTestDrv, EWrongRequest);
			break;
			
		};
	
	
    return KErrNone;
	}



//###################################################################################
//#            LDD factory, DKPLoggerTestHelperLDDFactory class implementation 
//###################################################################################

DKPLoggerTestHelperLDDFactory::DKPLoggerTestHelperLDDFactory()
	{
    iUnitsMask = 0x00; //-- don't support units
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);	
	}

DKPLoggerTestHelperLDDFactory::~DKPLoggerTestHelperLDDFactory()
	{
    __PRINT("#KPLogTest:~DKPLoggerTestHelperLDDFactory()");
	}

//-----------------------------------------------------------------------------------

/**
	static factory function for the LDD factory.

	@return pointer to the created instance of the class
*/
DKPLoggerTestHelperLDDFactory* DKPLoggerTestHelperLDDFactory::CreateInstance()
	{
    __PRINT("#KPLogTest:DKPLoggerTestHelperLDDFactory::CreateInstance()");
    
    //-- create LDD factory
    DKPLoggerTestHelperLDDFactory* pSelf = new DKPLoggerTestHelperLDDFactory;
	
    if(!pSelf)
		{//-- OOM 
        __PRINT("#KPLogTest:DKPLoggerTestHelperLDDFactory::CreateInstance(): Unable to create class instance !");
		}
	
    return pSelf;
	}

//-----------------------------------------------------------------------------------

/**
*/
TInt DKPLoggerTestHelperLDDFactory::Install()
	{
    return SetName(&KPLoggerHelperTestDrv); // Set our name and return error code
	}

//-----------------------------------------------------------------------------------

/**
*/
void DKPLoggerTestHelperLDDFactory::GetCaps(TDes8& /*aDes*/) const
	{//-- not supported
	}

//-----------------------------------------------------------------------------------

/**
	LDD factory function. Creates LDD object.
	@param  aChannel    A pointer to an LDD channel object which will be initialised on return.
	@return KErrNone    if object successfully allocated, KErrNoMemory if not.
	@return KErrAlreadyExists	if the client tries to creae more than 1 instance of the channel

*/
TInt DKPLoggerTestHelperLDDFactory::Create(DLogicalChannelBase*& aChannel)
	{

	if(DKPLoggerTestHelperLDD::pSelf)
		{//-- channel is a singleton, can't have more than one instance
        __PRINT("#DKPLoggerTestHelperLDDFactory::Create: Attmpt to create another instance of the LDD!");
        return KErrAlreadyExists;
		}
    
	aChannel = DKPLoggerTestHelperLDD::CreateInstance();
    if(!aChannel)
        return KErrNoMemory;  
	
    return KErrNone;
	}


//-----------------------------------------------------------------------------------

/**
	"Standard LDD" entrypoint.
	Is called on CreateLogicalDevice() if the user calls LoadLogicalDevice(). Creates LDD factory.

	@return pointer to the LDD factory object.
*/
DECLARE_STANDARD_LDD()
	{
    DKPLoggerTestHelperLDDFactory* pLDDFactory = DKPLoggerTestHelperLDDFactory::CreateInstance();
    return  pLDDFactory;
	}





























