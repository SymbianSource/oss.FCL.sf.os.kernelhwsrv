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


#ifndef CUSBOTGWATCHER_H
#define CUSBOTGWATCHER_H


class CUsbOtg;

class CUsbOtgBaseWatcher : public CActive
	{
public:
	CUsbOtgBaseWatcher(RUsbOtgDriver& aLdd);
	virtual ~CUsbOtgBaseWatcher();

	// From CActive
	virtual void RunL() = 0;
	virtual void DoCancel() = 0;

	virtual void Start();

protected:
	virtual void Post() = 0;

protected:
	RUsbOtgDriver& iLdd;
	};




const TUid KUidUsbManCategory = {0x101fe1db};

enum TUsbManPropertyKeys
    {
    KUsbRequestSessionProperty = 6
    };

enum TUsbManSessionState
    {
    KUsbManSessionOpen = 0x1,
    KUsbManSessionClose
    };




class CRequestSessionWatcher: public CActive
    {
public:
    static CRequestSessionWatcher* NewL(MUsbRequestSessionObserver& aObserver);
    ~CRequestSessionWatcher();
private:
    CRequestSessionWatcher(MUsbRequestSessionObserver& aObserver);
    void ConstructL();

private:
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);
    //void StartRequestSession();

private:
    MUsbRequestSessionObserver& iObserver;
    RProperty iProperty;
    };



class CUsbOtgEventWatcher: public CUsbOtgBaseWatcher
    {
public:
    static CUsbOtgEventWatcher* NewL(RUsbOtgDriver& aLdd,
                                     CUsbOtg& iUsbOtg);
    ~CUsbOtgEventWatcher();
private:
    CUsbOtgEventWatcher(RUsbOtgDriver& aLdd,
                        CUsbOtg& iUsbOtg);
    void ConstructL();

protected:
    void Post();

private:
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

private:
    RUsbOtgDriver::TOtgEvent iEvent;

    CUsbOtg& iUsbOtg;
    };

#endif
