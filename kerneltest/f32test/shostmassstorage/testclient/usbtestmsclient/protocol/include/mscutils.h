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
//



/**
 @file
 @internalTechnology
*/

#ifndef MSCUTILS_H
#define MSCUTILS_H

/**
Inserts and extracts integers in big-endian format.
*/
class BigEndian
    {
public:
    static TUint32 Get32(const TUint8* aPtr);
    static TUint16 Get16(const TUint8* aPtr);
    static void Put32(TUint8* aPtr, TUint32 aVal);
    static void Put16(TUint8* aPtr, TUint16 aVal);
    };

/**
Inserts and extracts integers in little-endian format.
*/
class LittleEndian
    {
public:
    static TUint32 Get32(const TUint8* aPtr);
    static TUint16 Get16(const TUint8* aPtr);
    static void Put32(TUint8* aPtr, TUint32 aVal);
    static void Put16(TUint8* aPtr, TUint16 aVal);
    };

#include "mscutils.inl"

#endif // MSCUTILS_H
