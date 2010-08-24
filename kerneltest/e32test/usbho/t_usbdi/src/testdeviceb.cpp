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
// @file testdeviceb.cpp
// @internalComponent
// 
//

#include "modelleddevices.h"
#include "testinterfacesettingbase.h"
#include "testinterfacebase.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testdevicebTraces.h"
#endif
#include <e32test.h>

namespace NUnitTesting_USBDI
	{

RUsbDeviceB::RUsbDeviceB()
	{
	OstTraceFunctionEntry1( RUSBDEVICEB_RUSBDEVICEB_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEB_RUSBDEVICEB_EXIT, this );
	}
	
RUsbDeviceB::RUsbDeviceB(CBaseTestCase* aTestCase):RUsbTestDevice(aTestCase)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEB_RUSBDEVICEB_ENTRY_DUP01, this );
	OstTraceFunctionExit1( RUSBDEVICEB_RUSBDEVICEB_EXIT_DUP01, this );
	}

RUsbDeviceB::~RUsbDeviceB()
	{
	OstTraceFunctionEntry1( RUSBDEVICEB_RUSBDEVICEB_ENTRY_DUP02, this );
	OstTraceFunctionExit1( RUSBDEVICEB_RUSBDEVICEB_EXIT_DUP02, this );
	}


void RUsbDeviceB::OpenL(const TDesC& aSerialNumber)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEB_OPENL_ENTRY, this );
	
	RUsbTestDevice::OpenL();
	
	// Device/Product information
	SetUsbSpecification(0x0200); // Usb spec. 2.0
	SetVendor(0x0E22);
	SetProduct(0x0040,KTestDeviceB, KManufacturer, aSerialNumber);
	SetClassCode(0xFF,0xFF,0xFF);
	SetConfigurationString(KConfigurationString);
		
	// Establish the an interface
	
	TUsbcInterfaceInfoBuf interfaceSetting;
	TBuf16<64> name(_L("interfaceSetting"));
	interfaceSetting().iString = &name;
	TUsbcEndpointInfo ep(KUsbEpTypeBulk,KUsbEpDirOut,64,0,0);
	interfaceSetting().iEndpointData[0] = ep;
	interfaceSetting().iEndpointData[1] = ep;
	interfaceSetting().iEndpointData[2] = ep;
	interfaceSetting().iTotalEndpointsUsed = 3;

	TBuf8<75> cs_interfaceDescriptor;
	cs_interfaceDescriptor.Fill(0xFF,cs_interfaceDescriptor.MaxLength());
	cs_interfaceDescriptor[0] = 75;
	cs_interfaceDescriptor[1] = KUsbDescType_CS_Interface;

	TBuf8<75> cs_endpointDescriptor;
	cs_endpointDescriptor.FillZ(cs_endpointDescriptor.MaxLength());
	cs_endpointDescriptor[0] = 75;
	cs_endpointDescriptor[1] = KUsbDescType_CS_Endpoint;


	// Interface 0
	
	TInt err(iClientDriver.SetInterface(0,interfaceSetting));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL, "<Error %d> Unable to set interface setting 0",err);
		User::Leave(err);
		}
		
	// class-specific.....
	
	// .....interface
	
	err = iClientDriver.SetCSInterfaceDescriptorBlock(0,cs_interfaceDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP01, "<Error %d> Unable to set class-specific interface setting 0 descriptor",err);
		User::Leave(err);
		}

	// .....endpoints
	
	err = iClientDriver.SetCSEndpointDescriptorBlock(0,1,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP02, "<Error %d> Unable to set class-specific endpoint 1 descriptor on setting 0",err);
		User::Leave(err);
		}
			
	err = iClientDriver.SetCSEndpointDescriptorBlock(0,2,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP03, "<Error %d> Unable to set class-specific endpoint 2 descriptor on setting 0",err);
		User::Leave(err);
		}	
		
	err = iClientDriver.SetCSEndpointDescriptorBlock(0,3,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP04, "<Error %d> Unable to set class-specific endpoint 3 descriptor on setting 0",err);
		User::Leave(err);
		}
	
	// Interface 1
	
	err = iClientDriver.SetInterface(1,interfaceSetting);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP05, "<Error %d> Unable to set interface setting 1",err);
		User::Leave(err);
		}
	
	// class-specific......
	
	// ......interface
	
	err = iClientDriver.SetCSInterfaceDescriptorBlock(1,cs_interfaceDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP06, "<Error %d> Unable to set class-specific interface setting 1 descriptor",err);
		User::Leave(err);
		}

	// .....endpoints
		
	err = iClientDriver.SetCSEndpointDescriptorBlock(1,1,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP07, "<Error %d> Unable to set class-specific endpoint 1 descriptor on setting 1",err);
		User::Leave(err);
		}
	
	err = iClientDriver.SetCSEndpointDescriptorBlock(1,2,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP08, "<Error %d> Unable to set class-specific endpoint 2 descriptor on setting 1",err);
		User::Leave(err);
		}
		
	err = iClientDriver.SetCSEndpointDescriptorBlock(1,3,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP09, "<Error %d> Unable to set class-specific endpoint 3 descriptor on setting 1",err);
		User::Leave(err);
		}

	// Interface 2

	err = iClientDriver.SetInterface(2,interfaceSetting);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP10, "<Error %d> Unable to set interface setting 2",err);
		User::Leave(err);
		}

	// class-specific......
	
	// ......interface	

	err = iClientDriver.SetCSInterfaceDescriptorBlock(2,cs_interfaceDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP11, "<Error %d> Unable to set class-specific interface setting 2 descriptor",err);
		User::Leave(err);
		}

	// ......endpoints		
		
	err = iClientDriver.SetCSEndpointDescriptorBlock(2,1,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP12, "<Error %d> Unable to set class-specific endpoint 1 descriptor on setting 2",err);
		User::Leave(err);
		}
	
	err = iClientDriver.SetCSEndpointDescriptorBlock(2,2,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP13, "<Error %d> Unable to set class-specific endpoint 2 descriptor on setting 2",err);
		User::Leave(err);
		}
		
	err = iClientDriver.SetCSEndpointDescriptorBlock(2,3,cs_endpointDescriptor);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, RUSBDEVICEB_OPENL_DUP14, "<Error %d> Unable to set class-specific endpoint 3 descriptor on setting 2",err);
		User::Leave(err);
		}
	OstTraceFunctionExit1( RUSBDEVICEB_OPENL_EXIT, this );
	}


void RUsbDeviceB::OnStateChangeL(TUsbcDeviceState aNewState)
	{
	OstTraceFunctionEntryExt( RUSBDEVICEB_ONSTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( RUSBDEVICEB_ONSTATECHANGEL_EXIT, this );
	}
	
	
	}


