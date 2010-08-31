// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\inc\f32plugin.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/


#if !defined(__F32PLUGIN_H__)
#define __F32PLUGIN_H__

#include <f32file.h>
#include <f32file_private.h>
#include <f32fsys.h>

const TInt KPluginMessageForward  = 1; ///< Returned from ::Dispatch() to indicate that a request has been processed synchronously and should be passed down the stack.
const TInt KPluginMessageComplete = 2; ///< Returned from ::Dispatch() to indicate that a request has been processed synchronously and should be passed back up the stack.

const TInt KCountNeeded=KMinTInt;

/**
List of file server operations
*/
enum TFsMessage
	{
	EFsAddFileSystem,           ///< Adds a file system
	EFsRemoveFileSystem,        ///< Removes a file system
	EFsMountFileSystem,         ///< Mounts a file system
	EFsNotifyChange,            ///< Notifies file and/or directory change
	EFsNotifyChangeCancel,      ///< Cancels change notification
	EFsDriveList,               ///< Gets a list of the available drive
	EFsDrive,                   ///< Gets information about a drive and the medium mounted on it
	EFsVolume,                  ///< Gets volume information for a formatted device
	EFsSetVolume,               ///< Sets the label for a volume
    EFsSubst,                   ///< Gets the path assigned to a drive letter
	EFsSetSubst,				///< -- 10, Assigns a path to a drive letter
	EFsRealName,                ///< Gets the real name of a file
	EFsDefaultPath,             ///< Gets the system default path
	EFsSetDefaultPath,          ///< Sets the system default path
	EFsSessionPath,             ///< Gets the session path
	EFsSetSessionPath,          ///< Sets the session path for the current file server client
	EFsMkDir,                   ///< Makes directory
	EFsRmDir,                   ///< Removes a directory
	EFsParse,                   ///< Parses a filename specification
	EFsDelete,                  ///< Deletes file
	EFsRename,					///< -- 20 Renames a single file or directory
	EFsReplace,                 ///< Replaces a single file with another
	EFsEntry,                   ///< Gets a file's attributes
	EFsSetEntry,                ///< Sets both the attributes and the last modified date and time for a file or directory
	EFsGetDriveName,            ///<  Gets the name of a drive
	EFsSetDriveName,            ///< Sets the name of a drive
	EFsFormatSubClose,          ///< Closes the Format subsession
	EFsDirSubClose,             ///< Closes the directory.
	EFsFileSubClose,            ///< Closes the file
	EFsRawSubClose,             ///< Closes the direct access channel to the disk
	EFsFileOpen,				///< -- 30 Opens file
	EFsFileCreate,				///< Creates and opens a new file
	EFsFileReplace,             ///< Replaces a file of the same name or creates a new file
	EFsFileTemp,				///< Creates and opens a temporary file
	EFsFileRead,                ///< Reads from the file
	EFsFileWrite,               ///< Writes to the file
	EFsFileLock,                ///< Locks a region within the file
	EFsFileUnLock,              ///< Unlocks a region within the file
	EFsFileSeek,                ///< Sets the the current file position
	EFsFileFlush,               ///< Commits data to the storage device
	EFsFileSize,				///< -- 40 Gets the current file size
	EFsFileSetSize,             ///< Sets the file size
	EFsFileAtt,                 ///< Gets the file's attributes
	EFsFileSetAtt,				///< Sets or clears file attributes
	EFsFileModified,            ///< Gets local date and time the file was last modified
	EFsFileSetModified,         ///< Sets the date and time the file was last modified
	EFsFileSet,                 ///< Sets the file’s attributes, last modification date/time
	EFsFileChangeMode,          ///< Switches an open file's access mode
	EFsFileRename,              ///< Renames a file
	EFsDirOpen,                 ///< Opens a directory
	EFsDirReadOne,				///< -- 50 Reads a single directory entry
	EFsDirReadPacked,           ///< Reads all filtered directory entries
	EFsFormatOpen,              ///< Opens a device for formatting
	EFsFormatNext,              ///< Executes the next format step
	EFsRawDiskOpen,             ///< Opens a direct access channel to the disk */
	EFsRawDiskRead,             ///< Reads directly from the disk
	EFsRawDiskWrite,            ///< Writes directly to the disk
	EFsResourceCountMarkStart,  ///< Marks the start of resource count checking
	EFsResourceCountMarkEnd,    ///< Ends resource count checking
	EFsResourceCount,           ///< Gets the number of currently open resources
	EFsCheckDisk,				///< -- 60 Checks the integrity of the disk on the specified drive
	EFsGetShortName,            ///< Gets the short filename
	EFsGetLongName,             ///< Gets the long filename
	EFsIsFileOpen,              ///< Tests whether a file is open
	EFsListOpenFiles,           ///< get a list of open files */
	EFsGetNotifyUser,           ///< Tests user notification of file access failure is in effect
	EFsSetNotifyUser,           ///< Sets if the user should be notified of file access failure
	EFsIsFileInRom,             ///< Gets a pointer to the specified file, if it is in ROM
	EFsIsValidName,             ///< Tests whether a filename and path are syntactically correct
	EFsDebugFunction,           ///< Different debugging info
	EFsReadFileSection,			///< -- 70 Reads data from a file without opening it
	EFsNotifyChangeEx,          ///< Requests a notification of change to files or directories
	EFsNotifyChangeCancelEx,    ///< Cancels all outstanding requests for notification of change
	EFsDismountFileSystem,		///< Dismounts the file system from the specified drive
	EFsFileSystemName,          ///< Gets the name of the file system mounted on the specified drive
	EFsScanDrive,               ///< Checks the specified drive for specific errors and corrects them
	EFsControlIo,               ///< General purpose test interface
	EFsLockDrive,               ///< Locks a MultiMedia card in the specified drive
	EFsUnlockDrive,             ///< Unlocks the MultiMedia card in the specified drive
	EFsClearPassword,           ///< Clears the password from the locked MultiMedia card
	EFsNotifyDiskSpace,			///< -- 80 Disk space change notification
	EFsNotifyDiskSpaceCancel,   ///< Cancels a specific outstanding notification
	EFsFileDrive,               ///< Gets drive information on which this file resides
	EFsRemountDrive,            ///< Forces a remount of the specified drive
	EFsMountFileSystemScan,     ///< Mounts a file system and performs a scan on a drive
	EFsSessionToPrivate,        ///< Sets the session path to point to the private path
	EFsPrivatePath,             ///< Creates the text defining the private path
	EFsCreatePrivatePath,       ///< Creates the private path for a process
	EFsAddExtension,            ///< Adds the specified extension
	EFsMountExtension,          ///< Mounts the the specified extension
	EFsDismountExtension,		///< -- 90 Dismounts the specified extension
	EFsRemoveExtension,         ///< Removes the specified extension
	EFsExtensionName,	        ///< Gets the name of the extension on the specified drive
	EFsStartupInitComplete,		///< Noifies the file server of startup initialisation completion
	EFsSetLocalDriveMapping,    ///< Set the local drive mapping
	EFsFinaliseDrive,           ///< Finalise a specific drive
	EFsFileDuplicate,           ///< Makes a duplicate of this file handle
	EFsFileAdopt,               ///< Adopts an already open file
	EFsSwapFileSystem,          ///< Swaps file systems
	EFsErasePassword,           ///< Erase the password from the locked MultiMedia card
	EFsReserveDriveSpace,		///< -- 100 Reserves an area of a drive
	EFsGetReserveAccess,        ///< Get exclusive access to reserved area
	EFsReleaseReserveAccess,    ///< Release exclusive access to reserved area
	EFsFileName,                ///< Gets the final part of a filename
    EFsGetMediaSerialNumber,    ///<  Gets the serial number of media
	EFsFileFullName,            ///< Gets the full filename
	EFsAddPlugin,               ///< Adds the specified plugin
	EFsRemovePlugin,            ///< Removes the specified plugin
	EFsMountPlugin,			    ///< Mounts the specified plugin
	EFsDismountPlugin,	        ///< Dismounts the specified plugin
	EFsPluginName,				///<-- 110 Gets a plugin's name in specific position and drive
	EFsPluginOpen,              ///< Opens the plugin
	EFsPluginSubClose,          ///< Closes the plugin
	EFsPluginDoRequest,         ///< Issues an asynchronous plugin request
	EFsPluginDoControl,         ///< Issues a synchronous plugin request
	EFsPluginDoCancel,          ///< Cancels an synchronous plugin request
	EFsNotifyDismount,          ///< Issues a request to asynchronously dismount the file system
	EFsNotifyDismountCancel,    ///< Cancels a request to asynchronously dismount the file system
	EFsAllowDismount,           ///< Notifies that it is safe to dismount the file system
    EFsSetStartupConfiguration, ///< Configures file server at startup
    EFsFileReadCancel,          ///< -- 120 Cancels an outstanding asynchronous read request
    EFsAddCompositeMount,       ///< Add a mount to the composite file system
    EFsSetSessionFlags,         ///< Set/Clear session specific flags
    EFsSetSystemDrive,			///< Set SystemDrive
	EFsBlockMap,				///< Fetches the BlockMap of a file
    EFsUnclamp,					///< Re-enable modification of a specified file in storage media
    EFsFileClamp,				///< Disable modification of a specified file in storage media
	EFsQueryVolumeInfoExt,		///< Query interface to retrieve extended volume information
	EFsInitialisePropertiesFile,///< Read in the F32 properties file provided by ESTART
	EFsFileWriteDirty,			///< Writes dirty data to disk. Used when write caching enabled
	EFsSynchroniseDriveThread,	///< -- 130 Synchronises the asynchronous operation which executes in driver thread
	EFsAddProxyDrive,			///< Loads  a proxy drive
	EFsRemoveProxyDrive,		///< Unloads a proxy drive
	EFsMountProxyDrive,			///< Mounts a proxy drive
	EFsDismountProxyDrive,		///< Dismounts a proxy drive
	EFsNotificationOpen,		///< Opens the notification
	EFsNotificationBuffer,		///< Communicates buffer to file server
	EFsNotificationRequest,		///< Sends the notification request
	EFsNotificationCancel,		///< Cancels the notification request
	EFsNotificationSubClose,	///< Closes the notification
	EFsNotificationAdd,			///< -- 140 Adds filter to the server, comprising a path and notification type
	EFsNotificationRemove,		///< Removes filters from Server-Side
	EFsLoadCodePage,			///< Loads a code page library
	EMaxClientOperations		///< This must always be the last operation insert above
	};

class CFsRequest;

/**
Request wrapper for plugins
*/
class TFsPluginRequest
	{
public:
	
	/**
	@publishedPartner
	@released
	
	Used for getting parameters from
	a CFsRequest by a F32 plugin.
	*/
	enum TF32ArgType
		{
		EPosition,
		ELength,
		EData,
		ESize,
		EName,
		ENewName,
		EEntry,
		ETime,
		ESetAtt,
		EClearAtt,
		EMode,
		EAtt,
		EAttMask,
		EUid,
		EEntryArray,
		ENewPosition,
		EVolumeInfo
		};
	
	IMPORT_C TFsPluginRequest(CFsRequest* aRequest);
	
	/**
	@publishedPartner
	@released
	
	Get the current Function number, as defined in TFsMessage
	*/
	IMPORT_C TInt Function() const;
	
	/**
	@publishedPartner
	@released
	
	Returns the drive number of the current request
	*/
	IMPORT_C TInt DriveNumber() const;
	IMPORT_C TParse& Src() const;
	IMPORT_C TParse& Dest() const;
	IMPORT_C TDrive* Drive() const;
	IMPORT_C TDrive* SubstedDrive() const;
	IMPORT_C const RMessage2& Message() const;
	IMPORT_C CFsRequest* Request() const;
	IMPORT_C TUint ScratchValue() const;
	IMPORT_C TInt64 ScratchValue64() const;
	IMPORT_C TBool IsPostOperation() const;
	
	/**
	@publishedPartner
	@released
	
	Utility function for returning a TInt type of TF32ArgType value from the current request.
	i.e. ELength.
	
	To obtain EPosition, authors should use ::Read(TF32ArgType,TInt64&)
	*/
	IMPORT_C TInt Read(TF32ArgType aType, TInt& aVal);
	
	/**
	@publishedPartner
	@released
	
	Utility function for returning a TUint type of TF32ArgType value from the current request.
	
	To obtain EPosition, authors should use ::Read(TF32ArgType,TInt64&)
	*/
	IMPORT_C TInt Read(TF32ArgType aType, TUint& aVal);
	
	/**
	@publishedPartner
	@released
	
	Utility function for returning a TInt64 type of TF32ArgType value from the current request.
	i.e. EPosition.
	*/
	IMPORT_C TInt Read(TF32ArgType aType, TInt64& aVal);

	/**
	@publishedPartner
	@released
	
	Utility function for receving descriptor or packaged paramters of the current request.
	For non-descriptor types such a TEntry, plugin authors should pass an object of type TPckg<TEntry> in to aDes.
	*/
	IMPORT_C TInt Read(TF32ArgType aType, TDes8& aDes,  TInt aOffset = 0);
	
	/**
	@publishedPartner
	@released
	
	Utility function for receving descriptor or packaged paramters of the current request.
	For non-descriptor types such a TEntry, plugin authors should pass an object of type TPckg<TEntry> in to aDes.
	*/
	IMPORT_C TInt Read(TF32ArgType aType, TDes16& aDes, TInt aOffset = 0);

	/**
	@publishedPartner
	@released
	
	Utilty function for writing data back to the request.
	Non-descriptor values should be packaged i.e TPckg<TUidType>
	*/
	IMPORT_C TInt Write(TF32ArgType aType, const TDesC8& aDes,  TInt aOffset = 0);
	
	/**
	@publishedPartner
	@released
	
	Utilty function for writing data back to the request.
	Non-descriptor values should be packaged i.e TPckg<TUidType>
	*/
	IMPORT_C TInt Write(TF32ArgType aType, const TDesC16& aDes, TInt aOffset = 0);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C TInt FileName(TDes& aName);
	
	/**
	@publishedPartner
	@released
	
	When plugin authors perform early completion of read or write requests, then SetSharePos should be called in order to
	update the share position to the client.
	*/
	IMPORT_C TInt SetSharePos(TInt64& aPos);

private:
	TInt ClientSubSessionHandle(TInt& aHandle);
	TInt ShareFromClientHandle(CFileShare*& aShare);

	void NameFromShare(CFileShare& aFileShare, TDes& aName);

private:
	CFsRequest* iFsRequest;

	friend class RFilePlugin;
	};

/**
@publishedPartner
@released

Helper class for plugins to use to access TEntryArray data
*/
class TRawEntryArray : public TEntryArray
	{
public:
	IMPORT_C TDes8& Buf();
	IMPORT_C void SetBuf(TDes8& aBuf);
	IMPORT_C TInt EntrySize(TInt aIdx);
	};

class CFsPlugin;
class CPluginThread;
class CFsPluginConn;

/**
Plugin factory class. It is created when RFs::AddPlugin is called.
*/
class CFsPluginFactory : public CFsObject
	{
public:
	IMPORT_C CFsPluginFactory();
	IMPORT_C ~CFsPluginFactory();
	IMPORT_C virtual TInt Remove();
	IMPORT_C void SetLibrary(RLibrary aLib);
	IMPORT_C RLibrary Library() const;
public:
	/**
	@internalTechnology
	Installs the plugin factory
	@return KErrNone or one of the system wide errors
	*/
	virtual TInt Install()=0;
	/**
	@internalTechnology
	Creates a new plugin
	@return plugin object
	*/
	virtual CFsPlugin* NewPluginL()=0;
	/**
	@internalTechnology
	Returns unique position of the plugin
	@return unique position of the plugin
	*/
	virtual TInt UniquePosition()=0;
public:
	TBool IsDriveSupported(TInt aDrive);
	inline TInt MountedPlugins();
	inline TInt SupportedDrives(); //aSupportedDrives is a bit mask
private:
	inline void IncrementMounted();
	inline void DecrementMounted();
protected:
	TInt iSupportedDrives;
	inline void SetSupportedDrives(TInt aSupportedDrives); //aSupportedDrives is a bit mask
private:
	TInt iMountedPlugins;
	TInt iUniquePos;
	RLibrary iLibrary;
	friend class FsPluginManager;
	};

/**
    A base class for File Server Plugins
*/
class CFsPlugin : public CFsObject
	{
protected:
	
    /** Intercept attribute to specify the order of request handling */
	enum TInterceptAtts
		{
		EPreIntercept     = 0x01, ///< handles the request before the next plugin in chain
		EPostIntercept    = 0x02, ///< handles the request after the next plugin in chain
		EPrePostIntercept = EPreIntercept | EPostIntercept ///< covers both pre and post intercept
		};
public:
	IMPORT_C CFsPlugin();
	IMPORT_C ~CFsPlugin();
	inline TInt Drive();
	inline void SetDrive(TInt aDrive);
	inline virtual TInt SessionDisconnect(CSessionFs* aSession);
protected:
	IMPORT_C virtual void InitialiseL();
	IMPORT_C virtual TInt Deliver(TFsPluginRequest& aRequest);
	virtual TInt DoRequestL(TFsPluginRequest& aRequest) = 0;
	
	IMPORT_C virtual CFsPluginConn* NewPluginConnL();

	IMPORT_C TInt RegisterIntercept(TInt aMessage, TInterceptAtts aInterceptAtts);
	IMPORT_C TInt UnregisterIntercept(TInt aMessage, TInterceptAtts aInterceptAtts);

	/** @prototype */
	IMPORT_C TInt FileRead(TFsPluginRequest& aRequest, TDes8& aDes, TInt64 aPos);
	IMPORT_C TInt FileWrite(TFsPluginRequest& aRequest, const TDesC8& aDes, TInt64 aPos);
	IMPORT_C static TInt ClientRead(TFsPluginRequest& aRequest, TDes8& aDes,TInt aOffset=0);
	IMPORT_C static TInt ClientWrite(TFsPluginRequest& aRequest, const TDesC8& aDes,TInt aOffset=0);

	//Overloaded function - checks all types of TInterceptAtts
	TBool IsRegistered(TInt aMessage);
	TBool IsRegistered(TInt aMessage, TInterceptAtts aInterceptAtts);
	TBool OriginatedFromPlugin(CFsRequest& aMessage);
	TBool IsMounted(TInt aDrive);

private:
	TInt DispatchOperation(TFsPluginRequest& aRequest, TDes8& aDes, TInt64 aPos, TInt aFunction);
	static TInt Complete(CFsRequest* aRequest, TInt aError);
	static TInt Complete(CFsRequest* aRequest);
	TInt WaitForRequest();

protected:
	TThreadId iThreadId;
private:
	CPluginThread* iThreadP;
	TInt iDrive;

	/**
	The remaining space in this base class in release 9.1 is defined as follows:
		TUint8 iRegisteredIntercepts[EMaxClientOperations << 1];	244 bytes
		TInt iUniquePos;											  4 bytes
															TOTAL	248 bytes
	where EMaxClientOperations = 122.

	Unfortunately, the remaining space in release 9.2+ WAS defined as follows:
	    enum {KIntcArrSize = 123*2};
	    TUint8  iRegisteredIntercepts[KIntcArrSize];				246 bytes
																	  2 bytes (padding)
		TInt iUniquePos;											  4 bytes
															TOTAL:	252 bytes

	This meant that a 9.1-compiled plugin running on 9.2+ would have its first data
	member overwritten when the base class (CFsPlugin) wrote to iUniquePos.

	To maintain Binary Compatibility (BC), we need to preserve both the (smaller) 9.1
	and (larger) 9.2+ class sizes.
	To allow 9.1 plugins to work unchanged on 9.2+ iUniquePos has been moved to BEFORE
	the iRegisteredIntercepts byte array

	N.B. - the iRegisteredIntercepts array uses only 2 bits per function, so the
	array size only needs to be >= EMaxClientOperations/4.
	*/
    enum {KIntcArrSize = 132};
	TInt iUniquePos;									//			  4 bytes
    TUint8 iRegisteredIntercepts[KIntcArrSize];			//			132 bytes
	TInt iLastError;									//            4 bytes
	TInt iMountedOn;	//bitmask						//			  4 bytes
    TUint iSpare[26];									//			104 bytes
	// extra 4 bytes to preserve BC with 9.1 plugins. Don't move !
	const TUint iReadOnly;								//			  4 bytes
														// TOTAL:	252
	friend class FsPluginManager;
	friend class CFsRequest;
	friend class CFsMessageRequest;
	friend class CFsInternalRequest;
	friend class CPluginThread;
	friend class TFsDismountPlugin;
	friend class TPluginSessionHelper;
	friend class TFsDirOpen; //For access to TInterceptAtts
	};

class TPluginSessionHelper
	{
public:
	TPluginSessionHelper();
	TPluginSessionHelper(TFsPluginRequest* aRequest, TBool aDirectToDrive);
	TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs, TInt* aReply);
	TInt SendReceive(TInt aFunction, const TIpcArgs& aArgs) const;
	TInt SendReceive(TInt aFunction, const TIpcArgs& aArgs, TInt aSubSessionHandle) const;
	inline TFsPluginRequest* Request();
private:
	TInt Dispatch(TInt aFunction, TIpcArgs& aArgs) const;
private:
    CFsPlugin* iPlugin;     // owner; used for setting iCurrentPlugin in CFsMessageRequest
	CSessionFs* iSession;
	RLocalMessage iMessage;
    TBool iDirectToDrive;
	TFsPluginRequest* iRequest;
	TUint iSpare[4];
	};

/**
A class for making file server request internally from within a
file server plugin.

See also RFilePlugin and RDirPlugin.

@publishedPartner
@released
*/
class RFsPlugin : private RFs
   {
public:
	IMPORT_C  RFsPlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive = EFalse);
	IMPORT_C ~RFsPlugin();

	IMPORT_C TInt Connect();
	IMPORT_C void Close();

	IMPORT_C TInt Delete(const TDesC& aName);
	IMPORT_C TInt Rename(const TDesC& anOldName,const TDesC& aNewName);
	IMPORT_C TInt Replace(const TDesC& anOldName,const TDesC& aNewName);
	IMPORT_C TInt Entry(const TDesC& aName,TEntry& anEntry) const;
	IMPORT_C TInt SetEntry(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	IMPORT_C TInt ReadFileSection(const TDesC& aName,TInt64 aPos,TDes8& aDes,TInt aLength) const;
	IMPORT_C TInt Volume(TVolumeInfo &aVol,TInt aDrive=KDefaultDrive) const;

protected:
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;

private:
	RFsPlugin();
	RFsPlugin(const RFsPlugin&);
	RFsPlugin& operator=(const RFsPlugin&);
	void SetHandle(TInt aHandle);

private:
	TPluginSessionHelper iSessionHelper;
	friend class RFs;
    };

/**
@publishedPartner
@released
*/
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
class RFilePlugin : private RFile
#else
class RFilePlugin : private RFile64
#endif
	{
public:
	IMPORT_C  RFilePlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive = EFalse);
	IMPORT_C ~RFilePlugin();

    // open a NEW file using same session as passed request
	IMPORT_C TInt Open(const TDesC& aName,TUint aMode);
	IMPORT_C TInt Create(const TDesC& aName,TUint aFileMode);
    IMPORT_C TInt Replace(const TDesC& aName,TUint aFileMode);
    IMPORT_C TInt Temp(const TDesC& aPath,TFileName& aName,TUint aFileMode);

    // re-open SAME file as client's request
    IMPORT_C TInt AdoptFromClient();

    // Transfer the plugin's open file to the client
	IMPORT_C TInt TransferToClient();

	IMPORT_C void Close();

    // RFile overloads
    IMPORT_C TInt Read(TInt64 aPos,TDes8& aDes) const;
	IMPORT_C TInt Read(TInt64 aPos,TDes8& aDes,TInt aLength) const;
    IMPORT_C TInt Write(TInt64 aPos,const TDesC8& aDes);
    IMPORT_C TInt Write(TInt64 aPos,const TDesC8& aDes,TInt aLength);
	IMPORT_C TInt Lock(TInt64 aPos,TInt64 aLength) const;
	IMPORT_C TInt UnLock(TInt64 aPos,TInt64 aLength) const;
	IMPORT_C TInt Seek(TSeek aMode,TInt64& aPos) const;
	IMPORT_C TInt Flush();
	IMPORT_C TInt Size(TInt64& aSize) const;
	IMPORT_C TInt SetSize(TInt64 aSize);
	IMPORT_C TInt Att(TUint& aAttValue) const;
	IMPORT_C TInt SetAtt(TUint aSetAttMask,TUint aClearAttMask);
	IMPORT_C TInt Modified(TTime& aTime) const;
	IMPORT_C TInt SetModified(const TTime& aTime);
	IMPORT_C TInt Set(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	IMPORT_C TInt ChangeMode(TFileMode aNewMode);
	IMPORT_C TInt Rename(const TDesC& aNewName);

protected:
	// RSubSessionBase overrides
	TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs);
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;
	void CloseSubSession(TInt aFunction);

private:
	RFilePlugin();
	RFilePlugin(const RFilePlugin&);
	RFilePlugin& operator=(const RFilePlugin&);

	void SetHandle(TInt aHandle);
	void SetSubSessionHandle(TInt aHandle);

private:
	TPluginSessionHelper iSessionHelper;
	friend class RFile;
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	friend class RFile64;
#endif
	};

/**
@publishedPartner
@released
*/
class RDirPlugin : private RDir
   {
public:
   	IMPORT_C  RDirPlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive = EFalse);
   	IMPORT_C ~RDirPlugin();

	IMPORT_C TInt Open(const TDesC& aMatchName,const TUidType& aUidType);
	IMPORT_C TInt Open(const TDesC& aMatchName,TUint anAttMask);
	IMPORT_C void Close();

	/**
	Plugin authors should pass in a TRawEntryArray if their intent is to modify the data.
	*/
	IMPORT_C TInt Read(TEntryArray& aArray);
	IMPORT_C TInt Read(TEntry& aEntry);

protected:
	// RSubSessionBase overrides
	TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs);
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;
	void CloseSubSession(TInt aFunction);

private:
	RDirPlugin();
	RDirPlugin(const RDirPlugin&);
	RDirPlugin& operator=(const RDirPlugin&);

	void SetHandle(TInt aHandle);
	void SetSubSessionHandle(TInt aHandle);

private:
	TPluginSessionHelper iSessionHelper;
	friend class RDir;
	};


class CFsRequest;
class CFsPluginConnRequest : public CBase
	{
public:
	CFsPluginConnRequest(CFsPluginConn* aPluginConn);
public:
	inline TInt Function() const;
	inline TDes8* Param1() const;
	inline TDes8* Param2() const;
	inline void WriteParam1L(const TDesC8& aDes) const;
	inline void WriteParam2L(const TDesC8& aDes) const;
	inline void ReadParam1L(TDes8& aDes) const;
	inline void ReadParam2L(TDes8& aDes) const;
	inline const RMessagePtr2& Message() const;
public:
	TInt DoControl();
	void DoRequest();
	inline void Complete(TInt aError);
public:
	TInt InitControl(CFsRequest* aRequest);
	TInt InitRequest(CFsRequest* aRequest);
private:
	TDblQueLink iLink;
	CFsPluginConn& iPluginConn;
	TInt iFunction;
	TDes8* iParam1;
	TDes8* iParam2;
public:
	RMessagePtr2 iMessage;

	friend class TPluginConnRequestQue;
	friend class CFsPluginConn;
	};

class TPluginConnRequestQue
	{
public:
	 TPluginConnRequestQue();
	~TPluginConnRequestQue();
	void DoAddRequest(CFsPluginConnRequest* aRequest);
	IMPORT_C void DoCancelAll(TInt aCompletionCode);
protected:
	TDblQue<CFsPluginConnRequest> iHeader;
	};

/**
Plugin connection object
*/
class CFsPluginConn : public CFsObject
	{
public:
	IMPORT_C CFsPluginConn();
	IMPORT_C ~CFsPluginConn();

	virtual TInt DoControl(CFsPluginConnRequest& aRequest) = 0;
	virtual void DoRequest(CFsPluginConnRequest& aRequest) = 0;
	virtual void DoCancel(TInt aReqMask) = 0;

	inline CFsPlugin* Plugin() const;
	inline TThreadId ClientId() const;
private:
	IMPORT_C void Close();
	CFsPlugin* iPluginP;
	TThreadId iClientId;
public:
	TPluginConnRequestQue iRequestQue;
	friend class FsPluginManager;
	};

#include <f32plugin.inl>

#endif // __F32PLUGIN_H

