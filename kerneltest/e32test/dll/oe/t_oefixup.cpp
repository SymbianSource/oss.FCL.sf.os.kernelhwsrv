// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_oeexport.cpp
// Overview:
// REMOVE_ME - Dummy test case to invoke t_oeexport and t_oeexport1 test cases
// Should be removed once tools have been updated and t_oeexport etc can be built
// API Information:
// Details:
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32panic.h>

RTest test(_L("T_OEFIXUP"));


TInt E32Main()
	{
	test.Title();

	RProcess process;
	TRequestStatus req;

	test.Start(_L("Test invoke t_oeexport"));
	test (process.Create(_L("t_oeexport.exe"), _L("x")) == KErrNone);
	process.Logon(req);
	process.Resume();
	User::WaitForRequest(req);
	test(process.ExitReason() == KErrNone);
	test(process.ExitType() != EExitPanic);
	process.Close();

	test.Next(_L("Test invoke t_oeexport1"));
	test (process.Create(_L("t_oeexport1.exe"), _L("x")) == KErrNone);
	process.Logon(req);
	process.Resume();
	User::WaitForRequest(req);
	test(process.ExitReason() == KErrNone);
	test(process.ExitType() != EExitPanic);
	process.Close();
	test.End();
	return KErrNone;
	}
