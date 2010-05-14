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
// f32test\manager\t_clobbr.cpp
// 
//

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_CLOBBR"));

GLDEF_C void CallTestsL(void)
//
// Test writing to a file and changing its size
//
{
	test.Next(_L("Write file & change size"));

	RFile file;
	TInt r=file.Replace(TheFs,_L("test.dat"),EFileWrite);
	test_KErrNone(r);
	
	r=file.Write(0,_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));
	test_KErrNone(r);
	TBuf8<0x40> buf1;
	r=file.Read(0,buf1);
	test_Value(r, r == KErrNone&&buf1.Length()==36);
	if (buf1!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("1). *BAD*\n"));
	
	r=file.SetSize(511);
	test_KErrNone(r);
	TBuf8<0x40> buf2;
	r=file.Read(0,buf2);
	test_Value(r, r == KErrNone&&buf2.Length()==0x40);
	buf2.SetLength(36);
	if (buf2!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("2). *BAD*\n"));
	
	r=file.SetSize(512);
	test_KErrNone(r);
	TBuf8<0x40> buf3;
	r=file.Read(0,buf3);
	test_Value(r, r == KErrNone&&buf3.Length()==0x40);
	buf3.SetLength(36);
	if (buf3!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("3). *BAD*\n"));
	
	r=file.Write(0,_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));
	test_KErrNone(r);
	TBuf8<0x40> buf4;
	r=file.Read(0,buf4);
	test_Value(r, r == KErrNone&&buf4.Length()==0x40);
	buf4.SetLength(36);
	if (buf4!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("4). *BAD*\n"));
	r=file.SetSize(511);
	test_KErrNone(r);
	TBuf8<0x40> buf5;
	r=file.Read(0,buf5);
	test_Value(r, r == KErrNone&&buf5.Length()==0x40);
	buf5.SetLength(36);
	if (buf5!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("5). *BAD*\n"));
	r=file.SetSize(512);
	test_KErrNone(r);
	TBuf8<0x40> buf6;
	r=file.Read(0,buf6);
	test_Value(r, r == KErrNone&&buf6.Length()==0x40);
	buf6.SetLength(36);
	if (buf6!=_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"))
		test.Printf(_L("6). *BAD*\n"));
//
	
#if !defined(_UNICODE)
	test.Printf(_L("\n"));
	test.Printf(_L("1). %S\n"),&buf1);
	test.Printf(_L("2). %S\n"),&buf2);
	test.Printf(_L("3). %S\n"),&buf3);
	test.Printf(_L("4). %S\n"),&buf4);
	test.Printf(_L("5). %S\n"),&buf5);
	test.Printf(_L("6). %S\n"),&buf6);
#endif
	file.Close();
    }
