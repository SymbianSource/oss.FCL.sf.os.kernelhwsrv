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

#ifndef TSCSISERVERCMDS_H
#define TSCSISERVERCMDS_H

class TMassStorageConfig;

/**
Sense Info
*/
class TSenseInfo
	{
public:
	// Spec: SCSI Primary Commands 3 (SPC-3)
	// Section 4.5.6 Sense key and sense code defintions
	// Table 27 - Sense key descriptions
	enum TSenseCode
		{
		ENoSense        = 0,
		ERecoveredError = 1,
		ENotReady       = 2,
		EMediumError    = 3,
		EHardwareError  = 4,
		EIllegalRequest = 5,
		EUnitAttention  = 6,
		EDataProtection = 7,
		EBlankCheck     = 8,
		EVendorSpecific = 9,
		ECopyAborted    = 10,
		EAbortedCommand = 11,
		EDataOverflow   = 13,
		EMisCompare     = 14
		};

	// Table 28 - ASC and ASQ assignments
	enum TAdditionalCode
		{
		EAscLogicalUnitNotReady					 = 0x04,
		EAscLogicalUnitDoesNotRespondToSelection = 0x05,
		EInvalidCmdCode							 = 0x20,
		ELbaOutOfRange							 = 0x21,
		EInvalidFieldInCdb						 = 0x24,
		ELuNotSupported							 = 0x25,
		EWriteProtected							 = 0x27,
		ENotReadyToReadyChange					 = 0x28,
		EMediaNotPresent						 = 0x3A,
		EInsufficientRes						 = 0x55
		};

	enum TAdditionalSenseCodeQualifier
		{
		EAscqLogicalUnitIsInProcessOfBecomingReady = 0x1
		};

    TSenseInfo() {}

public:
	TUint8 iSenseCode;
	TUint8 iAdditional;
	TUint8 iQualifier;
	};



class TSrvSenseInfo: public TSenseInfo
    {
public:
	TSrvSenseInfo();
	void SetSense(TSenseCode aSenseCode);

	void SetSense(TSenseCode aSenseCode,
                  TAdditionalCode aAdditional);

	void SetSense(TSenseCode aSenseCode,
                  TAdditionalCode aAdditional,
                  TUint8 aQualifier);

	TBool SenseOk();
	};



/**
TEST UNIT READY
SPC-2 7.25
SPC-3 6.33
 */
class TScsiServerTestUnitReadyReq: public TScsiServerReq
    {
public:
    TScsiServerTestUnitReadyReq() {};
    void DecodeL(const TDesC8& aPtr);
    };


/**
REQUEST SENSE
SPC-2 7.20
SPC-3 6.27
 */
class TScsiServerRequestSenseReq: public TScsiServerReq
    {
public:
    TScsiServerRequestSenseReq() {};
    TAllocationLength AllocationLength() const;

    void DecodeL(const TDesC8& aPtr);
public:
    TAllocationLength iAllocationLength;
    };

/**

 */
class TScsiServerRequestSenseResp: public MScsiServerResp
    {
public:
    enum TResponseCode
        {
        ECurrentErrors = 0x70,
        EDeferredErrors = 0x71
        };

    static const TUint KCommandLength = 18;

    TScsiServerRequestSenseResp() {};

    void Encode(TDes8& aBuffer) const;
    void SetResponseCode(TResponseCode aResponseCode) {iResponseCode = aResponseCode;}

    void SetAdditionalSenseLength(TUint8 aAdditionalSenseLength);

public:
    TResponseCode iResponseCode;
    TSrvSenseInfo* iSensePtr;

    TUint iAdditionalSenseLength;

    TAllocationLength iAllocationLength;
    };



/**
INQUIRY
SPC-2 7.3.1
SPC-3 6.4
 */
class TScsiServerInquiryReq: public TScsiServerReq
    {
public:
    TScsiServerInquiryReq() {};

    void DecodeL(const TDesC8& aPtr);

public:
	TBool iCmdDt;
	TBool iEvpd;
	TUint8 iPage;
    TAllocationLength iAllocationLength;
    };

/**


 */
class TScsiServerInquiryResp: public MScsiServerResp
    {
public:
    static const TUint KResponseLength = 36;

    TScsiServerInquiryResp(const TMassStorageConfig& aConfig);
    void Encode(TDes8& aBuffer) const;

    //void SetConfig(const TMassStorageConfig& aConfig) {iConfig = aConfig;}
    void SetNotRemovable() {iRemovable = EFalse;}
    void SetAllocationLength(TAllocationLength aAllocationLength) {iAllocationLength = aAllocationLength;}

    TUint Length() {return iAllocationLength;}

private:
    TBool iRemovable;
    TInt iResponseDataFormat;
    const TMassStorageConfig& iConfig;
    TAllocationLength iAllocationLength;
    };


/**
MODE SENSE(6)
 */
class TScsiServerModeSense6Req: public TScsiServerReq
    {
public:
    enum TPageControl
    {
        ECurrentValues = 0x0,
        EChangeableValues = 0x1,
        EDefaultValues = 0x2,
        ESavedValues = 0x3
    };

    static const TUint8 KAllPages = 0x3F;

    TScsiServerModeSense6Req() {};
    virtual void DecodeL(const TDesC8& aPtr);

public:
    TUint iPageCode;
    TUint iPageControl;

    TAllocationLength iAllocationLength;
    };
/**


 */
class TScsiServerModeSense6Resp: public MScsiServerResp
    {
public:
    static const TUint KCommandLength = 4;

    TScsiServerModeSense6Resp();
    void SetWp(TBool aWp);
    void Encode(TDes8& aBuffer) const;

    void SetAllocationLength(TAllocationLength aAllocationLength) {iAllocationLength = aAllocationLength;}
private:
    TBool iWp;
    TAllocationLength iAllocationLength;
    };

// **** START STOP UNIT ****
/**


 */
class TScsiServerStartStopUnitReq: public TScsiServerReq
    {
public:
    TScsiServerStartStopUnitReq() {};
    virtual void DecodeL(const TDesC8& aPtr);

public:
    TBool iImmed;
    TBool iStart;
    TBool iLoej;
    };


/**
PREVENT MEDIA REMOVAL
 */
class TScsiServerPreventMediaRemovalReq: public TScsiServerReq
    {
public:

    TScsiServerPreventMediaRemovalReq() {};
    virtual void DecodeL(const TDesC8& aPtr);

public:
    TPrevent iPrevent;
    };
// **** READ FORMAT CAPCITIES ****
/**


 */
class TScsiServerReadFormatCapacitiesReq: public TScsiServerReq
    {
public:
    TScsiServerReadFormatCapacitiesReq() {};
    virtual void DecodeL(const TDesC8& aPtr);
    TAllocationLength AllocationLength() const {return iAllocationLength;}
private:
    TAllocationLength iAllocationLength;
    };

/**


 */
class TScsiServerReadFormatCapacitiesResp: public MScsiServerResp
    {
public:
    static const TUint KResponseLength = 12;
    TScsiServerReadFormatCapacitiesResp(TAllocationLength aAllocationLength);
    void SetNumberBlocks(TUint32 aNumberBlocks) {iNumberBlocks = aNumberBlocks;}
    void Encode(TDes8& aBuffer) const;
private:
    TAllocationLength iAllocationLength;
    TUint32 iNumberBlocks;
    };


/**
READ CAPCAITY (10)
 */
class TScsiServerReadCapacity10Req: public TScsiServerReq
    {
public:
    TScsiServerReadCapacity10Req() {};
    virtual void DecodeL(const TDesC8& aPtr);

public:
	TBool iPmi;
	TUint32 iLogicalBlockAddress;
    };

/**


 */
class TScsiServerReadCapacity10Resp: public MScsiServerResp
    {
public:
    static const TUint KCommandLength = 8;
    TScsiServerReadCapacity10Resp() {};
    void Set(TUint aBlockSize, const TInt64& aNumberBlocks);
    void Encode(TDes8& aBuffer) const;

private:
    TInt64 iNumberBlocks;
    TUint iBlockSize;
    };

/**


 */
class TScsiServerRdWr10Req: public TScsiServerReq
    {
public:
    TScsiServerRdWr10Req() {};
    virtual void DecodeL(const TDesC8& aDes);

public:
    TLba iLogicalBlockAddress;
    TBlockTransferLength iTransferLength;
    TProtect iProtect;
    };


/**
READ (10)
 */
class TScsiServerRead10Req: public TScsiServerRdWr10Req
    {
public:
    TScsiServerRead10Req() {};

    void SetRdProtect(TProtect aProtect);
    TProtect RdProtect();
    };


/**
WRITE (10)
 */
class TScsiServerWrite10Req: public TScsiServerRdWr10Req
    {
public:
    TScsiServerWrite10Req() {};

    void SetWrProtect(TProtect aProtect);
    TProtect WrProtect();
    };


/**
VERIFY (10)
 */
class TScsiServerVerify10Req: public TScsiServerRdWr10Req
    {
public:
    TScsiServerVerify10Req() {};
    virtual void DecodeL(const TDesC8& aPtr);
public:
    TBool iBytchk;
    };

#include "tscsiservercmds.inl"

#endif // TSCSISERVERCMDS_H
