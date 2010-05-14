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
// f32test\server\b_mtst.cpp
// Tests file deleteing
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

RTest test(_L("B_MTST"));
//LOCAL_D	RFs TheFs;

const TInt nTimes=20;

GLDEF_D RFile file1;
GLDEF_D RFile file2;
GLDEF_D RFile file3;
GLDEF_D RFile file4;
GLDEF_D RFile file5;
GLDEF_D TBuf8<0x200> buf;
GLDEF_D TBuf8<0x200> buf1;
GLDEF_D TBuf8<0x200> buf2;
GLDEF_D TBuf8<0x200> buf3;
GLDEF_D TBuf8<0x200> buf4;
GLDEF_D TBuf8<0x200> buf5;
GLDEF_D TFileName nameBuf1=_L("B_MTST File 1");
GLDEF_D TFileName nameBuf2=_L("B_MTST File 2");
GLDEF_D TFileName nameBuf3=_L("B_MTST File 3");
GLDEF_D TFileName nameBuf4=_L("B_MTST File 4");
GLDEF_D TFileName nameBuf5=_L("B_MTST File 5");

const TInt len1=163;
const TInt len2=31;
const TInt len3=271;
const TInt len4=128;
const TInt len5=14;

LOCAL_C void checkPattern()
    {
    test.Printf(_L("Opening: %S\n"),&nameBuf1);
    TInt r=file1.Open(TheFs,nameBuf1,EFileStream);
	test_KErrNone(r);
    test.Printf(_L("Opening: %S\n"),&nameBuf2);
	r=file2.Open(TheFs,nameBuf2,EFileStream);
	test_KErrNone(r);
    test.Printf(_L("Checking test pattern...\n"));
	for (TInt i=0 ; i<nTimes ; i++)
        {
        r=file1.Read(buf,len1);
		test_KErrNone(r);
        TInt j;
		for (j=0 ; j< len1 ; j++)
            test(buf[j]==j);
        r=file2.Read(buf,len2);
		test_KErrNone(r);
        for (j=0 ; j< len2 ; j++)
            test(buf[j]=='A');
        }
    file1.Close();
    file2.Close();
    }


GLDEF_C void CallTestsL(void)
    {
	test.Title();
    test.Next(_L("Generate test patterns"));
	TInt i;
    for (i=0 ; i<len1 ; i++)
        buf1.Append((TUint8)i); 
    for (i=0 ; i<len2 ; i++)
        buf2.Append('A');       
    for (i=0 ; i<len3 ; i++)
        buf3.Append('B');       
    for (i=0 ; i<len4 ; i++)
        buf4.Append('C');       
    for (i=0 ; i<len5 ; i++)
        buf5.Append('D');       

    TInt r=file1.Create(TheFs,nameBuf1,EFileStream|EFileWrite);
	test_KErrNone(r);
    test.Printf(_L("Created: %S\n"),&nameBuf1);
    r=file2.Create(TheFs,nameBuf2,EFileStream|EFileWrite);
	test_KErrNone(r);
    test.Printf(_L("Created: %S\n"),&nameBuf2);
    r=file3.Create(TheFs,nameBuf3,EFileStream|EFileWrite);
	test_KErrNone(r);
    test.Printf(_L("Created: %S\n"),&nameBuf3);
    r=file4.Create(TheFs,nameBuf4,EFileStream|EFileWrite);
	test_KErrNone(r);
    test.Printf(_L("Created: %S\n"),&nameBuf4);
    r=file5.Create(TheFs,nameBuf5,EFileStream|EFileWrite);
	test_KErrNone(r);
    test.Printf(_L("Created: %S\n"),&nameBuf5);
    
    test.Next(_L("Writing test pattern..."));
    for (i=0 ; i<nTimes ; i++)
        {
        r=file1.Write(buf1,len1);
		test_KErrNone(r);
		r=file2.Write(buf2,len2);
		test_KErrNone(r);
		r=file3.Write(buf3,len3);
		test_KErrNone(r);
		r=file4.Write(buf4,len4);
		test_KErrNone(r);
		r=file5.Write(buf5,len5);
		test_KErrNone(r);
        }
    file1.Close();
	file2.Close();

    test.Next(_L("Check pattern"));
    checkPattern();

    test.Next(_L("Delete"));
    test.Printf(_L("Deleting: %S\n"),&nameBuf1);
    r=TheFs.Delete(nameBuf1);
	test_KErrNone(r);
    test.Printf(_L("Deleting: %S\n"),&nameBuf2);
	r=TheFs.Delete(nameBuf2);
	test_KErrNone(r);
    
    file3.Close();
    file4.Close();
    file5.Close();
    
    test.Printf(_L("Deleting: %S\n"),&nameBuf3);
    r=TheFs.Delete(nameBuf3);
	test_KErrNone(r);
    test.Printf(_L("Deleting: %S\n"),&nameBuf4);
	r=TheFs.Delete(nameBuf4);
	test_KErrNone(r);
    test.Printf(_L("Deleting: %S\n"),&nameBuf5);
	r=TheFs.Delete(nameBuf5);
	test_KErrNone(r);

//	test.Close();
    }

