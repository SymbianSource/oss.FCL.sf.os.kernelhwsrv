// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "vendordeviceTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	

RUsbDeviceVendor::RUsbDeviceVendor()
	{
	OstTraceFunctionEntry1( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_EXIT, this );
	}
	
RUsbDeviceVendor::RUsbDeviceVendor(CBaseTestCase* aTestCase):RUsbTestDevice(aTestCase)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_ENTRY_DUP01, this );
	OstTrace1(TRACE_NORMAL, RUSBDEVICEVENDOR_RUSBDEVICEVENDOR, "aTestCase = %d", aTestCase);
	OstTraceFunctionExit1( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_EXIT_DUP01, this );
	}
	

RUsbDeviceVendor::~RUsbDeviceVendor()
	{	
	OstTraceFunctionEntry1( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_ENTRY_DUP02, this );
	OstTraceFunctionExit1( RUSBDEVICEVENDOR_RUSBDEVICEVENDOR_EXIT_DUP02, this );
	}
	

void RUsbDeviceVendor::OpenL(const TDesC& aSerialNumber)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEVENDOR_OPENL_ENTRY, this );
	
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
	OstTraceFunctionExit1( RUSBDEVICEVENDOR_OPENL_EXIT, this );
	}


void RUsbDeviceVendor::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEVENDOR_ONSTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEVENDOR_ONSTATECHANGEL_EXIT, this );
	}

	
	}

