#ifndef __TEST_DEVICE_BASE_H
#define __TEST_DEVICE_BASE_H

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
* @file TestDeviceBase.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbc.h>
#include "UsbClientStateWatcher.h"
#include "ep0reader.h"
#include "BaseTestCase.h"
#include <d32otgdi.h>

namespace NUnitTesting_USBDI
	{
	
// Forward declarations
	
class CInterfaceBase;
class CSoftwareConnectTimer;
class CRemoteWakeupTimer;	

/**
This class represents a test USB device.  There is only one configuration supported
for this representation
*/
class RUsbTestDevice :	public MUsbClientStateObserver,
						public MRequestHandler
	{
public:
	/**
	Destructor
	*/
	
	virtual ~RUsbTestDevice();

	// ctor
	RUsbTestDevice(CBaseTestCase* aTestCase);

	void ResetState();
	
	/**
	Requests error/reports from this test device
	@param aObserverStatus the status of the entity that requests error reports
	*/
	
	void SubscribeToReports(TRequestStatus& aOberverStatus);
	
	/**
	Cancel request for any errors from operations from this device
	*/
	
	void CancelSubscriptionToReports();
	
	/**
	Open a basic device
	@leave KErrNoMemory
	*/
	
	void OpenL();
	
	/**
	Close this device, releaseing all resources
	*/
	
	void Close();

	/**
	Sets specific information for the device
	@param aClassCode the class code that is specified by the standard 
	   00h  Use class information in the Interface Descriptors 
	   02h  Communications and CDC Control (together with interface descriptor)
	   09h  Hub 
	   DCh  Diagnostic Device (together with interface descriptors)
	   EFh  Miscellaneous (together with interface descriptors)
	   FFh  Vendor Specific (together with interface descriptors)
	@param aSubClassCode the subclass code specified by the USB org 
	@param aDeviceProtocol
	*/
	
	TInt SetClassCode(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol);

	/**
	Set the USB specification that this device complies to
	@param aSpecification the specificatio nnumber
	*/
	
	TInt SetUsbSpecification(TUint16 aSpecification);

	/**
	Set the information for the vendor of this device
	@param aVendorId the identity number for the vendor
	*/
	
	TInt SetVendor(TUint16 aVendorId);
	
	/**
	Set the product information
	@param aProductId the identity of the product
	@param aProductString the string used to describe the product
	@param aManufacturerString the string used to describe the manufacturer
	@param aSerialNumberString the product serial number as string
	*/
	
	TInt SetProduct(TUint16 aProductId,const TDesC16& aProductString,const TDesC16& aManufacturerString,
				const TDesC16& aSerialNumberString);

	/**
	Establishes the configuration string for the device
	@param aConfigString the string for the configuration
	*/			
	
	TInt SetConfigurationString(const TDesC16& aConfigString);
				
	/**
	Adds an test interface to the test device
	@param aInterface a pointer to the interface
	*/
	
	void AddInterface(CInterfaceBase* aInterface);
	
	/**
	Access the interfacefor this device identified by the interface index number
	@param aIndex the index of the interface
	@return the interface
	*/
	
	CInterfaceBase& Interface(TInt aIndex);
	
	/**
	Software connect this test device to the host bus.
	The device must already by physically connected.
	*/
	
	void SoftwareConnect();
	
	/**
	Software dis-connect this test device from the host bus.
	The device can be left physically connected
	*/
	
	void SoftwareDisconnect();
	
	/**
	Perform a remote wake-up for the device
	*/
	
	void RemoteWakeup();

	/**
	Notifies the test case of any errors in operations for this test device
	@param aCompletionCode the error code to complete the test case with
	*/
	
	void ReportError(TInt aCompletionCode);	

	/**
	Sends a zero length data packet to acknowledge the request
	*/
	
	void AcknowledgeRequestReceived();
	
public: // From MUsbClientStateObserver

	/**
	Called when the device has changed state
	@param aNewState will hold the new state the device has been placed into by the host
	@param aChangeCompletionCode the operation comletion code
	*/
	void StateChangeL(TUsbcDeviceState aNewState,TInt aChangeCompletionCode);
	
public: // From MRequestHandler
	
	/**
	Process control requests from device endpoint 0
	@param aRequest the control request value
	@param aValue a parameter value for the request
	@param aIndex an index parameter for the request
	@param aDataReqLength the length of the data to be returned to the host
	@param aPayload the data payload sent to the device by the host in a data phase
	*/
	
	virtual TInt ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,
		TUint16 aDataReqLength,const TDesC8& aPayload);
	
protected:

	/**
	Constructor, build a test device
	*/
	
	RUsbTestDevice();

	/**
	Start listening for control requests sent by the host
	*/
	
	void StartEp0Reading();

	/**
	Stop reading control requests from endpoint 0
	*/
	
	void StopEp0Reading();
	
	/**
	Derived devices will be notified through this function when the device has changed state
	and this base class has performed basic functionality
	@param aNewState the new state of the device
	*/
	
	virtual void OnStateChangeL(TUsbcDeviceState aNewState) = 0;


protected:

	RDevUsbcClient iClientDriver; // The USB client driver object
	
	/**
	The device can have many interfaces offering different functions (part functions)
	They are kept and owned by the device
	*/
	RPointerArray<CInterfaceBase> iInterfaces;
	
	/**
	The watcher of states for the USB client 
	*/
	CUsbClientStateWatcher* iStateWatcher;
	TUsbcDeviceState iCurrentState;
	
	/**
	The information about the capabilities of the device.
	This is mostly hardware specific information
	*/
	TUsbDeviceCaps iDeviceCaps;
	
	/**
	The control endpoint 0, reading device directed requests
	*/
	CDeviceEndpoint0* iDeviceEp0;
	
	/**
	The timer required perform any connect, disconnect or re-connections
	*/
	CSoftwareConnectTimer* iConnectTimer;
	
	/**
	The timer that performs any remote wakeups
	*/
	CRemoteWakeupTimer* iWakeupTimer;

	/**
	*/
	TRequestStatus* iObserverStatus;
	
	/**
	A Aux buffer
	*/
	HBufC8* iAuxBuffer;

	CBaseTestCase*	iTestCase;
	
	RProcess iOtgUsbMan;
	};
	
	
	}


#endif