// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_prot.cpp
// Overview:
// Tests that the kernel panics the user rather than dying in 
// various situations
// API Information:
// N/A
// Details:
// - Verify that printing a long string does not panic.
// - Verify that a stack overflow in a thread causes a panic.
// - Create the thread which panics a dead thread, verify that 
// it isn't panicked for doing this.
// - Create the thread which panics a bad handle, verify that 
// it isn't panicked for doing this.
// - Verify that an RSession send from an uninitialised handle
// causes a panic.
// - Verify that the thread causing an exception, for a variety
// of reasons, is panicked.
// - Verify thread writing to kernel data is panicked.
// - Verify that RAM disk access either causes a panic or is
// denied, based on the platform security settings.
// - Verify that an RThread::RequestComplete() with an invalid
// address causes a panic. Verify results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "u32std.h"
#include <e32panic.h>
#include "../mmu/mmudetect.h"

void DoUndefinedInstruction();

const TInt KHeapSize=0x200;

_LIT(KLitKernExec,"KERN-EXEC");

class RSessionTest : public RSessionBase
	{
public:
	void DoSend();
	};


void RSessionTest::DoSend()
	{
    Send(34,TIpcArgs(78));
	//Send(34,(TAny*)78);
	};

LOCAL_D RTest test(_L("T_PROT"));
LOCAL_D RTest t(_L("T_PROT thread"));

enum TAction
	{
	EDataAbort=0,
	EPrefetchAbort=1,
	EUndefinedInstruction=2,
	};

typedef void (*PFV)(void);

#ifndef __MARM__
void DoUndefinedInstruction()
	{
#ifdef __GCC32__	
	asm("int 6");
#else
	_asm int 6;
#endif
	}
#endif

LOCAL_C TInt ExceptionThread(TAny* anAction)
	{
	TAction action=(TAction)((TInt) anAction);
	switch (action)
		{
		case EDataAbort:
			*(TInt*)0=0;
			break;
		case EPrefetchAbort:
			{
//			PFV f=(PFV)NULL;
			PFV f=(PFV)0x80000;	// don't use NULL since it is readable on Snowdrop
			(*f)();
			break;
			}
		case EUndefinedInstruction:
			DoUndefinedInstruction();
			break;
		};
	return 0;
	}

LOCAL_C void RunTestThread(TAction anAction)
	{
	RThread t;
	TInt r=t.Create(_L("TestThread"),ExceptionThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)anAction);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	test(s==KRequestPending);
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType()==EExitPanic);
	test(t.ExitCategory()==_L("KERN-EXEC"));
	test(t.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(t);
	}

LOCAL_C TInt UnintThread(TAny* )
	{

	RSessionTest rsb;
	rsb.DoSend();
	FOREVER
		;
	}

void testSession()
//
// Test 1
//
	{
	
	RThread thread;
	TRequestStatus stat;

	test.Next(_L("Create UnintThread"));
	TInt r=thread.Create(_L("UnintThread"),UnintThread,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);

	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume UnintThread"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check UnintThread panicked"));
	test(thread.ExitCategory()==_L("KERN-EXEC"));
//	test(thread.ExitReason()==EBadHandle);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	}

TInt dummy(TAny*)
	{
	return(KErrNone);
	}

// Dennis - modified this to panic a dead thread rather than a bad handle
TInt pdThread(TAny*)
	{
	RThread thread;
	TRequestStatus stat;
	TInt r=thread.Create(_L("dummy"),dummy,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);
	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	thread.Resume();
	User::WaitForRequest(stat);
	test(thread.ExitType()==EExitKill);
	test(stat.Int()==KErrNone);
	thread.Panic(_L("MYPANIC"),0x666); // this shouldn't panic pdThread
	test(thread.ExitType()==EExitKill);
	test(thread.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(thread);
	return(KErrNone);
	}

void testPanicDeadThread()
//
// Dennis - modified this to panic a dead thread rather than a bad handle
// Create the thread which panics a dead thread /*bad handle*/
// Check that it isn't panicked for doing this
//
	{
	RThread thread;
	TRequestStatus stat;
	test.Next(_L("Create PanicDeadThread"));
	TInt r=thread.Create(_L("PanicDeadThread"),pdThread,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);
	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume PanicDeadThread"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check PanicDeadThread did not panic"));
	test(thread.ExitReason()==KErrNone);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(thread);
	}

TInt doDeadThreadStuff(TAny*)
	{
	RThread thread;
	TRequestStatus stat;
	TInt r=thread.Create(_L("dummy2"),dummy,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);
	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	thread.Resume();
	User::WaitForRequest(stat);

	thread.SetPriority(EPriorityNormal);

	CLOSE_AND_WAIT(thread);
	return(KErrNone);
	}

void testDeadThread()
//
// Create the thread which panics a bad handle
// Check that it isn't panicked for doing this
//
	{
	RThread thread;
	TRequestStatus stat;
	test.Next(_L("Create doDeadThreadStuff"));
	TInt r=thread.Create(_L("doDeadThreadStuff"),doDeadThreadStuff,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);
	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume doDeadThreadStuff"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check doDeadThreadStuff did not panic"));
	test(thread.ExitReason()==KErrNone);
	test(thread.ExitType()==EExitKill);
	test(thread.ExitCategory()==_L("Kill"));
	CLOSE_AND_WAIT(thread);
	}

TInt MinimalThread(TAny*)
//
// Minimal thread, used in test 5
//
	{
	return(KErrNone);
	}

LOCAL_C TInt ExceptThread(TAny* )
	{

	TUint* nullPtr=0;
	*nullPtr=0xdead; // BANG!!
	return KErrNone;
	}


void testExcept()
//
// Test thread causing exception is panicked
//
	{
	
	RThread thread;
	TRequestStatus stat;

	test.Next(_L("Create ExceptThread"));
	TInt r=thread.Create(_L("ExceptThread"),ExceptThread,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);

	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume ExceptThread"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check ExceptThread panicked"));
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ECausedException);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	}

LOCAL_C TInt StackThread(TAny* )
	{

	TFileName heresAnotherOne;
	StackThread((TAny*)heresAnotherOne.Ptr()); // go recursive
	return KErrNone;
	}

void testStackOverflow()
//
// Thread overflowing its stack is panicked
//
	{

	RThread thread;
	TRequestStatus stat;

	test.Next(_L("Create StackThread"));
	TInt r=thread.Create(_L("StackThread"),StackThread,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);

	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume StackThread"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check StackThread panicked"));
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ECausedException);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	}

LOCAL_C TInt KernWriter(TAny* )
	{

	TUint* kernPtr=(TUint*)KernData();
	*kernPtr=0xdead; // BANG!!
	return KErrNone;
	}

LOCAL_C TInt RamDiskWriter(TAny* )
	{

	TFindChunk fChunk(_L("*TheRamDriveChunk*"));
	TFullName n;
	TInt r=fChunk.Next(n);
	RChunk ch;
	r=ch.Open(fChunk);
	if(r!=KErrNone)
		return r;
	TUint8* rdPtr=ch.Base();
	*rdPtr=0xaa; // BANG!!
	return KErrNone;
	}


void testKernelWriter()
//
// Thread writing to kernel data is panicked
//
	{

	RThread thread;
	TRequestStatus stat;

	test.Next(_L("Create KernWriter"));
	TInt r=thread.Create(_L("KernWriter"),KernWriter,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);

	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume KernWriter"));
	thread.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Check KernWriter panicked"));
	test(thread.ExitCategory()==_L("KERN-EXEC"));
	test(thread.ExitReason()==ECausedException);
	test(thread.ExitType()==EExitPanic);
	CLOSE_AND_WAIT(thread);
	}

void testRamDiskAccess()
	{

	RThread thread;
	TRequestStatus stat;

	test.Next(_L("Create RamDiskWriter"));
	TInt r=thread.Create(_L("RamDiskWriter"),RamDiskWriter,KDefaultStackSize,KHeapSize,KHeapSize,0);
	test(r==KErrNone);

	thread.Logon(stat);
	test(thread.ExitType()==EExitPending);
	test.Next(_L("Resume RamDiskWriter"));
	thread.Resume();
	User::WaitForRequest(stat);
	if((!PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation))||(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)))
		{
		test.Next(_L("Check RamDiskWriter panicked"));
		test(thread.ExitCategory()==_L("KERN-EXEC"));
		test(thread.ExitReason()==ECausedException);
		test(thread.ExitType()==EExitPanic);
		}
	else
		{
		test.Next(_L("Check RamDiskWriter was refused access"));
		test(thread.ExitReason()==KErrPermissionDenied);
		test(thread.ExitType()==EExitKill);
		}
	CLOSE_AND_WAIT(thread);
	}

RThread MainThread;

TInt RequestCompleteThread(TAny* aPtr)
	{
	TRequestStatus** pS=(TRequestStatus**)aPtr;
	MainThread.RequestComplete(*pS,123);
	return 0;
	}

_LIT(KReqCompThreadName,"ReqCompThread");
void StartRequestCompleteThread(RThread& aThread, TRequestStatus** aStatus)
	{
	TInt r=aThread.Create(KReqCompThreadName,RequestCompleteThread,0x1000,0x1000,0x10000,aStatus);
	test (r==KErrNone);
	aThread.SetPriority(EPriorityMore);
	TRequestStatus s;
	aThread.Logon(s);
	aThread.Resume();
	User::WaitForRequest(s);
	}

GLDEF_C TInt E32Main()
//
// Main
//
	{	

	// don't want just in time debugging as we trap panics
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 

	test.Title();
	test.Start(_L("Test protection & panicking"));

#if defined(__EPOC32__)
	// this next test doesn't work under WINS because the string needs to be converted
	// from UNICODE to ascii for printing to STDOUT, and that requires the string to be
	// zero-terminated which panics because the string is too long.
	test.Next(_L("Printing long string doesn't get me shot"));
	test.Printf(_L("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"));
	test.Printf(_L("\n"));
#endif

	if (HaveVirtMem())
		{
		test.Next(_L("Stack overflow shoots the thread not the kernel"));
		testStackOverflow();
		}

	test.Next(_L("Panicking a closed thread doesn't panic the panicker!!"));
	testPanicDeadThread();

	test.Next(_L("Dead thread tests"));
	testDeadThread();

	test.Next(_L("RSession send from uninitialised handle"));
	testSession();

	test.Next(_L("Thread causing exception is killed"));
	if (HaveVirtMem())
		{
		testExcept();
		RunTestThread(EDataAbort);
		RunTestThread(EPrefetchAbort);
		}
#ifndef __WINS__
	RunTestThread(EUndefinedInstruction);
#endif

#if defined(__EPOC32__)
	if (HaveDirectKernProt())
		{
		test.Next(_L("Thread writing to kernel data is killed"));
		testKernelWriter();
		}

	if (HaveProcessProt())
		{
		test.Next(_L("Check access to RamDisk is denied"));
		testRamDiskAccess();
		}

	if (HaveMMU())
		{
		test.Next(_L("RequestComplete() with bad address"));
		TInt rqc=RThread().RequestCount();
		test(MainThread.Duplicate(RThread())==KErrNone);
		RThread t;
		TRequestStatus s=KRequestPending;
		TRequestStatus* pS=&s;
		StartRequestCompleteThread(t,&pS);
		test(t.ExitType()==EExitKill);
		test(t.ExitReason()==KErrNone);
		CLOSE_AND_WAIT(t);
		test(s==123);
		test(pS==NULL);
		test(RThread().RequestCount()==rqc+1);
		TRequestStatus** bad=(TRequestStatus**)0x80000000;	// kernel space
		StartRequestCompleteThread(t,bad);
		test(t.ExitType()==EExitPanic);
		test(t.ExitReason()==ECausedException);
		test(t.ExitCategory()==KLitKernExec);
		CLOSE_AND_WAIT(t);
		test(RThread().RequestCount()==rqc+1);
		pS=(TRequestStatus*)0x80000000;
		StartRequestCompleteThread(t,&pS);
		// Request status poked user side, so we expect a KERN-EXEC 3...
		test(t.ExitType()==EExitPanic);
		test(t.ExitReason()==ECausedException);
		test(t.ExitCategory()==KLitKernExec);
		CLOSE_AND_WAIT(t);
		test(pS==NULL);
		test(RThread().RequestCount()==rqc+1);
		pS=(TRequestStatus*)(((TUint8*)&s)+0x80000);		// aligned, within chunk max size but invalid
		StartRequestCompleteThread(t,&pS);
		// Request status poked user side, so we expect a KERN-EXEC 3...
		test(t.ExitType()==EExitPanic);
		test(t.ExitReason()==ECausedException);
		test(t.ExitCategory()==KLitKernExec);
		CLOSE_AND_WAIT(t);
		test(pS==NULL);
		test(RThread().RequestCount()==rqc+1);
		}
#endif

	test.End();

	User::SetJustInTime(justInTime);
	return(KErrNone);
	}

