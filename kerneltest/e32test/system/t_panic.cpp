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
// e32test\system\t_panic.cpp
// 
//

#include <e32test.h>

RTest test(_L("T_PANIC"));

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Waiting..."));

	RUndertaker u;
	TInt r=u.Create();
	test(r==KErrNone);
	//to avoid RVCT4 warning of unreachable statement.
	volatile TInt forever = 0;
	while(forever)
		{
		TInt h;
		TRequestStatus s;
		r=u.Logon(s,h);
		test(r==KErrNone);
		User::WaitForRequest(s);
		RThread t;
		t.SetHandle(h);
		TBuf8<128> b;
		t.Context(b);
		TInt *pR=(TInt*)b.Ptr();
		TFullName tFullName = t.FullName();
		TExitCategoryName tExitCategory = t.ExitCategory();
		test.Printf(_L("Thread %S Exit %d %S %d\n"),&tFullName,t.ExitType(),&tExitCategory,t.ExitReason());
		test.Printf(_L("r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n"),pR[0],pR[1],pR[2],pR[3]);
		test.Printf(_L("r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n"),pR[4],pR[5],pR[6],pR[7]);
		test.Printf(_L("r8 =%08x r9 =%08x r10=%08x r11=%08x\n"),pR[8],pR[9],pR[10],pR[11]);
		test.Printf(_L("r12=%08x r13=%08x r14=%08x r15=%08x\n"),pR[12],pR[13],pR[14],pR[15]);
		test.Printf(_L("cps=%08x dac=%08x\n"),pR[16],pR[17]);
		t.Close();
		}
	return 0;
	}
