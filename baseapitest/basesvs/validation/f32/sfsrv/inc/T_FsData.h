/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
@test
@internalComponent

This contains CT_FsData
*/

#if (!defined __T_FS_DATA_H__)
#define __T_FS_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_DirData.h"

//	EPOC includes
#include <e32std.h>
#include <f32file.h>
#include <f32file_private.h>
#include <f32fsys.h>

//	User includes
#include "T_ActiveNotifyChange.h"

const TInt KBufferStringLength = 256;


class CT_FsData: public CDataWrapperBase
	{
public:
	static CT_FsData*	NewL();
	~CT_FsData();

	/**
	* Process a command read from the ini file
	*
	* @param	aCommand requiring command to be processed
	* @param	aSection the section in the ini file requiring the command to be processed
	* @param	aAsyncErrorIndex the index of asynchronous command error code belongs to.
	* 
	* @leave	system wide error
	*
	* @return	ETrue if the command is processed
	*/
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);

	/**
	* Return a pointer to the object that the data wraps
	*
	* @return	pointer to the object that the data wraps
	*/
	virtual TAny*	GetObject();

	/**
	* Query to see if there are any outstanding requests
	*
	* @return ETrue if there are any outstanding requests
	*/
	
	void RunL(CActive* aActive, TInt aIndex);
	void DoCancel(CActive* aActive, TInt aIndex);

protected:
	CT_FsData();
	void ConstructL();

private:

	virtual TBool	DoCommandDrivesL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TBool	DoCommandFilesL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TBool	DoCommandMountsL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);


	void	DoCmdNewL();
	void	DoCmdClose();
	void	DoCmdDestructor();

	// Mount commands
	void	DoCmdAddFileSystem(const TDesC& aSection);
	void	DoCmdMountFileSystem(const TDesC& aSection);
	void	DoCmdMountFileSystemAndScan(const TDesC& aSection);
	void	DoCmdDismountFileSystem(const TDesC& aSection);
	void	DoCmdRemoveFileSystem(const TDesC& aSection);
	void	DoCmdFileSystemName(const TDesC& aSection);
	void	DoCmdAddExtension(const TDesC& aSection);
	void	DoCmdMountExtension(const TDesC& aSection);
	void	DoCmdDismountExtension(const TDesC& aSection);
	void	DoCmdRemoveExtension(const TDesC& aSection);
	void	DoCmdExtensionName(const TDesC& aSection);
	void	DoCmdRemountDriveL(const TDesC& aSection);
	void	DoCmdNotifyDismountL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	void	DoCmdNotifyDismountCancel(const TDesC& aSection);
	void	DoCmdAllowDismount(const TDesC& aSection);
	void	DoCmdSetStartupConfigurationL(const TDesC& aSection);
	void	DoCmdAddCompositeMount(const TDesC& aSection);
	void	DoCmdStartupInitCompleteL(TInt aAsyncErrorIndex);
	void	DoCmdSetLocalDriveMappingL(const TDesC& aSection);
	void	DoCmdSwapFileSystem(const TDesC& aSection);
	void	DoCmdFinaliseDrives();
	void 	DoCmdFileSystemSubType(const TDesC& aSection);

	// Misc commands
	void	DoCmdConnect(const TDesC& aSection);
	void	DoCmdVersion(const TDesC& aSection);
	void	DoCmdNotifyChangeL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	void	DoCmdNotifyChangeCancel(const TDesC& aSection);
	void	DoCmdNotifyDiskSpaceL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	void	DoCmdNotifyDiskSpaceCancel(const TDesC& aSection);
	void	DoCmdIsRomAddress(const TDesC& aSection);
	void	DoCmdResourceCountMarkStart();
	void	DoCmdResourceCountMarkEnd();
	void	DoCmdResourceCount(const TDesC& aSection);
	void	DoCmdGetNotifyUser(const TDesC& aSection);
	void	DoCmdSetNotifyUser(const TDesC& aSection);
	void	DoCmdLoaderHeapFunctionL(const TDesC& aSection);
	void	DoCmdSetNotifyChange(const TDesC& aSection);
	void	DoCmdInitialisePropertiesFile(const TDesC& aSection);

	// Files commands
	void	DoCmdRealName(const TDesC& aSection);
	void	DoCmdSessionPath(const TDesC& aSection);
	void	DoCmdSetSessionPath(const TDesC& aSection);
	void	DoCmdParse(const TDesC& aSection);
	void	DoCmdMkDir(const TDesC& aSection);
	void	DoCmdMkDirAll(const TDesC& aSection);
	void	DoCmdRmDir(const TDesC& aSection);
	void	DoCmdGetDir(const TDesC& aSection);
	void	DoCmdDelete(const TDesC& aSection);
	void	DoCmdRename(const TDesC& aSection);
	void	DoCmdReplace(const TDesC& aSection);
	void	DoCmdAtt(const TDesC& aSection);
	void	DoCmdSetAtt(const TDesC& aSection);
	void	DoCmdModified(const TDesC& aSection);
	void	DoCmdSetModified(const TDesC& aSection);
	void	DoCmdEntryL(const TDesC& aSection);
	void	DoCmdSetEntry(const TDesC& aSection);
	void	DoCmdReadFileSectionL(const TDesC& aSection);
	void	DoCmdIsFileOpen(const TDesC& aSection);
	void	DoCmdGetShortName(const TDesC& aSection);
	void	DoCmdGetLongName(const TDesC& aSection);
	void	DoCmdIsFileInRom(const TDesC& aSection);
	void	DoCmdIsValidName(const TDesC& aSection);
	void	DoCmdSetSessionToPrivate(const TDesC& aSection);
	void	DoCmdPrivatePath(const TDesC& aSection);
	void	DoCmdCreatePrivatePath(const TDesC& aSection);

	// Drives commands
	void	DoCmdDriveList(const TDesC& aSection);
	void	DoCmdDriveL(const TDesC& aSection);
	void	DoCmdVolumeL(const TDesC& aSection);
	void	DoCmdSetVolumeLabel(const TDesC& aSection);
	void	DoCmdSubst(const TDesC& aSection);
	void	DoCmdSetSubst(const TDesC& aSection);
	void	DoCmdGetMediaSerialNumber(const TDesC& aSection);
	void	DoCmdIsValidDrive(const TDesC& aSection);
	void	DoCmdCharToDrive(const TDesC& aSection);
	void	DoCmdDriveToChar(const TDesC& aSection);
	void	DoCmdCheckDisk(const TDesC& aSection);
	void	DoCmdScanDrive(const TDesC& aSection);
	void	DoCmdGetDriveName(const TDesC& aSection);
	void	DoCmdSetDriveName(const TDesC& aSection);
	void	DoCmdLockDrive(const TDesC& aSection);
	void	DoCmdUnlockDrive(const TDesC& aSection);
	void	DoCmdClearPassword(const TDesC& aSection);
	void	DoCmdErasePassword(const TDesC& aSection);
	void	DoCmdReserveDriveSpace(const TDesC& aSection);
	void	DoCmdGetReserveAccess(const TDesC& aSection);
	void	DoCmdReleaseReserveAccess(const TDesC& aSection);
	void	DoCmdGetSystemDrive(const TDesC& aSection);
	void	DoCmdSetSystemDrive(const TDesC& aSection);
	void	DoCmdGetSystemDriveChar(const TDesC& aSection);
	void	DoCmdVolumeIOParam(const TDesC& aSection);

	// Helpers
	void	DoCleanup();
	void	DoCmdPromptL(const TDesC &aSection);
	inline RFs& CT_FsData::FileServer(){ return iFs2; }
	TBool 	VerifyTDriveInfoDataFromIniL(const TDesC& aSection, TDriveInfo& aDriveInfo);

	

	const TDesC&	ConvertToStrAttMask(TUint aAttMask);
	TBool		ConvertToSortKey(const TDesC& aSortKeyStr, TUint& aSortKey);	
	TBool		ConvertToNotifyType(const TDesC& aNotifyTypeStr, TNotifyType& aNotifyType);
	TBool		ConvertToMediaAtts(const TDesC& aMediaAttStr, TUint& aMediaAtt);
	TBool		ConvertToDriveAtts(const TDesC& aMediaAttStr, TUint& aMediaAtt);
	TBool		GetDriveNumberFromConfig(const TDesC& aSection, const TDesC& aParameterName, TDriveNumber& aDriveNumber);
	TBool		GetMediaTypeFromConfig(const TDesC& aSection, TMediaType& aMediaType);
	TBool		GetMediaAttsFromConfig(const TDesC& aSection, TUint& aMediaAtt);
	TBool		GetDriveAttsFromConfig(const TDesC& aSection, TUint& aDriveAtt);
	TBool		GetBatteryStateFromConfig(const TDesC& aSection, TBatteryState& aBatteryState);
	TBool 		GetDrvMapOperationFromConfig(const TDesC& aSection, const TDesC& aParameterName, TLocalDriveMappingInfo::TDrvMapOperation& aOperation);
public:
	TUint64 ThreadId();

private:
	/** RFs class instance that is tested */
	RFs*									iFs;

	/** RFs class instance for additional purpoces */
	RFs										iFs2;

	/** The request status for files/dir events */
	RPointerArray<CT_ActiveNotifyChange>	iNotifyChange;

	/** The request status for disk space events */
	RPointerArray<CActiveCallback>			iNotifyDiskSpace;

	/** The request status for dismounts a file system on a drive */
	RPointerArray<CActiveCallback>			iNotifyDismount;

	/** The request status for dismounts a file system on a drive */
	RPointerArray<CActiveCallback>			iStartupInitComplete;

	/** The volume label for a drive */
	TVolumeInfo								iVolumeInfo;

	/** Optional name of the volume */
	TFileName								iDriveName;
	TFileName								iVolumeLabel;

	/** Extension name */
	TFullName								iExtensionName;

	/** IsFileInRom result */
	TUint8*									iIsFileInRom;

	/**	FileSystemName retuned value */
	TFullName								iFileSystemName;

	/** LockDrive password */
	TMediaPassword							iPassword;
	};

#define GET_MANDATORY_STRING_PARAMETER(aParamName, aSection, aResult)	GetCommandStringParameter(aParamName, aSection, aResult, (TText8*)__FILE__, __LINE__, ETrue)
#define GET_MANDATORY_INT_PARAMETER(aParamName, aSection, aResult)		GetCommandIntParameter(aParamName, aSection, aResult,(TText8*) __FILE__, __LINE__, ETrue)
#define GET_MANDATORY_INT64_PARAMETER(aParamName, aSection, aResult)	GetCommandInt64Parameter(aParamName, aSection, aResult,(TText8*) __FILE__, __LINE__, ETrue)
#define GET_MANDATORY_BOOL_PARAMETER(aParamName, aSection, aResult)		GetCommandBoolParameter(aParamName, aSection, aResult, (TText8*)__FILE__, __LINE__, ETrue)

#define GET_OPTIONAL_STRING_PARAMETER(aParamName, aSection, aResult)	GetCommandStringParameter(aParamName, aSection, aResult, (TText8*) __FILE__, __LINE__, EFalse)
#define GET_OPTIONAL_INT_PARAMETER(aParamName, aSection, aResult)		GetCommandIntParameter(aParamName, aSection, aResult,(TText8*) __FILE__, __LINE__, EFalse)
#define GET_OPTIONAL_INT64_PARAMETER(aParamName, aSection, aResult)		GetCommandInt64Parameter(aParamName, aSection, aResult,(TText8*) __FILE__, __LINE__, EFalse)
#define GET_OPTIONAL_BOOL_PARAMETER(aParamName, aSection, aResult)		GetCommandBoolParameter(aParamName, aSection, aResult,(TText8*) __FILE__, __LINE__, EFalse)

#endif /* __T_FS_DATA_H__ */
