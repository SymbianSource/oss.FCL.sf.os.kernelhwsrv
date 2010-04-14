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

#include "usbtransferstrategy.h"

#include <d32usbtransfers.h>
#include <d32usbdi_errors.h>
#include "usbdiutils.h"


TPacketLengths::TPacketLengths(TUint16* aRecvPtr, TUint16* aReqPtr, TInt& aMaxNumPackets)
	: iRecvPtr(aRecvPtr)
	, iReqPtr(aReqPtr)
	, iMaxNumPackets(aMaxNumPackets)
	{}

EXPORT_C TPacketLengths::TLength TPacketLengths::At(TInt aIndex)
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < iMaxNumPackets, UsbdiUtils::Panic(UsbdiPanics::EOutOfBoundsOfLengthArray));
	return TPacketLengths::TLength(*(iRecvPtr + aIndex), *(iReqPtr + aIndex));
	}

EXPORT_C const TPacketLengths::TLength TPacketLengths::At(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < iMaxNumPackets, UsbdiUtils::Panic(UsbdiPanics::EOutOfBoundsOfLengthArray));
	return TPacketLengths::TLength(*(iRecvPtr + aIndex), *(iReqPtr + aIndex));
	}

EXPORT_C TPacketLengths::TLength TPacketLengths::operator[](TInt aIndex)
	{
	return At(aIndex);
	}

EXPORT_C const TPacketLengths::TLength TPacketLengths::operator[](TInt aIndex) const
	{
	return At(aIndex);
	}

EXPORT_C TInt TPacketLengths::MaxNumPackets()
	{
	return iMaxNumPackets;
	}
	
EXPORT_C TUint16 TPacketLengths::TLength::operator=(TUint16 aValue)
	{
	iRecv = aValue;
	iReq = aValue;
	return aValue;
	}

EXPORT_C TPacketLengths::TLength::operator TUint16() const
	{
	return iRecv;
	}

TPacketLengths::TLength::TLength(TUint16& aRecv, TUint16& aReq)
	: iRecv(aRecv)
	, iReq(aReq)
	{
	}
	
	
TPacketResults::TPacketResults(TInt* aResPtr, TInt& aMaxNumPackets)
	: iResPtr(aResPtr)
	, iMaxNumPackets(aMaxNumPackets)
	{
    }

EXPORT_C TInt TPacketResults::At(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < iMaxNumPackets, UsbdiUtils::Panic(UsbdiPanics::EOutOfBoundsOfResultArray));
	return *(iResPtr + aIndex);
	}

EXPORT_C TInt TPacketResults::operator[](TInt aIndex) const
	{
	return At(aIndex);
	}

EXPORT_C TInt TPacketResults::MaxNumPackets()
	{
	return iMaxNumPackets;
	}



void RUsbTransferStrategy::Close()
	{
	// Doesn't currently own any resources.
	}
	
void RUsbTransferStrategy::SetTransferHandle(RUsbTransferDescriptor& aTransfer, TInt aHandle) const
	{
	aTransfer.iHandle = aHandle;
	aTransfer.iTransferStrategy = const_cast<RUsbTransferStrategy*>(this);
	if(aTransfer.iType == RUsbTransferDescriptor::EIsochronous)
		{
		static_cast<RUsbIsocTransferDescriptor&>(aTransfer).iWriteHandle = aHandle;
		}
	}


