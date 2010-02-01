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
// e32test\dll\t_oeexport2.cpp
// Test loader copes with already loaded OE dlls by running T_OEEXPORT after
// we have already loaded a DLL which it depends on (T_OEDLL1.DLL).
// 
//

#include <e32test.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

RTest test(_L("T_OEEXPORT2"));

TInt E32Main()
	{
	test.Title();

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Start(_L("Preload t_oedll1.dll"));
	RLibrary library;
	test(library.Load(_L("t_oedll1.dll")) == KErrNone);

	test.Next(_L("Run T_OEEXPORT.EXE..."));
	RProcess p;
	TInt r=p.Create(_L("T_OEEXPORT.EXE"), _L("2"));
	test(r==KErrNone);
	TRequestStatus s;
	p.Logon(s);
	p.Resume();
	User::WaitForRequest(s);
	TExitCategoryName aExitCategory = p.ExitCategory();
	test.Printf(_L("Second test exits with: %d,%d,%S\n"),p.ExitType(),p.ExitReason(),&aExitCategory);
	test(p.ExitType()==EExitKill);
	test(p.ExitReason()==0);
	p.Close();

	library.Close();

	test.End();
	return KErrNone;
	}
