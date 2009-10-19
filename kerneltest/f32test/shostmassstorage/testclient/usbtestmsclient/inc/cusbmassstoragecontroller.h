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
// Class declaration for CUsbMassStorageController.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef CUSBMASSSTORAGECONTROLLER_H
#define CUSBMASSSTORAGECONTROLLER_H

class CDriveManager;
class CUsbMassStorageServer;
class MServerProtocol;
class MDeviceTransport;
class TTestParser;


/**
Mass Storage Controller class.
Encapsulates the drive manager, transport and protocol for USB Mass Storage.
Its main purpose is to instantiate and initialize these objects.
*/
class CUsbMassStorageController : public CBase
	{
public:
    static CUsbMassStorageController* NewL();
	~CUsbMassStorageController();
private:
    CUsbMassStorageController();
	void  ConstructL();

public:
	void CreateL(const TLunToDriveMap& aDriveMapping);
	CDriveManager& DriveManager();
	TInt Start(const TMassStorageConfig& aConfig);
	TInt Stop();
	void Reset();

private:
	CDriveManager* iDriveManager;
	MDeviceTransport* iTransport;
	MServerProtocol* iProtocol;
	CUsbMassStorageServer* iServer;
	TMassStorageConfig iConfig;
#ifdef MSDC_TESTMODE
    TTestParser* iTestParser;
#endif
	};

#endif // CUSBMASSSTORAGECONTROLLER_H
