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
//

/**
 @file
 @internalComponent
 
 The driver's name
 
 @return The name of the driver
 
 @internalComponent
*/
inline const TDesC& RUsbHubDriver::Name()
	{
	_LIT(KDriverName,"USBHUBDRIVER");
	return KDriverName;
	}

/**
  The driver's version

  @return The version number of the driver
*/
inline TVersion RUsbHubDriver::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}


#ifndef __KERNEL_MODE__

/**
Open a handle to the host controller.
@return System-wide error code giving status of connection attempt.
*/
TInt RUsbHubDriver::Open()
	{
	TInt rc = KErrNone;
	
	// Check to see if this object has already been opened - if it has,
	// there will be a handle set.
	
	if ( Handle() )
		{
		User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverAlreadyOpened);
		}
	
	rc = DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);
	
	if ( rc != KErrNone )
		{
		RDebug::Print(_L("********************************"));
		RDebug::Print(_L("* RUsbHubDriver::Open() Fault! *"));
		RDebug::Print(_L("********************************"));
		}
		
	return rc;
	}


/**
Start the host stack.
*/
TInt RUsbHubDriver::StartHost()
	{
	return DoControl(EStartHost);
	}


/**
Stop the host stack.
*/
void RUsbHubDriver::StopHost()
	{
	DoControl(EStopHost);
	}


/**
Wait for a bus event.  These include device attachments and detachments.
@see TBusEvent
@param aEvent The details of the event that occured, filled in when the request completes.
@param aStatus Completed when an event occurs
*/
void RUsbHubDriver::WaitForBusEvent(TBusEvent& aEvent, TRequestStatus& aStatus)
	{
	DoRequest(EWaitForBusEvent, aStatus, &aEvent);
	}


/**
Cancel a request to wait for bus events.
*/
void RUsbHubDriver::CancelWaitForBusEvent()
	{
	DoCancel(ECancelWaitForBusEvent);
	}
	

RUsbDevice::RUsbDevice()
	: iHeadDeviceDescriptor(NULL)
	, iHeadConfDescriptor(NULL)
	, iConfigurationDescriptorData(NULL)
	, iHub(NULL)
	, iHandle(0)
	{
	}

/**
Open a handle to a device.
*/
TInt RUsbDevice::Open(RUsbHubDriver& aHub, TUint aHandle)
	{
	if(iHandle)
		{
		return KErrInUse;
		}

	TInt err;
	err = aHub.DoControl(EOpen, (TAny*)aHandle);


	if (err == KErrNone)
		{
		iHub = &aHub;
		iHandle = aHandle;
		}
	else
		{
		return err;
		}

	TRAP(err, GetLocalDescriptorsL());
	// GetLocalDescriptorsL should roll back iHandle etc on error.
	__ASSERT_DEBUG(err == KErrNone || !iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverNoRollBackAfterFailedDeviceOpen));

	return err;
	}

void RUsbDevice::GetLocalDescriptorsL()
	{
	CleanupClosePushL(*this); // Ensure that we roll back to closed on error.

	// Get Device Descriptor Data.
	User::LeaveIfError(GetDeviceDescriptor(iDeviceDescriptorData));

	// Get Configuration Descriptor Data
	TInt configSize = 0;
	User::LeaveIfError(GetConfigurationDescriptorSize(configSize));
	
	iConfigurationDescriptorData = HBufC8::NewL(configSize);
	TPtr8 ptr = iConfigurationDescriptorData->Des();
	User::LeaveIfError(GetConfigurationDescriptor(ptr));


	TUsbGenericDescriptor* parsed = NULL;

	// Parse Device Descriptor
	User::LeaveIfError(UsbDescriptorParser::Parse(iDeviceDescriptorData, parsed));
	iHeadDeviceDescriptor = TUsbDeviceDescriptor::Cast(parsed);
	if(!iHeadDeviceDescriptor)
		{
		User::Leave(KErrCorrupt);
		}

	// Parse Configuration Descriptor
	User::LeaveIfError(UsbDescriptorParser::Parse(*iConfigurationDescriptorData, parsed));
	iHeadConfDescriptor = TUsbConfigurationDescriptor::Cast(parsed);
	if(!iHeadConfDescriptor)
		{
		User::Leave(KErrCorrupt);
		}

	CleanupStack::Pop(); // this
	}


/**
Close a handle to a device.
*/
void RUsbDevice::Close()
	{
	if(iHub)
		{
		iHub->DoControl(EClose, (TAny*)iHandle);
		}

	if(iHeadConfDescriptor)
		{
		iHeadConfDescriptor->DestroyTree();
		delete iHeadConfDescriptor;
		iHeadConfDescriptor = NULL;
		}

	if(iHeadDeviceDescriptor)
		{
		iHeadDeviceDescriptor->DestroyTree();
		delete iHeadDeviceDescriptor;
		iHeadDeviceDescriptor = NULL;
		}

	delete iConfigurationDescriptorData;
	iConfigurationDescriptorData = NULL;

	iHub = NULL;
	iHandle = 0;
	}
	

/**
Return the handle to a device
*/
TUint RUsbDevice::Handle() const
	{
	return iHandle;
	}


/**
Places the device into a suspended state.
*/
TInt RUsbDevice::Suspend()
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	return iHub->DoControl(ESuspend, (TAny*)iHandle);
	}


/**
Resumes the device from a suspended state.
*/
TInt RUsbDevice::Resume()
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	return iHub->DoControl(EResume, (TAny*)iHandle);
	}


TInt RUsbDevice::GetStringDescriptor(TDes8& aStringDescriptor, TInt aIndex, TInt aLangId)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	__ASSERT_ALWAYS(aStringDescriptor.MaxLength() >= 255,
		User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverInsufficientSizeToHoldStringDescriptor));

	aStringDescriptor.Zero();

	TStringDescParams stringDescParams;
	stringDescParams.iTarget = &aStringDescriptor;
	stringDescParams.iIndex  =  aIndex;
	stringDescParams.iLangId =  aLangId;

	return iHub->DoControl(EGetStringDescriptor, (TAny*)iHandle, &stringDescParams);
	}


/**
Return a token which may be used to uniquely identify the supplied interface on this device.  The returned
token may then be passed to a function driver, to allow it to open the required interface.

@param [in] aInterfaceNumber Interface to return a token for.
@param [out] aToken The token assigned to the interface.
@return System wide error code, for instance KErrNotFound if the supplied interface number is unknown.
*/
TInt RUsbDevice::GetTokenForInterface(TInt aInterfaceNumber, TUint32& aToken)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));

	TInterfaceTokenParameters params;
	params.iInterfaceNumber = aInterfaceNumber;
	params.iToken           = &aToken;

	return iHub->DoControl(EGetInterfaceToken, (TAny*)iHandle, &params);
	}

/**
Queues an asynchronous request for changes in the state of the device represented by this handle.

@param [out] aNewState The new state of the device
@param [out] aRequest The request status completed when a state change has occured.
*/
void RUsbDevice::QueueDeviceStateChangeNotification(TDeviceState& aNewState, TRequestStatus& aRequest)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	iHub->DoRequest(EDeviceStateChange, aRequest, (TAny*)iHandle, &aNewState);
	}


/**
Cancels an outstanding request for device state changes
@see QueueDeviceStateChangeNotification
*/
void RUsbDevice::CancelDeviceStateChangeNotification()
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	iHub->DoControl(ECancelDeviceStateChange, (TAny*)iHandle);
	}


/**
Return the USB Device Descriptor for this device.

Note: the supplied TUsbDeviceDescriptor is owned by the caller, but any descriptor objects linked to it
remain the property of the RUsbDevice object.  Memory leaks will result if the head pointer is not
cleaned up, but the pointed to objects should not be destroyed.

@param [out] aDescriptor The supplied TUsbDeviceDescriptor object will be populated from the data retrieved from the
device.

@return KErrNone on success, otherwise a system wide error code.
*/
TInt RUsbDevice::GetDeviceDescriptor(TUsbDeviceDescriptor& aDescriptor)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	aDescriptor = *iHeadDeviceDescriptor;
	return KErrNone;
	}

/**
Return the USB Configuration Descriptor for this device.

Note: the supplied TUsbConfigurationDescriptor is owned by the caller, but any descriptor objects linked to it
remain the property of the RUsbDevice object.  Memory leaks will result if the head pointer is not
cleaned up, but the pointed to objects should not be destroyed.

@param [out] aDescriptor The supplied TUsbConfigurationDescriptor object will be populated from the data retrieved from
the	device.  Note that the caller owns the head of the list, but not any children or peers.

@return KErrNone on success, otherwise a system wide error code.
*/
TInt RUsbDevice::GetConfigurationDescriptor(TUsbConfigurationDescriptor& aDescriptor)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	aDescriptor = *iHeadConfDescriptor;
	return KErrNone;
	}

TInt RUsbDevice::GetStringDescriptor(TUsbStringDescriptor*& aDescriptor, TDes8& aTarget, TInt aIndex)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));

	aDescriptor = NULL;
	// aTarget will be Zero-ed in the GetStringDescriptor overload.

	TInt err = GetStringDescriptor(aTarget, aIndex);
	if(err != KErrNone)
		{
		return err;
		}
	return ParseStringDescriptor(aDescriptor, aTarget);
	}

TInt RUsbDevice::GetStringDescriptor(TUsbStringDescriptor*& aDescriptor, TDes8& aTarget, TInt aIndex, TInt aLangId)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));

	aDescriptor = NULL;
	// aTarget will be Zero-ed in the GetStringDescriptor overload.

	TInt err = GetStringDescriptor(aTarget, aIndex, aLangId);
	if(err != KErrNone)
		{
		return err;
		}

	return ParseStringDescriptor(aDescriptor, aTarget);
	}
	
TInt RUsbDevice::ParseStringDescriptor(TUsbStringDescriptor*& aDescriptor, const TDesC8& aData)
	{
	TUsbGenericDescriptor* parsed = NULL;
	TInt err = UsbDescriptorParser::Parse(aData, parsed);
	if(err == KErrNone)
		{
		aDescriptor = TUsbStringDescriptor::Cast(parsed);
		if(aDescriptor)
			{
			return KErrNone;
			}
		}
	// If here then there has been an error when parsing the descriptor
	if(parsed)
		{
		parsed->DestroyTree();
		delete parsed;
		}
	return (err != KErrNone) ? err : KErrCorrupt;
	}




TInt RUsbDevice::GetDeviceDescriptor(TDes8& aDeviceDesc)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	return iHub->DoControl(EGetDeviceDescriptor, (TAny*)iHandle, &aDeviceDesc);
	}


TInt RUsbDevice::GetConfigurationDescriptorSize(TInt& aSize)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	return iHub->DoControl(EGetConfigurationDescriptorSize, (TAny*)iHandle, &aSize);
	}


TInt RUsbDevice::GetConfigurationDescriptor(TDes8& aConfigDesc)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbHubDriverPanicCat, UsbdiPanics::EUsbHubDriverRequestMadeWhileClosed));
	__ASSERT_DEBUG(iHub, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbDeviceHasHandleButNoHubDriver));
	return iHub->DoControl(EGetConfigurationDescriptor, (TAny*)iHandle, &aConfigDesc);
	}


#endif  // !__KERNEL_MODE__
