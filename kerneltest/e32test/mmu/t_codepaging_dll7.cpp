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
// e32test\mmu\t_codepaging_dll6.cpp
// 
//

#include "t_codepaging_dll.h"

EXPORT_C void** GetAddressOfRelocatedDataProxy(TInt& aSize, void*& aDataValue, void*& aCodeValue)
	{
	return GetAddressOfRelocatedData(aSize,aDataValue,aCodeValue);
	}
