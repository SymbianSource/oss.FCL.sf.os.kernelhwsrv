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
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "fs_utils.h"

#include "f32_test_utils.h"

using namespace F32_Test_Utils;

RTest test(_L("T_FILE"));

TBool gShortFileNamesSupported = EFalse;

static TBuf8<1500> gBuf;

TInt gDriveNum = -1;

static void testShare()
//
// Test file sharing.
//
	{

	test.Start(_L("Test exclusive sharing"));
	MakeFile(_L("TESTER"));

/*
	Extra code to answer a question about a potential WINS bug.  WINS Elocal returns
	KErrAccessDenied to the write operation but EFat returns
	KErrNone...

	RFile f1;
	TInt r=f1.Open(TheFs,_L("TESTER"),EFileRead|EFileShareAny);
	test_KErrNone(r);
	RFile f2;
	r=f2.Open(TheFs,_L("TESTER"),EFileWrite|EFileShareAny);
	test_KErrNone(r);

	r=f2.Write(_L("0"));
	test.Printf(_L("returned %d"),r);

	f1.Close();
	f2.Close();

	r=TheFs.Delete(_L("TESTER"));
	test_KErrNone(r);
*/

	RFile f1;
	TInt r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_KErrNone(r);
	RFile f2;
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_Value(r, r == KErrInUse);
	f1.Close();
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite|EFileShareExclusive);
	test_KErrNone(r);
	f1.Close();

	test.Next(_L("Test readers only sharing"));
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite|EFileShareReadersOnly);
	test_Value(r, r == KErrArgument);
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite|EFileShareReadersOnly);
	test_Value(r, r == KErrArgument);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	f1.Close();
	f2.Close();

	test.Next(_L("Test any sharing"));
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse);
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r);
	f1.Close();
	f2.Close();

	test.End();
	}

static void testChangeMode()
//
// Test changing the share mode of a file between EFileShareReadersOnly <-> EFileShareExclusive
//
	{

	test.Start(_L("Test change mode"));
	RFile f1;
	RFile f2;
	TInt r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_KErrNone(r); // Opened exclusive
	r=f1.ChangeMode(EFileShareReadersOnly);
	test_KErrNone(r); // Change to readers only
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r); // Open as reader
	r=f1.ChangeMode(EFileShareExclusive);
	test_Value(r, r == KErrAccessDenied); // Change back to exclusive fails
	r=f2.ChangeMode(EFileShareExclusive);
	test_Value(r, r == KErrAccessDenied); // Change to exclusive fails
	f1.Close(); // Close other reader
	r=f2.ChangeMode(EFileShareExclusive);
	test_KErrNone(r); // Change to exclusive succeeds.
	f2.Close();

	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r); // Opened readers only
	r=f1.ChangeMode(EFileShareExclusive);
	test_KErrNone(r); // Change to exclusive
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse); // Open as reader fails
	r=f1.ChangeMode(EFileShareReadersOnly);
	test_KErrNone(r); // Change to readers only
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r); // Open as reader
	r=f1.ChangeMode(EFileShareExclusive);
	test_Value(r, r == KErrAccessDenied); // Change back to exclusive fails
	r=f2.ChangeMode(EFileShareExclusive);
	test_Value(r, r == KErrAccessDenied); // Change to exclusive fails
	f1.Close(); // Close other reader
	r=f2.ChangeMode(EFileShareExclusive);
	test_KErrNone(r); // Change to exclusive succeeds.
	f2.Close();

	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite|EFileShareExclusive);
	test_KErrNone(r); // Opened exclusive for writing
	r=f1.ChangeMode(EFileShareReadersOnly);
	test_Value(r, r == KErrAccessDenied); // Change to readers fails
	r=f1.ChangeMode(EFileShareExclusive);
	test_KErrNone(r); // No change ok
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse); // Open as reader fails
	f1.Close();

	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r); // Opened share any
	r=f1.ChangeMode(EFileShareExclusive);
	test_Value(r, r == KErrAccessDenied); // Change to exclusive fails
	r=f1.ChangeMode(EFileShareReadersOnly);
	test_Value(r, r == KErrAccessDenied); // Change to readers only fails
	f1.Close();

	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_KErrNone(r); // Opened exclusive
	r=f1.ChangeMode(EFileShareAny);
	test_Value(r, r == KErrArgument); // Change to share any fails KErrArgument
	r=f1.ChangeMode((TFileMode)42);
	test_Value(r, r == KErrArgument); // Change to random value fails
	f1.Close();
	test.End();
	}

static void testReadFile()
//
// Test read file handling.
//
	{

	test.Start(_L("Test read file"));
	RFile f,ZFile;
	
    TInt r=f.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText);
	test_KErrNone(r);
	
    TFileName fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	
    r=ZFile.Open(TheFs,fn,EFileStreamText);
	test_KErrNone(r);

	// check the file on the Z: drive his read-only
	TEntry fileAtt;
	r=TheFs.Entry(fn,fileAtt);
	test_KErrNone(r);
	test((fileAtt.iAtt & KEntryAttReadOnly) == KEntryAttReadOnly);


	test.Next(_L("Read file"));
	TBuf8<0x100> a,b;
	
    for(;;)
		{
		r=f.Read(b);
		test_KErrNone(r);
		
        r=ZFile.Read(a);
		test_KErrNone(r);
		
        test(CompareBuffers(a, b));
		
        if (b.Length()<b.MaxLength())
			break;
		}

	b.SetLength(10);
	r=f.Read(b);
	test_KErrNone(r);
	test(b.Length()==0);
	f.Close();
	ZFile.Close();

	test.Next(_L("Read way beyond the end of the file"));
	r=f.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText);
	test_KErrNone(r);
	r=f.Read(3000000,gBuf);
	test_KErrNone(r);
	f.Close();

	test.Next(_L("Write way beyond the end of the file"));
	r=f.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileWrite);
	test_KErrNone(r);
	gBuf.SetLength(10);
	r=f.Write(3000000,gBuf);
	test_KErrNone(r);
	f.Close();
	test.End();
	}

static void testMultipleReadFile()
//
// Test multiple read file handling.
//
	{

	test.Start(_L("Test multiple read file"));
	
    RFile f1;
	TInt r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	
    RFile f2;
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);

	test.Next(_L("Read file"));
	
    TBuf8<0x100> b1;
    TBuf8<0x100> b2;
    
    for(;;)
		{
        r=f1.Read(b1);
		test_KErrNone(r);
        
		r=f2.Read(b2);
		test_KErrNone(r);
		
        test(CompareBuffers(b1, b2));

		if (b1.Length()<b1.MaxLength())
			break;
		}

	test.Next(_L("Close file"));
	f1.Close();
	f2.Close();

	test.End();
	}

static void testWriteFile()
//
// Test write file handling.
//
	{
	test.Start(_L("Test write file"));
	RFile file;
	TFileName fn = _L("File.File");
	TBuf8<16> testData=_L8("testData");

	// write test 1
	TInt r=file.Replace(TheFs,fn,EFileStreamText);
	test_KErrNone(r);

	test.Next(_L("Write file"));

	r=file.Write(testData);
	test_KErrNone(r);

	file.Close();

	// test write modes
	// test writing with EFileRead
	r=file.Open(TheFs,fn,EFileStreamText|EFileRead);
	test_KErrNone(r);

	test.Next(_L("Write file"));
	r=file.Write(testData);
	test_Value(r, r == KErrAccessDenied);
	file.Close();

	// test writing with EFileWrite
	r=file.Open(TheFs,fn,EFileStreamText|EFileWrite);
	test_KErrNone(r);

	test.Next(_L("Write file"));
	r=file.Write(testData);
	test_KErrNone(r);
	file.Close();

	// test writing with share mode EFileShareExclusive
	r=file.Open(TheFs,fn,EFileStreamText|EFileWrite|EFileShareExclusive);
	test_KErrNone(r);

	test.Next(_L("Write file"));
	r=file.Write(testData);
	test_KErrNone(r);
	file.Close();

	// test writing with share mode EFileShareReadersOnly (fails with KErrArgument)
	r=file.Open(TheFs,fn,EFileStreamText|EFileWrite|EFileShareReadersOnly);
	test_Value(r, r == KErrArgument);

	// test writing with share mode EFileShareReadersOrWriters
	r=file.Open(TheFs,fn,EFileStreamText|EFileWrite|EFileShareReadersOrWriters);
	test_KErrNone(r);

	test.Next(_L("Write file"));
	r=file.Write(testData);
	test_KErrNone(r);
	file.Close();

	// test writing with share mode EFileShareAny
	r=file.Open(TheFs,fn,EFileStreamText|EFileWrite|EFileShareAny);
	test_KErrNone(r);

	test.Next(_L("Write file"));
	r=file.Write(testData);
	test_KErrNone(r);
	file.Close();

	// tidy up
	r=TheFs.Delete(fn);
	test_KErrNone(r);

	test.End();
	}

static void CopyFileToTestDirectory()
//
// Make a copy of the file in ram
//
	{

	TFileName fn = _L("Z:\\TEST\\T_FILE.CPP");
	fn[0] = gExeFileName[0];
	TParse f;
	TInt r;
	r=TheFs.Parse(fn,f);
	test_KErrNone(r);
	TParse fCopy;
	r=TheFs.Parse(f.NameAndExt(),fCopy);
	test_KErrNone(r);

	RFile f1;
	r=f1.Open(TheFs,f.FullName(),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	RFile f2;
	r=f2.Replace(TheFs,fCopy.FullName(),EFileWrite);
	test_KErrNone(r);
	TBuf8<512> copyBuf;
	TInt rem;
	r=f1.Size(rem);
	test_KErrNone(r);
	TInt pos=0;
	while (rem)
		{
		TInt s=Min(rem,copyBuf.MaxSize());
		r=f1.Read(pos,copyBuf,s);
		test_KErrNone(r);
		test(copyBuf.Length()==s);
		r=f2.Write(pos,copyBuf,s);
		test_KErrNone(r);
		pos+=s;
		rem-=s;
		}
	f1.Close();
	f2.Close();
	}

static void testFileText()
//
// Test TFileText class methods
//
	{

	test.Next(_L("Test file text"));
	TPtrC record[5];
	record[0].Set(_L("First record"));
	record[1].Set(_L("Second record"));
	record[2].Set(_L("Third record"));
	record[3].Set(_L("Fourth record"));
	record[4].Set(_L("Fifth record"));

	RFile f;
	TInt r=f.Replace(TheFs,_L("TEXTFILE.TXT"),0);
	test_KErrNone(r);
	TFileText textFile;
	textFile.Set(f);
	TInt i=0;
	for (i=0;i<5;i++)
		{
		r=textFile.Write(record[i]);
		test_KErrNone(r);
		}
	r=textFile.Seek(ESeekStart);
	test_KErrNone(r);
	TBuf<16> recBuf;
	for(i=0;i<5;i++)
		{
		r=textFile.Read(recBuf);
		test_KErrNone(r);
		test(recBuf==record[i]);
		}
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrEof);
	test(recBuf.Length()==0);
	f.Close();

	test.Next(_L("Test dosfile terminator"));
	TPtrC8 trecord[7];
	TPtrC tTextrecord[7];
	tTextrecord[0].Set(_L("First record\r\n"));
	tTextrecord[1].Set(_L("Second record\r\n"));
	tTextrecord[2].Set(_L("Third record\r\n"));
	tTextrecord[3].Set(_L("Fourth record\r\n"));
	tTextrecord[4].Set(_L("Fifth record\r\n"));
	tTextrecord[5].Set(_L("Sixth record\n\r"));
	tTextrecord[6].Set(_L("Seventh record\n"));
	trecord[0].Set((TUint8*)tTextrecord[0].Ptr(),tTextrecord[0].Length()*sizeof(TText));
	trecord[1].Set((TUint8*)tTextrecord[1].Ptr(),tTextrecord[1].Length()*sizeof(TText));
	trecord[2].Set((TUint8*)tTextrecord[2].Ptr(),tTextrecord[2].Length()*sizeof(TText));
	trecord[3].Set((TUint8*)tTextrecord[3].Ptr(),tTextrecord[3].Length()*sizeof(TText));
	trecord[4].Set((TUint8*)tTextrecord[4].Ptr(),tTextrecord[4].Length()*sizeof(TText));
	trecord[5].Set((TUint8*)tTextrecord[5].Ptr(),tTextrecord[5].Length()*sizeof(TText));
	trecord[6].Set((TUint8*)tTextrecord[6].Ptr(),tTextrecord[6].Length()*sizeof(TText));
	r=f.Replace(TheFs,_L("TEXTFILE.TXT"),0);
	test_KErrNone(r);
	for(i=0;i<7;i++)
		{
		TBuf8<256> buf;
		buf.Copy(trecord[i]);
		r=f.Write(buf);
		test_KErrNone(r);
		}
	textFile.Set(f);
	textFile.Seek(ESeekStart);
	for(i=0;i<5;i++)
		{
		r=textFile.Read(recBuf);
		test_KErrNone(r);
		test(recBuf==record[i]);
		}
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("Sixth record"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("\rSeventh record"));
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrEof);
	test(recBuf.Length()==0);
	f.Close();

	test.Next(_L("Test read with bufferSize == dataSize"));
	r=f.Replace(TheFs,_L("TEXTFILE.TXT"),0);
	test_KErrNone(r);
	record[0].Set(_L("1234567890123456"));
//	trecord[0].Set(_L8("1234567890123456\r\n"));
//	trecord[1].Set(_L8("1234567890123456\n"));

	TPtrC tmpTextrecord;
	tmpTextrecord.Set(_L("1234567890123456\r\n"));
	trecord[0].Set((TUint8*)tmpTextrecord.Ptr(),tmpTextrecord.Length()*sizeof(TText));

	tmpTextrecord.Set(_L("1234567890123456\n"));
	trecord[1].Set((TUint8*)tmpTextrecord.Ptr(),tmpTextrecord.Length()*sizeof(TText));

	for (i=0;i<2;i++)
		{
		r=f.Write(trecord[i]);
		test_KErrNone(r);
		}
	textFile.Set(f);
	textFile.Seek(ESeekStart);
	for(i=0;i<2;i++)
		{
		r=textFile.Read(recBuf);
		test_KErrNone(r);
		test(recBuf==record[0]);
		}
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrEof);
	test(recBuf.Length()==0);
	f.Close();

	test.Next(_L("Read into a buffer < recordSize"));
	TBuf<8> smallBuf;
	r=f.Open(TheFs,_L("TEXTFILE.txt"),0);
	test_KErrNone(r);
	textFile.Set(f);
	for(i=0;i<2;i++)
		{
		r=textFile.Read(smallBuf);
		test_Value(r, r == KErrTooBig);
		test(smallBuf==_L("12345678"));
		}
	f.Close();

	test.Next(_L("Nasty cases: 1) \\r \\n split over buffer boundary"));
	r=f.Replace(TheFs,_L("TEXTFILE.txt"),0);
	test_KErrNone(r);
	HBufC* largeRecord=HBufC::NewL(600);
	largeRecord->Des().SetLength(250);
	largeRecord->Des().Fill('A');
	largeRecord->Des()[249]='\n';
	TPtrC8 bufPtr;
	bufPtr.Set((TUint8*)largeRecord->Ptr(),largeRecord->Size()); // Size() returns length in bytes
	r=f.Write(bufPtr);
	test_KErrNone(r);
	TBuf<16> boundaryBuf=_L("12345\r\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	r=f.Write(trecord[0]);
	test_KErrNone(r);

	textFile.Set(f);
	textFile.Seek(ESeekStart);
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrTooBig);
	test(recBuf==_L("AAAAAAAAAAAAAAAA"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("12345"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==record[0]);
	f.Close();

	test.Next(_L("Nasty cases: 2) \\r on buffer boundary"));
	r=f.Replace(TheFs,_L("TEXTFILE.txt"),0);
	test_KErrNone(r);
	largeRecord->Des().SetLength(250);
	largeRecord->Des().Fill('A');
	largeRecord->Des()[249]='\n';
	bufPtr.Set((TUint8*)largeRecord->Ptr(),largeRecord->Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	boundaryBuf=_L("12345\rxyz\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	r=f.Write(trecord[0]);
	test_KErrNone(r);

	textFile.Set(f);
	textFile.Seek(ESeekStart);
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrTooBig);
	test(recBuf==_L("AAAAAAAAAAAAAAAA"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("12345\rxyz"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==record[0]);
	f.Close();

	test.Next(_L("Nasty cases: 3) record size > buffer size"));
	r=f.Replace(TheFs,_L("TEXTFILE.txt"),0);
	test_KErrNone(r);
	largeRecord->Des().SetLength(600);
	largeRecord->Des().Fill('Z');
	largeRecord->Des()[511]='\r';
	largeRecord->Des()[599]='\n';
	bufPtr.Set((TUint8*)largeRecord->Ptr(),largeRecord->Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	boundaryBuf=_L("12345\rxyz\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	r=f.Write(trecord[0]);
	test_KErrNone(r);

	textFile.Set(f);
	textFile.Seek(ESeekStart);
	r=textFile.Read(recBuf);
	test_Value(r, r == KErrTooBig);
	test(recBuf==_L("ZZZZZZZZZZZZZZZZ"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("12345\rxyz"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==record[0]);

	TBuf<601> bigBuf;
	TPtrC largePtr((TText*)largeRecord->Ptr(),(largeRecord->Length()-1));
	textFile.Seek(ESeekStart);
	r=textFile.Read(bigBuf);
	test_KErrNone(r);
	test(bigBuf==largePtr);
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==_L("12345\rxyz"));
	r=textFile.Read(recBuf);
	test_KErrNone(r);
	test(recBuf==record[0]);
	f.Close();

	User::Free(largeRecord);
	}

static void testFileTextEndRecord()
//
// Test terminating record
//
	{

	test.Next(_L("Test FileText last record has no terminator"));
	RFile f;
	TInt r=f.Replace(TheFs,_L("TextFile"),0);
	test_KErrNone(r);
	TPtrC8 bufPtr;
	TBuf<16>boundaryBuf=_L("Record1\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	boundaryBuf=_L("Record2\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);
	boundaryBuf=_L("Record3\n");
	bufPtr.Set((TUint8*)boundaryBuf.Ptr(),boundaryBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);

	TFileText fText;
	fText.Set(f);
	r=fText.Seek(ESeekStart);
	test_KErrNone(r);
	TBuf<32> recBuf;
	r=fText.Read(recBuf);
	test_KErrNone(r);
	test(recBuf.MatchF(_L("record1"))!=KErrNotFound);
	r=fText.Read(recBuf);
	test_KErrNone(r);
	test(recBuf.MatchF(_L("record2"))!=KErrNotFound);
	r=fText.Read(recBuf);
	test_KErrNone(r);
	test(recBuf.MatchF(_L("record3"))!=KErrNotFound);
	r=fText.Read(recBuf);
	test_Value(r, r == KErrEof);
	test(recBuf.Length()==0);
	f.Close();

	TBuf<0x100> bigBuf(0x100);
	bigBuf.Fill('A');
	r=f.Replace(TheFs,_L("TextFile"),0);
	test_KErrNone(r);

    bufPtr.Set((TUint8*)bigBuf.Ptr(),bigBuf.Size());
	r=f.Write(bufPtr);
	test_KErrNone(r);

	fText.Set(f);
	r=fText.Seek(ESeekStart);
	test_KErrNone(r);
	bigBuf.SetLength(0);
	r=fText.Read(bigBuf);
	test.Printf(_L("fText.Read returns %d\n"),r);
	test_KErrNone(r);
	test.Printf(_L("BigBuf.Length()==%d\n"),bigBuf.Length());
	test(bigBuf.Length()==0x100);
	r=fText.Read(bigBuf);
	test_Value(r, r == KErrEof);
	test(bigBuf.Length()==0);
	f.Close();
	}

static void testFileNames()
//
// Test file names
//
	{

	test.Next(_L("Test temp filenames specify drive"));
	TFileName tempFileName;
	RFile f;
	TInt r=f.Temp(TheFs,_L(""),tempFileName,EFileRead);
	test_KErrNone(r);
	TParse p;
	p.Set(tempFileName,NULL,NULL);
	test(p.DrivePresent());
	test(p.PathPresent());
	test(p.NamePresent());
	test(p.ExtPresent());
	f.Close();

	r=f.Replace(TheFs,_L("WELCOMETO"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	r=f.Replace(TheFs,_L("WELCOMETO.WRD"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	}

// Nasty hack - mask attributes returned by RFile::Att() with this.
// File server used to do this but that stopped the XIP attribute on the ROM file system
// from being returned. It should really be up to the file system to return only
// the attributes which it supports rather than having the file server unilaterally
// mask off any attributes which don't exist on FAT.
#define ATT_MASK 0x3f
static void testFileAttributes()
//
// Test the archive attribute gets set
//
	{

	test.Next(_L("Archive att is set after creation"));
	RFile f;
	TInt r=TheFs.Delete(_L("FILEATT.ARC"));
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=f.Create(TheFs,_L("FILEATT.ARC"),EFileRead);
	test_KErrNone(r);
	TUint atts;
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==KEntryAttArchive);
	TEntry fileAtt;
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);

	test.Next(_L("Archive att is set after a write"));
	TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);
	r=f.Open(TheFs,_L("FILEATT.ARC"),EFileWrite);
	test_KErrNone(r);
	r=f.Write(_L8("Hello World"));
	test_KErrNone(r);
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);

	test.Next(_L("Archive att is set after setsize"));
	TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);
	r=f.Open(TheFs,_L("FILEATT.ARC"),EFileWrite);
	test_KErrNone(r);
	r=f.SetSize(447);
	test_KErrNone(r);
	TInt size;
	r=f.Size(size);
	test_KErrNone(r);
	test(size==447);
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);

	test.Next(_L("Archive att is not set after open"));
	r=TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttArchive);
	test_KErrNone(r);
	r=f.Open(TheFs,_L("FILEATT.ARC"),EFileWrite);
	test_KErrNone(r);
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==0);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);

	test.Next(_L("Archive att is not set after a read"));
	TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);
	r=f.Open(TheFs,_L("FILEATT.ARC"),EFileWrite);
	test_KErrNone(r);
	TBuf8<16> readBuf;
	r=f.Read(readBuf);
	test_KErrNone(r);
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==0);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==0);

	test.Next(_L("Archive att is set after replace"));
	r=f.Replace(TheFs,_L("FILEATT.ARC"),EFileWrite);
	test_KErrNone(r);
	r=f.Att(atts);
	test_KErrNone(r);
	test((atts&ATT_MASK)==KEntryAttArchive);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);
	f.Close();
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==KEntryAttArchive);

	test.Next(_L("Read only bit can be unset"));
	r=TheFs.SetAtt(_L("FILEATT.ARC"),KEntryAttReadOnly|KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttHidden);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttArchive));

	r=TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttArchive));

	r=TheFs.SetAtt(_L("FILEATT.ARC"),KEntryAttReadOnly|KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetAtt(_L("FILEATT.ARC"),0,KEntryAttHidden);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttArchive));

	TTime time(0);
	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,KEntryAttReadOnly|KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,0,KEntryAttHidden);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttArchive));

	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttArchive));

	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,KEntryAttReadOnly|KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttReadOnly|KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttHidden|KEntryAttArchive));

	r=TheFs.SetEntry(_L("FILEATT.ARC"),time,0,KEntryAttHidden);
	test_KErrNone(r);
	r=TheFs.Entry(_L("FILEATT.ARC"),fileAtt);
	test_KErrNone(r);
	test(fileAtt.iAtt==(KEntryAttArchive));

	test.Next(_L("Cashing the 'read-only' attribute"));
	const TDesC& fname = _L("TEST.RO");

	// Test RO attribute after creating a file
	r=f.Create(TheFs,fname,EFileWrite);
	test_KErrNone(r);
	r=f.SetAtt(KEntryAttReadOnly,0);
	test_KErrNone(r);
	r=f.Write(_L8("Hello World"));
	test_KErrNone(r);					// <-- here!
	f.Close();

	// Test we can't open for write or delete a RO file
	r=f.Open(TheFs,fname,EFileWrite);
	test_Value(r, r == KErrAccessDenied);
	r=TheFs.Delete(fname);
	test_Value(r, r == KErrAccessDenied);

	// Tidy up and re-create test file
	r=TheFs.SetAtt(fname,0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Delete(fname);
	test_KErrNone(r);
	r=f.Create(TheFs,fname,EFileWrite);
	test_KErrNone(r);
	f.Close();

	// Test RO attribute after opening a file
	r=f.Open(TheFs,fname,EFileWrite);
	test_KErrNone(r);
	r=f.SetAtt(KEntryAttReadOnly,0);
	test_KErrNone(r);
	r=f.Write(_L8("Hello World"));
	test_KErrNone(r);
	f.Close();


	test.Next(_L("Internal attribute can't be read"));

	r=TheFs.SetAtt(fname,0,KEntryAttReadOnly);
	test_KErrNone(r);

	r=f.Open(TheFs,fname,EFileWrite);
	test_KErrNone(r);

	r=f.SetAtt(KEntryAttReadOnly, 0);	// this will set internal attribut KEntryAttModified
	test_KErrNone(r);

	// copied \sf\os\kernelhwsrv\userlibandfileserver\fileserver\inc\common.h
	const TUint KEntryAttModified=0x20000000;	

	TUint att;
	r = f.Att(att);
	test_KErrNone(r);
    test.Printf(_L("att %x"), att);
	test_Value(att & KEntryAttModified, (att & KEntryAttModified) == 0);

	r=f.SetAtt(att | KEntryAttModified, 0);
	test_KErrNone(r);

	r = f.Att(att);
	test_KErrNone(r);
    test.Printf(_L("att %x"), att);
	test_Value(att & KEntryAttModified, (att & KEntryAttModified) == 0);

	test.Next(_L("file time is set"));
	r = f.Modified(time);
	test(time.Int64() != 0);

    r = f.Flush(); //-- this will flush attributes to the media
    test_KErrNone(r);
	f.Close();


	// Tidy up
	r=TheFs.SetAtt(fname,0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.Delete(fname);
	test_KErrNone(r);
	}

static void testShortNameAccessorFunctions()
//
// Test RFs::GetShortName(...)
//
	{
	test.Next(_L("Test short name accessor functions"));

    if(!gShortFileNamesSupported)
        {
        test.Printf(_L("Short Names are not supported!. Skipping..."));
        return;
        }


	TBuf<64> sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	RFile f;
	r=TheFs.MkDirAll(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\RANDOM.ENDBIT"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	r=f.Replace(TheFs,_L("LONGFILENAME.LONGEXT"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	r=f.Replace(TheFs,_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\LONGFILENAME.LONGEXT"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	r=f.Replace(TheFs,_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\BAD CHAR"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	r=f.Replace(TheFs,_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\GoodCHAR.TXT"),EFileWrite);
	test_KErrNone(r);
	f.Close();
	TBuf<12> shortName1;
	TBuf<12> shortName2;
	TBuf<12> shortName3;
	TBuf<12> shortName4;
	TBuf<12> shortName5;
	r=TheFs.GetShortName(_L("LONGFILENAME.LONGEXT"),shortName1);
	test_KErrNone(r);
	r=TheFs.GetShortName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\LONGFILENAME.LONGEXT"),shortName2);
	test_KErrNone(r);
	r=TheFs.GetShortName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\BAD CHAR"),shortName3);
	test_KErrNone(r);
	r=TheFs.GetShortName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\GOODCHAR.TXT"),shortName4);
	test_KErrNone(r);
	r=TheFs.GetShortName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY"),shortName5);
	test_KErrNone(r);

	if(Is_Win32(TheFs, gDriveNum))
		{
		test(shortName1==_L("LONGFI~1.LON"));
		test(shortName2==_L("LONGFI~1.LON"));
		test(shortName3==_L("BADCHA~1"));
		test(shortName4.FindF(_L("GOODCHAR.TXT"))>=0);
		test(shortName5==_L("MIDDLE~1"));
		}
	else if(!IsTestingLFFS())
		{
		// LFFS short names not the same as VFAT ones
		test(shortName1==_L("LONGFI~1.LON"));
		test(shortName2==_L("LONGFI~1.LON"));
		test(shortName3==_L("BAD_CHAR"));
		test(shortName4.FindF(_L("GOODCHAR.TXT"))>=0);
		test(shortName5==_L("MIDDLE~1"));
		}

	TFileName longName1;
	TFileName longName2;
	TFileName longName3;
	TFileName longName4;
	TFileName longName5;

	if (Is_Win32(TheFs, gDriveNum))
		{
		r=TheFs.GetLongName(_L("LONGFI~1.LON"),longName1);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\LONGFI~1.LON"),longName2);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\BADCHA~1"),longName3);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\GOODCHAR.TXT"),longName4);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE~1"),longName5);
		test_KErrNone(r);
		}
	else if (!IsTestingLFFS())
		{
		r=TheFs.GetLongName(_L("LONGFI~1.LON"),longName1);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\LONGFI~1.LON"),longName2);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\BAD_CHAR"),longName3);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\GOODCHAR.TXT"),longName4);
		test_KErrNone(r);
		r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE~1"),longName5);
		test_KErrNone(r);
		}
    else
    	{
		// LFFS longname tests
        r=TheFs.GetLongName(shortName1,longName1);
        test_KErrNone(r);
        r=TheFs.SetSessionPath(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-DIRECTORY\\LASTDIR\\"));
        test_KErrNone(r);
        r=TheFs.GetLongName(shortName2,longName2);
        test_KErrNone(r);
        r=TheFs.GetLongName(shortName3,longName3);
        test_KErrNone(r);
        r=TheFs.GetLongName(shortName4,longName4);
        test_KErrNone(r);
        r=TheFs.SetSessionPath(_L("\\F32-TST\\TFILE\\TOPLEVEL\\"));
        test_KErrNone(r);
        r=TheFs.GetLongName(shortName5,longName5);
        test_KErrNone(r);
        r=TheFs.SetSessionPath(sessionPath);
        test_KErrNone(r);
    	}

	test(longName1==_L("LONGFILENAME.LONGEXT"));
	test(longName2==_L("LONGFILENAME.LONGEXT"));
	test(longName3==_L("BAD CHAR"));
	test(longName4.FindF(_L("GOODCHAR.TXT"))>=0);
	test(longName5==_L("MIDDLE-DIRECTORY"));

	r=TheFs.GetShortName(_L("XXX.YYY"),shortName1);
	test_Value(r, r == KErrNotFound);
	r=TheFs.GetShortName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-YROTCERID\\LASTDIR\\BAD-CHAR"),shortName1);
	test_Value(r, r == KErrPathNotFound);
	r=TheFs.GetLongName(_L("XXX.YYY"),longName1);
	test_Value(r, r == KErrNotFound);
	r=TheFs.GetLongName(_L("\\F32-TST\\TFILE\\TOPLEVEL\\MIDDLE-YROTCERID\\LASTDIR\\BAD-CHAR"),longName1);
	test_Value(r, r == KErrPathNotFound);

	r=TheFs.Delete(_L("LONGFILENAME.LONGEXT"));
	test_KErrNone(r);

	TEntry romEntry;
	r=TheFs.Entry(_L("Z:\\System"),romEntry);
	if (r==KErrNotReady)
		{
		test.Printf(_L("ERROR: No rom filesystem present"));
		//test.Getch();
		//return;
		}
	test_KErrNone(r);
	TBuf<64> romFileName=_L("Z:\\");
	romFileName.Append(romEntry.iName);
	r=TheFs.GetShortName(romFileName,shortName1);
	test_Value(r, r == KErrNotSupported);
	r=TheFs.GetLongName(_L("Z:\\system"),longName1);
	test_Value(r, r == KErrNotSupported);
	}

static void RmDir(const TDesC& aDirName)
//
// Remove a directory
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TInt r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported)
		ReportCheckDiskFailure(r);

	TFileName removeDirName = gSessionPath;
	removeDirName.Append(aDirName);

	fMan->Attribs(removeDirName, 0, KEntryAttReadOnly, 0, CFileMan::ERecurse);
	r=fMan->RmDir(removeDirName);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrPathNotFound);

	delete fMan;
	}
//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_TFILE-0659
//! @SYMTestType			CT
//! @SYMREQ					INC112803
//! @SYMTestCaseDesc		Tests that RFs::GetShortName() considers the file extension while generating
//!							shortname from longname and applies ~num if applicable.
//! @SYMTestActions			1. Generates the shortname for the given filename.
//! 						2. Validates the generated shortname against the original filename.
//! @SYMTestExpectedResults	The operation completes with no error. Valid shortname is generated.
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented
//---------------------------------------------
static void TestINC112803()
	{

    if(!gShortFileNamesSupported)
        {
        test.Printf(_L("TestINC112803 : Short names are not supported!\n"));
        return;
        }


	TInt err =0;
	_LIT(KOrigFileName,"\\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\2222.JARX");
	_LIT(KOrigFileShortName,"\\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\2222~1.JAR");
	_LIT(KDestinationFileName,"\\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\2222.JAR");

	// Make sure the file does not already exist
	RmDir(_L("INC112803\\"));

	// Create directories and the file
	MakeDir(_L("\\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\"));
	MakeFile(KOrigFileName,_L8("FILE PATH : \\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\"));

	// Check the generated shortname of the original file
	TBuf<12> shortName;
	err = TheFs.GetShortName(KOrigFileName, shortName);
	test_KErrNone(err);

	// Validate the generated shortname against the original filename.
	if(!IsTestingLFFS())
		{
		// LFFS short names not the same as VFAT ones
		test(shortName==_L("2222~1.JAR"));
		}

	// Validate that the file "2222~1.JAR" can not be created as this is the shortname for "2222.JARX".
	MakeFile(KOrigFileShortName,_L8("FILE PATH : \\F32-TST\\TFILE\\INC112803\\Private2\\101f875a\\"));
	CheckFileExists(KOrigFileShortName, KErrNone, EFalse);

	err = TheFs.Rename(KOrigFileName,KDestinationFileName);
	test_KErrNone(err);

	// Clean up before leaving
	RmDir(_L("INC112803\\"));
	}

static void testIsFileOpen()
//
// Test the IsFileOpen method
//
	{

	test.Next(_L("Test IsFileOpen"));
	TBool answer;
	TInt r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_Value(r, r == KErrNotFound || (r==KErrNone && answer==EFalse));
	RFile f;
	r=f.Replace(TheFs,_L("OPEN.FILE"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_KErrNone(r);
	test(answer!=EFalse);
	f.Close();
	r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_KErrNone(r);
	test(answer==EFalse);
	r=TheFs.Delete(_L("OPEN.FILE"));
	test_KErrNone(r);

	RFile f2;
	r=f2.Replace(TheFs,_L("AnotherOpen.File"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.IsFileOpen(_L("AnotherOpen.File"),answer);
	test_KErrNone(r);
	test(answer!=EFalse);
	r=f.Replace(TheFs,_L("OPEN.FILE"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_KErrNone(r);
	test(answer!=EFalse);
	f2.Close();
	r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_KErrNone(r);
	test(answer!=EFalse);
	f.Close();
	r=TheFs.IsFileOpen(_L("OPEN.FILE"),answer);
	test_KErrNone(r);
	test(answer==EFalse);
	r=TheFs.Delete(_L("AnotherOpen.File"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("OPEN.FILE"));
	test_KErrNone(r);
	}

static void testDeleteOpenFiles()
//
// Test opened files cannot be deleted
//
	{

	test.Next(_L("Test opened files cannot be deleted"));
	RFile f;
	f.Close();
	TInt r=f.Replace(TheFs,_L("Open.File"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.Delete(_L("OPEN.FILE"));
	test_Value(r, r == KErrInUse);
	f.Close();
	f.Close();
	f.Close();
	r=TheFs.Delete(_L("Open.FILe"));
	test_KErrNone(r);

	TFileName fileName;
	r=f.Temp(TheFs,_L(""),fileName,EFileWrite);
	test_KErrNone(r);
	r=TheFs.Delete(fileName);
	test_Value(r, r == KErrInUse);
	f.Close();
	r=TheFs.Delete(fileName);
	test_KErrNone(r);

	MakeFile(_L("\\Documents\\TEstfile.txt"));
	r=f.Open(TheFs,_L("\\Documents\\TEstfile.txt"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\Documents\\TEstfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\documents\\TEstfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\Documents.\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\documents.\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\Documents\\Testfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\documents\\testfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\Documents.\\TEstfile.TXT"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\docUMENTS.\\TESTFILE.TXT"));
	test_Value(r, r == KErrBadName);
	f.Close();
	r=TheFs.Delete(_L("\\Documents\\TEstfile.TXT"));
	test_KErrNone(r);

	MakeFile(_L("\\Documents\\Documents\\TEstfile.txt"));
	r=f.Open(TheFs,_L("\\Documents\\Documents\\TEstfile.txt"),EFileWrite);
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\Documents\\documents.\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\documents\\Documents.\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\Documents.\\documents\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\documents.\\Documents\\TEstfile.txt"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\Documents\\Documents\\Testfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\documents\\documents\\testfile.txt"));
	test_Value(r, r == KErrInUse);
	r=TheFs.Delete(_L("\\Documents.\\Documents.\\TEstfile.TXT"));
	test_Value(r, r == KErrBadName);
	r=TheFs.Delete(_L("\\docUMENTS.\\docUMENTS.\\TESTFILE.TXT"));
	test_Value(r, r == KErrBadName);


	r=TheFs.RmDir(_L("\\Documents\\"));
	test_Value(r, r == KErrInUse);
	r=TheFs.RmDir(_L("\\documents\\"));
	test_Value(r, r == KErrInUse);
	r=TheFs.RmDir(_L("\\Documents.\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\documents.\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\Documents\\documents\\"));
	test_Value(r, r == KErrInUse);
	r=TheFs.RmDir(_L("\\documents\\documents.\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\Documents.\\Documents\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\documents.\\Documents.\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\Documents\\TestFile.TXT"));
	test_Value(r, r == KErrInUse);
	r=TheFs.RmDir(_L("\\documents\\TestFile"));
	test_Value(r, r == KErrInUse);
	r=TheFs.RmDir(_L("\\Documents.\\Testfile."));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\documents.\\t"));
	test_Value(r, r == KErrBadName);
	f.Close();
	r=TheFs.Delete(_L("\\Documents\\documents\\TEstfile.TXT"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\Documents\\documents.\\"));
	test_Value(r, r == KErrBadName);
	r=TheFs.RmDir(_L("\\Documents.\\"));
	test_Value(r, r == KErrBadName);
	}

static void testFileSeek()
//
// Test seeking
//
	{
	test.Next(_L("Test file seek"));
	RFile f;
	TInt r=f.Open(TheFs,_L("T_File.cpp"),EFileWrite);
	test_KErrNone(r);

	TBuf8<20> text1;TInt pos1=0;
	TBuf8<20> text2;TInt pos2=510;
	TBuf8<20> text3;TInt pos3=900;
	TBuf8<20> text4;TInt pos4=2010;
	TBuf8<20> text5;TInt pos5=4999;

	r=f.Read(pos1,text1);
	test_KErrNone(r);
	r=f.Read(pos2,text2);
	test_KErrNone(r);
	r=f.Read(pos3,text3);
	test_KErrNone(r);
	r=f.Read(pos4,text4);
	test_KErrNone(r);
	r=f.Read(pos5,text5);
	test_KErrNone(r);

	TBuf8<20> testBuf;

	r=f.Read(pos3,testBuf);
	test_KErrNone(r);
	test(testBuf==text3);

	r=f.Read(pos1,testBuf);
	test_KErrNone(r);
	test(testBuf==text1);

	r=f.Read(pos4,testBuf);
	test_KErrNone(r);
	test(testBuf==text4);

	r=f.Read(pos2,testBuf);
	test_KErrNone(r);
	test(testBuf==text2);

	r=f.Read(pos5,testBuf);
	test_KErrNone(r);
	test(testBuf==text5);

	r=f.Read(pos2,testBuf);
	test_KErrNone(r);
	test(testBuf==text2);
	r=f.SetSize(1023);
	test_KErrNone(r);
	r=f.Read(pos2,testBuf);
	test_KErrNone(r);
	test(testBuf==text2);
	r=f.SetSize(1024);
	test_KErrNone(r);
	r=f.Read(pos1,testBuf);
	test_KErrNone(r);
	test(testBuf==text1);
	r=f.Read(pos2,testBuf);
	test_KErrNone(r);
	test(testBuf==text2);

	r=f.Read(pos1,testBuf);
	test_KErrNone(r);
	test(testBuf==text1);
	r=f.SetSize(511);
	test_KErrNone(r);
	r=f.Read(pos1,testBuf);
	test_KErrNone(r);
	test(testBuf==text1);
	r=f.SetSize(512);
	test_KErrNone(r);
	r=f.Read(pos1,testBuf);
	test_KErrNone(r);
	test(testBuf==text1);
	f.Close();
	}

static void testMoreFileSeek()
//
//	Further test of RFile::Seek()
//
	{
//	Create a zero length file
	RFile file;
	TInt r=file.Replace(TheFs,_L("\\F32-TST\\TFILE\\seektest"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.SetSize(20);
	test_KErrNone(r);
//	Seek beyond the length of the file
	TInt seekPos;
	seekPos = 80;								//	Pick a likely offset
    TInt err = file.Seek(ESeekEnd, seekPos);	//	and go there
    test_KErrNone(err);
	test(seekPos==20);							//	Somewhat non-intuitive?

	r=file.Write(_L8("A Devil's Haircut"));
	test_KErrNone(r);
	TInt newFileSize;
	r=file.Size(newFileSize);
	test_KErrNone(r);

	seekPos = 0;
    err = file.Seek(ESeekCurrent, seekPos);		//	Find out where we ended up?
	test_KErrNone(err);
	test(seekPos==37);

	file.SetSize(512);
	seekPos=513;
	err=file.Seek(ESeekStart, seekPos);
	test_KErrNone(err);
	test(seekPos==513);

	err=file.Seek(ESeekEnd, seekPos);
	test_KErrNone(err);
	test(seekPos==512);

	seekPos=-530;
	err=file.Seek(ESeekEnd, seekPos);
	test_KErrNone(err);
	test(seekPos==0);

	seekPos=-10;
	err=file.Seek(ESeekEnd, seekPos);
	test_KErrNone(err);
	test(seekPos==502);

	seekPos=-10;
	err=file.Seek(ESeekStart,seekPos);
	test_Value(err, err == KErrArgument);
	test(seekPos==-10);

	seekPos=0;
	err=file.Seek(ESeekEnd,seekPos);
	test_KErrNone(err);
	test(seekPos==512);

	file.Close();
	r=TheFs.Delete(_L("\\F32-TST\\TFILE\\seektest"));
	test_KErrNone(r);
	}

static void testSetSize()
//
// Test setsize
//
	{

	test.Next(_L("Test SetSize"));
	RFile f1;
	TInt i=0;
	TInt r=f1.Replace(TheFs,_L("File.File"),EFileWrite);
	test_KErrNone(r);
	gBuf.SetLength(32);
	for(i=0;i<32;i++)
		gBuf[i]=(TUint8)i;
	r=f1.Write(gBuf);
	test_KErrNone(r);
	gBuf.SetLength(1334);
	for(i=64;i<1334+64;i++)
		gBuf[i-64]=(TUint8)i;
	r=f1.Write(30,gBuf);
	r=f1.Read(30,gBuf,1000);
	test_KErrNone(r);
	test(gBuf[0]==64);
	test(gBuf[1]==65);
	test(gBuf[2]==66);
	f1.Close();

	test.Next(_L("Open a large file"));
	r=f1.Replace(TheFs,_L("File.File"),EFileWrite);
	test_KErrNone(r);
	CheckDisk();
	r=f1.SetSize(131072); // 128K
	test_KErrNone(r);
	TBuf8<16> testData=_L8("testData");
	r=f1.Write(131060,testData);
	test_KErrNone(r);
	f1.Close();
	r=f1.Open(TheFs,_L("File.File"),EFileRead);
	test_KErrNone(r);
	TInt size;
	r=f1.Size(size);
	test_KErrNone(r);
	test(size==131072);
	TBuf8<16> testData2;
	r=f1.Read(131060,testData2,8);
	test_KErrNone(r);
	test(testData==testData2);
	f1.Close();
	TheFs.Delete(_L("File.file"));
	CheckDisk();
	}

static void testIsRomAddress()
	{
	RFile f;
	TInt r=f.Open(TheFs, PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("Z:\\Sys\\Bin\\eshell.exe"):_L("Z:\\System\\Bin\\eshell.exe"), EFileRead);
	test_KErrNone(r);
	TInt anAddress=0;
	r=f.Seek(ESeekAddress, anAddress);
	test_KErrNone(r);
#if !defined(__WINS__)
	test(RFs::IsRomAddress((TAny *)anAddress)); // Always returns EFalse if WINS
#endif
	test(RFs::IsRomAddress(NULL)==FALSE);
	f.Close();
	}

#include "../../../userlibandfileserver/fileserver/inc/message.h"
#include <f32plugin.h>

static void testMiscellaneousReportedBugs()
//
// Test bug reports, real or imaginary
//
	{

	test.Next(_L("Miscellaneous tests"));
	RFile f1;
	TInt temp;
	TInt r=f1.Replace(TheFs,_L("File.File"),EFileWrite);
	test_KErrNone(r);
	r=f1.Size(temp);
	test_KErrNone(r);
	test(temp==0);
	TUint data=0;
	TPtrC8 buf((TText8*)&data,1);
	r=f1.Write(buf);
//	r=f1.Write(_L("\0"));
	test_KErrNone(r);
	r=f1.Size(temp);
	test_KErrNone(r);
	test(temp==1);
	temp=0;
	r=f1.Seek(ESeekStart,temp);
	test_KErrNone(r);
	test(temp==0);
	TBuf8<32> testBuf;
	r=f1.Read(testBuf);
	test_KErrNone(r);
	test(testBuf==buf);
	f1.Close();

	class RHackFile : public RFile
	{
	public:
		inline TInt SendReceive(TInt aFunction, const TIpcArgs& aArgs) const
			{ return RSubSessionBase::SendReceive(aFunction, aArgs); }
	};

	RHackFile f2;
	f2.Open(TheFs, _L("File.File"), EFileRead);
	test_KErrNone(r);
	r = f2.SendReceive(/*47*/ EFsFileChangeMode, TIpcArgs(EFileRead | EFileWrite));	// <- must fail!
	test_Value(r, r == KErrArgument);
	r = f2.Write(_L8("Hacked!"));	// <- must fail!
	test_Value(r, r == KErrAccessDenied);
	f2.Close();

	r=TheFs.Delete(_L("File.FIle"));
	test_KErrNone(r);
	}

static void testFileRename()
//
// Test rename
//
	{

	test.Next(_L("Test rename"));
	TBuf<64> name1=_L("asdfasdfasdfasfd.qwer");
	TBuf<64> name2=_L("File.xyz");
	TBuf<64> name3=_L("ASdfasdFasdfasfd.qwer");
	TBuf8<64> contents;

    TInt r;
    RFile f1;

    //-- test renaming a file to a non-existing directory
    r = TheFs.MkDir(_L("\\temp\\"));
    test_Value(r, r == KErrNone || r==KErrAlreadyExists);

    r = f1.Replace(TheFs, _L("\\temp\\file1"), 0);
    test_KErrNone(r);

    r = f1.Rename(_L("\\temp\\temp\\file1"));
    test_Value(r, r == KErrPathNotFound);

    f1.Close();


	r=f1.Replace(TheFs,name2,EFileWrite);
	test_KErrNone(r);
	r=f1.Write(_L8("1234"));
	test_KErrNone(r);
	TInt len=CheckFileExists(name2,KErrNone);
	test(len==4);
	r=f1.Rename(name1);
	test_KErrNone(r);

	r=f1.Read(0,contents);
	test_KErrNone(r);
	test(contents==_L8("1234"));
	r=f1.Write(4,_L8("5678"));
	test_KErrNone(r);

	len=CheckFileExists(name1,KErrNone);
	test(len==8);
	CheckFileExists(name2,KErrNotFound);
	r=f1.Write(8,_L8("90"));
	test_KErrNone(r);
	f1.Close();
	len=CheckFileExists(name1,KErrNone);
	test(len==10);

	test.Next(_L("Test can change case using rename"));
	r=f1.Open(TheFs,name1,EFileRead|EFileWrite);
	test_KErrNone(r);
	r=f1.Rename(name3);
	test_KErrNone(r);
	CheckFileExists(name1,KErrNone,EFalse);
	len=CheckFileExists(name3,KErrNone);
	test(len==10);
	f1.Close();
	CheckFileExists(name1,KErrNone,EFalse);
	len=CheckFileExists(name3,KErrNone);
	test(len==10);

	test.Next(_L("Test can rename to an identical filename"));
	r=f1.Open(TheFs,name3,EFileRead|EFileWrite);
	test_KErrNone(r);
	r=f1.Rename(name3);
	test_KErrNone(r);
	len=CheckFileExists(name3,KErrNone);
	test(len==10);
	f1.Close();
	len=CheckFileExists(name3,KErrNone);
	test(len==10);

	test.Next(_L("Test rename to a name containing a wildcard is rejected"));
	r=f1.Open(TheFs,name3,EFileRead|EFileWrite);
	test_KErrNone(r);
	r=f1.Rename(_L("asdf*ASDF"));
	test_Value(r, r == KErrBadName);
	r=f1.Rename(_L("asdf?AF"));
	test_Value(r, r == KErrBadName);
	f1.Close();

	r=f1.Open(TheFs,name3,EFileRead);
	test_KErrNone(r);
	r=f1.Read(contents);
	test_KErrNone(r);
	test(contents==_L8("1234567890"));
	r=f1.Read(contents);
	test_KErrNone(r);
	test(contents.Length()==0);
	f1.Close();

	test.Next(_L("Check file date is retained"));
	TDateTime dateTime(1995,(TMonth)10,19,23,0,0,0);
	TTime oldTime(dateTime);
	r=TheFs.SetEntry(name3,oldTime,0,0);
	test_KErrNone(r);
	r=f1.Open(TheFs,name3,EFileRead|EFileWrite);
	test_KErrNone(r);
	TTime check;
	r=f1.Modified(check);
	test_KErrNone(r);
	test(check==oldTime);

	r=f1.Rename(_L("OldFile.Old"));
	test_KErrNone(r);

	r=f1.Modified(check);
	test_KErrNone(r);
	test(check==oldTime);
	r=TheFs.Modified(_L("oldfile.old"),check);
	test_KErrNone(r);
	test(check==oldTime);
	f1.Close();
	r=TheFs.Modified(_L("oldfile.old"),check);
	test_KErrNone(r);
	test(check==oldTime);
	}

static void TestFileUids()
//
// Test uids in files
//
	{

	test.Next(_L("Uids in files"));
	TUidType uidData(TUid::Uid(1),TUid::Uid(1),TUid::Uid(1));
	MakeFile(_L("Tmp04005.$$$"),uidData,_L8("Some other data"));
	TUidType uidData1(TUid::Uid(2),TUid::Uid(2),TUid::Uid(2));
	MakeFile(_L("Sketch(01)"),uidData1,_L8("A different sketch"));

	TEntry e;
	TInt r=TheFs.Entry(_L("Tmp04005.$$$"),e);
	test_KErrNone(r);
	test(uidData==e.iType);
	r=TheFs.Entry(_L("Sketch(01)"),e);
	test_KErrNone(r);
	test(uidData1==e.iType);

	test.Next(_L("Test replace preserves UIDs"));
	r=TheFs.Replace(_L("Tmp04005.$$$"),_L("Sketch(01)"));
	test_KErrNone(r);

	r=TheFs.Entry(_L("Tmp04005.$$$"),e);
	test_Value(r, r == KErrNotFound);
	r=TheFs.Entry(_L("Sketch(01)"),e);
	test_KErrNone(r);
	test(uidData==e.iType);
	}


static void TestMaxLengthFilenames()
//
// Test max length filenames can be created/deleted
//
	{
	if(Is_SimulatedSystemDrive(TheFs, gDriveNum))
		{
		test.Printf(_L("Skipping TestMaxLengthFilenames() on PlatSim/Emulator drive %C:\n"), gSessionPath[0]);
		return;
		}

	test.Next(_L("Test max length filenames"));
	TFileName bigName;
	CreateLongName(bigName,gSeed,255);
	bigName[0]='\\';
	RFile f;
	TInt r=f.Create(TheFs,bigName,EFileRead);
	test_Value(r, r == KErrBadName);
	bigName.SetLength(254);
	r=f.Create(TheFs,bigName,EFileRead);
	test_KErrNone(r);
	f.Close();

	TInt count;
	TFileName countedBigName=bigName;
    // This loop may not reach the '\' character, or we will get a bad path.
    for (count=0;count<('Z'-'A');count++)
		{
		countedBigName[2]=(TText)('A'+count);
		r=f.Create(TheFs,countedBigName,EFileRead);
		if (r==KErrDirFull)
			{
			r=TheFs.Delete(countedBigName);
			test_Value(r, r == KErrNotFound);
			break;
			}
		if (r!=KErrNone)
			test.Printf(_L("File create failed:%d"),r);
		test_KErrNone(r);
		f.Close();
		}
	while(count--)
		{
		countedBigName[2]=(TText)('A'+count);
		r=TheFs.Delete(countedBigName);
		test_KErrNone(r);
		}

	r=TheFs.Delete(bigName);
	test_KErrNone(r);

	TFileName subDirFileName=_L("\\F32-TST\\TFILE");
	bigName.SetLength(241);
	subDirFileName.Append(bigName);
	r=f.Create(TheFs,subDirFileName,EFileRead);
	test_Value(r, r == KErrBadName);
	subDirFileName.SetLength(254);
	r=f.Create(TheFs,subDirFileName,EFileRead);
	test_KErrNone(r);
	f.Close();
	r=TheFs.Delete(subDirFileName);
	test_KErrNone(r);
	}




static void testReadersWriters()
//
// Test EFileShareReadersOrWriters file sharing.
//
	{

	test.Start(_L("Test EFileShareReadersOrWriters sharing"));
	MakeFile(_L("TESTER"));

	// Open a file in EFileShareReadersOnly mode
	RFile f1;
	TInt r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);

	// Opening a share in EFileShareReadersOnly mode should succeed
	RFile f2;
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead|EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead|EFileWrite);
	test_Value(r, r == KErrInUse);

	// Opening a share in EShareAny mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_Value(r, r == KErrInUse);

	f1.Close();

	//////////////////////

	// Open a file in EFileShareReadersOrWriters mode for reading
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);

	// Opening a share in EFileShareExclusive mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOnly mode should succeed
	// (the share doesn't care if the file is opened for reading or writing)
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOnly mode with EFileRead accesss should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOnly mode with EFileWrite accesss should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOnly mode with EFileRead|EFileWrite accesss should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareAny mode should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r);
	f2.Close();

	f1.Close();

	//////////////////////

	// Open a file in EFileShareReadersOrWriters mode for writing
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
	test_KErrNone(r);

	// Opening a share in EFileShareExclusive mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOnly mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead|EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareAny mode should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r);
	f2.Close();

	f1.Close();

	//////////////////////////

	// Open a file in EFileShareAny mode
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r);

	// Opening a share in EFileShareExclusive mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareExclusive);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOnly mode should fail
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_Value(r, r == KErrInUse);

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareReadersOrWriters mode with EFileRead|EFileWrite access should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead|EFileWrite);
	test_KErrNone(r);
	f2.Close();

	// Opening a share in EFileShareAny mode with should succeed
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
	test_KErrNone(r);
	f2.Close();

	f1.Close();

	//////////////////////

	// Open a file in EFileShareReadersOrWriters mode for reading
	r=f1.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);

	// Opening a share in EFileShareReadersOnly mode should succeed
	//  - The share should now be promoted to EFileShareReadersOnly mode
	r=f2.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);

	TInt pass = 2;
	while(pass--)
		{
		RFile f3;
		// Opening a share in EFileShareReadersOnly mode with EFileRead accesss should succeed
		r=f3.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead);
		test_KErrNone(r);
		f3.Close();

		// Opening a share in EFileShareReadersOnly mode with EFileWrite accesss should fail
		r=f3.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileWrite);
		if(pass == 1)
			{
			// The share is promoted - should obey EFileShareReadersOnly rules
			test_Value(r, r == KErrInUse);
			}
		else
			{
			// The share is demoted - should obey EFileShareReadersOrWriters rules
			test_KErrNone(r);
			f3.Close();
			}

		// Opening a share in EFileShareReadersOnly mode with EFileRead|EFileWrite accesss should fail
		r=f3.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareReadersOrWriters|EFileRead|EFileWrite);
		if(pass == 1)
			{
			// The share is promoted - should obey EFileShareReadersOnly rules
			test_Value(r, r == KErrInUse);
			}
		else
			{
			// The share is demoted - should obey EFileShareReadersOrWriters rules
			test_KErrNone(r);
			f3.Close();
			}

		// Opening a share in EFileShareAny mode should fails
		r=f3.Open(TheFs,_L("T_FILE.CPP"),EFileStreamText|EFileShareAny);
		if(pass == 1)
			{
			// The share is promoted - should obey EFileShareReadersOnly rules
			test_Value(r, r == KErrInUse);
			f2.Close();
			}
		else
			{
			// The share is demoted - should obey EFileShareReadersOrWriters rules
			test_KErrNone(r);
			f3.Close();
			}
		}

	f1.Close();

	test.End();
	}


static void testINC070455()
//
// INC070455 - RFile.ChangeMode() panics
//
	{
	_LIT(KTestName, "Test INC070455 - RFile.ChangeMode() panics");
	test.Start(KTestName);


	// To reproduce this defect, we need a filename of less than 10 characters.
	// We cannot use the directories used by the rest of this test harness.
	_LIT(KShortName, "C:\\file.txt");


	test.Next(_L("Create file..."));
	RFile TheFile;
	TInt r = TheFile.Create(TheFs, KShortName, EFileWrite);
	test((KErrNone == r) || (KErrAlreadyExists == r));
	TheFile.Close();


	test.Next(_L("Re-open the file..."));
	r = TheFile.Open(TheFs, KShortName, EFileRead | EFileShareExclusive);
	test (KErrNone == r);


	test.Next(_L("Change the file\'s mode..."));
	r = TheFile.ChangeMode(EFileShareReadersOnly);
	test (KErrNone == r);
	TheFile.Close();


	test.Next(_L("Tidy up"));
	r = TheFs.Delete(KShortName);
	test (KErrNone == r);

	test.End();
	}


LOCAL_D TBuf8<0x80000> gLongBuf;
_LIT(KFileName, "\\zerolengthsourcedescriptortest.txt");

static void zeroSrcDesc()
	{
	gLongBuf.Zero();
	}

static void setSrcDescLen()
	{
	gLongBuf.SetLength(0x80000);
	}

static void createTestFile(RFile& aFile)
	{
	TInt r = aFile.Create(TheFs, KFileName, EFileWrite);
	test((KErrNone == r) || (KErrAlreadyExists == r));
	}

static void removeTestFile(RFile& aFile)
	{
	aFile.Close();
	TInt r = TheFs.Delete(KFileName);
	test (KErrNone == r);
	}

#ifdef _DEBUG
static TInt testWritePanic(TAny* aPtr)
	{
	RFile * ptr = (RFile *)aPtr;
	TInt r=ptr->Write(gLongBuf,0x80000);
	test (KErrNone == r);
	return KErrNone;
	}
#endif
static void testNegativeLengthToWrite()
//
// DEF091545 - Tests added to check the write function behaviour with Negative length
{

test.Start(_L("Test RFile::Write variants with Negative Length parameter"));

	LOCAL_D TBuf8<0x100> gBuf;
	RFile TheFile;
	TInt r;
	TRequestStatus status1(KRequestPending);
	TRequestStatus status2(KRequestPending);

// EXPORT_C TInt RFile::Write(const TDesC8& aDes,TInt aLength)
	createTestFile(TheFile);

	r=TheFile.Write(gBuf, -1);
	test_Value(r, r == KErrArgument);

	removeTestFile(TheFile);

// EXPORT_C void RFile::Write(const TDesC8& aDes,TInt aLength, TRequestStatus& aStatus)
	createTestFile(TheFile);
	TheFile.Write(gBuf,-1, status1);
	User::WaitForRequest(status1);
	test ( status1.Int() == KErrArgument);

	removeTestFile(TheFile);



// EXPORT_C TInt RFile::Write(TInt aPos,const TDesC8& aDes,TInt aLength)
	createTestFile(TheFile);
	r = TheFile.Write(0,gBuf,-1);
	test_Value(r, r == KErrArgument);
	removeTestFile(TheFile);


// EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes, TInt aLength, TRequestStatus& aStatus)
	createTestFile(TheFile);
	TheFile.Write(0, gBuf,-1, status2);
	User::WaitForRequest(status2);
	test ( status2.Int() == KErrArgument);
	removeTestFile(TheFile);

	test.End();



}

static TInt testLockPanic(TAny* aPtr)
	{
	TInt aPos=128;
	TInt aLen=-1;
	RFile * ptr = (RFile *)aPtr;
	TInt r=ptr->Lock(aPos, aLen);
	test (KErrNone == r);
	return KErrNone;
	}

static TInt testUnLockPanic(TAny* aPtr)
	{
	TInt aPos=128;
	TInt aLen=-1;
	RFile * ptr = (RFile *)aPtr;
	TInt r=ptr->UnLock(aPos, aLen);
	test (KErrNone == r);
	return KErrNone;
	}

static TInt testSetSizePanic(TAny* aPtr)
	{
	TInt aSize=-1;
	RFile * ptr = (RFile *)aPtr;
	TInt r=ptr->SetSize(aSize);
	test (KErrNone == r);
	return KErrNone;
	}

static void testNegativeLength()
	{
	test.Start(_L("Test RFile::Lock, RFile::Unlock and RFile::SetSize with Negative Length parameter"));

	test(TheFs.ShareProtected() == KErrNone);

	RFile TheFile;
	createTestFile(TheFile);
	TRequestStatus status = KRequestPending;

	// launch call on separate thread as expected to panic
	// Test Lock with a negative length
	User::SetJustInTime(EFalse);
	RThread t;
	test(t.Create(_L("testLockPanic"), testLockPanic, KDefaultStackSize, 0x2000, 0x2000, &TheFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	User::SetJustInTime(ETrue);
	test(t.ExitType() == EExitPanic);
	test(t.ExitReason() == 17);
	t.Close();


	// Test Unlock with a negative length
	User::SetJustInTime(EFalse);
	status = KRequestPending;
	test(t.Create(_L("testUnLockPanic"), testUnLockPanic, KDefaultStackSize, 0x2000, 0x2000, &TheFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(t.ExitType() == EExitPanic);
	test(t.ExitReason() == 18);
	t.Close();
	User::SetJustInTime(ETrue);

	// Test SetSize with a negative length
	User::SetJustInTime(EFalse);
	status = KRequestPending;
	test(t.Create(_L("testSetSizePanic"), testSetSizePanic, KDefaultStackSize, 0x2000, 0x2000, &TheFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(t.ExitType() == EExitPanic);
	test(t.ExitReason() == 20);
	t.Close();
	User::SetJustInTime(ETrue);

	removeTestFile(TheFile);
	test.End();
	}

static void testZeroLengthDescriptors()
//
// INC088416 - NAND thread crash when doing async writes to internal memory
//
// Test each variant of RFile::Write against zero length source descriptor arguements
	{
	test.Start(_L("Test RFile::Write variants with Zero Length Source Descriptor"));

	RFile TheFile;
	TRequestStatus status(KRequestPending);


//  EXPORT_C TInt RFile::Write(const TDesC8& aDes) PASS ZERO length descriptor

	createTestFile(TheFile);

	zeroSrcDesc();

	test.Next(_L("Execute sync call RFile::Write(const TDesC8& aDes) with zero length aDes"));
	TInt r=TheFile.Write(gLongBuf);
	test_KErrNone(r);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(const TDesC8& aDes,TRequestStatus& aStatus) PASS ZERO length descriptor

	createTestFile(TheFile);

	status = KRequestPending;
	zeroSrcDesc();

	test.Next(_L("Start async call RFile::Write(const TDesC8& aDes,TRequestStatus& aStatus) with zero length sDes"));
	TheFile.Write(gLongBuf,status);
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(const TDesC8& aDes,TRequestStatus& aStatus) SCROBBLE descriptor to ZERO length during async write

	createTestFile(TheFile);

	status = KRequestPending;
	setSrcDescLen();

	test.Next(_L("Start async call RFile::Write(const TDesC8& aDes,TRequestStatus& aStatus)"));
	TheFile.Write(gLongBuf,status);
	test.Printf(_L("Zero source descriptor during async write\n"));
	zeroSrcDesc();
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C TInt RFile::Write(const TDesC8& aDes,TInt aLength) PASS ZERO length descriptor

	createTestFile(TheFile);

	status = KRequestPending;
	zeroSrcDesc();

	test.Next(_L("Execute sync call RFile::Write(const TDesC8& aDes,TInt aLength) with zero length aDes"));
#ifdef _DEBUG
	// launch call on separate thread as expected to panic
	User::SetJustInTime(EFalse);
	status = KRequestPending;
	RThread t;
	test(t.Create(_L("testWritePanic"), testWritePanic, KDefaultStackSize, 0x2000, 0x2000, &TheFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(t.ExitType() == EExitPanic);
	test(t.ExitReason() == 27);
	t.Close();
	User::SetJustInTime(ETrue);
#else

	r=TheFile.Write(gLongBuf, 0x80000);
	test_KErrNone(r);
#endif

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(const TDesC8& aDes,TInt aLength, TRequestStatus& aStatus) PASS ZERO length descriptor

	createTestFile(TheFile);

	status = KRequestPending;
	zeroSrcDesc();

	test.Next(_L("Start async call RFile::Write(const TDesC8& aDes, TInt aLength,TRequestStatus& aStatus) with zero length sDes"));
	TheFile.Write(gLongBuf, 0x80000, status); ;
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(const TDesC8& aDes,TInt aLength, TRequestStatus& aStatus) SCROBBLE descriptor to ZERO length during async write

	createTestFile(TheFile);

	status = KRequestPending;
	setSrcDescLen();

	test.Next(_L("Start async call RFile::Write(const TDesC8& aDes, Int aLength,TRequestStatus& aStatus)"));
	TheFile.Write(gLongBuf,0x80000, status);
	test.Printf(_L("Zero source descriptor during async write\n"));
	zeroSrcDesc();
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C TInt RFile::Write(TInt aPos,const TDesC8& aDes) PASS ZERO length descriptor

	createTestFile(TheFile);

	zeroSrcDesc();

	test.Next(_L("Execute sync call RFile::Write(TInt aPos, const TDesC8& aDes) with zero length aDes"));
	r=TheFile.Write(0, gLongBuf);
	test_KErrNone(r);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus) PASS ZERO length descriptor

	createTestFile(TheFile);

	status = KRequestPending;
	zeroSrcDesc();

	test.Next(_L("Start async call RFile::Write(TInt aPos, const TDesC8& aDes, TRequestStatus& aStatus) with zero length sDes"));
	TheFile.Write(0, gLongBuf, status);
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus) SCROBBLE descriptor to ZERO length during async write

	createTestFile(TheFile);

	status = KRequestPending;
	setSrcDescLen();

	test.Next(_L("Start async call RFile::Write(TInt aPos, const TDesC8& aDes, TRequestStatus& aStatus)"));
	TheFile.Write(0, gLongBuf, status);
	test.Printf(_L("Zero source descriptor during async write\n"));
	zeroSrcDesc();
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C TInt RFile::Write(TInt aPos,const TDesC8& aDes,TInt aLength) PASS ZERO length descriptor

	createTestFile(TheFile);

	zeroSrcDesc();

	test.Next(_L("Execute sync call RFile::Write(TInt aPos, const TDesC8& aDes, TInt aLength) with zero length aDes"));
	r=TheFile.Write(0, gLongBuf, 0x80000);
	test_KErrNone(r);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes, TInt aLength, TRequestStatus& aStatus) PASS ZERO length descriptor

	createTestFile(TheFile);

	status = KRequestPending;
	zeroSrcDesc();

	test.Next(_L("Start async call RFile::Write(TInt aPos, const TDesC8& aDes, TInt aLength, TRequestStatus& aStatus) with zero length sDes"));
	TheFile.Write(0, gLongBuf, 0x80000, status);
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	// EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes, TInt aLength, TRequestStatus& aStatus) SCROBBLE descriptor to ZERO length during async write

	createTestFile(TheFile);

	status = KRequestPending;
	setSrcDescLen();

	test.Next(_L("Start async call RFile::Write(TInt aPos, const TDesC8& aDes, TInt aLength, TRequestStatus& aStatus"));
	TheFile.Write(0, gLongBuf, 0x80000, status);
	test.Printf(_L("Zero source descriptor during async write\n"));
	zeroSrcDesc();
	User::WaitForRequest(status);
	test(status.Int()==KErrNone);

	test.Printf(_L("Test case passed\n"));

	removeTestFile(TheFile);


	test.End();
	}

static void testReadBufferOverflow()
//
// Test each variant of RFile::Read when the specified extent to read is
// greater than the available buffer space
//
	{
	test.Start(_L("Test RFile::Read for oversized requests"));

	RFile file;
	TInt r = file.Create(TheFs, KFileName, EFileRead);
	test((KErrNone == r) || (KErrAlreadyExists == r));

	TInt err = KErrNone;
	TRequestStatus status(KRequestPending);
	TBuf8<2> buf8;

// EXPORT_C TInt RFile::Read(TDes8& aDes,TInt aLength) const
	err = file.Read(buf8,5);
	test_Value(err, err == KErrOverflow);
	err = KErrNone;

// EXPORT_C void RFile::Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
	file.Read(buf8,5,status);
	test(status.Int()==KErrOverflow);
	status = KRequestPending;

// EXPORT_C TInt RFile::Read(TInt aPos,TDes8& aDes,TInt aLength) const
	err = file.Read(0,buf8,5);
	test_Value(err, err == KErrOverflow);

// EXPORT_C void RFile::Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
	file.Read(0,buf8,5,status);
	test(status.Int()==KErrOverflow);

	removeTestFile(file);
	test.End();
	}

RSemaphore gSleepThread;
TFileName gLastTempFileName;
enum TTestDoCMode
	{
	EDoCPanic=1,
	EDoCDeleteOnClose=2
	};

static TInt DeleteOnCloseClientThread(TAny* aMode)
	{
	TTestDoCMode testMode = *(TTestDoCMode*)&aMode;
	TUint fileMode=EFileRead;
	RFs fs;
	RFile file;

	TInt r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	if (testMode & EDoCDeleteOnClose)
		fileMode|=EDeleteOnClose;
	r=file.Temp(fs,_L(""),gLastTempFileName,fileMode);
	test_KErrNone(r);
	// Signal controlling thread and pause for panic where requested
	// by caller.
	if (testMode & EDoCPanic)
		{
		gSleepThread.Signal();
		User::After(10000000);
		}
	file.Close();
	if (!(testMode & EDoCPanic))
		gSleepThread.Signal();
	fs.Close();
	return KErrNone;
	}

static	void TestDeleteOnClose()
//
// Test RFile::Temp delete on close behaviour
//
	{
	test.Start(_L("RFile::Temp default file close behaviour"));

	gSleepThread.CreateLocal(0);
	RThread clientThread;
	RFile file;
	RFile file2;
	TFileName filename =_L("DoCFile.tst");
	TInt r;

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0804
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying the original behaviour of RFile::Temp()
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test thread creates a file with DeleteOnClose flag unset and
//!		exits normally.
//!		The main test body attempts to delete the resulting temporary
//!		file.
//!
//! @SYMTestExpectedResults
//! 	1.	The temporary file is successfully created and deleted.
//---------------------------------------------------------------------------------------------------------------------
	r=clientThread.Create(_L("DeleteOnCloseClientThread 1"),DeleteOnCloseClientThread,KDefaultStackSize,0x2000,0x2000,(TAny*)0);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	r=TheFs.Delete(gLastTempFileName);
	test_KErrNone(r);
	clientThread.Close();

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0805
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying the Delete on Close behaviour of RFile::Temp()
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test thread creates a file with DeleteOnClose flag set and
//!		exits normally.
//!		The main test body attempts to delete the resulting temporary
//!		file.
//!
//! @SYMTestExpectedResults
//! 	1.	The temporary file is successfully created and automatically
//!		deleted upon close.   The subsequent attempted file deletion
//!		by the main test body should fail with KErrNotFound.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Temp EDeleteOnClose behaviour"));
	r=clientThread.Create(_L("DeleteOnCloseClientThread 2"),DeleteOnCloseClientThread,KDefaultStackSize,0x2000,0x2000,(TAny*)EDoCDeleteOnClose);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	r=TheFs.Delete(gLastTempFileName);
	test_Value(r, r == KErrNotFound);
	clientThread.Close();

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0806
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying the original, panic behaviour of RFile::Temp()
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test thread creates a file with DeleteOnClose flag unset and
//!		is paniced by the main test body.
//!		The main test body attempts to delete the resulting temporary
//!		file.
//!
//! @SYMTestExpectedResults
//! 	1.	The temporary file is successfully created and deleted.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Temp default panic behaviour"));
	r=clientThread.Create(_L("DeleteOnCloseClientThread 3"),DeleteOnCloseClientThread,KDefaultStackSize,0x2000,0x2000,(TAny*)EDoCPanic);
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	User::SetJustInTime(EFalse);
	clientThread.Panic(_L("Panic temp file thread #3"),KErrGeneral);
	User::SetJustInTime(ETrue);
	CLOSE_AND_WAIT(clientThread);
	FsBarrier();
	r=TheFs.Delete(gLastTempFileName);
	test_KErrNone(r);

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0807
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying the Delete on Close, panic behaviour of RFile::Temp()
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test thread creates a file with DeleteOnClose flag set and
//!		is paniced by the main test body.
//!		The main test body attempts to delete the resulting temporary
//!		file.
//!
//! @SYMTestExpectedResults
//! 	1.	The temporary file is successfully created and automatically
//!		deleted upon close.   The subsequent attempted file deletion
//!		by the main test body should fail with KErrNotFound.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Temp EDeleteOnClose panic behaviour"));
	r=clientThread.Create(_L("DeleteOnCloseClientThread 4"),DeleteOnCloseClientThread,KDefaultStackSize,0x2000,0x2000,(TAny*)(EDoCPanic|EDoCDeleteOnClose));
	test_KErrNone(r);
	clientThread.Resume();
	gSleepThread.Wait();
	User::SetJustInTime(EFalse);
	clientThread.Panic(_L("Panic temp file thread #4"),KErrGeneral);
	User::SetJustInTime(ETrue);
	CLOSE_AND_WAIT(clientThread);
	FsBarrier();
	r=TheFs.Delete(gLastTempFileName);
	test_Value(r, r == KErrNotFound);

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0808
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying RFile::Create() supports Delete On Close.
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test creates a file with DeleteOnClose flag set and
//!		then closes the file.
//!	2.	Test attempts to delete the file.
//!
//! @SYMTestExpectedResults
//! 	1.	The file creation should succeed.
//!	2.	The file deletion should fail with KErrNotFound.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Create EDeleteOnClose behaviour"));
 	r=file.Create(TheFs,_L("DoC5"),EFileRead|EFileWrite|EDeleteOnClose);
	test_KErrNone(r);
	file.Close();
	r=TheFs.Delete(filename);
	test_Value(r, r == KErrNotFound);

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0809
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying Delete On Close with multiple subsessions.
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test creates a file with DeleteOnClose and FileShareAny flags
//!		set, opens the file a second time with the FileShareAny flag set
//!		and then closes the first file handle.
//!	2.	Test attempts to delete the file.
//!	3.	The second file handle is closed and the test attempts to delete
//!		the file.
//!
//! @SYMTestExpectedResults
//! 	1.	The file create and file open should succeed.
//!	2.	The file deletion should fail with KErrInUse.
//!	3.	The file deletion should fail with KErrNotFound.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("DoC 6 - Multiple subsessions"));
 	r=file.Create(TheFs,filename,EFileShareAny|EFileRead|EFileWrite|EDeleteOnClose);
	test_KErrNone(r);
 	r=file2.Open(TheFs,filename,EFileShareAny|EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
	test_KErrNone(r);
	r=TheFs.Delete(filename);
	test_Value(r, r == KErrInUse);
	file2.Close();
	r=TheFs.Delete(filename);
	test_Value(r, r == KErrNotFound);

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0810
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying Delete On Close with preexisting file.
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//! 	1.	Test creates and closes a file, then attempts to create the same
//!		file with Delete on Close set.
//!
//! @SYMTestExpectedResults
//!	1.	The second create should fail with KErrAlreadyExists.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Create existing file behaviour"));
 	r=file.Create(TheFs,filename,EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();
 	r=file.Create(TheFs,filename,EFileRead|EFileWrite|EDeleteOnClose);
	test_Value(r, r == KErrAlreadyExists);

//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID	PBASE-t_file-0811
//! @SYMTestType	UT
//! @SYMTestCaseDesc	Verifying existing file cannot be opened with delete on close set.
//! @SYMPREQ		CR1266
//! @SYMTestPriority	High
//! @SYMTestActions
//!	1.	Test attempts to open an existing file with delete on close set.
//!
//! @SYMTestExpectedResults
//!	1.	The open should fail with KErrArgument.
//---------------------------------------------------------------------------------------------------------------------
	test.Next(_L("RFile::Open EDeleteOnClose flag validation"));
	r=file.Open(TheFs,filename,EFileRead|EFileWrite|EDeleteOnClose);
	test_Value(r, r == KErrArgument);
	r=TheFs.Delete(filename);
	test_KErrNone(r);

	gSleepThread.Close();
	test.End();
	}

//--------------------------------------------------------------
/**
    Test that flushing dirty file cache does not affect file attributes and time.
    This test shall pass disregarding if there is file cache or not.

*/
void TestFileAttributesAndCacheFlushing()
{
    test.Next(_L("Test that file cache flushing does not affect the file attributes."));
    if(Is_Win32(TheFs, gDriveNum))
    {
        test.Printf(_L("This test doesn't work on Win32 FS, skipping!\n"));
        return;
    }

    _LIT(KFile, "\\file1");

    TInt    nRes;
    TEntry  entry;
    TheFs.Delete(KFile);

    //-- 1. create test file
    nRes = CreateEmptyFile(TheFs, KFile, 33);
    test_KErrNone(nRes);

    //-- 2. open it for write
    RFile file;
    nRes = file.Open(TheFs, KFile, EFileWrite);
    test_KErrNone(nRes);

    //-- 3. write a couple of bytes there. This must cause 'Archive' attribute set
    nRes = file.Write(0, _L8("a"));
    test_KErrNone(nRes);
    nRes = file.Write(10, _L8("b"));
    test_KErrNone(nRes);

    nRes = TheFs.Entry(KFile, entry);
    test_KErrNone(nRes);

    test(entry.IsArchive());  //-- 'A' attribute must be set.

    //-- the file cache (if present) is dirty now. Dirty data timer starts to tick.
    //-- 4. set new file attributes (w/o 'A') and creation time
    const TUint newAtt = KEntryAttSystem ;
    nRes = file.SetAtt(newAtt, ~newAtt & KEntryAttMaskSupported);
    test_KErrNone(nRes);

    TTime newTime;
    nRes = newTime.Set(_L("19970310:101809.000000"));
    test_KErrNone(nRes);
    nRes = file.SetModified(newTime);
    test_KErrNone(nRes);

    //-- 5. wait 5 seconds. file server shall flush dirty data during this period.
    User::After(5*K1Sec);

    //-- 6. check that attributes haven't chanded because of flush
    nRes = file.Flush(); //-- this will flush attributes to the media
    test_KErrNone(nRes);

    nRes = TheFs.Entry(KFile, entry);
    test_KErrNone(nRes);

    test(entry.iAtt == newAtt);
    test(entry.iModified.DateTime().Year() == 1997);
    test(entry.iModified.DateTime().Month() == 3);
    test(entry.iModified.DateTime().Day() == 10);

    //-- 7. write some data and ensure that 'A' attribute is set now and 'modified' time updated
    nRes = file.Write(12, _L8("c"));
    test_KErrNone(nRes);

    file.Close(); //-- this will flush attributes to the media

    nRes = TheFs.Entry(KFile, entry);
    test_KErrNone(nRes);
    test(entry.iAtt == (newAtt | KEntryAttArchive));
    test(entry.iModified.DateTime().Year() != 1997);



}

/**
    Testing access to the very last bytes in the maximal (for FAT32) file size.
    This test must pass regardless of write caching configuration.
*/
void TestMaxFileSize()
{
    test.Next(_L("test maximal file size on FAT32\n"));

    if(!Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This test requires FAT32! skipping.\n"));
        return;
    }

    TInt nRes;

    //-- check disk space, it shall be > 4G
    TVolumeInfo volInfo;
    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);

    const TUint32 KMaxFAT32FileSize = 0xFFFFFFFF; // 4GB-1

    if(volInfo.iFree <= KMaxFAT32FileSize)
    {
        test.Printf(_L("Not enough space for 4GB file! skipping.\n"));
        return;
    }

    _LIT(KFileName, "\\huge_file");
    TBuf8<10> buf(10);
    RFile64 file64;

    //-- 1. create 4GB-1 file
    //-- this file has enabled write caching by default
    test.Printf(_L("creating maximal length file, size = 0x%x\n"),KMaxFAT32FileSize);
    nRes = file64.Replace(TheFs, KFileName, EFileWrite);
    test_KErrNone(nRes);

    const TInt64 fileSize = KMaxFAT32FileSize;

    nRes = file64.SetSize(fileSize);
    test_KErrNone(nRes);

    test.Printf(_L("seeking to the file end...\n"));
    TInt64 filePos = 0;
    nRes = file64.Seek(ESeekEnd, filePos);
    test_KErrNone(nRes);


    test.Printf(_L("test writing to the last bytes of the file (rel pos addressing) \n"));

    //-- 1. writing using relative position
    filePos = -1;
    nRes = file64.Seek(ESeekEnd, filePos);
    test_KErrNone(nRes);
    test(filePos == fileSize-1);

    nRes = file64.Write(_L8("z")); //-- write 1 byte a pos 0xFFFFFFFE, this is the last allowed position of the FAT32 file
    test_KErrNone(nRes);

    nRes = file64.Write(_L8("x")); //-- write 1 byte a pos 0xFFFFFFFF, beyond the max. allowed file size, this shall fail
    test_Value(nRes, nRes == KErrNotSupported);

    nRes = file64.Flush();
    test_KErrNone(nRes);

    //-- 1.1 check the result by reading data using rel. pos
    filePos = -1;
    nRes = file64.Seek(ESeekEnd, filePos);
    test_KErrNone(nRes);
    test(filePos == fileSize-1);

    test.Printf(_L("reading 1 byte at pos: 0x%x\n"), filePos);
    nRes = file64.Read(buf, 1); //-- read 1 byte a pos 0xFFFFFFFE, this is the last allowed position of the FAT32 file
    test_KErrNone(nRes);
    test(buf.Length() == 1 && buf[0]=='z');

    nRes = file64.Read(buf, 1); //-- read 1 byte a pos 0xFFFFFFFF, beyond the max. allowed file size, this shall fail
    test_KErrNone(nRes);
    test(buf.Length() == 0);

    file64.Close();

    test.Printf(_L("test writing to the last bytes of the file (absolute pos addressing) \n"));
    //-- 2. writing using absolute position
    nRes = file64.Open(TheFs, KFileName, EFileWrite);
    test_KErrNone(nRes);

    filePos = fileSize-1;

    nRes = file64.Write(filePos-2, _L8("0"), 1); //-- write 1 byte a pos 0xFFFFFFFC
    test_KErrNone(nRes);

    nRes = file64.Write(filePos, _L8("a"), 1);   //-- write 1 byte a pos 0xFFFFFFFE, this is the last allowed position of the FAT32 file
    test_KErrNone(nRes);

    nRes = file64.Write(filePos+1, _L8("b"), 1); //-- write 1 byte a pos 0xFFFFFFFF, beyond the max. allowed file size, this shall fail
    test_Value(nRes, nRes == KErrNotSupported);

    nRes = file64.Flush();
    test_KErrNone(nRes);

    //-- 1.1 check the result by reading data absolute rel. position

    nRes = file64.Read(filePos-2, buf, 1); //-- read 1 byte a pos 0xFFFFFFFD
    test_KErrNone(nRes);
    test(buf.Length() == 1 && buf[0]=='0');

    nRes = file64.Read(filePos, buf, 1);   //-- read 1 byte a pos 0xFFFFFFFE, this is the last allowed position of the FAT32 file
    test_KErrNone(nRes);
    test(buf.Length() == 1 && buf[0]=='a');

    nRes = file64.Read(filePos+1, buf, 1);  //-- read 1 byte a pos 0xFFFFFFFF, beyond the max. allowed file size
    test_KErrNone(nRes);
    test(buf.Length() == 0);

    nRes = file64.Read(filePos+2, buf, 1); //buf.Len must be 0
    test_KErrNone(nRes);
    test(buf.Length() == 0);

    file64.Close();

    test.Printf(_L("deleting the huge file.\n"));
    nRes = TheFs.Delete(KFileName);
    test_KErrNone(nRes);

    test.Printf(_L("RFile64 is not supported! Skipping.\n"));


}


//--------------------------------------------------------------

void CallTestsL()
	{

    //-- set up console output
    F32_Test_Utils::SetConsole(test.Console());

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDriveNum);

    //-- FAT Supports short file names
    if(Is_Fat(TheFs, gDriveNum))
        gShortFileNamesSupported = ETrue;
    
    if(Is_Win32(TheFs, gDriveNum)) 
    	{//-- find out if this is NTFS and if it supports short names (this feature can be switched OFF)
        
        _LIT(KLongFN, "\\this is a long file name");
        nRes = CreateEmptyFile(TheFs, KLongFN, 10);   
        test_KErrNone(nRes);

	    TBuf<12> shortName;
	    nRes = TheFs.GetShortName(KLongFN, shortName);
	    gShortFileNamesSupported = (nRes == KErrNone);
        
        nRes = TheFs.Delete(KLongFN);
        test_KErrNone(nRes);

        DeleteTestDirectory();
    	}
    else
    	{
        nRes = FormatDrive(TheFs, gDriveNum, ETrue);
        test_KErrNone(nRes);
    	}

	CreateTestDirectory(_L("\\F32-TST\\TFILE\\"));

	testFileRename();
	testSetSize();
	CopyFileToTestDirectory();
	testFileSeek();
	testMoreFileSeek();
	CopyFileToTestDirectory();
	testFileText();
	testFileTextEndRecord();
	testDeleteOpenFiles();
	testFileAttributes();
	testFileNames();
	testShare();
	testReadersWriters();
	testReadFile();
	testMultipleReadFile();
	testWriteFile();
	testChangeMode();
	testShortNameAccessorFunctions();
	testIsFileOpen();
	testMiscellaneousReportedBugs();
	testIsRomAddress();
	TestFileUids();
	TestMaxLengthFilenames();
	testINC070455();
	TestINC112803();
	testZeroLengthDescriptors();
	testNegativeLengthToWrite();
	testNegativeLength();
	testReadBufferOverflow();
	TestDeleteOnClose();
    TestFileAttributesAndCacheFlushing();
    TestMaxFileSize();

	DeleteTestDirectory();
	}

