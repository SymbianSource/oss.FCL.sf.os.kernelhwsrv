// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_ldd2.h
// 
//

#if !defined(__D_LDD2_H__)
#define __D_LDD2_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

IMPORT_C TInt LinkedTest1();
IMPORT_C TInt LinkedTest2();
IMPORT_C TInt LinkedTest3();
IMPORT_C TInt LinkedTest4();
IMPORT_C TInt LinkedTest5();
IMPORT_C TInt LinkedTest6(TInt aValue);
IMPORT_C TUint32 LinkedTest7();
IMPORT_C void LinkedTest8(TUint32 aValue);
IMPORT_C TInt LinkedTest9();

#endif

