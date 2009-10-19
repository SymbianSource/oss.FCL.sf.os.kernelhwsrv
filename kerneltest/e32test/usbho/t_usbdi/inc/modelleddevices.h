#ifndef __USB_TEST_DEVICES_H
#define __USB_TEST_DEVICES_H

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
* @file modelleddevices.h
* @internalComponent
* 
*
*/



#include "testdevicebase.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KVendorDevice,"VendorDevice");
_LIT(KTestDeviceB, "TestDeviceB");
_LIT(KManufacturer,"Symbian");
_LIT(KConfigurationString,"Test Configuration");
			 
/**
This class models a simple test device which has one default interface with the default interface setting
i.e. a test device that can only connect and disconnect with some basic vendor
and product information.  There are no extra endpoints for this device
*/
class RUsbDeviceVendor : public RUsbTestDevice
	{
public:
	/**
	Constructor, build a default vendor device
	@param aStatus the pointer to the status object for error reporting
	@param aSerialNumber the serial number of the vendor device
	*/
	
	RUsbDeviceVendor();
	
	// ctor
	RUsbDeviceVendor(CBaseTestCase* aTestCase);
	
	/**
	Destructor
	*/
	
	virtual ~RUsbDeviceVendor();

	/**
	Configuration
		Interface0  [setting0]
	*/
	
	void OpenL(const TDesC16& aSerialNumber);

public:
	/**
	Overidden
	*/
	virtual void OnStateChangeL(TUsbcDeviceState aNewState);

private:
	/**
	The serial number for the vendor device
	*/
	TBuf16<64> iSerialNumber;
	};	
	


/**
Describe a device that models this configuration:

Configuration
	Interface0	[setting0]
	Interface1	[setting0]
					[endpoint1] Bulk out
					[endpoint2] Bulk in
				[setting1]			
					[endpoint1] Interrupt in
					[endpoint2] Bulk out
					[endpoint3] Bulk in
*/
class RUsbDeviceA : public RUsbDeviceVendor
	{
public:

	/**
	*/
	RUsbDeviceA();
	
	/**
	*/
	RUsbDeviceA(CBaseTestCase* aTestCase);

	/**
	Destructor
	*/
	virtual ~RUsbDeviceA();

	/**
	*/
	void OpenL(const TDesC& aSerialNumber);		

	
public:
	/**
	Overidden
	*/
	virtual void OnStateChangeL(TUsbcDeviceState aNewState);
	};
	
	
/**
Describe a device that models this configuration:

Configuration	
	Interface0	[setting0]	
					[CS Interface]
					[endpoint1] Bulk out
						[CS Endpoint]
					[endpoint2] Bulk out
						[CS Endpoint]
					[endpoint3] Bulk out
						[CS Endpoint]
	Interface0	[setting1]				
					[CS Interface]
					[endpoint1] Bulk out
						[CS Endpoint]
					[endpoint2] Bulk out
						[CS Endpoint]
					[endpoint3] Bulk out
						[CS Endpoint]
	Interface0	[setting2]				
					[CS Interface]
					[endpoint1] Bulk out
						[CS Endpoint]
					[endpoint2] Bulk out
						[CS Endpoint]
					[endpoint3] Bulk out
						[CS Endpoint] 

*/
class RUsbDeviceB : public RUsbTestDevice
	{
public:
	/**
	*/
	RUsbDeviceB();

	/**
	*/
	RUsbDeviceB(CBaseTestCase* aTestCase);

	/**
	*/
	virtual ~RUsbDeviceB();
	
	/**
	*/
	void OpenL(const TDesC& aSerialNumber);
	
public:
	/**
	Overidden
	*/
	virtual void OnStateChangeL(TUsbcDeviceState aNewState);	
	
private:
	/**
	The serial number for the vendor device
	*/
	TBuf16<64> iSerialNumber;
	};

	
/**
Describe a device that models this configuration:

Configuration
	Interface0	[setting0]
	Interface1	[setting0]
					[endpoint1] Bulk out
					[endpoint2] Bulk in
				[setting1]			
					[endpoint1] Interrupt in
					[endpoint2] Bulk out
					[endpoint3] Bulk in
*/
class RUsbDeviceC : public RUsbDeviceVendor
	{
public:
	/**
	*/
	RUsbDeviceC();
	
	RUsbDeviceC(CBaseTestCase* aTestCase);

	/**
	Destructor
	*/
	virtual ~RUsbDeviceC();

	/**
	*/
	void OpenL(const TDesC& aSerialNumber);		

	
public:
	/**
	Overidden
	*/
	virtual void OnStateChangeL(TUsbcDeviceState aNewState);
	};


/**
Describe a device that models this configuration:

Configuration
	Interface0	[setting0]
	Interface1	[setting0]
					[endpoint1] Bulk out
					[endpoint2] Bulk in
				[setting1]			
					[endpoint1] Interrupt in
					[endpoint2] Bulk out
					[endpoint3] Bulk in
	Interface2	[setting0]
					[endpoint1] Bulk out
					[endpoint2] Bulk in
				[setting1]			
					[endpoint1] Bulk out
					[endpoint2] Bulk out
					[endpoint3] Bulk in
*/
class RUsbDeviceD : public RUsbDeviceVendor
	{
public:

	/**
	*/
	RUsbDeviceD();
	
	/**
	*/
	RUsbDeviceD(CBaseTestCase* aTestCase);

	/**
	Destructor
	*/
	virtual ~RUsbDeviceD();

	/**
	*/
	void OpenL(const TDesC& aSerialNumber);		

	
public:
	/**
	Overidden
	*/
	virtual void OnStateChangeL(TUsbcDeviceState aNewState);
	};

	}

#endif