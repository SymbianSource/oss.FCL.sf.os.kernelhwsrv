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
//



/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <d32usbc.h>

#include "mstypes.h"
#include "msctypes.h"
#include "usbmsclientpanic.h"
#include "botmsctypes.h"
#include "testman.h"
#include "botmscserver.h"
#include "mdevicetransport.h"
#include "mserverprotocol.h"
#include "cbulkonlytransport.h"

#include "cusbmassstoragecontroller.h"
#include "botcontrolinterface.h"


#include "debug.h"
#include "msdebug.h"

TTestConfig::TTestConfig()
:   iTestMode(ENone)
    {
    }


////////////////////////////////////
/**
Called by CBulkOnlyTransport to create an instance of CControlInterface

@param aParent reference to the CBulkOnlyTransport
*/
CBotControlInterface* CBotControlInterface::NewL(CBulkOnlyTransport& aParent)
	{
    __MSFNSLOG
	CBotControlInterface* self = new(ELeave) CBotControlInterface(aParent);
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	CleanupStack::Pop();
	return self;
	}


void CBotControlInterface::ConstructL()
	{
    __MSFNLOG
	}


/**
c'tor

@param aParent reference to the CBulkOnlyTransport
*/
CBotControlInterface::CBotControlInterface(CBulkOnlyTransport& aParent):
	CActive(EPriorityStandard),
	iParent(aParent),
	iCurrentState(ENone)
	{
    __MSFNLOG
	}


/**
d'tor
*/
CBotControlInterface::~CBotControlInterface()
	{
    __MSFNLOG
	Cancel();
	}


/**
Called by  CBulkOnlyTransportStart to start control interface
*/
TInt CBotControlInterface::Start()
	{
    __MSFNLOG
    if (IsActive())
        {
        __PRINT(_L("Still active\n"));
        return KErrServerBusy;
        }
	ReadEp0Data();
	return KErrNone;
	}


/**
Called by CBulkOnlyTransportStart to stop control interface
*/
void CBotControlInterface::Stop()
	{
    __MSFNLOG
	if (!IsActive())
		{
		__PRINT(_L("Not active\n"));
		return;
		}

	__PRINT(_L("\nStopping...\n"));
	iCurrentState = ENone;
	Cancel();
	}


/**
Cancel outstanding request (if any)
*/
void CBotControlInterface::DoCancel()
	{
    __MSFNLOG
	switch(iCurrentState)
		{
		case EReadEp0Data:
			iParent.Ldd().ReadCancel(EEndpoint0);
			break;
		case ESendMaxLun:
			iParent.Ldd().WriteCancel(EEndpoint0);
			break;
		default:
			__PRINT(_L("\nWrong state !\n"));
			__ASSERT_DEBUG(EFalse, User::Panic(KUsbMsClientPanicCat, EMsControlInterfaceBadState));
		}
	}


/**
Implement CBotControlInterface state machine
*/
void CBotControlInterface::RunL()
	{
    __MSFNLOG
	if (iStatus != KErrNone)
		{
		__PRINT1(_L("Error %d in RunL\n"), iStatus.Int());

		//read EP0  again
		ReadEp0Data();
		return;
		}

	switch (iCurrentState)
		{
	case ESendMaxLun:
		ReadEp0Data();
		break;

	case EReadEp0Data:
		DecodeEp0Data();
		break;

	default:
		__PRINT(_L("  error: (Shouldn't end up here...)\n"));
		__ASSERT_DEBUG(EFalse, User::Panic(KUsbMsClientPanicCat, EMsControlInterfaceBadState));
		break;
		}
	}


/**
Post a read request to EEndpoint0 to read request header
*/
void CBotControlInterface::ReadEp0Data()
	{
    __MSFNLOG
	iParent.Ldd().Read(iStatus, EEndpoint0, iData, KRequestHdrSize);
	iCurrentState = EReadEp0Data;
	SetActive();
	}


/**
Decode request header and do appropriate action - get max LUN info or post a reset request
*/
void CBotControlInterface::DecodeEp0Data()
	{
    __MSFNLOG
	TInt err = iRequestHeader.Decode(iData);
	if (err != KErrNone)
		return;

    switch(iRequestHeader.iRequest)
		{
	//
	// GET MAX LUN (0xFE)
	//
    case TBotFunctionReqCb::EReqGetMaxLun:
		{
		__PRINT1(_L("DecodeEp0Data : 'Get Max LUN' Request MaxLun = %d"),iParent.MaxLun());

        if (   iRequestHeader.iRequestType != 0xA1 //value from USB MS BOT spec
            || iRequestHeader.iIndex > 15
            || iRequestHeader.iValue != 0
            || iRequestHeader.iLength != 1)
            {
    	    __PRINT(_L("GetMaxLun command packet check error"));
            iParent.Ldd().EndpointZeroRequestError();
            ReadEp0Data();  //try to get another request
            }
        else
            {
            iData.FillZ(1);  //Return only 1 byte to host
            iData[0] = static_cast<TUint8>(iParent.MaxLun());	// Supported Units
            iParent.Ldd().Write(iStatus, EEndpoint0, iData, 1);

            iCurrentState = ESendMaxLun;
            SetActive();
            }
		}
        break;
	//
	// RESET (0xFF)
	//
	case TBotFunctionReqCb::EReqReset:
		{
		__PRINT(_L("DecodeEp0Data : 'Mass Storage Reset' Request"));
        __TESTMODEPRINT("------- BOT RESET ------");

        if (   iRequestHeader.iRequestType != 0x21 //value from USB MS BOT spec
            || iRequestHeader.iIndex > 15
            || iRequestHeader.iValue != 0
            || iRequestHeader.iLength != 0)
            {
		    __PRINT(_L("MSC Reset command packet check error"));
            iParent.Ldd().EndpointZeroRequestError();
            ReadEp0Data();  //try to get another request
            }
        else
            {
            iParent.HwStop();
            iParent.Controller().Reset();
            iParent.HwStart(ETrue);
            iParent.Ldd().SendEp0StatusPacket();
            }
		}
        break;
	//
	// Unknown?
	//
	default:
		__PRINT(_L("DecodeEp0Data : Unknown Request"));
        ReadEp0Data();  //try to get another request
        break;
        }
	}
