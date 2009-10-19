// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_emulex.cpp
// Test various things to do with exceptions on the emulator
// 
//

#include <e32test.h>

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winnt.h>

#define TestSuccess(r) { TInt _r = (r); if (_r != KErrNone) { test.Printf(_L("Error code: %d"), _r); test(EFalse); } }

// LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS ExceptionInfo);
// PVOID AddVectoredExceptionHandler(ULONG FirstHandler, PVECTORED_EXCEPTION_HANDLER VectoredHandler);
// ULONG RemoveVectoredExceptionHandler(PVOID Handler);

typedef TInt (WINAPI TVectoredHandler)(PEXCEPTION_POINTERS aExceptionInfo);
typedef TAny* (WINAPI TAddVectoredHandlerFunc)(TBool aFirstHandler, TVectoredHandler aHandler);
typedef TUint (WINAPI TRemoveVectoredHandlerFunc)(TAny* aHandler);

RTest test(_L("T_EMULEX"));

const TInt KSpinIterations = 5000;

TInt TestIterations = 500;
volatile TInt ExceptionCount;
volatile TInt SpinCount;
TAddVectoredHandlerFunc* AddVEHFunc;
TRemoveVectoredHandlerFunc* RemoveVEHFunc;

TBool LookupVEHFunctions()
//
// Look up VEH functions, return EFalse if not supported
//
	{
	HMODULE hLibrary = GetModuleHandleA("kernel32.dll");
	test(hLibrary != NULL);

	AddVEHFunc = (TAddVectoredHandlerFunc*)GetProcAddress(hLibrary, "AddVectoredExceptionHandler");
	RemoveVEHFunc = (TRemoveVectoredHandlerFunc*)GetProcAddress(hLibrary, "RemoveVectoredExceptionHandler");

	return AddVEHFunc && RemoveVEHFunc;
	}

TInt WINAPI VectoredHandler(PEXCEPTION_POINTERS /*ExceptionInfo*/)
	{
	for (TInt i = 0 ; i < KSpinIterations ; ++i)
		++SpinCount; // spin 
	return EXCEPTION_CONTINUE_SEARCH;
	}

TInt CppExceptionThreadFunction(TAny* /*aParam*/)
	{
	for (;;)
		{
		for (TInt i = 0 ; i < KSpinIterations ; ++i)
			++SpinCount; // spin
		try
			{
			throw 23;
			}
		catch(TInt /*x*/)
			{
			++ExceptionCount;
			}
		}
	}

TInt CppExceptionThreadFunction2(TAny* /*aParam*/)
	{
	for (;;)
		{
		for (TInt i = 0 ; i < KSpinIterations ; ++i)
			++SpinCount; // spin
		try
			{
			throw 23;
			}
		catch(TInt /*x*/)
			{
			User::Heap();
			++ExceptionCount;
			}
		}
	}

void HwExceptionHandler(TExcType /*aType*/)
	{
	++ExceptionCount;
	}

TInt HwExceptionThreadFunction(TAny* /*aParam*/)
	{
	User::SetExceptionHandler(HwExceptionHandler, KExceptionInteger);

	volatile int zero = 0, out;
	for (;;)
		{
		for (TInt i = 0 ; i < KSpinIterations ; ++i)
			++SpinCount; // spin
		out = 1 / zero;
		}
	}

void CreateTestThread(RThread& aThread, TThreadFunction aFunc)
	{
	TInt r = aThread.Create(_L("exceptionThread"),
							aFunc,
							KDefaultStackSize,
							NULL,
							NULL);
	test(r == KErrNone);
	}

void Test1(TThreadFunction aFunc)
	{
	for (TInt i = 0 ; i < TestIterations ; ++i)
		{
		RThread thread;
		CreateTestThread(thread, aFunc);
		ExceptionCount = 0;
		thread.SetPriority(EPriorityLess);
		thread.Resume();
		while (ExceptionCount == 0)
			User::AfterHighRes(1);

		test.Printf(_L("Iteration %d: ExceptionCount == %d\n"), i, ExceptionCount);

		TRequestStatus status;
		thread.Logon(status);
		thread.Kill(KErrGeneral);
		User::WaitForRequest(status);
		CLOSE_AND_WAIT(thread);
		}
	}

void Test2(TThreadFunction aFunc)
	{
	TAny* handle = (*AddVEHFunc)(ETrue, VectoredHandler);

	RThread thread;
	CreateTestThread(thread, aFunc);
	ExceptionCount = 0;
	SpinCount = 0;
	thread.Resume();

	for (TInt i = 0 ; i < TestIterations ; ++i)
		{
		ExceptionCount = 0;
		thread.SetPriority(EPriorityNormal);
		while (ExceptionCount == 0)
			;
		thread.SetPriority(EPriorityLess);
		
		test.Printf(_L("Iteration %d: ExceptionCount == %d\n"), i, ExceptionCount);

		// test exception handling doesn't deadlock
		try { throw 13; } catch (TInt /*x*/) { }
		}

	TRequestStatus status;
	thread.Logon(status);
	thread.Kill(KErrGeneral);
	User::WaitForRequest(status);
	CLOSE_AND_WAIT(thread);

	(*RemoveVEHFunc)(handle);
	}

void ParseCommandLine()
	{
	TBuf<128> buf;
	User::CommandLine(buf);
	if (buf != KNullDesC)
		{
		TLex lex(buf);
		lex.Val(TestIterations);
		}
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("T_EMULEX"));

	ParseCommandLine();

	test.Next(_L("Test killing a thread while it's taking a hardware exception"));
	Test1(HwExceptionThreadFunction);

	test.Next(_L("Test killing a thread while it's taking a C++ exception"));
	Test1(CppExceptionThreadFunction);

	test.Next(_L("Test killing a thread while it's taking a C++ exception, and subsequently calling into the kernel"));
	Test1(CppExceptionThreadFunction2);
	
	TBool vehSupported = LookupVEHFunctions();
	if (vehSupported)
		{
		test.Next(_L("Test thread preemption while taking a hardware exception"));
		Test2(HwExceptionThreadFunction);

		test.Next(_L("Test thread preemption while taking a C++ exception"));
		Test2(CppExceptionThreadFunction);
		}
		
	test.End();
	test.Close();
	return(0);
	}
