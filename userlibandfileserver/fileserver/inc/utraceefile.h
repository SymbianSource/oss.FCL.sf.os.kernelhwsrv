
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
// File Server tracing
// 
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef UTRACEEFILE_H
#define UTRACEEFILE_H


#include <f32tracedef.h>

/**
This defines trace attributes to be used by file-system tracing

*/
namespace UTraceModuleFileSys
    {

	/** 
	@SYMTraceFormatCategory EF32TraceUidFileSys 
    @SYMTraceFormatId
    */
    enum TFormatId
        {
		EReserved = UTF::KInitialClientFormat-1,

        /** @SYMTraceFormatString "+CFileSystem::CFileSystem()" */
		ECFileSystemConstructor,
        /** @SYMTraceFormatString "-CFileSystem::CFileSystem()" */
        ECFileSystemConstructorReturn,

        /** @SYMTraceFormatString "+~CFileSystem::CFileSystem()" */
		ECFileSystemDestructor,
        /** @SYMTraceFormatString "-~CFileSystem::CFileSystem()" */
        ECFileSystemDestructorReturn,

        /** @SYMTraceFormatString "+CFileSystem::New() handle %x" */
		ECFileSystemNew,
        /** @SYMTraceFormatString "-CFileSystem::New() fileSystem %x" */
        ECFileSystemNewRet,

        /** @SYMTraceFormatString "+CFileSystem::Remove() this %x" */
		ECFileSystemRemove,
        /** @SYMTraceFormatString "-CFileSystem::Remove()" */
        ECFileSystemRemoveRet,

        /** @SYMTraceFormatString "CFileSystem::IsExtensionSupported() this %x r %d" */
		ECFileSystemIsExtensionSupported,

		/** @SYMTraceFormatString "+CFileSystem::Install() this %x" */
		ECFileSystemInstall,
        /** @SYMTraceFormatString "-CFileSystem::Install() r %d Name %*S" */
        ECFileSystemInstallRet,

		/** @SYMTraceFormatString "+CFileSystem::NewMountL() this %x drive %d" */
		ECFileSystemNewMountL,
        /** @SYMTraceFormatString "-CFileSystem::NewMountL() r %d CMountCB %x" */
        ECFileSystemNewMountLRet,

		/** @SYMTraceFormatString "+CFileSystem::NewFileL() this %x drive %d" */
		ECFileSystemNewFileL,
        /** @SYMTraceFormatString "-CFileSystem::NewFileL() r %d CFileCB %x" */
        ECFileSystemNewFileLRet,

		/** @SYMTraceFormatString "+CFileSystem::NewDirL() this %x drive %d" */
		ECFileSystemNewDirL,
        /** @SYMTraceFormatString "-CFileSystem::NewDirL() r %d CDirCB %x" */
        ECFileSystemNewDirLRet,

		/** @SYMTraceFormatString "+CFileSystem::NewFormatL() this %x drive %d" */
		ECFileSystemNewFormatL,
        /** @SYMTraceFormatString "-CFileSystem::NewFormatL() r %d CFormatCB %x" */
        ECFileSystemNewFormatLRet,

		/** @SYMTraceFormatString "+CFileSystem::DriveInfo() this %x aDriveNumber %d" */
		ECFileSystemDriveInfo,
        /** @SYMTraceFormatString "-CFileSystem::DriveInfo() type %d driveAtt %x mediaAtt %x" */
        ECFileSystemDriveInfoRet,


        
		/** @SYMTraceFormatString "+CMountCB::ReMount() drive %d" */
		ECMountCBReMount,
        /** @SYMTraceFormatString "-CMountCB::ReMount() success %d" */
        ECMountCBReMountRet,

		/** @SYMTraceFormatString "+CMountCB::Dismounted() drive %d" */
		ECMountCBDismounted,
        /** @SYMTraceFormatString "-CMountCB::Dismounted()" */
        ECMountCBDismountedRet,


		/** @SYMTraceFormatString "+CMountCB::VolumeL() drive %d" */
		ECMountCBVolumeL,
        /** @SYMTraceFormatString "-CMountCB::VolumeL() r %d iUniqueID %x iSize %ld iFree %ld iFileCacheFlags %x" */
        ECMountCBVolumeLRet,

		/** @SYMTraceFormatString "+CMountCB::SetVolumeL() drive %d aName %*S" */
		ECMountCBSetVolumeL,
        /** @SYMTraceFormatString "-CMountCB::SetVolumeL() r %d" */
        ECMountCBSetVolumeLRet,

		/** @SYMTraceFormatString "+CMountCB::MkDirL() drive %d aName %*S" */
		ECMountCBMkDirL,
        /** @SYMTraceFormatString "-CMountCB::MkDirL() r %d" */
        ECMountCBMkDirLRet,

		/** @SYMTraceFormatString "+CMountCB::RmDirL() drive %d aName %*S" */
		ECMountCBRmDirL,
        /** @SYMTraceFormatString "-CMountCB::RmDirL() r %d" */
        ECMountCBRmDirLRet,

		/** @SYMTraceFormatString "+CMountCB::DeleteL() drive %d aName %*S" */
		ECMountCBDeleteL,
        /** @SYMTraceFormatString "-CMountCB::DeleteL() r %d" */
        ECMountCBDeleteLRet,

		/** @SYMTraceFormatString "+CMountCB::RenameL() drive %d anOldName %*S anNewName *S" */
		ECMountCBRenameL,
        /** @SYMTraceFormatString "-CMountCB::RenameL() r %d" */
        ECMountCBRenameLRet,

		/** @SYMTraceFormatString "+CMountCB::ReplaceL() drive %d anOldName %*S anNewName *S" */
		ECMountCBReplaceL,
        /** @SYMTraceFormatString "-CMountCB::ReplaceL() r %d" */
        ECMountCBReplaceLRet,

		/** @SYMTraceFormatString "+CMountCB::EntryL() drive %d aName %*S" */
		ECMountCBEntryL,
        /** @SYMTraceFormatString "-CMountCB::EntryL() r %d att %x modified %lx size %d" */
        ECMountCBEntryLRet,

		/** @SYMTraceFormatString "+CMountCB::SetEntryL() drive %d aName %*S aTime %lx aSetAttMask %x aClearAttMask %x" */
		ECMountCBSetEntryL,
        /** @SYMTraceFormatString "-CMountCB::SetEntryL() r %d" */
        ECMountCBSetEntryLRet,

        /** @SYMTraceFormatString "+CMountCB::FileOpenL() drive %d aName %*S aMode %x anOpen %d aFile %x" */
		ECMountCBFileOpenL,
        /** @SYMTraceFormatString "-CMountCB::FileOpenL() r %d" */
        ECMountCBFileOpenLRet,

        /** @SYMTraceFormatString "+CMountCB::DirOpenL() drive %d aName %*S aDir %x" */
		ECMountCBDirOpenL,
        /** @SYMTraceFormatString "-CMountCB::DirOpenL() r %d" */
        ECMountCBDirOpenLRet,

		/** @SYMTraceFormatString "+CMountCB::RawReadL() drive %d aPos %ld aLength %d aTrg %x anOffset %x threadId %x" */
		ECMountCBRawReadL,
        /** @SYMTraceFormatString "-CMountCB::RawReadL() r %d" */
        ECMountCBRawReadLRet,

		/** @SYMTraceFormatString "+CMountCB::RawWriteL() drive %d aPos %ld aLength %d aTrg %x anOffset %x threadId %x" */
		ECMountCBRawWriteL,
        /** @SYMTraceFormatString "-CMountCB::RawWriteL() r %d" */
        ECMountCBRawWriteLRet,

		/** @SYMTraceFormatString "+CMountCB::GetShortNameL() drive %d aLongName %*S" */
		ECMountCBGetShortNameL,
        /** @SYMTraceFormatString "-CMountCB::GetShortNameL() r %d aShortName %*S" */
        ECMountCBGetShortNameLRet,

		/** @SYMTraceFormatString "+CMountCB::GetLongNameL() drive %d aShortName %*S" */
		ECMountCBGetLongNameL,
        /** @SYMTraceFormatString "-CMountCB::GetLongNameL() r %d aLongName %*S" */
        ECMountCBGetLongNameLRet,

		/** @SYMTraceFormatString "+CMountCB::ReadFileSectionL() drive %d aName %*S aPos %ld aTrg %x aLength %d threadId %x" */
		ECMountCBReadFileSectionL,
        /** @SYMTraceFormatString "-CMountCB::ReadFileSectionL() r %d" */
        ECMountCBReadFileSectionLRet,

		/** @SYMTraceFormatString "+CMountCB::CheckDisk1() drive %d" */
		ECMountCBCheckDisk1,
        /** @SYMTraceFormatString "-CMountCB::CheckDisk1() r %d" */
        ECMountCBCheckDisk1Ret,

		/** @SYMTraceFormatString "+CMountCB::CheckDisk2() drive %d aOperation %d aParam1 %x aParam2 %x" */
		ECMountCBCheckDisk2,
        /** @SYMTraceFormatString "-CMountCB::CheckDisk2() r %d" */
        ECMountCBCheckDisk2Ret,

		/** @SYMTraceFormatString "+CMountCB::ScanDrive1() drive %d" */
		ECMountCBScanDrive1,
        /** @SYMTraceFormatString "-CMountCB::ScanDrive1() r %d" */
        ECMountCBScanDrive1Ret,

		/** @SYMTraceFormatString "+CMountCB::ScanDrive2() drive %d aOperation %d aParam1 %x aParam2 %x" */
		ECMountCBScanDrive2,
        /** @SYMTraceFormatString "-CMountCB::ScanDrive2() r %d" */
        ECMountCBScanDrive2Ret,

		/** @SYMTraceFormatString "+CMountCB::ControlIO() drive %d aCommand %d aParam1 %x aParam2 %x threadId %x" */
		ECMountCBControlIO,
        /** @SYMTraceFormatString "-CMountCB::ControlIO() r %d" */
        ECMountCBControlIORet,

		/** @SYMTraceFormatString "+CMountCB::Lock() drive %d aStore %d" */
		ECMountCBLock,
        /** @SYMTraceFormatString "-CMountCB::Lock() r %d" */
        ECMountCBLockRet,

		/** @SYMTraceFormatString "+CMountCB::Unlock() drive %d aStore %d" */
		ECMountCBUnlock,
        /** @SYMTraceFormatString "-CMountCB::Unlock() r %d" */
        ECMountCBUnlockRet,

		/** @SYMTraceFormatString "+CMountCB::ClearPassword() drive %d" */
		ECMountCBClearPassword,
        /** @SYMTraceFormatString "-CMountCB::ClearPassword() r %d" */
        ECMountCBClearPasswordRet,

		/** @SYMTraceFormatString "+CMountCB::ForceRemountDrive() drive %d aMountInfo %x aMountInfoMessageHandle %x aFlags %x" */
		ECMountCBForceRemountDrive,
        /** @SYMTraceFormatString "-CMountCB::ForceRemountDrive() r %d" */
        ECMountCBForceRemountDriveRet,

		/** @SYMTraceFormatString "+CMountCB::FinaliseMount1() drive %d" */
		ECMountCBFinaliseMount1,
        /** @SYMTraceFormatString "-CMountCB::FinaliseMount1() r %d" */
        ECMountCBFinaliseMount1Ret,

		/** @SYMTraceFormatString "+CMountCB::FinaliseMount2() drive %d" */
		ECMountCBFinaliseMount2,
        /** @SYMTraceFormatString "-CMountCB::FinaliseMount2() r %d" */
        ECMountCBFinaliseMount2Ret,

		/** @SYMTraceFormatString "+CMountCB::MountControl() drive %d aLevel %d aOption %x aParam %x" */
		ECMountCBMountControl,
        /** @SYMTraceFormatString "-CMountCB::MountControl() r %d" */
        ECMountCBMountControlRet,

		/** @SYMTraceFormatString "+CMountCB::ESQ_RequestFreeSpace() drive %d" */
		ECMountCBFreeSpace,
        /** @SYMTraceFormatString "-CMountCB::ESQ_RequestFreeSpace() r %d FreeSpace %ld" */
        ECMountCBFreeSpaceRet,

		/** @SYMTraceFormatString "+CMountCB::ESQ_GetCurrentFreeSpace() drive %d" */
		ECMountCBCurrentFreeSpace,
        /** @SYMTraceFormatString "-CMountCB::ESQ_GetCurrentFreeSpace() r %d FreeSpace %ld" */
        ECMountCBCurrentFreeSpaceRet,

		/** @SYMTraceFormatString "+CMountCB::ESQ_MountedVolumeSize() drive %d" */
		ECMountCBVolumeSize,
        /** @SYMTraceFormatString "-CMountCB::ESQ_MountedVolumeSize() r %d size %ld" */
        ECMountCBVolumeSizeRet,

		/** @SYMTraceFormatString "+CMountCB::ErasePassword() drive %d" */
		ECMountCBErasePassword,
        /** @SYMTraceFormatString "-CMountCB::ErasePassword() r %d" */
        ECMountCBErasePasswordRet,

		/** @SYMTraceFormatString "+CMountCB::GetInterface() drive %d aInterfaceId %d aInput %x" */
		ECMountCBGetInterface,
        /** @SYMTraceFormatString "-CMountCB::GetInterface() r %d aInterface %x" */
        ECMountCBGetInterfaceRet,

		/** @SYMTraceFormatString "+CFileCB::RenameL() this %x aNewName %*S" */
		ECFileCBRenameL,
        /** @SYMTraceFormatString "-CFileCB::RenameL() r %d" */
        ECFileCBRenameLRet,

		/** @SYMTraceFormatString "+CFileCB::ReadL() this %x aPos %ld aLength %d aDes %x threadId %x aOffset %x" */
		ECFileCBReadL,
        /** @SYMTraceFormatString "-CFileCB::ReadL() r %d" */
        ECFileCBReadLRet,

		/** @SYMTraceFormatString "+CFileCB::WriteL() this %x aPos %ld aLength %d aDes %x threadId %x aOffset %x" */
		ECFileCBWriteL,
        /** @SYMTraceFormatString "-CFileCB::WriteL() r %d" */
        ECFileCBWriteLRet,

		/** @SYMTraceFormatString "+CFileCB::SetSizeL() this %x aSize %ld" */
		ECFileCBSetSizeL,
        /** @SYMTraceFormatString "-CFileCB::SetSizeL() r %d" */
        ECFileCBSetSizeLRet,

		/** @SYMTraceFormatString "+CFileCB::SetEntryL() this %x aTime %lx aSetAttMask %x aClearAttMask %x" */
		ECFileCBSetEntryL,
        /** @SYMTraceFormatString "-CFileCB::SetEntryL() r %d" */
        ECFileCBSetEntryLRet,

		/** @SYMTraceFormatString "+CFileCB::FlushDataL() this %x" */
		ECFileCBFlushDataL,
        /** @SYMTraceFormatString "-CFileCB::FlushDataL() r %d" */
        ECFileCBFlushDataLRet,

		/** @SYMTraceFormatString "+CFileCB::GetInterface() aInterfaceId %d aInput %x" */
		ECFileCBGetInterface,
        /** @SYMTraceFormatString "-CFileCB::GetInterface() r %d aInterface %x" */
        ECFileCBGetInterfaceRet,


		/** @SYMTraceFormatString "+CDirCB::ReadL() this %x" */
		ECDirCBReadL,
        /** @SYMTraceFormatString "-CDirCB::ReadL() r %d att %x modified %lx size %d" */
        ECDirCBReadLRet,

		/** @SYMTraceFormatString "+CDirCB::StoreLongEntryNameL() this %x" */
		ECDirCBStoreLongEntryNameL,
        /** @SYMTraceFormatString "-CDirCB::StoreLongEntryNameL() r %d" */
        ECDirCBStoreLongEntryNameLRet,


		/** @SYMTraceFormatString "+CFormatCB::DoFormatStepL() this %x" */
		ECFormatCBDoFormatStepL,
        /** @SYMTraceFormatString "-CFormatCB::DoFormatStepL() r %d  iCurrentStep %d" */
        ECFormatCBDoFormatStepLRet,





        /**
        Provided to allow the following compile time assert.
        */
        EFormatIdHighWaterMark,
        };
    __ASSERT_COMPILE(EFormatIdHighWaterMark <= (UTF::KMaxFormatId + 1));
	}

namespace UTraceModuleProxyDrive
    {
	/** 
	@SYMTraceFormatCategory EF32TraceUidProxyDrive 
    @SYMTraceFormatId
    */
    enum TFormatId
        {
		EReserved = UTF::KInitialClientFormat-1,

		/** @SYMTraceFormatString "+CLocalProxyDrive::New() aMount %x drive %d" */
		ECLocalProxyDriveNew,
        /** @SYMTraceFormatString "-CLocalProxyDrive::New() proxyDrive %x" */
        ECLocalProxyDriveNewRet,


		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Initialise() this %x" */
		ECBaseExtProxyDriveInitialise,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Initialise() r %d" */
        ECBaseExtProxyDriveInitialiseRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Initialise() this %x" */
		ECLocalProxyDriveInitialise,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Initialise() r %d" */
        ECLocalProxyDriveInitialiseRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::ControlIO() this %x" */
		ECBaseExtProxyDriveControlIO,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::ControlIO() r %d" */
        ECBaseExtProxyDriveControlIORet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::ControlIO() this %x" */
		ECLocalProxyDriveControlIO,
        /** @SYMTraceFormatString "-CLocalProxyDrive::ControlIO() r %d" */
        ECLocalProxyDriveControlIORet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Read1() this %x aPos %ld aLength %d aTrg %x threadId %x aOffset %x aFlags %x" */
		ECBaseExtProxyDriveRead1,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Read1() r %d" */
        ECBaseExtProxyDriveRead1Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Read1() this %x aPos %ld aLength %d aTrg %x threadId %x aOffset %x aFlags %x" */
		ECLocalProxyDriveRead1,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Read1() r %d" */
        ECLocalProxyDriveRead1Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Read2() this %x aPos %ld aLength %d aTrg %x threadId %x aOffset %x" */
		ECBaseExtProxyDriveRead2,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Read2() r %d" */
        ECBaseExtProxyDriveRead2Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Read2() this %x aPos %ld aLength %d aTrg %x threadId %x aOffset %x" */
		ECLocalProxyDriveRead2,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Read2() r %d" */
        ECLocalProxyDriveRead2Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Read3() this %x aPos %ld aLength %d aTrg %x" */
		ECBaseExtProxyDriveRead3,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Read3() r %d" */
        ECBaseExtProxyDriveRead3Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Read3() this %x aPos %ld aLength %d aTrg %x" */
		ECLocalProxyDriveRead3,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Read3() r %d" */
        ECLocalProxyDriveRead3Ret,


		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Write1() this %x aPos %ld aLength %d aSrc %x threadId %x aOffset %x aFlags %x" */
		ECBaseExtProxyDriveWrite1,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Write1() r %d" */
        ECBaseExtProxyDriveWrite1Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Write1() this %x aPos %ld aLength %d aSrc %x threadId %x aOffset %x aFlags %x" */
		ECLocalProxyDriveWrite1,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Write1() r %d" */
        ECLocalProxyDriveWrite1Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Write2() this %x aPos %ld aLength %d aSrc %x threadId %x aOffset %x" */
		ECBaseExtProxyDriveWrite2,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Write2() r %d" */
        ECBaseExtProxyDriveWrite2Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Write2() this %x aPos %ld aLength %d aSrc %x threadId %x aOffset %x" */
		ECLocalProxyDriveWrite2,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Write2() r %d" */
        ECLocalProxyDriveWrite2Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Write3() this %x aPos %ld aLength %d aSrc %x" */
		ECBaseExtProxyDriveWrite3,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Write3() r %d" */
        ECBaseExtProxyDriveWrite3Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Write3() this %x aPos %ld aLength %d aSrc %x" */
		ECLocalProxyDriveWrite3,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Write3() r %d" */
        ECLocalProxyDriveWrite3Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Dismounted() this %x" */
		ECBaseExtProxyDriveDismounted,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Dismounted() r %d" */
        ECBaseExtProxyDriveDismountedRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Dismounted() this %x" */
		ECLocalProxyDriveDismounted,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Dismounted() r %d" */
        ECLocalProxyDriveDismountedRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Enlarge() this %x aLength %d" */
		ECBaseExtProxyDriveEnlarge,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Enlarge() r %d" */
        ECBaseExtProxyDriveEnlargeRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Enlarge() this %x aLength %d" */
		ECLocalProxyDriveEnlarge,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Enlarge() r %d" */
        ECLocalProxyDriveEnlargeRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::ReduceSize() this %x aPos %d aLength %d" */
		ECBaseExtProxyDriveReduceSize,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::ReduceSize() r %d" */
        ECBaseExtProxyDriveReduceSizeRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::ReduceSize() this %x aPos %d aLength %d" */
		ECLocalProxyDriveReduceSize,
        /** @SYMTraceFormatString "-CLocalProxyDrive::ReduceSize() r %d" */
        ECLocalProxyDriveReduceSizeRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Caps() this %x" */
		ECBaseExtProxyDriveCaps,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Caps() r %d" */
        ECBaseExtProxyDriveCapsRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Caps() this %x" */
		ECLocalProxyDriveCaps,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Caps() r %d" */
        ECLocalProxyDriveCapsRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Format1() this %x" */
		ECBaseExtProxyDriveFormat1,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Format1() r %d iFormatIsCurrent %d i512ByteSectorsFormatted %d iMaxBytesPerFormat %d" */
        ECBaseExtProxyDriveFormat1Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Format1() this %x" */
		ECLocalProxyDriveFormat1,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Format1() r %d iFormatIsCurrent %d i512ByteSectorsFormatted %d iMaxBytesPerFormat %d" */
        ECLocalProxyDriveFormat1Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Format2() this %x aPos %ld aLength %d" */
		ECBaseExtProxyDriveFormat2,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Format2() r %d" */
        ECBaseExtProxyDriveFormat2Ret,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Format2() this %x aPos %ld aLength %d" */
		ECLocalProxyDriveFormat2,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Format2() r %d" */
        ECLocalProxyDriveFormat2Ret,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::SetMountInfo() this %x" */
		ECBaseExtProxyDriveSetMountInfo,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::SetMountInfo() r %d" */
        ECBaseExtProxyDriveSetMountInfoRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::SetMountInfo() this %x" */
		ECLocalProxyDriveSetMountInfo,
        /** @SYMTraceFormatString "-CLocalProxyDrive::SetMountInfo() r %d" */
        ECLocalProxyDriveSetMountInfoRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::ForceRemount() this %x" */
		ECBaseExtProxyDriveForceRemount,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::ForceRemount() r %d" */
        ECBaseExtProxyDriveForceRemountRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::ForceRemount() this %x" */
		ECLocalProxyDriveForceRemount,
        /** @SYMTraceFormatString "-CLocalProxyDrive::ForceRemount() r %d" */
        ECLocalProxyDriveForceRemountRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Unlock() this %x aPassword %d" */
		ECBaseExtProxyDriveUnlock,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Unlock() r %d" */
        ECBaseExtProxyDriveUnlockRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Unlock() this %x aPassword %d" */
		ECLocalProxyDriveUnlock,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Unlock() r %d" */
        ECLocalProxyDriveUnlockRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Lock() this %x aPassword %d" */
		ECBaseExtProxyDriveLock,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Lock() r %d" */
        ECBaseExtProxyDriveLockRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Lock() this %x aPassword %d" */
		ECLocalProxyDriveLock,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Lock() r %d" */
        ECLocalProxyDriveLockRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::Clear() this %x" */
		ECBaseExtProxyDriveClear,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::Clear() r %d" */
        ECBaseExtProxyDriveClearRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::Clear() this %x" */
		ECLocalProxyDriveClear,
        /** @SYMTraceFormatString "-CLocalProxyDrive::Clear() r %d" */
        ECLocalProxyDriveClearRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::ErasePassword() this %x" */
		ECBaseExtProxyDriveErasePassword,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::ErasePassword() r %d" */
        ECBaseExtProxyDriveErasePasswordRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::ErasePassword() this %x" */
		ECLocalProxyDriveErasePassword,
        /** @SYMTraceFormatString "-CLocalProxyDrive::ErasePassword() r %d" */
        ECLocalProxyDriveErasePasswordRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::DeleteNotify() this %x aPos %ld aLength %d" */
		ECBaseExtProxyDriveDeleteNotify,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::DeleteNotify() r %d" */
        ECBaseExtProxyDriveDeleteNotifyRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::DeleteNotify() this %x aPos %ld aLength %d" */
		ECLocalProxyDriveDeleteNotify,
        /** @SYMTraceFormatString "-CLocalProxyDrive::DeleteNotify() r %d" */
        ECLocalProxyDriveDeleteNotifyRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::GetLastErrorInfo() this %x" */
		ECBaseExtProxyDriveGetLastErrorInfo,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::GetLastErrorInfo() r %d" */
        ECBaseExtProxyDriveGetLastErrorInfoRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::GetLastErrorInfo() this %x" */
		ECLocalProxyDriveGetLastErrorInfo,
        /** @SYMTraceFormatString "-CLocalProxyDrive::GetLastErrorInfo() r %d" */
        ECLocalProxyDriveGetLastErrorInfoRet,

		/** @SYMTraceFormatString "+CBaseExtProxyDrive::GetInterface() this %x aInterfaceId %d aInput %x" */
		ECBaseExtProxyDriveGetInterface,
        /** @SYMTraceFormatString "-CBaseExtProxyDrive::GetInterface() r %d aInterface %x" */
        ECBaseExtProxyDriveGetInterfaceRet,

		/** @SYMTraceFormatString "+CLocalProxyDrive::GetInterface() this %x aInterfaceId %d aInput %x" */
		ECLocalProxyDriveGetInterface,
        /** @SYMTraceFormatString "-CLocalProxyDrive::GetInterface() r %d aInterface %x" */
        ECLocalProxyDriveGetInterfaceRet,




        /**
        Provided to allow the following compile time assert.
        */
        EFormatIdHighWaterMark,
        };
    __ASSERT_COMPILE(EFormatIdHighWaterMark <= (UTF::KMaxFormatId + 1));



    } // end of namespace UTraceModuleFileSys

	
#endif // UTRACEEFILE_H
