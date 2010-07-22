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
// f32\sfsrv\cl_std.h
// 
//

#include "common.h"		
#include "message.h"
#include <f32file.h>
#include <f32file_private.h>
#ifndef  SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
#include <f32file64.h>
#endif
#include "f32notification.h"
#include <f32ver.h>
#include <e32svr.h>
#include <f32plugin.h>
#include "f32trace.h"

enum TClientPanic
	{
	EDriveUnitBadDrive,
	EDriveUnitBadDriveText,
	EFindFileIllegalMode,
	EBadUidIndex,
	ERawDiskCannotClose,
	EFTextIllegalSeekMode,
	ENotImplemented,
	EFManNotActive,
	EFManActive,
	EFManNoActionSet, //10
	EFManUnknownAction,
	EFManCurrentEntryInvalid,
	EFManBadValueFromObserver,
	EFManBadNames,
	EFManRecursiveRename,
	EDirListError,
	EAddDirBadName,
	ELockLengthZero,
	EUnlockLengthZero,
	EPosNegative, //20
	ESizeNegative,
	EAttributesIllegal,
	EEntryArrayBadIndex,
	ECDirBadSortType,
	EParsePtrBadDescriptor0,
	EParsePtrBadDescriptor1,
	EParsePtrCAccessError,
	EBadLength,
	EDefaultPathCalled,
	ESetDefaultPathCalled, //30
	EFindFileIllegalAttribute,
	ENotificationPanic
	};
//
enum TClientFault
	{
	ENotifyChangeCancel,
	ESessionClose,
	ESubSessionClose
	};
//
GLREF_C void Panic(TClientPanic aPanic);
GLREF_C void Fault(TClientFault aFault);
//
