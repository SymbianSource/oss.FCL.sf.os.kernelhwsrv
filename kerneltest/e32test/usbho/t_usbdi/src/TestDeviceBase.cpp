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
// @file testdevicebase.cpp
// @internalComponent
// 
//

#include "TestDeviceBase.h"
#include "UsbDescriptorOffsets.h"
#include "testdebug.h"
#include <e32test.h>
#include "softwareconnecttimer.h"
#include "wakeuptimer.h"
#include "controltransferrequests.h"
#include "testinterfacebase.h"
#include "PBASE-T_USBDI-0486.h"
#include <e32property.h>

namespace NUnitTesting_USBDI
	{

RUsbTestDevice::RUsbTestDevice()
: 	iStateWatcher(NULL), 
	iCurrentState(EUsbcDeviceStateUndefined), 
	iDeviceEp0(NULL), 
	iConnectTimer(NULL), iWakeupTimer(NULL),
	iAuxBuffer(NULL)
	{
	}
	
RUsbTestDevice::RUsbTestDevice(CBaseTestCase* aTestCase)
: 	iStateWatcher(NULL), 
	iCurrentState(EUsbcDeviceStateUndefined), 
	iDeviceEp0(NULL), 
	iConnectTimer(NULL), iWakeupTimer(NULL),
	iAuxBuffer(NULL)
	{
	LOG_FUNC	
	iTestCase = aTestCase;
	RDebug::Printf("iTestCase = %d", iTestCase);
	}
		
void RUsbTestDevice::ResetState()
	{
	iCurrentState = EUsbcDeviceStateUndefined;
	}

RUsbTestDevice::~RUsbTestDevice()
	{
	LOG_FUNC
	
	}
	
void RUsbTestDevice::Close()
	{
	LOG_FUNC

	delete iWakeupTimer;
	delete iConnectTimer;
	delete iDeviceEp0;
	delete iStateWatcher;
		
	iInterfaces.ResetAndDestroy();

	iClientDriver.Close(); 
	
	if(!iTestCase->IsHost()) // process only started in client rom
		{
        // create a publish/subscribe key to allow usbhost_usbman to be killed
        // cleanly
        static const TUid KWordOfDeathCat = {0x01066600};
        static const TInt KWordOfDeathKey = 0x01066601;
        static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
        TInt r = RProperty::Define(KWordOfDeathCat, KWordOfDeathKey, RProperty::EInt,KAllowAllPolicy, KAllowAllPolicy, 0);
        if(r != KErrNone)
            {
            RDebug::Print(_L("Could not create the WordOfDeath P&S   (%d)"), r);
            }
		RDebug::Printf("killing t_usbhost_usbman.exe");
        RProperty::Set(KWordOfDeathCat, KWordOfDeathKey, KErrAbort);   // Send the word of death
        User::After(1000000); //allow time for t_usbhost_usbman.exe to clean up
		}
	}


void RUsbTestDevice::SubscribeToReports(TRequestStatus& aObserverStatus)
	{
	LOG_FUNC
	
	// Signal the request as pending
	
	iObserverStatus = &aObserverStatus;
	*iObserverStatus = KRequestPending;
	}


void RUsbTestDevice::CancelSubscriptionToReports()
	{
	LOG_FUNC
	
	// Signal the request as cancelled
	User::RequestComplete(iObserverStatus,KErrCancel);
	}
	
		
void RUsbTestDevice::OpenL()
	{
	LOG_FUNC	
	TInt err = KErrNone;
	
	RDebug::Printf("starting t_usbhost_usbman.exe");
	TInt r = iOtgUsbMan.Create(_L("t_usbhost_usbman.exe"), _L("client"));
	gtest(r == KErrNone);
	iOtgUsbMan.Resume();	
	
	User::After(1500000);
	
	// Open channel to driver
	err = iClientDriver.Open(0);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open a channel to USB client driver",err);
		User::Leave(err);
		}
	
	// Hide bus from host while interfaces are being set up
	err = iClientDriver.DeviceDisconnectFromHost();
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> unable to disconnect device from host",err);
		User::Leave(err);
		}
	
	// Create the client usb state watcher
	iStateWatcher = CUsbClientStateWatcher::NewL(iClientDriver,*this);
		
	// Create the Ep0 reader
	iDeviceEp0 = CDeviceEndpoint0::NewL(*this);
		
	// Create the timer for software connection/disconnection
	iConnectTimer = CSoftwareConnectTimer::NewL(*this);
	
	// Create the timer for remote wakeup events
	iWakeupTimer = CRemoteWakeupTimer::NewL(*this);
	_LIT8(KYes, "yes");
	_LIT8(KNo, "no");
	User::LeaveIfError(iClientDriver.DeviceCaps(iDeviceCaps));
 	RDebug::Printf("------ USB device capabilities -------");
	RDebug::Printf("Number of endpoints:                %d",iDeviceCaps().iTotalEndpoints);	
	RDebug::Printf("Supports Software-Connect:          %S",iDeviceCaps().iConnect ? &KYes() : &KNo());
	RDebug::Printf("Device is Self-Powered:             %S",iDeviceCaps().iSelfPowered ? &KYes() : &KNo());
	RDebug::Printf("Supports Remote-Wakeup:             %S",iDeviceCaps().iRemoteWakeup ? &KYes() : &KNo());
	RDebug::Printf("Supports High-speed:                %S",iDeviceCaps().iHighSpeed ? &KYes() : &KNo());
	RDebug::Printf("Supports unpowered cable detection: %S",(iDeviceCaps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ? &KYes() : &KNo());
	RDebug::Printf("--------------------------------------");
	
	}


TInt RUsbTestDevice::SetClassCode(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol)
	{
	LOG_FUNC

	// Get Device descriptor
	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to obtain device descriptor",err);
		}
	else
		{
		deviceDescriptor[KDevDescOffset_bDeviceClass] = aClassCode;
		deviceDescriptor[KDevDescOffset_bDeviceSubClass] = aSubClassCode;
		deviceDescriptor[KDevDescOffset_bDeviceProtocol] = aDeviceProtocol;	
		
		err = iClientDriver.SetDeviceDescriptor(deviceDescriptor);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the device dsecriptor",err);
			}
		}
	return err;
	}


TInt RUsbTestDevice::SetUsbSpecification(TUint16 aSpecification)
	{
	LOG_FUNC

	// Get Device descriptor
	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to obtain device descriptor",err);
		}
	else
		{
		// Set bcdUSB
		TUint8* p = reinterpret_cast<TUint8*>(&aSpecification);
		deviceDescriptor[KDevDescOffset_bcdUSB] = *p;
		deviceDescriptor[KDevDescOffset_bcdUSB+1] = *(p+1);
		
		// Symbian currently supports only devices with one configuration by selecting the configurations
		// that has the lowest power consumption
		
		deviceDescriptor[KDevDescOffset_bNumConfigurations] = 0x01;
		
		err = iClientDriver.SetDeviceDescriptor(deviceDescriptor);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the device dsecriptor",err);
			}
		}
	return err;
	}


TInt RUsbTestDevice::SetVendor(TUint16 aVendorId)
	{
	LOG_FUNC

	// Get Device descriptor
	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to obtain device descriptor",err);
		}
	else
		{
		// Set VID
		TUint8* p = reinterpret_cast<TUint8*>(&aVendorId);
		deviceDescriptor[KDevDescOffset_idVendor] = *p;
		deviceDescriptor[KDevDescOffset_idVendor+1] = *(p+1);
		
		err = iClientDriver.SetDeviceDescriptor(deviceDescriptor);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the device descriptor",err);
			}
		}
	return err;
	}


TInt RUsbTestDevice::SetProduct(TUint16 aProductId,const TDesC16& aProductString,
				const TDesC16& aManufacturerString,const TDesC16& aSerialNumberString)
	{
	LOG_FUNC

	// Get Device descriptor
	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to obtain device descriptor",err);
		}
	else
		{
		// Set PID
		TUint8* p = reinterpret_cast<TUint8*>(&aProductId);
		deviceDescriptor[KDevDescOffset_idProduct] = *p;
		deviceDescriptor[KDevDescOffset_idProduct+1] = *(p+1);
		
		err = iClientDriver.SetDeviceDescriptor(deviceDescriptor);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the device dsecriptor",err);
			return err;
			}
		
		RDebug::Printf("Product Identity set");
	
		// Product string
		err = iClientDriver.SetProductStringDescriptor(aProductString);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set product string descriptor",err);
			return err;
			}

		// Manufacturer string
		err = iClientDriver.SetManufacturerStringDescriptor(aManufacturerString);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the manufacturer string descriptor",err);
			return err;
			}
	
		// Serial number string
		err = iClientDriver.SetSerialNumberStringDescriptor(aSerialNumberString);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to set the serial number string descriptor",err);
			return err;
			}
		}
	return KErrNone;
	}


TInt RUsbTestDevice::SetConfigurationString(const TDesC16& aConfigString)
	{
	LOG_FUNC

	TInt err(iClientDriver.SetConfigurationStringDescriptor(aConfigString));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to set configuration string descriptor",err);
		}
	return err;
	}


void RUsbTestDevice::AddInterface(CInterfaceBase* aInterface)
	{
	LOG_FUNC
		
	// Add the interface to the device
	TInt err = iInterfaces.Append(aInterface);
	
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to add interface",err);
		return ReportError(err);
		}
	}


CInterfaceBase& RUsbTestDevice::Interface(TInt aIndex)
	{
	return *iInterfaces[aIndex];
	}


void RUsbTestDevice::SoftwareConnect()
	{
	LOG_FUNC
	TInt err(iClientDriver.PowerUpUdc());
	if((err != KErrNone) && (err != KErrNotReady))
		{
		RDebug::Printf("<Error %d> Power Up Udc",err);
		ReportError(err);
		}
		
	if(iDeviceCaps().iConnect) 
		{
		err = iClientDriver.DeviceConnectToHost();
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to connect to the host",err);
			ReportError(err);
			}
		}
	else
		{
		RDebug::Printf("Please connect device to Host");
		}	
	}   	
	 
void RUsbTestDevice::SoftwareDisconnect()
	{
	LOG_FUNC
	
	if(iDeviceCaps().iConnect) 
		{
		TInt err(iClientDriver.DeviceDisconnectFromHost());
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to disconnect from the host",err);
			ReportError(err);
			}
		}
	else
		{
		RDebug::Printf("Please disconnect device from Host");
		}
		
	}       


void RUsbTestDevice::RemoteWakeup()
	{
	LOG_FUNC
	if(iDeviceCaps().iConnect) 
		{
		TInt err(iClientDriver.SignalRemoteWakeup());
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to perform a remote wakeup",err);
			ReportError(err);
			}
		}
	else
		{
		RDebug::Printf("remote wakeup not supported");
		}
	}


TInt RUsbTestDevice::ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,
	TUint16 aDataReqLength,const TDesC8& aPayload)
	{
	LOG_FUNC
	
	if(aRequest == KVendorEmptyRequest)
		{
		// Handle an empty request (i.e. do nothing)
		
		AcknowledgeRequestReceived();
		}
	else if(aRequest == KVendorReconnectRequest)
		{
		// Handle a reconnect requests from the host
		
		AcknowledgeRequestReceived();
		iConnectTimer->SoftwareReConnect(aValue);
		}
	else if(aRequest == KVendorDisconnectDeviceAThenConnectDeviceCRequest)
		{
		RDebug::Printf("**aRequest == KVendorDisconnectDeviceAThenConnectDeviceCRequest, this = 0x%08x", this);
		// Handle a reconnect requests from the host		
		AcknowledgeRequestReceived();			
		
		SoftwareDisconnect();	
		User::After(1000000);
		//	connect device C now	
		CUT_PBASE_T_USBDI_0486* iTestCaseT_USBDI_0486 = reinterpret_cast<CUT_PBASE_T_USBDI_0486*>(iTestCase);
		Close(); 		
		
		User::After(3000000);
		iTestCaseT_USBDI_0486->TestDeviceC()->OpenL(KTestDeviceC_SN);		
		// Connect the device to the host	
		iTestCaseT_USBDI_0486->TestDeviceC()->SoftwareConnect();
		return KErrAbort;
		}
	else if(aRequest == KVendorDisconnectDeviceCThenConnectDeviceARequest)
		{
		RDebug::Printf("**aRequest == KVendorDisconnectDeviceCThenConnectDeviceARequest, this = 0x%08x", this);
		// Handle a reconnect requests from the host		
		AcknowledgeRequestReceived();		
		 
		SoftwareDisconnect();	
		User::After(1000000); 
		 
		//	connect device A now	
		CUT_PBASE_T_USBDI_0486* iTestCaseT_USBDI_0486 = reinterpret_cast<CUT_PBASE_T_USBDI_0486*>(iTestCase);
		
		
		Close();		
		User::After(3000000); 		
		iTestCaseT_USBDI_0486->Cancel();
		iTestCaseT_USBDI_0486->HandleDeviceDConnection();
		iTestCaseT_USBDI_0486->TestDeviceD()->OpenL(iTestCaseT_USBDI_0486->TestCaseId());	
		
		// Connect the device to the host	
		iTestCaseT_USBDI_0486->TestDeviceD()->SoftwareConnect();	
		return KErrAbort;
		}		
	else if(aRequest == KVendorTestCasePassed)
		{ 
		// Test case has completed successfully 
		// so report to client test case that host is happy		
		
		AcknowledgeRequestReceived();
		ReportError(KErrNone);
		}
	else if(aRequest == KVendorTestCaseFailed)
		{
		// The test case has failed, so report to client test case
		// and display/log the error message
		
		AcknowledgeRequestReceived();
		
		HBufC16* msg = HBufC16::NewL(aPayload.Length());
		msg->Des().Copy(aPayload);
		RDebug::Printf("<Host> Test case failed: %S",msg);
		delete msg;
		msg = 0;
		ReportError(-aValue);
		}
	else if(aRequest == KVendorRemoteWakeupRequest)
		{
		// Handle a remote wakeup request

		AcknowledgeRequestReceived();
		iWakeupTimer->WakeUp(aValue);
		}
	else if(aRequest == KVendorPutPayloadRequest)
		{
		// Handle a payload request from the host

		AcknowledgeRequestReceived();
		RDebug::Printf("Put payload");
		if(aPayload.Compare(_L8("DEADBEEF")) != 0)
			{
			RDebug::Printf("<Error %d> Payload not as expected",KErrCorrupt);
			ReportError(KErrCorrupt);
			}
		}
	else if(aRequest == KVendorGetPayloadRequest)
		{
		// Handle a payload request to the host

		RDebug::Printf("Get payload");
		__ASSERT_DEBUG(iAuxBuffer, User::Panic(_L("Trying to write non-allocated buffer"), KErrGeneral));
		RDebug::Printf("iAuxBuffer = ....");
		RDebug::RawPrint(*iAuxBuffer);
		RDebug::Printf("\n");
		
		//Perform synchronous write to EP0
		//This allows the subsequent 'Read' request to
		//take place
		TInt ret = iDeviceEp0->SendDataSynchronous(*iAuxBuffer);
		RDebug::Printf("Write (from device callback) executed with error %d", ret);
		}
	else if(aRequest == KVendorUnrespondRequest)
		{
		// Do not acknowledge this request
		
		RDebug::Printf("Unrespond request: continually NAK the host");
		}
	else if(aRequest == KVendorStallRequest)
		{
		// Stall the specified endpoint
		
		AcknowledgeRequestReceived();
		RDebug::Printf("Stalling endpoint %d",aValue);
						
		}
	else
		{
		// Maybe forward to derived classes
		}
	return KErrNone;
	}


void RUsbTestDevice::StateChangeL(TUsbcDeviceState aNewState,TInt aChangeCompletionCode)
	{
	LOG_FUNC
	
	RDebug::Printf("Client state change to %d err=%d",aNewState,aChangeCompletionCode);
	
	// Notify the test case of failed state change notification
	
	if(aChangeCompletionCode != KErrNone)
		{
		return ReportError(aChangeCompletionCode);
		}
	
	// Check the state change
	
	if(iCurrentState == EUsbcDeviceStateConfigured)
		{
		// Device is already in a fully configured state
		
		if(aNewState == EUsbcDeviceStateConfigured)
			{
			// Do nothing as this is the current state anyway
			}
		else
			{
			// The is a state change from EUsbcDeviceStateConfigured to aNewState
			// so stop reading from control ep0
			
			RDebug::Printf("Ignoring control ep0");
			
			// Stop reading ep0 directed requests
			
			StopEp0Reading();
			
			// Update state
						
			iCurrentState = aNewState;
			}
		}
	else
		{
		// Device is not in a fully configured state
		
		if(aNewState == EUsbcDeviceStateConfigured)
			{
			// Device has now been placed into a fully configured state by the host
			// so start reading from control ep0
			
			RDebug::Printf("Reading from control ep0");
			
			// Start reading ep0 directed requests
			
			StartEp0Reading();
			
			// Update state
						
			iCurrentState = aNewState;
			}
		else
			{
			// Just update the state
			
			iCurrentState = aNewState;
			}
		}
				
	// Forward the state change notification to derived classes
	
	OnStateChangeL(aNewState);
	}
	

void RUsbTestDevice::StartEp0Reading()
	{
	LOG_FUNC

	// Start reading device directed ep0 requests
	
	TInt err(iDeviceEp0->Start());
	if(err != KErrNone)
		{
		return ReportError(err);
		}
		
	// Start reading interface directed requests
			
	TInt interfaceCount(iInterfaces.Count());
	for(TInt i=0; i<interfaceCount; i++)
		{
		iInterfaces[i]->StartEp0Reading();
		}
	}


void RUsbTestDevice::StopEp0Reading()
	{
	LOG_FUNC

	// Stop reading interface directed requests
	
	TInt interfaceCount(iInterfaces.Count());
	for(TInt i=0; i<interfaceCount; i++)
		{
		iInterfaces[i]->StopEp0Reading();
		}
		
	// Stop reading device directed requests from ep0
			
	TInt err(iDeviceEp0->Stop());
	if(err != KErrNone)
		{
		return ReportError(err);
		}
	}


void RUsbTestDevice::AcknowledgeRequestReceived()
	{
	LOG_FUNC
	
	TInt err(iDeviceEp0->Reader().Acknowledge());
	RDebug::Printf("err = %d",err);
	if(err != KErrNone)
		{
		ReportError(err);
		}
	}



void RUsbTestDevice::ReportError(TInt aCompletionCode)
	{
	LOG_FUNC
	RDebug::Printf("err or aCompletionCode = %d, observer status = %d, KRequestPending = %d",
			aCompletionCode, iObserverStatus->Int(), KRequestPending);
	if(*iObserverStatus == KRequestPending)
		{
		RDebug::Printf("In complete request");
		User::RequestComplete(iObserverStatus,aCompletionCode);
		}
	}
	

	}
	
