// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\clean_prepdc.cpp
// This test call all the other datacaging tests so that the relevant configuration and cleanup 
// for the test may be carried out.
// 
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <hal.h>
#include "t_server.h"

GLDEF_D RTest test(_L("clean_prepdc"));

//_LIT(KResourcePath, "?:\\Resource\\");
#ifndef __WINS__
//_LIT(KSystemPath,	"?:\\System\\");
#else
//_LIT(KSystemPath,	"?:\\Sys\\");
#endif
//_LIT(KPrivatePath,	"?:\\Private\\");

//_LIT(Kt_dcroot, "t_dcroot");
//_LIT(Kt_dctcb, "t_dctcb");
//_LIT(Kt_dcnone, "t_dcnone");
//_LIT(Kt_dcallfiles, "t_dcallfiles");
//_LIT(Kt_dcdiskadmin, "t_dcdiskadmin");
//_LIT(Kt_dcrootallfiles, "t_dcrootallfiles");
//_LIT(Kt_dctcballfiles, "t_dctcballfiles");
//_LIT(Kt_dcrootdiskadmin, "t_dcrootdiskadmin");
//_LIT(Kt_dctcbdiskadmin, "t_dctcbdiskadmin");
//_LIT(Kt_dcdiskadminallfiles, "t_dcdiskadminallfiles");



GLDEF_C void CleanupL()
//
//Tidy up after each security test
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r=fMan->RmDir(_L("\\Resource\\"));
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	r=fMan->RmDir(_L("\\Sys\\"));
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	r=fMan->RmDir(_L("\\Private\\"));
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	delete fMan;
	}

GLDEF_C void TestSetup()
//
//creates files for each security test
//
	{
	TInt r=TheFs.MkDir(_L("\\Resource\\"));
	test_KErrNone(r);
	r=TheFs.MkDir(_L("\\Sys\\"));
	test_KErrNone(r);
	RFile f;
	r=f.Create(TheFs,_L("\\Resource\\resourcefile.txt"),EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	f.Close();
	}	

GLDEF_C void CallTestsL(/*TChar aDriveLetter*/)
//
// Calls all data caging tests after setting up the file system for them
//
	{
	TBuf<30> tmp;
	TInt r= TheFs.SessionPath(tmp);
	test_KErrNone(r);
	RDebug::Print(_L("sessp=%S"),&tmp);
	CleanupL();
	TestSetup();
	}
