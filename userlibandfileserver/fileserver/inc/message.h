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
//

/**
 @file f32\inc\message.h
 @internalTechnology
*/

#if !defined(__MESSAGE_H__)
#define __MESSAGE_H__
#if !defined(__E32STD_H__)
#include <e32std.h>
#endif

//#include <f32plugin.h>
#include <e32capability.h>

//
// Structs used to reduce the number of parameters in messages
//

//
class TOpenFileListPos
/**
@internalTechnology
@released
*/
	{
public:
	TOpenFileListPos() {}
	TOpenFileListPos(TInt aSession,TInt iListPos) : iSession(aSession), iEntryListPos(iListPos) {}
public:	
	TInt iSession;
	TInt iEntryListPos;
	};

//
//	Function capabilities
//

_LIT_SECURITY_POLICY_C1(KCapAllFiles, ECapabilityAllFiles);
_LIT_SECURITY_POLICY_C1(KCapDiskAdmin, ECapabilityDiskAdmin);
_LIT_SECURITY_POLICY_C1(KCapTCB, ECapabilityTCB);

//KCapFsPri - Private directories
//KCapFsSys - System directory
//KCapFsRO - Resource directory

#define KCapFsAddFileSystem			KCapDiskAdmin
#define KCapFsRemoveFileSystem		KCapDiskAdmin
#define KCapFsMountFileSystem		KCapDiskAdmin
#define KCapFsDismountFileSystem	KCapDiskAdmin
#define KCapFsAddExtension			KCapDiskAdmin
#define KCapFsRemoveExtension		KCapDiskAdmin
#define KCapFsMountExtension		KCapDiskAdmin
#define KCapFsDismountExtension 	KCapDiskAdmin
#define KCapFsNotifyChange			KCapAllFiles
#define KCapFsNotifyChangeCancel	KCapAllFiles
#define KCapFsSetVolume				KCapDiskAdmin
#define KCapFsSetSubst				KCapDiskAdmin
#define KCapFsSysSetSubst			KCapTCB
#define KCapFsPriSetSubst			KCapAllFiles
#define KCapFsROSetSubst			KCapTCB
#define KCapFsSysRealName			KCapAllFiles
#define KCapFsPriRealName			KCapAllFiles
#define KCapFsSysSetSessionPath		KCapAllFiles
#define KCapFsPriSetSessionPath		KCapAllFiles
#define KCapFsSysMkDir				KCapTCB
#define KCapFsROMkDir				KCapTCB
#define KCapFsPriMkDir				KCapAllFiles
#define KCapFsSysRmDir				KCapTCB
#define KCapFsRORmDir				KCapTCB
#define KCapFsPriRmDir				KCapAllFiles
#define KCapFsSysDelete				KCapTCB
#define KCapFsRODelete				KCapTCB
#define KCapFsPriDelete				KCapAllFiles
#define KCapFsSysRename				KCapTCB
#define KCapFsRORename				KCapTCB
#define KCapFsPriRename				KCapAllFiles
#define KCapFsSysReplace			KCapTCB
#define KCapFsROReplace				KCapTCB
#define KCapFsPriReplace			KCapAllFiles
#define KCapFsEntry					KCapAllFiles
#define KCapFsSysSetEntry			KCapTCB
#define KCapFsROSetEntry			KCapTCB
#define KCapFsPriSetEntry			KCapAllFiles
#define KCapFsSetDriveName			KCapDiskAdmin
#define KCapFsPriFileOpen			KCapAllFiles
#define KCapFsSysFileOpenWr			KCapTCB
#define KCapFsROFileOpenWr			KCapTCB
#define KCapFsSysFileOpenRd			KCapAllFiles
#define KCapFsSysFileCreate			KCapTCB
#define KCapFsROFileCreate			KCapTCB
#define KCapFsPriFileCreate			KCapAllFiles
#define KCapFsSysFileReplace		KCapTCB
#define KCapFsROFileReplace			KCapTCB
#define KCapFsPriFileReplace		KCapAllFiles
#define KCapFsSysFileTemp			KCapTCB
#define KCapFsROFileTemp			KCapTCB
#define KCapFsPriFileTemp			KCapAllFiles
#define KCapFsSysFileRename			KCapTCB
#define KCapFsROFileRename			KCapTCB
#define KCapFsPriFileRename			KCapAllFiles
#define KCapFsSysNotificationAddFilter  KCapTCB
#define KCapFsRONotificationAddFilter   KCapTCB
#define KCapFsPriNotificationAddFilter  KCapAllFiles
#define KCapFsSysIsFileOpen			KCapAllFiles
#define KCapFsPriIsFileOpen			KCapAllFiles
#define KCapFsFileChangeMode		KCapDiskAdmin
#define KCapFsSysDirOpen			KCapAllFiles
#define KCapFsPriDirOpen			KCapAllFiles
#define KCapFsFormatOpen			KCapDiskAdmin
#define KCapFsFormatNext			KCapDiskAdmin
#define KCapFsRawDiskOpen			KCapTCB
#define KCapFsRawDiskRead			KCapTCB
#define KCapFsRawDiskWrite			KCapTCB
#define KCapFsCheckDisk				KCapDiskAdmin
#define KCapFsSysGetShortName		KCapAllFiles
#define KCapFsPriGetShortName		KCapAllFiles
#define KCapFsSysGetLongName		KCapAllFiles
#define KCapFsPriGetLongName		KCapAllFiles
#define KCapFsSysIsFileInRom		KCapAllFiles
#define KCapFsPriIsFileInRom		KCapAllFiles
#define KCapFsSysReadFileSection	KCapAllFiles
#define KCapFsPriReadFileSection	KCapAllFiles
#define KCapFsNotifyChangeEx		KCapAllFiles
#define KCapFsNotifyChangeCancelEx	KCapAllFiles
#define KCapFsScanDrive				KCapDiskAdmin
#define KCapFsLockDrive				KCapDiskAdmin
#define KCapFsUnlockDrive			KCapDiskAdmin
#define KCapFsClearPassword			KCapDiskAdmin
#define KCapFsFinaliseDrive			KCapDiskAdmin
#define KCapFsErasePassword			KCapDiskAdmin
#define KCapFsPlugin				KCapDiskAdmin
#define KCapFsAddCompositeMount		KCapDiskAdmin
#define KCapFsSetSystemDrive		KCapTCB
#define KCapFsAddProxyDrive			KCapDiskAdmin
#define KCapFsRemoveProxyDrive		KCapDiskAdmin
#define KCapFsMountProxyDrive		KCapDiskAdmin
#define KCapFsDismountProxyDrive	KCapDiskAdmin


#endif
