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
// e32test\emi\d_emitest.cpp
// 
//

#include <kernel/kern_priv.h>
#include "d_emitest.h"
#include "d_emitest_dev.h"
#include <kernel/emi.h>

//
// DEMITestFactory
//

/*
  Standard export function for LDDs. This creates a DLogicalDevice derived object,
  in this case, our DEMITestFactory
*/
DECLARE_STANDARD_LDD()
	{
	return new DEMITestFactory;
	}

DEMITestFactory::DEMITestFactory()
	{
	}

DEMITestFactory::~DEMITestFactory()
	{
	}

/*
  Second stage constructor for DEMITestFactory.
  This must at least set a name for the driver object.

  @return KErrNone or standard error code.
*/
TInt DEMITestFactory::Install()
	{
	return SetName(&KEMITestName);
	}

/*
  Return the drivers capabilities.
  Called in the response to an RDevice::GetCaps() request.
  The thread is in a critical section.

  @param aDes Descriptor to write capabilities information into
*/
void DEMITestFactory::GetCaps(TDes8&) const
	{
	}

/*
  Called by the kernel's device driver framework to create a Logical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aChannel Set to point to the created Logical Channel

  @return KErrNone or standard error code.
*/
TInt DEMITestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DEMITestChannel;
	if(!aChannel)
		return KErrNoMemory;

	return KErrNone;
	}


//
// Logical Channel
//

DEMITestChannel::DEMITestChannel()
: iTagMaskDFC(TagMaskDFC,NULL,1)
	{
	// Get pointer to client threads DThread object
	iClient=&Kern::CurrentThread();
	// Open a reference on client thread so it's control block can't dissapear until
	// this driver has finished with it.
	((DObject*)iClient)->Open();
	}

DEMITestChannel::~DEMITestChannel()
	{
	// Stop EMI, incase it wannt stopped manually.
  	EMI::TaskEventLogging(EFalse,0,NULL,NULL);
  	EMI::SetMask(0);

	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/*
  Second stage constructor called by the kernel's device driver framework.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aVer The version argument supplied by the client to RBusLogicalChannel::DoCreate

  @return KErrNone or standard error code.
*/
TInt DEMITestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion&)
	{
	iTagMaskDFC.SetDfcQ(Kern::DfcQue0());
	return KErrNone;
	}

/* 
  Normal test monitors
*/
TInt MyThreadStartMonitor(NThread* aNThread)
	{
	TTaskEventRecord rec;
	rec.iType=128;
	rec.iPrevious=((TInt*) aNThread)+1;	// This stops the event getting killed on thread exit.
						// This means there is no garantee the thread will still exist when
						// the record is read. This is only safe here as the test code never 
						// attempts to derefrance this pointer.
	EMI::AddTaskEvent(rec);
	return 0;
	}


void MyThreadExitMonitor(NThread* aNThread)
	{
	TTaskEventRecord rec;
	rec.iType=129;
	rec.iPrevious=aNThread;
	EMI::AddTaskEvent(rec);
	}
/* 
  Stress test monitors
  
  Vems =	 1: Passed.
  			 0: No monitors called.
  			-1: Wrong Exit monitor callled.
  			-2: Exit call before StartMonitor
  			-3: Jibberish VEMs value. (or anything ending 5-9)
  			-4: Exit called multiple times, 1st time ok.
  			<-9: Exit called multiple times.  See last digit for status 1st time.
  			Stops couting after -1000, where its clearly very sick!
*/


TInt SoakStartMonitor1(NThread* aNThread)
	{
 	EMI::SetThreadVemsData(aNThread,(TAny*)1000);
 	return 0;
	}
	
TInt SoakStartMonitor2(NThread* aNThread)
	{
 	EMI::SetThreadVemsData(aNThread,(TAny*)2000);
 	return 0;
	}



inline void SoakExitMonitor(NThread* aNThread, TInt aOwner)
	{
	TInt val = (TInt) EMI::GetThreadVemsData(aNThread);
	
	if (val>-1)
		{
		TInt notOwner = (aOwner==1000?2000:1000);

		if (val==aOwner)
			EMI::SetThreadVemsData(aNThread,(TAny*)1);
		else if (val==notOwner)
			EMI::SetThreadVemsData(aNThread,(TAny*)-1);
		else if (val==0)
			EMI::SetThreadVemsData(aNThread,(TAny*)-2);
		else if (val==1)
			EMI::SetThreadVemsData(aNThread,(TAny*)-4);
		else
			EMI::SetThreadVemsData(aNThread,(TAny*)-3);
		}
	else
		{
		if (val>-1000)
			EMI::SetThreadVemsData(aNThread,(TAny*)(val-10));
		else
			EMI::SetThreadVemsData(aNThread,(TAny*)val);
		}
	}
	
void SoakExitMonitor1(NThread* aNThread)
	{
	SoakExitMonitor(aNThread,1000);
	}
void SoakExitMonitor2(NThread* aNThread)
	{
	SoakExitMonitor(aNThread,2000);
	}

/*
  Process synchronous requests
*/
TInt DEMITestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported; 
	TTaskEventRecord rec;
	
	switch (aFunction)
		{
		case REMITest::ETaskEventLogging:
			{
			
			NKern::ThreadEnterCS();
			
			TMonitors mon = (TMonitors) ((TInt)a1 >> 1);
			TBool logging = (TBool) ((TInt)a1 & 1);

			switch (mon)
				{
				case ENone: r = EMI::TaskEventLogging(logging,(TInt) a2,NULL,NULL);
				break;
				case ENormal: r = EMI::TaskEventLogging(logging,(TInt) a2,&MyThreadStartMonitor,&MyThreadExitMonitor);
				break;
				case EStressFirst:r = EMI::TaskEventLogging(logging,(TInt) a2,SoakStartMonitor1,SoakExitMonitor1);
				break;
				case EStressSecond:r = EMI::TaskEventLogging(logging,(TInt) a2,SoakStartMonitor2,SoakExitMonitor2);
				}
			NKern::ThreadLeaveCS();	
			return r;
			}
		case REMITest::EGetTaskEvent:
			r = (TInt) EMI::GetTaskEvent(rec);
			if (r)
				kumemput(a1,&rec,sizeof(TTaskEventRecord));
			return r;
		case REMITest::EAddTaskEvent:
			kumemget(&rec,a1,sizeof(TTaskEventRecord));
			return (TInt) EMI::AddTaskEvent(rec);
		case REMITest::EGetIdleThread:
			return (TInt) EMI::GetIdleThread(); 
		case REMITest::EGetSigmaThread:
			return (TInt) EMI::GetSigmaThread(); 
		case REMITest::ESetVEMSData:
			EMI::SetThreadVemsData((NThread*) a1,a2);
			return KErrNone;
		case REMITest::EGetVEMSData:
			return (TInt) EMI::GetThreadVemsData((NThread*) a1);
		case REMITest::ESetThreadLoggable:
			EMI::SetThreadLoggable((NThread*) a1,(TBool) a2);
			return KErrNone;
		case REMITest::EGetThreadLoggable:
			return (TInt) EMI::GetThreadLoggable((NThread*) a1);
		case REMITest::ESetThreadTag:
			EMI::SetThreadTag((NThread*) a1,(TUint32) a2);
			return KErrNone;
		case REMITest::EGetThreadTag:
			return (TInt) EMI::GetThreadTag((NThread*) a1);
		case REMITest::ESetMask:
			EMI::SetMask((TInt) a1);
			return KErrNone;
		case REMITest::EGetMask:
			return EMI::GetMask();			
		case REMITest::ESetDFC:
			EMI::SetDfc(&iTagMaskDFC, 0);
			return KErrNone;
		case REMITest::ESetState:
			EMI::SetState((TInt) a1);
			return KErrNone;
		case REMITest::EGetState:
			return EMI::GetState();
		case REMITest::EGetNThread:
			{
			DThread* myThread;
			TInt myNThread;
			NKern::LockSystem();
			
			myThread = (DThread* ) Kern::CurrentThread().ObjectFromHandle((TInt)a1,EThread);
			myNThread=  (TInt) (myThread==NULL?NULL:&myThread->iNThread);
			NKern::UnlockSystem();
			return myNThread;
			}
		case REMITest::EAfterIdle:
			EMI::AfterIdle((TInt) a1);			
			return KErrNone;
	
		default:
			return KErrNotSupported;
		}
	}
	
/*
  DFC callback which gets triggered when the EMI tag anded with thread mask is true.
  Sets EMI state to be the value returned from GetDfcTriggerTag.
*/
void DEMITestChannel::TagMaskDFC(TAny*)
	{
	EMI::SetState(EMI::GetDfcTriggerTag());
	EMI::SetMask(0);
	}
