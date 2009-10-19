// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_ver1.cpp
// 
//

#include "t_ver.h"

TInt Global = 10;

EXPORT_C TInt Export1()
	{
	return 1;
	}

EXPORT_C TInt Export2()
	{
	return Global;
	}

EXPORT_C TInt Export3()
	{
	return ++Global;
	}

#ifdef __INCLUDE_EXPORT_4__
EXPORT_C TInt Export4()
	{
	return Global--;
	}
#endif

#ifdef __INCLUDE_EXPORT_5__
EXPORT_C TInt Export5(TInt a)
	{
	TInt r = Global;
	Global += a;
	return r;
	}
#endif

#ifdef __INCLUDE_EXPORT_6__
EXPORT_C TInt Export6(TInt a)
	{
	TInt r = Global;
	Global = a;
	return r;
	}
#endif




