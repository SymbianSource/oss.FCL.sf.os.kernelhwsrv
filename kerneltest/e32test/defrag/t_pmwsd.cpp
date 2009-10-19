// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\t_pmwsd.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "t_pmwsd.h"

TUint8 somedata[16384];

EXPORT_C TUint8* DllWsd::Address()
	{
	return somedata;
	}
