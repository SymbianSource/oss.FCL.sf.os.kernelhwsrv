// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\x86\cmonitor.cpp
// Kernel crash debugger - ARM specific portion
// 
//

#include <kernel/monitor.h>
#include <x86_mem.h>


void TTrapM::UnTrap()
//
// Pop the current trap frame
//
	{
	TheMonitorPtr->iFrame=TheMonitorPtr->iFrame->iNext;
	}

void MonitorInit(TAny* fc, TInt fr)
	{
	TheMonitorPtr->Init(fc,fr);
	}


void Monitor::DumpCpuThreadData(DThread* aThread)
	{
	}

