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
// e32\kernel\x86\d_entry.cpp
// 
//

#include <kernel/kernel.h>

// include the static data definitions
#include "win32crt.h"
#include "nwdl.h"


GLREF_C TInt KernelModuleEntry(TInt);


extern "C" {

TInt _E32Dll_Body(TInt aReason)
//
// EPOC Dll entrypoint for device drivers
//
	{
	if (aReason==KModuleEntryReasonProcessDetach)
		{
		destroyStatics();
		return KErrNone;
		}
	if (aReason==KModuleEntryReasonExtensionInit1 || aReason==KModuleEntryReasonProcessAttach)
		constructStatics();
	return KernelModuleEntry(aReason);
	}
}
