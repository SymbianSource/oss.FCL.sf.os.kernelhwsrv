// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_sprocess.cpp
// Overview:
// Test the platform security aspects of the RProcess class
// API Information:
// RProcess
// Details:
// - Test SetJustInTime on the current process, a new un-Resumed process
// and between processes. Verify results are as expected.
// - Test process renaming and verify results are as expected.
// - Test killing, terminating and panicking different processes, verify
// results are as expected.
// - Test resuming a process from a different process, verify results.
// - Test setting process priority in a variety of ways, verify results
// are as expected.
// - Test the RProcess SetType(), SetProtected(), CommandLineLength(), 
// CommandLine(), SetSystem(), SetOwner() and Owner() methods. Verify
// results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_SPROCESS"));

_LIT(KSyncMutext,"T_SPROCESS-sync-mutex");
RMutex SyncMutex;

void Wait()
	{
	RMutex syncMutex;
	if(syncMutex.OpenGlobal(KSyncMutext)!=KErrNone)
		User::Invariant();
	syncMutex.Wait();
	syncMutex.Signal();
	syncMutex.Close();
	}

enum TTestProcessFunctions
	{
	ETestProcessNull,
	ETestProcessSetJustInTime,
	ETestProcessKill,
	ETestProcessTerminate,
	ETestProcessPanic,
	ETestProcessKillSelf,
	ETestProcessTerminateSelf,
	ETestProcessPanicSelf,
	ETestProcessResume,
	ETestProcessPriority,
	ETestProcessPriorityControlOff,
	ETestProcessPriorityControlOn,
	ETestProcessPriorityControlOnAndLowPriority,
	ETestProcessSetPriority,
	};

#include "testprocess.h"

_LIT(KTestPanicCategory,"TEST PANIC");
_LIT(KTestProcessName,"TestName");


TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	RTestProcess process;
	TInt r;

	switch(aTestNum)
		{

	case ETestProcessNull:
		Wait();
		return KErrNone;

	case ETestProcessSetJustInTime:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.SetJustInTime(!process.JustInTime()); // Should panic us
		return r;
		}

	case ETestProcessResume:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.Resume(); // Should panic us
		return r;
		}

	case ETestProcessKill:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.Kill(999);
		return r;
		}

	case ETestProcessTerminate:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.Terminate(999);
		return r;
		}

	case ETestProcessPanic:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.Panic(KTestPanicCategory,999);
		return r;
		}

	case ETestProcessKillSelf:
		{
		RProcess().Kill(999);
		return KErrNone;
		}

	case ETestProcessTerminateSelf:
		{
		RProcess().Terminate(999);
		return KErrNone;
		}

	case ETestProcessPanicSelf:
		{
		RProcess().Panic(KTestPanicCategory,999);
		return KErrNone;
		}

	case ETestProcessSetPriority:
		{
		r = process.Open(aArg1);
		if(r==KErrNone)
			process.SetPriority((TProcessPriority)aArg2);
		return r;
		}


	case ETestProcessPriority:
		{
		r = process.Open(aArg1);
		if(r!=KErrNone)
			return r;
		TProcessPriority priority;
		priority = process.Priority();
		process.SetPriority(EPriorityLow);
		if(process.Priority()!=priority) // priority shouldn't have changed
			return KErrGeneral;
		process.SetPriority(EPriorityBackground);
		if(process.Priority()!=priority) // priority shouldn't have changed
			return KErrGeneral;
		process.SetPriority(EPriorityForeground);
		if(process.Priority()!=priority) // priority shouldn't have changed
			return KErrGeneral;
		return KErrNone;
		}

	case ETestProcessPriorityControlOnAndLowPriority:
		RProcess().SetPriority(EPriorityLow);
		// fall through...
	case ETestProcessPriorityControlOn:
		User::SetPriorityControl(ETrue);
		// fall through...
	case ETestProcessPriorityControlOff:
		RProcess::Rendezvous(0);
		Wait();
		return KErrNone;

	default:
		User::Panic(_L("T_SPROCESS"),1);
		}

	return KErrNone;
	}



void TestProcessForPlatformSecurityTrap(TTestProcessFunctions aFunction)
	{
	TRequestStatus logonStatus2;
	RTestProcess process;
	process.Create(~0u,aFunction,RProcess().Id(),EPriorityAbsoluteLow);
	process.Logon(logonStatus2);
	process.Resume();
	User::WaitForRequest(logonStatus2);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus2==EPlatformSecurityTrap);
	}



void TestSetJustInTime()
	{
	RTestProcess process;
	RTestProcess process2;
	TRequestStatus logonStatus;

	test.Start(_L("Test SetJustInTime on current process"));
	TBool jit = process.JustInTime();
	process.SetJustInTime(!jit);
	test((process.JustInTime()!=EFalse)!=(jit!=EFalse));
	process.SetJustInTime(jit);
	test((process.JustInTime()!=EFalse)==(jit!=EFalse));

	test.Next(_L("Test SetJustInTime on a new un-Resumed process"));
	process.RProcess::Create(RProcess().FileName(),_L(""));
	jit = process.JustInTime();
	process.SetJustInTime(!jit);
	test((process.JustInTime()!=EFalse)!=(jit!=EFalse));
	process.SetJustInTime(jit);
	test((process.JustInTime()!=EFalse)==(jit!=EFalse));
	process.Kill(0);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try other process using SetJustInTime on our created process"));
	process2.Create(0,ETestProcessNull);
	process.Create(~0u,ETestProcessSetJustInTime,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);
	process2.Kill(0);
	CLOSE_AND_WAIT(process2);

	test.Next(_L("Try other process to using SetJustInTime on us"));
	process.Create(~0u,ETestProcessSetJustInTime);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestRename()
	{
	TName name;

	test.Start(_L("Renaming the current process with User::RenameProcess"));
	name = RProcess().Name();
	name.SetLength(KTestProcessName().Length());
	test(name.CompareF(KTestProcessName)!=0);
	User::RenameProcess(KTestProcessName);
	name = RProcess().Name();
	name.SetLength(KTestProcessName().Length());
	test(name.CompareF(KTestProcessName)==0);


	test.End();
	}



void TestKill()
	{
	RTestProcess process;
	RTestProcess process2;
	TRequestStatus logonStatus;
	TRequestStatus logonStatus2;

	process2.Create(0,ETestProcessNull);
	process2.Logon(logonStatus2);

	// Test RProcess::Kill()

	test.Start(_L("Test killing an un-resumed process created by us"));
	process.Create(0,ETestProcessNull);
	process.Logon(logonStatus);
	process.Kill(999);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try killing un-resumed process not created by self"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessKill,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the process should still be alive
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a process killing itself"));
	process.Create(0,ETestProcessKillSelf);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try killing running process"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessKill,RProcess().Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	// Test RProcess::Teminate()

	test.Next(_L("Test terminating an un-resumed process created by us"));
	process.Create(0,ETestProcessNull);
	process.Logon(logonStatus);
	process.Terminate(999);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try terminating un-resumed process not created by self"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessTerminate,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the process should still be alive
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a process terminating itself"));
	process.Create(0,ETestProcessTerminateSelf);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try terminating running process"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessTerminate,RProcess().Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	// Test RProcess::Panic()

	test.Next(_L("Test panicking an un-resumed process created by us"));
	process.Create(0,ETestProcessNull);
	process.Logon(logonStatus);
	process.Panic(KTestPanicCategory,999);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try panicking un-resumed process not created by self"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessPanic,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	test(logonStatus2==KRequestPending); // the process should still be alive
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test a process panicking itself"));
	process.Create(0,ETestProcessPanicSelf);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try panicking running process"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessPanic,RProcess().Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	// 

	test(logonStatus2==KRequestPending); // the process should still be alive
	process2.Resume();
	User::WaitForRequest(logonStatus2);
	test(logonStatus2==KErrNone);
	CLOSE_AND_WAIT(process2);

	// Checks with ECapabilityPowerMgmt

	test.Next(_L("Test kill running process ECapabilityPowerMgmt"));
	process2.Create(0,ETestProcessNull);
	process2.Logon(logonStatus2);
	process.Create((1<<ECapabilityPowerMgmt),ETestProcessKill,process2.Id());
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process2.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);
	User::WaitForRequest(logonStatus2);
	test(process2.ExitType()==EExitKill);
	test(logonStatus2==999);
	process2.Close();
	SyncMutex.Signal();

	test.Next(_L("Test terminating running process ECapabilityPowerMgmt"));
	process2.Create(0,ETestProcessNull);
	process2.Logon(logonStatus2);
	process.Create((1<<ECapabilityPowerMgmt),ETestProcessTerminate,process2.Id());
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process2.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);
	User::WaitForRequest(logonStatus2);
	test(process2.ExitType()==EExitTerminate);
	test(logonStatus2==999);
	CLOSE_AND_WAIT(process2);
	SyncMutex.Signal();

	test.Next(_L("Test panicking running process ECapabilityPowerMgmt"));
	process2.Create(0,ETestProcessNull);
	process2.Logon(logonStatus2);
	process.Create((1<<ECapabilityPowerMgmt),ETestProcessPanic,process2.Id());
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process2.Resume();
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);
	User::WaitForRequest(logonStatus2);
	test(process2.ExitType()==EExitPanic);
	test(logonStatus2==999);
	CLOSE_AND_WAIT(process2);
	SyncMutex.Signal();

	//

	test.End();
	}



void TestResume()
	{
	RTestProcess process;
	RTestProcess process2;
	TRequestStatus logonStatus;
	TRequestStatus logonStatus2;

	test.Start(_L("Try to get another process to resume one we've created"));
	process2.Create(0,ETestProcessNull);
	process2.Logon(logonStatus2);
	process.Create(~0u,ETestProcessResume,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	User::After(1*1000*1000); // Give time for process to run (if it had been resumed)...
	test(logonStatus2==KRequestPending); // It shouldn't have, so logon will be pending
	process2.Kill(0);
	CLOSE_AND_WAIT(process2);

//	test.Next(_L("Test resuming a process we've created"));
//
//  No code for this, because this whole test program wouldn't work if this wasn't OK
//

	test.End();
	}



void TestSetPriority()
	{
	RTestProcess process;
	RTestProcess process2;
	TProcessPriority priority;
	TRequestStatus rendezvousStatus;
	TRequestStatus logonStatus;

	test.Start(_L("Test changing our own process priority"));
	priority = process.Priority();
	process.SetPriority(EPriorityLow);
	test(process.Priority()==EPriorityLow);
	process.SetPriority(EPriorityBackground);
	test(process.Priority()==EPriorityBackground);
	process.SetPriority(EPriorityForeground);
	test(process.Priority()==EPriorityForeground);
	process.SetPriority(priority);

	test.Next(_L("Test changing unresumed process priority (which we created)"));
	process.Create(0,ETestProcessNull);
	priority = process.Priority();
	process.SetPriority(EPriorityLow);
	test(process.Priority()==EPriorityLow);
	process.SetPriority(EPriorityBackground);
	test(process.Priority()==EPriorityBackground);
	process.SetPriority(EPriorityForeground);
	test(process.Priority()==EPriorityForeground);
	process.SetPriority(priority);
	process.Kill(0);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try other process changing the priority of our created process"));
	process2.Create(0,ETestProcessNull);
	process.Create(~0u,ETestProcessPriority,process2.Id());
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);
	process2.Kill(0);
	CLOSE_AND_WAIT(process2);

	test.Next(_L("Try changing other process's priority (no priority-control enabled)"));
	process.Create(~0u,ETestProcessPriorityControlOff);
	process.Rendezvous(rendezvousStatus);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus); // Process has started
	priority = process.Priority();
	TInt result = process.SetPriority(EPriorityLow);
	test(result == KErrPermissionDenied);
	test(process.Priority()==priority); // priority shouldn't have changed
	process.SetPriority(EPriorityBackground);
	test(result == KErrPermissionDenied);
	test(process.Priority()==priority); // priority shouldn't have changed
	process.SetPriority(EPriorityForeground);
	test(result == KErrPermissionDenied);
	test(process.Priority()==priority); // priority shouldn't have changed
	test(logonStatus==KRequestPending); // wait for process to end
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try changing other process's priority (priority-control enabled)"));
	process.Create(~0u,ETestProcessPriorityControlOn);
	process.Rendezvous(rendezvousStatus);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus); // Process has started
	priority = process.Priority();
	result = process.SetPriority(EPriorityForeground);
	test(result == KErrNone);
	test(process.Priority()==EPriorityForeground);
	result = process.SetPriority(EPriorityBackground);
	test(result == KErrNone);
	test(process.Priority()==EPriorityBackground);
	result = process.SetPriority(EPriorityForeground);
	test(result == KErrNone);
	test(process.Priority()==EPriorityForeground);
	result = process.SetPriority(EPriorityLow);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityForeground); // should still be foreground priority
	result = process.SetPriority(EPriorityHigh);
	test(result == KErrNone);
	test(process.Priority()==EPriorityHigh);
	result = process.SetPriority(priority);
	test(result == KErrNone);
	test(logonStatus==KRequestPending); // wait for process to end
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try changing other process's priority (priority-control enabled and low priority)"));
	process.Create(~0u,ETestProcessPriorityControlOnAndLowPriority);
	process.Rendezvous(rendezvousStatus);
	process.Logon(logonStatus);
	SyncMutex.Wait();
	process.Resume();
	User::WaitForRequest(rendezvousStatus); // Process has started
	test(process.Priority()==EPriorityLow);
	result = process.SetPriority(EPriorityForeground);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityLow);
	result = process.SetPriority(EPriorityBackground);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityLow);
	result = process.SetPriority(EPriorityForeground);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityLow);
	result = process.SetPriority(EPriorityLow);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityLow);
	result = process.SetPriority(EPriorityHigh);
	test(result == KErrPermissionDenied);
	test(process.Priority()==EPriorityLow);
	test(logonStatus==KRequestPending); // wait for process to end
	SyncMutex.Signal();
	User::WaitForRequest(logonStatus);
	CLOSE_AND_WAIT(process);

	test.End();
	}



GLDEF_C TInt E32Main()
    {
	TBuf16<512> cmd;
	User::CommandLine(cmd);
	if(cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TInt function = -1;
		TInt arg1 = -1;
		TInt arg2 = -1;
		TLex lex(cmd);

		lex.Val(function);
		lex.SkipSpace();
		lex.Val(arg1);
		lex.SkipSpace();
		lex.Val(arg2);
		return DoTestProcess(function,arg1,arg2);
		}

	test.Title();

	if((!PlatSec::ConfigSetting(PlatSec::EPlatSecProcessIsolation))||(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement)))
		{
		test.Start(_L("TESTS NOT RUN - PlatSecProcessIsolation is not enforced"));
		test.End();
		return 0;
		}

	test(SyncMutex.CreateGlobal(KSyncMutext)==KErrNone);
	
	test.Start(_L("Test SetJustInTime"));
	TestSetJustInTime();

	test.Next(_L("Test Rename"));
	TestRename();

	test.Next(_L("Test Kill, Panic and Teminate"));
	TestKill();

	test.Next(_L("Test Resume"));
	TestResume();

	test.Next(_L("Test SetPriority"));
	TestSetPriority();


	SyncMutex.Close();
	test.End();

	return(0);
    }

