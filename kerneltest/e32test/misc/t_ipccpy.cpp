// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_ipccpy.cpp
// Overview:
// Test and benchmark IPC reading, writing, copying.	
// API Information:
// RBusLogicalChannel, DLogicalChannel.	
// Details:
// - Load the specified logical device driver, open a channel to it, allocate 
// a cell of specified size from the current thread's heap, get Kernel HAL
// memory model information.
// - Make a synchronous Kernel Executive type request to the logical channel 
// to write specified data to the buffer, read the data and calculate the 
// time taken for writing and reading the data. Benchmark the time required 
// to for 1000 64K user->kernel and kernel->user copies.
// - Create a chunk, get a pointer to the base of the chunk's reserved region,
// create a server thread, establish a session with the server, signal 
// completion of the client's request when message is received, read, 
// write specified bits and check it is as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "d_ipccpy.h"
#include "u32std.h"
#include <e32kpan.h>
#include "../mmu/mmudetect.h"
#include <hal.h>

RTest test(_L("T_IPCCPY"));
TUint8* Buffer;
TUint8* Disc;
RIpcCpy Ipccpy;
TUint32 MainId;
TUint8 Bss[4096];
TUint8* Kern;
TUint8* RamDrive;
TUint8* Nonexistent;
TUint8* Unaligned=Bss+1;
TInt CloseTime;
TLinAddr HwChunkAddr[RIpcCpy::ENumHwChunkTypes];
TPtr8 UserDes(Buffer+96,96,96);

void SetupAddresses()
	{
	Kern=KernData();
	TUint32 mm_attr=MemModelAttributes();
	TUint32 mm_type=mm_attr & EMemModelTypeMask;
	switch (mm_type)
		{
		case EMemModelTypeDirect:
			RamDrive=(TUint8*)0;	// not used anyway
			Nonexistent=(TUint8*)0xa8000000;
			break;
		case EMemModelTypeMoving:
			RamDrive=(TUint8*)0x40000000;
			Nonexistent=(TUint8*)0x60f00000;
			break;
		case EMemModelTypeMultiple:
			RamDrive=(TUint8*)0xa0000000;
			Nonexistent=(TUint8*)0xfe000000;
			break;
		case EMemModelTypeFlexible:
			RamDrive=(TUint8*)0;
			Nonexistent=(TUint8*)0x8ff00000;
			break;
		case EMemModelTypeEmul:
			RamDrive=(TUint8*)0;	// not used anyway
			Nonexistent=(TUint8*)0xf0000000;
			break;
		default:
			test(0);
			break;
		}
	new (&UserDes) TPtr8(Buffer+96,96,96);
	Ipccpy.HardwareChunks(HwChunkAddr,UserDes);
	test.Printf(_L("Buffer=%08x\n"),Buffer);
	test.Printf(_L("Bss=%08x\n"),Bss);
	test.Printf(_L("Kern=%08x\n"),Kern);
	test.Printf(_L("RamDrive=%08x\n"),RamDrive);
	test.Printf(_L("Nonexistent=%08x\n"),Nonexistent);
	test.Printf(_L("Unaligned=%08x\n"),Unaligned);
	test.Printf(_L("HwChunkSupRw=%08x\n"),HwChunkAddr[RIpcCpy::EHwChunkSupRw]);
	test.Printf(_L("HwChunkUserRw=%08x\n"),HwChunkAddr[RIpcCpy::EHwChunkUserRw]);
	test.Printf(_L("HwChunkUserRo=%08x\n"),HwChunkAddr[RIpcCpy::EHwChunkUserRo]);
	}

_LIT(KLitKernExec,"KERN-EXEC");

void TestEq(TInt a, TInt b, TInt l);
void Test(TBool c, TInt l);

#define TESTEQ(a,b)		TestEq((a),(b),__LINE__)
#define TEST(c)			Test((c),__LINE__)

void TestEq(TInt a, TInt b, TInt l)
	{
	if (a!=b)
		{
		if (TUint32(RThread().Id())==MainId)
			{
			test.Printf(_L("Line %d a=%d, b=%d\n"),l,a,b);
			test(0);
			}
		else
			User::Panic(_L("TESTEQ"),l);
		}
	}

void Test(TBool c, TInt l)
	{
	if (!c)
		{
		if (TUint32(RThread().Id())==MainId)
			{
			test.Printf(_L("Line %d FAIL\n"),l);
			test(0);
			}
		else
			User::Panic(_L("TEST"),l);
		}
	}

struct SIpcTestInfo
	{
	const TAny* iLocal;
	const TAny* iRemote;
	TInt iOffset;
	TInt iMode;			// bit 0 = 1 for 16 bit, bit 1 = 1 for write
	};

class RLocalSession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus* aStat)
		{return CreateSession(aSrv,TVersion(),-1,EIpcSession_Unsharable,0,aStat);}
	void Test(const TAny* aRemote)
		{Send(0,TIpcArgs((const TDesC8*)aRemote,(const TDesC16*)aRemote,(TDes8*)aRemote,(TDes16*)aRemote));}
	void Wait()
		{SendReceive(1);}
	};

RServer2 IpcServer;

TInt IpcTestFn(TAny* aInfo)
	{
	SIpcTestInfo& i=*(SIpcTestInfo*)aInfo;

	if (IpcServer.Handle())
		IpcServer.Close();
	
	TESTEQ(IpcServer.CreateGlobal(KNullDesC),KErrNone);
	RLocalSession sess;
	TRequestStatus stat;
	TESTEQ(sess.Connect(IpcServer,&stat),KErrNone);
	RMessage2 m;
	IpcServer.Receive(m);
	m.Complete(KErrNone);	// connect
	User::WaitForRequest(stat);	// connection message report
	sess.Test(i.iRemote);
	IpcServer.Receive(m);

	TInt r=KMinTInt;
	switch (i.iMode)
		{
		case 0:
			{	// read 8 bit
			TDesC8* pR=(TDesC8*)i.iRemote;
			TDes8* pL=(TDes8*)i.iLocal;
			r=m.Read(0,*pL,i.iOffset);
			if (r==KErrNone)
				{
				TESTEQ(pL->Length(),pR->Length()-i.iOffset);
				TEST(*pL==pR->Mid(i.iOffset));
				}
			break;
			}
		case 1:
			{	// read 16 bit
			TDesC16* pR=(TDesC16*)i.iRemote;
			TDes16* pL=(TDes16*)i.iLocal;
			r=m.Read(1,*pL,i.iOffset);
			if (r==KErrNone)
				{
				TESTEQ(pL->Length(),pR->Length()-i.iOffset);
				TEST(*pL==pR->Mid(i.iOffset));
				}
			break;
			}
		case 2:
			{	// write 8 bit
			TDes8* pR=(TDes8*)i.iRemote;
			TDesC8* pL=(TDesC8*)i.iLocal;
			r=m.Write(2,*pL,i.iOffset);
			if (r==KErrNone)
				{
				TESTEQ(pR->Length(),pL->Length()+i.iOffset);
				TEST(*pL==pR->Mid(i.iOffset));
				}
			break;
			}
		case 3:
			{	// write 16 bit
			TDes16* pR=(TDes16*)i.iRemote;
			TDesC16* pL=(TDesC16*)i.iLocal;
			r=m.Write(3,*pL,i.iOffset);
			if (r==KErrNone)
				{
				TESTEQ(pR->Length(),pL->Length()+i.iOffset);
				TEST(*pL==pR->Mid(i.iOffset));
				}
			break;
			}
		default:
			User::Panic(_L("MODE"),i.iMode);
		}
	m.Complete(0);
	sess.Close();
	IpcServer.Close();

	return r;
	}

void _DoIpcTest(const TAny* aLocal, const TAny* aRemote, TInt aOffset, TInt aMode, const TDesC* aPanicCat, TInt aResult, TInt aLine)
	{
	test.Printf(_L("Line %d\n"),aLine);
	SIpcTestInfo info;
	info.iLocal=aLocal;
	info.iRemote=aRemote;
	info.iOffset=aOffset;
	info.iMode=aMode;
	if (!aPanicCat)
		{
		// do test in this thread
		TInt r=IpcTestFn(&info);
		TESTEQ(r,aResult);
		return;
		}
	TBool jit=User::JustInTime();
	RThread t;
	TInt r=t.Create(KNullDesC(),IpcTestFn,0x2000,NULL,&info);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	User::SetJustInTime(EFalse);
	t.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(jit);
	test(t.ExitType()==EExitPanic);
	test(t.ExitCategory()==*aPanicCat);
	TESTEQ(t.ExitReason(),aResult);
	t.Close();
	}

void DoIpcTest(const TUint8* aLocal, const TUint8* aRemote, TInt aLength, TInt aMode, const TDesC* aPanicCat, TInt aResult, TInt aLine)
	{
	TPtr8 local((TUint8*)aLocal,aLength,aLength);
	TPtr8 remote((TUint8*)aRemote,aLength,aLength);
	_DoIpcTest(&local,&remote,0,aMode,aPanicCat,aResult,aLine);
	}

void DoIpcTest(const TUint8* aLocal, const TDesC8& aRemote, TInt aLength, TInt aMode, const TDesC* aPanicCat, TInt aResult, TInt aLine)
	{
	TPtr8 local((TUint8*)aLocal,aLength,aLength);
	_DoIpcTest(&local,&aRemote,0,aMode,aPanicCat,aResult,aLine);
	}

void TestIpcCopyErrors()
	{
	RChunk c;
	TInt r=c.CreateDisconnectedLocal(0,0,0x500000);
	test(r==KErrNone);
	r=c.Commit(0,0x1000);
	test(r==KErrNone);
	r=c.Commit(0x2000,0x1000);
	test(r==KErrNone);
	r=c.Commit(0x3ff000,0x1000);
	test(r==KErrNone);
	Disc=c.Base();
	test.Printf(_L("Disc=%08x\n"),Disc);
	DoIpcTest(Buffer,(const TUint8*)&TestEq,100,0,NULL,KErrNone,__LINE__);
	DoIpcTest(Buffer,(const TUint8*)&TestEq,100,2,NULL,KErrBadDescriptor,__LINE__);
	DoIpcTest((const TUint8*)&TestEq,Buffer,100,2,NULL,KErrNone,__LINE__);
	DoIpcTest((const TUint8*)&TestEq,Buffer,100,0,&KLitKernExec,ECausedException,__LINE__);
	DoIpcTest(Buffer,Nonexistent,100,0,NULL,KErrBadDescriptor,__LINE__);
	DoIpcTest(Buffer,Nonexistent,100,2,NULL,KErrBadDescriptor,__LINE__);
	DoIpcTest(Nonexistent,Buffer,100,2,&KLitKernExec,ECausedException,__LINE__);
	DoIpcTest(Nonexistent,Buffer,100,0,&KLitKernExec,ECausedException,__LINE__);
	DoIpcTest(Buffer,Unaligned,100,0,NULL,KErrNone,__LINE__);
	DoIpcTest(Buffer,Unaligned,100,2,NULL,KErrNone,__LINE__);
	DoIpcTest(Unaligned,Buffer,100,2,NULL,KErrNone,__LINE__);
	DoIpcTest(Unaligned,Buffer,100,0,NULL,KErrNone,__LINE__);

	DoIpcTest(Disc+4001,Buffer,95,0,NULL,KErrNone,__LINE__);
	if (HaveVirtMem())
		DoIpcTest(Disc+4001,Buffer,96,0,&KLitKernExec,ECausedException,__LINE__);
	DoIpcTest(Buffer,Disc+4001,95,0,NULL,KErrNone,__LINE__);
	if (HaveVirtMem())
		DoIpcTest(Buffer,Disc+4001,96,0,NULL,KErrBadDescriptor,__LINE__);

	TPtr8* pdes;
	if (HaveVirtMem())
		{
		// test descriptor stored stradling chunk end...
		pdes = (TPtr8*)(Disc+0x3ffff4);
		memcpy(pdes,&UserDes,12);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrNone,__LINE__);
		pdes = (TPtr8*)(Disc+0x3ffff8);
		memcpy(pdes,&UserDes,8);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrBadDescriptor,__LINE__);
		pdes = (TPtr8*)(Disc+0x3ffffc);
		memcpy(pdes,&UserDes,4);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrBadDescriptor,__LINE__);
		r=c.Commit(0x400000,0x1000);
		test(r==KErrNone);
		pdes = (TPtr8*)(Disc+0x3ffff4);
		memcpy(pdes,&UserDes,12);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrNone,__LINE__);
		pdes = (TPtr8*)(Disc+0x3ffff8);
		memcpy(pdes,&UserDes,12);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrNone,__LINE__);
		pdes = (TPtr8*)(Disc+0x3ffffc);
		memcpy(pdes,&UserDes,12);
		DoIpcTest(Buffer,*pdes,pdes->Size(),0,NULL,KErrNone,__LINE__);
		}

	if (HaveMultAddr())
		{
		if(RamDrive)
			{
			DoIpcTest(Disc+0x100000,Buffer,96,0,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,Disc+0x100000,96,0,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(RamDrive,Buffer,4,0,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,RamDrive,4,0,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(RamDrive,Buffer,4,2,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,RamDrive,4,2,NULL,KErrBadDescriptor,__LINE__);
			}

		// if memory alising happens during IPC then the memory at 'Disc' would be aliased
		// at KIPCAliasAddress and so would not be protected by MMU permission checks.
		// However, the kernel should still prevent this, to avoid degrading process
		// protection for memory in other parts of the alias region.
#ifdef __CPU_X86
		const TUint8* KIPCAliasAddress;
		if((MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeFlexible)
			KIPCAliasAddress = (TUint8*)0x7e000000;
		else
			KIPCAliasAddress = (TUint8*)0xc0400000;
#else
		const TUint8* KIPCAliasAddress = (TUint8*)0x00200000;
#endif
		DoIpcTest(KIPCAliasAddress,Disc,4,0,&KLitKernExec,ECausedException,__LINE__);
		DoIpcTest(Disc,KIPCAliasAddress,4,0,NULL,KErrBadDescriptor,__LINE__);
		DoIpcTest(KIPCAliasAddress,Disc,4,2,&KLitKernExec,ECausedException,__LINE__);
		DoIpcTest(Disc,KIPCAliasAddress,4,2,NULL,KErrBadDescriptor,__LINE__);
		}

	if (HaveIPCKernProt())
		{
		DoIpcTest(Kern,Buffer,96,0,&KLitKernExec,ECausedException,__LINE__);
		DoIpcTest(Buffer,Kern,96,0,NULL,KErrBadDescriptor,__LINE__);
		TUint8* addrRW = (TUint8*)HwChunkAddr[RIpcCpy::EHwChunkSupRw];
		if(addrRW)
			{
			DoIpcTest(Buffer,*(TDes8*)addrRW,96,0,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(Buffer,*(TDes8*)addrRW,96,2,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(addrRW+96,Buffer,96,0,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,addrRW,96,0,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(addrRW+96,Buffer,96,2,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,addrRW,96,2,NULL,KErrBadDescriptor,__LINE__);
			}
		}

	if((MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeMultiple
		|| (MemModelAttributes()&EMemModelTypeMask) == EMemModelTypeFlexible
		)
		{
		// On multiple memory model, test IPC to Hardware Chunks.
		// IPC to hardware chunks not supported on Moving Memory
		TUint8* addrRW = (TUint8*)HwChunkAddr[RIpcCpy::EHwChunkUserRw];
		if(addrRW)
			{
			DoIpcTest(Buffer,*(TDes8*)addrRW,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(Buffer,*(TDes8*)addrRW,96,2,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRW+96,Buffer,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(Buffer,addrRW,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRW+96,Buffer,96,2,NULL,KErrNone,__LINE__);
			DoIpcTest(Buffer,addrRW,96,2,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRW+96,addrRW,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRW+96,addrRW,96,2,NULL,KErrNone,__LINE__);
			}
		TUint8* addrRO = (TUint8*)HwChunkAddr[RIpcCpy::EHwChunkUserRo];
		if(addrRO && HaveWriteProt())
			{
			DoIpcTest(Buffer,*(TDes8*)addrRO,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(Buffer,*(TDes8*)addrRO,96,2,&KLitKernExec,EBadIpcDescriptor,__LINE__);
			DoIpcTest(addrRO+96,Buffer,96,0,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(Buffer,addrRO,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRO+96,Buffer,96,2,NULL,KErrNone,__LINE__);
			DoIpcTest(Buffer,addrRO,96,2,NULL,KErrBadDescriptor,__LINE__);
			DoIpcTest(addrRW+96,addrRO,96,0,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRW+96,addrRW,96,2,NULL,KErrNone,__LINE__);
			DoIpcTest(addrRO+96,addrRO,96,0,&KLitKernExec,ECausedException,__LINE__);
			DoIpcTest(addrRO+96,addrRW,96,2,NULL,KErrNone,__LINE__);
			}
		}

	c.Close();
	}

RMessage2 Msg1, Msg2;

TInt SendAndExit(TAny* aPtr)
	{
	RLocalSession sess;
	TInt r=sess.Connect(IpcServer,NULL);
	if (r!=KErrNone)
		return r;
	sess.Test(aPtr);
	sess.Wait();
	sess.Close();
	User::AfterHighRes(1000*CloseTime);
	Msg1.Complete(0);		// complete my own message! - this removes message reference to thread
	return 0;
	}

void TestIpcAsyncClose()
	{

	// Create a 16MB chunk
	const TInt desSize = 8*1024*1024;
	RChunk chunk;
	test(chunk.CreateLocal(2 * desSize, 2 * desSize) == KErrNone);
	test(chunk.Adjust(2 * desSize) == KErrNone);

	TUint8* bigBuf=chunk.Base();
	test(bigBuf!=NULL);
	TUint8* bigBuf2=chunk.Base() + desSize;
	test(bigBuf2!=NULL);
	TPtr8 bigBufPtr(bigBuf, desSize, desSize);
	TPtr8 bigBufPtr2(bigBuf2, 0, desSize);

	if (IpcServer.Handle())
		IpcServer.Close();
	TESTEQ(IpcServer.CreateGlobal(KNullDesC),KErrNone);

	RThread t;
	TInt r=t.Create(KNullDesC,SendAndExit,0x1000,NULL,&bigBufPtr);
	test(r==KErrNone);
	TFullName fn(t.FullName());
	TRequestStatus s;
	t.Logon(s);
	t.SetPriority(EPriorityMuchMore);
	t.Resume();

	IpcServer.Receive(Msg1);	// connect
	Msg1.Complete(KErrNone);
	IpcServer.Receive(Msg1);	// test message
	IpcServer.Receive(Msg2);	// wait/synch message
	TUint32 initial = User::NTickCount();
	r=Msg1.Read(2,bigBufPtr2,0);	// arg2 is writable 8 bit descriptor
	TUint32 final = User::NTickCount();
	TUint32 elapsed = final - initial;
	if (elapsed<3)
		test.Printf(_L("*** WARNING! The big IPC only took %dms, which means the next test might fail! \n"),elapsed);
	else
		test.Printf(_L("Big IPC took %dms\n"),elapsed);
	CloseTime = (TInt)(elapsed>>2);
	Msg2.Complete(0);
	IpcServer.Receive(Msg2);	// disconnect
	TUint32 disconnect = User::NTickCount();
	
	// We expect this IPC read to fail part way through
	r=Msg1.Read(2,bigBufPtr2,0);	// arg2 is writable 8 bit descriptor
	test.Printf(_L("counters: initial=%d final=%d disconnect=%d current=%d\n"),initial,final,disconnect,User::NTickCount());
	test.Printf(_L("2nd Big IPC returned %d\n"),r);
	test(r==KErrDied);
	test(Msg1.IsNull());
	Msg2.Complete(0);		// complete session closure as well
	User::WaitForRequest(s);
	test(s==KErrNone);
	CLOSE_AND_WAIT(t);
	test(t.Open(fn)==KErrNotFound);
	IpcServer.Close();

	// t already closed
//	User::Free(bigBuf);
//	User::Free(bigBuf2);
	chunk.Close();
	}

void BenchmarkTest()
	{
	TAny* bigbuf = User::Alloc(65536);
	test(bigbuf != NULL);
	TInt i;
	TUint32 initial, final;
	initial = User::NTickCount();
	for (i=0; i<1000; ++i)
		Ipccpy.BigWrite(bigbuf, 0);
	final = User::NTickCount();
	TUint32 wcal = final - initial;
	initial = User::NTickCount();
	for (i=0; i<1000; ++i)
		Ipccpy.BigWrite(bigbuf, 65536);
	final = User::NTickCount();
	TUint32 write = final - initial;
	test.Printf(_L("64K user->kernel copy takes %d us\n"), write - wcal);
	initial = User::NTickCount();
	for (i=0; i<1000; ++i)
		Ipccpy.BigRead(bigbuf, 0);
	final = User::NTickCount();
	TUint32 rcal = final - initial;
	initial = User::NTickCount();
	for (i=0; i<1000; ++i)
		Ipccpy.BigRead(bigbuf, 65536);
	final = User::NTickCount();
	TUint32 read = final - initial;
	test.Printf(_L("64K kernel->user copy takes %d us\n"), read - rcal);
	User::Free(bigbuf);
//	User::After(10*1000*1000);
	}


RMessage2 IpcMesage;
const TInt KTestChunkSize = 1024*1024;
const TInt KReadSize = 4096;

TInt IpcMultipleAliasesThread(TAny* aBuffer)
	{
	TBuf8<KReadSize> data;
	TAny** dataStart = (TAny**)data.Ptr();
	TAny** dataEnd = (TAny**)(data.Ptr()+KReadSize-sizeof(TAny*));
	for(;;)
		{
		TInt offset;
		for(offset=0; offset<KTestChunkSize; offset+=KReadSize)
			{
			TInt r = IpcMesage.Read(0,data,offset);
			if(r!=KErrNone)
				return r;
			if(data.Size()!=KReadSize)
				return 1;
			TAny* expected = (TAny*)((TInt)aBuffer+offset);
			if(*dataStart != expected)
				{
				RDebug::Printf("Offset=%x, expected %x but read %x",offset,expected,*dataStart);
				return 2;
				}
			expected = (TAny*)((TInt)aBuffer+offset+KReadSize-sizeof(TAny*));
			if(*dataEnd != expected)
				{
				RDebug::Printf("Offset=%x, expected %x but read %x",offset,expected,*dataEnd);
				return 3;
				}
			}
		}
	}

/*
This tests exercises the situation where multiple threads are doing IPC simultaneousely.
On the Multiple Memory Model, this aims to test the per-thread memory aliasing code.
(DMemModelThread::Alias and company)
*/
void TestIpcMultipleThreads()
	{
	test.Start(_L("Test Multiple Threads IPC"));

	// create chunk for threads to do IPC from...
	RChunk chunk;
	TESTEQ(chunk.CreateLocal(KTestChunkSize,KTestChunkSize),KErrNone);
	TAny** buffer = (TAny**)chunk.Base();
	TAny** bufferEnd = (TAny**)((TInt)buffer+KTestChunkSize);
	for(; buffer<bufferEnd; ++buffer)
		*buffer=buffer;

	// create a server message which test threads can use to do IPC memory operations
	if (IpcServer.Handle())
		IpcServer.Close();	
	TESTEQ(IpcServer.CreateGlobal(KNullDesC),KErrNone);
	RLocalSession sess;
	TRequestStatus stat;
	TESTEQ(sess.Connect(IpcServer,&stat),KErrNone);
	RMessage2 m;
	IpcServer.Receive(m);
	m.Complete(KErrNone);	// connect
	User::WaitForRequest(stat);	// connection message report
	TAny* ptrMem = User::Alloc(0x2000);
	TPtr8* pptr = (TPtr8*)(((TInt)ptrMem&~0xfff)+0x1000-sizeof(TInt));
	new (pptr) TPtr8(chunk.Base(),KTestChunkSize,KTestChunkSize); // create a TPtr8 which straddles a page boundary
	sess.Test(pptr);
	IpcServer.Receive(IpcMesage);

	// create some test threads...
	const TInt KNumIpcThreads = 10;
	RThread threads[KNumIpcThreads];
	TRequestStatus stats[KNumIpcThreads];
	TInt i;
	for(i=0; i<KNumIpcThreads; i++)
		{
		TESTEQ(threads[i].Create(KNullDesC,IpcMultipleAliasesThread,KReadSize+0x1000,&User::Allocator(),chunk.Base()),KErrNone);
		threads[i].Logon(stats[i]);
		}
	test.Printf(_L("Resuming threads...\n"));
	for(i=0; i<KNumIpcThreads; i++)
		threads[i].Resume();

	User::After(10*1000000);
	for(i=0; i<KNumIpcThreads; i++)
		{
		test(stats[i]==KRequestPending); // theads should still be running
		}

	// close chunk whilst test threads are still doing IPC...
	test.Printf(_L("Closing chunk...\n"));
	chunk.Close();
	for(i=0; i<KNumIpcThreads; i++)
		{
		User::WaitForRequest(stats[i]);
		TInt r=stats[i].Int();
		test.Printf(_L("Thread %d result = %d\n"),i,r);
		test(r==KErrBadDescriptor);
		}

	IpcServer.Close();
	User::Free(ptrMem);
	test.End();
	}

GLDEF_C TInt E32Main()
	{
	MainId=TUint32(RThread().Id());
//	RThread().SetPriority(EPriorityAbsoluteForeground);
	test.Title();
	test.Start(_L("Load LDD"));
	TInt r=User::LoadLogicalDevice(_L("D_IPCCPY"));
	test(r==KErrNone || r==KErrAlreadyExists);
	test.Next(_L("Open channel"));
	r=Ipccpy.Open();
	test(r==KErrNone);
	test.Next(_L("Allocate heap buffer"));
	Buffer=(TUint8*)User::Alloc(4096);
	test(Buffer!=NULL);
	SetupAddresses();

	BenchmarkTest();

	TestIpcCopyErrors();
	TestIpcAsyncClose();
	TestIpcMultipleThreads();

	FOREVER
		{
		TRequestStatus s;
		Mem::Fill(Buffer,272,0xcd);
		TPtr8 ptr(Buffer,0,272);
		Ipccpy.IpcCpy(s,ptr);
		User::WaitForRequest(s);
		TInt x=s.Int();
		if (x<0)
			{
			test.Printf(_L("Error %d\n"),x);
			test(0);
			}
		TInt src_offset=x&3;
		TInt dest_offset=(x>>2)&3;
		TInt length=(x>>4)+1;
		TInt err=-1;
		TInt i;
		for (i=0; i<dest_offset && err<0; ++i)
			{
			if (Buffer[i]!=0xcd)
				err=i;
			}
		TUint8 v=(TUint8)src_offset;
		for (i=0; i<length && err<0; ++i)
			{
			++v;
			if (Buffer[i+dest_offset]!=v)
				err=i+dest_offset;
			}
		for (i=dest_offset+length; i<272 && err<0; ++i)
			{
			if (Buffer[i]!=0xcd)
				err=i;
			}
		if (err>=0)
			{
			test.Printf(_L("Sequence number %03x\nSrcOffset %d, DestOffset %d, Length %d\n"),x,src_offset,dest_offset,length);
			test.Printf(_L("First error at %d"),err);
			for (i=0; i<272; i+=16)
				{
				TInt j;
				test.Printf(_L("%03x:"),i);
				for (j=0; j<16; ++j)
					{
					test.Printf(_L(" %02x"),Buffer[i+j]);
					}
				}
			test(0);
			}
		if (x==4095)
			break;
		}
	Ipccpy.Close();
	test.End();
	return KErrNone;
	}
