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
// e32test\system\t_mstim2.cpp
// 
//

#include <e32test.h>
#include <e32uid.h>
#include "d_mstim.h"

RTest test(_L("T_MSTIM2"));
RMsTim mstim;

TBool PauseOnError = 0;
#define GETCH()		(PauseOnError&&test.Getch())

#define TEST(c)		((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),GETCH(),test(0),0)))
#define CHECK(c)	((void)(((c)==0)||(test.Printf(_L("Error %d at line %d\n"),(c),__LINE__),GETCH(),test(0),0)))

const TPtrC KLddFileName=_L("D_MSTIM.LDD");

GLDEF_C TInt E32Main()
//
// Test millisecond timers
//
    {
//	test.SetLogged(EFalse);
	test.Title();

	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	TEST(r==KErrNone || r==KErrAlreadyExists);
	
	r=mstim.Open();
	CHECK(r);

	test.Next(_L("Repeated long period timer"));
	TBool exit=EFalse;
	while(!exit)
		{
		TRequestStatus ts;
		TRequestStatus cs;
		CConsoleBase* console=test.Console();
		console->Read(cs);
		mstim.StartOneShotInt(ts,0,30000);
		FOREVER
			{
			User::WaitForRequest(cs,ts);
			if (ts!=KRequestPending)
				{
				console->ReadCancel();
				break;
				}
			if (cs!=KRequestPending)
				{
				TKeyCode k=console->KeyCode();
				if (k==EKeyEscape)
					exit=ETrue;
				else
					console->Read(cs);
				}
			}
		SMsTimerInfo info;
		r=mstim.GetInfo(0,info);
		CHECK(r);
		TEST(info.iCount==1);
		test.Printf(_L("30s timer took %dus\n"),info.iMin);
		}
	mstim.Close();
	test.End();
	return(KErrNone);
    }

