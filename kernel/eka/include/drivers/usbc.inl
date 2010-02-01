// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\usbc.inl
// Kernel side inline header file for USB device driver.
// 
//

/**
 @file usbc.inl
 @internalTechnology
*/

#ifndef __USBC_INL__
#define __USBC_INL__


// --- USB Logical Device Driver (LDD) ---
//

TInt TDmaBuf::RxBytesAvailable() const
	{
	return iTotalRxBytesAvail;
	}


TUsbcEndpointInfo* TUsbcEndpoint::EndpointInfo()
	{
	return &iEndpointInfo;
	}


TInt TUsbcEndpoint::RxBytesAvailable() const
	{
	return iDmaBuffers->RxBytesAvailable();
	}

TInt TUsbcEndpoint::BufferSize() const
    {
    return iDmaBuffers->BufferSize();
    }
TInt TUsbcEndpoint::SetBufferAddr( TInt aBufInd, TUint8* aBufAddr)
    {
    return iDmaBuffers->SetBufferAddr(aBufInd, aBufAddr);
    }
TInt TUsbcEndpoint::BufferNumber() const
    {
    return iDmaBuffers->BufferNumber();
    }

void TUsbcEndpoint::SetTransferInfo(TEndpointTransferInfo* aTransferInfo)
	{
	iTransferInfo = *aTransferInfo;
	iBytesTransferred = 0;
	}


void TUsbcEndpoint::ResetTransferInfo()
	{
	iTransferInfo.iDes = NULL;
	iTransferInfo.iTransferType = ETransferTypeNone;
	iTransferInfo.iTransferSize = 0;
	iTransferInfo.iZlpReqd = EFalse;
	iBytesTransferred = 0;
	}


void TUsbcEndpoint::SetClientReadPending(TBool aVal)
	{
	iClientReadPending = aVal;
	}


TBool TUsbcEndpoint::ClientReadPending()
	{
	return iClientReadPending;
	}


void TUsbcEndpoint::SetClientWritePending(TBool aVal)
	{
	iClientWritePending = aVal;
	}


TBool TUsbcEndpoint::ClientWritePending()
	{
	return iClientWritePending;
	}


void TUsbcEndpoint::SetRealEpNumber(TInt aRealEpNumber)
	{
	iRealEpNumber = aRealEpNumber;
	iRequestCallbackInfo->iRealEpNum = aRealEpNumber;
	}


TInt TUsbcEndpoint::RealEpNumber() const
	{
	return iRealEpNumber;
	}


#endif // __USBC_INL__
