// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\T_TRACEREDIRECT.cpp
// Print out RDebug::Prints captured by the 'trace redirection' LDD
// 
//

#include <e32cmn.h>
#include <e32cmn_private.h>
#include "d_traceredirect.h"
#include <e32debug.h>
#include <e32test.h>

RTest test(_L("T_TRACEREDIRECT"));

RTraceRedirect ldd;


TInt noisythreadfn(TAny *)
	{

	for (TInt i=0; i<12; i++)
		{

		User::After(700000); // 2+1/2 a second
		RDebug::Print(_L("Noisy thread 1 @ %lx"), User::TickCount());
		}
	RDebug::Print(_L("END"));
	return KErrNone;
	}

TInt noisythreadfn2(TAny *)
	{

	for (TInt i=0; i<20; i++)
		{

		User::After(300000); // 2+1/2 a second
		RDebug::Print(_L("Noisy thread 2 @ %lx"), User::TickCount());
		}
	return KErrNone;
	}

GLDEF_C TInt E32Main()
    {
	test.Title();
	TInt r;
	
	test.Start(_L("Loading LDD"));
	r = User::LoadLogicalDevice(_L("D_TRACEREDIRECT"));
	test(r==KErrNone || r==KErrAlreadyExists);

	TBool logged=test.Logged();
	test.SetLogged(EFalse);
	test.Next(_L("Open channel to LDD"));
	r = ldd.Open();
	test(r==KErrNone);


	test.Next(_L("Setup noisy thread"));
	RThread thread;
	TRequestStatus threadstat;
	r=thread.Create(KNullDesC,noisythreadfn,KDefaultStackSize,&User::Allocator(),NULL);
	test(r==KErrNone);
	thread.Logon(threadstat);
	thread.Resume();
	RThread thread2;
	TRequestStatus threadstat2;
	r=thread2.Create(KNullDesC,noisythreadfn2,KDefaultStackSize,&User::Allocator(),NULL);
	test(r==KErrNone);
	thread2.Logon(threadstat2);
	thread2.Resume();

	test.Next(_L("Capture RDebug::Prints until 'END' is captured"));
	TBuf8<256> buf;
	TBuf16<256> buf16;
	FOREVER
		{
		test(ldd.NextTrace(buf) == KErrNone);
		buf16.Copy(buf);
		if (buf16!=_L(""))
			test.Printf(_L("Captured: '%S'\n"),&buf16);
		if (buf16==_L("END"))
			break;
		buf=_L8("");
		User::After(250000); // poll every half a second
		}
	test.SetLogged(logged);

	test.Next(_L("Wait for noisy thread to die"));
	User::WaitForRequest(threadstat);
	User::WaitForRequest(threadstat2);
	test(threadstat==KErrNone);

	test.Next(_L("Closing ldd"));
	ldd.Close();
	
	test.End();

	return(0);
    }

