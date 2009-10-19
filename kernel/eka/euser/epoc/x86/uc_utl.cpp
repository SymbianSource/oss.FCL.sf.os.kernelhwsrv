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
// e32\euser\epoc\x86\uc_utl.cpp
// 
//

#include "u32std.h"
#include <e32panic.h>
#include "us_std.h"

EXPORT_C void EmptyFunction()
 //Function with an empty body 
	{
	}

#ifdef __GCC32__

extern "C" EXPORT_C  int __cxa_pure_virtual()
//
// Gets called for any unreplaced pure virtual methods.
//
	{
	Panic(EPureVirtualCalled);
	return 0;
	}

#endif
