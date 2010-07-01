/*
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Definitions required for RUsb
*
*/

/**
 @file
 @internalComponent
*/

#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include <e32std.h>
#include <e32base.h>


_LIT(KTransferServerName, "!transferserver");

const TInt KTransferSrvMajorVersionNumber = 1;
const TInt KTransferSrvMinorVersionNumber = 1;
const TInt KTransferSrvBuildVersionNumber = 0;

const TUid KTransferSvrUid = {0x101FE1DB};


enum TTransferMessages
	{
	ESetConfigFileName,
	ETransferNotSupport
	};

NONSHARABLE_CLASS(RTransferSrv) : public RSessionBase
    {
public:
    IMPORT_C RTransferSrv();
    IMPORT_C ~RTransferSrv();

public:
    IMPORT_C TInt Connect();
    IMPORT_C TVersion Version() const;
	IMPORT_C TInt SetConfigFileName(TDes& aString);

    };

#endif //__TRANSFER_H__

