/*
* Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
* Description: Some helper classes to assist with writing multi-threaded tests
*
*/


#ifndef __TEST_THREAD_H__
#define __TEST_THREAD_H__

#include <e32base.h>
#include <e32debug.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32cmn_private.h>

_LIT(KPanicCat, "test_thread.h");


static const TInt KHeapSize=0x2000;

enum TPanicCode
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

	static TInt RunFunctor(TAny* aFunctor);

	TRequestStatus iLogonStatus;
	static TInt iCount;
	};

class TTestThread : public TTestRemote
	{
public:
	TTestThread(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume=ETrue);

	/**
	Run aFunctor in another thread
	*/
	TTestThread(const TDesC& aName, TFunctor& aFunctor, TBool aAutoResume=ETrue);

	~TTestThread();

	void Resume();

	/**
	If thread exited normally, return its return code
	Otherwise, leave with exit reason
	*/
	virtual TInt WaitForExitL();

	virtual void Rendezvous(TRequestStatus& aStatus);

private:
	void Init(const TDesC& aName, TThreadFunction aFn, TAny* aData, TBool aAutoResume);

	RThread iThread;
	};

class CTest : public CBase, public TFunctor
	{
public:
	virtual ~CTest();

	virtual void operator()();
	virtual void RunTest() = 0;
	virtual CTest* Clone() const = 0;

	virtual void SetupL();

	/**
	Prints a formatted description of the test
	*/
	virtual void Announce() const;

	const TDesC& Name() const;

	/**
	Should print the type of test, with no newlines.
	eg. "Transfer", "Fragmentation"
	*/
	virtual void PrintTestType() const = 0;

	/**
	Display any information about test environment, with no newlines
	eg. "DMA channel 16"
	The base class version prints nothing.
	*/
	virtual void PrintTestInfo() const;

protected:
	CTest(const TDesC& aName, TInt aIterations);
	CTest(const CTest& aOther);

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
void MultipleTestRun(RTest& test, const CTest& aTest, TInt aNumberOfThreads);

void MultipleTestRun(const RPointerArray<CTest>& aTests);
#endif // #ifndef __TEST_THREAD_H__

