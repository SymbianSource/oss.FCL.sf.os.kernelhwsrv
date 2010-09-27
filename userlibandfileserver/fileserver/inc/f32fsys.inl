// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\inc\f32fsys.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#define __IS_DRIVETHREAD() {__ASSERT_DEBUG(IsDriveThread(),DriveFault(ETrue));}
#define __IS_MAINTHREAD() {__ASSERT_DEBUG(IsMainThread(),DriveFault(EFalse));}


//---------------------------------------------------------------------------------------------------------------------------------
// Class TDrive

/**
Gets last error reason.

@return	TInt	Returns last error reason.
*/
inline TInt TDrive::GetReason() const
	{
	__IS_DRIVETHREAD(); 
	return(iReason);
	}



/**
Sets a flag to state that the drive contents has changed.

@param	aValue	True if contents has changed; False if unchanged.
*/
inline void TDrive::SetChanged(TBool aValue)
	{
	iChanged=aValue;
	}




/**
Determines whether the drive content has changed.

@return	True if contents changed , False if unchanged.
*/
inline TBool TDrive::IsChanged() const
	{
	return(iChanged);
	}




/**
Returns the drive number.

@return The drive number. 

@see TDriveNumber
*/
inline TInt TDrive::DriveNumber() const
	{return(iDriveNumber);}




/**
Determines whether the drive is mounted.

@return True if drive is mounted, False if drive is not mounted.
*/
inline TBool TDrive::IsMounted() const
	{
	__IS_DRIVETHREAD();
	return(iCurrentMount!=NULL);
	}




/**
Determines whether attribute is set to local.

@return True if attribute is set to KDriveAttLocal, False for all other attributes.
*/
inline TBool TDrive::IsLocal()  const
	{return(iAtt & KDriveAttLocal);}




/**
Determines whether the drive is ROM drive.

@return True if drive attribute is set as ROM drive  , False if not set as ROM drive.
*/
inline TBool TDrive::IsRom()  const
	{return( iAtt & KDriveAttRom);}




/**
Determines whether the drive is removable.

@return True if drive attribute is set to removable , False for all other attributes.
*/
inline TBool TDrive::IsRemovable() const
	{return( iAtt & KDriveAttRemovable);}




/**
Determines whether the drive is substed.

@return True if drive attribute is set to substed (KDriveAttSubsted), False for all other attributes.
*/
inline TBool TDrive::IsSubsted() const
	{return( iAtt & KDriveAttSubsted);}//	KDriveAttSubsted = 0x08




/**
Gets a reference to the object representing the current mount.

@return The file's mount.
*/
inline CMountCB& TDrive::CurrentMount() const
	{
	__IS_DRIVETHREAD();
	return(*iCurrentMount);
	}

inline  TBool TDrive::IsCurrentMount(CMountCB& aMount) const
	{return(iCurrentMount == &aMount);}



/**
Gets the substed drive.

@return		A pointer to the drive which is substed.
*/
inline TDrive& TDrive::SubstedDrive()const
	{
	__IS_MAINTHREAD();
	return(*iSubstedDrive);
	}




/**

Sets the drive as substed to the path set by an earlier call to SetSubst().

@param	aDrive	A pointer to the drive on which the volume is mounted.

*/
inline void TDrive::SetSubstedDrive(TDrive* aDrive)
	{
	__IS_MAINTHREAD();
	iSubstedDrive=aDrive;
	}




/**
Gets the substed path set by an earlier call to SetSubst().

@return	A reference to a heap descriptor containing the substed path.
*/
inline HBufC& TDrive::Subst() const
	{
	__IS_MAINTHREAD();
	return(*iSubst);
	}




/**
Assigns a path to a drive.

@param	aSubst	Path will be assigned to a drive.

*/
inline void TDrive::SetSubst(HBufC* aSubst)
	{
	__IS_MAINTHREAD();
	iSubst=aSubst;
	}



/**

Gets a reference to the object representing the mount on which the file resides.

@return The Drives's mount.

*/
inline CFsObjectCon& TDrive::Mount() const
	{return(*iMount);}
/**

Gets a reference to the object representing the file system

@return The reference to file system.

*/
inline CFileSystem& TDrive::FSys()
	{return(*iFSys);}
/**

Gets the object representing the file system

@return The file system.

*/
inline CFileSystem*& TDrive::GetFSys()
	{return(iFSys);}
/**

Gets the object representing the TDriveExtInfo.

@return The Drive extension information object.

@see	TDriveExtInfo

*/
inline TDriveExtInfo& TDrive::ExtInfo()
	{
	__IS_DRIVETHREAD();
	return(iExtInfo);
	}
/**
Sets the notification flag ON. The client will receive notifications on Read or Write 
failures from the file system.

*/
inline void TDrive::SetNotifyOn()
	{
	__IS_DRIVETHREAD();
	iDriveFlags &= ~ENotifyOff;
	}
/**
Sets the notification flag OFF. The client will not receive notifications on Read or Write 
failures from the file system.

*/
inline void TDrive::SetNotifyOff()
	{
	__IS_DRIVETHREAD();
	iDriveFlags |= ENotifyOff;
	}

/**
Locks the drive.This function acquires iLock mutex.
*/
inline void TDrive::Lock()
	{iLock.Wait();}

/**
UnLocks the drive.This function signals the iLock mutex.
*/
inline void TDrive::UnLock()
	{iLock.Signal();}


/**
@return	Amount of space reserved in bytes.
*/
inline TInt TDrive::ReservedSpace() const
	{
    return iReservedSpace;
    }

/**
    Reserves space on a drive. The amount of 'reserved space' is subtracted  
    from the amount of available free space on the volume reported by file system, when the user 
    queries it.

    @param	aReservedSpace	Amount of space to reserve in bytes.
*/
inline void TDrive::SetReservedSpace(const TInt aReservedSpace)
	{
    iReservedSpace=aReservedSpace; 
    }

/**
    Sets the 'rugged mode' flag in the drive object. The file system associated with this TDrive object 
    can use this flag for changing its behaviour.
    For example, FAT file system if this flag is set, operates in 'Rugged FAT' mode, when the performance is 
    sacrificed for the sake of reliability. 

    @param aIsRugged  the 'rugged mode' flag.
*/
inline void TDrive::SetRugged(TBool aIsRugged)
	{
	if (!aIsRugged)
		iDriveFlags |= ENotRugged;
	else
		iDriveFlags &= ~ENotRugged;
	}

/**
    @return 'Is rugged' flag.
    See TDrive::SetRugged()
*/
inline TBool TDrive::IsRugged() const
	{
    return !(iDriveFlags & ENotRugged); 
    }


/**
    @return ETrue if the drive is synchronous, i.e. runs in the main file server thread.
*/
inline TBool TDrive::IsSynchronous() const
{
    return iDriveFlags & EDriveIsSynch;
}

/**
    Set or reset internal EDriveIsSynch flag for the TDrive.
*/
inline void TDrive::SetSynchronous(TBool aIsSynch)
{
    if(aIsSynch)
        iDriveFlags |= EDriveIsSynch;
    else
        iDriveFlags &= ~EDriveIsSynch;
    
}

inline TBool TDrive::DismountDeferred() const
	{ return(iDriveFlags & EDismountDeferred); }


// Class CMountCB

/**
Gets a reference to the object representing the drive on which
the volume is mounted.

@return The drive on which the volume is mounted.
*/
inline TDrive& CMountCB::Drive() const
	{return(*iDrive);}




/**
Sets a pointer to the object representing the drive on which
the volume is mounted.

@param aDrive A pointer to the drive on which the volume is mounted.
*/
inline void CMountCB::SetDrive(TDrive* aDrive)
	{iDrive=aDrive;}




/**
Gets a reference to a heap descriptor containing the name of
the mounted volume.

@return A reference to a heap descriptor containing the volume name.
*/
inline HBufC& CMountCB::VolumeName() const
	{return(*iVolumeName);}




/**
Sets a pointer to a heap descriptor containing the name of the mounted volume.

@param aName A pointer to a heap descriptor containing the name of
             the mounted volume to be set.
*/
inline void CMountCB::SetVolumeName(HBufC* aName)
	{iVolumeName=aName;}




/**
Tests whether the client is notified of any read or write failures.

The notification status is a property of the current session with
the file server, the value of which is stored in CSessionFs::iNotifyUser.
If set to true, the client will receive notifications from the file system.

Typically, this function might be used to save the current notification
state prior to temporarily disabling notifiers. This allows the original
notification state to be restored. 

Note that GetNotifyUser() is only available once the drive has been set for
the mount control block (using SetDrive()), since the notification status
is held by the session and accessed via the drive.

@return True if the client receives notifications from the file system,
        false otherwise.
*/
inline TBool CMountCB::GetNotifyUser() const
	{return(Drive().GetNotifyUser());}




/**
*/
inline void CMountCB::SetNotifyOn()
	{Drive().SetNotifyOn();}




/**
*/
inline void CMountCB::SetNotifyOff()
	{Drive().SetNotifyOff();}


//---------------------------------------------------------------------------------------------------------------------------------

/**

    Increment mount's lock counter. This happens on following events:
        - RemoveResource() call, when file (share) or directory is closed
        - AddDiskAccess() call,  when disk access object (like Format or RawDisk access) is opened on the mount.

    See also: CMountCB::LockStatus()   
*/
inline void CMountCB::IncLock()
	{
    iLockMount++;
    }


//---------------------------------------------------------------------------------------------------------------------------------

/**
    Decrement mount's lock counter. This happens on following events:

    AddResource()       call when file (share) or directory is opened on the mount 
    RemoveDiskAccess()  call when disk access object (like Format or RawDisk access) is closed.

    See also: CMountCB::LockStatus()   
*/
inline void CMountCB::DecLock()
	{
    iLockMount--;
    }

//---------------------------------------------------------------------------------------------------------------------------------

/**
    @return value of the Mount's lock counter value.
    
    The meaning of 'iLockMount' is overloaded a bit, it's value is:
        0   when there are no files, directories, formats and any other objects opened on the mount
        >0  when there are disk access objects opened (raw disk access or format)
        <0  when there are files or directories opened on the mount, and the value reflects their total number 
*/
inline TInt CMountCB::LockStatus() const
	{
    return iLockMount;
    }



//---------------------------------------------------------------------------------------------------------------------------------
/**
Tests whether the mount is currently locked. 

A mount is locked when the internal lock counter is greater than zero.
    This happens when a format, resource or raw disk subsession is opened on the mount.

    See also: CMountCB::LockStatus()   

    @return True if the mount is locked by having disk access objects opened
*/
inline TBool CMountCB::Locked() const
	{
    return iLockMount > 0; 
    }




/**
Tests whether the mount control block represents the current mount on
the associated drive.

A drive has only one mount which is accessible: the current mount.
Any mount other than the current mount relates to a partition (i.e. volume)
that was present on a removable media which has since been removed.
The reason the mount has persisted is because resources (i.e. files/directories)
are still open on it.

This function is only available when the drive has been set for the mount
control block (using SetDrive()), since the current mount is held by the drive.

@return True if the mount is the current mount on the drive, false otherwise.
*/
inline TBool CMountCB::IsCurrentMount() const
	{return(this==&iDrive->CurrentMount());}




/**
*/
inline TInt64 CMountCB::Size() const
	{
    return iSize;
    }




/**
Set the unique mount number
@param aMountNumber - The unique mount number
*/
const TInt KMountDismounted = 0x80000000;

inline void CMountCB::SetMountNumber(TInt aMountNumber)
	{ 
    iMountNumber = (aMountNumber & ~KMountDismounted); 
    }




/**
    Set the mount flag indicating that it is in 'dismounted' state.
    The CMountCB object in this case is still alive, but any attempts to access resources on this 
    mount result in KErrDismounted.
*/
inline void CMountCB::SetDismounted(TBool aDismounted)
	{
	if(aDismounted)
		iMountNumber |= KMountDismounted;
	else
		iMountNumber &= ~KMountDismounted;
	}




/**
@return The unique mount number
*/
inline TInt CMountCB::MountNumber() const
	{ 
    return(iMountNumber &~ KMountDismounted); 
    }




/**
@return ETrue if the mount is flagged as dismounted
*/
inline TBool CMountCB::IsDismounted() const
	{ 
    return(iMountNumber & KMountDismounted); 
    }



/**
Retrieves TBusLocalDrive object associated with the mount
*/
inline TInt CMountCB::LocalDrive(TBusLocalDrive*& aLocalDrive)
	{
	aLocalDrive = NULL;
	return GetInterface(EGetLocalDrive, (TAny*&) aLocalDrive, NULL);
	}

/**
Retrieves CProxyDrive object associated with the mount
*/
inline TInt CMountCB::ProxyDrive(CProxyDrive*& aProxyDrive)
	{
	aProxyDrive = NULL;
	return GetInterface(EGetProxyDrive, (TAny*&) aProxyDrive, NULL);
	}

inline TInt CMountCB::AddToCompositeMount(TInt aMountIndex)
	{ 
		TAny *mountInterface = NULL; 
		return(GetInterface(EAddToCompositeMount, mountInterface, (TAny*)aMountIndex)); 
	}

/**
Returns whether the mount (and any extensions) support file caching
*/
inline TInt CMountCB::LocalBufferSupport(CFileCB* aFile)
	{
	TAny* dummyInterface;
	return GetInterface(ELocalBufferSupport, dummyInterface, aFile);
	}

inline TInt CMountCB::MountControl(TInt /*aLevel*/, TInt /*aOption*/, TAny* /*aParam*/)
    {
    return KErrNotSupported;
    }


inline void CMountCB::FinaliseMountL(TInt aOperation, TAny* /*aParam1=NULL*/, TAny* /*aParam2=NULL*/) 
    {
    if(aOperation == RFs::EFinal_RW)
        {//-- call the legacy method
        FinaliseMountL();
        return;
        }
    
    User::Leave(KErrNotSupported);
    }

inline TInt CMountCB::CheckDisk(TInt /*aOperation*/, TAny* /*aParam1=NULL*/, TAny* /*aParam2=NULL*/) 
    {
    return(KErrNotSupported);
    }	

inline TInt CMountCB::ScanDrive(TInt /*aOperation*/, TAny* /*aParam1=NULL*/, TAny* /*aParam2=NULL*/) 
    {
    return(KErrNotSupported);
    }	


//---------------------------------------------------------------------------------------------------------------------------------

/**
    Check is this file system can be mounted on the drive at all. The file system implementation may, for example, 
    read and validate the boot region without real mounting overhead. 
    

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            this file system can be mounted on this drive
            KErrLocked          the media is locked on a physical level.
            other error codes depending on the implementation, indicating that this filesystem can't be mouned 
*/
inline TInt CMountCB::CheckFileSystemMountable() 
    {
    return MountControl(ECheckFsMountable, 0, NULL);
    }

//---------------------------------------------------------------------------------------------------------------------------------
/** 
    Query if the mount is finalised, corresponds to the EMountStateQuery control code only.
    @param  aFinalised  out: ETrue if the mount is finalised, EFalse otherwise. 

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            on success    
*/
inline TInt CMountCB::IsMountFinalised(TBool &aFinalised) 
    {
    return MountControl(EMountStateQuery, ESQ_IsMountFinalised, &aFinalised);
    }

//---------------------------------------------------------------------------------------------------------------------------------
/**
    Corresponds to EMountVolParamQuery. Request a certain amount of free space on the volume.
    If _current_ amount of free space is >= than required or it is not being updated in background by the mount, returns immediately;
    If mount is still counting free space and If _current_ amount of free space is < than required, the caller will be blocked
    until mount finds enough free space or reports that the _final_ amount of free space is less than required.

    @param   aFreeSpaceBytes in: number of free bytes on the volume required, out: resulted amount of free space. It can be less than 
                             required if there isn't enough free space on the volume at all.

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            on success    
*/
inline TInt CMountCB::RequestFreeSpace(TUint64 &aFreeSpaceBytes) 
    {
    return MountControl(EMountVolParamQuery, ESQ_RequestFreeSpace, &aFreeSpaceBytes);
    }

//---------------------------------------------------------------------------------------------------------------------------------
/**
    Corresponds to EMountVolParamQuery. A request to obtain the _current_ amount of free space on the volume asynchronously, without blocking.
    Some mounts implementations can count volume free space in the background. 
        
    @param  aFreeSpaceBytes  in: none; out: _current_ amount of free space on the volume.

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            on success    
*/
inline TInt CMountCB::GetCurrentFreeSpaceAvailable(TInt64 &aFreeSpaceBytes)
    {
    return MountControl(EMountVolParamQuery, ESQ_GetCurrentFreeSpace, &aFreeSpaceBytes);
    }
 
//---------------------------------------------------------------------------------------------------------------------------------
/**
    Corresponds to EMountVolParamQuery. A request to obtain size of the mounted volume without blocking (CMountCB::VolumeL() can block).
    @param  aVolSizeBytes  in: none; out: mounted volume size, same as TVolumeInfo::iSize

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            on success    
*/
inline TInt CMountCB::MountedVolumeSize(TUint64& aVolSizeBytes)  
    {
    return MountControl(EMountVolParamQuery, ESQ_MountedVolumeSize, &aVolSizeBytes);
    }


//---------------------------------------------------------------------------------------------------------------------------------
/** 
    Get Maximum file size, which is supported by the file system that has produced this mount. 
    @param  aVolSizeBytes  in: none; out: Theoretical max. supported by this file system file size.

    @return KErrNotSupported    this feature is not supported by the file system
            KErrNone            on success    
*/
inline TInt CMountCB::GetMaxSupportedFileSize(TUint64 &aSize) 
    {
    return MountControl(EMountFsParamQuery, ESQ_GetMaxSupportedFileSize, &aSize);
    }
	


//###############################################################################################
// Class CFileCB

/**
Sets the mount associated with the file.

@param aMount The mount.
*/
inline void CFileCB::SetMount(CMountCB * aMount)
	{iMount=aMount;}

/**
Gets a reference to the object representing the drive on which
the file resides.

@return A reference to the file's drive.
*/
inline TDrive& CFileCB::Drive() const
	{return(*iDrive);}




/**
Gets a reference to the object representing the drive on which the file was created.

The 'created drive' is only different from the 'drive', as returned by Drive(), if 
the 'drive' was a substitute for the 'created drive' in the file server session.

@return A reference to the drive on which the file was created.
*/
inline TDrive& CFileCB::CreatedDrive() const
	{return(*iCreatedDrive);}




/**
Gets a reference to the object representing the mount on which the file resides.

@return The file's mount.
*/
inline CMountCB& CFileCB::Mount() const
	{return(*iMount);}




/**
Gets a reference to a heap descriptor containing the full file name.

@return A heap descriptor containing the full file name.
*/
inline HBufC& CFileCB::FileName() const
	{return(*iFileName);}

/**
Gets a reference to a heap descriptor containing the folded full file name.

@return A heap descriptor containing the full file name.
*/
inline HBufC& CFileCB::FileNameF() const
	{return(*iFileNameF);}

/**
Gets the hash of the folded filename

@return hash of the folded file name
*/
inline TUint32 CFileCB::NameHash() const
	{return(iNameHash);}


/**
Gets a reference to the file share locks array being used by the file.
@return The file share lock.
*/
inline TFileLocksArray& CFileCB::FileLocks()
	{return(*iFileLocks);}




/**
Gets the file object's unique ID, as returned by CObject::UniqueID().

@return The object's unique ID.

@see CObject
*/
inline TInt CFileCB::UniqueID() const
	{return(CFsObject::UniqueID());}




/**
Gets the iShare value, which defines the level of access allowed to the file.

@return The value of iShare

@see CFileCB::iShare
*/
inline TShare CFileCB::Share() const
	{return(iShare);}




/**
Sets the iShare value, which defines the level of access allowed to the file.

@param aShare The new value.

@see CFileCB::iShare
*/
inline void CFileCB::SetShare(TShare aShare)
	{iShare=aShare;}




/**
Gets the size of the file.

@return The size of the file.
*/
inline TInt CFileCB::Size() const
	{return(iSize);}




/**
Sets the size of the file.

@param aSize The size of the file.
*/
inline void CFileCB::SetSize(TInt aSize)
	{iSize=aSize;}




/**
Gets the file's attributes.

@return An integer containing the file attribute bit mask.
*/
inline TInt CFileCB::Att() const
	{return(iAtt);}




/**
Sets the file's attributes.

@param aAtt The file attribute bit mask.
*/
inline void CFileCB::SetAtt(TInt aAtt)
	{iAtt=aAtt;}	




/**
Gets the universal time when the file was last modified.

@return The universal time when the file was last modiified.
*/
inline TTime CFileCB::Modified() const 
	{return(iModified);}




/**
Sets the universal time when the file was last modified.

@param aModified The universal time when the file was last modified.
*/
inline void CFileCB::SetModified(TTime aModified)
	{iModified=aModified;}




/**
Tests whether the file is corrupt.

@return ETrue if the file is corrupt; EFalse otherwise.
*/
inline TBool CFileCB::FileCorrupt() const
	{return iFileCorrupt;}




/**
Sets whether the file is corrupt.

@param aFileCorrupt ETrue, if the file is corrupt; EFalse, otherwise.
*/
inline void CFileCB::SetFileCorrupt(TBool aFileCorrupt)
	{iFileCorrupt=aFileCorrupt;}




/**
Gets the iBadPower value.

@return The value of iBadPower

@see CFileCB::iBadPower
*/
inline TBool CFileCB::BadPower() const
	{return (iBadPower);}




/**
Sets the iBadPower value.

@param aBadPower ETrue, if an operation on the file has failed due
                 to bad power;
				 EFalse if power has been found to be good.

@see CFileCB::iBadPower
*/
inline void CFileCB::SetBadPower(TBool aBadPower)
	{iBadPower=aBadPower;}


/**
Retrieves the BlockMap of a file.

@param aInfo

@param aStartPos

@param aEndPos

@return 
*/
inline TInt CFileCB::BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos)
	{
	TAny* pM;
	TInt r = GetInterface(EBlockMapInterface, pM, (TAny*) this);
	if (KErrNone!=r)
		return r;
	return reinterpret_cast<CFileCB::MBlockMapInterface*>(pM)->BlockMap(aInfo, aStartPos, aEndPos);
	}


/**
Retrieves TBusLocalDrive object associated with an open file.
*/
inline TInt CFileCB::LocalDrive(TBusLocalDrive*& aLocalDrive)
	{
	aLocalDrive = NULL;
	return GetInterface(EGetLocalDrive, (TAny*&) aLocalDrive, NULL);
	}

//---------------------------------------------------------------------------------------------------------------------------------
// Class RLocalMessage
inline RLocalMessage::RLocalMessage()
	{InitHandle();}

inline void RLocalMessage::InitHandle()
	{iHandle = KLocalMessageHandle; iFunction=-1;}

inline void RLocalMessage::SetFunction(TInt aFunction)
	{iFunction = aFunction;}
	
inline void RLocalMessage::SetArgs(TIpcArgs& aArgs)
	{
	iArgs[0] = aArgs.iArgs[0];
	iArgs[1] = aArgs.iArgs[1];
	iArgs[2] = aArgs.iArgs[2];
	iArgs[3] = aArgs.iArgs[3];

	}

inline TInt RLocalMessage::Arg(TInt aIndex) const
	{return iArgs[aIndex];}

//---------------------------------------------------------------------------------------------------------------------------------
// Class CFileShare
/**
Gets a reference to the object representing an open file that is being shared.

@return A reference to the shared file.
*/
inline CFileCB& CFileShare::File()
	{return(*iFile);}


// Returns ETrue if the file share mode is EFileBifFile 
// indicating large file access for the file share
inline TBool CFileShare::IsFileModeBig()
	{
	return (iMode & EFileBigFile) ? (TBool)ETrue:(TBool)EFalse;
	}


//---------------------------------------------------------------------------------------------------------------------------------
// Class CDirCB

/**
Gets a reference to the object representing the drive on which
the directory resides.

@return A reference to the directory's drive.
*/
inline TDrive& CDirCB::Drive() const
	{return(*iDrive);}




/**
Gets a reference to the object representing the mount on which
the directory resides.

@return A reference to the directory's mount.
*/
inline CMountCB& CDirCB::Mount() const
	{return(*iMount);}




/**
Tests whether the preceding entry details should be returned when
multiple entries are being read.

@return True if the preceding entry details should be returned;
        false otherwise.
*/
inline TBool CDirCB::Pending() const
	{return iPending;}




/**
Sets whether the preceding entry details should be returned when
multiple entries are being read.

@param aPending ETrue if the preceding entry details should be returned;
                EFalse otherwise.
*/
inline void CDirCB::SetPending(TBool aPending)
	{iPending=aPending;}



//---------------------------------------------------------------------------------------------------------------------------------
// class CFormatCB

/**
Gets the object representing the drive on which the disk to
be formatted resides.

@return The drive for the format action.
*/
inline TDrive& CFormatCB::Drive()  const
	{return(*iDrive);}




/**
Gets the object representing the mount on which the disk to
be formatted resides.

@return The mount for the format action.
*/
inline CMountCB& CFormatCB::Mount()  const
	{return(*iMount);}




/**
Gets the mode of the format operation.

@return The value of the format mode.
*/
inline TFormatMode CFormatCB::Mode()  const
	{return(iMode);}




/**
Gets the current stage in the format operation.

@return The stage the current format operation has reached.
*/
inline TInt& CFormatCB::CurrentStep() 
	{return(iCurrentStep);}



//---------------------------------------------------------------------------------------------------------------------------------
// class CRawDiskCB

/**
Gets a reference to an object representing the drive on which the disk resides.

@return  A reference to the drive on which the disk resides.
*/
inline TDrive& CRawDiskCB::Drive()
	{return(iMount->Drive());}




/**
Gets an object representing the mount on which the disk resides.

@return The mount on which the disk resides.
*/
inline CMountCB& CRawDiskCB::Mount()
	{return(*iMount);}




/**
Tests whether the mount on which the disk resides is write protected.

@return True if the mount is write protected, false otherwise.
*/
inline TBool CRawDiskCB::IsWriteProtected() const
	{ return(iFlags & EWriteProtected); }





/**
Stores the write protected state of the disk.
*/
inline void CRawDiskCB::SetWriteProtected()
	{ iFlags |= EWriteProtected; }




/**
Tests whether the disk contents has changed (due to a write operation)

@return True if the disk contents has changed
*/
inline TBool CRawDiskCB::IsChanged() const
	{ return(iFlags & EChanged); }




/**
Set a flag to state that the disk contents has changed (due to a write operation)
*/
inline void CRawDiskCB::SetChanged()
	{ iFlags |= EChanged; }



//---------------------------------------------------------------------------------------------------------------------------------
// class CProxyDriveFactory
/**
Sets the Library (DLL) handle to be used by the CProxyDriveFactory
*/
inline void CProxyDriveFactory::SetLibrary(RLibrary aLib)
	{iLibrary=aLib;}
/**
Gets the Library (DLL) handle in use by the CProxyDriveFactory
@return Library (DLL) handle 
*/
inline RLibrary CProxyDriveFactory::Library() const
	{return(iLibrary);}


inline void CExtProxyDriveFactory::SetLibrary(RLibrary aLib)
	{iLibrary=aLib;}

inline RLibrary CExtProxyDriveFactory::Library() const
	{return(iLibrary);}


//---------------------------------------------------------------------------------------------------------------------------------
// class CProxyDrive
/**
Gets the mount control block object for a specific volume on a drive.
	
@return either a currently mounted volume in the system or the volume that has been removed but still has
subsession objects open.
*/
inline CMountCB* CProxyDrive::Mount() const
	{return(iMount);}

inline void CProxyDrive::SetMount(CMountCB *aMount)
	{
	iMount = aMount;
	}

/**
Returns wheher the drive (and any extensions) support file caching
*/
inline TInt CProxyDrive::LocalBufferSupport()
	{
	TAny* dummyInterface;
	return GetInterface(ELocalBufferSupport, dummyInterface, NULL);
	}

/**
return whether proxy drive supports file caching
*/
inline TInt CBaseExtProxyDrive::LocalBufferSupport()
	{
	return iProxy->LocalBufferSupport();
	}

//---------------------------------------------------------------------------------------------------------------------------------
// Surrogate Pair hepler apis
/**
Determines if aChar is the outsite BMP.

@param aChar character to checked if that is outside BMP.
@return ETrue if outside BMP, EFalse otherwise.
*/
inline TBool IsSupplementary(TUint aChar)
	{
	return (aChar > 0xFFFF);
	}

/**
Determines if aInt16 is  a high surrogate.

@param aInt16 character to checked if that is high surrogate.
@return ETrue if high surrogate, EFalse otherwise.
*/
inline TBool IsHighSurrogate(TText16 aInt16)
	{
	return (aInt16 & 0xFC00) == 0xD800;
	}

/**
Determines if aInt16 is  a low surrogate.

@param aInt16 character to checked if that is low surrogate.
@return ETrue if low surrogate, EFalse otherwise.
*/
inline TBool IsLowSurrogate(TText16 aInt16)
	{
	return (aInt16 & 0xFC00) == 0xDC00;
	}

/**
Joins high surrogate character aHighSurrogate and low surrogate character aLowSurrogate.

@param aHighSurrogate a high surrogate character to be joined.
@param aLowSurrogate a low surrogate character to be joined.
@return joined character that is outside BMP.
*/
inline TUint JoinSurrogate(TText16 aHighSurrogate, TText16 aLowSurrogate)
	{
	return ((aHighSurrogate - 0xD7F7) << 10) + aLowSurrogate;
	}

//---------------------------------------------------------------------------------------------------------------------------------
// class CExtProxyDrive
inline TInt CExtProxyDrive::DriveNumber()
	{return iDriveNumber;};
inline void CExtProxyDrive::SetDriveNumber(TInt aDrive)
	{iDriveNumber = aDrive;};
inline CExtProxyDriveFactory* CExtProxyDrive::FactoryP()
	{return iFactory;};

//---------------------------------------------------------------------------------------------------------------------------------
// class CLocDrvMountCB
/**
Gets the mounted local drive object

@return The local drive.
*/
inline CProxyDrive* CLocDrvMountCB::LocalDrive() const
	{return(iProxyDrive);}	

//---------------------------------------------------------------------------------------------------------------------------------	
// class CFsObject
inline CFsObjectCon* CFsObject::Container() const
	{ return iContainer; }
inline TInt CFsObject::Inc()
	{ return __e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0); }
inline TInt CFsObject::Dec()
	{ return __e32_atomic_tas_ord32(&iAccessCount, 1, -1, 0); }

inline TInt CFsObject::AccessCount() const
	{return iAccessCount;}

