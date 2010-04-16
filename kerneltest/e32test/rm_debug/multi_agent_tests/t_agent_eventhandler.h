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
// Definitions for event handling via the DSS and target specific information
//
//

#ifndef RMDEBUG_AGENT_EVENTHANDLER_H
#define RMDEBUG_AGENT_EVENTHANDLER_H

#include "t_multi_agent.h"

using namespace Debug;

class CMultiAgent;

/**
 Class for gathering event data from the run-mode driver
 */
class TAgentEventInfo
{
public:
	TAgentEventInfo() : 
	iEventInfoBuf((TUint8*)&iEventInfo, sizeof(TEventInfo), sizeof(TEventInfo))
	{ 	
	}

public:
	// This is the underlying class for event interaction with the Run Mode debug API 
	TEventInfo			iEventInfo;
	
	// A convenience handle for iEventInfo used across the Debug::GetEvent() method 
	TPtr8				iEventInfoBuf;
};

/**
  Active object class used to trap asynchronous events
  Also, contains target specific parameters
  */
class CAgentAsyncEvent : public CActive
	{
public:
	// Close buffers and Cancel and destroy
	~CAgentAsyncEvent();	

	// Two-phased constructor
	static CAgentAsyncEvent* NewLC(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig);

	// Two-phased constructor
	static CAgentAsyncEvent* NewL(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig);

	// set up ASP and issue another request	
	void Watch();

	TDesC& GetExecutable() { return iExeName; }
 	TDesC& GetExeConfig() { return iExeConfig; }
	RProcess& GetProcHandle() { return iProc; }

protected:
	// from CActive
	virtual void RunL();
	virtual void DoCancel();
	virtual TInt RunError(TInt aError);

private:
	CAgentAsyncEvent(CMultiAgent& aDriver, const TDesC& aExeName, const TDesC& aExeConfig);
	void ConstructL(const TDesC& aExeName, const TDesC& aExeConfig);

private:
	RBuf iExeName;
	RBuf iExeConfig;
	RProcess iProc;

	CMultiAgent& iDriver;
	TAgentEventInfo iSEventInfo;
	};

#endif // RMDEBUG_AGENT_EVENTHANDLER_H


