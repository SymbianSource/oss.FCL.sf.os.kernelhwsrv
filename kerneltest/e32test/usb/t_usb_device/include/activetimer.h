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
// e32test/usb/t_usb_device/include/activetimer.h
// 
//

#ifndef __ACTIVETIMER_H__
#define __ACTIVETIMER_H__

class CActiveTimer : public CActive
	{
public:
	static CActiveTimer* NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort);
	~CActiveTimer();
	void Activate(TTimeIntervalMicroSeconds32 aDelay);

private:
	CActiveTimer(CConsoleBase* aConsole, RDEVCLIENT* aPort);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDEVCLIENT* iPort;
	RTimer iTimer;
	};

#endif	// __ACTIVETIMER_H__
