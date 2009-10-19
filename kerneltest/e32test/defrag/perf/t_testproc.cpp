// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/defrag/perf/t_testproc.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>

#include "..\d_pagemove.h"
#include "t_perf.h"
#include "t_testdll.h"


LOCAL_D RTest test(_L("defragtest proc"));

GLDEF_C TInt E32Main()
	{
	
	/* Map the DLL to this process and Run the tests */
	DllDefrag dll;
	TEST_PRINTF(_L("Test Process: Attempting to Load DLL\n"));
	TInt ret = dll.LoadTheLib(0, test);
	if (ret != KErrNone) 
		{
		test.Printf(_L("A Process failed to load the DLL\n"));
		return ret;
		}

	//TODO: Touch all the DLL pages !
	RProcess::Rendezvous(KErrNone);
	for (;;)
		{
		User::AfterHighRes(10000);
		}
	}


