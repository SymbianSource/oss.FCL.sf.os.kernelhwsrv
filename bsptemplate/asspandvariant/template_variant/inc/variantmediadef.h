// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\inc\variantmediadef.h
// Media definitions for Template variant.
// Each Media Driver requires the following definitions
// DRIVECOUNT - The total number of local drive object to be assigned to the Media Driver (1-KMaxLocalDrives)
// DRIVELIST - A list of the local drive numbers (each separated with a comma) to be assigned to the Media Driver.
// Each in the range 0 - (KMaxLocalDrives-1). Total number of drive numbers must equal the value
// of DRIVECOUNT.
// NUMMEDIA - The total number of media objects to be assigned to the Media Driver.
// DRIVENAME - A name for the drive group.
// For the complete set of media definitions
// - The total number of local drive objects assigned should not exceed KMaxLocalDrives.
// - Each Media Driver should be assigned a unique set of drive numbers - no conflicts between Media Drivers.
// - The total number of media objects assigned should not exceed KMaxLocalDrives.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//
 
#ifndef __VARIANTMEDIADEF_H__
#define __VARIANTMEDIADEF_H__

// Variant parameters for IRAM Media Driver (MEDINT.PDD)
#define IRAM_DRIVECOUNT 1
#define IRAM_DRIVELIST 0
#define IRAM_NUMMEDIA 1	
#define IRAM_DRIVENAME "IRam"

// Variant parameters for LFFS Media Driver (MEDLFS.PDD)
#define LFFS_DRIVECOUNT 1
#define LFFS_DRIVELIST 8
#define LFFS_NUMMEDIA 1	
#define LFFS_DRIVENAME "Flash"

// Variant parameters for the MMC Controller (EPBUSMMC.DLL)
#define MMC0_DRIVECOUNT 1
#define MMC0_DRIVELIST 3
#define MMC0_NUMMEDIA 1	
#define MMC0_DRIVENAME "MultiMediaCard0"

// Variant parameters for the NAND media driver (MEDNAND.PDD)
// Note that the NANDLOADER code expects the are to be 2 drives/partitions 
#define NAND_DRIVECOUNT 2
#define NAND_DRIVELIST 6,7
#define NAND_NUMMEDIA 1	
#define NAND_DRIVENAME "Nand"

#endif
