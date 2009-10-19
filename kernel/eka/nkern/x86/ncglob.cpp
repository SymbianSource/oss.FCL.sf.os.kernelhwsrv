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
// e32\nkern\x86\ncglob.cpp
// 
//

#include <x86.h>

//
extern "C" {
TUint32 X86_IrqStack[IRQ_STACK_SIZE/4];
SFullX86RegSet X86_Regs;
TLinAddr X86_IrqHandler;
TInt X86_IrqNestCount;
SCpuIdleHandler CpuIdleHandler;
TX86Tss* X86_TSS_Ptr;
TBool X86_UseGlobalPTEs;
TUint64 DefaultCoprocessorState[64];
}
TUint32 X86::DefaultCR0;

TScheduler TheScheduler;

SBTraceData BTraceData = { {0}, 0, 0 };
