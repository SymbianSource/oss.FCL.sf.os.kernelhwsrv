// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Kernel Event handler for Run Mode Debug.
//

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/arm/arm.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <nk_trace.h>

#include <rm_debug_api.h>
#include "debug_logging.h"
#include "d_process_tracker.h"
#include "d_rmd_stepping.h"
#include "rm_debug_kerneldriver.h"
#include "rm_debug_driver.h"
#include "rm_debug_eventhandler.h"


DRM_DebugEventHandler::DRM_DebugEventHandler()
	:	DKernelEventHandler(EventHandler, this)
{
	LOG_MSG("DRM_DebugEventHandler::DRM_DebugEventHandler()");

	for(TInt i=0; i<EEventLimit; i++)
		{
		iEventHandlers[i] = &DRM_DebugChannel::HandleUnsupportedEvent;
		}
	iEventHandlers[EEventUserTrace] = &DRM_DebugChannel::HandleUserTrace;
	iEventHandlers[EEventRemoveLibrary] = &DRM_DebugChannel::RemoveLibrary;
	iEventHandlers[EEventAddLibrary] = &DRM_DebugChannel::AddLibrary;
	iEventHandlers[EEventStartThread] = &DRM_DebugChannel::StartThread;
	iEventHandlers[EEventSwExc] = &DRM_DebugChannel::HandleSwException;
	iEventHandlers[EEventHwExc] = &DRM_DebugChannel::HandleHwException;
	iEventHandlers[EEventKillThread] = &DRM_DebugChannel::HandleEventKillThread;
	iEventHandlers[EEventAddProcess] = &DRM_DebugChannel::HandleAddProcessEvent;
	iEventHandlers[EEventRemoveProcess] = &DRM_DebugChannel::HandleRemoveProcessEvent;
}

TInt DRM_DebugEventHandler::Create(DLogicalDevice* aDevice, DLogicalChannel* aChannel, DThread* aClient)
{
	LOG_MSG3("DRM_DebugEventHandler::Create(), aClientthread=0x%08x id=%d", aClient, aClient->iId);

	TInt err;
	err = aDevice->Open();
	if (err != KErrNone)
		return err;
	iDevice = aDevice;
	
	iChannel = (DRM_DebugChannel*)aChannel; //Don't add ref the channel, since channel closes the event handler before it ever gets destroyed.

	err = aClient->Open();
	if (err != KErrNone)
		return err;
	iClientThread = aClient;

	// Use a semaphore to protect our data structures from concurrent access.
	err = Kern::SemaphoreCreate(iProtectionLock, _L("RM_DebugEventHandlerLock"), 1 /* Initial count */);
	if (err != KErrNone)
		return err;


	return Add();
}


DRM_DebugEventHandler::~DRM_DebugEventHandler()
{
	LOG_MSG("DRM_DebugEventHandler::~DRM_DebugEventHandler()");

	if (iProtectionLock)
		iProtectionLock->Close(NULL);
	
	if (iDevice)
		iDevice->Close(NULL);	
	
	if (iClientThread)
		Kern::SafeClose((DObject*&)iClientThread, NULL);
		
}


TInt DRM_DebugEventHandler::Start()
{
	LOG_MSG("DRM_DebugEventHandler::Start()");

	iTracking = ETrue;

	return KErrNone;
}


TInt DRM_DebugEventHandler::Stop()
{
	LOG_MSG("DRM_DebugEventHandler::Stop()");

	iTracking = EFalse;

	return KErrNone;
}


TUint DRM_DebugEventHandler::EventHandler(TKernelEvent aType, TAny* a1, TAny* a2, TAny* aThis)
{
	return ((DRM_DebugEventHandler*)aThis)->HandleEvent(aType, a1, a2);
}



TUint DRM_DebugEventHandler::HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2)
	{
	
	/*
	 * Check if we are tracking things at all OR 
	 * this event is beyond the limit of known events OR 
	 * this event is from the debug thread itself (don't want to debug ourselves) OR
	 * this event has a handler (there is no point in proceeding without a handler)
	 */
	if( (!iTracking) || 
			(aType > (TUint32)EEventLimit) ||
			(iClientThread == &Kern::CurrentThread()) ||
	    (iEventHandlers[aType] == &DRM_DebugChannel::HandleUnsupportedEvent) )
		{
		return ERunNext;
		}
	
	return HandleSpecificEvent(aType,a1,a2) ? EExcHandled : ERunNext;

	}

TBool DRM_DebugEventHandler::HandleSpecificEvent(TKernelEvent aType, TAny* a1, TAny* a2)
	{
	TBool ret = EFalse;

	NKern::ThreadEnterCS();
	LockDataAccess();


	if (iChannel)
		{
		ret = (iChannel->*(iEventHandlers[aType]))(a1, a2);
		}
	ReleaseDataAccess();
	NKern::ThreadLeaveCS();

	switch(aType)
		{
		case EEventHwExc:
		case EEventKillThread:
			{
			LOG_MSG2("DRM_DebugEventHandler::HandleEvent() -> FSWait(), kernel event type: %d", (TUint32)aType);
			TheDProcessTracker.FSWait();
			LOG_MSG("DRM_DebugEventHandler::HandleEvent() <- FSWait()");
			break;
			}
		default:
			break;
		}
	return ret;
	}

