// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tspcclientinterfaceTraces.h"
#endif

#include "msctypes.h"
#include "mtransport.h"
#include "mprotocol.h"
#include "tscsiclientreq.h"
#include "tscsiprimarycmds.h"
#include "tscsiblockcmds.h"
#include "tspcclientinterface.h"


/**
Constructor.

@param aTransport Referance to the transport interface to be used to send the
SCSI SPC message
*/
TSpcClientInterface::TSpcClientInterface(MTransport& aTransport)
:   iTransport(aTransport)
    {
    }

/**
Destructor.
*/
TSpcClientInterface::~TSpcClientInterface()
    {
    }

/**
Create a SCSI INQUIRY command and send the command via the transport layer. The
function leaves if the device response is not compliant with the protocol
standard.

@param aInfo The returned information by the peripheral device

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt TSpcClientInterface::InquiryL(TPeripheralInfo& aInfo)
    {
    TScsiClientInquiryReq inquiryReq;

    TScsiClientInquiryResp inquiryResp(aInfo);

    TInt err = iTransport.SendControlCmdL(&inquiryReq, &inquiryResp);
    return err;
    }


/**
Create a SCSI REQUEST SENSE command and send the command via the transport
layer. The function leaves if the device response is not compliant with the
protocol standard.

@param aSenseInfo The returned SENSE INFO

@return TInt TInt KErrNone if successful otherwise KErrCommandFailed to indicate
a device status error
*/
TInt TSpcClientInterface::RequestSenseL(TSenseInfo& aSenseInfo)
    {
    TScsiClientRequestSenseReq requestSenseReq;
    TScsiClientRequestSenseResp requestSenseResp;

    TInt err = iTransport.SendControlCmdL(&requestSenseReq, &requestSenseResp);
    aSenseInfo = requestSenseResp.iSenseInfo;

    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, TSPCCLIENTINTERFACE_10,
                 "SCSI SENSE INFO Response %x Code=%x",
                 (TUint32)requestSenseResp.iResponseCode, (TUint32)aSenseInfo.iSenseCode);
    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, TSPCCLIENTINTERFACE_11,
                 "                Qual=%08x Add=%08x",
                 (TUint32)aSenseInfo.iQualifier, (TUint32)aSenseInfo.iAdditional);
    return err;
    }


/**
Create a SCSI TEST UNIT READY command and send the command via the transport
layer. The function leaves if the device response is not compliant with the
protocol standard.

@return TInt KErrNone if successful or otherwise KErrCommandFailed to indicate a
device status error
*/
TInt TSpcClientInterface::TestUnitReadyL()
    {
    TScsiClientTestUnitReadyReq testUnitReadyReq;

    TInt err = iTransport.SendControlCmdL(&testUnitReadyReq);
    return err;
    }


/**
Creates a SCSI PREVENT ALLOW MEDIUM REMOVAL command and sends the command via
the transport layer. The function leaves if the device response is not
compliant with the protocol standard.

@param aPrevent Set the PREVENT flag

@return TInt KErrNone if successful or otherwise KErrCommandFailed to indicate a
device status error
*/
TInt TSpcClientInterface::PreventAllowMediumRemovalL(TBool aPrevent)
    {
    TScsiClientPreventMediaRemovalReq preventAllowMediaRemovalReq(aPrevent);
    TInt err = iTransport.SendControlCmdL(&preventAllowMediaRemovalReq);
    return err;
    }



