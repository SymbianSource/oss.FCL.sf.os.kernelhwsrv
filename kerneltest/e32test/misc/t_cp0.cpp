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
// e32test\misc\t_cp0.cpp
// 
//

#include <e32test.h>
#include <cpudefs.h>
#include "nk_cpu.h"
#include "../misc/prbs.h"

RTest test(_L("T_CP0"));

extern void GetAcc0(TInt64&);
extern void SetAcc0(const TInt64&);
extern void InnerProduct(TInt64& /*aResult*/, const TInt16* /*aVec1*/, const TInt16* /*aVec2*/, TInt /*aLength*/);
extern void InnerProduct2(TInt64& /*aResult*/, const TInt16* /*aVec1*/, const TInt16* /*aVec2*/, TInt /*aLength*/);


TInt DoCP0Test(TAny* aPtr)
	{
	TUint seed[2];
	seed[0]=(TUint)aPtr;
	seed[1]=0;
	TInt16 vec1[128];
	TInt16 vec2[128];
	TInt run;
	for (run=0; run<100000; ++run)
		{
		TInt n=(Random(seed)&63)+64;		// vector length
		TInt i;
		for (i=0; i<n; ++i)
			{
			vec1[i]=(TInt16)(Random(seed)&0xffff);
			vec2[i]=(TInt16)(Random(seed)&0xffff);
			TInt64 result, result2;
			InnerProduct(result,vec1,vec2,n);
			InnerProduct2(result2,vec1,vec2,n);
			if (result != result2)
				{
				User::Panic(_L("ERROR"),run);
				}
			}
		}
	return 0;
	}

void CheckExit(TInt aThreadNum, RThread aThread)
	{
	TInt exitType=aThread.ExitType();
	TInt exitReason=aThread.ExitReason();
	TBuf<32> exitCat=aThread.ExitCategory();
	test.Printf(_L("Thread %d: %d,%d,%S\n"),aThreadNum,exitType,exitReason,&exitCat);
	test(exitType==EExitKill);
	test(exitReason==KErrNone);
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing XScale DSP Coprocessor"));

	TInt64 acc0;
	GetAcc0(acc0);
	test.Printf(_L("acc0=%lx\n"),acc0);

	SetAcc0(0);
	GetAcc0(acc0);
	test.Printf(_L("acc0=%lx\n"),acc0);

	test.Next(_L("Test CP0 in single thread"));
	DoCP0Test((TAny*)487);

	test.Next(_L("Test CP0 in multiple threads"));
	RThread t1, t2;
	TRequestStatus s1, s2;
	TInt r=t1.Create(KNullDesC(),DoCP0Test,0x1000,NULL,(TAny*)0xddb3d743);
	test(r==KErrNone);
	r=t2.Create(KNullDesC(),DoCP0Test,0x1000,NULL,(TAny*)0xb504f334);
	test(r==KErrNone);
	t1.Logon(s1);
	t2.Logon(s2);
	t1.Resume();
	t2.Resume();
	User::WaitForRequest(s1);
	User::WaitForRequest(s2);
	CheckExit(1,t1);
	CheckExit(2,t2);
	CLOSE_AND_WAIT(t1);
	CLOSE_AND_WAIT(t2);

	test.End();
	return 0;
	}

