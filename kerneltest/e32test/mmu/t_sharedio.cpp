// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_sharedio.cpp
// Overview:
// Verifies the correct implementation of Shared IO Buffers
// API information:
// DSharedIoBuffer
// Details:
// 1. Loading the shared IO buffer test device driver
// 2. Create buffer of a specified size
// - it passes a request to the device driver to create a buffer
// - the driver creates a shared io buffer and it zero fills it
// - it checks the size is as specified
// 3. Map in buffer
// - it passes a request to the device driver to map the buffer created at
// step 1 into this user process
// - the driver maps the buffer into this user process
// - checks if UserToKernel and KernelToUser methods work as expected
// - fills a buffer
// - returns the user address and size to the user process
// - using the address and size returned by the driver, the user process
// checks the buffer is filled as expected
// 4. Fill and check shared buffer
// - user process fills the buffer and driver checks it
// - driver fills the buffer and user process checks it
// 5. Map Out Buffer
// - requests to the driver that the buffer should be unmapped from this
// process' address space
// - the driver checks that iUserAddress becomes NULL after unmapping
// 6. Destroy Buffer
// - requests to the driver to destroy the buffer
// 7. Create a buffer with a physical address
// (not performed on WINS)
// - requests to the driver to create a buffer by specifying a physical address
// - the driver allocates a physical address
// - creates a shared IO buffer over that physical address
// - fills the buffer with a pattern
// - destroys the buffer
// - creates a hardware chunk over the same physical address
// - checks the buffer contains the pattern
// - closes the chunk
// 8. Check using the same buffer by 2 different user processes
// (not performed on WINS)
// - process 1 maps a global buffer (the global buffer will be
// created in the context of process 1)
// - fills it
// - unmaps it
// - process 2 maps the global buffer
// - checks if it's filled accordingly
// - unmaps it
// - destroys the global buffer
// 9. Checking buffers are protected at context switching
// (not relevant on WINS)
// - creates a shared buffer and map it into this process
// - creates a new process
// - the new process tries to access the buffer mapped into the first process
// by zeroing the raw buffer passed from the first process. This relies on the
// fact that each shared buffer is created in the Home Section, so they will be
// available at the same address
// - tests if the new process was panicked due to access violation
// - tests if the contents of the buffer haven't been changed
// 10.Checking writing to unmapped buffer
// (not performed on WINS)
// - creates a new process
// - the new process creates a buffer, maps it and unmaps it
// - the new process tries to use the buffer after unmapping
// - the parent process logs the exit type and reason and checks
// these are EExitPanic and 3 (Kern 3 - access violation)
// 11.Checking address lookup is implemented
// (not relevant on WINS)
// - creates a new process
// - the new process will ask the device driver to read and write a descriptor
// in the old process, the kernel will perform an address lookup before
// reading or writing the descriptor, to make sure the address location does
// belong to the old process. The descriptor is a TPtr pointing to a buffer
// located in a shared io buffer.
// - device driver will return an error in case address lookup fails or ThreadRead
// or ThreadWrite fail
// - the new process returns the error code returned by the device driver
// - the old process tests for the exit code for the new process being KErrNone and
// that the thread is not panicked, and also that the values written and read
// are those expected.
// 12.Closing test driver
// - Trivial, but it will test if a created & mapped shared io buffer gets released
// successfully when the logical channel is closed (which in turn will delete the
// shared io buffer associated with the channel)
// The test comes in 4 flavours, in order to test memory protection when context switching
// between different types of processes.
// T_SHAREDIO:
// Main process is a moving process which creates another moving process.
// T_SHAREDIO2:
// Main process is a fixed process which creates a moving process.
// T_SHAREDIO3:
// Main process is a fixed process which creates another fixed process.
// T_SHAREDIO4:
// Main process is a moving process which creates a fixed process.
// Platforms/Drives/Compatibility:
// All (some steps will not be performed on emulator)
// Assumptions/Requirement/Pre-requisites:
// The test needs D_SHAREDIO.LDD, the device driver that actually operates the API. 
// Failures and causes:
// Failures of this test will indicate defects in the implementation of Shared Io Buffers.
// Base Port information:
// No?
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32math.h>
#include "d_sharedio.h"
#include <e32hal.h>
#include "u32std.h"
#include <u32hal.h>
#include <e32svr.h>
#include <f32dbg.h>
#include <e32def.h>
#include <e32def_private.h>

LOCAL_D RTest test(_L("T_SHAREDIO"));

const TInt KTestBufferSize = 0x100000;

TUint MemModelAttributes;
TBool PhysicalCommitSupported;

RTestLdd ldd;

TUint32 TestBufferSizes[]={0x1000, 0x10453, 0x100000, 0x100001, 0x203000, 0};

TInt checkBuffer(TAny* buffer, TUint32 aSize, TUint32 key)
	{
	TInt r=KErrNone;
	TUint8* m=(TUint8*)buffer;
	for(TUint32 size=0;size<aSize;size++,key+=5,m++)
		{
		if(*m!=(TUint8)(key%256))
			{
			r=KErrCorrupt;
			break;
			}
		}
	return r;
	}

TInt fillBuffer(TAny* buffer, TUint32 aSize, TUint32 key)
	{
	TUint8* m=(TUint8*)buffer;
	for(TUint32 size=0;size<aSize;size++,key+=5,m++)
		{
		*m=(TUint8)(key%256);
		}
	return KErrNone;
	}

TBool CheckBuffer(TAny* aBuffer,TInt aSize)
	{
	TAny** p = (TAny**)aBuffer;
	TAny** end = (TAny**)((TInt)p+aSize);
	while(p<end)
		{
		if(*p!=p)
			return EFalse;
		++p;
		}
	return ETrue;
	}

enum TTestProcessFunctions
	{
	ETestProcess1,
	ETestProcess2,
	ETestProcess3,
	};

class RTestProcess : public RProcess
	{
public:
	void Create(TTestProcessFunctions aFunction,TInt aArg1=-1,TInt aArg2=-1);
	};

void RTestProcess::Create(TTestProcessFunctions aFunction,TInt aArg1,TInt aArg2)
	{
	if(aArg1==-1)
		aArg1 = RProcess().Id();
	TBuf<512> commandLine;
	commandLine.Num((TInt)aFunction);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg1);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg2);
#ifdef __FIXED__
	//fixed process creating a moving process
	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_SHAREDIO.EXE");
	TInt r = RProcess::Create(filename,commandLine);
#else
#ifdef __SECOND_FIXED__
	//fixed process creating another fixed process
	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_SHAREDIO2.EXE");
	TInt r = RProcess::Create(filename,commandLine);
#else
#ifdef __MOVING_FIXED__
	//moving process creating a fixed process
	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_SHAREDIO2.EXE");
	TInt r = RProcess::Create(filename,commandLine);
#else
	//moving process creating a moving process
	TInt r = RProcess::Create(RProcess().FileName(),commandLine);
#endif
#endif
#endif
	test(r==KErrNone);
	SetJustInTime(EFalse);
	}

const TInt KProcessRendezvous = KRequestPending+1;

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;

	RTestLdd ldd;
	TInt r;
	r=User::LoadLogicalDevice(KSharedIoTestLddName);
	if(r!=KErrNone && r!=KErrAlreadyExists)
		return KErrGeneral;
	r=ldd.Open();
	if(r!=KErrNone)
		return r;

	switch(aTestNum)
		{
	case ETestProcess1:
		{
		TAny* gbuffer;
		TUint32 gsize;
		r=User::GetTIntParameter(1,(TInt&)gbuffer);
		if(r!=KErrNone)
			return r;
		r=User::GetTIntParameter(2,(TInt&)gsize);
		if(r!=KErrNone)
			return r;

		r=checkBuffer(gbuffer,gsize,23454);
		if(r!=KErrNone)
			return r;
		r=ldd.MapOutGlobalBuffer();
		if(r!=KErrNone)
			return r;

		r=ldd.CreateBuffer(KTestBufferSize);
		if(r!=KErrNone)
			return r;

		TAny* buffer;
		TUint32 size;
		r=ldd.MapInBuffer(&buffer,&size);
		if(r!=KErrNone)
			return r;

		if(!CheckBuffer(buffer,size))
			return KErrGeneral;

		r=ldd.MapOutBuffer();
		if(r!=KErrNone)
			return r;

		RProcess::Rendezvous(KProcessRendezvous);

		*(TInt*)buffer = 0;   // Should cause exception
		break;
		}
	case ETestProcess2:
		{
		TInt size=aArg2;
		TUint8* p=(TUint8*)aArg1;

		RProcess::Rendezvous(KProcessRendezvous);
		for(TInt i=0;i<size;i++)
			p[i]=0; // Should cause exception
		break;
		}
	case ETestProcess3:
		{
		TAny* buffer;
		TUint32 size;

		r=ldd.CreateBuffer(KTestBufferSize);
		if(r!=KErrNone)
			return r;

		r=ldd.MapInBuffer(&buffer,&size);
		if(r!=KErrNone)
			return r;

		if(!CheckBuffer(buffer,size))
			return KErrGeneral;

		*(TInt*)buffer=KMagic1;
		TPckg<TInt> buf(*(TInt*)buffer);
		r=ldd.ThreadRW(buf);
		if(r!=KErrNone)
			return r;

		if(*(TInt*)buffer!=KMagic2)
			return KErrCorrupt;

		r=ldd.ThreadRW(*(TDes8*)aArg1,aArg2);
		if(r!=KErrNone)
			return r;
		
		r=ldd.MapOutBuffer();
		if(r!=KErrNone)
			return r;

		break;
		}
	default:
		User::Panic(_L("T_SHAREDIO"),1);
		}

	ldd.Close();	
	return KErrNone;
	}

void CreateWithOOMCheck(TInt aSize, TBool aPhysicalAddress)
	{
	TInt failResult=KErrGeneral;

	for(TInt failCount=1; failCount<1000; failCount++)
		{
		test.Printf(_L("alloc fail count = %d\n"),failCount);

		User::__DbgSetAllocFail(ETrue,RAllocator::EFailNext,failCount);
		__KHEAP_MARK;
		
		if (aPhysicalAddress)
			failResult=ldd.CreateBufferPhysAddr(aSize);
		else
			failResult=ldd.CreateBuffer(aSize);

		if(failResult==KErrNone)
			break;

		test(failResult==KErrNoMemory);
		__KHEAP_MARKEND;
		}

	__KHEAP_RESET;

	test.Next(_L("Destroy buffer"));
	if (aPhysicalAddress)
		ldd.DestroyBufferPhysAddr();
	else
		ldd.DestroyBuffer();
	
	__KHEAP_MARKEND;
	}

GLDEF_C TInt E32Main()
    {
	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TInt function = -1;
		TInt arg1 = -1;
		TInt arg2 = -1;
		TLex lex(cmd);
		lex.Val(function);
		lex.SkipSpace();
		lex.Val(arg1);
		lex.SkipSpace();
		lex.Val(arg2);
		return DoTestProcess(function,arg1,arg2);
		}

	MemModelAttributes=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint mm=MemModelAttributes&EMemModelTypeMask;
	PhysicalCommitSupported = mm!=EMemModelTypeDirect && mm!=EMemModelTypeEmul;

// Turn off lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Title();

#if defined(__FIXED__) || defined(__SECOND_FIXED__) || defined(__MOVING_FIXED__)
	if(mm!=EMemModelTypeMoving)
		{
		test.Printf(_L("TESTS NOT RUN - Only applicable to moving memory model\r\n"));
		return 0;
		}
#endif

	test.Start(_L("Loading test driver..."));

	TInt r;
	r=User::LoadLogicalDevice(KSharedIoTestLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	r=User::LoadLogicalDevice(KSharedIoTestLddName);
	test(r==KErrAlreadyExists);
	r=ldd.Open();
	test(r==KErrNone);

	TAny* buffer;
	TUint32 size;
	TUint32 key;

	TInt testBufferSize=0;
	for(; TestBufferSizes[testBufferSize]!=0; ++testBufferSize)
		{
		test.Printf(_L("Test buffer size = %08x\n"),TestBufferSizes[testBufferSize]);

		test.Next(_L("Create buffer"));
		r=ldd.CreateBuffer(TestBufferSizes[testBufferSize]);
		if(r!=KErrNone)
			test.Printf(_L("Creating buffer failed client r=%d"), r);
		test(r==KErrNone);

		test.Next(_L("Map In Buffer"));
		r=ldd.MapInBuffer(&buffer,&size);
		
		test.Next(_L("CheckBuffer"));
		test(CheckBuffer(buffer,size));
		test(r==KErrNone);
		test.Next(_L("Fill and check shared buffer"));
		key=Math::Random();
		fillBuffer(buffer,size,key);
		test(ldd.CheckBuffer(key)==KErrNone);

		key=Math::Random();
		test(ldd.FillBuffer(key)==KErrNone);
		test(checkBuffer(buffer,size,key)==KErrNone);

		test.Next(_L("Map Out Buffer"));
		r=ldd.MapOutBuffer();
		test(r==KErrNone);

		test.Next(_L("Destroy Buffer"));
		r=ldd.DestroyBuffer();
		test(r==KErrNone);

		test.Next(_L("Create a buffer under OOM conditions"));
		CreateWithOOMCheck(TestBufferSizes[testBufferSize], EFalse);

		if(PhysicalCommitSupported)
			{
			test.Next(_L("Create a buffer with a physical address under OOM conditions"));
			CreateWithOOMCheck(TestBufferSizes[testBufferSize], ETrue);

			test.Next(_L("Create a buffer with a physical address"));
			r=ldd.CreateBufferPhysAddr(0x1000);
			test(r==KErrNone);

			test.Next(_L("Map In physical address Buffer"));
			r=ldd.MapInBuffer(&buffer,&size);
			test(r==KErrNone);

			test.Next(_L("Fill and check physical address shared buffer"));
			key=Math::Random();
			fillBuffer(buffer,size,key);
			test(ldd.CheckBuffer(key)==KErrNone);

			key=Math::Random();
			test(ldd.FillBuffer(key)==KErrNone);
			test(checkBuffer(buffer,size,key)==KErrNone);

			test.Next(_L("Map Out physical address Buffer"));
			r=ldd.MapOutBuffer();
			test(r==KErrNone);

			test.Next(_L("Destroy a buffer with a physical address"));
			r=ldd.DestroyBufferPhysAddr();
			test(r==KErrNone);
		}

		test.Next(_L("Check using the same buffer by 2 different user processes"));
		TAny* gbuffer;
		TUint32 gsize;
		r=ldd.MapInGlobalBuffer(RProcess().Id(),gbuffer,gsize);
		test(r==KErrNone);

		fillBuffer(gbuffer,gsize,23454);

		r=ldd.MapOutGlobalBuffer();
		test(r==KErrNone);

		r=ldd.CreateBuffer(TestBufferSizes[testBufferSize]);
		test(r==KErrNone);

		r=ldd.MapInBuffer(&buffer,&size);
		test(r==KErrNone);

		test(CheckBuffer(buffer,size));

		key=Math::Random();
		fillBuffer(buffer,size,key);
		test(ldd.CheckBuffer(key)==KErrNone);

		RTestProcess rogueP;
		TRequestStatus rendezvous;
		TRequestStatus logon;

		if(MemModelAttributes&EMemModelAttrProcessProt)
			{
			test.Next(_L("Checking buffers are protected at context switching"));
			rogueP.Create(ETestProcess2,(TInt)buffer,(TInt)size);
			rogueP.Logon(logon);
			rogueP.Rendezvous(rendezvous);
			rogueP.Resume();
			User::WaitForRequest(rendezvous);
			test(rendezvous==KProcessRendezvous);
			User::WaitForRequest(logon);
			test(rogueP.ExitType()==EExitPanic);
			test(logon==3);
			test(ldd.CheckBuffer(key)==KErrNone);
			}

		r=ldd.MapOutBuffer();
		test(r==KErrNone);

		r=ldd.DestroyBuffer();
		test(r==KErrNone);

		RTestProcess process;

		if((MemModelAttributes&EMemModelAttrKernProt) && (MemModelAttributes&EMemModelTypeMask)!=EMemModelTypeDirect)
			{
			test.Next(_L("Checking writing to unmapped buffer"));
			process.Create(ETestProcess1);
			process.Logon(logon);
			process.Rendezvous(rendezvous);
			test(ldd.MapInGlobalBuffer(process.Id(),gbuffer,gsize)==KErrNone);
			test(process.SetParameter(1,(TInt)gbuffer)==KErrNone);
			test(process.SetParameter(2,(TInt)gsize)==KErrNone);
			process.Resume();
			User::WaitForRequest(rendezvous);
			test(rendezvous==KProcessRendezvous);
			User::WaitForRequest(logon);
			test(process.ExitType()==EExitPanic);
			test(logon==3);
			process.Close();
			}

		r=ldd.CreateBuffer(TestBufferSizes[testBufferSize]);
		if(r!=KErrNone)
			return r;

		r=ldd.MapInBuffer(&buffer,&size);
		if(r!=KErrNone)
			return r;

		if(!CheckBuffer(buffer,size))
			return KErrGeneral;

		*(TInt*)buffer=KMagic1;
		TPckg<TInt> buf(*(TInt*)buffer);

		RTestProcess proc;
		test.Next(_L("Checking address lookup is implemented"));
		proc.Create(ETestProcess3,(TInt)&buf,RThread().Id());
		proc.Logon(logon);
		proc.Resume();
		User::WaitForRequest(logon);

		test(proc.ExitType()==EExitKill);
		test(logon==0);
		test(*(TInt*)buffer==KMagic2);

		ldd.DestroyBuffer();

		// Check process death whilst buffer is mapped in
		// Test case for defect DEF051851 - Shared IO Buffer fault when process dies
		test.Next(_L("Checking process death whilst buffer is mapped in"));
			process.Create(ETestProcess1);
			process.Logon(logon);
			test.Start(_L("Map buffer into another process"));
			test(ldd.MapInGlobalBuffer(process.Id(),gbuffer,gsize)==KErrNone);
			test.Next(_L("Kill other process"));
			process.Kill(99);
			User::WaitForRequest(logon);
			test(process.ExitType()==EExitKill);
			test(logon==99);
			process.Close();
			test.Next(_L("Map out buffer"));
			r=ldd.MapOutGlobalBuffer();
			test.Printf(_L("result = %d\n"),r);
			test(r==KErrNone);

			test.Next(_L("Map buffer into this process"));
			test(ldd.MapInGlobalBuffer(RProcess().Id(),gbuffer,gsize)==KErrNone);
			test.Next(_L("Map out buffer from this process"));
			r=ldd.MapOutGlobalBuffer();
			test.Printf(_L("result = %d\n"),r);
			test(r==KErrNone);

			process.Create(ETestProcess1);
			process.Logon(logon);
			test.Next(_L("Map buffer into another process"));
			test(ldd.MapInGlobalBuffer(process.Id(),gbuffer,gsize)==KErrNone);
			test.Next(_L("Kill other process"));
			process.Kill(99);
			User::WaitForRequest(logon);
			test(process.ExitType()==EExitKill);
			test(logon==99);
			process.Close();
			test.Next(_L("Map out buffer"));
			r=ldd.MapOutGlobalBuffer();
			test.Printf(_L("result = %d\n"),r);
			test(r==KErrNone);
			test.End();
	} // loop for next buffer size

	test.Next(_L("Create and map in buffer"));
	r=ldd.CreateBuffer(KTestBufferSize);
	test(r==KErrNone);
	r=ldd.MapInBuffer(&buffer,&size);
	test(r==KErrNone);

//  Test for DEF053512 - Can't delete SharedIo buffers in DLogicalDevice destructor 

	test.Next(_L("Map in global buffer"));
	TAny* gbuffer;
	TUint32 gsize;
	test(ldd.MapInGlobalBuffer(RProcess().Id(),gbuffer,gsize)==KErrNone);

	test.Next(_L("Closing channel (with a buffer still mapped in)"));
	ldd.Close();

//  Test for DEF053512 - Can't delete SharedIo buffers in DLogicalDevice destructor 

	test.Next(_L("Unload driver (whilst global buffer still mapped in)"));
	r=User::FreeLogicalDevice(KSharedIoTestLddName);
	test(r==KErrNone);

	test.End();



	return(0);
    }


