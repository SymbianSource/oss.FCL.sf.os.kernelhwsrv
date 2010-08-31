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
// f32test\server\t_fname.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32dbg.h>
#include <e32debug.h>


LOCAL_D RTest test(_L("T_FNAME"));

GLDEF_C TInt E32Main(void)
    {

	test.Title();
	test.Start(_L("Test Dll::FileName"));

	TFileName xfn(RProcess().FileName());
	TFileName path = PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("Z:\\sys\\bin\\"):_L("Z:\\system\\bin\\");
	path[0] = xfn[0];
	RLibrary lib;
	TInt r=lib.Load(_L("t_start1.dll"), path);
	test_KErrNone(r);
	TFileName name;
	name=lib.FileName();
	path += _L("t_start1.dll");
	test(name==path);
	lib.Close();


	test.End();
	return(KErrNone);
    }

