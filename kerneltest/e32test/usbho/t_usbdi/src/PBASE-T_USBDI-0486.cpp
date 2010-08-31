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
// @file PBASE-T_USBDI-0486.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0486.h"
#include "testpolicy.h"
#include "testdebug.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0486Traces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0486");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0486,TBool> CUT_PBASE_T_USBDI_0486::iFunctor(KTestCaseId);	
const TInt KBulkTranferId = 2;
const TInt KExpectedDataSize = 26;

CUT_PBASE_T_USBDI_0486* CUT_PBASE_T_USBDI_0486::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0486* self = new (ELeave) CUT_PBASE_T_USBDI_0486(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_NEWL_EXIT, ( TUint )( self ) );
	return self;  
	} 
	

CUT_PBASE_T_USBDI_0486::CUT_PBASE_T_USBDI_0486(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EWaitForDeviceCConnection)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_CUT_PBASE_T_USBDI_0486_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_CUT_PBASE_T_USBDI_0486_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0486::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();
	iTestDeviceC = new RUsbDeviceC(this);  // TODO check tree for device C, once inserted
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_CONSTRUCTL_EXIT, this );
	}

CUT_PBASE_T_USBDI_0486::~CUT_PBASE_T_USBDI_0486()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_CUT_PBASE_T_USBDI_0486_ENTRY_DUP01, this );
	
	delete iTestDeviceC;
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_CUT_PBASE_T_USBDI_0486_EXIT_DUP01, this );
	}
	
/**
Called when the device has reported any kind of error in its operation
or when the device has been informed by the host to report success
*/
void CUT_PBASE_T_USBDI_0486::DeviceRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_DEVICERUNL_ENTRY, this );
	// Complete the test case request	
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_DEVICERUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0486::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_DEVICEDOCANCEL_ENTRY, this );
	iTestDeviceC->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_DEVICEDOCANCEL_EXIT, this );
	}
	
TBool CUT_PBASE_T_USBDI_0486::CheckSN(const TDesC16& aSerialNumberGot, const TDesC& aExpectedSerialNumber)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_CHECKSN_ENTRY, this );
	TBool areSNsIdenticals = (aSerialNumberGot.Compare(aExpectedSerialNumber) == 0);	
	 
	if(!areSNsIdenticals)
		{
		// Incorrect device for this test case	
		OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_CHECKSN, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,aSerialNumberGot, aExpectedSerialNumber);
		}
	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0486_CHECKSN_EXIT, this, areSNsIdenticals );
	return areSNsIdenticals;
	}
	
	
void CUT_PBASE_T_USBDI_0486::Ep0TransferCompleteL(TInt aCompletionCode)
	{
    OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_EP0TRANSFERCOMPLETEL_ENTRY, this );

	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_EP0TRANSFERCOMPLETEL_DUP01, "--->Ep0TransferCompleteL, case step = %d", iCaseStep);

	switch(iCaseStep)
		{
		case EDeviceCConnected:
			{
			if(aCompletionCode != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_EP0TRANSFERCOMPLETEL_DUP02, "<Error %d> aCompletionCode != KErrNone",aCompletionCode);
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0486::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_TRANSFERCOMPLETEL, "--->TransferCompleteL, case step = %d", iCaseStep);

	if(aTransferId == KBulkTranferId)
		{							
		if(aCompletionCode != KErrCancel && aCompletionCode != KErrUsbIOError)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> The transfer completed with no errors but should have done so"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_TRANSFERCOMPLETEL_DUP01, msg);
			TTestCaseFailed request(KErrCorrupt,msg);
			return iControlEp0->SendRequest(request,this);
			} 					
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_TRANSFERCOMPLETEL_DUP02, "<Error> a transfer completed (id=%d) that was not expected",aTransferId);
		return TestFailed(KErrCorrupt);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_TRANSFERCOMPLETEL_EXIT, this );
	}	
	
	
void CUT_PBASE_T_USBDI_0486::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL, "--->DeviceInsertedL, case step = %d", iCaseStep);

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
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP01, "<Error %d> Unable to retrieve token for interface 0",err);
				TestFailed(err);
				} 
			// Open the interface	
			err = iUsbInterface0.Open(iToken0DeviceC);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP02, "<Error %d> Unable to open interface 0", err);
				TestFailed(err);
				}
				
			TUint32 token1;
			err = testDevice.Device().GetTokenForInterface(1,token1);
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP03, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
				
				
			err = iUsbInterface1.Open(token1); // Default interface setting 0
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token1);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP04, msg);
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
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP05, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP06, "Endpoint adress %08x",endpointAddress);
	
			err = iUsbInterface1.OpenPipeForEndpoint(iTestPipeInterface1BulkIn,endpointAddress,ETrue);
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP07, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}
			
			// Create the bulk transfers	
			iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,256,*this, KBulkTranferId);
		
			// Initialise the descriptors for transfer		
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP08, "Initialising the transfer descriptors");
			err = iUsbInterface1.InitialiseTransferDescriptors();
			if(err != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Unable to initialise transfer descriptors"),err);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP09, msg);
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
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP10, "iUsbInterface0.Open(iToken0DeviceC) === %d", err);
				TestFailed(err);
				}
			
			// get token now 
			TUint32 token0DeviceA; 
			err = testDevice.Device().GetTokenForInterface(0,token0DeviceA);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP11, "<Error %d> Unable to retrieve token(device A) for interface 0",err);
				TestFailed(err);
				} 
			// Open the interface	
			err = iUsbInterface0.Open(token0DeviceA);
			if(err != KErrNone)
				{
				OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP12, "<Error %d> Unable to open interface 0(device A)", err);
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_DUP13, "<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			}	
			break;
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_DEVICEINSERTEDL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0486::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0486_DEVICEREMOVEDL_ENTRY, this );
	Cancel();	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEREMOVEDL, "--->DeviceRemovedL, case step = %d", iCaseStep);

	switch(iCaseStep)
		{
		case EWaitForDeviceCDisconnection: // device C is disconnected now, interface 0 has been closed before
			{
			iCaseStep = EWaitForDeviceAConnection;
			iActorFDF->Monitor();
			}
			break;		
	
		default:
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0486_DEVICEREMOVEDL_DUP01, "<Error> Test case actions out of sync");
			TestFailed(KErrCorrupt);
			break;
			}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_DEVICEREMOVEDL_EXIT, this );
	}
	
void CUT_PBASE_T_USBDI_0486::HandleDeviceDConnection()
	{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_HANDLEDEVICEDCONNECTION_ENTRY, this );
		
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_HANDLEDEVICEDCONNECTION_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0486::ExecuteDeviceTestCaseL()	
	{ 
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_EXECUTEDEVICETESTCASEL_ENTRY, this );
	
	// Create the test device	
	iTestDeviceC->OpenL(KTestDeviceC_SN());
	iTestDeviceC->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host	
	iTestDeviceC->SoftwareConnect();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0486_EXECUTEDEVICETESTCASEL_EXIT, this );
	}

RUsbDeviceC* CUT_PBASE_T_USBDI_0486::TestDeviceC()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_TESTDEVICEC_ENTRY, this );
	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0486_TESTDEVICEC_EXIT, this, ( TUint )( iTestDeviceC ) );
	return iTestDeviceC;
	}

RUsbDeviceD* CUT_PBASE_T_USBDI_0486::TestDeviceD()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0486_TESTDEVICED_ENTRY, this );
	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0486_TESTDEVICED_EXIT, this, ( TUint )( iTestDevice ) );
	return iTestDevice; //from CBaseBulkTestCase
	}
	
	}
