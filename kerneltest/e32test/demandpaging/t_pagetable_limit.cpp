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

RTest test(_L("T_PAGETABLE_LIMIT"));


_LIT(KClientPtServerName, "CClientPtServer");
_LIT(KClientProcessName, "T_PAGETABLE_LIMIT");

enum TClientMsgType
	{
	EClientConnect = -1,
	EClientDisconnect = -2,
	EClientGetChunk = 0,
	EClientReadChunks = 1,
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
		return (SendReceive(aFunction, aPtr));
		}
	TInt PublicSend(TInt aFunction, const TIpcArgs &aPtr)
		{
		return (Send(aFunction, aPtr));
		}
	};


TInt ClientProcess(TInt aLen)
	{
	// Read the command line to get the number of chunk to map and whether or 
	// not to access their data.
	HBufC* buf = HBufC::New(aLen);
	test(buf != NULL);
	TPtr ptr = buf->Des();
	User::CommandLine(ptr);

	TLex lex(ptr);
	TInt chunkCount;
	TInt r = lex.Val(chunkCount);
	test_KErrNone(r);
	lex.SkipSpace();

	TBool accessData;
	r = lex.Val(accessData);
	test_KErrNone(r);


	RDataPagingSession session;
	test_KErrNone(session.CreateSession(KClientPtServerName, 1));

	RChunk* chunks = new RChunk[chunkCount];
	for (TInt i = 0; i < chunkCount; i++)
		{
		TInt r = chunks[i].SetReturnedHandle(session.PublicSendReceive(EClientGetChunk, TIpcArgs(i)));
		if (r != KErrNone)
			{
			test.Printf(_L("Failed to create a handle to the server's chunk r=%d\n"), r);
			for (TInt j = 0; j < i; j++)
				chunks[j].Close();
			session.Close();
			return r;
			}
		test_Value(chunks[i].Size(), chunks[i].Size() >= gPageSize);
		}
	if (!accessData)
		{
		// Touch the 1st page of each of the chunks.
		for (TInt i = 0; i < chunkCount; i++)
			{
			// Write the chunk data from top to bottom of the chunk's first page.
			TUint8* base = chunks[i].Base();
			TUint8* end = base + gPageSize - 1;
			*base = *end;
			}
		// Tell parent we've touched each chunk.
		TInt r =  (TThreadId)session.PublicSendReceive(EClientReadChunks,TIpcArgs());	// Assumes id is only 32-bit.
		test_KErrNone(r);
		for(;;)
			{// Wake up every 100ms to be killed by the main process.
			User::After(100000);
			}
		}
	else
		{
		for (;;)
			{
			TInt offset = 0;
			for (TInt i = 0; i < chunkCount; i++)
				{
				// Write the chunk data from top to bottom of the chunk's first page.
				TUint8* base = chunks[i].Base();
				TUint8* end = base + gPageSize - 1;
				*(base + offset) = *(end - offset);
				}
			if (++offset >= (gPageSize >> 1))
				offset = 0;
			}
		}
	}


void TestMaxPt()
	{
	// Flexible memory model reserves 0xF800000-0xFFF00000 for page tables
	// this allows 130,048 pages tables.  Therefore mapping 1000 one 
	// page chunks into 256 processes would require 256,000 page tables, i.e.
	// more than enough to hit the limit.  So that the limit is reached in the middle,
	// map 500 unpaged and 500 paged chunks in each process.
	const TUint KNumChunks = 1000;
	const TUint KPagedChunksStart = (KNumChunks >> 1);
	const TUint KNumProcesses = 256;
	const TInt KMinFreeRam = (1000 * gPageSize) + (130048 * (gPageSize>>2));
	TInt freeRam;
	HAL::Get(HALData::EMemoryRAMFree, freeRam);
	if (freeRam < KMinFreeRam)
		{
		test.Printf(_L("Only 0x%x bytes of free RAM not enough to perform the test.  Skipping test.\n"), freeRam);
		return;
		}

	// Remove the maximum limit on the cache size as the test requires that it can
	// allocate as many page tables as possible but without stealing any pages as
	// stealing pages may indirectly steal paged page table pages.
	TUint minCacheSize, maxCacheSize, currentCacheSize;
	DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
	test_KErrNone(DPTest::SetCacheSize(minCacheSize, KMaxTUint));

	RServer2 ptServer;
	TInt r = ptServer.CreateGlobal(KClientPtServerName);
	test_KErrNone(r);

	// Create the global unpaged chunks.  They have one page committed
	// but have a maximum size large enough to prevent their page tables being
	// shared between the chunks.  On arm with 4KB pages each page table maps 1MB
	// so make chunk 1MB+4KB so chunk requires 2 page tables and is not aligned on
	// a 1MB boundary so it is a fine memory object.
	const TUint KChunkSize = (1024 * 1024) + gPageSize;
	RChunk* chunks = new RChunk[KNumChunks];
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(gPageSize, KChunkSize);
	createInfo.SetGlobal(KNullDesC);
	createInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	TUint i = 0;
	for (; i < KPagedChunksStart; i++)
		{
		r = chunks[i].Create(createInfo);
		test_KErrNone(r);
		}
	// Create paged chunks.
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	for (; i< KNumChunks; i++)
		{
		r = chunks[i].Create(createInfo);
		test_KErrNone(r);
		}

	// Start remote processes, giving each process handles to each chunk.
	RProcess* processes = new RProcess[KNumProcesses];
	RMessage2 ptMessage;
	TUint processIndex = 0;
	TUint processLimit = 0;
	for (; processIndex < KNumProcesses; processIndex++)
		{
		// Start the process.
		test.Printf(_L("Creating process %d\n"), processIndex);
		TBuf<80> args;
		args.AppendFormat(_L("%d %d"), KNumChunks, EFalse);
		r = processes[processIndex].Create(KClientProcessName, args);
		test_KErrNone(r);
		TRequestStatus s;
		processes[processIndex].Logon(s);
		test_Equal(KRequestPending, s.Int());
		processes[processIndex].Resume();

		ptServer.Receive(ptMessage);
		test_Equal(EClientConnect, ptMessage.Function());
		ptMessage.Complete(KErrNone);
		TInt func = EClientGetChunk;
		TUint chunkIndex = 0;
		for (; chunkIndex < KNumChunks && func == EClientGetChunk; chunkIndex++)
			{// Pass handles to all the unpaged chunks to the new process.
			ptServer.Receive(ptMessage);
			func = ptMessage.Function();
			if (func == EClientGetChunk)
				{
				TUint index = ptMessage.Int0();
				ptMessage.Complete(chunks[index]);
				}
			}
		if (func != EClientGetChunk)
			{
			// Should hit the limit of page tables and this process instance should exit
			// sending a disconnect message in the process.
			test_Equal(EClientDisconnect, func);
			// Should only fail when mapping unpaged chunks.
			test_Value(chunkIndex, chunkIndex < (KNumChunks >> 1));
			break;
			}
		// Wait for the process to access all the chunks and therefore 
		// allocate the paged page tables before moving onto the next process.
		ptServer.Receive(ptMessage);
		func = ptMessage.Function();
		test_Equal(EClientReadChunks, func);
		ptMessage.Complete(KErrNone);

		// Should have mapped all the required chunks.
		test_Equal(KNumChunks, chunkIndex);
		}
	// Should hit page table limit before KNumProcesses have been created.
	test_Value(processIndex, processIndex < KNumProcesses - 1);
	processLimit = processIndex;

	// Now create more processes to access paged data even though the page table 
	// address space has been exhausted.  Limit to 10 more processes as test takes 
	// long enough already.
	processIndex++;
	TUint excessProcesses = KNumProcesses - processIndex;
	TUint pagedIndexEnd = (excessProcesses > 10)? processIndex + 10 : processIndex + excessProcesses;
	for (; processIndex < pagedIndexEnd; processIndex++)
		{
		// Start the process.
		test.Printf(_L("Creating process %d\n"), processIndex);
		TBuf<80> args;
		args.AppendFormat(_L("%d %d"), KNumChunks-KPagedChunksStart, ETrue);
		r = processes[processIndex].Create(KClientProcessName, args);
		if (r != KErrNone)
			{// Have hit the limit of processes.
			processIndex--;
			// Should have created at least one more process.
			test_Value(processIndex, processIndex > processLimit);
			break;
			}
		TRequestStatus s;
		processes[processIndex].Logon(s);
		test_Equal(KRequestPending, s.Int());
		processes[processIndex].Resume();

		ptServer.Receive(ptMessage);
		test_Equal(EClientConnect, ptMessage.Function());
		ptMessage.Complete(KErrNone);

		TInt func = EClientGetChunk;
		TUint chunkIndex = KPagedChunksStart;
		for (; chunkIndex < KNumChunks && func == EClientGetChunk; chunkIndex++)
			{// Pass handles to all the unpaged chunks to the new process.
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
		test_Equal(KNumChunks, chunkIndex);
		}
	// If we reached the end of then ensure that we kill only the running processes.
	if (processIndex == pagedIndexEnd)
		processIndex--;
	// Kill all the remote processes
	for(TInt j = processIndex; j >= 0; j--)
		{
		test.Printf(_L("killing process %d\n"), j);
		TRequestStatus req;
		processes[j].Logon(req);
		if (req == KRequestPending)
			{
			processes[j].Kill(KErrNone);
			User::WaitForRequest(req);
			}
		processes[j].Close();
		}
	delete[] processes;
	// Close the chunks.
	for (TUint k = 0; k < KNumChunks; k++)
		chunks[k].Close();
	delete[] chunks;
	
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
