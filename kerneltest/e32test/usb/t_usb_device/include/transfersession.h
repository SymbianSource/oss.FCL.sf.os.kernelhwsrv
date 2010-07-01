/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Implements a Session of a Symbian OS server for the RUsb API
* 
*
*/



/**
 @file
*/

#ifndef __CTRANSFERSESSION_H__
#define __CTRANSFERSESSION_H__

#include <e32std.h>
#include "general.h"

class CTransferServer;
class CTransferHandle;

NONSHARABLE_CLASS(CTransferSession) : public CSession2
	{
public:
	static CTransferSession* NewL(CTransferServer* aServer);
	virtual ~CTransferSession();

	// CSession2
	virtual void ServiceL(const RMessage2& aMessage);
	virtual void CreateL();
	void TransferHandleL();

protected:
	CTransferSession(CTransferServer* aServer);

	void DispatchMessageL(const RMessage2& aMessage);

private:
	CTransferServer* iTransferServer;
	CConsoleBase* iConsole;
		
	};

#endif //__CTRANSFERSESSION_H__
