// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\realtime\t_lat2.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32property.h>
#include <e32atomics.h>
#include "runtests.h"
#include "d_latncy.h"

_LIT(KLatencyLddFileName,"D_LATNCY");
_LIT(KThreadName,"LatencyThreadU");

RTest test(_L("Latency"));
RThread Main;
TUint TicksPerMs;

struct SFullLatencyResults : public SLatencyResults
	{
	TUint iKernRetAddr;
	TUint iUserRetAddr;
	TInt64 iCount;
	TInt64 iSumIntTicks;
	TInt64 iSumKernTicks;
	TInt64 iSumUserTicks;
	TUint iIntCpsr;
	TUint iKernCpsr;
	TUint iKernR14;
	TUint iUserCpsr;
	TUint iUserR14;

	void Update(SLatencyResults& aResults);
	};

SFullLatencyResults Latencies;
volatile TUint32 UpdateCount=0;

TUint TimerToMicroseconds(TUint aTimerValue)
	{
	return (aTimerValue*1000+TicksPerMs-1)/TicksPerMs;
	}

void SFullLatencyResults::Update(SLatencyResults& aResults)
	{
	__e32_atomic_add_acq32(&UpdateCount, 1);

	// memory barrier

	if (aResults.iIntTicks>iIntTicks)
		{
		iIntTicks=aResults.iIntTicks;
		iIntRetAddr=aResults.iIntRetAddr;
#ifdef __CAPTURE_EXTRAS
		iIntCpsr=aResults.iIntSpsr;
		iIntR14=aResults.iIntR14;
#endif
		}
	if (aResults.iKernThreadTicks>iKernThreadTicks)
		{
		iKernThreadTicks=aResults.iKernThreadTicks;
		iKernRetAddr=aResults.iIntRetAddr;
#ifdef __CAPTURE_EXTRAS
		iKernCpsr=aResults.iIntSpsr;
		iKernR14=aResults.iIntR14;
#endif
		}
	if (aResults.iUserThreadTicks>iUserThreadTicks)
		{
		iUserThreadTicks=aResults.iUserThreadTicks;
		iUserRetAddr=aResults.iIntRetAddr;
#ifdef __CAPTURE_EXTRAS
		iUserCpsr=aResults.iIntSpsr;
		iUserR14=aResults.iIntR14;
#endif
		}
	iSumIntTicks+=aResults.iIntTicks;
	iSumKernTicks+=aResults.iKernThreadTicks;
	iSumUserTicks+=aResults.iUserThreadTicks;
	++iCount;

	// memory barrier

	__e32_atomic_add_rel32(&UpdateCount, 1);
	}

TInt LatencyThread(TAny* aStatus)
	{
	TRequestStatus* pS=(TRequestStatus*)aStatus;
	RLatency l;
	TInt r=l.Open();
	if (r!=KErrNone)
		return r;
	TicksPerMs=l.TicksPerMs();
	Mem::FillZ(&Latencies,sizeof(Latencies));
	Main.RequestComplete(pS,0);
	SLatencyResults results;

	l.Start();
	volatile TInt forever = 1;
	while(forever)
		{
		User::WaitForAnyRequest();
		l.GetResults(results);
		Latencies.Update(results);
		}
	return 0;
	}

void GetLatencies(SFullLatencyResults& aResults)
	{
	FOREVER
		{
		TUint32 u1 = UpdateCount;
		__e32_memory_barrier();
		aResults=Latencies;
		__e32_memory_barrier();
		TUint32 u2 = UpdateCount;
		if (u1==u2 && !(u1&1))	// no good if it changed partway through or was changing when we started
			break;
		}
	}

_LIT(KPrefixRuntests, "RUNTESTS: RT");
void DisplayMaxValues(const TDesC& aPrefix)
	{
	SFullLatencyResults v;
	GetLatencies(v);
	TUint i=TimerToMicroseconds(v.iIntTicks);
	TUint k=TimerToMicroseconds(v.iKernThreadTicks);
	TUint u=TimerToMicroseconds(v.iUserThreadTicks);
	TUint ia=v.iIntRetAddr;
	TUint ka=v.iKernRetAddr;
	TUint ua=v.iUserRetAddr;
	test.Printf(_L("%SMAX: Int %4d %08x Kern %4d %08x User %4d %08x\n"),&aPrefix,i,ia,k,ka,u,ua);
	}

void DisplayAvgValues(const TDesC& aPrefix)
	{
	SFullLatencyResults v;
	GetLatencies(v);
	TUint i=TimerToMicroseconds(I64LOW(v.iSumIntTicks/v.iCount));
	TUint k=TimerToMicroseconds(I64LOW(v.iSumKernTicks/v.iCount));
	TUint u=TimerToMicroseconds(I64LOW(v.iSumUserTicks/v.iCount));
	test.Printf(_L("%SAVG: Int %4d Kern %4d User %4d Count %Ld\n"),&aPrefix,i,k,u,v.iCount);
	}

#ifdef __CAPTURE_EXTRAS
void DisplayExtras(const TDesC& aPrefix)
	{
	SFullLatencyResults v;
	GetLatencies(v);
	test.Printf(_L("%SInt : Cpsr %08x R14 %08x\n"),&aPrefix,v.iIntCpsr,v.iIntR14);
	test.Printf(_L("%SKern: Cpsr %08x R14 %08x\n"),&aPrefix,v.iKernCpsr,v.iKernR14);
	test.Printf(_L("%SUser: Cpsr %08x R14 %08x\n"),&aPrefix,v.iUserCpsr,v.iUserR14);
	}
#endif

void ClearMaxValues()
	{
	Mem::FillZ(&Latencies,6*sizeof(TUint));
	}

void ClearAvgValues()
	{
	Mem::FillZ(&Latencies.iCount,4*sizeof(TInt64));
	}

_LIT_SECURITY_POLICY_PASS(KPersistencePropReadPolicy);
_LIT_SECURITY_POLICY_PASS(KPersistencePropWritePolicy);
void AnnouncePersistence()
	{
	TInt r = RProperty::Define(KRuntestsIntentionalPersistenceKey, RProperty::EInt, KPersistencePropReadPolicy, KPersistencePropWritePolicy);
	test(r==KErrNone || r==KErrAlreadyExists);
	r = RProperty::Set(RProcess().SecureId(), KRuntestsIntentionalPersistenceKey, KRuntestsIntentionalPersistenceValue);
	test(r==KErrNone);
	}

class CConsoleReader : public CActive
	{
public:
	CConsoleReader();
	static void New();
	void Start();
	virtual void RunL();
	virtual void DoCancel();
public:
	CConsoleBase* iConsole;
	};

CConsoleReader::CConsoleReader()
	:	CActive(0)
	{
	}

void CConsoleReader::RunL()
	{
	TKeyCode k = iConsole->KeyCode();
	switch(k)
		{
		case '1':
			test.Printf(_L("Clearing Maximum Values\n"));
			ClearMaxValues();
			break;
		case '2':
			DisplayMaxValues(KNullDesC);
			break;
		case '3':
			test.Printf(_L("Clearing Average Values\n"));
			ClearAvgValues();
			break;
		case '4':
			DisplayAvgValues(KNullDesC);
			break;
#ifdef __CAPTURE_EXTRAS
		case '5':
			DisplayExtras(KNullDesC);
			break;
#endif
		case 'x':
		case 'X':
			CActiveScheduler::Stop();
			return;
		default:
			break;
		}
	Start();
	}

void CConsoleReader::DoCancel()
	{
	iConsole->ReadCancel();
	}

void CConsoleReader::New()
	{
	CConsoleReader* crdr = new CConsoleReader;
	test(crdr != NULL);
	crdr->iConsole = test.Console();
	CActiveScheduler::Add(crdr);
	crdr->Start();
	}

void CConsoleReader::Start()
	{
	iConsole->Read(iStatus);
	SetActive();
	}

class CPubSubWatcher : public CActive
	{
public:
	CPubSubWatcher();
	static void New();
	void Start();
	virtual ~CPubSubWatcher();
	virtual void RunL();
	virtual void DoCancel();
public:
	RProperty iProperty;
	};

CPubSubWatcher::CPubSubWatcher()
	:	CActive(0)
	{
	}

void CPubSubWatcher::RunL()
	{
	Start();
	DisplayMaxValues(KPrefixRuntests);
	DisplayAvgValues(KPrefixRuntests);
	}

void CPubSubWatcher::DoCancel()
	{
	iProperty.Cancel();
	}

void CPubSubWatcher::New()
	{
	CPubSubWatcher* psw = new CPubSubWatcher;
	test(psw != NULL);
	TInt r = psw->iProperty.Attach(KRuntestsCategory, KRuntestsCurrentTestKey, EOwnerThread);
	test(r==KErrNone);
	CActiveScheduler::Add(psw);
	psw->Start();
	}

void CPubSubWatcher::Start()
	{
	iProperty.Subscribe(iStatus);
	SetActive();
	}

CPubSubWatcher::~CPubSubWatcher()
	{
	iProperty.Close();
	}

GLDEF_C TInt E32Main()
	{
#ifdef _DEBUG
	// Don't run automatically on debug builds
	TUint32 creator_sid = User::CreatorSecureId();
	if (creator_sid == TUint32(KRuntestsCategoryValue))
		return KErrNone;
#endif
	// disable anything which will interfere, e.g. plat sec diagnostics
	User::SetDebugMask(UserSvr::DebugMask(2)|4, 2);

	test.Title();
	
	test.Printf(_L("*** Please note ***\n"));
	test.Printf(_L("\n"));
	test.Printf(_L("t_lat2 runs in the backgroud to measure latency while other tests are\n"));
	test.Printf(_L("running.  It should not be run as a standalone test, only as part of a\n"));
	test.Printf(_L("test run coordinated by runtests.  If run on its owm, it will simply wait\n"));
	test.Printf(_L("forever.\n"));
	test.Printf(_L("\n"));
	
	test.Start(_L("Load LDD"));
	TInt r=User::LoadLogicalDevice(KLatencyLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Duplicate handle"));
	r=Main.Duplicate(RThread());
	test(r==KErrNone);

	test.Next(_L("Create thread"));
	RThread t;
	TRequestStatus sx;
	TRequestStatus sc;
	r=t.Create(KThreadName,LatencyThread,0x1000,NULL,&sc);
	test(r==KErrNone);
	t.Logon(sx);
	t.Resume();
	User::WaitForRequest(sx,sc);
	if (sx!=KRequestPending)
		{
		if (t.ExitType()==EExitKill && t.ExitReason()==KErrAlreadyExists)
			{
			test.Printf(_L("T_LAT2 already running.\n"));
			test.End();
			return 0;
			}
		test.Printf(_L("Initialisation failed, error %d\n"),sx.Int());
		test(0);
		}
	test(sc==KErrNone);

	CTrapCleanup* tcln = CTrapCleanup::New();
	test(tcln != NULL);
	CActiveScheduler* as = new CActiveScheduler;
	test(as != NULL);
	CActiveScheduler::Install(as);
	CConsoleReader::New();
	CPubSubWatcher::New();
	AnnouncePersistence();
	RProcess::Rendezvous(KErrNone);

	CActiveScheduler::Start();

	// latency test over
	User::SetDebugMask(UserSvr::DebugMask(2)&~4, 2);

	test.End();
	return 0;
	}
