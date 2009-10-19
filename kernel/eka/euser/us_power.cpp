// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_power.cpp
// 
//

#include <e32power.h>
#include "us_std.h"




/**
Enables wakeup events.

Starts a system-wide transition to a low power state enabling wakeup events.

The exact specification of wakeup events depends on the target power state
and the platform.
For example, an absolute timer expiration event wakes up the system from
the EPwStandby power state on all currently supported platforms.

If, at the time this function is called, wakeup events are enabled, then
the implementation disables them before enabling them again.  

The caller must have PowerMgnt capability to perform this call; otherwise
it panics with EPlatformSecurityTrap code.

@param aState   The target low power state; this can be EPwStandby or KPwOff.
				
@return KErrNone, if successful,
        KErrArgument, if aState is neither EPwStandby nor KPwOff;
        otherwise one of the other system-wide error codes.
		
@panic KERN-EXEC 46 if the caller does not have PowerMgnt capability.
		
@see Power::DisableWakeupEvents()		

@capability PowerMgmt
*/
EXPORT_C TInt Power::EnableWakeupEvents(TPowerState aState)
	{
	return Exec::PowerEnableWakeupEvents(aState);
	}




/**
Disables wakeup events.

If, at the time this function is called, wakeup events are disabled, then
the function does nothing and returns immediately.

The caller must have PowerMgnt capability to perform this call; otherwise it
panics with EPlatformSecurityTrap code.
		
@panic KERN-EXEC 46 if the caller does not have PowerMgnt capability.		

@capability PowerMgmt
*/
EXPORT_C void Power::DisableWakeupEvents()
	{
	Exec::PowerDisableWakeupEvents();
	}




/**
Requests notification of a subsequent wakeup event.

Typically the user-side domain manager issues this request 
before starting a system-wide transition to a low power state by
enabling wakeup events.

Only one pending request is allowed. The request completes immediately
with KErrInUse status if another request is currently pending.

The caller must have PowerMgnt capability to perform this call; otherwise
it panics with EPlatformSecurityTrap code.

@param aStatus The request status to signal on wakeup event

@panic KERN-EXEC 46 if the caller does not have PowerMgnt capability.

@see Power::EnableWakeupEvents()

@capability PowerMgmt
*/
EXPORT_C void Power::RequestWakeupEventNotification(TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	Exec::PowerRequestWakeupEventNotification(&aStatus);
	}




/**
Cancels a pending wakeup event notification request.

If, at the time this function is called, the notification request is still
pending, then the request completes with KErrCancel status.

The caller must have PowerMgnt capability to perform this call; otherwise it panics with
EPlatformSecurityTrap code

@panic KERN-EXEC 46 if the caller does not have PowerMgnt capability.

@capability PowerMgmt
*/
EXPORT_C void Power::CancelWakeupEventNotification()
	{
	Exec::PowerCancelWakeupEventNotification();
	}




/**
Lowers the kernel power state.
	 
Changes the kernel power state from EPwActive to the state specified by the last 
call to EnableWakeupEvents().

If the target state is EPwStandby, the function returns either immediately,
if at least one wakeup event has occurred since the last call
to EnableWakeupEvent(), or when a wakeup event eventually occurs.
		
When this function returns, wakeup events are disabled.

If the target state is EPwOff, this function never returns,
the system is powered off, and can subsequently only come back by rebooting.

The caller must have PowerMgnt capability to perform this call; otherwise
it panics with EPlatformSecurityTrap code

@return KErrNone, if successful;
        KErrNotReady, if wakeup events are disabled at the time this function is called;
        otherwise one of the other system-wide error codes.

@panic KERN-EXEC 46 if the caller does not have PowerMgnt capability.

@capability PowerMgmt
*/
EXPORT_C TInt Power::PowerDown()
	{ 
	return Exec::PowerDown();
	}	
