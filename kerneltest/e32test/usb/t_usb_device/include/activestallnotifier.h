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
// e32test/usb/t_usb_device/include/activestallnotifier.h
// 
//

#ifndef __ACTIVESTALLNOTIFIER_H__
#define __ACTIVESTALLNOTIFIER_H__

#include "general.h"

class CActiveStallNotifier : public CActive
	{
public:
	static CActiveStallNotifier* NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort);
	~CActiveStallNotifier();
	void Activate();

private:
	CActiveStallNotifier(CConsoleBase* aConsole, RDEVCLIENT* aPort);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDEVCLIENT* iPort;
	TUint iEndpointState;
	};


#endif	// __ACTIVESTALLNOTIFIER_H__
