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
// e32test/defrag/perf/t_testdll.cpp
// Used to create t_defragdll0.dll
// 
//

#include "u32std.h"
#define TPS_DECLARE_ARRAY
#include "t_testdll.h"


TInt TestAlignment0(TInt aParam1, TInt aParam2);
TInt TestAlignment255(TInt aParam1, TInt aParam2);

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

extern "C" EXPORT_C TAny *Function0Addr()
	{
	return (TAny *)&TestAlignment0;
	}

extern "C" EXPORT_C TAny *FunctionNAddr()
	{
	return (TAny *)&TestAlignment255;
	}

