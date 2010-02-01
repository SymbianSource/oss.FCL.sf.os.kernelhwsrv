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
// e32test\mmu\t_sharedchunk.cpp
// Overview:
// Test sharing an RChunk with a logical device
// API Information:
// RChunk
// Details:
// - Load and open the logical device driver ("D_SHAREDCHUNK"). Verify
// results.
// - Test and verify results of creating shared chunks under OOM conditions. Also verify 
// creating a chunk with a bad type, bad size and too large all fail as 
// expected.
// - Test and verify opening and closing chunk user handles work as expected.
// - Test and verify thread local and process local handles work as expected
// - Test and verify setting restrictions on RChunk if created as shared chunk are as expected.
// - Test and verify memory access for multiply and singly shared chunks, 
// is as expected. Including IPC, kernel, DFC and ISR reads & writes.
// - Test and verify discontinuous memory commits for multiply and singly 
// shared chunks are as expected.
// - Test and verify continuous memory commits for multiply and singly shared
// chunks are as expected.
// - Test and verify discontinuous and continuous physical memory commits for 
// multiply and singly shared chunks is as expected.
// - Test Kern::OpenSharedChunk for multiply and singly shared chunks. Verify
// results are as expected.
// - Test that physical memory can be freed immediately after the chunk that mapped 
// it has been closed.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

//! @file
//! @SYMTestCaseID KBASE-T_SHAREDCHUNK
//! @SYMREQ 3699
//! @SYMTestPriority High
//! @SYMTestActions Check creation, memory allocation and access to Shared Chunks
//! @SYMTestExpectedResults Test runs until this message is emitted: RTEST: SUCCESS : T_SHAREDCHUNK test completed O.K.
//! @SYMTestType UT
#define __E32TEST_EXTENSION__

#include "d_sharedchunk.h"
#include "d_gobble.h"
#include <e32test.h>
#include <e32hal.h>
#include "u32std.h"
#include <u32hal.h>
#include <e32svr.h>
#include <f32dbg.h>
#include <e32def.h>
#include <e32def_private.h>
#include "freeram.h"

enum TSlaveCommand
	{
	ESlaveCheckChunk,
	ESlaveCreateChunk,
	};

//
// Global data
//


RSharedChunkLdd Ldd;
RChunk TheChunk;
TInt PageSize;
TUint32 MemModelAttributes;
TBool PhysicalCommitSupported;
TBool CachingAttributesSupported;
TUint8 BssMem[100];

const TUint ChunkSize = 0x400000;	// 4 meg reserved space for test chunk

_LIT(KSecondProcessName,"t_sharedchunk");
#ifdef __T_SHAREDCHUNKF__
_LIT(KOtherProcessName,"t_sharedchunk");
LOCAL_D RTest test(_L("T_SHAREDCHUNKF"));
#else
_LIT(KOtherProcessName,"t_sharedchunkf");
LOCAL_D RTest test(_L("T_SHAREDCHUNK"));
#endif

_LIT8(KTestString, "lks4b7qeyfcea5fyaifyaefyi4flwdysuxanabxa");
_LIT8(KTestString2,"jhfcalurnhfirlxszhrvcvduhrvndrucxnshxcsx");
const TUint32 KTestValue = 0x12345678;
const TUint32 KTestValue2 = KTestValue^0x0f0f0f0f;

//
// Utilitiies for use by tests
//


_LIT(KLitKernExec, "KERN-EXEC");

void CheckFailMessage(const char* n,const char* c,const char* f,TInt r)
	{
	TPtrC8 nn((const TUint8*)n);
	TPtrC8 cc((const TUint8*)c);
	TPtrC8 ff((const TUint8*)f);
	RBuf8 buf;
	buf.Create((nn.Size()+cc.Size()+ff.Size()+64)*2);
	buf.AppendFormat(_L8("\nCHECK failed: %S == 0x%x but was tested for %S%S\n"),&ff,r,&cc,&nn);
	test.Printf(buf.Expand());
	buf.Close();
	}

#define CHECK(n,c,f) \
	{	\
	TInt _r=(TInt)(f);	\
	if(!((TInt)(n)c(_r)))	\
		{	\
		CheckFailMessage(#n,#c,#f,_r);	\
		test(0);	\
		}	\
	}

#define KCHECK_MEMORY(result,offset)	\
	{	\
	/* test.Printf(_L("check offset 0x%08x\r"),offset); */	\
	CHECK(result,==,Ldd.CheckMemory(offset));	\
	}

#define KWRITE_MEMORY(offset,value)	\
	{	\
	CHECK(KErrNone,==,Ldd.WriteMemory(offset,value));	\
	}

#define UREAD_MEMORY(offset,value)	\
	{	\
	CHECK(value,==,*(TUint*)(Base+offset));	\
	}

inline TUint32 Tag(TUint32 offset)
	{ return (69069u*(offset*4+1)); }

TInt MemoryAccessThread(TAny* aAddress)
	{
	TInt r = *(volatile TUint8*)aAddress;	// read from aAddress
	(void)r;
	return 0;
	}

TInt CheckUMemory(TAny* aAddress)
	{
	RThread thread;
	thread.Create(KNullDesC,MemoryAccessThread,PageSize,&User::Heap(),aAddress);
	TRequestStatus status;
	thread.Logon(status);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(status);
	User::SetJustInTime(jit);
	TInt r;
	if(thread.ExitType()==EExitKill && thread.ExitReason()==0)
		r = 1;	// Memory access pass
	else if(thread.ExitType()==EExitPanic && thread.ExitCategory()==KLitKernExec && thread.ExitReason()==3 )
		r = 0;	// Memory access failed
	else
		r = -1;	// Unexpected result
	CLOSE_AND_WAIT(thread);
	return r;
	}

#define UCHECK_MEMORY(result,offset)	\
	{	\
	/* test.Printf(_L("ucheck offset 0x%08x\r"),offset); */	\
	CHECK(result,==,CheckUMemory(Base+offset));	\
	}

TInt CheckPlatSecPanic(TThreadFunction aThreadFunction,TInt aThreadArgument)
	{
	RThread thread;
	thread.Create(KNullDesC,aThreadFunction,PageSize,&User::Heap(),(TAny*)aThreadArgument);
	TRequestStatus status;
	thread.Logon(status);
	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(status);
	User::SetJustInTime(jit);
	TInt r;
	if(thread.ExitType()==EExitPanic && thread.ExitCategory()==KLitKernExec && thread.ExitReason()==46 )
		r = 1;	// PlatSec panic
	else if(thread.ExitType()==EExitKill && thread.ExitReason()==0)
		r = 0;	// Exit without error
	else
		r = -1;	// Unexpected result
	CLOSE_AND_WAIT(thread);
	return r;
	}

//
// The tests
//

void CreateWithOomCheck(TInt aCreateFlags)
	{
	TInt failResult=KErrGeneral;
	for(TInt failCount=1; failCount<1000; failCount++)
		{
		test.Printf(_L("alloc fail count = %d\n"),failCount);
		User::__DbgSetBurstAllocFail(ETrue,RAllocator::EFailNext,failCount,1000);
		__KHEAP_MARK;
		failResult = Ldd.CreateChunk(aCreateFlags);
		if(failResult==KErrNone)
			break;
		CHECK(KErrNoMemory,==,failResult);
		Ldd.IsDestroyed();	// This includes delay to let idle thread do cleanup
		__KHEAP_MARKEND;
		}
	User::__DbgSetAllocFail(ETrue,RAllocator::ENone,0);
	__KHEAP_RESET;
	CHECK(KErrNone,==,failResult);
	}

void TestCreate()
	{
	test.Start(_L("Creating chunk type Single,OwnsMemory"));
	CreateWithOomCheck(ChunkSize|ESingle|EOwnsMemory);
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Close kernel handle"));
	CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted
	CHECK(1,==,Ldd.IsDestroyed());

	test.Next(_L("Creating chunk type Multiple,OwnsMemory"));
	CreateWithOomCheck(ChunkSize|EMultiple|EOwnsMemory);
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Close kernel handle"));
	CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted
	CHECK(1,==,Ldd.IsDestroyed());

	if(PhysicalCommitSupported)
		{
		test.Next(_L("Creating chunk type Single,!OwnsMemory"));
		CreateWithOomCheck(ChunkSize|ESingle);
		CHECK(0,==,Ldd.IsDestroyed());

		test.Next(_L("Close kernel handle"));
		CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted
		CHECK(1,==,Ldd.IsDestroyed());

		test.Next(_L("Creating chunk type Multiple,!OwnsMemory"));
		CreateWithOomCheck(ChunkSize|EMultiple);
		CHECK(0,==,Ldd.IsDestroyed());

		test.Next(_L("Close kernel handle"));
		CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted
		CHECK(1,==,Ldd.IsDestroyed());
		}
	else
		{
		test.Next(_L("Try creating unsupported chunk type Single,!OwnsMemory"));
		CHECK(KErrNotSupported,==,Ldd.CreateChunk(ChunkSize|ESingle));

		test.Next(_L("Try creating unsupported chunk type Multiple,!OwnsMemory"));
		CHECK(KErrNotSupported,==,Ldd.CreateChunk(ChunkSize|EMultiple));
		}

	test.Next(_L("__KHEAP_MARK"));
	__KHEAP_MARK;

	test.Next(_L("Creating chunk (bad type)"));
	CHECK(KErrArgument,==,Ldd.CreateChunk(EBadType|EOwnsMemory|ChunkSize));

	test.Next(_L("Creating chunk (bad size)"));
	CHECK(KErrArgument,==,Ldd.CreateChunk(ESingle|EOwnsMemory|0xffffff00));

	test.Next(_L("Creating chunk (size too big)"));
	CHECK(KErrNoMemory,==,Ldd.CreateChunk(ESingle|EOwnsMemory|0x7fffff00));

	test.Next(_L("__KHEAP_MARKEND"));
	__KHEAP_MARKEND;

	test.End();
	}


void OpenWithOomCheck(RChunk& aChunk)
	{
	TInt failResult=KErrGeneral;
	for(TInt failCount=1; failCount<1000; failCount++)
		{
		test.Printf(_L("alloc fail count = %d\n"),failCount);
		User::__DbgSetBurstAllocFail(ETrue,RAllocator::EFailNext,failCount,1000);
		__KHEAP_MARK;
		failResult = Ldd.GetChunkHandle(aChunk);
		if(failResult==KErrNone)
			break;
		CHECK(KErrNoMemory,==,failResult);
		Ldd.IsDestroyed();	// This includes delay to let idle thread do cleanup
		__KHEAP_MARKEND;
		}
	User::__DbgSetAllocFail(ETrue,RAllocator::ENone,0);
	__KHEAP_RESET;
	CHECK(KErrNone,==,failResult);
	}

void TestHandles()
	{
	TUint ChunkAttribs = ChunkSize|ESingle|EOwnsMemory;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Open user handle"));
	OpenWithOomCheck(TheChunk);

	test.Next(_L("Close user handle"));
	TheChunk.Close();

	test.Next(_L("Check chunk not destroyed"));
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Close kernel handle"));
	CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted

	test.Next(_L("Check chunk destroyed"));
	CHECK(1,==,Ldd.IsDestroyed());

	// Another chunk - closing handles in reverse order

	test.Next(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Open user handle"));
	OpenWithOomCheck(TheChunk);

	test.Next(_L("Close kernel handle"));
	CHECK(KErrNone,==,Ldd.CloseChunk());

	test.Next(_L("Check chunk not destroyed"));
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Using user handle to check chunk info"));
	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		CHECK(0,==,TheChunk.Size());
		}
	CHECK(ChunkSize,==,TheChunk.MaxSize());

	test.Next(_L("Close user handle"));
	TheChunk.Close();

	test.Next(_L("Check chunk destroyed"));
	CHECK(1,==,Ldd.IsDestroyed());

	test.End();
	}

TInt HandleOwnershipThread(TAny* aArg)
	{
	// Use existing handle and attempt to read from chunk
	TInt handle = (TInt) aArg;
	RChunk chunk;
	chunk.SetHandle(handle);
	TInt r = *(volatile TUint8*)chunk.Base();
	(void)r;
	CLOSE_AND_WAIT(chunk);
	return KErrNone;
	}

void TestHandleOwnership()
	{
	TUint ChunkAttribs = ChunkSize|ESingle|EOwnsMemory;
	RThread thread;
	TRequestStatus rs;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Commit page to chunk"));
	CHECK(KErrNone,==,Ldd.CommitMemory(EDiscontiguous,PageSize));

	test.Next(_L("Check can access memory kernel side"));
	KCHECK_MEMORY(ETrue, 0);

	// Handle is thread-owned
	test.Next(_L("Open user handle (thread-owned)"));
	CHECK(0,<=,Ldd.GetChunkHandle(TheChunk, ETrue));

	test.Next(_L("Get memory size info"));
	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		CHECK(PageSize,==,TheChunk.Size());
		}
	CHECK(ChunkSize,==,TheChunk.MaxSize());
	TUint8* Base = TheChunk.Base();
	CHECK(Base,!=,0);

	test.Next(_L("Check can access memory user side"));
	UCHECK_MEMORY(ETrue, 0);

	test.Next(_L("Use handle in a new thread"));
	CHECK(KErrNone,==,thread.Create(_L("thread1"), HandleOwnershipThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, (TAny*)TheChunk.Handle()));
	thread.Logon(rs);
	thread.Resume();
	User::WaitForRequest(rs);
	CHECK(EExitPanic,==,thread.ExitType());
	CHECK(0,==,thread.ExitReason()); // KERN-EXEC 0
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Close user handle"));
	TheChunk.Close();

	// Handle is process-owned
	test.Next(_L("Open user handle (process-owned"));
	CHECK(0,<=,Ldd.GetChunkHandle(TheChunk, EFalse));

	test.Next(_L("Check can access memory user side"));
	UCHECK_MEMORY(ETrue, 0);

	test.Next(_L("Close kernel handle"));
	CHECK(KErrNone,==,Ldd.CloseChunk());

	test.Next(_L("Check chunk destroyed"));
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Use handle in a new thread"));
	CHECK(KErrNone,==,thread.Create(_L("thread2"), HandleOwnershipThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, (TAny*)TheChunk.Handle()));
	thread.Logon(rs);
	thread.Resume();
	User::WaitForRequest(rs);
	CHECK(EExitKill,==,thread.ExitType());
	CHECK(KErrNone,==,thread.ExitReason());
	CLOSE_AND_WAIT(thread);

	test.Next(_L("Check chunk destroyed"));
	CHECK(1,==,Ldd.IsDestroyed()); // Object was deleted

	test.End();
	}

void SetCreateFlags(TUint& aCreateFlags,TCommitType aCommitType)
	{
	if(!((TInt)aCommitType&EPhysicalMask))
		aCreateFlags |= EOwnsMemory;
	else
		aCreateFlags &= ~EOwnsMemory;
	}


void TestAccess(TUint aCreateFlags,TCommitType aCommitType)
	{
	const TUint32 offset = 0;
	const TUint32 size = PageSize;

	SetCreateFlags(aCreateFlags,aCommitType);

	test.Start(_L("Create chunk"));
	TUint8* kernelAddress;
	CHECK(KErrNone,==,Ldd.CreateChunk(aCreateFlags|ChunkSize,(TAny**)&kernelAddress));

	if((MemModelAttributes&TUint32(EMemModelAttrNonExProt)) && (MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		test.Next(_L("Check can't access memory"));
		KCHECK_MEMORY(EFalse,offset);
		}

	test.Next(_L("Commit page to chunk"));
	CHECK(KErrNone,==,Ldd.CommitMemory(aCommitType|offset,size));

	test.Next(_L("Check can access memory kernel side"));
	KCHECK_MEMORY(ETrue, offset);

	if((MemModelAttributes&EMemModelAttrKernProt) && (MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		test.Next(_L("Check user side can't access kernel memory"));
		TUint8* Base = kernelAddress;
		UCHECK_MEMORY(EFalse, offset);
		}

	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	test.Next(_L("Get memory size info"));
	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		CHECK(PageSize,==,TheChunk.Size());
		}
	CHECK(ChunkSize,==,TheChunk.MaxSize());
	TUint8* Base = TheChunk.Base();
	CHECK(Base,!=,0);

	test.Next(_L("Check can access memory user side"));
	UCHECK_MEMORY(ETrue, offset);

	test.Next(_L("Check user and kernel access same memory"));
	KWRITE_MEMORY(offset,~Tag(offset));
	UREAD_MEMORY(offset,~Tag(offset));
	KWRITE_MEMORY(offset,Tag(offset));
	UREAD_MEMORY(offset,Tag(offset));

	test.Next(_L("Close user handle"));
	CHECK(0,==,Ldd.CloseChunkHandle(TheChunk));
	CHECK(0,==,Ldd.IsDestroyed());

	if((MemModelAttributes&EMemModelAttrKernProt) && (MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		test.Next(_L("Check can no-longer access memory user side"));
		UCHECK_MEMORY(EFalse,offset);
		}

	test.Next(_L("Check can still access memory kernel side"));
	KCHECK_MEMORY(ETrue, offset);

	test.Next(_L("Open user handle again"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	test.Next(_L("Check can access chunk user side again"));
	CHECK(Base,==,TheChunk.Base());
	CHECK(ChunkSize,==,TheChunk.MaxSize());
	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		CHECK(size,==,TheChunk.Size());
		}
	UREAD_MEMORY(offset,Tag(offset));

	test.Next(_L("Close kernel handle"));
	CHECK(0,==,Ldd.CloseChunk());
	CHECK(0,==,Ldd.IsDestroyed());

	test.Next(_L("Check can still access chunk user side"));
	CHECK(Base,==,TheChunk.Base());
	CHECK(ChunkSize,==,TheChunk.MaxSize());
	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		CHECK(size,==,TheChunk.Size());
		}
	UREAD_MEMORY(offset,Tag(offset));

	test.Next(_L("Close user handle"));
	TheChunk.Close();
	CHECK(1,==,Ldd.IsDestroyed());

	test.Next(_L("Create chunk in another process"));

	// Create test server
	RServer2 server;
	RMessage2 message;
	TRequestStatus status;
	CHECK(KErrNone,==,server.CreateGlobal(KSecondProcessName));
	server.Receive(message,status);

	// Launch slave process
	RProcess process;
	CHECK(KErrNone,==,process.Create(KSecondProcessName,KNullDesC));
	CHECK(KErrNone,==,process.SetParameter(1,ESlaveCreateChunk));
	CHECK(KErrNone,==,process.SetParameter(2,(RBusLogicalChannel&)Ldd));
	CHECK(KErrNone,==,process.SetParameter(3,ChunkSize|aCreateFlags));
	CHECK(KErrNone,==,process.SetParameter(4,aCommitType));
	CHECK(KErrNone,==,process.SetParameter(5,PageSize));
	TRequestStatus logon;
	process.Logon(logon);
	process.Resume();

	// Wait for slave to connect to test server
	User::WaitForRequest(logon,status);
	CHECK(KRequestPending,==,logon.Int())
	CHECK(KErrNone,==,status.Int());
	CHECK(RMessage2::EConnect,==,message.Function());
	message.Complete(KErrNone);
	server.Receive(message,status);

	// Wait for message
	User::WaitForRequest(logon,status);
	CHECK(KRequestPending,==,logon.Int())
	CHECK(KErrNone,==,status.Int());
	CHECK(0,==,message.Function());

	test.Next(_L("Check IPC read/write"));
	RBuf8 buf;
	buf.Create(KTestString().Size());
	CHECK(KErrNone,==,message.Read(0,buf));
	CHECK(ETrue,==,buf==KTestString());
	CHECK(KErrNone,==,message.Write(0,KTestString2));
	CHECK(KErrNone,==,message.Read(0,buf));
	CHECK(ETrue,==,buf==KTestString2());

	test.Next(_L("Check Kernel read/write"));
	TInt n;
	TUint32 value;
	for(n=0; n<KTestString2().Size()-(TInt)sizeof(TInt)+1; n+=sizeof(TInt))
		{
		Ldd.ReadMemory(n,value);
		CHECK(*(TInt*)&KTestString2()[n],==,value);
		CHECK(KErrNone,==,Ldd.WriteMemory(n,*(TInt*)&KTestString()[n]));
		Ldd.ReadMemory(n,value);
		CHECK(*(TInt*)&KTestString()[n],==,value);
		}
	CHECK(KErrNone,==,message.Read(0,buf));
	CHECK(ETrue,==,buf==KTestString());
	buf.Close();

	test.Next(_L("Check read/write from DFC"));
	CHECK(KErrNone,==,Ldd.WriteMemory(0,KTestValue));
	value = KTestValue2;
	CHECK(KErrNone,==,Ldd.DfcReadWrite(0,value));
	CHECK(KTestValue,==,value);
	CHECK(KErrNone,==,Ldd.ReadMemory(0,value));
	CHECK(KTestValue2,==,value);
	
	test.Next(_L("Check read/write from ISR"));
	CHECK(KErrNone,==,Ldd.WriteMemory(0,KTestValue));
	value = KTestValue2;
	CHECK(KErrNone,==,Ldd.IsrReadWrite(0,value));
	CHECK(KTestValue,==,value);
	CHECK(KErrNone,==,Ldd.ReadMemory(0,value));
	CHECK(KTestValue2,==,value);

	test.Next(_L("Cleanup resources"));
	server.Close();
	User::WaitForRequest(logon);
	CLOSE_AND_WAIT(process);

	test.End();
	}


RProcess OtherProcess;

TInt HandleShare(TAny* aHandle)
	{
	TInt r=OtherProcess.Create(KOtherProcessName,KNullDesC,EOwnerProcess);
	if(r==KErrNone)
		{
		r = OtherProcess.SetParameter(1,(RChunk&)aHandle);
		}
	return r;
	}

void TestRestrictions(TInt aAttrib)
	{
	TUint ChunkAttribs = ChunkSize|aAttrib|EOwnsMemory;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	test.Next(_L("Try changing restrictions"));
	CHECK(KErrAccessDenied,==,TheChunk.SetRestrictions(0));

	test.Next(_L("Check allocation restrictions"));
	CHECK(KErrAccessDenied,==,TheChunk.Adjust(ChunkSize/2));
	CHECK(KErrAccessDenied,==,TheChunk.AdjustDoubleEnded(PageSize,ChunkSize));
	CHECK(KErrAccessDenied,==,TheChunk.Commit(PageSize,PageSize));
	CHECK(KErrAccessDenied,==,TheChunk.Allocate(PageSize));
	CHECK(KErrAccessDenied,==,TheChunk.Decommit(PageSize,PageSize));

	test.Next(_L("Duplicate handle in same process"));
	RChunk chunk2;
	chunk2.SetHandle(TheChunk.Handle());
	CHECK(KErrNone,==,chunk2.Duplicate(RThread(),EOwnerProcess));

	test.Next(_L("Try passing handle to other process"));
	if(aAttrib==EMultiple)
		{
		CHECK(0,==,CheckPlatSecPanic(HandleShare,*(TInt*)&chunk2));
		}
	else
		{
		CHECK(1,==,CheckPlatSecPanic(HandleShare,*(TInt*)&chunk2));
		}
	// Cleanup leftover process
	OtherProcess.Kill(0);
	OtherProcess.Close();

	test.Next(_L("Close handles"));
	chunk2.Close();
	CHECK(0,==,Ldd.CloseChunk());
	TheChunk.Close();

	test.End();
	}


class TCommitRegion
	{
public:
	TInt iOffset;
	TInt iSize;
	TInt iExpectedResult;
	};

void CheckRegion(TCommitRegion aRegion, TBool aCheckClear)
	{
	TUint8* Base = TheChunk.Base();
	TInt offset = aRegion.iOffset*PageSize;
	TInt limit = offset+aRegion.iSize*PageSize;
	while(offset<limit)
		{
		KCHECK_MEMORY(1,offset);
		UCHECK_MEMORY(1,offset);
		offset += PageSize;
		}
	if(!aCheckClear)
		return;

	TUint32* ptr = (TUint32*)(Base+aRegion.iOffset*PageSize);
	TUint32* end = (TUint32*)((TInt)ptr+aRegion.iSize*PageSize);
	while(ptr<end)
		if(*ptr++!=0x03030303u)
			{
			CHECK(0x03030303u,==,*ptr);
			}
	};

void SetTags(TCommitRegion aRegion)
	{
	TUint8* Base = TheChunk.Base();
	TInt offset = aRegion.iOffset*PageSize;
	TInt limit = offset+aRegion.iSize*PageSize;
	while(offset<limit)
		{
		*(TUint*)(Base+offset) = Tag(offset);
		offset += sizeof(TUint);
		}
	};

void CheckTags(const TCommitRegion& aRegion)
	{
	TUint8* Base = TheChunk.Base();
	TInt offset = aRegion.iOffset*PageSize;
	TInt limit = offset+aRegion.iSize*PageSize;
	while(offset<limit)
		{
		KCHECK_MEMORY(1,offset);	// Check page exists
		TInt limit2 = offset+PageSize;
		while(offset<limit2)
			{
			UREAD_MEMORY(offset,Tag(offset));	// Check contents match tags we set previousely
			offset += sizeof(TUint);
			}
		}
	};

void CheckOldTags(const TCommitRegion& aRegion, TInt aNewOffset)
	{
	TUint8* Base = TheChunk.Base();
	TInt oldOffset = aRegion.iOffset*PageSize;
	TInt offset = aNewOffset;
	TInt limit = offset+aRegion.iSize*PageSize;
	while(offset<limit)
		{
		KCHECK_MEMORY(1,offset);	// Check page exists
		TInt limit2 = offset+PageSize;
		while(offset<limit2)
			{
			UREAD_MEMORY(offset,Tag(oldOffset));	// Check contents matched old tags
			offset += sizeof(TUint);
			oldOffset += sizeof(TUint);
			}
		}
	};

// Following assumes 4k pages...
static const TCommitRegion CommitList[] =
	{
		{0,0},		// zero size commit
		{1,1},		// page 1

		// Regions which overlap previous commit
		{1,1,KErrAlreadyExists},
		{0,2,KErrAlreadyExists},
		{1,2,KErrAlreadyExists},
		{0,3,KErrAlreadyExists},

		{0,1},		// page 0
		{2,1},		// page 2

		{250,6},	// pages at end of chunk boundary

		{768,256},	// whole 1meg chunk

		{400,512,KErrAlreadyExists},	// Monster commit which overlaps previous regions

		{256,257},	// big commit which straddles more than one 1meg chunk

		{-1,-1} // End marker
	};

void CheckCommitedContents(TInt aIndex)
	{
	while(--aIndex>=0)
		if(CommitList[aIndex].iExpectedResult==KErrNone)
			CheckTags(CommitList[aIndex]);
	};

void CheckCommitState(TInt aIndex)
	{
	TInt page=0;
	TInt lastPage=TheChunk.MaxSize()/PageSize;
	while(page<lastPage)
		{
		TInt i=aIndex;
		while(--i>=0)
			if(CommitList[i].iExpectedResult==KErrNone)
				if((TUint)(page-CommitList[i].iOffset) < (TUint)CommitList[i].iSize)
					break;
		TInt offset = page*PageSize;
		if(i>=0)
			{
			KCHECK_MEMORY(1,offset);	// Check page exists
			}
		else
			{
			KCHECK_MEMORY(0,offset);	// Check page doesn't exists
			}
		++page;
		}
	};


void TestCommit(TUint aCreateFlags,TCommitType aCommitType)
	{
	SetCreateFlags(aCreateFlags,aCommitType);
	TUint ChunkAttribs = ChunkSize|aCreateFlags;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Check wrong commit type"));
	CHECK(KErrNotSupported,==,Ldd.CommitMemory((aCommitType^EPhysicalMask)|0,PageSize));
	CHECK(KErrNotSupported,==,Ldd.CommitMemory((aCommitType^EPhysicalMask^EContiguous)|0,PageSize));

	if((TInt)aCommitType&EPhysicalMask)
		{
		test.Next(_L("Check commit with bad pysical address"));
		CHECK(KErrArgument,==,Ldd.CommitMemory((aCommitType|EBadPhysicalAddress)|0,PageSize));
		}

	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	const TCommitRegion* list = CommitList;
	for(;list->iOffset>=0; ++list)
		{
		TInt offset = list->iOffset*PageSize;
		TInt size = list->iSize*PageSize;
		TInt expectedResult = list->iExpectedResult;
		if((MemModelAttributes&EMemModelTypeMask)==EMemModelTypeDirect && expectedResult==KErrAlreadyExists)
			continue;
		TBuf<100> text;
		text.AppendFormat(_L("Commit pages: offset=%08x size=%08x expectedResult=%d"),offset,size,expectedResult);
		test.Next(text);

		test.Start(_L("Do the Commit"));
		CHECK(expectedResult,==,Ldd.CommitMemory(aCommitType|offset,size));

		if(expectedResult==KErrNone)
			{
			test.Next(_L("Check new memory has been comitted"));
			CheckRegion(*list,!(aCommitType&EPhysicalMask));
			}

		if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
			{
			test.Next(_L("Check commit state of all pages in chunk"));
			CheckCommitState(list-CommitList+1);
			}

		test.Next(_L("Check contents of previous commited regions are unchanged"));
		CheckCommitedContents(list-CommitList);

		if(expectedResult==KErrNone)
			{
			test.Next(_L("Mark new memory"));
			SetTags(*list);
			}
		test.End();
		}

	if((aCreateFlags&EMultiple) && (MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		test.Next(_L("Check another process sees same chunk state"));
		TInt regionCount = list-CommitList;

		// create another process
		RProcess process;
		CHECK(KErrNone,==,process.Create(KOtherProcessName,KNullDesC));
		CHECK(KErrNone,==,process.SetParameter(1,ESlaveCheckChunk));
		CHECK(KErrNone,==,process.SetParameter(2,(RBusLogicalChannel&)Ldd));
		CHECK(KErrNone,==,process.SetParameter(3,(RChunk&)TheChunk));
		CHECK(KErrNone,==,process.SetParameter(4,regionCount));
		TRequestStatus status;
		process.Logon(status);
		process.Resume();

		// Check chunk again in this process, concurrently with other process
		CheckCommitedContents(regionCount);
		CheckCommitState(regionCount);

		// wait for other process to finish
		User::WaitForRequest(status);
		CHECK(EExitKill,==,process.ExitType());
		CHECK(0,==,process.ExitReason());
		CLOSE_AND_WAIT(process);
		}

	test.Next(_L("Close handles"));
	TheChunk.Close();
	CHECK(1,==,Ldd.CloseChunk());

	if(aCommitType&EPhysicalMask)
		{
		// For Physical commit tests, check correct allocation by creating a new chunk
		// and checking that pages comitted contain the old TAGs placed there by the
		// tests above.

		test.Next(_L("Check commit uses correct physical pages"));

		test.Start(_L("Create chunk"));
		CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

		test.Next(_L("Open user handle"));
		CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

		TInt offset = 0;
		for(list=CommitList; list->iOffset>=0; ++list)
			{
			if(list->iExpectedResult!=KErrNone)
				continue;

			TInt size = list->iSize*PageSize;
			TBuf<100> text;
			text.AppendFormat(_L("Commit pages: offset=%08x size=%08x"),offset,size);
			test.Next(text);
			CHECK(KErrNone,==,Ldd.CommitMemory(aCommitType|offset,size));

			test.Next(_L("Check RAM contents preserved from previous usage"));
			CheckOldTags(*list,offset);
			offset += size;
			}

		test.Next(_L("Close handles"));
		TheChunk.Close();
		CHECK(1,==,Ldd.CloseChunk());

		test.End();
		}
	else
		{
		// We don't do these OOM tests for Physical commit because we can't do it reliably
		// (as only a couple of page tables come from the free pool not the whole memory
		// to be comitted)
		test.Next(_L("Check Out Of Memory conditions"));

		// Make sure any clean up has happened otherwise the amount of free RAM may change.
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

		test.Start(_L("Gobble up most of RAM"));
		test.Next(_L("Load gobbler LDD"));
		TInt r = User::LoadLogicalDevice(KGobblerLddFileName);
		test(r==KErrNone || r==KErrAlreadyExists);
		RGobbler gobbler;
		r = gobbler.Open();
		test(r==KErrNone);
		TUint32 taken = gobbler.GobbleRAM(2*1024*1024);
		test.Printf(_L("Gobbled: %dK\n"), taken/1024);
		test.Printf(_L("Free RAM 0x%08X bytes\n"),FreeRam());

		test.Next(_L("Get baseline free memory"));
		__KHEAP_MARK;
		TInt freeRam1 = FreeRam();

		test.Next(_L("Create shared chunk"));
		CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));
		TInt freeRam2 = FreeRam();

		test.Next(_L("Commit memory which will causes OOM"));
		CHECK(KErrNoMemory,==,Ldd.CommitMemory(aCommitType,4096*1024));

		test.Next(_L("Check free RAM unchanged"));
		CHECK(freeRam2,==,FreeRam());

		test.Next(_L("Check OOM during ChunkCommit"));
		TInt failResult=KErrGeneral;
		for(TInt failCount=1; failCount<1000; failCount++)
			{
			User::__DbgSetAllocFail(ETrue,RAllocator::EFailNext,failCount);
			failResult = Ldd.CommitMemory(aCommitType,1);
			if(failResult==KErrNone)
				break;
			CHECK(KErrNoMemory,==,failResult);
			}
		User::__DbgSetAllocFail(ETrue,RAllocator::ENone,0);
		CHECK(KErrNone,==,failResult);

		test.Next(_L("Destroy shared chunk"));
		CHECK(1,==,Ldd.CloseChunk());
		CHECK(1,==,Ldd.IsDestroyed());

		test.Next(_L("Check free memory returns to baseline"));
		CHECK(freeRam1,==,FreeRam());
		__KHEAP_MARKEND;

		test.Next(_L("Free gobbled RAM"));
		gobbler.Close();

		test.End();
		}

	test.End();
	}


void TestOpenSharedChunk(TUint aCreateFlags,TCommitType aCommitType)
	{
	SetCreateFlags(aCreateFlags,aCommitType);
	TUint ChunkAttribs = ChunkSize|aCreateFlags;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	test.Next(_L("Commit some memory"));
	CHECK(KErrNone,==,Ldd.CommitMemory(aCommitType|1*PageSize,PageSize));
	CHECK(KErrNone,==,Ldd.CommitMemory(aCommitType|2*PageSize,PageSize));
	CHECK(KErrNone,==,Ldd.CommitMemory(aCommitType|4*PageSize,PageSize));

	test.Next(_L("Check OpenSharedChunk with handle"));
	CHECK(KErrNone,==,Ldd.TestOpenHandle(TheChunk.Handle()));

	test.Next(_L("Check OpenSharedChunk with wrong chunk handle"));
	RChunk testChunk;
	CHECK(KErrNone,==,testChunk.CreateLocal(PageSize,PageSize));
	CHECK(KErrNotFound,==,Ldd.TestOpenHandle(testChunk.Handle()));
	testChunk.Close();

	test.Next(_L("Check OpenSharedChunk with wrong handle type"));
	CHECK(KErrNotFound,==,Ldd.TestOpenHandle(RThread().Handle()));

	test.Next(_L("Check OpenSharedChunk with bad handle"));
	CHECK(KErrNotFound,==,Ldd.TestOpenHandle(0));

	test.Next(_L("Check OpenSharedChunk with address"));
	TUint8* Base = TheChunk.Base();
	CHECK(KErrNone,==,Ldd.TestOpenAddress(Base));
	CHECK(KErrNone,==,Ldd.TestOpenAddress(Base+ChunkSize-1));

	test.Next(_L("Check OpenSharedChunk with bad address"));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(Base-1));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(Base+ChunkSize));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(0));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress((TAny*)~0));

	test.Next(_L("Check OpenSharedChunk with stack memory address"));
	TUint8 stackMem[100];
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(stackMem));

	test.Next(_L("Check OpenSharedChunk with heap memory address"));
	TUint8* heapMem = new TUint8[100];
	CHECK(0,!=,heapMem);
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(heapMem));
	delete [] heapMem;

	test.Next(_L("Check OpenSharedChunk with BSS memory address"));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(BssMem));

	test.Next(_L("Check OpenSharedChunk with code memory address"));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress((TAny*)&TestOpenSharedChunk));

	test.Next(_L("Check OpenSharedChunk with NULL address"));
	CHECK(KErrNotFound,==,Ldd.TestOpenAddress(0));

	test.Next(_L("Check ChunkAddress for given memory region"));
	static const TCommitRegion regions[] = 
		{
			{0,1,KErrNotFound},
			{0,2,KErrNotFound},
			{0,3,KErrNotFound},
			{1,1},
			{1,2},
			{1,3,KErrNotFound},
			{2,1},
			{2,2,KErrNotFound},
			{2,3,KErrNotFound},
			{3,1,KErrNotFound},
			{3,2,KErrNotFound},
			{3,3,KErrNotFound},
			{4,1},
			{4,2,KErrNotFound},
			{4,3,KErrNotFound},
			{0,10240,KErrArgument}, // too big
			{1,0,KErrArgument}, // bad size
			{1,-1,KErrArgument}, // bad size
			{10240,1,KErrArgument}, // bad offset
			{-2,2,KErrArgument}, // bad offset
			{-1}
		};
	const TCommitRegion* region = regions;
	for(;region->iOffset!=-1; ++region)
		{
		TUint32 offset = region->iOffset*PageSize;
		TUint32 size = region->iSize*PageSize;
		TInt expectedResult = region->iExpectedResult;
		if((MemModelAttributes&EMemModelTypeMask)==EMemModelTypeDirect && expectedResult==KErrNotFound)
			continue;
		TBuf<100> text;
		text.AppendFormat(_L("Memory region: offset=%08x size=%08x expectedResult=%d"),offset,size,expectedResult);
		test.Next(text);
		CHECK(expectedResult,==,Ldd.TestAddress(offset,size));
		}

	test.Next(_L("Close handles"));
	TheChunk.Close();
	CHECK(1,==,Ldd.CloseChunk());

	test.End();
	}


void AccessSpeed(TCreateFlags aCreateFlags, TInt& aRead,TInt& aWrite)
	{
//	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkSize|ESingle|EOwnsMemory|aCreateFlags));

//	test.Next(_L("Commit some memory"));
	if((MemModelAttributes&EMemModelTypeMask)==EMemModelTypeDirect)
		{
		CHECK(KErrNone,==,Ldd.CommitMemory(EDiscontiguous|0*PageSize,PageSize));
		}
	else
		{
		// Allocate contiguous memory when possible so that the
		// Cache::SyncMemoryBeforeXxxx calls in the test driver get exercised
		CHECK(KErrNone,==,Ldd.CommitMemory(EContiguous|0*PageSize,PageSize));
		}

//	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));
	volatile TUint32* p = (TUint32*)TheChunk.Base();

	TUint32 time;
	TInt itterCount=128;
	do
		{
		itterCount *= 2;
		TUint32 lastCount=User::NTickCount();
		for(TInt i=itterCount; i>0; --i)
			{
			TUint32 x=p[0]; x=p[1]; x=p[2]; x=p[3]; x=p[4]; x=p[5]; x=p[6]; x=p[7];
			}
		time = User::NTickCount()-lastCount;
		}
	while(time<200);
	aRead = itterCount*8/time;

	itterCount=128;
	do
		{
		itterCount *= 2;
		TUint32 lastCount=User::NTickCount();
		for(TInt i=itterCount; i>0; --i)
			{
			p[0]=i; p[1]=i; p[2]=i; p[3]=i; p[4]=i; p[5]=i; p[6]=i; p[7]=i;
			}
		time = User::NTickCount()-lastCount;
		}
	while(time<200);
	aWrite = itterCount*8/time;

	TBuf<100> text;
	text.AppendFormat(_L("Read speed=%7d    Write speed=%7d\n"),aRead,aWrite);
	test.Printf(text);

//	test.Next(_L("Close handles"));
	TheChunk.Close();
	CHECK(1,==,Ldd.CloseChunk());

//	test.End();
	}

void TestMappingAttributes()
	{
	test.Start(_L("Fully Blocking"));
	TInt blockedRead;
	TInt blockedWrite;
	AccessSpeed(EBlocking,blockedRead,blockedWrite);

	TInt read;
	TInt write;

	test.Next(_L("Write Buffered"));
	AccessSpeed(EBuffered,read,write);
	CHECK(2*blockedRead,>,read);
//	CHECK(2*blockedWrite,<,write);  // Write buffering doesn't seem to work when cache disabled (?)

	test.Next(_L("Fully Cached"));
	AccessSpeed(ECached,read,write);
	CHECK(2*blockedRead,<,read);
#ifndef __X86__	// X86 seems to do always do write buffering
	// Following check disabled because most dev boards only seem to be a bit faster
	// and asserting a particular speed improvement is unreliable
//	CHECK(2*blockedWrite,<,write);
#endif

	test.End();
	}

class RSession : public RSessionBase
	{
public:
	inline TInt CreateSession(const TDesC& aServer,const TVersion& aVersion)
		{ return RSessionBase::CreateSession(aServer,aVersion); }
	inline TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
		{ return RSessionBase::SendReceive(aFunction,aArgs); }
	};

TInt SlaveCommand(TSlaveCommand aCommand)
	{
	RDebug::Print(_L("Slave Process - Command %d\n"),aCommand);
	CHECK(KErrNone,==,UserHal::PageSizeInBytes(PageSize));
	CHECK(KErrNone,==,((RBusLogicalChannel&)Ldd).Open(2,EOwnerProcess));
	switch(aCommand)
		{
	case ESlaveCheckChunk:
		{
		RDebug::Print(_L("Slave Process - TheChunk.Open()\n"));
		CHECK(KErrNone,==,TheChunk.Open(3));
		RDebug::Print(_L("Slave Process - Get Region Count\n"));
		TInt regionCount;
		CHECK(KErrNone,==,User::GetTIntParameter(4,regionCount));
		RDebug::Print(_L("Slave Process - CheckCommitedContents(%d)\n"),regionCount);
		CheckCommitedContents(regionCount);
		RDebug::Print(_L("Slave Process - CheckCommitState(%d)\n"),regionCount);
		CheckCommitState(regionCount);
		RDebug::Print(_L("Slave Process - Done\n"));
		return 0;
		}

	case ESlaveCreateChunk:
		{
		RDebug::Print(_L("Slave Process - Get parameters\n"));
		TInt createFlags;
		TInt commitType;
		TInt commitSize;
		CHECK(KErrNone,==,User::GetTIntParameter(3,createFlags));
		CHECK(KErrNone,==,User::GetTIntParameter(4,commitType));
		CHECK(KErrNone,==,User::GetTIntParameter(5,commitSize));

		RDebug::Print(_L("Slave Process - Create Chunk\n"));
		CHECK(KErrNone,==,Ldd.CreateChunk(createFlags));
		CHECK(KErrNone,==,Ldd.CommitMemory(commitType,commitSize));
		CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));
		TUint8* chunkBase=TheChunk.Base();
		memcpy(chunkBase,KTestString().Ptr(),KTestString().Size());

		RDebug::Print(_L("Slave Process - Connecting to test server\n"));
		RSession session;
		CHECK(KErrNone,==,session.CreateSession(KSecondProcessName,TVersion()));

		RDebug::Print(_L("Slave Process - Sending message\n"));
		TPtr8 ptr(chunkBase,commitSize,commitSize);
		session.SendReceive(0,TIpcArgs(&ptr));

		RDebug::Print(_L("Slave Process - Destroy Chunk\n"));
		TheChunk.Close();
		CHECK(1,==,Ldd.CloseChunk()); // 1==DObject::EObjectDeleted
		CHECK(1,==,Ldd.IsDestroyed());
		return 0;
		}

	default:
		RDebug::Print(_L("Slave Process - Bad Command\n"));
		return KErrArgument;
		}
	}

void TestChunkUserBase()
	{
	TUint ChunkAttribs = ChunkSize|ESingle|EOwnsMemory;

	test.Start(_L("Create chunk"));
	CHECK(KErrNone,==,Ldd.CreateChunk(ChunkAttribs));

	test.Next(_L("Open user handle"));
	CHECK(KErrNone,==,Ldd.GetChunkHandle(TheChunk));

	test.Next(_L("Commit some memory"));
	CHECK(KErrNone,==,Ldd.CommitMemory(EDiscontiguous|1*PageSize,PageSize));

	test.Next(_L("Check OpenSharedChunk with handle"));
	CHECK(KErrNone,==,Ldd.TestOpenHandle(TheChunk.Handle()));

	test.Next(_L("Get Kernel's user base"));
	TAny *kernelUserAddress;
	CHECK(KErrNone,==,Ldd.GetChunkUserBase(&kernelUserAddress));
	TAny *userAddress = TheChunk.Base();
	test(kernelUserAddress == userAddress);
	
	TheChunk.Close();
	CHECK(1,==,Ldd.CloseChunk());

	test.End();
	}


TInt E32Main()
	{
	// Running as slave?
	TInt slaveCommand;
	if(User::GetTIntParameter(1,slaveCommand)==KErrNone)
		return SlaveCommand((TSlaveCommand)slaveCommand);

// Turn off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Title();

	MemModelAttributes=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint mm=MemModelAttributes&EMemModelTypeMask;
#ifdef __T_SHAREDCHUNKF__
	if(mm!=EMemModelTypeMoving)
		{
		test.Start(_L("TESTS NOT RUN - Only valid on Moving Memory Model"));
		test.End();
		return 0;
		}
#endif

	test.Start(_L("Initialise"));
	CHECK(KErrNone,==,UserHal::PageSizeInBytes(PageSize));
	PhysicalCommitSupported = mm!=EMemModelTypeDirect && mm!=EMemModelTypeEmul;
	CachingAttributesSupported = mm!=EMemModelTypeDirect && mm!=EMemModelTypeEmul;


	test.Next(_L("Loading test driver"));
	TInt r = User::LoadLogicalDevice(KSharedChunkLddName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Opening channel"));
	CHECK(KErrNone,==,Ldd.Open());

	// now 'unload' test driver, however, it will remain loaded whilst
	// we still have a channel open with it... 
	User::FreeLogicalDevice(KSharedChunkLddName);

	test.Next(_L("Test chunk create"));
	TestCreate();

	test.Next(_L("Test handles"));
	TestHandles();

	test.Next(_L("Test handle ownership"));
	TestHandleOwnership();

	test.Next(_L("Test restrictions for multiply shared chunks"));
	TestRestrictions(EMultiple);
	test.Next(_L("Test restrictions for singly shared chunks"));
	TestRestrictions(ESingle);

	test.Next(_L("Test memory access for multiply shared chunks"));
	TestAccess(EMultiple|EOwnsMemory,EDiscontiguous);
	test.Next(_L("Test memory access for singly shared chunks"));
	TestAccess(ESingle|EOwnsMemory,EDiscontiguous);

	test.Next(_L("Test Discontiguous memory commit for multiply shared chunks"));
	TestCommit(EMultiple,EDiscontiguous);
	test.Next(_L("Test Discontiguous memory commit for singly shared chunks"));
	TestCommit(ESingle,EDiscontiguous);

	if((MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
		{
		test.Next(_L("Test Contiguous memory commit for multiply shared chunks"));
		TestCommit(EMultiple,EContiguous);
		test.Next(_L("Test Contiguous memory commit for singly shared chunks"));
		TestCommit(ESingle,EContiguous);
		}

	if(PhysicalCommitSupported)
		{
		test.Next(_L("Test Discontiguous Physical commit for multiply shared chunks"));
		TestCommit(EMultiple,EDiscontiguousPhysical);
		test.Next(_L("Test Discontiguous Physical commit for singly shared chunks"));
		TestCommit(ESingle,EDiscontiguousPhysical);

		test.Next(_L("Test Contiguous Physical commit for multiply shared chunks"));
		TestCommit(EMultiple,EContiguousPhysical);
		test.Next(_L("Test Contiguous Physical commit for singly shared chunks"));
		TestCommit(ESingle,EContiguousPhysical);
		}

	test.Next(_L("Test Kern::OpenSharedChunk for multiply shared chunks"));
	TestOpenSharedChunk(EMultiple,EDiscontiguous);
	test.Next(_L("Test Kern::OpenSharedChunk for singly shared chunks"));
	TestOpenSharedChunk(ESingle,EDiscontiguous);

	if(CachingAttributesSupported)
		{
		test.Next(_L("Test Mapping Attributes"));
		TestMappingAttributes();
		}
	
	test.Next(_L("Testing Kern::ChunkUserBase for shared chunks"));
	TestChunkUserBase();

	if (PhysicalCommitSupported)
		{
		test.Next(_L("Testing Kern::ChunkClose allows immediate freeing of physical ram"));
		test_KErrNone(Ldd.TestChunkCloseAndFree());
		}

	test.Next(_L("Close test driver"));
	Ldd.Close();

	test.End();

	return 0;
	}
