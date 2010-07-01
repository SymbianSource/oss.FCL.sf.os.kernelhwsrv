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
@internalTechnology
*/

#include <e32base.h>
#include "tranhandlesrv.h"
#include "tranhandleserverconsts.h"


/** Constructor */
EXPORT_C RTranHandleSrv::RTranHandleSrv()
    {
    }

/** Destructor */
EXPORT_C RTranHandleSrv::~RTranHandleSrv()
    {
    }

EXPORT_C TVersion RTranHandleSrv::Version() const
    {
    return TVersion(    KTranHandleSrvMajorVersionNumber,
                        KTranHandleSrvMinorVersionNumber,
                        KTranHandleSrvBuildNumber
                    );
    }

EXPORT_C TInt RTranHandleSrv::Connect()
    {
    return CreateSession(KTranHandleServerName, Version(), 1);
    }

EXPORT_C TInt RTranHandleSrv::TransferHandle(RHandleBase& aHandle, RHandleBase& aChunk)
    {
    return SendReceive(ETransferHandle, TIpcArgs(aHandle, aChunk));
    }

