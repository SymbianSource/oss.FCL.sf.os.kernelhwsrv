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
// Purpose: Kernel-side tracking of debug agent information associated
// with each process being debugged.
// 
//

#ifndef D_DEBUG_AGENT_H
#define D_DEBUG_AGENT_H

#include <rm_debug_api.h>
#include "d_driver_event_info.h"

class DDebugAgent : public DBase
{
public:
	static DDebugAgent* New(TUint64);
	~DDebugAgent();

	TInt SetEventAction(Debug::TEventType aEvent, Debug::TKernelEventAction aEventAction);
	void GetEvent(TClientDataRequest<Debug::TEventInfo>* aAsyncGetValueRequest, Debug::TEventInfo* aEventInfo, DThread* aClientThread);
	TInt EventAction(Debug::TEventType aEvent);

	TInt CancelGetEvent(void);
	void NotifyEvent(const TDriverEventInfo& aEventInfo);
	TUint64 Id();

protected:
	DDebugAgent(TUint64 aId);
	TInt Construct();

private:
	void QueueEvent(TDriverEventInfo& aEventInfo);
	TBool BufferEmpty() const;
	TBool BufferFull() const;
	TBool BufferCanStoreEvent() const;
	TBool BufferAtCriticalLevel() const;
	void IncrementPosition(TInt& aPosition);
	TInt NumberOfEmptySlots() const;

private:
	TUint64	iId;
	Debug::TKernelEventAction iEventActions[Debug::EEventsLast];

	//iEventInfo is a pointer to an object owned by the security server, so
	//no clean up needs be performed on it
	Debug::TEventInfo* iEventInfo;
	RArray<TDriverEventInfo> iEventQueue;	// ring buffer.
	TClientDataRequest<Debug::TEventInfo>* iRequestGetEventStatus;
	DThread* iClientThread;

	// Ring buffer data
	TInt iHead;	// points to the next empty slot in iEventQueue (exc. when iFull == ETrue)
	TInt iTail; // points to the oldest full slot in iEventQueue (exc. when iEmpty == ETrue)
	
	//if we have told the agent that we are ignoring trace events
	TBool iIgnoringTrace;
};

#endif // D_DEBUG_AGENT_H

