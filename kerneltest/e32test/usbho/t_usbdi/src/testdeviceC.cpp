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
// @file testdeviceA.cpp
// @internalComponent
// 
//

#include "modelleddevices.h"
#include "testinterfacebase.h"
#include "testinterfacesettingbase.h"


namespace NUnitTesting_USBDI
	{
	

RUsbDeviceC::RUsbDeviceC()
	{
	LOG_FUNC	
	}
	
RUsbDeviceC::RUsbDeviceC(CBaseTestCase* aTestCase):RUsbDeviceVendor(aTestCase)
	{
	LOG_FUNC
	}
	
RUsbDeviceC::~RUsbDeviceC()
	{
	LOG_FUNC	
	}
	

void RUsbDeviceC::OpenL(const TDesC16& aSerialNumber)
	{
	LOG_FUNC
	
	RUsbDeviceVendor::OpenL(aSerialNumber);
	
	// Create another interface (interface1)	
	CInterfaceBase* interface1 = new (ELeave) CInterfaceBase(*this,_L("i1"));
	CleanupStack::PushL(interface1);
	interface1->BaseConstructL();
	
	// Create default interface setting 0	
	CInterfaceSettingBase* interface1s0 = new (ELeave) CInterfaceSettingBase(_L("i1s0"));
	CleanupStack::PushL(interface1s0);
	interface1s0->SetClassCodeL(0xFF,0xFF,0xFF);

	// Create some endpoints
	TBulkOutEndpoint s0_ep1(EEndpoint1);
	TBulkInEndpoint s0_ep2(EEndpoint2);
	
	interface1s0->AddEndpoint(s0_ep1);
	interface1s0->AddEndpoint(s0_ep2);

	interface1->AddInterfaceSettingL(interface1s0);	
	CleanupStack::Pop(interface1s0);
	
	// Create alternate interface setting 1	
	CInterfaceSettingBase* interface1s1 = new (ELeave) CInterfaceSettingBase(_L("i1s1"));
	CleanupStack::PushL(interface1s1);
	interface1s1->SetClassCodeL(0xFF,0xFF,0xFF);
	
	// Create endpoints	
	TIntInEndpoint s1_ep1(EEndpoint1,64);
	TBulkOutEndpoint s1_ep2(EEndpoint2);
	TBulkInEndpoint s1_ep3(EEndpoint3);
	
	interface1s1->AddEndpoint(s1_ep1);
	interface1s1->AddEndpoint(s1_ep2);
	interface1s1->AddEndpoint(s1_ep3);
	
	interface1->AddInterfaceSettingL(interface1s1);	
	CleanupStack::Pop(interface1s1);
	
	
	// Create alternate interface setting 2	
	CInterfaceSettingBase* interface1s2 = new (ELeave) CInterfaceSettingBase(_L("i1s2"));
	CleanupStack::PushL(interface1s2);
	interface1s2->SetClassCodeL(0xFF,0xFF,0xFF);
							
	// Create endpoints 		
	TBulkOutEndpoint s2_ep1(EEndpoint1);
	TBulkInEndpoint s2_ep2(EEndpoint2);

	
	interface1s2->AddEndpoint(s2_ep1);
	interface1s2->AddEndpoint(s2_ep2);

	
	interface1->AddInterfaceSettingL(interface1s2);	
	CleanupStack::Pop(interface1s2);
	
	AddInterface(interface1);
	CleanupStack::Pop(interface1);
	}

	
void RUsbDeviceC::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	LOG_FUNC
	}	
	
	
	}
