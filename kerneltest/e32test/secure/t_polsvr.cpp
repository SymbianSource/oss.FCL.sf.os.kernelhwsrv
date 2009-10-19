// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_polsvr.cpp
// Overview:
// Policy Server Tests
// API Information:
// CPolicyServer
// Details:
// - Perform various policy server tests:
// Server1 has implementations of CustomSecurityCheckL and
// CustomFailureActionL This is the test for all the paths in a policy servers
// implementations -- not all connect paths though
// This test also ensures that every path through the binary search is
// covered.
// Policy2,3,4,5,6, are bad policies that should cause the server to panic
// in debug mode.  In release, they'll just pass, however, if you later tried
// to use them, something would go horribly wrong
// Policies 7,8,9 check various types of connect policies.
// Server 1,2,3 are used here all with policy 8 because you can only test 1
// type of connect policy per server.
// Sever2 does not have implementations of CustomSecurityCheckL and
// CustomFailureActionL. When these functions are called it should crash
// Server4 is used for checking what happens when the custom functions use
// another active object.  This test encompasses leaving due to OOM, and
// cancellation.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32debug.h>

LOCAL_D RTest test(_L("T_POLSVR"));

_LIT(KPolSvr, "T_POLSVR");

#define Debug(x) RDebug::Print(_L(x))

const TUint32 KTestCapabilities =(1<<ECapabilityTCB)
								|(1<<ECapabilityPowerMgmt)
								|(1<<ECapabilityReadDeviceData)
								|(1<<ECapabilityDRM)
								|(1<<ECapabilityProtServ)
								|(1<<ECapabilityNetworkControl)
								|(1<<ECapabilitySwEvent)
								|(1<<ECapabilityLocalServices)
								|(1<<ECapabilityWriteUserData);

const TInt KCustomCheckMask		= 0x01000000;
const TInt KCustomActionMask	= 0x02000000;
const TInt KServiceLMask		= 0x04000000;
const TInt KActiveCheckMask 	= 0x08000000;
const TInt KActiveActionMask	= 0x00100000;
const TInt KActiveRunLMask  	= 0x00200000;
const TInt KActiveRunErrorMask	= 0x00400000;
const TInt KActiveDoCancelMask	= 0x00800000;
const TInt KServiceErrorMask	= 0x00010000;

//Ideally this constant should go in your derived server.  However, I couldn't
//be bothered to put the same constant in every one of the derived servers in
//this test code
const TInt KQueryUser = -1;

void OrInFlags(const RMessage2& aMsg, TUint aMask)
	{
	TBuf8<4> flags(0);
	TInt r = aMsg.Read(0, flags);
	test(r == KErrNone);
	(*(TUint32*)(flags.Ptr())) |= aMask;
	flags.SetLength(4);
	r = aMsg.Write(0, flags);
	test(r == KErrNone);
	}

void SetFlags(TDes8& aDes, TUint aValue)
	{
	(*(TUint32*)(aDes.Ptr())) = aValue;
	}

TUint FlagsValue(const TDes8& aDes)
	{
	return (*(TUint32*)(aDes.Ptr()));
	}

enum TTestProcessFunctions
	{
	ETestProcessPolicyServer,
	};

enum TTestServerIndex
	{
	ETestServer1=0,
	ETestServer2,
	ETestServer3,
	ETestServer4,
	};

enum TServerPolicyIndex
	{
	EPolicy1=0,
	EPolicy2,
	EPolicy3,
	EPolicy4,
	EPolicy5,
	EPolicy6,
	EPolicy7,
	EPolicy8,
	EPolicy9,
	EPolicy10,
	EPolicy11,
	EPolicy12,
	};

//
// EPolicy1
//

const TUint gServerPolicy1RangeCount = 12;
const TInt gServerPolicy1Ranges[gServerPolicy1RangeCount] = { 
	0, //ENotSupported
	1, //ECustomCheck
	6, //EAlwaysPass
	7, //ENotSupported
	8, //->0
	9, //->1
	10, //->2
	11, //->3
	12, //->3
	13, //ENotSupported
	100, //EAlwaysPass -> Shutdown
	101, //ENotSupported
	};

const TUint8 gServerPolicy1ElementsIndex[] = { 
	CPolicyServer::ENotSupported,
	CPolicyServer::ECustomCheck, 
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	0, //RequireNetworkControl or EFail
	1, //RequireNetworkControl or EQueryUser
	2, //RequireDiskAdmin or EFail
	3, //RequireDiskAdmin or EQueryUser
	3, //RequireDiskAdmin or EQueryUser
	CPolicyServer::ENotSupported,
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	};

const CPolicyServer::TPolicyElement gServerPolicy1Elements[] =
	{
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),KQueryUser},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),KQueryUser},
	};

const CPolicyServer::TPolicy gServerPolicy1 = 
	{
	CPolicyServer::EAlwaysPass,gServerPolicy1RangeCount,
	gServerPolicy1Ranges,
	gServerPolicy1ElementsIndex,
	gServerPolicy1Elements,
	};

//
//EPolicy2
//

const TUint gServerPolicy2RangeCount = 1;
//Invalid Policy -- doesn't start with 0
const TInt gServerPolicy2Ranges[gServerPolicy2RangeCount] = { 
	1, //KErrNotSupported
	};

const TUint8 gServerPolicy2ElementsIndex[] = { 
	CPolicyServer::ENotSupported,
	};

const CPolicyServer::TPolicyElement gServerPolicy2Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy2 = 
	{
	0,gServerPolicy2RangeCount,
	gServerPolicy2Ranges,
	gServerPolicy2ElementsIndex,
	gServerPolicy2Elements
	};

//
//EPolicy3
//

const TUint gServerPolicy3RangeCount = 12;
//Invalid Policy -- range values not increasing
const TInt gServerPolicy3Ranges[gServerPolicy3RangeCount] = { 
	0, //ECustomCheck
	6, //EAlwaysPass
	7, //ENotSupported
	8, //->0
	9, //->1
	10, //->2
	11, //->3
	12, //->3
	13, //ENotSupported
	100, //EAlwaysPass -> Shutdown
	101, //ENotSupported
	99, //EAlwaysPass
	};

const TUint8 gServerPolicy3ElementsIndex[] = { 
	CPolicyServer::ECustomCheck, 
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	0, //RequireNetworkControl or EFail
	1, //RequireNetworkControl or EQueryUser
	2, //RequireDiskAdmin or EFail
	3, //RequireDiskAdmin or EQueryUser
	3, //RequireDiskAdmin or EQueryUser
	CPolicyServer::ENotSupported,
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	CPolicyServer::EAlwaysPass, 
	};

const CPolicyServer::TPolicyElement gServerPolicy3Elements[] =
	{
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),KQueryUser},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),KQueryUser},
	};

const CPolicyServer::TPolicy gServerPolicy3 = 
	{
	0,gServerPolicy3RangeCount,
	gServerPolicy3Ranges,
	gServerPolicy3ElementsIndex,
	gServerPolicy3Elements
	};

//
//EPolicy4
//

const TUint gServerPolicy4RangeCount = 1;
//Invalid Policy -- Elements Index has invalid values
const TInt gServerPolicy4Ranges[gServerPolicy4RangeCount] = { 
	0, //Invalid value
	};

const TUint8 gServerPolicy4ElementsIndex[] = { 
	CPolicyServer::ESpecialCaseHardLimit,
	};

const CPolicyServer::TPolicyElement gServerPolicy4Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy4 = 
	{
	0,gServerPolicy4RangeCount,
	gServerPolicy4Ranges,
	gServerPolicy4ElementsIndex,
	gServerPolicy4Elements
	};

//
//EPolicy5
//

const TUint gServerPolicy5RangeCount = 1;
//Invalid Policy -- Elements Index has invalid values
const TInt gServerPolicy5Ranges[gServerPolicy5RangeCount] = { 
	0, //uses Invalid value
	};

const TUint8 gServerPolicy5ElementsIndex[] = { 
	//Uses invalid value 
	CPolicyServer::ESpecialCaseLimit,
	};

const CPolicyServer::TPolicyElement gServerPolicy5Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy5 = 
	{
	0,gServerPolicy5RangeCount,
	gServerPolicy5Ranges,
	gServerPolicy5ElementsIndex,
	gServerPolicy5Elements
	};

//
//EPolicy6
//

const TUint gServerPolicy6RangeCount = 1;
//Invalid Policy -- Elements Index has invalid values
const TInt gServerPolicy6Ranges[gServerPolicy6RangeCount] = { 
	0,
	};

const TUint8 gServerPolicy6ElementsIndex[] = { 
	CPolicyServer::ENotSupported,
	};

const CPolicyServer::TPolicyElement gServerPolicy6Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy6 = 
	{
	//Uses invalid value for iOnConnect
	CPolicyServer::ESpecialCaseHardLimit,gServerPolicy6RangeCount,
	gServerPolicy6Ranges,
	gServerPolicy6ElementsIndex,
	gServerPolicy6Elements
	};

//
//EPolicy7
//
// Connect not supported
const TUint gServerPolicy7RangeCount = 1;
const TInt gServerPolicy7Ranges[gServerPolicy7RangeCount] = { 
	0,
	};

const TUint8 gServerPolicy7ElementsIndex[] = { 
	0
	};

const CPolicyServer::TPolicyElement gServerPolicy7Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy7 = 
	{
	CPolicyServer::ENotSupported,gServerPolicy7RangeCount,
	gServerPolicy7Ranges,
	gServerPolicy7ElementsIndex,
	gServerPolicy7Elements
	};

//
//EPolicy8
//
// Connect Custom Check
const TUint gServerPolicy8RangeCount = 1;
const TInt gServerPolicy8Ranges[gServerPolicy8RangeCount] = { 
	0,
	};

const TUint8 gServerPolicy8ElementsIndex[] = { 
	0
	};

const CPolicyServer::TPolicyElement gServerPolicy8Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EPanicClient},
	};

const CPolicyServer::TPolicy gServerPolicy8 = 
	{
	CPolicyServer::ECustomCheck,gServerPolicy8RangeCount,
	gServerPolicy8Ranges,
	gServerPolicy8ElementsIndex,
	gServerPolicy8Elements
	};

//
//EPolicy9
//
// Connect has a static policy but it fails.
const TUint gServerPolicy9RangeCount = 1;
const TInt gServerPolicy9Ranges[gServerPolicy9RangeCount] = { 
	0,
	};

const TUint8 gServerPolicy9ElementsIndex[] = { 
	0
	};

const CPolicyServer::TPolicyElement gServerPolicy9Elements[] =
	{
	{_INIT_SECURITY_POLICY_FAIL,CPolicyServer::EFailClient},
	};

const CPolicyServer::TPolicy gServerPolicy9 = 
	{
	0,gServerPolicy9RangeCount,
	gServerPolicy9Ranges,
	gServerPolicy9ElementsIndex,
	gServerPolicy9Elements
	};

//
// EPolicy10
//

const TUint gServerPolicy10RangeCount = 13;
const TInt gServerPolicy10Ranges[gServerPolicy10RangeCount] = { 
	0, //ECustomCheck
	5, //EAlwaysPass
	6, //ENotSupported
	8, //->0
	9, //->3
	10, //->2
	11, //->1
	12, //ENotSupported
	55, //->3
	58, //ENotSupported
	100, //EAlwaysPass -> Shutdown
	101, //ENotSupported
	KMaxTInt, //EAlways Pass
	};

const TUint8 gServerPolicy10ElementsIndex[] = { 
	CPolicyServer::ECustomCheck, 
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	0, //RequireNetworkControl or EFail
	3, //RequireDiskAdmin or EQueryUser
	2, //RequireDiskAdmin or EFail
	1, //RequireNetworkControl or EQueryUser
	CPolicyServer::ENotSupported,
	3,
	CPolicyServer::ENotSupported,
	CPolicyServer::EAlwaysPass, 
	CPolicyServer::ENotSupported,
	CPolicyServer::EAlwaysPass, 
	};

const CPolicyServer::TPolicyElement gServerPolicy10Elements[] =
	{
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl),KQueryUser},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),CPolicyServer::EFailClient},
	{_INIT_SECURITY_POLICY_C1(ECapabilityDiskAdmin),KQueryUser},
	};

const CPolicyServer::TPolicy gServerPolicy10 = 
	{
	CPolicyServer::EAlwaysPass,gServerPolicy10RangeCount,
	gServerPolicy10Ranges,
	gServerPolicy10ElementsIndex,
	gServerPolicy10Elements,
	};


//A list of all the global policies
const CPolicyServer::TPolicy* gPolicyIndex[] = {
	&gServerPolicy1,
	&gServerPolicy2,
	&gServerPolicy3,
	&gServerPolicy4,
	&gServerPolicy5,
	&gServerPolicy6,
	&gServerPolicy7,
	&gServerPolicy8,
	&gServerPolicy9,
	&gServerPolicy10,
	};


#include "testprocess.h"

TInt StartServer(const CPolicyServer::TPolicy& aPolicy, TUint aServerIndex);

TInt DoTestProcess(TInt aTestNum,TInt aArg1,TInt aArg2)
	{
	switch(aTestNum)
		{

	case ETestProcessPolicyServer:
		{
		__ASSERT_ALWAYS(aArg1 >= 0 && aArg1 < (TInt)sizeof(gPolicyIndex)>>2, User::Panic(KPolSvr, KErrArgument));
		const CPolicyServer::TPolicy& policy = *(gPolicyIndex[aArg1]);
		__ASSERT_ALWAYS(aArg2 >= 0, User::Panic(KPolSvr, KErrArgument));
		TInt r;
		r=StartServer(policy, TUint(aArg2));
		if(r==KErrAlreadyExists)
			{
			User::After(2*1000*1000);
			r=StartServer(policy, TUint(aArg2));
			}
		return r;
		}

	default:
		User::Panic(_L("T_POLSVR"),1);
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
// CTestSession1
//

class CTestSession1 : public CSession2
	{
public:
	enum {EShutdown=100};
public:
	CTestSession1();
	virtual void ServiceL(const RMessage2& aMsg);
public:
	};

CTestSession1::CTestSession1()
	: CSession2()
	{}


void CTestSession1::ServiceL(const RMessage2& aMsg)
	{
	OrInFlags(aMsg, KServiceLMask);
	TInt fn = aMsg.Function();
	switch(fn)
		{
		case 2:
		case 4:
		case 5:
		case 6:
		case 8:
		case 9:
		case 12:
		case KMaxTInt:
			aMsg.Complete(KErrNone);
			break;

		case CTestSession1::EShutdown:
			CActiveScheduler::Stop();
			aMsg.Complete(KErrNone);
			break;
		default:
			//If we get here we have an unhandled condition in the test code.
			//The test code is specifically setup to try and catch all branches
			//through the policy server.  If you get, there is some problem in
			//the setup.
			test(0);
			break;
		}
	}

//
// CTestPolicyServer1
//

class CTestPolicyServer1 : public CPolicyServer
	{
public:
	CTestPolicyServer1(TInt aPriority, const TPolicy& aPolicy);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMsg) const;
	virtual TCustomResult CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing);
	virtual TCustomResult CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing);
	};

CTestPolicyServer1::CTestPolicyServer1(TInt aPriority, const TPolicy& aPolicy)
	: CPolicyServer(aPriority, aPolicy)
	{
	}

CSession2* CTestPolicyServer1::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMsg*/) const
	{
	return new (ELeave) CTestSession1();
	}

CPolicyServer::TCustomResult CTestPolicyServer1::CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing)
	{
	TInt fn = aMsg.Function();
	if(fn >= 0)
		{
		//Connect messages don't use this debugging system
		OrInFlags(aMsg, KCustomCheckMask);
		}
	if(fn == -1) //Connect
		{
		return EPass;
		}
	else if(fn == 1)
		{
		aMissing.iCaps.AddCapability(ECapabilityCommDD);
		return EFail;
		}
	else if(fn == 2)
		{
		return EPass;
		}
	else if(fn == 3)
		{
		aMissing.iCaps.AddCapability(ECapabilityCommDD);
		aAction = KQueryUser;
		return EFail;
		}
	else if(fn == 4)
		{
		aMissing.iCaps.AddCapability(ECapabilityCommDD);
		aAction = KQueryUser;
		return EFail;
		}
	else if(fn == 5)
		{
		//Since we are returning ETrue here, setting the action shouldn't affect
		//anything.  This should result in the same as 2.
		aAction = KQueryUser;
		return EPass;
		}
	//If we get here we have an unhandled condition in the test code.  The test
	//code is specifically setup to try and catch all branches through the
	//policy server.  If you get, there is some problem in the setup.
	test(0);
	return EFail;
	}

CPolicyServer::TCustomResult CTestPolicyServer1::CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing)
	{
	(void)aMissing;
	(void)aMsg;
	(void)aAction;
	TInt fn = aMsg.Function();
	if(fn >= 0)
		{
		//Connect messages don't use this debugging system
		OrInFlags(aMsg, KCustomActionMask);
		}
	switch(fn)
		{
		case 3:
			return EFail;
		case 4:
			return EPass;
		case 11:
			return EFail;
		case 12:
			return EPass;
		default:
			break;
		}

	//If we get here we have an unhandled condition in the test code.  The test
	//code is specifically setup to try and catch all branches through the
	//policy server.  If you get, there is some problem in the setup.
	test(0);
	return EFail;
	}

//
// CTestPolicyServer2
//

class CTestPolicyServer2 : public CPolicyServer
	{
public:
	CTestPolicyServer2(TInt aPriority, const TPolicy& aPolicy);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMsg) const;
	//virtual TBool CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction);
	//virtual TBool CustomFailureActionL(const TSecurityPolicy* aPolicy, const RMessage2& aMsg, TInt aAction);
	};

//
// CTestSession2
//

class CTestSession2 : public CSession2
	{
public:
	enum {EShutdown=100};
public:
	CTestSession2();
	virtual void ServiceL(const RMessage2& aMsg);
public:
	};

CTestSession2::CTestSession2()
	: CSession2()
	{
	}

void CTestSession2::ServiceL(const RMessage2& aMsg)
	{
	TInt fn = aMsg.Function();
	switch(fn)
		{
		case CTestSession2::EShutdown:
			CActiveScheduler::Stop();
			aMsg.Complete(KErrNone);
			break;
		default:
			//If we get here we have an unhandled condition in the test code.
			//The test code is specifically setup to try and catch all branches
			//through the policy server.  If you get, there is some problem in
			//the setup.
			test(0);
			break;
		}
	}

CTestPolicyServer2::CTestPolicyServer2(TInt aPriority, const TPolicy& aPolicy)
	: CPolicyServer(aPriority, aPolicy)
	{
	}

CSession2* CTestPolicyServer2::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMsg*/) const
	{
	return new (ELeave) CTestSession2();
	}

//
// CTestPolicyServer3
//

class CTestPolicyServer3 : public CPolicyServer
	{
public:
	CTestPolicyServer3(TInt aPriority, const TPolicy& aPolicy);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMsg) const;
	virtual TCustomResult CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo&);
	virtual TCustomResult CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing);
	};

CPolicyServer::TCustomResult CTestPolicyServer3::CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing)
	{
	(void)aAction;
	(void)aMsg;
	(void)aMissing;
	//If we get here we have an unhandled condition in the test code.  The test
	//code is specifically setup to try and catch all branches through the
	//policy server.  If you get, there is some problem in the setup.
	test(0);
	return EFail;
	}

CPolicyServer::TCustomResult CTestPolicyServer3::CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing)
	{
	(void)aMissing;
	(void)aMsg;
	(void)aAction;
	TInt fn = aMsg.Function();
	switch(fn)
		{
		case -1:
			return EPass;
		default:
			break;
		}

	//If we get here we have an unhandled condition in the test code.  The test
	//code is specifically setup to try and catch all branches through the
	//policy server.  If you get, there is some problem in the setup.
	test(0);
	return EFail;
	}

//
// CTestSession3
//

class CTestSession3 : public CSession2
	{
public:
	enum {EShutdown=100};
public:
	CTestSession3();
	virtual void ServiceL(const RMessage2& aMsg);
public:
	};

CTestSession3::CTestSession3()
	: CSession2()
	{
	}

void CTestSession3::ServiceL(const RMessage2& aMsg)
	{
	TInt fn = aMsg.Function();
	switch(fn)
		{
		case CTestSession3::EShutdown:
			CActiveScheduler::Stop();
			aMsg.Complete(KErrNone);
			break;
		default:
			//If we get here we have an unhandled condition in the test code.
			//The test code is specifically setup to try and catch all branches
			//through the policy server.  If you get, there is some problem in
			//the setup.
			test(0);
			break;
		}
	}

CTestPolicyServer3::CTestPolicyServer3(TInt aPriority, const TPolicy& aPolicy)
	: CPolicyServer(aPriority, aPolicy)
	{
	}

CSession2* CTestPolicyServer3::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMsg*/) const
	{
	return new (ELeave) CTestSession3();
	}

_LIT(KCustomActive, "CCustomActive");
//
// CCustomActive
//

class CCustomActive : public CActive
	{
public:
	static CCustomActive* NewL(CPolicyServer& aServer);
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();
	void CustomSecurityCheckL(const RMessage2& aMsg, TInt aAction);
	void CustomFailureActionL(const RMessage2& aMsg, TInt aAction);
protected:
	void HandleSecurityCheckResultL();
	void HandleFailureActionResultL();
	void ConstructL();	
private:
	CCustomActive(CPolicyServer& aServer);
	RTimer iTimer;
	CPolicyServer& iServer;
	const RMessage2* iMsg;
	enum TState {
		ECustomSecurityCheck,
		EFailureAction,
		};
	TState iState;
	TInt iAction;
	TSecurityInfo iMissing;
	};

CCustomActive::CCustomActive(CPolicyServer& aServer)
	: CActive(0), iServer(aServer), iMsg(0), iState(ECustomSecurityCheck)
	{
	CActiveScheduler::Add(this);
	}

CCustomActive* CCustomActive::NewL(CPolicyServer& aServer)
	{
	CCustomActive* self = new(ELeave)CCustomActive(aServer);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CCustomActive::ConstructL()
	{
	User::LeaveIfError(iTimer.CreateLocal());
	memset(&iMissing, 0, sizeof(TSecurityInfo));
	}

void CCustomActive::HandleSecurityCheckResultL()
	{
	TInt fn = iMsg->Function();
	switch(fn)
		{
		case 0:
			iStatus = KErrGeneral;
			break;
		case 1: //pass
		case 4:
			break;
		case 2:
		case 3:
			iStatus = KErrGeneral;
			iAction = KQueryUser;
			break;
		default:
			test(0);
		}
	if(iStatus == KErrNone)
		{
		iServer.ProcessL(*iMsg);
		iMsg=0;
		}
	else
		{
		const RMessage2* ptr = iMsg;
		iMsg=0;
		iServer.CheckFailedL(*ptr, iAction, iMissing);
		}
	}

void CCustomActive::HandleFailureActionResultL()
	{
	TInt fn = iMsg->Function();
	switch(fn)
		{
		case 2:
			iStatus = KErrGeneral;
			break;
		case 3: //pass
		case 9:
		case 57: //passes through so ServiceL can leave with NoMem
			break;
		case 56:
			User::Leave(KErrNoMemory);
			break;
			
		default:
			test(0);
		}
	if(iStatus == KErrNone)
		iServer.ProcessL(*iMsg);
	else
		iMsg->Complete(KErrPermissionDenied);
	iMsg=0;
	}

void CCustomActive::RunL()
	{
	OrInFlags(*iMsg, KActiveRunLMask);

	switch(iState)
		{
		case ECustomSecurityCheck:
			HandleSecurityCheckResultL();
			break;
		case EFailureAction:
			HandleFailureActionResultL();
			break;
		default:
			//Invalid state
			User::Panic(KCustomActive, 11);
			break;
		}
	}

TInt CCustomActive::RunError(TInt aError)
	{
	OrInFlags(*iMsg, KActiveRunErrorMask);
	if(iMsg)
		{
		iServer.ProcessError(*iMsg, aError);
		iMsg = 0;
		iAction = CPolicyServer::EFailClient;
		return KErrNone;
		}
	else
		return aError;
	}

void CCustomActive::DoCancel()
	{
	OrInFlags(*iMsg, KActiveDoCancelMask);
	iTimer.Cancel();
	if(iMsg)
		{
		iMsg->Complete(KErrCancel);
		}
	iMsg = 0;
	iAction = CPolicyServer::EFailClient;
	}

void CCustomActive::CustomSecurityCheckL(const RMessage2& aMsg, TInt aAction)
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KCustomActive, 1));
	__ASSERT_ALWAYS(iMsg == 0, User::Panic(KCustomActive, 2));

	OrInFlags(aMsg, KActiveCheckMask);

	iTimer.After(iStatus, 100000);
	SetActive();
	iMsg = &aMsg;
	iState = ECustomSecurityCheck;
	iAction = aAction;
	}

void CCustomActive::CustomFailureActionL(const RMessage2& aMsg, TInt aAction)
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KCustomActive, 3));
	__ASSERT_ALWAYS(iMsg == 0, User::Panic(KCustomActive, 4));

	OrInFlags(aMsg, KActiveActionMask);

	iTimer.After(iStatus, 50000);
	SetActive();
	iMsg = &aMsg;
	iState = EFailureAction;
	iAction = aAction;

	if(aMsg.Function() == 55)
		{
		Cancel();
		}
	}

//
// CTestPolicyServer4
//

class CTestPolicyServer4 : public CPolicyServer
	{
public:
	static CTestPolicyServer4* NewL(TInt aPriority, const TPolicy& aPolicy);
	CTestPolicyServer4(TInt aPriority, const TPolicy& aPolicy);
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMsg) const;
	virtual TCustomResult CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing);
	virtual TCustomResult CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing);
	CCustomActive* iActiveCheck;
protected:
	void ConstructL();
	};

CTestPolicyServer4* CTestPolicyServer4::NewL(TInt aPriority, const TPolicy& aPolicy)
	{
	CTestPolicyServer4* self = new(ELeave)CTestPolicyServer4(aPriority, aPolicy);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CTestPolicyServer4::ConstructL()
	{
	iActiveCheck = CCustomActive::NewL(*this);
	}

CPolicyServer::TCustomResult CTestPolicyServer4::CustomSecurityCheckL(const RMessage2& aMsg, TInt& aAction, TSecurityInfo& aMissing)
	{
	(void)aMissing;
	OrInFlags(aMsg, KCustomCheckMask);
	iActiveCheck->CustomSecurityCheckL(aMsg, aAction);
	return EAsync;
	}

CPolicyServer::TCustomResult CTestPolicyServer4::CustomFailureActionL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing)
	{
	(void)aMissing;
	OrInFlags(aMsg, KCustomActionMask);
	iActiveCheck->CustomFailureActionL(aMsg, aAction);
	return EAsync;
	}

//
// CTestSession4
//

class CTestSession4 : public CSession2
	{
public:
	enum {EShutdown=100};
public:
	CTestSession4();
	virtual void ServiceL(const RMessage2& aMsg);
	virtual void ServiceError(const RMessage2& aMsg, TInt aError);
public:
	};

CTestSession4::CTestSession4()
	: CSession2()
	{
	}

void CTestSession4::ServiceL(const RMessage2& aMsg)
	{
	TInt fn = aMsg.Function();
	OrInFlags(aMsg, KServiceLMask);
	switch(fn)
		{
		case 1:
		case 3:
		case 4:
		case 5:
		case 8:
		case 9:
		case 11:
			aMsg.Complete(KErrNone);
			break;
		case 57:
			User::Leave(KErrNoMemory);
			break;
		case CTestSession4::EShutdown:
			CActiveScheduler::Stop();
			aMsg.Complete(KErrNone);
			break;
		case KMaxTInt:
			//KMaxTInt would otherwise interfere with the OrInFlags value, so
			//we return our location this way instead.
			aMsg.Complete(KMaxTInt);
			break;
		default:
			//If we get here we have an unhandled condition in the test code.
			//The test code is specifically setup to try and catch all branches
			//through the policy server.  If you get, there is some problem in
			//the setup.
			test(0);
			break;
		}
	}

void CTestSession4::ServiceError(const RMessage2& aMsg, TInt aError)
	{
	OrInFlags(aMsg, KServiceErrorMask);
	if(!aMsg.IsNull())
		aMsg.Complete(aError);
	}

CTestPolicyServer4::CTestPolicyServer4(TInt aPriority, const TPolicy& aPolicy)
	: CPolicyServer(aPriority, aPolicy)
	{
	}

CSession2* CTestPolicyServer4::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMsg*/) const
	{
	return new (ELeave) CTestSession4();
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

_LIT(KServerName,"T_POLSVR-server");
const TInt KServerRendezvous = KRequestPending+1;

void DoStartServer(const CPolicyServer::TPolicy& aPolicy, TUint aServerIndex)
	{
	CTestActiveScheduler* activeScheduler = new (ELeave) CTestActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	CleanupStack::PushL(activeScheduler);

	CPolicyServer* server = 0;
	switch(aServerIndex)
		{
		case 0:
			server = new (ELeave) CTestPolicyServer1(0,aPolicy);
			break;
		case 1:
			server = new (ELeave) CTestPolicyServer2(0,aPolicy);
			break;
		case 2:
			server = new (ELeave) CTestPolicyServer3(0,aPolicy);
			break;
		case 3:
			server = CTestPolicyServer4::NewL(0,aPolicy);
			break;
		default:
			User::Panic(KPolSvr, KErrArgument);
		}
	CleanupStack::PushL(server);

	User::LeaveIfError(server->Start(KServerName));

	RProcess::Rendezvous(KServerRendezvous);

	CActiveScheduler::Start();

	CleanupStack::PopAndDestroy(2);
	}

TInt StartServer(const CPolicyServer::TPolicy& aPolicy, TUint aServerIndex)
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		return KErrNoMemory;
	TRAPD(leaveError,DoStartServer(aPolicy,aServerIndex))
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
//	inline TInt Send(TInt aFunction)
//		{ return RSessionBase::SendReceive(aFunction); }
	inline TInt Send(TInt aFunction,const TIpcArgs& aArgs)
		{ return RSessionBase::SendReceive(aFunction,aArgs); }
	inline void Send(TInt aFunction,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aStatus); }
	inline void Send(TInt aFunction,const TIpcArgs& aArgs,TRequestStatus& aStatus)
		{ RSessionBase::SendReceive(aFunction,aArgs,aStatus); }
	};



RTestSession Session;

#include <e32svr.h>

void TestServer1WithPolicy1()
	{
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy1,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	test.Next(_L("Server 1, Policy 1"));
	TInt r = Session.Connect();
	test(r==KErrNone);

	TBuf8<4> flags(4);

	//case 0: Not Supported, returned from policy server level.
	SetFlags(flags, 0);
	r = Session.Send(0, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (0));

	//case 1: Custom Check, fails with KErrPermissionDenied
	SetFlags(flags, 1);
	r = Session.Send(1, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (1 | KCustomCheckMask) ); 

	//case 2: Custom Check passes.
	SetFlags(flags, 2);
	r = Session.Send(2, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (2 | KCustomCheckMask | KServiceLMask) ); 

	//case 3: Custom Check fails but action set to EQueryUser, query
	//subsequently fails
	SetFlags(flags, 3);
	r = Session.Send(3, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (3 | KCustomCheckMask | KCustomActionMask ));

	//case 4: Custom Check fails but action set to EQueryUser, query
	//subsequently passes
	SetFlags(flags, 4);
	r = Session.Send(4, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (4 | KCustomCheckMask | KCustomActionMask | KServiceLMask ));
	
	//case 5: Custom Check passes and action is set.  Action set shouldn't make
	//a difference.  Should be same result as case 2.
	SetFlags(flags, 5);
	r = Session.Send(5, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (5 | KCustomCheckMask | KServiceLMask ));

	//case 6: Always passes at the policy server level.
	SetFlags(flags, 6);
	r = Session.Send(6, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (6 | KServiceLMask) );

	//case 7: Not Supported, returned from policy server level.
	SetFlags(flags, 7);
	r = Session.Send(7, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (7));

	//case 8: Requires NetworkControl, which we have, so it passes.
	SetFlags(flags, 8);
	r = Session.Send(8, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (8 | KServiceLMask));

	//case 9: Requires NetworkControl -> pass.  Thrown in a EQueryUser to see
	//if it causes any problems -> it shouldn't.  Should be same as case 8.
	SetFlags(flags, 9);
	r = Session.Send(9, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (9 | KServiceLMask));

	//case 10: Requires DiskAdmin which we don't have.  
	SetFlags(flags, 10);
	r = Session.Send(10, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (10));

	//case 11: Requires DiskAdmin, which we don't have. EQueryUser is set, and
	//it fails.
	SetFlags(flags, 11);
	r = Session.Send(11, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (11 | KCustomActionMask));

	//case 12: Requires DiskAdmin, which we don't have. EQueryUser is set, and
	//it passes.
	SetFlags(flags, 12);
	r = Session.Send(12, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (12 | KCustomActionMask | KServiceLMask));

	//case 13: Not Supported, returned from policy server level.
	SetFlags(flags, 13);
	r = Session.Send(13, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (13));

	//case 14: Not Supported, returned from policy server level.
	SetFlags(flags, 14);
	r = Session.Send(14, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (14));

	//case 55: Not Supported, returned from policy server level.
	SetFlags(flags, 55);
	r = Session.Send(55, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (55));

	//case 86: Not Supported, returned from policy server level.
	SetFlags(flags, 86);
	r = Session.Send(86, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (86));

	//case 99: Not Supported, returned from policy server level.
	SetFlags(flags, 99);
	r = Session.Send(99, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (99));

	//case 101: Not Supported, returned from policy server level.
	SetFlags(flags, 101);
	r = Session.Send(101, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (101));

	//case 1000191: Not Supported, returned from policy server level.
	SetFlags(flags, 1000191);
	r = Session.Send(1000191, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (1000191));

	//case 1000848: Not Supported, returned from policy server level.
	SetFlags(flags, 1000848);
	r = Session.Send(1000848, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (1000848));

	//case KMaxTInt-1: Not Supported, returned from policy server level.
	SetFlags(flags, KMaxTInt-1);
	r = Session.Send(KMaxTInt-1, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (KMaxTInt-1));

	//case KMaxTInt: Not Supported, returned from policy server level.
	SetFlags(flags, KMaxTInt);
	r = Session.Send(KMaxTInt, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (TUint)(KMaxTInt));

	r = Session.Send(CTestSession1::EShutdown, TIpcArgs(&flags));
	test.Printf(_L("r = %d\n"),r);
	test(r==KErrNone);

	Session.Close();
	CLOSE_AND_WAIT(server);
	}

#include <e32panic.h>

void TestServer1WithPolicy2()
	{
	test.Next(_L("Server 1, Policy 2"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy2,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
#ifdef _DEBUG
	//Debug mode does a policy integrity check
	test(rendezvous==EPolSvr1stRangeNotZero);
#else
	test(rendezvous==KServerRendezvous);
	server.Terminate(0);
#endif
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy3()
	{
	test.Next(_L("Server 1, Policy 3"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy3,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
#ifdef _DEBUG
	//Debug mode does a policy integrity check
	test(rendezvous==EPolSvrRangesNotIncreasing);
#else
	test(rendezvous==KServerRendezvous);
	server.Terminate(0);
#endif
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy4()
	{
	test.Next(_L("Server 1, Policy 4"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy4,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
#ifdef _DEBUG
	//Debug mode does a policy integrity check
	test(rendezvous==EPolSvrElementsIndexValueInvalid);
#else
	test(rendezvous==KServerRendezvous);
	server.Terminate(0);
#endif
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy5()
	{
	test.Next(_L("Server 1, Policy 5"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy5,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
#ifdef _DEBUG
	//Debug mode does a policy integrity check
	test(rendezvous==EPolSvrElementsIndexValueInvalid);
#else
	test(rendezvous==KServerRendezvous);
	server.Terminate(0);
#endif
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy6()
	{
	test.Next(_L("Server 1, Policy 6"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy6,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
#ifdef _DEBUG
	//Debug mode does a policy integrity check
	test(rendezvous==EPolSvrIOnConnectValueInvalid);
#else
	test(rendezvous==KServerRendezvous);
	server.Terminate(0);
#endif
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy7()
	{
	test.Next(_L("Server 1, Policy 7"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy7,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	TInt r = Session.Connect();
	test(r==KErrNotSupported);

	//We can do this because we have power management
	server.Terminate(KErrGeneral);
	CLOSE_AND_WAIT(server);
	}

void TestServer1WithPolicy8()
	{
	test.Next(_L("Server 1, Policy 8"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy8,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	//This will be calling through CustomSecurityCheckL (server1 will return
	//pass), but there is no easy way to determine that it has followed the
	//correct path.
	TInt r = Session.Connect();
	test(r==KErrNone);

	server.Terminate(KErrGeneral);
	Session.Close();
	CLOSE_AND_WAIT(server);
	}

void TestServer2WithPolicy8()
	{
	test.Next(_L("Server 2, Policy 8"));
	RTestProcess server;
	TRequestStatus rendezvous;
	TRequestStatus logon;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy8,ETestServer2);
	server.Rendezvous(rendezvous);
	server.Logon(logon);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	TInt r = Session.Connect();
	test(r==KErrServerTerminated);

	//This is a simple way of testing that CustomSecurityCheckL is called for a
	//connect custom check.  Server2 doesn't have an implementation of CustomSecurityCheckL
	User::WaitForRequest(logon);
	test(logon == EPolSvrCallingBaseImplementation);

	Session.Close();
	CLOSE_AND_WAIT(server);
	}

void TestServer3WithPolicy8()
	{
	test.Next(_L("Server 3, Policy 8"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy8,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	//This will be calling through CustomSecurityCheckL
	//(server3::CustomSecurityCheckL will fail this, but set the action to
	//EQueryUser, which will call CustomFailureActionL which should will in
	//this case pass) but there is no easy way to determine that it has
	//followed the correct path.
	TInt r = Session.Connect();
	test(r==KErrNone);

	//This policy doesn't have any IPC's that work.  Only way to shutdown
	//server is to kill it.
	server.Terminate(KErrGeneral);
	Session.Close();
	CLOSE_AND_WAIT(server);
	}


void TestServer1WithPolicy9()
	{
	test.Next(_L("Server 1, Policy 9"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy9,ETestServer1);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	TInt r = Session.Connect();
	test(r==KErrPermissionDenied);

	//We can do this because we have power management
	server.Terminate(KErrGeneral);
	CLOSE_AND_WAIT(server);
	}

void TestServer2WithPolicy1()
	{	
	test.Next(_L("Server 2, Policy 1"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy1,ETestServer2);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	TInt r = Session.Connect();
	test(r==KErrNone);

	TBuf8<4> flags(4);
	r = Session.Send(CTestSession2::EShutdown, TIpcArgs(&flags));
	test(r == KErrNone);
	Session.Close();
	CLOSE_AND_WAIT(server);
	}

void TestServer4WithPolicy10()
	{	
	test.Next(_L("Server 4, Policy 10"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessPolicyServer,EPolicy10,ETestServer4);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	TInt r = Session.Connect();
	test(r==KErrNone);

	TBuf8<4> flags(4);

	//case 0: Custom Check, fails with KErrPermissionDenied
	SetFlags(flags, 0);
	r = Session.Send(0, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (0 | KCustomCheckMask | KActiveCheckMask | KActiveRunLMask) ); 

	//case 1: Custom Check passes.
	SetFlags(flags, 1);
	r = Session.Send(1, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (1 | KCustomCheckMask | KActiveCheckMask | KActiveRunLMask | KServiceLMask) ); 

	//case 2: Custom Check fails but action set to EQueryUser, query
	//subsequently fails
	SetFlags(flags, 2);
	r = Session.Send(2, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (2 | KCustomCheckMask | KActiveCheckMask | KActiveRunLMask | KCustomActionMask | KActiveActionMask ));

	//case 3: Custom Check fails but action set to EQueryUser, query
	//subsequently passes
	SetFlags(flags, 3);
	r = Session.Send(3, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (3 | KCustomCheckMask | KActiveCheckMask | KActiveRunLMask | KCustomActionMask | KActiveActionMask | KServiceLMask ));
	
	//case 4: Custom Check passes and action is set.  Action set shouldn't make
	//a difference.  Should be same result as case 1.
	SetFlags(flags, 4);
	r = Session.Send(4, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (4 | KCustomCheckMask | KActiveCheckMask | KActiveRunLMask | KServiceLMask) );

	//case 5: Always passes at the policy server level.
	SetFlags(flags, 5);
	r = Session.Send(5, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (5 | KServiceLMask) );

	//case 6: Not Supported, returned from policy server level.
	SetFlags(flags, 6);
	r = Session.Send(6, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (6));

	//case 7: Not Supported, returned from policy server level.
	SetFlags(flags, 7);
	r = Session.Send(7, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (7));

	//case 8: Requires NetworkControl, which we have, so it passes.
	SetFlags(flags, 8);
	r = Session.Send(8, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (8 | KServiceLMask));

	//case 9: Requires DiskAdmin, which we don't have. EQueryUser is set, and
	//it passes.
	SetFlags(flags, 9);
	r = Session.Send(9, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (9 | KCustomActionMask | KActiveActionMask | KActiveRunLMask | KServiceLMask));

	//case 10: Requires DiskAdmin which we don't have.  
	SetFlags(flags, 10);
	r = Session.Send(10, TIpcArgs(&flags));
	test(r==KErrPermissionDenied);
	test(FlagsValue(flags) == (10));

	//case 11: Requires NetworkControl -> pass.  Thrown in a EQueryUser to see
	//if it causes any problems -> it shouldn't.  Should be same as case 8.
	SetFlags(flags, 11);
	r = Session.Send(11, TIpcArgs(&flags));
	test(r==KErrNone);
	test(FlagsValue(flags) == (11 | KServiceLMask));

	//case 12: Not Supported, returned from policy server level.
	SetFlags(flags, 12);
	r = Session.Send(12, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (12));

	//case 13: Not Supported, returned from policy server level.
	SetFlags(flags, 13);
	r = Session.Send(13, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (13));

	//case 54: Not Supported, returned from policy server level.
	SetFlags(flags, 54);
	r = Session.Send(54, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (54));

	//case 55: Requires DiskAdmin -> Fail.  But then we query user, and then we
	//cancel that.
	SetFlags(flags, 55);
	r = Session.Send(55, TIpcArgs(&flags));
	test(r==KErrCancel);
	test(FlagsValue(flags) == (55 | KCustomActionMask | KActiveActionMask | KActiveDoCancelMask));

	//case 56: Requires DiskAdmin -> Fail.  But then we query user which leaves.
	SetFlags(flags, 56);
	r = Session.Send(56, TIpcArgs(&flags));
	test(r==KErrNoMemory);
	test(FlagsValue(flags) == (56 | KCustomActionMask | KActiveActionMask | KActiveRunLMask | KActiveRunErrorMask));

	//case 57: Requires DiskAdmin -> Fail.  But then we query user which passes
	//and then we leave in the ServiceL
	SetFlags(flags, 57);
	r = Session.Send(57, TIpcArgs(&flags));
	test(r==KErrNoMemory);
	test(FlagsValue(flags) == (57 | KCustomActionMask | KActiveActionMask | KActiveRunLMask | KServiceLMask | KActiveRunErrorMask | KServiceErrorMask ));

	//case 58: Not Supported, returned from policy server level.
	SetFlags(flags, 58);
	r = Session.Send(58, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (58));

	//case 86: Not Supported, returned from policy server level.
	SetFlags(flags, 86);
	r = Session.Send(86, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (86));

	//case 99: Not Supported, returned from policy server level.
	SetFlags(flags, 99);
	r = Session.Send(99, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (99));

	//case 101: Not Supported, returned from policy server level.
	SetFlags(flags, 101);
	r = Session.Send(101, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (101));

	//case 5000: Not Supported, returned from policy server level.
	SetFlags(flags, 5000);
	r = Session.Send(5000, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (5000));

	//case KMaxTInt-1: Not Supported, returned from policy server level.
	SetFlags(flags, KMaxTInt-1);
	r = Session.Send(KMaxTInt-1, TIpcArgs(&flags));
	test(r==KErrNotSupported);
	test(FlagsValue(flags) == (KMaxTInt-1));

	//case KMaxTInt: Always pass from policy framework
	//This also tests another exit condition from the binary search
	SetFlags(flags, 0);
	r = Session.Send(KMaxTInt, TIpcArgs(&flags));
	//Instead of KErrNone we return KMaxTInt as we can't fit the KMaxTInt in
	//the flags without overwriting stuff
	test(r==KMaxTInt);
	test(FlagsValue(flags) == (0 | KServiceLMask ));

	r = Session.Send(CTestSession2::EShutdown, TIpcArgs(&flags));
	test(r == KErrNone);
	Session.Close();
	CLOSE_AND_WAIT(server);
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

	test.Start(_L("Policy Server Tests"));

	/* Server1 has implementations of CustomSecurityCheckL and
	CustomFailureActionL This is the test for all the paths in a policy servers
	implementations -- not all connect paths though
	This test also ensures that every path through the binary search is
	covered. */
	TestServer1WithPolicy1();

	/* Policy2,3,4,5,6, are bad policies that should cause the server to panic
	in debug mode.  In release, they'll just pass, however, if you later tried
	to use them, something would go horribly wrong */
	TestServer1WithPolicy2();
	TestServer1WithPolicy3();
	TestServer1WithPolicy4();
	TestServer1WithPolicy5();
	TestServer1WithPolicy6();

	/* Policies 7,8,9 check various types of connect policies. */
	TestServer1WithPolicy7();
	// Server 1,2,3 are used here all with policy 8 because you can only test 1
	// type of connect policy per server.
	TestServer1WithPolicy8();
	TestServer2WithPolicy8();
	TestServer3WithPolicy8();
	TestServer1WithPolicy9();

	/* Sever2 does not have implementations of CustomSecurityCheckL and
	CustomFailureActionL. When these functions are called it should crash */
	TestServer2WithPolicy1();

	/* Server4 is used for checking what happens when the custom functions use
	another active object.  This test encompasses leaving due to OOM, and
	cancellation. */
	TestServer4WithPolicy10();

	test.End();
	return(0);
    }

