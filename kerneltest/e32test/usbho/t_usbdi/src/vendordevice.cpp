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
// @file vendordevice.cpp
// @internalComponent
// 
//

#include "modelleddevices.h"
#include "testinterfacebase.h"
#include "testinterfacesettingbase.h"

namespace NUnitTesting_USBDI
	{
	

RUsbDeviceVendor::RUsbDeviceVendor()
	{
	}
	
RUsbDeviceVendor::RUsbDeviceVendor(CBaseTestCase* aTestCase):RUsbTestDevice(aTestCase)
	{
	LOG_FUNC
	RDebug::Printf("aTestCase = %d", aTestCase);
	}
	

RUsbDeviceVendor::~RUsbDeviceVendor()
	{	
	}
	

void RUsbDeviceVendor::OpenL(const TDesC& aSerialNumber)
	{
	LOG_FUNC
	
	RUsbTestDevice::OpenL();
	
	// Device/Product information
	SetUsbSpecification(0x0200); // Usb spec. 2.0
	SetVendor(0x0E22);
	SetProduct(0x0040, KVendorDevice, KManufacturer,aSerialNumber);
	SetClassCode(0xFF,0xFF,0xFF);
	SetConfigurationString(KConfigurationString);
	
	// Establish a default interface with a default setting
	CInterfaceBase* interface0 = new (ELeave) CInterfaceBase(*this,_L("i0"));
	CleanupStack::PushL(interface0);
	interface0->BaseConstructL();
	
	// Default interface setting 0
	CInterfaceSettingBase* interface0s0 = new (ELeave) CInterfaceSettingBase(_L("i0s0"));
	CleanupStack::PushL(interface0s0);
	interface0s0->SetClassCodeL(0xFF,0xFF,0xFF);
	
	interface0->AddInterfaceSettingL(interface0s0);	
	CleanupStack::Pop(interface0s0);
	AddInterface(interface0);
	CleanupStack::Pop(interface0);
	}


void RUsbDeviceVendor::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	LOG_FUNC
	}

	
	}

