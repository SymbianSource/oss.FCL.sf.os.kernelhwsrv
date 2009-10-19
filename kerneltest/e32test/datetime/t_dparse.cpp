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
// e32test\datetime\t_dparse.cpp
// 
//

#include <e32test.h>

RTest test(_L("T_DPARSE"));
TInt offset=0;

LOCAL_C TInt DateTimeParse(TDateTime& aDateTime,const TDesC& aDes,TInt aCenturyOffset=0)
	{
	TTime time;
	TInt r=time.Parse(aDes,aCenturyOffset);
	if (r>=0)
		aDateTime=time.DateTime();
	return r;
	}

LOCAL_D TInt ConvertDesToDateTimeInteractively()
	{
	TKeyCode code;
	TBuf<32> buf;
	buf.SetLength(0);
	FOREVER
		{
		code = test.Console()->Getch();
		if (code==EKeyEnter || code==EKeyLineFeed)
			break;
		if (code==EKeyEscape)
			return KErrGeneral;
		if ((code==EKeyDelete || code==EKeyBackspace) && buf.Length()>0)
			{
			TChar del(code);
			TBuf<1> delBuf;
			delBuf.Append(del);
			test.Printf(delBuf);
			buf.SetLength(buf.Length()-1);
			continue;
			}
		if (buf.Length()==0 && code==EKeyF1)
			{
			test.Printf(_L("Enter a two digit value. "));	
			TKeyCode code1 = test.Console()->Getch();
			TKeyCode code2 = test.Console()->Getch();
			TChar ch1(code1);
			TChar ch2(code2);
			TBuf<2> centuryOffset;
			centuryOffset.Append(ch1);
			centuryOffset.Append(ch2);
			TLex lex(centuryOffset);
			if (lex.Val(offset)!=KErrNone)
				return KErrGeneral;
			test.Printf(_L("The century offset is set to %d\n"),offset);
			if (ConvertDesToDateTimeInteractively()==KErrGeneral)
				return KErrGeneral;
			}
		TChar ch(code);
		buf.Append(ch);
		TBuf<1> charBuf;
		charBuf.Append(ch);
		test.Printf(charBuf);
		} 
	test.Printf(_L(" = "));
	TDateTime dateTime;
	TInt error = DateTimeParse(dateTime,buf,offset);
	TInt year = dateTime.Year();
	TInt month = dateTime.Month()+1;
	TInt day = dateTime.Day()+1;
	TInt hour = dateTime.Hour(); 
	TInt min = dateTime.Minute();
	TInt sec = dateTime.Second();
	TInt msec = dateTime.MicroSecond();
	switch(error)
		{
		case EParseTimePresent:
			test.Printf(_L("%d:%d:%d.%d  "),hour,min,sec,msec);
			test.Printf(_L("Time\n"));
			break;
		case EParseDatePresent:
			test.Printf(_L("%d/%d/%d  "),day,month,year);
			test.Printf(_L("Date\n"));
			break;
		case EParseTimePresent|EParseDatePresent:
			test.Printf(_L("%d:%d:%d.%d %d/%d/%d  "),hour,min,sec,msec,day,month,year);
			test.Printf(_L("DateAndTime\n"));
			break;
		default:
			test.Printf(_L("error\n"));
			break;
		}
	if (ConvertDesToDateTimeInteractively()==KErrGeneral)
		return KErrGeneral;
	return KErrNone;
	}


TInt E32Main()
	{
	test.Start(_L("Begin tests"));
	test.Console()->Printf(_L("Convert a Des To a DateTime.\n"));
	test.Printf(_L("Enter a Date, a Time or a Date & Time.\n"));
	test.Printf(_L("Press Escape to exit tests.\n"));
	test.Printf(_L("Press the F1 key to set the century offset.\n"));
	ConvertDesToDateTimeInteractively();
	test.End();
	return KErrNone;
	}


