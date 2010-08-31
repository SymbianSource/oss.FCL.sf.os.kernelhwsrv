// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// os/kernelhwsrv/kerneltest/e32test/dmav2/dma2_sim.cpp
// Partial simulation of DMA2 PSL
//
//


#include <kernel/kern_priv.h>

#include <drivers/dma.h>
#include <drivers/dma_hai.h>

#include "d_dma2.h"

// Debug support
static const char KDmaPanicCat[] = "DMA PSL - " __FILE__;

static const TInt KMaxTransferLen = 0x1000;	// max transfer length for this DMAC
static const TInt KMemAlignMask = 0; // memory addresses passed to DMAC must be multiple of 8
static const TInt KDesCount = 160;				// Initial DMA descriptor count

#define TEST_RETURN(X) if (!(X))\
	{\
	__KTRACE_OPT(KPANIC, Kern::Printf("Simulated Dma test failure: " __FILE__ " line %d", __LINE__));\
	return KErrAbort;\
	}

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

/**
TO DO: Fill in to provide information to the V2 test harness (t_dma2.exe)
*/
TDmaV2TestInfo TestInfov2 =
	{
	0,
	0,
	0,
	0,
	{0},
	0,
	{0},
	1,
	{0}
	};

EXPORT_C const TDmaV2TestInfo& DmaTestInfoV2()
	{
	return TestInfov2;
	}


//////////////////////////////////////////////////////////////////////////////
// Simulated channel
//////////////////////////////////////////////////////////////////////////////

/**
An interface class to add simulation specific functionallity to any DMA channel
*/
class MSimChannel
	{
public:
	virtual TInt PreOpen() =0;
	virtual TDmaChannel& Channel() =0;
	};

//////////////////////////////////////////////////////////////////////////////
// Derived Channel (Scatter/Gather)
//////////////////////////////////////////////////////////////////////////////

const SDmacCaps KSimSgChanCaps =
	{0,										// TInt iChannelPriorities;
	 EFalse,								// TBool iChannelPauseAndResume;
	 EFalse,								// TBool iAddrAlignedToElementSize;
	 EFalse,								// TBool i1DIndexAddressing;
	 EFalse,								// TBool i2DIndexAddressing;
	 KDmaSyncAuto,						   // TUint iSynchronizationTypes;
	 KDmaBurstSizeAny,					   // TUint iBurstTransactions;
	 EFalse,							   // TBool iDescriptorInterrupt;
	 EFalse,							   // TBool iFrameInterrupt;
	 EFalse,							   // TBool iLinkedListPausedInterrupt;
	 EFalse,							   // TBool iEndiannessConversion;
	 KDmaGraphicsOpNone,				   // TUint iGraphicsOps;
	 EFalse,							   // TBool iRepeatingTransfers;
	 EFalse,							   // TBool iChannelLinking;
	 ETrue,								   // TBool iHwDescriptors;
	 EFalse,							   // TBool iSrcDstAsymmetry;
	 EFalse,							   // TBool iAsymHwDescriptors;
	 EFalse,							   // TBool iBalancedAsymSegments;
	 EFalse,							   // TBool iAsymCompletionInterrupt;
	 EFalse,							   // TBool iAsymDescriptorInterrupt;
	 EFalse,							   // TBool iAsymFrameInterrupt;
	 {0, 0, 0, 0, 0}					   // TUint32 iReserved[5];
	};

const SDmacCaps KSimSwChanCaps =
	{0,										// TInt iChannelPriorities;
	 EFalse,								// TBool iChannelPauseAndResume;
	 EFalse,								// TBool iAddrAlignedToElementSize;
	 EFalse,								// TBool i1DIndexAddressing;
	 EFalse,								// TBool i2DIndexAddressing;
	 KDmaSyncAuto,						   // TUint iSynchronizationTypes;
	 KDmaBurstSizeAny,					   // TUint iBurstTransactions;
	 EFalse,							   // TBool iDescriptorInterrupt;
	 EFalse,							   // TBool iFrameInterrupt;
	 EFalse,							   // TBool iLinkedListPausedInterrupt;
	 EFalse,							   // TBool iEndiannessConversion;
	 KDmaGraphicsOpNone,				   // TUint iGraphicsOps;
	 EFalse,							   // TBool iRepeatingTransfers;
	 EFalse,							   // TBool iChannelLinking;
	 EFalse,							   // TBool iHwDescriptors;
	 EFalse,							   // TBool iSrcDstAsymmetry;
	 EFalse,							   // TBool iAsymHwDescriptors;
	 EFalse,							   // TBool iBalancedAsymSegments;
	 EFalse,							   // TBool iAsymCompletionInterrupt;
	 EFalse,							   // TBool iAsymDescriptorInterrupt;
	 EFalse,							   // TBool iAsymFrameInterrupt;
	 {0, 0, 0, 0, 0}					   // TUint32 iReserved[5];
	};

class TEmptyChannel : public TDmaChannel, public MSimChannel
	{
public:
	// Virtual from TDmaChannel
	void DoCancelAll();

	void CallDefaultVirtuals();
	TInt CheckExtensionStubs();

	// From MSimChannel
	TInt PreOpen();
	TDmaChannel& Channel() {return *this;}
	};

void TEmptyChannel::DoCancelAll()
	{
	__DMA_CANT_HAPPEN();
	}

void TEmptyChannel::CallDefaultVirtuals()
	{
	DMA_PSL_TRACE("Calling default virtual TDmaChannel functions");

	const DDmaRequest* req = NULL;
	SDmaDesHdr* hdr = NULL;

	DoQueue(*req);
	DoDfc(*req, hdr);
	DoDfc(*req, hdr, hdr);

	QueuedRequestCountChanged();
	}

TInt TEmptyChannel::CheckExtensionStubs()
	{
	DMA_PSL_TRACE("Calling extension stubs");

	TInt r = Extension(0, NULL);
	TEST_RETURN(r == KErrNotSupported)

	r = StaticExtension(0, NULL);
	TEST_RETURN(r == KErrNotSupported)

	return KErrNone;
	}

TInt TEmptyChannel::PreOpen()
	{
	CallDefaultVirtuals();
	return CheckExtensionStubs();
	}

//////////////////////////////////////////////////////////////////////////////
// Derived SkelControllerSw Class
//////////////////////////////////////////////////////////////////////////////

class TSkelDmac : public TDmac
	{
public:
	TSkelDmac(const SCreateInfo& aInfo);
	TInt Create(const SCreateInfo& aInfo);
private:
	// from TDmac (PIL pure virtual)
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TUint MaxTransferLength(TDmaChannel& aChannel, TUint aSrcFlags,
									TUint aDstFlags, TUint32 aPslInfo);
	virtual TUint AddressAlignMask(TDmaChannel& aChannel, TUint aSrcFlags,
								   TUint aDstFlags, TUint32 aPslInfo);

	inline TDmaDesc* HdrToHwDes(const SDmaDesHdr& aHdr);

	void CallDefaultVirtuals();
	TInt TestPool();

public:
	static const SCreateInfo KDmacInfoHw;
	static const SCreateInfo KDmacInfoSw;

	TEmptyChannel iChannel;
	};


const TDmac::SCreateInfo TSkelDmac::KDmacInfoHw =
	{
	ETrue,													// iCapsHwDes
	KDesCount,												// iDesCount
	sizeof(TDmaDesc),										// iDesSize
#ifndef __WINS__
	EMapAttrSupRw | EMapAttrFullyBlocking					// iDesChunkAttribs
#endif
	};

const TDmac::SCreateInfo TSkelDmac::KDmacInfoSw =
	{
	EFalse,													// iCapsHwDes
	KDesCount,												// iDesCount
	sizeof(TDmaTransferArgs),										// iDesSize
#ifndef __WINS__
	EMapAttrSupRw | EMapAttrFullyBlocking					// iDesChunkAttribs
#endif
	};

static TSkelDmac SkelControllerSw(TSkelDmac::KDmacInfoSw);
static TSkelDmac SkelControllerHw(TSkelDmac::KDmacInfoHw);


TSkelDmac::TSkelDmac(const SCreateInfo& aInfo)
//
// Constructor.
//
	: TDmac(aInfo)
	{
	TInt r = Create(aInfo);
	__NK_ASSERT_ALWAYS(r == KErrNone);

	CallDefaultVirtuals();
	r = TestPool();
	__NK_ASSERT_ALWAYS(r == KErrNone);
	}


TInt TSkelDmac::Create(const SCreateInfo& aInfo)
//
// Second phase construction.
//
	{
	TInt r = TDmac::Create(aInfo);							// Base class Create()
	if (r == KErrNone)
		{
		__DMA_ASSERTA(ReserveSetOfDes(1) == KErrNone);
		}
	return r;
	}


void TSkelDmac::StopTransfer(const TDmaChannel& aChannel)
//
// Stops a running channel.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());

	__KTRACE_OPT(KDMA, Kern::Printf(">TSkelDmac::StopTransfer channel=%d (unsupported)", i));

	(void) i;

	}


TBool TSkelDmac::IsIdle(const TDmaChannel& aChannel)
//
// Returns the state of a given channel.
//
	{
	const TUint8 i = static_cast<TUint8>(aChannel.PslId());

	__KTRACE_OPT(KDMA, Kern::Printf(">TSkelDmac::IsIdle channel=%d (unsupported)", i));

	// TO DO (for instance): Return the state of the RUN bit of the channel.
	// The return value should reflect the actual state.
	(void) i;

	return ETrue;
	}


TUint TSkelDmac::MaxTransferLength(TDmaChannel& /*aChannel*/, TUint /*aSrcFlags*/,
									   TUint /*aDstFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the maximum transfer length in bytes for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMaxTransferLen;
	}


TUint TSkelDmac::AddressAlignMask(TDmaChannel& /*aChannel*/, TUint /*aSrcFlags*/,
									  TUint /*aDstFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the memory buffer alignment restrictions mask for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMemAlignMask;
	}


inline TDmaDesc* TSkelDmac::HdrToHwDes(const SDmaDesHdr& aHdr)
//
// Changes return type of base class call.
//
	{
	return static_cast<TDmaDesc*>(TDmac::HdrToHwDes(aHdr));
	}

/**
Call the default virtual functions on the TDmac,
that would never otherwise be called

*/
void TSkelDmac::CallDefaultVirtuals()
	{
	DMA_PSL_TRACE("Calling default virtual TDmac functions");

	TDmaChannel* channel = NULL;
	SDmaDesHdr* hdr = NULL;

	Transfer(*channel, *hdr);
	Transfer(*channel, *hdr, *hdr);

	const TDmaTransferArgs args;
	TInt r = KErrNone;

	r = InitHwDes(*hdr, args);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	r = InitSrcHwDes(*hdr, args);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	r = InitDstHwDes(*hdr, args);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	r = UpdateHwDes(*hdr, KPhysAddrInvalid, KPhysAddrInvalid, 0, 0);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	r = UpdateSrcHwDes(*hdr, KPhysAddrInvalid, 0, 0);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	r = UpdateDstHwDes(*hdr, KPhysAddrInvalid, 0, 0);
	__NK_ASSERT_ALWAYS(r == KErrGeneral);

	ChainHwDes(*hdr, *hdr);
	AppendHwDes(*channel, *hdr, *hdr);
	AppendHwDes(*channel, *hdr, *hdr, *hdr, *hdr);
	UnlinkHwDes(*channel, *hdr);

	TUint32 count = 0;

	count = HwDesNumDstElementsTransferred(*hdr);
	__NK_ASSERT_ALWAYS(count == 0);

	count = HwDesNumSrcElementsTransferred(*hdr);
	__NK_ASSERT_ALWAYS(count == 0);
	}

TInt TSkelDmac::TestPool()
	{
	DMA_PSL_TRACE("TSkelDmac::TestPool()");
	TInt count = 0;
	SDmaDesHdr* hdr = iFreeHdr;
	TAny* des = iDesPool;

	TInt r = KErrNone;
	while(hdr->iNext)
		{
		TAny* receivedDes = NULL;
		if(iCapsHwDes)
			{
			receivedDes = HdrToHwDes(*hdr);
			}
		else
			{
			TDmaTransferArgs& args = HdrToDes(*hdr);
			receivedDes = &args;
			}

		if(receivedDes != des)
			{
			DMA_PSL_TRACE1("TSkelDmac::TestPool() failure: count=%d", count);
			r = KErrGeneral;
			break;
			}

		hdr = hdr->iNext;
		des = (TAny*)((TUint)des + iDesSize);
		count++;
		}

	if(count != (KDesCount - 1))
		{
		DMA_PSL_TRACE2("TSkelDmac::TestPool() failure: count = %d != (iMaxDesCount -1) = %d", count, KDesCount-1);
		r = KErrUnknown;
		}
	return r;
	}

//////////////////////////////////////////////////////////////////////////////
// Simulated Fragmentation Dmac
//////////////////////////////////////////////////////////////////////////////


const SDmacCaps KSimAsymmChanCaps =
	{0,										// TInt iChannelPriorities;
	 EFalse,								// TBool iChannelPauseAndResume;
	 EFalse,								// TBool iAddrAlignedToElementSize;
	 EFalse,								// TBool i1DIndexAddressing;
	 EFalse,								// TBool i2DIndexAddressing;
	 KDmaSyncAuto,						   // TUint iSynchronizationTypes;
	 KDmaBurstSizeAny,					   // TUint iBurstTransactions;
	 EFalse,							   // TBool iDescriptorInterrupt;
	 EFalse,							   // TBool iFrameInterrupt;
	 EFalse,							   // TBool iLinkedListPausedInterrupt;
	 EFalse,							   // TBool iEndiannessConversion;
	 KDmaGraphicsOpNone,				   // TUint iGraphicsOps;
	 EFalse,							   // TBool iRepeatingTransfers;
	 EFalse,							   // TBool iChannelLinking;
	 ETrue,								   // TBool iHwDescriptors;
	 EFalse,							   // TBool iSrcDstAsymmetry;
	 ETrue,								   // TBool iAsymHwDescriptors;
	 EFalse,							   // TBool iBalancedAsymSegments;
	 EFalse,							   // TBool iAsymCompletionInterrupt;
	 EFalse,							   // TBool iAsymDescriptorInterrupt;
	 EFalse,							   // TBool iAsymFrameInterrupt;
	 {0, 0, 0, 0, 0}					   // TUint32 iReserved[5];
	};

const SDmacCaps KSimAsymmBalancedChanCaps =
	{0,										// TInt iChannelPriorities;
	 EFalse,								// TBool iChannelPauseAndResume;
	 EFalse,								// TBool iAddrAlignedToElementSize;
	 EFalse,								// TBool i1DIndexAddressing;
	 EFalse,								// TBool i2DIndexAddressing;
	 KDmaSyncAuto,						   // TUint iSynchronizationTypes;
	 KDmaBurstSizeAny,					   // TUint iBurstTransactions;
	 EFalse,							   // TBool iDescriptorInterrupt;
	 EFalse,							   // TBool iFrameInterrupt;
	 EFalse,							   // TBool iLinkedListPausedInterrupt;
	 EFalse,							   // TBool iEndiannessConversion;
	 KDmaGraphicsOpNone,				   // TUint iGraphicsOps;
	 EFalse,							   // TBool iRepeatingTransfers;
	 EFalse,							   // TBool iChannelLinking;
	 ETrue,								   // TBool iHwDescriptors;
	 EFalse,							   // TBool iSrcDstAsymmetry;
	 ETrue,								   // TBool iAsymHwDescriptors;
	 ETrue,								   // TBool iBalancedAsymSegments;
	 EFalse,							   // TBool iAsymCompletionInterrupt;
	 EFalse,							   // TBool iAsymDescriptorInterrupt;
	 EFalse,							   // TBool iAsymFrameInterrupt;
	 {0, 0, 0, 0, 0}					   // TUint32 iReserved[5];
	};


class TAsymmDmac : public TDmac
	{
	struct THwDes
		{
		TUint iAddr;
		TUint iLength;
		TUint iCookie;
		};
public:
	TAsymmDmac();
	TInt Create();
private:
	// Work around for compiler which forbids this
	// class from accessing the protected, nested TDmac::SCreateInfo
	using TDmac::SCreateInfo;

	// from TDmac (PIL pure virtual)
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TUint MaxTransferLength(TDmaChannel& aChannel, TUint aSrcFlags,
									TUint aDstFlags, TUint32 aPslInfo);
	virtual TUint AddressAlignMask(TDmaChannel& aChannel, TUint aSrcFlags,
								   TUint aDstFlags, TUint32 aPslInfo);
	// from TDmac (PIL virtual)
	TInt InitSrcHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/);
	TInt InitDstHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/);

	void ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr);
	void UnlinkHwDes(const TDmaChannel& aChannel, SDmaDesHdr& aHdr);

	inline THwDes* HdrToHwDes(const SDmaDesHdr& aHdr);

private:
	static const SCreateInfo KInfo;
public:
	static const TInt iChannelCount;
	TEmptyChannel iChannel;
	};

const TAsymmDmac::SCreateInfo TAsymmDmac::KInfo =
	{
	ETrue,													// iCapsHwDes
	KDesCount,												// iDesCount
	sizeof(THwDes),											// iDesSize
#ifndef __WINS__
	EMapAttrSupRw | EMapAttrFullyBlocking					// iDesChunkAttribs
#endif
	};

const TInt TAsymmDmac::iChannelCount = 1;

static TAsymmDmac AsymController;

TAsymmDmac::TAsymmDmac()
//
// Constructor.
//
	: TDmac(KInfo)
	{
	TInt r = Create();
	__NK_ASSERT_ALWAYS(r == KErrNone);
	}


TInt TAsymmDmac::Create()
//
// Second phase construction.
//
	{
	TInt r = TDmac::Create(KInfo);							// Base class Create()
	if (r == KErrNone)
		{
		__DMA_ASSERTA(ReserveSetOfDes(iChannelCount) == KErrNone);
		}
	return r;
	}


void TAsymmDmac::StopTransfer(const TDmaChannel& /*aChannel*/)
//
// Stops a running channel.
//
	{
	__DMA_CANT_HAPPEN();
	}


TBool TAsymmDmac::IsIdle(const TDmaChannel& /*aChannel*/)
//
// Returns the state of a given channel.
//
	{
	__DMA_CANT_HAPPEN();
	return ETrue;
	}


TUint TAsymmDmac::MaxTransferLength(TDmaChannel& /*aChannel*/, TUint /*aSrcFlags*/,
									   TUint /*aDstFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the maximum transfer length in bytes for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMaxTransferLen;
	}


TUint TAsymmDmac::AddressAlignMask(TDmaChannel& /*aChannel*/, TUint /*aSrcFlags*/,
									  TUint /*aDstFlags*/, TUint32 /*aPslInfo*/)
//
// Returns the memory buffer alignment restrictions mask for a given transfer.
//
	{
	// TO DO: Determine the proper return value, based on the arguments.

	// For instance:
	return KMemAlignMask;
	}


inline TAsymmDmac::THwDes* TAsymmDmac::HdrToHwDes(const SDmaDesHdr& aHdr)
//
// Changes return type of base class call.
//
	{
	return static_cast<THwDes*>(TDmac::HdrToHwDes(aHdr));
	}

TInt TAsymmDmac::InitSrcHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/)
	{
	return KErrNone;
	}

TInt TAsymmDmac::InitDstHwDes(const SDmaDesHdr& /*aHdr*/, const TDmaTransferArgs& /*aTransferArgs*/)
	{
	return KErrNone;
	}

void TAsymmDmac::ChainHwDes(const SDmaDesHdr& /*aHdr*/, const SDmaDesHdr& /*aNextHdr*/)
	{
	}

void TAsymmDmac::UnlinkHwDes(const TDmaChannel& /*aChannel*/, SDmaDesHdr& /*aHdr*/)
	{
	}



//////////////////////////////////////////////////////////////////////////////
// Channel Opening/Closing (Channel Allocator)
//////////////////////////////////////////////////////////////////////////////

struct TChanEntry
	{
	TChanEntry(TDmac& aController, MSimChannel& aChannel, const SDmacCaps& aCaps)
		:
			iController(aController),
			iSimChannel(aChannel),
			iCaps(aCaps)
	{}

	TDmac& iController;
	MSimChannel& iSimChannel;
	const SDmacCaps& iCaps;
	};

const TChanEntry ChannelTable[] =
	{
		TChanEntry(SkelControllerSw, SkelControllerSw.iChannel, KSimSwChanCaps),
		TChanEntry(SkelControllerHw, SkelControllerHw.iChannel, KSimSgChanCaps),
		TChanEntry(AsymController, AsymController.iChannel, KSimAsymmChanCaps),
		TChanEntry(AsymController, AsymController.iChannel, KSimAsymmBalancedChanCaps)
	};

static const TInt KChannelCount = ARRAY_LENGTH(ChannelTable);

TDmaChannel* DmaChannelMgr::Open(TUint32 aOpenId, TBool /*aDynChannel*/, TUint /*aPriority*/)
//
//
//
	{
	__KTRACE_OPT(KDMA, Kern::Printf(">DmaChannelMgr::Open aOpenId=%d", aOpenId));

	__DMA_ASSERTA(aOpenId < static_cast<TUint32>(KChannelCount));

	const TChanEntry& entry = ChannelTable[aOpenId];
	TDmaChannel* pC = &entry.iSimChannel.Channel();
	if (pC->IsOpened())
		{
		pC = NULL;
		}
	else
		{
		pC->iController = &entry.iController;
		pC->iPslId = aOpenId;
		pC->iDmacCaps = &entry.iCaps;

		// It is safe to signal here,
		// setting iController marks the channel
		// as taken
		Signal();

		TInt r = entry.iSimChannel.PreOpen();

		Wait();

		// If there was an error
		// Close channel after retaking mutex
		if(r != KErrNone)
			{
			pC->iController = NULL;
			pC = NULL;
			}


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
