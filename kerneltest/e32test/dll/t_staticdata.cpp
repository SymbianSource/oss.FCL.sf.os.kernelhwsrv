// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_staticdata.cpp
// Overview:
// Test static data is initialised correctly
// API Information:
// n/a
// Details:
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>


const TUint32 KTestValue = 0x12345678;
TInt E32Main();

// initialised static data...
TUint32  Data[4]		= {0,KTestValue,~KTestValue,0xffffffffu};
TInt   (*CodePointer)()	= &E32Main;
TUint32* DataPointer	= Data;
TUint32  Bss[4]			= {0};


TInt E32Main()
	{
	RTest test(_L("T_STATICDATA"));
	test.Title();

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Start(_L("Test static data in EXEs"));

	test_Equal(0,Data[0]);
	test_Equal(KTestValue,Data[1]);
	test_Equal(~KTestValue,Data[2]);
	test_Equal(0xffffffffu,Data[3]);

	test_Equal(&E32Main,CodePointer);

	test_Equal(Data,DataPointer);

	TInt i;
	for(i=0; i<TInt(sizeof(Bss)/sizeof(Bss[0])); ++i)
		test_Equal(0,Bss[i]);

	// check a second concurrent process also works...
	if(User::CommandLineLength()==0)
		{
		test.Next(_L("Test static data in second EXE instance"));
		RProcess p;
		test_KErrNone(p.Create(p.FileName(),_L("2")));
		TRequestStatus s;
		p.Logon(s);
		p.Resume();
		User::WaitForRequest(s);
		test_Equal(0,p.ExitReason());
		test_Equal(EExitKill,p.ExitType());
		}
	test.End();
	return(KErrNone);
	}

