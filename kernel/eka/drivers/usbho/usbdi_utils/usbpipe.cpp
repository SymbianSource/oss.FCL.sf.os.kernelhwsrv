// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <d32usbdi.h>
#include <d32usbtransfers.h>

#include "usbtransferstrategy.h"


/**
Queue a transfer.
@param[in] aTransfer The transfer descriptor to execute.
@param[out] aRequest Holds completion status of the transfer.
*/
EXPORT_C void RUsbPipe::Transfer(RUsbTransferDescriptor& aTransfer, TRequestStatus& aRequest)
	{
	IssueTransfer(aTransfer.iHandle, aRequest);
	}

