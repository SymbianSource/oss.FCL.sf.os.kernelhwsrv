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
// @file testdeviceD.cpp
// @internalComponent
// 
//

#include "modelleddevices.h"
#include "testinterfacebase.h"
#include "testinterfacesettingbase.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testdeviceDTraces.h"
#endif




namespace NUnitTesting_USBDI
	{
	
RUsbDeviceD::RUsbDeviceD()
	{
	OstTraceFunctionEntry1( RUSBDEVICED_RUSBDEVICED_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICED_RUSBDEVICED_EXIT, this );
	}
	
RUsbDeviceD::RUsbDeviceD(CBaseTestCase* aTestCase):RUsbDeviceVendor(aTestCase)
	{
	OstTraceFunctionEntryExt( RUSBDEVICED_RUSBDEVICED_ENTRY_DUP01, this );
	OstTraceFunctionExit1( RUSBDEVICED_RUSBDEVICED_EXIT_DUP01, this );
	}
	
RUsbDeviceD::~RUsbDeviceD()
	{
	OstTraceFunctionEntry1( RUSBDEVICED_RUSBDEVICED_ENTRY_DUP02, this );
	OstTraceFunctionExit1( RUSBDEVICED_RUSBDEVICED_EXIT_DUP02, this );
	}
	

void RUsbDeviceD::OpenL(const TDesC16& aSerialNumber)
	{
	OstTraceFunctionEntryExt( RUSBDEVICED_OPENL_ENTRY, this );
	
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
	TBulkOutEndpoint if1_s0_ep1(EEndpoint1);
	TBulkInEndpoint if1_s0_ep2(EEndpoint2);
	
	interface1s0->AddEndpoint(if1_s0_ep1);
	interface1s0->AddEndpoint(if1_s0_ep2);

	interface1->AddInterfaceSettingL(interface1s0);	
	CleanupStack::Pop(interface1s0);
	
	// Create alternate interface setting 1
	
	CInterfaceSettingBase* interface1s1 = new (ELeave) CInterfaceSettingBase(_L("i1s1"));
	CleanupStack::PushL(interface1s1);
	interface1s1->SetClassCodeL(0xFF,0xFF,0xFF);
	
	// Create endpoints
	
	TIntInEndpoint if1_s1_ep1(EEndpoint1,64);
	TBulkOutEndpoint if1_s1_ep2(EEndpoint2);
	TBulkInEndpoint if1_s1_ep3(EEndpoint3);
	
	interface1s1->AddEndpoint(if1_s1_ep1);
	interface1s1->AddEndpoint(if1_s1_ep2);
	interface1s1->AddEndpoint(if1_s1_ep3);
	
	interface1->AddInterfaceSettingL(interface1s1);	
	CleanupStack::Pop(interface1s1);
	
	AddInterface(interface1);
	CleanupStack::Pop(interface1);

	

	
	
	// Create another interface (interface2)
	
	CInterfaceBase* interface2 = new (ELeave) CInterfaceBase(*this,_L("i2"));
	CleanupStack::PushL(interface2);
	interface2->BaseConstructL();
	
	// Create default interface setting 0
	
	CInterfaceSettingBase* interface2s0 = new (ELeave) CInterfaceSettingBase(_L("i2s0"));
	CleanupStack::PushL(interface2s0);
	interface2s0->SetClassCodeL(0xFF,0xFF,0xFF);

	// Create some endpoints
	TBulkOutEndpoint if2_s0_ep1(EEndpoint1);
	TBulkOutEndpoint if2_s0_ep2(EEndpoint2); //this endpoint is designed to use a 32 byte max packet size
	TBulkInEndpoint if2_s0_ep3(EEndpoint3);
	
	interface2s0->AddEndpoint(if2_s0_ep1);
	interface2s0->AddEndpoint(if2_s0_ep2);
	interface2s0->AddEndpoint(if2_s0_ep3);

	interface2->AddInterfaceSettingL(interface2s0);	
	CleanupStack::Pop(interface2s0);
	
	AddInterface(interface2);
	CleanupStack::Pop(interface2);
	OstTraceFunctionExit1( RUSBDEVICED_OPENL_EXIT, this );
	}

	
void RUsbDeviceD::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	OstTraceFunctionEntryExt( RUSBDEVICED_ONSTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICED_ONSTATECHANGEL_EXIT, this );
	}	
	
	
	}
