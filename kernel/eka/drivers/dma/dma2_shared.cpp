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
// e32/drivers/dma2_shared.cpp
// DMA Platform Independent Layer (PIL)
//
//

#include <drivers/dmadefs.h>


//
// Exported default constructor.
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferConfig::TDmaTransferConfig()
	: iAddr(0),
	  iAddrMode(KDmaAddrModeConstant),
	  iElementSize(0),
	  iElementsPerFrame(0),
	  iElementsPerPacket(0),
	  iFramesPerTransfer(0),
	  iElementSkip(0),
	  iFrameSkip(0),
	  iBurstSize(KDmaBurstSizeAny),
	  iFlags(0),
	  iSyncFlags(KDmaSyncAuto),
	  iPslTargetInfo(0),
	  iRepeatCount(0),
	  iDelta(~0u),
	  iReserved(0)
	{
	}


//
// General use version.
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferConfig::TDmaTransferConfig(
	TUint32 aAddr,
	TUint aTransferFlags,
	TDmaAddrMode aAddrMode,
	TUint aSyncFlags,
	TDmaBurstSize aBurstSize,
	TUint aElementSize,
	TUint aElementsPerPacket,
	TUint aPslTargetInfo,
	TInt aRepeatCount
	)
	:
	iAddr(aAddr),
	iAddrMode(aAddrMode),
	iElementSize(aElementSize),
	iElementsPerFrame(0),
	iElementsPerPacket(aElementsPerPacket),
	iFramesPerTransfer(0),
	iElementSkip(0),
	iFrameSkip(0),
	iBurstSize(aBurstSize),
	iFlags(aTransferFlags),
	iSyncFlags(aSyncFlags),
	iPslTargetInfo(aPslTargetInfo),
	iRepeatCount(aRepeatCount),
	iDelta(~0u),
	iReserved(0)
	{
	}


//
// 1D/2D version.
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferConfig::TDmaTransferConfig(
	TUint32 aAddr,
	TUint aElementSize,
	TUint aElementsPerFrame,
	TUint aFramesPerTransfer,
	TInt aElementSkip,
	TInt aFrameSkip,
	TUint aTransferFlags,
	TUint aSyncFlags,
	TDmaBurstSize aBurstSize,
	TUint aElementsPerPacket,
	TUint aPslTargetInfo,
	TInt aRepeatCount
	)
	:
	iAddr(aAddr),
	iAddrMode( // deduce transfer mode from skips
		(aFrameSkip != 0) ? KDmaAddrMode2DIndex :
		(aElementSkip != 0)? KDmaAddrMode1DIndex :
		KDmaAddrModePostIncrement),
	iElementSize(aElementSize),
	iElementsPerFrame(aElementsPerFrame),
	iElementsPerPacket(aElementsPerPacket),
	iFramesPerTransfer(aFramesPerTransfer),
	iElementSkip(aElementSkip),
	iFrameSkip(aFrameSkip),
	iBurstSize(aBurstSize),
	iFlags(aTransferFlags),
	iSyncFlags(aSyncFlags),
	iPslTargetInfo(aPslTargetInfo),
	iRepeatCount(aRepeatCount),
	iDelta(~0u),
	iReserved(0)
	{
	}


//
// Exported default constructor.
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferArgs::TDmaTransferArgs()
	: iTransferCount(0),
	  iGraphicsOps(KDmaGraphicsOpNone),
	  iFlags(0),
	  iChannelPriority(KDmaPriorityNone),
	  iPslRequestInfo(0),
	  iChannelCookie(0),
	  iDelta(~0u),
	  iReserved1(0),
	  iReserved2(0)
	{
	}


//
// .
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferArgs::TDmaTransferArgs (
	TUint aSrcAddr,
	TUint aDstAddr,
	TUint aCount,
	TUint aDmaTransferFlags,
	TUint aDmaSyncFlags,
	TUint aDmaPILFlags,
	TDmaAddrMode aMode,
	TUint aElementSize,
	TUint aChannelPriority,
	TDmaBurstSize aBurstSize,
	TUint aPslRequestInfo,
	TDmaGraphicsOps aGraphicOp,
	TUint32 aColour
	)
	:
	iSrcConfig(aSrcAddr, aDmaTransferFlags, aMode, aDmaSyncFlags, aBurstSize, aElementSize),
	iDstConfig(aDstAddr, aDmaTransferFlags, aMode, aDmaSyncFlags, aBurstSize, aElementSize),
	iTransferCount(aCount),
	iGraphicsOps(aGraphicOp),
	iColour(aColour),
	iFlags(aDmaPILFlags),
	iChannelPriority(aChannelPriority),
	iPslRequestInfo(aPslRequestInfo)
	{
	}


//
// .
//
#ifdef DMA_APIV2
KEXPORT_C
#endif
TDmaTransferArgs::TDmaTransferArgs(
	const TDmaTransferConfig& aSrc,
	const TDmaTransferConfig& aDst,
	TUint32 aFlags,
	TUint aChannelPriority,
	TUint aPslRequestInfo,
	TDmaGraphicsOps aGraphicOp,
	TUint32 aColour
	)
	: iSrcConfig(aSrc),
	  iDstConfig(aDst),
	  iTransferCount(0),
	  iGraphicsOps(aGraphicOp),
	  iColour(aColour),
	  iFlags(aFlags),
	  iChannelPriority(aChannelPriority),
	  iPslRequestInfo(aPslRequestInfo),
	  iChannelCookie(0),
	  iDelta(~0u),
	  iReserved1(0),
	  iReserved2(0)
	{
	}
