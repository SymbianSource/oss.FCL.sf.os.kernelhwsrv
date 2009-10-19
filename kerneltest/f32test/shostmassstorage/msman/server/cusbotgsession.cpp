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
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#include <e32cmn.h>
#include <e32base.h>

#include "tmslog.h"
#include "msmanclientserver.h"

#include <f32file.h>
#include <d32usbdi_hubdriver.h>
#include <d32otgdi.h>
#include "usbtypes.h"

#include "cusbotg.h"

#include "cusbotgserver.h"
#include "cusbotgsession.h"


CUsbOtgSession* CUsbOtgSession::NewL()
	{
    __MSFNSLOG
	CUsbOtgSession* r = new (ELeave) CUsbOtgSession();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}


CUsbOtgSession::CUsbOtgSession()
	{
    __MSFNSLOG

	}

void CUsbOtgSession::ConstructL()
	{
    __MSFNSLOG
 	}


void CUsbOtgSession::CreateL()
	{
    __MSFNSLOG
    Server().AddSession();
	}


CUsbOtgSession::~CUsbOtgSession()
	{
    __MSFNSLOG
    Server().RemoveSession();
	}


void CUsbOtgSession::ServiceL(const RMessage2& aMessage)
	{
    __MSFNSLOG
	DispatchMessageL(aMessage);
	}


void CUsbOtgSession::DispatchMessageL(const RMessage2& aMessage)
	{
    __MSFNSLOG
	TInt ret = KErrNone;

	switch (aMessage.Function())
		{
    case EUsbOtgDeviceInserted:
        DeviceInsertedL(aMessage);
        break;
    case EUsbOtgNotifyChange:
        NotifyChange(aMessage);
        break;
    case EUsbOtgNotifyChangeCancel:
        NotifyChangeCancel();
        break;
	case EUsbOtgBusDrop:
		ret = BusDrop();
		break;
	default:
		aMessage.Panic(KUsbOtgClientPncCat, EUsbOtgPanicIllegalIPC);
		break;
		}

	aMessage.Complete(ret);
	}


void CUsbOtgSession::DeviceInsertedL(const RMessage2& aMessage)
    {
    __MSFNSLOG
    TBool inserted = Server().iUsbOtg->DeviceInserted();
    TPckgBuf<TBool> p(inserted);
    aMessage.WriteL(0,p);
    }

void CUsbOtgSession::NotifyChange(const RMessage2& aMessage)
    {
    __MSFNSLOG
    Server().iUsbOtg->NotifyChange(aMessage);
    }


void CUsbOtgSession::NotifyChangeCancel()
    {
    __MSFNSLOG
    Server().iUsbOtg->NotifyChangeCancel();
    }


TInt CUsbOtgSession::BusDrop()
    {
    __MSFNSLOG
    return Server().iUsbOtg->BusDrop();
	}
