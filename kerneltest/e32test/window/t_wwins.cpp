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
// e32test\window\t_wwins.cpp
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
	test.Next(_L("User::InfoPrint"));
	TInt r=User::InfoPrint(_L("A short infoprint"));
	test(r==KErrNone);
	r=User::InfoPrint(_L("This infoprint is much longer.   ( 50  characters)"));
	test(r==KErrNone);
	User::After(10000);
	r=w.Create();
	TSize scsz;
	w.ScreenSize(scsz);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Init(_L("T_WWINS"),TSize(Min(50,scsz.iWidth-4),Min(15,scsz.iHeight-4)));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	for(TInt x=0;x<600;x++)
		{
		TBuf<0x40> b;
		b.Format(_L("%05d hello world\r\n"),x);
		w.Write(b);
		}
	w.Destroy();
	w.Close();
//
	RNotifier textWS;
	test.Next(_L("RNotify::Connect"));
	r=textWS.Connect();
	test(r==KErrNone);
	test.Next(_L("RNotify::Notify"));
	TRequestStatus stat;
	TPtrC line1(_L("This is a notifier."));
	TPtrC line2(_L("They stay on top."));
	TPtrC line3(_L(" A  longer  notifier  with  just  one  button. "));
	TPtrC null(KNullDesC);
	TPtrC butt1(_L("Esc"));
	TPtrC butt2(_L("Enter"));
	TInt val=-1;
	textWS.Notify(line1,line2,butt1,butt2,val,stat);
	User::WaitForRequest(stat);
	test(val==0||val==1);
	textWS.Notify(line3,null,butt2,null,val,stat);
	User::WaitForRequest(stat);
	test(val==1);
	test.Next(_L("User::InfoPrint again"));
	r=User::InfoPrint(_L("Infoprints stay on top..."));
	test(r==KErrNone);
	textWS.Close();
	test.Close();
	}

TInt Test2Window2(TAny* /*aDirective*/)
	{
	RConsole w;
	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TSize scsz;
	w.ScreenSize(scsz);
	r=w.Control(_L("+Scrollbars +Lock"));
	r=w.Init(_L("T_WWINS Window 2"),TSize(Min(50,scsz.iWidth-4),Min(15,scsz.iHeight-4)));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TConsoleKey k;
	TRequestStatus s;
	FOREVER
		{
		w.Read(k,s);
		FOREVER
			{
			if (s!=KRequestPending)
				break;
			else
				User::After(1000000);
			w.Control(_L("-Scrollbars"));
			if (s!=KRequestPending)
				break;
			else
				User::After(1000000);
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
	}

TInt Test2Window1(TAny* /*aDirective*/)
	{
	RConsole w;
	User::After(200000);
	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TSize scsz;
	w.ScreenSize(scsz);
	r=w.Init(_L("T_WWINS Window 1"),TSize(Min(50,scsz.iWidth-4),Min(15,scsz.iHeight-4)));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	w.Control(_L("+Maximised"));
	FOREVER
		User::After(10000000);
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
	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	TSize scsz;
	w.ScreenSize(scsz);
	r=w.Init(_L("T_WWINS Window 3"),TSize(Min(50,scsz.iWidth-4),Min(15,scsz.iHeight-4)));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	w.Control(_L("+Scrollbars"));
	TConsoleKey k;
	TKeyCode c;
	do
		{
		w.Read(k);
		c=k.Code();
		if(c<256)
			{
			TText a=(TText)c;
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



