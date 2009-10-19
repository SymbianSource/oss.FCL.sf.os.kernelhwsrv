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

#ifndef CSCSISERVERPROTOCOL_H
#define CSCSISERVERPROTOCOL_H

class MDeviceTransport;
class CDriveManager;
class CMassStorageDrive;
class CUsbWriteTransferPublisher;
class CUsbReadTransferPublisher;

class TTestParser;


// Display time taken to write data to disk
//#define MEASURE_AND_DISPLAY_WRITE_TIME
// Display time taken to read data from disk
//#define MEASURE_AND_DISPLAY_READ_TIME


// Maximum size for SCSI Read10 Write10 and Verify10 commands
// Windows requests size of 64K whereas MAC requests size of 128K
static const TUint32 KMaxBufSize = 128 * 1024;

class TMediaWriteMan
    {
public:

    static const TLun KUndefinedLun = 0xFFFF;

    // Write to media when data is available
    static const TUint32 KDefaultMediaWriteSize = 4 * 1024;

    // Use in the HS case a write size of 64KB
    static const TUint32 KHsMediaWriteSize = 64 * 1024;

    TMediaWriteMan();

    void ReportHighSpeedDevice();

    TInt64 Start(TUint32 aLba, TUint32 aLength, TUint32 aBlockSize);
    TUint32 NextPacket();
    void Reset();
    void SetOffset(const TInt64& aOffset, TUint aLength);

    void SetBytesRemain(TUint32 aLength) {iBytesRemain = aLength;}
    TUint32 BytesRemain() const {return iBytesRemain;}

    TUint32 GetPacketLength() const;
    TUint32 GetLength() const;

    TBool Active() const {return iActive;}
    TInt64 Offset() const {return iOffset;}

private:
    /** Write is in progress */
    TBool iActive;

    /** Start offset (in bytes) for Write */
    TInt64 iOffset;

    /** The number of bytes remaining to be read from the host for write operations */
    TUint32 iBytesRemain;

    /** Write to the media when this amount of data is available */
    TUint32 iMediaWriteSize;
    };


/**
The CScsiServerProtocol is responsible for interpreting the data received from
the Transpor layer and where appropriate routing specific requests through to
the appropriate drive unit.

@internalTechnology
*/
class CScsiServerProtocol : public CBase, public MServerProtocol
	{
public:
    static const TUint KInquiryCommandLength = 36;
    static const TUint KCommandBufferLength = KInquiryCommandLength;

public:
	static CScsiServerProtocol* NewL(CDriveManager& aDriveManager);
#ifdef MSDC_TESTMODE
	static CScsiServerProtocol* NewL(CDriveManager& aDriveManager, TTestParser* aTestParser);
#endif
	~CScsiServerProtocol();

private:
	CScsiServerProtocol(CDriveManager& aDriveManager);
	CScsiServerProtocol(CDriveManager& aDriveManager, TTestParser* aTestParser);
	void  ConstructL();

public:
	void RegisterTransport(MDeviceTransport* aTransport);
	void ReportHighSpeedDevice();
	TBool DecodePacket(TPtrC8& aData, TUint8 aLun);
	TInt MediaWritePacket(TUint& aBytesWritten);
    void MediaWriteAbort();
	void SetParameters(const TMassStorageConfig& aConfig);
	TInt Cancel();

private:
	CMassStorageDrive* GetCheckDrive();
	TBool HandleUnitReady();
	TBool HandleRequestSense(const TScsiServerReq& aRequest);
	TBool HandleInquiry(const TScsiServerReq& aRequest);
	TBool HandleStartStopUnit(const TScsiServerReq& aRequest);
	TBool HandlePreventMediaRemoval(const TScsiServerReq& aRequest);
	TBool HandleReadCapacity10(const TScsiServerReq& aRequest);
	TBool HandleRead10(const TScsiServerReq& aRequest);
	TBool HandleWrite10(const TScsiServerReq& aRequest);
	TBool HandleModeSense6(const TScsiServerReq& aRequest);
	TBool HandleReadFormatCapacities(const TScsiServerReq& aRequest);

private:
	/** Configuration data for INQUIRY command*/
	TMassStorageConfig iConfig;

	/** reference to the Drive Manager */
	CDriveManager& iDriveManager;

	/** pointer to the transport level */
	MDeviceTransport* iTransport;

	/** Sense Info */
	TSrvSenseInfo iSenseInfo;

    TBuf8<KCommandBufferLength> iCommandBuf;
    /** General buffer for read/write operations */
    //TBuf8<KMaxBufSize> iDataBuf;
    RBuf8 iDataBuf;

    TMediaWriteMan iMediaWriteMan;

    /** LUN for SetupRead */
    TLun iLun;

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

#ifdef MSDC_TESTMODE
    TTestParser* iTestParser;
#endif
	};

#endif // CSCSISERVERPROTOCOL_H
