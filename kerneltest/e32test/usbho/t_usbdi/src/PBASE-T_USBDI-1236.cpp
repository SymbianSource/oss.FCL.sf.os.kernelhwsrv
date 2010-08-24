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
// @file PBASE-T_USBDI-1236.cpp
// @internalComponent
//



#include "PBASE-T_USBDI-1236.h"
#include <d32usbc.h>
#include <d32usbdescriptors.h>
#include "testdebug.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1236Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-1236");
const TFunctorTestCase<CUT_PBASE_T_USBDI_1236,TBool> CUT_PBASE_T_USBDI_1236::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_1236* CUT_PBASE_T_USBDI_1236::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_1236* self = new (ELeave) CUT_PBASE_T_USBDI_1236(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_NEWL_EXIT, ( TUint )( self ) );
	return self; 
	}
	   

CUT_PBASE_T_USBDI_1236::CUT_PBASE_T_USBDI_1236(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_CUT_PBASE_T_USBDI_1236_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_CUT_PBASE_T_USBDI_1236_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_1236::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_CONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceA(this);
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_1236::~CUT_PBASE_T_USBDI_1236()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_CUT_PBASE_T_USBDI_1236_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_CUT_PBASE_T_USBDI_1236_EXIT_DUP01, this );
	}


void CUT_PBASE_T_USBDI_1236::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_EXECUTEHOSTTESTCASEL_ENTRY, this );
	iCaseStep = EStepGetInterfaceString;
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	
	// Monitor for device connections
	iActorFDF->Monitor();

	// Start the connection timeout	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1236::ExecuteDeviceTestCaseL()
	{
    OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_EXECUTEDEVICETESTCASEL_ENTRY, this );

	// Construct the device for the test case
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the test device	
	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_1236::DeviceInsertedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_ENTRY, this );

	Cancel(); // Cancel the timer
	TInt err(KErrNone);
	iDeviceHandle = aDeviceHandle;
	iActorFDF->Monitor();
	
	// Validate that device is as expected
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case

		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_EXIT, this );
		return;
		}	
		
	// Perform the correct test step				
	switch(iCaseStep)
		{
		case EStepGetInterfaceString:
			{
			TUint32 token1(0);
			TUint32 token2(0);
	
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP01, "Obtaining token for interface 0");
			err = testDevice.Device().GetTokenForInterface(0,token1);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP02, "<Error %d> Token for interface 0 could not be retrieved",err);
				return TestFailed(err);
				}
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP03, "Token 1 (%d) retrieved",token1);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP04, "Opening interface 0");
			err = iUsbInterface0.Open(token1); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP05, "<Error %d> Interface 0 could not be opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP06, "Interface 0 opened");
		
																
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP07, "Obtaining token for interface 1");
			err = testDevice.Device().GetTokenForInterface(1,token2);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP08, "<Error %d> Token for interface 1 could not be retrieved",err);
				return TestFailed(err);			
				}	
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP09, "Opening interface 1");
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP10, "<Error %d> Interface 1 could not be opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP11, "Interface 1 opened");
	
			// close it
			iUsbInterface1.Close();		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP12, "Interface 1 closed");
	
			//re-open now
			err = iUsbInterface1.Open(token2); // Alternate interface setting 0
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP13, "<Error %d> Interface 1 could not be re-opened",err);
				return TestFailed(err);
				}
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP14, "Interface 1 re-opened");
			

			
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
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP15, "interface String match");
				iCaseStep = EPassed;
				TTestCasePassed request;
				iControlEp0->SendRequest(request,this);
				}
			else
				{
				OstTraceExt1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_DUP16, "interface String not match %s", interfaceString);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICEINSERTEDL_EXIT_DUP01, this );
	}


	
void CUT_PBASE_T_USBDI_1236::DeviceRemovedL(TUint aDeviceHandle)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_1236::BusErrorL(TInt aError)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus

	TestFailed(aError);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_BUSERRORL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_1236::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_DEVICESTATECHANGEL, "Device State change from %d to %d err=%d",aPreviousState,aNewState,aCompletionCode);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICESTATECHANGEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1236::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1236_EP0TRANSFERCOMPLETEL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_EP0TRANSFERCOMPLETEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1236::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1236_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_HOSTRUNL_EXIT, this );
	}
	

void CUT_PBASE_T_USBDI_1236::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_DEVICERUNL_ENTRY, this );
	
	// Disconnect the device
	
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICERUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_1236::HostDoCancel()
	{
OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_HOSTDOCANCEL_ENTRY, this );

	// Cancel the timeout timer
	CancelTimeout();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_HOSTDOCANCEL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_1236::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1236_DEVICEDOCANCEL_ENTRY, this );
	
	// Cancel the device	
	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1236_DEVICEDOCANCEL_EXIT, this );
	}
	
	}
