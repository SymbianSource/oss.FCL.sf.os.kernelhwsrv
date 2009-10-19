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
// e32test\mmu\t_codepaging_dll2.cpp
// 
//

#include "t_codepaging_dll.h"

// Nearly 3Mb of mostly zeros - we want the dll to be mapped into exactly 3Mb of pages
const TUint8 TestData[(3 * 1024 - 4) * 1024] = { 1 };

const TInt KSizeOfTestData = sizeof(TestData);

EXPORT_C const TUint* GetAddressOfData(TInt& aSize)
	{
	aSize = KSizeOfTestData;
	return (TUint*)TestData;
	}
