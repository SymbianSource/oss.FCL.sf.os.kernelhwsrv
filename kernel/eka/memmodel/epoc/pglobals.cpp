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
// e32\memmodel\epoc\pglobals.cpp
// 
//

#include "plat_priv.h"

extern "C" {
TLinAddr RomHeaderAddress;
}

TLinAddr PP::RomRootDirAddress;
TInt PP::MaxUserThreadStack;
TInt PP::UserThreadStackGuard;
TInt PP::MaxStackSpacePerProcess;
TInt PP::SupervisorThreadStackGuard;
TUint32 PP::NanoWaitCal;
TUint32 PP::MonitorEntryPoint[3];
TLinAddr PP::RamDriveStartAddress;
TInt PP::RamDriveMaxSize;
TInt PP::RamDriveRange;
DChunk* PP::TheRamDriveChunk;
