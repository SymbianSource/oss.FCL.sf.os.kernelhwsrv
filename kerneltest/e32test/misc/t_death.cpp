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
// e32test\misc\t_death.cpp
// 
//

#include <e32svr.h>

GLDEF_C TInt E32Main()
	{
	RThread().SetPriority(EPriorityAbsoluteHigh);
	RUndertaker u;
	TInt r=u.Create();
	if (r!=KErrNone)
		User::Panic(_L("T_DEATH 0"),r);
//	RThread().SetSystem(ETrue);
	FOREVER
		{
		TRequestStatus s;
		TInt h;
		u.Logon(s,h);
		User::WaitForRequest(s);
		RThread t;
		t.SetHandle(h);
		TFullName fn=t.FullName();
		TExitType exit=t.ExitType();
		TBuf<32> exitCat;
		exitCat=t.ExitCategory();
		TInt exitReason=t.ExitReason();
		RDebug::Print(_L("Thread %S Exited %d,%d,%S"),&fn,exit,exitReason,&exitCat);
//		if (exit==EExitPanic)
//			{
//			*(TInt*)0xdead0000=0;
//			}
		t.Close();
		}
	}
