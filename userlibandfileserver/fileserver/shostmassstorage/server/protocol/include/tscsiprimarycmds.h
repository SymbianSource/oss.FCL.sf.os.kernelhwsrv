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

#ifndef TSCSIPRIMARYCMDS_H
#define TSCSIPRIMARYCMDS_H


/**
Encodes SCSI INQUIRY REQUEST
Ref SPC-2 7.3.1
Ref SPC-3 6.4
*/
class TScsiClientInquiryReq: public TScsiClientReq
    {
public:
    /** Length of INQUIRY response */
    static const TUint KResponseLength = 36;

    TScsiClientInquiryReq();
    TInt EncodeRequestL(TDes8& aBuffer) const;

    void SetEvpd(TUint8 aPageCode);
    void SetCmdDt(TUint8 aOperationCode);
    void SetAllocationLength(TAllocationLength aAllocationLength);

public:
    /** SCSI CmdDt field */
    TBool iCmdDt;
    /** SCSI EVPD field */
	TBool iEvpd;
    /** SCSI PAGE field */
	TUint8 iPage;
    /** SCSI ALLOCATION LENGTH */
    TAllocationLength iAllocationLength;
    };


/**
Represents the SCSI INQUIRY RESPONSE PRODUCT IDENTIFICATION information
*/
class TMassStorageConfig
    {
public:
    /** SCSI VENDOR ID field */
    TBuf<8> iVendorId;
    /** SCSI PRODUCT ID field */
    TBuf<16> iProductId;
    /** SCSI PRODUCT REVISION field */
    TBuf<4> iProductRev;
    };

/**
Represents the SCSI INQUIRY RESPONSE PERIPHERAL device information
*/
class TPeripheralInfo
    {
public:
    /** Medium removable flag */
    TBool iRemovable;

    /** SCSI PERIPHERAL QUALIFIER */
    TUint8 iPeripheralQualifier;
    /** SCSI PERIPHEREAL DEVICE TYPE */
    TUint8 iPeripheralDeviceType;
    /** SCSI RESPONSE DATA FORMAT */
    TUint8 iResponseDataFormat;
    /** SCSI VERSION */
    TUint8 iVersion;
    /** Device product information (vendor id, product id, revision) */
    TMassStorageConfig iIdentification;
    };

/**
Decodes SCSI INQUIRY REQUEST
*/
class TScsiClientInquiryResp: public TScsiClientResp
    {
public:
    /** Length of INQUIRY response */
    static const TInt KResponseLength = 36;
    TScsiClientInquiryResp(TPeripheralInfo& aPeripheralInfo);

    TInt DataLength() const {return KResponseLength;}
    void DecodeL(const TDesC8& aPtr);

private:
    void DecodeInquiry(const TDesC8& aPtr);

public:
    /** Device peripheral info fields */
    TPeripheralInfo& iPeripheralInfo;
    };

/**
Data type for PREVENT field
SPC-2, 7.12 Table 78
SPC-3, 6.13 Table 119
*/
typedef TBool TPrevent;

/**
Encodes SCSI PREVENT MEDIA REMOVAL REQUEST
SPC-2 7.12
SPC-3 6.13
*/
class TScsiClientPreventMediaRemovalReq: public TScsiClientReq
    {
public:
    TScsiClientPreventMediaRemovalReq(TPrevent aPrevent);
    TInt EncodeRequestL(TDes8& aBuffer) const;

private:
    TPrevent iPrevent;
    };


/**
Encodes SCSI REQUEST SENSE REQUEST
*/
class TScsiClientRequestSenseReq: public TScsiClientReq
    {
public:
    /** Length of REQUEST SENSE request */
    static const TUint KResponseLength = 18;
    TScsiClientRequestSenseReq();
    TInt EncodeRequestL(TDes8& aBuffer) const;

private:
    void SetAllocationLength(TAllocationLength aAllocationLength);

public:
    /** SCSI ALLOCATION LENGTH field */
    TAllocationLength iAllocationLength;
    };


/**
Decodes SCSI SENSE RESPONSE
*/
class TScsiClientRequestSenseResp: public TScsiClientResp
    {
public:
    /** SCSI Response Code, SBC-3 4.5 */
    enum TResponseCode
        {
        ECurrentErrors = 0x70,
        EDeferredErrors = 0x71
        };

    /** Length of the REQUEST SENSE response */
    static const TInt KResponseLength = 18;
    /** Constructor */
    TScsiClientRequestSenseResp() {};

    TInt DataLength() const {return KResponseLength;}
    void DecodeL(const TDesC8& aPtr);

private:
    void DecodeSenseInfo(const TDesC8& aPtr);

public:
    /** Returned SCSI RESPONSE CODE */
    TResponseCode iResponseCode;
    /** Returned SCSI Sense Info */
    TSenseInfo iSenseInfo;
    /** Returned SCSI ADDITIONAL SENSE LENGTH */
    TUint iAdditionalSenseLength;
    };


/**
Encodes SCSI TEST UNIT READY REQUEST
Ref SPC-2 7.25
Ref SPC-3 6.33
*/
class TScsiClientTestUnitReadyReq: public TScsiClientReq
    {
public:
    TScsiClientTestUnitReadyReq();
    TInt EncodeRequestL(TDes8& aBuffer) const;
    };


#include "tscsiprimarycmds.inl"

#endif // TSCSIPRIMARYCMDS_H
