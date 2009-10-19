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
* Class declaration for CBulkOnlyTransportUsbcScLdd.
*
*/


/** 
 @file
 @internalTechnology
*/

#ifndef __CBULKONLYTRANSPORTUSBCSCLDD_H__
#define __CBULKONLYTRANSPORTUSBCSCLDD_H__

#include "cbulkonlytransport.h"
#include "cactivedevicestatenotifierbase.h"

// This the length of every buffer corresponding to each Bulk endpoint. 
// Length is to support double buffering, maximum size of host transfers(64 for Windows/128 for MAC) + 2K (to send CSW(IN ep's) and to recwive CBW's(OUT ep's))
LOCAL_D const TUint KMaxScBufferSize = 258 * 1024; 
LOCAL_D const TUint KMaxScReadSize = 64 * 1024;


//Forward Declaration
class CBulkOnlyTransportUsbcScLdd;

/**
Represent session with control endpoint (Ep0).
handles the control interface, and responds to the class specific commands (RESET and GET_MAX_LUN).  
*/

class CControlInterfaceUsbcScLdd : public CActive
	{
public:
	enum TControlState
		{
		ENone,
		EReadEp0Data,
		ESendMaxLun
		};

public:
	static CControlInterfaceUsbcScLdd* NewL(CBulkOnlyTransportUsbcScLdd& aParent);
	~CControlInterfaceUsbcScLdd();
	TInt Start();
	void Stop();
	virtual void RunL();
	virtual void DoCancel();
	TInt OpenEp0();

private:
	CControlInterfaceUsbcScLdd(CBulkOnlyTransportUsbcScLdd& aParent);
	void ConstructL();
	TInt ReadEp0Data();
	void DecodeEp0Data();
	TInt ReadUsbEp0();

private:
	TEndpointBuffer iEp0Buf;
	TAny* iEp0Packet;
	TUint iEp0Size;
	TBool iEp0Zlp;

	TUsbRequestHdr iRequestHeader;

	/** reference to the  CBulkOnlyTransport*/
	CBulkOnlyTransportUsbcScLdd& iParent;

	/** represent carrent state for state mashine */
	TControlState iCurrentState;
	};


/** Transport Class that accessess the SC LDD */
class CBulkOnlyTransportUsbcScLdd : public CBulkOnlyTransport, public MLddDeviceStateNotification
	{
public:
	CBulkOnlyTransportUsbcScLdd(TInt aNumDrives,CUsbMassStorageController& aController);

	~CBulkOnlyTransportUsbcScLdd();
	void ConstructL();

	RDevUsbcScClient& Ldd();
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
	void SetReadDataBufPtr( TUint aLength);

#ifdef MSDC_MULTITHREADED
	virtual void GetBufferPointers(TPtr8& aDes1, TPtr8& aDes2);
#endif

    void Activate(TRequestStatus& aStatus, TUint& aValue);
    void Cancel();

private:
	TInt ReadUsb(TUint aLength = 0);
	TInt OpenEndpoints();
	void ProcessDataFromHost(); // As USB Read API can return with KErrCompletion saying there is data already and 
								// that we can process the data without queuong a request, this function effectively does
								// what Runl() did earlier

private:
	RChunk* iChunk;
	RDevUsbcScClient iLdd;
	CControlInterfaceUsbcScLdd* iControlInterface;
	CActiveDeviceStateNotifierBase* iDeviceStateNotifier;

	/** To remember chunk specifics one defined, instead of walking it every time something required */
	TUint iInEndpoint;
	TUint iOutEndpoint;
	TEndpointBuffer iSCReadEndpointBuf;
	TEndpointBuffer iSCWriteEndpointBuf;
	
	/** Pointer to the data in chunk which is read from the host */
	TAny* iSCReadData;

	/** Size of data read from the host in a 'transfer' */
	TUint iSCReadSize;

	/** If data read from host was termintated with a ZLP or not */
	TBool iReadZlp;

	/** Pointer to start of IN buffer in chunk which can be written into by protocol/media */
	TAny* iDataPtr; 

	/** Length of IN buffer */
	TUint iInBufferLength;
	};

#endif





