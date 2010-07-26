// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\freeram.h
// 
//

#ifndef __FREERAM_H__
#define __FREERAM_H__

/**
Get the amount of free RAM in the system in bytes.

This calls the kernel HAL supervisor barrier to ensure that asynchronous cleanup in the supervisor
happens before the amount of free RAM is measured.

There is also the option to wait for upto a specified timeout for the system to become idle.

@param aIdleTimeoutMs If non-zero, the number of milliseconds to wait for the system to become idle.

@return On sucess returns the amount of free RAM in bytes, KErrTimedOut if the timeout expired
        without the system becoming idle, or one of the other system-wide error codes.
*/
TInt FreeRam(TInt aIdleTimeoutMs = 0)
	{
	// wait for any async cleanup in the supervisor to finish first...
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier,
								  (TAny*)aIdleTimeoutMs, 0);
	if (r != KErrNone)
		return r;

	TMemoryInfoV1Buf meminfo;
	r = UserHal::MemoryInfo(meminfo);
	if (r != KErrNone)
		return r;
	
	return meminfo().iFreeRamInBytes;
	}

#endif
