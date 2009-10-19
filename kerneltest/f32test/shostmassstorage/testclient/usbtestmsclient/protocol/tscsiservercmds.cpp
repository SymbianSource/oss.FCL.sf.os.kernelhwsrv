// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//


#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <e32std.h>

#include "mstypes.h"
#include "msctypes.h"
#include "mscutils.h"
#include "scsimsctypes.h"

#include "tscsiserverreq.h"
#include "tscsiservercmds.h"
#include "debug.h"
#include "msdebug.h"

/**
Default constructor for TSenseInfo
*/
TSrvSenseInfo::TSrvSenseInfo()
    {
    __MSFNLOG
    iSenseCode = ENoSense;
    iAdditional = 0;
	iQualifier = 0;
    }

/**
Set sense with no additional info.

@param aSenseCode sense key
*/
void TSrvSenseInfo::SetSense(TSenseCode aSenseCode)
	{
    __MSFNLOG
	iSenseCode	= static_cast<TUint8>(aSenseCode);
	iAdditional = 0;
	iQualifier  = 0;
	}


/**
Set sense with additional info.

@param aSenseCode sense key
@param aAdditional additional sense code (ASC)
*/
void TSrvSenseInfo::SetSense(TSenseCode aSenseCode,
                          TAdditionalCode aAdditional)

	{
    __MSFNLOG
	iSenseCode = static_cast<TUint8>(aSenseCode);
	iAdditional = static_cast<TUint8>(aAdditional);
	iQualifier = 0;
	}


/**
Set sense with additional info and qualifier.

@param aSenseCode sense key
@param aAdditional additional sense code (ASC)
@param aQualifier additional sense code qualifier (ASCQ)
*/
void TSrvSenseInfo::SetSense(TSenseCode aSenseCode,
                          TAdditionalCode aAdditional,
                          TUint8 aQualifier)
	{
    __MSFNLOG
	iSenseCode = static_cast<TUint8>(aSenseCode);
	iAdditional = static_cast<TUint8>(aAdditional);
	iQualifier = aQualifier;
	}


// **** TEST UNIT READY ****
// **** REQUEST SENSE ****
void TScsiServerRequestSenseResp::Encode(TDes8& aBuffer) const
    {
    __MSFNSLOG
    aBuffer.FillZ(KCommandLength);
    __PRINT(_L("->PROTOCOL(SCSI) REQUEST SENSE\n"));
    //additional sense length
	aBuffer[07] = static_cast<TUint8>(KCommandLength - 8);

    aBuffer[0] = iResponseCode;
	aBuffer[02] = static_cast<TUint8>(iSensePtr->iSenseCode);
	aBuffer[12] = iSensePtr->iAdditional;
	aBuffer[13] = iSensePtr->iQualifier;

    //truncate to Allocation Length of the Request
    TUint length = iAllocationLength < KCommandLength ?
                    iAllocationLength : KCommandLength;
    aBuffer.SetLength(length);
    }

// **** INQUIRY ****
void TScsiServerInquiryReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);
    iCmdDt = aPtr[1] & 0x2;
    iEvpd = aPtr[1] & 0x1;
    iPage = aPtr[2];
    iAllocationLength = aPtr[4];
    __PRINT(_L("<-PROTOCOL(SCSI) INQUIRY\n"));
    }


void TScsiServerInquiryResp::Encode(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __PRINT(_L("->PROTOCOL(SCSI) INQUIRY\n"));

	aBuffer.FillZ(KResponseLength);

    // MSB: RMB : Removable
    if (iRemovable)
        {
        aBuffer[1] |= 0x80;
        }

    // AERC, TrmTsk, NormACA, Response Data Format
    aBuffer[3] |= (iResponseDataFormat & 0x0F);

    // Additional Length
	aBuffer[4] = 0x1F;

    // Vendor ID (Vendor Specific/Logged by T10)
	TPtr8 vendorId(&aBuffer[8], 8, 8);
	vendorId.Fill(' ', 8);
	vendorId.Copy(iConfig.iVendorId);

    // Product ID (Vendor Specific)
    TPtr8 productId(&aBuffer[16], 16, 16);
    productId.Fill(' ', 16);
    productId.Copy(iConfig.iProductId);

    // Product Revision Level (Vendor Specific)
    TPtr8 productRev(&aBuffer[32], 4, 4);
    productRev.Fill(' ', 4);
    productRev.Copy(iConfig.iProductRev);

    // Truncate to Allocation Length of the Request
    TUint length = iAllocationLength < KResponseLength ?
                    iAllocationLength : KResponseLength;
    aBuffer.SetLength(length);
    }


// ****	MODE SENSE (6) ****
void TScsiServerModeSense6Req::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);
    iPageCode = aPtr[2] & 0x3F;
    iPageControl = static_cast<TPageControl>(aPtr[2] >> 6);
    iAllocationLength = aPtr[4];
    __PRINT(_L("<-PROTOCOL(SCSI) MODE SENSE (6)\n"));
    }


void TScsiServerModeSense6Resp::Encode(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __PRINT(_L("->PROTOCOL(SCSI) MODE SENSE (6)\n"));
    // reserve 4 bytes for Length, Media type, Device-specific parameter and
    // Block descriptor length
    aBuffer.FillZ(KCommandLength);

    // Mode Parameter List
    // SPC-3 7.4.2
    // - Mode Parameter Header
    // - Block Descriptor(s)
    // - Mode Page(s)

    // Mode Parameter Header
    // SPC-3 7.4.3
    // [0] Mode Data Length
    // [1] Medium Type
    // [2] Device-Specific Paramater
    // [3] Block Descriptor Length

    // [0] Mode Date Length
    // Sending only Mode parameter header
    aBuffer[0] = 3;

    // [1] Medium Type
    // 0x00 for SBC

    // [2] Device specific parameter
    // SBC-3 6.3.1
    // set SWP bit at the Device Specific parameters
    if (iWp)
        {
        aBuffer[2] |= 0x80;
        }

    // [3] Block Descriptor Length
    // 0x00 for no descriptors

    // No Block Descriptors

    // No Mode Pages

    // Truncate to Allocation Length of the Request
    TUint length = iAllocationLength < KCommandLength ?
                    iAllocationLength : KCommandLength;
    aBuffer.SetLength(length);
    }

// ****	START STOP UNIT ****
void TScsiServerStartStopUnitReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);

    const TUint8 KStartMask = 0x01;
    const TUint8 KImmedMask = 0x01;
    const TUint8 KLoejMask = 0x02;

    iImmed = aPtr[1] & KImmedMask ? ETrue : EFalse;
    iStart = aPtr[4] & KStartMask ? ETrue : EFalse;
    iLoej = aPtr[4] & KLoejMask ? ETrue : EFalse;

    __PRINT2(_L("<-PROTOCOL(SCSI) START STOP UNIT Data %X %X\n"), aPtr[1], aPtr[4]);
    __PRINT1(_L("IMMED = %d\n"), iImmed);
    __PRINT1(_L("START = %d\n"), iStart);
    __PRINT1(_L("LOEJ = %d\n"), iLoej);
    }


// ****	PREVENT MEDIA REMOVAL ****
void TScsiServerPreventMediaRemovalReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);
	iPrevent = aPtr[4] & 0x01;
	__PRINT1(_L("<-PROTOCOL(SCSI) PREVENT MEDIA REMOVAL prevent = %d\n"), iPrevent);
    }


// ****	READ FORMAT CAPACITIES ****
void TScsiServerReadFormatCapacitiesReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);
    const TUint8* ptr = aPtr.Ptr();
    iAllocationLength = BigEndian::Get32(ptr+7);
	__PRINT(_L("<-PROTOCOL(SCSI) READ FORMAT CAPACITIES\n"));
    }


void TScsiServerReadFormatCapacitiesResp::Encode(TDes8& aBuffer) const
    {
    __MSFNSLOG
	__PRINT(_L("->PROTOCOL(SCSI) READ FORMAT CAPACITIES\n"));
	aBuffer.FillZ(KResponseLength);
	aBuffer[3] = 0x08;	// Capacity List Length

	aBuffer[4] = static_cast<TUint8>(iNumberBlocks >> 24);	// Number of blocks
	aBuffer[5] = static_cast<TUint8>(iNumberBlocks >> 16);	//
	aBuffer[6] = static_cast<TUint8>(iNumberBlocks >> 8);	//
	aBuffer[7] = static_cast<TUint8>(iNumberBlocks);		//

	aBuffer[8] = 0x02;	// Formatted size

	aBuffer[9]  = 0x00;	// 512 Byte Blocks
	aBuffer[10] = 0x02;	//
	aBuffer[11] = 0x00;	//

    // Truncate to Allocation Length of the Request
    // Truncate to Allocation Length of the Request
    TUint length = iAllocationLength < KResponseLength ?
                    iAllocationLength : KResponseLength;
    aBuffer.SetLength(length);
    }


// ****	READ CAPACITY (10) ****
void TScsiServerReadCapacity10Req::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerReq::DecodeL(aPtr);
    iPmi = aPtr[8] & 0x01;
    const TUint8* ptr = aPtr.Ptr();
	iLogicalBlockAddress = BigEndian::Get32(ptr+2);
    __PRINT(_L("<-PROTOCOL(SCSI) READ CAPACITY (10)\n"));
    }


void TScsiServerReadCapacity10Resp::Encode(TDes8& aBuffer) const
    {
    __MSFNSLOG
    aBuffer.FillZ(KCommandLength);

    __PRINT3(_L("->PROTOCOL(SCSI) READ CAPACITY (10) Block size=0x%X, NumBlocks=0x%08X%08X\n"),
             iBlockSize,
             I64HIGH(iNumberBlocks),
             I64LOW(iNumberBlocks));

    if (I64HIGH(iNumberBlocks) == 0)
        {
        TUint32 numBlocks = I64LOW(iNumberBlocks);

        // Number of blocks
        aBuffer[0] = static_cast<TUint8>(numBlocks >> 24);
        aBuffer[1] = static_cast<TUint8>(numBlocks >> 16);
        aBuffer[2] = static_cast<TUint8>(numBlocks >> 8);
        aBuffer[3] = static_cast<TUint8>(numBlocks);
        }
    else
        {
        // indicate that size more then )0xFFFFFFFF
        aBuffer[0] = aBuffer[1] = aBuffer[2] = aBuffer[3] = 0xFF;
        }

	// Block Size
    aBuffer[4] = static_cast<TUint8>(iBlockSize >> 24);
    aBuffer[5] = static_cast<TUint8>(iBlockSize >> 16);
    aBuffer[6] = static_cast<TUint8>(iBlockSize >> 8);
    aBuffer[7] = static_cast<TUint8>(iBlockSize);
    }


// ****	RdWr10 ****
void TScsiServerRdWr10Req::DecodeL(const TDesC8& aDes)
{
    __MSFNLOG
	TScsiServerReq::DecodeL(aDes);

    // PROTECT
	iProtect = aDes[1] >> 5;

    const TUint8* ptr = aDes.Ptr();
    // LOGICAL BLOCK ADDRESS
	iLogicalBlockAddress = BigEndian::Get32(ptr+2);
    // TRANSFER LENGTH
	iTransferLength = BigEndian::Get16(ptr+7);

	__PRINT2(_L("<-PROTOCOL(SCSI) RD/WR (10) : LBA = %x, Length = %x  (blocks)\n"),
             iLogicalBlockAddress, iTransferLength);
}


// ****	READ (10) ****
// ****	WRITE (10) ****
// ****	VERIFY (10) ****
void TScsiServerVerify10Req::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
	TScsiServerRdWr10Req::DecodeL(aPtr);
	iBytchk = aPtr[1] & 0x02 ? ETrue : EFalse;
    }
