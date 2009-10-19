// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>
#include <e32twin.h>
#include <e32debug.h>

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
    	TConsoleKey key;
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
	for(TInt t=0;t<256;t++)
		{
		if(!(t%16))
			col.SetCursorPosAbs(TPoint(1,1+t/16));
		col.SetTextColors(0,t);
		col.Write(_L(" "));
		}
	TConsoleKey key;
	col.Read(key);
	col.Destroy();
	}

GLDEF_C TInt E32Main()
    {
	ColorTest t;

	TInt r = t.con.Init(_L("Colour Test"),TSize(28,12));
	if (r != KErrNone)
		{
		RDebug::Printf("Could not create text console: %d", r);
		return r;
		}
	
	t.con.Control(_L("+Max"));
	t.con.SetCursorPosAbs(TPoint(3,2));
	t.con.Write(_L("Testing mode EMono:"));
	t.ModesAndText(EMono);
	t.con.Write(_L("Testing mode EGray4:"));
	t.ModesAndText(EGray4);
	t.con.Write(_L("Testing mode EGray16:"));
	t.ModesAndText(EGray16);
	t.con.Write(_L("Testing mode EColor256:"));
	t.ModesAndText(EColor256);

	return(0);
    }
