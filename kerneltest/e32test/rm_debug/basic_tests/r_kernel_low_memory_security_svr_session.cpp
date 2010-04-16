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
// Implementation of RKernelLowMemorySecuritySvrSession
// 
//

#include "r_kernel_low_memory_security_svr_session.h"

void RKernelLowMemorySecuritySvrSession::FailAlloc(const TInt aCount)
	{
	__KHEAP_FAILNEXT(aCount);
	}

void RKernelLowMemorySecuritySvrSession::HeapReset()
	{
	__KHEAP_RESET;
	}

void RKernelLowMemorySecuritySvrSession::MarkHeap()
	{
	__KHEAP_MARK;
	}

void RKernelLowMemorySecuritySvrSession::MarkHeapEnd()
	{
	__KHEAP_MARKEND;
	}

