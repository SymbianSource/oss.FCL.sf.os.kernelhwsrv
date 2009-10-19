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
// e32\euser\epoc\up_gcc_abort.cpp
// This file contains general gcc helper functions
// 
//

#include "u32std.h"

extern "C" {

GLDEF_C void atexit()
	{
	}

// GCC insists on calling this in noreturn functions
GLDEF_C void abort()
	{
	abort();
	}
}
