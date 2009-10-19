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
// e32test\dll\t_loadfail.cpp
// Overview:
// Tests failure codes when loading a RProcess or RLibrary.
// Introduced as test for defect DEF092502
// API Information:
// RProcess, RLibrary
// Details:
// - Test attempting to open a non-existent DLL fails gracefully
// - Test attempting to open a DLL of an invalid name fails gracefully
// - Test attempting to open a non-existent EXE fails gracefully
// - Test attempting to open an EXE of an invalid name fails gracefully
// - Test loading a dll with an incorrect version number as part of it name
// will return KErrCorupt
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Assumes drive 'z' always contains \img\t_ver1{00010001}.dll
// Assumes drive 'c' is writeable
// Failures and causes:
// Base Port information:
// Error codes returned on h/w and emulator should be the same
// 
//

#include <e32test.h>
#include <f32file.h>

RTest test(_L("T_LOADFAIL"));

// Test loading dll with incorrect version number in the name returns KErrCorrupt
// as the loader will compare the image header version number against that in the
// image file's name
LOCAL_D TInt TestVersionL()
	{
// Test only valid on target h/w as emulator uses MS Windows loader which doesn't allow verison 
// no. to be in part of the dll name.
#ifndef __WINS__
	RFs rfs;
	rfs.Connect();
	CFileMan *fileman = CFileMan::NewL(rfs);
	_LIT(KCorrectDll,"z:\\img\\t_ver1{00010001}.dll");
	_LIT(KWrongDll,"c:\\sys\\bin\\t_ver1{00010011}.dll");
	test(KErrNone == fileman->Copy(KCorrectDll,KWrongDll,CFileMan::ERecurse));
	RLibrary lib;
	test(KErrCorrupt == lib.Load(KWrongDll));
	delete fileman;
	rfs.Close();
#endif
	return KErrNone;
	}

TInt E32Main()
	{
	test.Title();

	test.Printf(_L("Test failure modes/codes when loading bad & absent libraries and processes\n"));
	
	test.Start(_L("Test DLL loading fails gracefully"));
	RLibrary library;
	
	// test non-existent DLL loading
	test(library.Load(_L("t_asdasdsadsasd.dll"))==KErrNotFound);

	// test invalid name loading
	test(library.Load(_L("z:\\sys\\bin\\")) == KErrNotFound);
	test(library.Load(_L("\\sys\\bin\\")) == KErrNotFound);
	test(library.Load(_L("..")) == KErrNotFound);
	test(library.Load(_L(".")) == KErrNotFound);
	TInt r = library.Load(_L("\\."));
	test(r == KErrNotFound || r == KErrAccessDenied);
	test(library.Load(_L(".\\")) == KErrNotFound);
	test(library.Load(_L("\\")) == KErrNotFound);
	test(library.Load(_L("")) == KErrNotFound);

	// test loading from odd/bad paths
	test(library.Load(_L("t_foo.dll"), _L("..")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L(".")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L("\\;")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L(";\\")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L(";")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L("\\")) == KErrNotFound);
	test(library.Load(_L("t_foo.dll"), _L("")) == KErrNotFound);

	// test loading a too long name fails
	TBufC16<KMaxFileName+1> buf;
	TPtr16 ptr = buf.Des();
	ptr.SetLength(KMaxFileName+1);
	test(library.Load(ptr) == KErrBadName);
	
	test.Next(_L("Test EXE loading fails gracefully"));
	RProcess process;
	
	//  test non-existent EXE loading
	test(process.Create(_L("t_safcxvxcvsw.exe"), _L("x")) == KErrNotFound);
	
	
	// test invalid name loading
	test(process.Create(_L("z:\\sys\\bin\\"),_L("x")) == KErrNotFound);
	test(process.Create(_L("\\sys\\bin\\"),_L("x")) == KErrNotFound);
	test(process.Create(_L(".."), _L("x")) == KErrNotFound);
	test(process.Create(_L("."), _L("x")) == KErrNotFound);
	r = process.Create(_L("\\."), _L("x"));
	test(r == KErrNotFound || r == KErrAccessDenied);
	test(process.Create(_L(".\\"), _L("x")) == KErrNotFound);
	test(process.Create(_L("\\"), _L("x")) == KErrNotFound);
	test(process.Create(_L(""), _L("x")) == KErrNotFound);
	
	// test loading a too long name fails
	test(process.Create(ptr, _L("x")) == KErrBadName);
	
	test.Next(_L("Test loading dll with incorrect verison number as part of its name"));
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();
	TRAPD(ret,TestVersionL());
	test(ret==KErrNone);
	delete cleanup;
	__UHEAP_MARKEND;
	test.End();

	return KErrNone;
	}
