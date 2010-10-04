// Copyright (c) 2005-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\t_debugapi.cpp
// User-side harness for LDD-based debug agent that checks debug API 
// interface provided by kernel extension kdebug.dll (ARMv5) or kdebugv6 (ARMv6).
// It uses debug port to print the list of processes, threads, etc.
// Usage: t_debugapi [process] [thread] [chunk] [ipaccess]
// Performs all steps if there are no input arguments.
// The test is automated (does not require any input argument or manual assistance, but needs
// non-standard image file that includes kdebug.dll (ARMv5 based target) or kdebugV6.dll  
// (ARMv6 based target). It can be achieved by adding: #define STOP_MODE_DEBUGGING in .oby/iby file.
// Supported and tested on H2 (ARMv5) and integrator_1136 (ARMv6) platforms.
// It requires D_DEBUGAPI.DLL as well.
// Using debug interface only, it completes (prints) the list of:
// - processes;
// - threads;
// - chunks;
// in the system. On multiple-memory-model based target (ARMv6 architecture), 
// it also reads from address space of another process. This part is not checked 
// on moving-memory-model target (ARMv5) (e.g. always passes).
// Note: The test may cause system fail on ARM1136(r0p2) with L210 cache due to Erratum 317041.
// In that case, uncomment two relevant lines in DDebugAPIChecker::ReadFromOtherProcessArmv6 (in
// d_debugapi.cia) and rebuild d_debugapi.ldd.
// 
//

//! @file t_debugapi.h
//! @SYMTestCaseID KBASE/T_DEBUGAPI
//! @SYMTestType UT
//! @SYMTestCaseDesc
//! @SYMREQ PREQ835
//! @SYMTestActions
//! @SYMTestExpectedResults Test program completes with no errors.
//! @SYMTestPriority Low
//! @SYMTestStatus Defined


#include <e32test.h>
#include "d_debugapi.h"

RTest test(_L("T_DebugAPI"));

_LIT(KProcessName,"t_DebugAPI.exe");
_LIT(KProcess,"Process");
_LIT(KChunk,"Chunk");
_LIT(KThread,"Thread");
_LIT(KIPAccess,"IPAccess");
_LIT(KIPAccessStep2,"IPAccessStep2");

TBuf<64> command;

//The main program for the first instance of the process.
void Main()
	{
	RDebugAPIChecker debugAPI;
	TInt r;
	TBool checkAll = EFalse;

	if (command.Length() == 0)	
		checkAll = ETrue;

	r = User::LoadLogicalDevice(_L("D_DebugAPI.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);
	test (debugAPI.Open() == KErrNone);

	if (checkAll || command.FindF(KProcess) >= 0)
		{
		test.Next(_L("Printing process info"));
		test(debugAPI.Process() == KErrNone);
		}

	if (checkAll || command.FindF(KChunk) >= 0)
		{
		test.Next(_L("Printing chunk info"));
		test(debugAPI.Chunk() == KErrNone);
		}

	if (checkAll || command.FindF(KThread) >= 0)
		{
		test.Next(_L("Printing thread info"));
		test(debugAPI.Thread() == KErrNone);
		}

	if (checkAll || command.FindF(KIPAccess) >= 0)
		{
		test.Next(_L("KIPAccess"));

		RProcess process;
		TRequestStatus status;
		
		//The other process will try to read this variable.
		TInt probeVariable = 0x55555555;
		TUint id = process.Id();
		TBuf<64> command;
		command.Format(_L("%08x %08x %08x IPAccessStep2"), id, &probeVariable, probeVariable);

		test(process.Create(KProcessName, command) == KErrNone);
		process.Logon(status);
		process.Resume();
		User::WaitForRequest(status);
		test(process.ExitType() == EExitKill);
		test(process.ExitReason() == KErrNone);
		process.Close();

		//Now try another value.
		probeVariable = 0xaaaaaaaa;
		command.Format(_L("%08x %08x %08x IPAccessStep2"), id, &probeVariable, probeVariable);

		test(process.Create(KProcessName, command) == KErrNone);
		process.Logon(status);
		process.Resume();
		User::WaitForRequest(status);
		test(process.ExitType() == EExitKill);
		test(process.ExitReason() == KErrNone);
		process.Close();
		}

	debugAPI.Close();
	r = User::FreeLogicalDevice(KTestLddName);
	test(r == KErrNone || r == KErrNotFound);
	}

/**
The main program for the second instance of the process.
We need two processes two check inter-process data access.
*/
void SecondProcessMain()
	{
	RDebugAPIChecker debugAPI;
	TInt r;
	
	r = User::LoadLogicalDevice(_L("D_DebugAPI.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);
	test (debugAPI.Open() == KErrNone);

	if (command.FindF(KIPAccessStep2) >= 0)
		{
		RDebugAPIChecker::IPAccessArgs args;

		TPtrC ptr = command.Mid(0,8);
		TLex lex(ptr);
		lex.Val(args.iProcessID, EHex);

		ptr.Set(command.Mid(9,8));
		lex.Assign(ptr);
		lex.Val(args.iAddress, EHex);

		ptr.Set(command.Mid(18,8));
		lex.Assign(ptr);
		lex.Val(args.iValue, EHex);

		r = debugAPI.IPAccess(&args);
		test(r == KErrNone || r==KErrNotSupported);
		}

	debugAPI.Close();
	r = User::FreeLogicalDevice(KTestLddName);
	}

TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;

	User::CommandLine(command);

	if (command.FindF(KIPAccessStep2) < 0) //The second process is recognized by the specific input parameter
		{
		//This is the first instance of the running process.
		test.Start(_L("Testing Debug API"));
		Main();
		test.End();
		}
	else
		{
		//This is the second instance of the running process.
		SecondProcessMain();
		}

	__UHEAP_MARKEND;
	return 0;
	}
