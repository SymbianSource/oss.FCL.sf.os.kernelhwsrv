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
// f32\sfat\inc\fat_dir_entry.inl
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(FAT_DIR_ENTRY_INL)
#define FAT_DIR_ENTRY_INL

//-------------------------------------------------------------------------------------------------------------------


/**
Defined cast of Fat directory entry data read to structure allowing access to data
*/
#define pDir ((SFatDirEntry*)&iData[0])


inline TFatDirEntry::TFatDirEntry() 
    {
    InitZ();
    }       

/** zero-fill the entry contents  */
inline void TFatDirEntry::InitZ() 
    {
    Mem::FillZ(iData, KSizeOfFatDirEntry);
    }

/**
Return the Dos name of a directory entry

@return A descriptor containing the Dos name of a directory entry
*/
inline const TPtrC8 TFatDirEntry::Name() const
    {return TPtrC8((TUint8*)&(pDir->iName),KFatDirNameSize);}
/**
@return The attributes for the Directory entry
*/
inline TUint TFatDirEntry::Attributes() const
    {return pDir->iAttributes;}
/**
@param aOffset This offset will be subtracted from the returned time.
@return Time of file modification
*/
inline TTime TFatDirEntry::Time(TTimeIntervalSeconds aOffset) const
    {
    TTime time=DosTimeToTTime(pDir->iTime,pDir->iDate);
    return time-=aOffset;
    }
/**
@return The Start cluster for the file or directory for this entry 
*/
inline TUint32 TFatDirEntry::StartCluster() const      
    {
    const TUint16 KStClustMaskHi = 0x0FFF;  
    return ((pDir->iStartClusterHi & KStClustMaskHi) << 16) | pDir->iStartClusterLo;
    }

/**
@return The size of file or directory for this entry 
*/
inline TUint32 TFatDirEntry::Size() const
    {
    return pDir->iSize;
    }

/**
@return True if the entry is erased
*/
inline TBool TFatDirEntry::IsErased() const
    {return (TBool)(iData[0]==KEntryErasedMarker);}
/**
@return True if the entry refers to the current directory
*/
inline TBool TFatDirEntry::IsCurrentDirectory() const
    {return (TBool)(iData[0]==KDotEntryByte && iData[1]==KBlankSpace);}
/**
@return True if the Entry refers to the parent directory
*/
inline TBool TFatDirEntry::IsParentDirectory() const
    {return (TBool)(iData[0]==KDotEntryByte && iData[1]==KDotEntryByte);}
/**
@return True if end of directory
*/
inline TBool TFatDirEntry::IsEndOfDirectory() const
    {return (TBool)(iData[0]==0x00);}
/**
Set the Dos name of a directory entry

@param aDes A descriptor containg the name
*/
inline void TFatDirEntry::SetName(const TDesC8& aDes)
    {
    __ASSERT_DEBUG(aDes.Length()<=KFatDirNameSize,Fault(EFatBadDirEntryParameter));
    TPtr8 name(pDir->iName,KFatDirNameSize);
    name=aDes;
    }
/**
Set the file or directory attributes for this entry

@param anAtts The file or directory attributes
*/
inline void TFatDirEntry::SetAttributes(TUint anAtts)
    {
    __ASSERT_DEBUG(!(anAtts&~KMaxTUint8),Fault(EFatBadDirEntryParameter));
    pDir->iAttributes=(TUint8)anAtts;
    }
/**
Set the modification time and date of the directory entry

@param aTime Modification time of the file or directory
@aOffset aOffset This offset will be added to the time. 
*/
inline void TFatDirEntry::SetTime(TTime aTime, TTimeIntervalSeconds aOffset)
    {
    aTime+=aOffset;
    pDir->iTime=(TUint16)DosTimeFromTTime(aTime);
    pDir->iDate=(TUint16)DosDateFromTTime(aTime);
    }

inline void TFatDirEntry::SetCreateTime(TTime aTime, TTimeIntervalSeconds aOffset)
    {
    aTime+=aOffset;
    pDir->iTimeC=(TUint16)DosTimeFromTTime(aTime);
    pDir->iDateC=(TUint16)DosDateFromTTime(aTime);
    }

/**
Set the start cluster number of the file or directory refered to by the entry

@param aStartCluster The start cluster number
*/
inline void TFatDirEntry::SetStartCluster(TUint32 aStartCluster)
    {
    pDir->iStartClusterLo=(TUint16)(aStartCluster);
    pDir->iStartClusterHi=(TUint16)(aStartCluster >> 16);
    }
/**
Set the size of the file or directory refered to by the entry

@param aFileSize Size of the file
*/
inline void TFatDirEntry::SetSize(TUint32 aFileSize)
    {
    pDir->iSize=aFileSize;
    }

/** Set the directory entry as erased */
inline void TFatDirEntry::SetErased()
    {
    iData[0]=KEntryErasedMarker;
    }

/** Set the current entry to refer to the current directory */
inline void TFatDirEntry::SetCurrentDirectory()
    {
    iData[0]='.';
    Mem::Fill(&iData[1],KFatDirNameSize-1,' ');
    }
 
/** Set the current entry to refer to the parent directory */
inline void TFatDirEntry::SetParentDirectory()
    {
    iData[0]='.';iData[1]='.';
    Mem::Fill(&iData[2],KFatDirNameSize-2,' ');
    }

/** Set the current entry to be the end of directory marker */
inline void TFatDirEntry::SetEndOfDirectory()
    {
    Mem::FillZ(&iData[0],KFatDirNameSize);
    }

/**
    Get VFAT entry ID. Uset by Rugged FAT and Scan Drive to fix broken entries
    Uses 1 byte from "Last Access Date" field, offset 19. Hack.
*/
TUint16 TFatDirEntry::RuggedFatEntryId() const
    {
    return pDir->iReserved2;
    }

/**
    Set VFAT entry ID. Uset by Rugged FAT and Scan Drive to fix broken entries
    Uses 1 byte from "Last Access Date" field, offset 19. Hack.
*/
void  TFatDirEntry::SetRuggedFatEntryId(TUint16 aId) 
    {
    pDir->iReserved2 = aId;
    }


/** @return True if the entry is the start of a long name set of entries */
inline TBool TFatDirEntry::IsLongNameStart() const
    {
    return (iData[0] & 0x40);
    }

/** @return True is the Entry is a VFat entry */
inline TBool TFatDirEntry::IsVFatEntry() const
    {
    return (Attributes()==KVFatEntryAttribute && IsEndOfDirectory() == EFalse);
    }

/** @return The number of following VFat entries */
inline TInt TFatDirEntry::NumFollowing() const
    {
    return (iData[0]&0x3F);
    }


inline TUint8 TFatDirEntry::CheckSum() const
    {
        ASSERT(IsVFatEntry());
        return iData[13];
    }


/**
@return  ETrue if the Directory entry contains garbage data
*/
inline TBool TFatDirEntry::IsGarbage() const
    {
    return (iData[0]==0xFF);
    }



//-----------------------------------------------------------------------------
/**
    Checks if the entry has the same "modification time" as given (with 2 seconds granularity precision, see FAT specs). 
    
    @param  aTime   time to check
    @param  aOffset time offset

    @return ETrue if the given time+offset is the same (with 2 second granularity) as the one set in the entry.
*/
inline TBool TFatDirEntry::IsTimeTheSame(TTime aTime, TTimeIntervalSeconds aOffset) const
    {
    aTime+=aOffset;

    const TUint16 time = (TUint16)DosTimeFromTTime(aTime);
    if(time != pDir->iTime)
        return EFalse;

    const TUint16 date = (TUint16)DosDateFromTTime(aTime);
    return (date == pDir->iDate);
    }


#endif //FAT_DIR_ENTRY_INL














