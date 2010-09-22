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
// @file PBASE-T_USBDI-0475.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0475.h"
#include "descriptorsRawData.h"
#include <d32usbdescriptors.h>
#include "testpolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0475Traces.h"
#endif



namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0475");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0475,TBool> CUT_PBASE_T_USBDI_0475::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0475* CUT_PBASE_T_USBDI_0475::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0475* self = new (ELeave) CUT_PBASE_T_USBDI_0475(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0475::CUT_PBASE_T_USBDI_0475(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole,ETrue),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0475_CUT_PBASE_T_USBDI_0475_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_CUT_PBASE_T_USBDI_0475_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0475::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_CONSTRUCTL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0475::~CUT_PBASE_T_USBDI_0475()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_CUT_PBASE_T_USBDI_0475_ENTRY_DUP01, this );
	
	Cancel();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_CUT_PBASE_T_USBDI_0475_EXIT_DUP01, this );
	}
	
	
void CUT_PBASE_T_USBDI_0475::ExecuteHostTestCaseL()	
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_EXECUTEHOSTTESTCASEL_ENTRY, this );
	TInt err(KErrNone);
	
	TUsbGenericDescriptor* parsed = NULL;
	TUsbDeviceDescriptor* devDesc = 0;
	TUsbConfigurationDescriptor* configDesc = 0;
	
	// Parse Device Descriptor
	err	= UsbDescriptorParser::Parse(KDeviceDescriptorData, parsed);
	CHECK(err == KErrNone) // KErrNone
	devDesc = TUsbDeviceDescriptor::Cast(parsed);
	CHECK(devDesc != 0)	
	CHECK(err == KErrNone) // KErrNone
	
	// Parse Configuration Descriptor ---------- Case 1
	err	= UsbDescriptorParser::Parse(KConfigurationDescriptorInsufficientDataTestCase1, parsed);
	configDesc = TUsbConfigurationDescriptor::Cast(parsed);

	CHECK(configDesc == 0)
	CHECK(err == KErrCorrupt) // no tree generated
		
	// Parse Configuration Descriptor ---------- Case 2
	err	= UsbDescriptorParser::Parse(KConfigurationDescriptorInsufficientDataTestCase2, parsed);
	configDesc = TUsbConfigurationDescriptor::Cast(parsed);

	CHECK(configDesc == 0) // tree is freed if there are errors, refer to source code
	CHECK(err == KErrCorrupt) // KErrCorrupt
	
	// Parse Configuration Descriptor ---------- Case 3
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorUnexpectedLengthTestCase3, 3) == KErrNone);

	TestPassed();	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_EXECUTEHOSTTESTCASEL_EXIT, this );
	}   
	
void CUT_PBASE_T_USBDI_0475::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_HOSTDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_HOSTDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0475::ExecuteDeviceTestCaseL()	
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_EXECUTEDEVICETESTCASEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_EXECUTEDEVICETESTCASEL_EXIT, this );
	}	
	
void CUT_PBASE_T_USBDI_0475::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_DEVICEDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_DEVICEDOCANCEL_EXIT, this );
	}	



void CUT_PBASE_T_USBDI_0475::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_HOSTRUNL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_HOSTRUNL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0475::DeviceRunL()
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0475_DEVICERUNL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0475_DEVICERUNL_EXIT, this );
	}

	
	}
