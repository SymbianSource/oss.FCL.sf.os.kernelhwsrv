// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cusbmassstoragescheduler.h"
#include "cusbmassstorageserver.h"

/**
 @file
 @internalTechnology
 
 Creates a new Active scheduler
 */
CUsbMassStorageScheduler* CUsbMassStorageScheduler::NewL()
	{
	return new(ELeave) CUsbMassStorageScheduler;
	}

/**
 Destructor
 
 @internalTechnology
 */
CUsbMassStorageScheduler::~CUsbMassStorageScheduler()
	{
    // Nothing to do
	}

/**
 Give us a reference to the server
 
 @internalTechnology
 @param	aServer	A reference to the server
 */
void CUsbMassStorageScheduler::SetServer(CUsbMassStorageServer& aServer)
	{
	iMsServer = &aServer;	
	}

/**
 Inform the server that an error has occurred
 
 @internalTechnology
 @param	aError	Error that has occurred
 */
void CUsbMassStorageScheduler::Error(TInt aError) const
	{
	__PRINT1(_L("CUsbMassStorageScheduler::Error aError=%d"), aError);

	if (iMsServer)
		{
		iMsServer->Error(aError);
		}
	}

