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
// Class declaration for CBulkOnlyTransport.
//
//


/**
 @file
 @internalTechnology
*/

#ifndef CBULKONLYTRANSPORT_H
#define CBULKONLYTRANSPORT_H

#include "botmsctypes.h"
#include "botmscserver.h"

class CUsbMassStorageController;


//---------------------------------------
class CBulkOnlyTransport;  //forward declaration

class CBotControlInterface;


//
// --- class CActiveDeviceStateNotifier -----------------------------
//

class CActiveDeviceStateNotifier : public CActive
	{
public:
	// Construction
	static CActiveDeviceStateNotifier* NewL(CBulkOnlyTransport& aParent);

	// Destruction
	~CActiveDeviceStateNotifier();

	void Activate();

private:
	// Construction
	CActiveDeviceStateNotifier(CBulkOnlyTransport& aParent);
	void ConstructL();

	// Cancel request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();

	// Service completed request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();

private:
	CBulkOnlyTransport& iParent;
	TUint iDeviceState;
	TUint iOldDeviceState;
	};
//=====================



//----------------------
/** handles the data transport and communications with the SCSI protocol */
class CBulkOnlyTransport : public CActive, public MDeviceTransport
	{
public:
	enum TTransportState
		{
		ENone,
		EWaitForCBW,
		ESendingCSW,
		EHandleDataIn,
		EHandleDataOut,
        EPermErr
		};

    /** size of buffer for command padding or data flushing */
    static const TUint KBOTMaxBufSize = 0x200;

    //This value defined in USB Mass Storage Bulk Only Transrt spec and not supposed to be changed
    static const TInt KRequiredNumberOfEndpoints = 2; // in addition to endpoint 0.
    static const TInt KUsbNumInterfacesOffset = 4;

public:
	static CBulkOnlyTransport* NewL(TInt aNumDrives,
                                    CUsbMassStorageController& aController);

#ifdef MSDC_TESTMODE
	static CBulkOnlyTransport* NewL(TInt aNumDrives,
                                    CUsbMassStorageController& aController,
                                    TTestParser* aTestParser);
#endif
	~CBulkOnlyTransport();

private:
	CBulkOnlyTransport(TInt aNumDrives,
                       CUsbMassStorageController& aController);

	CBulkOnlyTransport(TInt aNumDrives,
                       CUsbMassStorageController& aController,
                       TTestParser* aTestParser);
	void ConstructL();

public:
	void SetupDataOut(TPtr8& aData);
	void SetupDataIn(TPtrC8& aData);

	TInt Start();
	TInt Stop();
	void RegisterProtocol(MServerProtocol& aProtocol);
	TInt BytesAvailable();

	CUsbMassStorageController& Controller();
	TInt MaxLun();
	RDevUsbcClient& Ldd();
	TInt HwStart(TBool aDiscard = EFalse);
	void HwStop();
	void HwSuspend();
	void HwResume();

	virtual void RunL();
	virtual void DoCancel();

private:
	TInt SetupConfigurationDescriptor(TBool aUnset = EFalse);
	TInt SetupInterfaceDescriptors();
    void ClientReadCbw();
	void ReadCbw();
	void DecodeCbwL();
	void SetPermError();
	void SendCsw(TUint aTag, TUint32 aDataResidue, TBotCsw::TCswStatus aStatus);
	void DataInWriteRequest(TUint aLength, TBool aZlpRequired = EFalse);
	void DataOutReadRequest(TUint aLength);
	void StallEndpointAndWaitForClear(TEndpointNumber aEndpoint);
	void Activate(TInt aReason);

    void FlushDataIn(TInt aLength);
    void FlushDataOut(TInt aLength);

private:
	/** maximun logic unit number supported (started from 0*/
	TInt iMaxLun;

	CUsbMassStorageController& iController;

	CBotControlInterface* iBotControlInterface;
	MServerProtocol* iProtocol;
	RDevUsbcClient iLdd;
	TTransportState iCurrentState;

	/** buffer for Command Block Wrapper */
	TBuf8 <TBotCbw::KCbwLength> iCbwBuf;
    TBuf8 <TBotCsw::KCswLength> iCsw;

    TDataTransferMan iDataTransferMan;

	TUint32 iCbwTag;
	TBotCsw::TCswStatus iCmdStatus;

	/** Indicate whether SCSI prot started or not */
	TBool iStarted;

    /** internal buffer for padding or flushing */
	TBuf8<KBOTMaxBufSize> iBuf;
	TBool iStallAllowed;

	CActiveDeviceStateNotifier* iDeviceStateNotifier;
	TBool iInterfaceConfigured;
#ifdef MSDC_TESTMODE
    TTestParser* iTestParser;
#endif
	};

#endif // CBULKONLYTRANSPORT_H
