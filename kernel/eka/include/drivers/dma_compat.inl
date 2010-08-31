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
// include/drivers/dma_compat.inl
// DMA Framework - Client API definition.
//
// Inline implementations of functions which (originally) exposed DMA HAI
// details that are now, after the header file split into dma_v2.h and
// dma_hai.h, no longer meant to be visible to DMA clients. This file is only
// included for DMA_APIV2 clients.
//
// This file is not meant to be a permanent one, and may eventually be removed
// together with the deprecated functions it implements.
//
//


inline const TDmac* TDmaChannel::Controller() const
	{
	return iController;
	}

static inline TUint32 RequestFlags2SrcConfigFlags(TUint aFlags)
	{
	TUint32 flags = (aFlags & KDmaMemSrc) ? KDmaMemAddr : 0;
	flags |= (aFlags & KDmaPhysAddrSrc) ? KDmaPhysAddr : 0;
	if ((flags & KDmaMemAddr) && (flags & KDmaPhysAddr))
		flags |= KDmaMemIsContiguous;
	return flags;
	}

static inline TUint32 RequestFlags2DstConfigFlags(TUint aFlags)
	{
	TUint32 flags = (aFlags & KDmaMemDest) ? KDmaMemAddr : 0;
	flags |= (aFlags & KDmaPhysAddrDest) ? KDmaPhysAddr : 0;
	if ((flags & KDmaMemAddr) && (flags & KDmaPhysAddr))
		flags |= KDmaMemIsContiguous;
	return flags;
	}

inline TInt TDmaChannel::MaxTransferSize(TUint aFlags, TUint32 aPslInfo)
	{
	TUint src_flags = RequestFlags2SrcConfigFlags(aFlags);
	TUint dst_flags = RequestFlags2DstConfigFlags(aFlags);
	return MaxTransferLength(src_flags, dst_flags, aPslInfo);
	}

inline TUint TDmaChannel::MemAlignMask(TUint aFlags, TUint32 aPslInfo)
	{
	TUint src_flags = RequestFlags2SrcConfigFlags(aFlags);
	TUint dst_flags = RequestFlags2DstConfigFlags(aFlags);
	return AddressAlignMask(src_flags, dst_flags, aPslInfo);
	}


// ---
