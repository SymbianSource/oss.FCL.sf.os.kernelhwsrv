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
// Description:
//

/**
 @file
 @internalComponent
*/

#ifndef ZEROCOPYMETADATA_H
#define ZEROCOPYMETADATA_H

#include <e32def.h>

// The type used to represent an address various betwen user and kernel
// mode.  To aid us we use a macro to produce only one definition.
#ifndef __KERNEL_MODE__
#define TAddrType TUint8*
#else // __KERNEL_MODE__
#define TAddrType TLinAddr
#endif // __KERNEL_MODE__


NONSHARABLE_CLASS(UsbZeroCopyChunkHeaderBase)
	{
public:
	static inline RUsbTransferDescriptor::TTransferType& TransferType(TAddrType aBase, TInt aHeaderOffset);
protected:
	enum THeaderBaseSizes
		{
		ETransferTypeSize = sizeof(RUsbTransferDescriptor::TTransferType)
		};
	enum THeaderBaseLayout
		{
		ETransferType	= 0,
		// End of fields
		EHeaderBaseSize	= ETransferType + ETransferTypeSize
		};
	};


NONSHARABLE_CLASS(UsbZeroCopyBulkIntrChunkHeader) : public UsbZeroCopyChunkHeaderBase
	{
public:
	static inline TInt HeaderSize();
	
	static inline TInt& DataOffset(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& DataLength(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& DataMaxLength(TAddrType aBase, TInt aHeaderOffset);
	static inline RUsbTransferDescriptor::TZlpStatus& ZlpStatus(TAddrType aBase, TInt aHeaderOffset);
private:
	enum THeaderSizes
		{
		EDataOffsetSize		= sizeof(TInt),
		EDataLengthSize		= sizeof(TInt),
		EDataMaxLengthSize	= sizeof(TInt),
		EZlpStatusSize		= sizeof(RUsbTransferDescriptor::TZlpStatus)
		};
	enum THeaderLayout
		{
		EDataOffset		= EHeaderBaseSize,
		EDataLength		= EDataOffset + EDataOffsetSize,
		EDataMaxLength	= EDataLength + EDataLengthSize,
		EZlpStatus		= EDataMaxLength + EDataMaxLengthSize,
		// End of fields
		EHeaderSize		= EZlpStatus + EZlpStatusSize
		};
	};


NONSHARABLE_CLASS(UsbZeroCopyIsocChunkHeader) : public UsbZeroCopyChunkHeaderBase
	{
public: // Lengths Array constants
	static const TInt KLengthsElementSize = sizeof(TUint16);
	static const TInt KResultsElementSize = sizeof(TInt);
public:
	static inline TInt HeaderSize();
	
	static inline TInt& FirstElementOffset(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& MaxNumPackets(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& MaxPacketSize(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& LengthsOffset(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& ReqLenOffset(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& ResultsOffset(TAddrType aBase, TInt aHeaderOffset);
private:
	enum THeaderSizes
		{
		EFirstElementOffsetSize	= sizeof(TInt),
		EMaxNumPacketsSize		= sizeof(TInt),
		EMaxPacketSizeSize		= sizeof(TInt),
		ELengthsOffsetSize		= sizeof(TInt),
		EReqLenOffsetSize		= sizeof(TInt),
		EResultsOffsetSize		= sizeof(TInt)
		};
	enum THeaderLayout
		{
		EFirstElementOffset	= EHeaderBaseSize,
		EMaxNumPackets		= EFirstElementOffset + EFirstElementOffsetSize,
		EMaxPacketSize		= EMaxNumPackets + EMaxNumPacketsSize,
		ELengthsOffset		= EMaxPacketSize + EMaxPacketSizeSize,
		EReqLenOffset		= ELengthsOffset + ELengthsOffsetSize,
		EResultsOffset		= EReqLenOffset + EReqLenOffsetSize,
		// End of fields
		EHeaderSize			= EResultsOffset + EResultsOffsetSize
		};
	};


NONSHARABLE_CLASS(UsbZeroCopyIsocChunkElement)
	{
public: 
	// NumOfPackets constants
	static const TInt KInvalidElement = -1;
	// NextElementOffset constants
	static const TInt KEndOfList = -1;
public:
	static inline TInt ElementSize();
	
	static inline TInt& DataOffset(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& NumPackets(TAddrType aBase, TInt aHeaderOffset);
	static inline TInt& NextElementOffset(TAddrType aBase, TInt aHeaderOffset);
private:
	enum THeaderSizes
		{
		EDataOffsetSize			= sizeof(TInt),
		ENumPacketsSize			= sizeof(TInt),
		ENextElementOffsetSize	= sizeof(TInt),
		};
	enum THeaderLayout
		{
		EDataOffset			= 0,
		ENumPackets			= EDataOffset + EDataOffsetSize,
		ENextElementOffset	= ENumPackets + ENumPacketsSize,
		// End of fields
		EElementSize		= ENextElementOffset + ENextElementOffsetSize
		};
	};

#include "zerocopymetadata.inl"

#undef TAddrType // Prevent the macro from leaking outside this header

#endif // ZEROCOPYMETADATA_H
