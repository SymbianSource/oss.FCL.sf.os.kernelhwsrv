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
// e32test\misc\systemize.cpp
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("Systemize"));

GLDEF_C TInt E32Main()
	{
	test.Title();
	TBuf<256> cmd;
	TFullName fn;
	User::CommandLine(cmd);
	TFindThread ft(cmd);
	while (ft.Next(fn)==KErrNone)
		{
		test.Printf(_L("Systemizing %S\n"),&fn);
		RThread t;
		TInt r=t.Open(ft);
		if (r==KErrNone)
			{
			//	FIXME: Need device driver to do this
//			t.SetSystem(ETrue);
			t.Close();
			}
		}
	return 0;
	}
