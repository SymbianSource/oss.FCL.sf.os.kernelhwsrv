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
// Purpose: Kernel-side tracking of process state
// 
//
 
#ifndef D_PROCESS_TRACKER_H
#define D_PROCESS_TRACKER_H

#include "d_target_process.h"

// The global class which tracks all debugged processes.
// 
// Note that multiple debug agents may attach to a process,
// as the security server will ensure only one is an 'active'
// agent, preventing conflicts. Other agents will be 'passive',
// typically interested only in recording events.
//
// The above requirement generates the requirement for the class
// to track the agent IDs, as multiple debug agents may be interested
// in a process.

class DProcessTracker : public DBase
{
public:
	DProcessTracker();
	~DProcessTracker();

	TInt AttachProcess(const TDesC8& aProcessName, TUint64 aAgentId);

	TInt DetachProcess(const TDesC8& aProcessName, TUint64 aAgentId);

	TInt DetachAgent(TUint64 aAgentId);

	DTargetProcess* FindProcess(const TDesC8& aProcessName);

	DTargetProcess* FuzzyFindProcess(const TDesC8& aProcessName);

	TBool CheckSuspended(const TUint64 aTargetThreadId) const;
	TBool CheckSuspended(DThread* aTargetThread) const;
	TInt SuspendThread(DThread* aTargetThread, TBool aFreezeThread=EFalse);
	TInt ResumeThread(DThread* aTargetThread);
	void FSWait();

private:
	TInt RemoveSuspendedThread(DThread* aThread);
	TInt AddSuspendedThread(DThread* aThread);
	HBuf* GetFileName(DThread* aThread) const;

private:
	RPointerArray<DTargetProcess> iProcesses;
};

// static global object
extern DProcessTracker TheDProcessTracker;

#endif // D_PROCESS_TRACKER_H
