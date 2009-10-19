// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a Symbian OS server that exposes the RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#include <e32svr.h>
#include <f32file.h>

#include <d32usbdi_hubdriver.h>
#include <d32otgdi.h>

#include "usbtypes.h"

#include "cusbhostao.h"
#include "cusbhost.h"


#include "msmanclientserver.h"
#include "cusbhostserver.h"
#include "cusbhostsession.h"

#include "tmslog.h"

CUsbHostServer* CUsbHostServer::NewLC()
	{
    __MSFNSLOG
	CUsbHostServer* r = new (ELeave) CUsbHostServer();
	CleanupStack::PushL(r);
    r->ConstructL();
	return r;
	}


CUsbHostServer::CUsbHostServer()
:   CServer2(EPriorityLow)
	{
    __MSFNSLOG
	}


void CUsbHostServer::ConstructL()
    {
    __MSFNSLOG
    iUsbHost = CUsbHost::NewL();
	StartL(KUsbHostServerName);
    }


CUsbHostServer::~CUsbHostServer()
	{
    __MSFNSLOG
    delete iUsbHost;
	}


CSession2* CUsbHostServer::NewSessionL(const TVersion &aVersion, const RMessage2& /*aMessage*/) const
	{
    __MSFNSLOG
	TVersion v(KUsbHostSrvMajorVersionNumber, KUsbHostSrvMinorVersionNumber, KUsbHostSrvBuildVersionNumber);

	if (!User::QueryVersionSupported(v, aVersion))
		User::Leave(KErrNotSupported);

	CUsbHostSession* session = CUsbHostSession::NewL();

	return session;
	}



TInt CUsbHostServer::RunError(TInt aError)
	{
    __MSFNSLOG

	Message().Complete(aError);
	ReStart();
    return KErrNone;
	}


void CUsbHostServer::AddSession()
    {
    __MSFNSLOG
    ++iSessionCount;
    }

void CUsbHostServer::RemoveSession()
    {
    __MSFNSLOG
    if (--iSessionCount == 0)
        {
        User::After(1000000);
        CActiveScheduler::Stop();
        }
    }
