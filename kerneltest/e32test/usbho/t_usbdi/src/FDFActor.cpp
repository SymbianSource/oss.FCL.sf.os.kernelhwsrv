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
// @file fdfactor.cpp
// @internalComponent
// 
//

#include "FDFActor.h"
#include "testdebug.h"
#include <Usb.h>
#include "UsbDescriptorOffsets.h"
#include "BaseTestCase.h"
#include <e32property.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{
static const TUid KWordOfDeathCat = {0x01066600};
static const TInt KWordOfDeathKey = 0x01066601;

static const TInt KNonExistantStringNumber = 97; //the device does not have a string descriptor associated with the number 97

CActorFDF* CActorFDF::NewL(MUsbBusObserver& aObserver)
	{
	CActorFDF* self = new (ELeave) CActorFDF(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


CActorFDF::CActorFDF(MUsbBusObserver& aObserver)
:	CActive(EPriorityStandard),
	iObserver(aObserver)
	{
	}


void CActorFDF::ConstructL()
	{
	LOG_FUNC
	CActiveScheduler::Add(this);
	
	TInt err(iDriver.Open());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open driver channel",err);
		User::Leave(err);
		}		
	
	RDebug::Printf("PBASE-T_USBDI-xxxx: Stack starting");
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
		RDebug::Printf("<Error %d> USB Host stack not starting",err);		
		gtest(EFalse);
		}
	  
	RDebug::Printf("starting t_usbhost_usbman.exe");
	TInt r = iOtgUsbMan.Create(_L("t_usbhost_usbman.exe"), KNullDesC); //LIT does not seem to work here
	if(r != KErrNone && r != KErrAlreadyExists)
		{
  		RDebug::Printf("can't start t_usbhost_usbman.exe");
		gtest(EFalse);
		}
	
    // create a publish/subscribe key to allow usbhost_usbman to be killed
    // cleanly
    static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
    r = RProperty::Define(KWordOfDeathCat, KWordOfDeathKey, RProperty::EInt,KAllowAllPolicy, KAllowAllPolicy, 0);
	if(r != KErrNone && r != KErrAlreadyExists)
        {
        RDebug::Printf("Could not create the WordOfDeath P&S   (%d)", r);
 		gtest(EFalse);
        }

	iOtgUsbMan.Resume();	
	
	User::After(1500000); //allow time for t_usbhost_usbman.exe to start	
	}

void KillTest()
	{
	RDebug::Printf("BEFORE gtest(EFalse)");
	gtest(EFalse);
	RDebug::Printf("AFTER gtest(EFalse)");
	}

CActorFDF::~CActorFDF()
	{
	LOG_FUNC
	Cancel();
	
	// Destroy all test device objects that represented connected devices
	
	RHashMap<TUint,CUsbTestDevice*>::TIter it(iDevices);
	TInt count(0);
	for(count=0; count<iDevices.Count(); count++)
		{
		delete *it.NextValue();
		}

	RDebug::Printf("killing t_usbhost_usbman.exe");
	TInt r = RProperty::Set(KWordOfDeathCat, KWordOfDeathKey, KErrAbort);   // Send the word of death
    if(r != KErrNone)
        {
        RDebug::Printf("failed to kill t_usbhost-usbhan  (%d)", r);      
		RProperty::Delete(KWordOfDeathCat, KWordOfDeathKey); //try to clean this up ready for next test
		User::After(1000000); //allow time for property to clean up
		gtest(EFalse);
       }
	User::After(1000000); //allow time for t_usbhost_usbman.exe to clean up

	// Stop the USB Hub driver
		
	RDebug::Printf("UT-USBD-P1782-TN0001: Hub driver stopping...");

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
	        RDebug::Printf("failed to delete wordofdeath P&S  (%d)", r);
			gtest(EFalse);
			}
        }

	}
	
void CActorFDF::DoCancel()
	{
	LOG_FUNC


	RDebug::Printf("Cancelling bus event notifications");
	iDriver.CancelWaitForBusEvent();
	gtest((iStatus == KErrCancel) || (iStatus == KErrNone));	
	RDebug::Printf("Bus event notifications successfully cancelled");	
	}
	
void CActorFDF::Monitor()
	{
	LOG_FUNC

	RDebug::Printf("Monitoring bus events");
	iDriver.WaitForBusEvent(iBusEvent,iStatus);
	SetActive();
	}
	
	
			
CUsbTestDevice& CActorFDF::DeviceL(TUint aDeviceHandle)
	{
	return *iDevices.FindL(aDeviceHandle);
	}
	
void CActorFDF::RunL()
	{
	LOG_FUNC

	// Obtain completion code
	TInt completionCode(iStatus.Int());
	RDebug::Printf("Completion code  : %d",completionCode);
	
	 if(completionCode == KErrNone)
		{
		if(iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceAttached)
			{
			// Device Attached
			RDebug::Printf("Usb device attached: %d",iBusEvent.iDeviceHandle);
			
			// Create the test device object
			iDevices.InsertL(iBusEvent.iDeviceHandle,CUsbTestDevice::NewL(iDriver,iBusEvent.iDeviceHandle,iObserver));
			
			// Notify observer
			iObserver.DeviceInsertedL(iBusEvent.iDeviceHandle);
			}
		else if(iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceRemoved)
			{
			// Device Removed
			RDebug::Printf("Usb device removed: %d",iBusEvent.iDeviceHandle);
			
			// Notify observer
			iObserver.DeviceRemovedL(iBusEvent.iDeviceHandle);
			
			// Destroy the device for the handle and remove from the map
			delete iDevices.FindL(iBusEvent.iDeviceHandle);
			iDevices.Remove(iBusEvent.iDeviceHandle);
			}
		else
			{
			// TODO: Upcall for USB Man etc
			RDebug::Printf("<Warning> Bus event %d occured, still monitoring, reason = %d",iBusEvent.iEventType, iBusEvent.iReason);
			iDriver.WaitForBusEvent(iBusEvent,iStatus);
			SetActive();
			}
		}
	else
		{
		RDebug::Printf("<Error %d> Bus event %d",completionCode,iBusEvent.iEventType);
		iObserver.BusErrorL(completionCode);
		}
	}
	

TInt CActorFDF::RunError(TInt aError)
	{
	LOG_FUNC

	RDebug::Printf("<Error %d> CActorFDF::RunError",aError);
	return KErrNone;
	}
	
	
	
	
	
	
	
	
	
	
	
CUsbTestDevice* CUsbTestDevice::NewL(RUsbHubDriver& aHubDriver,TUint aDeviceHandle,MUsbBusObserver& aObserver)
	{
	CUsbTestDevice* self = new (ELeave) CUsbTestDevice(aHubDriver,aDeviceHandle,aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CUsbTestDevice::CUsbTestDevice(RUsbHubDriver& aHubDriver,TUint aHandle,MUsbBusObserver& aObserver)
:	CActive(EPriorityUserInput),
	iDriver(aHubDriver),
	iHandle(aHandle),
	iObserver(aObserver)
	{
	LOG_FUNC

	CActiveScheduler::Add(this);
	}
	
CUsbTestDevice::~CUsbTestDevice()
	{
	LOG_FUNC
	Cancel();
	iDevice.Close();
	}
	
void CUsbTestDevice::ConstructL()
	{
	LOG_FUNC

	// Open the usb device object
	User::LeaveIfError(iDevice.Open(iDriver,iHandle));

	TInt err(iDevice.GetDeviceDescriptor(iDeviceDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) descriptor",err,iHandle);
		User::Leave(err);
		}
	
	err = iDevice.GetConfigurationDescriptor(iConfigDescriptor);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) configuration descriptor",err,iHandle);
		User::Leave(err);
		}

	iDeviceSpec = iDeviceDescriptor.USBBcd();
	iPid = iDeviceDescriptor.ProductId();
	iVid = iDeviceDescriptor.VendorId();
	
	RDebug::Printf("%dmA configuration maximum power consumption",iConfigDescriptor.MaxPower()*2);
	RDebug::Printf("%d number of interface(s)",iConfigDescriptor.NumInterfaces());
	RDebug::Printf("Product Id=0x%04x, Vendor Id=0x%04x",iPid,iVid);
	RDebug::Printf("TotalLength() = %d",iConfigDescriptor.TotalLength());

	// The manufacturer string
	err = iDevice.GetStringDescriptor(iManufacturerStringDesc,iManufacturerStringData,iDeviceDescriptor.ManufacturerIndex());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) manufacturer string descriptor",err,iHandle);
		User::Leave(err);
		}
	iManufacturerStringDesc->StringData(iManufacturerString);
	
	// The product string
	err = iDevice.GetStringDescriptor(iProductStringDesc,iProductStringData,iDeviceDescriptor.ProductIndex());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) product string descriptor",err,iHandle);
		User::Leave(err);
		}
	iProductStringDesc->StringData(iProductString);
		
	// The serial number 
	err = iDevice.GetStringDescriptor(iSerialNumberDesc,iSerialNumberStringData,iDeviceDescriptor.SerialNumberIndex());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) serial number string descriptor",err,iHandle);
		User::Leave(err);
		}
	iSerialNumberDesc->StringData(iSerialNumber);
	
	// The configuration string
	err = iDevice.GetStringDescriptor(iConfigStringDesc,iConfigStringData,iConfigDescriptor.ConfigurationIndex());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Getting device (%d) configuration string descriptor",err,iHandle);
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
		RDebug::Printf("GetStringDescriptor with nonExistentString didn't return KErrUsbStalled",err,iHandle);
		User::Leave(err);
		}
	RDebug::Printf("String not present error(%d)",err);

	// Get changes in device state
	iDevice.QueueDeviceStateChangeNotification(iCurrentState,iStatus); // iCurrentState now holds the current device state
	SetActive();
	}
	
RUsbDevice& CUsbTestDevice::Device()
	{
	return iDevice;
	}
	
TUint16 CUsbTestDevice::DeviceSpec() const
	{
	return iDeviceSpec;
	}
	
TUint16 CUsbTestDevice::ProductId() const
	{
	return iPid;
	}
	
TUint16 CUsbTestDevice::VendorId() const
	{
	return iVid;
	}

const TDesC16& CUsbTestDevice::SerialNumber() const
	{
	return iSerialNumber;
	}

const TDesC16& CUsbTestDevice::Manufacturer() const
	{
	return iManufacturerString;
	}
	
const TDesC16& CUsbTestDevice::Product() const
	{
	return iProductString;
	}
	
const TDesC16& CUsbTestDevice::ConfigurationString() const
	{
	return iConfigString;
	}

const TUsbConfigurationDescriptor& CUsbTestDevice::ConfigurationDescriptor() const
	{
	return iConfigDescriptor;
	}
		
const TUsbDeviceDescriptor& CUsbTestDevice::DeviceDescriptor() const
	{
	return iDeviceDescriptor;
	}

void CUsbTestDevice::DoCancel()
	{
	LOG_FUNC

	iDevice.CancelDeviceStateChangeNotification();
	}


void CUsbTestDevice::RunL()
	{
	LOG_FUNC

	TInt completionCode(iStatus.Int());
	RDebug::Printf("CUsbTestDevice::RunL completionCode(%d)",completionCode);

	if(completionCode == KErrNone)
		{
		RUsbDevice::TDeviceState newState;
		iDevice.QueueDeviceStateChangeNotification(newState,iStatus);
		SetActive();
		iObserver.DeviceStateChangeL(iCurrentState,newState,completionCode);
		iCurrentState = newState;
		}
	}


TInt CUsbTestDevice::RunError(TInt aError)
	{
	LOG_FUNC

	RDebug::Printf("<Error %d>",aError);
	return KErrNone;
	}

	}

