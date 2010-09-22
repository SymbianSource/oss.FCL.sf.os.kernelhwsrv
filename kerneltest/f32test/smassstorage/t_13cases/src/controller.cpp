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
// 


#include "protocol.h"
#include "cusbmassstoragecontroller.h"
#include "cbulkonlytransport.h"

/**
 Stub MS controller for bulk only transport testing.  
*/

CUsbMassStorageController::CUsbMassStorageController() : CActive(EPriorityStandard)
	{
	}
	
CUsbMassStorageController::~CUsbMassStorageController()
	{
	delete iProtocol;
	delete iTransport;
	}
	
void CUsbMassStorageController::CreateL(TUint aMaxDrives)
	{
	iMaxDrives = aMaxDrives;
	iStatus = KRequestPending;
	CActiveScheduler::Add(this);
	SetActive();
	}
	
TInt CUsbMassStorageController::Start()
	{
	TInt err;
	TRAP(err, iProtocol = CScsiProtocol::NewL());
	if (err != KErrNone)
		{
		return err;
		}
		
	TRAP(err, iTransport = CBulkOnlyTransport::NewL(iMaxDrives, *this));
	if (err != KErrNone)
		{
		return err;
		}

    TRAP(err, iTransport->InitialiseTransportL(1));
		
	iTransport->RegisterProtocol(*iProtocol);
	iProtocol->RegisterTransport(iTransport);
	
	iTransport->Start();
	
	return KErrNone;
	}
	
TInt CUsbMassStorageController::Stop()
	{
	return 0;
	}
	
void CUsbMassStorageController::Reset()
	{
	delete iProtocol;
	TRAPD(err, iProtocol = CScsiProtocol::NewL());
	__ASSERT_ALWAYS(err == KErrNone, User::Invariant());
	iTransport->RegisterProtocol(*iProtocol);
	iProtocol->RegisterTransport(iTransport);
	}


void CUsbMassStorageController::GetTransport(MTransportBase* &aTransport)
	{
	aTransport = iTransport;
	}

void CUsbMassStorageController::SetTransport(MTransportBase* aTransport)
	{
	iTransport = aTransport;
	}


void CUsbMassStorageController::RunL()
	{
	CActiveScheduler::Stop();
	}
	
void CUsbMassStorageController::DoCancel()
	{
	}

CDriveManager& CUsbMassStorageController::DriveManager()
	{
	return iStubDriveManager;
	}

TInt CDriveManager::Connect(TUint /*aLun*/)
	{
	return KErrNone;
	}

TInt CDriveManager::Disconnect(TUint /*aLun*/)
	{
	return KErrNone;
	}
