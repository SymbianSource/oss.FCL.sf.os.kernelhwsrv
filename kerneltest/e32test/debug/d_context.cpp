// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_context.cpp
// Test LDD exercising hardware/software exception hooks 
// and get/set user context APIs.
// 
//

#include "platform.h"
#include <kernel/kern_priv.h>
#include "kern_test.h"
#include "d_context.h"

_LIT(KClientPanicCat, "D_CONTEXT");

extern TUint32 SpinInKernel(TBool);

DThread* ThreadFromId(TUint aId)
	{
	// This is risky because the thread could die on us an the DThread* become invalid.
	// We are relying on this never happening in our test code.
	NKern::ThreadEnterCS(); 
	DObjectCon& threads=*Kern::Containers()[EThread];
	threads.Wait(); 
	DThread* thread = Kern::ThreadFromId(aId);
	threads.Signal();
	NKern::ThreadLeaveCS(); 
	return thread;
	}

void ModifyContext(TArmRegSet& aContext)
	{
	TArmReg* end = (TArmReg*)(&aContext+1);
	for (TArmReg* p = (TArmReg*)&aContext; p<end; ++p)
		*p = ~*p;
	}

void DumpContext(TArmRegSet& aContext)
	{
	Kern::Printf("  r0 =%08x r1 =%08x r2 =%08x r3 =%08x",aContext.iR0,aContext.iR1,aContext.iR2,aContext.iR3);
	Kern::Printf("  r4 =%08x r5 =%08x r6 =%08x r7 =%08x",aContext.iR4,aContext.iR5,aContext.iR6,aContext.iR7);
	Kern::Printf("  r8 =%08x r9 =%08x r10=%08x r11=%08x",aContext.iR8,aContext.iR9,aContext.iR10,aContext.iR11);
	Kern::Printf("  r12=%08x r13=%08x r14=%08x r15=%08x",aContext.iR12,aContext.iR13,aContext.iR14,aContext.iR15);
	Kern::Printf("  cpsr=%08x dacr=%08x",aContext.iFlags, aContext.iDacr);
	}

inline TBool IsRegisterAvailable(TInt aReg, TUint32 aMask)
	{
	return aMask & (1<<aReg);
	}

TInt CheckSetContext(const TArmRegSet& aSetContext, const TArmRegSet& aContextAfterSet, TUint32 aAvailMask)
	{
	Kern::Printf("Checking all available registers have been modified (0%08x)", aAvailMask);

	const TArmReg* pSet = (const TArmReg*)&aSetContext;
	const TArmReg* pAfterSet = (const TArmReg*)&aContextAfterSet;

	for (int i=0; i<=EArmPc; ++i)
		{
		if (pAfterSet[i] == 0 && IsRegisterAvailable(i, aAvailMask) && pSet[i] != 0)
			{
			Kern::Printf("Register %d not set (expected %08x actual %08x)", i, pSet[i], pAfterSet[i]);
			return KErrCorrupt;
			}
		if (pAfterSet[i] != 0 && ! IsRegisterAvailable(i, aAvailMask))
			{
			Kern::Printf("Register %d incorrectly set (expected %08x actual %08x)", i, 0, pAfterSet[i]);
			return KErrCorrupt;
			}
		}

	return KErrNone;
	}

//////////////////////////////////////////////////////////////////////////////

class DEventHandler : public DKernelEventHandler
	{
public:
	DEventHandler();
	TInt Create(DLogicalDevice* aDevice);
	~DEventHandler();
	void Cancel();
private:
	static TUint EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	TUint HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2);
	TBool HandleException(TRequestStatus*& aStatusPtr, TClientRequest*& aRequestPtr);
	void HandleThreadDeath();
	void Cleanup();
private:
	DThread* iClientThread;
	DMutex* iLock; // serialise calls to handler
public:
	DLogicalDevice* iDevice;	// open reference to LDD for avoiding lifetime issues
	// software exception fields
	TExcType iSwExcLastType;
	TInt* iSwExcCounterPtr;
	TRequestStatus* iSwExcStatusPtr;
	TClientRequest* iClientRequestSwExc;
	// hardware exception fields
	TRequestStatus* iHwExcStatusPtr;
	TClientRequest* iClientRequestHwExc;
	// fields used for both hardware and software exceptions
	TUint iExcThreadId;
	TAny* iExcContextPtr;
	TBool iExcKillThread;
	// thread death event fields
	TUint iDeathThreadId;
	TRequestStatus* iDeathStatusPtr;
	TAny* iDeathContextPtr;
	TClientRequest* iClientRequestDeath;
	};


DEventHandler::DEventHandler() 
	: 	DKernelEventHandler(EventHandler, this) 
	{
	}


TInt DEventHandler::Create(DLogicalDevice* aDevice)
	{
	TInt r;
	r = aDevice->Open();
	if (r != KErrNone)
		goto error;
	iDevice = aDevice;
	iClientThread = &Kern::CurrentThread();
	r = Kern::CreateClientRequest(iClientRequestSwExc);
	if (r != KErrNone)
		goto error;
	r = Kern::CreateClientRequest(iClientRequestHwExc);
	if (r != KErrNone)
		goto error;
	r = Kern::CreateClientRequest(iClientRequestDeath);
	if (r != KErrNone)
		goto error;
	r = Kern::MutexCreate(iLock, _L("EventHandlerLock"), KMutexOrdDebug);
	if (r != KErrNone)
		goto error;
	return Add();
error:
	Cleanup();
	return r;
	}

void DEventHandler::Cleanup()
	{
	if (iLock)
		iLock->Close(NULL);
	if (iDevice)
		iDevice->Close(NULL);
	if (iClientRequestSwExc)
		{
		Kern::DestroyClientRequest(iClientRequestSwExc);
		iClientRequestSwExc = NULL;
		}
	if (iClientRequestHwExc)
		{
		Kern::DestroyClientRequest(iClientRequestHwExc);
		iClientRequestHwExc = NULL;
		}
	if (iClientRequestDeath)
		{
		Kern::DestroyClientRequest(iClientRequestDeath);
		iClientRequestDeath = NULL;
		}
	}

DEventHandler::~DEventHandler()
	{
	Cleanup();
	}


void DEventHandler::Cancel()
	{
	Kern::MutexWait(*iLock);
	if (iHwExcStatusPtr)
		{
		Kern::QueueRequestComplete(iClientThread, iClientRequestHwExc, KErrCancel);
		iHwExcStatusPtr = NULL;
		}
	if (iSwExcStatusPtr)
		{
		Kern::QueueRequestComplete(iClientThread, iClientRequestSwExc, KErrCancel);
		iSwExcStatusPtr = NULL;
		}
	Kern::MutexSignal(*iLock);
	}


TUint DEventHandler::EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis)
	{
	return ((DEventHandler*)aThis)->HandleEvent(aEvent, a1, a2);
	}


// called in thread CS
TUint DEventHandler::HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2)
	{
	// Ensure handler always called in thread critical section
	NThread& nt = Kern::CurrentThread().iNThread;
	__ASSERT_ALWAYS(nt.iCsCount != 0, (NKern::Lock(),*(TInt*)0xfaece5=0));

	TUint r = DKernelEventHandler::ERunNext;

	switch (aType)
		{
	case EEventSwExc:
		// HACK, EVIL UNSAFE MEMORY ACCESS FOLLOWS...
		TInt counter;
		// folowing will kill system if memory is bad (because we're in a critical section)
		umemget32(&counter, iSwExcCounterPtr, sizeof(TInt*));
		++counter;
		umemput32(iSwExcCounterPtr, &counter, sizeof(TInt));

		Kern::MutexWait(*iLock);
		iSwExcLastType = (TExcType)(TInt)a1;
		if (iSwExcStatusPtr)
			HandleException(iSwExcStatusPtr, iClientRequestSwExc);
		Kern::MutexSignal(*iLock);
		break;
	case EEventHwExc:
		Kern::MutexWait(*iLock);
		if (iHwExcStatusPtr)
			if (HandleException(iHwExcStatusPtr, iClientRequestHwExc))
				r |= (TUint)EExcHandled;
		Kern::MutexSignal(*iLock);
		break;
	case EEventKillThread:
		Kern::MutexWait(*iLock);
		HandleThreadDeath();
		Kern::MutexSignal(*iLock);
		break;
	default:
		// NO-OP
		break;
		}
	
	return r;
	}


// called in thread CS
TBool DEventHandler::HandleException(TRequestStatus*& aStatusPtr, TClientRequest*& aRequestPtr)
	{
	DThread& t = Kern::CurrentThread();
	TBool handled = EFalse;

	if (iExcThreadId == t.iId)
		{
		Kern::Printf("Trapped exception");
		TArmRegSet context1;
		TUint32 availmask;
		NKern::ThreadGetUserContext(&t.iNThread, &context1, availmask);
		XTRAPD(r, XT_DEFAULT, umemput(iExcContextPtr, &context1, sizeof(context1)));

		if (r == KErrNone)
			{
			if (iExcKillThread)
				{
				// We must preserve PC for software exceptions because execution
				// goes back user-side and only then is the thread panicked.
				TArmReg r15usr = context1.iR15;
				ModifyContext(context1);
				context1.iR15 = r15usr;
				NKern::ThreadSetUserContext(&t.iNThread, &context1);

				TArmRegSet context2;
				memclr(&context2, sizeof context2);
				NKern::ThreadGetUserContext(&t.iNThread, &context2, availmask);
				r = CheckSetContext(context1, context2, availmask);
				}
			else
				{
				Kern::ThreadSuspend(t, 1);
				handled = ETrue;
				}
			}
		Kern::QueueRequestComplete(iClientThread, aRequestPtr, r);
		aStatusPtr = NULL;
		}
	return handled;
	}


// called in thread CS
void DEventHandler::HandleThreadDeath()
	{
	DThread& t = Kern::CurrentThread();
	if (iDeathStatusPtr && iDeathThreadId == t.iId)
		{
		Kern::Printf("Trapping thread death: %O", &t);
		TArmRegSet context;
		TUint32 unused;
		NKern::ThreadGetUserContext(&t.iNThread, &context, unused);
		XTRAPD(r, XT_DEFAULT, umemput(iDeathContextPtr, &context, sizeof(context)));
		Kern::QueueRequestComplete(iClientThread, iClientRequestDeath, r);
		iDeathStatusPtr = NULL;
		}
	}

//////////////////////////////////////////////////////////////////////////////

class DTestChannel : public DLogicalChannelBase
	{
public:
	virtual ~DTestChannel();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt SetAndGetBackContext(TUint aId, TAny* aContext);
	TInt SetAndGetFullContext(TUint aId, TAny* aContext);
	void GetContext(TUint aId, TAny* aContext);
	void GetKernelContext(TUint aId, TAny* aContext);
	void AddUserCallback(TUint aId, TUserCallbackState aCallback);
private:
	DEventHandler* iHandler;
	};


// called in thread critical section
TInt DTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

// called in thread critical section
DTestChannel::~DTestChannel()
	{
	if (iHandler)
		{
		iHandler->Cancel();
		iHandler->Close();
		}
	}

TInt DTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	RContextLdd::TTrapInfo info;
	DThread* pT;
	TInt r = KErrNone;
	switch (aFunction)
		{
	case RContextLdd::EHook:
		NKern::ThreadEnterCS();
		iHandler = new DEventHandler;
		if (!iHandler)
			r = KErrNoMemory;
		else
			{
			r = iHandler->Create(iDevice);
			iHandler->iSwExcCounterPtr = (TInt*)a1;
			}
		NKern::ThreadLeaveCS();
		break;
	case RContextLdd::EGetLastExc:
		r = iHandler->iSwExcLastType;
		break;
	case RContextLdd::ETrapNextSwExc:
	case RContextLdd::ETrapNextHwExc:
		umemget(&info, a1, sizeof(info));
		if (aFunction == RContextLdd::ETrapNextHwExc)
			{
			__ASSERT_ALWAYS(iHandler->iSwExcStatusPtr==NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			iHandler->iHwExcStatusPtr = info.iStatusPtr;
			r = iHandler->iClientRequestHwExc->SetStatus(iHandler->iHwExcStatusPtr);
			__ASSERT_ALWAYS(r==KErrNone, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			}
		else
			{
			__ASSERT_ALWAYS(iHandler->iHwExcStatusPtr==NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			iHandler->iSwExcStatusPtr = info.iStatusPtr;
			r = iHandler->iClientRequestSwExc->SetStatus(iHandler->iSwExcStatusPtr);
			__ASSERT_ALWAYS(r==KErrNone, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			}
		iHandler->iExcThreadId = info.iThreadId;
		iHandler->iExcContextPtr = info.iContextPtr;
		iHandler->iExcKillThread = info.iKillThread;
		break;
	case RContextLdd::ETrapNextDeath:
		umemget(&info, a1, sizeof(info));
		iHandler->iDeathThreadId = info.iThreadId;
		iHandler->iDeathContextPtr = info.iContextPtr;
		iHandler->iDeathStatusPtr = info.iStatusPtr;
		r = iHandler->iClientRequestDeath->SetStatus(iHandler->iDeathStatusPtr);
		__ASSERT_ALWAYS(r==KErrNone, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
		break;
	case RContextLdd::EGetContext:
		GetContext((TUint)a1, a2);
		break;
	case RContextLdd::ESetGetContext:
		r = SetAndGetBackContext((TUint)a1, a2);
		break;
	case RContextLdd::ESetGetFullContext:
		r = SetAndGetFullContext((TUint)a1, a2);
		break;
	case RContextLdd::EGetKernelContext:
		GetKernelContext((TUint)a1, a2);
		break;
	case RContextLdd::ESpinInKernel:
		r = SpinInKernel((TBool)a1);
		break;
	case RContextLdd::EAddUserCallback:
		AddUserCallback((TUint)a1, (TUserCallbackState)(TUint)a2);
		break;
	case RContextLdd::EResumeTrappedThread:
		pT = ThreadFromId((TUint)a1);
		Kern::ThreadResume(*pT);
		break;
	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		break;
		}
	return r;
	}


TInt DTestChannel::SetAndGetBackContext(TUint aId, TAny* aContext)
	{
	DThread* pT = ThreadFromId(aId);
	__ASSERT_ALWAYS(pT!=NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));

	// The following code assumes the inspected thread does not run between the
	// set context and get context call.

	TArmRegSet context1;
	umemget(&context1, aContext, sizeof context1);
	NKern::ThreadSetUserContext(&pT->iNThread, &context1);

	TArmRegSet context2;
	memclr(&context2, sizeof context2);
	TUint32 availmask;
	NKern::ThreadGetUserContext(&pT->iNThread, &context2, availmask);
	umemput(aContext, &context2, sizeof context2);

	return CheckSetContext(context1, context2, availmask);
	}

#ifdef __SMP__
class NKTest
	{
public:
	static TBool IsDead(NThreadBase* aT)
		{ return aT->iWaitState.ThreadIsDead(); }
	};
#endif

TInt DTestChannel::SetAndGetFullContext(TUint aId, TAny* aContext)
	{
	DThread* pT = ThreadFromId(aId);
	__ASSERT_ALWAYS(pT != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	TBool dead;
#ifdef __SMP__
	dead = NKTest::IsDead(&pT->iNThread);
#else
	dead = pT->iNThread.iSpare1 == NThreadBase::EDead;
#endif
	__ASSERT_ALWAYS(dead, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));

	TArmFullContext contextData;
	umemget(&contextData, aContext, sizeof contextData);
	NKern::ThreadSetUserContext(&pT->iNThread, &contextData.iUserContext);

	NKern::ThreadGetUserContext(&pT->iNThread, &contextData.iUserContext, contextData.iUserAvail);
	NKern::ThreadGetSystemContext(&pT->iNThread, &contextData.iSystemContext, contextData.iSystemAvail);

	umemput(aContext, &contextData, sizeof contextData);

	return KErrNone;
	}

void DTestChannel::GetContext(TUint aId, TAny* aContext)
	{
	DThread* pT = ThreadFromId(aId);
	__ASSERT_ALWAYS(pT!=NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));

	TArmRegSet context;
	memclr(&context, sizeof context);
	TUint32 unused;
	NKern::ThreadGetUserContext(&pT->iNThread, &context, unused);
	umemput(aContext, &context, sizeof context);
	}

void DTestChannel::GetKernelContext(TUint aId, TAny* aContext)
	{
	DThread* pT = ThreadFromId(aId);
	__ASSERT_ALWAYS(pT!=NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));

	TArmRegSet context;
	memclr(&context, sizeof context);
	TUint32 unused;
	NKern::ThreadGetSystemContext(&pT->iNThread, &context, unused);
	umemput(aContext, &context, sizeof context);
	}

void DTestChannel::AddUserCallback(TUint aId, TUserCallbackState aCallback)
	{
	DThread* pT = ThreadFromId(aId);
	__ASSERT_ALWAYS(pT!=NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));

	switch (aCallback)
		{
	case ESpinningCallback:
		KernTest::Test(KernTest::EUserModeCallbackSpin, pT);
		break;
	case ESleepingCallback:
		KernTest::Test(KernTest::EUserModeCallbackSleep, pT);
		break;
	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		}
	}

//////////////////////////////////////////////////////////////////////////////

class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iVersion = RContextLdd::Version();
    // iParseMask = 0; // no unit, no info, no PDD
    // iUnitsMask = 0; // only one thing
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel=new DTestChannel;
	return aChannel ? KErrNone : KErrNoMemory;
    }

TInt DTestFactory::Install()
    {
    return SetName(&KTestLddName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
