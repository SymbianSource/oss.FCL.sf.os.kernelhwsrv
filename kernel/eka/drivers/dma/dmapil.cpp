// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\dmapil.cpp
// DMA Platform Independent Layer (PIL)
//
//

#include <drivers/dma.h>
#include <kernel/kern_priv.h>


static const char KDmaPanicCat[] = "DMA";

NFastMutex DmaChannelMgr::Lock;

class TDmaCancelInfo : public SDblQueLink
	{
public:
	TDmaCancelInfo();
	void Signal();
public:
	NFastSemaphore iSem;
	};

TDmaCancelInfo::TDmaCancelInfo()
	:	iSem(0)
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
static TInt MaxPhysSize(TLinAddr aLinAddr, const TInt aMaxSize)
	{
	const TPhysAddr physBase = LinToPhys(aLinAddr);
	TLinAddr lin = aLinAddr;
	TInt size = 0;
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
	  iDesSize(aInfo.iDesSize),
	  iCaps(aInfo.iCaps)
	{
	__DMA_ASSERTD(iMaxDesCount > 0);
	__DMA_ASSERTD((iCaps & ~KCapsBitHwDes) == 0); // undefined bits set?
	__DMA_ASSERTD(iDesSize > 0);
	}

//
// Second-phase c'tor
//

TInt TDmac::Create(const SCreateInfo& aInfo)
	{
	iHdrPool = new SDmaDesHdr[iMaxDesCount];
	if (iHdrPool == NULL)
		return KErrNoMemory;

	TInt r = AllocDesPool(aInfo.iDesChunkAttribs);
	if (r != KErrNone)
		return KErrNoMemory;

	// Link all descriptor headers together on the free list
	iFreeHdr = iHdrPool;
	TInt i;
	for (i = 0; i < iMaxDesCount - 1; i++)
		iHdrPool[i].iNext = iHdrPool + i + 1;
	iHdrPool[iMaxDesCount-1].iNext = NULL;

	__DMA_INVARIANT();
	return KErrNone;
	}


TDmac::~TDmac()
	{
	__DMA_INVARIANT();

	FreeDesPool();
	delete[] iHdrPool;
	}


// Calling thread must be in CS
TInt TDmac::AllocDesPool(TUint aAttribs)
	{
	TInt r;
	if (iCaps & KCapsBitHwDes)
		{
		TInt size = iMaxDesCount*iDesSize;
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
		iDesPool = new SDmaPseudoDes[iMaxDesCount];
		r = iDesPool ? KErrNone : KErrNoMemory;
		}
	return r;
	}


// Calling thread must be in CS
void TDmac::FreeDesPool()
	{
	if (iCaps & KCapsBitHwDes)
		{
#ifdef __WINS__
		delete[] iDesPool;
#else
		if (iHwDesChunk)
			{
			TPhysAddr phys = iHwDesChunk->PhysicalAddress();
			TInt size = iHwDesChunk->iSize;
			iHwDesChunk->Close(NULL);
			Epoc::FreePhysicalRam(phys, size);
			}
#endif
		}
	else
		Kern::Free(iDesPool); 
	}


/**
 Prealloc the given number of descriptors.
 */

TInt TDmac::ReserveSetOfDes(TInt aCount)
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">TDmac::ReserveSetOfDes count=%d", aCount));
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
	__KTRACE_OPT(KDMA, Kern::Printf("<TDmac::ReserveSetOfDes r=%d", r));
	return r;
	}


/**
 Return the given number of preallocated descriptors to the free pool.
 */

void TDmac::ReleaseSetOfDes(TInt aCount)
	{
	__DMA_ASSERTD(aCount >= 0);
	Wait();
	iAvailDesCount += aCount;
	Signal();
	__DMA_INVARIANT();
	}


/**
 Queue DFC and update word used to communicate with DFC.

 Called in interrupt context by PSL.
 */

void TDmac::HandleIsr(TDmaChannel& aChannel, TBool aIsComplete)
	{
	//__KTRACE_OPT(KDMA, Kern::Printf("TDmac::HandleIsr channel=%d complete=%d", aChannelIdx, aIsComplete));

	// Queue DFC if necessary.  The possible scenarios are:
	// * no DFC queued --> need to queue DFC
	// * DFC queued (not running yet) --> just need to update iIsrDfc
	// * DFC running / iIsrDfc already reset --> need to requeue DFC
	// * DFC running /  iIsrDfc not reset yet --> just need to update iIsrDfc
	// Set error flag if necessary.
	TUint32 inc = aIsComplete ? 1u : TUint32(TDmaChannel::KErrorFlagMask)|1u;
	TUint32 orig = __e32_atomic_tau_ord32(&aChannel.iIsrDfc, TUint32(TDmaChannel::KCancelFlagMask), 0, inc);

	// As transfer should be suspended when an error occurs, we
	// should never get there with the error flag already set.
	__DMA_ASSERTD((orig & inc & (TUint32)TDmaChannel::KErrorFlagMask) == 0);

	if (orig == 0)
		aChannel.iDfc.Add();
	}


void TDmac::InitDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
					TUint aFlags, TUint32 aPslInfo, TUint32 aCookie)
	{
 	if (iCaps & KCapsBitHwDes)
		InitHwDes(aHdr, aSrc, aDest, aCount, aFlags, aPslInfo, aCookie);
	else
		{
		SDmaPseudoDes& des = HdrToDes(aHdr);
		des.iSrc = aSrc;
		des.iDest = aDest;
		des.iCount = aCount;
		des.iFlags = aFlags;
		des.iPslInfo = aPslInfo;
		des.iCookie = aCookie;
		}
	}


void TDmac::InitHwDes(const SDmaDesHdr& /*aHdr*/, TUint32 /*aSrc*/, TUint32 /*aDest*/, TInt /*aCount*/,
					  TUint /*aFlags*/, TUint32 /*aPslInfo*/, TUint32 /*aCookie*/)
	{
	// concrete controller must override if KCapsBitHwDes set
	__DMA_CANT_HAPPEN();
	}


void TDmac::ChainHwDes(const SDmaDesHdr& /*aHdr*/, const SDmaDesHdr& /*aNextHdr*/)
	{
	// concrete controller must override if KCapsBitHwDes set
	__DMA_CANT_HAPPEN();
	}


void TDmac::AppendHwDes(const TDmaChannel& /*aChannel*/, const SDmaDesHdr& /*aLastHdr*/,
						const SDmaDesHdr& /*aNewHdr*/)
	{
 	// concrete controller must override if KCapsBitHwDes set
	__DMA_CANT_HAPPEN();
	}


void TDmac::UnlinkHwDes(const TDmaChannel& /*aChannel*/, SDmaDesHdr& /*aHdr*/)
	{
 	// concrete controller must override if KCapsBitHwDes set
	__DMA_CANT_HAPPEN();
	}


TInt TDmac::FailNext(const TDmaChannel& /*aChannel*/)
	{
	return KErrNotSupported;
	}


TInt TDmac::MissNextInterrupts(const TDmaChannel& /*aChannel*/, TInt /*aInterruptCount*/)
	{
	return KErrNotSupported;
	}


TInt TDmac::Extension(TDmaChannel& /*aChannel*/, TInt /*aCmd*/, TAny* /*aArg*/)
	{
	// default implementation - NOP
	return KErrNotSupported;
	}


#ifdef _DEBUG

void TDmac::Invariant()
	{
	Wait();
	__DMA_ASSERTD(0 <= iAvailDesCount && iAvailDesCount <= iMaxDesCount);
	__DMA_ASSERTD(! iFreeHdr || IsValidHdr(iFreeHdr));
	for (TInt i = 0; i < iMaxDesCount; i++)
		__DMA_ASSERTD(iHdrPool[i].iNext == NULL || IsValidHdr(iHdrPool[i].iNext));
	Signal();
	}


TBool TDmac::IsValidHdr(const SDmaDesHdr* aHdr)
	{
	return (iHdrPool <= aHdr) && (aHdr < iHdrPool + iMaxDesCount);
	}

#endif

//////////////////////////////////////////////////////////////////////////////
// DDmaRequest


EXPORT_C DDmaRequest::DDmaRequest(TDmaChannel& aChannel, TCallback aCb, TAny* aCbArg, TInt aMaxTransferSize)
	: iChannel(aChannel),
	  iCb(aCb),
	  iCbArg(aCbArg),
	  iMaxTransferSize(aMaxTransferSize)
	{
	// iDesCount = 0;
	// iFirstHdr = iLastHdr = NULL;
	// iQueued = EFalse;
	iChannel.iReqCount++;
	__DMA_INVARIANT();
	}



EXPORT_C DDmaRequest::~DDmaRequest()
	{
	__DMA_ASSERTD(!iQueued);
	__DMA_INVARIANT();
	FreeDesList();
	iChannel.iReqCount--;
	}



EXPORT_C TInt DDmaRequest::Fragment(TUint32 aSrc, TUint32 aDest, TInt aCount, TUint aFlags, TUint32 aPslInfo)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Fragment thread %O "
									"src=0x%08X dest=0x%08X count=%d flags=0x%X psl=0x%08X",
									&Kern::CurrentThread(), aSrc, aDest, aCount, aFlags, aPslInfo));
	__DMA_ASSERTD(aCount > 0);
	__DMA_ASSERTD(!iQueued);

	const TUint alignMask = iChannel.MemAlignMask(aFlags, aPslInfo);
	const TBool memSrc  = aFlags & KDmaMemSrc;
	const TBool memDest = aFlags & KDmaMemDest;

	// Memory buffers must satisfy alignment constraint
	__DMA_ASSERTD(!memSrc || ((aSrc & alignMask) == 0));
	__DMA_ASSERTD(!memDest || ((aDest & alignMask) == 0));

	// Ask the PSL what the maximum size possible for this transfer is
	TInt maxTransferSize = iChannel.MaxTransferSize(aFlags, aPslInfo);
	if (!maxTransferSize)
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("Error: maxTransferSize == 0"));
		return KErrArgument;
		}

	if (iMaxTransferSize)
		{
		// User has set a size cap
		__DMA_ASSERTA((iMaxTransferSize <= maxTransferSize) || (maxTransferSize == -1));
		maxTransferSize = iMaxTransferSize;
		}
	else
		{
		// User doesn't care about max size
		if (maxTransferSize == -1)
			{
			// No maximum imposed by controller
			maxTransferSize = aCount;
			}
		}

	const TInt maxAlignedSize = (maxTransferSize & ~alignMask);
	__DMA_ASSERTD(maxAlignedSize > 0);						// bug in PSL if not true

	FreeDesList();

	TInt r = KErrNone;
	do
		{
		// Allocate fragment
		r = ExpandDesList();
		if (r != KErrNone)
			{
			FreeDesList();
			break;
			}

		// Compute fragment size
		TInt c = Min(maxTransferSize, aCount);
		if (memSrc && ((aFlags & KDmaPhysAddrSrc) == 0))
			c = MaxPhysSize(aSrc, c);
		if (memDest && ((aFlags & KDmaPhysAddrDest) == 0))
			c = MaxPhysSize(aDest, c);
		if ((memSrc || memDest) && (c < aCount) && (c > maxAlignedSize))
			{
			// This is not last fragment of transfer to/from memory. We must
			// round down fragment size so next one is correctly aligned.
			c = maxAlignedSize;
			}

		// Initialise fragment
		__KTRACE_OPT(KDMA, Kern::Printf("fragment: src=0x%08X dest=0x%08X count=%d", aSrc, aDest, c));
		iChannel.iController->InitDes(*iLastHdr, aSrc, aDest, c, aFlags, aPslInfo, iChannel.PslId());

		// Update for next iteration
		aCount -= c;
		if (memSrc)
			aSrc += c;
		if (memDest)
			aDest += c;
		}
	while (aCount > 0);

	__DMA_INVARIANT();
	return r;
	}



EXPORT_C void DDmaRequest::Queue()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaRequest::Queue thread %O", &Kern::CurrentThread()));
	__DMA_ASSERTD(iDesCount > 0);	// Not configured? call Fragment() first !
	__DMA_ASSERTD(!iQueued);

	// append request to queue and link new descriptor list to existing one.
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

	if (!(iChannel.iIsrDfc & (TUint32)TDmaChannel::KCancelFlagMask))
		{
		iQueued = ETrue;
		iChannel.iReqQ.Add(&iLink);
		*iChannel.iNullPtr = iFirstHdr;
		iChannel.iNullPtr = &(iLastHdr->iNext);
		iChannel.DoQueue(*this);
		__DMA_INVARIANT();
		iChannel.Signal();
		}
	else
		{
		// Someone is cancelling all requests...
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
	}

EXPORT_C TInt DDmaRequest::ExpandDesList(TInt aCount)
	{
	__DMA_ASSERTD(!iQueued);
	__DMA_ASSERTD(aCount > 0);

	if (aCount > iChannel.iAvailDesCount)
		return KErrTooBig;

	iChannel.iAvailDesCount -= aCount;
	iDesCount += aCount;

	TDmac& c = *(iChannel.iController);
	c.Wait();

	if (iFirstHdr == NULL)
		{
		// handle empty list specially to simplify following loop
		iFirstHdr = iLastHdr = c.iFreeHdr;
		c.iFreeHdr = c.iFreeHdr->iNext;
		--aCount;
		}
	else
		iLastHdr->iNext = c.iFreeHdr;

	// Remove as many descriptors and headers from free pool as necessary and
	// ensure hardware descriptors are chained together.
	while (aCount-- > 0)
		{
		__DMA_ASSERTD(c.iFreeHdr != NULL);
		if (c.iCaps & TDmac::KCapsBitHwDes)
			c.ChainHwDes(*iLastHdr, *(c.iFreeHdr));
		iLastHdr = c.iFreeHdr;
		c.iFreeHdr = c.iFreeHdr->iNext;
		}

	c.Signal();

	iLastHdr->iNext = NULL;

	__DMA_INVARIANT();
	return KErrNone;
	}




EXPORT_C void DDmaRequest::FreeDesList()
	{
	__DMA_ASSERTD(!iQueued);
	if (iDesCount > 0)
		{
		iChannel.iAvailDesCount += iDesCount;
		TDmac& c = *(iChannel.iController);
		c.Wait();
		iLastHdr->iNext = c.iFreeHdr;
		c.iFreeHdr = iFirstHdr;
		c.Signal();
		iFirstHdr = iLastHdr = NULL;
		iDesCount = 0;
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

	__DMA_ASSERTD(iChannel.IsOpened());
	__DMA_ASSERTD(0 <= iMaxTransferSize);
	__DMA_ASSERTD(0 <= iDesCount && iDesCount <= iChannel.iMaxDesCount);
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

	if(channelLockAquired)
		{
		iChannel.Signal();
		}
	}

#endif


//////////////////////////////////////////////////////////////////////////////
// TDmaChannel


EXPORT_C TInt TDmaChannel::StaticExtension(TInt aCmd, TAny* aArg)
	{
	return DmaChannelMgr::StaticExtension(aCmd, aArg);
	}


TDmaChannel::TDmaChannel()
	: iController(NULL),
	  iPslId(0),
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
	  iCancelInfo(NULL)
	{
	__DMA_INVARIANT();
	}


EXPORT_C TInt TDmaChannel::Open(const SCreateInfo& aInfo, TDmaChannel*& aChannel)
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Open thread %O", &Kern::CurrentThread()));
	__DMA_ASSERTD(aInfo.iDfcQ != NULL);
	__DMA_ASSERTD(aInfo.iDfcPriority < KNumDfcPriorities);
	__DMA_ASSERTD(aInfo.iDesCount >= 1);

	aChannel = NULL;

	DmaChannelMgr::Wait();
	TDmaChannel* pC = DmaChannelMgr::Open(aInfo.iCookie);
	DmaChannelMgr::Signal();
	if (!pC)
		return KErrInUse;

	TInt r = pC->iController->ReserveSetOfDes(aInfo.iDesCount);
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
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::Close %d", iPslId));
	__DMA_ASSERTD(IsOpened());
	__DMA_ASSERTD(IsQueueEmpty());
	__DMA_ASSERTD(iReqCount == 0);

	__DMA_ASSERTD(iQueuedRequests == 0);

	// descriptor leak? bug in request code
	__DMA_ASSERTD(iAvailDesCount == iMaxDesCount);

	iController->ReleaseSetOfDes(iMaxDesCount);
	iAvailDesCount = iMaxDesCount = 0;

	DmaChannelMgr::Wait();
	DmaChannelMgr::Close(this);
	iController = NULL;
	DmaChannelMgr::Signal();

	__DMA_INVARIANT();
	}


EXPORT_C void TDmaChannel::CancelAll()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("TDmaChannel::CancelAll thread %O channel - %d",
									&Kern::CurrentThread(), iPslId));
	__DMA_ASSERTD(IsOpened());

	NThread* nt = NKern::CurrentThread();
	TBool wait = FALSE;
	TDmaCancelInfo c;
	TDmaCancelInfo* waiters = 0;

	NKern::ThreadEnterCS();
	Wait();
	const TUint32 req_count_before = iQueuedRequests;
	NThreadBase* dfcnt = iDfc.Thread();
	__e32_atomic_store_ord32(&iIsrDfc, (TUint32)KCancelFlagMask);
	// ISRs after this point will not post a DFC, however a DFC may already be queued or running or both
	if (!IsQueueEmpty())
		{
		// There is a transfer in progress.  It may complete before the DMAC
		// has stopped, but the resulting ISR will not post a DFC.
		// ISR should not happen after this function returns.
		iController->StopTransfer(*this);

		ResetStateMachine();

		// Clean-up the request queue.
		SDblQueLink* pL;
		while ((pL = iReqQ.GetFirst()) != NULL)
			{
			iQueuedRequests--;
			DDmaRequest* pR = _LOFF(pL, DDmaRequest, iLink);
			pR->OnDeque();
			}
		}
	if (!dfcnt || dfcnt==nt)
		{
		// no DFC queue or DFC runs in this thread, so just cancel it and we're finished
		iDfc.Cancel();

		// if other calls to CancelAll() are waiting for the DFC, release them here
		waiters = iCancelInfo;
		iCancelInfo = 0;

		// reset the ISR count
		__e32_atomic_store_rel32(&iIsrDfc, 0);
		}
	else
		{
		// DFC runs in another thread. Make sure it's queued and then wait for it to run.
		if (iCancelInfo)
			c.InsertBefore(iCancelInfo);
		else
			iCancelInfo = &c;
		wait = TRUE;
		iDfc.Enque();
		}
	const TUint32 req_count_after = iQueuedRequests;
	Signal();
	if (waiters)
		waiters->Signal();
	if (wait)
		NKern::FSWait(&c.iSem);
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


/**
 DFC callback function (static member).
 */

void TDmaChannel::Dfc(TAny* aArg)
	{
	((TDmaChannel*)aArg)->DoDfc();
	}


void TDmaChannel::DoDfc()
	{
	Wait();

	// Atomically fetch and reset the number of DFC queued by ISR and the error
	// flag. Leave the cancel flag alone for now.
	const TUint32 w = __e32_atomic_and_ord32(&iIsrDfc, (TUint32)KCancelFlagMask);
	TUint32 count = w & KDfcCountMask;
	const TBool error = w & (TUint32)KErrorFlagMask;
	TBool stop = w & (TUint32)KCancelFlagMask;
	__DMA_ASSERTD(count>0 || stop);
	const TUint32 req_count_before = iQueuedRequests;
	TUint32 req_count_after = 0;

	while(count && !stop)
		{
		--count;

		// If an error occurred it must have been reported on the last interrupt since transfers are
		// suspended after an error.
		DDmaRequest::TResult res = (count==0 && error) ? DDmaRequest::EError : DDmaRequest::EOk;
		__DMA_ASSERTA(!iReqQ.IsEmpty());
		DDmaRequest* pCompletedReq = NULL;
		DDmaRequest* pCurReq = _LOFF(iReqQ.First(), DDmaRequest, iLink);
		DDmaRequest::TCallback cb = 0;
		TAny* arg = 0;

		if (res == DDmaRequest::EOk)
			{
			// Update state machine, current fragment, completed fragment and
			// tell DMAC to transfer next fragment if necessary.
			SDmaDesHdr* pCompletedHdr = NULL;
			DoDfc(*pCurReq, pCompletedHdr);

			// If just completed last fragment from current request, switch to next
			// request (if any).
			if (pCompletedHdr == pCurReq->iLastHdr)
				{
				pCompletedReq = pCurReq;
				pCurReq->iLink.Deque();
				iQueuedRequests--;
				if (iReqQ.IsEmpty())
					iNullPtr = &iCurHdr;
				pCompletedReq->OnDeque();
				}
			}
		else if (res == DDmaRequest::EError)
			pCompletedReq = pCurReq;
		else
			__DMA_CANT_HAPPEN();
		if (pCompletedReq)
			{
			cb = pCompletedReq->iCb;
			arg = pCompletedReq->iCbArg;
			Signal();
			__KTRACE_OPT(KDMA, Kern::Printf("notifying DMA client result=%d", res));
			(*cb)(res,arg);
			Wait();
			}
		if (pCompletedReq || Flash())
			stop = __e32_atomic_load_acq32(&iIsrDfc) & (TUint32)KCancelFlagMask;
		}

	// Some interrupts may be missed (double-buffer and scatter-gather
	// controllers only) if two or more transfers complete while interrupts are
	// disabled in the CPU. If this happens, the framework will go out of sync
	// and leave some orphaned requests in the queue.
	//
	// To ensure correctness we handle this case here by checking that the request
	// queue is empty when all transfers have completed and, if not, cleaning up
	// and notifying the client of the completion of the orphaned requests.
	//
	// Note that if some interrupts are missed and the controller raises an
	// error while transferring a subsequent fragment, the error will be reported
	// on a fragment which was successfully completed.  There is no easy solution
	// to this problem, but this is okay as the only possible action following a
	// failure is to flush the whole queue.
	if (stop)
		{
		TDmaCancelInfo* waiters = iCancelInfo;
		iCancelInfo = 0;

		// make sure DFC doesn't run again until a new request completes
		iDfc.Cancel();

		// reset the ISR count - new requests can now be processed
		__e32_atomic_store_rel32(&iIsrDfc, 0);

		req_count_after = iQueuedRequests;
		Signal();

		// release threads doing CancelAll()
		waiters->Signal();
		}
#ifndef DISABLE_MISSED_IRQ_RECOVERY
	// (iController may be NULL here if the channel was closed in the client callback.)
	else if (!error &&
			 iController && iController->IsIdle(*this) &&
			 !iReqQ.IsEmpty() &&
			 !iDfc.Queued())
		{
		// Wait for a bit. If during that time the condition goes away then it
		// was a 'spurious missed interrupt', in which case we just do nothing.
		TBool spurious = EFalse;
		const TUint32 nano_secs_per_loop = 1000 * 1000;			// 1ms
		for (TInt i = 5; i > 0; i--)
			{
			if (!iController->IsIdle(*this))
				{
				__KTRACE_OPT(KDMA, Kern::Printf("DMAC no longer idle (i = %d)", i));
				spurious = ETrue;
				break;
				}
			else if (iDfc.Queued())
				{
				__KTRACE_OPT(KDMA, Kern::Printf("DFC now queued (i = %d)", i));
				spurious = ETrue;
				break;
				}
			Kern::NanoWait(nano_secs_per_loop);
			}
		if (!spurious)
			{
			__KTRACE_OPT(KDMA,
						 Kern::Printf("Missed interrupt(s) - draining request queue on ch %d",
									  PslId()));
			ResetStateMachine();

			// Move orphaned requests to temporary queue so channel queue can
			// accept new requests.
			SDblQue q;
			q.MoveFrom(&iReqQ);

			SDblQueLink* pL;
			while ((pL = q.GetFirst()) != NULL)
				{
				iQueuedRequests--;
				DDmaRequest* pR = _LOFF(pL, DDmaRequest, iLink);
				__KTRACE_OPT(KDMA, Kern::Printf("Removing request from queue and notifying client"));
				pR->OnDeque();
				DDmaRequest::TCallback cb = pR->iCb;
				TAny* arg = pR->iCbArg;
				if (cb)
					{
					Signal();
					(*cb)(DDmaRequest::EOk, arg);
					Wait();
					}
				}
			}
		req_count_after = iQueuedRequests;
		Signal();
		}
#endif  // #ifndef DISABLE_MISSED_IRQ_RECOVERY
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


/** Reset state machine only, request queue is unchanged */

void TDmaChannel::ResetStateMachine()
	{
	DoCancelAll();
	iCurHdr = NULL;
	iNullPtr = &iCurHdr;
	}


/** Unlink the last item of a LLI chain from the next chain.
	Default implementation does nothing. This is overridden by scatter-gather channels. */

void TDmaChannel::DoUnlink(SDmaDesHdr& /*aHdr*/)
	{
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
	// should always point to NULL pointer ending fragment queue
	__DMA_ASSERTD(*iNullPtr == NULL);

	__DMA_ASSERTD(0 <= iAvailDesCount && iAvailDesCount <= iMaxDesCount);

	__DMA_ASSERTD(iCurHdr == NULL || iController->IsValidHdr(iCurHdr));

	if (IsOpened())
		{
		__DMA_ASSERTD((iCurHdr && !IsQueueEmpty()) || (!iCurHdr && IsQueueEmpty()));
		if (iCurHdr == NULL)
			__DMA_ASSERTD(iNullPtr == &iCurHdr);
		}
	else
		{
		__DMA_ASSERTD(iCurHdr == NULL);
		__DMA_ASSERTD(iNullPtr == &iCurHdr);
		__DMA_ASSERTD(IsQueueEmpty());
		}

	Signal();
	}

#endif

//////////////////////////////////////////////////////////////////////////////
// TDmaSbChannel

void TDmaSbChannel::DoQueue(DDmaRequest& /*aReq*/)
	{
	if (!iTransferring)
		{
		iController->Transfer(*this, *iCurHdr);
		iTransferring = ETrue;
		}
	}


void TDmaSbChannel::DoCancelAll()
	{
	__DMA_ASSERTD(iTransferring);
	iTransferring = EFalse;
	}


void TDmaSgChannel::DoUnlink(SDmaDesHdr& aHdr)
	{
	iController->UnlinkHwDes(*this, aHdr);
	}


void TDmaSbChannel::DoDfc(DDmaRequest& /*aCurReq*/, SDmaDesHdr*& aCompletedHdr)
	{
	__DMA_ASSERTD(iTransferring);
	aCompletedHdr = iCurHdr;
	iCurHdr = iCurHdr->iNext;
	if (iCurHdr != NULL)
		iController->Transfer(*this, *iCurHdr);
	else
		iTransferring = EFalse;
	}


//////////////////////////////////////////////////////////////////////////////
// TDmaDbChannel

void TDmaDbChannel::DoQueue(DDmaRequest& aReq)
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


void TDmaDbChannel::DoDfc(DDmaRequest& /*aCurReq*/, SDmaDesHdr*& aCompletedHdr)
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

void TDmaSgChannel::DoQueue(DDmaRequest& aReq)
	{
	if (iTransferring)
		{
		__DMA_ASSERTD(!aReq.iLink.Alone());
		DDmaRequest* pReqPrev = _LOFF(aReq.iLink.iPrev, DDmaRequest, iLink);
		iController->AppendHwDes(*this, *(pReqPrev->iLastHdr), *(aReq.iFirstHdr));
		}
	else
		{
		iController->Transfer(*this, *(aReq.iFirstHdr));
		iTransferring = ETrue;
		}
	}


void TDmaSgChannel::DoCancelAll()
	{
	__DMA_ASSERTD(iTransferring);
	iTransferring = EFalse;
	}


void TDmaSgChannel::DoDfc(DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr)
	{
	__DMA_ASSERTD(iTransferring);
	aCompletedHdr = aCurReq.iLastHdr;
	iCurHdr = aCompletedHdr->iNext;
	iTransferring = (iCurHdr != NULL);
	}
