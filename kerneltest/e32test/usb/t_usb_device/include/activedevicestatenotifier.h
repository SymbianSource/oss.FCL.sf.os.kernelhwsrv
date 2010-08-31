// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/include/activedevicestatenotifier.h
// 
//

#ifndef __ACTIVEDEVICESTATENOTIFIER_H__
#define __ACTIVEDEVICESTATENOTIFIER_H__

#include "general.h"

class CActiveDeviceStateNotifier : public CActive
	{
public:
	static CActiveDeviceStateNotifier* NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort, TUint aPortNumber);
	~CActiveDeviceStateNotifier();
	void Activate();

private:
	CActiveDeviceStateNotifier(CConsoleBase* aConsole, RDEVCLIENT* aPort, TUint aPortNumber);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDEVCLIENT* iPort;
	TUint iDeviceState;
	TUint iPortNumber; 
	};

#endif	// __ACTIVEDEVICESTATENOTIFIER_H__
