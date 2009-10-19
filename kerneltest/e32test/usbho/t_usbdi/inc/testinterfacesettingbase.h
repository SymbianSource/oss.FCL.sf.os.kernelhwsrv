#ifndef __TEST_INTERFACE_SETTING_BASE_H
#define __TEST_INTERFACE_SETTING_BASE_H

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
* @file testinterfacesettingbase.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbc.h>
#include <e32hashtab.h>
#include "testendpointbase.h"
#include "endpointreader.h"
#include "endpointwriter.h"

namespace NUnitTesting_USBDI
	{	

// For the RHashMap
inline TUint32 EndpointNumberHash(const TEndpointNumber& aEndpointNumber)
	{
	return DefaultHash::Integer(static_cast<TInt>(aEndpointNumber));
	}
	
inline TBool EndpointIdentityRelationship(const TEndpointNumber& aArg1,const TEndpointNumber& aArg2)
	{
	return aArg1 == aArg2;
	}
		
	
// Forward declarations
class TEndpoint;
	
/**
This class represents a base class for alternate interface settings.
The class contains the endpoints that will be configured for use when this interface setting
is selected.
*/
class CInterfaceSettingBase : public CBase
	{
	
	friend class CInterfaceBase;

	typedef RHashMap<TEndpointNumber,TEndpoint> REndpointMap;

public:
	
	/**
	Constructor, build an interface setting
	@param aString the name of this interface setting
	*/
	
	explicit CInterfaceSettingBase(const TDesC& aString);	
	
	/**
	Destructor
	*/
	virtual ~CInterfaceSettingBase();
	
	/**
	Sets the specific class code, subclass code and protocol for this interface
	codes are stated by the USB org.
	@param aClassCode the class code for this interface
		   01h  Audio   
		   02h  Communications and CDC Control (together with device)
		   03h  HID (Human Interface Device) 
		   05h  Physical 
		   06h  Image 
		   07h  Printer 
		   08h  Mass Storage 
		   0Ah  CDC-Data 
		   0Bh  Smart Card 
		   0Dh  Content Security 
		   0Eh  Video 
		   DCh  Diagnostic Device (together with device)
		   E0h  Wireless Controller 
		   EFh  Miscellaneous (together with device)
		   FEh  Application Specific 
		   FFh  Vendor Specific (together with device)
	@param aSubClassCode the subclass code specified by the USB org 
	@param aDeviceProtocol
	*/
	void SetClassCodeL(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol);
	
	/**
	Add an endpoint to this alternate interface setting
	@param anEndpoint the endpoint resource to add
	@return KErrNone if successful or KErrOverflow if the endpoint cannot be added due to
			resource limitation
	*/
	TInt AddEndpoint(TEndpoint& anEndpoint);
	
	/**
	Create 
	*/
	void CreateEndpointReaderL(RDevUsbcClient& aClientDriver,TUint aEndpoint);
	
	/**
	Create 
	*/
	void CreateEndpointWriterL(RDevUsbcClient& aClientDriver,TUint aEndpoint);
		
	/**
	Write the supplied data to the specified endpoint that can be found on 
	this interface setting
	@param aData the data to write to the host
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void WriteSpecifiedDataToEndpointL(const TDesC8& aData,TUint16 aEndpointNumber);

	/**
	Cancel a current asynchronous 'Write' on the specified endpoint that can be found on 
	this interface setting
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void CancelWriteDataToEndpointL(TUint16 aEndpointNumber);
	
	/**
	Write the supplied data to the specified endpoint that can be found on 
	this interface setting
	@param aDataPattern the data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write using this data pattern
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void WriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, TUint aNumBytes, TUint16 aEndpointNumber);
	
	/**
	Write the supplied data to the specified endpoint that can be found on 
	this interface setting
	@param aDataPattern the data pattern to use when writing to the host
	@param aNumBytesPerWrite the number of bytes to write at each call to 'Write' using this data pattern
	@param aTotalNumBytes the total number of bytes to write
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void RepeatedWriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerWrite, TUint aTotalNumBytes, TUint16 aEndpointNumber);
	
	/**
	Write the data cached on the 'source' endpoint to the 'write' endpoint. These endpoints can be found on 
	this interface setting
	@param aSourceEndpointNumber the number of the endpoint on this setting that should contain cached data.
	@param aWriteEndpointNumber the number of the endpoint on this setting that the cached data on 'aSourceEndpointNumber'
			should be written to.
	*/
	void WriteCachedEndpointDataToEndpointL(const TUint16 aSourceEndpointNumber,TUint16 aWriteEndpointNumber);


	/**
	Write the supplied data to the specified endpoint that can be found on 
	this interface setting. Wait for Completion.
	@param aDataPattern the data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write using this data pattern
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void WriteSynchronousSpecifiedDataToEndpointL(const TDesC8& aDataPattern, TUint aNumBytes, TUint16 aEndpointNumber);
	
	/**
	Write the supplied data to the specified endpoint that can be found on 
	this interface setting, and halt that endpoint.  Wait for Completion.
	@param aDataPattern the data pattern to use when writing to the host
	@param aNumBytes the number of bytes to write using this data pattern
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void WriteSynchronousSpecifiedDataToAndHaltEndpointL(const TDesC8& aDataPattern, TUint aNumBytes, TUint16 aEndpointNumber);

	/**
	Synchronously write the data cached on the 'source' endpoint to the 'write' endpoint. These endpoints can be found on 
	this interface setting.  Wait for Completion.
	@param aSourceEndpointNumber the number of the endpoint on this setting that should contain cached data.
	@param aWriteEndpointNumber the number of the endpoint on this setting that the cached data on 'aSourceEndpointNumber'
			should be written to.
	*/
	void WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aSourceEndpointNumber,TUint16 aWriteEndpointNumber);

	/**
	Synchronously write a section of the data cached on the 'source' endpoint to the 'write' endpoint. These endpoints can be found on 
	this interface setting.  Wait for Completion.
	@param aSourceEndpointNumber the number of the endpoint on this setting that should contain cached data.
	@param aWriteEndpointNumber the number of the endpoint on this setting that the cached data on 'aSourceEndpointNumber'
			should be written to.
	@param aStartPoint the beginning of the part of the cached data that is to be written
	@param aLength the length of the part of the cached data that is to be written
	*/
	void WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aSourceEndpointNumber,TUint16 aWriteEndpointNumber, TUint aStartPoint, TUint aLength);
	
	/**
	Get the cached result of a validation from an endpoint
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	@return ETrue if the data is validated, EFalse if not
	*/
	TBool CachedEndpointResultL(const TUint16 aEndpointNumber);

	/**
	Get the cached of the number of bytes read so far on a repeated (asynchronous) 'Read' being perfomed on an endpoint
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	@return the number of bytes read so far
	*/
	TInt NumBytesReadSoFarL(const TUint16 aEndpointNumber);
	
	/**
	Get the cached of the number of bytes written so far on a repeated (asynchronous) 'Write' being perfomed on an endpoint
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	@return the number of bytes written so far
	*/
	TInt NumBytesWrittenSoFarL(const TUint16 aEndpointNumber);
	
	/**
	Validate the data read on the supplied endpoint using the global pattern.
	@param aDataPattern basic data pattern sent by host for comparison
	@param aNumBytes - the number of bytes to validate using that data pattern
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	@return ETrue if the data is validated, EFalse if not
	*/
	TBool ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aNumBytes, const TUint16 aEndpointNumber);
	
	/**
	Validate the data read on the supplied endpoint using the global pattern.
	@param aDataPattern basic data pattern sent by host for comparison
	@param aStartPoint - the point (in bytes) in the data pattern to start (this value is used cyclically - so may be greater than the data pattern length)
	@param aNumBytes - the number of bytes to validate using that data pattern
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	@return ETrue if the data is validated, EFalse if not
	*/
	TBool ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes, const TUint16 aEndpointNumber);
	
	/**
	Read the supplied number of bytes on the specified endpoint that can be found on 
	this interface setting
	@param aNumBytes the amount of data to be read from the host
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void ReadDataFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber);

	/**
	Cancel Read on the specified endpoint that can be found on 
	this interface setting
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void CancelAnyReadDataFromEndpointL(TUint16 aEndpointNumber);

	/**
	Read the supplied number of bytes on the specified endpoint that can be found on 
	this interface setting ... then halt the endpoint
	@param aNumBytes the amount of data to be read from the host
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void ReadDataFromAndHaltEndpointL(TUint aNumBytes, TUint16 aEndpointNumber);

	/**
	Read the supplied number of bytes on the specified endpoint that can be found on 
	this interface setting.
	Do these in sections, performing multiple 'Reads'
	@param aDataPattern the data pattern to use in validation
	@param aNumBytesPerRead the amount of data to be read from the host in each section
	@param aTotalNumBytes the total amount of data to be read from the host
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void RepeatedReadAndValidateFromEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes, TUint16 aEndpointNumber);
/**
	Read the supplied number of bytes (or fewer if a short packet arrives)
	on the specified endpoint that can be found on this interface setting
	@param aNumBytes the amount of data to be read from the host
	@param aEndpointNumber the number of the endpoint on this setting that a host 
	       has an opened pipe to.
	*/
	void ReadDataUntilShortFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber);

	/**
	Get the name of this interface setting
	@return the interface setting name
	*/
	const TDesC& Name() const;
	
private:

	/**
	Disable default constructor
	*/
	CInterfaceSettingBase();

protected:
	
	/**
	The information for this interface setting about endpoints
	*/
	TUsbcInterfaceInfoBuf iInterfaceInfo;
	
	/**
	The name for this interface setting
	*/
	TBuf<64> iSettingString;
	
	/**
	The array of endpoints for this interface setting
	*/
	THashFunction32<TEndpointNumber> iHashEndpointFunction;
	TIdentityRelation<TEndpointNumber> iIdRelEndpoint;
	REndpointMap iEndpoints;
	
	/**
	The readers for the endpoints on this interface setting
	*/
	RPointerArray<CEndpointReader> iEndpointReaders;
	
	/**
	The writers for the endpoints on this interface setting
	*/
	RPointerArray<CEndpointWriter> iEndpointWriters;
	};
	
	}
	
#endif
