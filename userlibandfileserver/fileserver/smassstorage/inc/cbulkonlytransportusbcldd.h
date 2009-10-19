/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* Class declaration for CBulkOnlyTransportUsbcLdd.
*
*/


/** 
 @file
 @internalTechnology
*/

#ifndef __CBULKONLYTRANSPORTUSBCLDD_H__
#define __CBULKONLYTRANSPORTUSBCLDD_H__

#include "cbulkonlytransport.h"
#include "cactivedevicestatenotifierbase.h"

// Maximum size for SCSI Read10 Write10 and Verify10 commands
// Windows requests size of 64K whereas MAC requests size of 128K
static const TUint32 KMaxBufSize = 128 * 1024;

//Forward Declaration
class CBulkOnlyTransportUsbcLdd;

/**
Represent session with control endpoint (Ep0).
handles the control interface, and responds to the class specific commands (RESET and GET_MAX_LUN).  
*/
class CControlInterfaceUsbcLdd : public CActive
	{
public:
	enum TControlState
		{
		ENone,
		EReadEp0Data,
		ESendMaxLun
		};

public:
	static CControlInterfaceUsbcLdd* NewL(CBulkOnlyTransportUsbcLdd& aParent);
	~CControlInterfaceUsbcLdd();
	TInt Start();
	void Stop();
	virtual void RunL();
	virtual void DoCancel();

private:
	CControlInterfaceUsbcLdd(CBulkOnlyTransportUsbcLdd& aParent);
	void ConstructL();
	TInt ReadEp0Data();
	void DecodeEp0Data();
	TInt ReadUsbEp0();

private:
	/** Buffer for request data*/
	TBuf8<KRequestHdrSize> iData;

	TUsbRequestHdr iRequestHeader;
	
	/** reference to the  CBulkOnlyTransport*/
	CBulkOnlyTransportUsbcLdd& iParent;

	/** represent carrent state for state mashine */
	TControlState iCurrentState;
	};


/** Transport Class that accessess the Non-SC LDD */
class CBulkOnlyTransportUsbcLdd : public CBulkOnlyTransport, public MLddDeviceStateNotification
	{
public:
	CBulkOnlyTransportUsbcLdd(TInt aNumDrives,CUsbMassStorageController& aController);

	~CBulkOnlyTransportUsbcLdd();
	void ConstructL();

	RDevUsbcClient& Ldd();
	TInt BytesAvailable(); // from Mtransport base class

	TInt SetupConfigurationDescriptor(TBool aUnset = EFalse);
	TInt SetupInterfaceDescriptors();
	void ReleaseInterface();
	void CancelControlInterface();
	TInt StartControlInterface();
	void ActivateDeviceStateNotifier();
	void CancelDeviceStateNotifier();
	void CancelReadWriteRequests();
	void AllocateEndpointResources();
	TInt GetDeviceStatus(TUsbcDeviceState& deviceStatus);
	void FlushData();
	void ReadAndDiscardData(TInt aBytes);
	void ReadCBW();
	void ExpireData(TAny* aAddress = NULL);
	void ProcessCbwEvent();
	void StallEndpointAndWaitForClear();
	void ReadData(TUint aLength = 0);
	void WriteUsb(TRequestStatus& aStatus, TPtrC8& aDes, TUint aLength, TBool aZlpRequired = EFalse);
	void SetCbwPtr();
	TPtr8& SetCommandBufPtr(TUint aLength);
	TPtr8& SetDataBufPtr();
	void SetPaddingBufPtr(TUint aLength);
	void SetCswBufPtr(TUint aLength);
	void ProcessReadingDataEvent();
	void DiscardData(TUint aLength);
	void WriteToClient(TUint aLength);
	void SetReadDataBufPtr(TUint aLength);

#ifdef MSDC_MULTITHREADED
	virtual void GetBufferPointers(TPtr8& aDes1, TPtr8& aDes2);
#endif

    void Activate(TRequestStatus& aStatus, TUint& aValue);
    void Cancel();

private:
	TInt ReadUsb(TUint aLength = 0);

private:
	RDevUsbcClient iLdd;
	CControlInterfaceUsbcLdd* iControlInterface;
	CActiveDeviceStateNotifierBase* iDeviceStateNotifier;
	
	/** buffer for Command Block Wrapper */
	TBuf8 <KCbwLength> iCbwBuf;

	TBuf8<KCommandBufferLength> iCommandBuf; // For Responses to commands 

	TBuf8<KMaxBufSize> iDataBuf1;	// For data transfers (Reading and Writing)

	TBuf8<KMaxBufSize> iDataBuf2;

	TBool iSwap;
	/** internal buffer for CSW */
	TBuf8<KCswLength> iCswBuf;

	/** internal buffer for padding */
	TBuf8<KBOTMaxBufSize> iBuf;

	/** internal buffer for garbage */
	TBuf8<512> iDiscardBuf;
	};

#endif





