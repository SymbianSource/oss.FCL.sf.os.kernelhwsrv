// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides some configurable constants used by all the tests
// 
//

#ifndef user_config_h
#define user_config_h

#include <f32file.h>

//
// Need to provide drive number and Flash address.
//	KDriveNumber is the TBusLocalDrive number that the media driver mounts on.
//	This will match the Variant::DeviceInfoFromDrive function.
//	For example if your LFFS drive mounts on EFixedMedia0 and DeviceInfoFromDrive
//	returns EFixedMedia0 for drive number 1, then KDriveNumber must be 1.
//
//	KFlashPhysicalAddress is the physical address of the start of the
//	LFFS area in Flash. This is used to map a chunk that the test apps
//	can use to read directly from Flash.
//

//#error You must edit user_config.h to provide driver number and flash address
#ifdef __WINS__
const TInt KDriveNumber = 8;
const TChar KLffsLogicalDriveNumber = EDriveW;
#else
#if 0
const TInt KDriveNumber = 0;
const TChar KLffsLogicalDriveNumber = EDriveC;
#else
// XXX TONYL: Unmounting C wouldn't work for me - fault a KERN FAULT 4
const TChar KLffsLogicalDriveNumber = EDriveK;
const TInt KDriveNumber = 8;
#endif
#endif

// Unmount drive before starting test - reccommended
#define UNMOUNT_DRIVE

// Don't attempt to load the LFFS driver - define this if the driver is already loaded
#define SKIP_PDD_LOAD

_LIT( KLfsDriverName, "MEDLFS" );

#endif
