#ifndef __TEST_INTERFACE_BASE_H
#define __TEST_INTERFACE_BASE_H

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
* @file testinterfacebase.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include "usbclientstatewatcher.h"
#include "controlendpointreader.h"
#include "endpointwriter.h"
#include "endpointstallwatcher.h"


namespace NUnitTesting_USBDI
	{

// Forward declarations

class RUsbTestDevice;
class CInterfaceSettingBase;

/**
This class represents a test USB interface for the test USB device

*/	
class CInterfaceBase : public CBase, public MAlternateSettingObserver, public MRequestHandler
	{
public:
	/**
	Constructor, build an interface for a USB modelled device
	@param aTestDevice the device that this is an interface for
	@param aName the name given to the interface
	*/
	
	CInterfaceBase(RUsbTestDevice& aTestDevice,const TDesC16& aName);
	
	/**
	Base class 2nd phase construction
	*/
	
	void BaseConstructL();
	
	/**
	Destructor
	*/
	
	virtual ~CInterfaceBase();
	
	/**
	Adds an alternate interface setting for this interface
	@param aInterface an alternate interface setting associated with this interface
	*/
	
	void AddInterfaceSettingL(CInterfaceSettingBase* aInterfaceSetting);
	
	/**
	Accesses an alternate interface setting with the specified setting number
	@param aSettingNumber the alternate interface setting number
	@return a referrence to the alternate setting 
	*/
	
	CInterfaceSettingBase& AlternateSetting(TInt aSettingNumber) const;
	
	/**
	Query the number of alternate interface settings for this interface
	@return the number of alternate interface settings
	*/
	
	TInt InterfaceSettingCount() const;
	
	/**
	Start the interface reading interface directed control transfers on endpoint 0
	*/
	
	void StartEp0Reading();
	
	/**
	Stop the interface from reading interface directed control transfers on endpoint 0
	*/
	
	void StopEp0Reading();
	
	/**
	Stall the specified endpoint
	@param aEndpointNumber the endpoint to stall
	@return KErrNone if successful
	*/
	
	TInt StallEndpoint(TUint16 aEndpointNumber);
		
public: // From MAlternateSettingObserver

	/**
	Get notification when the host selects an alternate interface setting on this interface
	@param aAlternateInterfaceSetting the alternate interface setting number	
	*/
	virtual void AlternateInterfaceSelectedL(TInt aAlternateInterfaceSetting);

public: // From MEndpointDataHandler

	/**
	Process any Ep0 control transfer requests that are interface directed
	@param aRequest the request number (id)
	@param aValue the parameter to the request
	@param aIndex the interface number that the request is directed at
	@param aDataReqLength the data size in the transfer DATA1 packet(s)
	@param aPayload the request content data (i.e. all the data from DATA1 packets)
	*/
	virtual TInt ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,TUint16 aDataReqLength,const TDesC8& aPayload);

private:
	TUint32 ExtractNumberL(const TDesC8& aPayload);
	void ExtractTwoNumbersL(const TDesC8& aPayload, TUint32& aFirstNum, TUint32& aSecondNum);

private:
	/**
	The test device object that owns this interface
	*/
	RUsbTestDevice& iDevice;
	
	/**
	The USB client driver
	Only use interface related API
	*/
	RDevUsbcClient iClientDriver;
	
	/**
	The alternate settings for this interface
	*/
	RPointerArray<CInterfaceSettingBase> iAlternateSettings;
	
	/**
	The watcher of alternate interface selection by host
	*/
	CAlternateInterfaceSelectionWatcher* iSelectionWatcher;
	
	/**
	The watcher of endpoint stalling
	*/
	CEndpointStallWatcher* iStallWatcher;
	
	/**
	The reader of interface control endpoint 0
	*/
	CControlEndpointReader* iEp0Reader;
	
	/**
	The writer of interface control endpoint 0 
	*/
	CEndpointWriter* iEp0Writer;
	
	/**
	*/
	TBuf16<64> iInterfaceName;

	/**
	An Auxiliary buffer, to be used for anything temporary
	*/	
	HBufC8* iAuxBuffer;
	
	/**
	The current alternate interface setting that is selected (defaults to zero)
	*/
	TInt iCurrentAlternateInterfaceSetting;
	};
	

	}
	
#endif

