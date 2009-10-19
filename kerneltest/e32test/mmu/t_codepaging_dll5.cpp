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
// e32test\mmu\t_codepaging_dll5.cpp
// 
//

#include "t_codepaging_dll.h"

#define C (void*)GetAddressOfRelocatedData
#define D (void*)TestData

#define A128 C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D,C,D
#define A1k A128,A128,A128,A128,A128,A128,A128,A128
#define A8k A1k,A1k,A1k,A1k,A1k,A1k,A1k,A1k

#ifdef CONST_DATA
static const void* const TestData[] =
#else
static const void* TestData[] =
#endif
	{
	A8k,A8k,A8k,A8k,A8k,A8k,A8k,A8k
	};

const TInt KSizeOfTestData = sizeof(TestData);

EXPORT_C void** GetAddressOfRelocatedData(TInt& aSize, void*& aDataValue, void*& aCodeValue)
	{
	aDataValue = D;
	aCodeValue = C;
	aSize = KSizeOfTestData;
	return (void**)TestData;
	}
