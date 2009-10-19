// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_add2p.cpp
// Overview:
// Test RProcess creation, notification.
// API Information:
// RProcess.	
// Details:
// - Start a new process with specified name, set specified priority to it, 
// request notification when this process dies, make the main thread 
// in the process eligible for execution, kill the process and check 
// notification is as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

RTest test(_L("T_ADD2P"));

_LIT(KLitExe2Name, "T_ADD2P2");

GLDEF_C TInt E32Main()
	{


	if (User::CommandLineLength())
		return 0;
	test.Title();
	RProcess p;
	TInt r=p.Create(KLitExe2Name, KLitExe2Name);
	test.Printf(_L("Returns %d\n"),r);
	test(r==KErrNone);
	p.SetPriority(EPriorityBackground);
	TRequestStatus s;
	p.Logon(s);
	p.Resume();
	User::AfterHighRes(2000);
	p.Kill(1);
	p.Close();
	User::WaitForRequest(s);
	test.Printf(_L("Exit code %d\n"),s.Int());
	return 0;
	}
