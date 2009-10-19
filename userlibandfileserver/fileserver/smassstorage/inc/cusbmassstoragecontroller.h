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
// Class declaration for CUsbMassStorageController.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __CUSBMASSSTORAGECONTROLLER_H__
#define __CUSBMASSSTORAGECONTROLLER_H__

#include <e32base.h>
#include <usbmsshared.h>
#include "drivemanager.h"
#include "cusbmassstorageserver.h"
#include "protocol.h"

/**
Mass Storage Controller class.
Encapsulates the drive manager, transport and protocol for USB Mass Storage.
Its main purpose is to instantiate and initialize these objects.
*/
class CUsbMassStorageController : public CBase
	{
	public:
	~CUsbMassStorageController();
	void CreateL(RArray<TInt>& aDriveMapping);
	CDriveManager& DriveManager();
	TInt Start(TMassStorageConfig& aConfig);
	TInt Stop();
	void Reset();
	void GetTransport(MTransportBase* &aTransport);
	enum TTransportldd {EUsbc = 1, EUsbcsc};
	
	private:
	CDriveManager* iDriveManager;
	MTransportBase* iTransport;
	MProtocolBase* iProtocol;
	CUsbMassStorageServer* iServer;
	TMassStorageConfig iConfig;
	TInt iMaxDrives;
	TTransportldd iTransportLddFlag;
	};

#endif //__CUSBMASSSTORAGECONTROLLER_H__
