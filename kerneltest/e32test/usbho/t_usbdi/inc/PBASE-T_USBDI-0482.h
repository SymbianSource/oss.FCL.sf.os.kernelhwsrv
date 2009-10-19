#ifndef __TEST_CASE_PBASE_T_USBDI_0482_H
#define __TEST_CASE_PBASE_T_USBDI_0482_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file PBASE-T_USBDI-0482.h
* @internalComponent
* 
*
*/



 
#include "basetestcase.h"
#include "testcasefactory.h"
#include "FDFActor.h"
#include <d32usbc.h>
#include <e32debug.h>
#include "hosttransfers.h"
#include "modelleddevices.h"

namespace NUnitTesting_USBDI
	{

/**

	
	@SYMTestCaseID				PBASE-T_USBDI-0482
	@SYMTestCaseDesc			Isochronous transfers supporting audio data rate
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7051 [USBD : Support for audio data rate]
								7056 [USBD : Isochronous transfers]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			
	@SYMTestExpectedResults 	
	@SYMTestStatus				Implemented
	

*/
class CUT_PBASE_T_USBDI_0482 : public CBaseTestCase, public MUsbBusObserver, public MTransferObserver, public MCommandObserver
	{
public:
	static CUT_PBASE_T_USBDI_0482* NewL(TBool aHostRole);
	~CUT_PBASE_T_USBDI_0482(); 

public: 
	// From MUsbBusObserver
	void DeviceInsertedL(TUint aDeviceHandle);
	void DeviceRemovedL(TUint aDeviceHandle);
	void BusErrorL(TInt aError);
	void DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState, TInt aCompletionCode);
	// From MCommandObserver
	void Ep0TransferCompleteL(TInt aCompletionCode);

public: // From MTransferObserver
	void TransferCompleteL(TInt aTransferId,TInt aCompletionCode);	
	

private:
	CUT_PBASE_T_USBDI_0482(TBool aHostRole);
	void ConstructL();
	void ExecuteHostTestCaseL();
	void ExecuteDeviceTestCaseL();
	void HostDoCancel();
	void DeviceDoCancel();
	void HostRunL();
	void DeviceRunL();
	TInt FindOUTIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor);
	TInt FindINIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor);
	TBool ReplayRecordedData();
	
private:
	
	enum TCaseStep
		{
		EInProgress,		
		EFailed,
		EWaitEndOfMusicTrack,		
		EReplayRecordedData		
		};
		
	CActorFDF* iActorFDF;
	CEp0Transfer* iControlEp0;	
	RUsbInterface iUsbInterface0;
	RUsbInterface iUsbInterface1;
	RUsbInterface iUsbInterface2;
	RUsbPipe iPipeOut;
	RUsbPipe iPipeIn;
	TInt iEndpointAddressIn;
	TInt iEndpointAddressOut;
	RBuf8 iOutTransferBuf;
	CIsochTransfer* iIsochInTransfer;
	TCaseStep iCaseStep;
	RPointerArray<CIsochTransfer> iOutTransfers;
	RBuf8 iDataPolledBuf;

	static TInt iExpectedTransferId;
									
private:
	/**
	The functor for this test case for the factory
	*/
	const static TFunctorTestCase<CUT_PBASE_T_USBDI_0482,TBool> iFunctor;
	};

	}


#endif
