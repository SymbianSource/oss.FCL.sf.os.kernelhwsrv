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
 
#ifndef TRANHANDLESERVERSECURITYPOLICY_H
#define TRANHANDLESERVERSECURITYPOLICY_H

#include <e32base.h>
#include "tranhandleserverconsts.h"

const TInt KTranHanleServerRanges[] = 
    {
    ETransferHandle,             /** pass */
    ENotSupport,        /** fail (to KMaxTInt) */
    };

const TUint KTranHanleServerRangeCount = sizeof(KTranHanleServerRanges) / sizeof(KTranHanleServerRanges[0]);

const TInt KPolicyPass = 0;

const TUint8 KTranHanleServerElementsIndex[KTranHanleServerRangeCount] = 
    {
    KPolicyPass,                  /** All (valid) APIs */
    CPolicyServer::ENotSupported,   /** remainder of possible IPCs */
    };


const CPolicyServer::TPolicyElement KTranHanleServerElements[] = 
    {
        { 
        _INIT_SECURITY_POLICY_PASS
        },
    };

/** Main policy */
const CPolicyServer::TPolicy KTranHandleServerPolicy = 
    {
    CPolicyServer::EAlwaysPass, /** Specifies all connect attempts should pass */
    KTranHanleServerRangeCount,
    KTranHanleServerRanges,
    KTranHanleServerElementsIndex,
    KTranHanleServerElements,
    };
    
#endif // TRANHANDLESERVERSECURITYPOLICY_H