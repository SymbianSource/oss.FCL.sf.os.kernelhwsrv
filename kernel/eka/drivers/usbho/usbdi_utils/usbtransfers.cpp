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

#include <d32usbtransfers.h>

#include <d32usbdi_errors.h>
#include "usbtransferstrategy.h"
#include "usbdiutils.h"


// ========================
// RUsbTransferDescriptor
// ========================

/**
Constructor protected to as this class is only intended as a base class.
*/
RUsbTransferDescriptor::RUsbTransferDescriptor(TTransferType aType, TInt aMaxSize, TInt aMaxNumPackets)
	: iHandle(KInvalidHandle)
	, iType(aType)
	, iMaxSize(aMaxSize)
	, iMaxNumPackets(aMaxNumPackets)
	{
	}

/**
Releases resources allocated to this transfer descriptor.
*/
EXPORT_C void RUsbTransferDescriptor::Close()
	{
	// Do nothing - the buffer is owned by the {R,D}UsbInterface.
	// This is provided in case the descriptor owns resources in future.
	}


// ============================
// RUsbIsocTransferDescriptor
// ============================

EXPORT_C RUsbIsocTransferDescriptor::RUsbIsocTransferDescriptor(TInt aMaxSize, TInt aMaxNumPackets)
	: RUsbTransferDescriptor(EIsochronous, aMaxSize, aMaxNumPackets)
	, iWriteHandle(KInvalidHandle)
	{
	}

EXPORT_C void RUsbIsocTransferDescriptor::Reset()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	iWriteHandle = iHandle;
	iTransferStrategy->IsocReset(iHandle);
	}

EXPORT_C TPacketLengths RUsbIsocTransferDescriptor::Lengths()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	return iTransferStrategy->IsocLengths(iHandle);
	}
	
EXPORT_C TPacketResults RUsbIsocTransferDescriptor::Results()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	return iTransferStrategy->IsocResults(iHandle);
	}
	
EXPORT_C TInt RUsbIsocTransferDescriptor::MaxPacketSize()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	return iTransferStrategy->IsocMaxPacketSize(iHandle);
	}

EXPORT_C TPtr8 RUsbIsocTransferDescriptor::WritablePackets(TInt aNumPacketsRequested, TInt& aMaxNumOfPacketsAbleToWrite)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	if(iWriteHandle == KInvalidHandle)
		{
		return TPtr8(NULL, 0);
		}
	return iTransferStrategy->IsocWritablePackets(iHandle, iWriteHandle, aNumPacketsRequested, aMaxNumOfPacketsAbleToWrite);
	}

EXPORT_C void RUsbIsocTransferDescriptor::SaveMultiple(TInt aNumOfPackets)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	__ASSERT_ALWAYS(iWriteHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorWriteHandle));
	TInt writeHandle = iTransferStrategy->IsocSaveMultiple(iHandle, iWriteHandle, aNumOfPackets);
	iWriteHandle = (writeHandle < 0) ? KInvalidHandle : writeHandle;
	}

EXPORT_C TPtrC8 RUsbIsocTransferDescriptor::Packets(TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumOfPacketsReturned) const
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	return iTransferStrategy->IsocPackets(iHandle, aFirstPacketIndex, aNumPacketsRequested, aNumOfPacketsReturned);
	}

EXPORT_C void RUsbIsocTransferDescriptor::ReceivePackets(TInt aNumOfPackets)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIsocTransferDescriptorHandle));
	iTransferStrategy->IsocReceivePackets(iHandle, aNumOfPackets);
	}


// ============================
// RUsbBulkTransferDescriptor
// ============================

EXPORT_C RUsbBulkTransferDescriptor::RUsbBulkTransferDescriptor(TInt aMaxSize)
	: RUsbTransferDescriptor(EBulk, aMaxSize, 0)
	{
	}

/**
@return A modifiable pointer to the entire data buffer.
*/
EXPORT_C TPtr8 RUsbBulkTransferDescriptor::WritableBuffer()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadBulkTransferDescriptorHandle));
	return iTransferStrategy->BulkWritableBuffer(iHandle);
	}

/**
Update the transfer descriptor given the length of data supplied.
@param[in] aLength Length of data to write or expect.
*/
EXPORT_C void RUsbBulkTransferDescriptor::SaveData(TInt aLength)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadBulkTransferDescriptorHandle));
	iTransferStrategy->BulkSaveData(iHandle, aLength);
	}

/**
@return A non-modifiable pointer to the entire data buffer.
*/
EXPORT_C TPtrC8 RUsbBulkTransferDescriptor::Buffer() const
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadBulkTransferDescriptorHandle));
	return iTransferStrategy->BulkBuffer(iHandle);
	}
	
/**
@param aZlpStatus the ZLP type to use for the transfer
*/
EXPORT_C void RUsbBulkTransferDescriptor::SetZlpStatus(TZlpStatus aZlpStatus)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadBulkTransferDescriptorHandle));
	iTransferStrategy->BulkSetZlpStatus(iHandle, aZlpStatus);
	}


// ============================
// RUsbIntrTransferDescriptor
// ============================

EXPORT_C RUsbIntrTransferDescriptor::RUsbIntrTransferDescriptor(TInt aMaxSize)
	: RUsbTransferDescriptor(EInterrupt, aMaxSize, 0)
	{
	}

/**
@return A modifiable pointer to the entire data buffer.
*/
EXPORT_C TPtr8 RUsbIntrTransferDescriptor::WritableBuffer()
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIntrTransferDescriptorHandle));
	return iTransferStrategy->IntrWritableBuffer(iHandle);
	}

/**
Update the transfer descriptor given the length of data supplied.
@param[in] aLength Length of data to write or expect.
*/
EXPORT_C void RUsbIntrTransferDescriptor::SaveData(TInt aLength)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIntrTransferDescriptorHandle));
	iTransferStrategy->IntrSaveData(iHandle, aLength);
	}

/**
@return A non-modifiable pointer to the entire data buffer.
*/
EXPORT_C TPtrC8 RUsbIntrTransferDescriptor::Buffer() const
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIntrTransferDescriptorHandle));
	return iTransferStrategy->IntrBuffer(iHandle);
	}
	
/**
@param aZlpStatus the ZLP type to use for the transfer
*/
EXPORT_C void RUsbIntrTransferDescriptor::SetZlpStatus(TZlpStatus aZlpStatus)
	{
	__ASSERT_ALWAYS(iHandle != KInvalidHandle, UsbdiUtils::Panic(UsbdiPanics::EBadIntrTransferDescriptorHandle));
	return iTransferStrategy->IntrSetZlpStatus(iHandle, aZlpStatus);
	}
