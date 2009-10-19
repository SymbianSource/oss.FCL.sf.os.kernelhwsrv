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
// e32\include\drivers\NandDevice.h
// 
//

#ifndef __NAND_DEVICE_H__
#define __NAND_DEVICE_H__


/**
This enum contains an entry for every manufacturer ID of NAND flash which
Symbian OS currently supports.
@publishedPartner
@released
*/
enum TManufacturerId
	{
	ESamsungId = 0xEC
	};

/**
This enum specifies characterisitics exhibited by NAND devices.
@publishedPartner
@released
*/
enum TDeviceFlags
	{
	ELargeBlock = 0x01,
	ECycle5Dev = 0x02,
	EDataIoWidth16 = 0x04		// i/o width is 16 bits wide
	};

/**
Container for storing all of the information about a particular
type of NAND flash device.
@publishedPartner
@released
*/
struct TNandDeviceInfo
	{ 
	TManufacturerId     iManufacturerCode;
	TUint8				iDeviceCode;
	TUint32				iNumBlocks;			// no. of erase blocks in a device
	TUint32				iNumSectorsPerBlock;// no. of sectors in an erase block
	TUint32				iNumBytesMain;		// size of a main array for one sector
	TUint32				iNumBytesSpare;		// size of a spare array for one sector
    TUint8				iSectorShift;		// shift value for a sector
    TUint8				iBlockShift;		// shift value for a block	
	TUint16				iBlksInRsv;			// number of blocks in reservoir
    TUint8				iBadPos;			// BadBlock Information Position in spare block
	TUint8				iLsnPos;			// Lsn position in spare array
	TUint8				iECCPos;			// ECC position in spare array
	TDeviceFlags		iFlags;
	};

#endif
