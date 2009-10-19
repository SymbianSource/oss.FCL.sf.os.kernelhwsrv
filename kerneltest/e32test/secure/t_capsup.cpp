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
// e32test\secure\t_capsup.cpp
// Overview:
// Test support of platform security capabilities.
// API Information:
// TCapabilitySet, TSecurityInfo, TSecurityPolicy and various other methods.
// Details:
// - Verify the enumeration of each of the capability names.
// - Test the TCapabilitySet class methods by setting and checking various
// capabilities. Verify results are as expected.
// - Test the TSecurityInfo class methods and verify results.
// - Test the RProcess, RThread and RMessage2 SecureId() methods along with
// User::CreatorSecureId(). Verify results are as expected.
// - Test the RProcess, RThread and RMessage2 VendorId() methods along with
// User::CreatorVendorId(). Verify results are as expected.
// - Test the RProcess, RThread and RMessage2 HasCapability() methods along
// with User::CreatorHasCapability(). Verify results are as expected.
// - Test TSecurityPolicy constructors including macros for compile-time
// construction. Verify results are as expected.
// - Test kernel APIs.  Verify results are as expected.
// - Test setting KernelConfigFlags.  Ensure that, if __PLATSEC_UNLOCKED__
// is not set, the PlatSec flags cannot be unset.
// - Test the use of platform security diagnostic strings.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __INCLUDE_CAPABILITY_NAMES__
#define __E32TEST_EXTENSION__

#include <e32cmn.h>
#include <e32cmn_private.h>
#include <u32exec.h>
#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "d_sldd.h"

TBool PlatSecEnforcement;

_LIT_SECURE_ID(KTestSecureId,0x101f534d);
_LIT_SECURE_ID(KTestSecureId2,0x101f534e);
_LIT_VENDOR_ID(KTestVendorId,0x01234567);
_LIT_VENDOR_ID(KTestVendorId2,0x01234568);

const TUint32 KTestCapabilities =(1<<ECapabilityTCB)
								|(1<<ECapabilityPowerMgmt)
								|(1<<ECapabilityReadDeviceData)
								|(1<<ECapabilityDRM)
								|(1<<ECapabilityProtServ)
								|(1<<ECapabilityNetworkControl)
								|(1<<ECapabilitySwEvent)
								|(1<<ECapabilityLocalServices)
								|(1<<ECapabilityWriteUserData)
								|(1<<ECapabilitySurroundingsDD);

LOCAL_D RTest test(_L("T_CAPSUP"));

enum TTestProcessFunctions
	{
	ETestProcessServer,
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

	default:
		User::Panic(_L("T_CAPSUP"),1);
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
	enum {EShutdown,
		ETestProcessSecurityInfo,ETestThreadSecurityInfo,ETestMessageSecurityInfo,ETestCreatorSecurityInfo,
		ETestProcessSecureId,ETestThreadSecureId,ETestMessageSecureId,ETestCreatorSecureId,
		ETestProcessVendorId,ETestThreadVendorId,ETestMessageVendorId,ETestCreatorVendorId,
		ETestProcessHasCapability1,ETestProcessHasCapability2,
		ETestThreadHasCapability1,ETestThreadHasCapability2,
		ETestMessageHasCapability1,ETestMessageHasCapability2,
		ETestMessageHasCapabilityL1,ETestMessageHasCapabilityL2,
		ETestCreatorHasCapability1,ETestCreatorHasCapability2,
		ETestSecurityPolicyAgainstMessage,
		ETestSecurityPolicyAgainstCreator
		};
	enum {EPolicyCheckPassed = 15, EPolicyCheckFailed = 16};
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
	const RMessagePtr2 m(aMessage);
	switch (aMessage.Function())
		{
		case CTestSession::ETestProcessSecureId:
			{
			RProcess process;
			TInt r=process.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = process.SecureId();
				process.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestThreadSecureId:
			{
			RThread thread;
			TInt r=thread.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = thread.SecureId();
				thread.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageSecureId:
			{
			TInt32 id = m.SecureId();
			m.Complete(id);
			}
			break;

		case CTestSession::ETestCreatorSecureId:
			{
			m.Complete(User::CreatorSecureId());
			}
			break;

		case CTestSession::ETestProcessVendorId:
			{
			RProcess process;
			TInt r=process.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = process.VendorId();
				process.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestThreadVendorId:
			{
			RThread thread;
			TInt r=thread.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = thread.VendorId();
				thread.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageVendorId:
			{
			TInt32 id = m.VendorId();
			m.Complete(id);
			}
			break;

		case CTestSession::ETestCreatorVendorId:
			{
			m.Complete(User::CreatorVendorId());
			}
			break;

		case CTestSession::ETestProcessSecurityInfo:
			{
			RProcess process;
			TInt r=process.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				TSecurityInfo info;
				info.Set(process);
				r = m.Write(1,TPtrC8((TUint8*)&info,sizeof(info)));
				process.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestThreadSecurityInfo:
			{
			RThread thread;
			TInt r=thread.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				TSecurityInfo info;
				info.Set(thread);
				r = m.Write(1,TPtrC8((TUint8*)&info,sizeof(info)));
				thread.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageSecurityInfo:
			{
			TSecurityInfo info;
			info.Set(m);
			TInt r = m.Write(0,TPtrC8((TUint8*)&info,sizeof(info)));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestCreatorSecurityInfo:
			{
			TSecurityInfo info;
			info.SetToCreatorInfo();
			TInt r = m.Write(0,TPtrC8((TUint8*)&info,sizeof(info)));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestProcessHasCapability1:
			{
			RProcess process;
			TInt r=process.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = process.HasCapability((TCapability)aMessage.Int1(),__PLATSEC_DIAGNOSTIC_STRING("ETestProcessHasCapability1"));
				process.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestProcessHasCapability2:
			{
			RProcess process;
			TInt r=process.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = process.HasCapability((TCapability)aMessage.Int1(),(TCapability)aMessage.Int2(),__PLATSEC_DIAGNOSTIC_STRING("ETestProcessHasCapability2"));
				process.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestThreadHasCapability1:
			{
			RThread thread;
			TInt r=thread.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = thread.HasCapability((TCapability)aMessage.Int1(),__PLATSEC_DIAGNOSTIC_STRING("ETestThreadHasCapability1"));
				thread.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestThreadHasCapability2:
			{
			RThread thread;
			TInt r=thread.Open(aMessage.Int0());
			if(r==KErrNone)
				{
				r = thread.HasCapability((TCapability)aMessage.Int1(),(TCapability)aMessage.Int2(),__PLATSEC_DIAGNOSTIC_STRING("ETestThreadHasCapability2"));
				thread.Close();
				}
			else
				r = KErrGeneral;
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageHasCapability1:
			{
			TInt r = m.HasCapability((TCapability)aMessage.Int0(),__PLATSEC_DIAGNOSTIC_STRING("ETestMessageHasCapability1"));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageHasCapabilityL1:
			{
			TRAPD(r,m.HasCapabilityL((TCapability)aMessage.Int0(),__PLATSEC_DIAGNOSTIC_STRING("ETestMessageHasCapabilityL1")))
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageHasCapability2:
			{
			TInt r = m.HasCapability((TCapability)aMessage.Int0(),(TCapability)aMessage.Int1(),__PLATSEC_DIAGNOSTIC_STRING("ETestMessageHasCapability2"));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestMessageHasCapabilityL2:
			{
			TRAPD(r,m.HasCapabilityL((TCapability)aMessage.Int0(),(TCapability)aMessage.Int1(),__PLATSEC_DIAGNOSTIC_STRING("ETestMessageHasCapabilityL2")))
			m.Complete(r);
			}
			break;

		case CTestSession::ETestCreatorHasCapability1:
			{
			TInt r = User::CreatorHasCapability((TCapability)aMessage.Int0(),__PLATSEC_DIAGNOSTIC_STRING("ETestCreatorHasCapability1"));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestCreatorHasCapability2:
			{
			TInt r = User::CreatorHasCapability((TCapability)aMessage.Int0(),(TCapability)aMessage.Int1(),__PLATSEC_DIAGNOSTIC_STRING("ETestCreatorHasCapability2"));
			m.Complete(r);
			}
			break;

		case CTestSession::ETestSecurityPolicyAgainstMessage:
			{
			TBuf8<sizeof(TSecurityPolicy)> buf(0);
			TInt len = m.GetDesLength(0);
			TInt r = KErrArgument;
			if(len>0 && len <=buf.MaxSize())
				{
				r = m.Read(0, buf, 0);
				if(r==KErrNone)
					{
					TSecurityPolicy policy;
					r = policy.Set(buf);
					if(r == KErrNone)
						{
						r = policy.CheckPolicy(m, __PLATSEC_DIAGNOSTIC_STRING("Testing message against policy -- sample additional diagnostic."));
						if(r)
							r = EPolicyCheckPassed;
						else
							r = EPolicyCheckFailed;
						}
					}
				}
			m.Complete(r);
			break;
			}

		case CTestSession::ETestSecurityPolicyAgainstCreator:
			{
			TBuf8<sizeof(TSecurityPolicy)> buf(0);
			TInt len = m.GetDesLength(0);
			TInt r = KErrArgument;
			if(len>0 && len <=buf.MaxSize())
				{
				r = m.Read(0, buf, 0);
				if(r==KErrNone)
					{
					TSecurityPolicy policy;
					r = policy.Set(buf);
					if(r == KErrNone)
						{
						r = policy.CheckPolicyCreator(__PLATSEC_DIAGNOSTIC_STRING("Testing creator against policy -- sample additional diagnostic."));
						if(r)
							r = EPolicyCheckPassed;
						else
							r = EPolicyCheckFailed;
						}
					}
				}
			m.Complete(r);
			break;
			}

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

_LIT(KServerName,"T_CAPSUP-server");
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
		return KErrNoMemory;
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
		test((!a1.HasCapability((TCapability)i))==(!a2.HasCapability((TCapability)i)));
	}

void TestCapabilitySet()
	{
	TCapabilitySet s;
	TCapabilitySet all;
	TCapabilitySet empty;
	TInt i,j,k;

	test.Start(_L("Each test stage also implicity tests HasCapability()"));

	test.Next(_L("Test SetEmpty()"));
	memset(&empty,-1,sizeof(empty));
	empty.SetEmpty();
	for(i=0; i<ECapability_HardLimit; i++)
		test(!empty.HasCapability((TCapability)i));
	// test memory cleared - assumes knowledge of internal representation
	for(i=0; i<(TInt)sizeof(empty); ++i)
		test(((TUint8*)&empty)[i] == 0);

	test.Next(_L("Test SetAllSupported()"));
	all.SetAllSupported();
	// This code makes the assumption that there are no gaps in the TCapability enumeration
	for(i=0; i<ECapability_Limit; i++)
		test(all.HasCapability((TCapability)i));
	for(; i<ECapability_HardLimit; i++)
		test(!all.HasCapability((TCapability)i));

	test.Next(_L("Test Set(TCapability)"));
	for(j=-2; j<ECapability_HardLimit; j++)
		{
		if(j&1)
			s.SetAllSupported();
		else
			s.SetEmpty();
		s.Set((TCapability)j);
		for(i=0; i<=ECapability_HardLimit; i++)
			{
			if(i==j)
				test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
			else
				test(!s.HasCapability((TCapability)i));
			
			}
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}

	test.Next(_L("Test Set(TCapability,TCapability)"));
	for(k=-2; k<ECapability_HardLimit; k++)
		{
		for(j=-2; j<ECapability_HardLimit; j++)
			{
			if((j^k)&1)
				s.SetAllSupported();
			else
				s.SetEmpty();
			s.Set((TCapability)j,(TCapability)k);
			for(i=0; i<=ECapability_HardLimit; i++)
				{
				if(i==j || i==k)
					test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
				else
					test(!s.HasCapability((TCapability)i));

				}
			test(!s.HasCapability(ECapability_Denied));
			test(s.HasCapability(ECapability_None));
			}
		}

	test.Next(_L("Test TCapability(TCapability)"));
	for(j=-2; j<ECapability_HardLimit; j++)
		{
		TCapabilitySet t((TCapability)j);
		for(i=0; i<=ECapability_HardLimit; i++)
			{
			if(i==j)
				test(!t.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
			else
				test(!t.HasCapability((TCapability)i));

			}
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}

	test.Next(_L("Test TCapability(TCapability,TCapability)"));
	for(k=-2; k<ECapability_HardLimit; k++)
		{
		for(j=-2; j<ECapability_HardLimit; j++)
			{
			TCapabilitySet t((TCapability)j,(TCapability)k);
			for(i=0; i<=ECapability_HardLimit; i++)
				{
				if(i==j || i==k)
					test(!t.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
				else
					test(!t.HasCapability((TCapability)i));

				}
			test(!s.HasCapability(ECapability_Denied));
			test(s.HasCapability(ECapability_None));
			}
		}

	test.Next(_L("Test AddCapability(TCapability)"));
	s.SetEmpty();
	for(j=-2; j<ECapability_HardLimit; j++) // Add each capability in sequence
		{
		s.AddCapability((TCapability)j);
		for(i=0; i<=j; i++)
			test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
		for(; i<ECapability_HardLimit; i++)
			test(!s.HasCapability((TCapability)i));
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}
	s.SetEmpty();
	for(j=ECapability_HardLimit-1; j>=-2; j--) // Add each capability in reverse sequence
		{
		s.AddCapability((TCapability)j);
		for(i=ECapability_HardLimit-1; i>=j && i>=0; i--)
			test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
		for(; i>=0; i--)
			test(!s.HasCapability((TCapability)i));
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}

	test.Next(_L("Test RemoveCapability(TCapability)"));
	s.SetAllSupported();
	for(j=-2; j<ECapability_HardLimit; j++) // Remove each capability in sequence
		{
		s.RemoveCapability((TCapability)j);
		for(i=0; i<=j; i++)
			test(!s.HasCapability((TCapability)i));
		for(; i<ECapability_HardLimit; i++)
			test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}
	s.SetAllSupported();
	for(j=ECapability_HardLimit-1; j>=-2; j--) // Remove each capability reverse sequence
		{
		s.RemoveCapability((TCapability)j);
		for(i=ECapability_HardLimit-1; i>=j && i >=0; i--)
			test(!s.HasCapability((TCapability)i));
		for(; i>=0; i--)
			test(!s.HasCapability((TCapability)i) == !all.HasCapability((TCapability)i)); // Should have capability (if in set of All capabilities)
		test(!s.HasCapability(ECapability_Denied));
		test(s.HasCapability(ECapability_None));
		}

	test.Next(_L("Building test sets"));
	TCapabilitySet even;
	even.SetEmpty();
	for(i=0; i<ECapability_Limit; i+=2) even.AddCapability((TCapability)i);
	TCapabilitySet odd;
	odd.SetEmpty();
	for(i=1; i<ECapability_Limit; i+=2) odd.AddCapability((TCapability)i);

	test.Next(_L("Test Union(TCapabilitySet)"));
	s.SetEmpty();
	CheckCapabilitySetEqual(s,empty);
	s.Union(odd);
	CheckCapabilitySetEqual(s,odd);
	s.Union(odd);
	CheckCapabilitySetEqual(s,odd);
	s.Union(empty);
	CheckCapabilitySetEqual(s,odd);
	s.Union(even);
	CheckCapabilitySetEqual(s,all);
	s.Union(even);
	CheckCapabilitySetEqual(s,all);
	s.SetEmpty();
	CheckCapabilitySetEqual(s,empty);
	s.Union(even);
	CheckCapabilitySetEqual(s,even);
	s.Union(even);
	CheckCapabilitySetEqual(s,even);
	s.Union(empty);
	CheckCapabilitySetEqual(s,even);
	s.Union(odd);
	CheckCapabilitySetEqual(s,all);
	s.Union(odd);
	CheckCapabilitySetEqual(s,all);

	test.Next(_L("Test Intersection(TCapabilitySet)"));
	s.SetAllSupported();
	CheckCapabilitySetEqual(s,all);
	s.Intersection(odd);
	CheckCapabilitySetEqual(s,odd);
	s.Intersection(odd);
	CheckCapabilitySetEqual(s,odd);
	s.Intersection(even);
	CheckCapabilitySetEqual(s,empty);
	s.Intersection(even);
	CheckCapabilitySetEqual(s,empty);
	s.SetAllSupported();
	CheckCapabilitySetEqual(s,all);
	s.Intersection(even);
	CheckCapabilitySetEqual(s,even);
	s.Intersection(even);
	CheckCapabilitySetEqual(s,even);
	s.Intersection(odd);
	CheckCapabilitySetEqual(s,empty);
	s.Intersection(odd);
	CheckCapabilitySetEqual(s,empty);

	test.Next(_L("Test Remove(TCapabilitySet)"));
	s.SetAllSupported();
	CheckCapabilitySetEqual(s,all);
	s.Remove(odd);
	CheckCapabilitySetEqual(s,even);
	s.Remove(odd);
	CheckCapabilitySetEqual(s,even);
	s.Remove(empty);
	CheckCapabilitySetEqual(s,even);
	s.Remove(even);
	CheckCapabilitySetEqual(s,empty);
	s.Remove(even);
	CheckCapabilitySetEqual(s,empty);
	s.SetAllSupported();
	CheckCapabilitySetEqual(s,all);
	s.Remove(even);
	CheckCapabilitySetEqual(s,odd);
	s.Remove(even);
	CheckCapabilitySetEqual(s,odd);
	s.Remove(empty);
	CheckCapabilitySetEqual(s,odd);
	s.Remove(odd);
	CheckCapabilitySetEqual(s,empty);
	s.Remove(odd);
	CheckCapabilitySetEqual(s,empty);

	test.Next(_L("Test HasCapabilities(TCapabilitySet)"));
	for(i=0; i<ECapability_Limit; i++)
		{
		if(!all.HasCapability((TCapability)i))
			continue;
		TCapabilitySet t((TCapability)i);
		test(t.HasCapabilities(t));
		test(all.HasCapabilities(t));
		test(!empty.HasCapabilities(t));
		if(i&1)
			{
			test(odd.HasCapabilities(t));
			test(!even.HasCapabilities(t));
			}
		else
			{
			test(!odd.HasCapabilities(t));
			test(even.HasCapabilities(t));
			}
		test(!t.HasCapabilities(all));
		test(!t.HasCapabilities(even));
		test(!t.HasCapabilities(odd));
		test(t.HasCapabilities(empty));
		}

	test(all.HasCapabilities(all));
	test(all.HasCapabilities(even));
	test(all.HasCapabilities(odd));
	test(all.HasCapabilities(empty));

	test(!even.HasCapabilities(all));
	test(even.HasCapabilities(even));
	test(!even.HasCapabilities(odd));
	test(even.HasCapabilities(empty));

	test(!odd.HasCapabilities(all));
	test(!odd.HasCapabilities(even));
	test(odd.HasCapabilities(odd));
	test(odd.HasCapabilities(empty));

	test(!empty.HasCapabilities(all));
	test(!empty.HasCapabilities(even));
	test(!empty.HasCapabilities(odd));
	test(empty.HasCapabilities(empty));

	test.End();
	}

void CheckSecurityInfo(const TSecurityInfo& a1,const TSecurityInfo& a2)
	{
	test(a1.iSecureId==a2.iSecureId);
	test(a1.iVendorId==a2.iVendorId);
	TInt i;
	for(i=0; i<ECapability_Limit; i++)
		test((!a1.iCaps.HasCapability((TCapability)i))==(!a2.iCaps.HasCapability((TCapability)i)));
	}

void TestSecurityInfo()
	{
	TSecurityInfo self;
	TSecurityInfo info;
	TPckg<TSecurityInfo> infoPtr(info);
	TInt i;

	test.Start(_L("Test TSecurityInfo::SetToCurrentInfo"));
	self.SetToCurrentInfo();
	test(self.iSecureId==KTestSecureId);
	test(self.iVendorId==KTestVendorId);
	for(i=0; i<ECapability_Limit; i++)
		test((TUint32)(self.iCaps.HasCapability((TCapability)i)!=0)==((KTestCapabilities>>i)&1));

	test.Next(_L("Test TSecurityInfo::Set(RProcess)"));
	Mem::FillZ(&info,sizeof(info));
	TInt32 r = Session.Send(CTestSession::ETestProcessSecurityInfo,TIpcArgs(TUint(RProcess().Id()),&infoPtr));
	test(r==KErrNone);
	CheckSecurityInfo(self,info);

	test.Next(_L("Test TSecurityInfo::Set(RThread)"));
	Mem::FillZ(&info,sizeof(info));
	r = Session.Send(CTestSession::ETestThreadSecurityInfo,TIpcArgs(TUint(RThread().Id()),&infoPtr));
	test(r==KErrNone);
	CheckSecurityInfo(self,info);

	test.Next(_L("Test TSecurityInfo::Set(RMessagePtr2)"));
	Mem::FillZ(&info,sizeof(info));
	r = Session.Send(CTestSession::ETestMessageSecurityInfo,TIpcArgs(&infoPtr));
	test(r==KErrNone);
	CheckSecurityInfo(self,info);

	test.Next(_L("Test TSecurityInfo::SetToCreatorInfo"));
	Mem::FillZ(&info,sizeof(info));
	r = Session.Send(CTestSession::ETestCreatorSecurityInfo,TIpcArgs(&infoPtr));
	test(r==KErrNone);
	CheckSecurityInfo(self,info);

	test.End();
	}

void TestSecureId()
	{
	test.Start(_L("Test RProcess::SecureId()"));
	TInt r = Session.Send(CTestSession::ETestProcessSecureId,TIpcArgs(TUint(RProcess().Id())));
	test((TUint32)r==KTestSecureId);

	test.Next(_L("Test RThread::SecureId()"));
	r = Session.Send(CTestSession::ETestThreadSecureId,TIpcArgs(TUint(RThread().Id())));
	test((TUint32)r==KTestSecureId);

	test.Next(_L("Test RMessage2::SecureId()"));
	r = Session.Send(CTestSession::ETestMessageSecureId);
	test((TUint32)r==KTestSecureId);

	test.Next(_L("Test User::CreatorSecureId()"));
	r = Session.Send(CTestSession::ETestCreatorSecureId);
	test((TUint32)r==KTestSecureId);

	test.End();
	}

void TestVendorId()
	{
	test.Start(_L("Test RProcess::VendorId()"));
	TInt r = Session.Send(CTestSession::ETestProcessVendorId,TIpcArgs(TUint(RProcess().Id())));
	test((TUint32)r==KTestVendorId);

	test.Next(_L("Test RThread::VendorId()"));
	r = Session.Send(CTestSession::ETestThreadVendorId,TIpcArgs(TUint(RThread().Id())));
	test((TUint32)r==KTestVendorId);

	test.Next(_L("Test RMessage2::VendorId()"));
	r = Session.Send(CTestSession::ETestMessageVendorId);
	test((TUint32)r==KTestVendorId);

	test.Next(_L("Test User::CreatorVendorId()"));
	r = Session.Send(CTestSession::ETestCreatorVendorId);
	test((TUint32)r==KTestVendorId);

	test.End();
	}

void TestHasCapability()
	{
	TInt failResult=PlatSecEnforcement ? 0 : 1;
	TInt failResultL=PlatSecEnforcement ? KErrPermissionDenied : KErrNone;

	test.Start(_L("Test RProcess::HasCapability(TCapability)"));
	TInt r = Session.Send(CTestSession::ETestProcessHasCapability1,TIpcArgs(TUint(RProcess().Id()),ECapabilityLocalServices));
	test(r);
	r = Session.Send(CTestSession::ETestProcessHasCapability1,TIpcArgs(TUint(RProcess().Id()),ECapabilityNetworkServices));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestProcessHasCapability1,TIpcArgs(TUint(RProcess().Id()),ECapability_None));
	test(r);
	r = Session.Send(CTestSession::ETestProcessHasCapability1,TIpcArgs(TUint(RProcess().Id()),ECapability_Denied));
	test(r==failResult);

	test.Next(_L("Test RProcess::HasCapability(TCapability,TCapability)"));
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapabilityLocalServices,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapabilityNetworkServices,ECapabilityWriteUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapabilityLocalServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapabilityNetworkServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapability_None,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestProcessHasCapability2,TIpcArgs(TUint(RProcess().Id()),ECapability_Denied,ECapabilityWriteUserData));
	test(r==failResult);

	test.Next(_L("Test RThread::HasCapability(TCapability)"));
	r = Session.Send(CTestSession::ETestThreadHasCapability1,TIpcArgs(TUint(RThread().Id()),ECapabilityLocalServices));
	test(r);
	r = Session.Send(CTestSession::ETestThreadHasCapability1,TIpcArgs(TUint(RThread().Id()),ECapabilityNetworkServices));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestThreadHasCapability1,TIpcArgs(TUint(RThread().Id()),ECapability_None));
	test(r);
	r = Session.Send(CTestSession::ETestThreadHasCapability1,TIpcArgs(TUint(RThread().Id()),ECapability_Denied));
	test(r==failResult);

	test.Next(_L("Test RThread::HasCapability(TCapability,TCapability)"));
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapabilityLocalServices,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapabilityNetworkServices,ECapabilityWriteUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapabilityLocalServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapabilityNetworkServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapability_None,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestThreadHasCapability2,TIpcArgs(TUint(RThread().Id()),ECapability_Denied,ECapabilityWriteUserData));
	test(r==failResult);

	test.Next(_L("Test RMessagePtr2::HasCapability(TCapability)"));
	r = Session.Send(CTestSession::ETestMessageHasCapability1,TIpcArgs(ECapabilityLocalServices));
	test(r);
	r = Session.Send(CTestSession::ETestMessageHasCapability1,TIpcArgs(ECapabilityNetworkServices));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestMessageHasCapability1,TIpcArgs(ECapability_None));
	test(r);
	r = Session.Send(CTestSession::ETestMessageHasCapability1,TIpcArgs(ECapability_Denied));
	test(r==failResult);

	test.Next(_L("Test RMessagePtr2::HasCapabilityL(TCapability)"));
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL1,TIpcArgs(ECapabilityLocalServices));
	test(r==KErrNone);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL1,TIpcArgs(ECapabilityNetworkServices));
	test(r==failResultL);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL1,TIpcArgs(ECapability_None));
	test(r==KErrNone);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL1,TIpcArgs(ECapability_Denied));
	test(r==failResultL);

	test.Next(_L("Test RMessagePtr2::HasCapability(TCapability,TCapability)"));
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapabilityLocalServices,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapabilityNetworkServices,ECapabilityWriteUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapabilityLocalServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapabilityNetworkServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapability_None,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestMessageHasCapability2,TIpcArgs(ECapability_Denied,ECapabilityWriteUserData));
	test(r==failResult);

	test.Next(_L("Test RMessagePtr2::HasCapabilityL(TCapability,TCapability)"));
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapabilityLocalServices,ECapabilityWriteUserData));
	test(r==KErrNone);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapabilityNetworkServices,ECapabilityWriteUserData));
	test(r==failResultL);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapabilityLocalServices,ECapabilityReadUserData));
	test(r==failResultL);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapabilityNetworkServices,ECapabilityReadUserData));
	test(r==failResultL);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapability_None,ECapabilityWriteUserData));
	test(r==KErrNone);
	r = Session.Send(CTestSession::ETestMessageHasCapabilityL2,TIpcArgs(ECapability_Denied,ECapabilityWriteUserData));
	test(r==failResultL);

	test.Next(_L("Test User::CreatorHasCapability(TCapability)"));
	r = Session.Send(CTestSession::ETestCreatorHasCapability1,TIpcArgs(ECapabilityLocalServices));
	test(r);
	r = Session.Send(CTestSession::ETestCreatorHasCapability1,TIpcArgs(ECapabilityNetworkServices));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestCreatorHasCapability1,TIpcArgs(ECapability_None));
	test(r);
	r = Session.Send(CTestSession::ETestCreatorHasCapability1,TIpcArgs(ECapability_Denied));
	test(r==failResult);

	test.Next(_L("Test User::CreatorHasCapability(TCapability,TCapability)"));
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapabilityLocalServices,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapabilityNetworkServices,ECapabilityWriteUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapabilityLocalServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapabilityNetworkServices,ECapabilityReadUserData));
	test(r==failResult);
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapability_None,ECapabilityWriteUserData));
	test(r);
	r = Session.Send(CTestSession::ETestCreatorHasCapability2,TIpcArgs(ECapability_Denied,ECapabilityWriteUserData));
	test(r==failResult);

	test.End();
	}

TBool SecurityPoliciesEqual(const TSecurityPolicy& a,const TSecurityPolicy& b)
	{
	return Mem::Compare((TUint8*)&a, sizeof(TSecurityPolicy), (TUint8*)&b, sizeof(TSecurityPolicy))==0;
	}

void TestSecurityPolicy()
	{
	test.Start(_L("Test TSecurityPolicy Constructors"));

	TBool failResult=!PlatSecEnforcement;

	test.Next(_L("Empty Constructor"));
		{
		TSecurityPolicy empty;
		TPtrC8 ptr = empty.Package();
		TSecurityPolicy empty2;
		test(empty2.Set(ptr) == KErrNone);
		test(SecurityPoliciesEqual(empty, empty2));

		test(failResult!=!empty.CheckPolicy(RProcess()));
		test(failResult!=!empty.CheckPolicy(RThread()));
		test(failResult!=!empty2.CheckPolicy(RProcess()));
		test(failResult!=!empty2.CheckPolicy(RThread()));

		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);

		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("Always Fail Constructor"));
		{
		TSecurityPolicy fail(TSecurityPolicy::EAlwaysFail);
		TPtrC8 ptr = fail.Package();
		TSecurityPolicy fail2;
		test(fail2.Set(ptr) == KErrNone);
		test(SecurityPoliciesEqual(fail, fail2));

		test(failResult!=!fail.CheckPolicy(RProcess()));
		test(failResult!=!fail.CheckPolicy(RThread()));
		test(failResult!=!fail2.CheckPolicy(RProcess()));
		test(failResult!=!fail2.CheckPolicy(RThread()));

		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);

		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("Always Pass Constructor"));
		{
		TSecurityPolicy pass(TSecurityPolicy::EAlwaysPass);
		TPtrC8 ptr = pass.Package();
		TSecurityPolicy pass2;
		test(pass2.Set(ptr) == KErrNone);
		test(SecurityPoliciesEqual(pass, pass2));

		test(pass.CheckPolicy(RProcess()));
		test(pass.CheckPolicy(RThread()));
		test(pass2.CheckPolicy(RProcess()));
		test(pass2.CheckPolicy(RThread()));

		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);

		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("3 Capability Constructor"));
		{
		TSecurityPolicy threeCaps(ECapabilityTCB,ECapabilityDRM,ECapabilityProtServ);
		//Current process has these three
		test(threeCaps.CheckPolicy(RProcess()));
		test(threeCaps.CheckPolicy(RThread()));
		TPtrC8 ptr = threeCaps.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy threeCaps(ECapabilityTCB,ECapabilityProtServ,ECapabilityCommDD);
		//Current process doesn't have ECapabilityCommDD
		test(failResult!=!(threeCaps.CheckPolicy(RProcess())));
		test(failResult!=!(threeCaps.CheckPolicy(RThread())));
		TPtrC8 ptr = threeCaps.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy threeCaps(ECapabilityCommDD);
		//Current process doesn't have ECapabilityCommDD
		test(failResult!=!(threeCaps.CheckPolicy(RProcess())));
		test(failResult!=!(threeCaps.CheckPolicy(RThread())));
		TPtrC8 ptr = threeCaps.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy threeCaps(ECapabilityTCB);
		//Current process has TCB + 2 * ECapability_None
		test(threeCaps.CheckPolicy(RProcess()));
		test(threeCaps.CheckPolicy(RThread()));

		TPtrC8 ptr = threeCaps.Package();
		TSecurityPolicy threeCaps2;
		TBuf8<sizeof(TSecurityPolicy)> invalid;
		for(TInt i=4; i<=7; i++)
			{
			invalid=ptr;
			invalid[i] = 0; // Set unused capability to be invalid
			test(threeCaps2.Set(invalid)==KErrArgument);
			}
		test(threeCaps2.Set(ptr)==KErrNone);
		test(SecurityPoliciesEqual(threeCaps, threeCaps2));

		test(threeCaps2.CheckPolicy(RProcess()));
		test(threeCaps2.CheckPolicy(RThread()));

		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);

		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("7 Capability Constructor"));
		{
		TSecurityPolicy sevenCaps(ECapabilityTCB,ECapabilityWriteUserData,ECapabilitySwEvent,ECapabilityProtServ,ECapabilityNetworkControl,ECapabilityDRM,ECapabilityReadDeviceData);
		//Current process has all 7 of these.
		test(sevenCaps.CheckPolicy(RProcess()));
		test(sevenCaps.CheckPolicy(RThread()));
		TPtrC8 ptr = sevenCaps.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}
		
		{
		TSecurityPolicy sevenCaps(ECapabilityTCB,ECapabilityWriteUserData,ECapabilityMultimediaDD,ECapabilityProtServ,ECapabilityNetworkControl,ECapabilityDRM,ECapabilityReadDeviceData);
		//Current process doesn't have MultimediaDD
		test(failResult!=!(sevenCaps.CheckPolicy(RProcess())));
		test(failResult!=!(sevenCaps.CheckPolicy(RThread())));
		TPtrC8 ptr = sevenCaps.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy sevenCaps(ECapabilityTCB,ECapabilityWriteUserData,ECapabilityMultimediaDD,ECapabilityProtServ,ECapabilityTrustedUI,ECapabilityDRM,ECapabilityReadDeviceData);
		//Current process doesn't have MultiMediaDD or TrustedUI
		test(failResult!=!(sevenCaps.CheckPolicy(RProcess())));
		test(failResult!=!(sevenCaps.CheckPolicy(RThread())));

		TPtrC8 ptr = sevenCaps.Package();
		TSecurityPolicy sevenCaps2;
		test(sevenCaps2.Set(ptr)==KErrNone);
		test(SecurityPoliciesEqual(sevenCaps,sevenCaps2));
		test(failResult!=!(sevenCaps2.CheckPolicy(RProcess())));
		test(failResult!=!(sevenCaps2.CheckPolicy(RThread())));
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);;
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);;
		}

	test.Next(_L("SID + 3 constructor"));
		{
		TSecurityPolicy sid(KTestSecureId, ECapabilityProtServ, ECapabilityDRM, ECapabilityReadDeviceData);
		//Current process has all three + sid
		test(sid.CheckPolicy(RProcess()));
		test(sid.CheckPolicy(RThread()));

		TPtrC8 ptr = sid.Package();
		TSecurityPolicy sid2;
		test(sid2.Set(ptr)==KErrNone);
		test(SecurityPoliciesEqual(sid,sid2));
		test(sid2.CheckPolicy(RProcess()));
		test(sid2.CheckPolicy(RThread()));

		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy sid(KTestSecureId2, ECapabilityProtServ, ECapabilityDRM, ECapabilityReadDeviceData);
		//Current process has all three caps but not sid
		test(failResult!=!(sid.CheckPolicy(RProcess())));
		test(failResult!=!(sid.CheckPolicy(RThread())));
		TPtrC8 ptr = sid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy sid(KTestSecureId, ECapabilityProtServ, ECapabilityWriteDeviceData);
		//Current process has sid but missing ECapabilityWriteDeviceData
		test(failResult!=!(sid.CheckPolicy(RProcess())));
		test(failResult!=!(sid.CheckPolicy(RThread())));
		TPtrC8 ptr = sid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy sid(KTestSecureId2, ECapabilityProtServ, ECapabilityWriteDeviceData);
		//Current process is missing sid and ECapabilityWriteDeviceData
		test(failResult!=!(sid.CheckPolicy(RProcess())));
		test(failResult!=!(sid.CheckPolicy(RThread())));
		TPtrC8 ptr = sid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("VID + 3 constructor"));
		{
		TSecurityPolicy vid(KTestVendorId, ECapabilityProtServ, ECapabilityDRM, ECapabilityReadDeviceData);
		//Current process has all three + vid
		test(vid.CheckPolicy(RProcess()));
		test(vid.CheckPolicy(RThread()));

		TPtrC8 ptr = vid.Package();
		TSecurityPolicy vid2;
		test(vid2.Set(ptr)==KErrNone);
		test(SecurityPoliciesEqual(vid,vid2));
		test(vid2.CheckPolicy(RProcess()));
		test(vid2.CheckPolicy(RThread()));
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy vid(KTestVendorId2, ECapabilityProtServ, ECapabilityDRM, ECapabilityReadDeviceData);
		//Current process has all three caps but not vid
		test(failResult!=!(vid.CheckPolicy(RProcess())));
		test(failResult!=!(vid.CheckPolicy(RThread())));
		TPtrC8 ptr = vid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy vid(KTestVendorId, ECapabilityProtServ, ECapabilityWriteDeviceData);
		//Current process has vid but missing ECapabilityWriteDeviceData
		test(failResult!=!(vid.CheckPolicy(RProcess())));
		test(failResult!=!(vid.CheckPolicy(RThread())));
		TPtrC8 ptr = vid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

		{
		TSecurityPolicy vid(KTestVendorId2, ECapabilityProtServ, ECapabilityWriteDeviceData);
		//Current process is missing vid and ECapabilityWriteDeviceData
		test(failResult!=!(vid.CheckPolicy(RProcess())));
		test(failResult!=!(vid.CheckPolicy(RThread())));
		TPtrC8 ptr = vid.Package();
		TInt r = Session.Send(CTestSession::ETestSecurityPolicyAgainstMessage,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		r = Session.Send(CTestSession::ETestSecurityPolicyAgainstCreator,TIpcArgs(&ptr));
		test(r==failResult?CTestSession::EPolicyCheckFailed:CTestSession::EPolicyCheckPassed);
		}

	test.Next(_L("Macros for compile-time construction"));
		{
		static _LIT_SECURITY_POLICY_C7(pc7,1,2,3,4,5,6,7);
		static _LIT_SECURITY_POLICY_C6(pc6,1,2,3,4,5,6);
		static _LIT_SECURITY_POLICY_C5(pc5,1,2,3,4,5);
		static _LIT_SECURITY_POLICY_C4(pc4,1,2,3,4);
		static _LIT_SECURITY_POLICY_C3(pc3,1,2,3);
		static _LIT_SECURITY_POLICY_C2(pc2,1,2);
		static _LIT_SECURITY_POLICY_C1(pc1,1);
		test(SecurityPoliciesEqual(pc7,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5,(TCapability)6,(TCapability)7)));
		test(SecurityPoliciesEqual(pc6,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5,(TCapability)6)));
		test(SecurityPoliciesEqual(pc5,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5)));
		test(SecurityPoliciesEqual(pc4,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4)));
		test(SecurityPoliciesEqual(pc3,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(pc2,TSecurityPolicy((TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(pc1,TSecurityPolicy((TCapability)1)));
		
		static _LIT_SECURITY_POLICY_S3(ps3,0x12345678,1,2,3);
		static _LIT_SECURITY_POLICY_S2(ps2,0x12345678,1,2);
		static _LIT_SECURITY_POLICY_S1(ps1,0x12345678,1);
		static _LIT_SECURITY_POLICY_S0(ps0,0x12345678);
		test(SecurityPoliciesEqual(ps3,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(ps2,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(ps1,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1)));
		test(SecurityPoliciesEqual(ps0,TSecurityPolicy(TSecureId(0x12345678))));

		static _LIT_SECURITY_POLICY_V3(pv3,0x12345678,1,2,3);
		static _LIT_SECURITY_POLICY_V2(pv2,0x12345678,1,2);
		static _LIT_SECURITY_POLICY_V1(pv1,0x12345678,1);
		static _LIT_SECURITY_POLICY_V0(pv0,0x12345678);
		test(SecurityPoliciesEqual(pv3,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(pv2,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(pv1,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1)));
		test(SecurityPoliciesEqual(pv0,TSecurityPolicy(TVendorId(0x12345678))));

		static _LIT_SECURITY_POLICY_FAIL(fail);
		static _LIT_SECURITY_POLICY_PASS(pass);
		test(SecurityPoliciesEqual(fail,TSecurityPolicy(TSecurityPolicy::EAlwaysFail)));
		test(SecurityPoliciesEqual(pass,TSecurityPolicy(TSecurityPolicy::EAlwaysPass)));
		}

	test.Next(_L("Macros for compile-time initialisation"));
		{
		const TStaticSecurityPolicy pc7 = _INIT_SECURITY_POLICY_C7(1,2,3,4,5,6,7);
		const TStaticSecurityPolicy pc6 = _INIT_SECURITY_POLICY_C6(1,2,3,4,5,6);
		const TStaticSecurityPolicy pc5 = _INIT_SECURITY_POLICY_C5(1,2,3,4,5);
		const TStaticSecurityPolicy pc4 = _INIT_SECURITY_POLICY_C4(1,2,3,4);
		const TStaticSecurityPolicy pc3 = _INIT_SECURITY_POLICY_C3(1,2,3);
		const TStaticSecurityPolicy pc2 = _INIT_SECURITY_POLICY_C2(1,2);
		const TStaticSecurityPolicy pc1 = _INIT_SECURITY_POLICY_C1(1);
		test(SecurityPoliciesEqual(pc7,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5,(TCapability)6,(TCapability)7)));
		test(SecurityPoliciesEqual(pc6,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5,(TCapability)6)));
		test(SecurityPoliciesEqual(pc5,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4,(TCapability)5)));
		test(SecurityPoliciesEqual(pc4,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3,(TCapability)4)));
		test(SecurityPoliciesEqual(pc3,TSecurityPolicy((TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(pc2,TSecurityPolicy((TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(pc1,TSecurityPolicy((TCapability)1)));
		
		const TStaticSecurityPolicy ps3 = _INIT_SECURITY_POLICY_S3(0x12345678,1,2,3);
		const TStaticSecurityPolicy ps2 = _INIT_SECURITY_POLICY_S2(0x12345678,1,2);
		const TStaticSecurityPolicy ps1 = _INIT_SECURITY_POLICY_S1(0x12345678,1);
		const TStaticSecurityPolicy ps0 = _INIT_SECURITY_POLICY_S0(0x12345678);
		test(SecurityPoliciesEqual(ps3,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(ps2,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(ps1,TSecurityPolicy(TSecureId(0x12345678),(TCapability)1)));
		test(SecurityPoliciesEqual(ps0,TSecurityPolicy(TSecureId(0x12345678))));

		const TStaticSecurityPolicy pv3 = _INIT_SECURITY_POLICY_V3(0x12345678,1,2,3);
		const TStaticSecurityPolicy pv2 = _INIT_SECURITY_POLICY_V2(0x12345678,1,2);
		const TStaticSecurityPolicy pv1 = _INIT_SECURITY_POLICY_V1(0x12345678,1);
		const TStaticSecurityPolicy pv0 = _INIT_SECURITY_POLICY_V0(0x12345678);
		test(SecurityPoliciesEqual(pv3,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1,(TCapability)2,(TCapability)3)));
		test(SecurityPoliciesEqual(pv2,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1,(TCapability)2)));
		test(SecurityPoliciesEqual(pv1,TSecurityPolicy(TVendorId(0x12345678),(TCapability)1)));
		test(SecurityPoliciesEqual(pv0,TSecurityPolicy(TVendorId(0x12345678))));

		const TStaticSecurityPolicy fail = _INIT_SECURITY_POLICY_FAIL;
		const TStaticSecurityPolicy pass = _INIT_SECURITY_POLICY_PASS;
		test(SecurityPoliciesEqual(fail,TSecurityPolicy(TSecurityPolicy::EAlwaysFail)));
		test(SecurityPoliciesEqual(pass,TSecurityPolicy(TSecurityPolicy::EAlwaysPass)));

		}

	test.End();
	}

#define CHECK_NAME(name)	\
	test(0==TPtrC8((TUint8*)#name).Compare(TPtrC8((TUint8*)CapabilityNames[ECapability##name])));

void TestCapabilityNames()
	{
	CHECK_NAME(TCB);
	CHECK_NAME(CommDD);
	CHECK_NAME(PowerMgmt);
	CHECK_NAME(MultimediaDD);
	CHECK_NAME(ReadDeviceData);
	CHECK_NAME(WriteDeviceData);
	CHECK_NAME(DRM);
	CHECK_NAME(TrustedUI);
	CHECK_NAME(ProtServ);
	CHECK_NAME(DiskAdmin);
	CHECK_NAME(NetworkControl);
	CHECK_NAME(AllFiles);
	CHECK_NAME(SwEvent);
	CHECK_NAME(NetworkServices);
	CHECK_NAME(LocalServices);
	CHECK_NAME(ReadUserData);
	CHECK_NAME(WriteUserData);
	CHECK_NAME(Location);
	CHECK_NAME(SurroundingsDD);
	CHECK_NAME(UserEnvironment);
	}


void TestKernelAPIs()
	{
	RLddTest ldd;
	TInt r=User::LoadLogicalDevice(_L("D_SLDD.LDD"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=ldd.OpenLocal();
	test(r==KErrNone);

	RLddTest::TIds ids;
	memclr(&ids,sizeof(ids));
	ldd.GetIds(ids);
	test.Printf(_L("Thread VID,SID = %08x,%08x\n\r"),ids.iThreadVID.iId,ids.iThreadSID.iId);
	test.Printf(_L("Process VID,SID = %08x,%08x\n\r"),ids.iProcessVID.iId,ids.iProcessSID.iId);
	test(ids.iThreadVID==KTestVendorId);
	test(ids.iThreadSID==KTestSecureId);
	test(ids.iProcessVID==KTestVendorId);
	test(ids.iProcessSID==KTestSecureId);

	// Test kernel-mode TSecurityInfo-getting APIs
	TSecurityInfo infoProcess, infoThread;
	ldd.GetSecureInfos(&infoThread, &infoProcess);
	// Check the vendor & secure IDs are what's expected
	test(infoThread.iVendorId==KTestVendorId);
	test(infoThread.iSecureId==KTestSecureId);
	test(infoProcess.iVendorId==KTestVendorId);
	test(infoProcess.iSecureId==KTestSecureId);
	// Check process caps == thread caps
	TUint32* capsT = (TUint32*)&infoThread.iCaps;
	TUint32* capsP = (TUint32*)&infoProcess.iCaps;
	test(capsT[0]==capsP[0]);
	test(capsT[1]==capsP[1]);
	// Check the caps match what the user API gives
	RProcess this_process;
	for (TInt i=0 ; i<64 ; i++) {
		TCapability cap = (TCapability)i;
		test(infoProcess.iCaps.HasCapability(cap) == this_process.HasCapability(cap));
	}


	ldd.Close();
	}


void TestPlatSecUnlocked()
	{
	RLddTest ldd;
	TInt r = User::LoadLogicalDevice(_L("D_SLDD.LDD"));
	test(r == KErrNone || r == KErrAlreadyExists);
	r = ldd.OpenLocal();
	test_KErrNone(r);

	TUint32 flags0, flags;
	TInt enforced;

	flags0 = ldd.GetKernelConfigFlags();

	const TUint32 bits = (TUint32)(EKernelConfigTest | EKernelConfigPlatSecEnforcement);

	// Different test cases depending on whether __PLATSEC_UNLOCKED__ defined.
	// Ask the kernel whether EKernelConfigPlatSecLocked is set, and hope that
	// it's not lying to us!
	//
	// Best thing to do is to check the log and verify the printf() output.
	//

	if (PlatSec::ConfigSetting(PlatSec::EPlatSecLocked))
		{
		/*
		 * Tests for __PLATSEC_UNLOCKED__ not defined
		 *
		 * The device driver is built with __PLATSEC_FORCED_FLAGS__ set to 0, so we can't use
		 * its GetKernelConfigFlags() interface (the accessor functions are inlines) when we're
		 * testing for PlatSec locked.  Instead, use PlatSec::ConfigSetting(), which makes an
		 * exec call to get iKernelConfigFlags.
		 */
		test.Printf(_L("__PLATSEC_UNLOCKED_ does NOT appear to have been used\n"));

		/* Check that PlatSecEnforcement is set */
		enforced = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
		test(enforced != 0);

		/* Check that the device driver is able to set our test bit */
		ldd.SetKernelConfigFlags(flags0 | EKernelConfigTest);

		flags = ldd.GetKernelConfigFlags();
		test((flags & EKernelConfigTest) == EKernelConfigTest);

		/* Check that the device driver is able to clear our test bit, but not able to clear enforcement bit */
		ldd.SetKernelConfigFlags(flags0 & ~bits);

		enforced = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
		test(enforced != 0);
		}
	else
		{
		/*
		 * Tests for __PLATSEC_UNLOCKED__ defined
		 */
		test.Printf(_L("__PLATSEC_UNLOCKED_ DOES appear to have been used\n"));

		/* Check that the device driver is able to set our test bit and PlatSecEnforcement */
		ldd.SetKernelConfigFlags(flags0 | bits);

		flags = ldd.GetKernelConfigFlags();
		test((flags & bits) == bits);

		/* And verify that the kernel sees the same result */
		enforced = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
		test(enforced != 0);			// (yes, "!= 0" is redundant)

		/* Check that the device driver is able to clear our test bit and PlatSecEnforcement */
		ldd.SetKernelConfigFlags(flags0 & ~bits);

		flags = ldd.GetKernelConfigFlags();
		test((flags & bits) == 0);

		/* Verify that the kernel sees the same result */
		enforced = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);
		test(enforced == 0);
		}

	/* Restore original flags value */
	ldd.SetKernelConfigFlags(flags0);

	// Now test handling of iDisabledCapabilities

	const TUint32 test_value = 0x31415926;						// Just some random number

	TCapabilitySet disabled0, disabled;
	disabled0.SetDisabled();

	SCapabilitySet *caps0 = (SCapabilitySet *)&disabled0;
	SCapabilitySet *caps = (SCapabilitySet *)&disabled;

	if (PlatSec::ConfigSetting(PlatSec::EPlatSecLocked))
		{
		/*
		 * Tests for __PLATSEC_UNLOCKED__ not defined
		 */

		/* Check that the capability set is 0 */
		int i;

		for (i = 0; i < SCapabilitySet::ENCapW; i++)
			{
			test_Equal(0, caps0->iCaps[i]);
			}

		/* Check that the device driver is not able to set word 0 of disabled capabilities */
		ldd.SetDisabledCapabilities0(test_value);

		/*
		 * It's okay to use SetDisabled() here, since this makes an exec call to get the
		 * set of disabled capabilities.
		 */
		disabled.SetDisabled();
		test_Equal(0, caps->iCaps[0]);
		}
	else
		{
		/*
		 * Tests for __PLATSEC_UNLOCKED__ defined
		 */

		/* Check that the device driver is able to set word 0 of disabled capabilities */
		ldd.SetDisabledCapabilities0(test_value);

		disabled.SetDisabled();
		test_Equal(test_value, caps->iCaps[0]);
		}

	/* Restore original value */
	ldd.SetDisabledCapabilities0(caps0->iCaps[0]);

	ldd.Close();
	}

#include <e32svr.h>

GLDEF_C TInt E32Main()
    {
	PlatSecEnforcement = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement);

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

	if(PlatSecEnforcement)
		test.Printf(_L("PlatSecConfig appears to be ON\n"));
	else
		test.Printf(_L("PlatSecConfig appears to be OFF\n"));

	test.Start(_L("Starting test server"));
	RTestProcess server;
	TRequestStatus rendezvous;
	server.Create(~KTestCapabilities,ETestProcessServer,0,0);
	server.Rendezvous(rendezvous);
	server.Resume();
	User::WaitForRequest(rendezvous);
	test(rendezvous==KServerRendezvous);

	test.Next(_L("Openning server session"));
	TInt r = Session.Connect();
	RDebug::Print(_L("%d"),r);
	test(r==KErrNone);

	test.Next(_L("Test Capability Names"));
	TestCapabilityNames();

	test.Next(_L("Test TCapabilitySet"));
	TestCapabilitySet();

	test.Next(_L("Test TSecurityInfo"));
	TestSecurityInfo();

	test.Next(_L("Test SecureId functions"));
	TestSecureId();

	test.Next(_L("Test VendorId functions"));
	TestVendorId();

	test.Next(_L("Test HasCapability functions"));
	TestHasCapability();

	test.Next(_L("Test TSecurityPolicy"));
	TestSecurityPolicy();

	test.Next(_L("Test Kernel APIs"));
	TestKernelAPIs();

	test.Next(_L("Test __PLATSEC_UNLOCKED__"));
	TestPlatSecUnlocked();

	test.Next(_L("Test diagnostic message suppression"));
	RThread().HasCapability(ECapabilityReadUserData,0);
	test.Printf(_L("There should be a diagnostic message just before this\n"));
	RThread().HasCapability(ECapabilityReadUserData,__PLATSEC_DIAGNOSTIC_STRING("You should see this"));
	test.Printf(_L("There should be a diagnostic message just before this\n"));
	RThread().HasCapability(ECapabilityReadUserData,KSuppressPlatSecDiagnostic);
	test.Printf(_L("There should NOT be a diagnostic message just before this\n"));

	test.Next(_L("Closing server session"));
	Session.Send(CTestSession::EShutdown);
	Session.Close();
	server.Close();

	test.End();
	return(0);
    }

