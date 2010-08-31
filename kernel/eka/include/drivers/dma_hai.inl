// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// include/drivers/dma_hai.inl
// DMA Framework - Symbian Hardware Abstraction Interface (SHAI).
//
//


// TDmac

inline void TDmac::Wait()
	{
	NKern::FMWait(&iLock);
	}

inline void TDmac::Signal()
	{
	NKern::FMSignal(&iLock);
	}

inline TDmaTransferArgs& TDmac::HdrToDes(const SDmaDesHdr& aHdr) const
	{
	return static_cast<TDmaTransferArgs*>(iDesPool)[&aHdr - iHdrPool];
	}

inline TAny* TDmac::HdrToHwDes(const SDmaDesHdr& aHdr) const
	{
	return static_cast<TUint8*>(iDesPool) + iDesSize * (&aHdr - iHdrPool);
	}

inline TUint32 TDmac::HwDesLinToPhys(TAny* aDes) const
	{
#ifdef __WINS__
	(void)aDes;
	return 0xDEADBEEF;
#else
	return iHwDesChunk->iPhysAddr +
		(reinterpret_cast<TLinAddr>(aDes) - iHwDesChunk->iLinAddr);
#endif
	}


// ---
