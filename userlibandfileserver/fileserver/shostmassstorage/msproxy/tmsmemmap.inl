// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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


inline void TMsDataMemMap::Reset()
    {
    iDataOffset = static_cast<TInt64>(0);
    iSize = static_cast<TUint64>(0);
    }


inline TInt TMsDataMemMap::BlockLength() const
    {
    return KSectorSize;
    }


inline TUint64 TMsDataMemMap::DataSize() const
    {
    return iSize - iDataOffset;
    }

inline void TMsDataMemMap::InitDataArea(TUint32 aFirstDataSector, TUint32 aNumSectors)
    {
    iDataOffset = static_cast<TInt64>(aFirstDataSector) * KSectorSize;
    iSize = static_cast<TInt64>(aNumSectors) * KSectorSize;
    }


inline void TMsDataMemMap::InitDataArea(TUint64 aSize)
    {
    iSize = aSize - iDataOffset;
    }


inline TInt64 TMsDataMemMap::GetDataPos(TInt64 aPos) const
    {
    return aPos + iDataOffset;
    }

