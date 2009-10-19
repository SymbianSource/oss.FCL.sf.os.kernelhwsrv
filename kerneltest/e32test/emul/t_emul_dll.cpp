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
// e32test\emul\t_emul_dll.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "t_emul.h"

#ifdef __INCLUDE_STATIC_DATA__

struct TStaticData
	{
	TStaticData();
	~TStaticData();
	TInt* iData;
	};

TStaticData::TStaticData()
	{
	iData = (TInt*) User::Alloc(1);
	}

TStaticData::~TStaticData()
	{
	User::Free(iData);
	}

TStaticData staticData;

#endif

EXPORT_C void TrapExceptionInDll()
	{
	TRAP_IGNORE(User::Leave(KErrGeneral));
	}
