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
// Asynchronous security server active object class.
// 
//

#include <e32base.h>
#include <e32base_private.h>

#include "c_security_svr_async.h"
#include "rm_debug_logging.h"

using namespace Debug;

// ctor
CSecuritySvrAsync::CSecuritySvrAsync(CSecuritySvrSession* aSession, TProcessId aAgentId)
	: CActive(CActive::EPriorityStandard),
	iSession(aSession),
	iProcessName(NULL),
	iAgentId(aAgentId),
	iEventBalance(0)
	{
	LOG_MSG("CSecuritySvrAsync::CSecuritySvrAsync()");
	CActiveScheduler::Add(this);
	}

// returns a CSecuritySvrAsync active object associated with
// the specified agent and debugged process.
CSecuritySvrAsync* CSecuritySvrAsync::NewL(CSecuritySvrSession* aSession, const TDesC8& aProcessName, TProcessId aAgentId)
	{
	LOG_MSG("CSecuritySvrAsync::NewL()");
	CSecuritySvrAsync* me = new (ELeave) CSecuritySvrAsync(aSession, aAgentId);

	CleanupStack::PushL(me);

	me->ConstructL(aProcessName);

	CleanupStack::Pop(me);

	return (me);
	}

// dtor
CSecuritySvrAsync::~CSecuritySvrAsync()
	{
	LOG_MSG("CSecuritySvrAsync::~CSecuritySvrAsync()");

	// NOTE: the Cancel() function calls DoCancel() which may rely on class members so be careful not
	// to destroy/close data members before this call if they are needed
	Cancel();
	iProcessName.Close();
	}

// Associates the agent id and process name with the Active Object being constructed
void CSecuritySvrAsync::ConstructL(const TDesC8& aProcessName)
	{
	LOG_MSG("CSecuritySvrAsync::ConstructL()");
	iProcessName.CreateL(aProcessName.Length());
	iProcessName.Copy(aProcessName);
	}

// RunL() completes a previously issued call (currently only GetEvent() completion)
void CSecuritySvrAsync::RunL()
	{

	LOG_MSG3("CSecuritySvrAsync::RunL() &iInfo=0x%08x, iEventBalance=%d", (TUint8*)&iInfo, iEventBalance);

	// Something bad happened in the driver
	User::LeaveIfError(iStatus.Int());

	// Write back the event data to the debug agent.
	// For compatibility we need to check the size of the buffer that the
	// client has passed in as the size of TEventInfo will increase over time.
	// Clients can calculate the required size from the EApiConstantsTEventInfoSize entry 
	// in the Debug Functionality block but may still pass in buffers which
	// are smaller than the Debug Security Server's calculation of sizeof(TEventInfo), 
	// returning KErrTooBig in this case would be
	// inappropriate as we would break compatibility.
	TInt dataLengthToReturn = sizeof(TEventInfo);
	TInt maxLengthClientSide = iMessage.GetDesMaxLengthL(1);
	if(maxLengthClientSide < dataLengthToReturn)
		{
		dataLengthToReturn = maxLengthClientSide;
		}

	TPtr8 data((TUint8*)&iInfo,dataLengthToReturn,dataLengthToReturn);

	iMessage.WriteL(1,data,0);

	iMessage.Complete(KErrNone);
	--iEventBalance;
	}

// Cancels the oustanding GetEvent call. May cope with other async calls in future.
void CSecuritySvrAsync::DoCancel()
	{
	LOG_MSG2("CSecuritySvrAsync::DoCancel() iEventBalance=%d", iEventBalance);
	iSession->Server().iKernelDriver.CancelGetEvent(iProcessName,iAgentId.Id());

	iMessage.Complete(KErrCancel);
	iEventBalance=0;
	}

// Report any leave to the client if possible.
TInt CSecuritySvrAsync::RunError(TInt aError)
	{
	LOG_MSG2("CSecuritySvrAsync::RunError()=%d", aError);
	iMessage.Complete(aError);

	return KErrNone;
	}

/*
 * Start an asynchronous GetEvent call to the debug driver
 * and activates this active object. 
 */
void CSecuritySvrAsync::GetEvent(const RMessage2& aMessage)
	{
	iMessage = aMessage;

	iEventBalance++;
	LOG_MSG5("CSecuritySvrAsync::GetEvent() this = 0x%08x, iInfo=0x%08x, iStatus=0x%08x \
		iEventBalance=%d : >SetActive() > GetEvent() ",
		this, &iInfo, &iStatus, iEventBalance );

	/* 
	SetActive is called before sending the message to the driver so 
	that we do not get stray signal panics, since the driver may complete immediately
 	*/
	SetActive();
	iSession->Server().iKernelDriver.GetEvent(iProcessName,iAgentId.Id(),iStatus,iInfo);
	}

// Used for identifying which AO is associated with a debugged process
const TDesC8& CSecuritySvrAsync::ProcessName(void)
	{
	return iProcessName;
	}

// End of file - c_security_svr_async.cpp

