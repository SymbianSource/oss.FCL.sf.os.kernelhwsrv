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
// e32\include\drivers\usbcsc.inl
// Kernel side inline header file for USB device driver.
// 
//

/**
 @file usbcsc.inl
 @internalTechnology
*/

#ifndef __USBCSC_INL__
#define __USBCSC_INL__
 

//
// --- USB Logical Device Driver (LDD) ---
//

TUsbcScEndpointInfo* TUsbcScEndpoint::EndpointInfo()
	{
	return &iEndpointInfo;
	}


void TUsbcScEndpoint::SetClientReadPending(TBool aVal)
	{
	iClientReadPending = aVal;
	}


TBool TUsbcScEndpoint::ClientReadPending()
	{
	return iClientReadPending;
	}


void TUsbcScEndpoint::SetClientWritePending(TBool aVal)
	{
	iClientWritePending = aVal;
	}


TBool TUsbcScEndpoint::ClientWritePending()
	{
	return iClientWritePending;
	}


void TUsbcScEndpoint::SetRealEpNumber(TInt aRealEpNumber)
	{
	iRealEpNumber = aRealEpNumber;
	iRequestCallbackInfo->iRealEpNum = aRealEpNumber;
	}


TInt TUsbcScEndpoint::RealEpNumber() const
	{
	return iRealEpNumber;
	}
TInt TUsbcScEndpoint::EpNumber() const
	{
	return iEndpointNumber;
	}
void TUsbcScEndpoint::StartBuffer()
	{
	iBuffer->StartEndpoint(iRequestCallbackInfo, iEndpointInfo.iFlags);
	}

void TUsbcScEndpoint::SetBuffer(TUsbcScBuffer* aBuffer)
	{
		__ASSERT_ALWAYS(iBuffer==NULL, Kern::Fault("TUsbcScEndpoint::SetBuffer", __LINE__));
		iBuffer = aBuffer;
	}

TUsbcScBuffer* TUsbcScEndpoint::GetBuffer()
	{
		__ASSERT_DEBUG(iBuffer!=NULL, Kern::Fault("TUsbcScEndpoint::GetBuffer", __LINE__));
	return iBuffer;
	}

inline TBool DLddUsbcScChannel::ValidEndpoint(TInt aEndpoint)
	{
	return (aEndpoint <= iNumberOfEndpoints && aEndpoint >= 0);
	}

inline TBool DLddUsbcScChannel::Configured()
	{
	if (iValidInterface && 
		(iDeviceState == EUsbcDeviceStateConfigured || iDeviceState == EUsbcDeviceStateSuspended))
		return ETrue;
	return EFalse;
	}

#endif // __USBCSC_INL__
