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
// e32test\debug\t_context.cpp
// Overview:
// Exercise kernel-side functions to get, set user-side context and 
// kernel-side exception handlers.
// API Information:
// RBusLogicalChannel, DLogicalChannel.
// Details:
// - Load the context logical device driver, open user side handle to 
// a LDD channel and check software exception scheme when handle is 
// not created.
// - Check user-side handler called when kernel-side handler is not
// created and exception is handled as expected.
// - Make a synchronous Kernel Executive type request to logical channel
// for a specific functionality and check that it is as expected.
// - Check that kernel-side handler is not triggered by IsExceptionHandler 
// method.
// - Test user-side, kernel-side handlers are as expected.
// - In the context of thread taking hardware exception with synchronous
// and asynchronous killing:
// - check context captured in hardware exception hook and
// context captured in RemoveThread hook are as expected.
// - In the context of thread taking software exception with synchronous 
// and asynchronous killing:
// - check context captured in software exception hook and
// context captured in RemoveThread hook are as expected.
// - In the context of thread blocked on WFAR:
// - check context captured while thread is blocked, 
// context can be modified while thread is blocked, 
// context captured in RemoveThread hook are as expected.
// - In the context of thread killed after being pre-empted by interrupt 
// while in user mode:
// - check context captured while thread is spinning, context can be 
// modified while thread is blocked, context captured in RemoveThread 
// hook are as expected.
// - Check whether heap has been corrupted by all the tests.
// - Check that the system doesn't crash when the context of a dead thread is
// set or retrieved.
// Platforms/Drives/Compatibility:
// Hardware (Automatic). 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include <u32hal.h>
#include "d_context.h"
#include "context.h"

enum TKillMode 
	{ 
	ESynchronousKill, // thread kills itself in exception handler
	EAsynchronousKill // thread suspends itself in exception handler and is killed by main thread
	};

_LIT(KUserExecPanic, "USER-EXEC");

RTest test(_L("T_CONTEXT"));

TInt KernelExcCount; // incremented when user-side exception handler called
TInt UserExcCount;   // incremented when kernel-side exception handler called
TExcType LastUserExcType; // set by user-side exception handler when called

RContextLdd Channel;


TInt CallDeviceDriverAndSpin(TAny* aExpected)
	{
	TArmRegSet* expectedContext = (TArmRegSet*)aExpected;
	expectedContext->iR13 = Channel.SpinInKernel(EFalse);
	Channel.SpinInKernel(ETrue);
	return 0;
	}

TInt UserRaiseExceptionThread(TAny* aExcType)
	{
	TExcType e = static_cast<TExcType>(reinterpret_cast<TInt>(aExcType));
	User::RaiseException(e);
	return 0;
	}

TInt RThreadRaiseExceptionThread(TAny* aExcType)
	{
	TExcType e = static_cast<TExcType>(reinterpret_cast<TInt>(aExcType));
	User::RaiseException(e);
	return 0;
	}

void UserExceptionHandler(TExcType aType)
	{
	// Check kernel-side handler was called before user-side one
	if (Channel.IsHooked() && KernelExcCount != 1)
		User::Invariant(); // force failure in RaiseExceptionHarness()

	LastUserExcType = aType;
	UserExcCount++;
	User::Panic(KUserExecPanic, ECausedException);
	}

TInt UserTrapExceptionThread(TAny* aExcType)
	{
	TInt r = User::SetExceptionHandler(UserExceptionHandler, 0xFFFFFFFF);
	if (r != KErrNone)
		return 0; // force failure in RaiseExceptionHarness()
	TExcType e = static_cast<TExcType>(reinterpret_cast<TInt>(aExcType));
	User::RaiseException(e);
	return 0;
	}
	
TInt NaturalDeathFunc(TAny*)
	{// Don't do too much but be sure to complete.
	return KErrNone;
	}


void RaiseExceptionHarness(TThreadFunction aFn, TExcType aType)
	{
	RThread t;
	TInt r = t.Create(_L("RaiseException"), aFn, KDefaultStackSize, NULL, 
						reinterpret_cast<TAny*>(aType));
	test(r == KErrNone);

	TRequestStatus s;
	t.Logon(s);
	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Resume();
	User::WaitForRequest(s);
	User::SetJustInTime(oldJitSetting);

	test(t.ExitType() == EExitPanic);
	test(t.ExitCategory() == KUserExecPanic);
	test(t.ExitReason() == ECausedException);
	CLOSE_AND_WAIT(t);
	}


void TestSwExcNoHandlers()
	{
	test.Next(_L("Check software exception scheme when no handler"));

	RaiseExceptionHarness(UserRaiseExceptionThread, EExcGeneral);
	RaiseExceptionHarness(RThreadRaiseExceptionThread, EExcGeneral);
	}

void TestUserSwExcHandlerAlone()
	{
	test.Next(_L("Check user-side handler called when no kernel-side handler"));

	UserExcCount = 0;
	RaiseExceptionHarness(UserTrapExceptionThread, EExcIntegerDivideByZero);
	test(LastUserExcType == EExcIntegerDivideByZero);
	test(UserExcCount == 1);
	}

void TestUserAndKernelSwExcHandlers()
	{
	test.Next(_L("Check kernel-side exception handler"));
	test.Start(_L("Check kernel-side handler called"));

	KernelExcCount = 0;
	RaiseExceptionHarness(UserRaiseExceptionThread, EExcUserInterrupt);
	test(KernelExcCount == 1);
	test(Channel.LastException() == EExcUserInterrupt);

	KernelExcCount = 0;
	RaiseExceptionHarness(RThreadRaiseExceptionThread, EExcIntegerDivideByZero);
	test(KernelExcCount == 1);
	test(Channel.LastException() == EExcIntegerDivideByZero);

	test.Next(_L("Check kernel-side handler not triggered by IsExceptionHandler()"));
	
	KernelExcCount = 0;
	(void) User::IsExceptionHandled(EExcUserInterrupt);
	test(KernelExcCount == 0);

	test.Next(_L("Check user-side + kernel-side handler"));

	UserExcCount = 0;
	KernelExcCount = 0;
	RaiseExceptionHarness(UserTrapExceptionThread, EExcIntegerDivideByZero);
	test(LastUserExcType == EExcIntegerDivideByZero);
	test(UserExcCount == 1);
	test(KernelExcCount == 1);
	test(Channel.LastException() == EExcIntegerDivideByZero);

	test.End();
	}


//////////////////////////////////////////////////////////////////////////////

void DumpContext(const TDesC& aTitle, TArmRegSet& aContext)
	{
	test.Printf(_L("%S\n"), &aTitle);
	test.Printf(_L("  r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n"),aContext.iR0,aContext.iR1,aContext.iR2,aContext.iR3);
	test.Printf(_L("  r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n"),aContext.iR4,aContext.iR5,aContext.iR6,aContext.iR7);
	test.Printf(_L("  r8 =%08x r9 =%08x r10=%08x r11=%08x\n"),aContext.iR8,aContext.iR9,aContext.iR10,aContext.iR11);
	test.Printf(_L("  r12=%08x r13=%08x r14=%08x r15=%08x\n"),aContext.iR12,aContext.iR13,aContext.iR14,aContext.iR15);
	test.Printf(_L("  cpsr=%08x dacr=%08x\n"),aContext.iFlags, aContext.iDacr);
	}

void ModifyContext(TArmRegSet& aContext)
	{
	TArmReg* end = (TArmReg*)(&aContext+1);
	for (TArmReg* p = (TArmReg*)&aContext; p<end; ++p)
		*p = ~*p;
	}


void TestHwExcContext(TKillMode aKillMode, TUserCallbackState aCallback)
	{
	test.Next(_L("Check context of thread taking hardware exception"));
	if (aKillMode == EAsynchronousKill)
		test.Printf(_L("Thread will be asynchronously killed\n"));
	else
		test.Printf(_L("Thread will kill itself\n"));

	TArmRegSet expectedContext;
	RThread t;
	TInt r = t.Create(_L("ContextHwExc"), ThreadContextHwExc, KDefaultStackSize, NULL, &expectedContext);
	test(r == KErrNone);

	TArmRegSet exceptionContext;
	TRequestStatus exceptionStatus;
	Channel.TrapNextHwExc(t.Id(), &exceptionContext, exceptionStatus, aKillMode==ESynchronousKill);

	TRequestStatus deathStatus;
	TArmRegSet deathContext;
	Channel.TrapNextDeath(t.Id(), &deathContext, deathStatus);

	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Resume();
	User::WaitForRequest(exceptionStatus);
	User::SetJustInTime(oldJitSetting);
	test(exceptionStatus == KErrNone); // See CheckSetContext() in LDD

	test.Start(_L("Check context captured in hardware exception hook"));
	exceptionContext.iR15 += 4; // See CheckContextHwExc()
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Context in hw exception hook:"), exceptionContext);
	test(CheckContextHwExc(&exceptionContext, &expectedContext));

	if (aCallback != ENoCallback)
		{
		test.Printf(_L("Adding user callback type %d\n"), aCallback);
		Channel.AddUserCallback(t.Id(), aCallback);
		Channel.ResumeTrappedThread(t.Id());
		User::After(500000);

		test.Next(_L("Check context captured while thread is in callback"));
		TArmRegSet callbackContext;
		Channel.GetContext(t.Id(), &callbackContext);
		callbackContext.iR15 += 4; // See CheckContextHwExc()
		DumpContext(_L("Expected context:"), expectedContext);
		DumpContext(_L("Actual context:"), callbackContext);
		test(CheckContextHwExc(&callbackContext, &expectedContext));
		}

	ModifyContext(expectedContext);
	DumpContext(_L("Expected context after modification:"), expectedContext);

	if (aKillMode == EAsynchronousKill)
		{
		test.Next(_L("Check context can be modified by another thread"));
		TArmRegSet modifiedContext = expectedContext;
		test(Channel.SetAndGetBackContext(t.Id(), &modifiedContext) == KErrNone);
		modifiedContext.iR15 = exceptionContext.iR15; // See CheckContextHwExc()
		DumpContext(_L("Actual context after modification:"), modifiedContext);
		test(CheckContextHwExc(&modifiedContext, &expectedContext));

		User::SetJustInTime(EFalse);
		t.Panic(_L("TEST"), 42);
		}
	else
		{
		// The thread has modified its own user context in the exception hook.
		User::SetJustInTime(EFalse);
		}

	User::WaitForRequest(deathStatus);
	User::SetJustInTime(oldJitSetting);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Check context captured in RemoveThread hook"));
	deathContext.iR15 = exceptionContext.iR15; // See CheckContextHwExc()
	DumpContext(_L("context in RemoveThread hook:"), deathContext);
	test(CheckContextHwExc(&deathContext, &expectedContext));
	test.End();
	}


void TestSwExcContext(TKillMode aKillMode, TUserCallbackState aCallback)
	{
	test.Next(_L("Check context of thread taking software exception (slow executive call)"));
	if (aKillMode == EAsynchronousKill)
		test.Printf(_L("Thread will be asynchronously killed\n"));
	else
		test.Printf(_L("Thread will kill itself\n"));

	TArmRegSet expectedContext;
	RThread t;
	TInt r = t.Create(_L("ContextSwExc"), ThreadContextSwExc, KDefaultStackSize, NULL, &expectedContext);
	test(r == KErrNone);

	TArmRegSet exceptionContext;
	TRequestStatus exceptionStatus;
	Channel.TrapNextSwExc(t.Id(), &exceptionContext, exceptionStatus, aKillMode == ESynchronousKill);

	TRequestStatus deathStatus;
	TArmRegSet deathContext;
	Channel.TrapNextDeath(t.Id(), &deathContext, deathStatus);

	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Resume();
	User::WaitForRequest(exceptionStatus);
	User::SetJustInTime(oldJitSetting);
	test(exceptionStatus == KErrNone); // See CheckSetContext() in LDD

	test.Start(_L("Check context captured in software exception hook"));
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Context in sw exception hook:"), exceptionContext);
	test(CheckContextSwExc(&exceptionContext, &expectedContext));

	if (aCallback != ENoCallback)
		{
		test.Printf(_L("Adding user callback type %d\n"), aCallback);
		Channel.AddUserCallback(t.Id(), aCallback);
		Channel.ResumeTrappedThread(t.Id());
		User::After(500000);

		test.Next(_L("Check context captured while thread is in callback"));
		TArmRegSet callbackContext;
		Channel.GetContext(t.Id(), &callbackContext);
		DumpContext(_L("Expected context:"), expectedContext);
		DumpContext(_L("Actual context:"), callbackContext);
		test(CheckContextSwExc(&callbackContext, &expectedContext));
		}

	ModifyContext(expectedContext);
	DumpContext(_L("Expected context after modification:"), expectedContext);

	if (aKillMode == EAsynchronousKill)
		{
		test.Next(_L("Check context can be modified by another thread"));
		TArmRegSet modifiedContext = expectedContext;
		test(Channel.SetAndGetBackContext(t.Id(), &modifiedContext) == KErrNone);
		modifiedContext.iR14 = exceptionContext.iR14; // See CheckContextSwExc()
		modifiedContext.iR15 = exceptionContext.iR15; // See CheckContextSwExc()
		DumpContext(_L("Actual context after modification:"), modifiedContext);
		test(CheckContextSwExc(&modifiedContext, &expectedContext));

		User::SetJustInTime(EFalse);
		t.Panic(_L("TEST"), 24);
		}
	else
		{
		// The thread has modified its own user context in the exception hook.
		User::SetJustInTime(EFalse);
		}

	User::WaitForRequest(deathStatus);
	User::SetJustInTime(oldJitSetting);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Check context captured in RemoveThread hook"));
	deathContext.iR14 = exceptionContext.iR14; // See CheckContextSwExc()
	deathContext.iR15 = exceptionContext.iR15; // See CheckContextSwExc()
	DumpContext(_L("context in RemoveThread hook:"), deathContext);
	test(CheckContextSwExc(&deathContext, &expectedContext));
	test.End();
	}

void TestKillWFAR(TUserCallbackState aCallback)
	{
	test.Next(_L("Check context of thread blocked on WFAR"));

	TArmRegSet expectedContext;
	RThread t;
	TInt r = t.Create(_L("ContextWFAR"), ThreadContextWFAR, KDefaultStackSize, NULL, &expectedContext);
	test(r == KErrNone);

	TRequestStatus deathStatus;
	TArmRegSet deathContext;
	Channel.TrapNextDeath(t.Id(), &deathContext, deathStatus);

	// Boost thread's priority and kick it off.  This ensures we'll 
	// run again only after it is blocked on its request semaphore.
	t.SetPriority(EPriorityMore);
	t.Resume();
	User::After(500000);

	if (aCallback != ENoCallback)
		{
		test.Printf(_L("Adding user callback type %d\n"), aCallback);
		Channel.AddUserCallback(t.Id(), aCallback);
		t.SetPriority(EPriorityNormal);
		t.RequestSignal();
		User::After(500000);
		}

	test.Start(_L("Check context captured while thread is blocked"));
	TArmRegSet blockedContext;
	Channel.GetContext(t.Id(), &blockedContext);
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Actual context:"), blockedContext);
	test(CheckContextWFAR(&blockedContext, &expectedContext));

	test.Next(_L("Check context can be modified while thread is blocked"));
	ModifyContext(expectedContext);
	expectedContext.iR14 = blockedContext.iR14; // See CheckContextWFAR()
	expectedContext.iR15 = blockedContext.iR15; // See CheckContextWFAR()
	TArmRegSet modifiedContext = expectedContext;
	test(Channel.SetAndGetBackContext(t.Id(), &modifiedContext) == KErrNone);
	DumpContext(_L("Expected context after modification:"), expectedContext);
	DumpContext(_L("Actual context after modification:"), modifiedContext);
	test(CheckContextWFAR(&modifiedContext, &expectedContext));
	
	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Panic(_L("TEST"), 999);
	User::WaitForRequest(deathStatus);
	User::SetJustInTime(oldJitSetting);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Check context captured in RemoveThread hook"));
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("context in RemoveThread hook:\n"), deathContext);
	test(CheckContextWFARDied(&deathContext, &expectedContext));
	test.End();
	}

void TestKillInterrupt(TUserCallbackState aCallback)
	{
	test.Next(_L("Check context of thread killed after being preempted by interrupt while in user mode"));

	TArmRegSet expectedContext;
	RThread t;
	TInt r = t.Create(_L("ContextKillInterrupt"), ThreadContextUserInt, KDefaultStackSize, NULL, &expectedContext);
	test(r == KErrNone);

	TArmRegSet trappedContext;
	TRequestStatus trapStatus;
	Channel.TrapNextDeath(t.Id(), &trappedContext, trapStatus);
	
	t.Resume();
	User::After(500000);

	if (aCallback != ENoCallback)
		{
		test.Printf(_L("Adding user callback type %d\n"), aCallback);
		Channel.AddUserCallback(t.Id(), aCallback);
		User::After(500000);
		}

	test.Start(_L("Check context captured while thread is spinning"));
	TArmRegSet spinContext;
	Channel.GetContext(t.Id(), &spinContext);
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Actual context:"), spinContext);
	test(CheckContextUserInt(&spinContext, &expectedContext));

	test.Next(_L("Check context can be modified while thread is spinning"));
	ModifyContext(expectedContext);
	expectedContext.iR15 = spinContext.iR15; // see CheckContextUserInt()
	expectedContext.iFlags = spinContext.iFlags; // must be left unmodified
	expectedContext.iDacr = spinContext.iDacr; // must be left unmodified
	DumpContext(_L("Expected context after modification:"), expectedContext);
	TArmRegSet modifiedContext = expectedContext;
	test(Channel.SetAndGetBackContext(t.Id(), &modifiedContext) == KErrNone);
	DumpContext(_L("Actual context after modification:"), modifiedContext);
	test(CheckContextUserInt(&modifiedContext, &expectedContext));
	
	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Kill(-1);
	User::WaitForRequest(trapStatus);
	User::SetJustInTime(oldJitSetting);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Check context captured in RemoveThread hook"));
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Trapped context:"), trappedContext);
	test(CheckContextUserIntDied(&trappedContext, &expectedContext));
	test.End();
	}

void TestKernelContext()
	{
	test.Start(_L("Test kernel context function"));
	
	TArmRegSet kernelContext, expectedContext;
	for (TInt i=0; i<15; ++i)
		*(((TUint32*)&expectedContext)+i) = 0xa0000000 + i;
	
	RThread t;
	TInt r = t.Create(_L("SpinInKernel"), CallDeviceDriverAndSpin, KDefaultStackSize, NULL, &expectedContext);
	test(r == KErrNone);
	t.Resume();
	
	User::After(500000);
	Channel.GetKernelContext(t.Id(), &kernelContext);
	
	TBool oldJitSetting = User::JustInTime();
	User::SetJustInTime(EFalse);
	t.Kill(-1);
	User::SetJustInTime(oldJitSetting);
	CLOSE_AND_WAIT(t);

	test.Next(_L("Check kernel context is as expected"));
	DumpContext(_L("Expected context:"), expectedContext);
	DumpContext(_L("Actual context:"), kernelContext);
	test(CheckContextKernel(&kernelContext, &expectedContext));

	test.End();
	}

void TestNaturalDeathContext()
	{
	test.Start(_L("Test setting and retrieving the context of a dead thread"));
	TArmFullContext context;
	memclr(&context, sizeof context);

	RThread t;
	TInt r = t.Create(_L("NaturalDeath"), NaturalDeathFunc, KDefaultStackSize, NULL, NULL);
	test_KErrNone(r);
	t.Resume();
	
	User::After(500000);
	Channel.SetAndGetFullContext(t.Id(), &context);

	// Verify that no registers should be available for this dead thread.
	test_Equal(0, context.iUserAvail);
	test_Equal(0, context.iSystemAvail);

	t.Close();
	test.End();
	}

TInt E32Main()
	{
	test.Title();
	
	test.Start(_L("Loading LDD"));
	TInt r = User::LoadLogicalDevice(_L("D_CONTEXT"));
	test(r == KErrNone || r == KErrAlreadyExists);
	
	// Must be after LDD loading because User::FreeLogicalDevice() does not
	// free anything currently.
	__KHEAP_MARK; 

	test.Next(_L("Opening LDD channel"));
	r = Channel.Open();
	test(r == KErrNone);

	TestSwExcNoHandlers();
	TestUserSwExcHandlerAlone();

	test.Next(_L("Enable event hook"));
	r = Channel.Hook(&KernelExcCount);
	test(r == KErrNone);

	TestUserAndKernelSwExcHandlers();
	TestHwExcContext(ESynchronousKill, ENoCallback);
	TestHwExcContext(EAsynchronousKill, ENoCallback);
	TestSwExcContext(ESynchronousKill, ENoCallback);
	TestSwExcContext(EAsynchronousKill, ENoCallback);
	TestKillWFAR(ENoCallback);
	TestKillInterrupt(ENoCallback);
	TestKernelContext();
	TestNaturalDeathContext();
#ifdef _DEBUG
	TestHwExcContext(EAsynchronousKill, ESpinningCallback);
	TestHwExcContext(EAsynchronousKill, ESleepingCallback);
	TestSwExcContext(EAsynchronousKill, ESpinningCallback);
	TestSwExcContext(EAsynchronousKill, ESleepingCallback);
	TestKillWFAR(ESpinningCallback);
	TestKillWFAR(ESleepingCallback);
	TestKillInterrupt(ESpinningCallback);
	TestKillInterrupt(ESleepingCallback);
#endif

	Channel.Close();

	// Wait for idle + async cleanup (waits for DKernelEventHandler to go away)
	r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	test_KErrNone(r);

	__KHEAP_MARKEND;

	// Don't check user heap because if TLS is stored on the user-side this will not be freed when
	// the thread exits due to an exception.

	test.End();
	return 0;
	}
