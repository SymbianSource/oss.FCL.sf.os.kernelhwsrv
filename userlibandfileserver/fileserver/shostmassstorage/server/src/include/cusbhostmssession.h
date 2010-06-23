// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// cusbmassstoragesession.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef CUSBMASSSTORAGESESSION_H
#define CUSBMASSSTORAGESESSION_H


_LIT(KUsbHostMsSrvPncCat, "UsbHosMsServer");
enum TUsbMsPanicClient
    {
    EUsbMsPanicIllegalIPC
    };

//
// Forward declarations
//
class CUsbHostMsServer;
class CUsbHostMsDeviceThread;

/**
 The CUsbMassStorageSession class
 Implements a Session of a Symbian OS server for the RUsbMassStorage API
 */
class CUsbHostMsSession : public CSession2
	{
public:
	static CUsbHostMsSession* NewL(CUsbHostMsServer& aServer);
	virtual ~CUsbHostMsSession();

	// CSession
	virtual void ServiceL(const RMessage2& aMessage);

    void MessageRequest(TRequestStatus& aStatus);

protected:
	CUsbHostMsSession(CUsbHostMsServer& aServer);
	void ConstructL();
	void CreateDeviceThreadL(const RMessage2& aMessage);

	void DispatchMessageL(const RMessage2& aMessage);

private:
	CUsbHostMsServer& iUsbHostMsServer;
	CUsbHostMsDeviceThread* iDeviceThread;
	RThread	iThread;

	TBool iCleanupInProgress;
    TRequestStatus* iClientStatus;
	};

#endif //__CUSBMASSSTORAGESESSION_H__
