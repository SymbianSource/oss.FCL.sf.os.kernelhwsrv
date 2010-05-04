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
*/

#ifndef USBTRANSFERSTRATEGY_H
#define USBTRANSFERSTRATEGY_H

#include <e32std.h>
#include <d32usbtransfers.h>


NONSHARABLE_CLASS(RUsbTransferStrategy)
	{
public:
	virtual void Close();

	virtual TInt RegisterTransferDescriptor(RUsbTransferDescriptor& aTransferDesc, TInt aRequiredSize, TUint aStartAlignment, TInt aRequiredMaxPackets) =0;
	virtual void ResetTransferDescriptors() =0;
	virtual TInt InitialiseTransferDescriptors(RUsbInterface& aInterface) =0;

public: // Interrupt transfer descriptor methods
	virtual TPtr8	IntrWritableBuffer(TInt aHandle) =0;
	virtual void	IntrSaveData(TInt aHandle, TInt aLength) =0;
	virtual void	IntrSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus) =0;
	virtual TPtrC8	IntrBuffer(TInt aHandle) const =0;

public: // Bulk transfer descriptor methods
	virtual TPtr8	BulkWritableBuffer(TInt aHandle) =0;
	virtual void	BulkSaveData(TInt aHandle, TInt aLength) =0;
	virtual void	BulkSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus) =0;
	virtual TPtrC8	BulkBuffer(TInt aHandle) const =0;

public: // Isochronous transfer descriptor methods
	virtual void	IsocReset(TInt aHandle) =0;
	virtual TPacketLengths IsocLengths(TInt aHandle) =0;
	virtual TPacketResults IsocResults(TInt aHandle) =0;
	virtual TInt	IsocMaxPacketSize(TInt aHandle) =0;
	virtual TPtr8	IsocWritablePackets(TInt aHandle, TInt aWriteHandle, TInt aNumPacketsRequested, TInt& aMaxNumPacketsAbleToWrite) =0;
	virtual TInt	IsocSaveMultiple(TInt aHandle, TInt aWriteHandle, TInt aNumOfPackets) =0;
	virtual TPtrC8	IsocPackets(TInt aHandle, TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumPacketsReturned) const =0;
	virtual void	IsocReceivePackets(TInt aHandle, TInt aNumOfPackets) =0;

protected:
	void SetTransferHandle(RUsbTransferDescriptor& aTransfer, TInt aHandle) const;
	};


#endif // USBTRANSFERSTRATEGY_H
