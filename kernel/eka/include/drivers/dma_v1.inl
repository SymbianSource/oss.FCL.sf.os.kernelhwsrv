// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0""
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// include/drivers/dma_v1.inl
// DMA framework public inline functions
// This file should not be modified when porting the DMA framework to
// new hardware.
// TDmaChannel
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

inline void TDmaChannel::Wait()
	{
	NKern::FMWait(&iLock);
	}

inline void TDmaChannel::Signal()
	{
	NKern::FMSignal(&iLock);
	}

inline TBool TDmaChannel::Flash()
	{
	return NKern::FMFlash(&iLock);
	}

inline TBool TDmaChannel::IsOpened() const
	{
	return iController != NULL;
	}

inline TBool TDmaChannel::IsQueueEmpty() const
	{
	return const_cast<TDmaChannel*>(this)->iReqQ.IsEmpty();
	}

inline TUint32 TDmaChannel::PslId() const
	{
	return iPslId;
	}

inline TInt TDmaChannel::FailNext(TInt /*aFragmentCount*/)
	{
	return iController->FailNext(*this);
	}

inline TInt TDmaChannel::MissNextInterrupts(TInt aInterruptCount)
	{
	return iController->MissNextInterrupts(*this, aInterruptCount);
	}

/** Function allowing platform-specific layer to extend API with new
	channel-specific operations.
 	@param aCmd Command identifier.  Negative values are reserved for Symbian use.
	@param aArg PSL-specific
	@return KErrNotSupported if aCmd is not supported.  PSL-specific value otherwise.
 */

inline TInt TDmaChannel::Extension(TInt aCmd, TAny* aArg)
	{
	return iController->Extension(*this, aCmd, aArg);
	}

inline const TDmac* TDmaChannel::Controller() const
	{
	return iController;
	}

inline TInt TDmaChannel::MaxTransferSize(TUint aFlags, TUint32 aPslInfo)
	{
	return iController->MaxTransferSize(*this, aFlags, aPslInfo);
	}

inline TUint TDmaChannel::MemAlignMask(TUint aFlags, TUint32 aPslInfo)
	{
	return iController->MemAlignMask(*this, aFlags, aPslInfo);
	}

// DDmaRequest

/** Called when request is removed from request queue in channel */

inline void DDmaRequest::OnDeque()
	{
	iQueued = EFalse;
	iLastHdr->iNext = NULL;
	iChannel.DoUnlink(*iLastHdr);
	}

// TDmac

inline void TDmac::Wait()
	{
	NKern::FMWait(&iLock);
	}

inline void TDmac::Signal()
	{
	NKern::FMSignal(&iLock);
	}

inline SDmaPseudoDes& TDmac::HdrToDes(const SDmaDesHdr& aHdr) const
	{
	return static_cast<SDmaPseudoDes*>(iDesPool)[&aHdr - iHdrPool];
	}

inline TAny* TDmac::HdrToHwDes(const SDmaDesHdr& aHdr) const
	{
	return static_cast<TUint8*>(iDesPool) + iDesSize*(&aHdr - iHdrPool);
	}

inline TUint32 TDmac::DesLinToPhys(TAny* aDes) const
	{
#ifdef __WINS__
	(void)aDes;
	return 0xDEADBEEF;
#else
	return iHwDesChunk->iPhysAddr + ((TLinAddr)aDes - iHwDesChunk->iLinAddr);
#endif
	}

// DmaChannelMgr

inline void DmaChannelMgr::Wait()
	{
	NKern::FMWait(&Lock);
	}

inline void DmaChannelMgr::Signal()
	{
	NKern::FMSignal(&Lock);
	}

//---
