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

#ifndef RMDEBUG_MULTI_TARGET_H
#define RMDEBUG_MULTI_TARGET_H

#include "t_rmdebug_app.h"

#include <rm_debug_api.h>

class CMultiTargetAgent;

//
// class CRunModeAgent
//
// The basic run mode agent.
//
class CMultiTargetAgent : public CBase
	{
public:
	static CMultiTargetAgent* NewL();
	~CMultiTargetAgent();
	void ClientAppL();
	
    TInt LaunchProcess(RProcess& aProcess, TDesC & aExeName, TDesC & aCommandLine );
    
private:
	CMultiTargetAgent();
	void ConstructL();

	void ReportPerformance(void);

	TInt HelpTicksPerSecond(void);

	enum TTestMode 
		{
		//run all the tests
		EModeAll = 1<<0,
		//run the specified tests in reverse order
		EModeReverse = 1<<1,
		//print out help
		EModeHelp = 1<<2,
		//print out help
		EModeVersion = 1<<3
		};
	
	TInt LaunchTargetsInOrderL();
	void SetupDebugServerL();

private:

#if defined(KERNEL_OOM_TESTING)
	RKernelLowMemorySecuritySvrSession iServSession;
#elif defined (USER_OOM_TESTING)
	RUserLowMemorySecuritySvrSession iServSession;
#else
	Debug::RSecuritySvrSession iServSession;
#endif
	RSemaphore iAddressGlobSem;

	TUid iMySid;

	// Timing information
	TInt iStartTick;
	TInt iStopTick;

	RArray<RBuf> iTargets;
  TRequestStatus iStatus;
  Debug::TEventInfo iEventInfo;
  TPtr8 iEventPtr;
	};

#endif // RMDEBUG_MULTI_TARGET_H
