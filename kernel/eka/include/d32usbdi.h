// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef D32USBDI_H
#define D32USBDI_H

#ifdef __KERNEL_MODE__
#include <kernel/klib.h>
#else
#include <e32base.h>
#include <d32usbdescriptors.h>
#endif
#include <e32ver.h>
#include <d32usbdi_errors.h>

/**
@file
@publishedPartner
@prototype
Intended to be available to 3rd parties later
*/


class RUsbPipe;
class RUsbTransferDescriptor;
class RUsbTransferStrategy;

typedef TUint32 TUsbBusId;
typedef void* TUsbBus; // handle for an os_bus

/**
Functions which act on a specific interface on a remote device.
*/
class RUsbInterface : public RBusLogicalChannel
	{
public:
	NONSHARABLE_CLASS(TUsbTransferRequestDetails)
		{
	public:
		enum TEp0TransferFlags
			{
			EShortTransferOk	= 0x04,
			};

	public:
		TUint8			iRequestType;
		TUint8			iRequest;
		TUint16			iValue;
		TUint16			iIndex;
		TInt			iFlags;

        // Internal: these should not be set or used, however making them private
        // would require the internal DUsbChannel class be exposed as a friend in
        // userside
		const TDesC8*	iSend;
		TDes8*			iRecv;
		};

	NONSHARABLE_CLASS(TChunkRequestDetails)
		{
	public:
		// In
		TInt iRequestSize;
		//Out
		TInt* iChunkHandle;
		TInt* iOffset;
		};

	NONSHARABLE_CLASS(TTransferMemoryDetails)
		{
	public:
		TInt	iType;
		TUint	iAlignment;
		TInt	iSize;
		TInt	iMaxPackets;
		};

	enum TDeviceSpeed
		{
		ELowSpeed,
		EFullSpeed,
		EHighSpeed
		};
	enum TUsbInterfaceRequest
		{
		ESuspend,
		EEp0Transfer,
		};
	enum TUsbInterfaceControl
		{
		EOpenPipe,
		EPermitRemoteWakeup,
		EGetInterfaceDescriptorSize,
		EGetInterfaceDescriptor,
		EGetStringDescriptor,
		ESelectAlternateInterface,
		EAllocChunk,
		EGetDeviceSpeed,
		EGetBusId,
		EHcdPageSize,
		EGetSizeAndAlignment,
		};
	enum TUsbInterfaceCancel
		{
		ECancelSuspend			= 0x00000001,
		ECancelWaitForResume    = 0x00000002,
		ECancelEp0Transfer		= 0x00000004
		};

public:
	inline static const TDesC& Name();
	inline static TVersion VersionRequired();

#ifndef __KERNEL_MODE__
friend class RUsbPipe;
friend class RUsbZeroCopyTransferStrategy;

public:
	inline RUsbInterface();

	//
	// Standard R-class lifetime controls
	//
	IMPORT_C TInt Open(TUint32 aToken, TOwnerType aType = EOwnerProcess);
	IMPORT_C void Close();

	//
	// Transfer allocation/initialisation functions
	//
	IMPORT_C TInt RegisterTransferDescriptor(RUsbTransferDescriptor& aTransfer);
	IMPORT_C void ResetTransferDescriptors();
	IMPORT_C TInt InitialiseTransferDescriptors();

	//
	// Endpoint Zero transfer utilities
	//
	inline void Ep0Transfer(TUsbTransferRequestDetails& aDetails, const TDesC8& aSend, TDes8& aRecv, TRequestStatus& aRequest);
	inline void CancelEP0Transfer();
	inline TInt GetStringDescriptor(TDes8& aStringDescriptor, TUint8 aIndex, TUint16 aLangId);

	//
	// Suspend/Resume functionality
	//
	inline void PermitSuspendAndWaitForResume(TRequestStatus& aResumeSignal);
	inline void CancelPermitSuspend();
	inline void CancelWaitForResume();
	inline TInt PermitRemoteWakeup(TBool aPermitted);
	
	
	//
	// Descriptor access functions
	//
	inline TInt GetInterfaceDescriptor(TUsbInterfaceDescriptor& aDescriptor);
	inline TInt GetAlternateInterfaceDescriptor(TInt aAlternateInterface, TUsbInterfaceDescriptor& aDescriptor);
	inline TInt GetEndpointDescriptor(TInt aAlternateInterface, TInt aEndpoint, TUsbEndpointDescriptor& aDescriptor);

	// Utility functions to avoid having to parse the entire descriptor tree in simple cases.
	inline TInt GetAlternateInterfaceCount();
	inline TInt EnumerateEndpointsOnInterface(TInt aAlternateInterface);

	//
	// Interface configuration functions
	//
	inline TInt SelectAlternateInterface(TInt aAlternateInterface);
	inline TInt OpenPipeForEndpoint(RUsbPipe& aPipe, TInt aEndpoint, TBool aUseDMA);
	
	//
	// Some utility functions
	//
	inline TInt GetBusId(TUsbBusId& aBusId);
	inline TInt GetHcdPageSize(TInt& aHcdPageSize);
	inline TInt GetDeviceSpeed(TDeviceSpeed& aDeviceSpeed);

private:
	inline TInt AllocateSharedChunk(RChunk& aChunk, TInt aSize, TInt& aOffset);
	TInt RegisterTransferDescriptor(RUsbTransferDescriptor& aTransfer, TUsbBusId aBusId);
	inline TInt GetEndpointDescriptor(TInt aAlternateInterface, TInt aEndpoint, TUsbEndpointDescriptor*& aDescriptor);

private:
	TUsbInterfaceDescriptor* iHeadInterfaceDescriptor;
	HBufC8* iInterfaceDescriptorData;
	
	RUsbTransferStrategy* iTransferStrategy;
	
	TInt iAlternateSetting;
#endif
	};

typedef TUint64 TUsbEndpointId;

/**
Functions which act on an individual pipe established between the local host and a remote device.
*/
class RUsbPipe
	{
public:
	enum TUsbPipeRequest
		{
        EIssueTransfer		= 0x4000000,            // Start from a different value compared to interfaces
                                                    // to allow them to be distinguished while debugging
		};
	enum TUsbPipeControl
		{
		EClose					= 0x4000000,        // Start from a different value compared to interfaces
                                                    // to allow them to be distinguished while debugging
		EAbort,
		EClearRemoteStall,
		EGetEndpointId,
		};

#ifndef __KERNEL_MODE__
friend class RUsbInterface;
friend class RUsbTransferStrategy;
friend class RUsbZeroCopyTransferStrategy;
public:
	inline RUsbPipe();

	inline void Close();
	inline TUint32 Handle() const;

	inline TInt GetEndpointId(TUsbEndpointId& aEndpointId);
	inline TInt GetBusId(TUsbBusId& aBusId);

	inline TInt GetEndpointDescriptor(TUsbEndpointDescriptor& aDescriptor);

	IMPORT_C void Transfer(RUsbTransferDescriptor& aTransfer, TRequestStatus& aRequest);
	inline void CancelAllTransfers();

	inline TInt ClearRemoteStall();
	
private:
	inline void IssueTransfer(TInt aTransferHandle, TRequestStatus& aRequest);

private:
	TUint32 iHandle;
	RUsbInterface* iInterface;
	TUsbEndpointDescriptor* iHeadEndpointDescriptor;
#endif
	};


#include <d32usbdi.inl>

#endif	// D32USBDI_H
