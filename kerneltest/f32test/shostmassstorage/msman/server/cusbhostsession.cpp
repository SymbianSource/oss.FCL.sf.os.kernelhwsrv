// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
//
//



/**
 @file
 @internalTechnology
*/

#include <e32cmn.h>
#include <e32base.h>

#include "msmanclientserver.h"

#include <f32file.h>
#include <d32usbdi_hubdriver.h>
#include <d32otgdi.h>
#include "usbtypes.h"

#include "cusbhostao.h"
#include "cusbhost.h"


#include "cusbhostserver.h"
#include "cusbhostsession.h"


CUsbHostSession* CUsbHostSession::NewL()
    {
    CUsbHostSession* r = new (ELeave) CUsbHostSession();
    CleanupStack::PushL(r);
    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


CUsbHostSession::CUsbHostSession()
    {
    }

void CUsbHostSession::ConstructL()
    {
    }


void CUsbHostSession::CreateL()
    {
    Server().AddSession();
    }


CUsbHostSession::~CUsbHostSession()
    {
    Server().RemoveSession();
    }


void CUsbHostSession::ServiceL(const RMessage2& aMessage)
    {
    DispatchMessageL(aMessage);
    }


void CUsbHostSession::DispatchMessageL(const RMessage2& aMessage)
    {
    TInt ret = KErrNone;

    switch (aMessage.Function())
        {
    case EUsbHostStart:
        ret = Start(aMessage);
        break;
    default:
        aMessage.Panic(KUsbHostClientPncCat, EUsbHostPanicIllegalIPC);
        break;
        }

    aMessage.Complete(ret);
    }


TInt CUsbHostSession::Start(const RMessage2& aMessage)
    {
    Server().iUsbHost->Start();
    return KErrNone;
    }
