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

#ifndef TMSMEMMAP_H
#define TMSMEMMAP_H



class TMsDataMemMap
	{
public:
    static const TInt KSectorSize = 0x200; // 512
    static const TInt KFormatSectorShift = 9;

	TMsDataMemMap();
    void Reset();

    void InitDataArea(TUint32 aFirstDataSector, TUint32 aNumSectors);
    void InitDataArea(TUint64 aSize);

    TInt BlockLength() const;
    TUint64 DataSize() const;

    TInt64 GetDataPos(TInt64 aPos) const;
    TInt TranslateDataPos(TInt64& aPos, TInt& aLength) const;
    TInt CheckBlockInRange(TInt64& aPos, TInt aLength) const;

private:
    // Whole media
    // Size
    TUint64 iSize;

    // Data Area
    // Offset
    TInt64 iDataOffset;
	};

#include "tmsmemmap.inl"

#endif // TMSMEMMAP_H
