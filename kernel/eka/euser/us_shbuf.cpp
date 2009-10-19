// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/euser/us_shbuf.cpp
// Shareable Data Buffers

#include "e32shbuf.h"
#include <u32exec.h>
#include <e32base.h>

EXPORT_C TShPoolInfo::TShPoolInfo()
	{
	memclr(this, sizeof(TShPoolInfo));
	}


EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iFlags = aFlag;
	iInfo.iAlignment = 0;
	SetSizingAttributes(aInitialBufs, 0,0,0);
	}


EXPORT_C TShPoolCreateInfo::TShPoolCreateInfo(TShPoolNonPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment)
	{
	iInfo.iBufSize = aBufSize;
	iInfo.iInitialBufs = aInitialBufs;
	iInfo.iAlignment = aAlignment;
	iInfo.iFlags = aFlag;
	SetSizingAttributes(aInitialBufs, 0,0,0);
	}


EXPORT_C TInt TShPoolCreateInfo::SetSizingAttributes(TUint aMaxBufs, TUint aGrowTriggerRatio,
					TUint aGrowByRatio, TUint aShrinkHysteresisRatio)
	{
	if (aGrowTriggerRatio == 0 || aGrowByRatio == 0)	// No automatic growing/shrinking
		{
		aGrowTriggerRatio = aGrowByRatio = 0;
		if (aMaxBufs != iInfo.iInitialBufs)
			return KErrArgument;
		}
	else
		{
		// aGrowTriggerRatio must be < 1.0 (i.e. 256)
		// aShrinkHysteresisRatio must be > 1.0
		if (aGrowTriggerRatio >= 256 || aShrinkHysteresisRatio <= 256)
			return KErrArgument;

		if ((iInfo.iFlags & EShPoolContiguous) && (iInfo.iFlags & EShPoolNonPageAlignedBuffer))
			return KErrNotSupported;
		}

	iInfo.iMaxBufs			= aMaxBufs;
	iInfo.iGrowTriggerRatio	= aGrowTriggerRatio;
	iInfo.iGrowByRatio		= aGrowByRatio;
	iInfo.iShrinkHysteresisRatio = aShrinkHysteresisRatio;

	return KErrNone;
	}


EXPORT_C TInt TShPoolCreateInfo::SetExclusive()
	{
	iInfo.iFlags |= EShPoolExclusiveAccess;

	return KErrNone;
	}


EXPORT_C TInt TShPoolCreateInfo::SetGuardPages()
	{
	iInfo.iFlags |= EShPoolGuardPages;

	return KErrNone;
	}


EXPORT_C RShPool::RShPool()
	{
	memclr(this, sizeof(RShPool));
	}


EXPORT_C TInt RShPool::Create(const TShPoolCreateInfo& aInfo, TUint aFlags)
	{
	return SetReturnedHandle(Exec::ShPoolCreate(aInfo.iInfo, aFlags));
	}


EXPORT_C void RShBuf::Close()
	{
	if ((iHandle!=KNullHandle) && ((iHandle & CObjectIx::ENoClose)==0))
		{
		iBase = 0;
		iSize = 0;

		TInt h = iHandle;
		iHandle=0;
		Exec::HandleClose(h);
		}
	}

EXPORT_C void RShPool::GetInfo(TShPoolInfo& aInfo) const
	{
	Exec::ShPoolGetInfo(iHandle, aInfo);
	}


EXPORT_C TInt RShPool::SetBufferWindow(TInt aWindowSize, TBool aAutoMap)
	{
	return Exec::ShPoolBufferWindow(iHandle, aWindowSize, aAutoMap);
	}


EXPORT_C TUint RShPool::FreeCount() const
	{
	return Exec::ShPoolFreeCount(iHandle);
	}


EXPORT_C void RShPool::RequestLowSpaceNotification(TUint aThreshold, TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	Exec::ShPoolNotification(iHandle, EShPoolLowSpace, aThreshold, aStatus);
	}


EXPORT_C void RShPool::RequestFreeSpaceNotification(TUint aThreshold, TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	Exec::ShPoolNotification(iHandle, EShPoolFreeSpace, aThreshold, aStatus);
	}


EXPORT_C void RShPool::CancelLowSpaceNotification(TRequestStatus& aStatus)
	{
	Exec::ShPoolNotificationCancel(iHandle, EShPoolLowSpace, aStatus);
	}


EXPORT_C void RShPool::CancelFreeSpaceNotification(TRequestStatus& aStatus)
	{
	Exec::ShPoolNotificationCancel(iHandle, EShPoolFreeSpace, aStatus);
	}


EXPORT_C TInt RShBuf::Alloc(RShPool& aPool, TUint aFlags)
	{
	SShBufBaseAndSize bs;

	TInt handle = Exec::ShPoolAlloc(aPool.Handle(), aFlags, bs);

	TInt r = SetReturnedHandle(handle);

	if (r == KErrNone)
		{
		iBase = bs.iBase;
		iSize = bs.iSize;
		}

	return r;
	}


EXPORT_C TInt RShBuf::Map(TBool aReadOnly)
	{
	SShBufBaseAndSize bs;
	TInt r = Exec::ShBufMap(iHandle, aReadOnly, bs);

	if (r == KErrNone)
		{
	    iBase = bs.iBase;
		iSize = bs.iSize;
		}
	return r;
	}


EXPORT_C TInt RShBuf::UnMap()
	{
	return Exec::ShBufUnMap(iHandle);
	}


EXPORT_C TUint8* RShBuf::Ptr()
	{
	// if iBase is null RShBuf has not been intialised
	if (iBase == 0)
		{
		SShBufBaseAndSize bs;
		Exec::ShBufBaseAndSize(iHandle, bs);
		iBase = bs.iBase;
		iSize = bs.iSize;
		}
	return reinterpret_cast<TUint8*>(iBase);
	}


EXPORT_C TUint RShBuf::Size()
	{
	// if iBase is null RShBuf has not been intialised
	if (iBase == 0)
		{
		SShBufBaseAndSize bs;
		Exec::ShBufBaseAndSize(iHandle, bs);
		iBase = bs.iBase;
		iSize = bs.iSize;
		}
	return iSize;
	}
