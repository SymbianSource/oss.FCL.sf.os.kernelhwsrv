// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\emul\win32\mglobals.cpp
// 
//

#include "memmodel.h"

TAny* MM::KernelHeapAddress;
DMutex* MM::RamAllocatorMutex;
TInt MM::RamChunkSize;
TInt MM::RamChunkShift;
TInt MM::RamPageSize;
TInt MM::RamPageShift;
TInt MM::FreeMemory;
TInt MM::CacheMemory = 0;
TInt MM::ReclaimedCacheMemory = 0;
TInt MM::InitialFreeMemory;
TBool MM::AllocFailed;
RArray<SWin32Module> MM::Win32Modules(32, _FOFF(SWin32Module,iWin32ModuleHandle), 512);
TInt MM::NextCodeSegId;
