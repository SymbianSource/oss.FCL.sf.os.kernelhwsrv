/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
//
// tmsmemmap.cpp
//
// Maps a position to mass storage address space
//

/** @file
@internalTechnology
*/

#include <e32def.h>
#include <e32err.h>

#include "tmsmemmap.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmsmemmapTraces.h"
#endif


TMsDataMemMap::TMsDataMemMap()
    {
    Reset();
    }


/**
    Checks that the block is within the limits of the media memory address space
    and truncates the length if block extends beyond media size.

   @param aPos [IN] Position of start address [OUT] Adjusted position to real
   address on media.
   @param aLength [IN] Number of bytes [OUT] Number of bytes truncated in case
   of block overflow.

   @return TInt KErrNone if block fits. KErrArgument if start position is
   greater than the media size. KErrEof if block extends beyond media size.
 */
TInt TMsDataMemMap::TranslateDataPos(TInt64& aPos, TInt& aLength) const
    {
    // Map to the actual position on the media
    aPos += iDataOffset;

    if (aPos > iSize)
        {
        return KErrArgument;
        }

    TInt64 endPos = aPos + aLength;
    if (endPos > iSize)
        {
        aLength = iSize - aPos;
        return KErrEof;
        }
    return KErrNone;
    }

/**
   Checks that the block is within the limits of the media memory address space

   @param aPos [IN] Position of start address [OUT] Adjusted position to real
   address on media.
   @param aLength Number of bytes.

   @return TInt KErrNone if block fits. KErrArgument if start position is
   greater than the media size. KErrEof if block extends beyond media size.
 */
TInt TMsDataMemMap::CheckBlockInRange(TInt64& aPos, TInt aLength) const
    {
    // Map to the actual position on the media
    aPos += iDataOffset;

    if (aPos > iSize)
        {
        return KErrArgument;
        }

    TInt64 endPos = aPos + aLength;
    if (endPos > iSize)
        {
        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_PROXY, TMSMEMMAP_1,
                     "EOF found Pos=0x%x %x Len=0x%x", I64HIGH(aPos), I64LOW(aPos), aLength);
        return KErrEof;
        }

    return KErrNone;
    }
