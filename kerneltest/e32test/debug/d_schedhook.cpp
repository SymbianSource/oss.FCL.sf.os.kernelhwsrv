// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_schedhook.cpp
// 
//

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include "platform.h"
#include <kernel/cache.h>
#include <arm.h>
#include "d_schedhook.h"

_LIT(KLddName,"D_SCHEDHOOK");

NThread* TestThread;
TInt TestCount;

const TInt KMajorVersionNumber = 0;
const TInt KMinorVersionNumber = 1;
const TInt KBuildVersionNumber = 1;

class DSchedhookTest;

class DSchedhookTestFactory : public DLogicalDevice
	{
public:
	DSchedhookTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DHwExcHandler : public DKernelEventHandler
	{
public:
	DHwExcHandler() : DKernelEventHandler(HandleEvent, this) {}
private:
	static TUint HandleEvent(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	};

class DSchedhookTest : public DLogicalChannelBase
	{
public:
	virtual ~DSchedhookTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
public:
	DHwExcHandler* iHwExcHandler;
	};

DECLARE_STANDARD_LDD()
	{
	return new DSchedhookTestFactory;
	}

//
// DSchedhookTestFactory
//

DSchedhookTestFactory::DSchedhookTestFactory()
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DSchedhookTestFactory::Create(DLogicalChannelBase*& aChannel)
/**
 Create a new DSchedhookTest on this logical device
*/
	{
	aChannel=new DSchedhookTest;
	return aChannel ? KErrNone : KErrNoMemory;
	}

TInt DSchedhookTestFactory::Install()
/**
 Install the LDD - overriding pure virtual
*/
	{
	return SetName(&KLddName);
	}

void DSchedhookTestFactory::GetCaps(TDes8& aDes) const
/**
 Get capabilities - overriding pure virtual
*/
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}

//
// DHwExcHandler
//

/**
 Handle exceptions on our test thread by suspending it
*/
TUint DHwExcHandler::HandleEvent(TKernelEvent aEvent, TAny* a1, TAny* /*a2*/, TAny* /*aThis*/)
	{
	if (aEvent == EEventHwExc)
		{
		if(&Kern::CurrentThread().iNThread!=TestThread)
			return ERunNext;
		TArmExcInfo *pR=(TArmExcInfo*)a1;
		pR->iR15+=4;
		Kern::ThreadSuspend(Kern::CurrentThread(),1);
		return (TUint)EExcHandled;
		}
	return (TUint)ERunNext;
	}

//
// DSchedhookTest
//

TInt DSchedhookTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
/**
 Create channel
*/
	{
	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	iHwExcHandler = new DHwExcHandler;
	if (! iHwExcHandler)
		return KErrNoMemory;
	return iHwExcHandler->Add();
	}

DSchedhookTest::~DSchedhookTest()
	{
	if (iHwExcHandler)
		iHwExcHandler->Close();
	}



//
// Scheduler Hook functions
//

NThread* CurrentThread=NULL;

void DisableRescheduleCallback()
/**
 Stops the Scheduler calling us on every context switch
*/
	{
	// Prevent rescheduling whilst we disable the callback
	NKern::Lock();
	// Disable Callback
	NKern::SetRescheduleCallback(NULL);
	// Invalidate CurrentThread
	CurrentThread = NULL;
	// Callback now disabled...
	NKern::Unlock();
	}



void RemoveSchedulerHooks()
/**
 Removes the patches added to the Scheduler code.
*/
	{
	// Make sure callback is disabled (required before removing hooks)
	DisableRescheduleCallback();
	// Get range of memory used by hooks
	TLinAddr start,end;
	NKern::Lock();
	NKern::SchedulerHooks(start,end);
	NKern::Unlock();
	// Free shadow pages which cover hooks
	TUint32 pageSize=Kern::RoundToPageSize(1);
	NKern::ThreadEnterCS();
	for(TLinAddr a=start; a<end; a+=pageSize)
		Epoc::FreeShadowPage(a);   // Ignore errors because we're trying to clean up anyway
	NKern::ThreadLeaveCS();
	}



TInt InsertSchedulerHooks()
/**
 Enables use of the Scheduler callback by using shadow pages to patch the Scheduler code.
*/
	{
	// Get range of memory used by hooks	
	TLinAddr start=0,end=0;
	NKern::Lock();
	NKern::SchedulerHooks(start,end);
	NKern::Unlock();
	if (start==0 && end==0) return KErrNotSupported;

	// Create shadow pages for hooks
	TUint32 pageSize=Kern::RoundToPageSize(1);
	for(TLinAddr a=start; a<end; a+=pageSize)
		{
		NKern::ThreadEnterCS();
		TInt r=Epoc::AllocShadowPage(a);
		NKern::ThreadLeaveCS();
		if(r!=KErrNone && r!=KErrAlreadyExists)
			{
			RemoveSchedulerHooks();
			return r;
			}
		}
	// Put hooks in
	NKern::Lock();
	NKern::InsertSchedulerHooks();
	NKern::Unlock();
	// Make I and D caches consistant for hook region
	Cache::IMB_Range(start,end-start);

	return KErrNone;
	}



void RescheduleTestFunction()
/**
 Test function to be called on each thread reschedule
*/
	{
	// Count rechedules to the test thread
	if(CurrentThread==TestThread)
		++TestCount;
	}



void RescheduleCallback(NThread* aNThread)
/**
 Main scheduler callback.
 Called with the Kernel Lock (Preemption Lock) held.
*/
	{
#ifndef __SMP__
	// The 'CurrentThread' is now unscheduled and has become the 'previous thread'
	// Set this thread 'UserContextType'...
	CurrentThread->SetUserContextType();
#endif
	// Make the newly scheduled thread the CurrentThread
	CurrentThread = aNThread;
	// Test function
	RescheduleTestFunction();
	}



void RescheduleCallbackFirst(NThread* aNThread)
/**
 Scheduler callback used once for initialisation.
 Called with the Kernel Lock (Preemption Lock) held.
*/
	{
	// Initialise CurrentThread
	CurrentThread = aNThread;
	// Switch future callbacks to the main RescheduleCallback
	NKern::SetRescheduleCallback(RescheduleCallback);
	// Test function
	RescheduleTestFunction();
	}



void EnableRescheduleCallback()
/**
 Sets the Scheduler to call us back on every thread reschedule
*/
	{
	// Reset the User Context Type for all threads, because these values
	// will be out-of-date. (It is our Rescheduler callback which set's them.
	// and we're just about enable that.)

	NKern::ThreadEnterCS();  // Prevent us from dying or suspending whilst holding a DMutex
	DObjectCon& threads=*Kern::Containers()[EThread];  // Get containing holding threads
	threads.Wait();  // Obtain the container mutex so the list does get changed under us

#ifndef __SMP__
	// For each thread...
	TInt c=threads.Count();
	for (TInt i=0; i<c; i++)
		((DThread*)threads[i])->iNThread.ResetUserContextType();
#endif

	threads.Signal();  // Release the container mutex
	NKern::ThreadLeaveCS();  // End of critical section

	// Ask for callback
	NKern::SetRescheduleCallback(RescheduleCallbackFirst);
	}



TInt GetThreadUserContext(NThread* aThread,TArmRegSet& aContext)
/**
 Test function to get a threads User Context
*/
	{
	// Get the User Context Type which our Reschedule hook set
	TInt type = aThread->iSpare3; /*iUserContextType*/

	// Get the table corresponding to the User Context Type
	const TArmContextElement* c = NThread::UserContextTables()[type];

	// Set the User Context by using the table we got
	TUint32* sp  = (TUint32*)aThread->iSavedSP;
	TUint32* st  = (TUint32*)((TUint32)aThread->iStackBase+(TUint32)aThread->iStackSize);
	TArmReg* out = (TArmReg*)(&aContext);
	TArmReg* end = (TArmReg*)(&aContext+1);
	do
		{
		TInt v = c->iValue;
		TInt t = c->iType;
		++c;
		if(t==TArmContextElement::EOffsetFromSp)
			v = sp[v];
		else if(t==TArmContextElement::EOffsetFromStackTop)
			v = st[-v];
		*out++ = v;
		}
	while(out<end);

	return type;
	}



void DumpContext(TArmRegSet& aContext,TInt aType)
	{
	Kern::Printf("  Context type %d",aType);
	Kern::Printf("  r0 =%08x r1 =%08x r2 =%08x r3 =%08x",aContext.iR0,aContext.iR1,aContext.iR2,aContext.iR3);
	Kern::Printf("  r4 =%08x r5 =%08x r6 =%08x r7 =%08x",aContext.iR4,aContext.iR5,aContext.iR6,aContext.iR7);
	Kern::Printf("  r8 =%08x r9 =%08x r10=%08x r11=%08x",aContext.iR8,aContext.iR9,aContext.iR10,aContext.iR11);
	Kern::Printf("  r12=%08x r13=%08x r14=%08x r15=%08x",aContext.iR12,aContext.iR13,aContext.iR14,aContext.iR15);
	Kern::Printf("  cpsr=%08x dacr=%08x",aContext.iFlags, aContext.iDacr);
	}



TInt TestGetThreadContext(DThread* aThread,TDes8* aContext)
	{
	TArmRegSet context1;
	TArmRegSet context2;

	memclr(&context1,sizeof(context1));
	memclr(&context2,sizeof(context2));
	NKern::Lock();
	TUint32 unused;
	NKern::ThreadGetUserContext(&aThread->iNThread,&context1,unused);
	TInt r=GetThreadUserContext(&aThread->iNThread,context2);
	NKern::Unlock();
	DumpContext(context1,-1);
	DumpContext(context2,r);

	TInt len,maxLen;
	Kern::KUDesInfo(*aContext,len,maxLen);
	if(maxLen>KMaxThreadContext)
		maxLen = KMaxThreadContext;
	TPtr8 ptr((TUint8*)&context1,maxLen,maxLen);
	Kern::KUDesPut(*aContext,ptr);

	for(TUint i=0; i<sizeof(context1); i++)
		if(((TUint8*)&context1)[i]!=((TUint8*)&context2)[i])
			return KErrGeneral;
	return r;
	}


DThread* ThreadFromId(TUint aId)
	{
	// This is risky because the thread could die on us an the DThread* become invalid.
	// We are relying on this never happening in our test code.
	NKern::ThreadEnterCS();  // Prevent us from dying or suspending whilst holding a DMutex
	DObjectCon& threads=*Kern::Containers()[EThread];  // Get containing holding threads
	threads.Wait();  // Obtain the container mutex so the list does get changed under us
	DThread* thread = Kern::ThreadFromId(aId);
	threads.Signal();  // Release the container mutex
	NKern::ThreadLeaveCS();  // End of critical section
	return thread;
	}

TInt DSchedhookTest::Request(TInt aFunction, TAny* a1, TAny* a2)
/**
 Handle requests from the test program
*/
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RSchedhookTest::EInstall:
			r=InsertSchedulerHooks();
			break;

		case RSchedhookTest::EUninstall:
			RemoveSchedulerHooks();
			return KErrNone;

		case RSchedhookTest::EInsertHooks:
			{
			NKern::Lock();
			NKern::InsertSchedulerHooks();
			TLinAddr start,end;
			NKern::SchedulerHooks(start,end);
			NKern::Unlock();
			Cache::IMB_Range(start,end-start);
			}
			return KErrNone;

		case RSchedhookTest::ERemoveHooks:
			{
			NKern::Lock();
			NKern::SetRescheduleCallback(NULL);
			NKern::RemoveSchedulerHooks();
			TLinAddr start,end;
			NKern::SchedulerHooks(start,end);
			NKern::Unlock();
			Cache::IMB_Range(start,end-start);
			}
			return KErrNone;

		case RSchedhookTest::EEnableCallback:
			NKern::Lock();
			NKern::SetRescheduleCallback(RescheduleCallbackFirst);
			NKern::Unlock();
			return KErrNone;

		case RSchedhookTest::EDisableCallback:
			DisableRescheduleCallback();
			return KErrNone;

		case RSchedhookTest::ESetTestThread:
			{
			NKern::ThreadEnterCS();  // Prevent us from dying or suspending whilst holding a DMutex
			DObjectCon& threads=*Kern::Containers()[EThread];  // Get containing holding threads
			threads.Wait();  // Obtain the container mutex so the list does get changed under us
			TestThread = &ThreadFromId((TUint)a1)->iNThread;
			threads.Signal();  // Release the container mutex
			NKern::ThreadLeaveCS();  // End of critical section
			TestCount = 0;
			}
			break;

		case RSchedhookTest::EGetThreadContext:
			return TestGetThreadContext(ThreadFromId((TUint)a1),(TDes8*)a2);

		case RSchedhookTest::EGetTestCount:
			return TestCount;

		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

