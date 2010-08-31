// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\ccthrash.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32hal.h>
#include <e32svr.h>
#include <e32atomics.h>
#include "u32std.h"
#include "prbs.h"

RTest test(_L("Core Control Thrasher"));

TInt NumberOfCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test(r>0);
	return r;
	}

const TInt NCPU = NumberOfCpus();
volatile TInt STFU = 1;

TInt NumberOfActiveCpus(TInt* a)
	{
	SCpuStates s;
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalCpuStates, &s, 0);
	test_KErrNone(r);
	TUint32 ta = s.iTA;
	TUint32 ia = s.iIA;
	if (a)
		*a = __e32_bit_count_32(ta);
	return __e32_bit_count_32(ia);
	}

TInt StableState()
	{
	SCpuStates s;
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalCpuStates, &s, 0);
	test_KErrNone(r);
	if (s.iCU || s.iDC)
		return KErrNotReady;
	if (s.iTA != s.iIA)
		return KErrNotReady;
	if (s.iGD & s.iIA)
		return KErrNotReady;
	TInt i;
	for (i=0; i<NCPU; ++i)
		{
		TBool attached = (s.iUDC[i] == s.iUAC[i]);
		TBool on = s.iIA & (1u<<i);
		if ((on && !attached) || (!on && attached))
			return KErrNotReady;
		}
	return __e32_bit_count_32(s.iTA);
	}

class CThrasher : public CBase
	{
public:
	CThrasher();
	~CThrasher();
	static CThrasher* New();
	TInt Construct();
	void Run();
	void Display();
	TInt ChangeNumberOfCores(TInt aNum, TInt* aCores);
public:
	TUint64 iRunCount;
	TUint64 iTotalTime;
	TUint32 iTimeouts;
	TUint32 iDiff;
	TUint32 iPeriod;
	TUint iSeed[2];
	TUint32 iLastDisplay;
	TUint32 iDisplayPeriod;
	TUint32 iTimeoutPeriod;
	};

CThrasher::CThrasher()
	{
	iSeed[0] = 0xb504f334u;
	iPeriod = 19;
	iLastDisplay = User::NTickCount();
	iDisplayPeriod = 2000;
	iTimeoutPeriod = 200;
	}

CThrasher::~CThrasher()
	{
	}

TInt CThrasher::Construct()
	{
	return KErrNone;
	}

CThrasher* CThrasher::New()
	{
	CThrasher* p = new CThrasher;
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

TInt CThrasher::ChangeNumberOfCores(TInt aNum, TInt* aCores)
	{
	TUint32 initial = User::NTickCount();
	TUint32 final = initial;
	TUint32 elapsed = 0;
	TInt n = -1;
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSetNumberOfCpus, (TAny*)aNum, 0);
	test_KErrNone(r);
	FOREVER
		{
		n = StableState();
		final = User::NTickCount();
		elapsed = final - initial;
		if (n<0 || elapsed<iTimeoutPeriod)
			break;
		User::AfterHighRes(1000);
		}
	if (aCores)
		*aCores = n;
	if (n>=0)
		return elapsed;
	return KErrTimedOut;
	}

void CThrasher::Run()
	{
	TUint x = Random(iSeed);
	x %= iPeriod;
	x += 1;
	x *= 1000;
	User::AfterHighRes(x);
	TUint y = Random(iSeed);
	y %= TUint(NCPU);
	y += 1;
	TInt n = 0;
	TInt t = ChangeNumberOfCores(y, &n);
	if (t < 0)
		{
		++iTimeouts;
		iTotalTime += TUint64(iTimeoutPeriod);
		}
	else
		{
		iTotalTime += TUint64(t);
		if (TUint(n) != y)
			++iDiff;
		}
	++iRunCount;
	TUint32 now = User::NTickCount();
	TUint32 elapsed = now - iLastDisplay;
	if (elapsed >= iDisplayPeriod)
		{
		iLastDisplay = now;
		Display();
		}
	}

void CThrasher::Display()
	{
	TUint64 avg = iTotalTime;
	avg *= TUint64(100);
	avg += TUint64(50);
	avg /= iRunCount;
	TUint32 frac = TUint32(avg % TUint64(100));
	TUint32 integer = TUint32(avg / TUint64(100));
	TUint32 rc32 = (TUint32)iRunCount;
	test.Printf(_L("RC:%10u AVG:%10u.%02u TO:%10u D:%10u\n"), rc32, integer, frac, iTimeouts, iDiff);
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	RThread().SetPriority(EPriorityAbsoluteHigh);

	if (NCPU==1)
		{
		test.Printf(_L("Only works on SMP systems\n"));
		return 0;
		}

	CThrasher* p = CThrasher::New();
	test_NotNull(p);
	while(STFU)
		{
		p->Run();
		}


	return 0;
	}

