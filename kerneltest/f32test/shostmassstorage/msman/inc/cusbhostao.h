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
//


#ifndef CUSBHOSTAO_H
#define CUSBHOSTAO_H

class MUsbHostBusEventObserver
    {
public:
    virtual void ProcessBusEventL() = 0;
    };

class CUsbHostAo: public CActive
    {
public:
	static CUsbHostAo* NewL(RUsbHubDriver& aHubDriver,
                            RUsbHubDriver::TBusEvent& aEvent,
                            MUsbHostBusEventObserver& aObserver);
	~CUsbHostAo();
protected:
	CUsbHostAo(RUsbHubDriver& aHubDriver,
               RUsbHubDriver::TBusEvent& aEvent,
               MUsbHostBusEventObserver& aObserver);
	void ConstructL();

public:
    void Wait();

protected:
    virtual void DoCancel();
    virtual void RunL();
	virtual TInt RunError(TInt aError);

protected:
    RUsbHubDriver& iHubDriver;
    RUsbHubDriver::TBusEvent& iEvent;

    MUsbHostBusEventObserver& iObserver;
    };




#endif


