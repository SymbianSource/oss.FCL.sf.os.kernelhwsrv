// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// restart the board via an appropriate method either back into the bootloader
// or to boot an image
// 
//

/**
 @file
*/

#include <e32const.h>
#include <e32const_private.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32cons.h>
#include <f32file.h>
#include <hal.h>
#include <u32hal.h>
#include "bootloader_variantconfig.h"
#include <nkern/nk_trace.h>
#include <e32twin.h>

#define FILE_ID	0x594D555D
#include "bootldr.h"
#include "ubootldrldd.h"

GLDEF_C void Restart(TInt aCode)
	{
	TInt r = User::LoadLogicalDevice(KBootldrLddName);

	RUBootldrLdd Ldd;
	r = Ldd.Open();
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to LDD open\r\n"));
			BOOT_FAULT();
			}
	if (aCode)
		Ldd.Reboot(aCode);

	Ldd.Reboot(KtRestartReasonHardRestart);
	}
