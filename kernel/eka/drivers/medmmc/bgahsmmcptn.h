// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Partition Management for Embedded MMC devices
// 
//

#ifndef BGAHSMMCPTN_H
#define BGAHSMMCPTN_H

/* Partition attributes */
#define BGAHSMMCPTN_ATTR_READ 0x00
#define BGAHSMMCPTN_ATTR_WRITE 0x01

/* Partition information ID and fixed size */
#define BGAHSMMCPTN_PI_ID "BGAHSMMCPI:"
#define BGAHSMMCPTN_PI_ID_SIZE 12 /* Fixed (4B aligned) */

/* Partition information version */
#define BGAHSMMCPTN_PI_VER_MAJOR	1
#define BGAHSMMCPTN_PI_VER_MINOR	0

#define BGAHSMMCPTN_LAST_DRIVE		7 /* MMC1_DRIVECOUNT - defined in variantmediadef.h */

typedef struct
{
	TUint32 start_sector;			/* Partition start sector */
	TUint32 size;					/* Partition size in sectors */
	TUint32 attributes;				/* RO, RW attributes (bitmask) */
	TUint8  partition_id;			/* Partition number */
	TUint8  reserved1[3];			/* Reserved (padding for compiler alignment)*/
	TUint32 partition_attributes;	/* Partition attributes (see e32const.h) */
	TUint8  reserved2[8];			/* Reserved */
/* = 28 bytes */
} BGAHSMMCPTN_PART_STR;

typedef struct
{
	TUint8 id[BGAHSMMCPTN_PI_ID_SIZE];	/* Partition information version */
	TUint32 sector_size;			/* Used sector size */
	TUint16 major_ver;				/* Major version number */
	TUint16 minor_ver;				/* Minor version number */
	TUint16 partition_amount;		/* The amount of partitions */
	TUint8 reserved[42];			/* Reserved */
/* = 64 bytes */
	BGAHSMMCPTN_PART_STR partitions[BGAHSMMCPTN_LAST_DRIVE];
} BGAHSMMCPTN_PI_STR;

#define BGAHSMMCPTN_PI_STR_SIZE sizeof( BGAHSMMCPTN_PI_STR )

#endif /*BGAHSMMCPTN_H*/
