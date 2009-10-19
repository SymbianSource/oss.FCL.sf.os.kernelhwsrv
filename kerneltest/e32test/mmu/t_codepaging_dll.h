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
// e32test\mmu\t_codepaging_dll.h
// 
//

#include <e32std.h>

#ifndef __T_CODEPAGING_DLL_H__
#define __T_CODEPAGING_DLL_H__

/// Test function in t_codepaging_dll called to test executing paged code
IMPORT_C TInt TestFunction();
typedef TInt (*TTestFunction)();
const TInt KTestFunctionOrdinal = 1;

/// Function in t_codepaging_dll2 called to get size and address of test data
IMPORT_C const TUint* GetAddressOfData(TInt& aSize);
typedef TUint* (*TGetAddressOfDataFunction)(TInt&);
const TInt KGetAddressOfDataFunctionOrdinal = 1;

/// Function in t_codepaging_dll5+6 called to get size and address of test data
IMPORT_C void** GetAddressOfRelocatedData(TInt& aSize, void*& aDataValue, void*& aCodeValue);
typedef void** (*TGetAddressOfRelocatedDataFunction)(TInt&,void*&,void*&);
const TInt KGetAddressOfRelocatedDataFunctionOrdinal = 1;

#endif
