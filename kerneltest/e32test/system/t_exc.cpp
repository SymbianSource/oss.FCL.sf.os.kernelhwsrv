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
// e32test\system\t_exc.cpp
// In WINS T_EXC should be run from the command line only.
// T_EXC will not complete when run under the MSDEV debugger.
// Overview:
// Test and verify exception handling.
// API Information:
// User::SetExceptionHandler, User::RaiseException
// Details:
// - Create a global semaphore, verify success.
// - Test exceptions with no handlers: verify that divide by zero and 
// User::RaiseException() panic as expected.
// - Test exceptions with handlers: verify that divide by zero and
// User::RaiseException() call their exception handlers as expected.
// - Test exception raised in exception handler: verify divide by zero
// causes exception and an exception in the exception handler causes 
// a panic.
// - Verify the results are as expected when a thread causes a divide
// by zero exception.
// - Get context of interrupted thread, get context of thread waiting 
// for request, get context of suspended thread. Verify results are
// as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32svr.h>
#include "u32std.h"

#pragma warning( disable : 4723 ) // disable divide by zero warnings

#ifdef __MARM__
void UndefinedInstruction();
#endif

LOCAL_D RTest test(_L("T_EXC"));
RDebug debug;

TInt gCount=0;
RSemaphore gSem;
TBool outsideExcSupported=ETrue;

void excHandler(TExcType /*aType*/)
//
// An exception handler that does nothing
//
	{

	gCount++;
	}

void excHandlerFault(TExcType aType)
//
// An exception handler that causes an exception
//
	{
	debug.Print(_L("Handling exception %d and causing exception in handler"), aType);
	gCount++;

#ifdef __MARM__
	// There is no Divide By Zero exception on the Arm
	// Use undefined instruction instead
	UndefinedInstruction();
#else
	// Cause a div by zero exception
	volatile int i=0;
	volatile int j=10;
	int l=j/i;
	for (i=0; i<l; i++)
		;
#endif
	}

TInt waitSemaphore(TAny *)
//
// Sleep
//
	{
	RSemaphore s;
	TInt r=s.OpenGlobal(_L("A SEMAPHORE"));
	test(r==KErrNone);
	s.Wait();
	return KErrNone;
	}

TInt garfield(TAny *)
//
// Sleep for a long time
//
	{

	User::After(10000000);
	return KErrNone;
	}

TInt odie(TAny *)
//
// Run round in circles
//
	{
	FOREVER
		;
	}

TInt divideByZero(TAny *)
//
// Divide by zero
//
	{
#ifdef __MARM__
	// There is no Divide By Zero exception on the Arm
	// Use undefined instruction instead
	UndefinedInstruction();
	return(KErrNone);
#else
#ifdef __WINS__
#pragma warning( disable : 4189 )	// local variable is initialized but not referenced
#endif
	volatile int i=0, j=10;
	volatile int l=j/i;
	FOREVER
		;
#endif
	}
#pragma warning( default : 4723 )

TInt raiseException(TAny *)
//
// Raise an exception
//
	{

	User::RaiseException(EExcIntegerDivideByZero);
	User::After(500000);
	return KErrNone;
	}

void test1()
	{

	test.Start(_L("Create a thread (divideByZero)"));
	User::SetJustInTime(EFalse);
	RThread t;
	TRequestStatus s;
	TInt r;
	r=t.Create(_L("divideByZero"),divideByZero,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	test.Next(_L("Resume and wait for div by zero exception"));
	t.Resume();
	User::WaitForRequest(s);
	test.Printf(_L("Exit Type %d\r\n"),(TInt)t.ExitType());
	test(t.ExitType()==EExitPanic);
	test(t.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(t);
//
	
	test.Next(_L("Create a thread (raiseException)"));
	r=t.Create(_L("raiseException"),raiseException,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	test.Next(_L("Resume, and wait for raise exception"));
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType()==EExitPanic);
	test(t.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(t);

	test.End();
	}

TInt divideByZero2(TAny *)
	{
#ifdef __MARM__
	// There is no Div By Zero exception on the Arm so we use a data abort instead
	User::SetExceptionHandler(&excHandler, KExceptionAbort|KExceptionFault|KExceptionUserInterrupt);
#else
	User::SetExceptionHandler(&excHandler, KExceptionInteger);
#endif
	return divideByZero(0);
	}

TInt raiseException2(TAny *)
	{
	User::SetExceptionHandler(&excHandler, KExceptionInteger);
	return raiseException(0);
	}

void test2()
	{

	test.Start(_L("Create a thread (odie)"));
	RThread t;
	TRequestStatus s;
	TInt r;

	test.Next(_L("Create a thread (divideByZero)"));
	r=t.Create(_L("divideByZero"),divideByZero2,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	gCount=0;

	test.Next(_L("Resume, and wait for 10 divide by zero exceptions"));
	t.Resume();
	while (gCount<10)
		User::After(500000);
	test(gCount>=10);
	test.Next(_L("Kill the thread"));
	t.Kill(666);
	User::WaitForRequest(s);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==666);
	CLOSE_AND_WAIT(t);
//
	test.Next(_L("Create a thread (raiseException2)"));
	r=t.Create(_L("raiseException2"),raiseException2,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	gCount=0;
	test.Next(_L("Resume"));
	t.Resume();
	test.Next(_L("Wait for thread to finish"));
	User::WaitForRequest(s);
	test.Next(_L("Test thread raised an exception on itself"));
	test(gCount==1);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(t);

	test.End();
	}

TInt divideByZero3(TAny *)
	{
#ifdef __MARM__
	User::SetExceptionHandler(&excHandlerFault, KExceptionAbort|KExceptionFault|KExceptionUserInterrupt);
#else
	User::SetExceptionHandler(&excHandlerFault, KExceptionInteger);
#endif
	return divideByZero(0);
	}

void test3()
	{
	test.Start(_L("Create a thread (divideByZero3)"));
	RThread t;
	TRequestStatus s;
	TInt r;
	r=t.Create(_L("divideByZero3"),divideByZero3,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	gCount=0;
	test.Next(_L("Resume, and wait for thread to die"));
	t.Resume();
	User::WaitForRequest(s);
	test.Next(_L("Test thread raised one exception"));
	test(gCount==1);
	test.Next(_L("Test thread paniced on double exception"));
	test(t.ExitType()==EExitPanic);
	test(t.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(t);
//
	test.End();
	}

extern TInt dividebyzeroFn(TInt x)
	{
	volatile int i=x, j=10;
	volatile int l=j/i;
	for (i=0; i<l; i++)
		;
	return KErrNone;
	}

TInt dividebyzeroThread(TAny *)
	{
	return dividebyzeroFn(0);
	}

void testDivException()
	{
	RThread t;
	TRequestStatus s;
	test.Next(_L("Create divide by zero thread"));
	TInt r=t.Create(_L("drop dead"),dividebyzeroThread,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,NULL);
	test(r==KErrNone);
	t.Logon(s);
	test.Next(_L("Resume"));
	t.Resume();
	User::WaitForRequest(s);
	test.Next(_L("Test thread died because of EExcDivideByZero"));
	test(t.ExitType()==EExitPanic);
	test(t.ExitReason()==ECausedException);
	CLOSE_AND_WAIT(t);
	}

#ifndef __EPOC32__
TInt ContextThread0(TAny *)
	{
	FOREVER;
	}
#else
TInt ContextThread0(TAny *);
#endif

#ifndef __EPOC32__
TInt ContextThread1(TAny *)
	{
	FOREVER;
	}
#else
TInt ContextThread1(TAny *);
#endif

#ifndef __EPOC32__
TInt ContextThread2(TAny *)
	{
	FOREVER;
	}
#else
TInt ContextThread2(TAny *);
#endif

TUint32 RunContextThread(TThreadFunction aFunction, TUint32* aRegs)
	{
#ifdef __EPOC32__
	TUint32 pc = (TUint32)aFunction((TAny*)0x80000000);
#else
	TUint32 pc = 0;
#endif
	RThread t;
	TRequestStatus s;
	TInt r=t.Create(_L("Context"),aFunction,KDefaultStackSize,NULL,NULL);
	test(r==KErrNone);
	t.Logon(s);
	t.Resume();
	User::After(100000);

	TPtr8 buf((TUint8*)aRegs, 32*4, 32*4);
	TInt i;
	for (i=0; i<32; i++)
		aRegs[i]=0xbad00bad;
	t.Context(buf);
	if (buf.Length()==0)
		pc = 0;	// not supported
	t.Kill(KErrNone);
	User::WaitForRequest(s);
	CLOSE_AND_WAIT(t);
	return pc;
	}

struct SRegValues
	{
	TUint32		iValidMask;
	TUint32		iValues[32];
	};

const TInt KNumContextTests = 3;

static const TThreadFunction ContextTestFn[KNumContextTests] =
	{
	&ContextThread0,
	&ContextThread1,
	&ContextThread2
	};

#if defined(__CPU_ARM)
const TInt KPCIndex = 15;
static const SRegValues ExpectedRegs[KNumContextTests] =
	{
		{0x1ffff,
			{	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x00,
				0xa0000010, 0, 0, 0,	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0
			}
		},
		{0x0eff0,
			{	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x00,
				0xa0000010, 0, 0, 0,	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0
			}
		},
		{0x0eff0,
			{	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x00,
				0xa0000010, 0, 0, 0,	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0
			}
		}
	};

static const TUint32 KRegValidBitsMask[32] =
	{
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
		0xf00000ffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
	};

#elif defined(__CPU_X86)
const TInt KPCIndex = 15;
static const SRegValues ExpectedRegs[KNumContextTests] =
	{
		{0xe7ff,
			{	0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xe50e50e5, 0xeb0eb0eb, 0xe51e51e5, 0xed1ed1ed,
				0x0000001b, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000cd5, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
			}
		},
		{0xe7ff,
			{	0x00000000, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xe50e50e5, 0xeb0eb0eb, 0xe51e51e5, 0xed1ed1ed,
				0x0000001b, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000cd5, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
			}
		},
		{0xe7fa,	// EAX=exec number, ECX trashed by preprocess handler
			{	0xaaaaaaaa, 0xbbbbbbbb, 0xffff8001, 0xdddddddd, 0xe50e50e5, 0xeb0eb0eb, 0xe51e51e5, 0xed1ed1ed,
				0x0000001b, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000023, 0x00000cd5, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
			}
		}
	};

static const TUint32 KRegValidBitsMask[32] =
	{
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
		0x0000ffffu, 0x0000ffffu, 0x0000ffffu, 0x0000ffffu,		0x0000ffffu, 0x0000ffffu, 0x00000cd5u, 0xffffffffu,
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
	};

#endif

void CheckContextValues(TUint32* aRegs, const SRegValues& aExpected, TUint32 aPC)
	{
	test.Printf(_L("PC=%08x Context returned:\n"), aPC);
	TInt i;
	const TUint32* s = aRegs;
	for (i=0; i<8; ++i, s+=4)
		test.Printf(_L("%08x %08x %08x %08x\n"), s[0], s[1], s[2], s[3]);
	TUint32 v = aExpected.iValidMask;
	TBool ok = ETrue;
	for (i=0; i<32; ++i, v>>=1)
		{
		if (!(v&1))
			continue;
		TUint32 mask = KRegValidBitsMask[i];
		TUint32 actual = aRegs[i] & mask;
		TUint32 exp = (i == KPCIndex) ? aPC&mask : aExpected.iValues[i];
		if (actual != exp)
			{
			test.Printf(_L("%d: Expected %08x but got %08x\n"), i, exp, actual);
			ok = EFalse;
			}
		}
	test(ok);
	}

void testContext()
	{

	TUint32 regs[32];

	test.Next(_L("Get context of interrupted thread"));
	TInt tn;
	for (tn=0; tn<KNumContextTests; ++tn)
		{
		test.Printf(_L("Context test %d\n"), tn);
		TUint32 pc = RunContextThread(ContextTestFn[tn], regs);
		if (pc)
			{
			CheckContextValues(regs, ExpectedRegs[tn], pc);
			}
		}
	}

GLDEF_C TInt E32Main()
//
// __KHEAP_SETFAIL etc. not available in release mode, so don't test
//
	{

	test.Title();
	test.Start(_L("Create a semaphore"));
	TInt r=gSem.CreateGlobal(_L("A SEMAPHORE"),0);
	test(r==KErrNone);
	test.Next(_L("Test exceptions with no handlers"));
	test1();
	test.Next(_L("Test exceptions with handlers"));
	test2();
	test.Next(_L("Test exception raised in exception handler"));
	test3();
	test.Next(_L("Divide by zero exception"));
	testDivException();
	test.Next(_L("Test Context"));
	testContext();
	test.End();
	return(KErrNone);
	}
