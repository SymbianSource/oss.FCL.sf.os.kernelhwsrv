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
// f32\sfat32\inc\fat_table32.h
// FAT32 File Allocation Table classes definitions
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef FAT_TABLE_32_H
#define FAT_TABLE_32_H

//---------------------------------------------------------------------------------------------------------------------------------------

class CFatHelperThreadBase;
class CFat32ScanThread;

TInt FAT32_ScanThread(TAny* apHostObject);

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Fat table abstraction for all media types except RAM.
    Supports FAT12/16/32
*/
class CAtaFatTable : public CFatTable
    {

public:
    
    static CAtaFatTable* NewL(CFatMountCB& aOwner);
    ~CAtaFatTable();

    //-- overrides from th ebase class
    TUint32 ReadL(TUint32 aFatIndex) const;
    void WriteL(TUint32 aFatIndex, TUint32 aValue);
    void MountL(const TMountParams& aMountParam);

    TInt64 DataPositionInBytesL(TUint32 aCluster) const;

    void InitializeL();
    void Dismount(TBool aDiscardDirtyData);
    void FlushL();
    
    void InvalidateCacheL();
    void InvalidateCacheL(TInt64 aPos, TUint32 aLength);

    TUint32 FreeClusterHint() const;
    TUint32 NumberOfFreeClusters(TBool aSyncOperation=EFalse) const;

    TBool ConsistentState() const;

    inline void AcquireLock() const {iDriveInteface.AcquireLock();} 
    inline void ReleaseLock() const {iDriveInteface.ReleaseLock();} 
    
    inline TDriveInterface& DriveInterface() const;
    inline CFatMountCB* OwnerMount() const;


private:
    
    /** internal states of this object */
    enum TState
        {
        ENotInitialised,    ///< 0 initial invalid state
        EInitialised,       ///< 1 initialised, i.e. internal objects created, but unusable becasue number of free entries isn't known
        EMounting,          ///< 2 mounting started
        EMounted,           ///< 3 successfully mounted; number of free entries is known. This is the only consistent state.
        EDismounted,        ///< 4 FAT table object is dismounted
        EFreeClustersScan,  ///< 5 FAT32 scan thread is currently scanning FAT table for free clusters
        EMountAborted       ///< 6 Mounting failed, probably because of FAT32 free clusters scan thread failure.
        };

public:

    /** A helper class used in FAT scanning for free clusters*/
    class TFatScanParam
        {
    public:
        inline TFatScanParam();

    public:
        TUint32 iEntriesScanned;        ///< total number of FAT entries scanned by DoParseFatBuf()
        TUint32 iFirstFree;             ///< first free FAT entry found 
        TUint32 iCurrFreeEntries;       ///< current number of free FAT entries found by DoParseFatBuf()
        TUint32 iCurrOccupiedEntries;   ///< current number of non-free FAT entries found by DoParseFatBuf()
        };


private:
    CAtaFatTable(CFatMountCB& aOwner);

    void RequestRawWriteAccess(TInt64 aPos, TUint32 aLen) const;
    
    void SetFreeClusters(TUint32 aFreeClusters);
    void SetFreeClusterHint(TUint32 aCluster);
    void CountFreeClustersL();
    void CreateCacheL();
    
    virtual void DecrementFreeClusterCount(TUint32 aCount); 
    virtual void IncrementFreeClusterCount(TUint32 aCount);
    virtual TUint32 FindClosestFreeClusterL(TUint32 aCluster);

    void DoCountFreeClustersL();
    void DoParseFatBuf(const TPtrC8& aBuf, TFatScanParam& aScanParam) const;

    TBool RequestFreeClusters(TUint32 aClustersRequired) const;
    void DoLaunchFat32FreeSpaceScanThreadL();
    void DestroyHelperThread();

    inline TState State() const;
    inline void SetState(TState aState);
    
private:

    
    CFatCacheBase*          iCache;         ///< FAT cache, fixed or LRU depending on the FAT type
    TDriveInterface&        iDriveInteface; ///< reference to the drive interface
    CFatHelperThreadBase*   ipHelperThread; ///< helper thread object pointer. NULL if it is not present
    TState                  iState;         ///< state of this object 

    //-- friends
    friend TInt FAT32_ScanThread(TAny* apHostObject);
    friend class CFat32ScanThread;
    friend class CFat32FreeSpaceScanner;
    friend class CFat32BitCachePopulator;
    };

//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Fat table abstraction for RAM media type.
    Supports FAT16/32 only
*/
class CRamFatTable : public CFatTable
    {
public:
    
    static CRamFatTable* NewL(CFatMountCB& aOwner);
    void InitializeL();
    void MountL(const TMountParams& aMountParam);


    TUint32 ReadL(TUint32 aFatIndex) const;
    void WriteL(TUint32 aFatIndex, TUint32 aValue);
    TInt64 DataPositionInBytesL(TUint32 aCluster) const;
    void FreeClusterListL(TUint32 aCluster);
    TUint32 AllocateSingleClusterL(TUint32 aNearestCluster);
    void ExtendClusterListL(TUint32 aNumber, TUint32& aCluster);
    TUint32 AllocateClusterListL(TUint32 aNumber,TUint32 aNearestCluster);
    
    virtual TUint32 CountContiguousClustersL(TUint32 aStartCluster, TUint32& anEndCluster, TUint32 aMaxCount) const;

private:
    CRamFatTable(CFatMountCB& aOwner);

    inline TUint8 *RamDiskBase() const;
    inline TInt AllocateClusterNumber();
    inline void WriteFatTable(TInt aFatIndex,TInt aValue);
    inline void WriteFatTable(TInt aFatIndex,TInt aFatValue,TInt anIndirectionTableValue);
    inline void ReadIndirectionTable(TUint32& aCluster) const;
    inline void WriteIndirectionTable(TInt aFatIndex,TInt aValue);
    inline TUint8* MemCopy(TAny* aTrg,const TAny* aSrc,TInt aLength);
    inline TUint8* MemCopyFillZ(TAny* aTrg, TAny* aSrc, TInt aLength);
    inline void ZeroFillClusterL(TInt aCluster);
    
    void UpdateIndirectionTable(TUint32 aStartCluster,TUint32 anEndCluster,TInt aNum);

protected:

    TInt iFatTablePos;          ///< Current position in the fat table
    TInt iIndirectionTablePos;  ///< Current position in indirection table, second fat used for this
    TUint8* iRamDiskBase;       ///< Pointer to the Ram disk base
    };



//---------------------------------------------------------------------------------------------------------------------------------------

/**
    Abstract base class for the FAT32 helper threads.
    Provides basic functionality of the helper threads and interface to the owher CAtaFatTable.
*/
class CFatHelperThreadBase : public CBase
{
 public: 
    
    /** Possible types of the FAT32 helper threads */
    enum TFatHelperThreadType
        {
        EInvalidType,       ///< invalid type
        EFreeSpaceScanner,  ///< Free FAT32 entries counter, see CFat32FreeSpaceScanner
        EBitCachePopulator  ///< FAT32 bit supercache populating thread.
        };
    
    /** this object states, mostly related to the worker thread activity and results */
    enum TState
        {
        EInvalid,       ///< invalid initial state
        ENotStarted,    ///< the worker thread hasn't started yet
        EWorking,       ///< worker thread is working
        EFinished_OK,   ///< worker thread has successfully finished, everything is fine.
        EFailed         ///< worker thread failed to finish its job for some reason; see the thread completion status
        };
 
 public:
    ~CFatHelperThreadBase();

    //-- virtual interface
    virtual TFatHelperThreadType Type() const = 0;
    virtual TInt Launch()=0; 
    virtual void RequestFatEntryWriteAccess(TUint32 aFatIndex) const=0;

    //-- non-virtual interface for external user only
    void Close();
    void ForceStop(); 
    
    inline TState State() const;

    inline void Suspend() const;
    inline void Resume()  const;
    
    inline TInt  ThreadCompletionCode() const;
    inline TBool ThreadWorking() const;
    
    inline void BoostPriority(TBool aBoost) const;
    inline TBool IsPriorityBoosted() const;

    inline TThreadId ThreadId() const;

    TInt WaitToFinish() const;

 protected:
    CFatHelperThreadBase(CAtaFatTable& aOwner);
     
    //-- outlaws
    CFatHelperThreadBase();
    CFatHelperThreadBase(const CFatHelperThreadBase&);
    CFatHelperThreadBase& operator=(const CFatHelperThreadBase&);

    /** the worker thread priorities values */
    enum 
        {
        EHelperPriorityNormal  = EPriorityMuchLess, ///< FAT32 Helper thread normal priority (assigned on start)
        EHelperPriorityBoosted = EPriorityNormal    ///< FAT32 Helper thread bosted priority
        };                                   

    TInt DoLaunchThread(TThreadFunction aFunction, TAny* aThreadParameter);

    inline void SetState(TState aState);
    inline TBool AllowedToLive() const;
    inline void  AllowToLive(TBool aAllow);

 protected:
    CAtaFatTable&           iOwner;         ///< owner, CAtaFatTable

 private:
    TState                  iState;         ///< internal state of this object
    RThread                 iThread;        ///< FAT helper thread handle
    mutable TRequestStatus  iThreadStatus;  ///< helper thread logon status
    
            TBool           iAllowedToLive   : 1;    ///< if EFalse the worker thread must gracefully finish ASAP.
    mutable TBool           iPriorityBoosted : 1;    ///< ETrue when thread priority is boosted by BoostPriority() call


};

//---------------------------------------------------------------------------------------------------------------------------------------
/**
    Abstract base class for the FAT32 helper threads that read FAT by big chunks of data and parse it.
    Provides basic functionality of the helper threads and interface to the owher CAtaFatTable.
*/
class CFat32ScanThread : public CFatHelperThreadBase
{
 public:   

    virtual TInt Launch();

 protected:
    CFat32ScanThread(CAtaFatTable& aOwner);
    
    //-- virtual private interface for the thread function.
    virtual TInt Thread_Preamble();
    virtual TInt Thread_Postamble(TInt aResult);
    virtual TInt Thread_ProcessCollectedFreeEntries(const CAtaFatTable::TFatScanParam& aFatScanParam)=0;    

    friend TInt FAT32_ScanThread(TAny* apHostObject); ///< FAT32 scanner thread function, generic functionality for derived classes

 protected:
     
    RBuf8   iFatChunkBuf; ///< a buffer for reading FAT directly from the media

    TTime   iTimeStart;   ///< thread start time, used to measure how long thread worked
    TTime   iTimeEnd;     ///< thread end time, used to measure how long thread worked

    CFatBitCache *ipFatBitCache; ///< interface to the FAT bit supercache (if it is present)
};


//---------------------------------------------------------------------------------------------------------------------------------------
/**
    FAT32 free FAT entries scanner thread.
    Represents transient FAT32 helper thread that can be launched on FAT table object mounting stage and will be 
    counting free FAT entries in order to find out free space on the volume.
*/
class CFat32FreeSpaceScanner : public CFat32ScanThread
{
 public:   

    static CFat32FreeSpaceScanner* NewL(CAtaFatTable& aOwner);

    virtual inline TFatHelperThreadType Type() const;

 private:
    CFat32FreeSpaceScanner(CAtaFatTable& aOwner);
    
    void RequestFatEntryWriteAccess(TUint32 aFatIndex) const;

    //-- virtual private interface for the thread function.
    TInt Thread_Preamble();
    TInt Thread_Postamble(TInt aResult);
    TInt Thread_ProcessCollectedFreeEntries(const CAtaFatTable::TFatScanParam& aFatScanParam);    
    //--

 private:
    
    void SetClustersScanned(TUint32 aClusters);
    TUint32 ClustersScanned() const;
    
    friend TInt FAT32_ScanThread(TAny* apHostObject); ///< FAT32 scanner thread function, generic functionality for CFat32ScanThread derived classes

 private:

    enum 
        {
        KFatChunkBufSize_Small = 16*K1KiloByte, //-- buffer size for reading small FAT tables
        KFatChunkBufSize_Big   = 64*K1KiloByte, //-- buffer size for reading large FAT tables
        
        KBigSzFat_Threshold    = 2*K1MegaByte,  //-- if FAT table size > this value, larger FAT read chunk (KFatChunkBufSize_Big) will be used
        }; 

    TUint32  iClustersScanned;  ///<  Number of FAT entries already scanned by the thread. Counts from the beginning of the FAT

    //-- volume space treshold in bytes that causes CMountCB::SetDiskSpaceChange() to be called by FAT32 free space scanner thread.
    //-- This thread will be calling CMountCB::SetDiskSpaceChange() after processing number of FAT32 entries corresponding to
    //-- this amount of space in FAT clusters. e.g. after processing amount of FAT32 entries comprising 256MB volume space
    enum {KVolSpaceNotifyThreshold = 256 * K1MegaByte}; 

    TUint32 iEntriesNotifyThreshold; ///< the value of FAT32 entries need to be counted for CMountCB::SetDiskSpaceChange() call
    TUint32 iNfyThresholdInc;        ///< Threshold increment in FAT32 entries. 

    
};

//---------------------------------------------------------------------------------------------------------------------------------------
/**
    FAT32 Bit supercache populating thread.
    Represents transient FAT32 helper thread that is populating bit supercache in backgroud.
*/
class CFat32BitCachePopulator : public CFat32ScanThread
{
 public:   

    static CFat32BitCachePopulator* NewL(CAtaFatTable& aOwner);

    virtual inline TFatHelperThreadType Type() const;

 private:
    CFat32BitCachePopulator(CAtaFatTable& aOwner);
    
    void RequestFatEntryWriteAccess(TUint32 aFatIndex) const;

    //-- virtual private interface for the thread function.
    TInt Thread_Preamble();
    TInt Thread_Postamble(TInt aResult);
    TInt Thread_ProcessCollectedFreeEntries(const CAtaFatTable::TFatScanParam& aFatScanParam);    
    //--

 private:
    friend TInt FAT32_ScanThread(TAny* apHostObject); ///< FAT32 scanner thread function, generic functionality for CFat32ScanThread derived classes
    enum {KFatChunkBufSize = 16*K1KiloByte}; //-- buffer size for FAT reading 

 private:
    TUint32  iTotalOccupiedFatEntries; ///< total counted number of non-free FAT entries

};


//---------------------------------------------------------------------------------------------------------------------------------------

#include "fat_table32.inl"

#endif //FAT_TABLE_32_H























