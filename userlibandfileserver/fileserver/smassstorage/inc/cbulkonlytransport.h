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
// Class declaration for CBulkOnlyTransport.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __CBULKONLYTRANSPORT_H__
#define __CBULKONLYTRANSPORT_H__

#include <e32std.h>
#if !defined(__WINS__) && !defined(__X86__) 
#include <d32usbcsc.h>
#endif
#include <d32usbc.h>
#include "protocol.h"  
#include "cusbmassstoragecontroller.h"
#include "mldddevicestatenotification.h"

static const TUint KCbwLength = 31;
static const TUint KCommandBufferLength = 36;
// for control endpoint 
static const TUint KRequestHdrSize = 8;


/** size of buffer for command padding */
static const TUint KBOTMaxBufSize 		= 512; 

/** size of csw */
LOCAL_D const TInt KCswLength = 13;


/**
Represent Endpoint0 request
*/
class TUsbRequestHdr
	{
public:
	enum TEp0Request
		{
		EReqGetMaxLun = 0xFE,
		EReqReset	  = 0xFF
		};
public:
	TInt Decode(const TDesC8& aBuffer);
	TBool IsDataResponseRequired() const;

public:
	TUint8 iRequestType;
	TEp0Request iRequest;
	TUint16 iValue;
	TUint16 iIndex;
	TUint16 iLength;
	};


/** handles the data transport and communications with the SCSI protocol */
class CBulkOnlyTransport : public CActive, public MTransportBase
	{
public:
	enum TCswStatus
		{
		ECommandPassed	= 0,
		ECommandFailed	= 1,
		EPhaseError		= 2
		};

	enum TTransportState
		{
		ENone,
		EWaitForCBW,
		ESendingCSW,
		EWritingData,
		EReadingData,
        EPermErr
		};
public:
	static CBulkOnlyTransport* NewL(TInt aNumDrives,CUsbMassStorageController& aController, CUsbMassStorageController::TTransportldd aTransportLddFlag);
	static CBulkOnlyTransport* NewL(TInt aNumDrives,CUsbMassStorageController& aController); 

protected:
    ~CBulkOnlyTransport();
	CBulkOnlyTransport(TInt aNumDrives,CUsbMassStorageController& aController);

public:
	TInt InitialiseTransportL(TInt aTransportLddFlag);


	TInt Start();
	TInt Stop();
	void RegisterProtocol(MProtocolBase& aProtocol);

	CUsbMassStorageController& Controller();
	TInt MaxLun();
	void SetupReadData(TUint aLength);
	void SetupWriteData(TPtrC8& aData);

	void GetCommandBufPtr(TPtr8& aDes, TUint aLength); // Ptr to iCommandBuf to send responses to commands 
	void GetReadDataBufPtr(TPtr8& aDes); // Ptr to DataBuf's  
	void GetWriteDataBufPtr(TPtrC8& aDes);
#ifdef MSDC_MULTITHREADED
	void ProcessReadData(TAny* aAddress);
#endif

	TInt HwStart(TBool aDiscard = EFalse);
	TInt HwStop();
    void StopBulkOnlyEndpoint();
	TInt HwSuspend();
	TInt HwResume();

	virtual void RunL();
	virtual void DoCancel();

	virtual TInt SetupConfigurationDescriptor(TBool aUnset = EFalse) = 0;
	virtual TInt SetupInterfaceDescriptors() = 0;
	virtual void ReleaseInterface() = 0;
	virtual void CancelControlInterface() = 0;
	virtual TInt StartControlInterface() = 0;
	virtual void ActivateDeviceStateNotifier() = 0;
	virtual void CancelDeviceStateNotifier() = 0;
	virtual void CancelReadWriteRequests() = 0;
	virtual void AllocateEndpointResources() = 0;
	virtual TInt GetDeviceStatus(TUsbcDeviceState& deviceStatus) = 0;
	virtual void FlushData() = 0;
	virtual void ReadAndDiscardData(TInt aBytes) = 0;
	virtual void ReadCBW() = 0;
	virtual void ExpireData(TAny* aAddress = NULL) = 0;
	virtual void ProcessCbwEvent() = 0;
	virtual void StallEndpointAndWaitForClear() = 0;
	virtual void ReadData(TUint aLength = 0) = 0;
	virtual void WriteUsb(TRequestStatus& aStatus,  TPtrC8& aDes, TUint aLength, TBool aZlpRequired = EFalse) = 0;
	virtual void SetCbwPtr() = 0;
	virtual TPtr8& SetCommandBufPtr(TUint aLength) = 0; // pointer to buf for sending responses to commands 
	virtual TPtr8& SetDataBufPtr() = 0; // to swap between the two buffers
	virtual void SetPaddingBufPtr(TUint aLength) = 0;
	virtual void SetCswBufPtr(TUint aLength) = 0;
	virtual void ProcessReadingDataEvent() = 0;
	virtual void DiscardData(TUint aLength) = 0;
	virtual void WriteToClient(TUint aLength) = 0;
	virtual void SetReadDataBufPtr( TUint aLength) = 0;

#ifdef MSDC_MULTITHREADED
	virtual void GetBufferPointers(TPtr8& aDes1, TPtr8& aDes2) = 0;
#endif

protected:
	void DecodeCBW();
	TBool CheckCBW();
	void SetPermError();
	void SendCSW(TUint aTag, TUint aDataResidue, TCswStatus aStatus);
	void WriteData(TRequestStatus& aStatus,  TPtrC8& aDes, TUint aLength, TBool aZlpRequired = EFalse);

	void CallReadAndDiscardData(TInt aBytes);
	void Activate(TInt aReason);

protected:
	/** maximun logic unit number supported (started from 0*/
	TInt iMaxLun;

	CUsbMassStorageController& iController;

	MProtocolBase* iProtocol;

	TTransportState  iCurrentState;

	/** Shows how much data was not send/received */
	TUint32 iDataResidue;
	TUint32 iCbwTag;
	TCswStatus iCmdStatus;

	/** Indicate if SCSI prot has data to sent */
	TBool iWriteSetUp;

	/** Indicate if SCSI prot expected additional data */
	TBool iReadSetUp;

	/** Indicate whether SCSI prot started or not */
	TBool iStarted;

	TBool iStallAllowed;

	TBool iInterfaceConfigured;

	TPtr8 iCommandBufPtr; // Ptr to buffer to write responses to commands
	TPtr8 iDataBufPtr;
	TPtr8 iCswBufPtr;
	TPtr8 iPaddingBufPtr;

	/** Size of data, Used to tell transport how much protocol/media has to send */
	TUint iBufSize; 

	TPtrC8 iWriteBufPtr;

	TPtr8 iReadBufPtr;

	/** Internal TPtr to check validity of and decode CBW */
	TPtrC8 iCbwBufPtr;
	};


#endif // __CBULKONLYTRANSPORT_H__


