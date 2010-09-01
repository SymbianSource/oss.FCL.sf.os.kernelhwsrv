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

//
// returns the free RAM in bytes
//
inline TInt FreeRam()
	{
	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

	TMemoryInfoV1Buf meminfo;
	UserHal::MemoryInfo(meminfo);
	return meminfo().iFreeRamInBytes;
	}

#endif
