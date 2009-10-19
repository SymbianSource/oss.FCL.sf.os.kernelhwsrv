// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __CUSBMASSSTORAGESESSION_H__
#define __CUSBMASSSTORAGESESSION_H__

#include <e32std.h>
#include <e32base.h>
#include "massstoragedebug.h"
#include "usbmsshared.h"

_LIT(KUsbMsCliPncCat, "UsbMs-Client");

enum TUsbMsPanicClient
    {
    EUsbMsPanicIllegalIPC
    };
//
// Forward declarations
//
class CUsbMassStorageServer;

/**
 The CUsbMassStorageSession class
 Implements a Session of a Symbian OS server for the RUsbMassStorage API
 */
class CUsbMassStorageSession : public CSession2
	{
public:
	static CUsbMassStorageSession* NewL(CUsbMassStorageServer& aServer);
	virtual ~CUsbMassStorageSession();

	// CSession
	virtual void ServiceL(const RMessage2& aMessage);

protected:
	CUsbMassStorageSession(CUsbMassStorageServer& aServer);
	void ConstructL();

	void DispatchMessageL(const RMessage2& aMessage);
	TInt Start(const RMessage2& aMessage);
	TInt Stop();
	TInt Shutdown();

private:
 	void GetMsConfigL(const RMessage2& aMessage, TMassStorageConfig& aMsStorage);
	
private:
	CUsbMassStorageServer& iUsbMsServer;
	};

#endif //__CUSBMASSSTORAGESESSION_H__

