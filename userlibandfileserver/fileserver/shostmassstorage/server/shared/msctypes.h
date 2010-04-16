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
//

/**
 @file
 @internalTechnology
*/

#ifndef MSCTYPES_H
#define MSCTYPES_H

/** Device Token */
typedef TUint32 TToken;
/** Device Logical Unit Number */
typedef TUint TLun;
/** RD/WR Logical Block Address */
typedef TUint32 TLba;
/** RD/WR Position */
typedef TUint64 TPos;

/** Sub Class Code, USB Mass Storage Class Overview Spec - Section 2 */
enum TProtocolType
    {
	ScsiProtocol = 0x06
    };


/** Protocol Code, USB Mass Storage Class Overview Spec - Section 3 */
enum TTransportType
    {
	BulkOnlyTransport = 0x50
    };


/** Transport layer error code */
const TInt KErrCommandFailed = 0x100;
/** Transport layer error code */
const TInt KErrCommandStalled = 0x101;


/**
Class to represent the SCSI Sense Information returned by SCSI REQUEST SENSE
REQUEST
Ref SPC-2 7.20
Ref SPC-3 6.27
*/
class TSenseInfo
	{
public:

    /**
    Sense key and sense code defintions, Ref SPC-3 Section 4.5.6 and Table 27
    */
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

    /**
    ASC assignments, SPC-3 Table 28
    */
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

    /**
    ASQ assignments, SPC-3 Table 28
    */
	enum TAdditionalSenseCodeQualifier
		{
		EAscqLogicalUnitIsInProcessOfBecomingReady = 0x01,
        EAscqInitializingCommandRequired = 0x02
		};

    TSenseInfo() {}

public:
    /** SCSI SENSE CODE, SPC-3 4.5.2 */
	TUint8 iSenseCode;
    /** SCSI ADDITIONAL SENSE CODE, SPC-3 4.5.2 */
	TUint8 iAdditional;
    /** SCSI ADDITIONAL SENSE CODE QUALIFIER, SPC-3 4.5.2 */
	TUint8 iQualifier;
	};

#endif // MSCTYPES_H

