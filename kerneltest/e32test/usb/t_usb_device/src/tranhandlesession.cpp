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

#include <e32base.h>

#include "tranhandleserverconsts.h"
#include "tranhandlesession.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tranhandlesessionTraces.h"
#endif
#include "activecontrol.h"



CTranHandleSession* CTranHandleSession::NewL(CActiveControl& aControl)
    {
    CTranHandleSession* self = new(ELeave) CTranHandleSession(aControl);
    return self;
    }

CTranHandleSession::CTranHandleSession(CActiveControl& aControl)
    : iActiveControl(aControl)
    {
    }

CTranHandleSession::~CTranHandleSession()
    {
    OstTrace0(TRACE_NORMAL, CTRANHANDLESESSION_DCTRANHANDLESESSION, "CTranHandleSession::~CTranHandleSession");
    }

void CTranHandleSession::ServiceL(const RMessage2& aMessage)
    {
	OstTrace0(TRACE_NORMAL, CTRANHANDLESESSION_SERVICEL, "CTranHandleSession::ServiceL");
	TInt r;
    switch ( aMessage.Function() )
        {
    case ETransferHandle:
        {
        RMessagePtr2 messagePtr(aMessage);
        TRAP(r, iActiveControl.ConstructLOnSharedLdd(messagePtr));

        aMessage.Complete(r);
        break;
        }

    default:
        aMessage.Complete(KErrNotSupported);
        break;
        }
    }
