// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\cpumeter.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32hal.h>
#include <e32svr.h>
#include "u32std.h"

RTest test(_L("CPU METER"));

TBool CpuTimeSupported()
	{
	TTimeIntervalMicroSeconds time;
	TInt err = RThread().GetCpuTime(time);
	test(err == KErrNone || err == KErrNotSupported);
	return err == KErrNone;
	}

TInt NumberOfCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test(r>0);
	return r;
	}

class CCpuMeter : public CBase
	{
public:
	CCpuMeter();
	~CCpuMeter();
	static CCpuMeter* New();
	TInt Construct();
	void Measure();
	void Display(TInt aInterval);
	void DisplayCoreControlInfo();
	void ChangeNumberOfCores(TInt aNum);
public:
	TInt iNumCpus;
	TInt iNextMeas;
	RThread* iNullThreads;
	TTimeIntervalMicroSeconds* iMeas[2];
	TInt* iDelta;
	};

CCpuMeter::CCpuMeter()
	{
	}

CCpuMeter::~CCpuMeter()
	{
	TInt i;
	if (iNullThreads)
		{
		for (i=0; i<iNumCpus; ++i)
			iNullThreads[i].Close();
		User::Free(iNullThreads);
		}
	User::Free(iMeas[0]);
	User::Free(iMeas[1]);
	User::Free(iDelta);
	}

TInt CCpuMeter::Construct()
	{
	iNumCpus = NumberOfCpus();
	iNullThreads = (RThread*)User::AllocZ(iNumCpus*sizeof(RThread));
	iDelta = (TInt*)User::AllocZ(iNumCpus*sizeof(TInt));
	iMeas[0] = (TTimeIntervalMicroSeconds*)User::AllocZ(iNumCpus*sizeof(TTimeIntervalMicroSeconds));
	iMeas[1] = (TTimeIntervalMicroSeconds*)User::AllocZ(iNumCpus*sizeof(TTimeIntervalMicroSeconds));
	if (!iNullThreads || !iDelta || !iMeas[0] || !iMeas[1])
		return KErrNoMemory;
	TFullName kname;
	_LIT(KLitKernelName, "ekern.exe*");
	_LIT(KLitNull, "::Null");
	TFindProcess fp(KLitKernelName);
	test_KErrNone(fp.Next(kname));
	test.Printf(_L("Found kernel process: %S\n"), &kname);
	kname.Append(KLitNull);
	TInt i;
	for (i=0; i<iNumCpus; ++i)
		{
		TFullName tname(kname);
		TFullName tname2;
		if (i>0)
			tname.AppendNum(i);
		TFindThread ft(tname);
		test_KErrNone(ft.Next(tname2));
		TInt r = iNullThreads[i].Open(ft);
		test_KErrNone(r);
		iNullThreads[i].FullName(tname2);
		test.Printf(_L("Found and opened %S\n"), &tname2);
		}
	for (i=0; i<iNumCpus; ++i)
		iNullThreads[i].GetCpuTime(iMeas[0][i]);
	iNextMeas = 1;
	return KErrNone;
	}

CCpuMeter* CCpuMeter::New()
	{
	CCpuMeter* p = new CCpuMeter;
	if (!p)
		return 0;
	TInt r = p->Construct();
	if (r!=KErrNone)
		{
		delete p;
		return 0;
		}
	return p;
	}

void CCpuMeter::Measure()
	{
	TInt i;
	for (i=0; i<iNumCpus; ++i)
		iNullThreads[i].GetCpuTime(iMeas[iNextMeas][i]);
	TInt prev = 1 - iNextMeas;
	for (i=0; i<iNumCpus; ++i)
		iDelta[i] = TInt(iMeas[iNextMeas][i].Int64() - iMeas[prev][i].Int64());
	iNextMeas = prev;
	}

void CCpuMeter::Display(TInt aInterval)
	{
	TBuf<80> buf;
	TInt i;
	for (i=0; i<iNumCpus; ++i)
		{
		TInt dv = (1000*(aInterval - iDelta[i]))/aInterval;
		if (dv<0)
			dv=0;
		if (dv>1000)
			dv=1000;
		buf.AppendFormat(_L(" %4d"),dv);
		}
	buf.Append(TChar('\n'));
	test.Printf(buf);
	}

void CCpuMeter::DisplayCoreControlInfo()
	{
	SCpuStates s;
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalCpuStates, &s, 0);
	if (r != KErrNone)
		{
		test.Printf(_L("Error %d\n"), r);
		return;
		}
	test.SetLogged(ETrue);
	test.Printf(_L("  TA=%08x IA=%08x CU=%08x GD=%08x DC=%08x\n"), s.iTA, s.iIA, s.iCU, s.iGD, s.iDC);
	test.Printf(_L("  SC=%08x RC=%08x PO=%02x      CCS=%08x PODC=%08x\n"), s.iSC, s.iRC, s.iPO, s.iCCS, s.iPODC);
	TInt i;
	for (i=0; i<iNumCpus; ++i)
		{
		test.Printf(_L("%1d:DS=%08x UDC=%08x UAC=%08x OP=%08x F=%08x\n"), i, s.iDS[i], s.iUDC[i], s.iUAC[i], s.iOP[i], s.iF[i]);
		}
	test.SetLogged(EFalse);
	}

void CCpuMeter::ChangeNumberOfCores(TInt aNum)
	{
	test.SetLogged(ETrue);
	test.Printf(_L("#CORES->%d\n"), aNum);
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSetNumberOfCpus, (TAny*)aNum, 0);
	if (r != KErrNone)
		{
		test.Printf(_L("Error %d\n"), r);
		}
	test.SetLogged(EFalse);
	}

void UseKernelCpuTime()
	{
	test.Start(_L("Create CCpuMeter"));
	CCpuMeter* m = CCpuMeter::New();
	test_NotNull(m);
	TInt iv = 1000500;	// on average 1000.5 ms
	TRequestStatus s;
	CConsoleBase* console = test.Console();
	console->Read(s);
	FOREVER
		{
		User::AfterHighRes(1000000);
		m->Measure();
		m->Display(iv);
		while (s!=KRequestPending)
			{
			User::WaitForRequest(s);
			TKeyCode k = console->KeyCode();
			if (k == EKeyEscape)
				{
				delete m;
				return;
				}
			if (m->iNumCpus > 1)
				{
				// SMP only options
				if (k == EKeySpace)
					m->DisplayCoreControlInfo();
				else if (k>='1' && k<=('0'+m->iNumCpus))
					m->ChangeNumberOfCores(k - '0');
				}
			console->Read(s);
			}
		}
	}



TUint32 NopCount=0;
TUint MaxCycles;
_LIT(KLitThreadName,"IdleThread");
extern TInt CountNops(TAny*);

void MeasureByNOPs()
	{
	test.Start(_L("Create thread"));
	RThread t;
	TInt r=t.Create(KLitThreadName,CountNops,0x1000,NULL,NULL);
	test(r==KErrNone);
	t.SetPriority(EPriorityAbsoluteVeryLow);
	t.Resume();

	test.Next(_L("Get processor clock frequency"));
	TMachineInfoV2Buf buf;
	TMachineInfoV2& info=buf();
	r=UserHal::MachineInfo(buf);
	test(r==KErrNone);
	MaxCycles=info.iProcessorClockInKHz*1000;
	test.Printf(_L("Clock frequency %dHz\n"),MaxCycles);
	TRequestStatus s;
	CConsoleBase* console=test.Console();
	console->Read(s);
#ifdef __WINS__
	TInt timerperiod = 5;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalIntProperty,(TAny*)"TimerResolution",&timerperiod);
#endif

	FOREVER
		{
		TUint32 init_count=NopCount;
		TUint32 init_ms=User::NTickCount();
		User::After(1000000);
		TUint32 final_count=NopCount;
		TUint32 final_ms=User::NTickCount();
		TUint32 cycles=final_count-init_count;
		TUint32 ms=final_ms-init_ms;
#ifdef __WINS__
		ms*=timerperiod;
#endif
		while (s!=KRequestPending)
			{
			User::WaitForRequest(s);
			TKeyCode k=console->KeyCode();
			if (k==EKeyTab)
				{
				// calibrate
				TInt64 inst64 = MAKE_TINT64(0, cycles);
				inst64*=1000;
				inst64/=MAKE_TINT64(0,ms);
				MaxCycles=I64LOW(inst64);
				test.Printf(_L("NOPs per second %u\n"),MaxCycles);
				}
			else if (k==EKeyEscape)
				return;
			console->Read(s);
			}
		TInt64 used64=MAKE_TINT64(0, MaxCycles);

		used64-=MAKE_TINT64(0,cycles);
		used64*=1000000;
		used64/=MAKE_TINT64(0,ms);
		used64/=MAKE_TINT64(0, MaxCycles);
		test.Printf(_L("%4d\n"),I64INT(used64));
		}
	}


GLDEF_C TInt E32Main()
	{
	test.SetLogged(EFalse);
	test.Title();
	RThread().SetPriority(EPriorityAbsoluteHigh);

	if (CpuTimeSupported())
		{
		UseKernelCpuTime();
		}
	if (NumberOfCpus()>1)
		{
		test.Printf(_L("Needs RThread::GetCpuTime() on SMP systems\n"));
		}
	else
		MeasureByNOPs();

	return 0;
	}

