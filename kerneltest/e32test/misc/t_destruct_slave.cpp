// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_destruct_slave.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32debug.h>
#include <e32msgqueue.h>

#include "t_destruct.h"

_LIT(KDynamicDll, "t_destruct_dll2");

class TTestObject
	{
public:
	TTestType iTestType;
public:
	TTestObject();
	~TTestObject();
	};

RTest test(_L("t_desruct_slave"));
TThreadId MainThreadId;
TTestObject GlobalObjectWithDestructor;

void Panic(TInt aReason)
	{
	User::Panic(_L("t_destruct_slave"), aReason);
	}

TInt ExitThread(TAny*)
	{
	return KErrNone;
	}

TInt PanicThread(TAny*)
	{
	Panic(KErrNone);
	return KErrNone;
	}

TInt LoopThread(TAny*)
	{
	// Open handle on dynamic DLL in this thread
	RLibrary library;
	test_KErrNone(library.Load(KDynamicDll));
	for (;;)
		;
	}

TInt ChildThread(TAny* aArg)
	{
	TInt testType = (TInt)aArg;
	RThread mainThread;
	TInt r = mainThread.Open(MainThreadId);
	if (r != KErrNone)
		return r;
	// Open handle on dynamic DLL in this thread
	RLibrary library;
	test_KErrNone(library.Load(KDynamicDll));
	RThread().Rendezvous(KErrNone);
	TRequestStatus status;
	mainThread.Logon(status);
	User::WaitForRequest(status);
	if (mainThread.ExitType() != EExitKill)
		return KErrGeneral;
	if (mainThread.ExitReason() != KErrNone)
		return mainThread.ExitReason();
	mainThread.Close();
	if (testType != ETestRecursive)
		{
		RMsgQueue<TMessage> messageQueue;
		r = messageQueue.OpenGlobal(KMessageQueueName);
		if (r != KErrNone)
			return r;
		r = messageQueue.Send(EMessagePreDestruct);
		if (r != KErrNone)
			return r;
		}
	return testType;
	}

TInt PermanentThread(TAny* aArg)
	{
	TInt testType = (TInt)aArg;
	// Open handle on dynamic DLL in this thread
	RLibrary library;
	TInt r = library.Load(KDynamicDll);
	if (r != KErrNone)
		return r;
	RMsgQueue<TMessage> messageQueue;
	r = messageQueue.OpenGlobal(KMessageQueueName);
	if (r != KErrNone)
		return r;
	r = messageQueue.Send(EMessagePreDestruct);
	if (r != KErrNone)
		return r;
	messageQueue.Close();
	User::SetCritical(User::EProcessPermanent);
	User::Exit(testType);
	return KErrGeneral;
	}

TTestObject::TTestObject()
	{
	RDebug::Printf("t_destruct_slave constructor called\n");
	RMsgQueue<TMessage> messageQueue;
	TInt r = messageQueue.OpenGlobal(KMessageQueueName);
	if (r != KErrNone)
		Panic(r);
	messageQueue.Send(EMessageConstruct);
	if (r != KErrNone)
		Panic(r);
	}

TTestObject::~TTestObject()
	{
	RDebug::Printf("t_destruct_slave destructor called\n");
	if (iTestType == ETestRecursive)
		{
		// Start child thread passing this thread's id
		MainThreadId = RThread().Id();
		RThread childThread;
		test_KErrNone(childThread.Create(_L("ChildThread"), ChildThread, 4096, NULL, (TAny*)iTestType));
		TRequestStatus status;
		childThread.Rendezvous(status);
		childThread.Resume();

		// Wait for child to open handle on this thread
		User::WaitForRequest(status);
		test_KErrNone(status.Int());
		childThread.Close();

		// Set this thread non-critical
		User::SetCritical(User::ENotCritical);
		}
	else if (iTestType == ETestDestructorExits)
		{
		User::Exit(iTestType);
		}

	RMsgQueue<TMessage> messageQueue;
	TInt r = messageQueue.OpenGlobal(KMessageQueueName);
	if (r != KErrNone)
		Panic(r);
	messageQueue.Send(EMessageDestruct);
	if (r != KErrNone)
		Panic(r);
	}

TInt E32Main()
	{
	StaticMain();
	
	RBuf cmd;
	test_KErrNone(cmd.Create(User::CommandLineLength()));
	User::CommandLine(cmd);

	TLex lex(cmd);
	TTestType type;
	test_KErrNone(lex.Val((TInt&)type));
	GlobalObjectWithDestructor.iTestType = type;

	RMsgQueue<TMessage> messageQueue;
	test_KErrNone(messageQueue.OpenGlobal(KMessageQueueName));

	// Dynamically load DLL with global data
	RLibrary library;
	test_KErrNone(library.Load(KDynamicDll));
	
	switch(type)
		{
		case ETestMainThreadReturn:
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			return type;

		case ETestMainThreadExit:
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			User::Exit(type);
			break;

		case ETestChildThreadReturn:
			{
			// Start child thread passing this thread's id
			MainThreadId = RThread().Id();
			RThread childThread;
			test_KErrNone(childThread.Create(_L("ChildThread"), ChildThread, 4096, NULL, (TAny*)type));
			TRequestStatus status;
			childThread.Rendezvous(status);
			childThread.Resume();

			User::After(1);

			// Wait for child to open handle on this thread
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			childThread.Close();

			// Set this thread non-critical and exit
			User::SetCritical(User::ENotCritical);
			}
			break;

		case ETestOtherThreadExit:
			{
			RThread childThread;
			test_KErrNone(childThread.Create(_L("ChildThread"), ExitThread, 4096, NULL, (TAny*)type));
			childThread.Resume();
			TRequestStatus status;
			childThread.Logon(status);
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			childThread.Close();
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			}
			return type;

		case ETestOtherThreadPanic:
			{
			RThread childThread;
			test_KErrNone(childThread.Create(_L("ChildThread"), PanicThread, 4096, NULL, (TAny*)type));
			childThread.Resume();
			TRequestStatus status;
			childThread.Logon(status);
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			childThread.Close();
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			}
			return type;
			
		case ETestOtherThreadRunning:
			{
			RThread childThread;
			test_KErrNone(childThread.Create(_L("ChildThread"), LoopThread, 4096, NULL, (TAny*)type));
			childThread.Resume();
			childThread.Close();
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			}
			return type;
			
		case ETestPermanentThreadExit:
			{
			RThread childThread;
			test_KErrNone(childThread.Create(_L("ChildThread"), PermanentThread, 4096, NULL, (TAny*)type));
			childThread.Resume();
			TRequestStatus status;
			childThread.Logon(status);
			User::WaitForRequest(status);
			test_KErrNone(status.Int());
			childThread.Close();
			}
			break;
			
		case ETestRecursive:
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			break;

		case ETestDestructorExits:
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			break;

		case ETestLastThreadPanic:
			test_KErrNone(messageQueue.Send(EMessagePreDestruct));
			Panic(type);
			break;
			
		default:
			test(EFalse);
		}
	return KErrNone;
	}
