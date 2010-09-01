// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\usbcsc\d_usbcsc.cpp
// LDD for USB Device driver stack, using shared chunks:
// The channel object.
// 
//

/**
 @file d_usbcsc.cpp
 @internalTechnology
*/

#include <drivers/usbcsc.h>
#include "platform.h"

/*****************************************************************************\
*   DUsbcScLogDevice                                                          *
*                                                                             *
*   Inherits from DLogicalDevice, the USB Shared Chunk LDD factory class      *
*                                                                             *
\*****************************************************************************/

_LIT(KUsbScLddName, "Usbcsc");

static const TInt KUsbRequestCallbackPriority = 2;

/** Real entry point from the Kernel: return a new driver.
 */
DECLARE_STANDARD_LDD()
	{
	return new DUsbcScLogDevice;
	}

/** Create a channel on the device.

	@internalComponent
*/
TInt DUsbcScLogDevice::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DLddUsbcScChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}


DUsbcScLogDevice::DUsbcScLogDevice()
      {
	  iParseMask = KDeviceAllowUnit;
	  iUnitsMask = 0xffffffff;								// Leave units decision to the Controller
      iVersion = TVersion(KUsbcScMajorVersion, KUsbcScMinorVersion, KUsbcScBuildVersion);
      }


TInt DUsbcScLogDevice::Install()
	{
	// Only proceed if we have the Controller underneath us
	if (!DUsbClientController::UsbcControllerPointer())
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("LDD Install: USB Controller Not Present"));
		return KErrGeneral;
		}
	return SetName(&KUsbScLddName);
	}


//
// Return the USB controller capabilities.
//
void DUsbcScLogDevice::GetCaps(TDes8& aDes) const
	{
	TPckgBuf<TCapsDevUsbc> b;
	b().version = iVersion;
	Kern::InfoCopy(aDes, b);
	}

// End DUsbcScLogDevice

/*****************************************************************************\
*   TUsbcScChunkInfo                                                          *
*                                                                             *
*   Where Chunk information is stored for the channel, and preseved for the   *
*   life of the chunk.                                                        *
*                                                                             *
\*****************************************************************************/

void DfcChunkCleanup(TAny*);

TUsbcScChunkInfo::TUsbcScChunkInfo(DLogicalDevice* aLdd)
	: 	iChunk(NULL),
		iCleanup((TDfcFn)&DfcChunkCleanup,this,Kern::SvMsgQue(),0),
		iChunkMem(NULL),
		iLdd(aLdd)
	{
	iPageNtz = (TInt8)__e32_find_ls1_32(Kern::RoundToPageSize(1));
	}

TInt TUsbcScChunkInfo::CreateChunk(TInt aTotalSize)
	{
	// First, reserve an TUint of memory for each of pages needed to hold aTotalSize of memory.
	// This will form the chunk map, so that we can look up the memory geometry.
	iAllocatedSize = (aTotalSize>>iPageNtz)*sizeof(TUint);
	iPhysicalMap = (TUint*) Kern::AllocZ(iAllocatedSize);
	TInt r;
	if (iPhysicalMap==NULL)
		r = KErrNoMemory;
	else
		{
		TChunkCreateInfo chunkInfo;
		chunkInfo.iType = TChunkCreateInfo::ESharedKernelMultiple;
		chunkInfo.iMaxSize = aTotalSize;
		chunkInfo.iMapAttr = EMapAttrCachedMax;
		chunkInfo.iOwnsMemory = EFalse;
		chunkInfo.iDestroyedDfc = &iCleanup;

		TLinAddr chunkMem;
		r = Kern::ChunkCreate(chunkInfo, iChunk, chunkMem, iChunkMapAttr);
		iChunkMem = (TInt8*) chunkMem;
		if (r==KErrNone)
			iLdd->Open();
		}

	return r;
}


// This method requests closing the chunk.
// Note that nothing may happen immediately, as something else may have the chunk open.
void TUsbcScChunkInfo::Close()
{
	Kern::ChunkClose(iChunk);	
}


TInt TUsbcScChunkInfo::ChunkAlloc(TInt aOffset, TInt aSize)
	{
	TUint pageMask = (~0)<<iPageNtz;
	TUint rleMask = ~pageMask;
	TUint pageSize = rleMask+1;
	TInt r;
	TLinAddr physAddr;

	__KTRACE_OPT(KUSB, Kern::Printf("::chunkalloc  AllocPhysicalRam aSize %d", aSize));

	r = Epoc::AllocPhysicalRam(aSize, physAddr);
	__KTRACE_OPT(KUSB, if (r!=KErrNone) Kern::Printf("::chunkalloc AllocPhysicalRam r=%d  (Error!)", r));
	if (r==KErrNone)
		{	
		__KTRACE_OPT(KUSB, Kern::Printf("::chunkalloc ChunkCommitPhysical iChunk 0x%x size(%d), aOffset 0x%x, aSize 0x%x phsAddr 0x%x",
																	 				iChunk, sizeof(DChunk), aOffset, aSize,physAddr ));

		r = Kern::ChunkCommitPhysical(iChunk, aOffset, aSize, physAddr);
		__KTRACE_OPT(KUSB, if (r!=KErrNone) Kern::Printf("::chunkalloc ChunkCommitPhysical r=%d  (Error!)", r));

		if (r!=KErrNone)
				Epoc::FreePhysicalRam(physAddr, aSize);
		else 
			{ // record physical address and length in physical map
			TInt rle;
			TInt i=0;
			for (rle=(aSize>>iPageNtz); rle>0; rle--, i++,physAddr+=pageSize) 
				{
				__KTRACE_OPT(KUSB, Kern::Printf("::phys offset 0x%x = 0x%x",
												(aOffset>>iPageNtz)+i,  (physAddr & pageMask) | ((rle>(TInt)rleMask)?(TInt)rleMask:rle)));
				iPhysicalMap[(aOffset>>iPageNtz)+i] = (physAddr & pageMask) | ((rle>(TInt)rleMask)?(TInt)rleMask:rle);
				}
			}
		}
	else if (r==KErrNoMemory)
		r = -KErrNoMemory;  // Semi-expected error.
	return r;
	}

/**
This method retrieves the physical address of a given offset into the Chunk, and returns
the length of contiguous physical memory from this point.

@param aOffset		the offset from the start of the chunk, to be queried.
@param aPhysical	a pointer to a TPhysAddr, to be filled with the physical
					address of the memory at the given offset.

@returns the length of contiguous physical memory from the given offset.
*/

TInt TUsbcScChunkInfo::GetPhysical(TInt aOffset, TPhysAddr* aPhysical)
	{
	// Use masks, to retrieve the two components from the physical map, we created of the memory.
	TUint pageMask = (~0)<<iPageNtz;
	TUint val =  iPhysicalMap[aOffset>>iPageNtz];
	*aPhysical=(val & pageMask)+(aOffset & ~pageMask);
	return ((val & ~pageMask)<<iPageNtz) -  (aOffset & ~pageMask);
	}


// DFC calls this fuction, which invokes the cleanup method.

void DfcChunkCleanup(TAny* aChunkInfo)
	{
	((TUsbcScChunkInfo*) aChunkInfo)->ChunkCleanup();
	}


void TUsbcScChunkInfo::ChunkCleanup()
{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScChunkInfo::ChunkCleanup()"));
	TUint physAddr;
	TInt length;
	TInt offset = 0;
	
	// The part of the field used for the physical page address.
	TUint pageMask = (~0)<<iPageNtz;

	// The part of the field used for the run length encoding, of the contiguous pages.
	TUint rleMask = ~pageMask;
	TInt records=(iAllocatedSize>>2);

	while (offset < records) 
		{
		physAddr = 	iPhysicalMap[offset] & pageMask;
		length = iPhysicalMap[offset] & rleMask;

		if (physAddr>0)	
			Epoc::FreePhysicalRam(physAddr, length);

		offset += (length>0)?length:1;
		}
	Kern::Free(iPhysicalMap);

	DLogicalDevice* ldd = iLdd;
	delete this;
	ldd->Close(NULL);
}

TInt TUsbcScChunkInfo::New(TUsbcScChunkInfo*& aChunk, TInt aSize, DLogicalDevice* aLdd)
{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScChunkInfo::New totalSize %d", aSize));

	aChunk = new TUsbcScChunkInfo(aLdd);
	if (aChunk==NULL)
		{
		return KErrNoMemory;
		}
					
	TInt r = aChunk->CreateChunk(aSize);
	if (r!=KErrNone)
		{
		delete aChunk;
		aChunk=NULL;
		return r;
		}

	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScChunkInfo::New Created at 0x%x",  aChunk->iChunkMem  ));
	return KErrNone;
}

// End TUsbcScChunkInfo

/*****************************************************************************\
*    TUsbcScBuffer                                                            *
*                                                                             *
*    Represents a buffer, within a chunk.  Each buffers can be used by        *
*    differt endpoint on differnt alt settings                                *
*                                                                             *
\*****************************************************************************/


TInt TUsbcScBuffer::Construct(TInt aDirection, DLddUsbcScChannel* aLdd, TInt aBufferOffset, TInt aBufferEndOffset, TInt aMinReadSize, TInt aMaxPacketSize, TInt aMaxReadSize)
	{
	TInt r;
#ifdef _DEBUG
	iSequence = aBufferOffset; // Initialized at this, so that each buffer starts with a diffrent sequence number
#endif
	iMinReadSize = aMinReadSize;
	TInt size = (aBufferEndOffset - aBufferOffset);
	TInt pageSize = Kern::RoundToPageSize(1);
	if (aMaxReadSize > 0)
		iMaxReadSize = aMaxReadSize;
	else
		iMaxReadSize = pageSize + ((size/3) & ~(pageSize -1));
	iLdd = aLdd;
	iDirection = aDirection;
	iMode=0;
	iChunkInfo = aLdd->iChunkInfo;
	iChunkAddr = (TLinAddr) (aLdd->iChunkInfo->iChunkMem);  //aChunkAddr;

	TInt headerSize =  sizeof(TUsbcScTransferHeader)-4; // TransferHeader includes 4 bytes of data.


	TUint maxAlignment; // Note:  This is a mask for max Alignment, 

	if (aMaxPacketSize)
		{ // EP0 packets are not DMAed, and so dont need ialignment.
		iAlignMask = ~3;
		maxAlignment = 3;
		}
	else
		 maxAlignment = 1023; // We don't know what the alignment requirement will be until enumeration, so assume worse case.

	iFirstPacket = aBufferOffset + sizeof(SUsbcScBufferHeader) + headerSize;
	iFirstPacket = (iFirstPacket + maxAlignment) & ~maxAlignment;
	
	iBufferStart = (SUsbcScBufferHeader *) (iChunkAddr+aBufferOffset);
	iBufferEnd = aBufferEndOffset;

	if ((iDirection&1)==KUsbcScOut)
		iHead = iFirstPacket-headerSize;//aBufferOffset + sizeof(SUsbcScBufferHeader);
	else
		iSent = 0;

	iStalled=0;
	iMaxPacketSize=0;
	
	r =  iStatusList.Construct((aDirection==KUsbcScIn)?KUsbcScInRequests:KUsbcScOutRequests, iLdd->iClient);
	if (!r)
		{
		iMaxPacketSize = aMaxPacketSize; // Indicates configured if ep0, otherwise not.
		}
	return r;
	}


void TUsbcScBuffer::CreateChunkBufferHeader()
{
	if ((iDirection&1)==KUsbcScOut)
		{
		iBufferStart->iHead= iHead;
		iBufferStart->iTail= iHead; // Initially no data!
		iBufferStart->iBilTail=iHead;
		__KTRACE_OPT(KUSB, Kern::Printf("Realize:  iHead 0x%x  bufferHeader 0x%x", iHead,iBufferStart ));

		// Dont need to round here, as we will round it up on endpoint change. (configuration)
		}
}

/*
TUsbcScBuffer::StartEndpoint

This method sets the nessesary paramenters to the buffer, for use for a particular endpoint.

*/
void TUsbcScBuffer::StartEndpoint(TUsbcRequestCallback* aRequestInfo, TUint aFlags)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartEndpoint (0x%x) : ep %d(%d)",this,aRequestInfo->iEndpointNum, aRequestInfo->iRealEpNum));
	
	iCallback=aRequestInfo;
	iMaxPacketSize =  iLdd->iController->EndpointPacketSize(iLdd, aRequestInfo->iRealEpNum);
	iAlignMask = ~(((iMaxPacketSize+1) & 0xFFFFFFF8)-1);
	iMode = aFlags;
    __KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartEndpoint : max Packets %d, mask 0x%x flags 0x%x", iMaxPacketSize, iAlignMask, iMode));
	if ((iDirection&1)==KUsbcScOut)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UsbcScOut\n"));
		// Add dummy packet (doesnt have to be aligned, which avoids what if it changes issue)
		// And Start next read.
		iNeedsPacket=KEpIsStarting;
		}
	}



void TUsbcScBuffer::Destroy()
{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::Destroy()"));
	Cancel(KErrCancel);
	if (iLdd->iController && ((iDirection&1)==KUsbcScOut))  
		{  // Me must cancel reads to LDD to, an there will be no list for the callbacks to look into.
		iLdd->iController->CancelReadBuffer(iLdd, iCallback->iRealEpNum);
		}
	iStatusList.Destroy();
}



TInt TUsbcScBuffer::StartDataRead()
{
	if (!iMaxPacketSize)
	{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataRead() - Not Configured"));
		return KErrNone;
	}
	if (iStatusList.iState!=ENotRunning) 
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataRead() - Already Stated! (%d)",iStatusList.iState));
		return KErrNone;
		}

	TInt maxLength;
	TInt freeSpace;
	TPhysAddr physAddr;

	// get next request
	TUsbcScStatusElement* nextJob = iStatusList.Next();
	if (nextJob == NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("No more jobs"));
		if (iMode && KUsbScCoupledRead)
			return KErrEof;
		iStatusList.iState=EReadingAhead;
		}
	else
		iStatusList.iState=EInProgress;

	TInt tail = iBufferStart->iTail;
	TInt headerSize =  sizeof(TUsbcScTransferHeader)-4; // TransferHeader includes 4 bytes of data.
	maxLength = iChunkInfo->GetPhysical(iHead + headerSize, &physAddr); //returns all the bytes available after iHead + headerSize)

	__ASSERT_DEBUG(maxLength>0,Kern::Fault("TUsbcScBuffer::StartDataRead(", __LINE__)); 


	if (tail>iHead)  //  # # # H _ _ _ T # # # #
		{
		__KTRACE_OPT(KUSB,Kern::Printf("TUsbcScBuffer::StartDataRead() - tail 0x%x>head 0x%x, maxlength 0x%x", tail, iHead, maxLength));

		freeSpace = (tail & iAlignMask) - (iHead +headerSize + (~iAlignMask+1) );  // Cant read right up to last buffer, or head/tail will cross.

		if (freeSpace<iMinReadSize)
			{
			iStatusList.iState=ENotRunning;
			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataRead() - Stall!!"));
			return KErrOverflow; 				// Read STALL !! !! !!
			}

		if (freeSpace<maxLength)
			maxLength = freeSpace;
		}
	if (maxLength> iMaxReadSize) 
		maxLength =  iMaxReadSize;
	// else  tail<iHead (or empty)      _ _ _ T # # # H _ _ _ _
	// We would not have set iHead here if too small. So must be ok.
		
	__ASSERT_DEBUG(maxLength>=iMinReadSize,Kern::Fault("TUsbcScBuffer::StartDataRead(", __LINE__)); 

	TUint8* data = ((TUsbcScTransferHeader *) (iHead + iChunkAddr))->iData.b;
	// set up callback stucture

	iCallback->SetRxBufferInfo(data, physAddr, iIndexArray, iSizeArray,maxLength);
	TInt r;
	// Go!!
	r = iLdd->iController->SetupReadBuffer(*iCallback);
	if (r!=KErrNone)
		{
		__KTRACE_OPT(KUSB,Kern::Printf("SetupReadBuffer Error: %d, RT %d",r, iStatusList.iState));
		iStatusList.Complete(r);
		}
	// After this, TUsbcScEndpoint::RequestCallback is called in a DFC.
	// This in turn calls either TUsbcScBuffer::CompleteRead.
	return KErrNone;
}


void TUsbcScBuffer::CompleteRead(TBool aStartNextRead)
{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::CompleteRead buff=%x",this));

    // The first packet always contains the total #of bytes
	const TInt byteCount = iCallback->iPacketSize[0];
	const TInt packetCount = iCallback->iRxPackets;
 	iCallback->iRxPackets=0;
	TUint flags = 0;

	if (iCallback->iPacketSize[packetCount - 1] < (TUint) iMaxPacketSize)
		flags = KUsbcScShortPacket;

	UpdateBufferList(byteCount, flags, aStartNextRead);
}


// This method "submits" the current transfer, and starts off the next read.

void TUsbcScBuffer::UpdateBufferList(TInt aByteCount,TUint aFlags, TBool aStartNextRead)
	{

	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst aByteCount %d, flags 0x%x iHead 0x%x", aByteCount, aFlags, iHead));

	TInt headerSize =  sizeof(TUsbcScTransferHeader)-4; // TransferHeader includes 4 bytes of data.
	TLinAddr dummy;
	__KTRACE_OPT(KUSB, Kern::Printf("iHead 0x%x headerSize 0x%x",iHead, headerSize));

	// Find iNext

	TInt next =  iHead +  headerSize + aByteCount; // next unused byte in buffer.
	TInt maxLength; 

	// This may take a few loops before we settle on a value.
	do 
		{
		// round up.
		next = (next + headerSize + ~iAlignMask) & iAlignMask;
		maxLength = iChunkInfo->GetPhysical(next, &dummy);

		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst  next %x  buffer end %x min-read: %x  maxRun %x", next, iBufferEnd, iMinReadSize, maxLength));
		// At the end of the buffer - wrap it if needbe.
		if ((TUint)(next + iMinReadSize) > iBufferEnd)
			{
			next = iFirstPacket;
			continue;
			}
		// Not enough space, move onto next block.
		if (maxLength<iMinReadSize) 
			{
			next+=maxLength;
			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst Skip exhausted block. next %x max %d", next, maxLength));
			continue;
			}
		}
	while (EFalse);

	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst next (pre deduct): %x, Fill in header at head: 0x%x,  BuffStart: 0x%x.", next, iHead, iBufferStart));
	
	next -=  headerSize;  // Move next back from the data start position, to the header start.

	TUsbcScTransferHeader* header = (TUsbcScTransferHeader*) (iHead + iChunkAddr);
	
// Create Header
#ifdef _DEBUG
	header->iHashId=59*(iLdd->iAlternateSetting+1)+iCallback->iRealEpNum; // Alt setting realated....
	header->iSequence=iSequence;
	iSequence++;
#endif
	header->iBytes=aByteCount;
	header->iNext=next;
	header->iAltSettingSeq=iLdd->iAsSeq;
	header->iAltSetting=iLdd->iAlternateSetting;
	header->iFlags=aFlags;
	__KTRACE_OPT(KUSB, Kern::Printf("We set next to 0x%x", next));

	iStatusList.iState=ENotRunning;
	if (next==iBufferStart->iTail) //or (othwise is as good as full)
		{
			iStalled=next;
		}
	else
		{

		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst StartRead?? "));
		TInt oldHead=iHead;
		iHead = next;

		if ((aStartNextRead) && (StartDataRead() == KErrOverflow))
			{ // Oh crumbs, set state as slalled.
			if (oldHead != iBufferStart->iBilTail) 
				// If user has not read everything in the buffer
				// then set up a stall, so that ldd get to be woken early
				{
				iStalled=next;
				iHead=oldHead;
				}
			else // otherwise if everything is read
				// no choice but to return what we have
				{
				iBufferStart->iHead = iHead;
				}
			}
		else
			{
			iBufferStart->iHead = next;
			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::UpdateBUfferLIst Compleating\n"));
			}
		// Complete userside
		iStatusList.Complete();
		}  
	}

void TUsbcScBuffer::PopStall()
	{
	if (iStalled==iBufferStart->iTail)
		return;  // Still stalled.

	if (iStalled!=-1) // If not Alt packet only stall
	{
		// pop off packet	
		iHead = iStalled;
 	}
	iStalled=0;
	// If Alt setting of the popped packet is different to now
	// Add alt setting change packet.


	if (StartDataRead() == KErrOverflow)
	{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::PopStall Warning: Transfer was freed, but still no space!\n"));
	}

	iBufferStart->iHead = iHead;
	}



void TUsbcScBuffer::StartDataWrite()
	{
	
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataWrite()"));
	TUsbcScStatusElement* nextJob = iStatusList.Next();
	TBool zlpReqd;
	TInt length;
	TUint start;
	TUint8* startAddr;
	TInt maxLength;
	TPhysAddr physAddr;
	TInt r;
	if (!iMaxPacketSize)
	{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataWrite() - Not Configured"));
		return;
	}

	if (nextJob == NULL)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataWrite() - No more jobs d=%d", iDirection));
		if (iDirection==KUsbcScBiIn) // assume this is EP0, if this is true.
			{
			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::StartDataWrite() Queue Read on EP0."));	
			// Start other read again.
			iLdd->iBuffers[iLdd->iEP0OutBuff].StartDataRead();
			}
		}
	else
		{
		if (iStatusList.iState==ENotRunning)
			iSent=0;
		iStatusList.iState=EInProgress;

		start = nextJob->iStart;
		startAddr = (TUint8*) (start + ((TUint) (iChunkInfo->iChunkMem)));

		length = nextJob->iLength;
		zlpReqd = (nextJob->iFlags & KUsbcScWriteFlagsZlp) !=0;
		// get max read length
		maxLength = iChunkInfo->GetPhysical( start, &physAddr); 

		if (maxLength < length)
			{
				// modify request.
				nextJob->iStart += maxLength;
				nextJob->iLength -= maxLength;
				// start this request.
				iStatusList.iState=EFramgementInProgress;
				zlpReqd=EFalse;
				length =  maxLength;
			}

		if (iDirection==KUsbcScBiIn) // this is for EP0
			{
			iLdd->iController->CancelReadBuffer(iLdd, iCallback->iRealEpNum);
			iLdd->iBuffers[iLdd->iEP0OutBuff].iStatusList.iState=ENotRunning;
			}
		
		iCallback->SetTxBufferInfo(startAddr, physAddr, length);
		iCallback->iZlpReqd = zlpReqd;
		r = iLdd->iController->SetupWriteBuffer(*iCallback);
		if (r!=KErrNone)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("SetupWriteBUffer Error: %d",r));
			iStatusList.Complete(r);
			}
		}

	}

void TUsbcScBuffer::CompleteWrite()
	{
	TInt error = iCallback->iError;
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::CompleteWrite buff=%x, err=%d",this, error));
	iSent+= iCallback->iTxBytes;

	// More to send?
	if (error || iStatusList.iState!=EFramgementInProgress)
		{
		// complete request with error (if one).
		// Some data could have been transmitted, even with an error. 
		iStatusList.Complete(error);
		}

	// Start next request, or next part of this one.
	StartDataWrite();
	
	}

// Cancels the current request's callback.
// This is not to say it will cancel the actual operation,
// However it will cancel any further sections of the user perceived operation
// that are not yet started.
void TUsbcScBuffer::Cancel(TInt aErrorCode)
	{
	iStatusList.CancelQueued();
	if (iLdd->iController && ((iDirection&1)==KUsbcScIn))
		{
		iLdd->iController->CancelWriteBuffer(iLdd, iCallback->iRealEpNum);
		}

	iStatusList.Complete(aErrorCode);
	}

void TUsbcScBuffer::Ep0CancelLddRead()
	{
	// Stopping a read isn't as easy as one might think.
	// We cancel the callback, but then check if any data was received (but not returned to us).
	// If so, we must de-queue the request, and call the completion code.
	
	iLdd->iController->CancelReadBuffer(iLdd, iCallback->iRealEpNum);
	if (iCallback->iRxPackets) // received data?
		{
		// remove DFC (if infact sent)
		iCallback->iDfc.Cancel();

		// process the callback now, but dont start another
		CompleteRead(EFalse);
		}
	}

void TUsbcScBuffer::SendEp0StatusPacket(TInt aState)
{
	__KTRACE_OPT(KUSB, Kern::Printf(" TUsbcScBuffer::SendEp0StatusPacket(%d)", aState));

	// We need to add a packet to the buffer, so we must stop the pending read, and start
	// another after we have added out packet.  
	Ep0CancelLddRead();

	TUint* state = ((TUsbcScTransferHeader *) (iHead + iChunkAddr))->iData.i;
	*state = aState;
	UpdateBufferList(4,KUsbcScStateChange);
}

// End TUsbcScBuffer

/*****************************************************************************\
*    TUsbcScStatusList                                                        *
*                                                                             *
*    This is a list of read or write requests, containing user status         *
*    requests, that should later be completed.                                *
*                                                                             *
\*****************************************************************************/

/**
Constructor for TUsbcScStatusList.

@param aSize	is the number of requests to allow at any one time.  This value
				must be a power of two, for correct operation.

@returns KErrNoMemory if memory allocation failure, otherwise KErrNone.
*/

TInt TUsbcScStatusList::Construct(TInt aSize, DThread* aClient)
	{
	iSize=aSize;
	iHead = 0;
	iLength = 0;
	iClient = aClient;
	iElements=(TUsbcScStatusElement *) Kern::AllocZ(sizeof(TUsbcScStatusElement)*aSize);
	return (iElements==NULL)?KErrNoMemory:KErrNone;	
	};


// StatusList must be inactive before destroying.
void TUsbcScStatusList::Destroy()
	{
	if (iState!=ENotRunning)
		Kern::Fault("TUsbcScStatusList::Destroy", __LINE__);
	if (iElements)
		{
		Kern::Free(iElements);	
		iElements=NULL;
		}
	iClient=NULL;
}

void TUsbcScStatusList::Pop()
	{
	if (iLength>0)
		{
		iLength--;
		iHead = ((iHead+1) & (iSize-1));
		}
	}

TUsbcScStatusElement* TUsbcScStatusList::Next()
	{
	return (iLength==0)?NULL:&(iElements[iHead]);
	}

TInt TUsbcScStatusList ::Add(TRequestStatus* aStatus, TInt aLength, TUint aStart, TUint aFlags)
	{
	__KTRACE_OPT(KUSB,Kern::Printf("Adding request.  iLength %d  iSize %d", iLength, iSize));
	if (iLength<iSize)
		{
		TUsbcScStatusElement& e = iElements[((iHead+iLength) & (iSize-1))];
		e.iStatus = aStatus;
		e.iLength = aLength;
		e.iStart = aStart;
		e.iFlags = aFlags;
		iLength++;
		__KTRACE_OPT(KUSB,Kern::Printf("Adding request.  new iLength %d", iLength));

		return KErrNone;
		}
	else
		return KErrInUse;
	}



// This method cancels any requests that have yet to be started.

void TUsbcScStatusList::CancelQueued(TInt aError)
{
	if ((iLength==0) || ((iState!=ENotRunning) && (iLength==1)))  // Nothing to do.
		return;  
	TInt elements2Complete = iLength - (iState?1:0);
	TInt head = iHead;
	iLength = 0;
	if (iState)	// If (iState != ENotRunning), complete all elements excepting the one at head
		{
		head = ((head+1) & (iSize-1)); // To iterate through the queue
		iLength = 1;
		}
	// complete them all.
	for (; elements2Complete>0; elements2Complete--)
  		{
		Kern::RequestComplete(iClient, iElements[head].iStatus, aError);
		head = ((head+1) & (iSize-1)); 
  		}
	
}


/* This method Completes the head status request, and pops it from its list.
This version of Complete is to be used in cases where the next request is not
chained - usually because of an error.

@Param aError - the code to complete with.

returns KErrNotFound if there was no request to complete
*/


TInt TUsbcScStatusList::Complete(TInt aError)
	{
	if (iState==ENotRunning)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScStatusList::Complete() - iState == ENotRunning!"));
		}
 	else
		{
		iState=ENotRunning;
		if (iLength==0)
			return KErrNotFound;

		Kern::RequestComplete(iClient, iElements[iHead].iStatus, aError);

		iLength--;
		iHead = ((iHead+1) & (iSize-1));
		}
	return KErrNone;
	}


/* This method Completes the head status request, and pops it from its list. (If found.)
This version of Complete is to be used in cases where the request is successful, and
 next request after this has (if present) been chained.
*/

void TUsbcScStatusList::Complete()
	{
	if (iLength==0)
		return;
	__KTRACE_OPT(KUSB, Kern::Printf("Completing request.  iLength %d", iLength));

	Kern::RequestComplete(iClient, iElements[iHead].iStatus, KErrNone);

	iLength--;
	iHead = ((iHead+1) & (iSize-1));
	}

// End TUsbcScStatusList

/*****************************************************************************\
*   TRealizeInfo                                                              *
*                                                                             *
*   Used by DLddUsbcScChannel::RealizeInterface to set up the chunk           *
*                                                                             *
\*****************************************************************************/

// Init
//
// This method works out the number potential maximum number of endpoints
// and the number of alt settings.  With this information it allocs
// the necessary space for the given stucture to store information about
// the endpoints.  
// This is intended to be called by RealizeInterface.  This stucture is
// intended to be only temporary, and the space will be freed with Free()
// before RealizeInteface has finished.

void TRealizeInfo::Init(TUsbcScAlternateSettingList* aAlternateSettingList)
{
	iAlternateSettingList = aAlternateSettingList;
	iMaxEndpoints=0;
	iTotalSize   =0;
	iTotalBuffers=0;
	iAltSettings =0;
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: work out max endpoint"));
	// Work out max endpoints and number of alternate settings.

	if (iAlternateSettingList)
		{
		TUsbcScAlternateSetting* alt = iAlternateSettingList->iHead;
		while (alt != NULL) 
			{
			iAltSettings++;
			if (alt->iNumberOfEndpoints>iMaxEndpoints)
				iMaxEndpoints = alt->iNumberOfEndpoints;
			// could work out in/out specifics, but unnecessary.
			alt = alt->iNext;
			};
		}
	
	// Alloc some temporary working space for temp endpoint metadata 
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: Alloc temp.  Maxendpoints %d", iMaxEndpoints));
	TInt inout;
	for (inout=KUsbcScIn; inout<KUsbcScDirections; inout++)
		{
		iBufs[inout].iEp = (TUsbcScEndpoint **) Kern::AllocZ(iAltSettings*iMaxEndpoints*sizeof(TUsbcScEndpoint *));
		iBufs[inout].iSizes = (TInt *) Kern::AllocZ(iMaxEndpoints*sizeof(TInt));
		}
}

// CopyAndSortEndpoints
//
// This method copies pointers to the endpoint records into TRealizeInfo
// such that they are sorted in order of size per alt setting.
// In and Out endpoints are separated, and kept separate.
// The provided data structure is assumed to have been initialised with
// Realize_InitRealizeInfo. 
//
// Return KErrArgument if the direction field is neither In or Out.
//

TInt TRealizeInfo::CopyAndSortEndpoints()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: copy And sort"));

	TInt altSetting = 0;
	TInt endpointOffs;
	TInt endpoint;
	TInt altEp;
	TInt inout;
	TBool placed;
	TUsbcScAlternateSetting* alt;
	TEndpointSortBufs* bufsd;

	if (iAlternateSettingList)
		{
		for (alt = iAlternateSettingList->iHead;alt!=NULL;alt = alt->iNext )
			{		
			__KTRACE_OPT(KUSB, Kern::Printf("Realize:   AlternateSetting %x", alt));

			iBufs[KUsbcScIn].iEps =0;
			iBufs[KUsbcScOut].iEps =0;

			// For alt setting, iterate eps
			for (altEp=1; altEp <= alt->iNumberOfEndpoints; altEp++)
				{
				__KTRACE_OPT(KUSB, Kern::Printf("Realize:     Endpoint to add: %d",altEp));

				TUsbcScEndpoint* nextEp = alt->iEndpoint[altEp];

				__KTRACE_OPT(KUSB, Kern::Printf("Realize:      ep Buffer Size: %d",nextEp->EndpointInfo()->iBufferSize));
				
				inout = (nextEp->EndpointInfo()->iDir==KUsbEpDirIn)?KUsbcScIn:
						(nextEp->EndpointInfo()->iDir==KUsbEpDirOut)?KUsbcScOut:KUsbcScUnknown;
				if (inout==KUsbcScUnknown)
					{
					__KTRACE_OPT(KUSB, Kern::Printf("Realize:     KUsbcScUnknown %x",nextEp->EndpointInfo()->iDir));
					return KErrArgument;
					}

				bufsd = &(iBufs[inout]);
				__KTRACE_OPT(KUSB, Kern::Printf("Realize:      ep direction: %x # endpoints %d", inout, bufsd->iEps));


				// find and position ep, and insert.

				if (bufsd->iEps==0) // First entry.
					{
					__KTRACE_OPT(KUSB, Kern::Printf("Realize:       Add first endpoint"));
					endpointOffs = altSetting*iMaxEndpoints;
					bufsd->iEp[endpointOffs] = nextEp;
					}
				else
					{
					placed = EFalse;
					// Move down the list, until we find the right place.
					for (endpoint=bufsd->iEps-1; endpoint>-1; endpoint--)
						{
						endpointOffs = altSetting*iMaxEndpoints + endpoint;
						if (bufsd->iEp[endpointOffs]->EndpointInfo()->iBufferSize < nextEp->EndpointInfo()->iBufferSize)
							{
							__KTRACE_OPT(KUSB, Kern::Printf("Realize:       Shift Endpoint %d", endpoint));
		
							bufsd->iEp[endpointOffs+1] = bufsd->iEp[endpointOffs];
							}
						else
							{
							__KTRACE_OPT(KUSB, Kern::Printf("Realize:       Insert After Endpoint %d", endpoint));

							bufsd->iEp[endpointOffs+1] = nextEp;
							placed = ETrue;
							break;
							}
						} // end for endpoint
						if (!placed) // if we didn't place it, it must be the biggest so far, so goes at the top.
							bufsd->iEp[0] = nextEp;
					} // endif
				bufsd->iEps++;			
				} // for altEp
				altSetting++;
			} // for alt
		}// if iAltsettingList
	return KErrNone;
	}

// CalcBuffSizes
//
// This works out the sizes of all the buffers, and stores the result in aBufInfo
// based on the buffer information provided in the same structure.
// Realize_CopyAndSortEndpoints is used to fill the structure with the informaition
// required.

void TRealizeInfo::CalcBuffSizes()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: Calculate Buffers"));

	TInt endpoint;
	TInt inout;
	TInt altSetting;
	TUsbcScEndpoint* nextEp;
	TInt bufferSize;
	TEndpointSortBufs* bufsd;

	for (inout=KUsbcScIn; inout<KUsbcScDirections; inout++)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Realize:   Direction: %d", inout));


		bufsd = &(iBufs[inout]);
		// for each row, ie, buffer, find largest buffer need.
		for (endpoint=0; endpoint<iMaxEndpoints; endpoint++)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("Realize:     endpoint %d", endpoint));
			TInt bufMaxSize=0;
			for (altSetting=0; altSetting< iAltSettings; altSetting++)
				{
				__KTRACE_OPT(KUSB, Kern::Printf("Realize:       altSetting %d", altSetting));
				nextEp= bufsd->iEp[altSetting* iMaxEndpoints + endpoint];
				if (nextEp!=NULL)
					{
					bufferSize = nextEp->EndpointInfo()->iBufferSize;
					__KTRACE_OPT(KUSB, Kern::Printf("Realize:       comparing size %d", bufferSize));
					if (bufferSize> bufMaxSize)
						 bufMaxSize = bufferSize;
					}
				} // for altsetting
			__KTRACE_OPT(KUSB, Kern::Printf("Realize:     bufMaxSize %d", bufMaxSize));
			bufsd->iSizes[endpoint] = bufMaxSize;
			if (bufMaxSize>0) 
				{
				iTotalSize += bufsd->iSizes[endpoint];
				iTotalBuffers++;
				}
			} // for endpoint
		} // for in/out	
}

// Free
//
// Cleans up after Init()

void TRealizeInfo::Free()
	{
	TInt inout;
	for (inout=KUsbcScIn; inout<KUsbcScDirections; inout++)
		{
		Kern::Free(iBufs[inout].iEp);
		Kern::Free(iBufs[inout].iSizes);
		}
	}

// End TRealizeInfo


// LayoutChunkHeader
//
// Sets up some geometry for the chunk;

void TRealizeInfo::LayoutChunkHeader(TUsbcScChunkInfo* aChunkInfo)
{ 
	// First set up the indexes to the header structures.
	TUsbcScChunkHdrOffs* chkHdr = (TUsbcScChunkHdrOffs*) aChunkInfo->iChunkMem;

	chkHdr->iBuffers = sizeof(TUsbcScChunkHdrOffs); // First struct just after this one.
	iChunkStuct = (TUsbcScChunkBuffersHeader*) ( (TInt) aChunkInfo->iChunkMem + chkHdr->iBuffers);

	// Store number of buffers in chunk
	iChunkStuct->iRecordSize = sizeof(TUsbcScBufferRecord);
	iChunkStuct->iNumOfBufs=iTotalBuffers;

	iAltSettingsTbl = (TUsbcScChunkAltSettingHeader*) &(iChunkStuct->iBufferOffset[(iTotalBuffers+2)*sizeof(TUsbcScBufferRecord)]); // 2 extra for EP0 in and out.

	chkHdr->iAltSettings = (TUint) iAltSettingsTbl - (TUint) aChunkInfo->iChunkMem;

	iAltSettingsTbl->iEpRecordSize = sizeof(TUint);
	iAltSettingsTbl->iNumOfAltSettings = iAltSettings;


	TInt tableOffset  = (TUint) iAltSettingsTbl->iAltTableOffset - (TUint) aChunkInfo->iChunkMem + iAltSettings*sizeof(TInt);
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: table offset: 0x%x, altTble %x iChnkMem %x altSettings %x",tableOffset, iAltSettingsTbl, aChunkInfo->iChunkMem, iAltSettings ));

	__KTRACE_OPT(KUSB, Kern::Printf("Realize: populate chunk - create alt settings table"));

	// Create alt settings table.  Set each element of altsettings table, to each induivatual alt setting table.
	// then fill in the number of endpoints for that alt setting, in the table.

	TInt* noEpForAlt;
	TInt altSetting;
	TUsbcScAlternateSetting* alt;
	if (iAlternateSettingList)
		{
		alt = iAlternateSettingList->iHead;
		for (altSetting=0; altSetting<iAltSettings; altSetting++) 
			{
				__KTRACE_OPT(KUSB, Kern::Printf("Realize:   altSetting %d, tableOffset %d", altSetting, tableOffset));

				iAltSettingsTbl->iAltTableOffset[altSetting] = tableOffset;
				noEpForAlt = (TInt*) &aChunkInfo->iChunkMem[tableOffset];
			 
				*noEpForAlt = alt->iNumberOfEndpoints;  // Set NumberofEndpoints field in Altsetting table
				tableOffset+= sizeof(TInt)+ alt->iNumberOfEndpoints*sizeof(TUsbcScHdrEndpointRecord);
				alt = alt->iNext;
			}
		}		

} // end LayoutChunkHeader



/*****************************************************************************\
*   DLddUsbcScChannel                                                         *
*                                                                             *
*   Inherits from DLogicalDevice, the USB Shared Chunk LDD factory class      *
*                                                                             *
\*****************************************************************************/

//
// Constructor
//
DLddUsbcScChannel::DLddUsbcScChannel()
	: iValidInterface(EFalse),
	  iAlternateSettingList(NULL),
	  iEndpoint(NULL),
	  iCompleteAllCallbackInfo(this, DLddUsbcScChannel::EmergencyCompleteDfc, KUsbRequestCallbackPriority),
	  iStatusChangePtr(NULL),
	  iStatusCallbackInfo(this, DLddUsbcScChannel::StatusChangeCallback, KUsbRequestCallbackPriority),
	  iEndpointStatusChangePtr(NULL),
	  iEndpointStatusCallbackInfo(this, DLddUsbcScChannel::EndpointStatusChangeCallback,
								  KUsbRequestCallbackPriority),
      iOtgFeatureChangePtr(NULL),
      iOtgFeatureCallbackInfo(this, DLddUsbcScChannel::OtgFeatureChangeCallback, KUsbRequestCallbackPriority),
	  iNumberOfEndpoints(0),
	  iDeviceState(EUsbcDeviceStateUndefined),
	  iOwnsDeviceControl(EFalse),
	  iAlternateSetting(0),
	  iAsSeq(0),
	  iStatusFifo(NULL),
	  iUserKnowsAltSetting(ETrue),
	  iDeviceStatusNeeded(EFalse),
	  iChannelClosing(EFalse),
	  iRealizeCalled(EFalse),
	  iChunkInfo(NULL),
	  iNumBuffers(-1),
	  iBuffers(NULL),
	  iEp0Endpoint(NULL)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DLddUsbcScChannel()"));
	iClient = &Kern::CurrentThread();
	iClient->Open();
	for (TInt i = 1; i < KUsbcMaxRequests; i++)
		{
		iRequestStatus[i] = NULL;
		}
	}


//
// Destructor
//

DLddUsbcScChannel::~DLddUsbcScChannel()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::~DLddUsbcScChannel()"));
	if (iController)
		{
		iController->DeRegisterClient(this);
		iStatusCallbackInfo.Cancel();
		iEndpointStatusCallbackInfo.Cancel();
	    iOtgFeatureCallbackInfo.Cancel();
        iCompleteAllCallbackInfo.Cancel();
		DestroyAllInterfaces();
		if (iOwnsDeviceControl)
			{
			iController->ReleaseDeviceControl(this);
			iOwnsDeviceControl = EFalse;
			}
		iController=NULL;
		DestroyEp0();
		if (iStatusFifo!=NULL)
			{
			delete iStatusFifo;
			}
		}
	__KTRACE_OPT(KUSB, Kern::Printf("Closing buffers"));
	if (iBuffers)
		{
		TInt i;
		for (i=0; i<(iNumBuffers+2); i++) 
			{
			iBuffers[i].Destroy();
			}
		Kern::Free(iBuffers);
		}

	if (iRealizeCalled)
		{
		// Close Chunk
		iChunkInfo->Close();
		// ChunkInfo will delete itself with DFC, but the pointer here is no longer needed.		
		iChunkInfo=NULL;
		}
	__KTRACE_OPT(KUSB, Kern::Printf("about to SafeClose"));
	Kern::SafeClose((DObject*&)iClient, NULL);
	}


//
// DoCreate - Create channel
//

TInt DLddUsbcScChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("LDD DoCreateL 1 Ver = %02d %02d %02d",
									aVer.iMajor, aVer.iMinor, aVer.iBuild));
	if (!Kern::CurrentThreadHasCapability(ECapabilityCommDD,
										  __PLATSEC_DIAGNOSTIC_STRING("Checked by USBCSC.LDD (USB Driver)")))
		{
		return KErrPermissionDenied;
		}

	iController = DUsbClientController::UsbcControllerPointer();

	if (!iController)
		{
		return KErrGeneral;
		}

	iStatusFifo = new TUsbcDeviceStatusQueue;
	if (iStatusFifo == NULL)
		{
		return KErrNoMemory;
		}

  	if (!Kern::QueryVersionSupported(TVersion(KUsbcScMajorVersion, KUsbcScMinorVersion, KUsbcScBuildVersion), aVer))
		{
		return KErrNotSupported;
		}

	// set up the correct DFC queue
	SetDfcQ(iController->DfcQ(0));							// sets the channel's dfc queue
    iCompleteAllCallbackInfo.SetDfcQ(iDfcQ);
	iStatusCallbackInfo.SetDfcQ(iDfcQ);						// use the channel's dfcq for this dfc
	iEndpointStatusCallbackInfo.SetDfcQ(iDfcQ);				// use the channel's dfcq for this dfc
	iOtgFeatureCallbackInfo.SetDfcQ(iDfcQ);
	iMsgQ.Receive();										//start up the message q
	TInt r = iController->RegisterClientCallback(iCompleteAllCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForStatusChange(iStatusCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForEndpointStatusChange(iEndpointStatusCallbackInfo);
	if (r != KErrNone)
		return r;
	r = iController->RegisterForOtgFeatureChange(iOtgFeatureCallbackInfo);
	if (r != KErrNone)
		return r;

	return r;
	}
// end DoCreate.


//
// HandleMsg
//
// Events from userside arrive here, and delegated to either DoRequest, DoControl or DoCancel.
//

void DLddUsbcScChannel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	__KTRACE_OPT(KUSB, Kern::Printf("HandleMsg 0x%x", id));

	if (id == (TInt) ECloseMsg)
		{
		iChannelClosing = ETrue;
		m.Complete(KErrNone, EFalse);
		return;
		}

	TInt r;
	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*) m.Ptr0();
		r = DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		m.Complete(r, ETrue);
		}
	else if (id & RDevUsbcScClient::ERequestCancel)
		{
		// DoCancel
		r = DoCancel(id, (TUint) m.Ptr0(), (TUint) m.Ptr1());
		m.Complete(r, ETrue);
	}
	else
		{
		// DoControl
		r = DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(r, ETrue);
		}
	}
// end HandleMsg.


#define BREAK_IF_NULL_ARG(a,r) if (a==NULL) { r = KErrArgument; __KTRACE_OPT(KUSB,Kern::Printf("NULL Argument")); break; }

//
// DoRequest - Asynchronous requests
//
// Overrides pure virtual, called by HandleMsg. (Above)
//
TInt DLddUsbcScChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	TInt reqNo = aReqNo & RDevUsbcScClient::KFieldIdMask;
	TInt r = KErrNone;  // return via request notify
	TBool needsCompletion =EFalse;

	__KTRACE_OPT(KUSB, Kern::Printf("DoRequest 0x%08x", aReqNo));

	if ((reqNo>RDevUsbcScClient::ERequestReadDataNotify) &&
		(reqNo<RDevUsbcScClient::ERequestMaxRequests))
		{
		if (iRequestStatus[reqNo])
			{
			PanicClientThread(ERequestAlreadyPending);
			return 0;
			}
		iRequestStatus[reqNo] = aStatus;
		}

	switch (reqNo)
		{
	case RDevUsbcScClient::ERequestWriteData:
		{
		TInt buffer =  (aReqNo>>RDevUsbcScClient::KFieldBuffPos)&RDevUsbcScClient::KFieldBuffMask;
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestWriteData"));
		BREAK_IF_NULL_ARG(a2,r);

		r = DoWriteData( aStatus, buffer, (TInt) a1 /*Start*/, (TInt) a2 /* Length */,
						 aReqNo>>RDevUsbcScClient::KFieldFlagsPos ); // Flags
		break;
		}
	case RDevUsbcScClient::ERequestReadDataNotify:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestReadDataNotify"));
		return DoReadDataNotify(aStatus, (TInt) a1, (TInt) a2); // a1 = aBufferNumber, a2 - aLength;
		} 

	case RDevUsbcScClient::ERequestAlternateDeviceStatusNotify:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestAlternateDeviceStatusNotify"));
		BREAK_IF_NULL_ARG(a1,r);
		iDeviceStatusNeeded = ETrue;
		iStatusChangePtr = a1;
		needsCompletion = AlternateDeviceStateTestComplete();
		break;
		}
	case RDevUsbcScClient::ERequestReEnumerate:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestReEnumerate"));
		// If successful, this will complete via the status notification.
		r = iController->ReEnumerate();
		break;
		}
	case RDevUsbcScClient::ERequestEndpointStatusNotify:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestEndpointStatusNotify"));
		BREAK_IF_NULL_ARG(a1,r);
		
		iEndpointStatusChangePtr = a1;
		break;
		}
	case RDevUsbcScClient::ERequestOtgFeaturesNotify:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("ERequestOtgFeaturesNotify"));
		BREAK_IF_NULL_ARG(a1,r);
			
		iOtgFeatureChangePtr = a1;
		break;
		}
    default:
		r = KErrNotSupported;
		}

	if ((needsCompletion) || (r != KErrNone))
		{
		iRequestStatus[reqNo] = aStatus;
		Kern::RequestComplete(iClient, iRequestStatus[reqNo], r);
		}
	return KErrNone;
	}
// end DoRequest.


//
// DoReadDataNotify
//
// This method sets up the request to facilitate the userside being notifed when new data has been read.
//
TInt DLddUsbcScChannel::DoReadDataNotify(TRequestStatus* aStatus, TInt aBufferNum, TInt aLength)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoReadDataNotify(x, %d, 0x%x)", aBufferNum, aLength));
	TInt r = KErrNone;
	// check range
	if ((aBufferNum<0) ||  (aBufferNum>=iNumBuffers))  // Indirectly checks that we are set up.
		{
		if (aBufferNum!=KUsbcScEndpointZero)
			{
	        __KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoReadDataNotify : Bad Buffer Number!"));
			return KErrArgument;
			}
		else
			{
			aBufferNum = iEP0OutBuff;
			}
		}
	else
		{
		// check direction
		if (iBuffers[aBufferNum].iDirection!=KUsbcScOut)
			{
   		     __KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoReadDataNotify : Bad Buffer Direction!"));
			return KErrNotSupported;
			}
		if (!Configured())
			return KErrUsbInterfaceNotReady;
		}
	SUsbcScBufferHeader* scBuffer = (SUsbcScBufferHeader*) iBuffers[aBufferNum].iBufferStart;

	__KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoReadDataNotify  head %x tail %x", iBuffers[aBufferNum].iHead , scBuffer->iTail ));

	if (iBuffers[aBufferNum].iHead != scBuffer->iBilTail)
		r = KErrCompletion;
	else
		if (iBuffers[aBufferNum].iStalled)
			{
			iBuffers[aBufferNum].PopStall();
			return KErrCompletion;
			}
		else
			r = iBuffers[aBufferNum].iStatusList.Add(aStatus, aLength, 0,0);

	if (iBuffers[aBufferNum].iStatusList.iState==ENotRunning)
		{
		iBuffers[aBufferNum].StartDataRead();
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Job in Progress!"));
		}
	return r;
	}
// end DoReadDataNotify.



//
// DoWriteData
//
// This method sets up the request to write data to USB from userside.
//
TInt DLddUsbcScChannel::DoWriteData(TRequestStatus* aStatus,TInt aBufferNum, TUint aStart, TUint aLength, TUint aFlags)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoWriteData(%d, 0x%x, 0x%x, 0x%x)",  aBufferNum, aStart, aLength, aFlags));
	if (!iUserKnowsAltSetting)
		return KErrEof;
	// Check Buffer Number
	if ((aBufferNum<0) ||  (aBufferNum>=iNumBuffers))
		{
		if ((TUint)aBufferNum!=RDevUsbcScClient::KFieldBuffMask)  // KUsbcScEndpointZero & KFieldBuffMas = KFieldBuffMas;
			{
	        __KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoWriteData : Bad Buffer Number!"));
			return KErrArgument;
			}
		else
			{
			aBufferNum = iEP0InBuff;
			}
		}
	else
		{
		// check direction
		if (iBuffers[aBufferNum].iDirection!=KUsbcScIn)
			{
	    	    __KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoWriteData Bad endpoint Direction"));
				return KErrArgument;
			}
		}

	TUsbcScBuffer& buf=iBuffers[aBufferNum];

	if ((aStart< (((TLinAddr) buf.iBufferStart)-buf.iChunkAddr)) || ((aStart+aLength)>iBuffers[aBufferNum].iBufferEnd))
		{
		__KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DoWriteData Bad Range aStart or aLength 0x%x > 0x%x + 0x%x < 0x%x", (((TLinAddr) buf.iBufferStart)-buf.iChunkAddr),aStart, aLength, iBuffers[aBufferNum].iBufferEnd ));
		return KErrArgument;
		}

	if ( (aBufferNum != iEP0InBuff) && !Configured())
		return KErrUsbInterfaceNotReady;

	if (aStart & ~buf.iAlignMask)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScBuffer::DoDataWrite: address 0x%x unaligned.",aStart));
		return KErrArgument;
		}
			
	TInt r = iBuffers[aBufferNum].iStatusList.Add(aStatus, aLength, aStart, aFlags); //update

	if (iBuffers[aBufferNum].iStatusList.iState==ENotRunning)
		{
			iBuffers[aBufferNum].StartDataWrite();
		}
	else
		{	
		__KTRACE_OPT(KUSB, Kern::Printf("Job in Progress!"));
		}


	return r;
	}
// end DoWriteData.


//
// Cancel an outstanding request						// Cancel need reworking.
//
TInt DLddUsbcScChannel::DoCancel(TInt aReqNo, TUint aBuff, TUint aSpair)
	{
	TInt r = KErrNone;
	TInt direction=KUsbcScOut;

	__KTRACE_OPT(KUSB, Kern::Printf("DoCancel: 0x%x aBuff 0x%x", aReqNo, aBuff));
	switch (aReqNo)
		{
	case RDevUsbcScClient::ERequestCancel:
		TInt buffer;
		TInt mask;

		for (buffer=1, mask=1; buffer<iNumBuffers; buffer++,mask<<=1)
			if (aBuff&mask)
				iBuffers[buffer].Cancel(KErrCancel);

		return KErrNone;

	// coverity[missing_break]
	case RDevUsbcScClient::ERequestWriteDataCancel:
		direction = KUsbcScIn;
	case RDevUsbcScClient::ERequestReadDataNotifyCancel:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel Direction %d endpoints: 0x%x",direction, aReqNo));

		if (((TInt)aBuff)==KUsbcScEndpointZero) // EP0 is bi-directional, so pick correct buffer for call type
			{
			__KTRACE_OPT(KUSB, Kern::Printf("DoCancel Cancel Endpoint 0/%d",direction));
			iEp0Endpoint->AbortTransfer();
			if (direction==KUsbcScIn)
				aBuff=iEP0InBuff;
			else
				aBuff=iEP0OutBuff;
			} 
		else if ((TInt)aBuff >= iNumBuffers) // check buff no range.
			{
			__KTRACE_OPT(KUSB, Kern::Printf("DoCancel Error: Bad buffer number"));
			return KErrArgument;
			}

		if ((iBuffers[aBuff].iDirection&1)!=direction) // Does direction match call type?
			{
			__KTRACE_OPT(KUSB, Kern::Printf("DoCancel Error: Bad buffer direction"));
			return KErrArgument;
			}	
		iBuffers[aBuff].iStatusList.CancelQueued();
		iBuffers[aBuff].Cancel(KErrCancel);
		
		return KErrNone;

	case RDevUsbcScClient::ERequestAlternateDeviceStatusNotifyCancel:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel: ERequestAlternateDeviceStatusNotify 0x%x", aReqNo));
		iDeviceStatusNeeded = EFalse;
		iStatusFifo->FlushQueue();
		if (iStatusChangePtr)
			{
			TInt deviceState = iController->GetDeviceStatus();
			r = Kern::ThreadRawWrite(iClient, iStatusChangePtr, &deviceState, sizeof(deviceState), iClient);
			if (r != KErrNone)
				PanicClientThread(r);
			iStatusChangePtr = NULL; 
			}
	break;

	case RDevUsbcScClient::ERequestReEnumerateCancel:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel ERequestReEnumerate: 0x%x", aReqNo));
	break;

	case RDevUsbcScClient::ERequestEndpointStatusNotifyCancel:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel ERequestEndpointStatusNotify: 0x%x", aReqNo));
		CancelNotifyEndpointStatus();
	break;

 	case RDevUsbcScClient::ERequestOtgFeaturesNotifyCancel:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel ERequestOtgFeaturesNotify: 0x%x", aReqNo));
		CancelNotifyOtgFeatures();
	break;

	default:
		__KTRACE_OPT(KUSB, Kern::Printf("DoCancel Unknown! 0x%x", aReqNo));
		return KErrArgument;
		}

	Kern::RequestComplete(iClient,iRequestStatus[aReqNo & ~RDevUsbcScClient::ERequestCancel], KErrCancel);
	return r;
	}


void DLddUsbcScChannel::CancelNotifyEndpointStatus()
	{
	if (iEndpointStatusChangePtr)
		{
		TUint epBitmap = 0;
		for (TInt i = 1; i <= iNumberOfEndpoints; i++)
			{
			TInt v = iController->GetEndpointStatus(this, iEndpoint[i]->RealEpNumber());
			TUint b;
			(v == EEndpointStateStalled) ? b = 1 : b = 0;
			epBitmap |= b << i;
			}
		TInt r=Kern::ThreadRawWrite(iClient, iEndpointStatusChangePtr, (TUint8*) &epBitmap, sizeof(epBitmap), iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		iEndpointStatusChangePtr = NULL;
		}
	}

void DLddUsbcScChannel::CancelNotifyOtgFeatures()
	{
    if (iOtgFeatureChangePtr)
        {
        TUint8 features;
        iController->GetCurrentOtgFeatures(features);
		TInt r=Kern::ThreadRawWrite(iClient, iOtgFeatureChangePtr, (TUint8*)&features, sizeof(features), iClient);
		if (r != KErrNone)
			PanicClientThread(r);
        iOtgFeatureChangePtr = NULL;
        }
    }



//
// DoControl - Synchronous requests
//
// Called from HandleMsg.

TInt DLddUsbcScChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DoControl: %d", aFunction));

	TInt r = KErrNone;
	TInt ep, param;
	TUsbcScEndpoint* pEndpoint;
	TPtrC8 pZeroDesc(NULL, 0);
	TEndpointDescriptorInfo epInfo;
	TUsbcScIfcInfo ifcInfo;
	TCSDescriptorInfo desInfo;
	TUsbcEndpointResource epRes;

	switch (aFunction)
		{
	case RDevUsbcScClient::EControlEndpointZeroRequestError:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlEndpointZeroRequestError"));
		r = KErrNone;
		if (iOwnsDeviceControl || (iValidInterface && iDeviceState == EUsbcDeviceStateConfigured))
			{
			iController->Ep0Stall(this);
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcScClient::EControlGetAlternateSetting:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetAlternateSetting"));
		if (iValidInterface && iDeviceState == EUsbcDeviceStateConfigured)
			{
			r = iController->GetInterfaceNumber(this, param);
			if (r == KErrNone)
				{
				r = Kern::ThreadRawWrite(iClient, a1, &param, sizeof(param), iClient);
				if (r != KErrNone)
					PanicClientThread(r);
				}
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcScClient::EControlDeviceStatus:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDeviceStatus"));
		param = iController->GetDeviceStatus();
		r = Kern::ThreadRawWrite(iClient, a1, &param, sizeof(param), iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		break;

	case RDevUsbcScClient::EControlEndpointStatus:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlEndpointStatus"));
		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			pEndpoint = iEndpoint[(TInt)a1];
			if (pEndpoint == NULL)
				r = KErrNotSupported;
			else
				{
				param = iController->GetEndpointStatus(this, iEndpoint[(TInt)a1]->RealEpNumber());
				r = Kern::ThreadRawWrite(iClient, a2, &param, sizeof(param), iClient);
				if (r != KErrNone)
					PanicClientThread(r);
				}
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcScClient::EControlEndpointCaps:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlEndpointCaps"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		iController->EndpointCaps(this, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlDeviceCaps:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDeviceCaps"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		iController->DeviceCaps(this, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlSendEp0StatusPacket:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSendEp0StatusPacket"));
		iController->SendEp0StatusPacket(this);
		break;

	case RDevUsbcScClient::EControlHaltEndpoint:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlHaltEndpoint"));
		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			r = iController->HaltEndpoint(this, iEndpoint[(TInt)a1]->RealEpNumber());
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcScClient::EControlClearHaltEndpoint:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlClearHaltEndpoint"));
		if (iValidInterface && ValidEndpoint((TInt) a1))
			{
			r = iController->ClearHaltEndpoint(this, iEndpoint[(TInt)a1]->RealEpNumber());
			}
		else
			{
			if (iDeviceState != EUsbcDeviceStateConfigured)
				r = KErrUsbDeviceNotConfigured;
			else
				r = KErrUsbInterfaceNotReady;
			}
		break;

	case RDevUsbcScClient::EControlDumpRegisters:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDumpRegisters"));
		iController->DumpRegisters();
		break;

	case RDevUsbcScClient::EControlReleaseDeviceControl:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlReleaseDeviceControl"));
		iController->ReleaseDeviceControl(this);
		iOwnsDeviceControl = EFalse;
		break;

	case RDevUsbcScClient::EControlEndpointZeroMaxPacketSizes:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlEndpointZeroMaxPacketSizes"));
		r = iController->EndpointZeroMaxPacketSizes();
		break;

	case RDevUsbcScClient::EControlSetEndpointZeroMaxPacketSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetEndpointZeroMaxPacketSize"));
		r = iController->SetEndpointZeroMaxPacketSize(reinterpret_cast<TInt>(a1));
		break;

	case RDevUsbcScClient::EControlGetEndpointZeroMaxPacketSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetEndpointZeroMaxPacketSize"));
		r = iController->Ep0PacketSize();
		break;

	case RDevUsbcScClient::EControlGetDeviceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetDeviceDescriptor"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetDeviceDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlSetDeviceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetDeviceDescriptor"));
		BREAK_IF_NULL_ARG(a1,r);
		r = iController->SetDeviceDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlGetDeviceDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetDeviceDescriptorSize"));
		BREAK_IF_NULL_ARG(a1,r);
		r = iController->GetDeviceDescriptorSize(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlGetConfigurationDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetConfigurationDescriptor"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0 , 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlGetConfigurationDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetConfigurationDescriptorSize"));
		if (a1 != NULL)
			{
			r = iController->GetConfigurationDescriptorSize(iClient, *((TDes8*) a1));
			}
		else
			r = KErrArgument;
		break;

	case RDevUsbcScClient::EControlSetConfigurationDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetConfigurationDescriptor"));
		r = iController->SetConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlGetInterfaceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetInterfaceDescriptor"));
		r = iController->GetInterfaceDescriptor(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcScClient::EControlGetInterfaceDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetInterfaceDescriptorSize"));
		r = iController->GetInterfaceDescriptorSize(iClient, this, (TInt) a1, *(TDes8*) a2);
		break;

	case RDevUsbcScClient::EControlSetInterfaceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetInterfaceDescriptor"));
		r = iController->SetInterfaceDescriptor(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcScClient::EControlGetEndpointDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetEndpointDescriptor"));
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = (ep<0)?ep:iController->GetEndpointDescriptor(iClient, this, epInfo.iSetting,
											   ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcScClient::EControlGetEndpointDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetEndpointDescriptorSize"));
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetEndpointDescriptorSize(iClient, this, epInfo.iSetting,
												   ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcScClient::EControlSetEndpointDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetEndpointDescriptor"));
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->SetEndpointDescriptor(iClient, this, epInfo.iSetting,
											   ep, *(TDes8*)epInfo.iArg);
		break;

	case RDevUsbcScClient::EControlGetDeviceQualifierDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetDeviceQualifierDescriptor"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0, 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetDeviceQualifierDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlSetDeviceQualifierDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetDeviceQualifierDescriptor"));
		BREAK_IF_NULL_ARG(a1,r);
		r = iController->SetDeviceQualifierDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlGetOtherSpeedConfigurationDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetOtherSpeedConfigurationDescriptor"));
		r = Kern::ThreadDesWrite(iClient, a1, pZeroDesc, 0 , 0, iClient);
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->GetOtherSpeedConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlSetOtherSpeedConfigurationDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetOtherSpeedConfigurationDescriptor"));
		r = iController->SetOtherSpeedConfigurationDescriptor(iClient, *((TDes8*) a1));
		break;


	case RDevUsbcScClient::EControlGetCSInterfaceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetCSInterfaceDescriptor"));
		r = iController->GetCSInterfaceDescriptorBlock(iClient, this, (TInt) a1, *((TDes8*) a2));
		break;

	case RDevUsbcScClient::EControlGetCSInterfaceDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetCSInterfaceDescriptorSize"));
		r = iController->GetCSInterfaceDescriptorBlockSize(iClient, this, (TInt) a1, *(TDes8*) a2);
		break;

	case RDevUsbcScClient::EControlGetCSEndpointDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetCSEndpointDescriptor"));
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetCSEndpointDescriptorBlock(iClient, this, epInfo.iSetting,
													  ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcScClient::EControlGetCSEndpointDescriptorSize:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetCSEndpointDescriptorSize"));
		r = Kern::ThreadRawRead(iClient, a1, &epInfo, sizeof(epInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(epInfo.iSetting, epInfo.iEndpoint);
		r = iController->GetCSEndpointDescriptorBlockSize(iClient, this, epInfo.iSetting,
														  ep, *(TDes8*) epInfo.iArg);
		break;

	case RDevUsbcScClient::EControlSignalRemoteWakeup:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSignalRemoteWakeup"));
		r = iController->SignalRemoteWakeup();
		break;

	case RDevUsbcScClient::EControlDeviceDisconnectFromHost:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDeviceDisconnectFromHost"));
		r = iController->UsbDisconnect();
		break;

	case RDevUsbcScClient::EControlDeviceConnectToHost:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDeviceConnectToHost"));
		r = iController->UsbConnect();
		break;

	case RDevUsbcScClient::EControlDevicePowerUpUdc:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDevicePowerUpUdc"));
		r = iController->PowerUpUdc();
		break;

	case RDevUsbcScClient::EControlSetDeviceControl:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetDeviceControl"));
		r = iController->SetDeviceControl(this);
		if (r == KErrNone)
			{
			iOwnsDeviceControl = ETrue;
			if (iEp0Endpoint == NULL)
				{
				__KTRACE_OPT(KUSB, Kern::Printf("EControlSetDeviceControl"));
				r = SetupEp0();
				if (r != KErrNone)
					{
					__KTRACE_OPT(KPANIC, Kern::Printf("  Error: SetupEp0() failed"));
					iController->ReleaseDeviceControl(this);
					iOwnsDeviceControl=EFalse;
					DestroyEp0();
					}
				}
			}
		else
			r = KErrInUse;
		break;

	case RDevUsbcScClient::EControlCurrentlyUsingHighSpeed:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlCurrentlyUsingHighSpeed"));
		r = iController->CurrentlyUsingHighSpeed();
		break;

	case RDevUsbcScClient::EControlSetInterface:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetInterface"));
		r = Kern::ThreadRawRead(iClient, a2, &ifcInfo, sizeof(ifcInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		r = SetInterface((TInt) a1, &ifcInfo);
		break;

	case RDevUsbcScClient::EControlReleaseInterface: 
		__KTRACE_OPT(KUSB, Kern::Printf("EControlReleaseInterface"));
		if (!iRealizeCalled)
			{
			r = iController->ReleaseInterface(this, (TInt) a1);
			if (r == KErrNone)
				{
				DestroyInterface((TUint) a1);
				}
			else
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error in PIL: LDD interface won't be released."));
				}
			}
		else
			r = KErrUsbAlreadyRealized;
		break;

	case RDevUsbcScClient::EControlSetCSInterfaceDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetCSInterfaceDescriptor"));
		r = Kern::ThreadRawRead(iClient, a1, &desInfo, sizeof(desInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		r = iController->SetCSInterfaceDescriptorBlock(iClient, this, desInfo.iSetting,
													   *reinterpret_cast<const TDes8*>(desInfo.iArg),
													   desInfo.iSize);
		break;

	case RDevUsbcScClient::EControlSetCSEndpointDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetCSEndpointDescriptor"));
		r = Kern::ThreadRawRead(iClient, a1, &desInfo, sizeof(desInfo));
		if (r != KErrNone)
			PanicClientThread(r);
		ep = EpFromAlternateSetting(desInfo.iSetting, desInfo.iEndpoint);
		r = iController->SetCSEndpointDescriptorBlock(iClient, this, desInfo.iSetting, ep,
													  *reinterpret_cast<const TDes8*>(desInfo.iArg),
													  desInfo.iSize);
		break;

	case RDevUsbcScClient::EControlGetStringDescriptorLangId:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetStringDescriptorLangId"));
		r = iController->GetStringDescriptorLangId(iClient, *((TDes8*) a1));
		break;

	case RDevUsbcScClient::EControlSetStringDescriptorLangId:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetStringDescriptorLangId"));
		r = iController->SetStringDescriptorLangId(reinterpret_cast<TUint>(a1));
		break;

	case RDevUsbcScClient::EControlGetManufacturerStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetManufacturerStringDescriptor"));
		r = iController->GetManufacturerStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlSetManufacturerStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetManufacturerStringDescriptor"));
		r = iController->SetManufacturerStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlRemoveManufacturerStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRemoveManufacturerStringDescriptor"));
		r = iController->RemoveManufacturerStringDescriptor();
		break;

	case RDevUsbcScClient::EControlGetProductStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetProductStringDescriptor"));
		r = iController->GetProductStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlSetProductStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetProductStringDescriptor"));
		r = iController->SetProductStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlRemoveProductStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRemoveProductStringDescriptor"));
		r = iController->RemoveProductStringDescriptor();
		break;

	case RDevUsbcScClient::EControlGetSerialNumberStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetSerialNumberStringDescriptor"));
		r = iController->GetSerialNumberStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlSetSerialNumberStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetSerialNumberStringDescriptor"));
		r = iController->SetSerialNumberStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlRemoveSerialNumberStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRemoveSerialNumberStringDescriptor"));
		r = iController->RemoveSerialNumberStringDescriptor();
		break;

	case RDevUsbcScClient::EControlGetConfigurationStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetConfigurationStringDescriptor"));
		r = iController->GetConfigurationStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlSetConfigurationStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetConfigurationStringDescriptor"));
		r = iController->SetConfigurationStringDescriptor(iClient, *((TPtr8*) a1));
		break;

	case RDevUsbcScClient::EControlRemoveConfigurationStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRemoveConfigurationStringDescriptor"));
		r = iController->RemoveConfigurationStringDescriptor();
		break;

	case RDevUsbcScClient::EControlGetStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetStringDescriptor"));
		r = iController->GetStringDescriptor(iClient, (TUint8) (TInt) a1, *((TPtr8*) a2));
		break;

	case RDevUsbcScClient::EControlSetStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetStringDescriptor"));
		r = iController->SetStringDescriptor(iClient, (TUint8) (TInt) a1, *((TPtr8*) a2));
		break;

	case RDevUsbcScClient::EControlRemoveStringDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRemoveStringDescriptor"));
		r = iController->RemoveStringDescriptor((TUint8) (TInt) a1);
		break;

	case RDevUsbcScClient::EControlAllocateEndpointResource:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("EControlAllocateEndpointResource"));
		epRes = (TUsbcEndpointResource)((TInt) a2);
		TInt realEp=-1;
		r = GetRealEpForEpResource((TInt)a1, realEp);
		if (r==KErrNone)
			r = iController->AllocateEndpointResource(this, realEp, epRes);
		break;
		}
	case RDevUsbcScClient::EControlDeAllocateEndpointResource:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("EControlDeAllocateEndpointResource"));
		epRes = (TUsbcEndpointResource)((TInt) a2);
		TInt realEp=-1;
		r = GetRealEpForEpResource((TInt)a1, realEp);
		if (r==KErrNone)
			r = iController->DeAllocateEndpointResource(this, realEp, epRes);
		break;
		}
	case RDevUsbcScClient::EControlQueryEndpointResourceUse:
		{
		__KTRACE_OPT(KUSB, Kern::Printf("EControlQueryEndpointResourceUse"));
		epRes = (TUsbcEndpointResource)((TInt) a2);
		TInt realEp=-1;
		r = GetRealEpForEpResource((TInt)a1, realEp);
		if (r==KErrNone)
			r = iController->QueryEndpointResource(this, realEp, epRes);
		break;
		}
	case RDevUsbcScClient::EControlSetOtgDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlSetOtgDescriptor"));
		r = iController->SetOtgDescriptor(iClient, *((const TDesC8*)a1));
		break;

	case RDevUsbcScClient::EControlGetOtgDescriptor:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetOtgDescriptor"));
		r = iController->GetOtgDescriptor(iClient, *((TDes8*)a1));
		break;

	case RDevUsbcScClient::EControlGetOtgFeatures:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlGetOtgFeatures"));
		r = iController->GetOtgFeatures(iClient, *((TDes8*)a1));
		break;

	case RDevUsbcScClient::EControlRealizeInterface:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlRealizeInterface"));
		r = RealizeInterface();
		break;
	case RDevUsbcScClient::EControlStartNextInAlternateSetting:
		__KTRACE_OPT(KUSB, Kern::Printf("EControlStartNextInAlternateSetting"));
		r = StartNextInAlternateSetting();
		break;

    default:
		__KTRACE_OPT(KUSB, Kern::Printf("Function code not supported"));
		r = KErrNotSupported;
		}

	return r;
	}
// end DoControl.



//
// Overriding DObject virtual
//
TInt DLddUsbcScChannel::RequestUserHandle(DThread* aThread, TOwnerType /*aType*/)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::RequestUserHandle"));
	// The USB client LDD is not designed for a channel to be shared between
	// threads. It saves a pointer to the current thread when it is opened, and
	// uses this to complete any asynchronous requests.
	// It is therefore not acceptable for the handle to be duplicated and used
	// by another thread:
	if (aThread == iClient)
		{
		return KErrNone;
		}
	else
		{
		return KErrAccessDenied;
		}
	}

inline TInt DLddUsbcScChannel::GetRealEpForEpResource(TInt aEndpoint, TInt& aRealEp)
	{
	if (iEndpoint) // if we've enumerated at least once, proceed as normal.
		{
		if  (aEndpoint <= iNumberOfEndpoints && aEndpoint >= 0)
			{
			aRealEp=iEndpoint[aEndpoint]->RealEpNumber();
			return KErrNone;
			}
		}
	else // Assume alternate setting 0.
		{
		if (iAlternateSettingList)   // Check it has been set up.
			{
			TUsbcScAlternateSetting* alt = iAlternateSettingList->iHead;
			if (alt &&  (aEndpoint <= alt->iNumberOfEndpoints && aEndpoint >= 0))
				{
				aRealEp= alt->iEndpoint[aEndpoint]->RealEpNumber();
				return KErrNone;
				}
			}
		}
	return KErrUsbDeviceNotConfigured;
	}


TUsbcEndpointInfoArray::TUsbcEndpointInfoArray(const TUsbcScEndpointInfo* aData, TInt aDataSize)
	{
	iType = EUsbcScEndpointInfo;
	iData = (TUint8*) aData;	
	if (aDataSize>0)
		iDataSize = aDataSize;
	else
		iDataSize = sizeof(TUsbcScEndpointInfo);
	}


//
// SetInterface
//
// Called from DoControl.  Sets the configuration of a given Interface.					// Needs changing
// All interfaces must be configured before one can be used.  
//

TInt DLddUsbcScChannel::SetInterface(TInt aInterfaceNumber, TUsbcScIfcInfo* aInfoBuf)
	{
	// Copy interface description.

	if (iRealizeCalled)
		return KErrUsbAlreadyRealized;

	if (!iAlternateSettingList)
		{
		iAlternateSettingList = new TUsbcScAlternateSettingList;
		if (iAlternateSettingList==NULL)
			{
			return KErrNoMemory;
			}
		}

	// Read descriptor in
	TUsbcScInterfaceInfoBuf ifc_info_buf;
	TUsbcScInterfaceInfoBuf* const ifc_info_buf_ptr = aInfoBuf->iInterfaceData;
	const TInt srcLen = Kern::ThreadGetDesLength(iClient, ifc_info_buf_ptr);

	__KTRACE_OPT(KUSB, Kern::Printf("SetInterface srcLen = %d len = %d", srcLen, ifc_info_buf.Length() ));

	if (srcLen < ifc_info_buf.Length())
		{
		__KTRACE_OPT(KUSB, Kern::Printf("SetInterface can't copy"));
		PanicClientThread(EDesOverflow);
		}

	TInt r = Kern::ThreadDesRead(iClient, ifc_info_buf_ptr, ifc_info_buf, 0, KChunkShiftBy0);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("SetInterface Copy failed reason=%d", r));
		PanicClientThread(r);
		}

	// The list of endpoints is within the interface info.
	TUsbcScEndpointInfo* pEndpointData = ifc_info_buf().iEndpointData;

	const TInt num_endpoints = ifc_info_buf().iTotalEndpointsUsed;
	__KTRACE_OPT(KUSB, Kern::Printf("SetInterface num_endpoints=%d", num_endpoints));
	if (num_endpoints>KMaxEndpointsPerClient)
		return KErrOverflow;


	// Initialize real ep numbers list.
	TInt i;
	TInt real_ep_numbers[KMaxEndpointsPerClient+1]; // range 1->KMaxEndpointsPerClient (0 not used)
	for (i=0; i<=KMaxEndpointsPerClient; i++)
		real_ep_numbers[i] = -1;


	// See if PIL will accept this interface
	__KTRACE_OPT(KUSB, Kern::Printf("SetInterface Calling controller"));
	TUsbcEndpointInfoArray endpointData = TUsbcEndpointInfoArray(ifc_info_buf().iEndpointData);

	r = iController->SetInterface(this,
								  iClient,
								  aInterfaceNumber,
								  ifc_info_buf().iClass,
								  aInfoBuf->iString,
								  (TInt) ifc_info_buf().iTotalEndpointsUsed,
								  endpointData,
								  &real_ep_numbers[0],
								  ifc_info_buf().iFeatureWord);

	__KTRACE_OPT(KUSB, Kern::Printf("SetInterface controller returned %d", r));
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("SetInterface failed reason=%d", r));
		return r;
		}

	// create alternate setting record
    TUsbcScAlternateSetting* alternateSettingListRec = new TUsbcScAlternateSetting;
	if (!alternateSettingListRec)
		{
		r = KErrNoMemory;
		goto ReleaseInterface;
		}
	
	// other endpoints
	for (TInt i = 1; i <= num_endpoints; i++, pEndpointData++)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("SetInterface for ep=%d", i));

		if ((pEndpointData->iType==KUsbEpTypeControl)
			|| (pEndpointData->iDir != KUsbEpDirIn && pEndpointData->iDir != KUsbEpDirOut)
			|| (pEndpointData->iSize > 1024) || (pEndpointData->iSize<=0))
			{
			r = KErrUsbBadEndpoint;
			goto CleanUp;
			}
		// Check data

		TUint* bufferSize = &(pEndpointData->iBufferSize);
		if (*bufferSize==0)
			*bufferSize= KUsbcScDefaultBufferSize;

		TInt pageSize = Kern::RoundToPageSize(1);
		// Round buffersize up to nearest pagesize.
		*bufferSize = (*bufferSize+pageSize-1) & ~(pageSize-1);

		TUsbcScEndpoint* ep = new TUsbcScEndpoint(this, iController, pEndpointData, i);
		alternateSettingListRec->iEndpoint[i] = ep;
		if (!ep)
			{
			r = KErrNoMemory;
			goto CleanUp;
			}
		if (ep->Construct() != KErrNone)
			{
			r = KErrNoMemory;
			goto CleanUp;
			}

	
		__KTRACE_OPT(KUSB, Kern::Printf("SetInterface for ep=%d rec=0x%08x ep==0x%08x",
										i, alternateSettingListRec, ep));
		}

	if (iAlternateSettingList->iHead)
		{
		iAlternateSettingList->iTail->iNext = alternateSettingListRec;
		alternateSettingListRec->iPrevious = iAlternateSettingList->iTail;
		iAlternateSettingList->iTail = alternateSettingListRec;	
		}
	else
		{
		iAlternateSettingList->iHead = alternateSettingListRec;	
		iAlternateSettingList->iTail = alternateSettingListRec;	
		}	
	
	alternateSettingListRec->iNext = NULL;
	alternateSettingListRec->iSetting = aInterfaceNumber;
	alternateSettingListRec->iNumberOfEndpoints = num_endpoints;

	// Record the 'real' endpoint number used by the PDD in both the Ep and
	// the Req callback:
	for (TInt i = 1; i <= num_endpoints; i++)
		{
		alternateSettingListRec->iEndpoint[i]->SetRealEpNumber(real_ep_numbers[i]);
		}

	return KErrNone;

 CleanUp:
	delete alternateSettingListRec;
	//Fall Through

 ReleaseInterface:
#if _DEBUG
	TInt r1 = iController->ReleaseInterface(this, aInterfaceNumber);
	__KTRACE_OPT(KUSB, Kern::Printf("Release Interface controller returned %d", r1));
#else
	(void)	iController->ReleaseInterface(this, aInterfaceNumber);
#endif
	return r;
	}
// end SetInterface



#ifdef _DEBUG
void RealizeInterface_Dump(TUint* aMem)
	{
	TUint *mem= NULL;
	__KTRACE_OPT(KUSB, mem = aMem);
	if (mem!=NULL)
		{
		TInt j;
 		Kern::Printf("Final chunk header State:");
		for (j=0; j<30; j+=8)
 			Kern::Printf("%2x: %8x %8x %8x %8x %8x %8x %8x %8x", j, mem[j], mem[j+1], mem[j+2], mem[j+3], mem[j+4], mem[j+5], mem[j+6], mem[j+7] );
		};
	};
#endif


/*
Chunk Created, filled with structure, and passed back to userside.
*/
TInt DLddUsbcScChannel::RealizeInterface(void)
{
	if (iRealizeCalled) 
		return KErrUsbAlreadyRealized;

	TRealizeInfo bufInfo;
	
	TInt errorOrChunk = KErrNone;
	TBool openedCS = EFalse;
	TInt offset =0;
		
	// Start by creating a temporary scratchpad for endpoint calculations.
	bufInfo.Init(iAlternateSettingList);

	// Fill in our scratchpad with all the required endpoints, sorting them
	// in order of size required.
	errorOrChunk = bufInfo.CopyAndSortEndpoints();
	if (errorOrChunk!=KErrNone)
		{
		goto realize_end;
		}

	// We now have endpoints sorted in order of size for each altsetting.
	// The very largest for each endpoint will share the first buffer, and all of
	// the second largest ends points will share the second buffer, and so on.
	// Find the highest buffer size for each row, to determine the buffer size,
	// and keep a total of total space needed. 
	bufInfo.CalcBuffSizes();

	// We now have the max sizes wanted for each endpoint buffer.
	// we also have to total size for all endpoints.
	// and finally we have the total number of buffers.

	// Add on size for header, then add on size for guard pages.
	bufInfo.iTotalSize+= KHeaderSize + bufInfo.iTotalBuffers * KGuardSize;

	// Create shared Chunk .  .  .  .  .  .  .  .  .  . 
	if (iChunkInfo==NULL)
		{
			NKern::ThreadEnterCS();
			openedCS = ETrue;
			errorOrChunk = TUsbcScChunkInfo::New(iChunkInfo, bufInfo.iTotalSize, (DLogicalDevice*) iDevice);
			if (errorOrChunk!=KErrNone)
				{
				goto realize_end;
				}
		}
	else
		{
		// As of writing, the was no way for iChunk to be anything other then NULL.  
		// You cannot 'unrealise' and iChunk cannot be set any other way.
		Kern::Fault("DLddUsbcScChannel::RealizeInterface", __LINE__);
		}

	// Populate the shared chunk . .  . . . . . 


	// First create chunk header.
	errorOrChunk = iChunkInfo->ChunkAlloc(offset, KHeaderSize);
	if (errorOrChunk!=KErrNone)
		{
		if (errorOrChunk==-KErrNoMemory)
			errorOrChunk=KErrNoMemory;
		goto realize_end;
		} 


	offset+=KHeaderSize + KGuardSize; // Also any more for EP0?

	// Next, lay out the geometry of the chunk header.

	bufInfo.LayoutChunkHeader(iChunkInfo);		


	{ // Scope ep0Size
	TInt ep0Size=0;
	
	// Create K-side buffer table
	if (!iBuffers)
		iBuffers = (TUsbcScBuffer *) Kern::AllocZ(sizeof(TUsbcScBuffer) * (bufInfo.iTotalBuffers+2)); // +2 is for ep0.
	if (!iBuffers)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Realize: Error: Alloc iBufers failed!"));
		errorOrChunk = KErrNoMemory;
		goto realize_end;
		}


	errorOrChunk = SetupEp0();
	if (errorOrChunk)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Realize: SetupEp0 . ERROR %d",errorOrChunk));
		goto realize_end;
		}

	ep0Size = iEp0Endpoint->EndpointInfo()->iSize;
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: Setup EP0. max packet size %d", ep0Size));

	// Create EP0 buffers
	iEP0OutBuff=bufInfo.iTotalBuffers;
	errorOrChunk = iBuffers[iEP0OutBuff].Construct(KUsbcScBiOut,  this,   KUsbScEP0OutBufPos, KUsbScEP0OutBufEnd, ep0Size, ep0Size, ep0Size);
	if (errorOrChunk)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Realize: Setup EP0 Out. ERROR %d",errorOrChunk));
		goto realize_end;
		}

	iBuffers[iEP0OutBuff].CreateChunkBufferHeader();
	iBuffers[iEP0OutBuff].iCallback =  iEp0Endpoint->iRequestCallbackInfo;
	((TUsbcScBufferRecord*) &(
							bufInfo.iChunkStuct->iBufferOffset[KUsbcScEp0OutBuff*sizeof(TUsbcScBufferRecord)]
							)) ->Set(KUsbScEP0OutBufPos, KUsbScEP0OutBufEnd);


	iEP0InBuff=bufInfo.iTotalBuffers+1;
	errorOrChunk = iBuffers[iEP0InBuff].Construct( KUsbcScBiIn ,  this,   KUsbScEP0InBufPos , KUsbScEP0InBufEnd , ep0Size, ep0Size, ep0Size);	
	if (errorOrChunk)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Realize: Setup EP0 In. ERROR %d",errorOrChunk));
		goto realize_end;
		}
	
	iBuffers[iEP0InBuff].iCallback =  iEp0Endpoint->iRequestCallbackInfo;

	  ((TUsbcScBufferRecord*) &(
	  							bufInfo.iChunkStuct->iBufferOffset[KUsbcScEp0InBuff*sizeof(TUsbcScBufferRecord)]
								))->Set(KUsbScEP0InBufPos, KUsbScEP0InBufEnd);


	} // end ep0Size scope

	// Create resources and tables.  .   .   .   .   .
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: Create resources tables"));

	{ // scope of bufNum
	// For each EP buffer
	TInt buffNum=0;
	TInt buffMinSize;
	TInt endpointNumber;
	TUsbcScEndpoint* endpointRecord;
	TInt endpoint;
	TInt inout;
	TEndpointSortBufs* bufsd;
	TUsbcScHdrEndpointRecord* epRecord;
	for (endpoint=0; endpoint<bufInfo.iMaxEndpoints; endpoint++)  // endpoint = buf row.
		{
		for (inout=KUsbcScIn; inout<KUsbcScDirections; inout++)
			{
			buffMinSize = KUsbSc_BigBuff_MinimumRamRun;

			TInt needed =  bufInfo.iBufs[inout].iSizes[endpoint];
			if (needed) 
				{
				TInt bufStart = offset;

				__KTRACE_OPT(KUSB, Kern::Printf("Realize:    buf row:%d inout %d, iBufferOffset[%d+2]=%x",endpoint, inout, buffNum, bufStart));

				bufsd =  &(bufInfo.iBufs[inout]);
				// and then point all endpoints that use it, towards it.
				TInt altSetting;	
				TUint maxReadSize = ~0;
				for (altSetting=0; altSetting < bufInfo.iAltSettings; altSetting++)
					{
					endpointRecord =bufsd->iEp[altSetting*bufInfo.iMaxEndpoints + endpoint];
					if (endpointRecord)
						{
						endpointNumber = endpointRecord->EpNumber();
						endpointRecord->SetBuffer(&iBuffers[buffNum]);
				
						epRecord = (TUsbcScHdrEndpointRecord*) &iChunkInfo->iChunkMem[
																(bufInfo.iAltSettingsTbl->iAltTableOffset[altSetting]) 	// i.e. Just after altSettingsTbl
																+sizeof(TInt)									// after number of endpoints field
																+(endpointNumber-1)*sizeof(TUsbcScHdrEndpointRecord)
																];
						epRecord->iBufferNo = (TUint8) buffNum;

					TInt epType=(endpointRecord->EndpointInfo()->iType);
					epType= (epType& KUsbEpTypeControl)?KUsbScHdrEpTypeControl:
							(epType& KUsbEpTypeIsochronous)?KUsbScHdrEpTypeIsochronous:
							(epType& KUsbEpTypeBulk)?KUsbScHdrEpTypeBulk:
							(epType& KUsbEpTypeInterrupt)?KUsbScHdrEpTypeInterrupt:KUsbScHdrEpTypeUnknown;

					epRecord->iType = (inout+1) | (epType<<2);

					if (endpointRecord->EndpointInfo()->iReadSize)
						maxReadSize = (maxReadSize <= endpointRecord->EndpointInfo()->iReadSize) ? maxReadSize : endpointRecord->EndpointInfo()->iReadSize;
					
					__KTRACE_OPT(KUSB, Kern::Printf("Realize:      endpointNum %d in altSetting %d, alt table @ %d",
													 endpointNumber, altSetting,bufInfo.iAltSettingsTbl->iAltTableOffset[altSetting]));
						}
					else
						{
						__KTRACE_OPT(KUSB, Kern::Printf("Realize:      endpointNum NA in altSetting %d", altSetting));
						}

					} // end for


				// Alloc memory for buffer.
				TInt grabSize = needed;
				// Generally, a buffer fragmented into smaller memory regions will reduce the efficiency 
				// of reading or writing data, and so avoiding the allocation of very small sections
				// is advantageous.
				// However, if only a small amount is being allocated to start with, it is likely
				// smaller amounts of data are to be sent (reducing this advantage), and 1 memory page 
				// is a much bigger proportion of the buffer, and so more worth allocating individually.

				TInt minimumGrab;
				if (needed<KUsbScBigBuffIs)
					{
					minimumGrab=Kern::RoundToPageSize(1);
					buffMinSize = KUsbSc_SmallBuff_MinimumRamRun; // 1k
					}
				else
					{
					minimumGrab = buffMinSize+Kern::RoundToPageSize(1);
					}

				// Grab required memory, in bits as big as possible, down to the minimum size. 
				while (needed >= minimumGrab)
					{
					TInt r;
					r = iChunkInfo->ChunkAlloc(offset, grabSize);
					if (r==KErrNone)
						{
						offset+=grabSize;	
						needed-=grabSize;
						}
					else
						{
						if (r==-KErrNoMemory)
							{
							grabSize>>=1;
							}
						if ((grabSize<minimumGrab) || (r!=-KErrNoMemory))
							{
							errorOrChunk = r;
							goto realize_end;
							}
						}
					} // end while needed
				
				// Initialize buffer
				iBuffers[buffNum].Construct(inout,  this,   bufStart, offset, buffMinSize, 0, maxReadSize);
				iBuffers[buffNum].CreateChunkBufferHeader();
				((TUsbcScBufferRecord*) &(
										bufInfo.iChunkStuct->iBufferOffset[(buffNum+2)*sizeof(TUsbcScBufferRecord)]
										))->Set(bufStart, offset);


				// inc pointers for next buffer
				buffNum++;
				offset+=KGuardSize;
				} // end if needed

			} // end for inout
		} // end for each buffer
	} // scope of bufNum 

#ifdef _DEBUG
 RealizeInterface_Dump((TUint*) iChunkInfo->iChunkMem); // Debug only tracing
#endif

realize_end:
	__KTRACE_OPT(KUSB, Kern::Printf("Realize: cleanup.  Err=%d", errorOrChunk));
	// Here we clean up after either success, or after bailing out early.

	bufInfo.Free();
	
	if (iChunkInfo)
		{
		if (errorOrChunk==KErrNone)
			{ 
			// Everything is looking good - create RChunk for Userside.
			errorOrChunk = Kern::MakeHandleAndOpen(iClient, iChunkInfo->iChunk);
			iRealizeCalled = (errorOrChunk>=0);
			} // endif errorOrChunk

		if (errorOrChunk<0)  // If error, destroy the chunk.
			{
			iChunkInfo->Close();
			// ChunkInfo will delete itself with DFC, but the pointer here is no longer needed.
			iChunkInfo=NULL;

			// Destroy iBuffers
			if (iBuffers)
				{
				TInt i;
				for (i=0; i<(iNumBuffers+2); i++) 
					{
					iBuffers[i].iStatusList.Destroy();
					}
				Kern::Free(iBuffers);
				iBuffers=NULL;
				}

			}
		else
			{
			iNumBuffers = bufInfo.iTotalBuffers;
			iValidInterface = ETrue;  // Let the games commence!
			}

		} // endif iChunkInfo
	if (openedCS)
		NKern::ThreadLeaveCS();

	__KTRACE_OPT(KUSB, Kern::Printf("Realize: returning %x (%d)", errorOrChunk, errorOrChunk));
	return errorOrChunk;
} // End RealizeInterface


//
// DestroyAllInterfaces
//

void DLddUsbcScChannel::DestroyAllInterfaces()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DestroyAllInterfaces"));
	// Removes all interfaces
	if (iAlternateSettingList)
		{
		if (iAlternateSettingList->iHead != NULL)
			{
			TUsbcScAlternateSetting* alternateSettingListRec = iAlternateSettingList->iTail;
			while (alternateSettingListRec)
				{
				iAlternateSettingList->iTail = alternateSettingListRec->iPrevious; 
				// If this contains NULL now that is only possible if the record to be deleted was at the head
				__KTRACE_OPT(KUSB, Kern::Printf("Release interface %d \n", alternateSettingListRec->iSetting));
				iController->ReleaseInterface(this, alternateSettingListRec->iSetting);
				delete alternateSettingListRec;
				if (iAlternateSettingList->iTail == NULL) //No more interfaces left 
					break;
				else
					{
					iAlternateSettingList->iTail->iNext = NULL;
					alternateSettingListRec = iAlternateSettingList->iTail;
					}
				}
			}
		delete iAlternateSettingList;	
		}

	iNumberOfEndpoints = 0;
	iAlternateSettingList = NULL;
	iValidInterface = EFalse;

	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DestroyAllInterfaces done"));
	}


		


//
// DestroyInterface
//

void DLddUsbcScChannel::DestroyInterface(TUint aInterfaceNumber)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DestroyInterface \n"));
	
	if (iAlternateSetting == aInterfaceNumber)
		{
		ResetInterface(KErrUsbInterfaceNotReady);
		iValidInterface = EFalse;
		iNumberOfEndpoints = 0;
		}
	if (iAlternateSettingList)
		{
		TUsbcScAlternateSetting* alternateSettingListRec = iAlternateSettingList->iTail;
		TUsbcScAlternateSetting* alternateSettingListRecFound = NULL;
		while (alternateSettingListRec)
			{
			if (alternateSettingListRec->iSetting == aInterfaceNumber)
				{
				alternateSettingListRecFound = alternateSettingListRec;
				if (alternateSettingListRec->iPrevious == NULL)	//Interface is at HEAD OF List, Should only be if Interface is also at Tail of list
					{
					iAlternateSettingList->iHead = alternateSettingListRec->iNext;	// Should be NULL
					if (alternateSettingListRec->iNext)
						iAlternateSettingList->iHead->iPrevious = NULL;
					}
				else if (alternateSettingListRec->iNext == NULL) //Interface is at TAIL OF List
					{
					iAlternateSettingList->iTail = alternateSettingListRecFound->iPrevious;
					iAlternateSettingList->iTail->iNext = NULL;
					}
				else	//Somewhere in the middle (would not expect this in normal operation, but here for completeness)
					{
					__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DestroyInterface Middle interface!\n"));
					alternateSettingListRec->iPrevious->iNext = alternateSettingListRec->iNext;
					alternateSettingListRec->iNext->iPrevious = alternateSettingListRec->iPrevious;
					}	

				delete alternateSettingListRecFound;
				break;
				}
 			alternateSettingListRec = alternateSettingListRec->iPrevious;
			}
		}
	}

//
// SetupEp0
//

TInt DLddUsbcScChannel::SetupEp0()
	{
	__ASSERT_ALWAYS(iEp0Endpoint==NULL, Kern::Fault("DLddUsbcScChannel::SetupEp0", __LINE__));

	TUsbcScEndpointInfo ep0Info = TUsbcScEndpointInfo(KUsbEpTypeControl, KUsbEpDirBidirect);
	ep0Info.iSize =  iController->Ep0PacketSize();

	TUsbcScEndpoint* ep0 = new TUsbcScEndpoint(this, iController, &ep0Info, 0);
	if (ep0 == NULL)
		{
		return KErrNoMemory;
		}

	TInt r = ep0->Construct();
	if (r != KErrNone)
		{
		delete ep0;
		return KErrNoMemory;
		}

	ep0->SetRealEpNumber(0);
	ep0->SetBuffer(NULL); // Cannot find it this way.

	iEp0Endpoint = ep0;
	return KErrNone;
	}

//
// DestroyEp0
//

void DLddUsbcScChannel::DestroyEp0()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(" DLddUsbcScChannel::DestroyEp0"));
	delete iEp0Endpoint;
	iEp0Endpoint = NULL;
	}


void DLddUsbcScChannel::RequestCallbackEp0(TAny* aDLddUsbcScChannel)
    {
	DLddUsbcScChannel* channel = (DLddUsbcScChannel*) aDLddUsbcScChannel;

	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::RequestCallbackEp0"));

	if (channel->ChannelClosing())
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Channel Closing: Completion not accepted!"));
		return;
		}

	switch (channel->iEp0Endpoint->iRequestCallbackInfo->iTransferDir)
		{
	case EControllerWrite:
		channel->iBuffers[channel->iEP0InBuff].CompleteWrite();
		return;
	case EControllerRead:
		channel->iBuffers[channel->iEP0OutBuff].CompleteRead();
		return;
	default:
		Kern::Printf("DLddUsbcScChannel::RequestCallbackEp0 - Unexpected completion direction %d",channel->iEp0Endpoint->iRequestCallbackInfo->iTransferDir);
		Kern::Fault("DLddUsbcScChannel::RequestCallbackEp0", __LINE__);
		} 
	}






//
// EndpointStatusChangeCallback
//

void DLddUsbcScChannel::EndpointStatusChangeCallback(TAny* aDLddUsbcScChannel)
    {
	__KTRACE_OPT(KUSB, Kern::Printf("EndpointStatusChangeCallback"));
    DLddUsbcScChannel* dUsbc = (DLddUsbcScChannel*) aDLddUsbcScChannel;
	if (dUsbc->iChannelClosing)
		return;
	TUint endpointState = dUsbc->iEndpointStatusCallbackInfo.State();
	const TInt reqNo = (TInt) RDevUsbcScClient::ERequestEndpointStatusNotify;
	if (dUsbc->iRequestStatus[reqNo])
		{
		__KTRACE_OPT(KUSB, Kern::Printf("EndpointStatusChangeCallback Notify status"));
		DThread* client = dUsbc->iClient;
		// set client descriptor length to zero
		TInt r = Kern::ThreadRawWrite(client, dUsbc->iEndpointStatusChangePtr, &endpointState,
									  sizeof(TUint), client);
		if (r != KErrNone)
			dUsbc->PanicClientThread(r);
		Kern::RequestComplete(dUsbc->iClient, dUsbc->iRequestStatus[reqNo], r);
		dUsbc->iEndpointStatusChangePtr = NULL;
		}
	}


//
// StatusChangeCallback
//

void DLddUsbcScChannel::StatusChangeCallback(TAny* aDLddUsbcScChannel)
	{
    DLddUsbcScChannel* dUsbc = (DLddUsbcScChannel*) aDLddUsbcScChannel;
	if (dUsbc->iChannelClosing)
		return;

    TUsbcDeviceState deviceState;
    TInt i;
 	for (i = 0;
 		 (i < KUsbcDeviceStateRequests) && ((deviceState = dUsbc->iStatusCallbackInfo.State(i)) != EUsbcNoState);
 		 ++i)
		{
 		__KTRACE_OPT(KUSB, Kern::Printf("StatusChangeCallBack status=%d", deviceState));
		if (deviceState & KUsbAlternateSetting)
			{
			dUsbc->ProcessAlternateSetting(deviceState);
			}
		else
			{
			dUsbc->ProcessDeviceState(deviceState);
			// Send Status to EP0 buffer.		
			// Before the client calls RDevUsbcScClient::FinalizeInterface(),
			// this function might be called.
			// So we add a guard for dUsbc->iBuffers
			if( dUsbc->iBuffers )
				{
				dUsbc->iBuffers[dUsbc->iEP0OutBuff].SendEp0StatusPacket(deviceState);
				}
			}

		// Only queue if userside is interested
		if (dUsbc->iDeviceStatusNeeded)
			{
			dUsbc->iStatusFifo->AddStatusToQueue(deviceState);
			const TInt reqNo = (TInt) RDevUsbcScClient::ERequestAlternateDeviceStatusNotify;
			if (dUsbc->AlternateDeviceStateTestComplete())
				Kern::RequestComplete(dUsbc->iClient, dUsbc->iRequestStatus[reqNo], KErrNone);
			}
		}
 	// We don't want to be interrupted in the middle of this:
	const TInt irqs = NKern::DisableInterrupts(2);
 	dUsbc->iStatusCallbackInfo.ResetState();
	NKern::RestoreInterrupts(irqs);
	}


void DLddUsbcScChannel::OtgFeatureChangeCallback(TAny* aDLddUsbcScChannel)
    {
	__KTRACE_OPT(KUSB, Kern::Printf("OtgFeatureChangeCallback"));
    DLddUsbcScChannel* dUsbc = (DLddUsbcScChannel*) aDLddUsbcScChannel;
	if (dUsbc->iChannelClosing)
		return;

    TUint8 features;
    // No return value check. Assume OTG always supported here
    dUsbc->iController->GetCurrentOtgFeatures(features);

    const TInt reqNo = (TInt) RDevUsbcScClient::ERequestOtgFeaturesNotify;
	if (dUsbc->iRequestStatus[reqNo])
		{
		__KTRACE_OPT(KUSB, Kern::Printf("OtgFeatureChangeCallback Notify status"));
		TInt r = Kern::ThreadRawWrite(dUsbc->iClient, dUsbc->iOtgFeatureChangePtr,
                                      &features, sizeof(TUint8), dUsbc->iClient);
		if (r != KErrNone)
			dUsbc->PanicClientThread(r);
		Kern::RequestComplete(dUsbc->iClient, dUsbc->iRequestStatus[reqNo], r);
		dUsbc->iOtgFeatureChangePtr = NULL;
		}
    }


//
// SelectAlternateSetting
//

TInt DLddUsbcScChannel::SelectAlternateSetting(TUint aAlternateSetting)
	{
	TUsbcScEndpoint* ep;

	// First, find the alt setting record, which corresponds to the alt setting number.
	TUsbcScAlternateSetting* alternateSettingListRec;
	if(iAlternateSettingList)
		{
		for (alternateSettingListRec = iAlternateSettingList->iHead; alternateSettingListRec; alternateSettingListRec = alternateSettingListRec->iNext)
			if (alternateSettingListRec->iSetting == aAlternateSetting)
				{
				// Record has been located.

				// Update current ep setting vars 
				iEndpoint = alternateSettingListRec->iEndpoint;
				iNumberOfEndpoints = alternateSettingListRec->iNumberOfEndpoints;



				// Reset buffers for new ep set
				for (TInt i = 1; i <= KMaxEndpointsPerClient; i++)
					{
					ep = alternateSettingListRec->iEndpoint[i];
					if (ep!=NULL)
						ep->StartBuffer(); // Buffer::StartEndpoint(...)   sets the necessary parameters to the buffer, for use for a perticular endpoint.
					}

				return KErrNone;
				}
		}
	return KErrGeneral;
	}

/* The user calls this to move into the next alternate setting.  After this call, it is assumed the user wants to
Transmit using endpoints belonging to this alternate Setting.  Writes to the IN endpoints will be allowed until
the host changed the alternate setting again
Returns a 32 int with the top 16 bits represents the sequance, and the botten, the alternatre setting no.
*/
TInt32 DLddUsbcScChannel::StartNextInAlternateSetting()
	{
	iUserKnowsAltSetting = ETrue;
	return iAsSeq<<16 | iAlternateSetting;
	} 


//
// EpFromAlternateSetting
//

TInt DLddUsbcScChannel::EpFromAlternateSetting(TUint aAlternateSetting, TInt aEndpoint)
	{
	TUsbcScAlternateSetting* alternateSettingListRec = iAlternateSettingList->iHead;
	while (alternateSettingListRec)
		{
		if (alternateSettingListRec->iSetting == aAlternateSetting)
			{
			if ((aEndpoint <= alternateSettingListRec->iNumberOfEndpoints) &&
				(aEndpoint > 0))
				{
				return alternateSettingListRec->iEndpoint[aEndpoint]->RealEpNumber();
				}
			else
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("  Error: aEndpoint %d wrong for aAlternateSetting %d",
												  aEndpoint, aAlternateSetting));
				return KErrNotFound;
				}
			}
		alternateSettingListRec = alternateSettingListRec->iNext;
		}
	__KTRACE_OPT(KPANIC, Kern::Printf("  Error: no aAlternateSetting %d found", aAlternateSetting));
	return KErrNotFound;
	}

//
// ProcessAlternateSetting
//

TInt DLddUsbcScChannel::ProcessAlternateSetting(TUint aAlternateSetting)
	{

	TUint newSetting = aAlternateSetting&(~KUsbAlternateSetting);
	__KTRACE_OPT(KUSB, Kern::Printf("ProcessAlternateSetting 0x%08x selecting alternate setting 0x%08x", aAlternateSetting, newSetting));
	iUserKnowsAltSetting=EFalse;
	iAlternateSetting = newSetting;
	iAsSeq++; 
	
	ResetInterface(KErrUsbInterfaceChange);					// kill any outstanding IN transfers

	TInt r = SelectAlternateSetting(newSetting);
	if (r != KErrNone)
		return r;


	StartEpReads();
    return KErrNone;
	}


//
//  ProcessDeviceState
//
// Called from StatusChangeCallback.

TInt DLddUsbcScChannel::ProcessDeviceState(TUsbcDeviceState aDeviceState)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::ProcessDeviceState(%d -> %d)", iDeviceState, aDeviceState));
	if (iDeviceState == aDeviceState)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  No state change => nothing to be done."));
		return KErrNone;
		}
	if (iDeviceState == EUsbcDeviceStateSuspended)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Coming out of Suspend: old state = %d", iOldDeviceState));
		iDeviceState = iOldDeviceState;
		if (iDeviceState == aDeviceState)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("  New state same as before Suspend => nothing to be done."));
			return KErrNone;
			}
		}
	TBool renumerateState = (aDeviceState == EUsbcDeviceStateConfigured);
	TBool deconfigured = EFalse;
	TInt cancellationCode = KErrNone;
	if (aDeviceState == EUsbcDeviceStateSuspended)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("  Suspending..."));
		iOldDeviceState = iDeviceState;
		// Put PSL into low power mode here
		}
	else
		{
		deconfigured = (iDeviceState == EUsbcDeviceStateConfigured &&
						aDeviceState != EUsbcDeviceStateConfigured);
		if (iDeviceState == EUsbcDeviceStateConfigured)
			{
			if (aDeviceState == EUsbcDeviceStateUndefined)
				cancellationCode = KErrUsbCableDetached;
			else if (aDeviceState == EUsbcDeviceStateAddress)
				cancellationCode = KErrUsbDeviceNotConfigured;
			else if (aDeviceState == EUsbcDeviceStateDefault)
				cancellationCode = KErrUsbDeviceBusReset;
			else
				cancellationCode = KErrUsbDeviceNotConfigured;
			}
		}
	iDeviceState = aDeviceState;
	if (iValidInterface || iOwnsDeviceControl)
		{

		// This LDD may not own an interface. It could be some manager reenumerating
		// after its subordinate LDDs have setup their interfaces.
		if (deconfigured)
			{
		    DeConfigure(cancellationCode);
			}
		else if (renumerateState)
			{
 			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScChannel:: Reumerated!"));
			// Select main interface & latch in new endpoint set
			SelectAlternateSetting(0);
			__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScChannel:: StartReads!"));
			StartEpReads();
			}
		}

	const TInt reqNo = (TInt) RDevUsbcScClient::ERequestReEnumerate;
	if (renumerateState && iRequestStatus[reqNo])
		{
		// This lot must be done if we are reenumerated
		Kern::RequestComplete(iClient, iRequestStatus[reqNo], KErrNone);
		}

    return KErrNone;
    }


TBool DLddUsbcScChannel::AlternateDeviceStateTestComplete()
	{
	TBool completeNow = EFalse;
	const TInt reqNo = (TInt) RDevUsbcScClient::ERequestAlternateDeviceStatusNotify;
	if (iRequestStatus[reqNo])
		{
		// User req is outstanding
		TUint32 deviceState;
		if (iStatusFifo->GetDeviceQueuedStatus(deviceState) == KErrNone)
			{
			// Device state waiting to be sent userside
			completeNow = ETrue;
			__KTRACE_OPT(KUSB, Kern::Printf("StatusChangeCallback Notify status"));
			// set client descriptor length to zero
			TInt r = Kern::ThreadRawWrite(iClient, iStatusChangePtr, &deviceState,
										  sizeof(TUint32), iClient);
			if (r != KErrNone)
				PanicClientThread(r);
			iStatusChangePtr = NULL;
			}
		}
	return completeNow;
	}


void DLddUsbcScChannel::DeConfigure(TInt aErrorCode)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DeConfigure()"));
	// Called after deconfiguration. Cancels transfers on all endpoints.
	ResetInterface(aErrorCode);
	// Cancel the endpoint status notify request if it is outstanding.
	const TInt KEpNotReq = RDevUsbcScClient::ERequestEndpointStatusNotify;
	if (iRequestStatus[KEpNotReq])
		{
		CancelNotifyEndpointStatus();
		Kern::RequestComplete(iClient, iRequestStatus[KEpNotReq], aErrorCode);
		}
	// We have to reset the alternate setting number when the config goes away.
 	SelectAlternateSetting(0);
	iAlternateSetting = 0;
	}


void DLddUsbcScChannel::StartEpReads()
	{
	// Queued after enumeration. Starts reads on all endpoints.
	// The endpoint itself decides if it can do a read
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::StartEpReads - 1"));
	
	TInt i;
	TInt8 needsPacket;

	for (i=0; i<iNumBuffers; i++)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::StartEpReads - 2 %d",i));

		needsPacket = iBuffers[i].iNeedsPacket;
		if (needsPacket)
			{
			__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::StartEpReads - 3"));
			iBuffers[i].UpdateBufferList(0,0,(needsPacket==TUsbcScBuffer::KEpIsStarting));
			}
		}

	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::StartEpReads - 4"));

		// now update ep0
		iBuffers[iEP0OutBuff].Ep0CancelLddRead();
		iBuffers[iEP0OutBuff].UpdateBufferList(0,0);
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::StartEpReads - 5"));

	}


void DLddUsbcScChannel::ResetInterface(TInt aErrorCode)
	{
	if (!iValidInterface && !iOwnsDeviceControl)
			return;
		
	TInt i;
	for (i=0; i<iNumBuffers; i++)
		{
		iBuffers[i].iNeedsPacket=TUsbcScBuffer::KNoEpAssigned;
		}

	TUsbcScBuffer* buffer;

	for (i = 1; i <= iNumberOfEndpoints; i++)
		{
		// Firstly, cancel ('garbge collect') any stale reads/writes into PIL.

		__KTRACE_OPT(KUSB, Kern::Printf("Cancelling transfer ep=%d", i));
		iEndpoint[i]->AbortTransfer();

		// All OUT endpoints need a packet sent, to indicate the termination of the current ep 'pipe'.
		// This will complete any current read, or will be read later.
		// All IN endpoints must be simply cancelled, including anything queued.
		// Ep0 operates outside alt settings, and so we don't cancel anything.

		buffer=iEndpoint[i]->GetBuffer();
		if (buffer->iDirection==KUsbcScIn)
			{
			buffer->iStatusList.Complete(KErrCancel);	//aErrorCode 
			buffer->iStatusList.CancelQueued();			//aErrorCode
			}
		else
			buffer->iNeedsPacket=TUsbcScBuffer::KEpIsEnding;	// We will send a packet on re-start, which doubles as a 'cancel'
															 	// for the old alt setting.
		}
	}



void DLddUsbcScChannel::EmergencyCompleteDfc(TAny* aDLddUsbcScChannel)
	{
	((DLddUsbcScChannel*) aDLddUsbcScChannel)->DoEmergencyComplete();
	}

TInt DLddUsbcScChannel::DoEmergencyComplete()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DLddUsbcScChannel::DoEmergencyComplete"));
	// cancel any pending DFCs
	// complete all client requests

	TUsbcScBuffer* buffer;
	TInt i;	
	// Complete EP0 request

	TInt direction=iEp0Endpoint->iRequestCallbackInfo->iTransferDir;
	if (direction==EControllerWrite)
		{
		iBuffers[iEP0InBuff].iStatusList.CancelQueued();
		iBuffers[iEP0InBuff].iStatusList.Complete(KErrDisconnected);
		}
	else if (direction==EControllerRead)
		{
		iBuffers[iEP0OutBuff].iStatusList.CancelQueued();
		iBuffers[iEP0OutBuff].iStatusList.Complete(KErrDisconnected);
		}
		
	// Complete other Eps request
	for (i = 1; i <= iNumberOfEndpoints; i++)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Cancelling transfer ep=%d", i));
		buffer=iEndpoint[i]->GetBuffer();
		buffer->iStatusList.CancelQueued();
		buffer->iStatusList.Complete(KErrDisconnected);
		}

	// Complete remaining requests

    for (TInt i = 0; i < KUsbcMaxRequests; i++)
        {
        if (iRequestStatus[i])
            {
            __KTRACE_OPT(KUSB, Kern::Printf("Complete request 0x%x", iRequestStatus[i]));
            Kern::RequestComplete(iClient, iRequestStatus[i], KErrDisconnected);
            }
        }
    iStatusCallbackInfo.Cancel();
    iEndpointStatusCallbackInfo.Cancel();
    iOtgFeatureCallbackInfo.Cancel();

	return KErrNone;
	}


void DLddUsbcScChannel::PanicClientThread(TInt aReason)
	{
	Kern::ThreadKill(iClient, EExitPanic, aReason, KUsbLDDKillCat);
	}

// End DLddUsbcScChannel

/*****************************************************************************\
*    TUsbcScEndpoint                                                          *
*                                                                             *
*                                                                             *
*                                                                             *
\*****************************************************************************/


// Constructor
TUsbcScEndpoint::TUsbcScEndpoint(DLddUsbcScChannel* aLDD, DUsbClientController* aController,
							 const TUsbcScEndpointInfo* aEndpointInfo, TInt aEndpointNum
							 )
	: iRequestCallbackInfo(NULL),
	  iController(aController),
	  iEndpointInfo(*aEndpointInfo),
	  iClientReadPending(EFalse),
	  iClientWritePending(EFalse),
	  iEndpointNumber(aEndpointNum),
	  iRealEpNumber(-1),
	  iLdd(aLDD),
	  iError(KErrNone),
	  iBytesTransferred(0),
	  iBuffer(NULL)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint::TUsbcScEndpoint"));
	}


TInt TUsbcScEndpoint::Construct()
	{
	__KTRACE_OPT(KUSB,Kern::Printf("TUsbcScEndpoint::TUsbcScEndpoint iEndpointNumber %d\n",iEndpointNumber));

	iRequestCallbackInfo = new TUsbcRequestCallback(iLdd,
													iEndpointNumber,
													(iEndpointNumber==0)?DLddUsbcScChannel::RequestCallbackEp0:TUsbcScEndpoint::RequestCallback,
													(iEndpointNumber==0)?  (TAny*) iLdd:  (TAny*) this,
													iLdd->iDfcQ,
													KUsbRequestCallbackPriority);

	return (iRequestCallbackInfo == NULL)?KErrNoMemory:KErrNone;
	}


TUsbcScEndpoint::~TUsbcScEndpoint()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint::~TUsbcScEndpoint(%d)", iEndpointNumber));
	AbortTransfer();
	delete iRequestCallbackInfo;
	}

// This is called by the PIL, on return from a read or write.
// Inturn it calls either the read or write function for that buffer.

void TUsbcScEndpoint::RequestCallback(TAny* aTUsbcScEndpoint)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint::RequestCallback"));

	if (((TUsbcScEndpoint*)aTUsbcScEndpoint)->iLdd->ChannelClosing())
		{
		__KTRACE_OPT(KUSB, Kern::Printf("Channel Closing: Completion not accepted!"));
		return;
		}

	switch (((TUsbcScEndpoint*) aTUsbcScEndpoint)->iRequestCallbackInfo->iTransferDir)
	{
	case EControllerWrite:
		((TUsbcScEndpoint*) aTUsbcScEndpoint)->iBuffer->CompleteWrite();
		return;
	case EControllerRead:
		((TUsbcScEndpoint*) aTUsbcScEndpoint)->iBuffer->CompleteRead();
		return;
	default:
		Kern::Printf("TUsbcScEndpoint::RequestCallback - Unexpected compleation direction %d",((TUsbcScEndpoint*) aTUsbcScEndpoint)->iRequestCallbackInfo->iTransferDir);
		Kern::Fault("TUsbcScEndpoint::RequestCallback", __LINE__);
	} 
	}


/*

This is used to tidy up cancel calls into the PIL, regardless of them being reads or writes

*/

void TUsbcScEndpoint::AbortTransfer()
	{
	if (!iLdd->iRealizeCalled)
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint::AbortTransfer Ep# %d Real Ep # %d - N.R.",iEndpointNumber, iRealEpNumber));
		return;
		} 
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint::AbortTransfer Ep# %d Real Ep # %d",iEndpointNumber, iRealEpNumber));
		}

	
	if (iBuffer && (iBuffer->iStatusList.iState) || (!iRealEpNumber))
		{
		if (iRequestCallbackInfo->iTransferDir==EControllerWrite)
			iController->CancelWriteBuffer(iLdd, iRealEpNumber);
		else if (iRequestCallbackInfo->iTransferDir==EControllerRead)
			iController->CancelReadBuffer(iLdd, iRealEpNumber);
		else
			{
			if (iEndpointNumber!=0) // endpoint zero starts off not sent in any direction, then keeps changing.
				{
				__KTRACE_OPT(KUSB,Kern::Printf("\nTUsbcScEndpoint::AbortTransfer WARNING: Invalid Direction %d on (%d,%d)!\n",iRequestCallbackInfo->iTransferDir,iEndpointNumber, iRealEpNumber));
				}
			else
				{
				__KTRACE_OPT(KUSB, Kern::Printf("\nTUsbcScEndpoint::AbortTransfer Can't stop direction %d on (%d,%d)!\n",iRequestCallbackInfo->iTransferDir,iEndpointNumber, iRealEpNumber));
				}
			}
		}
	else if (!iBuffer)
		{
		__KTRACE_OPT(KUSB,Kern::Printf("\nTUsbcScEndpoint::AbortTransfer WARNING: iBuffer is NULL on (%d,%d)\n",iEndpointNumber, iRealEpNumber));
		return;
		}
	
	if (iRequestCallbackInfo)
		iRequestCallbackInfo->iDfc.Cancel();
	else
		{
		__KTRACE_OPT(KUSB,Kern::Printf("\nTUsbcScEndpoint::AbortTransfer WARNING: iRequestCallbackInfo is NULL\n"));
		}
		
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScEndpoint Done."));
	}

// End TUsbcScEndpoint


/*****************************************************************************\
*    TUsbcScAlternateSettingList                                              *
*                                                                             *
*                                                                             *
*                                                                             *
\*****************************************************************************/


TUsbcScAlternateSetting::TUsbcScAlternateSetting()
	: iNext(NULL),
	  iPrevious(NULL),
	  iNumberOfEndpoints(0),
	  iSetting(0)
	{
	for (TInt i = 0; i <= KMaxEndpointsPerClient; i++)
		{
		iEndpoint[i] = NULL;
		}
	}


TUsbcScAlternateSetting::~TUsbcScAlternateSetting()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcScAlternateSetting::~TUsbcScAlternateSetting()"));
	for (TInt i = 0; i <= KMaxEndpointsPerClient; i++)
		{
		delete iEndpoint[i];
		}
	}

// End TUsbcScAlternateSettingList



TUsbcScAlternateSettingList::TUsbcScAlternateSettingList()
	: iHead(NULL),
	  iTail(NULL)
	{
	}

TUsbcScAlternateSettingList::~TUsbcScAlternateSettingList()
	{
	}



/*****************************************************************************\
*   TUsbcDeviceStatusQueue                                                    *
*                                                                             *
*                                                                             *
*                                                                             *
\*****************************************************************************/


TUsbcDeviceStatusQueue::TUsbcDeviceStatusQueue()
	{
	FlushQueue();
	}


void TUsbcDeviceStatusQueue::FlushQueue()
	{
	for (TInt i = 0; i < KUsbDeviceStatusQueueDepth; i++)
		{
		iDeviceStatusQueue[i] = KUsbDeviceStatusNull;
		}
	iStatusQueueHead = 0;
	}


void TUsbcDeviceStatusQueue::AddStatusToQueue(TUint32 aDeviceStatus)
	{
	// Only add a new status if it is not a duplicate of the one at the head of the queue
	if (!(iStatusQueueHead != 0 &&
		  iDeviceStatusQueue[iStatusQueueHead - 1] == aDeviceStatus))
		{
		if (iStatusQueueHead == KUsbDeviceStatusQueueDepth)
			{
			// Discard item at tail of queue
			TUint32 status;
			GetDeviceQueuedStatus(status);
			}
		iDeviceStatusQueue[iStatusQueueHead] = aDeviceStatus;
		iStatusQueueHead++;
		}
	}


TInt TUsbcDeviceStatusQueue::GetDeviceQueuedStatus(TUint32& aDeviceStatus)
	{
	TInt r = KErrNone;
	if (iStatusQueueHead <= 0)
		{
		r = KErrGeneral;
		aDeviceStatus = KUsbDeviceStatusNull;
		}
	else
		{
		aDeviceStatus = iDeviceStatusQueue[0];
		for(TInt i = 1; i < KUsbDeviceStatusQueueDepth; i++)
			{
			TUint32 s = iDeviceStatusQueue[i];
			iDeviceStatusQueue[i - 1] = s;
			}
		iStatusQueueHead--;
		iDeviceStatusQueue[KUsbDeviceStatusQueueDepth - 1] = KUsbDeviceStatusNull;
		}
	return r;
	}

// End TUsbcDeviceStatusQueue

//---

