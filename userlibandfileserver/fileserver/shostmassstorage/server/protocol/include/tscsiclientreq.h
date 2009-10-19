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

#ifndef TSCSICLIENTREQ_H
#define TSCSICLIENTREQ_H

/**
Data type for GROUP CODE
SAM-2 5.2.2
SPC-3 4.3.4.1
*/
typedef TUint8 TGroupCode;

/**
Data type for ALLOCATION LENGTH
SPC-2 4.3.4.6
SPC-3 4.3.4.6
*/
typedef TUint TAllocationLength;    // 1 Byte

/**
Base class to decode SCSI RESPONSE
*/
class TScsiClientResp: public MClientCommandServiceResp
    {
public:
    TScsiClientResp() {};
    };


/**
Base class to encode a SCSI REQUEST which implements the SCSI header encoding
*/
class TScsiClientReq: public MClientCommandServiceReq
    {
public:
    /** SCSI OPERATION CODE */
    enum TOperationCode
        {
        ETestUnitReady			= 0x00,
        ERequestSense			= 0x03,
        EInquiry 				= 0x12,
        EModeSense6				= 0x1A,
        EStartStopUnit			= 0x1B,
        EPreventMediaRemoval	= 0x1E,
        EReadFormatCapacities	= 0x23,
        EReadCapacity10			= 0x25,
        ERead10 				= 0x28,
        EWrite10				= 0x2A,
        EVerify10				= 0x2f,
        EModeSense10			= 0x5A,
        EUndefinedCommand		= 0xFF
        };

    TScsiClientReq(TOperationCode aOperationCode);

    virtual TInt EncodeRequestL(TDes8& aBuffer) const;

private:
    TUint8 GetControlByte() const;
    TGroupCode GetGroupCode() const;

    TBool IsDataDirectionOut() const;
    TUint32 DataLength() const;

public:
    /** SCSI OPERATION CODE of CDB byte, SAM-2 5.2.2 */
    const TOperationCode iOperationCode;
    /** SCSI NACA field of CDB CONTROL Byte. SAM-2 5.2.3 */
    TBool iNaca;
    /** SCSI LINK field of CDB CONTROL Byte. SAM-2 5.2.3 */
    TBool iLink;
    };

#include "tscsiclientreq.inl"

#endif // TSCSICLIENTREQ_H
