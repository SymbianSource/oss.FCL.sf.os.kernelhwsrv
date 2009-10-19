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
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef CUSBOTGSESSION_H
#define CUSBOTGSESSION_H


_LIT(KUsbOtgClientPncCat, "UsbOtgServer");

class CUsbOtgServer;

enum TMsManPanicClient
    {
    EUsbOtgPanicIllegalIPC
    };


class CUsbOtgSession : public CSession2
	{
public:
	static CUsbOtgSession* NewL();
    virtual void CreateL();

protected:
	CUsbOtgSession();
	void ConstructL();

private:
	~CUsbOtgSession();


public:
	// CSession2
	void ServiceL(const RMessage2& aMessage);
    void DispatchMessageL(const RMessage2& aMessage);

private:
    // Services
    void DeviceInsertedL(const RMessage2& aMessage);
	void NotifyChange(const RMessage2& aMessage);
	void NotifyChangeCancel();
    TInt BusDrop();

protected:
    // panic the client
    void PanicClient(const RMessage2& aMessage,TInt aPanic) const;

private:
    CUsbOtgServer& Server();
	};


inline CUsbOtgServer& CUsbOtgSession::Server()
    {
    return *static_cast<CUsbOtgServer*>(const_cast<CServer2*>(CSession2::Server()));
    }


#endif // CUSBOTGSESSION_H
