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
// @file PBASE-T_USBDI-0476.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0476.h"
#include "descriptorsRawData.h"
#include <d32usbdescriptors.h>
#include "testpolicy.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0476");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0476,TBool> CUT_PBASE_T_USBDI_0476::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0476* CUT_PBASE_T_USBDI_0476::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0476* self = new (ELeave) CUT_PBASE_T_USBDI_0476(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0476::CUT_PBASE_T_USBDI_0476(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole,ETrue),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0476::ConstructL()
	{
	LOG_FUNC
	}


CUT_PBASE_T_USBDI_0476::~CUT_PBASE_T_USBDI_0476()
	{
	LOG_FUNC
	
	Cancel();
	}
	

			
void CUT_PBASE_T_USBDI_0476::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
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
	}   
	
void CUT_PBASE_T_USBDI_0476::HostDoCancel()
	{
	LOG_FUNC	
	}
	
	
void CUT_PBASE_T_USBDI_0476::ExecuteDeviceTestCaseL()	
	{	
	}	
	
void CUT_PBASE_T_USBDI_0476::DeviceDoCancel()
	{
	LOG_FUNC
	}


void CUT_PBASE_T_USBDI_0476::HostRunL()
	{
	LOG_FUNC
	}


void CUT_PBASE_T_USBDI_0476::DeviceRunL()
	{	
	LOG_FUNC
	}

	
	}
