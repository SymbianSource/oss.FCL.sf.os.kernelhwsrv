// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\runtests\runtests.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32def_private.h>
#include <e32svr.h>
#include <e32ldr.h>
#include <hal.h>
#include "f32file.h"
#include "f32dbg.h"
#include "runtests.h"
#include <u32hal.h>

#define __PANIC(r) Panic(__LINE__,r)

//#define _RUN_FOREVER_
//#define _PANIC_ON_FAILURE_

const TInt KDefaultTimeout=1200;		// 20 minutes
const TInt KBackgroundTimeout=3000000;	// 3 seconds
const TInt KDestroyTimeout=1000000;		// 1 second

_LIT(KLitBackslash,"\\");
_LIT(KDebugMessage, "Testing finished, and the system will be panicked");
#ifdef __EPOC32__
_LIT(KLitDefaultTestPath,"Z:\\TEST");
#else
_LIT(KLitDefaultTestPath,"");
#endif

#ifdef _DEBUG
_LIT(KBuildType, "UDEB");
#else
_LIT(KBuildType, "UREL");
#endif

typedef RArray<TUint> RProcIdList;

GLDEF_D TPath TheTestPath = KLitDefaultTestPath();
GLDEF_D RFs TheFs;
GLDEF_D RLoader TheLoaderSession;
GLDEF_D TDesC8* TheTestList;
GLDEF_D RTimer TheTimer;
GLDEF_D TBuf<256> TheProcessCommand=KNullDesC();
GLDEF_D TInt TheTimeOut = KDefaultTimeout;
GLDEF_D TInt TheCurrentProcessList;
GLDEF_D RProcIdList ProcLists[2];
GLDEF_D RTimer IdleWaitTimer;
GLDEF_D RTimer DestructionTimer;
GLDEF_D RProperty CurrTest;
TBool ShowTimings = 0;
TInt TickPeriod = 15625;
TBool CleanUpProcesses = EFalse;

#ifdef _ENABLE_BTRACE_ANALYSIS_

// BTrace analysis forward declarations
const TInt KDefaultBTraceLevel = 0;
TBool BtraceAnalysis = EFalse;
TInt BTraceAnalysisLevel;
TInt BTraceAnalyseSetup();
void BTraceAnalyseEnd();
void BTraceAnalyse(TInt aAnalysisLevel);

#endif //_ENABLE_BTRACE_ANALYSIS_

_LIT(KLitPanicCategory,"RUNTESTS-");
_LIT(KLitLogPreamble,"RUNTESTS: ");

void LogMsg(TRefByValue<const TDesC> aFmt,...);

void DisableSimulatedFailure()
	{
	// Turn off simulated failure mechanisms for all base servers
	TheFs.SetAllocFailure(KAllocFailureOff);	// F32 heap failure
	TheFs.SetErrorCondition(KErrNone, 0);		// F32 other failure
	TheLoaderSession.DebugFunction(ELoaderDebug_SetHeapFail, 0, 0, 0);	// Loader heap failure
	TheLoaderSession.DebugFunction(ELoaderDebug_SetRFsFail, KErrNone, 0, 0);	// Loader RFs failure

	// make sure kernel heap debug is off
	__KHEAP_TOTAL_RESET;
	}

GLDEF_C void Panic(TInt aLine, TInt aReason)
	{
	TBuf<16> cat=KLitPanicCategory();
	cat.AppendNum(aLine);
	User::Panic(cat,aReason);
	}

TInt CloseAndWait(RHandleBase aH, TRequestStatus *aN = NULL)
	{
	TRequestStatus tempS;
	if(!aN)
		{
		// Create a destruction notifier if none was supplied.
		aH.NotifyDestruction(tempS);
		aN = &tempS;
		}
	if (*aN!=KRequestPending)
		{
		User::WaitForRequest(*aN);
		aH.Close();
		return KErrNoMemory;
		}
	TRequestStatus t;
	DestructionTimer.After(t, KDestroyTimeout);
	aH.Close();
	User::WaitForRequest(*aN, t);
	if (*aN != KRequestPending)
		{
		DestructionTimer.Cancel();
		User::WaitForRequest(t);
		return KErrNone;
		}
	User::CancelMiscNotifier(*aN);
	User::WaitForRequest(*aN);
	return KErrTimedOut;
	}

void CloseWaitAndWarn(RHandleBase aH, TRequestStatus *aN = NULL)
	{
	TFullName fn(aH.FullName());
	TInt r = CloseAndWait(aH, aN);
	if (r == KErrNoMemory)
		LogMsg(_L("WARNING OOM checking destruction of %S"), &fn);
	else if (r == KErrTimedOut)
		LogMsg(_L("ERROR Destruction of %S timed out"), &fn);
	}

TInt InitIdleWait()
	{
	TInt r = IdleWaitTimer.CreateLocal();
	if (r!=KErrNone)
		return r;
	return KErrNone;
	}

void WaitForIdle()
	{
	TRequestStatus idle_req;
	TRequestStatus timer_req;
	IdleWaitTimer.After(timer_req, KBackgroundTimeout);
	User::NotifyOnIdle(idle_req);
	User::WaitForRequest(idle_req, timer_req);
	if (idle_req != KRequestPending)
		{
		IdleWaitTimer.Cancel();
		User::WaitForRequest(timer_req);
		}
	else
		{
		User::CancelMiscNotifier(idle_req);
		User::WaitForRequest(idle_req);
		LogMsg(_L("WARNING Excessive Background Activity Detected"));
		}
	}

TBool IntentionallyPersistent(RProcess aProcess)
	{
	TInt v;
	TInt r = RProperty::Get(aProcess.SecureId(), KRuntestsIntentionalPersistenceKey, v);
	if (r==KErrNone && TUint(v)==KRuntestsIntentionalPersistenceValue)
		return ETrue;
	return EFalse;
	}

TInt GetProcessListThread(TAny* a)
	{
	RProcIdList& pl = *(RProcIdList*)a;
	TFindProcess fp(_L("*"));
	TFullName fn;
	TInt r = KErrNone;
	while (r==KErrNone && fp.Next(fn)==KErrNone)
		{
		RProcess p;
		r = p.Open(fp, EOwnerThread);
		if (r==KErrNone)
			{
			TUint id = (TUint)p.Id();
			r = pl.Append(id);
			p.Close();
			}
		}
	return r;
	}

TInt GetProcessList(RProcIdList& aList)
	{
	aList.Reset();
	RThread t;
	TRequestStatus s;
	TInt r = t.Create(KNullDesC, &GetProcessListThread, 0x1000, NULL, &aList);
	if (r==KErrNone)
		{
		t.Logon(s);
		t.SetPriority(EPriorityAbsoluteHigh);
		if (s==KRequestPending)
			t.Resume();
		User::WaitForRequest(s);
		r=s.Int();
		if (t.ExitType()==EExitPending)
			{
			t.Kill(0);
			WaitForIdle();
			}
		else if (t.ExitType()!=EExitKill)
			{
			r = -99;
			}
		CloseWaitAndWarn(t);
		}
	aList.Sort();
	return r;
	}

TBool ParseNumber(TLex& aLex, TUint& aNumber, TBool isTime)
	{
	TPtrC numberDes = aLex.NextToken();
	TInt len = numberDes.Length();
	if (len == 0)
		{
		return EFalse;
		}

	aNumber = 0;
	TInt magnitude = 1;
	TChar c = numberDes[len-1];
	if (isTime)
		{
		switch (c)
			{
			case 'h':
			case 'H':
				len -= 1;
				magnitude = 3600;
				break;

			case 'm':
			case 'M':
				len -= 1;
				/*FALLTHRU*/
			default:
				magnitude = 60;
				break;

			case 's':
			case 'S':
				len -= 1;
				magnitude = 1;
				break;
			}
		}

	for (TInt i = len-1; i >= 0; --i)
		{
		c = numberDes[i];
		if (c < '0' || c > '9')
			__PANIC(KErrArgument);
		aNumber += ((TInt)c-'0')*magnitude;
		magnitude *= 10;
		}

	return ETrue;
	}

void GetTimeOut(TLex& aLex)
//
//
//
	{
	TheTimeOut = KDefaultTimeout;
	TUint timeOut = 0;
	if (ParseNumber(aLex, timeOut, ETrue))
		{
		TheTimeOut = timeOut;
		}
	}

#ifdef _ENABLE_BTRACE_ANALYSIS_

void GetAnalysisLevel(TLex& aLex)
	{
	BTraceAnalysisLevel = KDefaultBTraceLevel;
	TUint level;
	if (ParseNumber(aLex, level, EFalse))
		{
		BTraceAnalysisLevel = level;
		}
	}
#endif //_ENABLE_BTRACE_ANALYSIS_

void LogMsg(TRefByValue<const TDesC> aFmt,...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> buf=KLitLogPreamble();
	buf.AppendFormatList(aFmt,list);
	RDebug::Print(_L("%S"),&buf);
	}

_LIT(KLitError, "Error ");
TBool LogProcess(TUint aId, TBool aInit)
	{
	TFullName pn;
	TFileName fn;
	RProcess p;
	TBool killed = EFalse;
	TInt r = p.Open(TProcessId(aId));
	if (r==KErrNone)
		{
		if (IntentionallyPersistent(p))
			{
			p.Close();
			return killed;
			}
		pn = p.FullName();
		fn = p.FileName();
		if (!aInit && CleanUpProcesses && p.ExitType()==EExitPending)
			{// p is a left over process so terminate it.
			killed = ETrue;
			TRequestStatus status;
			p.Logon(status);
			p.Kill(KErrNone);	// Kill with KErrNone to suppress extra debug output from kernel.
			User::WaitForRequest(status);
			CloseAndWait(p);
			}
		else
			{
			p.Close();
			}
		}
	else
		{
		pn = KLitError;
		pn.AppendNum(r);
		}
	if (aInit)
		LogMsg(_L("Running process id=%d: %S (%S)"),aId,&pn,&fn);
	else
		{
		if(killed)
			LogMsg(_L("ERROR Leftover process was killed id=%d: %S (%S)"),aId,&pn,&fn);
		else
			LogMsg(_L("ERROR Leftover process id=%d: %S (%S)"),aId,&pn,&fn);
		}
	return killed;
	}

void ListProcesses()
	{
	RProcIdList& cur_list = ProcLists[TheCurrentProcessList];
	TInt cc = cur_list.Count();
	TInt ci;
	for (ci=0; ci<cc; ++ci)
		{
		LogProcess(cur_list[ci], ETrue);
		}
	}

void CheckProcesses()
	{
	RProcIdList& cur_list = ProcLists[TheCurrentProcessList];
	RProcIdList& new_list = ProcLists[1-TheCurrentProcessList];
	TInt r = GetProcessList(new_list);
	if (r!=KErrNone)
		{
		LogMsg(_L("WARNING Problem getting process list, error %d"),r);
		return;
		}

	TInt cc = cur_list.Count();
	TInt nc = new_list.Count();
	TInt ci = 0;
	TInt ni = 0;
	while (ci<cc || ni<nc)
		{
		TUint id1=0;
		TUint id2=0;
		if (ci<cc)
			id1 = cur_list[ci];
		if (ni<nc)
			id2 = new_list[ni];
		if (ci==cc)
			{
			// extra process has appeared so kill it and output an error message.
			if (LogProcess(id2, EFalse))
				{// Remove from list as we don't want it to be considered as vanished when the next test completes.
				new_list.Remove(ni);
				nc--;
				}
			else
				{// Extra process was left running so just move onto the next one.
				ni++;
				}
			}
		else if (ni<nc && id1==id2)
			{
			// process remains
			++ci, ++ni;
			}
		else
			{
			// process has disappeared
			LogMsg(_L("WARNING Vanished process, id=%d"),id1);
			++ci;
			}
		}

	// current list = new list
	TheCurrentProcessList = 1 - TheCurrentProcessList;

	// throw away the old list
	cur_list.Reset();
	}

_LIT8 (KLitRemark,"REM");
_LIT8 (KLitAtRemark,"@REM");

void ProcessLine(const TDesC8& aLine)
	{
	TLex8 lex(aLine);
	TPtrC8 testname=lex.NextToken();
	if (testname.Length()>=2 && testname[0]=='/' && testname[1]=='/')
		return;
	// ignore this line if it begins with rem or @rem 
	if (testname.CompareF(KLitRemark) == 0 || testname.CompareF(KLitAtRemark) == 0)
		return;
	TFileName testnameU;
	testnameU.Copy(testname);
	TFileName fullpathname;
	if (testnameU.Locate(TChar('\\'))==KErrNotFound)
		fullpathname=TheTestPath;
	fullpathname+=testnameU;
	if (testname.Locate(TChar('.'))==KErrNotFound)
		fullpathname+=_L(".EXE");
	TInt r;

	RFile file;
	r=file.Open(TheFs,fullpathname,EFileRead);
	if (r!=KErrNone)
		{
		// Remove path to let loader locate exe
		fullpathname = fullpathname.Mid(fullpathname.LocateReverse('\\')+1);
		}
	else
		file.Close();

	RProcess p;
	if(TheProcessCommand==KNullDesC)
		{
		TheProcessCommand.Copy(lex.Remainder());
		r=p.Create(fullpathname, TheProcessCommand);
		TheProcessCommand=KNullDesC();
		}
	else
		r=p.Create(fullpathname, TheProcessCommand);
	if (r!=KErrNone)
		{
		LogMsg(_L("Test %S ERROR Could not load file, error %d"),&fullpathname,r);
		return;
		}
	else
		{
		LogMsg(_L("Started test %S"),&fullpathname);
		}
	TRequestStatus ds;
	p.NotifyDestruction(ds);	// allocate the destruction notifier early so that it doesn't get flagged as a leak by kernel heap checking in e.g., efile (DEF133800)
	CurrTest.Set(p.FileName());
	User::After(100000);		// allow latency measurements to be output
	p.SetJustInTime(EFalse);	// we don't want the automatic test run to be halted by the debugger
	TRequestStatus ps;
	p.Rendezvous(ps);
	TInt time_remain = TheTimeOut;
	TRequestStatus ts;
	TUint start = User::TickCount();
	p.Resume();
	TBool persist = EFalse;
	TBool timer_running = EFalse;
	FOREVER
		{
		TInt nsec = Min(time_remain, 1800);
		if (!timer_running)
			TheTimer.After(ts, nsec*1000000);
		timer_running = ETrue;
		User::WaitForRequest(ps,ts);
		if (ps!=KRequestPending)
			{
			if (p.ExitType()==EExitPending)
				{
				// rendezvous completed but process not terminated
				if (!IntentionallyPersistent(p))
					{
					// not persistent - wait for process to terminate
					p.Logon(ps);
					continue;
					}
				persist = ETrue;
				}
			break;
			}
		timer_running = EFalse;
		time_remain -= nsec;
		if (time_remain==0)
			{
			LogMsg(_L("Going to kill test %S: it's taken %u seconds, which is too long"),&fullpathname,TheTimeOut);
			p.Kill(0);
			User::WaitForRequest(ps);
			p.Logon(ps);
			User::WaitForRequest(ps);

			CloseWaitAndWarn(p, &ds);
			RDebug::Print(_L("\n"));
			LogMsg(_L("Test %S TIMEOUT"),&fullpathname);
			return;
			}
		else
			{
			LogMsg(_L("Taken %u seconds so far"),TheTimeOut-time_remain);
			}
		}
	TUint end = User::TickCount();
	if(timer_running)
		{
		TheTimer.Cancel();
		User::WaitForRequest(ts);
		}

#ifdef _ENABLE_BTRACE_ANALYSIS_
	//
	//
	//
	if (BtraceAnalysis)
		{// Analyse BTrace buffer
		BTraceAnalyse(BTraceAnalysisLevel);
		}
#endif //_ENABLE_BTRACE_ANALYSIS_

	TBuf<32> exitCat=p.ExitCategory();
	TExitType exitType=p.ExitType();
	TInt exitReason=p.ExitReason();
	if (persist || (exitType==EExitKill && exitReason==KErrNone))
		{
		TUint time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
		if(ShowTimings)
			{
			LogMsg(_L("Test %S OK - Seconds Taken: %u"),&fullpathname, time);
			}
		else
			{
			LogMsg(_L("Test %S OK"),&fullpathname);
			}
		if (persist)
			{
			// We do not need this destruction notifier so cancel it.
			User::CancelMiscNotifier(ds);
			User::WaitForRequest(ds);
			
			p.Close();
			}
		else
			{
			CloseWaitAndWarn(p, &ds);
			}
		return;
		}
	LogMsg(_L("Test %S FAIL - Exit code %d,%d,%S"),&fullpathname,exitType,exitReason,&exitCat);
	CloseWaitAndWarn(p, &ds);
#if defined(_PANIC_ON_FAILURE_)
	__PANIC(KErrGeneral);
#endif
	}

void ProcessTestList()
	{
	TUint start = User::TickCount();

	TLex8 llex(*TheTestList);
	while(!llex.Eos())
		{
		llex.SkipSpace();
		llex.Mark();
		while(!llex.Eos() && llex.Peek()!='\n' && llex.Peek()!='\r')
			llex.Inc();
		TPtrC8 line=llex.MarkedToken();
		if (line.Length()!=0)
			ProcessLine(line);

		// allow cleanup to complete before starting the next test
		WaitForIdle();

		// make sure simulated failure is off
		DisableSimulatedFailure();

		// check for leftover processes
		CheckProcesses();
		
		// Reset the demand paging cache to its default size.
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize, 0, 0);
		}

	TUint end = User::TickCount();
	TUint time = TUint((TUint64)(end-start)*(TUint64)TickPeriod/(TUint64)1000000);
	LogMsg(_L("Elapsed Seconds: %u"), time);
	}

void Help()
	{
	RDebug::Print(_L("Runtests test list [-x where tests are] [-d drive letter to test] [-t timeout] [-p] [-st] [-c]"));
	RDebug::Print(_L("where -p sets runtests to be system permanent, -st enables timing information,"));
	RDebug::Print(_L("-c enables left over processes to be cleaned up"));
	}

GLDEF_C TInt E32Main()
	{
	HAL::Get(HAL::ESystemTickPeriod, TickPeriod);
	RThread().SetPriority(EPriorityAbsoluteHigh);
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TFileName thisfile=RProcess().FileName();
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	if (token.Length()==0)
		{//	__PANIC(0);
		Help();
		return 0;
		}
	TFileName listfilename=token;
	while (!lex.Eos())
		{
		token.Set(lex.NextToken());
		if (token.Length()==0)
			break;	// ignore trailing whitespace
		else if (token==_L("-x"))
			{
			token.Set(lex.NextToken());
			TheTestPath = token;
			}
		else if (token==_L("-d"))
			{
			token.Set(lex.NextToken());
			TheProcessCommand = token;
			}
		else if (token==_L("-t"))
			{
			GetTimeOut(lex);
			}
		else if (token==_L("-p"))
			{
			User::SetCritical(User::ESystemPermanent);
			}
		else if (token==_L("-st"))
			ShowTimings = 1;
		
#ifdef _ENABLE_BTRACE_ANALYSIS_		
		else if (token == _L("-a"))
			{
			BtraceAnalysis = ETrue;
			GetAnalysisLevel(lex);
			TInt r = BTraceAnalyseSetup();
			if (r != KErrNone)
				{
				RDebug::Print(_L("ERROR - Couldn't open BTrace driver (Code %d)"), r);
				return 0;
				}
			}
#endif //_ENABLE_BTRACE_ANALYSIS_

		else if (token == _L("-c"))
			CleanUpProcesses = ETrue;
		else
			{
			RDebug::Print(_L("Unknown option %S"), &token);
			Help();
			return 0;
			}
		}
		
	RDebug::Print(_L("TPTT= %S \n"), &TheProcessCommand);											
	RDebug::Print(_L("TTL= %S \n"), &listfilename);
	RDebug::Print(_L("TTP= %S \n"), &TheTestPath);
	RDebug::Print(_L("TO= %d seconds\n"), TheTimeOut);

	TInt l=TheTestPath.Length();
	if (l > 0 && TheTestPath[l-1]!='\\')
		TheTestPath+=KLitBackslash;
	if (listfilename.Locate(TChar('\\'))==KErrNotFound)
		listfilename.Insert(0,TheTestPath);
	TInt r=TheFs.Connect();
	if (r!=KErrNone)
		__PANIC(r);
	r = TheLoaderSession.Connect();
	if (r!=KErrNone)
		__PANIC(r);
	DisableSimulatedFailure();
	r=TheFs.SetSessionPath(_L("Z:\\test\\"));
	if (r!=KErrNone)
		__PANIC(r);
	r=TheTimer.CreateLocal();
	if (r!=KErrNone)
		__PANIC(r);
	RFile listfile;
	r=listfile.Open(TheFs,listfilename,EFileRead|EFileShareAny);
	if (r!=KErrNone)
		__PANIC(r);
	TInt listfilesize = 0;
	r=listfile.Size(listfilesize);
	if (r!=KErrNone)
		__PANIC(r);
	HBufC8* pL=HBufC8::New(listfilesize);
	if (!pL)
		__PANIC(KErrNoMemory);
	TPtr8 ptr=pL->Des();
	TheTestList=pL;
	r=listfile.Read(ptr);
	if (r!=KErrNone)
		__PANIC(r);
	listfile.Close();
	LogMsg(_L("Running test script %S"),&listfilename);
	LogMsg(_L("Build %S"),&KBuildType);
	LogMsg(_L("Path to test %S"),&TheProcessCommand);

	r = RProperty::Define(	KRuntestsCurrentTestKey,
							RProperty::EText,
							TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
							TSecurityPolicy(RProcess().SecureId()),
							512
						);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		__PANIC(r);
	r = CurrTest.Attach(RProcess().SecureId(), KRuntestsCurrentTestKey);
	if (r!=KErrNone)
		__PANIC(r);
	r = CurrTest.Set(KNullDesC);
	if (r!=KErrNone)
		__PANIC(r);

	r = DestructionTimer.CreateLocal();
	if (r!=KErrNone)
		__PANIC(r);
	TheCurrentProcessList = 0;
	r = GetProcessList(ProcLists[0]);
	if (r!=KErrNone)
		__PANIC(r);
	ListProcesses();
	r = InitIdleWait();
	if (r!=KErrNone)
		__PANIC(r);
#if defined(_RUN_FOREVER_)
	FOREVER
#endif
	ProcessTestList();
	r = CurrTest.Set(KNullDesC);
	if (r!=KErrNone)
		__PANIC(r);
	CurrTest.Close();
	User::After(1000000);	// allow latency measurements to be output before exiting
	LogMsg(_L("Completed test script %S"),&listfilename);
	TheLoaderSession.Close();
	TheFs.Close();
	TheTimer.Close();
	IdleWaitTimer.Close();
	DestructionTimer.Close();

#ifdef _ENABLE_BTRACE_ANALYSIS_
	BTraceAnalyseEnd();
#endif //_ENABLE_BTRACE_ANALYSIS_
	if(User::Critical()==User::ESystemPermanent)
		RDebug::Print(KDebugMessage);
	return KErrNone;
	}
