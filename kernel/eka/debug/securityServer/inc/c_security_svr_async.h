// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Asynchronous security server responder active object class.
// 
//

#ifndef C_SECURITY_SVR_ASYNC_H
#define C_SECURITY_SVR_ASYNC_H

#include <rm_debug_api.h>

#include "c_security_svr_session.h"
#include "c_security_svr_server.h"

// forward declaration
class CSecuritySvrSession;

/**
Class used to handle asynchronous events within a DSS session. Currently this
is only used to handle GetEvent() calls. It sets up an active object when a 
client makes a GetEvent() call, and completes it when ready, or cancels it
if the client so wishes.

Only one outstanding active object per client session is permitted.
*/
class CSecuritySvrAsync : public CActive
{
public:
	~CSecuritySvrAsync();
	static CSecuritySvrAsync* NewL(CSecuritySvrSession* aSession, const TDesC8& aProcessName, TProcessId aAgentId);

	void GetEvent(const RMessage2& aMessage);
	const TDesC8& ProcessName(void);

protected:
	CSecuritySvrAsync(CSecuritySvrSession* aSession, TProcessId aAgentId);

	void ConstructL(const TDesC8& aProcessName);

	virtual void RunL();
	virtual void DoCancel();
	virtual TInt RunError(TInt aError);

private:

    /*
     * The last GetEvent message details. Needed for completion by RunL()
     */
    RMessagePtr2 iMessage;			

    /*
     * Temporary storage area for rm_debug.ldd to return data asynchronously
     */
	Debug::TEventInfo iInfo;

	/*
	 * Identity of this server session. Used for completing iMessage
	 */
	CSecuritySvrSession* iSession;

	/*
	 * Name of the process being debugged associated with this AO
	 */
	RBuf8 iProcessName;

	/*
	 * Debug Agent Id
	 */
	TProcessId iAgentId;
	
	/*
	 * Balance between event requests and event deliveries.
	 * @see GetEvent()
	 * @see NotifyEvent()
	 * @see DoCancel()
	 */
	TInt iEventBalance;
};
 
#endif	// C_SECURITY_SVR_ASYNC_H
