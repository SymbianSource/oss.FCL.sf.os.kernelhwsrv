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
// e32\euser\epoc\x86\uc_trp.cpp
// 
//

#include <u32exec.h>
#include <e32panic.h>

GLREF_C void Panic(TCdtPanic);

#ifndef __LEAVE_EQUALS_THROW__

void DoLeave(TTrap*);

void __stdcall DoTrap(TTrap* aFrame)
	{
	TTrapHandler* pH=Exec::PushTrapFrame(aFrame);
	if (pH)
		pH->Trap();
	}

EXPORT_C void User::Leave(TInt aReason)
//
// Leave to the current control region.
//
	{
	TTrap* pF=Exec::PopTrapFrame();
	if (!pF)
		::Panic(EUserLeaveWithoutTrap);
	*pF->iResult=aReason;
	TTrapHandler* pH=pF->iHandler;
	if (pH)
		pH->Leave(aReason);
	DoLeave(pF);
	}

#endif // !__LEAVE_EQUALS_THROW__
