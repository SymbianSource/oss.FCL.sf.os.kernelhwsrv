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
// e32\memmodel\emul\win32\mkernel.cpp
// 
//

#include "memmodel.h"

EXPORT_C TInt Kern::FreeRamInBytes()
	{
	return MM::FreeMemory+MM::CacheMemory;
	}

/********************************************
 * Thread
 ********************************************/

TInt DThread::AllocateSupervisorStack()
	{
	return KErrNone;
	}

void DThread::FreeSupervisorStack()
	{
	}

TInt DThread::AllocateUserStack(TInt, TBool)
	{
	return KErrNone;
	}

void DThread::FreeUserStack()
	{
	iUserStackRunAddress=0;
	}

TInt DWin32Thread::SetupContext(SThreadCreateInfo& )
	{
	iUserStackSize = 1024*1024; // The default stack reserve size on Windows
	iUserStackRunAddress = iNThread.iUserStackBase-iUserStackSize;
	iNThread.SetAddressSpace(iOwningProcess);
	return KErrNone;
	}

TBool Exc::IsMagic(TLinAddr /*anAddress*/)
//
// Return TRUE if anAddress is a 'magic' exception handling instruction
//
	{
	return EFalse;
	}

