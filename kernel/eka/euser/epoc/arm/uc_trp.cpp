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
// e32\euser\epoc\arm\uc_trp.cpp
// 
//

#include "u32std.h"
#include <e32panic.h>
#include "uc_std.h"

GLREF_C void Panic(TCdtPanic);

GLDEF_C void PanicNoTrapFrame()
//
// Panic if there is no trap frame
//
    {

    Panic(EUserLeaveWithoutTrap);
    }

