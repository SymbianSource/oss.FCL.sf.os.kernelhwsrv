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
// @file PBASE-T_USBDI-0486.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0486.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0486");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0486,TBool> CUT_PBASE_T_USBDI_0486::iFunctor(KTestCaseId);	
const TInt KBulkTranferId = 2;
const TInt KExpectedDataSize = 26;

CUT_PBASE_T_USBDI_0486* CUT_PBASE_T_USBDI_0486::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0486* self = new (ELeave) CUT_PBASE_T_USBDI_0486(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;  
	} 
	

CUT_PBASE_T_USBDI_0486::CUT_PBASE_T_USBDI_0486(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EWaitForDeviceCConnection)
	{
	} 


void CUT_PBASE_T_USBDI_0486::ConstructL()
	{
	BaseBulkConstructL();
	iTestDeviceC = new RUsbDeviceC(this);  // TODO check tree for device C, once inserted
	}

CUT_PBASE_T_USBDI_0486::~CUT_PBASE_T_USBDI_0486()
	{
	LOG_FUNC
	
	delete iTestDeviceC;
	}
	
/**
Called when the device has reported any kind of error in its operation
or when the device has been informed by the host to report success
*/
void CUT_PBASE_T_USBDI_0486::DeviceRunL()
	{
	LOG_FUNC	
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

void CUT_PBASE_T_USBDI_0486::DeviceDoCancel()
	{
	LOG_FUNC
	iTestDeviceC->CancelSubscriptionToReports();
	}
	
TBool CUT_PBASE_T_USBDI_0486::CheckSN(const TDesC16& aSerialNumberGot, const TDesC& aExpectedSerialNumber)
	{
	TBool areSNsIdenticals = (aSerialNumberGot.Compare(aExpectedSerialNumber) == 0);	
	 
	if(!areSNsIdenticals)
		{
		// Incorrect device for this test case	
		RDebug::Printf("<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,&aSerialNumberGot, &aExpectedSerialNumber);
		}
	return areSNsIdenticals;
	}
	
	
void CUT_PBASE_T_USBDI_0486::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC

	RDebug::Printf("Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	RDebug::Printf("--->Ep0TransferCompleteL, case step = %d", iCaseStep);

	switch(iCaseStep)
		{
		case EDeviceCConnected:
			{
			if(aCompletionCode != KErrNone)
				{
				RDebug::Printf("<Error %d> aCompletionCode != KErrNone",aCompletionCode);
				return TestFailed(aCompletionCode);
				}
			// close interface 0			
			iUsbInterface0.Close();			
			iCaseStep = EWaitForDeviceCDisconnection;
			}
			break; 
			
		case EPassed:
			TestPassed();
			break;		
	
		default:
			TestFailed(KErrCompletion);
			break;
		}
	}
	
void CUT_PBASE_T_USBDI_0486::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	RDebug::Printf("--->TransferCompleteL, case step = %d", iCaseStep);

	if(aTransferId == KBulkTranferId)
		{							
		if(aCompletionCode != KErrCancel && aCompletionCode != KErrUsbIOError)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> The transfer completed with no errors but should have done so"),aCompletionCode);
			RDebug::Print(msg);
			TTestCaseFailed request(KErrCorrupt,msg);
			return iControlEp0->SendRequest(request,this);
			} 					
		}
	else
		{
		RDebug::Printf("<Error> a transfer completed (id=%d) that was not expected",aTransferId);
		return TestFailed(KErrCorrupt);
		}	
	}	
	
	
void CUT_PBASE_T_USBDI_0486::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	RDebug::Printf("--->DeviceInsertedL, case step = %d", iCaseStep);

	// Cancel the timeout timer	
	Cancel();	
	TInt err(KErrNone);
	
	// Validate that device is as expected	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);

	// Perform the correct test step		
	switch(iCaseStep)
		{
		case EWaitForDeviceCConnection:
			{
			if(!CheckSN(testDevice.SerialNumber(), KTestDeviceC_SN()))
				{
				return TestFailed(KErrNotFound);
				}
			iCaseStep = EDeviceCConnected;
			
			err = testDevice.Device().GetTokenForInterface(0,iToken0DeviceC);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to retrieve token for interface 0",err);
				TestFailed(err);
				} 
			// Open the interface	
			err = iUsbInterface0.Open(iToken0DeviceC);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to open interface 0");
				TestFailed(err);
				}
				
			TUint32 token1;
			err = testDevice.Device().GetTokenForInterface(1,token1);
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
				
				
			err = iUsbInterface1.Open(token1); // Default interface setting 0
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
				
			TInt endpointAddress;
			// Queue a bulk in transfer on endpoint 2	
			err = GetEndpointAddress(iUsbInterface1,0,KTransferTypeBulk,KEpDirectionIn,endpointAddress);
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Address for bulk in endpoint could not be obtained"),err);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
			RDebug::Printf("Endpoint adress %08x",endpointAddress);
	
			err = iUsbInterface1.OpenPipeForEndpoint(iTestPipeInterface1BulkIn,endpointAddress,ETrue);
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
			
			// Create the bulk transfers	
			iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,256,*this, KBulkTranferId);
		
			// Initialise the descriptors for transfer		
			RDebug::Printf("Initialising the transfer descriptors");
			err = iUsbInterface1.InitialiseTransferDescriptors();
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
				RDebug::Print(msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}				
			iInTransfer[0]->TransferIn(KExpectedDataSize);
					
			// 	disconnect device C and connect device A								
			TDisconnectDeviceCThenConnectDeviceARequest request;
			iControlEp0->SendRequest(request,this);	
	        										
			// Monitor for the reconnection from the client		
			iActorFDF->Monitor();
			TimeoutIn(30);
						
			}
			break; 
					
		case EWaitForDeviceAConnection:
			{
			if(!CheckSN(testDevice.SerialNumber(), TestCaseId()))
				{
				return TestFailed(KErrNotFound);
			    }
			// try to open interface 0			
			err = iUsbInterface0.Open(iToken0DeviceC);
			if(err != KErrNotFound) // invalid token
				{
				RDebug::Printf("iUsbInterface0.Open(iToken0DeviceC) === %d", err);
				TestFailed(err);
				}
			
			// get token now 
			TUint32 token0DeviceA; 
			err = testDevice.Device().GetTokenForInterface(0,token0DeviceA);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to retrieve token(device A) for interface 0",err);
				TestFailed(err);
				} 
			// Open the interface	
			err = iUsbInterface0.Open(token0DeviceA);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> Unable to open interface 0(device A)");
				TestFailed(err);
				}
			
			// ok, send EPassed request			
			iCaseStep = EPassed;
			// Send test case passed request to client
			User::After(2000000);	
			TTestCasePassed request;
			iControlEp0->SendRequest(request,this);			
			} 
			break;
			
		default:
			{
			RDebug::Printf("<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			}	
			break;
		}	
	}
	
void CUT_PBASE_T_USBDI_0486::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	Cancel();	
	RDebug::Printf("--->DeviceRemovedL, case step = %d", iCaseStep);

	switch(iCaseStep)
		{
		case EWaitForDeviceCDisconnection: // device C is disconnected now, interface 0 has been closed before
			{
			iCaseStep = EWaitForDeviceAConnection;
			iActorFDF->Monitor();
			}
			break;		
	
		default:
			RDebug::Printf("<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
			}
	}
	
void CUT_PBASE_T_USBDI_0486::HandleDeviceDConnection()
	{
	LOG_FUNC
		
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	}

void CUT_PBASE_T_USBDI_0486::ExecuteDeviceTestCaseL()	
	{ 
	LOG_FUNC
	
	// Create the test device	
	iTestDeviceC->OpenL(KTestDeviceC_SN());
	iTestDeviceC->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host	
	iTestDeviceC->SoftwareConnect();
	}

RUsbDeviceC* CUT_PBASE_T_USBDI_0486::TestDeviceC()
	{
	return iTestDeviceC;
	}

RUsbDeviceD* CUT_PBASE_T_USBDI_0486::TestDeviceD()
	{
	return iTestDevice; //from CBaseBulkTestCase
	}
	
	}
