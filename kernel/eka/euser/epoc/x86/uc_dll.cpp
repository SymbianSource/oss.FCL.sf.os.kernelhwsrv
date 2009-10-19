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
// e32\euser\epoc\x86\uc_dll.cpp
// This file contains the DLL entrypoint
// 
//

#include "u32std.h"

// include the static data definitions
#define __FLTUSED
#include "win32crt.h"
#include "nwdl.h"


extern "C" {

TInt _E32Dll_Body(TInt aReason)
	{
	if (aReason==KModuleEntryReasonProcessAttach)
		constructStatics();
	else if (aReason==KModuleEntryReasonProcessDetach)
		destroyStatics();
	return KErrNone;
	}

void _fltused() {}
}
