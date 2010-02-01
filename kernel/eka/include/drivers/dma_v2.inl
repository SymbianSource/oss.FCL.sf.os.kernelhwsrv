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
// include/drivers/dma_v2.inl
// DMA Framework - Client API v2 definition.
//
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.


//
// TDmaChannel
//

inline void TDmaChannel::Wait()
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iMutex);
	}

inline void TDmaChannel::Signal()
	{
	Kern::MutexSignal(*iMutex);
	NKern::ThreadLeaveCS();
	}

inline void TDmaChannel::Flash()
	{
	Kern::MutexSignal(*iMutex);
	Kern::MutexWait(*iMutex);
	}

inline TBool TDmaChannel::IsOpened() const
	{
	__KTRACE_OPT(KDMA, Kern::Printf("Warning: TDmaChannel::IsOpened() is deprecated"));
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


// ---
