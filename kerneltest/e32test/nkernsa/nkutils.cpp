// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\nkutils.cpp
// 
//

#include <nktest/nkutils.h>

extern TDfcQue* CleanupDfcQ;

class NThreadX : public NThread
	{
public:
	NThreadX();
	static void KillDfcFn(TAny*);
	static TDfc* ExitHandler(NThread* aThread);
	static void ExceptionHandler(TAny* aPtr, NThread* aThread);
	static void SignalSemaphoreOnExit(TAny* aP, NThread* aT, TInt aC);
public:
	TDfc iKillDfc;
	TExitFunc iExitFunc;
	TAny* iExitParam;
	};

extern const SNThreadHandlers ThreadHandlers =
	{
	&NThreadX::ExitHandler,
	NTHREAD_DEFAULT_STATE_HANDLER,
	&NThreadX::ExceptionHandler,
	0
	};

NThreadX::NThreadX()
	: iKillDfc(&KillDfcFn, this, 1), iExitFunc(0)
	{
	}

void NThreadX::KillDfcFn(TAny* a)
	{
	NThreadX* t = (NThreadX*)a;
	TExitFunc f = t->iExitFunc;
	TAny* p = t->iExitParam;
	if (f)
		(*f)(p, t, 1);
#ifdef __SMP__
	free((TAny*)t->iNThreadBaseSpare8);
#else
	free((TAny*)t->iSpare8);
#endif
	free((TAny*)t->iStackBase);
	free(t);
	if (f)
		(*f)(p, t, 2);
	}

TDfc* NThreadX::ExitHandler(NThread* aT)
	{
	NThreadX* t = (NThreadX*)aT;
	if (t->iExitFunc)
		(*t->iExitFunc)(t->iExitParam, t, 0);
	return &t->iKillDfc;
	}

extern "C" void ExcFault(TAny*);
void NThreadX::ExceptionHandler(TAny* aPtr, NThread*)
	{
	NKern::DisableAllInterrupts();
	ExcFault(aPtr);
	}

extern "C" unsigned int strlen(const char*);

NThread* CreateThread(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TBool aResume, TInt aTimeslice, TExitFunc aExitFunc, TAny* aExitParam, TUint32 aCpuAffinity, NThreadGroup* aGroup)
	{
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("CreateThread %s pri %d", aName, aPri));
	TInt nlen = (TInt)strlen(aName);
	NThreadX* t = new NThreadX;
	TAny* stack = malloc(KStackSize);
	memset(stack, 0xee, KStackSize);
	TAny* namebuf = malloc(nlen+1);
	memcpy(namebuf, aName, nlen+1);
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("CreateThread -> thread at %08x stack %08x", t, stack));

	SNThreadCreateInfo info;

	info.iFunction = aFunc;
	info.iStackBase = stack;
	info.iStackSize = KStackSize;
	info.iPriority = aPri;
	info.iTimeslice = aTimeslice;
	info.iAttributes = 0;
	info.iHandlers = &ThreadHandlers;
	info.iFastExecTable = 0;
	info.iSlowExecTable = 0;
	info.iParameterBlock = (const TUint32*)aParams;
	info.iParameterBlockSize = aPSize;
#ifdef __SMP__
	info.iCpuAffinity = aCpuAffinity;
	info.iGroup = aGroup;
#endif

	TInt r = NKern::ThreadCreate(t, info);
	__NK_ASSERT_ALWAYS(r==KErrNone);
#ifdef __SMP__
	t->iNThreadBaseSpare8 = (TUint32)namebuf;
#else
	t->iSpare8 = (TUint32)namebuf;
#endif
	t->iKillDfc.SetDfcQ(CleanupDfcQ);
	t->iExitFunc = aExitFunc;
	t->iExitParam = aExitParam;
	if (aResume)
		NKern::ThreadResume(t);
	return t;
	}

void NThreadX::SignalSemaphoreOnExit(TAny* aP, NThread* aT, TInt aC)
	{
	NFastSemaphore* s = (NFastSemaphore*)aP;
	(void)aT;
	if (aC==EAfterFree)
		NKern::FSSignal(s);
	}

NThread* CreateThreadSignalOnExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, NFastSemaphore* aExitSem, TUint32 aCpuAffinity, NThreadGroup* aGroup)
	{
	return CreateThread(aName, aFunc, aPri, aParams, aPSize, TRUE, aTimeslice, &NThreadX::SignalSemaphoreOnExit, aExitSem, aCpuAffinity, aGroup);
	}

NThread* CreateUnresumedThreadSignalOnExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, NFastSemaphore* aExitSem, TUint32 aCpuAffinity, NThreadGroup* aGroup)
	{
	return CreateThread(aName, aFunc, aPri, aParams, aPSize, FALSE, aTimeslice, &NThreadX::SignalSemaphoreOnExit, aExitSem, aCpuAffinity, aGroup);
	}

void CreateThreadAndWaitForExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, TUint32 aCpuAffinity, NThreadGroup* aGroup)
	{
	NFastSemaphore s(0);
	CreateThread(aName, aFunc, aPri, aParams, aPSize, TRUE, aTimeslice, &NThreadX::SignalSemaphoreOnExit, &s, aCpuAffinity, aGroup);
	NKern::FSWait(&s);
	}

TDfcQue* CreateDfcQ(const char* aName, TInt aPri, TUint32 aCpuAffinity, NThreadGroup* aGroup)
	{
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("CreateDfcQ %s pri %d cpu %08x", aName, aPri, aCpuAffinity));
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("NKern::CurrentThread() = %08x\n", NKern::CurrentThread()));
	TDfcQue* q = new TDfcQue;
	__KTRACE_OPT(KTHREAD,DEBUGPRINT("CreateDfcQ -> %08x", q));
	NThread* t = CreateThread(aName, &TDfcQue::ThreadFunction, aPri, q, 0, FALSE, KTimeslice, 0, 0, aCpuAffinity, aGroup);
	q->iThread = t;
	NKern::ThreadResume(t);
	return q;
	}

void killDfcFn(TAny* aPtr)
	{
	TDfcQue* q = (TDfcQue*)aPtr;
	delete q;
	NKern::Exit();
	}

void DestroyDfcQ(TDfcQue* aQ)
	{
	NFastSemaphore exitSem(0);
	TDfc killDfc(&killDfcFn, aQ, aQ, 0);
	NThreadX* t = (NThreadX*)aQ->iThread;
	t->iExitFunc = &NThreadX::SignalSemaphoreOnExit;
	t->iExitParam = &exitSem;
	killDfc.Enque();
	NKern::FSWait(&exitSem);
	}

#ifdef __SMP__
class NKTest
	{
public:
	static TInt FSWait(NFastSemaphore* aS, TUint32 aTimeout);
	};

TInt WaitWithTimeout(NFastSemaphore* aS, TUint32 aTimeout)
	{
	return NKTest::FSWait(aS, aTimeout);
	}

TInt NKTest::FSWait(NFastSemaphore* aS, TUint32 aTimeout)
	{
	NThreadBase* pC = NKern::LockC();
	pC->iWaitState.SetUpWait(NThreadBase::EWaitFastSemaphore, 0, aS, aTimeout);
	if (aS->Dec(pC))					// full barrier
		pC->iWaitState.CancelWait();	// don't have to wait
	else
		RescheduleNeeded();				// have to wait
	NKern::PreemptionPoint();
	TInt r = pC->iWaitState.iWtC.iRetVal;
	NKern::Unlock();
	return r;
	}
#else
TInt WaitWithTimeout(NFastSemaphore* aS, TUint32 aTimeout)
	{
	NThreadBase* pC = NKern::LockC();
	if (--aS->iCount < 0)
		{
		NKern::NanoBlock(aTimeout, NThreadBase::EWaitFastSemaphore, aS);
		}
	NKern::PreemptionPoint();
	TInt r = pC->iReturnValue;
	if (r == KErrNone)
		pC->Release(KErrNone);	// cancel the timer on normal completion
	NKern::Unlock();
	return r;
	}
#endif

void FMWaitFull(NFastMutex* aMutex)
	{
	NKern::Lock();
	aMutex->Wait();
	NKern::Unlock();
	}

void FMSignalFull(NFastMutex* aMutex)
	{
	NKern::Lock();
	aMutex->Signal();
	NKern::Unlock();
	}

void WaitForRequest(NRequestStatus& aStatus)
	{
	TInt n = -1;
	do	{
		++n;
		NKern::WaitForAnyRequest();
		} while (aStatus == KRequestPending);
	if (n > 0)
		NKern::ThreadRequestSignal(0, n);
	}


extern "C" {
void SpinWait(TUint32 aTicks)
	{
	TUint32 tc = NKern::TickCount() + aTicks;
	TUint32 x;
	do	{
		x = NKern::TickCount();
		} while (TInt(x-tc)<0);
	}
}




