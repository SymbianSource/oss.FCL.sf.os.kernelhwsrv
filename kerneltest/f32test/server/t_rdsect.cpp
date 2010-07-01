// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_rdsect.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>

#include "t_server.h"
#include "t_chlffs.h"

#if defined(__WINS__)
#define WIN32_LEAN_AND_MEAN
#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (default:4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

GLDEF_D RTest test(_L("T_RDSECT"));


TBuf<26> alphaBuffer=_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
TBuf<10> numberBuffer=_L("0123456789");
TBuf<14> humptyBuffer=_L("Humpty-Dumpty!");
TPtr8 alphaPtr((TText8*)alphaBuffer.Ptr(),alphaBuffer.Size(),alphaBuffer.Size());
TPtr8 numberPtr((TText8*)numberBuffer.Ptr(),numberBuffer.Size(),numberBuffer.Size());
TPtr8 humptyPtr((TText8*)humptyBuffer.Ptr(),humptyBuffer.Size(),humptyBuffer.Size());


/*

  What this test is for:

  Tests the implementation of RFs::ReadFileSection() which has been designed
  to allow reading from a file regardless of its lock state.  
  Under the EPOC FAT filesystem, the function accesses the raw file data without 
  opening the file using a share mode.  This is obviously a dangerous activity
  should the file be currently open for writing by another user - but the file
  server makes no guarantees as to the data it returns!  The function allows the
  caller to specify the starting position in the file from which to read the data
  and the length of data required.
  
  This test creates a number of files and tests the use of the function when the 
  files are closed and when they are open in a number of access modes.  
  It also tests reading a UID from the files using this new function.

*/

LOCAL_C void CreateTestFiles()
//
// Create files with uids for testing
//
	{

	test.Next(_L("Create test files"));
	TInt r=TheFs.MkDir(_L("\\F32-TST\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	RFile file;

//	Create \\gSessionPath\\UIDCHK.BLG - with uid no data
	r=file.Replace(TheFs,_L("\\F32-TST\\UIDCHK.BLG"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType(TUid::Uid('U'),TUid::Uid('I'),TUid::Uid('D'));
	TCheckedUid checkedUid(uidType);
	TPtrC8 buf((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	file.Close();

//	Create \\gSessionPath\\UIDCHK.MSG - with uid and data
	r=file.Replace(TheFs,_L("\\F32-TST\\UIDCHK.MSG"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType2(TUid::Uid('X'),TUid::Uid('Y'),TUid::Uid('Z'));
	checkedUid.Set(uidType2);
	buf.Set((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	r=file.Write(_L8("More file data"));
	test_KErrNone(r);
	file.Close();
	
//	Create \\gSessionPath\\UIDCHK.DAT - uid stored only in the file
	r=file.Replace(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType3(TUid::Uid('D'),TUid::Uid('A'),TUid::Uid('T'));
	checkedUid.Set(uidType3);
	buf.Set((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	r=file.Write(_L8("More file data"));
	test_KErrNone(r);
	file.Close();

//	Make a few random files - these will be deleted before the test begins
//	but are necessary to try to split file ReadFileSection into non-contiguous clusters
	
	r=file.Replace(TheFs,_L("\\F32-TST\\Temp1.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(550);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp2.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp3.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();
	
	r=file.Replace(TheFs,_L("\\F32-TST\\Temp4.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(550);
	file.Close();
	
	r=file.Replace(TheFs,_L("\\F32-TST\\Temp5.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp6.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp7.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp8.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp9.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	r=file.Replace(TheFs,_L("\\F32-TST\\Temp10.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.SetSize(256);
	file.Close();

	TheFs.Delete(_L("\\F32-TST\\Temp2.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp4.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp6.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp8.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp10.txt"));
	
	r=file.Replace(TheFs,_L("\\F32-TST\\ReadFileSection1.txt"),EFileRead|EFileWrite);
	test_KErrNone(r);

//	Write 5000 bytes of nonsense
	
	TInt i=0;
	for ( ; i<100; i++)
		{
		r=file.Write(alphaPtr);
		test_KErrNone(r);
		r=file.Write(numberPtr);
		test_KErrNone(r);
		r=file.Write(humptyPtr);
		test_KErrNone(r);
		}
	
	file.Close();

	TheFs.Delete(_L("\\F32-TST\\Temp1.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp3.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp5.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp7.txt"));
	TheFs.Delete(_L("\\F32-TST\\Temp9.txt"));
	}

#if !defined(_UNICODE)

LOCAL_C void Test1()
//
// Test RFs::ReadFileSection()
//
	{

	test.Next(_L("Use RFs::ReadFileSection() to read from a file"));

//	First, test for non-existant file
	TBuf<256> testDes;
	TInt r=TheFs.ReadFileSection(_L("\\F32-tst\\NonExistantFile.txt"),0,testDes,52);
	test_Value(r, r == KErrNotFound);
	test(testDes.Length()==0);

//	Test with file closed
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,testDes,26);
	test_KErrNone(r);
	test(testDes.Length()==26);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),26,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),36,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);

//	Test with file open	EFileShareAny|EFileRead
	RFile file;
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileRead);
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),50,testDes,26);
	test_KErrNone(r);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),76,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),86,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
	file.Close();

//	Test with file open	EFileShareExclusive|EFileRead
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareExclusive|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),100,testDes,26);
	test_KErrNone(r);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),126,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),136,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
	file.Close();
	
//	Test with file open	EFileShareExclusive|EFileWrite
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareExclusive|EFileWrite);	
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),150,testDes,26);
	test_KErrNone(r);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),176,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),186,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
	file.Close();
	

//	Test with file open	EFileShareReadersOnly|EFileRead
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareReadersOnly|EFileRead);
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),200,testDes,26);
	test_KErrNone(r);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),226,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),236,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
	file.Close();

//	Test with several accesses to a file EFileShareAny|EFileWrite
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);

	RFile secondFile;
	r=secondFile.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	RFile thirdFile;
	r=thirdFile.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),250,testDes,26);
	test_KErrNone(r);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),276,testDes,10);
	test_KErrNone(r);
	test(testDes==_L("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),286,testDes,14);
	test_KErrNone(r);
	test(testDes==_L("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
//	Test across potential cluster boundaries

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),500,testDes,100);
	test_KErrNone(r);
	test(testDes.Length()==100);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!"));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),1000,testDes,100);
	test_KErrNone(r);
	test(testDes.Length()==100);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!"));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),1500,testDes,100);
	test_KErrNone(r);
	test(testDes.Length()==100);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!"));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),2000,testDes,100);
	test_KErrNone(r);
	test(testDes.Length()==100);
	test(testDes==_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789Humpty-Dumpty!"));

	file.Close();
	secondFile.Close();
	thirdFile.Close();
	}


LOCAL_C void Test2()
//
// Test RFs::ReadFileSection() on UID reads
//
	{

	
	test.Next(_L("Use RFs::ReadFileSection() to read UIDs from files"));

	TBuf8<sizeof(TCheckedUid)> uidBuf(sizeof(TCheckedUid));
	TPtr uidPtr((TText*)uidBuf.Ptr(),sizeof(TCheckedUid),sizeof(TCheckedUid));
	
	TInt r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.BLG"),0,uidPtr,sizeof(TCheckedUid));
	test_KErrNone(r);
	TCheckedUid uid(uidBuf);
	TUidType uidType=uid.UidType();
	test(uidType.IsValid());
	
	test(uidType[0]==TUid::Uid('U') && uidType[1]==TUid::Uid('I') && uidType[2]==TUid::Uid('D'));

	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.MSG"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('X') && uidType[1]==TUid::Uid('Y') && uidType[2]==TUid::Uid('Z'));
	
	
	// Test for Null File length
	TBuf8<256> testDesN;
	test.Next(_L("Check for null file name"));
 	r=TheFs.ReadFileSection(_L(""),0,testDesN,26);
 	test_Value(r, r == KErrBadName);
 	
	// Check the lentgh of descriptor.	
 	TInt x = testDesN.Length();
 	test ( x == 0);
 	
 	test.Next(_L("Check for non existing file"));
	r=TheFs.ReadFileSection(_L("sdfsd.dfg"),0,testDesN,26);
 	test.Printf(_L("Return %d"),r);
 	test_Value(r, (r == KErrNotFound) || (r == KErrPathNotFound));
 	
	// Check the lentgh of descriptor.	
 	x = testDesN.Length();
	test ( x == 0);
	
	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));
	

	}

LOCAL_C void Test3()
//
//	Test uid's can be read when the file is open
//
//	EFileShareExclusive,EFileShareReadersOnly,EFileShareAny,
//	EFileStream=0,EFileStreamText=0x100,
//	EFileRead=0,EFileWrite=0x200
//
	{

	test.Next(_L("Test that UIDs can be read from open files"));
	RFile file;
	
//	Test with file open	EFileShareExclusive|EFileRead	
	TInt r=file.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareExclusive|EFileRead);
	test_KErrNone(r);

	TBuf8<sizeof(TCheckedUid)> uidBuf;
	
	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	
	TCheckedUid uid(uidBuf);
	TUidType uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));

	file.Close();

//	Test with file open	EFileShareExclusive|EFileWrite	
	r=file.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareExclusive|EFileWrite);
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));

	file.Close();

//	Test with file open	EFileShareReadersOnly|EFileRead		
	r=file.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareReadersOnly|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));
	
	file.Close();


//	Test with file open	EFileShareAny|EFileRead		
	r=file.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareAny|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));
	
	file.Close();

//	Test with several accesses to file open	EFileShareAny|EFileWrite			
	r=file.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	RFile secondFile;
	r=secondFile.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	RFile thirdFile;
	r=thirdFile.Open(TheFs,_L("\\F32-TST\\UIDCHK.DAT"),EFileShareAny|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-TST\\UIDCHK.DAT"),0,uidBuf,sizeof(TCheckedUid));
	test_KErrNone(r);
	uid.Set(uidBuf);
	uidType=uid.UidType();
	test(uidType.IsValid());
	test(uidType[0]==TUid::Uid('D') && uidType[1]==TUid::Uid('A') && uidType[2]==TUid::Uid('T'));
	
	file.Close();
	secondFile.Close();
	thirdFile.Close();

	}


LOCAL_C void TestErrors()
//
//	Test errors and boundary conditions
//
	{

	test.Next(_L("Test Error handling"));

//	Test that specifying a zero length section returns a zero length descriptor
	TBuf<30> testDes;
	TInt r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),25,testDes,0);
	test_KErrNone(r);
	test(testDes.Length()==0);
	
//	Test that specifying a negative starting position causes a panic
//	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),-1,testDes,10);
//	This will panic: See RFs::ReadFileSection() code - relevant lines are
//	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
//	Test that specifying a section of greater length than the descriptor to
//	hold the data will cause a panic
//	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,testDes,45);
//	This will panic: See RFs::ReadFileSection() code - relevant lines are	
//	__ASSERT_ALWAYS(aDes.MaxLength()>=aLength,Panic(EBadLength));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),2000,testDes,-20);
	test_Value(r, r == KErrArgument);

//	Test that specifying a position and length which extends beyond the end of
//	the	file returns a zero length descriptor and KErrNone

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),4993,testDes,20);
	test_KErrNone(r);	
	test(testDes.Length()==7);
	test(testDes==_L("Dumpty!"));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),4999,testDes,1);
	test_KErrNone(r);	
	test(testDes.Length()==1);
	test(testDes==_L("!"));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),5000,testDes,1);
	test_KErrNone(r);	
	test(testDes.Length()==0);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),5550,testDes,20);
	test_KErrNone(r);	
	test(testDes.Length()==0);

//	Test reading the whole file	
	HBufC* hDes=HBufC::New(5002);
	if (!hDes)
		User::Leave(KErrNoMemory);
	TPtr pDes=hDes->Des();

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,5000);
	test_KErrNone(r);	
	test(pDes.Length()==5000);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,5000);
	test_KErrNone(r);	
	test(pDes.Length()==5000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,5002);
	test_KErrNone(r);	
	test(pDes.Length()==5000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),2000,pDes,3000);
	test_KErrNone(r);	
	test(pDes.Length()==3000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),2000,pDes,4002);
	test_KErrNone(r);	
	test(pDes.Length()==3000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),5000,pDes,5002);
	test_KErrNone(r);	
	test(pDes.Length()==0);
	
	delete hDes;
	}

#else


//	BEGINNING OF UNICODE TESTS

LOCAL_C void TestUnicode()
//
// Test RFs::ReadFileSection()
//
	{
	test.Next(_L("Use RFs::ReadFileSection() to read from a file"));

	//	First, test for non-existant file
	TBuf8<256> testDes;
	TInt r=TheFs.ReadFileSection(_L("\\F32-tst\\NonExistantFile.txt"),0,testDes,52);
	test_Value(r, r == KErrNotFound);
	test(testDes.Length()==0);
	
	//	Test with file closed
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,testDes,52);
	test_KErrNone(r);
	test(testDes.Length()==52);
	test(testDes==alphaPtr);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),52,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),72,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
    //  Test for Null File length
	TBuf8<256> testDesN;
	test.Next(_L("Check for null file name"));
 	r=TheFs.ReadFileSection(_L(""),0,testDesN,26);
 	test_Value(r, r == KErrBadName);
 	
	//  Check the lentgh of descriptor.	
 	TInt x = testDesN.Length();
 	test ( x == 0);
 	
 	test.Next(_L("Check for non existing file"));
	r=TheFs.ReadFileSection(_L("sdfsd.dfg"),0,testDesN,26);
 	test.Printf(_L("Return %d"),r);
 	test_Value(r, (r == KErrNotFound) || (r == KErrPathNotFound));
 	
	//  Check the lentgh of descriptor.	
 	x = testDesN.Length();
	test ( x == 0);

    //  Test for Empty directory  
    r=TheFs.ReadFileSection(_L("\\F32-tst\\"),0,testDesN,52);
    test_Value(r, r == KErrBadName);
    test(testDesN.Length()==0);

    //  Test for File with wildcard name 
    r=TheFs.ReadFileSection(_L("\\F32-tst\\*.txt"),0,testDesN,52);
    test_Value(r, r == KErrBadName);
    test(testDesN.Length()==0);

    //  Test for Folder with wildcard name 
    r=TheFs.ReadFileSection(_L("\\F32-tst*\\ReadFileSection1.txt"),0,testDesN,52);
    test_Value(r, r == KErrBadName);
    test(testDesN.Length()==0);
      
    //  Test for Root directory
    r=TheFs.ReadFileSection(_L("\\"),0,testDesN,52);
    test_Value(r, r == KErrBadName);
    test(testDesN.Length()==0);

    //  Test for no file being specified.
    r=TheFs.ReadFileSection(_L(""),0,testDesN,26);
    test_Value(r, r == KErrBadName);
    test(testDesN.Length()==0); 


    // Test with file open	EFileShareAny|EFileRead
	RFile file;
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileRead);
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),100,testDes,52);
	test_KErrNone(r);
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),152,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),172,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
	file.Close();

//	Test with file open	EFileShareExclusive|EFileRead
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareExclusive|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),200,testDes,52);
	test_KErrNone(r);
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),252,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),272,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
	file.Close();
	
//	Test with file open	EFileShareExclusive|EFileWrite
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareExclusive|EFileWrite);	
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),300,testDes,52);
	test_KErrNone(r);
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),352,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),372,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
	file.Close();
	

//	Test with file open	EFileShareReadersOnly|EFileRead
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareReadersOnly|EFileRead);
	test_KErrNone(r);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),400,testDes,52);
	test_KErrNone(r);
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),452,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),472,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
	file.Close();

//	Test with several accesses to a file EFileShareAny|EFileWrite
	r=file.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);

	RFile secondFile;
	r=secondFile.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	RFile thirdFile;
	r=thirdFile.Open(TheFs,_L("\\F32-tst\\ReadFileSection1.txt"),EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),500,testDes,52);
	test_KErrNone(r);

#if defined(__WINS__)
#if defined(_DEBUG)
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
#endif
#else
	test(testDes==alphaPtr);
	test(testDes.Length()==52);
#endif
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),552,testDes,20);
	test_KErrNone(r);
	test(testDes==numberPtr);
	test(testDes.Length()==20);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),572,testDes,28);
	test_KErrNone(r);
	test(testDes==humptyPtr);
	test(testDes.Length()==28);
	
//	Test across potential cluster boundaries

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),1000,testDes,200);
	test_KErrNone(r);
	test(testDes.Length()==200);
	TBuf8<200> amalgam;
	TInt i=0;
	for (; i<2; i++)
		{
		amalgam.Append(alphaPtr);
		amalgam.Append(numberPtr);
		amalgam.Append(humptyPtr);
		}

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),1000,testDes,200);
	test_KErrNone(r);
	test(testDes.Length()==200);
	test(testDes==amalgam);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),3000,testDes,200);
	test_KErrNone(r);
	test(testDes.Length()==200);
	test(testDes==amalgam);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),4000,testDes,200);
	test_KErrNone(r);
	test(testDes.Length()==200);
	test(testDes==amalgam);

	file.Close();
	secondFile.Close();
	thirdFile.Close();

//	Test errors and boundary conditions
	test.Next(_L("Test Error handling"));

//	Test that specifying a zero length section returns a zero length descriptor
	TBuf8<30> testDes2;
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),50,testDes2,0);
	test_KErrNone(r);
	test(testDes2.Length()==0);
	
//	Test that specifying a negative starting position causes a panic
//	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),-1,testDes2,10);
//	This will panic: See RFs::ReadFileSection() code - relevant lines are
//	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
//	Test that specifying a section of greater length than the descriptor to
//	hold the data will cause a panic
//	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,testDes2,45);
//	This will panic: See RFs::ReadFileSection() code - relevant lines are	
//	__ASSERT_ALWAYS(aDes.MaxLength()>=aLength,Panic(EBadLength));


	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),2000,testDes2,-20);
	test_Value(r, r == KErrArgument);

//	Test that specifying a position and length which extends beyond the end of
//	the	file returns a zero length descriptor and KErrNone

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),9993,testDes2,30);
	test_KErrNone(r);	
	test(testDes2.Length()==7);
	test(testDes2==humptyPtr.Right(7));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),9999,testDes2,1);
	test_KErrNone(r);	
	test(testDes2.Length()==1);
	test(testDes2==humptyPtr.Right(1));

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),10000,testDes2,1);
	test_KErrNone(r);	
	test(testDes2.Length()==0);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),10550,testDes2,20);
	test_KErrNone(r);	
	test(testDes2.Length()==0);

//	Test reading the whole file	
	HBufC8* hDes=HBufC8::New(10002);
	if (!hDes)
		User::Leave(KErrNoMemory);
	TPtr8 pDes=hDes->Des();

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,10000);
	test_KErrNone(r);	
	test(pDes.Length()==10000);
	
	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,10000);
	test_KErrNone(r);	
	test(pDes.Length()==10000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),0,pDes,10002);
	test_KErrNone(r);	
	test(pDes.Length()==10000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),4000,pDes,6000);
	test_KErrNone(r);	
	test(pDes.Length()==6000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),4000,pDes,8002);
	test_KErrNone(r);	
	test(pDes.Length()==6000);

	r=TheFs.ReadFileSection(_L("\\F32-tst\\ReadFileSection1.txt"),10000,pDes,10002);
	test_KErrNone(r);	
	test(pDes.Length()==0);
	
	delete hDes;
	}
	
#endif


LOCAL_C void TestZ()
//
// Test Rom filesystem
//
	{
	test.Next(_L("Use RFs::ReadFileSection() to read from a file on the ROM"));
#if defined (__WINS__)
//	Requires a copy of t_rdsect.txt in z directory (\EPOC32\RELEASE\WINS\BUILD\Z\TEST)
//	Initially, test with file closed
	TBuf8<256> testDes;
	TBuf8<27> temp1;
	TInt r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),0,temp1,26);
	test_KErrNone(r);
	test(temp1.Length()==26);
	test(temp1==_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	
	TBuf8<11> temp2;
	r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),26,temp2,10);
	test_KErrNone(r);
	test(temp2==_L8("0123456789"));
	test(temp2.Length()==10);

	r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),36,testDes,14);
	test_KErrNone(r);
	test(testDes==_L8("Humpty-Dumpty!"));
	test(testDes.Length()==14);

//	Test with file open	EFileShareAny|EFileRead
	RFile file;
	r=file.Open(TheFs,_L("Z:\\test\\t_rdsect.txt"),EFileShareAny|EFileRead);
	test_KErrNone(r);
	
	r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),50,testDes,26);
	test_KErrNone(r);
	test(testDes==_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	test(testDes.Length()==26);
	
	r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),76,testDes,10);
	test_KErrNone(r);
	test(testDes==_L8("0123456789"));
	test(testDes.Length()==10);

	r=TheFs.ReadFileSection(_L("Z:\\test\\t_rdsect.txt"),86,testDes,14);
	test_KErrNone(r);
	test(testDes==_L8("Humpty-Dumpty!"));
	test(testDes.Length()==14);
	
	file.Close();
#else
//	Test for MARM builds - oby file puts file in ROM (z:\test\)
//	The file is the ASCII version

	test.Next(_L("read small descriptor\n"));	
	TBuf8<256> testDes;
	TBuf8<27> temp1;
	TInt r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),0,temp1,26);
	test_KErrNone(r);
	test(temp1.Length()==26);	
	test(temp1==_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	
	TBuf8<11> temp2;
	r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),26,temp2,10);
	test_KErrNone(r);
	test(temp2.Length()==10);
#if !defined (UNICODE)
	test(testDes==_L8("0123456789"));
#endif
	r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),36,testDes,14);
	test_KErrNone(r);
	test(testDes.Length()==14);
#if !defined (UNICODE)	
	test(testDes==_L8("Humpty-Dumpty!"));
#endif
	r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),50,testDes,26);
	test_KErrNone(r);
	test(testDes.Length()==26);
#if !defined (UNICODE)	
	test(testDes==_L8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
#endif
	r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),76,testDes,10);
	test_KErrNone(r);
	test(testDes.Length()==10);
#if !defined (UNICODE)	
	test(testDes==_L8("0123456789"));
#endif
	r=TheFs.ReadFileSection(_L("Z:\\test\\T_RDSECT.txt"),86,testDes,14);
	test_KErrNone(r);
	test(testDes.Length()==14);
#if !defined (UNICODE)	
	test(testDes==_L8("Humpty-Dumpty!"));
#endif
#endif
	}

LOCAL_C void DoTestsL()
//
// Do all tests
//
	{
	CreateTestFiles();
#if !defined (_UNICODE)
	Test1();
	Test2();
	Test3();
	TestErrors();
#else
	TestUnicode();
#endif
	TestZ();
	}

GLDEF_C void CallTestsL(void)
//
// Test formatting
//
    {

	test.Title();
	test.Start(_L("Testing filesystem on default drive"));

	TChar driveLetter;
	if (IsSessionDriveLFFS(TheFs,driveLetter))
		{
		test.Printf(_L("CallTestsL: Skipped: test does not run on LFFS.\n"));
		return;
		}	

	DoTestsL();
	CheckDisk();

	test.End();
	test.Close();
	return;
    }

