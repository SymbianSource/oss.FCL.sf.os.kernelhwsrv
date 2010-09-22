// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\t_colour.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32twin.h>
#include <e32debug.h>
#include <e32svr.h>

const TInt KNumColors=256;

LOCAL_D RTest test(_L("T_COLOUR"));

LOCAL_C void SimulateKeyPress(TStdScanCode aScanCode)
    {
    TRawEvent eventDown;
    eventDown.Set(TRawEvent::EKeyDown, aScanCode);
    UserSvr::AddEvent(eventDown);
    TRawEvent eventUp;
    eventUp.Set(TRawEvent::EKeyUp, aScanCode);
    UserSvr::AddEvent(eventUp);    
    }

class ColorTest
	{
public:
	void ModesAndText(TVideoMode aMode);
	void Colors();
	RConsole con;
	};

void ColorTest::ModesAndText(TVideoMode aMode)
	{

	TInt r=con.SetMode(aMode);
	con.SetCursorPosAbs(TPoint(1,4));
	if(r!=KErrNone)
        {
		con.Write(_L("Not supported."));
		test.Printf(_L("Not supported.\r\n"));
    	TConsoleKey key;
    	SimulateKeyPress(EStdKeyEnter);
    	con.Read(key);
        }
	else
		{
		con.Write(_L("Normal text..."));
		con.SetCursorPosAbs(TPoint(1,6));
		con.SetTextAttribute(ETextAttributeBold);
		con.Write(_L("Bold text..."));
		con.SetCursorPosAbs(TPoint(1,8));
		con.SetTextAttribute(ETextAttributeInverse);
		con.Write(_L("Inverted text..."));
		con.SetCursorPosAbs(TPoint(1,10));
		con.SetTextAttribute(ETextAttributeHighlight);
		con.Write(_L("Highlighted text..."));
		Colors();
		}
	con.ClearScreen();
	con.SetTextAttribute(ETextAttributeNormal);
	con.SetCursorPosAbs(TPoint(3,2));
	}
	
void ColorTest::Colors()
	{
	RConsole col;

	col.Create();
	col.Control(_L("-Vis"));
	col.Init(_L("Colours"),TSize(18,18));
	col.Control(_L("+Max"));
	col.SetWindowPosAbs(TPoint(1,0));
	col.Control(_L("+Vis -Cursor"));
	for(TInt t=0;t<KNumColors;t++)
		{
		if(!(t%16))
			{
			col.SetCursorPosAbs(TPoint(1,1+t/16));
			}
		col.SetTextColors(0,t);
		col.Write(_L(" "));
		}
	
	for(TInt i=0;i<KNumColors;i++)
	    {
        TUint8 red, green, blue;
        col.GetPaletteEntry(i, red, green, blue);
        }
	
	col.SetPaletteEntry(1,20,250,250);	
	
	TInt j = 0;
	// Window background color
	for(j=0;j<KNumColors;j++)
	    {
	    col.SetUIColors(j,0,0);
	    }
	// Border color
	for(j=0;j<KNumColors;j++)
	    {
	    col.SetUIColors(0,j,0);
	    }
	// Screen color
	for(j=0;j<KNumColors;j++)
	    {
	    col.SetUIColors(0,0,j);
	    }
	
	TConsoleKey key;
	SimulateKeyPress(EStdKeyEnter);
	col.Read(key);
	col.ClearScreen();
	col.Destroy();
	}

GLDEF_C TInt E32Main()
    {
    test.Title();
    __UHEAP_MARK;
    
    test.Start(_L("Testing RConsole"));
	ColorTest t;

	TInt r = t.con.Init(_L("Colour Test"),TSize(28,12));
	if (r != KErrNone)
		{
		RDebug::Printf("Could not create text console: %d", r);
		return r;
		}
	
	TInt ret = t.con.SetSize(TSize(10,10));
	test_Equal(KErrNotSupported, ret);
	 
	t.con.Control(_L("+Max"));
	t.con.SetCursorPosAbs(TPoint(3,2));
	test.Next(_L("Testing mode EMono:"));
	t.con.Write(_L("Testing mode EMono:"));
	t.ModesAndText(EMono);
	test.Next(_L("Testing mode EGray4:"));
	t.con.Write(_L("Testing mode EGray4:"));
	t.ModesAndText(EGray4);
	test.Next(_L("Testing mode EGray16:"));
	t.con.Write(_L("Testing mode EGray16:"));
	t.ModesAndText(EGray16);
	test.Next(_L("Testing mode EColor256:"));
	t.con.Write(_L("Testing mode EColor256:"));
	t.ModesAndText(EColor256);
	test.Next(_L("Testing mode EColor4K:"));
	t.con.Write(_L("Testing mode EColor4K:"));
	t.ModesAndText(EColor4K);
	test.Next(_L("Testing mode EColor64K:"));
	t.con.Write(_L("Testing mode EColor64K:"));
	t.ModesAndText(EColor64K);
	test.Next(_L("Testing mode EColor16M:"));
	t.con.Write(_L("Testing mode EColor16M:"));
	t.ModesAndText(EColor16M);
		
	__UHEAP_MARKEND;
	test.End();
	return(0);
    }
