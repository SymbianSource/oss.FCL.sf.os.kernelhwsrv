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


/** @file
@internalTechnology
Class declaration for CUsbMassStorageController.
*/
 
#ifndef __CUSBMASSSTORAGECONTROLLER_H__
#define __CUSBMASSSTORAGECONTROLLER_H__

#include "cusbmassstorageserver.h"
#include "t_13cases_protocol.h"
#include "usbmsshared.h"

class CBulkOnlyTransport;

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
Stub Mass Storage Controller class for bulk-only transport testing.
*/
class CUsbMassStorageController : public CActive
	{
public:
	CUsbMassStorageController();
	~CUsbMassStorageController();
	void CreateL(TUint aMaxDrives);
	TInt Start();
	TInt Stop();
	void Reset();
	void RunL();
	void DoCancel();
	CDriveManager& DriveManager();
	void GetTransport(MTransportBase* &aTransport);
	void SetTransport(MTransportBase* aTransport);
    enum TTransportldd {EUsbc = 1, EUsbcsc};
	
private:
	TInt iMaxDrives;
	CScsiProtocol* iProtocol;
	MTransportBase* iTransport;
	CDriveManager iStubDriveManager;
	};

#endif //__CUSBMASSSTORAGECONTROLLER_H__
