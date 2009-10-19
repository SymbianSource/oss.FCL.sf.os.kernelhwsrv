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
// A helper test driver for testing Kernel Performance Logger. User part.
// 
//

/**
 @file
 @internalAll
*/


#ifndef T_PERFLOGGER_DRV_H__
#define T_PERFLOGGER_DRV_H__

#include <e32cmn.h>
#include <e32ver.h>

/** Test helper LDD name */ 
_LIT(KPLoggerHelperTestDrv, "d_perflogger_test");


/** 
	Log control structure.
	Specifies a task arguments to the helper driver
*/
struct TTestLogCtrl
	{
    inline TTestLogCtrl();
	
    TInt    iLogsNum;        ///< numbers of loggings   
    TUint   iLogPeriodTick;  ///< Period of logging, nanokernel ticks, see NTimer::OneShot()
	
    //-- this part will go to the logging function
    
    TUint8  iCategory;
    TUint8  iSubCategory;
    TUint32 iUserData;       ///< user data.
    TUint32 iUserData2;      ///< user data.
	
	};


/**
    User - side interface to the perf. logger test helper LDD.
*/
class RKPLoggerTestHelper : public RBusLogicalChannel
	{
	
	public:
#ifndef __KERNEL_MODE__
		
		//-- user application interface methods
		
		inline TInt Open(const TVersion& aVer);
		inline TVersion VersionRequired() const;
		
		inline TInt MakeLogFromUserThread(const TTestLogCtrl& aLogCtrl);
		inline TInt TestDifferentMacros(const TTestLogCtrl& aLogCtrl);
		
		
		inline void MakeLogFromISR(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl);
		inline void CancelLogFromISR();
		
		inline void MakeLogFromIDFC(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl);
		inline void CancelLogFromIDFC();
		
		inline void MakeLogFromDFC(TRequestStatus& aRqStat, const TTestLogCtrl& aLogCtrl);
		inline void CancelLogFromDFC();
		
		inline void Close();
		
#endif  //__KERNEL_MODE__
		
	public:
		
		/** LDD version */
		enum TVer{EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=1};
		
		/** Synchronous Control functions */
		enum TControl
			{
			EDoLogFromUserThread,   ///< make logging from user thread context. See also TTestLogStruct
			EDoTestMacros           ///< test different PERF_LOG macros, logging from the user thread 
			};
		
		/** Asynchronous Request functions */
		enum TRequest
			{
			EDoLogFromISR,  ///< make logging from ISR. See also TTestLogStruct
			EDoLogFromIDFC, ///< make logging from IDFC. See also TTestLogStruct
			EDoLogFromDFC,  ///< make logging from DFC. See also TTestLogStruct
			};
	};

#include "t_perflogger_drv.inl"

#endif  //T_PERFLOGGER_DRV_H__




