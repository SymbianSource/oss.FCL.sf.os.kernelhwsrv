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
 
 The driver's name
 
 @return The name of the driver
 
 @internalComponent
*/
const TDesC& RUsbInterface::Name()
	{
	_LIT(KDriverName,"USBDI");
	return KDriverName;
	}

/**
The driver's version

@return The version number of the driver

@internalComponent
*/
TVersion RUsbInterface::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}



#ifndef __KERNEL_MODE__
RUsbInterface::RUsbInterface()
	: iHeadInterfaceDescriptor(NULL)
	, iInterfaceDescriptorData(NULL)
	, iTransferStrategy(NULL)
	, iAlternateSetting(0)
	{
	}

/**
Signals to the hub driver that this interface is idle and may be suspended.
As suspend operates at the device level, this will only trigger a removal of bus activity if all interfaces
associated with the device are marked as suspended.

@param [in] aResumeSignal The TRequestStatus that will be completed when the interface is resumed.
*/
void RUsbInterface::PermitSuspendAndWaitForResume(TRequestStatus& aResumeSignal)
	{
	DoRequest(ESuspend, aResumeSignal);
	}

/**
Cancel the outstanding permission to suspend.
*/
void RUsbInterface::CancelPermitSuspend()
	{
	DoCancel(ECancelSuspend);
	}

/**
Cancel the outstanding PermitSuspendAndWaitForResume request with KErrCancel
*/
void RUsbInterface::CancelWaitForResume()
	{
	DoCancel(ECancelWaitForResume);
	}

/**
Request or clear the interface's remote wakeup flag.  If any interface on the device
has this flag set, suspending the device will cause it to have remote wakeup capability
enabled.  This function may only be called when the interface is active -- the device will
not be woken to change the status if it is currently suspended.
Note that clearing this flag will not prevent a device from using remote wakeup -- this
will happen only if all interfaces on the device do not require it.
By default the device will not have remote wakeup enabled.

@param aAllowed ETrue if remote wakeup should be permitted, EFalse if this interface does
not require it.

@return KErrNotReady if two calls have been made in succession with the same parameter.
KErrUsbDeviceSuspended if the interface is currently marked as suspended.
*/
TInt RUsbInterface::PermitRemoteWakeup(TBool aAllowed)
	{
	return DoControl(EPermitRemoteWakeup, (TAny*)aAllowed);
	}


/**
Select the specified alternate interface.

Asserts that all open pipes have been closed.

@param [in] aAlternateInterface The alternate interface to select.

@return KErrArgument if the specified alternate interface does not exist.
@return KErrOverflow if selecting this alternate interface would overcommit the bus' bandwidth.
*/
TInt RUsbInterface::SelectAlternateInterface(TInt aAlternateInterface)
	{
	TInt err = DoControl(ESelectAlternateInterface, (TAny*)aAlternateInterface);
	if(err == KErrNone)
		{
		iAlternateSetting = aAlternateInterface;
		}
	return err;
	}

TInt RUsbInterface::GetStringDescriptor(TDes8& aStringDescriptor, TUint8 aIndex, TUint16 aLangId)
	{
	TUint32 params;
	params = aIndex | (aLangId << 16);
	return DoControl(EGetStringDescriptor, &aStringDescriptor, &params);
	}


/**  
Performs an Endpoint 0 transfer.
*/
void RUsbInterface::Ep0Transfer(TUsbTransferRequestDetails& aDetails, const TDesC8& aSend, TDes8& aRecv, TRequestStatus& aRequest)
	{
	aDetails.iSend         = &aSend;
	aDetails.iRecv         = &aRecv;
	DoRequest(EEp0Transfer, aRequest, (TAny*)&aDetails);
	}

/**  
Cancel an Endpoint 0 transfer.
*/
void RUsbInterface::CancelEP0Transfer()
	{
	DoCancel(ECancelEp0Transfer);
	}

/**
Establish a pipe between host and device.  The RUsbPipe object returned is ready for use.

@param aPipe The pipe to connect to the remote endpoint. [out]
@param aEndpoint The endpoint on the remote device to connect to. [in]
@param aUseDMA In future implementations where DMA is supported this flag indicates DMA must be used. The Open attempt will fail if DMA cannot be offered on the pipe. [in]

@return KErrArgument if the specified endpoint does not exist.
@see SelectAlternateInterface
*/
TInt RUsbInterface::OpenPipeForEndpoint(RUsbPipe& aPipe, TInt aEndpoint, TBool /*aUseDMA*/)
	{
	if(aPipe.iHandle)
		{
		return KErrInUse;
		}

	TInt err = GetEndpointDescriptor(iAlternateSetting, aEndpoint, aPipe.iHeadEndpointDescriptor);
	// Allow KErrNotFound as audio endpoint descriptors are not valid endpoint descriptors
	if ((err == KErrNone) || (err == KErrNotFound))
		{
		TUint32 pipeHandle;
		err = DoControl(EOpenPipe, &pipeHandle, reinterpret_cast<TAny*>(aEndpoint));
		if (err == KErrNone)
			{
			aPipe.iHandle = pipeHandle;
			aPipe.iInterface = this;
			}
		}

	return err;
	}

/**
@internalComponent
*/
TInt RUsbInterface::AllocateSharedChunk(RChunk& aChunk, TInt aSize, TInt& aOffset)
	{
	TInt chunkHandle = 0;
	RUsbInterface::TChunkRequestDetails details;
	details.iRequestSize = aSize;
	details.iChunkHandle = &chunkHandle;
	details.iOffset = &aOffset;
	TInt err = DoControl(EAllocChunk, &details);
	if(err == KErrNone)
		{
		aChunk.SetHandle(chunkHandle);
		}
	return err;
	}

/**
Return the section of the USB Configuration Descriptor under this interface, including any alternate
interfaces.

Note: the supplied TUsbInterfaceDescriptor is owned by the caller, but any descriptor objects linked to it
remain the property of the RUsbInterface object.  Memory leaks will result if the head pointer is not
cleaned up, but the pointed to objects should not be destroyed.

@param [out] aDescriptor The supplied TUsbInterfaceDescriptor object will be populated from the data retrieved from
the	device.  Note that the caller owns the head of the list, but not any children or peers.

@return System wide error code.
*/
TInt RUsbInterface::GetInterfaceDescriptor(TUsbInterfaceDescriptor& aDescriptor)
	{
	if (!iHeadInterfaceDescriptor)
		{
		return KErrNotReady;
		}

	aDescriptor = *iHeadInterfaceDescriptor;
	return KErrNone;
	}

/**
Find and return the section of the USB Configuration Descriptor under the supplied alternate interface.

Note: the supplied TUsbInterfaceDescriptor is owned by the caller, but any descriptor objects linked to it
remain the property of the RUsbInterface object.  Memory leaks will result if the head pointer is not
cleaned up, but the pointed to objects should not be destroyed.

@param aAlternateInterface The alternate interface number to return the descriptor for. [in]
@param aDescriptor The supplied TUsbInterfaceDescriptor object will be populated from the data retrieved from
the	device.  Note that the caller owns the head of the list, but not any children or peers. [out]

@return KErrArgument if the specified alternate interface does not exist.
*/
TInt RUsbInterface::GetAlternateInterfaceDescriptor(TInt aAlternateInterface, TUsbInterfaceDescriptor& aDescriptor)
	{
	if (!iHeadInterfaceDescriptor)
		{
		return KErrNotReady;
		}

	TUsbGenericDescriptor* descriptor = iHeadInterfaceDescriptor;
	while (descriptor)
		{
		TUsbInterfaceDescriptor* interface = TUsbInterfaceDescriptor::Cast(descriptor);
		if (interface)
			{
			if (interface->AlternateSetting() == aAlternateInterface)
				{
				aDescriptor = *interface;
				return KErrNone;
				}
			}
        // we must check any Interface Association Descriptors for
        // Alternate Interface settings.  The spec is abiguous on how these may be organised so we
        // presume the worst and do a full search
		TUsbInterfaceAssociationDescriptor* iad = TUsbInterfaceAssociationDescriptor::Cast(descriptor);
		if (iad)
			{
			TUsbGenericDescriptor* assocDes = iad->iFirstChild;
			while (assocDes)
				{
				interface = TUsbInterfaceDescriptor::Cast(assocDes);
				if (interface)
					{
					if (interface->AlternateSetting() == aAlternateInterface)
						{
						aDescriptor = *interface;
						return KErrNone;
						}
					}
				assocDes = assocDes->iNextPeer;
				}
			}
		descriptor = descriptor->iNextPeer;
		}

	return KErrArgument;
	}

/**
Find and return the section of the USB Configuration Descriptor under the supplied endpoint.

Note: the supplied TUsbEndpointDescriptor is owned by the caller, but any descriptor objects linked to it
remain the property of the RUsbInterface object.  Memory leaks will result if the head pointer is not
cleaned up, but the pointed to objects should not be destroyed.

@param aAlternateInterface The alternate interface number to return the descriptor for. [in]
@param aEndpoint The endpoint number to return the descriptor for. [in]
@param aDescriptor The supplied TUsbEndpointDescriptor object will be populated from the data retrieved from
the	device.  Note that the caller owns the head of the list, but not any children or peers. [out]

@return KErrArgument if the specified alternate interface does not exist, or KErrNotFound if the specified
endpoint cannot be found on the alternate interface.
*/
TInt RUsbInterface::GetEndpointDescriptor(TInt aAlternateInterface, TInt aEndpoint, TUsbEndpointDescriptor& aDescriptor)
	{
	TUsbEndpointDescriptor* descriptor = &aDescriptor;
	TInt err = GetEndpointDescriptor(aAlternateInterface, aEndpoint, descriptor);
    if ((err == KErrNone) && descriptor)
		{
		aDescriptor = *descriptor;
		}
	return err;
	}

TInt RUsbInterface::GetEndpointDescriptor(TInt aAlternateInterface, TInt aEndpoint, TUsbEndpointDescriptor*& aDescriptor)
	{
	aDescriptor = NULL;

	TUsbInterfaceDescriptor alternate;
	TInt err = GetAlternateInterfaceDescriptor(aAlternateInterface, alternate);
	if (err != KErrNone)
		{
		return err;
		}

	TUsbGenericDescriptor* descriptor = alternate.iFirstChild;
	while (descriptor)
		{
		TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(descriptor);
		if (endpoint && (endpoint->EndpointAddress() == aEndpoint))
			{
			aDescriptor = endpoint;
			return KErrNone;
			}

		descriptor = descriptor->iNextPeer;
		}

	return KErrNotFound;
	}

/**
@return Number of alternate interface options on this interface.
*/
TInt RUsbInterface::GetAlternateInterfaceCount()
	{
	if (!iHeadInterfaceDescriptor)
		{
		return KErrNotReady;
		}

	TInt count = 0;

	// Don't need to look for children of the interface -- all the alternates
    // must be peers.
	TUsbGenericDescriptor* descriptor = iHeadInterfaceDescriptor;
	while (descriptor)
		{
		TUsbInterfaceDescriptor* interface = TUsbInterfaceDescriptor::Cast(descriptor);
		if (interface)
			{
			++count;
			}
		else
            {
            // we must check any Interface Association Descriptors for
            // Alternate Interface settings.  The spec is abiguous on how these may be organised so we
            // presume the worst and do a full search
            TUsbInterfaceAssociationDescriptor* iad = TUsbInterfaceAssociationDescriptor::Cast(descriptor);
            if (iad)
                {
                TUsbGenericDescriptor* assocDes = iad->iFirstChild;
                while (assocDes)
                    {
                    interface = TUsbInterfaceDescriptor::Cast(assocDes);
                    if (interface)
                        {
                        ++count;
                        }
                    assocDes = assocDes->iNextPeer;
                    }
                }
            }
		descriptor = descriptor->iNextPeer;
		}

	return count;
	}

/**
Count the endpoints on an alternate interface.

@param [in] aAlternateInterface The alternate interface to count endpoints on.
@return Number of endpoionts on the requested alternate interface or an error code.
*/
TInt RUsbInterface::EnumerateEndpointsOnInterface(TInt aAlternateInterface)
	{
	TUsbInterfaceDescriptor alternate;
	TInt err = GetAlternateInterfaceDescriptor(aAlternateInterface, alternate);
	if (err != KErrNone)
		{
		return err;
		}

	return alternate.NumEndpoints();
	}
	
/**
Returns an identifier that is unique for the bus that the device that provides this interface is on.
@param aBusId On success provides an identifier that is unique for the bus this interface is on.
@return KErrNone on success, otherwise a system-wide error code.
*/
TInt RUsbInterface::GetBusId(TUsbBusId& aBusId)
	{
	return DoControl(EGetBusId, &aBusId);
	}

/**
Returns the size of pages used by the HCD.
@internalComponent
@param aHcdPageSize on success provides the HCD's page size.
@return KErrNone on success, otherwise a system-wide error code.
*/
TInt RUsbInterface::GetHcdPageSize(TInt& aHcdPageSize)
	{
	return DoControl(EHcdPageSize, &aHcdPageSize);
	}

/**
Returns the speed the remote device is connected at.
@param aDeviceSpeed On sucess an enumeration value describing the current speed of the remote device.
@return KErrNone on success, otherwise a system-wide error code.
*/
TInt RUsbInterface::GetDeviceSpeed(RUsbInterface::TDeviceSpeed& aDeviceSpeed)
	{
	return DoControl(EGetDeviceSpeed, &aDeviceSpeed);
	}





RUsbPipe::RUsbPipe()
	: iHandle(0)
	, iInterface(NULL)
	{
	}

TUint32 RUsbPipe::Handle() const
	{
	return iHandle;
	}

/**
Close a pipe to a remote device.
*/
void RUsbPipe::Close()
	{
	if (iInterface)
		{
		static_cast<void>(iInterface->DoControl(EClose, (TAny*)iHandle));
		}
	iHeadEndpointDescriptor = NULL;
	iInterface = NULL;
	iHandle = 0;
	}

/**
Clear a stall on the remote endpoint.

@return System-wide error code.
*/
TInt RUsbPipe::ClearRemoteStall()
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
	__ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));
	return iInterface->DoControl(EClearRemoteStall, (TAny*)iHandle);
	}

/**
Cancel all queued transfers
*/
void RUsbPipe::CancelAllTransfers()
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
	__ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));
	static_cast<void>(iInterface->DoControl(EAbort, (TAny*)iHandle));
	}

/**
Issues a transfer.
@internalComponent
*/
void RUsbPipe::IssueTransfer(TInt aTransferHandle, TRequestStatus& aRequest)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
	__ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));
	iInterface->DoRequest(EIssueTransfer, aRequest, (TAny*)iHandle, (TAny*)aTransferHandle);
	}

/**
Get endpoint ID
*/
TInt RUsbPipe::GetEndpointId(TUsbEndpointId& aEndpointId)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
	__ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));
	return iInterface->DoControl(EGetEndpointId, reinterpret_cast<TAny*>(iHandle), &aEndpointId);
	}

/**
Get Bus ID
*/
TInt RUsbPipe::GetBusId(TUsbBusId& aBusId)
	{
	__ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
	__ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));
	return iInterface->GetBusId(aBusId);
	}

/**
Return the section of the USB Configuration Descriptor under the supplied endpoint.

@param [out] aDescriptor The descriptor tree for this endpoint.
@return System-wide error code.
*/
TInt RUsbPipe::GetEndpointDescriptor(TUsbEndpointDescriptor& aDescriptor)
	{
    __ASSERT_ALWAYS(iHandle, User::Panic(UsbdiPanics::KUsbdiPanicCat, UsbdiPanics::EPipeRequestMadeWhileClosed));
    __ASSERT_DEBUG(iInterface, User::Panic(UsbdiFaults::KUsbdiFaultCat, UsbdiFaults::EUsbPipeHasHandleButNoInterface));

	if (iHeadEndpointDescriptor)
		{
		aDescriptor = *iHeadEndpointDescriptor;
		return KErrNone;
		}
	else
		{
		return KErrNotFound;
		}
	}

#endif
