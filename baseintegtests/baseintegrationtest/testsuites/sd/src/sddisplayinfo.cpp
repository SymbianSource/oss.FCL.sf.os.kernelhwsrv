// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Display contents of the card registers
// 
//

#include "sddisplayinfo.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDDisplayInfo::CBaseTestSDDisplayInfo()
	{
	SetTestStepName(KTestStepDisplayInfo);
	}

/*
Test Step Preamble
 - Load device driver for direct disk access

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDDisplayInfo::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDeviceDriver())
		return TestStepResult();

	SetTestStepResult(EPass);
	return TestStepResult();
	}

/**
Extract a value out of a 128-bit block (little-endian)

@param aArrayPtr Pointer to an array of 16x 8-bit values
@param aStart Position of the first bit
@param aEnd Position of the last bit

@return The value requested
*/
TUint32 CBaseTestSDDisplayInfo::Slice128(TUint8* aArrayPtr, TInt aStart, TInt aEnd)
	{
	TInt retval = 0;
	for (TInt i = aStart; i >= aEnd; i--)
		{
		retval = retval << 1;
		retval += ((*(aArrayPtr + i / 8)) & (1 << (i % 8))) >> (i % 8);
		}
	return retval;
	}
	
/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDDisplayInfo::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		INFO_PRINTF1(_L("Display contents of card registers"));
		INFO_PRINTF1(_L("++CID"));
		INFO_PRINTF2(_L("--CID/MID:                  %02xh"),				Slice128(iCardInfo.iCID, 127, 120));
		INFO_PRINTF3(_L("--CID/OID:                  '%c%c'"),				iCardInfo.iCID[14], iCardInfo.iCID[13]);
		INFO_PRINTF6(_L("--CID/PNM:                  '%c%c%c%c%c'"),		iCardInfo.iCID[12], iCardInfo.iCID[11], iCardInfo.iCID[10], iCardInfo.iCID[9], iCardInfo.iCID[8]);
		INFO_PRINTF3(_L("--CID/PRV:                  %d.%d"),				Slice128(iCardInfo.iCID, 63, 60), Slice128(iCardInfo.iCID, 59, 56));
		INFO_PRINTF5(_L("--CID/PSN:                  %02x%02x%02x%02xh"),	iCardInfo.iCID[6], iCardInfo.iCID[5], iCardInfo.iCID[4], iCardInfo.iCID[3]);
		INFO_PRINTF3(_L("--CID/MDT:                  %02d/%d"),				Slice128(iCardInfo.iCID, 11, 8), 2000 + Slice128(iCardInfo.iCID, 19, 12));
		INFO_PRINTF2(_L("--CID/CRC:                  %02xh"),				Slice128(iCardInfo.iCID, 7, 1));
	
		INFO_PRINTF2(_L("++RCA:                      %04x"), iCardInfo.iRCA);
	
		INFO_PRINTF1(_L("++CSD"));
		INFO_PRINTF2(_L("--CSD/CSD_STRUCTURE:        %02xh"),		Slice128(iCardInfo.iCSD, 127, 126));
		INFO_PRINTF2(_L("--CSD/TAAC:                 %02xh"),		Slice128(iCardInfo.iCSD, 119, 112));
		INFO_PRINTF2(_L("--CSD/NSAC:                 %02xh"),		Slice128(iCardInfo.iCSD, 111, 104));
		INFO_PRINTF2(_L("--CSD/TRAN_SPEED:           %02xh"),		Slice128(iCardInfo.iCSD, 103, 96));
		INFO_PRINTF2(_L("--CSD/CCC:                  %012bb"),		Slice128(iCardInfo.iCSD, 95, 84));
		INFO_PRINTF2(_L("--CSD/READ_BL_LEN:          %xh"), 		Slice128(iCardInfo.iCSD, 83, 80));
		INFO_PRINTF2(_L("--CSD/READ_BL_PARTIAL:      %b"), 			Slice128(iCardInfo.iCSD, 79, 79));
		INFO_PRINTF2(_L("--CSD/WRITE_BLK_MISALIGN:   %b"),			Slice128(iCardInfo.iCSD, 78, 78));
		INFO_PRINTF2(_L("--CSD/READ_BLK_MISALIGN:    %b"),			Slice128(iCardInfo.iCSD, 77, 77));
		INFO_PRINTF2(_L("--CSD/DSR_IMP:              %b"), 			Slice128(iCardInfo.iCSD, 76, 76));
		if (Slice128(iCardInfo.iCSD, 127, 126) == 0)
			{
			INFO_PRINTF2(_L("--CSD/C_SIZE:               %03xh"), Slice128(iCardInfo.iCSD, 73, 62));
			}
		else if (Slice128(iCardInfo.iCSD, 127, 126) == 1) // High capacity
			{
			INFO_PRINTF2(_L("--CSD/C_SIZE:               %06xh"), Slice128(iCardInfo.iCSD, 69, 48));
			}
		INFO_PRINTF2(_L("--CSD/VDD_R_CURR_MIN:       %03bb"),		Slice128(iCardInfo.iCSD, 61, 59));
		INFO_PRINTF2(_L("--CSD/VDD_R_CURR_MAX:       %03bb"),		Slice128(iCardInfo.iCSD, 58, 56));
		INFO_PRINTF2(_L("--CSD/VDD_W_CURR_MIN:       %03bb"),		Slice128(iCardInfo.iCSD, 55, 53));
		INFO_PRINTF2(_L("--CSD/VDD_W_CURR_MAX:       %03bb"),		Slice128(iCardInfo.iCSD, 52, 50));
		INFO_PRINTF2(_L("--CSD/C_SIZE_MULT:          %03bb"),		Slice128(iCardInfo.iCSD, 49, 47));
		INFO_PRINTF2(_L("--CSD/ERASE_BLK_EN:         %b"),			Slice128(iCardInfo.iCSD, 46, 46));
		INFO_PRINTF2(_L("--CSD/SECTOR_SIZE:          %07bb"),		Slice128(iCardInfo.iCSD, 45, 39));
		INFO_PRINTF2(_L("--CSD/WP_GRP_SIZE:          %07bb"),		Slice128(iCardInfo.iCSD, 38, 32));
		INFO_PRINTF2(_L("--CSD/WP_GRP_ENABLE:        %d"),			Slice128(iCardInfo.iCSD, 31, 31));
		INFO_PRINTF2(_L("--CSD/R2W_FACTOR:           %03bb"),		Slice128(iCardInfo.iCSD, 28, 26));
		INFO_PRINTF2(_L("--CSD/WRITE_BL_LEN:         %04bb"),		Slice128(iCardInfo.iCSD, 25, 22));
		INFO_PRINTF2(_L("--CSD/WRITE_BL_PARTIAL:     %b"),			Slice128(iCardInfo.iCSD, 21, 21));
		INFO_PRINTF2(_L("--CSD/FILE_FORMAT_GRP:      %b"),			Slice128(iCardInfo.iCSD, 15, 15));
		INFO_PRINTF2(_L("--CSD/COPY:                 %b"),			Slice128(iCardInfo.iCSD, 14, 14));
		INFO_PRINTF2(_L("--CSD/PERM_WRITE_PROTECT:   %b"),			Slice128(iCardInfo.iCSD, 13, 13));
		INFO_PRINTF2(_L("--CSD/TMP_WRITE_PROTECT:    %b"),			Slice128(iCardInfo.iCSD, 12, 12));
		INFO_PRINTF2(_L("--CSD/FILE_FORMAT:          %02b"),		Slice128(iCardInfo.iCSD, 11, 10));
		INFO_PRINTF2(_L("--CSD/CRC:                  %07b"),		Slice128(iCardInfo.iCSD, 7, 1));
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}
