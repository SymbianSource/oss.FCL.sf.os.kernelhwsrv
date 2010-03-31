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
// e32test\secure\t_suser.cpp
// Overview:
// Test the platform security aspects of the User and UserSvr classes.
// API Information:
// User, UserSvr
// Details:
// - Attempt to get and set the machine configuration with and without 
// the proper privileges. Verify results are as expected.
// - Test various critical threads and processes with different capabilities,
// verify results are as expected.
// - Verify that the SetPriorityControl and PriorityControl methods work as
// expected.
// - Verify that the UserSvr::CaptureEventHook, ReleaseEventHook, RequestEvent,
// and RequestEventCancel work as expected.
// - Test handled and unhandled exceptions work as expected.
// - Test ResetInactivityTime() with different capabilities. Verify results.
// - Test SetHomeTime() with different capabilities. Verify results.
// - Test SetMemoryThresholds() with different capabilities. Verify results.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <nkern/nk_trace.h>
#include <e32hal.h>
#include <hal.h>

LOCAL_D RTest test(_L("T_SUSER"));

_LIT(KSyncSemaphoreName,"T_SUSER-SyncSemaphore");
RSemaphore SyncSemaphore;


void SlaveWait()
	{
	RSemaphore sem;
	if(sem.OpenGlobal(KSyncSemaphoreName,EOwnerThread)!=KErrNone)
		User::Invariant();
	sem.Wait();
	sem.Close();
	}


void SignalSlave()
	{
	RSemaphore sem;
	if(sem.OpenGlobal(KSyncSemaphoreName,EOwnerThread)!=KErrNone)
		User::Invariant();
	sem.Signal();
	sem.Close();
	}


class RTestThread : public RThread
	{
public:
	void Create(TThreadFunction aFunction,TInt aArg=0);
	};

TInt GetCriticalValue(TAny* aArg)
	{
	TUint id = (TUint)aArg;
	RThread thread;
	TInt r = thread.Open(TThreadId(id));
	if (r == KErrNone)
		r = (TInt)User::Critical(thread);
	thread.Close();
	return r;
	}

void SetThreadCritical(User::TCritical aCritical)
	{
	// set thread critical as specified
	if(User::SetCritical(aCritical)!=KErrNone)
		User::Invariant();
	// check critical value was as we set
	if(User::Critical()!=aCritical) 
		User::Invariant();
	// check from another thread
	RTestThread thread;
	thread.Create(GetCriticalValue, (TUint)thread.Id());
	TRequestStatus logonStatus;
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	if (logonStatus!=(TInt)aCritical)
		User::Invariant();
	}

TInt TestThreadProcessCritical(TAny* aArg)
	{
	// check thread is not critical
	if(User::Critical()!=User::ENotCritical)
		User::Invariant();
	// set thread as process critical
	SetThreadCritical(User::EProcessCritical);
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	// Kill this thread which should also kill the process
	switch((TExitType)(TInt)aArg)
		{
		case EExitKill:
			RThread().Kill(999);
			break;
		case EExitTerminate:
			RThread().Terminate(999);
			break;
		case EExitPanic:
			User::Panic(_L("TestPanic"),999);
			break;
		default:
			break;
		}
	return KErrNone;
	}

TInt TestThreadSystemCritical(TAny*)
	{
	User::TCritical critical = User::Critical();
	// check thread is not already system critical
	if(User::Critical()==User::ESystemCritical)
		User::Invariant();
	// set thread as system critical
	SetThreadCritical(User::ESystemCritical);
	// Can't test system critical thread dying so put back to normal and end
	SetThreadCritical(critical);
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	return KErrNone;
	}

TInt TestThreadSystemPermanent(TAny*)
	{
	User::TCritical critical = User::Critical();
	// check thread is not already system permanent
	if(User::Critical()==User::ESystemPermanent)
		User::Invariant();
	// set thread as system permanent
	SetThreadCritical(User::ESystemPermanent);
	// Can't test system permanent thread dying so put back to normal and end
	SetThreadCritical(critical);
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	return KErrNone;
	}

TInt TestThreadSystemProcessCritical(TAny*)
	{
	User::TCritical critical = User::ProcessCritical();
	// check thread is not already system critical
	if(User::ProcessCritical()==User::ESystemCritical)
		User::Invariant();
	// set thread as system critical
	if(User::SetProcessCritical(User::ESystemCritical)!=KErrNone)
		User::Invariant();
	// check critical value was as we set
	if(User::ProcessCritical()!=User::ESystemCritical) 
		User::Invariant();
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	// wait for main test thread to tell us to continue...
	SlaveWait();
	// Can't test system critical thread dying so put back to normal and end
	if(User::SetProcessCritical(critical)!=KErrNone)
		User::Invariant();
	if(User::ProcessCritical()!=critical) 
		User::Invariant();
	return KErrNone;
	}

TInt TestThreadSystemProcessPermanent(TAny*)
	{
	User::TCritical critical = User::ProcessCritical();
	// check thread is not already system permanent
	if(User::ProcessCritical()==User::ESystemPermanent)
		User::Invariant();
	// set thread as system permanent
	if(User::SetProcessCritical(User::ESystemPermanent)!=KErrNone)
		User::Invariant();
	// check critical value was as we set
	if(User::ProcessCritical()!=User::ESystemPermanent) 
		User::Invariant();
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	// wait for main test thread to tell us to continue...
	SlaveWait();
	// Can't test system permanent thread dying so put back to normal and end
	if(User::SetProcessCritical(critical)!=KErrNone)
		User::Invariant();
	if(User::ProcessCritical()!=critical) 
		User::Invariant();
	return KErrNone;
	}

TInt TestThreadTraceKallthreadssystem(TAny*)
	{
	TUint32 mask = UserSvr::DebugMask(DEBUGMASKWORD2);
	// check thread does not already have KALLTHREADSSYSTEM bit set
	if (mask & (1 << (KALLTHREADSSYSTEM%32)))
		User::Invariant();
	// set KALLTHREADSSYSTEM bit
	User::SetDebugMask(mask | (1 << (KALLTHREADSSYSTEM%32)), DEBUGMASKWORD2);
	// check KALLTHREADSSYSTEM bit was as we set
	if(!(UserSvr::DebugMask(DEBUGMASKWORD2) & (1 << (KALLTHREADSSYSTEM%32)))) 
		User::Invariant();
	// restore original mask
	User::SetDebugMask(mask, DEBUGMASKWORD2);
	if(UserSvr::DebugMask(DEBUGMASKWORD2) & (1 << (KALLTHREADSSYSTEM%32))) 
		User::Invariant();
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	return KErrNone;
	}

TInt TestThreadAllThreadsCritical(TAny* aArg)
	{
	// check that thread was created process critical 
	if(User::Critical()!=User::EProcessCritical)
		User::Invariant();
	// complete rendezvous to let test code know we got this far ok
	RProcess::Rendezvous(KErrNone);
	// Kill this thread which should also kill the process
	switch((TExitType)(TInt)aArg)
		{
		case EExitKill:
			RThread().Kill(999);
			break;
		case EExitTerminate:
			RThread().Terminate(999);
			break;
		case EExitPanic:
			User::Panic(_L("TestPanic"),999);
			break;
		default:
			break;
		}
	return KErrNone;
	}

TInt TestAllThreadsCritical(TExitType aExitType)
	{
	// check process does not already have all threads critical
	if(User::ProcessCritical()==User::EAllThreadsCritical)
		User::Panic(_L("TestAllThreadsCritical"),__LINE__);
	// set process as all threads critical
	if(User::SetProcessCritical(User::EAllThreadsCritical)!=KErrNone)
		User::Panic(_L("TestAllThreadsCritical"),__LINE__);
	// check critical value was as we set
	if(User::ProcessCritical()!=User::EAllThreadsCritical)
		User::Panic(_L("TestAllThreadsCritical"),__LINE__);
	// spawn a thread that exits in the specifed way
	RTestThread thread;
	thread.Create(TestThreadAllThreadsCritical,aExitType);
	TRequestStatus logonStatus;
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	return KErrNone;
	}

void RTestThread::Create(TThreadFunction aFunction,TInt aArg)
	{
	TInt r=RThread::Create(_L(""),aFunction,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,(TAny*)aArg);
	test(r==KErrNone);
	}



enum TTestProcessFunctions
	{
	ETestProcessMachineConfigGet,
	ETestProcessMachineConfigSet,
	ETestProcessProcessCriticalNormalEnd,
	ETestProcessProcessCriticalKill,
	ETestProcessProcessCriticalTerminate,
	ETestProcessProcessCriticalPanic,
	ETestProcessAllThreadsCriticalNormalEnd,
	ETestProcessAllThreadsCriticalKill,
	ETestProcessAllThreadsCriticalTerminate,
	ETestProcessAllThreadsCriticalPanic,
	ETestProcessSystemCritical,
	ETestProcessSystemPermanent,
	ETestProcessSystemProcessCritical,
	ETestProcessSystemProcessPermanent,
	ETestProcessCaptureEventHook,
	ETestProcessReleaseEventHook,
	ETestProcessRequestEvent,
	ETestProcessRequestEventCancel,
	ETestProcessSetHomeTime,
	ETestProcessSetMemoryThresholds,
	ETestProcessSetUTCOffset,
	ETestProcessSetUTCTime,
	ETestProcessSetUTCTimeAndOffset,
	ETestProcessTraceKallthreadssystem,
	ETestProcessLocaleSet,
	ETestProcessUserSetCurrencySymbol,
	ETestProcessChangeLocale,
	ETestProcessSaveSystemSettings,
	ETestProcessSetCurrencySymbol,
	ETestProcessAddEventESwitchOff,
	ETestProcessAddEventECaseOpen,
	ETestProcessAddEventECaseClose
	};

#include "testprocess.h"

const TInt KMachineConfigSize = 1024;
TBuf8<KMachineConfigSize> MachineConfig;

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;

	RThread me;
	me.SetPriority(EPriorityLess);
	
	switch(aTestNum)
		{

	case ETestProcessMachineConfigGet:
		{
		TInt size=0;
		TInt r=User::MachineConfiguration(MachineConfig,size);
		if(r!=KErrNone)
			User::Invariant();
		MachineConfig.SetLength(size);
		return KErrNone;
		}

	case ETestProcessMachineConfigSet:
		{
		TInt size=0;
		TInt r=User::MachineConfiguration(MachineConfig,size);
		if(r!=KErrNone)
			User::Invariant();
		MachineConfig.SetLength(size);

		r=User::SetMachineConfiguration(MachineConfig);
		if(r!=KErrNone)
			User::Invariant();
		return KErrNone;
		}

	case ETestProcessProcessCriticalNormalEnd:
		{
		RTestThread thread;
		thread.Create(TestThreadProcessCritical,-1);
		TRequestStatus logonStatus;
		thread.Logon(logonStatus);
		thread.Resume();
		User::WaitForRequest(logonStatus);
		return KErrNone;
		}
		
	case ETestProcessProcessCriticalKill:
		{
		RTestThread thread;
		thread.Create(TestThreadProcessCritical,EExitKill);
		TRequestStatus logonStatus;
		thread.Logon(logonStatus);
		thread.Resume();
		User::WaitForRequest(logonStatus);
		return KErrNone;
		}

	case ETestProcessProcessCriticalTerminate:
		{
		RTestThread thread;
		thread.Create(TestThreadProcessCritical,EExitTerminate);
		TRequestStatus logonStatus;
		thread.Logon(logonStatus);
		thread.Resume();
		User::WaitForRequest(logonStatus);
		return KErrNone;
		}

	case ETestProcessProcessCriticalPanic:
		{
		RTestThread thread;
		thread.Create(TestThreadProcessCritical,EExitPanic);
		TRequestStatus logonStatus;
		thread.Logon(logonStatus);
		thread.Resume();
		User::WaitForRequest(logonStatus);
		return KErrNone;
		}

	case ETestProcessAllThreadsCriticalNormalEnd:
		return TestAllThreadsCritical((TExitType)-1);
		
	case ETestProcessAllThreadsCriticalKill:
		return TestAllThreadsCritical(EExitKill);

	case ETestProcessAllThreadsCriticalTerminate:
		return TestAllThreadsCritical(EExitTerminate);

	case ETestProcessAllThreadsCriticalPanic:
		return TestAllThreadsCritical(EExitPanic);

	case ETestProcessSystemCritical:
		return TestThreadSystemCritical(NULL);

	case ETestProcessSystemPermanent:
		return TestThreadSystemPermanent(NULL);

	case ETestProcessSystemProcessCritical:
		return TestThreadSystemProcessCritical(NULL);

	case ETestProcessSystemProcessPermanent:
		return TestThreadSystemProcessPermanent(NULL);

	case ETestProcessCaptureEventHook:
		UserSvr::CaptureEventHook();
		break;

	case ETestProcessReleaseEventHook:
		UserSvr::ReleaseEventHook();
		break;

	case ETestProcessRequestEvent:
		{
		TRawEventBuf event;
		TRequestStatus status;
		UserSvr::RequestEvent(event,status);
		}
		break;

	case ETestProcessRequestEventCancel:
		UserSvr::RequestEventCancel();
		break;

	case ETestProcessSetHomeTime:
		{
		TTime time;
		time.HomeTime();
		User::SetHomeTime(time);
		}
		break;

	case ETestProcessSetUTCOffset:
		{
		User::SetUTCOffset(0);
		}
		break;

	case ETestProcessSetUTCTime:
		{
		TTime time;
		time.UniversalTime();
		User::SetUTCTime(time);
		}
		break;

	case ETestProcessSetUTCTimeAndOffset:
		{
		TTime time;
		time.UniversalTime();
		User::SetUTCTimeAndOffset(time,0);
		}
		break;

	case ETestProcessSetMemoryThresholds:
		{
		return UserSvr::SetMemoryThresholds(0,KMaxTInt);
		}

	case ETestProcessTraceKallthreadssystem:
		return TestThreadTraceKallthreadssystem(NULL);

	case ETestProcessLocaleSet:
		return TLocale().Set();

	case ETestProcessUserSetCurrencySymbol:
		return User::SetCurrencySymbol(TCurrencySymbol());

	case ETestProcessChangeLocale:
		return UserSvr::ChangeLocale(KNullDesC);

	case ETestProcessSaveSystemSettings:
		{
		TExtendedLocale locale;
		locale.LoadSystemSettings();
		return locale.SaveSystemSettings();
		}

	case ETestProcessSetCurrencySymbol:
		{
		TExtendedLocale locale;
		locale.LoadSystemSettings();
		return locale.SetCurrencySymbol(TCurrencySymbol());
		}

	case ETestProcessAddEventESwitchOff:
		{
		TRawEvent event;
		event.Set(TRawEvent::ESwitchOff);
		return UserSvr::AddEvent(event);
		}

	case ETestProcessAddEventECaseOpen:
		{
		TRawEvent event;
		event.Set(TRawEvent::ECaseOpen);
		return UserSvr::AddEvent(event);
		}

	case ETestProcessAddEventECaseClose:
		{
		TRawEvent event;
		event.Set(TRawEvent::ECaseClose);
		return UserSvr::AddEvent(event);
		}

	default:
		User::Panic(_L("T_SUSER"),1);
		}

	return KErrNone;
	}



void TestMachineConfiguration()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try getting machine-config without ECapabilityReadDeviceData"));
	process.Create(~(1u<<ECapabilityReadDeviceData),ETestProcessMachineConfigGet);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try setting machine-config without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessMachineConfigSet);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test getting machine-config with ECapabilityReadDeviceData"));
	process.Create(1<<ECapabilityReadDeviceData,ETestProcessMachineConfigGet);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setting machine-conig with ECapabilityWriteDeviceData"));
	process.Create((1<<ECapabilityWriteDeviceData)|(1<<ECapabilityReadDeviceData),ETestProcessMachineConfigSet);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.End();
	}


	
void TestSetCritical()
	{
	RTestProcess process;
	TRequestStatus rendezvousStatus;
	TRequestStatus logonStatus;

	test.Start(_L("Test process critical thread exiting normally"));
	process.Create(ETestProcessProcessCriticalNormalEnd);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test process critical thread being killed"));
	process.Create(ETestProcessProcessCriticalKill);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone); // Killed thread doesn't take down process
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test process critical thread being terminated"));
	process.Create(ETestProcessProcessCriticalTerminate);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test process critical thread being panicked"));
	process.Create(ETestProcessProcessCriticalPanic);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test all threads critical process with thread exiting normally"));
	process.Create(ETestProcessAllThreadsCriticalNormalEnd);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::EAllThreadsCritical);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test all threads critical process with thread being killed"));
	process.Create(ETestProcessAllThreadsCriticalKill);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::EAllThreadsCritical);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone); // Killed thread doesn't take down process
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test all threads critical process with thread being terminated"));
	process.Create(ETestProcessAllThreadsCriticalTerminate);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::EAllThreadsCritical);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitTerminate);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test all threads critical process with thread being panicked"));
	process.Create(ETestProcessAllThreadsCriticalPanic);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::EAllThreadsCritical);
	User::WaitForRequest(logonStatus);
	test.Printf(_L("Exit type == %d\n"), process.ExitType());
	test(process.ExitType()==EExitPanic);
	test(logonStatus==999);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try to setup a system critical thread without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessSystemCritical);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==EPlatformSecurityTrap);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setup of a system critical thread with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessSystemCritical);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try to setup a system permanent thread without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessSystemPermanent);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==EPlatformSecurityTrap);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setup of a system permanent thread with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessSystemPermanent);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try to setup a system critical process without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessSystemProcessCritical);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==EPlatformSecurityTrap);
	test(User::ProcessCritical(process) == User::ENotCritical);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setup of a system critical process with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessSystemProcessCritical);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::ESystemCritical);
	SignalSlave();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try to setup a system permanent process without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessSystemProcessPermanent);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==EPlatformSecurityTrap);
	test(User::ProcessCritical(process) == User::ENotCritical);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setup of a system permanent process with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessSystemProcessPermanent);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	test(User::ProcessCritical(process) == User::ESystemPermanent);
	SignalSlave();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try to setup a trace bit KALLTHREADSSYSTEM process without ECapabilityProtServ"));
	process.Create(~(1u<<ECapabilityProtServ),ETestProcessTraceKallthreadssystem);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==EPlatformSecurityTrap);
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test setup of a trace bit KALLTHREADSSYSTEM process with ECapabilityProtServ"));
	process.Create(1<<ECapabilityProtServ,ETestProcessTraceKallthreadssystem);
	process.Logon(logonStatus);
	process.Rendezvous(rendezvousStatus);
	process.Resume();
	User::WaitForRequest(rendezvousStatus);
	test(rendezvousStatus==KErrNone);
	User::WaitForRequest(logonStatus);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);

	test.End();
	}



TUint KTestUid = 0x87654321;

void SetAbsoluteTimeout(RTimer& aTimer, TUint aUs, TRequestStatus& aStatus)
	{
	TTime wakeup;
	wakeup.HomeTime();
	wakeup += TTimeIntervalMicroSeconds(aUs);
	aTimer.At(aStatus, wakeup);
	}

void TestEvents()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try UserSvr::CaptureEventHook()"));
	process.Create(~0u,ETestProcessCaptureEventHook);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::ReleaseEventHook()"));
	process.Create(~0u,ETestProcessReleaseEventHook);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EEventNotCaptured);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::RequestEvent()"));
	process.Create(~0u,ETestProcessRequestEvent);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EEventNotCaptured);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::RequestEventCancel()"));
	process.Create(~0u,ETestProcessRequestEventCancel);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EEventNotCaptured);
	CLOSE_AND_WAIT(process);

	
	
	test.Next(_L("Try UserSvr::AddEvent(ECaseOpen) without ECapabilityPowerMgmt"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessAddEventECaseOpen);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::AddEvent(ECaseOpen) without ECapabilitySwEvent"));
	process.Create(~(1u<<ECapabilitySwEvent),ETestProcessAddEventECaseOpen);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Calling UserSvr::AddEvent(ECaseOpen) with ECapabilityPowerMgmt & ECapabilitySwEvent"));
	process.Create((1u<<ECapabilitySwEvent)|(1u<<ECapabilityPowerMgmt),ETestProcessAddEventECaseOpen);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);



	test.Next(_L("Try UserSvr::AddEvent(ECaseClose) without ECapabilityPowerMgmt"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessAddEventECaseClose);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::AddEvent(ECaseClose) without ECapabilitySwEvent"));
	process.Create(~(1u<<ECapabilitySwEvent),ETestProcessAddEventECaseClose);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Calling UserSvr::AddEvent(ECaseClose) with ECapabilityPowerMgmt & ECapabilitySwEvent"));
	process.Create((1u<<ECapabilitySwEvent)|(1u<<ECapabilityPowerMgmt),ETestProcessAddEventECaseClose);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrNone);
	CLOSE_AND_WAIT(process);


	test.Next(_L("Try UserSvr::AddEvent(ESwitchOff) without ECapabilityPowerMgmt"));
	process.Create(~(1u<<ECapabilityPowerMgmt),ETestProcessAddEventESwitchOff);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Try UserSvr::AddEvent(ESwitchOff) without ECapabilitySwEvent"));
	process.Create(~(1u<<ECapabilitySwEvent),ETestProcessAddEventESwitchOff);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	TInt muid = 0;
	HAL::Get(HAL::EMachineUid, muid);
	if(muid==HAL::EMachineUid_OmapH2 || muid==HAL::EMachineUid_OmapH4 || muid==HAL::EMachineUid_OmapH6 || muid==HAL::EMachineUid_NE1_TB || muid==HAL::EMachineUid_X86PC || muid==HAL::EMachineUid_Win32Emulator)
		{
		test.Next(_L("Calling UserSvr::AddEvent(ESwitchOff) with ECapabilityPowerMgmt & ECapabilitySwEvent"));
		TRequestStatus absstatus;
		RTimer abstimer;
		TInt r = abstimer.CreateLocal();
		test (r == KErrNone);
		SetAbsoluteTimeout(abstimer, 5000000, absstatus); // 5 sec
		process.Create((1u<<ECapabilitySwEvent)|(1u<<ECapabilityPowerMgmt),ETestProcessAddEventESwitchOff);
		process.Logon(logonStatus);
		process.Resume();
		User::WaitForRequest(absstatus);
		abstimer.Close();
		User::WaitForRequest(logonStatus);
		test(process.ExitType()==EExitKill);
		test(logonStatus==KErrNone);
		CLOSE_AND_WAIT(process);
		}

	test.End();
	}


const TInt KThreadCompleteOk = 0x80000002;
TInt TestExceptionResult = KErrGeneral;

void ExceptionHandler(TExcType /*aType*/)
	{
	TestExceptionResult = KErrNone;
	}

TInt TestExceptionThread(TAny* aArg)
	{
	TestExceptionResult = KErrGeneral;

	User::SetExceptionHandler(ExceptionHandler,KExceptionAbort);

	if(User::ExceptionHandler()!=ExceptionHandler)
		return KErrGeneral;

	if(User::IsExceptionHandled(EExcAbort))
		User::ModifyExceptionMask(KExceptionAbort,0);

	if(User::IsExceptionHandled(EExcAbort))
		return KErrGeneral;

	User::ModifyExceptionMask(0, KExceptionAbort);
	if(!User::IsExceptionHandled((TExcType)EExcAbort))
		return KErrGeneral;

	if(User::RaiseException((TExcType)(TInt)aArg)!=KErrNone)
		return KErrGeneral;

	return KThreadCompleteOk;
	}

void TestException()
	{
	RTestThread thread;
	TRequestStatus logonStatus;

	test.Start(_L("Test handled exceptions"));
	thread.Create(TestExceptionThread,EExcAbort);
	thread.Logon(logonStatus);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	test(logonStatus==KThreadCompleteOk);
	test(TestExceptionResult==KErrNone);

	test.Next(_L("Test unhandled exceptions"));
	thread.Create(TestExceptionThread,EExcKill);
	thread.Logon(logonStatus);
	TInt jit = User::JustInTime();
	User::SetJustInTime(EFalse);
	thread.Resume();
	User::WaitForRequest(logonStatus);
	User::SetJustInTime(jit);
	test(logonStatus==ECausedException);
	test(TestExceptionResult==KErrGeneral);

	test.End();
	}

void TestSetHomeTime()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessSetHomeTime,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,ETestProcessSetHomeTime,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestSetUTCOffset()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessSetUTCOffset,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,ETestProcessSetUTCOffset,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestSetUTCTime()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessSetUTCTime,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,ETestProcessSetUTCTime,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestSetUTCTimeAndOffset()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessSetUTCTimeAndOffset,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,ETestProcessSetUTCTimeAndOffset,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestSetMemoryThresholds()
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),ETestProcessSetMemoryThresholds,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitPanic); // Process should have got a Platform Security panic
	test(logonStatus==EPlatformSecurityTrap);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,ETestProcessSetMemoryThresholds,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
	CLOSE_AND_WAIT(process);

	test.End();
	}



void TestWithWriteDeviceData(TTestProcessFunctions aFunction)
	{
	RTestProcess process;
	TRequestStatus logonStatus;

	test.Start(_L("Try call without ECapabilityWriteDeviceData"));
	process.Create(~(1u<<ECapabilityWriteDeviceData),aFunction,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==KErrPermissionDenied);
	CLOSE_AND_WAIT(process);

	test.Next(_L("Test call with ECapabilityWriteDeviceData"));
	process.Create(1<<ECapabilityWriteDeviceData,aFunction,KTestUid);
	process.Logon(logonStatus);
	process.Resume();
	User::WaitForRequest(logonStatus);
	test(process.ExitType()==EExitKill);
	test(logonStatus==0);
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

	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
		{
		test.Start(_L("TESTS NOT RUN - EPlatSecEnforcement is OFF"));
		test.End();
		return 0;
		}

	test_KErrNone(SyncSemaphore.CreateGlobal(KSyncSemaphoreName,0));

	test.Start(_L("Test MachineConfiguration()"));
	TestMachineConfiguration();

	test.Next(_L("Test SetCritical()"));
	TestSetCritical();

	test.Next(_L("Test Set/PriorityControl()"));
	User::SetPriorityControl(ETrue);
	test(User::PriorityControl());
	User::SetPriorityControl(EFalse);
	test(!User::PriorityControl());

	test.Next(_L("Test Event functions"));
	TestEvents();

	test.Next(_L("Test Exception functions"));
	TestException();

	test.Next(_L("Test SetHomeTime()"));
	TestSetHomeTime();

	test.Next(_L("Test SetUTCOffset()"));
	TestSetUTCOffset();

	test.Next(_L("Test SetUTCTime()"));
	TestSetUTCTime();

	test.Next(_L("Test SetUTCTimeAndOffset()"));
	TestSetUTCTimeAndOffset();

	test.Next(_L("Test SetMemoryThresholds"));
	TestSetMemoryThresholds();

	test.Next(_L("Test Locale::Set"));
	TestWithWriteDeviceData(ETestProcessLocaleSet);

	test.Next(_L("Test User::SetCurrencySymbol"));
	TestWithWriteDeviceData(ETestProcessUserSetCurrencySymbol);

	test.Next(_L("Test UserSvr::ChangeLocale"));
	TestWithWriteDeviceData(ETestProcessChangeLocale);

	test.Next(_L("Test TExtendedLocale::SaveSystemSettings"));
	TestWithWriteDeviceData(ETestProcessSaveSystemSettings);

	test.Next(_L("Test TExtendedLocale::SetCurrencySymbol"));
	TestWithWriteDeviceData(ETestProcessSetCurrencySymbol);

	SyncSemaphore.Close();
	test.End();
	return(0);
    }

