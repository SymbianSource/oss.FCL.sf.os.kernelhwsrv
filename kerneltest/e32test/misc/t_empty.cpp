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
// e32test\misc\t_empty.cpp
// Overview:
// Does absolutely nothing! Has no imports.
// API Information:
// None
// Details:
// Does absolutely nothing! Has no imports.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>

#ifdef __VC32__
TAny* User::Alloc(TInt)
	{
	return NULL;
	}

void User::Free(TAny*)
	{
	}
#endif

GLDEF_C TInt E32Main()
	{
	return 0;
	}
