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
// @file testdeviceA.cpp
// @internalComponent
// 
//

#include "modelleddevices.h"
#include "testinterfacebase.h"
#include "testinterfacesettingbase.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testdeviceCTraces.h"
#endif



namespace NUnitTesting_USBDI
	{
	

RUsbDeviceC::RUsbDeviceC()
	{
	OstTraceFunctionEntry1( RUSBDEVICEC_RUSBDEVICEC_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEC_RUSBDEVICEC_EXIT, this );
	}
	
RUsbDeviceC::RUsbDeviceC(CBaseTestCase* aTestCase):RUsbDeviceVendor(aTestCase)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEC_RUSBDEVICEC_ENTRY_DUP01, this );
	OstTraceFunctionExit1( RUSBDEVICEC_RUSBDEVICEC_EXIT_DUP01, this );
	}
	
RUsbDeviceC::~RUsbDeviceC()
	{
	OstTraceFunctionEntry1( RUSBDEVICEC_RUSBDEVICEC_ENTRY_DUP02, this );
	OstTraceFunctionExit1( RUSBDEVICEC_RUSBDEVICEC_EXIT_DUP02, this );
	}
	

void RUsbDeviceC::OpenL(const TDesC16& aSerialNumber)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEC_OPENL_ENTRY, this );
	
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
	OstTraceFunctionExit1( RUSBDEVICEC_OPENL_EXIT, this );
	}

	
void RUsbDeviceC::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEC_ONSTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEC_ONSTATECHANGEL_EXIT, this );
	}	
	
	
	}
