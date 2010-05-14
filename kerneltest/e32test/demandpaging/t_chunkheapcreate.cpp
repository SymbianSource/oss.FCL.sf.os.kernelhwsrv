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
// e32test\demandpaging\t_chunkheapcreate.cpp
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

enum
	{
	ETestLocal, 
	ETestGlobal,
	ETestExisting,
	};

static TPtrC gGlobalDes = _L("GlobalHeap");

void UseOrCreateChunk(TChunkHeapCreateInfo aCreateInfo, TInt aTestType)
	{
	switch(aTestType)
		{
		case ETestLocal:
			aCreateInfo.SetCreateChunk(NULL);
			break;

		case ETestGlobal:
			aCreateInfo.SetCreateChunk(&gGlobalDes);
			break;
			
		case ETestExisting:
			aCreateInfo.SetUseChunk(gChunk);
			break;
		}
	}

/**
Attempt to create a global or local UserHeap with the given attributes

@return ETrue The chunk is paged or unpaged as expected.
*/
TInt UserHeapAtt(TBool aPaged, TChunkHeapCreateInfo& aCreateInfo)
	{
	UpdatePaged(aPaged);
	
	RHeap* heap = UserHeap::ChunkHeap(aCreateInfo);
	test_NotNull(heap);
	RChunk chunk;
	chunk.SetHandle(((TestHybridHeap*) heap)->ChunkHandle());
	TBool paged = chunk.IsPaged();
	chunk.Close();
	return (aPaged == paged);
	}

TInt PanicUserHeap(TAny*)
	{
	TChunkHeapCreateInfo createInfo(0, gPageSize);
	createInfo.SetCreateChunk(NULL);
	createInfo.SetMode((TUint)~UserHeap::EChunkHeapMask);
	test(UserHeapAtt(gProcessPaged, createInfo));
	return KErrGeneral;	// Shouldn't reach here.
	}

TInt PanicChunkHeapCreate(TAny* aCreateInfo)
	{
	TChunkHeapCreateInfo createInfo((*(TChunkHeapCreateInfo*) aCreateInfo));
	UserHeap::ChunkHeap(createInfo);
	return KErrGeneral; // Should never reahc here
	}

//
// TestChunkHeapCreate
//
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID			KBASE-T_CHUNKHEAPCREATE-xxxx
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1954
//! @SYMTestCaseDesc		Testing TChunkHeapCreateInfo
//!							Verify the chunk heap creation implementation
//! @SYMTestActions		
//! 1.	Create a local chunk by calling SetCreateChunk(NULL). 
//!		Following this check the paging status of the chunk heap. 
//! 2.	Create a local chunk by calling SetCreateChunk(NULL). Set the chunk heap to EPaged. 
//!		Following this check the paging status of the chunk heap. 
//! 3.	Create a local chunk by calling SetCreateChunk(NULL). Set the chunk heap to EUnPaged. 
//!		Following this check the paging status of the chunk heap. 
//! 4.	Create a heap within a local chunk and set the minimum length of the heap to be greater 
//!		than the maximum length of the heap
//! 5.	Create a heap within a local chunk and set the minimum length of the heap to -1
//! 6.	Create a heap within a local chunk and set the maximum length of the heap to 
//!		less than KMinHeapSize
//! 7.	Call TChunkHeapCreateInfo::SetUseChunk()  on a created local chunk calling SetCreateChunk(NULL) 
//!		before hand. Following this call UserHeap::ChunkHeap() and compare the heap and the chunk handles
//! 8.	Call TChunkHeapCreateInfo::SetUseChunk()  on a created local chunk calling SetCreateChunk(NULL) 
//!		after. Following this call UserHeap::ChunkHeap() and compare the heap and the chunk handles
//! 9.	Call TChunkHeapCreateInfo::SetSingleThread() on a created local chunk specifying EFalse. 
//!		Following this call UserHeap::ChunkHeap();
//! 10.	Call TChunkHeapCreateInfo::SetSingleThread() on a created local chunk specifying ETrue. 
//!		Following this call UserHeap::ChunkHeap();
//! 11.	Call TChunkHeapCreateInfo::SetAlignment() on a created local chunk specifying aAlign to 
//!		be a valid value. Following this call UserHeap::ChunkHeap();
//! 12.	Call TChunkHeapCreateInfo::SetAlignment() on a created local chunk specifying aAlign to be 
//!		an invalid value (-1). Following this call UserHeap::ChunkHeap();
//! 13.	Call TChunkHeapCreateInfo::SetGrowBy() on a created local chunk specifying aGrowBy to be 
//!		a valid value. Following this call UserHeap::ChunkHeap(). Following this create and array of 
//!		integers on the heap and check the size of the chunk. 
//!	14.	Call TChunkHeapCreateInfo::SetGrowBy() on a created local chunk specifying aGrowBy to
//!		be greater than the maximum size of the chunk. Following this call UserHeap::ChunkHeap();
//! 15.	Call TChunkHeapCreateInfo::SetOffset() on a created local chunk specifying aOffset to 
//!		be a valid value. Following this call UserHeap::ChunkHeap();
//! 16.	Call TChunkHeapCreateInfo:: SetMode () on a created local chunk specifying aMode to 
//!		be an invalid value. Following this call UserHeap::ChunkHeap();
//!
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestChunkHeapCreate(TInt aTestType)
	{
	

	test.Start(_L("Test UserHeap::ChunkHeap() data paging attributes"));
		{
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		
		UseOrCreateChunk(createInfo, aTestType);

		test(UserHeapAtt(gProcessPaged, createInfo));

		createInfo.SetPaging(TChunkHeapCreateInfo::EPaged);
		test(UserHeapAtt(ETrue, createInfo));
		
		createInfo.SetPaging(TChunkHeapCreateInfo::EUnpaged);
		test(UserHeapAtt(EFalse, createInfo));
		}

	test.Next(_L("Test maxHeapSize < minHeapSize"));
		{
		TChunkHeapCreateInfo createInfo(KMinHeapSize + gPageSize, KMinHeapSize);
		UseOrCreateChunk(createInfo, aTestType);
		RThread thread;
		test_KErrNone(thread.Create(_L("Panic UserHeap"), PanicChunkHeapCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(thread, EExitPanic, ETHeapCreateMaxLessThanMin));
		}

	test.Next(_L("Test invalid minHeapSize"));
		{
		TChunkHeapCreateInfo createInfo(-1, KMinHeapSize);
		UseOrCreateChunk(createInfo, aTestType);
		RThread thread;
		test_KErrNone(thread.Create(_L("Panic UserHeap"), PanicChunkHeapCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(thread, EExitPanic, ETHeapMinLengthNegative));
		}

	test.Next(_L("Test invalid maxHeapSize"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, 0);
		UseOrCreateChunk(createInfo, aTestType);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		if (heap == NULL)
			{
			test.Printf(_L("RHeap not created"));
			}
		TUint maxLength = heap->MaxLength();
		test.Printf(_L("heap max length = 0x%x\n"), maxLength);

		// Need to round up to page size as RHeap maxLength is rounded 
		// up to the nearest page size
		TUint expectedMaxSize = _ALIGN_UP(KMinHeapSize, gPageSize); 
		test_Equal(expectedMaxSize, maxLength);
		heap->Close();
		}

	test.Next(_L("Test SetUseChunk - calling SetCreate before"));
		{
		RHeap* heap;
		RChunk chunky;
		
		TChunkCreateInfo chunkCreateInfo;
		chunkCreateInfo.SetNormal(gPageSize, gPageSize);
		test_KErrNone(chunky.Create(chunkCreateInfo));

		TChunkHeapCreateInfo createInfo(0, gPageSize);

		createInfo.SetCreateChunk(NULL);
		createInfo.SetUseChunk(chunky);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		if (heap == NULL)
			{
			test.Printf(_L("RHeap not created\n"));
			}
		test.Printf(_L("chunkHandle = %d heapHandle = %d\n"),chunky.Handle(), ((TestHybridHeap*) heap)->ChunkHandle());
		test_Equal(chunky.Handle(), ((TestHybridHeap*) heap)->ChunkHandle());
		heap->Close();
		}

	test.Next(_L("Test SetUseChunk - calling SetCreate after"));
		{
		RHeap* heap;
		RChunk chunky;
		
		TChunkCreateInfo chunkCreateInfo;
		chunkCreateInfo.SetNormal(gPageSize, gPageSize);
		test_KErrNone(chunky.Create(chunkCreateInfo));

		TChunkHeapCreateInfo createInfo(0, gPageSize);
		createInfo.SetUseChunk(chunky);
		createInfo.SetCreateChunk(NULL);
		
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		if (heap == NULL)
			{
			test.Printf(_L("RHeap not created\n"));
			}
		test.Printf(_L("chunkHandle = %d heapHandle = %d\n"),chunky.Handle(), ((TestHybridHeap*) heap)->ChunkHandle());
		TBool isSame = EFalse;
		if (chunky.Handle() == ((TestHybridHeap*) heap)->ChunkHandle())
			isSame = ETrue;
		test_Equal(EFalse, isSame);
		heap->Close();
		}

	test.Next(_L("Test SetSingleThread - ETrue"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetSingleThread(ETrue);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		test_NotNull(heap);	
		heap->Close();
		}

	test.Next(_L("Test SetSingleThread - EFalse"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetSingleThread(EFalse);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		test_NotNull(heap);
		heap->Close();
		}

	test.Next(_L("Test SetAlignment 0"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetAlignment(0);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		test_NotNull(heap);
		heap->Close();
		}

	
	test.Next(_L("Test SetAlignment -1"));
		{
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetAlignment(-1);

		RThread thread;
		test_KErrNone(thread.Create(_L("Panic UserHeap"), PanicChunkHeapCreate, KDefaultStackSize, KMinHeapSize, 
															KMinHeapSize,  (TAny*) &createInfo));

		test_KErrNone(TestThreadExit(thread, EExitPanic, ETHeapNewBadAlignment));
		}


	test.Next(_L("Test SetGrowby"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize * 10);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetGrowBy(gPageSize);
		createInfo.SetMode(UserHeap::EChunkHeapSwitchTo);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		test_NotNull(heap);
		RChunk chunk;
		chunk.SetHandle(((TestHybridHeap*) heap)->ChunkHandle());
		TInt* numBuf = new TInt[gPageSize];
		test_NotNull(numBuf);
		test.Printf(_L("chunkSize = %d\n"), chunk.Size());
		test(chunk.Size() > KMinHeapGrowBy);
		delete numBuf;
		heap->Close();
		}

	test.Next(_L("Test SetGrowby growBy > maxSize"));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize * 5);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetGrowBy(gPageSize * 6);
		createInfo.SetMode(UserHeap::EChunkHeapSwitchTo);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		test_NotNull(heap);
		RChunk chunk;
		chunk.SetHandle(((TestHybridHeap*) heap)->ChunkHandle());
		TInt* numBuf = new TInt[gPageSize];
		test_Equal(NULL, numBuf);

		delete numBuf;
		heap->Close();
		}


	test.Next(_L("Test SetOffset "));
		{
		RHeap* heap;
		TChunkHeapCreateInfo createInfo(0, gPageSize * 10);
		UseOrCreateChunk(createInfo, aTestType);
		createInfo.SetOffset(8);
		createInfo.SetMode(UserHeap::EChunkHeapSwitchTo);
		heap = (RHeap*)UserHeap::ChunkHeap(createInfo);
		TInt heapAddr = (TInt)heap;
		RChunk chunk;
		chunk.SetHandle(((TestHybridHeap*) heap)->ChunkHandle());
		test_Equal((TInt)chunk.Base() + 8, heapAddr);
		test_NotNull(heap);
		heap->Close();
		}

	test.Next(_L("Test UserHeap::ChunkHeap() SetMode() invalid"));
		{
		TChunkHeapCreateInfo createInfo(0, gPageSize);
		UseOrCreateChunk(createInfo, aTestType);
		
		// Test creating a UserHeap with invalid attributes panics.
		RThread thread;
		test_KErrNone(thread.Create(_L("Panic UserHeap"), PanicUserHeap, KDefaultStackSize, 
									KMinHeapSize, KMinHeapSize, NULL));
		test_KErrNone(TestThreadExit(thread, EExitPanic, EHeapCreateInvalidMode));
		}

	test.End();
	}

TInt TestingTChunkHeapCreate()
	{
	test.Start(_L("Test TChunkHeapCreateInfo - heap within local chunk"));
	TestChunkHeapCreate(ETestLocal);

	test.Next(_L("Test TChunkHeapCreateInfo - heap within global chunk"));
	TestChunkHeapCreate(ETestGlobal);

	test.Next(_L("Test TChunkHeapCreateInfo - heap within existing chunk"));
	TestChunkHeapCreate(ETestExisting);

	test.End();	
	return 0;
	}
