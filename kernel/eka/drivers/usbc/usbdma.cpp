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
// e32\drivers\usbc\usbdma.cpp
// LDD for USB Device driver stack:
// Management of DMA-capable data buffers.
// 
//

/**
 @file usbdma.cpp
 @internalTechnology
*/

#include <drivers/usbc.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "usbdmaTraces.h"
#endif



#if defined(_DEBUG)
static const char KUsbPanicLdd[] = "USB LDD";
#endif


TDmaBuf::TDmaBuf(TUsbcEndpointInfo* aEndpointInfo, TInt aBandwidthPriority)
	: iBufBasePtr(NULL),
	  iCurrentDrainingBuffer(NULL),
	  iCurrentPacket(0),
	  iCurrentPacketIndexArray(NULL),
	  iCurrentPacketSizeArray(NULL)
	{
	iMaxPacketSize = aEndpointInfo->iSize;
	iEndpointType = aEndpointInfo->iType;

	switch (aEndpointInfo->iType)
		{
	case KUsbEpTypeControl:
		iBufSz = KUsbcDmaBufSzControl;
		iNumberofBuffers = KUsbcDmaBufNumControl;
		break;
	case KUsbEpTypeIsochronous:
		iBufSz = KUsbcDmaBufSzIsochronous;
		iNumberofBuffers = KUsbcDmaBufNumIsochronous;
		break;
	case KUsbEpTypeBulk:
		{
		if (aEndpointInfo->iDir == KUsbEpDirOut)
			{
			const TInt priorityOUT = aBandwidthPriority & 0x0f;
			iBufSz = KUsbcDmaBufSizesBulkOUT[priorityOUT];
			}
		else
			{
			const TInt priorityIN = (aBandwidthPriority >> 4) & 0x0f;
			iBufSz = KUsbcDmaBufSizesBulkIN[priorityIN];
			}
		iNumberofBuffers = KUsbcDmaBufNumBulk;
		}
		break;
	case KUsbEpTypeInterrupt:
		iBufSz = KUsbcDmaBufSzInterrupt;
		iNumberofBuffers = KUsbcDmaBufNumInterrupt;
		break;
	default:
		iBufSz = 0;
		iNumberofBuffers = 0;
		}

	if (aEndpointInfo->iDir == KUsbEpDirIn)
		{
		iNumberofBuffers = 1;								// IN endpoints only have 1 buffer
		}

	for (TInt i = 0; i < KUsbcDmaBufNumMax; i++)
		{
		// Buffer logical addresses (pointers)
		iBuffers[i] = NULL;
		// Buffer physical addresses
		iBufferPhys[i] = 0;
		// Packet indexes base array
		iPacketIndex[i] = NULL;
		// Packet sizes base array
		iPacketSize[i] = NULL;
		}
	}


TInt TDmaBuf::Construct(TUsbcEndpointInfo* aEndpointInfo)
	{
	if (aEndpointInfo->iDir != KUsbEpDirIn)
		{
		// IN endpoints don't need a packet array

		// At most 2 packets (clump of max packet size packets) + possible zlp
		TUsbcPacketArray* bufPtr = iPacketInfoStorage;
		// this divides up the packet indexing & packet size array over the number of buffers
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_CONSTRUCT,
		        "TDmaBuf::Construct() array base=0x%08x", bufPtr );
		for (TInt i = 0; i < iNumberofBuffers; i++)
			{
			iPacketIndex[i] = bufPtr;
			bufPtr += KUsbcDmaBufMaxPkts;
			iPacketSize[i] = bufPtr;
			bufPtr += KUsbcDmaBufMaxPkts;
            OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_CONSTRUCT_DUP1,
                    "TDmaBuf::Construct() packetIndex[%d]=0x%08x packetSize[%d]=0x%08x",
                    i, reinterpret_cast<TUint>(iPacketIndex[i]), i, 
                    reinterpret_cast<TUint>(iPacketSize[i]) );

			}
		}
	else
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_CONSTRUCT_DUP2,
                "TDmaBuf::Construct() IN endpoint" );
		}
	Flush();
	return KErrNone;
	}


TDmaBuf::~TDmaBuf()
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_FLOW, TDMABUF_TDMABUF_DES, "TDmaBuf::~TDmaBuf()" );
	}

TInt TDmaBuf::BufferTotalSize() const
	{
	return iBufSz * iNumberofBuffers;
	}

TInt TDmaBuf::BufferSize() const
    {
    return iBufSz;
    }

TInt TDmaBuf::SetBufferAddr(TInt aBufInd, TUint8* aBufAddr)
    {
    __ASSERT_DEBUG((aBufInd < iNumberofBuffers),
                       Kern::Fault(KUsbPanicLdd, __LINE__));
    iDrainable[aBufInd] = iCanBeFreed[aBufInd] = EFalse;
    iBuffers[aBufInd] = aBufAddr;
    iBufferPhys[aBufInd] = Epoc::LinearToPhysical((TLinAddr)aBufAddr);
    OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_SETBUFFERADDR,
            "TDmaBuf::SetBufferAddr() iBuffers[%d]=0x%08x", aBufInd, 
            reinterpret_cast<TUint>(iBuffers[aBufInd]) );
    return KErrNone;
    }

TInt TDmaBuf::BufferNumber() const
    {
    return iNumberofBuffers;
    }

void TDmaBuf::SetMaxPacketSize(TInt aSize)
	{
	iMaxPacketSize = aSize;
	}


void TDmaBuf::Flush()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_FLUSH,
	        "TDmaBuf::Flush %x", this );
	iRxActive = EFalse;
	iTxActive = EFalse;
	iExtractOffset = 0;
	iTotalRxBytesAvail = 0;
	iTotalRxPacketsAvail = 0;
	iCurrentDrainingBufferIndex = KUsbcInvalidBufferIndex;
	iCurrentFillingBufferIndex = 0;
	iDrainQueueIndex = KUsbcInvalidDrainQueueIndex;
	for (TInt i = 0; i < KUsbcDmaBufNumMax; i++)
		{
		iDrainable[i] = EFalse;
		iCanBeFreed[i] = EFalse;
		iNumberofBytesRx[i] = 0;
		iNumberofPacketsRx[i] = 0;
		iError[i] = KErrGeneral;
		iDrainQueue[i] = KUsbcInvalidBufferIndex;
#if defined(USBC_LDD_BUFFER_TRACE)
		iFillingOrderArray[i] = 0;
		iNumberofBytesRxRemain[i] = 0;
		iNumberofPacketsRxRemain[i] = 0;
#endif
		}
	// Drain queue is 1 oversized
	iDrainQueue[KUsbcDmaBufNumMax] = KUsbcInvalidBufferIndex;

#if defined(USBC_LDD_BUFFER_TRACE)
	iFillingOrder = 0;
	iDrainingOrder = 0;
#endif
	}


void TDmaBuf::RxSetActive()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXSETACTIVE, 
	        "TDmaBuf::RxSetActive %x", this );
	iRxActive = ETrue;
	}


void TDmaBuf::RxSetInActive()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXSETINACTIVE, 
	        "TDmaBuf::RxSetInActive %x", this );
	iRxActive = EFalse;
	}


TBool TDmaBuf::RxIsActive()
	{
	return iRxActive;
	}


void TDmaBuf::TxSetActive()
	{
	iTxActive = ETrue;
	}


void TDmaBuf::TxSetInActive()
	{
	iTxActive = EFalse;
	}


TBool TDmaBuf::TxIsActive()
	{
	return iTxActive;
	}


/**************************** Rx DMA Buffer Access *************************/

void TDmaBuf::ModifyTotalRxBytesAvail(TInt aVal)
	{
	iTotalRxBytesAvail += aVal;
	}


void TDmaBuf::ModifyTotalRxPacketsAvail(TInt aVal)
	{
	iTotalRxPacketsAvail += aVal;
	}


TBool TDmaBuf::AdvancePacket()
	{
	ModifyTotalRxPacketsAvail(-1);
	TBool r = ETrue;
	__ASSERT_DEBUG((iCurrentDrainingBufferIndex >= 0),
					   Kern::Fault(KUsbPanicLdd, __LINE__));
	if (++iCurrentPacket >= iNumberofPacketsRx[iCurrentDrainingBufferIndex])
		{
		r = NextDrainableBuffer();
		}
	iExtractOffset = 0;
	__ASSERT_DEBUG((iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex) ||
				   (iCurrentPacket < KUsbcDmaBufMaxPkts),
				   Kern::Fault(KUsbPanicLdd, __LINE__));
	return r;
	}


TInt TDmaBuf::PeekNextPacketSize()
	{
	TUint pkt = iCurrentPacket;
	TInt index = iCurrentDrainingBufferIndex;
	TInt size = -1;
	if (pkt >= iNumberofPacketsRx[index])
		{
		index = PeekNextDrainableBuffer();
		pkt = 0;
		}

	if ((index != KUsbcInvalidBufferIndex) && iNumberofPacketsRx[index])
		{
		const TUsbcPacketArray* sizeArray = iPacketSize[index];
		size = (TInt)sizeArray[pkt];
		}

	__ASSERT_DEBUG((iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex) ||
				   (iCurrentPacket < KUsbcDmaBufMaxPkts),
				   Kern::Fault(KUsbPanicLdd, __LINE__));
	return size;
	}


inline TInt TDmaBuf::GetCurrentError()
	{
	// USB bus errors are v.rare. To avoid having an error code attached to every packet since
	// almost every errorcode will be KErrNone, we have a single error code per buffer
	// If the error code is != KErrNone then it refers to the LAST packet in the buffer
	TInt errorCode = KErrNone;
	//Check the index, it's not equal to negative (-1) value defined in 
	//KUsbcInvalidBufferIndex.
	__ASSERT_DEBUG((iCurrentDrainingBufferIndex >= 0),
					   Kern::Fault(KUsbPanicLdd, __LINE__));
	
	if (iError[iCurrentDrainingBufferIndex] != KErrNone)
		{
		// See if we are at the last packet
		if ((iCurrentPacket + 1) == iNumberofPacketsRx[iCurrentDrainingBufferIndex])
			{
			errorCode = iError[iCurrentDrainingBufferIndex];
			}
		}
	return errorCode;
	}


// used to decide whether a client read can complete straight away
TBool TDmaBuf::IsReaderEmpty()
	{
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_ISREADEREMPTY, 
	        "TDmaBuf::IsReaderEmpty iTotalRxPacketsAvail=%d", iTotalRxPacketsAvail);
	return (iTotalRxPacketsAvail == 0);
	}


void TDmaBuf::ReadXferComplete(TInt aNoBytesRecv, TInt aNoPacketsRecv, TInt aErrorCode)
	{
	// Adjust pending packet
	if ((aNoBytesRecv == 0) && (aErrorCode != KErrNone))
		{
		// Make the buffer available for reuse
		iDrainable[iCurrentFillingBufferIndex] = EFalse;
		return;
		}

	ModifyTotalRxBytesAvail(aNoBytesRecv);
	ModifyTotalRxPacketsAvail(aNoPacketsRecv);
	iNumberofBytesRx[iCurrentFillingBufferIndex] = aNoBytesRecv;
	iNumberofPacketsRx[iCurrentFillingBufferIndex] = aNoPacketsRecv;

#if defined(USBC_LDD_BUFFER_TRACE)
	iNumberofBytesRxRemain[iCurrentFillingBufferIndex] = aNoBytesRecv;
	iNumberofPacketsRxRemain[iCurrentFillingBufferIndex] = aNoPacketsRecv;
#endif

	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_READXFERCOMPLETE,
	        "TDmaBuf::ReadXferComplete 2 # of bytes=%d # of packets=%d", iTotalRxBytesAvail, iTotalRxPacketsAvail );
	iDrainable[iCurrentFillingBufferIndex] = ETrue;
	iError[iCurrentFillingBufferIndex] = aErrorCode;
	AddToDrainQueue(iCurrentFillingBufferIndex);
	if (iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex)
		{
		NextDrainableBuffer();
		}
	}


TInt TDmaBuf::RxGetNextXfer(TUint8*& aBufferAddr, TUsbcPacketArray*& aIndexArray,
							TUsbcPacketArray*& aSizeArray, TInt& aLength, TPhysAddr& aBufferPhys)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXGETNEXTXFER,
	        "TDmaBuf::RxGetNextXfer 1" );
	if (RxIsActive())
		{
        OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXGETNEXTXFER_DUP1,
                " ---> RxIsActive, returning" );
		return KErrInUse;
		}

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXGETNEXTXFER_DUP2,
            "TDmaBuf::RxGetNextXfer Current buffer=%d", iCurrentFillingBufferIndex );
	if (iDrainable[iCurrentFillingBufferIndex])
		{
		// If the controller refused the last read request, then the current buffer will still be marked
		// as !Drainable, because the controller never completed the read to the ldd. and therefore the buffer
		// can be reused.
		if (!NextFillableBuffer())
			{
			return KErrNoMemory;
			}
		}

    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXGETNEXTXFER_DUP3,
            "TDmaBuf::RxGetNextXfer New buffer=%d", iCurrentFillingBufferIndex );
	aBufferAddr = iBuffers[iCurrentFillingBufferIndex];
	aBufferPhys = iBufferPhys[iCurrentFillingBufferIndex];
	aIndexArray = iPacketIndex[iCurrentFillingBufferIndex];
	aSizeArray = iPacketSize[iCurrentFillingBufferIndex];
	aLength = iBufSz;

#if defined(USBC_LDD_BUFFER_TRACE)
	iFillingOrderArray[iCurrentFillingBufferIndex] = ++iFillingOrder;
#endif

	return KErrNone;
	}


TInt TDmaBuf::RxCopyPacketToClient(DThread* aThread, TClientBuffer *aTcb, TInt aLength)
	{
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT,
            "TDmaBuf::RxCopyPacketToClient 1" );

#if defined(USBC_LDD_BUFFER_TRACE)
	const TInt numPkts = NoRxPackets();
	const TInt numPktsAlt = NoRxPacketsAlt();
	const TInt numBytes = RxBytesAvailable();
	const TInt numBytesAlt = NoRxBytesAlt();

	if (numPkts != numPktsAlt)
		{
	    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP1,
	            "TDmaBuf::RxCopyPacketToClient: Error: #pkts mismatch global=%d actual=%d",
	            numPkts, numPktsAlt);
		}
	if (numBytes != numBytesAlt)
		{
        OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP2,
                "TDmaBuf::RxCopyPacketToClient: Error: #bytes mismatch global=%d actual=%d",
                numBytes, numBytesAlt);

		}
	if ((numPkts == 0) && (numBytes !=0))
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP3,
			"TDmaBuf::RxCopyPacketToClient: Error: global bytes & pkts mismatch pkts=%d bytes=%d",
			numPkts, numBytes);
		}
	if ((numPktsAlt == 0) && (numBytesAlt !=0))
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP4,
			"TDmaBuf::RxCopyPacketToClient: Error: actual bytes & pkts mismatch pkts=%d bytes=%d",
			numPktsAlt, numBytesAlt);
		}
#endif

	if (!NoRxPackets())
		return KErrNotFound;

    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP5,
            "TDmaBuf::RxCopyPacketToClient 2" );
	// the next condition should be true because we have some packets available
	// coverity[var_tested_neg]
	if (iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex)
		{
		// Marked as Coverity "Intentional" as the member variable
		// iCurrentDrainingBufferIndex is attentionaly negative, from previous 
		// initialization to KUsbcInvalidBufferIndex (which equals -1).
		if (!NextDrainableBuffer())
			return KErrNotFound;
		}

	__ASSERT_DEBUG((iCurrentDrainingBufferIndex >= 0 ),
						   Kern::Fault(KUsbPanicLdd, __LINE__));
	
	if (!iDrainable[iCurrentDrainingBufferIndex])
		return KErrNotFound;

	// Calculate copy-from address & adjust for the fact that
	// some data may have already been read from the packet
	TUint8* logicalSrc = iCurrentDrainingBuffer + iCurrentPacketIndexArray[iCurrentPacket] + iExtractOffset;
	TInt packetSz = iCurrentPacketSizeArray[iCurrentPacket];
	TInt thisPacketSz = packetSz - iExtractOffset;
	TInt errorCode;
	// try and sort out what a "packet" might mean.
	// in a multi-packet dma environment, we might see super-packets
	// i.e. we might just see one packet, maybe 4K or so long, made of lots of small packets
	// Since we don't know where the packet boundaries will be, we have to assume that
	// any 'packet' larger than the max packet size of the ep is, in fact, a conglomeration
	// of smaller packets. However, for the purposes of the packet count, this is still regarded
	// as a single packet and the packet count only decremented when it is consumed.
	// As before, if the user fails to read an entire packet out then the next packet is moved onto anyway
	// To be safe the user must always supply a buffer of at least max packet size bytes.
	if (thisPacketSz > iMaxPacketSize)
		{
		// Multiple packets left in buffer
		// calculate number of bytes to end of packet
		if (iEndpointType == KUsbEpTypeBulk)
			{
			thisPacketSz = iMaxPacketSize - (iExtractOffset & (iMaxPacketSize - 1));
			}
		else
			{
			thisPacketSz = iMaxPacketSize - (iExtractOffset % iMaxPacketSize);
			}
		errorCode = KErrNone;
		}
	else
		{
		errorCode = GetCurrentError();						// single packet left
		}

	iExtractOffset += thisPacketSz;			// iExtractOffset is now at the end of the real or notional packet

	ModifyTotalRxBytesAvail(-thisPacketSz);
#if defined(USBC_LDD_BUFFER_TRACE)
	iNumberofBytesRxRemain[iCurrentDrainingBufferIndex] -= thisPacketSz;
#endif
	// this can only be untrue if the "packet" is a conglomeration of smaller packets:
	if (iExtractOffset == packetSz)
		{
		// packet consumed, advance to next packet in buffer
#if defined(USBC_LDD_BUFFER_TRACE)
		iNumberofPacketsRxRemain[iCurrentDrainingBufferIndex] -= 1;
#endif
		AdvancePacket();
		}

	TPtrC8 des(logicalSrc, thisPacketSz);
	TInt r=Kern::ThreadBufWrite(aThread, aTcb, des, 0, 0, aThread);
	if (r == KErrNone)
		{
		r = errorCode;
		}
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXCOPYPACKETTOCLIENT_DUP6,
            "TDmaBuf::RxCopyPacketToClient 3" );


	FreeDrainedBuffers();

	// Use this error code to complete client read request:
	return r;
	}


TInt TDmaBuf::RxCopyDataToClient(DThread* aThread, TClientBuffer *aTcb, TInt aLength, TUint32& aDestOffset,
								 TBool aRUS, TBool& aCompleteNow)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT,
	        "TDmaBuf::RxCopyDataToClient 1" );
	aCompleteNow = ETrue;

#if defined(USBC_LDD_BUFFER_TRACE)
	const TInt numPkts = NoRxPackets();
	const TInt numPktsAlt = NoRxPacketsAlt();
	const TInt numBytes = RxBytesAvailable();
	const TInt numBytesAlt = NoRxBytesAlt();

	if (numPkts != numPktsAlt)
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP1,
			"TDmaBuf::RxCopyDataToClient: Error: #pkts mismatch global=%d actual=%d",
			numPkts, numPktsAlt);
		}
	if (numBytes != numBytesAlt)
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP2,
			"TDmaBuf::RxCopyDataToClient: Error: #bytes mismatch global=%d actual=%d",
			numBytes, numBytesAlt);
		}
	if ((numPkts == 0) && (numBytes != 0))
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP3,
			"TDmaBuf::RxCopyDataToClient: Error: global bytes & pkts mismatch pkts=%d bytes=%d",
			numPkts, numBytes);
		}
	if ((numPktsAlt == 0) && (numBytesAlt != 0))
		{
    OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP4,
			"TDmaBuf::RxCopyDataToClient: Error: actual bytes & pkts mismatch pkts=%d bytes=%d",
			numPktsAlt, numBytesAlt);
		}
#endif

	if (!NoRxPackets())
		{
		return KErrNotFound;
		}

	// coverity[var_tested_neg]
	if (iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex)
		{
		// Marked as Coverity "Inentional" as the member variable
		// iCurrentDrainingBufferIndex is attentionaly negative, from previous 
		// initialization to KUsbcInvalidBufferIndex (which equals -1).

		if (!NextDrainableBuffer())
			{
#if defined(USBC_LDD_BUFFER_TRACE)
            OstTraceExt2( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP5,
                    "TDmaBuf::RxCopyDataToClient: Error:  No buffer draining=%d, packets=%d",
				    iCurrentDrainingBufferIndex, iTotalRxPacketsAvail);
#endif
			return KErrNotFound;
			}
		}
#if defined(USBC_LDD_BUFFER_TRACE)

	__ASSERT_DEBUG((iCurrentDrainingBufferIndex >= 0 ),
							   Kern::Fault(KUsbPanicLdd, __LINE__));
		
	if (iDrainingOrder != iFillingOrderArray[iCurrentDrainingBufferIndex])
		{
        OstTrace1( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP6,
		        "!!! Out of Order Draining TDmaBuf::RxCopyDataToClient 10 draining=%d",
			    iCurrentDrainingBufferIndex);
		}
#endif
    OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP7,
            "TDmaBuf::RxCopyDataToClient 2" );

	TUint8* blockStartAddr = iCurrentDrainingBuffer + iCurrentPacketIndexArray[iCurrentPacket] + iExtractOffset;
	TUint8* lastEndAddr = blockStartAddr;					// going to track the contiguity of the memory
	TUint8* thisStartAddr = blockStartAddr;
	TInt toDo = Min(aLength - (TInt)aDestOffset, iTotalRxBytesAvail);
#if defined(USBC_LDD_BUFFER_TRACE)
	TInt bufnum = iCurrentDrainingBufferIndex;
#endif
	TInt errorCode = KErrNone;
	TBool isShortPacket = EFalse;
	const TInt maxPacketSizeMask = iMaxPacketSize - 1;
	do
		{
#if defined(USBC_LDD_BUFFER_TRACE)
		if (bufnum != iCurrentDrainingBufferIndex)
			{
			bufnum = iCurrentDrainingBufferIndex;
			if (iDrainingOrder != iFillingOrderArray[iCurrentDrainingBufferIndex])
				{
                OstTrace1( TRACE_NORMAL, TDMABUF_RXCOPYDATATOCLIENT_DUP8,
				        "!!! Out of Order Draining TDmaBuf::RxCopyDataToClient 20 draining=%d",
					    iCurrentDrainingBufferIndex);
				}
			}
#endif
		if (errorCode == KErrNone)
			{
			errorCode = GetCurrentError();
			}
		thisStartAddr = iCurrentDrainingBuffer + iCurrentPacketIndexArray[iCurrentPacket] + iExtractOffset;
		const TInt thisPacketSize = iCurrentPacketSizeArray[iCurrentPacket];
		const TInt size = thisPacketSize - iExtractOffset;
		if (aRUS)
			{
			if (iEndpointType == KUsbEpTypeBulk)
				{
                if(iExtractOffset & maxPacketSizeMask)
                	{
                    isShortPacket = ((size+iExtractOffset) < iMaxPacketSize) || ((size+iExtractOffset) & maxPacketSizeMask);
                	}
                else
                	{
                    isShortPacket = (size < iMaxPacketSize) || (size & maxPacketSizeMask);
                	}
				}
			else
				{
				// this 'if' block is arranged to avoid a division on packet sizes <= iMaxPacketSize
				isShortPacket = (size < iMaxPacketSize) ||
					((size > iMaxPacketSize) && (size % iMaxPacketSize));
				}
			}
		TInt copySize = Min(size, toDo);
		iExtractOffset += copySize;
		toDo -= copySize;
		if (thisStartAddr != lastEndAddr)
			{
			TInt bytesToCopy = lastEndAddr - blockStartAddr;
			TInt r=CopyToUser(aThread, blockStartAddr, bytesToCopy, aTcb, aDestOffset);
			if(r != KErrNone)
				Kern::ThreadKill(aThread, EExitPanic, r, KUsbLDDKillCat);
			blockStartAddr = thisStartAddr;
			}

		ModifyTotalRxBytesAvail(-copySize);
#if defined(USBC_LDD_BUFFER_TRACE)
		iNumberofBytesRxRemain[iCurrentDrainingBufferIndex] -= copySize;
#endif
		lastEndAddr = thisStartAddr + copySize;
		if (iExtractOffset == thisPacketSize)
			{
			// More data to copy, so need to access new packet
#if defined(USBC_LDD_BUFFER_TRACE)
			iNumberofPacketsRxRemain[iCurrentDrainingBufferIndex] -= 1;
#endif
			if (!AdvancePacket())
				{
				break;										// no more packets left
				}
			}
		} while (toDo > 0 && !isShortPacket);

	if (thisStartAddr != lastEndAddr)
		{
		TInt bytesToCopy = lastEndAddr - blockStartAddr;
		TInt r=CopyToUser(aThread, blockStartAddr, bytesToCopy, aTcb, aDestOffset);
		if(r != KErrNone)
			Kern::ThreadKill(aThread, EExitPanic, r, KUsbLDDKillCat);
		}

	// If we have transferred the requested amount of data it is still possible that
	// the next packet is a zlp which needs to be bumped over

	if (aRUS && (toDo == 0) && (iExtractOffset == 0) && (!isShortPacket) && (!IsReaderEmpty()) &&
		(PeekNextPacketSize() == 0))
		{
		// swallow a zlp
		isShortPacket = ETrue;
#if defined(USBC_LDD_BUFFER_TRACE)
		iNumberofPacketsRxRemain[iCurrentDrainingBufferIndex] -= 1;
#endif
		AdvancePacket();
		}
	aCompleteNow = isShortPacket || (((TInt)aDestOffset) == aLength) || (errorCode != KErrNone);

	FreeDrainedBuffers();

	// Use this error code to complete client read request
	return errorCode;
	}


inline TInt TDmaBuf::CopyToUser(DThread* aThread, const TUint8* aSourceAddr,
								TInt aLength, TClientBuffer *aTcb, TUint32& aDestOffset)
	{
	TPtrC8 des(aSourceAddr, aLength);
	TInt errorCode = Kern::ThreadBufWrite(aThread, aTcb, des, aDestOffset, KChunkShiftBy0, aThread);
	if (errorCode == KErrNone)
		{
		aDestOffset += aLength;
		}
	return errorCode;
	}


inline TInt TDmaBuf::NoRxPackets() const
	{
	return iTotalRxPacketsAvail;
	}


inline void TDmaBuf::IncrementBufferIndex(TInt& aIndex)
	{
	if (++aIndex == iNumberofBuffers)
		aIndex = 0;
	}


TBool TDmaBuf::NextDrainableBuffer()
	{
	TBool r = EFalse;
	if (iCurrentDrainingBufferIndex != KUsbcInvalidBufferIndex)
		{
		iCanBeFreed[iCurrentDrainingBufferIndex] = ETrue;
		iNumberofPacketsRx[iCurrentDrainingBufferIndex] = 0; // Current buffer is empty
		iNumberofBytesRx[iCurrentDrainingBufferIndex] = 0;	// Current buffer is empty

#if defined(USBC_LDD_BUFFER_TRACE)
		TUint& bytesRemain = iNumberofBytesRxRemain[iCurrentDrainingBufferIndex];
		TUint& pktsRemain = iNumberofPacketsRxRemain[iCurrentDrainingBufferIndex];
		if ((bytesRemain != 0) || (pktsRemain != 0))
			{
			OstTraceExt3( TRACE_NORMAL, TDMABUF_NEXTDRAINABLEBUFFER,
				    "TDmaBuf::NextDrainableBuffer: Error: data discarded buffer=%d pkts=%d bytes=%d",
				    iCurrentDrainingBufferIndex, pktsRemain, bytesRemain);
			bytesRemain = 0;
			pktsRemain = 0;
			}
#endif

		iCurrentDrainingBufferIndex = KUsbcInvalidBufferIndex;
		iCurrentPacket = KUsbcInvalidPacketIndex;
		}

	if (iDrainQueueIndex != KUsbcInvalidDrainQueueIndex)
		{
		r = ETrue;
		const TInt index = iDrainQueue[0];
		iDrainQueueIndex--;
		for (TInt i = 0; i < iNumberofBuffers; i++)
			{
			iDrainQueue[i] = iDrainQueue[i+1];
			}

#if defined(USBC_LDD_BUFFER_TRACE)
		if (index != KUsbcInvalidBufferIndex)
			iDrainingOrder++;
#endif

		iCurrentDrainingBufferIndex = index;
		iCurrentDrainingBuffer = iBuffers[index];
		iCurrentPacketIndexArray = iPacketIndex[index];
		iCurrentPacketSizeArray = iPacketSize[index];
		iCurrentPacket = 0;
		}
	return r;
	}


TInt TDmaBuf::PeekNextDrainableBuffer()
	{
	TInt r = KUsbcInvalidBufferIndex;
	if (iDrainQueueIndex != KUsbcInvalidDrainQueueIndex)
		{
		r = iDrainQueue[0];
		}
	return r;
	}


TBool TDmaBuf::NextFillableBuffer()
	{
	TBool r = EFalse;
	TInt index = iCurrentFillingBufferIndex;
	IncrementBufferIndex(index);
	// the sequence will restart at 0 if a buffer can't be found this time
	iCurrentFillingBufferIndex = 0;
	for (TInt i = 0; i < iNumberofBuffers; i++)
		{
		if (!iDrainable[index])
			{
			iCurrentFillingBufferIndex = index;
			r = ETrue;
			break;
			}
		IncrementBufferIndex(index);
		}
	return r;
	}


void TDmaBuf::FreeDrainedBuffers()
	{
	for (TInt i = 0; i < iNumberofBuffers; i++)
		{
		if (iDrainable[i] && iCanBeFreed[i])
			{
			iDrainable[i] = iCanBeFreed[i] = EFalse;
			}
		}
	}


TBool TDmaBuf::ShortPacketExists()
	{
	// Actually, a short packet or residue data
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_SHORTPACKETEXISTS,
	        "TDmaBuf::ShortPacketExists 1" );
	TInt index = iCurrentDrainingBufferIndex;
	TUsbcPacketArray* pktSizeArray = iCurrentPacketSizeArray;

	if (iMaxPacketSize > 0)
		{
		// No buffers available for draining
		if ((iCurrentDrainingBufferIndex == KUsbcInvalidBufferIndex) ||
			(iCurrentPacket == KUsbcInvalidPacketIndex))
			return EFalse;

		// Zlp waiting at tail
		if ((iTotalRxBytesAvail == 0) && (NoRxPackets() == 1))
			return ETrue;

		if (iEndpointType == KUsbEpTypeBulk)
			{
			const TInt mask = iMaxPacketSize - 1;


			for (TInt i = 0; i < iNumberofBuffers; i++)
				{
				if (index == KUsbcInvalidBufferIndex)
					break;
				if (iDrainable[index])
					{
					const TInt packetCount = iNumberofPacketsRx[index];
					const TInt lastPacketSize=pktSizeArray[packetCount - 1];
					if ((lastPacketSize < iMaxPacketSize) || (lastPacketSize & mask))
						{
						return ETrue;
						}
					}
				index = iDrainQueue[i];
				pktSizeArray = iPacketSize[index];
				}
			}
		else
			{
			if (iTotalRxBytesAvail % iMaxPacketSize)
				return ETrue;

			// residue==0; this can be because
			// zlps exist, or short packets combine to n * max_packet_size
			// This means spadework
			const TInt s = iCurrentPacketSizeArray[iCurrentPacket] - iExtractOffset;
			if ((s == 0) || (s % iMaxPacketSize))
				{
				return ETrue;
				}

			for (TInt i = 0; i < iNumberofBuffers; i++)
				{
				if (index == KUsbcInvalidBufferIndex)
					break;
				if (iDrainable[index])
					{
					const TInt packetCount = iNumberofPacketsRx[index];
					const TInt lastPacketSize = pktSizeArray[packetCount - 1];
					if ((lastPacketSize < iMaxPacketSize) || (lastPacketSize % iMaxPacketSize))
						{
						return ETrue;
						}
					}
				index = iDrainQueue[i];
				pktSizeArray = iPacketSize[index];
				}
			}
		}

	return EFalse;
	}


void TDmaBuf::AddToDrainQueue(TInt aBufferIndex)
	{
	if (iDrainQueue[iDrainQueueIndex + 1] != KUsbcInvalidBufferIndex)
		{
#if defined(USBC_LDD_BUFFER_TRACE)
		OstTrace0( TRACE_NORMAL, TDMABUF_ADDTODRAINQUEUE, "TDmaBuf::AddToDrainQueue: Error: invalid iDrainQueue[x]" );
#endif
		}
	iDrainQueue[++iDrainQueueIndex] = aBufferIndex;
	}


#if defined(USBC_LDD_BUFFER_TRACE)
TInt TDmaBuf::NoRxPacketsAlt() const
	{
	TInt pktCount = 0;
	for(TInt i = 0; i < iNumberofBuffers; i++)
		{
		if (iDrainable[i])
			{
			pktCount += iNumberofPacketsRxRemain[i];
			}
		}
	return pktCount;
	}


TInt TDmaBuf::NoRxBytesAlt() const
	{
	TInt byteCount = 0;
	for(TInt i = 0; i < iNumberofBuffers; i++)
		{
		if (iDrainable[i])
			{
			byteCount += iNumberofBytesRxRemain[i];
			}
		}
	return byteCount;
	}
#endif


// We only store 1 transaction, no other buffering is done
TInt TDmaBuf::TxStoreData(DThread* aThread, TClientBuffer *aTcb, TInt aTxLength, TUint32 aBufferOffset)
	{
	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_TXSTOREDATA,
	        "TDmaBuf::TxStoreData 1" );
	if (!IsReaderEmpty())
		return KErrInUse;

	OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TDMABUF_TXSTOREDATA_DUP1,
	        "TDmaBuf::TxStoreData 2" );
	TInt remainTxLength = aTxLength;
	TUint32 bufferOffset = aBufferOffset;
	// Store each buffer separately
	for( TInt i=0;(i<iNumberofBuffers)&&(remainTxLength>0);i++)
	    {
	    TUint8* logicalDest = iBuffers[i];
	    TInt xferSz = Min(remainTxLength, iBufSz);
	    TPtr8 des(logicalDest, xferSz, xferSz);
	    TInt r = Kern::ThreadBufRead(aThread, aTcb, des, bufferOffset, KChunkShiftBy0);
	    if(r != KErrNone)
	        {
	        Kern::ThreadKill(aThread, EExitPanic, r, KUsbLDDKillCat);
	        return r;
	        }
	    remainTxLength -= iBufSz;
	    bufferOffset += iBufSz;
	    }

	return KErrNone;
	}


TInt TDmaBuf::TxGetNextXfer(TUint8*& aBufferAddr, TInt& aTxLength, TPhysAddr& aBufferPhys)
	{
	if (iTxActive)
		return KErrInUse;

	aBufferAddr = iBuffers[0];								// only 1 tx buffer
	aBufferPhys = iBufferPhys[0];
	aTxLength = BufferTotalSize();

	return KErrNone;
	}

