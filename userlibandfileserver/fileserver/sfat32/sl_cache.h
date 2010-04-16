// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat32\inc\sl_cache.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef SL_CACHE_H
#define SL_CACHE_H


//---------------------------------------------------------------------------------------------------------------------------------
//-- dedicated FAT directory cache related stuff


//---------------------------------------------------------------------------------------------------------------------------------


/** 
    An abstract interface to the media Write-Through cache
*/
class MWTCacheInterface
    {
public:
        
	/** Enums for control functions. See Control() */
	enum TControl
	    {
	    EDisableCache = 0, 	///< disable/enable cache, can be used for debug purposes
	    EDumpCache = 1, 	///< print full cache content, can be used for debug purposes
	    ECacheInfo = 2, 	///< print cache info, can be used for debug purposes
	    };

        virtual ~MWTCacheInterface() {}
        
        /** the same meaning and parameters as in CRawDisk::ReadL */
        virtual void    ReadL(TInt64 aPos, TInt aLength, TDes8& aDes)=0;
        
        /** the same meaning and parameters as in CRawDisk::WriteL */
        virtual void    WriteL(TInt64 aPos,const TDesC8& aDes)=0;
        
        /** Invalidates whole directory cache*/
        virtual void    InvalidateCache(void)=0;

        /** invalidate a single cache page if the aPos is cached*/
        virtual void    InvalidateCachePage(TUint64 aPos)=0;
        
        /**
        Finds out if the media position "aPosToSearch" is in the cache and returns cache page information in this case.
        @param  aPosToSearch    linear media position to lookup in the cache
        @return 0 if aPosToSearch isn't cached, otherwise  cache page size in bytes (see also aCachedPosStart).
        */
        virtual TUint32  PosCached(TInt64 aPosToSearch) = 0;
        
        /**
        @return size of the cache in bytes. Can be 0.
        */
        virtual TUint32 CacheSizeInBytes() const = 0;
        
        /**
        Make the page indexed by aPos the MRU page in the cache.
        Assumes cache evicts pages according to LRU algorithm.
        */
        virtual void MakePageMRU(TInt64 aPos) = 0;

        /**
        @return log2 number of the size of the cache in bytes.
        */
        virtual TUint32 PageSizeInBytesLog2() const = 0;

        /**
        Control method.
        
          @param  aFunction   control function
          @param  aParam1     just arbitrary parameter 
          @param  aParam2     just arbitrary parameter 
          @return Standard error code.
        */
        virtual TInt Control(TUint32 aFunction, TUint32 aParam1, TAny* aParam2)=0;
        
        /**
        Set cache base position at aBasePos
        @param  aBasePos  base position of the cache pages. Affects pages alignment.
        */
        virtual void SetCacheBasePos(TInt64 aBasePos)=0;
        
    };

//---------------------------------------------------------------------------------------------------------------------------------

/**
This class represents the media Write-Through cache page
*/
class CWTCachePage
    {
public:   
        
        static CWTCachePage* NewL(TUint32 aPageSizeLog2);
        
        ~CWTCachePage();
        
        inline TBool   PosCached(TInt64 aPos) const;
        inline TUint32 PosInCachePage(TInt64 aPos) const; 
        inline TUint8* PtrInCachePage(TInt64 aPos) const; 
        inline TUint32 PageSize() const;
        
protected:
        
        CWTCachePage();
        CWTCachePage(const CWTCachePage&);
        CWTCachePage& operator=(const CWTCachePage&);
        
public:
        
        TInt32  iValid;     ///< 0 if the page doesn't contain valid data
        TInt64  iStartPos;  ///< cache page base media position
        RBuf8   iData;      ///< page Data
    };

//---------------------------------------------------------------------------------------------------------------------------------

/**
    Media Write-through cache.
*/
class CMediaWTCache : public CBase, public MWTCacheInterface
    {
public:
        ~CMediaWTCache();
        
        static CMediaWTCache* NewL(TDriveInterface& aDrive, TUint32 aNumPages, TUint32 aPageSizeLog2, TUint32 aWrGranularityLog2);
        
        //-- overloads from the base class
        void    ReadL (TInt64 aPos,TInt aLength,TDes8& aDes);
        void    WriteL(TInt64 aPos,const TDesC8& aDes);
        void    InvalidateCache(void);
        void    InvalidateCachePage(TUint64 aPos);


        TUint32 PosCached(TInt64 aPosToSearch);
        TUint32 CacheSizeInBytes()  const;
        void 	MakePageMRU(TInt64 aPos);
        TUint32 PageSizeInBytesLog2()	const;
        TInt    Control(TUint32 aFunction, TUint32 aParam1, TAny* aParam2);
        inline void SetCacheBasePos(TInt64 aBasePos);
        //--
        
protected:
        CMediaWTCache();
        CMediaWTCache(TDriveInterface& aDrive);
        
        void InitialiseL(TUint32 aNumPages, TUint32 aPageSizeLog2, TUint32 aWrGranularityLog2);

        inline TInt64  CalcPageStartPos(TInt64 aPos) const;
        inline TUint32 PageSize() const;
        
        void MakePageLRU(TInt aPageNo);
        
        TInt    FindPageByPos(TInt64 aPos) const;
        TUint32 GrabPage() const;
        TUint32 GrabReadPageL(TInt64 aPos);
        TUint32 FindOrGrabReadPageL(TInt64 aPos);
        
protected:
        TDriveInterface& iDrive;        ///< reference to the driver for media access
        
        TUint32             iPageSizeLog2;      ///< Log2(cache page size or read granularity unit) 
        TUint32             iWrGranularityLog2; ///< Log2(cache write granularity unit). Can't be > iPageSizeLog2. '0' has a special meaning - "don't use write granularity"

        mutable TBool       iAllPagesValid;///< ETrue if all cache pages have valid data
        TInt64              iCacheBasePos; ///< Cache pages base position, used to align them at cluster size
        RPointerArray<CWTCachePage> iPages; ///< array of pointers to the cache pages. Used for organising LRU list
        
        TUint32             iCacheDisabled :1; ///< if not 0 the cache is disabled totally and all reads and writes go via TDriveInterface directly
    };




#include"sl_cache.inl"

#endif //SL_CACHE_H




