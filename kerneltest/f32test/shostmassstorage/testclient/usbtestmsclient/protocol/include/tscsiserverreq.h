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
// SCSI Protocol layer for USB Mass Storage
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef TSCSISERVERREQ_H
#define TSCSISERVERREQ_H


typedef TUint TProtect;     // 3 bits
typedef TUint TAllocationLength;    // 1 Byte
typedef TUint TAllocationLength2;   // 2 Byte

typedef TBool TPrevent;
typedef TUint8 TGroupCode;


/**
Base Class for SCSI commands. This class represents the Command Description
Block described in SAM-2

@internalTechnology
 */
class TScsiServerReq
    {
public:
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
            EUndefinedCommand		= 0xFF
        };


public:
	TScsiServerReq() {};
    TScsiServerReq(TOperationCode aOperationCode);

    static TScsiServerReq* CreateL(TScsiServerReq::TOperationCode aOperationCode, TDesC8& aDes);

    virtual void DecodeL(const TDesC8& aPtr);
private:
    TGroupCode GetGroupCode() const;

public:
    TOperationCode iOperationCode;
    TGroupCode iGroupCode;

    // CDB CONTOL Byte SAM-2 5.2.3
    TBool iNaca;
    TBool iLink;
    };


class MScsiServerResp
    {
public:
    virtual void Encode(TDes8& aBuffer) const = 0;

private:
    };


#include "tscsiserverreq.inl"

#endif // TSCSISERVERREQ_H
