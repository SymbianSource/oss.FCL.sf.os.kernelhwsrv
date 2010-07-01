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


#include "tranhandleserver.h"
#include "tranhandlesession.h"
#include "tranhandleserversecuritypolicy.h"
#include "tranhandleserverconsts.h"
#include "general.h"



CTranHandleServer* CTranHandleServer::NewL(CActiveControl& aControl)
    {
    RDebug::Printf("CTranHandleServer::NewL");
    CTranHandleServer* self = new(ELeave) CTranHandleServer(aControl);
    CleanupStack::PushL(self);
    TInt err = self->Start(KTranHandleServerName);

    if ( err != KErrAlreadyExists )
        {
        User::LeaveIfError(err);
        }
    CleanupStack::Pop(self);
    return self;
    }

CTranHandleServer::~CTranHandleServer()
    {
    RDebug::Printf("CTranHandleServer::~CTranHandleServer");
    }

CTranHandleServer::CTranHandleServer(CActiveControl& aControl)
 :  CPolicyServer(CActive::EPriorityStandard, KTranHandleServerPolicy, ESharableSessions),
    iActiveControl(aControl)
    {
    }

CSession2* CTranHandleServer::NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const
    {
    //Validate session as coming from UsbSvr
	RDebug::Printf("CTranHandleServer::NewSessionL");
    CTranHandleSession* sess = CTranHandleSession::NewL(iActiveControl);
    return sess;
    }
