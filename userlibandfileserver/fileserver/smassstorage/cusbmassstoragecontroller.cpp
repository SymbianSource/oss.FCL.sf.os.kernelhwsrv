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

#include <e32std.h>
#include "mtransport.h"
#include "mprotocol.h"

#include "scsiprot.h"
#include "cusbmassstorageserver.h"
#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"
#include "cbulkonlytransport.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cusbmassstoragecontrollerTraces.h"
#endif

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
    OstTraceFunctionEntry0(CUSBMASSSTORAGECONTROLLER_100);
#if !defined(__WINS__) && !defined(__X86__)
    iTransportLddFlag = EUsbcsc; // Create transport object using SC Ldd By default
#else
    iTransportLddFlag = EUsbc;
#endif
    //Save this value for use in the Reset method.
    iMaxDrives = aDriveMapping.Count();
    OstTrace1(TRACE_SMASSSTORAGE_FS, CUSBMASSSTORAGECONTROLLER_101,
              "MaxDrives = %d", iMaxDrives);
    //Create and init drive manager
    iDriveManager = CDriveManager::NewL(aDriveMapping);

    //Create transport and protocol and initialize them
    iTransport = CBulkOnlyTransport::NewL(iMaxDrives, *this, iTransportLddFlag);
    if (!iTransport)
        User::Leave(KErrNoMemory);

    iProtocol = CScsiProtocol::NewL(*iDriveManager);

    //Create and start server
    iServer = CUsbMassStorageServer::NewLC(*this);
    CleanupStack::Pop(iServer);
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
    OstTraceFunctionEntry0(CUSBMASSSTORAGECONTROLLER_200);
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
        ((CScsiProtocol*)iProtocol)->SetScsiParameters(aConfig);
        err = iTransport->Start();
        }

    return err;
    }

/**
Stops the transport.
*/
TInt CUsbMassStorageController::Stop()
    {
    OstTraceFunctionEntry0(CUSBMASSSTORAGECONTROLLER_110);
    TInt err = KErrNotReady;
    if (iTransport)
        {
        err = iTransport->Stop();
        }
    TInt i=0;
    for (i=0; i<=iMaxDrives; ++i)
        {
        iDriveManager->SetCritical(i, EFalse);   //unset critical
        }
    return err;
    }

/**
Delete the transport and protocol and start new ones.
*/
void CUsbMassStorageController::Reset()
    {
    OstTraceFunctionEntry0(CUSBMASSSTORAGECONTROLLER_120);
    delete iProtocol;
    iProtocol = NULL;

    //Create transport and protocol and initialize them
    TRAPD(err,iProtocol = CScsiProtocol::NewL(*iDriveManager));
    err = err;
    __ASSERT_DEBUG(err==KErrNone, User::Invariant());
    iTransport->RegisterProtocol(*iProtocol);
    iProtocol->RegisterTransport(iTransport);
    }

