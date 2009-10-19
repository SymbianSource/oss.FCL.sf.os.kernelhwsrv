#ifndef __EP0_READER_H
#define __EPO_READER_H

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
* @file Ep0Reader.h
* @internalComponent
* 
*
*/



#include "controlendpointreader.h"
#include "endpointwriter.h"

namespace NUnitTesting_USBDI
	{

/**
This class describes a processor for device directed requests and associated data
payload packets
*/
class CDeviceEndpoint0 : public CBase
	{
public:
	
	/**
	Constructor (2-phase) 
	@return a pointer to a new instance of control Ep0 reader
	*/
	
	static CDeviceEndpoint0* NewL(MRequestHandler& aRequestHandler);
	
	/**
	Destructor
	*/
	
	~CDeviceEndpoint0();

	/**
	Starts this reader receiving device directed ep0 requests
	@return KErrNone if successful or system wide error code
	*/

	TInt Start();

	/**
	Stops this reader receiving device directed ep0 requests
	@return KErrNone if successful or system wide error code
	*/
	TInt Stop();
	
	/**
	Send data back to the host through endpoint 0
	@param aData the descriptor with the data to send back
	*/
	void SendData(const TDesC8& aData);

	/**
	Send data back to the host through endpoint 0 and do not return until completion
	@param aData the descriptor with the data to send back
	*/
	TInt SendDataSynchronous(const TDesC8& aData);
	
	/**
	Access the reader object for this control endpoint 0
	*/
	CControlEndpointReader& Reader();
	
private:
	
	/**
	Constructor, build a reader for control endpoint 0
	*/
	
	CDeviceEndpoint0();
	
	/**
	2nd phase constructor
	*/
	
	void ConstructL(MRequestHandler& aRequestHandler);

private:
	
	/**
	The channel to the USB client driver
	*/
	
	RDevUsbcClient iClientDriver;
	
	/**
	The endpoint reader for endpoint 0
	*/
	
	CControlEndpointReader* iEndpoint0Reader;
		
	/**
	The endpoint writer for endpoint 0
	*/
	
	CEndpointWriter* iEndpoint0Writer;
	};
		
	}

#endif


