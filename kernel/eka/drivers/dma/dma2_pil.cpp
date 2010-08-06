// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32/drivers/dma2_pil.cpp
// DMA Platform Independent Layer (PIL)
//
//

#include <drivers/dma.h>
#include <drivers/dma_hai.h>

#include <kernel/kern_priv.h>


// Symbian _Min() & _Max() are broken, so we have to define them ourselves
inline TUint _Min(TUint aLeft, TUint aRight)
	{return(aLeft < aRight ? aLeft : aRight);}
inline TUint _Max(TUint aLeft, TUint aRight)
	{return(aLeft > aRight ? aLeft : aRight);}


// The following section is used only when freezing the DMA2 export library
/*
TInt DmaChannelMgr::StaticExtension(TInt, TAny*) {return 0;}
TDmaChannel* DmaChannelMgr::Open(TUint32, TBool, TUint) {return 0;}
void DmaChannelMgr::Close(TDmaChannel*) {}
EXPORT_C const TDmaTestInfo& DmaTestInfo() {static TDmaTestInfo a; return a;}
EXPORT_C const TDmaV2TestInfo& DmaTestInfoV2() {static TDmaV2TestInfo a; return a;}
*/

static const char KDmaPanicCat[] = "DMA " __FILE__;

//////////////////////////////////////////////////////////////////////
// DmaChannelMgr
//
// Wait, Signal, and Initialise are defined here in the PIL.
// Open, Close and Extension must be defined in the PSL.

NFastMutex DmaChannelMgr::Lock;


void DmaChannelMgr::Wait()
	{
	NKern::FMWait(&Lock);
	}


void DmaChannelMgr::Signal()
	{
	NKern::FMSignal(&Lock);
	}


TInt DmaChannelMgr::Initialise()
	{
	return KErrNone;
	}


class TDmaCancelInfo : public SDblQueLink
	{
public:
	TDmaCancelInfo();
	void Signal();
public:
	NFastSemaphore iSem;
	};


TDmaCancelInfo::TDmaCancelInfo()
	: iSem(0)
	{
	iNext = this;
	iPrev = this;
	}


void TDmaCancelInfo::Signal()
	{
	TDmaCancelInfo* p = this;
	FOREVER
		{
		TDmaCancelInfo* next = (TDmaCancelInfo*)p->iNext;
		if (p!=next)
			p->Deque();
		NKern::FSSignal(&p->iSem);	// Don't dereference p after this
		if (p==next)
			break;
		p = next;
		}
	}


//////////////////////////////////////////////////////////////////////////////

#ifdef __DMASIM__
#ifdef __WINS__
typedef TLinAddr TPhysAddr;
#endif
static inline TPhysAddr LinToPhys(TLinAddr aLin) {return aLin;}
#else
static inline TPhysAddr LinToPhys(TLinAddr aLin) {return Epoc::LinearToPhysical(aLin);}
#endif

//
// Return minimum of aMaxSize and size of largest physically contiguous block
// starting at aLinAddr.
//
static TUint MaxPhysSize(TLinAddr aLinAddr, const TUint aMaxSize)
	{
	const TPhysAddr physBase = LinToPhys(aLinAddr);
	__DMA_ASSERTD(physBase != KPhysAddrInvalid);
	TLinAddr lin = aLinAddr;
	TUint size = 0;
	for (;;)
		{
		// Round up the linear address to the next MMU page boundary
		const TLinAddr linBoundary = Kern::RoundToPageSize(lin + 1);
		size += linBoundary - lin;
		if (size >= aMaxSize)
			return aMaxSize;
		if ((physBase + size) != LinToPhys(linBoundary))
			return size;
		lin = linBoundary;
		}
	}


//////////////////////////////////////////////////////////////////////////////
// TDmac

TDmac::TDmac(const SCreateInfo& aInfo)
	: iMaxDesCount(aInfo.iDesCount),
	  iAvailDesCount(aInfo.iDesCount),
	  iHdrPool(NULL),
#ifndef __WINS__
	  iHwDesChunk(NULL),
#endif
	  iDesPool(NULL),
	  iDesSize(aInfo.iDesSize),
	  iCapsHwDes(aInfo.iCapsHwDes),
	  iFreeHdr(NULL)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::TDmac"));
	__DMA_ASSERTD(iMaxDesCount > 0);
	__DMA_ASSERTD(iDesSize > 0);
	}


//
// Second-phase c'tor
//
TInt TDmac::Create(const SCreateInfo& aInfo)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::Create"));
	iHdrPool = new SDmaDesHdr[iMaxDesCount];
	if (iHdrPool == NULL)
		{
		return KErrNoMemory;
		}

	TInt r = AllocDesPool(aInfo.iDesChunkAttribs);
	if (r != KErrNone)
		{
		return KErrNoMemory;
		}

	// Link all descriptor headers together on the free list
	iFreeHdr = iHdrPool;
	for (TInt i = 0; i < iMaxDesCount - 1; i++)
		iHdrPool[i].iNext = iHdrPool + i + 1;
	iHdrPool[iMaxDesCount-1].iNext = NULL;

	__DMA_INVARIANT();
	return KErrNone;
	}


TDmac::~TDmac()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::~TDmac"));
	__DMA_INVARIANT();

	FreeDesPool();
	delete[] iHdrPool;
	}


void TDmac::Transfer(const TDmaChannel& /*aChannel*/, const SDmaDesHdr& /*aHdr*/)
	{
	// TDmac needs to override this function if it has reported the channel
	// type for which the PIL calls it.
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmac::Transfer(const TDmaChannel& /*aChannel*/, const SDmaDesHdr& /*aSrcHdr*/,
					 const SDmaDesHdr& /*aDstHdr*/)
	{
	// TDmac needs to override this function if it has reported the channel
	// type for which the PIL calls it.
	__DMA_UNREACHABLE_DEFAULT();
	}


TInt TDmac::PauseTransfer(const TDmaChannel& /*aChannel*/)
	{
	// TDmac needs to override this function if it has reported support for
	// channel pausing/resuming.
	return KErrNotSupported;
	}


TInt TDmac::ResumeTransfer(const TDmaChannel& /*aChannel*/)
	{
	// TDmac needs to override this function if it has reported support for
	// channel pausing/resuming.
	return KErrNotSupported;
	}


TInt TDmac::AllocDesPool(TUint aAttribs)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::AllocDesPool"));
	// Calling thread must be in CS
	__ASSERT_CRITICAL;
	TInt r;
	if (iCapsHwDes)
		{
		const TInt size = iMaxDesCount * iDesSize;
#ifdef __WINS__
		(void)aAttribs;
		iDesPool = new TUint8[size];
		r = iDesPool ? KErrNone : KErrNoMemory;
#else
		// Chunk not mapped as supervisor r/w user none? incorrect mask passed by PSL
		__DMA_ASSERTD((aAttribs & EMapAttrAccessMask) == EMapAttrSupRw);
		TPhysAddr phys;
		r = Epoc::AllocPhysicalRam(size, phys);
		if (r == KErrNone)
			{
			r = DPlatChunkHw::New(iHwDesChunk, phys, size, aAttribs);
			if (r == KErrNone)
				{
				iDesPool = (TAny*)iHwDesChunk->LinearAddress();
				__KTRACE_OPT(KDMA, Kern::Printf("descriptor hw chunk created lin=0x%08X phys=0x%08X, size=0x%X",
												iHwDesChunk->iLinAddr, iHwDesChunk->iPhysAddr, size));
				}
			else
				Epoc::FreePhysicalRam(phys, size);
			}
#endif
		}
	else
		{
		iDesPool = Kern::Alloc(iMaxDesCount * sizeof(TDmaTransferArgs));
		r = iDesPool ? KErrNone : KErrNoMemory;
		}
	return r;
	}


void TDmac::FreeDesPool()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::FreeDesPool"));
	// Calling thread must be in CS
	__ASSERT_CRITICAL;
	if (iCapsHwDes)
		{
#ifdef __WINS__
		delete[] iDesPool;
#else
		if (iHwDesChunk)
			{
			const TPhysAddr phys = iHwDesChunk->PhysicalAddress();
			const TInt size = iHwDesChunk->iSize;
			iHwDesChunk->Close(NULL);
			Epoc::FreePhysicalRam(phys, size);
			}
#endif
		}
	else
		{
		Kern::Free(iDesPool);
		}
	}


//
// Prealloc the given number of descriptors.
//
TInt TDmac::ReserveSetOfDes(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::ReserveSetOfDes count=%d", aCount));
	__DMA_ASSERTD(aCount > 0);
	TInt r = KErrTooBig;
	Wait();
	if (iAvailDesCount - aCount >= 0)
		{
		iAvailDesCount -= aCount;
		r = KErrNone;
		}
	Signal();
	__DMA_INVARIANT();
	return r;
	}


//
// Return the given number of preallocated descriptors to the free pool.
//
void TDmac::ReleaseSetOfDes(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::ReleaseSetOfDes count=%d", aCount));
	__DMA_ASSERTD(aCount >= 0);
	Wait();
	iAvailDesCount += aCount;
	Signal();
	__DMA_INVARIANT();
	}


//
// Queue DFC and update word used to communicate with channel DFC.
//
// Called in interrupt context by PSL.
//
void TDmac::HandleIsr(TDmaChannel& aChannel, TUint aEventMask, TBool aIsComplete)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::HandleIsr"));

	// Function needs to be called by PSL in ISR context
	__DMA_ASSERTD(NKern::CurrentContext() == NKern::EInterrupt);

	// First the ISR callback stuff

	// Is this a transfer completion notification?
	if (aEventMask & EDmaCallbackRequestCompletion)
		{
		// If so, has the client requested an ISR callback?
		if (__e32_atomic_load_acq32(&aChannel.iIsrCbRequest))
			{
			__KTRACE_OPT(KDMA, Kern::Printf("ISR callback"));

			// Since iIsrCbRequest was set no threads will be
			// modifying the request queue.
			const DDmaRequest* const req = _LOFF(aChannel.iReqQ.First(), DDmaRequest, iLink);

			// We expect the request to have requested
			// ISR callback
			__NK_ASSERT_DEBUG(req->iIsrCb);

			TDmaCallback const cb = req->iDmaCb;
			TAny* const arg = req->iDmaCbArg;
			// Execute the client callback
			(*cb)(EDmaCallbackRequestCompletion,
				  (aIsComplete ? EDmaResultOK : EDmaResultError),
				  arg,
				  NULL);
			// Now let's see if the callback rescheduled the transfer request
			// (see TDmaChannel::IsrRedoRequest()).
			const TBool redo = aChannel.iRedoRequest;
			aChannel.iRedoRequest = EFalse;
			const TBool stop = __e32_atomic_load_acq32(&aChannel.iIsrDfc) &
				(TUint32)TDmaChannel::KCancelFlagMask;
			// There won't be another ISR callback if this callback didn't
			// reschedule the request, or the client cancelled all requests, or
			// this callback rescheduled the request with a DFC callback.
			if (!redo || stop || !req->iIsrCb)
				{
				__e32_atomic_store_rel32(&aChannel.iIsrCbRequest, EFalse);
				}
			if (redo && !stop)
				{
				// We won't queue the channel DFC in this case and just return.
				__KTRACE_OPT(KDMA, Kern::Printf("CB rescheduled xfer -> no DFC"));
				return;
				}
			// Not redoing or being cancelled means we've been calling the
			// request's ISR callback for the last time. We're going to
			// complete the request via the DFC in the usual way.
			}
		}
	else
		{
		// The PIL doesn't support yet any completion types other than
		// EDmaCallbackRequestCompletion.
		__DMA_CANT_HAPPEN();
		}

	// Now queue a DFC if necessary. The possible scenarios are:
	// a) DFC not queued (orig == 0)              -> update iIsrDfc + queue DFC
	// b) DFC queued, not running yet (orig != 0) -> just update iIsrDfc
	// c) DFC running / iIsrDfc not reset yet (orig != 0) -> just update iIsrDfc
	// d) DFC running / iIsrDfc already reset (orig == 0) -> update iIsrDfc + requeue DFC

	// Set error flag if necessary.
	const TUint32 inc = aIsComplete ? 1u : TUint32(TDmaChannel::KErrorFlagMask) | 1u;

	// Add 'inc' (interrupt count increment + poss. error flag) to 'iIsrDfc' if
	// cancel flag is not set, do nothing otherwise. Assign original value of
	// 'iIsrDfc' to 'orig' in any case.
	const TUint32 orig = __e32_atomic_tau_ord32(&aChannel.iIsrDfc,
												TUint32(TDmaChannel::KCancelFlagMask),
												0,
												inc);

	// As transfer should be suspended when an error occurs, we
	// should never get there with the error flag already set.
	__DMA_ASSERTD((orig & inc & (TUint32)TDmaChannel::KErrorFlagMask) == 0);

	if (orig == 0)
		{
		aChannel.iDfc.Add();
		}
	}


TInt TDmac::InitDes(const SDmaDesHdr& aHdr, const TDmaTransferArgs& aTransferArgs)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::InitDes"));
	TInt r;
	if (iCapsHwDes)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("iCaps.iHwDescriptors"));
		r = InitHwDes(aHdr, aTransferArgs);
		}
	else
		{
		TDmaTransferArgs& args = HdrToDes(aHdr);
		args = aTransferArgs;
		r = KErrNone;
		}
	return r;
	}


TInt TDmac::InitHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/)
	{
	// concrete controller must override if SDmacCaps::iHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


TInt TDmac::InitSrcHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/)
	{
	// concrete controller must override if SDmacCaps::iAsymHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


TInt TDmac::InitDstHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/)
	{
	// concrete controller must override if SDmacCaps::iAsymHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


TInt TDmac::UpdateDes(const SDmaDesHdr& aHdr, TUint32 aSrcAddr, TUint32 aDstAddr,
					  TUint aTransferCount, TUint32 aPslRequestInfo)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmac::UpdateDes"));
	TInt r;
	if (iCapsHwDes)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("iCaps.iHwDescriptors"));
		r = UpdateHwDes(aHdr, aSrcAddr, aDstAddr, aTransferCount, aPslRequestInfo);
		}
	else
		{
		TDmaTransferArgs& args = HdrToDes(aHdr);
		if (aSrcAddr != KPhysAddrInvalid)
			args.iSrcConfig.iAddr = aSrcAddr;
		if (aDstAddr != KPhysAddrInvalid)
			args.iDstConfig.iAddr = aDstAddr;
		if (aTransferCount)
			args.iTransferCount = aTransferCount;
		if (aPslRequestInfo)
			args.iPslRequestInfo = aPslRequestInfo;
		r = KErrNone;
		}
	return r;
	}


TInt TDmac::UpdateHwDes(const SDmaDesHdr& /*aHdr*/, TUint32 /*aSrcAddr*/, TUint32 /*aDstAddr*/,
						TUint /*aTransferCount*/, TUint32 /*aPslRequestInfo*/)
	{
	// concrete controller must override if SDmacCaps::iHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


TInt TDmac::UpdateSrcHwDes(const SDmaDesHdr& /*aHdr*/, TUint32 /*aSrcAddr*/,
						   TUint /*aTransferCount*/, TUint32 /*aPslRequestInfo*/)
	{
	// concrete controller must override if SDmacCaps::iAsymHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


TInt TDmac::UpdateDstHwDes(const SDmaDesHdr& /*aHdr*/, TUint32 /*aDstAddr*/,
						   TUint /*aTransferCount*/, TUint32 /*aPslRequestInfo*/)
	{
	// concrete controller must override if SDmacCaps::iAsymHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	return KErrGeneral;
	}


void TDmac::ChainHwDes(const SDmaDesHdr& /*aHdr*/, const SDmaDesHdr& /*aNextHdr*/)
	{
	// concrete controller must override if SDmacCaps::iHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmac::AppendHwDes(const TDmaChannel& /*aChannel*/, const SDmaDesHdr& /*aLastHdr*/,
						const SDmaDesHdr& /*aNewHdr*/)
	{
 	// concrete controller must override if SDmacCaps::iHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmac::AppendHwDes(const TDmaChannel& /*aChannel*/,
						const SDmaDesHdr& /*aSrcLastHdr*/, const SDmaDesHdr& /*aSrcNewHdr*/,
						const SDmaDesHdr& /*aDstLastHdr*/, const SDmaDesHdr& /*aDstNewHdr*/)
	{
	// concrete controller must override if SDmacCaps::iAsymHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmac::UnlinkHwDes(const TDmaChannel& /*aChannel*/, SDmaDesHdr& /*aHdr*/)
	{
 	// concrete controller must override if SDmacCaps::iHwDescriptors set
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmac::ClearHwDes(const SDmaDesHdr& /*aHdr*/)
	{
	// default implementation - NOP; concrete controller may override
	return;
	}


TInt TDmac::LinkChannels(TDmaChannel& /*a1stChannel*/, TDmaChannel& /*a2ndChannel*/)
	{
	// default implementation - NOP; concrete controller may override
	return KErrNotSupported;
	}


TInt TDmac::UnlinkChannel(TDmaChannel& /*aChannel*/)
	{
	// default implementation - NOP; concrete controller may override
	return KErrNotSupported;
	}


TInt TDmac::FailNext(const TDmaChannel& /*aChannel*/)
	{
	// default implementation - NOP; concrete controller may override
	return KErrNotSupported;
	}


TInt TDmac::MissNextInterrupts(const TDmaChannel& /*aChannel*/, TInt /*aInterruptCount*/)
	{
	// default implementation - NOP; concrete controller may override
	return KErrNotSupported;
	}


TInt TDmac::Extension(TDmaChannel& /*aChannel*/, TInt /*aCmd*/, TAny* /*aArg*/)
	{
	// default implementation - NOP; concrete controller may override
	return KErrNotSupported;
	}


TUint32 TDmac::HwDesNumDstElementsTransferred(const SDmaDesHdr& /*aHdr*/)
	{
 	// Concrete controller must override if SDmacCaps::iHwDescriptors set.
	__DMA_UNREACHABLE_DEFAULT();
	return 0;
	}


TUint32 TDmac::HwDesNumSrcElementsTransferred(const SDmaDesHdr& /*aHdr*/)
	{
 	// Concrete controller must override if SDmacCaps::iHwDescriptors set.
	__DMA_UNREACHABLE_DEFAULT();
	return 0;
	}


#ifdef _DEBUG

void TDmac::Invariant()
	{
	Wait();
	__DMA_ASSERTD(0 <= iAvailDesCount && iAvailDesCount <= iMaxDesCount);
	__DMA_ASSERTD(!iFreeHdr || IsValidHdr(iFreeHdr));
	for (TInt i = 0; i < iMaxDesCount; i++)
		__DMA_ASSERTD(iHdrPool[i].iNext == NULL || IsValidHdr(iHdrPool[i].iNext));
	Signal();
	}


TBool TDmac::IsValidHdr(const SDmaDesHdr* aHdr)
	{
	return (iHdrPool <= aHdr) && (aHdr < iHdrPool + iMaxDesCount);
	}

#endif


//
// Internal compat version, used by legacy Fragment()
//
TDmaTransferConfig::TDmaTransferConfig(TUint32 aAddr, TUint aFlags, TBool aAddrInc)
	: iAddr(aAddr),
	  iAddrMode(aAddrInc ? KDmaAddrModePostIncrement : KDmaAddrModeConstant),
	  iElementSize(0),
	  iElementsPerFrame(0),
	  iElementsPerPacket(0),
	  iFramesPerTransfer(0),
	  iElementSkip(0),
	  iFrameSkip(0),
	  iBurstSize(KDmaBurstSizeAny),
	  iFlags(aFlags),
	  iSyncFlags(KDmaSyncAuto),
	  iPslTargetInfo(0),
	  iRepeatCount(0),
	  iDelta(~0u),
	  iReserved(0)
	{
	__KTRACE_OPT(KDMA,
				 Kern::Printf("TDmaTransferConfig::TDmaTransferConfig "
							  "aAddr=0x%08X aFlags=0x%08X aAddrInc=%d",
							  aAddr, aFlags, aAddrInc));
	}


//
// Internal compat version, used by legacy Fragment()
//
TDmaTransferArgs::TDmaTransferArgs(TUint32 aSrc, TUint32 aDest, TInt aCount,
								   TUint aFlags, TUint32 aPslInfo)
	: iSrcConfig(aSrc, RequestFlags2SrcConfigFlags(aFlags), (aFlags & KDmaIncSrc)),
	  iDstConfig(aDest, RequestFlags2DstConfigFlags(aFlags), (aFlags & KDmaIncDest)),
	  iTransferCount(aCount),
	  iGraphicsOps(KDmaGraphicsOpNone),
	  iColour(0),
	  iFlags(0),
	  iChannelPriority(KDmaPriorityNone),
	  iPslRequestInfo(aPslInfo),
	  iChannelCookie(0),
	  iDelta(~0u),
	  iReserved1(0),
	  iReserved2(0)
	{
	__KTRACE_OPT(KDMA,
				 Kern::Printf("TDmaTransferArgs::TDmaTransferArgs"));
	__KTRACE_OPT(KDMA,
				 Kern::Printf("  aSrc=0x%08X aDest=0x%08X aCount=%d aFlags=0x%08X aPslInfo=0x%08X",
							  aSrc, aDest, aCount, aFlags, aPslInfo));
	}


//
// As DDmaRequest is derived from DBase, the initializations with zero aren't
// strictly necessary here, but this way it's nicer.
//
EXPORT_C DDmaRequest::DDmaRequest(TDmaChannel& aChannel, TCallback aCb,
								  TAny* aCbArg, TInt aMaxTransferSize)
	: iChannel(aChannel),
	  iCb(aCb),
	  iCbArg(aCbArg),
	  iDmaCb(NULL),
	  iDmaCbArg(NULL),
	  iIsrCb(EFalse),
	  iDesCount(0),
	  iFirstHdr(NULL),
	  iLastHdr(NULL),
	  iSrcDesCount(0),
	  iSrcFirstHdr(NULL),
	  iSrcLastHdr(NULL),
	  iDstDesCount(0),
	  iDstFirstHdr(NULL),
	  iDstLastHdr(NULL),
	  iQueued(EFalse),
	  iMaxTransferSize(aMaxTransferSize),
	  iTotalNumSrcElementsTransferred(0),
	  iTotalNumDstElementsTransferred(0)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::DDmaRequest =0x%08X (old style)", this));
	iChannel.iReqCount++;
	__DMA_ASSERTD(0 <= aMaxTransferSize);
	__DMA_INVARIANT();
	}


//
// As DDmaRequest is derived from DBase, the initializations with zero aren't
// strictly necessary here, but this way it's nicer.
//
EXPORT_C DDmaRequest::DDmaRequest(TDmaChannel& aChannel, TDmaCallback aDmaCb,
								  TAny* aCbArg, TUint aMaxTransferSize)
	: iChannel(aChannel),
	  iCb(NULL),
	  iCbArg(NULL),
	  iDmaCb(aDmaCb),
	  iDmaCbArg(aCbArg),
	  iIsrCb(EFalse),
	  iDesCount(0),
	  iFirstHdr(NULL),
	  iLastHdr(NULL),
	  iSrcDesCount(0),
	  iSrcFirstHdr(NULL),
	  iSrcLastHdr(NULL),
	  iDstDesCount(0),
	  iDstFirstHdr(NULL),
	  iDstLastHdr(NULL),
	  iQueued(EFalse),
	  iMaxTransferSize(aMaxTransferSize),
	  iTotalNumSrcElementsTransferred(0),
	  iTotalNumDstElementsTransferred(0)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::DDmaRequest =0x%08X (new style)", this));
	__e32_atomic_add_ord32(&iChannel.iReqCount, 1);
	__DMA_INVARIANT();
	}


EXPORT_C DDmaRequest::~DDmaRequest()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::~DDmaRequest"));
	__DMA_ASSERTD(!iQueued);
	__DMA_INVARIANT();
	if (iChannel.iDmacCaps->iAsymHwDescriptors)
		{
		FreeSrcDesList();
		FreeDstDesList();
		}
	else
		{
		FreeDesList();
		}
	__e32_atomic_add_ord32(&iChannel.iReqCount, TUint32(-1));
	}


EXPORT_C TInt DDmaRequest::Fragment(TUint32 aSrc, TUint32 aDest, TInt aCount,
									TUint aFlags, TUint32 aPslInfo)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Fragment thread %O (old style)",
									&Kern::CurrentThread()));

	__DMA_ASSERTD(aCount > 0);

	TDmaTransferArgs args(aSrc, aDest, aCount, aFlags, aPslInfo);

	return Frag(args);
	}


EXPORT_C TInt DDmaRequest::Fragment(const TDmaTransferArgs& aTransferArgs)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Fragment thread %O (new style)",
									&Kern::CurrentThread()));

	// Writable temporary working copy of the transfer arguments.
	// We need this because we may have to modify some fields before passing it
	// to the PSL (for example iChannelCookie, iTransferCount,
	// iDstConfig::iAddr, and iSrcConfig::iAddr).
	TDmaTransferArgs args(aTransferArgs);

	return Frag(args);
	}


TInt DDmaRequest::CheckTransferConfig(const TDmaTransferConfig& aTarget, TUint aCount) const
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::CheckTransferConfig"));

	if (aTarget.iElementSize != 0)
		{
		if ((aCount % aTarget.iElementSize) != 0)
			{
			// 2, 7 (These strange numbers refer to some test cases documented
			// elsewhere - they will be removed eventually.)
			__KTRACE_OPT(KPANIC,
						 Kern::Printf("Error: ((aCount %% iElementSize) != 0)"));
			return KErrArgument;
			}
		if (aTarget.iElementsPerFrame != 0)
			{
			if ((aTarget.iElementSize * aTarget.iElementsPerFrame *
				 aTarget.iFramesPerTransfer) != aCount)
				{
				// 3, 8
				__KTRACE_OPT(KPANIC,
							 Kern::Printf("Error: ((iElementSize * "
										  "iElementsPerFrame * "
										  "iFramesPerTransfer) != aCount)"));
				return KErrArgument;
				}
			}
		}
	else
		{
		if (aTarget.iElementsPerFrame != 0)
			{
			// 4, 9
			__KTRACE_OPT(KPANIC,
						 Kern::Printf("Error: (iElementsPerFrame != 0)"));
			return KErrArgument;
			}
		if (aTarget.iFramesPerTransfer != 0)
			{
			// 5, 10
			__KTRACE_OPT(KPANIC,
						 Kern::Printf("Error: (iFramesPerTransfer != 0)"));
			return KErrArgument;
			}
		if (aTarget.iElementsPerPacket != 0)
			{
			// 6, 11
			__KTRACE_OPT(KPANIC,
						 Kern::Printf("Error: (iElementsPerPacket != 0)"));
			return KErrArgument;
			}
		}
	return KErrNone;
	}


TInt DDmaRequest::CheckMemFlags(const TDmaTransferConfig& aTarget) const
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::CheckMemFlags"));

	const TBool mem_target = (aTarget.iFlags & KDmaMemAddr);

	if (mem_target && (aTarget.iFlags & KDmaPhysAddr) && !(aTarget.iFlags & KDmaMemIsContiguous))
		{
		// Physical memory address implies contiguous range
		// 13, 15
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: mem_target && KDmaPhysAddr && !KDmaMemIsContiguous"));
		return KErrArgument;
		}
	else if ((aTarget.iFlags & KDmaMemIsContiguous) && !mem_target)
		{
		// Contiguous range implies memory address
		// 14, 16
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: KDmaMemIsContiguous && !mem_target"));
		return KErrArgument;
		}
	return KErrNone;
	}


// Makes sure an element or frame never straddles two DMA subtransfer
// fragments. This would be a fragmentation error by the PIL.
//
TInt DDmaRequest::AdjustFragmentSize(TUint& aFragSize, TUint aElementSize,
									 TUint aFrameSize)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::AdjustFragmentSize FragSize=%d ES=%d FS=%d",
									aFragSize, aElementSize, aFrameSize));

	TUint rem = 0;
	TInt r = KErrNone;

	FOREVER
		{
		// If an element size is defined, make sure the fragment size is
		// greater or equal.
		if (aElementSize)
			{
			if (aFragSize < aElementSize)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("Error: aFragSize < aElementSize"));
				r = KErrArgument;
				break;
				}
			}
		// If a frame size is defined, make sure the fragment size is greater
		// or equal.
		if (aFrameSize)
			{
			if (aFragSize < aFrameSize)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("Error: aFragSize < aFrameSize"));
				r = KErrArgument;
				break;
				}
			}
		// If a frame size is defined, make sure the fragment ends on a frame
		// boundary.
		if (aFrameSize)
			{
			rem = aFragSize % aFrameSize;
			if (rem != 0)
				{
				aFragSize -= rem;
				// 20, 22
				__KTRACE_OPT(KDMA, Kern::Printf("aFragSize %% aFrameSize != 0 --> aFragSize = %d",
												aFragSize));
				// aFragSize has changed, so we have to do all the checks
				// again.
				continue;
				}
			}
		// If an element size is defined, make sure the fragment ends on an
		// element boundary.
		if (aElementSize)
			{
			rem = aFragSize % aElementSize;
			if (rem != 0)
				{
				aFragSize -= rem;
				// 21, 23
				__KTRACE_OPT(KDMA, Kern::Printf("aFragSize %% aElementSize != 0 --> aFragSize = %d",
												aFragSize));
				// aFragSize has changed, so we have to do all the checks
				// again.
				continue;
				}
			}
		// Done - all checks passed. Let's get out.
		break;
		}

	return r;
	}


TUint DDmaRequest::GetTransferCount(const TDmaTransferArgs& aTransferArgs) const
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::GetTransferCount"));

	const TDmaTransferConfig& src = aTransferArgs.iSrcConfig;
#ifdef _DEBUG
	const TDmaTransferConfig& dst = aTransferArgs.iDstConfig;
#endif	// #ifdef _DEBUG

	TUint count = aTransferArgs.iTransferCount;
	if (count == 0)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("iTransferCount == 0"));
		count = src.iElementSize * src.iElementsPerFrame *
			src.iFramesPerTransfer;
#ifdef _DEBUG
		const TUint dst_cnt = dst.iElementSize * dst.iElementsPerFrame *
			dst.iFramesPerTransfer;
		if (count != dst_cnt)
			{
			// 1
			__KTRACE_OPT(KPANIC, Kern::Printf("Error: (count != dst_cnt)"));
			return 0;
			}
#endif	// #ifdef _DEBUG
		}
	else
		{
		__KTRACE_OPT(KDMA, Kern::Printf("iTransferCount == %d", count));
#ifdef _DEBUG
		// Client shouldn't specify contradictory or incomplete things
		if (CheckTransferConfig(src, count) != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("Error: CheckTransferConfig(src)"));
			return 0;
			}
		if (CheckTransferConfig(dst, count) != KErrNone)
			{
			__KTRACE_OPT(KPANIC, Kern::Printf("Error: CheckTransferConfig(dst)"));
			return 0;
			}
#endif	// #ifdef _DEBUG
		}
	return count;
	}


TUint DDmaRequest::GetMaxTransferlength(const TDmaTransferArgs& aTransferArgs, TUint aCount) const
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::GetMaxTransferlength"));

	const TDmaTransferConfig& src = aTransferArgs.iSrcConfig;
	const TDmaTransferConfig& dst = aTransferArgs.iDstConfig;

	// Ask the PSL what the maximum length is for a single transfer
	TUint max_xfer_len = iChannel.MaxTransferLength(src.iFlags, dst.iFlags,
													aTransferArgs.iPslRequestInfo);
	if (iMaxTransferSize)
		{
		// (User has set a transfer size cap)
		__KTRACE_OPT(KDMA, Kern::Printf("iMaxTransferSize: %d", iMaxTransferSize));
		if ((max_xfer_len != 0) && (iMaxTransferSize > max_xfer_len))
			{
			// Not really an error, but still...
			__KTRACE_OPT(KPANIC, Kern::Printf("Warning: iMaxTransferSize > max_xfer_len"));
			}
		max_xfer_len = iMaxTransferSize;
		}
	else
		{
		// (User doesn't care about max transfer size)
		if (max_xfer_len == 0)
			{
			// '0' = no maximum imposed by controller
			max_xfer_len = aCount;
			}
		}
	__KTRACE_OPT(KDMA, Kern::Printf("max_xfer_len: %d", max_xfer_len));

	// Some sanity checks
#ifdef _DEBUG
	if ((max_xfer_len < src.iElementSize) || (max_xfer_len < dst.iElementSize))
		{
		// 18
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: max_xfer_len < iElementSize"));
		return 0;
		}
	if ((max_xfer_len < (src.iElementSize * src.iElementsPerFrame)) ||
		(max_xfer_len < (dst.iElementSize * dst.iElementsPerFrame)))
		{
		// 19
		__KTRACE_OPT(KPANIC,
					 Kern::Printf("Error: max_xfer_len < (iElementSize * iElementsPerFrame)"));
		return 0;
		}
#endif	// #ifdef _DEBUG

	return max_xfer_len;
	}


// Unified internal fragmentation routine, called by both the old and new
// exported Fragment() functions.
//
// Depending on whether the DMAC uses a single or two separate descriptor
// chains, this function branches into either FragSym() or FragAsym(), and the
// latter function further into either FragAsymSrc()/FragAsymDst() or
// FragBalancedAsym().
//
TInt DDmaRequest::Frag(TDmaTransferArgs& aTransferArgs)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Frag"));
	__DMA_ASSERTD(!iQueued);

	// Transfer count + checks
	const TUint count = GetTransferCount(aTransferArgs);
	if (count == 0)
		{
		return KErrArgument;
		}

	// Max transfer length + checks
	const TUint max_xfer_len = GetMaxTransferlength(aTransferArgs, count);
	if (max_xfer_len == 0)
		{
		return KErrArgument;
		}

	// ISR callback requested?
	const TBool isr_cb = (aTransferArgs.iFlags & KDmaRequestCallbackFromIsr);
	if (isr_cb)
		{
		// Requesting an ISR callback w/o supplying one?
		if (!iDmaCb)
			{
			// 12
			__KTRACE_OPT(KPANIC, Kern::Printf("Error: !iDmaCb"));
			return KErrArgument;
			}
		}

	// Set the channel cookie for the PSL
	aTransferArgs.iChannelCookie = iChannel.PslId();

	// Client shouldn't specify contradictory or invalid things
	TInt r = CheckMemFlags(aTransferArgs.iSrcConfig);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: CheckMemFlags(src)"));
		return r;
		}
	r =  CheckMemFlags(aTransferArgs.iDstConfig);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: CheckMemFlags(dst)"));
		return r;
		}

	// Now the actual fragmentation
	if (iChannel.iDmacCaps->iAsymHwDescriptors)
		{
		r = FragAsym(aTransferArgs, count, max_xfer_len);
		}
	else
		{
		r = FragSym(aTransferArgs, count, max_xfer_len);
		}

	if (r == KErrNone)
		{
		iIsrCb = isr_cb;
		}

	__DMA_INVARIANT();
	return r;
	};


TInt DDmaRequest::FragSym(TDmaTransferArgs& aTransferArgs, TUint aCount,
						  TUint aMaxTransferLen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragSym"));

	TDmaTransferConfig& src = aTransferArgs.iSrcConfig;
	TDmaTransferConfig& dst = aTransferArgs.iDstConfig;
	const TBool mem_src = (src.iFlags & KDmaMemAddr);
	const TBool mem_dst = (dst.iFlags & KDmaMemAddr);

	const TUint align_mask_src = iChannel.AddressAlignMask(src.iFlags,
														   src.iElementSize,
														   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask_src: 0x%x", align_mask_src));
	const TUint align_mask_dst = iChannel.AddressAlignMask(dst.iFlags,
														   dst.iElementSize,
														   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask_dst: 0x%x", align_mask_dst));

	// Memory buffers must satisfy alignment constraint
	__DMA_ASSERTD(!mem_src || ((src.iAddr & align_mask_src) == 0));
	__DMA_ASSERTD(!mem_dst || ((dst.iAddr & align_mask_dst) == 0));

	// Max aligned length is used to make sure the beginnings of subtransfers
	// (i.e. fragments) are correctly aligned.
	const TUint max_aligned_len = (aMaxTransferLen &
								   ~(_Max(align_mask_src, align_mask_dst)));
	__KTRACE_OPT(KDMA, Kern::Printf("max_aligned_len: %d", max_aligned_len));
	// Client and PSL sane?
	__DMA_ASSERTD(max_aligned_len > 0);

	if (mem_src && mem_dst &&
		align_mask_src && align_mask_dst &&
		(align_mask_src != align_mask_dst) &&
		(!(src.iFlags & KDmaMemIsContiguous) || !(dst.iFlags & KDmaMemIsContiguous)))
		{
		// We don't support transfers which satisfy ALL of the following conditions:
		// 1) from memory to memory,
		// 2) both sides have address alignment requirements,
		// 3) those alignment requirements are not the same,
		// 4) the memory is non-contiguous on at least one end.
		//
		// [A 5th condition is that the channel doesn't support fully
		// asymmetric h/w descriptor lists,
		// i.e. TDmaChannel::DmacCaps::iAsymHwDescriptors is reported as EFalse
		// or iBalancedAsymSegments as ETrue. Hence this check is done in
		// FragSym() and FragBalancedAsym() but not in FragAsym().]
		//
		// The reason for this is that fragmentation could be impossible. The
		// memory layout (page break) on the side with the less stringent
		// alignment requirement can result in a misaligned target address on
		// the other side.
		//
		// Here is an example:
		//
		// src.iAddr =  3964 (0x0F7C), non-contiguous,
		// align_mask_src = 1 (alignment = 2 bytes)
		// dst.iAddr = 16384 (0x4000), contiguous,
		// align_mask_dst = 7 (alignment = 8 bytes)
		// count = max_xfer_len = 135 bytes
		// => max_aligned_len = 128 bytes
		//
		// Now, suppose MaxPhysSize() returns 132 bytes because src has 132
		// contiguous bytes to the end of its current mem page.
		// Trying to fragment this leads to:
		//
		// frag_1 = 128 bytes: src reads from 3964 (0x0F7C),
		//                     dst writes to 16384 (0x4000).
		// (Fragment 1 uses the max_aligned_len instead of 132 bytes because
		// otherwise the next fragment would start for the destination at
		// dst.iAddr + 132 = 16516 (0x4084), which is not 8-byte aligned.)
		//
		// frag_2 = 4 bytes: src reads from 4092 (0x0FFC),
		//                   dst writes to 16512 (0x4080).
		// (Fragment 2 uses just 4 bytes instead of the remaining 7 bytes
		// because there is a memory page break on the source side after 4 bytes.)
		//
		// frag_3 = 3 bytes: src reads from 4096 (0x1000),
		//                   dst writes to 16516 (0x4084).
		//
		// And there's the problem: the start address of frag_3 is going to be
		// misaligned for the destination side - it's not 8-byte aligned!
		//
		// 17
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: Different alignments for src & dst"
										  " + non-contiguous target(s)"));
		return KErrArgument;
		}

	TInt r;
	// Revert any previous fragmentation attempt
	FreeDesList();
	do
		{
		// Allocate fragment
		r = ExpandDesList(/*1*/);
		if (r != KErrNone)
			{
			break;
			}
		// Compute fragment size
		TUint c = _Min(aMaxTransferLen, aCount);
		__KTRACE_OPT(KDMA, Kern::Printf("c = _Min(aMaxTransferLen, aCount) = %d", c));

		// SRC
		if (mem_src && !(src.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(src.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(src.iAddr, c) = %d", c));
			}

		// DST
		if (mem_dst && !(dst.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(dst.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(dst.iAddr, c) = %d", c));
			}

		// SRC & DST
		if ((mem_src || mem_dst) && (c < aCount) && (c > max_aligned_len))
			{
			// This is not the last fragment of a transfer to/from memory.
			// We must round down the fragment size so the next one is
			// correctly aligned.
			c = max_aligned_len;
			__KTRACE_OPT(KDMA, Kern::Printf("c = max_aligned_len = %d", c));
			//
			// But can this condition actually occur if src and dst are
			// properly aligned to start with?
			//
			// If we disallow unequal alignment requirements in connection with
			// non-contiguous memory buffers (see the long comment above in
			// this function for why) and if both target addresses are
			// correctly aligned at the beginning of the transfer then it
			// doesn't seem possible to end up with a fragment which is not
			// quite the total remaining size (c < aCount) but still larger
			// than the greatest aligned length (c > max_aligned_len).
			//
			// That's because address alignment values are always a power of
			// two (at least that's what we assume - otherwise
			// AddressAlignMask() doesn't work), and memory page sizes are also
			// always a power of two and hence a multiple of the alignment
			// value (as long as the alignment is not greater than the page
			// size, which seems a reasonable assumption regardless of the
			// actual page size). So if we start properly aligned anywhere in a
			// memory page then the number of bytes to the end of that page is
			// always a multiple of the aligment value - there's no remainder.
			//
			// So let's see if we ever hit this assertion:
			Kern::Printf("Unexpected: (mem_src || mem_dst) && (c < aCount) && (c > max_aligned_len)");
			__DMA_ASSERTA(EFalse);
			}

		// If this is not the last fragment...
		if (c < aCount)
			{
			const TUint es_src = src.iElementSize;
			const TUint es_dst = dst.iElementSize;
			const TUint fs_src = es_src * src.iElementsPerFrame;
			const TUint fs_dst = es_dst * dst.iElementsPerFrame;
			TUint c_prev;
			do
				{
				c_prev = c;
				// If fs_src is !0 then es_src must be !0 as well (see
				// CheckTransferConfig).
				if (es_src)
					{
					r = AdjustFragmentSize(c, es_src, fs_src);
					if (r != KErrNone)
						{
						break;							// while (c != c_prev);
						}
					}
				// If fs_dst is !0 then es_dst must be !0 as well (see
				// CheckTransferConfig).
				if (es_dst)
					{
					r = AdjustFragmentSize(c, es_dst, fs_dst);
					if (r != KErrNone)
						{
						break;							// while (c != c_prev);
						}
					}
				} while (c != c_prev);
			if (r != KErrNone)
				{
				break;									 // while (aCount > 0);
				}
			}

		// Set transfer count for the PSL
		aTransferArgs.iTransferCount = c;
		__KTRACE_OPT(KDMA, Kern::Printf("this fragm.: %d (0x%x) total remain.: %d (0x%x)",
										c, c, aCount, aCount));
		// Initialise fragment
		r = iChannel.iController->InitDes(*iLastHdr, aTransferArgs);
		if (r != KErrNone)
			{
			break;
			}
		// Update for next iteration
		aCount -= c;
		if (mem_src)
			{
			src.iAddr += c;
			}
		if (mem_dst)
			{
			dst.iAddr += c;
			}
		} while (aCount > 0);

	if (r != KErrNone)
		{
		FreeDesList();
		}
	return r;
	}


TInt DDmaRequest::FragAsym(TDmaTransferArgs& aTransferArgs, TUint aCount,
						   TUint aMaxTransferLen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragAsym"));

	TInt r;
	if (iChannel.iDmacCaps->iBalancedAsymSegments)
		{
		r = FragBalancedAsym(aTransferArgs, aCount, aMaxTransferLen);
		if (r != KErrNone)
			{
			FreeSrcDesList();
			FreeDstDesList();
			}
		return r;
		}
	r = FragAsymSrc(aTransferArgs, aCount, aMaxTransferLen);
	if (r != KErrNone)
		{
		FreeSrcDesList();
		return r;
		}
	r = FragAsymDst(aTransferArgs, aCount, aMaxTransferLen);
	if (r != KErrNone)
		{
		FreeSrcDesList();
		FreeDstDesList();
		}
	return r;
	}


TInt DDmaRequest::FragAsymSrc(TDmaTransferArgs& aTransferArgs, TUint aCount,
							  TUint aMaxTransferLen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragAsymSrc"));

	TDmaTransferConfig& src = aTransferArgs.iSrcConfig;
	const TBool mem_src = (src.iFlags & KDmaMemAddr);

	const TUint align_mask = iChannel.AddressAlignMask(src.iFlags,
													   src.iElementSize,
													   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask: 0x%x", align_mask));

	// Memory buffers must satisfy alignment constraint
	__DMA_ASSERTD(!mem_src || ((src.iAddr & align_mask) == 0));

	// Max aligned length is used to make sure the beginnings of subtransfers
	// (i.e. fragments) are correctly aligned.
	const TUint max_aligned_len = (aMaxTransferLen & ~align_mask);
	__KTRACE_OPT(KDMA, Kern::Printf("max_aligned_len: %d", max_aligned_len));
	// Client and PSL sane?
	__DMA_ASSERTD(max_aligned_len > 0);

	TInt r;
	// Revert any previous fragmentation attempt
	FreeSrcDesList();
	do
		{
		// Allocate fragment
		r = ExpandSrcDesList(/*1*/);
		if (r != KErrNone)
			{
			break;
			}
		// Compute fragment size
		TUint c = _Min(aMaxTransferLen, aCount);
		__KTRACE_OPT(KDMA, Kern::Printf("c = _Min(aMaxTransferLen, aCount) = %d", c));

		if (mem_src && !(src.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(src.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(src.iAddr, c) = %d", c));
			}

		if (mem_src && (c < aCount) && (c > max_aligned_len))
			{
			// This is not the last fragment of a transfer from memory.
			// We must round down the fragment size so the next one is
			// correctly aligned.
			__KTRACE_OPT(KDMA, Kern::Printf("c = max_aligned_len = %d", c));
			//
			// But can this condition actually occur if src is properly aligned
			// to start with?
			//
			// If the target address is correctly aligned at the beginning of
			// the transfer then it doesn't seem possible to end up with a
			// fragment which is not quite the total remaining size (c <
			// aCount) but still larger than the greatest aligned length (c >
			// max_aligned_len).
			//
			// That's because address alignment values are always a power of
			// two (at least that's what we assume - otherwise
			// AddressAlignMask() doesn't work), and memory page sizes are also
			// always a power of two and hence a multiple of the alignment
			// value (as long as the alignment is not greater than the page
			// size, which seems a reasonable assumption regardless of the
			// actual page size). So if we start properly aligned anywhere in a
			// memory page then the number of bytes to the end of that page is
			// always a multiple of the aligment value - there's no remainder.
			//
			// So let's see if we ever hit this assertion:
			Kern::Printf("Unexpected: mem_src && (c < aCount) && (c > max_aligned_len)");
			__DMA_ASSERTA(EFalse);
			}

		// If this is not the last fragment...
		if (c < aCount)
			{
			const TUint es = src.iElementSize;
			const TUint fs = es * src.iElementsPerFrame;
			// If fs is !0 then es must be !0 as well (see
			// CheckTransferConfig).
			if (es)
				{
				r = AdjustFragmentSize(c, es, fs);
				if (r != KErrNone)
					{
					break;								 // while (aCount > 0);
					}
				}
			}

		// Set transfer count for the PSL
		aTransferArgs.iTransferCount = c;
		__KTRACE_OPT(KDMA, Kern::Printf("this fragm.: %d (0x%x) total remain.: %d (0x%x)",
										c, c, aCount, aCount));
		// Initialise fragment
		r = iChannel.iController->InitSrcHwDes(*iSrcLastHdr, aTransferArgs);
		if (r != KErrNone)
			{
			break;
			}
		// Update for next iteration
		aCount -= c;
		if (mem_src)
			{
			src.iAddr += c;
			}
		} while (aCount > 0);

	return r;
	}


TInt DDmaRequest::FragAsymDst(TDmaTransferArgs& aTransferArgs, TUint aCount,
							  TUint aMaxTransferLen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragAsymDst"));

	TDmaTransferConfig& dst = aTransferArgs.iDstConfig;
	const TBool mem_dst = (dst.iFlags & KDmaMemAddr);

	const TUint align_mask = iChannel.AddressAlignMask(dst.iFlags,
													   dst.iElementSize,
													   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask: 0x%x", align_mask));

	// Memory buffers must satisfy alignment constraint
	__DMA_ASSERTD(!mem_dst || ((dst.iAddr & align_mask) == 0));

	// Max aligned length is used to make sure the beginnings of subtransfers
	// (i.e. fragments) are correctly aligned.
	const TUint max_aligned_len = (aMaxTransferLen & ~align_mask);
	__KTRACE_OPT(KDMA, Kern::Printf("max_aligned_len: %d", max_aligned_len));
	// Client and PSL sane?
	__DMA_ASSERTD(max_aligned_len > 0);

	TInt r;
	// Revert any previous fragmentation attempt
	FreeDstDesList();
	do
		{
		// Allocate fragment
		r = ExpandDstDesList(/*1*/);
		if (r != KErrNone)
			{
			break;
			}
		// Compute fragment size
		TUint c = _Min(aMaxTransferLen, aCount);
		__KTRACE_OPT(KDMA, Kern::Printf("c = _Min(aMaxTransferLen, aCount) = %d", c));

		if (mem_dst && !(dst.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(dst.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(dst.iAddr, c) = %d", c));
			}

		if (mem_dst && (c < aCount) && (c > max_aligned_len))
			{
			// This is not the last fragment of a transfer to memory.
			// We must round down the fragment size so the next one is
			// correctly aligned.
			__KTRACE_OPT(KDMA, Kern::Printf("c = max_aligned_len = %d", c));
			//
			// But can this condition actually occur if dst is properly aligned
			// to start with?
			//
			// If the target address is correctly aligned at the beginning of
			// the transfer then it doesn't seem possible to end up with a
			// fragment which is not quite the total remaining size (c <
			// aCount) but still larger than the greatest aligned length (c >
			// max_aligned_len).
			//
			// That's because address alignment values are always a power of
			// two (at least that's what we assume - otherwise
			// AddressAlignMask() doesn't work), and memory page sizes are also
			// always a power of two and hence a multiple of the alignment
			// value (as long as the alignment is not greater than the page
			// size, which seems a reasonable assumption regardless of the
			// actual page size). So if we start properly aligned anywhere in a
			// memory page then the number of bytes to the end of that page is
			// always a multiple of the aligment value - there's no remainder.
			//
			// So let's see if we ever hit this assertion:
			Kern::Printf("Unexpected: mem_dst && (c < aCount) && (c > max_aligned_len)");
			__DMA_ASSERTA(EFalse);
			}

		// If this is not the last fragment...
		if (c < aCount)
			{
			const TUint es = dst.iElementSize;
			const TUint fs = es * dst.iElementsPerFrame;
			// If fs is !0 then es must be !0 as well (see
			// CheckTransferConfig).
			if (es)
				{
				r = AdjustFragmentSize(c, es, fs);
				if (r != KErrNone)
					{
					break;								 // while (aCount > 0);
					}
				}
			}

		// Set transfer count for the PSL
		aTransferArgs.iTransferCount = c;
		__KTRACE_OPT(KDMA, Kern::Printf("this fragm.: %d (0x%x) total remain.: %d (0x%x)",
										c, c, aCount, aCount));
		// Initialise fragment
		r = iChannel.iController->InitDstHwDes(*iDstLastHdr, aTransferArgs);
		if (r != KErrNone)
			{
			break;
			}
		// Update for next iteration
		aCount -= c;
		if (mem_dst)
			{
			dst.iAddr += c;
			}
		}
	while (aCount > 0);

	return r;
	}


TInt DDmaRequest::FragBalancedAsym(TDmaTransferArgs& aTransferArgs, TUint aCount,
								   TUint aMaxTransferLen)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragBalancedAsym"));

	TDmaTransferConfig& src = aTransferArgs.iSrcConfig;
	TDmaTransferConfig& dst = aTransferArgs.iDstConfig;
	const TBool mem_src = (src.iFlags & KDmaMemAddr);
	const TBool mem_dst = (dst.iFlags & KDmaMemAddr);

	const TUint align_mask_src = iChannel.AddressAlignMask(src.iFlags,
														   src.iElementSize,
														   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask_src: 0x%x", align_mask_src));
	const TUint align_mask_dst = iChannel.AddressAlignMask(dst.iFlags,
														   dst.iElementSize,
														   aTransferArgs.iPslRequestInfo);
	__KTRACE_OPT(KDMA, Kern::Printf("align_mask_dst: 0x%x", align_mask_dst));

	// Memory buffers must satisfy alignment constraint
	__DMA_ASSERTD(!mem_src || ((src.iAddr & align_mask_src) == 0));
	__DMA_ASSERTD(!mem_dst || ((dst.iAddr & align_mask_dst) == 0));

	// Max aligned length is used to make sure the beginnings of subtransfers
	// (i.e. fragments) are correctly aligned.
	const TUint max_aligned_len = (aMaxTransferLen &
								   ~(_Max(align_mask_src, align_mask_dst)));
	__KTRACE_OPT(KDMA, Kern::Printf("max_aligned_len: %d", max_aligned_len));
	// Client and PSL sane?
	__DMA_ASSERTD(max_aligned_len > 0);

	if (mem_src && mem_dst &&
		align_mask_src && align_mask_dst &&
		(align_mask_src != align_mask_dst) &&
		(!(src.iFlags & KDmaMemIsContiguous) || !(dst.iFlags & KDmaMemIsContiguous)))
		{
		// We don't support transfers which satisfy ALL of the following conditions:
		// 1) from memory to memory,
		// 2) both sides have address alignment requirements,
		// 3) those alignment requirements are not the same,
		// 4) the memory is non-contiguous on at least one end.
		//
		// [A 5th condition is that the channel doesn't support fully
		// asymmetric h/w descriptor lists,
		// i.e. TDmaChannel::DmacCaps::iAsymHwDescriptors is reported as EFalse
		// or iBalancedAsymSegments as ETrue. Hence this check is done in
		// FragSym() and FragBalancedAsym() but not in FragAsym().]
		//
		// The reason for this is that fragmentation could be impossible. The
		// memory layout (page break) on the side with the less stringent
		// alignment requirement can result in a misaligned target address on
		// the other side.
		//
		// Here is an example:
		//
		// src.iAddr =  3964 (0x0F7C), non-contiguous,
		// align_mask_src = 1 (alignment = 2 bytes)
		// dst.iAddr = 16384 (0x4000), contiguous,
		// align_mask_dst = 7 (alignment = 8 bytes)
		// count = max_xfer_len = 135 bytes
		// => max_aligned_len = 128 bytes
		//
		// Now, suppose MaxPhysSize() returns 132 bytes because src has 132
		// contiguous bytes to the end of its current mem page.
		// Trying to fragment this leads to:
		//
		// frag_1 = 128 bytes: src reads from 3964 (0x0F7C),
		//                     dst writes to 16384 (0x4000).
		// (Fragment 1 uses the max_aligned_len instead of 132 bytes because
		// otherwise the next fragment would start for the destination at
		// dst.iAddr + 132 = 16516 (0x4084), which is not 8-byte aligned.)
		//
		// frag_2 = 4 bytes: src reads from 4092 (0x0FFC),
		//                   dst writes to 16512 (0x4080).
		// (Fragment 2 uses just 4 bytes instead of the remaining 7 bytes
		// because there is a memory page break on the source side after 4 bytes.)
		//
		// frag_3 = 3 bytes: src reads from 4096 (0x1000),
		//                   dst writes to 16516 (0x4084).
		//
		// And there's the problem: the start address of frag_3 is going to be
		// misaligned for the destination side - it's not 8-byte aligned!
		//
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: Different alignments for src & dst"
										  " + non-contiguous target(s)"));
		return KErrArgument;
		}

	TInt r;
	// Revert any previous fragmentation attempt
	FreeSrcDesList();
	FreeDstDesList();
	__DMA_ASSERTD(iSrcDesCount == iDstDesCount);
	do
		{
		// Allocate fragment
		r = ExpandSrcDesList(/*1*/);
		if (r != KErrNone)
			{
			break;
			}
		r = ExpandDstDesList(/*1*/);
		if (r != KErrNone)
			{
			break;
			}
		__DMA_ASSERTD(iSrcDesCount == iDstDesCount);
		// Compute fragment size
		TUint c = _Min(aMaxTransferLen, aCount);
		__KTRACE_OPT(KDMA, Kern::Printf("c = _Min(aMaxTransferLen, aCount) = %d", c));

		// SRC
		if (mem_src && !(src.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(src.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(src.iAddr, c) = %d", c));
			}

		// DST
		if (mem_dst && !(dst.iFlags & KDmaMemIsContiguous))
			{
			c = MaxPhysSize(dst.iAddr, c);
			__KTRACE_OPT(KDMA, Kern::Printf("c = MaxPhysSize(dst.iAddr, c) = %d", c));
			}

		// SRC & DST
		if ((mem_src || mem_dst) && (c < aCount) && (c > max_aligned_len))
			{
			// This is not the last fragment of a transfer to/from memory.
			// We must round down the fragment size so the next one is
			// correctly aligned.
			c = max_aligned_len;
			__KTRACE_OPT(KDMA, Kern::Printf("c = max_aligned_len = %d", c));
			//
			// But can this condition actually occur if src and dst are
			// properly aligned to start with?
			//
			// If we disallow unequal alignment requirements in connection with
			// non-contiguous memory buffers (see the long comment above in
			// this function for why) and if both target addresses are
			// correctly aligned at the beginning of the transfer then it
			// doesn't seem possible to end up with a fragment which is not
			// quite the total remaining size (c < aCount) but still larger
			// than the greatest aligned length (c > max_aligned_len).
			//
			// That's because address alignment values are always a power of
			// two (at least that's what we assume - otherwise
			// AddressAlignMask() doesn't work), and memory page sizes are also
			// always a power of two and hence a multiple of the alignment
			// value (as long as the alignment is not greater than the page
			// size, which seems a reasonable assumption regardless of the
			// actual page size). So if we start properly aligned anywhere in a
			// memory page then the number of bytes to the end of that page is
			// always a multiple of the aligment value - there's no remainder.
			//
			// So let's see if we ever hit this assertion:
			Kern::Printf("Unexpected: (mem_src || mem_dst) && (c < aCount) && (c > max_aligned_len)");
			__DMA_ASSERTA(EFalse);
			}

		// If this is not the last fragment...
		if (c < aCount)
			{
			const TUint es_src = src.iElementSize;
			const TUint es_dst = dst.iElementSize;
			const TUint fs_src = es_src * src.iElementsPerFrame;
			const TUint fs_dst = es_dst * dst.iElementsPerFrame;
			TUint c_prev;
			do
				{
				c_prev = c;
				// If fs_src is !0 then es_src must be !0 as well (see
				// CheckTransferConfig).
				if (es_src)
					{
					r = AdjustFragmentSize(c, es_src, fs_src);
					if (r != KErrNone)
						{
						break;							// while (c != c_prev);
						}
					}
				// If fs_dst is !0 then es_dst must be !0 as well (see
				// CheckTransferConfig).
				if (es_dst)
					{
					r = AdjustFragmentSize(c, es_dst, fs_dst);
					if (r != KErrNone)
						{
						break;							// while (c != c_prev);
						}
					}
				} while (c != c_prev);
			if (r != KErrNone)
				{
				break;									 // while (aCount > 0);
				}
			}

		// Set transfer count for the PSL
		aTransferArgs.iTransferCount = c;
		__KTRACE_OPT(KDMA, Kern::Printf("this fragm.: %d (0x%x) total remain.: %d (0x%x)",
										c, c, aCount, aCount));
		// Initialise SRC fragment
		r = iChannel.iController->InitSrcHwDes(*iSrcLastHdr, aTransferArgs);
		if (r != KErrNone)
			{
			break;
			}
		// Initialise DST fragment
		r = iChannel.iController->InitDstHwDes(*iDstLastHdr, aTransferArgs);
		if (r != KErrNone)
			{
			break;
			}
		// Update for next iteration
		aCount -= c;
		if (mem_src)
			{
			src.iAddr += c;
			}
		if (mem_dst)
			{
			dst.iAddr += c;
			}
		}
	while (aCount > 0);

	return r;
	}


EXPORT_C TInt DDmaRequest::Queue()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Queue thread %O", &Kern::CurrentThread()));
	// Not configured? Call Fragment() first!
	if (iChannel.iDmacCaps->iAsymHwDescriptors)
		{
		__DMA_ASSERTD((iSrcDesCount > 0) && (iDstDesCount > 0));
		}
	else
		{
		__DMA_ASSERTD(iDesCount > 0);
		}
	__DMA_ASSERTD(!iQueued);

	// Append request to queue and link new descriptor list to existing one.
	iChannel.Wait();

	TUint32 req_count = iChannel.iQueuedRequests++;
	if (iChannel.iCallQueuedRequestFn)
		{
		if (req_count == 0)
			{
			iChannel.Signal();
			iChannel.QueuedRequestCountChanged();
			iChannel.Wait();
			}
		}

	TInt r = KErrGeneral;
	const TBool ch_isr_cb = __e32_atomic_load_acq32(&iChannel.iIsrCbRequest);
	if (ch_isr_cb)
		{
		// Client mustn't try to queue any new request while one with an ISR
		// callback is already queued on this channel. This is to make sure
		// that the channel's Transfer() function is not called by both the ISR
		// and the client thread at the same time.
		__KTRACE_OPT(KPANIC, Kern::Printf("An ISR cb request exists - not queueing"));
		// Undo the request count increment...
		req_count = --iChannel.iQueuedRequests;
		__DMA_INVARIANT();
		iChannel.Signal();
		if (iChannel.iCallQueuedRequestFn)
			{
			if (req_count == 0)
				{
				iChannel.QueuedRequestCountChanged();
				}
			}
		}
	else if (iIsrCb && !iChannel.IsQueueEmpty())
		{
		// Client mustn't try to queue an ISR callback request whilst any
		// others are still queued on this channel. This is to make sure that
		// the ISR callback doesn't get executed together with the DFC(s) of
		// any previous request(s).
		__KTRACE_OPT(KPANIC, Kern::Printf("Request queue not empty - not queueing"));
		// Undo the request count increment...
		req_count = --iChannel.iQueuedRequests;
		__DMA_INVARIANT();
		iChannel.Signal();
		if (iChannel.iCallQueuedRequestFn)
			{
			if (req_count == 0)
				{
				iChannel.QueuedRequestCountChanged();
				}
			}
		}
	else if (iChannel.iIsrDfc & (TUint32)TDmaChannel::KCancelFlagMask)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("Channel requests cancelled - not queueing"));
		// Someone is cancelling all requests - undo the request count increment...
		req_count = --iChannel.iQueuedRequests;
		__DMA_INVARIANT();
		iChannel.Signal();
		if (iChannel.iCallQueuedRequestFn)
			{
			if (req_count == 0)
				{
				iChannel.QueuedRequestCountChanged();
				}
			}
		}
	else
		{
		iQueued = ETrue;
		iChannel.iReqQ.Add(&iLink);
		iChannel.SetNullPtr(*this);
		if (iIsrCb)
			{
			// Since we've made sure that there is no other request in the
			// queue before this, the only thing of relevance is the channel
			// DFC which might yet have to complete for the previous request,
			// and this function might indeed have been called from there via
			// the client callback. This should be all right though as once
			// we've set the following flag no further Queue()'s will be
			// possible.
			__e32_atomic_store_rel32(&iChannel.iIsrCbRequest, ETrue);
			}
		iChannel.DoQueue(*this);
		r = KErrNone;
		__DMA_INVARIANT();
		iChannel.Signal();
		}

	return r;
	}


EXPORT_C TInt DDmaRequest::ExpandDesList(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::ExpandDesList aCount=%d", aCount));
	return ExpandDesList(aCount, iDesCount, iFirstHdr, iLastHdr);
	}


EXPORT_C TInt DDmaRequest::ExpandSrcDesList(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::ExpandSrcDesList"));
	return ExpandDesList(aCount, iSrcDesCount, iSrcFirstHdr, iSrcLastHdr);
	}


EXPORT_C TInt DDmaRequest::ExpandDstDesList(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::ExpandDstDesList"));
	return ExpandDesList(aCount, iDstDesCount, iDstFirstHdr, iDstLastHdr);
	}


TInt DDmaRequest::ExpandDesList(TInt aCount, TInt& aDesCount,
								SDmaDesHdr*& aFirstHdr,
								SDmaDesHdr*& aLastHdr)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::ExpandDesList"));
	__DMA_ASSERTD(!iQueued);
	__DMA_ASSERTD(aCount > 0);

	if (aCount > iChannel.iAvailDesCount)
		{
		return KErrTooBig;
		}

	iChannel.iAvailDesCount -= aCount;
	aDesCount += aCount;

	TDmac& c = *(iChannel.iController);
	c.Wait();

	if (aFirstHdr == NULL)
		{
		// Handle an empty list specially to simplify the following loop
		aFirstHdr = aLastHdr = c.iFreeHdr;
		c.iFreeHdr = c.iFreeHdr->iNext;
		--aCount;
		}
	else
		{
		aLastHdr->iNext = c.iFreeHdr;
		}

	// Remove as many descriptors and headers from the free pool as necessary
	// and ensure hardware descriptors are chained together.
	while (aCount-- > 0)
		{
		__DMA_ASSERTD(c.iFreeHdr != NULL);
		if (c.iCapsHwDes)
			{
			c.ChainHwDes(*aLastHdr, *(c.iFreeHdr));
			}
		aLastHdr = c.iFreeHdr;
		c.iFreeHdr = c.iFreeHdr->iNext;
		}

	c.Signal();

	aLastHdr->iNext = NULL;

	__DMA_INVARIANT();
	return KErrNone;
	}


EXPORT_C void DDmaRequest::FreeDesList()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FreeDesList"));
	FreeDesList(iDesCount, iFirstHdr, iLastHdr);
	}


EXPORT_C void DDmaRequest::FreeSrcDesList()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FreeSrcDesList"));
	FreeDesList(iSrcDesCount, iSrcFirstHdr, iSrcLastHdr);
	}


EXPORT_C void DDmaRequest::FreeDstDesList()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FreeDstDesList"));
	FreeDesList(iDstDesCount, iDstFirstHdr, iDstLastHdr);
	}


void DDmaRequest::FreeDesList(TInt& aDesCount, SDmaDesHdr*& aFirstHdr, SDmaDesHdr*& aLastHdr)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FreeDesList count=%d", aDesCount));
	__DMA_ASSERTD(!iQueued);

	if (aDesCount > 0)
		{
		iChannel.iAvailDesCount += aDesCount;
		TDmac& c = *(iChannel.iController);
		const SDmaDesHdr* hdr = aFirstHdr;
		while (hdr)
			{
			__DMA_ASSERTD(c.IsValidHdr(hdr));

			// This (potential) PSL call doesn't follow the "overhead
			// principle", and something should be done about this.
			c.ClearHwDes(*hdr);
			hdr = hdr->iNext;
			};

		c.Wait();
		__DMA_ASSERTD(c.IsValidHdr(c.iFreeHdr));
		aLastHdr->iNext = c.iFreeHdr;
		c.iFreeHdr = aFirstHdr;
		c.Signal();

		aFirstHdr = aLastHdr = NULL;
		aDesCount = 0;
		}
	}


EXPORT_C void DDmaRequest::EnableSrcElementCounting(TBool /*aResetElementCount*/)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::EnableSrcElementCounting"));

	// Not yet implemented.
	return;
	}


EXPORT_C void DDmaRequest::EnableDstElementCounting(TBool /*aResetElementCount*/)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::EnableDstElementCounting"));

	// Not yet implemented.
	return;
	}


EXPORT_C void DDmaRequest::DisableSrcElementCounting()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::DisableSrcElementCounting"));

	// Not yet implemented.
	return;
	}


EXPORT_C void DDmaRequest::DisableDstElementCounting()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::DisableDstElementCounting"));

	// Not yet implemented.
	return;
	}


EXPORT_C TUint32 DDmaRequest::TotalNumSrcElementsTransferred()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::TotalNumSrcElementsTransferred"));

	// Not yet implemented.
	return iTotalNumSrcElementsTransferred;
	}


EXPORT_C TUint32 DDmaRequest::TotalNumDstElementsTransferred()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::TotalNumDstElementsTransferred"));

	// Not yet implemented.
	return iTotalNumDstElementsTransferred;
	}


EXPORT_C TInt DDmaRequest::FragmentCount()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragmentCount"));
	return FragmentCount(iFirstHdr);
	}


EXPORT_C TInt DDmaRequest::SrcFragmentCount()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::SrcFragmentCount"));
	return FragmentCount(iSrcFirstHdr);
	}


EXPORT_C TInt DDmaRequest::DstFragmentCount()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::DstFragmentCount"));
	return FragmentCount(iDstFirstHdr);
	}


TInt DDmaRequest::FragmentCount(const SDmaDesHdr* aHdr)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::FragmentCount aHdr=0x%08x", aHdr));
	TInt count = 0;
	for (const SDmaDesHdr* pH = aHdr; pH != NULL; pH = pH->iNext)
		{
		count++;
		}
	return count;
	}


//
// Called when request is removed from request queue in channel
//
inline void DDmaRequest::OnDeque()
	{
	iQueued = EFalse;
	if (iChannel.iDmacCaps->iAsymHwDescriptors)
		{
		iSrcLastHdr->iNext = NULL;
		iDstLastHdr->iNext = NULL;
		iChannel.DoUnlink(*iSrcLastHdr);
		iChannel.DoUnlink(*iDstLastHdr);
		}
	else
		{
		iLastHdr->iNext = NULL;
		iChannel.DoUnlink(*iLastHdr);
		}
	}


#ifdef _DEBUG
void DDmaRequest::Invariant()
	{
	// This invariant may be called either with,
	// or without the channel lock already held
	TBool channelLockAquired=EFalse;
	if(!iChannel.iLock.HeldByCurrentThread())
		{
		iChannel.Wait();
		channelLockAquired = ETrue;
		}

	__DMA_ASSERTD(LOGICAL_XOR(iCb, iDmaCb));
	if (iChannel.iDmacCaps->iAsymHwDescriptors)
		{
		__DMA_ASSERTD((0 <= iSrcDesCount) && (iSrcDesCount <= iChannel.iMaxDesCount) &&
					  (0 <= iDstDesCount) && (iDstDesCount <= iChannel.iMaxDesCount));
		if (iSrcDesCount == 0)
			{
			// Not fragmented yet
			__DMA_ASSERTD(iDstDesCount == 0);
			__DMA_ASSERTD(!iQueued);
			__DMA_ASSERTD(!iSrcFirstHdr && !iSrcLastHdr &&
						  !iDstFirstHdr && !iDstLastHdr);
			}
		else if (iDstDesCount == 0)
			{
			// Src side only fragmented yet
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iSrcFirstHdr));
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iSrcLastHdr));
			}
		else
			{
			// Src & Dst sides fragmented
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iSrcFirstHdr));
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iSrcLastHdr));
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iDstFirstHdr));
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iDstLastHdr));
			}
		}
	else
		{
		__DMA_ASSERTD((0 <= iDesCount) && (iDesCount <= iChannel.iMaxDesCount));
		if (iDesCount == 0)
			{
			__DMA_ASSERTD(!iQueued);
			__DMA_ASSERTD(!iFirstHdr && !iLastHdr);
			}
		else
			{
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iFirstHdr));
			__DMA_ASSERTD(iChannel.iController->IsValidHdr(iLastHdr));
			}
		}

	if(channelLockAquired)
			{
			iChannel.Signal();
			}
	}
#endif


//////////////////////////////////////////////////////////////////////////////
// TDmaChannel

TDmaChannel::TDmaChannel()
	: iController(NULL),
	  iDmacCaps(NULL),
	  iPslId(0),
	  iDynChannel(EFalse),
	  iPriority(KDmaPriorityNone),
	  iCurHdr(NULL),
	  iNullPtr(&iCurHdr),
	  iDfc(Dfc, NULL, 0),
	  iMaxDesCount(0),
	  iAvailDesCount(0),
	  iIsrDfc(0),
	  iReqQ(),
	  iReqCount(0),
	  iQueuedRequests(0),
	  iCallQueuedRequestFn(ETrue),
	  iCancelInfo(NULL),
	  iRedoRequest(EFalse),
	  iIsrCbRequest(EFalse)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::TDmaChannel =0x%08X", this));
	__DMA_INVARIANT();
	}


//
// static member function
//
EXPORT_C TInt TDmaChannel::Open(const SCreateInfo& aInfo, TDmaChannel*& aChannel)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Open thread %O", &Kern::CurrentThread()));

	if (aInfo.iDesCount < 1)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("DMA channel failed to open: iDescount<1"));
		return KErrArgument;
		}

	__DMA_ASSERTD(aInfo.iPriority <= KDmaPriority8);
	__DMA_ASSERTD(aInfo.iDfcQ != NULL);
	__DMA_ASSERTD(aInfo.iDfcPriority < KNumDfcPriorities);

	aChannel = NULL;

	DmaChannelMgr::Wait();
	TDmaChannel* pC = DmaChannelMgr::Open(aInfo.iCookie, aInfo.iDynChannel, aInfo.iPriority);
	DmaChannelMgr::Signal();
	if (!pC)
		{
		return KErrInUse;
		}
	__DMA_ASSERTD(pC->iController != NULL);
	__DMA_ASSERTD(pC->iDmacCaps != NULL);
	__DMA_ASSERTD(pC->iController->iCapsHwDes == pC->DmacCaps().iHwDescriptors);
	// PSL needs to set iDynChannel if and only if dynamic channel was requested
	__DMA_ASSERTD(!LOGICAL_XOR(aInfo.iDynChannel, pC->iDynChannel));

	const TInt r = pC->iController->ReserveSetOfDes(aInfo.iDesCount);
	if (r != KErrNone)
		{
		pC->Close();
		return r;
		}
	pC->iAvailDesCount = pC->iMaxDesCount = aInfo.iDesCount;

	new (&pC->iDfc) TDfc(&Dfc, pC, aInfo.iDfcQ, aInfo.iDfcPriority);

	aChannel = pC;

#ifdef _DEBUG
	pC->Invariant();
#endif
	__KTRACE_OPT(KDMA, Kern::Printf("opened channel %d", pC->iPslId));
	return KErrNone;
	}


EXPORT_C void TDmaChannel::Close()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Close %d iReqCount=%d", iPslId, iReqCount));
	__DMA_ASSERTD(IsQueueEmpty());
	__DMA_ASSERTD(iReqCount == 0);

	__DMA_ASSERTD(iQueuedRequests == 0);

	// Descriptor leak? -> bug in request code
	__DMA_ASSERTD(iAvailDesCount == iMaxDesCount);

	__DMA_ASSERTD(!iRedoRequest);
	__DMA_ASSERTD(!iIsrCbRequest);

	iController->ReleaseSetOfDes(iMaxDesCount);
	iAvailDesCount = iMaxDesCount = 0;

	DmaChannelMgr::Wait();
	DmaChannelMgr::Close(this);
	// The following assignment will be removed once IsOpened() has been
	// removed. That's because 'this' shouldn't be touched any more once
	// Close() has returned from the PSL.
	iController = NULL;
	DmaChannelMgr::Signal();
	}


EXPORT_C TInt TDmaChannel::LinkToChannel(TDmaChannel* aChannel)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::LinkToChannel thread %O",
									&Kern::CurrentThread()));
	if (aChannel)
		{
		return iController->LinkChannels(*this, *aChannel);
		}
	else
		{
		return iController->UnlinkChannel(*this);
		}
	}


EXPORT_C TInt TDmaChannel::Pause()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Pause thread %O",
									&Kern::CurrentThread()));
	return iController->PauseTransfer(*this);
	}


EXPORT_C TInt TDmaChannel::Resume()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Resume thread %O",
									&Kern::CurrentThread()));
	return iController->ResumeTransfer(*this);
	}


EXPORT_C void TDmaChannel::CancelAll()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::CancelAll thread %O channel - %d",
									&Kern::CurrentThread(), iPslId));
	NThread* const nt = NKern::CurrentThread();
	TBool wait = EFalse;
	TDmaCancelInfo cancelinfo;
	TDmaCancelInfo* waiters = NULL;

	NKern::ThreadEnterCS();
	Wait();
	const TUint32 req_count_before = iQueuedRequests;
	NThreadBase* const dfc_nt = iDfc.Thread();
	// Shouldn't be NULL (i.e. an IDFC)
	__DMA_ASSERTD(dfc_nt);

	__e32_atomic_store_ord32(&iIsrDfc, (TUint32)KCancelFlagMask);
	// ISRs after this point will not post a DFC, however a DFC may already be
	// queued or running or both.
	if (!IsQueueEmpty())
		{
		// There is a transfer in progress. It may complete before the DMAC
		// has stopped, but the resulting ISR will not post a DFC.
		// ISR should not happen after this function returns.
		iController->StopTransfer(*this);

		DoCancelAll();
		ResetNullPtr();

		// Clean-up the request queue.
		SDblQueLink* pL;
		while ((pL = iReqQ.GetFirst()) != NULL)
			{
			iQueuedRequests--;
			DDmaRequest* pR = _LOFF(pL, DDmaRequest, iLink);
			pR->OnDeque();
			}
		}
	if (dfc_nt == nt)
		{
		// DFC runs in this thread, so just cancel it and we're finished
		iDfc.Cancel();

		// If other calls to CancelAll() are waiting for the DFC, release them here
		waiters = iCancelInfo;
		iCancelInfo = NULL;

		// Reset the ISR count
		__e32_atomic_store_rel32(&iIsrDfc, 0);
		}
	else
		{
		// DFC runs in another thread. Make sure it's queued and then wait for it to run.
		if (iCancelInfo)
			{
			// Insert cancelinfo into the list so that it precedes iCancelInfo
			cancelinfo.InsertBefore(iCancelInfo);
			}
		else
			{
			iCancelInfo = &cancelinfo;
			}
		wait = ETrue;
		iDfc.Enque();
		}

	const TUint32 req_count_after = iQueuedRequests;

	Signal();

	if (waiters)
		{
		waiters->Signal();
		}
	else if (wait)
		{
		NKern::FSWait(&cancelinfo.iSem);
		}

 	NKern::ThreadLeaveCS();

	// Only call PSL if there were requests queued when we entered AND there
	// are now no requests left on the queue.
	if (iCallQueuedRequestFn)
		{
		if ((req_count_before != 0) && (req_count_after == 0))
			{
			QueuedRequestCountChanged();
			}
		}

	__DMA_INVARIANT();
	}


EXPORT_C TInt TDmaChannel::IsrRedoRequest(TUint32 aSrcAddr, TUint32 aDstAddr,
										  TUint aTransferCount,
										  TUint32 aPslRequestInfo,
										  TBool aIsrCb)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::IsrRedoRequest src=0x%08X, "
									"dst=0x%08X, count=%d, pslInfo=0x%08X, isrCb=%d",
									aSrcAddr, aDstAddr, aTransferCount, aPslRequestInfo,
									aIsrCb));
	// Function needs to be called in ISR context.
	__DMA_ASSERTD(NKern::CurrentContext() == NKern::EInterrupt);

	__DMA_ASSERTD(!iReqQ.IsEmpty());
	__DMA_ASSERTD(iIsrCbRequest);

#ifdef _DEBUG
	if ((aSrcAddr != KPhysAddrInvalid) && (aSrcAddr == aDstAddr))
		{
		__KTRACE_OPT(KPANIC,
					 Kern::Printf("Error: Updating src & dst to same address: 0x%08X",
								  aSrcAddr));
		return KErrArgument;
		}
#endif

	// We assume here that the just completed request is the first one in the
	// queue, i.e. that even if there is more than one request in the queue,
	// their respective last and first (hw) descriptors are *not* linked.
	// (Although that's what apparently happens in TDmaSgChannel::DoQueue() /
	// TDmac::AppendHwDes() @@@).
	DDmaRequest* const pCurReq = _LOFF(iReqQ.First(), DDmaRequest, iLink);
	TInt r;

	if (iDmacCaps->iAsymHwDescriptors)
		{
		// We don't allow multiple-descriptor chains to be updated here.
		// That we just panic (instead of returning an error), and also only in
		// the UDEB case (instead of always) is not ideal, but done here in the
		// interest of performance.
		__DMA_ASSERTD((pCurReq->iSrcDesCount == 1) && (pCurReq->iDstDesCount == 1));

		// Adjust parameters if necessary (asymmetrical s/g variety)
		const SDmaDesHdr* const pSrcFirstHdr = pCurReq->iSrcFirstHdr;
		if ((aSrcAddr != KPhysAddrInvalid) || aTransferCount || aPslRequestInfo)
			{
			r = iController->UpdateSrcHwDes(*pSrcFirstHdr, aSrcAddr,
											aTransferCount, aPslRequestInfo);
			if (r != KErrNone)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("Src descriptor updating failed in PSL"));
				return r;
				}
			}
		const SDmaDesHdr* const pDstFirstHdr = pCurReq->iDstFirstHdr;
		if ((aDstAddr != KPhysAddrInvalid) || aTransferCount || aPslRequestInfo)
			{
			r = iController->UpdateDstHwDes(*pDstFirstHdr, aSrcAddr,
											aTransferCount, aPslRequestInfo);
			if (r != KErrNone)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("Dst descriptor updating failed in PSL"));
				return r;
				}
			}
		// Reschedule the request
		iController->Transfer(*this, *pSrcFirstHdr, *pDstFirstHdr);
		}
	else
		{
		// We don't allow a multiple-descriptor chain to be updated here.
		// That we just panic (instead of returning an error), and also only in
		// the UDEB case (instead of always) is not ideal, but done here in the
		// interest of performance.
		__DMA_ASSERTD(pCurReq->iDesCount == 1);

		// Adjust parameters if necessary (symmetrical s/g and non-s/g variety)
		const SDmaDesHdr* const pFirstHdr = pCurReq->iFirstHdr;
		if ((aSrcAddr != KPhysAddrInvalid) || (aDstAddr != KPhysAddrInvalid) ||
			aTransferCount || aPslRequestInfo)
			{
			r = iController->UpdateDes(*pFirstHdr, aSrcAddr, aDstAddr,
									   aTransferCount, aPslRequestInfo);
			if (r != KErrNone)
				{
				__KTRACE_OPT(KPANIC, Kern::Printf("Descriptor updating failed"));
				return r;
				}
			}
		// Reschedule the request
		iController->Transfer(*this, *pFirstHdr);
		}

	if (!aIsrCb)
		{
		// Not another ISR callback please
		pCurReq->iIsrCb = aIsrCb;
		}
	iRedoRequest = ETrue;

	return KErrNone;
	}


EXPORT_C TInt TDmaChannel::FailNext(TInt /*aFragmentCount*/)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::FailNext"));
	return iController->FailNext(*this);
	}


EXPORT_C TInt TDmaChannel::MissNextInterrupts(TInt aInterruptCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::MissNextInterrupts"));
	return iController->MissNextInterrupts(*this, aInterruptCount);
	}


EXPORT_C TInt TDmaChannel::Extension(TInt aCmd, TAny* aArg)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Extension"));
	return iController->Extension(*this, aCmd, aArg);
	}


//
// static member function
//
EXPORT_C TInt TDmaChannel::StaticExtension(TInt aCmd, TAny* aArg)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::StaticExtension"));
	return DmaChannelMgr::StaticExtension(aCmd, aArg);
	}


EXPORT_C TUint TDmaChannel::MaxTransferLength(TUint aSrcFlags, TUint aDstFlags,
											  TUint32 aPslInfo)
	{
	return iController->MaxTransferLength(*this, aSrcFlags, aDstFlags, aPslInfo);
	}


EXPORT_C TUint TDmaChannel::AddressAlignMask(TUint aTargetFlags, TUint aElementSize,
											 TUint32 aPslInfo)
	{
	return iController->AddressAlignMask(*this, aTargetFlags, aElementSize, aPslInfo);
	}


EXPORT_C const SDmacCaps& TDmaChannel::DmacCaps()
	{
	return *iDmacCaps;
	}


//
// DFC callback function (static member).
//
void TDmaChannel::Dfc(TAny* aArg)
	{
	static_cast<TDmaChannel*>(aArg)->DoDfc();
	}


//
// This is quite a long function, but what can you do...
//
void TDmaChannel::DoDfc()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::DoDfc thread %O channel - %d",
									&Kern::CurrentThread(), iPslId));
	Wait();

	// Atomically fetch and reset the number of DFCs queued by the ISR and the
	// error flag. Leave the cancel flag alone for now.
	const TUint32 w = __e32_atomic_and_ord32(&iIsrDfc, (TUint32)KCancelFlagMask);
	TUint32 count = w & KDfcCountMask;
	const TBool error = w & (TUint32)KErrorFlagMask;
	TBool stop = w & (TUint32)KCancelFlagMask;
	const TUint32 req_count_before = iQueuedRequests;
	TUint32 req_count_after = 0;

	__DMA_ASSERTD((count > 0) || stop);
	__DMA_ASSERTD(!iRedoRequest); // We shouldn't be here if this is true

	while (count && !stop)
		{
		--count;

		__DMA_ASSERTA(!iReqQ.IsEmpty());

		// If an error occurred it must have been reported on the last
		// interrupt since transfers are suspended after an error.
		DDmaRequest::TResult const res = (count == 0 && error) ?
			DDmaRequest::EError : DDmaRequest::EOk;
		DDmaRequest* pCompletedReq = NULL;
		DDmaRequest* const pCurReq = _LOFF(iReqQ.First(), DDmaRequest, iLink);

		if (res == DDmaRequest::EOk)
			{
			// Update state machine, current fragment, completed fragment and
			// tell the DMAC to transfer the next fragment if necessary.
			TBool complete;
			if (iDmacCaps->iAsymHwDescriptors)
				{
				SDmaDesHdr* pCompletedSrcHdr = NULL;
				SDmaDesHdr* pCompletedDstHdr = NULL;
				DoDfc(*pCurReq, pCompletedSrcHdr, pCompletedDstHdr);
				// We don't support asymmetrical ISR notifications and request
				// completions yet, hence we can do the following assert test
				// here; also 'complete' is determined equally by either the
				// SRC or DST side.
				__DMA_ASSERTD(!LOGICAL_XOR((pCompletedSrcHdr == pCurReq->iSrcLastHdr),
										   (pCompletedDstHdr == pCurReq->iDstLastHdr)));
				complete = (pCompletedDstHdr == pCurReq->iDstLastHdr);
				}
			else
				{
				SDmaDesHdr* pCompletedHdr = NULL;
				DoDfc(*pCurReq, pCompletedHdr);
				complete = (pCompletedHdr == pCurReq->iLastHdr);
				}
			// If just completed last fragment from current request, switch to
			// next request (if any).
			if (complete)
				{
				pCompletedReq = pCurReq;
				pCurReq->iLink.Deque();
				iQueuedRequests--;
				if (iReqQ.IsEmpty())
					ResetNullPtr();
				pCompletedReq->OnDeque();
				}
			}
		else
			{
			pCompletedReq = pCurReq;
			}

		if (pCompletedReq && !pCompletedReq->iIsrCb)
			{
			// Don't execute ISR callbacks here (they have already been called)
			DDmaRequest::TCallback const cb = pCompletedReq->iCb;
			if (cb)
				{
				// Old style callback
				TAny* const arg = pCompletedReq->iCbArg;
				Signal();
				__KTRACE_OPT(KDMA, Kern::Printf("Client CB res=%d", res));
				(*cb)(res, arg);
				Wait();
				}
			else
				{
				// New style callback
				TDmaCallback const ncb = pCompletedReq->iDmaCb;
				if (ncb)
					{
					TAny* const arg = pCompletedReq->iDmaCbArg;
					TDmaResult const result = (res == DDmaRequest::EOk) ?
						EDmaResultOK : EDmaResultError;
					Signal();
					__KTRACE_OPT(KDMA, Kern::Printf("Client CB result=%d", result));
					(*ncb)(EDmaCallbackRequestCompletion, result, arg, NULL);
					Wait();
					}
				}
			}
		// Allow another thread in, in case they are trying to cancel
		if (pCompletedReq || Flash())
			{
			stop = __e32_atomic_load_acq32(&iIsrDfc) & (TUint32)KCancelFlagMask;
			}
		}

	if (stop)
		{
		// If another thread set the cancel flag, it should have
		// cleaned up the request queue
		__DMA_ASSERTD(IsQueueEmpty());

		TDmaCancelInfo* const waiters = iCancelInfo;
		iCancelInfo = NULL;

		// make sure DFC doesn't run again until a new request completes
		iDfc.Cancel();

		// reset the ISR count - new requests can now be processed
		__e32_atomic_store_rel32(&iIsrDfc, 0);

		req_count_after = iQueuedRequests;
		Signal();

		// release threads doing CancelAll()
		waiters->Signal();
		}
	else
		{
		req_count_after = iQueuedRequests;
		Signal();
		}

	// Only call PSL if there were requests queued when we entered AND there
	// are now no requests left on the queue (after also having executed all
	// client callbacks).
	if (iCallQueuedRequestFn)
		{
		if ((req_count_before != 0) && (req_count_after == 0))
			{
			QueuedRequestCountChanged();
			}
		}

	__DMA_INVARIANT();
	}


void TDmaChannel::DoQueue(const DDmaRequest& /*aReq*/)
	{
	// Must be overridden
	__DMA_UNREACHABLE_DEFAULT();
	}


//
// Unlink the last item of a LLI chain from the next chain.
// Default implementation does nothing. This is overridden by scatter-gather
// channels.
//
void TDmaChannel::DoUnlink(SDmaDesHdr& /*aHdr*/)
	{
	}


void TDmaChannel::DoDfc(const DDmaRequest& /*aCurReq*/, SDmaDesHdr*& /*aCompletedHdr*/)
	{
	// To make sure this version of the function isn't called for channels for
	// which it isn't appropriate (and which therefore don't override it) we
	// put this check in here.
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmaChannel::DoDfc(const DDmaRequest& /*aCurReq*/, SDmaDesHdr*& /*aSrcCompletedHdr*/,
						SDmaDesHdr*& /*aDstCompletedHdr*/)
	{
	// To make sure this version of the function isn't called for channels for
	// which it isn't appropriate (and which therefore don't override it) we
	// put this check in here.
	__DMA_UNREACHABLE_DEFAULT();
	}


void TDmaChannel::SetNullPtr(const DDmaRequest& aReq)
	{
	// iNullPtr points to iCurHdr for an empty queue
	*iNullPtr = aReq.iFirstHdr;
	iNullPtr = &(aReq.iLastHdr->iNext);
	}


void TDmaChannel::ResetNullPtr()
	{
	iCurHdr = NULL;
	iNullPtr = &iCurHdr;
	}


/** PSL may override */
void TDmaChannel::QueuedRequestCountChanged()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::QueuedRequestCountChanged(): "
									"disabling further calls"));
	Wait();
	iCallQueuedRequestFn = EFalse;
	Signal();
	}


#ifdef _DEBUG
void TDmaChannel::Invariant()
	{
	Wait();

	__DMA_ASSERTD(iReqCount >= 0);

	__DMA_ASSERTD(iCurHdr == NULL || iController->IsValidHdr(iCurHdr));

	// should always point to NULL pointer ending fragment queue
	__DMA_ASSERTD(*iNullPtr == NULL);

	__DMA_ASSERTD((0 <= iAvailDesCount) && (iAvailDesCount <= iMaxDesCount));

	__DMA_ASSERTD(LOGICAL_XOR(iCurHdr, IsQueueEmpty()));
	if (iCurHdr == NULL)
		{
		__DMA_ASSERTD(iNullPtr == &iCurHdr);
		}

	Signal();
	}
#endif


//////////////////////////////////////////////////////////////////////////////
// TDmaSbChannel

void TDmaSbChannel::DoQueue(const DDmaRequest& /*aReq*/)
	{
	if (iState != ETransferring)
		{
		iController->Transfer(*this, *iCurHdr);
		iState = ETransferring;
		}
	}


void TDmaSbChannel::DoCancelAll()
	{
	__DMA_ASSERTD(iState == ETransferring);
	iState = EIdle;
	}


void TDmaSbChannel::DoDfc(const DDmaRequest& /*aCurReq*/, SDmaDesHdr*& aCompletedHdr)
	{
	__DMA_ASSERTD(iState == ETransferring);
	aCompletedHdr = iCurHdr;
	iCurHdr = iCurHdr->iNext;
	if (iCurHdr != NULL)
		{
		iController->Transfer(*this, *iCurHdr);
		}
	else
		{
		iState = EIdle;
		}
	}


//////////////////////////////////////////////////////////////////////////////
// TDmaDbChannel

void TDmaDbChannel::DoQueue(const DDmaRequest& aReq)
	{
	switch (iState)
		{
	case EIdle:
		iController->Transfer(*this, *iCurHdr);
		if (iCurHdr->iNext)
			{
			iController->Transfer(*this, *(iCurHdr->iNext));
			iState = ETransferring;
			}
		else
			iState = ETransferringLast;
		break;
	case ETransferring:
		// nothing to do
		break;
	case ETransferringLast:
		iController->Transfer(*this, *(aReq.iFirstHdr));
		iState = ETransferring;
		break;
	default:
		__DMA_CANT_HAPPEN();
		}
	}


void TDmaDbChannel::DoCancelAll()
	{
	iState = EIdle;
	}


void TDmaDbChannel::DoDfc(const DDmaRequest& /*aCurReq*/, SDmaDesHdr*& aCompletedHdr)
	{
	aCompletedHdr = iCurHdr;
	iCurHdr = iCurHdr->iNext;
	switch (iState)
		{
	case ETransferringLast:
		iState = EIdle;
		break;
	case ETransferring:
		if (iCurHdr->iNext == NULL)
			iState = ETransferringLast;
		else
			iController->Transfer(*this, *(iCurHdr->iNext));
		break;
	default:
		__DMA_CANT_HAPPEN();
		}
	}


//////////////////////////////////////////////////////////////////////////////
// TDmaSgChannel

void TDmaSgChannel::DoQueue(const DDmaRequest& aReq)
	{
	if (iState == ETransferring)
		{
		__DMA_ASSERTD(!aReq.iLink.Alone());
		DDmaRequest* pReqPrev = _LOFF(aReq.iLink.iPrev, DDmaRequest, iLink);
		iController->AppendHwDes(*this, *(pReqPrev->iLastHdr), *(aReq.iFirstHdr));
		}
	else
		{
		iController->Transfer(*this, *(aReq.iFirstHdr));
		iState = ETransferring;
		}
	}


void TDmaSgChannel::DoCancelAll()
	{
	__DMA_ASSERTD(iState == ETransferring);
	iState = EIdle;
	}


void TDmaSgChannel::DoUnlink(SDmaDesHdr& aHdr)
	{
	iController->UnlinkHwDes(*this, aHdr);
	}


void TDmaSgChannel::DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr)
	{
	__DMA_ASSERTD(iState == ETransferring);
	aCompletedHdr = aCurReq.iLastHdr;
	iCurHdr = aCompletedHdr->iNext;
	iState = (iCurHdr != NULL) ? ETransferring : EIdle;
	}


//////////////////////////////////////////////////////////////////////////////
// TDmaAsymSgChannel

TDmaAsymSgChannel::TDmaAsymSgChannel()
	: iSrcCurHdr(NULL),
	  iSrcNullPtr(&iSrcCurHdr),
	  iDstCurHdr(NULL),
	  iDstNullPtr(&iDstCurHdr)
	{
	__DMA_INVARIANT();
	}


void TDmaAsymSgChannel::SetNullPtr(const DDmaRequest& aReq)
	{
	// i{Src|Dst}NullPtr points to i{Src|Dst}CurHdr for an empty queue
	*iSrcNullPtr = aReq.iSrcFirstHdr;
	*iDstNullPtr = aReq.iDstFirstHdr;
	iSrcNullPtr = &(aReq.iSrcLastHdr->iNext);
	iDstNullPtr = &(aReq.iDstLastHdr->iNext);
	}


void TDmaAsymSgChannel::ResetNullPtr()
	{
	iSrcCurHdr = NULL;
	iSrcNullPtr = &iSrcCurHdr;
	iDstCurHdr = NULL;
	iDstNullPtr = &iDstCurHdr;
	}


void TDmaAsymSgChannel::DoQueue(const DDmaRequest& aReq)
	{
	if (iState == ETransferring)
		{
		__DMA_ASSERTD(!aReq.iLink.Alone());
		DDmaRequest* pReqPrev = _LOFF(aReq.iLink.iPrev, DDmaRequest, iLink);
		iController->AppendHwDes(*this,
								 *(pReqPrev->iSrcLastHdr), *(aReq.iSrcFirstHdr),
								 *(pReqPrev->iDstLastHdr), *(aReq.iDstFirstHdr));
		}
	else
		{
		iController->Transfer(*this, *(aReq.iSrcFirstHdr), *(aReq.iDstFirstHdr));
		iState = ETransferring;
		}
	}


void TDmaAsymSgChannel::DoCancelAll()
	{
	__DMA_ASSERTD(iState == ETransferring);
	iState = EIdle;
	}


void TDmaAsymSgChannel::DoUnlink(SDmaDesHdr& aHdr)
	{
	iController->UnlinkHwDes(*this, aHdr);
	}


void TDmaAsymSgChannel::DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aSrcCompletedHdr,
							  SDmaDesHdr*& aDstCompletedHdr)
	{
	__DMA_ASSERTD(iState == ETransferring);
	aSrcCompletedHdr = aCurReq.iSrcLastHdr;
	iSrcCurHdr = aSrcCompletedHdr->iNext;
	aDstCompletedHdr = aCurReq.iDstLastHdr;
	iDstCurHdr = aDstCompletedHdr->iNext;
	// Must be either both NULL or none of them.
	__DMA_ASSERTD(!LOGICAL_XOR(iSrcCurHdr, iDstCurHdr));
	iState = (iSrcCurHdr != NULL) ? ETransferring : EIdle;
	}


#ifdef _DEBUG
void TDmaAsymSgChannel::Invariant()
	{
	Wait();

	__DMA_ASSERTD(iReqCount >= 0);

	__DMA_ASSERTD(iSrcCurHdr == NULL || iController->IsValidHdr(iSrcCurHdr));
	__DMA_ASSERTD(iDstCurHdr == NULL || iController->IsValidHdr(iDstCurHdr));

	// should always point to NULL pointer ending fragment queue
	__DMA_ASSERTD(*iSrcNullPtr == NULL);
	__DMA_ASSERTD(*iDstNullPtr == NULL);

	__DMA_ASSERTD((0 <= iAvailDesCount) && (iAvailDesCount <= iMaxDesCount));

	__DMA_ASSERTD((iSrcCurHdr && iDstCurHdr && !IsQueueEmpty()) ||
				  (!iSrcCurHdr && !iDstCurHdr && IsQueueEmpty()));
	if (iSrcCurHdr == NULL)
		{
		__DMA_ASSERTD(iSrcNullPtr == &iSrcCurHdr);
		}
	if (iDstCurHdr == NULL)
		{
		__DMA_ASSERTD(iDstNullPtr == &iDstCurHdr);
		}

	Signal();
	}
#endif
