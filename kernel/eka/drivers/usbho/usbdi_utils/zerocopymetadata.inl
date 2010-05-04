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


template<typename XReturnType, TInt XFieldOffset>
inline XReturnType& Field(TAddrType aBase, TInt aHeaderOffset)
	{
	TInt offset = aHeaderOffset + XFieldOffset;
	return *reinterpret_cast<XReturnType*>(aBase + offset);
	}


//
// UsbZeroCopyChunkHeaderBase
//

inline RUsbTransferDescriptor::TTransferType& UsbZeroCopyChunkHeaderBase::TransferType(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<RUsbTransferDescriptor::TTransferType, ETransferType>(aBase, aHeaderOffset);
	}



//
// UsbZeroCopyBulkIntrChunkHeader
//

inline TInt UsbZeroCopyBulkIntrChunkHeader::HeaderSize()
	{
	__ASSERT_COMPILE(EHeaderSize % sizeof(TInt) == 0);
	return EHeaderSize;
	}


inline TInt& UsbZeroCopyBulkIntrChunkHeader::DataOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EDataOffset>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyBulkIntrChunkHeader::DataLength(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EDataLength>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyBulkIntrChunkHeader::DataMaxLength(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EDataMaxLength>(aBase, aHeaderOffset);
	}
	
inline RUsbTransferDescriptor::TZlpStatus& UsbZeroCopyBulkIntrChunkHeader::ZlpStatus(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<RUsbTransferDescriptor::TZlpStatus, EZlpStatus>(aBase, aHeaderOffset);
	}



//
// UsbZeroCopyIsocChunkHeader
//

inline TInt UsbZeroCopyIsocChunkHeader::HeaderSize()
	{
	__ASSERT_COMPILE(EHeaderSize % sizeof(TInt) == 0);
	return EHeaderSize;
	}


inline TInt& UsbZeroCopyIsocChunkHeader::FirstElementOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EFirstElementOffset>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyIsocChunkHeader::MaxNumPackets(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EMaxNumPackets>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyIsocChunkHeader::MaxPacketSize(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EMaxPacketSize>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyIsocChunkHeader::LengthsOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, ELengthsOffset>(aBase, aHeaderOffset);
	}
	
inline TInt& UsbZeroCopyIsocChunkHeader::ReqLenOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EReqLenOffset>(aBase, aHeaderOffset);
	}
	
inline TInt& UsbZeroCopyIsocChunkHeader::ResultsOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EResultsOffset>(aBase, aHeaderOffset);
	}



//
// UsbZeroCopyIsocChunkHeader
//

inline TInt UsbZeroCopyIsocChunkElement::ElementSize()
	{
	__ASSERT_COMPILE(EElementSize % sizeof(TInt) == 0);
	return EElementSize;
	}


inline TInt& UsbZeroCopyIsocChunkElement::DataOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, EDataOffset>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyIsocChunkElement::NumPackets(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, ENumPackets>(aBase, aHeaderOffset);
	}

inline TInt& UsbZeroCopyIsocChunkElement::NextElementOffset(TAddrType aBase, TInt aHeaderOffset)
	{
	return Field<TInt, ENextElementOffset>(aBase, aHeaderOffset);
	}
	

