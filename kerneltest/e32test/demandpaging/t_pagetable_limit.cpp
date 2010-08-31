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
// e32test\demandpaging\t_pagetable_limit.cpp
// Tests to expose the limit of page table virtual address space.
// 
//

//! @SYMTestCaseID			KBASE-T_PAGETABLE_LIMIT
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1490
//! @SYMTestCaseDesc		Tests to expose the limit of page table virtual address space.
//! @SYMTestActions			Test that a paged page table can always be acquired.
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32svr.h>
#include <u32std.h>
#include <hal.h>

#include "t_dpcmn.h"
#include "../mmu/freeram.h"

RTest test(_L("T_PAGETABLE_LIMIT"));

// The flexible memory model reserves 0xF800000-0xFFF00000 for page tables, which allows 130,048
// pages tables.
//
// We attempt to map 40 * 40 * 100 == 160,000 page tables.
// 
// So that the limit is reached in the middle, half the chunks are mapped as paged and half as
// unpaged.

const TUint KPageTablesPerChunk = 40;
const TUint KChunksPerProcess = 40;
const TUint KNumProcesses = 100;

const TUint KSizeMappedByPageTable = 1024 * 1024;  // the amount of RAM mapped by one page table


_LIT(KClientPtServerName, "CClientPtServer");
_LIT(KClientProcessName, "T_PAGETABLE_LIMIT");

enum TClientMsgType
	{
	EClientConnect = -1,
	EClientDisconnect = -2,
	EClientGetChunk = 0,
	EClientReadChunks = 1,
	EClientGetParentProcess = 2,
	};

class RDataPagingSession : public RSessionBase
	{
public:
	TInt CreateSession(const TDesC& aServerName, TInt aMsgSlots) 
		{ 
		return RSessionBase::CreateSession(aServerName,User::Version(),aMsgSlots);
		}
	TInt PublicSendReceive(TInt aFunction, const TIpcArgs &aPtr)
		{
		return SendReceive(aFunction, aPtr);
		}
	};

#define CLIENT_TEST_IMPL(condition, code, line)			\
	if (!(condition))									\
		{												\
		RDebug::Printf("Test %s failed at line %d");	\
		r = (code);										\
		goto exit;										\
		}

#define CLIENT_TEST(condition, code) CLIENT_TEST_IMPL(condition, code, __LINE__)

TInt ClientProcess(TInt aLen)
	{
	// Read the command line to get the number of chunk to map and whether or not to access their
	// data.
	HBufC* buf = HBufC::New(aLen);
	test(buf != NULL);
	TPtr ptr = buf->Des();
	User::CommandLine(ptr);
	TLex lex(ptr);

	RChunk* chunks = NULL;
	RDataPagingSession session;
	RProcess parent;
	TRequestStatus parentStatus;
	TInt offset = 0;
	TInt i;
	
	TInt chunkCount;
	TInt r = lex.Val(chunkCount);
	CLIENT_TEST(r == KErrNone, r);	
	lex.SkipSpace();
	chunks = new RChunk[chunkCount];
	CLIENT_TEST(chunks, KErrNoMemory);

	TBool accessData;
	r = lex.Val(accessData);
	CLIENT_TEST(r == KErrNone, r);

	r = session.CreateSession(KClientPtServerName, 1);
	CLIENT_TEST(r == KErrNone, r);

	r = parent.SetReturnedHandle(session.PublicSendReceive(EClientGetParentProcess, TIpcArgs()));
	CLIENT_TEST(r == KErrNone, r);
	
	for (i = 0; i < chunkCount; i++)
		{
		r = chunks[i].SetReturnedHandle(session.PublicSendReceive(EClientGetChunk, TIpcArgs(i)));
		if (r != KErrNone)
			{
			RDebug::Printf("Failed to create a handle to chunk %d r=%d", i, r);
			goto exit;
			}
		CLIENT_TEST(chunks[i].Size() >= gPageSize, KErrGeneral);
		}

	// Logon to parent process
	parent.Logon(parentStatus);

	// Touch each mapped page of all of the chunks.
	do
		{
		for (TInt i = 0; i < chunkCount; i++)
			{
			for (TUint j = 0 ; j < KPageTablesPerChunk ; j++)
				{
				// Write the chunk data from top to bottom of the chunk's first page.
				TUint8* base = chunks[i].Base() + j * KSizeMappedByPageTable;
				TUint8* end = base + gPageSize - 1;
				*(base + offset) = *(end - offset);

				User::After(0);

				// Check whether main process is still running
				if (parentStatus != KRequestPending)
					{
					// if we get here the test failed and the main process exited without killing
					// us, so just exit quietly
					User::WaitForRequest(parentStatus);
					r = KErrGeneral;
					goto exit;
					}
				}
			}
		offset = (offset + 1) % (gPageSize / 2);
		}
	while (accessData);

	// Tell parent we've touched each page.
	r = (TThreadId)session.PublicSendReceive(EClientReadChunks,TIpcArgs());	
	CLIENT_TEST(r == KErrNone, r);
		
	// Wait till we get killed by the main process or the main process dies
	User::WaitForRequest(parentStatus);
	// if we get here the test failed and the main process exited without killing us, so just exit
	r = KErrGeneral;

exit:
	if (chunks)
		{
		for (TInt i = 0 ; i < chunkCount; ++i)
			chunks[i].Close();
		}
	session.Close();
	parent.Close();
			
	return r;
	}

TInt FreeSwap()
	{
	SVMSwapInfo swapInfo;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo, 0));
	return swapInfo.iSwapFree;
	}

void PrintFreeRam()
	{
	test.Printf(_L("%d KB RAM / %d KB swap free\n"), FreeRam() / 1024, FreeSwap() / 1024);
	}

void CreateChunk(RChunk& aChunk, TInt aChunkIndex, TInt aPageTables, TBool aPaged)
	{
	// Creates a global chunk.
	//
	// The chunk uses KPageTablesPerChunk page tables by committing that number of pages at 1MB
	// intervals.  Its max size is set so that it is not a multiple of 1MB and hence the FMM will
	// use a fine mapping objects whose page tables are not shared between processes.
	
	test.Printf(_L("  creating chunk %d: "), aChunkIndex);
	PrintFreeRam();
	
	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(0, 0, aPageTables * KSizeMappedByPageTable + gPageSize);
	createInfo.SetPaging(aPaged ? TChunkCreateInfo::EPaged : TChunkCreateInfo::EUnpaged);
	TBuf<32> name;
	name.AppendFormat(_L("t_pagetable_limit chunk %d"), aChunkIndex);
	createInfo.SetGlobal(name);
	test_KErrNone(aChunk.Create(createInfo));
	for (TInt i = 0 ; i < aPageTables ; ++i)
		test_KErrNone(aChunk.Commit(i * KSizeMappedByPageTable, gPageSize));
	}

void CreateProcess(RProcess& aProcess, TInt aProcessIndex, TInt aNumChunks, TBool aAccessData)
	{
	test.Printf(_L("  creating process %d: "), aProcessIndex);
	PrintFreeRam();
	
	TBuf<80> args;
	args.AppendFormat(_L("%d %d"), aNumChunks, aAccessData);
	test_KErrNone(aProcess.Create(KClientProcessName, args));
	aProcess.SetPriority(EPriorityLow);
	}

void TestMaxPt()
	{
	test.Printf(_L("Waiting for system idle and kernel cleanup\n"));
	// If this test was run previously, there may be lots of dead processes waiting to be cleaned up
	TInt r;
	while((r = FreeRam(10 * 1000)), r == KErrTimedOut)
		{
		test.Printf(_L("  waiting: "));
		PrintFreeRam();
		}

	// Remove the maximum limit on the cache size as the test requires that it can
	// allocate as many page tables as possible but without stealing any pages as
	// stealing pages may indirectly steal paged page table pages.
	test.Printf(_L("Set paging cache max size unlimited\n"));
	TUint minCacheSize, maxCacheSize, currentCacheSize;
	test_KErrNone(DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize));
	test_KErrNone(DPTest::SetCacheSize(minCacheSize, KMaxTUint));

	const TInt KMinFreeRam = (1000 * gPageSize) + (130048 * (gPageSize>>2));

	// Ensure enough RAM available
	PrintFreeRam();
	TInt freeRam = FreeRam();
	if (freeRam < KMinFreeRam)
		{
		test.Printf(_L("Only 0x%x bytes of free RAM not enough to perform the test.  Skipping test.\n"), freeRam);
		return;
		}

	test.Printf(_L("Start server\n"));
	RServer2 ptServer;
	r = ptServer.CreateGlobal(KClientPtServerName);
	test_KErrNone(r);

	test.Printf(_L("Create chunks\n"));
	const TUint KPagedChunksStart = (KChunksPerProcess >> 1);
	RChunk* chunks = new RChunk[KChunksPerProcess];
	test_NotNull(chunks);
	TUint i = 0;
	for (i = 0 ; i< KChunksPerProcess; i++)
		CreateChunk(chunks[i], i, KPageTablesPerChunk, i >= KPagedChunksStart);

	// Start remote processes, giving each process handles to each chunk.
	test.Printf(_L("Start remote processes\n"));
	RProcess* processes = new RProcess[KNumProcesses];
	test_NotNull(processes);
	TRequestStatus* statuses = new TRequestStatus[KNumProcesses];
	test_NotNull(statuses);
	TUint processIndex = 0;	
	for (; processIndex < KNumProcesses; processIndex++)
		{
		// Start the process.
		CreateProcess(processes[processIndex], processIndex, KChunksPerProcess, EFalse);
		
		// logon to process
		processes[processIndex].Logon(statuses[processIndex]);
		test_Equal(KRequestPending, statuses[processIndex].Int());
		processes[processIndex].Resume();

		// wait for connect message
		RMessage2 ptMessage;
		ptServer.Receive(ptMessage);
		test_Equal(EClientConnect, ptMessage.Function());
		ptMessage.Complete(KErrNone);

		// pass client a handle to this process
		ptServer.Receive(ptMessage);
		test_Equal(EClientGetParentProcess, ptMessage.Function());
		ptMessage.Complete(RProcess());

		// pass client chunk handles
		TInt func;
		TUint chunkIndex = 0;
		for (; chunkIndex < KChunksPerProcess ; chunkIndex++)
			{// Pass handles to all the unpaged chunks to the new process.
			ptServer.Receive(ptMessage);
			func = ptMessage.Function();
			if (func != EClientGetChunk)
				break;
			ptMessage.Complete(chunks[ptMessage.Int0()]);
			}
		if (func != EClientGetChunk)
			{
			// Should hit the limit of page tables and this process instance should exit
			// sending a disconnect message in the process.
			test_Equal(EClientDisconnect, func);
			// Should only fail when mapping unpaged chunks.
			test_Value(chunkIndex, chunkIndex < (KChunksPerProcess >> 1));
			break;
			}
		
		// Wait for the process to access all the chunks and therefore 
		// allocate the paged page tables before moving onto the next process.
		ptServer.Receive(ptMessage);
		func = ptMessage.Function();
		test_Equal(EClientReadChunks, func);
		ptMessage.Complete(KErrNone);

		// Should have mapped all the required chunks.
		test_Equal(KChunksPerProcess, chunkIndex);
		}
	
	// Should hit page table limit before KNumProcesses have been created.
	test_Value(processIndex, processIndex < KNumProcesses - 1);
	TUint processLimit = processIndex;

	// Now create more processes to access paged data even though the page table address space has
	// been exhausted.  Limit to 10 more processes as test takes long enough already.
	test.Printf(_L("Start accessor processes\n"));
	processIndex++;
	TUint excessProcesses = KNumProcesses - processIndex;
	TUint pagedIndexEnd = (excessProcesses > 10)? processIndex + 10 : processIndex + excessProcesses;
	for (; processIndex < pagedIndexEnd; processIndex++)
		{
		// start the process.
		CreateProcess(processes[processIndex], processIndex, KChunksPerProcess-KPagedChunksStart, ETrue);

		// logon to process
		processes[processIndex].Logon(statuses[processIndex]);
		test_Equal(KRequestPending, statuses[processIndex].Int());
		processes[processIndex].Resume();

		// wait for connect message
		RMessage2 ptMessage;
		ptServer.Receive(ptMessage);
		test_Equal(EClientConnect, ptMessage.Function());
		ptMessage.Complete(KErrNone);

		// pass client a handle to this process
		ptServer.Receive(ptMessage);
		test_Equal(EClientGetParentProcess, ptMessage.Function());
		ptMessage.Complete(RProcess());
		
		TInt func = EClientGetChunk;
		TUint chunkIndex = KPagedChunksStart;
		for (; chunkIndex < KChunksPerProcess && func == EClientGetChunk; chunkIndex++)
			{// Pass handles to all the paged chunks to the new process.
			ptServer.Receive(ptMessage);
			func = ptMessage.Function();
			if (func == EClientGetChunk)
				{
				TUint index = ptMessage.Int0() + KPagedChunksStart;
				ptMessage.Complete(chunks[index]);
				}
			}
		if (func != EClientGetChunk)
			{// Reached memory limits so exit.
			test_Equal(EClientDisconnect, func);
			// Should have created at least one more process.
			test_Value(processIndex, processIndex > processLimit+1);
			break;
			}

		// Should have mapped all the required chunks.
		test_Equal(KChunksPerProcess, chunkIndex);
		}
	
	// If we reached the end of then ensure that we kill only the running processes.
	if (processIndex == pagedIndexEnd)
		processIndex--;

	// Let the accessor processes run awhile
	test.Printf(_L("Waiting...\n"));
	User::After(10 * 1000000);
	
	// Kill all the remote processes
	test.Printf(_L("Killing processes...\n"));
	for(TInt j = processIndex; j >= 0; j--)
		{
		test.Printf(_L("  killing process %d\n"), j);
		if (statuses[j] == KRequestPending)
			processes[j].Kill(KErrNone);
		processes[j].Close();
		User::WaitForRequest(statuses[j]);
		}
	delete [] processes;
	delete [] statuses;
	
	// Close the chunks.
	for (TUint k = 0; k < KChunksPerProcess; k++)
		chunks[k].Close();
	delete[] chunks;

	// Reset live list size
	test_KErrNone(DPTest::SetCacheSize(minCacheSize, maxCacheSize));
	}


TInt E32Main()
	{
	test_KErrNone(UserHal::PageSizeInBytes(gPageSize));

	TUint len = User::CommandLineLength();
	if (len > 0)
		{
		return ClientProcess(len);
		}

	test.Title();
	test_KErrNone(GetGlobalPolicies());

	if (!gDataPagingSupported)
		{
		test.Printf(_L("Data paging not enabled so skipping test...\n"));
		return KErrNone;
		}
	
	test.Start(_L("Test the system can always acquire a paged page table"));
	TestMaxPt();
	
	test.End();
	return KErrNone;
	}
