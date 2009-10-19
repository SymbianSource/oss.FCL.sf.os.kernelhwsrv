#ifndef __ENDPOINT_READER_H
#define __ENDPOINT_READER_H

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
* @file endpointreader.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbc.h>

namespace NUnitTesting_USBDI
	{
	
/*
 * The buffer to read control packet data into
 * Low-speed 8bytes
 * High-speed 8,16,32 or 64bytes
 * Full-speed 64bytes
 */

const TInt KFullSpeedPacketSize = 64;

/**
This class describes a handler for bytes received from the endpoint
*/
class MEndpointDataHandler
	{
public:
	/**
	Called when data is read from an endpoint
	@param aEndpointNumber the number of the endpoint that was read from
	@param aData the data read from the endpoint (valid data if the completion code is KErrNone)
	@param aCompletionCode the completion code for the read 
	*/
	virtual void DataReceivedFromEndpointL(TEndpointNumber aEndpointNumber,const TDesC8& aData) = 0;
	
	/**
	Called when the read operation from the endpoint completes with an error
	@param aEndpointNumber the number of the endpoint read from
	@param aErrorCode the operation error completion code
	*/
	virtual void EndpointReadError(TEndpointNumber aEndpointNumber,TInt aErrorCode) = 0;
	};


/**
This class describes a general asyncronous endpoint reader
*/
class CEndpointReader : public CActive
	{
public:
	enum TCompletionAction
		{
		ENone,
		EHaltEndpoint,
		ERepeatedRead,
		};
	/**
	Constructor, build a reader that reads byte data from the specified endpoint number
	@param aClientDriver the driver channel to the usb client driver
	@param aEndpoint the endpoint number to read from
	*/
	CEndpointReader(RDevUsbcClient& aClientDriver,TEndpointNumber aEndpoint);

	/**
	Destructor
	*/	
	virtual ~CEndpointReader();

	/**
	Return data buffer used for Reads - could be NULL!
	@return data buffer
	*/
	TPtr8 Buffer();

	/**
	Return the result of the last recorded validation result!
	@return that result
	*/
	TBool IsValid();

	/**
	Return the number of bytes read so far on a repeated (asynchronous)'Read'
	@return that number of bytes
	*/
	TUint NumBytesReadSoFar();

	/**
	Read a data packet from the endpoint specified
	@param aHandler a pointer to the handler of the data that will be read from the host
	*/
	void ReadPacketL(MEndpointDataHandler* aHandler);
	
	/**
	Read the specified number of bytes from the endpoint
	@param aByteCount the number of bytes to read
	*/
	void ReadL(TInt aByteCount);

	/**
	Read the specified number of bytes (or fewer id a short packet arrives) from the endpoint
	@param aByteCount the number of bytes to read
	*/
	void ReadUntilShortL(TInt aByteCount);

	/**
	Read the specified number of bytes from the endpoint
	Flag the need to halt the endpoint when the read has completed
	@param aByteCount the number of bytes to read
	*/
	void ReadAndHaltL(TInt aByteCount);

	/**
	Read a specified number of bytes from the endpoint in sections,
	performing a new 'Read' for each section.
	After each 'Read' use the data pattern to validate the results. 
	Expect the data pattern to be repeated in its entirety until 
	the total number of bytes have been sent.
	@param aDataPattern the data pattern to be used for validation
	@param aNumBytesPerRead the number of bytes to ask for at each 'Read'
	@param aTotalNumBytes the total number of bytes to read
	*/
	void RepeatedReadAndValidateL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes);

	/**
	Send an acknowledgment back to the host
	This will be a zero length DATA1 packet
	@return KErrNone if successful otherwise a system wide error code
	*/
	TInt Acknowledge();

protected:
	/**
	Cancels the reading from the host
	*/
	void DoCancel();
	
	/**
	*/
	virtual void RunL();
	
	/**
	The framework error function from RunL
	@param aError the error from a RunL leave
	@return KErrNone
	*/
	TInt RunError(TInt aError);

protected:
	/**
	The channel to use to communicate to the client driver
	*/
	RDevUsbcClient& iClientDriver;
	
	/**
	The endpoint number that this reader will read from
	*/
	TEndpointNumber iEndpoint;	
	
	/**
	The handler for Endpoint zero requests received
	*/
	MEndpointDataHandler* iHandler;
	
	/**
	The buffer for the data read from the endpoint
	*/
	HBufC8* iDataBuffer;
	TPtr8 iDataPtr;
	
	/**
	The buffer for the data read from the endpoint
	*/
	HBufC8* iValidationPatternBuffer;
	TPtr8 iValidationPatternPtr;
	
	/**
	Competion Action
	*/
	TCompletionAction iCompletionAction;

	/**
	Needed if completion action is ERepeatedRead
	*/
	TUint iRepeatedReadTotalNumBytes;
	TUint iRepeatedReadNumBytesPerRead;
	TUint iNumBytesReadSoFar;
	TUint iDataPatternLength;
	TBool iIsValid;
	};


	}

#endif


	