// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_cli.cpp
// Overview:
// Command line tests
// API Information:
// User::CommandLine, User::CommandLineLength
// Details:
// - Test the command line data and the command line length. 
// Verify the results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32panic.h>

_LIT(KLitKernExec,"KERN-EXEC");
_LIT(KCmdLine, "Turnip goat fridge, far larder mungo humm.  Yes?");

const TInt EExitError = -1;
const TInt EExitCmdLineMatch = 42;
const TInt EExitCmdLine256 = 43;

LOCAL_D RTest test(_L("T_CLI"));

void runtests()
	{

	test.Next(_L("Test short commmand line"));
	TRequestStatus stat;
	RProcess p;
	TBuf<280> text=KCmdLine();
	TFileName filename;
	filename=p.FileName();
	TInt r=p.Create(filename,text);
	test(r==KErrNone);
	test.Next(_L("Run and close process"));
	p.Logon(stat);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==EExitCmdLineMatch);
	CLOSE_AND_WAIT(p);

	test.Next(_L("Test command line with length 256"));
	text.SetLength(256);
	text.Fill('Z');
	test.Next(_L("Create process"));
	r=p.Create(filename, text);
	test(r==KErrNone);
	test.Next(_L("Run and close process"));	
	p.Logon(stat);
	p.Resume();
	User::WaitForRequest(stat);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==EExitCmdLine256);
	CLOSE_AND_WAIT(p);

	test.Next(_L("Test long command line"));
	text.SetLength(280);
	text.Fill('a');
	r=p.Create(filename, text);
	test(r==KErrNone);
	test.Next(_L("Run and close process"));
	p.Logon(stat);
	p.SetJustInTime(EFalse);	// don't let debugger spoil the test
	p.Resume();
	User::WaitForRequest(stat);
	test.Next(_L("Test Panic type"));
	test.Printf(_L("%d %d\n"), p.ExitType(), p.ExitReason());
	test(p.ExitType()==EExitPanic);
	test(p.ExitReason()==EKUDesSetLengthOverflow);
	test(p.ExitCategory()==KLitKernExec);
	CLOSE_AND_WAIT(p);
	}

GLDEF_C TInt E32Main()
    {

	test.Title();
	test.Start(_L("Command line"));

	TInt r;
	TInt len=User::CommandLineLength();
	TBuf<256> c;
	if (len==0)
		{
		runtests();
		r = 0;
		}
	else if (len == KCmdLine().Length())
		{
		User::CommandLine(c);
		r = (c.Match(KCmdLine) == 0) ? EExitCmdLineMatch : EExitError;
		}
	else
		{
		test.Next(_L("Test CommandLine(void)"));
		// This should panic for command lines > 256
		User::CommandLine(c);
		r = EExitCmdLine256;
		}

	test.End();
	return r;
    }
