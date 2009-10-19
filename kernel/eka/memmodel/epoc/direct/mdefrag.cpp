// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\direct\mdefrag.cpp
// 
//

#include <memmodel.h>
#include "platform.h"

// None of this is supported on the direct memory model

EXPORT_C TInt Epoc::MovePhysicalPage(TPhysAddr /*aOld*/, TPhysAddr& /*aNew*/, TRamDefragPageToMove /*aPageToMove*/)
	{
	return KErrNotSupported;
	}

TInt M::RamDefragFault(TAny* /*aExceptionInfo*/)
	{
	return KErrAbort;
	}

EXPORT_C TRamDefragRequest::TRamDefragRequest()
	: TAsyncRequest(NULL, NULL, 0)
	{
	}

EXPORT_C TInt TRamDefragRequest::DefragRam(TInt aPriority, TInt aMaxPages)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::DefragRam(NFastSemaphore* aSem, TInt aPriority, TInt aMaxPages)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::DefragRam(TDfc* aDfc, TInt aPriority, TInt aMaxPages)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, NFastSemaphore* aSem, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::EmptyRamZone(TUint aId, TDfc* aDfc, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, NFastSemaphore* aSem, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TDfc* aDfc, TInt aPriority)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt TRamDefragRequest::Result()
	{
	return KErrNotSupported;
	}

EXPORT_C void TRamDefragRequest::Cancel()
	{
	}
