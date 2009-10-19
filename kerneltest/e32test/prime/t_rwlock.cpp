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
// Contributors:
//
// Description:
// e32test\prime\t_rwlock.cpp
// Overview:
// Test the RReadWriteLock type.
// API Information:
// RReadWriteLock
// Details:
// Test all functions individually and in combination.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

//! @SYMTestCaseID             KBASE-T_RWLOCK-2444
//! @SYMTestType               UT
//! @SYMTestCaseDesc           Verify correct operation of RReadWriteLock
//! @SYMPREQ                   PREQ2094
//! @SYMTestPriority           High
//! @SYMTestActions            Call all functions of RReadWriteLock in a variety
//!                            of circumstances and verify correct results                                            
//! @SYMTestExpectedResults    All tests pass

#include <e32atomics.h>
#include <e32test.h>
#include <e32panic.h>
#include <e32def.h>
#include <e32def_private.h>

RTest Test(_L("T_RWLOCK"));
RReadWriteLock TheLock;
volatile TInt ThreadsRunning;
TInt LogIndex;
TBool LogReaders[20];

// Check creating, using and closing a lock doesn't leak memory
void TestCreation()
	{
	Test.Next(_L("Creation"));
	
    __KHEAP_MARK;
    __UHEAP_MARK;

	Test(TheLock.CreateLocal() == KErrNone);
	TheLock.ReadLock();
	TheLock.Unlock();
	TheLock.WriteLock();
	TheLock.Unlock();
	TheLock.Close();

	__UHEAP_MARKEND;
	__KHEAP_MARKEND;
	}

TInt ReadEntryPoint(TAny* aArg)
	{
	*(TBool*)aArg = ETrue;
	__e32_atomic_add_ord32(&ThreadsRunning, 1);
	TheLock.ReadLock();
	const TInt index = __e32_atomic_add_ord32(&LogIndex, 1);
	LogReaders[index] = ETrue;
	TheLock.Unlock();
	__e32_atomic_add_ord32(&ThreadsRunning, TUint32(-1));
	return KErrNone;
	}

TInt WriteEntryPoint(TAny* aArg)
	{
	*(TBool*)aArg = ETrue;
	__e32_atomic_add_ord32(&ThreadsRunning, 1);
	TheLock.WriteLock();
	const TInt index = __e32_atomic_add_ord32(&LogIndex, 1);
	LogReaders[index] = EFalse;
	TheLock.Unlock();
	__e32_atomic_add_ord32(&ThreadsRunning, TUint32(-1));
	return KErrNone;
	}

void Init()
	{
	__e32_atomic_store_ord32(&ThreadsRunning, 0);
	__e32_atomic_store_ord32(&LogIndex, 0);
	}

void CreateThread(TBool aReader)
	{
	RThread newThread;
	TBool threadStarted = EFalse;
	TInt ret = newThread.Create(KNullDesC, aReader ? ReadEntryPoint : WriteEntryPoint, KDefaultStackSize, KMinHeapSize, KMinHeapSize, &threadStarted, EOwnerProcess);
	Test(ret == KErrNone, __LINE__);
	newThread.SetPriority(EPriorityMore);
	newThread.Resume();
	while (!threadStarted)
		User::After(1000);
	newThread.Close();
	}

void WaitForThreadsToClose(TInt aThreads = 0)
	{
	while (ThreadsRunning > aThreads)
		{
		User::After(1000);
		}
	}

// Check that queuing multiple reads and writes on a lock with writer priority
// results in the correct type of client being released in the correct order
// (can' predict exact client order on multi-processor systems though)
void TestWriterPriority()
	{
	Test.Next(_L("Writer Priority"));
	TInt ret = TheLock.CreateLocal(RReadWriteLock::EWriterPriority);
	Test(ret == KErrNone, __LINE__);
	TheLock.WriteLock();

	Init();
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);

	TheLock.Unlock();
	WaitForThreadsToClose();
	TheLock.ReadLock();

	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);

	TheLock.Unlock();
	WaitForThreadsToClose();

	TheLock.Close();

	Test(LogIndex == 15, __LINE__);
	const TBool expected[] = { EFalse, EFalse, EFalse, EFalse, ETrue, ETrue, ETrue, ETrue, ETrue, ETrue, EFalse, EFalse, ETrue, ETrue, ETrue };
	for (TInt index = 0; index < LogIndex; index++)
		{
		Test(LogReaders[index] == expected[index], __LINE__);
		}
	}

// Check that queuing multiple reads and writes on a lock with alternate priority
// results in the correct type of client being released in the correct order
// (can' predict exact client order on multi-processor systems though)
void TestAlternatePriority()
	{
	Test.Next(_L("Alternate Priority"));
	TInt ret = TheLock.CreateLocal(RReadWriteLock::EAlternatePriority);
	Test(ret == KErrNone, __LINE__);
	TheLock.WriteLock();

	Init();
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(EFalse);
	CreateThread(EFalse);
	CreateThread(EFalse);
	CreateThread(EFalse);

	TheLock.Unlock();
	WaitForThreadsToClose();
	TheLock.ReadLock();

	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);

	TheLock.Unlock();
	WaitForThreadsToClose();

	TheLock.Close();

	Test(LogIndex == 15, __LINE__);
	const TInt expected[] = { ETrue, EFalse, ETrue, EFalse, ETrue, EFalse, ETrue, EFalse, ETrue, EFalse, EFalse, ETrue, EFalse, ETrue, ETrue };
	for (TInt index = 0; index < LogIndex; index++)
		{
		Test(LogReaders[index] == expected[index], __LINE__);
		}
	}

// Check that queuing multiple reads and writes on a lock with reader priority
// results in the correct type of client being released in the correct order
// (can' predict exact client order on multi-processor systems though)
void TestReaderPriority()
	{
	Test.Next(_L("Reader Priority"));
	TInt ret = TheLock.CreateLocal(RReadWriteLock::EReaderPriority);
	Test(ret == KErrNone, __LINE__);
	TheLock.WriteLock();

	Init();
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);

	TheLock.Unlock();
	WaitForThreadsToClose();
	TheLock.WriteLock();

	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);

	TheLock.Unlock();
	WaitForThreadsToClose();

	TheLock.Close();

	Test(LogIndex == 15, __LINE__);
	const TInt expected[] = { ETrue, ETrue, ETrue, ETrue, ETrue, ETrue, EFalse, EFalse, EFalse, EFalse, ETrue, ETrue, ETrue, EFalse, EFalse };
	for (TInt index = 0; index < LogIndex; index++)
		{
		Test(LogReaders[index] == expected[index], __LINE__);
		}
	}

void DoTestTryLock(TBool aWriterFirst)
	{
	TheLock.ReadLock();

		TBool tryLock = TheLock.TryWriteLock();
		Test(!tryLock, __LINE__);

			tryLock = TheLock.TryReadLock();
			Test(tryLock, __LINE__);
			TheLock.Unlock();

		Init();
		CreateThread(EFalse);
		tryLock = TheLock.TryReadLock();
		if (tryLock)
			{
			Test(!aWriterFirst, __LINE__);
			TheLock.Unlock();
			}
		else
			{
			Test(aWriterFirst, __LINE__);
			}
		tryLock = TheLock.TryWriteLock();
		Test(!tryLock, __LINE__);

	TheLock.Unlock();
	WaitForThreadsToClose();

	TheLock.WriteLock();

		tryLock = TheLock.TryReadLock();
		Test(!tryLock, __LINE__);
		tryLock = TheLock.TryWriteLock();
		Test(!tryLock, __LINE__);

	TheLock.Unlock();
	TheLock.Close();
	}

// Check that the TryReadLock and TryWriteLock functions block only when they
// should for the different types of priority
void TestTryLock()
	{
	Test.Next(_L("Try Lock"));

	TInt ret = TheLock.CreateLocal(RReadWriteLock::EWriterPriority);
	Test(ret == KErrNone, __LINE__);
	DoTestTryLock(ETrue);

	ret = TheLock.CreateLocal(RReadWriteLock::EAlternatePriority);
	Test(ret == KErrNone, __LINE__);
	DoTestTryLock(ETrue);

	ret = TheLock.CreateLocal(RReadWriteLock::EReaderPriority);
	Test(ret == KErrNone, __LINE__);
	DoTestTryLock(EFalse);

	TheLock.Close();
	}

void DoTestUpgrade(RReadWriteLock::TReadWriteLockPriority aPriority)
	{
	TInt ret = TheLock.CreateLocal(aPriority);
	Test(ret == KErrNone, __LINE__);
	TheLock.ReadLock();

	TBool success = TheLock.TryUpgradeReadLock();
	Test(success, __LINE__);
	TheLock.Unlock();

	TheLock.ReadLock();
	TheLock.ReadLock();
	success = TheLock.TryUpgradeReadLock();
	Test(!success, __LINE__);
	TheLock.Unlock();
	TheLock.Unlock();

	TheLock.ReadLock();
	Init();
	CreateThread(EFalse);
	success = TheLock.TryUpgradeReadLock();
	Test(success || !(aPriority == RReadWriteLock::EReaderPriority), __LINE__);

	TheLock.Unlock();
	WaitForThreadsToClose();
	TheLock.Close();
	}

// Check that upgrading a lock succeeds only when it should
void TestUpgrade()
	{
	Test.Next(_L("Upgrade Lock"));

	DoTestUpgrade(RReadWriteLock::EWriterPriority);
	DoTestUpgrade(RReadWriteLock::EAlternatePriority);
	DoTestUpgrade(RReadWriteLock::EReaderPriority);
	}

void DoTestDowngrade(RReadWriteLock::TReadWriteLockPriority aPriority)
	{
	TInt ret = TheLock.CreateLocal(aPriority);
	Test(ret == KErrNone, __LINE__);
	TheLock.WriteLock();

	Init();
	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);

	TheLock.DowngradeWriteLock();

	switch (aPriority)
		{
	case RReadWriteLock::EWriterPriority:
	case RReadWriteLock::EAlternatePriority:
		{
		Test(LogIndex == 0, __LINE__);
		break;
		}
	case RReadWriteLock::EReaderPriority:
		{
		WaitForThreadsToClose(2);
		Test(LogIndex == 2, __LINE__);
		Test(LogReaders[0], __LINE__);
		Test(LogReaders[1], __LINE__);
		break;
		}
		};

	CreateThread(ETrue);
	CreateThread(EFalse);
	CreateThread(ETrue);
	CreateThread(EFalse);

	TheLock.Unlock();
	WaitForThreadsToClose();
	TheLock.Close();

	Test(LogIndex == 8, __LINE__);

	switch (aPriority)
		{
	case RReadWriteLock::EWriterPriority:
		{
		const TInt expected[] = { EFalse, EFalse, EFalse, EFalse, ETrue, ETrue, ETrue, ETrue };
		for (TInt index = 0; index < LogIndex; index++)
			{
			Test(LogReaders[index] == expected[index], __LINE__);
			}
		break;
		}
	case RReadWriteLock::EAlternatePriority:
		{
		const TInt expected[] = { EFalse, ETrue, EFalse, ETrue, EFalse, ETrue, EFalse, ETrue };
		for (TInt index = 0; index < LogIndex; index++)
			{
			Test(LogReaders[index] == expected[index], __LINE__);
			}
		break;
		}
	case RReadWriteLock::EReaderPriority:
		{
		const TInt expected[] = { ETrue, ETrue, ETrue, ETrue, EFalse, EFalse, EFalse, EFalse };
		for (TInt index = 0; index < LogIndex; index++)
			{
			Test(LogReaders[index] == expected[index], __LINE__);
			}
		break;
		}
		};
	}

// Check that downgrading a lock succeeds only when it should
void TestDowngrade()
	{
	Test.Next(_L("Downgrade Lock"));

	DoTestDowngrade(RReadWriteLock::EWriterPriority);
	DoTestDowngrade(RReadWriteLock::EAlternatePriority);
	DoTestDowngrade(RReadWriteLock::EReaderPriority);
	}

TInt PanicEntryPoint(TAny* aArg)
	{
	switch (TInt(aArg))
		{
		case 0: // Check priority lower bound
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EWriterPriority-1));
			break;
		case 1: // Check priority upper bound
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EReaderPriority+1));
			break;
		case 2: // Check close while holding read lock
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EAlternatePriority));
			TheLock.ReadLock();
			TheLock.Close();
			break;
		case 3: // Check close while holding write lock
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EAlternatePriority));
			TheLock.WriteLock();
			TheLock.Close();
			break;
		case 4: // Check max readers
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EReaderPriority));
			{
			for (TInt count = 0; count < RReadWriteLock::EReadWriteLockClientCategoryLimit; count++)
				TheLock.ReadLock();
			}
			TheLock.ReadLock();
			break;
		case 5: // Check max pending readers
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EReaderPriority));
			TheLock.WriteLock();
			{
			TUint16* hackLock = (TUint16*)&TheLock;
			hackLock[2] = KMaxTUint16; // Hack readers pending field
			}
			TheLock.ReadLock();
			break;
		case 6: // Check max pending writers
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EReaderPriority));
			TheLock.ReadLock();
			{
			TUint16* hackLock = (TUint16*)&TheLock;
			hackLock[3] = KMaxTUint16; // Hack writers pending field
			}
			TheLock.WriteLock();
			break;
		case 7: // Check lock held when unlocking
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EAlternatePriority));
			TheLock.Unlock();
			break;
		case 8: // Check lock held when unlocking after read lock/unlock
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EAlternatePriority));
			TheLock.ReadLock();
			TheLock.Unlock();
			TheLock.Unlock();
			break;
		case 9: // Check lock held when unlocking after write lock/unlock
			TheLock.CreateLocal(RReadWriteLock::TReadWriteLockPriority(RReadWriteLock::EAlternatePriority));
			TheLock.WriteLock();
			TheLock.Unlock();
			TheLock.Unlock();
			break;
		default:
			return KErrNone;
		};

	return KErrNotSupported;
	}

TBool CreatePanicThread(TInt aTest)
	{
	User::SetJustInTime(EFalse);
	TBool finished = EFalse;

	RThread panicThread;
	TInt ret = panicThread.Create(KNullDesC, PanicEntryPoint, KDefaultStackSize, KMinHeapSize, KMinHeapSize, (TAny*)aTest, EOwnerThread);
	Test(ret == KErrNone, __LINE__);
	panicThread.Resume();

	TRequestStatus stat;
	panicThread.Logon(stat);
	User::WaitForRequest(stat);
	User::SetJustInTime(ETrue);

	if (panicThread.ExitType() == EExitPanic)
		{
		TInt panicValue = 0;
		switch (aTest)
			{
		case 0:
		case 1:
			panicValue = EReadWriteLockInvalidPriority;
			break;
		case 2:
		case 3:
			panicValue = EReadWriteLockStillPending;
			break;
		case 4:
		case 5:
		case 6:
			panicValue = EReadWriteLockTooManyClients;
			break;
		case 7:
		case 8:
		case 9:
			panicValue = EReadWriteLockBadLockState;
			break;
		default:
			Test(0, __LINE__);
			break;
			};
	
		Test(stat == panicValue, __LINE__);
		Test(panicThread.ExitReason() == panicValue, __LINE__);
		}
	else
		{
		Test(stat == KErrNone, __LINE__);
		finished = ETrue;
		}

	RTest::CloseHandleAndWaitForDestruction(panicThread);
	
	switch (aTest)
		{
		case 2: // Check close while holding read lock
		case 3: // Check close while holding write lock
			TheLock.Unlock();
			TheLock.Close();
			break;
		case 4: // Check max readers
			{
			for (TInt count = 0; count < RReadWriteLock::EReadWriteLockClientCategoryLimit; count++)
				TheLock.Unlock();
			}
			TheLock.Close();
			break;
		case 5: // Check max pending readers
		case 6: // Check max pending writers
			{
			TUint16* hackLock = (TUint16*)&TheLock;
			hackLock[2] = 0; // Reset readers pending field
			hackLock[3] = 0; // Reset writers pending field
			}
			TheLock.Unlock();
			TheLock.Close();
			break;
		case 7: // Check lock held when unlocking
		case 8: // Check lock held when unlocking after read lock/unlock
		case 9: // Check lock held when unlocking after write lock/unlock
			TheLock.Close();
			break;
		default:
			break;
		};
	return finished;
	}

// Check that the various asserts guarding invalid conditions can be reached
void TestPanics()
	{
	Test.Next(_L("Panics"));

	for (TInt testIndex = 0; !CreatePanicThread(testIndex); testIndex++) ;
	}

TInt E32Main()
    {
	Test.Title();
	Test.Start(_L("RReadWriteLock Testing"));

	TestCreation();
	TestWriterPriority();
	TestAlternatePriority();
	TestReaderPriority();
	TestTryLock();
	TestUpgrade();
	TestDowngrade();
	TestPanics();

	Test.End();
	return KErrNone;
    }


