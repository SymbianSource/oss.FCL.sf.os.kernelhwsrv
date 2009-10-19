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
// CUsbMassStorageSession.h
// Implements a Session of a Symbian OS server for the RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef CUSBHOSTSESSION_H
#define CUSBHOSTSESSION_H


_LIT(KUsbHostClientPncCat, "UsbHostClient");

enum TUsbHostPanicClient
    {
    EUsbHostPanicIllegalIPC
    };


class CUsbHostSession : public CSession2
	{
public:
	static CUsbHostSession* NewL();
    virtual void CreateL();

protected:
	CUsbHostSession();
	void ConstructL();

private:
	~CUsbHostSession();


public:
	// CSession2
	void ServiceL(const RMessage2& aMessage);
    void DispatchMessageL(const RMessage2& aMessage);

private:
    // Services
	TInt Start(const RMessage2& aMessage);

protected:
    // panic the client
    void PanicClient(const RMessage2& aMessage,TInt aPanic) const;

private:
    CUsbHostServer& Server();
	};


inline CUsbHostServer& CUsbHostSession::Server()
    {
    return *static_cast<CUsbHostServer*>(const_cast<CServer2*>(CSession2::Server()));
    }


#endif //CUSBHOSTSESSION_H
