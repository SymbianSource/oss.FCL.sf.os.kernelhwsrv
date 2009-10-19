#ifndef __CONTROL_TRANSFER_REQUESTS_H
#define __CONTROL_TRANSFER_REQUESTS_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file ControlTransferRequests.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <e32ver.h>
#include <d32usbdi.h>

namespace NUnitTesting_USBDI
	{

//These values MUST be kept in line with each other
const _LIT8(KNumberFormatString, "%08d");
const TUint KNumberStringLength = 8;

//These values MUST be kept in line with each other
const _LIT8(KTwoNumberFormatString, "%08d%08d");
const TUint KTwoNumberStringLength = 16;

//These values MUST be kept in line with each other
const _LIT8(KSplitWriteNumberFormatString, "%08d%08d%08d");
const TUint KSplitWriteNumberStringLength = 24;
const TUint KNumSplitWriteSections = 3;

//These values MUST be kept in line with each other
const _LIT8(KClientPassString, "PASS");
const _LIT8(KClientFailString, "FAIL");
const TUint KPassFailStringLength = 4;

const TUint KTestBufferLength = 32; //must be more than KPassFailStringLength

const TUint KMaxSendSize = 2048;	



// ------------- bRequest -----------------
const TUint8 KVendorEmptyRequest(0x00);
const TUint8 KVendorPutPayloadRequest(0x01);
const TUint8 KVendorGetPayloadRequest(0x02);
const TUint8 KVendorGetRecordedNumBytesReadInPayloadRequest (0x03);
const TUint8 KVendorGetRecordedNumBytesWrittenInPayloadRequest (0x04);
const TUint8 KVendorStallRequest(0x05);
const TUint8 KVendorRemoteWakeupRequest(0x06);
const TUint8 KVendorReconnectRequest(0x07);
const TUint8 KVendorWriteToEndpointRequest(0x08);
const TUint8 KVendorPatternWriteToEndpointRequest(0x09);
const TUint8 KVendorCancelWriteToEndpointRequest(0x0a);
const TUint8 KVendorPatternWriteSynchronousToEndpointRequest(0x0b);
const TUint8 KVendorPatternWriteSynchronousToAndHaltEndpointRequest(0x0c);
const TUint8 KVendorStringValidationRequest(0x0d);
const TUint8 KVendorWriteSynchronousCachedReadRequest(0x0e);
const TUint8 KVendorWriteCachedReadRequest(0x0f);
const TUint8 KVendorRepeatedPatternWriteDataRequest (0x10);
const TUint8 KVendorSplitWriteSynchronousCachedReadRequest(0x11);
const TUint8 KVendorReadFromEndpointRequest(0x12);
const TUint8 KVendorReadUntilShortFromEndpointRequest(0x13);
const TUint8 KVendorReadFromAndHaltEndpointRequest(0x14);
const TUint8 KVendorRepeatedReadAndValidateDataRequest(0x15);
const TUint8 KVendorRecordedValidationResultRequest (0x16);
const TUint8 KVendorCancelAnyReadFromEndpointRequest (0x17);
const TUint8 KVendorTestCasePassed(0x20);
const TUint8 KVendorTestCaseFailed(0x21);
const TUint8 KVendorUnrespondRequest(0x22);
const TUint8 KVendorDisconnectDeviceAThenConnectDeviceCRequest(0x23);
const TUint8 KVendorDisconnectDeviceCThenConnectDeviceARequest(0x24);

// class specific
// audio
const TUint8 KAudioClassSetCur(0x01);

// ------------- bmRequestType -----------------

// D7: Data phase transfer direction
const TUint8 KHostToDevice(0x00);
const TUint8 KDeviceToHost(0x80);

// D6..5: Type
const TUint8 KTypeStandard(0x00);
const TUint8 KTypeClass(0x20);
const TUint8 KTypeVendor(0x40);

// D4..0: Recipient
const TUint8 KRecipientDevice(0x00);
const TUint8 KRecipientInterface(0x01);
const TUint8 KRecipientEndpoint(0x02);
const TUint8 KRecipientOther(0x03);

// ------------------------------------------------------------------

// The customised requests (i.e. non standard control requests)

// ------------------------------------------------------------------
	
/**
This policy class represents the basic control request setup packet
that the user will derive from and customise to describe usable control requests
otherwise with the default values it represents a 'standard device request'
*/
class TControlSetupPacket : public ::RUsbInterface::TUsbTransferRequestDetails
	{
protected:
	TControlSetupPacket()
		{
		iRequestType = 0;
		iRequest = 0;
		iValue = 0;
		iIndex = 0;
		}
	};

/**
This class represents a control request to the client that does not request 
data from the client.  It merely instructs the client
*/
class TEmptyRequest : public TControlSetupPacket
	{
protected:
	TEmptyRequest()
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		iRequest = KVendorEmptyRequest;
		}
	};

/**
 */
 class TDataSendRequest : public TControlSetupPacket
 	{
 	friend class CEp0Transfer;

 protected:
 	TDataSendRequest()
 		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		}
	
 	explicit TDataSendRequest(const TDesC8& aData)
 		{
 		iRequestType |= KHostToDevice;
 		iRequestType |= KTypeVendor;
 		iSendData.Copy(aData);
 		}

 	explicit TDataSendRequest(const TDesC16& aData)
 		{
 		iRequestType |= KHostToDevice;
 		iRequestType |= KTypeVendor;
 		iSendData.Copy(aData);
 		}
 	protected:
 	TBuf8<KMaxSendSize> iSendData;
 	};
 		
		
/**
*/
class TClassDataSendRequest : public TControlSetupPacket
	{
	friend class CEp0Transfer;

protected:
	explicit TClassDataSendRequest(const TDesC8& aData)
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeClass;
		iSendData.Copy(aData);
		}

	explicit TClassDataSendRequest(const TDesC16& aData)
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeClass;
		iSendData.Copy(aData);
		}
	protected:
	TBuf8<KMaxSendSize> iSendData;
	};


/**
This class represents a control request to the client that requests some data 
in response to the request
*/
class TDataRecvRequest : public TControlSetupPacket
	{
	friend class CEp0Transfer;

protected:
	explicit TDataRecvRequest(TDes8& aData) : iRecvData(aData)
		{
		iRequestType |= KDeviceToHost;
		iRequestType |= KTypeVendor;
		}

protected:
	TDes8& iRecvData;
	};


/**
*/
class TDescriptorGetRequest : public TDataRecvRequest
	{
public:
	/**
	Constructor, build a request to fetch a descriptor from the device
	@param aTypeAndIndex the type of the descriptor and the index
	@param aLanguage Id the identity of the language
	@param aData the symbian descriptor to hold the usb descriptor data 
	*/
	
	TDescriptorGetRequest(TUint16 aTypeAndIndex,TUint16 aLanguageId,TDes8& aData)
	:	TDataRecvRequest(aData)
		{
		iRequestType = 0; // Will overwrite KTypeVendor in base class
		iRequestType |= KDeviceToHost;	
		iRequest = 0x06; // Standard device GET_DESCRIPTOR
		iValue = aTypeAndIndex;
		iIndex = aLanguageId;
		}
	};

	
/**
This class represents an empty request that is directed at the device.
i.e. a request that does not require data from the client device
*/
class TEmptyDeviceRequest : public TEmptyRequest
	{
public:
	TEmptyDeviceRequest()
		{
		iRequestType |= KRecipientDevice;
		}
	};

/** 
This class represents an empty request that is directed at the interface.
i.e. a request that does not require data from the client interface
*/
class TEmptyInterfaceRequest : public TEmptyRequest
	{
public:
	explicit TEmptyInterfaceRequest(TUint16 aInterfaceNumber)
		{
		iRequestType |= KRecipientInterface;
		iIndex = aInterfaceNumber;
		}
	};
	
/**
This class represents a device directed request that send a payload
*/
class TDevicePutPayloadRequest : public TDataSendRequest
	{
public:
	TDevicePutPayloadRequest(const TDesC8& aData) : TDataSendRequest(aData)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorPutPayloadRequest;
		iValue = 0;
		iIndex = 0;
		}
	};
	
/**
This class represents a device directed request that retrieves a payload
from the client device
*/
class TDeviceGetPayloadRequest : public TDataRecvRequest
	{
public:
	explicit TDeviceGetPayloadRequest(TDes8& aData) : TDataRecvRequest(aData)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorGetPayloadRequest;
		iValue = 0;
		iIndex = 0;
		}
	};
	
/**
This class represents a device directed request that retrieves a payload
from the client device containing the number of bytes read on an endpoint 
performing a 'Repeated Read'. 
*/
class TInterfaceGetRecordedNumBytesReadInPayload : public TDataRecvRequest
	{
public:
	explicit TInterfaceGetRecordedNumBytesReadInPayload(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,TDes8& aData) : TDataRecvRequest(aData)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorGetRecordedNumBytesReadInPayloadRequest;
		iValue = aReadEndpointNumber;
		iIndex = aInterfaceNumber;
		}
	};
	
/**
This class represents a device directed request that retrieves a payload
from the client device containing the number of bytes written on an endpoint 
performing a 'Repeated Write'. 
*/
class TInterfaceGetRecordedNumBytesWrittenInPayload : public TDataRecvRequest
	{
public:
	explicit TInterfaceGetRecordedNumBytesWrittenInPayload(const TUint16 aInterfaceNumber,const TUint8 aWriteEndpointNumber,TDes8& aData) : TDataRecvRequest(aData)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorGetRecordedNumBytesWrittenInPayloadRequest;
		iValue = aWriteEndpointNumber;
		iIndex = aInterfaceNumber;
		}
	};
	


/**
This class represents an interface directed request that sends a payload
*/
class TInterfacePutPayloadRequest : public TDataSendRequest
	{
public:
	TInterfacePutPayloadRequest(TUint16 aInterfaceNumber,const TDesC8& aData) : TDataSendRequest(aData)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorPutPayloadRequest;
		iValue = 0;
		iIndex = aInterfaceNumber;
		}
	};
	
/**
This class represents a device directed request that retrieves a payload
from the client device
*/
class TInterfaceGetPayloadRequest : public TDataRecvRequest
	{
public:
	explicit TInterfaceGetPayloadRequest(TUint16 aInterfaceNumber,TDes8& aData) : TDataRecvRequest(aData)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorGetPayloadRequest;
		iValue = 0;
		iIndex = aInterfaceNumber;
		}
	};

/**
This class represents a control request to stall a specified endpoint
*/
class TInterfaceEndpointBaseRequest : public TEmptyRequest
	{
public:
	/**
	Constructor, build a request containing a specified endpoint and a specified interface
	@param aEndpointNumber the endpoint to use
	@param aInterfaceNumber the interface to use
	*/
	
	TInterfaceEndpointBaseRequest(TUint16 aEndpointNumber,TUint16 aInterfaceNumber)
	: TEmptyRequest()
		{
		iRequestType |= KRecipientInterface;
		iValue = aEndpointNumber;
		iIndex = aInterfaceNumber;
		}
	};

/**
This class represents a control request to stall a specified endpoint
*/
class TStallEndpointRequest : public TInterfaceEndpointBaseRequest
	{
public:
	/**
	Constructor, build a request to stall a specified endpoint
	@param aEndpointNumber the endpoint to stall
	*/
	
	TStallEndpointRequest(TUint16 aEndpointNumber,TUint16 aInterfaceNumber)
	: TInterfaceEndpointBaseRequest(aEndpointNumber, aInterfaceNumber)
		{
		iRequest = KVendorStallRequest;
		}
	};

/**
This class represents a control request to use a validation previously recorded in the endpoint
to update the interface's PASS\FAIL string.
*/
class TRecordedValidationResultRequest : public TInterfaceEndpointBaseRequest
	{
public:
	/**
	Constructor, build a request to update the interface's PASS\FAIL 
	string using a previously recorded validation.
	@param aEndpointNumber the endpoint to from which to retrieve the previously recorded validation
	*/
	
TRecordedValidationResultRequest(TUint16 aEndpointNumber,TUint16 aInterfaceNumber)
	: TInterfaceEndpointBaseRequest(aEndpointNumber, aInterfaceNumber)
		{
		iRequest = KVendorRecordedValidationResultRequest;
		}
	};

/**
This class represents a control request to stall a specified endpoint
*/
class TEndpointCancelWriteRequest : public TInterfaceEndpointBaseRequest
	{
public:
	/**
	Constructor, build a request to stall a specified endpoint
	@param aEndpointNumber the endpoint to stall
	*/
	
	TEndpointCancelWriteRequest(TUint16 aEndpointNumber,TUint16 aInterfaceNumber)
	: TInterfaceEndpointBaseRequest(aEndpointNumber, aInterfaceNumber)
		{
		iRequest = KVendorCancelWriteToEndpointRequest;
		}
	};

/**
This class represents a control request to stall a specified endpoint
*/
class TEndpointCancelReadRequest : public TInterfaceEndpointBaseRequest
	{
public:
	/**
	Constructor, build a request to stall a specified endpoint
	@param aEndpointNumber the endpoint to stall
	*/
	
	TEndpointCancelReadRequest(TUint16 aEndpointNumber,TUint16 aInterfaceNumber)
	: TInterfaceEndpointBaseRequest(aEndpointNumber, aInterfaceNumber)
		{
		iRequest = KVendorCancelAnyReadFromEndpointRequest;
		}
	};

/**
This class represents a control request to the device to initiate a remote
wake-up after the supplied interval has elapsed.
*/
class TRemoteWakeupRequest : public TEmptyRequest
	{
public:
	explicit TRemoteWakeupRequest(TUint16 aWakeupInterval)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorRemoteWakeupRequest;
		iValue = aWakeupInterval;
		}
	};

/**
This class represents a control request to the device to reconnect to the host after 
the supplied interval has elapsed
*/
class TReconnectRequest : public TEmptyRequest
	{
public:
	explicit TReconnectRequest(TUint16 aReconnectInterval)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorReconnectRequest;
		iValue = aReconnectInterval;		
		}
	};
	
/**
This class represents a control request to the device to disconnect device A and connect 
device C to the host 
*/
class TDisconnectDeviceAThenConnectDeviceCRequest : public TEmptyRequest
	{
public:
	/**
	Constructor, build a request that informs the client device of a successful test case status
	*/
	
	TDisconnectDeviceAThenConnectDeviceCRequest()
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorDisconnectDeviceAThenConnectDeviceCRequest;
		iValue = KErrNone;
		}
	};

/**
This class represents a control request to the device to disconnect device C and connect 
device A to the host 
*/
class TDisconnectDeviceCThenConnectDeviceARequest : public TEmptyRequest
	{
public:
	/**
	Constructor, build a request that informs the client device of a successful test case status
	*/
	
	TDisconnectDeviceCThenConnectDeviceARequest()
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorDisconnectDeviceCThenConnectDeviceARequest;
		iValue = KErrNone;
		}
	};
	
/**
This class represents an instruction to the client to write the data 
supplied to an endpoint that is specified
*/
class TEndpointWriteRequest : public TDataSendRequest
	{
public:
	/**
	Constructor, build a request that instructs an interface on the client device to write data
	to a specified endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aData the data to write to that endpoint
	*/
	
	TEndpointWriteRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TDesC8& aData) : TDataSendRequest(aData)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorWriteToEndpointRequest;
		iValue = aEndpointNumber;
		iIndex = aInterfaceNumber;
		}
	};

/**
This class represents an instruction to the client to write specified data a number of times 
on a given endpoint
*/
class TEndpointPatternWriteRequest : public TDataSendRequest
 	{
 	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to write specified data
	to an endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aData the data pattern to use when writing to that endpoint
	@param aNumBytes the number of bytes to write using that data pattern
	*/
	TEndpointPatternWriteRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TDesC8& aData,const TUint aNumBytes)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorPatternWriteToEndpointRequest;
		iValue = aEndpointNumber; 
		iIndex = aInterfaceNumber;

		iSendData.Zero();
 		iSendData.Format(KNumberFormatString, aNumBytes);
 		iSendData.Append(aData);
 		}
 	};

	
		
/**
This class represents an instruction to the client to synchronously write specified data a number of times 
on a given endpoint
*/
class TEndpointPatternSynchronousWriteRequest : public TEndpointPatternWriteRequest
 	{
 	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to write specified data
	to an endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aData the data pattern to use when writing to that endpoint
	@param aNumBytes the number of bytes to write using that data pattern
	*/
	TEndpointPatternSynchronousWriteRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TDesC8& aData,const TUint aNumBytes)
	: TEndpointPatternWriteRequest(aInterfaceNumber,aEndpointNumber,aData,aNumBytes)
		{
		iRequest = KVendorPatternWriteSynchronousToEndpointRequest;
		}
 	};

	
		
/**
This class represents an instruction to the client synchronously to write specified data a number of times 
on a given endpoint and then halt that endpoint
*/
class TEndpointPatternSynchronousWriteAndHaltRequest : public TEndpointPatternSynchronousWriteRequest
 	{
 	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to write specified data
	to an endpoint and then halt that endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aData the data pattern to use when writing to that endpoint
	@param aNumBytes the number of bytes to write using that data pattern
	*/
	TEndpointPatternSynchronousWriteAndHaltRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TDesC8& aData,const TUint aNumBytes)
	: TEndpointPatternSynchronousWriteRequest(aInterfaceNumber, aEndpointNumber, aData, aNumBytes)
		{
		iRequest = KVendorPatternWriteSynchronousToAndHaltEndpointRequest;
 		}
 	};

/**
This class represents an instruction to the client to validate data read on an endpoint with specified data 
on a given endpoint
*/
 class TEndpointStringValidationRequest : public TEndpointPatternWriteRequest
 	{
 	friend class CEp0Transfer;

 public:
	/**
	Constructor, build a request that instructs an interface on the client device to 
	verify data on an endpoint using the data pattern provided
	@param aInterfaceNumber the number of the interface which has this endpoint
	@param aEndpointNumber the number of the endpoint to use (a pipe must be open on this endpoint)
	@param aData the data pattern to use when verifying on that endpoint
	@param aNumBytes the number of bytes to write using that data pattern	*/
	TEndpointStringValidationRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TDesC8& aData,const TUint aNumBytes)
 	: TEndpointPatternWriteRequest(aInterfaceNumber,aEndpointNumber,aData,aNumBytes)
 		{
 		iRequest = KVendorStringValidationRequest;
		}
 	};
	 	
	
		
/**
This class represents an instruction to the client to write back data just read.
*/
class TWriteSynchronousCachedReadDataRequest : public TControlSetupPacket
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to use the data
	just read on one endpoint to send back on another.
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aReadEndpointNumber the number of the endpoint that has just been read from (a pipe must be open on this endpoint)
	@param aWriteEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	*/
	TWriteSynchronousCachedReadDataRequest(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,const TUint8 aWriteEndpointNumber)
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		iRequestType |= KRecipientInterface;
		iRequest = KVendorWriteSynchronousCachedReadRequest;
		iValue = aReadEndpointNumber << 8 | aWriteEndpointNumber; 
		iIndex = aInterfaceNumber;
		}
	};
	
/**
This class represents an instruction to the client to write back data just read.
*/
class TWriteCachedReadDataRequest : public TWriteSynchronousCachedReadDataRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to use the data
	just read on one endpoint to send back on another.
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aReadEndpointNumber the number of the endpoint that has just been read from (a pipe must be open on this endpoint)
	@param aWriteEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	*/
	TWriteCachedReadDataRequest(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,const TUint8 aWriteEndpointNumber)
	: TWriteSynchronousCachedReadDataRequest(aInterfaceNumber, aReadEndpointNumber, aWriteEndpointNumber)
		{
		iRequest = KVendorWriteCachedReadRequest;
		}
	};
	
	
/**
This class represents an instruction to the client to write back data just read - in sections.
*/
class TSplitWriteCachedReadDataRequest : public TDataSendRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to use the data
	just read on one endpoint to send back on another.
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aReadEndpointNumber the number of the endpoint that has just been read from (a pipe must be open on this endpoint)
	@param aWriteEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	*/
	TSplitWriteCachedReadDataRequest(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,const TUint8 aWriteEndpointNumber,const TUint aNumBytes[KNumSplitWriteSections])
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		iRequestType |= KRecipientInterface;
		iRequest = KVendorSplitWriteSynchronousCachedReadRequest;
		iValue = aReadEndpointNumber << 8 | aWriteEndpointNumber; 
		iIndex = aInterfaceNumber;

		iSendData.Zero();
		for(TUint i = 0; i<KNumSplitWriteSections; ++i)
			{
			TBuf8<KNumberStringLength> buf;
			buf.Format(KNumberFormatString, aNumBytes[i]);
			iSendData.Append(buf);
			}
 		}
	};
	
/**
This class represents an instruction to the client to write back data just read - in sections.
*/
class TRepeatedWriteDataRequest : public TDataSendRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to perform 
	a repeated read using the data as a pattern to validate the data as it arrives.
	Constructor, build a request that instructs an interface on the client device to perform 
	a repeated write using the data as a pattern.
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aReadEndpointNumber the number of the endpoint that has just been read from (a pipe must be open on this endpoint)
	@param aData the data pattern to be used for validation
	@param aNumBytesPerRead the number of bytes to read at each 'Read'
	@param aTotalNumBytes the total number of bytes to read (over all 'Read's)
	*/
TRepeatedWriteDataRequest(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,const TDesC8& aDataPattern,const TUint aNumBytesPerRead,const TUint aTotalNumBytes)
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		iRequestType |= KRecipientInterface;
		iRequest = KVendorRepeatedPatternWriteDataRequest;
		iValue = aReadEndpointNumber; 
		iIndex = aInterfaceNumber;

		iSendData.Zero();
		TBuf8<KTwoNumberStringLength> buf;
		buf.Format(KTwoNumberFormatString, aNumBytesPerRead, aTotalNumBytes);
		iSendData.Append(buf);
		iSendData.Append(aDataPattern);
		}
	};
	
	 	
	
/**
This class represents an instruction to the client to read a number of bytes of data 
on an the endpoint.
*/
class TEndpointReadRequest : public TControlSetupPacket
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to read a specified amount data
	on the specified endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aNumBytes the amount of data to read on that endpoint
	*/
	TEndpointReadRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TUint aNumBytes)
		{
		iRequestType |= KHostToDevice;
		iRequestType |= KTypeVendor;
		iRequestType |= KRecipientInterface;
		iRequest = KVendorReadFromEndpointRequest;
		iValue = aEndpointNumber; 
		iIndex = aInterfaceNumber;

		iReadSpecificationData.Zero();
		iReadSpecificationData.Format(KNumberFormatString, aNumBytes);
		}

protected:
	TBuf8<KNumberStringLength> iReadSpecificationData;
	};

/**
This class represents an instruction to the client to read a number of bytes of data 
on an the endpoint. Reading will complete early if a short packet is detected.
*/
class TEndpointReadUntilShortRequest : public TEndpointReadRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to read the data
	on the specified endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aNumBytes the amount of data to read on that endpoint (if a short packet does not arrive sooner)
	*/
	TEndpointReadUntilShortRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TUint aNumBytes)
	: TEndpointReadRequest(aInterfaceNumber, aEndpointNumber, aNumBytes)
		{
		iRequest = KVendorReadUntilShortFromEndpointRequest;
		}
	};

/**
This class represents an instruction to the client to read a number of bytes of data 
on an the endpoint, and then, when the specified number of bytes have been read, to stall the endpoint.
*/
class TEndpointReadAndHaltRequest : public TEndpointReadRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to read the data
	on the specified endpoint
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aEndpointNumber the number of the endpoint to write to (a pipe must be open on this endpoint)
	@param aNumBytes the amount of data to read on that endpoint (if a short packet does not arrive sooner)
	*/
	TEndpointReadAndHaltRequest(TUint16 aInterfaceNumber,TUint16 aEndpointNumber,const TUint aNumBytes)
	: TEndpointReadRequest(aInterfaceNumber, aEndpointNumber, aNumBytes)
		{
		iRequest = KVendorReadFromAndHaltEndpointRequest;
		}
	};

/**
This class represents an instruction to the client to write back data just read - in sections.
*/
class TRepeatedReadAndValidateDataRequest : public TRepeatedWriteDataRequest
	{
	friend class CEp0Transfer;

public:
	/**
	Constructor, build a request that instructs an interface on the client device to perform 
	a repeated read using the data as a pattern to validate the data as it arrives.
	@param aInterfaceNumber the number of the interface which has this endpoint to write to
	@param aReadEndpointNumber the number of the endpoint that has just been read from (a pipe must be open on this endpoint)
	@param aData the data pattern to be used for validation
	@param aNumBytesPerRead the number of bytes to read at each 'Read'
	@param aTotalNumBytes the total number of bytes to read (over all 'Read's)
	*/
TRepeatedReadAndValidateDataRequest(const TUint16 aInterfaceNumber,const TUint8 aReadEndpointNumber,const TDesC8& aDataPattern,const TUint aNumBytesPerRead,const TUint aTotalNumBytes)
: TRepeatedWriteDataRequest(aInterfaceNumber,aReadEndpointNumber,aDataPattern,aNumBytesPerRead,aTotalNumBytes)		
		{
		iRequest = KVendorRepeatedReadAndValidateDataRequest;
		}
	};
	
	 	
/**
This class represents a request that requests the client device
to negatively acknowledge the request
*/
class TNakRequest : public TEmptyRequest
	{
public:
	/**
	Constructor, build a request that instructs an interface to continually NAK the request
	@param aInterfaceNumber the interface number
	*/
	
	explicit TNakRequest(TUint16 aInterfaceNumber)
		{
		iRequestType |= KRecipientInterface;
		iRequest = KVendorUnrespondRequest;
		iIndex = aInterfaceNumber;
		}
	};
	
/**
This class represents a control request to the device to indicate that test case
has successfully completed and to disconnect the device
*/
class TTestCasePassed : public TEmptyRequest
	{
public:
	/**
	Constructor, build a request that informs the client device of a successful test case status
	*/
	
	TTestCasePassed()
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorTestCasePassed;
		iValue = KErrNone;
		}
	};
	
/**
This class represents a control request to the device of an unsuccessful test case execution
and to complete the device side test case with the supplied error and error message
*/
class TTestCaseFailed : public TDataSendRequest	
	{
public:
	/**
	Constructor, build a request that informs the client device of an unsuccessful test case status
	@param aError the error that the host side test case reports
	@param aErrorMsg the message text (non-unicode) for the client to display when this request has been received 
	*/
	
	TTestCaseFailed(TInt aError,const TDesC8& aErrorMsg) : TDataSendRequest(aErrorMsg)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorTestCaseFailed;
		iValue = -aError; // Invert the error as symbian system errors are negative
		}
	
	/**
	Constructor, build a request that informs the client device of an unsuccessful test case status
	@param aError the error that the host side test case reports
	@param aErrorMsg the message text (unicode) for the client to display when this request has been received 
	*/
	
	TTestCaseFailed(TInt aError,const TDesC16& aErrorMsg) : TDataSendRequest(aErrorMsg)
		{
		iRequestType |= KRecipientDevice;
		iRequest = KVendorTestCaseFailed;
		iValue = -aError; // Invert the error as symbian system errors are negative
		}
	};


/**
This class represents the SET_CUR audio class control request
*/
class TSetCurRequest : public TClassDataSendRequest
	{
public:
	explicit TSetCurRequest(const TDesC8& aData, const TUint aEndpointAddress) : TClassDataSendRequest(aData) //TUint16 aSamplingFrequency)
		{
		iRequestType |= KRecipientEndpoint;
		iRequest = KAudioClassSetCur;		
		iValue = 0x0100;
		iIndex = aEndpointAddress;
		}
	};

	
/**
This class describes an observer to the Ep0 control transfer
*/
class MCommandObserver
	{
public:
	/**
	Called when an endpoint zero test command transfer has completed
	@param aCompletionCode the completion code of the asynchronous transfer to Ep0
	*/
	virtual void Ep0TransferCompleteL(TInt aCompletionCode) = 0;
	};

/**
This class represents a transfer to a control endpoint 0
*/
class CEp0Transfer : public CActive
	{
public:
	/**
	Constructor, build a test command to send to the connected client
	@param aToken the token 
	*/
	CEp0Transfer(RUsbInterface& aInterface0);
	
	/**
	Destructor
	*/
	~CEp0Transfer();
	
	/**
	Send a control transfer request to endpoint zero of the client
	@param aSetupPacket a control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TEmptyRequest& aSetupPacket,MCommandObserver* aObserver);
	
	/**
	Send a control transfer request to endpoint zero of the client
	@param aSetupPacket a control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TDataRecvRequest& aSetupPacket,MCommandObserver* aObserver);
	
	/**
	Send a control transfer request to endpoint zero of the client that will also send a payload in data packets
	@param aSetupPacket a control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TDataSendRequest& aSetupPacket,MCommandObserver* aObserver);

	/**
	Send a control transfer request to endpoint zero of the client
	@param aSetupPacket a control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TWriteSynchronousCachedReadDataRequest& aSetupPacket,MCommandObserver* aObserver);

	/**
	Send a control transfer request to endpoint zero of the client that will also send a payload in data packets
	@param aSetupPacket a control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TEndpointReadRequest& aSetupPacket,MCommandObserver* aObserver);

	/**
	Send a class control transfer request to endpoint zero of the client that will also send a payload in data packets
	@param aSetupPacket a class control request
	@param aObserver the observer that will be notified of transfer completion
	*/
	void SendRequest(TClassDataSendRequest& aSetupPacket,MCommandObserver* aObserver);
	
	/**
	Cancel last SendRequest if still active. This does NOT cancel the active object
	which must be done separately using the normal 'Cancel' function.
	*/
	void CancelSendRequest();

	/**
	Gets the time the last SendRequest was sent.
	*/
	void LastRequestStartTime( TTime& aDuration);

	/**
	Gets the time the last SendRequest was completed in RunL.
	*/
	void LastRequestCompletionTime( TTime& aDuration);

private:
	
	/**
	Currently no way to cancel a single transfer, so not going to try
	*/
	void DoCancel();

	/**
	*/
	void RunL();
	
	/**
	*/
	TInt RunError(TInt aError);

private:

	/**
	The USB interface endpoint zero which receives the command
	For the test devices modelled 
	*/
	RUsbInterface& iUsbInterface0;
	
	/**
	The observer of client responses to host commands (uses-a)
	*/
	MCommandObserver* iObserver;

	/**
	The actual amount of data received by the host from the client
	in response to a command sent by the host
	*/
	TInt iActualLength;
	
	/**
	The flag to indicate if the request sent required the clieent device to answer with
	some data
	*/
	TBool iDataRequest;
	
	TBuf8<1> iTemp;

	/**
	 The time the last SendRequest was sent.
	 */
	TTime iRequestTime;

	/**
	 The time the last SendRequest was completed in RunL
	 */
	TTime iCompletionTime;
	};


	}


#endif

