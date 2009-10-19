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
// \e32\drivers\medmmc\mmcptn.cpp
// 
//

#include "sdcard.h"

const TInt KDiskSectorShift = 9;

LOCAL_C void SetPartitionType(const TMMCard& aCard, TMBRPartitionEntry& aPartitionEntry)
	{
	if(aCard.IsHighCapacity())
		{
		aPartitionEntry.iPartitionType = KPartitionTypeWin95FAT32;
		}
	else if(aPartitionEntry.iNumSectors < 32680)
		{
		aPartitionEntry.iPartitionType = KPartitionTypeFAT12;
		}
	else if(aPartitionEntry.iNumSectors < 65536)
		{
		aPartitionEntry.iPartitionType = KPartitionTypeFAT16small;
		}
	else if (aPartitionEntry.iNumSectors < 1048576)
		{
		aPartitionEntry.iPartitionType = KPartitionTypeFAT16;
 		}
	else
		{
		aPartitionEntry.iPartitionType = KPartitionTypeWin95FAT32;
		}
	}

/**
Get the write block size to allow the FAT file system to calculate a suitable cluster size
*/
GLDEF_C TInt BlockSize(const TMMCard* aCardP)
	{
	if ((aCardP == NULL) || ((const TSDCard *) aCardP)->IsSDCard())
		return KMMCardHighCapBlockSize;

	const TExtendedCSD& extCSD = aCardP->ExtendedCSD();

	TInt blockSize = 
		extCSD.ExtendedCSDRev() >= 3 && extCSD.AccessSize() < 9 ? 
		512 << (extCSD.AccessSize()-1) :
			KMMCardHighCapBlockSize;			// default write block size = 512

	return blockSize;
	}

/**
Get Erase block size to allow FAT file system to align first usable cluster correctly
*/
GLDEF_C TInt EraseBlockSize(const TMMCard* aCardP)
	{
	if (aCardP == NULL)
		return 0;

	
	const TUint K8KBShift = 13;
	if (((const TSDCard *) aCardP)->IsSDCard())
		{
		//Allocation Unit is returned as EraseBlockSize for SD card
		//as calculation for MMC card is not valid for SD card.
		TUint8 auSize = ((const TSDCard *) aCardP)->GetAUSize();
		//eraseBlkSize is 2^(auSize + K8KBShift)
		TInt eraseBlkSize = 1 << (auSize + K8KBShift) ;
		return (eraseBlkSize);
		}


	TInt eraseBlockSize;

	// Revision 1.0 of the "File Formats Specification for MultiMediaCards" dictates that 
	// for High-Capacity MMC cards (i.e. FAT32 cards), 
	// "There shall be a Master Boot Record with valid partition information. The partition(s)
	// "and user data area(s) shall start from the beginning of an erasable unit (i.e. physical
	// "block boundary).
	// Version 1.0 of the "The MultiMediaCard FAT16 File System Specification" recommends having a 
	// 16KB MBR and aligning the root directory entry (which is also 16KB long) and the first usable 
	// sector to a 16KB boundary

	const TUint K512KBShift = 19;
	const TInt K16KBytes = 16384;
	const TExtendedCSD& extCSD = aCardP->ExtendedCSD();

	if (extCSD.ExtendedCSDRev() >= 3 && extCSD.HighCapacityEraseGroupSize() && extCSD.EraseGroupDef())
		eraseBlockSize = extCSD.HighCapacityEraseGroupSize() << K512KBShift;
	else if (aCardP->IsHighCapacity())
		eraseBlockSize = aCardP->CSD().EraseGroupSize();
	else
		eraseBlockSize = K16KBytes;

	return eraseBlockSize;
	}

/**
	Calculates the default partition information for an MMC Card
	@param aPartitionEntry The TMBRPartitionEntry to be filled in with the format parameters
	@return Standard Symbian OS Error Code
 */
GLDEF_C TInt GetMediaDefaultPartitionInfo(TMBRPartitionEntry& aPartitionEntry, TUint16& aReservedSectors, const TMMCard* aCardP)
	{
	const TUint32 KTotalSectors = I64LOW(aCardP->DeviceSize64() >> KDiskSectorShift);

	aPartitionEntry.iFirstSector = EraseBlockSize(aCardP) >> KDiskSectorShift;
	aPartitionEntry.iNumSectors  = KTotalSectors - aPartitionEntry.iFirstSector;
	aPartitionEntry.iX86BootIndicator = 0x00;
	SetPartitionType(*aCardP, aPartitionEntry);

	aReservedSectors = 0;	// Let the filesystem decide on the appropriate value..

	return(KErrNone);
	}

/**
	Returns whether having a Master Boot Record is mandatory
	
	@return ETrue if MBR is mandatory for this card
 */
GLDEF_C TBool MBRMandatory(const TMMCard* /*aCardP*/)
	{
	return EFalse;
	}
/**
	Returns whether to create a Master Boot Record after a format
	
	@return ETrue if MBR should be created
 */
GLDEF_C TBool CreateMBRAfterFormat(const TMMCard* aCardP)
	{
	if(aCardP == NULL)
		{
		return EFalse;
		}

	// Create an MBR on high capacity (FAT32) cards and optionally on low-capacity FAT16/FAT32 MMC cards
	if (aCardP->IsHighCapacity())
		{
		return ETrue;
		}
#ifdef SYMBIAN_CREATE_MBR_ON_LOW_CAPACITY_MMC
	if ((I64LOW(aCardP->DeviceSize64()) >> KDiskSectorShift) >= 32680)
		{
		return ETrue;
		}
#endif
	
	return EFalse;
	}

GLDEF_C TInt GetCardFormatInfo(const TMMCard* /*aCardP*/, TLDFormatInfo& /*aFormatInfo*/)
/**
 * Returns the preferred format parameters for the primary partition.
 * @return Standard Symbian OS error code.
 */
	{
	return KErrNotSupported;
	}

