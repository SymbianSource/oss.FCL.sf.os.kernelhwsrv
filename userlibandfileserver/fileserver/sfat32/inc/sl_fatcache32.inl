// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat32\inc\sl_facache32.h
// FAT32 various cache classes definition
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef SL_FAT_CACHE_32_INL
#define SL_FAT_CACHE_32_INL


//-----------------------------------------------------------------------------


void CFatBitCache::SetState(CFatBitCache::TState aState) 
    {
    iState = aState;
    }

/**
    Converts FAT index to the corresponding bit array bit number (or FAT cache sector number).
    @param  aFatIndex index in the FAT 
    @return corresponding index in the FAT bit array
*/
TUint32 CFatBitCache::FatIndexToCacheSectorNumber(TUint32 aFatIndex) const
    {
    ASSERT(aFatIndex >= KFatFirstSearchCluster);
    return (aFatIndex >> iFatIdxToSecCoeff);
    }

/**
    Converts FAT32 cache sector number to the FAT32 entry index (in the beginning of this cache sector)
    @param  aCacheSecNum index in the FAT bit array (or FAT cache sector number)
    @return corresponding index in the FAT 

*/
TUint32 CFatBitCache::CacheSectorNumberToFatIndex(TUint32 aCacheSecNum) const
    {
    ASSERT(aCacheSecNum < iBitCache.Size());
    return (aCacheSecNum << iFatIdxToSecCoeff);
    }

/** @return state of the cache*/
CFatBitCache::TState CFatBitCache::State() const 
    {
    return iState;
    }

/** 
    @return ETrue if the cache can be used, i.e. is fully populated
*/
TBool CFatBitCache::UsableState() const
    {
    return State() == EPopulated;
    }


/**
    @return ETrue if the FAT cache sector number "aFatSectorNum" is marked as having at least one free FAT entry.
    N.B. The cache must be in consistent state, i.e. fully populated.
*/
TBool CFatBitCache::FatSectorHasFreeEntry(TUint32 aFatSectorNum) const
    {
    ASSERT(UsableState());
    return iBitCache[aFatSectorNum];
    }

/**
    Mark FAT cache sector number "aFatSectorNum" is as having/not having free FAT entry.
    N.B. The cache must be in consistent state, i.e. fully populated.

    @param  aFatSectorNum FAT32 cache sector number [index in a bit vector] 
    @param  aHasFreeEntry ETrue  if we want to mark this sector as having free FAT entries; 
                          EFalse if we want to mark this sector as NOT having free FAT entries; 
*/
void CFatBitCache::SetFreeEntryInFatSector(TUint32 aFatSectorNum, TBool aHasFreeEntry)
    {
    ASSERT(UsableState());
    iBitCache.SetBitVal(aFatSectorNum, aHasFreeEntry);
    }


#endif //SL_FAT_CACHE_32_INL























