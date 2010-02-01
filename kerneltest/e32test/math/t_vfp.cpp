// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_vfp.cpp
// Overview:
// Test the ARM Vector Floating Point operations.
// API Information:
// VFP
// Details:
// - Check that the HAL agrees with the hardware about whether
// VFP is supported.
// - Test setting VFP to IEEE with no exceptions mode, if IEEE mode is
// supported, otherwise leave the mode alone.
// - Test single and double precision vector floating point operations:
// ABS, NEG, ADD, SUB, MUL, DIV, NMUL, SQRT, MAC, MSC, NMAC and NMSC.
// Verify results are as expected - if IEEE mode was set, verify
// bit-for-bit, in accordance with the IEEE specification, otherwise
// use normal floating point equality.
// - Test VFP context save.
// - Test various VFP operations that cause bounces to support code if
// IEEE mode is supported.
// - Test setting VFP to RunFast mode if RunFast mode is supported.
// - Test setting VFP rounding mode.
// - Test inheriting VFP mode to created threads.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

//! @file
//! @SYMTestCaseID KBASE-0017-T_VFP
//! @SYMTestCaseDesc VFPv2 general functionality and bounce handling
//! @SYMREQ 5159
//! @SYMTestPriority Critical
//! @SYMTestActions Check VFP functions correctly in all modes and that mode switching works correctly.
//! @SYMTestExpectedResults Test runs until this message is emitted: RTEST: SUCCESS : T_VFP test completed O.K.
//! @SYMTestType UT

#include "t_vfp.h"
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include <hal.h>
#include <e32svr.h>
#include <u32hal.h>

RTest test(_L("T_VFP"));
TUint32 FPSID;
TUint32 ArchVersion; 
TBool Double;
TBool IEEEMode;
TInt CPUs;
TInt CurrentCpu1;
TInt CurrentCpu2;

typedef void TSglTest(const TReal32* aArgs, TReal32* aResults);
typedef void TDblTest(const TReal64* aArgs, TReal64* aResults);

TBool DetectVFP()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalFloatingPointSystemId, &FPSID, NULL);
	return (r==KErrNone);
	}

TInt TestVFPInitThreadFn(TAny* aPtr)
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)CurrentCpu1, 0);
	TReal32* p = (TReal32*)aPtr;
	TInt i;
	for (i=0; i<32; ++i)
		*p++ = Vfp::SReg(i);
	return 0;
	}

void TestVFPInitialState()
	{
	for (CurrentCpu1 = 0; CurrentCpu1 < CPUs; CurrentCpu1++)
		{
		TReal32 f[32];
		RThread t;
		TInt r = t.Create(KNullDesC, &TestVFPInitThreadFn, 0x1000, NULL, f);
		test(r==KErrNone);
		TRequestStatus s;
		t.Logon(s);
		t.Resume();
		User::WaitForRequest(s);
		TInt xt = t.ExitType();
		TInt xr = t.ExitReason();
		test(xt == EExitKill && xr == KErrNone);
		CLOSE_AND_WAIT(t);
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)CurrentCpu1, 0);
		test.Printf(_L("FPSCR = %08x for core %d\n"), Vfp::Fpscr(), CurrentCpu1);
		const TUint32* p = (const TUint32*)f;
		for (TInt i=0; i<32; ++i)
			{
			if (f[i] != 0.0f)
				{
				test.Printf(_L("S%d = 0x%08x\n"), i, p[i]);
				test(f[i] == 0.0f);
				}
			}
		}
	}

void TestVFPSglRegs(TInt aIter=2)
	{
	TInt i;
	TInt j;
	TInt nSglRegs=0; 

	switch(ArchVersion)	
		{ 
		case ARCH_VERSION_VFPV2:
		case ARCH_VERSION_VFPV3_SUBARCH_V2:
		case ARCH_VERSION_VFPV3_SUBARCH_NULL:
		case ARCH_VERSION_VFPV3_SUBARCH_V3:
			nSglRegs = 32;
			break; 		
		case 0:
		default:
			__ASSERT_ALWAYS(0, User::Panic(_L("Bad VFP version"),__LINE__)); 
			/* NOTREACHED */
		} 

	for (i=0; i<aIter; ++i)
		{
		for (j=0; j<nSglRegs; ++j)
			{
			TInt32 f = i + j;
			Vfp::SetSReg(f, j);
			}
		for (j=0; j<nSglRegs; ++j)
			{
			TInt32 f = i + j;
			TInt32 g = Vfp::SRegInt(j);
			test(f == g);
			}
		}
	}

TInt TestVFPSglRegsThread(TAny*)
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)CurrentCpu1, 0);
	TestVFPSglRegs(KMaxTInt);
	return 0;
	}

void TestVFPDblRegs(TInt aIter=2)
	{
	TInt i;
	TInt j;
	TInt nDblRegs=0; 

	switch(ArchVersion)
		{ 
		case ARCH_VERSION_VFPV2:
			{
			nDblRegs = 16;
			break;
			}
		case ARCH_VERSION_VFPV3_SUBARCH_V2:
		case ARCH_VERSION_VFPV3_SUBARCH_NULL:
		case ARCH_VERSION_VFPV3_SUBARCH_V3:
			{
			TInt vfpType;
			TInt ret = HAL::Get(HALData::EHardwareFloatingPoint, vfpType);
			if (ret == KErrNone && vfpType == EFpTypeVFPv3)
				nDblRegs = 32;
			else
				nDblRegs = 16;
			break;
				}
		case 0:
		default:
			__ASSERT_ALWAYS(0, User::Panic(_L("Bad VFP version"),__LINE__)); 
		} 


	for (i=0; i<aIter; ++i)
		{
		for (j=0; j<nDblRegs; ++j)
			{
			TInt64 f = i + j + KMaxTUint;
			Vfp::SetDReg(f, j);
			}
		for (j=0; j<nDblRegs; ++j)
			{
			TInt64 f = i + j + KMaxTUint;
			TInt64 g = Vfp::DRegInt(j);
			test(f == g);
			}
		}
	}

TInt TestVFPDblRegsThread(TAny*)
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)CurrentCpu2, 0);
	TestVFPDblRegs(KMaxTInt);
	return 0;
	}

void TestVFPContextSave()
	{
	for (CurrentCpu2 = 0; CurrentCpu2 < CPUs; CurrentCpu2++)
		{
		for (CurrentCpu1 = 0; CurrentCpu1 < CPUs; CurrentCpu1++)
			{
			TThreadFunction tf1 = &TestVFPSglRegsThread;
			TThreadFunction tf2 = Double ? &TestVFPDblRegsThread : &TestVFPSglRegsThread;
			RThread t1, t2;
			TInt r;
			r = t1.Create(KNullDesC, tf1, 0x1000, 0x1000, 0x1000, NULL);
			test(r==KErrNone);
			t1.SetPriority(EPriorityLess);
			r = t2.Create(KNullDesC, tf2, 0x1000, 0x1000, 0x1000, NULL);
			test(r==KErrNone);
			t2.SetPriority(EPriorityLess);
			TRequestStatus s1, s2;
			t1.Logon(s1);
			t2.Logon(s2);
			t1.Resume();
			t2.Resume();
			test.Printf(_L("Let threads run concurrently (cores %d and %d)\n"), CurrentCpu1, CurrentCpu2);
			User::After(20*1000*1000/CPUs);

			test.Printf(_L("Kill threads\n"));
			t1.Kill(0);
			t2.Kill(0);
			User::WaitForRequest(s1);
			User::WaitForRequest(s2);
			test(t1.ExitType()==EExitKill && t1.ExitReason()==KErrNone);
			test(t2.ExitType()==EExitKill && t2.ExitReason()==KErrNone);
			CLOSE_AND_WAIT(t1);
			CLOSE_AND_WAIT(t2);
			}
		}
	}

TInt TestBounceCtxThread1(TAny*)
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)Max(CPUs-1, 0), 0);
	for(TInt iter=0; iter<KMaxTInt; ++iter)
		{
		Vfp::SReg(0);
		}
	return KErrNone;
	}

TInt TestBounceCtxThread2(TAny*)
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)Max(CPUs-1, 0), 0);
	TInt start_rep = 0x00800000; // smallest single precision normal number, 1*2^-126
	TReal32 start = *(TReal32*)&start_rep;
	for(TInt iter=0; iter<KMaxTInt; ++iter)
		{
		Vfp::SetSReg(start, 1);
		Vfp::SetSReg(2.0f, 2);
		Vfp::DivS();
		Vfp::CpyS0(1);
		Vfp::MulS();
		Vfp::CpyS0(1);
		TReal32 end = Vfp::SReg(0);
		TInt end_rep = *(TInt*)&end;
		if (start_rep != end_rep)
			{
			RDebug::Printf("mismatch in iter %d, start %08x end %08x\n", iter, start_rep, end_rep);
			test(0);
			}
		}
	return KErrNone;
	}

void DoBounceContextSwitchTests()
	{
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, 0, 0);
	RThread t1, t2;
	TInt r;
	r = t1.Create(KNullDesC, &TestBounceCtxThread1, 0x1000, 0x1000, 0x1000, NULL);
	test(r==KErrNone);
	t1.SetPriority(EPriorityLess);
	r = t2.Create(KNullDesC, &TestBounceCtxThread2, 0x1000, 0x1000, 0x1000, NULL);
	test(r==KErrNone);
	t2.SetPriority(EPriorityLess);
	TRequestStatus s1, s2;
	t1.Logon(s1);
	t2.Logon(s2);
	t1.Resume();
	t2.Resume();
	test.Printf(_L("Let threads run concurrently ...\n"));
	User::After(20*1000*1000);

	test.Printf(_L("Kill threads\n"));
	t1.Kill(0);
	t2.Kill(0);
	User::WaitForRequest(s1);
	User::WaitForRequest(s2);
	test(t1.ExitType()==EExitKill && t1.ExitReason()==KErrNone);
	test(t2.ExitType()==EExitKill && t2.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(t1);
	CLOSE_AND_WAIT(t2);
	}

void TestAbsS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::AbsS();
	r[0] = Vfp::SReg(0);
	r[1] = Abs(a[0]);
	}

void TestAddS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SetSReg(a[1], 2);
	Vfp::AddS();
	r[0] = Vfp::SReg(0);
	r[1] = a[0] + a[1];
	}

void TestDivS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SetSReg(a[1], 2);
	Vfp::DivS();
	r[0] = Vfp::SReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.DivEq(y);
	r[1] = (TReal32)x;
	}

void TestMacS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 0);
	Vfp::SetSReg(a[1], 1);
	Vfp::SetSReg(a[2], 2);
	Vfp::MacS();
	r[0] = Vfp::SReg(0);
	r[1] = a[0] + a[1] * a[2];
	}

void TestMscS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 0);
	Vfp::SetSReg(a[1], 1);
	Vfp::SetSReg(a[2], 2);
	Vfp::MscS();
	r[0] = Vfp::SReg(0);
	r[1] = a[1] * a[2] - a[0];
	}

void TestMulS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SetSReg(a[1], 2);
	Vfp::MulS();
	r[0] = Vfp::SReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.MultEq(y);
	r[1] = (TReal32)x;
	}

void TestNegS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::NegS();
	r[0] = Vfp::SReg(0);
	r[1] = -a[0];
	}

void TestNMacS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 0);
	Vfp::SetSReg(a[1], 1);
	Vfp::SetSReg(a[2], 2);
	Vfp::NMacS();
	r[0] = Vfp::SReg(0);
	r[1] = a[0] - a[1] * a[2];
	}

void TestNMscS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 0);
	Vfp::SetSReg(a[1], 1);
	Vfp::SetSReg(a[2], 2);
	Vfp::NMscS();
	r[0] = Vfp::SReg(0);
	r[1] = -a[1] * a[2] - a[0];
	}

void TestNMulS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SetSReg(a[1], 2);
	Vfp::NMulS();
	r[0] = Vfp::SReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.MultEq(y);
	r[1] = -(TReal32)x;
	}

void TestSqrtS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SqrtS();
	r[0] = Vfp::SReg(0);
	TReal x = a[0];
	TReal y;
	Math::Sqrt(y, x);
	r[1] = (TReal32)y;
	}

void TestSubS(const TReal32* a, TReal32* r)
	{
	Vfp::SetSReg(a[0], 1);
	Vfp::SetSReg(a[1], 2);
	Vfp::SubS();
	r[0] = Vfp::SReg(0);
	r[1] = a[0] - a[1];
	}



void TestAbsD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::AbsD();
	r[0] = Vfp::DReg(0);
	r[1] = Abs(a[0]);
	}

void TestAddD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SetDReg(a[1], 2);
	Vfp::AddD();
	r[0] = Vfp::DReg(0);
	r[1] = a[0] + a[1];
	}

void TestDivD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SetDReg(a[1], 2);
	Vfp::DivD();
	r[0] = Vfp::DReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.DivEq(y);
	r[1] = (TReal64)x;
	}

void TestMacD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 0);
	Vfp::SetDReg(a[1], 1);
	Vfp::SetDReg(a[2], 2);
	Vfp::MacD();
	r[0] = Vfp::DReg(0);
	r[1] = a[0] + a[1] * a[2];
	}

void TestMscD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 0);
	Vfp::SetDReg(a[1], 1);
	Vfp::SetDReg(a[2], 2);
	Vfp::MscD();
	r[0] = Vfp::DReg(0);
	r[1] = a[1] * a[2] - a[0];
	}

void TestMulD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SetDReg(a[1], 2);
	Vfp::MulD();
	r[0] = Vfp::DReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.MultEq(y);
	r[1] = (TReal64)x;
	}

void TestNegD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::NegD();
	r[0] = Vfp::DReg(0);
	r[1] = -a[0];
	}

void TestNMacD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 0);
	Vfp::SetDReg(a[1], 1);
	Vfp::SetDReg(a[2], 2);
	Vfp::NMacD();
	r[0] = Vfp::DReg(0);
	r[1] = a[0] - a[1] * a[2];
	}

void TestNMscD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 0);
	Vfp::SetDReg(a[1], 1);
	Vfp::SetDReg(a[2], 2);
	Vfp::NMscD();
	r[0] = Vfp::DReg(0);
	r[1] = -a[1] * a[2] - a[0];
	}

void TestNMulD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SetDReg(a[1], 2);
	Vfp::NMulD();
	r[0] = Vfp::DReg(0);
	TRealX x(a[0]);
	TRealX y(a[1]);
	x.MultEq(y);
	r[1] = -(TReal64)x;
	}

void TestSqrtD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SqrtD();
	r[0] = Vfp::DReg(0);
	TReal x = a[0];
	TReal y;
	Math::Sqrt(y, x);
	r[1] = (TReal64)y;
	}

void TestSubD(const TReal64* a, TReal64* r)
	{
	Vfp::SetDReg(a[0], 1);
	Vfp::SetDReg(a[1], 2);
	Vfp::SubD();
	r[0] = Vfp::DReg(0);
	r[1] = a[0] - a[1];
	}

#define DO_SGL_TEST1(name, func, a1)			DoSglTest(name, __LINE__, func, a1)
#define DO_SGL_TEST2(name, func, a1, a2)		DoSglTest(name, __LINE__, func, a1, a2)
#define DO_SGL_TEST3(name, func, a1, a2, a3)	DoSglTest(name, __LINE__, func, a1, a2, a3)
void DoSglTest(const char* aName, TInt aLine, TSglTest aFunc, TReal32 a1, TReal32 a2=0.0f, TReal32 a3=0.0f)
	{
	TPtrC8 name8((const TText8*)aName);
	TBuf<128> name16;
	name16.Copy(name8);
	test.Printf(_L("%S(%g,%g,%g)\n"), &name16, a1, a2, a3);
	TReal32 args[3] = {a1, a2, a3};
	TReal32 results[2];
	(*aFunc)(args, results);
	if (IEEEMode)
		{
		if (*((TUint32*)&(results[0])) == *((TUint32*)&(results[1])))
			return;
		}
	else
		{
		if (results[0] == results[1])
			return;
		}
	const TUint32* pa = (const TUint32*)args;
	const TUint32* pr = (const TUint32*)results;
	test.Printf(_L("a1=%08x a2=%08x a3=%08x\n"), pa[0], pa[1], pa[2]);
	test.Printf(_L("actual result = %08x (%g)\nexpected result = %08x (%g)\n"), pr[0], results[0], pr[1], results[1]);
	test.Printf(_L("Test at line %d failed\n"), aLine);
	test(0);
	}

void DoSglTests()
	{
	// ABS
	DO_SGL_TEST1("ABS", &TestAbsS, 1.0f);
	DO_SGL_TEST1("ABS", &TestAbsS, -1.0f);
	DO_SGL_TEST1("ABS", &TestAbsS, 0.0f);
	DO_SGL_TEST1("ABS", &TestAbsS, -3.1415926536f);

	// NEG
	DO_SGL_TEST1("NEG", &TestNegS, 1.0f);
	DO_SGL_TEST1("NEG", &TestNegS, -1.0f);
	DO_SGL_TEST1("NEG", &TestNegS, 0.0f);
	DO_SGL_TEST1("NEG", &TestNegS, -3.1415926536f);

	// ADD
	DO_SGL_TEST2("ADD", &TestAddS, 0.0f, 0.0f);
	DO_SGL_TEST2("ADD", &TestAddS, 0.0f, 1.0f);
	DO_SGL_TEST2("ADD", &TestAddS, -1.0f, 1.0f);
	DO_SGL_TEST2("ADD", &TestAddS, 1.0f, 2.5f);
	DO_SGL_TEST2("ADD", &TestAddS, 1.0f, 6.022045e23f);
	DO_SGL_TEST2("ADD", &TestAddS, -7.3890561f, 1.414213562f);
	DO_SGL_TEST2("ADD", &TestAddS, -7.3890561f, -1.414213562f);

	// SUB
	DO_SGL_TEST2("SUB", &TestSubS, 0.0f, 0.0f);
	DO_SGL_TEST2("SUB", &TestSubS, 0.0f, 1.0f);
	DO_SGL_TEST2("SUB", &TestSubS, 1.0f, 1.0f);
	DO_SGL_TEST2("SUB", &TestSubS, 1.0f, 2.5f);
	DO_SGL_TEST2("SUB", &TestSubS, 91.0f, 2.5f);
	DO_SGL_TEST2("SUB", &TestSubS, 1.0f, 6.022045e23f);
	DO_SGL_TEST2("SUB", &TestSubS, -7.3890561f, 1.414213562f);
	DO_SGL_TEST2("SUB", &TestSubS, -7.3890561f, -1.414213562f);

	// MUL
	DO_SGL_TEST2("MUL", &TestMulS, 0.0f, 0.0f);
	DO_SGL_TEST2("MUL", &TestMulS, 1.0f, 0.0f);
	DO_SGL_TEST2("MUL", &TestMulS, 0.0f, 1.0f);
	DO_SGL_TEST2("MUL", &TestMulS, 2.5f, 6.5f);
	DO_SGL_TEST2("MUL", &TestMulS, -39.6f, 19.72f);
	DO_SGL_TEST2("MUL", &TestMulS, -10.1f, -20.1f);
	DO_SGL_TEST2("MUL", &TestMulS, 1e20f, 1e20f);
	DO_SGL_TEST2("MUL", &TestMulS, 1e-30f, 1e-30f);

	// DIV
	DO_SGL_TEST2("DIV", &TestDivS, 0.0f, 1.0f);
	DO_SGL_TEST2("DIV", &TestDivS, 1.0f, 5.0f);
	DO_SGL_TEST2("DIV", &TestDivS, 1.0f, -5.0f);
	DO_SGL_TEST2("DIV", &TestDivS, -1.0f, 5.0f);
	DO_SGL_TEST2("DIV", &TestDivS, -1.0f, -5.0f);
	DO_SGL_TEST2("DIV", &TestDivS, 7.3890561f, 2.7182818f);
	DO_SGL_TEST2("DIV", &TestDivS, 1e20f, 1e-20f);
	DO_SGL_TEST2("DIV", &TestDivS, 1e-30f, 1e30f);

	// NMUL
	DO_SGL_TEST2("NMUL", &TestNMulS, 0.0f, 0.0f);
	DO_SGL_TEST2("NMUL", &TestNMulS, 1.0f, 0.0f);
	DO_SGL_TEST2("NMUL", &TestNMulS, 0.0f, 1.0f);
	DO_SGL_TEST2("NMUL", &TestNMulS, 2.5f, 6.5f);
	DO_SGL_TEST2("NMUL", &TestNMulS, -39.6f, 19.72f);
	DO_SGL_TEST2("NMUL", &TestNMulS, -10.1f, -20.1f);
	DO_SGL_TEST2("NMUL", &TestNMulS, 1e20f, 1e20f);
	DO_SGL_TEST2("NMUL", &TestNMulS, 1e-30f, 1e-30f);

	// SQRT
	DO_SGL_TEST1("SQRT", &TestSqrtS, 0.0f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 1.0f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 2.0f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 3.0f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 9096256.0f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 1e36f);
	DO_SGL_TEST1("SQRT", &TestSqrtS, 1e-36f);

	// MAC
	DO_SGL_TEST3("MAC", &TestMacS, 0.0f, 0.0f, 0.0f);
	DO_SGL_TEST3("MAC", &TestMacS, 0.0f, 1.0f, 0.0f);
	DO_SGL_TEST3("MAC", &TestMacS, 0.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("MAC", &TestMacS, -1.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("MAC", &TestMacS, 0.8f, 0.1f, 8.0f);
	DO_SGL_TEST3("MAC", &TestMacS, 0.8f, -0.1f, 8.0f);
	DO_SGL_TEST3("MAC", &TestMacS, -0.8f, -0.1f, -8.0f);
	DO_SGL_TEST3("MAC", &TestMacS, 0.8f, 0.3333333333f, 3.1415926536f);

	// MSC
	DO_SGL_TEST3("MSC", &TestMscS, 0.0f, 0.0f, 0.0f);
	DO_SGL_TEST3("MSC", &TestMscS, 0.0f, 1.0f, 0.0f);
	DO_SGL_TEST3("MSC", &TestMscS, 0.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("MSC", &TestMscS, -1.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("MSC", &TestMscS, 0.8f, 0.1f, 8.0f);
	DO_SGL_TEST3("MSC", &TestMscS, 0.8f, -0.1f, 8.0f);
	DO_SGL_TEST3("MSC", &TestMscS, -0.8f, -0.1f, -8.0f);
	DO_SGL_TEST3("MSC", &TestMscS, 0.8f, 0.3333333333f, 3.1415926536f);

	// NMAC
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.0f, 0.0f, 0.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.0f, 1.0f, 0.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, -1.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.8f, 0.1f, 8.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.8f, -0.1f, 8.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, -0.8f, -0.1f, -8.0f);
	DO_SGL_TEST3("NMAC", &TestNMacS, 0.8f, 0.3333333333f, 3.1415926536f);

	// NMSC
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.0f, 0.0f, 0.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.0f, 1.0f, 0.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, -1.0f, 1.0f, 1.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.8f, 0.1f, 8.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.8f, -0.1f, 8.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, -0.8f, -0.1f, -8.0f);
	DO_SGL_TEST3("NMSC", &TestNMscS, 0.8f, 0.3333333333f, 3.1415926536f);
	}

#define DO_DBL_TEST1(name, func, a1)			DoDblTest(name, __LINE__, func, a1)
#define DO_DBL_TEST2(name, func, a1, a2)		DoDblTest(name, __LINE__, func, a1, a2)
#define DO_DBL_TEST3(name, func, a1, a2, a3)	DoDblTest(name, __LINE__, func, a1, a2, a3)
void DoDblTest(const char* aName, TInt aLine, TDblTest aFunc, TReal64 a1, TReal64 a2=0.0, TReal64 a3=0.0)
	{
	TPtrC8 name8((const TText8*)aName);
	TBuf<128> name16;
	name16.Copy(name8);
	test.Printf(_L("%S(%g,%g,%g)\n"), &name16, a1, a2, a3);
	TReal64 args[3] = {a1, a2, a3};
	TReal64 results[2];
	SDouble sargs[3];
	sargs[0] = a1;
	sargs[1] = a2;
	sargs[2] = a3;
	(*aFunc)(args, results);
	if (IEEEMode)
		{
		if (*((TUint64*)&(results[0])) == *((TUint64*)&(results[1])))
			return;
		}
	else
		{
		if (results[0] == results[1])
			return;
		}
	SDouble sres[3];
	sres[0] = results[0];
	sres[1] = results[1];
	test.Printf(_L("a1=%08x %08x\na2=%08x %08x\na3=%08x %08x\n"), sargs[0].iData[1], sargs[0].iData[0],
								sargs[1].iData[1], sargs[1].iData[0], sargs[2].iData[1], sargs[2].iData[0]);
	test.Printf(_L("actual result = %08x %08x (%g)\nexpected result = %08x %08x (%g)\n"),
			sres[0].iData[1], sres[0].iData[0], results[0], sres[1].iData[1], sres[1].iData[0], results[1]);
	test.Printf(_L("Test at line %d failed\n"), aLine);
	test(0);
	}

void DoDblTests()
	{
	// ABS
	DO_DBL_TEST1("ABS", &TestAbsD, 1.0);
	DO_DBL_TEST1("ABS", &TestAbsD, -1.0);
	DO_DBL_TEST1("ABS", &TestAbsD, 0.0);
	DO_DBL_TEST1("ABS", &TestAbsD, -3.1415926536);

	// NEG
	DO_DBL_TEST1("NEG", &TestNegD, 1.0);
	DO_DBL_TEST1("NEG", &TestNegD, -1.0);
	DO_DBL_TEST1("NEG", &TestNegD, 0.0);
	DO_DBL_TEST1("NEG", &TestNegD, -3.1415926536);

	// ADD
	DO_DBL_TEST2("ADD", &TestAddD, 0.0, 0.0);
	DO_DBL_TEST2("ADD", &TestAddD, 0.0, 1.0);
	DO_DBL_TEST2("ADD", &TestAddD, -1.0, 1.0);
	DO_DBL_TEST2("ADD", &TestAddD, 1.0, 2.5);
	DO_DBL_TEST2("ADD", &TestAddD, 1.0, 6.022045e23);
	DO_DBL_TEST2("ADD", &TestAddD, -7.3890561, 1.414213562);
	DO_DBL_TEST2("ADD", &TestAddD, -7.3890561, -1.414213562);

	// SUB
	DO_DBL_TEST2("SUB", &TestSubD, 0.0, 0.0);
	DO_DBL_TEST2("SUB", &TestSubD, 0.0, 1.0);
	DO_DBL_TEST2("SUB", &TestSubD, 1.0, 1.0);
	DO_DBL_TEST2("SUB", &TestSubD, 1.0, 2.5);
	DO_DBL_TEST2("SUB", &TestSubD, 91.0, 2.5);
	DO_DBL_TEST2("SUB", &TestSubD, 1.0, 6.022045e23);
	DO_DBL_TEST2("SUB", &TestSubD, -7.3890561, 1.414213562);
	DO_DBL_TEST2("SUB", &TestSubD, -7.3890561, -1.414213562);

	// MUL
	DO_DBL_TEST2("MUL", &TestMulD, 0.0, 0.0);
	DO_DBL_TEST2("MUL", &TestMulD, 1.0, 0.0);
	DO_DBL_TEST2("MUL", &TestMulD, 0.0, 1.0);
	DO_DBL_TEST2("MUL", &TestMulD, 2.5, 6.5);
	DO_DBL_TEST2("MUL", &TestMulD, -39.6, 19.72);
	DO_DBL_TEST2("MUL", &TestMulD, -10.1, -20.1);
	DO_DBL_TEST2("MUL", &TestMulD, 1e20, 1e20);
	DO_DBL_TEST2("MUL", &TestMulD, 1e100, 1e300);
	DO_DBL_TEST2("MUL", &TestMulD, 1e-20, 1e-20);
	DO_DBL_TEST2("MUL", &TestMulD, 1e-200, 1e-300);

	// DIV
	DO_DBL_TEST2("DIV", &TestDivD, 0.0, 1.0);
	DO_DBL_TEST2("DIV", &TestDivD, 1.0, 5.0);
	DO_DBL_TEST2("DIV", &TestDivD, 1.0, -5.0);
	DO_DBL_TEST2("DIV", &TestDivD, -1.0, 5.0);
	DO_DBL_TEST2("DIV", &TestDivD, -1.0, -5.0);
	DO_DBL_TEST2("DIV", &TestDivD, 7.3890561, 2.7182818);
	DO_DBL_TEST2("DIV", &TestDivD, 1e20, 1e-20);
	DO_DBL_TEST2("DIV", &TestDivD, 1e-20, 1e20);
	DO_DBL_TEST2("DIV", &TestDivD, 1e-50, 1e300);

	// NMUL
	DO_DBL_TEST2("NMUL", &TestNMulD, 0.0, 0.0);
	DO_DBL_TEST2("NMUL", &TestNMulD, 1.0, 0.0);
	DO_DBL_TEST2("NMUL", &TestNMulD, 0.0, 1.0);
	DO_DBL_TEST2("NMUL", &TestNMulD, 2.5, 6.5);
	DO_DBL_TEST2("NMUL", &TestNMulD, -39.6, 19.72);
	DO_DBL_TEST2("NMUL", &TestNMulD, -10.1, -20.1);
	DO_DBL_TEST2("NMUL", &TestNMulD, 1e20, 1e20);
	DO_DBL_TEST2("NMUL", &TestNMulD, 1e100, 1e300);
	DO_DBL_TEST2("NMUL", &TestNMulD, 1e-20, 1e-20);
	DO_DBL_TEST2("NMUL", &TestNMulD, 1e-200, 1e-300);

	// SQRT
	DO_DBL_TEST1("SQRT", &TestSqrtD, 0.0);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 1.0);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 2.0);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 3.0);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 9096256.0);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 1e36);
	DO_DBL_TEST1("SQRT", &TestSqrtD, 1e-36);

	// MAC
	DO_DBL_TEST3("MAC", &TestMacD, 0.0, 0.0, 0.0);
	DO_DBL_TEST3("MAC", &TestMacD, 0.0, 1.0, 0.0);
	DO_DBL_TEST3("MAC", &TestMacD, 0.0, 1.0, 1.0);
	DO_DBL_TEST3("MAC", &TestMacD, -1.0, 1.0, 1.0);
	DO_DBL_TEST3("MAC", &TestMacD, 0.8, 0.1, 8.0);
	DO_DBL_TEST3("MAC", &TestMacD, 0.8, -0.1, 8.0);
	DO_DBL_TEST3("MAC", &TestMacD, -0.8, -0.1, -8.0);
	DO_DBL_TEST3("MAC", &TestMacD, 0.8, 0.3333333333, 3.1415926536);

	// MSC
	DO_DBL_TEST3("MSC", &TestMscD, 0.0, 0.0, 0.0);
	DO_DBL_TEST3("MSC", &TestMscD, 0.0, 1.0, 0.0);
	DO_DBL_TEST3("MSC", &TestMscD, 0.0, 1.0, 1.0);
	DO_DBL_TEST3("MSC", &TestMscD, -1.0, 1.0, 1.0);
	DO_DBL_TEST3("MSC", &TestMscD, 0.8, 0.1, 8.0);
	DO_DBL_TEST3("MSC", &TestMscD, 0.8, -0.1, 8.0);
	DO_DBL_TEST3("MSC", &TestMscD, -0.8, -0.1, -8.0);
	DO_DBL_TEST3("MSC", &TestMscD, 0.8, 0.3333333333, 3.1415926536);

	// NMAC
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.0, 0.0, 0.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.0, 1.0, 0.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.0, 1.0, 1.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, -1.0, 1.0, 1.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.8, 0.1, 8.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.8, -0.1, 8.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, -0.8, -0.1, -8.0);
	DO_DBL_TEST3("NMAC", &TestNMacD, 0.8, 0.3333333333, 3.1415926536);

	// NMSC
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.0, 0.0, 0.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.0, 1.0, 0.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.0, 1.0, 1.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, -1.0, 1.0, 1.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.8, 0.1, 8.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.8, -0.1, 8.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, -0.8, -0.1, -8.0);
	DO_DBL_TEST3("NMSC", &TestNMscD, 0.8, 0.3333333333, 3.1415926536);
	}

void DoBounceTests()
	{
	test.Next(_L("Test denormal handling - single"));
	DO_SGL_TEST2("ADD", &TestAddS, 1e-39f, 1e-39f);
	test.Next(_L("Test potential underflow - single"));
	DO_SGL_TEST2("MUL", &TestMulS, 3.162e-20f, 3.162e-20f);
// fails on VFPv2 hardware. ARM's library should be fixed
//	test.Next(_L("Test NaN input - single"));
//	TReal32 aSingleNaN;
//	*((TUint32*)&aSingleNaN) = 0x7F9ABCDE;
//	Vfp::SetSReg(aSingleNaN, 1);
//	Vfp::SetSReg(aSingleNaN, 2);
//	Vfp::AddS();
//	TReal32 aSingleResult = Vfp::SReg(0);
//	test(*((TUint32*)&aSingleResult) == 0x7FDABCDE);

	if (Double)
		{
		test.Next(_L("Test denormal handling - double"));
		DO_DBL_TEST2("ADD", &TestAddD, 3.1234e-322, 3.1234e-322);
		test.Next(_L("Test potential underflow - double"));
		DO_DBL_TEST2("MUL", &TestMulD, 1.767e-161, 1.767e-161);
// fails on VFPv2 hardware. ARM's library should be fixed
//		test.Next(_L("Test NaN input - double"));
//		TReal64 aDoubleNaN;
//		*((TUint64*)&aDoubleNaN) = 0x7FF0123456789ABCll;
//		Vfp::SetDReg(aDoubleNaN, 1);
//		Vfp::SetDReg(aDoubleNaN, 2);
//		Vfp::AddD();
//		TReal64 aDoubleResult = Vfp::DReg(0);
//		test(*((TUint64*)&aDoubleResult) == 0x7FF8123456789ABC);
		}
	}

void DoRunFastTests()
	{
	test.Next(_L("Test flushing denormals to zero - single"));
	Vfp::SetSReg(1e-39f, 1);
	Vfp::SetSReg(1e-39f, 2);
	Vfp::AddS();
	test(Vfp::SReg(0)==0);

	test.Next(_L("Test flushing underflow to zero - single"));
	Vfp::SetSReg(3.162e-20f, 1);
	Vfp::SetSReg(3.162e-20f, 2);
	Vfp::MulS();
	test(Vfp::SReg(0)==0);

	test.Next(_L("Test default NaNs - single"));
	TReal32 aSingleNaN;
	*((TUint32*)&aSingleNaN) = 0x7F9ABCDE;
	Vfp::SetSReg(aSingleNaN, 1);
	Vfp::SetSReg(aSingleNaN, 2);
	Vfp::AddS();
	TReal32 aSingleResult = Vfp::SReg(0);
	test(*((TUint32*)&aSingleResult) == 0x7FC00000);

	if (Double)
		{
		test.Next(_L("Test flushing denormals to zero - double"));
		Vfp::SetDReg(3.1234e-322, 1);
		Vfp::SetDReg(3.1234e-322, 2);
		Vfp::AddD();
		test(Vfp::DReg(0)==0);
	
		test.Next(_L("Test flushing underflow to zero - double"));
		Vfp::SetDReg(1.767e-161, 1);
		Vfp::SetDReg(1.767e-161, 2);
		Vfp::MulD();
		test(Vfp::DReg(0)==0);

		test.Next(_L("Test default NaNs - double"));
		TReal64 aDoubleNaN;
		*((TUint64*)&aDoubleNaN) = 0x7FF0123456789ABCll;
		Vfp::SetDReg(aDoubleNaN, 1);
		Vfp::SetDReg(aDoubleNaN, 2);
		Vfp::AddD();
		TReal64 aDoubleResult = Vfp::DReg(0);
		test(*((TUint64*)&aDoubleResult) == 0x7FF8000000000000ll);
		}
	}

void TestAddSResult(const TReal32 a, const TReal32 b, const TReal32 r)
	{
	Vfp::SetSReg(a, 1);
	Vfp::SetSReg(b, 2);
	Vfp::AddS();
	test(Vfp::SReg(0) == r);
	}

void DoRoundingTests()
	{
	TFloatingPointMode fpmode = IEEEMode ? EFpModeIEEENoExceptions : EFpModeRunFast;
	test.Next(_L("Check default rounding to nearest"));
	test(User::SetFloatingPointMode(fpmode) == KErrNone);
	test.Next(_L("Check nearest-downward"));
	TestAddSResult(16777215, 0.49f, 16777215);
	test.Next(_L("Check nearest-upward"));
	TestAddSResult(16777215, 0.51f, 16777216);
	test.Next(_L("Set rounding mode to toward-plus-infinity"));
	test(User::SetFloatingPointMode(fpmode, EFpRoundToPlusInfinity) == KErrNone);
	test.Next(_L("Check positive rounding goes upward"));
	TestAddSResult(16777215, 0.49f, 16777216);
	test.Next(_L("Check negative rounding goes upward"));
	TestAddSResult(-16777215, -0.51f, -16777215);
	test.Next(_L("Set rounding mode to toward-minus-infinity"));
	test(User::SetFloatingPointMode(fpmode, EFpRoundToMinusInfinity) == KErrNone);
	test.Next(_L("Check positive rounding goes downward"));
	TestAddSResult(16777215, 0.51f, 16777215);
	test.Next(_L("Check negative rounding goes downward"));
	TestAddSResult(-16777215, -0.49f, -16777216);
	test.Next(_L("Set rounding mode to toward-zero"));
	test(User::SetFloatingPointMode(fpmode, EFpRoundToZero) == KErrNone);
	test.Next(_L("Check positive rounding goes downward"));
	TestAddSResult(16777215, 0.51f, 16777215);
	test.Next(_L("Check negative rounding goes upward"));
	TestAddSResult(-16777215, -0.51f, -16777215);
	}

TInt RunFastThread(TAny* /*unused*/)
	{
	Vfp::SetSReg(1e-39f, 1);
	Vfp::SetSReg(1e-39f, 2);
	Vfp::AddS();
	return (Vfp::SReg(0)==0) ? KErrNone : KErrGeneral;
	}

TInt IEEECompliantThread(TAny* /*unused*/)
	{
	Vfp::SetSReg(1e-39f, 1);
	Vfp::SetSReg(1e-39f, 2);
	Vfp::AddS();
	return (Vfp::SReg(0)==2e-39f) ? KErrNone : KErrGeneral;
	}

void TestVFPModeInheritance()
	{
	test.Printf(_L("Set floating point mode to RunFast\n"));
	test(User::SetFloatingPointMode(EFpModeRunFast)==KErrNone);
	RThread t;
	TInt r = t.Create(KNullDesC, &RunFastThread, 0x1000, NULL, NULL);
	test(r==KErrNone);
	TRequestStatus s;
	t.Logon(s);
	test.Printf(_L("Run RunFast test in another thread...\n"));
	t.Resume();
	test.Printf(_L("Wait for other thread to terminate\n"));
	User::WaitForRequest(s);
	test(t.ExitType() == EExitKill);
	test(s == KErrNone);
	CLOSE_AND_WAIT(t);
	test.Printf(_L("Set floating point mode to IEEE\n"));
	test(User::SetFloatingPointMode(EFpModeIEEENoExceptions)==KErrNone);
	r = t.Create(KNullDesC, &IEEECompliantThread, 0x1000, NULL, NULL);
	test(r==KErrNone);
	t.Logon(s);
	test.Printf(_L("Run IEEE test in another thread...\n"));
	t.Resume();
	test.Printf(_L("Wait for other thread to terminate\n"));
	User::WaitForRequest(s);
	test(t.ExitType() == EExitKill);
	test(s == KErrNone);
	CLOSE_AND_WAIT(t);
	}


void TestVFPv3()
	{
	test.Next(_L("Transferring to and from fixed point"));
	
	Vfp::SetSReg(2.5f, 0);
	test(Vfp::SReg(0)==2.5f);
	Vfp::ToFixedS(3);				// Convert to fixed (3) precision
	test(Vfp::SRegInt(0)==0x14);	// 10.100 in binary fixed(3) format
	Vfp::FromFixedS(3);				//Convert from fixed (3) precision
	test(Vfp::SReg(0)==2.5f);

	
	test.Next(_L("Setting immediate value to floating point registers"));
	
	Vfp::SetSReg(5.0f, 0);
	test(Vfp::SReg(0) == 5.0f);
	Vfp::TconstS2();
	test(Vfp::SReg(0) == 2.0f);
	Vfp::SetSReg(5.0f, 0);
	Vfp::TconstS2_8();
	test(Vfp::SReg(0) == 2.875f);
	
	Vfp::SetDReg(5.0f, 0);
	test(Vfp::DReg(0) == 5.0f);
	Vfp::TconstD2();
	test(Vfp::DReg(0) == 2.0f);
	Vfp::TconstD2_8();
	test(Vfp::DReg(0) == 2.875f);
	}

void TestNEON()
	{
	RThread t;
	TRequestStatus s;
	test.Next(_L("Test creating a thread to execute an F2-prefix instruction"));
	test_KErrNone(t.Create(KNullDesC, &NeonWithF2, 0x1000, NULL, NULL));
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType() == EExitKill);
	test(s == KErrNone);
	t.Close();
	test.Next(_L("Test creating a thread to execute an F3-prefix instruction"));
	test_KErrNone(t.Create(KNullDesC, &NeonWithF3, 0x1000, NULL, NULL));
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType() == EExitKill);
	test(s == KErrNone);
	t.Close();
	test.Next(_L("Test creating a thread to execute an F4x-prefix instruction"));
	test_KErrNone(t.Create(KNullDesC, &NeonWithF4x, 0x1000, NULL, NULL));
	t.Logon(s);
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType() == EExitKill);
	test(s == KErrNone);
	t.Close();
	}

void TestThumb()
	{
	RThread t;
	TRequestStatus s;
	TInt testStep = 0;
	do {
		test_KErrNone(t.Create(KNullDesC, &ThumbMode, 0x1000, NULL, (TAny*)testStep++));
		t.Logon(s);
		t.Resume();
		User::WaitForRequest(s);
		test(s == KErrNone || s == 1);
		test(t.ExitType() == EExitKill);
		t.Close();
		}
	while (s == KErrNone);

	test(s == 1);
	test(testStep == 7);
	}

TInt TestThreadMigration(TAny* aPtr)
	{
	const TInt inc = (TInt)aPtr;
	for (TInt32 switches = 0; switches < KMaxTInt; switches += inc)
		{
		Vfp::SetSReg(switches, switches % 16);
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny*)(switches % CPUs), 0);
		test(Vfp::SRegInt(switches % 16) == switches);
		}
	return KErrNone;
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Ask HAL if we have hardware floating point"));

	CPUs = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	TInt supportedTypes;
	TInt HalVfp = HAL::Get(HALData::EHardwareFloatingPoint, supportedTypes);
	if (HalVfp == KErrNone) 
		{ 
		if (supportedTypes == EFpTypeVFPv2) 
			{ 
			test.Printf(_L("HAL reports VFPv2\n"));
			} 
		else if (supportedTypes == EFpTypeVFPv3)
			{ 
			test.Printf(_L("HAL reports VFPv3\n"));
			} 
		else if (supportedTypes == EFpTypeVFPv3D16)
			{ 
			test.Printf(_L("HAL reports VFPv3-D16\n"));
			} 
		else
			{
			test.Printf(_L("HAL reports an unknown floating point type\n"));
			test(0);
			}
		} 
	else
		{ 
		test.Printf(_L("HAL reports no VFP support\n"));
		} 
		
	test.Next(_L("Check VFP present"));
	TBool present = DetectVFP();
	if (!present)
		{
		test.Printf(_L("No VFP detected\n"));
		test(HalVfp == KErrNotSupported || 
						((supportedTypes != EFpTypeVFPv2) && 
						(supportedTypes != EFpTypeVFPv3) && 
						(supportedTypes != EFpTypeVFPv3D16))
						);
		test.End();
		return 0;
		}
	
	test.Printf(_L("VFP detected. FPSID = %08x\n"), FPSID);
	test(HalVfp == KErrNone);

	// Verify that the HAL architecture ID matches the FPSID values
	// ARMv7 redefines some of these bits so the masks are different :(
	if (supportedTypes == EFpTypeVFPv2)
		{
		// assume armv5/6's bit definitions, where 19:16 are the arch version
		// and 20 is the single-precision-only bit
		ArchVersion = (FPSID >> 16) & 0xf;
		test(ArchVersion == ARCH_VERSION_VFPV2);
		Double = !(FPSID & VFP_FPSID_SNG);
		}
	else if (supportedTypes == EFpTypeVFPv3 || supportedTypes == EFpTypeVFPv3D16)
		{
		// assume armv7's bit definitions, where 22:16 are the arch version
		ArchVersion = (FPSID >> 16) & 0x7f;
		test(ArchVersion == ARCH_VERSION_VFPV3_SUBARCH_V2
		  || ArchVersion == ARCH_VERSION_VFPV3_SUBARCH_NULL
		  || ArchVersion == ARCH_VERSION_VFPV3_SUBARCH_V3); 
		// there are bits for this in MVFR0 but ARM implementations should always have it?
		Double = ETrue;
		}

	if (Double)
		test.Printf(_L("Both single and double precision supported\n"), FPSID);
	else
		test.Printf(_L("Only single precision supported\n"), FPSID);

	test.Next(_L("Test VFP Initial State"));
	TestVFPInitialState();

	test.Next(_L("Test setting VFP to IEEE no exceptions mode"));
	IEEEMode = User::SetFloatingPointMode(EFpModeIEEENoExceptions) == KErrNone;
	if (!IEEEMode)
		test.Printf(_L("IEEE no exceptions mode not supported, continuing in RunFast\n"));

	test.Next(_L("Test VFP calculations - single"));
	DoSglTests();
	if (Double)
		{
		test.Next(_L("Test VFP calculations - double"));
		DoDblTests();
		}

	test.Next(_L("Test VFP Context Save"));
	TestVFPContextSave();

	if (IEEEMode)
		{
		test.Next(_L("Test bounce handling"));
		DoBounceTests();
		test.Next(_L("Test bouncing while context switching"));
		DoBounceContextSwitchTests();
		test.Next(_L("Test setting VFP to RunFast mode"));
		test(User::SetFloatingPointMode(EFpModeRunFast) == KErrNone);
		DoRunFastTests();
		}

	test.Next(_L("Test VFP rounding modes"));
	DoRoundingTests();

	if (IEEEMode)
		{
		test.Next(_L("Test VFP mode inheritance between threads"));
		TestVFPModeInheritance();
		}

	if (supportedTypes == EFpTypeVFPv3 || supportedTypes == EFpTypeVFPv3D16)
		{
		test.Next(_L("Test VFPv3"));
		TestVFPv3();

		if (supportedTypes == EFpTypeVFPv3)
			{
			test.Next(_L("Test NEON"));
			TestNEON();

#if defined(__SUPPORT_THUMB_INTERWORKING)
			test.Next(_L("Test Thumb Decode"));
			TestThumb();
#endif
			}
		}

	if (CPUs > 1)
		{
		test.Next(_L("Test SMP Thread Migration"));
		TInt inc = 1;
		RThread t[8];
		TRequestStatus s[8];
		TInt count;
		for (count = 0; count < CPUs + 1; count++)
			{
			TInt r = t[count].Create(KNullDesC, &TestThreadMigration, 0x1000, NULL, (TAny*)(inc++));
			test(r==KErrNone);
			t[count].Logon(s[count]);
			}
		for (count = 0; count < CPUs + 1; count++)
			{
			t[count].Resume();
			}
		User::After(10*1000*1000);
		for (count = 0; count < CPUs + 1; count++)
			{
			t[count].Kill(0);
			}
		for (count = 0; count < CPUs + 1; count++)
			{
			User::WaitForAnyRequest();
			}
		for (count = 0; count < CPUs + 1; count++)
			{
			TInt xt = t[count].ExitType();
			TInt xr = t[count].ExitReason();
			test(xt == EExitKill && xr == KErrNone);
			}
		for (count = 0; count < CPUs + 1; count++)
			{
			CLOSE_AND_WAIT(t[count]);
			}
		}

	test.End();
	return 0;
	}
