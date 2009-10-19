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
// e32\euser\epoc\win32\uc_stub.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32wins.h>

extern "C"
GLDEF_C TInt _E32Startup()
//
// Unused in the stub
//
	{
	return KErrNone;
	}

GLDEF_C void __stdcall _E32Bootstrap()
//
// stub for bootstrapping EPOC
//
	{
	BootEpoc(EFalse);
	}

