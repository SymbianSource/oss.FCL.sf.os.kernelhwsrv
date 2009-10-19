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
// @file PBASE-T_USBDI-1236.cpp
// @internalComponent
//



#include "PBASE-T_USBDI-1236.h"
#include <d32usbc.h>
#include <d32usbdescriptors.h>
#include "testdebug.h"
#include "TestPolicy.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-1236");
const TFunctorTestCase<CUT_PBASE_T_USBDI_1236,TBool> CUT_PBASE_T_USBDI_1236::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_1236* CUT_PBASE_T_USBDI_1236::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_1236* self = new (ELeave) CUT_PBASE_T_USBDI_1236(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self; 
	}
	   

CUT_PBASE_T_USBDI_1236::CUT_PBASE_T_USBDI_1236(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole)
	{
	} 


void CUT_PBASE_T_USBDI_1236::ConstructL()
	{
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_1236::~CUT_PBASE_T_USBDI_1236()
	{
	LOG_FUNC
	
	// Cancel any async operations
	
	Cancel(); // Cancel host timer
	
	// Close the interfaces
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	
	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}		
	delete iTestDevice;
	}


void CUT_PBASE_T_USBDI_1236::ExecuteHostTestCaseL()
	{
	LOG_FUNC
	iCaseStep = EStepGetInterfaceString;
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for device connections
	iActorFDF->Monitor();

	// Start the connection timeout	
	TimeoutIn(30);
	}

void CUT_PBASE_T_USBDI_1236::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC

	// Construct the device for the test case
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device	
	iTestDevice->SoftwareConnect();
	}
	
	
void CUT_PBASE_T_USBDI_1236::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	Cancel(); // Cancel the timer
	TInt err(KErrNone);
	iDeviceHandle = aDeviceHandle;
	iActorFDF->Monitor();
	
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
		case EStepGetInterfaceString:
			{
			TUint32 token1(0);
			TUint32 token2(0);
	
			RDebug::Printf("Obtaining token for interface 0");
			err = testDevice.Device().GetTokenForInterface(0,token1);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);
				return TestFailed(err);
				}
			RDebug::Printf("Token 1 (%d) retrieved",token1);
			RDebug::Printf("Opening interface 0");
			err = iUsbInterface0.Open(token1); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 0 could not be opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 0 opened");
		
																
			RDebug::Printf("Obtaining token for interface 1");
			err = testDevice.Device().GetTokenForInterface(1,token2);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Token for interface 1 could not be retrieved",err);
				return TestFailed(err);			
				}	
			RDebug::Printf("Opening interface 1");
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 1 could not be opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 1 opened");
	
			// close it
			iUsbInterface1.Close();		
			RDebug::Printf("Interface 1 closed");
	
			//re-open now
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Interface 1 could not be re-opened",err);
				return TestFailed(err);
				}
			RDebug::Printf("Interface 1 re-opened");
			

			
			TBuf8<256> interfaceStringDes;
			const TUint16 KLangIdUsEng = 0x0409;
			TUsbInterfaceDescriptor interfaceDescriptor;
			iUsbInterface1.GetInterfaceDescriptor(interfaceDescriptor);
			TUint8 index = interfaceDescriptor.Interface();
			iUsbInterface1.GetStringDescriptor(interfaceStringDes, index, KLangIdUsEng);
			TUint8 length = interfaceStringDes[0];
			TUint8 type = interfaceStringDes[1];
			TPtr8 interfaceString = interfaceStringDes.MidTPtr(2, length-2);
			TBuf16<128> u16String((TUint16*)interfaceString.Ptr());
			u16String.SetLength(interfaceString.Length()/2);
			if (u16String.Compare(_L16("i1s0")) == 0)
				{
				RDebug::Printf("interface String match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				RDebug::Printf("interface String not match %S", &interfaceString);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCompletion, _L8("interface string not match"));
				iControlEp0->SendRequest(request,this);
				}
			}
			break;
			
		default:
			TestFailed(KErrCorrupt);
			break;
		}	
	}


	
void CUT_PBASE_T_USBDI_1236::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	}
	
	
void CUT_PBASE_T_USBDI_1236::BusErrorL(TInt aError)
	{
	LOG_FUNC

	// This test case handles no failiures on the bus

	TestFailed(aError);
	}
	
void CUT_PBASE_T_USBDI_1236::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	RDebug::Printf("Device State change from %d to %d err=%d",aPreviousState,aNewState,aCompletionCode);
	}


void CUT_PBASE_T_USBDI_1236::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	switch(iCaseStep)
		{
		// Fail the test case
		default:
		case EFailed:
			TestFailed(KErrCompletion);
			break;
	
		// Pass the test case
	
		case EPassed:
			TestPassed();
			break;
			}
	}


void CUT_PBASE_T_USBDI_1236::HostRunL()
	{
	LOG_FUNC
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		RDebug::Printf("<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		RDebug::Printf("<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	}
	

void CUT_PBASE_T_USBDI_1236::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

void CUT_PBASE_T_USBDI_1236::HostDoCancel()
	{
	LOG_FUNC

	// Cancel the timeout timer
	CancelTimeout();
	}


void CUT_PBASE_T_USBDI_1236::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	}
