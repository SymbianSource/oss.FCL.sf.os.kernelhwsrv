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
// e32\kernel\x86\v_entry.cpp
// 
//

#include <kernel/kernel.h>

// include the static data definitions
#include "win32crt.h"
#include "nwdl.h"

IMPORT_C void AsicInitialise();

extern "C" {
TInt __Variant_Flags__ = 0;

int atexit(void (__cdecl *)(void))
	{
	return 0;
	}

TInt _E32Dll_Body(TInt aReason)
//
// EPOC Dll entrypoint for variant
//
	{
	if (aReason==KModuleEntryReasonVariantInit0)
		{
		constructStatics();
		AsicInitialise();
		return __Variant_Flags__;
		}
	return KErrGeneral;
	}
}
