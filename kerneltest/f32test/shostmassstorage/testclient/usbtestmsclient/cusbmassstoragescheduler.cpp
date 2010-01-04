// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include "CUsbMassStorageScheduler.h"
#include "CUsbMassStorageServer.h"

/**
 * @file
 * @internal
 *
 * Creates a new Active scheduler
 */
CUsbMassStorageScheduler* CUsbMassStorageScheduler::NewL()
	{
	CUsbMassStorageScheduler* self = new(ELeave) CUsbMassStorageScheduler;
	CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
	return self;
	}

void CUsbMassStorageScheduler::ConstructL()
/**
 * Performs 2nd phase construction of the USB scheduler.
 *
 * @internal
 */
    {
    // Do nothing for now. May be remove altogether
	}
 
/**
 * Destructor
 *
 * @internal
 */
CUsbMassStorageScheduler::~CUsbMassStorageScheduler()
	{
    // Nothing to do
	}

/**
 * Give us a reference to the server
 *
 * @internal
 * @param	aServer	A reference to the server
 */
void CUsbMassStorageScheduler::SetServer(CUsbMassStorageServer& aServer)
	{
	iMsServer = &aServer;	
	}

/**
 * Inform the server that an error has occurred
 *
 * @internal
 * @param	aError	Error that has occurred
 */
void CUsbMassStorageScheduler::Error(TInt aError) const
	{
	__PRINT1(_L("CUsbMassStorageScheduler::Error aError=%d"), aError);

	if (iMsServer)
		{
		iMsServer->Error(aError);
		}
	}
