// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\up_std.h
// 
//

#if !defined(__UPSTD_H__)
#define __UPSTD_H__
#include <e32std.h>
#include <e32std_private.h>
#include <e32const_private.h>

extern "C" {
GLREF_C TInt _E32Dll(TInt);
#define MODULE_HANDLE ((TInt)_E32Dll)
}

#endif
