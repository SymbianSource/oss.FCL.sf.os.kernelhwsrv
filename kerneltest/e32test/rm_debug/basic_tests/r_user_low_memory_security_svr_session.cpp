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
// r_kernel_low_memory_security_svr_session.cpp
// Implementation of RUserLowMemorySecuritySvrSession
// 
//

#include "r_user_low_memory_security_svr_session.h"
#include <rm_debug_api.h>
#ifdef _DEBUG
#include "low_mem_requests.h"
#endif

void RUserLowMemorySecuritySvrSession::FailAlloc(const TInt aCount)
	{
#ifdef _DEBUG
	TIpcArgs args(aCount);
	SendReceive(EDebugServFailAlloc, args);
#endif
	}

void RUserLowMemorySecuritySvrSession::HeapReset()
	{
#ifdef _DEBUG
	TIpcArgs args(0);
	SendReceive(EDebugServFailAlloc, args);
#endif
	}

void RUserLowMemorySecuritySvrSession::MarkHeap()
	{
#ifdef _DEBUG
	SendReceive(EDebugServMarkHeap);
#endif
	}

void RUserLowMemorySecuritySvrSession::MarkHeapEnd()
	{
#ifdef _DEBUG
	SendReceive(EDebugServMarkEnd);
#endif
	}

