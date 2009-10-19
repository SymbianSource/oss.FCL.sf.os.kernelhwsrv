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
// e32test\mmu\t_pagemove_dll.h
// 
//

#include <e32std.h>

#ifndef __T_PAGEMOVE_DLL_H__
#define __T_PAGEMOVE_DLL_H__

/// Test function in t_pagemove_dll called to test moving paged code.
IMPORT_C TInt DllTestFunction();
typedef TInt (*TTestFunction)();

const TInt KArbitraryNumber=4;
#endif
