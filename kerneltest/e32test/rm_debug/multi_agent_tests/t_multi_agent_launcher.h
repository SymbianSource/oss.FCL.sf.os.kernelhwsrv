// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definitions for agent launcher
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef RMDEBUG_MULTI_AGENT_LAUNCHER_H
#define RMDEBUG_MULTI_AGENT_LAUNCHER_H

#include <e32test.h>

// Default test runs
const TInt KNumTestRuns = 1; 

// Default number of targets per agent
const TInt KNumTargets = 5;

// Default number of agents, if changing this make sure there are enough apps being built 
const TInt KNumAgents = 2; 

_LIT(KAgentExe,"z:\\sys\\bin\\t_rmdebug_multi_agent.exe");
_LIT(KAgentOptions,"-n%d -o%d -a%d");

_LIT(KLaunchSemaphoreName, "t_rmdebug_launch_semaphore");
_LIT(KLaunchSemaphoreSearchString, "t_rmdebug_launch_semaphore*");

#endif // RMDEBUG_MULTI_AGENT_LAUNCHER_H

