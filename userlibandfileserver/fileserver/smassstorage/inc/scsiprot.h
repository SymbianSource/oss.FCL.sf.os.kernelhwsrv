// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __SCSIPROT_H__
#define __SCSIPROT_H__


// Header files
#include "drivemanager.h"
#include "protocol.h"

// Define MSDC_MULTITHREADED to use Mass Storage multi-threaded (Double-buffering) disk read/writes.
// smassstorage_db.mmp defines this macro.

#if defined MSDC_MULTITHREADED 
class CWriteDriveThread;
class CReadDriveThread;
#endif


// Display time taken to write data to disk
//#define MEASURE_AND_DISPLAY_WRITE_TIME
// Display time taken to read data from disk
//#define MEASURE_AND_DISPLAY_READ_TIME


// Maximum size for SCSI Read10 Write10 and Verify10 commands
// Windows requests size of 64K whereas MAC requests size of 128K
static const TUint32 KMaxBufSize = 128 * 1024;

// Write to media when data is available
static const TUint32 KDefaultMediaWriteSize = 4 * 1024;

// Use in the HS case a write size of 64KB
static const TUint32 KHsMediaWriteSize = 64 * 1024;


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
		EAscNull								 = 0x00,
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
		EAscqNull								   = 0x00,
		EAscqLogicalUnitIsInProcessOfBecomingReady = 0x01
		};

public:
	TSenseInfo();

	void SetSense(TSenseCode aSenseCode);

	void SetSense(TSenseCode aSenseCode,
				  TAdditionalCode aAdditional);

	void SetSense(TSenseCode aSenseCode,
				  TAdditionalCode aAdditional,
				  TAdditionalSenseCodeQualifier aQualifier);

	TBool SenseOk();

public:
	TUint8 iSenseCode;
	TUint8 iAdditional;
	TUint8 iQualifier;
	};


/**
Returns EFalse if a sense code has been set. 
Note that ENoSense indicates that there is no specific sense key infotmation
to be reported and the command was successful. 
*/
inline TBool TSenseInfo::SenseOk()
	{
	return (iSenseCode == ENoSense);
	}


const TUint KModeSense6CommandLength = 4;
const TUint KModeSense10CommandLength = 8;
const TUint KReadCapacityCommandLength = 8;
const TUint KReadFormatCapacitiesCommandLength = 12;
const TUint KRequestSenseCommandLength = 18;
const TUint KInquiryCommandLength = 36;


/**
The CScsiProtocol is responsible for interpreting the data received from the Transpor layer
and where appropriate routing specific requests through to the appropriate drive unit.

@internalTechnology
*/
class CScsiProtocol : public CBase, public MProtocolBase
	{
public:
	enum TCommand
	{
	ETestUnitReady			= 0x00,
	ERequestSense			= 0x03,
	EInquiry 				= 0x12,
	EModeSense6				= 0x1A,
	EStartStopUnit			= 0x1B,
	EPreventMediaRemoval	= 0x1E,
	EReadFormatCapacities	= 0x23,
	EReadCapacity			= 0x25,
	ERead10 				= 0x28,
	EWrite10				= 0x2A,
	EVerify10				= 0x2f,
    EModeSense10			= 0x5A,
	EUndefinedCommand		= 0xFF
	};


public:

	static CScsiProtocol* NewL(CDriveManager& aDriveManager);
	void RegisterTransport(MTransportBase* aTransport);
	void ReportHighSpeedDevice();
	TBool DecodePacket(TPtrC8& aData, TUint aLun);
	TInt ReadComplete(TInt aError);
	TInt SetScsiParameters(TMassStorageConfig aConfig);
	TInt Cancel();
	~CScsiProtocol();

#ifdef MSDC_MULTITHREADED
	void InitializeBufferPointers(TPtr8& aDes1, TPtr8& aDes2);
	static void ProcessWriteComplete (TUint8* aAddress, TAny* aPtr); //todo const
#endif

private:
	CScsiProtocol(CDriveManager& aDriveManager);
	void  ConstructL();
	CMassStorageDrive* GetCheckDrive(TUint aLun);
	TBool HandleUnitReady(TUint aLun);
	TBool HandleRequestSense(TPtrC8& aData);
	TBool HandleInquiry(TPtrC8& aData, TUint aLun);
	TBool HandleStartStopUnit(TPtrC8& aData, TUint aLun);
	TBool HandlePreventMediaRemoval(TPtrC8& aData, TUint aLun);
	TBool HandleReadCapacity(TPtrC8& aData, TUint aLun);
	TBool HandleRead10(TPtrC8& aData, TUint aLun);
	TBool HandleWrite10(TPtrC8& aData, TUint aLun);
	TBool HandleVerify10(TPtrC8& aData, TUint aLun);
	TBool HandleModeSense6(TPtrC8& aData, TUint aLun);
    TBool HandleModeSense10(TPtrC8& aData, TUint aLun);
	TBool HandleReadFormatCapacities(TUint aLun);

private:
	/** Configuration data for INQUIRY command*/
	TMassStorageConfig iConfig;

	/** reference to the Drive Manager */
	CDriveManager& iDriveManager;
	
	/** pointer to the transport level */
	MTransportBase* iTransport; 

	/** Sense Info */
	TSenseInfo iSenseInfo;

#ifdef MSDC_MULTITHREADED
	/** Sense Info */
	TSenseInfo iDeferredSenseInfo;
#endif

	/** Start offset (in bytes) for Write/Verify */
	TInt64 iOffset;

	/** Last command for SetupRead (Write or Verify) */
	TUint8 iLastCommand;

	/** LUN for SetupRead */
	TUint iLastLun;

#ifdef SIMDISK
	CArrayFixFlat<TUint8>* iSimDisk;
#endif

	/** The number of bytes remaining to be read from the host for write operations */
	TUint32 iBytesRemain;

	/** Write to the media when this amount of data is available */
	TUint32 iMediaWriteSize;

#ifdef MSDC_MULTITHREADED
	/** Ptr to Write Thread instance */
	CWriteDriveThread* iWriteDriveThread;

	/** Ptr to Read Thread instance */
	CReadDriveThread* iReadDriveThread;
#endif // MSDC_MULTITHREADED

#ifdef USB_TRANSFER_PUBLISHER
	/**
	Publish and subscribe properties for tracking data transfer volume
	*/
	CUsbWriteTransferPublisher* iWriteTransferPublisher;
	CUsbReadTransferPublisher* iReadTransferPublisher;

	/**
	Cumulative bytes read
	*/
	TFixedArray<TInt64, KUsbMsMaxDrives> iBytesRead;
	/**
	Cumulative bytes written
	*/
	TFixedArray<TInt64, KUsbMsMaxDrives> iBytesWritten;
#else
	/**
	Publish and subscribe properties for tracking data transfer volume
	*/
	CDriveWriteTransferPublisher* iWriteTransferPublisher;
	CDriveReadTransferPublisher* iReadTransferPublisher;
#endif
	};

#endif // __SCSIPROT_H__
