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

#ifndef ZEROCOPYTRANSFERSTRATEGY_H
#define ZEROCOPYTRANSFERSTRATEGY_H

#include "usbtransferstrategy.h"


NONSHARABLE_CLASS(RUsbZeroCopyTransferStrategy) : public RUsbTransferStrategy
	{
public:
	RUsbZeroCopyTransferStrategy();
	virtual TInt RegisterTransferDescriptor(RUsbTransferDescriptor& aTransferDesc, TInt aRequiredSize, TUint aStartAlignment, TInt aRequiredMaxPackets);
	virtual void ResetTransferDescriptors();
	virtual TInt InitialiseTransferDescriptors(RUsbInterface& aInterface);

	virtual void Close();

public: // Interrupt transfer descriptor methods
	virtual TPtr8	IntrWritableBuffer(TInt aHandle);
	virtual void	IntrSaveData(TInt aHandle, TInt aLength);
	virtual void	IntrSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus);
	virtual TPtrC8	IntrBuffer(TInt aHandle) const;
public: // Bulk transfer descriptor methods
	virtual TPtr8	BulkWritableBuffer(TInt aHandle);
	virtual void	BulkSaveData(TInt aHandle, TInt aLength);
	virtual void	BulkSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus);
	virtual TPtrC8	BulkBuffer(TInt aHandle) const;
public: // Isochronous transfer descriptor methods
	virtual void	IsocReset(TInt aHandle);
	virtual TPacketLengths IsocLengths(TInt aHandle);
	virtual TPacketResults IsocResults(TInt aHandle);
	virtual TInt	IsocMaxPacketSize(TInt aHandle);
	virtual TPtr8	IsocWritablePackets(TInt aHandle, TInt aWriteHandle, TInt aNumPacketsRequested, TInt& aMaxNumPacketsAbleToWrite);
	virtual TInt	IsocSaveMultiple(TInt aHandle, TInt aWriteHandle, TInt aNumOfPackets);
	virtual TPtrC8	IsocPackets(TInt aHandle, TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumPacketsReturned) const;
	virtual void	IsocReceivePackets(TInt aHandle, TInt aNumOfPackets);


private: // Standard (Bulk, Ctrl and Intr) Buffer methods
	TPtr8	WritableBuffer(TInt aHandle);
	void	SaveData(TInt aHandle, TInt aLength);
	void	SetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus);
	TPtrC8	Buffer(TInt aHandle) const;

private: // Isoc Buffer methods
	void	Reset(TInt aHandle);
	TPacketLengths Lengths(TInt aHandle);
	TPacketResults Results(TInt aHandle);
	TInt	MaxPacketSize(TInt aHandle);
	TPtr8	WritablePackets(TInt aHandle, TInt aWriteHandle, TInt aNumPacketsRequested, TInt& aMaxNumPacketsAbleToWrite);
	TInt	SaveMultiple(TInt aHandle, TInt aWriteHandle, TInt aNumOfPackets);
	TPtrC8	Packets(TInt aHandle, TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumPacketsReturned) const;
	void	ReceivePackets(TInt aHandle, TInt aNumOfPackets);
	
private:
	NONSHARABLE_STRUCT(TUsbTransferDescriptorDetails)
		{
        TUsbTransferDescriptorDetails(RUsbTransferDescriptor&, TInt, TUint, TInt);
		RUsbTransferDescriptor& iTransferDesc;
		const TInt iRequiredSize;
		TUint iRequiredAlignment;
		const TInt iRequiredMaxPackets;
		// Members to aid internal logic
		TInt iAssignedOffset;
		TInt iLengthsOffset; // Only applicable to isoc
		TInt iReqLenOffset; // Only applicable to isoc
		TInt iResultsOffset; // Only applicable to isoc
		TInt iNumElements; // Only applicable to isoc
		};

private:
	TInt CalculateDataLayout(TInt& aCurrentOffset, TInt& aNumStandardTransfers, TInt& aNumIsocTransfers, TInt& aNumIsocElements);
	void CalculateMetaDataLayout(TInt& aCurrentOffset, TInt& aMetaDataStart, TInt aNumStandardTransfers, TInt aNumIsocTransfers, TInt aNumIsocElements);
	void InitialiseMetaData(TInt aMetaDataOffset, TInt aNumStandardTransfers, TInt aNumIsocTransfers, TInt aNumIsocElements);
	TInt UsedPackets(TInt aHeaderOffset);
	TBool IsPowerOfTwo(TUint aNumber);
	TInt IncNeededToAlign(TInt aOffset, TUint aAlignment);
	static TBool CompareTransferDescriptor(const RUsbTransferDescriptor* aTransferDesc, const TUsbTransferDescriptorDetails& aDetails);

private: //Calculate additional alignment related methods
	TInt GetMaximumMaxPacketSize(TInt& aMaxMaxBulk, TInt& aMaxMaxInterrupt);
	TInt CaculateAdditionalAlignment(TInt aCurrentOffset, TInt aMaxMaxBulk, TInt aMaxMaxInterrupt, TUsbTransferDescriptorDetails& aTransferDetails);
private:
	RArray<TUsbTransferDescriptorDetails> iRegisteredTransfers;

private:
	RUsbInterface* iInterfaceHandle;
	RChunk iChunk;
	TInt iBaseOffset;
	TInt iPageSize;
	};

#endif // ZEROCOPYTRANSFERSTRATEGY_H
