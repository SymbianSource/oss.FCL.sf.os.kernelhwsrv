// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\demandpaging\loader\t_pageldrtst.cpp
// 
//

#include <e32svr.h>
#include "u32std.h"
#include <e32base.h>
#include <e32base_private.h>
#include <e32math.h>

#define TPS_DECLARE_ARRAY
#include "t_pagestress.h"

extern "C" EXPORT_C TInt Init()
	{
	return KErrNone;
	}

extern "C" EXPORT_C TUint32 FunctionCount()
	{
	return PAGESTRESS_FUNC_COUNT;
	}

extern "C" EXPORT_C TInt CallFunction(TInt aParam1, TInt aParam2, TUint32 aIndex)
	{
	return CallTestFunc(aParam1, aParam2, aIndex);
	}


extern "C" EXPORT_C TInt SetClose()
	{
	return KErrNone;
	}
