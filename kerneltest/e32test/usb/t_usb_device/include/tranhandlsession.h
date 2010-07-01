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
const TInt8 KNcmSrvMajorVersionNumber = 1;
const TInt8 KNcmSrvMinorVersionNumber = 1;
const TInt16 KNcmSrvBuildNumber = 0;

/** IPC messages supported by the server. */
enum TNcmIpc
    {
    ENcmTransferHandle              = 0,
    ENcmSetIapId,
    ENcmSetDhcpResult,
    ENcmDhcpProvisionNotify,
    ENcmDhcpProvisionNotifyCancel,
    ENcmTransferBufferSize,
    ENcmNotSupport
    };
    
/** Panic category with which the ACM server panics the client. */
_LIT(KNcmSrvPanic,"NCMServer");

/** Panic codes with which the ACM server panics the client. */
enum TNcmSvrPanic
    {
    /** The client has sent a bad IPC message number. */
    ENcmBadNcmMessage        = 0
    };
    
/** Set value to server. the value is defined by type.*/
enum TNcmSetType
    {
    ENcmSetTypeIapId,
    ENcmSetTypeDhcpReturn
    };

#endif // ACMSERVERCONSTS_H
