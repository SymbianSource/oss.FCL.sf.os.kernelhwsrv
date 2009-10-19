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
// invoke the USB mass storage application
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

GLDEF_C TBool StartUSBMS()
	{
	RFs fs;
	TInt r = fs.Connect();
	if (r != KErrNone)
		{
		// BOOTFAULT
		RDebug::Print(_L("FAULT: Connecting RFs returned %d\r\n"), r);
		BOOT_FAULT();
		}

	TInt drive;
	RFs::CharToDrive('D', drive);		// XXX variant constant drivepath

	TDriveInfo info;
	r = fs.Drive(info, drive);
	if (r != KErrNone)
		{
		// BOOTFAULT
		RDebug::Print(_L("FAULT: Calling Drive() on RFs returned %d\r\n"), r);
		BOOT_FAULT();
		}

	if (info.iType == EMediaNotPresent)
		{
		return EFalse;
		}

	LoadDevice = ELoadUSBMS;
	WriteConfig();

	RProcess proc;
	TName command = _L("D");
	r = proc.Create(_L("z:\\sys\\bin\\usbboot.exe"), command);
	if (r != KErrNone)
		{
		// BOOTFAULT
		RDebug::Print(_L("FAULT: error starting usbboot %d\r\n"), r);
		BOOT_FAULT();
		}
	proc.Resume();
	return ETrue;
	}

GLDEF_C void TryUSBMS()
	{
	// Check first whether this boot is intended to load and boot an image from
	// the media.
	if (LoadDevice == EBootUSBMS)
		{
		PrintToScreen(_L("USB-MS boot scanning drives\r\n"));
		// search drives for file - returns true if an image has been found
		if (SearchDrives())
			DoDownload();
		}

	PrintToScreen(_L("Starting USB Mass Storage\r\n"));
	DisableMenu();
	if(StartUSBMS())
		{
		// USB Mass Storage boot has started - don't continue with the
		// bootloader, sleep here.
		while(1)
			User::After(10000000);
		}
	else
		{
		EnableMenu();
		// Not started (probably no card in drive) revert to normal bootloader
		// mode and notify the variant to rewrite it's configuration.
		PrintToScreen(_L("NO VALID MEDIA\r\n"));
		PrintToScreen(_L("Leaving USB Mass Storage mode\r\n"));
		LoadDevice = ELoadDrive;
		WriteConfig();
		}

	return;
	}
