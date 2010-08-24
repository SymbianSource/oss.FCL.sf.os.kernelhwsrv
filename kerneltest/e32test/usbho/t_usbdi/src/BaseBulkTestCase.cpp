// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file BaseBulkTestCase.cpp
// @internalComponent
//
//

#include "BaseBulkTestCase.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "BaseBulkTestCaseTraces.h"
#endif



namespace NUnitTesting_USBDI
	{

//*****************************************************************************************************

//Bulk Timer Class
CBulkTestTimer* CBulkTestTimer::NewL(MBulkTestTimerObserver& aParent)
	{
	OstTraceFunctionEntry1( CBULKTESTTIMER_NEWL_ENTRY, ( TUint )&( aParent ) );
	CBulkTestTimer* self = new (ELeave) CBulkTestTimer(aParent);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CBULKTESTTIMER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}


CBulkTestTimer::CBulkTestTimer(MBulkTestTimerObserver& aParent)
:	CTimer(EPriorityStandard),
	iParent(aParent)
	{
	OstTraceFunctionEntryExt( CBULKTESTTIMER_CBULKTESTTIMER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CBULKTESTTIMER_CBULKTESTTIMER_EXIT, this );
	}


CBulkTestTimer::~CBulkTestTimer()
	{
	OstTraceFunctionEntry1( CBULKTESTTIMER_CBULKTESTTIMER_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CBULKTESTTIMER_CBULKTESTTIMER_EXIT_DUP01, this );
	}


void CBulkTestTimer::ConstructL()
	{
	OstTraceFunctionEntry1( CBULKTESTTIMER_CONSTRUCTL_ENTRY, this );
	CTimer::ConstructL();
	OstTraceFunctionExit1( CBULKTESTTIMER_CONSTRUCTL_EXIT, this );
	}


void CBulkTestTimer::RunL()
	{
    OstTraceFunctionEntry1( CBULKTESTTIMER_RUNL_ENTRY, this );

	iParent.HandleBulkTestTimerFired();
	OstTraceFunctionExit1( CBULKTESTTIMER_RUNL_EXIT, this );
	}



//*****************************************************************************************************


//Bulk Test Case Base Class
CBaseBulkTestCase::CBaseBulkTestCase(const TDesC& aTestCaseId,TBool aHostFlag, TBool aHostOnly)
	: CBaseTestCase(aTestCaseId, aHostFlag, aHostOnly),
	iInBufferPtr(NULL,0),
	iOutBufferPtr(NULL,0),
	iValidateBufferPtr(NULL,0)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_CBASEBULKTESTCASE_ENTRY, this );

	OstTraceFunctionExit1( CBASEBULKTESTCASE_CBASEBULKTESTCASE_EXIT, this );
	}


void CBaseBulkTestCase::BaseBulkConstructL()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_BASEBULKCONSTRUCTL_ENTRY, this );
	iTestDevice = new RUsbDeviceD(this);
	BaseConstructL();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_BASEBULKCONSTRUCTL_EXIT, this );
	}


CBaseBulkTestCase::~CBaseBulkTestCase()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_CBASEBULKTESTCASE_ENTRY_DUP01, this );

	Cancel();

	//Do this before deleting the transfer objects
	//NB this should do nothing if already called from a derived test class
	CloseInterfaceAndPipes();

	delete iValidateBuffer;
	delete iInBuffer;
	delete iOutBuffer;

	delete iBulkTestTimer;

	TUint8 count;
	for(count=0;count<KMaxNumOutTransfers;count++)
		{
		delete iOutTransfer[count];
		}
	for(count=0;count<KMaxNumInTransfers;count++)
		{
		delete iInTransfer[count];
		}

	delete iControlEp0;
	delete iActorFDF;
	if(!IsHost() && iTestDevice)
		{
		iTestDevice->Close();
		}
	delete iTestDevice;
	OstTraceFunctionExit1( CBASEBULKTESTCASE_CBASEBULKTESTCASE_EXIT_DUP01, this );
	}

void CBaseBulkTestCase::ExecuteHostTestCaseL()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_EXECUTEHOSTTESTCASEL_ENTRY, this );

	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	OstTraceFunctionExit1( CBASEBULKTESTCASE_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

void CBaseBulkTestCase::HostDoCancel()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_HOSTDOCANCEL_ENTRY, this );

	// Cancel the test step timeout timer

	CancelTimeout();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_HOSTDOCANCEL_EXIT, this );
	}


void CBaseBulkTestCase::ExecuteDeviceTestCaseL()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_EXECUTEDEVICETESTCASEL_ENTRY, this );

	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();

	// Connect the device to the host

	iTestDevice->SoftwareConnect();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_EXECUTEDEVICETESTCASEL_EXIT, this );
	}

void CBaseBulkTestCase::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_DEVICEDOCANCEL_ENTRY, this );

	// Cancel the test device error reports

	iTestDevice->CancelSubscriptionToReports();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_DEVICEDOCANCEL_EXIT, this );
	}


void CBaseBulkTestCase::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
				RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_DEVICESTATECHANGEL_ENTRY, this );
	Cancel();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_DEVICESTATECHANGEL_EXIT, this );
	}


TInt CBaseBulkTestCase::BaseBulkDeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL, "this - %08x", this);
	return BaseBulkDeviceInsertedL(aDeviceHandle, EFalse);
	}

void CBaseBulkTestCase::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_DEVICEINSERTEDL_ENTRY, this );
	//to be implemnted in individual test cases, possibly with the help of BaseBulkDeviceInsertedL
	BaseBulkDeviceInsertedL(aDeviceHandle);
	OstTraceFunctionExit1( CBASEBULKTESTCASE_DEVICEINSERTEDL_EXIT, this );
	};

TInt CBaseBulkTestCase::BaseBulkDeviceInsertedL(TUint aDeviceHandle, TBool aUseTwoInterfaces)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_ENTRY_DUP01, this );
	TInt err(KErrNone);

	// Validate connected device
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);

	OstTraceExt1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP08, "device serial number (%S)",testDevice.SerialNumber());
	OstTraceExt1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP01, "Manufacturer (%S)",testDevice.Manufacturer());
	OstTraceExt1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP02, "Product (%S)",testDevice.Product());
	OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP03, "ProductId (%d)",testDevice.ProductId());
	OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP04, "VendorId (%d)",testDevice.VendorId());

	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case

		OstTraceExt3(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP05, "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,testDevice.SerialNumber(),TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_EXIT, this, EDeviceConfigurationError );
		return EDeviceConfigurationError;
		}


	TUint32 token0;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP06, "<Error %d> Token for interface 0 could not be retrieved",err);

		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_EXIT_DUP01, this, EDeviceConfigurationError );
		return EDeviceConfigurationError;
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_DUP07, "<Error %d> Unable to open interface 0 using token %d",err,token0);
		// Start the connection timeout again
		TimeoutIn(30);
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_EXIT_DUP02, this, EDeviceConfigurationError );
		return EDeviceConfigurationError;
		}

	err = SetUpInterfaceAndPipesL(aDeviceHandle, 1);
	if(err != ENone)
		//msg already setup, and failure message sent
		{
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_EXIT_DUP03, this, EDeviceConfigurationError );
		return EDeviceConfigurationError;
		}
	OstTraceFunctionExitExt( CBASEBULKTESTCASE_BASEBULKDEVICEINSERTEDL_EXIT_DUP04, this, ENone );
	return ENone;
	}


TInt CBaseBulkTestCase::SetUpInterfaceAndPipesL(TUint aDeviceHandle, TUint8 aInterfaceNum)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_ENTRY, this );
	TInt err(KErrNone);
	TInt endpointAddress;
	RUsbInterface* pTestInterface = NULL;
	RUsbPipe* pTestPipeBulkIn = NULL;
	RUsbPipe* pTestPipeBulkOut1 = NULL;
	RUsbPipe* pTestPipeBulkOut2 = NULL;
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);

	switch(aInterfaceNum)
		{
		case 1:
			pTestInterface = &iUsbInterface1;
			pTestPipeBulkIn = &iTestPipeInterface1BulkIn;
			pTestPipeBulkOut1 = &iTestPipeInterface1BulkOut;
			pTestPipeBulkOut2 = NULL;
			break;
		case 2:
			pTestInterface = &iUsbInterface2;
			pTestPipeBulkIn = &iTestPipeInterface2BulkIn;
			pTestPipeBulkOut1 = &iTestPipeInterface2BulkOut1;
			pTestPipeBulkOut2 = &iTestPipeInterface2BulkOut2;
			break;
		default:
			User::Panic(_L("Bulk Interface Number Out Of Range"), KErrArgument);
			break;
		}

	OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL, "this - %08x", this);

	TUint32 token;
	err = testDevice.Device().GetTokenForInterface(aInterfaceNum,token);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP01, msg);
		TTestCaseFailed request(err,msg);
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT, this, EDeviceConfigurationError );
		return EDeviceConfigurationError;
		}
	if(pTestInterface != NULL)
		{
		err = pTestInterface->Open(token); // Default interface setting 1
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP02, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP01, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}
		}

	if(pTestPipeBulkIn != NULL)
		{
		err = GetEndpointAddress(*pTestInterface,0,KTransferTypeBulk,KEpDirectionIn,endpointAddress);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Address for bulk in endpoint could not be obtained"),err);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP03, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP02, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}

		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP04, "IN Endpoint address %08x",endpointAddress);

		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkIn,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP05, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP03, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}
		}

	if(pTestPipeBulkOut1 != NULL)
		{
		err = GetEndpointAddress(*pTestInterface,0,KTransferTypeBulk,KEpDirectionOut,endpointAddress);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Address for(first) bulk out endpoint could not be obtained"),err);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP06, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP04, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}

		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP07, "OUT Endpoint address %08x",endpointAddress);

		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkOut1,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP08, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP05, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}
		}

	if(pTestPipeBulkOut2 != NULL)
		{
		err = GetEndpointAddress(*pTestInterface,0,KTransferTypeBulk,KEpDirectionOut,1,endpointAddress);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Address for(second) bulk out endpoint could not be obtained"),err);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP09, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP06, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}

		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP10, "OUT Endpoint address %08x",endpointAddress);

		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkOut2,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_DUP11, msg);
			TTestCaseFailed request(err,msg);
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP07, this, EDeviceConfigurationError );
			return EDeviceConfigurationError;
			}
		}

	OstTraceFunctionExitExt( CBASEBULKTESTCASE_SETUPINTERFACEANDPIPESL_EXIT_DUP08, this, ENone );
	return ENone;
	}

void CBaseBulkTestCase::CloseInterfaceAndPipes()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_CLOSEINTERFACEANDPIPES_ENTRY, this );

	// Close the pipe(s) before interface(s)
	iTestPipeInterface2BulkIn.Close();
	iTestPipeInterface2BulkOut1.Close();
	iTestPipeInterface2BulkOut2.Close();
	iTestPipeInterface1BulkIn.Close();
	iTestPipeInterface1BulkOut.Close();

	iUsbInterface2.Close();
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	OstTraceFunctionExit1( CBASEBULKTESTCASE_CLOSEINTERFACEANDPIPES_EXIT, this );
	}

void CBaseBulkTestCase::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_DEVICEREMOVEDL_ENTRY, this );

	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error

	TestFailed(KErrDisconnected);
	OstTraceFunctionExit1( CBASEBULKTESTCASE_DEVICEREMOVEDL_EXIT, this );
	}


void CBaseBulkTestCase::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_BUSERRORL_ENTRY, this );

	// This test case handles no failiures on the bus

	TestFailed(KErrCompletion);
	OstTraceFunctionExit1( CBASEBULKTESTCASE_BUSERRORL_EXIT, this );
	}

void CBaseBulkTestCase::HostRunL()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_HOSTRUNL_ENTRY, this );
	// Obtain the completion code
	TInt completionCode(iStatus.Int());

	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CBASEBULKTESTCASE_HOSTRUNL_EXIT, this );
	}

void CBaseBulkTestCase::DeviceRunL()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_DEVICERUNL_ENTRY, this );

	// Disconnect the device
	iTestDevice->SoftwareDisconnect();

	// Complete the test case request
	TestPolicy().SignalTestComplete(iStatus.Int());
	OstTraceFunctionExit1( CBASEBULKTESTCASE_DEVICERUNL_EXIT, this );
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_VALIDATEDATA_ENTRY, this );
	return ValidateData(aDataToValidate, aDataPattern, aDataPattern.Length());
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aNumBytes)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_VALIDATEDATA_ENTRY_DUP01, this );
	return ValidateData(aDataToValidate, aDataPattern, 0, aNumBytes);
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_VALIDATEDATA_ENTRY_DUP02, this );

	__ASSERT_DEBUG(aDataPattern.Length()!=0, User::Panic(_L("Trying to validate with ZERO LENGTH STRING"), KErrArgument));

	if(aDataToValidate.Length()!=aNumBytes)
		{
		OstTraceExt2(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA, "ROUND TRIP VALIDATION: Length Match Failure, Sent = %d, Returned = %d", aNumBytes, aDataToValidate.Length());
		OstTraceFunctionExitExt( CBASEBULKTESTCASE_VALIDATEDATA_EXIT, this, EFalse );
		return EFalse;
		}
	TUint startPoint = aStartPoint%aDataPattern.Length();
	TUint numStartBytes = (aDataPattern.Length() - startPoint)%aDataPattern.Length();
	numStartBytes = aNumBytes<numStartBytes?aNumBytes:numStartBytes; //never test for more than aNumBytes
	TUint fullRepeats = (aNumBytes-numStartBytes)/aDataPattern.Length();
	TUint startEndPoint = (fullRepeats*aDataPattern.Length()) + numStartBytes;
	TUint numEndBytes = aNumBytes - startEndPoint;//fullRepeats*aDataPattern.Length() - numStartBytes;
	if(numStartBytes)
		{
		if(aDataToValidate.Left(numStartBytes).Compare(aDataPattern.Mid(startPoint, numStartBytes)) != 0)
			{
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP01, "ROUND TRIP VALIDATION: Start Bytes Match Failure");
			OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP02, "ROUND TRIP VALIDATION: numStartBytes = %d", numStartBytes);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP03, "Start of EXPECTED data ...");
			const TPtrC8& midDataPattern = aDataPattern.Mid(startPoint, numStartBytes);
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP50, "", midDataPattern.Ptr(), midDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP04, "\n");
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP05, "Start of RETURNED data ...");
			const TPtrC8& leftDataToValidate = aDataToValidate.Left(numStartBytes);
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP51, "", leftDataToValidate.Ptr(), leftDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP06, "\n");
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_VALIDATEDATA_EXIT_DUP01, this, EFalse );
			return EFalse;
			}
		}
	if(numEndBytes)
		{
		if(aDataToValidate.Mid(startEndPoint,numEndBytes).Compare(aDataPattern.Left(numEndBytes)) != 0)
			{
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP07, "ROUND TRIP VALIDATION: End Bytes Match Failure");
			OstTraceExt2(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP08, "ROUND TRIP VALIDATION: startEndPoint = %d, numEndBytes = %d", startEndPoint, numEndBytes);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP09, "End of EXPECTED data ...");
			const TPtrC8& leftDataPattern = aDataPattern.Left(numEndBytes);
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP52, "", leftDataPattern.Ptr(), leftDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP10, "\n");
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP11, "End of RETURNED data ...");
			const TPtrC8& midDataToValidate = aDataToValidate.Mid(startEndPoint,numEndBytes);
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP53, "", midDataToValidate.Ptr(), midDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP12, "\n");
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_VALIDATEDATA_EXIT_DUP02, this, EFalse );
			return EFalse;
			}
		}
	for(TInt i=0; i<fullRepeats; i++)
		{
		if(aDataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()).Compare(aDataPattern) != 0)
			{
			OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP13, "ROUND TRIP VALIDATION: Repeated Bytes Match Failure, Repeat %d",i);
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP14, "Middle block of EXPECTED data ...");
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP54, "", aDataPattern.Ptr(), aDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP15, "\n");
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP16, "Middle block of RETURNED data ...");
			const TPtrC8& midDataToValidate = aDataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length());
			OstTraceData(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP55, "", midDataToValidate.Ptr(), midDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CBASEBULKTESTCASE_VALIDATEDATA_DUP17, "\n");
			OstTraceFunctionExitExt( CBASEBULKTESTCASE_VALIDATEDATA_EXIT_DUP03, this, EFalse );
			return EFalse; //from 'for' loop
			}
		}
	OstTraceFunctionExitExt( CBASEBULKTESTCASE_VALIDATEDATA_EXIT_DUP04, this, ETrue );
	return ETrue;
	}

void CBaseBulkTestCase::RecordTime(const TUint8 aTimerIndex)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_RECORDTIME_ENTRY, this );
	if(aTimerIndex >= KMaxNumTimers)
		{
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_RECORDTIME, "Record Timer with index %d called - index OUT OF RANGE", aTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}
	iEndTime[aTimerIndex].HomeTime();
	iTimeElapsed[aTimerIndex] = iEndTime[aTimerIndex].MicroSecondsFrom(iStartTime[aTimerIndex]);
	OstTraceExt2(TRACE_NORMAL, CBASEBULKTESTCASE_RECORDTIME_DUP01, "Timer with index %d completed in %d uSec", aTimerIndex, (TInt)(iTimeElapsed[aTimerIndex].Int64()));
	OstTraceFunctionExit1( CBASEBULKTESTCASE_RECORDTIME_EXIT, this );
	}

TInt CBaseBulkTestCase::CheckTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_CHECKTIMES_ENTRY, this );
	if(aFirstTimerIndex >= KMaxNumTimers)
		{
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_CHECKTIMES, "First timer with index %d called - index OUT OF RANGE", aFirstTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}
	if(aSecondTimerIndex >= KMaxNumTimers)
		{
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_CHECKTIMES_DUP01, "Second timer with index %d called - index OUT OF RANGE", aSecondTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}

	TInt ret = KErrNone;
	OstTraceExt4(TRACE_NORMAL, CBASEBULKTESTCASE_CHECKTIMES_DUP02, "Transfer %d completed in %d uSec\nTransfer %d completed in %d uSec", aFirstTimerIndex, (TInt)(iTimeElapsed[aFirstTimerIndex].Int64()), aSecondTimerIndex, (TInt)(iTimeElapsed[aSecondTimerIndex].Int64()));
	if(aPercentage*iTimeElapsed[aFirstTimerIndex].Int64() > KPercent*iTimeElapsed[aSecondTimerIndex].Int64())
		{
		ret = KErrTooBig;
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_CHECKTIMES_DUP03, "Time %d too big", aFirstTimerIndex);
		}
	if(aPercentage*iTimeElapsed[aSecondTimerIndex].Int64() > KPercent*iTimeElapsed[aFirstTimerIndex].Int64())
		{
		ret = KErrTooBig;
		OstTrace1(TRACE_NORMAL, CBASEBULKTESTCASE_CHECKTIMES_DUP04, "Time %d too big", aSecondTimerIndex);
		}

	OstTraceFunctionExitExt( CBASEBULKTESTCASE_CHECKTIMES_EXIT, this, ret );
	return ret;
	}

void CBaseBulkTestCase::ResetTimes(const TUint8 aTimerIndex)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_RESETTIMES_ENTRY, this );
	iStartTime[aTimerIndex] = 0;
	iEndTime[aTimerIndex] = 0;
	iTimeElapsed[aTimerIndex] = 0;
	OstTraceFunctionExit1( CBASEBULKTESTCASE_RESETTIMES_EXIT, this );
	}

TInt CBaseBulkTestCase::CheckAndResetTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage)
	{
	OstTraceFunctionEntryExt( CBASEBULKTESTCASE_CHECKANDRESETTIMES_ENTRY, this );

	TInt ret = CheckTimes(aFirstTimerIndex, aSecondTimerIndex, aPercentage);
	ResetTimes(aFirstTimerIndex);
	ResetTimes(aSecondTimerIndex);

	OstTraceFunctionExitExt( CBASEBULKTESTCASE_CHECKANDRESETTIMES_EXIT, this, ret );
	return ret;
	}

void CBaseBulkTestCase::HandleBulkTestTimerFired()
	{
	OstTraceFunctionEntry1( CBASEBULKTESTCASE_HANDLEBULKTESTTIMERFIRED_ENTRY, this );
	//do nothing here - leave to derived class if required
	OstTraceFunctionExit1( CBASEBULKTESTCASE_HANDLEBULKTESTTIMERFIRED_EXIT, this );
	}

	}//end namespace

