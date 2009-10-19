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
// e32test\emi\d_emitest.h
// 
//

#ifndef __EMITEST_H__
#define __EMITEST_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


struct TUserTaskEventRecord
	{
	TUint8	iType;		//	Type of event, 	0 = Reschedule
						//	1..127 = Reserved
						//	> 127 = User Defined
	TUint8	iFlags;		//	Includes:	
						//Bit 0 - Events lost before this event. (All types) 
	 					//Bit 1 - Previous thread now waiting. (Reschedule only)
	TUint16	iExtra;		//This has no use in reschedule events, but may be used by other event types.
	TUint32	iUserState;	//The state variable at the time of the event, which will probably indicate the clock frequency at the time of the event.
	TUint32	iTime;		//Time that the event occurred.  Units defined by the GET_HIGH_RES_TICK macro.
	TAny*	iPrevious;	//	The NThread that was executing before the switch.
	TAny*	iNext;		//The NThread that was executing after the switch.
	
	};

const TInt KEMI_EventLost  =1;
const TInt KEMI_PrevWaiting=2;

_LIT(KEMITestName,"EMITEST");

enum TMonitors
	{
	ENone,
	ENormal,
	EStressFirst,
	EStressSecond
	};

class REMITest : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		ETaskEventLogging,
		EGetTaskEvent,
		EAddTaskEvent,
		EGetIdleThread,
		EGetSigmaThread,
		ESetVEMSData,
		EGetVEMSData,
		ESetThreadLoggable,
		EGetThreadLoggable,
		ESetThreadTag,
		EGetThreadTag,
		ESetMask,
		EGetMask,		
		ESetDFC,
		ESetState,
		EGetState,
		EGetNThread,
		EAfterIdle
		};

	enum TRequest
		{
		ENumRequests,
		EAllRequests = (1<<ENumRequests)-1
		};
		
 public:
	inline TInt Open();
	inline TInt TaskEventLogging(TBool, TInt, TMonitors);
	inline TBool GetTaskEvent(TUserTaskEventRecord&);
	inline TBool AddTaskEvent(TUserTaskEventRecord&);
	inline TAny* GetIdleThread();
	inline TAny* GetSigmaThread();
	inline void SetVEMSData(TAny* aThread, TAny* aData);
	inline TAny* GetVEMSData(TAny* aThread);
	inline void SetThreadLoggable(TAny* aThread, TBool aLoggable);
	inline TBool GetThreadLoggable(TAny* aThread);
	inline void SetThreadTag(TAny* aThread,TUint32 aTag);
	inline TUint32 GetThreadTag(TAny* aThread);
	inline void SetMask(TUint32 aMask);
	inline TUint32 GetMask();	
	inline void SetDfc();
	inline void SetState(TInt);
	inline TInt GetState();
	
	inline TAny* GetNThread(const RThread&);
	inline void AfterIdle(TInt);
	};

#ifndef __KERNEL_MODE__

inline TInt REMITest::Open()
	{ return DoCreate(KEMITestName,TVersion(1,0,1),KNullUnit,NULL,NULL); }

inline TInt REMITest::TaskEventLogging(TBool aLogging, TInt aSize, TMonitors aMon)
	{
	 TInt param= aLogging?1:0;
	 param |= (aMon<<1);	
	 return DoControl(ETaskEventLogging,(TAny*)param,(TAny*)aSize); }  

inline TBool REMITest::GetTaskEvent(TUserTaskEventRecord& aTask)
	{ return (TBool) DoControl(EGetTaskEvent,&aTask); }	

inline TBool REMITest::AddTaskEvent(TUserTaskEventRecord& aTask)
	{ return (TBool) DoControl(EAddTaskEvent,&aTask); }	

inline TAny* REMITest::GetIdleThread()
	{ return (TAny*) DoControl(EGetIdleThread,NULL); }	

inline TAny* REMITest::GetSigmaThread()
	{ return (TAny*) DoControl(EGetSigmaThread,NULL); }	

inline void REMITest::SetVEMSData(TAny* aThread, TAny* aData)
	{ DoControl(ESetVEMSData,aThread, aData); }		

inline TAny* REMITest::GetVEMSData(TAny* aThread)
	{ return (TAny*) DoControl(EGetVEMSData,aThread); } 

inline void REMITest::SetThreadLoggable(TAny* aThread, TBool aLoggable)
	{ DoControl(ESetThreadLoggable,aThread, (TAny*) aLoggable); }	

inline TBool REMITest::GetThreadLoggable(TAny* aThread)
	{ return (TBool) DoControl(EGetThreadLoggable,aThread); }	

inline void REMITest::SetThreadTag(TAny* aThread,TUint32 aTag)
	{ DoControl(ESetThreadTag,aThread, (TAny*) aTag); }	

inline TUint32 REMITest::GetThreadTag(TAny* aThread)
	{ return DoControl(EGetThreadTag,aThread); }	

inline void REMITest::SetMask(TUint32 aMask)
	{ DoControl(ESetMask,(TAny*) aMask); }

inline TUint32 REMITest::GetMask()
	{ return DoControl(EGetMask,NULL); }		

inline void REMITest::SetDfc()
	{ DoControl(ESetDFC,NULL); }	

inline void REMITest::SetState(TInt aState)
	{ DoControl(ESetState,(TAny*) aState); }	

inline TInt REMITest::GetState()
	{ return DoControl(EGetState,NULL); }

inline TAny* REMITest::GetNThread(const RThread& aRThread)
	{return (TAny*) DoControl(EGetNThread,(TAny*)aRThread.Handle());}

inline void REMITest::AfterIdle(TInt aDelay)
	{ DoControl(EAfterIdle,(TAny*)aDelay);}


#endif

#endif
