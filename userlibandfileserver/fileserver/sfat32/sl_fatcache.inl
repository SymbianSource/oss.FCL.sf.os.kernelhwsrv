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
// f32\sfat\sl_fatcache.inl
// 
//

/**
 @file
*/

#ifndef SL_FAT_CACHE_INL
#define SL_FAT_CACHE_INL




//-----------------------------------------------------------------------------

TUint32 CFatCacheBase::FatStartPos() const 
    {
    return iFatStartPos;
    }

TUint32 CFatCacheBase::FatSize() const 
    {
    return iFatSize;
    }

TFatType CFatCacheBase::FatType() const 
    {
    return iFatType;
    }

TBool CFatCacheBase::IsDirty() const 
    {
    return iDirty;
    }

void CFatCacheBase::SetDirty(TBool aDirty) 
    {
    iDirty = aDirty;
    }

TUint CFatCacheBase::NumFATs() const 
    {
    return iNumFATs;
    }

TUint CFatCacheBase::FAT_SectorSzLog2() const 
    {
    return iFatSecSzLog2;
    }

TUint CFatCacheBase::FAT_SectorSz() const 
    {
    return 1 << iFatSecSzLog2;
    }

TUint CFatCacheBase::FAT_ClusterSzLog2() const 
    {
    return iFatClustSzLog2;
    }
    
TUint32 CFatCacheBase::MaxFatEntries() const 
    {
    return iMaxFatEntries;
    }

TBool CFatCacheBase::FatIndexValid(TUint32 aIndex) const 
    {
    return (aIndex >= KFatFirstSearchCluster &&  aIndex < iMaxFatEntries);
    }

    

//-----------------------------------------------------------------------------

/** @return number of FAT cache sectors in this fixed cache */
TUint32 CFat12Cache::NumSectors() const   
    {
    return iSectorsInCache;
    }


//-----------------------------------------------------------------------------


/** @return Log2(page size in bytes) */
TUint CFatPagedCacheBase::PageSizeLog2() const 
    {
    return iPageSizeLog2;
    }

/** @return page size in bytes */
TUint CFatPagedCacheBase::PageSize() const 
    {
    return Pow2(iPageSizeLog2);
    }

/** @return Log2(size of the logical sector of the page in bytes) */    
TUint CFatPagedCacheBase::SectorSizeLog2() const 
    {
    return iSectorSizeLog2;
    }

/** @return number of the logical sector in the page */
TUint CFatPagedCacheBase::SectorsInPage() const 
    {
    return Pow2(iPageSizeLog2 - iSectorSizeLog2);
    }


//-----------------------------------------------------------------------------

TUint CFat16FixedCache::NumPages() const 
    {
    return (TUint)iPages.Count();
    } 

//-----------------------------------------------------------------------------


/** @return  the index in the FAT table this page starts from */
TUint32 CFatCachePageBase::StartFatIndex() const 
    {
    return iStartIndexInFAT;
    }

/** @return number of FAT entries in the page */
TUint32 CFatCachePageBase::EntriesInPage() const 
    {
    return iFatEntriesInPage;
    }

/** @return page state */
CFatCachePageBase::TState CFatCachePageBase::State() const
    {
    return iState;
    }

/** sets the state of the page */
void CFatCachePageBase::SetState(TState aState)
    {
    iState = aState;
    }

/** @return ETrue if the page is dirty, i.e. contains non-flushed dirty sectors */
TBool CFatCachePageBase::IsDirty() const
    {
    if(State() == EDirty)
        {
        ASSERT(iDirtySectors.HasBitsSet());
        return ETrue;
        }
    else
        {
        ASSERT(!iDirtySectors.HasBitsSet());
        return EFalse;
        }
    }

/** @return  ETrue if the page data are valid */
TBool CFatCachePageBase::IsValid() const
    {
    return (State() == EClean || State() == EDirty);
    }

/** force the page to the clean state */
void CFatCachePageBase::SetClean()
    {
    iDirtySectors.Clear(); //-- clear dirty sectors bitmap
    SetState(EClean);
    }

/** @return page size in bytes */
TUint32 CFatCachePageBase::PageSize() const 
    {
    return iCache.PageSize();
    }

/** @return number of logical sectors in the page */
TUint32 CFatCachePageBase::NumSectors() const 
    {
    return iCache.SectorsInPage();
    }

/** @return ETrue if the entry at aFatIndex belongs to this page */
TBool CFatCachePageBase::IsEntryCached(TUint32 aFatIndex) const
    {
    return (aFatIndex >= iStartIndexInFAT && aFatIndex < iStartIndexInFAT+EntriesInPage());
    } 


//---------------------------------------------------------------------------------------------------------------------------------






#endif //SL_FAT_CACHE_INL























