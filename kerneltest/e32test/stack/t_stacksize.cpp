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
// e32test\stack\t_stacksize.cpp
// Overview:
// Verifies the correct implementation of CreateStackOverride method of RProcess.
// Meanings of constants:
// KDefaultStackSize: 
// - System wide default stack size in actual version;
// KNumberOfFitIteration: 
// - Number of DummyRecursion which can fit in KDefaultStackSize. 
// Value = KDefaultStackSize / 2KB - 1 
// KNumberOfUnfitIteration: 
// - Number of DummyRecursion which is raised a Panic based on stack overflow.
// Value = KDefaultStackSize / 2KB rounded up to nearest integer
// Details:
// ETestProcess1: Create a process with original RProcess::Create() and start it
// - it passes with KNumberOfFitIteration DummyRecursion .
// ETestProcess2: Create a process with RProcess::CreateWithStackOverride(), default 
// stack size and start it
// - it passes with KNumberOfFitIteration DummyRecursion.
// ETestProcess3: Create a process with original RProcess::Create() and start it
// - it raised Panic on Stack overflow with KNumberOfUnfitIteration DummyRecursion.
// ETestProcess4: Create a process with RProcess::CreateWithStackOverride(), default 
// stack size and start it
// - it raised Panic on Stack overflow with KNumberOfUnfitIteration DummyRecursion.
// ETestProcess5: Create a process with RProcess::CreateWithStackOverride(), 2*KDefaultStackSize 
// stack and start it
// - it passes with KNumberOfUnfitIteration DummyRecursion.
// ETestProcess6: Create a process with RProcess::CreateWithStackOverride(), -2*KDefaultStackSize 
// stack.
// - the process creation interrupted with KErrArgument error code
// ETestProcess7: Create a process with RProcess::CreateWithStackOverride(), KDefaultStackSize/2 
// stack and start it
// - it passes with KNumberOfFitIteration DummyRecursion, because the process will be 
// create with (App. image) default stack size.
// Platforms/Drives/Compatibility:
// It is won't work on emulator, because the unlimited stack size.
// Assumptions/Requirement/Pre-requisites:
// No
// Failures and causes:
// Failures of this test will indicate defects in the implementation of CreateWithStackOverride().
// Base Port information:
// No?
// 
//

#include <e32test.h>
#include "u32std.h"
#include <u32hal.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_STACKSIZE"));
LOCAL_D RTest test2(_L("*** T_STACKSIZE SLAVE ***"));

// We are here making the assumption that one quarter of the stack should be sufficient
// to accommodate for the overhead generated by the recursive calls as well as
// unpredictable compiler optimisations.
const TInt KStackGobbleSize = KDefaultStackSize / 4 - 8;
const TInt KNumberOfFitIteration = 3;
const TInt KNumberOfUnfitIteration = 4;

const TInt KImageStackSize = 0x2000; // Test app image stack size (epocstacksize in MMP file)

enum TTestProcessFunctions
	{
	ETestProcess1,
	ETestProcess2,
	ETestProcess3,
	ETestProcess4,
	ETestProcess5,
	ETestProcess6,
	ETestProcess7,
	};

class RTestProcess : public RProcess
	{
public:
	TInt Create(TTestProcessFunctions aFunction, TInt aStackSize=0, TInt aArg1=-1,TInt aArg2=-1);
	};

TInt RTestProcess::Create(TTestProcessFunctions aFunction, TInt aStackSize, TInt aArg1,TInt aArg2)
	{
	
	test.Printf(_L("RTestProcess::Create started with stack size:%d bytes\n"), aStackSize);
	if(aArg1==-1)
		aArg1 = RProcess().Id();
	TBuf<512> commandLine;
	commandLine.Num((TInt)aFunction);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg1);
	commandLine.Append(_L(" "));
	commandLine.AppendNum(aArg2);

	TFileName filename(RProcess().FileName());
	TInt pos=filename.LocateReverse(TChar('\\'));
	filename.SetLength(pos+1);
	filename+=_L("T_STACKSIZE.EXE");
	
	TInt r=KErrNone;
	switch (aFunction)
		{
		case ETestProcess1:
			test.Printf(_L("Call original Create.\n"));
			r = RProcess::Create(filename,commandLine); 
			break;
			
		case ETestProcess2:
			test.Printf(_L("Call CreateWithStackOverride\n"));	
			r = RProcess::CreateWithStackOverride(filename, commandLine, TUidType(), aStackSize, EOwnerProcess);
			break;
			
		case ETestProcess3:
			test.Printf(_L("Call original Create.\n"));
			r = RProcess::Create(filename,commandLine); 
			break;
			
		case ETestProcess4:
			test.Printf(_L("Call CreateWithStackOverride\n"));	
			r = RProcess::CreateWithStackOverride(filename, commandLine, TUidType(), aStackSize, EOwnerProcess);
			break;
			
		case ETestProcess5:
			test.Printf(_L("Call CreateWithStackOverride\n"));	
			r = RProcess::CreateWithStackOverride(filename, commandLine, TUidType(), aStackSize, EOwnerProcess);
			break;
			
		case ETestProcess6:
			test.Printf(_L("Call CreateWithStackOverride\n"));	
			r = RProcess::CreateWithStackOverride(filename, commandLine, TUidType(), aStackSize, EOwnerProcess);
			break;
			
		case ETestProcess7:
			test.Printf(_L("Call CreateWithStackOverride\n"));	
			r = RProcess::CreateWithStackOverride(filename, commandLine, TUidType(), aStackSize, EOwnerProcess);
			break;

		default:
			User::Panic(_L("T_STACKSIZE"),1);
			
		}

	test.Printf(_L("RTestProcess::Create end.\n"));
	return r;
	}


TInt DummyRecursion(TInt aA)
	{
	TBuf8<KStackGobbleSize> gobble;	// gobble 1/4 of the stack
	gobble.Append(TChar(aA));	// stop compiler optimising out gobble buffer
	if (aA <= 1)
		return aA;
	return 1 + DummyRecursion(aA-1);
	}

// Make these global so we don't push too much stuff on the stack
TThreadStackInfo ThreadStackInfo;
_LIT(KStackSizeText, "Stack size is %d bytes\n");

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;
	RThread().StackInfo(ThreadStackInfo);

	switch(aTestNum)
		{
	case ETestProcess1:
		{
		test2.Printf(_L("ETestProcess1 started with %d recursion...\n"), KNumberOfFitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KDefaultStackSize);
#endif
		test2(DummyRecursion(KNumberOfFitIteration)==KNumberOfFitIteration);
		break;
		}
		
	case ETestProcess2:
		{
		test2.Printf(_L("ETestProcess2 started with %d recursion...\n"), KNumberOfFitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KDefaultStackSize);
#endif
		test2(DummyRecursion(KNumberOfFitIteration)==KNumberOfFitIteration);
		break;
		}
		
	case ETestProcess3:
		{
		test2.Printf(_L("ETestProcess3 started with %d recusion...\n"), KNumberOfUnfitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KDefaultStackSize);
#endif
		test2(DummyRecursion(KNumberOfUnfitIteration)==KNumberOfUnfitIteration);
		break;
		}
		
	case ETestProcess4:
		{
		test2.Printf(_L("ETestProcess4 started with %d recursion...\n"), KNumberOfUnfitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KDefaultStackSize);
#endif
		test2(DummyRecursion(KNumberOfUnfitIteration)==KNumberOfUnfitIteration);
		break;
		}
		
	case ETestProcess5:
		{
		test2.Printf(_L("ETestProcess5 started with %d recursion...\n"), KNumberOfUnfitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KDefaultStackSize * 2);
#endif
		test2(DummyRecursion(KNumberOfUnfitIteration)==KNumberOfUnfitIteration);
		break;
		}
		
	case ETestProcess6:
		{
		test2(EFalse); // Process creation should have failed
		}

	case ETestProcess7:
		{
		test2.Printf(_L("ETestProcess7 started with %d recursion\n"), KNumberOfFitIteration);
		test2.Printf(KStackSizeText, ThreadStackInfo.iBase - ThreadStackInfo.iLimit);
#ifndef __WINS__
		test2((TInt) (ThreadStackInfo.iBase - ThreadStackInfo.iLimit) == KImageStackSize); // Should default to stack size set in image header
		if (KImageStackSize == KDefaultStackSize) // If this is not the case, results can be a bit unpredictable
			{
			test2(DummyRecursion(KNumberOfFitIteration)==KNumberOfFitIteration);
			}
#endif
		break;
		}
		
	default:
		User::Panic(_L("T_STACKSIZE"),1);
		}

	test2.Printf(_L("\n\t finished.\n"));
	
	return KErrNone;
	}

GLDEF_C TInt E32Main()
    {
    
	
	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		test2.Title();
		test2.Start(_L("Slave process started..."));
		TInt function = -1;
		TInt arg1 = -1;
		TInt arg2 = -1;
		TLex lex(cmd);
		lex.Val(function);
		lex.SkipSpace();
		lex.Val(arg1);
		lex.SkipSpace();
		lex.Val(arg2);
		int r = DoTestProcess(function,arg1,arg2); 
		test2.End();
		return r;
		}

	test.Title();

	RTestProcess rogueP;
	TRequestStatus rendezvous;

	test.Start(_L("Create process with original Create and default stack size"));
	TInt r = rogueP.Create(ETestProcess1, KDefaultStackSize);
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType() );
	test(rogueP.ExitType()==EExitKill);
	CLOSE_AND_WAIT(rogueP);
	
	test.Next(_L("Create process with CreateWithStackOverride and default stack size"));
	r = rogueP.Create(ETestProcess2, KDefaultStackSize);
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType() );
	test(rogueP.ExitType()==EExitKill);
	CLOSE_AND_WAIT(rogueP);
	
	test.Next(_L("Create process with original Create and default stack size"));
	r = rogueP.Create(ETestProcess3, KDefaultStackSize);
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType() );
#if !defined(__WINS__)
	test(rogueP.ExitType()==EExitPanic);
#else
	test(rogueP.ExitType()==EExitKill);
#endif
	CLOSE_AND_WAIT(rogueP);
	
	test.Next(_L("Create process with CreateWithStackOverride and default stack size"));
	r = rogueP.Create(ETestProcess4, KDefaultStackSize);
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType());
#if !defined(__WINS__)
	test(rogueP.ExitType()==EExitPanic);
#else
	test(rogueP.ExitType()==EExitKill);
#endif
	CLOSE_AND_WAIT(rogueP);

	test.Next(_L("Create process with CreateWithStackOverride and 2 * KDefaultStackSize stack size"));
	r = rogueP.Create(ETestProcess5, 2 * KDefaultStackSize );
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType() );
	test(rogueP.ExitType()==EExitKill);
	CLOSE_AND_WAIT(rogueP);

#if !defined(__WINS__)
	test.Next(_L("Create process with CreateWithStackOverride and negative stack size"));
	r = rogueP.Create(ETestProcess6, - 2 * KDefaultStackSize );
	test(r==KErrArgument);
#endif

	test.Next(_L("Create process with CreateWithStackOverride and KImageStackSize/2 stack size"));
	r = rogueP.Create(ETestProcess7, KImageStackSize / 2 );
	test(r==KErrNone);
	rogueP.Rendezvous(rendezvous);
	rogueP.Resume();
	User::WaitForRequest(rendezvous);
	test.Printf(_L("ExitType:%d\n"),rogueP.ExitType() );
	test(rogueP.ExitType()==EExitKill);
	CLOSE_AND_WAIT(rogueP);
	
	test.Printf(_L("Test finished.\n"));
	test.End();

	return(0);
    }

