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
#include "t_gml_tur_protocol.h"
#include "usbmsshared.h"

/**
Stub drive manager for bulk-only transport testing.
*/
class CDriveManager
	{
public:
	TInt Connect(TUint aLun);
	TInt Disconnect(TUint aLun);
	};

/**
Mass Storage Controller class.
Encapsulates the drive manager, transport and protocol for USB Mass Storage.
Its main purpose is to instantiate and initialize these objects.
*/
class CUsbMassStorageController : public CActive
	{
public:
	CUsbMassStorageController();
	~CUsbMassStorageController();
	void CreateL(TUint aMaxDrives);
	TInt Start(TMassStorageConfig& aConfig);
	TInt Stop();
	void Reset();
	TBool IsReset();
	void RunL();
	void DoCancel();
	CDriveManager& DriveManager();
	void GetTransport(MTransportBase* &aTransport);
	void SetTransport(MTransportBase* aTransport);
	enum TTransportldd {EUsbc = 1, EUsbcsc};

private:
	TBool iReset;
	CDriveManager iStubDriveManager;
	MTransportBase* iTransport;
	};

#endif //__CUSBMASSSTORAGECONTROLLER_H__
