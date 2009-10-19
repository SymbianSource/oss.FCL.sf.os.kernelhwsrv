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
// Implementation of RLowMemorySecuritySvrSession
// 
//

#include "r_low_memory_security_svr_session.h"
#include <e32debug.h>

// test the effects of heap failure on global RSecuritySvrSession::GetList() in debug mode,
// in release mode normal call is made (heap checking not applicable)
TInt RLowMemorySecuritySvrSession::GetList(const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	TInt failAt = 0;
	TInt err = KErrNoMemory;
	while(err == KErrNoMemory)
		{
		failAt++;
		FailAlloc(failAt);
		MarkHeap();
		err = this->RSecuritySvrSession::GetList(aListId, aListData, aDataSize);
		if(KErrNoMemory == err)
			{
			MarkHeapEnd();
			}
		HeapReset();
		//RDebug::Printf("Debug::RLowMemorySecuritySvrSession::GetList(): failAt: %d, err: %d", failAt, err);
		}
	return err;
	}

// test the effects of heap failure on thread-specific RSecuritySvrSession::GetList() in debug mode,
// in release mode normal call is made (heap checking not applicable)
TInt RLowMemorySecuritySvrSession::GetList(const TThreadId aThreadId, const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	TInt failAt = 0;
	TInt err = KErrNoMemory;
	while(err == KErrNoMemory)
		{
		failAt++;
		FailAlloc(failAt);
		MarkHeap();
		err = this->RSecuritySvrSession::GetList(aThreadId, aListId, aListData, aDataSize);
		if(KErrNoMemory == err)
			{
			MarkHeapEnd();
			}
		HeapReset();
		//RDebug::Printf("Debug::RLowMemorySecuritySvrSession::GetList(TThreadId): failAt: %d, err: %d", failAt, err);
		}
	return err;
	}

// test the effects of heap failure on process-specific RSecuritySvrSession::GetList() in debug mode,
// in release mode normal call is made (heap checking not applicable)
TInt RLowMemorySecuritySvrSession::GetList(const TProcessId aProcessId, const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	TInt failAt = 0;
	TInt err = KErrNoMemory;
	while(err == KErrNoMemory)
		{
		failAt++;
		FailAlloc(failAt);
		MarkHeap();
		err = this->RSecuritySvrSession::GetList(aProcessId, aListId, aListData, aDataSize);
		if(KErrNoMemory == err)
			{
			MarkHeapEnd();
			}
		HeapReset();
		//RDebug::Printf("Debug::RLowMemorySecuritySvrSession::GetList(TProcessId): failAt: %d, err: %d", failAt, err);
		}
	return err;
	}

