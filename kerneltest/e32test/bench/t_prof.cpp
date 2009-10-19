// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_prof.cpp
// 
//

#include <e32test.h>
#include "d_prof.h"

RTest test(_L("T_PROFILE"));
RProfile Profile;

LOCAL_C void ProfileAllThreads()
	{
	TFindThread ft(_L("*"));
	TFullName fullname;
	test.Console()->ClearScreen();
	FOREVER
		{
		TInt r=ft.Next(fullname);
		if (r!=KErrNone)
			break;
		RThread t;
		r=t.Open(ft);
		if (r==KErrNone)
			{
			TProfileData data;
			r=Profile.Read(t,data);
			if (r==KErrNone)
				{
				while(fullname.Length()<40)
					fullname.Append(TChar(' '));
				test.Printf(_L("%S T=%9d C=%9d Y=%9d\n"),
					&fullname,data.iTotalCpuTime,data.iMaxContinuousCpuTime,data.iMaxTimeBeforeYield);
				}
			t.Close();
			}
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	TInt r=User::LoadLogicalDevice(_L("D_PROF"));
	if (r!=KErrNone && r!=KErrAlreadyExists)
		User::Panic(_L("T_PROF0"),r);
	r=Profile.Open();
	if (r!=KErrNone)
		User::Panic(_L("T_PROF1"),r);
	FOREVER
		{
		TKeyCode key=test.Getch();
		if (key==TKeyCode('r'))
			{
			Profile.Reset();
			}
		else if (key==TKeyCode('p'))
			{
			ProfileAllThreads();
			}
		else if (key==TKeyCode('x'))
			break;
		}
	Profile.Close();
	User::FreeLogicalDevice(_L("Profile"));
	return KErrNone;
	}
