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
// e32\euser\us_data.h
// Defines accessor functions for thread local data, either using Exec calls or the thread ID
// register if available
// 
//

#ifndef __US_DATA_H__
#define __US_DATA_H__

#include "us_std.h"

#ifdef __USERSIDE_THREAD_DATA__

TLocalThreadData* LocalThreadData();

inline RAllocator* GetHeap()
	{
	return LocalThreadData()->iHeap;
	}

inline CActiveScheduler* GetActiveScheduler()
	{
	return LocalThreadData()->iScheduler;
	}

inline void SetActiveScheduler(CActiveScheduler* aS)
	{
	LocalThreadData()->iScheduler = aS;
	}

inline TTrapHandler* GetTrapHandler()
	{
	return LocalThreadData()->iTrapHandler;
	}

#else

inline RAllocator* GetHeap()
	{
	return Exec::Heap();
	}

inline CActiveScheduler* GetActiveScheduler()
	{
	return Exec::ActiveScheduler();
	}

inline void SetActiveScheduler(CActiveScheduler* aS)
	{
	Exec::SetActiveScheduler(aS);
	}

inline TTrapHandler* GetTrapHandler()
	{
	return Exec::TrapHandler();
	}

#endif

#endif

