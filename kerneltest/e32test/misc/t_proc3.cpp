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
// e32test\misc\t_proc3.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

_LIT(KSecondProcessName,"T_PROC3A");

RTest test(_L("T_PROC3"));

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing thread create/process kill"));
	TUint seed[2];
	seed[0]=0xc90fdaa2;
	seed[1]=0;

	TInt n=0;
	FOREVER
		{
		TUint x=Random(seed);
		TBuf<16> buf;
		buf.Num(x,EHex);
		RProcess p;
		TInt r=p.Create(KSecondProcessName,buf);
		if (r!=KErrNone)
			{
			test.Printf(_L("Process create failed, code %d\n"),r);
			test.Getch();
			test(0);
			}
		TRequestStatus s;
		p.Logon(s);
		test(s==KRequestPending);
		++n;
		p.Resume();
		User::WaitForRequest(s);
		if (p.ExitType()!=EExitKill || (TUint)p.ExitReason()!=x)
			{
			TExitCategoryName aExitCategory = p.ExitCategory();
			test.Printf(_L("Exit      %d,%d,%S\n"),p.ExitType(),p.ExitReason(),&aExitCategory);
			test.Printf(_L("Should be 0,%d,Kill\n"),x);
			test.Getch();
			test(0);
			}
		p.Close();
		test.Printf(_L("%d\n"),n);
		}

//	test.End();
	}
