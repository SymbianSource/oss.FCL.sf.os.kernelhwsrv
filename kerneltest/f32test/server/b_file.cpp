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
// f32test\server\b_file.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32math.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("B_FILE"));

LOCAL_D RFile TheFile;
LOCAL_D TInt bret; // Expected error return
LOCAL_D TInt aret; // Actual error return
LOCAL_D TBuf8<10000> tbuf; // Test buffer
LOCAL_D TPtrC tbin(_S("\\F32-TST\\TEST.BIN"));
LOCAL_D TPtrC rndm(_S("\\F32-TST\\RANDOM.TST"));
LOCAL_D TPtrC tzzz(_S("\\F32-TST\\ZZZZZZ.ZZZ"));
LOCAL_D const TInt KRandomNumbers=1024;

LOCAL_C void bopen(TUint aMode)
//
// Open the binary file.
//
    {

    aret=TheFile.Open(TheFs,tbin,aMode);
    test(aret==bret);
    }

LOCAL_C void bcreate(TUint aMode)
//
// Open the binary file.
//
    {

    aret=TheFile.Create(TheFs,tbin,aMode);
    test(aret==bret);
    }

LOCAL_C void breplace(TUint aMode)
//
// Open the binary file.
//
    {

    aret=TheFile.Replace(TheFs,tbin,aMode);
    test(aret==bret);
    }

LOCAL_C void bwrite(TInt aLength)
//
// Write aLength bytes of test data at current position.
//
    {

	CheckDisk();
    test.Printf(_L("bwrite1,len=%u\n"),aLength);
    TInt pos=0; // Relative position zero
    aret=TheFile.Seek(ESeekCurrent,pos);
    test.Printf(_L("bwrite2,pos=%u\n"),pos);
    test_KErrNone(aret);
    TInt count=pos&0xff;
	tbuf.SetLength(aLength);
	TText8* p=(TText8*)tbuf.Ptr();
	TText8* pE=p+aLength;
    while (p<pE)
        {
        *p++=(TText8)count++;
        count&=0xff;
        }
    test.Printf(_L("bwrite3\n"));
    aret=TheFile.Write(tbuf);
    test.Printf(_L("bwrite4\n"));
    test(aret==bret);
	CheckDisk();
    }

LOCAL_C void bread(TInt aLength)
//
// Read and check aLength bytes of test data at current position.
//
    {

	CheckDisk();
    TInt pos=0; // Relative position zero
    aret=TheFile.Seek(ESeekCurrent,pos);
    test_KErrNone(aret);
    TInt count=pos&0xff;
    aret=TheFile.Read(tbuf,aLength);
    if (bret<KErrNone)
		{
        test(bret==aret);
		return;
		}
    test(((TInt)tbuf.Length())==bret);
	const TText8* p=tbuf.Ptr();
	const TText8* pE=p+bret;
    while (p<pE)
        {
        if (*p++!=count++)
			test.Panic(_L("bread data different"));
        count&=0xff;
        }
	CheckDisk();
    }

LOCAL_C void bposa(TInt aPos)
//
// Position absolute.
//
    {

	CheckDisk();
    TInt newpos=aPos;
    aret=TheFile.Seek(ESeekStart,newpos);
    test_KErrNone(aret);
    test(newpos==aPos);
	CheckDisk();
    }

LOCAL_C void bclose()
//
// Close the file.
//
    {

	CheckDisk();
    TheFile.Close();
	CheckDisk();
    }

LOCAL_C void btest1(TUint aMode)
//
// Binary file tests.
//
    {

    test.Start(_L("BTEST1..."));
    bret=0;
    breplace(aMode|EFileWrite);
    bret=0; bread(1);
    bret=0; bwrite(1); bposa(0l);
    bret=1; bread(2);
    bret=0; bread(1);
    bret=0; bposa(0l);
    bret=1; bread(1);
    bret=0; bread(1); bret=0;
    bclose();
    bret=KErrAlreadyExists;bcreate(aMode|EFileWrite);bret=0;
    bopen(aMode|EFileRead);
    bret=KErrAccessDenied; bwrite(1); bret=0;
    bclose();
	aret=TheFile.Open(TheFs,tzzz,EFileRead);
	test_Value(aret, aret == KErrNotFound);
	test.End();
    }

LOCAL_C void btest2(TUint aMode)
//
// Binary file tests.
//
    {

    test.Start(_L("BTEST2..."));
    bret=0;
    breplace(aMode|EFileWrite);
    bwrite(11);
    bposa(0);
    bret=5; bread(5); bret=0;
    bwrite(45);
    bposa(5);
    bret=45; bread(45); bret=0;
    bwrite(1000);
    bposa(600);
    bret=300; bread(300); bret=0;
    bclose();
    bopen(aMode|EFileWrite);
    bposa(5);
    bret=5; bread(5); 
    bret=1000; bread(1000); bret=0;
    bclose();
    bopen(aMode|EFileWrite);
    bposa(KMaxTInt);
    bwrite(50);
    bposa(0);
    bret=1100; bread(1100); bret=0;
    aret=TheFile.Flush();
    test_KErrNone(aret);
    aret=TheFile.SetSize(2000);
    test_KErrNone(aret);
    TInt pos=0;
    aret=TheFile.Seek(ESeekCurrent,pos);
    test_Value(aret, aret == KErrNone && pos==1100);
    pos=0;
    aret=TheFile.Seek(ESeekEnd,pos);
    test_Value(aret, aret == KErrNone && pos==2000);
    bclose();
	test.End();
    }

LOCAL_C void rndtest(TUint aMode)
//
// Tests the file handling by writing a file of random numbers, 
// closing the file, reseeding the random number generator with
// the same number, then reading the file back again, checking
// it against the generator.
//
    {

    TInt cnt;
    test.Start(_L("RNDTEST..."));
    TInt64 seed(0),zero(0);
	aret=TheFile.Replace(TheFs,rndm,EFileWrite|aMode);
	test_KErrNone(aret);
    for (cnt=0;cnt<KRandomNumbers;cnt++)
        {
		TBuf8<0x10> b;
		b.Format(TPtrC8((TUint8*)"%8x"),Math::Rand(seed));
		aret=TheFile.Write(b);
		test_KErrNone(aret);
        }
	TheFile.Close();
//
    test.Next(_L("Reading back"));
    seed=zero;
	aret=TheFile.Open(TheFs,rndm,aMode);
	test_KErrNone(aret);
    for (cnt=0;cnt<KRandomNumbers;cnt++)
        {
		TBuf8<8> b;
		b.Format(TPtrC8((TUint8*)"%8x"),Math::Rand(seed));
		TBuf8<8> r;
		aret=TheFile.Read(r);
		test_KErrNone(aret);
		test(b==r);
        }
	TheFile.Close();
    aret=TheFs.Delete(rndm);
	test_KErrNone(aret);
//
	test.End();
    }

LOCAL_C void testAutoClose()
//
// Tests TAutoClose template class
//
    {

    test.Start(_L("TAutoClose..."));
	TAutoClose<RFile> f;
	aret=f.iObj.Replace(TheFs,rndm,EFileWrite);
	test_KErrNone(aret);
    TInt64 seed;
    for (TInt cnt=0;cnt<KRandomNumbers;cnt++)
        {
		TBuf8<0x10> b;
		b.Format(TPtrC8((TUint8*)"%8x"),Math::Rand(seed));
		aret=f.iObj.Write(b);
		test_KErrNone(aret);
        }
	test.End();
    }

LOCAL_C void readWithNegativeLengthTest()
{
    test.Start(_L("Read with Negative Length Test..."));
    TInt	ret;
	TRequestStatus status = KRequestPending;
	TheFile.Open(TheFs,tbin,EFileRead);
 	ret = TheFile.Read(0,tbuf,-1);			// sync
 	test_Value(ret, ret == KErrArgument);
 	TheFile.Read(0,tbuf,-1,status);		// async
 	User::WaitForRequest(status);
 	test(status.Int() == KErrArgument);
	TheFile.Close();
	test.End();
}

LOCAL_C void readWithNegativeLengthTestForEmptyFile() 
	
	{

	test.Start(_L("Read with Negative Length Test(For EmptyFile)..."));
	RFile f;             
	MakeFile(_L("C:\\F32-TST\\TFILE\\hello2.txt"));
	TInt r=f.Open(TheFs,_L("C:\\F32-TST\\TFILE\\hello2.txt"),EFileRead); 
	test_KErrNone(r);

	TBuf8<0x100> a;
	test.Next(_L("Check Negative length when file is empty"));
	r=f.Read(a, -10);
	test_Value(r, r == KErrArgument);
	r=f.Read(0,a, -1);
	test_Value(r, r == KErrArgument);
	r=f.Read(0,a, -10);
	test_Value(r, r == KErrArgument);
	TRequestStatus	stat1;
	f.Read(0,a,-5,stat1);
	User::WaitForRequest(stat1);
	test(stat1.Int() == KErrArgument);
	f.Read(a,-5,stat1);
	User::WaitForRequest(stat1);
	test(stat1.Int() == KErrArgument);
	f.Close();	
	test.End();
	
	}

GLDEF_C void CallTestsL()
//
// Call tests that may leave
//
	{
	
	testAutoClose();
    btest1(EFileStream);
    btest2(EFileStream);
    btest1(EFileStreamText);
    btest2(EFileStreamText);
    rndtest(EFileStream);
	readWithNegativeLengthTest();
	readWithNegativeLengthTestForEmptyFile();
	
	}
