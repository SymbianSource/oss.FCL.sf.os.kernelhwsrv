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
// e32test\mmu\t_mmustress.cpp
// Stress test for memory management services performed by the kernel's memory model.
// 
//

/**
 @file
*/

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "u32std.h"
#include <u32hal.h>
#include <e32svr.h>
#include <dptest.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_memorytest.h"
#include "..\defrag\d_pagemove.h"

TBool TRACE = 0;

LOCAL_D RTest test(_L("T_MMUSTRESS"));

TUint32 MemModelAttributes;
TUint32 MemModel;
TInt PageSize;
TInt PageMask;

#if !defined(__WINS__) && !defined(__X86__)
const TPtrC KMoveLddFileName=_L("D_PAGEMOVE.LDD");
RPageMove MoveLdd;
#endif

RMemoryTestLdd Ldd;

const TUint KNumTestChunks = 6;
RChunk Chunks[KNumTestChunks];
TInt Committed[KNumTestChunks] = {0}; // for each chunk, is the 'owned' region uncommited(0), commited(1) or mixed(-1)
class TNicePtr8 : public TPtr8 { public: TNicePtr8() : TPtr8(0,0) {} } ChunkPtr[KNumTestChunks];

const TUint KNumSlaveProcesses = 4;
RProcess Slaves[KNumSlaveProcesses];
TRequestStatus SlaveLogons[KNumSlaveProcesses];
TRequestStatus SlaveRendezvous[KNumSlaveProcesses];

TInt SlaveNumber = -1; // master process is slave -1

const TInt KLocalIpcBufferSize = 0x10000;
TUint8* LocalIpcBuffer = 0;

RSemaphore StartSemaphore;

//
// Random number generation
//

TUint32 RandomSeed;

TUint32 Random()
	{
	RandomSeed = RandomSeed*69069+1;
	return RandomSeed;
	}

TUint32 Random(TUint32 aRange)
	{
	return (TUint32)((TUint64(Random())*TUint64(aRange))>>32);
	}

void RandomInit(TUint32 aSeed)
	{
	RandomSeed = aSeed+(aSeed<<8)+(aSeed<<16)+(aSeed<<24);
	Random();
	Random();
	}



//
// Chunk utils
//

TBuf<KMaxKernelName> ChunkName(TInt aChunkNumber)
	{
	TBuf<KMaxKernelName> name;
	name.Format(_L("T_MMUSTRESS-Chunk%d"),aChunkNumber);
	return name;
	}

#ifdef __WINS__
TInt KChunkShift = 16;
#elif defined(__X86__)
TInt KChunkShift = 22;
#else
TInt KChunkShift = 20;
#endif

TInt ChunkSize(TInt aChunkNumber)
	{
	// biggest chunk (number 0) is big enough for each slave to own a region which is
	// 2 page tables ('chunks') in size...
	return (2*KNumSlaveProcesses)<<(KChunkShift-aChunkNumber);
	}

// check smallest chunk is less than 'chunk' size...
__ASSERT_COMPILE((2*KNumSlaveProcesses>>(KNumTestChunks-1))==0);


/* Memory region 'owned' by this slave process */
void ChunkOwnedRegion(TInt aChunkNumber,TInt& aOffset,TInt& aSize)
	{
	TInt size = ChunkSize(aChunkNumber)/KNumSlaveProcesses;
	aSize = size;
	aOffset = SlaveNumber*size;
	test_Equal(0,size&PageMask);
	}

void ChunkMarkRegion(TInt aChunkNumber,TInt aOffset,TInt aSize)
	{
	TInt pageSize = PageSize;
	TUint32 mark = aOffset|aChunkNumber|(SlaveNumber<<4);
	TUint8* ptr = Chunks[aChunkNumber].Base()+aOffset;
	TUint8* ptrEnd = ptr+aSize;
	while(ptr<ptrEnd)
		{
		((TUint32*)ptr)[0] = mark;
		((TUint32*)ptr)[1] = ~mark;
		mark += pageSize;
		ptr += pageSize;
		}
	}

void ChunkCheckRegion(TInt aChunkNumber,TInt aOffset,TInt aSize)
	{
	TInt pageSize = PageSize;
	TUint32 mark = aOffset|aChunkNumber|(SlaveNumber<<4);
	TUint8* ptr = Chunks[aChunkNumber].Base()+aOffset;
	TUint8* ptrEnd = ptr+aSize;
	while(ptr<ptrEnd)
		{
		test_Equal(mark,((TUint32*)ptr)[0]);
		test_Equal(~mark,((TUint32*)ptr)[1]);
		mark += pageSize;
		ptr += pageSize;
		}
	}

TInt ChunkOpen(TInt aChunkNumber)
	{
	RChunk& chunk = Chunks[aChunkNumber];
	if(chunk.Handle()!=0)
		return KErrNone;

	if(TRACE) RDebug::Printf("%d %d Open",SlaveNumber,aChunkNumber);
	TInt r = chunk.OpenGlobal(ChunkName(aChunkNumber),false);
	if(r!=KErrNoMemory)
		test_KErrNone(r);
	return r;
	}

//
// Server utils
//

TBuf<KMaxKernelName> ServerName(TInt aSlaveNumber)
	{
	TBuf<KMaxKernelName> name;
	name.Format(_L("T_MMUSTRESS-Server%d"),aSlaveNumber);
	return name;
	}

RServer2 Server;
RMessage2 ServerMessage;
TRequestStatus ServerStatus;

class RTestSession : public RSessionBase
	{
public:
	TInt Connect(TInt aServerNumber)
		{
		return CreateSession(ServerName(aServerNumber),TVersion(),1,EIpcSession_Unsharable,0,&iStatus);
		}
	TInt Send(TInt aChunkNumber)
		{
		return RSessionBase::Send(0,TIpcArgs(SlaveNumber,aChunkNumber,&ChunkPtr[aChunkNumber]));
		}
	TRequestStatus iStatus;
	};
RTestSession Sessions[KNumSlaveProcesses];


//
//
//

void SlaveInit()
	{
	RDebug::Printf("Slave %d initialising",SlaveNumber);

	TBuf<KMaxKernelName> name;
	name.Format(_L("T_MMUSTRESS-Slave%d"),SlaveNumber);
	User::RenameThread(name);

	test_KErrNone(StartSemaphore.Open(2));
	TInt r;
#if !defined(__WINS__) && !defined(__X86__)
	// Move ldd may not be in the ROM so needs to be loaded.
	r=User::LoadLogicalDevice(KMoveLddFileName);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	test_KErrNone(MoveLdd.Open());
#endif

	test_KErrNone(Ldd.Open());
	test_KErrNone(Ldd.CreateVirtualPinObject());

	LocalIpcBuffer = (TUint8*)User::Alloc(KLocalIpcBufferSize);
	test(LocalIpcBuffer!=0);

	test_KErrNone(Server.CreateGlobal(ServerName(SlaveNumber)));

	TUint i;

	// create sessions with other slaves...
	for(i=0; i<KNumSlaveProcesses; i++)
		{
		for(;;)
			{
			r = Sessions[i].Connect(i);
//			RDebug::Printf("%d Session %d = %d,%d",SlaveNumber,i,r,Sessions[i].iStatus.Int());
			if(r==KErrNotFound)
				{
				// give other slaves time to create their servers...
				User::After(10000);
				continue;
				}
			test_KErrNone(r);
			break;
			}
		}

	// process session connect messages...
	for(i=0; i<KNumSlaveProcesses; i++)
		{
		RMessage2 m;
//		RDebug::Printf("%d Server waiting for connect message",SlaveNumber);
		Server.Receive(m);
		test_Equal(RMessage2::EConnect,m.Function())
		m.Complete(KErrNone);
		}

	// wait for our session connections...
	for(i=0; i<KNumSlaveProcesses; i++)
		{
//		RDebug::Printf("%d Session wait %d",SlaveNumber,i);
		User::WaitForRequest(Sessions[i].iStatus);
		}

	// prime server for receiving mesages...
	Server.Receive(ServerMessage,ServerStatus);

	// synchronise with other processes...
	RDebug::Printf("Slave %d waiting for trigger",SlaveNumber);
	RProcess::Rendezvous(KErrNone);
	StartSemaphore.Wait();
	RDebug::Printf("Slave %d started",SlaveNumber);
	}



//
// Test by random operations...
//

void DoTest()
	{
	RandomInit(SlaveNumber);
	TInt r;
	for(;;)
		{
		// select random chunk...
		TInt chunkNumber = Random(KNumTestChunks);
		RChunk& chunk = Chunks[chunkNumber];

		// get the region of this chunk which this process 'owns'...
		TInt offset;
		TInt size;
		ChunkOwnedRegion(chunkNumber,offset,size);

		// calculate a random region in the owned part...
		TInt randomOffset = offset+(Random(size)&~PageMask);
		TInt randomSize = (Random(size-(randomOffset-offset))+PageMask)&~PageMask;
		if(!randomSize)
			continue; // try again

		// pick a random slave...
		TInt randomSlave = Random(KNumSlaveProcesses);

		// open chunk if it isn't already...
		r = ChunkOpen(chunkNumber);
		if(r==KErrNoMemory)
			continue; // can't do anything with chunk if we can't open it

		// check our contents of chunk...
		if(Committed[chunkNumber]==1)
			{
			if(TRACE) RDebug::Printf("%d %d Check %08x+%08x",SlaveNumber,chunkNumber,offset,size);
			ChunkCheckRegion(chunkNumber,offset,size);
			}

		// perform random operation...
		switch(Random(12))
			{
		case 0:
		case 1:
			// close chunk...
			if(TRACE) RDebug::Printf("%d %d Close",SlaveNumber,chunkNumber);
			chunk.Close();
			break;

		case 2:
			// commit all...
			if(TRACE) RDebug::Printf("%d %d Commit all %08x+%08x",SlaveNumber,chunkNumber,offset,size);
			if(Committed[chunkNumber]!=0)
				{
				r = chunk.Decommit(offset,size);
				test_KErrNone(r);
				Committed[chunkNumber] = 0;
				}
			r = chunk.Commit(offset,size);
			if(r!=KErrNoMemory)
				{
				test_KErrNone(r);
				Committed[chunkNumber] = 1;
				ChunkMarkRegion(chunkNumber,offset,size);
				}
			break;

		case 3:
			// decommit all...
			if(TRACE) RDebug::Printf("%d %d Decommit all %08x+%08x",SlaveNumber,chunkNumber,offset,size);
			r = chunk.Decommit(offset,size);
			test_KErrNone(r);
			Committed[chunkNumber] = 0;
			break;

		case 4:
		case 5:
			// commit random...
			if(TRACE) RDebug::Printf("%d %d Commit %08x+%08x",SlaveNumber,chunkNumber,randomOffset,randomSize);
			r = chunk.Commit(randomOffset,randomSize);
			if(r!=KErrNoMemory)
				{
				if(Committed[chunkNumber]==0)
					{
					test_KErrNone(r);
					Committed[chunkNumber] = -1;
					}
				else if(Committed[chunkNumber]==1)
					{
					test_Equal(KErrAlreadyExists,r);
					}
				else
					{
					if(r!=KErrAlreadyExists)
						test_KErrNone(r);
					}
				}
			break;

		case 6:
		case 7:
			// decommit random...
			if(TRACE) RDebug::Printf("%d %d Decommit %08x+%08x",SlaveNumber,chunkNumber,randomOffset,randomSize);
			r = chunk.Decommit(randomOffset,randomSize);
			test_KErrNone(r);
			if(Committed[chunkNumber]==1)
				Committed[chunkNumber] = -1;
			break;

		case 8:
			if(TRACE) RDebug::Printf("%d %d IPC Send->%d",SlaveNumber,chunkNumber,randomSlave);
//			ChunkPtr[chunkNumber].Set(chunk.Base(),ChunkSize(chunkNumber),ChunkSize(chunkNumber));
			ChunkPtr[chunkNumber].Set(chunk.Base()+offset,size,size);
			Sessions[randomSlave].Send(chunkNumber);
			break;

		case 9:
			// process IPC messages...
			if(ServerStatus.Int()==KRequestPending)
				continue;
			User::WaitForRequest(ServerStatus);

			{
			TInt sourceSlave = ServerMessage.Int0();
			chunkNumber = ServerMessage.Int1();
			if(TRACE) RDebug::Printf("%d %d IPC Receive<-%d",SlaveNumber,chunkNumber,sourceSlave);
			test_Equal(0,ServerMessage.Function());

			// get local descriptor for owned region in chunk...
			size = ServerMessage.GetDesMaxLength(2);
			test_NotNegative(size);
			if(size>KLocalIpcBufferSize)
				size = KLocalIpcBufferSize;
			TPtr8 local(LocalIpcBuffer,size,size);

//			if(Random(2))
				{
				// IPC read from other slave...
				if(TRACE) RDebug::Printf("%d %d IPC Read<-%d",SlaveNumber,chunkNumber,sourceSlave);
				TInt panicTrace = Ldd.SetPanicTrace(EFalse);
				r = ServerMessage.Read(2,local);
				Ldd.SetPanicTrace(panicTrace);
				if(r!=KErrBadDescriptor)
					test_KErrNone(r);
				}
//			else
//				{
//				// IPC write to other slave...
//				if(TRACE) RDebug::Printf("%d %d IPC Write->%d",SlaveNumber,chunkNumber,sourceSlave);
//				r = ServerMessage.Write(2,local,offset);
//				if(r!=KErrBadDescriptor)
//					test_KErrNone(r);
//				if(Committed[chunkNumber]==1)
//					ChunkMarkRegion(chunkNumber,offset,size);
//				}
			}

			ServerMessage.Complete(KErrNone);
			Server.Receive(ServerMessage,ServerStatus);
			break;

		case 10:
		case 11:
			// pin memory...
			{
			test_KErrNone(Ldd.UnpinVirtualMemory());
			for(TInt tries=10; tries>0; --tries)
				{
				TInt chunkSize = ChunkSize(chunkNumber);
				offset = Random(chunkSize);
				TInt maxSize = chunkSize-offset;
				if(maxSize>0x1000)
					maxSize = 0x1000;
				size = Random(maxSize);
				r = Ldd.PinVirtualMemory((TLinAddr)chunk.Base()+offset, size);
				if(r!=KErrNotFound && r!=KErrNoMemory)
					{
					test_KErrNone(r);
					break;
					}
				}
			}
			break;
		case 12:
		case 13:
			// Move any page in the chunk, not just the owned region.
			{
#if !defined(__WINS__) && !defined(__X86__)
			for(TInt tries=10; tries>0; --tries)
				{
				TInt chunkSize = ChunkSize(chunkNumber);
				offset = Random(chunkSize);
				MoveLdd.TryMovingUserPage((TAny*)(chunk.Base()+offset), ETrue);
				// Allow the move to fail for any reason as the page of the chunk
				// may or may not be currently committed, pinned, or accessed.
				}
#endif
			}
			break;
		default:
			test(false); // can't happen
			break;
			}
		}
	}



TInt E32Main()
	{
	// get system info...
	MemModelAttributes = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	MemModel = MemModelAttributes&EMemModelTypeMask;
	UserHal::PageSizeInBytes(PageSize);
	PageMask = PageSize-1;

	// see if we are a slave process...
	if(User::GetTIntParameter(1,SlaveNumber)==KErrNone)
		{
		// do testing...
		SlaveInit();
		DoTest();
		return KErrGeneral; // shouldn't have returned from testing
		}

	// master process...
	TBool pass = true; // final test result
	test.Title();
	if((MemModelAttributes&EMemModelAttrVA)==false)
		{
		test.Start(_L("TESTS NOT RUN - Not relevent for the memory model"));
		test.End();
		return KErrNone;
		}

	// get time to run tests for...
	TInt timeout = 10; // time in seconds
	TInt cmdLineLen = User::CommandLineLength();
	if(cmdLineLen)
		{
		// get timeout value from command line
		RBuf cmdLine;
		test_KErrNone(cmdLine.Create(cmdLineLen));
		User::CommandLine(cmdLine);
		test_KErrNone(TLex(cmdLine).Val(timeout));
		if(timeout==0)
			timeout = KMaxTInt;
		}
	TTimeIntervalMicroSeconds32 tickTime;
	test_KErrNone(UserHal::TickPeriod(tickTime));
	TInt ticksPerSecond = 1000000/tickTime.Int();
	TInt timeoutTicks;
	if(timeout<KMaxTInt/ticksPerSecond)
		timeoutTicks = timeout*ticksPerSecond;
	else
		{
		timeoutTicks = KMaxTInt;
		timeout = timeoutTicks/ticksPerSecond;
		}

	// master process runs at higher priority than slaves so it can timeout and kill them...
	RThread().SetPriority(EPriorityMore);

	test.Start(_L("Creating test chunks"));
	TUint i;
	for(i=0; i<KNumTestChunks; i++)
		{
		test.Printf(_L("Size %dkB\r\n"),ChunkSize(i)>>10);
		test_KErrNone(Chunks[i].CreateDisconnectedGlobal(ChunkName(i),0,0,ChunkSize(i)));
		}

	test.Next(_L("Spawning slave processes"));
	test_KErrNone(StartSemaphore.CreateGlobal(KNullDesC,0));
	TFileName processFile(RProcess().FileName());
	for(i=0; i<KNumSlaveProcesses; i++)
		{
		test.Printf(_L("Slave %d\r\n"),i);
		RProcess& slave = Slaves[i];
		test_KErrNone(slave.Create(processFile,KNullDesC));
		test_KErrNone(slave.SetParameter(1,i));
		test_KErrNone(slave.SetParameter(2,StartSemaphore));
		slave.Logon(SlaveLogons[i]);
		test_Equal(KRequestPending,SlaveLogons[i].Int());
		slave.Rendezvous(SlaveRendezvous[i]);
		test_Equal(KRequestPending,SlaveRendezvous[i].Int());
		}

	test.Next(_L("Create timer"));
	RTimer timer;
	test_KErrNone(timer.CreateLocal());

	test.Next(_L("Resuming slave processes"));
	for(i=0; i<KNumSlaveProcesses; i++)
		Slaves[i].Resume();

	// this test must now take care not to die (e.g. panic due to assert fail)
	// until it has killed the slave processes

	test.Next(_L("Change paging cache size"));
	TUint cacheOriginalMin = 0;
	TUint cacheOriginalMax = 0;
	TUint cacheCurrentSize = 0;
	DPTest::CacheSize(cacheOriginalMin, cacheOriginalMax, cacheCurrentSize);
	DPTest::SetCacheSize(1, 2*ChunkSize(0)); // big enough for all the test chunks

	test.Next(_L("Wait for slaves to initialise"));
	TRequestStatus timeoutStatus;
	timer.After(timeoutStatus,10*1000000); // allow short time for slaves to initialise
	for(i=0; i<KNumSlaveProcesses; i++)
		{
		User::WaitForAnyRequest(); // wait for a rendexvous
		if(timeoutStatus.Int()!=KRequestPending)
			{
			test.Printf(_L("Timeout waiting for slaves to initialise\r\n"));
			pass = false;
			break;
			}
		}

	test.Next(_L("Restore paging cache size"));
	DPTest::SetCacheSize(cacheOriginalMin, cacheOriginalMax);

	if(pass)
		{
		timer.Cancel();
		User::WaitForAnyRequest(); // swallow timer signal

		test.Next(_L("Check slaves are ready"));
		for(i=0; i<KNumSlaveProcesses; i++)
			{
			if(SlaveRendezvous[i].Int()!=KErrNone || Slaves[i].ExitType()!=EExitPending)
				{
				test.Printf(_L("Slaves not ready or died!\r\n"));
				pass = false;
				break;
				}
			}
		}

	if(pass)
		{
		test.Next(_L("Setup simulated kernel heap failure"));
		__KHEAP_SETFAIL(RAllocator::EDeterministic,100);

		TBuf<80> text;
		text.Format(_L("Stressing for %d seconds..."),timeout);
		test.Next(text);
		timer.AfterTicks(timeoutStatus,timeoutTicks);
		StartSemaphore.Signal(KNumSlaveProcesses); // release slaves to start testing
		User::WaitForAnyRequest(); // wait for timeout or slave death via logon completion

		pass = timeoutStatus.Int()==KErrNone; // timeout means slaves are still running OK

		test.Next(_L("Check slaves still running"));
		for(i=0; i<KNumSlaveProcesses; i++)
			if(Slaves[i].ExitType()!=EExitPending)
				pass = false;

		test.Next(_L("Clear kernel heap failure"));
		TUint kheapFails = __KHEAP_CHECKFAILURE;
		__KHEAP_RESET;
		test.Printf(_L("Number of simulated memory failures = %d\r\n"),kheapFails);
		}

	test.Next(_L("Killing slave processes"));
	for(i=0; i<KNumSlaveProcesses; i++)
		Slaves[i].Kill(0);

	test.Next(_L("Assert test passed"));
	test(pass);

	test.End();

	for(i=0; i<KNumSlaveProcesses; i++)
		Slaves[i].Close();
	for(i=0; i<KNumTestChunks; i++)
		Chunks[i].Close();
	timer.Close();
	for(i=0; i<KNumSlaveProcesses; i++)
		User::WaitForRequest(SlaveLogons[i]);

	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);

	return KErrNone;
	}
