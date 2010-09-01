// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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


 
namespace NUnitTesting_USBDI
	{

//*****************************************************************************************************

//Bulk Timer Class 
CBulkTestTimer* CBulkTestTimer::NewL(MBulkTestTimerObserver& aParent)
	{
	CBulkTestTimer* self = new (ELeave) CBulkTestTimer(aParent);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


CBulkTestTimer::CBulkTestTimer(MBulkTestTimerObserver& aParent)
:	CTimer(EPriorityStandard),
	iParent(aParent)
	{
	CActiveScheduler::Add(this);
	}


CBulkTestTimer::~CBulkTestTimer()
	{
	}


void CBulkTestTimer::ConstructL()
	{
	LOG_FUNC
	CTimer::ConstructL();
	}


void CBulkTestTimer::RunL()
	{
	LOG_FUNC

	iParent.HandleBulkTestTimerFired();
	}



//*****************************************************************************************************


//Bulk Test Case Base Class
CBaseBulkTestCase::CBaseBulkTestCase(const TDesC& aTestCaseId,TBool aHostFlag, TBool aHostOnly)
	: CBaseTestCase(aTestCaseId, aHostFlag, aHostOnly),
	iInBufferPtr(NULL,0),
	iOutBufferPtr(NULL,0),
	iValidateBufferPtr(NULL,0)	
	{
	
	}


void CBaseBulkTestCase::BaseBulkConstructL()
	{
	iTestDevice = new RUsbDeviceD(this);
	BaseConstructL();
	}


CBaseBulkTestCase::~CBaseBulkTestCase()
	{
	LOG_FUNC
	
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
	}
	
void CBaseBulkTestCase::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	TimeoutIn(30);
	}
	
void CBaseBulkTestCase::HostDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test step timeout timer
	
	CancelTimeout();
	}
	
	
void CBaseBulkTestCase::ExecuteDeviceTestCaseL()
	{
	LOG_FUNC
	
	iTestDevice->OpenL(TestCaseId());
	iTestDevice->SubscribeToReports(iStatus);
	SetActive();
	
	// Connect the device to the host
	
	iTestDevice->SoftwareConnect();
	}
	
void CBaseBulkTestCase::DeviceDoCancel()
	{
	LOG_FUNC
	
	// Cancel the test device error reports
	
	iTestDevice->CancelSubscriptionToReports();
	}
	
	
void CBaseBulkTestCase::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,
				RUsbDevice::TDeviceState aNewState,TInt aCompletionCode)
	{
	LOG_FUNC
	Cancel();
	}
	
	
TInt CBaseBulkTestCase::BaseBulkDeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	RDebug::Printf("this - %08x", this);
	return BaseBulkDeviceInsertedL(aDeviceHandle, EFalse);
	}

void CBaseBulkTestCase::DeviceInsertedL(TUint aDeviceHandle)
	{
	//to be implemnted in individual test cases, possibly with the help of BaseBulkDeviceInsertedL
	BaseBulkDeviceInsertedL(aDeviceHandle);
	};

TInt CBaseBulkTestCase::BaseBulkDeviceInsertedL(TUint aDeviceHandle, TBool aUseTwoInterfaces)
	{
	LOG_FUNC
	TInt err(KErrNone);
	
	// Validate connected device	
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	
	RDebug::Printf("device serial number (%S)",&testDevice.SerialNumber());
	RDebug::Printf("Manufacturer (%S)",&testDevice.Manufacturer());
	RDebug::Printf("Product (%S)",&testDevice.Product());
	RDebug::Printf("ProductId (%d)",testDevice.ProductId());
	RDebug::Printf("VendorId (%d)",testDevice.VendorId());
	
	if(testDevice.SerialNumber().Compare(TestCaseId()) != 0)
		{
		// Incorrect device for this test case	

		RDebug::Printf("<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
			KErrNotFound,&testDevice.SerialNumber(),&TestCaseId());

		// Start the connection timeout again
		TimeoutIn(30);
		return EDeviceConfigurationError;
		}	


	TUint32 token0;
	err = testDevice.Device().GetTokenForInterface(0,token0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Token for interface 0 could not be retrieved",err);

		// Start the connection timeout again
		TimeoutIn(30);
		return EDeviceConfigurationError;
		}
	err = iUsbInterface0.Open(token0); // Default interface setting 0
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open interface 0 using token %d",err,token0);
		// Start the connection timeout again
		TimeoutIn(30);
		return EDeviceConfigurationError;
		}

	err = SetUpInterfaceAndPipesL(aDeviceHandle, 1);
	if(err != ENone)
		//msg already setup, and failure message sent
		{
		return EDeviceConfigurationError;	
		}
	return ENone;
	}


TInt CBaseBulkTestCase::SetUpInterfaceAndPipesL(TUint aDeviceHandle, TUint8 aInterfaceNum)
	{
	LOG_FUNC
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

	RDebug::Printf("this - %08x", this);
	
	TUint32 token;
	err = testDevice.Device().GetTokenForInterface(aInterfaceNum,token);
	if(err != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Token for interface 1 could not be retrieved"),err);
		RDebug::Print(msg);
		TTestCaseFailed request(err,msg);
		return EDeviceConfigurationError;
		}
	if(pTestInterface != NULL)
		{
		err = pTestInterface->Open(token); // Default interface setting 1
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open interface 1 using token %d"),err,token);
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
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
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
			return EDeviceConfigurationError;
			}
		
		RDebug::Printf("IN Endpoint address %08x",endpointAddress);
		
		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkIn,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
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
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
			return EDeviceConfigurationError;
			}
		
		RDebug::Printf("OUT Endpoint address %08x",endpointAddress);
		
		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkOut1,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
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
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
			return EDeviceConfigurationError;
			}
		
		RDebug::Printf("OUT Endpoint address %08x",endpointAddress);
		
		err = pTestInterface->OpenPipeForEndpoint(*pTestPipeBulkOut2,endpointAddress,ETrue);
		if(err != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Unable to open pipe for endpoint %08x"),err,endpointAddress);
			RDebug::Print(msg);
			TTestCaseFailed request(err,msg);
			return EDeviceConfigurationError;
			}
		}

	return ENone;
	}
	
void CBaseBulkTestCase::CloseInterfaceAndPipes()
	{
	LOG_FUNC
	
	// Close the pipe(s) before interface(s)
	iTestPipeInterface2BulkIn.Close();
	iTestPipeInterface2BulkOut1.Close();
	iTestPipeInterface2BulkOut2.Close();
	iTestPipeInterface1BulkIn.Close();
	iTestPipeInterface1BulkOut.Close();
	
	iUsbInterface2.Close();
	iUsbInterface1.Close();
	iUsbInterface0.Close();
	}

void CBaseBulkTestCase::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// The test device should not be removed until the test case has passed
	// so this test case has not completed, and state this event as an error
	
	TestFailed(KErrDisconnected);
	}
	
	
void CBaseBulkTestCase::BusErrorL(TInt aError)
	{
	LOG_FUNC
	
	// This test case handles no failiures on the bus
	
	TestFailed(KErrCompletion);
	}

void CBaseBulkTestCase::HostRunL()
	{
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

void CBaseBulkTestCase::DeviceRunL()
	{
	LOG_FUNC
	
	// Disconnect the device
	iTestDevice->SoftwareDisconnect();
	
	// Complete the test case request
	TestPolicy().SignalTestComplete(iStatus.Int());
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern)
	{
	return ValidateData(aDataToValidate, aDataPattern, aDataPattern.Length());
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aNumBytes)
	{
	return ValidateData(aDataToValidate, aDataPattern, 0, aNumBytes);
	}

TBool CBaseBulkTestCase::ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes)
	{
	LOG_FUNC
		
	__ASSERT_DEBUG(aDataPattern.Length()!=0, User::Panic(_L("Trying to validate with ZERO LENGTH STRING"), KErrArgument));

	if(aDataToValidate.Length()!=aNumBytes)
		{
		RDebug::Printf("ROUND TRIP VALIDATION: Length Match Failure, Sent = %d, Returned = %d", aNumBytes, aDataToValidate.Length());
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
			RDebug::Printf("ROUND TRIP VALIDATION: Start Bytes Match Failure");
			RDebug::Printf("ROUND TRIP VALIDATION: numStartBytes = %d", numStartBytes);
			RDebug::Printf("Start of EXPECTED data ...");
			RDebug::RawPrint(aDataPattern.Mid(startPoint, numStartBytes));
			RDebug::Printf("\n");
			RDebug::Printf("Start of RETURNED data ...");				
			RDebug::RawPrint(aDataToValidate.Left(numStartBytes));
			RDebug::Printf("\n");
			return EFalse;
			}
		}
	if(numEndBytes)
		{
		if(aDataToValidate.Mid(startEndPoint,numEndBytes).Compare(aDataPattern.Left(numEndBytes)) != 0)
			{
			RDebug::Printf("ROUND TRIP VALIDATION: End Bytes Match Failure");
			RDebug::Printf("ROUND TRIP VALIDATION: startEndPoint = %d, numEndBytes = %d", startEndPoint, numEndBytes);
			RDebug::Printf("End of EXPECTED data ...");
			RDebug::RawPrint(aDataPattern.Left(numEndBytes));
			RDebug::Printf("\n");
			RDebug::Printf("End of RETURNED data ...");				
			RDebug::RawPrint(aDataToValidate.Mid(startEndPoint,numEndBytes));
			RDebug::Printf("\n");
			return EFalse;
			}
		}
	for(TInt i=0; i<fullRepeats; i++)
		{
		if(aDataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()).Compare(aDataPattern) != 0)
			{
			RDebug::Printf("ROUND TRIP VALIDATION: Repeated Bytes Match Failure, Repeat %d",i);
			RDebug::Printf("Middle block of EXPECTED data ...");
			RDebug::RawPrint(aDataPattern);
			RDebug::Printf("\n");
			RDebug::Printf("Middle block of RETURNED data ...");
			RDebug::RawPrint(aDataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()));
			RDebug::Printf("\n");
			return EFalse; //from 'for' loop
			}
		}
	return ETrue;
	}

void CBaseBulkTestCase::RecordTime(const TUint8 aTimerIndex)
	{
	LOG_FUNC
	if(aTimerIndex >= KMaxNumTimers)
		{
		RDebug::Printf("Record Timer with index %d called - index OUT OF RANGE", aTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}
	iEndTime[aTimerIndex].HomeTime();
	iTimeElapsed[aTimerIndex] = iEndTime[aTimerIndex].MicroSecondsFrom(iStartTime[aTimerIndex]);
	RDebug::Printf("Timer with index %d completed in %d uSec", aTimerIndex, (TInt)(iTimeElapsed[aTimerIndex].Int64()));
	}

TInt CBaseBulkTestCase::CheckTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage)
	{
	LOG_FUNC
	if(aFirstTimerIndex >= KMaxNumTimers)
		{
		RDebug::Printf("First timer with index %d called - index OUT OF RANGE", aFirstTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}
	if(aSecondTimerIndex >= KMaxNumTimers)
		{
		RDebug::Printf("Second timer with index %d called - index OUT OF RANGE", aSecondTimerIndex);
		User::Panic(_L("BAD TIMER INDEX"), KErrArgument);
		}

	TInt ret = KErrNone;
	RDebug::Printf("Transfer %d completed in %d uSec\nTransfer %d completed in %d uSec", aFirstTimerIndex, (TInt)(iTimeElapsed[aFirstTimerIndex].Int64()), aSecondTimerIndex, (TInt)(iTimeElapsed[aSecondTimerIndex].Int64()));
	if(aPercentage*iTimeElapsed[aFirstTimerIndex].Int64() > KPercent*iTimeElapsed[aSecondTimerIndex].Int64())
		{
		ret = KErrTooBig;
		RDebug::Printf("Time %d too big", aFirstTimerIndex);
		}
	if(aPercentage*iTimeElapsed[aSecondTimerIndex].Int64() > KPercent*iTimeElapsed[aFirstTimerIndex].Int64())
		{
		ret = KErrTooBig;
		RDebug::Printf("Time %d too big", aSecondTimerIndex);
		}
	
	return ret;
	}

void CBaseBulkTestCase::ResetTimes(const TUint8 aTimerIndex)
	{
	iStartTime[aTimerIndex] = 0;
	iEndTime[aTimerIndex] = 0;
	iTimeElapsed[aTimerIndex] = 0;
	}

TInt CBaseBulkTestCase::CheckAndResetTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage)
	{
	LOG_FUNC
	
	TInt ret = CheckTimes(aFirstTimerIndex, aSecondTimerIndex, aPercentage);
	ResetTimes(aFirstTimerIndex);
	ResetTimes(aSecondTimerIndex);
	
	return ret;
	}

void CBaseBulkTestCase::HandleBulkTestTimerFired()
	{
	//do nothing here - leave to derived class if required
	}

	}//end namespace

