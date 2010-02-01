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
// f32\inc\common.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#if !defined(__COMMON_H__)
#define __COMMON_H__
#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif
#include <d32locd.h>
#include "u32std.h"

//
// Common constants used by both EFSRV and the filesystems
//



/**
A bit mask representing a volume and file attribute combination.

It represents a set of attributes that must not be set, unset or used
in calls to RFile::Set() and CMountCB::MatchEntryAtt().

@see CMountCB::MatchEntryAtt()
@see RFile::Set()
*/
const TUint KEntryAttIllegal=(KEntryAttVolume|KEntryAttDir);



/**
A file attribute that marks the file as having been modified. This is an indication
that the file is modified but the modifications are not yet committed.

@see RFs::SetEntry()
*/
const TUint KEntryAttModified=0x20000000;



/**
Defines a criteria that states that an entry must be a file.

This can be used in calls to CMountCB::MatchEntryAtt().

@see CMountCB::MatchEntryAtt()
*/
const TUint KEntryAttMustBeFile=0x80000000;



/**
Indicates that data is to be read from the current read position when
passed to any of the Read() overloaded functions of RFile.

This is maintained for BC. File server and file server client library, internally, 
use KCurrentPosition64 for indicating a read / write request from current file position. 

@see RFile::Read()
@see KCurrentPosition64
*/
const TInt KCurrentPosition=KMinTInt;


/**
Indicates that data is to be read from the current read position when
passed to any of the Read() overloaded functions of RFile.

@prototype

@see RFile::Read()
*/
const TInt64 KCurrentPosition64=(TInt64)KMaxTUint64;



#endif
