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
// f32test\server\t_parse.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_PARSE"));
TPtrC test_string=_L("Y:\\");

LOCAL_C void Test1()
//
// Test all TParse methods
//
	{

	test.Start(_L("Test all TParse methods"));
	TBuf<16> relatedFiles(_L(".CCC"));
	TBuf<16> defaultPath(_L("C:\\"));
	TParse parser;
	TInt r=parser.Set(_L("\\WWW\\XXX\\YYY\\ZZZ\\AAA"),&relatedFiles,&defaultPath);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\AAA.CCC"));
	test(parser.Drive()==_L("C:"));
	test(parser.Path()==_L("\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.DriveAndPath()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.Name()==_L("AAA"));
	test(parser.Ext()==_L(".CCC"));
	test(parser.NameAndExt()==_L("AAA.CCC"));
	test(parser.DrivePresent()==EFalse);
	test(parser.PathPresent());
	test(parser.NamePresent());
	test(parser.ExtPresent()==EFalse);
	test(parser.NameOrExtPresent());
	test(parser.IsRoot()==EFalse);
	test(parser.IsWild()==EFalse);
	test(parser.IsNameWild()==EFalse);
	test(parser.IsExtWild()==EFalse);
	r=parser.SetNoWild(_L("\\WWW\\XXX\\YYY\\ZZZ\\AAA.EXT"),&relatedFiles,&defaultPath);
	test_KErrNone(r);
	test(parser.PopDir()==KErrNone);
	test(parser.AddDir(_L("BBB"))==KErrNone);
	test.End();
	}

LOCAL_C void Test2()
//
// Test multiple PopDirs
//
	{

	test.Start(_L("Test multiple PopDirs"));
	TParse parser;
	TInt r=parser.Set(_L("\\WWW\\XXX\\YYY\\ZZZ\\"),NULL,NULL);
//	TParsePtrC parser(_L("\\WWW\\XXX\\YYY\\ZZZ\\"));
	test_KErrNone(r);
	r=parser.PopDir();
	test_KErrNone(r);
	test(parser.Path()==_L("\\WWW\\XXX\\YYY\\"));
	r=parser.PopDir();
	test_KErrNone(r);
	test(parser.Path()==_L("\\WWW\\XXX\\"));
	r=parser.PopDir();
	test_KErrNone(r);
	test(parser.Path()==_L("\\WWW\\"));
	r=parser.PopDir();
	test_KErrNone(r);
	test(parser.Path()==_L("\\"));
	r=parser.PopDir();
	test_Value(r, r == KErrGeneral);
//
	test(parser.Set(_L("C:\\Documents\\.TXT"),NULL,NULL)==KErrNone);
	test(parser.PopDir()==KErrNone);
	test(parser.FullName()==_L("C:\\.TXT"));
	test.End();
	}

LOCAL_C void Test3()
//
// Test conflicting drive letters
//
	{

	test.Start(_L("Test conflicting default drive letters"));
	TParse parser;
    TPtrC one=_L("\\ONE\\");
    TPtrC null=_L("\\");
    TPtrC x=_L("X:");
    TPtrC x2=_L("X:\\");
    TPtrC z=_L("Z:");
	TInt r=parser.Set(_L("Z:\\Hello"),&one,&null);
	test_KErrNone(r);
	test(parser.FullName()==_L("Z:\\Hello"));
    TPtrC sht=_L("*.SHT");
    r=parser.Set(_L("Z:"),&sht,&x);
	test_KErrNone(r);
	test(parser.FullName()==_L("Z:*.SHT"));
	r=parser.Set(_L("Hello"),&z,&x2);
	test_KErrNone(r);
	test(parser.FullName()==_L("Z:\\Hello"));
	r=parser.Set(_L("W:\\Hello"),&z,&x2);
	test_KErrNone(r);
	test(parser.FullName()==_L("W:\\Hello"));
    TPtrC abcdefg=_L("abcdefg");
    TPtrC onetwo=_L("X:\\ONE\\TWO\\.CCC");
	r=parser.Set(_L("W:"),&abcdefg,&onetwo);
	test_KErrNone(r);
	test(parser.FullName()==_L("W:\\ONE\\TWO\\abcdefg.CCC"));
    TPtrC y=_L("Y:");
    TPtrC xhello=_L("X:\\HELLO\\");
    r=parser.Set(_L("World"),&y,&xhello);
	test_KErrNone(r);
	test(parser.FullName()==_L("Y:\\HELLO\\World"));
    TPtrC xhelloext=_L("X:\\HELLO\\.EXT");
    r=parser.Set(_L("World"),&y,&xhelloext);
	test_KErrNone(r);
	test(parser.FullName()==_L("Y:\\HELLO\\World.EXT"));
	test.End();
	}

LOCAL_C void Test4()
//
// Conflicting relative path drives and names
//
	{

	test.Start(_L("Test conflicting relative drive letters"));
	TParse parser;
    TPtrC xone=_L("X:\\ONE\\");
    TPtrC y=_L("Y:\\");
	TInt r=parser.Set(_L("Z:\\Hello"),&xone,&y);
	test_KErrNone(r);
	test(parser.FullName()==_L("Z:\\Hello"));
    TPtrC zone=_L("Z:\\ONE\\");
    TPtrC xnew=_L("X:\\NEW\\");
    r=parser.Set(_L("\\Hello"),&zone,&xnew);
	test_KErrNone(r);
	test(parser.FullName()==_L("Z:\\Hello"));
    TPtrC aone=_L("A:\\ONE\\");
    TPtrC anew=_L("A:\\NEW\\");
    r=parser.Set(_L("A:Hello"),&aone,&anew);
	test_KErrNone(r);
	test(parser.FullName()==_L("A:\\ONE\\Hello"));
    TPtrC a=_L("A:\\");
    r=parser.Set(_L("Hello"),&a,&xnew);
	test_KErrNone(r);
	test(parser.FullName()==_L("A:\\Hello"));
	r=parser.Set(_L("Hello"),&aone,&xnew);
	test_KErrNone(r);
	test(parser.FullName()==_L("A:\\ONE\\Hello"));
	test.End();
	}


LOCAL_C void Test5()
//
// Test illegal paths
//
	{

	test.Start(_L("Test errors returned by illegal paths"));
	TParse parser;
	TInt r=parser.Set(_L("FOO\\"),NULL,NULL);
	test_Value(r, r == KErrBadName);
	r=parser.Set(_L("C:\\FOO\\\\"),NULL,NULL);
	test_KErrNone(r);
	test.End();
	}

LOCAL_C void Test6()
//
// Test AddDir
//
	{

	test.Start(_L("Test AddDir"));
	TParse parser;
	test(parser.Set(_L("C:\\"),NULL,NULL)==KErrNone);
	test(parser.IsRoot());
	test(parser.FullName()==_L("C:\\"));
	test(parser.AddDir(_L("HELLO"))==KErrNone);
	test(parser.IsRoot()==EFalse);
	test(parser.FullName()==_L("C:\\HELLO\\"));
	test(parser.AddDir(_L("BYEBYE"))==KErrNone);
	test(parser.IsRoot()==EFalse);
	test(parser.FullName()==_L("C:\\HELLO\\BYEBYE\\"));
	test(parser.PopDir()==KErrNone);
	test(parser.IsRoot()==EFalse);
	test(parser.FullName()==_L("C:\\HELLO\\"));
	test(parser.PopDir()==KErrNone);
	test(parser.IsRoot());
	test(parser.FullName()==_L("C:\\"));
//
	test(parser.AddDir(_L(""))==KErrNone);
	test(parser.IsRoot());
	test(parser.FullName()==_L("C:\\"));
	test(parser.AddDir(_L("HELLO"))==KErrNone);
	test(parser.IsRoot()==EFalse);
	test(parser.FullName()==_L("C:\\HELLO\\"));
	test(parser.AddDir(_L(""))==KErrNone);
	test(parser.IsRoot()==EFalse);
	test(parser.FullName()==_L("C:\\HELLO\\"));
//
	test(parser.Set(_L("C:\\Documents\\.TXT"),NULL,NULL)==KErrNone);
	test(parser.AddDir(_L("Documents"))==KErrNone);
	test(parser.FullName()==_L("C:\\Documents\\Documents\\.TXT"));
	test.End();
	}

LOCAL_C void Test7()
//
// Test TParsePtr
//
	{

	test.Start(_L("Test TParsePtr"));
	TBuf<128> nameBuf=_L("C:\\WWW\\XXX\\YYY\\ZZZ\\AAA.CCC");
	TParsePtr parser(nameBuf);

	test(parser.FullName()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\AAA.CCC"));
	test(parser.Drive()==_L("C:"));
	test(parser.Path()==_L("\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.DriveAndPath()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.Name()==_L("AAA"));
	test(parser.Ext()==_L(".CCC"));
	test(parser.NameAndExt()==_L("AAA.CCC"));
	test(parser.DrivePresent());
	test(parser.PathPresent());
	test(parser.NamePresent());
	test(parser.ExtPresent());
	test(parser.NameOrExtPresent());
	test(parser.IsRoot()==EFalse);
	test(parser.IsWild()==EFalse);
	test(parser.IsNameWild()==EFalse);
	test(parser.IsExtWild()==EFalse);
	
	test(parser.AddDir(_L("HELLO"))==KErrNone);
	test(parser.Path()==_L("\\WWW\\XXX\\YYY\\ZZZ\\HELLO\\"));

	TBuf<16> shortName=_L("1234567812345678");
	TParsePtr parser2(shortName);
	test(parser2.FullName()==_L("1234567812345678"));
	test(parser2.Path()==_L(""));
	test(parser2.DriveAndPath()==_L(""));
	test(parser2.Ext()==_L(""));
	test(parser2.Name()==_L("1234567812345678"));
	test(parser2.AddDir(_L("TOOBIG"))==KErrGeneral);
	test.End();
	}

LOCAL_C void Test8()
//
// Test TParsePtrC
//
	{
	
	test.Start(_L("Test TParsePtrC"));
	TBuf<128> nameBuf=_L("C:\\WWW\\XXX\\YYY\\ZZZ\\AAA.CCC");
	TParsePtrC parser(nameBuf);

	test(parser.FullName()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\AAA.CCC"));
	test(parser.Drive()==_L("C:"));
	test(parser.Path()==_L("\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.DriveAndPath()==_L("C:\\WWW\\XXX\\YYY\\ZZZ\\"));
	test(parser.Name()==_L("AAA"));
	test(parser.Ext()==_L(".CCC"));
	test(parser.NameAndExt()==_L("AAA.CCC"));
	test(parser.DrivePresent());
	test(parser.PathPresent());
	test(parser.NamePresent());
	test(parser.ExtPresent());
	test(parser.NameOrExtPresent());
	test(parser.IsRoot()==EFalse);
	test(parser.IsWild()==EFalse);
	test(parser.IsNameWild()==EFalse);
	test(parser.IsExtWild()==EFalse);
	test.End();
	}

LOCAL_C void Test9()
//
// Test names with leading spaces
//
	{

	test.Start(_L("Test names with leading spaces"));
	TParse parser;
	TBuf<16> nameBuf=_L("   name.txt");
	TBuf<16> pathBuf=_L("\\PATH\\");
	
	TInt r=parser.Set(pathBuf,NULL,&nameBuf);
	test_KErrNone(r);
	test(parser.FullName()==_L("\\PATH\\   name.txt"));
	r=parser.Set(_L(""),&nameBuf,&pathBuf);
	test_KErrNone(r);
	test(parser.FullName()==_L("\\PATH\\   name.txt"));
	r=parser.Set(_L("   name.txt"),NULL,&pathBuf);
	test_KErrNone(r);
	test(parser.FullName()==_L("\\PATH\\   name.txt"));
	r=parser.Set(nameBuf,&pathBuf,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("\\PATH\\   name.txt"));
	
	TBuf<16> badPath=_L("   \\PATH\\");
	r=parser.Set(_L("C:\\"),NULL,&badPath);
	test_Value(r, r == KErrBadName);
	r=parser.Set(_L("C:\\"),&badPath,NULL);
	test_Value(r, r == KErrBadName);

	TBuf<16> spacePath=_L("\\  PATH\\");
	r=parser.Set(_L("C:"),&nameBuf,&spacePath);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\  PATH\\   name.txt"));

	TBuf<32> spacename=_L("\\  name   .   txt  ");
	r=parser.Set(_L("C:"),&spacename,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\  name   .   txt"));

// Illegal (?) values

	TBuf<16> pureSpace=_L("     ");
	r=parser.Set(_L("C:\\NAME\\"),NULL,&pureSpace);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\")); // Trims right off name
	r=parser.Set(_L("C:\\NAME\\   "),NULL,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\"));

	TBuf<16> spacePlusExt=_L("    .   ext  ");
	r=parser.Set(_L("C:\\NAME\\"),NULL,&spacePlusExt);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\    .   ext")); // Trims right off ext
	r=parser.Set(_L("C:\\NAME\\    .   ext   "),NULL,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\    .   ext"));

	TBuf<32> pathSpace=_L("\\asdf\\zxcv\\   \\asdf\\");
	r=parser.Set(_L("C:"),NULL,&pathSpace);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\asdf\\zxcv\\   \\asdf\\")); // Leaves spaces in path
	r=parser.Set(_L("C:\\NAME\\ \\alt.sdf"),NULL,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\ \\alt.sdf"));


	TBuf<32> zeroPath=_L("\\asdf\\wqer\\\\asdf\\");
	r=parser.Set(_L("NAME.TXT"),NULL,&zeroPath);
	test_KErrNone(r);
	test(parser.FullName()==_L("\\asdf\\wqer\\\\asdf\\NAME.TXT")); // Leaves zerolength path
	r=parser.Set(_L("C:\\NAME\\\\alt.sdf"),NULL,NULL);
	test_KErrNone(r);
	test(parser.FullName()==_L("C:\\NAME\\\\alt.sdf"));
	test.End();
	}

LOCAL_C void Test10()
//
// Test a very long path
//
	{

	test.Next(_L("Test a Path > 256 chars"));
	TBuf<16> pathPart=_L("\\2345678");
	TBuf<512> testPath;

	for(TInt i=0;i<63;i++)
		testPath+=pathPart;

	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	TParse parse;
	r=fs.Parse(testPath,parse);
	test_Value(r, r == KErrBadName);
	fs.Close();

	TFileName longFileName;
	longFileName.SetLength(254);
//	Mem::Fill((TUint8*)longFileName.Ptr(),254,'A');
	Mem::Fill((TUint8*)longFileName.Ptr(),254*sizeof(TText),'A');
	longFileName[0]='\\';
	longFileName[253]='\\';
	r=parse.Set(longFileName,&test_string,NULL);
	test_KErrNone(r);
	r=parse.PopDir();
	test_KErrNone(r);

	longFileName[123]='\\';
	r=parse.Set(longFileName,&test_string,NULL);
	test_KErrNone(r);
	r=parse.PopDir();
	test_KErrNone(r);
	TPtrC startPath((TText*)longFileName.Ptr(),124);
	test(parse.Path()==startPath);

	TPtrC endPath((TText*)longFileName.Ptr()+124,252-124+1);
	r=parse.AddDir(endPath);
	test_KErrNone(r);
	test(parse.Path()==longFileName);
	}

LOCAL_C void DoTestsL()
//
// Call all tests
//
	{

	Test1();
	Test2();
	Test3();
	Test4();
	Test5();
	Test6();
	Test7();
	Test8();
	Test9();
	Test10();
	}

GLDEF_C void CallTestsL(void)
//
// Test resource counting
//
    {
	test.Title();

	test.Start(_L("Starting TParse Tests ..."));
	DoTestsL();

	test.End();
	test.Close();
	return;
    }

