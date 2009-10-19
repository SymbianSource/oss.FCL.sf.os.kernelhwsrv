// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_oeexport.cpp
// Overview:
// Tests that getting 0th ordinal from non-stdexe returns NULL
// API Information:
// RProcess
// Details:
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>

RTest test(_L("T_OEEXPORT1"));

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Test retrieving 0th ordinal from non-stdexe returns NULL"));
	test(RProcess::ExeExportData()==NULL);

	test.End();
	return KErrNone;
	}
