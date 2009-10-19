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
// e32\include\e32lmsg.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __E32LMSG_H__
#define __E32LMSG_H__
#include <e32cmn.h>

const TInt KMaxLocaleMessageText=0x80;
//
enum TLocaleMessage
	{
	EFileServer_Button1,
	EFileServer_Button2,
	EFileServer_PutTheCardBackLine1,
	EFileServer_PutTheCardBackLine2,
	EFileServer_LowPowerLine1,
	EFileServer_LowPowerLine2,
	EFileServer_DiskErrorLine1,
	EFileServer_DiskErrorLine2,
	ESoundDriver_Chimes,
	ESoundDriver_Rings,
	ESoundDriver_Signal,
	EMediaDriver_DiskNameInternal,
	EMediaDriver_DiskNameExternal1,
	EMediaDriver_DiskNameExternal2,
	EMediaDriver_DiskNameExternal3,
	EMediaDriver_DiskNameExternal4,
	EMediaDriver_DiskNameExternal5,
	EMediaDriver_DiskNameExternal6,
	EMediaDriver_DiskNameExternal7,
	EMediaDriver_DiskNameExternal8,
	EMediaDriver_SocketName0,
	EMediaDriver_SocketName1,
	EMediaDriver_SocketName2,
	EMediaDriver_SocketName3,
	ELocaleMessages_LastMsg
	};

class TLocaleMessageText : public TBuf<KMaxLocaleMessageText>
	{
public:
	IMPORT_C TLocaleMessageText();
	IMPORT_C TLocaleMessageText(TLocaleMessage aMessageNo);
	IMPORT_C void Set(TLocaleMessage aMessageNo);
	};

#endif

