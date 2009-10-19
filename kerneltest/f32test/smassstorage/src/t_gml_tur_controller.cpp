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

#include "e32base.h"
#include "e32base_private.h"
#include "t_gml_tur_controller.h"

/**
 Stub MS controller for bulk only transport testing.  
*/

CUsbMassStorageController::CUsbMassStorageController() : CActive(EPriorityStandard)
	{
	}

CUsbMassStorageController::~CUsbMassStorageController()
	{
	}
	
void CUsbMassStorageController::CreateL(TUint /*aMaxDrives*/)
	{
	CActiveScheduler::Add(this);
	iStatus = KRequestPending;
	SetActive();
	}
	
TInt CUsbMassStorageController::Start(TMassStorageConfig& /* aConfig */)
	{
	return 0;
	}
	
TInt CUsbMassStorageController::Stop()
	{
	return 0;
	}
	
void CUsbMassStorageController::Reset()
	{
	iReset = ETrue;
	}

void CUsbMassStorageController::GetTransport(MTransportBase* &aTransport)
	{
	aTransport = iTransport;
	}

void CUsbMassStorageController::SetTransport(MTransportBase* aTransport)
	{
	iTransport = aTransport;
	}

TBool CUsbMassStorageController::IsReset()
	{
	TBool r = iReset;
	iReset = EFalse;
	return r;
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

