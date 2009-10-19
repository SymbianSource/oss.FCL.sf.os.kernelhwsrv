// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\demandpaging\t_chunkcreate.cpp
// 
//

//
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"

#include "t_dpcmn.h"

_LIT(KGlobalChunkName, "TestChunk");

enum
	{
	ECreateNormal,
	ECreateCode,
	ECreateGlobal,
	ECreateLocalDE,
	ECreateGlobalDE,
	ECreateLocalDC,
	ECreateGlobalDC,
	};

enum
	{
	EPagingUnspec,	// Has to be first as can't clear back to unspecified.
	EPagingOn,
	EPagingOff,
	EPagingNumberAttribs,
	};


void VerifyChunkPaged(RChunk& aChunk, TBool aPaged, TInt aR)
	{
	UpdatePaged(aPaged);

	test_KErrNone(aR);
	test.Printf(_L("aPaged = %d, aChunk.IsPaged() = %d\n"), aPaged, aChunk.IsPaged());
	test_Equal(aPaged, aChunk.IsPaged());

	// Uses same name for global chunks so needs to be fully closed before next call
	CLOSE_AND_WAIT(aChunk);
	}


void VerifyChunkPaged(TChunkCreateInfo& aCreateInfo)
	{
	TBool paged = EFalse;
	for (TInt i = 0; i < EPagingNumberAttribs; i++)
		{
		switch(i)
			{
			case EPagingUnspec :
				paged = gProcessPaged;	// Should default to process's paged status.
				test.Printf(_L("Should default to process's paged status\n"));
				break;
			case EPagingOn :
				aCreateInfo.SetPaging(TChunkCreateInfo::EPaged);
				paged = ETrue;
				test.Printf(_L("Paging should be on\n"));
				break;
			case EPagingOff :
				aCreateInfo.SetPaging(TChunkCreateInfo::EUnpaged);
				paged = EFalse;
				test.Printf(_L("Paging should be off\n"));
				break;
			}
		RChunk chunk;
		TInt r = chunk.Create(aCreateInfo);
		VerifyChunkPaged(chunk, paged, r);
		}
	}



TInt PanicChunkCreate(TAny* aCreateInfo)
	{
	TChunkCreateInfo createInfo((*(TChunkCreateInfo*) aCreateInfo));
	gChunk.Create(createInfo);
	return KErrGeneral; // Should never reach here
	}


void TestPanicChunkCreate1(TInt aType, TInt aSize, TInt aMaxSize, TInt aPanicCode)
	{
	TChunkCreateInfo createInfo;
	switch (aType)
		{
		case ECreateNormal:
			createInfo.SetNormal(aSize, aMaxSize);
			break;

		case ECreateCode:
			createInfo.SetCode(aSize, aMaxSize);
			break;

		case ECreateGlobal:
			createInfo.SetNormal(aSize, aMaxSize);
			createInfo.SetGlobal(KGlobalChunkName);
			break;

		}

	
	RThread thread;
	test_KErrNone(thread.Create(_L("Panic CreateChunk"), PanicChunkCreate, KDefaultStackSize, KMinHeapSize, 
														KMinHeapSize,  (TAny*) &createInfo));

	test_KErrNone(TestThreadExit(thread, EExitPanic, aPanicCode));
	}

void TestPanicChunkCreate2(TInt aType, TInt aBottom, TInt aTop, TInt aMaxSize, TInt aPanicCode)
	{
	TChunkCreateInfo createInfo;
	switch (aType)
		{		
		case ECreateLocalDE:
			createInfo.SetDoubleEnded(aBottom, aTop, aMaxSize);
			break;

		case ECreateGlobalDE:
			createInfo.SetDoubleEnded(aBottom, aTop, aMaxSize);
			createInfo.SetGlobal(KGlobalChunkName);
			break;

		case ECreateLocalDC:
			createInfo.SetDisconnected(aBottom, aTop, aMaxSize);
			break;

		case ECreateGlobalDC:
			createInfo.SetDisconnected(aBottom, aTop, aMaxSize);
			createInfo.SetGlobal(KGlobalChunkName);
			break;
		}

	
	RThread thread;
	test_KErrNone(thread.Create(_L("Panic CreateChunk"), PanicChunkCreate, KDefaultStackSize, KMinHeapSize, 
														KMinHeapSize,  (TAny*) &createInfo));

	test_KErrNone(TestThreadExit(thread, EExitPanic, aPanicCode));
	}

// 
// TestLocalChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the local chunk creation implementation.
//! @SYMTestActions			
//! 1.	Create a local chunk and specify the following paging options. 
//!		Following this, check the paging status of the chunk by calling IsPaged
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a local chunk and specify aMaxSize to be negative
//! 3.	Create a local chunk and specify aSize to be negative
//! 4.	Create a local chunk and specify aMaxSize to be less than aSize.
//!
//! @SYMTestExpectedResults 
//! 1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER100
//! 4.	Panic USER101
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestLocalChunk()
	{
	test.Start(_L("Test RChunk::CreateLocal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetNormal(gPageSize, gPageSize);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateLocal(gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}

	test.Next(_L("Test RChunk::CreateLocal - invalid max size"));
	TestPanicChunkCreate1(ECreateNormal, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateLocal - invalid  size"));
	TestPanicChunkCreate1(ECreateNormal, -1, gPageSize, EChkCreateSizeNotPositive);

	
	test.Next(_L("Test RChunk::CreateLocal - aSize > aMaxSize"));
	TestPanicChunkCreate1(ECreateNormal, gPageSize << 1, gPageSize, EChkCreateMaxLessThanMin);
	test.End();
	}

// 
// TestCodeChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the user code chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a user code chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a user code chunk and specify aMaxSize to be negative
//! 3.	Create a user code chunk and specify aSize to be negative
//! 4.	Create a user code chunk and specify aMaxSize to be less than aSize.
//!
//! @SYMTestExpectedResults 
//! 1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER100
//! 4.	Panic USER101
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestCodeChunk()
	{
	test.Start(_L("Test RChunk::CreateLocalCode - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetCode(gPageSize, gPageSize);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateLocal(gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}

	test.Next(_L("Test RChunk::CreateLocalCode - invalid max size"));
	TestPanicChunkCreate1(ECreateCode, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateLocalCode - invalid  size"));
	TestPanicChunkCreate1(ECreateCode, -1, gPageSize, EChkCreateSizeNotPositive);

	
	test.Next(_L("Test RChunk::CreateLocalCode - aSize > aMaxSize"));
	TestPanicChunkCreate1(ECreateCode, gPageSize << 1, gPageSize, EChkCreateMaxLessThanMin);

	test.End();
	}

// 
// TestGlobalChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the global chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a global chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 1.	Create a global chunk and specify aMaxSize to be negative
//! 2.	Create a global chunk and specify aSize to be negative
//! 3.	Create a global chunk and specify aMaxSize to be less than aSize.
//!
//! @SYMTestExpectedResults 
//! 1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER100
//! 4.	Panic USER101
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestGlobalChunk()
	{
	test.Start(_L("Test RChunk::CreateGlobal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetNormal(gPageSize, gPageSize);
		createInfo.SetGlobal(KGlobalChunkName);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateLocal(gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}

	test.Next(_L("Test RChunk::CreateGlobal - invalid max size"));
	TestPanicChunkCreate1(ECreateGlobal, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateGlobal - invalid  size"));
	TestPanicChunkCreate1(ECreateGlobal, -1, gPageSize, EChkCreateSizeNotPositive);

	
	test.Next(_L("Test RChunk::CreateGlobal - aSize > aMaxSize"));
	TestPanicChunkCreate1(ECreateGlobal, gPageSize << 1, gPageSize, EChkCreateMaxLessThanMin);

	test.End();
	}

// 
// TestLocDEChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the local double ended chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a local, double ended, chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a local, double ended, chunk and specify aMaxSize to be negative
//! 3.	Create a local, double ended, chunk and specify aInitialBottom to be negative
//! 4.	Create a local, double ended, chunk and specify aInitialTop to be negative.
//! 5.	Create a local, double ended, chunk and specify aInitialBottom to be greater than aInitialTop.
//! 6.	Create a local, double ended, chunk and specify aInitialTop to be greater than aMaxSize.
//!
//! @SYMTestExpectedResults 
//! 1.	1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER120
//! 4.	Panic USER121
//! 5.	Panic USER122
//! 6.	Panic USER123
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestLocDEChunk()
	{
	test.Start(_L("Test RChunk::CreateDoubleEndedLocal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetDoubleEnded(0, gPageSize, gPageSize);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateDoubleEndedLocal(0, gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}
	

	test.Next(_L("Test RChunk::CreateDoubleEndedLocal - invalid max size"));
	TestPanicChunkCreate2(ECreateLocalDE, 0, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedLocal - invalid bottom"));
	TestPanicChunkCreate2(ECreateLocalDE, -1, gPageSize, gPageSize, EChkCreateBottomNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedLocal - invalid top"));
	TestPanicChunkCreate2(ECreateLocalDE, 0, -1, gPageSize, EChkCreateTopNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedLocal - bottom > top"));
	TestPanicChunkCreate2(ECreateLocalDE, gPageSize, 0, gPageSize, EChkCreateTopLessThanBottom);

	test.Next(_L("Test RChunk::CreateDoubleEndedLocal - top > max size"));
	TestPanicChunkCreate2(ECreateLocalDE, 0, gPageSize << 1, gPageSize, EChkCreateTopBiggerThanMax);

	test.End();
	}


// 
// TestGlobDEChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the global double ended chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a global, double ended, chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a global, double ended, chunk and specify aMaxSize to be negative
//! 3.	Create a global, double ended, chunk and specify aInitialBottom to be negative
//! 4.	Create a global, double ended, chunk and specify aInitialTop to be negative.
//! 5.	Create a global, double ended, chunk and specify aInitialBottom to be greater than aInitialTop.
//! 6.	Create a global, double ended, chunk and specify aInitialBottom to be greater than aMaxSize.
//! 7.	Create a global, double ended, chunk and specify aInitialTop to be greater than aMaxSize.
//!
//! @SYMTestExpectedResults 
//! 1.	1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER120
//! 4.	Panic USER121
//! 5.	Panic USER122
//! 6.	Panic USER123
//! 7.	Panic USER123
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestGlobDEChunk()
	{
	test.Start(_L("Test RChunk::CreateDoubleEndedGlobal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetDoubleEnded(0, gPageSize, gPageSize);
		createInfo.SetGlobal(KGlobalChunkName);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateDoubleEndedLocal(0, gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}
	

	test.Next(_L("Test RChunk::CreateDoubleEndedGlobal - invalid max size"));
	TestPanicChunkCreate2(ECreateGlobalDE, 0, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedGlobal - invalid bottom"));
	TestPanicChunkCreate2(ECreateGlobalDE, -1, gPageSize, gPageSize, EChkCreateBottomNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedGlobal - invalid top"));
	TestPanicChunkCreate2(ECreateGlobalDE, 0, -1, gPageSize, EChkCreateTopNegative);

	test.Next(_L("Test RChunk::CreateDoubleEndedGlobal - bottom > top"));
	TestPanicChunkCreate2(ECreateGlobalDE, gPageSize, 0, gPageSize, EChkCreateTopLessThanBottom);

	test.Next(_L("Test RChunk::CreateDoubleEndedGlobal - top > max size"));
	TestPanicChunkCreate2(ECreateGlobalDE, 0, gPageSize << 1, gPageSize, EChkCreateTopBiggerThanMax);
	test.End();
	}


// 
// TestLocDiscChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the local disconnected chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a local, disconnected chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a local, disconnected chunk and specify aMaxSize to be negative
//! 3.	Create a local, disconnected chunk and specify aInitialBottom to be negative
//! 4.	Create a local, disconnected chunk and specify aInitialTop to be negative.
//! 5.	Create a local, disconnected chunk and specify aInitialBottom to be greater than aInitialTop.
//! 6.	Create a local, disconnected chunk and specify aInitialBottom to be greater than aMaxSize.
//! 7.	Create local, disconnected chunk and specify aInitialTop to be greater than aMaxSize.
//!
//! @SYMTestExpectedResults 
//! 1.	1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER120
//! 4.	Panic USER121
//! 5.	Panic USER122
//! 6.	Panic USER123
//! 7.	Panic USER123
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestLocDiscChunk()
	{
	test.Start(_L("Test RChunk::CreateDisconnectedLocal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetDisconnected(0, gPageSize, gPageSize);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateDoubleEndedLocal(0, gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}
	

	test.Next(_L("Test RChunk::CreateDisconnectedLocal - invalid max size"));
	TestPanicChunkCreate2(ECreateLocalDC, 0, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedLocal - invalid bottom"));
	TestPanicChunkCreate2(ECreateLocalDC, -1, gPageSize, gPageSize, EChkCreateBottomNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedLocal - invalid top"));
	TestPanicChunkCreate2(ECreateLocalDC, 0, -1, gPageSize, EChkCreateTopNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedLocal - bottom > top"));
	TestPanicChunkCreate2(ECreateLocalDC, gPageSize, 0, gPageSize, EChkCreateTopLessThanBottom);

	test.Next(_L("Test RChunk::CreateDisconnectedLocal - top > max size"));
	TestPanicChunkCreate2(ECreateLocalDC, 0, gPageSize << 1, gPageSize, EChkCreateTopBiggerThanMax);
	
	test.End();
	}


// 
// TestGlobDiscChunk
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Verify the global disconnected chunk creation implementation
//! @SYMTestActions			
//! 1.	Create a global, disconnected chunk and specify the following paging options. 
//!		Following this check the paging status of the chunk by calling IsPaged().
//!		a.	Not specified
//!		b.	Paged
//!		c.	Unpaged
//! 2.	Create a global, disconnected chunk and specify aMaxSize to be negative
//! 3.	Create a global, disconnected chunk and specify aInitialBottom to be negative
//! 4.	Create a global, disconnected chunk and specify aInitialTop to be negative.
//! 5.	Create a global, disconnected chunk and specify aInitialBottom to be greater than aInitialTop.
//! 6.	Create a global, disconnected chunk and specify aInitialBottom to be greater than aMaxSize.
//! 7.	Create global, disconnected chunk and specify aInitialTop to be greater than aMaxSize.
//!
//! @SYMTestExpectedResults 
//! 1.	1.	The following results are expected:
//!		a.	The chunk should take on the paging status of the process
//!		b.	ETrue
//!		c.	EFalse
//! 2.	Panic USER99
//! 3.	Panic USER120
//! 4.	Panic USER121
//! 5.	Panic USER122
//! 6.	Panic USER123
//! 7.	Panic USER123
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestGlobDiscChunk()
	{
	test.Start(_L("Test RChunk::CreateDisconnectedGlobal - paging attributes"));
		{
		TChunkCreateInfo createInfo;
		createInfo.SetDisconnected(0, gPageSize, gPageSize);
		createInfo.SetGlobal(KGlobalChunkName);
		VerifyChunkPaged(createInfo);
		// Test default create method
		TInt r = gChunk.CreateDoubleEndedLocal(0, gPageSize, gPageSize);
		VerifyChunkPaged(gChunk, gProcessPaged, r);
		}
	

	test.Next(_L("Test RChunk::CreateDisconnectedGlobal - invalid max size"));
	TestPanicChunkCreate2(ECreateGlobalDC, 0, gPageSize, -1, EChkCreateMaxSizeNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedGlobal - invalid bottom"));
	TestPanicChunkCreate2(ECreateGlobalDC, -1, gPageSize, gPageSize, EChkCreateBottomNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedGlobal - invalid top"));
	TestPanicChunkCreate2(ECreateGlobalDC, 0, -1, gPageSize, EChkCreateTopNegative);

	test.Next(_L("Test RChunk::CreateDisconnectedGlobal - bottom > top"));
	TestPanicChunkCreate2(ECreateGlobalDC, gPageSize, 0, gPageSize, EChkCreateTopLessThanBottom);

	test.Next(_L("Test RChunk::CreateDisconnectedGlobal - top > max size"));
	TestPanicChunkCreate2(ECreateGlobalDC, 0, gPageSize << 1, gPageSize, EChkCreateTopBiggerThanMax);

	test.End();
	}




TInt TestingTChunkCreate()
	{
	test.Start(_L("Test TChunkCreateInfo - Local Chunk"));
	TestLocalChunk();

	test.Next(_L("Test TChunkCreateInfo - Code Chunk"));
	TestCodeChunk();

	test.Next(_L("Test TChunkCreateInfo - Global Chunk"));
	TestGlobalChunk();

	test.Next(_L("Test TChunkCreateInfo - Local Double Ended Chunk"));
	TestLocDEChunk();

	test.Next(_L("Test TChunkCreateInfo - Global Double Ended Chunk"));
	TestGlobDEChunk();

	test.Next(_L("Test TChunkCreateInfo - Local Disconnected Chunk"));
	TestLocDiscChunk();

	test.Next(_L("Test TChunkCreateInfo - Global Disconnected Chunk"));
	TestGlobDiscChunk();

	test.End();
	return 0;
	}
