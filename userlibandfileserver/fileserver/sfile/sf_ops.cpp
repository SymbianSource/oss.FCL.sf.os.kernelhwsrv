// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_ops.cpp
// 
//

#include "sf_std.h"



TBool TOperation::IsOpenSubSess() const
//
//	If so need to allocate for both open and close as close sub session must happen
//
	{
	switch(iFunction)
		{
		case EFsDirOpen:
		case EFsFileDuplicate:
		case EFsFileOpen:
		case EFsFileCreate:
		case EFsFileReplace:
		case EFsFileTemp:
		case EFsFormatOpen:
		case EFsRawDiskOpen:
		case EFsPluginOpen:
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
		case EFsNotificationOpen:
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
			return ETrue;
		default:
			return EFalse;
		}	
	}

TBool TOperation::IsCloseSubSess() const
//
//	If so need to close sub session 
//
	{
	switch (iFunction)
		{
		case EFsFileSubClose:
		case EFsFormatSubClose:
		case EFsDirSubClose:
		case EFsRawSubClose:
		case EFsPluginSubClose:
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION			
		case EFsNotificationSubClose:
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
			return ETrue;
		default:
			return EFalse;
		}
	}


TBool TOperation::IsChangeNotify() const
//
//
//
	{
	switch(iFunction)
		{
		case EFsFileCreate:
		case EFsFileReplace:
		case EFsDelete:
		case EFsReplace:
		case EFsFileRename:
		case EFsMkDir:
		case EFsRmDir:
		case EFsRename:					
		case EFsSetVolume:				
		case EFsFileSet:
		case EFsFileSetAtt:
		case EFsFileSetModified:
		case EFsFileSetSize:
		case EFsSetEntry:
		case EFsFileWrite:
		case EFsFileWriteDirty:
		case EFsRawDiskWrite:			
		case EFsFileTemp:		
		case EFsSetDriveName:
		case EFsLockDrive:
		case EFsUnlockDrive:
			return ETrue;
		default:
			return EFalse;
		}
	}

TBool TOperation::IsDiskSpaceNotify() const
//
//
//
	{
	switch(iFunction)
		{
		case EFsSetVolume:
		case EFsMkDir:
		case EFsRmDir:
		case EFsDelete:
		case EFsRename:
		case EFsReplace:
		case EFsFileCreate:
		case EFsFileReplace:
		case EFsFileTemp:
		case EFsFileWrite:
		case EFsFileWriteDirty:
		case EFsFileFlush:	
		case EFsFileSetSize:
		case EFsFileRename:
		case EFsReserveDriveSpace:
		case EFsFileSubClose:
			return ETrue;
		default: 
			return EFalse;
		}
	}

TBool TOperation::IsWrite() const
//
// return true if operation may cause write to media
//
	{
	switch(iFunction)
		{
		case EFsSetVolume:
		case EFsMkDir:
		case EFsRmDir:
		case EFsDelete:
		case EFsRename:
		case EFsReplace:
		case EFsSetEntry:
		case EFsFileCreate:
		case EFsFileReplace:
		case EFsFileTemp:
		case EFsFileWrite:
		case EFsFileWriteDirty:
		case EFsFileSetSize:
		case EFsFileSetAtt:
		case EFsFileSetModified:
		case EFsFileSet:
		case EFsFileRename:
		case EFsFormatOpen:
		case EFsFormatNext:
		case EFsRawDiskWrite:
		case EFsScanDrive:
		case EFsLockDrive:
		case EFsUnlockDrive:
			return ETrue;
		default:
			return EFalse;
		}
	}

TUint TOperation::NotifyType() const
//
//	Convert operation that caused the notification to a 
//	value corresponding to the correct TNotifyType enum  
//
	{
	switch (iFunction)
		{
	case EFsFileCreate:
	case EFsFileReplace:
	case EFsDelete:
	case EFsReplace:
	case EFsFileRename:
		return(ENotifyFile|ENotifyEntry);	
	case EFsMkDir:
	case EFsRmDir:
		return(ENotifyDir|ENotifyEntry);	
	case EFsRename:					
		return(ENotifyDir|ENotifyFile|ENotifyEntry);					
	case EFsSetVolume:					
		return(ENotifyDisk|ENotifyEntry);	
	case EFsFileSet:
	case EFsFileSetAtt:
	case EFsFileSetModified:
	case EFsFileSetSize:
	case EFsSetEntry:
		return(ENotifyAttributes);	
	case EFsFileWrite:
	case EFsFileWriteDirty:
		return(ENotifyWrite);	
	case EFsRawDiskWrite:
	case EFsLockDrive:
	case EFsUnlockDrive:
		return(ENotifyDisk);	
	case EFsFileTemp:		
	case EFsSetDriveName:
		return(ENotifyAll);	
	default:
		return(0);
		}
	}

TBool TOperation::IsCompleted() const
//
//
//
	{
	switch(iFunction)
		{
		case EFsNotifyChange:
		case EFsNotifyChangeEx:
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION			
		case EFsNotificationBuffer:
		case EFsNotificationRequest:
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION			
		case EFsNotifyDiskSpace:
		case EFsNotifyDismount:
		case EFsStartupInitComplete:
		case EFsResourceCountMarkEnd:
		case EFsPluginDoRequest:
		case EFsFileWriteDirty:
			return(EFalse);
		default:
			return(ETrue);
		}
	}
