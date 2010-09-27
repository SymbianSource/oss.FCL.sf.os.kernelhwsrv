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
// f32\inc\f32fsys.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @released
*/

#if !defined(__F32FSYS_H__)
#define __F32FSYS_H__
#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif
#include <e32atomics.h>
#include <d32locd.h>

//
#if defined(_UNICODE)
#define KFileSystemUidValue KFileSystemUidValue16
#define KFileServerUidValue KFileServerUidValue16
#define KFileServerDllUidValue KFileServerDllUidValue16
#else
#define KFileSystemUidValue KFileSystemUidValue8
#define KFileServerUidValueKFileServerUidValue8
#define KFileServerDllUidValueKFileServerDllUidValue8
#endif


/**
Filesystem error code 1 : indicates an item cannot be found,
because it has been hidden.
*/
const TInt KErrHidden=(1);

/**
Filesystem error code 2 : in the context of file operations, a path
was not found, because it has been hidden.
*/
const TInt KErrPathHidden=(2);


const TInt KFileShareLockGranularity=2;
const TInt KAsyncRequestArrayGranularity=2;

/**
@publishedPartner
@released

File system UID value 16.
*/
const TInt KFileSystemUidValue16=0x100039df;




/**
@publishedPartner
@released

File system UID value 8.
*/
const TInt KFileSystemUidValue8=0x1000008f;




/**
@publishedPartner
@released

File server UID value 16.
*/
const TInt KFileServerUidValue16=0x100039e3;




/**
@publishedPartner
@released

File server UID value 8.
*/
const TInt KFileServerUidValue8=0x100000bb;




/**
@publishedPartner
@released

File server DLL UID value 16.
*/
const TInt KFileServerDllUidValue16=0x100039e4;




/**
@publishedPartner
@released

File server DLL UID value 8.
*/
const TInt KFileServerDllUidValue8=0x100000bd;




/**
@publishedPartner
@released

Local file system UID value.
*/
const TInt KLocalFileSystemUidValue=0x100000d6;




/**
@publishedPartner
@released

Estart component UID value.
*/
const TInt KEstartUidValue=0x10272C04;



/**
@publishedPartner
@released
Maximum length of a volume name.
*/
const TInt KMaxVolumeNameLength=11;




/**
@publishedPartner
@released

First local drive indicator.
*/
const TInt KFirstLocalDrive=EDriveC;


const TInt KMaxExtensionCount=2;
//
const TInt KDriveInvalid=-1;
//
_LIT(KMediaPWrdFile, "?:\\sys\\data\\mmcstore");
//

/** 
@internalTechnology
*/
const TUint KSystemDriveKey = 0x10283049;


/**
@publishedPartner
@released

Enumeration that specifies whether, on opening a file:
- an existing file is opened
- a new file is created 
- an existing file is replaced.
*/
enum TFileOpen {EFileOpen,EFileCreate,EFileReplace};




/**
@publishedPartner
@released

The file share mode.
*/
typedef TFileMode TShare;




class CMountCB;
class CFileSystem;
class CFileCB;
class CDirCB;
class CFileShare;
class CSessionFs;
class CFsPlugin;
class CFileBody;
class CMountBody;
class CFsMessageRequest;
class CProxyDrive;
class CFormatCB;

//
class CFsObjectCon;
class CFileCache;
//
class CExtNotifyMediaChange;
//

/**
@publishedPartner
@released

Implements reference counting to track concurrent references to itself.
 
An object of this type arranges automatic destruction of itself when the final 
reference is removed.

A reference counting object is any object which has CFsObject as its base class. 
Constructing a CFsObject derived type or calling its Open() member function 
adds a reference to that object by adding one to the reference count; calling 
its Close() member function removes a reference by subtracting one from the 
reference count; when the last user of the object calls Close(), the reference 
count becomes zero and the object is automatically destroyed.
*/
class CFsObject : public CBase

	{
public:
	IMPORT_C CFsObject();
	IMPORT_C virtual TInt Open();
	IMPORT_C virtual void Close();
	IMPORT_C TInt SetName(const TDesC* aName);
	IMPORT_C TName Name() const;
	IMPORT_C virtual TBool IsCorrectThread();
	inline CFsObjectCon* Container() const;
	inline TInt AccessCount() const;
protected:
	void DoClose();
	TInt UniqueID() const;
	inline TInt Inc();
	inline TInt Dec();
	IMPORT_C ~CFsObject();
private:
	TInt iAccessCount;
	CFsObjectCon* iContainer;
	HBufC* iName;   
friend class CFsObjectCon;
friend class CFsObjectIx;
	};




class CFsRequest;
class CFsInternalRequest;

/**
Implements a request dispatcher.
 
Base class for file server resources.
for example subsessions that are opened, such as RFile etc, that need closing are closed by 
issuing a subsession close request, handled by this object.

@publishedPartner
@released
*/
class CFsDispatchObject : public CFsObject
	{
public:
	CFsDispatchObject();
	/**
	Returns the drive number.
	@return Drive number.
	*/
	TInt DriveNumber() const {return(iDriveNumber);}
	IMPORT_C void Close();
	IMPORT_C virtual TBool IsCorrectThread();
protected:
	void DoInitL(TInt aDrvNumber);
	void Dispatch();
	~CFsDispatchObject();
private:
	CFsInternalRequest* iRequest;
	TInt iDriveNumber;
friend class TFsCloseObject;
friend class CFileShare;	// needed to override the close operation so that the file cache can be flushed on a close
	};




/**
Notifier class must be unique to each thread so one per drive or threaded plugin should be used
allocated in the file system. No longer global

@publishedPartner
@released
*/
NONSHARABLE_CLASS(CAsyncNotifier) : public CBase
	{
public:
	IMPORT_C static CAsyncNotifier* New();
	IMPORT_C ~CAsyncNotifier();
	IMPORT_C TInt Notify(const TDesC& aLine1,const TDesC& aLine2,const TDesC& aButton1,const TDesC& aButton2,TInt& aButtonVal);
	inline void SetMount(CMountCB* aMount) { iMount = aMount; };
protected:
	CAsyncNotifier();
	TInt Connect(); 
private:
	RNotifier iNotifier;
	CMountCB* iMount;
	};




class CProxyDriveFactory;

/**
@publishedPartner
@released

Structure containing information related to a single drive extension.
*/
struct TExtensionInfo
	{
	TBool iIsPrimary;            	///< Is the primary drive extension for a given drive  
	CProxyDriveFactory* iFactory;  	///< Pointer to the drive extension's object factory
	};




/**
@publishedPartner
@released

Represents information related to the Drive extension(s) in use for a given drive.
*/
struct TDriveExtInfo
	{
	TDriveExtInfo();
	
	TInt iCount; 								///< Number of drive extensions in use                               

	TExtensionInfo iInfo[KMaxExtensionCount]; 	///< Drive extension related information    
	};




/**
@publishedPartner
@released

Represents a drive in the file server.

Note that drives may act as substitutes for paths on other drives,
in which case any access to this drive letter will be translated into
a reference to the assigned path. In this way drives can act as short
cuts to paths on other drives.
*/
class TDrive
	{
public:
	TDrive();
	void CreateL(TInt aDriveNumber);
	TInt CheckMount();
	TInt CheckMountAndEntryName(const TDesC& aName);
    TInt FinaliseMount();
    TInt FinaliseMount(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);
    TInt MountControl(TInt aLevel, TInt aOption, TAny* aParam);
    void MountFileSystem(TBool aForceMount, TUint32 aFsNameHash = 0);
    void FlushCachedFileInfoL();
	TInt FlushCachedFileInfo(TBool aPurgeCache = EFalse);
	void PurgeDirty(CMountCB& aMount);
	void DriveInfo(TDriveInfo& anInfo);
	TInt Volume(TVolumeInfo& aVolume);
	TInt SetVolume(const TDesC& aName);
	TInt MkDir(const TDesC& aName);
	TInt RmDir(const TDesC& aName);
	TInt Delete(const TDesC& aName);
	TInt Rename(const TDesC& anOldName,const TDesC& aNewName);
	TInt Replace(const TDesC& anOldName,const TDesC& aNewName);
	TInt Entry(const TDesC& aName,TEntry& anEntry);
	TInt SetEntry(const TDesC& aName,const TTime& aTime,TUint aMask,TUint aVal);
	TInt FileTemp(CFsRequest* aRequest,TInt& aHandle,const TDesC& aPath,TDes& aName,TUint aMode);
	TInt FileOpen(CFsRequest* aRequest,TInt& aHandle,const TDesC& aName,TUint aMode,TFileOpen anOpen);
	TInt DirOpen(CSessionFs* aSession,TInt& aHandle,const TDesC& aName,TUint anAtt,const TUidType& aUidType);
    CFormatCB* FormatOpenL(CFsRequest* aRequest, TInt& aFmtHandle, TFormatMode aFmtMode, const TLDFormatInfo*  apLDFormatInfo, const TVolFormatParam*  apVolFormatParam);
	
    TInt CheckDisk(); 
    TInt CheckDisk(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);
	
    TInt ScanDrive(); 
	TInt ScanDrive(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);

    TInt ReadFileSection(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
    TInt ReadFileSection64(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
	TInt GetShortName(const TDesC& aLongName,TDes& aShortName);
	TInt GetLongName(const TDesC& aShortName,TDes& aLongName);
	TInt IsFileOpen(const TDesC& aFileName,CFileCB*& aFileCB);
	TInt IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	TInt LockDevice(TMediaPassword& aOld,TMediaPassword& aNew,TBool aStore);
	TInt UnlockDevice(TMediaPassword& aPassword,TBool aStore);
	TInt ClearDevicePassword(TMediaPassword& aPassword);
	TInt EraseDevicePassword();
	TInt FreeDiskSpace(TInt64& aFreeDiskSpace);
	TInt ForceRemountDrive(const TDesC8* aMountInfo,TInt aMountInfoMessageHandle,TUint aFlags);
	TBool IsWriteProtected();
	TInt MountExtension(CProxyDriveFactory* aFactory,TBool aIsPrimary);
	TInt DismountExtension(CProxyDriveFactory* aFactory,TBool aIsPrimary);
	TInt ExtensionName(TDes& aExtensionName,TInt aPos);
	TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);	
	void SetAtt(TUint aValue);
	IMPORT_C TUint Att();
	IMPORT_C TBool GetNotifyUser();		
	IMPORT_C void Dismount();		
	IMPORT_C TBool IsWriteableResource() const;	
	IMPORT_C TBool IsCurrentWriteFunction() const; 
	inline TInt GetReason() const;	
	inline void SetChanged(TBool aValue);	
	inline TBool IsChanged() const;
	inline TInt DriveNumber() const;
	inline TBool IsMounted() const;
	inline TBool IsLocal()	const;			
	inline TBool IsRom()	const;			
	inline TBool IsRemovable()	const;		
	inline TBool IsSubsted() 	const;		
	inline CMountCB& CurrentMount() const;
	inline  TBool IsCurrentMount(CMountCB& aMount) const;
	inline TDrive& SubstedDrive() const;
	inline void SetSubstedDrive(TDrive* aDrive);
	inline HBufC& Subst() const;
	inline void SetSubst(HBufC* aSubst);
	inline CFsObjectCon& Mount() const;	
	inline CFileSystem& FSys();			
	inline CFileSystem*& GetFSys();
	inline TDriveExtInfo& ExtInfo();	
	inline void SetNotifyOn();			
	inline void SetNotifyOff();	
	inline TInt ReservedSpace() const;
	inline void SetReservedSpace(const TInt aReservedSpace);
	
    inline void SetRugged(TBool aIsRugged);
	inline TBool IsRugged() const;
    
    inline TBool IsSynchronous() const;
    inline void SetSynchronous(TBool aIsSynch);

	TInt DismountProxyDrive();
    TInt ForceUnmountFileSystemForFormatting();

public:
	void DismountLock();
	TInt DismountUnlock();
	TInt DismountLocked() const;
	void SetDismountDeferred(TBool aPending);
	void ForceDismount();
	TInt ActiveMounts() const;
	void ReactivateMounts();
	TInt ClampFile(const TDesC& aName,TAny* aHandle);
	TInt UnclampFile(CMountCB* aMount, RFileClamp* aHandle);
	TInt ClampsOnDrive();
	inline TBool DismountDeferred() const;
	TInt DeferredDismount();
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TInt ClearDeferredDismount();
#endif
	void SetClampFlag(TBool aClamped);
	TBool ClampFlag();
	inline void Lock();
	inline void UnLock();
    void MultiSlotDriveCheck();	
    
    TInt RequestFreeSpaceOnMount(TUint64 aFreeSpaceRequired);
    TInt MountedVolumeSize(TUint64& aSize);

	TBool ReMount(CMountCB& aMount);

private:

    void DoMountFileSystemL(CMountCB*& apMount, TBool aForceMount, TUint32 aFsNameHash);

    void SetVolumeL(const TDesC& aName,HBufC*& aBuf);
	void DirOpenL(CSessionFs* aSession,TInt& aHandle,const TDesC& aName,TUint anAtt,const TUidType& aUidType,CDirCB*& aDir);
	void FileOpenL(CFsRequest* aRequest,TInt& aHandle,const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB*& aFileCB,CFileShare*& aFileShare);
	TInt CheckMountAndEntryNames(const TDesC& anOldName,const TDesC& aNewName);
	CFileCB* LocateFileByPath(const TDesC& aPath);
	TInt CheckDirectories(const TDesC& anOldName,const TDesC& aNewName);
	void DoEntryL(const TDesC& aName,TEntry& anEntry);
	void ReadSectionL(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
	TInt ValidateShare(CFileCB& aFile,TShare aReqShare);
	TInt CheckAttributes(TUint& aSetAttMask,TUint& aClearAttMask);
	TBool IsExtensionMounted(CProxyDriveFactory* aFactory);
	CFileCB* LocateFile(const TDesC& aName);
	CFileCache* LocateClosedFile(const TDesC& aName, TBool aResurrect = ETrue);
	TBool ReMount();
	IMPORT_C TBool IsDriveThread() const;
	IMPORT_C TBool IsMainThread() const;
	IMPORT_C void DriveFault(TBool aDriveError) const;
    void DoDismount();
    void DoCompleteDismountNotify(TInt aCompletionCode);

private:
	
    //-- intrinsic TDrive flags. Used in iDriveFlags.
    enum 
	    { 
        ENotifyOff       = 0x01, 
        EDismountDeferred= 0x02,
        ENotRugged       = 0x04, 
        EClampPresent    = 0x08,
        EDriveIsSynch    = 0x10, //-- is set on mount when the drive is synchronous (doesn't have its own thread)
		};	

private:
	TInt            iDriveNumber;
	TUint           iAtt;
	TBool           iChanged;
	TInt            iReason;
	TInt            iMountNumber;
	CFileSystem*    iFSys;
	CMountCB*       iCurrentMount;
	TDrive*         iSubstedDrive;
	HBufC*          iSubst;
	CFsObjectCon*   iMount;
	RFastLock       iLock;
	TDriveExtInfo   iExtInfo;	
	TInt            iDriveFlags;   ///< intrinsic TDrive flags
	TInt            iReservedSpace;
	TInt            iDismountLock;
	TInt            iMountFailures;		// number of times the mount has failed
	TInt            iLastMountError;

	TInt iSpare1;			
	TInt iSpare2;

	
	friend class LocalDrives;			// for access to iChanged flag
	friend class CExtNotifyMediaChange; // for access to iChanged flag
	
#if defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE)
	friend class TFsControlIo;			// for access to LocateDrives()
#endif
	};

class CFileCB;
class CDirCB;

__ASSERT_COMPILE(sizeof(TVolFormatParam) != sizeof(TLDFormatInfo));



/**
@publishedPartner
@released

A file server interface class representing a mount.

An instance of this object is referred to as a mount control block.

A mount control block needs to be created for a specific volume (partition) on
a drive in order to be able to access that volume. Volumes may be permanent
or represent removable media. Note that removable media may also be mounted directly onto
a device with no drive. Volumes can be formatted, unlike drives.

The volume represented is either a currently mounted volume in the system or,
in the case of removable volumes, a volume that has been removed but still has
subsession objects open.

A plug-in file system implements this class.
*/
class CMountCB : public CFsDispatchObject
	{
public:
	IMPORT_C CMountCB();
	IMPORT_C ~CMountCB();
	IMPORT_C TBool operator!=(const CMountCB& aMount) const;
	IMPORT_C TBool MatchEntryAtt(TUint anAtt,TUint aMatt) const;
	IMPORT_C void SetDiskSpaceChange(TInt64 aFreeDiskSpace);
    IMPORT_C void InitL(TDrive& aDrive, CFileSystem* apFileSystem);

    inline TDrive& Drive() const;
	inline void SetDrive(TDrive* aDrive);
	inline HBufC& VolumeName() const; 
	inline void SetVolumeName(HBufC* aName);
	inline TBool GetNotifyUser() const;
	inline void SetNotifyOn();
	inline void SetNotifyOff();
	inline void IncLock();
	inline void DecLock();
	inline TInt LockStatus() const; 
	inline TBool IsCurrentMount() const; 
	inline TBool Locked() const;
	inline TInt64 Size() const; 
	inline TInt LocalDrive(TBusLocalDrive*& aLocalDrive);
	inline TInt ProxyDrive(CProxyDrive*& aProxyDrive);
	inline TInt LocalBufferSupport(CFileCB* aFile = NULL);
	inline TInt AddToCompositeMount(TInt aMountIndex);
	
// Pure virtual

    /**
    Attempts to set the mount control block properties using
    the current mount (i.e. volume) on the associated drive.

    The function should set the volume name (iVolumeName),
    the unique ID (iUniqueID) and the volume size (iSize)
    by reading and processing the current mount.

    When aForceMount is set to ETrue, the properties of a corrupt volume should
    be forcibly stored. The classic case of when this is desirable is when
    a corrupt volume needs to be formatted.

    The function should leave, on error detection, with an appropriate error code.

    @param aForceMount Indicates whether the properties of a corrupt
                       volume should be stored.
                       
    @leave KErrCorrupt The properties of the current mount on the drive were
           not successfully mounted due to corruption of volume information,
           assuming that aForceMount is not set.
    */
	virtual void MountL(TBool aForceMount) =0;


    /**
    Checks whether the mount control block represents the current mount on
    the associated drive.

    The function should read mount information from the current volume,
    and check it against the mount information from this mount - typically
    iVolumeName and iUniqueID. If the mount information matches, the function
    should return KErrNone, otherwise it should return KErrGeneral.

    Called by the associated TDrive object when the drive has no current mounts,
    which is the case on first access to the drive and following a volume
    change on a drive associated with removable media. In this circumstance,
    this function is called systematically on every mount control block owned
    by the drive. If ReMount() calls for all existing mount
    control blocks fail, the drive creates a new mount control block and calls
    CMountCB::MountL() on that object; the new object is added to the list of
    mount control blocks owned by the drive.

    @return KErrNone if the mount represented by this object is found to be
            the current mount;
            KErrGeneral if this object is found not to represent
            the current mount;
            otherwise one of the other sytem wide error codes.
    */
	virtual TInt ReMount() =0;


    /**
    Carries out any clean-up necessary for a volume dismount. 

    Dismounting a volume will always succeed, so the function does not need
    to return an error value. Any cached information should be discarded and no
    attempt should be made to access the volume. For removable media it may be
    that the media has already been removed. This function is called when
    a media change is detected.
    */
	virtual void Dismounted() =0;


    /**
    Gets volume information.

    The only information that the function has to supply is the free space,
    TVolumeInfo::iFree, since the remaining members have already been set by
    the calling function.

    The function should leave, on error detection, with
    an appropriate error code.

    @param aVolume On return, a reference to the filled volume
                   information object.
    */
	virtual void VolumeL(TVolumeInfo& aVolume) const =0;


    /**
    Sets the volume name for the mount, thus writing the new volume name
    to the corresponding volume.

    This function should leave on error detection.

    @param aName A reference to a descriptor containing the new volume name.

    @leave KErrBadName If the specified volume name is longer than the maximum
           allowed length for a volume name
    */
	virtual void SetVolumeL(TDes& aName) =0;


    /**
    Creates a new directory on the mount.

    The directory to be created is identified through its full name in aName.
    The full name is in the form:
    @code
    \\dirA\\dirB\\dirC\\dirD
    @endcode
    where dirD is the new directory to be created in \\dirA\\dirB\\dirC\\.
    This means that dirC is the leaf directory in which dirD will be created.

    The function should leave, on error detection, with an appropriate
    error code.

    @param aName A reference to a descriptor containing the full name of
                 the directory to be created.
                 
    @leave  KErrPathNotFound Part of the path in aName does not exist.
    @leave  KErrAlreadyExists dirD already exists in \\dirA\\dirB\\dirC\\
    @leave  KErrAccessDenied dirD already exists but is not a directory.
    @leave  KErrDirFull There is no room in \\dirA\\dirB\\dirC\\ for the new entry,
            which is especially applicable to the root directory.
    */
	virtual void MkDirL(const TDesC& aName) =0;


    /**
    Removes the directory specified by aName (its full name) from the volume.

    The directory specified by aName is in the form:
    @code
    \\dirA\\dirB\\dirC\\dirD
    @endcode
    where dirD is the directory to be removed from \\dirA\\dirB\\dirC\\.
    This means that dirC is the leaf directory from which dirD should be removed.

    The function can assume that the directory exists and is not read-only. 

    The function should leave with a suitable error code if it cannot complete
    successfully for any reason. 
    
    @param aName A reference to a descriptor containing the full name of
                 the directory to be removed.
    
    @leave KErrInUse dirD contains entries other than the parent (..)
           and current (.) entries.
    */
	virtual void RmDirL(const TDesC& aName) =0;


    /**
    Deletes the specified file from the mount.

    The function can assume that the file is closed.

    The file name specified by aName is of the form:
    @code
    \\dirA\\dirB\\dirC\\file.ext
    @endcode
    
    The extension is optional.

    The function should leave on error detection, with
    an appropriate error code.

	@param aName A reference to a descriptor containing the full path name
	             of the file that will be removed.
	
	@leave KErrAccessDenied aName specifies a file whose attributes state that
	       the file is read-only or aName specifies a directory.
    */
	virtual void DeleteL(const TDesC& aName) =0;


    /**
    Renames or moves a single file or directory on the mount.

    It can be used to move a file or directory since both
    anOldName and anNewName specify the respective entries with full names;
    for example,
    @code
    \\dirA\\dirB\\dirC\\oldEntryName
    @endcode
    
    and
    
    @code
    \\dirE\\dirF\\dirG\\newEntryName
    @endcode

    If oldEntryName is a file, it can be assumed that it is closed.
    If oldEntryName is a directory, it can be assumed that there are no
    open files in this directory. Furthermore, if newEntryName specifies
    a directory, it can be assumed that it is not a subdirectory of oldEntryName.

    The function should leave with an appropriate error code if it cannot
    complete successfully for any reason. 

	@param anOldName A reference to a descriptor containing the full entry
	                 name of the entry to be renamed.

	@param anNewName A reference to a descriptor containing the new full entry
	                 name for the entry to be renamed.
 
    @leave KErrAlreadyExists The new entry already exists.
    */
	virtual void RenameL(const TDesC& anOldName,const TDesC& anNewName) =0;


    /**
    Replaces one file on the mount with another.

    The function can assume that both anOldName and, if it exists, anNewName
    contain the full file names of files, and that these files are not open.

    If the file aNewName does not exist it should be created.

    The file anOldName should have its contents, attributes, and the universal
    date and time of its last modification, copied to the file aNewName,
    overwriting any existing contents and attribute details.
    Finally anOldName should be deleted.

    The function should leave with an appropriate error code if it cannot
    complete successfully for any reason.

    @param anOldName A reference to a descriptor containing the full file name
                     of the file to replace the file specified by anNewName
    @param anNewName A reference to a descriptor containing the new full file
                     name for the entry to be replaced.
    */
	virtual void ReplaceL(const TDesC& anOldName,const TDesC& anNewName) =0;


    /**
    Gets the entry details for the specified file or directory.

    anEntry should be filled with details from the file or directory with the
    full name aName. aName is of the form
    @code
    \\dirA\\dirB\\dirC\\entry.
    @endcode
    
    Note that anEntry.iType (the entry UID) should only be set for a file whose
    size is greater than or equal to sizeof(TCheckedUid).

    The function should leave with an appropriate error code if it cannot
    complete successfully for any reason.

    @param aName   A reference to a descriptor containing the full name of
                   the entry whose details are required.
    @param anEntry On return, a reference to the filled entry object.
    
    @leave KErrPathNotFound The entry, aName, cannot be found.
    */
	virtual void EntryL(const TDesC& aName,TEntry& anEntry) const =0;


    /**
    Sets entry details for a specified file or directory.

    The entry identfied by the full name descriptor aName should have
    its modification time and its attributes mask updated as required.

    The entry receives a new universal modified time from aTime.
    The entry attributes are set with aSetAttMask and cleared
    with aClearAttMask:
    the bits that are set in aSetAttMask should be set
    in the entry attribute mask;
    the bits that are set in aClearAttMask
    should be cleared from the entry attribute mask.

    The function can assume that aSetAttMask and aClearAttMask do not change
    the type of attribute (i.e. volume or directory). Furthermore, if aName
    specifies a file, it can be assumed that this file is closed.

    The function should leave with an appropriate error code on error detection.
    
    @param aName         A reference to a descriptor containing the full name of
                         the entry to be updated.
    @param aTime         A reference to the time object holding the new universal
                         modified time for aName.
    @param aSetAttMask   Attribute mask for setting the entry's attributes.
    @param aClearAttMask Attribute mask for clearing the entry's attributes.
    */
	virtual void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask) =0;


    /**
    Customises the opening of a new or existing file on the mount.

    The function is called internally (via TDrive::FileOpen()) as a result of
    a call by the client, and the file is created, if necessary, and opened by
    the calling function. However this function implements any replacement
    functionality, as well as any other behaviour particular to the file system.

    If anOpen specifies EFileReplace (rather than EFileCreate or EFileOpen) then,
    if replacement functionality is required, the data contained in the file
    should be discarded, the archive attribute should be set, and the size of
    the file should be set to zero. Note that it can be assumed that if anOpen
    specifies EFileReplace then the file already exists.

    After successful completion of the function, the file control block pointer
    will be added to the file server's global files container.

    The function should leave with a suitable error code if it cannot be completed
    successfully.

    @param aName  The full name of the file that will be opened.
    @param aMode  The file share mode. The following share modes are available:
                  EFileShareExclusive;
                  EFileShareReadersOnly;
                  EFileShareAny;
                  EFileShareReadersOrWriters;
                  EFileStream;
                  EFileStreamText;
                  EFileRead;
                  EFileWrite.
    @param anOpen IndicatES how the file will be opened. It can be one of
                  the following:
                  EFileOpen;
                  EFileCreate;
                  EFileReplace.
    @param aFile  Pointer to the file control block which will, on success,
                  represent the open file.
                  
    @leave KErrAccessDenied aName may specify a directory, or the function may
           be attempting to open a file on a ROM drive.
    */
	virtual void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile) =0;


    /**
    Customises the opening of a directory on the mount.

    The function is called internally, and the directory will have been created
    and initialised by the calling function. Any customisation specific to
    a file system should be implemented in this function.

    Note that aName is of the form
    @code
    \\dirA\\dirB\\dirC\\file.ext
    @endcode
    
    where \\dirA\\dirB\\dirC\\ is the directory to be opened and file.ext is
    an optional entry name and extension.

    After successful completion of the function, the directory control block
    pointer will be added to the file server global directories container.

    The function should leave with a suitable error code if it cannot complete
    successfully for any reason.

    @param aName A reference to a descriptor containing the full name of
                 the directory that will be opened.
    @param aDir  Points to a directory control block which will, on success,
                 represent the open directory.
    */
	virtual void DirOpenL(const TDesC& aName,CDirCB* aDir) =0;


    /**
    Reads the specified length of data from the specified position on
    the volume directly into the client thread.

    It can be assumed that if this function is called,
    then there has been a successful mount.

    This function should leave with an appropriate error code when
    an error is detected.

    @param aPos     Start position in the volume for the read operation,
                    in bytes.
    @param aLength  The number of bytes to be read.
    @param aTrg     A pointer to the buffer into which data is to be read.
    @param anOffset The offset at which to start adding data to the read buffer.
    @param aMessage
    */
	virtual void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const = 0;


    /**
    Writes a specified length of data from the client thread to the volume
    at the specified position.

    It can be assumed that if this function is called, then there has been
    a successful mount.

    This function should leave with an appropriate error code when
    an error is detected.

    @param aPos     Start position in the volume for the write operation,
                    in bytes.
    @param aLength  The number of bytes to be written.
    @param aSrc     Pointer to the buffer from which data will be written.
    @param anOffset The offset in the buffer at which to start writing data.
    @param aMessage
    */
	virtual void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage) = 0;


    /**
    Gets the short name of the file or directory with the given full name.

    This function is used in circumstances where a file system mangles
    Symbian OS natural names, in order to be able to store them on
    a file system that is not entirely compatible.

    The function should leave with a suitable error code if it cannot complete
    successfully for any reason.

	@param aLongName  A reference to a descriptor containing the full name
	                  of the entry.
	@param aShortName On return, a reference to a descriptor containing
	                  the short name of the entry.
    
    @leave KErrNotFound The entry specified by its long name cannot be found.
    */
	virtual void GetShortNameL(const TDesC& aLongName,TDes& aShortName) = 0;


    /**
    Gets the long name of the file or directory associated with
    the given short name.

    This function is used in circumstances where a file system mangles
    Symbian OS natural names in order to be able to store them on
    a file system that is not entirely compatible. 

    The function should leave with a suitable error code if it cannot complete
    successfully for any reason.

	@param aShorName  A reference to a descriptor containing the short name
	                  of the entry.

    @param aLongName  On return, a reference to a descriptor containing
                      the long name of the entry.

    @leave KErrNotFound The entry specified by its short name cannot be found.
    */
	virtual void GetLongNameL(const TDesC& aShorName,TDes& aLongName) = 0;


    /**
    Reads a specified section of the file, regardless of the file's lock state.

    The function should leave with a suitable error code if it cannot complete
    successfully for any reason.
    
    @param aName   A reference to a descriptor containing the full name of
                   the file to be read from
    @param aPos    The byte position to start reading from.
    @param aTrg    A pointer to the buffer into which data is to be read.
    @param aLength The length of data to be read, in bytes.
    @param aMessage

	@leave KErrEof aPos is past the end of the file.
    */
	virtual void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)=0;


    /**
    Checks the integrity of the file system on the volume and returns an appropriate error value. 
    The default implementation must be overridden by a derived class.
    
    @return KErrNone if the file system is stable; otherwise one of the other system wide error codes.
            The default implementation returns KErrNotSupported.
    */
    virtual TInt CheckDisk() {return(KErrNotSupported);}
    
    /**
    The same as original CheckDisk(), but with some parameters.
    */
    virtual TInt CheckDisk(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);

	
	/**
    Scans through and corrects errors found in the volume.

    The default implementation must be overridden by a derived class.
    
    @return KErrNone if no errors are found or all errors are corrected; otherwise one of the other system wide error codes.
            The default implementation returns KErrNotSupported.
    */
	virtual TInt ScanDrive() {return(KErrNotSupported);}

    /**
    The same as original ScanDrive(), but with some parameters.
    */
    virtual TInt ScanDrive(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);
    
    IMPORT_C virtual void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	
	
	/**
        Low-level control IO
    */
	virtual TInt ControlIO( const RMessagePtr2& /*aMessage*/,TInt /*aCommand*/,TAny* /*aParam1*/,TAny* /*aParam2*/)  {return(KErrNotSupported);}


	/**
	Locks a media which supports password protection and replaces
	the old password with a new one. 

    If aStore is set to ETrue, then the new password should be saved to
    the password store file, KMediaPWrdFile, using the exported file server
    function WriteToDisk(). 

    The password file is used to initialise the password store on boot up,
    so the user does not need to be prompted for the password again if
    it is saved here.

    The default implementation must be overridden in a derived class.

    @param aOld   A reference to the old password.
    @param aNew   A reference to the new password.
    @param aStore ETrue if the new password is to be saved to 
                  the password file store; EFalse if not.

    @return KErrNone if successful; otherwise another of the system wide
            error codes. The default implementation returns KErrNotSupported.
    */
	virtual TInt Lock(TMediaPassword& /*aOld*/,TMediaPassword& /*aNew*/,TBool /*aStore*/) {return(KErrNotSupported);}


	/**
	Unlocks a media which supports password protection.

    If aStore is set to ETrue then the password should be saved to
    the password store file specified by KMediaPWrdFile using the exported file
    server function WriteToDisk().

    The password file is used to initialise the password store on boot up,
    so the user does not need to be prompted for the password again if
    it is saved here.

    The default implementation must be overridden in a derived class.

    @param aPassword A reference to the password.
    @param aStore    ETrue if the password is to be saved to
                     the password store file; EFalse otherwise.
                     
    @return KErrNone if successful; otherwise another of the system wide
            error codes. The default implementation returns KErrNotSupported.                     
    */
	virtual TInt Unlock(TMediaPassword& /*aPassword*/,TBool /*aStore*/) {return(KErrNotSupported);}


	/**
	Clears a password from a media that supports write protection. 

    The default implementation must be overridden in a derived class.

    @param aPassword A reference to the password to be cleared.

    @return KErrNone if successful; otherwise another of the system wide
            error codes. The default implementation returns KErrNotSupported.
    */
	virtual TInt ClearPassword(TMediaPassword& /*aPassword*/) {return(KErrNotSupported);}


	/**
    */
	virtual TInt ForceRemountDrive(const TDesC8* /*aMountInfo*/,TInt /*aMountInfoMessageHandle*/,TUint /*aFlags*/) {return(KErrNotSupported);}

    
    /**
        Legacy method: finalise the mount and put it to the consistent state.
    */
	virtual void FinaliseMountL() {return;}
    
    /** 
        finalise the mount and put it to the consistent state.

        @param  aOperation  describes finalisation operation, see RFs::TFinaliseDrvMode
        @param  aParam1     not used, for future expansion
        @param  aParam2     not used, for future expansion
   */
    virtual	void FinaliseMountL(TInt aOperation, TAny* aParam1=NULL, TAny* aParam2=NULL);
    
    
    protected:
    /** Mount Control levels or operations to perform */
    enum TMntCtlLevel 
        {
        //-- reserved generic mount (CMountCB) control codes
        
        EMountStateQuery,       ///< query mount state, see TMntCtlOption, ESQ_IsMountFinalised
        EMountVolParamQuery,    ///< mount-specific queries for volume parameters. See ESQ_RequestFreeSpace, ESQ_GetCurrentFreeSpace 
        ECheckFsMountable,      ///< extended mount functionality, checks if this file system can be mounted on specified drive. See CheckFileSystemMountable()
        
        //-- starting from the next code someone may define some specific mount type control codes, like ESpecificMountCtl+17
        ESpecificMountCtl = 0x40000000,

        //-- starting from the next code someone may define some specific File System control codes
        ESpecificFsCtl = 0x40001000,
        
        EMountFsParamQuery,     ///< File System parameters queries; File System properties can be "static" i.e not requiring properly mounted volume. See ESpecificFsCtlOpt

        };

    /** Mount Control options that makes sense only for certain control codes, see TMntCtlLevel */
    enum TMntCtlOption
        {
        //-- reserved generic mount (CMountCB) control options codes
        
        /** query if the mount is finalised, corresponds to the EMountStateQuery control code only. @see IsMountFinalised() */
        ESQ_IsMountFinalised, 

        //-----------------------------------------------------------------------------------------------------------------------------
        
        //-- starting from the next code someone may define some specific mount type control options
        ESpecificMountCtlOpt = 0x40000000,

        /** Corresponds to EMountVolParamQuery. Request a certain amount of free space on the volume. @see RequestFreeSpace() */
        ESQ_RequestFreeSpace,
        
        /** Corresponds to EMountVolParamQuery. A request to obtain the _current_ amount of free space on the volume asynchronously, without blocking. */
        ESQ_GetCurrentFreeSpace,
        
        /** Corresponds to EMountVolParamQuery. A request to obtain size of the mounted volume without blocking (CMountCB::VolumeL() can block). */
        ESQ_MountedVolumeSize,

        //-----------------------------------------------------------------------------------------------------------------------------
        
        //-- starting from the next code someone may define some specific File System control options
        ESpecificFsCtlOpt = 0x40001000,

        /** Get Maximum file size, which is supported by the file system that has produced this mount. */
        ESQ_GetMaxSupportedFileSize, 

        };

    
    public:
    
    /**
        Generic mount control method.
        @param  aLevel  specifies the operation to perfrom on the mount
        @param  aOption specific option for the given operation
        @param  aParam  pointer to generic parameter, its meaning depends on aLevel and aOption

        @return standard error code. Default imlementation returns KErrNotSupported
    */
    virtual	TInt MountControl(TInt aLevel, TInt aOption, TAny* aParam);


	/**
	Erase a password from a media that supports write protection. 

    The default implementation must be overridden in a derived class.

    @return KErrNone if successful; otherwise another of the system wide
            error codes. The default implementation returns KErrNotSupported.
    */
	virtual TInt ErasePassword() {return(KErrNotSupported);}

	/** 
	An interface class which may optionally be returned by a file system
	by calling GetInterface(EFileAccessor, ...)
	*/
	class MFileAccessor
		{
	public:
		virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId) = 0;
		virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2) = 0;
		virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2) = 0;
		virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2) = 0;
		};

	/**
	@prototype

	CMountCB::MFileExtendedInterface interface provides extended interface for CMountCB to 
	read a specified section of large file (size greater than 2GB - 1). 

	The interface could be retrieved by calling CMountCB::GetInterface() with 
	EFileExtendedInterface as an argument.

	Sub classes of CMountCB who does support large file access will need to multiple-inherit 
	with this class and implement the interface. The implementation of the interface will be 
	retrieved via GetInterface() and provided to user by non-virtual APIs to avoid breaking
	binary compatibility.

	NOTE: Do not try to delete CMountCB::MFileExtendedInterface interface pointer!

	@see CMountCB::GetInterface()
	*/

	class MFileExtendedInterface
		{
	public:
		/**
	    Reads a specified section of the file, regardless of the file's lock state.
	    
	    The function should leave with a suitable error code if it cannot complete
	    successfully for any reason.
	    
		This function should be implemented in file systems supporting files 
		of size greater than 2GB - 1.
		
	    @param aName   A reference to a descriptor containing the full name of
	                   the file to be read from
	    @param aPos    The byte position to start reading from.
	    @param aTrg    A pointer to the buffer into which data is to be read.
	    @param aLength The length of data to be read, in bytes.
	    @param aMessage

		@leave KErrEof aPos is past the end of the file.
		
		@see CMountCB::ReadSectionL()
		*/
		virtual void ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage) = 0;
		};
	
	/**
	Enumeration of the aInterfaceIDs used in GetInterface.
	*/
	enum TInterfaceIds
		{
		EAddFsToCompositeMount = 0,
		EGetLocalDrive = 1,
		EFileAccessor = 2,
		EGetFileSystemSubType = 3,
		EGetClusterSize = 4,
		ELocalBufferSupport = 5,
		EAddToCompositeMount = 6,
		EGetProxyDrive = 7,
		EFileExtendedInterface = 8
		};

	// File clamping support
	TInt ClampFile(const TInt aDriveNo,const TDesC& aName,TAny* aHandle);
	TInt UnclampFile(RFileClamp* aHandle);
	IMPORT_C TInt IsFileClamped(const TInt64 aUniqueId);
	TInt NoOfClamps();

	// File accessor support
	TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

	// Extensions of interface
	TInt FileSystemSubType(TDes& aName);
	TInt FileSystemClusterSize();
	void FileSystemName(TDes& aName);

	// Large file support 
	void ReadSection64L(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);

    inline TInt CheckFileSystemMountable();
    inline TInt RequestFreeSpace(TUint64 &aFreeSpaceBytes);
    inline TInt MountedVolumeSize(TUint64& aVolSizeBytes);
    inline TInt GetCurrentFreeSpaceAvailable(TInt64 &aFreeSpaceBytes);
    inline TInt IsMountFinalised(TBool &aFinalised);
    inline TInt GetMaxSupportedFileSize(TUint64 &aSize);
	
protected:
	inline void SetMountNumber(TInt aMountNumber);
	inline void SetDismounted(TBool aDismounted=ETrue);
	inline TInt MountNumber() const;
	inline TBool IsDismounted() const;

	void SetProxyDriveDismounted();
	TBool ProxyDriveDismounted();

    IMPORT_C CFileSystem* FileSystem() const;

	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	
	// calls GetInterface() with tracepoints added
	TInt GetInterfaceTraced(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);


    

private:
    void SetFileSystem(CFileSystem* aFS);

    //-- these factory methods mus be used to produce objects representing files, directories etc. as soon as all these objects are
    //-- associated with the mount, not the file sytem (file system is a factory for corresponding mounts)
    //-- corresponding CFileSystem:: methods must not be used. 
    //-- CMountCB has a reference to the CFileSystem that produced it. 
    CFileCB* NewFileL() const;
    CDirCB* NewDirL() const;
    CFormatCB* NewFormatL() const;

protected:

	TInt iMountNumber;  ///< Unique mount number set by the TDrive object representing the drive on which the object resides.
	TUint iUniqueID;    ///< volume Unique ID. Set in MountL().
	TInt64 iSize;       ///< Size of the volume. First set in MountL().

    /**
    A list of all open files on that mount.
    Set by the TDrive object representing the drive of which the mount resides.
    */
	TDblQue<CFileCB> iMountQ;
	
    friend class TDrive;
	friend class TFsAddCompositeMount;

private:
	TInt        iLockMount;
	TDrive*     iDrive;
	HBufC*      iVolumeName;
	CMountBody* iBody;          ///< used for extending CMountCB functionality
	};


/**
@internalTechnology

MFileSystemSubType interface provides extended interface for CMountCB to retrieve sub type
of mounted file systems.

The interface could be retrieved by calling CMountCB::GetInterface() with EGetFileSystemSubType
as an argument.

If the file system does not support sub types, MFileSystemSubType cannot be retieved.
Sub classes of CMountCB who does support sub types will need to multiple-inherit with
this class and implement the interface. The implementation of the interface will be 
retrieved via GetInterface() and provided to user by non-virtual APIs to avoid breaking
binary compatibility.

NOTE: Do not try to delete MFileSystemSubType interface pointer!

@see CMountCB::GetInterface()
*/
class MFileSystemSubType
	{
public:
	/**
	Retrieves file system's sub type name (E.g. FAT16), if the file system does not have sub 
	types (E.g. Rofs), return the file system's name.
	@param aName Returned descriptor contains file system name or sub type name.
	@return KErrNone if successful. 
	*/
	virtual TInt SubType(TDes& aName) const = 0;
	};

/**
@internalTechnology

MFileSystemClusterSize interface provides extended interface for CMountCB to retrieve cluster size
of mounted file systems.

The interface could be retrieved by calling CMountCB::GetInterface() with EGetClusterSize
as an argument.

If the file system does not support clustering, MFileSystemClusterSize cannot be retieved.
Sub classes of CMountCB who does support clustering will need to multiple-inherit with
this class and implement the interface. The implementation of the interface will be 
retrieved via GetInterface() and provided to user by non-virtual APIs to avoid breaking
binary compatibility.

NOTE: Do not try to delete MFileSystemSubType interface pointer!

@see CMountCB::GetInterface()
*/
class MFileSystemClusterSize
	{
public:
	/**
	Retrieves file system's cluster size
	@return None-zero cluster size if successful.
	*/
	virtual TInt ClusterSize() const = 0;
	};


class CFileShare;



/**
@internalTechnology
*/
class TAsyncReadRequest
	{
public:
	TAsyncReadRequest(TInt64 aEndPos, CFileShare* aOwningShareP, CFsRequest* aRequestP);
	TBool CompleteIfMatching(CFileShare* aOwningShareP, TRequestStatus* aStatusP, TInt aError);
private:
	TAsyncReadRequest();
public:
	TInt64 iEndPos;					// The request is completed file length >= iEndPos.
	CFileShare* iOwningShareP;		// The share that owns this outstanding request.
	const TRequestStatus* iStatusP;	// Used to identify the request when cancelling.
	CSessionFs* iSessionP;			// The owning session of the original request.
	RMessage2 iMessage;				// The message to be completed when data is available.
	};


/**
    @internalTechnology
    @released

    File share lock

    The lock specifies the lowest and highest position in the file to be locked.
    Note that files may have many locks on it, but overlapping sections cannot be locked.
    This is used by a file control block, a CFileCB object.

    @see CFileCB
*/
class TFileShareLock
	{
public:
    TFileShareLock(const CFileShare* aOwner, TUint64 aPosLow, TUint64 aPosHigh);

   
    inline TUint64  PosLow()  const; 
    inline TUint64  PosHigh() const; 
    inline TBool MatchOwner(const CFileShare* aShare) const;

    TBool MatchByPos(TUint64 aPosLow, TUint64 aPosHigh) const;

private:
    TFileShareLock();
    TFileShareLock(const TFileShareLock&);
    TFileShareLock& operator=(const TFileShareLock&);

private:

	const CFileShare* iOwner;   ///<The owning file share object.
	TUint64 iPosLow;            ///<The start of the section of the file to be locked.
	TUint64 iPosHigh;           ///<The end of the section of the file to be locked.

    friend class CFileCB;	
    };

/** @internalTechnology */
typedef RArray<TFileShareLock> TFileLocksArray;


/**
@publishedPartner
@released

A file server interface class representing an open file.

An instance of this object is referred to as a file control block.

A file control block needs to be created for a specific file to be able to
access that file within a directory.

A plug-in file system implements this class.
*/
class CFileCB : public CFsDispatchObject
	{
public:
	IMPORT_C CFileCB();
	IMPORT_C ~CFileCB();
	
    IMPORT_C void InitL(TDrive* aDrive,TDrive* aCreatedDrive, HBufC* aName);
	
    inline void SetMount(CMountCB * aMount);
	inline TDrive& Drive() const;
	inline TDrive& CreatedDrive() const;
	inline CMountCB& Mount() const;
	inline HBufC& FileName() const;
	inline HBufC& FileNameF() const;
    inline TInt UniqueID() const;
	TInt FindLock(TInt aPosLow,TInt aPosHigh);
	TInt AddLock(CFileShare* aFileShare,TInt aPos,TInt aLength);
	TInt RemoveLock(CFileShare* aFileShare,TInt aPos,TInt aLength);
	TInt CheckLock(CFileShare* aFileShare,TInt aPos,TInt aLength);
	void RemoveLocks(CFileShare* aFileShare);
	inline TShare Share() const;
	inline void SetShare(TShare aShare);
	inline TInt Size() const;
	inline void SetSize(TInt aSize);
	inline TInt Att() const;
	inline void SetAtt(TInt aAtt);
	inline TTime Modified() const;
	inline void SetModified(TTime aModified);
	inline TBool FileCorrupt() const;
	inline void SetFileCorrupt(TBool aFileCorrupt);
	inline TBool BadPower() const; 
	inline void SetBadPower(TBool aBadPower);
	inline TUint32 NameHash() const;
	TInt CheckMount();
	inline TInt BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos=-1);
	inline TInt LocalDrive(TBusLocalDrive*& aLocalDrive);

	TBool LocalBufferSupport() const;
	void SetLocalBufferSupport(TBool aEnabled);

	/** File caching support methods */

	CFileCache* FileCache() const;
	TInt FairSchedulingLen() const;
	void ResetReadAhead();

	void SetDeleteOnClose();
	TBool DeleteOnClose() const;
 
	
	void SetNotifyAsyncReadersPending(TBool aNotifyAsyncReadersPending);
	TBool NotifyAsyncReadersPending() const;
	TInt CancelAsyncReadRequest(CFileShare* aShareP, TRequestStatus* aStatusP);

	/** Extended API support methods */

	TBool ExtendedFileInterfaceSupported();
	void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	void SetSizeL(TInt64 aSize);

	IMPORT_C TInt64 Size64() const;
	IMPORT_C void SetSize64(TInt64 aSize, TBool aDriveLocked);
    IMPORT_C void SetMaxSupportedSize(TUint64 aMaxFileSize);
	IMPORT_C TBool DirectIOMode(const RMessagePtr2& aMessage);


    TInt64 CachedSize64() const;
	void SetCachedSize64(TInt64 aSize);
	TInt FindLock64(TInt64 aPosLow,TInt64 aPosHigh);
	TInt AddLock64(CFileShare* aFileShare,TInt64 aPos,TInt64 aLength);
	TInt RemoveLock64(CFileShare* aFileShare,TInt64 aPos,TInt64 aLength);
	TInt CheckLock64(CFileShare* aFileShare,TInt64 aPos,TInt64 aLength);

	/** Sequential mode */
	
	IMPORT_C TBool IsSequentialMode() const;
	void SetSequentialMode(TBool aSequential);

	/**
	The FileShare List contains the file shares of an open file.
	*/
	TDblQue<CFileShare>& FileShareList() const;
	void AddShare(CFileShare& aFileShare);

    /**
    Renames the file with the full file name provided.

    Because the full name of the file includes the path, the function can
    also be used to move the file.

    It can be assumed that no other sub-session has access to the file:
    i.e. the file has not been opened in EFileShareAny share mode.
    It can also be assumed that the file has been opened for writing. 

    The function should leave with KErrAlreadyExists if aNewName already exists.
    An appropriate error code should also be generated if the function leaves
    before completion for any other reason.

	@param aNewName The new full name of the file.
	
	@see CFileCB::iFileName
    */
	virtual void RenameL(const TDesC& aNewName) =0;
	
	
    /**
    Reads a specified number of bytes from the open file starting at
    the specified postition, and writes the result into a descriptor.

    It can be assumed that aPos is inside the file and aLength > 0.
    The file should only be read up to its end regardless of
    the value of aPos + aLength. The number of bytes read should be stored
    in aLength on return.

    If the function leaves before completion for any reason it should generate
    an appropriate error code, and in this situation,
    the arguments are not valid on return.

	@param aPos     Represents a position relative to the start of the file
	                where ReadL() should start to read.
	@param aLength  On entry, specifies the number of bytes to be read
	                from the file. On return, this should contain the number
	                of bytes read, but this is not valid if the function leaves.
	@param aDes     Pointer to a descriptor into which the data should be written.
	@param aMessage
    */
    virtual void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage) =0;
	
	
    /**
    Writes data to the open file.

    iModified and iSize are set by the file server after this function
    has completed successfully.

    It can be assumed that aPos is within the file range and aLength > 0.
    When aPos + aLength is greater than the file size then the file should
    be enlarged using SetSizeL(). The number of bytes written should be
    returned through the argument aLength. 

    If the function leaves before completion for any reason it should generate
    an appropriate error code, and in this situation the arguments are
    not valid on return.

   	@param aPos     Represents a position relative to the start of the file
   	                where WriteL() should start to write.
	@param aLength  Specifies the number of bytes to be written to the file.
	                On return, the number of bytes written, but this is not
	                valid if the function leaves.
	@param aDes     Pointer to a descriptor containing the data to be written
	                to the file.
	@param aMessage 
	
	@see CFileCB::iModified
	@see CFileCB::iSize
	@see CFileCB::SetSizeL
	
    @leave KErrDiskFull The operation cannot be completed because the disk is full.
    */
	virtual void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage) =0;
	
	
    /**
    Extends or truncates the file by re-setting the file size.

    The function should not change iModified and iSize attributes of
    the file object: this is done by the file server.
    If the file is extended, nothing should be written in the extended area.

    The function should leave with a suitable error code on error detection.

    @param aSize The new file size in number of bytes.
    
    @leave KErrDiskFull The operation cannot be completed because the disk is full.
    
    @see CFileCB::iModified
	@see CFileCB::iSize
    */
	virtual void SetSizeL(TInt aSize) =0;
	
	
    /**
    Sets the attribute mask, iAtt, and the modified time of the file, iModified.

    If aMask|aVal does not equal zero, then aMask should be OR'ed with iAtt,
    whilst the inverse of aVal should be AND'ed with iAtt.
    If the modified flag is set in aMask then iModified should be set to aTime.

    The function should leave with a suitable error code on error detection.

	@param aTime The new modified time, if the modified flag is set in aMask.
	@param aMask Bit mask containing bits set (to 1) that are to be set (to 1)
	             in iAtt.
	@param aVal  Bitmask containing bits set (to 1) that are to be unset (to 0)
	             in iAtt.
	
    @see CFileCB::iModified
	@see CFileCB::iAtt
    */
    virtual void SetEntryL(const TTime& aTime,TUint aMask,TUint aVal) =0;
	
	
    /**
    Flushes, to disk, the cached information necessary for the integrity
    of recently written data, such as the file size.

    The function should leave with a suitable error code on error detection.
    */
	virtual void FlushDataL() =0;
	
	
    /**
    Flushes, to disk, all cached file data (e.g. attributes, modification time,
    file size). 

    The modified bit in the file attributes mask should be cleared if
    the flush was successful.

    The function should leave with a suitable error code on error detection.
    */
	virtual void FlushAllL() =0;
	IMPORT_C virtual TInt Address(TInt& aPos) const;
	IMPORT_C void SetArchiveAttribute();

	/**
	Block Map API interface 
	*/
	class MBlockMapInterface
		{
	public:
		virtual TInt BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos)=0;
		};
		
	/** 
	An interface class which may optionally be returned by a file system
	by calling GetInterface(EExtendedFileInterface, ...)
	The purpose of this interface is twofold:
	- to support fair scheduling (by use of the aOffset parameter)
	- to enable large file support
	*/
	class MExtendedFileInterface
		{
	public:
		/** 
		Functionally equivalent to CFileCB::ReadL(), but supports large files and fair scheduling

		Reads a specified number of bytes from the open file starting at
		the specified postition, and writes the result into a descriptor.

		@param aPos     Represents a position relative to the start of the file
						where ReadL() should start to read. 
						Note that the filesystem may not support positions above KMaxTInt,
						in which case it leaves with KErrNotSupported.
		@param aLength  On entry, specifies the number of bytes to be read
						from the file. On return, this contains the number
						of bytes read, this value is not valid if the function leaves.
		@param aDes     Pointer to a descriptor into which the data is written.
		@param aMessage A reference to a client message or an RLocalMessage.
		@param aOffset	The offset into the descriptor where the data is to be written.
						This is non-zero if the read was fair-scheduled
						
		@see CFileCB::ReadL		
		@see RLocalMessage				
		*/
		virtual void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset) = 0;
		
		/** 
		Functionally equivalent to CFileCB::WriteL(), but supports large files and fair scheduling

	    Writes data to the open file.

   		@param aPos     Represents a position relative to the start of the file
   						where WriteL() starts to write.
						Note that the filesystem may not support positions above KMaxTInt,
						in which case it leaves with KErrNotSupported.
		@param aLength  Specifies the number of bytes to be written to the file.
						On return this is the number of bytes written, this value is not
						valid if the function leaves.
		@param aDes     Pointer to a descriptor containing the data to be written
						to the file.
		@param aMessage A reference to a client message or an RLocalMessage
		@param aOffset	The offset into the descriptor where the data is to be read from.
						This is non-zero if the read was fair-scheduled
						
		@see CFileCB::WriteL
		@see RLocalMessage						
		*/
		virtual void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset) = 0;

		/** 
		Functionally equivalent to CFileCB::SetSizeL(), but supports large files

		Extends or truncates the file by re-setting the file size.

		The function does not change the iModified and iSize attributes of
		the file object: this is done by the file server.
		If the file is extended, nothing is written in the extended area.

		The function leaves with a suitable error code when an error is to detected.

		@param aSize The new file size in bytes.
    
		@leave KErrDiskFull The operation cannot be completed because the disk is full.
    		
    	@see CFileCB::SetSizeL
		@see CFileCB::iModified
		@see CFileCB::iSize
		*/
		virtual void SetSizeL(TInt64 aSize) = 0;
		};


protected:
	
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

	// calls GetInterface() with tracepoints added
	TInt GetInterfaceTraced(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);


   	enum TInterfaceIds
   		{
		EBlockMapInterface = 0,
		EGetLocalDrive = 1,
		EExtendedFileInterface
   		};

    TUint64 MaxSupportedSize(void) const;

    
    inline TFileLocksArray& FileLocks(); 



private:

	void DemoteShare(CFileShare* aFileShare);
	void PromoteShare(CFileShare* aFileShare);

	RArray<TAsyncReadRequest>& AsyncReadRequests();
	TInt AddAsyncReadRequest(CFileShare* aFileShareP, TInt64 aPos, TInt aLength, CFsRequest* aRequestP);
	void NotifyAsyncReaders();

protected:

    /**
    Inititally, the mode that the file was opened with, which defines the level
    of access allowed to the file. Set by the TDrive object
    (representing the drive on which the file resides) when the file
    control block is created.
    */
	TShare iShare;


    /**
    The size of the file. This is the low 32 bit part of the file size.
    The upper 32 bit part of the file size is saved on the file server side 
    for File Systems supporting file size > 4GB - 1.
    File Systems supporting file size > 4GB - 1 shall use CFileCB::Size64() 
    to query the file size and CFileCB::SetSize64() to set file size. 
    */
    TInt iSize;


    /**
    The attributes of the file.
    */
	TInt iAtt;


    /**
    The universal time at which the file was last modified.
    */
	TTime iModified;


    /**
    Indicates whether the file that the object represents is corrupt:
    true if it is corrupt, false otherwise.
    */
	TBool iFileCorrupt;


    /**
    Indicates whether a recent access to the file that the object represents
    failed due to KErrBadPower.
    */
	TBool iBadPower;

public:

	/**
	The full name of the file, including its extension.
	*/
	HBufC* iFileName;

	/**
	The full name of the file, including its extension - Folded.
	*/
	HBufC* iFileNameF;

private:
	TUint32	            iNameHash;
	TDrive*             iCreatedDrive;
	TDrive*             iDrive;
	CMountCB*           iMount;
	TFileLocksArray*    iFileLocks;		// An array of file position locks
	TDblQueLink         iMountLink;
	CFileBody*			iBody;

	friend class TDrive;
	friend class CMountCB;
	friend class CFileShare;
	friend class TFsFileRead;
	friend class TFsFileWrite;
	friend class TFsFileSetSize;
	friend class TFsFileReadCancel;
	friend class TFsFileDuplicate;
	friend class TFsFileRename;
	friend class CCompFileCB;
	friend class CFileCache;
	};


/**
Helper class to construct a dummy RMessage2 object. This allows the file server to 
read and write local buffers to a file system's CFileCB-derived interface.

@internalTechnology
*/
class RLocalMessage : public RMessage2
	{
public:
	inline RLocalMessage();

	inline void InitHandle();
	inline void SetFunction(TInt aFunction);
	inline void SetArgs(TIpcArgs& aArgs);
	inline TInt Arg(TInt aIndex) const;
	};


/**
@publishedPartner
@released

A file server interface class representing an open file that is being shared.
For example multiple reading of the same file.

@see CFileCB
@see TFileMode
*/
NONSHARABLE_CLASS(CFileShare) : public CFsDispatchObject
	{
public:
	CFileShare(CFileCB* aFileCB);
	~CFileShare();
	TInt CheckMount();
	void InitL();
	inline CFileCB& File();

	// override CFsDispatchObject::Close() so that we can flush dirty data
	void Close();

	// For serialising aync requests 
	TBool RequestStart(CFsMessageRequest* aRequest);
	void RequestEnd(CFsMessageRequest* aRequest);
	TBool RequestInProgress() const;
	inline TBool IsFileModeBig();
	
public:
	/**
	File share mode. The mode in which the file was opened first.
	@see TFileMode.
	*/
	TUint iMode;   
	/**
	Current file position. This is the position at which reading and writing takes place.
	*/
	TInt64 iPos;
	/**
	Error condition due to flush.
	*/
	TInt iFlushError;
private:
	CFileCB* iFile;

	// A pointer to the current request. Used for serializing client 
	// async read/write requests which might otherwise be processed out
	// of order due to fair scheduling.
	CFsMessageRequest* iCurrentRequest;
	
	// The FileShare List link object.
	TDblQueLink iShareLink;

friend class CFileBody;	// For access to iShareLink
	};




/**
@publishedPartner
@released

A file server interface class representing an open directory

An instance of this object is referred to as a directory control block.

A directory control block must be created for a specific directory to access
that directory within a volume.

A plug-in file system implements this class.
*/
class CDirCB : public CFsDispatchObject
	{
public:
	IMPORT_C CDirCB();
	IMPORT_C ~CDirCB();
	TInt CheckMount();
	IMPORT_C void InitL(TDrive* aDrive);
	inline void SetMount(CMountCB * aMount){iMount=aMount;};
	inline TDrive& Drive() const;
	inline CMountCB& Mount() const;
	inline TBool Pending() const; 
	inline void SetPending(TBool aPending);
	

    /**
    Gets information from the first suitable entry in the directory,
    starting from the current read position.

    The function should read successive entries until a suitable entry is found.
    An entry is suitable if the entry attributes match the criteria set by this
    object's attributes, which are set on initialisation.
    For example, if the directory control block has the attribute
    KEntryAttMaskSupported, and the file has the attribute KEntryAttVolume,
    then the entry will be deemed unsuitable and the next entry will be read.

    This function is called by the file server.

    If, on return, the entry's full file name, TEntry::iName, is longer than
    the maximum buffer size, then the entry cannot be returned to the client.
    In this case the file server will set iPending to true and will call
    StoreLongEntryName() before calling this function again.
    In this case (when iPending is true), the function should re-read
    the last entry to be read; it should also set iPending to false and
    should not advance the current read position.

    The time stored in the iModified member of anEntry should not be converted,
    but left as UTC time.

    When storing the iName member of anEntry, the current (.),
    or parent marker (..) in the directory should not be returned.

    If the KEntryAttAllowUid flag is set in the iAtt member of anEntry, then
    the entry UID type of an entry will be read. If, on reading the UID from
    a file, KErrCorrupt is generated, because the file is corrupt,
    ReadL() should not leave with this error message, but should return
    as normal.
    If any other errors are raised the function should leave.

    All of the properties of a TEntry, other than the UID types, are always read.

    ReadL() should leave with a suitable error code if it cannot complete
    successfully for any reason. 

    @param anEntry Entry information object.
    */
	virtual void ReadL(TEntry& anEntry) =0;
	
public:
	IMPORT_C virtual void StoreLongEntryNameL(const TDesC& aName);
	
protected:
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	
protected:
    /**
    Bitmask of the attributes of interest.

    Set using the the TDrive friend class instance representing
    the directory's drive after the object is made.
    */
	TUint iAtt;
	
	
	/**
	Set after construction using the TDrive friend class instance representing
	the directory's drive.
	*/
	TUidType iUidType;


    /**
    Flag to indicate whether preceding entry details should be returned when
    multiple entries are being read.
    */
	TBool iPending;
	friend class TDrive;
private:
	TDrive* iDrive;
	CMountCB* iMount;
	TUint32 iReserved;				// Reserved for future expansion
	};




class CFormatCBBody;

/**
@publishedPartner
@released

A file server interface class representing a format operation on a disk.

An instance of this object is referred to as a format control block.

The type of format operation to be applied depends on the type of disk,
and is stored in iMode. Each format operation has a number of steps and
is kept track of using iCurrentStep.

A format control block needs to be created for a specific mount control block
for the disk controlled via that mount to be formatted.

A plug-in file system provides an implementation of this class.
*/

class CFormatCB : public CFsDispatchObject
	{
public:
	IMPORT_C CFormatCB();
	IMPORT_C ~CFormatCB();
	IMPORT_C TInt CheckMount();
	
    void InitL(TDrive* aDrive, TFormatMode aMode);
    
    void SetFormatParameters(const TLDFormatInfo* apLDFormatInfo);
	TInt SetFormatParameters(const TVolFormatParam* apVolFormatParam);


    inline TDrive& Drive() const;
	inline CMountCB& Mount()  const;
	inline TFormatMode Mode()  const; 
	inline TInt& CurrentStep();

    /**
    Performs a formatting step on the drive.

    The step performed should depend on the values of iMode and iCurrentStep. 

    It can be assumed that there are no resources open on the mount,
    that the media is formattable, and that the media is not write protected.

    If iMode == EQuickFormat, then only meta data is to be written.
    This should be carried out in a single step, with iCurrentStep set
    to zero on completion.

    If iMode != EQuickFormat, then the format step performed by
    this function should depend on iCurrentStep. When the function
    returns with iCurrentStep set to zero, the formatting of the drive is complete.

    On error detection, the function should leave with an appropriate error code.

    @see CFormatCB::iMode
    @see CFormatCB::iCurrentStep
    */
	virtual void DoFormatStepL() =0;
	
protected:
	
    /** Enumeration of the aInterfaceIDs used in GetInterface */
	enum TInterfaceIds
		{
		ESetFmtParameters = 1, ///< used in implementation of SetFormatParameters(const TVolFormatParam* apVolFormatParam)
		};



	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
		
protected:

	TInt                  iCurrentStep; ///< current format step counter 100...0
	TFormatMode           iMode;        ///< Format mode. This is set by the file server when this objetc is instantiated.
    TSpecialFormatInfoBuf iSpecialInfo; ///< Buffer containing user-specified format parameters.

private:
	
    TDrive*         iDrive; 
	CMountCB*       iMount;  ///< parent Mount   
    CFormatCBBody*  iBody;   ///< additional data holder
	};




/**
@publishedPartner
@released

A file server interface class representing a raw disk.

An instance of this object is referred to as a raw disk control block.

This is not an abstract base class and does not need to be derived from
when implementing a file system. This is because direct disk access is
implemented by the file server directly calling RawReadL() and RawWriteL()
from the derived CMountCB object of the file system.
*/
NONSHARABLE_CLASS(CRawDiskCB) : public CFsDispatchObject
	{
public:
	CRawDiskCB();
	~CRawDiskCB();
	void InitL(CMountCB* aMount,TBool aIsWriteProtected);
	inline CMountCB& Mount();
	inline TDrive& Drive();
	inline TBool IsWriteProtected() const;
	inline void SetChanged();
private:
	enum { EWriteProtected = 1, EChanged = 2 };
	inline void SetWriteProtected();
	inline TBool IsChanged() const;
private:
	CMountCB* iMount;
	TUint32 iFlags;
	};


class CFileSystemBody;

/**
@publishedPartner
@released

A file server interface class, representing the factory class for a file system.

A plug-in file system implements this class.

Creates objects derived from CMountCB, CFileCB, CDirCB and CFormatCB.

@see CMountCB
@see CFileCB
@see CDirCB
@see CFormatCB
*/
class CFileSystem : public CFsObject
	{
public:
	IMPORT_C CFileSystem();
	IMPORT_C ~CFileSystem();
	IMPORT_C virtual TInt Remove();
	IMPORT_C virtual TBool QueryVersionSupported(const TVersion& aVer) const;
	IMPORT_C virtual TBool IsExtensionSupported() const;
	IMPORT_C TBool IsProxyDriveSupported();
	IMPORT_C void SetLibrary(RLibrary aLib);
	IMPORT_C RLibrary Library() const;
// Pure virtual
    

	/**
    Installs the file system.

    The function should set the name of the file system object through a call
    to CObject::SetName(), thus making it accessible, internally, 
    using FileSystems->FindByFullName(). This enables the file server
    to find and handle installed file systems. 
    The function should also set the file system version.
    The version is determined by the file system implementation.
    It is used in calls to CFileSystem::QueryVersionSupported().

    This function is called as a result of a call to RFs::AddFileSystem().
    
    @return KErrNone if succesful; otherwise one of the other  system-wide error
    codes.
    
    @see RFs::AddFileSystem
    @see CObject::SetName
    @see RFs
    @see CObject
    */
	virtual TInt Install() =0;
	
		
	/**
	Creates a new mount control block, a CMountCB derived object. 

    On success, a pointer to the new mount object should be returned,
    otherwise the function should leave.

	@return A pointer to the new mount object.

    @see CMountCB
	*/
	virtual CMountCB* NewMountL() const =0;
	
	
	/**
	Creates a new file control block, i.e. a CFileCB derived object.

    On success, a pointer to the new file object should be returned,
    otherwise the function should leave.

    @return A pointer to the new file object.
    
    @see CFileCB
	*/
	virtual CFileCB* NewFileL() const =0;
	
	
	/**
	Creates a new directory control block, i.e. a CDirCB derived object.

    On success, a pointer to the new directory control block should be returned,
    otherwise the function should leave.

    @return A pointer to the new directory object.
    
    @see CDirCB
	*/
	virtual CDirCB* NewDirL() const =0;
	
	
	/**
	Creates a new volume format control block, i.e. a CFormatCB derived object.

    On success, a pointer to the new volume format control block should be returned,
    otherwise the function should leave.

    @return A pointer to the new volume format object.
    
    @see CFormatCB
	*/
	virtual CFormatCB* NewFormatL() const =0;
	
	
	/**
	Retrieves drive information.

    The function should set anInfo.iMediaAtt and anInfo.iType according to
    the specified drive number.

    Note that anInfo.iDriveAtt will already have been set by the calling
    function.

    The function can obtain the necessary information by calling
    the appropriate TBusLocalDrive::Caps() function using the argument aDriveNumber.

	@param anInfo       On return, contains the drive information.
	@param aDriveNumber The drive number.
	*/
	IMPORT_C virtual void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
		
    virtual TInt DefaultPath(TDes& aPath) const;

	/** Enumeration of the aInterfaceIDs used in GetInterface.*/
	enum TInterfaceIds
		{
		EProxyDriveSupport = 0,
		EExtendedFunctionality, ///< corresponds to MFileSystemExtInterface
		};
   
    /** This is interface corresponds to the extended CFileSystem functionality*/
    class MFileSystemExtInterface
        {
    public:
        virtual CMountCB* NewMountExL(TDrive* apDrive, CFileSystem** apFileSystem, TBool aForceMount, TUint32 aFsNameHash) = 0;
        virtual TInt GetSupportedFileSystemName(TInt aFsNumber, TDes& aFsName) const = 0;   

        };

public:	
     CMountCB* NewMountExL(TDrive* apDrive, CFileSystem** apFileSystem, TBool aForceMount, TUint32 aFsNameHash);
     TInt GetSupportedFileSystemName(TInt aFsNumber, TDes& aFsName);


protected:	
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	
protected:
	TVersion iVersion;
private:
	RLibrary         iLibrary;  ///< *.fsy file system plugin dll handle
    CFileSystemBody* iBody;     ///< extension of this class, used to provide extended functionality w/o changing this class size (BC issue)

friend class TDrive;

	};




class CProxyDriveBody;	// fwd ref
/**
@publishedPartner
@released

Base abstract class.
Interface between a local plugin file system and a media subsystem.

@see CLocalProxyDrive
@see CBaseExtProxyDrive
*/
class CProxyDrive : public CBase	
	{
public:
	CProxyDrive(CMountCB* aMount);
	~CProxyDrive();		
	inline CMountCB* Mount() const;
	inline void SetMount(CMountCB *aMount);
// virtual
	IMPORT_C virtual TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	IMPORT_C virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	IMPORT_C virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset,TInt aFlags);
	IMPORT_C virtual TInt DeleteNotify(TInt64 aPos, TInt aLength);
	IMPORT_C virtual TInt GetLastErrorInfo(TDes8& aErrorInfo);

	inline TInt LocalBufferSupport();

	TInt SetAndOpenLibrary(RLibrary aLibrary);
	RLibrary GetLibrary();

	
// pure virtual

	/**
	Initialise the proxy drive. 
	
	Derived class must provide an implementation for it.
	
	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	virtual TInt Initialise()=0;
	
	/**
	It invokes Dismounted() on the proxy drive.
	
	Derived class must provide an implementation for it. 
	
	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	virtual TInt Dismounted()=0;
	
	/**
	Increase the size of the proxy drive by the specified length (in bytes).
	
	Derived class must provide an implementation for it. 

	@param aLength The length (in bytes) of which the drive is to be increased by.
	
	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	virtual TInt Enlarge(TInt aLength)=0;
	
	/**
	Reduce the size of the proxy drive by removing the specified length
	(in bytes) starting at the specified position.

	Derived class must provide an implementation for it.

	@param aPos    The start position of area to be removed.
	@param aLength The length/size (in bytes) by which the drive is to be reduced.
	
	@return System-wide error codes based on the status of the operation.
	*/	
	virtual TInt ReduceSize(TInt aPos, TInt aLength)=0;
	
	/**
	Read from the proxy drive.

	Derived class must provide an implementation for it.

	@param aPos    The address from where the read begins.
	@param aLength The length of the read.
	@param aTrg    A descriptor of the memory buffer from which to read.
	@param aThreadHandle The handle-number representing the drive thread.
	@param aOffset Offset into aTrg to read the data from.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)=0;
	
	/**
	Read from the proxy drive.

	Derived class must provide an implementation for it.
	
	@param aPos    The address from where the read begins.
	@param aLength The length of the read.
	@param aTrg    A descriptor of the memory buffer from which to read.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg)=0;
	
	/**
	Write to the proxy drive.

	Derived class must provide an implementation for it.
	
	@param aPos    The address from where the write begins.
	@param aLength The length of the write.
	@param aSrc    A descriptor of the memory buffer from which to write.
	@param aThreadHandle The handle-number representing the drive thread.
	@param aOffset Offset into aSrc to write the data to.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)=0;
	
	/**
	Write to the proxy drive.
	
	Derived class must provide an implementation for it.
	
	@param aPos    The address from where the write begins.
	@param aSrc    A descriptor of the memory buffer from which to write.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc)=0;
	
	/**
	Get the proxy drive's capabilities information.
	
	Derived class must provide an implementation for it.

	@param anInfo A descriptor of the connected drives capabilities.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Caps(TDes8& anInfo)=0;
	
	/**
	Format the connected drive.
	
	Derived class must provide an implementation for it.
	
	@param anInfo Device specific format information.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Format(TFormatInfo& anInfo)=0;
	
	/**
	Format the proxy drive.

	Derived class must provide an implementation for it.
	
	@param aPos    The position of the data which is being formatted.
	@param aLength The length of the data which is being formatted.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Format(TInt64 aPos,TInt aLength)=0;
	
	/**
	Set the mount information on the proxy drive.
	
	Derived class must provide an implementation for it.
	
	@param aMountInfo Information passed down to the media driver. 
					  The meaning of this information depends on the media driver.
	@param aMountInfoThreadHandle  Message thread handle number.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle=KCurrentThreadHandle)=0;
	
	/**
	Forces a remount on the proxy drive
	
	Derived class must provide an implementation for it.
	
	@param aFlags Flags to be passed into the driver.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt ForceRemount(TUint aFlags=0)=0;
	
	/**
	Unlocks a password-enabled proxy drive.

	Derived class must provide an implementation for it.

	@param aPassword A descriptor containing the existing password.
	@param aStorePassword If ETrue, the password is added to the password store.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Unlock(TMediaPassword &aPassword, TBool aStorePassword)=0;
	
	/**
	Locks a password-enabled proxy drive with the new password.
	
	Derived class must provide an implementation for it.

	@param aOldPassword A descriptor containing the existing password.
	@param aNewPassword A descriptor containing the new password.
	@param aStorePassword If ETrue, the password is added to the password store.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword)=0;
	
	/**
	Clears a password from a proxy drive - controller sets password to null.

	Derived class must provide an implementation for it.

	@param aPassword A descriptor containing the password.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt Clear(TMediaPassword &aPassword)=0;
	
	/**
	Forcibly unlock a password-enabled proxy drive.
	
	Derived class must provide an implementation for it.
	
	@return System-wide error codes based on the status of the operation.
	*/
	virtual TInt ErasePassword()=0;

// implementation using GetInterface(..)
	enum TInterfaceIds
		{
		EGetLocalDrive,
		ELocalBufferSupport,
		EGetProxyDrive,
		EFinalised,
		};

	/**
	Retrieves TBusLocalDrive object associated with the file.
	
	@return System-wide error codes based on the status of the operation.
	*/
	IMPORT_C TInt GetLocalDrive(TBusLocalDrive*& aLocDrv);
	
	/**
	Informs the extension that the mount has been finalised and is in a consistent state.
	
	@return System-wide error codes based on the status of the operation.

	@internalTechnology
	*/
	IMPORT_C TInt Finalise(TBool aFinalised);

protected:
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
		
private:
	CMountCB* iMount;
	CProxyDriveBody* iBody;

friend class LocalDrives;
	};




/**
@publishedPartner
@released

Local drive specific mount control block.
*/
class CLocDrvMountCB : public CMountCB
	{
public:
	IMPORT_C CLocDrvMountCB();
	IMPORT_C ~CLocDrvMountCB();			
	IMPORT_C TInt CreateLocalDrive(TBusLocalDrive& aLocDrv);
	IMPORT_C TInt CreateDrive(TInt aDriveNumber);
	IMPORT_C TInt InitLocalDrive();
	IMPORT_C void DismountedLocalDrive();
	inline CProxyDrive* LocalDrive() const;	

private:
	CProxyDrive* iProxyDrive;
	};





/**
@publishedPartner
@released

Local drive specific proxy drive interface.
Class passes commands directly to TBusLocalDrive.

@see CProxyDrive   
*/
NONSHARABLE_CLASS(CLocalProxyDrive) : public CProxyDrive
	{
public:	
	static CLocalProxyDrive* New(CMountCB* aMount,TBusLocalDrive& aLocDrv);
// virtual
	virtual TInt Initialise();	
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset, TInt aFlags);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Caps(TDes8& anInfo);
	virtual TInt Format(TFormatInfo& anInfo);
	virtual TInt Format(TInt64 aPos,TInt aLength);
	virtual TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle=KCurrentThreadHandle);
	virtual TInt ForceRemount(TUint aFlags=0);
	virtual TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	virtual TInt Unlock(TMediaPassword &aPassword, TBool aStorePassword);
	virtual TInt Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword);
	virtual TInt Clear(TMediaPassword &aPassword);
	virtual TInt ErasePassword();
	virtual TInt DeleteNotify(TInt64 aPos, TInt aLength);
	virtual TInt GetLastErrorInfo(TDes8& aErrorInfo);
protected:
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);	
private:
	CLocalProxyDrive(CMountCB* aMount,TBusLocalDrive& aLocDrv);
private:
	TBusLocalDrive& iLocDrv;
	};




/**
@publishedPartner
@released

Media subsystem extensions must be derived from this specific class interface.
Objects of this type should be created through use of a derived CProxyDriveFactory class.

Class passes commands directly to CProxyDrive.

@see CProxyDrive
@see CProxyDriveFactory
*/
class CBaseExtProxyDrive : public CProxyDrive
	{
public:
	IMPORT_C CBaseExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
	IMPORT_C ~CBaseExtProxyDrive();
	IMPORT_C virtual TInt Initialise();
	IMPORT_C virtual TInt Dismounted();
	IMPORT_C virtual TInt Enlarge(TInt aLength);
	IMPORT_C virtual TInt ReduceSize(TInt aPos, TInt aLength);
	IMPORT_C virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	IMPORT_C virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	IMPORT_C virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	IMPORT_C virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags);
	IMPORT_C virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	IMPORT_C virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	IMPORT_C virtual TInt Caps(TDes8& anInfo);
	IMPORT_C virtual TInt Format(TFormatInfo& anInfo);
	IMPORT_C virtual TInt Format(TInt64 aPos,TInt aLength);
	IMPORT_C virtual TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle=KCurrentThreadHandle);
	IMPORT_C virtual TInt ForceRemount(TUint aFlags=0);
	IMPORT_C virtual TInt Unlock(TMediaPassword &aPassword, TBool aStorePassword);
	IMPORT_C virtual TInt Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword);
	IMPORT_C virtual TInt Clear(TMediaPassword &aPassword);
	IMPORT_C virtual TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	IMPORT_C virtual TInt ErasePassword();
	IMPORT_C virtual TInt GetLastErrorInfo(TDes8& aErrorInfo);
    IMPORT_C virtual TInt DeleteNotify(TInt64 aPos, TInt aLength);
	inline TInt LocalBufferSupport();

protected:
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);	
private:
	CProxyDrive* iProxy;
	};




/**
@publishedPartner
@released

Abstract base class for Proxy drive factory classes.

Class is used for the creation of media subsystem extensions CBaseExtProxyDrive.

@see CBaseExtProxyDrive
*/
class CProxyDriveFactory : public CFsObject
	{
public:
	IMPORT_C CProxyDriveFactory();
	IMPORT_C virtual TInt Remove();
	inline void SetLibrary(RLibrary aLib);
	inline RLibrary Library() const;

    /**
    Installation of the factory object.
    @return system wide error code
    */
	virtual TInt Install() =0;	
	/**
	Instantiates a CProxyDrive object.
	@param aProxy Proxy drive to be used.
	@param aMount Mount control block.
	
	@return pointer to Instantiated CProxyDrive object.
	*/		
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)=0;
private:
	RLibrary iLibrary;
	};



/**
@internalTechnology
*/
class CExtProxyDriveFactory : public CFsObject
	{
public:
	IMPORT_C CExtProxyDriveFactory();
	IMPORT_C virtual TInt Remove();
	inline void SetLibrary(RLibrary aLib);
	inline RLibrary Library() const;
// pure virtual
	virtual TInt Install() =0;			
	virtual TInt CreateProxyDrive(CProxyDrive*& aMountProxyDrive, CMountCB* aMount)=0;

	IMPORT_C virtual void AsyncEnumerate();

private:
	RLibrary iLibrary;
	};


/**
@internalTechnology
*/
class CExtProxyDrive : public CProxyDrive
	{
public:
	IMPORT_C CExtProxyDrive(CMountCB* aMount,CExtProxyDriveFactory* aDevice);
	IMPORT_C ~CExtProxyDrive();
	
	IMPORT_C virtual TInt NotifyChange(TDes8 &aChanged, TRequestStatus* aStatus);
	IMPORT_C virtual void NotifyChangeCancel();
	IMPORT_C virtual TInt SetInfo(const RMessage2 &msg, TAny* aMessageParam2, TAny* aMessageParam3);

	inline TInt DriveNumber();
	inline void SetDriveNumber(TInt aDrive);
	inline CExtProxyDriveFactory* FactoryP();

protected:
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
	IMPORT_C virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

	TInt SetupMediaChange();

protected:
	CExtProxyDriveFactory* iFactory;
	TInt iDriveNumber;

private:
	CExtNotifyMediaChange* iMediaChangeNotifier;

	TUint32 iReserved[4];	// Reserved bytes for future expansion
	
	friend class LocalDrives;
	};


/**
@internalTechnology
*/
NONSHARABLE_CLASS(CExtNotifyMediaChange) : public CActive
	{
public:
	static CExtNotifyMediaChange* NewL(CExtProxyDrive* aDrive);
    ~CExtNotifyMediaChange();
	void RequestL();

private:
	CExtNotifyMediaChange(CExtProxyDrive* aDrive);
	void ConstructL();

	void DoCancel();
	void RunL();

private:
	CExtProxyDrive* iDrive;
	TPtr8 iPtr;
	};

/**
@publishedPartner
@released

Gets the local bus drive.

@param aLocalDrive The local drive number.

@return The local bus drive.
*/
IMPORT_C TBusLocalDrive& GetLocalDrive(TInt aLocalDrive);

/**
@internalTechnology

Gets the proxy drive device for a given drive

@param aLocalDrive The local drive number.

@return The local bus drive.
*/
IMPORT_C CExtProxyDrive* GetProxyDrive(TInt aDrive);

/**
@internalTechnology

Gets the proxy drive for a given drive

@param aLocalDrive The local drive number.

@return The proxy drive
*/
IMPORT_C TInt GetProxyDrive(TInt aDrive, CProxyDrive*& aProxyDrive);

/**
@internalTechnology

Return ETrue if drive is actually a proxy instead of a local drive 

@param The drive number.

@return ETrue if drive is actually a proxy instead of a local drive 
*/
IMPORT_C TBool IsProxyDrive(TInt aDrive);

/**
@publishedPartner
@released

Checks a given drive number is mapped to a local drive.

@param aDrive The local drive number.

@return specified drive number is mapped to a local drive.
*/
IMPORT_C TBool IsValidLocalDriveMapping(TInt aDrive);

/** 
@internalTechnology 
 
Sets the media attributes and type in the anInfo parameter to those of the 
specified drive. 

@param anInfo TDriveInfo object to store the drive information.
@param aDriveNumber The number of the drive to get the information from.
*/
IMPORT_C void GetDriveInfo(TDriveInfo& anInfo,TInt aDriveNumber);

/**
@publishedPartner
@released

Returns the local drive number for a given drive number.

@param aDrive The drive number.

@return KDriveInvalid if drive is not mapped to a local drive.
        otherwise the local drive number.
*/
IMPORT_C TInt DriveNumberToLocalDriveNumber(TInt aDrive);

/**
*/
IMPORT_C TInt GetLocalDriveNumber(TBusLocalDrive* aLocDrv);


/**
@internalTechnology
*/
struct TFatUtilityFunctions;

/**
	Representation of FAT Utility Functions as provided by a Code Page DLL.
	These functions are to be implemented by Code Page-DLLs.
	@internaltechnology
*/

struct TCodePageFunctions
	{
	typedef TBool (*TConvertFromUnicode)(TDes8& aForeign, const TDesC16& aUnicode, const TDesC8& aReplacementForUnconvertibleCharacter);
	typedef TInt  (*TConvertFromUnicodeL)(TDes8& aForeign, const TDesC16& aUnicode, TBool leaveWhenOverflow);
	typedef TBool (*TConvertToUnicode)(TDes16& aUnicode, const TDesC8& aForeign);
	typedef TInt  (*TConvertToUnicodeL)(TDes16& aUnicode, const TDesC8& aForeign, TBool leaveWhenOverflow);
	typedef TBool (*TIsLegalShortNameCharacter)(TUint aCharacter);	

	TConvertFromUnicode iConvertFromUnicode;
	TConvertFromUnicodeL iConvertFromUnicodeL;
	TConvertToUnicode iConvertToUnicode;
	TConvertToUnicodeL iConvertToUnicodeL;
	TIsLegalShortNameCharacter iIsLegalShortNameCharacter;
	};

/**
	A utility class for Codepage Dll. Controls overflow action. Provides current status of 
	Locale/codepage dll loaded. Provides conversions functions to be used by Codepage Dlls.
	
	@internaltechnology
*/
class TCodePageUtils
	{
public:

	/** 
	Determines the Overflow action in case of if overflow occurs.
	*/
 	enum TOverflowAction
		{
		/**
		Will leave if an overflow occurs.
		*/
		EOverflowActionLeave,
		/** 
		Will truncate the data if an overflow occurs.
		*/
		EOverflowActionTruncate
		};

	/** 
	Determines the current status of Locale dll / Codepage dll.
	*/
	enum TCodepageLoaded
		{
		/** 
		No Locale Dll or Codepage Dll is loaded.
		*/
		ENone = 0,
		/** 
		Locale Dll is loaded.
		*/
		ELocaleDll,
		/** 
		Codepage Dll is loaded.
		*/
		ECodePageDll
		};
public:

	/**
	Convert from Unicode, truncating if there is not enough room in the output.

	@param aForeign The output is appended here.
	@param aUnicode The input.

	@return False if and only if aForeign has not enough space remaining. 
	*/
	TBool ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode, TOverflowAction aOverflowAction) const;

	/**
	Convert from Unicode, truncating if there is not enough room in the output.

	@param aForeign The output is appended here.
	@param aUnicode The input.

	@leave KErrOverflow if aForeign is too short for the output.
	*/
	IMPORT_C void ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TOverflowAction aOverflowAction=EOverflowActionLeave) const;

	/* 
	Convert to Unicode, truncating if there is not enough room in the output.

	@param aUnicode The output is appended here.
	@param aForeign The input.

	@return False if and only if aUnicode has not enough space remaining.
	*/
	TBool ConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign ) const;

	/* 
	Convert to Unicode, leaving if there is not enough room in the output.

	@param aUnicode The output is appended here.
	@param aForeign The input.

	@leave KErrOverflow if aUnicode is too short for the output.
	*/
	IMPORT_C void ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TOverflowAction aOverflowAction=EOverflowActionLeave) const;

	/** 
	Returns true if the input character is legal in a short name.

	@param aCharacter Character, in the foreign character encoding.

	@return true if aCharacter is legal in a FAT short name.
	*/
	IMPORT_C TBool IsLegalShortNameCharacter(TUint aCharacter,TBool aUseExtendedChars=EFalse) const;

public:

	/** 
	Constructor for TCodePageUtils.
	*/
	TCodePageUtils();

	/** 
	Returns whether a Codepage dll is loaded.
	
	@return True if Codepage dll is loaded.
			False otherwise
	*/
	TBool IsCodepageLoaded() const;

	/** 
	Returns the type of active codepage.
	@return 	ENone			if no dll is loaded
				ELocaleDll		if Locale Dll is loaded
				ECodepageDll	if Codepage Dll is loaded
	*/
	TCodepageLoaded CodepageLoaded() const;

	/** 
	Sets the current codepage to that provided by the current Locale DLL.
	
	@param		aFunctions	Pointer to FAT conversion functions to be used.
	
	@return 	None
	*/
	void SetLocaleCodePage(TFatUtilityFunctions* aFunctions);

	/** 
	Gets the function pointer to the read Locale conversions functions.
	
	@return function pointer to the read Locale conversions functions.
	*/
	TFatUtilityFunctions* LocaleFatUtilityFunctions() const;

	/** 
	Gets structure to function pointers to the read Codepage conversions functions.
	
	@return structure to function pointers to the read Codepage conversions functions.
	*/
	TCodePageFunctions CodepageFatUtilityFunctions() const;

private:

	/** 
	Structure to function pointers to the read Codepage conversions functions.
	*/
	TCodePageFunctions iCodePageFunctions;

	/** 
	Function pointer to the read Locale conversions functions.
	*/
	TFatUtilityFunctions* iLocaleFatUtilityFunctions;

	/** 
	Variable to hold the active codepage type.
	*/
	TCodepageLoaded iCodepageLoaded;

	friend class TFsLoadCodePage;
	};

/** 
@internaltechnology

Gets the pointer to the current FAT conversions functions.

@return 	Pointer to the current FAT conversions functions.
*/
IMPORT_C const TFatUtilityFunctions* GetFatUtilityFunctions();

/** 
@internaltechnology

Gets the instance of TCodePageUtils class.

@return 	Instance of TCodePageUtils class.
*/
IMPORT_C const TCodePageUtils& GetCodePage();



/**
@publishedPartner
@released

Copies data to a buffer.

If necessary, the buffer, a heap descriptor, is allocated or re-allocated
before copying takes place.

@param aBuf A reference to a pointer to heap descriptor forming the buffer.
            This will be allocated if it does not already exist,
            or re-allocated if the existing buffer is not large enough.
@param aDes The data to be copied.
*/
IMPORT_C void AllocBufferL(HBufC*& aBuf,const TDesC& aDes);





/**
@publishedPartner
@released

Notifies sessions of a debug event if aFunction has the KDebugNotifyMask set.

This function can only be used in debug builds or if _DEBUG
or _DEBUG_RELEASE is defined.

@param aFunction A function.
@param aDrive    A drive.
*/
IMPORT_C void DebugNotifySessions(TInt aFunction,TInt aDrive);




/**
@publishedPartner
@released

Writes data from a buffer to a file.

Called by the mount control block lock and the unlock functions.

@param aFileName The file to be written to.
@param aBuf      The data to be written.
*/
IMPORT_C void WriteToDisk(const TDesC& aFileName,const TDesC8& aBuf);




/**
Create a proxy drive using the local proxy drive passed in
and any extensions that have been added to the drive.

@param aConcreteDrive local proxy drive
@param aMount local proxy drive mount control block

@return pointer to instantiated CProxyDrive object.
*/
IMPORT_C CProxyDrive* CreateProxyDriveL(CProxyDrive* aConcreteDrive,CMountCB* aMount);



/**
@deprecated 6.1
*/
IMPORT_C TInt CompareFilenames(const TDesC& aFileName1,const TDesC& aFileName2);
//
/**
Lookup a file system by name.

@param aName file system name.

@return pointer to instantiated CFileSystem object.
*/
IMPORT_C CFileSystem* GetFileSystem(const TDesC& aName);



/**
@internalTechnology

A static class for retrieving F32 properties
*/
class F32Properties
	{
private:
	F32Properties();
public:
	IMPORT_C static TBool Initialise(TInt aRomAddress, TInt aLength);
	IMPORT_C static TBool GetString(const TDesC8& aSection, const TDesC8& aProperty, TDes8&  aPropVal);
	IMPORT_C static TBool GetInt(const TDesC8& aSection,    const TDesC8& aProperty, TInt32& aPropVal);
	IMPORT_C static TBool GetBool(const TDesC8& aSection,   const TDesC8& aProperty, TBool&  aPropVal);
    static void  GetDriveSection(TInt aDrvNum, TDes8& aSectionName);
 
private:
	static TBool iInitialised;
	static TInt iRomAddress;
	static TInt iRomLength;
	};

#include <f32fsys.inl>
#endif
