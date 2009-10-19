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

#ifndef TSCSIBLOCKCMDS_H
#define TSCSIBLOCKCMDS_H


/**
Data type for Logical Block Address
*/
typedef TUint32 TLba;

/**
Data type for PROTECT filed used in READ (10) and WRITE (10)
SBC-3 5.6 Table 33 and 5.25 Table 68
*/
typedef TUint TProtect;     // 3 bits

class TScsiClientReq;


/**
Encode SCSI MODE SENSE(6) REQUEST
Ref SPC-2 7.8 Request
Ref SPC-3 6.9 Request
Ref SPC-3 7.4 Response format
Ref SBC-3 6.3 Response format for SBC
*/
class TScsiClientModeSense6Req: public TScsiClientReq
    {
public:
    /** SCSI PAGE CONTROL values */
    enum TPageControl
        {
        ECurrentValues = 0x00,
        EChangeableValues = 0x01,
        EDefaultValues = 0x02,
        ESavedValues = 0x03
        };

    /** Length of MODE SENSE (6) response */
    static const TUint KResponseLength = 4;

    TScsiClientModeSense6Req(TPageControl aPageControl,
                             TUint aPageCode,
                             TUint aSubPageCode = 0);

    TInt EncodeRequestL(TDes8& aBuffer) const;

public:
    /** SCSI PAGE CONTROL field */
    TPageControl iPageControl;
    /** SCSI PAGE CODE field */
    TUint iPageCode;
    /** SCSI SUB PAGE CODE field */
    TUint iSubPageCode;
    /** SCSI ALLOCATION LENGTH field */
    TAllocationLength iAllocationLength;
    };


/**
Encode SCSI MODE SENSE(6) RESPONSE
Ref SPC-3 6.9 Request
Ref SPC-3 7.4 Response format
Ref SBC-3 6.3 Response format for SBC
*/
class TScsiClientModeSense6Resp: public TScsiClientResp
    {
public:
    /** Length of MODE SENSE (6) response */
    static const TUint KResponseLength = 4;

    TScsiClientModeSense6Resp() {};

    TInt DataLength() const {return KResponseLength;}
    void DecodeL(const TDesC8& aPtr);

public:
    /** Returned SCSI WP flag */
    TBool iWriteProtected;
    };


/**
Encode MODE SENSE (10) REQUEST
Ref SPC-2 7.9  Request
Ref SPC-3 6.10 Request
Ref SPC-2 8.2  Response format
Ref SPC-3 7.4  Response format
Ref SBC-3 6.3  Response format for SBC
*/
class TScsiClientModeSense10Req: public TScsiClientReq
    {
public:
    /** SCSI PAGE CONTROL values */
    enum TPageControl
        {
        ECurrentValues = 0x00,
        EChangeableValues = 0x01,
        EDefaultValues = 0x02,
        ESavedValues = 0x03
        };

    /** Length of MODE SENSE (10) response */
    static const TUint KResponseLength = 8;

    TScsiClientModeSense10Req(TPageControl aPageControl,
                              TUint aPageCode,
                              TUint aSubPageCode = 0);

    TInt EncodeRequestL(TDes8& aBuffer) const;

public:
    /** SCSI PAGE CONTROL field */
    TPageControl iPageControl;
    /** SCSI PAGE CODE field */
    TUint iPageCode;
    /** SCSI SUB PAGE CODE field */
    TUint iSubPageCode;
    /** SCSI ALLOCATION LENGTH field */
    TAllocationLength iAllocationLength;
    };


/**
Decode SCSI MODE SENSE (10) RESPONSE
Ref SPC-3 6.10 Request
Ref SPC-3 7.4  Response format
Ref SBC-3 6.3  Response format for SBC
*/
class TScsiClientModeSense10Resp: public TScsiClientResp
    {
public:
    /** Length of MODE SENSE (10) response */
    static const TUint KResponseLength = 8;

    TScsiClientModeSense10Resp() {};

    TInt DataLength() const {return KResponseLength;}
    void DecodeL(const TDesC8& aPtr);

public:
    /** Returned SCSI WP flag */
    TBool iWriteProtected;
    };

/**
Encode READ CAPCAITY (10) REQUEST
Ref SBC 2 5.10
Ref SBC 3 5.10
*/
class TScsiClientReadCapacity10Req: public TScsiClientReq
    {
public:
    /** Length of READ CAPACITY (10) response */
    static const TInt KResponseLength = 8;
    TScsiClientReadCapacity10Req();
    TScsiClientReadCapacity10Req(TLba aLba);
    TInt EncodeRequestL(TDes8& aBuffer) const;

public:
    /** SCSI LOGICAL BLOCK ADDRESS */
    TLba iLba;
    };

/**
Decode READ CAPCAITY (10) RESPONSE
*/
class TScsiClientReadCapacity10Resp: public TScsiClientResp
    {
public:
    /** Length of READ CAPACITY (10) response */
    static const TInt KResponseLength = 8;
    TScsiClientReadCapacity10Resp() {};

    void DecodeL(const TDesC8& aPtr);
    TInt DataLength() const {return KResponseLength;}

public:
    /** Returned SCSI LOGICAL BLOCK ADDRESS */
    TLba iLba;
    /** Returned SCSI BLOCK LENGTH IN BYTES */
    TUint32 iBlockSize;
    };


/**
Base class for encoding SCSI READ (10) and SCSI WRITE (10) requests
*/
class TScsiClientRdWr10Req: public TScsiClientReq
    {
public:
    TScsiClientRdWr10Req(TOperationCode aOperationCode);
    TInt EncodeRequestL(TDes8& aBuffer) const;

public:
    /** SCSI LOGICAL BLOCK ADDRESS field   */
    TUint32 iLogicalBlockAddress;
    /** SCSI TRANSFER LENGTH field   */
    TUint16 iBlockTransferLength;
    /** SCSI RDPROTECT or SCSI WRPROTECT field */
    TProtect iProtect;
    };


/**
Encode SCSI READ (10) REQUEST
*/
class TScsiClientRead10Req: public TScsiClientRdWr10Req
    {
public:
    TScsiClientRead10Req();
    TInt EncodeRequestL(TDes8& aBuffer) const;
    };



/**
Encode START STOP UNIT REQUEST
Ref SBC-2 5.17
*/
class TScsiClientStartStopUnitReq: public TScsiClientReq
    {
public:
    TScsiClientStartStopUnitReq();
    TInt EncodeRequestL(TDes8& aBuffer) const;

public:
    /** SCSI IMMED flag */
    TBool iImmed;
    /** SCSI START flag */
    TBool iStart;
    /** SCSI LOEJ flag */
    TBool iLoej;
    };


/**
Encode SCSI WRITE (10) REQUEST
*/
class TScsiClientWrite10Req: public TScsiClientRdWr10Req
    {
public:
    TScsiClientWrite10Req();
    TInt EncodeRequestL(TDes8& aBuffer) const;
    };


#include "tscsiblockcmds.inl"

#endif // TSCSIBLOCKCMDS_H
