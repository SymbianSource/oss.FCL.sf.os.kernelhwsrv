// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides the debug security server's shutdown timer implementation
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#include <rm_debug_api.h>
#include "c_shutdown_timer.h"
#include "rm_debug_logging.h"

/**
Constructor. Adds the timer to the thread's active scheduler,
*/
CShutdownTimer::CShutdownTimer()
	:CTimer(KActivePriorityShutdown)
	{
	LOG_MSG("CShutdownTimer::CShutdownTimer()\n");
	CActiveScheduler::Add(this);
	}

/**
Initialisation of timer
*/
void CShutdownTimer::ConstructL()
	{
	LOG_MSG("CShutdownTimer::ConstructL()\n");
	CTimer::ConstructL();
	}

/**
Starts the timer which would expire after KShutdownDelay
*/
void CShutdownTimer::Start()
	{
	LOG_MSG("CShutdownTimer::Start()\n");
	After(KShutdownDelay);
	}

/**
Stops the active scheduler. Stopping the active scheduler effectively closes
the Debug Security Server
*/
void CShutdownTimer::RunL()
	{
	LOG_MSG("CShutdownTimer::RunL()\n");
	CActiveScheduler::Stop();
	}

