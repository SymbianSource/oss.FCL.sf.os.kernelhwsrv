// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_lddns.cpp
// 
//
// This dummy driver is built for a negative test: although the name
// suggests it's an LDD, it's actually a PDD; then, the test program
// checks that it CANNOT actually be loaded as an LDD.


#include <e32def.h>
#include <kernel/kernel.h>

DECLARE_STANDARD_PDD()
	{
	return NULL;
	}

