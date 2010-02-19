// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Description: Some helper classes to assist with writing multi-threaded tests  

#ifndef TEST_THREAD_H
#define TEST_THREAD_H

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32des8.h>
#include <e32des8_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32std.h>
#include <e32std_private.h>
    

_LIT(KPciPanicCat, "test_thread.h");

static const TInt KPciHeapSize=0x2000;

enum TPciPanicCode
	{
	EThreadCreateFailed
	};

/**
A utility class for running functions in other threads/processes
*/
class TTestRemote
	{
public:
	virtual TInt WaitForExitL() = 0;
	virtual ~TTestRemote()
		{}

	virtual void Rendezvous(TRequestStatus& aStatus) = 0;

protected:
	TTestRemote()
		{}

	static TInt RunFunctor(TAny* aFunctor)
		{
		TFunctor& functor = *(TFunctor*)aFunctor;
		functor();
		return KErrNone;
		}

	TRequestStatus iLogonStatus;
	static TInt iCount;
	};
TInt TTestRemote::iCount=0;

class TTestThread : public TTestRemote
	{
public:
	TTestThread(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume=ETrue)
		{
		Init(aName, aFn, aData, aAutoResume);
		}

	/**
	Run aFunctor in another thread
	*/
	TTestThread(const TDesC& aName, TFunctor& aFunctor, TBool aAutoResume=ETrue)
		{
		Init(aName, RunFunctor, &aFunctor, aAutoResume);
		}

	~TTestThread()
		{
		//RTest::CloseHandleAndWaitForDestruction(iThread);
		iThread.Close();
		}

	void Resume()
		{
		iThread.Resume();
		}

	/**
	If thread exited normally, return its return code
	Otherwise, leave with exit reason
	*/
	virtual TInt WaitForExitL()
		{
		User::WaitForRequest(iLogonStatus);
		const TInt exitType = iThread.ExitType();
		const TInt exitReason = iThread.ExitReason();

		__ASSERT_ALWAYS(exitType != EExitPending, User::Panic(_L("TTestThread"),0));

		if(exitType != EExitKill)
			User::Leave(exitReason);

		return exitReason;
		}

	virtual void Rendezvous(TRequestStatus& aStatus)
		{
		iThread.Rendezvous(aStatus);
		}

private:
	void Init(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume)
		{
		TKName name(aName);
		name.AppendFormat(_L("-%d"), iCount++);	
		TInt r=iThread.Create(name, aFn, KDefaultStackSize, KPciHeapSize, KPciHeapSize, aData);
		if(r!=KErrNone)
			{
			RDebug::Printf("RThread::Create failed, code=%d", r);
			User::Panic(KPciPanicCat, EThreadCreateFailed);
			}
		
		iThread.Logon(iLogonStatus);
		__ASSERT_ALWAYS(iLogonStatus == KRequestPending, User::Panic(_L("TTestThread"),0));

		if(aAutoResume)
			iThread.Resume();
		}

	RThread iThread;
	};

class CTest : public CBase, public TFunctor
	{
public:
	~CTest()
		{
		iName.Close();
		}

	virtual void operator()()
		{
		RTest test(iName);
		test.Start(iName);
		for(TInt i=0; i<iIterations; i++)
			{
			test.Next(iName);
			RunTest();
			}
		test.End();
		}

	virtual void RunTest() = 0; 

	virtual CTest* Clone() const = 0;

	const TDesC& Name() const
		{
		return iName;
		}

protected:
	CTest(const TDesC& aName, TInt aIterations)
		:iIterations(aIterations)
		{
		iName.CreateL(aName);
		}


	
	CTest(const CTest& aOther)
		:iIterations(aOther.iIterations)
		{
		iName.CreateL(aOther.iName);
		}

	//It would be useful to have an RTest member, but this can't be
	//initialised untill the new thread is running as it will refer to
	//the creating thread
	RBuf iName;
	const TInt iIterations; 
	};

/**
Make aNumberOfThreads copies of aTest and run
each in its own thread

@param test Reference to test object
@param aTest Referance
*/
void MultipleTestRun(RTest& test, const CTest& aTest, TInt aNumberOfThreads)
	{
	RPointerArray<CTest> testArray;
	RPointerArray<TTestThread> threadArray;

	for(TInt i=0; i<aNumberOfThreads; i++)
		{		
		test.Printf(_L("Create test thread"));
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
		TTestThread* thread = threadArray[j];
		
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
		}
	
	threadArray.ResetAndDestroy();
	threadArray.Close();

	testArray.ResetAndDestroy();
	testArray.Close();
	}

#endif //TEST_THREAD_H

