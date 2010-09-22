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
// e32test\secure\t_platsecconfig.cpp
// This test checks the correct functioning of the Platform Security configuration.
// To use this test for verification of the features, perform the following steps
// On the WINS Emulator
// 1.  Run "T_PLATSECCONFIG.EXE"
// Check that the results reported are:
// PlatSecEnforcement is OFF
// Disabled Capabilites: NONE
// Check EPOCWIND.OUT and verify that it contains no diagnostic messages. (These will start with the text "*PlatSec* ")
// 2.  Run "T_PLATSECCONFIG.EXE -mt_platsecconfig --"
// Check that the results reported are:
// PlatSecEnforcement is ON
// Disabled Capabilites: CommDD MultimediaDD WriteDeviceData TrustedUI DiskAdmin AllFiles NetworkServices ReadUserData Location (These are all ODD numbered capabilities)
// Check EPOCWIND.OUT and verify that it contains two lines starting with "*PlatSec* ERROR - Capability check failed"
// On reference hardware
// 3.  Build a Text Shell ROM contining the E32 tests. E.g.
// cd \cedar\generic\base\e32\rombuild
// rom -v=lubbock -t=e32test
// Boot this ROM and run "T_PLATSECCONFIG.EXE"
// Check that the results reported are:
// PlatSecEnforcement is OFF
// Disabled Capabilites: NONE
// Check the output of the debug port and verify that it contains no diagnostic messages.
// (These will start with the text "*PlatSec* ")
// 4.  Build a Text Shell ROM using the T_PLATSECCONFIG.OBY file. E.g.
// cd \cedar\generic\base\e32\rombuild
// rom -v=lubbock -t=t_platsecconfig
// Boot this ROM and run "T_PLATSECCONFIG.EXE"
// Check that the results reported are:
// PlatSecEnforcement is ON
// Disabled Capabilites: CommDD MultimediaDD WriteDeviceData TrustedUI DiskAdmin AllFiles NetworkServices ReadUserData Location (These are all ODD numbered capabilities)
// Check the output of the debug port and verify that it contains two lines with "*PlatSec* ERROR - Capability check failed"
// To check ROMBUILD configuration
// 5.  Build a Text Shell ROM using the T_PLATSECCONFIG_WARNIG.OBY file. E.g.
// cd \cedar\generic\base\e32\rombuild
// rom -v=lubbock -t=t_platsecconfig_warning
// This should produce the following warning:
// WARNING: *PlatSec* WARNING - Capability check failed. Can't load \Epoc32\RELEASE\ARM4\UDEB\t_psc_static.exebecause it links to t_psc_dll{00010000}.dll which has the following capabilities missing: TCB PowerMgmt ReadDeviceData DRM ProtServ NetworkControl SwEvent LocalServices WriteUserData
// 6.  Build a Text Shell ROM using the T_PLATSECCONFIG_ERROR.OBY file. E.g.
// cd \cedar\generic\base\e32\rombuild
// rom -v=lubbock -t=t_platsecconfig_error
// This should produce the following error:
// ERROR: *PlatSec* ERROR - Capability check failed. Can't load \Epoc32\RELEASE\ARM4\UDEB\t_psc_static.exe because it links to t_psc_dll{00010000}.dll which has the following capabilities missing: TCB PowerMgmt ReadDeviceData DRM ProtServ NetworkControl SwEvent LocalServices WriteUserData
// 
//

/**
 @file
*/

#define __INCLUDE_CAPABILITY_NAMES__

#include <e32test.h>

LOCAL_D RTest test(_L("T_PLATSECCONFIG"));

enum TTestProcessFunctions
	{
	ETestProcessServer,
	ETestProcessLoadLib,
	};

#include "testprocess.h"

TInt StartServer();

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	(void)aArg1;
	(void)aArg2;

	switch(aTestNum)
		{

	case ETestProcessServer:
		return StartServer();

	case ETestProcessLoadLib:
		{
		RLibrary lib;
		TInt r = lib.Load(_L("T_PSC_DLL"));
		lib.Close();
		return r;
		}

	default:
		User::Panic(_L("T_PLATSECCONFIG"),1);
		}

	return KErrNone;
	}



//
// RTestThread
//

class RTestThread : public RThread
	{
public:
	void Create(TThreadFunction aFunction,TAny* aArg=0);
	};

void RTestThread::Create(TThreadFunction aFunction,TAny* aArg)
	{
	TInt r=RThread::Create(_L(""),aFunction,KDefaultStackSize,KDefaultStackSize,KDefaultStackSize,aArg);
	test(r==KErrNone);
	}


//
// CTestSession
//

class CTestSession : public CSession2
	{
public:
	enum {EShutdown,EGetSecurityInfo};
public:
	CTestSession();
	virtual void ServiceL(const RMessage2& aMessage);
public:
	};

CTestSession::CTestSession()
	: CSession2()
	{}

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	RMessagePtr2 m(aMessage);
	switch (aMessage.Function())
		{
		case CTestSession::EGetSecurityInfo:
			{
			TSecurityInfo info;
			info.Set(RProcess());
			TInt r = aMessage.Write(0,TPtrC8((TUint8*)&info,sizeof(info)));
			m.Complete(r);
			}
			break;

		case CTestSession::EShutdown:
			CActiveScheduler::Stop();
			break;

		default:
			m.Complete(KErrNotSupported);
			break;
		}
	}



//
// CTestServer
//

class CTestServer : public CServer2
	{
public:
	CTestServer(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	};

CTestServer::CTestServer(TInt aPriority)
	: CServer2(aPriority)
	{
	}

CSession2* CTestServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const
	{
	return new (ELeave) CTestSession();
	}



//
// CTestActiveScheduler
//

class CTestActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;
	};

void CTestActiveScheduler::Error(TInt anError) const
	{
	User::Panic(_L("TestServer Error"),anError);
	}



//
// Server thread
//

_LIT(KServerName,"T_PLATSECCONFIG-server");
const TInt KServerRendezvous = KRequestPending+1;

void DoStartServer()
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	CTestServer* server = new (ELeave) CTestServer(0);
	CleanupStack::PushL(server);

	User::LeaveIfError(server->Start(KServerName));

	RProcess::Rendezvous(KServerRendezvous);

	CActiveScheduler::Start();

	CleanupStack::PopAndDestroy(2);
	}

TInt StartServer()
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		{
		return KErrNoMemory;
		}
	TRAPD(leaveError,DoStartServer())
	delete cleanupStack;
	return leaveError;
	}



//
// RTestSession
//

class RTestSession : public RSessionBase
	{
public:
	inline TInt Connect()
		{ return CreateSession(KServerName,TVersion());}
	inline TInt Send(TInt aFunction)
		{ return RSessionBase::SendReceive(aFunction); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs)
		{ return RSessionBase::SendReceive(aFunction,aArgs); }
	inline void Send(TInt aFunction,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aStatus); }
	inline void Send(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aArgs,aStatus); }
	};



RTestSession Session;



void CheckCapabilitySetEqual(const TCapabilitySet& a1,const TCapabilitySet& a2)
	{
	TInt i;
	for(i=0; i<ECapability_Limit; i++)
		{
		test((!a1.HasCapability((TCapability)i))==(!a2.HasCapability((TCapability)i)));
		}
	}

TBuf8<1024> CapabilityNameBuffer;
TBool PlatSecEnforcement=0;

TPtrC16 CapabilityList(const TCapabilitySet& aCaps)
	{
	TCapabilitySet allCaps;
	allCaps.SetAllSupported();
	CapabilityNameBuffer.Zero();
	TBool odd=ETrue;
	TBool even=ETrue;
	TInt i;
	for(i=0; i<ECapability_Limit; i++)
		{
		if(!aCaps.HasCapability((TCapability)i))
			{
			if(allCaps.HasCapability((TCapability)i))
				{
				if(i&1)
					{
					odd = EFalse;
					}
				else
					{
					even = EFalse;
					}
				}
			continue;
			}
		TUint8* ptr=(TUint8*)CapabilityNames[i];
		TPtrC8 name(ptr,User::StringLength(ptr));
		CapabilityNameBuffer.Append(name);
		CapabilityNameBuffer.Append((TChar)' ');
		}
	if(!CapabilityNameBuffer.Length())
		{
		CapabilityNameBuffer.Append(_L8("NONE"));
		}
	if(even)
		{
		CapabilityNameBuffer.Append(_L8("(These are all EVEN numbered capabilities)"));
		}
	if(odd)
		{
		CapabilityNameBuffer.Append(_L8("(These are all ODD numbered capabilities)"));
		}
	return CapabilityNameBuffer.Expand();
}

void TestPlatSecDisabledCaps()
	{
	TSecurityInfo info;
	TPckg<TSecurityInfo> infoPtr(info);

	test.Start(_L("Get disabled capabilities set"));
	TCapabilitySet disabled;
	disabled.SetDisabled();
	TPtrC16 list(CapabilityList(disabled));
	test.Printf(_L("  %S\n"),&list);

	test.Next(_L("Get capabilites from this EXE"));
	Mem::FillZ(&info,sizeof(info));
	info.SetToCurrentInfo();

	test.Next(_L("Check capabilities are same as disabled set"));
	CheckCapabilitySetEqual(info.iCaps,disabled);

	test.Next(_L("Get capabilites from other EXE"));
	Mem::FillZ(&info,sizeof(info));
	TInt r = Session.Send(CTestSession::EGetSecurityInfo,TIpcArgs(&infoPtr));
	test(r==KErrNone);

	test.Next(_L("Check capabilities are same as disabled set"));
	CheckCapabilitySetEqual(info.iCaps,disabled);

	test.Next(_L("Test PlatSec::IsCapabilityEnforced is consistant with our findings"));
	TCapabilitySet notEnforced;
	notEnforced.SetEmpty();
	for(TInt i=0; i<ECapability_HardLimit; i++)
		{
		if(!PlatSec::IsCapabilityEnforced((TCapability)i))
			{
			notEnforced.AddCapability((TCapability)i);
			}
		}	
	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
		{
		disabled.SetAllSupported();
		}
	CheckCapabilitySetEqual(notEnforced,disabled);

	test.End();
	}

_LIT(KRomExe,"T_PLATSECCONFIG2.EXE");
_LIT(KOn,"ON");
_LIT(KOff,"OFF");

void TestPlatSecEnforcement()
	{
	RProcess process;
	TInt r;

	test.Start(_L("Getting PlatSecEnforcement setting"));
	PlatSecEnforcement = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
	test.Printf(_L("  PlatSecEnforcement setting returns %S\n"),PlatSecEnforcement?&KOn:&KOff);

	test.Next(_L("Check dynamic linkage without required capabilities"));
	TBuf<10> arg;
	arg.Num((TInt)ETestProcessLoadLib);
	r=process.Create(_L("T_PSC_DYNAMIC"),arg);
	test(r==KErrNone);
	TRequestStatus logon;
	process.Logon(logon);
	process.Resume();
	User::WaitForRequest(logon);
	test(process.ExitType()==EExitKill);
	r=logon.Int();
	CLOSE_AND_WAIT(process);

	if(PlatSecEnforcement)
		{
		test(r==KErrPermissionDenied);
		}
	else
		{
		test(r==KErrNone);
		}

	test.Next(_L("Check static linkage without required capabilities"));
	r=process.Create(_L("T_PSC_STATIC"),_L(""));
	if(PlatSecEnforcement)
		{
		test(r==KErrPermissionDenied);
		}	
	else
		{
		test(r==KErrNone);
		process.Kill(0);
		CLOSE_AND_WAIT(process);
		}

	test.End();
	}

/**
Do simple test of diagnostic function without parameters (EmitDiagnostic())
*/
void TestPlatSecDiagnostic()
	{
	TInt r;
	r=PlatSec::EmitDiagnostic();
	if(PlatSecEnforcement)
		{
		test(r==KErrPermissionDenied);
		}
	else
		{
		test(r==KErrNone);
		}
	}

#include <e32svr.h>
IMPORT_C void dummyExport();

GLDEF_C TInt E32Main()
    {
#ifdef STATIC_TEST_LINK
	dummyExport(); // Use dummy export from staticly linked DLL
#endif
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

	test.Start(_L("Starting test server"));
	RTestProcess server;
	TRequestStatus rendezvous;
	TBuf<10> arg;
	arg.Num((TInt)ETestProcessServer);
	TInt r=server.RProcess::Create(KRomExe,arg);
	test(r==KErrNone);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	test.Next(_L("Openning server session"));
	r = Session.Connect();
	RDebug::Print(_L("%d"),r);
	test(r==KErrNone);

	test.Next(_L("Test PlatSecDisabledCaps"));
	TestPlatSecDisabledCaps();

	test.Next(_L("Test PlatSecEnforcement"));
	TestPlatSecEnforcement();

	test.Next(_L("Test PlatSecDiagnostic()"));
	TestPlatSecDiagnostic();

	test.Next(_L("Closing server session"));
	Session.Send(CTestSession::EShutdown);
	Session.Close();
	CLOSE_AND_WAIT(server);

	// Show results requiring manual inspection
	_LIT(KSeperatorText,"----------------------------------------------------------------------------\n"); 
	test.Printf(_L("\n"));
	test.Printf(_L("RESULTS (To be checked against expected values)\n")); 
	test.Printf(KSeperatorText);
	test.Printf(_L("*  PlatSecEnforcement is %S\n"),PlatSecEnforcement?&KOn:&KOff);
	test.Printf(KSeperatorText);
	TCapabilitySet disabled;
	disabled.SetDisabled();
	TPtrC16 list(CapabilityList(disabled));
	test.Printf(_L("*  Disabled Capabilites: %S\n"),&list);
	test.Printf(KSeperatorText);

	// Wait for a while, or for a key press
	test.Printf(_L("Waiting a short while for key press...\n"));
	TRequestStatus keyStat;
	test.Console()->Read(keyStat);
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,20*1000000);
	User::WaitForRequest(timerStat,keyStat);
	if(keyStat!=KRequestPending)
		{
		(void)test.Console()->KeyCode();
		}
	timer.Cancel();
	test.Console()->ReadCancel();
	User::WaitForAnyRequest();

	test.End();
	return(0);
    }

