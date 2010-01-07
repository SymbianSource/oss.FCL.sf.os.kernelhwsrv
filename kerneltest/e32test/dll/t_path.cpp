// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_path.cpp
// Overview:
// Test imported DLL path search order.
// API Information:
// None (implicit searching when loading an executable)
// Details:
// - t_pathdll1 and t_pathdll2 both export the same function which returns 1
// 2 respectively.
// - Run t_path2 (linked to t_pathdll1) and check return value from DLL - should
// be 1.
// - Copy t_pathdll2 to C: and rename to t_pathdll1 - it should now be loaded 
// instead.
// - Run t_path2 again and check return value from DLL - should be 2.
// Platforms/Drives/Compatibility:
// Not on emulator.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

RTest test(_L("T_PATH"));
RFs	gFs;
CFileMan* gFileMan;

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test imported DLL search path order"));

	if (!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		{
		test.Printf(_L("Skipping test as sysbin enforcement is disabled"));
		return KErrNone;
		}

	// Turn off evil lazy dll unloading
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();

	CTrapCleanup* ct = CTrapCleanup::New();
	test_NotNull(ct);
	test_KErrNone(gFs.Connect());
	TRAPD(r, gFileMan=CFileMan::NewL(gFs));
	test_KErrNone(r);

	test.Next(_L("Run t_path2, expecting DLL 1 to be loaded"));
	_LIT(KPath2, "t_path2");
	RProcess path2;
	test_KErrNone(path2.Create(KPath2, KNullDesC));
	TRequestStatus stat;
	path2.Logon(stat);
	path2.Resume();
	User::WaitForRequest(stat);
	test_Equal(EExitKill, path2.ExitType());
	test_Equal(1, path2.ExitReason());
	path2.Close();

	test.Next(_L("Copy DLL 2 to C drive"));
	_LIT(KCopySrc, "Z:\\sys\\bin\\t_pathdll2.dll");
	_LIT(KCopyDest, "C:\\sys\\bin\\");
	test_KErrNone(gFileMan->Copy(KCopySrc(), KCopyDest(), CFileMan::ERecurse));
	_LIT(KRenSrc, "C:\\sys\\bin\\t_pathdll2.dll");
	_LIT(KRenDest, "C:\\sys\\bin\\t_pathdll1.dll");
	test_KErrNone(gFileMan->Rename(KRenSrc(), KRenDest()));

	test.Next(_L("Run t_path2, expecting DLL 2 to be loaded"));
	test_KErrNone(path2.Create(KPath2, KNullDesC));
	path2.Logon(stat);
	path2.Resume();
	User::WaitForRequest(stat);
	test_Equal(EExitKill, path2.ExitType());
	test_Equal(2, path2.ExitReason());
	path2.Close();

	// cleanup
	gFileMan->Delete(KRenDest);
	delete gFileMan;
	gFs.Close();
	delete ct;
	test.End();
	return KErrNone;
	}

