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
// e32test\misc\t_busy.cpp
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("BUSY"));

LOCAL_D TInt Counter=0;

GLDEF_C TInt E32Main()
	{
	test.Title();
	RThread().SetPriority(EPriorityAbsoluteHigh);
	CConsoleBase* console=test.Console();
	TRequestStatus ks;
	console->Read(ks);
	TInt period=1000;
	test.Printf(_L("\nPeriod %dus"),period);
	FOREVER
		{
		User::AfterHighRes(period);
		Counter+=period;
		if (Counter>=1000000)
			{
			Counter-=1000000;
			test.Printf(_L("."));
			}
		if (ks!=KRequestPending)
			{
			TInt k=(TInt)console->KeyCode();
			if (k==EKeyEscape)
				break;
			if (k>='0' && k<='9')
				{
				TInt n=k-'0';
				if (n==0)
					n=10;
				period=n*1000;
				test.Printf(_L("\nPeriod %dus"),period);
				}
			console->Read(ks);
			}
		}

	return 0;
	}
