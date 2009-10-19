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
// Definitions for the security server server.
// 
//

#ifndef C_SECURITY_SVR_SERVER_H
#define C_SECURITY_SVR_SERVER_H

/**
@file
@internalTechnology
@released
*/

#include <rm_debug_api.h>
#include "c_process_pair.h"
#include "c_shutdown_timer.h"
#include "rm_debug_kerneldriver.h"

_LIT(KDebugDriverFileName,"rm_debug.ldd");
class CSecuritySvrSession;

/**
Definition of a Debug Security Server. Responsible for managing all debug agent clients,
including attachment/detachment from target executables. Keeps track of which executables
are being debugged.
*/
class CSecuritySvrServer : public CServer2
	{
	public:
		CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
		TInt AttachProcessL(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId, const TBool aPassive);
		TInt DetachProcess(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId);
		void DetachAllProcesses(const TProcessId aDebugAgentProcessId);
		TBool CheckAttached(const TThreadId aTargetThreadId, const RMessage2& aMessage, const TBool aPassive);
		TBool CheckAttached(const TProcessId aTargetProcessId, const RMessage2& aMessage, const TBool aPassive);
		TBool CheckAttachedProcess(const TDesC& aTargetProcessName, const RMessage2& aMessage, const TBool aPassive) const;
		TBool IsDebugged(const TDesC& aTargetProcessName, const TBool aPassive) const;
		void SessionClosed();
		void SessionOpened();
		static CSecuritySvrServer* NewLC();

		TBool OEMTokenPermitsDebugL(const TCapabilitySet aTokenCaps, const TCapabilitySet aTargetCaps);
		TBool OEMTokenPermitsFlashAccessL(const TCapabilitySet aTokenCaps);

	protected:
		CSecuritySvrServer(CActive::TPriority aActiveObjectPriority);
		void ConstructL();
		
	private:
		~CSecuritySvrServer();
		TBool IsActiveDebugger(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId) const;
		TBool IsDebugger(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId) const;
		TInt GetProcessIdFromMessage(TProcessId& aProcessId, const RMessage2& aMessage) const;

	private:
		RPointerArray<CProcessPair> iActiveDebugMap;
		RPointerArray<CProcessPair> iPassiveDebugMap;
		TInt iSessionCount;
		CShutdownTimer iShutdown;
		RRM_DebugDriver iKernelDriver;

	// Declare the CSecuritySvrAsync as a friend so it can use the iKernelDriver too
	friend class CSecuritySvrAsync;
	friend class CSecuritySvrSession;
	};

#endif // C_SECURITY_SVR_SERVER_H
