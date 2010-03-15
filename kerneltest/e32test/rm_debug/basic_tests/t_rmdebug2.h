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
// Definitions for the run mode debug tests
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef RMDEBUG_H
#define RMDEBUG_H

#include "t_rmdebug_app.h"
#include "r_user_low_memory_security_svr_session.h"
#include "r_kernel_low_memory_security_svr_session.h"


class CRunModeAgent;

// Create a pointer to function type
typedef void (CRunModeAgent::*testFunction)();

class TFunctionData
	{
public:
	testFunction iFunctionPtr;
	TBuf<40> iFunctionName;
	};

//number of test functions that we have
const TInt KMaxTests = 28;

//
// class CRunModeAgent
//
// The basic run mode agent.
//
class CRunModeAgent : public CBase
	{
public:
	static CRunModeAgent* NewL();
	~CRunModeAgent();
	void ClientAppL();

private:
	CRunModeAgent();
	void ConstructL();
	void SetupAndAttachToDSS();

	TInt TestStartup();
	TInt TestShutdown();

	void TestGetExecutablesList();
	void TestGetProcessList();
	void TestGetThreadList();
	void TestGetCodeSegsList();
	void TestGetXipLibrariesList();
	void TestGetListInvalidData();

	void DoTestGetThreadList(const TBool aShouldPass, const Debug::TListScope aListScope, const TUint64 aTargetId=0);
	void DoTestGetCodeSegsList(const TBool aShouldPass, const Debug::TListScope aListScope, const TUint64 aTargetId=0);

	void DoGetList(const Debug::TListId aListId, const Debug::TListScope aListScope, RBuf8& aBuffer, TUint32& aSize, const TUint64 aTargetId=0);

	void TestMemoryAccess();
	void TestSuspendResume();
	void TestBreakPoints();
	void TestConsecutiveBreakPoints();
	void TestModifyBreak();
	void DoTestModifyBreak(TBool aThreadSpecific);
	void TestBreakInfo();
	void DoTestBreakInfo(TBool aThreadSpecific);
	void TestRunToBreak();
	void DoTestRunToBreak(TBool aThreadSpecific);
	void TestRegisterAccess();
	void TestAttachExecutable();
	void TestDebugFunctionality();
	void TestStep();
	void DoTestStep(TBool aThreadSpecific);
	void TestDriverSecurity();
	void TestSecurity();
	void TestEvents();
	void TestEventsForExternalProcess();
	void TestDemandPaging();
	void TestTraceSecurity();
	void TestDllUsage();
	void TestKillProcess();
	void TestProcessBreakPoints();
	void TestMultipleTraceEvents();
	void TestAddRemoveProcessEvents();
	void TestProcessKillBreakpoint();
	void DoTestProcessKillBreakpoint();

	//crash flash test functions
	void TestCrashFlash();
		
	TInt GetFlag(const TDes8 &aFlags, const TUint aOffset, Debug::TRegisterFlag &aFlagValue) const;

	void ReportPerformance(void);

	// helper functions
	void HelpTestSecurityAttachDetachExecutable(const TDesC& aProcessName, TBool aExpectSuccess);

	TInt HelpTestStepSetBreak(Debug::TBreakId& aBreakId, TThreadId aThreadId, const TUint32 aBreakAddress, Debug::TArchitectureMode aMode, TBool aThreadSpecific=ETrue, TProcessId aProcessId=0);
	TInt HelpTestStepClearBreak(const Debug::TBreakId aBreakId, const TThreadId aThreadId, TBool aThreadSpecific);
	TInt HelpTestStepWaitForBreak(const TDesC& aProcessName, Debug::TEventInfo& aEventInfo);
	TInt HelpTestStepReadPC(TThreadId aThreadId, TUint32& aPC);
	TInt HelpTestStep(TThreadId aThreadId, TUint32 aStartAddress, TUint32 aEndAddress, Debug::TArchitectureMode aMode, TUint aNumSteps, TBool aThreadSpecific=ETrue, TProcessId=0);

	TInt HelpTicksPerSecond(void);

	// helper functions
	void HelpStartTestTimer(void) { iStartTick = User::NTickCount(); iStopTick = 0; };
	void HelpStopTestTimer(void) { iStopTick = User::NTickCount(); };
	TInt HelpGetTestTicks(void) { return (iStopTick - iStartTick); };
	TInt SwitchTestFunction(TTestFunction aTestFunction);
	TInt LaunchProcess(RProcess& aProcess, const TDesC& aFileName, TDebugFunctionType aFunctionType, TUint32 aDelay=0, TUint32 aExtraThreads=0);
	Debug::TTagHeader* GetTagHdr(const TDesC8& aDebugFunctionalityBlock, const Debug::TTagHeaderId aTagHdrId) const;
	Debug::TTag* GetTag(const Debug::TTagHeader* aTagHdr, const TInt aElement) const;
	Debug::TTag GetTag(const Debug::TTagHeaderId aTagHdrId, const TInt aElement);
	TBool ProcessExists(const TProcessId aProcessId);
	TBool ThreadExistsForProcess(const TThreadId aThreadId, const TProcessId aProcessId);
	TBool ListingSupported(const Debug::TListId aListId, const Debug::TListScope aListScope);
	void TestEventsWithExtraThreads(Debug::TKernelEventAction aActionMain, Debug::TKernelEventAction aActionExtra, TUint32 aExtraThreads);
	void FillArray();
	void PrintUsage();
	void PrintVersion();

	enum TTestMode 
		{
		//run all the tests
		EModeAll = 1<<0,
		//run the specified tests in reverse order
		EModeReverse = 1<<1,
		//print out help
		EModeHelp = 1<<2,
		//print out help
		EModeVersion = 1<<3
		};

	void RunTest(TInt aTestNumber);
	void ParseCommandLineL(TUint32& aMode, RArray<TInt>& aTests);

	TBool ProcessExists(const TDesC& aProcessName);

private:

	TFunctionData iTestArray[KMaxTests];
#if defined(KERNEL_OOM_TESTING)
	RKernelLowMemorySecuritySvrSession iServSession;
#elif defined (USER_OOM_TESTING)
	RUserLowMemorySecuritySvrSession iServSession;
#else
	Debug::RSecuritySvrSession iServSession;
#endif
	RThread	iDebugThread;
	RProcess iDSSProcess;
	RSemaphore iAddressGlobSem;
	TThreadId iThreadID;
	TFileName iFileName;
	TUid iMySid;

	// Performance data
	TInt iMemoryReadKbytesPerSecond;	
	TInt iMemoryWriteKbytesPerSecond;	
	TInt iBreakpointsPerSecond;
	TInt iMaxBreakpoints;
	TInt iStepsPerSecond;

	// Timing information
	TInt iStartTick;
	TInt iStopTick;
	};

#endif // RMDEBUG_H
