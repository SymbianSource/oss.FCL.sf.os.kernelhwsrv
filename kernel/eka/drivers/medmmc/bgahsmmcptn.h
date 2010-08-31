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
#define BGAHSMMCPTN_PI_VER_MINOR	1

/* Partition type field supported from version 1.1 onwards */
#define BGAHSMMCPTN_PART_TYPE_SUPP_VER_MINOR    1

#define BGAHSMMCPTN_LAST_DRIVE		16 /* MMC1_DRIVECOUNT - defined in variantmediadef.h */

typedef struct
{
	TUint32 iStart_sector;			/* Partition start sector */
	TUint32 iSize;					/* Partition size in sectors */
	TUint32 iAttributes;			/* RO, RW attributes (bitmask) */
	TUint8  iPartition_id;			/* Partition number - v1.0 - partition type v1.1 - drive number*/
	TUint8  iPartition_type;         /* Partition Type - v1.1 onwards */
	TUint8  iReserved1[2];			/* Reserved (padding for compiler alignment)*/
	TUint32 iPartition_attributes;	/* Partition attributes (see e32const.h) */
	TUint8  iReserved2[8];			/* Reserved */
/* = 28 bytes */
} BGAHSMMCPTN_PART_STR;

typedef struct
{
	TUint8  iId[BGAHSMMCPTN_PI_ID_SIZE];	/* Partition information version */
	TUint32 iSector_size;			/* Used sector size */
	TUint16 iMajor_ver;				/* Major version number */
	TUint16 iMinor_ver;				/* Minor version number */
	TUint16 iPartition_amount;		/* The amount of partitions */
	TUint8  iReserved[42];			/* Reserved */
/* = 64 bytes */
	BGAHSMMCPTN_PART_STR iPartitions[BGAHSMMCPTN_LAST_DRIVE];	
} BGAHSMMCPTN_PI_STR;

#define BGAHSMMCPTN_PI_STR_SIZE sizeof( BGAHSMMCPTN_PI_STR )

const TUint32 KBB5HeaderSizeInSectors   = 8;
const TUint32 KPIOffsetFromMediaEnd     = 1;

#endif /*BGAHSMMCPTN_H*/
