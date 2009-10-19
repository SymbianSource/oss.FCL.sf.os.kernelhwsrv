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
// e32test\misc\t_empty2.cpp
// Overview:
// Does almost nothing!
// API Information:
// None
// Details:
// Does almost nothing -- call: RDebug::RawPrint(_L("E32Main entered\r\n")) 
// and exit.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32svr.h>
#include "../dll/t_dll.h"

// Dummy function to make codewarrior keep T_DLL1 in the import list
#ifdef __CW32__
static void Dummy()
	{
	TestDll1::Data();
	}
#endif

#ifdef __VC32__
TAny* User::Alloc(TInt)
	{
	return NULL;
	}

void User::Free(TAny*)
	{
	}
#endif

GLDEF_C TInt E32Main()
	{
	RDebug::RawPrint(_L("E32Main entered\r\n"));
	return 0;
	}
