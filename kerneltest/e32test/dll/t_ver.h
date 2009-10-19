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
// e32test\dll\t_ver.h
// 
//

#ifndef __T_VER_H__
#define __T_VER_H__
#include <e32cmn.h>

IMPORT_C TInt Export1();
IMPORT_C TInt Export2();
IMPORT_C TInt Export3();

#ifdef __INCLUDE_EXPORT_4__
IMPORT_C TInt Export4();
#endif

#ifdef __INCLUDE_EXPORT_5__
IMPORT_C TInt Export5(TInt a);
#endif

#ifdef __INCLUDE_EXPORT_6__
IMPORT_C TInt Export6(TInt a);
#endif

#endif
