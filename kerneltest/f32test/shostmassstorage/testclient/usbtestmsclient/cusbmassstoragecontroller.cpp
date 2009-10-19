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
// CUsbMassStorageController implementation.
// 
//



/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <d32usbc.h>

#include "mstypes.h"
#include "msctypes.h"
#include "testman.h"
#include "drivemanager.h"

#include "tscsiserverreq.h"
#include "tscsiservercmds.h"
#include "mserverprotocol.h"
#include "cscsiserverprotocol.h"
#include "mdevicetransport.h"
#include "cbulkonlytransport.h"

#include "cusbmassstorageserver.h"
#include "cusbmassstoragecontroller.h"
#include "debug.h"
#include "msdebug.h"




CUsbMassStorageController* CUsbMassStorageController::NewL()
    {
    __MSFNSLOG
	CUsbMassStorageController* self = new (ELeave) CUsbMassStorageController();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }


CUsbMassStorageController::CUsbMassStorageController()
    {
    }


void CUsbMassStorageController::ConstructL()
	{
#ifdef MSDC_TESTMODE
    // TestParser for modifying client behaviour to create  BOT 13 Case device
    // exceptions conditions
    __TESTMODEPRINT("Test Mode is active");
    iTestParser = new (ELeave) TTestParser;
#endif
	}

/**
Destructor
*/
CUsbMassStorageController::~CUsbMassStorageController()
	{
    __MSFNLOG
	delete iServer;
	delete iProtocol;
	delete iTransport;
	delete iDriveManager;
	}

/**
Creates the drive manager, transport, protocol and server

@param aMaxDrives Maximum number of Mass Storage drives supported.
*/
void CUsbMassStorageController::CreateL(const TLunToDriveMap& aDriveMapping)
	{
    __MSFNLOG
	//Create and init drive manager
	iDriveManager = CDriveManager::NewL(aDriveMapping);

	//Create transport and protocol and initialize them
	__PRINT(_L("CUsbMassStorageController::CreateL: Creating transport and protocol"));
#ifdef MSDC_TESTMODE
	iTransport = CBulkOnlyTransport::NewL(aDriveMapping.Count(), *this, iTestParser);
	iProtocol = CScsiServerProtocol::NewL(*iDriveManager, iTestParser);
#else
	iTransport = CBulkOnlyTransport::NewL(aDriveMapping.Count(), *this);
	iProtocol = CScsiServerProtocol::NewL(*iDriveManager);
#endif
	iTransport->RegisterProtocol(*iProtocol);
	iProtocol->RegisterTransport(iTransport);

	//Create and start server
	__PRINT(_L("CUsbMassStorageController::CreateL: Creating server"));
	iServer = CUsbMassStorageServer::NewLC(*this);
	CleanupStack::Pop(iServer);
	}

/**
Returns a reference to the drive manager

@return A reference to the drive manager
*/
CDriveManager& CUsbMassStorageController::DriveManager()
	{
    __MSFNLOG
	return *iDriveManager;
	}

/**
Starts the transport and initializes the protocol.

@param aConfig Reference to Mass Storage configuration data
*/
TInt CUsbMassStorageController::Start(const TMassStorageConfig& aConfig)
	{
    __MSFNLOG
	//Save this value for use in the Reset method.
	iConfig = aConfig;
	TInt err = KErrNotReady;
	if (iProtocol && iTransport)
		{
		__PRINT(_L("CUsbMassStorageController::Start: Starting"));
		iProtocol->SetParameters(aConfig);
		err = iTransport->Start();
		}
	return err;
	}

/**
Stops the transport.
*/
TInt CUsbMassStorageController::Stop()
	{
    __MSFNLOG
	TInt err = KErrNotReady;
	if (iTransport)
		{
		__PRINT(_L("CUsbMassStorageController::Stop: Stopping"));
		err = iTransport->Stop();
		}

	iDriveManager->SetCritical(CDriveManager::KAllLuns, EFalse);   //unset critical
	return err;
	}

/**
Delete the transport and protocol and start new ones.
*/
void CUsbMassStorageController::Reset()
	{
    __MSFNLOG
	delete iProtocol;
	iProtocol = NULL;

	//Create transport and protocol and initialize them
	__PRINT(_L("CUsbMassStorageController::Reset: Creating  protocol"));

#ifdef MSDC_TESTMODE
	TRAPD(err,iProtocol = CScsiServerProtocol::NewL(*iDriveManager, iTestParser));
#else
	TRAPD(err,iProtocol = CScsiServerProtocol::NewL(*iDriveManager));
#endif
	err = err;
	__ASSERT_DEBUG(err==KErrNone, User::Invariant());
	iTransport->RegisterProtocol(*iProtocol);
	iProtocol->RegisterTransport(iTransport);
    iProtocol->SetParameters(iConfig);
	}
