/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalComponent
*/

#ifndef TRANHANDLESERVERCONSTS_H
#define TRANHANDLESERVERCONSTS_H

#include <e32base.h>

_LIT(KTranHandleServerName, "!TranHandleSrv");

/** Version numbers. */
const TInt8 KTranHandleSrvMajorVersionNumber = 1;
const TInt8 KTranHandleSrvMinorVersionNumber = 1;
const TInt16 KTranHandleSrvBuildNumber = 0;

/** IPC messages supported by the server. */
enum TNcmIpc
    {
    ETransferHandle              = 0,
    ENotSupport
    };
    

#endif // TRANHANDLESERVERCONSTS_H