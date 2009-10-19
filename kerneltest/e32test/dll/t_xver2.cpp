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
// e32test\dll\t_xver2.cpp
// 
//

#include <e32test.h>
#include "t_ver2.h"

RTest test(_L("T_XVER2"));


TInt E32Main()
	{
	test.Title();

	TUint ver = (TUint)Export01();
	test.Printf(_L("Version %d.%d\n"), ver>>16, ver&0xffff);

#if defined(__USE02__)
	test.Printf(_L("Test 2\n"));
	test(Export02() == 2*2);
#endif
#if defined(__USE03__)
	test.Printf(_L("Test 3\n"));
	test(Export03() == 3*3);
#endif
#if defined(__USE04__)
	test.Printf(_L("Test 4\n"));
	test(Export04() == 4*4);
#endif
#if defined(__USE05__)
	test.Printf(_L("Test 5\n"));
	test(Export05() == 5*5);
#endif
#if defined(__USE06__)
	test.Printf(_L("Test 6\n"));
	test(Export06() == 6*6);
#endif
#if defined(__USE07__)
	test.Printf(_L("Test 7\n"));
	test(Export07() == 7*7);
#endif
#if defined(__USE08__)
	test.Printf(_L("Test 8\n"));
	test(Export08() == 8*8);
#endif
#if defined(__USE09__)
	test.Printf(_L("Test 9\n"));
	test(Export09() == 9*9);
#endif
#if defined(__USE10__)
	test.Printf(_L("Test 10\n"));
	test(Export10() == 10*10);
#endif
#if defined(__USE11__)
	test.Printf(_L("Test 11\n"));
	test(Export11() == 11*11);
#endif
#if defined(__USE12__)
	test.Printf(_L("Test 12\n"));
	test(Export12() == 12*12);
#endif
#if defined(__USE13__)
	test.Printf(_L("Test 13\n"));
	test(Export13() == 13*13);
#endif
#if defined(__USE14__)
	test.Printf(_L("Test 14\n"));
	test(Export14() == 14*14);
#endif
#if defined(__USE15__)
	test.Printf(_L("Test 15\n"));
	test(Export15() == 15*15);
#endif
#if defined(__USE16__)
	test.Printf(_L("Test 16\n"));
	test(Export16() == 16*16);
#endif
#if defined(__USE17__)
	test.Printf(_L("Test 17\n"));
	test(Export17() == 17*17);
#endif
#if defined(__USE18__)
	test.Printf(_L("Test 18\n"));
	test(Export18() == 18*18);
#endif
#if defined(__USE19__)
	test.Printf(_L("Test 19\n"));
	test(Export19() == 19*19);
#endif
#if defined(__USE20__)
	test.Printf(_L("Test 20\n"));
	test(Export20() == 20*20);
#endif
#if defined(__USE21__)
	test.Printf(_L("Test 21\n"));
	test(Export21() == 21*21);
#endif
#if defined(__USE22__)
	test.Printf(_L("Test 22\n"));
	test(Export22() == 22*22);
#endif
#if defined(__USE23__)
	test.Printf(_L("Test 23\n"));
	test(Export23() == 23*23);
#endif
#if defined(__USE24__)
	test.Printf(_L("Test 24\n"));
	test(Export24() == 24*24);
#endif
#if defined(__USE25__)
	test.Printf(_L("Test 25\n"));
	test(Export25() == 25*25);
#endif
#if defined(__USE26__)
	test.Printf(_L("Test 26\n"));
	test(Export26() == 26*26);
#endif
#if defined(__USE27__)
	test.Printf(_L("Test 27\n"));
	test(Export27() == 27*27);
#endif
#if defined(__USE28__)
	test.Printf(_L("Test 28\n"));
	test(Export28() == 28*28);
#endif
#if defined(__USE29__)
	test.Printf(_L("Test 29\n"));
	test(Export29() == 29*29);
#endif
#if defined(__USE30__)
	test.Printf(_L("Test 30\n"));
	test(Export30() == 30*30);
#endif
#if defined(__USE31__)
	test.Printf(_L("Test 31\n"));
	test(Export31() == 31*31);
#endif
#if defined(__USE32__)
	test.Printf(_L("Test 32\n"));
	test(Export32() == 32*32);
#endif
#if defined(__USE33__)
	test.Printf(_L("Test 33\n"));
	test(Export33() == 33*33);
#endif
#if defined(__USE34__)
	test.Printf(_L("Test 34\n"));
	test(Export34() == 34*34);
#endif
#if defined(__USE35__)
	test.Printf(_L("Test 35\n"));
	test(Export35() == 35*35);
#endif
#if defined(__USE36__)
	test.Printf(_L("Test 36\n"));
	test(Export36() == 36*36);
#endif
#if defined(__USE37__)
	test.Printf(_L("Test 37\n"));
	test(Export37() == 37*37);
#endif
#if defined(__USE38__)
	test.Printf(_L("Test 38\n"));
	test(Export38() == 38*38);
#endif
#if defined(__USE39__)
	test.Printf(_L("Test 39\n"));
	test(Export39() == 39*39);
#endif
#if defined(__USE40__)
	test.Printf(_L("Test 40\n"));
	test(Export40() == 40*40);
#endif
#if defined(__USE41__)
	test.Printf(_L("Test 41\n"));
	test(Export41() == 41*41);
#endif
#if defined(__USE42__)
	test.Printf(_L("Test 42\n"));
	test(Export42() == 42*42);
#endif
#if defined(__USE43__)
	test.Printf(_L("Test 43\n"));
	test(Export43() == 43*43);
#endif
#if defined(__USE44__)
	test.Printf(_L("Test 44\n"));
	test(Export44() == 44*44);
#endif
#if defined(__USE45__)
	test.Printf(_L("Test 45\n"));
	test(Export45() == 45*45);
#endif
#if defined(__USE46__)
	test.Printf(_L("Test 46\n"));
	test(Export46() == 46*46);
#endif
#if defined(__USE47__)
	test.Printf(_L("Test 47\n"));
	test(Export47() == 47*47);
#endif
#if defined(__USE48__)
	test.Printf(_L("Test 48\n"));
	test(Export48() == 48*48);
#endif
#if defined(__USE49__)
	test.Printf(_L("Test 49\n"));
	test(Export49() == 49*49);
#endif
#if defined(__USE50__)
	test.Printf(_L("Test 50\n"));
	test(Export50() == 50*50);
#endif
#if defined(__USE51__)
	test.Printf(_L("Test 51\n"));
	test(Export51() == 51*51);
#endif
#if defined(__USE52__)
	test.Printf(_L("Test 52\n"));
	test(Export52() == 52*52);
#endif
#if defined(__USE53__)
	test.Printf(_L("Test 53\n"));
	test(Export53() == 53*53);
#endif
#if defined(__USE54__)
	test.Printf(_L("Test 54\n"));
	test(Export54() == 54*54);
#endif
#if defined(__USE55__)
	test.Printf(_L("Test 55\n"));
	test(Export55() == 55*55);
#endif
#if defined(__USE56__)
	test.Printf(_L("Test 56\n"));
	test(Export56() == 56*56);
#endif
#if defined(__USE57__)
	test.Printf(_L("Test 57\n"));
	test(Export57() == 57*57);
#endif
#if defined(__USE58__)
	test.Printf(_L("Test 58\n"));
	test(Export58() == 58*58);
#endif
#if defined(__USE59__)
	test.Printf(_L("Test 59\n"));
	test(Export59() == 59*59);
#endif

	return ver;
	}
