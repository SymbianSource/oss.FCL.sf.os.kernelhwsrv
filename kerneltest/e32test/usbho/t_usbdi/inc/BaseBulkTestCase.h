#ifndef __BULK_BASE_TEST_CASE_H
#define __BULK_BASE_TEST_CASE_H

/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file BaseBulkTestCase.h
* @internalComponent
* 
*
*/



 
#include "BaseTestCase.h"
#include "hosttransfers.h"
#include "modelleddevices.h"
#include "controltransferrequests.h"
#include "TestCaseFactory.h"
#include "FDFActor.h"
#include <d32usbc.h>
#include <d32usbdi.h>
#include <e32debug.h>


namespace NUnitTesting_USBDI
	{
const TUint  KPercent 				= 100;
const TUint8 KMaxNumTimers 			= 4; //max number of timers used in any test
const TUint8 KMaxNumOutTransfers	= 4; //max number of queued OUT transfers used in any Bulk  test
const TUint8 KMaxNumInTransfers		= 4; //max number of queued IN transfers used in any Bulk  test

class MBulkTestTimerObserver
	{
public:
	virtual void HandleBulkTestTimerFired() = 0;
	};

class CBulkTestTimer : public CTimer
	{
public:

	/**
	2 phase construction
	@param aTestDevice the test device object that will perform the remote wakeup event
	*/
	static CBulkTestTimer* NewL(MBulkTestTimerObserver& aParent);

	/**
	Destructor
	*/
	~CBulkTestTimer();


private:
	/**
	*/
	CBulkTestTimer(MBulkTestTimerObserver& aParent);

	/**
	*/
	void ConstructL();

private: // From CTimer
	/**
	*/
	void RunL();

private:
	/**
	The test device object to instruct to connect of disonnect
	*/
	MBulkTestTimerObserver& iParent;
	};


typedef TBuf<256> TMessageBuf;

/**
Base policy class for basic bulk test cases.  Test cases are active objects that execute their respective test cases
and asynchronously complete when test case has finished
*/
class CBaseBulkTestCase : public CBaseTestCase,
						  public MUsbBusObserver,
						  public MBulkTestTimerObserver
	{
public:
	static CBaseBulkTestCase* NewL(TBool aHostRole);
	~CBaseBulkTestCase(); 

public: // From MUsbBusObserver
	virtual void DeviceInsertedL(TUint aDeviceHandle);
	virtual void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
			TInt aCompletionCode);
	void HandleBulkTestTimerFired();
	
protected:

	enum TDeviceInsertedError
		{
		ENone,
		EDeviceConfigurationError
		};
		
protected:
/**
	Constructor
	@param aTestCaseId the identity that this test case is to represent
	@param aHostFlag the flag for host role
	*/
	CBaseBulkTestCase(const TDesC& aTestCaseId,TBool aHostFlag, TBool aHostOnly = EFalse);
	
	/**
	Base class 2nd phase constructor
	*/
	void CBaseBulkTestCase::BaseBulkConstructL();

	/**
	Common implentations for most bulk transfer tests
	*/
	TInt BaseBulkDeviceInsertedL(TUint aDeviceHandle);
	TInt BaseBulkDeviceInsertedL(TUint aDeviceHandle, TBool aUseTwoInterfaces);
	TInt SetUpInterfaceAndPipesL(TUint aDeviceHandle, TUint8 aInterfaceNum);
	void CloseInterfaceAndPipes();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();
	TBool ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern);
	TBool ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aNumBytes);
	TBool ValidateData (const TDesC8& aDataToValidate, const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes);
	void RecordTime(const TUint8 aTimerIndex);
	TInt CheckTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage);
	TInt CheckAndResetTimes(const TUint8 aFirstTimerIndex, const TUint8 aSecondTimerIndex, const TUint aPercentage);
	void ResetTimes(const TUint8 aTimerIndex);

protected:
	
	CActorFDF* iActorFDF;
	RUsbPipe iTestPipeInterface1BulkOut;
	RUsbPipe iTestPipeInterface1BulkIn;
	RUsbPipe iTestPipeInterface2BulkOut1;
	RUsbPipe iTestPipeInterface2BulkOut2;
	RUsbPipe iTestPipeInterface2BulkIn;

	CEp0Transfer* iControlEp0;
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	RUsbInterface iUsbInterface2;
	
	// Transfer objects
	CBulkTransfer* iOutTransfer[KMaxNumOutTransfers];
	CBulkTransfer* iInTransfer[KMaxNumInTransfers];

	// Timer that may be used to fire mid bulk transfer EP0 requests at the client
	CBulkTestTimer* iBulkTestTimer;
	
	// The test device to be used for this test case
	RUsbDeviceD* iTestDevice;
	// Buffer for storing transfer IN data
	HBufC8* iInBuffer;
	// Ptr for 'iInBuffer'
	TPtr8 iInBufferPtr;
	// Buffer for storing transfer OUT data
	HBufC8* iOutBuffer;
	// Ptr for 'iOutBuffer'
	TPtr8 iOutBufferPtr;
	// Buffer for storing validation data
	HBufC8* iValidateBuffer;
	// Ptr for 'iValidateBuffer'
	TPtr8 iValidateBufferPtr;
	
	TInt iTransferResult;
	TMessageBuf iMsg;
	
	// TransferComplete Bit Mask
	TUint	iTransferComplete;
	
	// Set of timing facilities
	TTime iStartTime[KMaxNumTimers];
	TTime iEndTime[KMaxNumTimers];
	TTimeIntervalMicroSeconds iTimeElapsed[KMaxNumTimers];
	TInt iTimingError;
	};

	} //end namespace
#endif
