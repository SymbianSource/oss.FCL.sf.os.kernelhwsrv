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
// e32test\nkernsa\main.cpp
// 
//

#include <nktest/nkutils.h>

extern void TestFastSemaphore();
extern void TestFastMutex();
extern void TestSuspendKillMigrate();
extern void BasicThreadTests();
extern void TestIPI();
extern void TestDFCs();
extern void TestWaitFreePipe();
extern void BenchmarkTests();
extern void TestRWSpinLock();
extern void TestTiedEvents();

void Main(TAny*)
	{
	BenchmarkTests();

	TestWaitFreePipe();

	TestFastSemaphore();

	TestIPI();

	BasicThreadTests();

	TestDFCs();

	TestTiedEvents();

	TestRWSpinLock();

//	InitBTraceHandler();
//	StartBTrace();

	TestFastMutex();

	TestSuspendKillMigrate();

//	DumpBTraceBuffer();

	TEST_PRINT("Tests Completed");

	__finish();
	}

