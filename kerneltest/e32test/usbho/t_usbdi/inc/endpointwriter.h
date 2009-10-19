#ifndef __ENDPOINT_WRITER_H
#define __ENDPOINT_WRITER_H

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
* @file endpointwriter.h
* @internalComponent
* 
*
*/



#include <d32usbc.h>

namespace NUnitTesting_USBDI
	{

/**
This class describes a generic writer of data to host endpoints
*/
class CEndpointWriter : public CActive
	{
public:
	/**
	Constructor, build an endpoint writer
	@param aClientDriver a referrence to a channel to the client USB driver
	@param aEndpoint the endpoint number to write to
	*/
	CEndpointWriter(RDevUsbcClient& aClientDriver,TEndpointNumber aEndpoint);

	/**
	Destructor
	*/
	virtual ~CEndpointWriter();
	
	/**
	Get numebr of bytes writtenat an instant in time if doing a multiple asynchronous 'Write' 
	*/
	TUint NumBytesWrittenSoFar();
	
	/**
	Write the supplied data to through the endpoint to the host
	@param aData the byte data to write to the host
	@param aUseZLP send a zero length packet if appropriate
	@param aCreateBuffer reallocate this object's 'iBuffer' and copy the data into it (required for aData is transient)
	*/
	void Write(const TDesC8& aData, TBool aUseZLP, TBool aCreateBuffer = ETrue);

	/**
	Write the supplied data to through the endpoint to the host and wait for completion
	@param aData the byte data to write to the host
	@return error
	*/
	TInt WriteSynchronous(const TDesC8& aData, TBool aUseZLP);
	
	/**
	Write 'aNumBytes' bytes of data using 'aDataPattern' through the endpoint to the host
	@param aDataPattern the byte data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write to the host
	@param aUseZLP use a zero lengt packet if last write packet conatins max packet's worth of data 
	*/
	void WriteSynchronousUsingPatternL(const TDesC8& aDataPattern, const TUint aNumBytesconst, TBool aUseZLP);

	/**
	Write 'aNumBytes' bytes of data using 'aDataPattern' through the endpoint to the host
	and wait for completion.
	@param aDataPattern the byte data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write to the host
	*/
	void WriteSynchronousUsingPatternL(const TDesC8& aDataPattern, const TUint aNumBytes);
	
	/**
	Write 'aNumBytes' bytes of data using 'aDataPattern' through the endpoint to the host, 
	and then halt the endpoint.
	@param aDataPattern the byte data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write to the host
	*/
	void WriteSynchronousUsingPatternAndHaltL(const TDesC8& aDataPattern, const TUint aNumBytes);
	
	/**
	Kick off a 'Write' of 'aNumBytes' bytes of data using 'aDataPattern' through the endpoint to the host.
	@param aData the byte data to write to the host
	@param aNumRepeats the number of times to repeat in the buffer to be used when sending	
	*/
	void WriteUsingPatternL(const TDesC8& aData, const TUint aNumBytes, const TBool aUseZLP);
	
	/**
	Kick off a sequence of 'Writes' which in total will write 'aNumBytes' bytes of data using 'aDataPattern' 
	through the endpoint to the host.
	@param aNumBytesPerWrite the number of bytes of data to write to the host per call to USB client 'Write'
	@param aTotalNumBytes the total number of bytes of data to write to the host
	@param aNumRepeats the number of times to repeat in the buffer to be used when sending	
	*/
	void WriteInPartsUsingPatternL(const TDesC8& aData, const TUint aNumBytesPerWrite, TUint aTotalNumBytes, const TBool aUseZLP);

	
private: // From CActive
	/**
	*/
	void DoCancel();
	
	/**
	*/
	void RunL();

	/**
	*/
	TInt RunError(TInt aError);

	/**
	*/
	void CreateBigBuffer(const TDesC8& aData, const TUint aRepeats);

private:
	/**
	The channel to the USB client driver
	*/
	RDevUsbcClient& iClientDriver;

	/**
	The endpoint number that this writer will write to
	*/
	TEndpointNumber iEndpoint;	

	/**
	The total number of bytes in a repeated 'Write'
	*/
	TUint iTotalNumBytes;	

	/**
	The number of bytes currently writte in a repeated write
	*/
	TUint iNumBytesWritten;	

	/**
	The number of bytes to write at each successive a 'Write'
	*/
	TUint iNumBytesOnCurrentWrite;	

	/**
	The length of the data pattern in a repeated write
	*/
	TUint iDataPatternLength;	
	
	/**
	Is a ZLP required for the last 'Write' in a successive 'Write'
	*/
	TUint iUseZLP;	
	
	/**
	Buffer for WriteInOne
	*/
	HBufC8* iBuffer;
	
	/**
	Ptr to buffer for WriteInOne
	*/ 
	TPtr8 iBufPtr;

	};


	}




#endif