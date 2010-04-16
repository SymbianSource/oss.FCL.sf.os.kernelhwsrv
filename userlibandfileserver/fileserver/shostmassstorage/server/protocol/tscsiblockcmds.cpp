// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include "debug.h"
#include "msdebug.h"
#include "msctypes.h"
#include "mscutils.h"

#include "mtransport.h"
#include "mprotocol.h"
#include "tscsiclientreq.h"
#include "tscsiblockcmds.h"


// ****	MODE SENSE (6) ****
void TScsiClientModeSense6Resp::DecodeL(const TDesC8& aPtr)
	{
    __MSFNSLOG
    __SCSIPRINT(_L("--> SCSI MODE SENSE (6)"));
    __SCSIPRINT1(_L("len=%d"), aPtr.Length());
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


    // validate length
    if (aPtr.Length() < KResponseLength)
        {
        User::Leave(KErrGeneral);
        }

    TInt modeDataLength = aPtr[0];
    if (aPtr.Length() - 1 > modeDataLength)
        {
        User::Leave(KErrGeneral);
        }
   
    TInt mediumType = aPtr[1];
    TUint8 deviceSpecificParameter = aPtr[2];
    // TInt blockDescriptorLength = aPtr[3];

    __SCSIPRINT2(_L("Medium Type=%d DSP=0x%x"), mediumType, deviceSpecificParameter);
    // [1] Medium Type
    // 0x00 for SBC
    if (mediumType == 0)
        {
        // [2] Device specific parameter
        // SBC-3 6.3.1
        // get the WP bit from the Device Specific parameters
        iWriteProtected = (deviceSpecificParameter & 0x80) ? ETrue : EFalse;
        }
    else
        {
        // unsupported medium type
        iWriteProtected = EFalse;
        }

    // [3] Block Descriptor Length
    // 0x00 for no descriptors

    // No Block Descriptors

    // No Mode Pages

	}


// ****	MODE SENSE (10) ****
void TScsiClientModeSense10Resp::DecodeL(const TDesC8& aPtr)
	{
    __MSFNSLOG
    __SCSIPRINT(_L("--> SCSI MODE SENSE (10)"));
    __SCSIPRINT1(_L("len=%d"), aPtr.Length());
    // Mode Parameter List
    // SPC-3 7.4.2
    // - Mode Parameter Header
    // - Block Descriptor(s)
    // - Mode Page(s)

    // Mode Parameter Header
    // SPC-3 7.4.3
    // [0] Mode Data Length
    // [1] Mode Data Length
    // [2] Medium Type
    // [3] Device-Specific Paramater
    // [4] Reserved
    // [5] Reserved
    // [6] Block Descriptor Length
    // [7] Block Descriptor Length

    // validate length
    if (aPtr.Length() < KResponseLength)
        {
        User::Leave(KErrGeneral);
        }
    
    TInt modeDataLength = BigEndian::Get16(&aPtr[0]);
    if (aPtr.Length() - 2 > modeDataLength)
        {
        User::Leave(KErrGeneral);
        }

    TInt mediumType = aPtr[2];
    TUint8 deviceSpecificParameter = aPtr[3];
    // TInt blockDescriptorLength = BigEndian::Get32(&aPtr[6]);;

    __SCSIPRINT2(_L("Medium Type=%d DSP=0x%x"), mediumType, deviceSpecificParameter);
    // [1] Medium Type
    // 0x00 for SBC
    if (mediumType == 0)
        {
        // [2] Device specific parameter
        // SBC-3 6.3.1
        // get the WP bit from the Device Specific parameters
        iWriteProtected = (deviceSpecificParameter & 0x80) ? ETrue : EFalse;
        }
    else
        {
        // unsupported medium type
        iWriteProtected = EFalse;
        }

    // [3] Block Descriptor Length
    // 0x00 for no descriptors

    // No Block Descriptors

    // No Mode Pages

	}



// ****	READ CAPACITY (10) ***
TInt TScsiClientReadCapacity10Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- READ CAPACITY 10"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    if (iLba)
        {
        // PMI bit
        aBuffer[8] = 0x01;
        // LBA
        BigEndian::Put32(&aBuffer[2], iLba);
        }
    return length;
    }


void TScsiClientReadCapacity10Resp::DecodeL(const TDesC8& aPtr)
	{
    __MSFNSLOG
    __SCSIPRINT(_L("--> SCSI READ CAPACITY (10)"));
    iLba = BigEndian::Get32(&aPtr[0]);
    iBlockSize = BigEndian::Get32(&aPtr[4]);
	}


// ****	RdWr10 ****
TInt TScsiClientRdWr10Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    // PROTECT
    if (iProtect)
        aBuffer[1] = iProtect << 5;

    __SCSIPRINT2(_L("LBA=%08x LEN=%08x"), iLogicalBlockAddress, iBlockTransferLength);
    // LOGICAL BLOCK ADDRESS
    BigEndian::Put32(&aBuffer[2], iLogicalBlockAddress);
    // TRANSFER LENGTH
    BigEndian::Put16(&aBuffer[7], iBlockTransferLength);
    return length;
    }

// ****	READ (10) ****
TInt TScsiClientRead10Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI READ (10)"));
	TInt length = TScsiClientRdWr10Req::EncodeRequestL(aBuffer);
    return length;
    }


// ****	START STOP UNIT ****
TInt TScsiClientStartStopUnitReq::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("--> SCSI START STOP UNIT"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    // byte 1 mask
    const TUint8 KImmedMask = 0x01;

    // byte 4 mask
    const TUint8 KStartMask = 0x01;
    const TUint8 KLoejMask = 0x02;

    if (iImmed)
        aBuffer[1] |= KImmedMask;
    if (iStart)
        aBuffer[4] |= KStartMask;
    if (iLoej)
        aBuffer[4] |= KLoejMask;
    return length;
    }


// ****	WRITE (10) ****
TInt TScsiClientWrite10Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI WRITE 10"));

    TInt length = TScsiClientRdWr10Req::EncodeRequestL(aBuffer);
    return length;
    }

