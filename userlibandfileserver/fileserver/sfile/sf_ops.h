// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_ops.h
// 
//

#ifndef SF_OPS_H
#define SF_OPS_H


#define MSG0(aType) ((TUint32)(TFsPluginRequest::aType))
#define MSG1(aType) ((TUint32)(TFsPluginRequest::aType) << 8)  
#define MSG2(aType) ((TUint32)(TFsPluginRequest::aType) << 16)  
#define MSG3(aType) ((TUint32)(TFsPluginRequest::aType) << 24)  

static const TOperation OperationArray[EMaxClientOperations]=
	{/*		function					Flags					iInitialise									iPostInitialise						iDoRequestL									iMessageArguments*/
		{	EFsAddFileSystem,			ESync,					&TFsAddFileSystem::Initialise,				NULL,								&TFsAddFileSystem::DoRequestL				},
		{	EFsRemoveFileSystem,		ESync,					&TFsRemoveFileSystem::Initialise,			NULL,								&TFsRemoveFileSystem::DoRequestL			},
		{	EFsMountFileSystem,			0,						&TFsMountFileSystem::Initialise,			NULL,								&TFsMountFileSystem::DoRequestL				},
		{	EFsNotifyChange,			ESync,					&TFsNotifyChange::Initialise,				NULL,								&TFsNotifyChange::DoRequestL				},
		{	EFsNotifyChangeCancel,		ESync,					&TFsNotifyChangeCancel::Initialise,			NULL,								&TFsNotifyChangeCancel::DoRequestL			},
		{	EFsDriveList,				ESync,					&TFsDriveList::Initialise,					NULL,								&TFsDriveList::DoRequestL					},
		{	EFsDrive,					ESync,					&TFsDrive::Initialise,						NULL,								&TFsDrive::DoRequestL						},
		{	EFsVolume,					0,						&TFsVolume::Initialise,						NULL,								&TFsVolume::DoRequestL						, MSG0(EVolumeInfo)},
		{	EFsSetVolume,				0,						&TFsSetVolume::Initialise,					NULL,								&TFsSetVolume::DoRequestL					},
		{	EFsSubst,					ESync,					&TFsSubst::Initialise,						NULL,								&TFsSubst::DoRequestL						},
		{	EFsSetSubst,				ESync | EParseSrc,		&TFsSetSubst::Initialise,					NULL,								&TFsSetSubst::DoRequestL					},
		{	EFsRealName,				ESync | EParseSrc,		&TFsRealName::Initialise,					NULL,								&TFsRealName::DoRequestL					},
		{	EFsDefaultPath,				ESync,					&TFsDefaultPath::Initialise,				NULL,								&TFsDefaultPath::DoRequestL					},
		{	EFsSetDefaultPath,			ESync,					&TFsSetDefaultPath::Initialise,				NULL,								&TFsSetDefaultPath::DoRequestL				},
		{	EFsSessionPath,				ESync,					&TFsSessionPath::Initialise,				NULL,								&TFsSessionPath::DoRequestL					},
		{	EFsSetSessionPath,			ESync,					&TFsSetSessionPath::Initialise,				NULL,								&TFsSetSessionPath::DoRequestL				},
		{	EFsMkDir,					EParseSrc,				&TFsMkDir::Initialise,						NULL,								&TFsMkDir::DoRequestL						, MSG0(EName) | MSG1(EMode)},
		{	EFsRmDir,					EParseSrc,				&TFsRmDir::Initialise,						NULL,								&TFsRmDir::DoRequestL						, MSG0(EName)},
		{	EFsParse,					ESync,					&TFsParse::Initialise,						NULL,								&TFsParse::DoRequestL						},
		{	EFsDelete,					EParseSrc,				&TFsDelete::Initialise,						NULL,								&TFsDelete::DoRequestL						, MSG0(EName)},
		{	EFsRename,					EParseDst | EParseSrc,	&TFsRename::Initialise,						NULL,								&TFsRename::DoRequestL						, MSG0(EName) | MSG1(ENewName)},
		{	EFsReplace,					EParseDst | EParseSrc,	&TFsReplace::Initialise,					NULL,								&TFsReplace::DoRequestL						, MSG0(EName) | MSG1(ENewName)},
		{	EFsEntry,					EParseSrc,				&TFsEntry::Initialise,						NULL,								&TFsEntry::DoRequestL						, MSG0(EName) | MSG1(EEntry)},
		{	EFsSetEntry,				EParseSrc,				&TFsSetEntry::Initialise,					NULL,								&TFsSetEntry::DoRequestL					, MSG0(EName) | MSG1(ETime) | MSG2(ESetAtt) | MSG3(EClearAtt)},
		{	EFsGetDriveName,			ESync,					&TFsGetDriveName::Initialise,				NULL,								&TFsGetDriveName::DoRequestL				},
		{	EFsSetDriveName,			ESync,					&TFsSetDriveName::Initialise,				NULL,								&TFsSetDriveName::DoRequestL				},
		{	EFsFormatSubClose,			ESync,					&TFsSubClose::Initialise,					NULL,								&TFsSubClose::DoRequestL					},
		{	EFsDirSubClose,				ESync,					&TFsSubClose::Initialise,					NULL,								&TFsSubClose::DoRequestL					},
		{	EFsFileSubClose,			EFileShare,				&TFsSubClose::Initialise,					NULL,								&TFsSubClose::DoRequestL					},
		{	EFsRawSubClose,				ESync,					&TFsSubClose::Initialise,					NULL,								&TFsSubClose::DoRequestL					},
		{	EFsFileOpen,				EParseSrc,				&TFsFileOpen::Initialise,					NULL,								&TFsFileOpen::DoRequestL					, MSG0(EName)     | MSG1(EMode)},
		{	EFsFileCreate,				EParseSrc,				&TFsFileCreate::Initialise,					NULL,								&TFsFileCreate::DoRequestL					, MSG0(EName)     | MSG1(EMode)},
		{	EFsFileReplace,				EParseSrc,				&TFsFileReplace::Initialise,				NULL,								&TFsFileReplace::DoRequestL					, MSG0(EName)     | MSG1(EMode)},
		{	EFsFileTemp,				EParseSrc,				&TFsFileTemp::Initialise,					NULL,								&TFsFileTemp::DoRequestL					, MSG0(EName)     | MSG1(EMode)   | MSG2(ENewName)},
		{	EFsFileRead,				EParseSrc | EFileShare,	&TFsFileRead::Initialise,					&TFsFileRead::PostInitialise,		&TFsFileRead::DoRequestL					, MSG0(EData)     | MSG1(ELength) | MSG2(EPosition) },
		{	EFsFileWrite,				EParseSrc | EFileShare,	&TFsFileWrite::Initialise,					&TFsFileWrite::PostInitialise,		&TFsFileWrite::DoRequestL					, MSG0(EData)     | MSG1(ELength) | MSG2(EPosition) },
		{	EFsFileLock,				EParseSrc | EFileShare,	&TFsFileLock::Initialise,					NULL,								&TFsFileLock::DoRequestL					, MSG0(EPosition) | MSG1(ELength)},
		{	EFsFileUnLock,				EParseSrc | EFileShare,	&TFsFileUnlock::Initialise,					NULL,								&TFsFileUnlock::DoRequestL					, MSG0(EPosition) | MSG1(ELength)},
		{	EFsFileSeek,				EParseSrc | EFileShare,	&TFsFileSeek::Initialise,					NULL,								&TFsFileSeek::DoRequestL					, MSG0(EPosition) | MSG1(EMode)   | MSG2(ENewPosition)},
		{	EFsFileFlush,				EParseSrc | EFileShare,	&TFsFileFlush::Initialise,					NULL,								&TFsFileFlush::DoRequestL					},
		{	EFsFileSize,				EParseSrc | EFileShare,	&TFsFileSize::Initialise,					NULL,								&TFsFileSize::DoRequestL					, MSG0(ESize)},
		{	EFsFileSetSize,				EParseSrc | EFileShare,	&TFsFileSetSize::Initialise,				NULL,								&TFsFileSetSize::DoRequestL					, MSG0(ESize)},
		{	EFsFileAtt,					EParseSrc | EFileShare,	&TFsFileAtt::Initialise,					NULL,								&TFsFileAtt::DoRequestL						, MSG0(EAtt)},
		{	EFsFileSetAtt,				EParseSrc | EFileShare,	&TFsFileSetAtt::Initialise,					NULL,								&TFsFileSetAtt::DoRequestL					, MSG0(ESetAtt) | MSG1(EClearAtt)},
		{	EFsFileModified,			EParseSrc | EFileShare,	&TFsFileModified::Initialise,				NULL,								&TFsFileModified::DoRequestL				, MSG0(ETime)},
		{	EFsFileSetModified,			EParseSrc | EFileShare,	&TFsFileSetModified::Initialise,			NULL,								&TFsFileSetModified::DoRequestL				, MSG0(ETime)},
		{	EFsFileSet,					EParseSrc | EFileShare,	&TFsFileSet::Initialise,					NULL,								&TFsFileSet::DoRequestL						, MSG0(ETime) | MSG1(ESetAtt) | MSG2(EClearAtt)},
		{	EFsFileChangeMode,			EParseSrc | EFileShare,	&TFsFileChangeMode::Initialise,				NULL,								&TFsFileChangeMode::DoRequestL				, MSG0(EMode)},
		{	EFsFileRename,				EParseDst | EParseSrc,	&TFsFileRename::Initialise,					NULL,								&TFsFileRename::DoRequestL					, MSG0(ENewName)},
		{	EFsDirOpen,					EParseSrc,				&TFsDirOpen::Initialise,					NULL,								&TFsDirOpen::DoRequestL						, MSG0(EName) | MSG1(EAttMask) | MSG2(EUid)},
		{	EFsDirReadOne,				0,						&TFsDirReadOne::Initialise,					NULL,								&TFsDirReadOne::DoRequestL					, MSG0(EEntry)},
		{	EFsDirReadPacked,			0,						&TFsDirReadPacked::Initialise,				NULL,								&TFsDirReadPacked::DoRequestL				, MSG0(EEntryArray)},
		{	EFsFormatOpen,				EParseSrc,				&TFsFormatOpen::Initialise,					NULL,								&TFsFormatOpen::DoRequestL					},
		{	EFsFormatNext,				EParseSrc,				&TFsFormatNext::Initialise,					NULL,								&TFsFormatNext::DoRequestL					},
		{	EFsRawDiskOpen,				0,						&TFsRawDiskOpen::Initialise,				NULL,								&TFsRawDiskOpen::DoRequestL					},
		{	EFsRawDiskRead,				EParseSrc,				&TFsRawDiskRead::Initialise,				NULL,								&TFsRawDiskRead::DoRequestL					},
		{	EFsRawDiskWrite,			EParseSrc,				&TFsRawDiskWrite::Initialise,				NULL,								&TFsRawDiskWrite::DoRequestL				},
		{	EFsResourceCountMarkStart,	ESync,					&TFsResourceCountMarkStart::Initialise,		NULL,								&TFsResourceCountMarkStart::DoRequestL		},
		{	EFsResourceCountMarkEnd,	ESync,					&TFsResourceCountMarkEnd::Initialise,		NULL,								&TFsResourceCountMarkEnd::DoRequestL		},
		{	EFsResourceCount,			ESync,					&TFsResourceCount::Initialise,				NULL,								&TFsResourceCount::DoRequestL				},
		{	EFsCheckDisk,				EParseSrc,				&TFsCheckDisk::Initialise,					NULL,								&TFsCheckDisk::DoRequestL					},
		{	EFsGetShortName,			EParseSrc,				&TFsGetShortName::Initialise,				NULL,								&TFsGetShortName::DoRequestL				},
		{	EFsGetLongName,				EParseSrc,				&TFsGetLongName::Initialise,				NULL,								&TFsGetLongName::DoRequestL					},
		{	EFsIsFileOpen,				EParseSrc,				&TFsIsFileOpen::Initialise,					NULL,								&TFsIsFileOpen::DoRequestL					},
		{	EFsListOpenFiles,			ESync,					&TFsListOpenFiles::Initialise,				NULL,								&TFsListOpenFiles::DoRequestL				},
		{	EFsGetNotifyUser,			ESync,					&TFsGetNotifyUser::Initialise,				NULL,								&TFsGetNotifyUser::DoRequestL				},
		{	EFsSetNotifyUser,			ESync,					&TFsSetNotifyUser::Initialise,				NULL,								&TFsSetNotifyUser::DoRequestL				},
		{	EFsIsFileInRom,				EParseSrc,				&TFsIsFileInRom::Initialise,				NULL,								&TFsIsFileInRom::DoRequestL					},
		{	EFsIsValidName,				ESync,					&TFsIsValidName::Initialise,				NULL,								&TFsIsValidName::DoRequestL					},
		{	EFsDebugFunction,			ESync,					&TFsDebugFunc::Initialise,					NULL,								&TFsDebugFunc::DoRequestL					},
		{	EFsReadFileSection,			EParseSrc,				&TFsReadFileSection::Initialise,			NULL,								&TFsReadFileSection::DoRequestL				, MSG0(EData) | MSG1(EName) | MSG2(EPosition) | MSG3(ELength)},
		{	EFsNotifyChangeEx,			ESync | EParseSrc,		&TFsNotifyChangeEx::Initialise,				NULL,								&TFsNotifyChangeEx::DoRequestL				},
		{	EFsNotifyChangeCancelEx,	ESync,					&TFsNotifyChangeCancelEx::Initialise,		NULL,								&TFsNotifyChangeCancelEx::DoRequestL		},
		{	EFsDismountFileSystem,		0,						&TFsDismountFileSystem::Initialise,			NULL,								&TFsDismountFileSystem::DoRequestL			},
		{	EFsFileSystemName,			ESync,					&TFsFileSystemName::Initialise,				NULL,								&TFsFileSystemName::DoRequestL				},
		{	EFsScanDrive,				EParseSrc,				&TFsScanDrive::Initialise,					NULL,								&TFsScanDrive::DoRequestL					},
		{	EFsControlIo,				0,						&TFsControlIo::Initialise,					NULL,								&TFsControlIo::DoRequestL					},
		{	EFsLockDrive,				0,						&TFsLockDrive::Initialise,					NULL,								&TFsLockDrive::DoRequestL					},
		{	EFsUnlockDrive,				0,						&TFsUnlockDrive::Initialise,				NULL,								&TFsUnlockDrive::DoRequestL					},
		{	EFsClearPassword,			0,						&TFsClearPassword::Initialise,				NULL,								&TFsClearPassword::DoRequestL				},
		{	EFsNotifyDiskSpace,			0,						&TFsNotifyDiskSpace::Initialise,			NULL,								&TFsNotifyDiskSpace::DoRequestL				},
		{	EFsNotifyDiskSpaceCancel,	ESync,					&TFsNotifyDiskSpaceCancel::Initialise,		NULL,								&TFsNotifyDiskSpaceCancel::DoRequestL		},
		{	EFsFileDrive,				EParseSrc | EFileShare,	&TFsFileDrive::Initialise,					NULL,								&TFsFileDrive::DoRequestL					},
		{	EFsRemountDrive,			0,						&TFsRemountDrive::Initialise,				NULL,								&TFsRemountDrive::DoRequestL				},
		{	EFsMountFileSystemScan,		0,						&TFsMountFileSystemScan::Initialise,		NULL,								&TFsMountFileSystemScan::DoRequestL			},
		{	EFsSessionToPrivate,		ESync,					&TFsSessionToPrivate::Initialise,			NULL,								&TFsSessionToPrivate::DoRequestL			},
		{	EFsPrivatePath,				ESync,					&TFsPrivatePath::Initialise,				NULL,								&TFsPrivatePath::DoRequestL					},
		{	EFsCreatePrivatePath,		0,						&TFsCreatePrivatePath::Initialise,			NULL,								&TFsCreatePrivatePath::DoRequestL			},
		{	EFsAddExtension,			ESync,					&TFsAddExtension::Initialise,				NULL,								&TFsAddExtension::DoRequestL				},
		{	EFsMountExtension,			0,						&TFsMountExtension::Initialise,				NULL,								&TFsMountExtension::DoRequestL				},
		{	EFsDismountExtension,		0,						&TFsDismountExtension::Initialise,			NULL,								&TFsDismountExtension::DoRequestL			},
		{	EFsRemoveExtension,			ESync,					&TFsRemoveExtension::Initialise,			NULL,								&TFsRemoveExtension::DoRequestL				},
		{	EFsExtensionName,			0,						&TFsExtensionName::Initialise,				NULL,								&TFsExtensionName::DoRequestL				},
		{	EFsStartupInitComplete,		ESync,					&TFsStartupInitComplete::Initialise,		NULL,								&TFsStartupInitComplete::DoRequestL			},
		{	EFsSetLocalDriveMapping,	ESync,					&TFsSetLocalDriveMapping::Initialise,		NULL,								&TFsSetLocalDriveMapping::DoRequestL		},
		{	EFsFinaliseDrive,			0,						&TFsFinaliseDrive::Initialise,				NULL,								&TFsFinaliseDrive::DoRequestL				},
		{	EFsFileDuplicate,			0 | EFileShare,			&TFsFileDuplicate::Initialise,				NULL,								&TFsFileDuplicate::DoRequestL				},	// Not done
		{	EFsFileAdopt,				ESync,					&TFsFileAdopt::Initialise,					NULL,								&TFsFileAdopt::DoRequestL					},	// Not done
		{	EFsSwapFileSystem,			ESync,					&TFsSwapFileSystem::Initialise,				NULL,								&TFsSwapFileSystem::DoRequestL				},
		{	EFsErasePassword,			0,						&TFsErasePassword::Initialise,				NULL,								&TFsErasePassword::DoRequestL				},
		{	EFsReserveDriveSpace,		0,						&TFsReserveDriveSpace::Initialise,			NULL,								&TFsReserveDriveSpace::DoRequestL			},
		{	EFsGetReserveAccess,		ESync,  				&TFsGetReserveAccess::Initialise,			NULL,								&TFsGetReserveAccess::DoRequestL			},
		{	EFsReleaseReserveAccess,	ESync,					&TFsReleaseReserveAccess::Initialise,		NULL,								&TFsReleaseReserveAccess::DoRequestL		},
		{	EFsFileName,				ESync,					&TFsFileName::Initialise,					NULL,								&TFsFileName::DoRequestL					, MSG0(EName)},
		{	EFsGetMediaSerialNumber,	0,					    &TFsGetMediaSerialNumber::Initialise,		NULL,								&TFsGetMediaSerialNumber::DoRequestL		},
		{	EFsFileFullName,			ESync,					&TFsFileFullName::Initialise,				NULL,								&TFsFileFullName::DoRequestL				, MSG0(EName)},	// Wasn't in original list?
		{	EFsAddPlugin,				ESync,					&TFsAddPlugin::Initialise,					NULL,								&TFsAddPlugin::DoRequestL					},
		{	EFsRemovePlugin,			ESync,					&TFsRemovePlugin::Initialise,				NULL,								&TFsRemovePlugin::DoRequestL				},
		{	EFsMountPlugin,				ESync,					&TFsMountPlugin::Initialise,				NULL,								&TFsMountPlugin::DoRequestL					},
		{	EFsDismountPlugin,			0, /*PluginThrdContxt*/	&TFsDismountPlugin::Initialise,				NULL,								&TFsDismountPlugin::DoRequestL				},
		{	EFsPluginName,				ESync,					&TFsPluginName::Initialise,					NULL,								&TFsPluginName::DoRequestL					},
		{	EFsPluginOpen,				ESync | EParseSrc,		&TFsPluginOpen::Initialise,					NULL,								&TFsPluginOpen::DoRequestL					},
		{	EFsPluginSubClose,			ESync,					&TFsSubClose::Initialise,					NULL,								&TFsSubClose::DoRequestL					},
		{	EFsPluginDoRequest,			0,						&TFsPluginDoRequest::Initialise,			NULL,								&TFsPluginDoRequest::DoRequestL				},
		{	EFsPluginDoControl,			0,						&TFsPluginDoControl::Initialise,			NULL,								&TFsPluginDoControl::DoRequestL				},
		{	EFsPluginDoCancel,			0,						&TFsPluginDoCancel::Initialise,				NULL,								&TFsPluginDoCancel::DoRequestL				},
		{	EFsNotifyDismount,			0,						&TFsNotifyDismount::Initialise,				NULL,								&TFsNotifyDismount::DoRequestL				},
		{	EFsNotifyDismountCancel,	ESync,					&TFsNotifyDismountCancel::Initialise,		NULL,								&TFsNotifyDismountCancel::DoRequestL		},
		{	EFsAllowDismount,			0,						&TFsAllowDismount::Initialise,				NULL,								&TFsAllowDismount::DoRequestL				},
		{	EFsSetStartupConfiguration,	ESync,					&TFsSetStartupConfiguration::Initialise,	NULL,								&TFsSetStartupConfiguration::DoRequestL		},
		{	EFsFileReadCancel,			ESync,					&TFsFileReadCancel::Initialise,				NULL,								&TFsFileReadCancel::DoRequestL				},
		{	EFsAddCompositeMount,		ESync,					&TFsAddCompositeMount::Initialise,			NULL,								&TFsAddCompositeMount::DoRequestL			},
		{	EFsSetSessionFlags,			ESync,					&TFsSetSessionFlags::Initialise,			NULL,								&TFsSetSessionFlags::DoRequestL				},
		{	EFsSetSystemDrive,			ESync,					&TFsSetSystemDrive::Initialise,				NULL,								&TFsSetSystemDrive::DoRequestL				},
		{	EFsBlockMap,				EParseSrc,				&TFsBlockMap::Initialise,					NULL,								&TFsBlockMap::DoRequestL					},
		{	EFsUnclamp,					0,						&TFsUnclamp::Initialise,					NULL,								&TFsUnclamp::DoRequestL						},
		{	EFsFileClamp,				EParseSrc,				&TFsFileClamp::Initialise,					NULL,								&TFsFileClamp::DoRequestL					},
		{	EFsQueryVolumeInfoExt,		0,						&TFsQueryVolumeInfoExt::Initialise,			NULL,								&TFsQueryVolumeInfoExt::DoRequestL			},
		{	EFsInitialisePropertiesFile,0,						&TFsInitialisePropertiesFile::Initialise,	NULL,								&TFsInitialisePropertiesFile::DoRequestL	},
		{	EFsFileWriteDirty,			EFileShare,				NULL,										&TFsFileWriteDirty::PostInitialise,	&TFsFileWrite::DoRequestL					},
		{	EFsSynchroniseDriveThread,	0,						&TFsSynchroniseDriveThread::Initialise,		NULL,								&TFsSynchroniseDriveThread::DoRequestL		},
		{	EFsAddProxyDrive,			ESync,					&TFsAddProxyDrive::Initialise,				NULL,								&TFsAddProxyDrive::DoRequestL				},
		{	EFsRemoveProxyDrive,		ESync,					&TFsRemoveProxyDrive::Initialise,			NULL,								&TFsRemoveProxyDrive::DoRequestL			},
		{	EFsMountProxyDrive,			0,						&TFsMountProxyDrive::Initialise,			NULL,								&TFsMountProxyDrive::DoRequestL				},
		{	EFsDismountProxyDrive,		0,						&TFsDismountProxyDrive::Initialise,			NULL,								&TFsDismountProxyDrive::DoRequestL			},
		{	EFsNotificationOpen,		ESync,					&TFsNotificationOpen::Initialise,			NULL,								&TFsNotificationOpen::DoRequestL			},
		{	EFsNotificationBuffer,		ESync,					&TFsNotificationBuffer::Initialise,			NULL,								&TFsNotificationBuffer::DoRequestL			},		
		{	EFsNotificationRequest,		ESync,					&TFsNotificationRequest::Initialise,		NULL,								&TFsNotificationRequest::DoRequestL			},
		{	EFsNotificationCancel,		ESync,					&TFsNotificationCancel::Initialise,			NULL,								&TFsNotificationCancel::DoRequestL			},
		{	EFsNotificationSubClose,	ESync,					&TFsNotificationSubClose::Initialise,		NULL,								&TFsNotificationSubClose::DoRequestL		},
		{	EFsNotificationAdd,			ESync,					&TFsNotificationAdd::Initialise,			NULL,								&TFsNotificationAdd::DoRequestL				},
		{	EFsNotificationRemove,		ESync,					&TFsNotificationRemove::Initialise,			NULL,								&TFsNotificationRemove::DoRequestL			},
		{	EFsLoadCodePage,			0,						&TFsLoadCodePage::Initialise,				NULL,								&TFsLoadCodePage::DoRequestL				},
	};

#endif //SF_OPS_H
