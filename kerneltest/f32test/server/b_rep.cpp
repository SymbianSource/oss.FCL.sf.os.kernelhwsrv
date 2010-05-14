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
// f32test\server\b_rep.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("B_REP"));
LOCAL_D RFile TheFile;
GLDEF_D TFileName nameBuf1=_L("B_REP Test File.PAR");
GLDEF_D TBuf8<0x200> testPat1;
GLDEF_D TBuf8<0x200> chkPat1;

const TInt noTimes=500;
const TInt len=24;

void DoTest()
	{

	TBuf8<0x10> buf;
    TBuf8<0x10> testBuf;
	buf.SetLength(sizeof(TInt));
    TInt r;
	for (TInt numb=0;numb<noTimes;numb++)
        {
		if ((numb&0x1f)==0)
            test.Printf(_L("%d\r"),numb);

        // Replace file and write data
		r=TheFile.Replace(TheFs,nameBuf1,EFileStream);
		test_KErrNone(r);
		r=TheFile.Write(testPat1);
		test_KErrNone(r);
		
		Mem::Copy(&buf[0],&numb,sizeof(TInt));
		r=TheFile.Write(buf);
		test_KErrNone(r);

        // Seek to 0 and check data
		TInt pos=0; 
		r=TheFile.Seek(ESeekStart,pos);
		test_KErrNone(r);
		test(pos==0);
		r=TheFile.Read(chkPat1,len);
        test_KErrNone(r);
        test(chkPat1==testPat1);
		r=TheFile.Read(testBuf,sizeof(TInt));
        test_KErrNone(r);
		TInt chkNumb=*((TInt*)testBuf.Ptr());
        test(chkNumb==numb);

        // Close, then re-open file and check data
		TheFile.Close();
		r=TheFile.Open(TheFs,nameBuf1,EFileStream);
		test_KErrNone(r);
		r=TheFile.Read(chkPat1,len);
        test_KErrNone(r);
        test(chkPat1==testPat1);
		r=TheFile.Read(testBuf,sizeof(TInt));
        test_KErrNone(r);
		chkNumb=*((TInt*)testBuf.Ptr());
        test(chkNumb==numb);
		TheFile.Close();
		}
	test.Printf(_L("\n"));
	r=TheFs.Delete(nameBuf1);
	test_KErrNone(r);
	}

GLDEF_C void CallTestsL(void)
    {
    testPat1=_L8("TextProcessorPa");
	testPat1.Append(0);
	testPat1.Append(0);
	testPat1.Append(0);
	testPat1.Append(0x16);
	testPat1.Append(0);
	testPat1.Append(0);
	testPat1.Append(0);
	testPat1.Append(4);
	testPat1.Append(0x10);

	test.Start(_L("Root"));

	DoTest();

	test.Next(_L("Subdirectory"));
	gSessionPath=_L("\\F32-TST\\TEST1\\");
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	DoTest();

	test.End();
	}
