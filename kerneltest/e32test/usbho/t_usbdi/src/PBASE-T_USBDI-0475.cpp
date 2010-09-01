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
// @file PBASE-T_USBDI-0475.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0475.h"
#include "descriptorsRawData.h"
#include <d32usbdescriptors.h>
#include "testpolicy.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0475");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0475,TBool> CUT_PBASE_T_USBDI_0475::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0475* CUT_PBASE_T_USBDI_0475::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0475* self = new (ELeave) CUT_PBASE_T_USBDI_0475(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0475::CUT_PBASE_T_USBDI_0475(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole,ETrue),
	iCaseStep(EInProgress)
	{
	} 


void CUT_PBASE_T_USBDI_0475::ConstructL()
	{
	LOG_FUNC
	}


CUT_PBASE_T_USBDI_0475::~CUT_PBASE_T_USBDI_0475()
	{
	LOG_FUNC
	
	Cancel();
	}
	
	
void CUT_PBASE_T_USBDI_0475::ExecuteHostTestCaseL()	
	{	
	LOG_FUNC
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

	CHECK(configDesc != 0) // tree should be kept, even if there is a parse error at the end
	CHECK(err == KErrCorrupt) // KErrCorrupt
	
	// Parse Configuration Descriptor ---------- Case 3
	CHECK(ParseConfigDescriptorAndCheckTree(devDesc, KConfigurationDescriptorUnexpectedLengthTestCase3, 3) == KErrNone);

	TestPassed();	
	}   
	
void CUT_PBASE_T_USBDI_0475::HostDoCancel()
	{
	LOG_FUNC	
	}
	
	
void CUT_PBASE_T_USBDI_0475::ExecuteDeviceTestCaseL()	
	{	
	}	
	
void CUT_PBASE_T_USBDI_0475::DeviceDoCancel()
	{
	LOG_FUNC
	}	



void CUT_PBASE_T_USBDI_0475::HostRunL()
	{
	LOG_FUNC
	}


void CUT_PBASE_T_USBDI_0475::DeviceRunL()
	{	
	LOG_FUNC
	}

	
	}
