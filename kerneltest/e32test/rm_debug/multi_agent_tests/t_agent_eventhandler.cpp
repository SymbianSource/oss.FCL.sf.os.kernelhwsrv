// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the handling of run mode events for a particular target executable
// 

#include <e32base.h>
#include <e32property.h>
#include <e32test.h>

#include "t_rmdebug_app.h"
#include "t_agent_eventhandler.h"
#include "t_multi_agent.h"
#include "t_debug_logging.h" 

using namespace Debug;

CAgentAsyncEvent::CAgentAsyncEvent(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig):
	CActive(EPriorityStandard), iDriver(aDriver)
	{
	}

CAgentAsyncEvent* CAgentAsyncEvent::NewLC(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig)
	{
	CAgentAsyncEvent* self = new(ELeave) CAgentAsyncEvent(aDriver, aExeName, aExeConfig);
	CleanupStack::PushL(self);
	self->ConstructL(aExeName, aExeConfig);
	return self;
	}

CAgentAsyncEvent* CAgentAsyncEvent::NewL(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig)
	{
	CAgentAsyncEvent* self = CAgentAsyncEvent::NewLC(aDriver, aExeName, aExeConfig);
	CleanupStack::Pop(); // self
	return self;
	}

void CAgentAsyncEvent::ConstructL(const TDesC& aExeName, const TDesC& aExeConfig)
	{
	iExeName.CreateL(aExeName);
	iExeConfig.CreateL(aExeConfig);
	CActiveScheduler::Add(this);
	}

CAgentAsyncEvent::~CAgentAsyncEvent()
	{
	LOG_MSG2("~CAgentAsyncEvent(), this = 0x%08x", this);

	iSEventInfo.iEventInfoBuf.Delete(0, sizeof(TEventInfo));
	iExeName.Close();
	iExeConfig.Close();
	iProc.Close();
	Cancel(); // Cancel any request, if outstanding
	}

/*
 * Issue request to DSS and notify the active scheduler
 */
void CAgentAsyncEvent::Watch()
	{
	LOG_MSG2("ENTER: CAgentAsyncEvent::Watch, this = 0x%08x", this);
	iDriver.DebugDriver().GetEvent(GetExecutable(), iStatus, iSEventInfo.iEventInfoBuf);

	if (!IsActive())
		{
		LOG_MSG("CAgentAsyncEvent::Watch(): SetActive()");
		SetActive();
		}

	LOG_MSG("EXIT: CAgentAsyncEvent::Watch");
	}

void CAgentAsyncEvent::RunL()
	{
	LOG_MSG4("ENTER: CAgentAsyncEvent::RunL iDebugType=%d, iStatus.Int() %d, this 0x%x08", 
			 iSEventInfo.iEventInfo.iEventType, iStatus.Int(), this);
	
    LOG_MSG2("%S", &TPtr8((TUint8*)GetExecutable().Ptr(), 2*GetExecutable().Length(), 2*GetExecutable().Length()));	
	iDriver.HandleEvent(iSEventInfo.iEventInfo);

	LOG_MSG2("iDriver.GetNumApps() %d: ", iDriver.GetNumApps());
	LOG_MSG2("iDriver.iLaunchCompleted  %d: ", iDriver.GetLaunchCompleted());

	if (iDriver.GetLaunchCompleted() < iDriver.GetNumApps())
		{
		// Do not call Watch() if target has run to completion but test is still on going
		if (iSEventInfo.iEventInfo.iEventType != EEventsRemoveProcess)
			{
			LOG_MSG("CAgentAsyncEvent::RunL Setting Watch()");
			Watch();
			}
		}
	else
		{
		// Stop event handling for all targets
		LOG_MSG("CAgentAsyncEvent::RunL CActiveScheduler::Stop() & Cancel");
		CActiveScheduler::Stop();
		}

	LOG_MSG2("EXIT: CAgentAsyncEvent::RunL", KNullDesC);
	}

void CAgentAsyncEvent::DoCancel()
	{
	LOG_MSG("CAgentAsyncEvent::DoCancel");
	}

TInt CAgentAsyncEvent::RunError(TInt aError)
	{
	LOG_MSG3(" RunL() has left with error %d, this 0x%08X", aError, this);
	return aError; 
	// Can we handle this error? Not at the moment!
	}

