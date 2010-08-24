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
// @file fdfactor.cpp
// @internalComponent
// 
//

#include "FDFActor.h"
#include "testdebug.h"
#include <Usb.h>
#include "UsbDescriptorOffsets.h"
#include "BaseTestCase.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "FDFActorTraces.h"
#endif
#include <e32property.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{
static const TUid KWordOfDeathCat = {0x01066600};
static const TInt KWordOfDeathKey = 0x01066601;

static const TInt KNonExistantStringNumber = 97; //the device does not have a string descriptor associated with the number 97

CActorFDF* CActorFDF::NewL(MUsbBusObserver& aObserver)
	{
	OstTraceFunctionEntry1( CACTORFDF_NEWL_ENTRY, ( TUint )&( aObserver ) );
	CActorFDF* self = new (ELeave) CActorFDF(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CACTORFDF_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}


CActorFDF::CActorFDF(MUsbBusObserver& aObserver)
:	CActive(EPriorityStandard),
	iObserver(aObserver)
	{
	OstTraceFunctionEntryExt( CACTORFDF_CACTORFDF_ENTRY, this );
	OstTraceFunctionExit1( CACTORFDF_CACTORFDF_EXIT, this );
	}


void CActorFDF::ConstructL()
	{
	OstTraceFunctionEntry1( CACTORFDF_CONSTRUCTL_ENTRY, this );
	CActiveScheduler::Add(this);
	
	TInt err(iDriver.Open());
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CACTORFDF_CONSTRUCTL, "<Error %d> Unable to open driver channel",err);
		User::Leave(err);
		}		
	
	OstTrace0(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP01, "PBASE-T_USBDI-xxxx: Stack starting");
	/*
	@SYMTestCaseID				PBASE-T_USBDI-xxxx
	@SYMTestCaseDesc			Test for host stack initiation
	@SYMFssID 
	@SYMPREQ					1782
	@SYMREQ						7097 [USBMAN : Activation and deactivation of USB Host functionality]
	@SYMTestType				UT
	@SYMTestPriority			1 
	@SYMTestActions 			
	@SYMTestExpectedResults	 	KErrNone from RUsbHubDriver::StartHost()
	@SYMTestStatus				Implemented
	*/
	err = iDriver.StartHost();
	if(err != KErrNone)
		{
		// Test case did not run successfully
		OstTrace1(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP02, "<Error %d> USB Host stack not starting",err);		
		gtest(EFalse);
		}
	  
	OstTrace0(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP03, "starting t_usbhost_usbman.exe");
	TInt r = iOtgUsbMan.Create(_L("t_usbhost_usbman.exe"), KNullDesC); //LIT does not seem to work here
	if(r != KErrNone && r != KErrAlreadyExists)
		{
  		OstTrace0(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP04, "can't start t_usbhost_usbman.exe");
		gtest(EFalse);
		}
	
    // create a publish/subscribe key to allow usbhost_usbman to be killed
    // cleanly
    static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
    r = RProperty::Define(KWordOfDeathCat, KWordOfDeathKey, RProperty::EInt,KAllowAllPolicy, KAllowAllPolicy, 0);
	if(r != KErrNone && r != KErrAlreadyExists)
        {
        OstTrace1(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP05, "Could not create the WordOfDeath P&S   (%d)", r);
 		gtest(EFalse);
        }

	iOtgUsbMan.Resume();	
	
	User::After(1500000); //allow time for t_usbhost_usbman.exe to start	
	OstTraceFunctionExit1( CACTORFDF_CONSTRUCTL_EXIT, this );
	}

void KillTest()
	{
	OstTraceFunctionEntry0( _KILLTEST_ENTRY );
	OstTrace0(TRACE_NORMAL, KILLTEST_KILLTEST, "BEFORE gtest(EFalse)");
	gtest(EFalse);
	OstTrace0(TRACE_NORMAL, KILLTEST_KILLTEST_DUP01, "AFTER gtest(EFalse)");
	OstTraceFunctionExit0( _KILLTEST_EXIT );
	}

CActorFDF::~CActorFDF()
	{
	OstTraceFunctionEntry1( CACTORFDF_CACTORFDF_ENTRY_DUP01, this );
	Cancel();
	
	// Destroy all test device objects that represented connected devices
	
	RHashMap<TUint,CUsbTestDevice*>::TIter it(iDevices);
	TInt count(0);
	for(count=0; count<iDevices.Count(); count++)
		{
		delete *it.NextValue();
		}

	OstTrace0(TRACE_NORMAL, CACTORFDF_DCACTORFDF, "killing t_usbhost_usbman.exe");
	TInt r = RProperty::Set(KWordOfDeathCat, KWordOfDeathKey, KErrAbort);   // Send the word of death
    if(r != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, CACTORFDF_DCACTORFDF_DUP01, "failed to kill t_usbhost-usbhan  (%d)", r);      
		RProperty::Delete(KWordOfDeathCat, KWordOfDeathKey); //try to clean this up ready for next test
		User::After(1000000); //allow time for property to clean up
		gtest(EFalse);
       }
	User::After(1000000); //allow time for t_usbhost_usbman.exe to clean up

	// Stop the USB Hub driver
		
	OstTrace0(TRACE_NORMAL, CACTORFDF_DCACTORFDF_DUP02, "UT-USBD-P1782-TN0001: Hub driver stopping...");

	// Close the channel to the USB Hub driver	
	iDriver.Close();	
	
	User::After(500000);
	
    // delete word of death P&S
    r = RProperty::Delete(KWordOfDeathCat, KWordOfDeathKey);
    if(r != KErrNone)
        {
        //try again
		User::After(1000000); //allow time for property to clean up
		r = RProperty::Delete(KWordOfDeathCat, KWordOfDeathKey);
		if(r != KErrNone)
			//give up
			{
			User::After(1000000); //allow time for property to clean up JUST IN CASE it can despite returning an error!
	        OstTrace1(TRACE_NORMAL, CACTORFDF_DCACTORFDF_DUP03, "failed to delete wordofdeath P&S  (%d)", r);
			gtest(EFalse);
			}
        }

	OstTraceFunctionExit1( CACTORFDF_CACTORFDF_EXIT_DUP01, this );
	}
	
void CActorFDF::DoCancel()
	{
OstTraceFunctionEntry1( CACTORFDF_DOCANCEL_ENTRY, this );


	OstTrace0(TRACE_NORMAL, CACTORFDF_DOCANCEL, "Cancelling bus event notifications");
	iDriver.CancelWaitForBusEvent();
	gtest((iStatus == KErrCancel) || (iStatus == KErrNone));	
	OstTrace0(TRACE_NORMAL, CACTORFDF_DOCANCEL_DUP01, "Bus event notifications successfully cancelled");
	OstTraceFunctionExit1( CACTORFDF_DOCANCEL_EXIT, this );
	}
	
void CActorFDF::Monitor()
	{
OstTraceFunctionEntry1( CACTORFDF_MONITOR_ENTRY, this );

	OstTrace0(TRACE_NORMAL, CACTORFDF_MONITOR, "Monitoring bus events");
	iDriver.WaitForBusEvent(iBusEvent,iStatus);
	SetActive();
	OstTraceFunctionExit1( CACTORFDF_MONITOR_EXIT, this );
	}
	
	
			
CUsbTestDevice& CActorFDF::DeviceL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CACTORFDF_DEVICEL_ENTRY, this );
	return *iDevices.FindL(aDeviceHandle);
	}
	
void CActorFDF::RunL()
	{
OstTraceFunctionEntry1( CACTORFDF_RUNL_ENTRY, this );

	// Obtain completion code
	TInt completionCode(iStatus.Int());
	OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL, "Completion code  : %d",completionCode);
	
	 if(completionCode == KErrNone)
		{
		if(iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceAttached)
			{
			// Device Attached
			OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP01, "Usb device attached: %d",iBusEvent.iDeviceHandle);
			
			// Create the test device object
			iDevices.InsertL(iBusEvent.iDeviceHandle,CUsbTestDevice::NewL(iDriver,iBusEvent.iDeviceHandle,iObserver));
			
			// Notify observer
			iObserver.DeviceInsertedL(iBusEvent.iDeviceHandle);
			}
		else if(iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceRemoved)
			{
			// Device Removed
			OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP02, "Usb device removed: %d",iBusEvent.iDeviceHandle);
			
			// Notify observer
			iObserver.DeviceRemovedL(iBusEvent.iDeviceHandle);
			
			// Destroy the device for the handle and remove from the map
			delete iDevices.FindL(iBusEvent.iDeviceHandle);
			iDevices.Remove(iBusEvent.iDeviceHandle);
			}
		else
			{
			// TODO: Upcall for USB Man etc
			OstTraceExt2(TRACE_NORMAL, CACTORFDF_RUNL_DUP03, "<Warning> Bus event %d occured, still monitoring, reason = %d",iBusEvent.iEventType, iBusEvent.iReason);
			iDriver.WaitForBusEvent(iBusEvent,iStatus);
			SetActive();
			}
		}
	else
		{
		OstTraceExt2(TRACE_NORMAL, CACTORFDF_RUNL_DUP04, "<Error %d> Bus event %d",completionCode,iBusEvent.iEventType);
		iObserver.BusErrorL(completionCode);
		}
	OstTraceFunctionExit1( CACTORFDF_RUNL_EXIT, this );
	}
	

TInt CActorFDF::RunError(TInt aError)
	{
OstTraceFunctionEntryExt( CACTORFDF_RUNERROR_ENTRY, this );

	OstTrace1(TRACE_NORMAL, CACTORFDF_RUNERROR, "<Error %d> CActorFDF::RunError",aError);
	OstTraceFunctionExitExt( CACTORFDF_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}
	
	
	
	
	
	
	
	
	
	
	
CUsbTestDevice* CUsbTestDevice::NewL(RUsbHubDriver& aHubDriver,TUint aDeviceHandle,MUsbBusObserver& aObserver)
	{
	OstTraceFunctionEntryExt( CUSBTESTDEVICE_NEWL_ENTRY, 0 );
	CUsbTestDevice* self = new (ELeave) CUsbTestDevice(aHubDriver,aDeviceHandle,aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUSBTESTDEVICE_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
CUsbTestDevice::CUsbTestDevice(RUsbHubDriver& aHubDriver,TUint aHandle,MUsbBusObserver& aObserver)
:	CActive(EPriorityUserInput),
	iDriver(aHubDriver),
	iHandle(aHandle),
	iObserver(aObserver)
	{
OstTraceFunctionEntryExt( CUSBTESTDEVICE_CUSBTESTDEVICE_ENTRY, this );

	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CUSBTESTDEVICE_CUSBTESTDEVICE_EXIT, this );
	}
	
CUsbTestDevice::~CUsbTestDevice()
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_CUSBTESTDEVICE_ENTRY_DUP01, this );
	Cancel();
	iDevice.Close();
	OstTraceFunctionExit1( CUSBTESTDEVICE_CUSBTESTDEVICE_EXIT_DUP01, this );
	}
	
void CUsbTestDevice::ConstructL()
	{
OstTraceFunctionEntry1( CUSBTESTDEVICE_CONSTRUCTL_ENTRY, this );

	// Open the usb device object
	User::LeaveIfError(iDevice.Open(iDriver,iHandle));

	TInt err(iDevice.GetDeviceDescriptor(iDeviceDescriptor));
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL, "<Error %d> Getting device (%u) descriptor",err,iHandle);
		User::Leave(err);
		}
	
	err = iDevice.GetConfigurationDescriptor(iConfigDescriptor);
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP01, "<Error %d> Getting device (%u) configuration descriptor",err,iHandle);
		User::Leave(err);
		}

	iDeviceSpec = iDeviceDescriptor.USBBcd();
	iPid = iDeviceDescriptor.ProductId();
	iVid = iDeviceDescriptor.VendorId();
	
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP02, "%dmA configuration maximum power consumption",iConfigDescriptor.MaxPower()*2);
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP03, "%d number of interface(s)",iConfigDescriptor.NumInterfaces());
	OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP04, "Product Id=0x%04x, Vendor Id=0x%04x",(TUint32)iPid,(TUint32)iVid);
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP05, "TotalLength() = %d",iConfigDescriptor.TotalLength());

	// The manufacturer string
	err = iDevice.GetStringDescriptor(iManufacturerStringDesc,iManufacturerStringData,iDeviceDescriptor.ManufacturerIndex());
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP06, "<Error %d> Getting device (%u) manufacturer string descriptor",err,iHandle);
		User::Leave(err);
		}
	iManufacturerStringDesc->StringData(iManufacturerString);
	
	// The product string
	err = iDevice.GetStringDescriptor(iProductStringDesc,iProductStringData,iDeviceDescriptor.ProductIndex());
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP07, "<Error %d> Getting device (%u) product string descriptor",err,iHandle);
		User::Leave(err);
		}
	iProductStringDesc->StringData(iProductString);
		
	// The serial number 
	err = iDevice.GetStringDescriptor(iSerialNumberDesc,iSerialNumberStringData,iDeviceDescriptor.SerialNumberIndex());
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP08, "<Error %d> Getting device (%u) serial number string descriptor",err,iHandle);
		User::Leave(err);
		}
	iSerialNumberDesc->StringData(iSerialNumber);
	
	// The configuration string
	err = iDevice.GetStringDescriptor(iConfigStringDesc,iConfigStringData,iConfigDescriptor.ConfigurationIndex());
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP09, "<Error %d> Getting device (%u) configuration string descriptor",err,iHandle);
		User::Leave(err);
		}
	iConfigStringDesc->StringData(iConfigString);
	
	// Trying to obtain a string descriptor that is not present, expecting a stall	
	TBuf8<255> nonExistentString;
	TUsbStringDescriptor* unusedStringDescriptor =  NULL;
	err = iDevice.GetStringDescriptor(unusedStringDescriptor,nonExistentString,KNonExistantStringNumber);
	if(err != KErrUsbStalled)
		{
		delete unusedStringDescriptor; //in case 'err ==  KErrNone', in which case this will need freeing
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP10, "GetStringDescriptor with %d Handle %u",err,iHandle);
		User::Leave(err);
		}
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP11, "String not present error(%d)",err);

	// Get changes in device state
	iDevice.QueueDeviceStateChangeNotification(iCurrentState,iStatus); // iCurrentState now holds the current device state
	SetActive();
	OstTraceFunctionExit1( CUSBTESTDEVICE_CONSTRUCTL_EXIT, this );
	}
	
RUsbDevice& CUsbTestDevice::Device()
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_DEVICE_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_DEVICE_EXIT, this, ( TUint )&( iDevice ) );
	return iDevice;
	}
	
TUint16 CUsbTestDevice::DeviceSpec() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_DEVICESPEC_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_DEVICESPEC_EXIT, this, ( TUint16 )( iDeviceSpec ) );
	return iDeviceSpec;
	}
	
TUint16 CUsbTestDevice::ProductId() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_PRODUCTID_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_PRODUCTID_EXIT, this, ( TUint16 )( iPid ) );
	return iPid;
	}
	
TUint16 CUsbTestDevice::VendorId() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_VENDORID_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_VENDORID_EXIT, this, ( TUint16 )( iVid ) );
	return iVid;
	}

const TDesC16& CUsbTestDevice::SerialNumber() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_SERIALNUMBER_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_SERIALNUMBER_EXIT, this, ( TUint )&( iSerialNumber ) );
	return iSerialNumber;
	}

const TDesC16& CUsbTestDevice::Manufacturer() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_MANUFACTURER_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_MANUFACTURER_EXIT, this, ( TUint )&( iManufacturerString ) );
	return iManufacturerString;
	}
	
const TDesC16& CUsbTestDevice::Product() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_PRODUCT_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_PRODUCT_EXIT, this, ( TUint )&( iProductString ) );
	return iProductString;
	}
	
const TDesC16& CUsbTestDevice::ConfigurationString() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_CONFIGURATIONSTRING_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_CONFIGURATIONSTRING_EXIT, this, ( TUint )&( iConfigString ) );
	return iConfigString;
	}

const TUsbConfigurationDescriptor& CUsbTestDevice::ConfigurationDescriptor() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_CONFIGURATIONDESCRIPTOR_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_CONFIGURATIONDESCRIPTOR_EXIT, this, ( TUint )&( iConfigDescriptor ) );
	return iConfigDescriptor;
	}
		
const TUsbDeviceDescriptor& CUsbTestDevice::DeviceDescriptor() const
	{
	OstTraceFunctionEntry1( CUSBTESTDEVICE_DEVICEDESCRIPTOR_ENTRY, this );
	OstTraceFunctionExitExt( CUSBTESTDEVICE_DEVICEDESCRIPTOR_EXIT, this, ( TUint )&( iDeviceDescriptor ) );
	return iDeviceDescriptor;
	}

void CUsbTestDevice::DoCancel()
	{
    OstTraceFunctionEntry1( CUSBTESTDEVICE_DOCANCEL_ENTRY, this );

	iDevice.CancelDeviceStateChangeNotification();
	OstTraceFunctionExit1( CUSBTESTDEVICE_DOCANCEL_EXIT, this );
	}


void CUsbTestDevice::RunL()
	{
    OstTraceFunctionEntry1( CUSBTESTDEVICE_RUNL_ENTRY, this );

	TInt completionCode(iStatus.Int());
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_RUNL, "CUsbTestDevice::RunL completionCode(%d)",completionCode);

	if(completionCode == KErrNone)
		{
		RUsbDevice::TDeviceState newState;
		iDevice.QueueDeviceStateChangeNotification(newState,iStatus);
		SetActive();
		iObserver.DeviceStateChangeL(iCurrentState,newState,completionCode);
		iCurrentState = newState;
		}
	OstTraceFunctionExit1( CUSBTESTDEVICE_RUNL_EXIT, this );
	}


TInt CUsbTestDevice::RunError(TInt aError)
	{
    OstTraceFunctionEntryExt( CUSBTESTDEVICE_RUNERROR_ENTRY, this );

	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_RUNERROR, "<Error %d>",aError);
	OstTraceFunctionExitExt( CUSBTESTDEVICE_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}

	}

