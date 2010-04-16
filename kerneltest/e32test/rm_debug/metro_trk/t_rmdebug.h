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

#ifndef RMDEBUG_H
#define RMDEBUG_H

// Function codes (opcodes) used in message passing between client and server
enum TRunModeAgentState
	{
	ERunModeAgentUnInit,
    ERunModeAgentRunning,
	};


//
// class CRunModeAgent
//
// The basic run mode agent.
//
class CRunModeAgent : public CBase
	{
	public:
		static CRunModeAgent* NewL();
		~CRunModeAgent();
		void ClientAppL();

	private:
		CRunModeAgent();
		void ConstructL();

		TInt TestStartup();
		TInt TestShutdown();

		void TestProcessList();
		void TestThreadList();
		void TestMemoryAccess();
		void TestSuspendResume();

	private:
		RDebugServSession	iServSession;
		RThread				iDebugThread;
		TInt				iProcessID;
		TInt				iThreadID;
        TInt                iState;
	};

#endif // RMDEBUG_H
