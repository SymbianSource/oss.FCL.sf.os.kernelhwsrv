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
// f32\sfat32\inc\sl_std.inl
// 
//

#ifndef SL_STD_INL
#define SL_STD_INL



//---------------------------------------------------------------------------------------------------------------------------------
// class TEntryPos
TUint32 TEntryPos::Cluster() const 
    {
    return (TUint32) iCluster;
    }

TUint32 TEntryPos::Pos() const 
    {
    return (TUint32) iPos;
    }

TBool TEntryPos::operator==(const TEntryPos& aRhs) const
    {
    ASSERT(this != &aRhs);
    return (iCluster == aRhs.iCluster && iPos == aRhs.iPos);
    }

/** set "end of directory" indicator*/
void TEntryPos::SetEndOfDir()
    {
    iCluster = EOF_32Bit;
    }


//---------------------------------------------------------------------------------------------------------------------------------
// class CFatMountCB


/** @return ETrue if the value of aClusterNo is valid*/
inline TBool CFatMountCB::ClusterNumberValid(TUint32 aClusterNo) const 
    {   
    ASSERT(ConsistentState()); 
            
    if(!aClusterNo && !Is32BitFat())
        return ETrue; //-- root dir. cluster for FAT12/16

    return (aClusterNo >= KFatFirstSearchCluster) && (aClusterNo < UsableClusters()+KFatFirstSearchCluster); 
    }


inline TUint32 CFatMountCB::RootDirectorySector() const
    {
    return iVolParam.RootDirectorySector();
    }

inline TUint32 CFatMountCB::RootDirEnd() const
    {
    return iVolParam.RootDirEnd();
    }

inline TUint32 CFatMountCB::RootClusterNum() const
    {
    return iVolParam.RootClusterNum();
    }        


inline TUint32 CFatMountCB::StartCluster(const TFatDirEntry & anEntry) const
	{
	if(Is32BitFat())	
		return anEntry.StartCluster();
	else
		return anEntry.StartCluster() & 0xFFFF;
	}

/**
returns true for root dir on Fat12/16 (fixed root dir versions of Fat) false on fat32 
this function is used to handle special cases for reading/writing the root directory on FAT via the use of cluster zero.

    @param entryPos     directory entry position
    @return ETrue       if entryPos belongs to the FAT12/16 root directory
*/
TBool CFatMountCB::IsRootDir(const TEntryPos &entryPos) const
	{
	//-- for FAT12/16 cluster 0 means "root directory"
    return !(Is32BitFat() || entryPos.iCluster);
	}
/**
Indicates the root directory cluster, For Fat12/16 root is always indicated by cluster number zero, on Fat32 the is a root cluster number
@return The root cluster indicator
*/
TUint32 CFatMountCB::RootIndicator() const
	{
	if(Is32BitFat())
        return iVolParam.RootClusterNum();
	else
		return 0;
	}


/** @return Log2 of cluster size on volume */
TUint32 CFatMountCB::ClusterSizeLog2() const
    {
    return(iVolParam.ClusterSizeLog2());
    }

/** @return Log2 of media sector size  */
TUint32 CFatMountCB::SectorSizeLog2() const
    {
    return(iVolParam.SectorSizeLog2());
    }

/** @return sector per cluster */
TUint32 CFatMountCB::SectorsPerCluster() const
    {
    return(1<<(iVolParam.ClusterSizeLog2()-iVolParam.SectorSizeLog2()));
    }

/** @return the base position of a cluster */
TUint32 CFatMountCB::ClusterBasePosition() const
	{
    return(iFirstFreeByte);
    }

/** @return the offset into a cluster of a byte address */
TUint32 CFatMountCB::ClusterRelativePos(TUint32 aPos) const
	{
    return(aPos & ((1<<ClusterSizeLog2())-1));
    }

/**
Calculates the maximum number of clusters
@return  maximum number of clusters
*/
TUint32 CFatMountCB::MaxClusterNumber() const
    {return(TotalSectors()>>(ClusterSizeLog2()-SectorSizeLog2()));}

/** @return the the total sectors on volume */
TUint32 CFatMountCB::TotalSectors() const
    {
    return iVolParam.TotalSectors();
    }

/** @return total size of a Fat in bytes */
TUint32 CFatMountCB::FatSizeInBytes() const
    {
    return iVolParam.FatSizeInBytes();
    }

/** @return first sector of the Fat */
TUint32 CFatMountCB::FirstFatSector() const
    {
    return iVolParam.FirstFatSector();
    }

/** @return the byte offset of the Fat */
TUint32 CFatMountCB::StartOfFatInBytes() const
	{
    return(FirstFatSector()<<SectorSizeLog2());
    }

/** @return Number of Fats used by the volume */
TUint32 CFatMountCB::NumberOfFats() const
    {
    return iVolParam.NumberOfFats();
    }


/** @return reference to the fat table owned by the mount */
CFatTable& CFatMountCB::FAT() const
	{
    return(*iFatTable);
    }

/**
    @return reference to the file system object that has produced this CFatMountCB
*/
CFatFileSystem& CFatMountCB::FatFileSystem() const
	{
    return (CFatFileSystem&)(*FileSystem()); //-- CMountCB::FileSystem() provides correct answer
    }


/** @return  refrence to a raw disk object owned by the mount */
CRawDisk& CFatMountCB::RawDisk() const
	{return(*iRawDisk);}

/**
@return ETrue if aCluster value is bad cluster marker defined in FAT specification
*/
TBool CFatMountCB::IsBadCluster(TUint32 aCluster) const
	{
    return Is32BitFat() ? aCluster==0xFFFFFF7 : Is16BitFat() ? aCluster==0xFFF7 : aCluster==0xFF7;
    }

/**
Returns whether the current mount is running as rugged Fat or not, this is held in the file system object
    @return ETrue if this is Rugged FAT
*/
TBool CFatMountCB::IsRuggedFSys() const
	{
    return Drive().IsRugged();
    }

/**
Sets the rugged flag in the file system object
@param Flag to set or clear the rugged flag
*/
void CFatMountCB::SetRuggedFSys(TBool aVal)
	{
    Drive().SetRugged(aVal);
    }

/**
    @return Log2(Media atomic write granularity).
    This is mostly to be used in Rugged FAT mode, see IsRuggedFSys(). For Rugged FAT the media shall support atomic writes.
    By default this is the sector (512 bytes)

*/
TUint32 CFatMountCB::AtomicWriteGranularityLog2() const
    {
    return KDefSectorSzLog2;    
    }


/** @return the usable clusters count for a volume */
TUint32 CFatMountCB::UsableClusters() const
    {
    return(iUsableClusters);
    }


TUint32 CFatMountCB::StartOfRootDirInBytes() const
    {
    return iVolParam.RootDirectorySector()<<SectorSizeLog2();
    }


/** @return FAT type for this mount */
TFatType CFatMountCB::FatType() const
{
    return iFatType;
}

/** @return ETrue if the mount has 16bit FAT */
TBool CFatMountCB::Is16BitFat() const
{
    return FatType() == EFat16;
} 

/** @return ETrue if the mount has 32bit FAT */
TBool CFatMountCB::Is32BitFat() const
{   
    return FatType() == EFat32;
}

CAsyncNotifier* CFatMountCB::Notifier() const
	{
    return iNotifier;
    }	



/**
    Set or reset Read Only mode for the mount.
    @param    aReadOnlyMode if ETrue, the mount will be set RO.
*/
void  CFatMountCB::SetReadOnly(TBool aReadOnlyMode) 
    {
    iReadOnly = aReadOnlyMode;
    }


/**
    @return ETrue if the volume is in Read-Only state
*/
TBool CFatMountCB::ReadOnly(void) const
    {
    return iReadOnly;
    }

/** @return state of the CFatMountCB*/
CFatMountCB::TFatMntState CFatMountCB::State() const 
    {
    return iState;
    }

/** 
    Set state of the CFatMountCB
    @param  aState state to set
*/
void CFatMountCB::SetState(TFatMntState aState)
    {
    __PRINT3(_L("#- CFatMountCB::SetState() drv:%d, %d->%d\n"),DriveNumber(),iState,aState);
    iState = aState;
    }


TDriveInterface& CFatMountCB::DriveInterface() const 
    {
    return (TDriveInterface&)iDriverInterface; 
    }

const TFatConfig& CFatMountCB::FatConfig() const 
    {
    return iFatConfig;
    }

//---------------------------------------------------------------------------------------------------------------------------------
/** 
Check if the XFileCreationHelper object is initialised.
*/
TBool CFatMountCB::XFileCreationHelper::IsInitialised() const 
	{
	return isInitialised;
	}

/** 
Get number of new entries for file creation.
*/
TUint16	CFatMountCB::XFileCreationHelper::NumOfAddingEntries() const
	{
	ASSERT(isInitialised); 
	return iNumOfAddingEntries;
	}

/** 
Get position of new entries for file creation.
*/
TEntryPos CFatMountCB::XFileCreationHelper::EntryAddingPos() const 
	{
	ASSERT(isInitialised); 
	return iEntryAddingPos;
	}

/** 
Check if position of new entries has been found.
*/
TBool CFatMountCB::XFileCreationHelper::IsNewEntryPosFound() const 
	{
	ASSERT(isInitialised); 
	return isNewEntryPosFound;
	}

/** 
Check if file name of the new file is a legal dos name.
*/
TBool CFatMountCB::XFileCreationHelper::IsTrgNameLegalDosName() const
	{
	ASSERT(isInitialised); 
	return isTrgNameLegalDosName;
	}

/** 
Set entry position for new entries to be added.
*/
void CFatMountCB::XFileCreationHelper::SetEntryAddingPos(const TEntryPos& aEntryPos) 
	{
	iEntryAddingPos = aEntryPos;
	}

/** 
Set condition if position of new entries has been found.
*/
void CFatMountCB::XFileCreationHelper::SetIsNewEntryPosFound(TBool aFound) 
	{
	isNewEntryPosFound = aFound;
	}


/**
    Checks for "EOC" for all Fat types
    @param  aCluster FAT table entry (cluster number) to check
    @return ETrue    if aCluster is a EOC for the FAT type being used by CFatMountCB
*/
TBool CFatMountCB::IsEndOfClusterCh(TUint32 aCluster) const
	{
    ASSERT(iFatEocCode);
    ASSERT((TUint32)aCluster <= iFatEocCode+7); //-- aCluster value is always masked accordingly.

    return (aCluster >= iFatEocCode);
    }

/**
    Sets "End of Cluster Chain" value in aCluster depending on the FAT type.
    @param aCluster cluster to set to end of chain marker
*/
void CFatMountCB::SetEndOfClusterCh(TUint32 &aCluster) const
	{
    ASSERT(iFatEocCode);
    aCluster = iFatEocCode+7;
	}


CFatMountCB::TEntrySetChunkInfo::TEntrySetChunkInfo()
                                :iNumEntries(KMaxTUint) 
    {
    }


TBool CFatMountCB::TEntrySetChunkInfo::operator==(const TEntrySetChunkInfo& aRhs)
    {
    ASSERT(&aRhs != this);
    return (iNumEntries == aRhs.iNumEntries) && (iEntryPos==aRhs.iEntryPos);
    }




//-------  debug methods
#ifdef  _DEBUG
/**
Debug function indicates whether write fails are active or not, for test
@return ETrue if write fails on or not
*/
TBool CFatMountCB::IsWriteFail()const
	{return(iIsWriteFail);}
/**
Switches write fails on or off, for test
@param aIsWriteFail set true or false to set write fails on or off
*/
void CFatMountCB::SetWriteFail(TBool aIsWriteFail)
	{iIsWriteFail=aIsWriteFail;}

/** @return number of write fails to occur, for test */
TInt CFatMountCB::WriteFailCount()const
	{return(iWriteFailCount);}

/**
Set the number of Write fails 
@param aFailCount number of write fails, for test
*/
void CFatMountCB::SetWriteFailCount(TInt aFailCount)
	{iWriteFailCount=aFailCount;}

/** Decrement the number of write fails, for test */
void CFatMountCB::DecWriteFailCount()
	{--iWriteFailCount;}

/** @return Error for a write failure, for test */
TInt CFatMountCB::WriteFailError()const
	{return iWriteFailError;}

/**
Set the write fail error code, for test
@param aErrorValue The Error for a write fails
*/
void CFatMountCB::SetWriteFailError(TInt aErrorValue)
	{iWriteFailError=aErrorValue;}

#endif





//---------------------------------------------------------------------------------------------------------------------------------
// class CFatFormatCB

/** @return pointer to the owning mount object */
CFatMountCB& CFatFormatCB::FatMount()
    {
    return *(CFatMountCB*)&Mount();
    }

/**
Returns the local drive used by the file systems from the owning mount
@return Pointer to the local drive 
*/
CProxyDrive* CFatFormatCB::LocalDrive()
	{
    return(FatMount().LocalDrive());
    }


TBool CFatFormatCB::FatTypeValid() const  
    {
    return (iFatType == EFat12 || iFatType == EFat16 || iFatType == EFat32);
    }
    
TFatType CFatFormatCB::FatType() const
    {
    ASSERT(FatTypeValid()); 
    return iFatType;
    }

void CFatFormatCB::SetFatType(TFatType aType) 
    {
    ASSERT(aType != EInvalid); 
    iFatType = aType;
    }

TBool CFatFormatCB::Is16BitFat() const
    {
    ASSERT(FatTypeValid()); 
    return iFatType == EFat16;
    }

TBool CFatFormatCB::Is32BitFat() const  
    {
    ASSERT(FatTypeValid());
    return iFatType == EFat32;
    }


//---------------------------------------------------------------------------------------------------------------------------------
// class CFatFileCB

/**
Returns the owning mount from file object

@return pointer to the owning mount object
*/
CFatMountCB& CFatFileCB::FatMount() const
	{return((CFatMountCB&)Mount());}

/**
Returns the fat table used by the file system for this mount

@return Reference to the Fat table owned by the mount
*/
CFatTable& CFatFileCB::FAT()
	{return(FatMount().FAT());}

/**
Position with in a cluster for a given address

@param aPos Byte position 
*/
TInt CFatFileCB::ClusterRelativePos(TInt aPos)
	{return(FatMount().ClusterRelativePos(aPos));}
/**
Returns Log2 of cluster size from mount

@return cluster size
*/
TInt CFatFileCB::ClusterSizeLog2()
	{return(FatMount().ClusterSizeLog2());}


//---------------------------------------------------------------------------------------------------------------------------------
TBool CFatFileCB::FileSizeModified() const 
    {
    return iFileSizeModified;
    }  

void CFatFileCB::IndicateFileSizeModified(TBool aModified) 
	{
	iFileSizeModified = aModified;
	}

//-----------------------------------------------------------------------------
/** @return ETrue if file attributes' 'Modified' flag is set*/
TBool CFatFileCB::FileAttModified() const 
    {
    return (Att() & KEntryAttModified);
    }   

/** 
    Set or reset a flag indicating that file attributes had beed modified
    @param aModified ETrue means that attributes are modified
*/

void  CFatFileCB::IndicateFileAttModified(TBool aModified)
    {
    if(aModified)
        iAtt |= KEntryAttModified;
    else
        iAtt &= ~KEntryAttModified;
    }

TUint32 CFatFileCB::FCB_StartCluster() const
    {
    return iStartCluster;
    }


void CFatFileCB::FCB_SetStartCluster(TUint32 aVal)
    {
    ASSERT(aVal == 0 || (aVal >= KFatFirstSearchCluster));
    iStartCluster = aVal;
    }

/** @return file size from CFileCB */
TUint32 CFatFileCB::FCB_FileSize() const
    {
    return Size();
    } 

/** set file size in the CFileCB*/
void CFatFileCB::FCB_SetFileSize(TUint32 aVal)
    {
    SetSize(aVal);
    } 

TBool CFatFileCB::FileTimeModified() const 
    {
    return iFileTimeModified;
    }

void  CFatFileCB::IndicateFileTimeModified(TBool aModified)
    {
    iFileTimeModified = aModified;
    }



//---------------------------------------------------------------------------------------------------------------------------------
// class CFatDirCB

/**
Returns the owning mount from directory object

@return pointer to the owning mount object
*/
CFatMountCB& CFatDirCB::FatMount()
	{return((CFatMountCB&)Mount());}



//---------------------------------------------------------------------------------------------------------------------------------
// class CFatTable

TUint32 CFatTable::FreeClusters() const 
    {
    return iFreeClusters;
    }


//---------------------------------------------------------------------------------------------------------------------------------

inline TFatType CFatTable::FatType() const 
    {
    return iFatType;
    }

inline TBool CFatTable::IsFat12() const
    {
    return iFatType == EFat12;
    }

inline TBool CFatTable::IsFat16() const
    {
    return iFatType == EFat16;
    }

inline TBool CFatTable::IsFat32() const
    {
    return iFatType == EFat32;
    }


/**
    Checks for "EOC" for all Fat types
    @param  aCluster FAT table entry (cluster number) to check
    @return ETrue    if aCluster is a EOC for the FAT type being used by CFatMountCB that owns the CFatTable
*/
inline TBool CFatTable::IsEndOfClusterCh(TUint32 aCluster) const
    {
    ASSERT(iFatEocCode);

    if(aCluster >= iFatEocCode)
        return ETrue;

    ASSERT((TUint32)aCluster <= iFatEocCode+7);
	return EFalse;
    }


//-----------------------------------------------------------------------------
/**
    @return ETrue if the cluster number aClusterNo is valid, i.e. belongs to the FAT table
*/
inline TBool CFatTable::ClusterNumberValid(TUint32 aClusterNo) const 
    {
    return (aClusterNo >= KFatFirstSearchCluster) && (aClusterNo < iMaxEntries); 
    }


/**
@return Maximal number of addresable FAT entries. This value is taken from the owning mount
*/
inline TUint32 CFatTable::MaxEntries() const
    {
        ASSERT(iMaxEntries > 0);
        return iMaxEntries;
    }


// class TDriveInterface
TBool TDriveInterface::NotifyUser() const
	{return(iMount->GetNotifyUser());}


//----------------------------------------------------------------------------------------------------
// class CRawDisk

/**
    Get pointer to the directory cache interface. Any client that reads/writes directory entries 
    MUST do it via this interface.
    Default implementation returns NULL

    @return     pointer to the MWTCacheInterface interface, or NULL if it is not present.
*/
MWTCacheInterface* CRawDisk::DirCacheInterface()
    {
    return NULL;
    }

//---------------------------------------------------------------------------------------------------------------------------------	

/**
    Calculate offset of the page starting position in the cluster 
    @param aPos  the current entry position in bytes in the cluster
    @param aPageSzLog2	page size in log2
    @return		the starting position of the page that contains aPos
*/
inline TUint32 CalculatePageOffsetInCluster(TUint32 aPos, TUint aPageSzLog2)
	{
	return (aPos >> aPageSzLog2) << aPageSzLog2;
	}

#endif //SL_STD_INL





