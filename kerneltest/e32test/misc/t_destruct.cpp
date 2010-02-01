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
// e32test\misc\t_destruct.cpp
// Test that global objects are destroyed correctly
// 
//

#define __E32TEST_EXTENSION__

#include <e32std.h>
#include <e32std_private.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <f32file.h>
#include <e32test.h>
#include <e32msgqueue.h>

#include "t_destruct.h"

_LIT(KDestructSlave, "t_destruct_slave");

const char* KMessageNames[] =
	{
	"Construct",
	"ConstructStatic",
	"ConstructDynamic",
	"ConstructStatic3",
	"PreDestruct",
	"Destruct",
	"DestructStatic",
	"DestructDynamic",
	"DestructStatic3"
	};

RTest test(_L("t_destruct"));
RMsgQueue<TMessage> MessageQueue;

void TestNextMessage(TMessage aExpected, TBool& ok)
	{
	TMessage message;
	TInt r = MessageQueue.Receive(message);
	if (r == KErrUnderflow)
		{
		RDebug::Printf(" * expected message %s but got underflow", KMessageNames[aExpected]);
		ok = EFalse;
		return;
		}
	test_KErrNone(r);
	test(message < ENumMessges);
	test(aExpected < ENumMessges);
	if (message == aExpected)
		RDebug::Printf(" - received message %s", KMessageNames[message]);
	else
		{
		RDebug::Printf(" * expected message %s but got %s", KMessageNames[aExpected], KMessageNames[message]);
		ok = EFalse;
		}
	}

void TestDestruction(TTestType aTestType)
	{
	TBuf<4> cmd;
	cmd.AppendFormat(_L("%d"), aTestType);

	RProcess p;
	test_KErrNone(p.Create(KDestructSlave, cmd));
	
	TRequestStatus status;
	p.Logon(status);
	p.Resume();
	User::WaitForRequest(status);

	TExitType expectedExit = aTestType != ETestLastThreadPanic ? EExitKill : EExitPanic;
	test_Equal(expectedExit, p.ExitType());
	test_Equal(aTestType, p.ExitReason());

	CLOSE_AND_WAIT(p);
	
	TMessage message;
	TBool ok = ETrue;

	TestNextMessage(EMessageConstructStatic3, ok);
	TestNextMessage(EMessageConstructStatic, ok);
	TestNextMessage(EMessageConstruct, ok);
	TestNextMessage(EMessageConstructDynamic, ok);
	TestNextMessage(EMessagePreDestruct, ok);
	
	if (aTestType != ETestLastThreadPanic && aTestType != ETestDestructorExits)
		{
		TestNextMessage(EMessageDestruct, ok);
		TestNextMessage(EMessageDestructStatic, ok);
		TestNextMessage(EMessageDestructStatic3, ok);
		}
	
	if (aTestType != ETestLastThreadPanic)
		TestNextMessage(EMessageDestructDynamic, ok);
	
	test(ok);
	test_Equal(KErrUnderflow, MessageQueue.Receive(message));
	}

TInt E32Main()
	{
    test.Title();
    test.Start(_L("t_destruct"));

	// Turn off evil lazy dll unloading
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();

	test_KErrNone(MessageQueue.CreateGlobal(KMessageQueueName, 10));
	
	test.Next(_L("Test global object destruction when main thread returns"));
	TestDestruction(ETestMainThreadReturn);
	
	test.Next(_L("Test global object destruction when main thread exits"));
	TestDestruction(ETestMainThreadExit);
	
	test.Next(_L("Test global object destruction when child thread exits"));
	TestDestruction(ETestChildThreadReturn);

	test.Next(_L("Test global object destruction when other thread has exited"));
	TestDestruction(ETestOtherThreadExit);

	test.Next(_L("Test global object destruction when other thread has panicked"));
	TestDestruction(ETestOtherThreadPanic);

	test.Next(_L("Test global object destruction when other thread killed by critial thread exit"));
	TestDestruction(ETestOtherThreadRunning);

	test.Next(_L("Test global object destruction when permanent thread exits"));
	TestDestruction(ETestPermanentThreadExit);

	test.Next(_L("Test global object destruction only happens once when destrctor creates new thread"));
	TestDestruction(ETestRecursive);

	test.Next(_L("Test global object destruction only happens once when destrctor calls User::Exit"));
	TestDestruction(ETestDestructorExits);

	test.Next(_L("Test NO global object destruction when last thread panics"));
	TestDestruction(ETestLastThreadPanic);
	
    test.End();
	return KErrNone;
	}
