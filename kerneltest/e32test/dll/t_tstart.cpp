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
// e32test\dll\t_tstart.cpp
// Overview:
// Check DLLs are started in correct order
// API Information:
// N/A
// Details:
// - Test and verify that DLLs are started in correct order
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("T_TSTART"));

IMPORT_C TUint32 GetStartTime1();
IMPORT_C TUint32 GetStartTime3();
IMPORT_C TUint32 GetStartTime2();

GLDEF_C TInt E32Main()
    {
	test.Title();
//
	test.Start(_L("Checking Dlls started in correct order"));

	TUint32 start1=GetStartTime1();
	TUint32 start2=GetStartTime2();
	TUint32 start3=GetStartTime3();
	test.Printf(_L("Start tickcount for Dlls T_START1, T_START2 and T_START3 are:\n"));
	test.Printf(_L("                         %08x  %08x     %08x\n"), start1,start2, start3);
	test(start2>start1);
	test(start3>start2);

	test.End();
	return 0;
    }

