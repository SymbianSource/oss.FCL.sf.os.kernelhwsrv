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

#ifndef TMSMEMMAP_H
#define TMSMEMMAP_H



class TMsDataMemMap
	{
public:

	TMsDataMemMap();
    void Reset();

    void InitDataArea(TUint32 aFirstDataSector, TUint32 aNumSectors, TUint32 aSectorSize);
    void InitDataArea(TUint64 aSize);

    TUint32 BlockLength() const;
    TUint64 DataSize() const;

    TInt64 GetDataPos(TInt64 aPos) const;
    TInt TranslateDataPos(TInt64& aPos, TInt& aLength) const;
    TInt CheckBlockInRange(TInt64& aPos, TInt aLength) const;
    TInt FormatSectorShift() const;

private:
    // Whole media
    // Size
    TUint64 iSize;

    // Data Area
    // Offset
    TInt64 iDataOffset;

    // Sector Size (Media Block Size)
    TUint32 iSectorSize;
    
    // Sector-size dependant
    TInt iFormatSectorShift;
	};

#include "tmsmemmap.inl"

#endif // TMSMEMMAP_H
