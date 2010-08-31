/*
* Copyright (c) 1997-2010 Nokia Corporation and/or its subsidiary(-ies).
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
* Implements a Session of a Symbian OS server for the RUsb API
*
*/

/**
 @file
*/

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <usb.h>
#include "transfersession.h"
#include "transferserver.h"
#include "transfersrv.h"
#include "tranhandlesrv.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "transfersessionTraces.h"
#endif


CTransferSession* CTransferSession::NewL(CTransferServer* aServer)
	{
	return (new (ELeave) CTransferSession(aServer));
	}


CTransferSession::CTransferSession(CTransferServer* aServer)
	: iTransferServer(aServer)
	{
	iTransferServer->IncrementSessionCount();
	}


CTransferSession::~CTransferSession()
	{
	TUSB_PRINT("CTransferSession::~CTransferSession");
	OstTrace0(TRACE_NORMAL, CTRANSFERSESSION_DCTRANSFERSESSION, "CTransferSession::~CTransferSession");
	iTransferServer->DecrementSessionCount();
	TUSB_PRINT("<<CTransferSession::~CTransferSession");
	OstTrace0(TRACE_NORMAL, CTRANSFERSESSION_DCTRANSFERSESSION_DUP01, "<<CTransferSession::~CTransferSession");
	}


void CTransferSession::ServiceL(const RMessage2& aMessage)
	{
	DispatchMessageL(aMessage);
	}

void CTransferSession::CreateL()
	{
	}

void CTransferSession::DispatchMessageL(const RMessage2& aMessage)
	{
	TInt ret = KErrNone;
	TName string;

	switch (aMessage.Function())
		{
	case ESetConfigFileName:
		ret = aMessage.Read(0, string);
		if (ret != KErrNone)
			break;
		ret = iTransferServer->SetupLdds(string);
		break;

	default:
		ret = KErrNotSupported;
		break;
		}

	aMessage.Complete(ret);
	}

