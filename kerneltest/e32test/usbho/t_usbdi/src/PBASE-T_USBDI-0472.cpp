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
// @file PBASE-T_USBDI-0472.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0472.h"
#include <e32test.h>
#include <d32usbc.h>
#include <e32debug.h>
#include "UsbDescriptorOffsets.h"
#include "TestPolicy.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0472");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0472,TBool> CUT_PBASE_T_USBDI_0472::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0472* CUT_PBASE_T_USBDI_0472::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0472* self = new (ELeave) CUT_PBASE_T_USBDI_0472(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0472::CUT_PBASE_T_USBDI_0472(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole)
	{
	} 


void CUT_PBASE_T_USBDI_0472::ConstructL()
	{
	iTestDevice = new RUsbDeviceVendor(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0472::~CUT_PBASE_T_USBDI_0472()
	{
	LOG_FUNC
	Cancel();

	delete iClientAction;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}


void CUT_PBASE_T_USBDI_0472::ExecuteHostTestCaseL()
	{
	LOG_FUNC
	iActorFDF = CActorFDF::NewL(*this);
	iClientAction = new (ELeave) CEp0Transfer(iInterface0);
	iCaseStep = EConnectDevice;
	iActorFDF->Monitor();
	TimeoutIn(30);
	}

void CUT_PBASE_T_USBDI_0472::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);	
	SetActive();
	iTestDevice->SoftwareConnect();
	}
	
void CUT_PBASE_T_USBDI_0472::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step action timeout timer
	
	CancelTimeout();
	}


void CUT_PBASE_T_USBDI_0472::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test device
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CUT_PBASE_T_USBDI_0472::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	Cancel();
	TInt err(KErrNone);
	
	// Validate that device is as expected
	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		RDebug::Printf("<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,&testDevice.SerialNumber(),&TestCaseId());

		// Start the connection timeout again

		TimeoutIn(30);
		return;
		}	
		
	// Perform the correct test step
		
	switch(iCaseStep)
		{
		case EConnectDevice:
			{
			// Validate vendor identity
			RDebug::Printf("Vendor identity: 0x%04x",testDevice.VendorId());
			if(testDevice.VendorId() != 0x0E22)
				{
				RDebug::Printf("<Error> Vendor identity is not 0x0E22");
				return TestFailed(KErrCorrupt);
				}
				
			// Validate product information
			RDebug::Printf("Usb device supported specification: 0x%04x",testDevice.DeviceSpec());
			if(testDevice.DeviceSpec() != 0x0200)
				{
				RDebug::Printf("<Error> Usb device supported specification is not 2.0");
				return TestFailed(KErrCorrupt);
				}
			RDebug::Printf("Usb product identity: 0x%04x",testDevice.ProductId());
			if(testDevice.ProductId() != 0x0040)
				{
				RDebug::Printf("<Error> Usb product idenity is not 0x0040");
				return TestFailed(KErrCorrupt);
				}
		
			RDebug::Printf("testDevice.ConfigurationString() = %S",&testDevice.ConfigurationString());		
			gtest(KErrNone == testDevice.ConfigurationString().Compare(KConfigurationString()));		
					
			RDebug::Printf("testDevice.Manufacturer() = %S",&testDevice.Manufacturer());
			gtest(KErrNone == testDevice.Manufacturer().Compare(KManufacturer()));
			
			RDebug::Printf("testDevice.SerialNumber = %s",&testDevice.SerialNumber());	
			gtest(KErrNone == testDevice.SerialNumber().Compare(KTestCaseId()));
		
			RDebug::Printf("Waiting for device removal");
			iCaseStep = ERemoveDevice;
			
			TUint32 token;
			err = testDevice.Device().GetTokenForInterface(0,token);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to get token for interface 0",err);
				return TestFailed(err);
				}
			err = iInterface0.Open(token);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to open interface 0",err);
				return TestFailed(err);
				}
			
			// Send request to client to reconnect in 3 seconds
			
			TReconnectRequest request(3);
			iClientAction->SendRequest(request,this);
			
			// Monitor for the reconnection from the client
			
			iActorFDF->Monitor();
			TimeoutIn(30);
			}
			break;
			
		case EConnectCancelled:
			RDebug::Printf("<Error> Bus event cancellation not successful");
			TestFailed(KErrCorrupt);
			break;
			
		case EPassed:	// Test case has been successfully concluded
			{
			TUint32 token;
			err = testDevice.Device().GetTokenForInterface(0,token);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to get token for interface 0",err);
				return TestFailed(err);
				}
			err = iInterface0.Open(token);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to open interface 0",err);
				return TestFailed(err);
				}
	
			// Send test case passed request to client
			TTestCasePassed request;
			iClientAction->SendRequest(request,this);
			}
			break;
			
		default:
			RDebug::Printf("<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
		}
	}


void CUT_PBASE_T_USBDI_0472::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	
	switch(iCaseStep)
		{
		case EPassed:
			{
			TestPassed();
			}
			break;
			
		case EFailed:
			{
			TestFailed(KErrCompletion);
			}
			break;
	
		case ERemoveDevice:
			{
			// Doing nothing, client should be re-connecting
			}
			break;
			
		default:
			RDebug::Printf("<Error %d> Unknown test step",KErrCorrupt);
			TestFailed(KErrCorrupt);
			break;
		}
	}
	
	
void CUT_PBASE_T_USBDI_0472::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	Cancel();
	
	switch(iCaseStep)
		{
		case ERemoveDevice:
			{
			iCaseStep = EConnectCancelled;
		
			// start timer now
			TimeoutIn(5);		
			}
			break;
			
		default:
			RDebug::Printf("<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0472::BusErrorL(TInt aError)
	{
	// This test case handles no failiures on the bus

	TestFailed(aError);
	}

void CUT_PBASE_T_USBDI_0472::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
	RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	}
	
void CUT_PBASE_T_USBDI_0472::HostRunL()
	{
	LOG_FUNC
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{		
		if(iCaseStep == EConnectCancelled)
			{
			// not a time-out 
			RDebug::Printf("Timer elapsed, reactivating notifications now");
			iCaseStep = EPassed;			
			iActorFDF->Monitor();
			}
		else
			{
			// Action timeout
			RDebug::Printf("<Error> Action timeout");
			TestFailed(KErrTimedOut);
			}	
		}
	else
		{
		RDebug::Printf("<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	}
	

void CUT_PBASE_T_USBDI_0472::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}	
	

	}
	
