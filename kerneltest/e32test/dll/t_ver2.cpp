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
// e32test\dll\t_ver2.cpp
// 
//

#include "t_ver2.h"

EXPORT_C TInt Export01()
	{
#if defined(__VER3_0__)
	return 0x00030000;
#elif defined(__VER2_0__)
	return 0x00020000;
#elif defined(__VER1_1__)
	return 0x00010001;
#else
	return 0x00010000;
#endif
	}

#if !defined(__VER2_0__) && !defined(__VER3_0__)
EXPORT_C TInt Export02()
	{ return 2*2; }
EXPORT_C TInt Export03()
	{ return 3*3; }
#endif
#if !defined(__VER3_0__)
EXPORT_C TInt Export04()
	{ return 4*4; }
#endif
EXPORT_C TInt Export05()
	{ return 5*5; }
EXPORT_C TInt Export06()
	{ return 6*6; }
EXPORT_C TInt Export07()
	{ return 7*7; }
EXPORT_C TInt Export08()
	{ return 8*8; }
EXPORT_C TInt Export09()
	{ return 9*9; }

EXPORT_C TInt Export10()
	{ return 10*10; }
EXPORT_C TInt Export11()
	{ return 11*11; }
EXPORT_C TInt Export12()
	{ return 12*12; }
EXPORT_C TInt Export13()
	{ return 13*13; }
EXPORT_C TInt Export14()
	{ return 14*14; }
EXPORT_C TInt Export15()
	{ return 15*15; }
EXPORT_C TInt Export16()
	{ return 16*16; }
EXPORT_C TInt Export17()
	{ return 17*17; }
EXPORT_C TInt Export18()
	{ return 18*18; }
EXPORT_C TInt Export19()
	{ return 19*19; }

#if defined(__VER1_1__) || defined(__VER2_0__) || defined(__VER3_0__)
EXPORT_C TInt Export20()
	{ return 20*20; }
EXPORT_C TInt Export21()
	{ return 21*21; }
EXPORT_C TInt Export22()
	{ return 22*22; }
#if !defined(__VER2_0__) && !defined(__VER3_0__)
EXPORT_C TInt Export23()
	{ return 23*23; }
EXPORT_C TInt Export24()
	{ return 24*24; }
#endif
EXPORT_C TInt Export25()
	{ return 25*25; }
EXPORT_C TInt Export26()
	{ return 26*26; }
EXPORT_C TInt Export27()
	{ return 27*27; }
EXPORT_C TInt Export28()
	{ return 28*28; }
EXPORT_C TInt Export29()
	{ return 29*29; }
#endif

#if defined(__VER2_0__) || defined(__VER3_0__)
EXPORT_C TInt Export30()
	{ return 30*30; }
EXPORT_C TInt Export31()
	{ return 31*31; }
EXPORT_C TInt Export32()
	{ return 32*32; }
EXPORT_C TInt Export33()
	{ return 33*33; }
EXPORT_C TInt Export34()
	{ return 34*34; }
EXPORT_C TInt Export35()
	{ return 35*35; }
EXPORT_C TInt Export36()
	{ return 36*36; }
EXPORT_C TInt Export37()
	{ return 37*37; }
EXPORT_C TInt Export38()
	{ return 38*38; }
#if !defined(__VER3_0__)
EXPORT_C TInt Export39()
	{ return 39*39; }
#endif
#endif

#if defined(__VER3_0__)
EXPORT_C TInt Export40()
	{ return 40*40; }
EXPORT_C TInt Export41()
	{ return 41*41; }
EXPORT_C TInt Export42()
	{ return 42*42; }
EXPORT_C TInt Export43()
	{ return 43*43; }
EXPORT_C TInt Export44()
	{ return 44*44; }
EXPORT_C TInt Export45()
	{ return 45*45; }
EXPORT_C TInt Export46()
	{ return 46*46; }
EXPORT_C TInt Export47()
	{ return 47*47; }
EXPORT_C TInt Export48()
	{ return 48*48; }
EXPORT_C TInt Export49()
	{ return 49*49; }

EXPORT_C TInt Export50()
	{ return 50*50; }
EXPORT_C TInt Export51()
	{ return 51*51; }
EXPORT_C TInt Export52()
	{ return 52*52; }
EXPORT_C TInt Export53()
	{ return 53*53; }
EXPORT_C TInt Export54()
	{ return 54*54; }
EXPORT_C TInt Export55()
	{ return 55*55; }
EXPORT_C TInt Export56()
	{ return 56*56; }
EXPORT_C TInt Export57()
	{ return 57*57; }
EXPORT_C TInt Export58()
	{ return 58*58; }
EXPORT_C TInt Export59()
	{ return 59*59; }
#endif
