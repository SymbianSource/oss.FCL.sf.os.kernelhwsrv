// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_emul.cpp
// Overview:
// Test the mechanism to escape threads from the emulator in order
// to block on Windows objects
// Test launching processes doesn't leak windows TLS indices
// API Information:
// Emulator
// Details:
// - Test the mechanism to escape threads from the emulator in order
// to block on Windows objects:
// - test escape and re-enter mechanism.
// - block on Windows in EPOC thread and escaped EPOC thread.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32atomics.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include "e32test.h"
#include "emulator.h"
#include "t_emul.h"

#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union

LOCAL_D RTest test(_L("t_emul"));

static TInt ELock;
static HANDLE ESemaphore;
static TInt EResult;

TInt EscapeTimeoutThread(TAny*)
	{
	User::After(1000000);
	if (__e32_atomic_add_ord32(&ELock, 1) == 0)
		{
		EResult=KErrTimedOut;
		ReleaseSemaphore(ESemaphore,1,NULL);
		}
	return KErrNone;
	}

TInt EscapeSignalThread(TAny*)
	{
	if (__e32_atomic_add_ord32(&ELock, 1) == 0)
		{
		EResult=KErrNone;
		ReleaseSemaphore(ESemaphore,1,NULL);
		}
	return KErrNone;
	}

TInt DoTestEscape(TBool aDoEscape)
//
// Test the mechanism to escape threads from the emulator in order to block on Windows objects
//
	{
	ELock = 0;
	EResult = KRequestPending;
	ESemaphore = CreateSemaphoreA(NULL,0,1,NULL);
	test (ESemaphore != NULL);
	RThread t1,t2;
	TRequestStatus s1,s2;
	TInt r = t1.Create(KNullDesC,&EscapeSignalThread,0x1000,NULL,NULL);
	test (r == KErrNone);
	t1.SetPriority(EPriorityLess);
	t1.Logon(s1);
	t1.Resume();
	r = t2.Create(KNullDesC,&EscapeTimeoutThread,0x1000,NULL,NULL);
	test (r == KErrNone);
	t2.SetPriority(EPriorityMore);
	t2.Logon(s2);
	t2.Resume();
//
	if (aDoEscape)
		Emulator::Escape();
	r = WaitForSingleObject(ESemaphore,INFINITE);
	if (aDoEscape)
		Emulator::Reenter();
	test (r==WAIT_OBJECT_0);
//
	r = EResult;
	t1.Kill(0);
	t2.Kill(0);
	t1.Close();
	t2.Close();
	User::WaitForRequest(s1);
	User::WaitForRequest(s2);
//
	CloseHandle(ESemaphore);
	return r;
	}

void TestEscape()
//
// Test the mechanism to escape threads from the emulator in order to block on Windows objects
//
	{
	test.Start(_L("Test escape and reenter mechanism"));
	for (TInt i = 0;i<10000;++i)
		{
		Emulator::Escape();
		if (i%100 == 0)
			Sleep(10);
		Emulator::Reenter();
		}
	test.Next(_L("Block on Windows in EPOC thread"));
	TInt r = DoTestEscape(EFalse);
	test (r == KErrTimedOut);
	test.Next(_L("Block on Windows in escaped EPOC thread"));
	r = DoTestEscape(ETrue);
	test (r == KErrNone);
	test.End();
	}

TInt CountRemainingTlsIndicies()
	{
	const TInt KMax = 2000;
	
	TBool allocated[KMax];
	memclr(allocated, sizeof(TBool) * KMax);
	TInt i;
	for (i = 0 ; i < KMax ; ++i)
		{
		TInt index = TlsAlloc();
		if (index == TLS_OUT_OF_INDEXES)
			break;
		test(index >= 0 && index < KMax);
		allocated[index] = ETrue;
		}
	for (TInt j = 0 ; j < KMax ; ++j)
		{
		if (allocated[j])
			test(TlsFree(j));
		}
	return i;
	}
	   
void RunSlave(TSlaveAction aAction)
	{
	RProcess p;
	TBuf<8> arg;
	arg.Format(_L("%d"), aAction);
	test_KErrNone(p.Create(KTEmulSlaveName, arg));
	p.Resume();
	TRequestStatus status;
	p.Logon(status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	test_Equal(EExitKill, p.ExitType());
	p.Close();
	}

void TestRuntimeCleanup()
	{
	test.Start(_L("Test Codewarrior runtime library is correctly cleaned up"));
	TInt initIndicies = CountRemainingTlsIndicies();
	
	test.Next(_L("Test creating a process doesn't leak windows TLS indicies"));
	RunSlave(ESlaveDoNothing);
	test_Equal(initIndicies, CountRemainingTlsIndicies());

	test.Next(_L("Test leaving in an exe doesn't leak windows TLS indicies"));
	RunSlave(ESlaveTrapExceptionInExe);
	test_Equal(initIndicies, CountRemainingTlsIndicies());
	
	test.Next(_L("Test leaving in a linked DLL doesn't leak windows TLS indicies"));
	RunSlave(ESlaveTrapExceptionInLinkedDll);
	test_Equal(initIndicies, CountRemainingTlsIndicies());
	
	test.Next(_L("Test leaving in a loaded DLL doesn't leak windows TLS indicies"));
	RunSlave(ESlaveTrapExceptionInLoadedDll);
	test_Equal(initIndicies, CountRemainingTlsIndicies());

	test.Next(_L("Test cleanup doesn't happen while DLL still loaded"));
	RLibrary l;
	test_KErrNone(l.Load(KTEmulDll2Name));
	RunSlave(ESlaveTrapExceptionInLoadedDll);
	TInt midCount = CountRemainingTlsIndicies();
	test(initIndicies > midCount);

	test.Next(_L("Test previous detach doesn't cause runtime to be re-initalised"));
	TTrapExceptionInDllFunc func =
		(TTrapExceptionInDllFunc)l.Lookup(KTrapExceptionInDllOrdinal);
	test_NotNull(func);
	func();
	test_Equal(midCount, CountRemainingTlsIndicies());	
	l.Close();
	test_Equal(initIndicies, CountRemainingTlsIndicies());
	
	test.End();
	}


void DoSomething2L()
	{
	test.Printf(_L("@\n"));
	}


void DoSomething1L()
	{
	TInt i = -1, j = 1;
	for ( ; ; )
		{
		i++;
		//DoSomething2L() must only be called once , else we know that trap mechanism didn't work 
		test( i<2);
		if (i == j)  
			{
			User::Leave(KErrNotFound);
			}

		TRAP_IGNORE(DoSomething2L());
		TRAPD(errr, DoSomething2L());
		TInt r;
		TRAP(r, DoSomething2L());

		}
	}


void LeaveIfArgIsTwo(TInt aCall);

class CFred : public CBase
{
	public:
static CFred* NewLC();
	CFred();
	~CFred();
};

void DoSomething3L()
	{
	// Push something on the cleanup stack so we can see whyen it gets cleaned up.
	CFred *fred = CFred::NewLC();
		
	TInt beforeFunc=0;
	TInt betweenFuncAndTRAPD=0;
	TInt afterTRAPD=0;
	for(TInt loop=1; loop<=2; ++loop)
		{
		++beforeFunc;
		test.Printf(_L("Before LeaveIfArgIsTwo()\n"));

		// The first time around the loop, this function call works.
		// The second time, it leaves KErrGeneral
		// Then when TRAP mechanism isn't working properly the emulator (correctly) 
		// would delete fred, and (incorrectly) jump to the line
		// just after the TRAPD call a few lines further down the file!
		LeaveIfArgIsTwo(loop);

		++betweenFuncAndTRAPD;
		test.Printf(_L("Between LeaveIfArgIsTwo() and TRAPD\n"));

		TRAPD(err, test.Printf(_L("Inside TRAPD\n") ) );


		// It should only be possible to reach this section of code by executing all the lines
		// between LeaveIfArgIsTwo and here.
		++afterTRAPD;
		
		}

	// Should NEVER get here because LeaveIfArgIsTwo did a leave the second time around the loop
	test.Printf(_L("After loop (should NEVER get here) -\n\
					beforeFunc %d\n\
					betweenFuncAndTRAPD %d\n\
					afterTRAPD %d\n"),
					beforeFunc, betweenFuncAndTRAPD, afterTRAPD);
	
	test.Printf(_L("It should be impossible for afterTRAPD to be larger than betweenFuncAndTRAPD\n"));
	test(afterTRAPD <= betweenFuncAndTRAPD);

	// Cleanup our cleanup stack. 
	CleanupStack::PopAndDestroy(&fred);
	}

void LeaveIfArgIsTwo(TInt aCall)
	{
	
	test.Printf(_L("aCall    %d\n"), aCall);
	if(aCall == 2)
		{
		User::Leave(KErrGeneral);
		}
	}

CFred *CFred::NewLC()
	{
	CFred *self = new(ELeave) CFred;
	CleanupStack::PushL(self);
	return self;
	}

CFred::CFred()
	{
	test.Printf(_L("CFred %x\n"), this);
	}

CFred::~CFred()
	{
	test.Printf(_L("~CFred %x\n"), this);
	}



GLDEF_C TInt E32Main()
//
//
//
	{
	test.Title();
	test.Start(_L("Starting tests ..."));

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();
	
	TestEscape();
#ifdef __CW32__
	TestRuntimeCleanup();
#endif

	// The following tests were added to test that the TRAP mechanism works correctly in case of
	// nested TRAP's. A compiler bug (winscw) caused the following tests to fail, since the wrong 
	// trap handler would be invoked, when User::Leave() was called.
	
	test.Next(_L("Check User::Leave is handled by the correct TRAP handler when nested TRAPs are present - Simple scenario\n")); 
		
 	TRAPD(err, DoSomething1L());
	test(err == KErrNotFound);
	
	test.Next(_L("Check User::Leave is handled by the correct TRAP handler when nested TRAPs are present - Cleanup stack scenario\n")); 		
	__UHEAP_MARK;
	CTrapCleanup* tc = CTrapCleanup::New();
	if (tc == 0) return KErrNoMemory;
	
	TRAPD(err2, DoSomething3L());
	test(err2==KErrGeneral);	

	delete tc;
	__UHEAP_MARKEND;
	
	
	test.End();
	test.Close();
	return KErrNone;
	}

