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
// e32test\misc\freemem.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>

GLDEF_C TInt E32Main()
	{
	RTest test(_L("Available memory"));
	test.Title();

	TMemoryInfoV1 mem;
	TPckg<TMemoryInfoV1> memPckg(mem);

	TKeyCode k=EKeyEnter;
	while (k!='x' && k!='X')
		{
		TInt r=UserHal::MemoryInfo(memPckg);
		test(r==KErrNone);
		test.Printf(_L("Free RAM %d\n"),mem.iFreeRamInBytes);
		k=test.Getch();
		}

	test.Close();
	return 0;
	}
