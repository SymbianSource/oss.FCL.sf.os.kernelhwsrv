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
// e32test\window\t_calib.cpp
// 
//

#include <e32test.h>
#include <e32twin.h>
#include <e32hal.h>
#include <e32svr.h>

RConsole w;

enum TFault
	{
	EConnect,
	EConsole
	};

LOCAL_C void printf(TRefByValue<const TDesC> aFmt,...)
//
// Print to the console
//
	{

	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	TInt r=w.Write(aBuf);
	__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Write-Console"),0));
	}

LOCAL_C void Fault(TFault aFault)
//
// Panic the program.
//
	{

	User::Panic(_L("T_CALIB fault"),aFault);
	}

GLDEF_C void DrawBlob(TPoint aPos)
    {
    TPoint pos(aPos.iX/8-1,aPos.iY/10-1);
    w.SetCursorPosAbs(TPoint(0,0));
	TBuf<0x100> aBuf;
	aBuf.AppendFormat(_L("(%d,%d)    "),aPos.iX,aPos.iY);
    w.Write(aBuf);
    if (pos.iX!=0)
        {
        w.SetCursorPosAbs(pos);
        w.Write(_L("*"));
        }
    }

GLDEF_C void TestBlob()
//
// Draw blob when digitizer pressed
//
    {

    FOREVER
        {
        TConsoleKey k;
        w.Read(k);
        if((TInt)k.Code()==27 && k.Type()==EKeyPress)
            break;
        if(k.Type()==EMouseClick)
            DrawBlob(k.MousePos());
        };
    w.ClearScreen();
    }

GLDEF_C TInt E32Main()
//
// Calibrate digitizer
//
    {

	TInt r=w.Create();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Init(_L("T_CALIB window"),TSize(KConsFullScreen,KConsFullScreen));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Control(_L("+Maximised"));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Control(_L("+Inform"));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
	r=w.Control(_L("+Pointer"));
	__ASSERT_ALWAYS(r==KErrNone,Fault(EConsole));
    TConsoleKey k;

    TestBlob();

    printf(_L("Please touch the three points, in the order TL, BL, BR\r\n"));

    TDigitizerCalibration cal;
    UserHal::CalibrationPoints(cal);

    printf(_L("Points are at: (%d,%d), (%d,%d), (%d,%d)\r\n")
                                                ,cal.iTl.iX,cal.iTl.iY
                                                ,cal.iBl.iX,cal.iBl.iY
                                                ,cal.iBr.iX,cal.iBr.iY);

    DrawBlob(cal.iTl);
    DrawBlob(cal.iBl);
    DrawBlob(cal.iBr);

    // Set digitiser to 'bypass' calibration

//    UserHal::SetXYInputCalibration(cal);

    TPoint pos(0,6);

    w.SetCursorPosAbs(pos);

    TInt count=0;
    do
        {
        w.Read(k);
        if(k.Type()==EMouseClick)
            {
            TPoint mouse=k.MousePos();
            printf(_L("You touched point %d,%d\r\n"),mouse.iX,mouse.iY);
            switch(count++)
                {
                case 0:
                    cal.iTl=mouse;
                    break;
                case 1:
                    cal.iBl=mouse;
                    break;
                case 2:
                    cal.iBr=mouse;
                }
            }
        } while(count!=3);

    UserHal::SetXYInputCalibration(cal);

    printf(_L("Digitizer calibrated! Click away!\r\n"));

	// Test to validate when invalid calibration input values are supplied.
	
   	cal.iTl.iX = 10;
   	cal.iTl.iY = 20;
   	
   	cal.iBl.iX = 10;
   	cal.iBl.iY = 20;
   	
   	cal.iBr.iX = 10;
   	cal.iBr.iY = 20;
   	
   	r = UserHal::SetXYInputCalibration(cal);
   	
   	if (r != KErrArgument)
		// Test failure panic.
   		User::Panic(_L("T_CALIB Test Failure"),84);


    TestBlob();

    w.Read(k);
	return(0);
    }

