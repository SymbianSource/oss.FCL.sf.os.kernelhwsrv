// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "debug.h"
#include "msdebug.h"

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
	__MSFNLOG
    }

/**
Destructor.
*/
TSpcClientInterface::~TSpcClientInterface()
    {
	__MSFNLOG
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
	__MSFNLOG
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
	__MSFNLOG
    TScsiClientRequestSenseReq requestSenseReq;
    TScsiClientRequestSenseResp requestSenseResp;

    TInt err = iTransport.SendControlCmdL(&requestSenseReq, &requestSenseResp);
    aSenseInfo = requestSenseResp.iSenseInfo;

    __SCSIPRINT4(_L("SCSI SENSE INFO Response%08x Code=%08x, Qual=%08x Add=%08x"),
                 requestSenseResp.iResponseCode,
                 aSenseInfo.iSenseCode, aSenseInfo.iQualifier, aSenseInfo.iAdditional);
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
	__MSFNLOG
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
	__MSFNLOG
    TScsiClientPreventMediaRemovalReq preventAllowMediaRemovalReq(aPrevent);
    TInt err = iTransport.SendControlCmdL(&preventAllowMediaRemovalReq);
	return err;
    }



