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
// Definitions for the run mode debug tests
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef RMDEBUG_MULTI_AGENT_H
#define RMDEBUG_MULTI_AGENT_H

#include <u32hal.h>
#include <f32file.h>
#include <rm_debug_api.h>

using namespace Debug;

_LIT(KLaunchSemaphoreName, "t_rmdebug_launch_semaphore");
_LIT(KLaunchSemaphoreSearchString, "t_rmdebug_launch_semaphore*");

// Currently the targets are instances of t_rmdebug_app.exe
_LIT(KTargetExe,"z:\\sys\\bin\\t_rmdebug_app%d.exe");

_LIT(KTargetOptions,"-f%d -a%d");

// If changing this, make sure there are enough apps built/in the rom 
const TInt KNumApps = 5;

// Default CPU execution for Agent
const TInt KAgentCpu = 0;

// Workaround to ensure we have the same agent binary when running multiple agents
const TInt KTargetOffset = 0;

class CAgentAsyncEvent;

/**
  @Class CRunModeAgent
  
  The basic run mode agent
  */
class CMultiAgent : public CBase
	{
	public:
	static CMultiAgent* NewL();
	~CMultiAgent();
	void ClientAppL();  
	RSecuritySvrSession&  DebugDriver() { return iServSession; };	
	void HandleEvent(TEventInfo& aSEventInfo);

	public:
	TInt GetLaunchCompleted() const { return iLaunchCompleted; }
	TInt GetNumApps() const { return iNumApps; }
	TInt GetTargetOffset() const { return iTargetNameOffset; }

	private:
	CMultiAgent();
	void ConstructL();
	TInt StartTest();
	TInt LaunchProcess(RProcess& aProcess, const TDesC& aExeName, const TDesC& aCommandLine);
	
	private:

	/**
	 * CPU agent executes on; by default this is 0 
	 */
	TInt iAgentCpuNo;

	/*
	 * Offset for running multiple targets using the same agent
	 */
	TInt iTargetNameOffset;

	/** 
	 * Number of applications/targets per agent
	 */
	TInt iNumApps;

	/**
	 * Flag used for terminating the event handling for a target 
	 */	
	TInt iLaunchCompleted;

	/*
	 * Handle to DSS
	 */
	RSecuritySvrSession iServSession;
	
	/**
	 * Array to target parameters required by the agent
	 */	
	RPointerArray<CAgentAsyncEvent> iTargetList;
	};

#endif // RMDEBUG_MULTI_AGENT_H
