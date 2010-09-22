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
// fileserver\sfat32\inc\sl_std.h
//

/**
 @file
 @internalTechnology
*/

#ifndef SL_STD_H
#define SL_STD_H


//
// #define _DEBUG_RELEASE
//

#include "common.h"
#include <f32ver.h>
#include <e32svr.h>
#include <kernel/localise.h>

#include "filesystem_fat.h"
using namespace FileSystem_FAT;

#include "common_constants.h"
#include "sl_bpb.h"
#include "fat_config.h"
#include "fat_dir_entry.h"
#include "bit_vector.h"



#ifdef _DEBUG
_LIT(KThisFsyName,"EFAT32.FSY"); ///< This FSY name
#endif

//-----------------------------------------------------------------------------
//-- FAT32 specific declarations

const TUint32 KFat32EntryMask = 0x0FFFFFFF;

//-----------------------------------------------------------------------------

class CFatMountCB;
class CFatFileSystem;

/**
Represents the position of a directory entry in terms of a cluster and offset into it
*/
class TEntryPos
	{
public:
	TEntryPos() : iCluster(EOF_32Bit), iPos(0) {}
	TEntryPos(TUint aCluster,TUint aPos) : iCluster(aCluster), iPos(aPos) {}

    inline TUint32 Cluster() const;
    inline TUint32 Pos() const;
    inline TBool operator==(const TEntryPos& aRhs) const;
    inline void SetEndOfDir();

public:
	TUint32 iCluster;
	TUint32 iPos;
	};


/**
    Interface class between the file system and the local drive media interface,
    handles incomplete writes to media with the ability to notify the user.
    This class can't be instantinated by user; only CFatMountCB can do this; see CFatMountCB::DriveInterface()

*/
class TDriveInterface
    {
public:
	enum TAction {ERetry=1};

public:

    //-- public interface to the local drive. Provides media driver's error handling (critical and non-critical user notifiers)
    //-- and thread-safety if required.
	TInt ReadNonCritical(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const;
	TInt ReadNonCritical(TInt64 aPos,TInt aLength,TDes8& aTrg) const;
	TInt ReadCritical(TInt64 aPos,TInt aLength,TDes8& aTrg) const;
	
    TInt WriteCritical(TInt64 aPos,const TDesC8& aSrc);
    TInt WriteNonCritical(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag);
	
    TInt GetLastErrorInfo(TDes8& aErrorInfo) const;

    //-- lock the mutex guarding CProxyDrive interface in order to be sure that no other thread can access it.
    //-- The thread that calls this method may be suspended until another signals the mutex, i.e. leaves the critical section.
    inline void AcquireLock() const {iProxyDrive.EnterCriticalSection();}
    
    //-- release the mutex guarding CProxyDrive.
    inline void ReleaseLock() const {iProxyDrive.LeaveCriticalSection();} 


protected:
    TDriveInterface();
   ~TDriveInterface() {Close();}

    //-- outlawed
    TDriveInterface(const TDriveInterface&);
    TDriveInterface& operator=(const TDriveInterface&);
    void* operator new(TUint); //-- disable creating objets of this class on the heap.
    void* operator new(TUint, void*);

    TBool Init(CFatMountCB* aMount);
    void Close(); 

	inline TBool NotifyUser() const;
	TInt HandleRecoverableError(TInt aRes) const;
	TInt HandleCriticalError(TInt aRes) const;
	TInt UnlockAndReMount() const;
	TBool IsDriveWriteProtected() const;
	TBool IsRecoverableRemount() const;

private:
	
	/** 
        An internal class that represents a thread-safe wrapper around raw interface to the CProxyDrive 
        and restricts access to it.
    */
    class XProxyDriveWrapper
        {
     public:
       
        XProxyDriveWrapper();
       ~XProxyDriveWrapper();

        TBool Init(CProxyDrive* aProxyDrive);
            
        inline void EnterCriticalSection() const {iLock.Wait();}
        inline void LeaveCriticalSection() const {iLock.Signal();}

        //-- methods' wrappers that are used by TDriveInterface
        TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const;
        TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg) const;
        TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag);
        TInt Write(TInt64 aPos, const TDesC8& aSrc);
        TInt GetLastErrorInfo(TDes8& aErrorInfo) const;
        TInt Caps(TDes8& anInfo) const;

     private:
        CProxyDrive*    iLocalDrive; ///< raw interface to the media operations
        mutable RMutex  iLock;       ///< used for sorting out multithreaded access to the iLocalDrive
        };
    
    CFatMountCB*        iMount;      ///< Pointer to the owning file system mount
    XProxyDriveWrapper  iProxyDrive; ///< wrapper around raw interface to the media operations
    
    };


/**
    Class providing FAT table interface and basic functionality.
*/	
class CFatTable : public CBase
	{
    
public:
	
    virtual ~CFatTable();
    
    static CFatTable* NewL(CFatMountCB& aOwner, const TLocalDriveCaps& aLocDrvCaps);

    /** FAT table mounting parameters */
    struct TMountParams
        {
        TMountParams() :iFreeClusters(0), iFirstFreeCluster(0), iFsInfoValid(0) {};
        
        TUint32 iFreeClusters;      ///< Free clusters count, obtained from FSInfo
        TUint32 iFirstFreeCluster;  ///< First free cluster,  obtained from FSInfo
        TUint32 iFsInfoValid : 1;   ///< ETrue if the information in iFreeClusters &  iFirstFreeCluster is valid
        };

    //-----------------------------------------------------------------
    //-- pure virtual interface
    virtual TUint32 ReadL(TUint32 aFatIndex) const = 0;
	virtual void WriteL(TUint32 aFatIndex, TUint32 aValue) = 0;
	virtual TInt64 DataPositionInBytesL(TUint32 aCluster) const = 0;
    virtual void MountL(const TMountParams& aMountParam) = 0;
    //-----------------------------------------------------------------
    //-- just virtual interface
    
    virtual void Dismount(TBool /*aDiscardDirtyData*/) {}
	virtual void FlushL() {};
    
    virtual void InvalidateCacheL() {};
    virtual void InvalidateCacheL(TInt64 /*aPos*/,TUint32 /*aLength*/) {};

	virtual void FreeClusterListL(TUint32 aCluster);
	virtual void ExtendClusterListL(TUint32 aNumber, TUint32& aCluster);
	
    virtual TUint32 AllocateSingleClusterL(TUint32 aNearestCluster);
	virtual TUint32 AllocateClusterListL(TUint32 aNumber,TUint32 aNearestCluster);
    
    virtual void RequestRawWriteAccess(TInt64 /*aPos*/, TUint32 /*aLen*/) const {};

    virtual TUint32 FreeClusterHint() const; 
    virtual TUint32 NumberOfFreeClusters(TBool aSyncOperation=EFalse) const;
    virtual TBool RequestFreeClusters(TUint32 aClustersRequired) const; 

    virtual void InitializeL();
    virtual TBool ConsistentState() const {return ETrue;} //-- dummy

    virtual TUint32 CountContiguousClustersL(TUint32 aStartCluster, TUint32& anEndCluster, TUint32 aMaxCount) const;

    //-----------------------------------------------------------------
    //-- non-virtual interface
    TBool GetNextClusterL(TUint32& aCluster) const;
    void WriteFatEntryEofL(TUint32 aFatIndex);

    void MarkAsBadClusterL(TUint32 aCluster);
    
    
    inline TUint32 MaxEntries() const;
    
    TUint32 PosInBytes(TUint32 aFatIndex) const;	    


protected:
	CFatTable(CFatMountCB& aOwner);

    //-- outlawed
    CFatTable();
    CFatTable(const CFatTable&);
    CFatTable& operator=(const CFatTable&);

    virtual void SetFreeClusterHint(TUint32 aCluster);

    virtual void DecrementFreeClusterCount(TUint32 aCount);
	virtual void IncrementFreeClusterCount(TUint32 aCount);

    virtual void SetFreeClusters(TUint32 aFreeClusters); 
    virtual void CountFreeClustersL();
    virtual TUint32 FindClosestFreeClusterL(TUint32 aCluster);
    
    
    inline TInt     SectorSizeLog2() const;
    inline TUint32  FreeClusters() const;

    inline TBool IsEndOfClusterCh(TUint32 aCluster) const;


    inline TFatType FatType() const;
    inline TBool IsFat12() const;
    inline TBool IsFat16() const;
    inline TBool IsFat32() const;
    

    inline TBool ClusterNumberValid(TUint32 aClusterNo) const;

    typedef RArray<TUint> RClusterArray;
    void DoFreedClustersNotifyL(RClusterArray &aFreedClusters);


protected:
	
    CFatMountCB* iOwner;            ///< Owning file system mount
    TUint        iMediaAtt;         ///< Cached copy of TLocalDriveCaps::iMediaAtt

private:   

    TUint32  iFreeClusters;     ///< Number of free cluster in the fat table
	TUint32  iFreeClusterHint;  ///< this is just a hint to the free cluster number, not required to contain exact information.
	TFatType iFatType;          ///< FAT type 12/16/32, cached from the iOwner
    TUint32  iFatEocCode;       ///< End Of Cluster Chain code, 0xff8 for FAT12, 0xfff8 for FAT16, and 0xffffff8 for FAT32 
    TUint32  iMaxEntries;       ///< maximal number of FAT entries in the table. This value is taken from the CFatMount that calculates it

    };


class MWTCacheInterface;


//---------------------------------------------------------------------------------------------------------------------------------

/**
Base class abstraction of a raw media disk
*/


class CRawDisk : public CBase
	{
public:

    static CRawDisk* NewL(CFatMountCB& aOwner, const TLocalDriveCaps& aLocDrvCaps);

    virtual void InitializeL();
	
    virtual TInt GetLastErrorInfo(TDes8& aErrorInfo) const;
public:
	
    /**
	Read data from the media via simple WT data cache if it is present. Some media types, like RAM do not have caches.
    This method is mostly used to read UIDs of executable modules and store them in the cache.

	@param aPos		Media position in bytes
	@param aLength	Length in bytes of read
	@param aDes		Data from read
	*/
	virtual void ReadCachedL(TInt64 aPos,TInt aLength,TDes8& aDes) const = 0;
	
    /**
	Write data to the media via simple WT data cache if it is present. Some media types, like RAM do not have caches.
	@param aPos		Media position in bytes
	@param aDes		Data to write
	*/
	virtual void WriteCachedL(TInt64 aPos,const TDesC8& aDes) = 0;
    
    virtual void InvalidateUidCache() {}
    virtual void InvalidateUidCachePage(TUint64 /*aPos*/) {}
    

	/**
	Disk read function
	
	@param aPos		Media position in bytes
	@param aLength	Length in bytes of read
	@param aTrg		Pointer to the data descriptor, i.e. (const TAny*)(&TDes8)
	@param aMessage	Refrence to server message from request
	@param anOffset	Offset into read data to write
	*/
	virtual void ReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const = 0;

	/**
	Disk write function

	@param aPos		Media position in bytes
	@param aLength	Length in bytes of write
	@param aTrg		Pointer to the data descriptor, i.e. (const TAny*)(&TDes8)
	@param aMessage	Refrence to server message from request, contains data
	@param anOffset	Offset into write data to use in write
	*/
	virtual void WriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) = 0;

    
    virtual inline MWTCacheInterface* DirCacheInterface();




protected:

    CRawDisk(CFatMountCB& aOwner);

    //-- outlawed
    CRawDisk(); 
    CRawDisk(const CRawDisk&); 
    CRawDisk& operator=(const CRawDisk&);


protected:

	CFatMountCB* iFatMount; ///< Owning file system mount
    
    

	};

class CFatFileCB;
class RBitVector;

/** 
    A helper class. Holds the FAT volume parameters, which in turn are obtained from the Boot Sector
*/
class TFatVolParam
    {
public:   

    TFatVolParam();
    void Populate(const TFatBootSector& aBootSector);
    TBool operator==(const TFatVolParam& aRhs) const;

    //-- simple getters
    TUint32 ClusterSizeLog2() const     {return iClusterSizeLog2;    }        
    TUint32 SectorSizeLog2() const      {return iSectorSizeLog2;     }        
    TUint32 RootDirEnd() const          {return iRootDirEnd;         }        
    TUint32 SectorsPerCluster() const   {return iSectorsPerCluster;  }        
    TUint32 RootDirectorySector() const {return iRootDirectorySector;}        
    TUint32 FirstFatSector() const      {return iFirstFatSector;     }        
    TUint32 TotalSectors() const        {return iTotalSectors;       }        
    TUint32 NumberOfFats() const        {return iNumberOfFats;       }        
    TUint32 FatSizeInBytes() const      {return iFatSizeInBytes;     }        
    TUint32 RootClusterNum() const      {return iRootClusterNum;     }        
    TUint32 FSInfoSectorNum() const     {return iFSInfoSectorNum;    }        
    TUint32 BkFSInfoSectorNum() const   {return iBkFSInfoSectorNum;  }        
 
protected:
    TUint32 iClusterSizeLog2;      ///< Log2 of fat file system cluster size
    TUint32 iSectorSizeLog2;       ///< Log2 of media sector size
    TUint32 iRootDirEnd;           ///< End position of the root directory for Fat12/16
    TUint32 iSectorsPerCluster;    ///< Sector per cluster ratio for mounted Fat file system volume 
    TUint32 iRootDirectorySector;  ///< Start sector of the root directory for Fat12/16
    TUint32 iFirstFatSector;       ///< Start sector of the first Fat table in volume
    TUint32 iTotalSectors;         ///< Total sectors on media partition
    TUint32 iNumberOfFats;         ///< Number of Fats the volume has
    TUint32 iFatSizeInBytes;       ///< Size of a single Fat table in volume
    TUint32 iRootClusterNum;       ///< Cluster number for Root directory, for Fat32
    TUint32 iFSInfoSectorNum;      ///< FSInfo Sector number. If 0, this means that corresponding value isn't set in BPB
    TUint32 iBkFSInfoSectorNum;    ///< backup FSInfo Sector number
    };


TBool IsLegalDosName(const TDesC&  aName, TBool anAllowWildCards, TBool aUseExtendedChars, TBool aInScanDrive, TBool aAllowLowerCase, TBool aIsForFileCreation);
TBool IsLegalDOSNameChar(TChar aCharacter, TBool aUseExtendedChars);

TUint32 CalculatePageOffsetInCluster(TUint32 aPos, TUint aPageSzLog2);

class CLruCache;
class TLeafDirData;
class CLeafDirCache;


/**
Fat file system mount implementation, provides all that is required of a plug in
file system mount as well as Fat mount specific functionality 
*/
class CFatMountCB : public CLocDrvMountCB, 
					public MFileSystemSubType,
					public MFileSystemClusterSize,
					public CMountCB::MFileAccessor,
					public CMountCB::MFileExtendedInterface
	{
public:
	static CFatMountCB* NewL();
	~CFatMountCB();
    void ConstructL();

public:
	
	//-- overrides from the abstract CMountCB
    void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aMask,TUint aVal);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage);
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
    TInt CheckDisk();
	TInt ScanDrive();
	TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	TInt Lock(TMediaPassword& aOld,TMediaPassword& aNew,TBool aStore);
	TInt Unlock(TMediaPassword& aPassword,TBool aStore);
	TInt ClearPassword(TMediaPassword& aPassword);
	TInt ErasePassword();
	TInt ForceRemountDrive(const TDesC8* aMountInfo,TInt aMountInfoMessageHandle,TUint aFlags);
    
    void FinaliseMountL();
    void FinaliseMountL(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);
    TInt MountControl(TInt aLevel, TInt aOption, TAny* aParam);
	TTimeIntervalSeconds TimeOffset() const;

protected:

    /** CFatMountCB states */
    enum  TFatMntState
        {
        ENotMounted = 0, ///< 0, initial state, not mounted     (mount state is inconsistent)
        EMounting,       ///< 1, Mounting started               (mount state is inconsistent)
        EInit_R,         ///< 2, Initialised and not written    (mount state is Consistent)
        EInit_W,         ///< 3, Initialised and written        (mount state is Consistent)
        EFinalised,      ///< 4, Finalised                      (mount state is Consistent)
        EDismounted,     ///< 5, Dismounted                     (mount state is inconsistent)
        EInit_Forced,    ///< 6, forcedly mounted, special case (mount state is inconsistent)
        };

    inline TFatMntState State() const;
    inline void SetState(TFatMntState aState); 
    TInt OpenMountForWrite();
    TInt IsFinalised(TBool& aFinalised);

    /** 
        A wrapper around TDriveInterface providing its instantination and destruction.
        You must not create objects of this class, use DriveInterface() method for obtaining the reference to the driver interface.
    */
    class XDriveInterface: public TDriveInterface
        {
      public:
        using TDriveInterface::Init;
        };


public:
    
	enum TRenMode {EModeReplace,EModeRename};

    TBool ConsistentState() const; 
    void CheckWritableL() const;
    void CheckStateConsistentL() const;

	inline TBool ReadOnly(void) const;
    inline void  SetReadOnly(TBool aReadOnlyMode);
	inline TUint32 StartCluster(const TFatDirEntry & anEntry) const;
	inline CRawDisk& RawDisk() const;
	inline CFatFileSystem& FatFileSystem() const;
	inline CFatTable& FAT() const;
	inline TUint32 ClusterSizeLog2() const;
	inline TUint32 SectorSizeLog2() const;
	inline TUint32 TotalSectors() const;
	inline TUint32 SectorsPerCluster() const;
	inline TUint32 ClusterBasePosition() const;
    inline TUint32 RootDirectorySector() const;
    inline TUint32 RootDirEnd() const;
    inline TUint32 RootClusterNum() const;

	inline TFatType FatType() const;
    inline TBool Is16BitFat() const;
    inline TBool Is32BitFat() const;

	inline TUint32 MaxClusterNumber() const;
	inline TUint32 StartOfFatInBytes() const;
    inline TUint32 FirstFatSector() const;

	inline TUint32 NumberOfFats() const;
	inline TUint32 FatSizeInBytes() const;
	inline TUint32 ClusterRelativePos(TUint32 aPos) const;
	inline TUint32 StartOfRootDirInBytes() const;
	inline TUint32 UsableClusters() const;
    inline TBool ClusterNumberValid(TUint32 aClusterNo) const;
	inline TBool IsBadCluster(TUint32 aCluster) const;
	
	inline TBool IsRuggedFSys() const;
	inline void SetRuggedFSys(TBool aVal);
	inline TUint32 AtomicWriteGranularityLog2() const;

	
	inline TUint32 RootIndicator() const;
	
    inline TBool IsRootDir(const TEntryPos &aEntry) const;
	inline CAsyncNotifier* Notifier() const;
	inline TDriveInterface& DriveInterface() const;

    inline TBool IsEndOfClusterCh(TUint32 aCluster) const;
	inline void SetEndOfClusterCh(TUint32 &aCluster) const;

    
    void ReadUidL(TUint32 aCluster,TEntry& anEntry) const;
	
    void ReadDirEntryL(const TEntryPos& aPos,TFatDirEntry& aDirEntry) const;
	void WriteDirEntryL(const TEntryPos& aPos,const TFatDirEntry& aDirEntry);

    void DirReadL(const TEntryPos& aPos,TInt aLength,TDes8& aDes) const;
    void DirWriteL(const TEntryPos& aPos,const TDesC8& aDes);

	void ReadFromClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2& aMessage,TInt anOffset, TUint aFlag) const;
    void WriteToClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2& aMessage,TInt anOffset, TUint& aBadcluster, TUint& aGoodcluster, TUint aFlag);
	
	void MoveToNextEntryL(TEntryPos& aPos) const;
	void MoveToDosEntryL(TEntryPos& aPos,TFatDirEntry& anEntry) const;
	void EnlargeL(TInt aSize);
	void ReduceSizeL(TInt aPos,TInt aLength);
	void DoDismount();
    TBool ProcessFSInfoSectors(CFatTable::TMountParams& aFatInitParams) const;
    

	void DoRenameOrReplaceL(const TDesC& anOldName,const TDesC& aNewName,TRenMode aMode,TEntryPos& aNewPos);
	void FindDosNameL(const TDesC& aName,TUint anAtt,TEntryPos& aDosEntryPos,TFatDirEntry& aDosEntry,TDes& aFileName,TInt anError) const;
	
	void Dismount();
	
    void InitializeRootEntry(TFatDirEntry & anEntry) const;

    TInt64 MakeLinAddrL(const TEntryPos& aPos) const;
	
    inline const TFatConfig& FatConfig() const;
    TBool CheckVolumeTheSame();

    void InvalidateLeafDirCache();
    
    void BlockMapReadFromClusterListL(TEntryPos& aPos, TInt aLength, SBlockMapInfo& aInfo);
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

public:
	
    // interface extension implementation
	virtual TInt SubType(TDes& aName) const;
	virtual TInt ClusterSize() const;
    virtual void ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage);

private:
    
    /** An ad hoc internal helper object for using in DoFindL() method and its derivatives */
    class TFindHelper
        {
        public:
            TFindHelper() {isInitialised = EFalse;}
            void  InitialiseL(const TDesC&  aTargetName);
            TBool MatchDosEntryName(const TUint8* apDosEntryName) const;
            TBool TrgtNameIsLegalDos() const {ASSERT (isInitialised) ;return isLegalDosName;}   
        
        private:
            TFindHelper(const TFindHelper&);
            TFindHelper& operator=(const TFindHelper&);
        public:
            TPtrC       iTargetName;        ///< pointer to the aTargetName, used in DoRummageDirCacheL() as a parameter
        private:
            TBool       isInitialised  :1;  ///< ETrue if the object is initialised. It can be done only once.
            TBool       isLegalDosName :1;  ///< ETrue if iTargetName is a legal DOS name
            TShortName  iShortName;         ///< a short DOS name in XXXXXXXXYYY format generated from aTargetName
        };


    	/** 
	   	An ad hoc internal helper object for entry creations 
	    */
	    class XFileCreationHelper
	    {
	    public:
	    	XFileCreationHelper();
	    	~XFileCreationHelper();
	    	void Close();
	    	void InitialiseL(const TDesC&  aTargetName);
	        TInt GetValidatedShortName(TShortName& aShortName) const;
	    	void CheckShortNameCandidates(const TUint8* apDosEntryName);
	
	        // inline functions for sets and gets
	        //  note all the get functions have been checked against initialisation status
	    	inline TBool 	IsInitialised() const;
	        inline TUint16	NumOfAddingEntries() const;
	        inline TEntryPos EntryAddingPos()const;
	        inline TBool 	IsNewEntryPosFound() const;
	    	inline TBool 	IsTrgNameLegalDosName() const;
	
	    	inline void	SetEntryAddingPos(const TEntryPos& aEntryPos);
	    	inline void	SetIsNewEntryPosFound(TBool aFound);
	
	    private:
	    	XFileCreationHelper(const XFileCreationHelper&);
	    	XFileCreationHelper& operator=(const TFindHelper&);
	
	    private:
	        TPtrC       iTargetName;		///< pointer to hold the long file name of the target file
	        TUint16		iNumOfAddingEntries;///< calculated number of entries to add
	        TEntryPos	iEntryAddingPos;	///< contains new entry position for adding if found any
	        TBool		isNewEntryPosFound; ///< flags whether the position for new entries is found
	        TBool       isInitialised	:1;	///< flags whether the object is initialised
	        TBool       isTrgNameLegalDosName	:1;	///< flags whether the target file name is a valid Dos name
	        /**
	        an array that holds short name candidates, prepared on initialisation.
	        */
	        RArray<TShortName>  iShortNameCandidates;
	    };

   
    /** a helper class that describes a continuous chunk of diectory entries*/
    class TEntrySetChunkInfo
        {
     public:
        inline TEntrySetChunkInfo();
        inline TBool operator==(const TEntrySetChunkInfo& aRhs);

        //-- FAT entryset can't span more than 3 clusters/sectors
        enum {KMaxChunks = 3};

     public:   
        TEntryPos iEntryPos;    ///< entryset chunk dir. starting position
        TUint     iNumEntries;  ///< number of entries in the chunk
        };

    
    void DoEraseEntrySetChunkL(const TEntrySetChunkInfo& aEntrySetChunk);
	

	TBool DoRummageDirCacheL(TUint anAtt,TEntryPos& aStartEntryPos,TFatDirEntry& aStartEntry,TEntryPos& aDosEntryPos,TFatDirEntry& aDosEntry,TDes& aFileName, const TFindHelper& aAuxParam, XFileCreationHelper* aFileCreationHelper, const TLeafDirData& aLeafDir) const;
    TBool DoFindL(const TDesC& aName,TUint anAtt,TEntryPos& aStartEntryPos,TFatDirEntry& aStartEntry,TEntryPos& aDosEntryPos,TFatDirEntry& aDosEntry,TDes& aFileName,TInt anError, XFileCreationHelper* aFileCreationHelper, const TLeafDirData& aLeafDirData) const;
    void FindEntryStartL(const TDesC& aName,TUint anAtt,TFatDirEntry& anEntry,TEntryPos& aPos, XFileCreationHelper* aFileCreationHelper) const;

    void FindEntryStartL(const TDesC& aName,TUint anAtt,TFatDirEntry& anEntry,TEntryPos& aPos) const;

	void CheckFatForLoopsL(const TFatDirEntry& anEntry) const;
	void DoCheckFatForLoopsL(TUint32 aCluster, TUint32& aPreviousCluster, TUint32& aChangePreviousCluster, TUint32& aCount) const;
    void InitializeL(const TLocalDriveCaps& aLocDrvCaps, TBool aIgnoreFSInfo=EFalse);

	void DoReadFromClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2& aMessage,TInt anOffset, TUint aFlag) const;
    void DoWriteToClusterListL(TEntryPos& aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2& aMessage,TInt anOffset, TUint aLastcluster, TUint& aBadcluster, TUint& aGoodcluster, TUint aFlag);

	TBool IsUniqueNameL(const TShortName& aName, TUint32 aDirCluster);
	TBool FindShortNameL(const TShortName& aName,TEntryPos& anEntryPos);
	void ReplaceClashingNameL(const TShortName& aNewName,const TEntryPos& anEntryPos);
    TBool GenerateShortNameL(TUint32 aDirCluster,const TDesC& aLongName,TShortName& aShortName, TBool aForceRandomize=EFalse);
    TInt FindLeafDirL(const TDesC& aName, TLeafDirData& aLeafDir) const;
	
	TInt GetDirEntry(TEntryPos& aPos,TFatDirEntry& aDosEntry,TFatDirEntry& aStartEntry,TDes& aLongFileName) const;
    TBool DoGetDirEntryL(TEntryPos& aPos,TFatDirEntry& aDosEntry,TFatDirEntry& aStartEntry,TDes& aLongFileName) const;
    
	void WriteDirEntryL(TEntryPos& aPos,const TFatDirEntry& aFatDirEntry,const TDesC& aLongFileName);
	void EraseDirEntryL(TEntryPos aPos,const TFatDirEntry& anEntry);
	void EraseDirEntryL(const TEntryPos& aPos);
	void InitializeFirstDirClusterL(TUint32 aCluster, TUint32 aParentCluster);
	void AddDirEntryL(TEntryPos& aPos,TInt aNameLength);
	void ZeroDirClusterL(TUint32 aCluster);
	
    TInt DoWriteBootSector(TInt64 aMediaPos, const TFatBootSector& aBootSector) const;
	TInt DoReadBootSector(TInt64 aMediaPos, TFatBootSector& aBootSector) const;
    TInt ReadBootSector(TFatBootSector& aBootSector, TBool aDoNotReadBkBootSec=EFalse);

    TInt WriteFSInfoSector(TInt64 aMediaPos, const TFSInfo& aFSInfo) const;
	TInt ReadFSInfoSector(TInt64 aMediaPos, TFSInfo& aFSInfo) const;

    TBool IsDirectoryEmptyL(TUint32 aCluster);
	void ExtendClusterListZeroedL(TUint32 aNumber, TUint32& aCluster);
	void WritePasswordData();
	
    void WriteVolumeLabelL(const TDesC8& aVolumeLabel) const;
    TInt ReadVolumeLabelFile(TDes8& aLabel);
	void WriteVolumeLabelFileL(const TDesC8& aNewName);
	void FindVolumeLabelFileL(TDes8& aLabel, TEntryPos& aDosEntryPos, TFatDirEntry& aDosEntry);
	void GetVolumeLabelFromDiskL(const TFatBootSector& aBootSector);
	void TrimVolumeLabel(TDes8& aLabel) const;

    TInt    DoRunScanDrive();
    TBool   VolumeCleanL();
    void    SetVolumeCleanL(TBool aClean);
    TBool   VolCleanFlagSupported() const;

    void    DoUpdateFSInfoSectorsL(TBool aInvalidateFSInfo);
    void    UnFinaliseMountL();
    void    DoReMountL();
    void    SetFatType(TFatType aFatType);

    TUint64 VolumeSizeInBytes() const;

private:
	

	CFatMountCB();

	TInt GetDosEntryFromNameL(const TDesC& aName, TEntryPos& aDosEntryPos, TFatDirEntry& aDosEntry);

    TInt MntCtl_DoCheckFileSystemMountable();


private:

    TBool iReadOnly         : 1;///< if true, the drive is in read-only mode 
    TBool iRamDrive         : 1;///< true if this is a RAM drive    
    TBool iMainBootSecValid : 1;///< true if the main boot sector is valid, if false, a backup boot sector may be in use. 

    TFatMntState iState;            ///< this mounnt internal state

    TFatType iFatType;              ///< FAT type, FAT12,16 or 32
    TUint32  iFatEocCode;           ///< End Of Cluster Chain code, 0xff8 for FAT12, 0xfff8 for FAT16, and 0xffffff8 for FAT32 

    CLeafDirCache* iLeafDirCache;	///< A cache for most recently visited directories, only valid when limit is set bigger than 1

	TFatVolParam iVolParam;         ///< FAT volume parameters, populated form the boot sector values.
    
	TUint32 iFirstFreeByte;         ///< First free byte in media (start of the data area on the volume)
	TUint32 iUsableClusters;        ///< Number of usable cluster on the volume 
	
    CFatTable* iFatTable;           ///< Pointer to the volume Fat 
	CRawDisk*  iRawDisk;            ///< Pointer to the raw data interface class
	
    CAsyncNotifier* iNotifier;      ///< Async notifier for notifying user of Fat error conditions 

    XDriveInterface iDriverInterface; ///< the object representing interface to the drive, provides read/write access and notifiers
	TFatConfig      iFatConfig;       ///< FAT parametrers from estart.txt

	XFileCreationHelper iFileCreationHelper;


#ifdef  _DEBUG
    private:
    //-- debug odds and ends
    inline TBool IsWriteFail()const;
	inline void SetWriteFail(TBool aIsWriteFail);
	inline TInt WriteFailCount()const;
	inline void SetWriteFailCount(TInt aFailCount);
	inline void DecWriteFailCount();
	inline TInt WriteFailError()const;
	inline void SetWriteFailError(TInt aErrorValue);


	TBool   iIsWriteFail : 1; ///< Flag to indicate if write failed used for debugging
    TBool   iCBRecFlag   : 1; ///< in debug mode used for checking unwanted recursion

    TInt    iWriteFailCount;  ///< Write fail count for debug
	TInt    iWriteFailError;  ///< Write fail error to use for debug

#endif

friend class CFatFormatCB;
friend class CScanDrive;
friend class TDriveInterface;
	};


//---------------------------------------------------------------------------------------------------------------------------------

/**
Fat file system file subsession implmentation, provides all that is required of a plug in
file system file as well as Fat specific functionality
*/
class CFatFileCB : public CFileCB, public CFileCB::MBlockMapInterface, public CFileCB::MExtendedFileInterface
	{
public:
	CFatFileCB();
	~CFatFileCB();
public:
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aMask,TUint aVal);
	void FlushDataL();
	void FlushAllL();
public:
	void CheckPosL(TUint aPos);
	void SetupL(const TFatDirEntry& aFatDirEntry, const TEntryPos& aFileDosEntryPos);
	
	// from MBlockMapInterface
	TInt BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos);

	// from CFileCB
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

	// from CFileCB::MExtendedFileInterface
	virtual void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void SetSizeL(TInt64 aSize);

private:
	inline CFatMountCB& FatMount() const;
    inline CFatTable& FAT();
	inline TInt ClusterSizeLog2();
	inline TInt ClusterRelativePos(TInt aPos);


    void FlushStartClusterL();
	TInt SeekToPosition(TUint aNewCluster, TUint aClusterOffset);
	void SetSeekIndexValueL(TUint aFileCluster,TUint aStoredCluster);
	void ResizeIndex(TInt aNewMult,TUint aNewSize);
	TInt CalcSeekIndexSize(TUint aSize);
	void ClearIndex(TUint aNewSize);
	void DoSetSizeL(TUint aSize, TBool aForceCachesFlush);
	void WriteFileSizeL(TUint aSize);

    //----------------------------
    inline TUint32 FCB_StartCluster() const;
    inline TUint32 FCB_FileSize() const; 

    inline void  FCB_SetStartCluster(TUint32 aVal);
    inline void  FCB_SetFileSize(TUint32 aVal);  

    inline TBool FileAttModified() const;
    inline void  IndicateFileAttModified(TBool aModified);
    
    inline TBool FileSizeModified() const;
    inline void  IndicateFileSizeModified(TBool aModified);

    inline TBool FileTimeModified() const;
    inline void  IndicateFileTimeModified(TBool aModified);

    //----------------------------

	void CreateSeekIndex();

    void DoShrinkFileToZeroSizeL();
    void DoShrinkFileL(TUint32  aNewSize, TBool aForceCachesFlush);
    void DoExpandFileL(TUint32 aNewSize, TBool aForceCachesFlush);


private:

	TUint32* iSeekIndex;    ///< Seek index into file
	TInt iSeekIndexSize;    ///< size of seek index
	
    TUint     iStartCluster;     ///< Start cluster number of file
	TEntryPos iCurrentPos;  ///< Current position in file data
	
    TEntryPos iFileDosEntryPos;	///< File DOS dir. entry position
	
    TBool iFileSizeModified :1;	///< flag, indicating that file size was modified and needs to be flushed onto the media (see FlushL())
    TBool iFileTimeModified :1;	///< flag, indicating that file modification time was modified and needs to be flushed onto the media (see FlushL())

	};



//---------------------------------------------------------------------------------------------------------------------------------
/**
Fat file system directory subsession implmentation, provides all that is required of a plug in
file system directory as well as Fat specific functionality
*/
class CFatDirCB : public CDirCB
	{
public:
	static CFatDirCB* NewL();
	~CFatDirCB();
public:
	void ReadL(TEntry& anEntry);
	void StoreLongEntryNameL(const TDesC& aName);
public:
	void SetDirL(const TFatDirEntry& anEntry,const TDesC& aMatchName);
	inline CFatMountCB& FatMount();
private:
	CFatDirCB();
private:
	TFatDirEntry iEntry;       ///< Current directory entry in this directory
	TEntryPos    iCurrentPos;  ///< Current position in directory
	HBufC*       iMatch;       ///< Current name being searched in directory (Dos Name)
	HBufC*       iLongNameBuf; ///< Long name storage	
	TBool        iMatchUid;    ///< Flag to indicate if UID matches
	};

//---------------------------------------------------------------------------------------------------------------------------------
/**
    FAT Format Control Block class, responsible for FAT volumes formatting
*/
class CFatFormatCB : public CFormatCB
	{
public:
	CFatFormatCB();
	~CFatFormatCB();
public:
	
    //-- overrides from CFormatCB
    void DoFormatStepL();

private:
    //-- overrides from CFormatCB
    TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

private:	
    
    TInt DoProcessTVolFormatParam(const TVolFormatParam_FAT* apVolFormatParam);

    void CreateBootSectorL();
	void CreateFSInfoSectorL();
	void CreateReservedBootSectorL();
	void InitializeFormatDataL();
	void DoZeroFillMediaL(TInt64 aStartPos, TInt64 aEndPos);

    TInt ProcessVolParam_User(const TLocalDriveCapsV6& aCaps);
    TInt ProcessVolParam_Custom(const TLocalDriveCapsV6& aCaps);
    TInt ProcessVolParam_Default(const TLocalDriveCapsV6& aCaps);
    TInt ProcessVolParam_RamDisk(const TLocalDriveCapsV6& aCaps);
    
	void AdjustClusterSize(TUint aRecommendedSectorsPerCluster);
	TInt AdjustFirstDataSectorAlignment(TUint aBlockSize);
	TInt FirstDataSector() const;

	TInt HandleCorrupt(TInt aError);
	TInt BadSectorToCluster();
    void TranslateL();
    TInt DoTranslate(TPtr8& aBuf, RArray<TInt>& aArray);
    void RecordOldInfoL();
	
    TUint MaxFat12Sectors() const;
	TUint MaxFat16Sectors() const;
	TUint MaxFat32Sectors() const;
	
	inline CFatMountCB& FatMount();
	inline CProxyDrive* LocalDrive();
    TFatType SuggestFatType() const;

    
    inline TBool FatTypeValid() const;
    inline TFatType FatType() const;
    inline void SetFatType(TFatType aType);

    inline TBool Is16BitFat() const;
    inline TBool Is32BitFat() const;


private:
	
    TBool       iVariableSize;      ///< Flag to indicate if we are dealing with a variable size volume (RAM drive)
	
    TUint16 iBytesPerSector;    ///< Byte per sector of media
    TInt    iSectorSizeLog2;    ///< Sector size in log2
	TUint8  iNumberOfFats;      ///< Number of Fats the volume will contain
	TUint   iReservedSectors;   ///< Number of reserved sectors in the volume
	TUint16 iRootDirEntries;    ///< Nummer of root directory entries the root dir will have, specific to Fat12/16 volumes
	TUint   iSectorsPerCluster; ///< Sector per cluster ration the volume will be formatted with
	TUint   iSectorsPerFat;     ///< Number of sectors the Fat uses
	TUint32 iMaxDiskSectors;    ///< number of sectors the volume has
	TFormatInfo iFormatInfo;    ///< format information for a custom format
    TFatType    iFatType;           ///< FAT type

	TInt    iHiddenSectors;     ///< Number of hidden sectors in the volume
	TUint16 iNumberOfHeads;     ///< Number of heads the media device has, not used so far as only used on solid state media.
	TUint16 iSectorsPerTrack;   ///< Number of sectors the media device has, not used so far as only used on solid state media.
	TUint32 iRootClusterNum;    ///< cluster number used for root directory, Fat32 specific
	TUint32 iCountOfClusters;   ///< Count of clusters on the media
    
    RArray<TInt> iBadClusters;  ///< Array of bad cluster numbers
    RArray<TInt> iBadSectors;   ///< Array of bad sector numbers
    TBool   iDiskCorrupt;       ///< Disk is corrupt when format or not
    TInt    iOldFirstFreeSector;
    TInt    iOldSectorsPerCluster;
	};

/**
Required file system class used by file server to create the file system objects
*/
class CFatFileSystem : public CFileSystem
	{
public:
	static CFatFileSystem* New();
	~CFatFileSystem();
public:
	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
	TInt DefaultPath(TDes& aPath) const;
	TBool IsExtensionSupported() const;
	TBool GetUseLocalTime() const;
	void SetUseLocalTime(TBool aFlag);
	TInt GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput);
protected:
	CFatFileSystem();
	/**
	If true, then local time will be used when writing timestamps to FS. When reading,
	timestamps will be assumed local and converted back to UTC.
	At present, this behaviour will also be conditional upon a particular drive being logically removable.
	*/
	TBool iUseLocalTimeIfRemovable;
	};



/**
Locale utilities allows the file system to call into a specific locale for tasks
such as Dos name to unicode conversions and testing the legality of characters for
any given locale.
*/
class LocaleUtils

	{
public:
	static void ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TFatUtilityFunctions::TOverflowAction aOverflowAction=TFatUtilityFunctions::EOverflowActionTruncate);
	static void ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TFatUtilityFunctions::TOverflowAction aOverflowAction=TFatUtilityFunctions::EOverflowActionTruncate);
	static TBool IsLegalShortNameCharacter(TUint aCharacter,TBool aUseExtendedChars=EFalse);
	};
//

/**
Converts Dos time (from a directory entry) to TTime format

@param aDosTime Dos format time
@param aDosDate Dos format Date
@return TTime format of Dos time passed in 
*/
TTime DosTimeToTTime(TInt aDosTime,TInt aDosDate);
/**
Converts TTime format to Dos time format

@param aTime TTime to convert to Dos time
@return Dos time format
*/
TInt DosTimeFromTTime(const TTime& aTime);
/**
Converts TTime format to Dos time format

@param aTime TTime to convert to Dos Date
@return Dos Date format
*/
TInt DosDateFromTTime(const TTime& aTime);
/**
Converts Dos Directory entry format to 8.3 format

@param aDosName Directory entry format with space delimeter
@return 8.3 Dos filename format
*/
TBuf8<12> DosNameToStdFormat(const TDesC8& aDosName);
/**
Converts 8.3 format to Dos Directory entry format 

@param aStdFormatName 8.3 Dos filename format
@return Directory entry format with space delimeter
*/
TBuf8<12> DosNameFromStdFormat(const TDesC8& aStdFormatName);
/**
Fault function calls user panic with a fault reason

@param Enumerated fault reason
*/
void Fault(TFault aFault);
/**
calculates the number of VFat directory entries for a given file/directory name length

@param the length in characters of the name
@return the number of VFat entries required
*/
TUint NumberOfVFatEntries(TUint aNameLength);
/**
Calculates the check sum for a standard directory entry

@param the Dos name for the directory entry
@return the checksum
*/
TUint8 CalculateShortNameCheckSum(const TDesC8& aShortName);

TUint32 EocCodeByFatType(TFatType aFatType);


#include "sl_std.inl"
#include "sl_bpb.inl"
#include "fat_dir_entry.inl"
              
#endif //SL_STD_H
