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
//

/** Event handler and container for all objects being tracked.  */
#ifndef __RM_DEBUG_EVENTHANDLER_H__
#define __RM_DEBUG_EVENTHANDLER_H__


class DRM_DebugEventHandler : public DKernelEventHandler
{
	public:
		DRM_DebugEventHandler();
		TInt Create(DLogicalDevice* aDevice, DLogicalChannel* aChannel, DThread* aClient);
		~DRM_DebugEventHandler();
		TInt Start();
		TInt Stop();
		
		inline void DRM_DebugEventHandler::LockDataAccess()
		    {
		    Kern::SemaphoreWait(*iProtectionLock);
		    }

		inline void DRM_DebugEventHandler::ReleaseDataAccess()
		    {
		    Kern::SemaphoreSignal(*iProtectionLock);
		    }
		
	private:
		static TUint EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
		TUint HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2);
		TBool HandleSpecificEvent(TKernelEvent aType, TAny* a1, TAny* a2);
		
	private:
		/** Used to serialise access data structures */
		DSemaphore* iProtectionLock;

		TBool iTracking;

		DLogicalDevice* iDevice;	// open reference to LDD for avoiding lifetime issues
		DThread* iClientThread;
		DRM_DebugChannel* iChannel;

		// typdef for functions which handle our specific events
		typedef TBool (DRM_DebugChannel::*eventHandler)(TAny* a1, TAny* a2);
		eventHandler iEventHandlers[EEventLimit];
};

#endif //__RM_DEBUG_EVENTHANDLER_H__
