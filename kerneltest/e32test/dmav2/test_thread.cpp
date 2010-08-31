/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Implementation of test_thread.h
*
*/
#include "test_thread.h"

TInt TTestRemote::iCount=0;

TInt TTestRemote::RunFunctor(TAny* aFunctor)
	{
	TFunctor& functor = *(TFunctor*)aFunctor;
	functor();
	return KErrNone;
	}

TTestThread::TTestThread(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume)
	{
	Init(aName, aFn, aData, aAutoResume);
	}

TTestThread::TTestThread(const TDesC& aName, TFunctor& aFunctor, TBool aAutoResume)
	{
	Init(aName, RunFunctor, &aFunctor, aAutoResume);
	}

TTestThread::~TTestThread()
	{
	iThread.Close();
	}

void TTestThread::Resume()
	{
	iThread.Resume();
	}

TInt TTestThread::WaitForExitL()
	{
	User::WaitForRequest(iLogonStatus);
	const TInt exitType = iThread.ExitType();
	const TInt exitReason = iThread.ExitReason();

	__ASSERT_ALWAYS(exitType != EExitPending, User::Panic(_L("TTestThread"),0));

	if(exitType != EExitKill)
		User::Leave(exitReason);

	return exitReason;
	}

void TTestThread::Rendezvous(TRequestStatus& aStatus)
	{
	iThread.Rendezvous(aStatus);
	}

void TTestThread::Init(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume)
	{
	TKName name(aName);
	name.AppendFormat(_L("-%d"), iCount++);
	TInt r=iThread.Create(name, aFn, KDefaultStackSize, KHeapSize, KHeapSize, aData);
	if(r!=KErrNone)
		{
		RDebug::Printf("RThread::Create failed, code=%d", r);
		User::Panic(KPanicCat, EThreadCreateFailed);
		}

	iThread.Logon(iLogonStatus);
	__ASSERT_ALWAYS(iLogonStatus == KRequestPending, User::Panic(_L("TTestThread"),0));

	if(aAutoResume)
		iThread.Resume();
	}


CTest::~CTest()
	{
	iName.Close();
	}

void CTest::SetupL()
	{
	}

void CTest::operator()()
	{
	for(TInt i=0; i<iIterations; i++)
		{
		RunTest();
		}
	}


void CTest::Announce() const
{
	RDebug::RawPrint(_L("Test: "));
	PrintTestInfo();
	RDebug::RawPrint(_L(": "));
	PrintTestType();
	RDebug::RawPrint(_L(": "));
	RDebug::RawPrint(iName);
	RDebug::RawPrint(_L(": "));
	RDebug::Printf("(%d iterations)", iIterations);
}


void CTest::PrintTestInfo() const
	{
	}
const TDesC& CTest::Name() const
	{
	return iName;
	}

CTest::CTest(const TDesC& aName, TInt aIterations)
	:iIterations(aIterations)
	{
	iName.CreateL(aName);
	}

CTest::CTest(const CTest& aOther)
	:iIterations(aOther.iIterations)
	{
	iName.CreateL(aOther.iName);
	}

void MultipleTestRun(RTest& test, const CTest& aTest, TInt aNumberOfThreads)
	{
	RPointerArray<CTest> testArray;
	RPointerArray<TTestThread> threadArray;

	for(TInt i=0; i<aNumberOfThreads; i++)
		{
		test.Next(_L("Create test thread"));
		CTest* newTest = aTest.Clone();
		test_NotNull(newTest);

		TTestThread* thread = new TTestThread(aTest.Name(), *newTest);
		test_NotNull(thread);

		threadArray.AppendL(thread);
		testArray.AppendL(newTest);
		}

	const TInt count = threadArray.Count();
	for(TInt j=0; j<count; j++)
		{
		TTestThread* thread = threadArray[0];
		
		TInt r = KErrNone;
		TRAPD(leaveCode, r = thread->WaitForExitL());
		if(leaveCode != KErrNone)
			{
			test.Printf(_L("Thread %d: Panic code:%d\n"), j, leaveCode);
			test_KErrNone(leaveCode);
			}

		if(r!=KErrNone)
			{
			test.Printf(_L("Thread Number %d\n"), j);
			test_KErrNone(r);
			}

		threadArray.Remove(0);
		delete thread;
		}
	threadArray.Close();

	testArray.ResetAndDestroy();
	testArray.Close();
	}

/**
Runs each CTest in aTests in its own thread
Returns once all threads have terminated
*/
void MultipleTestRun(const RPointerArray<CTest>& aTests)
	{
	RTest test(_L("CTest::MultipleTestRun"));
	RPointerArray<TTestThread> threads;

	const TInt count = aTests.Count();

	TInt i;
	for(i=0; i<count; i++)
		{
		_LIT(KDmaTestThread, "DMA-test-thread");
		TTestThread* thread = new TTestThread(KDmaTestThread, *aTests[i], EFalse);
		test_NotNull(thread);
		TInt r = threads.Append(thread);
		test_KErrNone(r);
		}

	for(i=0; i<count; i++)
		{
		threads[i]->Resume();
		}


	for(i=0; i<count; i++)
		{
		TInt r = threads[i]->WaitForExitL();
		test_KErrNone(r);
		}

	threads.ResetAndDestroy();
	test.Close();
	}
