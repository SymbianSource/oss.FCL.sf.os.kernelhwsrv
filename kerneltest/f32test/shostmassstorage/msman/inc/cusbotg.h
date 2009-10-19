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


#ifndef CUSBOTG_H
#define CUSBOTG_H

class CUsbOtgEventWatcher;
class CRequestSessionWatcher;

class MUsbRequestSessionObserver
    {
public:
    virtual void BusRequestL() = 0;
    };


class ROtgStateChangeNotifier
    {
public:
    ROtgStateChangeNotifier();
    ~ROtgStateChangeNotifier();

	void Register(const RMessage2& aMessage);
    void DoNotifyL();
    void DoCancelL();

private:
	void CompleteNotifierL(TInt);

private:
    /** Notification service */
	RMessage2 iMessage;
    /** Flag to indicate that media change notification is active */
	TBool iRegistered;
    };





class CUsbOtg: public CBase,
               public MUsbRequestSessionObserver
    {
public:
    enum TOtgState {KOtgStateStart,
                    KOtgStateAPlugInserted,
                    KOtgStateSessionOpen,
                    KOtgStateSessionClose};


public:
	static CUsbOtg* NewL();
	~CUsbOtg();
protected:
	CUsbOtg();
	void ConstructL();

public:

    // MUsbRequestSessionObserver
    void BusRequestL();

    void HandleUsbOtgEvent(RUsbOtgDriver::TOtgEvent aEvent);

    TBool DeviceInserted();
    void NotifyChange(const RMessage2& aMessage);
    void NotifyChangeCancel();
    TInt BusDrop();

private:
    void StartL();
    void Stop();


private:
    TOtgState iOtgState;
	RUsbOtgDriver iUsbOtgDriver;

    CUsbOtgEventWatcher* iOtgEventWatcher;

    CRequestSessionWatcher* iRequestSessionWatcher;

    ROtgStateChangeNotifier iNotifier;
    };

#endif
