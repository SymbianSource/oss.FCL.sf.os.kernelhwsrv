// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_dobject.cpp
// Overview:
// Test RObjectIx strategy of memory reallocation and 
// free list maintenance.
// Test DObjectCon findhandle methods
// API Information:
// DObject, RObjectIx
// Details:
// - Add a number of DObjects to RObjectIx, then remove them 
// in the same order. Verify results are as expected. Time
// how long the process takes to complete.
// - Add a number of DObjects to RObjectIx, then remove them
// in the reverse order. Verify results are as expected. Time
// how long the process takes to complete.
// - Add and remove a random number of DObjects to/from RObjectIx.
// Time how long the process takes to complete.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include "d_dobject.h"

LOCAL_D RTest test(_L("T_DOBJECT"));

TInt TestRObjectIxAccess(TAny* aRObjectIxPtr)
	{
	const TInt  KConcurrentDObjectTestRepeats = 1;
	
	RTestDObject  ldd;
	TInt  ret;
	
	ret = ldd.Open();
	if (ret == KErrNone)
		{
		for (TInt  repeat = 0;  repeat < KConcurrentDObjectTestRepeats;  repeat++)
			{
			ret = ldd.RObjectIxThreadTestExerciseIx(aRObjectIxPtr);
			if (ret != KErrNone)
				{
				break;
				}
			}
		ldd.Close();
		}
	
	return ret;
	} // TestRObjectIxAccess

void TestConcurrentRObjectIxAccess(RTestDObject& aLdd)
	{
	const TInt  KConcurrentDObjectThreads = 4;
	_LIT(KConcurrentDObjectThreadName, "T_DObject_Thread");
	
	TInt ret;
	
	//
	// Create a RObjectIx in the driver (pointer not valid user side!)...
	//
	void*  objectIxPtr = NULL;
	
	ret = aLdd.RObjectIxThreadTestCreateIx(objectIxPtr);
	test_KErrNone(ret);
	
	//
	// Run KConcurrentDObjectThreads number of threads which random add/remove
	// DObjects to the same RObjectIx...
	//
	RThread  threadHandle[KConcurrentDObjectThreads];
	TRequestStatus  status[KConcurrentDObjectThreads];
	TBuf<32>  threadName;
	TInt  thread;

	for (thread = 0;  thread < KConcurrentDObjectThreads;  thread++)
		{
		threadName.Copy(KConcurrentDObjectThreadName);
		threadName.AppendNum(thread);
		
		ret = threadHandle[thread].Create(threadName, TestRObjectIxAccess, KDefaultStackSize, NULL, objectIxPtr);
		test_KErrNone(ret);

		threadHandle[thread].Logon(status[thread]);
		}

	//
	// The test thread must be higher priority to ensure all the threads start.
	// All the threads are then resumed and allowed to run to completion...
	//
	RThread().SetPriority(EPriorityMore);

	for (thread = 0;  thread < KConcurrentDObjectThreads;  thread++)
		{
		threadHandle[thread].Resume();
		}

	for (thread = 0;  thread < KConcurrentDObjectThreads;  thread++)
		{
		User::WaitForRequest(status[thread]);
		test_KErrNone(status[thread].Int());
		CLOSE_AND_WAIT(threadHandle[thread]);
		}
	
	RThread().SetPriority(EPriorityNormal);

	//
	// Free the RObjectIx in the driver...
	//
	ret = aLdd.RObjectIxThreadTestFreeIx(objectIxPtr);
	test_KErrNone(ret);
	} // TestConcurrentRObjectIxAccess


void ListAllMutexes()
	{
	test.Printf(_L("Mutexes:\n"));
	TFullName name;
	TFindMutex find;
	while (find.Next(name) == KErrNone)
		{
		test.Printf(_L("  %S (find handle == %08x)\n"), &name, find.Handle());
		}
	}

const TInt KObjectCount = 20;
_LIT(KDoubleMatch, "*double*");
_LIT(KTrippleMatch, "*tripple*");

RMutex Mutexes[KObjectCount];
TBuf<32> ObjectName;

const TDesC& MutexName(TInt i)
	{
	ObjectName.Zero();
	ObjectName.AppendFormat(_L("Mutex_%02d"), i);
	if (i % 2 == 0)
		ObjectName.Append(_L("_double"));
	if (i % 3 == 0)
		ObjectName.Append(_L("_tripple"));
	return ObjectName;
	}

void CreateMutexes()
	{
	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		test(Mutexes[i].CreateGlobal(MutexName(i)) == KErrNone);
		}
	}

void DeleteMutexes()
	{
	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		Mutexes[i].Close();
		}
	}

void TestMutexesCreated()
	{
	test.Next(_L("Test mutexes have been created"));
	
	TFullName name;

	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		}
	}

void TestMutexesDeleted()
	{
	test.Next(_L("Test mutexes deleted"));

	TFullName name;

	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNotFound);
		}
	}


void TestFindSpecificMutex()
	{
	test.Next(_L("Test finding specific mutexes"));
	
	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		TFullName name;
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		test(name == MutexName(i));
		RMutex mutex;
		test(mutex.Open(find) == KErrNone);
		test(mutex.Name() == MutexName(i));
		mutex.Close();
		test(find.Next(name) == KErrNotFound);
		}	
	}

void TestFindMutexGroups()
	{
	test.Next(_L("Test finding groups of mutexes using wildcard name matching"));

	TFullName name;
	TInt i;
	
	TFindMutex find2(KDoubleMatch);
	for (i = 0 ; i < KObjectCount ; i += 2)
		{
		test(find2.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find2.Handle());
		test(name == MutexName(i));
		}
	test(find2.Next(name) == KErrNotFound);

	TFindMutex find3(KTrippleMatch);
	for (i = 0 ; i < KObjectCount ; i += 3)
		{
		test(find3.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find3.Handle());
		test(name == MutexName(i));
		}
	test(find3.Next(name) == KErrNotFound);
	}

void TestMatchChange()
	{
	test.Next(_L("Test changing match half way through find"));


	TFullName name;
	TInt i;
	
	TFindMutex find(KDoubleMatch);
	for (i = 0 ; i < KObjectCount/2 ; i += 2)
		{
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		test(name == MutexName(i));
		}

	find.Find(KTrippleMatch);
	for (i = 0 ; i < KObjectCount ; i += 3)
		{
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		test(name == MutexName(i));
		}
	test(find.Next(name) == KErrNotFound);
	}

void TestFindAndDeleteMutex1()
	{
	test.Next(_L("Test finding mutexes when the last found object has been deleted"));

	// Find and delete even mutexes
	TFullName name;
	TInt i;
	for (i = 0 ; i < KObjectCount ; i += 2)
		{
		TFindMutex find2(MutexName(i));
		test(find2.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find2.Handle());
		Mutexes[i].Close();
		RMutex mutex;
		test(mutex.Open(find2) == KErrNotFound);
		}

	// Check odd mutexes remaining
	for (i = 1 ; i < KObjectCount ; i += 2)
		{
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		}
	}

void TestFindAndDeleteMutex2()
	{
	test.Next(_L("Test finding mutexes when the last found object has moved in the container"));

	// Find even mutexes and delete odd
	TFullName name;
	TInt i;
	for (i = 0 ; i < KObjectCount ; i += 2)
		{
		TFindMutex find2(MutexName(i));
		test(find2.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find2.Handle());
		Mutexes[(i+KObjectCount-1)%KObjectCount].Close();	// -1%n = -1 or n-1, unspecified
		RMutex mutex;
		test(mutex.Open(find2) == KErrNone);
		test(mutex.Name() == MutexName(i));
		mutex.Close();
		}

	// Check even mutexes remaining
	for (i = 0 ; i < KObjectCount ; i += 2)
		{
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		}
	
	}

void TestFindWithCreation()
	{
	test.Next(_L("Test finding mutexes interleaved with creation"));

	TFullName name;
	
	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		test(Mutexes[i].CreateGlobal(MutexName(i)) == KErrNone);
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		RMutex mutex;
		test(mutex.Open(find) == KErrNone);
		test(mutex.Name() == MutexName(i));
		mutex.Close();
		}
	}

void TestFindWithCreation2()
	{
	test.Next(_L("Test finding mutexes interleaved with creation and deletion"));

	TFullName name;

	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		RMutex mutex;
		test(mutex.CreateGlobal(MutexName(0)) == KErrNone);
		TFindMutex find(MutexName(0));
		test(find.Next(name) == KErrNone);
		test(name == MutexName(0));
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		mutex.Close();

		TFindMutex find2(MutexName(0));
		test(find2.Next(name) == KErrNotFound);
		}
	}

void TestFindHandleOutOfRange()
	{
	test.Next(_L("Test finding mutexes when find handle index is off the end of container's array"));

	TFullName name;

	for (TInt i = 0 ; i < KObjectCount ; ++i)
		{
		TFindMutex find(MutexName(i));
		test(find.Next(name) == KErrNone);
		test.Printf(_L("  %02d: found handle %08x\n"), i, find.Handle());
		RMutex mutex;
		test(mutex.Open(find) == KErrNone);
		test(mutex.Name() == MutexName(i));
		mutex.Close();

		// towards the end, suddenly delete half the mutexes
		if (i == (3 * KObjectCount) / 4)
			{
			for (TInt j = 0 ; j < KObjectCount / 2 ; ++j)
				Mutexes[j].Close();
			}
		}
	}

void TestFindHandles()
	{
	test.Start(_L("Test FindHandle APIs using mutex classes"));

	CreateMutexes();
	ListAllMutexes();
	TestMutexesCreated();
	TestFindSpecificMutex();
	TestFindMutexGroups();
	TestMatchChange();
	DeleteMutexes();
	TestMutexesDeleted();

	CreateMutexes();
	TestFindAndDeleteMutex1();
	DeleteMutexes();

	CreateMutexes();
	TestFindHandleOutOfRange();
	DeleteMutexes();
	
	CreateMutexes();
	TestFindAndDeleteMutex2();
	DeleteMutexes();

	TestFindWithCreation();
	DeleteMutexes();

	TestFindWithCreation2();
	TestMutexesDeleted();
	
	test.End();
	}

GLDEF_C TInt E32Main()
    {
	SParam param;
	TInt duration, r;
	RTestDObject ldd;

	test.Title();

	test.Start(_L("Loading test driver..."));

	r=User::LoadLogicalDevice(KDObjectTestLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	r=ldd.Open();
	test(r==KErrNone);

	test.Next(_L("RObjectIxTest1 test ..."));
	r=ldd.RObjectIxTest1(duration);
	test(KErrNone==r);
	test.Printf(_L("... completed in %d kernel ticks\n") , duration);

	test.Next(_L("RObjectIxTest2 test ..."));
	r=ldd.RObjectIxTest2(duration);
	test(KErrNone==r);
	test.Printf(_L("... completed in %d kernel ticks\n") , duration);

	test.Next(_L("RObjectIxTest3 test (performance) ..."));
	param.iSeed[0] = 0;
	param.iSeed[1] = 1;
	param.iPerformanceTest = ETrue;
	r=ldd.RObjectIxTest3(param);
	test(KErrNone==r);
	test.Printf(_L("... completed in %d kernel ticks\n") , param.duration);

	test.Next(_L("RObjectIxTest3 test (random)..."));
	param.iSeed[0]=User::TickCount();
	param.iSeed[1]=User::TickCount();
	param.iPerformanceTest = EFalse;
	test.Printf(_L("... seeds=%xh and %xh ..."),param.iSeed[0],param.iSeed[1]);
	r=ldd.RObjectIxTest3(param);
	test(KErrNone==r);
	test.Printf(_L("... completed in %d kernel ticks\n") , param.duration);
 
	test.Next(_L("RObjectIxTest4 test (reserved slots)..."));
	test_KErrNone(ldd.RObjectIxTest4(duration));
	test.Printf(_L("... completed in %d kernel ticks\n") , duration);

	test.Next(_L("Test Concurrent access to RObjectIx"));
	TestConcurrentRObjectIxAccess(ldd);

	test.Next(_L("Test Invalid handle look up"));
 	test_KErrNone(ldd.InvalidHandleLookupTest());

 	test.Next(_L("Test Kern::ValidateName and Kern::ValidateFullName"));
 	test_KErrNone(ldd.DObjectNameTest());
 
	test.Next(_L("Closing test driver"));
	ldd.Close();

	test.Next(_L("FindHandles test"));
	TestFindHandles();
	
	test.End();

	return(0);
	}
