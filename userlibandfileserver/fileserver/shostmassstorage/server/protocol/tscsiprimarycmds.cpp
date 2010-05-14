// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// scsiprimarycmds.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32base.h>

#include "msctypes.h"
#include "debug.h"
#include "msdebug.h"

#include "mtransport.h"
#include "mprotocol.h"

#include "tscsiclientreq.h"
#include "tscsiprimarycmds.h"


// **** TEST UNIT READY ****
TInt TScsiClientTestUnitReadyReq::EncodeRequestL(TDes8& aBuffer) const
    {
	__MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI TEST UNIT READY"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);
    return length;
    }


// **** REQUEST SENSE ****
void TScsiClientRequestSenseResp::DecodeSenseInfo(const TDesC8& aPtr)
    {
	__MSFNSLOG
    iResponseCode = static_cast<TResponseCode>(aPtr[0]);
    iSenseInfo.iSenseCode = aPtr[02];
    iSenseInfo.iAdditional = aPtr[12];
    iSenseInfo.iQualifier = aPtr[13];
    }


void TScsiClientRequestSenseResp::DecodeL(const TDesC8& aPtr)
	{
	__MSFNSLOG
    __SCSIPRINT(_L("--> SCSI REQUEST SENSE"));
    if (aPtr.Length() < KResponseLength)
        {
        // Handle short data.
        // The data not transferred is assumed to be zero.

        // Create full size buffer
        TBuf8<KResponseLength> buffer;

        // Copy data into buffer
        buffer = aPtr;
        // Fill remainder with 0's
        buffer.AppendFill(0, KResponseLength - aPtr.Length());

        DecodeSenseInfo(buffer);
        }
    else
        {
        DecodeSenseInfo(aPtr);
        }
	}


// **** INQUIRY ****
void TScsiClientInquiryResp::DecodeInquiry(const TDesC8& aPtr)
    {
	__MSFNSLOG
    iPeripheralInfo.iRemovable = (aPtr[1] & 0x80) ? ETrue : EFalse;

    iPeripheralInfo.iPeripheralQualifier = aPtr[0] >> 5;
    iPeripheralInfo.iPeripheralDeviceType = aPtr[0] & 0x1F;
    iPeripheralInfo.iVersion = aPtr[2];
    iPeripheralInfo.iResponseDataFormat = aPtr[3] & 0x0F;

    TPtrC8 vendorId(&aPtr[8], 8);
    iPeripheralInfo.iIdentification.iVendorId.Copy(vendorId);

    TPtrC8 productId(&aPtr[16], 16);
    iPeripheralInfo.iIdentification.iProductId.Copy(productId);

    TPtrC8 productRev(&aPtr[32], 4);
    iPeripheralInfo.iIdentification.iProductRev.Copy(productRev);
    }


void TScsiClientInquiryResp::DecodeL(const TDesC8& aPtr)
	{
    __MSFNSLOG
    __SCSIPRINT(_L("--> SCSI INQUIRY"));
    if (aPtr.Length() < KResponseLength)
        {
        // Handle short data.
        // The data not transferred is assumed to be zero.

        // Create zeroed buffer
        TBuf8<KResponseLength> buffer;
        buffer.FillZ(KResponseLength);

        // Copy data into buffer
        buffer = aPtr;
        buffer.SetLength(KResponseLength);

        DecodeInquiry(buffer);
        }
    else
        {
        DecodeInquiry(aPtr);
        }
	}


// ****	PREVENT MEDIA REMOVAL ****
TInt TScsiClientPreventMediaRemovalReq::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI PREVENT MEDIA REMOVAL"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);
	if (iPrevent)
        aBuffer[4] |= 0x01;
    return length;
    }

