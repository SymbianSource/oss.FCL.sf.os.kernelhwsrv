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
// e32test\window\t_wjpwin.cpp
// (this test program is formerly known as T_GUI.H)
// 
//

#include <e32test.h>
#include <e32twin.h>

enum TFault
	{
	EConnect,
	EConsole
	};

LOCAL_C void Fault(TFault aFault)
//
// Panic the program.
//
	{

	User::Panic(_L("T_WWINS fault"),aFault);
	}

class TestGui
	{
public:
	void Test1();
	void Test2();
private:
    };

void TestGui::Test1()
	{
	RConsole w;
	RTest test(_L("T_WWINS"));
	test.Title();
 	test.Start(_L("GUI Test 1\n"));
	TInt r=w.Init(_L("T_WWINS"),TSize(50,15));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	for(TInt x=0;x<200;x++)
		{
		TBuf<0x40> b;
#if defined(_UNICODE)	// K.K
		unsigned char unicode_chars[] = {0x25,0x30,0x33,0x64,0x20,0x90,0xa2,0x8a,0x45,0x82,0xe6,0x20,0x82,0xb1,0x82,0xf1,0x82,0xc9,0x82,0xbf,0x82,0xcd,13,10,0,0};
		//b.Format(_L("%03d ê¢äEÇÊ Ç±ÇÒÇ…ÇøÇÕ\r\n"),x);
		b.Format(TPtrC16((unsigned short*)unicode_chars),x);
#else	// K.K
		b.Format(_L("%03d hello world\r\n"),x);
#endif	// K.K
		w.Write(b);
		}
	w.Destroy();
	w.Close();
	test.Close();
	}

TInt Test2Window2(TAny* /*aDirective*/)
	{
	RConsole w;
	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Control(_L("+Scrollbars +Lock"));
	r=w.Init(_L("T_WWINS Window 2"),TSize(50,15));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TConsoleKey k;
	TRequestStatus s;
	TInt x;
	FOREVER
		{
		w.Read(k,s);
		FOREVER
			{
			for(x=0;x<10000000 && s==KRequestPending;x++) {};
			if (s!=KRequestPending)
				break;
			w.Control(_L("-Scrollbars"));
			for(x=0;x<10000000 && s==KRequestPending;x++) {};
			if (s!=KRequestPending)
				break;
			w.Control(_L("+Scrollbars"));
			}
		User::WaitForRequest(s);
		if (s==KErrNone)
			{
			TChar a=(TUint)k.Code();
			if (a.IsPrint())
				{
				TBuf<1> b(1);
				b[0]=(TText)a;
				w.Write(b);
				}
			}
		else
			User::Panic(_L("Read-Key"),0);
		}
//	return(KErrNone);
	}

TInt Test2Window1(TAny* /*aDirective*/)
	{
	RConsole w;
	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Control(_L("-Visibilty +Lock"));
	r=w.Init(_L("T_WWINS Window 1"),TSize(50,15));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	FOREVER
		{
		TInt x;
		for(x=0;x<200;x++)
			{
			w.Write(_L("Hello world\r\n"));
			}
		w.Control(_L("+Visibilty"));
		for(x=0;x<200;x++)
			{
			w.Write(_L("Hello world\r\n"));
			}
		w.Control(_L("-Visibility"));
		}
//	return(0);
	}

void TestGui::Test2()
	{
	TInt x;
	RThread t1, t2;
	RTest test(_L("T_WWINS"));
	test.Title();
 	test.Start(_L("GUI Test 2\n"));
	x=t1.Create(_L("Thread1"),Test2Window1,KDefaultStackSize,0x2000,0x2000,(TAny*)1);
	if(x==0) t1.Resume();
	x=t2.Create(_L("Thread2"),Test2Window2,KDefaultStackSize,0x2000,0x2000,(TAny*)2);
	if(x==0) t2.Resume();
	RConsole w;
	TInt r=w.Init(_L("T_WWINS Window 3"),TSize(50,15));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TConsoleKey k;
	TKeyCode c;
	do
		{
		w.Read(k);
		c=k.Code();
		if(c<256)
			{
			TText a=(TText)c;
#if defined(_UNICODE)	// K.K
			if (a >= 0x0061 && a <= 0x006a) a -= 0x0020;
			else {
				if (a >= 0x0041 && a <= 0x005a) a+= 0xff00;
			}
#endif	// K.K
			TPtrC s(&a,1);
			w.Write(s);
			}
		else
			{
			if(c==EKeyLeftArrow) w.SetCursorPosRel(TPoint(-1,0));
			if(c==EKeyRightArrow) w.SetCursorPosRel(TPoint(1,0));
			if(c==EKeyUpArrow) w.SetCursorPosRel(TPoint(0,-1));
			if(c==EKeyDownArrow) w.SetCursorPosRel(TPoint(0,1));
			if(c==EKeyF1) w.Control(_L("+Cursor"));
			if(c==EKeyF2) w.Control(_L("-Cursor"));
			if(c==EKeyF3) w.Control(_L("+Scroll"));
			if(c==EKeyF4) w.Control(_L("-Scroll"));
			if(c==EKeyF5) w.Control(_L("+Lock"));
			if(c==EKeyF6) w.Control(_L("-Lock"));
			if(c==EKeyF7) w.Control(_L("+Wrap"));
			if(c==EKeyF8) w.Control(_L("-Wrap"));
			if(c==EKeyF9) w.ClearToEndOfLine();
			}
		} while(k.Code()!=EKeyEscape);
	w.Close();
	t1.Terminate(0);
	t1.Close();
	t2.Terminate(0);
	t2.Close();
	test.End();
	}

// To get some data or bss into ths program
TInt dataMaker=0;

GLDEF_C TInt E32Main()
//
// Test the various kernel types.
//
    {
	TestGui t;
	t.Test1();
	t.Test2();
	return(0);
    }



