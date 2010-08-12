// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_demandpaging.cpp
// Functional tests for demand paging. The test suite covers simple
// paging operations as well as HAL configuration and tuning functions.
// 001.01 DPTest::Attributes
// 001.02 DPTest::FlushCache
// 001.03 DPTest::CacheSize
// 001.04 DPTest::SetCacheSize
// 001.04.01 Changing size of flushed VM cache
// 001.04.02 Changing size of full VM cache
// 002 Loading test drivers
// 003 Test thread realtime state
// 003.01 Enable KREALTIME tracing
// 003.02 Test ERealtimeStateOff
// 003.03 Test ERealtimeStateOn
// 003.04 Test ERealtimeStateWarn
// 003.05 Test server with ERealtimeStateOff
// 003.06 Test server with ERealtimeStateOn
// 003.07 Test server with ERealtimeStateWarn
// 003.08 Disable KREALTIME tracing
// 004 Lock Test
// 005 Lock Test again
// 006 Test writing to paged ROM
// 007 Test IPC read from paged memory
// 007.01 Create server
// 007.02 IPC read from ROM
// 007.03 Stop server
// 008 Test contiguous RAM allocation reclaims paged memory
// 008.01 Start...
// 008.02 Contiguous RAM test: alloc size = 128K align = 16
// 008.03 Contiguous RAM test: alloc size = 128K align = 0
// 008.04 Contiguous RAM test: alloc size = 64K align = 15
// 008.05 Contiguous RAM test: alloc size = 64K align = 14
// 008.06 Contiguous RAM test: alloc size = 64K align = 13
// 008.07 Contiguous RAM test: alloc size = 64K align = 12
// 008.08 Contiguous RAM test: alloc size = 64K align = 0
// 008.09 Contiguous RAM test: alloc size = 8K align = 13
// 008.10 Contiguous RAM test: alloc size = 8K align = 12
// 008.11 Contiguous RAM test: alloc size = 8K align = 0
// 008.12 Contiguous RAM test: alloc size = 4K align = 13
// 008.13 Contiguous RAM test: alloc size = 4K align = 12
// 008.14 Contiguous RAM test: alloc size = 4K align = 0
// 009 Test no kernel faults when copying data from unpaged rom with mutex held
// 010 Close test driver
// 011 Test setting publish and subscribe properties from paged area
// 012 Rom Paging Benchmark
// 012.01 Benchmark ROM paging...
// 
//

//! @SYMTestCaseID			KBASE-T_DEMANDPAGING-0334
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging functional tests.
//! @SYMTestActions			001 Test HAL interface
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32kpan.h>
#include <e32property.h>
#include <e32rom.h>
#include <u32hal.h>
#include "d_memorytest.h"
#include "d_demandpaging.h"
#include "d_gobble.h"
#include "mmudetect.h"
#include "t_codepaging_dll.h"
#include "freeram.h"

RTest test(_L("T_DEMANDPAGING"));

_LIT(KTCodePagingDll4, "t_codepaging_dll4.dll");
const TInt KMinBufferSize = 16384;
const TInt KMaxIPCSize = 256*1024;

TInt PageSize = 0;
RDemandPagingTestLdd Ldd;
RLibrary PagedLibrary;

// A buffer containing paged memory, contents may or may not be paged in
const TUint8* LargeBuffer = NULL;
TInt LargeBufferSize = 0;

// A buffer containing paged memeory, contents always paged out before access
const TUint8* SmallBuffer = NULL;
TInt SmallBufferSize = 0;

// A shared buffer mapped to the global address range
TInt SharedBufferSize = KMaxIPCSize+4096;
TLinAddr SharedBufferAddr = 0;
TUint8* SharedBuffer = NULL;

// A descriptor whose header is in paged memory (actually just a pointer to a zero word)
TDesC8* PagedHeaderDes = NULL;

// An area of paged rom if rom paging is supported, or zero
TUint8* RomPagedBuffer = NULL;
TInt RomPagedBufferSize = 0;

// An area of paged code if code paging is supported, or zero
TUint8* CodePagedBuffer = NULL;
TInt CodePagedBufferSize = 0;

// A data paged chunk used as a buffer, if data paging is supported
_LIT(KChunkName, "t_demandpaging chunk");
RChunk DataPagedChunk;
TBool DataPagingSupported = EFalse;
TUint8* DataPagedBuffer = NULL;

TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}

TUint8 WriteByte(volatile TUint8* aPtr)
	{
	return *aPtr = 1;
	}

#define READ(a) ReadByte((volatile TUint8*)(a))
#define WRITE(a) WriteByte((volatile TUint8*)(a))

void ThrashPaging(TUint aBytes)
	{
	TUint size = LargeBufferSize;
	if(size > aBytes)
		size = aBytes;

	TUint readCount = 5 * size/PageSize;
	
	test.Printf(_L("ThrashPaging %u %u\n"), size, readCount);
	
	TUint32 random=1;
	for(TUint i = 0 ; i < readCount ; ++i)
		{
		READ(LargeBuffer+((TInt64(random)*TInt64(size))>>32));
		random = random*69069+1;
		}
	}

void FragmentPagingCache(TUint aMaxBytes)
	{
	DPTest::FlushCache();

	TUint size = Min(LargeBufferSize, aMaxBytes);
	if(size<aMaxBytes)
		test.Printf(_L("WARNING: LargeBuffer not large enough!  Have you built a full test ROM?\n"));

	RChunk chunk;
	test(KErrNone==chunk.CreateDisconnectedLocal(0,0,size));

	TUint32 random = 0;
	for(TUint i=0; i<size; i += PageSize)
		{
		random = random*69069+1;
		if(random<0x40000000)
			chunk.Commit(i,PageSize); // to make paging cache fragmented
		READ(LargeBuffer + i);
		}

	CLOSE_AND_WAIT(chunk);

	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	}


void RomPagingBenchmark()
	{
	TInt r=DPTest::FlushCache();
	if(r!=KErrNone)
		return;

	test.Start(_L("Benchmark ROM paging..."));

	// change live list to be a small as possible
	test(KErrNone==DPTest::SetCacheSize(1,1));

	RTimer timer;
	test(KErrNone==timer.CreateLocal());
	TRequestStatus status;
	timer.After(status,1);
	User::WaitForRequest(status);

	TPckgBuf<DPTest::TEventInfo> events0;
	DPTest::EventInfo(events0);

	TInt KRunTime = 10*1000*1000;
	timer.After(status,KRunTime);
	while(status==KRequestPending)
		for(const TUint8* ptr=LargeBuffer; ptr<(LargeBuffer+LargeBufferSize); ptr+=PageSize)
			{
			READ(ptr);
			if(status!=KRequestPending)
				break;
			}

	TPckgBuf<DPTest::TEventInfo> events1;
	DPTest::EventInfo(events1);

	User::WaitForRequest(status);

	TUint pages = events1().iPageInReadCount-events0().iPageInReadCount;
	test.Printf(_L("%d pages in %d seconds = %d us/page\n"),pages,KRunTime/1000/1000,KRunTime/pages);

	// restore live list to default size...
	test(KErrNone==DPTest::SetCacheSize(0,0));

	test.End();
	}


TInt SetCacheSize(TUint aNewMin, TUint aNewMax)
	{
	// set cache size and test 'get' function returns expected values
	TInt r = DPTest::SetCacheSize(aNewMin, aNewMax);
	if (r == KErrNone)
		{
		TUint min, max, current;
		test_KErrNone(DPTest::CacheSize(min,max,current));
		test_Equal(aNewMin, min);
		test_Equal(aNewMax, max);
		test(current >= min && current <= max);
		}
	return r;
	}

void DoResizeCache(TUint min, TUint max, TInt result, TUint& sizeMin, TUint& sizeMax, TUint originalMin, TUint originalMax)
	{
	test.Printf(_L("DPTest::SetCacheSize min=%u, max=%u, expected result=%d\n"),min/PageSize,max/PageSize,result);
	TInt r=SetCacheSize(min,max);
	test_Equal(result, r);
	if(r==KErrNone)
		{
		// we've successfully changed the cache size...
		if(max)
			{
			sizeMin = min;
			sizeMax = max;
			}
		else
			{
			sizeMin = originalMin;
			sizeMax = originalMax;
			}
		}
	else if(r==KErrNoMemory)
		{
		// cache size after OOM is unpredictable, so reset our values
		test_KErrNone(SetCacheSize(sizeMin,sizeMax));
		}
	}


void TestResizeVMCache()
	{
	test.Start(_L("Test resizing VM cache"));
	TInt r = DPTest::SetCacheSize(0,0); // restore cache size to defaults
	test(r==KErrNone);
	TUint sizeMin = 0;
	TUint sizeMax = 0;
	TUint currentSize = 0;
	DPTest::CacheSize(sizeMin,sizeMax,currentSize);
	TUint originalMin = sizeMin;
	TUint originalMax = sizeMax;
	test.Printf(_L("original min=%u, original max=%u, current=%u\n"),
				originalMin/PageSize,originalMax/PageSize,currentSize/PageSize);

	int K = currentSize/PageSize+4;

	// Exercise the cache reszing code by testing all valid combinations of the relationships
	// between the current min size, current max size, new min size and new max size.
	//
	// This can be done using four cache size values.  Every assignment of these four values to the
	// four variables is generated, and invalid combinations rejected.  This repeats some
	// relationships but is simpler than calculating the minimum set of combinations exactly.

	const TUint combinations = 256;  // 4 ^ 4
	const TUint sizes[] = { K, K + 4, K + 8, K + 12, K + 16 };
	for (TUint perm = 0 ; perm < combinations ; ++perm)
		{
		TUint vars[4] = { sizes[ perm & 3 ],
						  sizes[ (perm >> 2) & 3 ],
						  sizes[ (perm >> 4) & 3 ],
						  sizes[ (perm >> 6) & 3 ]};
		if ((vars[0] == vars[2] && vars[1] == vars[3]) || // ensure current != new
			vars[0] > vars[1] ||                          // ensure current min <= current max
			vars[2] > vars[3])                            // ensure new min <= new max
			continue;
		
		test.Printf(_L("Test changing cache sizes from %u, %u to %u %u\n"),
					vars[0], vars[1], vars[2], vars[3]);
		
		test_KErrNone(SetCacheSize(PageSize * vars[0], PageSize * vars[1]));
		ThrashPaging(PageSize * vars[1]);
		test_KErrNone(SetCacheSize(PageSize * vars[2], PageSize * vars[3]));
		}
	test_KErrNone(SetCacheSize(originalMin, originalMax));

	// Now test some more specific resizings
	
	struct
		{
		TUint iMinPages;
		TUint iMaxPages;
		TInt iResult;
		}
	testArgs[] =
		{
			{	K,		K,		KErrNone},
			{	K-4,	K,		KErrNone},
			{	K,		K,		KErrNone},
			{	K,		K*2,	KErrNone},
			{	K,		K,		KErrNone},
			{	K-1,	K,		KErrNone},
			{	K,		K,		KErrNone},
			{	K,		K+1,	KErrNone},
			{	K,		K,		KErrNone},
			{	K+1,	K,		KErrArgument},
			{	K,		K-1,	KErrArgument},
			{	K,		K,		KErrNone},
			{	KMaxTInt,	KMaxTInt,	KErrNoMemory},
			{	K,		K,		KErrNone},

			{	0,		0,		KMaxTInt} // list end marker
		};

	for(TInt j=0; j<2; ++j)
		{
		if(!j)
			test.Next(_L("Changing size of empty VM cache"));
		else
			test.Next(_L("Changing size of full VM cache"));
		
		TInt i=0;
		while(testArgs[i].iResult!=KMaxTInt)
			{
			TUint min=testArgs[i].iMinPages*PageSize;
			TUint max=testArgs[i].iMaxPages*PageSize;
			TInt result=testArgs[i].iResult;

			if(!j)
				DPTest::FlushCache();
			else
				ThrashPaging(max);

			DoResizeCache(min, max, result, sizeMin, sizeMax, originalMin, originalMax);
			
			++i;
			}
		}
	
	test_KErrNone(SetCacheSize(originalMin, originalMax));
	test.End();
	}

void TestResizeVMCache2()
	{
	TUint originalMin = 0;
	TUint originalMax = 0;
	TUint currentSize = 0;
	test_KErrNone(DPTest::CacheSize(originalMax, originalMax, currentSize));
	test_KErrNone(DPTest::SetCacheSize(1, originalMax));
	TUint sizeMin = 0;
	TUint sizeMax = 0;
	test_KErrNone(DPTest::CacheSize(sizeMin, sizeMax, currentSize));
	test(sizeMin > 1);
	test_KErrNone(DPTest::SetCacheSize(originalMin, originalMax));
	}


void TestHAL()
	{
	test.Start(_L("DPTest::Attributes"));
	TUint32 attr=DPTest::Attributes();
	test.Printf(_L("Attributes = %08x\n"),attr);

	test.Next(_L("DPTest::FlushCache"));
	TInt r=DPTest::FlushCache();
	if(r==KErrNotSupported)
		test.Printf(_L("Not Supported\n"));
	else if(r<0)
		{
		test.Printf(_L("Error = %d\n"),r);
		test(0);
		}

	test.Next(_L("DPTest::CacheSize"));
	TUint oldMin = 0;
	TUint oldMax = 0;
	TUint currentSize = 0;
	r=DPTest::CacheSize(oldMin,oldMax,currentSize);
	if(r==KErrNotSupported)
		test.Printf(_L("Not Supported\n"));
	else if(r<0)
		{
		test.Printf(_L("Error = %d\n"),r);
		test(0);
		}
	else
		{
		test.Printf(_L("Size = %dk,%dk,%dk\n"),oldMin>>10,oldMax>>10,currentSize>>10);
		}

	test.Next(_L("DPTest::SetCacheSize"));
	r=DPTest::SetCacheSize(oldMin,oldMax);
	if(r==KErrNotSupported)
		test.Printf(_L("Not Supported\n"));
	else if(r<0)
		{
		test.Printf(_L("Error = %d\n"),r);
		test(0);
		}
	if(r==KErrNone)
		{
		TestResizeVMCache();
		TestResizeVMCache2();
		}

	test.End();
	}

// Test IPC and realtime state

enum TIpcDir
	{
	EServerRead,
	EServerWrite
	};

enum TIpcObjectPaged
	{
	ENothingPaged,
	EDesHeaderPaged,
	EDesContentPaged
	};

enum TThreadOutcome
	{
	ENoError,
	EBadDescriptor,
	EServerTerminated,
	ERealtimePanic,
	EAbortPanic
	};

class RTestSession : public RSessionBase
	{
public:
	TInt Create(RServer2 aServer)
		{
		return CreateSession(aServer,TVersion(),-1);
		}
	inline TInt Send(const TIpcArgs& aArgs)
		{
		return RSessionBase::SendReceive(0,aArgs);
		}
	};

RServer2 TestServer;

TInt IpcTestServerFunc(TAny* aArg)
	{
	TIpcDir dir = (TIpcDir)(((TInt)aArg) & 0xff);
	TIpcObjectPaged paged = (TIpcObjectPaged)((((TInt)aArg) >> 8) & 0xff);
	User::TRealtimeState realtime = (User::TRealtimeState)((((TInt)aArg) >> 16) & 0xff);
	User::TRealtimeState clientRealtime = (User::TRealtimeState)((((TInt)aArg) >> 24) & 0xff);

	TInt r;
	// We want the server to fault the client when it is realtime 
	// and accessing paged out memory.
	r = TestServer.CreateGlobal(KNullDesC, EIpcSession_Sharable, EServerRole_Default, EServerOpt_PinClientDescriptorsDisable);
	if (r != KErrNone)
		return r;
	RThread::Rendezvous(KErrNone);
	
	RMessage2 message;
	TestServer.Receive(message);
	if ((clientRealtime == User::ERealtimeStateOn) != message.ClientIsRealtime())
		return KErrGeneral;
	message.Complete(KErrNone); // complete connection request

	TRequestStatus s;
	TestServer.Receive(message,s);
	User::WaitForRequest(s);
	if (s != KErrNone)
		return s.Int();

	TInt32 unpagedContent;
	TPtr8 unpagedDes((TUint8*)&unpagedContent, 4, 4);
	TPtrC8 pagedContentBuf(SmallBuffer,sizeof(TInt));

	TPtr8* dataPagedHeaderDes = (TPtr8*)DataPagedBuffer;
	if (DataPagingSupported)
		new (dataPagedHeaderDes) TPtr8((TUint8*)&unpagedContent, 4);
	
	TPtr8 dataPagedContentDes(DataPagedBuffer + PageSize, 4);
				
	r = DPTest::FlushCache();
	if(r != KErrNone)
		return r;
	
	User::SetRealtimeState(realtime);
	if (dir == EServerRead)
		{
		switch (paged)
			{
			case ENothingPaged:
				r = message.Read(0,unpagedDes);
				break;
				
			case EDesHeaderPaged:
				r = DataPagingSupported ? message.Read(0,*dataPagedHeaderDes) : KErrNotSupported;
				break;
				
			case EDesContentPaged:
				r = DataPagingSupported ? message.Read(0,dataPagedContentDes) : KErrNotSupported;
				break;

			default:
				r = KErrArgument;
				break;
			}
		}
	else if (dir == EServerWrite)
		{
		switch (paged)
			{
			case ENothingPaged:
				r = message.Write(0,unpagedDes);
				break;
				
			case EDesHeaderPaged:
				r = message.Write(0,*PagedHeaderDes);
				break;
				
			case EDesContentPaged:
				r = message.Write(0,pagedContentBuf);
				break;

			default:
				r = KErrArgument;
				break;
			}
		}
	else
		r = KErrArgument;
	User::SetRealtimeState(User::ERealtimeStateOff);

	message.Complete(KErrNone);
	return r;
	}

TInt IpcTestClientFunc(TAny* aArg)
	{
	TIpcDir dir = (TIpcDir)(((TInt)aArg) & 0xff);
	TIpcObjectPaged paged = (TIpcObjectPaged)((((TInt)aArg) >> 8) & 0xff);
	User::TRealtimeState realtime = (User::TRealtimeState)((((TInt)aArg) >> 16) & 0xff);

	RTestSession session;
	TInt r = session.Create(TestServer);
	if(r != KErrNone)
		return r;

	TInt32 unpagedContent;
	TPtr8 unpagedDes((TUint8*)&unpagedContent, 4, 4);
	TPtrC8 pagedContentBuf(SmallBuffer + PageSize, sizeof(TInt));

	TPtr8* dataPagedHeaderDes = (TPtr8*)(DataPagedBuffer + (2 * PageSize));
	if (DataPagingSupported)
		new (dataPagedHeaderDes) TPtr8((TUint8*)&unpagedContent, 4);
	
	TPtr8 dataPagedContentDes(DataPagedBuffer + (3 * PageSize), 4);

	r = DPTest::FlushCache();
	if(r != KErrNone)
		return r;
	
	User::SetRealtimeState(realtime);
	if (dir == EServerRead)
		{
		switch (paged)
			{
			case ENothingPaged:
				r = session.Send(TIpcArgs(&unpagedDes));
				break;
				
			case EDesHeaderPaged:
				r = session.Send(TIpcArgs(PagedHeaderDes));
				break;
				
			case EDesContentPaged:
				r = session.Send(TIpcArgs(&pagedContentBuf));
				break;

			default:
				r = KErrArgument;
				break;
			}
		}
	else if (dir == EServerWrite)
		{
		switch (paged)
			{
			case ENothingPaged:
				r = session.Send(TIpcArgs(&unpagedDes));
				break;
				
			case EDesHeaderPaged:
				r = DataPagingSupported ? session.Send(TIpcArgs(dataPagedHeaderDes)) : KErrNotSupported;
				break;
				
			case EDesContentPaged:
				r = DataPagingSupported ? session.Send(TIpcArgs(&dataPagedContentDes)) : KErrNotSupported;
				break;

			default:
				r = KErrArgument;
				break;
			}
		}
	else
		r = KErrArgument;
	User::SetRealtimeState(User::ERealtimeStateOff);

	session.Close();
	return r;
	}

void TestThreadOutcome(RThread aThread, TThreadOutcome aOutcome)
	{
	switch(aOutcome)
		{
		case ENoError:
			test_Equal(EExitKill, aThread.ExitType());
			test_KErrNone(aThread.ExitReason());
			break;
			
		case EBadDescriptor:
			test_Equal(EExitKill, aThread.ExitType());
			test_Equal(KErrBadDescriptor, aThread.ExitReason());
			break;

		case EServerTerminated:
			test_Equal(EExitKill, aThread.ExitType());
			test_Equal(KErrServerTerminated, aThread.ExitReason());
			break;

		case ERealtimePanic:
			test_Equal(EExitPanic, aThread.ExitType());
			test(aThread.ExitCategory()==_L("KERN-EXEC"));
			test_Equal(EIllegalFunctionForRealtimeThread, aThread.ExitReason());
			break;

		case EAbortPanic:
			test_Equal(EExitPanic, aThread.ExitType());
			// category for paging errors tested elsewhere
			test_Equal(KErrAbort, aThread.ExitReason());
			break;

		default:
			test(EFalse);
		}
	}

void TestPagedIpc(TIpcDir aIpcDir,
				  TIpcObjectPaged aClientPaged,
				  TIpcObjectPaged aServerPaged,
				  User::TRealtimeState aClientState,
				  User::TRealtimeState aServerState,
				  TThreadOutcome aClientOutcome,
				  TThreadOutcome aServerOutcome,
				  TPagingErrorContext aSimulatedError = EPagingErrorContextNone)
	{
	test.Printf(_L("TestPagedIpc %d %d %d %d %d %d %d\n"), aIpcDir, aClientPaged, aServerPaged,
				aClientState, aServerState, aClientOutcome, aServerOutcome);
	
	RThread serverThread;
	RThread clientThread;
	TRequestStatus serverStatus;
	TRequestStatus clientStatus;

	TInt serverArg = aIpcDir | (aServerPaged << 8) | (aServerState << 16) | (aClientState << 24);
	test_KErrNone(serverThread.Create(KNullDesC, &IpcTestServerFunc, 0x1000, NULL, (TAny*)serverArg));
	TName name;
	name = serverThread.Name();
	test.Printf(_L("  server: %S\n"), &name);
	serverThread.Rendezvous(serverStatus);
	serverThread.Resume();
	User::WaitForRequest(serverStatus);
	test_KErrNone(serverStatus.Int());
	serverThread.Logon(serverStatus);
	
	TInt clientArg = aIpcDir | (aClientPaged << 8) | (aClientState << 16);
	test_KErrNone(clientThread.Create(KNullDesC, &IpcTestClientFunc, 0x1000, NULL, (TAny*)clientArg));
	name = clientThread.Name();
	test.Printf(_L("  client: %S\n"), &name);
	clientThread.Logon(clientStatus);

	// set up simulated failure if specifed
	if (aSimulatedError != EPagingErrorContextNone)
		test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalDebugSetFail, (TAny*)aSimulatedError, 0));
	
	clientThread.Resume();

	User::WaitForRequest(serverStatus);
	test.Printf(_L("  server exit type is %d %d\n"), serverThread.ExitType(), serverThread.ExitReason());
	TestServer.Close();  // because handle is process-relative, it's not closed if the server dies

	User::WaitForRequest(clientStatus);
	test.Printf(_L("  client exit type is %d %d\n"), clientThread.ExitType(), clientThread.ExitReason());
	
	// cancel any simulated failure
	if (aSimulatedError != EPagingErrorContextNone)
		test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalDebugSetFail, (TAny*)EPagingErrorContextNone, 0));

	TestThreadOutcome(serverThread, aServerOutcome);
	TestThreadOutcome(clientThread, aClientOutcome);
	
	CLOSE_AND_WAIT(serverThread);
	CLOSE_AND_WAIT(clientThread);
	}

TInt TestThreadFunction(TAny* aType)
	{
	// Ensure that pageable memory is paged out
	TInt r=DPTest::FlushCache();
	if(r!=KErrNone)
		return r;

	// Access pageable data whilst thread is in specified realttime state.
	User::SetRealtimeState((User::TRealtimeState)(TInt)aType);
	READ(SmallBuffer);
	return KErrNone;
	}

TInt RunTestThread(User::TRealtimeState aType, TThreadOutcome aOutcome)
	{
	RThread thread;
	TInt r=thread.Create(KNullDesC, &TestThreadFunction, 0x1000, NULL, (TAny*)aType);
	if(r!=KErrNone)
		return r;
	TRequestStatus s;
	thread.Logon(s);
	if(s.Int()!=KRequestPending)
		return s.Int();
	thread.Resume();
	User::WaitForRequest(s);
	TestThreadOutcome(thread, aOutcome);
	CLOSE_AND_WAIT(thread);
	return KErrNone;
	}

void TestRealtimeState()
	{
	// make sure live list is big enough
	test(KErrNone==DPTest::SetCacheSize(256*PageSize,256*PageSize));

	test.Start(_L("Enable KREALTIME tracing"));
	Ldd.SetRealtimeTrace(ETrue);

	test.Next(_L("Test ERealtimeStateOff"));
	RunTestThread(User::ERealtimeStateOff, ENoError);

	test.Next(_L("Test ERealtimeStateOn"));
	RunTestThread(User::ERealtimeStateOn, ERealtimePanic);

	test.Next(_L("Test ERealtimeStateWarn"));
	RunTestThread(User::ERealtimeStateWarn, ENoError);

	test.Next(_L("Test combinations of IPC with realtime state"));

	//           ipc dir:      client paged:     server paged:     client state:             server state:             client outcome:    server outcome:
	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);

	if (DataPagingSupported)
		{
	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOff,  ENoError,          ENoError);
		}

	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);

	if (DataPagingSupported)
		{
	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateWarn, ENoError,          ENoError);
		}

	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerRead,  EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ERealtimePanic,    EBadDescriptor);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateOn,   EServerTerminated, ERealtimePanic);
	TestPagedIpc(EServerWrite, ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateOn,   EServerTerminated, ERealtimePanic);

	if (DataPagingSupported)
		{
	TestPagedIpc(EServerRead,  ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesHeaderPaged,  User::ERealtimeStateOff,  User::ERealtimeStateOn,   EServerTerminated, ERealtimePanic);
	TestPagedIpc(EServerRead,  ENothingPaged,    EDesContentPaged, User::ERealtimeStateOff,  User::ERealtimeStateOn,   EServerTerminated, ERealtimePanic);
	TestPagedIpc(EServerWrite, ENothingPaged,    ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesHeaderPaged,  ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ENoError,          ENoError);
	TestPagedIpc(EServerWrite, EDesContentPaged, ENothingPaged,    User::ERealtimeStateOff,  User::ERealtimeStateOn,   ERealtimePanic,    EBadDescriptor);
		}
	
	test.End();

	// retore size of live list
	test(KErrNone==DPTest::SetCacheSize(0,0));
	}

enum TPageFaultType
	{
	EPageFaultRomRead,
	EPageFaultCodeRead,
	EPageFaultDataRead,
	EPageFaultDataWrite,
	};


TInt TestPagingErrorThreadFunction(TAny* aArg)
	{
	TUint8* ptr = (TUint8*)((TUint)aArg & ~1);
	TBool write = ((TUint)aArg & 1) != 0;

	if (write)
		{
		WRITE(ptr);
		return DPTest::FlushCache();
		}
	else
		{
		READ(ptr);
		return KErrNone;
		}
	}

void TestPagingError(TPageFaultType aPageFaultType,
					 TPagingErrorContext aSimulatedError,
					 TExitType aExpectedExitType,
					 const TDesC& aExpectedExitCategory,
					 TInt aExpectedExitReason)
	{
	test.Printf(_L("TestPagingError %d %d %d \"%S\" %d\n"), aPageFaultType, aSimulatedError,
				aExpectedExitType, &aExpectedExitCategory, aExpectedExitReason);
	
	TUint8* ptr;
	TBool write;

	switch(aPageFaultType)
		{
		case EPageFaultRomRead:   ptr = RomPagedBuffer;  write = EFalse; break;
		case EPageFaultCodeRead:  ptr = CodePagedBuffer; write = EFalse; break;
		case EPageFaultDataRead:  ptr = DataPagedBuffer; write = EFalse; break;
		case EPageFaultDataWrite: ptr = DataPagedBuffer; write = ETrue;  break;
		default: test(EFalse); return;
		}

	if (ptr == NULL) return;  // specified type of paging is not enabled

	if (write)
		READ(ptr);  // ensure data to be written is paged in
	else
		test_KErrNone(DPTest::FlushCache());  // ensure data to be read is paged out

	// set up simulated failure
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalDebugSetFail, (TAny*)aSimulatedError, 0));
	
	RThread thread;
	TAny* arg = (TAny*)((TUint)ptr | (write ? 1 : 0));
	test_KErrNone(thread.Create(KNullDesC, &TestPagingErrorThreadFunction, 0x1000, NULL, arg));
	TRequestStatus s;
	thread.Logon(s);
	test_Equal(KRequestPending, s.Int());
	thread.Resume();
	User::WaitForRequest(s);

	// cancel any simulated failure
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalDebugSetFail, (TAny*)EPagingErrorContextNone, 0));

	TExitCategoryName exitCategory = thread.ExitCategory();
	test.Printf(_L("  thread exit type is %d \"%S\" %d\n"),
				thread.ExitType(), &exitCategory, thread.ExitReason());
	
	test_Equal(aExpectedExitType, thread.ExitType());
	if (aExpectedExitType == EExitPanic)
		test_Equal(0, aExpectedExitCategory.Compare(exitCategory));	
	test_Equal(aExpectedExitReason, thread.ExitReason());	
	
	CLOSE_AND_WAIT(thread);
	}

void CreateDataPagedChunk()
	{
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(KMinBufferSize, KMinBufferSize);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetOwner(EOwnerProcess);
	createInfo.SetGlobal(KChunkName);
	test_KErrNone(DataPagedChunk.Create(createInfo));
	test(DataPagedChunk.IsPaged()); // this is only ever called if data paging is supported
	DataPagedBuffer = (TUint8*)DataPagedChunk.Base();
	}

void TestPagingErrors()
	{
	// test what happens when the paging system encounters errors such as failure when accessing
	// media or decompressing paged data

	//              page fault type:     simulated error:                   exit type:  exit category:          exit reason:
	TestPagingError(EPageFaultRomRead,   EPagingErrorContextNone,           EExitKill,  KNullDesC,              KErrNone);
	TestPagingError(EPageFaultRomRead,   EPagingErrorContextRomRead,        EExitPanic, _L("PAGED-ROM-READ"),   KErrAbort);
	TestPagingError(EPageFaultRomRead,   EPagingErrorContextRomDecompress,  EExitPanic, _L("PAGED-ROM-COMP"),   KErrAbort);
	
	TestPagingError(EPageFaultCodeRead,  EPagingErrorContextNone,           EExitKill,  KNullDesC,              KErrNone);
	TestPagingError(EPageFaultCodeRead,  EPagingErrorContextCodeRead,       EExitPanic, _L("PAGED-CODE-READ"),  KErrAbort);
	TestPagingError(EPageFaultCodeRead,  EPagingErrorContextCodeDecompress, EExitPanic, _L("PAGED-CODE-COMP"),  KErrAbort);

	if (DataPagedBuffer)
		{
		// Note WDP write faults are only reported on the next read
		WRITE(DataPagedBuffer);  // ensure page is not blank and will be read from swap
		
		//              page fault type:     simulated error:               exit type:  exit category:          exit reason:
		TestPagingError(EPageFaultDataRead,  EPagingErrorContextNone,       EExitKill,  KNullDesC,              KErrNone);
		TestPagingError(EPageFaultDataRead,  EPagingErrorContextDataRead,   EExitPanic, _L("PAGED-DATA-READ"),  KErrAbort);
		TestPagingError(EPageFaultDataWrite, EPagingErrorContextDataWrite,  EExitKill,  KNullDesC,              KErrNone);
		TestPagingError(EPageFaultDataRead,  EPagingErrorContextNone,       EExitPanic, _L("PAGED-DATA-WRITE"), KErrAbort);

		// this will now always panic when we try to access the first page so destroy and re-create it
		DataPagedChunk.Close();
		CreateDataPagedChunk();
		}

	// test attribution of errors during IPC
	TPagingErrorContext error;
	if (RomPagedBuffer)
		error = EPagingErrorContextRomRead;
	else if (CodePagedBuffer)
		error = EPagingErrorContextCodeRead;
	else
		error = EPagingErrorContextDataRead;
	//           ipc dir:     client paged:     server paged:  client state:            server state:            client outcome:  server outcome:
	TestPagedIpc(EServerRead, EDesContentPaged, ENothingPaged, User::ERealtimeStateOff, User::ERealtimeStateOff, EAbortPanic,     EBadDescriptor,  error);
	}

void TestLock()
	{
	// make sure live list is big enough
	test(KErrNone==DPTest::SetCacheSize(128 * PageSize, 256 * PageSize));

	TInt r = Ldd.LockTest(SmallBuffer,SmallBufferSize);
	if(r==KErrNone)
		{
		test.Next(_L("Lock Test again"));
		r = Ldd.LockTest(SmallBuffer,SmallBufferSize);
		}
	if(r)
		{
		test.Printf(_L("failed at D_DEMANPAGING.CPP line %d\n"),r);
		test(0);
		}
		
	// restore live list to default size...
	test(KErrNone==DPTest::SetCacheSize(0,0));
	}

const TInt KSmallPropertySize = 512;
const TInt KLargePropertySize = 16384;
	
struct SSetPropertyInfo
	{
	TUid iCategory;
	TInt iKey;
	TUint8* iData;
	TInt iLength;
	};

TInt SetPropertyThreadFunction(TAny* aArg)
	{
	SSetPropertyInfo* info = (SSetPropertyInfo*)aArg;
	TInt r;
	r = DPTest::FlushCache();
	if (r != KErrNone)
		return r;
	TPtrC8 data(info->iData, info->iLength);
	r = RProperty::Set(info->iCategory, info->iKey, data);
	if (r != KErrNone)
		return r;
	RBuf8 buffer;
	r = buffer.Create(KLargePropertySize);
	if (r != KErrNone)
		return r;
	r = RProperty::Get(info->iCategory, info->iKey, buffer);
	if (r == KErrNone && buffer != data)
		r = KErrGeneral;
	buffer.Close();
	return r;
	}

void TestPublishAndSubscribe()
	{
	RProcess thisProcess;
	TUid category = thisProcess.SecureId();

	TSecurityPolicy alwaysPass(TSecurityPolicy::EAlwaysPass);
	test(RProperty::Define(category, 0, RProperty::EByteArray, alwaysPass, alwaysPass) == KErrNone);
	test(RProperty::Define(category, 1, RProperty::ELargeByteArray, alwaysPass, alwaysPass) == KErrNone);
	
	TPtrC8 smallData(SmallBuffer, KSmallPropertySize);
	TPtrC8 largeData(SmallBuffer, KLargePropertySize);
	
	RBuf8 buffer;
	test(buffer.Create(KLargePropertySize) == KErrNone);

	// Set small property from paged data, method 1
	test(DPTest::FlushCache() == KErrNone);
	test(RProperty::Set(category, 0, smallData) == KErrNone);
	test(RProperty::Get(category, 0, buffer) == KErrNone);
	test(buffer == smallData);
	
	// Set small property from paged data, method 2
	RProperty smallProp;
	test(smallProp.Attach(category, 0) == KErrNone);
	test(DPTest::FlushCache() == KErrNone);
	test(smallProp.Set(smallData) == KErrNone);
	test(smallProp.Get(buffer) == KErrNone);
	test(buffer == smallData);

	// Set large property from paged data, method 1
	test(DPTest::FlushCache() == KErrNone);
	test(RProperty::Set(category, 1, largeData) == KErrNone);
	test(RProperty::Get(category, 1, buffer) == KErrNone);
	test(buffer == largeData);
	
	// Set large property from paged data, method 2
	RProperty largeProp;
	test(largeProp.Attach(category, 1) == KErrNone);
	test(DPTest::FlushCache() == KErrNone);
	test(largeProp.Set(largeData) == KErrNone);
	test(largeProp.Get(buffer) == KErrNone);
	test(buffer == largeData);

	// Set small property from unmapped address
	RThread thread;
#if !defined( __VC32__)
	SSetPropertyInfo info = { category, 0, 0, KSmallPropertySize };
#else
	SSetPropertyInfo info = { category.iUid, 0, 0, KSmallPropertySize };
#endif
	test(thread.Create(_L("SetPropertyThread"), SetPropertyThreadFunction, 4096, NULL, &info) == KErrNone);
	thread.Resume();
	TRequestStatus status;
	thread.Logon(status);
	User::WaitForRequest(status);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ECausedException);
	thread.Close();

	// Set large property from unmapped address
	info.iKey = 1;
	info.iLength = KLargePropertySize;
	test(thread.Create(_L("SetPropertyThread"), SetPropertyThreadFunction, 4096, NULL, &info) == KErrNone);
	thread.Resume();
	thread.Logon(status);
	User::WaitForRequest(status);
	test(thread.ExitType()==EExitPanic);
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ECausedException);
	thread.Close();
	
	test(RProperty::Delete(category, 0) == KErrNone);
	test(RProperty::Delete(category, 1) == KErrNone);
	}

void TestWriteToPagedArea()
	{
	RMemoryTestLdd memoryTest;
	test(KErrNone==memoryTest.Open());

	TModuleMemoryInfo exeInfo;
	test(KErrNone==RProcess().GetMemoryInfo(exeInfo));
	test.Printf(_L("test program code is %x+%x"),exeInfo.iCodeBase,exeInfo.iCodeSize);

	TUint8* ptr = const_cast<TUint8*>(LargeBuffer);
	TUint8* end = ptr + LargeBufferSize;
	while(ptr<end)
		{
		if(ptr>=(TUint8*)_ALIGN_DOWN(exeInfo.iCodeBase,PageSize) && ptr<(TUint8*)_ALIGN_UP(exeInfo.iCodeBase+exeInfo.iCodeSize,PageSize))
			{
			// avoid testing the ROM which contains this test program
			ptr += PageSize;
			continue;
			}
	
		DPTest::FlushCache();

		TInt stateBits = UserSvr::HalFunction(EHalGroupVM, EVMPageState, ptr, 0);
		test(stateBits>=0);
		// write to paged out memory should cause exception...
		test(KErrBadDescriptor==memoryTest.WriteMemory(ptr,0));
		// page state should be unchanged...
		TInt newStateBits = UserSvr::HalFunction(EHalGroupVM, EVMPageState, ptr, 0);
		if(stateBits!=newStateBits)
			{
			test.Printf(_L("ptr=%x stateBits=%x newStateBits=%x"),ptr,stateBits,newStateBits);
			test(0);
			}
		// page-in in memory...
		TUint32 value = *(TUint32*)ptr;
		// write to paged out memory should still cause exception...
		test(KErrBadDescriptor==memoryTest.WriteMemory(ptr,~value));
		// memory should be unchanged...
		test(value==*(TUint32*)ptr);
		ptr += PageSize;
		}

	memoryTest.Close();
	}

void TestContiguousRamAlloc()
	{
	test.Start(_L("Start..."));

	const TInt KCacheSize = 1024*1024;

	DPTest::SetCacheSize(0, KCacheSize); // make sure paging cache is a reasonable size

	TInt testData[][2] = /* array of page size (in units of 'half pages')  and align values */
		{
			{64,5},
			{64,0},
			{32,4},
			{32,3},
			{32,2},
			{32,1},
			{32,0},
			{4,2},
			{4,1},
			{4,0},
			{2,2},
			{2,1},
			{2,0},
			{1,0},
			{0,0}
		};
	TInt pageShift = 1;
	while((1<<pageShift)<PageSize)
		++pageShift;

	TInt* params = (TInt*)&testData;
	while(*params)
		{
		TInt size =  *params++<<(pageShift-1);	//Size is units of half pages, so one less shift to get required memory size
		TInt align = *params++;
		if(align)
			align += pageShift - 1;
		TBuf<256> title;
		title.AppendFormat(_L("Contiguous RAM test: alloc size = %dK align = %d"),size>>10, align);
		test.Next(title);
		FragmentPagingCache(KCacheSize);
		TInt r = Ldd.DoConsumeContiguousRamTest(align, size);
		if(r)
			{
			test.Printf(_L("failed at D_DEMANPAGING.CPP line %d\n"),r);
			test(0);
			}
		}

	DPTest::SetCacheSize(0,0); // back to defaults
	test.End();
	}

void TestReadHoldingMutex()
	{
	TUint8 localBuf[16];
	TUint8* localPtr = localBuf;
	if(DPTest::Attributes() & DPTest::EDataPaging) // if data paging supported...
		localPtr = 0; // use zero to make driver use kernel memory as data destination
	test(Ldd.ReadHoldingMutexTest(localPtr) == KErrNone);
	}

#if 0 // rom dump code...
#include <f32file.h>
	RFs fs;
	RFile file;
	test(KErrNone==fs.Connect());
	test(KErrNone==file.Replace(fs, _L("d:\\ROMDUMP"),EFileWrite));
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TPtrC8 rom((TUint8*)romHeader,romHeader->iRomSize);
	test(KErrNone==file.Write(rom));
	file.Close();
	fs.Close();
	return 0;
#endif


class RMySession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus& aStat)
		{
		TInt r=CreateSession(aSrv,TVersion(),-1,EIpcSession_Sharable,NULL,&aStat);
		if(!r) ShareAuto(); return r;
		}
	TInt Send(TInt function,const TIpcArgs& args)
		{return SendReceive(function,args);}
	};


TUint8* TestBuffer = 0;
RMySession MySession;

LOCAL_C TInt TestServerThread(TAny* aSize)
	{
	TInt r = TestServer.CreateGlobal(KNullDesC, EIpcSession_GlobalSharable);
	if(r==KErrNone)
		{
		TestBuffer = (TUint8*)User::Alloc(KMaxIPCSize);
		if(!TestBuffer)
			r = KErrNoMemory;
		}
	TPtr8 buffer(TestBuffer,KMaxIPCSize);
	RThread::Rendezvous(r);
	if (r != KErrNone)
		return r;

	RMessage2 m;
	TestServer.Receive(m);
	m.Complete(KErrNone);	// connect message

	TBool running = ETrue;
	while (running)
		{
		TestServer.Receive(m);
		RDebug::Printf("Server received: %d", m.Function());

		TInt r = KErrNone;
		switch(m.Function())
			{
			case 0:
				// Kill server
				running = EFalse;
				break;

			case 2:
				buffer.Set(SharedBuffer,KMaxIPCSize,KMaxIPCSize);
				// fall through...
			case 1:
				// Perform CRC of data passed
				{
				DPTest::FlushCache();
				r=m.Read(0,buffer);
				if (r!=KErrNone)
					break;
				TUint32 crc=0;
				Mem::Crc32(crc,buffer.Ptr(),buffer.Size());
				r = crc;
				}
				break;

			case 4:
				buffer.Set(SharedBuffer,KMaxIPCSize,KMaxIPCSize);
				// fall through...
			case 3:
				// Write data to client descriptor
				{
				DPTest::FlushCache();
				RDebug::Printf("Server writing %08x+%x", m.Int1(), m.Int2());
				TPtrC8 ptr((TUint8*)m.Int1(),m.Int2());
				r=m.Write(0,ptr);
				}
				break;
			
			default:
				// Just complete anything else
				break;
			}
		m.Complete(r);
		}

	RDebug::Printf("Server exiting");
	User::Free(TestBuffer);
	TestBuffer = NULL;
	TestServer.Close();
	return KErrNone;
	}

void TestIPC()
	{
	__KHEAP_MARK;
		
	const TUint8* start = LargeBuffer + 0x3df; // make range not page aligned
	const TUint8* end = start + Min(LargeBufferSize, KMaxIPCSize * 10) - 0x130; // make range not page aligned
	const TUint8* pos;
	
	test.Start(_L("Create server"));
	RThread t;
	TInt r=t.Create(KNullDesC,TestServerThread,0x1000,KMaxIPCSize+0x1000,KMaxIPCSize+0x1000,(void*)0);
	test(r==KErrNone);
	t.SetPriority(EPriorityMore);
	TRequestStatus s;
	t.Rendezvous(s);
	t.Resume();
	User::WaitForRequest(s);
	test(TestServer.Handle() != KNullHandle);

	test(MySession.Connect(TestServer,s) == KErrNone);
	User::WaitForRequest(s);	// connected

	TInt bufferType; // 0=server uses heap, 1=server uses SharedBuffer
	for(bufferType=0; bufferType<=1; ++bufferType)
		{
		test.Next(_L("IPC read from ROM"));
		pos = start;
		while(pos<end)
			{
			TInt size = end-pos;
			if(size>KMaxIPCSize)
				size = KMaxIPCSize;
			RDebug::Printf("read %x+%x",pos,size);
			TPtrC8 ptr(pos,size);
			TInt r = MySession.Send(1+bufferType,TIpcArgs(&ptr));
			DPTest::FlushCache();
			TUint32 crc=0;
			Mem::Crc32(crc,pos,size);
			RDebug::Printf("crc %08x %08x",r,crc);
			if((TUint32)r!=crc)
				{
				RDebug::Printf("FAIL");
				DPTest::FlushCache();
				TInt count = 0;
				for(TInt i=0; i<size; i+=4)
					{
					TUint32 a = pos[i];
					TUint32 b = TestBuffer[i];
					if(a!=b)
						RDebug::Printf("%08x %02x!=%02x",pos+i,a,b);
					if (++count > 100)
						break;
					}
				}
			test((TUint32)r==crc);
			pos+=size;
			}

		test.Next(_L("IPC write from ROM"));
		pos = start;
		while(pos<end)
			{
			TInt size = end-pos;
			if(size>KMaxIPCSize)
				size = KMaxIPCSize;
			RDebug::Printf("write %x+%x",pos,size);
			memclr(TestBuffer, KMaxIPCSize);
			TPtr8 ptr(TestBuffer,KMaxIPCSize);	// reuse the server's buffer
			TInt r = MySession.Send(3+bufferType,TIpcArgs(&ptr,pos,size));
			test_KErrNone(r);
			DPTest::FlushCache();
			TUint32 crc=0;
			Mem::Crc32(crc,pos,size);
			TUint32 crc2=0;
			Mem::Crc32(crc2,TestBuffer,size);
			RDebug::Printf("crc %08x %08x",crc,crc2);
			if((TUint32)crc!=crc2)
				{
				RDebug::Printf("FAIL");
				DPTest::FlushCache();
				TInt count = 0;
				for(TInt i=0; i<size; i+=4)
					{
					TUint32 a = pos[i];
					TUint32 b = TestBuffer[i];
					if(a!=b)
						RDebug::Printf("%08x %02x!=%02x",pos+i,a,b);
					if (++count > 100)
						break;
					}
				}
			test((TUint32)crc==crc2);
			pos+=size;
			}
		}

	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		test.Next(_L("Test passing descriptor headers in paged-out memory"));
		__KHEAP_MARK;

		DPTest::FlushCache();
		TInt r = MySession.Send(5,TIpcArgs(PagedHeaderDes));
		test_Equal(KErrNone, r);
		
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
		__KHEAP_MARKEND;
		}
	
	test.Next(_L("Stop server"));
	MySession.Send(0,TIpcArgs(0));
	MySession.Close();
	t.Logon(s);
	User::WaitForRequest(s);
	test_Equal(EExitKill, t.ExitType());
	test_Equal(KErrNone, t.ExitReason());
	CLOSE_AND_WAIT(t);	
	test.End();
	
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	__KHEAP_MARKEND;
	}


TInt E32Main()
	{
	test.Title();
	
	test_KErrNone(UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&PageSize,0));

	test.Start(_L("Initialisation"));
	
	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		test.Printf(_L("Rom paging supported\n"));
		TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
		test(romHeader->iPageableRomStart);
		// todo: for some reason the first part of page of paged rom doesn't seem to get paged out
		// when we flush the paging cache, hence RomPagedBuffer starts some way into this
		RomPagedBuffer = (TUint8*)romHeader + romHeader->iPageableRomStart + 64 * PageSize; 
		RomPagedBufferSize = romHeader->iPageableRomSize - 64 * PageSize;
		test(RomPagedBufferSize > 0);
		}
	
	if (DPTest::Attributes() & DPTest::ECodePaging)
		{
		test.Printf(_L("Code paging supported\n"));
		test_KErrNone(PagedLibrary.Load(KTCodePagingDll4));		
		TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)PagedLibrary.Lookup(KGetAddressOfDataFunctionOrdinal);
		CodePagedBuffer = (TUint8*)func(CodePagedBufferSize);
		test_NotNull(CodePagedBuffer);
		test(CodePagedBufferSize > KMinBufferSize);
		}
	
	if (DPTest::Attributes() & DPTest::EDataPaging)
		{
		test.Printf(_L("Data paging supported\n"));
		DataPagingSupported = ETrue;
		CreateDataPagedChunk();
		}

	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		// Use paged part of rom for testing
		LargeBuffer = RomPagedBuffer;
		LargeBufferSize = RomPagedBufferSize;
		}
	else if (DPTest::Attributes() & DPTest::ECodePaging)
		{
		// Use code paged DLL for testing
		LargeBuffer = CodePagedBuffer;
		LargeBufferSize = CodePagedBufferSize;
		}
	else if (DPTest::Attributes() & DPTest::EDataPaging)
		{
		// Use data paged chunk for testing
		LargeBuffer = DataPagedBuffer;
		LargeBufferSize = KMinBufferSize;
		}
	else
		{
		test.Printf(_L("Demand Paging not supported\n"));
		test.End();
		return 0;
		}
	
	// Find a paged zero word to set PagedHeaderDes to
	TUint* ptr = (TUint*)LargeBuffer;
	TUint* end = (TUint*)(LargeBuffer + LargeBufferSize);
	while (*ptr && ptr < end)
		++ptr;
	test(*ptr == 0);
	test.Printf(_L("Found zero word at %08x\n"), ptr);
	PagedHeaderDes = (TDesC8*)ptr;

	test.Next(_L("Test HAL interface"));
	TestHAL();
	
	test(LargeBufferSize >= KMinBufferSize);
	SmallBuffer = LargeBuffer;
	SmallBufferSize = KMinBufferSize;

	test.Next(_L("Loading test drivers"));
	TInt r = User::LoadLogicalDevice(KDemandPagingTestLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test(Ldd.Open()==KErrNone);

	test_KErrNone(Ldd.CreatePlatHwChunk(SharedBufferSize, SharedBufferAddr));
	SharedBuffer = (TUint8*)SharedBufferAddr;

	RDebug::Printf("SmallBuffer=%x, LargeBuffer=%x, SharedBuffer=%x\n",
		SmallBuffer, LargeBuffer, SharedBuffer);

	test.Next(_L("Gobble RAM"));
	r = User::LoadLogicalDevice(KGobblerLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	RGobbler gobbler;
	r = gobbler.Open();
	test(r==KErrNone);
	TUint32 taken = gobbler.GobbleRAM(64*1024*1024); // leave 64MB of free RAM
	test.Printf(_L("Gobbled: %dK\n"), taken/1024);
	test.Printf(_L("Free RAM 0x%08X bytes\n"),FreeRam());

	test.Next(_L("Test contiguous RAM allocation reclaims paged memory"));
	TestContiguousRamAlloc();

	test.Next(_L("Test thread realtime state"));
	TestRealtimeState();

	test.Next(_L("Lock Test"));
	TestLock();

	test.Next(_L("Test writing to paged area"));
	TestWriteToPagedArea();

	test.Next(_L("Test IPC read from paged memory"));
	TestIPC();

	test.Next(_L("Test no kernel faults when copying data from unpaged rom with mutex held"));
	TestReadHoldingMutex();
	
#ifdef _DEBUG
	// test hook in kernel not present in release mode
	if ((MemModelAttributes() & EMemModelTypeMask) == EMemModelTypeFlexible)
		{
		test.Next(_L("Test unrecoverable errors while paging"));
		TestPagingErrors();
		}
#endif

	test.Next(_L("Close test driver"));
	Ldd.DestroyPlatHwChunk();
	Ldd.Close();

	test.Next(_L("Test setting publish and subscribe properties from paged area"));
	TestPublishAndSubscribe();

#ifndef _DEBUG
	// no point benchmarking in debug mode
	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		test.Next(_L("Rom Paging Benchmark"));
		RomPagingBenchmark();
		}
#endif

	PagedLibrary.Close();
	gobbler.Close();
	test.End();

	return 0;
	}
