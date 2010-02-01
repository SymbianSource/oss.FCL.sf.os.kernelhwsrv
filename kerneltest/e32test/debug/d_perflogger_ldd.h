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
// A helper test driver for testing Kernel Performance Logger.
// 
//

/**
 @file
 @internalAll
*/

#ifndef D_PERFLOGGER_LDD_H__
#define D_PERFLOGGER_LDD_H__

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

#include "t_perflogger_drv.h"

//-- current LDD version
const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

//-----------------------------------------------------------------------------------
/** panic codes */
enum TPanic
	{
    EWrongRequest,      ///< wrong request number from the user side
		EReqAlreadyPending  ///< user asynchronous reques is already pending
	};

//-----------------------------------------------------------------------------------

/**
	Helper class that provides events generation in ISR, DFC or IDFC context
*/
class DLogTicker
	{
	public:
		
		DLogTicker();
		~DLogTicker();
		
		void Construct(DThread* apUserThreadContext, TDfcQue* apDfcQ, NKern::TContext aLogContext);
		void Start(const TTestLogCtrl* apLogControl, TRequestStatus* apRqStat);
		void Cancel(void);
		void CompleteRequest(TInt aCompletionCode=KErrNone);
		
	private:
		
		static void LogTimerCallback(TAny* apSelf);
		static void LogDFC(TAny* apSelf);
		
	protected:
		
		NTimer          iTimer;        ///< timer for generating events
		TTestLogCtrl    iLogControl;   ///< log control structure 
		TClientRequest *iRequest;      ///< encapsulates the client's request status that will be completed when the logging has finished
		TDfc           *iLogDFC;       ///< DFC for logging.
		
		DThread*        iUserThreadContext;///< thread context where the ipRqStat will be completed in
		NKern::TContext iLogContext;   ///< specifies the desirable context the logging function shall be called from (Kern thread, ISD, IDFC)
		
	};

//-----------------------------------------------------------------------------------

/**
	Logger test helper LDD class.
	Performs some actions from kernel side on behalf of the user test application.
	Used for testing kernel performance logger

	This is a singleton class, no more than 1 channel allowed
*/
class DKPLoggerTestHelperLDD : public DLogicalChannelBase
	{
	public:
		
		static DKPLoggerTestHelperLDD* CreateInstance();
		~DKPLoggerTestHelperLDD();
		
	protected:
		virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
		virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
		
		void DoCancel(TUint aReqNumber);
		TInt DoRequest(TInt aReqNumber, TRequestStatus* apRqStat, TAny* apArg1, TAny* apArg2);
		TInt DoControl(TInt aFunction,TAny* apArg1,TAny* apArg2);   
		
	private:
		DKPLoggerTestHelperLDD();
		
	private:
		TDynamicDfcQue*        iDfcQ;					///< pointer to the Kern::DfcQue0
		TTestLogCtrl    iLogControlUserThread;	///< log parameters for testting in User-thread context mode.
		DThread*        iClientThread;			///< pointer to the client thread for requests completion

	public:   
		
		
		DLogTicker      iIsrLogTicker;  ///< generates events in ISR context.
		DLogTicker      iDfcLogTicker;  ///< generates events in DFC context.
		DLogTicker      iIDfcLogTicker; ///< generates events in IDFC context.

		static DKPLoggerTestHelperLDD* pSelf;	///< static pointer to the single instance of this class

	};


//-----------------------------------------------------------------------------------


/**
	Logger test helper LDD factory class.
*/
class DKPLoggerTestHelperLDDFactory : public DLogicalDevice
	{
	public:
		
		static  DKPLoggerTestHelperLDDFactory* CreateInstance();
		~DKPLoggerTestHelperLDDFactory();
		
		// from DLogicalDevice
		virtual TInt Install();
		virtual void GetCaps(TDes8& aDes) const;
		virtual TInt Create(DLogicalChannelBase*& aChannel);
		
	private:
		
		DKPLoggerTestHelperLDDFactory();
	};


//-----------------------------------------------------------------------------------
//-- debug print macros. Debug print in this component is absolutely not important and exists only 
//-- for debugging purposes. So that, devising a mask for __KTRACE_OPT isn't necessary.
//-----------------------------------------------------------------------------------

//-- define this macro if you wish a debug trace from this driver.
//#define ENABLE_DEBUG_TRACE

#if defined(ENABLE_DEBUG_TRACE) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __PRINT(t)          { Kern::Printf(t);}
#define __PRINT1(t,a)       { Kern::Printf(t,a);}
#define __PRINT2(t,a,b)     { Kern::Printf(t,a,b);}
#define __PRINT3(t,a,b,c)   { Kern::Printf(t,a,b,c);}
#define __PRINT4(t,a,b,c,d) { Kern::Printf(t,a,b,c,d);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)
#define __PRINT4(t,a,b,c,d)
#endif

//-----------------------------------------------------------------------------------



#endif  //D_PERFLOGGER_LDD_H__




