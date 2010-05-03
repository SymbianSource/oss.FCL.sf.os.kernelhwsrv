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

#include "zerocopytransferstrategy.h"

#include <d32usbtransfers.h>
#include <d32usbdi.h>
#include <d32usbdi_errors.h>
#include "zerocopymetadata.h"
#include "usbdiutils.h"


RUsbZeroCopyTransferStrategy::TUsbTransferDescriptorDetails::TUsbTransferDescriptorDetails(RUsbTransferDescriptor& aTransferDesc, TInt aRequiredSize, TUint aRequiredAlignment, TInt aRequiredMaxPackets)
	: iTransferDesc(aTransferDesc)
	, iRequiredSize(aRequiredSize)
	, iRequiredAlignment(aRequiredAlignment)
	, iRequiredMaxPackets(aRequiredMaxPackets)
	{
	}
	
RUsbZeroCopyTransferStrategy::RUsbZeroCopyTransferStrategy()
	: iInterfaceHandle(NULL)
	{
	}


void RUsbZeroCopyTransferStrategy::Close()
	{
	iInterfaceHandle = NULL;
	iChunk.Close();
	iRegisteredTransfers.Close();
	RUsbTransferStrategy::Close();
	}


TInt RUsbZeroCopyTransferStrategy::RegisterTransferDescriptor(RUsbTransferDescriptor& aTransferDesc, TInt aRequiredSize, TUint aStartAlignment, TInt aRequiredMaxPackets)
	{
	__ASSERT_ALWAYS(!iInterfaceHandle, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorsAlreadyRegistered));
	if (iRegisteredTransfers.Find(aTransferDesc, CompareTransferDescriptor) != KErrNotFound)
		{
		return KErrAlreadyExists;
		}
	return iRegisteredTransfers.Append(TUsbTransferDescriptorDetails(aTransferDesc, aRequiredSize, aStartAlignment, aRequiredMaxPackets));
	}

TBool RUsbZeroCopyTransferStrategy::CompareTransferDescriptor(const RUsbTransferDescriptor* aTransferDesc, const TUsbTransferDescriptorDetails& aDetails)
	{
	return aTransferDesc == &aDetails.iTransferDesc;
	}


void RUsbZeroCopyTransferStrategy::ResetTransferDescriptors()
	{
	__ASSERT_ALWAYS(!iInterfaceHandle, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorsAlreadyRegistered));
	iRegisteredTransfers.Reset();
	}


TInt RUsbZeroCopyTransferStrategy::InitialiseTransferDescriptors(RUsbInterface& aInterface)
	{
	__ASSERT_ALWAYS(!iInterfaceHandle, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorsAlreadyRegistered));

	// This is the equivilent of a standard R-class Open() method, so initialise the references
	// we are going to use.
	iInterfaceHandle = &aInterface;

	// First get the page-size as we will need this for isoc transfer calculations.
	TInt hcdPageSize = 0;
	TInt err = aInterface.GetHcdPageSize(hcdPageSize);
	if (err != KErrNone)
		{
		Close(); // roll back
		return err;
		}
	iPageSize = hcdPageSize;

	TInt currentOffset = 0;
	TInt numStandardTransfers = 0;
	TInt numIsocTransfers = 0;
	TInt numIsocElements = 0;
	err = CalculateDataLayout(currentOffset, numStandardTransfers, numIsocTransfers, numIsocElements);
	if (err != KErrNone)
		{
		Close(); // roll back
		return err;
		}

	TInt metaDataStart = 0;
	CalculateMetaDataLayout(currentOffset, metaDataStart, numStandardTransfers, numIsocTransfers, numIsocElements);

	// currentOffset should now be just past the region required for all the data and meta data.
	// Therefore it equals the total size of the buffer we need to hold them all.
	err = iInterfaceHandle->AllocateSharedChunk(iChunk, currentOffset, iBaseOffset);
	if (err != KErrNone)
		{
		Close(); // roll back
		return err;
		}

	InitialiseMetaData(metaDataStart, numStandardTransfers, numIsocTransfers, numIsocElements);

	return KErrNone;
	}

TInt RUsbZeroCopyTransferStrategy::CalculateDataLayout(TInt& aCurrentOffset, TInt& aNumStandardTransfers, TInt& aNumIsocTransfers, TInt& aNumIsocElements)
	{
	const TUint32 pageAddrBits = iPageSize-1;
	const TUint32 pageTableMask = ~pageAddrBits;

	//Get the maximum wMaxPacketSize of the associated interface for Bulk/Interrupt EPs
	TInt maxMaxBulk = 0;
	TInt maxMaxInterrupt = 0;
	TInt err = GetMaximumMaxPacketSize(maxMaxBulk, maxMaxInterrupt);
	if (err != KErrNone)
		{
		return err;
		}
	
	// Work out where to place the transfers, and how much space is needed.
	TInt numTransfers = iRegisteredTransfers.Count();
	for (TInt i=0; i < numTransfers; ++i)
		{
		TUsbTransferDescriptorDetails& details = iRegisteredTransfers[i];
		
		err = CaculateAdditionalAlignment(aCurrentOffset, maxMaxBulk, maxMaxInterrupt, details);
		if (err != KErrNone)
			{
			return err;
			}
		
		// only allow intra-page alignment requests that are powers of 2 (so offset agnostic).
		__ASSERT_ALWAYS(details.iRequiredAlignment <= iPageSize, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorAlignmentOverPageBoundary));
		__ASSERT_ALWAYS(IsPowerOfTwo(details.iRequiredAlignment), UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorAlignmentNotPowerOfTwo));

		TInt alignPad = IncNeededToAlign(aCurrentOffset, details.iRequiredAlignment);
		__ASSERT_DEBUG(alignPad < iPageSize, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadAlignment)); // just re-asserting what should be guarded above
		aCurrentOffset += alignPad; // Align to the start of transfer buffer

		// There are stark differences between isoc transfers and transfer of other types.
		if (details.iTransferDesc.iType == RUsbTransferDescriptor::EIsochronous)
			{
			// First do some Isoc specific checks
			__ASSERT_ALWAYS(details.iRequiredMaxPackets > 0, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorNoPacketsRequested));

			// For the allocation we have to consider the worse case - that is that the max
			// number of packets at the max packet size.
			// We are constrained by the USB stack to not allow transfers across page boundaries.

			// As such we calculate how many packets we can fit into a page to determine the
			// number of pages for data we need.
			const TInt packetsPerPage = iPageSize/details.iRequiredSize;

			// Assign the start to an appropriate point.
			details.iAssignedOffset = aCurrentOffset;
			TInt packetsToStore = details.iRequiredMaxPackets;
			TInt numElements = 0; // for counting up the number of pages we need meta-data for.
			
			// The size requried to hold a length array for the descriptor
			const TInt lengthsArrayLength = UsbZeroCopyIsocChunkHeader::KLengthsElementSize * details.iRequiredMaxPackets;
			// The size required to hold a result array for the descriptor
			const TInt resultsArrayLength = UsbZeroCopyIsocChunkHeader::KResultsElementSize * details.iRequiredMaxPackets;

			// Determine how much we can fit into the remaining space of the current page.
			TBool samePage = (pageTableMask & aCurrentOffset) == (pageTableMask & (aCurrentOffset - alignPad));
			if (samePage)
				{
				TInt remainingSpace = iPageSize - (pageAddrBits & aCurrentOffset);
				TInt packetsThatFit = remainingSpace / details.iRequiredSize;
				if (packetsThatFit >= packetsToStore)
					{
					// We can fit it in this page so we finish here - this is the special case.
					aCurrentOffset += packetsToStore * details.iRequiredSize;
					++aNumIsocElements;
					++aNumIsocTransfers;
					details.iNumElements = 1;
					// Do the lengths array
					aCurrentOffset += IncNeededToAlign(aCurrentOffset, UsbZeroCopyIsocChunkHeader::KLengthsElementSize);
					details.iLengthsOffset = aCurrentOffset;
					aCurrentOffset += lengthsArrayLength;
					// The dual lengths array should be implicitly alligned
					details.iReqLenOffset = aCurrentOffset;
					aCurrentOffset += lengthsArrayLength;
					// Now handle the results array
					aCurrentOffset += IncNeededToAlign(aCurrentOffset, UsbZeroCopyIsocChunkHeader::KResultsElementSize);
					details.iResultsOffset = aCurrentOffset;
					aCurrentOffset += resultsArrayLength;
					continue;
					}
				aCurrentOffset = (pageTableMask & aCurrentOffset) + iPageSize; // Advance to next page
				packetsToStore -= packetsThatFit;
				++numElements;
				}
			__ASSERT_DEBUG(packetsToStore > 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorNoPacketsLeftToStore));

			// Determine the number of pages extra that are needed (minus one)
			TInt pagesRequired = packetsToStore / packetsPerPage;

			// Determine how much of the last page is actually needed.
			TInt trailingPackets = packetsToStore % packetsPerPage;
			TInt usedSpace = trailingPackets * details.iRequiredSize;

			// Commit the amount for the buffers.
			aCurrentOffset += usedSpace + pagesRequired*iPageSize;
			numElements += pagesRequired + /*the final page*/1; // We have already included the first page (if already partially used)
			aNumIsocElements += numElements;
			++aNumIsocTransfers;

			// Used to ensure only allocate an appropriate number per-descriptor.
			details.iNumElements = numElements;

			// We also need an array of lengths for each packet that we use (need to align to even bytes).
			aCurrentOffset += IncNeededToAlign(aCurrentOffset, UsbZeroCopyIsocChunkHeader::KLengthsElementSize);
			details.iLengthsOffset = aCurrentOffset;
			aCurrentOffset += lengthsArrayLength;
			// Dual length array should be implicitly aligned
			details.iReqLenOffset = aCurrentOffset;
			aCurrentOffset += lengthsArrayLength;
			// Now handle the results array
			aCurrentOffset += IncNeededToAlign(aCurrentOffset, UsbZeroCopyIsocChunkHeader::KResultsElementSize);
			details.iResultsOffset = aCurrentOffset;
			aCurrentOffset += resultsArrayLength;
			}
		else
			{
			details.iAssignedOffset = aCurrentOffset;
			aCurrentOffset += details.iRequiredSize;
			++aNumStandardTransfers;
			}
		}
	
	return KErrNone;
	}


void RUsbZeroCopyTransferStrategy::CalculateMetaDataLayout(TInt& aCurrentOffset, TInt& aMetaDataStart, TInt aNumStandardTransfers, TInt aNumIsocTransfers, TInt aNumIsocElements)
	{
	// Round up to 4 byte alignment for handling the meta-data correctly.
	aCurrentOffset += IncNeededToAlign(aCurrentOffset, sizeof(TInt));

	aMetaDataStart = aCurrentOffset;

	// Now calculate the size required for the transfer meta-data.
	aCurrentOffset += aNumStandardTransfers * UsbZeroCopyBulkIntrChunkHeader::HeaderSize();
	aCurrentOffset += aNumIsocTransfers * UsbZeroCopyIsocChunkHeader::HeaderSize();
	aCurrentOffset += aNumIsocElements * UsbZeroCopyIsocChunkElement::ElementSize();
	}
	
void RUsbZeroCopyTransferStrategy::InitialiseMetaData(TInt aMetaDataOffset, TInt aNumStandardTransfers, TInt aNumIsocTransfers, TInt aNumIsocElements)
	{
	const TUint32 pageAddrBits = iPageSize-1;
	const TUint32 pageTableMask = ~pageAddrBits;

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	TInt numTransfers = iRegisteredTransfers.Count();
	for (TInt i=0; i < numTransfers; ++i)
		{
		TUsbTransferDescriptorDetails details = iRegisteredTransfers[i];

		if (details.iTransferDesc.iType == RUsbTransferDescriptor::EIsochronous)
			{
			// Initialise Meta-data (minus elements).
			UsbZeroCopyIsocChunkHeader::TransferType(chunkBase, aMetaDataOffset) = details.iTransferDesc.iType;
			UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aMetaDataOffset) = details.iRequiredMaxPackets;
			UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aMetaDataOffset) = details.iRequiredSize;
			// Double check that the length array is aligned correctly.
			__ASSERT_DEBUG(details.iLengthsOffset % UsbZeroCopyIsocChunkHeader::KLengthsElementSize == 0,
				UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorLengthsArrayBadAlignment));
			UsbZeroCopyIsocChunkHeader::LengthsOffset(chunkBase, aMetaDataOffset) = details.iLengthsOffset;
			UsbZeroCopyIsocChunkHeader::ReqLenOffset(chunkBase, aMetaDataOffset) = details.iReqLenOffset;
			// Double check that the result array is aligned correctly.
			__ASSERT_DEBUG(details.iResultsOffset % UsbZeroCopyIsocChunkHeader::KResultsElementSize == 0,
				UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorResultsArrayBadAlignment));
			UsbZeroCopyIsocChunkHeader::ResultsOffset(chunkBase, aMetaDataOffset) = details.iResultsOffset;
			// Initialise transfer descriptor
			SetTransferHandle(details.iTransferDesc, aMetaDataOffset);
			// Move on to next meta-data slot
			TInt prevMetaOffset = aMetaDataOffset;
			aMetaDataOffset += UsbZeroCopyIsocChunkHeader::HeaderSize();

			// Initialise elements for transfers
			UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, prevMetaOffset) = aMetaDataOffset;
			
			TInt isocElementsUnmapped = details.iNumElements;
			// First element could be anywhere, the others are at the start of (virtually) contiguous pages
			TInt offset = details.iAssignedOffset;
			while (isocElementsUnmapped > 0)
				{
				// Update the data references
				UsbZeroCopyIsocChunkElement::DataOffset(chunkBase, aMetaDataOffset) = offset;
				UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, aMetaDataOffset) = 0; // Default value.
				// Move on to the next element and bind it to the chain.
				prevMetaOffset = aMetaDataOffset;
				aMetaDataOffset += UsbZeroCopyIsocChunkElement::ElementSize();
				UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, prevMetaOffset) = aMetaDataOffset;
				// Move to the next page
				offset = (pageTableMask&offset)+iPageSize;
				--isocElementsUnmapped;
				--aNumIsocElements;
				}
			// We have reached the end of the list so we should update the next element offset for the
			// last element to indicate that it is the terminator.
			UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, prevMetaOffset) = UsbZeroCopyIsocChunkElement::KEndOfList;
			--aNumIsocTransfers;
			}
		else
			{
			// Initialise Meta-data.
			UsbZeroCopyBulkIntrChunkHeader::TransferType(chunkBase, aMetaDataOffset) = details.iTransferDesc.iType;
			UsbZeroCopyBulkIntrChunkHeader::DataOffset(chunkBase, aMetaDataOffset) = details.iAssignedOffset;
			UsbZeroCopyBulkIntrChunkHeader::DataLength(chunkBase, aMetaDataOffset) = 0;
			UsbZeroCopyBulkIntrChunkHeader::DataMaxLength(chunkBase, aMetaDataOffset) = details.iRequiredSize;
			UsbZeroCopyBulkIntrChunkHeader::ZlpStatus(chunkBase, aMetaDataOffset) = RUsbTransferDescriptor::ESendZlpIfRequired;
			// Initialise transfer descriptor
			SetTransferHandle(details.iTransferDesc, aMetaDataOffset);
			// Move on to next meta-data slot
			aMetaDataOffset += UsbZeroCopyBulkIntrChunkHeader::HeaderSize();
			--aNumStandardTransfers;
			}
		}

	__ASSERT_DEBUG(aNumStandardTransfers == 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorIncompleteInitialisation));
	__ASSERT_DEBUG(aNumIsocTransfers == 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorIncompleteInitialisation));
	__ASSERT_DEBUG(aNumIsocElements == 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorIncompleteInitialisation));
	}


TBool RUsbZeroCopyTransferStrategy::IsPowerOfTwo(TUint aNumber)
	{
    return aNumber && !(aNumber & (aNumber - 1)); //this returns true if the integer is a power of two
    }


TInt RUsbZeroCopyTransferStrategy::IncNeededToAlign(TInt aOffset, TUint aAlignment)
	{
	if (aAlignment == 0)
		{
		return 0;
		}
	TInt remain = aOffset % aAlignment;
	return (aAlignment - remain) % aAlignment;
	}


// Standard Methods

TPtr8 RUsbZeroCopyTransferStrategy::WritableBuffer(TInt aHandle)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;
	
	TUint8* dataPtr = chunkBase + UsbZeroCopyBulkIntrChunkHeader::DataOffset(chunkBase, aHandle);
	TInt maxLength = UsbZeroCopyBulkIntrChunkHeader::DataMaxLength(chunkBase, aHandle);

	return TPtr8(dataPtr, 0, maxLength);
	}

void RUsbZeroCopyTransferStrategy::SaveData(TInt aHandle, TInt aLength)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	TInt maxLength = UsbZeroCopyBulkIntrChunkHeader::DataMaxLength(chunkBase, aHandle);
	__ASSERT_ALWAYS(aLength <= maxLength, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorSavedToMuchData));

	UsbZeroCopyBulkIntrChunkHeader::DataLength(chunkBase, aHandle) = aLength;
	}
	
void RUsbZeroCopyTransferStrategy::SetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	UsbZeroCopyBulkIntrChunkHeader::ZlpStatus(chunkBase, aHandle) = aZlpStatus;
	}

TPtrC8 RUsbZeroCopyTransferStrategy::Buffer(TInt aHandle) const
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;
	
	TUint8* dataPtr = chunkBase + UsbZeroCopyBulkIntrChunkHeader::DataOffset(chunkBase, aHandle);
	TInt length = UsbZeroCopyBulkIntrChunkHeader::DataLength(chunkBase, aHandle);

	return TPtrC8(dataPtr, length);
	}
	



// Isochronous Methods
	
void RUsbZeroCopyTransferStrategy::Reset(TInt aHandle)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	// Loop through and reset number of packets in each element as 0
	TInt elementOffset = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHandle);
	while (elementOffset != UsbZeroCopyIsocChunkElement::KEndOfList)
		{
		UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, elementOffset) = 0;
		elementOffset = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, elementOffset);
		}
	}

TPacketLengths RUsbZeroCopyTransferStrategy::Lengths(TInt aHandle)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	TInt lengthsOffset = UsbZeroCopyIsocChunkHeader::LengthsOffset(chunkBase, aHandle);
	TUint16* lengthsPtr = reinterpret_cast<TUint16*>(chunkBase + lengthsOffset);
	
	TInt reqLenOffset = UsbZeroCopyIsocChunkHeader::ReqLenOffset(chunkBase, aHandle);
	TUint16* reqLenPtr = reinterpret_cast<TUint16*>(chunkBase + reqLenOffset);

	TInt& maxNumPackets = UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle);

	return TPacketLengths(lengthsPtr, reqLenPtr, maxNumPackets);
	}
	
TPacketResults RUsbZeroCopyTransferStrategy::Results(TInt aHandle)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	TInt resultsOffset = UsbZeroCopyIsocChunkHeader::ResultsOffset(chunkBase, aHandle);
	TInt* resultsPtr = reinterpret_cast<TInt*>(chunkBase + resultsOffset);

	TInt& maxNumPackets = UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle);

	return TPacketResults(resultsPtr, maxNumPackets);
	}

TInt RUsbZeroCopyTransferStrategy::MaxPacketSize(TInt aHandle)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	TInt maxPacketSize = UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aHandle);
	
	return maxPacketSize;
	}

TPtr8 RUsbZeroCopyTransferStrategy::WritablePackets(TInt aHandle, TInt aWriteHandle, TInt aNumPacketsRequested, TInt& aMaxNumPacketsAbleToWrite)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));
	__ASSERT_DEBUG(aWriteHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadWriteHandle));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;
	
	const TUint32 pageAddrBits = iPageSize-1;
	const TUint32 pageTableMask = ~pageAddrBits;
	
	if (aHandle == aWriteHandle)
		{
		// The initial write handle will be the same as the standard handle so we need to find the actual 
		// element to work correctly.
		aWriteHandle = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHandle);
		}

	// Now we have two cases - the number of packets requested is contained in one page, or it crosses the page.
	// 1) If we cross the page then we get the buffer for upto the end of the page, and inform the user of the number
	// of packets they are able to write into it (normally this will be quite high as we can consider 0 length
	// packets.)
	// 2) If we are on one page then we provide a buffer to the end of the page and return the number of packets
	// the requested as the max they can write.  However we also now mark it so that an attempt to get a subsequent
	// writable buffer will return a 0 max length TPtr8 and 0 max number of packets to write.  If they want write
	// more they need to reset the descriptor and start again.

	if (UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, aWriteHandle) == UsbZeroCopyIsocChunkElement::KInvalidElement)
		{
		// Here we are testing the second case, if we previously marked an element as invalid then we must not
		// return a valid buffer.
		aMaxNumPacketsAbleToWrite = 0;
		return TPtr8(NULL, 0);
		}

	TInt dataOffset = UsbZeroCopyIsocChunkElement::DataOffset(chunkBase, aWriteHandle);
	
	TUint8* dataPtr = chunkBase + dataOffset;
	TInt totalMaxSize = aNumPacketsRequested * UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aHandle);
	// The USB stack requires isoc transfer to be limited to a page (not allowed to cross the boundary).
	TUint32 dataAddr = reinterpret_cast<TUint32>(dataPtr);
	TBool samePage = (pageTableMask & dataAddr) == (pageTableMask & (dataAddr + totalMaxSize));
	TInt allowableSize = samePage ? totalMaxSize : iPageSize - (pageAddrBits & dataAddr);

	TInt numPacketsRemaining = UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle) - UsedPackets(aHandle);

	if (aNumPacketsRequested < numPacketsRemaining)
		{
		// This is the 2nd case as documented in the comment.  So we mark the next packet as invalid.
		aMaxNumPacketsAbleToWrite = aNumPacketsRequested;
		TInt nextElement = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, aWriteHandle);
		if (nextElement != UsbZeroCopyIsocChunkElement::KEndOfList)
			{
			UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, nextElement) = UsbZeroCopyIsocChunkElement::KInvalidElement; // Mark as invalid.
			}
		// else we are at the end of the list anyway
		}
	else
		{
		aMaxNumPacketsAbleToWrite = numPacketsRemaining;
		}

	return TPtr8(dataPtr, allowableSize);
	}

TInt RUsbZeroCopyTransferStrategy::SaveMultiple(TInt aHandle, TInt aWriteHandle, TInt aNumPackets)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));
	__ASSERT_DEBUG(aWriteHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadWriteHandle));
	__ASSERT_ALWAYS(aNumPackets > 0, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorNoPacketsToSave));
	
	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	if (aHandle == aWriteHandle)
		{
		aWriteHandle = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHandle);
		}

	// if marked invalid then they shouldn't try to save it (they haven't been able to write anything into the data anyway).
	__ASSERT_ALWAYS(UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, aWriteHandle) != UsbZeroCopyIsocChunkElement::KInvalidElement,
		UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorInvalidSaveCall));

	// Ensure they've not tried to write in too many packets
	TInt usedPackets = UsedPackets(aHandle);
	__ASSERT_ALWAYS(aNumPackets + usedPackets <= UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle),
		UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorSavedTooManyPackets));

	// Check that the length values have not exceeded the maximum.
	TInt maxPacketSize = UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aHandle);
	TInt lengthsOffset = UsbZeroCopyIsocChunkHeader::LengthsOffset(chunkBase, aHandle);
	TUint16* lengthsPtr = reinterpret_cast<TUint16*>(chunkBase + lengthsOffset);
#ifdef _DEBUG
	// The requested length is only functionally needed for IN transfers, but it provides an
	// extra check that the length values that were requested by the user are those that are
	// been requested on the USB stack.
	TInt reqLenOffset = UsbZeroCopyIsocChunkHeader::ReqLenOffset(chunkBase, aHandle);
	TUint16* reqLenPtr = reinterpret_cast<TUint16*>(chunkBase + reqLenOffset);
#endif // _DEBUG
	for (TInt i=0; i < aNumPackets; ++i)
		{
		__ASSERT_ALWAYS(lengthsPtr[usedPackets + i] <= maxPacketSize, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorSavingTooLargeAPacket));
		__ASSERT_DEBUG(lengthsPtr[usedPackets + i] == reqLenPtr[usedPackets + i], UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorRequestedLengthDiffers)); // Belt 'n' Braces
		}

	// Commit the packets to the transfer descriptor.
	UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, aWriteHandle) = aNumPackets;
	TInt headerOffset = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, aWriteHandle);
	
	// Return the handle to the next region for writing.
	return (headerOffset == UsbZeroCopyIsocChunkElement::KEndOfList) ? KErrEof : headerOffset;
	}

/**
Used to walk the elements to total up the number of packets that have been saved in the transfer descriptor.
*/
TInt RUsbZeroCopyTransferStrategy::UsedPackets(TInt aHeaderOffset)
	{
	__ASSERT_DEBUG(aHeaderOffset >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorInvalidHeaderOffset));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;
	TInt elementOffset = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHeaderOffset);
	TInt totalNumPackets = 0;
	while (elementOffset != UsbZeroCopyIsocChunkElement::KEndOfList)
		{
		TInt numPackets = UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, elementOffset);
		if (numPackets == 0 || numPackets == UsbZeroCopyIsocChunkElement::KInvalidElement)
			{
			break;
			}
		totalNumPackets += numPackets;
		elementOffset = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, elementOffset);
		}
	return totalNumPackets;
	}

/**
Used to read packets out from the transfer descriptor.
Note that some of the panics are belt'n'braces, and are used to sanity test result that has been
provided.  These should be correct (as the results are set by the kernel), however because the user
has access to length array (for writing out packets) it is possible for them to 'corrupt' the result.
We panic explicitly in UDEB builds, in UREL the guards are not present and the user may get returned
a bad descriptor.
*/
TPtrC8 RUsbZeroCopyTransferStrategy::Packets(TInt aHandle, TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumPacketsReturned) const
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));
	__ASSERT_ALWAYS(aFirstPacketIndex >= 0, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorPacketNotInBounds));
	__ASSERT_ALWAYS(aNumPacketsRequested > 0, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorTooFewPacketsRequested));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;

	__ASSERT_ALWAYS(aNumPacketsRequested <= UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle),
		UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorTooManyPacketsRequested));

#ifdef _DEBUG
	const TUint32 pageAddrBits = iPageSize-1;
	const TUint32 pageTableMask = ~pageAddrBits;
#endif // _DEBUG
	const TInt maxPacketSize = UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aHandle);

	TInt elementOffset = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHandle);
	TInt packetCount = 0;
	while (elementOffset != UsbZeroCopyIsocChunkElement::KEndOfList)
		{
		TInt numPackets = UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, elementOffset);
		if (numPackets == 0 || numPackets == UsbZeroCopyIsocChunkElement::KInvalidElement)
			{
			// We've got to the end of the elements and not found the packets we are after.
			break;
			}
		TInt previousPacketCount = packetCount;
		packetCount += numPackets;
		if (aFirstPacketIndex < packetCount) // If true then start packet must be in this element
			{
			TInt intraElementIndex = aFirstPacketIndex - previousPacketCount;
			TInt maxPacketsForReturn = packetCount - aFirstPacketIndex;

			TInt lengthsOffset = UsbZeroCopyIsocChunkHeader::LengthsOffset(chunkBase, aHandle);
			TUint16* lengthsPtr = reinterpret_cast<TUint16*>(chunkBase + lengthsOffset + previousPacketCount * sizeof(TUint16));
			TInt reqLenOffset = UsbZeroCopyIsocChunkHeader::ReqLenOffset(chunkBase, aHandle);
			TUint16* reqLenPtr = reinterpret_cast<TUint16*>(chunkBase + reqLenOffset + previousPacketCount * sizeof(TUint16));

			aNumPacketsReturned = (aNumPacketsRequested < maxPacketsForReturn) ? aNumPacketsRequested : maxPacketsForReturn;

			TInt distanceToReqPacket = 0;
			for (TInt i=0; i < intraElementIndex; ++i)
				{
				TUint16 reqLen = reqLenPtr[i];
				__ASSERT_DEBUG(reqLen <= maxPacketSize,
					UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorReceivedTooLargeAPacket)); // Belt'n'Braces
				distanceToReqPacket += reqLen;
				}
			TInt dataOffset = UsbZeroCopyIsocChunkElement::DataOffset(chunkBase, elementOffset);
			TUint8* dataPtr = chunkBase + dataOffset + distanceToReqPacket;

			TInt totalLengthPackets = 0;
			for (TInt i=0; i < aNumPacketsReturned; ++i)
				{
				TUint16 len = lengthsPtr[intraElementIndex + i];
				TUint16 reqLen = reqLenPtr[intraElementIndex + i];
				__ASSERT_DEBUG(len <= maxPacketSize,
					UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorReceivedTooLargeAPacket)); // Belt'n'Braces

				totalLengthPackets += len;
				
				// Here we handle the potential gaps that may appear in the data stream if a short
				// packet is received.
				if (len < reqLen)
					{
					// if here then we received a short packet, as such we can only return up to here
					aNumPacketsReturned = i+1;
					break;
					}
				// Otherwise we expect them to be equal (if we got more than requested then something odd has happened.
				__ASSERT_DEBUG(len == reqLen, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorRequestedLengthDiffers)); // Belt 'n' Braces
				}

			// The USB stack requires isoc transfer to be limited to a page (not allowed to cross the boundary).
			// Therefore one of our elements must have data only on one page.
#ifdef _DEBUG
			TUint32 dataAddr = reinterpret_cast<TUint32>(dataPtr);
			TBool samePage = (totalLengthPackets == 0) || (pageTableMask & dataAddr) == (pageTableMask & (dataAddr + totalLengthPackets - 1));
			__ASSERT_DEBUG(samePage, UsbdiUtils::Panic(UsbdiPanics::EIsocTransferResultCrossesPageBoundary)); // Belt'n'Braces
#endif // _DEBUG

			return TPtrC8(dataPtr, totalLengthPackets);
			}
		
		// No luck so far, move on to try the next element
		elementOffset = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, elementOffset);
		}

	// No suitable packet range found.
	aNumPacketsReturned = 0;
	return TPtrC8(NULL, 0);
	}

void RUsbZeroCopyTransferStrategy::ReceivePackets(TInt aHandle, TInt aNumPackets)
	{
	__ASSERT_DEBUG(aHandle >= 0, UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorBadHandle));
	__ASSERT_ALWAYS(aNumPackets > 0, UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorTooFewPacketsRequested));

	TUint8* chunkBase = iChunk.Base() + iBaseOffset;
	
	__ASSERT_ALWAYS(aNumPackets <= UsbZeroCopyIsocChunkHeader::MaxNumPackets(chunkBase, aHandle),
		UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorTooManyPacketsRequested));

	const TUint32 pageAddrBits = iPageSize-1;
	const TUint32 pageTableMask = ~pageAddrBits;
	const TInt maxPacketSize = UsbZeroCopyIsocChunkHeader::MaxPacketSize(chunkBase, aHandle);

#ifdef _DEBUG
	// Here we make the best check we can that the user has set-up the requested lengths they require.
	// If there is a difference, they have either a corrupted metadata chunk, or they are reusing a 
	// previous buffer without setting the lengths requested.
	TInt lengthsOffset = UsbZeroCopyIsocChunkHeader::LengthsOffset(chunkBase, aHandle);
	TUint16* lengthsPtr = reinterpret_cast<TUint16*>(chunkBase + lengthsOffset);
	TInt reqLenOffset = UsbZeroCopyIsocChunkHeader::ReqLenOffset(chunkBase, aHandle);
	TUint16* reqLenPtr = reinterpret_cast<TUint16*>(chunkBase + reqLenOffset);
	for (TInt i=0; i < aNumPackets; ++i)
		{
		__ASSERT_DEBUG(lengthsPtr[i] == reqLenPtr[i],
			UsbdiUtils::Panic(UsbdiPanics::ETransferDescriptorRequestedLengthDiffers)); // Belt 'n' Braces
		}
#endif // _DEBUG

	TInt elementOffset = UsbZeroCopyIsocChunkHeader::FirstElementOffset(chunkBase, aHandle);
	while (aNumPackets)
		{
		__ASSERT_DEBUG(elementOffset != UsbZeroCopyIsocChunkElement::KEndOfList,
			UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorUnexpectedEndOfIsocList));

		TInt totalMaxSize = aNumPackets * maxPacketSize;

		TInt dataOffset = UsbZeroCopyIsocChunkElement::DataOffset(chunkBase, elementOffset);
		TUint8* dataPtr = chunkBase + dataOffset;
		TUint32 dataAddr = reinterpret_cast<TUint32>(dataPtr);
		TBool samePage = (pageTableMask & dataAddr) == (pageTableMask & (dataAddr + totalMaxSize));
		TInt allowableSize = samePage ? totalMaxSize : iPageSize - (pageAddrBits & dataAddr);
		TInt numPackets = allowableSize / maxPacketSize;

		// TODO We could assert here in debug as a double check using UsedPackets()

		__ASSERT_DEBUG(numPackets > 0,
			UsbdiUtils::Fault(UsbdiFaults::EUsbTransferDescriptorUnfillableElement));

		UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, elementOffset) = numPackets;
		aNumPackets -= numPackets;

		elementOffset = UsbZeroCopyIsocChunkElement::NextElementOffset(chunkBase, elementOffset);
		}

	if (elementOffset != UsbZeroCopyIsocChunkElement::KEndOfList)
		{
		UsbZeroCopyIsocChunkElement::NumPackets(chunkBase, elementOffset) = UsbZeroCopyIsocChunkElement::KInvalidElement; // Mark as invalid.
		}
	}
	
	



TPtr8 RUsbZeroCopyTransferStrategy::IntrWritableBuffer(TInt aHandle)
	{
	return WritableBuffer(aHandle);
	}

void RUsbZeroCopyTransferStrategy::IntrSaveData(TInt aHandle, TInt aLength)
	{
	SaveData(aHandle, aLength);
	}

void RUsbZeroCopyTransferStrategy::IntrSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus)
	{
	SetZlpStatus(aHandle, aZlpStatus);
	}

TPtrC8 RUsbZeroCopyTransferStrategy::IntrBuffer(TInt aHandle) const
	{
	return Buffer(aHandle);
	}

TPtr8 RUsbZeroCopyTransferStrategy::BulkWritableBuffer(TInt aHandle)
	{
	return WritableBuffer(aHandle);
	}

void RUsbZeroCopyTransferStrategy::BulkSaveData(TInt aHandle, TInt aLength)
	{
	SaveData(aHandle, aLength);
	}

void RUsbZeroCopyTransferStrategy::BulkSetZlpStatus(TInt aHandle, RUsbTransferDescriptor::TZlpStatus aZlpStatus)
	{
	SetZlpStatus(aHandle, aZlpStatus);
	}

TPtrC8 RUsbZeroCopyTransferStrategy::BulkBuffer(TInt aHandle) const
	{
	return Buffer(aHandle);
	}

void RUsbZeroCopyTransferStrategy::IsocReset(TInt aHandle)
	{
	Reset(aHandle);
	}

TPacketLengths RUsbZeroCopyTransferStrategy::IsocLengths(TInt aHandle)
	{
	return Lengths(aHandle);
	}
	
TPacketResults RUsbZeroCopyTransferStrategy::IsocResults(TInt aHandle)
	{
	return Results(aHandle);
	}

TInt RUsbZeroCopyTransferStrategy::IsocMaxPacketSize(TInt aHandle)
	{
	return MaxPacketSize(aHandle);
	}

TPtr8 RUsbZeroCopyTransferStrategy::IsocWritablePackets(TInt aHandle, TInt aWriteHandle, TInt aNumPacketsRequested, TInt& aMaxNumPacketsAbleToWrite)
	{
	return WritablePackets(aHandle, aWriteHandle, aNumPacketsRequested, aMaxNumPacketsAbleToWrite);
	}

TInt RUsbZeroCopyTransferStrategy::IsocSaveMultiple(TInt aHandle, TInt aWriteHandle, TInt aNumOfPackets)
	{
	return SaveMultiple(aHandle, aWriteHandle, aNumOfPackets);
	}

TPtrC8 RUsbZeroCopyTransferStrategy::IsocPackets(TInt aHandle, TInt aFirstPacketIndex, TInt aNumPacketsRequested, TInt& aNumPacketsReturned) const
	{
	return Packets(aHandle, aFirstPacketIndex, aNumPacketsRequested, aNumPacketsReturned);
	}

void RUsbZeroCopyTransferStrategy::IsocReceivePackets(TInt aHandle, TInt aNumOfPackets)
	{
	ReceivePackets(aHandle, aNumOfPackets);
	}


//Calculate-alignment related methods

/**
 Scan through all the bulk/interrupt endpoints associated with the particular interface
 (and all its alternate settings) to find the maximum bMaxPacketSize across all of these.
 For Interrupt, if there is EP of which the maxPacketSize is not power of 2,
 the maxmaxpaceketsize will be assigned the first maxPacketSize which is not power of 2.  
*/
TInt RUsbZeroCopyTransferStrategy::GetMaximumMaxPacketSize(TInt& aMaxMaxBulk, TInt& aMaxMaxInterrupt)
	{
	TUsbInterfaceDescriptor interfaceDesc;
	TInt err = iInterfaceHandle->GetInterfaceDescriptor(interfaceDesc);
	if (KErrNone != err)
		{
		return err;
		}

	const TUint8 KEPTransferTypeBulk = 0x02;
	const TUint8 KEPTransferTypeInterrupt = 0x03;
	const TUint8 KEPTransferTypeMask = 0x03;
	
	TBool ignoreInterruptEP = EFalse;
	//Traverse all related interface alternate settings
	TUsbGenericDescriptor* descriptor = &interfaceDesc;
	while (descriptor)
		{
		TUsbInterfaceDescriptor* interface = TUsbInterfaceDescriptor::Cast(descriptor);
		
		if (interface)
			{
			//Traverse all endpoint descriptor in the interface
			TUsbGenericDescriptor* subDescriptor = interface->iFirstChild;
			
			while (subDescriptor)
				{
				TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(subDescriptor);
				
				if (endpoint)
					{
					TBool isBulkEP = ((endpoint->Attributes() & KEPTransferTypeMask) == KEPTransferTypeBulk);
					TBool isInterruptEP = ((endpoint->Attributes() & KEPTransferTypeMask) == KEPTransferTypeInterrupt);
					TUint maxPacketSize = endpoint->MaxPacketSize();

					//Caculate the maximum maxPacketSize
					if (isBulkEP)
						{
						if (maxPacketSize > aMaxMaxBulk)
							{
							aMaxMaxBulk = maxPacketSize;
							}
						}
					else if(isInterruptEP && !ignoreInterruptEP)
						{
						if (!IsPowerOfTwo(maxPacketSize))
							{
							aMaxMaxInterrupt = maxPacketSize;
							ignoreInterruptEP = ETrue;
							}
						
						if (maxPacketSize > aMaxMaxInterrupt)
							{
							aMaxMaxInterrupt = maxPacketSize;
							}
						}
					}

				subDescriptor = subDescriptor->iNextPeer;
				}				
			}
		
		descriptor = descriptor->iNextPeer;
		}
	
	return KErrNone;	
	}

/**
Calculate the additional alignment requirement on bulk and interrupt transfer.
For Bulk transfer,
	Scan through all the bulk/interrupt endpoints associated with the particular interface
	to find the maximum wMaxPacketSize across all of these. The new alignment for the transfer
	is the maximum between the maximum bMaxPacketSize and the original alignment
For Interrupt transfer,
	Check if there is endpoints of which the wMaxPacketSize is not power of 2,
	if no, do the same as bulk;
	if yes, the size of transfer data is limited to one page size, and the additional alignment 
            calcualted to make the transfer data not to span page boundary

*/
TInt RUsbZeroCopyTransferStrategy::CaculateAdditionalAlignment(TInt aCurrentOffset, TInt aMaxMaxBulk, TInt aMaxMaxInterrupt, TUsbTransferDescriptorDetails& aTransferDetails)
	{
	RUsbTransferDescriptor::TTransferType transferType = aTransferDetails.iTransferDesc.iType;
	TBool isBulkTransfer = (transferType == RUsbTransferDescriptor::EBulk);
	TBool isInterruptTransfer = (transferType == RUsbTransferDescriptor::EInterrupt);

	if (isBulkTransfer)
		{
		if (aMaxMaxBulk > aTransferDetails.iRequiredAlignment)
			{
			aTransferDetails.iRequiredAlignment = aMaxMaxBulk;
			}
		}
	else if (isInterruptTransfer)
		{
		if (IsPowerOfTwo(aMaxMaxInterrupt))
			{
			if (aMaxMaxInterrupt > aTransferDetails.iRequiredAlignment)
				{
				aTransferDetails.iRequiredAlignment = aMaxMaxInterrupt;
				}
			}
		else
			{
			if (aTransferDetails.iRequiredSize > iPageSize)
				{
				//The transfer data can not span the page boundary
				//if there is EP of which wMaxPacketSize is not power-of-2,
				return KErrNotSupported;
				}
			else
				{
				TInt sizeLeftOfCurrentPage = IncNeededToAlign(aCurrentOffset,iPageSize);
				TInt alignPad = IncNeededToAlign(aCurrentOffset, aTransferDetails.iRequiredAlignment);
				
				//The transfer data can't fit into the current page
				//Align the trasfer data to the next page
				if ( sizeLeftOfCurrentPage < (alignPad + aTransferDetails.iRequiredSize) )
					{
					aTransferDetails.iRequiredAlignment = iPageSize;
					}
				}
			}
		}
	return KErrNone;
	}
