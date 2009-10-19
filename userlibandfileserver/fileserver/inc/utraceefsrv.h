
// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// File Server client-side tracing
// 
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef UTRACEEFSRV_H
#define UTRACEEFSRV_H


#include <f32tracedef.h>

#define MODULEUID EF32TraceUidEfsrv


/**
This defines trace attributes to be used by trace points within the efsrv.dll executable.
*/
namespace UTraceModuleEfsrv
    {

	/** 
	@SYMTraceFormatCategory EF32TraceUidEfsrv 
	*/



    /**
    @SYMTraceFormatId
    */
    enum TFormatId
        {
		EReserved = UTF::KInitialClientFormat-1,

        /** @SYMTraceFormatString "+RFile::Replace() sess %x mode %x FileName %*S" */
		EFileReplace,
        /** @SYMTraceFormatString "-RFile::Replace() r %d subs %x" */
        EFileReplaceReturn,

        /** @SYMTraceFormatString "+RFile::Create() sess %x mode %x FileName %*S" */
		EFileCreate,
        /** @SYMTraceFormatString "-RFile::Create() r %d subs %x" */
        EFileCreateReturn,

        /** @SYMTraceFormatString "+RFile::Open() sess %x mode %x FileName %*S" */
		EFileOpen,
        /** @SYMTraceFormatString "-RFile::Open() r %d subs %x" */
        EFileOpenReturn,

        /** @SYMTraceFormatString "+RFile::Adopt() sess %x subs %x" */
        EFileAdopt,
        /** @SYMTraceFormatString "-RFile::Adopt() r %d sess %x subs %x" */
        EFileAdoptReturn,

        /** @SYMTraceFormatString "+RFile::AdoptFromServer() sess %x subs %x" */
        EFileAdoptFromServer,
        /** @SYMTraceFormatString "-RFile::AdoptFromServer() r %d sess %x subs %x" */
        EFileAdoptFromServerReturn,

        /** @SYMTraceFormatString "+RFile::AdoptFromClient() sess %x subs %x aFsHandleIndex %d aFileHandleIndex %d " */
        EFileAdoptFromClient,
        /** @SYMTraceFormatString "-RFile::AdoptFromClient() r %d sess %x subs %x" */
        EFileAdoptFromClientReturn,

        /** @SYMTraceFormatString "+RFile::AdoptFromCreator() subs %x aFsHandleIndex %d aFileHandleIndex %d" */
        EFileAdoptFromCreator,
        /** @SYMTraceFormatString "-RFile::AdoptFromCreator() r %d sess %x subs %x" */
        EFileAdoptFromCreatorReturn,

        /** @SYMTraceFormatString "+RFile::Duplicate() sess %x subs %x aType %d" */
        EFileDuplicate,
        /** @SYMTraceFormatString "-RFile::Duplicate() r %d sess %x subs %x" */
        EFileDuplicateReturn,

        
        /** @SYMTraceFormatString "+RFile::TransferToServer() sess %x subs %x aFsHandleIndex %d aFileHandleIndex %d" */
        EFileTransferToServer,
        /** @SYMTraceFormatString "-RFile::TransferToServer() r %d" */
        EFileTransferToServerReturn,

        /** @SYMTraceFormatString "+RFile::TransferToClient() sess %x subs %x aFileHandleIndex %d" */
        EFileTransferToClient,
        /** @SYMTraceFormatString "-RFile::TransferToClient() r %d" */
        EFileTransferToClientReturn,

        /** @SYMTraceFormatString "+RFile::TransferToProcess() sess %x subs %x aFsHandleIndex %d aFileHandleIndex %d" */
        EFileTransferToProcess,
        /** @SYMTraceFormatString "-RFile::TransferToProcess() r %d" */
        EFileTransferToProcessReturn,

		
		/** @SYMTraceFormatString "+RFile::Name() sess %x subs %x" */
        EFileName,
		/** @SYMTraceFormatString "-RFile::Name() r %d aName %*S" */
        EFileNameReturn,

		/** @SYMTraceFormatString "+RFile::FullName() sess %x subs %x" */
        EFileFullName,
		/** @SYMTraceFormatString "-RFile::FullName() r %d aName %*S" */
        EFileFullNameReturn,

		
		
		/** @SYMTraceFormatString "+RFile::Temp() sess %x aPath %*S aMode %x" */
        EFileTemp,
        /** @SYMTraceFormatString "-RFile::Temp() r %d subs %x aName %*S" */
        EFileTempReturn,


        /** @SYMTraceFormatString "+RFile::Write1() sess %x subs %x len %d" */
        EFileWrite1,
		/** @SYMTraceFormatString "-RFile::Write1() r %d" */
        EFileWrite1Return,
		
        /** @SYMTraceFormatString "+RFile::Write2() sess %x subs %x len %d status %x" */
        EFileWrite2,
		/** @SYMTraceFormatString "-RFile::Write2()" */
        EFileWrite2Return,
		
        /** @SYMTraceFormatString "+RFile::Write3() sess %x subs %x pos %ld len %d" */
        EFileWrite3,
		/** @SYMTraceFormatString "-RFile::Write3() r %d" */
        EFileWrite3Return,
		
        /** @SYMTraceFormatString "+RFile::Write4() sess %x subs %x pos %ld len %d status %x" */
        EFileWrite4,
		/** @SYMTraceFormatString "-RFile::Write4()" */
        EFileWrite4Return,
		



        /** @SYMTraceFormatString "+RFile::Read1() sess %x subs %x maxlen %d" */
        EFileRead1,
		/** @SYMTraceFormatString "-RFile::Read1() r %d len %d" */
        EFileRead1Return,

        /** @SYMTraceFormatString "+RFile::Read2() sess %x subs %x maxlen %d status %x" */
        EFileRead2,
		/** @SYMTraceFormatString "-RFile::Read2()" */
        EFileRead2Return,

        /** @SYMTraceFormatString "+RFile::Read3() sess %x subs %x aPos %ld maxlen %d" */
        EFileRead3,
		/** @SYMTraceFormatString "-RFile::Read3() r %d len %d" */
        EFileRead3Return,

        /** @SYMTraceFormatString "+RFile::Read4() sess %x subs %x aPos %ld maxlen %d status %x" */
        EFileRead4,
		/** @SYMTraceFormatString "-RFile::Read4()" */
        EFileRead4Return,





		/** @SYMTraceFormatString "+RFile::Lock() sess %x subs %x aPos %ld aLength %d" */
        EFileLock,
        /** @SYMTraceFormatString "-RFile::Lock() r %d" */
        EFileLockReturn,

		/** @SYMTraceFormatString "+RFile::UnLock() sess %x subs %x aPos %ld aLength %d" */
        EFileUnLock,
        /** @SYMTraceFormatString "-RFile::UnLock() r %d" */
        EFileUnLockReturn,

		/** @SYMTraceFormatString "+RFile::Seek() sess %x subs %x aMode %x aPos %ld" */
        EFileSeek,
        /** @SYMTraceFormatString "-RFile::Seek() r %d" */
        EFileSeekReturn,

		/** @SYMTraceFormatString "+RFile::Flush() sess %x subs %x status %x" */
        EFileFlush,
        /** @SYMTraceFormatString "-RFile::Flush() r %d" */
        EFileFlushReturn,

		/** @SYMTraceFormatString "+RFile::Size1() sess %x subs %x" */
        EFileSize,
        /** @SYMTraceFormatString "-RFile::Size1() r %d aSize %d" */
        EFileSizeReturn,

		/** @SYMTraceFormatString "+RFile::Size2() sess %x subs %x" */
        EFileSize2,
        /** @SYMTraceFormatString "-RFile::Size2() r %d aSize %ld" */
        EFileSize2Return,

		/** @SYMTraceFormatString "+RFile::SetSize() sess %x subs %x aSize %ld" */
        EFileSetSize,
        /** @SYMTraceFormatString "-RFile::SetSize() r %d" */
        EFileSetSizeReturn,

		/** @SYMTraceFormatString "+RFile::Att() sess %x subs %x" */
        EFileAtt,
        /** @SYMTraceFormatString "-RFile::Att() r %d aVal %x" */
        EFileAttReturn,

		/** @SYMTraceFormatString "+RFile::SetAtt() sess %x subs %x aSetAttMask %x aClearAttMask %x" */
        EFileSetAtt,
        /** @SYMTraceFormatString "-RFile::SetAtt() r %d" */
        EFileSetAttReturn,

		/** @SYMTraceFormatString "+RFile::Modified() sess %x subs %x" */
        EFileModified,
        /** @SYMTraceFormatString "-RFile::Modified() r %d aTime %lx" */
        EFileModifiedReturn,

		/** @SYMTraceFormatString "+RFile::SetModified() sess %x subs %x aTime %lx" */
        EFileSetModified,
        /** @SYMTraceFormatString "-RFile::SetModified() r %d" */
        EFileSetModifiedReturn,

		/** @SYMTraceFormatString "+RFile::Set() sess %x subs %x aTime %lx aSetAttMask %x aClearAttMask %x" */
        EFileSet,
        /** @SYMTraceFormatString "-RFile::Set() r %d" */
        EFileSetReturn,

		/** @SYMTraceFormatString "+RFile::ChangeMode() sess %x subs %x aNewMode %x" */
        EFileChangeMode,
        /** @SYMTraceFormatString "-RFile::ChangeMode() r %d" */
        EFileChangeModeReturn,

		/** @SYMTraceFormatString "+RFile::Rename() sess %x subs %x aNewName %*S" */
        EFileRename,
        /** @SYMTraceFormatString "-RFile::Rename() r %d" */
        EFileRenameReturn,

		/** @SYMTraceFormatString "+RFile::Drive() sess %x subs %x" */
        EFileDrive,
        /** @SYMTraceFormatString "-RFile::Drive() r %d driveAtt %x mediaAtt %x type %x" */
        EFileDriveReturn,

		/** @SYMTraceFormatString "+RFile::Clamp() sess %x subs %x" */
        EFileClamp,
        /** @SYMTraceFormatString "-RFile::Clamp() r %d" */
        EFileClampReturn,

		/** @SYMTraceFormatString "+RFile::BlockMap() sess %x subs %x aStartPos %ld aEndPos %ld aBlockMapusage %d" */
        EFileBlockMap,
        /** @SYMTraceFormatString "-RFile::BlockMap() r %d" */
        EFileBlockMapReturn,

		/** @SYMTraceFormatString "+RFile::Close() sess %x subs %x" */
        EFileClose,
        /** @SYMTraceFormatString "-RFile::Close()" */
        EFileCloseReturn,



		/** @SYMTraceFormatString "+RFS::DriveToChar() aDrive %d" */
		EFsDriveToChar,
		/** @SYMTraceFormatString "-RFs::DriveToChar() r %d aChar %c" */
		EFsDriveToCharReturn,
		/** @SYMTraceFormatString "+RFs::IsRomAddress() aPtr %x" */
		EFsIsRomAddress,
		/** @SYMTraceFormatString "-RFS::IsRomAddress() r %d" */
		EFsIsRomAddressReturn,
		/** @SYMTraceFormatString "+RFs::GetSystemDrive()" */
		EFsGetSystemDrive,
		/** @SYMTraceFormatString "-RFs::GetSystemDrive() r %d" */
		EFsGetSystemDriveReturn,
		/** @SYMTraceFormatString "+RFs::GetSystemDriveChar()" */
		EFsGetSystemDriveChar,
		/** @SYMTraceFormatString "-RFs::GetSystemDriveChar() r %c" */
		EFsGetSystemDriveCharReturn,
		/** @SYMTraceFormatString "+RFs::SetSystemDrive() sess %x" */
		EFsSetSystemDrive,
		/** @SYMTraceFormatString "-RFs::SetSystemDrive() r %d" */
		EFsSetSystemDriveReturn,
		/** @SYMTraceFormatString "+RFs::Connect()" */
		EFsConnect,
		/** @SYMTraceFormatString "-RFs::Connect() r %d sess %x" */
		EFsConnectReturn,
		/** @SYMTraceFormatString "+RFs::SetSessionToPrivate() sess %x" */
		EFsSetSessionToPrivate,
		/** @SYMTraceFormatString "-RFs::SetSessionToPrivate() r %d" */
		EFsSetSessionToPrivateReturn,
		/** @SYMTraceFormatString "+RFs::PrivatePath() sess %x" */
		EFsPrivatePath,
		/** @SYMTraceFormatString "-RFs::PrivatePath() r %d aPath %*S" */
		EFsPrivatePathReturn,
		/** @SYMTraceFormatString "+RFs::CreatePrivatePath() sess %x aDrive %d" */
		EFsCreatePrivatePath,
		/** @SYMTraceFormatString "-RFs::CreatePrivatePath() r %d" */
		EFsCreatePrivatePathReturn,
		/** @SYMTraceFormatString "+RFs::Version() sess %x" */
		EFsVersion,
		/** @SYMTraceFormatString "-RFs::Version() iMajor %d iMinor %d iBuild %d" */
		EFsVersionReturn,
		/** @SYMTraceFormatString "+RFs::AddFileSystem() sess %x aFileName %*S" */
		EFsAddFileSystem,
		/** @SYMTraceFormatString "-RFs::AddFileSystem() r %d" */
		EFsAddFileSystemReturn,
		/** @SYMTraceFormatString "+RFs::RemoveFileSystem() sess %x aFileName %*S" */
		EFsRemoveFileSystem,
		/** @SYMTraceFormatString "-RFs::RemoveFileSystem() r %d" */
		EFsRemoveFileSystemReturn,
		/** @SYMTraceFormatString "+RFs::MountFileSystem1() sess %x aFileSystemName %*S aDrive %d" */
		EFsMountFileSystem1,
		/** @SYMTraceFormatString "-RFs::MountFileSystem1() r %d" */
		EFsMountFileSystem1Return,
		/** @SYMTraceFormatString "+RFs::MountFileSystem2() sess %x aFileSystemName %*S aDrive %d aIsSync %d" */
		EFsMountFileSystem2,
		/** @SYMTraceFormatString "-RFs::MountFileSystem2() r %d" */
		EFsMountFileSystem2Return,
		/** @SYMTraceFormatString "+RFs::MountFileSystem3() sess %x aFileSystemName %*S aExtensionName %*S aDrive %d" */
		EFsMountFileSystem3,
		/** @SYMTraceFormatString "-RFs::MountFileSystem3() r %d" */
		EFsMountFileSystem3Return,
		/** @SYMTraceFormatString "+RFs::MountFileSystem4() sess %x aFileSystemName %*S aExtensionName %*S aDrive %d aIsSync %d" */
		EFsMountFileSystem4,
		/** @SYMTraceFormatString "-RFs::MountFileSystem4() r %d" */
		EFsMountFileSystem4Return,
		/** @SYMTraceFormatString "+RFs::MountFileSystemAndScan1() sess %x aFileSystemName %*S aDrive %d" */
		EFsMountFileSystemAndScan1,
		/** @SYMTraceFormatString "-RFs::MountFileSystemAndScan1() r %d" */
		EFsMountFileSystemAndScan1Return,
		/** @SYMTraceFormatString "+RFs::MountFileSystemAndScan2() sess %x aFileSystemName %*S aExtensionName %*S aDrive %d " */
		EFsMountFileSystemAndScan2,
		/** @SYMTraceFormatString "-RFs::MountFileSystemAndScan2() r %d aIsMountSuccess %d" */
		EFsMountFileSystemAndScan2Return,
		/** @SYMTraceFormatString "+RFs::DismountFileSystem() sess %x aFileSystemName %*S aDrive %d" */
		EFsDismountFileSystem,
		/** @SYMTraceFormatString "-RFs::DismountFileSystem() r %d" */
		EFsDismountFileSystemReturn,
		/** @SYMTraceFormatString "+RFs::FileSystemName() sess %x aDrive %d" */
		EFsFileSystemName,
		/** @SYMTraceFormatString "-RFs::FileSystemName() r %d aName %*S" */
		EFsFileSystemNameReturn,
		/** @SYMTraceFormatString "+RFs::AddExtension() sess %x aFileName %*S" */
		EFsAddExtension,
		/** @SYMTraceFormatString "-RFs::AddExtension() r %d" */
		EFsAddExtensionReturn,
		/** @SYMTraceFormatString "+RFs::MountExtension() sess %x aExtensionName %*S aDrive %d" */
		EFsMountExtension,
		/** @SYMTraceFormatString "-RFs::MountExtension() r %d" */
		EFsMountExtensionReturn,
		/** @SYMTraceFormatString "+RFs::DismountExtension() sess %x aExtensionName %*S aDrive %d" */
		EFsDismountExtension,
		/** @SYMTraceFormatString "-RFs::DismountExtension() r %d" */
		EFsDismountExtensionReturn,
		/** @SYMTraceFormatString "+RFs::RemoveExtension() sess %x aExtensionName %*S" */
		EFsRemoveExtension,
		/** @SYMTraceFormatString "-RFs::RemoveExtension() r %d" */
		EFsRemoveExtensionReturn,
		/** @SYMTraceFormatString "+RFs::ExtensionName() sess %x aExtensionName %*S aDrive %d aPos %d" */
		EFsExtensionName,
		/** @SYMTraceFormatString "-RFs::ExtensionName() r %d" */
		EFsExtensionNameReturn,
		/** @SYMTraceFormatString "+RFs::RemountDrive() sess %x aDrive %d aMountInfo %x aFlags %x" */
		EFsRemountDrive,
		/** @SYMTraceFormatString "-RFs::RemountDrive() r %d" */
		EFsRemountDriveReturn,
		/** @SYMTraceFormatString "+RFs::NotifyChange1() sess %x aType %x status %x" */
		EFsNotifyChange1,
		/** @SYMTraceFormatString "-RFs::NotifyChange1() r %d" */
		EFsNotifyChange1Return,
		/** @SYMTraceFormatString "+RFs::NotifyChange2() sess %x aType %x status %x aPathName %*S" */
		EFsNotifyChange2,
		/** @SYMTraceFormatString "-RFs::NotifyChange2() r %d" */
		EFsNotifyChange2Return,
		/** @SYMTraceFormatString "+RFs::NotifyChangeCancel1() sess %x" */
		EFsNotifyChangeCancel1,
		/** @SYMTraceFormatString "-RFs::NotifyChangeCancel1() r %d" */
		EFsNotifyChangeCancel1Return,
		/** @SYMTraceFormatString "+RFs::NotifyChangeCancel2() sess %x status %x" */
		EFsNotifyChangeCancel2,
		/** @SYMTraceFormatString "-RFs::NotifyChangeCancel2() r %d" */
		EFsNotifyChangeCancel2Return,
		/** @SYMTraceFormatString "+RFs::NotifyDiskSpace() sess %x aThreshold %ld aDrive %d status %x" */
		EFsNotifyDiskSpace,
		/** @SYMTraceFormatString "-RFs::NotifyDiskSpace() r %d" */
		EFsNotifyDiskSpaceReturn,
		/** @SYMTraceFormatString "+RFs::NotifyDiskSpaceCancel1() sess %x status %x" */
		EFsNotifyDiskSpaceCancel1,
		/** @SYMTraceFormatString "-RFs::NotifyDiskSpaceCancel1() r %d" */
		EFsNotifyDiskSpaceCancel1Return,
		/** @SYMTraceFormatString "+RFs::NotifyDiskSpaceCancel2() sess %x" */
		EFsNotifyDiskSpaceCancel2,
		/** @SYMTraceFormatString "-RFs::NotifyDiskSpaceCancel2() r %d" */
		EFsNotifyDiskSpaceCancel2Return,
		/** @SYMTraceFormatString "+RFs::DriveList1() sess %x" */
		EFsDriveList1,
		/** @SYMTraceFormatString "-RFs::DriveList1() r %d" */
		EFsDriveList1Return,
		/** @SYMTraceFormatString "+RFs::DriveList2() sess %x aFlags %x" */
		EFsDriveList2,
		/** @SYMTraceFormatString "-RFs::DriveList2() r %d" */
		EFsDriveList2Return,
		/** @SYMTraceFormatString "+RFs::Drive() sess %x aDrive %d" */
		EFsDrive,
		/** @SYMTraceFormatString "-RFs::Drive() r %d driveAtt %x mediaAtt %x type %x" */
		EFsDriveReturn,
		/** @SYMTraceFormatString "+RFs::Volume1() sess %x aDrive %d" */
		EFsVolume1,
		/** @SYMTraceFormatString "-RFs::Volume1() r %d iUniqueID %x iSize %ld iFree %ld iFileCacheFlags %x" */
		EFsVolume1Return,
		/** @SYMTraceFormatString "+RFs::Volume2() sess %x aDrive %d status %x" */
		EFsVolume2,
		/** @SYMTraceFormatString "-RFs::Volume2() r %d" */
		EFsVolume2Return,
		/** @SYMTraceFormatString "+RFs::SetVolumeLabel() sess %x aName %*S aDrive %d" */
		EFsSetVolumeLabel,
		/** @SYMTraceFormatString "-RFs::SetVolumeLabel() r %d" */
		EFsSetVolumeLabelReturn,
		/** @SYMTraceFormatString "+RFs::Subst() sess %x aPath %*S aDrive %d" */
		EFsSubst,
		/** @SYMTraceFormatString "-RFs::Subst() r %d" */
		EFsSubstReturn,
		/** @SYMTraceFormatString "+RFs::SetSubst() sess %x aPath %%S aDrive %d" */
		EFsSetSubst,
		/** @SYMTraceFormatString "-RFs::SetSubst() r %d" */
		EFsSetSubstReturn,
		/** @SYMTraceFormatString "+RFs::RealName() sess %x aName %*S" */
		EFsRealName,
		/** @SYMTraceFormatString "-RFs::RealName() r %d aResult %*S" */
		EFsRealNameReturn,
		/** @SYMTraceFormatString "+RFs::GetMediaSerialNumber() sess %x aDrive %d" */
		EFsGetMediaSerialNumber,
		/** @SYMTraceFormatString "-RFs::GetMediaSerialNumber() r %d aSerialNum %*x" */
		EFsGetMediaSerialNumberReturn,
		/** @SYMTraceFormatString "+RFs::SessionPath() sess %x" */
		EFsSessionPath,
		/** @SYMTraceFormatString "-RFs::SessionPath() r %d aPath %*S" */
		EFsSessionPathReturn,
		/** @SYMTraceFormatString "+RFs::SetSessionPath() sess %x aPath %*S" */
		EFsSetSessionPath,
		/** @SYMTraceFormatString "-RFs::SetSessionPath() r %d" */
		EFsSetSessionPathReturn,
		/** @SYMTraceFormatString "+RFs::MkDir() sess %x aPath %*S" */
		EFsMkDir,
		/** @SYMTraceFormatString "-RFs::MkDir() r %d" */
		EFsMkDirReturn,
		/** @SYMTraceFormatString "+RFs::MkDirAll() sess %x aPath %*S" */
		EFsMkDirAll,
		/** @SYMTraceFormatString "-RFs::MkDirAll() r %d" */
		EFsMkDirAllReturn,
		/** @SYMTraceFormatString "+RFs::RmDir() sess %x aPath %*S" */
		EFsRmDir,
		/** @SYMTraceFormatString "-RFs::RmDir() r %d" */
		EFsRmDirReturn,
		/** @SYMTraceFormatString "+RFs::GetDir1() sess %x aName %*S aUidType0 %x aUidType1 %x aUidType2 %x aKey %x" */
		EFsGetDir1,
		/** @SYMTraceFormatString "-RFs::GetDir1() r %d" */
		EFsGetDir1Return,
		/** @SYMTraceFormatString "+RFs::GetDir2() sess %x aName %*S anAttMask %x aKey %x" */
		EFsGetDir2,
		/** @SYMTraceFormatString "-RFs::GetDir2() r %d" */
		EFsGetDir2Return,
		/** @SYMTraceFormatString "+RFs::GetDir3() sess %x aName %*S anAttMask %x aKey %x" */
		EFsGetDir3,
		/** @SYMTraceFormatString "-RFs::GetDir3() r %d" */
		EFsGetDir3Return,
		/** @SYMTraceFormatString "+RFs::Parse1() sess %x aName %*S" */
		EFsParse1,
		/** @SYMTraceFormatString "-RFs::Parse1() r %d" */
		EFsParse1Return,
		/** @SYMTraceFormatString "+RFs::Parse2() sess %x aName %*S aRelated %*S" */
		EFsParse2,
		/** @SYMTraceFormatString "-RFs::Parse2() r %d" */
		EFsParse2Return,
		/** @SYMTraceFormatString "+RFs::Delete() sess %x aName %*S" */
		EFsDelete,
		/** @SYMTraceFormatString "-RFs::Delete() r %d" */
		EFsDeleteReturn,
		/** @SYMTraceFormatString "+RFs::Rename() sess %x anOldName %*S aNewName %*S" */
		EFsRename,
		/** @SYMTraceFormatString "-RFs::Rename() r %d" */
		EFsRenameReturn,
		/** @SYMTraceFormatString "+RFs::Replace() sess %x anOldName %*S aNewName %*S" */
		EFsReplace,
		/** @SYMTraceFormatString "-RFs::Replace() r %d" */
		EFsReplaceReturn,
		/** @SYMTraceFormatString "+RFs::Att() sess %x aName %*S" */
		EFsAtt,
		/** @SYMTraceFormatString "-RFs::Att() r %d aVal %x" */
		EFsAttReturn,
		/** @SYMTraceFormatString "+RFs::SetAtt() sess %x aName %*S aSetAttMask %x aClearAttMask %x" */
		EFsSetAtt,
		/** @SYMTraceFormatString "-RFs::SetAtt() r %d" */
		EFsSetAttReturn,
		/** @SYMTraceFormatString "+RFs::Modified() sess %x aName %*S" */
		EFsModified,
		/** @SYMTraceFormatString "-RFs::Modified() r %d aTime %lx" */
		EFsModifiedReturn,
		/** @SYMTraceFormatString "+RFs::SetModified() sess %x aName %*S aTime %lx" */
		EFsSetModified,
		/** @SYMTraceFormatString "-RFs::SetModified() r %d" */
		EFsSetModifiedReturn,
		/** @SYMTraceFormatString "+RFs::Entry() sess %x aName %*S" */
		EFsEntry,
		/** @SYMTraceFormatString "-RFs::Entry() r %d att %x modified %lx size %d" */
		EFsEntryReturn,
		/** @SYMTraceFormatString "+RFs::SetEntry() sess %x aName %*S aTime %lx aSetAttMask %x aClearAttMask %x" */
		EFsSetEntry,
		/** @SYMTraceFormatString "-RFs::SetEntry() r %d" */
		EFsSetEntryReturn,
		/** @SYMTraceFormatString "+RFs::ReadFileSection() sess %x aName %*S aPos %ld aLength %d" */
		EFsReadFileSection,
		/** @SYMTraceFormatString "-RFs::ReadFileSection() r %d" */
		EFsReadFileSectionReturn,
		/** @SYMTraceFormatString "+RFs::ResourceCountMarkStart() sess %x" */
		EFsResourceCountMarkStart,
		/** @SYMTraceFormatString "-RFs::ResourceCountMarkStart() r %d" */
		EFsResourceCountMarkStartReturn,
		/** @SYMTraceFormatString "+RFs::ResourceCountMarkEnd() sess %x" */
		EFsResourceCountMarkEnd,
		/** @SYMTraceFormatString "-RFs::ResourceCountMarkEnd() r %d" */
		EFsResourceCountMarkEndReturn,
		/** @SYMTraceFormatString "+RFs::ResourceCount() sess %x" */
		EFsResourceCount,
		/** @SYMTraceFormatString "-RFs::ResourceCount() r %d" */
		EFsResourceCountReturn,
		/** @SYMTraceFormatString "+RFs::CheckDisk() sess %x aDrive %*S" */
		EFsCheckDisk,
		/** @SYMTraceFormatString "-RFs::CheckDisk() r %d" */
		EFsCheckDiskReturn,
		/** @SYMTraceFormatString "+RFs::ScanDrive() sess %x aDrive %*S" */
		EFsScanDrive,
		/** @SYMTraceFormatString "-RFs::ScanDrive() r %d" */
		EFsScanDriveReturn,
		/** @SYMTraceFormatString "+RFs::GetShortName() sess %x aLongName %*S" */
		EFsGetShortName,
		/** @SYMTraceFormatString "-RFs::GetShortName() r %d aShortName %*S" */
		EFsGetShortNameReturn,
		/** @SYMTraceFormatString "+RFs::GetLongName() sess %x aShortName %*S" */
		EFsGetLongName,
		/** @SYMTraceFormatString "-RFs::GetLongName() r %d aLongName %*S" */
		EFsGetLongNameReturn,
		/** @SYMTraceFormatString "+RFs::IsFileOpen() sess %x aFileName %*S" */
		EFsIsFileOpen,
		/** @SYMTraceFormatString "-RFs::IsFileOpen() r %d anAnswer %d" */
		EFsIsFileOpenReturn,
		/** @SYMTraceFormatString "+RFs::GetNotifyUser() sess %x" */
		EFsGetNotifyUser,
		/** @SYMTraceFormatString "-RFs::GetNotifyUser() r %d" */
		EFsGetNotifyUserReturn,
		/** @SYMTraceFormatString "+RFs::SetNotifyUser() sess %x aValue %d" */
		EFsSetNotifyUser,
		/** @SYMTraceFormatString "-RFs::SetNotifyUser() r %d" */
		EFsSetNotifyUserReturn,
		/** @SYMTraceFormatString "+RFs::IsFileInRom() sess %x aFileName %*S" */
		EFsIsFileInRom,
		/** @SYMTraceFormatString "-RFs::IsFileInRom() r %d" */
		EFsIsFileInRomReturn,
		/** @SYMTraceFormatString "+RFs::IsValidName1() sess %x aFileName %*S" */
		EFsIsValidName1,
		/** @SYMTraceFormatString "-RFs::IsValidName1() r %d" */
		EFsIsValidName1Return,
		/** @SYMTraceFormatString "+RFs::IsValidName2() sess %x aFileName %*S" */
		EFsIsValidName2,
		/** @SYMTraceFormatString "-RFs::IsValidName2() r %d aBadChar %c" */
		EFsIsValidName2Return,
		/** @SYMTraceFormatString "+RFs::IsValidName3() sess %x aFileName %*S" */
		EFsIsValidName3,
		/** @SYMTraceFormatString "-RFs::IsValidName3() r %d err %d" */
		EFsIsValidName3Return,
		/** @SYMTraceFormatString "+RFs::GetDriveName() sess %x aDrive %d" */
		EFsGetDriveName,
		/** @SYMTraceFormatString "-RFs::GetDriveName() r %d aDriveName %*S" */
		EFsGetDriveNameReturn,
		/** @SYMTraceFormatString "+RFs::SetDriveName() sess %x aDrive %d aDriveName %*S" */
		EFsSetDriveName,
		/** @SYMTraceFormatString "-RFs::SetDriveName() r %d" */
		EFsSetDriveNameReturn,
		/** @SYMTraceFormatString "+RFs::LockDrive() sess %x aDrv %d aStore %d" */
		EFsLockDrive,
		/** @SYMTraceFormatString "-RFs::LockDrive() r %d" */
		EFsLockDriveReturn,
		/** @SYMTraceFormatString "+RFs::UnlockDrive() sess %x aDrv %d aStore %d" */
		EFsUnlockDrive,
		/** @SYMTraceFormatString "-RFs::UnlockDrive() r %d" */
		EFsUnlockDriveReturn,
		/** @SYMTraceFormatString "+RFs::ClearPassword() sess %x aDrv %d" */
		EFsClearPassword,
		/** @SYMTraceFormatString "-RFs::ClearPassword() r %d" */
		EFsClearPasswordReturn,
		/** @SYMTraceFormatString "+RFs::ErasePassword() sess %x aDrv %d" */
		EFsErasePassword,
		/** @SYMTraceFormatString "-RFs::ErasePassword() r %d" */
		EFsErasePasswordReturn,
		/** @SYMTraceFormatString "+RFs::StartupInitComplete() sess %x status %x" */
		EFsStartupInitComplete,
		/** @SYMTraceFormatString "-RFs::StartupInitComplete() r %d" */
		EFsStartupInitCompleteReturn,
		/** @SYMTraceFormatString "+RFs::SetLocalDriveMapping() sess %x aMapping %*x" */
		EFsSetLocalDriveMapping,
		/** @SYMTraceFormatString "-RFs::SetLocalDriveMapping() r %d" */
		EFsSetLocalDriveMappingReturn,
		/** @SYMTraceFormatString "+RFs::FinaliseDrive() sess %x aDriveNo %d aMode %d" */
		EFsFinaliseDrive,
		/** @SYMTraceFormatString "-RFs::FinaliseDrive() r %d" */
		EFsFinaliseDriveReturn,
		/** @SYMTraceFormatString "+RFs::FinaliseDrives() sess %x" */
		EFsFinaliseDrives,
		/** @SYMTraceFormatString "-RFs::FinaliseDrives() r %d" */
		EFsFinaliseDrivesReturn,
		/** @SYMTraceFormatString "+RFs::SwapFileSystem() sess %x aOldFileSystemName %*S aNewFileSystemName %*S aDrive %d" */
		EFsSwapFileSystem,
		/** @SYMTraceFormatString "-RFs::SwapFileSystem() r %d" */
		EFsSwapFileSystemReturn,
		/** @SYMTraceFormatString "+RFs::AddCompositeMount() sess %x aFileSystemName %*S aLocalDriveToMount %d aCompositeDrive %d aSync %d" */
		EFsAddCompositeMount,
		/** @SYMTraceFormatString "-RFs::AddCompositeMount() r %d" */
		EFsAddCompositeMountReturn,
		/** @SYMTraceFormatString "+RFs::ReserveDriveSpace() sess %x aDriveNo %d aSpace %d" */
		EFsReserveDriveSpace,
		/** @SYMTraceFormatString "-RFs::ReserveDriveSpace() r %d" */
		EFsReserveDriveSpaceReturn,
		/** @SYMTraceFormatString "+RFs::GetReserveAccess() sess %x aDriveNo %d" */
		EFsGetReserveAccess,
		/** @SYMTraceFormatString "-RFs::GetReserveAccess() r %d" */
		EFsGetReserveAccessReturn,
		/** @SYMTraceFormatString "+RFs::ReleaseReserveAccess() sess %x aDriveNo %d" */
		EFsReleaseReserveAccess,
		/** @SYMTraceFormatString "-RFs::ReleaseReserveAccess() r %d" */
		EFsReleaseReserveAccessReturn,
		/** @SYMTraceFormatString "+RFs::NotifyDismount() sess %x aDrive %d stauts %x aMode %d" */
		EFsNotifyDismount,
		/** @SYMTraceFormatString "-RFs::NotifyDismount() r %d" */
		EFsNotifyDismountReturn,
		/** @SYMTraceFormatString "+RFs::NotifyDismountCancel1() sess %x status %x" */
		EFsNotifyDismountCancel1,
		/** @SYMTraceFormatString "-RFs::NotifyDismountCancel1() r %d" */
		EFsNotifyDismountCancel1Return,
		/** @SYMTraceFormatString "+RFs::NotifyDismountCancel2() sess %x" */
		EFsNotifyDismountCancel2,
		/** @SYMTraceFormatString "-RFs::NotifyDismountCancel2() r %d" */
		EFsNotifyDismountCancel2Return,
		/** @SYMTraceFormatString "+RFs::AllowDismount() sess %x aDrive %d" */
		EFsAllowDismount,
		/** @SYMTraceFormatString "-RFs::AllowDismount() r %d" */
		EFsAllowDismountReturn,
		/** @SYMTraceFormatString "+RFs::SetStartupConfiguration() sess %x aCommand %d aParam1 %x aParam2 %x" */
		EFsSetStartupConfiguration,
		/** @SYMTraceFormatString "-RFs::SetStartupConfiguration() r %d" */
		EFsSetStartupConfigurationReturn,
		/** @SYMTraceFormatString "+RFs::SetNotifyChange() sess %x aNotifyChange %d" */
		EFsSetNotifyChange,
		/** @SYMTraceFormatString "-RFs::SetNotifyChange() r %d" */
		EFsSetNotifyChangeReturn,
		/** @SYMTraceFormatString "+RFs::InitialisePropertiesFile() sess %x filePtr %x fileLen %d" */
		EFsInitialisePropertiesFile,
		/** @SYMTraceFormatString "-RFs::InitialisePropertiesFile() r %d" */
		EFsInitialisePropertiesFileReturn,
		/** @SYMTraceFormatString "+RFs::QueryVolumeInfoExt() sess %x aDrive %d aCommand %d" */
		EFsQueryVolumeInfoExt,
		/** @SYMTraceFormatString "-RFs::QueryVolumeInfoExt() r %d" */
		EFsQueryVolumeInfoExtReturn,
		/** @SYMTraceFormatString "+RFs::VolumeIOParam() sess %x aDrive %d" */
		EFsVolumeIOParam,
		/** @SYMTraceFormatString "-RFs::VolumeIOParam() r %d iBlockSize %d iClusterSize %d iRecReadBufSize %d iRecWriteBufSize %d" */
		EFsVolumeIOParamReturn,
		/** @SYMTraceFormatString "+RFs::FileSystemSubType() sess %x aDrive %d aName %*S" */
		EFsFileSystemSubType,
		/** @SYMTraceFormatString "-RFs::FileSystemSubType() r %d" */
		EFsFileSystemSubTypeReturn,

		/** @SYMTraceFormatString "+RFs::AddPlugin() sess %x aFileName %*S" */
        EFsAddPlugin,
        /** @SYMTraceFormatString "-RFs::AddPlugin() r %d" */
        EFsAddPluginReturn,

		/** @SYMTraceFormatString "+RFs::RemovePlugin() sess %x aPluginName %*S" */
        EFsRemovePlugin,
        /** @SYMTraceFormatString "-RFs::RemovePlugin() r %d" */
        EFsRemovePluginReturn,

		/** @SYMTraceFormatString "+RFs::MountPlugin1() sess %x aPluginName %*S" */
        EFsMountPlugin1,
        /** @SYMTraceFormatString "-RFs::MountPlugin1() r %d" */
        EFsMountPlugin1Return,

		/** @SYMTraceFormatString "+RFs::MountPlugin2() sess %x aPluginName %*S aDrive %d" */
        EFsMountPlugin2,
        /** @SYMTraceFormatString "-RFs::MountPlugin2() r %d" */
        EFsMountPlugin2Return,

		/** @SYMTraceFormatString "+RFs::MountPlugin3() sess %x aPluginName %*S aDrive %d aPos %d" */
        EFsMountPlugin3,
        /** @SYMTraceFormatString "-RFs::MountPlugin3() r %d" */
        EFsMountPlugin3Return,

		/** @SYMTraceFormatString "+RFs::DismountPlugin() sess %x aPluginName %*S" */
        EFsDismountPlugin1,
        /** @SYMTraceFormatString "-RFs::DismountPlugin() r %d" */
        EFsDismountPlugin1Return,

		/** @SYMTraceFormatString "+RFs::DismountPlugin2() sess %x aPluginName %*S aDrive %d" */
        EFsDismountPlugin2,
        /** @SYMTraceFormatString "-RFs::DismountPlugin2() r %d" */
        EFsDismountPlugin2Return,

		/** @SYMTraceFormatString "+RFs::DismountPlugin3() sess %x aPluginName %*S aDrive %d aPos %d" */
        EFsDismountPlugin3,
        /** @SYMTraceFormatString "-RFs::DismountPlugin3() r %d" */
        EFsDismountPlugin3Return,

		/** @SYMTraceFormatString "+RFs::PluginName() sess %x aDrive %d aPos %d" */
        EFsPluginName,
        /** @SYMTraceFormatString "-RFs::PluginName() r %d aName %*S" */
        EFsPluginNameReturn,

		/** @SYMTraceFormatString "+RFs::Close() sess %x" */
		EFsClose,
		/** @SYMTraceFormatString "-RFs::Close() r %d" */
		EFsCloseReturn,


		/** @SYMTraceFormatString "+RDir::Open1() sess %x aName %*S aUidType0 %x aUidType1 %x aUidType2 %x" */
        EDirOpen1,
        /** @SYMTraceFormatString "-RDir::Open1() r %d subs %x" */
        EDirOpen1Return,

		/** @SYMTraceFormatString "+RDir::Open2() sess %x aName %*S anAttMask %x" */
        EDirOpen2,
        /** @SYMTraceFormatString "-RDir::Open2() r %d subs %x" */
        EDirOpen2Return,

		/** @SYMTraceFormatString "+RDir::Close() sess %x subs %x" */
        EDirClose,
        /** @SYMTraceFormatString "-RDir::Close()" */
        EDirCloseReturn,

		/** @SYMTraceFormatString "+RDir::Read1() sess %x subs %x" */
        EDirRead1,
        /** @SYMTraceFormatString "-RDir::Read1() r %d count %d" */
        EDirRead1Return,

		/** @SYMTraceFormatString "+RDir::Read2() sess %x subs %x status %x" */
        EDirRead2,
        /** @SYMTraceFormatString "-RDir::Read2()" */
        EDirRead2Return,

		/** @SYMTraceFormatString "+RDir::Read3() sess %x subs %x" */
        EDirRead3,
        /** @SYMTraceFormatString "-RDir::Read3() r %d" */
        EDirRead3Return,

		/** @SYMTraceFormatString "+RDir::Read4() sess %x subs %x status %x" */
        EDirRead4,
        /** @SYMTraceFormatString "-RDir::Read4()" */
        EDirRead4Return,


		/** @SYMTraceFormatString "+RFormat::Open1() sess %x aName %*S aFormatMode %x" */
        EFormat1Open,
        /** @SYMTraceFormatString "-RFormat::Open1() r %d subs %x aCount %d" */
        EFormatOpen1Return,

		/** @SYMTraceFormatString "+RFormat::Open2() sess %x aName %*S aFormatMode %x aInfo %x" */
        EFormat2Open,
        /** @SYMTraceFormatString "-RFormat::Open2() r %d subs %x aCount %d" */
        EFormatOpen2Return,

		/** @SYMTraceFormatString "+RFormat::Close() sess %x subs %x" */
        EFormatClose,
        /** @SYMTraceFormatString "-RFormat::Close()" */
        EFormatCloseReturn,

		/** @SYMTraceFormatString "+RFormat::Next1() sess %x subs %x" */
        EFormatNext1,
        /** @SYMTraceFormatString "-RFormat::Next1() r %d aStep %d" */
        EFormatNext1Return,

		/** @SYMTraceFormatString "+RFormat::Next2() sess %x subs %x status %x" */
        EFormatNext2,
        /** @SYMTraceFormatString "-RFormat::Next2()" */
        EFormatNext2Return,



		/** @SYMTraceFormatString "+RPlugin::Open() aPos %d" */
        EPluginOpen,
        /** @SYMTraceFormatString "-RPlugin::Open() r %d subs %x" */
        EPluginOpenReturn,

		/** @SYMTraceFormatString "+RPlugin::Close() sess %x subs %x" */
        EPluginClose,
        /** @SYMTraceFormatString "-RPlugin::Close() r %d" */
        EPluginCloseReturn,

		/** @SYMTraceFormatString "+RPlugin::DoRequest1() sess %x subs %x aReqNo %d status %x" */
        EPluginDoRequest1,
        /** @SYMTraceFormatString "-RPlugin::DoRequest1()" */
        EPluginDoRequest1Return,

		/** @SYMTraceFormatString "+RPlugin::DoRequest2() sess %x subs %x aReqNo %d status %x a1 %x" */
        EPluginDoRequest2,
        /** @SYMTraceFormatString "-RPlugin::DoRequest2()" */
        EPluginDoRequest2Return,

		/** @SYMTraceFormatString "+RPlugin::DoRequest3() sess %x subs %x aReqNo %d status %x a1 %x a2 %x" */
        EPluginDoRequest3,
        /** @SYMTraceFormatString "-RPlugin::DoRequest3()" */
        EPluginDoRequest3Return,

		/** @SYMTraceFormatString "+RPlugin::DoControl() sess %x subs %x aFunction %d" */
        EPluginDoControl1,
        /** @SYMTraceFormatString "-RPlugin::DoControl() r %d" */
        EPluginDoControl1Return,

		/** @SYMTraceFormatString "+RPlugin::DoControl2() sess %x subs %x aFunction %d a1 %x" */
        EPluginDoControl2,
        /** @SYMTraceFormatString "-RPlugin::DoControl2() r %d" */
        EPluginDoControl2Return,

		/** @SYMTraceFormatString "+RPlugin::DoControl3() sess %x subs %x aFunction %d a1 %x a2 %x" */
        EPluginDoControl3,
        /** @SYMTraceFormatString "-RPlugin::DoControl3() r %d" */
        EPluginDoControl3Return,

		/** @SYMTraceFormatString "+RPlugin::DoCancel() sess %x subs %x aReqMask %x" */
        EPluginDoCancel,
        /** @SYMTraceFormatString "-RPlugin::DoCancel()" */
        EPluginDoCancelReturn,


		/** @SYMTraceFormatString "+CFileMan::NewL1() sess %x" */
        ECFileManNewL1,
        /** @SYMTraceFormatString "-CFileMan::NewL1() CFileMan* %x" */
        ECFileManNewL1Return,

		/** @SYMTraceFormatString "+CFileMan::NewL2() sess %x anObserver %x" */
        ECFileManNewL2,
        /** @SYMTraceFormatString "-CFileMan::NewL2() CFileMan* %x" */
        ECFileManNewL2Return,

		/** @SYMTraceFormatString "+CFileMan::~CFileMan() this %x" */
        ECFileManDestructor,
        /** @SYMTraceFormatString "-CFileMan::~CFileMan()" */
        ECFileManDestructorReturn,

		/** @SYMTraceFormatString "+CFileMan::CurrentAction() this %x" */
        ECFileManCurrentAction,
        /** @SYMTraceFormatString "-CFileMan::CurrentAction() action %d" */
        ECFileManCurrentActionReturn,

		/** @SYMTraceFormatString "+CFileMan::GetCurrentTarget() this %x" */
        ECFileManGetCurrentTarget,
        /** @SYMTraceFormatString "-CFileMan::GetCurrentTarget() aTrgName %*S" */
        ECFileManGetCurrentTargetReturn,

		/** @SYMTraceFormatString "+CFileMan::GetCurrentSource() this %x" */
        ECFileManGetCurrentSource,
        /** @SYMTraceFormatString "-CFileMan::GetCurrentSource() aSrcName %*S" */
        ECFileManGetCurrentSourceReturn,

		/** @SYMTraceFormatString ".CFileMan::BytesTransferredByCopyStep() this %x BytesTransferred %d" */
        ECFileManBytesTransferredByCopyStep,

		/** @SYMTraceFormatString "+CFileMan::Attribs1() this %x aName %*S aSetMask %x aClearMask %x aTime %lx aSwitches %d, status %x" */
        ECFileManAttribs1,
        /** @SYMTraceFormatString "-CFileMan::Attribs1() r %d" */
        ECFileManAttribs1Return,

		/** @SYMTraceFormatString "+CFileMan::Attribs2() this %x aName %*S aSetMask %x aClearMask %x aTime %lx aSwitches %d" */
        ECFileManAttribs2,
        /** @SYMTraceFormatString "-CFileMan::Attribs2() r %d" */
        ECFileManAttribs2Return,

		/** @SYMTraceFormatString "+CFileMan::Copy1() this %x anOld %*S aNew %*S aSwitches %d status %x" */
        ECFileManCopy1,
        /** @SYMTraceFormatString "-CFileMan::Copy1() r %d" */
        ECFileManCopy1Return,

		/** @SYMTraceFormatString "+CFileMan::Copy2() this %x anOld %*S aNew %*S aSwitches %d" */
        ECFileManCopy2,
        /** @SYMTraceFormatString "-CFileMan::Copy2() r %d" */
        ECFileManCopy2Return,

		/** @SYMTraceFormatString "+CFileMan::Copy3() this %x anOldSubs %x aNew %*S aSwitches %d" */
        ECFileManCopy3,
        /** @SYMTraceFormatString "-CFileMan::Copy3() r %d" */
        ECFileManCopy3Return,

		/** @SYMTraceFormatString "+CFileMan::Copy4() this %x anOldSubs %x aNew %*S aSwitches %dstatus %x" */
        ECFileManCopy4,
        /** @SYMTraceFormatString "-CFileMan::Copy4() r %d" */
        ECFileManCopy4Return,
		
		/** @SYMTraceFormatString "+CFileMan::Delete1() this %x aName %*S aSwitches %d status %x" */
        ECFileManDelete1,
        /** @SYMTraceFormatString "-CFileMan::Delete1() r %d" */
        ECFileManDelete1Return,

		/** @SYMTraceFormatString "+CFileMan::Delete2() this %x aName %*S aSwitches %d" */
        ECFileManDelete2,
        /** @SYMTraceFormatString "-CFileMan::Delete2() r %d" */
        ECFileManDelete2Return,

		/** @SYMTraceFormatString "+CFileMan::Move1() this %x anOld %*S aNew %*S aSwitches %d status %x" */
        ECFileManMove1,
        /** @SYMTraceFormatString "-CFileMan::Move1() r %d" */
        ECFileManMove1Return,

		/** @SYMTraceFormatString "+CFileMan::Move2() this %x anOld %*S aNew %*S aSwitches %d" */
        ECFileManMove2,
        /** @SYMTraceFormatString "-CFileMan::Move2() r %d" */
        ECFileManMove2Return,

		/** @SYMTraceFormatString "+CFileMan::Rename1() this %x anOld %*S aNew %*S aSwitches %d status %x" */
        ECFileManRename1,
        /** @SYMTraceFormatString "-CFileMan::Rename1() r %d" */
        ECFileManRename1Return,

		/** @SYMTraceFormatString "+CFileMan::Rename2() this %x anOld %*S aNew %*S aSwitches %d" */
        ECFileManRename2,
        /** @SYMTraceFormatString "-CFileMan::Rename2() r %d" */
        ECFileManRename2Return,

		/** @SYMTraceFormatString "+CFileMan::RmDir1() this %x aDirName %*S status %x" */
        ECFileManRmDir1,
        /** @SYMTraceFormatString "-CFileMan::RmDir1() r %d" */
        ECFileManRmDir1Return,

		/** @SYMTraceFormatString "+CFileMan::RmDir2() this %x aDirName %*S" */
        ECFileManRmDir2,
        /** @SYMTraceFormatString "-CFileMan::RmDir2() r %d" */
        ECFileManRmDir2Return,





		/** @SYMTraceFormatString "+CDirScan::NewL() sess %x" */
        ECDirScanNewL,
        /** @SYMTraceFormatString "-CDirScan::NewL() CDirScan* %x" */
        ECDirScanNewLReturn,

		/** @SYMTraceFormatString "+CDirScan::NewLC() sess %x" */
        ECDirScanNewLC,
        /** @SYMTraceFormatString "-CDirScan::NewLC() CDirScan* %x" */
        ECDirScanNewLCReturn,

		/** @SYMTraceFormatString "+CDirScan::~CDirScan() this %x" */
        ECDirScanDestructor,
        /** @SYMTraceFormatString "-CDirScan::~CDirScan()" */
        ECDirScanDestructorReturn,

		/** @SYMTraceFormatString "+CDirScan::SetScanDataL() this %x aMatchName %*S anEntryAttMask %x anEntrySortKey %d aScanDir %d" */
        ECDirScanSetScanDataL,
        /** @SYMTraceFormatString "-CDirScan::SetScanDataL() r %d" */
        ECDirScanSetScanDataLReturn,

		/** @SYMTraceFormatString "+CDirScan::NextL() this %x" */
        ECDirScanNextL,
        /** @SYMTraceFormatString "-CDirScan::NextL() r %d DirEntries %d" */
        ECDirScanNextLReturn,

        /** @SYMTraceFormatString "-CDirScan Leave r %d" */
        ECDirScanLeave,






        /** @SYMTraceFormatString "Efsrv.dll Panic r %d" */
        EPanic,

        
		/** @SYMTraceFormatString "TimeStamp" */
        ETimeStamp,

        /**
        Provided to allow the following compile time assert.
        */
        EFormatIdHighWaterMark,
        };
    __ASSERT_COMPILE(EFormatIdHighWaterMark <= (UTF::KMaxFormatId + 1));



    } // end of namespace UTraceModuleEfsrv

	
#endif // UTRACEEFSRV_H
