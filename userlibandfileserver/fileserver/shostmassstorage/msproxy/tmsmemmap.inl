// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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


inline TUint32 TMsDataMemMap::BlockLength() const
    {
    return iSectorSize;
    }


inline TUint64 TMsDataMemMap::DataSize() const
    {
    return iSize - iDataOffset;
    }

inline void TMsDataMemMap::InitDataArea(TUint32 aFirstDataSector, TUint32 aNumSectors, TUint32 aSectorSize)
	{
	iSectorSize = aSectorSize;

	iFormatSectorShift = 0;

    while(aSectorSize >>= 1)
        {
        ++iFormatSectorShift;
        }

	iDataOffset = static_cast<TInt64>(aFirstDataSector) * iSectorSize;
	iSize = static_cast<TInt64>(aNumSectors) * iSectorSize;
	}


inline void TMsDataMemMap::InitDataArea(TUint64 aSize)
    {
    iSize = aSize - iDataOffset;
    }


inline TInt64 TMsDataMemMap::GetDataPos(TInt64 aPos) const
    {
    return aPos + iDataOffset;
    }

inline TInt TMsDataMemMap::FormatSectorShift() const
	{
	return iFormatSectorShift;
	}

