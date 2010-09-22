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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestDeviceBaseTraces.h"
#endif
#include <e32property.h>

namespace NUnitTesting_USBDI
    {

RUsbTestDevice::RUsbTestDevice()
:     iStateWatcher(NULL), 
    iCurrentState(EUsbcDeviceStateUndefined), 
    iDeviceEp0(NULL), 
    iConnectTimer(NULL), iWakeupTimer(NULL),
    iAuxBuffer(NULL)
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_RUSBTESTDEVICE_ENTRY, this );
    OstTraceFunctionExit1( RUSBTESTDEVICE_RUSBTESTDEVICE_EXIT, this );
    }
    
RUsbTestDevice::RUsbTestDevice(CBaseTestCase* aTestCase)
:     iStateWatcher(NULL), 
    iCurrentState(EUsbcDeviceStateUndefined), 
    iDeviceEp0(NULL), 
    iConnectTimer(NULL), iWakeupTimer(NULL),
    iAuxBuffer(NULL)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_RUSBTESTDEVICE_ENTRY_DUP01, this );
    iTestCase = aTestCase;
    OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_RUSBTESTDEVICE, "iTestCase = %d", iTestCase);
    OstTraceFunctionExit1( RUSBTESTDEVICE_RUSBTESTDEVICE_EXIT_DUP01, this );
    }
        
void RUsbTestDevice::ResetState()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_RESETSTATE_ENTRY, this );
    iCurrentState = EUsbcDeviceStateUndefined;
    OstTraceFunctionExit1( RUSBTESTDEVICE_RESETSTATE_EXIT, this );
    }

RUsbTestDevice::~RUsbTestDevice()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_RUSBTESTDEVICE_ENTRY_DUP02, this );
    
    OstTraceFunctionExit1( RUSBTESTDEVICE_RUSBTESTDEVICE_EXIT_DUP02, this );
    }
    
void RUsbTestDevice::Close()
    {
OstTraceFunctionEntry1( RUSBTESTDEVICE_CLOSE_ENTRY, this );

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
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_CLOSE, "Could not create the WordOfDeath P&S   (%d)", r);
            }
        OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_CLOSE_DUP01, "killing t_usbhost_usbman.exe");
        RProperty::Set(KWordOfDeathCat, KWordOfDeathKey, KErrAbort);   // Send the word of death
        User::After(1000000); //allow time for t_usbhost_usbman.exe to clean up
        }
    OstTraceFunctionExit1( RUSBTESTDEVICE_CLOSE_EXIT, this );
    }


void RUsbTestDevice::SubscribeToReports(TRequestStatus& aObserverStatus)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SUBSCRIBETOREPORTS_ENTRY, this );
    
    // Signal the request as pending
    
    iObserverStatus = &aObserverStatus;
    *iObserverStatus = KRequestPending;
    OstTraceFunctionExit1( RUSBTESTDEVICE_SUBSCRIBETOREPORTS_EXIT, this );
    }


void RUsbTestDevice::CancelSubscriptionToReports()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_CANCELSUBSCRIPTIONTOREPORTS_ENTRY, this );
    
    // Signal the request as cancelled
    User::RequestComplete(iObserverStatus,KErrCancel);
    OstTraceFunctionExit1( RUSBTESTDEVICE_CANCELSUBSCRIPTIONTOREPORTS_EXIT, this );
    }
    
        
void RUsbTestDevice::OpenL()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_OPENL_ENTRY, this );
    TInt err = KErrNone;
    
    OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_OPENL, "starting t_usbhost_usbman.exe");
    TInt r = iOtgUsbMan.Create(_L("t_usbhost_usbman.exe"), _L("client"));
    gtest(r == KErrNone);
    iOtgUsbMan.Resume();    
    
    User::After(1500000);
    
    // Open channel to driver
    err = iClientDriver.Open(0);
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP01, "<Error %d> Unable to open a channel to USB client driver",err);
        User::Leave(err);
        }
    
    // Hide bus from host while interfaces are being set up
    err = iClientDriver.DeviceDisconnectFromHost();
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP02, "<Error %d> unable to disconnect device from host",err);
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

    User::LeaveIfError(iClientDriver.DeviceCaps(iDeviceCaps));

#ifdef OST_TRACE_COMPILER_IN_USE
    _LIT8(KYes, "yes");
    _LIT8(KNo, "no");
#endif
    
     OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP03, "------ USB device capabilities -------");
    OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP04, "Number of endpoints:                %d",iDeviceCaps().iTotalEndpoints);    
    OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP05, "Supports Software-Connect:          %s",iDeviceCaps().iConnect ? KYes() : KNo());
    OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP06, "Device is Self-Powered:             %S",iDeviceCaps().iSelfPowered ? KYes() : KNo());
    OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP07, "Supports Remote-Wakeup:             %S",iDeviceCaps().iRemoteWakeup ? KYes() : KNo());
    OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP08, "Supports High-speed:                %S",iDeviceCaps().iHighSpeed ? KYes() : KNo());
    OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP09, "Supports unpowered cable detection: %S",(iDeviceCaps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ? KYes() : KNo());
    OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_OPENL_DUP10, "--------------------------------------");
    
    OstTraceFunctionExit1( RUSBTESTDEVICE_OPENL_EXIT, this );
    }


TInt RUsbTestDevice::SetClassCode(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SETCLASSCODE_ENTRY, this );

    // Get Device descriptor
    TBuf8<KUsbDescSize_Device> deviceDescriptor;
    TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETCLASSCODE, "<Error %d> Unable to obtain device descriptor",err);
        }
    else
        {
        deviceDescriptor[KDevDescOffset_bDeviceClass] = aClassCode;
        deviceDescriptor[KDevDescOffset_bDeviceSubClass] = aSubClassCode;
        deviceDescriptor[KDevDescOffset_bDeviceProtocol] = aDeviceProtocol;    
        
        err = iClientDriver.SetDeviceDescriptor(deviceDescriptor);
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETCLASSCODE_DUP01, "<Error %d> Unable to set the device dsecriptor",err);
            }
        }
    OstTraceFunctionExitExt( RUSBTESTDEVICE_SETCLASSCODE_EXIT, this, err );
    return err;
    }


TInt RUsbTestDevice::SetUsbSpecification(TUint16 aSpecification)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SETUSBSPECIFICATION_ENTRY, this );

    // Get Device descriptor
    TBuf8<KUsbDescSize_Device> deviceDescriptor;
    TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETUSBSPECIFICATION, "<Error %d> Unable to obtain device descriptor",err);
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
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETUSBSPECIFICATION_DUP01, "<Error %d> Unable to set the device dsecriptor",err);
            }
        }
    OstTraceFunctionExitExt( RUSBTESTDEVICE_SETUSBSPECIFICATION_EXIT, this, err );
    return err;
    }


TInt RUsbTestDevice::SetVendor(TUint16 aVendorId)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SETVENDOR_ENTRY, this );

    // Get Device descriptor
    TBuf8<KUsbDescSize_Device> deviceDescriptor;
    TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETVENDOR, "<Error %d> Unable to obtain device descriptor",err);
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
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETVENDOR_DUP01, "<Error %d> Unable to set the device descriptor",err);
            }
        }
    OstTraceFunctionExitExt( RUSBTESTDEVICE_SETVENDOR_EXIT, this, err );
    return err;
    }


TInt RUsbTestDevice::SetProduct(TUint16 aProductId,const TDesC16& aProductString,
                const TDesC16& aManufacturerString,const TDesC16& aSerialNumberString)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SETPRODUCT_ENTRY, this );

    // Get Device descriptor
    TBuf8<KUsbDescSize_Device> deviceDescriptor;
    TInt err(iClientDriver.GetDeviceDescriptor(deviceDescriptor));
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT, "<Error %d> Unable to obtain device descriptor",err);
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
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT_DUP01, "<Error %d> Unable to set the device dsecriptor",err);
            OstTraceFunctionExitExt( RUSBTESTDEVICE_SETPRODUCT_EXIT, this, err );
            return err;
            }
        
        OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT_DUP02, "Product Identity set");
    
        // Product string
        err = iClientDriver.SetProductStringDescriptor(aProductString);
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT_DUP03, "<Error %d> Unable to set product string descriptor",err);
            OstTraceFunctionExitExt( RUSBTESTDEVICE_SETPRODUCT_EXIT_DUP01, this, err );
            return err;
            }

        // Manufacturer string
        err = iClientDriver.SetManufacturerStringDescriptor(aManufacturerString);
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT_DUP04, "<Error %d> Unable to set the manufacturer string descriptor",err);
            OstTraceFunctionExitExt( RUSBTESTDEVICE_SETPRODUCT_EXIT_DUP02, this, err );
            return err;
            }
    
        // Serial number string
        err = iClientDriver.SetSerialNumberStringDescriptor(aSerialNumberString);
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETPRODUCT_DUP05, "<Error %d> Unable to set the serial number string descriptor",err);
            OstTraceFunctionExitExt( RUSBTESTDEVICE_SETPRODUCT_EXIT_DUP03, this, err );
            return err;
            }
        }
    OstTraceFunctionExitExt( RUSBTESTDEVICE_SETPRODUCT_EXIT_DUP04, this, KErrNone );
    return KErrNone;
    }


TInt RUsbTestDevice::SetConfigurationString(const TDesC16& aConfigString)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_SETCONFIGURATIONSTRING_ENTRY, this );

    TInt err(iClientDriver.SetConfigurationStringDescriptor(aConfigString));
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SETCONFIGURATIONSTRING, "<Error %d> Unable to set configuration string descriptor",err);
        }
    OstTraceFunctionExitExt( RUSBTESTDEVICE_SETCONFIGURATIONSTRING_EXIT, this, err );
    return err;
    }


void RUsbTestDevice::AddInterface(CInterfaceBase* aInterface)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_ADDINTERFACE_ENTRY, this );
        
    // Add the interface to the device
    TInt err = iInterfaces.Append(aInterface);
    
    if(err != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_ADDINTERFACE, "<Error %d> Unable to add interface",err);
        return ReportError(err);
        }
    OstTraceFunctionExit1( RUSBTESTDEVICE_ADDINTERFACE_EXIT, this );
    }


CInterfaceBase& RUsbTestDevice::Interface(TInt aIndex)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_INTERFACE_ENTRY, this );
    OstTraceFunctionExit1( RUSBTESTDEVICE_INTERFACE_EXIT, this );
    return *iInterfaces[aIndex];
    }


void RUsbTestDevice::SoftwareConnect()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_SOFTWARECONNECT_ENTRY, this );
    TInt err(iClientDriver.PowerUpUdc());
    if((err != KErrNone) && (err != KErrNotReady))
        {
        OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SOFTWARECONNECT, "<Error %d> Power Up Udc",err);
        ReportError(err);
        }
        
    if(iDeviceCaps().iConnect) 
        {
        err = iClientDriver.DeviceConnectToHost();
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SOFTWARECONNECT_DUP01, "<Error %d> Unable to connect to the host",err);
            ReportError(err);
            }
        }
    else
        {
        OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_SOFTWARECONNECT_DUP02, "Please connect device to Host");
        }    
    OstTraceFunctionExit1( RUSBTESTDEVICE_SOFTWARECONNECT_EXIT, this );
    }       
     
void RUsbTestDevice::SoftwareDisconnect()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_SOFTWAREDISCONNECT_ENTRY, this );
    
    if(iDeviceCaps().iConnect) 
        {
        TInt err(iClientDriver.DeviceDisconnectFromHost());
        if(err != KErrNone)
            {
            OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_SOFTWAREDISCONNECT, "<Error %d> Unable to disconnect from the host",err);
            ReportError(err);
            }
        }
    else
        {
        OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_SOFTWAREDISCONNECT_DUP01, "Please disconnect device from Host");
        }
        
    OstTraceFunctionExit1( RUSBTESTDEVICE_SOFTWAREDISCONNECT_EXIT, this );
    }       


void RUsbTestDevice::RemoteWakeup()
	{
	OstTraceFunctionEntry1( RUSBTESTDEVICE_REMOTEWAKEUP_ENTRY, this );
	if(iDeviceCaps().iConnect) 
		{
		TInt err(iClientDriver.SignalRemoteWakeup());
		if(err != KErrNone)
			{
			OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_REMOTEWAKEUP, "<Error %d> Unable to perform a remote wakeup",err);
			ReportError(err);
			}
		}
	else
		{
		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_REMOTEWAKEUP_DUP01, "remote wakeup not supported");
		}
	OstTraceFunctionExit1( RUSBTESTDEVICE_REMOTEWAKEUP_EXIT, this );
	}


TInt RUsbTestDevice::ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,
	TUint16 aDataReqLength,const TDesC8& aPayload)
	{
	OstTraceFunctionEntryExt( RUSBTESTDEVICE_PROCESSREQUESTL_ENTRY, this );
	
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
		OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL, "**aRequest == KVendorDisconnectDeviceAThenConnectDeviceCRequest, this = 0x%08x", this);
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
		OstTraceFunctionExitExt( RUSBTESTDEVICE_PROCESSREQUESTL_EXIT, this, KErrAbort );
		return KErrAbort;
		}
	else if(aRequest == KVendorDisconnectDeviceCThenConnectDeviceARequest)
		{
		OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP01, "**aRequest == KVendorDisconnectDeviceCThenConnectDeviceARequest, this = 0x%08x", this);
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
		OstTraceFunctionExitExt( RUSBTESTDEVICE_PROCESSREQUESTL_EXIT_DUP01, this, KErrAbort );
		return KErrAbort;
		}		
	else if(aRequest == KVendorTestCasePassed)
		{ 
		// Test case has completed successfully 
		// so report to client test case that host is happy		
		
		AcknowledgeRequestReceived();
		
		// allow time for host side t_usbdi.exe so that CloseInterface/Pipe happen after CloseDevice
		User::After(1000*5); 
		
		ReportError(KErrNone);
		}
	else if(aRequest == KVendorTestCaseFailed)
		{
		// The test case has failed, so report to client test case
		// and display/log the error message
		
		AcknowledgeRequestReceived();
		
		// allow time for host side t_usbdi.exe so that CloseInterface/Pipe happen after CloseDevice
		User::After(1000*5); 
		
		HBufC16* msg = HBufC16::NewL(aPayload.Length());
		msg->Des().Copy(aPayload);
		OstTraceExt1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP02, "<Host> Test case failed: %S",*msg);
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
		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP03, "Put payload");
		if(aPayload.Compare(_L8("DEADBEEF")) != 0)
			{
			OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP04, "<Error %d> Payload not as expected",KErrCorrupt);
			ReportError(KErrCorrupt);
			}
		}
	else if(aRequest == KVendorGetPayloadRequest)
		{
		// Handle a payload request to the host

		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP05, "Get payload");
		__ASSERT_DEBUG(iAuxBuffer, User::Panic(_L("Trying to write non-allocated buffer"), KErrGeneral));
		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP06, "iAuxBuffer = ....");
        OstTraceData(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP56, "", iAuxBuffer->Ptr(), iAuxBuffer->Length());
		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP07, "\n");
		
		//Perform synchronous write to EP0
		//This allows the subsequent 'Read' request to
		//take place
		TInt ret = iDeviceEp0->SendDataSynchronous(*iAuxBuffer);
		OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP08, "Write (from device callback) executed with error %d", ret);
		}
	else if(aRequest == KVendorUnrespondRequest)
		{
		// Do not acknowledge this request
		
		OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP09, "Unrespond request: continually NAK the host");
		}
	else if(aRequest == KVendorStallRequest)
		{
		// Stall the specified endpoint
		
		AcknowledgeRequestReceived();
		OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_PROCESSREQUESTL_DUP10, "Stalling endpoint %d",aValue);
						
		}
	else
		{
		// Maybe forward to derived classes
		}
	OstTraceFunctionExitExt( RUSBTESTDEVICE_PROCESSREQUESTL_EXIT_DUP02, this, KErrNone );
	return KErrNone;
	}


void RUsbTestDevice::StateChangeL(TUsbcDeviceState aNewState,TInt aChangeCompletionCode)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_STATECHANGEL_ENTRY, this );
    
    OstTraceExt2(TRACE_NORMAL, RUSBTESTDEVICE_STATECHANGEL, "Client state change to %d err=%d",aNewState,aChangeCompletionCode);
    
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
            
            OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_STATECHANGEL_DUP01, "Ignoring control ep0");
            
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
            
            OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_STATECHANGEL_DUP02, "Reading from control ep0");
            
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
    OstTraceFunctionExit1( RUSBTESTDEVICE_STATECHANGEL_EXIT, this );
    }
    

void RUsbTestDevice::StartEp0Reading()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_STARTEP0READING_ENTRY, this );

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
    OstTraceFunctionExit1( RUSBTESTDEVICE_STARTEP0READING_EXIT, this );
    }


void RUsbTestDevice::StopEp0Reading()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_STOPEP0READING_ENTRY, this );

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
    OstTraceFunctionExit1( RUSBTESTDEVICE_STOPEP0READING_EXIT, this );
    }


void RUsbTestDevice::AcknowledgeRequestReceived()
    {
    OstTraceFunctionEntry1( RUSBTESTDEVICE_ACKNOWLEDGEREQUESTRECEIVED_ENTRY, this );
    
    TInt err(iDeviceEp0->Reader().Acknowledge());
    OstTrace1(TRACE_NORMAL, RUSBTESTDEVICE_ACKNOWLEDGEREQUESTRECEIVED, "err = %d",err);
    if(err != KErrNone)
        {
        ReportError(err);
        }
    OstTraceFunctionExit1( RUSBTESTDEVICE_ACKNOWLEDGEREQUESTRECEIVED_EXIT, this );
    }



void RUsbTestDevice::ReportError(TInt aCompletionCode)
    {
    OstTraceFunctionEntryExt( RUSBTESTDEVICE_REPORTERROR_ENTRY, this );
    OstTraceExt3(TRACE_NORMAL, RUSBTESTDEVICE_REPORTERROR, "err or aCompletionCode = %d, observer status = %d, KRequestPending = %d",
            aCompletionCode, iObserverStatus->Int(), KRequestPending);
    if(*iObserverStatus == KRequestPending)
        {
        OstTrace0(TRACE_NORMAL, RUSBTESTDEVICE_REPORTERROR_DUP01, "In complete request");
        User::RequestComplete(iObserverStatus,aCompletionCode);
        }
    OstTraceFunctionExit1( RUSBTESTDEVICE_REPORTERROR_EXIT, this );
    }
    

    }
    
