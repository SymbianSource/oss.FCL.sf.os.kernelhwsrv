// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//
//  Collection of common constants, utility functions, etc. for the file server and file systems.
//  Definitions here must be filesystem-agnostic, i.e. generic enougs to be used by every file system
//
//  This is the internal file and must not be exported.

/**
    @file
    @internalTechnology
*/

#include "filesystem_utils.h"

//-----------------------------------------------------------------------------

/**
    Calculates the log2 of a number

    @param aNum Number to calulate the log two of
    @return The log two of the number passed in
*/
TUint32 Log2(TUint32 aVal)
    {
    return Log2_inline(aVal);
    }

//-----------------------------------------------------------------------------
/**
    Calculates number of '1' bits in the aVal

    @param aVal some value
    @return number of '1' bits in the aVal
*/
TUint32 Count1Bits(TUint32 aVal)
    {
    return Count1Bits_inline(aVal);
    }

//-----------------------------------------------------------------------------
/**
    Removes trailing dots from aName.
    @return new string descriptor that may have its length adjusted
*/
TPtrC RemoveTrailingDots(const TDesC& aName)
    {
    TInt len = aName.Length();
    
    while(len > 0)
        {
        if(aName[len-1] == '.')
            len--;
        else
            break;
        }

    TPtrC ptrNoDots(aName.Ptr(), len);
    return ptrNoDots;
    }




















