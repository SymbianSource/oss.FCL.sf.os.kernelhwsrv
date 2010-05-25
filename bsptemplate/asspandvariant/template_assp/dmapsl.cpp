// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// bsptemplate/asspvariant/template_assp/dmapsl.cpp
// Template DMA Platform Specific Layer (PSL).
//
//


#include <kernel/kern_priv.h>
#include <template_assp.h>									// /assp/template_assp/

#include <drivers/dma.h>


// Debug support
static const char KDmaPanicCat[] = "DMA PSL - " __FILE__;

static const TInt KMaxTransferLen = 0x1FE0;					// max transfer length for this DMAC
static const TInt KMemAlignMask = 7;				  // memory addresses passed to DMAC must be multiple of 8
static const TInt KChannelCount = 16;						// we got 16 channels
static const TInt KDesCount = 1024;							// DMA descriptor count


class TDmaDesc
//
// Hardware DMA descriptor
//
	{
public:
	enum {KStopBitMask = 1};
public:
	TPhysAddr iDescAddr;
	TPhysAddr iSrcAddr;
	TPhysAddr iDestAddr;
	TUint32 iCmd;
	};


//////////////////////////////////////////////////////////////////////////////
// Test Support
//////////////////////////////////////////////////////////////////////////////

/**
TO DO: Fill in to provide information to the V1 test harness (t_dma.exe)
*/
TDmaTestInfo TestInfo =
	{
	0,
	0,
	0,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL
	};


EXPORT_C const TDmaTestInfo& DmaTestInfo()
//
//
//
	{
	return TestInfo;
	}


//////////////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////////////

inline TBool IsHwDesAligned(TAny* aDes)
//
// Checks whether given hardware descriptor is 16-bytes aligned.
//
	{
	return ((TLinAddr)aDes & 0xF) == 0;
	}


static TUint32 DcmdReg(TInt aCount, TUint aFlags, TUint32 aPslInfo)
//
// Returns value to set in DMA command register or in descriptor command field.
//
	{
	// TO DO: Construct CMD word from input values.
	// The return value should reflect the actual control word.
	return (aCount | aFlags | aPslInfo);
	}


//////////////////////////////////////////////////////////////////////////////
// Derived Channel (Scatter/Gather)
//////////////////////////////////////////////////////////////////////////////

class TTemplateSgChannel : public TDmaSgChannel
	{
public:
	TDmaDesc* iTmpDes;
	TPhysAddr iTmpDesPhysAddr;
	};


//////////////////////////////////////////////////////////////////////////////
// Derived Controller Class
//////////////////////////////////////////////////////////////////////////////

class TTemplateDmac : public TDmac
	{
public:
	TTemplateDmac();
	TInt Create();
private:
	// from TDmac (PIL pure virtual)
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr);
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TInt MaxTransferSize(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	virtual TUint MemAlignMask(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	// from TDmac (PIL virtual)
	virtual void InitHwDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
 						   TUint aFlags, TUint32 aPslInfo, TUint32 aCookie);
	virtual void ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr);
	virtual void AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
							 const SDmaDesHdr& aNewHdr);
	virtual void UnlinkHwDes(const TDmaChannel& aChannel, SDmaDesHdr& aHdr);
	// other
	static void Isr(TAny* aThis);
	inline TDmaDesc* HdrToHwDes(const SDmaDesHdr& aHdr);
private:
	static const SCreateInfo KInfo;
public:
	TTemplateSgChannel iChannels[KChannelCount];
	};


static TTemplateDmac Controller;


const TDmac::SCreateInfo TTemplateDmac::KInfo =
	{
	KChannelCount,
	KDesCount,
	TDmac::KCapsBitHwDes,
	sizeof(TDmaDesc),
	EMapAttrSupRw | EMapAttrFullyBlocking
	};


TTemplateDmac::TTemplateDmac()
//
// Constructor.
//
	: TDmac(KInfo)
	{}


TInt TTemplateDmac::Create()
//
// Second phase construction.
//
	{
	TInt r = TDmac::Create(KInfo);							// Base class Create()
	if (r == KErrNone)
		{
		__DMA_ASSERTA(ReserveSetOfDes(KChannelCount) == KErrNone);
		for (TInt i=0; i < KChannelCount; ++i)
			{
			TDmaDesc* pD = HdrToHwDes(*iFreeHdr);
			iChannels[i].iTmpDes = pD;
			iChannels[i].iTmpDesPhysAddr = DesLinToPhys(pD);
			iFreeHdr = iFreeHdr->iNext;
			}
		r = Interrupt::Bind(EAsspIntIdDma, Isr, this);
		if (r == KErrNone)
			{
			// TO DO: Map DMA clients (requests) to DMA channels here.

			r = Interrupt::Enable(EAsspIntIdDma);
			}
		}
	return r;
	}


void TTemplateDmac::Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr)
//
// Initiates a (previously constructed) request on a specific channel.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());
	TDmaDesc* pD = HdrToHwDes(aHdr);

	__KTRACE_OPT(KDMA, Kern::Printf(">TTemplateDmac::Transfer channel=%d des=0x%08X", i, pD));

	// TO DO (for instance): Load the first descriptor address into the DMAC and start it
	// by setting the RUN bit.
	(void) *pD, (void) i;

	}


void TTemplateDmac::StopTransfer(const TDmaChannel& aChannel)
//
// Stops a running channel.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());

	__KTRACE_OPT(KDMA, Kern::Printf(">TTemplateDmac::StopTransfer channel=%d", i));

	// TO DO (for instance): Clear the RUN bit of the channel.
	(void) i;

	}


TBool TTemplateDmac::IsIdle(const TDmaChannel& aChannel)
//
// Returns the state of a given channel.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());

	__KTRACE_OPT(KDMA, Kern::Printf(">TTemplateDmac::IsIdle channel=%d", i));

	// TO DO (for instance): Return the state of the RUN bit of the channel.
	// The return value should reflect the actual state.
	(void) i;

	return ETrue;
	}


TInt TTemplateDmac::MaxTransferSize(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the maximum transfer size for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMaxTransferLen;
	}


TUint TTemplateDmac::MemAlignMask(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the memory buffer alignment restrictions mask for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMemAlignMask;
	}


void TTemplateDmac::InitHwDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
							  TUint aFlags, TUint32 aPslInfo, TUint32 /*aCookie*/)
//
// Sets up (from a passed in request) the descriptor with that fragment's
// source and destination address, the fragment size, and the (driver/DMA
// controller) specific transfer parameters (mem/peripheral, burst size,
// transfer width).
//
	{
	TDmaDesc* pD = HdrToHwDes(aHdr);

	__KTRACE_OPT(KDMA, Kern::Printf("TTemplateDmac::InitHwDes 0x%08X", pD));

	// Unaligned descriptor? Bug in generic layer!
	__DMA_ASSERTD(IsHwDesAligned(pD));

	pD->iSrcAddr = (aFlags & KDmaPhysAddrSrc) ? aSrc : Epoc::LinearToPhysical(aSrc);
	__DMA_ASSERTD(pD->iSrcAddr != KPhysAddrInvalid);
	pD->iDestAddr = (aFlags & KDmaPhysAddrDest) ? aDest : Epoc::LinearToPhysical(aDest);
	__DMA_ASSERTD(pD->iDestAddr != KPhysAddrInvalid);
	pD->iCmd = DcmdReg(aCount, aFlags, aPslInfo);
	pD->iDescAddr = TDmaDesc::KStopBitMask;
	}


void TTemplateDmac::ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr)
//
// Chains hardware descriptors together by setting the next pointer of the original descriptor
// to the physical address of the descriptor to be chained.
//
	{
	TDmaDesc* pD = HdrToHwDes(aHdr);
	TDmaDesc* pN = HdrToHwDes(aNextHdr);

	__KTRACE_OPT(KDMA, Kern::Printf("TTemplateDmac::ChainHwDes des=0x%08X next des=0x%08X", pD, pN));

	// Unaligned descriptor? Bug in generic layer!
	__DMA_ASSERTD(IsHwDesAligned(pD) && IsHwDesAligned(pN));

	// TO DO: Modify pD->iCmd so that no end-of-transfer interrupt gets raised any longer.

	pD->iDescAddr = DesLinToPhys(pN);
	}


void TTemplateDmac::AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
								const SDmaDesHdr& aNewHdr)
//
// Appends a descriptor to the chain while the channel is running.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());

	TDmaDesc* pL = HdrToHwDes(aLastHdr);
	TDmaDesc* pN = HdrToHwDes(aNewHdr);

	__KTRACE_OPT(KDMA, Kern::Printf(">TTemplateDmac::AppendHwDes channel=%d last des=0x%08X new des=0x%08X",
									i, pL, pN));
	// Unaligned descriptor? Bug in generic layer!
	__DMA_ASSERTD(IsHwDesAligned(pL) && IsHwDesAligned(pN));

	TPhysAddr newPhys = DesLinToPhys(pN);

	const TInt irq = NKern::DisableAllInterrupts();
	StopTransfer(aChannel);

	pL->iDescAddr = newPhys;
	const TTemplateSgChannel& channel = static_cast<const TTemplateSgChannel&>(aChannel);
	TDmaDesc* pD = channel.iTmpDes;

	// TO DO: Implement the appropriate algorithm for appending a descriptor here.
	(void) *pD, (void) i;

	NKern::RestoreInterrupts(irq);

	__KTRACE_OPT(KDMA, Kern::Printf("<TTemplateDmac::AppendHwDes"));
	}


void TTemplateDmac::UnlinkHwDes(const TDmaChannel& /*aChannel*/, SDmaDesHdr& aHdr)
//
// Unlink the last item in the h/w descriptor chain from a subsequent chain that it was
// possibly linked to.
//
	{
 	__KTRACE_OPT(KDMA, Kern::Printf(">TTemplateDmac::UnlinkHwDes"));
  	TDmaDesc* pD = HdrToHwDes(aHdr);
  	pD->iDescAddr = TDmaDesc::KStopBitMask;

	// TO DO: Modify pD->iCmd so that an end-of-transfer interrupt will get raised.

	}


void TTemplateDmac::Isr(TAny* aThis)
//
// This ISR reads the interrupt identification and calls back into the base class
// interrupt service handler with the channel identifier and an indication whether the
// transfer completed correctly or with an error.
//
	{
	TTemplateDmac& me = *static_cast<TTemplateDmac*>(aThis);

	// TO DO: Implement the behaviour described above, call HandleIsr().

	HandleIsr(me.iChannels[5], 0);							// Example

	}


inline TDmaDesc* TTemplateDmac::HdrToHwDes(const SDmaDesHdr& aHdr)
//
// Changes return type of base class call.
//
	{
	return static_cast<TDmaDesc*>(TDmac::HdrToHwDes(aHdr));
	}


//////////////////////////////////////////////////////////////////////////////
// Channel Opening/Closing (Channel Allocator)
//////////////////////////////////////////////////////////////////////////////

TDmaChannel* DmaChannelMgr::Open(TUint32 aOpenId)
//
//
//
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">DmaChannelMgr::Open aOpenId=%d", aOpenId));

	__DMA_ASSERTA(aOpenId < static_cast<TUint32>(KChannelCount));

	TDmaChannel* pC = Controller.iChannels + aOpenId;
	if (pC->IsOpened())
		{
		pC = NULL;
		}
	else
		{
		pC->iController = &Controller;
		pC->iPslId = aOpenId;
		}

	return pC;
	}


void DmaChannelMgr::Close(TDmaChannel* /*aChannel*/)
//
//
//
	{
	// NOP
	}


TInt DmaChannelMgr::StaticExtension(TInt /*aCmd*/, TAny* /*aArg*/)
//
//
//
	{
	return KErrNotSupported;
	}


//////////////////////////////////////////////////////////////////////////////
// DLL Exported Function
//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_EXTENSION()
//
// Creates and initializes a new DMA controller object on the kernel heap.
//
	{
	__KTRACE_OPT2(KBOOT, KDMA, Kern::Printf("Starting DMA Extension"));

	return Controller.Create();
	}
