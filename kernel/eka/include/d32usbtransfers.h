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

#ifndef __D32USBTRANSFERS_H
#define __D32USBTRANSFERS_H

#ifdef __KERNEL_MODE__
#include <kernel/klib.h>
#else
#include <e32base.h>
#endif
#include <d32usbdi.h>


class RUsbTransferStrategy;

/**
Base class for all transfer descriptors.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(RUsbTransferDescriptor)
	{
public:
	enum TTransferType
		{
		EBulk,
		EIsochronous,
		EInterrupt
		};

	enum TZlpStatus
		{
		ESuppressZlp,
		ESendZlpIfRequired, // Default
		EAlwaysSendZlp
		};

#ifndef __KERNEL_MODE__
friend class RUsbPipe;
friend class RUsbTransferStrategy;

public:
	virtual void Close();

protected:
	RUsbTransferDescriptor(TTransferType aType, TInt aMaxSize, TInt aMaxNumPackets);
	
protected:
	static const TInt KInvalidHandle = -1;

protected:
	/**
	A pointer to the transfer strategy the descriptor is registered in.
	*/
	RUsbTransferStrategy* iTransferStrategy;

	/**
	Handle into the transfer strategy for the descriptor.
	*/
	TInt iHandle;

public:
	/**
	The type of transfer descriptor this instance represents.
	*/
	const TTransferType iType;

	/**
	For isochronous transfers this refers to the maximum packet size packets
	in this descriptor may be.
	For other transfers this refers to the maximum size of the transfer.
	*/
	const TInt iMaxSize;

	/**
	Used to specify the maximum number of packets the descriptor will hold.
	*/
	const TInt iMaxNumPackets;
#endif // __KERNEL_MODE__
	};


#ifndef __KERNEL_MODE__

/**
A class that refers to the list of packet lengths for a isochronous transfer
descriptor.

@publishedPartner
@prototype
*/
NONSHARABLE_CLASS(TPacketLengths)
	{
public:
	NONSHARABLE_CLASS(TLength)
		{
	public:
		IMPORT_C TUint16 operator=(TUint16 aValue);
		IMPORT_C operator TUint16() const;
	public:
		TLength(TUint16& aRecv, TUint16& aReq);
	private:
		TUint16& iRecv;
		TUint16& iReq;
		};
public:
	IMPORT_C TLength At(TInt aIndex);
	IMPORT_C const TLength At(TInt aIndex) const;
	IMPORT_C TLength operator[](TInt aIndex);
	IMPORT_C const TLength operator[](TInt aIndex) const;
	IMPORT_C TInt MaxNumPackets();

public:
	TPacketLengths(TUint16* aRecvPtr, TUint16* aReqPtr, TInt& aMaxNumPackets);

private:
	TUint16* iRecvPtr;
	TUint16* iReqPtr;
	TInt& iMaxNumPackets;
	};

/**
A class that refers to the list of packet results for a isochronous transfer
descriptor.

@publishedPartner
@prototype
*/
NONSHARABLE_CLASS(TPacketResults)
	{
public:
	IMPORT_C TInt At(TInt aIndex) const;
	IMPORT_C TInt operator[](TInt aIndex) const;
	IMPORT_C TInt MaxNumPackets();

public:
	TPacketResults(TInt* aResPtr, TInt& aMaxNumPackets);
	
private:
	TInt* iResPtr;
	TInt& iMaxNumPackets;
	};


/**
Provides *SEQUENTIAL* access to the packet slots in an isochronous transfer descriptor.
As some HCs may pack the buffer space tightly, with one packet starting immediately after the preceeding one,
random access is not possible -- in this implementation, even replacing the content of a slot with another packet
of the same size is not 'intentionally' possible.
Note that reading data is possible in a random access manner -- the sequential constraint only applies to writing.
@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(RUsbIsocTransferDescriptor) : public RUsbTransferDescriptor
	{
friend class RUsbTransferStrategy;

public:
    IMPORT_C RUsbIsocTransferDescriptor(TInt aMaxPacketSize, TInt aMaxNumPackets);

public:
	IMPORT_C void Reset();
	IMPORT_C TPacketLengths Lengths();
	IMPORT_C TPacketResults Results();
	IMPORT_C TInt MaxPacketSize();

public:		// Sending
	IMPORT_C TPtr8 WritablePackets(TInt aNumPacketsRequested, TInt& aMaxNumOfPacketsAbleToWrite);
	IMPORT_C void SaveMultiple(TInt aNumOfPackets);

public:		// Receiving
	IMPORT_C TPtrC8 Packets(TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumOfPacketsReturned) const;
	IMPORT_C void ReceivePackets(TInt aNumOfPackets);

private:
	/**
	The handle to represent the current point in writing an isoc. transfer.
	*/
	TInt iWriteHandle;
	};


/**
Provides buffer management for Bulk transfers
@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(RUsbBulkTransferDescriptor) : public RUsbTransferDescriptor
	{
public:
	IMPORT_C RUsbBulkTransferDescriptor(TInt aMaxSize);

public:		// Setters
	IMPORT_C TPtr8 WritableBuffer();
	IMPORT_C void SaveData(TInt aLength);
	IMPORT_C void SetZlpStatus(TZlpStatus aZlpStatus);

public:		// Getters
	IMPORT_C TPtrC8 Buffer() const;
	};



/**
Provides buffer management for Interrupt transfers
@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(RUsbIntrTransferDescriptor) : public RUsbTransferDescriptor
	{
public:
	IMPORT_C RUsbIntrTransferDescriptor(TInt aMaxSize);

public:		// Setters
	IMPORT_C TPtr8 WritableBuffer();
	IMPORT_C void SaveData(TInt aLength);
	IMPORT_C void SetZlpStatus(TZlpStatus aZlpStatus);

public:		// Getters
	IMPORT_C TPtrC8 Buffer() const;
	};

#endif // __KERNEL_MODE__

#endif	// __D32USBTRANSFERS_H
