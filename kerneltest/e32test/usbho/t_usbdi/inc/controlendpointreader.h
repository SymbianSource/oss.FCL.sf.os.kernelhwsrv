#ifndef __CONTROL_ENDPOINT_READER_H
#define __CONTROL_ENDPOINT_READER_H

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
* @file controlendpointreader.h
* @internalComponent
* 
*
*/



#include "endpointreader.h"

namespace NUnitTesting_USBDI
	{

/**
*/
class MRequestHandler
	{
public:
	/**
	Called when a control vendor request from the host is received and does not require data being
	sent back to the host.
	@param aRequest the control request value
	@param aValue a parameter value for the request
	@param aIndex an index parameter for the request
	@param aDataReqLength the length of the data to be returned to the host
	@param aPayload the data payload sent to the device by the host in a data phase
	*/
	
	virtual TInt ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,TUint16 aDataReqLength,
					const TDesC8& aPayload) = 0;
	};


	
/**
This class describes a entity that reads control requests from endpoint 0
*/
class CControlEndpointReader : public CEndpointReader, public MEndpointDataHandler
	{
public:

	/**
	Constructor, build a reader of requests from endpoint zero
	@param aClientDriver the channel to the USB client driver
	@param aRequestHandler the handler of control requests
	*/
	
	CControlEndpointReader(RDevUsbcClient& aClientDriver,MRequestHandler& aRequestHandler);
	
	/**
	Destructor
	*/
	
	virtual ~CControlEndpointReader();

	/**
	Reads requests sent by the host on endpoint 0.  This includes any data from the data phase
	of the transfer
	*/
	
	void ReadRequestsL();

public: // From MEndpointDataHandler

	/**
	Interprets the data received as a control request on endpoint 0
	@param aEndpointNumber the number of the endpoint read from
	@param aData the data successfully read from the endpoint
	*/
	
	virtual void DataReceivedFromEndpointL(TEndpointNumber aEndpointNumber,const TDesC8& aData);	
	
	/**
	Notified when an error occurs on a read from the endpoint
	@param aEndpointNumber the number of the endpoint read from
	@param aErrorCode the read operation completion error
	*/
	
	void EndpointReadError(TEndpointNumber aEndpointNumber,TInt aErrorCode);
	
private:

	// Flag to indicate
	TBool iDeviceToHost;
	TBool iDataPhase;
	
	/**
	The request status of the entity processing requests
	*/
	MRequestHandler& iRequestHandler;
	
	/**
	The fields of the control transfer request
	*/
	TUint8 ibRequestType;
	TUint8 ibRequest;
	TUint16 iwValue;
	TUint16 iwIndex;
	TUint16 iwLength;
	};
	
	}


#endif


