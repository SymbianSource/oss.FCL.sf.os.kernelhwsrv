// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cusbmassstoragecontroller.h"
#include "massstoragedebug.h"
#include "scsiprot.h"
#include "cbulkonlytransport.h"

/**
Destructor
*/
CUsbMassStorageController::~CUsbMassStorageController()
	{
	delete iServer;
	delete iProtocol;
	delete iTransport;
	delete iDriveManager;
	}

/**
Creates the drive manager, transport, protocol and server

@param aMaxDrives Maximum number of Mass Storage drives supported.
*/
void CUsbMassStorageController::CreateL(RArray<TInt>& aDriveMapping)
	{
	__PRINT(_L("CUsbMassStorageController::CreateL In"));
#if !defined(__WINS__) && !defined(__X86__)
	iTransportLddFlag = EUsbcsc; // Create transport object using SC Ldd By default
#else
	iTransportLddFlag = EUsbc;
#endif
	//Save this value for use in the Reset method.
	iMaxDrives = aDriveMapping.Count();
	//Create and init drive manager
	iDriveManager = CDriveManager::NewL(aDriveMapping);

	//Create transport and protocol and initialize them
	__PRINT(_L("CUsbMassStorageController::CreateL: Creating transport and protocol"));
	iTransport = CBulkOnlyTransport::NewL(iMaxDrives, *this, iTransportLddFlag);
	if (!iTransport)
		User::Leave(KErrNoMemory);

	iProtocol = CScsiProtocol::NewL(*iDriveManager);

	//Create and start server
	__PRINT(_L("CUsbMassStorageController::CreateL: Creating server"));
	iServer = CUsbMassStorageServer::NewLC(*this);
	CleanupStack::Pop(iServer);
	__PRINT(_L("CUsbMassStorageController::CreateL Out"));
	}

/**
Returns a reference to the drive manager

@return A reference to the drive manager
*/
CDriveManager& CUsbMassStorageController::DriveManager()
	{
	return *iDriveManager;
	}


void CUsbMassStorageController::GetTransport(MTransportBase* &aTransport)
	{
	aTransport = iTransport; 
	}

/**
Starts the transport and initializes the protocol.

@param aConfig Reference to Mass Storage configuration data
*/
TInt CUsbMassStorageController::Start(TMassStorageConfig& aConfig)
	{
	__PRINT(_L("CUsbMassStorageController::Start In"));
	//Save this value for use in the Reset method.
	iConfig = aConfig;

    __ASSERT_DEBUG(iTransport, User::Invariant());
	if ((iTransport->InitialiseTransportL((TInt) iTransportLddFlag)) != KErrNone)
		{
		iTransportLddFlag = EUsbc; // If SC Ldd not present use the default USB Client Ldd
		delete iTransport;
		iTransport = CBulkOnlyTransport::NewL(iMaxDrives, *this, iTransportLddFlag);
		if (!iTransport)
			User::Leave(KErrNoMemory);
		if ((iTransport->InitialiseTransportL((TInt) iTransportLddFlag)) != KErrNone)
			return KErrNotFound;
		}

	TInt err = KErrNotReady;

	if (iProtocol && iTransport)
		{
		iTransport->RegisterProtocol(*iProtocol);
		iProtocol->RegisterTransport(iTransport);
		__PRINT(_L("CUsbMassStorageController::Start: Starting"));
		((CScsiProtocol*)iProtocol)->SetScsiParameters(aConfig);
		err = iTransport->Start();
		}

	__PRINT(_L("CUsbMassStorageController::Start Out"));

	return err;
	}

/**
Stops the transport.
*/
TInt CUsbMassStorageController::Stop()
	{

	__PRINT(_L("CUsbMassStorageController::Stop In"));
	TInt err = KErrNotReady;
	if (iTransport)
		{
		__PRINT(_L("CUsbMassStorageController::Stop: Stopping"));
		err = iTransport->Stop();
		}
	TInt i=0;
	for (i=0; i<=iMaxDrives; ++i)
		{
		iDriveManager->SetCritical(i, EFalse);   //unset critical
		}
	__PRINT(_L("CUsbMassStorageController::Stop Out"));

	return err;
	}

/**
Delete the transport and protocol and start new ones.
*/
void CUsbMassStorageController::Reset()
	{

	delete iProtocol;
	iProtocol = NULL;

	//Create transport and protocol and initialize them
	__PRINT(_L("CUsbMassStorageController::Reset: Creating  protocol"));

	TRAPD(err,iProtocol = CScsiProtocol::NewL(*iDriveManager));
	err = err;
	__ASSERT_DEBUG(err==KErrNone, User::Invariant());
	iTransport->RegisterProtocol(*iProtocol);
	iProtocol->RegisterTransport(iTransport);
	}

