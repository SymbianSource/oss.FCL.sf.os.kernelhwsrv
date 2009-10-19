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
// e32test\dll\t_ver2.h
// 
//

#ifndef __T_VER2_H__
#define __T_VER2_H__
#include <e32cmn.h>

IMPORT_C TInt Export01();
#if !defined(__VER2_0__) && !defined(__VER3_0__)
IMPORT_C TInt Export02();
IMPORT_C TInt Export03();
#endif
#if !defined(__VER3_0__)
IMPORT_C TInt Export04();
#endif
IMPORT_C TInt Export05();
IMPORT_C TInt Export06();
IMPORT_C TInt Export07();
IMPORT_C TInt Export08();
IMPORT_C TInt Export09();
IMPORT_C TInt Export10();
IMPORT_C TInt Export11();
IMPORT_C TInt Export12();
IMPORT_C TInt Export13();
IMPORT_C TInt Export14();
IMPORT_C TInt Export15();
IMPORT_C TInt Export16();
IMPORT_C TInt Export17();
IMPORT_C TInt Export18();
IMPORT_C TInt Export19();

#if defined(__VER1_1__) || defined(__VER2_0__) || defined(__VER3_0__)
IMPORT_C TInt Export20();
IMPORT_C TInt Export21();
IMPORT_C TInt Export22();
#if !defined(__VER2_0__) && !defined(__VER3_0__)
IMPORT_C TInt Export23();
IMPORT_C TInt Export24();
#endif
IMPORT_C TInt Export25();
IMPORT_C TInt Export26();
IMPORT_C TInt Export27();
IMPORT_C TInt Export28();
IMPORT_C TInt Export29();
#endif

#if defined(__VER2_0__) || defined(__VER3_0__)
IMPORT_C TInt Export30();
IMPORT_C TInt Export31();
IMPORT_C TInt Export32();
IMPORT_C TInt Export33();
IMPORT_C TInt Export34();
IMPORT_C TInt Export35();
IMPORT_C TInt Export36();
IMPORT_C TInt Export37();
IMPORT_C TInt Export38();
#if !defined(__VER3_0__)
IMPORT_C TInt Export39();
#endif
#endif

#if defined(__VER3_0__)
IMPORT_C TInt Export40();
IMPORT_C TInt Export41();
IMPORT_C TInt Export42();
IMPORT_C TInt Export43();
IMPORT_C TInt Export44();
IMPORT_C TInt Export45();
IMPORT_C TInt Export46();
IMPORT_C TInt Export47();
IMPORT_C TInt Export48();
IMPORT_C TInt Export49();
IMPORT_C TInt Export50();
IMPORT_C TInt Export51();
IMPORT_C TInt Export52();
IMPORT_C TInt Export53();
IMPORT_C TInt Export54();
IMPORT_C TInt Export55();
IMPORT_C TInt Export56();
IMPORT_C TInt Export57();
IMPORT_C TInt Export58();
IMPORT_C TInt Export59();
#endif


#endif
