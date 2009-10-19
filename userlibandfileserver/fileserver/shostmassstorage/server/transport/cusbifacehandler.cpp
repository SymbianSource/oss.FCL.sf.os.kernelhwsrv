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
#include <e32base_private.h>
#include <d32usbdi_hubdriver.h>
#include <d32usbdi.h>
#include <d32otgdi.h>
#include <d32usbdescriptors.h>
#include <d32usbtransfers.h>
#include "botmsctypes.h"
#include "msctypes.h"
#include "mscutils.h"
#include "mtransport.h"
#include "mprotocol.h"
#include "cusbifacehandler.h"
#include "cbulkonlytransport.h"
#include "debug.h"
#include "msdebug.h"

CUsbInterfaceHandler* CUsbInterfaceHandler::NewL(RUsbInterface &aInterface)
	{
	return new (ELeave) CUsbInterfaceHandler(aInterface);
	}

CUsbInterfaceHandler::CUsbInterfaceHandler(RUsbInterface &aInterface)
:	CActive(EPriorityStandard),
	iInterface(aInterface)
	{
    __MSFNLOG
	CActiveScheduler::Add(this);
	}

CUsbInterfaceHandler::~CUsbInterfaceHandler()
	{
    __MSFNLOG
	if (iState != ENone)
		{
		iState = ENone;
		iBotGetMaxLun.Complete(KErrCancel);
		Cancel();
		}
	}

/**
Cancellation of outstanding request
*/
void CUsbInterfaceHandler::DoCancel()
	{
    __MSFNLOG
	}

/**
Completion of USB transport request.
*/
void CUsbInterfaceHandler::RunL()
    {
    __MSFNLOG
	TInt error = iStatus.Int();

	if (error == KErrUsbStalled && iState == EGetMaxLun)
        {
		__BOTPRINT(_L("...KErrUsbStalled"));
		iState = EReset;
		Reset();
		return;
        }

	if (error == KErrNone)
        {
		__BOTPRINT(_L("...KErrNone"));

		if (iState == EGetMaxLun)
            {
			__BOTPRINT(_L("...sending GetMaxLun response"));
			*ipGetMaxLun = iBuffer[0];
            }
		else
            {
			__BOTPRINT(_L("...defaulting to 0"));
			*ipGetMaxLun = 0;
            }
        }
    else
        {
        __BOTPRINT(_L("...completeing with KErrGeneral"));
        error = KErrGeneral;
        }

    iState = ENone;
	iBotGetMaxLun.Complete(error);
    }


void CUsbInterfaceHandler::GetMaxLun(TLun* aMaxLun, const RMessage2& aMessage)
	{
    __MSFNLOG

	/* Send the Get max lun command in the ep0 */
	RUsbInterface::TUsbTransferRequestDetails reqDetails;
	_LIT8(KNullDesC8,"");
	iBotGetMaxLun = aMessage;
	iState = EGetMaxLun;
	ipGetMaxLun = aMaxLun;

	reqDetails.iRequestType = 0xA1;
	reqDetails.iRequest = 0xFE;
	reqDetails.iValue = 0x0000;
	reqDetails.iIndex = 0x0000;
	reqDetails.iFlags = 0x04;		// Short transfer OK

	iBuffer.SetLength(1);
	iInterface.Ep0Transfer(reqDetails, KNullDesC8, iBuffer, iStatus);
	SetActive();
	}


void CUsbInterfaceHandler::Reset()
	{
    __MSFNLOG
	RUsbInterface::TUsbTransferRequestDetails reqDetails;
	_LIT8(KNullDesC8,"");

	reqDetails.iRequestType = 0x21;
	reqDetails.iRequest = 0xFF;
	reqDetails.iValue = 0x0000;
	reqDetails.iIndex = 0x0000;
	reqDetails.iFlags = 0x04;		// Short transfer OK

	iInterface.Ep0Transfer(reqDetails, KNullDesC8, (TDes8 &) KNullDesC8, iStatus);
    SetActive();
	}
