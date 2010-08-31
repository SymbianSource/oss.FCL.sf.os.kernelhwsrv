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
// @file PBASE-T_USBDI-0476.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0476.h"
#include "descriptorsRawData.h"
#include <d32usbdescriptors.h>
#include "testpolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0476Traces.h"
#endif



namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0476");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0476,TBool> CUT_PBASE_T_USBDI_0476::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0476* CUT_PBASE_T_USBDI_0476::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0476* self = new (ELeave) CUT_PBASE_T_USBDI_0476(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0476::CUT_PBASE_T_USBDI_0476(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole,ETrue),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0476_CUT_PBASE_T_USBDI_0476_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_CUT_PBASE_T_USBDI_0476_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0476::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_CONSTRUCTL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0476::~CUT_PBASE_T_USBDI_0476()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_CUT_PBASE_T_USBDI_0476_ENTRY_DUP01, this );
	
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_CUT_PBASE_T_USBDI_0476_EXIT_DUP01, this );
	}
	

			
void CUT_PBASE_T_USBDI_0476::ExecuteHostTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_EXECUTEHOSTTESTCASEL_ENTRY, this );
	TInt err(KErrNone);
	
	TUsbGenericDescriptor* parsed = NULL;
	TUsbDeviceDescriptor* devDesc = 0;

	// Parse Device Descriptor
	err	= UsbDescriptorParser::Parse(KDeviceDescriptorData, parsed);
	CHECK(err == KErrNone) // KErrNone
	devDesc = TUsbDeviceDescriptor::Cast(parsed);
	CHECK(devDesc != 0)	
	CHECK(err == KErrNone) // KErrNone
	
	// Parse Configuration Descriptor ---------- Case 1
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase1, 1) == KErrNone);	
	
	// Parse Configuration Descriptor ---------- Case 2
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase2, 2) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 3
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase3, 3) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 4
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KBtDongleDescriptorTestCase4			 , 4) == KErrGeneral);
	
	// Parse Configuration Descriptor ---------- Case 5
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase5, 5) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 6
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase6, 6) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 7
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase7, 7) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 8
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase8, 8) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 9
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase9, 9) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 10
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase10, 10) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 11
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase11, 11) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 12
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase12, 12) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 13
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase13, 13) == KErrNone);
	
	// Parse Configuration Descriptor ---------- Case 14
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorWithIADTestCase14, 14) == KErrNone);
	
	TestPassed();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_EXECUTEHOSTTESTCASEL_EXIT, this );
	}   
	
void CUT_PBASE_T_USBDI_0476::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_HOSTDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0476::ExecuteDeviceTestCaseL()	
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_EXECUTEDEVICETESTCASEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_EXECUTEDEVICETESTCASEL_EXIT, this );
	}	
	
void CUT_PBASE_T_USBDI_0476::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_DEVICEDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_DEVICEDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0476::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_HOSTRUNL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_HOSTRUNL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0476::DeviceRunL()
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0476_DEVICERUNL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0476_DEVICERUNL_EXIT, this );
	}

	
	}
